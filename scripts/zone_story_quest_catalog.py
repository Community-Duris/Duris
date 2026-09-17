#!/usr/bin/env python3

import argparse
import hashlib
import json
import pathlib
import re
import sys


REQUIRED_FIELDS = {
    "definition_id",
    "source_system",
    "zone_number",
    "source_area",
    "giver_vnum",
    "completion_key",
    "active",
    "eligible_for_zone_completion",
    "repeatable",
    "content_revision",
}

QUEST_BLOCK_RE = re.compile(r"^#(-?\d+)\s*$")
GOAL_RE = re.compile(r"^([GR])\s+([ITCSE])\s+(-?\d+)\s*$")


def active_quest_files(source_root):
    """Return qst files in the exact order consumed by make_qst.c.

    ``make_all`` runs ``make_qst`` with ``areas/`` as its working directory,
    so the compiler reads the top-level ``areas/AREA`` list.  The similarly
    named ``areas/qst/AREA`` file is a narrower historical list and is not
    the production static quest input.
    """
    areas_root = source_root / "areas"
    quest_root = areas_root / "qst"
    area_list = areas_root / "AREA"
    files = []
    for raw_line in area_list.read_text(encoding="utf-8", errors="replace").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("*"):
            continue
        parts = line.split()
        name = parts[0]
        path = quest_root / f"{name}.qst"
        if path.is_file():
            files.append(path)
    return files


def completion_key(give_goals, receive_goals, disappear):
    """Encode the legacy Q block identity without depending on prose text."""
    give = ",".join(f"{kind}:{number}" for kind, number in sorted(give_goals))
    receive = ",".join(f"{kind}:{number}" for kind, number in sorted(receive_goals))
    return f"give={give};receive={receive};disappear={int(disappear)}"


