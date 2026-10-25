import io
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

TOOLS = Path(__file__).resolve().parents[1] / "Programs" / "tools"
with patch.object(sys, "path", [str(TOOLS), *sys.path]):
    import build_native_crosswalk as crosswalk


class CrosswalkTests(unittest.TestCase):
    def test_pinned_only_outputs_keep_source_and_installed_precedence(self):
        baseline = crosswalk.bios_candidates(
            'local hud = {}\nhud = Module.parse_indication(1) or {}\n'
            'FA_18C_hornet:defineString("HUD_ATC_NWS_ENGAGED", function()\n'
            ' return Functions.coerce_nil_to_string(hud.NWS_cue)\nend, 6, "HUD", "ATC")\n'
            'FA_18C_hornet:defineFloatFromDrawArgument("EXT_LAUNCH_BAR", 85, "Ext", "Bar")\n'
            'FA_18C_hornet:defineFloat("EXISTING", 99, {0,1}, "x", "y")\n'
        )
        candidates = crosswalk.merge_baseline_candidates({"EXISTING": {"arg": 49}}, baseline)
        self.assertEqual(candidates["EXISTING"]["arg"], 49)
        self.assertEqual(candidates["EXISTING"]["definition_source"], "installed-module")
        self.assertEqual(candidates["EXT_LAUNCH_BAR"]["external_arg"], 85)
        self.assertEqual(candidates["EXT_LAUNCH_BAR"]["definition_source"], "baseline-module")
        report = crosswalk.build(
            {key: {"has_input": False} for key in candidates},
            {"p": {"EXISTING": {}}}, candidates, [],
            {("s", 1): {"arguments": {"85": {"transitions": 2}},
                         "fields": {("1", "NWS_cue"): {"max_length": 3, "raw_values": ["NWS"]}}}},
            {"NWS_cue": {"controller": "HUD_ATC_NWS_Cue"}},
        )
        rows = {row["identifier"]: row for row in report["controls"]}
        self.assertEqual(rows["EXT_LAUNCH_BAR"]["output_status"], "not-linked-to-capture")
        nws = rows["HUD_ATC_NWS_ENGAGED"]
        self.assertEqual(nws["output_status"], "candidate-field-observed")
        self.assertFalse(nws["installed_reference_present"])
        self.assertEqual(nws["observations"][0]["raw_values"], ["NWS"])
        self.assertIn("native_indication_declaration", nws)
        self.assertFalse(nws["identity_verified"])
        unresolved = crosswalk.bios_candidates(
            'FA_18C_hornet:defineString("HUD", function()\nreturn hud.NWS_cue\nend, 6, "x", "y")'
        )
        self.assertNotIn("field", unresolved["HUD"])

    def test_native_hud_declarations_keep_controller_and_states(self):
        with tempfile.TemporaryDirectory() as directory:
            scripts = Path(directory)
            pages = scripts / "Multipurpose_Display_Group" / "HUD_AVQ32" / "indicator" / "Pages"
            pages.mkdir(parents=True)
            (pages / "HUD_common.lua").write_text(
                'addStrokeText("NWS_cue", nil, font, "Left", {0, 1}, parent, '
                '{{"HUD_ATC_NWS_Cue"}},\n {"", "ATC", "NWS", "NWS HI"})\n', encoding="utf-8",
            )
            (pages / "HUD_AG.lua").write_text(
                'local laser = addStrokeText("MPD_FLIR_LaserStatus_label", "", font, "Center", '
                '{0, 1}, nil, {{"HUD_FLIR_LaserStatus"}},\n {"", "L ARM", "LTD/R"})\n',
                encoding="utf-8",
            )
            outputs = crosswalk.native_hud_outputs(scripts)
            self.assertEqual(outputs["NWS_cue"]["text_candidates"], ["", "ATC", "NWS", "NWS HI"])
            self.assertEqual(outputs["MPD_FLIR_LaserStatus_label"]["controller"], "HUD_FLIR_LaserStatus")

    def test_pinned_inventory_includes_missing_installed_entries(self):
        baseline = crosswalk.inventory(
            "### Panel\n"
            "| SWITCH | Switch | **Interactable** | x | Not verified |\n"
            "| MISSING | Output | **Output only** | x | Not verified |\n"
        )
        report = crosswalk.build(baseline, {"Panel": {"SWITCH": {}}}, {}, [], {})
        self.assertEqual(report["baseline_count"], 2)
        self.assertFalse(report["controls"][1]["installed_reference_present"])
        self.assertTrue(report["controls"][0]["has_input"])
        self.assertFalse(report["controls"][1]["has_input"])

    def test_external_draw_arguments_never_match_cockpit_slots(self):
        candidates = crosswalk.bios_candidates(
            'FA_18C_hornet:defineFloatFromDrawArgument("EXTERNAL", 49, "x", "y")\n'
            'FA_18C_hornet:defineFloat("INTERNAL", 49, {0,1}, "x", "y")\n'
            '--FA_18C_hornet:defineFloat("IGNORED", 49, {0,1}, "x", "y")\n'
        )
        self.assertEqual(candidates["EXTERNAL"]["external_arg"], 49)
        self.assertNotIn("arg", candidates["EXTERNAL"])
        report = crosswalk.build(
            {key: {"has_input": False} for key in candidates},
            {}, candidates, [], {("session", 1): {
                "arguments": {"49": {"min": 0, "max": 1, "transitions": 2}}, "fields": {},
            }},
        )
        self.assertEqual(report["controls"][0]["output_status"], "not-linked-to-capture")
        self.assertEqual(report["controls"][1]["output_status"], "candidate-argument-changed")
        self.assertNotIn("IGNORED", candidates)

    def test_multi_command_helpers_and_ejection_readback(self):
        text = (
            'FA_18C_hornet:defineRockerSwitch("ROCKER", 12, 3003, 3003, 3002, 3002, 377, "x", "y")\n'
            'FA_18C_hornet:defineMissionComputerSwitch("MC_SW", 3, 3025, 3026, 368, "x", "y")\n'
            'FA_18C_hornet:defineEmergencyParkingBrake("BRAKE", 5, 3007, 3006, 241, "x", "y")\n'
            'FA_18C_hornet:defineEjectionHandleSwitch("EJECTION_HANDLE_SW", 7, 3008, 510, "x", "y")\n'
        )
        candidates = crosswalk.bios_candidates(text)
        self.assertEqual(candidates["ROCKER"]["arg"], 377)
        self.assertEqual(candidates["ROCKER"]["commands"], [3003, 3002])
        self.assertNotIn("command", candidates["ROCKER"])
        self.assertEqual(candidates["MC_SW"]["arg"], 368)
        self.assertEqual(candidates["BRAKE"]["arg"], 241)
        self.assertEqual(candidates["EJECTION_HANDLE_SW"]["arg"], 510)
        report = crosswalk.build(
            {key: {"has_input": True} for key in candidates}, {}, candidates, [],
            {("s", 1): {"arguments": {"377": {"min": -1, "max": 1, "transitions": 2}}, "fields": {}}},
        )
        self.assertEqual(report["controls"][0]["mapping_status"], "multi-command-review-required")
        self.assertEqual(report["controls"][0]["output_status"], "candidate-argument-changed")
        self.assertIn("Exclude", report["controls"][-1]["test_plan"]["safety"])

    def test_direct_indication_and_radio_acquisition_gaps(self):
        text = (
            'FA_18C_hornet:defineString("HUD_LTDR", function()\n'
            ' return Functions.coerce_nil_to_string(Module.parse_indication(1)["Laser"])\nend, 5, "HUD", "Laser")\n'
            'FA_18C_hornet:defineIntegerFromGetter("FREQ", function()\n'
            ' return GetDevice(38):get_frequency() / 10000\nend, 65535, "x", "y")\n'
            'FA_18C_hornet:defineReadWriteRadio("COMM1", 38, 7, 3, 1000, "Radio")\n'
        )
        candidates = crosswalk.bios_candidates(text)
        self.assertEqual(candidates["HUD_LTDR"]["field"], "Laser")
        self.assertEqual(candidates["HUD_LTDR"]["display"], 1)
        self.assertEqual(candidates["FREQ"]["divisor"], 10000)
        report = crosswalk.build(
            {key: {"has_input": False} for key in candidates}, {}, candidates, [],
            {("s", 1): {"arguments": {}, "fields": {("1", "Laser"): {"max_length": 4}}}},
        )
        self.assertEqual(report["controls"][0]["output_status"], "candidate-field-observed")
        self.assertEqual(report["controls"][1]["test_plan"]["gap"], "radio-device-frequency-not-captured")
        external = crosswalk.test_plan("EXT_WOW_LEFT", {}, {"external_arg": 6}, "not-linked-to-capture")
        self.assertIn("grounded and airborne", external["next_step"])
        self.assertFalse(external["automatic_actuation"])
        field = crosswalk.test_plan(
            "HUD_LTDR", {}, {"display": 1, "field": "Laser"}, "not-linked-to-capture"
        )
        self.assertEqual(field["gap"], "indication-field-not-observed")

    def test_unique_conflicting_and_ambiguous_native_triples(self):
        base = {"SW": {"has_input": True}}
        hypothesis = {"SW": {"device": 23, "command": 3003, "arg": 49}}
        native = {"device": 23, "command": 3003, "arg": 49}
        for matches, expected in [
            ([native], "unique-native-triple-match"),
            ([native, native], "ambiguous-native-match"),
            ([dict(native, arg=50)], "native-argument-conflict"),
            ([], "native-unresolved"),
        ]:
            with self.subTest(expected=expected):
                row = crosswalk.build(base, {}, hypothesis, matches, {})["controls"][0]
                self.assertEqual(row["mapping_status"], expected)
                self.assertFalse(row["input_verified"])
                self.assertFalse(row["identity_verified"])

    def test_external_radio_evidence_not_substituted_for_cockpit(self):
        candidates = {
            "COCKPIT": {"arg": 25}, "EXT_HOOK": {"external_arg": 25},
            "COMM1_FREQ": {"radio_device": 38, "divisor": 10000},
        }
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "capture.jsonl"
            records = [
                ("start", "capture", {}), ("active", "capture", {}),
                ("arguments", "device:0", {"25": 0}),
                ("external_arguments", "own-aircraft", {"25": 0.75}),
                ("radio_frequency", "38", 251000000),
                ("external_arguments", "own-aircraft", {"25": 1}),
                ("radio_frequency", "38", 252000000), ("stop", "capture", {}),
            ]
            path.write_text("".join(json.dumps(dict(
                schema=1, session="s", segment=1, seq=index, time=index,
                kind=kind, key=key, value=value,
            )) + "\n" for index, (kind, key, value) in enumerate(records, 1)), encoding="utf-8")
            segments = crosswalk.observations([path])
        report = crosswalk.build(
            {key: {"has_input": False} for key in candidates}, {}, candidates, [], segments
        )
        rows = report["controls"]
        self.assertEqual(rows[0]["output_status"], "candidate-argument-constant")
        self.assertEqual(rows[1]["output_status"], "candidate-external-argument-changed")
        self.assertEqual(rows[2]["output_status"], "candidate-radio-frequency-changed")
        self.assertEqual(rows[2]["observations"][0]["max"], 252000000)
        self.assertNotEqual(rows[1]["test_plan"]["gap"], "external-aircraft-stream-not-captured")
        self.assertFalse(rows[2]["input_verified"])

    def test_native_argument_is_not_point_number(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            (path / "devices.lua").write_text(
                'local function counter() end\ndevices["SMS"] = counter()\n', encoding="utf-8"
            )
            (path / "command_defs.lua").write_text(
                'start_command = 3000\ncount = start_command\nSMS_commands = {\nArm = counter();\n}\n',
                encoding="utf-8",
            )
            (path / "clickabledata.lua").write_text(
                'elements["pnt_999"] = default_button(_("Arm"), devices.SMS, SMS_commands.Arm, 49)\n'
                'elements["pnt_888"] = custom_unknown(_("Unknown"), devices.SMS, SMS_commands.Arm, 50)\n'
                'elements["pnt_124"] = default_button_axis_tumb(_("COMM"), devices.SMS, '
                'SMS_commands.Arm, SMS_commands.Arm, 125, 124, 0.2)\n',
                encoding="utf-8",
            )
            result = crosswalk.native_mappings(path)
            self.assertEqual(len(result), 3)
            self.assertEqual(result[0]["arg"], 49)
            self.assertEqual(result[0]["command"], 3001)
            self.assertEqual(result[0]["point"], "pnt_999")
            self.assertEqual([(r["component"], r["arg"]) for r in result[1:]],
                             [("button", 125), ("axis", 124)])

    def test_channel_getter_uses_literal_map_without_inventing_cached_values(self):
        candidates = crosswalk.bios_candidates(
            'local comm_channel_map = {[" 1"] = 1, ["`0"] = 10, ["~0"] = 20, ["G"] = 21}\n'
            'FA_18C_hornet:defineIntegerFromGetter("COMM1_CHANNEL_NUMERIC", function()\n'
            ' return get_comm_channel(1)\nend, 24, "x", "y")\n'
        )
        item = candidates["COMM1_CHANNEL_NUMERIC"]
        self.assertEqual(item["field"], "UFC_Comm1Display")
        self.assertEqual(item["channel_map"], {" 1": 1, "`0": 10, "~0": 20, "G": 21})
        samples = [{"time": i, "raw": raw}
                   for i, raw in enumerate(["", " 1", " 1", "`0", "d", "~0", "G", "?", "_"])]
        decoded = crosswalk.channel_evidence(samples, item["channel_map"])
        self.assertEqual(decoded["numeric_candidates"], [1, 10, 20, 21])
        self.assertEqual(decoded["unresolved_raw_values"], ["", "?", "_", "d"])
        self.assertEqual(len(decoded["sequence"]), 8)
        self.assertIsNone(decoded["sequence"][3]["numeric_candidate"])
        self.assertEqual(decoded["sequence"][3]["state"], "receive-symbol")
        row = crosswalk.build(
            {"COMM1_CHANNEL_NUMERIC": {"has_input": False}}, {}, candidates, [],
            {("s", 1): {"arguments": {}, "fields": {
                ("6", "UFC_Comm1Display"): {"max_length": 2, "channel_samples": samples},
            }}},
        )["controls"][0]
        self.assertEqual(row["output_status"], "candidate-channel-decoded")
        self.assertFalse(row["identity_verified"])

    def test_unsupported_channel_map_is_not_silently_decoded(self):
        with self.assertRaisesRegex(ValueError, "Unsupported or duplicate"):
            crosswalk.bios_candidates('local comm_channel_map = {[" 1"] = 1, other()}\n')
        with self.assertRaisesRegex(ValueError, "Unsupported or duplicate"):
            crosswalk.bios_candidates('local comm_channel_map = {[" 1"] = 1, [" 1"] = 2}\n')

    def test_channel_capture_keeps_absence_and_segments_without_cache(self):
        def indication(raw):
            return "" if raw is None else "------\nUFC_Comm1Display\n" + raw + "\n"

        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "capture.jsonl"
            records = [
                (0, "start", "capture", {}),
                (1, "active", "capture", {}),
                (1, "indication", "6", indication("`0")),
                (1, "indication", "6", indication("`0")),
                (1, "indication", "6", indication(None)),
                (1, "indication", "6", indication("d")),
                (1, "indication", "6", indication("G")),
                (1, "inactive", "capture", {}),
                (2, "active", "capture", {}),
                (2, "indication", "6", indication("d")),
                (2, "stop", "capture", {}),
            ]
            path.write_text("".join(json.dumps(dict(
                schema=1, session="s", segment=segment, seq=i, time=i,
                kind=kind, key=key, value=value,
            )) + "\n" for i, (segment, kind, key, value) in enumerate(records, 1)), encoding="utf-8")
            segments = crosswalk.observations([path, path])
            samples = segments[("s", 1)]["fields"][("6", "UFC_Comm1Display")]["channel_samples"]
            self.assertEqual([s["raw"] for s in samples], ["`0", None, "d", "G"])
            result = crosswalk.channel_evidence(samples, {"`0": 10, "G": 21})
            self.assertEqual(result["sequence"][-1]["display_candidate"], "G")
            self.assertEqual(result["unresolved_raw_values"], [None, "d"])
            second = segments[("s", 2)]["fields"][("6", "UFC_Comm1Display")]["channel_samples"]
            self.assertEqual(crosswalk.channel_evidence(second, {"`0": 10})["numeric_candidates"], [])
            records[2] = (1, "indication", "6", indication(" 1") + indication(" 2"))
            path.write_text("".join(json.dumps(dict(
                schema=1, session="s", segment=segment, seq=i,
                kind=kind, key=key, value=value,
            )) + "\n" for i, (segment, kind, key, value) in enumerate(records, 1)), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "Ambiguous repeated"):
                crosswalk.observations([path])

    def test_native_selector_argument_and_display_feedback_remain_distinct(self):
        identifier = "UFC_COMM1_CHANNEL_SELECT"
        native = {"device": 25, "command": 3033, "arg": 124}
        segments = {("s", 1): {
            "arguments": {"124": {"min": 0, "max": 0, "transitions": 0}},
            "fields": {("6", "UFC_Comm1Display"): {
                "max_length": 2, "channel_samples": [{"time": 0, "raw": " 1"}, {"time": 1, "raw": " 4"}],
            }},
        }}
        row = crosswalk.build(
            {identifier: {"has_input": True}}, {},
            {identifier: {"device": 25, "command": 3033}}, [native], segments,
        )["controls"][0]
        self.assertEqual(row["native_output_argument"], 124)
        self.assertEqual(row["output_status"], "candidate-argument-constant")
        self.assertEqual(len(row["related_channel_feedback"][0]["samples"]), 2)
        self.assertFalse(row["input_verified"])
        ambiguous = crosswalk.build(
            {identifier: {"has_input": True}}, {},
            {identifier: {"device": 25, "command": 3033}}, [native, native], segments,
        )["controls"][0]
        self.assertNotIn("native_output_argument", ambiguous)
        self.assertEqual(ambiguous["output_status"], "not-linked-to-capture")

    def test_field_is_transformed_hypothesis_and_detects_length_conflict(self):
        candidates = crosswalk.bios_candidates(
            'FA_18C_hornet:defineString("TEMP", function()\n'
            ' return Functions.pad_left(ifei.txt_TEMP_L, 3)\nend, 3, "IFEI", "TEMP")'
        )
        self.assertTrue(candidates["TEMP"]["transformed_getter"])
        report = crosswalk.build(
            {"TEMP": {"category": "IFEI", "has_input": False}},
            {"IFEI": {"TEMP": {"outputs": [{"max_length": 3}]}}},
            candidates, [], {("session", 1): {
                "arguments": {}, "fields": {("5", "txt_TEMP_L"): {"max_length": 4}},
            }},
        )
        row = report["controls"][0]
        self.assertEqual(row["text_length_conflict"], {"raw_max": 4, "reference_max": 3})
        self.assertEqual(row["output_status"], "candidate-field-observed")
        text = io.StringIO()
        crosswalk.write_text(report, text)
        self.assertIn("[ ] Resolve raw indication/reference text-length mismatch", text.getvalue())

    def test_capture_overlap_deduplicates_but_conflicts_and_gaps_fail(self):
        def record(seq, kind, value=None):
            return dict(session="s", seq=seq, schema=1, segment=1, time=0,
                        kind=kind, key="capture", value=value)

        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "capture.jsonl"
            rows = [record(1, "start"), record(2, "active"),
                    record(3, "arguments", {"49": 0}), record(4, "stop")]
            path.write_text("".join(json.dumps(r) + "\n" for r in rows), encoding="utf-8")
            self.assertEqual(len(crosswalk.observations([path, path])), 1)
            other = Path(directory) / "other.jsonl"
            other.write_text(json.dumps(record(3, "arguments", {"49": 1})) + "\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "Conflicting"):
                crosswalk.observations([path, other])
            path.write_text(json.dumps(rows[0]) + "\n" + json.dumps(rows[-1]), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "sequence gap"):
                crosswalk.observations([path])


if __name__ == "__main__":
    unittest.main()
