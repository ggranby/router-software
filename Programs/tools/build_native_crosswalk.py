#!/usr/bin/env python3
"""Cross-reference the pinned inventory, installed BIOS declarations, native Lua and captures.

No Lua is executed. Only explicit supported literal declarations are interpreted.
BIOS mappings remain hypotheses even when their candidate arguments changed.
"""

import argparse
import hashlib
import json
import re
import sys
from collections import Counter
from pathlib import Path

from decode_native_capture import ROOT, indication_fields, load_json, validate_record


def uncomment(text):
    pattern = r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|--\[\[.*?\]\]|--[^\n]*'
    return re.sub(pattern, lambda m: m[0] if not m[0].startswith("--")
                  else "\n" * m[0].count("\n"), text, flags=re.S)


def inventory(text):
    entries = {}
    group = None
    for line in text.splitlines():
        if line.startswith("### "):
            group = line[4:]
        match = re.match(r"^\| ([A-Z][A-Z0-9_]+) \| (.*)", line)
        if match:
            identifier = match[1]
            if identifier in entries:
                raise ValueError(f"Duplicate baseline control: {identifier}")
            cells = re.split(r"(?<!\\)\|", line)[1:-1]
            entries[identifier] = {
                "category": group, "description": cells[1].strip(),
                "has_input": "**Interactable**" in cells[2],
            }
    if not entries:
        raise ValueError("Pinned inventory contains no control rows")
    return entries


def native_mappings(scripts):
    device_text = uncomment((scripts / "devices.lua").read_text(encoding="utf-8-sig"))
    devices = {}
    # Reject expressions or unexpected counter usage instead of guessing their values.
    assignments = re.findall(r'devices\["(\w+)"\]\s*=\s*counter\(\)', device_text)
    if device_text.count("counter()") != len(assignments) + 1:
        raise ValueError("Unsupported native device counter layout")
    for index, name in enumerate(assignments, 1):
        devices[name] = index
    command_text = uncomment((scripts / "command_defs.lua").read_text(encoding="utf-8-sig"))
    start = re.search(r"^start_command\s*=\s*(\d+)\s*$", command_text, re.M)
    if not start:
        raise ValueError("Native command start value is not a literal")
    commands = {}
    for match in re.finditer(
        r"count\s*=\s*start_command\s+(\w+)\s*=\s*\{([^}]+)\}", command_text, re.S
    ):
        table, body = match.groups()
        fields = re.findall(r"(\w+)\s*=\s*counter\(\)\s*;", body)
        remainder = re.sub(r"\w+\s*=\s*counter\(\)\s*;", "", body).strip()
        if remainder:
            continue
        for offset, field in enumerate(fields, 1):
            commands[f"{table}.{field}"] = int(start[1]) + offset
    text = uncomment((scripts / "clickabledata.lua").read_text(encoding="utf-8-sig"))
    # These helpers take device, command, argument in this order. Complex/custom
    # elements are deliberately left unresolved; pnt_N is never used as an argument.
    helpers = {
        "default_button", "short_way_button", "default_button2",
        "default_2_position_tumb", "default_3_position_tumb", "default_axis_limited",
        "default_axis", "default_red_cover", "default_cover", "multiposition_switch",
        "push_button_tumb", "AMPCD_switch_positive", "AMPCD_switch_negative",
    }
    result = []
    expression = re.compile(
        r'elements\["([^"]+)"\]\s*=\s*(\w+)\(\s*_\("((?:\\.|[^"\\])*)"\)\s*,'
        r"\s*devices\.(\w+)\s*,\s*(\w+\.\w+)\s*,\s*(\d+)\s*[,)]"
    )
    for match in expression.finditer(text):
        point, helper, label, device, command, arg = match.groups()
        if helper not in helpers or device not in devices or command not in commands:
            continue
        result.append({
            "point": point, "helper": helper, "label": label,
            "device": devices[device], "command": commands[command], "arg": int(arg),
            "device_symbol": device, "command_symbol": command,
            "line": text[:match.start()].count("\n") + 1,
        })
    combined = re.compile(
        r'elements\["([^"]+)"\]\s*=\s*default_button_axis_tumb\(\s*_\("((?:\\.|[^"\\])*)"\)\s*,'
        r"\s*devices\.(\w+)\s*,\s*(\w+\.\w+)\s*,\s*(\w+\.\w+)\s*,"
        r"\s*(\d+)\s*,\s*(\d+)\s*,"
    )
    for match in combined.finditer(text):
        point, label, device, button, axis, button_arg, axis_arg = match.groups()
        if device not in devices or button not in commands or axis not in commands:
            continue
        for component, command, arg in (("button", button, button_arg), ("axis", axis, axis_arg)):
            result.append({
                "point": point, "helper": "default_button_axis_tumb", "component": component,
                "label": label, "device": devices[device], "command": commands[command],
                "arg": int(arg), "device_symbol": device, "command_symbol": command,
                "line": text[:match.start()].count("\n") + 1,
            })
    return result


