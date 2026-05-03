# Hornet Link — Protocol Reference

Complete wire-level reference for all protocols used in the Hornet Link system.

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

See [RS485_PROTOCOL.md](../RS485_PROTOCOL.md) for the full specification.

Quick reference:

| Layer         | Value                                         |
|---------------|-----------------------------------------------|
| Start byte    | `0xFE`                                        |
| Frame         | STX DST SRC LEN_LO LEN_HI [payload] CRC_LO CRC_HI |
| CRC           | CRC-16/CCITT-FALSE (poly=0x1021, init=0xFFFF) |
| Baud rate     | 250 000 baud, 8N1                             |
| Max payload   | 512 bytes                                     |

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
