#!/usr/bin/env python3
"""Decode HornetLinkCapture records into raw JSONL, a timeline, and evidence coverage."""

import argparse
import hashlib
import json
import math
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MARKER = re.compile(r"HornetLinkCapture.*?: HLNC (\S+) (\d+) (\d+)/(\d+) (.*)$")
KINDS = {
    "start", "active", "inactive", "arguments", "argument_errors", "indication",
    "telemetry", "status", "heartbeat", "fatal", "stop",
    "external_arguments", "external_argument_errors", "radio_frequency",
}


def reject_constant(value):
    raise ValueError(f"Non-finite JSON number: {value}")


def load_json(text):
    return json.loads(text, parse_constant=reject_constant)


def decode(paths):
    """Yield complete records, rejecting missing, reordered, or truncated chunks."""
    pending = None
    chunks = []
    expected = {}
    stopped = set()
    found = False
    for path in paths:
        with path.open(encoding="utf-8", errors="strict") as stream:
            for number, line in enumerate(stream, 1):
                match = MARKER.search(line.rstrip("\r\n"))
                if not match:
                    if "HornetLinkCapture" in line and "HLNC " in line:
                        raise ValueError(f"{path}:{number}: malformed capture transport")
                    continue
                session, seq, part, total, payload = match.groups()
                seq, part, total = int(seq), int(part), int(total)
                if not 1 <= part <= total:
                    raise ValueError(f"{path}:{number}: invalid chunk number")
                identity = (session, seq, total)
                if pending is None:
                    if part != 1:
                        raise ValueError(f"{path}:{number}: missing first chunk")
                    pending, chunks = identity, []
                if pending != identity or part != len(chunks) + 1:
                    raise ValueError(f"{path}:{number}: missing/reordered/duplicate chunk")
                chunks.append(payload)
                if part != total:
                    continue
                record = load_json("".join(chunks))
                pending = None
                if not isinstance(record, dict):
                    raise ValueError("Capture record must be an object")
                if record.get("session") != session or record.get("seq") != seq:
                    raise ValueError("Transport/payload identity mismatch")
                if seq != expected.get(session, 1):
                    raise ValueError(f"{session}: expected sequence {expected.get(session, 1)}, got {seq}")
                validate_record(record)
                if seq == 1 and record["kind"] != "start":
                    raise ValueError("Session does not begin with a start record")
                if session in stopped:
                    raise ValueError("Records encountered after session stop")
                expected[session] = seq + 1
                if record["kind"] == "stop":
                    stopped.add(session)
                found = True
                yield record
    if pending:
        raise ValueError("Capture ends in an incomplete chunked record")
    if not found:
        raise ValueError("No HornetLinkCapture records found")
    incomplete = set(expected) - stopped
    if incomplete:
        raise ValueError(f"Session has no stop record (still running, crashed, or truncated): {sorted(incomplete)}")


def validate_record(record):
    if record.get("schema") != 1 or record.get("kind") not in KINDS:
        raise ValueError("Unsupported capture schema or record kind")
    if not isinstance(record.get("key"), str):
        raise ValueError("Record key must be a string")
    segment = record.get("segment")
    if type(segment) is not int or segment < 0:
        raise ValueError("Invalid cockpit segment")
    time = record.get("time")
    if time is not None and (type(time) not in (int, float) or not math.isfinite(time)):
        raise ValueError("Invalid model time")
    kind, value = record["kind"], record.get("value")
    if kind in {"arguments", "argument_errors", "external_arguments", "external_argument_errors"}:
        if not isinstance(value, dict):
            raise ValueError("Argument data must be an object")
        for key, item in value.items():
            if not key.isdecimal():
                raise ValueError("Argument key must be a non-negative integer")
            if kind in {"arguments", "external_arguments"} and (
                type(item) not in (int, float) or not math.isfinite(item)
            ):
                raise ValueError("Invalid argument sample")
            if kind in {"argument_errors", "external_argument_errors"} and not isinstance(item, str):
                raise ValueError("Argument error must be a string")
    if kind == "radio_frequency":
        if (record["key"] not in {"38", "39"} or type(value) not in (int, float)
                or not math.isfinite(value) or value < 0):
            raise ValueError("Invalid radio frequency sample (expected device 38/39, non-negative Hz)")
    if kind in {"indication", "telemetry", "status", "fatal"} and not isinstance(value, str):
        raise ValueError(f"{kind} value must be a string")
    if kind == "telemetry":
        if not isinstance(load_json(value), dict):
            raise ValueError("Telemetry must contain a packed result object")


