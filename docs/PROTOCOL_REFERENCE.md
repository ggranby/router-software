# Hornet Link — Protocol Reference

Complete wire-level reference for all protocols used in the Hornet Link system.

> **Protocol v1 (DCS-BIOS based).** A Hornet-native protocol v2 (COBS
> framing, polled bus, named and typed controls, no DCS-BIOS needed) is
> specified in [docs/PROTOCOL_V2.md](PROTOCOL_V2.md). v1 stays supported
> alongside v2 until v2 has been checked on real panels.

---

## 1. DCS-BIOS Export Protocol

Used on the **primary link** (PC ↔ master COM port) to push simulator state to panels.

### 1.1 Frame Structure

```
[sync: 0x55 0x55 0x55 0x55]
[write record 1]
[write record 2]
...
```

One frame begins with exactly one 4-byte sync word followed by zero or more write records.

### 1.2 Write Record

```
[addr_lo : 1] [addr_hi : 1] [len_lo : 1] [len_hi : 1] [data : len bytes]
```

| Field    | Description                                              |
|----------|----------------------------------------------------------|
| addr     | Even byte address (0x0000–0xFFFE) into the state space   |
| len      | Number of data bytes (always even; one word = 2 bytes)   |
| data     | Raw 16-bit little-endian words                           |

Consecutive addresses with changed values are merged into a single record (longer len).

### 1.3 State Space

64 KiB flat array (`BiosStateMap::raw()`).  Every address is a 16-bit word at
an even byte offset.  The parser accumulates writes frame-by-frame; the dirty list
after each sync word contains all addresses written in the preceding frame.

### 1.4 Byte Order

All multi-byte integers are **little-endian**.

---

## 2. Device Handshake Protocol

Occurs once per session when the PC opens a COM port.

### 2.1 Frame Magic

All handshake frames begin with `AA DE AD`.

### 2.2 Probe (Ping) — PC → Device

```
AA DE AD 01
```

Sent immediately after the port is opened.  The device must reply within 300 ms
or it is classified as a **legacy** device (receives full unfiltered stream).

### 2.3 Capability Response (Pong) — Device → PC

```
AA DE AD 02
[flags    : 1]
[name_len : 1]
[name     : name_len bytes]
[sub_count_lo : 1]
[sub_count_hi : 1]
[subscription × sub_count]
(if flags & 0x01: slave list — see §2.5)
```

#### Subscription Entry (5 bytes)

```
[addr_lo : 1] [addr_hi : 1] [mask_lo : 1] [mask_hi : 1] [shift : 1]
```

A wildcard entry (`addr = 0xFFFF, mask = 0xFFFF, shift = 0`) means "send everything".

#### Flags Byte

| Bit | Value | Meaning                           |
|-----|-------|-----------------------------------|
|  0  | 0x01  | RS-485 Master                     |
|  1  | 0x02  | RS-485 Slave                      |
|  2  | 0x04  | Bidirectional (can send imports)   |

### 2.4 Acknowledgement (Ack) — PC → Device

```
AA DE AD 03
```

Sent by the PC after successfully parsing the pong frame.  From this point the
bridge begins sending DCS-BIOS delta frames.

### 2.5 RS-485 Master Slave List

Appended to the pong frame immediately after subscription entries when `flags & 0x01`:

```
[slave_count : 1]
For each slave:
  [slave_addr     : 1]
  [slave_name_len : 1]
  [slave_name     : slave_name_len bytes]
  [slave_sub_count_lo : 1]
  [slave_sub_count_hi : 1]
  [subscription × slave_sub_count]
```

---

## 3. Operating-Mode Frames

### 3.1 Mode Push — PC → Device

```
AA DE AD 04 [mode : 1]
```

| Mode | Value | Description                                         |
|------|-------|-----------------------------------------------------|
| Sim  | 0x00  | Live DCS data; imports forwarded to DCS             |
| Preflight | 0x01 | BIT / lamp test; imports NOT forwarded         |
| Maintenance | 0x02 | Panel self-check; imports NOT forwarded      |

### 3.2 Mode Acknowledge — Device → PC

```
AA DE AD 05 [local_mode : 1]
```

---