def bios_candidates(text):
    text = uncomment(text)
    result = {}
    displays = {"ufc": 6, "ifei": 5}
    if re.search(r"\bhud\s*=\s*Module\.parse_indication\(1\)\s+or\s+\{\}", text):
        displays["hud"] = 1
    standard = {
        "definePushButton", "defineToggleSwitch", "defineToggleSwitchToggleOnly",
        "define3PosTumb", "definePotentiometer", "defineTumb", "defineRotary",
        "defineEjectionHandleSwitch",
    }
    outputs = {"defineIndicatorLight", "defineIndicatorLightInverted", "defineFloat"}
    for match in re.finditer(r'FA_18C_hornet:(define\w+)\("([^"]+)"\s*,([^\n]*)', text):
        helper, identifier, tail = match.groups()
        item = {"helper": helper, "line": text[:match.start()].count("\n") + 1}
        if helper in standard:
            numbers = re.match(r"\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*[,)]", tail)
            if numbers:
                item.update(zip(("device", "command", "arg"), map(int, numbers.groups())))
        elif helper in outputs:
            number = re.match(r"\s*(\d+)\s*[,)]", tail)
            if number:
                item["arg"] = int(number[1])
        elif helper == "defineFixedStepInput":
            numbers = re.match(r"\s*(\d+)\s*,\s*(\d+)\s*,", tail)
            if numbers:
                item.update(zip(("device", "command"), map(int, numbers.groups())))
        elif helper in {"defineFloatFromDrawArgument", "defineBitFromDrawArgument"}:
            number = re.match(r"\s*(\d+)\s*[,)]", tail)
            if number:
                item["external_arg"] = int(number[1])
        elif helper == "defineRockerSwitch":
            numbers = re.match(r"\s*" + r"(\d+)\s*,\s*" * 6, tail)
            if numbers:
                device, positive, positive_stop, negative, negative_stop, arg = map(int, numbers.groups())
                item.update(device=device, commands=[positive, negative],
                            stop_commands=[positive_stop, negative_stop], arg=arg)
        elif helper in {"defineEmergencyParkingBrake", "defineMissionComputerSwitch"}:
            numbers = re.match(r"\s*" + r"(\d+)\s*,\s*" * 4, tail)
            if numbers:
                device, first, second, arg = map(int, numbers.groups())
                item.update(device=device, commands=[first, second], arg=arg)
        elif helper == "defineReadWriteRadio":
            number = re.match(r"\s*(\d+)\s*,", tail)
            if number:
                item.update(radio_device=int(number[1]), required_api="get_frequency")
        # Custom getters and loop-defined controls need manual resolution.
        if identifier in result:
            raise ValueError(f"Duplicate literal BIOS definition: {identifier}")
        result[identifier] = item
    for match in re.finditer(
        r'FA_18C_hornet:defineString\("([^"]+)",\s*function\(\)(.*?)\nend,', text, re.S
    ):
        identifier, body = match.groups()
        fields = set(re.findall(r"\b(ufc|ifei|hud)\.(\w+)", body))
        if len(fields) == 1 and identifier in result:
            variable, field = next(iter(fields))
            if variable in displays:
                result[identifier].update(display=displays[variable], field=field,
                                          transformed_getter=True)
        direct = re.findall(r'Module\.parse_indication\((\d+)\)\["([^"]+)"\]', body)
        if len(direct) == 1 and not fields and identifier in result:
            display, field = direct[0]
            result[identifier].update(display=int(display), field=field, transformed_getter=True)
    for match in re.finditer(
        r'FA_18C_hornet:defineIntegerFromGetter\("([^"]+)",\s*function\(\)(.*?)\nend,',
        text, re.S,
    ):
        identifier, body = match.groups()
        frequency = re.fullmatch(r"\s*return GetDevice\((\d+)\):get_frequency\(\) / (\d+)\s*", body)
        if frequency and identifier in result:
            result[identifier].update(radio_device=int(frequency[1]),
                                      required_api="get_frequency", divisor=int(frequency[2]))
        channel = re.fullmatch(r"\s*return get_comm_channel\(([12])\)\s*", body)
        if channel and identifier in result:
            result[identifier].update(
                display=6, field=f"UFC_Comm{channel[1]}Display", transformed_getter=True,
                channel_decoder="installed-bios-literal-map",
            )
    channel_map = re.search(r"\blocal comm_channel_map\s*=\s*\{([^{}]*)\}", text, re.S)
    if channel_map:
        entries = re.findall(r'\["([^"\\]*)"\]\s*=\s*(\d+)\s*,?', channel_map[1])
        remainder = re.sub(r'\["([^"\\]*)"\]\s*=\s*(\d+)\s*,?', "", channel_map[1]).strip()
        if remainder or len(dict(entries)) != len(entries):
            raise ValueError("Unsupported or duplicate COMM channel map entries")
        for item in result.values():
            if "channel_decoder" in item:
                item["channel_map"] = {key: int(value) for key, value in entries}
    return result


