#!/usr/bin/env python3
"""Clavikord independent exact-head returns, competing sources and real access."""
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
            result.append((c, a[1], a[2], a[3], a[4], parent, row["line"]))
    return result


evidence = inventory_tool.area_evidence(ROOT, "lizard")
catalog = catalog_tool.production_catalog(ROOT)
mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "lizard")
assert (mapping["schema_version"], mapping["revision"], mapping["coverage"]) == (3, 1, "complete")
assert (len(mapping["stories"]), len(mapping["contacts"])) == (2, 13)
assert not mapping["exclusions"]
expected = {14: (6013, 6008, 6009), 40: (6014, 6005, 6007)}
assert len(evidence["requests"]) == 2
for request in evidence["requests"]:
    b = request["block"]
    giver, proof, reward = expected[b["line"]]
    assert (b["giver_vnum"], b["give"], b["receive"], b["disappear"]) == (giver, [("I", proof)], [("I", reward)], True)
    assert request["definition"]["zone_number"] == 60 and not request["definition"]["prerequisites"]
    story = next(s for s in mapping["stories"] if s["contracts"] == [b["binding"]])
    assert len(story["steps"]) == 2
    material, receipt = story["steps"]
    assert (material["kind"], material["item_vnums"], material["count"], material["optional"]) == ("carried_item", [proof], 1, True)
    assert receipt["kind"] == "completion" and receipt["contracts"] == story["contracts"]
assert [b["body"][0] for b in evidence["dialogue"]] == ["quest charge~", "bemon~"]
assert sum(len(c["topics"]) for c in mapping["contacts"]) == 3
rooms, mobs, objects = (bodies("lizard", k) for k in ("wld", "mob", "obj"))
assert (len(rooms), len(mobs), len(objects)) == (120, 24, 14)
assert set(rooms) == set(range(6000, 6122)) - {6100, 6101}
for contact in mapping["contacts"]:
    assert contact["keyword"] in mobs[contact["mob_vnum"]].split("~")[0].split()
assert (evidence["zone"]["zone_number"], evidence["zone"]["first_vnum"], evidence["zone"]["last_vnum"], evidence["zone"]["reset_mode"]) == (60, 6000, 6121, 1)
assert len(evidence["reset_commands"]) == 214
assert collections.Counter(r["command"] for r in evidence["reset_commands"]) == {"D": 2, "O": 3, "M": 195, "F": 5, "E": 7, "G": 2}
assert len((ROOT / "areas/zon/lizard.zon").read_text().splitlines()) == 231
assert len((ROOT / "areas/qst/lizard.qst").read_text().splitlines()) == 52
assert ("G", 6008, 1, 0, 100, (6003, 6022, 26), 28) in stock("lizard")
assert ("G", 6005, 1, 0, 100, (6013, 6099, 191), 193) in stock("lizard")
assert ("E", 6003, 1, 14, 100, (6011, 6099, 179), 180) in stock("lizard")
assert ("G", 6008, 999, 0, 100, (100015, 105302, 15), 16) in stock("random")
rare = next(r for r in evidence["reset_commands"] if r["line"] == 194)
assert rare["command"] == "M" and rare["arguments"][1:5] == [6014, 1, 6099, 25]
for proof in (6005, 6008):
    header, _ = properties(objects[proof])
    assert header[0] == 12 and header[8] & 32768  # Exact authored QUESTITEM, not generic carved part.
for item, destination in ((6010, 6103), (6011, 6102)):
    header, values = properties(objects[item])
    assert header[0] == 25 and values[:3] == [destination, 7, -1]
assert properties(objects[6002])[0][0] == 5
edges = [(room, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for room, body in rooms.items()
         for m in re.finditer(r"\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)", body, re.S)]
assert len(edges) == 328
assert (6002, 5, 1, 0, 6003) in edges and (6003, 4, 1, 0, 6002) in edges
doors = [r["arguments"] for r in evidence["reset_commands"] if r["command"] == "D"]
assert [a[1:5] for a in doors] == [[6002, 5, 5, 100], [6003, 4, 1, 100]]
assert (6098, 2, 0, 0, 634225) in edges and (6120, 0, 0, 0, 834369) in edges
guardian = [int(v) for v in re.search(r"^([\d -]+) S$", mobs[6015], re.M)[1].split()]
assert guardian[7] == 8192
assert re.search(r"^T\s+2\s+12\s+1\s+23$", objects[6003], re.M)
trap = (ROOT / "src/combat/trap.c").read_text()
damage_codes = {int(v) for v in re.findall(r"^#define TRAP_DAM_\w+ (\d+)$", trap, re.M)}
assert damage_codes == set(range(9)) and 12 not in damage_codes
assert "#define TRAP_EFF_MOVE BIT_1" in trap and "#define TRAP_EFF_OBJECT BIT_2" in trap and "#define TRAP_EFF_ROOM BIT_3" in trap
pickup = trap[trap.index("bool checkgetput("):trap.index("bool checkopen(")]
assert "TRAP_EFF_OBJECT" in pickup and "trapdamage(ch, obj);" in pickup and "return TRUE;" in pickup
damage = trap[trap.index("void trapdamage("):]
assert damage.index("obj->trap_charge--;") < damage.index("switch (obj->trap_dam)")
assert "case 12:" not in damage and "default:" not in damage
local_shop = (ROOT / "areas/shp/lizard.shp").read_text()
assert local_shop.startswith("#6014~\nN\n0\n") and "\n6014\n0\n6099\n" in local_shop
foreign_shop = (ROOT / "areas/shp/random.shp").read_text()
assert "#100015~\nN\n6008\n9225\n0\n" in foreign_shop
epic = (ROOT / "src/classes/epic_skills.c").read_text()
assert "{ 6013, SKILL_EXPERT_PARRY, 0, 100, 0, 0, 0 }" in epic
assert "economic_gameplay_authority::active()" in epic
binding = (ROOT / "src/world/epic.c").read_text()
assert "mob_index[real_mobile(epic_teachers[i].vnum)].func.mob = epic_teacher;" in binding
quest = (ROOT / "src/world/quest.c").read_text()
assert "mob_index[quest_index[count].quester].qst_func = quester;" in quest
assert "extract_obj(mob->carrying, TRUE);" in quest and "extract_char(mob);" in quest
carving = (ROOT / "src/classes/new_skills.c").read_text()
assert "carve = read_object(8, VIRTUAL);" in carving and "obj_to_obj(carve, corpse);" in carving
assert not evidence["special_assignments"]
assert catalog_tool.report_for(catalog)["eligible_by_zone"]["60"] == 2
print("Clavikord independent exact-head returns, supplied material, competing sources, rare givers, actual access, services and trap limits passed")
