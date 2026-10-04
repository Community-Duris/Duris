#!/usr/bin/env python3

import importlib.util
import collections
import json
import pathlib
import re
import subprocess
import sys
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts" / "zone_story_quest_catalog.py"
spec = importlib.util.spec_from_file_location("zone_story_quest_catalog", SCRIPT)
catalog_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(catalog_module)
native_header = (ROOT / "src/world/zone_story_quest_production.h").read_text()
native_limit = int(re.search(r"ZONE_STORY_QUEST_MAX_DURABLE_OFFERINGS\s*=\s*(\d+)", native_header)[1])
assert native_limit == catalog_module.MAX_DURABLE_ITEM_OFFERINGS

catalog = catalog_module.production_catalog(ROOT)
assert catalog["source"]["kind"] == "legacy_static_qst"
assert catalog["source"]["area_list"] == "areas/AREA"
assert catalog["source"]["excludes"] == ["bartender_random_world_quests"]
assert len(catalog["definitions"]) == 2668
assert all(item["zone_number"] >= 0 for item in catalog["definitions"])
assert all(item["source_system"] == "zone_story" for item in catalog["definitions"])
assert catalog["content_revision"] == 2
assert all(item["repeatable"] for item in catalog["definitions"] if item["daily_eligible"])
alatorin = [item for item in catalog["definitions"] if 83100 <= item["giver_vnum"] < 83600]
assert alatorin and all(item["zone_number"] == 831 for item in alatorin)
assert any(zone["discoverable"] and not any(item["zone_number"] == zone["zone_number"] for item in catalog["definitions"]) for zone in catalog["zones"])
assert any(item["daily_exclusion"] == "Item exchange" for item in catalog["definitions"])
assert any(item["daily_exclusion"] == "Unsupported durable offering" for item in catalog["definitions"])
for item in catalog["definitions"]:
    if item["daily_eligible"]:
        give = bytes.fromhex(item["completion_key"]).decode().split(";")[0][5:].split(",")
        assert len(give) <= catalog_module.MAX_DURABLE_ITEM_OFFERINGS
        assert all(goal.startswith("I:") and int(goal[2:]) > 0 for goal in give)
checked_in = json.loads((ROOT / "docs/reference/ZONE_STORY_QUEST_PRODUCTION_CATALOG.json").read_text())
assert checked_in == catalog
assert len({item["definition_id"] for item in catalog["definitions"]}) == len(catalog["definitions"])

inventory_spec = importlib.util.spec_from_file_location("zone_inventory", ROOT / "scripts/zone_story_quest_zone_inventory.py")
inventory_module = importlib.util.module_from_spec(inventory_spec)
sys.path.insert(0, str(ROOT / "scripts"))
inventory_spec.loader.exec_module(inventory_module)
rows, inventory_mobs, inventory_items = inventory_module.inventory(ROOT)
assert len(rows) == 350 and sum(bool(r["requests"]) for r in rows) == 221
assert inventory_items[5]["source"] == "areas/obj/limbo.obj"
assert inventory_items[5]["keywords"] == ["paper", "note"]
assert inventory_items[5]["name"] == "a blank piece of paper"
assert not any(r["zone"]["source_area"] == "limbo" for r in rows)
assert sum(len(r["requests"]) for r in rows) == 2668
assert {q["definition"]["definition_id"] for r in rows for q in r["requests"]} == {d["definition_id"] for d in catalog["definitions"]}
assert inventory_module.markdown(ROOT) == (ROOT / "docs/reference/ZONE_STORY_ZONE_INVENTORY.md").read_text(encoding="utf-8")
assignments = inventory_module.special_assignments('''
// mob_index[real_mobile0(99)].func.mob = commented;
/* world[real_room0(98)].funct = commented; */
mob_index[real_mobile0(10)].func.mob =
    mob_index[real_mobile0(11)].func.mob = shared;
obj_index[real_object0(12)].func.obj = NULL;
world[real_room0(13)].funct = gate;
const char *example = "mob_index[real_mobile0(97)].func.mob = quoted;";
''')
assert [(a["kind"], a["vnum"], a["function"], a["line"]) for a in assignments] == [
    ("mob", 10, "shared", 4), ("mob", 11, "shared", 5), ("room", 13, "gate", 7)]
multiline = inventory_module.special_assignments('''
obj_index [ real_object0 (
 14
) ]
 .func
 .obj = obj_index [ real_object0 (15) ] .func .obj = decay;
obj_index[real_object0(computed_vnum)].func.obj = computed;
''')
assert [(a["kind"], a["vnum"], a["function"], a["line"]) for a in multiline] == [
    ("obj", 14, "decay", 2), ("obj", 15, "decay", 6)]
wh_evidence = inventory_module.area_evidence(ROOT, "wh")
assert len(wh_evidence["requests"]) == 221 and len(wh_evidence["special_assignments"]) == 92
assert {a["vnum"] for a in wh_evidence["special_assignments"] if a["function"] == "wh_corpse_decay"} == set(range(55500, 55521))
assert len(wh_evidence["dialogue"]) == 191 and len(wh_evidence["reset_commands"]) == 1284
assert len(wh_evidence["mobs"]) == 314 and len(wh_evidence["items"]) == 483
wh_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "wh")
wh_contacts = {c["mob_vnum"]: c for c in wh_mapping["contacts"]}
nonempty_topics = 0
for response in wh_evidence["dialogue"]:
    aliases = set(response["body"][0].rstrip("~").split())
    # Empty native default bodies do not provide a useful advertised topic.
    if inventory_module.plain(" ".join(response["body"][1:]).replace("~", "")):
        assert aliases & set(wh_contacts[response["giver_vnum"]]["topics"])
        nonempty_topics += 1
assert nonempty_topics == 152 and len(wh_contacts) == 92
for vnum, contact in wh_contacts.items():
    assert contact["keyword"] in wh_evidence["mobs"][vnum]["keywords"]
twin = inventory_module.area_evidence(ROOT, "twin_towers_forest")
assert len(twin["requests"]) == 84 and len(twin["dialogue"]) == 58
assert len(twin["reset_commands"]) == 345
assert {a["vnum"] for a in twin["special_assignments"] if a["function"] == "forest_animals"} == {13505, 13508, 13511, 13513, 13515, 13516, 13517, 13518, 13519}
assert len([a for a in twin["special_assignments"] if a["function"] == "forest_corpse"]) == 9
assert len([a for a in twin["special_assignments"] if a["function"] == "gardener_block"]) == 12
plains = inventory_module.area_evidence(ROOT, "newbie2")
assert not plains["requests"] and len(plains["reset_commands"]) == 21
assert set(plains["specials"]) == {"newbie_paladin", "newbie_sign1", "newbie_sign2", "stream_of_life"}
assert 22809 in plains["items"]  # Prototype membership can exceed the zone room range.
ailvio = inventory_module.area_evidence(ROOT, "newbie")
assert len(ailvio["requests"]) == 116 and len(ailvio["dialogue"]) == 160
assert len(ailvio["reset_commands"]) == 314 and len(ailvio["special_assignments"]) == 11
mansion = inventory_module.area_evidence(ROOT, "braddistock")
assert len(mansion["requests"]) == 2 and len(mansion["dialogue"]) == 1
assert len(mansion["reset_commands"]) == 186
assert [(a["kind"], a["vnum"], a["function"]) for a in mansion["special_assignments"]] == [("obj", 1372, "jet_black_maul")]
# The identically named mobile procedure belongs to the other mansion area.
assert not any(a["function"] == "braddistock" for a in mansion["special_assignments"])
breale = inventory_module.area_evidence(ROOT, "breale")
assert len(breale["requests"]) == 6 and len(breale["dialogue"]) == 6
assert len(breale["reset_commands"]) == 399
assert len(breale["special_assignments"]) == 9 and set(breale["specials"]) == {"breale_townsfolk"}
elvish = inventory_module.area_evidence(ROOT, "elvish")
assert len(elvish["requests"]) == 4 and len(elvish["dialogue"]) == 6
assert len(elvish["reset_commands"]) == 138 and not elvish["special_assignments"]
krimman = inventory_module.area_evidence(ROOT, "krimman")
assert len(krimman["requests"]) == 9 and len(krimman["dialogue"]) == 9
assert len(krimman["reset_commands"]) == 176 and not krimman["special_assignments"]
bastine = inventory_module.area_evidence(ROOT, "bastine")
assert len(bastine["requests"]) == 14 and len(bastine["dialogue"]) == 4
assert len(bastine["reset_commands"]) == 428 and not bastine["special_assignments"]
pineholl = inventory_module.area_evidence(ROOT, "pineholl")
assert len(pineholl["requests"]) == 7 and len(pineholl["dialogue"]) == 10
assert len(pineholl["reset_commands"]) == 352 and not pineholl["special_assignments"]
assert len(pineholl["mobs"]) == 88 and len(pineholl["items"]) == 86
# Preserve the documented supply conflict until a reviewed world-content repair:
# the coat needs two huge skins while its only ordinary producer caps live skins at one.
huge_skin_sources = [r for r in pineholl["reset_commands"] if r["command"] in ("O", "P", "G", "E") and r["arguments"][1] == 16021]
assert len(huge_skin_sources) == 1 and huge_skin_sources[0]["arguments"][:3] == [1, 16021, 1]
quietus = inventory_module.area_evidence(ROOT, "quietus")
assert len(quietus["requests"]) == 16 and len(quietus["dialogue"]) == 14
assert len(quietus["reset_commands"]) == 216 and len(quietus["mobs"]) == 54 and len(quietus["items"]) == 55
assert {(a["kind"], a["vnum"], a["function"]) for a in quietus["special_assignments"]} == {
    ("mob", 1709, "world_quest"), ("room", 1719, "ship_shop_proc"),
    ("room", 1736, "inn"), ("room", 1734, "crew_shop_proc")}
# Preserve rare-source placement as a reviewed content decision, rather than guessing
# a missing head source from foreign-port rumors or the unused Aresliean load room.
assert {r["arguments"][3] for r in quietus["reset_commands"] if r["command"] == "M" and r["arguments"][1] in (1749, 1751)} == {1784}
assert any(r["command"] == "G" and r["arguments"][1] == 1746 for r in quietus["reset_commands"])
assert any(r["command"] == "P" and r["arguments"][1:4] == [1742, 1, 1701] for r in quietus["reset_commands"])
quietus_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "quietus")
quietus_contacts = {c["mob_vnum"]: c for c in quietus_mapping["contacts"]}
for response in quietus["dialogue"]:
    assert set(response["body"][0].rstrip("~").split()) & set(quietus_contacts[response["giver_vnum"]]["topics"])
for vnum, contact in quietus_contacts.items():
    assert contact["keyword"] in quietus["mobs"][vnum]["keywords"]
torg = inventory_module.area_evidence(ROOT, "torg")
assert len(torg["requests"]) == 15 and len(torg["dialogue"]) == 15
assert len(torg["reset_commands"]) == 597 and len(torg["mobs"]) == 126 and len(torg["items"]) == 100
assert {(a["kind"], a["vnum"], a["function"]) for a in torg["special_assignments"]} == {
    ("mob", 28961, "timoro_die"), ("mob", 29025, "lanella_heart"), ("room", 29103, "inn")}
# The alternate master remains a valid native contract without an ordinary reset.
assert not any(r["command"] in ("M", "F") and r["arguments"][1] == 29023 for r in torg["reset_commands"])
assert any(r["command"] == "F" and r["arguments"][1:4] == [29024, 1, 29116] for r in torg["reset_commands"])
# F changes the source parent: this chisel belongs to the jeweler, not the last M (Zarina).
fine_chisel = next(i for i, r in enumerate(torg["reset_commands"]) if r["command"] == "G" and r["arguments"][1] == 28959)
assert torg["reset_commands"][fine_chisel]["arguments"][4] == 20
assert torg["reset_commands"][fine_chisel - 1]["command"] == "F"
assert torg["reset_commands"][fine_chisel - 1]["arguments"][1] == 28936
legend_mobs = {28948, 28949, 28950, 28951, 28952, 28954, 28955, 28957}
legend_resets = [r for r in torg["reset_commands"] if r["command"] == "M" and r["arguments"][1] in legend_mobs]
assert len(legend_resets) == 8 and all(r["arguments"][2:5] == [1, 29061, 100] for r in legend_resets)
tranug_relics = [r for r in torg["reset_commands"] if r["command"] == "G" and r["arguments"][1] in (28983, 28984, 28985)]
assert len(tranug_relics) == 3 and all(r["arguments"][4] == 50 for r in tranug_relics)
torg_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "torg")
torg_contacts = {c["mob_vnum"]: c for c in torg_mapping["contacts"]}
for response in torg["dialogue"]:
    assert set(response["body"][0].rstrip("~").split()) & set(torg_contacts[response["giver_vnum"]]["topics"])
for vnum, contact in torg_contacts.items():
    assert contact["keyword"] in torg["mobs"][vnum]["keywords"]
solonar = inventory_module.area_evidence(ROOT, "solonar")
assert len(solonar["requests"]) == 15 and len(solonar["dialogue"]) == 12
assert len(solonar["reset_commands"]) == 152 and len(solonar["mobs"]) == 41 and len(solonar["items"]) == 72
# A literal range candidate is not proof that its target exists in the active world.
assert {(a["kind"], a["vnum"], a["function"]) for a in solonar["special_assignments"]} == {("room", 30511, "inn")}
active_areas = [line.split()[0] for line in (ROOT / "areas/AREA").read_text().splitlines()
                if line.strip() and not line.lstrip().startswith("*")]
assert not any(re.search(r"^#30511\s*$", path.read_text(), re.M) for area in active_areas
               for path in [ROOT / f"areas/wld/{area}.wld"] if path.is_file())
assert any(r["command"] == "M" and r["arguments"][1:5] == [30638, 1, 30607, 100] for r in solonar["reset_commands"])
assert any(r["command"] == "M" and r["arguments"][1:5] == [30636, 1, 30607, 100] for r in solonar["reset_commands"])
assert not any(r["command"] == "D" and r["arguments"][1] == 30686 for r in solonar["reset_commands"])
solonar_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "solonar")
solonar_contacts = {c["mob_vnum"]: c for c in solonar_mapping["contacts"]}
for response in solonar["dialogue"]:
    assert set(response["body"][0].rstrip("~").split()) & set(solonar_contacts[response["giver_vnum"]]["topics"])
for vnum, contact in solonar_contacts.items():
    assert contact["keyword"] in solonar["mobs"][vnum]["keywords"]
smokev = inventory_module.area_evidence(ROOT, "smokev")
assert len(smokev["requests"]) == 11 and len(smokev["dialogue"]) == 32
assert len(smokev["reset_commands"]) == 435 and not smokev["special_assignments"]
assert len(smokev["mobs"]) == 74 and len(smokev["items"]) == 59
smokev_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "smokev")
smokev_contacts = {c["mob_vnum"]: c for c in smokev_mapping["contacts"]}
assert len(smokev_contacts) == 9 and sum(len(c["topics"]) for c in smokev_contacts.values()) == 32
for response in smokev["dialogue"]:
    assert set(response["body"][0].rstrip("~").split()) & set(smokev_contacts[response["giver_vnum"]]["topics"])
for vnum, contact in smokev_contacts.items():
    assert contact["keyword"] in smokev["mobs"][vnum]["keywords"]
assert any(r["command"] == "O" and r["arguments"][1:5] == [20211, 1, 20216, 100] for r in smokev["reset_commands"])
# These proofs are existing NPC stock, not inferred births from their trophy names.
parent = None
proof_parents = {}
for reset in smokev["reset_commands"]:
    command, values = reset["command"], reset["arguments"]
    if command in ("M", "F"):
        parent = values[1]
    elif command == "G" and values[1] in {20200, 20209, 20244, 20253}:
        proof_parents[values[1]] = parent
        assert values[2] == 1 and values[4] == 100
assert proof_parents == {20200: 20233, 20209: 20235, 20244: 20237, 20253: 20224}
tezcat = inventory_module.area_evidence(ROOT, "tezcat")
parent = None
foreign_head_sources = []
for reset in tezcat["reset_commands"]:
    command, values = reset["command"], reset["arguments"]
    if command in ("M", "F"):
        parent = values[1]
    elif command == "G" and values[1] == 20255:
        foreign_head_sources.append((parent, values[2], values[4]))
assert foreign_head_sources == [(98961, 1, 100)]
assert "diabolus" in tezcat["mobs"][98961]["keywords"]
keeps = inventory_module.area_evidence(ROOT, "caertannad")
assert len(keeps["requests"]) == 30 and len(keeps["dialogue"]) == 37
assert len(keeps["reset_commands"]) == 1614 and len(keeps["mobs"]) == 123 and len(keeps["items"]) == 121
assert keeps["zone"]["reset_mode"] == 0
assert {(a["kind"], a["vnum"], a["function"]) for a in keeps["special_assignments"]} == {("mob", 78476, "caertannad_summon")}
keeps_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "caertannad")
keeps_contacts = {c["mob_vnum"]: c for c in keeps_mapping["contacts"]}
assert len(keeps_contacts) == 24 and not keeps_contacts[78484]["topics"]
for response in keeps["dialogue"]:
    assert set(response["body"][0].rstrip("~").split()) & set(keeps_contacts[response["giver_vnum"]]["topics"])
for vnum, contact in keeps_contacts.items():
    assert contact["keyword"] in keeps["mobs"][vnum]["keywords"]
# Exact nested source identity matters: Blackbeard and pirate chests differ.
assert any(r["command"] == "P" and r["arguments"][1:4] == [78499, 1, 78490] for r in keeps["reset_commands"])
assert any(r["command"] == "P" and r["arguments"][1:4] == [78492, 1, 78491] for r in keeps["reset_commands"])
assert any(r["command"] == "D" and r["arguments"][1:4] == [78781, 0, 8] for r in keeps["reset_commands"])
assert not any(r["command"] == "O" and r["arguments"][1] == 78503 for r in keeps["reset_commands"])
rifts = inventory_module.area_evidence(ROOT, "tharnrifts")
assert any(r["command"] == "O" and r["arguments"][1:5] == [78503, 1, 113375, 100] for r in rifts["reset_commands"])
assert any(r["command"] == "O" and r["arguments"][1:5] == [78455, 1, 116025, 65] for r in rifts["reset_commands"])
bs = inventory_module.area_evidence(ROOT, "bs")
assert len(bs["requests"]) == 65 and len(bs["dialogue"]) == 74
assert len(bs["reset_commands"]) == 1624 and len(bs["mobs"]) == 255 and len(bs["items"]) == 300
assert bs["zone"]["reset_mode"] == 2
bs_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "bs")
bs_contacts = {c["mob_vnum"]: c for c in bs_mapping["contacts"]}
assert len(bs_contacts) == 20 and len(bs_contacts[74254]["topics"]) <= 32
for response in bs["dialogue"]:
    assert set(response["body"][0].rstrip("~").split()) & set(bs_contacts[response["giver_vnum"]]["topics"])
for vnum, contact in bs_contacts.items():
    assert contact["keyword"] in bs["mobs"][vnum]["keywords"]
# These makers are definitions, not proof of an available world recipient.
missing_makers = {74250, 74252, 74254, 74257, 74258}
for area in active_areas:
    path = ROOT / f"areas/zon/{area}.zon"
    if path.is_file():
        assert not any(int(m[1]) in missing_makers for m in re.finditer(r"^[MF]\s+\d+\s+(\d+)\s+", path.read_text(), re.M))
# Parent and initial placement distinguish stocked sources from other instances.
parent = room = None
proof_sources = {}
for reset in bs["reset_commands"]:
    command, values = reset["command"], reset["arguments"]
    if command in ("M", "F"):
        parent, room = values[1], values[3]
    elif command == "G" and values[1] in {74054, 74252, 74262, 74295}:
        proof_sources.setdefault(values[1], []).append((parent, room, values[2]))
assert proof_sources[74054] == [(74076, 74578, 1)]
assert proof_sources[74252] == [(74132, 74579, 2), (74135, 74775, 2)]
assert len(proof_sources[74262]) == 1 and proof_sources[74262][0][1:] == (74371, 1)
assert proof_sources[74295] == [(74207, 74719, 1)]
assert not any(r["command"] in ("G", "E", "O", "P") and r["arguments"][1] == 74243 for r in bs["reset_commands"])
assert any(r["command"] == "D" and r["arguments"][1:4] == [74007, 5, 1] for r in bs["reset_commands"])
moria = inventory_module.area_evidence(ROOT, "moria")
assert len(moria["requests"]) == 5 and len(moria["dialogue"]) == 2
assert len(moria["reset_commands"]) == 424 and len(moria["mobs"]) == 38 and len(moria["items"]) == 75
assert moria["zone"]["reset_mode"] == 2 and len(moria["special_assignments"]) == 28
moria_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "moria")
moria_contacts = {c["mob_vnum"]: c for c in moria_mapping["contacts"]}
assert len(moria_contacts) == 7
for response in moria["dialogue"]:
    assert set(response["body"][0].rstrip("~").split()) & set(moria_contacts[response["giver_vnum"]]["topics"])
for vnum, contact in moria_contacts.items():
    assert contact["keyword"] in moria["mobs"][vnum]["keywords"]
