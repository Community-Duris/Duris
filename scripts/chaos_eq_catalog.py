#!/usr/bin/env python3
"""Build sanitized Chaos-mode equipment catalogs from analyzer evidence."""
from __future__ import annotations

import argparse
import json
import math
import re
import shutil
import subprocess
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any


RUNTIME_SLOT_NAMES = {
    0: "WEAR_LIGHT",
    1: "WEAR_FINGER_R",
    2: "WEAR_FINGER_L",
    3: "WEAR_NECK_1",
    4: "WEAR_NECK_2",
    5: "WEAR_BODY",
    6: "WEAR_HEAD",
    7: "WEAR_LEGS",
    8: "WEAR_FEET",
    9: "WEAR_HANDS",
    10: "WEAR_ARMS",
    11: "WEAR_SHIELD",
    12: "WEAR_ABOUT",
    13: "WEAR_WAIST",
    14: "WEAR_WRIST_R",
    15: "WEAR_WRIST_L",
    16: "PRIMARY_WEAPON",
    17: "SECONDARY_WEAPON",
    18: "HOLD",
    19: "WEAR_EYES",
    20: "WEAR_FACE",
    21: "WEAR_EARRING_R",
    22: "WEAR_EARRING_L",
    23: "WEAR_QUIVER",
    24: "GUILD_INSIGNIA",
    25: "THIRD_WEAPON",
    26: "FOURTH_WEAPON",
    27: "WEAR_BACK",
    28: "WEAR_ATTACH_BELT_1",
    29: "WEAR_ATTACH_BELT_2",
    30: "WEAR_ATTACH_BELT_3",
    31: "WEAR_ARMS_2",
    32: "WEAR_HANDS_2",
    33: "WEAR_WRIST_LR",
    34: "WEAR_WRIST_LL",
    35: "WEAR_HORSE_BODY",
    36: "WEAR_LEGS_REAR",
    37: "WEAR_TAIL",
    38: "WEAR_FEET_REAR",
    39: "WEAR_NOSE",
    40: "WEAR_HORN",
    41: "WEAR_IOUN",
    42: "WEAR_SPIDER_BODY",
}

# Core slots are the normal human-shaped equipment path. HOLD is reserved for
# class fundamentals so a melee offhand is not silently overwritten by a book.
CORE_SLOTS = tuple(list(range(1, 18)) + list(range(19, 25)) + list(range(27, 31)))

# Runtime wear-bit values from core/defines.h.  This is intentionally explicit
# so a saved item observed in an impossible slot cannot enter the catalog.
SLOT_WEAR_BITS = {
    **{slot: 1 << 1 for slot in (1, 2)},
    **{slot: 1 << 2 for slot in (3, 4)},
    5: 1 << 3,
    6: 1 << 4,
    7: 1 << 5,
    8: 1 << 6,
    9: 1 << 7,
    10: 1 << 8,
    11: 1 << 9,
    12: 1 << 10,
    13: 1 << 11,
    14: 1 << 12,
    15: 1 << 12,
    16: 1 << 13,
    17: 1 << 13,
    18: 1 << 14,
    19: 1 << 17,
    20: 1 << 18,
    21: 1 << 19,
    22: 1 << 19,
    23: 1 << 20,
    24: 1 << 21,
    25: 1 << 13,
    26: 1 << 13,
    27: 1 << 22,
    28: 1 << 23,
    29: 1 << 23,
    30: 1 << 23,
    31: 1 << 8,
    32: 1 << 7,
    33: 1 << 12,
    34: 1 << 12,
    35: 1 << 24,
    36: 1 << 5,
    37: 1 << 25,
    38: 1 << 6,
    39: 1 << 26,
    40: 1 << 27,
    41: 1 << 28,
    42: 1 << 29,
}

BOOK_CLASSES = {"Sorcerer", "Conjurer", "Necromancer", "Illusionist", "Bard", "Summoner", "Reaver", "Theurgist"}

# The risk score is evidence-oriented, not a claim about the game's exact
# combat power.  It suppresses one-off effect bundles from the starting kit and
# leaves them visible as alternatives in the report.
MAX_ITEM_RISK = 4.0


def identifier(value: str) -> str:
    value = re.sub(r"[^a-z0-9]+", "_", value.lower()).strip("_")
    return value or "unknown"


