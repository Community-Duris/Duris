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

# Dawndale retains exact competing bundles, actual producers and support/referral boundaries.
# Source-comprehensive coverage does not certify guarded payment or narrated world effects.
dawndale=inventory_module.area_evidence(ROOT,"airshipgrave")
dawndale_map=next(m for m in catalog["story_mappings"] if m["source_area"]=="airshipgrave")
dawndale_stories={s["id"]:s for s in dawndale_map["stories"]}
assert (dawndale_map["schema_version"],dawndale_map["revision"],dawndale_map["coverage"])==(3,1,"complete")
assert len(dawndale_stories)==12 and len(dawndale_map["contacts"])==27 and len(dawndale_map["exclusions"])==1
assert sum(s["category"]=="service" for s in dawndale_stories.values())==3
assert sum(t.get("optional",False) for s in dawndale_stories.values() for t in s["steps"])==31
assert (len(dawndale["requests"]),len(dawndale["dialogue"]),len(dawndale["mobs"]),len(dawndale["items"]),len(dawndale["reset_commands"]))==(13,4,69,66,321)
assert not dawndale["special_assignments"]
assert collections.Counter(b["kind"] for b in inventory_module.native_blocks(ROOT) if b["source"]=="areas/qst/airshipgrave.qst")=={"Q":11,"QA":2,"M":4}
bindings=[b for s in list(dawndale_stories.values())+dawndale_map["exclusions"] for b in s["contracts"]]
assert len(bindings)==13 and {tuple(sorted(b.items())) for b in bindings}=={
    tuple(sorted(r["block"]["binding"].items())) for r in dawndale["requests"]}
units=[u for u in catalog_module.story_units(catalog) if u["zone_number"]==775]
assert (len(units),sum(u["achievement"] for u in units),sum(u["daily_candidate"] for u in units))==(12,9,9)
by_line={r["block"]["line"]:r["block"] for r in dawndale["requests"]}
for line,required,reward,retire in (
    (2,[("C",250000)],[("I",77501)],0),
    (8,[("I",77550)],[("E",250000),("I",77562)],1),
    (21,[("I",77508)],[("I",77508)],0),
    (30,[("I",77523)],[("C",250000),("I",77524)],0),
    (41,[("I",77517)],[("I",77516)],0),
    (49,[("I",77522)],[("I",77523)],0),
    (60,[("I",77501),("I",77525)],[("I",34464),("I",77563)],1),
    (82,[("I",77524)],[("I",77525)],1),
    (131,[("I",77513)],[("E",250000),("I",77552)],0),
    (159,[("I",21673),("C",25000)],[("I",77566)],0),
    (180,[("I",40778)],[("I",40779)],1)):
    assert by_line[line]["give"]==required and by_line[line]["receive"]==reward
    assert by_line[line]["binding"]["completion_key"].endswith("disappear="+str(retire))
for line,reward in ((101,77548),(113,77558)):
    assert collections.Counter(by_line[line]["give"])==collections.Counter([("I",77515)]*2+[("I",v) for v in (77524,77556,77549,77551,77557)])
    assert by_line[line]["receive"]==[("I",reward)]
    story=dawndale_stories["astral-captain-supply" if line==101 else "dlalgarvara-captain-supply"]
    assert story["steps"][1]["count"]==2 and len([t for t in story["steps"] if t["kind"]=="carried_item"])==6
assert by_line[101]["binding"]!=by_line[113]["binding"]
final=dawndale_stories["refugee-key-and-device"]
assert [t["contracts"] for t in final["steps"][:4]]==[dawndale_stories[k]["contracts"] for k in ("refugee-gold-portrait","rockcutter-combustible-dust","engineer-explosive-device","exile-city-key")]
assert [t["item_vnums"] for t in final["steps"][4:-1]]==[[77501],[77525]]
assert "250 platinum" in dawndale_stories["exile-city-key"]["steps"][-1]["hint"]
assert "25 platinum" in dawndale_stories["smith-sand-lens"]["steps"][-1]["hint"]
assert "guarded" in dawndale_stories["smith-sand-lens"]["steps"][-1]["hint"]
emition=next(b for b in inventory_module.native_blocks(ROOT) if b["source"]=="areas/qst/divhome.qst" and b["line"]==156)
assert emition["give"]==[("I",77539)] and emition["receive"]==[("I",40778)]
assert dawndale_stories["whetstone-flute-return"]["steps"][0]["contracts"]==[emition["binding"]]
assert dawndale_map["exclusions"][0]["contracts"]==[by_line[21]["binding"]]
contacts={c["mob_vnum"]:c for c in dawndale_map["contacts"]}
for v,c in contacts.items():
    assert c["keyword"] in inventory_mobs[v]["keywords"] and len(c["topics"])<=32
    native=set(t for b in dawndale["dialogue"] if b["giver_vnum"]==v for t in b["body"][0].rstrip("~").split())
    assert set(c["topics"])==native
assert set(contacts[77543]["topics"])=={"lens","telescope","pile","fine","sand"}
assert contacts[77558]["topics"]==["metal"] and not contacts[77517]["topics"]
parent=room=None;sources=collections.defaultdict(list);families=set();mob_sources=collections.defaultdict(list)
assert collections.Counter(r["command"] for r in dawndale["reset_commands"])=={"M":161,"E":54,"D":38,"G":35,"O":20,"F":7,"P":6}
for r in dawndale["reset_commands"]:
    c,v=r["command"],r["arguments"]
    assert v[5:]==[0,0,0]
    if c in ("M","F"):parent,room=v[1],v[3];mob_sources[v[1]].append((room,v[4]))
    families.add((c,tuple(v[:3]),parent if c in ("G","E") else None))
    if c in ("G","E"):sources[v[1]].append((parent,room,v[2]))
assert len(families)==175
assert sources[77515]==[(77520,77593,3)]*3
for item,mob,room in ((77517,77541,77528),(77522,77554,77562),(77549,77567,77646),(77551,77568,77600),(77557,77569,77621),(77547,77566,77647),(77528,77556,77518),(77518,77546,77630),(77560,77521,77595)):
    assert sources[item]==[(mob,room,1)]
for v,room,chance in ((77501,77511,33),(77505,77549,50),(77561,77568,35),(77549,77581,35),(77568,77600,50)):
    assert mob_sources[v]==[(room,chance)]
