# Native DCS control verification

Use DCS's own cockpit scripts and raw export data, not DCS-BIOS addresses or
transformed values. Static mapping checks, observed cockpit outputs, and input
actuation are separate evidence. A correct argument number alone does not prove
that a command works or that a lamp's threshold is correct.

## Master Arm: initial static check

Checked locally on 2026-10-25 against DCS **2.9.30.28738**, using
`Mods/aircraft/FA-18C/Cockpit/Scripts/` in the installed simulator.
No DCS source code is copied into this repository.

| Catalogue control | Device | Command | Argument | Native evidence |
|---|---|---|---|---|
| `0x0101 MASTER_ARM` | 23 | 3003 | 49 | `clickabledata.lua`: `pnt_49`, `devices.SMS`, `SMS_commands.MasterArmSw` |
| `0x0102 MODE_AA` | 23 | 3001 | 458 | `clickabledata.lua`: `pnt_458`, `SMS_commands.AA_MasterModeSw` |
| `0x0103 MODE_AG` | 23 | 3002 | 459 | `clickabledata.lua`: `pnt_459`, `SMS_commands.AG_MasterModeSw` |
| `0x0104 AA_LT` | - | - | 47 | Not established statically; live correlation recorded below |
| `0x0105 AG_LT` | - | - | 48 | Not established statically; live correlation recorded below |
| `0x0106 READY_LT` | - | - | 44 | Candidate only; requires live visual correlation |
| `0x0107 DISCH_LT` | - | - | 45 | Candidate only; requires live visual correlation |
| `0x0108 FIRE_EXT` | 12 | 3008 | 46 | `clickabledata.lua`: `pnt_46`, `devices.ENGINES_INTERFACE`, `engines_commands.FireExtghDischSw` |
| `0x0109 EMER_JETT` | 23 | 3004 | 50 | `clickabledata.lua`: `pnt_50`, `SMS_commands.EmerJettSw` |

`devices.lua` resolves SMS to 23 and ENGINES_INTERFACE to 12.
`command_defs.lua` resolves the five command symbols above.
`clickable_defs.lua` defines button press/release values as 1/0 and the two-position
switch argument limits as 0..1. The animated switch uses directional click
increments; absolute `performClickableAction` behavior still needs a live test.
The four lamp candidates were not established by this static check.

**Status:** the five input mappings match the catalogue. The live run below
correlates five output arguments; native command actuation is not tested.
All nine entries remain
`verified: false` in [the catalogue](../catalog/fa18c.json). Leave their existing
source attribution intact until all fields of each entry have native evidence.
Do not promote the lamps' inherited 0.3 thresholds based on a static check.

## Live results: 2026-10-25

Native export session `2026-10-25T15:24:56Z-1`, DCS **2.9.30.28738**.
The user performed the sequence below with cockpit interactions and reported
that the A/A and A/G lamps behaved normally. Device 0 was sampled independently
of DCS-BIOS; although existing exporters remained installed, no DCS-BIOS packets,
memory-map addresses, or transformed values were used as evidence.

The successful capture contains **173 records**: start/active/stop markers,
**153 full rows** (17 snapshots of nine arguments), and **17 change rows**.
The last full snapshot is at model time **16.079 seconds**. The capture ended
normally and logged no verifier errors. An earlier failed attempt exposed the
restricted DCS file API and produced no usable samples; it is not evidence.

| Output | Raw argument | Observed values / model times (seconds) | Result |
|---|---|---|---|
| Master Arm | 49 | 0 initially; animation begins 2.439, reaches 1 at 2.479; returns to 0 at 5.666 | SAFE/ARM/SAFE endpoints observed |
| A/A button | 458 | Press animation begins 7.319, reaches 1 at 7.345; release reaches 0 at 10.226 | Press/release output observed |
| A/G button | 459 | Press animation begins 11.773, reaches 1 at 11.786; release reaches 0 at 14.892 | Press/release output observed |
| A/A lamp | 47 | 0 initially; 1 at 7.345; 0 when A/G selected at 11.786 | Correlates with A/A mode and visual report |
| A/G lamp | 48 | 0 initially; 1 at 11.786, remains 1 through 16.079 | Correlates with A/G mode and visual report |
| READY / DISCH | 44 / 45 | Both 0 throughout | No on-state evidence; unverified |
| FIRE EXT / EMER JETT | 46 / 50 | Both 0 throughout | No press evidence; unverified |

The mode lamps are latched mode outputs, not copies of the momentary button
arguments: A/A stayed lit after its button release; A/G stayed lit after its
button release. The existing native exporter treats these separately, consistent
with the capture. Button and switch arguments include intermediate animation
values, so only settled endpoints should establish state mappings.

Raw lamp levels were exactly 0 and 1 in this run. The inherited 0.3 threshold
classifies these observations correctly, but this does **not** establish correct
behavior under dimming, light tests, or other electrical conditions.
Native `performClickableAction`, bridge input delivery, and physical-panel
end-to-end operation remain untested. No catalogue values or verification flags
are changed on the strength of this output-only run.

The raw CSV was extracted from the log before rotation. Its SHA-256 is
`45baac57c5cc0177b7108427e59e43b7cb3d19f54c6952592c10ccca1280c7ee`.

## Read-only output capture

[HornetLinkVerify.lua](../Programs/dcsbios-serial-bridge/lua/HornetLinkVerify.lua)
is independent of DCS-BIOS and the bridge. It reads device 0 only, opens no sockets,
and never sends cockpit commands. It records the nine raw candidate arguments,
without thresholds, rounding to integer states, or catalogue interpretation.
Previous export callbacks are chained; existing export integrations can remain.

1. Close the current mission. Copy the verifier into
   `Saved Games\DCS\Scripts\HornetLinkVerify.lua`.
2. Back up `Saved Games\DCS\Scripts\Export.lua`, then append this loader **once**:

   ```lua
   local hlvOk, hlvErr = pcall(dofile, lfs.writedir() .. "Scripts/HornetLinkVerify.lua")
   if not hlvOk then log.write("HornetLinkVerify", log.ERROR, tostring(hlvErr)) end
   ```