def item_is_valid(metric: dict[str, Any], profile: str, class_id: int, slot: int) -> bool:
    if metric.get("exclusion_reasons"):
        return False
    if profile == "enhanceable" and not metric.get("enhanceable"):
        return False
    if not metric.get("race_portable"):
        return False
    if not metric.get("class_eligible", {}).get(str(class_id), False):
        return False
    static = metric.get("static") or {}
    wear_flags = static.get("wear_flags")
    if wear_flags is None or not (int(wear_flags) & SLOT_WEAR_BITS.get(slot, 0)):
        return False
    return float(metric.get("risk_score", 0.0)) <= MAX_ITEM_RISK


def fallback_score(metric: dict[str, Any], slot: int) -> float:
    slot_support = int(metric.get("slot_players", {}).get(str(slot), 0))
    return (
        slot_support * 3.0
        + min(float(metric.get("power_score", 0.0)), 80.0) * 0.35
        + int(metric.get("observed_players", 0)) * 0.25
        - float(metric.get("risk_score", 0.0)) * 5.0
    )


def choose_equipment(
    analysis: dict[str, Any],
    profile: str,
    class_name: str,
    class_id: int,
    metrics: dict[int, dict[str, Any]],
) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    selected: list[dict[str, Any]] = []
    decisions: list[dict[str, Any]] = []
    direct = {
        int(entry["slot"]): entry
        for entry in analysis["recommendations"][profile].get(class_name, [])
        if int(entry["slot"]) in CORE_SLOTS
    }
    for slot in CORE_SLOTS:
        entry = direct.get(slot)
        metric = metrics.get(int(entry["vnum"])) if entry else None
        choice_reason = "direct class/slot recommendation"
        if not metric or not item_is_valid(metric, profile, class_id, slot):
            choices = [
                candidate for candidate in metrics.values()
                if item_is_valid(candidate, profile, class_id, slot)
            ]
            choices.sort(key=lambda candidate: (fallback_score(candidate, slot), candidate["vnum"]), reverse=True)
            metric = choices[0] if choices else None
            choice_reason = "portable validated fallback; no safe direct recommendation"
        if not metric:
            decisions.append({"slot": slot, "status": "missing", "reason": "no candidate passes class/race/slot/risk checks"})
            continue
        item = {
            "slot": slot,
            "slot_name": RUNTIME_SLOT_NAMES[slot],
            "vnum": metric["vnum"],
            "name": metric["name"],
            "observed_players": metric.get("observed_players", 0),
            "observed_share": metric.get("observed_share", 0),
            "power_score": metric.get("power_score", 0),
            "risk_score": metric.get("risk_score", 0),
            "enhanceable": bool(metric.get("enhanceable")),
            "effect_summary": metric.get("effect_summary", {}),
            "reason": metric.get("reason", ""),
            "selection_reason": choice_reason,
        }
        selected.append(item)
        decisions.append({
            "slot": slot,
            "status": "selected",
            "vnum": metric["vnum"],
            "reason": choice_reason,
        })
    return selected, decisions


def fundamental_exceptions(metric: dict[str, Any]) -> list[str]:
    allowed = {"item_transient", "item_norent", "item_noshow", "item_nosell", "quest_item"}
    return [reason for reason in metric.get("exclusion_reasons", []) if reason not in allowed]


def choose_book(metrics: dict[int, dict[str, Any]], profile: str) -> dict[str, Any] | None:
    candidates = []
    for metric in metrics.values():
        if metric.get("type_name") != "spellbook":
            continue
        if profile == "enhanceable":
            if not metric.get("enhanceable"):
                continue
            if fundamental_exceptions(metric):
                continue
        else:
            if fundamental_exceptions(metric):
                continue
        if not metric.get("race_portable"):
            continue
        if not (int(metric["static"].get("wear_flags", 0)) & (1 << 23)):
            continue
        if not all(metric.get("class_eligible", {}).get(str(class_id), False) for class_id in range(1, 31)):
            continue
        candidates.append(metric)
    candidates.sort(key=lambda metric: (metric.get("observed_players", 0), -metric.get("risk_score", 0), -metric["vnum"]), reverse=True)
    return candidates[0] if candidates else None