assert any(r["command"]=="P" and r["arguments"][1:5]==[77550,1,77547,100] for r in dawndale["reset_commands"])
assert any(r["command"]=="O" and r["arguments"][1:5]==[77513,1,77622,100] for r in dawndale["reset_commands"])
assert re.search(r"^P\s+1\s+77551\s+1\s+40\s+100",(ROOT/"areas/zon/heavens.zon").read_text(),re.M)
assert re.search(r"^M\s+0\s+21673\s+1\s+21568\s+100",(ROOT/"areas/zon/clfhaven.zon").read_text(),re.M)
assert any(a["vnum"]==21673 and a["function"]=="wh_corpse_to_object" for a in inventory_module.special_assignments((ROOT/"src/specs/specs.assign.c").read_text()))
def dawndale_bodies(area,kind):
    return {int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",(ROOT/f"areas/{kind}/{area}.{kind}").read_text(),re.M|re.S)}
rooms=dawndale_bodies("airshipgrave","wld")
assert set(rooms)==set(range(77501,77651))
assert len({(b.split("~")[0].strip(),b.split("~")[1].strip()) for b in rooms.values()})==83
for room,flags,key,target in ((77542,12,0,77634),(77634,12,0,77542),(77508,3,77501,77516),(77525,3,77528,77526),(77533,3,77518,77534),(77595,2,77560,77596),(77596,2,77540,77604),(77604,2,0,77596),(77543,0,0,616080),(77557,0,0,617684)):
    assert f"{flags} {key} {target}" in rooms[room]
assert all(not re.search(r"^F\s+",b,re.M) for b in rooms.values())
assert "77627" not in rooms[77627] and "77602" in rooms[77627]
objects=dawndale_bodies("airshipgrave","obj")
def objvalues(body):return list(map(int,re.match(r"\s*((?:-?\d+\s+)+)",body.split("~")[4])[1].split()))
for v,room,direction in ((77541,77542,1),(77542,77634,3)):
    values=objvalues(objects[v]);assert values[0]==29 and values[11:15]==[270,room,direction,0]
assert objvalues(objects[77501])[12]==0
for v in (77540,77560):assert objvalues(objects[v])[12]==100
assert objvalues(objects[77528])[12]==0 and objvalues(objects[77521])[11:14]==[522469,7,-1]
assert objvalues(objects[77547])[12]==5 and objvalues(objects[77505])[12]==13 and re.search(r"\bT\s+516\s+5\s+5\s+50",objects[77505])
assert objvalues(objects[77550])[0]==8 and objvalues(objects[77531])[0]==20
assert "_vict_msg~" in objects[77548] and objvalues(objects[77548])[16:19]==[103,50,30]
flutes=dawndale_bodies("divhome","obj")
assert flutes[40778].split("~")[:3]==flutes[40779].split("~")[:3]
assert objvalues(flutes[40778])[11:13]==[184,0] and objvalues(flutes[40779])[11:13]==[184,35]
assert inventory_items[34464]["source"]=="areas/obj/long.obj" and all(("I",77559) not in b["receive"] for b in by_line.values())

# Juiblex retains exact sources, competing recipients and equivalent Marvin offers.
# Source-comprehensive guidance does not qualify custom generation/phase transfer.
abyss=inventory_module.area_evidence(ROOT,"juiblex")
abyss_map=next(m for m in catalog["story_mappings"] if m["source_area"]=="juiblex")
abyss_stories={s["id"]:s for s in abyss_map["stories"]}
assert (abyss_map["schema_version"],abyss_map["revision"],abyss_map["coverage"])==(3,1,"complete")
assert len(abyss_stories)==22 and len(abyss_map["contacts"])==40 and not abyss_map["exclusions"]
assert all(s["category"]=="story" for s in abyss_stories.values())
assert sum(t.get("optional",False) for s in abyss_stories.values() for t in s["steps"])==39
assert (len(abyss["requests"]),len(abyss["dialogue"]),len(abyss["mobs"]),len(abyss["items"]),len(abyss["reset_commands"]))==(23,25,117,113,378)
assert collections.Counter(b["kind"] for b in inventory_module.native_blocks(ROOT) if b["source"]=="areas/qst/juiblex.qst")=={"Q":22,"QA":1,"M":25}
by_line={r["block"]["line"]:r["block"] for r in abyss["requests"]}
for line,required,reward,retire in (
    (62,[87526],[87529],0),(78,[87527],[87530],0),(94,[87562],[87563],0),
    (155,[87509],[87548],0),(177,[71024],[87594],0),(248,[87512],[87571],0),
    (284,[87572,87571],[87514],1),(365,[87532],[87533],0),(379,[87544],[87549],0),
    (425,[87551],[87552],0),(487,[87577,87578,87579,87580],[87581],0),
    (513,[87582],[87593],0),(593,[87553,87554,87555,87556,87558],[87561],0),
    (623,[87606],[87607],0),(664,[87563],[87565],0),(705,[87510],[87576],0),
    (729,[87504],[87586],0),(759,[87587,87588,87589,87590],[87591],0),
    (787,[87599],[87600,87601],1),(822,[87609],[87610],1),
    (848,[87609],[87610],1),(863,[87610],[87608],0)):
    assert collections.Counter(by_line[line]["give"])==collections.Counter(("I",v) for v in required)
    assert by_line[line]["receive"]==[("I",v) for v in reward]
    assert by_line[line]["binding"]["completion_key"].endswith("disappear="+str(retire))
assert by_line[314]["give"]==[("I",55364),("I",55365)] and collections.Counter(by_line[314]["receive"])==collections.Counter([("C",500000),("I",55279)])
assert by_line[314]["binding"]["completion_key"].endswith("disappear=0")
bindings=[b for s in abyss_stories.values() for b in s["contracts"]]
assert len(bindings)==23 and {tuple(sorted(b.items())) for b in bindings}=={tuple(sorted(b["binding"].items())) for b in by_line.values()}
leash=abyss_stories["marvin-old-leash"]
assert leash["contracts"]==[by_line[822]["binding"],by_line[848]["binding"]]
assert abyss_stories["marvin-given-leash"]["steps"][0]["contracts"]==leash["contracts"]
for name,line in (("uz-wand-refill",248),("maxilo-placation-gloves",94)):
    assert abyss_stories[name]["steps"][0]["contracts"]==[by_line[line]["binding"]]
foreign={b["source"]:b for b in inventory_module.native_blocks(ROOT) if b["kind"] in ("Q","QA") and (("I",55363) in b["receive"] or ("I",71024) in b["receive"])}
assert foreign["areas/qst/torg.qst"]["give"]==[("I",v) for v in range(28944,28952)]
assert foreign["areas/qst/fields_between.qst"]["give"]==[("I",71005),("I",71016)]
assert abyss_stories["uz-legend-halves"]["steps"][0]["contracts"]==[foreign["areas/qst/torg.qst"]["binding"]]
assert abyss_stories["troll-brewer-fez"]["steps"][0]["contracts"]==[foreign["areas/qst/fields_between.qst"]["binding"]]
units=[u for u in catalog_module.story_units(catalog) if u["zone_number"]==875]
assert (len(units),sum(u["achievement"] for u in units),sum(u["daily_candidate"] for u in units))==(22,22,19)
assert {r["block"]["line"] for r in abyss["requests"] if not r["definition"]["daily_eligible"]}=={284,787,822,848}
contacts={c["mob_vnum"]:c for c in abyss_map["contacts"]}
for v,c in contacts.items():
    assert c["keyword"] in inventory_mobs[v]["keywords"] and len(c["topics"])<=32
    assert set(c["topics"])==set(t for b in abyss["dialogue"] if b["giver_vnum"]==v for t in b["body"][0].rstrip("~").split())
assert len(contacts[87527]["topics"])==32
assert collections.Counter(r["command"] for r in abyss["reset_commands"])=={"M":201,"D":48,"G":40,"E":39,"F":26,"O":22,"P":2}
parent=room=None;sources=collections.defaultdict(list);mobile_sources=collections.defaultdict(list);families=set()
for r in abyss["reset_commands"]:
    c,v=r["command"],r["arguments"]
    assert v[5:]==[0,0,0]
    if c in ("M","F"):parent,room=v[1],v[3];mobile_sources[v[1]].append((room,v[4]))
    families.add((c,tuple(v[:3]),parent if c in ("G","E") else None))
    if c in ("G","E"):sources[v[1]].append((parent,room,v[2]))
assert len(families)==257
for item,mob,where,chance in ((87553,87568,87654,25),(87554,87569,87654,100),(87555,87570,87645,33),(87556,87567,87648,33),(87558,87566,87643,33)):
    assert sources[item]==[(mob,where,1)] and mobile_sources[mob]==[(where,chance)]
for item,mob,where in ((87512,87529,87590),(87513,87530,87590),(87572,87509,87565),(87532,87557,87639),(87551,87579,87661),(87544,87573,87579),(87609,87581,87539),(87606,87611,87671),(87599,87608,87680),(87520,87544,87626),(87502,87561,87626)):
    assert sources[item]==[(mob,where,1)]
assert sources[87562]==[(87581,87539,1)] and mobile_sources[87581]==[(87539,40)]
world=(ROOT/"areas/wld/juiblex.wld").read_text()
room_bodies={int(m[1]):m[2] for m in re.finditer(r"^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)",world,re.M|re.S)}
assert len(room_bodies)==183
for local,target,mob in ((87671,71106,87600),(87672,29073,87601),(87673,9106,87602),(87674,71221,87603)):
    assert re.search(r"D5\s+~\s*~\s*0 0 "+str(target),room_bodies[local])
    assert not re.search(r"\bF\s+\d+",room_bodies[local]) and mobile_sources[mob]==[(local,60)]
    raw=(ROOT/inventory_mobs[mob]["source"]).read_text().split("#"+str(mob)+"\n",1)[1].split("#",1)[0]
    assert not int(raw.split("~")[4].split()[0])&(2|64)
objects=(ROOT/"areas/obj/juiblex.obj").read_text()
for key,chance in ((87513,0),(87514,100),(87515,100),(87520,100),(87570,100)):
    raw=objects.split("#"+str(key)+"\n",1)[1].split("#",1)[0]
    assert int(raw.split("~")[4].split()[12])==chance
assert re.search(r"D0\s+[^~]*~[^~]*~\s*9 0 87609",room_bodies[87606],re.S)
assert any(r["command"]=="D" and r["arguments"][:5]==[0,87606,0,9,100] for r in abyss["reset_commands"])
assert not any(r["command"]=="D" and r["arguments"][3]&4 for r in abyss["reset_commands"])
assert {(a["kind"],a["vnum"],a["function"]) for a in abyss["special_assignments"]}=={("obj",87612,"doombringer"),("mob",87542,"slime_lake"),("mob",87543,"juiblex_one"),("obj",87546,"mask_of_wildmagic"),("obj",87601,"flow_amulet"),("obj",87611,"juiblex_grid_mob_generator")}
procedures=(ROOT/"src/specs/specs.juiblex.c").read_text()
phase=procedures.split("int juiblex_one(",1)[1].split("int mask_of_wildmagic(",1)[0]
assert "char_to_room(tch" in phase and "char_from_room" not in phase
assert "refusing duplicate insertion" in (ROOT/"src/world/handler.c").read_text()
generator=procedures.split("int juiblex_grid_mob_generator(",1)[1]
assert all(str(v) in generator for v in (87507,87505,87508,87604,87599,87504,87506,87515,87552,87514))
assert not any(str(v) in generator for v in range(87566,87571))
assert "attack_continuation" in (ROOT/"src/specs/specs.underworld.c").read_text().split("int doombringer(",1)[1].split("int unholy_avenger_bloodlust(",1)[0]
legend=(ROOT/"src/specs/specs.winterhaven.c").read_text().split("int lorekeeper_scroll(",1)[1].split("\nint ",1)[0]
assert "extract_obj(obj, TRUE)" in legend and "number(55364, 55365)" in legend


# Surface commissions and alternatives preserve exact source contracts without extra credit.
surface_mini=inventory_module.area_evidence(ROOT,"surfacemini")
surface_map=next(m for m in catalog["story_mappings"] if m["source_area"]=="surfacemini")
surface_stories={s["id"]:s for s in surface_map["stories"]}
assert (surface_map["schema_version"],surface_map["revision"],surface_map["coverage"])==(3,1,"complete")
assert len(surface_stories)==21 and len(surface_map["contacts"])==38 and not surface_map["exclusions"]
assert collections.Counter(s["category"] for s in surface_stories.values())=={"story":5,"service":16}
assert sum(t.get("optional",False) for s in surface_stories.values() for t in s["steps"])==73
assert (len(surface_mini["requests"]),len(surface_mini["dialogue"]),len(surface_mini["mobs"]),len(surface_mini["items"]),len(surface_mini["reset_commands"]))==(25,32,25,42,145)
assert collections.Counter(b["kind"] for b in inventory_module.native_blocks(ROOT) if b["source"]=="areas/qst/surfacemini.qst")=={"Q":20,"QA":5,"M":30,"MA":4}
by_line={r["block"]["line"]:r["block"] for r in surface_mini["requests"]}
for line,required,reward,coins,retire in (
    (26,[15008],[97930],0,0),(33,[28553,28552],[97903],0,0),
    (165,[55554,55553,55552,55551,55550],[97920],100000,0),
    (174,[97920]*3,[97919],250000,0),(181,[53650,97921,400280,400280,400280],[53661],0,0),
    (189,[53651,97921,400280,400280,400280],[53662],0,0),(197,[53652,97921,400280,400280,400280],[53663],0,0),
    (205,[97919]*3,[97922],1000000,0),(212,[53646,400280,400280,400280,97921],[53664],0,0),
    (220,[97921,400280,400280,400280,500003],[97931],0,0),(228,[97921,400280,400280,400280,500046],[97932],0,0),
    (236,[55033,55033,55033,55246],[97933,97933],0,0),(261,[43138]*8,[97910],0,1),
    (341,[55198,55199,55200,55201,55202,55203],[97921,97923],0,1),
    (421,[55033,55246],[97934]*3,0,0),(428,[55033]*3+[55238],[97936],0,0),
    (435,[55550,55551,55552,55553,55554,55238],[97935]*2,0,0),
    (445,[41916]+[55033]*3,[55075],0,0),(452,[55247],[97937]*4,25000,0),
    (460,[55554],[97938]*2,10000,0),(466,[55553],[97938]*2,10000,0),
    (472,[55552],[97938]*2,10000,0),(478,[55551],[97938]*2,10000,0),(484,[55550],[97938]*2,10000,0)):
    assert by_line[line]["give"]==[("I",v) for v in required]+([("C",coins)] if coins else [])
    assert by_line[line]["receive"]==[("I",v) for v in reward]
    assert by_line[line]["binding"]["completion_key"].endswith("disappear="+str(retire))
assert by_line[302]["give"]==[("I",16635),("I",3003)] and by_line[302]["receive"]==[("E",15000),("C",16000)]
assert by_line[302]["binding"]["completion_key"].endswith("disappear=1")
bindings=[b for entry in surface_stories.values() for b in entry["contracts"]]
assert len(bindings)==25 and {tuple(sorted(b.items())) for b in bindings}=={tuple(sorted(b["binding"].items())) for b in by_line.values()}
cleansing=surface_stories["mug-cleansing-potions"]
assert cleansing["contracts"]==[by_line[n]["binding"] for n in (460,466,472,478,484)]
assert cleansing["steps"][0]["item_vnums"]==[55550,55551,55552,55553,55554] and cleansing["steps"][0]["count"]==1
for name,item,count in (("gleb-eight-glands",43138,8),("cosmo-greater-healing",97920,3),("cosmo-ultimate-healing",97919,3),("cosmo-damnation-staff",400280,3)):
    assert any(t.get("item_vnums")==[item] and t["count"]==count for t in surface_stories[name]["steps"])
assert {t["item_vnums"][0] for t in surface_stories["strange-six-essences"]["steps"] if t["kind"]=="carried_item"}=={55198,55199,55200,55201,55202,55203}
assert {t["item_vnums"][0] for t in surface_stories["hermit-clothing-recipe"]["steps"] if t["kind"]=="carried_item"}=={28552,28553}
assert {t["item_vnums"][0] for t in surface_stories["mug-time-vials"]["steps"] if t["kind"]=="carried_item"}=={55550,55551,55552,55553,55554,55238}
all_blocks=inventory_module.native_blocks(ROOT)
drug=surface_stories["mug-potent-elixirs"]["steps"][0]
assert drug["contracts"]==[next(b["binding"] for b in all_blocks if b["source"]==src and b["line"]==line) for src,line in (("areas/qst/wh.qst",1342),("areas/qst/alatorin.qst",6526))]
incarnate_producer=surface_stories["cosmo-lesser-healing"]["steps"][0]["contracts"][0]
assert incarnate_producer==next(b["binding"] for b in all_blocks if b["source"]=="areas/qst/wh.qst" and b["line"]==2619)
assert surface_stories["cosmo-greater-healing"]["steps"][0]["contracts"]==surface_stories["cosmo-lesser-healing"]["contracts"]
assert surface_stories["cosmo-ultimate-healing"]["steps"][0]["contracts"]==surface_stories["cosmo-greater-healing"]["contracts"]
contacts={c["mob_vnum"]:c for c in surface_map["contacts"]}
for dialogue in surface_mini["dialogue"]:
    assert set(dialogue["body"][0].rstrip("~").split())<=set(contacts[dialogue["giver_vnum"]]["topics"])
assert all("qc_action" not in c["topics"] for c in contacts.values())
assert collections.Counter(r["command"] for r in surface_mini["reset_commands"])=={"M":97,"O":14,"G":12,"E":11,"D":10,"R":1}
assert any(r["command"]=="R" and r["arguments"]==[1,97915,7,98001,100,0,0,0] for r in surface_mini["reset_commands"])
assert all(r["arguments"][-3:]==[0,0,0] for r in surface_mini["reset_commands"])
assert {(a["kind"],a["vnum"],a["function"]) for a in surface_mini["special_assignments"]}=={("obj",97923,"elemental_wand"),("obj",97932,"collar_frost"),("obj",97931,"collar_flames")}
assert {r["block"]["line"] for r in surface_mini["requests"] if not r["definition"]["daily_eligible"]}=={165,174,205,452,460,466,472,478,484}
surface_units=[u for u in catalog_module.story_units(catalog) if u["zone_number"]==979]
assert (len(surface_units),sum(u["achievement"] for u in surface_units),sum(u["daily_candidate"] for u in surface_units))==(21,5,5)
assert {s["id"] for s in surface_stories.values() if s["category"]=="story"}=={"hermit-green-potion","hermit-clothing-recipe","gleb-eight-glands","hermit-salmon-firebreather","strange-six-essences"}


# Peril Peaks preserves exact linked proof without inventing access or companion completion.
nexus=inventory_module.area_evidence(ROOT,"nexus")
nexus_map=next(m for m in catalog["story_mappings"] if m["source_area"]=="nexus")
nexus_stories={s["id"]:s for s in nexus_map["stories"]}
assert (nexus_map["schema_version"],nexus_map["revision"],nexus_map["coverage"])==(3,1,"complete")
assert len(nexus_stories)==10 and len(nexus_map["contacts"])==28 and not nexus_map["exclusions"]
assert all(s["category"]=="story" for s in nexus_stories.values())
assert collections.Counter(t["kind"] for s in nexus_stories.values() for t in s["steps"] if t.get("optional"))=={"carried_item":13,"completion":3}
assert (len(nexus["requests"]),len(nexus["dialogue"]),len(nexus["mobs"]),len(nexus["items"]),len(nexus["reset_commands"]))==(10,11,162,85,297)
assert collections.Counter(b["kind"] for b in inventory_module.native_blocks(ROOT) if b["source"]=="areas/qst/nexus.qst")=={"Q":10,"M":11}
by_line={r["block"]["line"]:r["block"] for r in nexus["requests"]}
for line,required,reward in ((12,[57522],[("I",57523)]),(36,[57535,57536,57534],[("I",57537)]),
    (46,[57555,57566],[("I",57519)]),(55,[57540],[("I",57569)]),(99,[57545],[("E",5000)]),
    (104,[57561],[("I",57548)]),(119,[57548],[("I",57511)]),(184,[57554],[("I",57549)]),
    (202,[57523],[("I",57583)]),(217,[57549],[("I",57504)])):
    assert by_line[line]["give"]==[("I",v) for v in required] and by_line[line]["receive"]==reward
    assert by_line[line]["binding"]["completion_key"].endswith("disappear=0")
bindings=[b for entry in nexus_stories.values() for b in entry["contracts"]]
assert len(bindings)==10 and {tuple(sorted(b.items())) for b in bindings}=={tuple(sorted(b["binding"].items())) for b in by_line.values()}
for later,earlier in (("roxon-silver-stud","foreman-silver-bag"),("gooran-parchment","barbarian-sasquach-arm"),("human-troll-eye","troll-skeleton-head")):
    assert nexus_stories[later]["steps"][0]["optional"] and nexus_stories[later]["steps"][0]["contracts"]==nexus_stories[earlier]["contracts"]
assert {t["item_vnums"][0] for t in nexus_stories["hunter-three-reptile-scales"]["steps"] if t["kind"]=="carried_item"}=={57534,57535,57536}
assert {t["item_vnums"][0] for t in nexus_stories["hunter-two-tentacles"]["steps"] if t["kind"]=="carried_item"}=={57555,57566}
assert all(t.get("count",1)==1 for entry in nexus_stories.values() for t in entry["steps"])
contacts={c["mob_vnum"]:c for c in nexus_map["contacts"]}
for dialogue in nexus["dialogue"]:
    assert set(dialogue["body"][0].rstrip("~").split())<=set(contacts[dialogue["giver_vnum"]]["topics"])
assert all("qc_action" not in c["topics"] for c in contacts.values())
assert not nexus["special_assignments"]
assert collections.Counter(r["command"] for r in nexus["reset_commands"])=={"M":140,"D":58,"E":55,"G":25,"F":10,"O":7,"P":2}
assert all(r["arguments"][-3:]==[0,0,0] for r in nexus["reset_commands"])
assert all(r["definition"]["daily_eligible"] for r in nexus["requests"])
assert len([u for u in catalog_module.story_units(catalog) if u["zone_number"]==575])==10
nexus_sources=set();nexus_parent=None
for reset in nexus["reset_commands"]:
    if reset["command"] in "MF":nexus_parent=reset["arguments"][1]
    elif reset["command"]=="G":nexus_sources.add((reset["arguments"][1],nexus_parent))
assert {(57522,57544),(57534,57549),(57535,57546),(57536,57545),(57555,57520),(57566,57531),(57540,57521),(57545,57570),(57561,57573),(57554,57586),(57558,57527),(57567,57528)}<=nexus_sources
objects=dawndale_bodies("nexus","obj");world=dawndale_bodies("nexus","wld");mobiles=dawndale_bodies("nexus","mob")
assert objvalues(objects[57504])[0]==18 and objvalues(objects[57504])[12]==0
assert objvalues(objects[57558])[11:15]==[320,57507,0,1] and objvalues(objects[57567])[11:15]==[320,57663,0,1]
assert objvalues(objects[57558])[7]==0 and objvalues(objects[57567])[7]==0
assert all(re.search(r"\bS\s+UG\b",mobiles[v]) for v in (57527,57528))
for item,room,target,command in ((57568,57500,57570,15),(57502,57570,57500,320),(57512,57511,57515,7),(57582,57628,57692,7)):
    assert objvalues(objects[item])[11:14]==[target,command,-1]
    assert any(r["command"]=="O" and r["arguments"][1]==item and r["arguments"][3]==room for r in nexus["reset_commands"])
assert all(not any(r["command"] in "OGEP" and r["arguments"][1]==item for r in nexus["reset_commands"]) for item in (57500,57506,57508,57556,57573))
for companion in (57513,57515,57512,57510,57514):
    assert any(r["command"]=="M" and r["arguments"][1]==companion and r["arguments"][3]==57551 for r in nexus["reset_commands"])
    assert not int(mobiles[companion].split("~")[4].split()[0])&2
assert re.search(r"D2\s*[^~]*~\s*[^~]*~\s*0 0 57505",world[57551])
assert not any("companion" in entry["id"] for entry in nexus_stories.values())
assert "prayer book" in nexus_stories["gooran-parchment"]["summary"] and "pending builder" in nexus_stories["gooran-parchment"]["summary"]

# Crakkaro keeps mounted sources, exact badge kinds and guarded oversized work distinct.
crakkaro=inventory_module.area_evidence(ROOT,"crakkaro")
crakkaro_map=next(m for m in catalog["story_mappings"] if m["source_area"]=="crakkaro")
crakkaro_stories={s["id"]:s for s in crakkaro_map["stories"]}
assert (crakkaro_map["schema_version"],crakkaro_map["revision"],crakkaro_map["coverage"])==(3,1,"complete")
assert len(crakkaro_stories)==11 and len(crakkaro_map["contacts"])==29 and not crakkaro_map["exclusions"]
assert collections.Counter(s["category"] for s in crakkaro_stories.values())=={"story":6,"service":5}
assert collections.Counter(t["kind"] for s in crakkaro_stories.values() for t in s["steps"] if t.get("optional"))=={"carried_item":24,"completion":3}
assert (len(crakkaro["requests"]),len(crakkaro["dialogue"]),len(crakkaro["mobs"]),len(crakkaro["items"]),len(crakkaro["reset_commands"]))==(11,9,37,68,501)
assert collections.Counter(b["kind"] for b in inventory_module.native_blocks(ROOT) if b["source"]=="areas/qst/crakkaro.qst")=={"QA":11,"MA":9}
by_line={r["block"]["line"]:r["block"] for r in crakkaro["requests"]}
for line,required,reward,disappear in ((18,[87007],[("I",87000)],0),
    (59,list(range(87016,87021)),[("I",87021)],0),(73,[87021],[("I",87022)],0),
    (84,[87022],[("I",87023)],0),(95,[87023],[("I",87024)],0),
    (107,[87028,87029],[("I",87030)],0),(119,[87031,87032,87033],[("I",87034)],0),
    (140,[87035,87036],[("I",87037)],0),(152,[87039,87040,87041],[("I",87042)],0),
    (166,[87080]*17,[("C",100000)],0),(211,[87044,87045,87046,87047],[("I",87064)],1)):
    assert by_line[line]["give"]==[("I",v) for v in required] and by_line[line]["receive"]==reward
    assert by_line[line]["binding"]["completion_key"].endswith("disappear="+str(disappear))
bindings=[b for entry in crakkaro_stories.values() for b in entry["contracts"]]
assert len(bindings)==11 and {tuple(sorted(b.items())) for b in bindings}=={tuple(sorted(b["binding"].items())) for b in by_line.values()}
for later,earlier in (("burnhard-ogre-bracer","burnhard-ogre-shield"),("burnhard-ogre-ring","burnhard-ogre-bracer"),("burnhard-ogre-earring","burnhard-ogre-ring")):
    assert crakkaro_stories[later]["steps"][0]["optional"] and crakkaro_stories[later]["steps"][0]["contracts"]==crakkaro_stories[earlier]["contracts"]
badges=crakkaro_stories["woman-four-badges"]
assert [t["item_vnums"] for t in badges["steps"][:-1]]==[[87044],[87045],[87046],[87047]]
assert len({t["text"] for t in badges["steps"][:-1]})==4
assert [t["item_vnums"] for t in crakkaro_stories["burnhard-ogre-shield"]["steps"][:-1]]==[[v] for v in range(87016,87021)]
fur=crakkaro_stories["burnhard-seventeen-furs"]
assert fur["category"]=="service" and fur["steps"][0]["count"]==17>catalog_module.MAX_DURABLE_ITEM_OFFERINGS
assert "reward" in fur["summary"] or "hundred platinum" in fur["summary"]
assert "guarded" in fur["steps"][-1]["hint"] and "fourteen" in fur["summary"]
assert all(t["optional"] for entry in crakkaro_stories.values() for t in entry["steps"][:-1])
assert all(entry["steps"][-1]["contracts"]==entry["contracts"] for entry in crakkaro_stories.values())
contacts={c["mob_vnum"]:c for c in crakkaro_map["contacts"]}
for dialogue in crakkaro["dialogue"]:
    assert set(dialogue["body"][0].rstrip("~").split())<=set(contacts[dialogue["giver_vnum"]]["topics"])
assert not crakkaro["special_assignments"] and crakkaro["zone"]["reset_mode"]==0
assert collections.Counter(r["command"] for r in crakkaro["reset_commands"])=={"M":191,"D":94,"E":82,"G":64,"O":51,"F":12,"P":6,"R":1}
assert all(r["arguments"][-3:]==[0,0,0] for r in crakkaro["reset_commands"])
definitions={r["block"]["line"]:r["definition"] for r in crakkaro["requests"]}
assert sum(d["daily_eligible"] for d in definitions.values())==9
assert definitions[166]["daily_exclusion"]=="Unsupported durable offering" and definitions[211]["daily_exclusion"]=="Story-only quest"
units=[u for u in catalog_module.story_units(catalog) if u["zone_number"]==870]
assert len(units)==11 and sum(u["achievement"] for u in units)==6 and sum(u["daily_candidate"] for u in units)==5
assert sum(u["achievement"] for u in catalog_module.story_units(catalog))==1615
sources=collections.defaultdict(list);parent=None;room=None
for reset in crakkaro["reset_commands"]:
    c,v=reset["command"],reset["arguments"]
    if c in "MFR":parent,room=v[1],v[3] # R changes the actual E/G target to the mount.
    elif c in "GE":sources[v[1]].append((c,parent,room,v[2],v[3]))
    elif c=="P":sources[v[1]].append((c,v[3],None,v[2],None))
assert sources[87000]==[("E",87000,87007,1,35)] and sources[87078]==[("G",87000,87007,1,0)]
assert any(r["command"]=="R" and r["arguments"]==[1,87000,1,87007,100,0,0,0] for r in crakkaro["reset_commands"])
assert sources[87044]==[("E",87011,87297,2,24),("G",87025,87338,2,0)]
assert sources[87045]==[("E",87006,87219,2,24),("G",87025,87337,2,0)]
assert sources[87046]==[("G",87008,87040,2,0),("G",87026,87335,2,0)]
assert sources[87047]==[("P",87012,None,1,None)] and sources[87071]==[("P",87072,None,1,None)]
assert collections.Counter(v[1] for v in sources[87080])=={87034:9,87003:4,87035:3,87002:4}
assert all(v[3]==20 for v in sources[87080])
objects=dawndale_bodies("crakkaro","obj");world=dawndale_bodies("crakkaro","wld")
assert len(world)==372 and len(objects)==68
assert len({tuple(objects[v].split("~")[:3]) for v in (87044,87045,87046,87047,87071)})==1
assert objvalues(objects[87064])[0]==18 and objvalues(objects[87064])[12]==100
assert objects[87064].split("~")[0].strip()=="ice white"
assert objvalues(objects[87062])[0]==29 and objvalues(objects[87062])[11:15]==[346,87418,5,1]
assert all(any(r["command"]=="D" and r["arguments"][1:4]==[room,direction,6] for r in crakkaro["reset_commands"]) for room,direction in ((87418,5),(87428,4)))
assert re.search(r"D5\s*[^~]*~\s*[^~]*~\s*7 87064 87428",world[87418])
assert all(objvalues(objects[v])[12]==100 for v in (87010,87015,87064))
assert all(objvalues(objects[v])[12]==0 for v in (87001,87006))
assert all(not any(r["command"] in "OGEP" and r["arguments"][1]==v for r in crakkaro["reset_commands"]) for v in (87027,87075,87082))
assert re.search(r"D2\s*[^~]*~\s*[^~]*~\s*0 0 259959",world[87008])
assert "same-prototype" in (ROOT/"docs/design/zone-stories/CRAKKAROS_LIAR.md").read_text(encoding="utf8").lower()

# Rogue Plains groups alternatives without losing distinct proof or foreign ownership.
rogue=inventory_module.area_evidence(ROOT,"roguerai")
rogue_map=next(m for m in catalog["story_mappings"] if m["source_area"]=="roguerai")
rogue_stories={s["id"]:s for s in rogue_map["stories"]}
assert (rogue_map["schema_version"],rogue_map["revision"],rogue_map["coverage"])==(3,1,"complete")
assert len(rogue_stories)==7 and len(rogue_map["contacts"])==24 and not rogue_map["exclusions"]
assert all(s["category"]=="story" for s in rogue_stories.values())
assert collections.Counter(t["kind"] for entry in rogue_stories.values() for t in entry["steps"] if t.get("optional"))=={"carried_item":13,"completion":3}
assert (len(rogue["requests"]),len(rogue["dialogue"]),len(rogue["mobs"]),len(rogue["items"]),len(rogue["reset_commands"]))==(9,3,58,57,223)
assert collections.Counter(b["kind"] for b in inventory_module.native_blocks(ROOT) if b["source"]=="areas/qst/roguerai.qst")=={"Q":9,"M":3}
by_line={r["block"]["line"]:r["block"] for r in rogue["requests"]}
for line,required,reward,disappear in ((16,[75847],[75846],0),(20,[75852,75853],[75854],0),
    (29,[75837,75845,75843,75844],[75855,75855],1),(42,[75832],[75833],0),
    (50,[75850],[75852],0),(54,[75851],[75852],0),(60,[75850],[75853],0),
    (64,[75851],[75853],0),(80,[75833,75836],[75838],0)):
    assert by_line[line]["give"]==[("I",v) for v in required] and by_line[line]["receive"]==[("I",v) for v in reward]
    assert by_line[line]["binding"]["completion_key"].endswith("disappear="+str(disappear))
bindings=[b for entry in rogue_stories.values() for b in entry["contracts"]]
assert len(bindings)==9 and {tuple(sorted(b.items())) for b in bindings}=={tuple(sorted(b["binding"].items())) for b in by_line.values()}
cloud=rogue_stories["cloud-giant-promise"];storm=rogue_stories["storm-giant-promise"]
for entry,lines in ((cloud,(50,54)),(storm,(60,64))):
    assert entry["contracts"]==[by_line[n]["binding"] for n in lines]
    assert entry["steps"][0]["item_vnums"]==[75850,75851] and entry["steps"][0]["count"]==1
promises=rogue_stories["mediator-two-promises"]
assert promises["steps"][0]["contracts"]==cloud["contracts"] and promises["steps"][1]["contracts"]==storm["contracts"]
assert [t["item_vnums"] for t in promises["steps"][2:-1]]==[[75852],[75853]]
flesh=rogue_stories["orc-four-flesh-kinds"]
assert [t["item_vnums"] for t in flesh["steps"][:-1]]==[[75837],[75843],[75844],[75845]]
assert len({t["text"] for t in flesh["steps"][:-1]})==4
medal=rogue_stories["mediator-lost-medal"]
assert medal["steps"][0]["optional"] and medal["steps"][0]["item_vnums"]==[75829]
assert "lockpicking" in medal["summary"] and by_line[16]["give"]==[("I",75847)]
assert rogue_stories["reaper-bones-and-soul"]["steps"][0]["contracts"]==rogue_stories["sage-hiking-boots"]["contracts"]
assert all(t["optional"] for entry in rogue_stories.values() for t in entry["steps"][:-1])
assert all(entry["steps"][-1]["contracts"]==entry["contracts"] for entry in rogue_stories.values())
contacts={c["mob_vnum"]:c for c in rogue_map["contacts"]}
for dialogue in rogue["dialogue"]:
    assert set(dialogue["body"][0].rstrip("~").split())<=set(contacts[dialogue["giver_vnum"]]["topics"])
assert contacts[82569]["topics"]==["hi","hello","howdy","hey"]
foreign=[b for b in inventory_module.native_blocks(ROOT) if b["giver_vnum"]==82569 and sorted(b["give"])==[("I",75825),("I",75826),("I",75834)]]
assert len(foreign)==1 and foreign[0]["source"]=="areas/qst/mira.qst" and sorted(foreign[0]["receive"])==[("I",22631),("I",82553),("I",82554),("I",82555)]
assert all(b["giver_vnum"]!=82569 for b in bindings)
assert rogue["zone"]["reset_mode"]==1 and all(r["definition"]["daily_eligible"] for r in rogue["requests"])
assert rogue["special_assignments"]==[{"kind":"obj","vnum":75857,"function":"master_set","source":"src/specs/specs.assign.c","line":1345}]
units=[u for u in catalog_module.story_units(catalog) if u["zone_number"]==758]
assert len(units)==7 and all(u["achievement"] and u["daily_candidate"] for u in units)
assert collections.Counter(r["command"] for r in rogue["reset_commands"])=={"M":127,"E":28,"O":24,"G":19,"D":12,"F":7,"P":4,"R":2}
sources=collections.defaultdict(list);parent=None;room=None
for reset in rogue["reset_commands"]:
    c,v=reset["command"],reset["arguments"]
    if c in "MFR":parent,room=v[1],v[3]
    elif c in "GE":sources[v[1]].append((c,parent,room,v[2],v[3]))
    elif c=="P":sources[v[1]].append((c,v[3],None,v[2],None))
assert sources[75831]==[("E",75802,75909,1,8)] and sources[75828]==[("G",75802,75948,1,0)]
assert sources[75832]==[("E",75819,75873,1,8)] and sources[75836]==[("G",75831,75893,1,0)]
assert sources[75847]==[("P",75816,None,1,None)]
assert len(sources[75837])==5 and all(v[1]==75812 and v[3]==5 for v in sources[75837])
assert sources[75843]==[("G",75812,75803,1,0)]
assert sources[75844]==[("G",75812,75823,1,0)] and sources[75845]==[("G",75812,75823,1,0)]
assert sources[75850]==[("G",75836,75879,1,0)] and sources[75851]==[("G",75841,75909,1,0)]
assert sources[75857]==[("E",75858,75949,1,24)]
assert all(any(r["command"]=="M" and r["arguments"][1]==mob and r["arguments"][3]==load_room for r in rogue["reset_commands"]) for mob,load_room in ((75858,75949),(75857,75951),(75844,75949),(75851,75950)))
objects=dawndale_bodies("roguerai","obj");world=dawndale_bodies("roguerai","wld")
assert len(world)==158 and len(objects)==57
assert len({tuple(objects[v].split("~")[:3]) for v in (75837,75843,75844,75845)})==1
assert len({tuple(objects[v].split("~")[:3]) for v in (75852,75853)})==1
assert all(objvalues(objects[v])[0]==10 and objvalues(objects[v])[11:15]==[20,16,0,0] for v in (75837,75843,75844,75845))
assert all(objvalues(objects[v])[0]==10 and objvalues(objects[v])[11:15]==[10,127,0,0] for v in (75850,75851))
assert objvalues(objects[75838])[0]==3 and objvalues(objects[75838])[11:15]==[50,3,3,143]
assert objvalues(objects[75816])[11:15]==[1000,15,75829,1000] and objvalues(objects[75802])[11:15]==[1000,13,75830,1000]
assert all(not (objvalues(objects[v])[12]&16) for v in (75816,75802))
assert all(objvalues(objects[v])[0]==18 and objvalues(objects[v])[12]==0 for v in (75829,75830))
assert all(any(r["command"]=="D" and r["arguments"][1:4]==[load_room,direction,5] for r in rogue["reset_commands"]) for load_room,direction in ((75836,2),(75897,0),(75849,0),(75888,2),(75905,4),(75910,5)))
assert re.search(r"D0\s*[^~]*~\s*[^~]*~\s*0 0 170047",world[75895])
legacy=(ROOT/"src/specs/specs.set.c").read_text(encoding="utf8")
assert re.search(r"set_master_vnum\[\]\s*=\s*\{\s*22063,\s*22237,\s*22621,\s*45530,\s*45531,\s*82545,\s*75857,\s*0",legacy)
adapter=(ROOT/"src/specs/specs.set_equipment.c").read_text(encoding="utf8")
assert "22063, 22237, 22621, 45530, 45531, 75857, 82545, 82559" in adapter
assert "no actual native" in (ROOT/"docs/design/zone-stories/ROGUE_PLAINS.md").read_text(encoding="utf8")

# Desolate preserves nested supply, guarded fees, distinct proofs and foreign ownership.
desolate=inventory_module.area_evidence(ROOT,"desolate")
desolate_map=next(m for m in catalog["story_mappings"] if m["source_area"]=="desolate")
desolate_stories={s["id"]:s for s in desolate_map["stories"]}
assert (desolate_map["schema_version"],desolate_map["revision"],desolate_map["coverage"])==(3,1,"complete")
assert len(desolate_stories)==11 and len(desolate_map["contacts"])==27 and not desolate_map["exclusions"]
assert collections.Counter(s["category"] for s in desolate_stories.values())=={"story":9,"service":2}
assert collections.Counter(t["kind"] for entry in desolate_stories.values() for t in entry["steps"] if t.get("optional"))=={"carried_item":12,"completion":4}
assert (len(desolate["requests"]),len(desolate["dialogue"]),len(desolate["mobs"]),len(desolate["items"]),len(desolate["reset_commands"]))==(11,1,114,92,400)
assert collections.Counter(b["kind"] for b in inventory_module.native_blocks(ROOT) if b["source"]=="areas/qst/desolate.qst")=={"Q":11,"M":1}
by_line={r["block"]["line"]:r["block"] for r in desolate["requests"]}
for line,required,reward,disappear in ((2,[("I",22230)],[("C",50000)],0),
    (10,[("I",22215)],[("C",10000),("I",22220)],0),
    (29,[("I",22229)],[("I",22230)],0),(39,[("I",22214)],[("I",22269),("C",15000)],0),
    (49,[("I",22251)],[("I",22252)],1),(61,[("I",22284),("I",22283)],[("I",22288)],0),
    (70,[("I",22216)],[("I",22286)],0),
    (81,[("I",22220),("I",22231),("C",5000)],[("I",22251)],0),
    (92,[("C",10000)],[("I",22255)],1),(102,[("I",22211)],[("I",22250)],0),
    (111,[("I",82543)],[("I",22289),("C",10000)],0)):
    assert by_line[line]["give"]==required and by_line[line]["receive"]==reward
    assert by_line[line]["binding"]["completion_key"].endswith("disappear="+str(disappear))
bindings=[b for entry in desolate_stories.values() for b in entry["contracts"]]
assert len(bindings)==11 and {tuple(sorted(b.items())) for b in bindings}=={tuple(sorted(b["binding"].items())) for b in by_line.values()}
assert all(t["optional"] for entry in desolate_stories.values() for t in entry["steps"][:-1])
assert all(entry["steps"][-1]["contracts"]==entry["contracts"] for entry in desolate_stories.values())
assert desolate_stories["mercenary-tankard"]["category"]==desolate_stories["merchant-potion"]["category"]=="service"
assert desolate_stories["halfling-ale"]["steps"][0]["contracts"]==[by_line[29]["binding"]]
assert desolate_stories["scotson-wheel-repair"]["steps"][0]["contracts"]==[by_line[10]["binding"]]
assert desolate_stories["driver-repaired-wheel"]["steps"][0]["contracts"]==[by_line[81]["binding"]]
assert [t["item_vnums"] for t in desolate_stories["beregan-two-threats"]["steps"][:-1]]==[[22284],[22283]]
assert [t["item_vnums"] for t in desolate_stories["scotson-wheel-repair"]["steps"][1:-1]]==[[22220],[22231]]
assert "guarded" in desolate_stories["scotson-wheel-repair"]["summary"]
foreign=[b for b in inventory_module.native_blocks(ROOT) if b["source"]=="areas/qst/mira.qst" and b["line"]==126]
assert len(foreign)==1 and foreign[0]["give"]==foreign[0]["receive"]==[("I",82543)]
assert foreign[0]["giver_vnum"]==82569 and desolate_stories["seraphim-letter"]["steps"][0]["contracts"]==[foreign[0]["binding"]]
assert all(b["giver_vnum"]!=82569 for b in bindings)
contacts={c["mob_vnum"]:c for c in desolate_map["contacts"]}
assert contacts[22234]["topics"]==["ale"] and contacts[22219]["topics"]==["level"]
assert contacts[82569]["topics"]==["hi","hello","howdy","hey"]
assert desolate["special_assignments"]==[{"kind":"obj","vnum":22237,"function":"master_set","source":"src/specs/specs.assign.c","line":1340}]
assert desolate["zone"]["reset_mode"]==2
definitions={r["block"]["line"]:r["definition"] for r in desolate["requests"]}
assert sum(d["daily_eligible"] for d in definitions.values())==9
assert not definitions[81]["daily_eligible"] and definitions[81]["daily_exclusion"]=="Unsupported durable offering"
assert not definitions[92]["daily_eligible"] and definitions[92]["daily_exclusion"]=="No repeatable item offering"
units=[u for u in catalog_module.story_units(catalog) if u["zone_number"]==222]
assert len(units)==11 and sum(u["achievement"] for u in units)==9 and sum(u["daily_candidate"] for u in units)==8
assert collections.Counter(r["command"] for r in desolate["reset_commands"])=={"M":195,"E":61,"D":60,"O":45,"G":24,"F":9,"P":4,"R":2}
sources=collections.defaultdict(list);parent=None;room=None
for reset in desolate["reset_commands"]:
    c,v=reset["command"],reset["arguments"]
    if c in "MFR":parent,room=v[1],v[3]
    elif c in "GE":sources[v[1]].append((c,parent,room,v[2],v[3]))
    elif c=="P":sources[v[1]].append((c,v[3],None,v[2],None))
assert sources[22215]==[("P",22214,None,1,None)] and sources[22211]==[("P",22209,None,1,None)]
assert sources[22229]==[("E",22268,22319,1,18)]
assert sources[22216]==[("E",22239,22224,1,24)] and sources[22283]==[("E",22306,22365,1,24)]
assert sources[22284]==[("G",22296,22342,1,0)]
assert all(not any(r["command"] in "OGEP" and r["arguments"][1]==v for r in desolate["reset_commands"]) for v in (22220,22230,22251))
objects=dawndale_bodies("desolate","obj");world=dawndale_bodies("desolate","wld")
assert len(world)==168 and len(objects)==92
assert objvalues(objects[22214])[0]==15 and objvalues(objects[22214])[11:15]==[100,5,0,100]
assert objvalues(objects[22230])[0]==17 and objvalues(objects[22230])[11:15]==[10,10,1,0]
assert objvalues(objects[22228])[0]==objvalues(objects[22229])[0]==13
assert objvalues(objects[22208])[11:15]==[1000,15,22210,1000] and not (objvalues(objects[22208])[12]&16)
assert objvalues(objects[22291])[11:15]==[100,77302,0,0]
assert any(r["command"]=="O" and r["arguments"]==[0,22291,1,22200,20,0,0,0] for r in desolate["reset_commands"])
for control,room,direction,destination in ((22223,22309,1,22310),(22224,22310,1,22311),
    (22247,22311,2,22312),(22248,22312,3,22313),(22236,22313,3,22314),(22249,22314,0,22309)):
    assert objvalues(objects[control])[11:15]==[270,room,direction,0]
    assert re.search(rf"D{direction}\s*[^~]*~\s*[^~]*~\s*8 0 {destination}",world[room])
assert objvalues(objects[22276])[11:15]==[340,22263,5,0] and objvalues(objects[22275])[11:15]==[270,22264,4,0]
assert any(r["command"]=="D" and r["arguments"][1:4]==[22263,5,13] for r in desolate["reset_commands"])
assert any(r["command"]=="D" and r["arguments"][1:4]==[22264,4,13] for r in desolate["reset_commands"])
assert objvalues(objects[22204])[11:15]==[22309,7,-1,0] and objvalues(objects[22222])[11:15]==[22273,7,-1,0]
assert len([r for r in desolate["reset_commands"] if r["command"]=="M" and r["arguments"][1:4]==[22255,3,22315]])==3
_,all_mobs,all_items=inventory_module.inventory(ROOT)
assert all(v not in all_items for v in (6070,6109,6110))
assert all(str(v) in (ROOT/"areas/shp/desolate.shp").read_text(encoding="utf8") and v in sources for v in (6070,6109,6110))
assert "Shipped native repair" in (ROOT/"docs/design/zone-stories/DESOLATE.md").read_text(encoding="utf8")

# Rift Valley Jungle preserves count, role, token and native recipient semantics.
rift=inventory_module.area_evidence(ROOT,"rftjngle")
rift_map=next(m for m in catalog["story_mappings"] if m["source_area"]=="rftjngle")
rift_stories={s["id"]:s for s in rift_map["stories"]}
assert (rift_map["schema_version"],rift_map["revision"],rift_map["coverage"])==(3,1,"complete")
assert len(rift_stories)==24 and len(rift_map["contacts"])==49 and len(rift_map["exclusions"])==3
assert collections.Counter(s["category"] for s in rift_stories.values())=={"story":12,"service":12}
assert collections.Counter(t["kind"] for s in rift_stories.values() for t in s["steps"] if t.get("optional"))=={"carried_item":28,"completion":2}
assert (len(rift["requests"]),len(rift["dialogue"]),len(rift["mobs"]),len(rift["items"]),len(rift["reset_commands"]))==(28,37,220,220,828)
assert collections.Counter(b["kind"] for b in inventory_module.native_blocks(ROOT) if b["source"]=="areas/qst/rftjngle.qst")=={"Q":28,"M":37}
assert collections.Counter(r["command"] for r in rift["reset_commands"])=={"M":411,"E":139,"O":121,"G":96,"D":42,"F":11,"P":8}
by_line={r["block"]["line"]:r["block"] for r in rift["requests"]}
for line,giver,required,rewards,disappear in (
 (29,80070,[80067]*3+[80065],[80066],1),(65,80071,[80067],[80067],0),
 (89,80080,[80078]*2+[('C',10000)],[80079],0),(100,80080,[80077]*2+[('C',10000)],[80080],0),
 (113,80080,[80021]*2+[('C',10000)],[80023],0),(125,80080,[80024],[80024],0),
 (135,80134,[80134],[80135],0),(153,80136,[80137],[80138],1),(187,80147,[80144],[80145],0),
 (219,80148,[80077]*3+[80078]*3,[80139,80140,80141],0),(234,80148,[80024],[80025],0),
 (242,80149,[80142],[80143],0),(251,80149,[80058],[80059],0),(279,80155,[80154],[80155],1),
 (333,80156,[80157],[80196],1),(350,80157,[80157],[80196],1),
 (366,80158,[80172],[80173],1),(382,80159,[80170],[80171],1),(400,80160,[80166],[80167],1),(417,80161,[80168],[80169],1),
 (474,80175,[('C',5000),80060],[80061],0),(485,80175,[('C',5000),80062],[80063,80064],0),
 (495,80175,[('C',5000),80028],[80199],0),(507,80175,[('C',5000),80075],[80076],0),
 (615,80209,[80053]*5+[80054]*5+[80055]*5,[80057],0),
 (647,80210,[80149],[80177,('C',25000)],1),(699,80211,[80153],[80153],0),(707,80211,[80150],[80203],1)):
    goals=lambda xs:[v if isinstance(v,tuple) else ('I',v) for v in xs]
    assert by_line[line]["giver_vnum"]==giver and by_line[line]["give"]==goals(required) and by_line[line]["receive"]==goals(rewards),line
    assert by_line[line]["binding"]["completion_key"].endswith('disappear='+str(disappear))
bindings=[b for s in rift_map["stories"]+rift_map["exclusions"] for b in s["contracts"]]
assert len(bindings)==28 and {tuple(sorted(b.items())) for b in bindings}=={tuple(sorted(b["binding"].items())) for b in by_line.values()}
assert all(t["optional"] for s in rift_stories.values() for t in s["steps"][:-1])
assert all(s["steps"][-1]["contracts"]==s["contracts"] for s in rift_stories.values())
assert rift_stories["couatl-stolen-eggs"]["steps"][0]["contracts"]==[by_line[65]["binding"]]
assert [(t["item_vnums"],t["count"]) for t in rift_stories["couatl-stolen-eggs"]["steps"] if t["kind"]=="carried_item"]==[([80067],3),([80065],1)]
assert rift_stories["leather-white-tiger"]["steps"][0]["contracts"]==[by_line[125]["binding"]]
assert [(t["item_vnums"],t["count"]) for t in rift_stories["leather-mixed-skins"]["steps"][:-1]]==[([80077],3),([80078],3)]
assert [(t["item_vnums"],t["count"]) for t in rift_stories["weaver-quetzel-cloak"]["steps"][:-1]]==[([80053],5),([80054],5),([80055],5)]
assert rift_stories["dragon-crystal-staff"]["contracts"]==[by_line[333]["binding"],by_line[350]["binding"]]
assert [t["item_vnums"] for t in rift_stories["chief-summerstorm-proof"]["steps"][:-1]]==[[80154]]
assert [t["item_vnums"] for t in rift_stories["scout-chief-proof"]["steps"][:-1]]==[[80150]]
assert {tuple(sorted(b.items())) for e in rift_map["exclusions"] for b in e["contracts"]}=={tuple(sorted(by_line[n]["binding"].items())) for n in (65,125,699)}
defs={r["block"]["line"]:r["definition"] for r in rift["requests"]}
assert sum(d["daily_eligible"] for d in defs.values())==17
assert all(not defs[n]["daily_eligible"] and defs[n]["daily_exclusion"]=='Unsupported durable offering' for n in (89,100,113,474,485,495,507,615))
assert all(not defs[n]["daily_eligible"] and defs[n]["daily_exclusion"]=='Item exchange' for n in (65,125,699))
units=[u for u in catalog_module.story_units(catalog) if u["zone_number"]==800]
assert len(units)==24 and sum(u["achievement"] for u in units)==12 and sum(u["daily_candidate"] for u in units)==11
contacts={c["mob_vnum"]:c for c in rift_map["contacts"]}
assert {b["giver_vnum"] for b in rift["dialogue"]}<=contacts.keys()
assert set(contacts[80194]["topics"])=={'names','wall','quest','map','abandon','resign'}
assert contacts[80123]["topics"]==['quest','map','abandon','resign']
assert rift["special_assignments"]==[{"kind":"mob","vnum":80194,"function":"world_quest","source":"src/specs/specs.assign.c","line":762},{"kind":"mob","vnum":80123,"function":"world_quest","source":"src/specs/specs.assign.c","line":763}]
assert rift["zone"]["reset_mode"]==2
sources=collections.defaultdict(list);owner=None;room=None
for r in rift["reset_commands"]:
    c,v=r["command"],r["arguments"]
    if c in 'MF':owner=v[1];room=v[3]
    if c in 'GE':sources[v[1]].append((c,owner,room,v[4],v[3]))
assert sources[80065]==[('E',80072,80441,100,15)]
assert sources[80157]==[('G',80173,80323,100,0)]
assert sources[80149]==[('E',80154,80266,100,16)]
assert sources[80150]==[('E',80155,80266,30,16)] and sources[80153]==[('G',80155,80266,100,0)]
assert sources[80154]==[('G',80196,80352,40,0)]
assert sources[80053]==[('G',80053,80440,100,0)]*3 and sources[80054]==[('G',80053,80440,100,0)]*2
assert sources[80055]==[('G',80054,80440,100,0)]*3
assert all(sources[v]==[('G',80212,80444,100,0)] for v in (80166,80168,80170,80172))
assert not any(re.search(r'^([MF])\s+\d+\s+80157\b',(ROOT/'areas/zon'/str(row['zone']['source_area']+'.zon')).read_text(encoding='utf8'),re.M) for row in rows)
objects=dawndale_bodies('rftjngle','obj');world=dawndale_bodies('rftjngle','wld');mob_bodies=dawndale_bodies('rftjngle','mob')
assert len(world)==470
teachers={v for v,b in mob_bodies.items() if int(b.split('~')[4].split()[0])&32768}
assert teachers=={80170,80175,80176,80186,80187,80195,80196,80197,80198,80199,80205,80210}
assert all('level' in contacts[v]['topics'] for v in teachers) and 80178 not in teachers
assert not int(mob_bodies[80212].split('~')[4].split()[0])&(2|64)
for obj,origin,target,command in ((80158,80061,80162,5),(80159,80162,80061,6),(80160,80072,80164,5),(80161,80164,80072,6),(80162,80060,80161,7),(80163,80161,80060,7),(80164,80071,80163,5),(80165,80163,80071,6)):
    assert objvalues(objects[obj])[0]==25 and objvalues(objects[obj])[11:15]==[target,command,-1,0]
    assert any(r['command']=='O' and r['arguments'][1:5]==[obj,1,origin,100] for r in rift['reset_commands'])
assert objvalues(objects[80206])[0]==13 and objvalues(objects[80200])[0]==13 and objvalues(objects[80200])[7]==0
assert all(objvalues(objects[v])[6]&4096 and 'secret' in next(t['hint'] for s in rift_stories.values() for t in s['steps'] if t['kind']=='carried_item' and t['item_vnums']==[v]) for v in (80067,80065,80028,80144,80157,80166,80168,80170,80172))
egg_loads=[r['arguments'][3:5] for r in rift['reset_commands'] if r['command']=='O' and r['arguments'][1]==80067]
assert egg_loads==[[80130,25],[80145,70],[80152,50],[80193,25],[80200,99]]
for item,source,roll in ((80134,80046,100),(80137,80220,100),(80142,80023,30),(80144,80333,100),(80028,80201,10)):
    assert any(r['command']=='O' and r['arguments'][1:5]==[item,1,source,roll] for r in rift['reset_commands'])
assert re.search(r'\bD2\s+[^~]*~[^~]*~\s+0 0 228371\b',world[80460],re.S)
assert re.search(r'\bD1\s+[^~]*~[^~]*~\s+0 0 568516\b',world[80469],re.S)

# Transparent Tower: common-token acceptance is not a mandatory three-source campaign.
tower=inventory_module.area_evidence(ROOT,'trnsptow')
tower_map=next(m for m in catalog['story_mappings'] if m['source_area']=='trnsptow')
assert tower_map['schema_version']==3 and tower_map['revision']==1 and tower_map['coverage']=='complete'
assert len(tower_map['contacts'])==14 and len(tower_map['stories'])==4 and not tower_map['exclusions']
assert all(s['category']=='story' for s in tower_map['stories'])
assert collections.Counter(t['kind'] for s in tower_map['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':8,'completion':3}
assert collections.Counter(b['kind'] for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/trnsptow.qst')=={'Q':4,'M':26}
by_line={r['block']['line']:r['block'] for r in tower['requests']}
assert set(by_line)=={68,104,195,281}
for line,giver in ((68,16203),(104,16206),(281,16234)):
    b=by_line[line]
    assert b['giver_vnum']==giver and b['give']==[('I',16241)] and b['receive']==[('I',16241),('I',16264)]
    assert b['binding']['completion_key'].endswith('disappear=1')
assert by_line[195]['giver_vnum']==16207 and collections.Counter(by_line[195]['give'])=={('I',16264):3,('I',16241):1}
assert by_line[195]['receive']==[('I',16258)] and by_line[195]['binding']['completion_key'].endswith('disappear=0')
tower_stories={s['id']:s for s in tower_map['stories']}
final=tower_stories['librarian-mist-key']
assert [t['contracts'] for t in final['steps'][:3]]==[[by_line[n]['binding']] for n in (68,104,281)]
assert [(t['item_vnums'],t['count']) for t in final['steps'][3:-1]]==[([16241],1),([16264],3)]
assert [t['item_vnums'] for t in tower_stories['gullivier-scepter-token']['steps'][:-1]]==[[16271],[16272],[16246],[16241]]
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in tower_map['stories'])
defs={r['block']['line']:r['definition'] for r in tower['requests']}
assert defs[195]['daily_eligible'] and all(not defs[n]['daily_eligible'] and defs[n]['daily_exclusion']=='Item exchange' for n in (68,104,281))
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==162]
assert len(units)==4 and all(u['achievement'] for u in units) and sum(u['daily_candidate'] for u in units)==1
contacts={c['mob_vnum']:c for c in tower_map['contacts']}
assert {b['giver_vnum'] for b in tower['dialogue']}<=contacts.keys()
assert {'four','three'}<=set(contacts[16203]['topics']) and {'book','books'}<=set(contacts[16207]['topics'])
assert tower['zone']['reset_mode']==1
assert collections.Counter(r['command'] for r in tower['reset_commands'])=={'M':73,'E':52,'O':38,'D':32,'P':5,'G':5,'F':3}
assert len(tower['mobs'])==40 and len(tower['items'])==75
objects=dawndale_bodies('trnsptow','obj');rooms=dawndale_bodies('trnsptow','wld');mobs=dawndale_bodies('trnsptow','mob')
assert set(rooms)==set(range(16200,16300)) and not (ROOT/'areas/shp/trnsptow.shp').exists()
assert all(not int(b.split('~')[4].split()[0])&32768 for b in mobs.values())
assert objvalues(objects[16239])[0]==15 and objvalues(objects[16239])[11:15]==[100,29,16246,100]
assert re.search(r'\bT\s+516\s+4\s+3\s+100',objects[16239])
assert all(objvalues(objects[v])[0]==18 and objvalues(objects[v])[12]==100 for v in (16271,16272,16246,16258))
assert any(r['command']=='P' and r['arguments'][1:5]==[16241,1,16239,100] for r in tower['reset_commands'])
sources=collections.defaultdict(list);owner=None;room=None
for r in tower['reset_commands']:
    c,v=r['command'],r['arguments']
    if c in 'MF':owner=v[1];room=v[3]
    if c=='G':sources[v[1]].append((owner,room,v[2],v[4]))
assert sources[16271]==[(16237,16241,1,100)] and sources[16272]==[(16238,16244,1,100)]
assert sources[16246]==[(16205,16253,1,100)] and not sources[16241]
for v,target,cmd in ((16201,16228,139),(16202,16256,3),(16203,16240,63),(16204,16271,4),(16205,16228,32),(16206,16228,2),(16207,16263,4),(16208,16228,53),(16209,16212,4),(16257,16228,4),(16260,16298,4)):
    assert objvalues(objects[v])[0]==25 and objvalues(objects[v])[11:15]==[target,cmd,-1,0]
for source,direction,key,target in ((16243,1,16271,16244),(16252,0,16272,16253),(16241,0,16258,16248),(16248,1,-2,16249),(16278,3,-2,16297),(16229,2,0,16212)):
    assert re.search(r'\bD'+str(direction)+r'\s+[^~]*~[^~]*~\s+\d+ '+str(key)+' '+str(target)+r'\b',rooms[source],re.S)
assert {(a['kind'],a['vnum'],a['function']) for a in tower['special_assignments']}=={('mob',16205,'transp_tow_acerlade'),('obj',16263,'artifact_stone'),('obj',16268,'artifact_stone'),('obj',16262,'trans_tower_shadow_globe'),('obj',16242,'zion_light_dark')}
proc=(ROOT/'src/specs/specs.trnsptow.c').read_text()
acer=proc[proc.index('int transp_tow_acerlade'):proc.index('#ifdef THARKUN_ARTIS')]
assert 'return TRUE' not in acer and 'recharm_ch' in acer
assert '#define THARKUN_ARTIS 1' in (ROOT/'src/core/config.h').read_text()
assert all(not any(r['command'] in 'OGEP' and r['arguments'][1]==v for r in tower['reset_commands']) for v in (16200,16262,16263,16268,16274))

# Tempest Court: exact foreign bundles, independent fragments and equivalent family return.
tempest=inventory_module.area_evidence(ROOT,'airp')
tempest_map=next(m for m in catalog['story_mappings'] if m['source_area']=='airp')
assert tempest_map['schema_version']==3 and tempest_map['revision']==1 and tempest_map['coverage']=='complete'
assert len(tempest_map['contacts'])==21 and len(tempest_map['stories'])==7 and not tempest_map['exclusions']
assert all(s['category']=='story' for s in tempest_map['stories'])
assert collections.Counter(t['kind'] for s in tempest_map['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':25,'completion':3}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/airp.qst']
assert collections.Counter(b['kind'] for b in raw)=={'Q':8,'M':17,'MA':1}
by_line={r['block']['line']:r['block'] for r in tempest['requests']}
for line,giver,inputs,reward,disappear in (
    (14,131615,[131627],131628,0),(261,131651,[131627],131628,0),
    (98,131616,[55553,8735,70976,88304,97066,131617,138268,138515,131647],131650,1),
    (152,131618,[131609,131610,131611],131612,0),(165,131621,[131615],131636,0),
    (186,131630,[131642,131643,131644,131645,131646],131647,0),
    (205,131635,[131605],131627,1),(240,131637,[131647,96000,96012,96055],131648,0)):
    b=by_line[line]
    assert b['giver_vnum']==giver and b['give']==[('I',v) for v in inputs] and b['receive']==[('I',reward)]
    assert b['binding']['completion_key'].endswith('disappear='+str(disappear))
stories={s['id']:s for s in tempest_map['stories']}
assert stories['alhajib-medallion-eyepiece']['contracts']==[by_line[n]['binding'] for n in (14,261)]
assert stories['alhajib-medallion-eyepiece']['steps'][0]['contracts']==[by_line[205]['binding']]
for id,line in (('north-wind-cloudseeker',98),('fearfrost-thrym-hammer',240)):
    entry=stories[id]
    assert entry['steps'][0]['contracts']==[by_line[186]['binding']] and entry['steps'][0]['optional']
    assert {t['item_vnums'][0] for t in entry['steps'][1:-1]}=={v for k,v in by_line[line]['give']}
assert [t['item_vnums'] for t in stories['chan-maelstrom-fragment']['steps'][:-1]]==[[131612],[131642],[131643],[131644],[131645],[131646]]
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in tempest_map['stories'])
defs={r['block']['line']:r['definition'] for r in tempest['requests']}
assert sum(d['daily_eligible'] for d in defs.values())==6
assert all(not defs[n]['daily_eligible'] and defs[n]['daily_exclusion']=='Story-only quest' for n in (98,205))
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==1316]
assert len(units)==7 and all(u['achievement'] for u in units) and sum(u['daily_candidate'] for u in units)==5
contacts={c['mob_vnum']:c for c in tempest_map['contacts']}
for b in raw:
    if b['kind'] in ('M','MA'):
        safe={a for a in b['body'][0].split('~')[0].split() if re.fullmatch('[a-z0-9_-]+',a)}
        assert safe<=set(contacts[b['giver_vnum']]['topics'])