def merge_baseline_candidates(installed, baseline):
    result = {key: dict(value, definition_source="installed-module")
              for key, value in installed.items()}
    for key, value in baseline.items():
        if key not in result:
            result[key] = dict(value, definition_source="baseline-module")
    return result


def native_hud_outputs(scripts):
    pages = scripts / "Multipurpose_Display_Group" / "HUD_AVQ32" / "indicator" / "Pages"
    result = {}
    for filename in ("HUD_common.lua", "HUD_AG.lua"):
        path = pages / filename
        text = uncomment(path.read_text(encoding="utf-8-sig"))
        pattern = (
            r'addStrokeText\("(NWS_cue|MPD_FLIR_LaserStatus_label)"[^\n]*'
            r'\{\{"(HUD_ATC_NWS_Cue|HUD_FLIR_LaserStatus)"\}\},\s*\{([^{}]*)\}\)'
        )
        for match in re.finditer(pattern, text):
            field, controller, states = match.groups()
            if field in result:
                raise ValueError(f"Duplicate native HUD field declaration: {field}")
            result[field] = {
                "controller": controller,
                "text_candidates": re.findall(r'"([^"\\]*)"', states),
                "path": str(path), "line": text[:match.start()].count("\n") + 1,
                "warning": "Static display declaration, not proof of controller activation conditions.",
            }
    return result


def channel_evidence(samples, mapping):
    sequence = []
    for sample in samples:
        raw = sample["raw"]
        numeric = mapping.get(raw)
        item = dict(sample, numeric_candidate=numeric,
                    state="mapped" if numeric is not None else
                    "absent" if raw is None else
                    "receive-symbol" if raw == "d" else
                    "blank" if raw == "" else "unmapped")
        item["display_candidate"] = (str(numeric) if numeric is not None and numeric <= 20
                                     else raw if numeric is not None else None)
        if not sequence or sequence[-1]["raw"] != raw:
            sequence.append(item)
    return {
        "decoder_source": "installed BIOS literal map; not independently verified",
        "sequence": sequence,
        "numeric_candidates": sorted({item["numeric_candidate"] for item in sequence
                                      if item["numeric_candidate"] is not None}),
        "unresolved_raw_values": sorted({item["raw"] for item in sequence
                                        if item["numeric_candidate"] is None},
                                       key=lambda raw: (raw is not None, raw or "")),
        "cache_policy": "No carry-forward or fabricated zero for absent, blank, receive, or unknown glyphs.",
    }