def native_labels(dcs_root):
    """Read only direct pnt entries; these are labels, not inferred command mappings."""
    if dcs_root is None:
        return {}, {}
    path = dcs_root / "Mods" / "aircraft" / "FA-18C" / "Cockpit" / "Scripts" / "clickabledata.lua"
    raw = path.read_bytes()
    labels = {}
    for line in raw.decode("utf-8-sig").splitlines():
        if line.lstrip().startswith("--"):
            continue
        match = re.search(r'elements\["pnt_(\d+)"\].*?_\("([^"]+)"\)', line)
        if match:
            labels[match[1]] = match[2]
    return labels, {
        "native_label_file": str(path),
        "sha256": hashlib.sha256(raw).hexdigest(),
        "direct_point_labels": len(labels),
        "note": "Point names are labeling hints, not proof of argument or input mappings.",
    }


def indication_fields(raw):
    """Retain repeated field names; raw indication text is always preserved separately."""
    result = {}
    key = None
    lines = []
    expecting_key = False

    def flush():
        if key is not None:
            result.setdefault(key, []).append("\n".join(lines))

    for line in raw.splitlines():
        if re.match(r"^-{6,}", line):
            flush()
            key, lines = None, []
            expecting_key = True
        elif expecting_key:
            key = line
            expecting_key = False
        elif key is not None and line not in {"{", "}", "children are {"}:
            lines.append(line)
    flush()
    return result


def flatten(value, prefix=""):
    if isinstance(value, dict) and value:
        result = {}
        for key, item in sorted(value.items()):
            result.update(flatten(item, f"{prefix}.{key}" if prefix else key))
        return result
    return {prefix: value}


def readable(value):
    # Lua encodes each non-ASCII byte losslessly. Decode UTF-8 only for presentation.
    if isinstance(value, str):
        try:
            value = value.encode("latin-1").decode("utf-8")
        except (UnicodeEncodeError, UnicodeDecodeError):
            pass
    return json.dumps(value, ensure_ascii=False, sort_keys=True)


def compact_ids(ids):
    values = sorted(int(item) for item in ids)
    ranges = []
    for value in values:
        if ranges and value == ranges[-1][1] + 1:
            ranges[-1][1] = value
        else:
            ranges.append([value, value])
    return ", ".join(str(lo) if lo == hi else f"{lo}-{hi}" for lo, hi in ranges)


