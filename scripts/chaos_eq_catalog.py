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

# Deliberate starter roles, including melee hybrids; never infer from cohort size.
PHYSICAL_CLASSES = frozenset({
    "Warrior", "Ranger", "Paladin", "Anti-Paladin", "Monk", "Rogue", "Assassin",
    "Mercenary", "Bard", "Thief", "Berserker", "Reaver", "Dreadlord", "Avenger", "Dragoon",
})
WEAPON_SLOTS = frozenset({16, 17, 25, 26})
PERMANENT_STRIP_FLAGS = ("ITEM_TRANSIENT", "ITEM_NODROP", "ITEM_INVISIBLE",
                         "ITEM_SECRET", "ITEM_NOSHOW", "ITEM_BURIED", "ITEM_NORENT")
PERMANENT_POLICY = {"strip_extra_flags": list(PERMANENT_STRIP_FLAGS),
                    "strip_extra2_flags": ["ITEM2_CRUMBLELOOT"],
                    "strip_affects": ["APPLY_CURSE"]}
# Historical shared potion 1716 excludes Warrior. 80186 preserves its three
# spells (Hawkvision, Lionrage, Elephantstrength) for all classes at level 20
# rather than 40. Keep the seed unchanged as a record of the prior header.
SHARED_SUPPORT_OVERRIDES = {1716: 80186}
UTILITY_POLICY = [
    {"vnum": 336, "skill": "SKILL_FISHING", "count": 1, "role": "fishing pole"},
    {"vnum": 412, "skill": "SKILL_PICK_LOCK", "count": 1, "role": "lockpicks"},
    {"vnum": 73, "skill": "SKILL_TRAP", "count": 3, "role": "consumable huntsman traps"},
    {"vnum": 400227, "skill": "SKILL_SALVAGE", "count": 3,
     "role": "consumable scientific tools; runtime configured VNUM"},
]


def role_item_valid(metric: dict[str, Any], class_name: str, slot: int) -> bool:
    static = metric.get("static") or {}
    if class_name == "Monk" and (slot in WEAPON_SLOTS or
            int(metric.get("item_type", 0)) in {5, 6, 7} or
            int(static.get("wear_flags", 0)) & (1 << 13)):
        return False
    if class_name in PHYSICAL_CLASSES:
        affects = metric.get("effect_summary", {}).get("affects", {})
        if float(affects.get("wis_max", 0)) > 0:
            return False
        mental_bonus = sum(max(0, float(affects.get(name, 0))) for name in
                           ("int", "wis", "mana", "int_max", "pow_max", "cha_max", "pow", "cha"))
        if mental_bonus > 0 and physical_score(metric) == 0:
            return False
    return True


def physical_score(metric: dict[str, Any]) -> float:
    affects = metric.get("effect_summary", {}).get("affects", {})
    return sum(max(0, float(affects.get(name, 0))) * weight for name, weight in {
        "damroll": 5, "hitroll": 3, "hit": .15, "str": 2, "dex": 2, "agi": 2,
        "con": 2, "str_max": 3, "dex_max": 3, "agi_max": 3, "con_max": 3,
    }.items())


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
        if class_name == "Monk" and slot in WEAPON_SLOTS:
            continue
        entry = direct.get(slot)
        metric = metrics.get(int(entry["vnum"])) if entry else None
        choice_reason = "direct class/slot recommendation"
        if (not metric or not item_is_valid(metric, profile, class_id, slot) or
                not role_item_valid(metric, class_name, slot)):
            choices = [
                candidate for candidate in metrics.values()
                if item_is_valid(candidate, profile, class_id, slot)
                and role_item_valid(candidate, class_name, slot)
            ]
            choices.sort(key=lambda candidate: (
                physical_score(candidate) if class_name in PHYSICAL_CLASSES else 0,
                fallback_score(candidate, slot), candidate["vnum"]), reverse=True)
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
            "permanent_policy": PERMANENT_POLICY,
        }
        selected.append(item)
        decisions.append({
            "slot": slot,
            "status": "selected",
            "vnum": metric["vnum"],
            "reason": choice_reason,
        })
    if class_name in PHYSICAL_CLASSES:
        if any(decision["status"] == "missing" for decision in decisions):
            raise ValueError(f"{profile}/{class_name}: incomplete physical role equipment")
        globe_item = next(item for item in selected if item["slot"] == 3)
        globe_item["granted_bitvector2"] = "AFF2_GLOBE"
    return selected, decisions