# One story owns every equal-offering variant, preserving older reward receipts.
bindings = [r["block"]["binding"] for r in moria["requests"]]
assert moria_mapping["stories"][0]["contracts"] == sorted(bindings, key=lambda b: b["completion_key"])
assert {b["completion_key"].split(";")[0] for b in bindings} == {"give=I:99002,I:99003,I:99004,I:99005,I:99006"}
assert {b["giver_vnum"] for b in bindings} == {99028}
parent = room = None
rune_sources = {}
for reset in moria["reset_commands"]:
    command, values = reset["command"], reset["arguments"]
    if command in ("M", "F"):
        parent, room = values[1], values[3]
    elif command == "G" and values[1] in range(99002, 99007):
        rune_sources[values[1]] = (parent, room, values[2], values[4])
assert rune_sources == {99002: (99003, 99017, 1, 100), 99003: (99004, 99043, 1, 100),
                       99004: (99005, 99070, 1, 100), 99005: (99006, 99158, 1, 100),
                       99006: (99007, 99185, 1, 100)}
assert any(r["command"] == "D" and r["arguments"][1:4] == [99251, 1, 0] for r in moria["reset_commands"])
claw = inventory_module.area_evidence(ROOT, "clwcvrn")
assert len(claw["requests"]) == 20 and len(claw["dialogue"]) == 8
assert len(claw["reset_commands"]) == 171 and len(claw["mobs"]) == 40 and len(claw["items"]) == 55
assert claw["zone"]["reset_mode"] == 2 and len(claw["special_assignments"]) == 4
claw_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "clwcvrn")
claw_contacts = {c["mob_vnum"]: c for c in claw_mapping["contacts"]}
assert len(claw_contacts) == 10
for response in claw["dialogue"]:
    assert set(response["body"][0].rstrip("~").split()) & set(claw_contacts[response["giver_vnum"]]["topics"])
for vnum, contact in claw_contacts.items():
    assert contact["keyword"] in claw["mobs"][vnum]["keywords"]
returns = [r["block"]["binding"] for r in claw["requests"] if r["block"]["give"] == r["block"]["receive"]]
assert claw_mapping["exclusions"][0]["contracts"] == returns and len(returns) == 13
assert {s["contracts"][0]["giver_vnum"] for s in claw_mapping["stories"] if s["category"] == "service"} == {80706, 80707, 80708, 80709, 80710, 80727}
assert claw_mapping["stories"][0]["contracts"] == [{"giver_vnum": 80724, "completion_key": "give=I:80734;receive=I:80728;disappear=0"}]
assert any(r["command"] == "D" and r["arguments"][1:4] == [80773, 3, 12] for r in claw["reset_commands"])
assert any(r["command"] == "O" and r["arguments"][1:5] == [80747, 1, 80785, 100] for r in claw["reset_commands"])
assert any(r["command"] == "M" and r["arguments"][1:5] == [80735, 1, 80775, 100] for r in claw["reset_commands"])
assert any(r["command"] == "G" and r["arguments"][1:5] == [80733, 1, 0, 100] for r in claw["reset_commands"])
assert not any(r["command"] in ("O", "P", "G", "E") and r["arguments"][1] == 80734 for r in claw["reset_commands"])
long = inventory_module.area_evidence(ROOT, "long")
assert len(long["requests"]) == 15 and len(long["dialogue"]) == 59
assert len(long["reset_commands"]) == 213 and len(long["mobs"]) == 77 and len(long["items"]) == 74
assert long["zone"]["reset_mode"] == 2 and not long["special_assignments"]
long_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "long")
long_contacts = {c["mob_vnum"]: c for c in long_mapping["contacts"]}
assert len(long_contacts) == 41 and {"master", "gear"} <= set(long_contacts[34440]["topics"])
assert "master's" not in long_contacts[34440]["topics"]
assert all("qc_action" not in c["topics"] for c in long_contacts.values())
long_responses = [b for b in inventory_module.native_blocks(ROOT)
                  if b["source"] == "areas/qst/long.qst" and b["kind"] == "M"]
assert len(long_responses) == 63
long_addressed = [b for b in long_responses if "qc_action" not in b["body"][0].split("~")[0].split()]
assert len(long_addressed) == 60
for response in long_addressed:
    assert set(response["body"][0].rstrip("~").split()) & set(long_contacts[response["giver_vnum"]]["topics"])
for vnum, contact in long_contacts.items():
    assert contact["keyword"] in long["mobs"][vnum]["keywords"]
long_stories = {s["id"]: s for s in long_mapping["stories"]}
assert long_stories["proof-against-the-siege-leaders"]["contracts"] == [{"giver_vnum": 34445, "completion_key": "give=I:34427,I:34428,I:34429,I:34430,I:34431;receive=I:34452,I:34464;disappear=0"}]
assert long_stories["recognition-by-selunes-solar"]["contracts"] == [{"giver_vnum": 34417, "completion_key": "give=I:34452;receive=I:34453;disappear=1"}]
assert long_mapping["exclusions"][0]["contracts"] == [{"giver_vnum": 34416, "completion_key": "give=;receive=;disappear=0"}]
cash_request = next(r for r in long["requests"] if ("I", 34445) in r["block"]["receive"])
assert ("C", 25000) in cash_request["block"]["receive"] and cash_request["definition"]["daily_eligible"]
paid = [r for r in long["requests"] if any(k == "C" for k, _ in r["block"]["give"])]
assert len(paid) == 4 and all(r["definition"]["daily_exclusion"] == "Unsupported durable offering" for r in paid)
parent = room = None
duplicate_sources = {}
for reset in long["reset_commands"]:
    command, values = reset["command"], reset["arguments"]
    if command in ("M", "F"):
        parent, room = values[1], values[3]
    elif command == "G" and values[1] in (34406, 34413, 34418):
        duplicate_sources[values[1]] = (parent, room, values[2], values[4])
assert duplicate_sources == {34406: (34452, 34435, 1, 75), 34413: (34451, 34410, 1, 80), 34418: (34466, 34419, 1, 80)}
assert sum(r["command"] == "D" for r in long["reset_commands"]) == 14
assert all(r["arguments"][3] == 0 for r in long["reset_commands"] if r["command"] == "D")
pearl = inventory_module.area_evidence(ROOT, "blackpearl")
assert len(pearl["requests"]) == 31 and len(pearl["dialogue"]) == 14
assert len(pearl["reset_commands"]) == 391 and len(pearl["mobs"]) == 63 and len(pearl["items"]) == 98
assert pearl["zone"]["reset_mode"] == 0 and not pearl["special_assignments"]
pearl_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "blackpearl")
pearl_contacts = {c["mob_vnum"]: c for c in pearl_mapping["contacts"]}
assert len(pearl_contacts) == 26
for response in pearl["dialogue"]:
    assert set(response["body"][0].rstrip("~").split()) & set(pearl_contacts[response["giver_vnum"]]["topics"])
for vnum, contact in pearl_contacts.items():
    assert contact["keyword"] in pearl["mobs"][vnum]["keywords"]
pearl_stories = {s["id"]: s for s in pearl_mapping["stories"]}
assert pearl_stories["reconstruct-warthehrs-dragonslayer"]["contracts"] == [{"giver_vnum": 142216, "completion_key": "give=I:142204,I:142205,I:142206,I:142208,I:142209,I:142224,I:142226,I:142227,I:142228;receive=I:142249;disappear=1"}]
assert pearl_stories["lyles-letter-and-abals-keys"]["contracts"] == [{"giver_vnum": 142229, "completion_key": "give=I:142214;receive=I:26201,I:142234;disappear=0"}]
pearl_givers = {r["block"]["giver_vnum"] for r in pearl["requests"]}
placements = [r["arguments"] for r in pearl["reset_commands"] if r["command"] == "M" and r["arguments"][1] in pearl_givers]
assert len(placements) == len(pearl_givers) == 13 and all(v[3] == 142200 for v in placements)
source_ids = set()
for row in rows:
    area = row["zone"]["source_area"]
    reset_path = ROOT / "areas/zon" / f"{area}.zon"
    for raw in reset_path.read_text(errors="replace").splitlines():
        match = re.match(r"^([MOGEPF])\s+((?:-?\d+\s*)+)", raw)
        if not match:
            continue
        command, values = match[1], list(map(int, match[2].split()))
        if area != "blackpearl" and command in ("M", "F"):
            assert not 142215 <= values[1] <= 142234
        if command in ("O", "G", "E", "P"):
            source_ids.add(values[1])
assert {142204, 142207, 142216, 142217, 142218} <= source_ids
assert not source_ids & {142206, 142225, 142231, 142233, 142235}
pearl_candidates = [r for r in pearl["requests"] if r["definition"]["daily_eligible"]]
assert len(pearl_candidates) == 13
assert all(not r["definition"]["repeatable"] for r in pearl["requests"] if r["block"]["disappear"])
travel = next(r for r in pearl["requests"] if ("I", 142207) in r["block"]["give"])
assert ("C", 100000) in travel["block"]["receive"] and travel["definition"]["daily_eligible"]
raven = inventory_module.area_evidence(ROOT, "ravenloft2")
assert len(raven["requests"]) == 37 and len(raven["dialogue"]) == 135
assert len(raven["mobs"]) == 98 and len(raven["items"]) == 327 and len(raven["reset_commands"]) == 1457
assert raven["zone"]["reset_mode"] == 0 and not raven["special_assignments"]
raven_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "ravenloft2")
raven_contacts = {c["mob_vnum"]: c for c in raven_mapping["contacts"]}
assert len(raven_contacts) == 25
for response in raven["dialogue"]:
    assert set(response["body"][0].rstrip("~").split()) & set(raven_contacts[response["giver_vnum"]]["topics"])
for vnum, contact in raven_contacts.items():
    assert contact["keyword"] in raven["mobs"][vnum]["keywords"]
assert sum(r["definition"]["daily_eligible"] for r in raven["requests"]) == 26
assert all(not r["definition"]["repeatable"] and not r["definition"]["daily_eligible"] for r in raven["requests"] if r["block"]["disappear"])
raven_stories = {s["id"]: s for s in raven_mapping["stories"]}
assert raven_stories["the-chaplains-favor"]["contracts"] == [{"giver_vnum": 59070, "completion_key": "give=I:59202,I:59202,I:59202,I:59202,I:59202,I:59281;receive=I:59294,I:59314,I:59315;disappear=0"}]
assert raven_stories["izeks-elven-wine"]["contracts"] == [{"giver_vnum": 59084, "completion_key": "give=I:59035;receive=C:250000;disappear=0"}]
assert raven_stories["ezmereldas-paid-reading"]["contracts"] == [{"giver_vnum": 59060, "completion_key": "give=C:10000;receive=;disappear=0"}]
# Existing toy stock is real even though the replenishing shop list differs.
parent = None
toy_sources = []
for reset in raven["reset_commands"]:
    if reset["command"] in ("M", "F"):
        parent = reset["arguments"][1]
    if reset["command"] == "G" and reset["arguments"][1] == 59252:
        toy_sources.append(parent)
assert toy_sources == [59081]
shop_source = (ROOT / "areas/shp/ravenloft2.shp").read_text()
assert "#59081~" in shop_source and "#59097~" in shop_source
assert "59252" not in shop_source
barovia = inventory_module.area_evidence(ROOT, "barovia")
assert len(barovia["requests"]) == 9 and len(barovia["dialogue"]) == 41
assert len(barovia["mobs"]) == 57 and len(barovia["items"]) == 66 and len(barovia["reset_commands"]) == 405
assert barovia["zone"]["reset_mode"] == 0 and not barovia["special_assignments"]
assert not (ROOT / "areas/shp/barovia.shp").exists()
barovia_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "barovia")
barovia_contacts = {c["mob_vnum"]: c for c in barovia_mapping["contacts"]}
assert len(barovia_contacts) == 24
for response in barovia["dialogue"]:
    assert set(response["body"][0].rstrip("~").split()) & set(barovia_contacts[response["giver_vnum"]]["topics"])
for vnum, contact in barovia_contacts.items():
    assert contact["keyword"] in barovia["mobs"][vnum]["keywords"]
barovia_stories = {s["id"]: s for s in barovia_mapping["stories"]}
assert barovia_stories["bildraths-nine-trinkets"]["contracts"] == [{"giver_vnum": 91007, "completion_key": "give=I:91015,I:91018,I:91026,I:91027,I:91042,I:91043,I:91044,I:91045,I:91046;receive=I:91047;disappear=0"}]
assert {t["item_vnums"][0] for t in barovia_stories["bildraths-nine-trinkets"]["steps"] if t["kind"] == "carried_item" and not t.get("optional")} == {91015, 91018, 91026, 91027, 91042, 91043, 91044, 91045, 91046}
assert barovia_stories["kolyans-forged-letter"]["contracts"] == [{"giver_vnum": 91011, "completion_key": "give=I:91022;receive=C:200000;disappear=0"}]
assert barovia_stories["hossas-ambush-plan"]["contracts"] == [{"giver_vnum": 91011, "completion_key": "give=I:91038;receive=I:91039;disappear=0"}]
assert barovia_stories["parriwimples-collection-clue"]["category"] == "service"
assert barovia_stories["parriwimples-collection-clue"]["contracts"] == [{"giver_vnum": 91018, "completion_key": "give=C:5000;receive=;disappear=0"}]
assert all(barovia_stories[name]["category"] == "service" for name in ("ireenas-first-letter-guidance", "ireenas-ambush-plan-guidance"))
assert sum(t.get("optional", False) for story in barovia_mapping["stories"] for t in story["steps"]) == 10
assert sum(r["definition"]["daily_eligible"] for r in barovia["requests"]) == 4
assert all(not r["definition"]["repeatable"] and not r["definition"]["daily_eligible"] for r in barovia["requests"] if r["block"]["disappear"])
barovia_units = [u for u in catalog_module.story_units(catalog) if u["zone_number"] == 910]
assert len(barovia_units) == 9 and sum(u["achievement"] for u in barovia_units) == 6 and sum(u["daily_candidate"] for u in barovia_units) == 4
parent = None
barovia_sources = {}
for reset in barovia["reset_commands"]:
    command, values = reset["command"], reset["arguments"]
    if command in ("M", "F"):
        parent = values[1]
    if command in ("G", "E") and values[1] in {91021, 91036, 91038, 91042, 91044, 91045, 91046}:
        barovia_sources[values[1]] = (command, parent, values[2])
    if command == "P" and values[1] in {91018, 91022}:
        barovia_sources[values[1]] = (command, values[3], values[2])
    if command == "O" and values[1] in {91015, 91026, 91027, 91043}:
        barovia_sources[values[1]] = (command, values[3], values[2])
assert barovia_sources == {91015: ("O", 91116, 1), 91018: ("P", 91017, 1), 91021: ("E", 91033, 1), 91022: ("P", 91010, 1), 91026: ("O", 91151, 1), 91027: ("O", 91135, 1), 91036: ("G", 91031, 1), 91038: ("G", 91046, 1), 91042: ("G", 91024, 1), 91043: ("O", 91125, 1), 91044: ("G", 91035, 1), 91045: ("E", 91048, 1), 91046: ("G", 91026, 1)}

tikitt = inventory_module.area_evidence(ROOT, "tikitt")
assert len(tikitt["requests"]) == 29 and len(tikitt["dialogue"]) == 9
assert len(tikitt["mobs"]) == 76 and len(tikitt["items"]) == 97 and len(tikitt["reset_commands"]) == 340
assert tikitt["zone"]["reset_mode"] == 0 and not (ROOT / "areas/shp/tikitt.shp").exists()
assert {(a["kind"], a["vnum"], a["function"]) for a in tikitt["special_assignments"]} == {
    ("obj", 44170, "artifact_hide"), ("obj", 44179, "madman_mangler"),
    ("obj", 44172, "madman_shield"), ("obj", 44188, "mentality_mace"),
    ("obj", 44165, "unmulti_altar")}
tikitt_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "tikitt")
tikitt_contacts = {c["mob_vnum"]: c for c in tikitt_mapping["contacts"]}
assert len(tikitt_contacts) == 19
for response in tikitt["dialogue"]:
    assert set(response["body"][0].rstrip("~").split()) & set(tikitt_contacts[response["giver_vnum"]]["topics"])
for vnum, contact in tikitt_contacts.items():
    assert contact["keyword"] in tikitt["mobs"][vnum]["keywords"]
    assert re.fullmatch(r"[a-z0-9_-]{1,64}", contact["keyword"])
tikitt_stories = {s["id"]: s for s in tikitt_mapping["stories"]}
assert collections.Counter(s["category"] for s in tikitt_stories.values()) == {"story": 2, "request": 2, "service": 25}
assert sum(t.get("optional", False) for story in tikitt_stories.values() for t in story["steps"]) == 44
native_tikitt = {(r["block"]["giver_vnum"], r["block"]["binding"]["completion_key"]): r["block"] for r in tikitt["requests"]}
for story in tikitt_stories.values():
    ref = story["contracts"][0]
    block = native_tikitt[(ref["giver_vnum"], ref["completion_key"])]
    required = {step["item_vnums"][0]: step["count"] for step in story["steps"] if step["kind"] == "carried_item" and not step.get("optional")}
    assert required == collections.Counter(v for k, v in block["give"] if k == "I")
    assert story["steps"][-1]["contracts"] == story["contracts"]
    optional_receipts = [(r["giver_vnum"], r["completion_key"]) for step in story["steps"] if step.get("optional") and step["kind"] == "completion" for r in step["contracts"]]
    assert len(optional_receipts) == len(set(optional_receipts))
assert {t["item_vnums"][0] for t in tikitt_stories["assemble-the-royal-treasure-key"]["steps"] if t["kind"] == "carried_item" and not t.get("optional")} == {44115, 44116, 44117, 44118, 44122}
assert {t["item_vnums"][0] for t in tikitt_stories["merge-three-sapphire-kinds"]["steps"] if t["kind"] == "carried_item" and not t.get("optional")} == {44164, 43703, 43752, 43753}
assert {t["item_vnums"][0] for t in tikitt_stories["merge-eight-flesh-ring-kinds"]["steps"] if t["kind"] == "carried_item" and not t.get("optional")} == {44164, 43705, 43707, 43710, 43714, 43715, 43717, 43718, 43739}
assert sum(r["definition"]["daily_eligible"] for r in tikitt["requests"]) == 29
tikitt_units = [u for u in catalog_module.story_units(catalog) if u["zone_number"] == 441]
assert len(tikitt_units) == 29 and sum(u["achievement"] for u in tikitt_units) == 4 and sum(u["daily_candidate"] for u in tikitt_units) == 4
parent = room = None
tikitt_sources = collections.defaultdict(list)
for reset in tikitt["reset_commands"]:
    command, values = reset["command"], reset["arguments"]
    if command in ("M", "F"):
        parent, room = values[1], values[3]
    if command in ("G", "E"):
        tikitt_sources[values[1]].append((command, parent, room, values[2]))
    if command == "P":
        tikitt_sources[values[1]].append((command, values[3], None, values[2]))
assert tikitt_sources[44189] == [("P", 44186, None, 1)]  # Cot, not its keyed rack.
assert tikitt_sources[44164] == [("P", 44165, None, 1)]  # Orb is inside the altar.
assert tikitt_sources[44122] == [("G", 44151, 44295, 1)]
assert any(r["command"] == "M" and r["arguments"][1:4] == [44101, 1, 44313] for r in tikitt["reset_commands"])
assert any(r["command"] == "O" and r["arguments"][1:4] == [44100, 1, 44290] for r in tikitt["reset_commands"])
assert any(r["command"] == "D" and r["arguments"][1:4] == [44290, 5, 14] for r in tikitt["reset_commands"])
assert not any(r["command"] == "D" and r["arguments"][1:3] == [44332, 2] for r in tikitt["reset_commands"])

jade = inventory_module.area_evidence(ROOT, "jade")
assert len(jade["requests"]) == 37 and len(jade["dialogue"]) == 3
assert len(jade["mobs"]) == 134 and len(jade["items"]) == 130
assert len(jade["reset_commands"]) == 583
assert collections.Counter(r["command"] for r in jade["reset_commands"]) == {"M": 298, "D": 76, "E": 71, "O": 70, "P": 34, "G": 30, "F": 4}
assert {(a["kind"], a["vnum"], a["function"]) for a in jade["special_assignments"]} == {("room", 76859, "crew_shop_proc"), ("room", 76659, "ship_shop_proc")}
jade_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "jade")
assert jade_mapping["schema_version"] == 3 and jade_mapping["coverage"] == "complete"
jade_contacts = {c["mob_vnum"]: c for c in jade_mapping["contacts"]}
assert len(jade_contacts) == 38 and {r["block"]["giver_vnum"] for r in jade["requests"]} <= jade_contacts.keys()
assert jade_contacts[76688]["topics"] == ["hi", "mande", "troggahn"]
for vnum, contact in jade_contacts.items():
    assert contact["keyword"] in jade["mobs"][vnum]["keywords"]
    assert re.fullmatch(r"[a-z0-9_-]{1,64}", contact["keyword"])
