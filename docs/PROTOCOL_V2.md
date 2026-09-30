# Hornet Link Protocol v2 (Hornet-native)

Protocol v2 connects cockpit panels to `hornet-link.exe` without DCS-BIOS. It
is built for the F/A-18C only. Controls have stable names and IDs from one
catalogue, values are typed, and the RS-485 bus is collision-free.

| Document | Contents |
|---|---|
| This file | Framing, bus rules, timing, addressing, version negotiation |
| [PROTOCOL_V2_MESSAGES.md](PROTOCOL_V2_MESSAGES.md) | Every message, payload layout and code (generated) |
| [`catalog/protocol_v2.json`](../catalog/protocol_v2.json) | Machine-readable spec that the tables and `HnSpec.h` are generated from |
| [F18C_CONTROL_REFERENCE.md](F18C_CONTROL_REFERENCE.md) | Every F/A-18C control: ID, name, type, positions (generated) |
| [DCS_DATA_PROVENANCE.md](DCS_DATA_PROVENANCE.md) | Where the DCS device/argument/command numbers came from |
| [FIRST_PANEL.md](FIRST_PANEL.md) | Beginner guide and troubleshooting table |

The legacy DCS-BIOS-based protocol (v1) is still in
[RS485_PROTOCOL.md](../RS485_PROTOCOL.md) and
[PROTOCOL_REFERENCE.md](PROTOCOL_REFERENCE.md). Both versions are supported
side by side until v2 has been checked on real panels.

## 1. The pieces

```
 DCS World (F/A-18C)
   Scripts/Export.lua  ── dofile ──>  HornetLinkNative.lua   (generated from catalog/fa18c.json)
        │  UDP 42003: "HLN 2 <hash> <aircraft> <active> <full>" + "PPCC=value" lines
        │  UDP 42004: "PPCC SET 1", "PPCC PRESS", ...
        ▼
 hornet-link.exe  (HornetNative.hpp: CatalogState, LinkSession, SyncTracker)
        │  USB serial, 250000 baud, v2 frames
        ▼
 Standalone panel (Hornet::Panel, beginUsb)      or      Bus master (Hornet::BusMaster)
                                                             │ RS-485, 250000 baud 8N1, half duplex
                                                             ▼
                                                   Slaves (Hornet::Panel, beginRs485)
```

- **Control ID**: 16 bits, high byte = panel, low byte = control
  (`0x0101` = `MASTER_ARM.MASTER_ARM`). IDs never change once released.
- **Catalogue hash**: a 32-bit hash of `catalog/fa18c.json`. Every HELLO and
  every exporter datagram carries it. The bridge warns when a board was built
  with a different catalogue.

## 2. Framing

```
raw frame  = [ver=2][dst][src][type][seq][len][payload: len bytes][crc_lo][crc_hi]
wire frame = COBS(raw frame) + 0x00
```

- **CRC**: CRC-16/CCITT-FALSE (poly `0x1021`, init `0xFFFF`) over header and
  payload. This is the same CRC as v1. Check value: `"123456789"` → `0x29B1`.
- **COBS** removes every `0x00` from the frame, so `0x00` only ever means
  "end of frame". A receiver that joins mid-stream or sees a corrupt byte
  resynchronises at the next `0x00`. It cannot lock onto a false start byte.
- **Limits**: payload ≤ 120 bytes, wire frame ≤ 131 bytes. A receiver discards
  input longer than that at once and counts an *overflow*.
- **Sequence number**: incremented per frame by the sender. It is used for
  ACK/NACK and duplicate detection (a retried INPUT keeps its number).
- A frame is rejected (and counted) for: wrong CRC, bad COBS or length
  mismatch (*framing*), version ≠ 2 (*version*), too long (*overflow*).

## 3. Addresses

| Address | Meaning |
|---|---|
| `0` | Broadcast |
| `1`–`239` | RS-485 slaves |
| `253` | The bridge (PC) |
| `254` | The board on the USB cable (bus master or standalone panel) |
| `255` | Unassigned (only as a source during discovery) |

**Fixed addresses are the default.** Most OpenHornet panels already have a set
bus address. Set it in the sketch with `panel.beginRs485(Serial1, 5, DE_PIN)`
and list it in the master sketch with `master.addSlave(5)`.

Optional extras:

- **Change in software**: `CONFIG BUS_ADDRESS` from the bridge. The node ACKs
  with its old address first, then switches. Save it with `CONFIG SAVE`; the
  sketch's `onAddressChange` / `onSave` hooks store it (e.g. EEPROM).
