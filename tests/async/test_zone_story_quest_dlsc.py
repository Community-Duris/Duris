#!/usr/bin/env python3
"""Domain exact hand-ins, repeated skull stock and independent native services."""
import collections
import json
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

evidence = inventory_tool.area_evidence(ROOT, "dlsc")
catalog = catalog_tool.production_catalog(ROOT)
mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "dlsc")
assert (mapping["schema_version"], mapping["revision"], mapping["coverage"]) == (3, 1, "complete")
assert (len(mapping["stories"]), len(mapping["contacts"])) == (2, 17)
expected = {17: (36822, [36856, 36860], [("I", 36859), ("E", 75500)], True),
            47: (36837, [55037] * 3, [("I", 55038)], False)}
assert len(evidence["requests"]) == len(expected)
for request in evidence["requests"]:
    block = request["block"]
    giver, items, rewards, disappear = expected[block["line"]]
    assert (block["giver_vnum"], block["give"], block["receive"], block["disappear"]) == (giver, [("I", v) for v in items], rewards, disappear)
    assert request["definition"]["zone_number"] == 368 and not request["definition"]["prerequisites"]
    story = next(s for s in mapping["stories"] if s["contracts"] == [block["binding"]])
    material = [s for s in story["steps"] if s["kind"] == "carried_item"]
    assert [(s["item_vnums"], s["count"], s["optional"]) for s in material] == [([v], count, True) for v, count in collections.Counter(items).items()]
    assert story["steps"][-1]["kind"] == "completion" and story["steps"][-1]["contracts"] == story["contracts"]
    assert all(s.get("optional") for s in story["steps"][:-1])
assert [b["body"][0] for b in evidence["dialogue"]] == ["contract curse wife~", "hero heroes~"]
assert sum(len(b["body"][0].rstrip("~").split()) for b in evidence["dialogue"]) == 5
rooms, mobs, objects = (bodies("dlsc", k) for k in ("wld", "mob", "obj"))
assert (len(rooms), len(mobs), len(objects)) == (100, 71, 100)
assert set(rooms) == set(range(36800, 36900))
assert (evidence["zone"]["zone_number"], evidence["zone"]["first_vnum"], evidence["zone"]["last_vnum"], evidence["zone"]["reset_mode"]) == (368, 36778, 36899, 1)
resets = evidence["reset_commands"]
assert len(resets) == 488 and collections.Counter(r["command"] for r in resets) == {"D": 30, "O": 32, "P": 16, "M": 145, "E": 158, "G": 71, "F": 36}
assert len((ROOT / "areas/zon/dlsc.zon").read_text().splitlines()) == 589
assert len((ROOT / "areas/qst/dlsc.qst").read_text().splitlines()) == 56
parent, stock = None, []
for r in resets:
    c, a = r["command"], r["arguments"]
    if c in {"M", "F"}:
        parent = a[1], a[3]
    if c in {"G", "E"}:
        stock.append((c, a[1], a[2], parent, r["line"]))
assert [s for s in stock if s[1] == 55037] == [
    ("G", 55037, 3, (36801, 36844), 291),
    ("G", 55037, 3, (36862, 36858), 362),
    ("G", 55037, 3, (36820, 36863), 410)]
assert not any(s[1] == 55037 and s[3][0] == 36812 for s in stock)
all_blocks = inventory_tool.native_blocks(ROOT)
for item in (36856, 36860):
    assert not any(k == "I" and v == item for b in all_blocks for k, v in b["receive"])
    for zone in catalog_tool.zone_registry(ROOT):
        for line in (ROOT / "areas/zon" / (zone["source_area"] + ".zon")).read_text().splitlines():
            m = re.match(r"^[OGEP]\s+\d+\s+(\d+)\b", line)
            assert not m or int(m[1]) != item
assert "Arganas" in objects[36856] and "Elizabyth" in objects[36860]
carve = (ROOT / "src/classes/new_skills.c").read_text().split("void do_carve(", 1)[1].split("void do_bind(", 1)[0]
assert "read_object(8, VIRTUAL)" in carve and "55037" not in carve
assert "HUMANOID_CORPSE" in carve and "obj_to_obj(carve, corpse)" in carve
assert "#8\nbodypart~" in (ROOT / "areas/obj/limbo.obj").read_text()
wh = bodies("wh", "obj")
assert "seasoned warrior _noquest_" in wh[55037]
assert sorted((b["giver_vnum"], tuple(b["receive"])) for b in all_blocks if b["give"] == [("I", 55037)] * 3) == [(36837, (("I", 55038),)), (55110, (("I", 55038), ("E", 250000)))]
assert any(b["giver_vnum"] == 55208 and b["give"] == [("I", 55038)] and b["receive"] == [("C", 250000), ("I", 55039)] for b in all_blocks)
edges = [(v, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for v, body in rooms.items()
         for m in re.finditer(r"\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)", body, re.S)]
assert len(edges) == 218
assert {(36801, 1, 0, 0, 36802), (36802, 3, 0, 0, 36801),
        (36825, 0, 3, 36825, 36828), (36828, 2, 3, 36825, 36825),
        (36850, 1, 3, 36846, 36859), (36859, 3, 3, 36846, 36850),
        (36861, 0, 3, -2, 36873), (36873, 2, 3, -2, 36861)} <= set(edges)
assert "340 36801 1 0 0 0 0 0" in objects[36800] and "340 36802 3 0 0 0 0 0" in objects[36801]
assert "door oak _nobash_ diabolus~" in rooms[36861] and "Diabolus" in rooms[36861]
assert "150 29 36872 250" in objects[36865]
assert {("G", 36846, 1, (36813, 36850), 317), ("E", 36892, 2, (36849, 36871), 477)} <= set(stock)
assert {("G", 36813, 1, (36800, 36803), 183),
        ("G", 36872, 1, (36831, 36888), 531),
        ("G", 36899, 1, (36856, 36877), 512),
        ("G", 36811, 1, (36856, 36877), 513)} <= set(stock)
assert {(r["command"], r["arguments"][1], r["arguments"][3], r["line"])
        for r in resets if r["command"] in {"O", "P"}} >= {
            ("O", 36800, 36801, 129), ("O", 36801, 36802, 130),
            ("P", 36822, 36819, 133), ("O", 36825, 36827, 136),
            ("O", 36826, 36845, 141), ("O", 36842, 36899, 168),
            ("O", 36865, 36889, 157), ("P", 36883, 36865, 158),
            ("P", 36884, 36865, 159), ("O", 72, 36889, 160)}
assign = (ROOT / "src/specs/specs.assign.c").read_text()
assert "real_object0(36884)].func.obj = kvasir_dagger" in assign and "real_object0(36894)].func.obj = critical_attack_proc" in assign
teachers = (ROOT / "src/classes/epic_skills.c").read_text()
assert "{ 36849, SKILL_TWOWEAPON, 0, 100, 0, 0, 0 }" in teachers
assert "Epic skill purchases are unavailable while economic accounting is active." in teachers
assert "mob_index[real_mobile(epic_teachers[i].vnum)].func.mob = epic_teacher;" in (ROOT / "src/world/epic.c").read_text()
print("Domain exact joint/three-copy returns, correct skull parent, source gaps, access and accounting-disabled training passed")