assert "si'ciltron" in contacts[131616]['description'] and {'galzron','ecthius'}<=set(contacts[131618]['topics'])
assert tempest['zone']['reset_mode']==0
assert collections.Counter(r['command'] for r in tempest['reset_commands'])=={'M':192,'E':39,'D':30,'O':17,'G':15,'F':8,'R':6}
assert len(tempest['mobs'])==52 and len(tempest['items'])==63
objects=dawndale_bodies('airp','obj');rooms=dawndale_bodies('airp','wld');mobs=dawndale_bodies('airp','mob')
assert set(rooms)==set(range(131600,131800)) and not (ROOT/'areas/shp/airp.shp').exists()
assert all(not int(b.split('~')[4].split()[0])&32768 for b in mobs.values())
assert objvalues(objects[131650])[0]==9
assert objvalues(objects[131605])[0]==18 and objvalues(objects[131605])[12]==0
assert objvalues(objects[131612])[0]==18 and objvalues(objects[131612])[12]==0
assert objvalues(objects[131608])[0]==18 and objvalues(objects[131608])[12]==2
for v,target in ((131600,131610),(131601,131605),(131603,131736),(131604,131720),(131606,131751),(131607,131730),(131613,131768),(131614,131647)):
    assert objvalues(objects[v])[0]==25 and objvalues(objects[v])[11:15]==[target,7,-1,0]
