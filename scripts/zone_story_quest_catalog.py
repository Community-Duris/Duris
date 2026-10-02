#!/usr/bin/env python3

import argparse
import hashlib
import json
import math
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
MAX_DURABLE_ITEM_OFFERINGS = 14


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


def zone_registry(source_root):
    zones = []
    for line in (source_root / "areas/AREA").read_text().splitlines():
        if not line.strip() or line.lstrip().startswith("*"): continue
        stem = line.split()[0]
        path = source_root / "areas/zon" / f"{stem}.zon"
        if not path.is_file(): continue
        lines = path.read_text(errors="replace").splitlines()
        header = next((i for i, text in enumerate(lines) if re.fullmatch(r"#-?\d+", text.strip())), None)
        if header is None: continue
        number = int(lines[header].strip()[1:])
        if number >= 2**31: number -= 2**32
        name_lines = []
        i = header + 1
        while i < len(lines):
            name_lines.append(lines[i].split("~")[0])
            if "~" in lines[i]: break
            i += 1
        terms = lines[i + 1].split()
        zones.append({"zone_number": number, "name": re.sub(r"&(?:\+[A-Za-z]|[A-Za-z0-9])", "", " ".join(name_lines)).strip(), "source_area": stem,
                      "last_vnum": int(terms[0]), "reset_mode": int(terms[1]), "discoverable": number > 0})
    zones.sort(key=lambda zone: zone["zone_number"])
    previous = -1
    for zone in zones:
        zone["first_vnum"] = previous + 1
        previous = zone["last_vnum"]
    return zones


def production_catalog(source_root, content_revision=2):
    """Build the eligible catalog from active legacy static/story qst sources.

    Bartender/random world quests are intentionally absent: they are generated
    at runtime and have no stable zone-story definition identity.
    """
    definitions = []
    zones = zone_registry(source_root)
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
            owner = next((zone for zone in zones if zone["first_vnum"] <= giver_vnum <= zone["last_vnum"]), None)
            if owner is None or owner["zone_number"] < 0: continue
            zone_number = owner["zone_number"]
            repeatable = not block["disappear"] or owner["reset_mode"] != 0
            give, receive = sorted(block["give"]), sorted(block["receive"])
            item_offering = any(kind == "I" and number > 0 for kind, number in give)
            returns_offering = any(kind == "I" and (kind, number) in receive for kind, number in give)
            durable_offering = len(give) <= MAX_DURABLE_ITEM_OFFERINGS and all(kind == "I" and number > 0 for kind, number in give)
            daily = zone_number > 0 and repeatable and item_offering and not returns_offering and durable_offering
            exclusion = "" if daily else ("Administrative content" if zone_number <= 0 else
                "Story-only quest" if not repeatable else "Item exchange" if returns_offering else
                "No repeatable item offering" if not item_offering else "Unsupported durable offering")
            definitions.append(
                {
                    "definition_id": f"zone-story:qst:{giver_vnum}:{encoded_key}",
                    "source_system": "zone_story",
                    "zone_number": zone_number,
                    "source_area": owner["source_area"],
                    "giver_vnum": giver_vnum,
                    "completion_key": encoded_key,
                    "active": True,
                    "eligible_for_zone_completion": zone_number > 0,
                    "repeatable": repeatable,
                    "daily_eligible": daily,
                    "daily_exclusion": exclusion,
                    "prerequisites": [],
                    "content_revision": content_revision,
                }
            )

    definitions.sort(key=lambda item: (item["zone_number"], item["definition_id"]))
    fingerprint_payload = json.dumps(definitions, sort_keys=True, separators=(",", ":")).encode()
    result = {
        "schema_version": 1,
        "content_revision": content_revision,
        "source": {
            "kind": "legacy_static_qst",
            "area_list": "areas/AREA",
            "excludes": ["bartender_random_world_quests"],
            "fingerprint_sha256": hashlib.sha256(fingerprint_payload).hexdigest(),
        },
        "zones": [zone for zone in zones if zone["zone_number"] >= 0],
        "definitions": definitions,
    }
    result["story_mappings"] = []
    for zone in zones:
        path = source_root / "areas/story" / f"{zone['source_area']}.story.json"
        if not path.exists():
            continue
        if not path.is_file():
            raise ValueError(f"{path}: unreadable story mapping")
        if path.stat().st_size > 256 * 1024:
            raise ValueError(f"{path}: story mapping exceeds 256 KiB")
        mapping = json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=unique_fields)
        if not isinstance(mapping, dict) or mapping.get("source_area") != zone["source_area"]:
            raise ValueError(f"{path}: source_area must match its filename")
        result["story_mappings"].append(mapping)
    items, mobs = set(), {}
    for zone in zones:
        path = source_root / "areas/obj" / f"{zone['source_area']}.obj"
        if path.is_file():
            items.update(int(match[1]) for line in path.read_text(errors="replace").splitlines()
                         if (match := QUEST_BLOCK_RE.fullmatch(line.strip())) and int(match[1]) > 0)
        path = source_root / "areas/mob" / f"{zone['source_area']}.mob"
        if path.is_file():
            for match in re.finditer(r"^#(\d+)\s*\n([^~]*)~", path.read_text(errors="replace"), re.M):
                mobs[int(match[1])] = match[2].lower().split()
    story_units(result, items, mobs)
    return result


