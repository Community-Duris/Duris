#!/usr/bin/env python3
"""Drifting Realm exact count/distinct proofs, real controls and foreign ownership."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import zone_story_quest_catalog as catalog_tool
import zone_story_quest_zone_inventory as inventory_tool

def bodies(area, kind):
    source = (ROOT / "areas" / kind / f"{area}.{kind}").read_text()
    return {int(m[1]): m[2] for m in re.finditer(r"^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)", source, re.M)}

def properties(body):
    lines = body.split("~", 4)[4].strip().splitlines()
    return [int(v) for v in lines[0].split()], [int(v) for v in lines[1].split()]

def stock(area):
    parent, result = None, []
    for row in inventory_tool.area_evidence(ROOT, area)["reset_commands"]:
        c, a = row["command"], row["arguments"]
        if c in {"M", "F"}:
            parent = a[1], a[3], row["line"]
        if c in {"G", "E"}:
            result.append((c, a[1], a[2], a[4], parent, row["line"]))
    return result

evidence = inventory_tool.area_evidence(ROOT, "dream")
catalog = catalog_tool.production_catalog(ROOT)
mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "dream")
assert (mapping["schema_version"], mapping["revision"], mapping["coverage"]) == (3, 1, "complete")
assert (len(mapping["stories"]), len(mapping["contacts"])) == (2, 14)
assert not mapping["exclusions"]
expected = {27: ([31311] * 4, 31313), 46: (list(range(31316, 31321)), 31315)}
bindings = {}
for request in evidence["requests"]:
    b = request["block"]
    items, reward = expected[b["line"]]
    assert (b["giver_vnum"], b["give"], b["receive"], b["disappear"]) == (31310, [("I", v) for v in items], [("I", reward)], False)
    assert request["definition"]["zone_number"] == 313 and not request["definition"]["prerequisites"]
    story = next(s for s in mapping["stories"] if s["contracts"] == [b["binding"]])
    bindings[b["line"]] = b["binding"]
    materials = [s for s in story["steps"] if s["kind"] == "carried_item"]
    assert [(s["item_vnums"], s["count"], s["optional"]) for s in materials] == [([v], n, True) for v, n in collections.Counter(items).items()]
    assert all(s.get("optional") for s in story["steps"][:-1])
    assert story["steps"][-1]["kind"] == "completion" and story["steps"][-1]["contracts"] == story["contracts"]
assert len(evidence["requests"]) == 2
assert mapping["stories"][1]["steps"][0]["contracts"] == [bindings[27]]
assert sum(len(s["steps"]) for s in mapping["stories"]) == 9
assert [b["body"][0] for b in evidence["dialogue"]] == ["hi help deliver boundless fantasies fantasy~", "yes quest~"]
assert sum(len(c["topics"]) for c in mapping["contacts"]) == 8
rooms, mobs, objects = (bodies("dream", k) for k in ("wld", "mob", "obj"))
assert (len(rooms), len(mobs), len(objects)) == (27, 18, 21)
assert set(rooms) == set(range(31300, 31327))
for c in mapping["contacts"]:
    assert c["keyword"] in mobs[c["mob_vnum"]].split("~")[0].split()
assert (evidence["zone"]["zone_number"], evidence["zone"]["first_vnum"], evidence["zone"]["last_vnum"], evidence["zone"]["reset_mode"]) == (313, 31261, 31326, 0)
assert len(evidence["reset_commands"]) == 90 and collections.Counter(r["command"] for r in evidence["reset_commands"]) == {"D": 18, "O": 8, "M": 50, "E": 7, "G": 7}
assert len((ROOT / "areas/zon/dream.zon").read_text().splitlines()) == 155
assert len((ROOT / "areas/qst/dream.qst").read_text().splitlines()) == 57
assert [s for s in stock("dream") if s[1] == 31311] == [
    ("G", 31311, 4, 100, (31307, 31309, 116), 117),
    ("G", 31311, 4, 100, (31306, 31315, 130), 131),
    ("G", 31311, 4, 100, (31308, 31316, 132), 133),
    ("G", 31311, 4, 100, (31305, 31317, 134), 135)]
for area, item, mob, room, ml, gl in (
    ("dream", 31316, 31300, 31318, 136, 138),
    ("mril", 31317, 33704, 33743, 88, 92),
    ("lava", 31318, 99713, 99759, 178, 180),
    ("arcium", 31319, 85747, 85811, 268, 272),
    ("fishermans_wharf", 31320, 88900, 88968, 136, 138)):
    assert ("G", item, 1, 100, (mob, room, ml), gl) in stock(area)
    header, _ = properties(objects[item])
    assert header[6] & 128 and header[6] & (1 << 23)  # Native NODROP and NORENT.
    assert "_proclib_" not in objects[item]
assert len({inventory_tool.plain(objects[v].split("~")[1]) for v in range(31316, 31321)}) == 1
for item, destination, command in ((31300, 31302, 15), (31301, 85805, 15), (31303, 85805, 264)):
    h, v = properties(objects[item]); assert h[0] == 25 and v[:3] == [destination, command, -1]
h, v = properties(objects[31302]); assert h[0] == 17 and v[:4] == [100, 100, 27, 0]
h, v = properties(objects[31304]); assert h[0] == 18 and v[1] == 100
h, v = properties(bodies("arcium", "obj")[85725]); assert h[0] == 25 and v[:3] == [31300, 7, -1]
edges = [(room, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for room, body in rooms.items()
         for m in re.finditer(r"\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)", body, re.S)]
assert len(edges) == 46
for start, end in ((31302, 31316), (31303, 31317), (31304, 31315), (31305, 31309)):
    assert (start, 1, 3, 31304, end) in edges and (end, 3, 3, 31304, start) in edges
assert (31301, 4, 0, 0, 12442) in edges
assert ("G", 83283, 1, 100, (31300, 31318, 136), 139) in stock("dream")
assert ("G", 83283, 1, 2, (83185, 83295, 2577), 2581) in stock("alatorin")
foreign = next(b for b in inventory_tool.native_blocks(ROOT) if b["source"] == "areas/qst/alatorin.qst" and b["line"] == 3646)
assert (foreign["giver_vnum"], foreign["give"], foreign["receive"], foreign["disappear"]) == (83244, [("I",32627),("I",83282),("I",83283),("I",83284)], [("I",83285)], True)
assert all(foreign["binding"] not in s["contracts"] for s in mapping["stories"])
shop = (ROOT / "areas/shp/dream.shp").read_text()
assert shop.startswith("#31310~\nN\n31314\n") and "\n31312\n" in shop
controller = (ROOT / "src/economy/shop.c").read_text()
assert "refuse_unported_shop_mutation" in controller and "economic_gameplay_authority::active()" in controller
loader = (ROOT / "src/world/db.c").read_text()
assert "ITEM_PROCLIB" in loader and '"_proclib_"' in loader
reset = (ROOT / "src/world/new_events.c").read_text()
assert re.search(r"if \(zone_table\[j\]\.reset_mode\)\s*\{\s*add_event\(event_reset_zone", reset)
assert catalog_tool.report_for(catalog)["eligible_by_zone"]["313"] == 2
print("Drifting Realm exact four-copy/five-identity returns, optional prior receipt, bound proofs, actual controls, foreign sources and mode-zero limits passed")