assert objvalues(objects[131641])[11:15]==[1823,0,0,0]
assert re.search(r'\bT\s+3576\s+5\s+500\s+31',objects[131602]) and not 3576&1
assert sum(r['command']=='O' and r['arguments'][1]==131602 for r in tempest['reset_commands'])==8
for room,direction,key,target in ((131600,5,0,24422),(131740,0,131605,131741),(131754,1,131608,131760),(131768,5,131612,131769)):
    assert re.search(r'\bD'+str(direction)+r'\s+[^~]*~[^~]*~\s+\d+ '+str(key)+' '+str(target)+r'\b',rooms[room],re.S)
sources=collections.defaultdict(list);owner=None;room=None;followers=0
for r in tempest['reset_commands']:
    c,v=r['command'],r['arguments']
    if c in 'MFR':
        owner,room=v[1],v[3]
        followers=followers+1 if c=='F' else 0
    if c in 'GE':sources[v[1]].append((c,owner,room,v[2],v[3],v[4],followers))
assert sources[131608]==[('G',131602,131759,1,0,100,2)]
for item,mob in ((131642,131623),(131643,131625),(131644,131624),(131645,131627),(131646,131626)):
    assert sources[item]==[('G' if item==131645 else 'E',mob,131633,1,0 if item==131645 else 18,100,0)]
assert {s[2] for s in sources[131658] if s[1]==131605} >= {131602,131624,131641,131669,131680,131723}
assert sources[131609][0][1:3]==(131636,131767) and sources[131610][0][1:3]==(131631,131748)
assert sources[131611][0][1:3]==(131611,131628) and sources[131615][0][1:3]==(131619,131613)
assert not any(r['command'] in 'MFR' and r['arguments'][1]==131615 for r in tempest['reset_commands'])
assert all(not any(r['command'] in 'OGEP' and r['arguments'][1]==v for r in tempest['reset_commands']) for v in (131651,131652,131654,131655,131656,131659,131660,131661,131662))
assert not re.search(r'\bD\d',rooms[131797]+rooms[131799])
assert {(a['kind'],a['vnum'],a['function']) for a in tempest['special_assignments']}=={('obj',131616,'dagger_of_wind')}
blocks=inventory_module.native_blocks(ROOT)
wisp=[b for b in blocks if b['giver_vnum']==55151 and b['give']==[('I',93011)]]
assert len(wisp)==1 and set(wisp[0]['receive'])=={('I',v) for v in range(55550,55555)}
cloak=[b for b in blocks if b['giver_vnum']==138545 and b['give']==[('I',138514)]]
assert len(cloak)==1 and cloak[0]['receive']==[('I',138515)] and cloak[0]['binding']['completion_key'].endswith('disappear=1')

# Caverns of Armageddon: loaded proof, salvage, independent bounties and consumed amulets.
hunt=inventory_module.area_evidence(ROOT,'hunt')
hunt_map=next(m for m in catalog['story_mappings'] if m['source_area']=='hunt')
assert hunt_map['schema_version']==3 and hunt_map['revision']==1 and hunt_map['coverage']=='complete'
assert len(hunt_map['contacts'])==40 and len(hunt_map['stories'])==18 and not hunt_map['exclusions']
assert all(s['category']=='story' and len(s['contracts'])==1 for s in hunt_map['stories'])
assert collections.Counter(t['kind'] for s in hunt_map['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':26,'completion':1}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/hunt.qst']
assert collections.Counter(b['kind'] for b in raw)=={'Q':18,'M':15}
by_line={r['block']['line']:r['block'] for r in hunt['requests']}
for line,giver,inputs,rewards,disappear in (
    (10,13300,[13354],[13333],0),(28,13301,[13355],[13334],0),
    (47,13302,[13347],[13317],0),(67,13303,[13346],[13319],0),
    (84,13304,[76613],[13368],0),(98,13305,[13343],[13351],0),
    (114,13308,[13345],[13344],0),(133,13309,[13357],[13320,13321,13321,13321,13321],0),
    (158,13310,[13342],[13326],0),(174,13311,[13352],[13309],0),
    (194,13312,[13358],[13337],0),(208,13313,[13318,13303],[13361],0),
    (223,13317,[13356],[13327],0),(242,13318,[13353],[13332],0),
    (262,13321,[13348],[13316],0),(282,13322,[13362],[13315],0),
    (294,13351,[13359,13330],[13329],0),
    (310,13364,[13335,13349,13350,13336],[13303,13318],1)):
    b=by_line[line]
    assert b['giver_vnum']==giver and b['give']==[('I',v) for v in inputs] and b['receive']==[('I',v) for v in rewards]
    assert b['binding']['completion_key'].endswith('disappear='+str(disappear))
stories={s['id']:s for s in hunt_map['stories']}
assert {tuple(s['contracts'][0].items()) for s in hunt_map['stories']}=={tuple(b['binding'].items()) for b in by_line.values()}
for s in hunt_map['stories']:
    assert s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional')
assert [t['item_vnums'][0] for t in stories['maveriss-maverick']['steps'][:-1]]==[13314,13313,13312,13352]
assert [t['item_vnums'][0] for t in stories['prisoner-two-company-tags']['steps'][:-1]]==[13359,13330]
assert {t['item_vnums'][0] for t in stories['blicatch-four-creature-parts']['steps'][:-1]}=={13335,13336,13349,13350}
queen=stories['dragon-queen-cosmos-amulet']
assert queen['steps'][0]['contracts']==[by_line[310]['binding']] and queen['steps'][0]['optional']
assert [t['item_vnums'][0] for t in queen['steps'][1:-1]]==[13303,13318]
assert hunt['zone']['reset_mode']==1 and all(r['definition']['daily_eligible'] for r in hunt['requests'])
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==133]
assert len(units)==18 and all(u['achievement'] and u['daily_candidate'] for u in units)
contacts={c['mob_vnum']:c for c in hunt_map['contacts']}
assert {c['mob_vnum'] for c in hunt_map['contacts'] if c['topics']}=={b['giver_vnum'] for b in raw if b['kind']=='M'}
assert all(set(contacts[b['giver_vnum']]['topics'])=={'orc','orcs'} for b in raw if b['kind']=='M')
assert not contacts[13337]['topics'] and not any(b['giver_vnum']==13337 for b in raw)
assert 'no active declared reset placement' in contacts[13309]['description']
assert 'no active declared reset placement' in stories['roland-maximas']['summary']
assert collections.Counter(r['command'] for r in hunt['reset_commands'])=={'M':270,'D':36,'O':26,'E':23,'G':23,'R':7,'P':5,'F':3}
assert len(hunt['mobs'])==94 and len(hunt['items'])==69 and not hunt['special_assignments']
objects=dawndale_bodies('hunt','obj');rooms=dawndale_bodies('hunt','wld');mobs=dawndale_bodies('hunt','mob')
assert set(rooms)==set(range(13300,13449)) and not (ROOT/'areas/shp/hunt.shp').exists()
assert all(not int(b.split('~')[4].split()[0])&32768 for b in mobs.values())
sources=collections.defaultdict(list);owner=None;room=None
for r in hunt['reset_commands']:
    c,v=r['command'],r['arguments']
    if c in 'MFR':owner,room=v[1],v[3]
    if c in 'GE':sources[v[1]].append((c,owner,room,v[2],v[3],v[4]))
for item,mob,where in ((13342,13361,13377),(13348,13368,13379),(13354,13349,13380),
    (13362,13386,13381),(13343,13374,13382),(13356,13362,13393),(13345,13347,13396),
    (13355,13342,13412),(13358,13357,13414),(13346,13365,13418),(13347,13328,13426),
    (13352,13326,13431),(13357,13378,13437),(13353,13382,13438),(13335,13343,13446),
    (13336,13355,13415),(13349,13344,13416),(13350,13345,13414),
    (13314,13370,13375),(13312,13390,13406),(13311,13327,13431),(13366,13327,13431)):
    assert sources[item]==[('G',mob,where,1,0,100)]
loaded={r['arguments'][1] for r in hunt['reset_commands'] if r['command'] in 'MFR'}
assert set(mobs)-loaded=={13309,13360}
for area in [r['zone']['source_area'] for r in inventory_module.inventory(ROOT)[0]]:
    assert not re.search(r'^[MFR]\s+\d+\s+(?:13309|13360)\s', (ROOT/f'areas/zon/{area}.zon').read_text(encoding='utf8',errors='replace'),re.M)
for key,breakage in ((13314,0),(13313,0),(13312,0),(13311,100)):
    assert objvalues(objects[key])[0]==18 and objvalues(objects[key])[12]==breakage
for v in (13323,13325,13338,13340):assert objvalues(objects[v])[0]==15 and objvalues(objects[v])[12]==5
assert objvalues(objects[13306])[0]==15 and objvalues(objects[13306])[12:14]==[29,13311]
assert objvalues(objects[13364])[0]==8 and objvalues(objects[13365])[0]==8
assert re.search(r'\bT\s+2\s+0\s+1\s+50\b',objects[13335])
for portal,target in ((13302,13350),(13339,13429),(13363,82627)):
    assert objvalues(objects[portal])[0]==25 and objvalues(objects[portal])[11:15]==[target,7,-1,0]
for room,direction,flags,key,target in ((13368,2,7,13311,13371),(13375,5,2,13314,13376),
    (13396,1,3,13313,13397),(13419,1,3,13312,13420),(13418,5,0,13312,13419),
    (13431,4,4,0,13439),(13439,5,8,0,13431),(13445,5,0,0,13355),
    (13307,3,0,0,649693),(13307,2,0,0,650096)):
    assert re.search(r'\bD'+str(direction)+r'\s+[^~]*~[^~]*~\s*'+str(flags)+' '+str(key)+' '+str(target)+r'\b',rooms[room],re.S)
assert [(r['arguments'][1:5]) for r in hunt['reset_commands'] if r['command']=='P']==[[13308,1,13306,100],[13359,2,13340,100],[13313,1,13325,100],[13330,1,13323,100],[13359,2,13338,100]]
assert [(r['arguments'][3]) for r in hunt['reset_commands'] if r['command']=='O' and r['arguments'][1]==13364]==[13442,13442,13444,13444]
blocks=inventory_module.native_blocks(ROOT)
foreign=[b for b in blocks if b['kind'] in ('Q','QA') and b['source']!='areas/qst/hunt.qst' and any(('I',v) in b['give'] for v in (13364,13365,13366))]
assert {b['giver_vnum'] for b in foreign}=={82515,82516,82522,82537,82543,76243}
assert next(b for b in foreign if b['giver_vnum']==82537)['receive']==[('I',13318)]
assert next(b for b in foreign if b['giver_vnum']==82543)['receive']==[('C',20000)]
assert next(b for b in foreign if b['giver_vnum']==76243)['receive']==[('I',76243)]
triggers=ROOT/'areas/world.trg'
if triggers.exists():
    assert not any(13300<=int(v)<=13448 for v in re.findall(r'^#(\d+)\s+[MOR]\b',triggers.read_text(encoding='utf8'),re.M))


# Tribal Forest: independent exchanges, supplied preparation and a same-kind refusal.
tribal=inventory_module.area_evidence(ROOT,'tribal')
tribal_map=next(m for m in catalog['story_mappings'] if m['source_area']=='tribal')
assert (tribal_map['schema_version'],tribal_map['revision'],tribal_map['coverage'])==(3,1,'complete')
assert len(tribal_map['stories'])==9 and len(tribal_map['exclusions'])==1 and len(tribal_map['contacts'])==17
assert all(s['category']=='story' and len(s['contracts'])==1 for s in tribal_map['stories'])
assert collections.Counter(t['kind'] for s in tribal_map['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':20,'completion':1}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/tribal.qst']
assert collections.Counter(b['kind'] for b in raw)=={'Q':10,'M':13,'MA':2}
by_line={r['block']['line']:r['block'] for r in tribal['requests']}
for line,giver,inputs,rewards,disappear in (
    (11,42200,[42265],[('I',42200),('E',80000)],0),
    (18,42200,[42202],[('I',42204),('E',33000)],0),
    (25,42200,[42201],[('I',42201)],0),
    (46,42209,[42201],[('I',42265),('E',50000)],0),
    (77,42219,[42222,42219,42220,42242],[('I',42268)],1),
    (119,42230,[42241],[('I',42260),('E',250000)],0),
    (180,42234,[42244],[('I',42224)],0),
    (191,42234,[42221,42227,42212,42263,42267],[('I',42262)],0),
    (266,42256,[42293,42294,42295,42296,42297],[('I',42285),('I',42291)],1),
    (308,42259,[42302],[('E',250000),('I',42301)],0)):
    b=by_line[line]
    assert b['giver_vnum']==giver and b['give']==[('I',v) for v in inputs] and b['receive']==rewards
    assert b['binding']['completion_key'].endswith('disappear='+str(disappear))
stories={s['id']:s for s in tribal_map['stories']}
refs=[s['contracts'][0] for s in tribal_map['stories']]+tribal_map['exclusions'][0]['contracts']
assert {tuple(ref.items()) for ref in refs}=={tuple(b['binding'].items()) for b in by_line.values()}
assert tribal_map['exclusions'][0]['contracts']==[by_line[25]['binding']]
assert 'same prototype kind' in tribal_map['exclusions'][0]['reason']
for s in tribal_map['stories']:
    assert s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional')
grain=stories['bluebird-grain-staff']
assert grain['steps'][0]['contracts']==[by_line[46]['binding']] and grain['steps'][0]['optional']
assert grain['steps'][1]['item_vnums']==[42265]
assert [t['item_vnums'][0] for t in stories['wife-four-part-escape']['steps'][:-1]]==[42222,42219,42220,42242]
assert [t['item_vnums'][0] for t in stories['shaman-five-ingredient-crystal']['steps'][:-1]]==[42221,42227,42212,42263,42267]
assert [t['item_vnums'][0] for t in stories['xazapath-five-body-parts']['steps'][:-1]]==[42293,42294,42295,42296,42297]
assert not any(t['kind']=='completion' for t in stories['shaman-five-ingredient-crystal']['steps'][:-1])
assert tribal['zone']['reset_mode']==1
definitions={r['block']['line']:r['definition'] for r in tribal['requests']}
assert sum(d['daily_eligible'] for d in definitions.values())==9
assert not definitions[25]['daily_eligible'] and definitions[25]['daily_exclusion']=='Item exchange'
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==422]
assert len(units)==9 and all(u['achievement'] and u['daily_candidate'] for u in units)
contacts={c['mob_vnum']:c for c in tribal_map['contacts']}
assert {v for v,c in contacts.items() if c['topics']}=={b['giver_vnum'] for b in raw if b['kind'] in ('M','MA')}
for v,c in contacts.items():
    aliases={t for b in raw if b['kind'] in ('M','MA') and b['giver_vnum']==v for t in b['body'][0].rstrip('~').split()}
    assert set(c['topics'])==aliases