def choose_instrument(metrics: dict[int, dict[str, Any]], instrument_value: int, profile: str, bard_id: int) -> dict[str, Any] | None:
    candidates = []
    for metric in metrics.values():
        if metric.get("type_name") != "instrument":
            continue
        if int((metric.get("static") or {}).get("values", [0])[0]) != instrument_value:
            continue
        if fundamental_exceptions(metric) or not metric.get("race_portable"):
            continue
        if not metric.get("class_eligible", {}).get(str(bard_id), False):
            continue
        if profile == "enhanceable" and not metric.get("enhanceable"):
            continue
        candidates.append(metric)
    candidates.sort(key=lambda metric: ("legendary" in metric["name"].lower() if profile == "standard" else False,
                                        metric.get("observed_players", 0),
                                        -metric.get("risk_score", 0),
                                        -metric["vnum"]), reverse=True)
    return candidates[0] if candidates else None


def choose_totem(metrics: dict[int, dict[str, Any]], profile: str, shaman_id: int) -> dict[str, Any] | None:
    candidates = []
    for metric in metrics.values():
        static = metric.get("static") or {}
        values = static.get("values", [0])
        if metric.get("type_name") != "totem" or not values or (int(values[0]) & 63) != 63:
            continue
        if fundamental_exceptions(metric) or not metric.get("race_portable"):
            continue
        if not metric.get("class_eligible", {}).get(str(shaman_id), False):
            continue
        if profile == "enhanceable" and not metric.get("enhanceable"):
            continue
        candidates.append(metric)
    candidates.sort(key=lambda metric: (metric.get("observed_players", 0), -metric.get("risk_score", 0), -metric["vnum"]), reverse=True)
    return candidates[0] if candidates else None


def choose_support_consumables(analysis: dict[str, Any]) -> list[dict[str, Any]]:
    standard = analysis.get("consumables", {}).get("standard", [])
    by_category: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for entry in standard:
        by_category[entry["category"]].append(entry)
    policy = {
        "potion": (4, 3),
        "scroll": (3, 2),
        "food": (1, 4),
        "bandage": (1, 4),
        "herb": (1, 3),
        "drinkcon": (1, 1),
    }
    support: list[dict[str, Any]] = []
    for category, (limit, count) in policy.items():
        entries = by_category.get(category, [])
        for entry in entries[:limit]:
            support.append({
                "category": category,
                "vnum": entry["vnum"],
                "name": entry["name"],
                "count": count,
                "observed_players": entry.get("observed_players", 0),
                "median_quantity": entry.get("median_quantity", 0),
                "upper_quartile_quantity": entry.get("upper_quartile_quantity", 0),
                "reason": entry.get("reason", "observed high-level carried/contained usage"),
            })
    return support