3. Start a fresh, disposable single-player F/A-18C mission with no stores.
   Verify that `dcs.log` contains a `HornetLinkVerify` capture-start message.
4. With the aircraft powered, wait two seconds at each settled state:
   SAFE, ARM, SAFE; then press/hold/release A/A, then A/G.
   Note the model time and the visible A/A and A/G lamps for each step.
5. Only in a disposable mission, manually press/hold/release FIRE EXT and
   EMER JETT if desired. They have simulator effects even though the verifier
   itself is read-only. Otherwise leave these controls unverified.
6. Observe READY and DISCH in known off/on conditions if available. Record any
   light-test conditions separately; a lamp that never changes is not verified.
7. End the mission to record the end of the capture.

Output: `Saved Games\DCS\Logs\dcs.log`. `HornetLinkVerify` messages starting with
`CSV ` carry records with these columns:

```text
session,model_time,event,arg,value
```

Strip the DCS log prefix and `CSV ` marker, and add the header above when
extracting a CSV. Preserve the log before another DCS launch rotates it.
The DCS export file API is not the complete Lua file API (the live test exposed
an unavailable `seek` method); the verifier uses DCS's own logger instead of
assuming standard file handles, flushing, or return conventions.

Session IDs contain UTC start time and a per-load counter. Model times are
simulator seconds. `full` rows contain all nine raw values on cockpit entry and
once per model second; `change` rows record changes after each rendered frame.
`start`, `active`, `inactive`, and `stop` delimit capture state. Values preserve
floating-point precision, including intermediate animation values. Pulses shorter
than one rendered frame can be missed. Persistence and rotation follow DCS's
logger; a crash can lose recently buffered messages.
Capture errors are reported in `dcs.log` and disable capture until the next
export start, rather than silently writing fabricated zero values.

To remove the verifier, close the mission, remove only its two loader lines from
`Export.lua`, and remove its script. Preserve other exporters and captured logs.

## Completing verification

For each control, retain the DCS version, mission conditions, model-time interval,
raw argument values, and visual observations. Check settled endpoints and both
directions; distinguish animated button arguments from resulting lamp outputs.
For lamps, measure raw off/on levels and dimming before choosing a threshold.

After output correlation, separately test native input commands in a disposable
mission, checking both the cockpit animation and the intended system state.
The capture script deliberately provides no command injection.
Do not claim bridge/hardware end-to-end verification from mouse interactions.

Only after those checks, update the corresponding entry's native `dcs.source`
and `verified` flag in [the catalogue](../catalog/fa18c.json), then regenerate:

```powershell
python Programs\tools\generate_hornet_catalog.py
python Programs\tools\generate_hornet_catalog.py --check
```

Run the capture regression tests from the repository root with Lua 5.1:

```powershell
lua tests\test_native_verification.lua
```

## Broad read-only discovery and automated evidence comparison

[HornetLinkCapture.lua](../Programs/dcsbios-serial-bridge/lua/HornetLinkCapture.lua)
is the broader recorder. Use it instead of the small panel verifier for a full
cockpit investigation. It never sends commands, opens no sockets, and does not
require DCS-BIOS. Its only output is the local DCS log.

### Scope and limits

- Probe device 0 cockpit argument slots **0..4095** at up to **10 Hz**.
  These are candidate slots, not 4,096 confirmed controls. An unused argument
  may return zero without an error. No public enumeration proves which slots
  exist; the finite scan bounds are recorded explicitly in the capture.
- Probe `list_indication` IDs **0..63** at **1 Hz**. Preserve the complete returned
  text, including unknown fields, nested elements, repeated names, whitespace,
  and blank values. Empty responses are not proof that an indicator exists.
  Indicators not exposing text cannot be reconstructed as images.
- Call **38 allowlisted, zero-argument native telemetry APIs** at **1 Hz**:
  version, own-aircraft identity/state/position, air data, attitude, engine and
  mechanism data, payload, navigation/route, HSI, warning/sighting systems, camera,
  velocity, wind, and available related state. Retain nested tables and all
  return positions, including nil returns. The API names are in each start record.
- Do not enumerate world objects, other players, arbitrary cockpit devices,
  internal simulator memory, or unknown functions. This is a broad accessible
  export, not a claim to extract every simulator variable.
- Record missing APIs, failed reads, invalid numbers, and unsupported table
  structures explicitly. Per-stream errors do not prevent other streams from
  being captured. Changed error states and recoveries are recorded, not logged
  every frame. Unexpected recorder errors produce a fatal record.
- Suppress only exact consecutive repetition. Argument batches contain changed
  values after their initial baseline. Indications and telemetry retain changed
  snapshots; the readable conversion further removes unchanged fields.
  Five-second heartbeats and stop records retain counters. There is no epsilon,
  integer normalization, lossy quantization, or cycle removal hiding real changes.
- Keep new baselines separate after aircraft changes, unit-name changes,
  or model-clock resets. Events shorter than the sample interval can be missed;
  a non-animated button may not expose its pressed state as an argument at all.
  CPU/frame-rate overhead and logs can grow significantly during a long mission.

The scan bounds and sampling intervals are constants at the top of the script.
Increasing them is an explicit performance trade-off, not an automatic fallback.
The native clickable point labels checked in this installation extend to 575;
this does not establish the maximum internal cockpit argument.

### Install and run

1. Back up `Saved Games\DCS\Scripts\Export.lua`.
2. Copy the broad recorder to `Saved Games\DCS\Scripts\HornetLinkCapture.lua`.
3. Append this loader once, after existing exporters:

   ```lua
   local hlcOk, hlcErr = pcall(dofile, lfs.writedir() .. "Scripts/HornetLinkCapture.lua")
   if not hlcOk then log.write("HornetLinkCapture", log.ERROR, tostring(hlcErr)) end
   ```