- **Hardware (DIP switches)**: read the switches in `setup()` and pass the
  value to `beginRs485()`.
- **Discovery**: `master.enableDiscovery(true)`. Once a second the master
  broadcasts `DISCOVER [8 slots][2 ms]`. Every slave answers HELLO in one
  random slot, including slaves already being polled. Its random seed includes
  its board ID and address, so duplicate fixed addresses can identify themselves
  in separate slots. The master adds new addresses to its poll list. If two
  HELLOs with the same address but different board IDs arrive in one window, it
  reports `BUS_EVENT CONFLICT`.

## 4. Bus discipline (RS-485)

**A slave only transmits when the master has just polled it.**

1. The master sends any queued downstream frames (STATE, MODE, ...). Slaves
   only listen.
2. The master sends `POLL` to one slave.
3. That slave sends exactly one frame, or `POLL_EMPTY` if it has nothing
   queued.
4. The master waits up to 8 ms for the reply, then moves on to the next slave.

Normally only the polled node drives the bus. The DISCOVER window uses random
slots, including for already-polled nodes, to identify duplicate fixed
addresses. Duplicate boards can collide when answering a poll; the discovery
window provides a separate opportunity for their HELLOs to arrive in different
slots. The unit tests run a simulated bus with three slaves and fail if any
slave ever transmits out of turn.

**Slave send priority** (one frame per poll):

1. Reply slot: ACK / NACK / MODE_ACK for a request from the bridge
2. HELLO
3. Retry of an unacknowledged INPUT
4. SUBSCRIBE
5. DESCRIBE pages
6. DIAG
7. SYNC_REPORT pages
8. New INPUT batch

**Drop and rejoin.** After 5 missed polls in a row the master marks the
slave offline and sends `BUS_EVENT DROP` upstream. Offline slaves are polled
once a second. When one answers, the master sends `BUS_EVENT JOIN`. The bridge
then sends HELLO_REQUEST, which makes the slave send HELLO, SUBSCRIBE and a full
SYNC_REPORT. The bridge also sends the full state again.

### Slot budget (250 kbaud, 8N1 = 25 kB/s, 40 µs per byte)

| Frame | Wire bytes | Time |
|---|---|---|
| POLL / POLL_EMPTY / HEARTBEAT | 10 | 0.40 ms |
| ACK | 11 | 0.44 ms |
| INPUT, 1 event | 15 | 0.60 ms |
| STATE, full 120-byte payload | ≈ 131 | ≈ 5.2 ms |
| Driver turnaround (each direction) | — | ≈ 0.1 ms |

An idle slave costs about **1 ms** per poll cycle (POLL + turnaround +
POLL_EMPTY + turnaround). An input event costs about 1.7 ms (poll, INPUT,
ACK in the next reply slot).

| Online slaves | Poll cycle (all idle) | Worst-case input latency |
|---|---|---|
| 4 | ≈ 4 ms | ≈ 6 ms |
| 8 | ≈ 8 ms | ≈ 10 ms |
| 16 | ≈ 16 ms | ≈ 18 ms |
| 32 (maximum) | ≈ 32 ms | ≈ 35 ms |

Add about 5 ms for each full STATE frame sent in that cycle. One broadcast
serves every slave: each slave keeps only the IDs it subscribed to. Each
offline slave costs one 8 ms timeout per second. Remove boards from the master
sketch that aren't fitted.

## 5. USB link

On USB there is only one node, so it may transmit at any time. There is no
polling. An unacknowledged INPUT frame is resent after 100 ms, at most 3
times, then dropped and counted in DIAG. On a bus the retry happens at the
next poll.

The bridge de-duplicates INPUT frames by `(source, seq)`, so a retry after a
lost ACK is ACKed again but not sent to DCS twice. The bus master does the
same for the slaves.

## 6. Connection and version negotiation

1. The bridge opens the COM port and broadcasts `HELLO_REQUEST`.
2. **v2 board:** answers HELLO (identity, firmware version, catalogue hash,
   board ID), then SUBSCRIBE and SYNC_REPORT. A bus master also answers with
   its own HELLO (role BUS_MASTER) and one `BUS_EVENT JOIN` per online slave.
3. **v1 board:** ignores the frame (no valid v1 start byte). If no v2 frame
   arrives within about 200 ms, the bridge falls back to the v1 handshake ping.