def choose_optional_variations(analysis: dict[str, Any], profile: str, metrics: dict[int, dict[str, Any]]) -> list[dict[str, Any]]:
    requirements = [
        ("four_hands", [25, 26, 31, 32, 33, 34], ["Thri-Kreen"], "HAS_FOUR_HANDS() or AFF3_FOUR_ARMS"),
        ("tail", [37], ["Centaur", "Minotaur", "Shadow Beast", "Kobold", "Tiefling"], "HAS_TAIL()"),
        ("nose", [39], ["Minotaur"], "IS_MINOTAUR()"),
        ("horns", [40], ["Minotaur", "Harpy", "Gargoyle", "Shadow Beast", "Tiefling"], "runtime horn-bearing race check"),
        ("horse_body", [35], ["Centaur"], "has_innate(INNATE_HORSE_BODY)"),
        ("spider_body", [42], ["Drider"], "has_innate(INNATE_SPIDER_BODY)"),
        ("rear_legs", [36], ["race/body-plan dependent"], "runtime currently rejects WEAR_LEGS_REAR"),
        ("rear_feet", [38], ["race/body-plan dependent"], "runtime currently rejects WEAR_FEET_REAR"),
    ]
    variations: list[dict[str, Any]] = []
    for name, slots, races, condition in requirements:
        for slot in slots:
            if name in {"rear_legs", "rear_feet"}:
                variations.append({
                    "status": "unavailable",
                    "variation": name,
                    "condition": condition,
                    "races": races,
                    "slot": slot,
                    "slot_name": RUNTIME_SLOT_NAMES[slot],
                    "reason": "the current runtime has_eq_slot() contract rejects this slot",
                })
                continue
            candidates = []
            for metric in metrics.values():
                if metric.get("exclusion_reasons") or not metric.get("race_portable"):
                    continue
                if profile == "enhanceable" and not metric.get("enhanceable"):
                    continue
                if not all(metric.get("class_eligible", {}).get(str(class_id), False) for class_id in range(1, 31)):
                    continue
                if not (int((metric.get("static") or {}).get("wear_flags", 0)) & SLOT_WEAR_BITS.get(slot, 0)):
                    continue
                if float(metric.get("risk_score", 0.0)) > MAX_ITEM_RISK:
                    continue
                support = int(metric.get("slot_players", {}).get(str(slot), 0))
                candidates.append((support, metric.get("observed_players", 0), -float(metric.get("risk_score", 0.0)), metric))
            candidates.sort(key=lambda item: (item[0], item[1], item[2], -item[3]["vnum"]), reverse=True)
            if candidates:
                metric = candidates[0][3]
                variations.append({
                    "status": "available",
                    "variation": name,
                    "condition": condition,
                    "races": races,
                    "slot": slot,
                    "slot_name": RUNTIME_SLOT_NAMES[slot],
                    "vnum": metric["vnum"],
                    "name": metric["name"],
                    "enhanceable": bool(metric.get("enhanceable")),
                    "risk_score": metric.get("risk_score", 0),
                    "reason": "portable low-risk optional slot variant chosen from observed wear usage",
                })
            else:
                variations.append({
                    "status": "unavailable",
                    "variation": name,
                    "condition": condition,
                    "races": races,
                    "slot": slot,
                    "slot_name": RUNTIME_SLOT_NAMES[slot],
                    "reason": "no candidate passes the active object, restriction, and profile checks",
                })
    return variations


def class_matrix(analysis: dict[str, Any], metrics: dict[int, dict[str, Any]], profile: str, fundamentals: dict[str, Any]) -> dict[str, dict[str, Any]]:
    class_ids = analysis.get("class_ids", {})
    result: dict[str, dict[str, Any]] = {}
    for class_name, class_id in class_ids.items():
        equipment, decisions = choose_equipment(analysis, profile, class_name, int(class_id), metrics)
        support: list[dict[str, Any]] = []
        if class_name in BOOK_CLASSES:
            book = fundamentals.get("spellbook")
            if book:
                support.append({"slot": 18, "slot_name": "HOLD", "vnum": book["vnum"], "name": book["name"], "role": "spellbook"})
        if class_name == "Bard":
            for instrument in fundamentals.get("bard_instruments", []):
                support.append({"slot": -1, "slot_name": "bag_support", "vnum": instrument["vnum"], "name": instrument["name"], "role": f"bard_instrument:{instrument['instrument_type']}"})
        if class_name == "Shaman":
            totem = fundamentals.get("shaman_totem")
            if totem:
                support.append({"slot": 18, "slot_name": "HOLD", "vnum": totem["vnum"], "name": totem["name"], "role": "three_sphere_high_circle_totem"})
        return_row = {
            "class_id": int(class_id),
            "observed_high_level_characters": analysis.get("class_counts", {}).get(class_name, 0),
            "evidence_status": "observed" if analysis.get("class_counts", {}).get(class_name, 0) else "role/static fallback",
            "equipment": equipment,
            "support_items": support,
            "decisions": decisions,
        }
        result[class_name] = return_row
    return result


def common_by_slot(matrix: dict[str, dict[str, Any]]) -> dict[str, dict[str, Any]]:
    counts: dict[str, Counter[int]] = defaultdict(Counter)
    total = len(matrix)
    for row in matrix.values():
        for item in row["equipment"]:
            counts[str(item["slot"])][item["vnum"]] += 1
    common: dict[str, dict[str, Any]] = {}
    for slot, counter in counts.items():
        vnum, count = counter.most_common(1)[0]
        if count >= math.ceil(total * 0.8):
            sample = next(item for row in matrix.values() for item in row["equipment"] if str(item["slot"]) == slot and item["vnum"] == vnum)
            common[slot] = {"slot": int(slot), "slot_name": sample["slot_name"], "vnum": vnum, "name": sample["name"], "class_count": count, "class_total": total}
    return common


