#!/usr/bin/env python3
"""Inventory active area evidence; candidate links are never runtime prerequisites."""
import argparse
import collections
import json
import pathlib
import re

import zone_story_quest_catalog as catalog_tool


def special_assignments(source):
    """Find literal assignment chains without comments or quoted examples.

    Preprocessor branches remain source leads; this does not evaluate a build or
    discover assignments through arbitrary expressions, aliases, or generated code.
    """
    tokens = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/')
    source = tokens.sub(lambda m: re.sub(r"[^\n]", " ", m[0]), source)
    target = (r"(?:mob_index\s*\[\s*real_mobile0\s*\(\s*\d+\s*\)\s*\]\s*\.\s*func\s*\.\s*mob"
              r"|obj_index\s*\[\s*real_object0\s*\(\s*\d+\s*\)\s*\]\s*\.\s*func\s*\.\s*obj"
              r"|world\s*\[\s*real_room0\s*\(\s*\d+\s*\)\s*\]\s*\.\s*funct)")
    chains = re.compile(rf"(?P<targets>(?:{target}\s*=\s*)+)(?P<function>\w+)\s*;")
    targets = re.compile(r"(?P<kind>mob_index|obj_index|world)\s*\[\s*(?:real_mobile0|real_object0|real_room0)\s*\(\s*(?P<vnum>\d+)\s*\)\s*\]")
    result = []
    for chain in chains.finditer(source):
        if chain["function"] in {"0", "NULL", "nullptr"}:
            continue
        for match in targets.finditer(chain["targets"]):
            result.append({"kind": {"mob_index": "mob", "obj_index": "obj", "world": "room"}[match["kind"]],
                           "vnum": int(match["vnum"]), "function": chain["function"],
                           "source": "src/specs/specs.assign.c", "line": source.count("\n", 0, chain.start() + match.start()) + 1})
    return result


def plain(value):
    return re.sub(r"&(?:\+[A-Za-z]|[A-Za-z0-9])", "", " ".join(value.split())).strip()


def prototypes(root, zones, kind):
    result = {}
    for zone in zones:
        path = root / "areas" / kind / f"{zone['source_area']}.{kind}"
        if not path.is_file():
            continue
        for match in re.finditer(r"^#(\d+)\s*\n([^~]*)~\s*\n([^~]*)~", path.read_text(errors="replace"), re.M):
            result[int(match[1])] = {"keywords": match[2].lower().split(), "name": plain(match[3]), "source": str(path.relative_to(root)).replace("\\", "/")}
    return result


def native_blocks(root):
    blocks = []
    for path in catalog_tool.active_quest_files(root):
        giver, block = None, None

        def finish():
            nonlocal block
            if block is not None:
                if block["kind"] in {"Q", "QA"}:
                    block["binding"] = {"giver_vnum": giver, "completion_key": catalog_tool.completion_key(block["give"], block["receive"], block["disappear"])}
                blocks.append(block)
                block = None

        for line_number, raw in enumerate(path.read_text(errors="replace").splitlines(), 1):
            line = raw.strip()
            if match := catalog_tool.QUEST_BLOCK_RE.fullmatch(line):
                finish()
                giver = int(match[1])
            elif line in {"M", "MA", "Q", "QA", "S", "$"}:
                finish()
                if giver is not None and line in {"M", "MA", "Q", "QA"}:
                    block = {"kind": line, "giver_vnum": giver, "source": str(path.relative_to(root)).replace("\\", "/"), "line": line_number, "body": [], "give": [], "receive": [], "disappear": False}
            elif block is not None:
                if match := catalog_tool.GOAL_RE.fullmatch(line):
                    direction, kind, number = match.groups()
                    block["give" if direction == "G" else "receive"].append((kind, int(number)))
                elif line == "D" and block["kind"] in {"Q", "QA"}:
                    block["disappear"] = True
                else:
                    block["body"].append(raw)
        finish()
    return blocks