assert {'gretings','husban'}<=set(contacts[42219]['topics'])
assert len(tribal['mobs'])==67 and len(tribal['items'])==103
assert len(tribal['reset_commands'])==377 and collections.Counter(r['command'] for r in tribal['reset_commands'])=={'M':133,'E':87,'O':49,'D':38,'G':30,'P':25,'F':15}
assert {(r['kind'],r['vnum'],r['function']) for r in tribal['special_assignments']}=={('obj',42235,'amethyst_orb')}
objects=dawndale_bodies('tribal','obj');rooms=dawndale_bodies('tribal','wld');mobs=dawndale_bodies('tribal','mob')
assert set(rooms)==set(range(42200,42375))
assert [v for v,b in mobs.items() if int(b.split('~')[4].split()[0])&32768]==[42209]
assert int(mobs[42256].split('~')[4].split()[0])&4 # Scavenger, not a guaranteed staging arrival.
sources=collections.defaultdict(list);owner=None;room=None
for r in tribal['reset_commands']:
    c,v=r['command'],r['arguments']
    if c in 'MFR':owner,room=v[1],v[3]
    if c in 'GE':sources[v[1]].append((c,owner,room,v[2],v[3],v[4]))
for item,mob,where,chance in ((42201,42201,42291,100),(42202,42202,42278,100),
    (42221,42216,42220,100),(42222,42218,42248,100),(42241,42217,42219,100),
    (42244,42215,42206,100),(42263,42232,42254,100),(42267,42237,42267,75),(42302,42237,42267,75)):
    assert sources[item]==[('G',mob,where,1,0,chance)]
assert sources[42227]==[('E',42231,42269,1,19,75)]
reset_lines=(ROOT/'areas/zon/tribal.zon').read_text(encoding='utf8').splitlines()
trees=[r for r in tribal['reset_commands'] if r['command']=='F' and r['arguments'][1]==42237]
assert len(trees)==3 and '42267' in reset_lines[trees[0]['line']] and '42302' in reset_lines[trees[1]['line']]
assert [(r['arguments'][3]) for r in tribal['reset_commands'] if r['command']=='M' and r['arguments'][1]==42200]==[42289]
assert [(r['arguments'][3]) for r in tribal['reset_commands'] if r['command']=='M' and r['arguments'][1]==42256]==[42317]
for item,where in ((42293,42339),(42294,42352),(42295,42335),(42296,42349),(42297,42346)):
    assert [(r['arguments'][2:5]) for r in tribal['reset_commands'] if r['command']=='O' and r['arguments'][1]==item]==[[1,where,100]]
    assert objvalues(objects[item])[6]&4096
assert objvalues(objects[42297])[0]==8 and all(objvalues(objects[v])[0]==13 for v in (42293,42294,42295,42296))
for key,breakage in ((42222,50),(42233,30),(42210,100)):
    assert objvalues(objects[key])[0]==18 and objvalues(objects[key])[12]==breakage
assert objvalues(objects[42213])[0]==15 and objvalues(objects[42213])[12]==0
assert re.search(r'\bT\s+2\s+9\s+1\s+50\b',objects[42212]) and re.search(r'\bT\s+2\s+10\s+1\s+50\b',objects[42290])
for item,command,where,direction in ((42271,270,42239,3),(42283,270,42285,0),(42284,270,42322,9),(42282,282,42315,5),(42287,270,42342,2)):
    assert objvalues(objects[item])[0]==29 and objvalues(objects[item])[11:15]==[command,where,direction,1]
for portal,target,command in ((42281,42315,7),(42234,42297,7),(42253,42295,7),(42250,42370,7),(42264,42281,139)):
    assert objvalues(objects[portal])[0]==25 and objvalues(objects[portal])[11:15]==[target,command,-1,0]
for room,direction,flags,key,target in ((42248,3,2,42222,42249),(42249,1,2,42222,42248),(42254,5,6,42233,42295),
    (42293,1,2,42210,42282),(42315,5,12,0,42316),(42322,9,12,0,42319),(42342,2,13,0,42353)):
    assert re.search(r'\bD'+str(direction)+r'\s+[^~]*~[^~]*~\s*'+str(flags)+' '+str(key)+' '+str(target)+r'\b',rooms[room],re.S)
shop=(ROOT/'areas/shp/tribal.shp').read_text(encoding='utf8')
assert shop.startswith('#42235~\nN\n42231\n42246\n42248\n42247\n0\n')
assert '\n42235\n0\n42259\n0\n28\n0\n28\n' in shop
assert objvalues(objects[42235])[7]==0 and objvalues(objects[42235])[11]==0
triggers=ROOT/'areas/world.trg'
assert not triggers.exists() or not re.search(r'^#(?:422\d\d|423[0-7]\d)\b',triggers.read_text(encoding='utf8',errors='replace'),re.M)

# Ironstar: linked supplied outcomes, guarded fees, exact ownership and effective access.
ironstar=inventory_module.area_evidence(ROOT,'lornecro')
ironstar_map=next(m for m in catalog['story_mappings'] if m['source_area']=='lornecro')
assert (ironstar_map['schema_version'],ironstar_map['revision'],ironstar_map['coverage'])==(3,1,'complete')
assert len(ironstar_map['stories'])==7 and not ironstar_map['exclusions'] and len(ironstar_map['contacts'])==15
assert collections.Counter(s['category'] for s in ironstar_map['stories'])=={'story':4,'service':3}
assert collections.Counter(t['kind'] for s in ironstar_map['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':12,'completion':2}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/lornecro.qst']
assert collections.Counter(b['kind'] for b in raw)=={'Q':7,'M':15}
by_line={r['block']['line']:r['block'] for r in ironstar['requests']}
for line,giver,inputs,reward,retire in (
    (16,138908,[('I',138913)],138914,0),
    (92,138921,[('I',138942)],138943,0),
    (107,138921,[('I',138909),('C',100000)],138945,0),
    (114,138921,[('I',138943),('C',500000)],138958,0),
    (124,138921,[('I',138944),('I',138952),('I',138965),('C',1000000)],138963,0),
    (134,138921,[('I',138962),('I',138952),('I',138965),('C',1000000)],138964,0),
    (228,138927,[('I',138914)],138950,1)):
    b=by_line[line]
    assert b['giver_vnum']==giver and b['give']==inputs and b['receive']==[('I',reward)]
    assert b['binding']['completion_key'].endswith('disappear='+str(retire))
stories={s['id']:s for s in ironstar_map['stories']}
assert {tuple(s['contracts'][0].items()) for s in stories.values()}=={tuple(b['binding'].items()) for b in by_line.values()}
for s in stories.values():
    assert len(s['contracts'])==1 and s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional')
assert stories['robert-soulcatcher']['steps'][0]['contracts']==[by_line[16]['binding']]
assert stories['dralor-vault-key']['steps'][0]['contracts']==[by_line[92]['binding']]
for id,inputs in (
    ('larra-wedding-ring',[138911,138913]),('robert-soulcatcher',[138914]),
    ('haldron-crown',[138942]),('dralor-vault-key',[138943]),('dralor-dragonbone-mail',[138909]),
    ('dralor-demonic-dagger',[138944,138952,138965]),('dralor-demonic-hammer',[138962,138952,138965])):
    assert [t['item_vnums'][0] for t in stories[id]['steps'] if t['kind']=='carried_item']==inputs
assert all(not any(t['kind']=='completion' for t in stories[id]['steps'][:-1]) for id in ('dralor-dragonbone-mail','dralor-demonic-dagger','dralor-demonic-hammer'))
definitions={r['block']['line']:r['definition'] for r in ironstar['requests']}
assert {line for line,d in definitions.items() if d['daily_eligible']}=={16,92,228}
assert all(definitions[line]['daily_exclusion']=='Unsupported durable offering' for line in (107,114,124,134))
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==1389]
assert len(units)==7 and sum(u['achievement'] for u in units)==4 and sum(u['daily_candidate'] for u in units)==3
contacts={c['mob_vnum']:c for c in ironstar_map['contacts']}
assert {v for v,c in contacts.items() if c['topics']}=={138908,138921,138927}
for v,c in contacts.items():
    assert set(c['topics'])=={t for b in raw if b['kind']=='M' and b['giver_vnum']==v for t in b['body'][0].rstrip('~').split()}
assert {'key','vault','mithril','maltheas','dagger','warhammer','mold'}<=set(contacts[138921]['topics'])
assert ironstar['zone']==dict(zone_number=1389,name='The Ancient Halls of Ironstar',source_area='lornecro',last_vnum=138999,reset_mode=1,discoverable=True,first_vnum=138667)
assert len(ironstar['mobs'])==34 and len(ironstar['items'])==66 and len(ironstar['reset_commands'])==198
assert collections.Counter(r['command'] for r in ironstar['reset_commands'])=={'M':51,'D':50,'E':27,'O':26,'F':21,'P':16,'G':7}
assert not ironstar['special_assignments'] and not (ROOT/'areas/shp/lornecro.shp').exists()
objects=dawndale_bodies('lornecro','obj');rooms=dawndale_bodies('lornecro','wld');mobs=dawndale_bodies('lornecro','mob')
assert set(rooms)==set(range(138900,139000))
assert not any(int(b.split('~')[4].split()[0])&32768 for b in mobs.values())
sources=collections.defaultdict(list);owner=None;room=None
for r in ironstar['reset_commands']:
    c,v=r['command'],r['arguments']
    if c in 'MF':owner,room=v[1],v[3]
    if c in 'GE':sources[v[1]].append((c,owner,room,v[2],v[3],v[4]))
for item,mob,where in ((138909,138923,138977),(138949,138926,138984),(138952,138928,138976),(138965,138906,138998)):
    assert sources[item]==[('G',mob,where,1,0,100)]
assert sources[138908]==[('E',138906,138998,1,16,100)]
assert sources[138960]==[('E',138932,138900,1,18,100)]
commands=ironstar['reset_commands']
dragons=[n for n,r in enumerate(commands) if r['command']=='M' and r['arguments'][1]==138926]
assert len(dragons)==4 and commands[dragons[2]+1]['command']=='G' and commands[dragons[2]+1]['arguments'][1]==138949
for mob,chance in ((138927,75),(138932,80)):
    assert [r['arguments'][2:5] for r in commands if r['command']=='M' and r['arguments'][1]==mob]==[[1,138900,chance]]
assert not any(r['command'] in 'MF' and r['arguments'][3] in (138991,138992) for r in commands)
for item,where in ((138911,138941),(138910,138937),(138934,138975),(138956,138987),(138944,138994),(138962,138994)):
    assert [r['arguments'][2:5] for r in commands if r['command']=='O' and r['arguments'][1]==item]==[[1,where,100]]
for item,parent in ((138913,138910),(138942,138934),(138955,138956),(138954,138953)):
    assert [r['arguments'][2:5] for r in commands if r['command']=='P' and r['arguments'][1]==item]==[[1,parent,100]]
assert objvalues(objects[138910])[0]==15 and objvalues(objects[138910])[12:14]==[29,138911]
assert objvalues(objects[138922])[0]==15 and objvalues(objects[138922])[12:14]==[2,138921]
assert objvalues(objects[138954])[0]==33 and 'Datherlion' in objects[138954]
assert objvalues(objects[138908])[0]==5 and objvalues(objects[138965])[0]==5
assert 'hammer' in objects[138944].lower() and 'dragon' in objects[138952].lower()
assert not any(objvalues(b)[0] in (25,29) for b in objects.values())
for room,direction,kind,key,target in ((138971,5,15,138944,138975),(138975,4,3,138944,138971),
    (138983,2,3,138949,138985),(138985,2,3,138955,138993),(138993,2,3,138958,138994),
    (138988,2,3,-2,138989),(138989,0,3,-2,138988)):
    assert re.search(r'\bD'+str(direction)+r'\s+[^~]*~[^~]*~\s*'+str(kind)+' '+str(key)+' '+str(target)+r'\b',rooms[room],re.S)
for room,direction,state in ((138971,5,0),(138975,4,0),(138988,2,0),(138989,0,0),
    (138983,2,2),(138985,0,1),(138985,2,2),(138993,0,1),(138993,2,2),(138994,0,1),(138980,2,5)):
    assert [r['arguments'][3] for r in commands if r['command']=='D' and r['arguments'][1:3]==[room,direction]]==[state]
assert all('grate adamant datherlion' in rooms[v] for v in (138988,138989))
assert re.search(r'\bD1\s+[^~]*~[^~]*~\s*0 0 138913\b',rooms[138911],re.S) and re.search(r'\bC\s+20 2\b',rooms[138911])
assert not re.search(r'\bD2\s',rooms[138911])
assert re.search(r'\bD0\s+[^~]*~[^~]*~\s*0 0 512296\b',rooms[138978],re.S)
triggers=ROOT/'areas/world.trg'
assert not triggers.exists() or not re.search(r'^#1389\d\d\b',triggers.read_text(encoding='utf8',errors='replace'),re.M)

# Brass: collectible currency kinds, duplicate rewards, competing proof and unfinished branches.
brass=inventory_module.area_evidence(ROOT,'brass')
brass_map=next(m for m in catalog['story_mappings'] if m['source_area']=='brass')
assert (brass_map['schema_version'],brass_map['revision'],brass_map['coverage'])==(3,1,'complete')
assert len(brass_map['stories'])==5 and len(brass_map['exclusions'])==2 and len(brass_map['contacts'])==25
assert collections.Counter(s['category'] for s in brass_map['stories'])=={'story':4,'service':1}
assert collections.Counter(t['kind'] for s in brass_map['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':18}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/brass.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':12,'MA':1,'Q':6,'QA':1}
by_line={r['block']['line']:r['block'] for r in brass['requests']}
for line,giver,inputs,outputs,retire in (
    (22,139083,[('I',139128)],[('C',1000000)],0),
    (33,139110,[('I',139018)],[],0),
    (89,139119,[('I',139028),('I',139031),('I',139026),('I',139033)],[('I',139070)],0),
    (118,139121,[('I',139127),('C',7500000)],[('I',139127),('C',7500000)],0),
    (131,139121,[('C',7500000),('I',139127),('I',139142)],[('I',139137)],0),
    (159,139124,[('I',139011),('I',139016),('I',139017)],[('I',139018),('I',139018)],0),
    (182,139132,[('I',139144),('I',139016),('I',139017),('I',139139),('I',139140),('I',139141)],[('I',139143)],1)):
    b=by_line[line]
    assert b['giver_vnum']==giver and b['give']==inputs and b['receive']==outputs
    assert b['binding']['completion_key'].endswith('disappear='+str(retire))
stories={s['id']:s for s in brass_map['stories']}
assert {tuple(c.items()) for s in brass_map['stories']+brass_map['exclusions'] for c in s['contracts']}=={tuple(b['binding'].items()) for b in by_line.values()}
assert {tuple(e['contracts'][0].items()) for e in brass_map['exclusions']}=={tuple(by_line[line]['binding'].items()) for line in (33,118)}
for id,inputs in (
    ('herl-exotic-blood',[139128]),
    ('tax-palace-key',[139028,139031,139026,139033]),
    ('yodono-three-heads',[139070,139011,139016,139017]),
    ('spy-six-heads',[139070,139144,139016,139017,139139,139140,139141]),
    ('armorer-pyrohydra-bracer',[139127,139142])):
    s=stories[id]
    assert [t['item_vnums'][0] for t in s['steps'] if t['kind']=='carried_item']==inputs
    assert len(s['contracts'])==1 and s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional')
    assert not any(t['kind']=='completion' for t in s['steps'][:-1])
assert set(by_line[159]['give']) & set(by_line[182]['give'])=={('I',139016),('I',139017)}
defs={r['block']['line']:r['definition'] for r in brass['requests']}
assert {line for line,d in defs.items() if d['daily_eligible']}=={22,33,89,159,182}
assert defs[131]['daily_exclusion']=='Unsupported durable offering'
assert defs[118]['daily_exclusion']=='Item exchange'
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==1390]
assert len(units)==5 and sum(u['achievement'] for u in units)==4 and sum(u['daily_candidate'] for u in units)==4
contacts={c['mob_vnum']:c for c in brass_map['contacts']}
assert {v for v,c in contacts.items() if c['topics']}=={139072,139083,139110,139119,139121,139124,139132}
for v,c in contacts.items():
    assert set(c['topics'])=={t for b in brass['dialogue'] if b['giver_vnum']==v for t in b['body'][0].rstrip('~').split()}
assert 'qc_action' not in contacts[139124]['topics']
assert len(brass['mobs'])==147 and len(brass['items'])==170 and len(brass['reset_commands'])==779
assert collections.Counter(r['command'] for r in brass['reset_commands'])=={'M':408,'G':133,'D':112,'E':78,'F':25,'O':13,'R':9,'P':1}
objects=dawndale_bodies('brass','obj');rooms=dawndale_bodies('brass','wld');mobs=dawndale_bodies('brass','mob')
assert set(rooms)==set(range(139000,139358))-{139248}
assert not any(int(b.split('~')[4].split()[0])&32768 for b in mobs.values())
assert len(re.findall(r'^#\d+~$',(ROOT/'areas/shp/brass.shp').read_text(encoding='utf8'),re.M))==18
for v in (139028,139031,139033,139127,139128,139142):assert objvalues(objects[v])[0]==8
assert objvalues(objects[139018])[0]==10 and objvalues(objects[139018])[11:15]==[55,267,0,0]
assert objvalues(objects[139070])[0]==18 and objvalues(objects[139070])[12]==100
commands=brass['reset_commands'];sources=collections.defaultdict(list);owner=None;where=None
for r in commands:
    c,v=r['command'],r['arguments']
    if c in 'MF':owner,where=v[1],v[3]
    if c in 'GE':sources[v[1]].append((c,owner,where,v[2],v[3],v[4]))
for item,mob,room in ((139128,139144,139091),(139028,139038,139178),(139031,139045,139246),(139011,139118,139041),
    (139016,139009,139299),(139017,139000,139292),(139144,139010,139286),(139139,139012,139330),
    (139140,139089,139336),(139141,139092,139344),(139142,139102,139247)):
    assert sources[item]==[('G',mob,room,1,0,100)]
assert sources[139127]==[('G',139145,139085,1,0,80)]
assert sources[139026]==[('E',139036,139192,1,18,100)]
assert sources[139025]==[('E',139036,139192,1,18,100)]
assert [r['arguments'][2:5] for r in commands if r['command']=='O' and r['arguments'][1]==139033]==[[1,139241,100]]
for mob,room,chance in ((139119,139020,100),(139121,139024,100),(139124,139217,100),(139132,139238,10),(139102,139247,60)):
    assert [r['arguments'][2:5] for r in commands if r['command']=='M' and r['arguments'][1]==mob]==[[1,room,chance]]
assert not any(r['command'] in 'MFR' and r['arguments'][1]==139110 for r in commands)
assert not int(mobs[139102].split('~')[4].split()[0])&2
assert not any(re.search(r'\bD\d+\s+[^~]*~[^~]*~\s*-?\d+ -?\d+ 139247\b',b,re.S) for b in rooms.values())
for room,direction,state in ((139000,0,9),(139001,2,1),(139140,0,2),(139249,2,1),
    (139283,0,2),(139290,2,1),(139284,1,2),(139285,3,1),(139291,1,2),(139292,3,2),
    (139311,0,2),(139357,2,1),(139357,0,2),(139312,2,1),(139312,0,2),(139313,2,2)):
    assert [r['arguments'][3] for r in commands if r['command']=='D' and r['arguments'][1:3]==[room,direction]]==[state]
for room,direction,kind,key,target in ((139140,0,3,139070,139249),(139249,2,1,0,139140),
    (139284,1,2,139130,139285),(139291,1,2,139129,139292),(139311,0,3,139021,139357),
    (139357,0,3,139022,139312),(139312,0,3,139023,139313)):
    assert re.search(r'\bD'+str(direction)+r'\s+[^~]*~[^~]*~\s*'+str(kind)+' '+str(key)+' '+str(target)+r'\b',rooms[room],re.S)
