#!/usr/bin/env python3
from __future__ import annotations

import json
import re
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path


PIN_DEFINE_RE = re.compile(r"^\s*#define\s+(PIN_[A-Z0-9_]+)\s+(\d+)\b")
TOKEN_RE = re.compile(r"[^a-z0-9]+")


def slugify(value: str) -> str:
    return TOKEN_RE.sub("-", value.lower()).strip("-")


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def parse_pin_macros(path: Path) -> dict[str, int]:
    macros: dict[str, int] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        match = PIN_DEFINE_RE.match(line)
        if match:
            macros[match.group(1)] = int(match.group(2))
    return macros


def ensure(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def validate_metadata(metadata: dict, pin_macros: dict[str, int]) -> list[str]:
    warnings: list[str] = []
    pins = metadata["pins"]
    restricted_ranges = metadata.get("restricted_ranges", [])

    source_macros = {pin["source_macro"] for pin in pins}
    missing_in_metadata = sorted(set(pin_macros) - source_macros)
    extra_in_metadata = sorted(source_macros - set(pin_macros))
    ensure(not missing_in_metadata, f"Missing pin metadata for macros: {', '.join(missing_in_metadata)}")
    ensure(not extra_in_metadata, f"Metadata references unknown macros: {', '.join(extra_in_metadata)}")

    for pin in pins:
        expected_gpio = pin_macros[pin["source_macro"]]
        ensure(
            pin["gpio_number"] == expected_gpio,
            f"{pin['source_macro']} metadata GPIO {pin['gpio_number']} does not match pin_config.h value {expected_gpio}",
        )

    counts = Counter(pin["gpio_number"] for pin in pins)
    duplicates = sorted(gpio for gpio, count in counts.items() if count > 1)
    ensure(not duplicates, f"Duplicate GPIO assignments in metadata: {', '.join(map(str, duplicates))}")

    restricted_members = set()
    for entry in restricted_ranges:
        restricted_members.update(range(entry["start_gpio"], entry["end_gpio"] + 1))
    overlaps = sorted(pin["gpio_number"] for pin in pins if pin["gpio_number"] in restricted_members)
    ensure(not overlaps, f"Tracked pins overlap restricted ranges: {', '.join(map(str, overlaps))}")

    gpio0 = next((pin for pin in pins if pin["gpio_number"] == 0), None)
    if gpio0 and "boot" not in gpio0.get("display_badges", []):
        warnings.append("GPIO0 should carry a boot badge for board view.")

    for pin in pins:
        if pin["status"] == "reserved" and pin.get("is_project_used") and not pin.get("notes"):
            warnings.append(f"{pin['signal_name']} is reserved but missing explanatory notes.")

    return warnings


def synthesize_free_pins(metadata: dict) -> list[dict]:
    page = metadata["page_config"]
    start = page["gpio_range"]["start"]
    end = page["gpio_range"]["end"]
    split = page["layout"]["side_split_gpio"]
    used = {pin["gpio_number"] for pin in metadata["pins"]}
    restricted = set()
    for entry in metadata.get("restricted_ranges", []):
        restricted.update(range(entry["start_gpio"], entry["end_gpio"] + 1))

    free_pins = []
    for gpio in range(start, end + 1):
        if gpio in used or gpio in restricted:
            continue
        side = "left" if gpio <= split else "right"
        free_pins.append(
            {
                "id": f"free-gpio-{gpio}",
                "kind": "free",
                "gpio_number": gpio,
                "signal_name": f"GPIO{gpio}",
                "short_label": f"GPIO{gpio}",
                "long_label": "Unassigned GPIO",
                "peripheral_group": "FREE",
                "render_group": "free",
                "status": "free",
                "direction": "available",
                "side": side,
                "order": gpio,
                "notes": "Synthesized by generator because no explicit project assignment exists.",
                "source_macro": "",
                "view_color": "free",
                "display_badges": [],
                "badge": "",
                "is_project_used": False,
                "anchor_class": f"free-gpio-{gpio}",
            }
        )
    return free_pins


def build_render_model(metadata: dict, warnings: list[str]) -> dict:
    pins = []
    for pin in metadata["pins"]:
        entry = dict(pin)
        entry["id"] = f"gpio-{pin['gpio_number']}"
        entry["kind"] = "pin"
        entry["detail_title"] = f"GPIO{pin['gpio_number']}"
        pins.append(entry)

    restricted = []
    for entry in metadata.get("restricted_ranges", []):
        item = dict(entry)
        item["kind"] = "restricted-range"
        item["detail_title"] = entry["label"]
        item["signal_name"] = entry["label"]
        item["is_project_used"] = False
        restricted.append(item)

    specials = []
    for entry in metadata["page_config"].get("board_specials", []):
        item = dict(entry)
        item["kind"] = "special"
        item["detail_title"] = entry["label"]
        item["render_group"] = item.get("kind", "system")
        item["view_color"] = "power" if item["kind"] == "power" else ("reserved" if item["kind"] == "ground" else "system")
        item["status"] = "special"
        item["is_project_used"] = item["id"] == "board-boot"
        specials.append(item)

    free_pins = synthesize_free_pins(metadata)

    all_items = pins + restricted + specials + free_pins
    all_items.sort(key=lambda item: (item.get("side", "left"), item.get("order", 0), item.get("gpio_number", 999)))

    left_items = [item for item in all_items if item.get("side") == "left"]
    right_items = [item for item in all_items if item.get("side") == "right"]

    return {
      "meta": {
        "board": metadata["board"],
        "mcu": metadata["mcu"],
        "title": metadata["page_config"]["title"],
        "subtitle": metadata["page_config"]["subtitle"],
        "board_style": metadata["page_config"]["board_style"],
      },
      "page_config": metadata["page_config"],
      "render_defaults": metadata["render_defaults"],
      "warnings": warnings,
      "left_items": left_items,
      "right_items": right_items,
      "all_items": all_items,
      "tracked_gpio_count": len(metadata["pins"]),
      "free_gpio_count": len(free_pins),
      "restricted_count": len(restricted),
    }


def generate_html(repo_root: Path, render_model: dict) -> str:
    template_path = repo_root / "scripts" / "pinout_board_template.html"
    template = template_path.read_text(encoding="utf-8")
    generated_at = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")
    html = template.replace("__PINOUT_DATA__", json.dumps(render_model, ensure_ascii=False))
    html = html.replace("__GENERATED_AT__", generated_at)
    return html


def main() -> None:
    repo_root = Path(__file__).resolve().parent.parent
    metadata_path = repo_root / "Docs" / "pinout_metadata.json"
    pin_header_path = repo_root / "include" / "pin_config.h"

    metadata = load_json(metadata_path)
    pin_macros = parse_pin_macros(pin_header_path)
    warnings = validate_metadata(metadata, pin_macros)
    render_model = build_render_model(metadata, warnings)

    output_path = repo_root / metadata["page_config"]["output_path"]
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(generate_html(repo_root, render_model), encoding="utf-8")

    print(f"Generated {output_path.relative_to(repo_root)}")
    print(f"Tracked pins: {render_model['tracked_gpio_count']}")
    print(f"Synthesized free GPIOs: {render_model['free_gpio_count']}")
    if warnings:
        print("Warnings:")
        for warning in warnings:
            print(f" - {warning}")


if __name__ == "__main__":
    main()
