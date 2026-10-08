#!/usr/bin/env python3
"""Vecna exact returns, real source parents, effective scripts and renewal."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import zone_story_quest_catalog as catalog_tool
import zone_story_quest_zone_inventory as inventory_tool


def bodies(area, kind):
    text = (ROOT / "areas" / kind / f"{area}.{kind}").read_text()
    return {int(m[1]): m[2] for m in re.finditer(r"^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)", text, re.M)}


def properties(body):
    lines = body.split("~", 4)[4].strip().splitlines()
    return [int(v) for v in lines[0].split()], [int(v) for v in lines[1].split()]


def edges(rooms):
    return [(room, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for room, body in rooms.items()
            for m in re.finditer(r"\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)", body, re.S)]


def function(text, name):
    start = text.index("int " + name + "(")
    end = text.find("\nint ", start + 1)
    return text[start:end if end != -1 else len(text)]


evidence = inventory_tool.area_evidence(ROOT, "vecna")
catalog = catalog_tool.production_catalog(ROOT)
mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "vecna")
assert (mapping["schema_version"], mapping["revision"], mapping["coverage"]) == (3, 1, "complete")
assert (len(mapping["stories"]), len(mapping["contacts"])) == (2, 17)
assert not mapping["exclusions"]
expected = {11: (130023, 130034, 130035), 30: (130024, 130033, 130036)}
assert len(evidence["requests"]) == 2
for request in evidence["requests"]:
    block = request["block"]
    giver, proof, reward = expected[block["line"]]
    assert (block["giver_vnum"], block["give"], block["receive"], block["disappear"]) == (giver, [("I", proof)], [("I", reward)], True)
    assert request["definition"]["zone_number"] == 1300 and not request["definition"]["prerequisites"]
    story = next(s for s in mapping["stories"] if s["contracts"] == [block["binding"]])
    assert len(story["steps"]) == 2
    preparation, receipt = story["steps"]
    assert (preparation["kind"], preparation["item_vnums"], preparation["count"], preparation["optional"]) == ("carried_item", [proof], 1, True)
    assert receipt["kind"] == "completion" and receipt["contracts"] == story["contracts"]
assert len(evidence["dialogue"]) == 2 and all(d["body"][0] == "quest vecna hello hi~" for d in evidence["dialogue"])
assert sum(len(c["topics"]) for c in mapping["contacts"]) == 8
rooms, mobs, objects = (bodies("vecna", kind) for kind in ("wld", "mob", "obj"))
assert (len(rooms), len(mobs), len(objects)) == (80, 40, 41)
assert set(rooms) == set(range(130000, 130080))
for contact in mapping["contacts"]:
    assert contact["keyword"] in mobs[contact["mob_vnum"]].split("~")[0].split()
assert (evidence["zone"]["zone_number"], evidence["zone"]["last_vnum"], evidence["zone"]["reset_mode"]) == (1300, 130079, 0)
assert len(evidence["reset_commands"]) == 227
assert collections.Counter(r["command"] for r in evidence["reset_commands"]) == {"D": 16, "O": 52, "P": 4, "M": 94, "E": 51, "G": 6, "R": 1, "F": 3}
assert len((ROOT / "areas/zon/vecna.zon").read_text().splitlines()) == 286
assert len((ROOT / "areas/qst/vecna.qst").read_text().splitlines()) == 38
reset = {r["line"]: (r["command"], r["arguments"][:5]) for r in evidence["reset_commands"]}
for line, command, args in [
    (119, "O", [0, 130023, 1, 130069, 100]), (121, "P", [1, 130034, 1, 130023, 100]),
    (254, "M", [0, 130023, 1, 130067, 100]), (256, "M", [0, 130024, 1, 130068, 100]),
    (275, "M", [0, 130027, 1, 130076, 100]), (276, "G", [1, 130033, 1, 0, 100]),
    (224, "M", [0, 130018, 1, 130050, 100]), (225, "G", [1, 130005, 1, 0, 100]),
    (116, "O", [0, 130012, 1, 130056, 100]), (118, "O", [0, 130006, 1, 130066, 100]),
    (103, "O", [0, 230038, 1, 130027, 100]), (265, "G", [1, 360, 1, 0, 100]),
    (266, "G", [1, 26666, 1, 0, 100])]:
    assert reset[line] == (command, args)
for vnum in (130023, 130024, 130025):
    assert properties(objects[vnum])[0][0] == 15
for vnum in (130033, 130034):
    header, _ = properties(objects[vnum])
    assert header[0] == 13 and header[8] & 32768
assert not any(r["command"] in {"G", "E"} and r["arguments"][1] == 130034 for r in evidence["reset_commands"])
assert not any(r["command"] in {"O", "P", "G", "E"} and r["arguments"][1] == 130002 for r in evidence["reset_commands"])
assert "T\n9 9 -1 60" in objects[130002]
for key in (130005, 130012):
    header, values = properties(objects[key])
    assert header[0] == 18 and values[1] == 100
assert properties(objects[130006])[0][0] == 25
assert properties(objects[130006])[1][:4] == [130072, 7, -1, 0]
graph = edges(rooms)
assert len(graph) == 187 and (130000, 2, 0, 0, 597408) in graph
assert (597408, 0, 0, 0, 130000) in edges({597408: bodies("surface", "wld")[597408]})
assert (14208, 2, 0, 0, 130021) in edges({14208: bodies("realm", "wld")[14208]})
assert (130005, 0, 3, 130005, 130049) in graph and (130049, 2, 3, 130005, 130005) in graph
assert (130070, 0, 3, 130012, 130071) in graph and (130071, 2, 3, 130012, 130070) in graph
assert not [edge for edge in graph if edge[-1] == 130069 or edge[0] == 130079]
assert re.search(r"\bF\s*100\b", rooms[130037])
specials = evidence["special_assignments"]
assert len(specials) == 27 and len({(a["kind"], a["vnum"]) for a in specials}) == 26
assert [a["function"] for a in specials if a["kind"] == "obj" and a["vnum"] == 130041] == ["vecna_ghosthands", "vecna_torturerroom"]
assert not any(a["kind"] == "obj" and a["vnum"] == 130037 for a in specials)
native = (ROOT / "src/specs/specs.vecna.c").read_text()
hand = function(native, "vecnas_fight_proc")
assert "CMD_GOTHIT" in hand and "GET_MAX_HIT(victim) * 0.5" in hand and "real_room(130069)" in hand
assert "IS_TRUSTED(victim)" in hand and "IS_NPC(victim)" not in hand and "130034" not in hand
mass = function(native, "vecna_black_mass")
assert "CMD_DEATH" in mass and "world[ch->in_room].people" in mass and "real_room(130076)" in mass
portal = function(native, "vecna_deathportal")
assert "130072, 130073, 130075" in portal and "rooms[number(0, 2)]" in portal
mist = function(native, "vecna_stonemist")
assert "cmd != CMD_PERIODIC || !IS_ALIVE(ch)" in mist
for name in ("vecna_ghosthands", "vecna_torturerroom"):
    body = function(native, name)
    assert "ITEM_CORPSE" in body and "CMD_PERIODIC" not in body
altar = function(native, "vecna_deathaltar")
assert "cmd == CMD_TOUCH || arg" in altar and "hecate->only.npc->R_num" in altar
db = (ROOT / "src/world/db.c").read_text()
assert "invoke_object_special(obj, 0, CMD_PERIODIC, 0)" in db
epic = (ROOT / "src/world/epic.c").read_text()
assert "touch.reset_requested = touch.record_zone &&" in epic and "!zone_table[real_zone0(zone_number)].reset_mode" in epic
assert "if (result.reset_requested)" in epic and "add_event(event_reset_zone, 1" in epic
assert "epic_zone_done_now(zone_table[zone_number].number)" in db
traps = (ROOT / "src/combat/trap.c").read_text()
assert {int(v) for v in re.findall(r"^#define TRAP_DAM_\w+ (\d+)", traps, re.M)} == set(range(9))
assert "carve = read_object(8, VIRTUAL);" in (ROOT / "src/classes/new_skills.c").read_text()
assert catalog_tool.report_for(catalog)["eligible_by_zone"]["1300"] == 2
print("Vecna independent exact returns, container/carrying origins, effective script and mode0 renewal qualification passed")