def observations(paths):
    segments = {}
    identities = {}
    for path in paths:
        with path.open(encoding="utf-8") as stream:
            for line_number, line in enumerate(stream, 1):
                record = load_json(line)
                validate_record(record)
                identity = (record["session"], record["seq"])
                canonical = json.dumps(record, sort_keys=True)
                if identity in identities:
                    if identities[identity] != canonical:
                        raise ValueError(f"Conflicting capture identity at {path}:{line_number}")
                    continue
                identities[identity] = canonical
                key = (record["session"], record["segment"])
                state = segments.setdefault(key, {
                    "arguments": {}, "fields": {}, "external_arguments": {}, "radio_frequencies": {},
                })
                kind = record["kind"]
                if kind in {"arguments", "external_arguments", "radio_frequency"}:
                    namespace = "radio_frequencies" if kind == "radio_frequency" else kind
                    samples = {record["key"]: record["value"]} if kind == "radio_frequency" else record["value"]
                    for arg, value in samples.items():
                        stats = state[namespace].setdefault(arg, {
                            "min": value, "max": value, "last": value, "transitions": 0,
                        })
                        stats["transitions"] += int(stats["last"] != value)
                        stats.update(min=min(stats["min"], value), max=max(stats["max"], value), last=value)
                elif record["kind"] == "indication":
                    fields = indication_fields(record["value"])
                    for field, values in fields.items():
                        stats = state["fields"].setdefault((record["key"], field), {"max_length": 0})
                        stats["max_length"] = max(stats["max_length"], *(len(v) for v in values))
                        if record["key"] == "1" and field in {"NWS_cue", "MPD_FLIR_LaserStatus_label"}:
                            stats["raw_values"] = sorted(set(stats.get("raw_values", [])) | set(values))
                    if record["key"] == "6":
                        for field in ("UFC_Comm1Display", "UFC_Comm2Display"):
                            stats = state["fields"].get(("6", field))
                            if stats is None:
                                continue
                            values = fields.get(field, [])
                            if len(values) > 1:
                                raise ValueError(f"Ambiguous repeated COMM channel field: {field}")
                            raw = values[0] if values else None
                            samples = stats.setdefault("channel_samples", [])
                            if not samples or samples[-1]["raw"] != raw:
                                samples.append({"time": record.get("time"), "raw": raw})
                elif record["kind"] == "fatal":
                    raise ValueError(f"Capture contains fatal error: {record['value']}")
    sessions = {}
    for session, seq in identities:
        sessions.setdefault(session, []).append(seq)
    for session, sequences in sessions.items():
        if sorted(sequences) != list(range(1, max(sequences) + 1)):
            raise ValueError(f"Capture sequence gap: {session}")
        first = json.loads(identities[(session, 1)])
        last = json.loads(identities[(session, max(sequences))])
        if first["kind"] != "start" or last["kind"] != "stop":
            raise ValueError(f"Incomplete capture session: {session}")
    if not identities:
        raise ValueError("No capture observations supplied")
    return segments