def build_fundamentals(analysis: dict[str, Any], metrics: dict[int, dict[str, Any]]) -> dict[str, Any]:
    class_ids = analysis.get("class_ids", {})
    bard_id = int(class_ids.get("Bard", 16))
    shaman_id = int(class_ids.get("Shaman", 9))
    standard_book = metrics.get(7)
    standard_book_row = None
    if standard_book:
        standard_book_row = {"vnum": 7, "name": standard_book["name"], "beltable": bool(int(standard_book["static"].get("wear_flags", 0)) & (1 << 23)), "enhanceable": False, "reason": "runtime master spellbook; dynamic spell filling remains in read_object()"}
    alt_book = choose_book(metrics, "enhanceable")
    alt_book_row = {"vnum": alt_book["vnum"], "name": alt_book["name"], "beltable": True, "enhanceable": True, "reason": "strict enhance-index alternative spellbook"} if alt_book else None
    instruments_standard = []
    instruments_alt = []
    for value, name in [(184, "flute"), (185, "lyre"), (186, "mandolin"), (187, "harp"), (188, "drums"), (189, "horn")]:
        standard = choose_instrument(metrics, value, "standard", bard_id)
        alternative = choose_instrument(metrics, value, "enhanceable", bard_id)
        if standard:
            instruments_standard.append({"instrument_type": name, "vnum": standard["vnum"], "name": standard["name"], "enhanceable": bool(standard["enhanceable"]), "observed_players": standard["observed_players"], "reason": "legendary series preferred for standard profile; quest-item status is an explicit fundamental exception when applicable"})
        if alternative:
            instruments_alt.append({"instrument_type": name, "vnum": alternative["vnum"], "name": alternative["name"], "enhanceable": True, "observed_players": alternative["observed_players"], "reason": "strict boot enhance-index alternative"})
    standard_totem = choose_totem(metrics, "standard", shaman_id)
    alt_totem = choose_totem(metrics, "enhanceable", shaman_id)
    return {
        "standard": {"spellbook": standard_book_row, "bard_instruments": instruments_standard, "shaman_totem": {"vnum": standard_totem["vnum"], "name": standard_totem["name"], "enhanceable": bool(standard_totem["enhanceable"]), "reason": "value0 mask 63: all three high-circle spheres"} if standard_totem else None},
        "enhanceable": {"spellbook": alt_book_row, "bard_instruments": instruments_alt, "shaman_totem": {"vnum": alt_totem["vnum"], "name": alt_totem["name"], "enhanceable": True, "reason": "value0 mask 63 and strict enhance-index eligibility"} if alt_totem else None},
    }