jade_stories = {s["id"]: s for s in jade_mapping["stories"]}
assert collections.Counter(s["category"] for s in jade_stories.values()) == {"story": 8, "request": 9, "service": 17}
assert sum(t.get("optional", False) for s in jade_stories.values() for t in s["steps"]) == 35
native_jade = {(r["block"]["giver_vnum"], r["block"]["binding"]["completion_key"]): r["block"] for r in jade["requests"]}
for story in jade_stories.values():
    assert story["steps"][-1]["contracts"] == story["contracts"]
    required = {s["item_vnums"][0]: s["count"] for s in story["steps"] if s["kind"] == "carried_item" and not s.get("optional")}
    if story["id"] == "one-fish-for-the-fisherman":
        assert {r["completion_key"] for r in story["contracts"]} == {"give=I:318;receive=C:10000;disappear=0", "give=I:319;receive=C:10000;disappear=0"}
        assert story["steps"][0]["item_vnums"] == [318, 319] and story["steps"][0]["count"] == 1
    else:
        ref = story["contracts"][0]
        block = native_jade[(ref["giver_vnum"], ref["completion_key"])]
        assert required == collections.Counter(v for k, v in block["give"] if k == "I")
    optional_receipts = [(r["giver_vnum"], r["completion_key"]) for s in story["steps"] if s.get("optional") and s["kind"] == "completion" for r in s["contracts"]]
    assert len(optional_receipts) == len(set(optional_receipts))
assert {r["completion_key"] for x in jade_mapping["exclusions"] for r in x["contracts"]} == {"give=I:76620;receive=C:0;disappear=1", "give=I:76706;receive=;disappear=0"}
assert jade_stories["the-daimyos-heart-briefing"]["contracts"] == [{"giver_vnum": 76669, "completion_key": "give=I:76665;receive=I:76665;disappear=0"}]
assert jade_stories["macavors-two-proofs"]["contracts"] == [{"giver_vnum": 76688, "completion_key": "give=I:67116,I:76730;receive=I:32019,I:32019,I:55324;disappear=0"}]
princess_preparation = next(s for s in jade_stories["the-princesss-royal-token"]["steps"] if s["id"] == "princess-preparation")
assert princess_preparation["optional"] and princess_preparation["contracts"] == [{"giver_vnum": 77214, "completion_key": "give=I:77204;receive=I:77205;disappear=1"}]
jade_units = [u for u in catalog_module.story_units(catalog) if u["zone_number"] == 766]
assert len(jade_units) == 34 and sum(u["achievement"] for u in jade_units) == 17 and sum(u["daily_candidate"] for u in jade_units) == 17
assert sum(r["definition"]["daily_eligible"] for r in jade["requests"]) == 31
jade_sources = collections.defaultdict(list)
parent = room = None
for reset in jade["reset_commands"]:
    command, values = reset["command"], reset["arguments"]
    if command in ("M", "F"):
        parent, room = values[1], values[3]
    if command in ("G", "E"):
        jade_sources[values[1]].append((command, parent, room, values[2], values[4]))
    elif command in ("O", "P"):
        jade_sources[values[1]].append((command, values[3], None, values[2], values[4]))
assert jade_sources[76623] == [("P", 76622, None, 1, 100)]
assert jade_sources[76660] == [("P", 76622, None, 1, 33)]
assert jade_sources[76679] == [("O", 76910, None, 1, 33)]
assert jade_sources[76678] == [("P", 76664, None, 1, 20)]
assert len(jade_sources[76690]) == 5 and all(s[3:] == (5, 20) for s in jade_sources[76690])
assert len(jade_sources[76608]) == 8 and all(s[3:] == (8, 20) for s in jade_sources[76608])
assert len(jade_sources[76619]) == 4 and all(s[3:] == (4, 100) for s in jade_sources[76619])
assert jade_sources[76688] == [("E", 76712, 76915, 1, 100)]
assert jade_sources[76710] == [("G", 76725, 76938, 1, 100)]
assert not jade_sources[233]

savannah = inventory_module.area_evidence(ROOT, "savannah")
assert len(savannah["requests"]) == 17 and len(savannah["dialogue"]) == 17
assert len(savannah["mobs"]) == 59 and len(savannah["items"]) == 39
assert len(savannah["reset_commands"]) == 315 and not savannah["special_assignments"]
assert collections.Counter(r["command"] for r in savannah["reset_commands"]) == {"M": 200, "E": 73, "F": 20, "G": 16, "D": 6}
savannah_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "savannah")
assert savannah_mapping["schema_version"] == 3 and savannah_mapping["coverage"] == "complete"
savannah_contacts = {c["mob_vnum"]: c for c in savannah_mapping["contacts"]}
assert len(savannah_contacts) == 17 and {r["block"]["giver_vnum"] for r in savannah["requests"]} <= savannah_contacts.keys()
all_native_blocks = inventory_module.native_blocks(ROOT)
raw_savannah_topics = [b for b in all_native_blocks if b["source"] == "areas/qst/savannah.qst" and b["kind"] == "M"]
assert len(raw_savannah_topics) == 19
# Plain command suggestions must survive a second source alias containing an
# apostrophe, even while the conservative evidence index omits that family.
for block in raw_savannah_topics:
    aliases = block["body"][0].split("~")[0].lower().split()
    safe = [a for a in aliases if re.fullmatch(r"[a-z0-9_-]{1,64}", a)]
    assert safe and safe[0] in savannah_contacts[block["giver_vnum"]]["topics"]
assert savannah_contacts[138527]["topics"] == ["contest"]
assert savannah_contacts[138545]["topics"] == ["contest"]
for vnum, contact in savannah_contacts.items():
    assert contact["keyword"] in savannah["mobs"][vnum]["keywords"]
savannah_stories = {s["id"]: s for s in savannah_mapping["stories"]}
assert collections.Counter(s["category"] for s in savannah_stories.values()) == {"story": 2, "request": 3, "service": 12}
assert not savannah_mapping["exclusions"]
assert sum(t.get("optional", False) for s in savannah_stories.values() for t in s["steps"]) == 12
native_savannah = {(r["block"]["giver_vnum"], r["block"]["binding"]["completion_key"]): r["block"] for r in savannah["requests"]}
assert {(r["giver_vnum"], r["completion_key"]) for s in savannah_stories.values() for r in s["contracts"]} == set(native_savannah)
legendary = {"lyre": (138267, 138535), "drums": (138268, 138533), "horn": (138269, 138534), "flute": (138271, 138536), "mandolin": (138272, 138537), "harp": (138270, 138538)}
kunji_binding = {"giver_vnum": 138261, "completion_key": "give=I:138267,I:138268,I:138269,I:138270,I:138271,I:138272;receive=I:138279;disappear=1"}
for name, (katana, base) in legendary.items():
    initial = savannah_stories["legendary-" + name]
    epic = savannah_stories["epic-" + name]
    assert initial["contracts"] == [{"giver_vnum": 138500, "completion_key": f"give=I:{katana};receive=I:{base};disappear=0"}]
    assert epic["contracts"] == [{"giver_vnum": 138500, "completion_key": f"give=I:138279,I:{base};receive=I:{base + 6};disappear=1"}]
    history = {t["id"]: t for t in epic["steps"] if t["kind"] == "completion" and t.get("optional")}
    assert history["matching-base-preparation"]["contracts"] == initial["contracts"]
    assert history["retribution-preparation"]["contracts"] == [kunji_binding]
    assert "leaves after" in epic["steps"][-1]["hint"]
for story in savannah_stories.values():
    binding = story["contracts"][0]
    block = native_savannah[(binding["giver_vnum"], binding["completion_key"])]
    required = collections.Counter({t["item_vnums"][0]: t["count"] for t in story["steps"] if t["kind"] == "carried_item" and not t.get("optional")})
    assert required == collections.Counter(n for k, n in block["give"] if k == "I")
savannah_units = [u for u in catalog_module.story_units(catalog) if u["zone_number"] == 1385]
assert len(savannah_units) == 17 and sum(u["achievement"] for u in savannah_units) == 5 and sum(u["daily_candidate"] for u in savannah_units) == 5
assert sum(r["definition"]["daily_eligible"] for r in savannah["requests"]) == 17
savannah_sources = collections.defaultdict(list)
parent = room = None
for reset in savannah["reset_commands"]:
    command, values = reset["command"], reset["arguments"]
    if command in {"M", "F"}:
        parent, room = values[1], values[3]
    if command in {"G", "E"}:
        savannah_sources[values[1]].append((command, parent, room, values[2], values[4]))
assert savannah_sources[138526] == [("G", 138522, 138519, 3, 100), ("G", 138523, 138519, 3, 100), ("G", 138522, 138519, 3, 100)]
assert savannah_sources[138528] == [("G", 138517, 138538, 3, 100), ("G", 138515, 138538, 3, 100), ("G", 138515, 138578, 3, 100)]
assert savannah_sources[138530] == [("G", 138519, 138533, 1, 100)]
assert savannah_sources[138516] == [("G", 138537, 138512, 1, 100)]
assert savannah_sources[138514] == [("G", 138546, 138629, 1, 100)]
assert not any(r["command"] in {"M", "F"} and r["arguments"][1] == 138558 for r in savannah["reset_commands"])
assert "hostel" not in {z["source_area"] for z in catalog["zones"]}
assert not any(138539 <= n <= 138544 for b in all_native_blocks for k, n in b["give"] if k == "I")

forge = inventory_module.area_evidence(ROOT, "alatorin")
assert len(forge["requests"]) == 495 and len(forge["dialogue"]) == 291
assert len(forge["reset_commands"]) == 3596
assert len(forge["mobs"]) == 425 and len(forge["items"]) == 602
forge_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "alatorin")
assert forge_mapping["coverage"] == "complete" and forge_mapping["schema_version"] == 3
assert len(forge_mapping["stories"]) == 253
assert collections.Counter(s["category"] for s in forge_mapping["stories"]) == {"story": 18, "request": 72, "service": 163}
assert sum(t.get("optional", False) for s in forge_mapping["stories"] for t in s["steps"]) == 548
forge_contacts = {c["mob_vnum"]: c for c in forge_mapping["contacts"]}
assert len(forge_contacts) == 94
raw_forge_topics = [b for b in all_native_blocks if b["source"] == "areas/qst/alatorin.qst" and
                    "binding" not in b and b["body"] and "qc_action" not in b["body"][0].split("~")[0].split()]
assert len(raw_forge_topics) == 296
for response in raw_forge_topics:
    assert set(response["body"][0].split("~")[0].split()) & set(forge_contacts[response["giver_vnum"]]["topics"])
for vnum, contact in forge_contacts.items():
    assert contact["keyword"] in forge["mobs"][vnum]["keywords"]
assert "ardgral" in forge_contacts[83141]["topics"]
assert "grtak" in forge_contacts[83436]["topics"] or "leaders" in forge_contacts[83436]["topics"]

def forge_rows(giver, reward=None):
    return [s for s in forge_mapping["stories"] if any(c["giver_vnum"] == giver and
            (reward is None or f"I:{reward}" in c["completion_key"].split(";")[1]) for c in s["contracts"])]

# The large support family keeps all ordinary exact alternatives and no false
# aggregate live-material check. Nebula/vellum/random/token rewards stay distinct.
ordinary = next(s for s in forge_mapping["stories"] if s["id"] == "collecting-salvaged-material-fragments")
assert ordinary["category"] == "service" and len(ordinary["contracts"]) == 209
assert {c["completion_key"] for c in ordinary["contracts"]} == {
    f"give=I:{v};receive=I:83245;disappear=0" for v in range(400000, 400209)}
assert all(t["kind"] == "completion" for t in ordinary["steps"])
assert any(c["completion_key"] == "give=I:400209;receive=C:200000,I:83245;disappear=0"
           for s in forge_mapping["stories"] for c in s["contracts"])
assert any(c["completion_key"].startswith("give=" + ",".join(["I:83458"] * 10) + ";receive=I:83245;")
           for s in forge_mapping["stories"] for c in s["contracts"])
# Repeated quantities must be checked per exact kind: two mixed arcanum/scroll
# ingredients, six mixed metals or eight mixed woods cannot look recipe-ready.
for story in forge_mapping["stories"]:
    for step in story["steps"]:
        if step["kind"] == "carried_item":
            assert len(step["item_vnums"]) == 1 and step["optional"]
for reward, alternatives in ((83685, 5), (83686, 5), (83687, 4)):
    row, = forge_rows(83140, reward)
    assert len(row["contracts"]) == alternatives
    components = [t for t in row["steps"] if t["kind"] == "carried_item" and t["count"] == 2]
    assert len(components) == alternatives
assert len(forge_rows(83291)) == 50 and sum(len(s["contracts"]) for s in forge_rows(83291)) == 55
assert len(forge_rows(83391)) == 4 and sum(len(s["contracts"]) for s in forge_rows(83391)) == 6
assert len(forge_rows(83342, 31544)) == 2  # Overlapping fee/token recipes remain separate.
buybacks = forge_rows(83302)
assert len(buybacks) == 11 and all(s["category"] == "service" for s in buybacks)
prices = {s["contracts"][0]["completion_key"] for s in buybacks}
assert "give=I:80804;receive=C:60000;disappear=0" in prices
assert "give=I:81403;receive=C:75000;disappear=0" in prices
assert len(forge_rows(83383)) == 7  # Food groups preserve each distinct XP tier.
returned = forge_mapping["exclusions"][0]["contracts"]
assert len(returned) == 4 and {c["giver_vnum"] for c in returned} == {83281, 83288, 83483}
for giver in (83289, 83415, 83508, 83517, 83518, 83519, 83521):
    row, = forge_rows(giver)
    assert row["category"] == "request" and "receive=" in row["contracts"][0]["completion_key"]
forge_units = [u for u in catalog_module.story_units(catalog) if u["zone_number"] == 831]
assert len(forge_units) == 253 and sum(u["achievement"] for u in forge_units) == 90
assert sum(u["daily_candidate"] for u in forge_units) == 82
assert (ROOT / "areas/story/alatorin.story.json").stat().st_size <= catalog_module.MAX_STORY_MAPPING_BYTES

haven = inventory_module.area_evidence(ROOT, "newhaven")
assert len(haven["requests"]) == 9 and len(haven["dialogue"]) == 2
assert len(haven["mobs"]) == 91 and len(haven["items"]) == 44
assert len(haven["reset_commands"]) == 252 and haven["zone"]["reset_mode"] == 2
haven_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "newhaven")
assert haven_mapping["coverage"] == "complete" and haven_mapping["schema_version"] == 3
assert collections.Counter(s["category"] for s in haven_mapping["stories"]) == {"story": 1, "request": 2, "service": 6}
assert not haven_mapping["exclusions"]
assert sum(t.get("optional", False) for s in haven_mapping["stories"] for t in s["steps"]) == 12
haven_contacts = {c["mob_vnum"]: c for c in haven_mapping["contacts"]}
assert len(haven_contacts) == 17
for response in haven["dialogue"]:
    assert set(response["body"][0].rstrip("~").split()) & set(haven_contacts[response["giver_vnum"]]["topics"])
for vnum, contact in haven_contacts.items():
    assert contact["keyword"] in haven["mobs"][vnum]["keywords"]
assert all(not c["topics"] for v, c in haven_contacts.items() if v not in (35204, 35211))
haven_stories = {s["id"]: s for s in haven_mapping["stories"]}
assert {s["contracts"][0]["giver_vnum"] for s in haven_mapping["stories"] if s["category"] != "service"} == {35204, 35213, 35216}
# Preserve the implemented pipe identity while treating the mismatched merchant
# trade as a service; ambient line/reel prose must not silently change receipts.
pipe = haven_stories["dibblys-snorkel-pipe-buyback"]
assert pipe["category"] == "service" and pipe["contracts"] == [{"giver_vnum": 35216, "completion_key": "give=I:88905;receive=C:5000;disappear=0"}]
history, = [t for t in pipe["steps"] if t["kind"] == "completion" and t.get("optional")]
assert history["contracts"] == [{"giver_vnum": 88907, "completion_key": "give=I:88909,I:88909,I:88909,I:88909;receive=E:7500,I:88905;disappear=0"}]
assert any(t["kind"] == "carried_item" and t["item_vnums"] == [88909] and t["count"] == 4 and t["optional"] for t in pipe["steps"])
for name, fee, reward in (("scale-and-balance-badge", 45000, 35228), ("hammer-and-anvil-badge", 55000, 35239)):
    badge = haven_stories[name]
    assert badge["category"] == "service" and badge["contracts"] == [{"giver_vnum": 35211, "completion_key": f"give=C:{fee},I:93901;receive=I:{reward};disappear=0"}]
assert haven_stories["vulgaris-veldian-collar"]["contracts"] == [{"giver_vnum": 35286, "completion_key": "give=C:100000,I:13221,I:98606;receive=I:35224;disappear=0"}]
haven_units = [u for u in catalog_module.story_units(catalog) if u["zone_number"] == 352]
assert len(haven_units) == 9 and sum(u["achievement"] for u in haven_units) == 3
assert sum(u["daily_candidate"] for u in haven_units) == 3
assert sum(r["definition"]["daily_eligible"] for r in haven["requests"]) == 4
paid = [r for r in haven["requests"] if any(k == "C" for k, _ in r["block"]["give"])]
assert len(paid) == 5 and all(r["definition"]["daily_exclusion"] == "Unsupported durable offering" for r in paid)
# Same-room lore, similar NPCs and repeated prototype appearances are not the
# item source. Keep actual G parents and independent ground declarations.
parent = room = None
material_sources = {}
ground = []
for reset in haven["reset_commands"]:
    c, v = reset["command"], reset["arguments"]
    if c in ("M", "F"):
        parent, room = v[1], v[3]
    if c == "G" and v[1] in (35226, 35227, 35237):
        material_sources[v[1]] = (parent, room, v[2], v[4])
    if c == "O" and v[1] in (35233, 35238):
        ground.append((v[1], v[2], v[3], v[4]))
assert material_sources == {35226: (35254, 35295, 1, 100), 35227: (35290, 35279, 1, 100), 35237: (35287, 35272, 1, 100)}
assert ground == [(35233, 2, 35267, 100), (35238, 1, 35270, 100), (35233, 2, 35279, 100)]
assert any(r["command"] == "M" and r["arguments"][1:5] == [35278, 1, 35270, 25] for r in haven["reset_commands"])
assert re.search(r"\bT\s+2\s+7\s+1\s+5\b", (ROOT / "areas/obj/newhaven.obj").read_text())
assert all(t.get("optional", False) and len(t["item_vnums"]) == 1 for s in haven_mapping["stories"] for t in s["steps"] if t["kind"] == "carried_item")

realm = inventory_module.area_evidence(ROOT, "realm")
assert len(realm["requests"]) == 7 and len(realm["dialogue"]) == 10
assert len(realm["mobs"]) == 74 and len(realm["items"]) == 123
assert len(realm["reset_commands"]) == 454 and realm["zone"]["reset_mode"] == 2
realm_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "realm")
assert realm_mapping["coverage"] == "complete" and realm_mapping["schema_version"] == 3
assert collections.Counter(s["category"] for s in realm_mapping["stories"]) == {"story": 3, "service": 2}
assert len(realm_mapping["exclusions"]) == 1
assert realm_mapping["exclusions"][0]["contracts"] == [{"giver_vnum": 14015, "completion_key": "give=I:14028;receive=I:14028;disappear=0"}]
realm_contacts = {c["mob_vnum"]: c for c in realm_mapping["contacts"]}
assert len(realm_contacts) == 10
for response in realm["dialogue"]:
    assert set(response["body"][0].rstrip("~").split()) & set(realm_contacts[response["giver_vnum"]]["topics"])
for vnum, contact in realm_contacts.items():
    assert contact["keyword"] in realm["mobs"][vnum]["keywords"]