def build(baseline, installed, candidates, native, segments, hud_outputs=None):
    installed = {key: value for entries in installed.values() for key, value in entries.items()}
    rows = []
    for identifier, reference in baseline.items():
        hypothesis = candidates.get(identifier, {})
        row = dict(identifier=identifier, **reference)
        row.update(installed_reference_present=identifier in installed,
                   bios_hypothesis=hypothesis, native_candidates=[], observations=[],
                   input_verified=False, identity_verified=False, checklist=[])
        if hypothesis.get("display") == 1 and hypothesis.get("field") in (hud_outputs or {}):
            row["native_indication_declaration"] = hud_outputs[hypothesis["field"]]
        if identifier not in installed:
            row["checklist"].append("Resolve baseline/installed reference version difference")
        if "device" in hypothesis and "command" in hypothesis:
            row["native_candidates"] = [
                item for item in native if item["device"] == hypothesis["device"]
                and item["command"] == hypothesis["command"]
            ]
            matches = row["native_candidates"]
            if "arg" in hypothesis:
                exact = [m for m in matches if m["arg"] == hypothesis["arg"]]
                row["mapping_status"] = (
                    "unique-native-triple-match" if len(exact) == 1 and len(matches) == 1
                    else "ambiguous-native-match" if exact else
                    "native-argument-conflict" if matches else "native-unresolved"
                )
            else:
                row["mapping_status"] = "native-command-candidate" if matches else "native-unresolved"
                if len(matches) == 1:
                    row["native_output_argument"] = matches[0]["arg"]
        else:
            row["mapping_status"] = "bios-hypothesis-only" if "arg" in hypothesis or "field" in hypothesis else "unresolved"
        if "commands" in hypothesis:
            row["native_candidates"] = [
                item for item in native if item["device"] == hypothesis["device"]
                and item["command"] in hypothesis["commands"]
            ]
            row["mapping_status"] = "multi-command-review-required"
        for (session, segment), state in segments.items():
            observation = None
            if "arg" in hypothesis:
                observation = state["arguments"].get(str(hypothesis["arg"]))
            elif "native_output_argument" in row:
                observation = state["arguments"].get(str(row["native_output_argument"]))
            elif "field" in hypothesis:
                observation = state["fields"].get((str(hypothesis["display"]), hypothesis["field"]))
            elif "external_arg" in hypothesis:
                observation = state.get("external_arguments", {}).get(str(hypothesis["external_arg"]))
            elif "radio_device" in hypothesis:
                observation = state.get("radio_frequencies", {}).get(str(hypothesis["radio_device"]))
            if observation is not None:
                evidence = dict(session=session, segment=segment, **observation)
                if "channel_decoder" in hypothesis:
                    evidence["channel_decode"] = channel_evidence(
                        observation.get("channel_samples", []), hypothesis.get("channel_map", {})
                    )
                row["observations"].append(evidence)
        stream = ("external-argument" if "external_arg" in hypothesis else
                  "radio-frequency" if "radio_device" in hypothesis else "argument")
        row["output_namespace"] = stream if "field" not in hypothesis else "indication"
        if any(o.get("transitions", 0) for o in row["observations"]):
            row["output_status"] = f"candidate-{stream}-changed"
        elif row["observations"] and "field" in hypothesis:
            decoded = any(o.get("channel_decode", {}).get("numeric_candidates")
                          for o in row["observations"])
            row["output_status"] = "candidate-channel-decoded" if decoded else "candidate-field-observed"
        elif row["observations"]:
            row["output_status"] = f"candidate-{stream}-constant"
        else:
            row["output_status"] = "not-linked-to-capture"
        if row["mapping_status"] != "unique-native-triple-match":
            row["checklist"].append("Resolve native mapping / identity independently")
        if row["output_status"].endswith("-constant") or row["output_status"] == "not-linked-to-capture":
            row["checklist"].append("Capture identified output in relevant states")
        if reference["has_input"]:
            row["checklist"].append("Test native command action and resulting system state")
        row["checklist"].append("Verify output semantics, ranges/formatting and visual identity")
        row["test_plan"] = test_plan(identifier, reference, hypothesis, row["output_status"])
        if "channel_decoder" in hypothesis:
            row["checklist"].append(
                "Confirm channel glyph decoding visually; blank/receive/unknown values are unresolved, not zero"
            )
        if identifier in {"UFC_COMM1_CHANNEL_SELECT", "UFC_COMM2_CHANNEL_SELECT"}:
            radio = 1 if identifier == "UFC_COMM1_CHANNEL_SELECT" else 2
            row["related_channel_feedback"] = [
                dict(session=session, segment=segment,
                     samples=state["fields"][("6", f"UFC_Comm{radio}Display")].get("channel_samples", []))
                for (session, segment), state in segments.items()
                if ("6", f"UFC_Comm{radio}Display") in state["fields"]
            ]
            row["checklist"].append(
                "Correlate one manual selector step/direction at a time with channel feedback; "
                "co-occurring changes alone do not verify causality or action magnitude"
            )
        lengths = [o["max_length"] for o in row["observations"] if "max_length" in o]
        limits = [output["max_length"] for output in installed.get(identifier, {}).get("outputs", [])
                  if "max_length" in output]
        if lengths and limits and max(lengths) > min(limits):
            row["text_length_conflict"] = {"raw_max": max(lengths), "reference_max": min(limits)}
            row["checklist"].insert(0, "Resolve raw indication/reference text-length mismatch")
        rows.append(row)
    return {
        "warning": "Candidate crosswalk only; BIOS metadata is not native proof. No flags changed.",
        "baseline_count": len(baseline), "installed_count": len(installed),
        "installed_only": sorted(set(installed) - set(baseline)),
        "mapping_counts": dict(Counter(r["mapping_status"] for r in rows)),
        "output_counts": dict(Counter(r["output_status"] for r in rows)), "controls": rows,
    }