assert objvalues(objects[139134])[12:14]==[29,139135]
assert [r['arguments'][2:5] for r in commands if r['command']=='P' and r['arguments'][1]==67244]==[[1,139134,100]]
assert 'qc_unblock 139000 north' in (ROOT/'areas/qst/plane_fire_one.qst').read_text(encoding='utf8')
assert re.search(r'\bD2\s+[^~]*~[^~]*~\s*0 0 25455\b',rooms[139000],re.S)


# Tower: grouped alternatives, exact quantities and physical/credit-owner boundaries.
lortower=inventory_module.area_evidence(ROOT,'lortower')
lortower_map=next(m for m in catalog['story_mappings'] if m['source_area']=='lortower')
assert (lortower_map['schema_version'],lortower_map['revision'],lortower_map['coverage'])==(3,1,'complete')
assert len(lortower_map['stories'])==6 and not lortower_map['exclusions'] and len(lortower_map['contacts'])==27
assert all(s['category']=='story' for s in lortower_map['stories'])
assert collections.Counter(t['kind'] for s in lortower_map['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':10,'completion':2}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/lortower.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':20,'Q':12}
blocks={b['line']:b for b in raw if b['kind']=='Q'}
for line,giver,inputs,outputs,retire in (
    (12,134010,[('I',134039)],[('E',500000)],0),
    (33,134026,[('I',134035)],[('I',134034),('E',70000)],0),
    (61,134029,[('C',100000)],[('I',134019)],1),
    (71,134029,[('I',134034)],[('I',134019)],0),
    (119,134053,[('I',134004)]*5+[('I',134030)],[('I',134028)],0),
    (143,134054,[('I',134026)],[('I',134030)],1),
    (155,134074,[('I',134111),('I',134112),('I',134113)],[('I',134117)],0),
    (201,134146,[('I',134105)],[('I',134106)],1),
    (218,134150,[('I',134006)],[('E',100000)],0),
    (266,134162,[('I',v) for v in range(134131,134136)],[('I',134125)],1),
    (292,134167,[('I',134144)],[('I',134145)],0),
    (304,134169,[('I',134048)],[('I',134144)],0)):
    b=blocks[line]
    assert b['giver_vnum']==giver and b['give']==inputs and b['receive']==outputs
    assert b['binding']['completion_key'].endswith('disappear='+str(retire))
owned={r['block']['line'] for r in lortower['requests']}
assert owned=={12,33,61,71,119,143,155}
assert {tuple(c.items()) for s in lortower_map['stories'] for c in s['contracts']}=={tuple(blocks[n]['binding'].items()) for n in owned}
for giver in (134146,134150,134162,134167,134169):
    defs=[d for d in catalog['definitions'] if d['giver_vnum']==giver]
    assert len(defs)==1 and defs[0]['source_area']=='brad' and defs[0]['zone_number']==1350
stories={s['id']:s for s in lortower_map['stories']}
assert stories['captain-key']['contracts']==[blocks[71]['binding'],blocks[61]['binding']]
assert 'currently unavailable' in stories['captain-key']['summary']
assert stories['dorthan-dubneth-shield']['steps'][0]['contracts']==[blocks[143]['binding']]
assert stories['captain-key']['steps'][0]['contracts']==[blocks[33]['binding']]
assert [(t['item_vnums'],t['count']) for t in stories['dorthan-dubneth-shield']['steps'] if t['kind']=='carried_item']==[([134030],1),([134004],5)]
assert [t['item_vnums'] for t in stories['three-keys-stasis']['steps'][:-1]]==[[134111],[134112],[134113]]
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in stories.values())
defs={r['block']['line']:r['definition'] for r in lortower['requests']}
assert {n for n,d in defs.items() if d['daily_eligible']}=={12,33,71,119,155}
assert defs[61]['daily_exclusion']==defs[143]['daily_exclusion']=='Story-only quest'
assert all(d['repeatable'] and d['daily_eligible'] for d in catalog['definitions'] if d['giver_vnum'] in (134146,134162))
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==1340]
assert len(units)==6 and sum(u['achievement'] for u in units)==6 and sum(u['daily_candidate'] for u in units)==5
contacts={c['mob_vnum']:c for c in lortower_map['contacts']}
for v,c in contacts.items():
    assert set(c['topics'])=={t for b in raw if b['kind']=='M' and b['giver_vnum']==v for t in b['body'][0].rstrip('~').split()}
assert len(lortower['mobs'])==170 and len(lortower['items'])==146 and len(lortower['reset_commands'])==580
assert not lortower['special_assignments']
assert collections.Counter(r['command'] for r in lortower['reset_commands'])=={'M':176,'E':116,'D':114,'O':74,'F':50,'G':32,'P':18}
objects=dawndale_bodies('lortower','obj');rooms=dawndale_bodies('lortower','wld');mobs=dawndale_bodies('lortower','mob')
assert set(rooms)==set(range(134000,134142))
assert not any(int(b.split('~')[4].split()[0])&32768 for b in mobs.values())
assert objvalues(objects[134083])[0]==15 and objvalues(objects[134083])[12:14]==[5,0]
assert objvalues(objects[134117])[0]==12 and objvalues(objects[134144])[0]==13
assert objvalues(objects[134050])[16:19]==[378,50,25] and 'Testing to see' in objects[134050]
commands=lortower['reset_commands'];sources=collections.defaultdict(list);owner=None;where=None
for r in commands:
    c,v=r['command'],r['arguments']
    if c in 'MF':owner,where=v[1],v[3]
    if c in 'GE':sources[v[1]].append((c,owner,where,v[2],v[3],v[4]))
for item,mob,room in ((134006,134004,134010),(134026,134055,134028),(134035,134066,134063),
    (134048,134081,134039),(134111,134104,134072),(134113,134141,134116),
    (134131,134040,134018),(134132,134062,134029),(134133,134081,134039),
    (134134,134091,134046),(134135,134133,134122)):
    assert sources[item]==[('G',mob,room,1,0,100)]
assert sources[134105]==[('E',134148,134114,1,18,100)]
assert [r['arguments'][2:5] for r in commands if r['command']=='P' and r['arguments'][1]==134039]==[[1,134037,100]]
assert [r['arguments'][2:5] for r in commands if r['command']=='P' and r['arguments'][1]==134112]==[[1,134083,100]]
assert [r['arguments'][2:5] for r in commands if r['command']=='P' and r['arguments'][1]==134050]==[[1,134047,30]]
for giver,room in ((134146,134112),(134150,134127),(134162,134138),(134167,134140),(134169,134042)):
    assert [r['arguments'][3] for r in commands if r['command']=='M' and r['arguments'][1]==giver]==[room]
for room,direction,state in ((134008,2,1),(134010,0,1),(134049,0,2),(134046,2,2)):
    assert [r['arguments'][3] for r in commands if r['command']=='D' and r['arguments'][1:3]==[room,direction]]==[state]
assert 'closed but unlocked' in stories['katalia-bindings']['steps'][0]['hint']
assert 'reset mode two' in lortower_map['orientation'][-1]
assert not re.search(r'\bD\d+',rooms[134120]) and re.search(r'\bD5\s+[^~]*~[^~]*~\s+0 0 134141\b',rooms[134140],re.S)
assert re.search(r'\bD1\s+[^~]*~[^~]*~\s+0 0 1883\b',rooms[134000],re.S)
for v,d,word in ((134034,0,'sargon'),(134040,1,'sargon'),(134041,3,'sargon'),(134073,4,'thothrontithos')):
    assert re.search(r'\bD'+str(d)+r'\s+[^~]*~[^~]*\b'+word+r'~\s+3 -2 \d+',rooms[v],re.S)

# Mushroom: physical availability, same-name identities and real money-item admission.
mushroom=inventory_module.area_evidence(ROOT,'mushroom_caverns')
mushroom_map=next(m for m in catalog['story_mappings'] if m['source_area']=='mushroom_caverns')
assert (mushroom_map['schema_version'],mushroom_map['revision'],mushroom_map['coverage'])==(3,1,'complete')
assert len(mushroom_map['stories'])==3 and len(mushroom_map['contacts'])==10 and not mushroom_map['exclusions']
assert all(s['category']=='story' for s in mushroom_map['stories'])
assert collections.Counter(t['kind'] for s in mushroom_map['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':3,'completion':2}
givers={24021,24022,24023};all_blocks=inventory_module.native_blocks(ROOT)
raw=[b for b in all_blocks if b['giver_vnum'] in givers]
assert collections.Counter(b['kind'] for b in raw)=={'M':13,'Q':2,'QA':1}
assert len(mushroom['dialogue'])==11
by_line={b['line']:b for b in raw if 'binding' in b}
for line,giver,inputs,outputs in (
    (25,24021,[('I',1515)],[('I',24013)]),
    (89,24022,[('I',4660)],[('I',24014),('C',150000)]),
    (127,24023,[('I',24013),('I',24014)],[('I',24016),('I',24018),('I',24017),('E',35000)])):
    b=by_line[line]
    assert b['giver_vnum']==giver and b['give']==inputs and b['receive']==outputs and b['disappear']
    definition=next(r['definition'] for r in mushroom['requests'] if r['block']['line']==line)
    assert definition['zone_number']==241 and definition['source_area']=='mushroom_caverns' and definition['repeatable'] and definition['daily_eligible']
stories={s['id']:s for s in mushroom_map['stories']}
assert {tuple(c.items()) for s in stories.values() for c in s['contracts']}=={tuple(b['binding'].items()) for b in by_line.values()}
assert stories['haz-goblet-half']['steps']==[stories['haz-goblet-half']['steps'][-1]]
assert stories['ozman-bracelet-half']['steps'][0]['item_vnums']==[4660]
finale=stories['kryz-two-halves']
assert [s['contracts'] for s in finale['steps'][:2]]==[[by_line[25]['binding']],[by_line[89]['binding']]]
assert [(s['item_vnums'],s['count']) for s in finale['steps'][2:4]]==[([24013],1),([24014],1)]
assert 'currently unavailable under active accounting' in finale['summary']
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in stories.values())
contacts={c['mob_vnum']:c for c in mushroom_map['contacts']}
for giver in givers:
    assert set(contacts[giver]['topics'])=={t for b in raw if b['kind']=='M' and b['giver_vnum']==giver for t in b['body'][0].rstrip('~').split() if re.fullmatch('[a-z0-9_-]{1,64}',t)}
assert sum(len(c['topics']) for c in contacts.values())==22
assert {'zochra','slin','ilith','olipth','sazarn'}<=set(contacts[24022]['topics'])
assert not {'haz\'on\'wyz','sa\'zarn'}&set(contacts[24022]['topics'])
assert len(mushroom['mobs'])==8 and len(mushroom['items'])==9 and len(mushroom['reset_commands'])==54
assert collections.Counter(r['command'] for r in mushroom['reset_commands'])=={'M':39,'O':8,'D':4,'E':2,'F':1}
objects=dawndale_bodies('mushroom_caverns','obj');rooms=dawndale_bodies('mushroom_caverns','wld')
assert set(rooms)==set(range(24101,24238))-{24201,24202,24203,24204,24224}
assert sum(not b.split('~')[1].strip() for b in rooms.values())==17
assert not mushroom['special_assignments'] and not (ROOT/'areas/shp/mushroom_caverns.shp').exists()
for v,target,flags in ((24103,24102,0),(24104,24108,8192),(24108,24229,2),(24109,24218,8194)):
    values=objvalues(objects[v]);assert values[0]==25 and values[6]==flags and values[11:14]==[target,7,-1]
for v in (24101,24102,24105,24107):assert objvalues(objects[v])[0]==13
assert all(int(b.split('~')[2].split()[2])!=39 for b in rooms.values())
for v,direction,state in ((24144,2,9),(24168,0,9),(24148,1,1),(24151,3,1)):
    assert [r['arguments'][3] for r in mushroom['reset_commands'] if r['command']=='D' and r['arguments'][1:3]==[v,direction]]==[state]
physical={r['arguments'][1] for r in mushroom['reset_commands'] if r['command'] in 'MF'}
assert len(physical)==19 and not any(b['giver_vnum'] in physical for b in all_blocks)
assert not physical&givers and 24103 not in physical
shared_mobs=dawndale_bodies('mobs_underdark','mob')
assert int(shared_mobs[24021].split('~')[4].split()[0])&32768
assert 'teacher flag' in contacts[24021]['description']
shared=dawndale_bodies('mobs_underdark','obj')
assert shared[24013].split('~')[1]==shared[24014].split('~')[1]
assert objvalues(shared[24013])[0]==8 and objvalues(shared[24014])[0]==18 and objvalues(shared[24014])[11:15]==[0,100,0,0]
policy=(ROOT/'src/item/item_command_policy.c').read_text(encoding='utf8')
assert 'object->type == ITEM_MONEY' in policy[:policy.index('bool item_command_object_is_takeable')]
quest=(ROOT/'src/world/quest.c').read_text(encoding='utf8')
assert '!item_command_uses_durable_ownership(selected)' in quest and 'submit_durable_quest_offering' in quest
modern=dawndale_bodies('underdark','obj')
assert objvalues(modern[700000])[0]==objvalues(modern[700001])[0]==9
assert inventory_module.plain(shared[24016].split('~')[1])==inventory_module.plain(modern[700005].split('~')[1])
modern_blocks=[b for b in all_blocks if b['giver_vnum']==700036 and 'binding' in b]
assert [(b['give'],b['receive'],b['disappear']) for b in modern_blocks]==[([('I',700008)],[('C',100000)],False),([('I',700000),('I',700001)],[('I',700005),('E',250000)],False)]
assert any(b['giver_vnum']==55151 and ('I',700005) in b['give'] for b in all_blocks)
assert not any(b['giver_vnum'] not in givers and any(('I',v) in b['give'] for v in (1515,4660,24013,24014,24016,24017,24018)) for b in all_blocks if 'binding' in b)
placements=collections.defaultdict(list);active_rooms=set();active_objects=set();bracelet_sources=[]
for zone in catalog_module.zone_registry(ROOT):
    area=zone['source_area'];p=ROOT/f'areas/zon/{area}.zon';parent=None;room=None
    if p.exists():
        for line in p.read_text(encoding='utf8',errors='replace').splitlines():
            match=re.match(r'^([MFOGEP])\s+((?:-?\d+\s*)+)',line)
            if not match:continue
            command=match[1];values=list(map(int,match[2].split()))
            if command in 'MF':
                parent,room=values[1],values[3]
                if parent in givers|{24103}:placements[parent].append((area,command,values))
            if command in 'GE' and values[1]==4660:bracelet_sources.append((area,command,parent,room,values))
    for kind,found in (('wld',active_rooms),('obj',active_objects)):
        p=ROOT/f'areas/{kind}/{area}.{kind}'
        if p.exists():found.update(int(n) for n in re.findall(r'^#(\d+)\s*$',p.read_text(encoding='utf8',errors='replace'),re.M))
assert placements[24021]==[('mobs_underdark','M',[0,24021,1,24015,100,0,0,0])]
assert not placements[24022] and not placements[24023] and not placements[24103]
assert 1515 not in active_objects and not {329339,329042,332030,331125,332935}&active_rooms
assert bracelet_sources==[('underworld','E',4680,4525,[1,4660,1,15,100,0,0,0])]


# Smoke: exact deliveries, non-credit services, rare follower supply and access repairs.
smoke=inventory_module.area_evidence(ROOT,'smoke')
smoke_map=next(m for m in catalog['story_mappings'] if m['source_area']=='smoke')
assert (smoke_map['schema_version'],smoke_map['revision'],smoke_map['coverage'])==(3,1,'complete')
assert len(smoke_map['stories'])==6 and len(smoke_map['contacts'])==12 and not smoke_map['exclusions']
assert collections.Counter(s['category'] for s in smoke_map['stories'])=={'story':2,'service':4}
assert collections.Counter(t['kind'] for s in smoke_map['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':8,'completion':4}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/smoke.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':5,'MA':3,'QA':4,'Q':2}
assert len(smoke['dialogue'])==8
by_line={b['line']:b for b in raw if 'binding' in b}
for line,giver,inputs,outputs,disappear in (
    (28,139811,[('I',139808)],[('I',139809),('I',139818)],True),
    (71,139813,[('I',139813)],[('I',139812)],True),
    (105,139823,[('I',139829),('I',139814)],[('I',139831)],False),
    (120,139823,[('I',139822)],[('I',139823)],False),
    (128,139823,[('I',139823)],[('I',139822)],False),
    (136,139823,[('I',139825),('I',139822)],[('I',139832)],False)):
    b=by_line[line]
    assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(giver,inputs,outputs,disappear)
    d=next(r['definition'] for r in smoke['requests'] if r['block']['line']==line)
    assert d['zone_number']==1398 and d['source_area']=='smoke' and d['repeatable'] and d['daily_eligible']
stories={s['id']:s for s in smoke_map['stories']}
assert {tuple(c.items()) for s in stories.values() for c in s['contracts']}=={tuple(b['binding'].items()) for b in by_line.values()}
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in stories.values())
blade=stories['erk-hate-and-discontent'];staff=stories['erk-jeweled-staff']
assert blade['steps'][0]['contracts']==[by_line[71]['binding']]
assert [(s['item_vnums'],s['count']) for s in blade['steps'][1:-1]]==[([139814],1),([139829],1)]
assert [s['contracts'] for s in staff['steps'][:2]]==[[by_line[28]['binding']],[by_line[128]['binding']]]
assert [(s['item_vnums'],s['count']) for s in staff['steps'][2:-1]]==[([139825],1),([139822],1)]
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==1398]
assert len(units)==6 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==2
contacts={c['mob_vnum']:c for c in smoke_map['contacts']}
for giver in (139811,139813,139823):
    assert set(contacts[giver]['topics'])=={t for b in raw if b['kind'] in ('M','MA') and b['giver_vnum']==giver for t in b['body'][0].rstrip('~').split()}
assert sum(len(c['topics']) for c in contacts.values())==16
assert 'default' in contacts[139813]['topics'] and 'not a wildcard' in contacts[139813]['description']
assert contacts[139822]['keyword']=='charrzlk' and contacts[139814]['keyword']=='roj'
assert len(smoke['mobs'])==26 and len(smoke['items'])==36 and len(smoke['reset_commands'])==172
assert collections.Counter(r['command'] for r in smoke['reset_commands'])=={'M':83,'E':35,'F':26,'O':9,'G':9,'P':6,'D':4}
assert not smoke['special_assignments'] and not (ROOT/'areas/shp/smoke.shp').exists()
objects=dawndale_bodies('smoke','obj');rooms=dawndale_bodies('smoke','wld')
assert set(rooms)==set(range(139800,139953))
assert sum(not b.split('~')[1].strip() for b in rooms.values())==1
assert collections.Counter(int(b.split('~')[2].split()[2]) for b in rooms.values())=={11:131,19:18,0:3,35:1}
assert int(rooms[139832].split('~')[2].split()[1])&4 and not int(rooms[139832].split('~')[2].split()[1])&2048
assert int(rooms[139945].split('~')[2].split()[2])==35
for v,d,target in ((139941,0,139942),(139942,2,139941)):
    assert re.search(r'\bD'+str(d)+r'\s+[^~]*~[^~]*~\s+3 139818 '+str(target)+r'\b',rooms[v],re.S)
for v in (139812,139818):assert objvalues(objects[v])[0]==18 and objvalues(objects[v])[12]==100
assert objvalues(objects[139819])[0]==25
assert set(objects[139829].split('~')[0].split())=={'wretched','broadsword','hate','discontent'}
assert 'discontent' not in objects[139814].split('~')[0].split()
assert objvalues(objects[139811])[12:15]==[29,139812,100]
assert 'T\n512 2 1 60' in objects[139811]
assert objvalues(objects[139830])[0]==15 and objvalues(objects[139830])[11:19]==[0]*8
assert objects[139825].split('~')[0]==objects[139832].split('~')[0]
assert objects[139825].split('~')[1]!=objects[139832].split('~')[1]
for v,destination in ((139802,139937),(139807,139863),(139819,139941),(139820,25464),(139821,24405)):
    assert objvalues(objects[v])[11:14]==[destination,7,-1]
for area,v,destination in (('plane_air_one',24407,139832),('plane_fire_one',25406,139894)):
    assert objvalues(dawndale_bodies(area,'obj')[v])[11:14]==[destination,7,-1]
reset={r['line']:r for r in smoke['reset_commands']}
for line,command,values in (
    (27,'O',[0,139811,1,139894,100,0,0,0]),
    (28,'P',[1,139814,1,139811,100,0,0,0]),
    (72,'M',[0,139809,1,139863,100,0,0,0]),
    (75,'G',[1,139808,1,0,100,0,0,0]),
    (76,'G',[1,359,1,0,100,0,0,0]),
    (77,'M',[0,139822,1,139863,40,0,0,0]),
    (78,'G',[1,139829,1,0,100,0,0,0]),
    (81,'F',[1,139804,8,139875,25,0,0,0]),
    (82,'G',[1,139813,1,0,100,0,0,0]),
    (159,'M',[0,139815,1,139942,40,0,0,0]),
    (160,'E',[1,139822,1,1,100,0,0,0]),
    (163,'M',[0,139818,1,139942,40,0,0,0]),
    (164,'E',[1,139823,1,1,100,0,0,0]),
    (171,'M',[0,139817,1,139942,40,0,0,0]),
    (172,'E',[1,139825,1,18,100,0,0,0])):
    assert (reset[line]['command'],reset[line]['arguments'])==(command,values)