## 4. Import Commands — Device → DCS

Bidirectional devices (flags & 0x04) send ASCII lines:

```
SET <CONTROL_NAME> <VALUE>\n
```

Examples:
```
SET UFC_KEY_1 1\n
SET MASTER_ARM_SW 2\n
```

The bridge receives these via `ImportLineParser`, verifies the bridge is in Sim mode,
then forwards them as UDP datagrams to DCS (`127.0.0.1:7778`).

---

## 5. RS-485 Sub-Bus Protocol

This section specifies the RS-485 sub-bus protocol used between an RS-485 **master**
Arduino board and its **slave** panel boards in the Hornet Link system (Protocol v1).

### 5.1 Overview

The primary serial link (USB-CDC, 500 000 baud) carries full DCS-BIOS frames between
the PC bridge (`hornet-link.exe`) and the master board. The RS-485 sub-bus runs at
**250 000 baud, 8N1** and carries:

- DCS-BIOS delta frames from master → slave (filtered to each slave's subscriptions)
- Import command lines from slave → master → PC → DCS
- Probe / keep-alive frames (bus enumeration)
- Operating-mode frames (Sim / Preflight / Maintenance)

### 5.2 Physical Layer

| Parameter       | Value                                |
|-----------------|--------------------------------------|
| Baud rate       | 250 000 baud                         |
| Data format     | 8N1 (8 data bits, no parity, 1 stop) |
| Transceiver     | MAX485 / MAX3485 (or equivalent)     |
| Bus topology    | Half-duplex multi-drop               |
| Direction pin   | Single GPIO → /RE + DE tied together |
| Maximum nodes   | 1 master + up to 16 slaves           |
| Maximum cable   | 1200 m @ 250 kbaud (rule of thumb)   |

### 5.3 Frame Format

Every RS-485 sub-bus frame has the following structure:

```
[STX : 1] [DST : 1] [SRC : 1] [LEN_LO : 1] [LEN_HI : 1]
[PAYLOAD : LEN bytes]
[CRC_LO : 1] [CRC_HI : 1]
```

| Field    | Size | Description                                                   |
|----------|------|---------------------------------------------------------------|
| STX      | 1    | Start byte, always `0xFE`                                     |
| DST      | 1    | Destination bus address (1–254); `0x00` = broadcast           |
| SRC      | 1    | Source bus address; `0x00` = master                           |
| LEN_LO   | 1    | Low byte of payload length (little-endian)                    |
| LEN_HI   | 1    | High byte of payload length                                   |
| PAYLOAD  | N    | Message-type byte followed by message body                    |
| CRC_LO   | 1    | Low byte of CRC-16/CCITT-FALSE over [DST..last payload byte]  |
| CRC_HI   | 1    | High byte of CRC                                              |

#### CRC Algorithm

**CRC-16/CCITT-FALSE** (Kermit / CCITT variant):

| Parameter    | Value  |
|--------------|--------|
| Polynomial   | 0x1021 |
| Initial      | 0xFFFF |
| Input refl.  | No     |
| Output refl. | No     |
| Final XOR    | 0x0000 |

CRC is computed over all bytes from DST through the last payload byte (inclusive).
The STX byte is excluded from the CRC.

### 5.4 Bus Addresses

| Address    | Assigned to                             |
|------------|-----------------------------------------|
| 0x00       | Broadcast — all nodes must process      |
| 0x01–0xFE  | Slave panels (user-assigned per device) |
| Master     | Always source address 0x00              |

### 5.5 Message Types

The first byte of every PAYLOAD field is the **message type**.

#### Master → Slave

| Type byte | Constant       | Description                                     |
|-----------|----------------|-------------------------------------------------|
| `0x10`    | `kRS485_PROBE` | Probe / keep-alive — "are you there?"           |
| `0x20`    | `kRS485_DATA`  | DCS-BIOS delta frame (filtered to slave's subs) |
| `0x40`    | `kRS485_MODE`  | Operating-mode change `[type][mode_value]`      |

#### Slave → Master

| Type byte | Constant           | Description                                  |
|-----------|--------------------|----------------------------------------------|
| `0x11`    | `kRS485_PROBE_ACK` | Probe acknowledgement — "I am here"          |
| `0x30`    | `kRS485_IMPORT`    | Import command line (ASCII, `\n` terminated) |
| `0x41`    | `kRS485_MODE_ACK`  | Mode acknowledgement `[type][local_mode]`    |

### 5.6 Message Payloads

#### Probe (`0x10`)

```
[0x10]
```

No body bytes. Sent by master periodically (default every 20 ms) to confirm each
slave is still alive. If a slave does not reply within 10 ms it is removed from
the active slave table.

#### Probe Ack (`0x11`)

```
[0x11]
```

No body bytes. Sent by slave in response to a Probe.

#### DCS-BIOS Delta Data (`0x20`)

```
[0x20] [sync: 0x55 0x55 0x55 0x55] [write records...]
```

The payload after the type byte is a complete DCS-BIOS export frame:

```
[sync: 0x55 0x55 0x55 0x55]
[addr_lo addr_hi len_lo len_hi data...]  (one or more records)
```

Multiple consecutive addresses may be merged into a single write record
(length > 2).

#### Import Command (`0x30`)

```
[0x30] [ASCII command line terminated with 0x0A]
```

Example payload (hex):
```
30 53 45 54 20 55 46 43 5F 4B 45 59 5F 31 20 31 0A
^  |<--- "SET UFC_KEY_1 1\n" in ASCII --->|
```

The master strips the `0x30` type byte and forwards the ASCII line verbatim to
the PC via USB-CDC.

#### Mode Change (`0x40`)

```
[0x40] [mode : 1]
```

| Mode byte | Meaning     |
|-----------|-------------|
| `0x00`    | Sim         |
| `0x01`    | Preflight   |
| `0x02`    | Maintenance |

#### Mode Ack (`0x41`)

```
[0x41] [local_mode : 1]
```

Slave echoes back the mode it has applied locally.

### 5.7 Timing Requirements

| Parameter                 | Value     | Notes                                           |
|---------------------------|-----------|-------------------------------------------------|
| Slave reply timeout       | 10 ms     | After a Probe or Data frame                     |
| Keep-alive poll interval  | 20 ms     | Master polls each slave in round-robin          |
| New-slave probe timeout   | 300 ms    | Declares a candidate address absent if no reply |
| Direction-pin settle time | 10 µs     | Before asserting TX; after de-asserting TX      |
| Maximum payload size      | 512 bytes | Larger frames are silently discarded            |

### 5.8 Error Handling

| Condition                              | Master action                           |
|----------------------------------------|-----------------------------------------|
| Slave does not reply to Probe in 10 ms | Remove from active table; log warning   |
| CRC mismatch on received frame         | Discard frame; increment error counter  |
| Payload length > 512 bytes             | Discard frame; log error                |
| Unknown message type byte              | Discard and continue                    |

### 5.9 Reference Implementation

- **C++ (PC bridge):** `RS485ProtocolSpec.hpp` — CRC function, frame encode/verify, constants.
- **C++ (PC bridge):** `DeviceRegistry.hpp` — `HandshakeParser` including slave-list parsing.
- **Arduino (master):** `libraries/HornetLink/src/HornetLinkMaster.h`
- **Arduino (slave):**  `libraries/HornetLink/src/HornetLinkSlave.h`

---

## 6. DcsDirectSource UDP Frame (Port 42002)

Each UDP datagram from the Lua exporter to port 42002 contains one complete
DCS-BIOS frame (sync word + write records).  The bridge feeds datagrams
directly into `ExportParser::processBytes()`.

There is no additional framing at the UDP level; the datagram boundary serves
as the frame boundary.

---

## 7. Replay File Format

Binary files used by `ReplayFileSource` for offline testing.

```
[timestamp_ms : uint32 LE]  — absolute time from start of recording
[payload_len  : uint16 LE]  — number of bytes in the DCS-BIOS payload
[payload      : payload_len bytes]  — raw DCS-BIOS frame
```

Records are stored in chronological order.  Inter-frame delay exceeding
`kMaxInterFrameDelayMs` (200 ms) is capped to prevent stalls on large pauses
in the original recording.  The speed multiplier (`setSpeedMultiplier()`)
scales all delays proportionally.