class Report:
    def __init__(self, catalogue, labels, timeline):
        self.catalogue = catalogue
        self.labels = labels
        self.timeline = timeline
        self.controls = [
            (panel, control) for panel in catalogue["panels"] for control in panel["controls"]
        ]
        self.by_arg = {}
        for panel, control in self.controls:
            arg = control.get("dcs", {}).get("arg")
            if arg is not None:
                self.by_arg.setdefault(str(arg), []).append(f"{panel['key']}.{control['key']}")
        self.segments = {}
        self.current = None
        self.last_stamp = None
        self.previous = {}
        self.sessions = {}
        timeline.write(
            "NATIVE DCS EXPORT - read-only evidence, not input verification\n"
            "Times are simulator seconds. No epsilon/rounding filter is applied.\n"
            "Identical samples are suppressed at source; heartbeat counters retain totals.\n"
            "Zero arguments may be unused slots. Changes alone do not prove identity.\n"
            "Scope: bounded cockpit scan, raw indications, allowlisted own-aircraft telemetry.\n"
            "Newer captures also include separate external arguments and radio frequencies in Hz.\n"
            "Full raw snapshots/deltas are retained in the companion JSONL.\n\n"
        )

    def line(self, stamp, text):
        if stamp != self.last_stamp:
            self.timeline.write(f"\n{stamp}\n")
            self.last_stamp = stamp
        self.timeline.write(f"  {text}\n")

    def accept(self, record):
        kind, key, value = record["kind"], record["key"], record.get("value")
        segment_key = (record["session"], record["segment"])
        stamp = f"[{record['session']} / {record['segment']} @ {record.get('time', '-')}]"
        if kind == "start":
            self.sessions[record["session"]] = value
        if kind == "active":
            if segment_key in self.segments:
                raise ValueError("Duplicate active segment")
            self.segments[segment_key] = {
                "arguments": {}, "fields": {}, "statuses": {},
                "argument_errors": {}, "time_start": record.get("time"), "diagnostics": [],
                "external_arguments": {}, "external_argument_errors": {}, "radio_frequencies": {},
            }
            self.current = segment_key
            self.previous = {}
        if kind in {"arguments", "argument_errors", "indication", "telemetry", "status", "heartbeat",
                    "external_arguments", "external_argument_errors", "radio_frequency"}:
            if self.current != segment_key or segment_key not in self.segments:
                raise ValueError("Sample outside an active cockpit segment")
        state = self.segments.get(segment_key)
        if kind == "arguments":
            zeros = []
            for arg, sample in sorted(value.items(), key=lambda item: int(item[0])):
                stats = state["arguments"].setdefault(arg, {
                    "samples": 0, "min": sample, "max": sample, "first": sample,
                    "last": sample, "transitions": 0, "expected_seen": [],
                })
                initial = stats["samples"] == 0
                if not initial and sample != stats["last"]:
                    stats["transitions"] += 1
                stats["samples"] += 1
                stats["min"], stats["max"], stats["last"] = (
                    min(stats["min"], sample), max(stats["max"], sample), sample
                )
                expected = [0, 1]
                for _, control in self.controls:
                    dcs = control.get("dcs", {})
                    if str(dcs.get("arg")) == arg and "values" in dcs:
                        expected += dcs["values"]
                for endpoint in expected:
                    if abs(sample - endpoint) <= 0.0001 and endpoint not in stats["expected_seen"]:
                        stats["expected_seen"].append(endpoint)
                if initial and sample == 0:
                    zeros.append(arg)
                else:
                    label = " / ".join(self.by_arg.get(arg, [])) or self.labels.get(arg, "unlabelled")
                    self.line(stamp, f"arg {arg} ({label}) = {sample!r}")
            if zeros:
                self.line(stamp, f"initial zero arguments ({len(zeros)}): {compact_ids(zeros)}")
        elif kind in {"external_arguments", "radio_frequency"}:
            namespace = "external_arguments" if kind == "external_arguments" else "radio_frequencies"
            samples = value if kind == "external_arguments" else {key: value}
            for sample_key, sample in sorted(samples.items(), key=lambda item: int(item[0])):
                stats = state[namespace].setdefault(sample_key, {
                    "min": sample, "max": sample, "first": sample, "last": sample,
                    "samples": 0, "transitions": 0,
                })
                if stats["samples"] and stats["last"] != sample:
                    stats["transitions"] += 1
                stats.update(min=min(stats["min"], sample), max=max(stats["max"], sample),
                             last=sample, samples=stats["samples"] + 1)
                description = f"external arg {sample_key}" if kind == "external_arguments" else (
                    f"radio device {sample_key} = COMM{int(sample_key) - 37}"
                )
                units = "" if kind == "external_arguments" else f" Hz ({sample / 1_000_000:g} MHz)"
                self.line(stamp, f"{description}: {sample!r}{units}")
        elif kind in {"indication", "telemetry"}:
            display_value = indication_fields(value) if kind == "indication" else load_json(value)
            if kind == "indication":
                state["fields"][key] = display_value
                history = state.setdefault("field_history", {}).setdefault(key, {})
                for field, texts in display_value.items():
                    entry = history.setdefault(field, {"observations": 0, "min_length": None, "max_length": 0})
                    for text in texts:
                        entry["observations"] += 1
                        length = len(text)
                        entry["min_length"] = length if entry["min_length"] is None else min(entry["min_length"], length)
                        entry["max_length"] = max(entry["max_length"], length)
            flattened = flatten(display_value)
            cache_key = (kind, key)
            before = self.previous.get(cache_key, {})
            for field in sorted(set(before) | set(flattened)):
                if field not in flattened:
                    self.line(stamp, f"{kind} {key}.{field} = <absent>")
                elif field not in before or before[field] != flattened[field]:
                    self.line(stamp, f"{kind} {key}.{field} = {readable(flattened[field])}")
            self.previous[cache_key] = flattened
            if kind == "indication" and value and not display_value:
                self.line(stamp, f"indication {key} (unparsed raw) = {readable(value)}")
        else:
            self.line(stamp, f"{kind} {key} = {readable(value)}")
            if kind == "status" and state is not None:
                state["statuses"][key] = value
                if value not in {"ok", "empty"}:
                    state["diagnostics"].append({"time": record.get("time"), "key": key, "message": value})
            if kind in {"argument_errors", "external_argument_errors"}:
                state[kind] = value
                if value:
                    state["diagnostics"].append({"time": record.get("time"), kind: value})
            if kind in {"heartbeat", "stop", "inactive"} and state is not None:
                state["counters"] = value
                state["time_end"] = record.get("time")
            if kind == "fatal":
                raise ValueError(f"Recorder failed: {value}")
        if kind == "inactive":
            self.current = None

    def coverage(self):
        result = {"warning": "Observations are not verification. No catalogue flags were changed.",
                  "sessions": self.sessions, "segments": []}
        for (session, segment), state in self.segments.items():
            controls = []
            for panel, control in self.controls:
                dcs = control.get("dcs", {})
                item = {
                    "id": f"0x{panel['id']:02X}{control['id']:02X}",
                    "name": f"{panel['key']}.{control['key']}",
                    "input_verified": False, "identity_verified": False,
                    "expectation_source": dcs.get("source"),
                }
                arg = str(dcs.get("arg"))
                if arg in state["arguments"]:
                    stats = state["arguments"][arg]
                    item.update(argument=int(arg), evidence=stats.copy())
                    item["status"] = "changed" if stats["transitions"] else "constant-only"
                    if "values" in dcs or dcs.get("style") in {"button", "magnetic"}:
                        expected = dcs.get("values", [0, 1])
                        item["missing_expected_states"] = [
                            value for value in expected if value not in stats["expected_seen"]
                        ]
                    if dcs.get("style") == "lamp":
                        threshold = dcs.get("threshold", 0.3)
                        item["observed_both_threshold_sides"] = stats["min"] < threshold <= stats["max"]
                        item["threshold_verified"] = False
                    if arg in state["argument_errors"]:
                        item["status"] = "read-error-after-observations"
                elif dcs.get("style") == "indication":
                    field = state.get("field_history", {}).get(str(dcs["display"]), {}).get(dcs["field"])
                    item["status"] = "field-observed" if field else "field-not-observed"
                    if field:
                        item["evidence"] = field
                        item["catalogue_length"] = control["length"]
                        item["exceeds_catalogue_length"] = field["max_length"] > control["length"]
                else:
                    item["status"] = "no-output-mapping" if "arg" not in dcs else "not-observed"
                controls.append(item)
            result["segments"].append({
                "session": session, "segment": segment, "controls": controls,
                "argument_slots_observed": len(state["arguments"]),
                "argument_slots_changed": sum(bool(s["transitions"]) for s in state["arguments"].values()),
                "native_labelled_points_not_sampled": sorted(set(self.labels) - set(state["arguments"]), key=int),
                "stream_statuses": state["statuses"], "argument_errors": state["argument_errors"],
                "diagnostics": state["diagnostics"], "counters": state.get("counters", {}),
                "model_time_start": state["time_start"], "model_time_end": state.get("time_end"),
                "external_arguments": state["external_arguments"],
                "external_argument_errors": state["external_argument_errors"],
                "radio_frequencies_hz": state["radio_frequencies"],
                "native_point_hints": [
                    {"argument_candidate": int(arg), "label": label,
                     "evidence": state["arguments"].get(arg), "mapping_verified": False}
                    for arg, label in sorted(self.labels.items(), key=lambda item: int(item[0]))
                ],
            })
        return result