def fundamental_exceptions(metric: dict[str, Any]) -> list[str]:
    allowed = {"item_transient", "item_norent", "item_noshow", "item_nosell", "quest_item"}
    return [reason for reason in metric.get("exclusion_reasons", []) if reason not in allowed]


def choose_book(
    metrics: dict[int, dict[str, Any]], profile: str, class_ids: dict[str, Any]
) -> dict[str, Any] | None:
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
        if not all(
            metric.get("class_eligible", {}).get(str(class_id), False)
            for class_id in class_ids.values()
        ):
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


def prepare_shared_consumables(analysis: dict[str, Any], metrics: dict[int, dict[str, Any]]) -> list[dict[str, Any]]:
    rows = analysis.get("seed_consumables") or choose_support_consumables(analysis)
    result = []
    seen: set[int] = set()
    for row in rows:
        source_vnum = int(row["vnum"])
        vnum = SHARED_SUPPORT_OVERRIDES.get(source_vnum, source_vnum)
        metric = metrics.get(vnum)
        if (not metric or fundamental_exceptions(metric) or not metric.get("race_portable") or
                not all(metric.get("class_eligible", {}).get(str(cid), False)
                        for cid in analysis["class_ids"].values())):
            raise ValueError(f"shared consumable {vnum}: not usable by every class and playable race")
        if metric["type_name"] != row["category"]:
            raise ValueError(f"shared consumable {vnum}: replacement changes support category")
        if vnum in seen:
            raise ValueError(f"shared consumable {vnum}: duplicate support family")
        seen.add(vnum)
        result.append({**row, "vnum": vnum, "name": metric["name"]})
        if source_vnum != vnum:
            result[-1]["replacement_for"] = source_vnum
            result[-1]["reason"] = f"all-class shared-support replacement for {source_vnum}"
            for key in ("observed_players", "median_quantity", "upper_quartile_quantity"):
                result[-1][key] = metric.get(key, 0)
    return result


