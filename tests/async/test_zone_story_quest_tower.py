#!/usr/bin/env python3
"""Tower independent exact bundles, source parents, effective roles and access."""
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


def edges(rooms):
    return [(room, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for room, body in rooms.items()
            for m in re.finditer(r"\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)", body, re.S)]


def source_stock(area):
    parent, result = None, []
    for row in inventory_tool.area_evidence(ROOT, area)["reset_commands"]:
        c, a = row["command"], row["arguments"]
        if c in {"M", "F"}:
            parent = a[1], a[3], row["line"]
        if c in {"G", "E", "P", "O"}:
            result.append((c, a[1], a[2], a[3], a[4], parent, row["line"]))
    return result


evidence = inventory_tool.area_evidence(ROOT, "tower")
catalog = catalog_tool.production_catalog(ROOT)
mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "tower")
assert (mapping["schema_version"], mapping["revision"], mapping["coverage"]) == (3, 1, "complete")
assert (len(mapping["stories"]), len(mapping["contacts"])) == (2, 18)
assert not mapping["exclusions"]
expected = {15: (9321, [5011, 5016, 5035], [("E", 18100), ("I", 9375), ("C", 53000)]),
            47: (9367, [9365, 9366], [("I", 9369), ("I", 9373), ("E", 33333)])}
assert len(evidence["requests"]) == 2
for request in evidence["requests"]:
    b = request["block"]
    giver, proofs, rewards = expected[b["line"]]
    assert (b["giver_vnum"], b["give"], b["receive"], b["disappear"]) == (giver, [("I", v) for v in proofs], rewards, True)
    assert request["definition"]["zone_number"] == 93 and not request["definition"]["prerequisites"]
    story = next(s for s in mapping["stories"] if s["contracts"] == [b["binding"]])
    assert len(story["steps"]) == len(proofs) + 1
    for step, proof in zip(story["steps"][:-1], proofs):
        assert (step["kind"], step["item_vnums"], step["count"], step["optional"]) == ("carried_item", [proof], 1, True)
    assert story["steps"][-1]["kind"] == "completion" and story["steps"][-1]["contracts"] == story["contracts"]
assert [b["body"][0] for b in evidence["dialogue"]] == ["labyrinth no return maze~", "kraken eye scale~"]
assert sum(len(c["topics"]) for c in mapping["contacts"]) == 7
rooms, mobs, objects = (bodies("tower", k) for k in ("wld", "mob", "obj"))
assert (len(rooms), len(mobs), len(objects)) == (100, 77, 77)
assert set(rooms) == set(range(9300, 9400))
for contact in mapping["contacts"]:
    assert contact["keyword"] in mobs[contact["mob_vnum"]].split("~")[0].split()
assert (evidence["zone"]["zone_number"], evidence["zone"]["first_vnum"], evidence["zone"]["last_vnum"], evidence["zone"]["reset_mode"]) == (93, 9289, 9399, 1)
assert len(evidence["reset_commands"]) == 233
assert collections.Counter(r["command"] for r in evidence["reset_commands"]) == {"D": 30, "O": 3, "M": 93, "E": 89, "G": 16, "F": 2}
assert len((ROOT / "areas/zon/tower.zon").read_text().splitlines()) == 334
assert len((ROOT / "areas/qst/tower.qst").read_text().splitlines()) == 75
local, foreign = source_stock("tower"), source_stock("labyrinth")
assert ("E", 9366, 1, 21, 100, (9360, 9389, 293), 295) in local
assert ("G", 9365, 1, 0, 100, (9361, 9391, 298), 300) in local
assert not any(r[1] in {9365, 9366} and r[5][0] == 9354 for r in local)
assert ("G", 5011, 1, 0, 100, (5016, 5129, 213), 214) in foreign
assert any(r[:5] == ("O", 5035, 1, 5003, 100) and r[-1] == 71 for r in foreign)
assert ("E", 5067, 1, 28, 100, (5019, 5189, 246), 250) in foreign
assert ("P", 5016, 1, 5067, 100, (5019, 5189, 246), 256) in foreign
assert properties(bodies("labyrinth", "obj")[5067])[0][0] == 15
assert properties(bodies("labyrinth", "obj")[5067])[1][:4] == [444, 0, 0, 444]
for proof in (9365, 9366):
    header, _ = properties(objects[proof])
    assert header[0] == 8 and header[8] & 32768
for item, values in ((9361, [340, 9353, 3, 0]), (9360, [340, 9372, 1, 0])):
    assert properties(objects[item])[0][0] == 29 and properties(objects[item])[1][:4] == values
graph = edges(rooms)
assert len(graph) == 219 and (9353, 3, 8, 0, 9372) in graph and (9372, 1, 8, 0, 9353) in graph
assert (9334, 2, 0, 0, 586837) in graph
assert (586837, 0, 0, 0, 9334) in edges({586837: bodies("surface", "wld")[586837]})
assert (9301, 4, 3, 9336, 9300) in graph and (9300, 5, 2, 9336, 9301) in graph
assert (9366, 5, 2, 9337, 9371) in graph and (9371, 4, 2, 9337, 9366) in graph
assert not [x for x in graph if x[0] == 9399]
sink_header = re.search(r"~\s*93\s+(\d+)\s+(\d+)", rooms[9399])
assert sink_header and not int(sink_header[1]) & 4 and int(sink_header[2]) != 8
assert any(x[0] == 9395 and x[-1] == 9399 for x in graph)
for key, chance in ((9300, 15), (9301, 0), (9336, 0), (9337, 0)):
    assert properties(objects[key])[0][0] == 18 and properties(objects[key])[1][1] == chance
assert properties(objects[9367])[0][0] == 22
specials = {(a["kind"], a["vnum"], a["function"]) for a in evidence["special_assignments"]}
assert specials == {("mob", v, "bulette") for v in (9342, 9344, 9345, 9346, 9347, 9348, 9349, 9350)}
underworld = (ROOT / "src/specs/specs.underworld.c").read_text()
bulette = underworld[underworld.index("int bulette("):underworld.index("int bulette(") + 1600]
assert "if (cmd != 0)" in bulette and "Flowers!" in bulette
rogue_class = int(re.search(r" S\s*\n\S+\s+\d+\s+(\d+)\s+\d+\s+-?\d+\s*\n", mobs[9316])[1])
assert rogue_class == 24576 and not rogue_class & 4096
db = (ROOT / "src/world/db.c").read_text()
assert "CLASS_ROGUE" in db and "func.obj = item_switch;" in db
conversion = (ROOT / "src/mob/mobconv.c").read_text()
assert "ch->points.base_hit = hits;" in conversion and "class_hitpoints[flag2idx(ch->player.m_class)]" in conversion
quest = (ROOT / "src/world/quest.c").read_text()
assert "put32(0, fee_terms ? 5 : 6);" in quest and "QUEST_REWARD_MAX_CREDITED_PIDS" in quest
assert "extract_obj(mob->carrying, TRUE);" in quest and "extract_char(mob);" in quest
carving = (ROOT / "src/classes/new_skills.c").read_text()
assert "carve = read_object(8, VIRTUAL);" in carving
assert "55441" in (ROOT / "areas/qst/wh.qst").read_text()
assert catalog_tool.report_for(catalog)["eligible_by_zone"]["93"] == 2
print("Tower independent exact bundles, mixed source parents, effective roles, actual access and reward qualification passed")