def inventory(root):
    catalog = catalog_tool.production_catalog(root)
    zones = catalog["zones"]
    # Administrative areas supply valid prototypes without owning discoverable
    # zones. Match the production catalog's full active prototype lookup.
    active_areas = catalog_tool.zone_registry(root)
    mobs, items = prototypes(root, active_areas, "mob"), prototypes(root, active_areas, "obj")
    blocks = native_blocks(root)
    by_contract = {(b["giver_vnum"], b["binding"]["completion_key"]): b for b in blocks if "binding" in b}
    definitions = collections.defaultdict(list)
    dialogue = collections.defaultdict(list)
    specials = collections.defaultdict(set)
    corrected_givers = {d["giver_vnum"] for d in catalog["definitions"] if "previous_zone_number" in d}
    giver_owners = collections.defaultdict(set)
    for definition in catalog["definitions"]:
        giver_owners[definition["giver_vnum"]].add(definition["source_area"])

    def owner(vnum):
        return next((z["source_area"] for z in zones if z["first_vnum"] <= vnum <= z["last_vnum"]), None)

    for definition in catalog["definitions"]:
        definitions[definition["source_area"]].append(definition)
    for block in blocks:
        if block["kind"] in {"M", "MA"} and block["body"]:
            aliases = block["body"][0].split("~")[0].lower().split()
            if aliases and all(re.fullmatch(r"[a-z0-9_-]{1,64}", a) for a in aliases) and "qc_action" not in aliases:
                giver = block["giver_vnum"]
                if giver in corrected_givers and len(giver_owners[giver]) != 1:
                    raise ValueError(f"quest dialogue: giver {giver} has conflicting reviewed owners")
                area = next(iter(giver_owners[giver])) if giver in corrected_givers else owner(giver)
                dialogue[area].append(block)
    assignments = special_assignments((root / "src/specs/specs.assign.c").read_text())
    area_assignments = collections.defaultdict(list)
    for assignment in assignments:
        prototype = {"mob": mobs, "obj": items}.get(assignment["kind"], {}).get(assignment["vnum"])
        area = pathlib.PurePosixPath(prototype["source"]).stem if prototype else owner(assignment["vnum"])
        area_assignments[area].append(assignment)
        specials[area].add(assignment["function"])
    rows = []
    for zone in zones:
        area = zone["source_area"]
        requests = []
        for definition in definitions[area]:
            key = bytes.fromhex(definition["completion_key"]).decode()
            block = by_contract[(definition["giver_vnum"], key)]
            requests.append({"definition": definition, "block": block})
        producers, consumers = collections.defaultdict(set), collections.defaultdict(set)
        for index, request in enumerate(requests):
            for kind, vnum in request["block"]["receive"]:
                if kind == "I" and vnum > 0:
                    producers[vnum].add(index)
            for kind, vnum in request["block"]["give"]:
                if kind == "I" and vnum > 0:
                    consumers[vnum].add(index)
        links = {v for v in producers.keys() & consumers.keys() if any(p != c for p in producers[v] for c in consumers[v])}
        rows.append({"zone": zone, "requests": requests, "dialogue": dialogue[area], "specials": sorted(specials[area]), "special_assignments": area_assignments[area], "candidate_link_items": sorted(links), "mapped": (root / "areas/story" / f"{area}.story.json").is_file()})
    return rows, mobs, items


def area_evidence(root, source_area):
    """Export exact sources for human review, without inferring a campaign."""
    rows, mobs, items = inventory(root)
    row = next((r for r in rows if r["zone"]["source_area"] == source_area), None)
    if row is None:
        raise ValueError(f"unknown active area: {source_area}")
    reset_path = root / "areas/zon" / f"{source_area}.zon"
    resets = []
    for line, raw in enumerate(reset_path.read_text(errors="replace").splitlines(), 1):
        if match := re.fullmatch(r"([A-Z])\s+((?:-?\d+\s*)+)(?:\*.*)?", raw):
            resets.append({"command": match[1], "arguments": [int(n) for n in match[2].split()],
                           "source": f"areas/zon/{source_area}.zon", "line": line})
    return {"schema_version": 1, "qualification": "source evidence only; no inferred prerequisites or gameplay proof",
            **row, "reset_commands": resets,
            "mobs": {v: p for v, p in mobs.items() if pathlib.PurePosixPath(p["source"]).stem == source_area},
            "items": {v: p for v, p in items.items() if pathlib.PurePosixPath(p["source"]).stem == source_area}}