def choose_optional_variations(analysis: dict[str, Any], profile: str, metrics: dict[int, dict[str, Any]]) -> list[dict[str, Any]]:
    class_ids = analysis.get("class_ids", {})
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
                if not role_item_valid(metric, "Warrior", slot):
                    continue
                if metric.get("exclusion_reasons") or not metric.get("race_portable"):
                    continue
                if profile == "enhanceable" and not metric.get("enhanceable"):
                    continue
                if not all(
                    metric.get("class_eligible", {}).get(str(class_id), False)
                    for class_id in class_ids.values()
                ):
                    continue
                if not (int((metric.get("static") or {}).get("wear_flags", 0)) & SLOT_WEAR_BITS.get(slot, 0)):
                    continue
                if float(metric.get("risk_score", 0.0)) > MAX_ITEM_RISK:
                    continue
                support = int(metric.get("slot_players", {}).get(str(slot), 0))
                candidates.append((support, metric.get("observed_players", 0), -float(metric.get("risk_score", 0.0)), metric))
            preferred = {item["vnum"] for item in analysis.get("seed_optional", {}).get(profile, [])
                         if item["slot"] == slot}
            candidates.sort(key=lambda item: (item[3]["vnum"] in preferred,
                            item[0], item[1], item[2], -item[3]["vnum"]), reverse=True)
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
    alt_book = choose_book(metrics, "enhanceable", class_ids)
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
        '#include "magic/spells.h"',
        "",
        "/* Generated by scripts/chaos_eq_catalog.py; do not edit by hand. */",
        "struct chaos_kit_item { int slot; int vnum; };",
        "struct chaos_eq_profile { const chaos_kit_item *items; };",
        "struct chaos_utility_item { int vnum; int skill; int count; };",
        "static const unsigned int chaos_eq_permanent_strip_flags = " + " | ".join(PERMANENT_STRIP_FLAGS) + ";",
        "static const unsigned int chaos_eq_permanent_strip_extra2_flags = ITEM2_CRUMBLELOOT;",
        "static const int chaos_eq_globe_slot = WEAR_NECK_1;",
        "",
    ]
    lines.append("static const bool chaos_eq_physical_classes[CLASS_COUNT + 1] = {")
    lines.append("    false,")
    for class_name, _ in sorted(class_ids.items(), key=lambda item: int(item[1])):
        lines.append(f"    {'true' if class_name in PHYSICAL_CLASSES else 'false'}, /* {class_name} */")
    lines.extend(["};", "", "static const chaos_utility_item chaos_eq_utility_items[] = {"])
    for item in catalog["utility_items"]:
        lines.append(f"    {{ {item['vnum']}, {item['skill']}, {item['count']} }},")
    lines.extend(["    { 0, 0, 0 }", "};", ""])
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
    fundamentals = analysis.get("seed_fundamentals") or build_fundamentals(analysis, metrics)
    profiles: dict[str, dict[str, dict[str, Any]]] = {}
    for profile in ("standard", "enhanceable"):
        profiles[profile] = class_matrix(analysis, metrics, profile, fundamentals[profile])
    consumables = prepare_shared_consumables(analysis, metrics)
    optional = {
        "standard": choose_optional_variations(analysis, "standard", metrics),
        "enhanceable": choose_optional_variations(analysis, "enhanceable", metrics),
    }
    common = {profile: common_by_slot(profiles[profile]) for profile in profiles}
    return {
        "schema_version": 2,
        "source": {
            "analysis_schema_version": analysis.get("schema_version"),
            "cohort": analysis.get("cohort"),
            "class_ids": analysis.get("class_ids", {}),
            "selection_policy": {"max_item_risk": MAX_ITEM_RISK, "core_slots": list(CORE_SLOTS),
                                 "physical_classes": sorted(PHYSICAL_CLASSES),
                                 "monk_excluded_slots": sorted(WEAPON_SLOTS),
                                 "physical_globe_slot": 3, "physical_globe_affect": "AFF2_GLOBE",
                                 "permanent_policy": PERMANENT_POLICY},
            "note": "equipment profiles are aggregate/template-level; optional support and race-slot variations are separated",
        },
        "fundamentals": fundamentals,
        "consumables": consumables,
        "utility_items": UTILITY_POLICY,
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


def emit_policy_report(path: Path, catalog: dict[str, Any]) -> None:
    lines = ["# Generated CHAOS starter catalog policy", "",
             "Generated by `scripts/chaos_eq_catalog.py`; do not edit by hand.", "",
             "Source: current active AREA templates and sanitized baseline slot/VNUM recommendations",
             "from `docs/data/chaos_eq_seed.json` (header at commit `5c3ee7a`). No player data,",
             "SQL access, live-game observations, or reconstructed observation counts are used.", "",
             "## Roles and permanence", "",
             "Physical membership (including melee hybrids): " + ", ".join(sorted(PHYSICAL_CLASSES)) + ".", "",
             "Monk excludes all weapon types, wieldable objects, and primary/secondary/third/fourth",
             "weapon slots. Physical profiles cover every core slot (except Monk weapons); runtime",
             "body-plan/class/dual-wield checks determine which slots actually apply to a character.",
             "Positive max-WIS is rejected for physical and shared optional gear. Mental-stat-only",
             "items without physical bonuses are rejected for physical roles; replacements rank",
             "damage, hitroll, hit points, strength, dexterity, agility and constitution first.", "",
             "Every physical profile grants `AFF2_GLOBE` in `bitvector2` of the first neck item.",
             "Active AREA prototypes have no permanent full-globe bit; this is an explicit generated",
             "starter-instance policy, not a prototype mutation or temporary spell/consumable.",
             "The neck slot is supported by every playable body plan, including Thri-Kreen.", "",
             "Permanent instance normalization strips " + ", ".join(f"`{flag}`" for flag in PERMANENT_STRIP_FLAGS) + ",",
             "`ITEM2_CRUMBLELOOT` and `APPLY_CURSE`. Intended magical/role flags remain, including",
             "wearer buffs (e.g. invisibility), distinct from item presentation flags. This normalization",
             "also applies to class fundamentals and tools. Normal food/charge/tool consumption remains.", "",
             "## Utilities", "", "| VNUM | Prospective skill | Count | Use |", "| --- | --- | --- | --- |"]
    for item in catalog["utility_items"]:
        lines.append(f"| {item['vnum']} | `{item['skill']}` | {item['count']} | {item['role']} |")
    lines.extend(["", "Eligibility uses class/race skill availability at CHAOS level 56 because the grant",
                  "precedes skill learning. Runtime deduplicates by resolved VNUM against equipment",
                  "and support; three trap/tool copies are intentional consumable quantities.",
                  "Fishing requires no bait; lockpicks and huntsman traps are self-contained. Scientific",
                  "tools use the configured crafting VNUM and are consumed by salvage. Existing CHAOS",
                  "material-pouch grants provide recipe materials; no disabled legacy forge hammer/",
                  "parchment path is enabled. Trap arming may subsequently set secret/decay flags as",
                  "part of ordinary skill use. Utility/support items may be stored in the bag; wearable",
                  "equipment arrives directly in inventory.", "", "Shared consumables must also be usable by every class and playable race.",
                  "Historical seed potion 1716 excludes Warrior. The explicit replacement is 80186",
                  "(three clear potions), preserving Hawkvision, Lionrage and Elephantstrength at",
                  "cast level 20 rather than 40. Both source restrictions and replacement category",
                  "are revalidated; no other restricted shared support is silently accepted.", "", "## Selected profiles", "",
                  "| Profile | Class | Core slots | Globe item VNUM | Slot:VNUM equipment |",
                  "| --- | --- | --- | --- | --- |"])
    for profile, matrix in catalog["profiles"].items():
        for name, row in matrix.items():
            equipment = row["equipment"]
            globe = next((str(i["vnum"]) for i in equipment if i.get("granted_bitvector2")), "none")
            pairs = ", ".join(f"{i['slot']}:{i['vnum']}" for i in equipment)
            lines.append(f"| {profile} | {name} | {len(equipment)} | {globe} | {pairs} |")
    lines.extend(["", "## Reproduction", "", "From the repository root with Python 3 and clang-format:", "", "```sh",
                  "python3 scripts/chaos_eq_catalog.py --static-seed docs/data/chaos_eq_seed.json --output-dir bin/chaos-catalog --header-out src/account/chaos_eq_data.h --policy-report-out docs/reference/CHAOS_KIT_CATALOG.md --repo-root .",
                  "python3 scripts/chaos_eq_validate.py --catalog bin/chaos-catalog/catalog.json --repo-root .",
                  "python3 tests/async/test_chaos_kit_policy.py", "```", ""])
    path.write_text("\n".join(lines), encoding="utf-8")


def static_analysis(seed: dict[str, Any], repo_root: Path) -> dict[str, Any]:
    """Revalidate a sanitized recommendation seed against active AREA templates.

    This is deliberately not reconstructed player/cohort evidence. All observed
    counts are zero, and scores/effects come from current source prototypes.
    """
    from chaos_eq_analyze import (
        ITEM_TYPE_NAMES, SAVE_APPLY_NAMES, STAT_APPLY_NAMES, area_file_names,
        clean_text, enhance_rejection_reasons, item_exclusion_reasons,
        make_effect_summary, object_class_allowed, object_race_portable,
        parse_defines, parse_enhance_config, parse_flag_maps, reconcile_area_objects,
    )
    constants = parse_defines(repo_root / "src/core/defines.h")
    objects, diagnostics = reconcile_area_objects(
        area_file_names(repo_root / "areas/obj", repo_root / "areas/AREA"), {}, constants)
    if diagnostics["parse_errors"]:
        raise ValueError(diagnostics["parse_errors"])
    enhance = parse_enhance_config(repo_root / "lib/enhance.cfg", constants)
    flag_names, _ = parse_flag_maps(constants)
    apply_names = {value: STAT_APPLY_NAMES.get(name, SAVE_APPLY_NAMES.get(name, name.lower()))
                   for name, value in constants.items() if name.startswith("APPLY_")}
    races = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 20, 30, 31, 32, 36, 37]
    candidates = []
    for obj in objects.values():
        summary = make_effect_summary(obj.affects, [], obj.bitvectors, flag_names, apply_names)
        exclusions = item_exclusion_reasons(obj, constants, races)
        if obj.ambiguous:
            exclusions.append("ambiguous_active_prototype")
        candidates.append({
            "vnum": obj.vnum, "name": clean_text(obj.short_description),
            "item_type": obj.object_type, "type_name": ITEM_TYPE_NAMES.get(obj.object_type, "other"),
            "observed_players": 0, "observed_share": 0, "slot_players": {},
            "effect_summary": summary, "power_score": summary["power_score"],
            "risk_score": summary["risk_score"], "exclusion_reasons": exclusions,
            "race_portable": object_race_portable(obj, races, constants)[0],
            "enhanceable": not enhance_rejection_reasons(obj, enhance, constants),
            "class_eligible": {str(cid): object_class_allowed(obj, int(cid), constants)
                               for cid in seed["class_ids"].values()},
            "static": {"wear_flags": obj.wear_flags, "extra_flags": obj.extra_flags,
                       "extra2_flags": obj.extra2_flags, "values": obj.values,
                       "bitvectors": obj.bitvectors, "affects": obj.affects},
            "reason": "current AREA prototype; no player observation evidence",
        })
    metrics = {item["vnum"]: item for item in candidates}
    fundamentals = {}
    for profile, chosen in seed["fundamentals"].items():
        book = metrics[chosen["spellbook"]]
        totem = metrics[chosen["shaman_totem"]]
        instruments = []
        for vnum in chosen["bard_instruments"]:
            item = metrics[vnum]
            instruments.append({"vnum": vnum, "name": item["name"],
                                "instrument_type": str(item["static"]["values"][0]),
                                "enhanceable": item["enhanceable"]})
        fundamentals[profile] = {
            "spellbook": {"vnum": book["vnum"], "name": book["name"],
                          "beltable": True, "enhanceable": book["enhanceable"]},
            "shaman_totem": {"vnum": totem["vnum"], "name": totem["name"],
                             "enhanceable": totem["enhanceable"]},
            "bard_instruments": instruments,
        }
    return {"schema_version": 2, "cohort": {"kind": "static AREA with baseline recommendation seed",
            "observed_characters": 0}, "class_ids": seed["class_ids"], "candidates": candidates,
            "recommendations": seed["recommendations"], "seed_fundamentals": fundamentals,
            "seed_consumables": [{**item, "name": metrics[item["vnum"]]["name"],
                "category": metrics[item["vnum"]]["type_name"]} for item in seed["consumables"]],
            "seed_optional": seed["optional"]}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--analysis")
    source.add_argument("--static-seed", help="sanitized baseline recommendations; no SQL or player access")
    parser.add_argument("--output-dir", required=True)
    parser.add_argument("--header-out")
    parser.add_argument("--policy-report-out")
    parser.add_argument("--repo-root", default=".")
    args = parser.parse_args()
    analysis = (static_analysis(json.loads(Path(args.static_seed).read_text(encoding="utf-8")),
                                Path(args.repo_root).resolve()) if args.static_seed else
                json.loads(Path(args.analysis).read_text(encoding="utf-8")))
    catalog = build_catalog(analysis)
    from chaos_eq_validate import validate
    issues = validate(catalog, Path(args.repo_root).resolve())
    if issues:
        raise ValueError("catalog validation failed: " + "; ".join(issues))
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    (output_dir / "catalog.json").write_text(json.dumps(catalog, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if args.header_out:
        emit_header(Path(args.header_out), analysis, catalog, Path(args.repo_root).resolve())
    if args.policy_report_out:
        emit_policy_report(Path(args.policy_report_out), catalog)
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