4. Start a new F/A-18C mission. Use disposable single-player scenarios for
   control sweeps; the recorder cannot prevent the effects of manual inputs.
   Hold buttons at least 0.3 seconds, exercise all switch positions and knob
   ranges, and visit different display pages. Cold-start, powered, lamp-test,
   and dimming scenarios provide different output evidence. Read-only recording
   does not automate these state changes.
5. End the mission. Save/convert the log **before restarting DCS**, which can
   rotate it. Remove just this loader and script when capture is no longer needed.
   Preserve other exporters. Log data can include position, route, and unit names;
   keep it local unless deliberately sharing it.

### Convert into human-readable and structured exports

From the repository root, using Python's standard library only:

```powershell
python Programs\tools\decode_native_capture.py `
  "$env:USERPROFILE\Saved Games\DCS\Logs\dcs.log" `
  --dcs-root "E:\DCS World" `
  --output "$env:USERPROFILE\Desktop\hornet-native-capture"
```

Choose a new output base path for each conversion; existing evidence is never
overwritten. The parent directory is created if needed. Four files are produced:

| Suffix | Contents |
|---|---|
| `.summary.txt` | Start here: capture totals, missing states, text-length mismatches, all catalogue controls, stream availability, diagnostics, and optional native clickable point label hints |
| `.txt` | Complete readable timestamp-grouped change timeline; initial zero slots condensed into ranges, unchanged telemetry/indication fields omitted |
| `.jsonl` | Lossless reconstructed structured records: baselines, deltas, complete raw indication strings, packed telemetry, errors, and counters |
| `.coverage.json` | Machine-readable per-cockpit-segment evidence for every current catalogue control, including missing expected states, min/max values, field lengths, and provisional native point labels |

The optional `--dcs-root` reads direct point labels from the installed Hornet
`clickabledata.lua` and records that file's SHA-256. These are **labeling hints**,
not parsed or verified device/command/argument mappings. The exporter scans
outside the catalogue as well; unmatched arguments remain visible by number.
The catalogue itself remains unchanged.

Each DCS log record is transported in numbered 1,500-byte chunks. Conversion
checks chunk ordering, sequence continuity, schema, finite values, start/stop
boundaries, and the existence of an active Hornet segment. Missing/truncated,
reordered, duplicate, or fatal captures fail explicitly, rather than producing a
success-shaped partial report. Pass multiple non-overlapping log files in
chronological order if a complete capture spans files. Partial sessions are not
silently accepted; allow a mission to stop before conversion.

Non-ASCII Lua string bytes are encoded losslessly as U+0080..U+00FF JSON escapes.
The raw export preserves this byte mapping; presentation decodes UTF-8 where
valid. Numeric table keys are represented as JSON object strings. Telemetry
return count `n` and each return's `present` flag distinguish nil from false or
an empty table. The raw indication snapshot remains authoritative if the
readable field parser cannot understand its layout.

Coverage labels mean **changed**, **constant-only**, **field-observed**,
**field-not-observed**, **not-observed**, or **no-output-mapping**; they never
mean verified. Expected states and lamp thresholds come from the unverified
catalogue, not an independent native specification. Seeing all endpoints is
necessary evidence, not proof of identity, command delivery, or hardware
operation. Endpoints are compared with a 0.0001 tolerance only in the coverage
analysis; captured numeric values are never rounded or altered.

### Broad live sweep: 2026-10-25

Session `2026-10-25T15:40:47Z-1`, DCS **2.9.30.28738**. The user reported a
cold-start scenario and pressing all buttons/switches. The recorder captured
**954.439 model seconds**, ending normally:

- **27,607 complete structured records**, reconstructed from 30,064 log chunks.
- **8,948 argument scans**, **953 detail scans**, **zero read/serialization errors**.
- **4,096 argument slots read; 379 changed**. Zero-returning unused slots are
  included in that first count, not promoted to confirmed controls.
- **36,554,172 exact repeated samples suppressed**.
- All **38 telemetry APIs** returned without an exception.
- Nonempty indication text appeared on IDs **1, 2, 3, 4, 5, 6, 7, 9, 10, 11,
  12, 13, 15** during the run. Remaining probes can be empty/unavailable.
- Catalogue evidence: **81 changed argument controls**, **13 constant-only
  argument controls**, **27 text fields observed**, and **2 rotary controls
  with no direct output mapping**.

Outstanding observations:

- Pressed argument state 1 was not observed for UFC OS1..OS5, EMCON, COMM2_PULL,
  or IFEI MODE/QTY/UP/DOWN/ZONE/ET. This does **not** prove missed user actions or
  broken commands. Several use the native non-animated `short_way_button`
  helper; sampled argument state is not sufficient to validate their behavior.
  Check resulting display/system state or separately instrument native commands.
- UFC COMM1_CHANNEL and COMM2_CHANNEL have no direct argument readback in the
  catalogue. Their displayed channel text is separate evidence.
- Native IFEI `txt_TEMP_L`, `txt_TEMP_R`, `txt_FF_L`, and `txt_FF_R` returned
  four-character **`1000`**, while the current catalogue allows three characters.
  These are real raw values, not parser-added padding. Catalogue/export text
  sizing needs a deliberate follow-up; no truncation is applied to this capture.
- READY, DISCH, FIRE EXT, and EMER JETT arguments now have 0/1 observations in
  this broad run, unlike the earlier short Master Arm run. Visual correlation,
  native command actuation, and complete control verification remain separate.

No catalogue mapping, text size, or verification flag was changed automatically.

### Follow-up for the 13 constant arguments: 2026-10-25

The user confirmed those controls had been missed in the broad sweep and
performed a targeted repeat. Session `2026-10-25T16:15:11Z-1` captured
**102.657 model seconds**, **961 argument scans**, and **103 detail scans**,
ending normally with **zero recorder errors**.

All 13 previously constant arguments now show both **0 and 1**, and finish at
**0**:

| Control | Argument | Observed transitions |
|---|---|---|
| UFC OS1 | 100 | 2 |
| UFC OS2 | 101 | 2 |
| UFC OS3 | 102 | 2 |
| UFC OS4 | 103 | 2 |
| UFC OS5 | 106 | 2 |
| UFC EMCON | 110 | 4 |
| UFC COMM2 pull | 127 | 8 |
| IFEI MODE | 168 | 24 |
| IFEI QTY | 169 | 14 |
| IFEI Up | 170 | 4 |
| IFEI Down | 171 | 2 |
| IFEI ZONE | 172 | 6 |
| IFEI ET | 173 | 2 |

Multiple transitions reflect repeated actions or sampled animation values, not
an inferred count of commands. The earlier constant results are no longer an
outstanding output-state gap. In particular, these observations demonstrate
that the non-animated helper did not prevent pressed-state readback in this
test; the earlier helper/sampling explanation was only a possibility, not an
established cause.

The saved retest report includes the earlier session because DCS retained both
in the same log; sessions remain separate in the coverage results. Comparing
the two runs, all **94 catalogue argument-backed controls** have now changed
in at least one run, and all **27 text fields** were observed in the broad run.
The **two rotary controls** still lack direct argument mappings, and the
**four IFEI text-length mismatches** remain outstanding. Output observations
still do not verify native input-command actuation, visual identity for every
control, or physical-panel behavior. Catalogue verification flags are unchanged.

### Tests

```powershell
lua tests\test_native_capture.lua
python -m unittest discover -s tests -p 'test_native_capture.py'
```

The Lua tests exercise the real recorder under a mocked read-only DCS environment.
The decoder tests check transport integrity, failure cleanup, no-overwrite
behavior, per-segment state evidence, missing states, raw text lengths, repeated
field names, and diagnostics surviving recovery. CI runs both suites, plus the
existing panel verifier tests and exporter lint.

## Full DCS-BIOS baseline crosswalk

[build_native_crosswalk.py](../Programs/tools/build_native_crosswalk.py) produces
a JSON crosswalk and a readable per-control checklist for **every control in
the pinned 505-entry inventory**, not just the 123-control native catalogue.
It reads local files only and never executes either DCS or DCS-BIOS Lua.

The installed DCS-BIOS reference checked on 2026-10-25 contains **503** controls
in 74 categories, SHA-256
`0d57cf41d2b8895f3fbce671ee420f705ca4338897885306cfefd821ef956d20`.
The pinned inventory additionally contains `EXT_LAUNCH_BAR` and
`HUD_ATC_NWS_ENGAGED`. Both are preserved and flagged for version reconciliation;
the installed reference is not silently treated as the original release baseline.

The crosswalk keeps three independent evidence layers:

1. **BIOS hypotheses:** explicit supported literal declarations provide candidate
   device/command/argument triples or indication fields. BIOS addresses are never
   used as native argument numbers. Custom helpers, dynamic identifiers, loops,
   and getters the parser cannot resolve remain unresolved. External aircraft
   draw arguments are kept distinct from device 0 cockpit arguments and are
   not linked to the cockpit capture just because the numbers match.
2. **Native static matches:** supported direct native clickable declarations
   resolve device and command symbols from the installed native files. An exact,
   unique device/command/argument triple is a static match only; it does not
   validate action values, helper semantics, cockpit identity, or actuation.
   Native point names such as `pnt_49` are not substituted for actual arguments.
   Complex native definitions are unresolved, not claimed missing.
3. **Captured observations:** all supplied complete sessions remain separate.
   Candidate argument min/max/transitions and indication lengths are recorded.
   Repeated session/sequence records from overlapping exports are deduplicated;
   conflicting records, sequence gaps, and incomplete sessions fail explicitly.
   Observed getter fields may still require BIOS padding/remapping semantics.

Every control retains `input_verified: false` and `identity_verified: false`,
with checklist items for unresolved mapping, missing state evidence, text-size
conflicts, native actuation where applicable, and output semantics.

Example, from the repository root (replace the capture/output paths):

```powershell
python Programs\tools\build_native_crosswalk.py `
  --bios-json "$env:USERPROFILE\Saved Games\DCS\Scripts\DCS-BIOS\doc\json\FA-18C_hornet.json" `
  --bios-module "$env:USERPROFILE\Saved Games\DCS\Scripts\DCS-BIOS\lib\modules\aircraft_modules\FA-18C_hornet.lua" `
  --dcs-root "E:\DCS World" `
  --captures "C:\captures\hornet.jsonl" `
  --output "C:\captures\hornet-crosswalk"
```