def test_plan(identifier, reference, hypothesis, status):
    plan = {
        "automatic_actuation": False,
        "safety": "Manual review before operating; no commands are generated.",
    }
    if "external_arg" in hypothesis and status == "not-linked-to-capture":
        plan.update(
            gap="external-aircraft-stream-not-captured",
            next_step=f"Capture LoGetAircraftDrawArgumentValue({hypothesis['external_arg']}); "
                      "do not substitute cockpit device 0.",
        )
    elif "radio_device" in hypothesis and status == "not-linked-to-capture":
        plan.update(
            gap="radio-device-frequency-not-captured",
            next_step=f"Read GetDevice({hypothesis['radio_device']}):get_frequency() in a "
                      "separately approved read-only recorder; change frequency manually.",
        )
    elif status == "not-linked-to-capture" and "field" in hypothesis:
        plan.update(
            gap="indication-field-not-observed",
            next_step=f"Confirm native indication {hypothesis['display']} field "
                      f"{hypothesis['field']} and the scenario that makes it appear.",
        )
    elif status == "not-linked-to-capture":
        plan.update(gap="mapping-or-getter-unresolved",
                    next_step="Resolve getter/native mapping before requesting another cockpit sweep.")
    elif status.endswith("-constant"):
        category = reference.get("category", "")
        if category in {"Stick", "Throttle Quadrant"}:
            step = "Exercise the identified HOTAS binding, holding each state; verify cockpit/system response."
        elif "Clock" in category:
            step = "Run the appropriate clock/timer long enough for this unit to change; include start/stop/reset."
        elif any(word in category for word in ("Altimeter", "Airspeed", "Radar Alt", "Attitude")):
            step = "Use a separate airborne scenario with varied altitude/speed/attitude and applicable instrument tests."
        else:
            step = "Identify this control visually; capture its distinct states under appropriate power/interlocks."
        plan.update(gap="state-not-observed", next_step=step)
    else:
        plan.update(gap="semantics-and-actuation-not-verified",
                    next_step="Confirm visual identity, states/ranges, and native action semantics independently.")
    if any(word in identifier for word in (
        "EJECT", "JETT", "TRIGGER", "WEAP_REL", "CRYPTO", "MC_SW", "HYD_ISOLATE", "CB_",
    )):
        plan["safety"] = "Exclude from routine sweeps; explicit confirmation in a disposable mission is required."
    if "WOW" in identifier:
        plan["next_step"] += " Compare grounded and airborne states."
    return plan


