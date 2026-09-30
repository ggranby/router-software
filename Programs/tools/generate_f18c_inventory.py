#!/usr/bin/env python3
"""Render a readable F/A-18C control inventory from DCS-BIOS reference JSON."""

import argparse
import hashlib
import json
from pathlib import Path


def format_hex(value):
    return "—" if value is None else f"0x{value:04X}"


def render_input(item):
    interface = item.get("interface", "unknown")
    details = [item.get("description", "")]
    for key in ("argument", "max_value", "suggested_step"):
        if key in item:
            value = item[key]
            details.append(f"{key}={value}")
    return f"`{interface}`" + (f" ({'; '.join(details)})" if any(details) else "")


def render_output(item):
    output_type = item.get("type", "unknown")
    address = format_hex(item.get("address"))
    fields = [f"`{output_type}` at `{address}`"]
    if item.get("mask") is not None:
        fields.append(f"mask `{format_hex(item['mask'])}`")
    if item.get("shift_by") is not None:
        fields.append(f"shift `{item['shift_by']}`")
    if item.get("max_value") is not None:
        fields.append(f"max `{item['max_value']}`")
    if item.get("max_length") is not None:
        fields.append(f"length `{item['max_length']}`")
    description = item.get("description")
    if description:
        fields.append(description)
    return ", ".join(fields)


def render_inventory(reference_path, version):
    raw = reference_path.read_bytes()
    reference = json.loads(raw)
    if not isinstance(reference, dict):
        raise ValueError("expected a JSON object keyed by equipment/category")

    controls = [(group, identifier, control)
                for group, entries in reference.items()
                for identifier, control in entries.items()]
    inputs_count = sum(bool(control.get("inputs")) for _, _, control in controls)
    outputs = [output for _, _, control in controls
               for output in control.get("outputs", [])]
    strings_count = sum(output.get("type") == "string" for output in outputs)
    integers_count = sum(output.get("type") == "integer" for output in outputs)
    source_url = (
        "https://github.com/DCS-Skunkworks/dcs-bios/blob/"
        f"{version}/Scripts/DCS-BIOS/doc/json/FA-18C_hornet.json"
    )

    lines = [
        "# F/A-18C Hornet export inventory",
        "",
        f"> **Baseline:** DCS-BIOS {version} reference JSON. This is a versioned "
        "working catalog, not a claim that DCS-BIOS is the permanent source of truth.",
        "> No live DCS capture has been used to verify these entries.",
        "",
        "This catalog groups each documented control under its DCS-BIOS equipment "
        "category. Controls with one or more input interfaces are marked "
        "**Interactable (documented input)**; those without inputs are "
        "**Output only (no documented input)**. This describes DCS-BIOS metadata, "
        "not proof that every input works in every DCS context.",
        "",
        f"Reference: [{source_url}]({source_url})  ",
        f"SHA-256 of the exact JSON input: `{hashlib.sha256(raw).hexdigest()}`",
        "",
        "## Coverage",
        "",
        f"- Equipment/category groups: **{len(reference)}**",
        f"- Documented controls: **{len(controls)}**",
        f"- Controls with input interfaces: **{inputs_count}**",
        f"- Controls without input interfaces: **{len(controls) - inputs_count}**",
        f"- Output records: **{len(outputs)}** "
        f"({integers_count} integer, {strings_count} string)",
        "",
        "The address, mask, and shift values below are from the DCS-BIOS memory "
        "map. They must not be treated as native DCS export addresses.",
        "",
        "## Inventory",
        "",
    ]

    for group, entries in reference.items():
        lines.extend([
            f"### {group}",
            "",
            "| Control | Description / kind | Interaction | DCS-BIOS output reference |",
            "|---|---|---|---|",
        ])
        for identifier, control in entries.items():
            inputs = control.get("inputs", [])
            interaction = (
                "**Interactable**<br>" + "<br>".join(render_input(item) for item in inputs)
                if inputs else "**Output only**<br>No documented input"
            )
            control_type = control.get("control_type", "unspecified")
            description = control.get("description", "")
            kind = f"`{control_type}`"
            desc = f"{description}<br>{kind}" if description else kind
            output_items = control.get("outputs", [])
            output_text = "<br>".join(render_output(item) for item in output_items)
            if not output_text:
                output_text = "No output descriptor in this reference"
            cells = (identifier, desc, interaction, output_text)
            lines.append("| " + " | ".join(
                cell.replace("|", r"\|").replace("\n", "<br>") for cell in cells
            ) + " |")
        lines.append("")

    lines.extend([
        "## Establishing native DCS evidence",
        "",
        "This baseline can be replaced or supplemented by evidence captured from "
        "DCS. Keep native observations distinct from DCS-BIOS addresses and IDs; "
        "do not infer an address correspondence from matching values alone.",
        "",
        "For each native observation, retain: DCS build and aircraft variant; "
        "mission/start conditions; control or output exercised; raw capture file "
        "and SHA-256; frame/record offset and bytes; observed value before and "
        "after the action; repeated result; and verification status. An observation "
        "can verify a value/address transition, but a capture cannot establish that "
        "an unobserved output does not exist. Mark those as **not observed**, not "
        "**absent**. Keep the release baseline and native evidence side by side "
        "until coverage is sufficient to promote the native source.",
        "",
        "### Native observation log template",
        "",
        "| DCS build / variant | Equipment / control | Scenario or action | Capture SHA-256 | Native frame/address/bytes | Result / status |",
        "|---|---|---|---|---|---|",
        "| Pending live DCS capture | | | | | Not verified |",
        "",
        "## Limitations",
        "",
        "- The inventory includes controls represented in the selected DCS-BIOS "
        "reference JSON, not every internal DCS state or cockpit item.",
        "- The DCS-BIOS category is retained as the grouping label; verify physical "
        "placement and naming against the Hornet cockpit and DCS documentation.",
        "- No DCS runtime is available in the generation environment. A live "
        "capture and systematic interaction pass are still required.",
        "",
    ])
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference_json", type=Path,
                        help="FA-18C_hornet.json from the selected DCS-BIOS release")
    parser.add_argument("--version", default="v0.11.7",
                        help="reference release/tag recorded in the report")
    parser.add_argument("--output", type=Path,
                        help="write Markdown here instead of stdout")
    args = parser.parse_args()
    rendered = render_inventory(args.reference_json, args.version)
    if args.output:
        args.output.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")


if __name__ == "__main__":
    main()