The `.txt` report contains the per-control checklist; `.json` retains mappings,
source locations, observations, and SHA-256 provenance for the baseline,
installed references/helpers, native files, and raw captures. Existing reports
are never overwritten. This process does not change catalogue entries.

Initial crosswalk against the broad sweep and targeted retest:

| Evidence classification | Controls |
|---|---|
| Unique native static triple match | 166 |
| BIOS input candidate not resolved in supported native syntax | 114 |
| BIOS output hypothesis only, without independent native mapping | 184 |
| Mapping unresolved | 41 |
| Candidate cockpit argument changed in at least one session | 364 |
| Candidate cockpit argument remained constant | 39 |
| Candidate indication field observed | 59 |
| Not linked to captured output | 43 |

The first four rows partition mapping status; the last four independently
partition output evidence. Do not add the two sets together or interpret a
candidate observation as a verified control. The four known IFEI length conflicts
are also detected against the full installed reference.

Run the crosswalk regression tests with:

```powershell
python -m unittest discover -s tests -p 'test_native_crosswalk.py'
```

### Follow-up: resolving the initial unlinked controls

The crosswalk now understands the explicit argument positions in BIOS rocker
switches, the Hornet-specific parking-brake and mission-computer helpers, and
the ejection-handle helper. Multi-command controls retain **all candidate
commands** and a `multi-command-review-required` mapping status; one direction
is never substituted for the full behavior. These remain BIOS hypotheses.
Direct indication getters and radio-frequency getters are classified separately.