def write_text(report, stream):
    stream.write("FULL HORNET NATIVE CROSSWALK - CANDIDATES, NOT VERIFIED CONTROLS\n\n")
    stream.write(f"Pinned baseline: {report['baseline_count']}; installed reference: {report['installed_count']}\n")
    stream.write("Mappings from BIOS remain hypotheses. Native triples are static checks, not actuation.\n")
    stream.write("Constant/changed arguments do not prove identity. Unsupported syntax is unresolved.\n")
    for name in ("mapping_counts", "output_counts"):
        stream.write(f"\n{name}: {json.dumps(report[name], sort_keys=True)}\n")
    stream.write("\nPER-CONTROL CHECKLIST\n")
    for row in report["controls"]:
        stream.write(f"\n{row['category']} / {row['identifier']}\n")
        stream.write(f"  {row['mapping_status']}; {row['output_status']}\n")
        stream.write(f"  BIOS hypothesis: {json.dumps(row['bios_hypothesis'], sort_keys=True)}\n")
        if "native_indication_declaration" in row:
            stream.write(f"  Native indication: {json.dumps(row['native_indication_declaration'], sort_keys=True)}\n")
        if "native_output_argument" in row:
            stream.write(f"  Native helper output argument candidate: {row['native_output_argument']}\n")
        if "related_channel_feedback" in row:
            stream.write(f"  Related channel feedback (not causality): "
                         f"{json.dumps(row['related_channel_feedback'], sort_keys=True)}\n")
        stream.write(f"  Test plan: {json.dumps(row['test_plan'], sort_keys=True)}\n")
        for item in row["native_candidates"]:
            stream.write(f"  Native: device {item['device']}, command {item['command']}, "
                         f"arg {item['arg']}, {item['label']} (clickabledata.lua:{item['line']})\n")
        for item in row["observations"]:
            stream.write(f"  Capture: {json.dumps(item, sort_keys=True)}\n")
        for todo in row["checklist"]:
            stream.write(f"  [ ] {todo}\n")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bios-json", type=Path, required=True)
    parser.add_argument("--bios-module", type=Path, required=True)
    parser.add_argument("--baseline-module", type=Path,
                        help="Optional pinned-release Lua module; fills missing installed identifiers only")
    parser.add_argument("--dcs-root", type=Path, required=True)
    parser.add_argument("--captures", type=Path, nargs="+", required=True)
    parser.add_argument("--baseline", type=Path, default=ROOT / "docs" / "F18C_EXPORT_INVENTORY.md")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(argv)
    created = []
    try:
        scripts = args.dcs_root / "Mods" / "aircraft" / "FA-18C" / "Cockpit" / "Scripts"
        sources = [args.baseline, args.bios_json, args.bios_module,
                   args.bios_module.parent.parent / "Module.lua",
                   scripts / "devices.lua", scripts / "command_defs.lua",
                   scripts / "clickabledata.lua", scripts / "clickable_defs.lua", *args.captures]
        hud_outputs = native_hud_outputs(scripts)
        sources.extend(dict.fromkeys(Path(item["path"]) for item in hud_outputs.values()))
        candidates = bios_candidates(args.bios_module.read_text(encoding="utf-8"))
        if args.baseline_module:
            sources.append(args.baseline_module)
            candidates = merge_baseline_candidates(
                candidates, bios_candidates(args.baseline_module.read_text(encoding="utf-8"))
            )
        else:
            candidates = merge_baseline_candidates(candidates, {})
        report = build(
            inventory(args.baseline.read_text(encoding="utf-8")),
            load_json(args.bios_json.read_text(encoding="utf-8")),
            candidates, native_mappings(scripts), observations(args.captures), hud_outputs,
        )
        report["provenance"] = [
            {"path": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
            for path in sources
        ]
        outputs = [Path(str(args.output) + suffix) for suffix in (".json", ".txt")]
        for path in outputs:
            if path.exists():
                raise FileExistsError(f"Refusing to overwrite {path}")
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with outputs[0].open("x", encoding="utf-8") as stream:
            created.append(outputs[0])
            json.dump(report, stream, indent=2, allow_nan=False)
            stream.write("\n")
        with outputs[1].open("x", encoding="utf-8") as stream:
            created.append(outputs[1])
            write_text(report, stream)
    except (OSError, ValueError, KeyError, TypeError) as error:
        for path in created:
            path.unlink(missing_ok=True)
        print(f"Crosswalk failed: {error}", file=sys.stderr)
        return 1
    print(f"Crosswalk contains {len(report['controls'])} controls.")
    print("Mappings:", report["mapping_counts"])
    print("Outputs:", report["output_counts"])
    for path in outputs:
        print(path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
