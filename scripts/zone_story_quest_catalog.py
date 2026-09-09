#!/usr/bin/env python3

import argparse
import json
import pathlib
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
    parser.add_argument("--catalog", type=pathlib.Path, required=True)
    parser.add_argument("--json", action="store_true", dest="as_json")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    report = report_for(load_catalog(args.catalog))
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