assert all(not c["topics"] for v, c in realm_contacts.items() if v not in (14015, 14028, 14073, 14074))
realm_stories = {s["id"]: s for s in realm_mapping["stories"]}
signet = realm_stories["finns-lost-signet"]
key = realm_stories["finns-castle-key"]
blade = realm_stories["celriyas-family-blade"]
ring = realm_stories["finns-glowing-ring-trade"]
forge = realm_stories["the-five-plane-forge-service"]
assert signet["contracts"] == [{"giver_vnum": 14015, "completion_key": "give=I:14036;receive=I:14018,I:14041;disappear=0"}]
assert key["contracts"] == [{"giver_vnum": 14015, "completion_key": "give=I:14037;receive=C:500000,I:14023;disappear=1"}]
assert blade["contracts"] == [{"giver_vnum": 14028, "completion_key": "give=I:14001;receive=I:14011,I:14076;disappear=1"}]
assert ring["category"] == "service" and ring["contracts"] == [{"giver_vnum": 14015, "completion_key": "give=I:14018;receive=C:500,I:14041;disappear=0"}]
assert forge["category"] == "service" and forge["contracts"] == [{"giver_vnum": giver, "completion_key": "give=C:5000000,I:14121,I:14122,I:14123,I:14124,I:14125;receive=I:14126;disappear=0"} for giver in (14073, 14074)]
parts = [t for t in forge["steps"] if t["kind"] == "carried_item"]
assert [t["item_vnums"] for t in parts] == [[v] for v in range(14121, 14126)]
assert all(t["optional"] and t["count"] == 1 for t in parts)
earlier, = [t for t in key["steps"] if t["kind"] == "completion" and t.get("optional")]
assert earlier["contracts"] == signet["contracts"] + ring["contracts"]
assert any(t["kind"] == "carried_item" and t["item_vnums"] == [14034] for t in blade["steps"])
assert sum(t.get("optional", False) for s in realm_mapping["stories"] for t in s["steps"]) == 13
assert all(t.get("optional", False) and len(t["item_vnums"]) == 1 and t["count"] == 1 for s in realm_mapping["stories"] for t in s["steps"] if t["kind"] == "carried_item")
realm_units = [u for u in catalog_module.story_units(catalog) if u["zone_number"] == 140]
assert len(realm_units) == 5 and sum(u["achievement"] for u in realm_units) == 3
assert sum(u["daily_candidate"] for u in realm_units) == 3
assert sum(r["definition"]["daily_eligible"] for r in realm["requests"]) == 4
paid = [r for r in realm["requests"] if any(k == "C" for k, _ in r["block"]["give"])]
assert len(paid) == 2 and all(r["definition"]["daily_exclusion"] == "Unsupported durable offering" for r in paid)
# Initial reset parents distinguish actual supply from chamber/tomb prose.
parent = room = None
realm_sources = {}
makers = []
for reset in realm["reset_commands"]:
    c, v = reset["command"], reset["arguments"]
    if c in ("M", "F"):
        parent, room = v[1], v[3]
        if parent in (14073, 14074):
            makers.append((parent, room, v[2], v[4]))
    if c in ("G", "E") and v[1] in (14036, 14038, 14037):
        realm_sources[v[1]] = (c, parent, room, v[2], v[4])
assert realm_sources == {14036: ("E", 14026, 14199, 1, 100), 14038: ("E", 14026, 14199, 1, 100), 14037: ("G", 14071, 14158, 1, 100)}
assert makers == [(14073, 14209, 1, 100), (14074, 14209, 1, 100)]
assert any(r["command"] == "O" and r["arguments"][1:5] == [14001, 1, 14112, 100] for r in realm["reset_commands"])

verspin = inventory_module.area_evidence(ROOT, "verspin")
assert len(verspin["requests"]) == 12 and len(verspin["dialogue"]) == 9
assert len(verspin["mobs"]) == 79 and len(verspin["items"]) == 57
assert len(verspin["reset_commands"]) == 389 and verspin["zone"]["reset_mode"] == 2
assert {(a["kind"], a["vnum"], a["function"]) for a in verspin["special_assignments"]} == {("room", 28281, "stat_shops"), ("room", 28197, "crew_shop_proc")}
verspin_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "verspin")
assert verspin_mapping["coverage"] == "complete" and verspin_mapping["schema_version"] == 3
assert collections.Counter(s["category"] for s in verspin_mapping["stories"]) == {"story": 6, "service": 6}
assert not verspin_mapping["exclusions"]
verspin_contacts = {c["mob_vnum"]: c for c in verspin_mapping["contacts"]}
assert len(verspin_contacts) == 18
for response in verspin["dialogue"]:
    assert set(response["body"][0].rstrip("~").split()) & set(verspin_contacts[response["giver_vnum"]]["topics"])
for vnum, contact in verspin_contacts.items():
    assert contact["keyword"] in verspin["mobs"][vnum]["keywords"]
assert all(not c["topics"] for v, c in verspin_contacts.items() if v not in (28116, 28145, 28147, 28155, 28173))
verspin_stories = {s["id"]: s for s in verspin_mapping["stories"]}
totems = verspin_stories["tottans-five-totems"]
lion = verspin_stories["the-golden-lions-collar"]
symbols = verspin_stories["the-shrines-five-symbols"]
amulets = verspin_stories["transos-three-amulets"]
monk = verspin_stories["the-monks-corruption-sigil"]
bone = verspin_stories["ramous-apple-for-a-bone"]
assert totems["contracts"] == [{"giver_vnum": 28116, "completion_key": "give=I:28144,I:28144,I:28144,I:28144,I:28144;receive=E:65000,I:28145;disappear=0"}]
assert lion["contracts"] == [{"giver_vnum": 28128, "completion_key": "give=I:28113;receive=I:28114;disappear=1"}]
assert symbols["contracts"] == [{"giver_vnum": 28147, "completion_key": "give=I:28138,I:28138,I:28138,I:28138,I:28138;receive=E:50000,I:28139;disappear=1"}]
assert amulets["contracts"] == [{"giver_vnum": 28155, "completion_key": "give=I:28123,I:28124,I:28125;receive=E:70000,I:28141;disappear=0"}]
assert monk["contracts"] == [{"giver_vnum": 28173, "completion_key": "give=I:74298;receive=E:85000,I:28153;disappear=0"}]
assert verspin_stories["vulms-stolen-amethyst"]["contracts"] == [{"giver_vnum": 28145, "completion_key": "give=I:28146;receive=E:50000,I:223,I:224,I:225;disappear=0"}]
assert bone["category"] == "service" and bone["contracts"] == [{"giver_vnum": 28126, "completion_key": "give=I:28112;receive=I:28113;disappear=0"}]
earlier, = [t for t in lion["steps"] if t["kind"] == "completion" and t.get("optional")]
assert earlier["contracts"] == bone["contracts"]
assert [(t["item_vnums"], t["count"]) for t in amulets["steps"] if t["kind"] == "carried_item"] == [([28123], 1), ([28124], 1), ([28125], 1)]
assert totems["steps"][0]["item_vnums"] == [28144] and totems["steps"][0]["count"] == 5
assert symbols["steps"][0]["item_vnums"] == [28138] and symbols["steps"][0]["count"] == 5
knife = verspin_stories["lozins-hunting-knife"]
shield = verspin_stories["lozins-hunting-shield"]
assert [(t["item_vnums"], t["count"]) for t in knife["steps"] if t["kind"] == "carried_item"] == [([28124], 2), ([28125], 2)]
assert shield["steps"][0]["item_vnums"] == [28124] and shield["steps"][0]["count"] == 4
assert sum(t.get("optional", False) for s in verspin_mapping["stories"] for t in s["steps"]) == 18
assert all(t.get("optional", False) and len(t["item_vnums"]) == 1 for s in verspin_mapping["stories"] for t in s["steps"] if t["kind"] == "carried_item")
verspin_units = [u for u in catalog_module.story_units(catalog) if u["zone_number"] == 281]
assert len(verspin_units) == 12 and sum(u["achievement"] for u in verspin_units) == 6
assert sum(u["daily_candidate"] for u in verspin_units) == 6
assert sum(r["definition"]["daily_eligible"] for r in verspin["requests"]) == 7
paid = [r for r in verspin["requests"] if any(k == "C" for k, _ in r["block"]["give"])]
assert len(paid) == 5 and all(r["definition"]["daily_exclusion"] == "Unsupported durable offering" for r in paid)
assert {r["block"]["binding"]["completion_key"] for r in paid} == {s["contracts"][0]["completion_key"] for s in verspin_mapping["stories"] if s["id"].startswith("lozins-")}
# Exact reset parents expose simultaneous cap limits and foreign-proof ownership.
parent = room = None
verspin_sources = collections.defaultdict(list)
apple_stalls = []
for reset in verspin["reset_commands"]:
    c, v = reset["command"], reset["arguments"]
    if c in ("M", "F"):
        parent, room = v[1], v[3]
    if c == "G" and v[1] in (28123, 28124, 28125, 28138, 28144, 28142, 28150):
        verspin_sources[v[1]].append((parent, room, v[2], v[4]))
    if c == "P" and v[1] == 28112:
        apple_stalls.append((v[3], v[2], v[4]))
assert verspin_sources[28144] == [(28104, r, 5, 100) for r in (28104, 28106, 28107, 28111, 28116)]
assert verspin_sources[28138] == [(28103, r, 5, 100) for r in (28104, 28111, 28114, 28117, 28136)]
assert verspin_sources[28124] == [(28101, 28101, 3, 100), (28108, 28113, 3, 100), (28143, 28162, 3, 100)]
assert verspin_sources[28123] == [(28105, 28111, 3, 100), (28135, 28132, 3, 100), (28143, 28169, 3, 100)]
assert verspin_sources[28142] == [(28161, 28214, 1, 100)] and verspin_sources[28150] == [(28171, 28255, 1, 100)]
assert apple_stalls == [(28111, 2, 100), (28110, 2, 100)]
assert any(r["command"] == "F" and r["arguments"][1:5] == [28128, 1, 28159, 100] for r in verspin["reset_commands"])
for area, item, expected_parent, expected_room, expected_chance in (("mntcastl", 28146, 37191, 37481, 33), ("bs", 74298, 74249, 74917, 100)):
    foreign = inventory_module.area_evidence(ROOT, area)
    parent = room = None
    sources = []
    for reset in foreign["reset_commands"]:
        c, v = reset["command"], reset["arguments"]
        if c in ("M", "F"):
            parent, room = v[1], v[3]
        if c == "G" and v[1] == item:
            sources.append((parent, room, v[2], v[4]))
    assert sources == [(expected_parent, expected_room, 1, expected_chance)]

# Ship Yards: exact port collections, optional briefing, alternate crates.
shipy = inventory_module.area_evidence(ROOT, "shipy")
shipy_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "shipy")
shipy_stories = {s["id"]: s for s in shipy_mapping["stories"]}
assert shipy_mapping["coverage"] == "complete" and shipy_mapping["revision"] == 1
assert len(shipy_stories) == 25 and len(shipy_mapping["contacts"]) == 32
assert sum(s["category"] == "service" for s in shipy_stories.values()) == 6
assert sum(t.get("optional", False) for s in shipy_stories.values() for t in s["steps"]) == 34
assert {tuple(sorted(b.items())) for s in shipy_stories.values() for b in s["contracts"]} == {
    tuple(sorted(r["block"]["binding"].items())) for r in shipy["requests"]}
pol = shipy_stories["pols-lure-materials"]
briefing = shipy_stories["pols-refunded-briefing"]
assert pol["contracts"] == [{"giver_vnum": 43103, "completion_key": "give=I:43137,I:43137,I:43137,I:43137,I:43137,I:43138,I:43138,I:43138,I:43138,I:43138;receive=C:300000,E:125000;disappear=1"}]
assert briefing["contracts"] == [{"giver_vnum": 43103, "completion_key": "give=C:300000;receive=C:300000,I:43136;disappear=0"}]
assert pol["steps"][0]["optional"] and pol["steps"][0]["contracts"] == briefing["contracts"]
assert pol["steps"][1]["item_vnums"] == [43136] and pol["steps"][1]["optional"]
assert [(t["item_vnums"],t["count"]) for t in pol["steps"][2:-1]] == [([43137],5),([43138],5)]
for id, expected in {
    "voshens-six-pike": [(318,6)], "martineks-six-clams": [(334,6)],
    "krintis-poison-reagents": [(43139,3),(43140,3)],
    "trismerks-five-katanas": [(43141,5)], "austugus-hydralisk-research": [(43142,5)],
    "aydens-horde-reagents": [(43138,3),(43139,3)],
    "vrashs-mace-materials": [(93501,3),(43140,3)],
    "cairmes-shivs-and-rations": [(43143,4),(13723,1)],
    "bestiles-six-potions": [(v,1) for v in (2800,9428,11562,40469,66723,93914)],
}.items():
    assert [(t["item_vnums"][0],t["count"]) for t in shipy_stories[id]["steps"] if t["kind"] == "carried_item"] == expected
crates = shipy_stories["grimashks-crate-recovery"]
assert crates["contracts"] == [
    {"giver_vnum":43177,"completion_key":"give=I:43101;receive=C:250;disappear=0"},
    {"giver_vnum":43177,"completion_key":"give=I:43125;receive=C:845;disappear=0"}]
assert crates["steps"][0]["item_vnums"] == [43101,43125] and crates["steps"][0]["count"] == 1
assert shipy_stories["chundels-port-crates"]["contracts"] == [{"giver_vnum":43139,"completion_key":"give=I:43101;receive=C:200;disappear=0"}]
assert shipy_stories["gringashs-city-map"]["contracts"] != shipy_stories["kruthurgurs-siege-map"]["contracts"]
assert shipy_stories["geldens-elemental-study"]["steps"][0]["item_vnums"] == [9440]
assert all(s["category"] == "service" for id,s in shipy_stories.items() if id.startswith("bronaks-"))
shipy_units = [u for u in catalog_module.story_units(catalog) if u["zone_number"] == 431]
assert len(shipy_units) == 25 and sum(u["achievement"] for u in shipy_units) == 19
assert sum(u["daily_candidate"] for u in shipy_units) == 19
assert sum(r["definition"]["daily_eligible"] for r in shipy["requests"]) == 20
assert len(shipy["requests"]) == 26 and len(shipy["dialogue"]) == 20
parent = room = None
supplies = collections.defaultdict(list)
for reset in shipy["reset_commands"]:
    c,v = reset["command"],reset["arguments"]
    if c in ("M","F"): parent,room = v[1],v[3]
    if c in ("G","E") and v[1] in (43111,43137,43138,43139,43140,43141,43142,43143,93914):
        supplies[v[1]].append((c,parent,room,v[2],v[3] if c == "E" else None,v[4]))
assert supplies[43111] == [("E",43120,43249,1,16,100)]
assert supplies[43137] == [("G",43192,43322,15,None,100)] * 15
assert collections.Counter(supplies[43138]) == {("G",43193,43323,19,None,100):16,("E",43193,43323,19,18,100):3}
assert supplies[43139] == [("G",43194,43324,20,None,100)] * 20
assert supplies[43140] == [("E",43195,43325,22,16,100)] * 22
assert supplies[43141] == [("E",43196,43326,8,16,100)] * 8
assert supplies[43142] == [("G",43197,43327,17,None,100)] * 17
assert supplies[43143] == [("G",43198,43328,11,None,100)] * 11
assert supplies[93914] == [("G",43179,43310,999,None,100)]

# Ultarium: competing exact souls, duplicate-name proof/output and rare sources.
cosmic = inventory_module.area_evidence(ROOT, "cosmic")
cosmic_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "cosmic")
cosmic_stories = {s["id"]: s for s in cosmic_mapping["stories"]}
assert cosmic_mapping["coverage"] == "complete" and cosmic_mapping["revision"] == 1
assert len(cosmic_stories) == 15 and len(cosmic_mapping["contacts"]) == 23
assert sum(s["category"] == "service" for s in cosmic_stories.values()) == 8
assert sum(t.get("optional", False) for s in cosmic_stories.values() for t in s["steps"]) == 22
assert {tuple(sorted(b.items())) for s in cosmic_stories.values() for b in s["contracts"]} == {
    tuple(sorted(r["block"]["binding"].items())) for r in cosmic["requests"]}
seal = cosmic_stories["four-soul-seal"]
assert seal["contracts"] == [{"giver_vnum":76047,"completion_key":"give=I:76038,I:76039,I:76040,I:76041;receive=I:55174,I:76032,I:76050,I:76051;disappear=1"}]
assert [t["item_vnums"] for t in seal["steps"][:-1]] == [[76011],[76049],[76045],[76038],[76039],[76040],[76041]]
assert cosmic_stories["zeeniums-soul-offering"]["contracts"] == [{"giver_vnum":76027,"completion_key":"give=I:76038;receive=I:76066;disappear=0"}]
assert cosmic_stories["other-soul-offerings"]["contracts"] == [
    {"giver_vnum":76027,"completion_key":f"give=I:{v};receive=;disappear=0"} for v in (76039,76040,76041)]
assert cosmic_stories["recovered-planetary-study"]["contracts"] == [{"giver_vnum":76029,"completion_key":"give=I:76068;receive=I:76069;disappear=0"}]
assert cosmic_stories["windwalkers-companion"]["contracts"] == [{"giver_vnum":76054,"completion_key":"give=I:76064;receive=I:76065;disappear=1"}]
assert cosmic_stories["smugglers-security-key"]["contracts"] == [{"giver_vnum":76048,"completion_key":"give=I:76028;receive=C:100000;disappear=1"}]
for id, expected in {"second-draft-blueprints":76027,"recovered-planetary-study":76068,"siege-golem-plans":31115,"windwalkers-companion":76064,"hydra-scale-craft":76631}.items():
    assert cosmic_stories[id]["steps"][0]["item_vnums"] == [expected]
cosmic_units = [u for u in catalog_module.story_units(catalog) if u["zone_number"] == 760]
assert len(cosmic_units) == 15 and sum(u["achievement"] for u in cosmic_units) == 7
assert sum(u["daily_candidate"] for u in cosmic_units) == 7
assert len(cosmic["requests"]) == 17 and len(cosmic["dialogue"]) == 5
assert all(r["definition"]["daily_eligible"] for r in cosmic["requests"])
contacts = {c["mob_vnum"]: c for c in cosmic_mapping["contacts"]}
for d in cosmic["dialogue"]:
    assert set(d["body"][0].rstrip("~").split()) <= set(contacts[d["giver_vnum"]]["topics"])
parent = room = None
sources = collections.defaultdict(list)
for reset in cosmic["reset_commands"]:
    c,v = reset["command"],reset["arguments"]
    if c in ("M","F"): parent,room = v[1],v[3]
    if c == "G" and v[1] in (76038,76039,76040,76041,76064,76068,76045,76049):
        sources[v[1]].append((parent,room,v[2],v[4]))
    if c == "P" and v[1] == 76027: assert v == [1,76027,1,76026,100,0,0,0]
assert sources == {76038:[(76038,76191,1,100)],76039:[(76058,76191,1,100)],
    76040:[(76050,76200,1,100)],76041:[(76052,76202,1,100)],
    76064:[(76075,76236,1,100)],76068:[(76072,76227,1,100)],
    76045:[(76065,76198,1,100)],76049:[(76071,76176,1,100)]}
assert [(r["arguments"][1],r["arguments"][3],r["arguments"][4]) for r in cosmic["reset_commands"] if r["command"] == "M" and r["arguments"][1] in (76047,76075)] == [(76047,76228,50),(76075,76236,33)]
mira = inventory_module.area_evidence(ROOT, "mira")
assert any(r["block"]["binding"] == {"giver_vnum":82507,"completion_key":"give=I:76069;receive=I:82522;disappear=0"} and r["definition"]["zone_number"] == 825 for r in mira["requests"])
for a,b in ((76068,76069),(76064,76065)):
    assert cosmic["items"][a]["name"] == cosmic["items"][b]["name"]
    assert cosmic["items"][a]["keywords"] == cosmic["items"][b]["keywords"]

# Surface Realm: five exact kinds, consumed quantities and distinct market alternatives.
surface = inventory_module.area_evidence(ROOT, "surface")
surface_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "surface")
surface_stories = {s["id"]: s for s in surface_mapping["stories"]}
assert surface_mapping["coverage"] == "complete" and surface_mapping["revision"] == 1
assert len(surface_stories) == 23 and len(surface_mapping["contacts"]) == 37
assert sum(s["category"] == "service" for s in surface_stories.values()) == 6
assert sum(t.get("optional", False) for s in surface_stories.values() for t in s["steps"]) == 31
assert {tuple(sorted(b.items())) for s in surface_stories.values() for b in s["contracts"]} == {
    tuple(sorted(r["block"]["binding"].items())) for r in surface["requests"]}
mystic = surface_stories["mystics-five-offerings"]
assert mystic["contracts"] == [{"giver_vnum":500023,"completion_key":"give=I:500028,I:500029,I:500030,I:500031,I:500032;receive=I:500033,I:500034;disappear=1"}]
assert [t["item_vnums"] for t in mystic["steps"][:-1]] == [[v] for v in range(500028,500033)]
assert [t["item_vnums"] for t in surface_stories["labyrinthmaster-trophies"]["steps"][:-1]] == [[v] for v in range(500016,500020)]
for id, giver, output in [("stargazers-air-locket",500075,500031),("philosophers-earth-locket",500076,500030),("travelers-water-locket",500078,500029),("enchanters-fire-locket",500079,500028)]:
    assert surface_stories[id]["contracts"] == [{"giver_vnum":giver,"completion_key":f"give=I:500015;receive=C:100000,E:100000,I:{output};disappear=1"}]
    assert surface_stories[id]["steps"][0]["item_vnums"] == [500015]
for id, expected in {
    "mountaineers-five-claws": [(500020,5)], "hunters-five-fangs": [(43140,5)],
    "wizards-three-crystals": [(500025,3)], "flegnus-crystal-wand": [(500025,4)],
    "agnuts-gland-bracelet": [(43142,3),(729,1)],
}.items():
    assert [(t["item_vnums"][0],t["count"]) for t in surface_stories[id]["steps"] if t["kind"] == "carried_item"] == expected
for id, items in [("jagunks-fish-market",[355,356]),("kres-bait-market",[2676,319,330,333,334,335]),("moldugs-food-market",[293,294,295])]:
    assert surface_stories[id]["category"] == "service"
    assert surface_stories[id]["steps"][0]["item_vnums"] == items
    assert surface_stories[id]["steps"][0]["count"] == 1
    assert len(surface_stories[id]["contracts"]) == len(items)
