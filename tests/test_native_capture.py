import importlib.util
import io
import json
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "decode_native_capture", ROOT / "Programs" / "tools" / "decode_native_capture.py"
)
capture = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(capture)


def record(seq, kind, value, key="capture", segment=1, time=1):
    return dict(schema=1, session="test", seq=seq, segment=segment,
                kind=kind, key=key, value=value, time=time)


def transport(item, size=1500):
    encoded = json.dumps(item, ensure_ascii=True)
    parts = [encoded[i:i + size] for i in range(0, len(encoded), size)]
    return "".join(
        f"TEST INFO HornetLinkCapture (Main): HLNC test {item['seq']} {i + 1}/{len(parts)} {part}\n"
        for i, part in enumerate(parts)
    )


CATALOGUE = {"panels": [{"id": 1, "key": "TEST", "controls": [
    {"id": 1, "key": "SWITCH", "type": "switch",
     "dcs": {"source": "dcsbios", "style": "tumb", "arg": 49, "values": [0, 1]}},
    {"id": 2, "key": "LAMP", "type": "lamp",
     "dcs": {"source": "dcsbios", "style": "lamp", "arg": 47, "threshold": 0.3}},
    {"id": 3, "key": "TEXT", "type": "text", "length": 3,
     "dcs": {"source": "dcsbios", "style": "indication", "display": 6, "field": "field"}},
    {"id": 4, "key": "STEP", "type": "rotary",
     "dcs": {"source": "dcsbios", "style": "step"}},
]}]}