for v,packed,level,chance,spells in ((139835,110351,50,35,[351,110,0]),(139831,14545072,56,30,[72,545,14])):
    values=objvalues(objects[v]);assert values[16:19]==[packed,level,chance]
    assert [packed%1000,(packed//1000)%1000,packed//1000000]==spells
assert objvalues(objects[139823])[7]==3  # Current earring is finger-worn; builder intent pending.
handler=(ROOT/'src/world/handler.c').read_text(encoding='utf8')
assert 'world[room].sector_type == SECT_NEG_PLANE' in handler
weapon=(ROOT/'src/combat/attack_effects.c').read_text(encoding='utf8')
assert 'selected_packed_weapon_action(obj, ch, victim)' in weapon and '.spell_pointer' in weapon
permanent=(ROOT/'src/magic/spell_permanent_stats.c').read_text(encoding='utf8')
assert 'void spell_perm_increase_pow' in permanent and 'base_stats.Pow + 1' in permanent
assignment=(ROOT/'src/specs/specs.assign.c').read_text(encoding='utf8')
assert 'obj_index[real_object0(359)].func.obj = epic_stone;' in assignment


# Fishermans Wharf: exact quantities, optional supply routes and foreign ownership.
wharf=inventory_module.area_evidence(ROOT,'fishermans_wharf')
wharf_map=next(m for m in catalog['story_mappings'] if m['source_area']=='fishermans_wharf')
assert (wharf_map['schema_version'],wharf_map['revision'],wharf_map['coverage'])==(3,1,'complete')
assert len(wharf_map['stories'])==5 and len(wharf_map['contacts'])==13 and not wharf_map['exclusions']
assert collections.Counter(s['category'] for s in wharf_map['stories'])=={'story':2,'request':3}
assert collections.Counter(t['kind'] for s in wharf_map['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':8,'completion':2}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/fishermans_wharf.qst']
assert collections.Counter(b['kind'] for b in raw)=={'MA':8,'QA':5} and len(wharf['dialogue'])==8
by_line={b['line']:b for b in raw if 'binding' in b}
for line,giver,inputs,outputs in (
    (14,88902,[('I',88913)]+[('I',88910)]*4+[('I',88911)]*3,[('I',88914)]),
    (49,88904,[('I',88900)],[('I',88904),('E',7500)]),
    (68,88905,[('I',88902),('I',88904)],[('I',88906),('E',7500)]),
    (94,88906,[('I',88912)]*4,[('I',88902),('E',7500)]),
    (114,88907,[('I',88909)]*4,[('I',88905),('E',7500)])):
    b=by_line[line]
    assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(giver,inputs,outputs,False)
    d=next(r['definition'] for r in wharf['requests'] if r['block']['line']==line)
    assert d['source_area']=='fishermans_wharf' and d['zone_number']==889 and d['repeatable'] and d['daily_eligible']
stories={s['id']:s for s in wharf_map['stories']}
assert {tuple(c.items()) for s in stories.values() for c in s['contracts']}=={tuple(b['binding'].items()) for b in by_line.values()}
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in stories.values())
assert [(s['item_vnums'],s['count']) for s in stories['baltiks-eight-supplies']['steps'][:-1]]==[([88913],1),([88910],4),([88911],3)]
adult=stories['adult-fishermans-supplies']
assert [s['contracts'] for s in adult['steps'][:2]]==[[by_line[94]['binding']],[by_line[49]['binding']]]
assert [(s['item_vnums'],s['count']) for s in adult['steps'][2:-1]]==[([88902],1),([88904],1)]
assert stories['old-fishermans-bottle-cleanup']['steps'][0]['count']==stories['eager-fishermans-frog-jelly']['steps'][0]['count']==4
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==889]
assert len(units)==sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==5
contacts={c['mob_vnum']:c for c in wharf_map['contacts']}
for giver in (88902,88904,88905,88906,88907):
    assert set(contacts[giver]['topics'])=={t for b in raw if b['kind']=='MA' and b['giver_vnum']==giver for t in b['body'][0].rstrip('~').split()}
assert sum(len(c['topics']) for c in contacts.values())==44
assert all(c['keyword'] in wharf['mobs'][v]['keywords'] for v,c in contacts.items())
assert len(wharf['mobs'])==23 and len(wharf['items'])==20 and len(wharf['reset_commands'])==119
assert collections.Counter(r['command'] for r in wharf['reset_commands'])=={'M':78,'O':19,'G':10,'D':6,'E':6}
assert len({(r['command'],tuple(r['arguments'])) for r in wharf['reset_commands']})==106
assert not wharf['special_assignments']
shop=(ROOT/'areas/shp/fishermans_wharf.shp').read_text(encoding='utf8')
assert shop.startswith('#88903~\nN\n88906\n88903\n88901\n0\n') and '\n88909\n' in shop
objects=dawndale_bodies('fishermans_wharf','obj');rooms=dawndale_bodies('fishermans_wharf','wld')
assert set(rooms)==set(range(88900,88970)) and set(objects)==set(range(88900,88920))
assert all(b.split('~')[1].strip() for b in rooms.values())
assert collections.Counter(int(b.split('~')[2].split()[2]) for b in rooms.values())=={4:6,1:6,0:2,3:15,6:18,7:5,2:10,9:4,10:4}
assert all(not int(b.split('~')[2].split()[1])&2048 for b in rooms.values())
assert re.search(r'\bF\s+50\b',rooms[88960])
for v,d,f,key,target in ((88965,2,3,88914,88968),(88968,0,1,0,88965),(88922,4,0,0,88960),(88960,5,0,0,88922),(88900,2,0,0,567269)):
    assert re.search(r'\bD'+str(d)+r'\s+[^~]*~[^~]*~\s+'+f'{f} {key} {target}'+r'\b',rooms[v],re.S)
assert all(int(rooms[v].split('~')[2].split()[1])&32 for v in range(88961,88969))
assert int(rooms[88969].split('~')[2].split()[1])&131072
assert objvalues(objects[88914])[0]==34 and objvalues(objects[88914])[12]==0
assert (objvalues(objects[88905])[7],objvalues(objects[88905])[22])==(67371009,2048)
assert (objvalues(objects[88917])[7],objvalues(objects[88917])[22])==(2049,2048)
assert objvalues(objects[88903])[0]==39 and objvalues(objects[88903])[7]==4210689
assert objvalues(objects[88906])[0]==15 and objvalues(objects[88906])[11:16]==[150,0,0,500,0] and objvalues(objects[88906])[19]==-75
assert objvalues(objects[88912])[6]&4096 and objvalues(objects[88912])[6]&8192
reset={r['line']:r for r in wharf['reset_commands']}
for line,command,values in (
    (33,'O',[0,88900,1,88903,100,0,0,0]),
    (55,'E',[1,88903,2,18,100,0,0,0]),(61,'E',[1,88903,2,27,100,0,0,0]),
    (125,'M',[0,88910,1,88960,100,0,0,0]),(126,'G',[1,88913,1,0,100,0,0,0]),
    (131,'E',[1,88917,1,13,100,0,0,0]),(138,'G',[1,31320,1,0,100,0,0,0])):
    assert (reset[line]['command'],reset[line]['arguments'])==(command,values)
assert len([r for r in wharf['reset_commands'] if r['command']=='O' and r['arguments'][1]==88910])==4
assert len([r for r in wharf['reset_commands'] if r['command']=='O' and r['arguments'][1]==88912])==6
assert len([r for r in wharf['reset_commands'] if r['command']=='G' and r['arguments'][1]==88909])==5
assert len([r for r in wharf['reset_commands'] if r['command']=='G' and r['arguments'][1]==88911])==3
assert wharf['mobs'][88910]['keywords']==wharf['mobs'][88911]['keywords']
skull=objvalues(dawndale_bodies('dream','obj')[31320]);assert skull[0]==8 and skull[11:19]==[0]*8 and skull[6]&128
foreign=[b for b in inventory_module.native_blocks(ROOT) if 'binding' in b and any(k=='I' and n in (88905,31320) for k,n in b['give']) and b['source']!='areas/qst/fishermans_wharf.qst']
assert {(b['giver_vnum'],b['source'],b['line']) for b in foreign}=={(35216,'areas/qst/newhaven.qst',177),(31310,'areas/qst/dream.qst',46)}
qin=next(b for b in foreign if b['giver_vnum']==31310)
assert qin['give']==[('I',v) for v in range(31316,31321)] and qin['receive']==[('I',31315)]
newhaven=next(m for m in catalog['story_mappings'] if m['source_area']=='newhaven')
buyback=next(s for s in newhaven['stories'] if s['id']=='dibblys-snorkel-pipe-buyback')
assert buyback['category']=='service' and any(t.get('optional') and t.get('contracts')==[by_line[114]['binding']] for t in buyback['steps'])
fish=(ROOT/'src/economy/tradeskill.c').read_text(encoding='utf8')
pole=fish[fish.index('P_obj get_pole'):fish.index('void do_fish')]
assert 'ch->carrying' in pole and '88903' in pole and 'equipment' not in pole
policy=(ROOT/'src/item/item_command_policy.c').read_text(encoding='utf8')
assert 'ITEM_NODROP' not in policy[policy.index('bool item_command_uses_durable_ownership'):policy.index('bool item_command_object_is_takeable')]


# Northern Lakes: independent courier handoffs and exact source identities.
nlakes=inventory_module.area_evidence(ROOT,'nlakes')
nlakes_map=next(m for m in catalog['story_mappings'] if m['source_area']=='nlakes')
assert (nlakes_map['schema_version'],nlakes_map['revision'],nlakes_map['coverage'])==(3,1,'complete')
assert len(nlakes_map['stories'])==6 and len(nlakes_map['contacts'])==13 and not nlakes_map['exclusions']
assert all(s['category']=='story' for s in nlakes_map['stories'])
assert collections.Counter(t['kind'] for s in nlakes_map['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':7,'completion':2}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/nlakes.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':8,'Q':6} and len(nlakes['dialogue'])==7
by_line={b['line']:b for b in raw if 'binding' in b}
for line,giver,inputs,outputs,disappear in (
    (17,75239,[('I',75252)],[('I',75263)],True),
    (44,75254,[('I',75271),('I',75271),('I',75215)],[('C',250000),('I',55287)],False),
    (64,75255,[('I',75274)],[('I',75273)],True),
    (101,75260,[('I',75268)],[('I',75281)],False),
    (110,75260,[('I',75280)],[('I',75279),('C',10000)],False),
    (126,75261,[('I',75281)],[('I',75280)],False)):
    b=by_line[line]
    assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(giver,inputs,outputs,disappear)
    d=next(r['definition'] for r in nlakes['requests'] if r['block']['line']==line)
    assert d['source_area']=='nlakes' and d['zone_number']==752 and d['repeatable'] and d['daily_eligible']
stories={s['id']:s for s in nlakes_map['stories']}
assert {tuple(c.items()) for s in stories.values() for c in s['contracts']}=={tuple(b['binding'].items()) for b in by_line.values()}
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in stories.values())
assert [(s['item_vnums'],s['count']) for s in stories['arteks-dragon-and-demon-bundle']['steps'][:-1]]==[([75271],2),([75215],1)]
assert stories['green-dragons-vial']['steps'][0]['item_vnums']==[75274]
assert stories['lost-humans-recall']['steps'][0]['item_vnums']==[75252]
assert stories['tamara-package-heart']['steps'][0]['item_vnums']==[75268]
for sid,earlier,material in [('aerin-receive-order',101,75281),('tamara-return-note',126,75280)]:
    assert stories[sid]['steps'][0]['contracts']==[by_line[earlier]['binding']]
    assert stories[sid]['steps'][0]['optional'] and stories[sid]['steps'][1]['item_vnums']==[material]
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==752]
assert len(units)==sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==6
contacts={c['mob_vnum']:c for c in nlakes_map['contacts']}
for giver in (75239,75254,75255,75260,75261):
    assert set(contacts[giver]['topics'])=={t for b in raw if b['kind']=='M' and b['giver_vnum']==giver for t in b['body'][0].rstrip('~').split()}
assert sum(len(c['topics']) for c in contacts.values())==20 and not contacts[75259]['topics']
assert all(c['keyword'] in nlakes['mobs'][v]['keywords'] for v,c in contacts.items())
assert len(nlakes['mobs'])==63 and len(nlakes['items'])==82 and len(nlakes['reset_commands'])==414
assert collections.Counter(r['command'] for r in nlakes['reset_commands'])=={'M':166,'E':115,'D':50,'P':31,'G':23,'F':5,'R':4,'O':20}
assert len({(r['command'],tuple(r['arguments'])) for r in nlakes['reset_commands']})==309
assert not nlakes['special_assignments']
objects=dawndale_bodies('nlakes','obj');rooms=dawndale_bodies('nlakes','wld');mobiles=dawndale_bodies('nlakes','mob')
assert set(rooms)==set(range(75200,75419)) and set(objects)==set(range(75200,75283))-{75266}
assert all(b.split('~')[1].strip() for b in rooms.values())
assert collections.Counter(int(b.split('~')[2].split()[2]) for b in rooms.values())=={7:83,0:61,2:38,1:23,3:13,6:1}
assert all(not int(b.split('~')[2].split()[1])&(2048|524288|1024) for b in rooms.values())
assert all(not int(b.split('~')[4].split()[0])&32768 for b in mobiles.values())
assert int(rooms[75264].split('~')[2].split()[1])&131072 and int(rooms[75392].split('~')[2].split()[1])&8192
for v,speed,direction in [(75234,45,2),(75235,45,2),(75236,45,2),(75237,45,2),(75309,29,1),(75310,29,5),(75311,29,1),(75312,35,5),(75313,30,1),(75331,30,2),(75332,30,5),(75333,30,2),(75334,30,5),(75335,30,3),(75336,30,5),(75337,30,3),(75338,15,3)]:
    assert re.search(r'\bC\s+'+f'{speed} {direction}'+r'\b',rooms[v])
for v,chance in [(75266,10),(75267,10),(75275,23)]:assert re.search(r'\bF\s+'+str(chance)+r'\b',rooms[v])
for v,d,f,target in ((75202,0,5,75203),(75203,2,5,75202),(75211,5,5,75212),(75212,4,5,75211),(75248,5,5,75249),(75249,4,5,75248),(75274,0,5,75414),(75414,2,1,75274),(75403,1,5,75404),(75404,3,5,75403),(75264,0,0,75546),(75410,0,0,55297),(75418,4,0,537871),(75337,1,0,537889),(75363,1,0,539488)):
    assert re.search(r'\bD'+str(d)+r'\s+[^~]*~[^~]*~\s+'+f'{f} 0 {target}'+r'\b',rooms[v],re.S)
assert set(objects[75225].split('~')[0].split())==set(objects[75274].split('~')[0].split())
assert objvalues(objects[75225])[0]==10 and objvalues(objects[75274])[0]==13
assert all(objvalues(objects[v])[11:19]==[0]*8 for v in (75252,75268,75274,75280,75281))
assert objvalues(objects[75252])[0]==8 and objvalues(objects[75280])[0]==8 and objvalues(objects[75281])[0]==13
assert mobiles[75255].split('~')[0]==mobiles[75256].split('~')[0]
reset={r['line']:r for r in nlakes['reset_commands']}
for line,command,values in (
    (294,'M',[0,75206,1,75213,100,0,0,0]),(295,'E',[1,75215,1,3,100,0,0,0]),
    (306,'M',[0,75209,1,75216,100,0,0,0]),(308,'G',[1,75268,1,0,100,0,0,0]),
    (419,'M',[0,75261,1,75264,100,0,0,0]),(437,'M',[0,75239,1,75278,100,0,0,0]),
    (455,'M',[0,75249,1,75292,100,0,0,0]),(457,'E',[1,75252,1,18,100,0,0,0]),
    (503,'M',[0,75244,1,75372,100,0,0,0]),(505,'E',[1,75274,1,18,100,0,0,0]),
    (535,'M',[0,75254,1,75391,100,0,0,0]),(539,'M',[0,75255,1,75393,100,0,0,0]),
    (540,'E',[1,75271,2,11,100,0,0,0]),(558,'M',[0,75260,1,75409,100,0,0,0]),
    (561,'M',[0,75256,1,75411,100,0,0,0]),(562,'G',[1,75271,2,0,100,0,0,0])):
    assert (reset[line]['command'],reset[line]['arguments'])==(command,values)
foreign=[b for b in inventory_module.native_blocks(ROOT) if 'binding' in b and ('I',55287) in b['give'] and b['source']!='areas/qst/nlakes.qst']
assert [(b['giver_vnum'],b['source'],b['line']) for b in foreign]==[(55132,'areas/qst/wh.qst',2333)]
assert foreign[0]['give']==[('I',55287)] and foreign[0]['receive']==[('I',55040),('E',250000)]
wh=next(m for m in catalog['story_mappings'] if m['source_area']=='wh')
aevenyl=next(s for s in wh['stories'] if any(c['giver_vnum']==55132 for c in s['contracts']))
assert aevenyl['contracts']==[foreign[0]['binding']] and aevenyl['category']=='request'
assert '_noquest_' in dawndale_bodies('wh','obj')[55287].split('~')[0].split()
assert all(objvalues(dawndale_bodies('heavens','obj')[v])[0]==22 for v in (429,431))

# Northern Lakes: the two pile-of-bones clues match actual reciprocal exits.
nlakes_rooms=dawndale_bodies('nlakes','wld')
for direction,target,word,reverse in ((1,75223,'east',3),(3,75262,'west',1)):
    clue=re.search(r'\bD'+str(direction)+r'\s+([^~]*)~[^~]*~\s+0 0 '+str(target)+r'\b',nlakes_rooms[75263],re.S)
    assert clue and re.search(r'\b'+word+r'\b',clue[1]), 'Northern Lakes pile-of-bones exit clue is reversed'
    assert re.search(r'\bD'+str(reverse)+r'\s+[^~]*~[^~]*~\s+0 0 75263\b',nlakes_rooms[target],re.S)

# Kobold: exact guarded commissions, optional services and physical source routes.
kobold=inventory_module.area_evidence(ROOT,'kobold')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='kobold')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==4 and len(mapping['contacts'])==16 and not mapping['exclusions']
assert collections.Counter(s['category'] for s in mapping['stories'])=={'service':3,'story':1}
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':5,'completion':2}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/kobold.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':13,'Q':4} and len(kobold['dialogue'])==6
by_line={b['line']:b for b in raw if 'binding' in b}
for line,inputs,outputs in ((52,[('I',1448),('I',1448),('C',170000)],[('I',1451)]),(65,[('I',1447)]*8+[('C',10000)],[('I',1448)]),(85,[('I',1431)],[('I',1431)]),(93,[('C',20000),('I',1433),('I',1431)],[('I',1432)])):
    b=by_line[line]
    assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(1420,inputs,outputs,False)
    d=next(r['definition'] for r in kobold['requests'] if r['block']['line']==line)
    assert d['source_area']=='kobold' and d['zone_number']==14 and not d['daily_eligible']
    assert d['daily_exclusion']==('Item exchange' if line==85 else 'Unsupported durable offering')
stories={s['id']:s for s in mapping['stories']}
assert {tuple(c.items()) for s in stories.values() for c in s['contracts']}=={tuple(b['binding'].items()) for b in by_line.values()}
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in stories.values())
assert [(t['item_vnums'],t['count']) for t in stories['silver-smelting']['steps'] if t['kind']=='carried_item']==[([1447],8)]
assert [(t['item_vnums'],t['count']) for t in stories['silver-shield']['steps'] if t['kind']=='carried_item']==[([1448],2)]
assert stories['silver-shield']['steps'][0]['contracts']==[by_line[65]['binding']]
assert stories['gem-spectacles']['steps'][0]['contracts']==[by_line[85]['binding']]
assert [(t['item_vnums'],t['count']) for t in stories['gem-spectacles']['steps'] if t['kind']=='carried_item']==[([1431],1),([1433],1)]
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==14]
assert len(units)==4 and sum(u['achievement'] for u in units)==1 and not any(u['daily_candidate'] for u in units)
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert contacts[1420]['topics']==['rod','rulership','tyxru','mine','mines','silver','fee','block','blocks','shield','nugget','nuggets','jewels','jewel','eyes','eye']
assert sum(len(c['topics']) for c in contacts.values())==16 and contacts[1413]['keyword']=='maid'
assert all(not contacts[v]['topics'] for v in (1418,1446,1447))
guidance=' '.join(mapping['orientation']+[s['summary'] for s in mapping['stories']])
for phrase in ('accounting','five','eight','frames','inspection','forge','pit','i>|uub','ambassador','rune-covered','reassembly'):
    assert phrase in guidance,phrase
rooms=dawndale_bodies('kobold','wld');objects=dawndale_bodies('kobold','obj');mobiles=dawndale_bodies('kobold','mob')
assert (len(rooms),len(mobiles),len(objects),len(kobold['reset_commands']))==(147,58,66,303)
assert collections.Counter(r['command'] for r in kobold['reset_commands'])=={'M':182,'E':42,'G':28,'O':23,'D':16,'P':12}
assert len({(r['command'],tuple(r['arguments'])) for r in kobold['reset_commands']})==201
assert all(objvalues(objects[v])[0]==15 for v in (1438,1463))
assert objvalues(objects[1414])[0]==22
assert all(objvalues(objects[v])[0]==8 and objvalues(objects[v])[11:19]==[0]*8 for v in (1454,1455,1456,1457))
assert not any(r['command'] in 'OGEP' and r['arguments'][1] in (1453,1454,1455,1456,1457) for r in kobold['reset_commands'])
resets={r['line']:r for r in kobold['reset_commands']}
for line,c,args in ((91,'O',[0,1463,1,1476,100,0,0,0]),(92,'P',[1,1433,1,1463,100,0,0,0]),(93,'P',[1,1437,1,1463,100,0,0,0]),(103,'O',[0,358,1,1482,100,0,0,0]),(104,'O',[0,55440,1,1482,100,0,0,0]),(109,'M',[0,1420,1,1406,100,0,0,0]),(266,'M',[0,1433,1,1478,100,0,0,0]),(267,'E',[1,1431,1,19,100,0,0,0]),(271,'M',[0,1437,1,1481,100,0,0,0]),(278,'M',[0,1438,2,1482,100,0,0,0]),(281,'M',[0,1436,1,1484,100,0,0,0])):
    assert (resets[line]['command'],resets[line]['arguments'])==(c,args)