assert surface_stories["nomads-white-potion"]["contracts"] == [{"giver_vnum":500096,"completion_key":"give=I:500023;receive=C:300000,E:50000;disappear=1"}]
assert all(t["kind"] == "carried_item" for t in surface_stories["nomads-white-potion"]["steps"][:-1])
assert surface_stories["seers-wyvern-egg"]["contracts"] != surface_stories["demilichs-wyvern-egg"]["contracts"]
surface_units = [u for u in catalog_module.story_units(catalog) if u["zone_number"] == 5000]
assert len(surface_units) == 23 and sum(u["achievement"] for u in surface_units) == 17
assert sum(u["daily_candidate"] for u in surface_units) == 17
assert len(surface["requests"]) == 31 and len(surface["dialogue"]) == 36
assert sum(r["definition"]["daily_eligible"] for r in surface["requests"]) == 29
contacts = {c["mob_vnum"]: c for c in surface_mapping["contacts"]}
for d in surface["dialogue"]:
    assert set(d["body"][0].rstrip("~").split()) <= set(contacts[d["giver_vnum"]]["topics"])
parent = room = None
sources = collections.defaultdict(list)
for reset in surface["reset_commands"]:
    c,v = reset["command"],reset["arguments"]
    if c in ("M","F"): parent,room = v[1],v[3]
    if c in ("G","E") and v[1] in range(500015,500025):
        sources[v[1]].append((c,parent,room,v[2],v[3] if c == "E" else None,v[4]))
assert sources[500020] == [("E",500087,586602,9,16,100)] * 9
assert sources[500024] == [("E",500014,629058,5,18,100)] * 5
assert [(r["arguments"][1],r["arguments"][3],r["arguments"][4]) for r in surface["reset_commands"] if r["command"] == "M" and r["arguments"][1] in (500000,500075,500076,500078,500079)] == [(v,619004,99) for v in (500000,500075,500076,500078,500079)]
assert [(r["arguments"][2],r["arguments"][3],r["arguments"][4]) for r in surface["reset_commands"] if r["command"] == "O" and r["arguments"][1] == 500026] == [(2,637471,100),(2,637874,100)]
assert len(surface["special_assignments"]) == 8

# Tharnadia: preparation is optional, exact toy kinds differ from borrowed stock,
# and administrative paper prototypes are valid without a discoverable owner.
tharnadia = inventory_module.area_evidence(ROOT, "tharnadia")
tharnadia_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "tharnadia")
tharnadia_stories = {s["id"]: s for s in tharnadia_mapping["stories"]}
assert tharnadia_mapping["schema_version"] == 3 and tharnadia_mapping["revision"] == 2
assert len(tharnadia_stories) == 16 and len(tharnadia_mapping["contacts"]) == 26
assert sum(s["category"] == "service" for s in tharnadia_stories.values()) == 8
assert sum(t.get("optional", False) for s in tharnadia_stories.values() for t in s["steps"]) == 23
assert len(tharnadia["requests"]) == 19 and len(tharnadia["dialogue"]) == 24
assert len(tharnadia["reset_commands"]) == 1480 and len(tharnadia["special_assignments"]) == 31
assert len(tharnadia["mobs"]) == 176 and len(tharnadia["items"]) == 206
assert {tuple(sorted(b.items())) for s in tharnadia_stories.values() for b in s["contracts"]} | {
    tuple(sorted(b.items())) for exclusion in tharnadia_mapping["exclusions"] for b in exclusion["contracts"]} == {
    tuple(sorted(r["block"]["binding"].items())) for r in tharnadia["requests"]}
assert tharnadia_mapping["exclusions"][0]["contracts"] == [{"giver_vnum":132581,"completion_key":"give=T:19;receive=E:33;disappear=1"}]
assert tharnadia_stories["ithilins-paper-map-service"]["contracts"] == [{"giver_vnum":132659,"completion_key":"give=I:5;receive=I:132705;disappear=0"}]
assert tharnadia_stories["menaes-wand-service"]["contracts"] == [{"giver_vnum":132672,"completion_key":"give=C:1000;receive=I:132703;disappear=0"}]
assert len(tharnadia_stories["menaes-wand-service"]["steps"]) == 1
assert [t["item_vnums"] for t in tharnadia_stories["arkelyns-three-toys"]["steps"][:-1]] == [[132682],[132683],[132684]]
assert tharnadia_stories["zechs-two-handed-sword-service"]["steps"][0]["item_vnums"] == [1110,1111]
assert len(tharnadia_stories["zechs-two-handed-sword-service"]["contracts"]) == 2
assert len(tharnadia_stories["brothel-ticket-service"]["contracts"]) == 2
chiln = tharnadia_stories["chilns-medicine-and-pendant"]
assert chiln["steps"][0]["contracts"] == tharnadia_stories["nebbles-medicine-service"]["contracts"]
assert all(t.get("optional") for t in chiln["steps"][:-1])
assert [t["item_vnums"] for t in chiln["steps"] if t["kind"] == "carried_item"] == [[132694],[132695],[132697],[132699]]
assert chiln["contracts"] == [{"giver_vnum":132667,"completion_key":"give=I:132697,I:132699;receive=E:3000,I:132698;disappear=1"}]
assert [t["item_vnums"] for t in tharnadia_stories["leodras-blank-book"]["steps"][:-1]] == [[132685]]
assert tharnadia_stories["rheds-lost-pup"]["steps"][0]["item_vnums"] == [132700]
tharnadia_units = [u for u in catalog_module.story_units(catalog) if u["zone_number"] == 1325]
assert len(tharnadia_units) == 16 and sum(u["achievement"] for u in tharnadia_units) == 8
assert sum(u["daily_candidate"] for u in tharnadia_units) == 8
contacts = {c["mob_vnum"]: c for c in tharnadia_mapping["contacts"]}
assert contacts[132661]["keyword"] == "arkeln"
for vnum, contact in contacts.items():
    assert contact["keyword"] in tharnadia["mobs"][vnum]["keywords"]
for d in tharnadia["dialogue"]:
    assert set(d["body"][0].rstrip("~").split()) <= set(contacts[d["giver_vnum"]]["topics"])
for name in ("Arkelyn", "Ithilin", "Thera", "Nebble", "Chiln", "Zaberetornaz", "Leodra", "Rhed", "Xero", "Menae", "Zech"):
    assert all(name not in line for line in tharnadia_mapping["orientation"])
parent = room = None
sources = collections.defaultdict(list)
for reset in tharnadia["reset_commands"]:
    c,v = reset["command"],reset["arguments"]
    assert len(v) == 8 and v[4:] == [100,0,0,0]
    if c in ("M","F"): parent,room = v[1],v[3]
    if c in ("G","E") and v[1] in (132682,132683,132684,132699):
        sources[v[1]].append((c,parent,room,v[2],v[3] if c == "E" else None))
assert sources[132682] == [("G",132664,132517,1,None)]
assert sources[132683] == [("G",132663,132517,1,None)]
assert sources[132684] == [("G",132662,132517,1,None)]
assert sources[132699] == [("E",132670,132836,1,3)]
assert [(r["arguments"][2],r["arguments"][3]) for r in tharnadia["reset_commands"] if r["command"] == "P" and r["arguments"][1] in (132694,132695,132700)] == [(1,132688)] * 3
assert any(r["command"] == "M" and r["arguments"][1] == 132677 for r in tharnadia["reset_commands"])
assert 132677 not in tharnadia["mobs"] and 132677 in tharnadia["items"]

# Mini Zones: a supplied sword set can skip the knight's independent release;
# four mixed-fee armor recipes are services rather than daily achievements.
mini = inventory_module.area_evidence(ROOT, "minizones")
mini_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "minizones")
mini_stories = {s["id"]: s for s in mini_mapping["stories"]}
assert mini_mapping["schema_version"] == 3 and mini_mapping["revision"] == 1
assert mini_mapping["coverage"] == "complete" and not mini_mapping["exclusions"]
assert len(mini_stories) == 7 and len(mini_mapping["contacts"]) == 18
assert collections.Counter(s["category"] for s in mini_stories.values()) == {"story":2,"request":1,"service":4}
assert sum(t.get("optional", False) for s in mini_stories.values() for t in s["steps"]) == 16
assert len(mini["requests"]) == 7 and len(mini["dialogue"]) == 11
assert len(mini["mobs"]) == 125 and len(mini["items"]) == 124
assert len(mini["reset_commands"]) == 548 and len(mini["special_assignments"]) == 6
assert {tuple(sorted(b.items())) for s in mini_stories.values() for b in s["contracts"]} == {
    tuple(sorted(r["block"]["binding"].items())) for r in mini["requests"]}
knight = mini_stories["release-worach"]
sword = mini_stories["restore-magik"]
assert knight["contracts"] == [{"giver_vnum":5800,"completion_key":"give=I:5804;receive=I:5794;disappear=1"}]
assert sword["contracts"] == [{"giver_vnum":5801,"completion_key":"give=I:5793,I:5794,I:5795,I:5806;receive=I:5805;disappear=1"}]
assert sword["steps"][0]["contracts"] == knight["contracts"]
assert all(t.get("optional") for t in sword["steps"][:-1])
assert [t["item_vnums"] for t in sword["steps"] if t["kind"] == "carried_item"] == [[5804],[5793],[5794],[5795],[5806]]
assert mini_stories["tips-for-the-dishwasher"]["contracts"] == [{"giver_vnum":5750,"completion_key":"give=I:5750;receive=;disappear=0"}]
for id, base, output in (("armplates",5811,5815),("legplates",5812,5816),("gloves",5813,5817),("boots",5814,5818)):
    recipe = mini_stories[f"thrulmar-{id}"]
    assert recipe["category"] == "service"
    assert [(t["item_vnums"],t["count"]) for t in recipe["steps"][:-1]] == [([base],1),([500025],5)]
    assert recipe["contracts"] == [{"giver_vnum":5813,"completion_key":f"give=C:400000,I:{base},I:500025,I:500025,I:500025,I:500025,I:500025;receive=I:{output};disappear=0"}]
    definition = next(r["definition"] for r in mini["requests"] if r["block"]["binding"] == recipe["contracts"][0])
    assert not definition["daily_eligible"] and definition["daily_exclusion"] == "Unsupported durable offering"
mini_units = [u for u in catalog_module.story_units(catalog) if u["zone_number"] == 57]
assert len(mini_units) == 7 and sum(u["achievement"] for u in mini_units) == 3
assert sum(u["daily_candidate"] for u in mini_units) == 3
contacts = {c["mob_vnum"]: c for c in mini_mapping["contacts"]}
for vnum, contact in contacts.items():
    assert contact["keyword"] in mini["mobs"][vnum]["keywords"]
assert contacts[5803]["keyword"] == "revanant" and contacts[5759]["keyword"] == "waittress"
assert not contacts[5755]["topics"]
for d in mini["dialogue"]:
    assert set(d["body"][0].rstrip("~").split()) <= set(contacts[d["giver_vnum"]]["topics"])
parent = room = None
sources = collections.defaultdict(list)
for reset in mini["reset_commands"]:
    c,v = reset["command"],reset["arguments"]
    assert len(v) == 8 and v[5:] == [0,0,0]
    if c in ("M","F"): parent,room = v[1],v[3]
    if c == "G" and v[1] in (5750,5793,5795,5796,5803,5807):
        sources[v[1]].append((parent,room,v[2],v[4]))
    if c == "P" and v[1] in (5804,5806):
        assert (v[1],v[2],v[3],v[4]) in ((5804,1,5803,100),(5806,1,5807,100))
assert sources == {5750:[(5759,5831,1,100)],5793:[(5798,5879,1,100)],
                   5795:[(5802,5920,1,100)],5796:[(5799,5882,1,100)],
                   5803:[(5803,5963,1,100)],5807:[(5804,5928,1,100)]}
assert not any(r["command"] in ("G","E","O","P") and r["arguments"][1] == 5794 for r in mini["reset_commands"])
assert {(a["kind"],a["vnum"],a["function"]) for a in mini["special_assignments"]} == {
    ("mob",5701,"dryad"),("mob",5702,"dryad"),("mob",5739,"navagator"),
    ("mob",5755,"world_quest"),("room",5783,"pet_shops"),("obj",5805,"sword_named_magik")}
mini_world = (ROOT / "areas/wld/minizones.wld").read_text()
rooms = {int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",mini_world,re.M|re.S)}
assert set(rooms) == set(range(5700,6000))
edges = {v:{int(d):(int(f),int(k),int(t)) for d,f,k,t in re.findall(r"\bD(\d+)\s+[^~]*~[^~]*~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)",b,re.S)} for v,b in rooms.items()}
position = 5940
for direction in (1,2,1,1,3,2,1,1):
    flags,key,position = edges[position][direction]
    assert flags == 0 and key == 0
assert position == 5960
for direction in (1,2,2): position = edges[position][direction][2]
assert position == 5963
assert edges[5833][1] == (3,-2,5834) and edges[5834][3] == (3,-2,5833)

# Torrhan: two different potion terminals, contested source materials, same-named
# cloak forms and exact sword/crown identities must not become invented campaigns.
torrhan = inventory_module.area_evidence(ROOT, "torrhan")
torrhan_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "torrhan")
torrhan_stories = {s["id"]: s for s in torrhan_mapping["stories"]}
assert torrhan_mapping["schema_version"] == 3 and torrhan_mapping["revision"] == 1
assert torrhan_mapping["coverage"] == "complete"
assert len(torrhan_stories) == 22 and len(torrhan_mapping["contacts"]) == 23
assert collections.Counter(s["category"] for s in torrhan_stories.values()) == {"story":6,"request":2,"service":14}
assert len(torrhan_mapping["exclusions"]) == 4
assert sum(t.get("optional",False) for s in torrhan_stories.values() for t in s["steps"]) == 28
assert len(torrhan["requests"]) == 26 and len(torrhan["dialogue"]) == 18
assert len(torrhan["mobs"]) == 152 and len(torrhan["items"]) == 125
assert len(torrhan["reset_commands"]) == 447
bindings = [b for s in torrhan_stories.values() for b in s["contracts"]]
bindings += [b for e in torrhan_mapping["exclusions"] for b in e["contracts"]]
assert len(bindings) == 26 and {tuple(sorted(b.items())) for b in bindings} == {
    tuple(sorted(r["block"]["binding"].items())) for r in torrhan["requests"]}
assert all(t.get("optional") for s in torrhan_stories.values() for t in s["steps"][:-1])
owl = torrhan_stories["owl-ladys-yellow-potion"]
king = torrhan_stories["king-torrhans-potion"]
torrok = torrhan_stories["torroks-restored-oblivion"]
assert owl["contracts"] == [{"giver_vnum":66701,"completion_key":"give=I:66666;receive=I:66667,I:66673;disappear=1"}]
assert king["contracts"] == [{"giver_vnum":66692,"completion_key":"give=I:66673;receive=I:66705;disappear=0"}]
assert king["steps"][0]["contracts"] == owl["contracts"]
assert [t["item_vnums"] for t in king["steps"] if t["kind"] == "carried_item"] == [[66666],[66673]]
assert torrok["steps"][0]["contracts"] == torrhan_stories["restore-oblivion"]["contracts"]
assert torrok["contracts"] == [{"giver_vnum":66721,"completion_key":"give=I:66718;receive=I:66717;disappear=0"}]
assert torrhan["items"][66652]["name"] == torrhan["items"][66705]["name"]
assert set(torrhan["items"][66606]["keywords"]) == set(torrhan["items"][66715]["keywords"])
for i,item in enumerate(range(66638,66646),1):
    recipe=torrhan_stories[f"cloak-form-{i}"]
    output=item+1 if i<8 else 66638
    assert recipe["category"] == "service" and recipe["steps"][0]["item_vnums"] == [item]
    assert recipe["contracts"] == [{"giver_vnum":66698,"completion_key":f"give=I:{item};receive=I:{output};disappear=0"}]
assert len({torrhan["items"][item]["name"] for item in range(66638,66646)}) == 1
units = [u for u in catalog_module.story_units(catalog) if u["zone_number"] == 666]
assert len(units) == 22 and sum(u["achievement"] for u in units) == 8
assert sum(u["daily_candidate"] for u in units) == 8
contacts = {c["mob_vnum"]:c for c in torrhan_mapping["contacts"]}
for v,c in contacts.items(): assert c["keyword"] in torrhan["mobs"][v]["keywords"]
for d in torrhan["dialogue"]:
    assert set(d["body"][0].rstrip("~").split()) <= set(contacts[d["giver_vnum"]]["topics"])
assert not contacts[66740]["topics"] and not contacts[66671]["topics"]
parent=room=None
sources=collections.defaultdict(list)
for reset in torrhan["reset_commands"]:
    c,v=reset["command"],reset["arguments"]
    assert len(v) == 8 and v[5:] == [0,0,0]
    if c in ("M","F"): parent,room=v[1],v[3]
    if c in ("G","E") and v[1] in (66611,66614,66619,66637,66668,66707,66715,66653):
        sources[v[1]].append((parent,room,v[2],v[4]))
    if c == "P": assert (v[1],v[2],v[3],v[4]) in ((66721,1,66719,100),(66666,1,66657,100))
assert sources == {66611:[(66648,66724,1,100)],66614:[(66658,66690,1,100)],
                   66619:[(66685,66782,1,100)],66637:[(66699,66900,1,100)],
                   66668:[(66701,66721,1,100)],66707:[(66754,66765,1,100)],
                   66715:[(66758,66901,1,100)],66653:[(66700,66807,1,100)]}
assert {(a["kind"],a["vnum"],a["function"]) for a in torrhan["special_assignments"]} == {
    ("room",66735,"crew_shop_proc"),("room",66689,"ship_shop_proc")}