class CaptureTests(unittest.TestCase):
    def decode_text(self, text):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "dcs.log"
            path.write_text(text, encoding="utf-8")
            return list(capture.decode([path]))

    def test_chunk_round_trip(self):
        records = [record(1, "start", {}), record(2, "active", {}),
                   record(3, "indication", 'quote"\n\\' * 1000, "6"), record(4, "stop", {})]
        self.assertEqual(self.decode_text("noise\n" + "".join(transport(r, 75) for r in records)), records)

    def test_dropped_reordered_duplicate_chunks(self):
        start = transport(record(1, "start", {}), 30).splitlines(keepends=True)
        for lines in (start[1:], start[:-1], start[:1] + start[:1] + start[1:],
                      list(reversed(start))):
            with self.subTest(lines=len(lines)), self.assertRaises(ValueError):
                self.decode_text("".join(lines))

    def test_sequence_gap_truncated_session_and_invalid_payload(self):
        cases = [
            transport(record(1, "start", {})),
            transport(record(1, "start", {})) + transport(record(3, "stop", {})),
            transport(record(1, "start", {})) + "TEST HornetLinkCapture: HLNC broken\n",
            transport(record(1, "arguments", {"49": float("nan")})),
            transport(record(1, "arguments", {"49": False})),
            transport(record(1, "start", {})) + transport(record(2, "stop", {}))
            + transport(record(3, "status", "ok")),
        ]
        for text in cases:
            with self.subTest(text=text[:100]), self.assertRaises(ValueError):
                self.decode_text(text)

    def test_coverage_does_not_claim_verification(self):
        timeline = io.StringIO()
        report = capture.Report(CATALOGUE, {}, timeline)
        items = [
            record(1, "start", {}),
            record(2, "active", {}),
            record(3, "arguments", {"49": 0, "47": 0}, "device:0"),
            record(4, "arguments", {"49": 0.125, "47": 1}, "device:0"),
            record(5, "indication", "------\nfield\nABCD\n", "6"),
            record(6, "arguments", {"49": 1}, "device:0"),
        ]
        for item in items:
            report.accept(item)
        controls = report.coverage()["segments"][0]["controls"]
        self.assertEqual(len(controls), 4)
        self.assertEqual(controls[0]["missing_expected_states"], [])
        self.assertTrue(controls[1]["observed_both_threshold_sides"])
        self.assertFalse(controls[1]["threshold_verified"])
        self.assertTrue(controls[2]["exceeds_catalogue_length"])
        self.assertEqual(controls[3]["status"], "no-output-mapping")
        self.assertTrue(all(not item["input_verified"] for item in controls))
        self.assertIn("0.125", timeline.getvalue())

    def test_external_and_radio_namespaces_and_legacy_compatibility(self):
        timeline = io.StringIO()
        report = capture.Report(CATALOGUE, {}, timeline)
        items = [
            record(1, "start", {}),
            record(2, "active", {}),
            record(3, "arguments", {"49": 0}, "device:0"),
            record(4, "external_arguments", {"49": 0.75}, "own-aircraft"),
            record(5, "radio_frequency", 251000000, "38"),
            record(6, "external_arguments", {"49": 1}, "own-aircraft"),
            record(7, "radio_frequency", 252000000, "38"),
            record(8, "external_argument_errors", {"49": "restricted"}, "own-aircraft"),
            record(9, "stop", {"errors": 1}),
        ]
        self.assertEqual(self.decode_text("".join(transport(item) for item in items)), items)
        for item in items:
            report.accept(item)
        segment = report.coverage()["segments"][0]
        self.assertEqual(segment["controls"][0]["status"], "constant-only")
        self.assertEqual(segment["external_arguments"]["49"]["transitions"], 1)
        self.assertEqual(segment["radio_frequencies_hz"]["38"]["max"], 252000000)
        self.assertIn("external arg 49", timeline.getvalue())
        self.assertIn("251 MHz", timeline.getvalue())
        summary = io.StringIO()
        capture.write_summary(summary, report.coverage(), len(items))
        self.assertIn("RADIO DEVICE FREQUENCIES", summary.getvalue())
        legacy = capture.Report(CATALOGUE, {}, io.StringIO())
        legacy.accept(record(1, "active", {}))
        self.assertEqual(legacy.coverage()["segments"][0]["external_arguments"], {})

    def test_new_streams_reject_invalid_numeric_values_and_error_shapes(self):
        for item in (
            record(1, "external_arguments", {"25": float("inf")}),
            record(1, "external_argument_errors", {"25": False}),
            record(1, "radio_frequency", -1, "38"),
            record(1, "radio_frequency", False, "39"),
            record(1, "radio_frequency", 251000000, "0"),
        ):
            with self.subTest(item=item), self.assertRaises(ValueError):
                capture.validate_record(item)

    def test_segments_do_not_pool_endpoint_evidence(self):
        report = capture.Report(CATALOGUE, {}, io.StringIO())
        report.accept(record(1, "active", {}))
        report.accept(record(2, "arguments", {"49": 0}, "device:0"))
        report.accept(record(3, "inactive", {}))
        report.accept(record(4, "active", {}, segment=2))
        report.accept(record(5, "arguments", {"49": 1}, "device:0", segment=2))
        segments = report.coverage()["segments"]
        self.assertEqual(segments[0]["controls"][0]["missing_expected_states"], [1])
        self.assertEqual(segments[1]["controls"][0]["missing_expected_states"], [0])

    def test_diagnostics_survive_recovery_and_summary_lists_missing_states(self):
        timeline = io.StringIO()
        report = capture.Report(CATALOGUE, {}, timeline)
        report.accept(record(1, "active", {}))
        report.accept(record(2, "arguments", {"49": 0}, "device:0"))
        report.accept(record(3, "argument_errors", {"49": "temporary error"}, "device:0"))
        report.accept(record(4, "argument_errors", {}, "device:0"))
        report.accept(record(5, "status", "error: restricted", "LoGetTest"))
        report.accept(record(6, "status", "ok", "LoGetTest"))
        report.accept(record(7, "stop", {"errors": 2, "repeated": 123}))
        coverage = report.coverage()
        segment = coverage["segments"][0]
        self.assertEqual(len(segment["diagnostics"]), 2)
        self.assertEqual(segment["stream_statuses"]["LoGetTest"], "ok")
        self.assertEqual(segment["counters"]["repeated"], 123)
        summary = io.StringIO()
        capture.write_summary(summary, coverage, 7)
        self.assertIn("expected states not seen: [1]", summary.getvalue())
        self.assertIn("temporary error", summary.getvalue())

    def test_repeated_indication_names_and_absent_telemetry(self):
        raw = "------\nname\nA\n------\nname\nB\n------\nempty\n"
        self.assertEqual(capture.indication_fields(raw), {"name": ["A", "B"], "empty": [""]})
        report = capture.Report(CATALOGUE, {}, io.StringIO())
        report.accept(record(1, "active", {}))
        report.accept(record(2, "telemetry", '{"n":1,"1":{"present":true,"value":{"x":1}}}', "LoGetTest"))
        report.accept(record(3, "telemetry", '{"n":1,"1":{"present":true,"value":{}}}', "LoGetTest"))
        self.assertIn("<absent>", report.timeline.getvalue())

    def test_cli_outputs_and_no_overwrite(self):
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            log = directory / "dcs.log"
            cat = directory / "catalogue.json"
            cat.write_text(json.dumps(CATALOGUE), encoding="utf-8")
            records = [record(1, "start", {}), record(2, "active", {}),
                       record(3, "arguments", {"49": 0}, "device:0"), record(4, "stop", {})]
            log.write_text("".join(transport(r) for r in records), encoding="utf-8")
            output = directory / "result"
            args = [str(log), "--catalogue", str(cat), "--output", str(output)]
            self.assertEqual(capture.main(args), 0)
            self.assertEqual(len((directory / "result.jsonl").read_text().splitlines()), 4)
            self.assertIn("FOLLOW-UP ITEMS", (directory / "result.summary.txt").read_text())
            raw = (directory / "result.jsonl").read_bytes()
            self.assertEqual(capture.main(args), 1)
            self.assertEqual((directory / "result.jsonl").read_bytes(), raw)
            log.write_text(transport(records[0]), encoding="utf-8")
            self.assertEqual(capture.main([str(log), "--output", str(directory / "bad")]), 1)
            self.assertFalse((directory / "bad.jsonl").exists())
            self.assertFalse((directory / "bad.txt").exists())
            self.assertFalse((directory / "bad.summary.txt").exists())

    def test_native_labels_are_hints_not_mappings(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path = root / "Mods" / "aircraft" / "FA-18C" / "Cockpit" / "Scripts" / "clickabledata.lua"
            path.parent.mkdir(parents=True)
            path.write_text(
                '--elements["pnt_1"] = default_button(_("Ignored"), ...)\n'
                'elements["pnt_49"] = default_button(_("Master Arm"), ...)\n',
                encoding="utf-8",
            )
            labels, provenance = capture.native_labels(root)
            self.assertEqual(labels, {"49": "Master Arm"})
            self.assertEqual(provenance["direct_point_labels"], 1)


if __name__ == "__main__":
    unittest.main()
