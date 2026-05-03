# RS-485 Sub-Bus Protocol Reference

This document is the authoritative specification for the RS-485 sub-bus protocol
used between an RS-485 **master** Arduino board and its **slave** panel boards in
the Hornet Link system.

---

## Overview

The primary serial link (USB-CDC, 500 000 baud) carries full DCS-BIOS frames between
the PC bridge (`hornet-link.exe`) and the master board.  The RS-485 sub-bus runs at
**250 000 baud, 8N1** and carries:

- DCS-BIOS delta frames from master → slave (filtered to each slave's subscriptions)
- Import command lines from slave → master → PC → DCS
- Probe / keep-alive frames (bus enumeration)
- Operating-mode frames (Sim / Preflight / Maintenance)

---

## Physical Layer

| Parameter       | Value                              |
|-----------------|------------------------------------|
| Baud rate       | 250 000 baud                       |
| Data format     | 8N1 (8 data bits, no parity, 1 stop) |
| Transceiver     | MAX485 / MAX3485 (or equivalent)   |
| Bus topology    | Half-duplex multi-drop             |
| Direction pin   | Single GPIO → /RE + DE tied together |
| Maximum nodes   | 1 master + up to 16 slaves         |
| Maximum cable   | 1200 m @ 250 kbaud (rule of thumb) |

---

## Frame Format

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

### CRC Algorithm

**CRC-16/CCITT-FALSE** (Kermit / CCITT variant):

| Parameter   | Value  |
|-------------|--------|
| Polynomial  | 0x1021 |
| Initial     | 0xFFFF |
| Input refl. | No     |
| Output refl.| No     |
| Final XOR   | 0x0000 |

CRC is computed over all bytes from DST through the last payload byte (inclusive).
The STX byte is excluded from the CRC.

---

## Bus Addresses

| Address | Assigned to                             |
|---------|-----------------------------------------|
| 0x00    | Broadcast — all nodes must process      |
| 0x01–0xFE | Slave panels (user-assigned per device) |
| Master  | Always source address 0x00              |

---

## Message Types

The first byte of every PAYLOAD field is the **message type**.

### Master → Slave

| Type byte | Constant           | Description                                       |
|-----------|--------------------|---------------------------------------------------|
| `0x10`    | `kRS485_PROBE`     | Probe / keep-alive — "are you there?"             |
| `0x20`    | `kRS485_DATA`      | DCS-BIOS delta frame (filtered to slave's subs)   |
| `0x40`    | `kRS485_MODE`      | Operating-mode change `[type][mode_value]`        |

### Slave → Master

| Type byte | Constant           | Description                                       |
|-----------|--------------------|---------------------------------------------------|
| `0x11`    | `kRS485_PROBE_ACK` | Probe acknowledgement — "I am here"               |
| `0x30`    | `kRS485_IMPORT`    | Import command line (ASCII, `\n` terminated)      |
| `0x41`    | `kRS485_MODE_ACK`  | Mode acknowledgement `[type][local_mode]`         |

---

## Message Payloads

### Probe (`0x10`)

```
[0x10]
```

No body bytes.  Sent by master periodically (default every 20 ms) to confirm each
slave is still alive.  If a slave does not reply within 10 ms it is removed from
the active slave table.

### Probe Ack (`0x11`)

```
[0x11]
```

No body bytes.  Sent by slave in response to a Probe.

### DCS-BIOS Delta Data (`0x20`)

```
[0x20] [sync: 0x55 0x55 0x55 0x55] [write records...]
```

The payload after the type byte is a complete DCS-BIOS export frame:

```
[sync: 0x55 0x55 0x55 0x55]
[addr_lo addr_hi len_lo len_hi data...]  (one or more records)
```

Multiple consecutive addresses may be merged into a single write record
(length > 2).  See the DCS-BIOS Export Protocol section below.

### Import Command (`0x30`)

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

### Mode Change (`0x40`)

```
[0x40] [mode : 1]
```

| Mode byte | Meaning      |
|-----------|--------------|
| `0x00`    | Sim          |
| `0x01`    | Preflight    |
| `0x02`    | Maintenance  |

### Mode Ack (`0x41`)

```
[0x41] [local_mode : 1]
```

Slave echoes back the mode it has applied locally.

---

## Timing Requirements

| Parameter                      | Value     | Notes                                            |
|--------------------------------|-----------|--------------------------------------------------|
| Slave reply timeout            | 10 ms     | After a Probe or Data frame                      |
| Keep-alive poll interval       | 20 ms     | Master polls each slave in round-robin           |
| New-slave probe timeout        | 300 ms    | Declares a candidate address absent if no reply  |
| Direction-pin settle time      | 10 µs     | Before asserting TX; after de-asserting TX       |
| Maximum payload size           | 512 bytes | Larger frames are silently discarded             |

---

## DCS-BIOS Export Protocol (Primary Link)

The primary link (PC ↔ master) uses the standard DCS-BIOS export format:

```
Sync word:        0x55 0x55 0x55 0x55
Write record:     [addr_lo] [addr_hi] [len_lo] [len_hi] [data bytes × len]
```

- All multi-byte integers are **little-endian**.
- A frame begins with exactly one 4-byte sync word, followed by zero or more write records.
- Addresses are **even** byte offsets into the 64 KiB DCS-BIOS state space.
- `len` is the number of data bytes (always even; one 16-bit word per 2 bytes).

---

## Handshake Protocol (Primary Link)

Occurs once when the PC opens the COM port.

```
PC → Device:    AA DE AD 01                        (Probe / Ping)
Device → PC:    AA DE AD 02 [flags] [name_len] [name] [sub_count_lo sub_count_hi]
                            [addr_lo addr_hi mask_lo mask_hi shift] × sub_count
                            (if flags & 0x01) [slave_count] [slaves...]
PC → Device:    AA DE AD 03                        (Ack / negotiation complete)
```

### Flags Byte

| Bit  | Value | Meaning                                           |
|------|-------|---------------------------------------------------|
| 0    | 0x01  | RS-485 Master (has downstream slave bus)           |
| 1    | 0x02  | RS-485 Slave  (managed by a master)                |
| 2    | 0x04  | Bidirectional (can send import commands to DCS)    |

### Slave List (RS-485 Masters Only)

Appended after the subscription entries when `flags & 0x01`:

```
[slave_count : 1]
For each slave:
  [slave_addr    : 1]  RS-485 bus address
  [slave_name_len: 1]
  [slave_name    : slave_name_len bytes]
  [slave_sub_count_lo : 1]
  [slave_sub_count_hi : 1]
  [addr_lo addr_hi mask_lo mask_hi shift] × slave_sub_count
```

---

## Operating Modes (Primary Link — Mode Frames)

Sent by the PC to change all device operating modes simultaneously.

```
PC → Device:    AA DE AD 04 [mode]
Device → PC:    AA DE AD 05 [local_mode]
```

| Mode | Value | Behaviour                                        |
|------|-------|--------------------------------------------------|
| Sim  | 0x00  | Normal — live DCS data, imports forwarded to DCS |
| Preflight | 0x01 | BIT / lamp-test — all lights illuminate   |
| Maintenance | 0x02 | Panel self-check, DCS not required      |

In **Preflight** and **Maintenance** modes the bridge does not forward import
commands from devices to DCS.

---

## Error Handling

| Condition                              | Master action                            |
|----------------------------------------|------------------------------------------|
| Slave does not reply to Probe in 10 ms | Remove from active table; log warning    |
| CRC mismatch on received frame         | Discard frame; increment error counter   |
| Payload length > 512 bytes             | Discard frame; log error                 |
| Unknown message type byte              | Discard and continue                     |

---

## Reference Implementation

- **C++ (PC bridge):** `RS485ProtocolSpec.hpp` — CRC function, frame encode/verify,
  constants.
- **C++ (PC bridge):** `DeviceRegistry.hpp` — `HandshakeParser` including slave-list
  parsing.
- **Arduino (master):** `libraries/HornetLink/src/HornetLinkMaster.h`
- **Arduino (slave):**  `libraries/HornetLink/src/HornetLinkSlave.h`