def write_summary(stream, coverage, count):
    stream.write(
        f"NATIVE DCS CAPTURE SUMMARY\n{count} complete structured records\n\n"
        "Read-only evidence, NOT verification of identity, native commands, or hardware.\n"
        "Successful reads of zero do not establish that an argument slot is used.\n"
        "Expected states and thresholds below are unverified catalogue hypotheses.\n"
        "Raw values are unchanged; only exact repetition is suppressed.\n\n"
    )
    for segment in coverage["segments"]:
        controls = segment["controls"]
        stream.write(
            f"SESSION {segment['session']} / cockpit segment {segment['segment']}\n"
            f"Model time: {segment['model_time_start']} .. {segment['model_time_end']} seconds\n"
            f"Argument slots read: {segment['argument_slots_observed']}; "
            f"slots changed: {segment['argument_slots_changed']}\n"
            f"Recorder counters: {json.dumps(segment['counters'], sort_keys=True)}\n"
        )
        for status in sorted({item["status"] for item in controls}):
            stream.write(f"  {status}: {sum(item['status'] == status for item in controls)}\n")
        stream.write("\nFOLLOW-UP ITEMS\n")
        for item in controls:
            issues = []
            if item.get("missing_expected_states"):
                issues.append(f"expected states not seen: {item['missing_expected_states']}")
            if item.get("exceeds_catalogue_length"):
                issues.append(
                    f"raw text length {item['evidence']['max_length']} exceeds "
                    f"catalogue length {item['catalogue_length']}"
                )
            if item["status"] in {"constant-only", "field-not-observed", "not-observed",
                                  "no-output-mapping", "read-error-after-observations"}:
                issues.append(item["status"])
            if item.get("observed_both_threshold_sides") is False:
                issues.append("both sides of inherited lamp threshold not observed")
            if issues:
                stream.write(f"  {item['id']} {item['name']}: {'; '.join(issues)}\n")
        stream.write("\nCATALOGUE CONTROL EVIDENCE\n")
        for item in controls:
            evidence = item.get("evidence", {})
            detail = ""
            if "argument" in item:
                detail = (f"arg={item['argument']}, min={evidence['min']!r}, "
                          f"max={evidence['max']!r}, transitions={evidence['transitions']}")
            elif "max_length" in evidence:
                detail = f"raw length={evidence['min_length']}..{evidence['max_length']}"
            stream.write(f"  {item['id']} {item['name']}: {item['status']} {detail}\n")
        stream.write("\nSTREAM AVAILABILITY (empty is not a confirmed valid indication)\n")
        for key, status in sorted(segment["stream_statuses"].items()):
            stream.write(f"  {key}: {status}\n")
        stream.write("\nEXTERNAL AIRCRAFT ARGUMENTS (not cockpit slots)\n")
        if not segment["external_arguments"]:
            stream.write("  Not observed in this segment (older captures do not contain this stream).\n")
        for arg, stats in sorted(segment["external_arguments"].items(), key=lambda item: int(item[0])):
            stream.write(f"  external {arg}: {json.dumps(stats, sort_keys=True)}\n")
        stream.write("\nRADIO DEVICE FREQUENCIES (raw Hz)\n")
        if not segment["radio_frequencies_hz"]:
            stream.write("  Not observed in this segment.\n")
        for device, stats in sorted(segment["radio_frequencies_hz"].items()):
            stream.write(f"  device {device}: {json.dumps(stats, sort_keys=True)}\n")
        stream.write(f"\nDiagnostics: {len(segment['diagnostics'])}\n")
        for diagnostic in segment["diagnostics"]:
            stream.write(f"  {json.dumps(diagnostic, sort_keys=True)}\n")
        stream.write("\nNATIVE CLICKABLE POINT LABEL HINTS (not command/argument proof)\n")
        for point in segment["native_point_hints"]:
            evidence = point["evidence"]
            detail = "not sampled" if evidence is None else (
                f"min={evidence['min']!r}, max={evidence['max']!r}, "
                f"transitions={evidence['transitions']}"
            )
            stream.write(f"  point {point['argument_candidate']} {point['label']}: {detail}\n")
        stream.write("\n")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("logs", nargs="+", type=Path, help="Complete DCS logs in chronological order")
    parser.add_argument("--output", type=Path, required=True, help="Output base path (no extension)")
    parser.add_argument("--catalogue", type=Path, default=ROOT / "catalog" / "fa18c.json")
    parser.add_argument("--dcs-root", type=Path, help="Optional local DCS install for native point labels")
    args = parser.parse_args(argv)
    outputs = [Path(str(args.output) + suffix) for suffix in (
        ".jsonl", ".txt", ".coverage.json", ".summary.txt"
    )]
    created = []
    try:
        catalogue = load_json(args.catalogue.read_text(encoding="utf-8"))
        labels, provenance = native_labels(args.dcs_root)
        for path in outputs:
            if path.exists():
                raise FileExistsError(f"Refusing to overwrite existing evidence: {path}")
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with outputs[0].open("x", encoding="utf-8") as raw:
            created.append(outputs[0])
            with outputs[1].open("x", encoding="utf-8") as timeline:
                created.append(outputs[1])
                report = Report(catalogue, labels, timeline)
                count = 0
                for record in decode(args.logs):
                    raw.write(json.dumps(record, ensure_ascii=True, allow_nan=False) + "\n")
                    report.accept(record)
                    count += 1
                if not report.segments:
                    raise ValueError("No active F/A-18C cockpit segment was captured")
                coverage = report.coverage()
                coverage["native_labels"] = provenance
                timeline.write("\nCATALOGUE EVIDENCE COVERAGE (not input verification)\n")
                for segment in coverage["segments"]:
                    timeline.write(f"\n{segment['session']} / {segment['segment']}\n")
                    for control in segment["controls"]:
                        timeline.write(f"  {control['id']} {control['name']}: {control['status']}\n")
        with outputs[2].open("x", encoding="utf-8") as stream:
            created.append(outputs[2])
            json.dump(coverage, stream, indent=2, ensure_ascii=True, allow_nan=False)
            stream.write("\n")
        with outputs[3].open("x", encoding="utf-8") as stream:
            created.append(outputs[3])
            write_summary(stream, coverage, count)
    except (OSError, ValueError, KeyError, TypeError) as error:
        for path in created:
            path.unlink(missing_ok=True)
        print(f"Capture conversion failed: {error}", file=sys.stderr)
        return 1
    print(f"Decoded {count} records across {len(report.segments)} cockpit segments.")
    for path in outputs:
        print(path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