Reprocessing the same complete captures (no new cockpit actions) links
**16 additional controls**: **14 changed** and **2 constant**. The constant
additions are the ejection handle and throttle dispense switch.
The native unique-triple count increases from 166 to **167** through the
ejection-handle mapping, but its command has not been actuated by these tools.

Updated independent output partition:

| Candidate output evidence | Controls |
|---|---|
| Cockpit argument changed | 378 |
| Cockpit argument constant | 41 |
| Indication field observed | 59 |
| Not linked to captured output | 27 |

The **27 remaining unlinked controls** have these concrete next steps:

| Group | Count | Next evidence needed |
|---|---|---|
| External aircraft model outputs | 16 | Capture `LoGetAircraftDrawArgumentValue` in its own namespace; existing cockpit scans cannot answer these |
| `COMM1`, `COMM2`, `COMM1_FREQ`, `COMM2_FREQ` | 4 | Read radio devices 38/39 `get_frequency()`; existing telemetry does not provide these specific readbacks |
| `COMM1_CHANNEL_NUMERIC`, `COMM2_CHANNEL_NUMERIC` | 2 | Independently check channel text decoding/cache rules against raw UFC display transitions |
| `UFC_COMM1_CHANNEL_SELECT`, `UFC_COMM2_CHANNEL_SELECT` | 2 | Correlate each rotation direction with the channel display; no direct argument readback is defined |
| `EXT_LAUNCH_BAR`, `HUD_ATC_NWS_ENGAGED` | 2 | Reconcile pinned-baseline entries absent from the installed BIOS reference before assigning mappings |
| `HUD_LTDR` | 1 | Candidate HUD indication field `MPD_FLIR_LaserStatus_label` was not observed; confirm native field and applicable display/mode conditions |

The external group covers formation/position/tail/strobe/probe lights, hook,
both nozzle positions, refuel probe, speed brake, boarding stair, wing fold,
and three weight-on-wheels outputs. Grounded and airborne scenarios are needed
for weight-on-wheels; a cockpit argument with the same number is not evidence.

Every crosswalk control now has a machine-readable `test_plan`, also printed
in the checklist. It reports the acquisition gap, a next step, and safety
classification. It never generates commands or assumes that a routine sweep
should exercise ejection, jettison, weapon release, circuit breakers, hydraulic
isolation, or other disruptive actions.

For the **41 constant candidates**, use separate conditions instead of repeating
the entire broad sweep:

- **Grounded, powered checks:** identified louver knobs, lighting controls,
  RWR brightness/special indication, radar-altimeter test/minimum-height setting,
  and gear-related controls only under appropriate interlocks.
- **Clock/timer run:** start the appropriate timer and wait for the indicated
  unit to change; a short test does not exercise elapsed minutes.
- **Airborne instrument scenario:** vary speed, altitude, and attitude for
  standby instruments and radar altitude. Ground-start zeros are insufficient.
- **HOTAS bindings:** test the identified stick/throttle bindings rather than
  assuming each action can be operated with a cockpit mouse click.
- **Restricted/disruptive items:** keep ejection/canopy-jettison, weapons,
  circuit-breaker, hydraulic-isolate, and similar checks in separately confirmed
  disposable scenarios; do not include them in an automatic input sweep.

These are test conditions, not claims that all indicated outputs should change
under all scenarios. Readback, system response, and native input actuation
remain distinct. No new live capture or command actuation was performed during
this parser/checklist follow-up.

## External-aircraft and radio follow-up recorder

The broad recorder now also includes these read-only streams, while retaining
compatibility with earlier captures:

- **External arguments:** `LoGetAircraftDrawArgumentValue` slots
  **1, 4, 6, 8, 21, 22, 25, 88, 89, 90, 190, 191, 192, 193, 212, 323**,
  sampled at up to **10 Hz**. This is the explicit set of 16 external candidates
  from the installed reference, not a scan of all external model slots.
- **Radio frequencies:** devices **38 and 39**, using only `get_frequency()` at
  **1 Hz**, preserved as raw **Hz**, including zero if returned. The report also
  shows MHz for readability without changing raw values. No frequency setter
  or clickable action is called.
- The start record lists both stream selections and rates. Numeric baselines,
  exact-repeat suppression, changed values, errors, and recovery are handled
  independently. An unchanged value is re-emitted after a failed read recovers.
  Missing/restricted APIs or radio devices report errors without inventing zeros
  or stopping the cockpit recorder.
- `external_arguments` / `external_argument_errors` and `radio_frequency`
  records have separate namespaces. External argument 25 can never be confused
  with cockpit argument 25. The decoder prints these separately in the timeline
  and summary and retains their own per-segment stats in the coverage report.
- The full crosswalk can now link `external_arg` and `radio_device` hypotheses
  to these new records. Radio observations remain raw Hz even where BIOS
  divides or formats them. It never treats a matched frequency as verification
  of BIOS formatting, channel selection, or command behavior.

These additions target the acquisition gaps for **20 baseline controls**;
successful readings or state changes must be established by a new live run.
Earlier sessions cannot retroactively supply these streams.

### Focused mission checklist

Use a powered, disposable single-player Hornet mission with no stores.
The recorder installation and conversion steps above are unchanged.

1. Wait five seconds for initial readings, then note the visible radio frequencies.
2. Select at least two different valid frequencies on **each radio**, waiting
   **three seconds** at each. Note both displayed values for comparison with Hz.
3. Turn **COMM1 and COMM2 channel selectors** several steps in both directions,
   waiting two seconds between steps. Note whether each channel display changes;
   display changes alone do not prove selector direction/action values.
4. Change exterior lighting controls, with several seconds at each setting.
   Native interlocks may prevent some lights from illuminating on the ground.
5. Where permitted by aircraft procedures/interlocks, cycle the **hook, refuel
   probe, speed brake, and wing fold**, allowing animations to settle. Do not
   bypass interlocks to force a change.