def c_string(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def emit_header(path: Path, analysis: dict[str, Any], catalog: dict[str, Any], repo_root: Path) -> None:
    class_ids = analysis["class_ids"]
    lines = [
        "#ifndef CHAOS_EQ_DATA_H",
        "#define CHAOS_EQ_DATA_H",
        "",
        "#include \"core/defines.h\"",
        "",
        "/* Generated by scripts/chaos_eq_catalog.py; do not edit by hand. */",
        "struct chaos_kit_item { int slot; int vnum; };",
        "struct chaos_eq_profile { const chaos_kit_item *items; };",
        "",
    ]
    profile_arrays: dict[str, dict[str, str]] = {"standard": {}, "enhanceable": {}}
    for profile in ("standard", "enhanceable"):
        matrix = catalog["profiles"][profile]
        for class_name, class_id in sorted(class_ids.items(), key=lambda item: int(item[1])):
            array_name = f"chaos_eq_{profile}_{identifier(class_name)}"
            profile_arrays[profile][class_name] = array_name
            lines.append(f"static const chaos_kit_item {array_name}[] = {{")
            for item in matrix[class_name]["equipment"] + matrix[class_name]["support_items"]:
                lines.append(f"    {{ {item['slot']}, {item['vnum']} }},")
            lines.append("    { WEAR_NONE, 0 }")
            lines.append("};")
            lines.append("")
    for profile in ("standard", "enhanceable"):
        optional_name = f"chaos_eq_{profile}_optional_slots"
        lines.append(f"static const chaos_kit_item {optional_name}[] = {{")
        for item in catalog["optional_race_slot_variations"][profile]:
            if item.get("status") == "available":
                lines.append(f"    {{ {item['slot']}, {item['vnum']} }},")
        lines.append("    { WEAR_NONE, 0 }")
        lines.append("};")
        lines.append("")
    lines.append("static const chaos_kit_item chaos_eq_support_consumables[] = {")
    for item in catalog["consumables"]:
        for _ in range(int(item["count"])):
            lines.append(f"    {{ WEAR_NONE, {item['vnum']} }},")
    lines.append("    { WEAR_NONE, 0 }")
    lines.append("};")
    lines.append("")
    lines.append("static const chaos_eq_profile chaos_eq_profiles[CLASS_COUNT + 1][2] = {")
    lines.append("    { { NULL }, { NULL } },")
    for class_name, class_id in sorted(class_ids.items(), key=lambda item: int(item[1])):
        lines.append(f"    {{ {{ {profile_arrays['standard'][class_name]} }}, {{ {profile_arrays['enhanceable'][class_name]} }} }},")
    lines.append("};")
    lines.append("")
    lines.append("#endif")
    lines.append("")
    path.write_text("\n".join(lines), encoding="utf-8")
    formatter = shutil.which("clang-format")
    style_file = repo_root / ".clang-format"
    if formatter and style_file.is_file():
        subprocess.run(
            [formatter, f"--style=file:{style_file}", "-i", str(path)],
            check=True,
        )


def build_catalog(analysis: dict[str, Any]) -> dict[str, Any]:
    metrics = {int(item["vnum"]): item for item in analysis["candidates"]}
    fundamentals = build_fundamentals(analysis, metrics)
    profiles: dict[str, dict[str, dict[str, Any]]] = {}
    for profile in ("standard", "enhanceable"):
        profiles[profile] = class_matrix(analysis, metrics, profile, fundamentals[profile])
    consumables = choose_support_consumables(analysis)
    optional = {
        "standard": choose_optional_variations(analysis, "standard", metrics),
        "enhanceable": choose_optional_variations(analysis, "enhanceable", metrics),
    }
    common = {profile: common_by_slot(profiles[profile]) for profile in profiles}
    return {
        "schema_version": 1,
        "source": {
            "analysis_schema_version": analysis.get("schema_version"),
            "cohort": analysis.get("cohort"),
            "class_ids": analysis.get("class_ids", {}),
            "selection_policy": {"max_item_risk": MAX_ITEM_RISK, "core_slots": list(CORE_SLOTS)},
            "note": "equipment profiles are aggregate/template-level; optional support and race-slot variations are separated",
        },
        "fundamentals": fundamentals,
        "consumables": consumables,
        "profiles": profiles,
        "common_by_slot": common,
        "optional_race_slot_variations": optional,
        "diagnostics": {
            "missing_equipment_by_profile": {
                profile: {
                    class_name: [decision for decision in row["decisions"] if decision["status"] == "missing"]
                    for class_name, row in profiles[profile].items()
                    if any(decision["status"] == "missing" for decision in row["decisions"])
                }
                for profile in profiles
            },
            "classes": len(analysis.get("class_ids", {})),
            "profiles": len(profiles),
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--analysis", required=True)
    parser.add_argument("--output-dir", required=True)
    parser.add_argument("--header-out")
    parser.add_argument("--repo-root", default=".")
    args = parser.parse_args()
    analysis = json.loads(Path(args.analysis).read_text(encoding="utf-8"))
    catalog = build_catalog(analysis)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    (output_dir / "catalog.json").write_text(json.dumps(catalog, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if args.header_out:
        emit_header(Path(args.header_out), analysis, catalog, Path(args.repo_root).resolve())
    print(json.dumps({
        "catalog": str((output_dir / "catalog.json").resolve()),
        "classes": catalog["diagnostics"]["classes"],
        "standard_profiles": len(catalog["profiles"]["standard"]),
        "enhanceable_profiles": len(catalog["profiles"]["enhanceable"]),
        "consumable_families": len(catalog["consumables"]),
        "standard_fundamentals": sum(1 for value in catalog["fundamentals"]["standard"].values() if value),
        "enhanceable_fundamentals": sum(1 for value in catalog["fundamentals"]["enhanceable"].values() if value),
    }, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