def production_catalog(source_root, content_revision=1):
    """Build the eligible catalog from active legacy static/story qst sources.

    Bartender/random world quests are intentionally absent: they are generated
    at runtime and have no stable zone-story definition identity.
    """
    definitions = []
    seen_contracts = set()
    for path in active_quest_files(source_root):
        current_giver = None
        current_block = None
        blocks = []

        def finish_block():
            nonlocal current_block
            if current_block is not None:
                blocks.append(current_block)
                current_block = None

        for raw_line in path.read_text(encoding="utf-8", errors="replace").splitlines():
            line = raw_line.strip()
            giver_match = QUEST_BLOCK_RE.match(line)
            if giver_match:
                finish_block()
                current_giver = int(giver_match.group(1))
                continue
            if line == "$":
                finish_block()
                current_giver = None
                continue
            if line in {"Q", "QA"}:
                finish_block()
                if current_giver is not None:
                    current_block = {
                        "giver_vnum": current_giver,
                        "give": [],
                        "receive": [],
                        "disappear": False,
                    }
                continue
            if line == "D" and current_block is not None:
                current_block["disappear"] = True
                continue
            goal_match = GOAL_RE.match(line)
            if goal_match and current_block is not None:
                group, kind, number = goal_match.groups()
                (current_block["give"] if group == "G" else current_block["receive"]).append(
                    (kind, int(number))
                )

        finish_block()
        for block in blocks:
            giver_vnum = block["giver_vnum"]
            key = completion_key(block["give"], block["receive"], block["disappear"])
            base_key = (giver_vnum, key)
            # Multiple Q blocks with the same giver and completion contract
            # are alternative prose/turn-in routes for one accomplishment.
            # Dedupe them so reordering those blocks cannot change identities
            # or inflate the denominator.
            if base_key in seen_contracts:
                continue
            seen_contracts.add(base_key)
            encoded_key = key.encode("utf-8").hex()
            # Runtime quest_data retains the giver VNUM but not the source
            # AREA filename.  Duris' zone namespace is the giver-vnum
            # hundred-block; the historical low-vnum heavens questers are
            # assigned to zone 1 explicitly.
            zone_number = max(1, giver_vnum // 100)
            definitions.append(
                {
                    "definition_id": f"zone-story:qst:{giver_vnum}:{encoded_key}",
                    "source_system": "zone_story",
                    "zone_number": zone_number,
                    "source_area": path.stem,
                    "giver_vnum": giver_vnum,
                    "completion_key": encoded_key,
                    "active": True,
                    "eligible_for_zone_completion": True,
                    "repeatable": True,
                    "content_revision": content_revision,
                }
            )

    definitions.sort(key=lambda item: (item["zone_number"], item["definition_id"]))
    fingerprint_payload = json.dumps(definitions, sort_keys=True, separators=(",", ":")).encode()
    return {
        "schema_version": 1,
        "content_revision": content_revision,
        "source": {
            "kind": "legacy_static_qst",
            "area_list": "areas/AREA",
            "excludes": ["bartender_random_world_quests"],
            "fingerprint_sha256": hashlib.sha256(fingerprint_payload).hexdigest(),
        },
        "definitions": definitions,
    }


def diagnostic(index, code, message):
    return {"index": index, "code": code, "message": message}


def load_catalog(path):
    with path.open(encoding="utf-8") as stream:
        return json.load(stream)


def validate_catalog(catalog):
    diagnostics = []
    if not isinstance(catalog, dict):
        diagnostics.append(diagnostic(-1, "invalid_catalog", "catalog must be an object"))
        return diagnostics

    if catalog.get("schema_version") != 1:
        diagnostics.append(diagnostic(-1, "unsupported_schema_version", "schema_version must be 1"))
    if not isinstance(catalog.get("content_revision"), int) or catalog["content_revision"] <= 0:
        diagnostics.append(diagnostic(-1, "invalid_content_revision", "content_revision must be positive"))

    definitions = catalog.get("definitions")
    if not isinstance(definitions, list):
        diagnostics.append(diagnostic(-1, "invalid_definitions", "definitions must be a list"))
        return diagnostics

    seen = set()
    for index, definition in enumerate(definitions):
        if not isinstance(definition, dict):
            diagnostics.append(diagnostic(index, "invalid_definition", "definition must be an object"))
            continue
        missing = sorted(REQUIRED_FIELDS - definition.keys())
        if missing:
            diagnostics.append(diagnostic(index, "missing_fields", ", ".join(missing)))
            continue
        definition_id = definition["definition_id"]
        if not isinstance(definition_id, str) or not definition_id:
            diagnostics.append(diagnostic(index, "invalid_definition_id", "definition_id must be non-empty"))
        elif definition_id in seen:
            diagnostics.append(diagnostic(index, "duplicate_definition_id", definition_id))
        else:
            seen.add(definition_id)
        if definition["source_system"] != "zone_story":
            diagnostics.append(diagnostic(index, "wrong_source_system", "source_system must be zone_story"))
        if not isinstance(definition["zone_number"], int) or definition["zone_number"] <= 0:
            diagnostics.append(diagnostic(index, "invalid_zone_number", "zone_number must be positive"))
        if not isinstance(definition["source_area"], str) or not definition["source_area"]:
            diagnostics.append(diagnostic(index, "invalid_source_area", "source_area must be non-empty"))
        if not isinstance(definition["giver_vnum"], int) or definition["giver_vnum"] <= 0:
            diagnostics.append(diagnostic(index, "invalid_giver_vnum", "giver_vnum must be positive"))
        if not isinstance(definition["completion_key"], str) or not definition["completion_key"]:
            diagnostics.append(diagnostic(index, "invalid_completion_key", "completion_key must be non-empty"))
        for field in ("active", "eligible_for_zone_completion", "repeatable"):
            if not isinstance(definition[field], bool):
                diagnostics.append(diagnostic(index, "invalid_boolean", field))
        if not isinstance(definition["content_revision"], int) or definition["content_revision"] <= 0:
            diagnostics.append(diagnostic(index, "invalid_definition_revision", "content_revision must be positive"))
        elif definition["content_revision"] != catalog.get("content_revision"):
            diagnostics.append(
                diagnostic(
                    index,
                    "revision_mismatch",
                    "definition revision does not match catalog revision",
                )
            )
        if definition["repeatable"] is not True:
            diagnostics.append(diagnostic(index, "non_repeatable_initial_scope", definition_id))

    return diagnostics


def report_for(catalog):
    diagnostics = validate_catalog(catalog)
    definitions = (
        catalog.get("definitions", [])
        if isinstance(catalog, dict) and isinstance(catalog.get("definitions"), list)
        else []
    )
    eligible_by_zone = {}
    repeatable_count = 0
    for definition in definitions:
        if not isinstance(definition, dict):
            continue
        if definition.get("repeatable") is True:
            repeatable_count += 1
        if (
            definition.get("active") is True
            and definition.get("eligible_for_zone_completion") is True
            and definition.get("content_revision") == catalog.get("content_revision")
        ):
            zone = str(definition.get("zone_number"))
            eligible_by_zone[zone] = eligible_by_zone.get(zone, 0) + 1

    ordered_definitions = sorted(
        definitions,
        key=lambda item: (
            item.get("zone_number", 0)
            if isinstance(item, dict) and isinstance(item.get("zone_number"), int)
            else 0,
            str(item.get("definition_id", "")) if isinstance(item, dict) else str(item),
        ),
    )
    return {
        "schema_version": catalog.get("schema_version") if isinstance(catalog, dict) else None,
        "content_revision": catalog.get("content_revision") if isinstance(catalog, dict) else None,
        "valid": not diagnostics,
        "definition_count": len(definitions),
        "eligible_by_zone": dict(sorted(eligible_by_zone.items())),
        "repeatable_definition_count": repeatable_count,
        "diagnostics": diagnostics,
        "definitions": ordered_definitions,
    }


def main():
    parser = argparse.ArgumentParser(description="Validate the zone-story quest catalog")
    parser.add_argument("--catalog", type=pathlib.Path)
    parser.add_argument("--source-root", type=pathlib.Path,
                        help="build a production catalog from areas/AREA and areas/qst")
    parser.add_argument("--production-output", type=pathlib.Path,
                        help="write the generated production catalog JSON")
    parser.add_argument("--content-revision", type=int, default=1)
    parser.add_argument("--json", action="store_true", dest="as_json")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if bool(args.catalog) == bool(args.source_root):
        parser.error("provide exactly one of --catalog or --source-root")
    if args.content_revision <= 0:
        parser.error("--content-revision must be positive")
    catalog = (
        production_catalog(args.source_root.resolve(), args.content_revision)
        if args.source_root
        else load_catalog(args.catalog)
    )
    if args.production_output:
        args.production_output.parent.mkdir(parents=True, exist_ok=True)
        args.production_output.write_text(json.dumps(catalog, indent=2, sort_keys=True) + "\n",
                                           encoding="utf-8")
    report = report_for(catalog)
    if args.as_json:
        print(json.dumps(report, indent=2, sort_keys=True))
    else:
        print(f"valid: {report['valid']}")
        print(f"definitions: {report['definition_count']}")
        print(f"eligible_by_zone: {report['eligible_by_zone']}")
        for item in report["diagnostics"]:
            print(f"{item['code']}: {item['message']}")
    if args.check and not report["valid"]:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