6. With engines running, vary throttle modestly if appropriate for the parked
   aircraft; nozzle positions may otherwise remain constant. Skip this if not
   appropriate. The boarding ladder may also remain unchanged in this mission.
7. End the mission, then preserve the log before restarting DCS.

This is not a requirement to operate unsafe controls or take off. A later
separate airborne scenario can supply weight-on-wheels transitions and varied
instrument conditions. Note skipped steps, actual visible frequencies, channel
display behavior, and any errors/performance impact when reporting completion.

### Ground follow-up evidence

The user completed a ground mission and initially did not recall the displayed
radio frequencies. After reviewing the captured sequences below, the user
confirmed both were correct. Complete session **`2026-10-25T16:46:09Z-1`** ended at model time
**204.447 seconds**, with **8,895 structured records**, **1,916 cockpit/external
scans**, **204 detail/radio scans**, **zero recorder errors**, and **7,857,922
exact-repeat samples suppressed**.

All **16 external candidates** were readable: **11 changed**, while five
remained at zero:

- `EXT_FORMATION_LIGHTS` (88).
- `EXT_POSITION_LIGHT_LEFT` (190).
- `EXT_POSITION_LIGHT_RIGHT` (191).
- `EXT_TAIL_LIGHT` (192).
- `EXT_STAIR` (323).

These are constant observations in this mission, not failed mappings.
Both radio devices returned changing frequencies, linking all four frequency
outputs. Raw distinct readings were:

- Device 38: **250000012, 256000028, 257000005, 262000028, 305000011,
  305000017 Hz**.
- Device 39: **256000019, 257000027, 259000010, 305000039 Hz**.

The few-Hz offsets are retained exactly as returned in raw evidence. For
operator comparison only, the sequences were presented at 0.001 MHz precision:

- **COMM1: 305.000 -> 256.000 -> 257.000 -> 262.000 -> 250.000 MHz**.
- **COMM2: 305.000 -> 256.000 -> 259.000 -> 257.000 MHz**.

The user explicitly confirmed **both sequences correct**, with no corrections.
This supplies operator-confirmed frequency readback correlation at displayed
precision, not exact-Hz calibration, automatic command actuation, BIOS encoding
equivalence, or channel-number decoding. COMM1's two slightly different initial
305 MHz samples were collapsed only in the operator-facing sequence.

The preserved log also contains both earlier completed sessions. Decoding
produced **40,123 complete records across three sessions**; the crosswalk merges
that evidence with the earlier retest, deduplicating shared session/sequence
identities. Updated output classification across all **505 controls**:

- **378** changed cockpit-argument candidates.
- **41** constant cockpit-argument candidates.
- **11** changed external-argument candidates.
- **5** constant external-argument candidates.
- **4** changing radio-frequency candidates.
- **59** observed indication-field candidates.
- **7** still unlinked: `COMM1_CHANNEL_NUMERIC`, `COMM2_CHANNEL_NUMERIC`,
  `UFC_COMM1_CHANNEL_SELECT`, `UFC_COMM2_CHANNEL_SELECT`, `EXT_LAUNCH_BAR`,
  `HUD_ATC_NWS_ENGAGED`, and `HUD_LTDR`.

The static mapping partition remains unchanged (167 unique native triple
matches). No command, input identity, expected state, or hardware verification
flag was promoted. Channel decoding/correlation and the two pinned-only
identifiers remain separate follow-ups; airborne comparison remains optional
future evidence, not something established by this ground run.

Session artifacts retain `external-radio-ground` raw JSONL, repeat-suppressed
timeline, coverage, summary, and transport, together with the updated
`hornet-505-external-radio-crosswalk` checklist and structured report.

### Channel decoding and native selector follow-up

Reprocessing the same captures now links the two numeric channel getters to
indication **6**, fields **`UFC_Comm1Display`** and **`UFC_Comm2Display`**. The
decoder reads the installed reference's explicit `comm_channel_map` instead of
executing its Lua or assuming every display string is a decimal number.

Native `UFC/indicator/UFC_main_page.lua` independently assigns these field names
to `CommChannel` controllers 1 and 2. Native `fonts.lua` identifies the backtick
glyph as the tens digit 1 and tilde as the tens digit 2. Thus raw `` `0 `` and
`` `1 `` are readable as **10** and **11**, not invalid decimal strings.
Special `G`, `M`, `C`, and `S` remain displayed as letters; their reference
numeric codes **21-24** are BIOS encoding candidates, not ordinary channel
numbers or independently verified native numeric output.

The latest ground session contains these candidate display sequences:

| Radio | Model times (seconds) | Decoded channel sequence |
|---|---|---|
| COMM1 | 0, 8.016, 30.079, 35.092 | **1 -> 4 -> 10 -> 6** |
| COMM2 | 0, 19.057, 39.109, 42.109 | **1 -> 4 -> 11 -> 8** |

The user explicitly confirmed **both channel sequences correct**, with no
corrections, after reviewing them. This supplies operator-confirmed channel
display correlation for these sampled states, not complete validation of all
24 reference codes or selector command direction/magnitude. Earlier captures
also contain additional numeric glyphs and `G`. Every decoded sample retains its
raw text and model time. Absent/blank fields, the `d` triangle glyph, and unknown
strings remain explicitly unresolved, never a fabricated channel zero. No
receiving-state cache is simulated, including across cockpit segments. This
does not claim complete equivalence to the reference getter, which uses a
cache and returns zero for unknown states.

Native `clickabledata.lua` uses `default_button_axis_tumb` for each combined
channel-selector/pull knob. Inspecting `clickable_defs.lua` confirms that the
first command/argument pair belongs to the button, while the second belongs to
rotation. The crosswalk now parses these components separately:

| Function | Device | Command | Cockpit argument |
|---|---|---|---|
| COMM1 rotation | 25 | 3033 | **124** |
| COMM2 rotation | 25 | 3034 | **126** |
| COMM1 pull | 25 | 3008 | **125** |
| COMM2 pull | 25 | 3009 | **127** |