def unique_fields(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON field: {key}")
        result[key] = value
    return result


def story_units(catalog, item_vnums=None, mob_keywords=None):
    """Validate sidecars and project native receipts without changing their IDs."""
    definitions = {d["definition_id"]: d for d in catalog["definitions"]}
    references = {(d["giver_vnum"], d["completion_key"]): d for d in catalog["definitions"]}
    areas = {z["source_area"]: z for z in catalog.get("zones", []) if z["discoverable"]}
    units, bound, mapped_areas, story_ids = [], set(), set(), set()

    def fields(obj, keys):
        if not isinstance(obj, dict) or set(obj) != set(keys):
            raise ValueError("story mapping: unknown, duplicate, or missing fields")

    def text(obj, key, empty=False):
        value = obj[key]
        if (not isinstance(value, str) or (not empty and not value) or len(value.encode("utf-8")) > 1024
                or any(ord(c) < 32 or ord(c) == 127 or c == "$" for c in value)):
            raise ValueError(f"story mapping: invalid text {key}")
        return value

    def number(value, low, high):
        if (isinstance(value, bool) or not isinstance(value, (int, float))
                or not low <= value <= high or (isinstance(value, float) and not math.isfinite(value))
                or value != int(value)):
            raise ValueError("story mapping: integer outside supported range")
        return int(value)

    def array(value, low, high):
        if not isinstance(value, list) or not low <= len(value) <= high:
            raise ValueError("story mapping: array outside supported bounds")
        return value

    def refs(value, zone=None):
        result = []
        for ref in array(value, 1, 4096):
            fields(ref, {"giver_vnum", "completion_key"})
            giver = number(ref["giver_vnum"], 1, 2**31 - 1)
            key = text(ref, "completion_key").encode().hex()
            definition = references.get((giver, key))
            if not definition or not definition["active"] or (zone is not None and definition["zone_number"] != zone):
                raise ValueError("story mapping: unknown or wrong-area native contract")
            if definition["definition_id"] in result:
                raise ValueError("story mapping: duplicate native contract")
            result.append(definition["definition_id"])
        return result

    def bind(ids):
        for id in ids:
            if id in bound:
                raise ValueError("story mapping: multiply bound native contract")
            bound.add(id)

    for mapping in array(catalog.get("story_mappings", []), 0, 350):
        if not isinstance(mapping, dict):
            raise ValueError("story mapping: expected object")
        schema = number(mapping.get("schema_version"), 1, 2)
        keys = {"schema_version", "revision", "source_area", "coverage", "stories", "exclusions"}
        if schema == 2:
            keys |= {"introduction", "orientation", "contacts"}
        fields(mapping, keys)
        number(mapping["revision"], 1, 2**31 - 1)
        area = text(mapping, "source_area")
        if not re.fullmatch(r"[a-z0-9_-]{1,64}", area) or area not in areas or area in mapped_areas:
            raise ValueError("story mapping: invalid or duplicate source_area")
        mapped_areas.add(area)
        zone = areas[area]["zone_number"]
        if text(mapping, "coverage") not in {"partial", "complete"}:
            raise ValueError("story mapping: coverage must be partial or complete")
        if schema == 2:
            text(mapping, "introduction")
            for action in array(mapping["orientation"], 0, 16):
                text({"action": action}, "action")
            contacts = set()
            for contact in array(mapping["contacts"], 0, 256):
                fields(contact, {"mob_vnum", "name", "keyword", "description", "topics"})
                vnum = number(contact["mob_vnum"], 1, 2**31 - 1)
                keyword = text(contact, "keyword")
                text(contact, "name")
                text(contact, "description")
                if vnum in contacts or not re.fullmatch(r"[a-z0-9_-]{1,64}", keyword):
                    raise ValueError("contact: duplicate NPC or invalid command keyword")
                if mob_keywords is not None and (vnum not in mob_keywords or keyword not in mob_keywords[vnum]):
                    raise ValueError("contact: unknown NPC prototype or command alias")
                contacts.add(vnum)
                topics = set()
                for topic in array(contact["topics"], 0, 32):
                    text({"topic": topic}, "topic")
                    if not re.fullmatch(r"[a-z0-9_-]{1,64}", topic) or topic in topics:
                        raise ValueError("contact: invalid or duplicate topic")
                    topics.add(topic)
        for story in array(mapping["stories"], 0, 256):
            fields(story, {"id", "title", "category", "summary", "contracts", "steps"})
            slug = text(story, "id")
            id = f"zone-story:story:{area}:{slug}"
            if not re.fullmatch(r"[a-z0-9_-]{1,64}", slug) or id in story_ids:
                raise ValueError("story mapping: invalid or duplicate story id")
            story_ids.add(id)
            text(story, "title")
            text(story, "summary")
            if text(story, "category") not in {"story", "request", "service"}:
                raise ValueError("story mapping: unsupported category")
            ids = refs(story["contracts"], zone)
            bind(ids)
            steps = set()
            for step in array(story["steps"], 1, 32):
                kind = step.get("kind") if isinstance(step, dict) else None
                keys = {"id", "text", "kind", "hint"}
                if kind == "completion":
                    keys |= {"contracts"}
                elif kind in {"carried_item", "equipped_item"}:
                    keys |= {"item_vnums", "count"}
                    if kind == "equipped_item":
                        keys.add("slot")
                else:
                    raise ValueError("story mapping: unsupported step kind")
                fields(step, keys)
                step_id = text(step, "id")
                if not re.fullmatch(r"[a-z0-9_-]{1,64}", step_id) or step_id in steps:
                    raise ValueError("story mapping: invalid or duplicate step id")
                steps.add(step_id)
                text(step, "text")
                text(step, "hint", empty=True)
                if kind == "completion":
                    refs(step["contracts"])
                else:
                    number(step["count"], 1, 3000)
                    if kind == "equipped_item":
                        number(step["slot"], -1, 42)
                    items = [number(v, 1, 2**31 - 1) for v in array(step["item_vnums"], 1, 64)]
                    if len(set(items)) != len(items) or (item_vnums is not None and not set(items) <= item_vnums):
                        raise ValueError("story mapping: duplicate or unknown item prototype")
            native = [definitions[id] for id in ids]
            units.append({"id": id, "zone_number": zone, "contracts": ids,
                          "achievement": story["category"] != "service" and any(d["eligible_for_zone_completion"] for d in native),
                          "daily_candidate": story["category"] != "service" and any(d["daily_eligible"] for d in native)})
        for exclusion in array(mapping["exclusions"], 0, 256):
            fields(exclusion, {"reason", "contracts"})
            text(exclusion, "reason")
            bind(refs(exclusion["contracts"], zone))
        if mapping["coverage"] == "complete" and any(d["active"] and d["source_area"] == area and d["definition_id"] not in bound for d in definitions.values()):
            raise ValueError("complete story mapping leaves a native contract unclassified")
    for id, d in definitions.items():
        if d["active"] and id not in bound:
            units.append({"id": id, "zone_number": d["zone_number"], "contracts": [id],
                          "achievement": d["eligible_for_zone_completion"], "daily_candidate": d.get("daily_eligible", False)})
    return units


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
        if not isinstance(definition["zone_number"], int) or definition["zone_number"] < 0:
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
        if definition.get("daily_eligible") and (not definition["repeatable"] or not definition["active"]):
            diagnostics.append(diagnostic(index, "invalid_daily_eligibility", definition_id))

    if not diagnostics and "story_mappings" in catalog:
        try:
            story_units(catalog)
        except (ValueError, KeyError, TypeError) as error:
            diagnostics.append(diagnostic(-1, "invalid_story_mapping", str(error)))
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

    units = story_units(catalog) if not diagnostics else []
    if not diagnostics:
        eligible_by_zone = {}
        for unit in units:
            if unit["achievement"]:
                zone = str(unit["zone_number"])
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
        "story_unit_count": len(units),
        "daily_unit_count": sum(unit["daily_candidate"] for unit in units),
        "mapped_area_count": len(catalog.get("story_mappings", [])) if not diagnostics else 0,
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
    parser.add_argument("--content-revision", type=int, default=2)
    parser.add_argument("--json", action="store_true", dest="as_json")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--story-inventory", metavar="AREA",
                        help="export readable native contract bindings for an area; no inferred story dependencies")
    args = parser.parse_args()
    if bool(args.catalog) == bool(args.source_root):
        parser.error("provide exactly one of --catalog or --source-root")
    if args.content_revision <= 0:
        parser.error("--content-revision must be positive")
    try:
        catalog = (
            production_catalog(args.source_root.resolve(), args.content_revision)
            if args.source_root
            else load_catalog(args.catalog)
        )
    except (OSError, ValueError, KeyError, TypeError) as error:
        report = {"valid": False, "diagnostics": [diagnostic(-1, "invalid_story_mapping", str(error))]}
        print(json.dumps(report) if args.as_json else f"invalid: {error}")
        return 1
    if args.production_output:
        args.production_output.parent.mkdir(parents=True, exist_ok=True)
        args.production_output.write_text(json.dumps(catalog, indent=2, sort_keys=True) + "\n",
                                           encoding="utf-8", newline="\n")
    report = report_for(catalog)
    if args.story_inventory:
        if not report["valid"] or not any(z["source_area"] == args.story_inventory for z in catalog.get("zones", [])):
            print("invalid: unknown area or invalid catalog", file=sys.stderr)
            return 1
        entries = [d for d in catalog["definitions"] if d["source_area"] == args.story_inventory]
        print(json.dumps({"source_area": args.story_inventory, "contracts": [
            {"binding": {"giver_vnum": d["giver_vnum"], "completion_key": bytes.fromhex(d["completion_key"]).decode()},
             "daily_eligible": d.get("daily_eligible", False), "daily_exclusion": d.get("daily_exclusion", "")}
            for d in entries]}, indent=2))
    elif args.as_json:
        print(json.dumps(report, indent=2, sort_keys=True))
    else:
        print(f"valid: {report['valid']}")
        print(f"definitions: {report['definition_count']}")
        print(f"eligible_by_zone: {report['eligible_by_zone']}")
        print(f"mapped areas: {report['mapped_area_count']}; daily quest units: {report['daily_unit_count']}")
        for item in report["diagnostics"]:
            print(f"{item['code']}: {item['message']}")
    if args.check and not report["valid"]:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