for line in (291,294,297,299,302):assert (resets[line]['command'],resets[line]['arguments'])==('G',[1,1447,5,0,100,0,0,0])
for v,d,kind,key,target in ((1463,0,9,0,1469),(1469,2,9,0,1463),(1449,1,9,0,1470),(1470,3,1,0,1449),(1481,1,2,-2,1482),(1482,3,1,-1,1481),(1400,2,0,0,622130),(1542,1,0,0,816080)):
    assert re.search(r'\bD'+str(d)+r'\s+[^~]*~[^~]*~\s+'+f'{kind} {key} {target}'+r'\b',rooms[v],re.S)
assert all(not re.search(r'^\s*[FC]\s+-?\d+',b,re.M) for b in rooms.values())
assert objvalues(objects[1421])[11:15]==[341,1463,0,1] and objvalues(objects[1425])[11:15]==[341,1469,2,1] and objvalues(objects[1427])[11:15]==[340,1449,1,0]
foreign=[b for b in inventory_module.native_blocks(ROOT) if 'binding' in b and ('I',55440) in b['give']]
assert [(b['giver_vnum'],b['source'],b['line']) for b in foreign]==[(55272,'areas/qst/wh.qst',3652)]
assert foreign[0]['receive']==[('I',55362),('C',1000000),('I',55033)]
wh=next(m for m in catalog['story_mappings'] if m['source_area']=='wh')
assert any(foreign[0]['binding'] in s['contracts'] for s in wh['stories'])


# Troll Caves: exact kinds, guarded preparation, dynamic controls and source alternatives.
troll=inventory_module.area_evidence(ROOT,'troll_caves')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='troll_caves')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==5 and len(mapping['contacts'])==8 and not mapping['exclusions']
assert collections.Counter(s['category'] for s in mapping['stories'])=={'service':4,'story':1}
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':6,'completion':2}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/troll_caves.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':5,'Q':5} and len(troll['dialogue'])==5
by_line={b['line']:b for b in raw if 'binding' in b}
for line,giver,inputs,outputs in ((23,96916,[('I',96933),('C',890000)],[('I',96915)]),(42,96916,[('I',96907),('C',400000)],[('I',96916)]),(61,96916,[('I',96906),('C',200000)],[('I',96917)]),(106,96925,[('I',96915),('I',96931)],[('I',96932)]),(148,96926,[('C',100000),('I',96910)],[('I',96933)])):
    b=by_line[line]
    assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(giver,inputs,outputs,False)
    d=next(r['definition'] for r in troll['requests'] if r['block']['line']==line)
    assert d['source_area']=='troll_caves' and d['zone_number']==969 and d['daily_eligible']==(line==106)
    assert d['daily_exclusion']==('' if line==106 else 'Unsupported durable offering')
assert ('I',96930) not in by_line[148]['give'] # The dialogue's wand is not a native prerequisite.
stories={s['id']:s for s in mapping['stories']}
assert {tuple(c.items()) for s in stories.values() for c in s['contracts']}=={tuple(b['binding'].items()) for b in by_line.values()}
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in stories.values())
assert stories['emerald-mace']['steps'][0]['contracts']==[by_line[148]['binding']]
assert stories['chalice-mace-blessing']['steps'][0]['contracts']==[by_line[23]['binding']]
assert [(t['item_vnums'],t['count']) for t in stories['chalice-mace-blessing']['steps'] if t['kind']=='carried_item']==[([96915],1),([96931],1)]
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==969]
assert len(units)==5 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==1
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert contacts[96916]['topics']==['weapons','weapon'] and contacts[96925]['topics']==['mace','hraaf','weapon','weapons','dagger','longsword'] and contacts[96926]['topics']==['emeralds']
assert sum(len(c['topics']) for c in contacts.values())==9 and not troll['special_assignments']
rooms=dawndale_bodies('troll_caves','wld');objects=dawndale_bodies('troll_caves','obj');mobiles=dawndale_bodies('troll_caves','mob')
assert (len(rooms),len(mobiles),len(objects),len(troll['reset_commands']))==(82,28,37,193)
assert (troll['zone']['first_vnum'],troll['zone']['last_vnum'],troll['zone']['reset_mode'])==(96886,96981,2)
assert collections.Counter(r['command'] for r in troll['reset_commands'])=={'M':111,'D':28,'E':20,'G':18,'O':11,'F':4,'P':1}
assert len({(r['command'],tuple(r['arguments'])) for r in troll['reset_commands']})==142
families=set();parent=leader=None
for r in troll['reset_commands']:
    c,a=r['command'],r['arguments'];target=leader if c=='F' else parent if c in 'GE' else a[3] if c=='P' else None
    families.add((c,tuple(a),target))
    if c=='M':parent=leader=(a[1],a[3])
    elif c=='F':parent=(a[1],a[3])
assert len(families)==152
assert objects[96915].split('~')[0]==objects[96932].split('~')[0] and objects[96915].split('~')[1]==objects[96932].split('~')[1]
assert objects[96910].split('~')[0]==objects[96933].split('~')[0]
assert objvalues(objects[96900])[11:15]==[270,96935,2,1] and objvalues(objects[96908])[11:15]==[270,96942,2,1]
assert objvalues(objects[96911])[11:14]==[96937,189,-1] and objvalues(objects[96912])[11:14]==[96940,320,-1]
resets={r['line']:r for r in troll['reset_commands']}
for line,c,args in ((121,'O',[0,96900,1,96935,100,0,0,0]),(123,'O',[0,96911,1,96939,100,0,0,0]),(124,'O',[0,96930,1,96940,100,0,0,0]),(125,'O',[0,96906,1,96942,100,0,0,0]),(126,'O',[0,96907,1,96942,100,0,0,0]),(128,'O',[0,96912,1,96943,100,0,0,0]),(130,'O',[0,96910,1,96944,100,0,0,0]),(166,'M',[0,96915,1,96920,100,0,0,0]),(169,'E',[1,96931,1,18,100,0,0,0]),(173,'M',[0,96916,1,96925,100,0,0,0]),(255,'M',[0,96925,1,96959,100,0,0,0]),(257,'M',[0,96926,1,96960,100,0,0,0])):
    assert (resets[line]['command'],resets[line]['arguments'])==(c,args)
falls={v:int(m[1]) for v,b in rooms.items() if (m:=re.search(r'^F\s*\n(\d+)',b,re.M))}
assert falls=={96904:10,96905:10,96906:10,96907:6,96908:20,96913:5,96916:10,96917:10,96918:10,96923:5,96926:3,96927:2,96928:2,96929:1,96941:100,96965:10,96966:10,96967:10,96968:10}
assert all(not re.search(r'^C\s*\n',b,re.M) for b in rooms.values())
stock=(ROOT/'areas/shp/surfacekeeps.shp').read_text().split('X')[0]
assert stock.startswith('#120003~\nN\n96910\n96907\n96906\n0\n') and '\n123215\n' in stock
for v,d,kind,key,target in ((96935,2,13,0,96936),(96936,0,13,0,96935),(96942,2,13,0,96943),(96943,0,13,0,96942),(96939,0,5,0,96940),(96940,2,5,0,96939),(96900,4,0,0,622564),(96976,3,0,0,23741)):
    assert re.search(r'\bD'+str(d)+r'\s+[^~]*~[^~]*~\s+'+f'{kind} {key} {target}'+r'\b',rooms[v],re.S)
assert all(resets[line]['arguments'][3]==0 for line in (88,92)) # Actual waterfall reset opens both sides.
guidance=' '.join(mapping['orientation']+[s['summary'] for s in mapping['stories']])
for phrase in ('accounting','wand','PUNCH','TOUCH','reverse','990','pre-blessing','enhanced','Alatorin','fall','forge'):
    assert phrase in guidance,phrase

# Centaur Villages: identical halves, supplied progression and actual hidden-name dispatch.
centaur=inventory_module.area_evidence(ROOT,'centaur_zone')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='centaur_zone')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==7 and len(mapping['contacts'])==8 and not mapping['exclusions']
assert collections.Counter(s['category'] for s in mapping['stories'])=={'story':4,'service':3}
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':8,'completion':8}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/centaur_zone.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':4,'Q':7} and len(centaur['dialogue'])==4
by_line={b['line']:b for b in raw if 'binding' in b}
for line,giver,inputs,outputs in ((21,93301,[('I',93310)],[('I',93311)]),(34,93301,[('I',93313)],[('I',93313)]),(46,93302,[('I',93311)],[('I',93311)]),(62,93302,[('I',93311),('I',93312)],[('I',93313)]),(126,93309,[('I',93313)],[('I',93313)]),(139,93309,[('I',93317)],[('I',93313)]),(152,93310,[('I',93313),('I',93313)],[('I',93314),('I',93330)])):
    b=by_line[line]
    assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(giver,inputs,outputs,line==152)
    d=next(r['definition'] for r in centaur['requests'] if r['block']['line']==line)
    assert d['zone_number']==933 and d['source_area']=='centaur_zone' and d['daily_eligible']==(line in (21,62,139,152))
    assert d['daily_exclusion']==('Item exchange' if line in (34,46,126) else '')
stories={s['id']:s for s in mapping['stories']}
assert {tuple(c.items()) for s in stories.values() for c in s['contracts']}=={tuple(b['binding'].items()) for b in by_line.values()}
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in stories.values())
assert [(t['item_vnums'],t['count']) for t in stories['centaur-honor']['steps'] if t['kind']=='carried_item']==[([93313],2)]
assert [t['contracts'] for t in stories['centaur-honor']['steps'] if t.get('optional') and t['kind']=='completion']==[[by_line[62]['binding']],[by_line[139]['binding']]]
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==933]
assert len(units)==7 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==4
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert contacts[93301]['topics']==['dragon','heart','amulet'] and contacts[93308]['topics']==['learned','one','banitoor','vorsileez'] and contacts[93309]['topics']==['amulet']
assert sum(len(c['topics']) for c in contacts.values())==8 and not centaur['special_assignments']
rooms=dawndale_bodies('centaur_zone','wld');objects=dawndale_bodies('centaur_zone','obj');mobiles=dawndale_bodies('centaur_zone','mob')
assert (len(rooms),len(mobiles),len(objects),len(centaur['reset_commands']))==(100,29,31,199)
assert (centaur['zone']['first_vnum'],centaur['zone']['last_vnum'],centaur['zone']['reset_mode'])==(93116,93399,2)
assert collections.Counter(r['command'] for r in centaur['reset_commands'])=={'M':70,'P':71,'G':23,'O':21,'F':7,'D':6,'E':1}
assert len({(r['command'],tuple(r['arguments'])) for r in centaur['reset_commands']})==117
families=set();parent=leader=None
for r in centaur['reset_commands']:
    c,a=r['command'],r['arguments'];target=leader if c=='F' else parent if c in 'GE' else a[3] if c=='P' else None
    families.add((c,tuple(a),target))
    if c=='M':parent=leader=(a[1],a[3])
    elif c=='F':parent=(a[1],a[3])
assert len(families)==125
for v,target,d,mode in ((93305,93377,0,1),(93306,93375,2,1),(93307,93363,0,1),(93308,93365,2,1),(93323,93326,1,0)):
    values=objvalues(objects[v]);assert values[0]==29 and values[6]&2 and values[11:15]==[270,target,d,mode]
assert objvalues(objects[93316])[0]==15 and objvalues(objects[93316])[6]&2
resets={r['line']:r for r in centaur['reset_commands']}
for line,c,args in ((53,'O',[0,93323,1,93326,100,0,0,0]),(118,'O',[0,93316,1,93370,100,0,0,0]),(119,'P',[1,93317,1,93316,100,0,0,0]),(125,'M',[0,93304,1,93317,100,0,0,0]),(128,'G',[1,93318,1,0,100,0,0,0]),(129,'P',[1,93312,1,93318,100,0,0,0]),(144,'M',[0,93310,1,93330,100,0,0,0]),(145,'M',[0,93301,1,93335,100,0,0,0]),(146,'G',[1,93311,1,0,100,0,0,0]),(173,'M',[0,93302,1,93366,100,0,0,0]),(174,'G',[1,93313,2,0,100,0,0,0]),(203,'M',[0,93308,1,93386,100,0,0,0]),(221,'M',[0,93300,1,93398,100,0,0,0]),(222,'G',[1,93310,1,0,100,0,0,0]),(223,'M',[0,93309,1,93399,100,0,0,0]),(225,'G',[1,93313,2,0,100,0,0,0])):
    assert (resets[line]['command'],resets[line]['arguments'])==(c,args)
falls={v:int(m[1]) for v,b in rooms.items() if (m:=re.search(r'^F\s*\n(\d+)',b,re.M))}
currents={v:tuple(map(int,m.groups())) for v,b in rooms.items() if (m:=re.search(r'^C\s*\n(\d+)\s+(\d+)',b,re.M))}
assert falls=={93305:38,93306:13} and currents=={93337:(5,1),93338:(6,1),93341:(7,2),93349:(8,2)}
assert [(resets[line]['arguments'][1:4]) for line in (8,12,16,20,24,28)]==[[93326,1,8],[93399,3,0],[93363,0,8],[93365,2,8],[93375,2,8],[93377,0,8]]
lookup=(ROOT/'src/world/handler.c').read_text(encoding='utf8')
assert 'CAN_SEE_OBJ(ch, i) || IS_NOSHOW(i)' in lookup and '(IS_NOSHOW(i) && OBJ_VNUM(i) != VNUM_TRACKS)' in lookup
assert not (ROOT/'areas/shp/centaur_zone.shp').exists()
import runpy
runpy.run_path(str(ROOT/'tests/async/test_centaur_directions.py'))['check'](ROOT)
guidance=' '.join(mapping['orientation']+[s['summary'] for s in mapping['stories']])
for phrase in ('accounting','two separate','same kind','PUSH','eastern edge','current','fall','disappears','supplied'):
    assert phrase in guidance,phrase

# Opal Phoenix: supplied errands, actual hidden-source selection and indexed rewards.
opal=inventory_module.area_evidence(ROOT,'opalphoenix')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='opalphoenix')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==3 and len(mapping['contacts'])==9 and not mapping['exclusions']
assert collections.Counter(s['category'] for s in mapping['stories'])=={'story':3}
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':3,'completion':1}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/opalphoenix.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':3,'Q':2,'QA':1} and len(opal['dialogue'])==3
by_line={b['line']:b for b in raw if 'binding' in b}
for line,giver,inputs,outputs in ((14,70801,[('I',70823)],[('I',70811),('I',70812),('E',40000)]),(34,70802,[('I',70812)],[('I',70823),('E',20000)]),(62,70815,[('I',70819)],[('I',70820)])):
    b=by_line[line]
    assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(giver,inputs,outputs,line!=62)
    d=next(r['definition'] for r in opal['requests'] if r['block']['line']==line)
    assert d['zone_number']==708 and d['source_area']=='opalphoenix' and d['daily_eligible'] and not d['daily_exclusion']
stories={s['id']:s for s in mapping['stories']}
assert {tuple(c.items()) for s in stories.values() for c in s['contracts']}=={tuple(b['binding'].items()) for b in by_line.values()}
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in stories.values())
assert stories['sand-delivery']['steps'][0]['contracts']==[by_line[34]['binding']] and stories['sand-delivery']['steps'][0]['optional']
assert [(s['id'],t['item_vnums'],t['count']) for s in mapping['stories'] for t in s['steps'] if t['kind']=='carried_item']==[('lost-quill',[70812],1),('sand-delivery',[70823],1),('forest-reagents',[70819],1)]
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==708]
assert len(units)==3 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==3
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert contacts[70801]['topics']==['hi','hello','sand'] and contacts[70802]['topics']==['hi','hello','lost','something'] and contacts[70815]['topics']==['elves']
assert sum(len(c['topics']) for c in contacts.values())==8
rooms=dawndale_bodies('opalphoenix','wld');objects=dawndale_bodies('opalphoenix','obj');mobiles=dawndale_bodies('opalphoenix','mob')
assert (len(rooms),len(mobiles),len(objects),len(opal['reset_commands']))==(76,18,24,84)
assert (opal['zone']['first_vnum'],opal['zone']['last_vnum'],opal['zone']['reset_mode'])==(70353,70876,2)
assert collections.Counter(r['command'] for r in opal['reset_commands'])=={'M':50,'G':13,'D':12,'E':4,'O':3,'P':2}
assert len({(r['command'],tuple(r['arguments'])) for r in opal['reset_commands']})==76
families=set();parent=None
for r in opal['reset_commands']:
    c,a=r['command'],r['arguments'];target=parent if c in 'GE' else a[3] if c=='P' else None
    families.add((c,tuple(a),target))
    if c=='M':parent=(a[1],a[3])
assert len(families)==76
assert objvalues(objects[70823])[:11]==[12,15,3,0,5,0,16384,16385,32768,0,0]  # Future rewards visible, NORESET preserved.
assert objvalues(objects[70810])[0]==15 and objvalues(objects[70810])[6]==0 and objvalues(objects[70810])[11:15]==[80,1,0,80]
assert objvalues(objects[70812])[0]==21 and objvalues(objects[70812])[6]==0 and objvalues(objects[70812])[7]&1
assert objvalues(objects[70816])[0]==15 and objvalues(objects[70816])[6]&4096 and objvalues(objects[70816])[11:15]==[30,13,70821,40]
assert objvalues(objects[70821])[0]==18 and objvalues(objects[70821])[6]==8409088 and objvalues(objects[70821])[12]==100
assert objvalues(objects[70819])[0]==12 and objvalues(objects[70819])[6]==4096
assert objvalues(objects[70822])[6]==16384 and objvalues(objects[70824])[0]==13
resets={r['line']:r for r in opal['reset_commands']}
for line,c,args in ((57,'O',[0,70810,1,70845,100,0,0,0]),(58,'P',[1,70812,1,70810,100,0,0,0]),(59,'O',[0,70816,1,70851,100,0,0,0]),(60,'P',[1,70822,1,70816,100,0,0,0]),(86,'M',[0,70802,1,70845,100,0,0,0]),(95,'M',[0,70806,1,70852,100,0,0,0]),(96,'E',[1,70818,1,18,100,0,0,0]),(97,'G',[1,70821,1,0,100,0,0,0]),(111,'M',[0,70816,2,70865,100,0,0,0]),(112,'G',[1,70819,1,0,100,0,0,0]),(114,'M',[0,70818,1,70868,100,0,0,0]),(115,'M',[0,70815,1,70870,100,0,0,0]),(117,'M',[0,70816,2,70874,100,0,0,0]),(118,'M',[0,70801,1,70876,100,0,0,0])):
    assert (resets[line]['command'],resets[line]['arguments'])==(c,args)
for v,d,target in ((70846,3,70847),(70847,1,70846),(70849,3,70852),(70852,1,70849),(70854,2,70857),(70857,0,70854),(70855,2,70856),(70856,0,70855),(70860,0,70861),(70861,2,70860),(70861,1,70876),(70876,3,70861)):
    assert re.search(r'\bD'+str(d)+r'\s+[^~]*~[^~]*~\s+1 0 '+str(target)+r'\b',rooms[v],re.S)
assert all(r['arguments'][3]==1 for r in opal['reset_commands'] if r['command']=='D')
assert not any(re.search(r'^[FC]\s*\n',b,re.M) for b in rooms.values())
assert not any(objvalues(b)[0] in (25,29) for b in objects.values())
for v in (70851,70852,70869):
    assert int(rooms[v].split('~')[2].split()[1])&524288
assert int(rooms[70870].split('~')[2].split()[1])&131072
assert re.search(r'\bD1\s+[^~]*~[^~]*~\s+0 0 528831\b',rooms[70875],re.S)
surface=dawndale_bodies('surface','wld')
assert re.search(r'\bD3\s+[^~]*~[^~]*~\s+0 0 70875\b',surface[528831],re.S)
shop=(ROOT/'areas/shp/opalphoenix.shp').read_text(encoding='utf8')
assert set(map(int,re.search(r'^N\s*\n((?:\d+\s*\n)+?)0\s*\n',shop,re.M)[1].split()))==set(range(70801,70810))|{70811,70817}
assert 'refuse_unported_shop_mutation' in (ROOT/'src/economy/shop.c').read_text(encoding='utf8')
assert '{ 70806, SKILL_NATURES_SANCTITY, 0, 100, 0, 0, 0 }' in (ROOT/'src/classes/epic_skills.c').read_text(encoding='utf8')
assignments=opal['special_assignments']
assert len(assignments)==23 and all(70801>row['vnum'] or row['vnum']>70876 for row in assignments)
guidance=' '.join(mapping['orientation']+[s['summary'] for s in mapping['stories']])
for phrase in ('accounting','quill','single carried item','SEARCH','breaks','closed but unlocked','inn','experience','leaves','Supplied'):
    assert phrase in guidance,phrase


for area in ("twin_towers_forest", "newbie2", "newbie", "braddistock", "breale", "elvish", "krimman", "bastine", "pineholl", "quietus", "torg", "solonar", "wh", "smokev", "caertannad", "bs", "moria", "clwcvrn", "long", "blackpearl", "ravenloft2", "barovia", "tikitt", "jade", "savannah", "alatorin", "newhaven", "realm", "verspin", "shipy", "cosmic", "surface", "tharnadia", "minizones", "torrhan", "gold_hal", "ashrumite", "hall", "sarmiz", "delwyn", "divhome", "halfcut", "scorchvalley", "court", "snogres", "airshipgrave", "juiblex", "surfacemini", "nexus", "crakkaro", "roguerai", "desolate", "rftjngle", "trnsptow", "airp", "hunt", "tribal", "lornecro", "brass", "lortower", "mushroom_caverns", "smoke", "fishermans_wharf", "nlakes", "kobold", "troll_caves", "centaur_zone", "opalphoenix"):
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