4. The bridge can ask any node for `DESCRIBE` (the controls it owns),
   `DIAG` (counters) and `SYNC_REPORT` (physical positions) at any time.

> **Status:** steps 1-4 are implemented in `LinkSession` (bridge) and the
> firmware. Wiring `LinkSession` and the native UDP source into the bridge's
> COM-port loop and UI is stage 2; see the future-work list in
> [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md).

## 7. Typed values and inputs

**STATE record** = `[id u16][kind u8][value]`:

| Kind | Value | Used for |
|---|---|---|
| `BOOL` | u8 0/1 | Lamps |
| `POSITION` | u8 index | Switch/selector position (index into the catalogue's position names) |
| `ANALOG` | u16 0–65535 | Gauges, pots |
| `TEXT` | `[len][bytes]` | UFC, IFEI, option displays (fixed length, padded) |

**INPUT record** = `[id u16][action u8][arg u16]`:

| Action | Arg | Example |
|---|---|---|
| `SET_POSITION` | position index | Master Arm → ARM (1) |
| `STEP` | signed count | Rotary/encoder +2 detents |
| `PRESS` / `RELEASE` | — | UFC key 1 |
| `ANALOG` | 0–65535 | Volume knob |

The bridge checks every input against the catalogue before it reaches DCS. It
answers with a **NACK reason**:

| Reason | Meaning | Typical cause |
|---|---|---|
| `UNKNOWN_CONTROL` | ID not in the catalogue | Library older/newer than the bridge |
| `BAD_VALUE` | Action or value doesn't fit the control | Position out of range, pot bound to a switch, output ID used as input |
| `WRONG_MODE` | Inputs refused in this mode | Lamp test or maintenance is active |
| `BAD_FRAME` | Malformed payload | Firmware bug, noise |
| `CATALOG_MISMATCH` | Catalogue hash differs | Rebuild the sketch with the current library |
| `UNSUPPORTED` | Request not supported by this node | e.g. CONFIG sent to a bus master |

## 8. Sync: DCS is the source of truth

On connect, spawn or rejoin, the bridge **never** pushes cockpit switch
positions into DCS. Instead:

1. Each node sends a SYNC_REPORT with the physical position of every input.
2. The bridge compares these with the positions DCS reports (`SyncTracker`).
   Momentary buttons and endless rotaries are skipped, and pots within about 6 %
   count as matching.
3. Differences are listed as `MASTER_ARM.MASTER_ARM: set SAFE (is ARM)` until
   the pilot moves each switch to match. Moving a switch sends a normal INPUT,
   so the list updates as you go.
4. The list can also be shown as an in-game overlay. This is a user setting
   (`SyncTracker::overlayEnabled`, on by default).

> The overlay text is produced by the bridge today. Drawing it inside DCS needs
> a small `Scripts/Hooks` GUI script. That is stage-2 work.

## 9. Modes

| Mode | Panel behaviour | Bridge behaviour |
|---|---|---|
| `SIM` | Normal | Forwards inputs to DCS |
| `LAMP_TEST` | All lamps on, displays show `8` | NACK `WRONG_MODE` for inputs |
| `MAINTENANCE` | Outputs keep their last state | NACK `WRONG_MODE` for inputs |
| `WIRING_TEST` | Normal | Logs every input by name, e.g. `WIRING BUS5 UFC.KEY_1 PRESS`, and doesn't send it to DCS |

## 10. ASCII debug mode (no bridge needed)

`panel.beginDebug(Serial)` swaps the binary protocol for plain text at
115200 baud, so you can watch a panel in the Arduino Serial Monitor:

```
MASTER_ARM.MASTER_ARM=ARM          <- printed when you flip the switch
UFC.KEY_1=PRESSED
MASTER_ARM.READY_LT=1              -> type this to light the LED
SYNC                               -> prints every input's position
MODE LAMP_TEST                     -> SIM, LAMP_TEST, MAINTENANCE, WIRING_TEST
```

The bridge's frame decoder (`hornet_native::decodeFrame`) prints v2 frames the
same way, e.g. `BUS3 -> LINK INPUT #7: UFC.KEY_1 PRESS`.

## 11. Changing the protocol

1. Edit `catalog/protocol_v2.json` (messages, codes, timing) or
   `catalog/fa18c.json` (controls).
2. Run `python3 Programs/tools/generate_hornet_catalog.py`. CI runs it with
   `--check` and fails if generated files are stale.
3. Update this document if framing or bus rules changed.
4. Run the unit tests; the simulated-bus tests check the timing rules.