def review_index(root, source_area):
    evidence = area_evidence(root, source_area)
    mapping_path = root / "areas/story" / f"{source_area}.story.json"
    mapping = json.loads(mapping_path.read_text(encoding="utf-8")) if mapping_path.is_file() else {}
    classifications = {}
    for story in mapping.get("stories", []):
        for ref in story["contracts"]:
            classifications[(ref["giver_vnum"], ref["completion_key"])] = story["category"] + ": " + story["title"]
    for exclusion in mapping.get("exclusions", []):
        for ref in exclusion["contracts"]:
            classifications[(ref["giver_vnum"], ref["completion_key"])] = "Excluded: " + exclusion["reason"]

    def cell(value):
        return plain(str(value)).replace("|", "\\|")

    def location(record):
        return f"[{record['source']}:{record['line']}](../../../{record['source']}#L{record['line']})"

    lines = [f"# {evidence['zone']['name']}: source review index", "",
             "Generated source evidence and current sidecar classification; this is not gameplay qualification.", "",
             "```bash", f"python3 scripts/zone_story_quest_zone_inventory.py --area-evidence {source_area} \\",
             f"  --evidence-format markdown --output docs/reference/zone-story-audits/{source_area}.md", "```", "",
             "## Native exchanges", "", "| Giver | Exact native terms | Classification | Source |", "| ---: | --- | --- | --- |"]
    for request in evidence["requests"]:
        block = request["block"]
        ref = block["binding"]
        classification = classifications.get((ref["giver_vnum"], ref["completion_key"]), "Native fallback; unreviewed")
        lines.append(f"| {ref['giver_vnum']} | `{ref['completion_key']}` | {cell(classification)} | {location(block)} |")
    if not evidence["requests"]:
        lines += ["", "No native Q contracts. Scripted outcomes require separate semantic review."]
    lines += ["", "## Dialogue responses", "", "Topics are source aliases, not recorded learned objectives.", "",
              "| Giver | Aliases | Source |", "| ---: | --- | --- |"]
    for block in evidence["dialogue"]:
        lines.append(f"| {block['giver_vnum']} | {cell(block['body'][0].split('~')[0])} | {location(block)} |")
    lines += ["", "## Literal special assignments", "", "Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.", "",
              "| Kind | VNUM | Procedure | Source |", "| --- | ---: | --- | --- |"]
    for assignment in evidence["special_assignments"]:
        lines.append(f"| {assignment['kind']} | {assignment['vnum']} | `{assignment['function']}` | {location(assignment)} |")
    counts = collections.Counter(r["command"] for r in evidence["reset_commands"])
    lines += ["", "## Reset coverage", "", f"{len(evidence['reset_commands'])} parsed reset commands: " + ", ".join(f"{cmd}: {count}" for cmd, count in sorted(counts.items())) + ".",
              "Full arguments, source locations, and prototypes are available with `--evidence-format json`.", "",
              "Reset declarations are possible sources; counts do not prove that admission, random rolls,",
              "population limits, conditional chains, or accounting permit them to execute.", ""]
    return "\n".join(lines)


