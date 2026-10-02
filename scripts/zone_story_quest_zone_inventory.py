#!/usr/bin/env python3
"""Inventory active area evidence; candidate links are never runtime prerequisites."""
import argparse
import collections
import pathlib
import re

import zone_story_quest_catalog as catalog_tool


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
    mobs, items = prototypes(root, zones, "mob"), prototypes(root, zones, "obj")
    blocks = native_blocks(root)
    by_contract = {(b["giver_vnum"], b["binding"]["completion_key"]): b for b in blocks if "binding" in b}
    definitions = collections.defaultdict(list)
    dialogue = collections.defaultdict(list)
    specials = collections.defaultdict(set)

    def owner(vnum):
        return next((z["source_area"] for z in zones if z["first_vnum"] <= vnum <= z["last_vnum"]), None)

    for definition in catalog["definitions"]:
        definitions[definition["source_area"]].append(definition)
    for block in blocks:
        if block["kind"] in {"M", "MA"} and block["body"]:
            aliases = block["body"][0].split("~")[0].lower().split()
            if aliases and all(re.fullmatch(r"[a-z0-9_-]{1,64}", a) for a in aliases) and "qc_action" not in aliases:
                dialogue[owner(block["giver_vnum"])].append(block)
    assignments = (root / "src/specs/specs.assign.c").read_text()
    for pattern in (r"mob_index\[real_mobile0\((\d+)\)\]\.func\.mob\s*=\s*(\w+)", r"obj_index\[real_object0\((\d+)\)\]\.func\.obj\s*=\s*(\w+)", r"world\[real_room0\((\d+)\)\]\.funct\s*=\s*(\w+)"):
        for vnum, function in re.findall(pattern, assignments):
            if function not in {"0", "NULL", "nullptr"}:
                specials[owner(int(vnum))].add(function)
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
        rows.append({"zone": zone, "requests": requests, "dialogue": dialogue[area], "specials": sorted(specials[area]), "candidate_link_items": sorted(links), "mapped": (root / "areas/story" / f"{area}.story.json").is_file()})
    return rows, mobs, items


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
    args = parser.parse_args()
    result = markdown(args.source_root.resolve())
    if args.output:
        args.output.write_bytes(result.encode("utf-8"))
    else:
        print(result, end="")