In the ground session, argument **124** changed **12** times, spanning
**0..0.970001220703125**; argument **126** changed **10** times, spanning
**0..0.29998779296875**. These are raw knob argument observations, not direct
channel-number values. Related display changes are reported alongside the
selector evidence, not presented as proof of direction, per-step magnitude,
causality, or command actuation.

The full output partition now contains **380 changed cockpit-argument
candidates**, **41 constant cockpit-argument candidates**, **2 candidate-decoded
channel outputs**, **4 changing radio-frequency candidates**, **11 changed and
5 constant external candidates**, **59 observed indication-field candidates**,
and only **3 unlinked entries**:

- `EXT_LAUNCH_BAR` and `HUD_ATC_NWS_ENGAGED`: pinned baseline identifiers absent
  from the installed reference; require version/identity resolution.
- `HUD_LTDR`: the candidate laser-status indication field has not appeared.

Static mapping results now include **169 unique native triple matches** and
**2 native command candidates** for rotation (which had no BIOS argument).
All input/identity flags remain false. The earlier "no direct output mapping"
classification for the two selectors is superseded by this native helper
analysis, not by an assumption about unanimated controls.

Validation: **24 focused Python tests pass**, covering combined-helper pair
selection, literal decoding, unknown/receive/absent values, repeated fields,
session/segment isolation, duplicate evidence, namespace separation, and
preserved false verification flags. The regenerated
`hornet-505-channel-crosswalk` reports retain the complete 505-control
partition and source hashes. No new mission or temporary installation was
needed for this analysis.

### Resolving the pinned-only outputs and preparing the final acquisition test