torrhan_world=(ROOT / "areas/wld/torrhan.wld").read_text()
rooms={int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",torrhan_world,re.M|re.S)}
assert len(rooms) == 289 and min(rooms) == 66600 and max(rooms) == 66903
edges={v:{int(d):(int(f),int(k),int(t)) for d,f,k,t in re.findall(r"\bD(\d+)\s+[^~]*~[^~]*~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)",b,re.S)} for v,b in rooms.items()}
boundary={(v,d,t) for v,dirs in edges.items() for d,(f,k,t) in dirs.items() if t>0 and t not in rooms}
assert boundary == {(66689,0,580480),(66689,3,580879),(66692,2,581280),(66712,1,580881)}
assert edges[66801][0] == (4,0,66804) and (edges[66801][0][0] & 3) == 0
assert edges[66804] == {0:(0,0,66805)}
assert not any(r["command"] == "D" and r["arguments"][1] in (66801,66804) for r in torrhan["reset_commands"])
objects={int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",(ROOT / "areas/obj/torrhan.obj").read_text(),re.M|re.S)}
throne_fields=objects[66633].split("~")[4].split()
assert throne_fields[0] == "29" and throne_fields[11:15] == ["270","66804","0","0"]
assert "\n6087\n" in (ROOT / "areas/shp/torrhan.shp").read_text()
assert any(r["command"] == "G" and r["arguments"][1] == 6087 for r in torrhan["reset_commands"])
for line in (ROOT / "areas/AREA").read_text().splitlines():
    if not line.strip() or line.startswith("*"): continue
    source=ROOT / "areas/obj" / (line.split()[0]+".obj")
    if source.exists(): assert not re.search(r"^#6087\s*$",source.read_text(errors="replace"),re.M)

# Golden Hall: all preparation stays optional, exact proofs remain distinct,
# and experience/retirement key-return rescues keep achievements without dailies.
gold = inventory_module.area_evidence(ROOT, "gold_hal")
gold_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "gold_hal")
gold_stories = {s["id"]:s for s in gold_mapping["stories"]}
assert gold_mapping["coverage"] == "complete" and gold_mapping["revision"] == 1
assert gold_mapping["schema_version"] == 3 and len(gold_mapping["contacts"]) == 27
assert len(gold_stories) == 16 and not gold_mapping["exclusions"]
assert collections.Counter(s["category"] for s in gold_stories.values()) == {"story":7,"request":2,"service":7}
assert sum(t.get("optional",False) for s in gold_stories.values() for t in s["steps"]) == 34
assert len(gold["requests"]) == 16 and len(gold["dialogue"]) == 17
assert len(gold["mobs"]) == 91 and len(gold["items"]) == 106
assert len(gold["reset_commands"]) == 534
bindings = [b for s in gold_stories.values() for b in s["contracts"]]
assert len(bindings) == 16 and {tuple(sorted(b.items())) for b in bindings} == {
    tuple(sorted(r["block"]["binding"].items())) for r in gold["requests"]}
assert all(t.get("optional") for s in gold_stories.values() for t in s["steps"][:-1])
finale = gold_stories["three-proofs-for-wasephius"]
assert finale["contracts"] == [{"giver_vnum":40417,"completion_key":"give=I:40465,I:40483,I:40495;receive=I:40496;disappear=1"}]
assert [t["item_vnums"] for t in finale["steps"] if t["kind"] == "carried_item"] == [[40463],[40495],[40483],[40465]]
assert finale["steps"][0]["contracts"] == gold_stories["the-bloodstained-note"]["contracts"]
assert finale["steps"][1]["contracts"] == gold_stories["release-pearla"]["contracts"]
assert finale["steps"][2]["contracts"] == gold_stories["tields-stolen-amulet"]["contracts"]
assert gold_stories["release-pearla"]["steps"][0]["contracts"] == gold_stories["release-the-half-elf"]["contracts"]
units = [u for u in catalog_module.story_units(catalog) if u["zone_number"] == 404]
assert len(units) == 16 and sum(u["achievement"] for u in units) == 9
assert sum(u["daily_candidate"] for u in units) == 7
for id in ("release-the-half-elf","release-the-kenku"):
    unit = next(u for u in units if u["id"] == "zone-story:story:gold_hal:"+id)
    assert unit["achievement"] and not unit["daily_candidate"]
for giver in (40459,40477):
    definition = next(d for d in catalog["definitions"] if d["giver_vnum"] == giver)
    assert definition["eligible_for_zone_completion"] and not definition["daily_eligible"]
    assert definition["daily_exclusion"] == "Item exchange"
contacts = {c["mob_vnum"]:c for c in gold_mapping["contacts"]}
for v,c in contacts.items(): assert c["keyword"] in gold["mobs"][v]["keywords"]
for d in gold["dialogue"]:
    assert set(d["body"][0].rstrip("~").split()) <= set(contacts[d["giver_vnum"]]["topics"])
assert contacts[40482]["keyword"] == "tield" and contacts[40482]["topics"] == ["glakkabb"]
parent=room=None
sources=collections.defaultdict(list)
for reset in gold["reset_commands"]:
    c,v=reset["command"],reset["arguments"]
    assert len(v) == 8 and v[5:] == [0,0,0]
    if c in ("M","F"): parent,room=v[1],v[3]
    if c in ("G","E") and v[1] in (40403,40407,40427,40463,40470,40479,40490,40491,40493,40494,40495,40505):
        sources[v[1]].append((parent,room,v[2],v[4]))
assert sources == {40403:[(40406,40441,1,100)],40407:[(40409,40454,1,100)],
    40427:[(40423,40492,1,100)],40463:[(40458,40635,1,100)],
    40470:[(40463,40645,1,100)],40505:[(40463,40645,1,100)],
    40479:[(40473,40657,1,100)],40490:[(40489,40696,1,100)],
    40491:[(40490,40697,1,100)],40493:[(40488,40695,1,100)],
    40494:[(40487,40694,1,100)],40495:[(40458,40635,1,100)]}
assert any(r["command"] == "P" and r["arguments"][1:4] == [40456,1,40454] for r in gold["reset_commands"])
assert any(r["command"] == "O" and r["arguments"][1:4] == [40454,1,40533] for r in gold["reset_commands"])
assert any(r["command"] == "P" and r["arguments"][1:4] == [40489,1,40488] for r in gold["reset_commands"])
assert {(a["kind"],a["vnum"],a["function"]) for a in gold["special_assignments"]} == {
    ("mob",40466,"world_quest"),("mob",40410,"world_quest"),
    ("obj",40409,"reliance_pegasus"),("room",40454,"inn")}
rooms={int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",(ROOT/"areas/wld/gold_hal.wld").read_text(),re.M|re.S)}
assert set(rooms) == set(range(40400,40700))
edges={v:{int(d):(int(f),int(k),int(t)) for d,f,k,t in re.findall(r"\bD(\d+)\s+[^~]*~[^~]*~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)",b,re.S)} for v,b in rooms.items()}
assert {(v,d,t) for v,dirs in edges.items() for d,(f,k,t) in dirs.items() if t>0 and t not in rooms} == {(40400,2,559699),(40655,3,717300)}
assert edges[40655][1] == (0,0,40546)
assert not any(r["command"] == "D" and r["arguments"][1:3] == [40655,1] for r in gold["reset_commands"])
assert edges[40455][5] == (13,0,40519) and edges[40519][4] == (13,0,40455)
assert edges[40563][9] == (8,0,40565)
assert edges[40538][4] == (7,40505,40698)
assert edges[40698] == {1:(3,40505,40538),2:(3,40505,40538)}
objects={int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",(ROOT/"areas/obj/gold_hal.obj").read_text(),re.M|re.S)}
for item,values in {40420:["274","40655","1","0"],40431:["274","40455","5","0"],40432:["274","40519","4","1"],40436:["270","40563","9","1"]}.items():
    fields=objects[item].split("~")[4].split()
    assert fields[0] == "29" and fields[11:15] == values
assert gold["items"][40454]["name"] == "the corpse of an adventurer"
for item,target in ((40503,40699),(40504,40698)):
    fields=objects[item].split("~")[4].split()
    assert fields[0] == "25" and fields[11:14] == [str(target),"42","-1"]
mobs={int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",(ROOT/"areas/mob/gold_hal.mob").read_text(),re.M|re.S)}
assert {v for v,b in mobs.items() if int(b.split("~")[4].split()[0]) & 32768} == {40482}

# Ashrumite: paid crafting is support, and matching names cannot substitute
# for exact current contracts or make missing prototypes available.
ash = inventory_module.area_evidence(ROOT, "ashrumite")
ash_mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "ashrumite")
ash_stories = {s["id"]:s for s in ash_mapping["stories"]}
assert (ash_mapping["schema_version"],ash_mapping["revision"],ash_mapping["coverage"]) == (3,2,"complete")
assert len(ash_stories) == 12 and len(ash_mapping["contacts"]) == 16
assert all(s["category"] == "service" for s in ash_stories.values()) and not ash_mapping["exclusions"]
assert sum(t.get("optional",False) for s in ash_stories.values() for t in s["steps"]) == 21
assert len(ash["requests"]) == 12 and len(ash["dialogue"]) == 13
assert len(ash["mobs"]) == 53 and len(ash["items"]) == 65 and len(ash["reset_commands"]) == 275
assert {tuple(sorted(b.items())) for s in ash_stories.values() for b in s["contracts"]} == {
    tuple(sorted(r["block"]["binding"].items())) for r in ash["requests"]}
assert all(s["steps"][-1]["contracts"] == s["contracts"] for s in ash_stories.values())
assert all(t.get("optional") for s in ash_stories.values() for t in s["steps"][:-1])
ash_units = [u for u in catalog_module.story_units(catalog) if u["zone_number"] == 660]
assert len(ash_units) == 12 and not any(u["achievement"] or u["daily_candidate"] for u in ash_units)
five = ash_stories["silversmith-five-raw-gems"]
assert (five["steps"][1]["item_vnums"],five["steps"][1]["count"]) == ([66033],5)
assert five["steps"][0]["contracts"] == ash_stories["silversmith-current-refining"]["contracts"]
necklace = ash_stories["jeweler-current-necklace"]
assert [t["item_vnums"] for t in necklace["steps"] if t["kind"] == "carried_item"] == [[66049],[66044],[66045],[66046],[66047],[66048]]
mage = ash_stories["mage-current-necklace-enchantment"]
assert mage["contracts"] == [{"giver_vnum":66017,"completion_key":"give=C:25000,I:4372,I:66050;receive=I:66051;disappear=0"}]
assert [t["item_vnums"] for t in mage["steps"] if t["kind"] == "carried_item"] == [[66050]]
assert "unavailable disc" in mage["summary"] and "producing pyrite" in mage["summary"]
assert not ash_stories["bartenders-paid-disc-rumor"]["steps"][:-1]
assert {4372,6089,66066,66067}.isdisjoint(inventory_items) and 4022 in inventory_items
assert ash["items"][66032]["name"] == ash["items"][66033]["name"] == "a raw gem"
assert ash["items"][66048]["name"] == ash["items"][66049]["name"]
assert ash["items"][66050]["name"] == ash["items"][66051]["name"]
contacts = {c["mob_vnum"]:c for c in ash_mapping["contacts"]}
for v,c in contacts.items(): assert c["keyword"] in ash["mobs"][v]["keywords"]
for d in ash["dialogue"]:
    assert set(d["body"][0].rstrip("~").split()) <= set(contacts[d["giver_vnum"]]["topics"])
parent=room=None
sources=collections.defaultdict(list)
placed=set()
for reset in ash["reset_commands"]:
    c,v=reset["command"],reset["arguments"]
    assert v[4:] == [100,0,0,0]
    if c == "M": parent,room=v[1],v[3];placed.add(parent)
    if c == "G" and v[1] in (66001,66002):sources[v[1]].append((parent,room,v[2]))
    if c == "O" and v[1] in (66032,66033,66039,66040,66041):sources[v[1]].append((v[3],v[2]))
assert sources[66001] == [(66039,66069,1)] and sources[66002] == [(66002,66040,1)]
assert {v:sources[v] for v in (66032,66033,66039,66040,66041)} == {66032:[(66035,25)],66033:[(66034,25)],66039:[(66033,25)],66040:[(66032,25)],66041:[(66034,25)]}
assert {66022,66023,66024,66025,66026}.isdisjoint(placed) and 66031 in placed
assert any(r["command"] == "G" and r["arguments"][1] == 6089 for r in ash["reset_commands"])
assert len(ash["special_assignments"]) == 15
assert {(a["vnum"],a["function"]) for a in ash["special_assignments"] if a["function"] == "guild_guard"} == {
    (66031,"guild_guard"),(66024,"guild_guard"),(66023,"guild_guard"),(66025,"guild_guard"),(66022,"guild_guard")}
rooms={int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",(ROOT/"areas/wld/ashrumite.wld").read_text(),re.M|re.S)}
assert set(rooms) == set(range(66001,66154))
edges={v:{int(d):(int(f),int(k),int(t)) for d,f,k,t in re.findall(r"\bD(\d+)\s+[^~]*~[^~]*~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)",b,re.S)} for v,b in rooms.items()}
assert {(v,d,t) for v,ds in edges.items() for d,(f,k,t) in ds.items() if t>0 and t not in rooms} == {
    (66140,2,550481),(66140,3,550080),(66087,1,701670),(66087,2,702069),(66149,1,224166)}
assert not edges[66117] and edges[66069][1] == (3,66001,66074)
shop=(ROOT/"areas/shp/ashrumite.shp").read_text()
assert len(re.findall(r"^#\d+~",shop,re.M)) == 12 and "#66032~" not in shop and "#66019~" not in shop

# Hall: two identical elder offerings have different semantics, recipes keep
# exact quantities, and optional custody never qualifies a death source.
hall=inventory_module.area_evidence(ROOT,"hall")
hall_map=next(m for m in catalog["story_mappings"] if m["source_area"]=="hall")
hall_stories={s["id"]:s for s in hall_map["stories"]}
assert (hall_map["schema_version"],hall_map["revision"],hall_map["coverage"])==(3,1,"complete")
assert len(hall_stories)==9 and len(hall_map["contacts"])==27 and len(hall_map["exclusions"])==2
assert sum(s["category"]=="service" for s in hall_stories.values())==4
assert sum(t.get("optional",False) for s in hall_stories.values() for t in s["steps"])==23
assert len(hall["requests"])==11 and len(hall["dialogue"])==14
assert len(hall["mobs"])==55 and len(hall["items"])==53 and len(hall["reset_commands"])==395
assert all(s["steps"][-1]["contracts"]==s["contracts"] for s in hall_stories.values())
assert all(t.get("optional") for s in hall_stories.values() for t in s["steps"][:-1])
bindings=[b for s in hall_stories.values() for b in s["contracts"]]+[b for x in hall_map["exclusions"] for b in x["contracts"]]
assert len(bindings)==11 and {tuple(sorted(b.items())) for b in bindings}=={
    tuple(sorted(r["block"]["binding"].items())) for r in hall["requests"]}
assert {b["giver_vnum"] for x in hall_map["exclusions"] for b in x["contracts"]}=={77739}
assert "consumes" in hall_map["exclusions"][0]["reason"] and "shadowed" in hall_map["exclusions"][1]["reason"]
units=[u for u in catalog_module.story_units(catalog) if u["zone_number"]==777]
assert len(units)==9 and sum(u["achievement"] for u in units)==5 and sum(u["daily_candidate"] for u in units)==2
belt=hall_stories["jadem-sixteen-part-device"]
assert [(t["item_vnums"],t["count"]) for t in belt["steps"][:-1]]==[([77712],2),([77729],2),([77719],2),([77742],4),([77748],6)]
assert sum(t["count"] for t in belt["steps"][:-1])==16>native_limit
armor=hall_stories["xamael-platemail-of-awe"]
assert [(t["item_vnums"],t["count"]) for t in armor["steps"] if t["kind"]=="carried_item"]==[
    ([77745],2),([77713],2),([77719],2),([77720],1),([77724],2),([77750],1)]
assert sum(t["count"] for t in armor["steps"] if t["kind"]=="carried_item")==10
assert "actual recipe requires one" in armor["summary"]
assert hall_stories["child-dagger-for-letter"]["steps"][0]["item_vnums"]==[18309]
assert hall_stories["lost-aberrate-letter-for-hair"]["steps"][0]["contracts"]==hall_stories["child-dagger-for-letter"]["contracts"]
assert hall_stories["shopkeeper-first-tower-key"]["contracts"]==[{
    "giver_vnum":77724,"completion_key":"give=C:10000,I:77724;receive=I:77728;disappear=0"}]
assert hall_stories["xamael-repair-tower-key"]["contracts"]==[{
    "giver_vnum":77740,"completion_key":"give=C:10000,I:77733;receive=I:77739;disappear=0"}]
assert hall["items"][77728]["name"]==hall["items"][77739]["name"]=="a tiny key"
contacts={c["mob_vnum"]:c for c in hall_map["contacts"]}
for v,c in contacts.items(): assert c["keyword"] in hall["mobs"][v]["keywords"]
for d in hall["dialogue"]: assert set(d["body"][0].rstrip("~").split())<=set(contacts[d["giver_vnum"]]["topics"])
parent=room=None
sources=collections.defaultdict(list)
for reset in hall["reset_commands"]:
    c,v=reset["command"],reset["arguments"]
    assert v[5:]==[0,0,0]
    if c in ("M","F"): parent,room=v[1],v[3]
    if c in ("G","E") and v[1] in (77712,77713,77724,77729,77742,77751):
        sources[v[1]].append((parent,room,v[2],v[4]))
    assert not(c in ("O","G","E","P") and v[1]==77750)
assert sources=={77712:[(77708,77805,1,100)],77713:[(77709,77806,1,100)],
    77724:[(77721,77861,1,100)],77729:[(77727,77867,1,25)],
    77742:[(77717,77854,1,100)],77751:[(77716,77845,1,100)]}
assert any(r["command"]=="O" and r["arguments"][:5]==[0,77748,1,77928,10] for r in hall["reset_commands"])
assert len(hall["special_assignments"])==12
assert {(a["vnum"],a["function"]) for a in hall["special_assignments"] if a["kind"]=="mob"}=={
    (77714,"morkoth_mother"),(77747,"akckx"),(77750,"human_girl"),(77751,"hoa_death"),(77752,"hoa_sin")}
rooms={int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",(ROOT/"areas/wld/hall.wld").read_text(encoding="utf8"),re.M|re.S)}
assert set(rooms)==set(range(77700,77947))
edges={v:{int(d):(int(f),int(k),int(t)) for d,f,k,t in re.findall(r"\bD(\d+)\s+[^~]*~[^~]*~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)",b,re.S)} for v,b in rooms.items()}
assert {(v,d,t) for v,ds in edges.items() for d,(f,k,t) in ds.items() if t>0 and t not in rooms}=={(77700,0,813247)}
assert edges[77879][1]==(3,77728,77881) and edges[77885][1]==(7,77739,77886)
assert edges[77893][5]==(9,0,77944) and edges[77944][4]==(9,0,77893)
assert not edges[77855] and edges[77854][1]==(0,0,77855)
door_states={(r["arguments"][1],r["arguments"][2]):r["arguments"][3]
             for r in hall["reset_commands"] if r["command"]=="D"}
assert door_states[77885,1]==6 and door_states[77879,1]==2
assert door_states[77893,5]==door_states[77944,4]==9
assert "hidden, locked eastern tower entrance" in hall_stories["xamael-repair-tower-key"]["steps"][-1]["hint"]

# Sarmiz preserves independent source-owned deliveries, exact foreign recipe
# materials and the usable sibling of a punctuated topic. Custom moonstone
# guidance must not manufacture a completion or remove its guarded core source.
sarmiz=inventory_module.area_evidence(ROOT,"sarmiz")
sarmiz_map=next(m for m in catalog["story_mappings"] if m["source_area"]=="sarmiz")
sarmiz_stories={s["id"]:s for s in sarmiz_map["stories"]}
assert (sarmiz_map["schema_version"],sarmiz_map["revision"],sarmiz_map["coverage"])==(3,1,"complete")
assert len(sarmiz_stories)==8 and len(sarmiz_map["contacts"])==24 and not sarmiz_map["exclusions"]
assert sum(t.get("optional",False) for s in sarmiz_stories.values() for t in s["steps"])==19
assert len(sarmiz["requests"])==8 and len(sarmiz["dialogue"])==12
assert (len(sarmiz["mobs"]),len(sarmiz["items"]),len(sarmiz["reset_commands"]),len(sarmiz["special_assignments"]))==(57,61,535,5)
assert all(s["steps"][-1]["contracts"]==s["contracts"] for s in sarmiz_stories.values())
bindings=[b for s in sarmiz_stories.values() for b in s["contracts"]]
assert len(bindings)==8 and {tuple(sorted(b.items())) for b in bindings}=={
    tuple(sorted(r["block"]["binding"].items())) for r in sarmiz["requests"]}
units=[u for u in catalog_module.story_units(catalog) if u["zone_number"]==94]
assert len(units)==sum(u["achievement"] for u in units)==sum(u["daily_candidate"] for u in units)==8
royal=sarmiz_stories["rodev-three-ingredients"]
advisor=sarmiz_stories["aberla-four-materials"]
assert [(t["item_vnums"],t["count"]) for t in royal["steps"][:-1]]==[([9445],1),([97135],1),([97099],1)]
assert [(t["item_vnums"],t["count"]) for t in advisor["steps"] if t["kind"]=="carried_item"]==[([9450],1),([9442],1),([9453],1),([9451],1)]
assert [t["contracts"] for t in advisor["steps"] if t["kind"]=="completion" and t.get("optional")]==[
    sarmiz_stories["diplomat-badge-formula"]["contracts"],sarmiz_stories["captain-ancient-sword-dust"]["contracts"],sarmiz_stories["severne-letter-potions"]["contracts"]]
assert sarmiz_stories["derval-dagger-letter"]["steps"][0]["contracts"]==sarmiz_stories["lawter-obsidian-dagger"]["contracts"]
assert sarmiz_stories["severne-letter-potions"]["steps"][0]["contracts"]==sarmiz_stories["derval-dagger-letter"]["contracts"]
assert "item identity only" in sarmiz_stories["arian-lockpicks-relic"]["summary"]
assert "no native branch exclusivity" in advisor["steps"][-1]["hint"]
contacts={c["mob_vnum"]:c for c in sarmiz_map["contacts"]}
for v,c in contacts.items(): assert c["keyword"] in sarmiz["mobs"][v]["keywords"]
raw_m=[b for b in inventory_module.native_blocks(ROOT) if b["source"]=="areas/qst/sarmiz.qst" and b["kind"]=="M"]
assert len(raw_m)==13 and len({b["giver_vnum"] for b in raw_m})==9
for b in raw_m:
    assert {t for t in b["body"][0].rstrip("~").split() if "'" not in t}<=set(contacts[b["giver_vnum"]]["topics"])
assert contacts[9452]["topics"]==["sarmiz"]
assert set(contacts[9453]["topics"])=={"hello","hi","quest","help","xexos","moonstone","automaton","automatons","cost","money","reward"}
assert all(b["giver_vnum"]!=9453 for b in bindings)
assert "deliberately disabled with accounting active" in " ".join(sarmiz_map["orientation"])
assert "foreign three-item commission" in " ".join(sarmiz_map["orientation"])
parent=room=None;sources=collections.defaultdict(list)
for r in sarmiz["reset_commands"]:
    c,v=r["command"],r["arguments"]
    assert v[5:]==[0,0,0]
    if c in ("M","F"): parent,room=v[1],v[3]
    if c in ("G","E") and v[1] in (9448,9431,9442,9445): sources[v[1]].append((parent,room,v[2],v[4]))
assert sources=={9448:[(9407,9597,1,40)],9431:[(9420,9733,2,100)],9442:[(9404,9749,1,100)],9445:[(9419,9786,1,100)]}
for item in (3095,3096,6076,6018):
    assert item not in inventory_items and any(r["command"]=="G" and r["arguments"][1]==item for r in sarmiz["reset_commands"])
obj_bodies={int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",(ROOT/"areas/obj/sarmiz.obj").read_text(encoding="utf8"),re.M|re.S)}
assert int(obj_bodies[9451].split("~")[4].split()[0])==13
assert int(obj_bodies[9454].split("~")[4].split()[0])==10
ship_source=(ROOT/"src/ships/ship_npc.c").read_text(encoding="utf8")
for name in ("load_cyrics_revenge()","load_cyrics_revenge_crew(P_ship ship)"):
    start=ship_source.index("bool "+name)
    assert "if (economic_gameplay_authority::active())\n\t\treturn false;" in ship_source[start:start+220]

# Delwyn's paid producer route stays separate from supplied item-only
# outcomes. Source declarations, ambient chatter and dangerous access do not
# create personal-recovery, payment or campaign credit.
delwyn=inventory_module.area_evidence(ROOT,"delwyn")
delwyn_map=next(m for m in catalog["story_mappings"] if m["source_area"]=="delwyn")
delwyn_stories={s["id"]:s for s in delwyn_map["stories"]}
assert (delwyn_map["schema_version"],delwyn_map["revision"],delwyn_map["coverage"])==(3,1,"complete")
assert len(delwyn_stories)==11 and len(delwyn_map["contacts"])==19 and not delwyn_map["exclusions"]
assert sum(t.get("optional",False) for s in delwyn_stories.values() for t in s["steps"])==17
assert len(delwyn["requests"])==11 and len(delwyn["dialogue"])==9
assert (len(delwyn["mobs"]),len(delwyn["items"]),len(delwyn["reset_commands"]),len(delwyn["special_assignments"]))==(96,41,278,0)
assert not (ROOT/"areas/shp/delwyn.shp").exists()
assert all(s["steps"][-1]["contracts"]==s["contracts"] for s in delwyn_stories.values())
bindings=[b for s in delwyn_stories.values() for b in s["contracts"]]
assert len(bindings)==11 and {tuple(sorted(b.items())) for b in bindings}=={
    tuple(sorted(r["block"]["binding"].items())) for r in delwyn["requests"]}
units=[u for u in catalog_module.story_units(catalog) if u["zone_number"]==828]
assert (len(units),sum(u["achievement"] for u in units),sum(u["daily_candidate"] for u in units))==(11,6,6)
services={s["id"] for s in delwyn_stories.values() if s["category"]=="service"}
assert services=={"spinner-fleece-yarn","dyer-white-crimson-yarn","weaver-crimson-fabric","seamstress-crimson-banner","alley-paid-service"}
fees={r["block"]["giver_vnum"]:sum(n for kind,n in r["block"]["give"] if kind=="C") for r in delwyn["requests"]}
assert {v:fees[v] for v in (82825,82821,82824,82823,82807)}=={82825:5000,82821:10000,82824:10000,82823:35000,82807:1000}
banner=delwyn_stories["magician-crimson-banner"]
warning=delwyn_stories["duke-note-and-braid"]
assert banner["steps"][0]["contracts"]==delwyn_stories["seamstress-crimson-banner"]["contracts"]
assert [(t["item_vnums"],t["count"]) for t in warning["steps"] if t["kind"]=="carried_item"]==[([82824],1),([82823],1)]
assert warning["steps"][0]["contracts"]==delwyn_stories["halfling-military-assessment"]["contracts"]
for producer,consumer in (("spinner-fleece-yarn","dyer-white-crimson-yarn"),("dyer-white-crimson-yarn","weaver-crimson-fabric"),("weaver-crimson-fabric","seamstress-crimson-banner"),("clockmaker-iron-bell","miller-iron-cog")):
    assert delwyn_stories[consumer]["steps"][0]["contracts"]==delwyn_stories[producer]["contracts"]
contacts={c["mob_vnum"]:c for c in delwyn_map["contacts"]}
for v,c in contacts.items(): assert c["keyword"] in delwyn["mobs"][v]["keywords"]
raw_m=[b for b in inventory_module.native_blocks(ROOT) if b["source"]=="areas/qst/delwyn.qst" and b["kind"]=="M"]
assert len(raw_m)==42 and sum(b["body"][0].startswith("qc_action ") for b in raw_m)==33
for b in delwyn["dialogue"]:
    assert set(b["body"][0].rstrip("~").split())<=set(contacts[b["giver_vnum"]]["topics"])
assert all("qc_action" not in c["topics"] for c in contacts.values())
assert not contacts[82878]["topics"] and contacts[82895]["topics"]==["flagpole","frown","flag"]
assert "60000 copper" in " ".join(delwyn_map["orientation"])
assert "guarded with accounting active" in banner["summary"] and "birthday" in warning["summary"]
parent=room=None;sources=collections.defaultdict(list)
for r in delwyn["reset_commands"]:
    c,v=r["command"],r["arguments"]
    assert v[4]==100 and v[5:]==[0,0,0]
    if c in ("M","F"): parent,room=v[1],v[3]
    if c in ("G","E") and v[1] in (82816,82823,82826,82828): sources[v[1]].append((parent,room,v[2]))
assert sources=={82816:[(82853,82821,1)],82823:[(82890,82965,1)],82826:[(82872,82980,1)],82828:[(82833,82879,1)]}
assert any(r["command"]=="P" and r["arguments"][:5]==[1,82822,1,82814,100] for r in delwyn["reset_commands"])
assert all(r["arguments"][3]==1 for r in delwyn["reset_commands"] if r["command"]=="D")
room_bodies={int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",(ROOT/"areas/wld/delwyn.wld").read_text(encoding="utf8"),re.M|re.S)}
assert set(room_bodies)==set(range(82800,83007))
assert "0 0 549230" in room_bodies[82805]
surface=(ROOT/"areas/wld/surface.wld").read_text(encoding="utf8")
surface_gate=re.search(r"^#549230\s*\n(.*?)(?=^#|^\$)",surface,re.M|re.S)[1]
assert re.search(r"D0\s+[^~]*~[^~]*~\s*0 0 82805",surface_gate,re.S)
for v,chance in ((83003,5),(83005,80),(83006,90)):
    assert re.search(r"^F\s+"+str(chance)+r"\s*$",room_bodies[v],re.M)
for v in (83005,83006): assert int(room_bodies[v].split("~")[2].split()[2])==8
obj_bodies={int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",(ROOT/"areas/obj/delwyn.obj").read_text(encoding="utf8"),re.M|re.S)}
assert re.search(r"\bT\s+512\s+4\s+1\s+30\b", obj_bodies[82814])
assert "wedding" in obj_bodies[82824]
assert "birthday" in " ".join(next(r["block"] for r in delwyn["requests"] if r["block"]["giver_vnum"]==82878)["body"])

# Divine contracts share recipients and sometimes visible item names. Keep
# exact delivery identity, reward-cash versus fees and support history separate.
divine=inventory_module.area_evidence(ROOT,"divhome")
divine_map=next(m for m in catalog["story_mappings"] if m["source_area"]=="divhome")
divine_stories={s["id"]:s for s in divine_map["stories"]}
assert (divine_map["schema_version"],divine_map["revision"],divine_map["coverage"])==(3,1,"complete")
assert len(divine_stories)==32 and len(divine_map["contacts"])==27 and not divine_map["exclusions"]
assert sum(t.get("optional",False) for s in divine_stories.values() for t in s["steps"])==48
assert (len(divine["requests"]),len(divine["dialogue"]),len(divine["mobs"]),len(divine["items"]),len(divine["reset_commands"]),len(divine["special_assignments"]))==(32,20,62,83,240,0)
bindings=[b for s in divine_stories.values() for b in s["contracts"]]
assert len(bindings)==32 and {tuple(sorted(b.items())) for b in bindings}=={
    tuple(sorted(r["block"]["binding"].items())) for r in divine["requests"]}
assert all(s["steps"][-1]["contracts"]==s["contracts"] for s in divine_stories.values())
units=[u for u in catalog_module.story_units(catalog) if u["zone_number"]==407]
assert (len(units),sum(u["achievement"] for u in units),sum(u["daily_candidate"] for u in units))==(32,20,20)
assert sum(s["category"]=="service" for s in divine_stories.values())==12
assert sum(s["id"].startswith("bounty-") for s in divine_stories.values())==7
treasures={s["id"]:s for s in divine_stories.values() if s["id"].startswith("wicks-")}
assert len(treasures)==9 and all(s["category"]=="request" for s in treasures.values())
by_line={r["block"]["line"]:r["block"] for r in divine["requests"]}
assert [(n,by_line[n]["receive"]) for n in (239,244,251,257,263,268,273,277,282)]==[
    (239,[("C",50000)]),(244,[("C",10000)]),(251,[("C",25000)]),(257,[("C",25000)]),
    (263,[("C",5000)]),(268,[("C",10000)]),(273,[("C",25000)]),(277,[("C",55000)]),(282,[("C",40000)])]
assert sum(any(k=="C" for k,n in b["give"]) for b in by_line.values())==6
assert by_line[178]["give"]==[("I",40780),("I",40782),("I",392),("C",200000)]
assert by_line[178]["receive"]==[("I",40781),("E",150000)]
assert by_line[392]["give"]==[("I",76697),("C",10000000)]
assert by_line[67]["receive"]==[("I",22625)] and by_line[78]["receive"]==[("I",22020)]
assert by_line[82]["receive"]==[("I",31341)] and 31341 not in inventory_items
assert "absent" in divine_stories["bounty-relazier"]["summary"]
assert by_line[295]["give"]==by_line[329]["give"]==[("I",40718)]
assert by_line[312]["receive"]==[("I",40745)] and by_line[405]["receive"]==[("I",40740)]
def divine_materials(id):return [(t["item_vnums"],t["count"]) for t in divine_stories[id]["steps"] if t["kind"]=="carried_item"]
assert divine_materials("siren-four-elements")==[([40762],1),([40763],1),([40764],1),([40765],1)]
for id,item in (("wicks-harpy-wyvern-egg",31105),("wicks-exotic-wyvern-egg",500026),("wicks-single-horseshoe",40734),("wicks-moria-horseshoes",99061)):
    assert divine_materials(id)==[([item],1)]
for producer,consumer in (("phoenix-fire-mace","emition-sun-longsword"),("shiva-frost-mace","emition-frost-longsword")):
    assert divine_stories[consumer]["steps"][0]["contracts"]==divine_stories[producer]["contracts"]
merge=divine_stories["emition-dusk-and-dawn"]
assert [t["contracts"] for t in merge["steps"][:2]]==[divine_stories[id]["contracts"] for id in ("emition-sun-longsword","emition-frost-longsword")]
assert "300000 copper" in merge["summary"] and "disabled" in " ".join(divine_map["orientation"])
contacts={c["mob_vnum"]:c for c in divine_map["contacts"]}
for v,c in contacts.items():assert c["keyword"] in divine["mobs"][v]["keywords"]
useful=0
for b in divine["dialogue"]:
    if inventory_module.plain(" ".join(b["body"][1:]).replace("~","")):
        useful+=1
        assert set(b["body"][0].rstrip("~").split())<=set(contacts[b["giver_vnum"]]["topics"])
assert useful==19 and "default" not in contacts[40706]["topics"] and not contacts[40761]["topics"]
parent=room=None;sources=collections.defaultdict(list)
for r in divine["reset_commands"]:
    c,v=r["command"],r["arguments"]
    assert v[4]==100 and v[5:]==[0,0,0]
    if c=="M":parent,room=v[1],v[3]
    if c in ("G","E") and v[1] in (40718,40727,40744,40756,40757,40758,40760,40776):sources[v[1]].append((parent,room,v[2]))
assert sources[40718]==[(40729,40772,1)] and sources[40727]==[(40736,40797,1)]
assert sources[40756]==[(40747,40803,1)] and sources[40757]==[(40755,40803,1)]
assert sources[40758]==[(40756,40813,1)] and sources[40776]==[(40761,40803,1)]
assert sources[40760]==[(40757,40812,1)] and len(sources[40744])==3
assert {(r["arguments"][1],r["arguments"][3]) for r in divine["reset_commands"] if r["command"]=="P"}=={(40719,40703),(40746,40743),(40751,40714)}
room_bodies={int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",(ROOT/"areas/wld/divhome.wld").read_text(encoding="utf8"),re.M|re.S)}
assert set(room_bodies)==set(range(40700,40822)) and "0 0 615667" in room_bodies[40780]
for v,chance in ((40747,30),(40760,30),(40768,40),(40788,50),(40811,25)):
    assert re.search(r"^F\s+"+str(chance)+r"\s*$",room_bodies[v],re.M)
for v,key in ((40748,40740),(40751,40740),(40781,40745),(40782,40745),(40812,40745)):
    assert re.search(r"\b"+str(key)+r"\s+\d+",room_bodies[v])
obj_bodies={int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",(ROOT/"areas/obj/divhome.obj").read_text(encoding="utf8"),re.M|re.S)}
assert re.search(r"\bT\s+2\s+2\s+1\s+20\b",obj_bodies[40760])
for v,target,cmd in ((40720,40773,7),(40721,40758,7),(40723,40781,320),(40724,40756,320),(40753,22053,7),(40754,40780,7),(40755,40700,7)):
    values=list(map(int,obj_bodies[v].split("~")[4].split()[:15]))
    assert values[0]==25 and values[11:14]==[target,cmd,-1]
assert (ROOT/"areas/shp/divhome.shp").read_text(encoding="utf8").startswith("#40712~")

# Halfcut's longer rescue and raid narratives still accept independent,
# supplied exact proof. A journal cannot repair missing native reward identity.
halfcut=inventory_module.area_evidence(ROOT,"halfcut")
halfcut_map=next(m for m in catalog["story_mappings"] if m["source_area"]=="halfcut")
halfcut_stories={s["id"]:s for s in halfcut_map["stories"]}
assert (halfcut_map["schema_version"],halfcut_map["revision"],halfcut_map["coverage"])==(3,1,"complete")
assert len(halfcut_stories)==13 and len(halfcut_map["contacts"])==19 and not halfcut_map["exclusions"]
assert sum(t.get("optional",False) for s in halfcut_stories.values() for t in s["steps"])==24
assert (len(halfcut["requests"]),len(halfcut["dialogue"]),len(halfcut["mobs"]),len(halfcut["items"]),len(halfcut["reset_commands"]),len(halfcut["special_assignments"]))==(13,15,83,60,413,1)
assert [(a["kind"],a["vnum"],a["function"]) for a in halfcut["special_assignments"]]==[("mob",27009,"crossbow_ambusher")]
bindings=[b for s in halfcut_stories.values() for b in s["contracts"]]
assert len(bindings)==13 and {tuple(sorted(b.items())) for b in bindings}=={
    tuple(sorted(r["block"]["binding"].items())) for r in halfcut["requests"]}
assert all(s["steps"][-1]["contracts"]==s["contracts"] for s in halfcut_stories.values())
units=[u for u in catalog_module.story_units(catalog) if u["zone_number"]==270]
assert (len(units),sum(u["achievement"] for u in units),sum(u["daily_candidate"] for u in units))==(13,13,13)
by_line={r["block"]["line"]:r["block"] for r in halfcut["requests"]}
assert all(all(k=="I" for k,n in b["give"]) for b in by_line.values())
assert by_line[13]["give"]==[("I",27037)] and by_line[13]["receive"]==[("C",25000),("E",25000)]
assert by_line[61]["receive"]==[("E",2000),("I",27040)]
assert by_line[136]["receive"]==[("E",20000),("I",27041)]
assert by_line[75]["receive"]==[("I",27042),("E",2500)]
assert by_line[112]["give"]==[("I",27040),("I",27041),("I",27042)]
assert by_line[121]["receive"]==[("C",150000),("I",27044)]
assert by_line[231]["receive"]==[("I",27056),("I",25000)] and 25000 not in inventory_items
assert "absent" in halfcut_stories["drow-duergar-scalp"]["summary"]
assert by_line[177]["give"]==[("I",27052),("I",27053),("I",27054),("I",27055),("I",27057),("I",27058)]
for consumer,item in ((197,27054),(214,27052),(231,27055),(246,27053)):
    assert by_line[consumer]["give"]==[("I",item)] and ("I",item) in by_line[177]["give"]
assert [t["contracts"] for t in halfcut_stories["bartis-three-badges"]["steps"][:3]]==[
    halfcut_stories[id]["contracts"] for id in ("first-old-miner-jar","second-old-miner-jar","young-miner-jar")]
assert halfcut_stories["sentry-bartis-note"]["steps"][0]["contracts"]==halfcut_stories["bartis-final-jar"]["contracts"]
assert "before" in halfcut_stories["bartis-three-badges"]["summary"]
contacts={c["mob_vnum"]:c for c in halfcut_map["contacts"]}
for v,c in contacts.items():assert c["keyword"] in halfcut["mobs"][v]["keywords"]
assert contacts[27005]["keyword"]=="dwarf" and contacts[27035]["keyword"]=="aden"
for b in halfcut["dialogue"]:
    assert set(b["body"][0].rstrip("~").split())<=set(contacts[b["giver_vnum"]]["topics"])
parent=room=None;sources=collections.defaultdict(list);families=set()
assert collections.Counter(r["command"] for r in halfcut["reset_commands"])=={
    "M":309,"D":42,"E":23,"G":19,"O":13,"P":7}
for r in halfcut["reset_commands"]:
    c,v=r["command"],r["arguments"]
    assert v[4]==100 and v[5:]==[0,0,0]
    if c=="M":parent,room=v[1],v[3]
    families.add((c,tuple(v[:3]),parent if c in ("G","E") else None))
    if c in ("G","E") and v[1] in (27038,27051,27052,27053,27054,27055,27057,27058):sources[v[1]].append((parent,room,v[2]))
assert len(families)==177 and sources[27038]==[(27058,27396,1)]
for item,npc,room in ((27051,27078,27465),(27052,27080,27469),(27053,27082,27467),(27054,27081,27468),(27055,27083,27466),(27057,27065,27433),(27058,27063,27431)):
    assert sources[item]==[(npc,room,1)]
jars=[r["arguments"][1:4] for r in halfcut["reset_commands"] if r["command"]=="P" and r["arguments"][1]==27039]
assert jars==[[27039,4,27038]]*4
room_bodies={int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",(ROOT/"areas/wld/halfcut.wld").read_text(encoding="utf8"),re.M|re.S)}
assert set(room_bodies)==set(range(27001,27471))
assert len({(b.split("~")[0].strip(),b.split("~")[1].strip()) for b in room_bodies.values()})==208
assert "0 0 629274" in room_bodies[27001] and "0 0 27423" in room_bodies[27429]
assert "524288" in room_bodies[27121]
assert "9 0 27432" in room_bodies[27431]
assert "27033 27450" in room_bodies[27440] and "27033 27440" in room_bodies[27450]
obj_bodies={int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",(ROOT/"areas/obj/halfcut.obj").read_text(encoding="utf8"),re.M|re.S)}
for v,target,cmd in ((27024,27242,65),(27025,27380,65),(27029,27429,7),(27035,27453,17)):
    values=list(map(int,obj_bodies[v].split("~")[4].split()[:15]))
    assert values[0]==25 and values[11:14]==[target,cmd,-1]
for v,cmd,room in ((27020,65,27046),(27031,340,27431)):
    values=list(map(int,obj_bodies[v].split("~")[4].split()[:15]))
    assert values[0]==29 and values[11:14]==[cmd,room,3]
assert obj_bodies[27039].split("~")[4].split()[0]=="13"
assert (ROOT/"areas/shp/halfcut.shp").read_text(encoding="utf8").startswith("#27072~\nN\n27006\n27010\n27011\n27046\n27047\n27048\n0")
assert "flaming" not in inventory_items[27037]["name"] and "flaming" in inventory_items[27046]["name"]

# Scorched Valley's exact deliveries retain supplied proof and foreign ownership.
# Native lifecycle/dispatch facts are evidence for pending repairs, not repaired here.
scorch=inventory_module.area_evidence(ROOT,"scorchvalley")
scorch_map=next(m for m in catalog["story_mappings"] if m["source_area"]=="scorchvalley")
scorch_stories={s["id"]:s for s in scorch_map["stories"]}
assert (scorch_map["schema_version"],scorch_map["revision"],scorch_map["coverage"])==(3,1,"complete")
assert len(scorch_stories)==9 and len(scorch_map["contacts"])==22 and not scorch_map["exclusions"]
assert sum(t.get("optional",False) for s in scorch_stories.values() for t in s["steps"])==22
assert (len(scorch["requests"]),len(scorch["dialogue"]),len(scorch["mobs"]),len(scorch["items"]),len(scorch["reset_commands"]))==(9,11,60,55,211)
assert [(a["kind"],a["vnum"],a["function"]) for a in scorch["special_assignments"]]==[
    ("mob",71223,"block_up"),("mob",71259,"yeenoghu"),("obj",71231,"artifact_invisible")]
bindings=[b for s in scorch_stories.values() for b in s["contracts"]]
assert len(bindings)==9 and {tuple(sorted(b.items())) for b in bindings}=={
    tuple(sorted(r["block"]["binding"].items())) for r in scorch["requests"]}
assert all(s["steps"][-1]["contracts"]==s["contracts"] for s in scorch_stories.values())
units=[u for u in catalog_module.story_units(catalog) if u["zone_number"]==712]
assert (len(units),sum(u["achievement"] for u in units),sum(u["daily_candidate"] for u in units))==(9,9,9)
by_line={r["block"]["line"]:r["block"] for r in scorch["requests"]}
assert all(all(k=="I" for k,n in b["give"]) and b["binding"]["completion_key"].endswith("disappear=0") for b in by_line.values())
for line,required,reward in (
    (23,[71205],[71218]),(89,[71212],[71213]),(113,[71227,71009,28980],[71226]),
    (129,[71228],[71230]),(165,[71219,71203,71204],[71220]),(181,[71250],[71251]),
    (206,[71240],[71235]),(228,[71239],[71224,71244,71245,71246]),
    (256,[71224,71244,71245,71246],[71248])):
    assert by_line[line]["give"]==[("I",v) for v in required]
    assert by_line[line]["receive"]==[("I",v) for v in reward]
    assert all(v in inventory_items for v in required+reward)
assert scorch_stories["wildmage-four-rings"]["steps"][0]["contracts"]==scorch_stories["seeker-tallin-blood"]["contracts"]
assert [t["item_vnums"] for t in scorch_stories["seeker-tallin-blood"]["steps"][:-1]]==[
    [71201],[71206],[71208],[71236],[71237],[71239]]
contacts={c["mob_vnum"]:c for c in scorch_map["contacts"]}
for v,c in contacts.items():
    assert c["keyword"] in scorch["mobs"][v]["keywords"] and len(c["topics"])<=32
    native=set(t for b in scorch["dialogue"] if b["giver_vnum"]==v for t in b["body"][0].rstrip("~").split())
    assert set(c["topics"])<=native
for b in scorch["dialogue"]:
    assert set(b["body"][0].rstrip("~").split())&set(contacts[b["giver_vnum"]]["topics"])
assert len(contacts[71236]["topics"])==31
assert "stays" in scorch_stories["council-godly-magic"]["summary"]
parent=room=None;sources=collections.defaultdict(list);families=set()
assert collections.Counter(r["command"] for r in scorch["reset_commands"])=={
    "M":114,"F":31,"E":22,"G":15,"D":14,"O":9,"P":6}
for r in scorch["reset_commands"]:
    c,v=r["command"],r["arguments"]
    assert v[4]==100 and v[5:]==[0,0,0]
    if c in ("M","F"):parent,room=v[1],v[3]
    families.add((c,tuple(v[:3]),parent if c in ("G","E") else None))
    if c in ("G","E"):sources[v[1]].append((parent,room,v[2]))
assert len(families)==118
for item,npc,room in ((71201,71201,71203),(71206,71232,71277),(71208,71234,71279),
    (71236,71254,71281),(71237,71236,71283),(71203,71214,71244),(71204,71213,71243),
    (71227,71210,71242),(71240,71255,71223),(71212,71246,71323),(71228,71248,71326)):
    assert sources[item]==[(npc,room,1)]
assert sources[71214]==[(71244,71321,1)] # E after an F belongs to Miska, not the Queen.
assert not sources[71215]
for area,item,npc,room in (("torg",28980,28937,29077),("fields_between",71009,71038,71151)):
    parent=location=None;found=[]
    for r in inventory_module.area_evidence(ROOT,area)["reset_commands"]:
        c,v=r["command"],r["arguments"]
        if c in ("M","F"):parent,location=v[1],v[3]
        if c in ("G","E") and v[1]==item:found.append((parent,location,v[2]))
    assert found==[(npc,room,1)]
room_bodies={int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",(ROOT/"areas/wld/scorchvalley.wld").read_text(encoding="utf8"),re.M|re.S)}
assert set(room_bodies)==set(range(71201,71333))
assert len({(b.split("~")[0].strip(),b.split("~")[1].strip()) for b in room_bodies.values()})==78
for room,target in ((71201,576166),(71326,9122),(71330,29071),(71331,71140)):
    assert f"0 0 {target}" in room_bodies[room]
for room,key,target in ((71203,71201,71204),(71267,71206,71270),(71279,71208,71280),(71281,71236,71282)):
    assert f"3 {key} {target}" in room_bodies[room]
obj_bodies={int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",(ROOT/"areas/obj/scorchvalley.obj").read_text(encoding="utf8"),re.M|re.S)}
for v,target in ((71209,71310),(71210,71284)):
    values=list(map(int,obj_bodies[v].split("~")[4].split()[:15]))
    assert values[0]==25 and values[11:14]==[target,7,-1]
assert list(map(int,obj_bodies[71238].split("~")[4].split()[:15]))[11:14]==[500,29,71237]
assert obj_bodies[71212].split("~")[4].split()[0]=="13"
assert not (ROOT/"areas/shp/scorchvalley.shp").exists()
assert [r["arguments"][1:4] for r in scorch["reset_commands"] if r["command"]=="P" and r["arguments"][1] in (71239,71250)]==[[71250,1,71249],[71239,1,71238]]
# Preserve the identified native dispatch evidence; a future intentional repair
# should replace these assertions with its focused executable qualification.
scorch_special=(ROOT/"src/specs/specs.scorchvalley.c").read_text(encoding="utf8")
boss=scorch_special.split("int yeenoghu(",1)[1]
assert "if (cmd == CMD_SET_PERIODIC)\n\t\treturn FALSE;" in boss and "if (cmd)\n\t\treturn FALSE;" in boss
assert "#define CMD_MOB_COMBAT -102" in (ROOT/"src/cmd/interp.h").read_text(encoding="utf8")

# Court keeps exact distinct seasons, counted scales and independent native receipts.
# Access/trap declarations below qualify guidance, not an actual content repair.
court=inventory_module.area_evidence(ROOT,"court")
court_map=next(m for m in catalog["story_mappings"] if m["source_area"]=="court")
court_stories={s["id"]:s for s in court_map["stories"]}
assert (court_map["schema_version"],court_map["revision"],court_map["coverage"])==(3,1,"complete")
assert len(court_stories)==9 and len(court_map["contacts"])==25 and not court_map["exclusions"]
assert sum(t.get("optional",False) for s in court_stories.values() for t in s["steps"])==16
assert (len(court["requests"]),len(court["dialogue"]),len(court["mobs"]),len(court["items"]),len(court["reset_commands"]))==(9,6,36,43,209)
assert not court["special_assignments"]
assert collections.Counter(b["kind"] for b in inventory_module.native_blocks(ROOT) if b["source"]=="areas/qst/court.qst")=={"QA":7,"Q":2,"MA":5,"M":1}
bindings=[b for s in court_stories.values() for b in s["contracts"]]
assert len(bindings)==9 and {tuple(sorted(b.items())) for b in bindings}=={
    tuple(sorted(r["block"]["binding"].items())) for r in court["requests"]}
units=[u for u in catalog_module.story_units(catalog) if u["zone_number"]==67]
assert (len(units),sum(u["achievement"] for u in units),sum(u["daily_candidate"] for u in units))==(9,9,9)
by_line={r["block"]["line"]:r["block"] for r in court["requests"]}
for line,required,reward in ((10,[6722],[("I",6717),("E",50000)]),
    (27,[6727],[("I",6716),("E",50000)]),(37,[6724],[("I",6714),("E",50000)]),
    (53,[6720],[("I",6715),("E",50000)]),(67,[6734],[("I",6735)]),
    (74,[6732],[("I",6733)]),(87,[6702]*12,[("I",6703)]),
    (114,[6714,6715,6716,6717],[("I",6713)]),(126,[6741],[("I",6740)])):
    assert by_line[line]["give"]==[("I",v) for v in required] and by_line[line]["receive"]==reward
    assert by_line[line]["binding"]["completion_key"].endswith("disappear="+str(int(line==87)))
admission=court_stories["priestess-four-seasons"]
assert [t["contracts"] for t in admission["steps"][:4]]==[
    court_stories[k]["contracts"] for k in ("spring-dew","autumn-leaf","summer-petals","winter-snowflake")]
assert [t["item_vnums"] for t in admission["steps"][4:-1]]==[[6714],[6715],[6716],[6717]]
assert court_stories["fisherman-dozen-scales"]["steps"][0]["count"]==12
contacts={c["mob_vnum"]:c for c in court_map["contacts"]}
for v,c in contacts.items():
    assert c["keyword"] in court["mobs"][v]["keywords"] and len(c["topics"])<=32
    native=set(t for b in court["dialogue"] if b["giver_vnum"]==v for t in b["body"][0].rstrip("~").split())
    assert set(c["topics"])==native
for b in court["dialogue"]:
    assert set(b["body"][0].rstrip("~").split())&set(contacts[b["giver_vnum"]]["topics"])
assert all(not contacts[v]["topics"] for v in (6708,6718,6735))
parent=room=None;sources=collections.defaultdict(list);families=set()
assert collections.Counter(r["command"] for r in court["reset_commands"])=={"M":122,"G":42,"E":23,"O":9,"D":8,"F":5}
for r in court["reset_commands"]:
    c,v=r["command"],r["arguments"]
    assert v[4]==100 and v[5:]==[0,0,0]
    if c in ("M","F"):parent,room=v[1],v[3]
    families.add((c,tuple(v[:3]),parent if c in ("G","E") else None))
    if c in ("G","E"):sources[v[1]].append((parent,room,v[2]))
assert len(families)==92
assert sources[6722]==[(6731,6792,1)] and sources[6724]==[(6734,6793,1)] and sources[6727]==[(6724,6752,1)]
assert sources[6742]==[(6717,6787,1)] # Current follower is frost spirit, not Sieck.
assert sources[6732]==[(v,6702,8) for v in (6700,6702,6703,6701)]+[(v,6703,8) for v in (6721,6723,6720,6722)]
assert sources[6702]==[(6715,6755,15)]*5+[(6715,6758,15)]*3+[(6715,6761,15)]*7
assert collections.Counter(v for v,room,cap in sources[6734])=={6723:4,6702:1,6711:2}
assert [r["arguments"][1:4] for r in court["reset_commands"] if r["command"]=="F" and r["arguments"][1]==6716]==[[6716,2,6740],[6716,2,6749]]
assert [r["arguments"][1:4] for r in court["reset_commands"] if r["command"]=="O" and r["arguments"][1] in (6719,6720,6741)]==[[6719,1,6718],[6720,1,6771],[6741,1,6786]]
room_bodies={int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",(ROOT/"areas/wld/court.wld").read_text(encoding="utf8"),re.M|re.S)}
assert set(room_bodies)==set(range(6700,6799))
assert len({(b.split("~")[0].strip(),b.split("~")[1].strip()) for b in room_bodies.values()})==80
for room,flags,key,target in ((6709,6,6719,6710),(6710,6,6724,6711),(6711,6,6724,6710),
    (6704,7,6713,6798),(6798,3,0,6704),(6768,5,0,6769),(6796,4,0,6700),(6716,0,0,531688)):
    assert f"{flags} {key} {target}" in room_bodies[room]
for room,chance in ((6700,25),(6701,25),(6759,75),(6786,50),(6787,50)):
    assert re.search(r"\bF\s+"+str(chance)+r"\b",room_bodies[room])
obj_bodies={int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",(ROOT/"areas/obj/court.obj").read_text(encoding="utf8"),re.M|re.S)}
for v,target,command in ((6700,6796,7),(6705,6720,7),(6709,6770,7),(6718,6706,7),(6730,6713,264)):
    values=list(map(int,obj_bodies[v].split("~")[4].split()[:15]))
    assert values[0]==25 and values[11:14]==[target,command,-1]
assert list(map(int,obj_bodies[6713].split("~")[4].split()[:15]))[12]==100
assert obj_bodies[6719].split("~")[4].split()[0]=="33" and obj_bodies[6724].split("~")[4].split()[0]=="8"
for v,damage,level in ((6714,9,16),(6715,12,16),(6716,9,16),(6717,3,16),(6721,12,31),(6740,16,16)):
    assert re.search(r"\bT\s+2\s+"+str(damage)+r"\s+1\s+"+str(level)+r"\b",obj_bodies[v])
assert (ROOT/"areas/shp/court.shp").read_text(encoding="utf8").startswith("#6728~")
assert sources[6706]==[(6728,6794,999)]

# Snow Ogres retains distinct shards and exact support terms without personal-kill credit.
# Source gaps and control declarations are evidence for pending qualification, not repairs.
snogres=inventory_module.area_evidence(ROOT,"snogres")
snogres_map=next(m for m in catalog["story_mappings"] if m["source_area"]=="snogres")
snogres_stories={s["id"]:s for s in snogres_map["stories"]}
assert (snogres_map["schema_version"],snogres_map["revision"],snogres_map["coverage"])==(3,1,"complete")
assert len(snogres_stories)==8 and len(snogres_map["contacts"])==16 and len(snogres_map["exclusions"])==1
assert sum(s["category"]=="service" for s in snogres_stories.values())==1
assert sum(t.get("optional",False) for s in snogres_stories.values() for t in s["steps"])==15
assert (len(snogres["requests"]),len(snogres["dialogue"]),len(snogres["mobs"]),len(snogres["items"]),len(snogres["reset_commands"]))==(9,6,44,38,200)
assert collections.Counter(b["kind"] for b in inventory_module.native_blocks(ROOT) if b["source"]=="areas/qst/snogres.qst")=={"Q":8,"QA":1,"M":6}
bindings=[b for s in list(snogres_stories.values())+snogres_map["exclusions"] for b in s["contracts"]]
assert len(bindings)==9 and {tuple(sorted(b.items())) for b in bindings}=={
    tuple(sorted(r["block"]["binding"].items())) for r in snogres["requests"]}
units=[u for u in catalog_module.story_units(catalog) if u["zone_number"]==877]
assert (len(units),sum(u["achievement"] for u in units),sum(u["daily_candidate"] for u in units))==(8,7,7)
by_line={r["block"]["line"]:r["block"] for r in snogres["requests"]}
for line,required,reward in ((19,[("I",87706)],87713),(34,[("I",87713),("I",87730),("I",87731)],87732),
    (54,[("I",87715)],87730),(64,[("I",87717)],87731),(71,[("I",87716)],87729),
    (83,[("I",87719)],87727),(91,[("I",87718)],87714),(108,[("I",87725)],87725),
    (159,[("C",2500000)]+[("I",87725)]*6+[("I",87710),("I",87711)],87728)):
    assert by_line[line]["give"]==required and by_line[line]["receive"]==[("I",reward)]
    assert by_line[line]["binding"]["completion_key"].endswith("disappear=0")
pyramid=snogres_stories["lich-three-shards"]
assert [t["contracts"] for t in pyramid["steps"][:3]]==[
    snogres_stories[k]["contracts"] for k in ("lich-reverse-hourglass","lich-astereater-eye","lich-illithid-tentacle")]
assert [t["item_vnums"] for t in pyramid["steps"][3:-1]]==[[87713],[87730],[87731]]
armor=snogres_stories["leppts-remorhaz-armor"]
assert armor["steps"][0]["count"]==6 and "2,500 platinum" in armor["steps"][-1]["hint"]
assert "guarded" in armor["steps"][-1]["hint"]
assert snogres_map["exclusions"][0]["contracts"]==[by_line[108]["binding"]]
contacts={c["mob_vnum"]:c for c in snogres_map["contacts"]}
for v,c in contacts.items():
    assert c["keyword"] in snogres["mobs"][v]["keywords"] and len(c["topics"])<=32
    native=set(t for b in snogres["dialogue"] if b["giver_vnum"]==v for t in b["body"][0].rstrip("~").split())
    assert set(c["topics"])==native
for b in snogres["dialogue"]:
    assert set(b["body"][0].rstrip("~").split())&set(contacts[b["giver_vnum"]]["topics"])
parent=room=None;sources=collections.defaultdict(list);families=set();mob_sources=collections.defaultdict(list)
assert collections.Counter(r["command"] for r in snogres["reset_commands"])=={"M":139,"E":22,"F":18,"G":15,"D":4,"O":2}
for r in snogres["reset_commands"]:
    c,v=r["command"],r["arguments"]
    assert v[5:]==[0,0,0]
    if c in ("M","F"):parent,room=v[1],v[3];mob_sources[v[1]].append((room,v[4]))
    families.add((c,tuple(v[:3]),parent if c in ("G","E") else None))
    if c in ("G","E"):sources[v[1]].append((parent,room,v[2]))
assert len(families)==80
assert sources[87706]==[(87729,87793,1)] and sources[87710]==sources[87711]==[(87704,87763,1)]
assert sources[87725]==[(87724,r,3) for r in (87747,87754,87759)]
assert mob_sources[87724]==[(r,100) for r in (87747,87752,87754,87759)]
for v,room,chance in ((87725,87746,35),(87736,87735,20),(87737,87722,15),(87715,87730,5),(87735,87739,15),(87741,87717,40)):
    assert mob_sources[v]==[(room,chance)]
assert not mob_sources[87742] and not sources[87719]
active=[row.split()[0] for row in (ROOT/"areas/AREA").read_text().splitlines() if row.strip() and not row.startswith("*")]
assert "brass-old-1" not in active
active_stalk=[];active_hides=[];leppts_sources=[]
for area in active:
    for line,raw in enumerate((ROOT/f"areas/zon/{area}.zon").read_text(errors="replace").splitlines(),1):
        m=re.match(r"^([MOGEPF])\s+((?:-?\d+\s*)+)",raw)
        if not m:continue
        c=m[1];v=list(map(int,m[2].split()))
        if c in "OGEP" and v[1]==87719:active_stalk.append((area,line))
        if c in "OGEP" and v[1]==87725:active_hides.append((area,line))
        if c=="M" and v[1]==87742:leppts_sources.append((area,v[2:5]))
assert not active_stalk and len(active_hides)==3 and {a for a,l in active_hides}=={"snogres"}
assert leppts_sources==[("surface",[1,660001,100])]
assert not any(("I",87719) in b["receive"] for b in inventory_module.native_blocks(ROOT))
assert [(b["giver_vnum"],b["give"]) for b in inventory_module.native_blocks(ROOT) if ("I",87725) in b["receive"]]==[(87733,[("I",87725)])]
room_bodies={int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",(ROOT/"areas/wld/snogres.wld").read_text(encoding="utf8"),re.M|re.S)}
assert set(room_bodies)==set(range(87700,87800))
assert len({(b.split("~")[0].strip(),b.split("~")[1].strip()) for b in room_bodies.values()})==71
for room,flags,key,target in ((87717,13,0,87718),(87718,5,0,87717),(87745,5,0,87746),
    (87746,5,0,87745),(87758,0,0,87766),(87700,0,0,620605)):
    assert f"{flags} {key} {target}" in room_bodies[room]
assert "D5" not in room_bodies[87720] and re.search(r"\bF\s+20\b",room_bodies[87720])
for v in (87794,87795):assert "D5" in room_bodies[v] and re.search(r"\bF\s+100\b",room_bodies[v])
obj_bodies={int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",(ROOT/"areas/obj/snogres.obj").read_text(encoding="utf8"),re.M|re.S)}
values=list(map(int,obj_bodies[87735].split("~")[4].split()[:15]))
assert values[0]==29 and values[11:15]==[270,87717,3,0]
assert len(snogres["special_assignments"])==12
assert [(a["vnum"],a["function"]) for a in snogres["special_assignments"] if a["vnum"]==87734]==[(87734,"block_dir"),(87734,"snogres_flesh_golem")]

for area in ("twin_towers_forest", "newbie2", "newbie", "braddistock", "breale", "elvish", "krimman", "bastine", "pineholl", "quietus", "torg", "solonar", "wh", "smokev", "caertannad", "bs", "moria", "clwcvrn", "long", "blackpearl", "ravenloft2", "barovia", "tikitt", "jade", "savannah", "alatorin", "newhaven", "realm", "verspin", "shipy", "cosmic", "surface", "tharnadia", "minizones", "torrhan", "gold_hal", "ashrumite", "hall", "sarmiz", "delwyn", "divhome", "halfcut", "scorchvalley", "court", "snogres"):
    assert inventory_module.review_index(ROOT, area) == (ROOT / f"docs/reference/zone-story-audits/{area}.md").read_text(encoding="utf-8")

with tempfile.TemporaryDirectory(prefix="duris-zone-story-production-catalog-") as temporary:
    output = pathlib.Path(temporary) / "catalog.json"
    subprocess.run(
        [
            sys.executable,
            str(SCRIPT),
            "--source-root",
            str(ROOT),
            "--production-output",
            str(output),
            "--check",
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    written = json.loads(output.read_text(encoding="utf-8"))
    assert written["source"]["fingerprint_sha256"] == catalog["source"]["fingerprint_sha256"]
    assert len(written["definitions"]) == len(catalog["definitions"])

with tempfile.TemporaryDirectory(prefix="duris-zone-story-sidecar-boundary-") as temporary:
    fixture = pathlib.Path(temporary)
    (fixture / "areas/zon").mkdir(parents=True)
    (fixture / "areas/story").mkdir()
    (fixture / "areas/AREA").write_text("sample\n", encoding="utf-8")
    (fixture / "areas/zon/sample.zon").write_text("#1\nSample~\n199 2\n", encoding="utf-8")
    mapping = {"schema_version": 1, "revision": 1, "source_area": "sample",
               "coverage": "complete", "stories": [], "exclusions": []}
    path = fixture / "areas/story/sample.story.json"
    encoded = json.dumps(mapping).encode("utf-8")
    assert catalog_module.MAX_STORY_MAPPING_BYTES == 512 * 1024
    path.write_bytes(encoded + b" " * (catalog_module.MAX_STORY_MAPPING_BYTES - len(encoded)))
    assert catalog_module.production_catalog(fixture)["story_mappings"] == [mapping]
    with path.open("ab") as output:
        output.write(b" ")
    try:
        catalog_module.production_catalog(fixture)
    except ValueError as error:
        assert "exceeds 512 KiB" in str(error)
    else:
        raise AssertionError("oversized source sidecar was accepted")

print("zone-story production catalog coverage regression passed")
