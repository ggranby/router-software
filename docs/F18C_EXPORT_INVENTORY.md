# F/A-18C Hornet export inventory

> **Baseline:** DCS-BIOS v0.11.7 reference JSON. This is a versioned working catalog, not a claim that DCS-BIOS is the permanent source of truth.
> No live DCS capture has been used to verify these entries.

This catalog groups each documented control under its DCS-BIOS equipment category. Controls with one or more input interfaces are marked **Interactable (documented input)**; those without inputs are **Output only (no documented input)**. This describes DCS-BIOS metadata, not proof that every input works in every DCS context.

Reference: [DCS-BIOS v0.11.7 release](https://github.com/DCS-Skunkworks/dcs-bios/releases/tag/v0.11.7), `DCS-BIOS/doc/json/FA-18C_hornet.json` in the release archive.
SHA-256 of the exact JSON input: `1edfb45c430e6b149dadd4c2469b6e4ac363f6834b3f43afed754f60090392ef`

Regenerate the baseline after extracting that JSON file from a chosen DCS-BIOS release:

```sh
python3 Programs/tools/generate_f18c_inventory.py /path/to/FA-18C_hornet.json --version vX.Y.Z --output docs/F18C_EXPORT_INVENTORY.md
```

## Coverage

- Equipment/category groups: **74**
- Documented controls: **505**
- Controls with input interfaces: **298**
- Controls without input interfaces: **207**
- Output records: **503** (440 integer, 63 string)

The address, mask, and shift values below are from the DCS-BIOS memory map. They must not be treated as native DCS export addresses.

## Inventory

### AMPCD

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| AMPCD_BRT_CTL | Brightness Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x74E0`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| AMPCD_CONT_SW | Contrast Control Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x746C`, mask `0x0C00`, shift `10`, max `2`, selector position | Not verified |
| AMPCD_GAIN_SW | Gain Control Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x746C`, mask `0x3000`, shift `12`, max `2`, selector position | Not verified |
| AMPCD_NIGHT_DAY | Night/Day Brightness Selector<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x7466`, mask `0x6000`, shift `13`, max `2`, selector position | Not verified |
| AMPCD_PB_01 | Pushbutton 1<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7466`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| AMPCD_PB_02 | Pushbutton 2<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x746C`, mask `0x4000`, shift `14`, max `1`, selector position | Not verified |
| AMPCD_PB_03 | Pushbutton 3<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x746C`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| AMPCD_PB_04 | Pushbutton 4<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747A`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |
| AMPCD_PB_05 | Pushbutton 5<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747A`, mask `0x0200`, shift `9`, max `1`, selector position | Not verified |
| AMPCD_PB_06 | Pushbutton 6<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747A`, mask `0x0400`, shift `10`, max `1`, selector position | Not verified |
| AMPCD_PB_07 | Pushbutton 7<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747A`, mask `0x0800`, shift `11`, max `1`, selector position | Not verified |
| AMPCD_PB_08 | Pushbutton 8<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747A`, mask `0x1000`, shift `12`, max `1`, selector position | Not verified |
| AMPCD_PB_09 | Pushbutton 9<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747A`, mask `0x2000`, shift `13`, max `1`, selector position | Not verified |
| AMPCD_PB_10 | Pushbutton 10<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747A`, mask `0x4000`, shift `14`, max `1`, selector position | Not verified |
| AMPCD_PB_11 | Pushbutton 11<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747A`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| AMPCD_PB_12 | Pushbutton 12<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747C`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |
| AMPCD_PB_13 | Pushbutton 13<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747C`, mask `0x0200`, shift `9`, max `1`, selector position | Not verified |
| AMPCD_PB_14 | Pushbutton 14<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747C`, mask `0x0400`, shift `10`, max `1`, selector position | Not verified |
| AMPCD_PB_15 | Pushbutton 15<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747C`, mask `0x0800`, shift `11`, max `1`, selector position | Not verified |
| AMPCD_PB_16 | Pushbutton 16<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747C`, mask `0x1000`, shift `12`, max `1`, selector position | Not verified |
| AMPCD_PB_17 | Pushbutton 17<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747C`, mask `0x2000`, shift `13`, max `1`, selector position | Not verified |
| AMPCD_PB_18 | Pushbutton 18<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747C`, mask `0x4000`, shift `14`, max `1`, selector position | Not verified |
| AMPCD_PB_19 | Pushbutton 19<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747C`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| AMPCD_PB_20 | Pushbutton 20<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747E`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |
| AMPCD_SYM_SW | Symbology Control Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x746C`, mask `0x0300`, shift `8`, max `2`, selector position | Not verified |

### APU Fire Warning Extinguisher Light

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| APU_FIRE_BTN | APU Fire Warning/Extinguisher Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740C`, mask `0x0008`, shift `3`, max `1`, selector position | Not verified |
| FIRE_APU_LT | FIRE APU Light (red)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740C`, mask `0x0004`, shift `2`, max `1`, 0 if light is off, 1 if light is on | Not verified |

### Angle of Attack Indexer Lights

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| AOA_INDEXER_HIGH | AOA Indexer High (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x0008`, shift `3`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| AOA_INDEXER_HIGH_F | AOA Indexer High as Float (green)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x758C`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| AOA_INDEXER_LOW | AOA Indexer Low (red)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x0020`, shift `5`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| AOA_INDEXER_LOW_F | AOA Indexer Low as Float (red)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7590`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| AOA_INDEXER_NORMAL | AOA Indexer Normal (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x0010`, shift `4`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| AOA_INDEXER_NORMAL_F | AOA Indexer Normal as Float (yellow)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x758E`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |

### Antenna Select Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| COMM1_ANT_SELECT_SW | COMM 1 Antenna Selector Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74C0`, mask `0x3000`, shift `12`, max `2`, selector position | Not verified |
| IFF_ANT_SELECT_SW | IFF Antenna Selector Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74C0`, mask `0xC000`, shift `14`, max `2`, selector position | Not verified |

### Arresting Hook Handle and Light

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| ARRESTING_HOOK_LT | Hook Light<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A0`, mask `0x0400`, shift `10`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| HOOK_LEVER | Hook Lever<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74A0`, mask `0x0200`, shift `9`, max `1`, selector position | Not verified |

### Auxiliary Power Unit Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| APU_CONTROL_SW | APU Control Switch, ON/OFF<br>`action` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set the switch position -- 0 = off, 1 = on; max_value=1)<br>`action` (toggle switch state; argument=TOGGLE) | `integer` at `0x74C2`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |
| APU_READY_LT | APU Ready Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74C2`, mask `0x0800`, shift `11`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| ENGINE_CRANK_SW | Engine Crank Switch<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74C2`, mask `0x0600`, shift `9`, max `2`, selector position | Not verified |

### Canopy Internal Jettison Handle

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| CANOPY_JETT_HANDLE_PULL | Canopy Jettison Handle Unlock Button - Press to jettison<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740C`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |
| CANOPY_JETT_HANDLE_UNLOCK | Canopy Jettison Handle Unlock Button - Press to unlock<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740C`, mask `0x0080`, shift `7`, max `1`, selector position | Not verified |

### Caution Light Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| CLIP_APU_ACC_LT | APU ACC (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A4`, mask `0x0100`, shift `8`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| CLIP_BATT_SW_LT | BATT SW (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A4`, mask `0x0200`, shift `9`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| CLIP_CK_SEAT_LT | CK SEAT (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A0`, mask `0x8000`, shift `15`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| CLIP_FCES_LT | FCES (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A4`, mask `0x4000`, shift `14`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| CLIP_FCS_HOT_LT | FCS HOT (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A4`, mask `0x0400`, shift `10`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| CLIP_FUEL_LO_LT | FUEL LO (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A4`, mask `0x2000`, shift `13`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| CLIP_GEN_TIE_LT | GEN TIE (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A4`, mask `0x0800`, shift `11`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| CLIP_L_GEN_LT | L GEN (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A8`, mask `0x0100`, shift `8`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| CLIP_R_GEN_LT | R GEN (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A8`, mask `0x0200`, shift `9`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| CLIP_SPARE_CTN1_LT | SPARE CTN1 (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A4`, mask `0x1000`, shift `12`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| CLIP_SPARE_CTN2_LT | SPARE CTN2 (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A4`, mask `0x8000`, shift `15`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| CLIP_SPARE_CTN3_LT | SPARE CTN3 (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A8`, mask `0x0400`, shift `10`, max `1`, 0 if light is off, 1 if light is on | Not verified |

### Clock

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| CLOCK_ELAPSED_MINUTES | Elapsed Minutes<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7510`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| CLOCK_ELAPSED_SECONDS | Elapsed Seconds<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7512`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| CLOCK_HOURS | Hours<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x750C`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| CLOCK_MINUTES | Minutes<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x750E`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |

### Cockpit Altimeter

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| PRESSURE_ALT | Pressure Altitude<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7514`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |

### Comms frequency

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| COMM1_CHANNEL_NUMERIC | Comm 1 Channel as number<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x7404`, mask `0x001F`, shift `0`, max `24`, Comm 1 Channel as number | Not verified |
| COMM1_FREQ | COMM1 FREQ<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x7400`, mask `0xFFFF`, shift `0`, max `65535`, COMM1 FREQ | Not verified |
| COMM2_CHANNEL_NUMERIC | Comm 2 Channel as number<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x7406`, mask `0x001F`, shift `0`, max `24`, Comm 2 Channel as number | Not verified |
| COMM2_FREQ | COMM2 FREQ<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x7402`, mask `0xFFFF`, shift `0`, max `65535`, COMM2 FREQ | Not verified |

### Communication Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| COM_AUX | AUX Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7538`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| COM_COMM_G_XMT_SW | COMM G XMT Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74BC`, mask `0x1800`, shift `11`, max `2`, selector position | Not verified |
| COM_COMM_RELAY_SW | Comm Relay Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74BC`, mask `0x0600`, shift `9`, max `2`, selector position | Not verified |
| COM_CRYPTO_SW | CRYPTO Switch<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74BE`, mask `0x0300`, shift `8`, max `2`, selector position | Not verified |
| COM_ICS | ICS Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x752C`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| COM_IFF_MASTER_SW | IFF Master Switch, EMER/NORM<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74BC`, mask `0x2000`, shift `13`, max `1`, selector position | Not verified |
| COM_IFF_MODE4_SW | IFF Mode 4 Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74BC`, mask `0xC000`, shift `14`, max `2`, selector position | Not verified |
| COM_ILS_CHANNEL_SW | ILS Channel Selector Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=19) | `integer` at `0x74BE`, mask `0xF800`, shift `11`, max `19`, selector position | Not verified |
| COM_ILS_UFC_MAN_SW | ILS UFC/MAN Switch, UFC/MAN<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74BE`, mask `0x0400`, shift `10`, max `1`, selector position | Not verified |
| COM_MIDS_A | MIDS A Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7532`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| COM_MIDS_B | MIDS B Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7534`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| COM_RWR | RWR Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x752E`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| COM_TACAN | TACAN Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7536`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| COM_VOX | VOX Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x752A`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| COM_WPN | WPN Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7530`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |

### Defog Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| DEFOG_HANDLE | Defog Handle<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7550`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| WSHIELD_ANTI_ICE_SW | Windshield Anti-Ice/Rain Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74CC`, mask `0x6000`, shift `13`, max `2`, selector position | Not verified |

### Dispenser/EMC Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| AUX_REL_SW | Auxiliary Release Switch, ENABLE/NORM<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7488`, mask `0x0800`, shift `11`, max `1`, selector position | Not verified |
| CMSD_DISPENSE_SW | DISPENSER Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x7484`, mask `0x6000`, shift `13`, max `2`, selector position | Not verified |
| CMSD_JET_SEL_BTN | ECM JETT JETT SEL Button - Push to jettison<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7484`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| CMSD_JET_SEL_L | ECM JETT JETT SEL Button Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74D4`, mask `0x8000`, shift `15`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| ECM_MODE_SW | ECM Mode Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=4) | `integer` at `0x7488`, mask `0x0700`, shift `8`, max `4`, selector position | Not verified |

### ECM Dispenser Button

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| CMSD_DISPENSE_BTN | Dispense Button - Push to dispense flares and chaff<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C2`, mask `0x4000`, shift `14`, max `1`, selector position | Not verified |

### Ejection Seat

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| EJECTION_HANDLE_SW | Ejection Control Handle<br>`toggle_switch` | **Interactable**<br>`set_state` (set the switch position -- 0 = off, 1 = on; max_value=1) | `integer` at `0x74CE`, mask `0x4000`, shift `14`, max `1`, switch position -- 0 = off, 1 = on | Not verified |
| EJECTION_SEAT_ARMED | Ejection Seat SAFE/ARMED Handle, SAFE/ARMED<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74CE`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| EJECTION_SEAT_MNL_OVRD | Ejection Seat Manual Override Handle, PULL/PUSH<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D0`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |
| HIDE_STICK_TOGGLE | Hide Stick toggle<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D0`, mask `0x1000`, shift `12`, max `1`, selector position | Not verified |
| SEAT_HEIGHT_SW | Seat Height Adjustment Switch<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74D0`, mask `0x0C00`, shift `10`, max `2`, selector position | Not verified |
| SHLDR_HARNESS_SW | Shoulder Harness Control Handle, LOCK/UNLOCK<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D0`, mask `0x0200`, shift `9`, max `1`, selector position | Not verified |

### Electrical Power Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| BATTERY_SW | Battery Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74C4`, mask `0x1800`, shift `11`, max `2`, selector position | Not verified |
| L_GEN_SW | Left Generator Control Switch, NORM/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C4`, mask `0x2000`, shift `13`, max `1`, selector position | Not verified |
| R_GEN_SW | Right Generator Control Switch, NORM/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C4`, mask `0x4000`, shift `14`, max `1`, selector position | Not verified |
| VOLT_E | Battery E Volts<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x753E`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| VOLT_U | Battery U Volts<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x753C`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |

### Emergency Jettison Button

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| EMER_JETT_BTN | Emergency Jettison Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742C`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |

### Emergency and Parking Brake Handle

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| EMERGENCY_PARKING_BRAKE_PULL | Emergency/Parking Brake Pull<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7484`, mask `0x0400`, shift `10`, max `1`, selector position | Not verified |
| EMERGENCY_PARKING_BRAKE_ROTATE | Emergency/Parking Brake Rotate<br>`emergency_parking_brake` | **Interactable**<br>`set_state` (set the switch position -- 0 = emergency, 1 = park, 2 = release; max_value=2) | `integer` at `0x7484`, mask `0x1800`, shift `11`, max `2`, switch position -- 0 = emergency, 1 = parking, 2 = release | Not verified |

### Environment Control Louver

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| LEFT_LOUVER | Left Louver<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7502`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| RIGHT_LOUVER | Right Louver<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7504`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |

### Environment Control System Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| BLEED_AIR_KNOB | Bleed Air Knob<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=3) | `integer` at `0x74C6`, mask `0x0300`, shift `8`, max `3`, selector position | Not verified |
| BLEED_AIR_PULL | Bleed Air Knob, AUG PULL<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C4`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| CABIN_PRESS_SW | Cabin Pressure Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74C6`, mask `0x3000`, shift `12`, max `2`, selector position | Not verified |
| CABIN_TEMP | Cabin Temperature Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7540`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| ECS_MODE_SW | ECS Mode Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74C6`, mask `0x0C00`, shift `10`, max `2`, selector position | Not verified |
| ENG_ANTIICE_SW | Engine Anti-Ice Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74C6`, mask `0xC000`, shift `14`, max `2`, selector position | Not verified |
| PITOT_HEAT_SW | Pitot Heater Switch, ON/AUTO<br>`action` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set the switch position -- 0 = off, 1 = on; max_value=1)<br>`action` (toggle switch state; argument=TOGGLE) | `integer` at `0x74C8`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |
| SUIT_TEMP | Suit Temperature Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7542`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |

### Exterior Lights Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| FORMATION_DIMMER | Formation Lights Dimmer<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7526`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| INT_WNG_TANK_SW | Internal Wing Tank Fuel Control Switch, INHIBIT/NORM<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74A8`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| POSITION_DIMMER | Position Lights Dimmer<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7524`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| STROBE_SW | Strobe Lights Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74B0`, mask `0x3000`, shift `12`, max `2`, selector position | Not verified |

### External Aircraft Model

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| EXT_FORMATION_LIGHTS | Formation Lights (light green)<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x7576`, mask `0xFFFF`, shift `0`, max `65535`, Formation Lights (light green) | Not verified |
| EXT_HOOK | Hook Position<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x7586`, mask `0xFFFF`, shift `0`, max `65535`, Hook Position | Not verified |
| EXT_LAUNCH_BAR | Launch Bar position<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x75AE`, mask `0xFFFF`, shift `0`, max `65535`, Launch Bar position | Not verified |
| EXT_NOZZLE_POS_L | Left Nozzle Position<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x757A`, mask `0xFFFF`, shift `0`, max `65535`, Left Nozzle Position | Not verified |
| EXT_NOZZLE_POS_R | Right Nozzle Position<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x7578`, mask `0xFFFF`, shift `0`, max `65535`, Right Nozzle Position | Not verified |
| EXT_POSITION_LIGHT_LEFT | Left Position Light (red)<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x74D6`, mask `0x0400`, shift `10`, max `1`, Left Position Light (red) | Not verified |
| EXT_POSITION_LIGHT_RIGHT | Right Position Light (green)<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x74D6`, mask `0x0800`, shift `11`, max `1`, Right Position Light (green) | Not verified |
| EXT_REFUEL_PROBE | Refuel Probe<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x7574`, mask `0xFFFF`, shift `0`, max `65535`, Refuel Probe | Not verified |
| EXT_REFUEL_PROBE_LIGHT | Refuel Probe Light (white)<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x74D6`, mask `0x0200`, shift `9`, max `1`, Refuel Probe Light (white) | Not verified |
| EXT_SPEED_BRAKE | Speed Brake<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x756E`, mask `0xFFFF`, shift `0`, max `65535`, Speed Brake | Not verified |
| EXT_STAIR | Stair<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x7572`, mask `0xFFFF`, shift `0`, max `65535`, Stair | Not verified |
| EXT_STROBE_LIGHTS | Strobe Lights (red)<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x74D6`, mask `0x2000`, shift `13`, max `1`, Strobe Lights (red) | Not verified |
| EXT_TAIL_LIGHT | Tail Light (white)<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x74D6`, mask `0x1000`, shift `12`, max `1`, Tail Light (white) | Not verified |
| EXT_WING_FOLDING | Wing Folding<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x7570`, mask `0xFFFF`, shift `0`, max `65535`, Wing Folding | Not verified |
| EXT_WOW_LEFT | Weight ON Wheels Left Gear<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x74D8`, mask `0x0100`, shift `8`, max `1`, Weight ON Wheels Left Gear | Not verified |
| EXT_WOW_NOSE | Weight ON Wheels Nose Gear<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x74D6`, mask `0x4000`, shift `14`, max `1`, Weight ON Wheels Nose Gear | Not verified |
| EXT_WOW_RIGHT | Weight ON Wheels Right Gear<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x74D6`, mask `0x8000`, shift `15`, max `1`, Weight ON Wheels Right Gear | Not verified |

### Fire Systems

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| FIRE_EXT_BTN | Fire Extinguisher Pushbutton<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x0001`, shift `0`, max `1`, selector position | Not verified |

### Fire Test Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| FIRE_TEST_SW | Fire and Bleed Air Test Switch, (RMB) TEST A/(LMB) TEST B<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74AC`, mask `0x0300`, shift `8`, max `2`, selector position | Not verified |

### Flaps, Landing Gear, Stores Indicator Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| FLP_LG_FLAPS_LT | FLAPS (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7466`, mask `0x0001`, shift `0`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| FLP_LG_FULL_FLAPS_LT | FULL FLAPS (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7430`, mask `0x8000`, shift `15`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| FLP_LG_HALF_FLAPS_LT | HALF FLAPS (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7430`, mask `0x4000`, shift `14`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| FLP_LG_LEFT_GEAR_LT | LEFT GEAR (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7430`, mask `0x1000`, shift `12`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| FLP_LG_NOSE_GEAR_LT | NOSE GEAR (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7430`, mask `0x0800`, shift `11`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| FLP_LG_RIGHT_GEAR_LT | RIGHT GEAR (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7430`, mask `0x2000`, shift `13`, max `1`, 0 if light is off, 1 if light is on | Not verified |

### Flight Computer Cool Switch

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| AV_COOL_SW | AV COOL Switch, NORM/EMERG<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74A0`, mask `0x4000`, shift `14`, max `1`, selector position | Not verified |

### Flight Control System Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| FCS_RESET_BTN | FCS RESET Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74B4`, mask `0x4000`, shift `14`, max `1`, selector position | Not verified |
| GAIN_SWITCH | GAIN Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74BC`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |
| GAIN_SWITCH_COVER | GAIN Switch Cover<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74B4`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| RUD_TRIM | RUD TRIM Control<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7528`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| TO_TRIM_BTN | T/O TRIM Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74B4`, mask `0x2000`, shift `13`, max `1`, selector position | Not verified |

### Fuel Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| EXT_CNT_TANK_SW | External Centerline Tank Fuel Control Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74B4`, mask `0x0600`, shift `9`, max `2`, selector position | Not verified |
| EXT_WNG_TANK_SW | External Wing Tanks Fuel Control Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74B4`, mask `0x1800`, shift `11`, max `2`, selector position | Not verified |
| FUEL_DUMP_SW | Fuel Dump Switch, ON/OFF<br>`action` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set the switch position -- 0 = off, 1 = on; max_value=1)<br>`action` (toggle switch state; argument=TOGGLE) | `integer` at `0x74B4`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |
| PROBE_SW | Probe Control Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74B0`, mask `0xC000`, shift `14`, max `2`, selector position | Not verified |

### Generator Tie Control Switch

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| GEN_TIE_COVER | Generator TIE Control Switch Cover, OPEN/CLOSE<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C2`, mask `0x1000`, shift `12`, max `1`, selector position | Not verified |
| GEN_TIE_SW | Generator TIE Control Switch, NORM/RESET<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C2`, mask `0x2000`, shift `13`, max `1`, selector position | Not verified |

### Ground Power Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| EXT_PWR_SW | External Power Switch<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74AC`, mask `0x0C00`, shift `10`, max `2`, selector position | Not verified |
| GND_PWR_1_SW | Ground Power Switch 1<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74AC`, mask `0x3000`, shift `12`, max `2`, selector position | Not verified |
| GND_PWR_2_SW | Ground Power Switch 2<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74AC`, mask `0xC000`, shift `14`, max `2`, selector position | Not verified |
| GND_PWR_3_SW | Ground Power Switch 3<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74B0`, mask `0x0300`, shift `8`, max `2`, selector position | Not verified |
| GND_PWR_4_SW | Ground Power Switch 4<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74B0`, mask `0x0C00`, shift `10`, max `2`, selector position | Not verified |

### HUD

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| HUD_ATC_NWS_ENGAGED | ATC - NWS Engaged<br>`display` | **Output only**<br>No documented input | `string` at `0x75A8`, length `6`, ATC - NWS Engaged | Not verified |
| HUD_LTDR | Laser Status<br>`display` | **Output only**<br>No documented input | `string` at `0x75A2`, length `5`, Laser Status | Not verified |

### HUD Control Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| HUD_ALT_SW | Altitude Switch, BARO/RDR<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742C`, mask `0x4000`, shift `14`, max `1`, selector position | Not verified |
| HUD_AOA_INDEXER | AOA Indexer Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x745E`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| HUD_ATT_SW | Attitude Selector Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x742E`, mask `0x0300`, shift `8`, max `2`, selector position | Not verified |
| HUD_BALANCE | Balance Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x745C`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| HUD_BLACK_LVL | Black Level Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x745A`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| HUD_SYM_BRT | HUD Symbology Brightness Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7458`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| HUD_SYM_BRT_SELECT | HUD Symbology Brightness Selector Knob, DAY/NIGHT<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742C`, mask `0x0800`, shift `11`, max `1`, selector position | Not verified |
| HUD_SYM_REJ_SW | HUD Symbology Reject Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x742C`, mask `0x0600`, shift `9`, max `2`, selector position | Not verified |
| HUD_VIDEO_CONTROL_SW | HUD Video Control Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x742C`, mask `0x3000`, shift `12`, max `2`, selector position | Not verified |

### HUD Video Bit Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| HUD_VIDEO_BIT | HUD Video BIT Initiate Pushbutton - Push to initiate BIT<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740A`, mask `0x0040`, shift `6`, max `1`, selector position | Not verified |

### HUD Video Record Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| IFEI | Brightness Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x74DE`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| MODE_SELECTOR_SW | Mode Selector Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x7466`, mask `0x1800`, shift `11`, max `2`, selector position | Not verified |
| SELECT_HMD_LDDI_RDDI | Selector Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x7466`, mask `0x0180`, shift `7`, max `2`, selector position | Not verified |
| SELECT_HUD_LDDI_RDDI | Selector Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x7466`, mask `0x0600`, shift `9`, max `2`, selector position | Not verified |

### HYD 1 and HYD Pressure Indicator

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| HYD_IND_LEFT | HYD Indicator Left<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x751E`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| HYD_IND_RIGHT | HYD Indicator Right<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7520`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |

### Heading and Course Set Switches

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| LEFT_DDI_CRS_SW | Course Set Switch<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74A8`, mask `0x6000`, shift `13`, max `2`, selector position | Not verified |
| LEFT_DDI_HDG_SW | Heading Set Switch<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74A8`, mask `0x1800`, shift `11`, max `2`, selector position | Not verified |

### Integrated Fuel/Engine Indicator (IFEI)

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| IFEI_BINGO | BINGO<br>`display` | **Output only**<br>No documented input | `string` at `0x7468`, length `5`, BINGO | Not verified |
| IFEI_BINGO_TEXTURE | BINGO Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74C6`, length `1`, BINGO Texture Visible: 1 = yes, 0 = no | Not verified |
| IFEI_CLOCK_H | CLOCK_H<br>`display` | **Output only**<br>No documented input | `string` at `0x746E`, length `2`, CLOCK_H | Not verified |
| IFEI_CLOCK_M | CLOCK_M<br>`display` | **Output only**<br>No documented input | `string` at `0x7470`, length `2`, CLOCK_M | Not verified |
| IFEI_CLOCK_S | CLOCK_S<br>`display` | **Output only**<br>No documented input | `string` at `0x7472`, length `2`, CLOCK_S | Not verified |
| IFEI_CODES | Codes<br>`display` | **Output only**<br>No documented input | `string` at `0x74AE`, length `3`, Codes | Not verified |
| IFEI_DD_1 | DD_1<br>`display` | **Output only**<br>No documented input | `string` at `0x747A`, length `1`, DD_1 | Not verified |
| IFEI_DD_2 | DD_2<br>`display` | **Output only**<br>No documented input | `string` at `0x747C`, length `1`, DD_2 | Not verified |
| IFEI_DD_3 | DD_3<br>`display` | **Output only**<br>No documented input | `string` at `0x747E`, length `1`, DD_3 | Not verified |
| IFEI_DD_4 | DD_4<br>`display` | **Output only**<br>No documented input | `string` at `0x7480`, length `1`, DD_4 | Not verified |
| IFEI_DWN_BTN | Down Arrow Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7466`, mask `0x0010`, shift `4`, max `1`, selector position | Not verified |
| IFEI_ET_BTN | ET Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7466`, mask `0x0040`, shift `6`, max `1`, selector position | Not verified |
| IFEI_FF_L | FF_L<br>`display` | **Output only**<br>No documented input | `string` at `0x7482`, length `3`, FF_L | Not verified |
| IFEI_FF_R | FF_R<br>`display` | **Output only**<br>No documented input | `string` at `0x7486`, length `3`, FF_R | Not verified |
| IFEI_FF_TEXTURE | FF Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74C0`, length `1`, FF Texture Visible: 1 = yes, 0 = no | Not verified |
| IFEI_FUEL_DOWN | FUEL_DOWN<br>`display` | **Output only**<br>No documented input | `string` at `0x748A`, length `6`, FUEL_DOWN | Not verified |
| IFEI_FUEL_UP | FUEL_UP<br>`display` | **Output only**<br>No documented input | `string` at `0x7490`, length `6`, FUEL_UP | Not verified |
| IFEI_L0_TEXTURE | Left 0 Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74CC`, length `1`, Left 0 Texture Visible: 1 = yes, 0 = no | Not verified |
| IFEI_L100_TEXTURE | Left 100 Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74D4`, length `1`, Left 100 Texture Visible: 1 = yes, 0 = no | Not verified |
| IFEI_L50_TEXTURE | Left 50 Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74D0`, length `1`, Left 50 Texture Visible: 1 = yes, 0 = no | Not verified |
| IFEI_LPOINTER_TEXTURE | Left Pointer Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74D8`, length `1`, Left Pointer Texture Visible: 1 = yes, 0 = no | Not verified |
| IFEI_LSCALE_TEXTURE | Left Scale Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74C8`, length `1`, Left Scale Texture Visible: 1 = yes, 0 = no | Not verified |
| IFEI_L_TEXTURE | Left Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x7582`, length `1`, Left Texture Visible: 1 = yes, 0 = no | Not verified |
| IFEI_MODE_BTN | Mode Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7466`, mask `0x0002`, shift `1`, max `1`, selector position | Not verified |
| IFEI_NOZ_TEXTURE | NOZZLE Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74C2`, length `1`, NOZZLE Texture Visible: 1 = yes, 0 = no | Not verified |
| IFEI_OIL_PRESS_L | OilPress_L<br>`display` | **Output only**<br>No documented input | `string` at `0x7496`, length `3`, OilPress_L | Not verified |
| IFEI_OIL_PRESS_R | OilPress_R<br>`display` | **Output only**<br>No documented input | `string` at `0x749A`, length `3`, OilPress_R | Not verified |
| IFEI_OIL_TEXTURE | OIL Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74C4`, length `1`, OIL Texture Visible: 1 = yes, 0 = no | Not verified |
| IFEI_QTY_BTN | QTY Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7466`, mask `0x0004`, shift `2`, max `1`, selector position | Not verified |
| IFEI_R0_TEXTURE | Right 0 Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74CE`, length `1`, Right 0 Texture Visible: 1 = yes, 0 = no | Not verified |
| IFEI_R100_TEXTURE | Right 100 Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74D6`, length `1`, Right 100 Texture Visible: 1 = yes, 0 = no | Not verified |
| IFEI_R50_TEXTURE | Right 50 Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74D2`, length `1`, Right 50 Texture Visible: 1 = yes, 0 = no | Not verified |
| IFEI_RPM_L | RPM_L<br>`display` | **Output only**<br>No documented input | `string` at `0x749E`, length `3`, RPM_L | Not verified |
| IFEI_RPM_R | RPM_R<br>`display` | **Output only**<br>No documented input | `string` at `0x74A2`, length `3`, RPM_R | Not verified |
| IFEI_RPM_TEXTURE | RPM Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74BC`, length `1`, RPM Texture Visible: 1 = yes, 0 = no | Not verified |
| IFEI_RPOINTER_TEXTURE | Right Pointer Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74DA`, length `1`, Right Pointer Texture Visible: 1 = yes, 0 = no | Not verified |
| IFEI_RSCALE_TEXTURE | Right Scale Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74CA`, length `1`, Right Scale Texture Visible: 1 = yes, 0 = no | Not verified |
| IFEI_R_TEXTURE | Right Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x7584`, length `1`, Right Texture Visible: 1 = yes, 0 = no | Not verified |
| IFEI_SP | SP<br>`display` | **Output only**<br>No documented input | `string` at `0x74B2`, length `3`, SP | Not verified |
| IFEI_T | T<br>`display` | **Output only**<br>No documented input | `string` at `0x757C`, length `6`, T | Not verified |
| IFEI_TEMP_L | TEMP_L<br>`display` | **Output only**<br>No documented input | `string` at `0x74A6`, length `3`, TEMP_L | Not verified |
| IFEI_TEMP_R | TEMP_R<br>`display` | **Output only**<br>No documented input | `string` at `0x74AA`, length `3`, TEMP_R | Not verified |
| IFEI_TEMP_TEXTURE | Temp Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74BE`, length `1`, Temp Texture Visible: 1 = yes, 0 = no | Not verified |
| IFEI_TIMER_H | TIMER_H<br>`display` | **Output only**<br>No documented input | `string` at `0x7474`, length `2`, TIMER_H | Not verified |
| IFEI_TIMER_M | TIMER_M<br>`display` | **Output only**<br>No documented input | `string` at `0x7476`, length `2`, TIMER_M | Not verified |
| IFEI_TIMER_S | TIMER_S<br>`display` | **Output only**<br>No documented input | `string` at `0x7478`, length `2`, TIMER_S | Not verified |
| IFEI_TIME_SET_MODE | Time Set Mode<br>`display` | **Output only**<br>No documented input | `string` at `0x74B6`, length `6`, Time Set Mode | Not verified |
| IFEI_UP_BTN | Up Arrow Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7466`, mask `0x0008`, shift `3`, max `1`, selector position | Not verified |
| IFEI_ZONE_BTN | ZONE Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7466`, mask `0x0020`, shift `5`, max `1`, selector position | Not verified |
| IFEI_Z_TEXTURE | Zulu Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74DC`, length `1`, Zulu Texture Visible: 1 = yes, 0 = no | Not verified |

### Interior Lights Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| CHART_DIMMER | CHART Light Dimmer<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x754A`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| COCKKPIT_LIGHT_MODE_SW | MODE Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74C8`, mask `0x0600`, shift `9`, max `2`, selector position | Not verified |
| CONSOLES_DIMMER | CONSOLES Lights Dimmer<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7544`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| FLOOD_DIMMER | FLOOD Light Dimmer<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7548`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| INST_PNL_DIMMER | INST PNL Dimmer<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7546`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| LIGHTS_TEST_SW | Lights Test Switch, TEST/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C8`, mask `0x0800`, shift `11`, max `1`, selector position | Not verified |
| WARN_CAUTION_DIMMER | WARN/CAUTION Light Dimmer<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x754C`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |

### Internal Canopy Switch

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| CANOPY_POS | Canopy Position<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7552`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| CANOPY_SW | Canopy Control Switch<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74CE`, mask `0x0300`, shift `8`, max `2`, selector position | Not verified |

### Internal Lights

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| CHART_INT_LT | Chart Lightning (light green)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x755E`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| CONSOLE_INT_LT | Console Lightning (light green)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7558`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| EMERG_INSTR_INT_LT | Emergency Instrument Lightning (light green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74D4`, mask `0x2000`, shift `13`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| ENG_INSTR_INT_LT | Eng Instrument Flood Lightning (light green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74D4`, mask `0x4000`, shift `14`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| FLOOD_INT_LT | Flood Lightning (light green)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x755A`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| IFEI_BTN_INT_LT | IFEI Buttons Lightning (light green)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7566`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| IFEI_DISP_INT_LT | IFEI Display Lightning (light green)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7564`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| INSTR_INT_LT | Instrument Lightning (light green)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7560`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| NVG_FLOOD_INT_LT | Nvg Flood Lightning (light green)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x755C`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| STBY_COMPASS_INT_LT | Stby Compass Lightning (light green)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7562`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |

### KY-58 Control

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| KY58_FILL_SELECT | KY-58 Fill Select Knob<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=7) | `integer` at `0x74CC`, mask `0x0700`, shift `8`, max `7`, selector position | Not verified |
| KY58_FILL_SEL_PULL | KY-58 Fill Select Knob, Pull<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D8`, mask `0x0200`, shift `9`, max `1`, selector position | Not verified |
| KY58_MODE_SELECT | KY-58 Mode Select Knob<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=3) | `integer` at `0x74CA`, mask `0xC000`, shift `14`, max `3`, selector position | Not verified |
| KY58_POWER_SELECT | KY-58 Power Select Knob<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74CC`, mask `0x1800`, shift `11`, max `2`, selector position | Not verified |
| KY58_VOLUME | KY-58 Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x754E`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |

### LH Advisory Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| LH_ADV_ASPJ_OH | ASPJ OH (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0008`, shift `3`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| LH_ADV_GO | GO (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0010`, shift `4`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| LH_ADV_L_BAR_GREEN | L BAR (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0002`, shift `1`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| LH_ADV_L_BAR_RED | L BAR (red)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x8000`, shift `15`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| LH_ADV_L_BLEED | L BLEED (red)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x0800`, shift `11`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| LH_ADV_NO_GO | NO GO (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0020`, shift `5`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| LH_ADV_REC | REC (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0001`, shift `0`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| LH_ADV_R_BLEED | R BLEED (red)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x1000`, shift `12`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| LH_ADV_SPD_BRK | SPD BRK (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x2000`, shift `13`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| LH_ADV_STBY | STBY (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x4000`, shift `14`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| LH_ADV_XMIT | XMIT (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0004`, shift `2`, max `1`, 0 if light is off, 1 if light is on | Not verified |

### LOX Indicator

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| OBOGS_SW | OBOGS Control Switch, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C0`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |
| OXY_FLOW | OXY Flow Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x753A`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |

### Landing Gear Handle and Warning Tone Silence

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| EMERGENCY_GEAR_ROTATE | Emergency Gear Rotate<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747E`, mask `0x2000`, shift `13`, max `1`, selector position | Not verified |
| GEAR_DOWNLOCK_OVERRIDE_BTN | Landing Gear Override<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747E`, mask `0x4000`, shift `14`, max `1`, selector position | Not verified |
| GEAR_LEVER | Gear Lever<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747E`, mask `0x1000`, shift `12`, max `1`, selector position | Not verified |
| GEAR_SILENCE_BTN | Warning Tone Silence Button - Push to silence<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747E`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| LANDING_GEAR_HANDLE_LT | Landing Gear Handle Light<br>`led` | **Output only**<br>No documented input | `integer` at `0x747E`, mask `0x0800`, shift `11`, max `1`, 0 if light is off, 1 if light is on | Not verified |

### Left DDI

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| LEFT_DDI_BRT_CTL | Brightness Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7410`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| LEFT_DDI_BRT_SELECT | Brightness Selector Knob<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x740E`, mask `0x0006`, shift `1`, max `2`, selector position | Not verified |
| LEFT_DDI_CONT_CTL | Contrast Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7412`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| LEFT_DDI_PB_01 | Pushbutton 1<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x0008`, shift `3`, max `1`, selector position | Not verified |
| LEFT_DDI_PB_02 | Pushbutton 2<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x0010`, shift `4`, max `1`, selector position | Not verified |
| LEFT_DDI_PB_03 | Pushbutton 3<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x0020`, shift `5`, max `1`, selector position | Not verified |
| LEFT_DDI_PB_04 | Pushbutton 4<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x0040`, shift `6`, max `1`, selector position | Not verified |
| LEFT_DDI_PB_05 | Pushbutton 5<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x0080`, shift `7`, max `1`, selector position | Not verified |
| LEFT_DDI_PB_06 | Pushbutton 6<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |
| LEFT_DDI_PB_07 | Pushbutton 7<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x0200`, shift `9`, max `1`, selector position | Not verified |
| LEFT_DDI_PB_08 | Pushbutton 8<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x0400`, shift `10`, max `1`, selector position | Not verified |
| LEFT_DDI_PB_09 | Pushbutton 9<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x0800`, shift `11`, max `1`, selector position | Not verified |
| LEFT_DDI_PB_10 | Pushbutton 10<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x1000`, shift `12`, max `1`, selector position | Not verified |
| LEFT_DDI_PB_11 | Pushbutton 11<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x2000`, shift `13`, max `1`, selector position | Not verified |
| LEFT_DDI_PB_12 | Pushbutton 12<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x4000`, shift `14`, max `1`, selector position | Not verified |
| LEFT_DDI_PB_13 | Pushbutton 13<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| LEFT_DDI_PB_14 | Pushbutton 14<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0001`, shift `0`, max `1`, selector position | Not verified |
| LEFT_DDI_PB_15 | Pushbutton 15<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0002`, shift `1`, max `1`, selector position | Not verified |
| LEFT_DDI_PB_16 | Pushbutton 16<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0004`, shift `2`, max `1`, selector position | Not verified |
| LEFT_DDI_PB_17 | Pushbutton 17<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0008`, shift `3`, max `1`, selector position | Not verified |
| LEFT_DDI_PB_18 | Pushbutton 18<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0010`, shift `4`, max `1`, selector position | Not verified |
| LEFT_DDI_PB_19 | Pushbutton 19<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0020`, shift `5`, max `1`, selector position | Not verified |
| LEFT_DDI_PB_20 | Pushbutton 20<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0040`, shift `6`, max `1`, selector position | Not verified |

### Left Engine Fire Warning Extinguisher Light

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| FIRE_LEFT_LT | FIRE LEFT (red)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x0040`, shift `6`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| LEFT_FIRE_BTN | Left Engine/AMAD Fire Warning/Extinguisher Light<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7408`, mask `0x0080`, shift `7`, max `1`, selector position | Not verified |
| LEFT_FIRE_BTN_COVER | Left Engine/AMAD Fire Warning Cover<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7408`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |

### Left Essential Circuit Breakers

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| CB_FCS_CHAN1 | CB FCS CHAN 1, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C2`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| CB_FCS_CHAN2 | CB FCS CHAN 2, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C4`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |
| CB_LAUNCH_BAR | CB LAUNCH BAR, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C4`, mask `0x0400`, shift `10`, max `1`, selector position | Not verified |
| CB_SPD_BRK | CB SPD BRK, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C4`, mask `0x0200`, shift `9`, max `1`, selector position | Not verified |

### Lock Shoot Lights

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| LS_LOCK | LOCK (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x0001`, shift `0`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| LS_SHOOT | SHOOT (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x0002`, shift `1`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| LS_SHOOT_STROBE | SHOOT STROBE (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x0004`, shift `2`, max `1`, 0 if light is off, 1 if light is on | Not verified |

### Map Gain/Spin Recovery Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| HMD_OFF_BRT | HMD OFF/BRT Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7456`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| IR_COOL_SW | IR Cooling Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x742A`, mask `0xC000`, shift `14`, max `2`, selector position | Not verified |
| SPIN_LT | Spin Light (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x742A`, mask `0x0800`, shift `11`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| SPIN_RECOVERY_COVER | Spin Recovery Switch Cover, OPEN/CLOSE<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742A`, mask `0x1000`, shift `12`, max `1`, selector position | Not verified |
| SPIN_RECOVERY_SW | Spin Recovery Switch, RCVY/NORM<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742A`, mask `0x2000`, shift `13`, max `1`, selector position | Not verified |

### Master Arm Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| MASTER_ARM_SW | Master Arm Switch, ARM/SAFE<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740C`, mask `0x2000`, shift `13`, max `1`, selector position | Not verified |
| MASTER_MODE_AA | Master Mode Button, A/A<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740C`, mask `0x0800`, shift `11`, max `1`, selector position | Not verified |
| MASTER_MODE_AA_LT | AA Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740C`, mask `0x0200`, shift `9`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| MASTER_MODE_AG | Master Mode Button, A/G<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740C`, mask `0x1000`, shift `12`, max `1`, selector position | Not verified |
| MASTER_MODE_AG_LT | AG Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740C`, mask `0x0400`, shift `10`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| MC_DISCH | DISCH Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740C`, mask `0x4000`, shift `14`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| MC_READY | READY Light (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740C`, mask `0x8000`, shift `15`, max `1`, 0 if light is off, 1 if light is on | Not verified |

### Master Caution Light

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| MASTER_CAUTION_LT | MASTER CAUTION (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x0200`, shift `9`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| MASTER_CAUTION_RESET_SW | MASTER CAUTION Reset Button - Press to reset<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7408`, mask `0x0400`, shift `10`, max `1`, selector position | Not verified |

### Mission Computer and Hydraulic Isolate Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| HYD_ISOLATE_OVERRIDE_SW | Hydraulic Isolate Override Switch, NORM/ORIDE<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C0`, mask `0x0800`, shift `11`, max `1`, selector position | Not verified |
| MC_SW | MC Switch<br>`mission_computer_switch` | **Interactable**<br>`set_state` (set the switch position -- 0 = 1OFF, 1 = NORM, 2 = 2OFF; max_value=2) | `integer` at `0x74C0`, mask `0x0600`, shift `9`, max `2`, switch position -- 0 = 1OFF, 1 = NORM, 2 = 2OFF | Not verified |

### RH Advisory Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| RH_ADV_AAA | AAA (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0800`, shift `11`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| RH_ADV_AI | AI (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0400`, shift `10`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| RH_ADV_CW | CW (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x1000`, shift `12`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| RH_ADV_DISP | DISP (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0100`, shift `8`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| RH_ADV_RCDR_ON | RCDR ON (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0080`, shift `7`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| RH_ADV_SAM | SAM (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0200`, shift `9`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| RH_ADV_SPARE_RH1 | SPARE RH1 (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x2000`, shift `13`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| RH_ADV_SPARE_RH2 | SPARE RH2 (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x4000`, shift `14`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| RH_ADV_SPARE_RH3 | SPARE RH3 (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x8000`, shift `15`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| RH_ADV_SPARE_RH4 | SPARE RH4 (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740C`, mask `0x0001`, shift `0`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| RH_ADV_SPARE_RH5 | SPARE RH5 (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740C`, mask `0x0002`, shift `1`, max `1`, 0 if light is off, 1 if light is on | Not verified |

### RWR Control Indicator

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| RWR_AUDIO_CTRL | ALR-67 AUDIO Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7554`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| RWR_BIT_BTN | ALR-67 BIT Pushbutton<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7498`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |
| RWR_BIT_LT | ALR-67 BIT Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x749C`, mask `0x1000`, shift `12`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| RWR_DISPLAY_BTN | ALR-67 DISPLAY Pushbutton<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7488`, mask `0x2000`, shift `13`, max `1`, selector position | Not verified |
| RWR_DISPLAY_LT | ALR-67 DISPLAY Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7498`, mask `0x4000`, shift `14`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| RWR_DIS_TYPE_SW | ALR-67 DIS TYPE Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=4) | `integer` at `0x7498`, mask `0x0E00`, shift `9`, max `4`, selector position | Not verified |
| RWR_DMR_CTRL | ALR-67 DMR Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7508`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| RWR_ENABLE_LT | ALR-67 ENABLE Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x749C`, mask `0x0200`, shift `9`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| RWR_FAIL_LT | ALR-67 FAIL Light (red)<br>`led` | **Output only**<br>No documented input | `integer` at `0x749C`, mask `0x0800`, shift `11`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| RWR_LIMIT_LT | ALR-67 LIMIT Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7498`, mask `0x2000`, shift `13`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| RWR_LOWER_LT | ALR-67 POWER Light ON (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7498`, mask `0x1000`, shift `12`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| RWR_LT_BRIGHT | RWR Lights Brightness<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7568`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| RWR_OFFSET_BTN | ALR-67 OFFSET Pushbutton<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7488`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| RWR_OFFSET_LT | ALR-67 OFFSET Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x749C`, mask `0x0400`, shift `10`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| RWR_POWER_BTN | ALR-67 POWER Pushbutton<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7488`, mask `0x1000`, shift `12`, max `1`, selector position | Not verified |
| RWR_RWR_INTESITY | RWR Intensity Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x750A`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| RWR_SPECIAL_BTN | ALR-67 SPECIAL Pushbutton<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7488`, mask `0x4000`, shift `14`, max `1`, selector position | Not verified |
| RWR_SPECIAL_EN_LT | ALR-67 SPECIAL ENABLE Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7498`, mask `0x8000`, shift `15`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| RWR_SPECIAL_LT | ALR-67 SPECIAL Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x749C`, mask `0x0100`, shift `8`, max `1`, 0 if light is off, 1 if light is on | Not verified |

### Radar Altimeter

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| LOW_ALT_WARN_LT | Low Alt Warning (red)<br>`led` | **Output only**<br>No documented input | `integer` at `0x749C`, mask `0x8000`, shift `15`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| RADALT_ALT_PTR | Altitude Pointer<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x751A`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| RADALT_GREEN_LAMP | Radar Altimeter Green Lamp (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A0`, mask `0x0100`, shift `8`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| RADALT_HEIGHT | Set low altitude pointer<br>`analog_dial` | **Interactable**<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7516`, mask `0xFFFF`, shift `0`, max `65535`, the rotation of the knob in the cockpit (not the value that is controlled by this knob!) | Not verified |
| RADALT_MIN_HEIGHT_PTR | Min Height Pointer<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7518`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| RADALT_OFF_FLAG | OFF Flag<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x751C`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| RADALT_TEST_SW | Push to Test Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x749C`, mask `0x4000`, shift `14`, max `1`, selector position | Not verified |

### Radio Frequencies

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| COMM1 | COMM1 Radio<br>`radio` | **Interactable**<br>`set_string` (The frequency to set, with or without a decimal place) | `string` at `0x7592`, length `7`, The current frequency the radio is set to | Not verified |
| COMM2 | COMM2 Radio<br>`radio` | **Interactable**<br>`set_string` (The frequency to set, with or without a decimal place) | `string` at `0x759A`, length `7`, The current frequency the radio is set to | Not verified |

### Right DDI

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| RIGHT_DDI_BRT_CTL | Brightness Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7452`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| RIGHT_DDI_BRT_SELECT | Brightness Selector Knob<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x7418`, mask `0x0060`, shift `5`, max `2`, selector position | Not verified |
| RIGHT_DDI_CONT_CTL | Contrast Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7454`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| RIGHT_DDI_PB_01 | Pushbutton 1<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x0080`, shift `7`, max `1`, selector position | Not verified |
| RIGHT_DDI_PB_02 | Pushbutton 2<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |
| RIGHT_DDI_PB_03 | Pushbutton 3<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x0200`, shift `9`, max `1`, selector position | Not verified |
| RIGHT_DDI_PB_04 | Pushbutton 4<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x0400`, shift `10`, max `1`, selector position | Not verified |
| RIGHT_DDI_PB_05 | Pushbutton 5<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x0800`, shift `11`, max `1`, selector position | Not verified |
| RIGHT_DDI_PB_06 | Pushbutton 6<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x1000`, shift `12`, max `1`, selector position | Not verified |
| RIGHT_DDI_PB_07 | Pushbutton 7<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x2000`, shift `13`, max `1`, selector position | Not verified |
| RIGHT_DDI_PB_08 | Pushbutton 8<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x4000`, shift `14`, max `1`, selector position | Not verified |
| RIGHT_DDI_PB_09 | Pushbutton 9<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| RIGHT_DDI_PB_10 | Pushbutton 10<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7428`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |
| RIGHT_DDI_PB_11 | Pushbutton 11<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7428`, mask `0x0200`, shift `9`, max `1`, selector position | Not verified |
| RIGHT_DDI_PB_12 | Pushbutton 12<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7428`, mask `0x0400`, shift `10`, max `1`, selector position | Not verified |
| RIGHT_DDI_PB_13 | Pushbutton 13<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7428`, mask `0x0800`, shift `11`, max `1`, selector position | Not verified |
| RIGHT_DDI_PB_14 | Pushbutton 14<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7428`, mask `0x1000`, shift `12`, max `1`, selector position | Not verified |
| RIGHT_DDI_PB_15 | Pushbutton 15<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7428`, mask `0x2000`, shift `13`, max `1`, selector position | Not verified |
| RIGHT_DDI_PB_16 | Pushbutton 16<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7428`, mask `0x4000`, shift `14`, max `1`, selector position | Not verified |
| RIGHT_DDI_PB_17 | Pushbutton 17<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7428`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| RIGHT_DDI_PB_18 | Pushbutton 18<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742A`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |
| RIGHT_DDI_PB_19 | Pushbutton 19<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742A`, mask `0x0200`, shift `9`, max `1`, selector position | Not verified |
| RIGHT_DDI_PB_20 | Pushbutton 20<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742A`, mask `0x0400`, shift `10`, max `1`, selector position | Not verified |

### Right Engine Fire Warning Extinguisher Light

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| FIRE_RIGHT_LT | FIRE RIGHT (red)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740C`, mask `0x0010`, shift `4`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| RIGHT_FIRE_BTN | Right Engine/AMAD Fire Warning/Extinguisher Light<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740C`, mask `0x0020`, shift `5`, max `1`, selector position | Not verified |
| RIGHT_FIRE_BTN_COVER | Right Engine/AMAD Fire Warning Cover<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740C`, mask `0x0040`, shift `6`, max `1`, selector position | Not verified |

### Right Essential Circuit Breakers

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| CB_FCS_CHAN3 | CB FCS CHAN 3, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74CC`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| CB_FCS_CHAN4 | CB FCS CHAN 4, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74CE`, mask `0x0400`, shift `10`, max `1`, selector position | Not verified |
| CB_HOOOK | CB HOOK, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74CE`, mask `0x0800`, shift `11`, max `1`, selector position | Not verified |
| CB_LG | CB LG, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74CE`, mask `0x1000`, shift `12`, max `1`, selector position | Not verified |
| FCS_BIT_SW | FCS BIT Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74CE`, mask `0x2000`, shift `13`, max `1`, selector position | Not verified |

### Rudder Pedal Adjust Level

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| RUDDER_PEDAL_ADJUST | Rudder Pedal Adjust Lever<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x749C`, mask `0x2000`, shift `13`, max `1`, selector position | Not verified |

### Select Jettison Button

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| ANTI_SKID_SW | Anti Skid<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7480`, mask `0x1000`, shift `12`, max `1`, selector position | Not verified |
| FLAP_SW | FLAP Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x7484`, mask `0x0300`, shift `8`, max `2`, selector position | Not verified |
| HOOK_BYPASS_SW | HOOK BYPASS Switch, FIELD/CARRIER<br>`action` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set the switch position -- 0 = off, 1 = on; max_value=1)<br>`action` (toggle switch state; argument=TOGGLE) | `integer` at `0x7480`, mask `0x4000`, shift `14`, max `1`, selector position | Not verified |
| HYD_IND_BRAKE | HYD Indicator Brake<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7506`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| LAUNCH_BAR_SW | Launch Bar Switch<br>`action` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set the switch position -- 0 = off, 1 = on; max_value=1)<br>`action` (toggle switch state; argument=TOGGLE) | `integer` at `0x7480`, mask `0x2000`, shift `13`, max `1`, selector position | Not verified |
| LDG_TAXI_SW | LDG/TAXI LIGHT Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7480`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| SEL_JETT_BTN | Selective Jettison Pushbutton<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7480`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |
| SEL_JETT_KNOB | Selective Jettison Knob<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=4) | `integer` at `0x7480`, mask `0x0E00`, shift `9`, max `4`, selector position | Not verified |

### Sensor Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| FLIR_SW | FLIR Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74C8`, mask `0x3000`, shift `12`, max `2`, selector position | Not verified |
| INS_SW | INS Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=7) | `integer` at `0x74CA`, mask `0x3800`, shift `11`, max `7`, selector position | Not verified |
| LST_NFLR_SW | LST/NFLR Switch, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C8`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| LTD_R_SW | LTD/R Switch, ARM/SAFE<br>`action` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set the switch position -- 0 = off, 1 = on; max_value=1)<br>`action` (toggle switch state; argument=TOGGLE) | `integer` at `0x74C8`, mask `0x4000`, shift `14`, max `1`, selector position | Not verified |
| RADAR_SW | RADAR Switch Change ,OFF/STBY/OPR/EMERG(PULL)<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=3) | `integer` at `0x74CA`, mask `0x0300`, shift `8`, max `3`, selector position | Not verified |
| RADAR_SW_PULL | RADAR Switch Pull (MW to pull), OFF/STBY/OPR/EMERG(PULL)<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74CA`, mask `0x0400`, shift `10`, max `1`, selector position | Not verified |

### Standby Airspeed Indicator

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| STBY_ASI_AIRSPEED | Airspeed<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74F0`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |

### Standby Altimeter

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| STBY_ALT_10000_FT_CNT | 10000 ft count<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74F6`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| STBY_ALT_1000_FT_CNT | 1000 ft count<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74F8`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| STBY_ALT_100_FT_PTR | 100 ft pointer<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74F4`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| STBY_PRESS_ALT | Pressure Setting Knob<br>`analog_dial` | **Interactable**<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x74F2`, mask `0xFFFF`, shift `0`, max `65535`, the rotation of the knob in the cockpit (not the value that is controlled by this knob!) | Not verified |
| STBY_PRESS_SET_0 | Pressure Setting 1<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74FA`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| STBY_PRESS_SET_1 | Pressure Setting 2<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74FC`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| STBY_PRESS_SET_2 | Pressure Setting 3<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74FE`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |

### Standby Attitude Reference Indicator

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| SAI_ATT_WARNING_FLAG | SAI Attitude Warning Flag<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74E8`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| SAI_ATT_WARN_FLAG_L | SAI Attitude Warning Flag as Light<br>`led` | **Output only**<br>No documented input | `integer` at `0x74D6`, mask `0x0100`, shift `8`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| SAI_BANK | SAI Bank<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74E6`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| SAI_CAGE | SAI Pull to uncage<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747E`, mask `0x0400`, shift `10`, max `1`, selector position | Not verified |
| SAI_MAN_PITCH_ADJ | SAI Manual Pitch Adjustment<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74EA`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| SAI_PITCH | SAI Pitch<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74E4`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| SAI_POINTER_HOR | SAI Horisontal Pointer<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x756C`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| SAI_POINTER_VER | SAI Vertical Pointer<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x756A`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| SAI_RATE_OF_TURN | SAI Rate Of Turn<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74EE`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| SAI_SET | SAI Adjust Attitude<br>`analog_dial` | **Interactable**<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x74E2`, mask `0xFFFF`, shift `0`, max `65535`, the rotation of the knob in the cockpit (not the value that is controlled by this knob!) | Not verified |
| SAI_SLIP_BALL | SAI Slip Ball<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74EC`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| SAI_TEST_BTN | SAI Test Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747E`, mask `0x0200`, shift `9`, max `1`, selector position | Not verified |

### Standby Compass

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| SBY_COMPASS_BANK | Standby Compass Bank<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7464`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| SBY_COMPASS_HDG | Standby Compass Heading<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7460`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| SBY_COMPASS_PITCH | Standby Compass Pitch<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7462`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |

### Standby Rate of Climb Indicator

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| VSI | Vertical Speed<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7500`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |

### Station Jettison Select

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| SJ_CTR | Station Jettison Select Button, CENTER<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742C`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| SJ_CTR_LT | CTR Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x742E`, mask `0x4000`, shift `14`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| SJ_LI | Station Jettison Select Button, LEFT IN<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742E`, mask `0x0400`, shift `10`, max `1`, selector position | Not verified |
| SJ_LI_LT | LI Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x742E`, mask `0x8000`, shift `15`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| SJ_LO | Station Jettison Select Button, LEFT OUT<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742E`, mask `0x0800`, shift `11`, max `1`, selector position | Not verified |
| SJ_LO_LT | LO Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7430`, mask `0x0100`, shift `8`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| SJ_RI | Station Jettison Select Button, RIGHT IN<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742E`, mask `0x1000`, shift `12`, max `1`, selector position | Not verified |
| SJ_RI_LT | RI Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7430`, mask `0x0200`, shift `9`, max `1`, 0 if light is off, 1 if light is on | Not verified |
| SJ_RO | Station Jettison Select Button, RIGHT OUT<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742E`, mask `0x2000`, shift `13`, max `1`, selector position | Not verified |
| SJ_RO_LT | RO Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7430`, mask `0x0400`, shift `10`, max `1`, 0 if light is off, 1 if light is on | Not verified |

### Stick

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| STICK_GUN_TRIGGER2 | Stick Gun Trigger, SECOND DETENT (Press to shoot)<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D2`, mask `0x0400`, shift `10`, max `1`, selector position | Not verified |
| STICK_N_WHEEL_SW | Stick Undesignate/Nose Wheel Steer Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D2`, mask `0x1000`, shift `12`, max `1`, selector position | Not verified |
| STICK_PADDLE_SW | Stick Autopilot/Nosewheel Steering Disengage (Paddle) Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D2`, mask `0x0800`, shift `11`, max `1`, selector position | Not verified |
| STICK_RECCE_SW | Stick RECCE Event Mark Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D2`, mask `0x0200`, shift `9`, max `1`, selector position | Not verified |
| STICK_WEAP_REL_BTN | Stick Weapon Release Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D2`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |

### TODO

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| LEFT_VIDEO_BIT | Left Video Sensor BIT Initiate Pushbutton - Push to initiate BIT<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D0`, mask `0x2000`, shift `13`, max `1`, selector position | Not verified |
| NUC_WPN_SW | NUC WPN Switch, ENABLE/DISABLE (no function)<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D0`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| RIGHT_VIDEO_BIT | Right Video Sensor BIT Initiate Pushbutton - Push to initiate BIT<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D0`, mask `0x4000`, shift `14`, max `1`, selector position | Not verified |

### Throttle Quadrant

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| INT_THROTTLE_LEFT | Left Throttle Position<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7588`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| INT_THROTTLE_RIGHT | Right Throttle Position<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x758A`, mask `0xFFFF`, shift `0`, max `65535`, gauge position | Not verified |
| THROTTLE_ATC_SW | Throttle ATC Engage/Disengage Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D4`, mask `0x0400`, shift `10`, max `1`, selector position | Not verified |
| THROTTLE_CAGE_BTN | Throttle Cage/Uncage Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D2`, mask `0x2000`, shift `13`, max `1`, selector position | Not verified |
| THROTTLE_DISP_SW | Throttle Dispense Switch, Aft(FLARE)/Center(OFF)/Forward(CHAFF)<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74D2`, mask `0xC000`, shift `14`, max `2`, selector position | Not verified |
| THROTTLE_EXT_L_SW | Throttle Exterior Lights Switch, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D4`, mask `0x1000`, shift `12`, max `1`, selector position | Not verified |
| THROTTLE_FOV_SEL_SW | Throttle RAID/FLIR FOV Select Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D4`, mask `0x0800`, shift `11`, max `1`, selector position | Not verified |
| THROTTLE_FRICTION | Throttles Friction Adjusting Lever<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7522`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| THROTTLE_RADAR_ELEV | Throttle Radar Elevation Control<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7556`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| THROTTLE_SPEED_BRK | Throttle Speed Brake Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74D4`, mask `0x0300`, shift `8`, max `2`, selector position | Not verified |

### Up Front Controller (UFC)

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| UFC_0 | UFC Keyboard Pushbutton, 0<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x0200`, shift `9`, max `1`, selector position | Not verified |
| UFC_1 | UFC Keyboard Pushbutton, 1<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x0400`, shift `10`, max `1`, selector position | Not verified |
| UFC_2 | UFC Keyboard Pushbutton, 2<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x0800`, shift `11`, max `1`, selector position | Not verified |
| UFC_3 | UFC Keyboard Pushbutton, 3<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x1000`, shift `12`, max `1`, selector position | Not verified |
| UFC_4 | UFC Keyboard Pushbutton, 4<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x2000`, shift `13`, max `1`, selector position | Not verified |
| UFC_5 | UFC Keyboard Pushbutton, 5<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x4000`, shift `14`, max `1`, selector position | Not verified |
| UFC_6 | UFC Keyboard Pushbutton, 6<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| UFC_7 | UFC Keyboard Pushbutton, 7<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x0001`, shift `0`, max `1`, selector position | Not verified |
| UFC_8 | UFC Keyboard Pushbutton, 8<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x0002`, shift `1`, max `1`, selector position | Not verified |
| UFC_9 | UFC Keyboard Pushbutton, 9<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x0004`, shift `2`, max `1`, selector position | Not verified |
| UFC_ADF | ADF Function Select Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x7416`, mask `0x00C0`, shift `6`, max `2`, selector position | Not verified |
| UFC_AP | Function Selector Pushbutton, A/P<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0080`, shift `7`, max `1`, selector position | Not verified |
| UFC_BCN | Function Selector Pushbutton, BCN<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x1000`, shift `12`, max `1`, selector position | Not verified |
| UFC_BRT | Brightness Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x741E`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| UFC_CLR | Keyboard Pushbutton, CLR<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x0008`, shift `3`, max `1`, selector position | Not verified |
| UFC_COMM1_CHANNEL_SELECT | COMM 1 Channel Select Knob<br>`fixed_step_dial` | **Interactable**<br>`fixed_step` (turn left or right) | No output descriptor in this reference | Not verified |
| UFC_COMM1_DISPLAY | Comm 1 Display<br>`display` | **Output only**<br>No documented input | `string` at `0x7424`, length `2`, Comm 1 Display | Not verified |
| UFC_COMM1_PULL | COMM 1 Channel Selector Knob Pull<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x4000`, shift `14`, max `1`, selector position | Not verified |
| UFC_COMM1_VOL | COMM 1 Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x741A`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| UFC_COMM2_CHANNEL_SELECT | COMM 2 Channel Select Knob<br>`fixed_step_dial` | **Interactable**<br>`fixed_step` (turn left or right) | No output descriptor in this reference | Not verified |
| UFC_COMM2_DISPLAY | Comm 2 Display<br>`display` | **Output only**<br>No documented input | `string` at `0x7426`, length `2`, Comm 2 Display | Not verified |
| UFC_COMM2_PULL | COMM 2 Channel Selector Knob Pull<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x8000`, shift `15`, max `1`, selector position | Not verified |
| UFC_COMM2_VOL | COMM 2 Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x741C`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer | Not verified |
| UFC_DL | Function Selector Pushbutton, D/L<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0800`, shift `11`, max `1`, selector position | Not verified |
| UFC_EMCON | Emission Control Pushbutton<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |
| UFC_ENT | Keyboard Pushbutton, ENT<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x0010`, shift `4`, max `1`, selector position | Not verified |
| UFC_IFF | Function Selector Pushbutton, IFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0100`, shift `8`, max `1`, selector position | Not verified |
| UFC_ILS | Function Selector Pushbutton, ILS<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0400`, shift `10`, max `1`, selector position | Not verified |
| UFC_IP | I/P Pushbutton<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x0020`, shift `5`, max `1`, selector position | Not verified |
| UFC_ONOFF | Function Selector Pushbutton, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x2000`, shift `13`, max `1`, selector position | Not verified |
| UFC_OPTION_CUEING_1 | Option Cueing 1<br>`display` | **Output only**<br>No documented input | `string` at `0x7428`, length `1`, Option Cueing 1 | Not verified |
| UFC_OPTION_CUEING_2 | Option Cueing 2<br>`display` | **Output only**<br>No documented input | `string` at `0x742A`, length `1`, Option Cueing 2 | Not verified |
| UFC_OPTION_CUEING_3 | Option Cueing 3<br>`display` | **Output only**<br>No documented input | `string` at `0x742C`, length `1`, Option Cueing 3 | Not verified |
| UFC_OPTION_CUEING_4 | Option Cueing 4<br>`display` | **Output only**<br>No documented input | `string` at `0x742E`, length `1`, Option Cueing 4 | Not verified |
| UFC_OPTION_CUEING_5 | Option Cueing 5<br>`display` | **Output only**<br>No documented input | `string` at `0x7430`, length `1`, Option Cueing 5 | Not verified |
| UFC_OPTION_DISPLAY_1 | Option Display 1<br>`display` | **Output only**<br>No documented input | `string` at `0x7432`, length `4`, Option Display 1 | Not verified |
| UFC_OPTION_DISPLAY_2 | Option Display 2<br>`display` | **Output only**<br>No documented input | `string` at `0x7436`, length `4`, Option Display 2 | Not verified |
| UFC_OPTION_DISPLAY_3 | Option Display 3<br>`display` | **Output only**<br>No documented input | `string` at `0x743A`, length `4`, Option Display 3 | Not verified |
| UFC_OPTION_DISPLAY_4 | Option Display 4<br>`display` | **Output only**<br>No documented input | `string` at `0x743E`, length `4`, Option Display 4 | Not verified |
| UFC_OPTION_DISPLAY_5 | Option Display 5<br>`display` | **Output only**<br>No documented input | `string` at `0x7442`, length `4`, Option Display 5 | Not verified |
| UFC_OS1 | Option Select Pushbutton 1<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x0001`, shift `0`, max `1`, selector position | Not verified |
| UFC_OS2 | Option Select Pushbutton 2<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x0002`, shift `1`, max `1`, selector position | Not verified |
| UFC_OS3 | Option Select Pushbutton 3<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x0004`, shift `2`, max `1`, selector position | Not verified |
| UFC_OS4 | Option Select Pushbutton 4<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x0008`, shift `3`, max `1`, selector position | Not verified |
| UFC_OS5 | Option Select Pushbutton 5<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x0010`, shift `4`, max `1`, selector position | Not verified |
| UFC_SCRATCHPAD_NUMBER_DISPLAY | Scratchpad Number Display<br>`display` | **Output only**<br>No documented input | `string` at `0x7446`, length `8`, Scratchpad Number Display | Not verified |
| UFC_SCRATCHPAD_STRING_1_DISPLAY | Scratchpad String 1 Display<br>`display` | **Output only**<br>No documented input | `string` at `0x744E`, length `2`, Scratchpad String 1 Display | Not verified |
| UFC_SCRATCHPAD_STRING_2_DISPLAY | Scratchpad String 2 Display<br>`display` | **Output only**<br>No documented input | `string` at `0x7450`, length `2`, Scratchpad String 2 Display | Not verified |
| UFC_TCN | Function Selector Pushbutton, TCN<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0200`, shift `9`, max `1`, selector position | Not verified |

### Wing Fold Switch

| Control | Description / kind | Interaction | DCS-BIOS output reference | Native DCS evidence |
|---|---|---|---|---|
| WING_FOLD_PULL | Wing Fold Control Handle Pull<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74A0`, mask `0x0800`, shift `11`, max `1`, selector position | Not verified |
| WING_FOLD_ROTATE | Wing Fold Control Handle<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74A0`, mask `0x3000`, shift `12`, max `2`, selector position | Not verified |

## Establishing native DCS evidence

This baseline can be replaced or supplemented by evidence captured from DCS. Keep native observations distinct from DCS-BIOS addresses and IDs; do not infer an address correspondence from matching values alone.

For each native observation, retain: DCS build and aircraft variant; mission/start conditions; control or output exercised; raw capture file and SHA-256; frame/record offset and bytes; observed value before and after the action; repeated result; and verification status. An observation can verify a value/address transition, but a capture cannot establish that an unobserved output does not exist. Mark those as **not observed**, not **absent**. Keep the release baseline and native evidence side by side until coverage is sufficient to promote the native source.

### Native observation log template

| DCS build / variant | Equipment / control | Scenario or action | Capture SHA-256 | Native frame/address/bytes | Result / status |
|---|---|---|---|---|---|
| Pending live DCS capture | | | | | Not verified |

## Limitations

- The inventory includes controls represented in the selected DCS-BIOS reference JSON, not every internal DCS state or cockpit item.
- The DCS-BIOS category is retained as the grouping label; verify physical placement and naming against the Hornet cockpit and DCS documentation.
- No DCS runtime is available in the generation environment. A live capture and systematic interaction pass are still required.
