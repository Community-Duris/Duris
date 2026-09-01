#!/usr/bin/env python3
"""Fail-closed validation for a generated Chaos equipment catalog."""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from chaos_eq_analyze import (  # noqa: E402
    area_file_names,
    enhance_rejection_reasons,
    item_exclusion_reasons,
    object_class_allowed,
    object_race_portable,
    parse_defines,
    parse_enhance_config,
    reconcile_area_objects,
)

# Keep the actual slot boundary local so this validator stays independent of
# analyzer implementation details.
RUNTIME_CORE_SLOTS = set(list(range(1, 18)) + list(range(19, 25)) + list(range(27, 31)))
PLAYABLE_RACE_IDS = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 17, 20, 30, 31, 32, 36, 37]
SLOT_WEAR_BITS = {
    **{slot: 1 << 1 for slot in (1, 2)},
    **{slot: 1 << 2 for slot in (3, 4)},
    5: 1 << 3, 6: 1 << 4, 7: 1 << 5, 8: 1 << 6, 9: 1 << 7,
    10: 1 << 8, 11: 1 << 9, 12: 1 << 10, 13: 1 << 11,
    14: 1 << 12, 15: 1 << 12, 16: 1 << 13, 17: 1 << 13,
    18: 1 << 14, 19: 1 << 17, 20: 1 << 18, 21: 1 << 19,
    22: 1 << 19, 23: 1 << 20, 24: 1 << 21, 25: 1 << 13,
    26: 1 << 13, 27: 1 << 22, 28: 1 << 23, 29: 1 << 23,
    30: 1 << 23, 31: 1 << 8, 32: 1 << 7, 33: 1 << 12,
    34: 1 << 12, 35: 1 << 24, 36: 1 << 5, 37: 1 << 25,
    38: 1 << 6, 39: 1 << 26, 40: 1 << 27, 41: 1 << 28, 42: 1 << 29,
}


def metric_basic_reasons(obj: Any, constants: dict[str, int], allow_fundamental: bool = False) -> list[str]:
    reasons = item_exclusion_reasons(obj, constants, PLAYABLE_RACE_IDS)
    if allow_fundamental:
        reasons = [reason for reason in reasons if reason not in {"item_transient", "item_norent", "item_noshow", "item_nosell", "quest_item"}]
    return reasons


def validate_item(
    item: dict[str, Any],
    profile: str,
    class_id: int | None,
    objects: dict[int, Any],
    constants: dict[str, int],
    enhance_config: dict[str, Any],
    allow_fundamental: bool = False,
) -> list[str]:
    vnum = int(item["vnum"])
    obj = objects.get(vnum)
    issues: list[str] = []
    if not obj:
        return [f"vnum {vnum}: missing from active AREA object sources"]
    reasons = metric_basic_reasons(obj, constants, allow_fundamental)
    if reasons:
        issues.extend(f"vnum {vnum}: base exclusion {reason}" for reason in reasons)
    if class_id is not None and not object_class_allowed(obj, class_id, constants):
        issues.append(f"vnum {vnum}: class {class_id} cannot use object")
    portable, reason = object_race_portable(obj, PLAYABLE_RACE_IDS, constants)
    if not portable:
        issues.append(f"vnum {vnum}: not race-portable ({reason})")
    slot = int(item.get("slot", -1))
    if slot >= 0 and not (obj.wear_flags & SLOT_WEAR_BITS.get(slot, 0)):
        issues.append(f"vnum {vnum}: object is not wearable in runtime slot {slot}")
    if profile == "enhanceable":
        enhance_reasons = enhance_rejection_reasons(obj, enhance_config, constants)
        issues.extend(f"vnum {vnum}: boot enhance rejection {reason}" for reason in enhance_reasons)
    return issues


def catalog_referenced_vnums(catalog: dict[str, Any]) -> set[int]:
    """Collect every object VNUM selected by a generated catalog."""
    vnums: set[int] = set()
    for matrix in catalog.get("profiles", {}).values():
        for row in matrix.values():
            for item in row.get("equipment", []) + row.get("support_items", []):
                if item.get("vnum") is not None:
                    vnums.add(int(item["vnum"]))
    for fundamentals in catalog.get("fundamentals", {}).values():
        for key in ("spellbook", "shaman_totem"):
            item = fundamentals.get(key)
            if item and item.get("vnum") is not None:
                vnums.add(int(item["vnum"]))
        for item in fundamentals.get("bard_instruments", []):
            if item.get("vnum") is not None:
                vnums.add(int(item["vnum"]))
    for variations in catalog.get("optional_race_slot_variations", {}).values():
        for item in variations:
            if item.get("status") == "available" and item.get("vnum") is not None:
                vnums.add(int(item["vnum"]))
    for item in catalog.get("consumables", []):
        if item.get("vnum") is not None:
            vnums.add(int(item["vnum"]))
    return vnums