Recovered the public
[v0.11.7 aircraft module](https://github.com/DCS-Skunkworks/dcs-bios/blob/v0.11.7/Scripts/DCS-BIOS/lib/modules/aircraft_modules/FA-18C_hornet.lua)
to compare the two missing identifiers with the installed module. Its SHA-256
is `7f5007d68af120499d7f710f2420996a7a332cb848f6b0fcfddc061cbf3218a9`.
The source is retained with session evidence as `FA-18C_hornet.v0.11.7.lua`.
The pinned definitions establish these hypotheses:

- `EXT_LAUNCH_BAR`: **external aircraft argument 85**, not cockpit switch
  argument 233. The previous recorder did not sample external 85.
- `HUD_ATC_NWS_ENGAGED`: **indication 1, `NWS_cue`**.

The crosswalk accepts optional `--baseline-module PATH`. It fills only
identifiers absent from the installed module, labels each definition as
`baseline-module` or `installed-module`, and hashes both input files. It does
not replace installed definitions, fabricate installed-reference presence, or
alter the pinned inventory. Use the existing full crosswalk command with:

```powershell
--baseline-module 'C:\path\to\FA-18C_hornet.v0.11.7.lua'
```

Native HUD declarations now accompany the two relevant indication hypotheses
in the reports, including source paths, lines, text candidates, and hashed
source files. Native `HUD_common.lua` confirms `NWS_cue` uses controller
`HUD_ATC_NWS_Cue`, with blank, `ATC`, `NWS`, and `NWS HI` text. Reprocessing
the captures links **`NWS`** in both the missed-controls retest and the ground
radio test. Other states are not yet observed; this is output linkage, not full
state verification. The installed-reference absence remains recorded.

The output partition is now **503 linked / 2 unlinked**, with **60 observed
indication-field candidates**; all other output counts from the channel
follow-up remain unchanged. The two unlinked entries are:

1. **`EXT_LAUNCH_BAR`**: candidate mapping recovered; external stream not yet
   acquired for argument 85.
2. **`HUD_LTDR`**: native field confirmed; field not yet observed.

The recorder now samples **17 external candidates**, adding **85** to the
earlier set of 16. All cockpit, radio, telemetry and display rates remain
unchanged. Native `HUD_AG.lua` declares `MPD_FLIR_LaserStatus_label` using
`HUD_FLIR_LaserStatus`. `Common_init.lua` includes that file in the HUD A/G
page. Possible labels include `L ARM`, `M ARM`, `LTD/R`, `MARK`, `MASK`, and
`LASER`. The native controller implementation/activation logic is not exposed
by these Lua declarations, so the exact sufficient conditions cannot yet be
claimed from source alone.

#### Focused acquisition procedure

Use a disposable single-player Hornet with a targeting pod, appropriate power,
and no weapons. This recorder is read-only; it never sends cockpit inputs.

1. With the aircraft parked and appropriate hydraulic power, hold the launch-bar
   switch in each normal position for at least three seconds. If permitted,
   visually inspect external bar movement. Note whether it actually moves;
   switch movement alone does not verify external argument 85. Do not bypass
   interlocks.
2. For optional additional NWS evidence, use normal low/high/disengaged states
   while stationary if permitted. Note the exact HUD cue; do not taxi merely
   for this test.
3. Power/warm the targeting pod normally and display its page. Select A/G HUD
   mode and observe any laser-status text with normal safe settings, holding
   each visible state for at least three seconds. Record the exact **HUD**
   text, or report that it remains absent.
4. **Do not fire the laser, press the trigger, release weapons, or defeat ground
   interlocks for this test.** If no status appears, stop and report the pod,
   power/mode, and visible messages. Another condition/scenario may be needed;
   acquisition is not presumed successful.
5. End the mission without restarting DCS until the log is preserved.

An identically named field exists on the FLIR MFD page with a different
controller. MFD observations must not be substituted for the HUD indication 1
field. The broad recorder can preserve both for diagnosis, in separate display
namespaces. No airborne test is required at this stage.

Validation: **26 focused Python tests** and the **Lua broad-recorder suite**
pass, including source precedence, recovered mappings, native HUD declarations,
and raw launch-bar samples. The session's
`hornet-505-pinned-output-crosswalk` reports preserve all 505 entries and leave
every identity/input verification flag false.

### Focused launch-bar/HUD ground capture results

The completed test used a warmed **ATFLIR**, a designated target, **A/G mode**,
and weight on wheels. The user confirmed that the external launch bar moved and
that NWS behaved as expected. The user reported no visible laser cues and
attributed the inability to arm the laser to weight on wheels; this is operator
scenario context, not independent verification of every laser interlock.

Complete session **`2026-10-25T17:08:10Z-1`** contains **10,775 records** and
ends at model time **260.884 seconds**, with **2,446 cockpit/external scans**,
**260 detail/radio scans**, **zero recorder errors**, **zero diagnostics**, and
**10,040,401 exact-repeat samples suppressed**.

- **External launch bar:** argument **85** changed **33** times, spanning
  **0..0.8675934672355652**, ending at **0**. Combined with the user's observed
  bar movement, this supplies candidate readback correlation, not proof of a
  full 0..1 range, every endpoint, exact position calibration, or input actuation.
- **HUD ATC/NWS:** indication **1**, `NWS_cue`, returned **`NWS`** and
  **`NWS HI`**. Blank and `ATC` have not been established by this run.
- **HUD laser status:** despite no cue being noticed visually, indication **1**
  did include `MPD_FLIR_LaserStatus_label`, with raw values **`MASK`** and
  **empty string**. The log includes **53** changed-indication snapshots
  containing `MASK` and **24** containing the empty value. These counts are
  whole-indication snapshots, not 53 laser-state transitions. This is actual
  HUD evidence, not substitution from an identically named MFD field.
  Visual confirmation of `MASK`, arming/firing statuses, and airborne
  conditions remain unverified.

**All 505 baseline entries now have candidate output observations linked**.
The merged partition is:

| Output evidence | Controls |
|---|---:|
| Changed cockpit-argument candidates | 381 |
| Constant cockpit-argument candidates | 40 |
| Candidate-decoded channel outputs | 2 |
| Changing radio-frequency candidates | 4 |
| Changed external-argument candidates | 12 |
| Constant external-argument candidates | 5 |
| Observed indication-field candidates | 61 |
| Unlinked | **0** |

This completes acquisition linkage, **not full verification**: constant outputs,
unobserved states, native input identity/actuation, formatting/ranges, and hardware
remain separate checklist items. All identity/input flags remain false and the
catalogue/generated files are unchanged. An airborne test is no longer required
just to link `HUD_LTDR`; it would test additional states instead.

The `launchbar-hud-ground` raw/transport/readable/coverage evidence and merged
`hornet-505-launchbar-hud-crosswalk` reports are retained in session artifacts.
The temporary recorder was removed and the previous exporter configuration
restored byte-for-byte from `Export.lua.before-launchbar-hud-test`.

## Handoff: next work after acquisition linkage

Current milestone: **505/505 candidate outputs linked, zero unlinked**.
This is not approval to mark catalogue controls verified. Resume with the merged
`hornet-505-launchbar-hud-crosswalk` checklist, not an earlier intermediate
report. Raw captures, reports, source snapshots, and installation backups are
local session evidence, not repository files. The recorder is currently removed.

Prioritized follow-up:

1. **Classify the 45 constant-output candidates** (40 cockpit, 5 external).
   Group by power/interlock, HOTAS binding, clock/timer, ground/airborne, and
   disruptive-control conditions. Distinguish legitimate constant outputs from
   missing test states. Do not request another indiscriminate cockpit sweep.
2. **Resolve output semantics before catalogue promotion.** Check text limits
   for `IFEI.TEMP_L`, `TEMP_R`, `FF_L`, and `FF_R`: raw `1000` has four characters,
   exceeding the existing three-character limit. Verify ranges/endpoints,
   lamp thresholds, special COMM channel codes and receive/blank handling.
   Preserve raw Hz offsets; the operator confirmed frequency/channel sequences
   at displayed precision, not exact-Hz calibration.
3. **Expand static native mapping conservatively.** Current partition:
   169 unique native triple matches, 110 native-unresolved, 188 BIOS-only
   hypotheses, 15 multi-command review cases, 21 unresolved, and 2 native
   command candidates. Support known native loops/custom helpers with regression
   tests rather than treating parser limits as proof that mappings are absent.
4. **Prepare bounded state tests only where needed.** Candidate topics include
   airborne instruments/weight-on-wheels, HUD `ATC`, and visually confirming
   laser-status `MASK` or other non-firing states. The ground ATFLIR test already
   linked the HUD laser field; flight is not needed to finish acquisition.
   Ask the operator to record exact before/after states and skipped actions.
5. **Verify input commands separately.** Start with one benign control,
   explicitly agree on permitted actuation, then correlate direction, step,
   press/release and system response. Current tools send no commands. Exclude
   ejection, jettison, weapons, crypto, circuit breakers and similar disruptive
   controls from routine automation.
6. **Promote only supported catalogue entries, then test hardware.** Record
   source/build and visual/state/command evidence per control; update canonical
   catalogue inputs and regenerate outputs instead of editing generated files.
   Firmware portability and physical I/O remain untested by these captures.

Before another live run: inspect the user's current exporter configuration,
make a fresh unique backup, install the tested recorder once, and preserve all
other exporters. Afterward preserve the log before a DCS restart, require a
complete session, decode/merge with provenance hashes, document operator
confirmation separately from raw observations, and restore the prior setup.

Relevant regression commands (repository root):

```powershell
$env:PYTHONDONTWRITEBYTECODE='1'
python -m unittest discover -s tests -p 'test_native_c*.py'
& 'E:\DCS World\bin\luae.exe' 'tests\test_native_capture.lua'
& 'E:\DCS World\bin\luae.exe' 'tests\test_native_verification.lua'
python Programs\tools\generate_hornet_catalog.py --check
git --no-pager diff --check
```

The bundled interpreter path is specific to the test installation; CI uses
`lua5.1` for both Lua suites and additionally runs `luacheck .` in the exporter
directory. Python tests use only the standard library.