def markdown(root):
    rows, mobs, items = inventory(root)
    quest_rows = [r for r in rows if r["requests"]]
    lines = ["# Active zone journal evidence inventory", "", "Generated from the authoritative `areas/AREA` list, native Q bindings, active prototypes,", "and literal special assignments. Counts describe static evidence, not gameplay qualification.", "Links between an exchange output and another input are review candidates; they do not", "prove mandatory order, personal sourcing, branches, or a scripted completion.", "", f"Scanned {len(rows)} catalog zones; {len(quest_rows)} have native Q contracts;", f"{sum(len(r['requests']) for r in rows)} distinct Q contracts; {sum(r['mapped'] for r in rows)} authored journals.", "", "Regenerate with:", "", "```bash", "python3 scripts/zone_story_quest_zone_inventory.py --output docs/reference/ZONE_STORY_ZONE_INVENTORY.md", "```", "", "The [priority roadmap](../design/ZONE_STORY_ZONE_PRIORITIES.md) supplies the reviewed order", "and proposed story families. Every unmapped area still has its native fallback journal", "when accounting is active and the area/giver has been discovered/encountered.", "", "## Areas with native quests", "", "Q = distinct native contracts; M = player-addressable dialogue response blocks;", "links = distinct items connecting separate local exchanges. Special names are inspection", "leads from literal assignments only, not an exhaustive script audit. Empty special lists", "do not establish the absence of shared, generated, or dynamically assigned procedures.", "", "| Area / source | Q | M | Links | Authored | Sample native offering → outcome | Assigned special leads |", "| --- | ---: | ---: | ---: | --- | --- | --- |"]

    def cell(value):
        return plain(str(value)).replace("|", "\\|")

    for row in sorted(quest_rows, key=lambda r: r["zone"]["source_area"]):
        zone = row["zone"]
        request = max(row["requests"], key=lambda r: (len(r["block"]["give"]), len(r["block"]["receive"])))
        block = request["block"]
        inputs = collections.Counter(v for k, v in block["give"] if k == "I" and v > 0)
        outputs = list(dict.fromkeys(v for k, v in block["receive"] if k == "I" and v > 0))
        example = "; ".join(f"{n} × {items.get(v, {}).get('name', 'item ' + str(v))}" for v, n in list(inputs.items())[:3]) or "native payment/conditions"
        if len(inputs) > 3:
            example += "; other required items"
        example += " → " + (", ".join(items.get(v, {}).get("name", "item " + str(v)) for v in outputs[:2]) or "native reward/response")
        source = f"../../{block['source']}#L{block['line']}"
        lines.append(f"| {cell(zone['name'])} (`{zone['source_area']}`) | {len(row['requests'])} | {len(row['dialogue'])} | {len(row['candidate_link_items'])} | {'Yes' if row['mapped'] else 'Fallback'} | [{cell(example)}]({source}) | {cell(', '.join(row['specials'][:6])) or '—'} |")
    lines += ["", "## Areas without native Q contracts", "", "These are not automatically empty of stories. Addressable dialogue and special assignments", "identify candidates for explicit semantic adapters. Do not invent turn-in achievements.", "", "| Area / source | M | Authored | Assigned special leads |", "| --- | ---: | --- | --- |"]
    for row in sorted((r for r in rows if not r["requests"]), key=lambda r: r["zone"]["source_area"]):
        zone = row["zone"]
        lines.append(f"| {cell(zone['name'])} (`{zone['source_area']}`) | {len(row['dialogue'])} | {'Yes' if row['mapped'] else 'No'} | {cell(', '.join(row['specials'][:6])) or '—'} |")
    return "\n".join(lines) + "\n"


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=pathlib.Path)
    parser.add_argument("--area-evidence", help="export a complete native/reset/special evidence inventory for one active area")
    parser.add_argument("--evidence-format", choices=("json", "markdown"), default="json")
    args = parser.parse_args()
    root = args.source_root.resolve()
    if args.area_evidence:
        result = (review_index(root, args.area_evidence) if args.evidence_format == "markdown" else
                  json.dumps(area_evidence(root, args.area_evidence), indent=2, ensure_ascii=False) + "\n")
    else:
        result = markdown(root)
    if args.output:
        args.output.write_bytes(result.encode("utf-8"))
    else:
        print(result, end="")