def validate(catalog: dict[str, Any], repo_root: Path) -> list[str]:
    constants = parse_defines(repo_root / "src/core/defines.h")
    area_root = repo_root / "areas/obj"
    area_list = repo_root / "areas/AREA"
    wiki: dict[int, dict[str, Any]] = {}
    objects, area_diag = reconcile_area_objects(area_file_names(area_root, area_list), wiki, constants)
    issues: list[str] = []
    referenced_vnums = catalog_referenced_vnums(catalog)
    for duplicate in area_diag.get("duplicate_vnums", []):
        vnum = int(duplicate.get("vnum", 0))
        if duplicate.get("ambiguous") and vnum in referenced_vnums:
            sources = ", ".join(str(source) for source in duplicate.get("sources", []))
            issues.append(
                f"catalog: VNUM {vnum}: ambiguous duplicate AREA prototypes ({sources})"
            )
    if area_diag["parse_errors"]:
        issues.extend(f"area parser: {error}" for error in area_diag["parse_errors"])
    enhance_config = parse_enhance_config(repo_root / "lib/enhance.cfg", constants)
    class_ids = {name: int(value) for name, value in catalog.get("source", {}).get("class_ids", {}).items()}
    if not class_ids:
        # catalog files made by the current generator carry class IDs at the
        # top-level profiles; recover the stable class order if needed.
        class_ids = {name: index + 1 for index, name in enumerate(catalog.get("profiles", {}).get("standard", {}))}
    if set(catalog.get("profiles", {})) != {"standard", "enhanceable"}:
        issues.append("catalog: expected standard and enhanceable profiles")
    for profile in ("standard", "enhanceable"):
        matrix = catalog.get("profiles", {}).get(profile, {})
        if len(matrix) != 30:
            issues.append(f"{profile}: expected 30 class profiles, found {len(matrix)}")
        for class_name, row in matrix.items():
            class_id = int(row.get("class_id", class_ids.get(class_name, 0)))
            seen_slots: set[int] = set()
            for item in row.get("equipment", []):
                slot = int(item.get("slot", -1))
                if slot not in RUNTIME_CORE_SLOTS:
                    issues.append(f"{profile}/{class_name}: invalid core slot {slot}")
                if slot in seen_slots:
                    issues.append(f"{profile}/{class_name}: duplicate core slot {slot}")
                seen_slots.add(slot)
                issues.extend(
                    f"{profile}/{class_name}: {issue}"
                    for issue in validate_item(item, profile, class_id, objects, constants, enhance_config)
                )
            for item in row.get("support_items", []):
                role = item.get("role", "support")
                issues.extend(
                    f"{profile}/{class_name}/{role}: {issue}"
                    for issue in validate_item(
                        item, profile, class_id, objects, constants, enhance_config, allow_fundamental=True
                    )
                )
                if int(item.get("vnum", 0)) == 1252:
                    issues.append(f"{profile}/{class_name}: placeholder VNUM 1252 present")
    for profile, fundamentals in catalog.get("fundamentals", {}).items():
        book = fundamentals.get("spellbook")
        if book:
            book_vnum = int(book["vnum"])
            book_obj = objects.get(book_vnum)
            attach_belt = constants.get("ITEM_ATTACH_BELT", 1 << 23)
            if not book_obj or not (book_obj.wear_flags & attach_belt):
                issues.append(f"{profile}: spellbook {book.get('vnum')} is not beltable")
            metric = {"vnum": book_vnum, "slot": -1}
            issues.extend(
                f"{profile}/spellbook: {issue}"
                for issue in validate_item(metric, profile, None, objects, constants, enhance_config, allow_fundamental=True)
            )
        for instrument in fundamentals.get("bard_instruments", []):
            metric = {"vnum": instrument["vnum"], "slot": -1}
            issues.extend(
                f"{profile}/instrument/{instrument.get('instrument_type')}: {issue}"
                for issue in validate_item(metric, profile, class_ids.get("Bard", 16), objects, constants, enhance_config, allow_fundamental=True)
            )
        totem = fundamentals.get("shaman_totem")
        if totem:
            metric = {"vnum": totem["vnum"], "slot": -1}
            issues.extend(
                f"{profile}/totem: {issue}"
                for issue in validate_item(metric, profile, class_ids.get("Shaman", 9), objects, constants, enhance_config, allow_fundamental=True)
            )
    for profile, variations in catalog.get("optional_race_slot_variations", {}).items():
        for variation in variations:
            if variation.get("status") != "available":
                if not variation.get("reason"):
                    issues.append(f"{profile}/optional/{variation.get('variation')}: unavailable variation lacks reason")
                continue
            item = {"vnum": variation["vnum"], "slot": variation["slot"]}
            issues.extend(
                f"{profile}/optional/{variation.get('variation')}: {issue}"
                for issue in validate_item(item, profile, None, objects, constants, enhance_config)
            )
    for item in catalog.get("consumables", []):
        if int(item.get("vnum", 0)) == 1252:
            issues.append("consumables: placeholder VNUM 1252 present")
        obj = objects.get(int(item.get("vnum", 0)))
        if not obj:
            issues.append(f"consumables: missing VNUM {item.get('vnum')}")
        elif metric_basic_reasons(obj, constants, allow_fundamental=True):
            issues.append(f"consumables: excluded VNUM {item.get('vnum')}")
    return sorted(set(issues))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", required=True)
    parser.add_argument("--repo-root", default=".")
    args = parser.parse_args()
    catalog = json.loads(Path(args.catalog).read_text(encoding="utf-8"))
    issues = validate(catalog, Path(args.repo_root).resolve())
    print(json.dumps({"status": "pass" if not issues else "fail", "issues": issues, "issue_count": len(issues)}, sort_keys=True))
    return 0 if not issues else 1


if __name__ == "__main__":
    raise SystemExit(main())
