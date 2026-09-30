# F/A-18C Hornet export inventory

> **Baseline:** DCS-BIOS v0.11.7 reference JSON. This is a versioned working catalog, not a claim that DCS-BIOS is the permanent source of truth.
> No live DCS capture has been used to verify these entries.

This catalog groups each documented control under its DCS-BIOS equipment category. Controls with one or more input interfaces are marked **Interactable (documented input)**; those without inputs are **Output only (no documented input)**. This describes DCS-BIOS metadata, not proof that every input works in every DCS context.

Reference: [https://github.com/DCS-Skunkworks/dcs-bios/blob/v0.11.7/Scripts/DCS-BIOS/doc/json/FA-18C_hornet.json](https://github.com/DCS-Skunkworks/dcs-bios/blob/v0.11.7/Scripts/DCS-BIOS/doc/json/FA-18C_hornet.json)  
SHA-256 of the exact JSON input: `1edfb45c430e6b149dadd4c2469b6e4ac363f6834b3f43afed754f60090392ef`

## Coverage

- Equipment/category groups: **74**
- Documented controls: **505**
- Controls with input interfaces: **298**
- Controls without input interfaces: **207**
- Output records: **503** (440 integer, 63 string)

The address, mask, and shift values below are from the DCS-BIOS memory map. They must not be treated as native DCS export addresses.

## Inventory

### AMPCD

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| AMPCD_BRT_CTL | Brightness Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x74E0`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| AMPCD_CONT_SW | Contrast Control Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x746C`, mask `0x0C00`, shift `10`, max `2`, selector position |
| AMPCD_GAIN_SW | Gain Control Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x746C`, mask `0x3000`, shift `12`, max `2`, selector position |
| AMPCD_NIGHT_DAY | Night/Day Brightness Selector<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x7466`, mask `0x6000`, shift `13`, max `2`, selector position |
| AMPCD_PB_01 | Pushbutton 1<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7466`, mask `0x8000`, shift `15`, max `1`, selector position |
| AMPCD_PB_02 | Pushbutton 2<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x746C`, mask `0x4000`, shift `14`, max `1`, selector position |
| AMPCD_PB_03 | Pushbutton 3<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x746C`, mask `0x8000`, shift `15`, max `1`, selector position |
| AMPCD_PB_04 | Pushbutton 4<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747A`, mask `0x0100`, shift `8`, max `1`, selector position |
| AMPCD_PB_05 | Pushbutton 5<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747A`, mask `0x0200`, shift `9`, max `1`, selector position |
| AMPCD_PB_06 | Pushbutton 6<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747A`, mask `0x0400`, shift `10`, max `1`, selector position |
| AMPCD_PB_07 | Pushbutton 7<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747A`, mask `0x0800`, shift `11`, max `1`, selector position |
| AMPCD_PB_08 | Pushbutton 8<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747A`, mask `0x1000`, shift `12`, max `1`, selector position |
| AMPCD_PB_09 | Pushbutton 9<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747A`, mask `0x2000`, shift `13`, max `1`, selector position |
| AMPCD_PB_10 | Pushbutton 10<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747A`, mask `0x4000`, shift `14`, max `1`, selector position |
| AMPCD_PB_11 | Pushbutton 11<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747A`, mask `0x8000`, shift `15`, max `1`, selector position |
| AMPCD_PB_12 | Pushbutton 12<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747C`, mask `0x0100`, shift `8`, max `1`, selector position |
| AMPCD_PB_13 | Pushbutton 13<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747C`, mask `0x0200`, shift `9`, max `1`, selector position |
| AMPCD_PB_14 | Pushbutton 14<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747C`, mask `0x0400`, shift `10`, max `1`, selector position |
| AMPCD_PB_15 | Pushbutton 15<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747C`, mask `0x0800`, shift `11`, max `1`, selector position |
| AMPCD_PB_16 | Pushbutton 16<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747C`, mask `0x1000`, shift `12`, max `1`, selector position |
| AMPCD_PB_17 | Pushbutton 17<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747C`, mask `0x2000`, shift `13`, max `1`, selector position |
| AMPCD_PB_18 | Pushbutton 18<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747C`, mask `0x4000`, shift `14`, max `1`, selector position |
| AMPCD_PB_19 | Pushbutton 19<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747C`, mask `0x8000`, shift `15`, max `1`, selector position |
| AMPCD_PB_20 | Pushbutton 20<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747E`, mask `0x0100`, shift `8`, max `1`, selector position |
| AMPCD_SYM_SW | Symbology Control Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x746C`, mask `0x0300`, shift `8`, max `2`, selector position |

### APU Fire Warning Extinguisher Light

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| APU_FIRE_BTN | APU Fire Warning/Extinguisher Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740C`, mask `0x0008`, shift `3`, max `1`, selector position |
| FIRE_APU_LT | FIRE APU Light (red)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740C`, mask `0x0004`, shift `2`, max `1`, 0 if light is off, 1 if light is on |

### Angle of Attack Indexer Lights

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| AOA_INDEXER_HIGH | AOA Indexer High (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x0008`, shift `3`, max `1`, 0 if light is off, 1 if light is on |
| AOA_INDEXER_HIGH_F | AOA Indexer High as Float (green)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x758C`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| AOA_INDEXER_LOW | AOA Indexer Low (red)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x0020`, shift `5`, max `1`, 0 if light is off, 1 if light is on |
| AOA_INDEXER_LOW_F | AOA Indexer Low as Float (red)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7590`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| AOA_INDEXER_NORMAL | AOA Indexer Normal (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x0010`, shift `4`, max `1`, 0 if light is off, 1 if light is on |
| AOA_INDEXER_NORMAL_F | AOA Indexer Normal as Float (yellow)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x758E`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |

### Antenna Select Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| COMM1_ANT_SELECT_SW | COMM 1 Antenna Selector Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74C0`, mask `0x3000`, shift `12`, max `2`, selector position |
| IFF_ANT_SELECT_SW | IFF Antenna Selector Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74C0`, mask `0xC000`, shift `14`, max `2`, selector position |

### Arresting Hook Handle and Light

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| ARRESTING_HOOK_LT | Hook Light<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A0`, mask `0x0400`, shift `10`, max `1`, 0 if light is off, 1 if light is on |
| HOOK_LEVER | Hook Lever<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74A0`, mask `0x0200`, shift `9`, max `1`, selector position |

### Auxiliary Power Unit Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| APU_CONTROL_SW | APU Control Switch, ON/OFF<br>`action` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set the switch position -- 0 = off, 1 = on; max_value=1)<br>`action` (toggle switch state; argument=TOGGLE) | `integer` at `0x74C2`, mask `0x0100`, shift `8`, max `1`, selector position |
| APU_READY_LT | APU Ready Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74C2`, mask `0x0800`, shift `11`, max `1`, 0 if light is off, 1 if light is on |
| ENGINE_CRANK_SW | Engine Crank Switch<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74C2`, mask `0x0600`, shift `9`, max `2`, selector position |

### Canopy Internal Jettison Handle

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| CANOPY_JETT_HANDLE_PULL | Canopy Jettison Handle Unlock Button - Press to jettison<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740C`, mask `0x0100`, shift `8`, max `1`, selector position |
| CANOPY_JETT_HANDLE_UNLOCK | Canopy Jettison Handle Unlock Button - Press to unlock<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740C`, mask `0x0080`, shift `7`, max `1`, selector position |

### Caution Light Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| CLIP_APU_ACC_LT | APU ACC (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A4`, mask `0x0100`, shift `8`, max `1`, 0 if light is off, 1 if light is on |
| CLIP_BATT_SW_LT | BATT SW (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A4`, mask `0x0200`, shift `9`, max `1`, 0 if light is off, 1 if light is on |
| CLIP_CK_SEAT_LT | CK SEAT (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A0`, mask `0x8000`, shift `15`, max `1`, 0 if light is off, 1 if light is on |
| CLIP_FCES_LT | FCES (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A4`, mask `0x4000`, shift `14`, max `1`, 0 if light is off, 1 if light is on |
| CLIP_FCS_HOT_LT | FCS HOT (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A4`, mask `0x0400`, shift `10`, max `1`, 0 if light is off, 1 if light is on |
| CLIP_FUEL_LO_LT | FUEL LO (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A4`, mask `0x2000`, shift `13`, max `1`, 0 if light is off, 1 if light is on |
| CLIP_GEN_TIE_LT | GEN TIE (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A4`, mask `0x0800`, shift `11`, max `1`, 0 if light is off, 1 if light is on |
| CLIP_L_GEN_LT | L GEN (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A8`, mask `0x0100`, shift `8`, max `1`, 0 if light is off, 1 if light is on |
| CLIP_R_GEN_LT | R GEN (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A8`, mask `0x0200`, shift `9`, max `1`, 0 if light is off, 1 if light is on |
| CLIP_SPARE_CTN1_LT | SPARE CTN1 (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A4`, mask `0x1000`, shift `12`, max `1`, 0 if light is off, 1 if light is on |
| CLIP_SPARE_CTN2_LT | SPARE CTN2 (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A4`, mask `0x8000`, shift `15`, max `1`, 0 if light is off, 1 if light is on |
| CLIP_SPARE_CTN3_LT | SPARE CTN3 (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A8`, mask `0x0400`, shift `10`, max `1`, 0 if light is off, 1 if light is on |

### Clock

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| CLOCK_ELAPSED_MINUTES | Elapsed Minutes<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7510`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| CLOCK_ELAPSED_SECONDS | Elapsed Seconds<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7512`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| CLOCK_HOURS | Hours<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x750C`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| CLOCK_MINUTES | Minutes<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x750E`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |

### Cockpit Altimeter

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| PRESSURE_ALT | Pressure Altitude<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7514`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |

### Comms frequency

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| COMM1_CHANNEL_NUMERIC | Comm 1 Channel as number<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x7404`, mask `0x001F`, shift `0`, max `24`, Comm 1 Channel as number |
| COMM1_FREQ | COMM1 FREQ<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x7400`, mask `0xFFFF`, shift `0`, max `65535`, COMM1 FREQ |
| COMM2_CHANNEL_NUMERIC | Comm 2 Channel as number<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x7406`, mask `0x001F`, shift `0`, max `24`, Comm 2 Channel as number |
| COMM2_FREQ | COMM2 FREQ<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x7402`, mask `0xFFFF`, shift `0`, max `65535`, COMM2 FREQ |

### Communication Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| COM_AUX | AUX Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7538`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| COM_COMM_G_XMT_SW | COMM G XMT Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74BC`, mask `0x1800`, shift `11`, max `2`, selector position |
| COM_COMM_RELAY_SW | Comm Relay Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74BC`, mask `0x0600`, shift `9`, max `2`, selector position |
| COM_CRYPTO_SW | CRYPTO Switch<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74BE`, mask `0x0300`, shift `8`, max `2`, selector position |
| COM_ICS | ICS Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x752C`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| COM_IFF_MASTER_SW | IFF Master Switch, EMER/NORM<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74BC`, mask `0x2000`, shift `13`, max `1`, selector position |
| COM_IFF_MODE4_SW | IFF Mode 4 Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74BC`, mask `0xC000`, shift `14`, max `2`, selector position |
| COM_ILS_CHANNEL_SW | ILS Channel Selector Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=19) | `integer` at `0x74BE`, mask `0xF800`, shift `11`, max `19`, selector position |
| COM_ILS_UFC_MAN_SW | ILS UFC/MAN Switch, UFC/MAN<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74BE`, mask `0x0400`, shift `10`, max `1`, selector position |
| COM_MIDS_A | MIDS A Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7532`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| COM_MIDS_B | MIDS B Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7534`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| COM_RWR | RWR Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x752E`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| COM_TACAN | TACAN Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7536`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| COM_VOX | VOX Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x752A`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| COM_WPN | WPN Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7530`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |

### Defog Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| DEFOG_HANDLE | Defog Handle<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7550`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| WSHIELD_ANTI_ICE_SW | Windshield Anti-Ice/Rain Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74CC`, mask `0x6000`, shift `13`, max `2`, selector position |

### Dispenser/EMC Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| AUX_REL_SW | Auxiliary Release Switch, ENABLE/NORM<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7488`, mask `0x0800`, shift `11`, max `1`, selector position |
| CMSD_DISPENSE_SW | DISPENSER Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x7484`, mask `0x6000`, shift `13`, max `2`, selector position |
| CMSD_JET_SEL_BTN | ECM JETT JETT SEL Button - Push to jettison<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7484`, mask `0x8000`, shift `15`, max `1`, selector position |
| CMSD_JET_SEL_L | ECM JETT JETT SEL Button Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74D4`, mask `0x8000`, shift `15`, max `1`, 0 if light is off, 1 if light is on |
| ECM_MODE_SW | ECM Mode Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=4) | `integer` at `0x7488`, mask `0x0700`, shift `8`, max `4`, selector position |

### ECM Dispenser Button

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| CMSD_DISPENSE_BTN | Dispense Button - Push to dispense flares and chaff<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C2`, mask `0x4000`, shift `14`, max `1`, selector position |

### Ejection Seat

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| EJECTION_HANDLE_SW | Ejection Control Handle<br>`toggle_switch` | **Interactable**<br>`set_state` (set the switch position -- 0 = off, 1 = on; max_value=1) | `integer` at `0x74CE`, mask `0x4000`, shift `14`, max `1`, switch position -- 0 = off, 1 = on |
| EJECTION_SEAT_ARMED | Ejection Seat SAFE/ARMED Handle, SAFE/ARMED<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74CE`, mask `0x8000`, shift `15`, max `1`, selector position |
| EJECTION_SEAT_MNL_OVRD | Ejection Seat Manual Override Handle, PULL/PUSH<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D0`, mask `0x0100`, shift `8`, max `1`, selector position |
| HIDE_STICK_TOGGLE | Hide Stick toggle<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D0`, mask `0x1000`, shift `12`, max `1`, selector position |
| SEAT_HEIGHT_SW | Seat Height Adjustment Switch<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74D0`, mask `0x0C00`, shift `10`, max `2`, selector position |
| SHLDR_HARNESS_SW | Shoulder Harness Control Handle, LOCK/UNLOCK<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D0`, mask `0x0200`, shift `9`, max `1`, selector position |

### Electrical Power Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| BATTERY_SW | Battery Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74C4`, mask `0x1800`, shift `11`, max `2`, selector position |
| L_GEN_SW | Left Generator Control Switch, NORM/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C4`, mask `0x2000`, shift `13`, max `1`, selector position |
| R_GEN_SW | Right Generator Control Switch, NORM/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C4`, mask `0x4000`, shift `14`, max `1`, selector position |
| VOLT_E | Battery E Volts<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x753E`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| VOLT_U | Battery U Volts<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x753C`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |

### Emergency Jettison Button

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| EMER_JETT_BTN | Emergency Jettison Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742C`, mask `0x0100`, shift `8`, max `1`, selector position |

### Emergency and Parking Brake Handle

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| EMERGENCY_PARKING_BRAKE_PULL | Emergency/Parking Brake Pull<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7484`, mask `0x0400`, shift `10`, max `1`, selector position |
| EMERGENCY_PARKING_BRAKE_ROTATE | Emergency/Parking Brake Rotate<br>`emergency_parking_brake` | **Interactable**<br>`set_state` (set the switch position -- 0 = emergency, 1 = park, 2 = release; max_value=2) | `integer` at `0x7484`, mask `0x1800`, shift `11`, max `2`, switch position -- 0 = emergency, 1 = parking, 2 = release |

### Environment Control Louver

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| LEFT_LOUVER | Left Louver<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7502`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| RIGHT_LOUVER | Right Louver<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7504`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |

### Environment Control System Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| BLEED_AIR_KNOB | Bleed Air Knob<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=3) | `integer` at `0x74C6`, mask `0x0300`, shift `8`, max `3`, selector position |
| BLEED_AIR_PULL | Bleed Air Knob, AUG PULL<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C4`, mask `0x8000`, shift `15`, max `1`, selector position |
| CABIN_PRESS_SW | Cabin Pressure Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74C6`, mask `0x3000`, shift `12`, max `2`, selector position |
| CABIN_TEMP | Cabin Temperature Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7540`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| ECS_MODE_SW | ECS Mode Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74C6`, mask `0x0C00`, shift `10`, max `2`, selector position |
| ENG_ANTIICE_SW | Engine Anti-Ice Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74C6`, mask `0xC000`, shift `14`, max `2`, selector position |
| PITOT_HEAT_SW | Pitot Heater Switch, ON/AUTO<br>`action` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set the switch position -- 0 = off, 1 = on; max_value=1)<br>`action` (toggle switch state; argument=TOGGLE) | `integer` at `0x74C8`, mask `0x0100`, shift `8`, max `1`, selector position |
| SUIT_TEMP | Suit Temperature Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7542`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |

### Exterior Lights Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| FORMATION_DIMMER | Formation Lights Dimmer<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7526`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| INT_WNG_TANK_SW | Internal Wing Tank Fuel Control Switch, INHIBIT/NORM<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74A8`, mask `0x8000`, shift `15`, max `1`, selector position |
| POSITION_DIMMER | Position Lights Dimmer<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7524`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| STROBE_SW | Strobe Lights Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74B0`, mask `0x3000`, shift `12`, max `2`, selector position |

### External Aircraft Model

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| EXT_FORMATION_LIGHTS | Formation Lights (light green)<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x7576`, mask `0xFFFF`, shift `0`, max `65535`, Formation Lights (light green) |
| EXT_HOOK | Hook Position<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x7586`, mask `0xFFFF`, shift `0`, max `65535`, Hook Position |
| EXT_LAUNCH_BAR | Launch Bar position<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x75AE`, mask `0xFFFF`, shift `0`, max `65535`, Launch Bar position |
| EXT_NOZZLE_POS_L | Left Nozzle Position<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x757A`, mask `0xFFFF`, shift `0`, max `65535`, Left Nozzle Position |
| EXT_NOZZLE_POS_R | Right Nozzle Position<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x7578`, mask `0xFFFF`, shift `0`, max `65535`, Right Nozzle Position |
| EXT_POSITION_LIGHT_LEFT | Left Position Light (red)<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x74D6`, mask `0x0400`, shift `10`, max `1`, Left Position Light (red) |
| EXT_POSITION_LIGHT_RIGHT | Right Position Light (green)<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x74D6`, mask `0x0800`, shift `11`, max `1`, Right Position Light (green) |
| EXT_REFUEL_PROBE | Refuel Probe<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x7574`, mask `0xFFFF`, shift `0`, max `65535`, Refuel Probe |
| EXT_REFUEL_PROBE_LIGHT | Refuel Probe Light (white)<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x74D6`, mask `0x0200`, shift `9`, max `1`, Refuel Probe Light (white) |
| EXT_SPEED_BRAKE | Speed Brake<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x756E`, mask `0xFFFF`, shift `0`, max `65535`, Speed Brake |
| EXT_STAIR | Stair<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x7572`, mask `0xFFFF`, shift `0`, max `65535`, Stair |
| EXT_STROBE_LIGHTS | Strobe Lights (red)<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x74D6`, mask `0x2000`, shift `13`, max `1`, Strobe Lights (red) |
| EXT_TAIL_LIGHT | Tail Light (white)<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x74D6`, mask `0x1000`, shift `12`, max `1`, Tail Light (white) |
| EXT_WING_FOLDING | Wing Folding<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x7570`, mask `0xFFFF`, shift `0`, max `65535`, Wing Folding |
| EXT_WOW_LEFT | Weight ON Wheels Left Gear<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x74D8`, mask `0x0100`, shift `8`, max `1`, Weight ON Wheels Left Gear |
| EXT_WOW_NOSE | Weight ON Wheels Nose Gear<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x74D6`, mask `0x4000`, shift `14`, max `1`, Weight ON Wheels Nose Gear |
| EXT_WOW_RIGHT | Weight ON Wheels Right Gear<br>`metadata` | **Output only**<br>No documented input | `integer` at `0x74D6`, mask `0x8000`, shift `15`, max `1`, Weight ON Wheels Right Gear |

### Fire Systems

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| FIRE_EXT_BTN | Fire Extinguisher Pushbutton<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x0001`, shift `0`, max `1`, selector position |

### Fire Test Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| FIRE_TEST_SW | Fire and Bleed Air Test Switch, (RMB) TEST A/(LMB) TEST B<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74AC`, mask `0x0300`, shift `8`, max `2`, selector position |

### Flaps, Landing Gear, Stores Indicator Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| FLP_LG_FLAPS_LT | FLAPS (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7466`, mask `0x0001`, shift `0`, max `1`, 0 if light is off, 1 if light is on |
| FLP_LG_FULL_FLAPS_LT | FULL FLAPS (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7430`, mask `0x8000`, shift `15`, max `1`, 0 if light is off, 1 if light is on |
| FLP_LG_HALF_FLAPS_LT | HALF FLAPS (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7430`, mask `0x4000`, shift `14`, max `1`, 0 if light is off, 1 if light is on |
| FLP_LG_LEFT_GEAR_LT | LEFT GEAR (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7430`, mask `0x1000`, shift `12`, max `1`, 0 if light is off, 1 if light is on |
| FLP_LG_NOSE_GEAR_LT | NOSE GEAR (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7430`, mask `0x0800`, shift `11`, max `1`, 0 if light is off, 1 if light is on |
| FLP_LG_RIGHT_GEAR_LT | RIGHT GEAR (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7430`, mask `0x2000`, shift `13`, max `1`, 0 if light is off, 1 if light is on |

### Flight Computer Cool Switch

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| AV_COOL_SW | AV COOL Switch, NORM/EMERG<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74A0`, mask `0x4000`, shift `14`, max `1`, selector position |

### Flight Control System Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| FCS_RESET_BTN | FCS RESET Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74B4`, mask `0x4000`, shift `14`, max `1`, selector position |
| GAIN_SWITCH | GAIN Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74BC`, mask `0x0100`, shift `8`, max `1`, selector position |
| GAIN_SWITCH_COVER | GAIN Switch Cover<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74B4`, mask `0x8000`, shift `15`, max `1`, selector position |
| RUD_TRIM | RUD TRIM Control<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7528`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| TO_TRIM_BTN | T/O TRIM Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74B4`, mask `0x2000`, shift `13`, max `1`, selector position |

### Fuel Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| EXT_CNT_TANK_SW | External Centerline Tank Fuel Control Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74B4`, mask `0x0600`, shift `9`, max `2`, selector position |
| EXT_WNG_TANK_SW | External Wing Tanks Fuel Control Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74B4`, mask `0x1800`, shift `11`, max `2`, selector position |
| FUEL_DUMP_SW | Fuel Dump Switch, ON/OFF<br>`action` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set the switch position -- 0 = off, 1 = on; max_value=1)<br>`action` (toggle switch state; argument=TOGGLE) | `integer` at `0x74B4`, mask `0x0100`, shift `8`, max `1`, selector position |
| PROBE_SW | Probe Control Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74B0`, mask `0xC000`, shift `14`, max `2`, selector position |

### Generator Tie Control Switch

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| GEN_TIE_COVER | Generator TIE Control Switch Cover, OPEN/CLOSE<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C2`, mask `0x1000`, shift `12`, max `1`, selector position |
| GEN_TIE_SW | Generator TIE Control Switch, NORM/RESET<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C2`, mask `0x2000`, shift `13`, max `1`, selector position |

### Ground Power Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| EXT_PWR_SW | External Power Switch<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74AC`, mask `0x0C00`, shift `10`, max `2`, selector position |
| GND_PWR_1_SW | Ground Power Switch 1<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74AC`, mask `0x3000`, shift `12`, max `2`, selector position |
| GND_PWR_2_SW | Ground Power Switch 2<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74AC`, mask `0xC000`, shift `14`, max `2`, selector position |
| GND_PWR_3_SW | Ground Power Switch 3<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74B0`, mask `0x0300`, shift `8`, max `2`, selector position |
| GND_PWR_4_SW | Ground Power Switch 4<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74B0`, mask `0x0C00`, shift `10`, max `2`, selector position |

### HUD

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| HUD_ATC_NWS_ENGAGED | ATC - NWS Engaged<br>`display` | **Output only**<br>No documented input | `string` at `0x75A8`, length `6`, ATC - NWS Engaged |
| HUD_LTDR | Laser Status<br>`display` | **Output only**<br>No documented input | `string` at `0x75A2`, length `5`, Laser Status |

### HUD Control Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| HUD_ALT_SW | Altitude Switch, BARO/RDR<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742C`, mask `0x4000`, shift `14`, max `1`, selector position |
| HUD_AOA_INDEXER | AOA Indexer Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x745E`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| HUD_ATT_SW | Attitude Selector Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x742E`, mask `0x0300`, shift `8`, max `2`, selector position |
| HUD_BALANCE | Balance Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x745C`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| HUD_BLACK_LVL | Black Level Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x745A`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| HUD_SYM_BRT | HUD Symbology Brightness Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7458`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| HUD_SYM_BRT_SELECT | HUD Symbology Brightness Selector Knob, DAY/NIGHT<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742C`, mask `0x0800`, shift `11`, max `1`, selector position |
| HUD_SYM_REJ_SW | HUD Symbology Reject Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x742C`, mask `0x0600`, shift `9`, max `2`, selector position |
| HUD_VIDEO_CONTROL_SW | HUD Video Control Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x742C`, mask `0x3000`, shift `12`, max `2`, selector position |

### HUD Video Bit Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| HUD_VIDEO_BIT | HUD Video BIT Initiate Pushbutton - Push to initiate BIT<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740A`, mask `0x0040`, shift `6`, max `1`, selector position |

### HUD Video Record Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| IFEI | Brightness Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x74DE`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| MODE_SELECTOR_SW | Mode Selector Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x7466`, mask `0x1800`, shift `11`, max `2`, selector position |
| SELECT_HMD_LDDI_RDDI | Selector Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x7466`, mask `0x0180`, shift `7`, max `2`, selector position |
| SELECT_HUD_LDDI_RDDI | Selector Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x7466`, mask `0x0600`, shift `9`, max `2`, selector position |

### HYD 1 and HYD Pressure Indicator

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| HYD_IND_LEFT | HYD Indicator Left<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x751E`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| HYD_IND_RIGHT | HYD Indicator Right<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7520`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |

### Heading and Course Set Switches

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| LEFT_DDI_CRS_SW | Course Set Switch<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74A8`, mask `0x6000`, shift `13`, max `2`, selector position |
| LEFT_DDI_HDG_SW | Heading Set Switch<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74A8`, mask `0x1800`, shift `11`, max `2`, selector position |

### Integrated Fuel/Engine Indicator (IFEI)

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| IFEI_BINGO | BINGO<br>`display` | **Output only**<br>No documented input | `string` at `0x7468`, length `5`, BINGO |
| IFEI_BINGO_TEXTURE | BINGO Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74C6`, length `1`, BINGO Texture Visible: 1 = yes, 0 = no |
| IFEI_CLOCK_H | CLOCK_H<br>`display` | **Output only**<br>No documented input | `string` at `0x746E`, length `2`, CLOCK_H |
| IFEI_CLOCK_M | CLOCK_M<br>`display` | **Output only**<br>No documented input | `string` at `0x7470`, length `2`, CLOCK_M |
| IFEI_CLOCK_S | CLOCK_S<br>`display` | **Output only**<br>No documented input | `string` at `0x7472`, length `2`, CLOCK_S |
| IFEI_CODES | Codes<br>`display` | **Output only**<br>No documented input | `string` at `0x74AE`, length `3`, Codes |
| IFEI_DD_1 | DD_1<br>`display` | **Output only**<br>No documented input | `string` at `0x747A`, length `1`, DD_1 |
| IFEI_DD_2 | DD_2<br>`display` | **Output only**<br>No documented input | `string` at `0x747C`, length `1`, DD_2 |
| IFEI_DD_3 | DD_3<br>`display` | **Output only**<br>No documented input | `string` at `0x747E`, length `1`, DD_3 |
| IFEI_DD_4 | DD_4<br>`display` | **Output only**<br>No documented input | `string` at `0x7480`, length `1`, DD_4 |
| IFEI_DWN_BTN | Down Arrow Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7466`, mask `0x0010`, shift `4`, max `1`, selector position |
| IFEI_ET_BTN | ET Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7466`, mask `0x0040`, shift `6`, max `1`, selector position |
| IFEI_FF_L | FF_L<br>`display` | **Output only**<br>No documented input | `string` at `0x7482`, length `3`, FF_L |
| IFEI_FF_R | FF_R<br>`display` | **Output only**<br>No documented input | `string` at `0x7486`, length `3`, FF_R |
| IFEI_FF_TEXTURE | FF Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74C0`, length `1`, FF Texture Visible: 1 = yes, 0 = no |
| IFEI_FUEL_DOWN | FUEL_DOWN<br>`display` | **Output only**<br>No documented input | `string` at `0x748A`, length `6`, FUEL_DOWN |
| IFEI_FUEL_UP | FUEL_UP<br>`display` | **Output only**<br>No documented input | `string` at `0x7490`, length `6`, FUEL_UP |
| IFEI_L0_TEXTURE | Left 0 Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74CC`, length `1`, Left 0 Texture Visible: 1 = yes, 0 = no |
| IFEI_L100_TEXTURE | Left 100 Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74D4`, length `1`, Left 100 Texture Visible: 1 = yes, 0 = no |
| IFEI_L50_TEXTURE | Left 50 Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74D0`, length `1`, Left 50 Texture Visible: 1 = yes, 0 = no |
| IFEI_LPOINTER_TEXTURE | Left Pointer Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74D8`, length `1`, Left Pointer Texture Visible: 1 = yes, 0 = no |
| IFEI_LSCALE_TEXTURE | Left Scale Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74C8`, length `1`, Left Scale Texture Visible: 1 = yes, 0 = no |
| IFEI_L_TEXTURE | Left Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x7582`, length `1`, Left Texture Visible: 1 = yes, 0 = no |
| IFEI_MODE_BTN | Mode Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7466`, mask `0x0002`, shift `1`, max `1`, selector position |
| IFEI_NOZ_TEXTURE | NOZZLE Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74C2`, length `1`, NOZZLE Texture Visible: 1 = yes, 0 = no |
| IFEI_OIL_PRESS_L | OilPress_L<br>`display` | **Output only**<br>No documented input | `string` at `0x7496`, length `3`, OilPress_L |
| IFEI_OIL_PRESS_R | OilPress_R<br>`display` | **Output only**<br>No documented input | `string` at `0x749A`, length `3`, OilPress_R |
| IFEI_OIL_TEXTURE | OIL Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74C4`, length `1`, OIL Texture Visible: 1 = yes, 0 = no |
| IFEI_QTY_BTN | QTY Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7466`, mask `0x0004`, shift `2`, max `1`, selector position |
| IFEI_R0_TEXTURE | Right 0 Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74CE`, length `1`, Right 0 Texture Visible: 1 = yes, 0 = no |
| IFEI_R100_TEXTURE | Right 100 Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74D6`, length `1`, Right 100 Texture Visible: 1 = yes, 0 = no |
| IFEI_R50_TEXTURE | Right 50 Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74D2`, length `1`, Right 50 Texture Visible: 1 = yes, 0 = no |
| IFEI_RPM_L | RPM_L<br>`display` | **Output only**<br>No documented input | `string` at `0x749E`, length `3`, RPM_L |
| IFEI_RPM_R | RPM_R<br>`display` | **Output only**<br>No documented input | `string` at `0x74A2`, length `3`, RPM_R |
| IFEI_RPM_TEXTURE | RPM Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74BC`, length `1`, RPM Texture Visible: 1 = yes, 0 = no |
| IFEI_RPOINTER_TEXTURE | Right Pointer Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74DA`, length `1`, Right Pointer Texture Visible: 1 = yes, 0 = no |
| IFEI_RSCALE_TEXTURE | Right Scale Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74CA`, length `1`, Right Scale Texture Visible: 1 = yes, 0 = no |
| IFEI_R_TEXTURE | Right Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x7584`, length `1`, Right Texture Visible: 1 = yes, 0 = no |
| IFEI_SP | SP<br>`display` | **Output only**<br>No documented input | `string` at `0x74B2`, length `3`, SP |
| IFEI_T | T<br>`display` | **Output only**<br>No documented input | `string` at `0x757C`, length `6`, T |
| IFEI_TEMP_L | TEMP_L<br>`display` | **Output only**<br>No documented input | `string` at `0x74A6`, length `3`, TEMP_L |
| IFEI_TEMP_R | TEMP_R<br>`display` | **Output only**<br>No documented input | `string` at `0x74AA`, length `3`, TEMP_R |
| IFEI_TEMP_TEXTURE | Temp Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74BE`, length `1`, Temp Texture Visible: 1 = yes, 0 = no |
| IFEI_TIMER_H | TIMER_H<br>`display` | **Output only**<br>No documented input | `string` at `0x7474`, length `2`, TIMER_H |
| IFEI_TIMER_M | TIMER_M<br>`display` | **Output only**<br>No documented input | `string` at `0x7476`, length `2`, TIMER_M |
| IFEI_TIMER_S | TIMER_S<br>`display` | **Output only**<br>No documented input | `string` at `0x7478`, length `2`, TIMER_S |
| IFEI_TIME_SET_MODE | Time Set Mode<br>`display` | **Output only**<br>No documented input | `string` at `0x74B6`, length `6`, Time Set Mode |
| IFEI_UP_BTN | Up Arrow Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7466`, mask `0x0008`, shift `3`, max `1`, selector position |
| IFEI_ZONE_BTN | ZONE Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7466`, mask `0x0020`, shift `5`, max `1`, selector position |
| IFEI_Z_TEXTURE | Zulu Texture Visible: 1 = yes, 0 = no<br>`display` | **Output only**<br>No documented input | `string` at `0x74DC`, length `1`, Zulu Texture Visible: 1 = yes, 0 = no |

### Interior Lights Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| CHART_DIMMER | CHART Light Dimmer<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x754A`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| COCKKPIT_LIGHT_MODE_SW | MODE Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74C8`, mask `0x0600`, shift `9`, max `2`, selector position |
| CONSOLES_DIMMER | CONSOLES Lights Dimmer<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7544`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| FLOOD_DIMMER | FLOOD Light Dimmer<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7548`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| INST_PNL_DIMMER | INST PNL Dimmer<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7546`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| LIGHTS_TEST_SW | Lights Test Switch, TEST/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C8`, mask `0x0800`, shift `11`, max `1`, selector position |
| WARN_CAUTION_DIMMER | WARN/CAUTION Light Dimmer<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x754C`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |

### Internal Canopy Switch

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| CANOPY_POS | Canopy Position<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7552`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| CANOPY_SW | Canopy Control Switch<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74CE`, mask `0x0300`, shift `8`, max `2`, selector position |

### Internal Lights

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| CHART_INT_LT | Chart Lightning (light green)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x755E`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| CONSOLE_INT_LT | Console Lightning (light green)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7558`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| EMERG_INSTR_INT_LT | Emergency Instrument Lightning (light green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74D4`, mask `0x2000`, shift `13`, max `1`, 0 if light is off, 1 if light is on |
| ENG_INSTR_INT_LT | Eng Instrument Flood Lightning (light green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74D4`, mask `0x4000`, shift `14`, max `1`, 0 if light is off, 1 if light is on |
| FLOOD_INT_LT | Flood Lightning (light green)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x755A`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| IFEI_BTN_INT_LT | IFEI Buttons Lightning (light green)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7566`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| IFEI_DISP_INT_LT | IFEI Display Lightning (light green)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7564`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| INSTR_INT_LT | Instrument Lightning (light green)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7560`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| NVG_FLOOD_INT_LT | Nvg Flood Lightning (light green)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x755C`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| STBY_COMPASS_INT_LT | Stby Compass Lightning (light green)<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7562`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |

### KY-58 Control

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| KY58_FILL_SELECT | KY-58 Fill Select Knob<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=7) | `integer` at `0x74CC`, mask `0x0700`, shift `8`, max `7`, selector position |
| KY58_FILL_SEL_PULL | KY-58 Fill Select Knob, Pull<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D8`, mask `0x0200`, shift `9`, max `1`, selector position |
| KY58_MODE_SELECT | KY-58 Mode Select Knob<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=3) | `integer` at `0x74CA`, mask `0xC000`, shift `14`, max `3`, selector position |
| KY58_POWER_SELECT | KY-58 Power Select Knob<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74CC`, mask `0x1800`, shift `11`, max `2`, selector position |
| KY58_VOLUME | KY-58 Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x754E`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |

### LH Advisory Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| LH_ADV_ASPJ_OH | ASPJ OH (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0008`, shift `3`, max `1`, 0 if light is off, 1 if light is on |
| LH_ADV_GO | GO (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0010`, shift `4`, max `1`, 0 if light is off, 1 if light is on |
| LH_ADV_L_BAR_GREEN | L BAR (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0002`, shift `1`, max `1`, 0 if light is off, 1 if light is on |
| LH_ADV_L_BAR_RED | L BAR (red)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x8000`, shift `15`, max `1`, 0 if light is off, 1 if light is on |
| LH_ADV_L_BLEED | L BLEED (red)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x0800`, shift `11`, max `1`, 0 if light is off, 1 if light is on |
| LH_ADV_NO_GO | NO GO (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0020`, shift `5`, max `1`, 0 if light is off, 1 if light is on |
| LH_ADV_REC | REC (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0001`, shift `0`, max `1`, 0 if light is off, 1 if light is on |
| LH_ADV_R_BLEED | R BLEED (red)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x1000`, shift `12`, max `1`, 0 if light is off, 1 if light is on |
| LH_ADV_SPD_BRK | SPD BRK (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x2000`, shift `13`, max `1`, 0 if light is off, 1 if light is on |
| LH_ADV_STBY | STBY (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x4000`, shift `14`, max `1`, 0 if light is off, 1 if light is on |
| LH_ADV_XMIT | XMIT (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0004`, shift `2`, max `1`, 0 if light is off, 1 if light is on |

### LOX Indicator

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| OBOGS_SW | OBOGS Control Switch, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C0`, mask `0x0100`, shift `8`, max `1`, selector position |
| OXY_FLOW | OXY Flow Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x753A`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |

### Landing Gear Handle and Warning Tone Silence

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| EMERGENCY_GEAR_ROTATE | Emergency Gear Rotate<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747E`, mask `0x2000`, shift `13`, max `1`, selector position |
| GEAR_DOWNLOCK_OVERRIDE_BTN | Landing Gear Override<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747E`, mask `0x4000`, shift `14`, max `1`, selector position |
| GEAR_LEVER | Gear Lever<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747E`, mask `0x1000`, shift `12`, max `1`, selector position |
| GEAR_SILENCE_BTN | Warning Tone Silence Button - Push to silence<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747E`, mask `0x8000`, shift `15`, max `1`, selector position |
| LANDING_GEAR_HANDLE_LT | Landing Gear Handle Light<br>`led` | **Output only**<br>No documented input | `integer` at `0x747E`, mask `0x0800`, shift `11`, max `1`, 0 if light is off, 1 if light is on |

### Left DDI

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| LEFT_DDI_BRT_CTL | Brightness Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7410`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| LEFT_DDI_BRT_SELECT | Brightness Selector Knob<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x740E`, mask `0x0006`, shift `1`, max `2`, selector position |
| LEFT_DDI_CONT_CTL | Contrast Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7412`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| LEFT_DDI_PB_01 | Pushbutton 1<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x0008`, shift `3`, max `1`, selector position |
| LEFT_DDI_PB_02 | Pushbutton 2<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x0010`, shift `4`, max `1`, selector position |
| LEFT_DDI_PB_03 | Pushbutton 3<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x0020`, shift `5`, max `1`, selector position |
| LEFT_DDI_PB_04 | Pushbutton 4<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x0040`, shift `6`, max `1`, selector position |
| LEFT_DDI_PB_05 | Pushbutton 5<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x0080`, shift `7`, max `1`, selector position |
| LEFT_DDI_PB_06 | Pushbutton 6<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x0100`, shift `8`, max `1`, selector position |
| LEFT_DDI_PB_07 | Pushbutton 7<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x0200`, shift `9`, max `1`, selector position |
| LEFT_DDI_PB_08 | Pushbutton 8<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x0400`, shift `10`, max `1`, selector position |
| LEFT_DDI_PB_09 | Pushbutton 9<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x0800`, shift `11`, max `1`, selector position |
| LEFT_DDI_PB_10 | Pushbutton 10<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x1000`, shift `12`, max `1`, selector position |
| LEFT_DDI_PB_11 | Pushbutton 11<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x2000`, shift `13`, max `1`, selector position |
| LEFT_DDI_PB_12 | Pushbutton 12<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x4000`, shift `14`, max `1`, selector position |
| LEFT_DDI_PB_13 | Pushbutton 13<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740E`, mask `0x8000`, shift `15`, max `1`, selector position |
| LEFT_DDI_PB_14 | Pushbutton 14<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0001`, shift `0`, max `1`, selector position |
| LEFT_DDI_PB_15 | Pushbutton 15<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0002`, shift `1`, max `1`, selector position |
| LEFT_DDI_PB_16 | Pushbutton 16<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0004`, shift `2`, max `1`, selector position |
| LEFT_DDI_PB_17 | Pushbutton 17<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0008`, shift `3`, max `1`, selector position |
| LEFT_DDI_PB_18 | Pushbutton 18<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0010`, shift `4`, max `1`, selector position |
| LEFT_DDI_PB_19 | Pushbutton 19<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0020`, shift `5`, max `1`, selector position |
| LEFT_DDI_PB_20 | Pushbutton 20<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0040`, shift `6`, max `1`, selector position |

### Left Engine Fire Warning Extinguisher Light

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| FIRE_LEFT_LT | FIRE LEFT (red)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x0040`, shift `6`, max `1`, 0 if light is off, 1 if light is on |
| LEFT_FIRE_BTN | Left Engine/AMAD Fire Warning/Extinguisher Light<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7408`, mask `0x0080`, shift `7`, max `1`, selector position |
| LEFT_FIRE_BTN_COVER | Left Engine/AMAD Fire Warning Cover<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7408`, mask `0x0100`, shift `8`, max `1`, selector position |

### Left Essential Circuit Breakers

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| CB_FCS_CHAN1 | CB FCS CHAN 1, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C2`, mask `0x8000`, shift `15`, max `1`, selector position |
| CB_FCS_CHAN2 | CB FCS CHAN 2, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C4`, mask `0x0100`, shift `8`, max `1`, selector position |
| CB_LAUNCH_BAR | CB LAUNCH BAR, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C4`, mask `0x0400`, shift `10`, max `1`, selector position |
| CB_SPD_BRK | CB SPD BRK, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C4`, mask `0x0200`, shift `9`, max `1`, selector position |

### Lock Shoot Lights

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| LS_LOCK | LOCK (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x0001`, shift `0`, max `1`, 0 if light is off, 1 if light is on |
| LS_SHOOT | SHOOT (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x0002`, shift `1`, max `1`, 0 if light is off, 1 if light is on |
| LS_SHOOT_STROBE | SHOOT STROBE (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x0004`, shift `2`, max `1`, 0 if light is off, 1 if light is on |

### Map Gain/Spin Recovery Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| HMD_OFF_BRT | HMD OFF/BRT Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7456`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| IR_COOL_SW | IR Cooling Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x742A`, mask `0xC000`, shift `14`, max `2`, selector position |
| SPIN_LT | Spin Light (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x742A`, mask `0x0800`, shift `11`, max `1`, 0 if light is off, 1 if light is on |
| SPIN_RECOVERY_COVER | Spin Recovery Switch Cover, OPEN/CLOSE<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742A`, mask `0x1000`, shift `12`, max `1`, selector position |
| SPIN_RECOVERY_SW | Spin Recovery Switch, RCVY/NORM<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742A`, mask `0x2000`, shift `13`, max `1`, selector position |

### Master Arm Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| MASTER_ARM_SW | Master Arm Switch, ARM/SAFE<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740C`, mask `0x2000`, shift `13`, max `1`, selector position |
| MASTER_MODE_AA | Master Mode Button, A/A<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740C`, mask `0x0800`, shift `11`, max `1`, selector position |
| MASTER_MODE_AA_LT | AA Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740C`, mask `0x0200`, shift `9`, max `1`, 0 if light is off, 1 if light is on |
| MASTER_MODE_AG | Master Mode Button, A/G<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740C`, mask `0x1000`, shift `12`, max `1`, selector position |
| MASTER_MODE_AG_LT | AG Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740C`, mask `0x0400`, shift `10`, max `1`, 0 if light is off, 1 if light is on |
| MC_DISCH | DISCH Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740C`, mask `0x4000`, shift `14`, max `1`, 0 if light is off, 1 if light is on |
| MC_READY | READY Light (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740C`, mask `0x8000`, shift `15`, max `1`, 0 if light is off, 1 if light is on |

### Master Caution Light

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| MASTER_CAUTION_LT | MASTER CAUTION (yellow)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7408`, mask `0x0200`, shift `9`, max `1`, 0 if light is off, 1 if light is on |
| MASTER_CAUTION_RESET_SW | MASTER CAUTION Reset Button - Press to reset<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7408`, mask `0x0400`, shift `10`, max `1`, selector position |

### Mission Computer and Hydraulic Isolate Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| HYD_ISOLATE_OVERRIDE_SW | Hydraulic Isolate Override Switch, NORM/ORIDE<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C0`, mask `0x0800`, shift `11`, max `1`, selector position |
| MC_SW | MC Switch<br>`mission_computer_switch` | **Interactable**<br>`set_state` (set the switch position -- 0 = 1OFF, 1 = NORM, 2 = 2OFF; max_value=2) | `integer` at `0x74C0`, mask `0x0600`, shift `9`, max `2`, switch position -- 0 = 1OFF, 1 = NORM, 2 = 2OFF |

### RH Advisory Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| RH_ADV_AAA | AAA (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0800`, shift `11`, max `1`, 0 if light is off, 1 if light is on |
| RH_ADV_AI | AI (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0400`, shift `10`, max `1`, 0 if light is off, 1 if light is on |
| RH_ADV_CW | CW (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x1000`, shift `12`, max `1`, 0 if light is off, 1 if light is on |
| RH_ADV_DISP | DISP (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0100`, shift `8`, max `1`, 0 if light is off, 1 if light is on |
| RH_ADV_RCDR_ON | RCDR ON (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0080`, shift `7`, max `1`, 0 if light is off, 1 if light is on |
| RH_ADV_SAM | SAM (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x0200`, shift `9`, max `1`, 0 if light is off, 1 if light is on |
| RH_ADV_SPARE_RH1 | SPARE RH1 (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x2000`, shift `13`, max `1`, 0 if light is off, 1 if light is on |
| RH_ADV_SPARE_RH2 | SPARE RH2 (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x4000`, shift `14`, max `1`, 0 if light is off, 1 if light is on |
| RH_ADV_SPARE_RH3 | SPARE RH3 (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740A`, mask `0x8000`, shift `15`, max `1`, 0 if light is off, 1 if light is on |
| RH_ADV_SPARE_RH4 | SPARE RH4 (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740C`, mask `0x0001`, shift `0`, max `1`, 0 if light is off, 1 if light is on |
| RH_ADV_SPARE_RH5 | SPARE RH5 (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740C`, mask `0x0002`, shift `1`, max `1`, 0 if light is off, 1 if light is on |

### RWR Control Indicator

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| RWR_AUDIO_CTRL | ALR-67 AUDIO Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7554`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| RWR_BIT_BTN | ALR-67 BIT Pushbutton<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7498`, mask `0x0100`, shift `8`, max `1`, selector position |
| RWR_BIT_LT | ALR-67 BIT Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x749C`, mask `0x1000`, shift `12`, max `1`, 0 if light is off, 1 if light is on |
| RWR_DISPLAY_BTN | ALR-67 DISPLAY Pushbutton<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7488`, mask `0x2000`, shift `13`, max `1`, selector position |
| RWR_DISPLAY_LT | ALR-67 DISPLAY Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7498`, mask `0x4000`, shift `14`, max `1`, 0 if light is off, 1 if light is on |
| RWR_DIS_TYPE_SW | ALR-67 DIS TYPE Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=4) | `integer` at `0x7498`, mask `0x0E00`, shift `9`, max `4`, selector position |
| RWR_DMR_CTRL | ALR-67 DMR Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7508`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| RWR_ENABLE_LT | ALR-67 ENABLE Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x749C`, mask `0x0200`, shift `9`, max `1`, 0 if light is off, 1 if light is on |
| RWR_FAIL_LT | ALR-67 FAIL Light (red)<br>`led` | **Output only**<br>No documented input | `integer` at `0x749C`, mask `0x0800`, shift `11`, max `1`, 0 if light is off, 1 if light is on |
| RWR_LIMIT_LT | ALR-67 LIMIT Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7498`, mask `0x2000`, shift `13`, max `1`, 0 if light is off, 1 if light is on |
| RWR_LOWER_LT | ALR-67 POWER Light ON (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7498`, mask `0x1000`, shift `12`, max `1`, 0 if light is off, 1 if light is on |
| RWR_LT_BRIGHT | RWR Lights Brightness<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7568`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| RWR_OFFSET_BTN | ALR-67 OFFSET Pushbutton<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7488`, mask `0x8000`, shift `15`, max `1`, selector position |
| RWR_OFFSET_LT | ALR-67 OFFSET Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x749C`, mask `0x0400`, shift `10`, max `1`, 0 if light is off, 1 if light is on |
| RWR_POWER_BTN | ALR-67 POWER Pushbutton<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7488`, mask `0x1000`, shift `12`, max `1`, selector position |
| RWR_RWR_INTESITY | RWR Intensity Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x750A`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| RWR_SPECIAL_BTN | ALR-67 SPECIAL Pushbutton<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7488`, mask `0x4000`, shift `14`, max `1`, selector position |
| RWR_SPECIAL_EN_LT | ALR-67 SPECIAL ENABLE Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7498`, mask `0x8000`, shift `15`, max `1`, 0 if light is off, 1 if light is on |
| RWR_SPECIAL_LT | ALR-67 SPECIAL Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x749C`, mask `0x0100`, shift `8`, max `1`, 0 if light is off, 1 if light is on |

### Radar Altimeter

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| LOW_ALT_WARN_LT | Low Alt Warning (red)<br>`led` | **Output only**<br>No documented input | `integer` at `0x749C`, mask `0x8000`, shift `15`, max `1`, 0 if light is off, 1 if light is on |
| RADALT_ALT_PTR | Altitude Pointer<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x751A`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| RADALT_GREEN_LAMP | Radar Altimeter Green Lamp (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x74A0`, mask `0x0100`, shift `8`, max `1`, 0 if light is off, 1 if light is on |
| RADALT_HEIGHT | Set low altitude pointer<br>`analog_dial` | **Interactable**<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7516`, mask `0xFFFF`, shift `0`, max `65535`, the rotation of the knob in the cockpit (not the value that is controlled by this knob!) |
| RADALT_MIN_HEIGHT_PTR | Min Height Pointer<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7518`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| RADALT_OFF_FLAG | OFF Flag<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x751C`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| RADALT_TEST_SW | Push to Test Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x749C`, mask `0x4000`, shift `14`, max `1`, selector position |

### Radio Frequencies

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| COMM1 | COMM1 Radio<br>`radio` | **Interactable**<br>`set_string` (The frequency to set, with or without a decimal place) | `string` at `0x7592`, length `7`, The current frequency the radio is set to |
| COMM2 | COMM2 Radio<br>`radio` | **Interactable**<br>`set_string` (The frequency to set, with or without a decimal place) | `string` at `0x759A`, length `7`, The current frequency the radio is set to |

### Right DDI

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| RIGHT_DDI_BRT_CTL | Brightness Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7452`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| RIGHT_DDI_BRT_SELECT | Brightness Selector Knob<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x7418`, mask `0x0060`, shift `5`, max `2`, selector position |
| RIGHT_DDI_CONT_CTL | Contrast Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7454`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| RIGHT_DDI_PB_01 | Pushbutton 1<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x0080`, shift `7`, max `1`, selector position |
| RIGHT_DDI_PB_02 | Pushbutton 2<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x0100`, shift `8`, max `1`, selector position |
| RIGHT_DDI_PB_03 | Pushbutton 3<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x0200`, shift `9`, max `1`, selector position |
| RIGHT_DDI_PB_04 | Pushbutton 4<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x0400`, shift `10`, max `1`, selector position |
| RIGHT_DDI_PB_05 | Pushbutton 5<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x0800`, shift `11`, max `1`, selector position |
| RIGHT_DDI_PB_06 | Pushbutton 6<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x1000`, shift `12`, max `1`, selector position |
| RIGHT_DDI_PB_07 | Pushbutton 7<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x2000`, shift `13`, max `1`, selector position |
| RIGHT_DDI_PB_08 | Pushbutton 8<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x4000`, shift `14`, max `1`, selector position |
| RIGHT_DDI_PB_09 | Pushbutton 9<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x8000`, shift `15`, max `1`, selector position |
| RIGHT_DDI_PB_10 | Pushbutton 10<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7428`, mask `0x0100`, shift `8`, max `1`, selector position |
| RIGHT_DDI_PB_11 | Pushbutton 11<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7428`, mask `0x0200`, shift `9`, max `1`, selector position |
| RIGHT_DDI_PB_12 | Pushbutton 12<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7428`, mask `0x0400`, shift `10`, max `1`, selector position |
| RIGHT_DDI_PB_13 | Pushbutton 13<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7428`, mask `0x0800`, shift `11`, max `1`, selector position |
| RIGHT_DDI_PB_14 | Pushbutton 14<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7428`, mask `0x1000`, shift `12`, max `1`, selector position |
| RIGHT_DDI_PB_15 | Pushbutton 15<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7428`, mask `0x2000`, shift `13`, max `1`, selector position |
| RIGHT_DDI_PB_16 | Pushbutton 16<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7428`, mask `0x4000`, shift `14`, max `1`, selector position |
| RIGHT_DDI_PB_17 | Pushbutton 17<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7428`, mask `0x8000`, shift `15`, max `1`, selector position |
| RIGHT_DDI_PB_18 | Pushbutton 18<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742A`, mask `0x0100`, shift `8`, max `1`, selector position |
| RIGHT_DDI_PB_19 | Pushbutton 19<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742A`, mask `0x0200`, shift `9`, max `1`, selector position |
| RIGHT_DDI_PB_20 | Pushbutton 20<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742A`, mask `0x0400`, shift `10`, max `1`, selector position |

### Right Engine Fire Warning Extinguisher Light

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| FIRE_RIGHT_LT | FIRE RIGHT (red)<br>`led` | **Output only**<br>No documented input | `integer` at `0x740C`, mask `0x0010`, shift `4`, max `1`, 0 if light is off, 1 if light is on |
| RIGHT_FIRE_BTN | Right Engine/AMAD Fire Warning/Extinguisher Light<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740C`, mask `0x0020`, shift `5`, max `1`, selector position |
| RIGHT_FIRE_BTN_COVER | Right Engine/AMAD Fire Warning Cover<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x740C`, mask `0x0040`, shift `6`, max `1`, selector position |

### Right Essential Circuit Breakers

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| CB_FCS_CHAN3 | CB FCS CHAN 3, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74CC`, mask `0x8000`, shift `15`, max `1`, selector position |
| CB_FCS_CHAN4 | CB FCS CHAN 4, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74CE`, mask `0x0400`, shift `10`, max `1`, selector position |
| CB_HOOOK | CB HOOK, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74CE`, mask `0x0800`, shift `11`, max `1`, selector position |
| CB_LG | CB LG, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74CE`, mask `0x1000`, shift `12`, max `1`, selector position |
| FCS_BIT_SW | FCS BIT Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74CE`, mask `0x2000`, shift `13`, max `1`, selector position |

### Rudder Pedal Adjust Level

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| RUDDER_PEDAL_ADJUST | Rudder Pedal Adjust Lever<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x749C`, mask `0x2000`, shift `13`, max `1`, selector position |

### Select Jettison Button

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| ANTI_SKID_SW | Anti Skid<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7480`, mask `0x1000`, shift `12`, max `1`, selector position |
| FLAP_SW | FLAP Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x7484`, mask `0x0300`, shift `8`, max `2`, selector position |
| HOOK_BYPASS_SW | HOOK BYPASS Switch, FIELD/CARRIER<br>`action` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set the switch position -- 0 = off, 1 = on; max_value=1)<br>`action` (toggle switch state; argument=TOGGLE) | `integer` at `0x7480`, mask `0x4000`, shift `14`, max `1`, selector position |
| HYD_IND_BRAKE | HYD Indicator Brake<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7506`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| LAUNCH_BAR_SW | Launch Bar Switch<br>`action` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set the switch position -- 0 = off, 1 = on; max_value=1)<br>`action` (toggle switch state; argument=TOGGLE) | `integer` at `0x7480`, mask `0x2000`, shift `13`, max `1`, selector position |
| LDG_TAXI_SW | LDG/TAXI LIGHT Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7480`, mask `0x8000`, shift `15`, max `1`, selector position |
| SEL_JETT_BTN | Selective Jettison Pushbutton<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7480`, mask `0x0100`, shift `8`, max `1`, selector position |
| SEL_JETT_KNOB | Selective Jettison Knob<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=4) | `integer` at `0x7480`, mask `0x0E00`, shift `9`, max `4`, selector position |

### Sensor Panel

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| FLIR_SW | FLIR Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74C8`, mask `0x3000`, shift `12`, max `2`, selector position |
| INS_SW | INS Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=7) | `integer` at `0x74CA`, mask `0x3800`, shift `11`, max `7`, selector position |
| LST_NFLR_SW | LST/NFLR Switch, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74C8`, mask `0x8000`, shift `15`, max `1`, selector position |
| LTD_R_SW | LTD/R Switch, ARM/SAFE<br>`action` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set the switch position -- 0 = off, 1 = on; max_value=1)<br>`action` (toggle switch state; argument=TOGGLE) | `integer` at `0x74C8`, mask `0x4000`, shift `14`, max `1`, selector position |
| RADAR_SW | RADAR Switch Change ,OFF/STBY/OPR/EMERG(PULL)<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=3) | `integer` at `0x74CA`, mask `0x0300`, shift `8`, max `3`, selector position |
| RADAR_SW_PULL | RADAR Switch Pull (MW to pull), OFF/STBY/OPR/EMERG(PULL)<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74CA`, mask `0x0400`, shift `10`, max `1`, selector position |

### Standby Airspeed Indicator

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| STBY_ASI_AIRSPEED | Airspeed<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74F0`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |

### Standby Altimeter

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| STBY_ALT_10000_FT_CNT | 10000 ft count<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74F6`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| STBY_ALT_1000_FT_CNT | 1000 ft count<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74F8`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| STBY_ALT_100_FT_PTR | 100 ft pointer<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74F4`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| STBY_PRESS_ALT | Pressure Setting Knob<br>`analog_dial` | **Interactable**<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x74F2`, mask `0xFFFF`, shift `0`, max `65535`, the rotation of the knob in the cockpit (not the value that is controlled by this knob!) |
| STBY_PRESS_SET_0 | Pressure Setting 1<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74FA`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| STBY_PRESS_SET_1 | Pressure Setting 2<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74FC`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| STBY_PRESS_SET_2 | Pressure Setting 3<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74FE`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |

### Standby Attitude Reference Indicator

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| SAI_ATT_WARNING_FLAG | SAI Attitude Warning Flag<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74E8`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| SAI_ATT_WARN_FLAG_L | SAI Attitude Warning Flag as Light<br>`led` | **Output only**<br>No documented input | `integer` at `0x74D6`, mask `0x0100`, shift `8`, max `1`, 0 if light is off, 1 if light is on |
| SAI_BANK | SAI Bank<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74E6`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| SAI_CAGE | SAI Pull to uncage<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747E`, mask `0x0400`, shift `10`, max `1`, selector position |
| SAI_MAN_PITCH_ADJ | SAI Manual Pitch Adjustment<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74EA`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| SAI_PITCH | SAI Pitch<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74E4`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| SAI_POINTER_HOR | SAI Horisontal Pointer<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x756C`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| SAI_POINTER_VER | SAI Vertical Pointer<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x756A`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| SAI_RATE_OF_TURN | SAI Rate Of Turn<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74EE`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| SAI_SET | SAI Adjust Attitude<br>`analog_dial` | **Interactable**<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x74E2`, mask `0xFFFF`, shift `0`, max `65535`, the rotation of the knob in the cockpit (not the value that is controlled by this knob!) |
| SAI_SLIP_BALL | SAI Slip Ball<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x74EC`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| SAI_TEST_BTN | SAI Test Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x747E`, mask `0x0200`, shift `9`, max `1`, selector position |

### Standby Compass

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| SBY_COMPASS_BANK | Standby Compass Bank<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7464`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| SBY_COMPASS_HDG | Standby Compass Heading<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7460`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| SBY_COMPASS_PITCH | Standby Compass Pitch<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7462`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |

### Standby Rate of Climb Indicator

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| VSI | Vertical Speed<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7500`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |

### Station Jettison Select

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| SJ_CTR | Station Jettison Select Button, CENTER<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742C`, mask `0x8000`, shift `15`, max `1`, selector position |
| SJ_CTR_LT | CTR Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x742E`, mask `0x4000`, shift `14`, max `1`, 0 if light is off, 1 if light is on |
| SJ_LI | Station Jettison Select Button, LEFT IN<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742E`, mask `0x0400`, shift `10`, max `1`, selector position |
| SJ_LI_LT | LI Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x742E`, mask `0x8000`, shift `15`, max `1`, 0 if light is off, 1 if light is on |
| SJ_LO | Station Jettison Select Button, LEFT OUT<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742E`, mask `0x0800`, shift `11`, max `1`, selector position |
| SJ_LO_LT | LO Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7430`, mask `0x0100`, shift `8`, max `1`, 0 if light is off, 1 if light is on |
| SJ_RI | Station Jettison Select Button, RIGHT IN<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742E`, mask `0x1000`, shift `12`, max `1`, selector position |
| SJ_RI_LT | RI Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7430`, mask `0x0200`, shift `9`, max `1`, 0 if light is off, 1 if light is on |
| SJ_RO | Station Jettison Select Button, RIGHT OUT<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x742E`, mask `0x2000`, shift `13`, max `1`, selector position |
| SJ_RO_LT | RO Light (green)<br>`led` | **Output only**<br>No documented input | `integer` at `0x7430`, mask `0x0400`, shift `10`, max `1`, 0 if light is off, 1 if light is on |

### Stick

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| STICK_GUN_TRIGGER2 | Stick Gun Trigger, SECOND DETENT (Press to shoot)<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D2`, mask `0x0400`, shift `10`, max `1`, selector position |
| STICK_N_WHEEL_SW | Stick Undesignate/Nose Wheel Steer Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D2`, mask `0x1000`, shift `12`, max `1`, selector position |
| STICK_PADDLE_SW | Stick Autopilot/Nosewheel Steering Disengage (Paddle) Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D2`, mask `0x0800`, shift `11`, max `1`, selector position |
| STICK_RECCE_SW | Stick RECCE Event Mark Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D2`, mask `0x0200`, shift `9`, max `1`, selector position |
| STICK_WEAP_REL_BTN | Stick Weapon Release Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D2`, mask `0x0100`, shift `8`, max `1`, selector position |

### TODO

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| LEFT_VIDEO_BIT | Left Video Sensor BIT Initiate Pushbutton - Push to initiate BIT<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D0`, mask `0x2000`, shift `13`, max `1`, selector position |
| NUC_WPN_SW | NUC WPN Switch, ENABLE/DISABLE (no function)<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D0`, mask `0x8000`, shift `15`, max `1`, selector position |
| RIGHT_VIDEO_BIT | Right Video Sensor BIT Initiate Pushbutton - Push to initiate BIT<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D0`, mask `0x4000`, shift `14`, max `1`, selector position |

### Throttle Quadrant

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| INT_THROTTLE_LEFT | Left Throttle Position<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x7588`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| INT_THROTTLE_RIGHT | Right Throttle Position<br>`analog_gauge` | **Output only**<br>No documented input | `integer` at `0x758A`, mask `0xFFFF`, shift `0`, max `65535`, gauge position |
| THROTTLE_ATC_SW | Throttle ATC Engage/Disengage Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D4`, mask `0x0400`, shift `10`, max `1`, selector position |
| THROTTLE_CAGE_BTN | Throttle Cage/Uncage Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D2`, mask `0x2000`, shift `13`, max `1`, selector position |
| THROTTLE_DISP_SW | Throttle Dispense Switch, Aft(FLARE)/Center(OFF)/Forward(CHAFF)<br>`selector` | **Interactable**<br>`set_state` (set the switch position -- 0 = held left/down, 1 = centered, 2 = held right/up; max_value=2) | `integer` at `0x74D2`, mask `0xC000`, shift `14`, max `2`, selector position |
| THROTTLE_EXT_L_SW | Throttle Exterior Lights Switch, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D4`, mask `0x1000`, shift `12`, max `1`, selector position |
| THROTTLE_FOV_SEL_SW | Throttle RAID/FLIR FOV Select Button<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74D4`, mask `0x0800`, shift `11`, max `1`, selector position |
| THROTTLE_FRICTION | Throttles Friction Adjusting Lever<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7522`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| THROTTLE_RADAR_ELEV | Throttle Radar Elevation Control<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x7556`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| THROTTLE_SPEED_BRK | Throttle Speed Brake Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74D4`, mask `0x0300`, shift `8`, max `2`, selector position |

### Up Front Controller (UFC)

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| UFC_0 | UFC Keyboard Pushbutton, 0<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x0200`, shift `9`, max `1`, selector position |
| UFC_1 | UFC Keyboard Pushbutton, 1<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x0400`, shift `10`, max `1`, selector position |
| UFC_2 | UFC Keyboard Pushbutton, 2<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x0800`, shift `11`, max `1`, selector position |
| UFC_3 | UFC Keyboard Pushbutton, 3<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x1000`, shift `12`, max `1`, selector position |
| UFC_4 | UFC Keyboard Pushbutton, 4<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x2000`, shift `13`, max `1`, selector position |
| UFC_5 | UFC Keyboard Pushbutton, 5<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x4000`, shift `14`, max `1`, selector position |
| UFC_6 | UFC Keyboard Pushbutton, 6<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x8000`, shift `15`, max `1`, selector position |
| UFC_7 | UFC Keyboard Pushbutton, 7<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x0001`, shift `0`, max `1`, selector position |
| UFC_8 | UFC Keyboard Pushbutton, 8<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x0002`, shift `1`, max `1`, selector position |
| UFC_9 | UFC Keyboard Pushbutton, 9<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x0004`, shift `2`, max `1`, selector position |
| UFC_ADF | ADF Function Select Switch<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x7416`, mask `0x00C0`, shift `6`, max `2`, selector position |
| UFC_AP | Function Selector Pushbutton, A/P<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0080`, shift `7`, max `1`, selector position |
| UFC_BCN | Function Selector Pushbutton, BCN<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x1000`, shift `12`, max `1`, selector position |
| UFC_BRT | Brightness Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x741E`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| UFC_CLR | Keyboard Pushbutton, CLR<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x0008`, shift `3`, max `1`, selector position |
| UFC_COMM1_CHANNEL_SELECT | COMM 1 Channel Select Knob<br>`fixed_step_dial` | **Interactable**<br>`fixed_step` (turn left or right) | No output descriptor in this reference |
| UFC_COMM1_DISPLAY | Comm 1 Display<br>`display` | **Output only**<br>No documented input | `string` at `0x7424`, length `2`, Comm 1 Display |
| UFC_COMM1_PULL | COMM 1 Channel Selector Knob Pull<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x4000`, shift `14`, max `1`, selector position |
| UFC_COMM1_VOL | COMM 1 Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x741A`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| UFC_COMM2_CHANNEL_SELECT | COMM 2 Channel Select Knob<br>`fixed_step_dial` | **Interactable**<br>`fixed_step` (turn left or right) | No output descriptor in this reference |
| UFC_COMM2_DISPLAY | Comm 2 Display<br>`display` | **Output only**<br>No documented input | `string` at `0x7426`, length `2`, Comm 2 Display |
| UFC_COMM2_PULL | COMM 2 Channel Selector Knob Pull<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x8000`, shift `15`, max `1`, selector position |
| UFC_COMM2_VOL | COMM 2 Volume Control Knob<br>`limited_dial` | **Interactable**<br>`set_state` (set the position of the dial; max_value=65535)<br>`variable_step` (turn the dial left or right; max_value=65535; suggested_step=3200) | `integer` at `0x741C`, mask `0xFFFF`, shift `0`, max `65535`, position of the potentiometer |
| UFC_DL | Function Selector Pushbutton, D/L<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0800`, shift `11`, max `1`, selector position |
| UFC_EMCON | Emission Control Pushbutton<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x0100`, shift `8`, max `1`, selector position |
| UFC_ENT | Keyboard Pushbutton, ENT<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7418`, mask `0x0010`, shift `4`, max `1`, selector position |
| UFC_IFF | Function Selector Pushbutton, IFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0100`, shift `8`, max `1`, selector position |
| UFC_ILS | Function Selector Pushbutton, ILS<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0400`, shift `10`, max `1`, selector position |
| UFC_IP | I/P Pushbutton<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x0020`, shift `5`, max `1`, selector position |
| UFC_ONOFF | Function Selector Pushbutton, ON/OFF<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x2000`, shift `13`, max `1`, selector position |
| UFC_OPTION_CUEING_1 | Option Cueing 1<br>`display` | **Output only**<br>No documented input | `string` at `0x7428`, length `1`, Option Cueing 1 |
| UFC_OPTION_CUEING_2 | Option Cueing 2<br>`display` | **Output only**<br>No documented input | `string` at `0x742A`, length `1`, Option Cueing 2 |
| UFC_OPTION_CUEING_3 | Option Cueing 3<br>`display` | **Output only**<br>No documented input | `string` at `0x742C`, length `1`, Option Cueing 3 |
| UFC_OPTION_CUEING_4 | Option Cueing 4<br>`display` | **Output only**<br>No documented input | `string` at `0x742E`, length `1`, Option Cueing 4 |
| UFC_OPTION_CUEING_5 | Option Cueing 5<br>`display` | **Output only**<br>No documented input | `string` at `0x7430`, length `1`, Option Cueing 5 |
| UFC_OPTION_DISPLAY_1 | Option Display 1<br>`display` | **Output only**<br>No documented input | `string` at `0x7432`, length `4`, Option Display 1 |
| UFC_OPTION_DISPLAY_2 | Option Display 2<br>`display` | **Output only**<br>No documented input | `string` at `0x7436`, length `4`, Option Display 2 |
| UFC_OPTION_DISPLAY_3 | Option Display 3<br>`display` | **Output only**<br>No documented input | `string` at `0x743A`, length `4`, Option Display 3 |
| UFC_OPTION_DISPLAY_4 | Option Display 4<br>`display` | **Output only**<br>No documented input | `string` at `0x743E`, length `4`, Option Display 4 |
| UFC_OPTION_DISPLAY_5 | Option Display 5<br>`display` | **Output only**<br>No documented input | `string` at `0x7442`, length `4`, Option Display 5 |
| UFC_OS1 | Option Select Pushbutton 1<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x0001`, shift `0`, max `1`, selector position |
| UFC_OS2 | Option Select Pushbutton 2<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x0002`, shift `1`, max `1`, selector position |
| UFC_OS3 | Option Select Pushbutton 3<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x0004`, shift `2`, max `1`, selector position |
| UFC_OS4 | Option Select Pushbutton 4<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x0008`, shift `3`, max `1`, selector position |
| UFC_OS5 | Option Select Pushbutton 5<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7416`, mask `0x0010`, shift `4`, max `1`, selector position |
| UFC_SCRATCHPAD_NUMBER_DISPLAY | Scratchpad Number Display<br>`display` | **Output only**<br>No documented input | `string` at `0x7446`, length `8`, Scratchpad Number Display |
| UFC_SCRATCHPAD_STRING_1_DISPLAY | Scratchpad String 1 Display<br>`display` | **Output only**<br>No documented input | `string` at `0x744E`, length `2`, Scratchpad String 1 Display |
| UFC_SCRATCHPAD_STRING_2_DISPLAY | Scratchpad String 2 Display<br>`display` | **Output only**<br>No documented input | `string` at `0x7450`, length `2`, Scratchpad String 2 Display |
| UFC_TCN | Function Selector Pushbutton, TCN<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x7414`, mask `0x0200`, shift `9`, max `1`, selector position |

### Wing Fold Switch

| Control | Description / kind | Interaction | DCS-BIOS output reference |
|---|---|---|---|
| WING_FOLD_PULL | Wing Fold Control Handle Pull<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=1)<br>`action` (Toggle switch state; argument=TOGGLE) | `integer` at `0x74A0`, mask `0x0800`, shift `11`, max `1`, selector position |
| WING_FOLD_ROTATE | Wing Fold Control Handle<br>`selector` | **Interactable**<br>`fixed_step` (switch to previous or next state)<br>`set_state` (set position; max_value=2) | `integer` at `0x74A0`, mask `0x3000`, shift `12`, max `2`, selector position |

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
