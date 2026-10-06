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
assert sum(u["achievement"] for u in catalog_module.story_units(catalog))==1522
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


# Myrabolus: exact access materials, competing deliveries and guarded support.
mira=inventory_module.area_evidence(ROOT,'mira')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='mira')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==16 and len(mapping['contacts'])==21 and not mapping['exclusions']
assert collections.Counter(s['category'] for s in mapping['stories'])=={'story':13,'service':3}
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':21,'completion':3}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/mira.qst']
assert collections.Counter(b['kind'] for b in raw)=={'Q':16,'M':1}
by_line={b['line']:b for b in raw if 'binding' in b}
for line,giver,inputs,outputs,departure in (
    (2,82500,[('I',13357)],[],False),
    (7,82507,[('I',82517)],[('I',82538)],False),
    (16,82507,[('I',13358)],[('I',13337)],False),
    (20,82507,[('I',76069)],[('I',82522)],False),
    (26,82515,[('I',13364)],[('I',13324)],False),
    (35,82516,[('I',13364)],[('I',13322)],False),
    (45,82518,[('I',82518),('I',82519)],[('I',82520)],False),
    (53,82518,[('I',82542),('C',50000)],[('I',40738)],False),
    (60,82522,[('I',13364)],[('I',13328)],True),
    (74,82537,[('I',13364)],[('I',13318)],False),
    (84,82538,[('I',82517)],[('I',82517),('I',82518)],True),
    (102,82543,[('I',13365)],[('C',20000)],False),
    (110,82565,[('I',431),('I',82550)],[('I',22279),('I',22280),('C',10000)],False),
    (126,82569,[('I',82543)],[('I',82543)],False),
    (134,82569,[('I',82547),('I',82549)],[('I',82551),('I',82552)],False),
    (140,82569,[('I',75825),('I',75826),('I',75834)],[('I',22631),('I',82555),('I',82554),('I',82553)],False),
):
    b=by_line[line]
    assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(giver,inputs,outputs,departure)
    d=next(r['definition'] for r in mira['requests'] if r['block']['line']==line)
    assert d['zone_number']==825 and d['source_area']=='mira'
    assert d['daily_eligible']==(line not in (53,84,126))
stories={s['id']:s for s in mapping['stories']}
assert {tuple(c.items()) for s in stories.values() for c in s['contracts']}=={tuple(b['binding'].items()) for b in by_line.values()}
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in stories.values())
assert {s['id'] for s in mapping['stories'] if s['category']=='service'}=={'roland-head-offering','andryn-dragon-armor','balance-letter-referral'}
assert stories['markam-parchment']['category']=='story'  # Returned note also grants a distinct new half.
assert [t['item_vnums'] for t in stories['andryn-keystone']['steps'] if t['kind']=='carried_item']==[[82518],[82519]]
assert [t['item_vnums'] for t in stories['officer-raft-sap']['steps'] if t['kind']=='carried_item']==[[431],[82550]]
assert [t['item_vnums'] for t in stories['balance-three-tokens']['steps'] if t['kind']=='carried_item']==[[75825],[75826],[75834]]
rigid=next(b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/cosmic.qst' and b['line']==40)
assert rigid['giver_vnum']==76029 and rigid['give']==[('I',76068)] and rigid['receive']==[('I',76069)]
assert stories['xavier-planetary-study']['steps'][0]['contracts']==[rigid['binding']]
assert stories['xavier-planetary-study']['steps'][1]['item_vnums']==[76069]
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==825]
assert len(units)==16 and sum(u['achievement'] for u in units)==13 and sum(u['daily_candidate'] for u in units)==12
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert contacts[82569]['topics']==['hi','hello','howdy','hey'] and sum(len(c['topics']) for c in contacts.values())==4
rooms=dawndale_bodies('mira','wld');objects=dawndale_bodies('mira','obj');mobiles=dawndale_bodies('mira','mob')
assert set(rooms)==set(range(82500,82688)) and set(mobiles)==set(range(82500,82578)) and set(objects)==set(range(82500,82562))
assert (mira['zone']['first_vnum'],mira['zone']['last_vnum'],mira['zone']['reset_mode'])==(82494,82687,1)
assert len(mira['reset_commands'])==316 and collections.Counter(r['command'] for r in mira['reset_commands'])=={'M':181,'D':44,'E':38,'G':22,'O':13,'F':13,'P':3,'R':2}
assert len({(r['command'],tuple(r['arguments'])) for r in mira['reset_commands']})==257
resets={r['line']:r for r in mira['reset_commands']}
for line,c,kind,parent in ((186,'O',82557,82519),(187,'P',82517,82557),(188,'O',82544,82579),(189,'P',82543,82544),(190,'O',82510,82588),(191,'P',82558,82510),(196,'O',2267,82626),(199,'O',82546,82662),(200,'O',82550,82673),(247,'G',82537,0),(310,'G',82519,0),(442,'E',82545,23),(443,'E',82559,16),(456,'G',82561,0)):
    r=resets[line];assert (r['command'],r['arguments'][1],r['arguments'][2],r['arguments'][3])==(c,kind,1,parent)
assert objvalues(objects[82557])[0]==15 and objvalues(objects[82557])[11:15]==[1000,13,82558,1000]
assert objvalues(objects[82510])[11:15]==[100,5,0,0]
assert objvalues(objects[82558])[0]==18 and objvalues(objects[82558])[6]&4096 and objvalues(objects[82558])[12]==100
assert objvalues(objects[82519])[0]==8 and objvalues(objects[82519])[6]==8917024
assert objvalues(objects[82520])[0]==18 and objvalues(objects[82520])[6]==8912896 and objvalues(objects[82520])[12]==100
assert objvalues(objects[82537])[0]==18 and objvalues(objects[82537])[12]==0
assert objvalues(objects[82538])[0]==18 and objvalues(objects[82538])[12]==0
assert objvalues(objects[82546])[0]==15 and objvalues(objects[82546])[6]==8192 and not objvalues(objects[82546])[7]&1  # Fixed FLOAT container, not buried or carried raft.
assert objvalues(objects[82544])[6]&2 and objvalues(objects[82544])[11:15]==[1000,5,0,1000]
assert objvalues(objects[82543])[0]==16 and objvalues(objects[82543])[6]&4096
for v,d,flags,key,target in ((82528,0,3,82537,82529),(82529,2,3,82537,82528),(82550,2,2,82538,82627),(82627,0,2,82538,82550),(82607,4,7,82520,82626),(82626,5,7,0,82607)):
    assert re.search(r'\bD'+str(d)+r'\s+[^~]*~[^~]*~\s+'+f'{flags} {key} {target}'+r'\b',rooms[v],re.S)
mobile_fields=list(map(int,re.match(r'\s*((?:-?\d+\s+)+)',mobiles[82537].split('~')[4])[1].split()))
assert mobile_fields[4]==268435472 and not mobile_fields[4]&(1<<31)
assert re.search(r'\n4 4 \d+\s*\n',mobiles[82537]) and not mobile_fields[1]&((1<<13)|(1<<18))
assert '#define AFF4_TUPOR BIT_31' in (ROOT/'src/core/defines.h').read_text()
assert re.search(r'IS_NPC\(k\) && IS_AWAKE\(k\) &&\s+mob_index\[GET_RNUM\(k\)\]\.qst_func',(ROOT/'src/cmd/interp.c').read_text())
assert not any(r['command'] in 'MFR' and r['arguments'][1] in (82542,82577) for r in mira['reset_commands'])
assert not any(r['command'] in 'OGEP' and r['arguments'][1] in (82511,82528,82530) for r in mira['reset_commands'])
assert all(('I',v) not in b['give'] for b in by_line.values() for v in (82511,82528,82530))
assert not any(re.search(rb'^#2267\s*$',p.read_bytes(),re.M) for p in (ROOT/'areas/obj').glob('*.obj'))
assert {(a['kind'],a['vnum'],a['function']) for a in mira['special_assignments']}=={('mob',82521,'money_changer'),('mob',82511,'world_quest'),('obj',82545,'master_set'),('obj',82500,'generic_parry_proc'),('room',82574,'inn'),('room',82641,'crew_shop_proc'),('room',82669,'ship_shop_proc')}
assign=(ROOT/'src/specs/specs.assign.c').read_text()
for text in ('storage_locker_obj_hook','spell_pool','huntsman_ward'):assert text in assign
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[t.get('hint','') for s in mapping['stories'] for t in s['steps']])
for phrase in ('accounting','delivery copy','invisible','breaks','WAKE','returned','loose','crew','locker','no declared reward'):
    assert phrase.lower() in guidance.lower(),phrase


# The Depths: exact alternatives, ten loose roots and causal source boundaries.
depths=inventory_module.area_evidence(ROOT,'surfacekeeps')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='surfacekeeps')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==12 and len(mapping['contacts'])==17 and not mapping['exclusions']
assert collections.Counter(s['category'] for s in mapping['stories'])=={'story':9,'service':3}
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':14,'completion':1}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/surfacekeeps.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':48,'MA':40,'QA':14,'Q':1}
by_line={b['line']:b for b in raw if 'binding' in b}
for line,giver,inputs,outputs,departure in (
    (22,120013,[('C',13000)],[('I',83223)],True),
    (109,120016,[('I',83340)],[('I',120038)],False),
    (115,120016,[('I',120038),('I',500121),('I',500104)],[('I',120039)],True),
    (289,120035,[('I',120014),('I',55033)],[('I',120054)],False),
    (355,120040,[('I',83376)],[('C',700000)],False),
    (360,120040,[('I',83377)],[('I',83377)],False),
    (367,120040,[('I',83118)],[('C',28000),('E',7000)],False),
    (375,120040,[('I',83172)],[('C',30000),('E',5000)],False),
    (383,120040,[('I',83428)],[('C',25000),('E',5000)],False),
    (390,120040,[('I',83417)],[('C',28750),('E',8750)],False),
    (484,120052,[('I',120051)],[('C',6800),('E',5000)],False),
    (491,120052,[('I',120052)],[('C',7777),('E',7777)],False),
    (497,120052,[('I',120053)],[('C',17433),('E',17332)],False),
    (536,120065,[('I',120049)],[('C',30000),('E',100000)],False),
    (562,120066,[('I',8)]*10,[('I',83463),('I',58814),('I',58814)],True),
):
    b=by_line[line]
    assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(giver,inputs,outputs,departure)
    d=next(r['definition'] for r in depths['requests'] if r['block']['line']==line)
    assert (d['zone_number'],d['source_area'])==(1200,'surfacekeeps')
    assert d['daily_eligible']==(line not in (22,360))
stories={s['id']:s for s in mapping['stories']}
assert {tuple(c.items()) for s in stories.values() for c in s['contracts']}=={tuple(b['binding'].items()) for b in by_line.values()}
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in stories.values())
assert {s['id'] for s in mapping['stories'] if s['category']=='service'}=={'boar-meat-service','permanent-blue-eyeglasses','ungalen-scale-referral'}
assert stories['ungalen-ale']['contracts']==[by_line[n]['binding'] for n in (367,375,383,390)]
assert stories['ungalen-ale']['steps'][0]['item_vnums']==[83118,83172,83428,83417] and stories['ungalen-ale']['steps'][0]['count']==1
assert stories['gulranor-ten-body-parts']['steps'][0]['item_vnums']==[8] and stories['gulranor-ten-body-parts']['steps'][0]['count']==10
assert stories['mystardala-paired-trophies']['steps'][0]['contracts']==[by_line[109]['binding']]
assert [t['item_vnums'] for t in stories['mystardala-paired-trophies']['steps'] if t['kind']=='carried_item']==[[120038],[500121],[500104]]
assert stories['tok-gloomwing-feather']['steps'][0]['item_vnums']==[120052]
assert stories['tok-stronger-gloomwing-feather']['steps'][0]['item_vnums']==[120053]
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==1200]
assert len(units)==12 and sum(u['achievement'] for u in units)==9 and sum(u['daily_candidate'] for u in units)==9
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert sum(len(c['topics']) for c in contacts.values())==109
for b in depths['dialogue']:
    assert set(b['body'][0].rstrip('~').split())<=set(contacts[b['giver_vnum']]['topics'])
for v,c in contacts.items():assert c['keyword'] in depths['mobs'][v]['keywords']
rooms=dawndale_bodies('surfacekeeps','wld');objects=dawndale_bodies('surfacekeeps','obj');mobiles=dawndale_bodies('surfacekeeps','mob')
assert len(rooms)==2645 and set(mobiles)==set(range(120000,120075)) and set(objects)==set(range(120000,120060))
assert len({(b.split('~')[0].strip(),b.split('~')[1].strip()) for b in rooms.values()})==101
assert (depths['zone']['first_vnum'],depths['zone']['last_vnum'],depths['zone']['reset_mode'])==(120000,123833,2)
exits=[]
for v,b in rooms.items():
    rest=b[b.index('~',b.index('~')+1)+1:]
    rest=re.sub(r'\bD\d+\s+[^~]*~[^~]*~\s*-?\d+\s+-?\d+\s+-?\d+\s*','',rest,flags=re.S).strip()
    assert re.fullmatch(r'1200\s+\d+\s+\d+\s+0\s+S',rest) and not int(rest.split()[1])&(1<<20)
    for d,desc,key,flags,door_key,target in re.findall(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S):
        assert not desc.strip() and not key.strip() and int(flags)==0
        exits.append((v,int(d),int(target)))
assert len(exits)==9452
active_rooms=set()
for z in catalog['zones']:
    path=ROOT/f"areas/wld/{z['source_area']}.wld"
    if path.is_file():active_rooms.update(int(m[1]) for m in re.finditer(r'^#(\d+)\s*$',path.read_text(),re.M))
assert {(v,d,t) for v,d,t in exits if t not in active_rooms}=={(120000,0,129900),(120000,3,120099),(120819,2,120919),(120918,1,120919),(120826,2,120926),(120925,1,120926)}
assert 'not transversable' in rooms[121257] and int(rooms[121257].split('~')[2].split()[2])==27
assert len(depths['reset_commands'])==611 and collections.Counter(r['command'] for r in depths['reset_commands'])=={'M':260,'E':137,'G':135,'F':32,'O':26,'R':11,'P':10}
assert len({(r['command'],tuple(r['arguments'])) for r in depths['reset_commands']})==406
resets={r['line']:r for r in depths['reset_commands']}
for line,c,kind,cap,parent,chance in ((64,'M',120052,1,120823,100),(67,'M',120066,1,120823,22),(94,'M',120060,1,120880,90),(95,'G',83376,1,0,100),(99,'G',83377,1,0,100),(201,'G',120016,1,0,100),(222,'M',120065,1,121359,100),(307,'M',120040,1,121680,100),(512,'M',120067,1,123047,100),(537,'M',120016,1,123133,100)):
    r=resets[line];assert (r['command'],r['arguments'][1:5])==(c,[kind,cap,parent,chance])
assert not any(r['command'] in 'MFR' and r['arguments'][1] in (120000,120001,120054) for r in depths['reset_commands'])
assert objects[120052].split('~')[0]==objects[120053].split('~')[0] and inventory_module.plain(objects[120052].split('~')[1])==inventory_module.plain(objects[120053].split('~')[1])
assert objvalues(objects[120049])[6]&4096 and objvalues(objects[120057])[:1]==[25] and objvalues(objects[120057])[11:14]==[83831,7,-1]
alatorin_objects=dawndale_bodies('alatorin','obj')
assert objvalues(alatorin_objects[83144])[11:15]==[10,29,83145,5] and objvalues(alatorin_objects[83144])[6]&4096
assert objvalues(alatorin_objects[83145])[0]==18 and objvalues(alatorin_objects[83145])[6]&4096 and objvalues(alatorin_objects[83145])[12]==0
alatorin_rows=(ROOT/'areas/zon/alatorin.zon').read_text().splitlines()
for line,command,kind,cap,parent in ((1463,'O',83144,1,83188),(1466,'P',83340,1,83144),(2121,'E',83145,3,18),(2786,'G',83145,3,0),(3900,'G',83145,3,0),(2850,'G',120053,6,0),(2852,'G',120052,5,0),(2860,'G',120053,6,0)):
    r=alatorin_rows[line-1].split();assert (r[0],list(map(int,r[2:5])))==(command,[kind,cap,parent])
assert alatorin_rows[2848].split()[2:5]==['83283','7','83381'] and alatorin_rows[2858].split()[2:5]==['83284','8','83381']
assert {(a['kind'],a['vnum'],a['function']) for a in depths['special_assignments']}=={('mob',120051,'wh_corpse_to_object'),('obj',120051,'wh_corpse_decay')}
carve=(ROOT/'src/classes/new_skills.c').read_text().split('void do_carve(',1)[1].split('void ',1)[0]
assert 'HUMANOID_CORPSE' in carve and 'obj_to_obj(carve, corpse)' in carve and 'read_object(8, VIRTUAL)' in carve
assert 'carve->type = ITEM_WEAPON' in carve and 'corpse->value[1] |= piece' in carve
custom=(ROOT/'src/specs/specs.winterhaven.c').read_text().split('int wh_corpse_to_object(',1)[1].split('/*\n * MOB PROCS',1)[0]
for phrase in ('read_object(GET_VNUM(ch), VIRTUAL)','obj_to_room(obj, ch->in_room)','obj->value[0]-- <= 0','VOBJ_WH_ROTTING_CORPSE','obj_to_obj(corpse, obj->loc.inside)','extract_obj(obj, TRUE)'):assert phrase in custom
for v in (120016,120054):assert 'pink' in objects[v].lower()  # Pending copy repair, not shipped here.
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[t.get('hint','') for s in mapping['stories'] for t in s['steps']])
for phrase in ('lockpicking','GET','CARVE','rotting','two','nine','supplied','loose'):
    assert phrase.lower() in guidance.lower(),phrase
assert catalog_module.report_for(catalog)['mapped_area_count']==167
assert catalog_module.report_for(catalog)['daily_unit_count']==1408
assert sum(catalog_module.report_for(catalog)['eligible_by_zone'].values())==1522
assert catalog_module.report_for(catalog)['story_unit_count']==2184


# Cloister: the native acceptance caption must name the actual required tablet.
mahr = next(b for b in inventory_module.native_blocks(ROOT)
            if b['source'] == 'areas/qst/cloister.qst' and b['line'] == 34)
assert mahr['give'] == [('I', 67100)] and mahr['receive'] == [('I', 67101)] and mahr['disappear']
assert 'smiles, accepting the tablet.' in '\n'.join(mahr['body'])
assert 'small tablet of adamantite' in dawndale_bodies('cloister', 'obj')[67100]

# IceCrag: quarantine incomplete content; preserve exact quantities and causal limits.
icecrag=inventory_module.area_evidence(ROOT,'icecrag')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='icecrag')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==10 and len(mapping['contacts'])==25 and len(mapping['exclusions'])==1
assert collections.Counter(s['category'] for s in mapping['stories'])=={'story':8,'service':2}
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':15,'completion':1}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/icecrag.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':55,'Q':10,'MA':1,'QA':1}
by_line={b['line']:b for b in raw if 'binding' in b}
for line,giver,inputs,outputs,departure in (
    (22,97001,[('C',1000)],[('I',97005)],True),
    (61,97002,[('I',11606),('I',11607)],[('I',97029),('E',50000)],True),
    (141,97006,[('I',16025),('I',15252),('I',315),('I',303),('I',11519),('I',6551),('I',66058)],[('I',97110)]+[('I',97136)]*3,False),
    (180,97008,[('I',97137),('I',97138),('I',97149)],[('I',97139)],True),
    (226,97010,[('C',800)],[('I',97146)]*2,True),
    (309,97014,[('I',90017)]*2+[('I',92048)]*2,[('C',75000),('E',30000),('I',97141)],True),
    (365,97020,[('I',97041),('I',97047),('I',97048),('C',25000)],[('I',97143),('I',97145)],True),
    (429,97021,[('I',97006)],[('I',97144)],True),
    (483,97023,[('I',97115)],[('I',97135)],True),
    (539,97029,[('I',97029)],[('I',97140)],True),
    (589,97039,[('I',97016),('I',97017)],[('C',250000),('I',55032)],False),
):
    b=by_line[line]
    assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(giver,inputs,outputs,departure)
    d=next(r['definition'] for r in icecrag['requests'] if r['block']['line']==line)
    assert (d['zone_number'],d['source_area'])==(970,'icecrag')
    assert d['daily_eligible']==(line not in (22,226,365))
stories={s['id']:s for s in mapping['stories']}
assert mapping['exclusions'][0]['contracts']==[by_line[141]['binding']]
assert '6551' in mapping['exclusions'][0]['reason']
classified=[c for s in mapping['stories']+mapping['exclusions'] for c in s['contracts']]
assert len(classified)==11 and {tuple(c.items()) for c in classified}=={tuple(b['binding'].items()) for b in by_line.values()}
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in stories.values())
assert [t['item_vnums'] for t in stories['priest-three-speech-pages']['steps'][:-1]]==[[97137],[97138],[97149]]
assert [(t['item_vnums'],t['count']) for t in stories['raucous-guest-wines']['steps'][:-1]]==[([90017],2),([92048],2)]
assert stories['commander-lost-book']['steps'][0]['item_vnums']==[97006]
assert stories['viscount-kitchen-onion']['steps'][0]['item_vnums']==[97115]
assert stories['siege-master-calfskin-shoes']['steps'][0]['contracts']==[by_line[61]['binding']]
assert [t['item_vnums'] for t in stories['myrke-two-hearts']['steps'][:-1]]==[[97016],[97017]]
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==970]
assert len(units)==10 and sum(u['achievement'] for u in units)==8 and sum(u['daily_candidate'] for u in units)==7
masha=next(r['definition']['definition_id'] for r in icecrag['requests'] if r['block']['line']==141)
assert not any(masha in u['contracts'] for u in units)
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert sum(len(c['topics']) for c in contacts.values())==129
for b in icecrag['dialogue']:assert set(b['body'][0].rstrip('~').split())<=set(contacts[b['giver_vnum']]['topics'])
for v,c in contacts.items():assert c['keyword'] in icecrag['mobs'][v]['keywords']
rooms=dawndale_bodies('icecrag','wld');objects=dawndale_bodies('icecrag','obj');mobiles=dawndale_bodies('icecrag','mob')
assert len(rooms)==243 and len(objects)==138 and set(mobiles)==set(range(97000,97066))
assert len({(b.split('~')[0].strip(),b.split('~')[1].strip()) for b in rooms.values()})==232
assert (icecrag['zone']['first_vnum'],icecrag['zone']['last_vnum'],icecrag['zone']['reset_mode'])==(96982,97289,2)
exits=[]
for v,b in rooms.items():
    assert not int(b.split('~')[2].split()[1])&(1<<20)
    for d,desc,key,flags,door_key,target in re.findall(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S):exits.append((v,int(d),int(flags),int(door_key),int(target),' '.join(key.split())))
assert len(exits)==628
assert (97026,0,3,-2,97027,'double doors auril') in exits
assert (97027,2,3,-2,97026,'double doors dontgivethiswordouttomortals') in exits
assert (97085,5,0,0,97352,'hatch') in exits
active_objects=set();active_rooms=set()
for z in catalog['zones']:
    for kind,target in (('obj',active_objects),('wld',active_rooms)):
        p=ROOT/f"areas/{kind}/{z['source_area']}.{kind}"
        if p.is_file():target.update(int(m[1]) for m in re.finditer(r'^#(\d+)\s*$',p.read_text(encoding='utf8'),re.M))
assert 6551 not in active_objects and 97352 not in active_rooms
assert {(v,d,t) for v,d,f,k,t,w in exits if t not in active_rooms}=={(97085,5,97352)}
assert objvalues(objects[97115])[8]&32768 and not objvalues(objects[97136])[8]&32768
assert objvalues(objects[97115])[11]==75 and objvalues(objects[97136])[11]==125
assert objects[97004].split('~')[:2]==objects[97006].split('~')[:2]
for kind in (97137,97138,97149):assert objects[kind].split('~')[:2]==objects[97137].split('~')[:2]
assert all(objvalues(objects[v])[6]&4096 for v in (97006,97009,97000,97107,97147))
assert objvalues(objects[97107])[11:15]==[2,29,97108,100]
assert objvalues(objects[97147])[11:15]==[250,15,97010,250] and objvalues(objects[97010])[12]==50
for v,target in ((97026,97026),(97133,97127)):assert objvalues(objects[v])[0]==25 and objvalues(objects[v])[11:14]==[target,259,-1]
assert len(icecrag['reset_commands'])==620 and collections.Counter(r['command'] for r in icecrag['reset_commands'])=={'E':247,'M':150,'D':148,'G':37,'O':30,'P':4,'F':4}
assert len({(r['command'],tuple(r['arguments'])) for r in icecrag['reset_commands']})==374
resets={r['line']:r for r in icecrag['reset_commands']}
for line,c,kind,cap,parent in ((604,'O',97115,1,97071),(712,'E',97149,1,18),(745,'G',97016,1,0),(842,'G',97017,1,0),(938,'E',97137,1,18),(969,'E',97138,1,18),(979,'G',97006,2,0),(1046,'G',97006,2,0)):
    r=resets[line];assert (r['command'],r['arguments'][1:4])==(c,[kind,cap,parent])
assert (ROOT/'areas/zon/undermountain.zon').read_text(encoding='utf8').splitlines()[983].split()[:5]==['P','1','92048','1','92040']
assignments=icecrag['special_assignments']
assert len(assignments)==20 and not any(a['kind']=='mob' and a['vnum'] in (97018,97019,97009,97012,97024) for a in assignments)
special=(ROOT/'src/specs/specs.winterhaven.c').read_text(encoding='utf8')
masha_proc=special.split('int ice_masha(',1)[1].split('int ice_tubby_merchant(',1)[0]
for phrase in ('CMD_GET','isname("onion", arg)','isname("all", arg)','bash(ch, pl)'):assert phrase in masha_proc
wolf=special.split('int ice_wolf(',1)[1].split('int ice_malice(',1)[0]
for phrase in ('character_list','97054','97055','97031','97032','extract_char(ch)'):assert phrase in wolf
assert 'zone_table' not in wolf
malice=special.split('int ice_malice(',1)[1]
assert 'CMD_DEATH' in malice and '97056' in malice and 'obj_to_char' in malice
speech=(ROOT/'src/cmd/actcomm.c').read_text(encoding='utf8').split('void check_magic_doors(',1)[1].split('void do_petition(',1)[0]
for phrase in ('key == -2','EX_LOCKED','isname(word, arg1)','back->to_room == ch->in_room'):assert phrase in speech
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[t.get('hint','') for s in mapping['stories'] for t in s['steps']])
for phrase in ('incomplete','world cap of one','RUB','supplied','three','loose','vapor','currently unavailable'):assert phrase.lower() in guidance.lower()
report=catalog_module.report_for(catalog)
assert (report['mapped_area_count'],sum(report['eligible_by_zone'].values()),report['daily_unit_count'],report['story_unit_count'])==(167,1522,1408,2184)


# Cloister: complete native classification, refusal semantics and optional source routes.
cloister=inventory_module.area_evidence(ROOT,'cloister')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='cloister')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==8 and len(mapping['contacts'])==16 and not mapping['exclusions']
assert collections.Counter(s['category'] for s in mapping['stories'])=={'story':7,'service':1}
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':9,'completion':2}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/cloister.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':26,'MA':2,'Q':7,'QA':1}
by_line={b['line']:b for b in raw if 'binding' in b}
for line,giver,inputs,outputs,departure in (
    (2,67100,[('I',76728)],[('I',67129)],True),
    (34,67102,[('I',67100)],[('I',67101)],True),
    (67,67103,[('I',67102)],[('I',67104)],False),
    (75,67103,[('I',67101)],[('I',67101)],False),
    (119,67104,[('I',83374)],[('I',67130)],False),
    (164,67107,[('I',67101)],[('E',15000)],True),
    (207,67114,[('I',67116)],[('I',67117)],True),
    (256,67120,[('I',67113),('I',67103)],[('I',67111)],True),
):
    b=by_line[line];assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(giver,inputs,outputs,departure)
    d=next(r['definition'] for r in cloister['requests'] if r['block']['line']==line)
    assert (d['zone_number'],d['source_area'],d['daily_eligible'])==(671,'cloister',line!=75)
assert 'accepting the tablet.' in '\n'.join(by_line[34]['body'])
stories={s['id']:s for s in mapping['stories']}
classified=[c for s in mapping['stories'] for c in s['contracts']]
assert len(classified)==8 and {tuple(c.items()) for c in classified}=={tuple(b['binding'].items()) for b in by_line.values()}
assert stories['tel-rejected-recommendation']['category']=='service'
assert stories['disciple-recommendation']['steps'][0]['contracts']==[by_line[34]['binding']]
assert stories['advisor-ring-and-poison']['steps'][0]['contracts']==[by_line[207]['binding']]
assert [t['item_vnums'] for t in stories['advisor-ring-and-poison']['steps'][1:-1]]==[[67113],[67103]]
assert stories['doss-mandes-robes']['steps'][0]['item_vnums']==[76728]
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in mapping['stories'])
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==671]
assert len(units)==8 and sum(u['achievement'] for u in units)==7 and sum(u['daily_candidate'] for u in units)==7
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert len(cloister['dialogue'])==19 and sum(len(c['topics']) for c in contacts.values())==28
all_mobs=inventory_module.prototypes(ROOT,catalog_module.zone_registry(ROOT),'mob')
for b in cloister['dialogue']:assert set(b['body'][0].rstrip('~').split())<=set(contacts[b['giver_vnum']]['topics'])
for v,c in contacts.items():assert c['keyword'] in all_mobs[v]['keywords']
rooms=dawndale_bodies('cloister','wld');objects=dawndale_bodies('cloister','obj');mobiles=dawndale_bodies('cloister','mob')
assert set(rooms)==set(range(67100,67171)) and set(objects)==set(range(67100,67131)) and set(mobiles)==set(range(67100,67124))
assert len({tuple(b.split('~')[:2]) for b in rooms.values()})==33
assert (cloister['zone']['first_vnum'],cloister['zone']['last_vnum'],cloister['zone']['reset_mode'])==(66904,67170,2)
assert not cloister['special_assignments']
exits=[]
for v,b in rooms.items():
    assert not int(b.split('~')[2].split()[1])&(1<<19)
    for d,desc,key,flags,door_key,target in re.findall(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S):exits.append((v,int(d),int(flags),int(door_key),int(target)))
assert len(exits)==157 and not any(t==67170 for v,d,f,k,t in exits)
for row in ((67140,0,8,0,67141),(67149,0,12,0,67169),(67169,2,4,0,67149),(67152,2,8,0,67155),(67164,3,12,0,67167),(67138,0,4,34230,34351)):assert row in exits
for v,cmd,room,direction in ((67105,17,67140,0),(67114,270,67152,2),(67115,270,67164,3),(67120,270,67149,0)):
    assert objvalues(objects[v])[0]==29 and objvalues(objects[v])[11:15]==[cmd,room,direction,0]
assert objvalues(objects[67118])[11:14]==[50,29,67117] and objvalues(objects[67121])[11:14]==[50,29,67111]
for v in (67107,67112):assert objvalues(objects[v])[0]==18 and objvalues(objects[v])[12]==100
for v in (67100,67102,67103,67113):assert objvalues(objects[v])[6]&4096
assert re.search(r'^T\s*\n2 4 1 100\s*$',objects[67116],re.M)
assert len(cloister['reset_commands'])==113 and collections.Counter(r['command'] for r in cloister['reset_commands'])=={'D':26,'O':8,'P':7,'M':45,'E':14,'G':5,'F':8}
assert len({(r['command'],tuple(r['arguments'])) for r in cloister['reset_commands']})==97
resets={r['line']:r for r in cloister['reset_commands']}
for line,c,kind,cap,parent in ((122,'P',67113,1,67118),(127,'P',67122,1,67121),(139,'G',67100,1,0),(140,'G',67103,1,0),(189,'G',67116,1,0),(192,'E',67102,1,18)):
    r=resets[line];assert (r['command'],r['arguments'][1:4])==(c,[kind,cap,parent])
assert 'G 1 67113 1 0 100' in (ROOT/'areas/zon/alatorin.zon').read_text(encoding='utf8')
assert 'G 1 83374 1 0 100' in (ROOT/'areas/zon/alatorin.zon').read_text(encoding='utf8')
assert 'G 1 76728 1 0 100' in (ROOT/'areas/zon/jade.zon').read_text(encoding='utf8')
switch=(ROOT/'src/specs/specs.object.c').read_text(encoding='utf8').split('int item_switch(',1)[1].split('int labelas(',1)[0]
for phrase in ('generic_find','obj->value[0] != cmd','EX_BLOCKED','EX_SECRET','Nothing happens'):assert phrase in switch
assert 'REMOVE_BIT(world[in_room].dir_option[door]->exit_info, EX_SECRET)' not in switch
loader=(ROOT/'src/world/db.c').read_text(encoding='utf8');assert 'obj->type == ITEM_SWITCH && !obj_index[nr].func.obj' in loader
search=(ROOT/'src/cmd/actobj.c').read_text(encoding='utf8').split('void do_search(',1)[1].split('void do_apply_poison(',1)[0]
assert '!IS_SET(EXIT(ch, door)->exit_info, EX_BLOCKED)' in search and 'REMOVE_BIT(EXIT(ch, door)->exit_info, EX_SECRET)' in search
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[t.get('hint','') for s in mapping['stories'] for t in s['steps']])
for phrase in ('PUSH','SEARCH','SAY Khildarak','supplied','rejection','world cap of one','trap','ten-minute','active, ready accounting'):assert phrase.lower() in guidance.lower(),phrase
report=catalog_module.report_for(catalog)
assert (report['mapped_area_count'],sum(report['eligible_by_zone'].values()),report['daily_unit_count'],report['story_unit_count'])==(167,1522,1408,2184)


# Turolopolis: exact ALL colours, retiring source, foreign giver and real portal/access roles.
willem=inventory_module.area_evidence(ROOT,'willem')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='willem')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==6 and len(mapping['contacts'])==26 and not mapping['exclusions']
assert all(s['category']=='story' for s in mapping['stories'])
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':11,'completion':1}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/willem.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':19,'Q':6}
by_line={b['line']:b for b in raw if 'binding' in b}
for line,giver,inputs,outputs,departure in (
    (27,7103,[('I',7103)],[('I',7100),('E',25000)],True),
    (117,7121,[('I',7124),('I',7116),('I',7113),('I',7107),('I',7110)],[('I',7131),('E',500000)],True),
    (149,7125,[('I',7135)],[('I',7137),('E',50000)],True),
    (165,7126,[('I',7131),('I',7139)],[('I',7138)],False),
    (204,7130,[('I',7141)],[('I',7142),('E',10000)],True),
    (273,7140,[('I',7154)],[('C',200000)],True),
):
    b=by_line[line];assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(giver,inputs,outputs,departure)
    d=next(r['definition'] for r in willem['requests'] if r['block']['line']==line)
    assert (d['zone_number'],d['source_area'],d['daily_eligible'])==(71,'willem',True)
stories={s['id']:s for s in mapping['stories']}
classified=[c for s in mapping['stories'] for c in s['contracts']]
assert len(classified)==6 and {tuple(c.items()) for c in classified}=={tuple(b['binding'].items()) for b in by_line.values()}
badge_steps=stories['lothrell-five-badges']['steps'][:-1]
assert [t['item_vnums'] for t in badge_steps]==[[7124],[7116],[7113],[7107],[7110]]
assert all(t['count']==1 and t['kind']=='carried_item' and t['optional'] for t in badge_steps)
assert 'blue ribbon laced with silver thread' in stories['emissary-timeworn-letter']['summary']
assert 'tightly bound gauze wrappings' in stories['minotaur-blue-ooze']['summary']
assert stories['kurtukr-bloodsaber-upgrade']['steps'][0]['contracts']==[by_line[117]['binding']]
assert [t['item_vnums'] for t in stories['kurtukr-bloodsaber-upgrade']['steps'][1:-1]]==[[7131],[7139]]
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in mapping['stories'])
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==71]
assert len(units)==6 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==6
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert len(willem['dialogue'])==19 and sum(len(c['topics']) for c in contacts.values())==39
all_mobs=inventory_module.prototypes(ROOT,catalog_module.zone_registry(ROOT),'mob')
for b in willem['dialogue']:assert set(b['body'][0].rstrip('~').split())<=set(contacts[b['giver_vnum']]['topics'])
for v,c in contacts.items():assert c['keyword'] in all_mobs[v]['keywords']
assert contacts[7126]['topics']==[] and contacts[7127]['topics']==['bloodsaber','saber']
rooms=dawndale_bodies('willem','wld');objects=dawndale_bodies('willem','obj');mobiles=dawndale_bodies('willem','mob')
assert set(rooms)==set(range(7100,7254)) and set(objects)==set(range(7100,7156)) and set(mobiles)==set(range(7100,7142))
assert len({tuple(b.split('~')[:2]) for b in rooms.values()})==109
assert sum(len(re.findall(r'^E\s*$',b,re.M)) for b in rooms.values())==22
assert (willem['zone']['first_vnum'],willem['zone']['last_vnum'],willem['zone']['reset_mode'])==(7044,7253,1)
assert not willem['special_assignments']
exits=[]
for v,b in rooms.items():
    assert not int(b.split('~')[2].split()[1])&(1<<19)
    for d,desc,key,flags,door_key,target in re.findall(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S):exits.append((v,int(d),int(flags),int(door_key),int(target)))
assert len(exits)==394 and not any(t==7198 for v,d,f,k,t in exits) and not any(v==7200 for v,d,f,k,t in exits)
for row in ((7102,2,0,0,548452),(7123,3,0,0,53000),(7140,4,7,7107,7158),(7159,2,7,7110,7158),(7164,0,7,7113,7158),(7165,1,7,7116,7158),(7172,3,7,7124,7158),(7158,4,0,0,7145),(7206,0,3,7134,7208),(7237,0,2,7143,7238)):
    assert row in exits,row
for v,target in ((7106,7140),(7109,7159),(7112,7164),(7115,7165),(7123,7172)):
    assert objvalues(objects[v])[0]==25 and objvalues(objects[v])[11:15]==[target,259,-1,0]
assert objvalues(objects[7155])[0]==25 and objvalues(objects[7155])[11:15]==[7145,7,-1,0]
for v in (7107,7110,7113,7116,7124):assert objvalues(objects[v])[0]==18 and objvalues(objects[v])[12]==0
for v in (7100,7134,7143):assert objvalues(objects[v])[0]==18 and objvalues(objects[v])[12]==100
assert objvalues(objects[7130])[0]==9
for v in (7139,7154):assert objvalues(objects[v])[6]&4096
assert not objvalues(objects[7135])[6]&4096
assert len(willem['reset_commands'])==199 and collections.Counter(r['command'] for r in willem['reset_commands'])=={'D':52,'O':16,'P':7,'M':77,'G':10,'E':29,'F':8}
assert len({(r['command'],tuple(r['arguments'])) for r in willem['reset_commands']})==160
source_rows=[];parent=None
for r in willem['reset_commands']:
    c,a=r['command'],r['arguments']
    if c in ('M','F'):parent=(c,a[1],a[3])
    if c in ('G','E'):source_rows.append((a[1],c,parent,a[2],a[3]))
for row in ((7107,'E',('M',7105,7140),1,24),(7110,'E',('M',7106,7159),1,24),(7113,'E',('M',7107,7164),1,24),(7116,'E',('M',7108,7165),1,24),(7124,'E',('M',7110,7172),1,24),(7130,'E',('M',7118,7197),1,24),(7133,'E',('F',7126,7199),1,8),(7103,'G',('M',7123,7205),1,0),(7139,'G',('M',7129,7227),1,0),(7135,'G',('M',7130,7231),1,0),(7154,'G',('M',7138,7253),1,0)):
    assert row in source_rows,row
assert sum(kind==7141 for kind,c,p,cap,slot in source_rows)==1
assert sum(r['command']=='M' and r['arguments'][1]==7115 for r in willem['reset_commands'])==4
assert sum(r['command']=='O' and r['arguments'][1]==7155 for r in willem['reset_commands'])==5
surface=(ROOT/'areas/zon/surface.zon').read_text(encoding='utf8')
assert len(re.findall(r'^M\s+0\s+7121\s+1\s+515264\s+100\b',surface,re.M))==1
assert not any(r['command']=='M' and r['arguments'][1]==7121 for r in willem['reset_commands'])
quest=(ROOT/'src/world/quest.c').read_text(encoding='utf8')
assert 'while (mob->carrying)' in quest and 'extract_obj(mob->carrying, TRUE);' in quest and 'extract_char(mob);' in quest
interpreter=(ROOT/'src/cmd/interp.c').read_text(encoding='utf8');assert 'check_item_teleport(exec_char, argument + begin + look_at, cmd)' in interpreter
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[t.get('hint','') for s in mapping['stories'] for t in s['steps']])
for phrase in ('RUB','ENTER','SEARCH','five exact colours','white badge','supplied','discard','foothills','plaza','living','active, ready accounting'):
    assert phrase.lower() in guidance.lower(),phrase
assert not any(phrase in guidance for phrase in ('tower’s','crumbling stairway','priest’s quarters','Ask about free','Ask about power'))
report=catalog_module.report_for(catalog)
assert (report['mapped_area_count'],sum(report['eligible_by_zone'].values()),report['daily_unit_count'],report['story_unit_count'])==(167,1522,1408,2184)


# Ixarkon: exact sources, optional guarded preparation and distinct campaign intent.
ixarkon=inventory_module.area_evidence(ROOT,'ixarkon')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='ixarkon')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,2,'complete')
assert len(mapping['stories'])==3 and len(mapping['contacts'])==16 and not mapping['exclusions']
assert [s['id'] for s in mapping['stories']]==['request-96419-68463578ae17','request-96423-9ef90d0b74d4','request-96436-719ce450900e']
assert [s['category'] for s in mapping['stories']]==['story','story','service']
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':3,'completion':1}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/ixarkon.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':15,'Q':3}
by_line={b['line']:b for b in raw if 'binding' in b}
for line,giver,inputs,outputs,daily in (
    (20,96419,[('I',96431)],[('C',25000),('E',50000)],True),
    (63,96423,[('I',96434)],[('E',5500),('I',96435)],True),
    (116,96436,[('I',96414),('C',1000000)],[('I',96434),('E',50000)],False),
):
    b=by_line[line];assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(giver,inputs,outputs,False)
    d=next(r['definition'] for r in ixarkon['requests'] if r['block']['line']==line)
    assert (d['zone_number'],d['source_area'],d['daily_eligible'])==(964,'ixarkon',daily)
    if not daily:assert d['daily_exclusion']=='Unsupported durable offering'
assert [s['contracts'] for s in mapping['stories']]==[[by_line[n]['binding']] for n in (20,63,116)]
assert mapping['stories'][1]['steps'][0]['contracts']==mapping['stories'][2]['contracts']
assert [t['item_vnums'] for s in mapping['stories'] for t in s['steps'] if t['kind']=='carried_item']==[[96431],[96434],[96414]]
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in mapping['stories'])
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==964]
assert len(units)==3 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==2
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert sum(len(c['topics']) for c in contacts.values())==22
for b in ixarkon['dialogue']:assert set(b['body'][0].rstrip('~').split())<=set(contacts[b['giver_vnum']]['topics'])
for v,c in contacts.items():assert c['keyword'] in ixarkon['mobs'][v]['keywords']
assert contacts[96436]['topics'][-1]=='cost' and contacts[96423]['keyword']=='pacer'
assert 'currently unavailable' in mapping['stories'][2]['summary'] and '1,000 platinum' in mapping['stories'][2]['summary']
rooms=dawndale_bodies('ixarkon','wld');objects=dawndale_bodies('ixarkon','obj');mobiles=dawndale_bodies('ixarkon','mob')
assert set(rooms)==set(range(96400,96601)) and set(objects)==set(range(96400,96444)) and set(mobiles)==set(range(96400,96451))
assert len({tuple(b.split('~')[:2]) for b in rooms.values()})==198
assert len({tuple(b.split('~')[2].splitlines()[1].split()) for b in rooms.values()})==15
assert sum(len(re.findall(r'^E\s*$',b,re.M)) for b in rooms.values())==1 and re.search(r'^E\s*$',rooms[96486],re.M)
assert (ixarkon['zone']['first_vnum'],ixarkon['zone']['last_vnum'],ixarkon['zone']['reset_mode'])==(96296,96600,2)
assert {(a['kind'],a['vnum'],a['function']) for a in ixarkon['special_assignments']}=={('mob',96449,'money_changer'),('room',96549,'pet_shops'),('obj',96402,'illithid_teleport_veil'),('room',96537,'inn')}
assert not any(int(b.split('~')[2].split()[1])&(1<<19) for b in rooms.values())
assert {v for v,b in mobiles.items() if int(b.split('~')[4].split()[0])&(1<<15)}=={96450}
exits=[]
for v,b in rooms.items():
    for d,desc,key,flags,door_key,target in re.findall(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S):exits.append((v,int(d),int(flags),int(door_key),int(target)))
assert len(exits)==468 and len(set(exits))==468
assert not any(v==96524 or t==96524 for v,d,f,k,t in exits)
for row in ((96423,1,0,0,96428),(96428,3,0,0,96423),(96583,2,5,0,96584),(96584,0,5,0,96583),(96588,1,4,0,4503),(96474,0,0,0,15412),(96471,2,0,0,828097),(96600,1,0,0,54238),(96600,7,0,0,96470)):
    assert row in exits,row
for v,values in ((96400,[340,96423,1,1]),(96401,[340,96428,3,1])):
    assert objvalues(objects[v])[0]==29 and objvalues(objects[v])[11:15]==values
assert objvalues(objects[96402])[0]==13 and objvalues(objects[96431])[0]==12 and objvalues(objects[96431])[6]&4096
assert not objvalues(objects[96414])[6]&4096 and not objvalues(objects[96434])[6]&4096
assert len(ixarkon['reset_commands'])==385 and collections.Counter(r['command'] for r in ixarkon['reset_commands'])=={'D':36,'O':6,'M':280,'E':34,'G':29}
assert len({(r['command'],tuple(r['arguments'])) for r in ixarkon['reset_commands']})==302
parent=None;families=[];sources=[];placements=[]
for r in ixarkon['reset_commands']:
    c,v=r['command'],r['arguments']
    if c=='M':parent=(c,tuple(v));placements.append((v[1],v[3]))
    families.append((c,tuple(v),parent if c in ('G','E') else None))
    if c in ('G','E') and v[1] in (96414,96431):sources.append((c,v[1],v[2],parent[1][1],parent[1][3]))
assert len(set(families))==323
assert sources==[('E',96414,1,96422,96500),('G',96431,1,96444,96597)]
assert sum(v==96422 and r==96500 for v,r in placements)==2 and sum(v==96444 for v,r in placements)==6
assert not any(v==96429 for v,r in placements)
assert {(v,r) for v,r in placements if v in (96419,96420,96423,96436,96450)}=={(96419,96446),(96420,96455),(96423,96500),(96436,96584),(96450,96537)}
assert not any(r['command'] in ('O','G','E') and r['arguments'][1]==96434 for r in ixarkon['reset_commands'])
assert {r['arguments'][3] for r in ixarkon['reset_commands'] if r['command']=='O' and r['arguments'][1]==3097}=={96524,96584}
assign=(ROOT/'src/specs/specs.assign.c').read_text(encoding='utf8')
assert re.search(r'world\[real_room0\(19890\)\]\.funct = GithyankiCave;',assign)
assert not any(a['vnum']==96443 for a in inventory_module.special_assignments(assign))
veil=(ROOT/'src/specs/specs.ixarkon.c').read_text(encoding='utf8')
targets=list(map(int,re.search(r'to_room\[MAX_SQUID_ROOM \+ 1\] = \{([^}]+)',veil,re.S)[1].replace(',',' ').split()))
assert targets==[2368,3404,4108,4109,4437,6900,11545,12528,12535,12536,12540,12541,15273,19022,19275,23805,23812,25458,25459,25484,36171,96563,96569,96803,96909]
assert 'str_cmp(" veil", arg)' in veil and 'IS_ILLITHID' not in veil and 'do_restore(ch, GET_NAME(ch), -4)' in veil
target_areas=('myconid','flind','caves_skelenak','underworld','worms','ghore','ethereal_main','faang','goblincave','highmoor','plane_earth_one','plane_fire_one','arac_wild','ixarkon','dirkn','troll_caves')
target_rooms={v:b for area in target_areas for v,b in dawndale_bodies(area,'wld').items() if v in targets}
assert set(target_rooms)==set(targets)
for z in catalog['zones']:
    p=ROOT/'areas/wld'/f"{z['source_area']}.wld"
    if p.is_file():assert not re.search(r'^#19890\s*$',p.read_text(encoding='utf8'),re.M)
quest=(ROOT/'src/world/quest.c').read_text(encoding='utf8')
assert 'goal->goal_type != QUEST_GOAL_ITEM' in quest and 'This quest cannot accept offerings right now.' in quest
services=(ROOT/'src/specs/specs.room.c').read_text(encoding='utf8')
assert 'Pet purchases are unavailable while active accounting is enabled.' in services and 'Pet rentals are unavailable while active accounting is enabled.' in services
assert 'Paid locker services are unavailable while active accounting is enabled.' in (ROOT/'src/item/storage_lockers.c').read_text(encoding='utf8')
assert len(re.findall(r'^#\d+~\s*$',(ROOT/'areas/shp/ixarkon.shp').read_text(encoding='utf8'),re.M))==5


# Du'Maathe: exact two-input/three-output recipe, optional histories and loaded gate states.
mntcastl=inventory_module.area_evidence(ROOT,'mntcastl')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='mntcastl')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==8 and len(mapping['contacts'])==15 and not mapping['exclusions']
assert [s['category'] for s in mapping['stories']]==['story']*4+['service']*4
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':11,'completion':3}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/mntcastl.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':13,'Q':8}
by_line={b['line']:b for b in raw if 'binding' in b}
for line,giver,inputs,outputs in (
    (70,37100,[('I',37103)],[('C',200000)]),
    (83,37100,[('I',37105)],[('C',200000)]),
    (137,37100,[('I',37104)],[('I',37100)]),
    (105,37100,[('I',37114)],[('I',37115)]),
    (237,37102,[('I',37106),('I',97903)],[('I',37105)]*3),
    (255,37102,[('I',37108)],[('I',37109)]),
    (272,37102,[('I',37110)],[('I',37111)]),
    (286,37102,[('I',37121)],[('I',37122)]),
):
    b=by_line[line];assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(giver,inputs,outputs,False)
    d=next(r['definition'] for r in mntcastl['requests'] if r['block']['line']==line)
    assert (d['zone_number'],d['source_area'],d['daily_eligible'])==(371,'mntcastl',True)
assert [s['contracts'] for s in mapping['stories']]==[[by_line[n]['binding']] for n in (70,83,137,105,237,255,272,286)]
assert mapping['stories'][1]['steps'][0]['contracts']==mapping['stories'][4]['contracts']
assert mapping['stories'][3]['steps'][0]['contracts']==mapping['stories'][2]['contracts']
assert [t['item_vnums'] for s in mapping['stories'] for t in s['steps'] if t['kind']=='carried_item']==[[37103],[37105],[37112],[37104],[37100],[37114],[37106],[97903],[37108],[37110],[37121]]
assert all(t.get('optional') for s in mapping['stories'] for t in s['steps'][:-1])
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in mapping['stories'])
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==371]
assert len(units)==8 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==4
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert sum(len(c['topics']) for c in contacts.values())==15
assert len(mntcastl['dialogue'])==12
for b in mntcastl['dialogue']:assert set(b['body'][0].rstrip('~').split())<=set(contacts[b['giver_vnum']]['topics'])
assert not any('qc_action80' in c['topics'] for c in contacts.values())
for v,c in contacts.items():assert c['keyword'] in mntcastl['mobs'][v]['keywords']
rooms=dawndale_bodies('mntcastl','wld');objects=dawndale_bodies('mntcastl','obj');mobiles=dawndale_bodies('mntcastl','mob')
assert set(rooms)==set(range(37100,37606)) and set(mobiles)==set(range(37100,37192))
assert set(objects)==set(range(37100,37148))|{37346}
assert len({tuple(b.split('~')[:2]) for b in rooms.values()})==126
assert len({tuple(b.split('~')[2].splitlines()[1].split()) for b in rooms.values()})==20
assert not any(re.search(r'^[EFT]\s*$',b,re.M) for b in rooms.values())
assert (mntcastl['zone']['first_vnum'],mntcastl['zone']['last_vnum'],mntcastl['zone']['reset_mode'])==(37100,37605,1)
assert {(a['kind'],a['vnum'],a['function']) for a in mntcastl['special_assignments']}=={('room',37313,'inn'),('room',37434,'inn')}
assert not any(int(b.split('~')[2].split()[1])&(1<<19) for b in rooms.values())
assert not any(int(b.split('~')[4].split()[0])&(1<<15) for b in mobiles.values())
exits=[];texts=set()
for v,b in rooms.items():
    for d,desc,kw,f,k,t in re.findall(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S):
        exits.append((v,int(d),int(f),int(k),int(t)));texts.add((desc,kw))
assert len(exits)==len(set(exits))==1224 and len(texts)==206
assert {(v,d,t) for v,d,f,k,t in exits if t not in rooms}=={(37416,3,563532),(37574,2,15968)}
assert {(v,d,k,t) for v,d,f,k,t in exits if k}=={(37288,2,37112,37291),(37346,0,37346,37347),(37384,1,37100,37385),(37385,3,37100,37384)}
resets=mntcastl['reset_commands'];assert len(resets)==419
assert collections.Counter(r['command'] for r in resets)=={'D':12,'O':3,'M':352,'E':35,'F':5,'G':12}
assert len({(r['command'],tuple(r['arguments'])) for r in resets})==303
families=[];parent=leader=None
for r in resets:
    c,v=r['command'],tuple(r['arguments'])
    if c=='M':leader=parent=(c,v)
    if c=='F':families.append((c,v,leader));parent=(c,v)
    else:families.append((c,v,parent if c in ('E','G') else None))
assert len(set(families))==312
doors={(r['arguments'][1],r['arguments'][2]):r['arguments'][3] for r in resets if r['command']=='D'}
assert doors[(37346,0)]==doors[(37347,2)]==2
assert doors[(37353,0)]==doors[(37356,2)]==5 and doors[(37384,1)]==6 and doors[(37385,3)]==2
assert (37117,5) not in doors and (37523,4) not in doors
loader=(ROOT/'src/world/db.c').read_text(encoding='utf8')
assert re.search(r'state\s*&=\s*3;',loader) and 'if (ZCMD.arg3 & 0x04)' in loader
assert all(int(rooms[v].split('~')[2].split()[2])==0 for v in range(37453,37458))
values={v:objvalues(b) for v,b in objects.items()}
assert {v:values[v][12] for v in (37100,37112,37115,37346)}=={37100:0,37112:100,37115:100,37346:100}
assert values[37140][0]==3 and values[37110][0]==5
assert values[37105][11:15]==[50,28,9,1] and values[37122][11:15]==[30,141,53,1]
assert values[37121][6]&4096
def loaded(kind):
    result=[];parent=None
    for r in resets:
        if r['command']=='M':parent=(r['arguments'][1],r['arguments'][3])
        if r['command'] in ('G','E') and r['arguments'][1]==kind:result.append((r['command'],parent,r['arguments'][2]))
    return result
assert loaded(37106)==[('E',(37101,37457),1)]
assert loaded(37108)==[('G',(37134,37548),1)]
assert loaded(37104)==[('G',(37105,37342),1)]
assert loaded(37112)==[('G',(37106,37288),1)]
assert loaded(37114)==[('G',(37108,37388),1)]
assert loaded(37110)==[('E',(37120,37346),1)] and loaded(37346)==[('G',(37120,37346),1)]
assert not loaded(37103)
assert [r['arguments'][3] for r in resets if r['command']=='O' and r['arguments'][1]==37121]==[37506]
blocks=inventory_module.native_blocks(ROOT)
assert not any(b['kind'] in ('Q','QA') and ('I',37103) in b['receive'] for b in blocks)
hermit=next(b for b in blocks if b['source']=='areas/qst/surfacemini.qst' and b['line']==33)
assert hermit['give']==[('I',28553),('I',28552)] and hermit['receive']==[('I',97903)]
assert mapping['stories'][4]['steps'][0]['contracts']==[hermit['binding']]
pod=[b for b in blocks if b['source']=='areas/qst/pods.qst' and b['line'] in (187,199)]
assert len(pod)==2 and pod[0]['give']==[('I',28554)]*6+[('I',28555),('C',1000)] and pod[1]['give']==[('I',28555)]*3+[('C',500)]
foreign=next(b for b in blocks if b['source']=='areas/qst/alatorin.qst' and b['line']==6323)
assert foreign['giver_vnum']==83385 and foreign['give']==[('I',37117)] and foreign['receive']==[('I',83495)]
epic=(ROOT/'src/classes/epic_skills.c').read_text(encoding='utf8')
assert '{ 37145, SKILL_SMELT, 0, 100, 0, 0, 0 }' in epic and 'Epic skill purchases are unavailable while economic accounting is active.' in epic
assert 'mob_index[real_mobile(epic_teachers[i].vnum)].func.mob = epic_teacher;' in (ROOT/'src/world/epic.c').read_text(encoding='utf8')
assert len(re.findall(r'^#\d+~\s*$',(ROOT/'areas/shp/mntcastl.shp').read_text(encoding='utf8'),re.M))==1
guidance=' '.join(mapping['orientation']+[s['summary'] for s in mapping['stories']]+[t['hint'] for s in mapping['stories'] for t in s['steps']])
for phrase in ('three','consumed','supplied','unresolved','no SEARCH','gardener belt','currently unavailable','active, ready accounting'):
    assert phrase.lower() in guidance.lower(),phrase
assert catalog_module.report_for(catalog)['mapped_area_count']==167
assert catalog_module.report_for(catalog)['daily_unit_count']==1408
assert sum(catalog_module.report_for(catalog)['eligible_by_zone'].values())==1522


# Tundra: ALL materials, partial foreign supply, retiring daily and automatic mirror.
tundra=inventory_module.area_evidence(ROOT,'tundra')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='tundra')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==7 and len(mapping['contacts'])==16 and not mapping['exclusions']
assert [s['category'] for s in mapping['stories']]==['story']*6+['service']
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':12,'completion':2}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/tundra.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':13,'Q':7}
by_line={b['line']:b for b in raw if 'binding' in b}
for line,giver,inputs,outputs,retire in (
    (40,13703,[('I',13708),('I',13709),('I',13710),('I',13711)],[('I',13713)],False),
    (109,13716,[('I',13713)],[('I',13712)],False),
    (120,13716,[('I',13722)],[('I',13720)],True),
    (139,13722,[('I',43138)],[('C',85000),('E',65000)],False),
    (144,13722,[('I',43137)],[('C',85000),('E',55000)],False),
    (157,13723,[('I',334),('I',318),('I',319)],[('I',13705)],False),
    (66,13710,[('I',13714),('C',500000)],[('I',13715)],False),
):
    b=by_line[line];assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(giver,inputs,outputs,retire)
    d=next(r['definition'] for r in tundra['requests'] if r['block']['line']==line)
    assert (d['zone_number'],d['source_area'],d['daily_eligible'])==(137,'tundra',line!=66)
assert [s['contracts'] for s in mapping['stories']]==[[by_line[n]['binding']] for n in (40,109,120,139,144,157,66)]
assert mapping['stories'][1]['steps'][0]['contracts']==mapping['stories'][0]['contracts']
assert mapping['stories'][5]['steps'][0]['contracts'][0]['giver_vnum']==29444
assert [t['item_vnums'] for s in mapping['stories'] for t in s['steps'] if t['kind']=='carried_item']==[[13708],[13709],[13710],[13711],[13713],[13722],[43138],[43137],[334],[318],[319],[13714]]
assert all(t.get('optional') for s in mapping['stories'] for t in s['steps'][:-1])
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in mapping['stories'])
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==137]
assert len(units)==7 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==6
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert sum(len(c['topics']) for c in contacts.values())==16
assert len(tundra['dialogue'])==12 and not contacts[13725]['topics']
for b in tundra['dialogue']:assert set(b['body'][0].rstrip('~').split())<=set(contacts[b['giver_vnum']]['topics'])
for v,c in contacts.items():assert c['keyword'] in tundra['mobs'][v]['keywords']
rooms=dawndale_bodies('tundra','wld');objects=dawndale_bodies('tundra','obj');mobiles=dawndale_bodies('tundra','mob')
assert set(rooms)==set(range(13700,13736))|set(range(13740,13836))|set(range(13859,13871))|{13889,13926}|set(range(13891,13913))|set(range(13932,13939))
assert set(mobiles)==set(range(13700,13726)) and set(objects)==set(range(13700,13731))|{13755}
assert len({tuple(b.split('~')[:2]) for b in rooms.values()})==64
assert len({tuple(b.split('~')[2].splitlines()[1].split()) for b in rooms.values()})==19
assert [(v,len(re.findall(r'^E\s*$',b,re.M))) for v,b in rooms.items() if re.search(r'^E\s*$',b,re.M)]==[(13835,1)]
assert not any(re.search(r'^[FT]\s*$',b,re.M) for b in rooms.values())
assert (tundra['zone']['first_vnum'],tundra['zone']['last_vnum'],tundra['zone']['reset_mode'])==(13600,13938,2)
assert {(a['kind'],a['vnum'],a['function']) for a in tundra['special_assignments']}=={('room',13714,'inn')}
assert not any(int(b.split('~')[2].split()[1])&(1<<19) for b in rooms.values())
assert not any(int(b.split('~')[4].split()[0])&(1<<15) for b in mobiles.values())
exits=[];texts=set()
for v,b in rooms.items():
    for d,desc,kw,f,k,t in re.findall(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S):
        exits.append((v,int(d),int(f),int(k),int(t)));texts.add((desc,kw))
assert len(exits)==len(set(exits))==401 and len({x[1:] for x in exits})==390 and len(texts)==9
assert {(v,d,t) for v,d,f,k,t in exits if t not in rooms}=={(13703,3,538336),(13712,2,514280),(13733,2,30264),(13778,1,538746),(13796,3,542729),(13869,2,-1),(13870,3,523898),(13893,1,110038),(13895,1,543120),(13936,3,543909)}
assert {(v,d,k,t) for v,d,f,k,t in exits if k}=={(13751,0,13701,13742),(13808,3,13705,13817),(13809,0,13730,13725)}
assert {(d,t) for v,d,f,k,t in exits if v==13835}=={(0,13834),(1,13870)}
resets=tundra['reset_commands'];assert len(resets)==246
assert collections.Counter(r['command'] for r in resets)=={'D':36,'O':8,'M':180,'G':16,'E':6}
assert len({(r['command'],tuple(r['arguments'])) for r in resets})==197
families=[];parent=None
for r in resets:
    c,v=r['command'],tuple(r['arguments'])
    if c=='M':parent=(c,v)
    families.append((c,v,parent if c in ('E','G') else None))
assert len(set(families))==197
doors={(r['arguments'][1],r['arguments'][2]):r['arguments'][3] for r in resets if r['command']=='D'}
assert doors[(13716,0)]==doors[(13728,2)]==1
assert doors[(13751,0)]==8 and doors[(13742,2)]==5 and doors[(13761,2)]==doors[(13762,0)]==1
assert doors[(13808,3)]==doors[(13809,0)]==2 and doors[(13817,1)]==doors[(13725,2)]==1
loader=(ROOT/'src/world/db.c').read_text(encoding='utf8')
assert re.search(r'state\s*&=\s*3;',loader) and 'if (ZCMD.arg3 & 0x04)' in loader and 'if (ZCMD.arg3 & 0x08)' in loader
assert 'obj_index[nr].func.obj = item_switch;' in loader
values={v:objvalues(b) for v,b in objects.items()}
assert values[13701][0]==29 and values[13701][11:15]==[270,13751,0,1]
assert {v:values[v][12] for v in (13705,13730)}=={13705:100,13730:100}
assert all(values[v][6]&4096 for v in (13708,13709,13710,13711,13714,13722,13730))
assert values[13726][11:16]==[30,66,65,64,10] and values[13727][11:16]==[50,1,14,43,0]
assert all(int(rooms[v].split('~')[2].split()[2])==10 for v in range(13729,13735))
assert int(rooms[13819].split('~')[2].split()[2])==4
def loaded(kind):
    result=[];parent=None
    for r in resets:
        if r['command']=='M':parent=(r['arguments'][1],r['arguments'][3])
        if r['command'] in ('G','E') and r['arguments'][1]==kind:result.append((r['command'],parent,r['arguments'][2]))
    return result
assert loaded(13708)==[('G',(13706,13752),1)] and loaded(13709)==[('G',(13704,13750),1)]
assert loaded(13710)==[('G',(13705,13751),1)] and loaded(13711)==[('G',(13707,13713),1)]
assert loaded(13714)==[('G',(13701,13753),1)] and loaded(13722)==[('G',(13717,13911),1)]
assert loaded(13730)==[('G',(13725,13809),1)] and not loaded(13713)
assert len([r for r in resets if r['command']=='M' and r['arguments'][1] in (13704,13705,13706)])==6
assert {(r['arguments'][1],r['arguments'][3]) for r in resets if r['command']=='O'}=={(13704,13712),(13718,13713),(13700,13714),(13717,13735),(13701,13751),(13703,13754),(13755,13764),(13728,13938)}
shop=(ROOT/'areas/shp/tundra.shp').read_text(encoding='utf8')
assert len(re.findall(r'^#\d+',shop,re.M))==1 and '13719' in shop and '13724' in shop
all_blocks=inventory_module.native_blocks(ROOT)
bom=next(b for b in all_blocks if b['source']=='areas/qst/harrow.qst' and b['line']==114)
assert bom['give']==[('I',29440)] and bom['receive']==[('I',318),('I',319),('I',330)] and not bom['disappear']
assert mapping['stories'][5]['steps'][0]['contracts']==[bom['binding']]
heavens=dawndale_bodies('heavens','obj')
assert [inventory_module.plain(heavens[v].split('~')[1]).strip() for v in (318,319,334,330)]==['a large pike fish','an impressive lobster','a small clam','a large hairy crab']
interesting=set(objects)|{318,319,334,43137,43138}
foreign=[b for b in all_blocks if b['source']!='areas/qst/tundra.qst' and b['kind'] in ('Q','QA') and any(k=='I' and v in interesting for k,v in b['give']+b['receive'])]
assert len(foreign)==148 and len({(b['source'],b['giver_vnum'],tuple(b['body'])) for b in foreign})==22
assert [(b['source'],b['line']) for b in foreign if any(k=='I' and v in objects for k,v in b['give']+b['receive'])]==[('areas/qst/shipy.qst',730)]
assert next(b for b in foreign if b['source']=='areas/qst/shipy.qst' and b['line']==198)['receive']==[('I',334)]
assert next(b for b in foreign if b['source']=='areas/qst/shipy.qst' and b['line']==487)['receive']==[('I',318)]
shipy=dawndale_bodies('shipy','obj');assert 'snapjaw turtle shell' in shipy[43137] and 'fire gland' in shipy[43138]
tradeskill=(ROOT/'src/economy/tradeskill.c').read_text(encoding='utf8')
assert re.search(r'const int fishes\[12\]\s*=\s*\{\s*293, 294, 295, 318, 319, 330, 332, 333, 334, 335, 355, 356\s*\}',tradeskill)
assert 'if (!IS_WATER_ROOM(ch->in_room))' in tradeskill and 'grant_tradeskill_item(ch, fish);' in tradeskill
assert 'economic_source_kind::crafting' in tradeskill
assert not re.search(r'^Duris3\s', (ROOT/'areas/AREA').read_text(encoding='utf8'),re.M)


# Fields Between: shared mithril, distinct heads, restored fixed portal and roaming mother.
import subprocess
fields_between=inventory_module.area_evidence(ROOT,'fields_between')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='fields_between')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,2,'complete')
assert len(mapping['stories'])==7 and len(mapping['contacts'])==22 and not mapping['exclusions']
assert all(s['category']=='story' for s in mapping['stories'])
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':16,'completion':1}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/fields_between.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':10,'Q':7}
by_line={b['line']:b for b in raw if 'binding' in b}
for line,giver,inputs,outputs,retire in (
 (10,71036,[('I',71007),('I',71008)],[('I',71020)],False),
 (42,71037,[('I',71005),('I',71016)],[('I',71024)],False),
 (95,71038,[('I',71010),('I',71011),('I',71012),('I',71013),('I',71014)],[('I',71017)],False),
 (125,71040,[('I',71030)],[('I',71033)],False),
 (147,71056,[('I',71022),('I',71021)],[('I',71023)],False),
 (190,71065,[('I',71021)],[('I',71027)],True),
 (222,71066,[('I',71027)],[('I',71028)],False),
):
 b=by_line[line];assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(giver,inputs,outputs,retire)
 d=next(r['definition'] for r in fields_between['requests'] if r['block']['line']==line)
 assert (d['zone_number'],d['source_area'],d['daily_eligible'])==(710,'fields_between',True)
assert [s['contracts'] for s in mapping['stories']]==[[by_line[n]['binding']] for n in (10,42,95,125,147,190,222)]
assert mapping['stories'][6]['steps'][0]['contracts']==mapping['stories'][5]['contracts']
assert [t['item_vnums'] for s in mapping['stories'] for t in s['steps'] if t['kind']=='carried_item']==[[71007],[71008],[71005],[71016],[71003],[71026],[71010],[71011],[71012],[71013],[71014],[71030],[71022],[71021],[71021],[71027]]
assert all(t.get('optional') for s in mapping['stories'] for t in s['steps'][:-1])
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in mapping['stories'])
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==710]
assert len(units)==sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==7
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert len(fields_between['dialogue'])==10 and sum(len(c['topics']) for c in contacts.values())==81
for b in fields_between['dialogue']:assert set(b['body'][0].rstrip('~').split())<=set(contacts[b['giver_vnum']]['topics'])
for v,c in contacts.items():assert c['keyword'] in fields_between['mobs'][v]['keywords']
rooms=dawndale_bodies('fields_between','wld');objects=dawndale_bodies('fields_between','obj');mobiles=dawndale_bodies('fields_between','mob')
assert set(rooms)==set(range(71001,71154)) and set(mobiles)==set(range(71001,71069)) and set(objects)==set(range(71001,71034))
assert len({tuple(b.split('~')[:2]) for b in rooms.values()})==77
assert len({tuple(b.split('~')[2].splitlines()[1].split()) for b in rooms.values()})==9
assert not any(re.search(r'^[EFT]\s*$',b,re.M) for b in rooms.values())
assert (fields_between['zone']['first_vnum'],fields_between['zone']['last_vnum'],fields_between['zone']['reset_mode'])==(71000,71153,2)
assert not fields_between['special_assignments'] and not (ROOT/'areas/shp/fields_between.shp').exists()
assert not any(int(b.split('~')[2].split()[1])&(1<<19) for b in rooms.values())
assert not any(int(b.split('~')[4].split()[0])&(1<<15) for b in mobiles.values())
assert not (int(mobiles[71066].split('~')[4].split()[0])&66)
exits=[];texts=set()
for v,b in rooms.items():
 for d,desc,kw,f,k,t in re.findall(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S):
  exits.append((v,int(d),int(f),int(k),int(t)));texts.add((desc,kw))
assert len(exits)==len(set(exits))==len({x[1:] for x in exits})==428 and len(texts)==16
assert {(v,d,t) for v,d,f,k,t in exits if t not in rooms}=={(71001,0,629085),(71152,5,71265)}
assert {(v,d,f,k,t) for v,d,f,k,t in exits if k}=={(71107,2,2,71003,71108),(71108,0,2,71003,71107),(71127,2,3,71026,71128),(71128,0,3,71026,71127)}
assert {(d,t) for v,d,f,k,t in exits if v==71153}=={(d,71153) for d in range(6)}
assert not re.search(r'\bD\d',rooms[71104])
resets=fields_between['reset_commands'];assert len(resets)==286
assert collections.Counter(r['command'] for r in resets)=={'D':22,'O':6,'M':230,'G':14,'E':11,'F':3}
assert len({(r['command'],tuple(r['arguments'])) for r in resets})==273
parents=[];parent=None;leader=None
for r in resets:
 c,a=r['command'],tuple(r['arguments'])
 if c=='M':leader=parent=(c,a)
 if c=='F':parents.append((c,a,leader));parent=(c,a)
 else:parents.append((c,a,parent if c in ('G','E') else None))
assert len(set(parents))==273
source_pairs={(c,a[1],p[1][1],p[1][3]) for c,a,p in parents if c in ('G','E')}
assert {('G',71003,71027,71107),('E',71005,71028,71114),('G',71016,71041,71119),('G',71026,71044,71127),('G',71011,71047,71122),('G',71010,71046,71123),('G',71012,71048,71125),('G',71013,71049,71126),('G',71014,71050,71128),('G',71022,71055,71132),('G',71007,71035,71136),('G',71008,71052,71139),('G',71021,71051,71149),('G',71009,71038,71151),('G',71032,71068,71153)}<=source_pairs
assert {p[1][1] for c,a,p in parents if c=='F'}=={71038}
assert next(a for c,a,p in parents if c=='G' and a[1]==71021)==(1,71021,1,0,33,0,0,0)
assert next(a for c,a,p in parents if c=='M' and a[1]==71065)==(0,71065,1,71104,10,0,0,0)
assert {(a[1],a[2],a[3]) for c,a,p in parents if c=='D' and a[1] in (71045,71046,71107,71108,71127,71128)}=={(71045,1,5),(71046,3,5),(71107,2,2),(71108,0,2),(71127,2,2),(71128,0,2)}
assert {v for v,b in objects.items() if objvalues(b)[0]==25}=={71001,71002,71030,71031}
for v,destination in ((71001,71104),(71002,71014),(71030,71153),(71031,71001)):
 assert objvalues(objects[v])[11:15]==[destination,7,-1,0]
for v in (71003,71026):assert objvalues(objects[v])[0]==18 and objvalues(objects[v])[12]==0
assert objvalues(objects[71005])[0]==37 and objvalues(objects[71005])[7]&512
assert objvalues(objects[71008])[0]==objvalues(objects[71016])[0]==13
assert all(objvalues(objects[v])[6]&4096 for v in range(71010,71015))
subprocess.run([sys.executable,str(ROOT/'tests/async/test_fields_between_portable_rift.py')],check=True)
foreign=list(inventory_module.native_blocks(ROOT))
for area,line,giver,inputs,outputs in (
 ('scorchvalley',113,71236,[('I',71227),('I',71009),('I',28980)],[('I',71226)]),
 ('juiblex',177,87518,[('I',71024)],[('I',87594)]),
 ('wh',3082,55213,[('I',55435)],[('I',55362),('C',1000000),('I',55033)]),
):
 b=next(b for b in foreign if b['source']==f'areas/qst/{area}.qst' and b['line']==line)
 assert (b['giver_vnum'],b['give'],b['receive'])==(giver,inputs,outputs)
assert any(b['giver_vnum']==87611 and b['disappear'] and b['give']==[('I',87609)] and b['receive']==[('I',87610)] for b in foreign)
assert not any(b['giver_vnum']==87600 and b['kind'] in ('M','Q','QA') for b in foreign)
assert '0 0 71140' in dawndale_bodies('scorchvalley','wld')[71331]
assert '0 0 71106' in dawndale_bodies('juiblex','wld')[87671]
assert '0 0 71128' in dawndale_bodies('wh','wld')[55633]
assert '0 0 71001' in dawndale_bodies('surface','wld')[629085]
assert '0 0 71152' not in dawndale_bodies('scorchvalley','wld')[71265]



# Moregeeth: optional keys/history, four exact components, trapped letter and paid crafts.
import subprocess
goblinht=inventory_module.area_evidence(ROOT,'goblinht')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='goblinht')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,2,'complete')
assert len(mapping['stories'])==7 and len(mapping['contacts'])==21 and len(mapping['exclusions'])==2
assert collections.Counter(s['category'] for s in mapping['stories'])=={'story':5,'service':2}
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':14,'completion':1}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/goblinht.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':8,'Q':6,'MA':1,'QA':5}
by_line={b['line']:b for b in raw if 'binding' in b}
for line,giver,inputs,outputs,retire,daily in (
 (8,70000,[('I',70030)],[('I',70028),('C',10000)],False,True),
 (28,70001,[('I',70001)],[('I',70002)],True,True),
 (61,70022,[('I',70021)],[('I',70022)],False,True),
 (70,70022,[('I',70013),('I',70014),('I',70015),('I',70016)],[('I',70017)],True,True),
 (84,70022,[('I',70093),('C',10000)],[('I',70094)],False,False),
 (98,70023,[('C',5)],[('I',70018)],False,False),
 (105,70031,[('I',70026)],[('I',70026)],False,False),
 (119,70060,[('I',70030)],[('I',70030)],False,False),
 (124,70060,[('I',70065)],[('I',70066)],False,True),
 (144,70078,[('I',70075)]*5+[('C',500)],[('I',70074)],False,False),
 (157,70104,[('C',5)],[('I',70018)],False,False),
):
 b=by_line[line];assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(giver,inputs,outputs,retire)
 d=next(r['definition'] for r in goblinht['requests'] if r['block']['line']==line)
 assert (d['zone_number'],d['source_area'],d['daily_eligible'])==(700,'goblinht',daily)
assert [s['contracts'] for s in mapping['stories']]==[[by_line[n]['binding']] for n in (8,28,84,70,61,124,144)]
assert {c['completion_key'] for x in mapping['exclusions'] for c in x['contracts']}=={by_line[n]['binding']['completion_key'] for n in (98,105,119,157)}
assert mapping['stories'][3]['steps'][0]['contracts']==mapping['stories'][4]['contracts']
assert [t['item_vnums'] for t in mapping['stories'][3]['steps'] if t['kind']=='carried_item']==[[70022],[70013],[70014],[70015],[70016]]
assert mapping['stories'][6]['steps'][0]['count']==5 and mapping['stories'][5]['steps'][0]['item_vnums']==[412,70060,70028]
assert all(t.get('optional') for s in mapping['stories'] for t in s['steps'][:-1])
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in mapping['stories'])
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==700]
assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(7,5,5)
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert len(goblinht['dialogue'])==9 and sum(len(c['topics']) for c in contacts.values())==22
for b in goblinht['dialogue']:assert set(b['body'][0].rstrip('~').split())<=set(contacts[b['giver_vnum']]['topics'])
for v,c in contacts.items():assert c['keyword'] in goblinht['mobs'][v]['keywords']
rooms=dawndale_bodies('goblinht','wld');objects=dawndale_bodies('goblinht','obj');mobiles=dawndale_bodies('goblinht','mob')
assert (len(rooms),len(mobiles),len(objects))==(353,109,96)
assert set(rooms)==set(range(70000,70353)) and set(mobiles)==set(range(70000,70109)) and set(objects)==set(range(70000,70096))
assert (goblinht['zone']['first_vnum'],goblinht['zone']['last_vnum'],goblinht['zone']['reset_mode'])==(69294,70352,2)
assert {(x['kind'],x['vnum'],x['function']) for x in goblinht['special_assignments']}=={('mob',70029,'world_quest'),('room',70175,'inn')}
assert objvalues(objects[70028])[0]==31 and objvalues(objects[70021])[0]==10 and objvalues(objects[70013])[0]==3
assert objvalues(objects[70019])[12]==objvalues(objects[70022])[12]==objvalues(objects[70057])[12]==0
assert not any(objvalues(b)[0]==29 for b in objects.values())
for v,dest,cmd in ((70006,70076,139),(70007,70168,3),(70008,70168,270),(70009,70168,139),(70010,70153,7),(70011,70168,320),(70012,70101,7),(70090,70124,320)):
 vals=objvalues(objects[v]);assert vals[0]==25 and vals[7]==0 and vals[11:15]==[dest,cmd,-1,0]
interpreter=(ROOT/'src/cmd/interp.h').read_text(encoding='utf8')
for name,number in [('STARE',139),('TOUCH',320),('ENTER',7),('PUSH',270),('SOUTH',3)]:assert re.search(rf'^#define CMD_{name}\s+{number}$',interpreter,re.M)
resets=goblinht['reset_commands'];assert len(resets)==549
assert collections.Counter(r['command'] for r in resets)=={'D':94,'O':17,'P':7,'M':306,'E':37,'G':72,'F':16}
assert len({(r['command'],tuple(r['arguments'])) for r in resets})==442
parents={};last_m=None
for r in resets:
 if r['command']=='M':last_m=r
 if r['command'] in ('G','E'):parents[r['line']]=(r,last_m)
for line,item,actor,room in ((411,70001,70000,70001),(462,70016,70021,70073),(475,70013,70017,70100),(488,70014,70014,70127),(501,70015,70011,70154),(550,70030,70034,70177),(614,70057,70058,70200),(660,70093,70072,70211),(794,70019,70006,70333)):
 r,m=parents[line];assert (r['arguments'][1],m['arguments'][1],m['arguments'][3])==(item,actor,room)
for line in (600,602,604,606,608):
 r,m=parents[line];assert (r['arguments'][1],m['arguments'][1],m['arguments'][3])==(70075,70057,70196)
assert not any(r['command']=='G' and m['arguments'][1]==70103 and r['arguments'][1]==70075 for r,m in parents.values())
assert any(r['command']=='P' and r['arguments'][1:4]==[70021,1,70020] for r in resets)
assert any(r['command']=='P' and r['arguments'][1:4]==[70065,1,70064] for r in resets)
assert len(re.findall(r'^#\d+~', (ROOT/'areas/shp/goblinht.shp').read_text(encoding='utf8'),re.M))==10
subprocess.run([sys.executable,str(ROOT/'tests/async/test_moregeeth_letter_desk.py')],check=True)
assert 'STARE flame' in mapping['orientation'][6] and 'TOUCH crystal' in mapping['orientation'][6]
assert 'lockpick' in mapping['stories'][0]['summary'] and 'potion' in mapping['stories'][4]['summary']



# Ceothia: one guild choice, independent Lenbrea rewards, paired crates and legacy training.
ceothia=inventory_module.area_evidence(ROOT,'ceothia')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='ceothia')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==6 and len(mapping['contacts'])==17 and not mapping['exclusions']
assert all(s['category']=='story' for s in mapping['stories'])
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':14,'completion':3}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/ceothia.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':12,'Q':8,'QA':1}
by_line={b['line']:b for b in raw if 'binding' in b}
for line,giver,inputs,outputs,retire in (
 (20,80801,[('I',80806),('I',80810),('I',80811)],[('I',80803),('I',80813)],True),
 (60,80802,[('I',80805),('I',80810),('I',80811)],[('I',80813),('I',80814)],True),
 (152,80803,[('I',80813)],[('I',80815),('C',500000)],False),
 (185,80803,[('I',81410)],[('C',500000),('I',80830)],False),
 (202,80803,[('I',81423)],[('I',80832)],False),
 (239,80807,[('I',80811),('I',80805),('I',80806)],[('I',80813),('I',80817)],True),
 (274,80808,[('I',80805),('I',80806),('I',80810)],[('I',80813),('I',80818)],True),
 (305,80875,[('I',80826)]*2,[('C',200000)],False),
 (341,80907,[('I',32490),('I',26614),('I',402)],[('I',404)],False),
):
 b=by_line[line];assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(giver,inputs,outputs,retire)
 d=next(r['definition'] for r in ceothia['requests'] if r['block']['line']==line)
 assert (d['zone_number'],d['source_area'],d['daily_eligible'])==(808,'ceothia',True)
stories={s['id']:s for s in mapping['stories']}
guild=stories['choose-surviving-thief-guild']
assert guild['contracts']==[by_line[n]['binding'] for n in (20,60,239,274)]
assert [t['item_vnums'] for t in guild['steps'][:-1]]==[[80805],[80806],[80810],[80811]]
for sid,line in [('lenbrea-ceothian-badge',152),('lenbrea-flickering-dragon-horn',185),('lenbrea-thread-of-time',202),('merchant-two-oaken-crates',305),('captain-legacy-dexterity-scroll',341)]:
 assert stories[sid]['contracts']==[by_line[line]['binding']]
assert stories['lenbrea-ceothian-badge']['steps'][0]['contracts']==guild['contracts']
for sid in ('lenbrea-flickering-dragon-horn','lenbrea-thread-of-time'):
 assert stories[sid]['steps'][0]['contracts']==stories['lenbrea-ceothian-badge']['contracts']
 assert stories[sid]['steps'][1]['item_vnums']==[80815]
assert stories['merchant-two-oaken-crates']['steps'][1]['count']==2
assert all(t.get('optional') for s in mapping['stories'] for t in s['steps'][:-1])
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in mapping['stories'])
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==808]
assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(6,6,6)
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert len(ceothia['dialogue'])==9 and sum(len(c['topics']) for c in contacts.values())==22
for b in ceothia['dialogue']:assert set(b['body'][0].rstrip('~').split())<=set(contacts[b['giver_vnum']]['topics'])
for v,c in contacts.items():assert c['keyword'] in inventory_mobs[v]['keywords']
assert contacts[80907]['topics']==[] and contacts[80862]['topics']==[]
assert [b['body'][0] for b in raw if b['giver_vnum']==80907 and b['kind']=='M']==['qc_action 45~','qc_action 45~','qc_action 48~']
rooms=dawndale_bodies('ceothia','wld');objects=dawndale_bodies('ceothia','obj');mobiles=dawndale_bodies('ceothia','mob')
assert (len(rooms),len(mobiles),len(objects))==(295,110,33)
assert set(rooms)==set(range(80800,81095)) and set(mobiles)==set(range(80800,80910)) and set(objects)==set(range(80800,80833))
assert (ceothia['zone']['first_vnum'],ceothia['zone']['last_vnum'],ceothia['zone']['reset_mode'])==(80789,81094,2)
assert len({tuple(b.split('~')[:2]) for b in rooms.values()})==107
assert len({b.split('~')[2].splitlines()[1] for b in rooms.values()})==7
assert len({b.split('~')[3] for b in mobiles.values()})==88
exits=[m for b in rooms.values() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(exits)==619 and len({(m[2],m[3]) for m in exits})==3
assert not any(re.search(r'^[EFT]\s*$',b,re.M) for b in rooms.values())
assert {(x['kind'],x['vnum'],x['function']) for x in ceothia['special_assignments']}=={
 ('obj',80830,'ogre_warlords_sword'),('room',81070,'inn'),('room',81028,'inn'),
 ('room',81019,'inn'),('room',81078,'inn'),('room',81003,'inn'),('room',81021,'crew_shop_proc')}
assert not any(int(b.split('~')[2].split()[1])&(1<<19) for b in rooms.values())
assert not any(objvalues(b)[0]==29 for b in objects.values())
assert objvalues(objects[80816])[0]==25 and objvalues(objects[80816])[11:15]==[81100,7,-1,0]
assert objvalues(objects[80826])[0]==12 and objvalues(objects[80826])[7]&1
assert objvalues(objects[80815])[0]==objvalues(objects[80832])[0]==18
assert objvalues(objects[80815])[12]==objvalues(objects[80832])[12]==100
assert objvalues(objects[80827])[12]==15
assert objvalues(objects[80824])[0]==15 and objvalues(objects[80824])[11:14]==[10000,15,80827]
# CONT_HARDPICK bit2 is compatibility-only; PICKPROOF is bit16, not bit2.
assert not objvalues(objects[80824])[12]&16 and objvalues(objects[80824])[7]==0
for v,d,f,key,target in ((80943,1,5,0,80998),(80998,3,1,0,80943),(80959,0,5,0,80999),(80999,2,1,0,80959),(80980,3,3,80815,80981),(80981,1,7,80815,80980),(80980,0,3,80832,81094),(81094,2,3,80832,80980)):
 assert re.search(rf'\bD{d}\s+[^~]*~[^~]*~\s*{f} {key} {target}\b',rooms[v],re.S)
resets=ceothia['reset_commands'];assert len(resets)==510
assert collections.Counter(r['command'] for r in resets)=={'D':10,'O':11,'P':2,'M':327,'E':155,'G':5}
assert len({(r['command'],tuple(r['arguments'])) for r in resets})==299
parent=None;families=[];parents={}
for r in resets:
 if r['command']=='M':parent=(r['command'],tuple(r['arguments']))
 families.append((r['command'],tuple(r['arguments']),parent if r['command'] in ('E','G') else None))
 if r['command'] in ('E','G'):parents[r['line']]=(r,parent)
assert len(set(families))==386
for line,item,actor,room in ((189,80806,80802,80923),(229,80805,80801,80935),(400,80810,80807,80998),(416,80811,80808,81001),(102,80827,80872,80845)):
 r,parent=parents[line];assert (r['arguments'][1],parent[1][1],parent[1][3])==(item,actor,room)
assert [r['arguments'] for r in resets if r['command']=='P']==[[1,80826,2,80824,100,0,0,0]]*2
assert any(r['command']=='O' and r['arguments'][1:4]==[80824,1,80845] for r in resets)
assert any(r['command']=='M' and r['arguments'][1:4]==[80875,1,81088] for r in resets)
assert any(r['command']=='O' and r['arguments'][1:4]==[62,1,81094] for r in resets)
assert len(re.findall(r'^#\d+~',(ROOT/'areas/shp/ceothia.shp').read_text(encoding='utf8'),re.M))==1
future=dawndale_bodies('ceofutur','obj');past=dawndale_bodies('ceopast','obj');heavens=dawndale_bodies('heavens','obj')
for b,target in ((past[81111],81400),(future[81413],80980),(future[81422],81676)):
 assert objvalues(b)[0]==25 and objvalues(b)[11:15]==[target,7,-1,0]
assert objvalues(future[81410])[0]==4 and objvalues(future[81423])[0]==13
assert objvalues(heavens[402])[0]==objvalues(heavens[404])[0]==objvalues(heavens[62])[0]==13
assert re.search(r'^T\s*\n6 2 1 50\b',dawndale_bodies('bctdl','obj')[32490],re.M)
future_resets=(ROOT/'areas/zon/ceofutur.zon').read_text(encoding='utf8')
assert re.search(r'M 0 81429 1 81674[^\n]*\nG 1 81410',future_resets)
assert re.search(r'M 0 81454 1 81676[^\n]*\nG 1 81423',future_resets)
assert 'O 0 81413 1 81677' in future_resets and 'O 0 81422 1 81670' in future_resets
all_blocks=inventory_module.native_blocks(ROOT)
assert not any(('I',402) in b.get('receive',[]) for b in all_blocks)
assert not any(re.search(r'^[OGEP] \d+ 402\b',p.read_text(encoding='utf8'),re.M) for p in (ROOT/'areas/zon').glob('*.zon'))
assign=(ROOT/'src/specs/specs.assign.c').read_text(encoding='utf8')
assert 'obj_index[real_object0(62)].func.obj = stat_pool_agi;' in assign
assert not re.search(r'func\.obj\s*=\s*skill_beacon\b',assign)
pool=(ROOT/'src/specs/specs.heavens.c').read_text(encoding='utf8')
pool=pool[pool.index('static int stat_pool_common('):pool.index('int spell_pool(')]
assert 'GET_LEVEL(ch) < 51' in pool and '60 * 60 * 24 * 2' in pool and 'TAG_POOL' in pool
assert 'BOUNDED(1, (*statPtr) + numb, 100)' in pool and 'GET_HIT(ch) = GET_MAX_HIT(ch);' in pool
epics=(ROOT/'src/classes/epic_skills.c').read_text(encoding='utf8')
assert '{ 80862, SKILL_TOUGHNESS' in epics and '{ 80907, SKILL_EPIC_DEXTERITY' in epics
assert 'Epic skill purchases are unavailable while economic accounting is active.' in epics
assert 'fixed, closed and locked; ordinary lockpicking or knock' in stories['merchant-two-oaken-crates']['steps'][0]['hint']



# Braddistock1350: preserve canonical ownership while explaining the physical Tower story.
brad=inventory_module.area_evidence(ROOT,'brad')
brad_mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='brad')
assert (brad_mapping['schema_version'],brad_mapping['revision'],brad_mapping['coverage'])==(3,2,'complete')
assert len(brad_mapping['stories'])==5 and len(brad_mapping['contacts'])==16 and not brad_mapping['exclusions']
assert all(s['category']=='story' for s in brad_mapping['stories'])
assert collections.Counter(t['kind'] for s in brad_mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':9,'completion':1}
brad_blocks=[b for b in inventory_module.native_blocks(ROOT) if 134142<=b['giver_vnum']<=135048]
assert collections.Counter(b['kind'] for b in brad_blocks)=={'M':9,'Q':5}
assert {b['source'] for b in brad_blocks}=={'areas/qst/lortower.qst'}
brad_by_line={b['line']:b for b in brad_blocks if 'binding' in b}
brad_ids=['request-134146-a4f6aa87c7b1','request-134150-cc22370abf33','request-134162-0aa9fcadf7a5','request-134167-d86a832b706b','request-134169-05c42346bab0']
assert [s['id'] for s in brad_mapping['stories']]==brad_ids
for story,(line,giver,inputs,outputs,retire) in zip(brad_mapping['stories'],(
 (201,134146,[('I',134105)],[('I',134106)],True),
 (218,134150,[('I',134006)],[('E',100000)],False),
 (266,134162,[('I',134131),('I',134132),('I',134133),('I',134134),('I',134135)],[('I',134125)],True),
 (292,134167,[('I',134144)],[('I',134145)],False),
 (304,134169,[('I',134048)],[('I',134144)],False),
)):
 b=brad_by_line[line];assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(giver,inputs,outputs,retire)
 assert story['contracts']==[b['binding']] and story['steps'][-1]['contracts']==story['contracts']
 d=next(r['definition'] for r in brad['requests'] if r['block']['line']==line)
 assert (d['zone_number'],d['source_area'],d['daily_eligible'])==(1350,'brad',True)
 assert 'Tower of Darkness' in story['summary']
assert brad_mapping['stories'][3]['steps'][0]['contracts']==brad_mapping['stories'][4]['contracts']
assert [t['item_vnums'] for t in brad_mapping['stories'][2]['steps'][:-1]]==[[134131],[134132],[134133],[134134],[134135]]
assert all(t.get('optional') for s in brad_mapping['stories'] for t in s['steps'][:-1])
assert not any(s['steps'][-1].get('optional') for s in brad_mapping['stories'])
brad_units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==1350]
assert (len(brad_units),sum(u['achievement'] for u in brad_units),sum(u['daily_candidate'] for u in brad_units))==(5,5,5)
brad_contacts={c['mob_vnum']:c for c in brad_mapping['contacts']}
assert sum(len(c['topics']) for c in brad_contacts.values())==32
for b in brad['dialogue']:assert set(b['body'][0].rstrip('~').split())<=set(brad_contacts[b['giver_vnum']]['topics'])
for v,c in brad_contacts.items():assert c['keyword'] in inventory_mobs[v]['keywords']
for v in (134150,135014,135003,135008,135009):assert brad_contacts[v]['topics']==[]
brad_rooms=dawndale_bodies('brad','wld');brad_mobs=dawndale_bodies('brad','mob');brad_objects=dawndale_bodies('brad','obj')
assert (len(brad_rooms),len(brad_mobs),len(brad_objects))==(48,14,73)
assert set(brad_rooms)==set(range(135001,135049)) and set(brad_mobs)==set(range(135001,135015)) and set(brad_objects)==set(range(135001,135074))
assert (brad['zone']['first_vnum'],brad['zone']['last_vnum'],brad['zone']['reset_mode'])==(134142,135048,2)
assert len({tuple(b.split('~')[:2]) for b in brad_rooms.values()})==47
assert len({b.split('~')[2].splitlines()[1] for b in brad_rooms.values()})==5
assert len({b.split('~')[3] for b in brad_mobs.values()})==14
brad_exits=[x for b in brad_rooms.values() for x in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(brad_exits)==99 and len({(x[2],x[3]) for x in brad_exits})==83
brad_extras=[x.groups() for b in brad_rooms.values() for x in re.finditer(r'^E\s*\n([^~]*)~([^~]*)~',b,re.M)]
assert len(brad_extras)==26 and len(set(brad_extras))==25
assert not any(re.search(r'^[FT]\b',b,re.M) for b in brad_rooms.values())
assert not any(re.search(r'^T\b',b,re.M) for b in brad_objects.values())
assert not any(objvalues(b)[0] in (25,29) for b in brad_objects.values())
assert not (ROOT/'areas/qst/brad.qst').exists() and not (ROOT/'areas/shp/brad.shp').exists()
assert not any(int(b.split('~')[2].split()[1])&(1<<19) for b in brad_rooms.values())
assert {(x['kind'],x['vnum'],x['function']) for x in brad['special_assignments']}=={('mob',135014,'braddistock')}
assert objvalues(brad_objects[135003])[0]==15 and objvalues(brad_objects[135003])[12:14]==[5,-1]
assert objvalues(brad_objects[135040])[0]==15 and objvalues(brad_objects[135040])[6]&32768 and not objvalues(brad_objects[135040])[6]&(32|4096)
assert objvalues(brad_objects[135046])[0]==15 and objvalues(brad_objects[135046])[12:14]==[13,135050]
assert objvalues(brad_objects[135052])[0]==22 and objvalues(brad_objects[135033])[0]==objvalues(brad_objects[135034])[0]==8
assert re.search(r'\bD2\s+[^~]*~[^~]*~\s*2 135016 135027\b',brad_rooms[135026],re.S)
assert re.search(r'\bD0\s+[^~]*~[^~]*~\s*1 135016 135026\b',brad_rooms[135027],re.S)
brad_resets=brad['reset_commands'];assert len(brad_resets)==175
assert collections.Counter(r['command'] for r in brad_resets)=={'D':10,'O':74,'P':26,'M':48,'E':17}
assert len({(r['command'],tuple(r['arguments'])) for r in brad_resets})==116
parent=None;families=[]
for r in brad_resets:
 if r['command']=='M':parent=(r['command'],tuple(r['arguments']))
 families.append((r['command'],tuple(r['arguments']),parent if r['command'] in ('E','G','F') else None))
assert len(set(families))==125
assert sum(r['command']=='P' and r['arguments'][3]==135024 for r in brad_resets)==9
assert any(r['command']=='P' and r['arguments'][1:4]==[135004,1,135003] for r in brad_resets)
assert any(r['command']=='P' and r['arguments'][1]==135042 and r['arguments'][3]==135040 for r in brad_resets)
assert not any(r['command'] in ('O','P','G','E') and r['arguments'][1] in (135050,135071,135073) for r in brad_resets)
brad_tower_objects=dawndale_bodies('lortower','obj')
assert [objvalues(brad_tower_objects[v])[0] for v in (134105,134006,134048,134125,134144,134145)]==[4,12,12,12,13,9]
assert all(objvalues(brad_tower_objects[v])[0]==12 for v in range(134131,134136))
brad_tower_resets=(ROOT/'areas/zon/lortower.zon').read_text(encoding='utf8')
for giver,room in ((134146,134112),(134150,134127),(134162,134138),(134167,134140),(134169,134042)):
 assert re.search(rf'^M 0 {giver} 1 {room} 100\b',brad_tower_resets,re.M)
for source,item,room in ((134004,134006,134010),(134040,134131,134018),(134062,134132,134029),(134081,134048,134039),(134081,134133,134039),(134091,134134,134046),(134133,134135,134122)):
 assert re.search(rf'^M 0 {source} 1 {room}[^\n]*\n(?:(?!M |O ).*\n)*?G 1 {item} 1 ',brad_tower_resets,re.M)
assert re.search(r'^M 0 134148 1 134114[^\n]*\nE 1 134105 1 18 ',brad_tower_resets,re.M)
assert inventory_module.area_evidence(ROOT,'lortower')['zone']['reset_mode']==0
assert 'obj_index[real_object0(1372)].func.obj = jet_black_maul;' in (ROOT/'src/specs/specs.assign.c').read_text(encoding='utf8')
assert 'send_to_char(' in (ROOT/'src/specs/specs.braddistock.c').read_text(encoding='utf8')
assert (ROOT/'tests/async/test_braddistock_entry.py').exists()


# Venan'Trut: exact eight outcomes, computed services and source/access boundaries.
desert=inventory_module.area_evidence(ROOT,'desert')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='desert')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==8 and len(mapping['contacts'])==22 and not mapping['exclusions']
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':10,'completion':1}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/desert.qst']
assert collections.Counter(b['kind'] for b in raw)=={'MA':1,'M':7,'Q':8}
by_line={b['line']:b for b in raw if 'binding' in b}
expected=((10,49020,49072,49099),(27,49061,49171,49042),(43,49087,49057,49058),(58,49099,49148,49167),(74,49155,49079,49098),(91,49161,49173,55371),(115,49220,49027,49170),(131,49223,49063,49173))
for line,giver,item,reward in expected:
 b=by_line[line];assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(giver,[('I',item)],[('I',reward)],False)
 s=next(s for s in mapping['stories'] if s['contracts']==[b['binding']]);assert s['category']=='story'
 assert s['steps'][-2]['item_vnums']==[item] and s['steps'][-1]['contracts']==s['contracts']
 assert all(t.get('optional') for t in s['steps'][:-1]) and not s['steps'][-1].get('optional')
 d=next(r['definition'] for r in desert['requests'] if r['block']['line']==line);assert (d['zone_number'],d['daily_eligible'])==(490,True)
stories={s['id']:s for s in mapping['stories']}
assert stories['eriic-lost-medallion']['steps'][0]['contracts']==stories['goranon-queen-royal-garb']['contracts']
assert stories['eriic-lost-medallion']['steps'][1]['item_vnums']==[49063]
assert stories['wizard-vernadad-signet']['steps'][0]['item_vnums']==[49106]
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert sum(len(c['topics']) for c in contacts.values())==46
for b in desert['dialogue']:assert b['body'][0].rstrip('~').split()==contacts[b['giver_vnum']]['topics']
for v,c in contacts.items():assert c['keyword'] in inventory_mobs[v]['keywords']
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==490]
assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(8,8,8)
rooms=dawndale_bodies('desert','wld');objects=dawndale_bodies('desert','obj');mobiles=dawndale_bodies('desert','mob')
assert (len(rooms),len(mobiles),len(objects))==(677,245,194)
assert (min(rooms),max(rooms))==(49000,49790) and set(mobiles)==set(range(49000,49245)) and set(objects)==set(range(49000,49194))
assert (desert['zone']['first_vnum'],desert['zone']['last_vnum'],desert['zone']['reset_mode'])==(49000,49790,1)
assert len({tuple(b.split('~')[:2]) for b in rooms.values()})==158
assert len({b.split('~')[2].strip().splitlines()[0] for b in rooms.values()})==35
assert len({tuple(b.split('~')[:4]) for b in mobiles.values()})==234
exits=[m for b in rooms.values() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(exits)==1710 and len({(m[2],m[3]) for m in exits})==16
assert {v:int(re.search(r'^F\s*\n(\d+)',b,re.M)[1]) for v,b in rooms.items() if re.search(r'^F\s*\n(\d+)',b,re.M)}=={49191:75,49192:50,49193:25}
assert not any(re.search(r'^[ET]\s*$',b,re.M) for b in rooms.values())
assert {(x['kind'],x['vnum'],x['function']) for x in desert['special_assignments']}=={('mob',49064,'world_quest'),('room',49051,'crew_shop_proc'),('room',49090,'ship_shop_proc')}
assert {v for v,b in rooms.items() if int(b.split('~')[2].split()[1])&(1<<19)}=={49000,49034,49051}
assert not any(int(b.split('~')[4].split()[0])&32768 for b in mobiles.values())
assert not any(re.search(r'^T\s*$',b,re.M) for b in objects.values())
assert objvalues(objects[49012])[0]==29 and objvalues(objects[49012])[11:15]==[270,49154,0,0]
assert sum(objvalues(b)[0]==25 for b in objects.values())==16
for v,target,command in ((49000,49567,7),(49001,49211,7),(49007,49151,7),(49010,49152,7),(49013,49153,7),(49022,49005,7),(49023,49200,7),(49036,49279,7),(49062,49556,7),(49082,49338,7),(49084,49681,7),(49087,49682,7),(49168,49382,7),(49174,49396,320),(49175,49023,320),(49176,49787,7)):
 assert objvalues(objects[v])[11:15]==[target,command,-1,0] and objvalues(objects[v])[7]==0
assert 49681 not in rooms and 49682 not in rooms
assert objvalues(objects[49170])[0]==11 and objvalues(objects[49170])[7]&1
assert objvalues(objects[49173])[0]==9 and objvalues(objects[49063])[0]==9
for v,d,f,key,target in ((49268,0,3,49106,49269),(49004,7,3,49107,49106),(49016,5,5,0,49272),(49272,4,5,0,49016),(49154,0,0,0,49155)):
 assert re.search(rf'\bD{d}\s+[^~]*~[^~]*~\s*{f} {key} {target}\b',rooms[v],re.S)
assert objvalues(objects[49163])[11:15]==[2999,29,49165,2999]
# First river is inbound; the trench's second portal is the return to River's End.
for v,d,target in ((49005,2,49007),(49007,2,49009),(49009,2,49196),(49196,1,49197),(49197,2,49198),(49198,2,49199),(49199,1,49200),(49200,0,49201),(49201,4,49202)):
 assert re.search(rf'\bD{d}\s+[^~]*~[^~]*~\s*0 0 {target}\b',rooms[v],re.S)
assert 'second river portal at the trench returns to the River’s End' in ' '.join(mapping['orientation'])

resets=desert['reset_commands'];assert len(resets)==1256
assert collections.Counter(r['command'] for r in resets)=={'D':124,'O':42,'P':2,'M':814,'E':168,'F':25,'G':81}
assert len({(r['command'],tuple(r['arguments'])) for r in resets})==942
parent=None;families=[];parents={}
for r in resets:
 if r['command']=='M':parent=(r['command'],tuple(r['arguments']))
 families.append((r['command'],tuple(r['arguments']),parent if r['command'] in ('E','G','F') else None))
 if r['command'] in ('E','G'):parents[r['line']]=(r,parent)
assert len(set(families))==964
for line,item,actor,room in ((901,49072,49142,49147),(1302,49171,49221,49538),(1156,49057,49116,49418),(1598,49148,49170,49677),(955,49027,49024,49202),(1437,49063,49121,49607),(1044,49106,49171,49268),(718,55285,49187,49054),(717,359,49187,49054)):
 r,parent=parents[line];assert (r['arguments'][1],parent[1][1],parent[1][3])==(item,actor,room)
for item,room in ((49012,49150),(49079,49272),(49022,49195),(49023,49201),(49174,49023),(49175,49396)):
 assert any(r['command']=='O' and r['arguments'][1]==item and r['arguments'][3]==room for r in resets)
assert any(r['command']=='D' and r['arguments'][1:4]==[49154,0,8] for r in resets)
assert not any(r['command']=='O' and r['arguments'][1] in (49082,49084,49087,49108,49111) for r in resets)
assert len(re.findall(r'^#\d+~',(ROOT/'areas/shp/desert.shp').read_text(encoding='utf8'),re.M))==10
assert not any(r['command'] in ('E','G') and r['arguments'][1]==49173 for r in resets)
all_blocks=inventory_module.native_blocks(ROOT)
for giver,item in ((43135,49179),(43167,49179),(55242,55371),(55242,55285),(55133,55332)):
 assert any(b['giver_vnum']==giver and ('I',item) in b.get('give',[]) for b in all_blocks)
assert not any(('I',49173) in b.get('receive',[]) and b['giver_vnum']!=49223 for b in all_blocks)
assign=(ROOT/'src/specs/specs.assign.c').read_text(encoding='utf8');assert 'obj_index[real_object0(359)].func.obj = epic_stone;' in assign
assert not re.search(r'func\.obj\s*=\s*skill_beacon\b',assign)
epics=(ROOT/'src/classes/epic_skills.c').read_text(encoding='utf8')
assert '{ 49161, SKILL_IMPROVED_ENDURANCE' in epics and '{ 49162, SKILL_TOTEMIC_MASTERY' in epics
assert 'Epic skill purchases are unavailable while economic accounting is active.' in epics
assert 'not the white-robed figure’s type11 reward' in ' '.join(mapping['orientation'])


# Past Ceothia: alternative pelt payments, exact components and preserved mismatches.
ceopast=inventory_module.area_evidence(ROOT,'ceopast')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='ceopast')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==4 and len(mapping['contacts'])==13 and not mapping['exclusions']
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':10,'completion':1}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/ceopast.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':8,'Q':6}
by_line={b['line']:b for b in raw if 'binding' in b}
expected=((25,81105,[('I',81102)],[('I',81101),('I',81104)],True),(104,81106,[('I',81103),('I',81101),('I',81106)],[('I',81105),('I',81107)],False),(149,81109,[('I',81108)],[('C',500000)],False),(161,81109,[('I',81114)],[('C',500000)],False),(173,81109,[('I',81115)],[('C',500000)],False),(215,81142,[('I',81108)],[('I',81120)],False))
for line,giver,inputs,rewards,depart in expected:
 b=by_line[line];assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(giver,inputs,rewards,depart)
 assert any(b['binding'] in s['contracts'] for s in mapping['stories'])
 d=next(r['definition'] for r in ceopast['requests'] if r['block']['line']==line)
 assert d['zone_number']==811 and d['daily_eligible']==(line!=25)
stories={s['id']:s for s in mapping['stories']}
assert stories['jamael-wolf-pelt-choice']['contracts']==[by_line[n]['binding'] for n in (149,161,173)]
assert stories['jamael-wolf-pelt-choice']['steps'][0]['item_vnums']==[81108,81114,81115]
assert stories['majelle-three-components']['steps'][0]['contracts']==stories['dryad-white-blossom']['contracts']
assert [t['item_vnums'] for t in stories['majelle-three-components']['steps'][1:-1]]==[[81101],[81103],[81106],[81105],[81118],[81117]]
assert [t['item_vnums'] for t in stories['wolfspeed-current-black-pelt']['steps'][:-1]]==[[81108],[81119]]
assert all(t.get('optional') for s in mapping['stories'] for t in s['steps'][:-1])
assert all(s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional') for s in mapping['stories'])
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert sum(len(c['topics']) for c in contacts.values())==21
for giver in (81105,81106,81109,81142):
 assert list(dict.fromkeys(t for b in ceopast['dialogue'] if b['giver_vnum']==giver for t in b['body'][0].rstrip('~').split()))==contacts[giver]['topics']
for v,c in contacts.items():assert c['keyword'] in inventory_mobs[v]['keywords']
assert contacts[81106]['keyword']=='majelle' and 'Marjelle' in contacts[81106]['description']
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==811]
assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(4,4,3)
rooms=dawndale_bodies('ceopast','wld');objects=dawndale_bodies('ceopast','obj');mobiles=dawndale_bodies('ceopast','mob')
assert set(rooms)==set(range(81100,81393)) and set(mobiles)==set(range(81100,81146)) and set(objects)==set(range(81100,81125))
assert (ceopast['zone']['first_vnum'],ceopast['zone']['last_vnum'],ceopast['zone']['reset_mode'])==(81095,81392,0)
assert len({tuple(b.split('~')[:2]) for b in rooms.values()})==38
assert len({b.split('~')[2].strip().splitlines()[0] for b in rooms.values()})==9
assert len({tuple(b.split('~')[:4]) for b in mobiles.values()})==39
exits=[m for b in rooms.values() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(exits)==990 and len({(m[2],m[3]) for m in exits})==13
assert all(int(m[6]) in rooms for m in exits) and not any(re.search(r'^[EFT]\s*$',b,re.M) for b in rooms.values())
assert not ceopast['special_assignments'] and not (ROOT/'areas/shp/ceopast.shp').exists()
assert not any(int(b.split('~')[2].split()[1])&(1<<19) for b in rooms.values())
assert not any(int(b.split('~')[4].split()[0])&32768 for b in mobiles.values())
assert not any(re.search(r'^T\s*$',b,re.M) for b in objects.values())
assert [v for v,b in objects.items() if objvalues(b)[0]==25]==[81111]
assert not any(objvalues(b)[0] in (15,29) for b in objects.values())
assert objvalues(objects[81111])[11:15]==[81400,7,-1,0] and objvalues(objects[81111])[7]==0
for item in (81105,81117,81118):assert objvalues(objects[item])[0]==18 and objvalues(objects[item])[12]==100
assert objvalues(objects[81102])[6]&4096 and objvalues(objects[81102])[7]&1
for item in (81103,81106,81108):assert objvalues(objects[item])[6]&32768
for item in (81103,81106,81108,81114,81115):assert objvalues(objects[item])[8]&32768
assert objvalues(objects[81101])[0]==12 and objvalues(objects[81119])[0]==11 and objvalues(objects[81120])[0]==9
assert objvalues(objects[81104])[0]==objvalues(objects[81107])[0]==10 and objvalues(objects[81116])[0]==3
for v,d,flag,key,target in ((81313,5,7,81105,81373),(81373,4,3,81105,81313),(81374,3,3,81118,81375),(81375,1,3,81118,81374),(81378,5,7,81117,81389),(81389,4,7,81117,81378)):
 assert re.search(rf'\bD{d}\s+[^~]*~[^~]*~\s*{flag} {key} {target}\b',rooms[v],re.S)
for v,d,target in ((81390,0,81392),(81390,1,81392),(81390,2,81391),(81390,3,81391),(81392,0,81391),(81392,1,81363),(81392,2,81391),(81392,3,81391)):
 assert re.search(rf'\bD{d}\s+[^~]*~[^~]*~\s*0 0 {target}\b',rooms[v],re.S)
assert not re.search(r'\bD\d+',rooms[81391]) and not any(int(m[6])==81390 for m in exits)
resets=ceopast['reset_commands'];assert len(resets)==129
assert collections.Counter(r['command'] for r in resets)=={'D':10,'O':6,'M':75,'E':5,'G':9,'F':24}
assert len({(r['command'],tuple(r['arguments'])) for r in resets})==108
parent=None;families=[];parents={}
for r in resets:
 if r['command']=='M':parent=(r['command'],tuple(r['arguments']))
 families.append((r['command'],tuple(r['arguments']),parent if r['command'] in ('E','G','F') else None))
 if r['command'] in ('E','G'):parents[r['line']]=(r,parent)
assert len(set(families))==108
for line,item,actor,room in ((57,81108,81100,81100),(73,81103,81145,81164),(101,81114,81129,81314),(110,81101,81105,81345),(114,81106,81107,81358),(116,81115,81126,81371),(132,81118,81116,81374),(155,81117,81124,81378),(166,81119,81141,81390),(168,81121,81142,81390)):
 r,parent=parents[line];assert (r['arguments'][1],parent[1][1],parent[1][3])==(item,actor,room)
assert {(r['arguments'][1],r['arguments'][3]) for r in resets if r['command']=='O'}=={(81102,81183),(81100,81216),(81111,81389),(81112,81389),(81116,81389),(81113,81389)}
assert [(r['arguments'][1],r['arguments'][2],r['arguments'][3]) for r in resets if r['command']=='D'][4:]==[(81313,5,6),(81373,4,2),(81374,3,2),(81375,1,2),(81378,5,6),(81389,4,6)]
all_blocks=inventory_module.native_blocks(ROOT)
touching=[b for b in all_blocks if b['kind'] in ('Q','QA') and any(k=='I' and v in objects for k,v in b['give']+b['receive'])]
assert len(touching)==6 and all(b['source']=='areas/qst/ceopast.qst' for b in touching)
assert any(('I',81101) in b.get('receive',[]) for b in raw) and not any(('I',81119) in b.get('give',[]) for b in raw)
assert 'skull of a winter wolf' in '\n'.join(by_line[215]['body']) and 'winter wolf\'s' in (ROOT/'areas/qst/ceopast.qst').read_text(encoding='utf8')
assert 'current native recipe accepts the black pelt' in ' '.join(mapping['orientation'])
present_objects=dawndale_bodies('ceothia','obj');present_rooms=dawndale_bodies('ceothia','wld')
assert objvalues(present_objects[80816])[11:15]==[81100,7,-1,0] and objvalues(present_objects[80816])[7]==0
assert re.search(r'\bD3\s+[^~]*~[^~]*~\s*3 80815 80981\b',present_rooms[80980],re.S)
assert objvalues(present_objects[80815])[0]==18 and objvalues(present_objects[80815])[12]==100
assert any(b['giver_vnum']==80803 and b.get('give')==[('I',80813)] and ('I',80815) in b.get('receive',[]) for b in all_blocks)
future_objects=dawndale_bodies('ceofutur','obj');assert objvalues(future_objects[81413])[11:15]==[80980,7,-1,0] and objvalues(future_objects[81413])[7]==0
epics=(ROOT/'src/classes/epic_skills.c').read_text(encoding='utf8');table=epics.split('epic_teacher_skill epic_teachers[] = {',1)[1].split('\n};',1)[0]
assert not any(81100<=int(v)<=81145 for v in re.findall(r'\{\s*(\d+),',table))


# Basin Wastes: exact component selection, grouped sales and non-credit refusal.
basin=inventory_module.area_evidence(ROOT,'basin_wa')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='basin_wa')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==7 and len(mapping['contacts'])==8 and not mapping['exclusions']
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':13,'completion':1}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/basin_wa.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':6,'Q':10}
by_line={b['line']:b for b in raw if 'binding' in b}
expected={35:([('I',34031),('I',34019),('I',34018)],[('I',34028),('E',500000)]),49:([('I',34030),('I',34002)],[('I',34019)]),56:([('I',34030),('I',34001)],[('I',34025)]),63:([('I',34030),('I',34000)],[('I',34026)]),70:([('I',34030),('I',34003)],[('I',34027)]),77:([('I',34000)],[('C',25000)]),82:([('I',34001)],[('C',25000)]),87:([('I',34002)],[('C',25000)]),92:([('I',34003)],[('C',25000)]),97:([('I',34024)],[('I',34024)])}
for line,(inputs,rewards) in expected.items():
 b=by_line[line];assert (b['giver_vnum'],b['give'],b['receive'],b['disappear'])==(34013,inputs,rewards,False)
 assert any(b['binding'] in s['contracts'] for s in mapping['stories'])
 d=next(r['definition'] for r in basin['requests'] if r['block']['line']==line)
 assert d['zone_number']==340 and d['daily_eligible']==(line!=97)
stories={s['id']:s for s in mapping['stories']}
assert stories['witch-part-payment']['contracts']==[by_line[n]['binding'] for n in (77,82,87,92)]
assert stories['witch-part-payment']['steps'][0]['item_vnums']==[34000,34001,34002,34003]
assert stories['witch-signet-ritual']['steps'][0]['contracts']==stories['witch-light-gland-potion']['contracts']
assert [t['item_vnums'] for t in stories['witch-signet-ritual']['steps'][1:-1]]==[[34031],[34019],[34018]]
assert stories['witch-heartstone-refusal']['category']=='service'
assert all(t.get('optional') for s in mapping['stories'] for t in s['steps'][:-1])
assert all(s['steps'][-1]['contracts']==s['contracts'] for s in mapping['stories'])
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert sum(len(c['topics']) for c in contacts.values())==14
assert contacts[34013]['topics']==list(dict.fromkeys(t for b in basin['dialogue'] for t in b['body'][0].rstrip('~').split()))
for v,c in contacts.items():assert c['keyword'] in inventory_mobs[v]['keywords']
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==340]
assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(7,6,6)
rooms=dawndale_bodies('basin_wa','wld');objects=dawndale_bodies('basin_wa','obj');mobiles=dawndale_bodies('basin_wa','mob')
assert set(rooms)==set(range(34000,34200)) and set(mobiles)==set(range(34000,34015)) and set(objects)==set(range(34000,34032))
assert (basin['zone']['first_vnum'],basin['zone']['last_vnum'],basin['zone']['reset_mode'])==(33765,34199,2)
assert len({tuple(b.split('~')[:2]) for b in rooms.values()})==91
assert len({b.split('~')[2].strip().splitlines()[0] for b in rooms.values()})==8
exits=[(v,m) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(exits)==692 and len({(m[2],m[3]) for v,m in exits})==78
assert len({tuple((int(m[1]),int(m[4]),int(m[5]),int(m[6])-v) for vv,m in exits if vv==v) for v in rooms})==69
assert all(int(m[4])==int(m[5])==0 for v,m in exits)
assert [(v,int(m[1]),int(m[6])) for v,m in exits if int(m[6]) not in rooms]==[(34000,4,612059)]
assert not any(re.search(r'^[EFT]\s*$',b,re.M) for b in rooms.values())
assert not any(int(b.split('~')[2].split()[1])&(1<<19) for b in rooms.values())
assert [(a['kind'],a['vnum'],a['function']) for a in basin['special_assignments']]==[('mob',34014,'block_dir')]
assert ' _block_east_' in mobiles[34014] and len(re.findall(r'\bD\d+',rooms[34199]))==1
assert re.search(r'\bD1\s+[^~]*~[^~]*~\s*0 0 34198\b',rooms[34199],re.S)
assert [v for v,b in mobiles.items() if int(b.split('~')[4].split()[0])&32768]==[34013]
assert not (ROOT/'areas/shp/basin_wa.shp').exists() and not any(re.search(r'^T\s*$',b,re.M) for b in objects.values())
assert not any(objvalues(b)[0] in (18,25,29) for b in objects.values())
for v in range(34011,34017):assert objvalues(objects[v])[0]==15 and objvalues(objects[v])[7]==0 and objvalues(objects[v])[11:15]==[20,0,-1,100]
for v in range(34005,34011):assert objvalues(objects[v])[0]==12 and objvalues(objects[v])[6]&4096
for v in (34000,34001,34002,34003,34018,34019,34024,34030,34031):assert objvalues(objects[v])[8]&32768 and objvalues(objects[v])[7]&1
assert objvalues(objects[34030])[6]&4096 and objvalues(objects[34030])[11:15]==[1,0,0,0]
assert objvalues(objects[34031])[0]==8 and objvalues(objects[34024])[0]==11 and objvalues(objects[34023])[11:15]==[9999,9999,17,0]
assert 'passage1' in objects[34010] and 'passage2' in objects[34010] and 'passage3' in objects[34010]
assert 'canteen' in objects[34021] and objvalues(objects[34021])[0]==9
resets=basin['reset_commands'];assert len(resets)==204
assert len({(r['command'],tuple(r['arguments'])) for r in resets})==151
parent=None;families=[];parents={}
for r in resets:
 if r['command']=='M':parent=(r['command'],tuple(r['arguments']))
 families.append((r['command'],tuple(r['arguments']),parent if r['command'] in ('E','G','F') else None))
 if r['command'] in ('E','G'):parents[r['line']]=(r,parent)
assert len(set(families))==191 and set(r['command'] for r in resets)=={'O','P','M','E','G'}
for line,item,actor,room in ((65,34030,34008,34052),(67,34029,34013,34052),(141,34024,34012,34110),(142,67240,34012,34110),(143,34031,34012,34110)):
 r,p=parents[line];assert (r['arguments'][1],p[1][1],p[1][3])==(item,actor,room)
assert [(r['arguments'][1],p[1][1],p[1][3]) for r,p in parents.values() if r['arguments'][1]==34002]==[(34002,34003,v) for v in range(34185,34195)]
assert {(r['arguments'][1],r['arguments'][3]) for r in resets if r['command']=='O'}=={(34014,34036),(34015,34065),(34011,34082),(34023,34088),(34012,34113),(34013,34150),(34016,34199)}
assert [(r['arguments'][1],r['arguments'][3]) for r in resets if r['command']=='P' and r['arguments'][1] in (34005,34006,34007,34008,34009,34010,34018)]==[(34008,34014),(34009,34015),(34005,34011),(34006,34012),(34018,34013),(34007,34013),(34010,34016)]
all_blocks=inventory_module.native_blocks(ROOT)
touching=[b for b in all_blocks if b['kind'] in ('Q','QA') and any(k=='I' and (v in objects or v==67240) for k,v in b['give']+b['receive'])]
assert len(touching)==10 and all(b['source']=='areas/qst/basin_wa.qst' for b in touching)
surface=dawndale_bodies('surface','wld');assert re.search(r'\bD5\s+[^~]*~[^~]*~\s*0 0 34000\b',surface[612059],re.S)
assert 'Khomani-Khan' in surface[612059] and 'Nizari' in rooms[34008] and 'no magic portal' in rooms[34199]
unique=dawndale_bodies('unique','obj');assert objvalues(unique[67240])[0]==9 and objvalues(unique[67240])[7]==17
quest=(ROOT/'src/world/quest.c').read_text(encoding='utf8')
assert 'qcp->next = quest_index[number_of_quests].quest_complete;' in quest
assert 'if (!matching)\n\t\t\tcontinue;' in quest and 'OBJ_VNUM(offering) == goal->number' in quest
assert 'giving a part chooses its cash sale' in ' '.join(mapping['orientation']).lower()
db=(ROOT/'src/world/db.c').read_text(encoding='utf8');assert 'if (IS_ACT(mob, ACT_TEACHER) && !mob_index[nr].func.mob)' in db
epics=(ROOT/'src/classes/epic_skills.c').read_text(encoding='utf8');table=epics.split('epic_teacher_skill epic_teachers[] = {',1)[1].split('\n};',1)[0]
assert not any(34000<=int(v)<=34014 for v in re.findall(r'\{\s*(\d+),',table))


# Crypt: exact bundle quantities, visually identical prototypes and F-loaded sources.
crypt=inventory_module.area_evidence(ROOT,'crypt')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='crypt')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==5 and len(mapping['contacts'])==9 and not mapping['exclusions']
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':13,'completion':1}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/crypt.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':6,'Q':5}
by_line={b['line']:b for b in raw if 'binding' in b}
expected={35:([('I',14305)]*4+[('I',14315)]*2,[],True),100:([('I',14494),('I',14501),('I',14521),('I',14536),('I',14540)],[('I',14531)],True),128:([('I',14526),('I',14500),('I',14498),('I',14534)],[('I',14539)],False),140:([('I',14539)],[('I',14542)],True),154:([('I',14556)],[('I',14557)],True)}
for line,(inputs,rewards,depart) in expected.items():
 b=by_line[line];assert (b['give'],b['receive'],b['disappear'])==(inputs,rewards,depart)
 assert sum(b['binding'] in s['contracts'] for s in mapping['stories'])==1
 assert next(r['definition'] for r in crypt['requests'] if r['block']['line']==line)['daily_eligible']
stories={s['id']:s for s in mapping['stories']}
assert [(t['item_vnums'],t['count']) for t in stories['hermit-firewood']['steps'][:-1]]==[([14305],4),([14315],2)]
assert [t['item_vnums'] for t in stories['surok-adamantite-gloves']['steps'][:-1]]==[[14494],[14501],[14521],[14536],[14540]]
assert stories['statue-token-bracelet']['steps'][0]['contracts']==stories['statue-four-trophies']['contracts']
assert stories['statue-collar-upgrade']['steps'][0]['item_vnums']==[14556]
assert all(t.get('optional') for s in mapping['stories'] for t in s['steps'][:-1])
assert all(s['steps'][-1]['contracts']==s['contracts'] for s in mapping['stories'])
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={14302,14432,14438,14420,14421,14431,14434,14449,14407}
assert sum(len(c['topics']) for c in contacts.values())==21
for v,c in contacts.items():
 assert c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==list(dict.fromkeys(t for b in crypt['dialogue'] if b['giver_vnum']==v for t in b['body'][0].rstrip('~').split()))
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==143]
assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(5,5,5)
rooms=dawndale_bodies('crypt','wld');objects=dawndale_bodies('crypt','obj');mobiles=dawndale_bodies('crypt','mob')
assert set(rooms)==set(range(14300,14594)) and set(objects)==set(range(14300,14340))|set(range(14400,14561))
assert set(mobiles)==set(range(14300,14308))|set(range(14400,14451))
assert (crypt['zone']['first_vnum'],crypt['zone']['last_vnum'],crypt['zone']['reset_mode'])==(14211,14593,1)
assert len({tuple(b.split('~')[:2]) for b in rooms.values()})==191 and len({b.split('~')[2].strip().splitlines()[0] for b in rooms.values()})==28
exits=[(v,m) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(exits)==653 and len({(m[2],m[3]) for v,m in exits})==113
assert len({tuple((int(m[1]),int(m[4]),int(m[5]),int(m[6])-v) for vv,m in exits if vv==v) for v in rooms})==177
assert [(v,int(m[1]),int(m[6])) for v,m in exits if int(m[6]) not in rooms]==[(14300,3,98737),(14568,4,265588)]
assert [(a['kind'],a['vnum'],a['function']) for a in crypt['special_assignments']]==[('room',14362,'inn')]
assert not any(int(b.split('~')[4].split()[0])&32768 for b in mobiles.values())
assert not (ROOT/'areas/shp/crypt.shp').exists()
switches={14323:[270,14388,5,0],14333:[270,14399,4,0],14420:[340,14411,1,0],14505:[270,14480,0,1]}
assert {v:objvalues(b)[11:15] for v,b in objects.items() if objvalues(b)[0]==29}==switches
assert [(v,objvalues(b)[11:15],objvalues(b)[7]) for v,b in objects.items() if objvalues(b)[0]==25]==[(14547,[14386,320,-1,0],0)]
assert objvalues(objects[14316])[11:15]==[0,5,0,0] and objvalues(objects[14316])[6]&4096
assert len({tuple(objects[v].split('~')[:3]) for v in (14494,14501,14521,14536,14540)})==1
assert objects[14556].split('~')[:3]==objects[14557].split('~')[:3] and objvalues(objects[14556])!=objvalues(objects[14557])
assert objvalues(objects[14480])[11:15]==[150,15,0,250] and objvalues(objects[14480])[7]==0
assert 'page17 17' in objects[14468] and 'page24 24' in objects[14468]
assert all(word in objects[v] for v,word in ((14303,'junamez'),(14457,'anol'),(14532,'unaqzl')))
assert all('junamezanolunaqzl' in rooms[v] for v in (14509,14518))
resets=crypt['reset_commands'];assert len(resets)==521 and set(r['command'] for r in resets)=={'D','O','P','M','E','G','F'}
assert len({(r['command'],tuple(r['arguments'])) for r in resets})==439
leader=None;holder=None;families=[];proof={}
for r in resets:
 if r['command']=='M':leader=(r['command'],tuple(r['arguments']));holder=r
 families.append((r['command'],tuple(r['arguments']),leader if r['command'] in ('E','G','F') else None))
 if r['command']=='F':holder=r
 if r['command'] in ('E','G') and r['arguments'][1] in (14498,14500,14526,14534,14556):proof[r['arguments'][1]]=(holder['command'],holder['arguments'][1],holder['arguments'][3])
assert len(set(families))==447
assert proof=={14498:('F',14420,14466),14500:('F',14421,14471),14526:('F',14431,14513),14534:('F',14434,14527),14556:('M',14449,14535)}
assert [(r['arguments'][1],r['arguments'][3]) for r in resets if r['command']=='O' and r['arguments'][1] in (14494,14501,14536,14540)]==[(14494,14458),(14501,14475),(14540,14539),(14536,14550)]
assert [(r['arguments'][1],r['arguments'][3]) for r in resets if r['command']=='P' and r['arguments'][1]==14521]==[(14521,14480)]
assert [(r['arguments'][1],r['arguments'][3]) for r in resets if r['command']=='D' and r['arguments'][3]&8]==[(14388,8),(14399,8),(14411,8),(14480,8)]
assert len([r for r in resets if r['command']=='O' and r['arguments'][1]==14305])==12 and len([r for r in resets if r['command']=='O' and r['arguments'][1]==14315])==7
assert [(r['arguments'][1],r['arguments'][3]) for r in resets if r['command']=='O' and r['arguments'][1]==14316]==[(14316,14347)]
db=(ROOT/'src/world/db.c').read_text(encoding='utf8')
assert "case 'F': /* follow last mob M loaded */" in db and 'if (!(mob = read_mobile(ZCMD.arg1, REAL)))' in db
assert 'world[room].dir_option[door] = NULL;' in db and 'obj_index[nr].func.obj = item_switch;' in db
speech=(ROOT/'src/cmd/actcomm.c').read_text(encoding='utf8');assert '(EXIT(ch, door)->key == -2)' in speech and 'if (isname(word, arg1))' in speech
switch=(ROOT/'src/specs/specs.object.c').read_text(encoding='utf8');assert 'obj->value[0] != cmd' in switch and 'Nothing happens.' in switch and 'REMOVE_BIT(world[in_room].dir_option[door]->exit_info, EX_BLOCKED)' in switch
epics=(ROOT/'src/classes/epic_skills.c').read_text(encoding='utf8').split('epic_teacher_skill epic_teachers[] = {',1)[1].split('\n};',1)[0]
assert not any(14300<=int(v)<=14450 for v in re.findall(r'\{\s*(\d+),',epics))
touching=[b for b in inventory_module.native_blocks(ROOT) if b['kind'] in ('Q','QA') and any(k=='I' and v in objects for k,v in b['give']+b['receive'])]
assert len(touching)==5 and all(b['source']=='areas/qst/crypt.qst' for b in touching)
assert re.search(r'\bD1\s+[^~]*~[^~]*~\s*0 0 14300\b',dawndale_bodies('unoutpst','wld')[98737],re.S)
heavens=dawndale_bodies('heavens','obj');assert objvalues(heavens[358])[0]==objvalues(heavens[72])[0]==13
assert '_noquest_' in dawndale_bodies('wh','obj')[55274]
assert not any(265588 in dawndale_bodies(z['source_area'],'wld') for z in catalog_module.zone_registry(ROOT))
assert 'fixed glowing orb' in ' '.join(mapping['orientation']) and 'non-takeable' in ' '.join(mapping['orientation'])


# Valois: exact distinct seals, F-held models, a supplied meal and two flower gifts.
val=inventory_module.area_evidence(ROOT,'val')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='val')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==8 and len(mapping['contacts'])==18 and not mapping['exclusions']
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':14,'completion':1}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/val.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':5,'Q':8}
expected={18:([('I',38415),('I',38416),('I',38417)],[('I',38418)],False),38:([('I',38410),('I',38409),('I',38413),('I',38414)],[('I',38420)],True),67:([('I',38430),('I',38431)],[('I',38432)],False),86:([('I',38424)],[('I',38425)],False),96:([('I',38425)],[('I',38428)],False),104:([('I',38442)],[('I',38456)],False),108:([('I',38451)],[('I',38455)],False),124:([('I',38438)],[('I',38439)],False)}
for b in raw:
 if 'binding' not in b:continue
 assert (b['give'],b['receive'],b['disappear'])==expected[b['line']]
 assert sum(b['binding'] in s['contracts'] for s in mapping['stories'])==1
 assert next(r['definition'] for r in val['requests'] if r['block']['line']==b['line'])['daily_eligible']
stories={s['id']:s for s in mapping['stories']}
assert stories['queen-dinner-token']['steps'][0]['contracts']==stories['cook-wine-for-dinner']['contracts']
assert [t['item_vnums'] for t in stories['king-three-family-seals']['steps'][:-1]]==[[38415],[38416],[38417]]
assert [t['item_vnums'] for t in stories['dwarf-four-crafting-models']['steps'][:-1]]==[[38410],[38409],[38413],[38414]]
assert all(t.get('optional') for s in mapping['stories'] for t in s['steps'][:-1])
assert all(s['steps'][-1]['contracts']==s['contracts'] for s in mapping['stories'])
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={38422,38426,38427,38434,38438,38449,38463,38419,38420,38421,38428,38429,38459,38416,38418,38439,38483,38445}
assert sum(len(c['topics']) for c in contacts.values())==26
for v,c in contacts.items():
 assert c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==list(dict.fromkeys(t for b in val['dialogue'] if b['giver_vnum']==v for t in b['body'][0].rstrip('~').split()))
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==384]
assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(8,8,8)
rooms=dawndale_bodies('val','wld');objects=dawndale_bodies('val','obj');mobiles=dawndale_bodies('val','mob')
assert set(rooms)==set(range(38400,38587))-{38415,38421,38486,38563,38565,38566,38568,38569,38576,38577,38578,38580,38585}
assert set(objects)==set(range(38400,38457)) and set(mobiles)==set(range(38400,38493))
assert (val['zone']['first_vnum'],val['zone']['last_vnum'],val['zone']['reset_mode'])==(38384,38586,1)
assert len({tuple(b.split('~')[:2]) for b in rooms.values()})==120 and len({b.split('~')[2].strip().splitlines()[0] for b in rooms.values()})==17
exits=[(v,m) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(exits)==374 and len({(m[2],m[3]) for v,m in exits})==11
assert len({tuple((int(m[1]),int(m[4]),int(m[5]),int(m[6])-v) for vv,m in exits if vv==v) for v in rooms})==145
assert [(v,int(m[1]),int(m[6])) for v,m in exits if int(m[6]) not in rooms]==[(38575,2,521073)]
assert not val['special_assignments'] and not any(int(b.split('~')[4].split()[0])&32768 for b in mobiles.values())
assert not (ROOT/'areas/shp/val.shp').exists()
assert not any(objvalues(b)[0] in (25,29) for b in objects.values())
assert {v:objvalues(b)[11:15] for v,b in objects.items() if objvalues(b)[0]==18}=={38406:[0,100,0,0],38407:[0,100,0,0],38429:[0,100,0,0],38454:[0,0,0,0]}
assert objvalues(objects[38424])[0]==8 and objvalues(objects[38424])[6]&4096
assert objvalues(objects[38425])[0]==13 and objvalues(objects[38438])[0]==13
assert objvalues(objects[38442])[0]==8 and objvalues(objects[38451])[0]==11
assert objvalues(objects[38456])[0]==11 and objvalues(objects[38443])[0]==13 and objvalues(objects[38443])[7]==0
unfinished={v for v,b in rooms.items() if 'You are in an unfinished room.' in b}
assert unfinished==set(range(38508,38518))|{38519,38520,38521,38522,38523}
assert all(int(m[6]) in unfinished for v,m in exits if v in unfinished)
assert not any(int(m[6]) in unfinished for v,m in exits if v not in unfinished)
assert not any(int(m[6]) in {38440,38441} for v,m in exits if v not in {38440,38441})
resets=val['reset_commands'];assert len(resets)==320
assert collections.Counter(r['command'] for r in resets)=={'D':106,'O':5,'M':142,'E':47,'G':18,'F':2}
assert len({(r['command'],tuple(r['arguments'])) for r in resets})==270
leader=None;holder=None;families=[];proof={}
for r in resets:
 if r['command']=='M':leader=(r['command'],tuple(r['arguments']));holder=r
 families.append((r['command'],tuple(r['arguments']),leader if r['command'] in ('E','G','F') else None))
 if r['command']=='F':holder=r
 if r['command'] in ('E','G') and r['arguments'][1] in {38410,38409,38413,38414,38415,38416,38417,38430,38431,38438,38451,38429,38454}:
  proof.setdefault(r['arguments'][1],set()).add((holder['command'],holder['arguments'][1],holder['arguments'][3]))
assert len(set(families))==280
assert proof=={38410:{('F',38445,38439),('M',38416,38487)},38409:{('M',38461,38445),('M',38461,38449),('M',38436,38464),('M',38416,38487)},38413:{('M',38418,38490)},38414:{('M',38418,38490)},38415:{('M',38419,38545)},38416:{('M',38420,38496)},38417:{('M',38421,38525)},38430:{('M',38428,38472)},38431:{('M',38429,38473)},38438:{('M',38459,38454)},38451:{('M',38483,38534)},38429:{('M',38439,38525)},38454:{('M',38475,38484)}}
assert [(r['arguments'][1],r['arguments'][3]) for r in resets if r['command']=='O']==[(38427,38411),(38443,38489),(38442,38495),(38424,38526),(38401,38534)]
assert [(r['arguments'][1],r['arguments'][2],r['arguments'][3]) for r in resets if r['command']=='D' and r['arguments'][1] in (38411,38418,38462,38478)]==[(38411,4,5),(38462,5,5),(38418,4,5),(38478,5,5)]
assert {(r['arguments'][1],r['arguments'][2],r['arguments'][3]) for r in resets if r['command']=='D' and (r['arguments'][1],r['arguments'][2]) in {(38429,2),(38552,0),(38484,0),(38495,2),(38518,2),(38524,0),(38525,5),(38526,4)}}=={(38429,2,2),(38552,0,2),(38484,0,2),(38495,2,2),(38518,2,2),(38524,0,2),(38525,5,2),(38526,4,2)}
assert not any(r['command'] in ('M','F') and r['arguments'][1] in (38478,38491) for r in resets)
db=(ROOT/'src/world/db.c').read_text(encoding='utf8')
assert "case 'F': /* follow last mob M loaded */" in db and 'if (!(mob = read_mobile(ZCMD.arg1, REAL)))' in db
assert 'if (world[room_nr].room_flags & ROOM_INN)' in db and 'world[room_nr].funct = inn;' in db
assert int(rooms[38493].split('~')[2].strip().splitlines()[0].split()[1])&524288
conv=(ROOT/'src/mob/mobconv.c').read_text(encoding='utf8');assert 'isname("_spec1_", GET_NAME(ch))' in conv and '_spec1_' in mobiles[38475]
epics=(ROOT/'src/classes/epic_skills.c').read_text(encoding='utf8').split('epic_teacher_skill epic_teachers[] = {',1)[1].split('\n};',1)[0]
assert not any(38400<=int(v)<=38492 for v in re.findall(r'\{\s*(\d+),',epics))
touching=[b for b in inventory_module.native_blocks(ROOT) if b['kind'] in ('Q','QA') and any(k=='I' and v in objects for k,v in b['give']+b['receive'])]
assert len(touching)==8 and all(b['source']=='areas/qst/val.qst' for b in touching)
assert re.search(r'\bD0\s+[^~]*~[^~]*~\s*0 0 38575\b',dawndale_bodies('surface','wld')[521073],re.S)
foreign=(ROOT/'areas/zon/verspin.zon').read_text(encoding='utf8').splitlines()
assert [' '.join(line.split()[:6]) for line in foreign[369:376]]==['M 0 28113 1 28139 100','E 1 28140 1 15 100','G 1 38421 1 0 100','F 1 28114 4 28139 100','F 1 28114 4 28139 100','F 1 28114 4 28139 100','F 1 28114 4 28139 100']
assert 'switch (ZCMD.arg3 & 0x03)' in db and 'if (ZCMD.arg3 & 0x04)' in db
assert 'secret, closed and unlocked' in ' '.join(mapping['orientation'])
assert 'No magical garden belt' in ' '.join(mapping['orientation'])


# Harrow: token alternatives, actual sources, room echo and fixed travel.
harrow=inventory_module.area_evidence(ROOT,'harrow')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='harrow')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==8 and len(mapping['contacts'])==14 and not mapping['exclusions']
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':16,'completion':4}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/harrow.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':20,'MA':1,'Q':5,'QA':3}
expected={20:([('I',29405)],[('I',29406)],True),46:([('I',29407),('I',29406),('I',29414)],[('I',29416)],False),55:([('I',29406),('I',29410),('I',29415)],[('I',29417)],False),64:([('I',29406),('I',29413),('I',29419)],[('I',29418)],False),73:([('I',29406),('I',29400),('I',29402)],[('I',29404)],False),114:([('I',29440)],[('I',318),('I',319),('I',330)],False),141:([('I',29460)],[('E',4000)],False),162:([('I',29444)],[('I',29453),('C',77777)],True)}
for b in raw:
 if 'binding' not in b:continue
 assert (b['give'],b['receive'],b['disappear'])==expected[b['line']]
 assert sum(b['binding'] in s['contracts'] for s in mapping['stories'])==1
 assert next(r['definition'] for r in harrow['requests'] if r['block']['line']==b['line'])['daily_eligible']
stories={s['id']:s for s in mapping['stories']}
for sid,inputs in [('glass-horn',[29406,29407,29414]),('many-colors-robe',[29406,29410,29415]),('wand-of-light',[29406,29413,29419]),('lucky-alchemist-sack',[29406,29400,29402])]:
 assert stories[sid]['steps'][0]['contracts']==stories['ring-token']['contracts']
 assert [t['item_vnums'][0] for t in stories[sid]['steps'][1:-1]]==inputs
assert all(t.get('optional') for s in mapping['stories'] for t in s['steps'][:-1])
assert all(s['steps'][-1]['contracts']==s['contracts'] for s in mapping['stories'])
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={29418,29421,29444,29449,29454,29422,29428,29434,29438,29440,29450,29404,29429,29406}
assert sum(len(c['topics']) for c in contacts.values())==12
for v,c in contacts.items():
 assert c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==list(dict.fromkeys(t for b in harrow['dialogue'] if b['giver_vnum']==v for t in b['body'][0].rstrip('~').split()))
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==294]
assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(8,8,8)
rooms=dawndale_bodies('harrow','wld');objects=dawndale_bodies('harrow','obj');mobiles=dawndale_bodies('harrow','mob')
assert set(rooms)==set(range(29400,29483))-{29458}
assert set(objects)==set(range(29400,29488)) and set(mobiles)==set(range(29400,29459))
assert (harrow['zone']['first_vnum'],harrow['zone']['last_vnum'],harrow['zone']['reset_mode'])==(29318,29482,2)
assert len({tuple(b.split('~')[:2]) for b in rooms.values()})==65 and len({b.split('~')[2].strip().splitlines()[0] for b in rooms.values()})==21
exits=[(v,m) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(exits)==187 and len({(m[2],m[3]) for v,m in exits})==10
assert len({tuple((int(m[1]),int(m[4]),int(m[5]),int(m[6])-v) for vv,m in exits if vv==v) for v in rooms})==76
assert [(v,int(m[1]),int(m[6])) for v,m in exits if int(m[6]) not in rooms]==[(29400,2,547676),(29400,3,547275)]
lucky=set(range(29477,29482))
assert not any(int(m[6]) in lucky for v,m in exits if v not in lucky)
assert [(v,int(m[1]),int(m[6])) for v,m in exits if v in lucky and int(m[6]) not in lucky]==[(29480,1,29476)]
assert not any(v in (29478,29479,29481) for v,m in exits)
assert {v:objvalues(b)[11:15] for v,b in objects.items() if objvalues(b)[0]==25}=={29425:[29470,7,-1,0],29430:[29471,7,-1,0],29431:[29440,7,-1,0],29441:[29472,7,-1,0],29442:[29443,264,-1,0]}
assert all(objvalues(b)[7]==0 for b in objects.values() if objvalues(b)[0]==25)
assert objvalues(objects[29418])[0]==3 and objvalues(objects[29420])[0]==12
assert objvalues(objects[29405])[6]&4096 and objvalues(objects[29410])[6]&4096 and objvalues(objects[29414])[6]&4096 and objvalues(objects[29419])[6]&4096
assert objvalues(objects[29485])[0]==15 and objvalues(objects[29485])[19]==-200
assert re.search(r'\bD0\s+[^~]*~[^~]*~\s*0 29420 29421\b',rooms[29407],re.S)
assert re.search(r'\nF\s+50\s+C\s+1 3\s+S',rooms[29469])
resets=harrow['reset_commands'];assert len(resets)==280
assert collections.Counter(r['command'] for r in resets)=={'M':161,'G':52,'D':24,'O':22,'E':20,'P':1}
assert len({(r['command'],tuple(r['arguments'])) for r in resets})==230
holder=None;families=[];proof={}
for r in resets:
 if r['command']=='M':holder=r['arguments']
 families.append((r['command'],tuple(r['arguments']),tuple(holder) if r['command'] in ('E','G') else None))
 if r['command'] in ('E','G') and r['arguments'][1] in {29400,29402,29404,29407,29413,29414,29415,29419,29440,29444}:
  proof.setdefault(r['arguments'][1],set()).add((holder[1],holder[3]))
assert len(set(families))==233
assert proof=={29400:{(29404,29405)},29402:{(29429,29437)},29404:{(29404,29405)},29407:{(29422,29418)},29413:{(29406,29416),(29406,29442)},29414:{(29434,29423)},29415:{(29428,29438)},29419:{(29438,29446)},29440:{(29438,29446),(29440,29446)},29444:{(29450,29474)}}
assert [(r['arguments'][1],r['arguments'][3]) for r in resets if r['command']=='O' and r['arguments'][1] in (29405,29410,29460)]==[(29405,29428),(29410,29451),(29460,29473),(29460,29475)]
assert [(r['arguments'][1],r['arguments'][3]) for r in resets if r['command']=='M' and r['arguments'][1]==29449]==[(29449,29472),(29449,29473),(29449,29473),(29449,29474),(29449,29475)]
assert next(r['arguments'][3] for r in resets if r['command']=='M' and r['arguments'][1]==29418)==29431
assert next(r['arguments'][3] for r in resets if r['command']=='M' and r['arguments'][1]==29444)==29461
shp=(ROOT/'areas/shp/harrow.shp').read_text(encoding='utf8')
shops={int(m[1]):m[2] for m in re.finditer(r'^#(\d+)~\n([\s\S]*?)(?=^#\d+~|\Z)',shp,re.M)}
assert set(shops)=={29404,29413,29429,29446,29452,29453,29456,29457,29458}
assert shops[29404].split('\n0\n',1)[0]=='N\n29400\n29401\n29411\n29412'
assert shops[29429].split('\n0\n',1)[0]=='N\n29409\n29408\n29402\n5\n29420\n29422\n29435\n29432\n29446'
assert int(mobiles[29428].split('~')[4].split()[0])&32768
assert harrow['special_assignments']==[{'kind':'room','vnum':29403,'function':'inn','source':'src/specs/specs.assign.c','line':2454}]
db=(ROOT/'src/world/db.c').read_text(encoding='utf8');assert 'IS_ACT(mob, ACT_TEACHER) && !mob_index[nr].func.mob' in db and 'mob_index[nr].func.mob = teacher;' in db
quest=(ROOT/'src/world/quest.c').read_text(encoding='utf8');assert "qmp->echoAll = (letterStrn[1] == 'A');" in quest and "qcp->echoAll = (letterStrn[1] == 'A');" in quest
assert sum(b['body'][0].startswith('qc_action ') for b in raw if b['kind'] in ('M','MA'))==17
touching=[b for b in inventory_module.native_blocks(ROOT) if b['kind'] in ('Q','QA') and any(k=='I' and v in objects for k,v in b['give']+b['receive'])]
assert len(touching)==8 and all(b['source']=='areas/qst/harrow.qst' for b in touching)
assert objvalues(dawndale_bodies('limbo','obj')[5])[0]==16
assert all(objvalues(dawndale_bodies('heavens','obj')[v])[0]==19 for v in (318,319,330))
orientation=' '.join(mapping['orientation']);assert 'they do not make requests automatic' in orientation


# Mountain Tracts: exact materials, valid fixed GRAB travel and potion allocation.
mountaintracks=inventory_module.area_evidence(ROOT,'mountaintracks')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='mountaintracks')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==4 and len(mapping['contacts'])==8 and not mapping['exclusions']
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':5,'completion':1}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/mountaintracks.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':4,'Q':4}
expected={13:([('I',20923)],[('I',20930)],False),34:([('I',20954)],[('I',20955)],False),57:([('I',20947),('I',20948)],[('I',20949)],False),85:([('I',20949)],[('I',20950)],False)}
for b in raw:
 if 'binding' not in b:continue
 assert (b['give'],b['receive'],b['disappear'])==expected[b['line']]
 assert sum(b['binding'] in s['contracts'] for s in mapping['stories'])==1
 assert next(r['definition'] for r in mountaintracks['requests'] if r['block']['line']==b['line'])['daily_eligible']
stories={s['id']:s for s in mapping['stories']}
assert stories['bumble-explorers-potion']['steps'][0]['contracts']==stories['futni-two-ingredients']['contracts']
assert [t['item_vnums'][0] for t in stories['futni-two-ingredients']['steps'][:-1]]==[20947,20948]
assert stories['bumble-explorers-potion']['steps'][1]['item_vnums']==[20949]
assert all(t.get('optional') for s in mapping['stories'] for t in s['steps'][:-1])
assert all(s['steps'][-1]['contracts']==s['contracts'] for s in mapping['stories'])
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={20928,20981,20983,20984,20982,20978,20979,20972}
assert sum(len(c['topics']) for c in contacts.values())==19
for v,c in contacts.items():
 assert c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==list(dict.fromkeys(t for b in mountaintracks['dialogue'] if b['giver_vnum']==v for t in b['body'][0].rstrip('~').split()))
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==209]
assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(4,4,4)
rooms=dawndale_bodies('mountaintracks','wld');objects=dawndale_bodies('mountaintracks','obj');mobiles=dawndale_bodies('mountaintracks','mob')
assert set(rooms)==set(range(20900,21150))-{20914,20915,20916,20917,20920}
assert set(objects)==set(range(20900,20963)) and set(mobiles)==set(range(20900,20998))
assert (mountaintracks['zone']['first_vnum'],mountaintracks['zone']['last_vnum'],mountaintracks['zone']['reset_mode'])==(20807,21149,2)
assert len({tuple(b.split('~')[:2]) for b in rooms.values()})==86 and len({b.split('~')[2].strip().splitlines()[0] for b in rooms.values()})==35
exits=[(v,m) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(exits)==578 and len({(m[2],m[3]) for v,m in exits})==15
assert len({tuple((int(m[1]),int(m[4]),int(m[5]),int(m[6])-v) for vv,m in exits if vv==v) for v in rooms})==199
missing={21202,21204,21206,21208}
assert [(v,int(m[1]),int(m[6])) for v,m in exits if int(m[6]) in missing or int(m[6])==-1]==[(20999,2,-1),(21126,1,21202),(21127,1,21206),(21127,3,21204),(21128,1,21208),(21128,3,21206)]
assert not any(int(m[6]) in {21127,21128,21149} for v,m in exits)
assert not any(v==21149 for v,m in exits)
assert re.search(r'\bD1\s+[^~]*~[^~]*~\s*0 0 21011\b',rooms[21011],re.S)
assert {v:objvalues(b)[11:15] for v,b in objects.items() if objvalues(b)[0]==25}=={20900:[20989,7,-1,0],20902:[20909,7,-1,0],20903:[20913,7,-1,0],20919:[21008,7,-1,0],20925:[20953,7,-1,0],20936:[21144,65,-1,0],20943:[20979,7,-1,0]}
assert all(objvalues(b)[7]==0 for b in objects.values() if objvalues(b)[0]==25)
assert objvalues(objects[20907])[7]==0 and objvalues(objects[20907])[19]==4393
assert objvalues(objects[20923])[0]==11 and objvalues(objects[20923])[6]&4096 and objvalues(objects[20923])[7]==2097153
assert objvalues(objects[20954])[0]==13 and objvalues(objects[20954])[6]&4096 and objvalues(objects[20954])[8]==32832
assert all(objvalues(objects[v])[6]==8392704 and objvalues(objects[v])[7]==1 and objvalues(objects[v])[8]==32768 for v in (20947,20948))
assert objvalues(objects[20949])[0]==10 and objvalues(objects[20949])[11:19]==[50,280,41,236,0,0,0,0]
assert {v:objvalues(b)[11:15] for v,b in objects.items() if objvalues(b)[0]==29}=={20917:[270,20949,5,1],20918:[270,20973,5,0],20935:[270,21139,2,0],20941:[270,20904,6,0],20945:[270,20921,1,0],20952:[270,20918,0,1],20960:[270,20922,4,0]}
assert not any(v==20904 and int(m[1])==6 or v==20921 and int(m[1])==1 for v,m in exits)
assert re.search(r'\nC\s+50 1\s+S',rooms[20913])
assert all(re.search(r'\nF\s+100\s+S',rooms[v]) for v in range(20923,20928))
assert sum(bool(re.search(r'\nF\s+10\s+S',b)) for b in rooms.values())==11
resets=mountaintracks['reset_commands'];assert len(resets)==259
assert collections.Counter(r['command'] for r in resets)=={'M':173,'D':40,'O':18,'E':16,'G':12}
assert len({(r['command'],tuple(r['arguments'])) for r in resets})==230
holder=None;families=[];proof={}
for r in resets:
 if r['command']=='M':holder=r['arguments']
 families.append((r['command'],tuple(r['arguments']),tuple(holder) if r['command'] in ('E','G') else None))
 if r['command'] in ('E','G') and r['arguments'][1] in {20947,20948,20954}:
  assert r['arguments'][2]==1
  proof.setdefault(r['arguments'][1],set()).add((holder[1],holder[3]))
assert len(set(families))==230
assert proof=={20954:{(20982,20952)},20948:{(20979,20978)},20947:{(20978,20979)}}
assert [(r['arguments'][1],r['arguments'][3]) for r in resets if r['command']=='O' and r['arguments'][1] in (20907,20923,20936)]==[(20907,20941),(20923,20941),(20936,21121)]
assert not any(r['command'] in ('O','E','G') and r['arguments'][1] in {20919,20943,20941,20945} for r in resets)
assert {r['arguments'][1]:r['arguments'][3] for r in resets if r['command']=='M' and r['arguments'][1] in contacts}=={20928:21024,20981:21025,20983:20945,20984:21015,20982:20952,20978:20979,20979:20978,20972:21025}
assert {tuple(r['arguments'][1:4]) for r in resets if r['command']=='D' and (r['arguments'][1],r['arguments'][2]) in {(20918,0),(20957,2),(20922,4),(20973,5),(20949,5),(20948,4),(21139,2),(21145,0)}}=={(20918,0,8),(20957,2,0),(20922,4,8),(20973,5,8),(20949,5,9),(20948,4,1),(21139,2,9),(21145,0,1)}
shp=(ROOT/'areas/shp/mountaintracks.shp').read_text(encoding='utf8')
assert shp.startswith('#20972~\nN\n20933\n20934\n20932\n20931\n20939\n0\n') and shp.count('#')==1
assert mountaintracks['special_assignments']==[]
assert not any(int(b.split('~')[4].split()[0])&32768 for b in mobiles.values())
assert not any(int(b.split('~')[2].strip().splitlines()[0].split()[1])&524288 for b in rooms.values())
table=(ROOT/'src/classes/epic_skills.c').read_text(encoding='utf8').split('epic_teacher_skill epic_teachers[] = {',1)[1].split('\n};',1)[0]
assert not set(map(int,re.findall(r'\{\s*(\d+)\s*,',table)))&set(mobiles)
cmd=(ROOT/'src/cmd/interp.h').read_text(encoding='utf8');assert '#define CMD_GRAB 65' in cmd and '#define CMD_CLIMB 556' in cmd
switch=(ROOT/'src/specs/specs.object.c').read_text(encoding='utf8').split('int item_switch(',1)[1].split('\n/*',1)[0]
assert 'REMOVE_BIT(world[in_room].dir_option[door]->exit_info, EX_BLOCKED);' in switch and not re.search(r'REMOVE_BIT\([^;]*EX_CLOSED',switch)
db=(ROOT/'src/world/db.c').read_text(encoding='utf8').split('void renum_world(void)',1)[1].split('void renum_zone_table',1)[0]
assert 'real_room0(' in db and 'FREE(invalid_exit);' in db and 'world[room].dir_option[door] = NULL;' in db
touching=[b for b in inventory_module.native_blocks(ROOT) if b['kind'] in ('Q','QA') and any(k=='I' and v in objects for k,v in b['give']+b['receive'])]
assert len(touching)==5
foreign=[b for b in touching if b['source']!='areas/qst/mountaintracks.qst']
assert len(foreign)==1 and (foreign[0]['source'],foreign[0]['line'],foreign[0]['giver_vnum'],foreign[0]['give'],foreign[0]['receive'],foreign[0]['disappear'])==('areas/qst/wh.qst',2199,55127,[('I',12001),('I',20950),('I',20949)],[('I',55062)],False)
assert (ROOT/'areas/qst/mountaintracks.qst').read_text(encoding='utf8').endswith('#20990\nS\n#20992\nS\n')


# Orcish Slave Camp: access-key allocation, tragic exact exchange and typed-food refusal.
shortc=inventory_module.area_evidence(ROOT,'shortc')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='shortc')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==2 and len(mapping['contacts'])==5 and len(mapping['exclusions'])==1
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':3,'completion':1}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/shortc.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':4,'Q':3}
expected={16:([('I',53200)],[('C',1500),('I',53201)],False),35:([('I',53201)],[('E',15000),('I',53203)],True),53:([('T',19)],[],False)}
for b in raw:
 if 'binding' not in b:continue
 assert (b['give'],b['receive'],b['disappear'])==expected[b['line']]
 assert sum(b['binding'] in s['contracts'] for s in mapping['stories']+mapping['exclusions'])==1
 assert next(r['definition'] for r in shortc['requests'] if r['block']['line']==b['line'])['daily_eligible']==(b['line']!=53)
master,hero=mapping['stories'];assert (master['id'],hero['id'])==('master-lost-key','hero-final-steak')
assert hero['steps'][0]['contracts']==master['contracts']
assert [t['item_vnums'][0] for t in hero['steps'][1:-1]]==[53200,53201]
assert all(t.get('optional') for s in mapping['stories'] for t in s['steps'][:-1])
assert all(s['steps'][-1]['contracts']==s['contracts'] for s in mapping['stories'])
assert 'type19' in mapping['exclusions'][0]['reason'] and 'Active accounting' in mapping['exclusions'][0]['reason']
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert set(contacts)=={53200,53201,53206,53222,53227}
assert sum(len(c['topics']) for c in contacts.values())==13
for v,c in contacts.items():
 assert c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==list(dict.fromkeys(t for b in shortc['dialogue'] if b['giver_vnum']==v for t in b['body'][0].rstrip('~').split()))
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==532]
assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(2,2,2)
rooms=dawndale_bodies('shortc','wld');objects=dawndale_bodies('shortc','obj');mobiles=dawndale_bodies('shortc','mob')
assert set(rooms)==set(range(53200,53214)) and set(objects)==set(range(53200,53218)) and set(mobiles)==set(range(53200,53231))
assert (shortc['zone']['first_vnum'],shortc['zone']['last_vnum'],shortc['zone']['reset_mode'])==(53200,53213,2)
assert len({tuple(b.split('~')[:2]) for b in rooms.values()})==14 and len({b.split('~')[2].strip().splitlines()[0] for b in rooms.values()})==4
exits=[(v,m) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(exits)==27 and len({(m[2],m[3]) for v,m in exits})==4
assert len({tuple((int(m[1]),int(m[4]),int(m[5]),int(m[6])-v) for vv,m in exits if vv==v) for v in rooms})==14
assert [(v,int(m[1]),int(m[6])) for v,m in exits if int(m[6]) not in rooms]==[(53200,3,10327)]
assert [(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,m in exits if int(m[5])]==[(53202,5,6,53200,53212),(53212,4,2,53200,53202)]
assert not any(objvalues(b)[0] in (25,29) for b in objects.values())
assert objvalues(objects[53200])[0]==18 and objvalues(objects[53200])[6]==4096 and objvalues(objects[53200])[11:19]==[0,20,0,0,0,0,0,0]
assert objvalues(objects[53201])[0]==13 and objvalues(objects[53201])[8]==32768 and objvalues(objects[53201])[11:19]==[24,0,0,3,0,0,0,0]
assert objvalues(objects[53203])[0]==5 and objvalues(objects[53204])[0]==19 and objvalues(objects[53204])[6]==4096
assert objvalues(objects[53205])[0]==15 and objvalues(objects[53214])[0]==17 and objvalues(objects[53214])[7]==0
assert objvalues(objects[53215])[11:15]==[41,297,-1,-1] and objvalues(objects[53216])[11:15]==[41,302,-1,-1]
resets=shortc['reset_commands'];assert len(resets)==64
assert collections.Counter(r['command'] for r in resets)=={'M':31,'D':16,'E':11,'G':3,'P':2,'O':1}
assert len({(r['command'],tuple(r['arguments'])) for r in resets})==64
holder=None;proof={}
for r in resets:
 if r['command']=='M':holder=r['arguments']
 if r['command']=='G':
  assert r['arguments'][2]==1
  proof[r['arguments'][1]]=(holder[1],holder[3])
assert proof=={53201:(53200,53202),53200:(53206,53203),53204:(53222,53210)}
assert {r['arguments'][1] for r in resets if r['command']=='M'}==set(mobiles)
assert {r['arguments'][1] for r in resets if r['command']=='M' and r['arguments'][4]==50}=={53223,53225,53228}
assert [(r['arguments'][1],r['arguments'][3]) for r in resets if r['command']=='P']==[(53215,53205),(53216,53205)]
assert [(r['arguments'][1],r['arguments'][3]) for r in resets if r['command']=='O']==[(53214,53202)]
assert [(r['arguments'][1],r['arguments'][2],r['arguments'][3]) for r in resets if r['command']=='D' and r['arguments'][1] in {53202,53212}]==[(53202,5,6),(53212,4,2)]
assert mobiles[53201].rstrip().endswith('5 5 1')
assert shortc['special_assignments']==[] and not (ROOT/'areas/shp/shortc.shp').exists()
assert not any(int(b.split('~')[4].split()[0])&32768 for b in mobiles.values())
assert not any(int(b.split('~')[2].strip().splitlines()[0].split()[1])&524288 for b in rooms.values())
quest=(ROOT/'src/world/quest.c').read_text(encoding='utf8')
durable=quest.split('static bool submit_durable_quest_offering(',1)[1].split('\nvoid tell_quest',1)[0]
assert 'goal->goal_type == QUEST_GOAL_ITEM &&' in durable and 'goal->goal_type != QUEST_GOAL_ITEM' in durable and 'QUEST_GOAL_ITEM_TYPE' not in durable
assert 'This quest cannot accept that durable item safely.' in quest and 'This quest cannot accept offerings right now.' in quest
legacy=quest.split('bool quest_completion(',1)[1].split('struct quest_reward_recovery_attempt',1)[0]
assert 'case QUEST_GOAL_ITEM_TYPE:' in legacy and 'obj->type == gp->number' in legacy
retirement=quest.split('static void finish_quest_reward(',1)[1].split('static bool publish_quest_offering',1)[0]
assert 'extract_char(mob);' in retirement and 'raw_kill' not in retirement
eat=(ROOT/'src/cmd/actobj.c').read_text(encoding='utf8').split('void do_eat(',1)[1].split('\nvoid do_pour',1)[0]
assert '(temp->type != ITEM_FOOD) && (GET_LEVEL(ch) < AVATAR)' in eat
movement=(ROOT/'src/cmd/actmove.c').read_text(encoding='utf8');key=movement.split('P_obj has_key(',1)[1].split('void do_lock',1)[0]
assert 'ch->equipment[HOLD]' in key and 'ch->carrying' in key
unlock=movement.split('void do_unlock(',1)[1].split('void do_pick',1)[0]
assert 'number(0, 99) < key_obj->value[1]' in unlock and 'break_key(ch, key_obj);' in unlock
db=(ROOT/'src/world/db.c').read_text(encoding='utf8');loader=db.split('void setup_dir(',1)[1].split('void renum_world',1)[0]
assert 'if (state == 2)' in loader and 'exit_info |= EX_PICKABLE;' in loader and 'if (state == 3)' in loader and 'exit_info |= EX_PICKPROOF;' in loader
touching=[b for b in inventory_module.native_blocks(ROOT) if b['kind'] in ('Q','QA') and any(k=='I' and v in objects for k,v in b['give']+b['receive'])]
assert len(touching)==2 and all(b['source']=='areas/qst/shortc.qst' for b in touching)
assert 'barbarian and gnomes' not in rooms[53204] and 'barbarians and gnomes' in rooms[53204]


# Lava Caves: a blocked sole producer, an exact supplied-material return and native access.
lavcav=inventory_module.area_evidence(ROOT,'lavcav')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='lavcav')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==1 and len(mapping['contacts'])==7 and len(mapping['exclusions'])==1
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':2,'completion':1}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/lavcav.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':4,'Q':2}
expected={43:([('I',35515)],[('I',35525),('E',50000)],False),62:([('C',100000)],[('I',35515)],True)}
for b in raw:
 if 'binding' not in b:continue
 assert (b['give'],b['receive'],b['disappear'])==expected[b['line']]
 assert sum(b['binding'] in s['contracts'] for s in mapping['stories']+mapping['exclusions'])==1
 assert next(r['definition'] for r in lavcav['requests'] if r['block']['line']==b['line'])['daily_eligible']==(b['line']==43)
lieutenant=mapping['stories'][0];assert lieutenant['id']=='lieutenant-platinum-horns'
assert lieutenant['steps'][0]['contracts']==mapping['exclusions'][0]['contracts']
assert [t['item_vnums'][0] for t in lieutenant['steps'][1:-1]]==[35505,35515]
assert all(t.get('optional') for t in lieutenant['steps'][:-1])
assert lieutenant['steps'][-1]['contracts']==lieutenant['contracts']
assert 'coin-only' in mapping['exclusions'][0]['reason'] and 'active accounting' in mapping['exclusions'][0]['reason']
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert set(contacts)=={35515,35517,35519,35523,35526,35535,35540}
assert sum(len(c['topics']) for c in contacts.values())==6
for v,c in contacts.items():
 assert c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==list(dict.fromkeys(t for b in lavcav['dialogue'] if b['giver_vnum']==v for t in b['body'][0].rstrip('~').split()))
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==355]
assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(1,1,1)
rooms=dawndale_bodies('lavcav','wld');objects=dawndale_bodies('lavcav','obj');mobiles=dawndale_bodies('lavcav','mob')
assert set(rooms)==set(range(35501,35643)) and set(mobiles)==set(range(35501,35549)) and len(objects)==33
assert (lavcav['zone']['first_vnum'],lavcav['zone']['last_vnum'],lavcav['zone']['reset_mode'])==(35300,35642,1)
assert len({tuple(b.split('~')[:2]) for b in rooms.values()})==38 and len({b.split('~')[2].strip().splitlines()[0] for b in rooms.values()})==11
exits=[(v,m) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(exits)==451 and len({(m[2],m[3]) for v,m in exits})==6
assert len({tuple((int(m[1]),int(m[4]),int(m[5]),int(m[6])-v) for vv,m in exits if vv==v) for v in rooms})==128
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,m in exits]
assert [(v,d,t) for v,d,s,k,t in edges if t not in rooms]==[(35501,0,21148)]
assert [(d,t) for v,d,s,k,t in edges if v==35635]==[(1,35634),(3,35544)]
assert not any(v==35634 for v,d,s,k,t in edges) and not any(t==35635 for v,d,s,k,t in edges)
reached={35501};front=[35501]
while front:
 v=front.pop()
 for a,d,s,k,t in edges:
  if a==v and t in rooms and t not in reached:reached.add(t);front.append(t)
assert set(rooms)-reached=={35634,35635}
assert [(v,d,s,k,t) for v,d,s,k,t in edges if k]==[(35580,2,3,35524,35582),(35582,0,3,35524,35580),(35620,1,2,35505,35621),(35621,3,2,35505,35620)]
assert (35537,3,9,0,35637) in edges and (35637,1,1,0,35537) in edges
assert (35581,1,5,0,35636) in edges and (35605,9,5,0,35606) in edges
assert not any(objvalues(b)[0]==25 for b in objects.values())
assert objvalues(objects[35505])[0]==18 and objvalues(objects[35505])[7]==1 and objvalues(objects[35505])[11:19]==[0]*8
assert objvalues(objects[35515])[0]==9 and objvalues(objects[35515])[7]==134217729 and objvalues(objects[35515])[8]==32768
assert objvalues(objects[35525])[0]==9 and objvalues(objects[35525])[7]==4097
assert objvalues(objects[35524])[0]==11 and objvalues(objects[35524])[11:19]==[500,500,13,0,0,0,0,0]
assert objvalues(objects[35592])[0]==29 and objvalues(objects[35592])[7]==0 and objvalues(objects[35592])[19]==1200 and objvalues(objects[35592])[11:19]==[270,35537,3,0,0,0,0,0]
assert objvalues(objects[35582])[11:15]==[50,352,3,0] and objvalues(objects[35590])[11:15]==[50,131,-1,-1] and objvalues(objects[35591])[11:15]==[50,108,-1,-1]
resets=lavcav['reset_commands'];assert len(resets)==163
assert collections.Counter(r['command'] for r in resets)=={'M':87,'E':48,'D':20,'G':5,'F':2,'O':1}
assert len({(r['command'],tuple(r['arguments'])) for r in resets})==134
assert not any(r['command'] in ('O','G','E','P') and r['arguments'][1]==35515 for r in resets)
holder=None;proof={}
for r in resets:
 if r['command']=='M':holder=r['arguments']
 if r['command']=='G':proof[r['arguments'][1]]=(holder[1],holder[3],r['arguments'][2],r['arguments'][4])
assert proof=={35505:(35519,35620,1,100),35524:(35526,35636,1,24),35582:(35540,35616,1,100),35590:(35540,35616,1,100),35591:(35540,35616,1,100)}
assert [(r['arguments'][2],r['arguments'][3],r['arguments'][4]) for r in resets if r['command']=='E' and r['arguments'][1]==35503]==[(1,27,40)]
placed={r['arguments'][1] for r in resets if r['command'] in ('M','F')};assert placed==set(mobiles)
assert {r['arguments'][1] for r in resets if r['command']=='F'}=={35538,35539}
assert [(r['arguments'][1],r['arguments'][3]) for r in resets if r['command']=='O']==[(35592,35537)]
assert [(r['arguments'][1],r['arguments'][2],r['arguments'][3]) for r in resets if r['command']=='D' and r['arguments'][1] in {35580,35582,35620,35621,35537}]==[(35537,3,9),(35580,2,0),(35582,0,0),(35620,1,2),(35621,3,1),(35621,2,0)]
assert not int(mobiles[35535].split('~')[4].split()[0])&2 and int(mobiles[35535].split('~')[4].split()[0])&64
assert mobiles[35515].rstrip().endswith('6 6 1')
assert lavcav['special_assignments']==[] and not (ROOT/'areas/shp/lavcav.shp').exists()
assert not any(int(b.split('~')[4].split()[0])&32768 for b in mobiles.values())
assert not any(int(b.split('~')[2].strip().splitlines()[0].split()[1])&524288 for b in rooms.values())
quest=(ROOT/'src/world/quest.c').read_text(encoding='utf8')
coin=quest.split('const bool giving_coins = isdigit(*temparg);',1)[1].split('void tell_quest',1)[0]
assert coin.index('if (!giving_coins)')<coin.index('if (economic_gameplay_authority::active())')<coin.index('This quest cannot accept offerings right now.')<coin.index('do_give(')
assert 'GET_MONEY(mob)' in quest and 'SUB_MONEY(mob' in quest
assert 'p = amount / 1000;' in (ROOT/'src/core/utility.c').read_text(encoding='utf8')
movement=(ROOT/'src/cmd/actmove.c').read_text(encoding='utf8');key=movement.split('P_obj has_key(',1)[1].split('void do_lock',1)[0]
assert 'ch->equipment[HOLD]' in key and 'ITEM_KEY' not in key
unlock=movement.split('void do_unlock(',1)[1].split('void do_pick',1)[0]
assert 'number(0, 99) < key_obj->value[1]' in unlock and 'break_key(ch, key_obj);' in unlock
switch=(ROOT/'src/specs/specs.object.c').read_text(encoding='utf8').split('int item_switch(',1)[1]
assert 'obj->type != ITEM_SWITCH || obj->value[0] != cmd' in switch and 'REMOVE_BIT(world[in_room].dir_option[door]->exit_info, EX_BLOCKED);' in switch
switch=switch[:switch.index('return TRUE;',switch.index('REMOVE_BIT(world[in_room]'))]
assert 'EX_CLOSED' not in switch
reset=(ROOT/'src/world/db.c').read_text(encoding='utf8').split('void reset_zone(',1)[1]
assert reset.index('economic_gameplay_authority::active()')<reset.index('reset_command_issues_item(ZCMD.command)')<reset.index('continue;')<reset.index('read_object(')
assert "(last_mob_followable && (ZCMD.command == 'F'))" in reset
fire=(ROOT/'src/mob/specials.c').read_text(encoding='utf8').split('void event_firesector(',1)[1].split('void event_underwatersector',1)[0]
assert 'SECT_FIREPLANE' in fire and 'SPELL_PROTECT_FROM_FIRE' in fire and 'SPELL_FIRE_WARD' in fire and 'affect_remove(ch, af);' in fire and 'GET_HIT(ch) -= 3;' in fire
random=(ROOT/'src/item/randomeq.c').read_text(encoding='utf8');assert '35523' in random.split('const int highdrop_mobs',1)[1].split('};',1)[0]
assert random.count('highdrop_mobs[i]')==2 and 'highdrop_mobs[i]' not in re.sub(r'/\*.*?\*/','',random,flags=re.S)
touching=[b for b in inventory_module.native_blocks(ROOT) if b['kind'] in ('Q','QA') and any(k=='I' and v in objects for k,v in b['give']+b['receive'])]
assert len(touching)==2 and all(b['source']=='areas/qst/lavcav.qst' for b in touching)
foreign=inventory_module.area_evidence(ROOT,'ravenloft2')['reset_commands']
assert any(r['command']=='P' and r['arguments'][1:4]==[35589,1,59045] for r in foreign)
assert (21148,2,0,0,35501) in [(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in dawndale_bodies('mountaintracks','wld').items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]

# Nomad Encampment: exact collateral chain, hidden transient proofs and clue/receipt boundaries.
nomads=inventory_module.area_evidence(ROOT,'nomads')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='nomads')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==2 and len(mapping['contacts'])==7 and not mapping['exclusions']
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':5,'completion':1}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/nomads.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':2,'MA':1,'QA':2}
expected={48:([('I',6217),('I',6218)],[('I',6221)],False),64:([('I',6219),('I',6220),('I',6221)],[('I',6222)],True)}
for b in raw:
 if 'binding' not in b:continue
 assert (b['give'],b['receive'],b['disappear'])==expected[b['line']]
 assert sum(b['binding'] in s['contracts'] for s in mapping['stories'])==1
 assert next(r['definition'] for r in nomads['requests'] if r['block']['line']==b['line'])['daily_eligible']
evidence,trophies=mapping['stories'];assert (evidence['id'],trophies['id'])==('septimus-planar-evidence','septimus-heads-and-collateral')
assert trophies['steps'][0]['contracts']==evidence['contracts']
assert [t['item_vnums'][0] for t in evidence['steps'][:-1]]==[6217,6218]
assert [t['item_vnums'][0] for t in trophies['steps'][1:-1]]==[6219,6220,6221]
assert all(t.get('optional') for s in mapping['stories'] for t in s['steps'][:-1])
assert all(s['steps'][-1]['contracts']==s['contracts'] for s in mapping['stories'])
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert set(contacts)=={6200,6214,6215,6216,6217,6218,6220}
assert sum(len(c['topics']) for c in contacts.values())==5
for v,c in contacts.items():
 assert c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==list(dict.fromkeys(t for b in nomads['dialogue'] if b['giver_vnum']==v for t in b['body'][0].rstrip('~').split()))
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==62]
assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(2,2,2)
rooms=dawndale_bodies('nomads','wld');objects=dawndale_bodies('nomads','obj');mobiles=dawndale_bodies('nomads','mob')
assert set(rooms)==set(range(6200,6249)) and set(objects)==set(range(6200,6223)) and set(mobiles)==set(range(6200,6221))
assert (nomads['zone']['first_vnum'],nomads['zone']['last_vnum'],nomads['zone']['reset_mode'])==(6122,6248,2)
assert len({tuple(b.split('~')[:2]) for b in rooms.values()})==36 and len({b.split('~')[2].strip().splitlines()[0] for b in rooms.values()})==5
exits=[(v,m) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(exits)==124 and len({(m[2],m[3]) for v,m in exits})==2
assert len({tuple((int(m[1]),int(m[4]),int(m[5]),int(m[6])-v) for vv,m in exits if vv==v) for v in rooms})==31
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,m in exits]
assert [(v,d,s,k,t) for v,d,s,k,t in edges if t not in rooms]==[(6203,0,0,0,582095),(6221,3,0,0,582494),(6227,1,0,0,582496),(6246,2,0,0,582895)]
assert all((s,k)==(1,0) for v,d,s,k,t in edges if s or k)
assert len([1 for v,d,s,k,t in edges if s])==16
assert not any(objvalues(b)[0] in (18,25,29) for b in objects.values())
assert all(objvalues(objects[v])[0]==8 and objvalues(objects[v])[7]==1 and objvalues(objects[v])[8]==32768 and objvalues(objects[v])[6]&4096 for v in (6217,6218,6219,6220))
assert all(objvalues(objects[v])[6]==8916992 for v in (6219,6220))
assert all(objvalues(objects[v])[6]==12353 for v in (6217,6218))
assert objvalues(objects[6221])[0]==9 and objvalues(objects[6221])[7]==3 and objvalues(objects[6221])[8]==32768
assert objvalues(objects[6222])[0]==9 and objvalues(objects[6222])[7]==17
assert objvalues(objects[6215])[0]==33 and objvalues(objects[6215])[6]==4168 and objvalues(objects[6215])[11:19]==[0,0,108,0,0,0,0,0]
assert 'page1~' in objects[6215] and 'page2~' in objects[6215] and 'Rellius' in objects[6215] and 'ironworker' in objects[6215]
assert objvalues(objects[6216])[0]==15 and objvalues(objects[6216])[7]==0 and objvalues(objects[6216])[19]==165 and objvalues(objects[6216])[6]==4096
assert objvalues(objects[6214])[11:15]==[50,192,234,-1]
assert '62 4194304 1' in rooms[6224] and 'logs' in rooms[6224]
resets=nomads['reset_commands'];assert len(resets)==102
assert collections.Counter(r['command'] for r in resets)=={'M':55,'E':22,'D':16,'O':4,'G':4,'P':1}
assert len({(r['command'],tuple(r['arguments'])) for r in resets})==90
assert all(r['arguments'][4]==100 for r in resets)
holder=None;proof={}
for r in resets:
 if r['command']=='M':holder=r['arguments']
 if r['command']=='G':proof[r['arguments'][1]]=(holder[1],holder[3],r['arguments'][2])
assert proof=={6219:(6216,6208,1),6217:(6215,6208,1),6220:(6217,6240,1),6218:(6218,6240,1)}
assert {r['arguments'][1] for r in resets if r['command']=='M'}==set(mobiles)
assert [(r['arguments'][1],r['arguments'][2],r['arguments'][3]) for r in resets if r['command']=='O']==[(6215,1,6236),(6214,1,6236),(6213,2,6238),(6216,1,6240)]
assert [(r['arguments'][1],r['arguments'][2],r['arguments'][3]) for r in resets if r['command']=='P']==[(6213,2,6216)]
assert not any(r['command'] in ('O','G','E','P') and r['arguments'][1] in {6221,6222} for r in resets)
assert mobiles[6214].rstrip().endswith('4 8 1') and mobiles[6216].rstrip().endswith('4 8 1')
assert nomads['special_assignments']==[] and not (ROOT/'areas/shp/nomads.shp').exists()
assert not any(int(b.split('~')[4].split()[0])&32768 for b in mobiles.values())
assert not any(int(b.split('~')[2].strip().splitlines()[0].split()[1])&524288 for b in rooms.values())
quest=(ROOT/'src/world/quest.c').read_text(encoding='utf8')
assert "qcp->echoAll = (letterStrn[1] == 'A');" in quest and "qmp->echoAll = (letterStrn[1] == 'A');" in quest
assert 'qmp->echoAll ? TO_ROOM : TO_VICT' in quest and 'zone_story_quest_runtime::encountered(pl, ch);' in quest and 'GET_STAT(ch) <= STAT_SLEEPING' in quest
durable=quest.split('static bool submit_durable_quest_offering(',1)[1].split('void tell_quest',1)[0]
assert 'for (P_obj item = actor->carrying;' in durable and 'used = used || roots[index] == item;' in durable and 'item_movement_transaction_submit_batch(' in durable and 'Bring all the requested items together before offering them.' in durable
look=(ROOT/'src/cmd/actinf.c').read_text(encoding='utf8');assert 'tmp_desc = find_ex_description(' in look
assert 'do_look(ch, buf, -4);' in look.split('void do_read(',1)[1].split('void do_examine',1)[0]
book=look.split('void ShowCharSpellBookSpells(',1)[1].split('void ',1)[0]
assert 'find_spell_description(obj)' in book and 'obj->value[2]' in book and 'pages left' in book
wake=(ROOT/'src/cmd/actmove.c').read_text(encoding='utf8').split('void do_wake(',1)[1].split('void ',1)[0]
assert 'GET_STAT(tmp_char) == STAT_SLEEPING' in wake and 'AFF_KNOCKED_OUT' in wake and 'resting_posture(tmp_char) + STAT_RESTING' in wake
handler=(ROOT/'src/world/handler.c').read_text(encoding='utf8')
assert 'set_obj_affected(object, 0, TAG_OBJ_DECAY, 0);' in handler
corpse=(ROOT/'src/combat/fight.c').read_text(encoding='utf8').split('P_obj make_corpse(',1)[1].split('void change_alignment',1)[0]
assert 'corpse->contains = ch->carrying;' in corpse and 'ch->carrying = NULL;' in corpse and 'IS_NPC(ch) && IS_NOCORPSE(ch)' in corpse
snapshot=(ROOT/'src/player/player_snapshot_capture.c').read_text(encoding='utf8')
assert 'IS_SET(object->extra_flags, ITEM_NORENT) && !active_durable_custody' in snapshot and 'durable_norent_included' in snapshot
reset=(ROOT/'src/world/db.c').read_text(encoding='utf8').split('void reset_zone(',1)[1]
assert reset.index('economic_gameplay_authority::active()')<reset.index('reset_command_issues_item(ZCMD.command)')<reset.index('continue;')<reset.index('read_object(')
assert 'zone_table[zone].reset_mode == 2 || ::is_empty(zone)' in (ROOT/'src/world/events.c').read_text(encoding='utf8')
prime=(ROOT/'src/classes/innates.c').read_text(encoding='utf8').split('void do_shift_prime(',1)[1].split('void do_blast',1)[0]
assert '6224' in prime and 'RACE_GITHZERAI' in prime and 'char_to_room(ch, r_room, -1)' in prime
assert re.search(r'\{\s*6,\s*6222\s*\}',(ROOT/'src/account/chaos_eq_data.h').read_text(encoding='utf8'))
assert 'economic_source_kind::starter_grant' in (ROOT/'src/account/nanny.c').read_text(encoding='utf8')
touching=[b for b in inventory_module.native_blocks(ROOT) if b['kind'] in ('Q','QA') and any(k=='I' and v in objects for k,v in b['give']+b['receive'])]
assert len(touching)==2 and all(b['source']=='areas/qst/nomads.qst' for b in touching)

# Undermountain: competing breakable access key, blank note and dormant controllers.
undermountain=inventory_module.area_evidence(ROOT,'undermountain')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='undermountain')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==2 and len(mapping['contacts'])==3 and not mapping['exclusions']
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':2,'completion':1}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/undermountain.qst']
assert collections.Counter(b['kind'] for b in raw)=={'M':3,'Q':1,'QA':1}
expected={13:([('I',92134)],[('I',92120)],False),28:([('I',92133)],[('I',92134)],True)}
for b in raw:
 if 'binding' not in b:continue
 assert (b['give'],b['receive'],b['disappear'])==expected[b['line']]
 assert sum(b['binding'] in s['contracts'] for s in mapping['stories'])==1
 assert next(r['definition'] for r in undermountain['requests'] if r['block']['line']==b['line'])['daily_eligible']
tamsil,durnan=mapping['stories'];assert (tamsil['id'],durnan['id'])==('tamsil-grate-key','durnan-tamsil-note')
assert durnan['steps'][0]['contracts']==tamsil['contracts'] and tamsil['steps'][0]['item_vnums']==[92133] and durnan['steps'][1]['item_vnums']==[92134]
assert all(t.get('optional') for s in mapping['stories'] for t in s['steps'][:-1]) and all(s['steps'][-1]['contracts']==s['contracts'] for s in mapping['stories'])
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert set(contacts)=={92002,92043,92082} and sum(len(c['topics']) for c in contacts.values())==8
for v,c in contacts.items():
 assert c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==list(dict.fromkeys(t for b in undermountain['dialogue'] if b['giver_vnum']==v for t in b['body'][0].rstrip('~').split()))
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==920];assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(2,2,2)
rooms=dawndale_bodies('undermountain','wld');objects=dawndale_bodies('undermountain','obj');mobiles=dawndale_bodies('undermountain','mob')
assert (len(rooms),len(objects),len(mobiles))==(441,135,95) and set(objects)==set(range(92000,92135)) and set(mobiles)==set(range(92000,92095))
assert (undermountain['zone']['first_vnum'],undermountain['zone']['last_vnum'],undermountain['zone']['reset_mode'])==(91168,92519,1)
assert len({tuple(b.split('~')[:2]) for b in rooms.values()})==332 and len({b.split('~')[2].strip().splitlines()[0] for b in rooms.values()})==16
exits=[(v,m) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(exits)==1059 and len({(m[2],m[3]) for v,m in exits})==201
assert len({tuple((int(m[1]),int(m[4]),int(m[5]),int(m[6])-v) for vv,m in exits if vv==v) for v in rooms})==354
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,m in exits]
assert (92289,5,2,92133,92519) in edges and (92519,4,2,0,92289) in edges
assert [(v,d,s,k,t) for v,d,s,k,t in edges if t not in rooms]==[(92501,2,0,0,74045),(92518,5,0,0,4557)]
resets=undermountain['reset_commands'];assert len(resets)==731 and collections.Counter(r['command'] for r in resets)=={'M':256,'D':226,'E':131,'O':55,'P':44,'G':11,'F':8}
assert all(r['arguments'][4]==100 for r in resets)
assert len({(r['command'],tuple(r['arguments'])) for r in resets})==509
for v,room in [(92002,92042),(92043,92289),(92082,92519)]:assert any(r['command']=='M' and r['arguments'][1]==v and r['arguments'][3]==room for r in resets)
parent=None;holders=[];families=[]
for r in resets:
 if r['command']=='M':parent=tuple(r['arguments'])
 families.append((r['command'],tuple(r['arguments']),parent if r['command'] in ('E','G','F') else None))
 if r['command']=='G' and r['arguments'][1]==92133:holders.append((parent[1],parent[3],r['arguments'][2]))
assert len(set(families))==548 and holders==[(92043,92289,1)]
assert all(any(r['command']=='D' and r['arguments'][1:4]==[v,d,2] for r in resets) for v,d in ((92289,5),(92519,4)))
assert not any(r['command'] in ('O','E','G','P') and r['arguments'][1] in (92120,92131,92134) for r in resets)
key=objvalues(objects[92133]);assert key[0]==18 and key[6]==4104 and key[7]==1 and key[8]==32768 and key[11:19]==[0,100,0,0,0,0,0,0]
note=objvalues(objects[92134]);assert note[0]==16 and note[7]==1 and note[8]==32768 and not objects[92134].split('~')[3].strip()
assert objvalues(objects[92018])[0]==13 and 'lockpicks' in objects[92018]
reward=objvalues(objects[92120]);assert reward[0]==5 and reward[6]&4096 and reward[7]==8193
for v,target,room in [(92122,92508,92278),(92129,92281,92450)]:
 vals=objvalues(objects[v]);assert vals[0]==25 and vals[7]==0 and vals[11:15]==[target,7,-1,0]
 assert any(r['command']=='O' and r['arguments'][1]==v and r['arguments'][3]==room for r in resets)
assert objvalues(objects[92130])[11:14]==[270,92068,5] and objvalues(objects[92131])[11:14]==[340,92005,4]
assert not any(v==92033 for v,m in exits) and '75%' in rooms[92033] and '25%' in rooms[92029]
assignment=(ROOT/'src/specs/specs.assign.c').read_text(encoding='utf8');disabled=assignment[assignment.index('#if 0',assignment.index('undermountain')):].split('#endif',1)[0]
assert all(f'= {fn};' in disabled for fn in ('um_durnan','um_tamsil','um_kevlar','um_thorn','um_korelar','um_mezzoloth','flying_dagger','ochre_jelly','helmed_horror','malodine_two'))
assert not re.search(r'real_object0\(92120\)\].func.obj',assignment)
move=(ROOT/'src/cmd/actmove.c').read_text(encoding='utf8');unlock=move.split('void do_unlock(',1)[1].split('void ',1)[0]
assert 'number(0, 99) < key_obj->value[1]' in unlock and 'break_key(ch, key_obj)' in unlock
assert 'item_movement_transaction_submit(' in move.split('static bool break_key(',1)[1].split('static void',1)[0]
look=(ROOT/'src/cmd/actinf.c').read_text(encoding='utf8');assert 'object->type == ITEM_NOTE' in look and 'object->action_description' in look and "It's blank." in look
write=(ROOT/'src/cmd/actcomm.c').read_text(encoding='utf8');assert 'paper->type != ITEM_NOTE' in write and 'ch->desc->str = &paper->action_description;' in write
search=(ROOT/'src/cmd/actobj.c').read_text(encoding='utf8').split('void do_search(',1)[1].split('void do_apply_poison',1)[0]
assert 'k = world[ch->in_room].contents;' in search and 'k = k->contains;' in search and 'if (CAN_SEE_OBJ(ch, k))' in search and 'SET_BIT(k->extra_flags, ITEM_SECRET)' in search
reset=(ROOT/'src/world/db.c').read_text(encoding='utf8').split('void reset_zone(',1)[1];assert reset.index('economic_gameplay_authority::active()')<reset.index('reset_command_issues_item(ZCMD.command)')<reset.index('continue;')<reset.index('read_object(')
assert 'obj->type == ITEM_SWITCH && !obj_index[nr].func.obj' in (ROOT/'src/world/db.c').read_text(encoding='utf8')

# Desolate Under Fire: independent rescues, exact bundles and incomplete native services.
desolateinv=inventory_module.area_evidence(ROOT,'desolateinv')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='desolateinv')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==17 and len(mapping['contacts'])==16 and not mapping['exclusions']
assert not any(c['topics'] for c in mapping['contacts'])
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':18,'completion':9}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/desolateinv.qst']
assert collections.Counter(b['kind'] for b in raw)=={'Q':16,'QA':1}
expected={2:(77311,[77326]),13:(77312,[77326]),23:(77313,[77326]),34:(77319,[77373,77385,77392]),53:(77324,[77326]),67:(77325,[77331]),75:(77327,[77316]),85:(77338,[77315]),93:(77338,[77326]),104:(77348,[77352]),116:(77350,[77345]*8),139:(77365,[77317]),148:(77370,[77326]),158:(77380,[]),165:(77383,[77312]),174:(77385,[77326]),185:(77386,[77326])}
for b in raw:
 assert (b['giver_vnum'],[v for k,v in b['give'] if k=='I'])==expected[b['line']]
 s=next(s for s in mapping['stories'] if b['binding'] in s['contracts'])
 assert sum(b['binding'] in s['contracts'] for s in mapping['stories'])==1
 assert s['steps'][-1]['contracts']==s['contracts'] and all(t.get('optional') for t in s['steps'][:-1])
 assert s['category']==('service' if b['line']==158 else 'story')
 assert collections.Counter(v for t in s['steps'] if t['kind']=='carried_item' for v in t['item_vnums'] for _ in range(t['count']))==collections.Counter(expected[b['line']][1])
rescues=[s for s in mapping['stories'] if any(t.get('item_vnums')==[77326] for t in s['steps'])];assert len(rescues)==8
beregan=next(s for s in mapping['stories'] if s['id']=='beregan-eight-bindings')
assert beregan['steps'][-2]['item_vnums']==[77345] and beregan['steps'][-2]['count']==8
assert [t['contracts'] for t in beregan['steps'][:-2]]==[s['contracts'] for s in rescues]
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==773];assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(17,16,16)
rooms=dawndale_bodies('desolateinv','wld');objects=dawndale_bodies('desolateinv','obj');mobiles=dawndale_bodies('desolateinv','mob')
assert set(rooms)==set(range(77301,77466)) and set(objects)==set(range(77301,77397)) and set(mobiles)==set(range(77301,77411))
exits=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(exits)==346 and [(v,d,f,k,t) for v,d,f,k,t in exits if t not in rooms]==[(77301,2,0,0,217609)]
assert not any(v==77465 for v,d,f,k,t in exits) and 'Load VNUM' in rooms[77465]
resets=desolateinv['reset_commands'];assert len(resets)==332 and collections.Counter(r['command'] for r in resets)=={'M':159,'D':64,'O':44,'E':30,'P':16,'F':9,'G':9,'R':1}
assert all(r['arguments'][4]==100 for r in resets) and len({(r['command'],tuple(r['arguments'])) for r in resets})==271
assert not any(r['command']=='M' and r['arguments'][1] in (77325,77348,77380) for r in resets)
assert not any(r['command'] in ('O','G','E','P') and r['arguments'][1]==77352 for r in resets)
assert not any(b['giver_vnum']==77368 for b in raw) and not any(('I',77352) in b['receive'] for b in inventory_module.native_blocks(ROOT))
assert sum(r['command'] in ('O','G','P') and r['arguments'][1]==77326 for r in resets)==9
assert objvalues(objects[77345])[6]==4194304 and not objvalues(objects[77345])[6]&524288
assert objvalues(objects[77315])[0]==15 and objvalues(objects[77315])[11:14]==[100,5,0]
assert any(r['command']=='P' and r['arguments'][1:4]==[77316,1,77315] for r in resets)
for v in (77312,77311,77384):assert objvalues(objects[v])[6]&8388608
for room,button,direction,target in ((77410,77324,1,77411),(77411,77325,1,77412),(77412,77348,2,77413),(77413,77349,3,77414),(77414,77337,3,77415),(77415,77350,0,77410)):
 assert objvalues(objects[button])[11:14]==[270,room,direction] and (room,direction,8,0,target) in exits
 assert any(r['command']=='O' and r['arguments'][1]==button and r['arguments'][3]==room for r in resets)
for v,target in ((77305,77410),(77323,77374),(77393,77343),(77394,77372),(77334,77427),(77372,77462),(77374,77464)):
 vals=objvalues(objects[v]);assert vals[0]==25 and vals[7]==0 and vals[11:15]==[target,7,-1,0]
normal=dawndale_bodies('desolate','obj');assert objvalues(normal[22291])[11:13]==[100,77302]
assert re.search(r'^O 0 22291 1 22200 20\b',(ROOT/'areas/zon/desolate.zon').read_text(),re.M)
random_exit=(ROOT/'src/item/objmisc.c').read_text().split('void event_random_exit(',1)[1].split('// Hidden NPC',1)[0]
assert 'obj->value[0] > number(0, 99)' in random_exit and 'ZONE_CLOSED' in random_exit and 'rev_dir[exit_dir]' in random_exit
assert 'world[real_room0(77442)].funct = inn;' in (ROOT/'src/specs/specs.assign.c').read_text()
quest=(ROOT/'src/world/quest.c').read_text();offering=quest.split('static bool submit_durable_quest_offering(',1)[1].split('void tell_quest(',1)[0]
assert 'roots[index] == item' in offering and 'actor->carrying' in offering and 'selected->obj_uid' not in offering
assert 'goal->goal_type != QUEST_GOAL_ITEM' in offering and 'roots[index]->obj_uid' in offering

# Storm Port Stronghold outside-path caption follows the existing reciprocal exits.
spshold_direction_rooms=dawndale_bodies('spshold','wld')
spshold_outside=re.sub(r'&(?:\+[A-Za-z]|[A-Za-z0-9])','',spshold_direction_rooms[22607].split('~')[1])
assert re.search(r'outpost\s+lying just to\s+the west\.',spshold_outside), 'Stronghold outside caption must place the outpost west'
assert re.search(r'path through the forest\s+is\s+just to the east\.',spshold_outside,re.I), 'Forest remains east'
for source,direction,target in ((22607,3,22606),(22606,1,22607),(22607,1,22630),(22630,3,22607)):
 assert re.search(r'\bD'+str(direction)+r'\s+[^~]*~[^~]*~\s+0 0 '+str(target)+r'\b',spshold_direction_rooms[source],re.S), 'Stronghold clue must follow an existing reciprocal route'

# Storm Port Stronghold: exact bundles, separate maps and foreign referral ownership.
spshold=inventory_module.area_evidence(ROOT,'spshold')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='spshold')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==4 and len(mapping['contacts'])==6 and not mapping['exclusions']
assert not any(c['topics'] for c in mapping['contacts'])
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':6,'completion':2}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/spshold.qst']
expected={2:(22617,[22613],[22621]),10:(22626,[22622],[22627,22625,40771]),18:(22626,[77209],[22633]),31:(22636,[22610,22612],[22613])}
assert len(raw)==4 and all(b['kind']=='Q' and not b['disappear'] for b in raw)
for b in raw:
 assert (b['giver_vnum'],[v for k,v in b['give'] if k=='I'],[v for k,v in b['receive'] if k=='I'])==expected[b['line']]
 s=next(s for s in mapping['stories'] if b['binding'] in s['contracts'])
 assert sum(b['binding'] in s['contracts'] for s in mapping['stories'])==1
 assert s['category']=='story' and s['steps'][-1]['contracts']==s['contracts'] and all(t.get('optional') for t in s['steps'][:-1])
stories={s['id']:s for s in mapping['stories']}
assert stories['decker-ticket']['steps'][0]['contracts']==stories['captain-coal-valve']['contracts']
referral={'giver_vnum':38037,'completion_key':'give=I:77209;receive=I:38037,I:77209;disappear=0'}
assert stories['hordine-torn-map']['steps'][0]['contracts']==[referral]
assert [t['item_vnums'] for t in stories['hordine-torn-map']['steps'] if t['kind']=='carried_item']==[[77209],[38037]]
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==226]
assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(4,4,4)
rooms=dawndale_bodies('spshold','wld');objects=dawndale_bodies('spshold','obj');mobiles=dawndale_bodies('spshold','mob')
assert set(rooms)==set(range(22600,22665)) and set(objects)==set(range(22600,22634)) and set(mobiles)==set(range(22600,22644))
resets=spshold['reset_commands']
assert len(resets)==129 and collections.Counter(r['command'] for r in resets)=={'M':76,'E':20,'D':14,'O':7,'G':7,'F':4,'P':1}
assert collections.Counter(r['arguments'][4] for r in resets)=={100:128,80:1}
assert len({(r['command'],tuple(r['arguments'])) for r in resets})==98
parent=None;stock=[]
for r in resets:
 c=r['command'];v=r['arguments']
 if c=='M':parent=(v[1],v[3])
 if c in ('G','E'):stock.append((c,v[1],parent,v[2],v[4]))
assert ('G',22612,(22618,22647),1,100) in stock and ('G',22622,(22623,22649),1,100) in stock
assert ('G',22611,(22624,22662),1,100) in stock and ('G',22602,(22626,22659),1,100) in stock
assert any(r['command']=='P' and r['arguments'][1:4]==[22610,1,22609] for r in resets)
unplaced=set(objects)-{r['arguments'][1] for r in resets if r['command'] in ('O','G','E','P')}
assert unplaced=={22613,22620,22621,22624,22625,22626,22627,22633}
assert objvalues(objects[22609])[0]==15 and objvalues(objects[22609])[7]==0 and objvalues(objects[22609])[11:15]==[1000,29,22611,1000]
for v in (22610,22612,22622):assert objvalues(objects[v])[6]&4096 and objvalues(objects[v])[8]&32768
for v in (22602,22611):assert objvalues(objects[v])[6]&8388608 and objvalues(objects[v])[11+1]==0
assert objvalues(objects[22633])[0]==18 and objvalues(objects[22633])[12]==100
for v,room,target,command in ((22617,22631,22640,5),(22618,22642,22631,6),(22604,22641,22647,7),(22605,22647,22641,7)):
 vals=objvalues(objects[v]);assert vals[0]==25 and vals[7]==0 and vals[11:15]==[target,command,-1,0]
 assert any(r['command']=='O' and r['arguments'][1:4]==[v,1,room] for r in resets)
exits=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(exits)==130 and not any(v==22647 for v,d,f,k,t in exits)
assert [(d,t) for v,d,f,k,t in exits if v==22649 and t not in rooms]==[(0,22255),(1,22113),(2,82540),(3,40717),(5,22423),(6,76616)]
assert not any(t==22649 for v,d,f,k,t in exits)
aphantan=int(mobiles[22623].split('~',4)[4].split()[0])
assert not aphantan&(1<<1) and not aphantan&(1<<6)
foreign=[b for b in inventory_module.native_blocks(ROOT) if b.get('binding')==referral]
assert len(foreign)==1 and foreign[0]['source']=='areas/qst/thetis.qst'
helmsman=[b for b in inventory_module.native_blocks(ROOT) if b.get('giver_vnum')==77218 and ('I',22622) in b['give']]
assert len(helmsman)==1 and helmsman[0]['source']=='areas/qst/jademini.qst' and not helmsman[0]['receive'] and not helmsman[0]['disappear']
jade=dawndale_bodies('jademini','wld');jadeobjects=dawndale_bodies('jademini','obj');thetisobjects=dawndale_bodies('thetis','obj')
assert re.search(r'\bD5\s+[^~]*~[^~]*~\s+6 38037 77262\b',jade[77261],re.S)
assert re.search(r'\bD4\s+[^~]*~[^~]*~\s+7 38037 77261\b',jade[77262],re.S)
assert objvalues(thetisobjects[38037])[0]==18 and objvalues(thetisobjects[38037])[12]==100
assert objvalues(jadeobjects[77210])[0]==15 and objvalues(jadeobjects[77210])[7]==0 and objvalues(jadeobjects[77210])[11:15]==[1000,29,22633,1000]
assign=(ROOT/'src/specs/specs.assign.c').read_text()
assert 'obj_index[real_object0(22621)].func.obj = master_set;' in assign
assert re.search(r'22648.*crew_shop_proc',assign) and not (ROOT/'areas/shp/spshold.shp').exists()

# Harpies: native recognition versus shadowed custom actor transitions.
harpyht=inventory_module.area_evidence(ROOT,'harpyht')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='harpyht')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,2,'complete')
assert len(mapping['stories'])==2 and len(mapping['contacts'])==5 and len(mapping['exclusions'])==1
assert [(c['mob_vnum'],c['topics']) for c in mapping['contacts'] if c['topics']]==[(31108,['undead'])]
assert collections.Counter(t['kind'] for s in mapping['stories'] for t in s['steps'] if t.get('optional'))=={'carried_item':2,'completion':1}
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/harpyht.qst']
expected={2:(31109,[31111],[],False),9:(31118,[31104],[31111],True),18:(31124,[31112],[],False)}
assert len(raw)==3 and all(b['kind']=='Q' for b in raw)
for b in raw:
 assert (b['giver_vnum'],[v for k,v in b['give'] if k=='I'],[v for k,v in b['receive'] if k=='I'],b['disappear'])==expected[b['line']]
 assert sum(b['binding'] in s['contracts'] for s in mapping['stories']+mapping['exclusions'])==1
queen=next(s for s in mapping['stories'] if s['id']=='queen-rescue-proof');dwarf=next(s for s in mapping['stories'] if s['id']=='free-the-chained-dwarf')
assert queen['steps'][0]['contracts']==dwarf['contracts'] and mapping['exclusions'][0]['contracts']==[next(b['binding'] for b in raw if b['line']==18)]
for s in mapping['stories']:assert s['steps'][-1]['contracts']==s['contracts'] and all(t.get('optional') for t in s['steps'][:-1])
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==311];assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(2,2,2)
rooms=dawndale_bodies('harpyht','wld');objects=dawndale_bodies('harpyht','obj');mobiles=dawndale_bodies('harpyht','mob')
assert set(rooms)==set(range(31100,31261)) and set(objects)==set(range(31100,31119)) and set(mobiles)==set(range(31100,31144))
resets=harpyht['reset_commands'];assert len(resets)==176 and collections.Counter(r['command'] for r in resets)=={'M':126,'D':16,'O':13,'G':11,'E':7,'F':2,'P':1}
assert all(r['arguments'][4]==100 for r in resets) and len({(r['command'],tuple(r['arguments'])) for r in resets})==149
assert set(objects)-{r['arguments'][1] for r in resets if r['command'] in ('O','G','E','P')}=={31111}
for v,room in ((31103,31195),(31104,31240),(31102,31207),(31109,31248),(31105,31210)):
 assert any(r['command']=='O' and r['arguments'][1:4]==[v,1,room] for r in resets)
assert any(r['command']=='P' and r['arguments'][1:4]==[31115,1,31109] for r in resets)
assert objvalues(objects[31101])[0]==13 and objvalues(objects[31109])[11:15]==[1000,5,0,1000]
for v,target,cmd in ((31103,31240,7),(31102,31210,340)):
 vals=objvalues(objects[v]);assert vals[0]==25 and vals[7]==0 and vals[11:15]==[target,cmd,-1,0]
for room,target in ((31240,31195),(31210,31207)):
 assert re.search(r'\bD5\s+[^~]*~[^~]*~\s+0 0 '+str(target)+r'\b',rooms[room],re.S)
assert '#define CMD_PULL 340' in (ROOT/'src/cmd/interp.h').read_text() and '#define CMD_CLIMB 556' in (ROOT/'src/cmd/interp.h').read_text()
for v in (31104,31105,31108,31109,31112,31115):assert objvalues(objects[v])[6]&4096
assert not objvalues(objects[31108])[8]&4
exits=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(exits)==357 and [(v,d,f,k,t) for v,d,f,k,t in exits if t not in rooms]==[(31100,2,0,0,620042)]
assert not any(t in (31240,31210) for v,d,f,k,t in exits)
assert int(rooms[31177].split('~')[2].split()[1])&(1<<19)
assign=(ROOT/'src/specs/specs.assign.c').read_text();assert '// obj_index[real_object0(31100)].func.obj = harpy_gate;' in assign
assert {(s['kind'],s['vnum'],s['function']) for s in harpyht['special_assignments']}=={('mob',31103,'money_changer'),('mob',31108,'gargoyle_master'),('mob',31109,'harpy_good'),('mob',31124,'harpy_evil')}
special=(ROOT/'src/cmd/interp.c').read_text().split('bool special(',1)[1]
assert special.index('func.mob)(k, ch, cmd, arg)')<special.index('qst_func)(k, ch, cmd, arg)')
custom=(ROOT/'src/specs/specs.harpy_hometown.c').read_text()
gargoyle=custom.split('int gargoyle_master(',1)[1].split('int harpy_gate(',1)[0]
active=re.sub(r'/\*[\s\S]*?\*/','',gargoyle)
assert '"gargoyle undead"' in active and 'corpse_num < 2' in active and 'NPC_CORPSE' in active and 'strstr(obj->name, "harpy")' in active
assert 'GET_RACE(pl) = RACE_GARGOYLE' not in active and 'extract_obj(' not in active
assert len(re.findall(r'^#\d+~\s*$',(ROOT/'areas/shp/harpyht.shp').read_text(),re.M))==2

# Herders: exact independent bundles, guarded services and current source custody.
herders=inventory_module.area_evidence(ROOT,'herders')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='herders')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==12 and len(mapping['contacts'])==15 and not mapping['exclusions']
assert sum(s['category']=='service' for s in mapping['stories'])==6
assert sum(len(c['topics']) for c in mapping['contacts'])==52
assert sum(t.get('optional',False) for s in mapping['stories'] for t in s['steps'])==19
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/herders.qst' and b['kind'] in ('Q','QA')]
expected={51:(94322,[('I',94311)],[('E',90000)],True),120:(94340,[('I',94318)]+[('I',94305)]*4+[('C',20000)],[('I',94328)],False),131:(94340,[('I',94319)]+[('I',94305)]*4+[('C',20000)],[('I',94329)],False),142:(94340,[('I',94335)]+[('I',94309)]*3,[('I',94341)],False),149:(94340,[('I',94306)]*4+[('C',20000)],[('I',94342)],False),157:(94340,[('I',94378)]+[('I',94306)]*2+[('I',94305)]*3+[('C',35000)],[('I',94379)],False),226:(94375,[('I',94385)],[('I',94386)],True),282:(94376,[('I',94343),('I',94340)],[('I',94351)],False),288:(94376,[('I',94392)]*5,[('I',94393)],False),312:(94413,[('I',94366)],[('E',100000)],True),371:(94429,[('I',94344),('I',94345)],[('I',94374)],True),403:(94441,[('I',94378)],[('I',94395)],False)}
assert len(raw)==len(expected)==12
services={120,131,142,149,157,403}
for b in raw:
 v,g,r,d=expected[b['line']];assert (b['giver_vnum'],sorted(b['give']),sorted(b['receive']),b['disappear'])==(v,sorted(g),sorted(r),d)
 s=next(s for s in mapping['stories'] if b['binding'] in s['contracts'])
 assert len(s['contracts'])==1 and s['category']==('service' if b['line'] in services else 'story')
 assert s['steps'][-1]['contracts']==s['contracts'] and all(t.get('optional') for t in s['steps'][:-1])
 assert {t['item_vnums'][0]:t['count'] for t in s['steps'][:-1]}==dict(collections.Counter(n for k,n in b['give'] if k=='I'))
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==943]
assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(12,6,6)
defs=[d for d in catalog['definitions'] if d['source_area']=='herders']
assert len(defs)==12 and sum(d['daily_eligible'] for d in defs)==8
assert all(d['daily_eligible'] for d in defs if d['giver_vnum'] in (94322,94413))
assert {c['mob_vnum']:set(c['topics']) for c in mapping['contacts']}=={v:set(w for d in herders['dialogue'] if d['giver_vnum']==v for w in d['body'][0].split('~')[0].split()) for v in {b['giver_vnum'] for b in raw}|{d['giver_vnum'] for d in herders['dialogue']}}
rooms=dawndale_bodies('herders','wld');objects=dawndale_bodies('herders','obj');mobiles=dawndale_bodies('herders','mob')
assert set(rooms)==set(range(94300,94647)) and set(objects)==set(range(94300,94397)) and set(mobiles)==set(range(94300,94445))-{94438}
resets=herders['reset_commands'];assert len(resets)==624 and collections.Counter(r['command'] for r in resets)=={'M':341,'D':132,'G':42,'E':35,'O':27,'P':17,'R':16,'F':14}
receiver=None;master=None;custody={}
for r in resets:
 c,a=r['command'],r['arguments']
 if c=='M':master=a[1];receiver=a[1]
 elif c in ('F','R'):receiver=a[1]
 if c in ('G','E'):custody.setdefault(a[1],[]).append((c,master,receiver,a[2],a[4]))
assert custody[94309]==[('G',94315,94317,10,100)] and custody[94389]==[('G',94320,94320,1,100)] and custody[94312]==[('G',94320,94323,1,100)] and custody[94352]==[('E',94430,94379,1,100)]
assert custody[94340]==[('G',94356,94356,1,30)]
assert len(custody[94392])==5 and all(x==('G',94443,94443,5,100) for x in custody[94392])
for v in (94348,94353,94364,94375,94376,94377,94390):assert objvalues(objects[v])[12]==0
for v,cmd,room,direction in ((94301,340,94303,0),(94302,340,94306,2),(94383,270,94509,0),(94391,270,94615,3)):
 vals=objvalues(objects[v]);assert vals[0]==29 and vals[11:15]==[cmd,room,direction,0]
for v,dest in ((94303,94320),(94347,94520)):
 vals=objvalues(objects[v]);assert vals[0]==25 and vals[7]==0 and vals[11:15]==[dest,7,-1,0]
exits=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(exits)==842 and (94623,1,3,94390,94634) in exits and (94634,3,3,0,94623) in exits
assert [(v,d,f,k,t) for v,d,f,k,t in exits if t not in rooms]==[(94300,2,0,0,594610),(94300,3,0,0,594209),(94646,3,0,0,746503)]
assert objvalues(objects[94385])[6]&(1<<23) and objvalues(objects[94385])[6]&4096
assert 'small piece of a bone fragment' in objects[94306] and 'ten tiny dragonkin scales' in re.sub(r'&(?:\+[A-Za-z]|[nNuUbBiIfF])','',objects[94341])
assert not herders['special_assignments']
epic=(ROOT/'src/world/epic.c').read_text();assert 'mob_index[real_mobile(epic_teachers[i].vnum)].func.mob = epic_teacher;' in epic
assert '{ 94364, SKILL_DEVASTATING_CRITICAL, 0, 100, 0, 0, 0 }' in (ROOT/'src/classes/epic_skills.c').read_text()
foreign=next(b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/shipy.qst' and b['line']==559)
assert foreign['giver_vnum']==43156 and foreign['give']==[('I',94307)] and sorted(foreign['receive'])==[('C',200000),('E',155000)] and foreign['disappear']
assert len(re.findall(r'^#\d+~\s*$',(ROOT/'areas/shp/herders.shp').read_text(),re.M))==1

# Jotunheim: independent exact proofs, shared routes and guarded services.
jotun=inventory_module.area_evidence(ROOT,'jotun')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='jotun')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==15 and len(mapping['contacts'])==10 and not mapping['exclusions']
assert sum(s['category']=='service' for s in mapping['stories'])==4
assert sum(len(c['topics']) for c in mapping['contacts'])==47
assert sum(t.get('optional',False) for s in mapping['stories'] for t in s['steps'])==19
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/jotun.qst' and b['kind']=='Q']
expected={31:(96013,[('I',96008)],[('I',96078),('E',80000)],False),39:(96013,[('I',96036),('I',96038),('I',96039),('I',96037),('I',96081)],[('I',96016)],False),48:(96013,[('I',96035)],[('I',96035)],False),53:(96013,[('I',96046)],[('I',96046)],False),78:(96049,[('I',96076)],[('I',96077),('E',50000)],False),110:(96051,[('I',96023)],[('I',96075),('E',100000)],False),134:(96058,[('I',96069),('C',250000)],[('I',96070)],False),144:(96058,[('I',96071),('C',250000)],[('I',96072)],False),189:(96068,[('I',96057)],[('I',96064),('E',100000)],False),199:(96068,[('I',96042)],[('I',96067),('E',600000)],True),237:(96069,[('I',96056)],[('I',96063),('E',150000)],True),285:(96071,[('I',96061)],[('I',96062),('E',100000)],True),319:(96072,[('I',96060)],[('I',96062),('E',100000)],True),367:(96075,[('I',96004)],[('I',96068),('E',175000)],True),421:(96077,[('I',96038)],[('I',96074),('E',175000)],True)}
assert len(raw)==len(expected)==15
services={48,53,134,144}
for b in raw:
 v,g,r,d=expected[b['line']];assert (b['giver_vnum'],sorted(b['give']),sorted(b['receive']),b['disappear'])==(v,sorted(g),sorted(r),d)
 s=next(s for s in mapping['stories'] if b['binding'] in s['contracts'])
 assert len(s['contracts'])==1 and s['category']==('service' if b['line'] in services else 'story')
 assert s['steps'][-1]['contracts']==s['contracts'] and all(t.get('optional') for t in s['steps'][:-1])
 assert {t['item_vnums'][0]:t['count'] for t in s['steps'][:-1]}==dict(collections.Counter(n for k,n in b['give'] if k=='I'))
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==960]
assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(15,11,11)
defs=[d for d in catalog['definitions'] if d['source_area']=='jotun'];assert len(defs)==15 and sum(d['daily_eligible'] for d in defs)==11
assert len(jotun['dialogue'])==34
assert {c['mob_vnum']:set(c['topics']) for c in mapping['contacts']}=={v:set(w for d in jotun['dialogue'] if d['giver_vnum']==v for w in d['body'][0].split('~')[0].split()) for v in {b['giver_vnum'] for b in raw}}
allblocks=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/jotun.qst']
assert sum(b['kind']=='M' for b in allblocks)==36 and sum(b['kind']=='M' and b['body'][0].startswith('qc_action 50') for b in allblocks)==2
rooms=dawndale_bodies('jotun','wld');objects=dawndale_bodies('jotun','obj');mobiles=dawndale_bodies('jotun','mob')
assert set(rooms)==set(range(96000,96296)) and set(objects)==set(range(96000,96082)) and set(mobiles)==set(range(96000,96078))-{96031}
resets=jotun['reset_commands'];assert len(resets)==392 and collections.Counter(r['command'] for r in resets)=={'M':201,'E':117,'D':50,'G':13,'O':6,'F':5}
assert jotun['zone']['reset_mode']==1
assert [(r['command'],r['arguments']) for r in resets if r['arguments'][4]!=100]==[('E',[1,96000,1,16,40,0,0,0])]
receiver=None;room=None;custody={}
for r in resets:
 c,a=r['command'],r['arguments']
 if c=='M':receiver=a[1];room=a[3]
 elif c=='F':receiver=a[1]
 if c in ('G','E'):custody.setdefault(a[1],[]).append((c,receiver,room,a[2],a[4]))
assert custody[96069]==[('G',96007,96026,2,100),('G',96007,96048,2,100)]
assert custody[96071]==[('G',96076,96288,1,100)] and custody[96023]==[('G',96012,96032,1,100)]
assert custody[96038]==[('E',96030,96183,1,100)] and custody[96057]==[('E',96069,96292,1,100)]
assert [(r['arguments'][1],r['arguments'][2],r['arguments'][3]) for r in resets if r['command']=='O' and r['arguments'][1] in (96076,96081)]==[(96076,1,96072),(96081,1,96180)]
assert not any(r['command'] in ('G','E','O') and r['arguments'][1] in (96026,96080) for r in resets)
assert not any(r['command']=='M' and r['arguments'][1]==96066 for r in resets)
for v in (96006,96007,96014):assert objvalues(objects[v])[12]==0
assert objvalues(objects[96023])[11:15]==[500,500,3,0]
assert objvalues(objects[96076])[6]&(1<<23) and not objvalues(objects[96081])[6]&4096
exits=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(exits)==746 and [(v,d,f,k,t) for v,d,f,k,t in exits if t not in rooms]==[(96004,5,0,0,-1)]
for v,d,t in ((96197,2,96198),(96198,0,96197),(96215,1,96252),(96252,3,96215)):assert any(x[0]==v and x[1]==d and x[3]==-2 and x[4]==t for x in exits)
assert not any(v==96213 and d==2 for v,d,f,k,t in exits)
assert {(d,t) for v,d,f,k,t in exits if v==96295}=={(0,96040),(1,96104),(2,96088),(3,96132),(4,96214),(5,96153)}
assert not any(v==96294 for v,d,f,k,t in exits)
assert any(v==96189 and d==0 and k==96007 and t==96190 for v,d,f,k,t in exits)
assert any(v==96190 and d==2 and k==0 and t==96189 for v,d,f,k,t in exits)
assert any(v==96276 and d==2 and t==96274 for v,d,f,k,t in exits)
assert objvalues(objects[96005])[11:15]==[19733,7,-1,0]
epic=(ROOT/'src/world/epic.c').read_text();assert 'mob_index[real_mobile(epic_teachers[i].vnum)].func.mob = epic_teacher;' in epic
assert '{ 96013, SKILL_SUMMON_BLIZZARD, 0, 100, 0, 0, 0 }' in (ROOT/'src/classes/epic_skills.c').read_text()
assert '{ 96058, { 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 119, -1 } }' in (ROOT/'src/economy/tradeskill.c').read_text()
assert not (ROOT/'areas/shp/jotun.shp').exists()
foreign=[b for b in inventory_module.native_blocks(ROOT) if b['kind'] in ('Q','QA') and b['source']!='areas/qst/jotun.qst' and any(k=='I' and 96000<=n<=96081 for k,n in b['give']+b['receive'])]
assert len(foreign)==2
mundorno=next(b for b in foreign if b['giver_vnum']==83342);assert mundorno['line']==5707 and sorted(mundorno['give'])==[('C',100000),('I',83191),('I',83377)] and mundorno['receive']==[('I',96021)]
fearfrost=next(b for b in foreign if b['giver_vnum']==131637);assert fearfrost['line']==240 and sorted(fearfrost['give'])==[('I',96000),('I',96012),('I',96055),('I',131647)] and fearfrost['receive']==[('I',131648)]

# Temple of Flames: six independent receipts, exact bundles and shared access.
temple=inventory_module.area_evidence(ROOT,'temple')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='temple')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==6 and len(mapping['contacts'])==9 and not mapping['exclusions']
assert all(s['category']=='story' for s in mapping['stories'])
assert sum(len(c['topics']) for c in mapping['contacts'])==42
assert sum(t.get('optional',False) for s in mapping['stories'] for t in s['steps'])==8
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/temple.qst' and b['kind']=='Q']
expected={20:(18302,[('I',18300)],[('I',18301),('I',18302)],True),87:(18310,[('I',18309)],[],True),233:(18325,[('I',18322),('I',18322),('I',18324),('I',18325)],[('C',500000)],False),273:(18331,[('I',18336)],[('I',18337),('I',18337)],True),294:(18336,[('I',18307)],[('I',18316)],True),325:(18337,[('I',18339)],[],False)}
assert len(raw)==len(expected)==6
for b in raw:
 v,g,r,d=expected[b['line']];assert (b['giver_vnum'],sorted(b['give']),sorted(b['receive']),b['disappear'])==(v,sorted(g),sorted(r),d)
 s=next(s for s in mapping['stories'] if b['binding'] in s['contracts'])
 assert len(s['contracts'])==1 and s['steps'][-1]['contracts']==s['contracts']
 assert all(t.get('optional') for t in s['steps'][:-1])
 assert {t['item_vnums'][0]:t['count'] for t in s['steps'][:-1]}==dict(collections.Counter(n for k,n in b['give'] if k=='I'))
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==183]
assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(6,6,6)
defs=[d for d in catalog['definitions'] if d['source_area']=='temple'];assert len(defs)==6 and all(d['daily_eligible'] for d in defs)
assert len(temple['dialogue'])==33
assert {c['mob_vnum']:set(c['topics']) for c in mapping['contacts']}=={v:set(w for d in temple['dialogue'] if d['giver_vnum']==v for w in d['body'][0].split('~')[0].split()) for v in {d['giver_vnum'] for d in temple['dialogue']}}
rooms=dawndale_bodies('temple','wld');objects=dawndale_bodies('temple','obj');mobiles=dawndale_bodies('temple','mob')
assert set(rooms)==set(range(18300,18623)) and set(objects)==set(range(18300,18354))-{18338} and set(mobiles)==set(range(18300,18351))
resets=temple['reset_commands'];assert len(resets)==346 and collections.Counter(r['command'] for r in resets)=={'M':206,'D':52,'O':41,'E':33,'G':12,'P':2}
assert temple['zone']['reset_mode']==1 and all(r['arguments'][4]==100 for r in resets)
receiver=None;room=None;custody={}
for r in resets:
 c,a=r['command'],r['arguments']
 if c=='M':receiver=a[1];room=a[3]
 if c in ('G','E'):custody.setdefault(a[1],[]).append((c,receiver,room,a[2],a[4]))
assert custody[18300]==[('G',18320,18621,1,100)] and custody[18309]==[('E',18308,18343,1,100)]
assert custody[18322]==[('E',18321,18409,2,100),('E',18321,18409,2,100)]
assert custody[18324]==[('G',18323,18414,1,100)] and custody[18325]==[('E',18324,18420,1,100)]
assert custody[18307]==[('E',18307,18337,1,100)] and custody[18339]==[('G',18339,18578,1,100)]
assert {(r['arguments'][1],r['arguments'][2],r['arguments'][3]) for r in resets if r['command']=='P'}=={(18312,1,18307),(18336,1,18315)}
assert objvalues(objects[18307])[11:15]==[4,5,0,4] and objvalues(objects[18315])[11:15]==[500,9,18316,500]
assert not objvalues(objects[18315])[12]&4 and objvalues(objects[18315])[12]&8
assert objvalues(objects[18316])[12]==100 and objvalues(objects[18337])[11:15]==[50,56,0,0]
assert objvalues(objects[18326])[11:15]==[60,60,28,0]
assert objvalues(objects[18327])[11:15]==[18432,139,-1,0] and objvalues(objects[18329])[11:15]==[18431,139,-1,0]
for v in (18300,18307,18312,18316,18336):assert objvalues(objects[v])[6]&(1<<23)
assert objects[18322].split('~')[0].strip()=='yellow dagger' and objects[18325].split('~')[0].strip()=='wooden sword blue'
assert 'golden locket' in objects[18307] and 'gem cage soul' in objects[18300]
exits=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(exits)==803 and [(v,d,f,k,t) for v,d,f,k,t in exits if t not in rooms]==[(18300,2,0,0,587801)]
for v,d,t in ((18362,2,18363),(18426,1,18427),(18561,1,18564)):assert any(a==v and b==d and k==-2 and dest==t for a,b,f,k,dest in exits)
assert [(v,d,t) for v,d,f,k,t in exits if t in (18576,18577,18578) and v not in (18576,18577,18578)]==[(18583,5,18576)]
assert not any(18579<=t<=18583 and not 18579<=v<=18583 for v,d,f,k,t in exits)
assert any(v==18578 and d==1 and t==18569 for v,d,f,k,t in exits)
for v in (18576,18577,18578):
 header=rooms[v].split('~',2)[2].strip().splitlines()[0].split();assert list(map(int,header))==[183,33554432,0]
 assert not int(header[1])&((1<<7)|(1<<9)|(1<<15))
foreign=[b for b in inventory_module.native_blocks(ROOT) if b['kind'] in ('Q','QA') and b['source']!='areas/qst/temple.qst' and any(k=='I' and 18300<=n<=18353 for k,n in b['give']+b['receive'])]
assert len(foreign)==1 and foreign[0]['giver_vnum']==77742 and foreign[0]['line']==223 and foreign[0]['give']==[('I',18309)] and foreign[0]['receive']==[('I',77743)]
assert not (ROOT/'areas/shp/temple.shp').exists()

# Pharr Valley Swamp: one shard story, three paid services and exact stock.
pods=inventory_module.area_evidence(ROOT,'pods')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='pods')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==4 and len(mapping['contacts'])==5 and not mapping['exclusions']
assert sum(s['category']=='story' for s in mapping['stories'])==1 and sum(s['category']=='service' for s in mapping['stories'])==3
assert sum(len(c['topics']) for c in mapping['contacts'])==52
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/pods.qst' and b['kind'] in ('Q','QA')]
expected={33:(28533,[('I',28579)],[('C',75000)],'story'),39:(28533,[('C',50000)],[('I',28598)],'service'),187:(28576,[('I',28554)]*6+[('I',28555),('C',1000)],[('I',28552)],'service'),199:(28576,[('I',28555)]*3+[('C',500)],[('I',28553)],'service')}
assert len(raw)==len(expected)==4
for b in raw:
 v,g,r,category=expected[b['line']]
 assert (b['giver_vnum'],sorted(b['give']),sorted(b['receive']),b['disappear'])==(v,sorted(g),sorted(r),False)
 s=next(s for s in mapping['stories'] if b['binding'] in s['contracts'])
 assert s['category']==category and len(s['contracts'])==1 and s['steps'][-1]['contracts']==s['contracts']
 assert all(t.get('optional') for t in s['steps'][:-1])
 assert {t['item_vnums'][0]:t['count'] for t in s['steps'][:-1]}==dict(collections.Counter(n for k,n in b['give'] if k=='I'))
 assert (category!='service') or 'unavailable while accounting is active' in s['steps'][-1]['hint']
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==285]
assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(4,1,1)
assert len(pods['dialogue'])==24 and collections.Counter(d['kind'] for d in pods['dialogue'])=={'M':18,'MA':6}
assert {c['mob_vnum']:set(c['topics']) for c in mapping['contacts']}=={v:set(w for d in pods['dialogue'] if d['giver_vnum']==v for w in d['body'][0].split('~')[0].split()) for v in {d['giver_vnum'] for d in pods['dialogue']}}
rooms=dawndale_bodies('pods','wld');objects=dawndale_bodies('pods','obj');mobiles=dawndale_bodies('pods','mob')
assert set(rooms)==set(range(28500,28875)) and set(objects)==set(range(28500,28599)) and set(mobiles)==set(range(28500,28603))
resets=pods['reset_commands'];assert len(resets)==1349 and collections.Counter(r['command'] for r in resets)=={'M':1015,'O':145,'G':118,'E':25,'D':18,'P':18,'F':10}
assert (pods['zone']['first_vnum'],pods['zone']['last_vnum'],pods['zone']['reset_mode'])==(28300,28874,2)
assert collections.Counter(r['arguments'][4] for r in resets)=={100:1315,33:33,35:1}
receiver=None;room=None;custody={}
for r in resets:
 c,a=r['command'],r['arguments']
 if c in ('M','F'):receiver=a[1];room=a[3]
 if c in ('G','E'):custody[r['line']]=(a[1],receiver,room,a[2],a[4])
assert custody[1259]==(28579,28587,28842,1,100)
assert custody[771]==(28598,28533,28703,1,100)
assert custody[543]==(28592,28600,28611,1,100) and custody[545]==(28597,28602,28611,1,100)
assert custody[567]==(28583,28591,28635,1,100) and custody[569]==(28584,28592,28635,1,100)
assert custody[614]==(28586,28593,28649,1,100) and custody[670]==(28588,28596,28662,1,100)
assert custody[774]==(28580,28534,28703,1,100)
assert {x[1] for x in custody.values() if x[0]==28554}=={28537}
assert {x[1] for x in custody.values() if x[0]==28555}=={28523,28524,28525,28526,28537}
assert all(x[3:]==(25,100) for x in custody.values() if x[0]==28554)
assert all(x[3:]==(47,100) for x in custody.values() if x[0]==28555)
assert {(r['arguments'][1],r['arguments'][2],r['arguments'][3]) for r in resets if r['command']=='P' and r['arguments'][1] in (28554,28555,28576)}=={(28554,25,28593),(28555,47,28593),(28576,1,28575)}
assert objvalues(objects[28575])[11:15]==[200,15,28598,20] and not objvalues(objects[28575])[12]&16
assert objvalues(objects[28598])[0]==18 and objvalues(objects[28598])[12]==100
assert objvalues(objects[28553])[19:]==[3,8450,100,2048,0,0,0]
controls={v:objvalues(b)[11:15] for v,b in objects.items() if objvalues(b)[0]==29}
assert controls=={28514:[340,28603,1,0],28516:[340,28605,2,0],28524:[29,28706,1,0],28525:[270,28707,3,1],28534:[340,28774,0,1],28535:[340,28775,2,1],28536:[340,28777,0,1],28537:[340,28794,2,1],28538:[340,28793,1,1],28539:[340,28563,3,1],28540:[340,28806,1,1],28541:[340,28818,3,1]}
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==754 and [(v,d,f,k,t) for v,d,f,k,t in edges if t not in rooms]==[(28500,3,0,0,40389)]
assert all(any(v==room and d==direction and f==(5 if room==28707 else 13) and k==0 for v,d,f,k,t in edges) for command,room,direction,mode in controls.values())
foreign=[b for b in inventory_module.native_blocks(ROOT) if b['kind'] in ('Q','QA') and b['source']!='areas/qst/pods.qst' and any(k=='I' and 28500<=n<=28598 for k,n in b['give']+b['receive'])]
assert len(foreign)==1 and foreign[0]['giver_vnum']==97901 and foreign[0]['line']==33 and sorted(foreign[0]['give'])==[('I',28552),('I',28553)] and foreign[0]['receive']==[('I',97903)]
assert set(int(n) for n in re.findall(r'^#(\d+)~\s*$',(ROOT/'areas/shp/pods.shp').read_text(),re.M))=={28566,28575,28582,28586}

# The Citadel: preserve exact independent deliveries and exclude the empty placeholder.
citadel=inventory_module.area_evidence(ROOT,'citadel')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='citadel')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==2 and len(mapping['contacts'])==20 and len(mapping['exclusions'])==1
assert sum(len(c['topics']) for c in mapping['contacts'])==63
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/citadel.qst' and b['kind'] in ('Q','QA')]
assert [(b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[(47,13003,[('I',13006)],[('C',50000)],True),(260,13030,[('I',13033)],[('I',13059)],False),(267,13030,[('I',0)],[],False)]
assert mapping['exclusions'][0]['contracts']==[raw[-1]['binding']]
for b in raw[:-1]:
 s=next(s for s in mapping['stories'] if b['binding'] in s['contracts'])
 assert s['category']=='story' and s['steps'][-1]['contracts']==s['contracts']==[b['binding']]
 assert len(s['steps'])==2 and s['steps'][0]['optional'] and s['steps'][0]['kind']=='carried_item' and s['steps'][0]['item_vnums']==[b['give'][0][1]] and s['steps'][0]['count']==1
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==130]
assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(2,2,2)
assert len(citadel['dialogue'])==39 and all(d['kind']=='M' for d in citadel['dialogue'])
assert {c['mob_vnum']:set(c['topics']) for c in mapping['contacts']}=={v:set(w for d in citadel['dialogue'] if d['giver_vnum']==v for w in d['body'][0].split('~')[0].split()) for v in {d['giver_vnum'] for d in citadel['dialogue']}}
rooms=dawndale_bodies('citadel','wld');objects=dawndale_bodies('citadel','obj');mobiles=dawndale_bodies('citadel','mob')
assert set(rooms)==set(range(13000,13200)) and set(objects)==set(range(13000,13063)) and set(mobiles)==set(range(13000,13031))
assert (citadel['zone']['first_vnum'],citadel['zone']['last_vnum'],citadel['zone']['reset_mode'])==(12973,13199,1)
rs=citadel['reset_commands'];assert len(rs)==343 and collections.Counter(r['command'] for r in rs)=={'D':84,'O':30,'P':22,'M':172,'E':30,'G':5}
assert collections.Counter(r['arguments'][4] for r in rs)=={100:341,35:2}
assert not any(r['command']=='M' and r['arguments'][1]==13025 for r in rs)
for line,command,args in ((346,'O',[0,13005,1,13029,100,0,0,0]),(348,'P',[1,13033,1,13005,100,0,0,0]),(389,'O',[0,13058,1,13189,100,0,0,0]),(390,'P',[1,13060,1,13058,35,0,0,0]),(391,'P',[1,13061,1,13058,35,0,0,0]),(392,'P',[1,13062,1,13058,100,0,0,0]),(495,'M',[0,13010,1,13105,100,0,0,0]),(496,'E',[1,13007,1,12,100,0,0,0]),(497,'P',[1,13006,1,13007,100,0,0,0]),(585,'M',[0,13030,1,13188,100,0,0,0])):
 assert any(r['line']==line and r['command']==command and r['arguments']==args for r in rs)
assert objvalues(objects[13005])[11:15]==[2000,15,13006,0] and objvalues(objects[13058])[11:15]==[100,29,13059,100]
assert objvalues(objects[13006])[12]==0 and objvalues(objects[13041])[12]==10 and objvalues(objects[13059])[12]==100
edges=[(v,int(m[1]),m[3].strip(),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==475 and [(v,d,f,k,t) for v,d,w,f,k,t in edges if t not in rooms]==[(13000,2,0,0,12796)]
assert [(v,d,w.split()[-1]) for v,d,w,f,k,t in edges if k==-2]==[(13000,0,'paradox'),(13003,2,'paradox'),(13129,0,'wind'),(13131,1,'dark'),(13132,3,'dark'),(13134,2,'wind'),(13145,0,'black'),(13146,1,'time'),(13147,3,'time'),(13150,2,'black'),(13185,1,'shalafi'),(13186,3,'shalafi')]
assert "Cry 'master' and enter." in rooms[13185]
assert set(r['arguments'][1] for r in rs if r['command']=='P' and r['arguments'][1] in (13001,13002,13003,13004,13025,13026,13027,13028,13029,13030,13031,13036))=={13001,13002,13003,13004,13025,13026,13027,13028,13029,13030,13031,13036}
assert not any(objvalues(b)[0] in (25,29) for b in objects.values())
assert not (ROOT/'areas/shp/citadel.shp').exists()
foreign=[b for b in inventory_module.native_blocks(ROOT) if b['kind'] in ('Q','QA') and b['source']!='areas/qst/citadel.qst' and any(k=='I' and 13000<=n<=13062 for k,n in b['give']+b['receive'])]
assert not foreign

# The Elemental Groves: exact independent offerings and separate access/treasure sources.
element=inventory_module.area_evidence(ROOT,'element')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='element')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==3 and len(mapping['contacts'])==20 and not mapping['exclusions']
assert sum(len(c['topics']) for c in mapping['contacts'])==84
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/element.qst' and b['kind'] in ('Q','QA')]
assert [(b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[(385,3819,[('I',3831)],[('I',3830),('E',200000)],True),(401,3819,[('I',3808)],[('E',20000)],False),(408,3819,[('I',3809)],[('E',40000)],False)]
for b in raw:
 s=next(s for s in mapping['stories'] if b['binding'] in s['contracts'])
 assert s['category']=='story' and s['steps'][-1]['contracts']==s['contracts']==[b['binding']]
 assert len(s['steps'])==2 and s['steps'][0]['optional'] and s['steps'][0]['kind']=='carried_item' and s['steps'][0]['item_vnums']==[b['give'][0][1]] and s['steps'][0]['count']==1
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==38]
assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(3,3,3)
assert len(element['dialogue'])==60 and all(d['kind']=='M' for d in element['dialogue'])
assert {c['mob_vnum']:set(c['topics']) for c in mapping['contacts']}=={v:set(w for d in element['dialogue'] if d['giver_vnum']==v for w in d['body'][0].split('~')[0].split()) for v in {d['giver_vnum'] for d in element['dialogue']}}
rooms=dawndale_bodies('element','wld');objects=dawndale_bodies('element','obj');mobiles=dawndale_bodies('element','mob')
assert len(rooms)==165 and set(objects)==set(range(3800,3838)) and set(mobiles)==set(range(3800,3823))
assert (element['zone']['first_vnum'],element['zone']['last_vnum'],element['zone']['reset_mode'])==(3752,3974,1)
rs=element['reset_commands'];assert len(rs)==144 and collections.Counter(r['command'] for r in rs)=={'D':34,'O':14,'P':8,'M':63,'E':20,'G':5}
assert collections.Counter(r['arguments'][4] for r in rs)=={100:142,60:2}
for command,args in (('O',[0,3808,1,3854,100,0,0,0]),('O',[0,3809,1,3823,100,0,0,0]),('M',[0,3819,1,3851,100,0,0,0]),('M',[0,3820,1,3950,100,0,0,0]),('E',[1,3831,1,18,100,0,0,0]),('O',[0,3832,1,3967,100,0,0,0]),('P',[1,3833,1,3832,100,0,0,0]),('P',[1,3817,1,3832,100,0,0,0])):
 assert any(r['command']==command and r['arguments']==args for r in rs)
parent=None;stock=[]
for r in rs:
 if r['command']=='M':parent=r['arguments'][1],r['arguments'][3]
 if r['command'] in ('E','G'):stock.append((parent,r['command'],r['arguments'][1]))
assert ((3820,3950),'E',3831) in stock
assert all(objvalues(objects[v])[12]==0 for v in range(3800,3816))
assert all(objvalues(objects[v])[8]&32768 for v in (3808,3809,3831))
assert objvalues(objects[3831])[0]==12 and objvalues(objects[3832])[0]==15 and objvalues(objects[3832])[7]==0 and objvalues(objects[3832])[11:15]==[200,5,0,100]
assert objvalues(objects[3818])[11:15]==objvalues(objects[3819])[11:15]==[100,5,-1,250]
assert [r['arguments'][3] for r in rs if r['command']=='O' and r['arguments'][1]==3818]==[3897,3908,3920,3927,3952]
assert [(r['arguments'][1],r['arguments'][3]) for r in rs if r['command']=='P' and 3810<=r['arguments'][1]<=3815]==[(v,3818 if v<3815 else 3819) for v in range(3810,3816)]
edges=[(v,int(m[1]),m[3].strip(),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==384 and [(v,d,f,k,t) for v,d,w,f,k,t in edges if t not in rooms]==[(3800,1,0,0,531681)]
for v,d,k,t in ((3951,4,3808,3952),(3952,5,3808,3951),(3947,4,3809,3953),(3953,5,3809,3947)):
 assert any((a,b,c,e)==(v,d,k,t) and f==3 for a,b,w,f,c,e in edges)
assert [(v,d,k,t) for v,d,w,f,k,t in edges if v in range(3960,3966) and d==0]==[(3960+i,0,3810+i,3961+i) for i in range(6)]
assert [(v,d,w.split()[-1],t) for v,d,w,f,k,t in edges if k==-2]==[(3966,0,'nothing',3967),(3967,2,'nothing',3966)]
assert any((v,d,k,t)==(3974,0,0,3812) for v,d,w,f,k,t in edges) and any((v,d,k,t)==(3892,1,0,3892) for v,d,w,f,k,t in edges)
assert [int(re.search(r'\bF\s+(\d+)',rooms[v])[1]) for v in range(3938,3943)]==[2,4,10,6,8]
assert all(int(rooms[v].split('~',2)[2].strip().splitlines()[0].split()[2])==8 for v in list(range(3932,3938))+[3926])
assert objvalues(objects[3837])[22]==2048 and objvalues(objects[3830])[22]==536870912 and len(objvalues(objects[3822]))==22
assert all(objvalues(b)[0] not in (25,29) for b in objects.values())
assert not (ROOT/'areas/shp/element.shp').exists()
assert 'obj_index[real_object0(3833)].func.obj = glades_dagger;' in (ROOT/'src/specs/specs.assign.c').read_text()
assert not any(d['giver_vnum'] in (3820,3821,3822) for d in element['dialogue'])
assert all('active' in s['steps'][-1]['hint'] and 'accounting' in s['steps'][-1]['hint'] for s in mapping['stories'])

# Temple of the Earth: independent accepted patrol proof versus separate native missions.
earth=inventory_module.area_evidence(ROOT,'earth')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='earth')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==2 and len(mapping['contacts'])==12 and not mapping['exclusions']
assert sum(len(c['topics']) for c in mapping['contacts'])==46
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/earth.qst' and b['kind'] in ('Q','QA')]
assert [(b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[(27,43502,[('I',43525)],[('I',43561)],True),(104,43509,[('I',43539)],[('I',43562)],True)]
for b in raw:
 s=next(s for s in mapping['stories'] if b['binding'] in s['contracts'])
 assert s['category']=='story' and len(s['steps'])==2 and s['steps'][-1]['contracts']==s['contracts']==[b['binding']]
 assert s['steps'][0]['optional'] and s['steps'][0]['kind']=='carried_item' and s['steps'][0]['item_vnums']==[b['give'][0][1]] and s['steps'][0]['count']==1
 assert 'accounting' in s['steps'][-1]['hint'] and 'active' in s['steps'][-1]['hint']
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==435]
assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(2,2,0)
assert len(earth['dialogue'])==22 and collections.Counter(d['kind'] for d in earth['dialogue'])=={'M':21,'MA':1}
assert {c['mob_vnum']:set(c['topics']) for c in mapping['contacts']}=={v:set(w for d in earth['dialogue'] if d['giver_vnum']==v for w in d['body'][0].split('~')[0].split()) for v in {d['giver_vnum'] for d in earth['dialogue']}}
rooms=dawndale_bodies('earth','wld');objects=dawndale_bodies('earth','obj');mobiles=dawndale_bodies('earth','mob')
assert set(rooms)==set(range(43500,43698)) and set(objects)==set(range(43500,43595)) and set(mobiles)==set(range(43500,43605))
assert (earth['zone']['first_vnum'],earth['zone']['last_vnum'],earth['zone']['reset_mode'])==(43329,43697,0)
rs=earth['reset_commands'];assert len(rs)==356 and collections.Counter(r['command'] for r in rs)=={'D':56,'O':51,'P':7,'M':165,'E':43,'G':12,'F':22}
assert collections.Counter(r['arguments'][4] for r in rs)=={100:348,25:4,20:1,40:1,60:1,80:1}
for command,args in (('M',[0,43502,1,43572,100,0,0,0]),('M',[0,43509,1,43512,100,0,0,0]),('M',[0,43504,1,43549,100,0,0,0]),('O',[0,43538,1,43628,100,0,0,0]),('P',[1,43539,1,43538,100,0,0,0]),('O',[0,43584,1,43550,100,0,0,0]),('O',[0,43595,1,43550,100,0,0,0]),('O',[0,43515,1,43611,100,0,0,0]),('O',[0,43583,1,43652,100,0,0,0]),('M',[0,43576,1,43682,100,0,0,0])):
 assert any(r['command']==command and r['arguments']==args for r in rs)
# Dialogue-only hydra sourcing must not become a fabricated badge drop.
assert all(not re.search(r'^[OPGE]\s+\d+\s+43525\b',p.read_text(encoding='utf8'),re.M) for p in (ROOT/'areas/zon').glob('*.zon'))
assert 'source for this badge is unresolved' in mapping['stories'][0]['steps'][0]['hint']
assert objvalues(objects[43538])[0]==15 and objvalues(objects[43538])[7]==0 and objvalues(objects[43538])[11:15]==[199,0,0,199]
assert objvalues(objects[43525])[0]==13 and objvalues(objects[43539])[0]==9 and all(objvalues(objects[v])[8]&32768 for v in (43525,43539))
assert 'Melkivar' in objects[43562] and 'Torm' in raw[1]['body'][2]
for v,room,cmd in ((43506,43594,320),(43515,43652,7),(43528,43536,320),(43580,43502,7),(43582,43502,7),(43583,43611,7)):
 assert objvalues(objects[v])[0]==25 and objvalues(objects[v])[11:14]==[room,cmd,-1]
assert [(r['arguments'][1],r['arguments'][3],r['arguments'][4]) for r in rs if r['command']=='O' and 43585<=r['arguments'][1]<=43589]==[(43585+i,43697,20*(i+1)) for i in range(5)]
assert all(objects[43590+i].split('~',1)[0]==word+'button' and objects[43585+i].split('~',1)[0]==word for i,word in enumerate(('limestone','obsidian','granite','adamantite','jade')))
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==636 and [(v,d,f,k,t) for v,d,f,k,t in edges if t not in rooms]==[(43500,3,0,0,804038),(43571,1,0,0,-1)]
assert (43550,0,13,-1,43551) in edges and (43601,3,6,43542,43614) in edges
assert [(v,d) for v,d,f,k,t in edges if v==43589 and t==43589]==[(43589,0),(43589,2)]
assert objvalues(objects[43542])[0]==13 and objvalues(objects[43584])[11:14]==[340,43550,0]
assert {a['function'] for a in earth['special_assignments']}=={'toe_chamber_switch','eligoth_rift_spawn','patrol_shops'}
assert 43341 not in rooms and 'Captain' in dawndale_bodies('aopal','wld')[43341]
assert not (ROOT/'areas/shp/earth.shp').exists()

# Githzerai Stronghold: independent accepted bundles and honest access context.
githzer=inventory_module.area_evidence(ROOT,'githzer')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='githzer')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==13 and len(mapping['contacts'])==8 and not mapping['exclusions']
assert sum(len(c['topics']) for c in mapping['contacts'])==35
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/githzer.qst' and b['kind'] in ('Q','QA')]
expected={9:(44402,[('I',44401)],[('I',44515)],False),41:(44422,[('I',44422),('I',44428),('I',44429)],[('I',44431),('I',44516)],False),55:(44422,[('C',500000)],[('I',44516)],False),60:(44422,[('I',44512),('I',44401),('I',44470)],[('I',44516)],False),69:(44422,[('I',44509)],[],False),87:(44431,[('I',44436),('I',44440),('I',44440)],[('I',44439)],False),94:(44431,[('I',44437),('I',44441),('I',44441)],[('I',44438)],False),107:(44437,[('I',44434),('I',44444),('I',44448)],[('I',44456)],False),139:(44461,[('I',44446)],[('I',44501)],True),189:(44465,[('I',44427)],[('I',44519)],False),198:(44465,[('I',44550)],[('I',44577)],False),212:(44494,[('I',44580)],[('I',44581)],True),283:(44509,[('I',44563)]*5,[('I',44572)],True)}
assert {b['line']:(b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw}==expected
for b in raw:
 entry=next(s for s in mapping['stories'] if b['binding'] in s['contracts'])
 assert entry['category']==('service' if b['line'] in (55,69) else 'story') and entry['steps'][-1]['contracts']==entry['contracts']==[b['binding']]
 assert entry['steps'][-1]['kind']=='completion' and all(t['optional'] and t['kind']=='carried_item' for t in entry['steps'][:-1])
 assert {t['item_vnums'][0]:t['count'] for t in entry['steps'][:-1]}==dict(collections.Counter(v for k,v in b['give'] if k=='I'))
 assert 'active' in entry['steps'][-1]['hint'] and 'accounting' in entry['steps'][-1]['hint']
assert sum(t.get('optional',False) for s in mapping['stories'] for t in s['steps'])==20
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==444]
assert (len(units),sum(u['achievement'] for u in units),sum(u['daily_candidate'] for u in units))==(13,11,11)
entries={s['id']:s for s in mapping['stories']}
assert len(entries['zangzk-paid-key']['steps'])==1 and 'currently unavailable' in entries['zangzk-paid-key']['summary']
assert 'no reward item' in entries['zangzk-adamantite-information']['summary']
assert len(githzer['dialogue'])==20
topics={v:set(w for d in githzer['dialogue'] if d['giver_vnum']==v for w in d['body'][0].split('~')[0].split()) for v in {d['giver_vnum'] for d in githzer['dialogue']}}
topics[44437]=set()
assert {c['mob_vnum']:set(c['topics']) for c in mapping['contacts']}==topics
ambient=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/githzer.qst' and b['kind']=='M' and b['body'][0].startswith('qc_action')]
assert [(b['giver_vnum'],b['body'][0]) for b in ambient]==[(44437,'qc_action 30~'),(44461,'qc_action 25~')]
rooms=dawndale_bodies('githzer','wld');objects=dawndale_bodies('githzer','obj');mobiles=dawndale_bodies('githzer','mob')
assert set(rooms)==set(range(44400,44786))-{44582,44750,44751,44756,44757}
assert set(objects)==set(range(44400,44582)) and set(mobiles)==set(range(44400,44524))
assert (githzer['zone']['zone_number'],githzer['zone']['first_vnum'],githzer['zone']['last_vnum'],githzer['zone']['reset_mode'])==(444,44335,44785,1)
rs=githzer['reset_commands']
assert len(rs)==590 and collections.Counter(r['command'] for r in rs)=={'D':108,'O':56,'P':17,'M':207,'G':61,'F':30,'E':110,'R':1}
assert collections.Counter(r['arguments'][4] for r in rs)=={100:589,80:1}
for command,args in (('M',[0,44402,3,44404,100,0,0,0]),('M',[0,44422,1,44462,100,0,0,0]),('M',[0,44431,1,44541,100,0,0,0]),('M',[0,44437,1,44519,100,0,0,0]),('M',[0,44461,1,44589,100,0,0,0]),('M',[0,44462,1,44590,100,0,0,0]),('M',[0,44465,1,44612,100,0,0,0]),('M',[0,44494,1,44665,100,0,0,0]),('M',[0,44509,1,44729,100,0,0,0]),('R',[1,44488,1,44565,100,0,0,0])):
 assert any(r['command']==command and r['arguments']==args for r in rs)
assert [r['arguments'][3] for r in rs if r['command']=='O' and r['arguments'][1]==44561]==[44752,44753,44754,44755,44758]
assert len([r for r in rs if r['command']=='P' and r['arguments'][1]==44563 and r['arguments'][2]==5 and r['arguments'][3]==44561])==5
assert objvalues(objects[44561])[0]==15 and objvalues(objects[44561])[12:14]==[5,0]
assert objvalues(objects[44509])[0]!=20 and objvalues(objects[44550])[0]==9 and objvalues(objects[44563])[0]==9
assert objects[44436].split('~',1)[0]==objects[44440].split('~',1)[0] and objects[44437].split('~',1)[0]==objects[44441].split('~',1)[0]
edges=[(v,int(m[1]),m[3].strip(),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==919
assert {(v,d,k.split()[-1]) for v,d,k,f,key,t in edges if key==-2}=={(44626,0,'rrakkma'),(44678,2,'kraange'),(44679,2,'raylen'),(44715,0,'tayr-dryn')}
travels={44426:(44473,7),44432:(44467,7),44477:(44726,98),44486:(44720,7),44488:(44768,5),44493:(44768,139),44498:(44585,320),44506:(44473,7),44517:(44613,4),44522:(44495,2),44523:(44500,1),44524:(44500,2),44525:(44500,3),44526:(44500,4),44527:(44617,2),44528:(44514,7),44529:(44552,1),44535:(44648,176),44541:(44650,320),44554:(44706,7),44555:(44569,7),44573:(44767,139)}
assert {v for v,b in objects.items() if objvalues(b)[0]==25}==set(travels)
for v,(target,cmd) in travels.items():assert objvalues(objects[v])[11:14]==[target,cmd,-1]
for v in (44517,44522,44529):
 assert all(not re.search(r'^[OPGE]\s+\d+\s+'+str(v)+r'\b',p.read_text(encoding='utf8'),re.M) for p in (ROOT/'areas/zon').glob('*.zon'))
assert objvalues(objects[44502])[11:14]==[179,44589,4]
for v,target in zip(range(44568,44572),range(44760,44764)):
 assert objvalues(objects[v])[0]==29 and objvalues(objects[v])[7]==0 and objvalues(objects[v])[11:14]==[340,target,5]
 assert any(r['command']=='G' and r['arguments'][1]==v for r in rs)
assert 'U 0' in mobiles[44517]
assert all(re.search(r'^F\s+'+str(chance)+r'\s*$',rooms[v],re.M) for v,chance in ((44586,80),(44587,80),(44624,35)))
assert {(a['kind'],a['vnum'],a['function']) for a in githzer['special_assignments']}=={('obj',44499,'lucky_weapon')}
foreign=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/alatorin.qst' and b['kind'] in ('Q','QA') and any(k=='I' and v in objects for k,v in b['give']+b['receive'])]
assert len(foreign)==4 and any(b['giver_vnum']==83236 and b['give']==[('I',44509)] and b['receive']==[('E',230000),('I',83501)] and b['disappear'] for b in foreign)
assert objvalues(dawndale_bodies('ravenloft2','obj')[59079])[11:14]==[44487,7,-1]
assert not (ROOT/'areas/shp/githzer.shp').exists()

# The Caverns of the Worms: exact independent equipment services and shared access.
worms = inventory_module.area_evidence(ROOT, 'worms')
mapping = next(m for m in catalog['story_mappings'] if m['source_area'] == 'worms')
assert (mapping['schema_version'], mapping['revision'], mapping['coverage']) == (3, 1, 'complete')
assert len(mapping['stories']) == 16 and len(mapping['contacts']) == 1 and not mapping['exclusions']
raw = [b for b in inventory_module.native_blocks(ROOT) if b['source'] == 'areas/qst/worms.qst' and b['kind'] in ('Q', 'QA')]
expected = [
 (93, [6900, 6908], 6915), (99, [6906, 6909], 6916), (105, [6901], 6917),
 (110, [6910, 6910, 6912, 6906], 6918), (118, [6903], 6919),
 (123, [6914, 6911, 6906, 6906, 6906], 6920), (132, [6900, 6908, 6905], 6921),
 (139, [6908, 6908], 6922), (145, [6902, 6910], 6923), (151, [6904, 6904], 6924),
 (157, [6900, 6909, 6912], 6925), (164, [6903, 6912], 6926), (170, [6902, 6909], 6927),
 (175, [6905, 6911], 6928), (181, [6913, 6907], 6929), (187, [6914, 6905, 6907, 6907, 6913], 6930),
]
assert [(b['line'], [v for k, v in b['give']], b['receive'][0][1]) for b in raw] == expected
assert all(b['kind'] == 'Q' and b['giver_vnum'] == 6915 and not b['disappear'] and all(k == 'I' for k, v in b['give'] + b['receive']) for b in raw)
assert {tuple(sorted(b['binding'].items())) for b in raw} == {tuple(sorted(c.items())) for s in mapping['stories'] for c in s['contracts']}
by_key = {b['binding']['completion_key']: b for b in raw}
for story in mapping['stories']:
 b = by_key[story['contracts'][0]['completion_key']]
 assert story['category'] == 'service' and story['steps'][-1]['contracts'] == story['contracts']
 assert {t['item_vnums'][0]: t['count'] for t in story['steps'][:-1]} == collections.Counter(v for k, v in b['give'])
 assert all(t['optional'] and t['kind'] == 'carried_item' and len(t['item_vnums']) == 1 for t in story['steps'][:-1])
assert sum(len(s['steps']) - 1 for s in mapping['stories']) == 34
assert sum(t['count'] for s in mapping['stories'] for t in s['steps'][:-1]) == 40
assert len(worms['dialogue']) == 17
aliases = list(dict.fromkeys(w for d in worms['dialogue'] for w in d['body'][0].split('~')[0].split()))
assert mapping['contacts'][0]['topics'] == aliases and len(aliases) == 20
assert mapping['contacts'][0]['mob_vnum'] == 6915 and mapping['contacts'][0]['keyword'] == 'wilms'
assert next(b for b in raw if b['line'] == 170)['body'] == ['~']
rooms = dawndale_bodies('worms', 'wld'); objects = dawndale_bodies('worms', 'obj'); mobiles = dawndale_bodies('worms', 'mob')
assert set(rooms) == set(range(6900, 7000)) and set(mobiles) == set(range(6900, 6916)) and set(objects) == set(range(6900, 6932))
assert len({(b.split('~')[0].strip(), b.split('~')[1].strip()) for b in rooms.values()}) == 21
assert (worms['zone']['zone_number'], worms['zone']['first_vnum'], worms['zone']['last_vnum'], worms['zone']['reset_mode']) == (69, 6870, 6999, 2)
rs = worms['reset_commands']
assert len(rs) == 204 and collections.Counter(r['command'] for r in rs) == {'D': 2, 'O': 1, 'M': 101, 'G': 100}
assert all(r['arguments'][4] == 100 for r in rs)
stock = [8, 3, 2, 12, 10, 4, 11, 6, 6, 10, 6, 4, 10, 4, 4]
assert collections.Counter(r['arguments'][1] for r in rs if r['command'] == 'G') == dict(zip(range(6900, 6915), stock))
for i, row in enumerate(rs):
 if row['command'] == 'G':
  parent = rs[i - 1]
  assert parent['command'] == 'M' and parent['arguments'][1] == row['arguments'][1] and parent['arguments'][2] == row['arguments'][2] == stock[row['arguments'][1] - 6900]
assert [('O', [0, 6931, 1, 6937, 100, 0, 0, 0])] == [(r['command'], r['arguments']) for r in rs if r['command'] == 'O']
assert any(r['command'] == 'M' and r['arguments'] == [0, 6915, 1, 6907, 100, 0, 0, 0] for r in rs)
assert [(r['arguments'][1], r['arguments'][2], r['arguments'][3]) for r in rs if r['command'] == 'D'] == [(6937, 2, 8), (6984, 0, 0)]
assert all(objvalues(objects[v])[8] & 32768 for v in range(6900, 6915))
assert objvalues(objects[6931])[0] == 29 and objvalues(objects[6931])[7] == 0 and objvalues(objects[6931])[11:15] == [270, 6937, 2, 1]
edges = [(v, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for v, b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', b, re.S)]
assert len(edges) == 220 and (6937, 2, 8, 0, 6984) in edges and (6984, 0, 0, 0, 6937) in edges
assert {(v, d, f, k, t) for v, d, f, k, t in edges if t not in rooms} == {(6900, 2, 0, 0, 811708), (6984, 2, 0, 0, 54211)}
assert '0 0 6900' in dawndale_bodies('underdark', 'wld')[811708] and '0 0 6984' in dawndale_bodies('connectorzones', 'wld')[54211]
assert 'lie to the west' in rooms[6900]  # Documented proposed wording fix; no native change here.
assert not worms['special_assignments'] and not (ROOT / 'areas/shp/worms.shp').exists()
assert all(objvalues(b)[0] != 25 for b in objects.values())
veil = (ROOT / 'src/specs/specs.ixarkon.c').read_text()
destinations = re.search(r'int to_room\[MAX_SQUID_ROOM \+ 1\] = \{([^}]+)\}', veil)[1]
assert len(re.findall(r'\d+', destinations)) == 25 and 6900 in map(int, re.findall(r'\d+', destinations))
assert 'str_cmp(" veil", arg)' in veil and 'do_restore(ch, GET_NAME(ch), -4)' in veil
assert 'obj_index[real_object0(96402)].func.obj = illithid_teleport_veil' in (ROOT / 'src/specs/specs.assign.c').read_text()
assert objvalues(dawndale_bodies('ixarkon', 'obj')[96402])[0] == 13 and '96524' in (ROOT / 'areas/zon/ixarkon.zon').read_text()


# Castle Ravenloft: distinct hand-ins, retained prop and unrecorded shared puzzle.
ravenloft = inventory_module.area_evidence(ROOT, 'ravenloft')
mapping = next(m for m in catalog['story_mappings'] if m['source_area'] == 'ravenloft')
assert (mapping['schema_version'], mapping['revision'], mapping['coverage']) == (3, 1, 'complete')
assert len(mapping['stories']) == 4 and len(mapping['contacts']) == 5 and len(mapping['exclusions']) == 1
raw = [b for b in inventory_module.native_blocks(ROOT) if b['source'] == 'areas/qst/ravenloft.qst' and b['kind'] in ('Q', 'QA')]
assert [(b['line'], b['giver_vnum'], b['give'], b['receive'], b['disappear']) for b in raw] == [
 (18, 58312, [('I', 58393), ('I', 58407)], [('I', 58408)], True),
 (29, 58343, [('I', 58427)], [('I', 58427)], False),
 (88, 58347, [('I', 58370)], [('I', 58416)], False),
 (137, 58367, [('I', 58369)], [('I', 58401)], True),
 (162, 58381, [('I', 58346), ('I', 58410)], [('I', 58411)], True),
]
bindings = {b['giver_vnum']: b['binding'] for b in raw}
assert mapping['exclusions'][0]['contracts'] == [bindings[58343]]
assert {tuple(sorted(b.items())) for s in mapping['stories'] for b in s['contracts']} == {tuple(sorted(bindings[v].items())) for v in (58312, 58347, 58367, 58381)}
by_giver = {b['giver_vnum']: b for b in raw}
for story in mapping['stories']:
 b = by_giver[story['contracts'][0]['giver_vnum']]
 assert story['category'] == 'story' and story['steps'][-1]['contracts'] == story['contracts']
 assert [(t['item_vnums'], t['count']) for t in story['steps'][:-1]] == [([v], 1) for k, v in b['give']]
 assert all(t['optional'] and t['kind'] == 'carried_item' for t in story['steps'][:-1])
assert sum(len(s['steps']) - 1 for s in mapping['stories']) == 6
assert len(ravenloft['dialogue']) == 17
contacts = {c['mob_vnum']: c for c in mapping['contacts']}
assert sum(len(c['topics']) for c in mapping['contacts']) == 42 and len(contacts[58347]['topics']) == 32
assert 'urik' in contacts[58347]['topics'] and contacts[58343]['topics'] == []
for d in ravenloft['dialogue']:
 assert set(d['body'][0].split('~')[0].split()) & set(contacts[d['giver_vnum']]['topics'])
for v, contact in contacts.items():
 assert contact['keyword'] in ravenloft['mobs'][v]['keywords']
 assert set(contact['topics']) <= {w for d in ravenloft['dialogue'] if d['giver_vnum'] == v for w in d['body'][0].split('~')[0].split()}
rooms = dawndale_bodies('ravenloft', 'wld'); objects = dawndale_bodies('ravenloft', 'obj'); mobiles = dawndale_bodies('ravenloft', 'mob')
assert set(rooms) == set(range(58300, 58569)) and set(objects) == set(range(58300, 58440)) and set(mobiles) == set(range(58300, 58389))
assert len({(b.split('~')[0].strip(), b.split('~')[1].strip()) for b in rooms.values()}) == 173
assert (ravenloft['zone']['zone_number'], ravenloft['zone']['first_vnum'], ravenloft['zone']['last_vnum'], ravenloft['zone']['reset_mode']) == (583, 57696, 58568, 0)
rs = ravenloft['reset_commands']
assert len(rs) == 937 and collections.Counter(r['command'] for r in rs) == {'D': 110, 'O': 50, 'P': 42, 'M': 316, 'E': 191, 'G': 78, 'F': 150}
def ravenloft_reset(command, args):
 return any(r['command'] == command and r['arguments'] == args for r in rs)
for v, room in ((58346,58317),(58410,58517),(58427,58449),(58413,58534),(58430,58564),(58412,58527),(58397,58532)):
 assert ravenloft_reset('O', [0,v,1,room,100,0,0,0])
for item, container in ((58369,58340),(58370,58340),(58362,58366),(58419,58420),(58415,58420),(25745,58430)):
 assert ravenloft_reset('P', [1,item,1,container,100,0,0,0])
parents = {}; parent = None
for row in rs:
 if row['command'] == 'M': parent = row['arguments']
 if row['command'] == 'G': parents.setdefault(row['arguments'][1], []).append((parent[1],parent[3]))
assert parents[58393] == [(58364,58424)] and parents[58407] == [(58379,58560)]
assert parents[58366] == [(58348,58321)] and parents[58420] == [(58383,58562)]
assert ravenloft_reset('M', [0,58367,1,58544,50,0,0,0])
assert ravenloft_reset('M', [0,58381,1,58459,100,0,0,0]) and ravenloft_reset('M', [0,58846,1,58567,100,0,0,0])
assert objvalues(objects[58340])[11:15] == [100,15,58362,100]
assert objvalues(objects[58430])[11:15] == [20,29,0,4]
assert objvalues(objects[58413])[7] == 0 and objvalues(objects[58427])[7] & 1
for item, command, room, direction, caption in ((58302,270,58397,0,1),(58307,270,58442,3,1),(58309,270,58440,3,1),(58306,259,58446,5,0),(58305,259,58449,8,0),(58422,10,58563,5,0),(58359,340,58457,1,1),(58349,340,58521,0,0),(58404,179,58556,5,0),(58406,270,58560,9,1)):
 assert objvalues(objects[item])[0] == 29 and objvalues(objects[item])[11:15] == [command,room,direction,caption]
edges = [(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges) == 689
for edge in ((58400,2,3,-2,58403),(58412,0,3,58408,58435),(58459,0,3,58411,58369),(58369,2,3,0,58459),(58567,2,3,58826,58568),(58568,0,3,58826,58567),(58397,0,8,0,58564)):
 assert edge in edges
assert 'door obox-ob~' in rooms[58400]
assert {(v,d,f,k,t) for v,d,f,k,t in edges if t not in rooms} == {(58538,5,0,0,59057),(58542,2,0,0,59352),(58567,0,0,0,58958)}
assert objvalues(objects[58300])[11:15] == [58325,63,-1,0] and objvalues(objects[58304])[11:15] == [58426,3,-1,0]
assert not any(r['command'] in ('O','P','G','E') and r['arguments'][1] in (58300,58304) for r in rs)
assert objvalues(dawndale_bodies('ravenloft2','obj')[59325])[11:15] == [58456,270,-1,0]
assert objvalues(dawndale_bodies('ravenloft2','obj')[59326])[11:15] == [59065,270,-1,0]
assert '59325 1 59065' in (ROOT/'areas/zon/ravenloft2.zon').read_text() and ravenloft_reset('O',[0,59326,1,58456,100,0,0,0])
assert 'coming soon' in objects[58343].lower() and 'Sunblade' in dawndale_bodies('bahamut','obj')[25745] and 'Sunlash' in dawndale_bodies('bahamut','obj')[25745]
spec = (ROOT/'src/specs/specs.ravenloft.c').read_text()
bell = spec[spec.index('int ravenloft_bell('):spec.index('/*\nint strahd_charm(')]
assert all(x in bell for x in ('ch->equipment[WIELD]','OBJ_WORN_POS(weapon, WIELD)','cmd == CMD_HIT','isname(arg, "bell")','real_room0(VROOM_RAVENLOFT_SWORDCASE)','OBJ_VNUM(swordcase) == VOBJ_RAVENLOFT_LOCKED_CASE','extract_obj(obj, TRUE)','REMOVE_BIT(swordcase->value[1], CONT_LOCKED)','set_short_description(swordcase, buf)'))
assert bell.index('if (!swordcase)') < bell.index('extract_obj(obj, TRUE)') < bell.index('REMOVE_BIT(swordcase->value[1], CONT_LOCKED)')
assert 'VOBJ_RAVENLOFT_UNLOCKED_CASE' not in bell and 'economic_gameplay_authority' not in bell
assert {(a['kind'],a['vnum'],a['function']) for a in ravenloft['special_assignments']} == {('obj',58424,'artifact_shadow_shield'),('obj',58413,'ravenloft_bell'),('mob',58387,'ravenloft_vistani_shout'),('obj',58400,'shimmer_shortsword')}
descend = (ROOT/'src/cmd/actoth.c').read_text(); descend = descend[descend.index('void do_descend('):descend.index('void do_old_descend(')]
assert descend.index('This command has been disabled') < descend.index('int tome = vnum_in_inv(ch, 58424)')
drop = (ROOT/'src/item/randomeq.c').read_text(); assert re.search(r'/\* Not worth the time to calculate\.[\s\S]*highdrop_mobs[\s\S]*?\*/',drop)
units = [u for u in catalog_module.story_units(catalog) if u['zone_number'] == 583]
assert len(units) == 4 and sum(u['achievement'] for u in units) == 4 and sum(u['daily_candidate'] for u in units) == 1

# Barovia Continued: independent proofs, returned mark, alternative keys and cross-zone contact.
barovia2 = inventory_module.area_evidence(ROOT, 'barovia2')
mapping = next(m for m in catalog['story_mappings'] if m['source_area'] == 'barovia2')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage']) == (3,1,'complete')
assert len(mapping['stories']) == 5 and len(mapping['contacts']) == 4 and not mapping['exclusions']
raw = [b for b in inventory_module.native_blocks(ROOT) if b['source'] == 'areas/qst/barovia2.qst' and b['kind'] in ('Q','QA')]
assert [(b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw] == [
 (41,58804,[('I',58416)],[('I',58849)],True),
 (132,58812,[('I',58817),('I',58824)],[('I',58825),('I',58826)],False),
 (143,58812,[('I',58845),('I',58844)],[('I',58846),('I',58844)],False),
 (176,58822,[('I',58809)],[('I',58810)],True),
 (307,58846,[('I',58834)],[('I',58826)],True),
]
assert {tuple(sorted(b.items())) for s in mapping['stories'] for b in s['contracts']} == {tuple(sorted(b['binding'].items())) for b in raw}
for story in mapping['stories']:
 b = next(b for b in raw if story['contracts'] == [b['binding']])
 assert story['category'] == 'story' and story['steps'][-1]['contracts'] == story['contracts']
 assert [(t['item_vnums'],t['count']) for t in story['steps'][:-1]] == [([v],1) for k,v in b['give']]
 assert all(t['optional'] and t['kind'] == 'carried_item' for t in story['steps'][:-1])
assert sum(len(s['steps'])-1 for s in mapping['stories']) == 7
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert len(barovia2['dialogue']) == 16 and sum(len(c['topics']) for c in mapping['contacts']) == 32
for v,c in contacts.items():
 assert c['keyword'] in barovia2['mobs'][v]['keywords']
 assert set(c['topics']) == {w for d in barovia2['dialogue'] if d['giver_vnum']==v for w in d['body'][0].split('~')[0].split()}
rooms=dawndale_bodies('barovia2','wld');objects=dawndale_bodies('barovia2','obj');mobiles=dawndale_bodies('barovia2','mob')
assert set(rooms)==set(range(58800,58997))-{58994} and set(objects)==set(range(58800,58855)) and set(mobiles)==set(range(58800,58849))
assert len({tuple(b.split('~')[:2]) for b in rooms.values()})==127
assert (barovia2['zone']['zone_number'],barovia2['zone']['first_vnum'],barovia2['zone']['last_vnum'],barovia2['zone']['reset_mode'])==(588,58569,58996,0)
rs=barovia2['reset_commands']
assert len(rs)==435 and collections.Counter(r['command'] for r in rs)=={'D':22,'O':16,'P':12,'M':172,'G':47,'E':131,'F':34,'R':1}
parents={};parent=None
for row in rs:
 if row['command'] in ('M','F'):parent=row['arguments']
 if row['command'] in ('G','E'):parents.setdefault(row['arguments'][1],[]).append((row['command'],parent[1],parent[3],row['arguments'][3]))
for item,expected in ((58817,[('G',58815,58941,0)]),(58824,[('E',58800,58832,19)]),(58845,[('E',58825,58848,24)]),(58844,[('E',58819,58894,24)]),(58809,[('G',58821,58891,0)]),(58834,[('G',58835,58984,0)]),(58830,[('G',58835,58984,0)]),(58854,[('E',58829,58960,3)])):
 assert parents[item]==expected,(item,parents[item])
assert any(r['command']=='M' and r['arguments']==[0,58836,2,58991,50,0,0,0] for r in rs)
for item,command,room,direction,caption in ((58818,270,58941,2,1),(58822,320,58989,5,0),(58823,320,58975,1,0)):
 assert objvalues(objects[item])[0]==29 and objvalues(objects[item])[11:15]==[command,room,direction,caption]
assert objects[58822].split('~')[:3]==objects[58823].split('~')[:3]
assert objvalues(objects[58832])[11:15]==[500,13,58833,50]
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==494
for edge in ((58941,2,8,0,58962),(58975,1,9,0,58977),(58989,5,0,0,58990),(58894,3,2,58833,58895),(58895,1,3,58833,58894),(58959,4,3,58854,58960),(58960,5,3,58854,58959),(58984,4,3,58830,58995),(58995,5,3,0,58984)):
 assert edge in edges,edge
assert {(v,d,f,k,t) for v,d,f,k,t in edges if t not in rooms}=={(58800,2,0,0,91162),(58958,2,0,0,58567)}
assert any(r['command']=='D' and r['arguments']==[0,58989,5,8,100,0,0,0] for r in rs)
for item,target in ((58800,58854),(58812,58897),(58819,58964),(58820,58988),(58831,58963)):
 assert objvalues(objects[item])[0]==25 and objvalues(objects[item])[11:15]==[target,7,-1,0]
assert not any(r['command'] in ('O','P','G','E') and r['arguments'][1]==58812 for r in rs)
assert {r['arguments'][3] for r in rs if r['command']=='M' and r['arguments'][1]==58812}=={58855}
assert 'M 0 58846 1 58567' in (ROOT/'areas/zon/ravenloft.zon').read_text()
foreign_rooms=dawndale_bodies('ravenloft','wld')
assert '3 58826 58568' in foreign_rooms[58567] and '3 58826 58567' in foreign_rooms[58568]
assert objvalues(dawndale_bodies('ravenloft2','obj')[59091])[11:15]==[58964,7,-1,0]
assert 'O 0 59091 1 59169' in (ROOT/'areas/zon/ravenloft2.zon').read_text()
assert 'keep this badge' in (ROOT/'areas/qst/barovia2.qst').read_text() and 'E\nmark saboteur~' in objects[58845]
assert {(a['kind'],a['vnum'],a['function']) for a in barovia2['special_assignments']}=={('obj',58825,'barovia_undead_necklace')}
spec=(ROOT/'src/specs/specs.barovia.c').read_text()
assert all(x in spec for x in ('cmd != CMD_GOTHIT','number(0, 19)','data->victim','!IS_ALIVE(victim) || !IS_UNDEAD(victim)','spell_destroy_undead(60','spell_damage(ch, victim, 200','fear_check(victim)','spell_destroy_undead(20'))
switch=(ROOT/'src/specs/specs.object.c').read_text();switch=switch[switch.index('int item_switch('):switch.index('int item_switch(')+5400]
assert 'obj->value[0] != cmd' in switch and 'obj->loc.room != in_room' in switch and 'ch->in_room != in_room' in switch
assert 'REMOVE_BIT(world[in_room].dir_option[door]->exit_info, EX_BLOCKED)' in switch and 'GET_RACE' not in switch
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==588]
assert len(units)==5 and sum(u['achievement'] for u in units)==5 and sum(u['daily_candidate'] for u in units)==1


# Werrun: distinct books, worn keepsake, blocked coin aid and generated missions.
werrun=inventory_module.area_evidence(ROOT,'werrun')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='werrun')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==4 and len(mapping['contacts'])==5 and not mapping['exclusions']
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/werrun.qst' and b['kind'] in ('Q','QA')]
assert [(b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[
 (80,38301,[('I',38311)],[('C',12345),('I',38308)],False),
 (91,38301,[('I',38312)],[('I',38313)],False),
 (144,38305,[('C',350000)],[('I',38304)],False),
 (160,38308,[('I',38326)],[('E',35000),('I',38327)],True)]
assert {tuple(sorted(b.items())) for s in mapping['stories'] for b in s['contracts']}=={tuple(sorted(b['binding'].items())) for b in raw}
for story in mapping['stories']:
 b=next(b for b in raw if story['contracts']==[b['binding']])
 assert story['category']=='story' and story['steps'][-1]['contracts']==story['contracts']
 assert [(t['item_vnums'],t['count']) for t in story['steps'][:-1]]==[([v],1) for k,v in b['give'] if k=='I']
 assert all(t['optional'] and t['kind']=='carried_item' for t in story['steps'][:-1])
assert sum(len(s['steps'])-1 for s in mapping['stories'])==3
debt=next(s for s in mapping['stories'] if s['id']=='mage-debt')
assert len(debt['steps'])==1 and 'accounting' in debt['summary'] and 'blocks' in debt['summary']
assert len(werrun['dialogue'])==16 and sum(len(c['topics']) for c in mapping['contacts'])==25
for c in mapping['contacts']:
 assert c['keyword'] in werrun['mobs'][c['mob_vnum']]['keywords']
 if c['mob_vnum']==38309: assert c['topics']==['quest','map','abandon','resign']
 else: assert set(c['topics'])=={w for d in werrun['dialogue'] if d['giver_vnum']==c['mob_vnum'] for w in d['body'][0].split('~')[0].split()}
rooms=dawndale_bodies('werrun','wld');objects=dawndale_bodies('werrun','obj');mobiles=dawndale_bodies('werrun','mob')
assert set(rooms)==set(range(38300,38384)) and set(objects)==set(range(38300,38338)) and set(mobiles)==set(range(38300,38326))
assert len({tuple(b.split('~')[:2]) for b in rooms.values()})==43
assert (werrun['zone']['zone_number'],werrun['zone']['first_vnum'],werrun['zone']['last_vnum'],werrun['zone']['reset_mode'])==(383,38177,38383,2)
rs=werrun['reset_commands']
assert len(rs)==132 and collections.Counter(r['command'] for r in rs)=={'D':20,'O':20,'P':5,'M':57,'R':1,'G':11,'E':16,'F':2}
parents={};parent=None
for row in rs:
 if row['command'] in ('M','F'):parent=row['arguments']
 if row['command'] in ('G','E'):parents.setdefault(row['arguments'][1],[]).append((row['command'],parent[1],parent[3],row['arguments'][3]))
assert parents[38312]==[('E',38317,38381,13)]
assert parents[38336]==[('G',38308,38339,0)] and parents[38322]==[('G',38319,38365,0)]
assert parents[55416]==[('G',38317,38381,0)] and parents[358]==[('G',38317,38381,0)]
for item in (38311,38326): assert [r['arguments'] for r in rs if r['command']=='O' and r['arguments'][1]==item]==[[0,item,1,38380,100,0,0,0]]
assert [r['arguments'] for r in rs if r['command']=='M' and r['arguments'][1]==38308]==[[0,38308,1,38339,100,0,0,0]]
assert [r['arguments'] for r in rs if r['command']=='F']==[[1,38324,2,38381,100,0,0,0]]*2
assert objvalues(objects[38316])[0]==29 and objvalues(objects[38316])[11:15]==[270,38300,3,1]
for item,target,placement in ((38317,38344,38342),(38318,38342,38344)):
 assert objvalues(objects[item])[0]==25 and objvalues(objects[item])[11:15]==[target,139,-1,0]
 assert [r['arguments'] for r in rs if r['command']=='O' and r['arguments'][1]==item]==[[0,item,1,placement,100,0,0,0]]
assert objvalues(objects[38307])[11:15]==[500,29,38308,500] and objvalues(objects[38337])[11:15]==[50,15,38336,0]
assert [(r['arguments'][1],r['arguments'][3]) for r in rs if r['command']=='P' and r['arguments'][3]==38307]==[(38309,38307),(38310,38307)]
assert 'scroll flight' in objects[38309] and 'scroll venom' in objects[38310]
assert 'four scrolls' in (ROOT/'areas/qst/werrun.qst').read_text() and 'healing' in (ROOT/'areas/qst/werrun.qst').read_text()
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==197
assert (38300,3,8,0,38340) in edges and (38365,2,3,38322,38366) in edges and (38366,0,3,38322,38365) in edges
assert {(v,d,f,k,t) for v,d,f,k,t in edges if v==38339}=={(38339,0,0,0,38337),(38339,1,0,0,38341),(38339,2,0,0,38341),(38339,3,0,0,38341)}
assert not any(v==38341 for v,d,f,k,t in edges)
assert {(v,d,f,k,t) for v,d,f,k,t in edges if t not in rooms}=={(38300,2,0,0,527537)}
assert '0 0 38300' in dawndale_bodies('surface','wld')[527537] and '0 0 38316' in dawndale_bodies('wh','wld')[55614]
assert {(a['kind'],a['vnum'],a['function']) for a in werrun['special_assignments']}=={('mob',38309,'world_quest')}
spec=(ROOT/'src/specs/specs.world_quest.c').read_text()
assert all(x in spec for x in ('WORLD_QUEST_MIN_LEVEL','world_quest_payment_committed','createQuestForGiverVnum','quest_full_reward(pl, ch, 1)','quest_buy_map(pl)','isname("abandon", what)','isname("resign", what)','world_quest_refund_payment'))
native=(ROOT/'src/world/quest.c').read_text()
assert 'if (economic_gameplay_authority::active())\n\t\t{\n\t\t\tsend_to_char("This quest cannot accept offerings right now.' in native
assert 'goal->goal_type != QUEST_GOAL_ITEM' in native and 'reward->goal_type == QUEST_GOAL_EXP' in native and 'next_level_xp / 10' in native
policy=(ROOT/'src/world/world_quest_policy.c').read_text()
assert 'collect_quest_item_vnums()' in policy and 'quest_items.count(obj_index[rnum].virtual_number)' in policy
assert catalog['source']['excludes']==['bartender_random_world_quests'] and not any(d['giver_vnum']==38309 for d in catalog['definitions'])
ship=next(b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/shipy.qst' and b['kind']=='Q' and b['giver_vnum']==43181 and b['give']==[('I',38314)])
assert ship['line']==822 and ship['receive']==[('C',66666)] and ship['disappear']
memory=next(b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/wh.qst' and b['kind']=='Q' and b['giver_vnum']==55136)
assert memory['give']==[('I',55416)] and memory['receive']==[('I',55362),('C',1000000),('I',55033)] and not memory['disappear']
assert 'M 0 43181 1 43292' in (ROOT/'areas/zon/shipy.zon').read_text()
assert 'M 0 55136 2 55005' in (ROOT/'areas/zon/wh.zon').read_text() and 'M 0 55136 2 55400' in (ROOT/'areas/zon/wh.zon').read_text()
assert '_noquest_' in dawndale_bodies('wh','obj')[55416]
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==383]
assert len(units)==4 and sum(u['achievement'] for u in units)==4 and sum(u['daily_candidate'] for u in units)==3


# New Hope: exact paid services, pickproof vault and two equipment source systems.
newhope=inventory_module.area_evidence(ROOT,'newhope')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='newhope')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==4 and len(mapping['contacts'])==3 and not mapping['exclusions']
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/newhope.qst' and b['kind'] in ('Q','QA')]
assert [(b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[
 (35,89102,[('I',89117),('C',45000),('I',89141)],[('I',89118)],False),
 (43,89102,[('I',89117),('C',100000),('I',89142)],[('I',89119)],False),
 (51,89102,[('I',89117),('C',250000),('I',89142)],[('I',89120)],False),
 (60,89102,[('I',89117),('C',10000),('I',89140)],[('I',89143)],False)]
assert {tuple(sorted(b.items())) for s in mapping['stories'] for b in s['contracts']}=={tuple(sorted(b['binding'].items())) for b in raw}
for story in mapping['stories']:
 b=next(b for b in raw if story['contracts']==[b['binding']])
 assert story['category']=='service' and story['steps'][-1]['contracts']==story['contracts']
 assert [(t['item_vnums'],t['count']) for t in story['steps'][:-1]]==[([v],1) for k,v in b['give'] if k=='I']
 assert all(t['optional'] and t['kind']=='carried_item' for t in story['steps'][:-1])
 assert 'Accounting currently blocks' in story['summary'] and str(next(v for k,v in b['give'] if k=='C')) in story['steps'][-1]['hint']
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={89102,89165,89166} and sum(len(c['topics']) for c in contacts.values())==21
assert len(newhope['dialogue'])==15
for d in newhope['dialogue']:assert set(d['body'][0].rstrip('~').split())<=set(contacts[d['giver_vnum']]['topics'])
assert [contacts[v]['keyword'] for v in (89102,89165,89166)]==['vitrius','janitor','visitor']
rooms=dawndale_bodies('newhope','wld');objects=dawndale_bodies('newhope','obj');mobiles=dawndale_bodies('newhope','mob')
assert set(rooms)==set(range(89000,89228)) and set(objects)==set(range(89000,89190)) and set(mobiles)==set(range(89000,89201))
assert len({tuple(b.split('~')[:2]) for b in rooms.values()})==210
assert (newhope['zone']['zone_number'],newhope['zone']['first_vnum'],newhope['zone']['last_vnum'],newhope['zone']['reset_mode'])==(890,88970,89227,2)
rs=newhope['reset_commands'];assert len(rs)==1033
assert collections.Counter(r['command'] for r in rs)=={'D':184,'O':37,'P':32,'M':571,'G':120,'E':69,'F':20}
sources={};parent=None
for row in rs:
 if row['command'] in ('M','F'):parent=row['arguments']
 if row['command'] in ('G','E'):sources.setdefault(row['arguments'][1],[]).append((row['command'],parent[1],parent[3],row['arguments'][2],row['arguments'][3],row['arguments'][4]))
assert sources[89019]==[('G',89156,89189,1,0,100)]
assert sources[89188]==[('G',89181,89220,1,0,100)] and sources[89144]==[('E',89181,89220,1,16,100)]
for item,mob in ((89140,89170),(89142,89171),(89141,89172)):
 assert sorted(sources[item])==[('E',mob,room,3,16,100) for room in (89201,89202,89203)]
assert [r['arguments'] for r in rs if r['command']=='O' and r['arguments'][1]==89117]==[[0,89117,2,89098,100,0,0,0],[0,89117,2,89162,100,0,0,0]]
for item in range(89145,89150):assert [r['arguments'] for r in rs if r['command']=='O' and r['arguments'][1]==item]==[[0,item,1,89227,20,0,0,0]]
assert [r['arguments'] for r in rs if r['command']=='O' and r['arguments'][1]==89152]==[[0,89152,2,89227,100,0,0,0]]*2
assert not any(r['command'] in ('G','E','O','P') and r['arguments'][1]==358 for r in rs)
for item,chance in ((89019,0),(89188,10),(89152,1)):
 assert objvalues(objects[item])[0]==18 and objvalues(objects[item])[11:15]==[0,chance,0,0]
assert not (objvalues(objects[89188])[7]&524288) and not (objvalues(objects[89144])[7]&524288)
assert 'shortsword' in objects[89141] and 'longsword' in objects[89142] and 'dagger' in objects[89140]
assert not any(objvalues(b)[0] in (25,29) for b in objects.values())
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==550
for edge in ((89188,5,6,89019,89218),(89218,4,5,0,89188),(89220,0,3,89188,89227),(89227,2,3,89152,89220),(89085,0,1,0,89164)):assert edge in edges
assert {(v,d,f,k,t) for v,d,f,k,t in edges if t not in rooms}=={(89217,1,0,0,618434),(89217,2,0,0,618833)}
assert '0 0 89217' in dawndale_bodies('surface','wld')[618434] and '0 0 89217' in dawndale_bodies('surface','wld')[618833]
assert rooms[89219].split('~',2)[2].strip().splitlines()[0].endswith(' 26') and rooms[89220].split('~',2)[2].strip().splitlines()[0].endswith(' 9')
assert {(a['kind'],a['vnum'],a['function']) for a in newhope['special_assignments']}=={('mob',89181,'tentacler_death')}
spec=(ROOT/'src/specs/specs.newhope.c').read_text()
assert 'obj_load = number(0, 4)' in spec and 'obj_to_room(tempobj, real_room(89227))' in spec
assert all('read_object('+str(v)+', VIRTUAL)' in spec for v in range(89145,89150)) and '89188' not in spec
db=(ROOT/'src/world/db.c').read_text();native=(ROOT/'src/world/quest.c').read_text()
assert 'if (state == 3)' in db and 'exit_info |= EX_PICKPROOF' in db and 'ZCMD.arg3 & 0x04' in db
assert 'goal->goal_type != QUEST_GOAL_ITEM' in native and 'This quest cannot accept offerings right now.' in native
assert 'mob_index[quest_index[count].quester].qst_func = quester' in native
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==890]
assert len(units)==4 and not any(u['achievement'] or u['daily_candidate'] for u in units)
assert all(r['definition']['daily_exclusion']=='Unsupported durable offering' for r in newhope['requests'])
assert sum(len(s['steps'])-1 for s in mapping['stories'])==8


# Goblin City: three exact returns, addressed/ambient authority, hidden access and explicit service intent.
malch=inventory_module.area_evidence(ROOT,'malch')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='malch')
expected={24:(23623,[('I',93901)]*3,[('C',66666),('E',66666)],'shadowclave-circles'),50:(23624,[('I',23622)],[('E',25000),('C',25000)],'pirate-hat'),89:(23626,[('I',43131)]*3,[('E',65000),('C',40000)],'storm-altar-charms')}
assert len(malch['requests'])==len(mapping['stories'])==3
for q,s in zip(malch['requests'],mapping['stories']):
 b=q['block'];giver,give,reward,storyid=expected[b['line']]
 assert (b['giver_vnum'],b['disappear'],s['id'])==(giver,True,storyid)
 assert [tuple(x) for x in b['give']]==give and [tuple(x) for x in b['receive']]==reward
 assert s['contracts']==s['steps'][-1]['contracts']==[b['binding']]
 assert {t['item_vnums'][0]:t['count'] for t in s['steps'][:-1]}==collections.Counter(v for k,v in give if k=='I')
 assert all(t['kind']=='carried_item' and t['optional'] for t in s['steps'][:-1]) and s['steps'][-1]['kind']=='completion'
 assert q['definition']['repeatable'] and q['definition']['daily_eligible'] and q['definition']['eligible_for_zone_completion'] and not q['definition']['daily_exclusion']
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
assert [s['id'] for s in mapping['stories']]==['shadowclave-circles','pirate-hat','storm-altar-charms']
assert len(mapping['contacts'])==12 and sum(len(c['topics']) for c in mapping['contacts'])==7
assert sum(len(s['steps']) for s in mapping['stories'])==6 and sum(t.get('optional',False) for s in mapping['stories'] for t in s['steps'])==3
assert [s['steps'][0]['count'] for s in mapping['stories']]==[3,1,3]
for c in mapping['contacts']:
 v=c['mob_vnum'];assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for d in malch['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert {c['mob_vnum'] for c in mapping['contacts'] if c['topics']}=={23623,23624,23626}
assert collections.Counter(d['kind'] for d in malch['dialogue'])=={'MA':3}
assert (malch['zone']['zone_number'],malch['zone']['first_vnum'],malch['zone']['last_vnum'],malch['zone']['reset_mode'])==(236,23561,23760,2)
rooms=dawndale_bodies('malch','wld');objects=dawndale_bodies('malch','obj');mobiles=dawndale_bodies('malch','mob')
assert (len(rooms),len(mobiles),len(objects))==(161,27,23) and set(rooms)==set(range(23600,23761))
edges={(v,int(m[1])):(int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'^D(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.M|re.S)}
assert len(edges)==360 and {(src,dest) for (src,d),(_,k,dest) in edges.items() if dest not in rooms}=={(23600,632944),(23721,849235),(23741,96976),(23760,43320)}
resets=malch['reset_commands'];assert len(resets)==291 and collections.Counter(r['command'] for r in resets)=={'D':18,'O':14,'P':3,'M':240,'E':5,'G':11}
assert len((ROOT/'areas/zon/malch.zon').read_text().splitlines())==356 and len((ROOT/'areas/qst/malch.qst').read_text().splitlines())==100
stock=[];parent=room=None
for r in resets:
 a=r['arguments'];cmd=r['command']
 if cmd=='M':parent,room=a[1],a[3]
 elif cmd in ('E','G'):stock.append((cmd,a[1],parent,room,a[2],a[3],a[4]))
 assert cmd not in ('M','D','O') or (a[1] if cmd=='D' else a[3]) in rooms
 assert cmd!='M' or a[1] in inventory_mobs
 assert cmd not in ('O','P','G','E') or a[1] in inventory_items
assert [s for s in stock if s[1]==23622]==[('E',23622,23622,23756,1,6,100)]
assert [s for s in stock if s[1]==23621]==[('E',23621,23622,23756,1,19,100)]
assert [s for s in stock if s[1]==40474]==[('G',40474,23622,23756,1,0,100)]
def malch_foreign_stock(area,item):
 rows=[];parent=room=None
 for line in (ROOT/'areas/zon'/f'{area}.zon').read_text().splitlines():
  m=re.match(r'^([MFEG])\s+((?:-?\d+\s+){7}-?\d+)',line)
  if not m:continue
  a=[int(v) for v in m[2].split()];cmd=m[1]
  if cmd in ('M','F'):parent,room=a[1],a[3]
  elif a[1]==item:rows.append((cmd,parent,room,a[2],a[3],a[4]))
 return rows
assert sorted(malch_foreign_stock('shadowcl',93901))==sorted([('E',93900,r,8,24,100) for r in (93904,93908,93911,93926,93931)]+[('E',93903,93921,8,24,100),('E',93902,93925,8,24,100),('E',93906,93929,8,24,100)])
assert malch_foreign_stock('alatorin',93901)==[('G',83410,83750,1,0,100)]
assert malch_foreign_stock('shipy',43131)==[('E',43188,43317,3,3,100)]*3
assert 'fisherman' in inventory_mobs[43131]['keywords'] and 'warlock' in inventory_mobs[43188]['keywords']
hatdata=objvalues(objects[23622]);assert hatdata[6]==4096+8388608 and not hatdata[6]&524288
assert objvalues(dawndale_bodies('shipy','obj')[43131])[6]&4096
deskdata=objvalues(objects[23607]);assert deskdata[0]==15 and deskdata[11:19]==[100,15,0,100,0,0,0,0] and not deskdata[12]&16
assert re.search(r'\nT\s+518\s+2\s+10\s+71',objects[23607]) and objvalues(objects[23608])[6]&4096
assert objvalues(objects[23601])[11:19]==[50,0,0,30,0,0,0,0]
assert {(r['arguments'][1],r['arguments'][3]) for r in resets if r['command']=='P'}=={(23608,23607),(23602,23601),(23604,23601)}
doors={(r['arguments'][1],r['arguments'][2]):r['arguments'][3] for r in resets if r['command']=='D'}
assert doors[(23758,0)]==doors[(23759,2)]==8 and doors[(23716,3)]==5 and doors[(23717,1)]==1
for v,where,d in ((23618,23758,0),(23619,23759,2)):
 data=objvalues(objects[v]);flags,values=data[:11],data[11:19];assert flags[0]==29 and not flags[7]&1 and values==[270,where,d,1,0,0,0,0]
 assert any(r['command']=='O' and r['arguments'][1]==v and r['arguments'][3]==where for r in resets)
 assert edges[(where,d)][2]==(23759 if where==23758 else 23758)
def malch_reachable(start,switches):
 found={start};cleared=set();changed=True
 while changed:
  changed=False
  if switches:
   for where,d in ((23758,0),(23759,2)):
    if where in found and (where,d) not in cleared:cleared.update({(where,d),(edges[(where,d)][2],2 if d==0 else 0)});changed=True
  for edge,(_,key,dest) in edges.items():
   if edge[0] not in found or dest not in rooms:continue
   state=doors.get(edge,0)
   if state&8 and edge not in cleared:continue
   if state&3 in (2,3):continue
   if dest not in found:found.add(dest);changed=True
 return found
for start in (23600,23721,23741):
 assert set(rooms)-malch_reachable(start,False)=={23759,23760} and len(malch_reachable(start,True))==161
assert malch_reachable(23760,False)=={23759,23760} and len(malch_reachable(23760,True))==161
assert re.search(r'\nF\s+60\s+S',rooms[23742]) and all(re.search(r'\nF\s+5\s+S',rooms[v]) for v in range(23743,23761))
def malch_act(v):return int(re.search(r'^(\d+)(?:\s+-?\d+){8}\s+S\s*$',mobiles[v],re.M)[1])
assert all(malch_act(v)&32768 for v in (23608,23614)) and all(not malch_act(v)&32768 for v in (23613,23617))
assert malch_act(23623)&2 and malch_act(23624)&2 and not malch_act(23626)&2
native_qst=(ROOT/'areas/qst/malch.qst').read_text();assert native_qst.count('qc_action ')==7 and 'Gisban Flegidigo' in native_qst and 'Skarg SkagTooth' in mobiles[23623]
quest_source=(ROOT/'src/world/quest.c').read_text()
assert 'TO_ROOM' in quest_source and 'zone_story_quest_runtime::encountered(pl, ch)' in quest_source
guild=(ROOT/'src/guild/guild.c').read_text();assert 'IS_SET(teacher->specials.act, ACT_TEACHER)' in guild and 'Paid skill practice is unavailable while active accounting is enabled.' in guild
snapshot=(ROOT/'src/player/player_snapshot_capture.c').read_text();assert 'omit_norent && IS_SET(object->extra_flags, ITEM_NORENT) && !active_durable_custody' in snapshot

# Varathorn Keep: independent returns, exact follower talons, hidden recovery and effective melee dispatch.
kastle=inventory_module.area_evidence(ROOT,'kastle')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='kastle')
expected={24:(99434,[('I',98914),('I',98914)],[('I',99447),('C',384000)],'night-crawler-talons'),84:(99436,[('I',99414)],[('E',124000),('C',50000)],'stolen-ring'),153:(99453,[('I',99430)],[('I',99419)],'varathorn-ossuary')}
assert len(kastle['requests'])==len(mapping['stories'])==3
for q,s in zip(kastle['requests'],mapping['stories']):
 b=q['block'];giver,give,reward,storyid=expected[b['line']]
 assert (b['giver_vnum'],b['disappear'],s['id'])==(giver,True,storyid)
 assert [tuple(x) for x in b['give']]==give and [tuple(x) for x in b['receive']]==reward
 assert s['contracts']==s['steps'][-1]['contracts']==[b['binding']]
 assert {t['item_vnums'][0]:t['count'] for t in s['steps'][:-1]}==collections.Counter(v for k,v in give if k=='I')
 assert all(t['kind']=='carried_item' and t['optional'] for t in s['steps'][:-1]) and s['steps'][-1]['kind']=='completion'
 assert q['definition']['repeatable'] and q['definition']['daily_eligible'] and q['definition']['eligible_for_zone_completion'] and not q['definition']['daily_exclusion']
assert [s['id'] for s in mapping['stories']]==['night-crawler-talons','stolen-ring','varathorn-ossuary']
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
assert len(mapping['contacts'])==12 and sum(len(c['topics']) for c in mapping['contacts'])==10
assert sum(len(s['steps']) for s in mapping['stories'])==6 and sum(t.get('optional',False) for s in mapping['stories'] for t in s['steps'])==3
for c in mapping['contacts']:
 v=c['mob_vnum'];assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for d in kastle['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert {c['mob_vnum'] for c in mapping['contacts'] if c['topics']}=={99434,99436,99453}
assert collections.Counter(d['kind'] for d in kastle['dialogue'])=={'M':3}
assert (kastle['zone']['zone_number'],kastle['zone']['first_vnum'],kastle['zone']['last_vnum'],kastle['zone']['reset_mode'])==(994,99336,99499,2)
rooms=dawndale_bodies('kastle','wld');objects=dawndale_bodies('kastle','obj');mobiles=dawndale_bodies('kastle','mob')
assert (len(rooms),len(mobiles),len(objects))==(100,63,49) and set(rooms)==set(range(99400,99500))
edges={(v,int(m[1])):(int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)}
assert len(edges)==222 and {(src,dest) for (src,d),(_,k,dest) in edges.items() if dest not in rooms}=={(99400,578633),(99439,575843)}
resets=kastle['reset_commands'];assert len(resets)==331 and collections.Counter(r['command'] for r in resets)=={'D':32,'O':6,'P':2,'M':181,'E':108,'G':2}
assert len((ROOT/'areas/zon/kastle.zon').read_text().splitlines())==438 and len((ROOT/'areas/qst/kastle.qst').read_text().splitlines())==164
stock=[];parent=room=None
for r in resets:
 a=r['arguments'];cmd=r['command']
 if cmd=='M':parent,room=a[1],a[3]
 elif cmd in ('E','G'):stock.append((cmd,a[1],parent,room,a[2],a[3],a[4]))
 assert cmd not in ('M','D','O') or (a[1] if cmd=='D' else a[3]) in rooms
 assert cmd!='M' or a[1] in inventory_mobs
 assert cmd not in ('O','P','G','E') or a[1] in inventory_items
assert [s for s in stock if s[1]==99414]==[('E',99414,99401,99471,1,2,100)]
assert [s for s in stock if s[1]==99448]==[('E',99448,99401,99471,1,24,100)]
assert [s for s in stock if s[1]==99404]==[('E',99404,99435,99492,1,28,100)]
for v,where in ((99434,99498),(99436,99493),(99453,99491)):
 assert [r['arguments'][1:5] for r in resets if r['command']=='M' and r['arguments'][1]==v]==[[v,1,where,100]]
assert [r['arguments'][1:5] for r in resets if r['command']=='O' and r['arguments'][1]==99430]==[[99430,1,99463,100]]
assert objvalues(objects[99430])[0]==15 and objvalues(objects[99430])[6]&4096 and objvalues(objects[99430])[11:15]==[9,0,0,9]
assert objvalues(objects[99402])[0]==15 and objvalues(objects[99402])[11:15]==[81,0,0,81]
assert [(r['arguments'][1],r['arguments'][2],r['arguments'][3],r['arguments'][4]) for r in resets if r['command']=='P']==[(99403,1,99402,100),(99445,999,99402,100)]
assert all(objvalues(objects[v])[6]&4096 for v in (99403,99445))
doors={(r['arguments'][1],r['arguments'][2]):r['arguments'][3] for r in resets if r['command']=='D'}
assert {edge for edge,state in doors.items() if state&8}=={(99406,5),(99418,4),(99420,2),(99463,0)}
assert all(doors[edge]==1 for edge in ((99482,1),(99483,3),(99486,5),(99487,4)))
assert edges[(99482,1)][1]==edges[(99483,3)][1]==99404
assert {v:int(re.search(r'\bF\s+(\d+)',b)[1]) for v,b in rooms.items() if re.search(r'\bF\s+(\d+)',b)}=={99486:81}
for v,where,cmd,direction in ((99400,99406,270,5),(99401,99418,320,4),(99405,99420,320,2)):
 assert objvalues(objects[v])[0]==29 and not objvalues(objects[v])[7]&1 and objvalues(objects[v])[11:15]==[cmd,where,direction,0]
 assert [r['arguments'][1:5] for r in resets if r['command']=='O' and r['arguments'][1]==v]==[[v,1,where,100]]
# Source-only graph assumes successful opening/search and adequate movement; no stock or played-journey proof.
reverse={0:2,2:0,1:3,3:1,4:5,5:4,6:9,9:6,7:8,8:7}
def kastle_reachable(switches):
 found={99400};cleared=set();changed=True
 while changed:
  changed=False
  if switches:
   for where,d in ((99406,5),(99418,4),(99420,2)):
    if where in found and (where,d) not in cleared:
     cleared.add((where,d));dest=edges[(where,d)][2];back=(dest,reverse[d])
     if not doors.get((where,d),0)&4 and back in edges and edges[back][2]==where:cleared.add(back)
     changed=True
  for edge,(_,key,dest) in edges.items():
   if edge[0] not in found or dest not in rooms:continue
   state=doors.get(edge,0)
   if state&8 and edge not in cleared:continue
   if state&3 in (2,3):continue
   if dest not in found:found.add(dest);changed=True
 return found
assert kastle_reachable(False)==kastle_reachable(True)==set(range(99400,99492))
assign=(ROOT/'src/specs/specs.assign.c').read_text();boot=(ROOT/'src/world/db.c').read_text()
assert 'obj_index[real_object0(99447)].func.obj = nightcrawler_dagger;' in assign and 'obj_index[real_object0(99432)].func.obj = zarthos_vampire_slayer;' in assign
assert assign.index('void assign_rooms(')<assign.index('obj_index[real_object0(99432)]') and assign.index('void assign_objects(')<assign.index('obj_index[real_object0(99447)]')<assign.index('void assign_rooms(')
assert 'assign_objects();' in boot and 'assign_rooms();' in boot and 'state &=\n\t\t3;' in boot
assert objvalues(objects[99447])[11:19]==[2,3,3,11,0,0,0,0] and objvalues(objects[99432])[11:19]==[13,4,6,3,0,48,41,30]
dispatch=(ROOT/'src/combat/attack_effects.c').read_text();at=dispatch.index('bool weapon_proc(');proc=dispatch[at:dispatch.index('\n}',at)+2]
assert proc.index('if (!obj->value[5] || obj->value[7] <= 0)')<proc.index('selected_packed_weapon_action') and 'CMD_MELEE_HIT' in proc and 'spells[0] = obj->value[5] % 1000' in proc
assert re.search(r'CMD_MELEE_HIT\s+1000\b',(ROOT/'src/cmd/interp.h').read_text())
assert re.search(r'SPELL_FIRE_BREATH\s+48\b',(ROOT/'src/magic/spells.h').read_text())
special=(ROOT/'src/specs/specs.kastle.c').read_text();assert 'OBJ_WORN' in special and 'spell_devitalize(50' in special and 'spell_destroy_undead(39' in special and 'spell_cure_serious(39' in special
# Data selects the packed slot, but normal startup does not register its spell pointer.
skill_source=(ROOT/'src/classes/skills.c').read_text()
spell_ids={name:int(value) for name,value in re.findall(r'^#define\s+(\w+)\s+(\d+)\b',(ROOT/'src/magic/spells.h').read_text(),re.M)}
registered=re.findall(r'\bSPELL_CREATE(?:_MSG|2)?\(\s*"[^"]*"\s*,\s*([A-Z0-9_]+)',skill_source)
assert 'skills[i] = Skill{};' in skill_source and 'SPELL_FIRE_BREATH' not in registered and all(spell_ids.get(name)!=48 for name in registered)
assert all(name in registered for name in ('SPELL_DEVITALIZE','SPELL_DESTROY_UNDEAD','SPELL_CURE_SERIOUS'))
assert 'if (skills[spells[count]].spell_pointer)' in proc
assert any('special effect needs builder review' in line for line in mapping['orientation'])
search=(ROOT/'src/cmd/actobj.c').read_text();at=search.index('void do_search(');search=search[at:search.index('\n}',at)+2]
assert 'ITEM_SECRET' in search and 'find_chance(ch)' in search and 'CMD_FOUND' in search
fall=(ROOT/'src/world/falling.c').read_text();assert '!VIRTUAL_CAN_GO(ch->in_room, DIR_DOWN)' in fall and 'AFF_LEVITATE' in fall
wandering=(ROOT/'src/mob/mobact.c').read_text();at=wandering.index('PROFILE_START(mundane_wander)');wander=wandering[at:wandering.index('\nnormal:',at)]
assert all(t in wander for t in ('ACT_SENTINEL','ROOM_NO_MOB','SECT_NO_GROUND','ACT_STAY_ZONE','do_move(ch'))
tezcat=inventory_module.area_evidence(ROOT,'tezcat');talons=[];parent=room=None
for r in tezcat['reset_commands']:
 a=r['arguments']
 if r['command'] in ('M','F'):parent,room=a[1],a[3]
 if r['command']=='E' and a[1]==98914:talons.append((parent,room,a[2],a[3],a[4]))
assert talons==[(98904,98949,2,16,100),(98904,98949,2,17,100)]
tharnadia=inventory_module.area_evidence(ROOT,'tharnadia')
quill=next(q['block'] for q in tharnadia['requests'] if q['block']['giver_vnum']==132613 and q['block']['line']==69)
assert [tuple(x) for x in quill['give']]==[('I',132664)] and [tuple(x) for x in quill['receive']]==[('I',132620),('I',99412)] and not quill['disappear']


# Chasm of the Misty Vale: exact duplicate scales, independent departures and optional bridge aid.
mist_chasm=inventory_module.area_evidence(ROOT,'mist_chasm')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='mist_chasm')
expected={11:(15006,[('I',15017),('I',15018)],[('I',15008)],'shaman-potion'),35:(15007,[('I',15017)],[('C',2000)],'small-scale-sale'),47:(15007,[('I',15018)],[('C',4000)],'large-scale-sale'),69:(15018,[('I',15017),('I',15017),('I',15018),('I',15018)],[('I',15019),('E',81000)],'scale-shield')}
assert len(mist_chasm['requests'])==4 and len(mapping['stories'])==4
for q,s in zip(mist_chasm['requests'],mapping['stories']):
 b=q['block'];giver,give,reward,storyid=expected[b['line']]
 assert (b['giver_vnum'],b['disappear'],s['id'])==(giver,True,storyid)
 assert [tuple(x) for x in b['give']]==give and [tuple(x) for x in b['receive']]==reward
 assert s['contracts']==[b['binding']] and s['steps'][-1]['contracts']==[b['binding']]
 needed=collections.Counter(v for k,v in give if k=='I')
 assert {t['item_vnums'][0]:t['count'] for t in s['steps'][:-1]}==needed
 assert all(t['kind']=='carried_item' and t['optional'] for t in s['steps'][:-1]) and s['steps'][-1]['kind']=='completion'
 assert q['definition']['repeatable'] and q['definition']['daily_eligible'] and q['definition']['eligible_for_zone_completion'] and not q['definition']['daily_exclusion']
assert [s['id'] for s in mapping['stories']]==['shaman-potion','small-scale-sale','large-scale-sale','scale-shield']
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
assert len(mapping['contacts'])==10 and sum(len(c['topics']) for c in mapping['contacts'])==11
assert sum(len(s['steps']) for s in mapping['stories'])==10 and sum(t.get('optional',False) for s in mapping['stories'] for t in s['steps'])==6
for c in mapping['contacts']:
 v=c['mob_vnum'];assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for d in mist_chasm['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert {c['mob_vnum'] for c in mapping['contacts'] if c['topics']}=={15006,15007,15018}
assert collections.Counter(d['kind'] for d in mist_chasm['dialogue'])=={'M':3}
assert (mist_chasm['zone']['zone_number'],mist_chasm['zone']['first_vnum'],mist_chasm['zone']['last_vnum'],mist_chasm['zone']['reset_mode'])==(150,14927,15099,1)
rooms=dawndale_bodies('mist_chasm','wld');objects=dawndale_bodies('mist_chasm','obj');mobiles=dawndale_bodies('mist_chasm','mob')
assert (len(rooms),len(mobiles),len(objects))==(100,20,25) and set(rooms)==set(range(15000,15100))
edges={(v,int(m[1])):(int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)}
assert len(edges)==209 and {(src,dest) for (src,d),(_,k,dest) in edges.items() if dest not in rooms}=={(15000,638825)}
resets=mist_chasm['reset_commands'];assert len(resets)==142 and collections.Counter(r['command'] for r in resets)=={'D':6,'O':9,'P':3,'M':81,'G':24,'E':19}
assert len((ROOT/'areas/zon/mist_chasm.zon').read_text().splitlines())==171 and len((ROOT/'areas/qst/mist_chasm.qst').read_text().splitlines())==88
stock=[];parent=room=None
for r in resets:
 a=r['arguments'];cmd=r['command']
 if cmd=='M':parent,room=a[1],a[3]
 elif cmd in ('E','G'):stock.append((cmd,a[1],parent,room,a[2],a[4]))
 assert cmd not in ('M','D','O') or (a[1] if cmd=='D' else a[3]) in rooms
 assert cmd!='M' or a[1] in inventory_mobs
 assert cmd not in ('O','P','G','E') or a[1] in inventory_items
assert [s for s in stock if s[1]==15018]==[('G',15018,15005,v,6,100) for v in (15000,15003,15005,15027,15028,15028)]
assert [s for s in stock if s[1]==15017]==[('G',15017,15014,v,7,100) for v in (15088,15090,15093,15094,15096,15098)]
assert [r['arguments'][1:5] for r in resets if r['command']=='O' and r['arguments'][1]==15017]==[[15017,7,15099,100]]
for v,cap,locations in ((15006,1,[15029]),(15007,4,[15005,15007,15011,15014]),(15018,1,[15092])):
 assert [r['arguments'][1:5] for r in resets if r['command']=='M' and r['arguments'][1]==v]==[[v,cap,where,100] for where in locations]
assert [s for s in stock if s[1]==15024]==[('G',15024,15013,15078,1,100)]
assert [s for s in stock if s[1]==67273]==[('G',67273,15008,15063,1,100)] and 67273 in inventory_items
assert {r['arguments'][1] for r in resets if r['command'] in ('O','P','G','E') and r['arguments'][1] not in objects}=={67273}
assert objvalues(objects[15008])[0]==10 and objvalues(objects[15008])[11:15]==[50,83,35,43]
assert objvalues(objects[15010])[0]==15 and objvalues(objects[15010])[11:14]==[250,5,15024]
assert objvalues(objects[15024])[0]==18 and objvalues(objects[15024])[12]==100
assert [(r['arguments'][1],r['arguments'][2],r['arguments'][3],r['arguments'][4]) for r in resets if r['command']=='P']==[(15011,1,15010,100),(15012,1,15010,100),(15015,1,15010,100)]
doors={(r['arguments'][1],r['arguments'][2]):r['arguments'][3] for r in resets if r['command']=='D'}
assert doors=={(15004,1):8,(15087,3):8,(15029,5):8,(15030,4):0,(15073,4):6,(15077,5):2}
for v,where,cmd,direction,message_mode in ((15000,15029,46,5,1),(15016,15004,320,1,0),(15023,15087,320,3,0)):
 assert objvalues(objects[v])[0]==29 and not objvalues(objects[v])[7]&1 and objvalues(objects[v])[11:15]==[cmd,where,direction,message_mode]
 assert [r['arguments'][1:5] for r in resets if r['command']=='O' and r['arguments'][1]==v]==[[v,1,where,100]]
assert objvalues(objects[15003])[0]==12 and objvalues(objects[15003])[11:15]==[46,30,5,1] and re.search(r'\bT\s+5\s+2\s+5\s+35\b',objects[15003])
assert edges[(15073,4)][1]==edges[(15077,5)][1]==-2
assert {v:int(re.search(r'\bF\s+(\d+)',b)[1]) for v,b in rooms.items() if re.search(r'\bF\s+(\d+)',b)}=={**{v:5 for v in range(15021,15025)},**{v:3 for v in range(15030,15035)}}
# Bounded source graph assumes successful SEARCH/open, sufficient movement ability and available switches; not played qualification.
reverse={0:2,2:0,1:3,3:1,4:5,5:4,6:9,9:6,7:8,8:7}
def mist_chasm_reachable(switches):
 found={15000};cleared=set();changed=True
 while changed:
  changed=False
  if switches:
   for where,d in ((15029,5),(15004,1),(15087,3)):
    if where in found and (where,d) not in cleared:
     cleared.add((where,d));dest=edges[(where,d)][2];back=(dest,reverse[d])
     if not doors.get((where,d),0)&4:
      if back in edges and edges[back][2]==where:cleared.add(back)
     changed=True
  for edge,(_,key,dest) in edges.items():
   if edge[0] not in found or dest not in rooms:continue
   state=doors.get(edge,0)
   if state&8 and edge not in cleared:continue
   if state&3 in (2,3):continue
   if dest not in found:found.add(dest);changed=True
 return found
assert 15092 in mist_chasm_reachable(True)
assign=(ROOT/'src/world/db.c').read_text();special=(ROOT/'src/specs/specs.object.c').read_text()
assert 'obj_index[nr].func.obj = item_switch;' in assign and 'item_switch(' in special and 'EX_BLOCKED' in special and 'Nothing happens.' in special
commands=(ROOT/'src/cmd/interp.h').read_text();assert re.search(r'CMD_WAKE\s+46\b',commands) and re.search(r'CMD_TOUCH\s+320\b',commands)
quaff=(ROOT/'src/cmd/actoth.c').read_text();at=quaff.index('void do_quaff(');body=quaff[at:quaff.index('\n}',at)+2]
assert 'ROOM_NO_MAGIC' in body and 'extract_obj(' in body and 'skills[j].spell_pointer' in body and 'ITEM_POTION' in body
interp=(ROOT/'src/cmd/interp.c').read_text();assert 'world[ch->in_room].chance_fall' in interp and 'falling_start(ch' in interp
fall=(ROOT/'src/world/falling.c').read_text();assert 'falling_start(' in fall and 'AFF_LEVITATE' in fall and 'AFF_FLY' in fall
move=(ROOT/'src/cmd/actmove.c').read_text();assert 'AFF2_PASSDOOR' in move and 'EX_PICKPROOF' in move
for name in ('do_pick','do_unlock'):
 at=move.index('void '+name+'(');body=move[at:move.index('\n}',at)+2];assert re.search(r'EXIT\(ch, door\)->key < 0',body)
wh=inventory_module.area_evidence(ROOT,'wh');lancer=next(q for q in wh['requests'] if q['block']['line']==2627)
assert lancer['block']['giver_vnum']==55151 and lancer['block']['give']==[('I',15012)] and lancer['block']['receive']==[('I',33719)] and 33719 not in inventory_items
assert 'Native exchange references missing item prototypes' in (ROOT/'docs/reference/zone-story-audits/wh.md').read_text()
hermit=inventory_module.area_evidence(ROOT,'surfacemini');outside=next(q for q in hermit['requests'] if q['block']['line']==26)
assert outside['block']['kind']=='QA' and outside['block']['giver_vnum']==97901 and outside['block']['give']==[('I',15008)] and outside['block']['receive']==[('I',97930)] and 97930 in inventory_items
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==150];assert len(units)==4 and all(u['achievement'] and u['daily_candidate'] for u in units)
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[s['summary'] for s in mapping['stories']]).lower()
for phrase in ('active, ready accounting','two small and two large','wake head','touch flowers','closed but unlocked','reward prototype is missing','four independent histories'):assert phrase in guidance,phrase


# Obsidian Citadel: independent mixed-fee choices, exact follower stock and effective services.
obcita=inventory_module.area_evidence(ROOT,'obcita')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='obcita')
expected={75604:(13,75604,[('I',75602)],False),75608:(27,75614,[('I',75607),('C',20000)],True),75619:(37,75614,[('C',10000),('I',75618)],True),75621:(47,75614,[('C',100000),('I',75620)],True),75642:(68,75630,[('I',75641)],False)}
assert len(obcita['requests'])==5
for q,s in zip(obcita['requests'],mapping['stories']):
 b=q['block'];reward=b['receive'][0][1];line,giver,give,departure=expected[reward]
 assert (b['line'],b['giver_vnum'],b['disappear'])==(line,giver,departure)
 assert [tuple(x) for x in b['give']]==give and b['receive']==[('I',reward)]
 assert s['contracts']==[b['binding']] and s['steps'][1]['contracts']==[b['binding']]
 proof=next(v for k,v in give if k=='I');assert s['steps'][0]['item_vnums']==[proof] and s['steps'][0]['count']==1 and s['steps'][0]['optional']
 assert [t['kind'] for t in s['steps']]==['carried_item','completion']
 assert q['definition']['daily_eligible']==(not departure) and q['definition']['eligible_for_zone_completion']
 assert q['definition']['repeatable']
 if departure:assert q['definition']['daily_exclusion']=='Unsupported durable offering' and 'Currently unavailable while accounting is active' in s['summary']
 if departure:
  fee=next(v for k,v in give if k=='C')
  assert str(fee//1000)+' platinum' in s['steps'][0]['hint'] and 'checks only this loose item' in s['steps'][0]['hint']
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
assert [s['id'] for s in mapping['stories']]==['cleric-staff','shadow-ring','shadow-boots','shadow-bracelet','rolart-shield']
assert len(mapping['contacts'])==17 and sum(len(c['topics']) for c in mapping['contacts'])==17
contacts={c['mob_vnum']:c for c in mapping['contacts']}
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for d in obcita['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert set(c['mob_vnum'] for c in mapping['contacts'] if c['topics'])=={75604,75614,75630}
assert collections.Counter(d['kind'] for d in obcita['dialogue'])=={'M':3}
assert (obcita['zone']['zone_number'],obcita['zone']['first_vnum'],obcita['zone']['last_vnum'],obcita['zone']['reset_mode'])==(756,75599,75778,1)
rooms=dawndale_bodies('obcita','wld');objects=dawndale_bodies('obcita','obj');mobiles=dawndale_bodies('obcita','mob')
assert (len(rooms),len(mobiles),len(objects))==(179,61,70) and set(rooms)==set(range(75600,75779))
edges={(v,int(m[1])):(int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)}
assert len(edges)==382 and {(src,dest) for (src,d),(_,k,dest) in edges.items() if dest not in rooms}=={(75600,579804),(75778,736889),(75778,737288)}
resets=obcita['reset_commands'];assert len(resets)==345 and collections.Counter(r['command'] for r in resets)=={'D':64,'O':34,'P':22,'M':170,'E':32,'G':16,'F':7}
assert len((ROOT/'areas/zon/obcita.zon').read_text().splitlines())==548 and len((ROOT/'areas/qst/obcita.qst').read_text().splitlines())==75
stock=[];parent=room=parent_kind=None
for r in resets:
 a=r['arguments'];cmd=r['command']
 if cmd in ('M','F'):parent,room,parent_kind=a[1],a[3],cmd
 elif cmd in ('E','G'):stock.append((cmd,a[1],parent,room,a[2],a[4],parent_kind))
 assert cmd not in ('M','F','D','O') or (a[1] if cmd=='D' else a[3]) in rooms
 assert cmd not in ('M','F') or a[1] in inventory_mobs
 assert cmd not in ('O','P','G','E') or a[1] in inventory_items
for row in [('E',75602,75602,75610,1,100,'M'),('G',75618,75603,75622,1,100,'M'),('G',75607,75613,75636,1,100,'M'),('G',75620,75618,75668,1,100,'F'),('E',75641,75615,75699,1,100,'M')]:assert [s for s in stock if s[1]==row[1]]==[row]
assert [r['arguments'][1:5] for r in resets if r['command'] in ('M','F') and r['arguments'][1]==75618]==[[75618,2,75668,100],[75618,2,75668,100]]
for v,where,dest in ((75660,75768,75762),(75661,75762,75768)):
 assert objvalues(objects[v])[0]==25 and not objvalues(objects[v])[7]&1 and objvalues(objects[v])[11:14]==[dest,26,-1]
 assert [r['arguments'][1:5] for r in resets if r['command']=='O' and r['arguments'][1]==v]==[[v,1,where,100]]
assert objvalues(objects[75622])[0]==15 and 'prove thy insanity' in objects[75660]
assert (objvalues(objects[75631])[0],bool(objvalues(objects[75631])[7]&1))==(27,True) and objvalues(objects[75632])[13]==75631
assert {v:objvalues(objects[v])[12] for v in (75627,75630,75656)}=={75627:100,75630:100,75656:100}
assert edges[(75673,2)][1]==75627 and edges[(75674,0)][1]==75628 and objvalues(objects[75643])[13]==75656
doors={(r['arguments'][1],r['arguments'][2]):r['arguments'][3] for r in resets if r['command']=='D'}
reverse={0:2,2:0,1:3,3:1,4:5,5:4,6:9,9:6,7:8,8:7}
def obcita_reachable(keys,teleport=False):
 found={75600};unlocked=set();changed=True
 while changed:
  changed=False
  for edge,(_,key,dest) in edges.items():
   if edge[0] not in found or dest not in rooms:continue
   if doors.get(edge,0) in (2,3,6,7) and edge not in unlocked:
    if key not in keys:continue
    unlocked.add(edge);back=(dest,reverse[edge[1]])
    if back in edges and edges[back][2]==edge[0]:unlocked.add(back)
   if dest not in found:found.add(dest);changed=True
  if teleport:
   for src,dest in ((75768,75762),(75762,75768)):
    if src in found and dest not in found:found.add(dest);changed=True
 return found
keys={75627,75628,75630,75631,75634,75640,75644,75654,75656}
assert [len(obcita_reachable(k,t)) for k,t in ((set(),False),(keys,False),(keys,True))]==[147,178,179]
assert set(rooms)-obcita_reachable(keys)=={75762}
assign=(ROOT/'src/specs/specs.assign.c').read_text();special=(ROOT/'src/specs/specs.obsidian_citadel.c').read_text()
for v in range(75631,75640):assert re.search(r'real_mobile0\('+str(v)+r'\)\]\.func\.mob\s*=\s*obsid_cit_death_knight;',assign)
assert '// mob_index[real_mobile0(75615)].func.mob = obsid_cit_death_knight;' in assign
assert re.search(r'real_mobile0\(75640\)\]\.func\.mob\s*=\s*obsid_cit_satar_ghulan;',assign)
assert 'summon_creature(75648' in special and 'if (cmd)' in special
assert [r['arguments'][1:5] for r in resets if r['command']=='M' and r['arguments'][1]==75648]==[[75648,3,75756,100]]*3
pet=(ROOT/'src/mob/pet_lifecycle.c').read_text();setup=(ROOT/'src/classes/necromancy.c').read_text()
assert 'setup_pet(' in pet and 'IS_NPC(ch)' in setup and 'af.duration = -1;' in setup and 'GET_EXP(mob) = 0;' in setup
epic=(ROOT/'src/world/epic.c').read_text();training=(ROOT/'src/classes/epic_skills.c').read_text()
assert re.search(r'75615\s*,\s*SKILL_EXPERT_RIPOSTE',training) and 'void epic_initialization(' in epic and '.func.mob = epic_teacher;' in epic
assert 'economic_gameplay_authority::active()' in training
forge=(ROOT/'src/economy/tradeskill.c').read_text();assert '75628' in forge and 'initialize_tradeskills(' in forge and 'economic_gameplay_authority::active()' in forge
assert 'choice > i' in forge and 'ch->carrying' in forge
ascend=(ROOT/'src/cmd/actoth.c').read_text();at=ascend.index('void do_ascend(');body=ascend[at:ascend.index('\n}',at)+2]
assert body.index('!IS_NPC(ch)')<body.index('75610') and 'ascend_committed' in ascend
travel=(ROOT/'src/magic/spell_travel.c').read_text();assert 'check_item_teleport(' in travel and 'generic_find(' in travel and 'teleport_to(' in travel
move=(ROOT/'src/cmd/actmove.c').read_text();at=move.index('P_obj has_key(');body=move[at:move.index('\n}',at)+2]
assert 'ITEM_KEY' not in body and 'HOLD' in body and 'ch->carrying' in body
interp=(ROOT/'src/cmd/interp.h').read_text();assert re.search(r'CMD_CACKLE\s+26\b',interp)
for v,damage,charges,level in ((75608,5,1,20),(75619,5,1,5),(75616,7,5,50)):assert re.search(r'\bT\s+1\s+'+str(damage)+r'\s+'+str(charges)+r'\s+'+str(level)+r'\b',objects[v])
assert {r['arguments'][1] for r in resets if r['command'] in ('O','P','E','G') and r['arguments'][1] not in objects}=={359,55188}
quest=(ROOT/'src/world/quest.c').read_text();at=quest.index('static bool submit_durable_quest_offering(');admission=quest[at:quest.index('\n}',at)+2]
assert 'goal->goal_type != QUEST_GOAL_ITEM' in admission and 'supported = false;' in admission and 'This quest cannot accept that durable item safely.' in admission
assert 'if (economic_gameplay_authority::active())' in quest and 'This quest cannot accept offerings right now.' in quest
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==756];assert len(units)==5 and sum(u['achievement'] for u in units)==5 and sum(u['daily_candidate'] for u in units)==2
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[s['steps'][0]['hint'] for s in mapping['stories']]).lower()
for phrase in ('checks the loose item only','coin readiness is not supported','active, ready accounting','cackle bookshelf','following greater','shade stays after the accepted return'):assert phrase in guidance,phrase


# Tiamat: same-giver exact assemblies, addressed MA echoes and distinct death outcomes.
tiamat=inventory_module.area_evidence(ROOT,'tiamat')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='tiamat')
blocks={tuple(v for k,v in q['block']['receive']):q['block'] for q in tiamat['requests']}
expected={(19602,):(14,[19616,19617]),(19603,):(19,[19608,19609,19610]),(19604,):(25,[19618,19619]),(19605,):(30,[19621,19622,19623]),(19611,):(36,[19626,19627]),(19612,):(41,[19629,19631,19632]),(19613,):(47,[19633,19635,19636]),(19639,19640):(53,[19641])}
assert set(blocks)==set(expected)
for rewards,(line,proof) in expected.items():
 b=blocks[rewards];assert (b['line'],b['giver_vnum'],b['disappear'])==(line,19616,False)
 assert [tuple(x) for x in b['give']]==[('I',v) for v in proof] and [tuple(x) for x in b['receive']]==[('I',v) for v in rewards]
 assert b['binding']['completion_key']=='give='+','.join('I:'+str(v) for v in sorted(proof))+';receive='+','.join('I:'+str(v) for v in rewards)+';disappear=0'
assert all(q['definition']['daily_eligible'] and q['definition']['repeatable'] and not q['definition']['daily_exclusion'] for q in tiamat['requests'])
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
assert [(s['id'],len(s['steps'])) for s in mapping['stories']]==[('bright-key',3),('ruby-key',4),('golden-key',3),('jeweled-key',4),('bronze-key',3),('red-key',4),('green-key',4),('tiamat-cadaver-tribute',2)]
assert sum(t.get('optional',False) for s in mapping['stories'] for t in s['steps'])==19
for s in mapping['stories']:
 b=next(b for b in blocks.values() if s['contracts']==[b['binding']])
 assert {t['item_vnums'][0]:t['count'] for t in s['steps'] if t['kind']=='carried_item'}==dict(collections.Counter(v for k,v in b['give']))
 assert all(t.get('optional',False) for t in s['steps'][:-1]) and s['steps'][-1]['kind']=='completion' and s['steps'][-1]['contracts']==[b['binding']]
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={19616,19600,19601,19700,19617,19603,19604,19605,19606,19607,19608,19609,19610,19611,19612,19618,19619}
assert collections.Counter(d['kind'] for d in tiamat['dialogue'])=={'MA':2,'M':1} and sum(len(c['topics']) for c in contacts.values())==10
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for d in tiamat['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert contacts[19616]['topics']==['key','fragment','item','something','for','me','tiamat'] and contacts[19617]['topics']==['torment','beg','end']
qst=(ROOT/'areas/qst/tiamat.qst').read_text();assert len(qst.splitlines())==64 and len(re.findall(r'^Q$',qst,re.M))==8 and len(re.findall(r'^MA$',qst,re.M))==2 and len(re.findall(r'^M$',qst,re.M))==1
assert (tiamat['zone']['zone_number'],tiamat['zone']['first_vnum'],tiamat['zone']['last_vnum'],tiamat['zone']['reset_mode'])==(196,19525,19639,0)
rooms=dawndale_bodies('tiamat','wld');objects=dawndale_bodies('tiamat','obj');mobiles=dawndale_bodies('tiamat','mob')
assert (len(rooms),len(mobiles),len(objects))==(39,20,43) and set(rooms)==set(range(19600,19640))-{19623}
edges={(v,int(m[1])):(int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)}
assert len(edges)==116 and edges[(19600,2)][2]==32031 and edges[(19617,3)][2]==19622
resets=tiamat['reset_commands'];assert len(resets)==90 and collections.Counter(r['command'] for r in resets)=={'D':20,'O':7,'M':23,'G':27,'F':13}
assert len((ROOT/'areas/zon/tiamat.zon').read_text().splitlines())==161
assert [r['arguments'] for r in resets if r['command']=='M' and r['arguments'][1]==19616]==[[0,19616,1,19611,100,0,0,0]]
assert [r['arguments'][1:5] for r in resets if r['command']=='M' and r['arguments'][1]==19700]==[[19700,2,19617,100]]
parent=room=None;stock=[]
for r in resets:
 a=r['arguments']
 if r['command'] in ('M','F'):parent,room=a[1],a[3]
 elif r['command']=='G':stock.append((a[1],parent,room,a[2],a[4]))
source_rows={19616:(19604,19613),19617:(19603,19613),19608:(19606,19615),19609:(19618,19615),19610:(19618,19615),19618:(19608,19614),19619:(19619,19614),19621:(19607,19616),19622:(19609,19616),19623:(19609,19616),19626:(19611,19610),19627:(19612,19610),19629:(19610,19618),19631:(19603,19618),19632:(19605,19618),19633:(19603,19619),19635:(19605,19619),19636:(19610,19619)}
for item,(mob,where) in source_rows.items():assert [s for s in stock if s[0]==item]==[(item,mob,where,1,100)]
assert [s for s in stock if s[0]==19601]==[(19601,19601,19612,1,100)]
assert [s for s in stock if s[0] in (19606,19641,19642,51006)]==[(19606,19700,19617,1,100),(19641,19700,19617,1,33),(19642,19700,19617,1,10),(51006,19700,19617,1,100)]
assert not objvalues(objects[19600])[7]&1 and objvalues(objects[19600])[0]==15
assert all(objvalues(objects[v])[0] not in (25,29) for v in objects)
assert objvalues(objects[19601])[12]==0 and all(objvalues(objects[v])[12]==100 for v in (19602,19603,19604,19605,19606,19611,19612,19613))
assert len({objects[v].split('~')[1].strip() for v in (19608,19609,19610)})==1 and len({objects[v].split('~')[1].strip() for v in (19616,19617,19618,19619,19621,19622,19623,19626,19627,19629,19631,19632,19633,19635,19636)})==1
doors={(r['arguments'][1],r['arguments'][2]):r['arguments'][3] for r in resets if r['command']=='D'}
assert {edge for edge,state in doors.items() if state==6}=={(19602,0),(19606,1),(19608,2),(19610,1),(19617,2)}
def tiamat_reachable(keys=None):
 found={19600};pending=[19600]
 while pending:
  src=pending.pop()
  for edge,(state,key,dest) in edges.items():
   if edge[0]!=src or dest not in rooms or dest in found:continue
   if keys is not None and doors.get(edge,0)&2 and key not in keys:continue
   found.add(dest);pending.append(dest)
 return found
keys={19601};stages=[]
for reward in (None,19602,19603,19604,19605,19611,19612,19613,19606):
 if reward:keys.add(reward)
 stages.append(len(tiamat_reachable(keys)))
assert stages==[13,14,15,17,18,19,20,22,23] and len(tiamat_reachable())==23 and 19639 not in tiamat_reachable()
assert 25106 not in inventory_items and '#25106\n' in (ROOT/'areas/obj/brass-old-3.obj').read_text() and 'brass-old-3' not in {z['source_area'] for z in catalog_module.zone_registry(ROOT)}
assign=(ROOT/'src/specs/specs.assign.c').read_text();db=(ROOT/'src/world/db.c').read_text();quest=(ROOT/'src/world/quest.c').read_text()
assert "letterStrn[1] == 'A'" in quest and 'qmp->echoAll ? TO_ROOM : TO_VICT' in quest and 'if (cmd == CMD_ASK || cmd == CMD_TELL)' in quest and '(vict != ch)' in quest
for v,fn in ((19600,'block_dir'),(19617,'tiamat_human_to_rareloads'),(19700,'tiamat')):assert re.search(r'real_mobile0\('+str(v)+r'\)\]\.func\.mob\s*=\s*'+fn+r';',assign)
assert '// world[real_room0(VROOM_TIAMAT_HOME)].funct = TiamatThrone;' in assign and 'real_room(19623)' in (ROOT/'src/specs/specs.room.c').read_text()
assert "if (zone_table[zone].cmd[comm].arg1 == -1)" in db and "zone_table[zone].cmd[comm].command = '!';" in db
underworld=(ROOT/'src/specs/specs.underworld.c').read_text();at=underworld.index('int tiamat(');boss=underworld[at:underworld.index('\n}',at)+2]
assert boss.index('IS_IMMOBILE(ch)')<boss.index('cmd == CMD_DEATH') and 'obj_to_room(unequip_char(ch, i), ch->in_room)' in boss and 'read_object(VOBJ_WH_DRAGONHEART_TIAMAT, VIRTUAL)' in boss and '3 * SECS_PER_MUD_DAY' in boss
winter=(ROOT/'src/specs/specs.winterhaven.c').read_text();at=winter.index('int tiamat_human_to_rareloads(');slave=winter[at:winter.index('\n}',at)+2]
assert all('read_object('+str(v)+', VIRTUAL)' in slave for v in (19911,19916,19637,19638)) and 'number(0, 3)' in slave and 'obj->value[0] = SECS_PER_MUD_DAY' in slave
at=winter.index('int dragon_heart_decay(');heart=winter[at:winter.index('\n}',at)+2]
assert all(phrase in heart for phrase in ('CMD_PERIODIC','OBJ_CARRIED','OBJ_WORN','OBJ_ROOM','OBJ_INSIDE','extract_obj(obj, TRUE)','VOBJ_WH_DRAGONHEART_ROTTED'))
assert re.search(r'real_object0\(66\)[^;]*stat_pool_wis',assign) and re.search(r'real_object0\(360\)[^;]*epic_stone',assign)
assert re.search(r'real_object0\(19638\)[^;]*zion_shield_absorb_proc',assign)
assert [r['arguments'][1:5] for r in resets if r['command']=='O' and r['arguments'][1]==25106]==[[25106,999,19632,100]]
foreign=inventory_module.area_evidence(ROOT,'astral_tiamat')
assert [r['arguments'][1:5] for r in foreign['reset_commands'] if r['command']=='O' and r['arguments'][1]==19907]==[[19907,2,19902,100],[19907,2,19949,100]]
assert [r['arguments'][1:5] for r in foreign['reset_commands'] if r['command']=='O' and r['arguments'][1]==19607]==[[19607,1,19953,25]]
assert objvalues(dawndale_bodies('astral_tiamat','obj')[19907])[11:14]==[19600,7,-1]
assert not any(re.match(r'[OPGE]\s+-?\d+\s+(?:19608|19609|19610|19616|19617|19618|19619|19621|19622|19623|19626|19627|19629|19631|19632|19633|19635|19636|19641)\s',line) for area in catalog_module.zone_registry(ROOT) if area['source_area']!='tiamat' for line in (ROOT/'areas/zon'/(area['source_area']+'.zon')).read_text().splitlines())
guard=db[db.index('static bool reset_command_issues_item('):db.index('/* execute the reset command table')]
assert all("case '"+c+"':" in guard for c in 'AOPGE') and re.search(r'economic_gameplay_authority::active\(\)\s*&&\s*reset_command_issues_item\(ZCMD.command\)\)\s*\{\s*last_cmd = 0;\s*continue;',db)
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==196];assert len(units)==8 and all(u['achievement'] and u['daily_candidate'] for u in units)
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[s['summary'] for s in mapping['stories']]).lower()
for phrase in ('echoed to the room','same visible name','two tribute','timed-heart','potential dailies','active, ready accounting'):assert phrase in guidance,phrase


# Mountain of the Banished: exact returns, spoken routes and independent native services.
mount=inventory_module.area_evidence(ROOT,'mount')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='mount')
blocks={r['block']['giver_vnum']:r['block'] for r in mount['requests']}
expected={9107:(11,[9106],[9109]),9123:(31,[9107],[9108]),9136:(56,[9117,9118,9119,9120],[9132,9135])}
assert set(blocks)==set(expected)
for giver,(line,proof,rewards) in expected.items():
    b=blocks[giver]
    assert (b['line'],b['giver_vnum'],b['disappear'])==(line,giver,False)
    assert [tuple(x) for x in b['give']]==[('I',v) for v in proof] and [tuple(x) for x in b['receive']]==[('I',v) for v in rewards]
    assert b['binding']['completion_key']=='give='+','.join('I:'+str(v) for v in sorted(proof))+';receive='+','.join('I:'+str(v) for v in sorted(rewards))+';disappear=0'
assert all(q['definition']['repeatable'] and q['definition']['daily_eligible'] and not q['definition']['daily_exclusion'] for q in mount['requests'])
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
assert [(s['id'],len(s['steps'])) for s in mapping['stories']]==[('battered-warangel-return',2),('mirrors-for-snowglobe',2),('four-essences-for-lokpan',5)]
assert sum(t.get('optional',False) for s in mapping['stories'] for t in s['steps'])==6
for s in mapping['stories']:
    b=next(b for b in blocks.values() if s['contracts']==[b['binding']])
    rows={t['item_vnums'][0]:t['count'] for t in s['steps'] if t['kind']=='carried_item'}
    assert rows==dict(collections.Counter(v for _,v in b['give']))
    assert all(t.get('optional',False) for t in s['steps'][:-1]) and s['steps'][-1]['kind']=='completion' and s['steps'][-1]['contracts']==[b['binding']]
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={9107,9123,9136,9124,9110,9114,9125,9119,9100,9133,9101,9121,9122,9112}
assert len(mount['dialogue'])==4 and sum(len(c['topics']) for c in contacts.values())==29
for v,c in contacts.items():
    assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
    assert c['topics']==[w for d in mount['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert contacts[9107]['topics']==['torture','angel','warangel','torturing','enemy','capture','limp','battered']
assert contacts[9123]['topics']==['lightning','bolt','bounce','contest','bet','walls','reward']
assert contacts[9136]['topics']==['prophecy','await','wait','waiting','dream','dreaming','lokpan','soul','souls','pakar','tolog','zooox','essence','essences']
qst=(ROOT/'areas/qst/mount.qst').read_text()
assert len(qst.splitlines())==72 and len(re.findall(r'^Q$',qst,re.M))==3 and len(re.findall(r'^M$',qst,re.M))==4
assert (mount['zone']['zone_number'],mount['zone']['first_vnum'],mount['zone']['last_vnum'],mount['zone']['reset_mode'])==(91,9039,9166,1)
rooms=dawndale_bodies('mount','wld');objects=dawndale_bodies('mount','obj');mobiles=dawndale_bodies('mount','mob')
assert (len(rooms),len(mobiles),len(objects))==(67,40,39) and sorted(rooms)==list(range(9100,9167))
edges={(v,int(m[1])):(int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)}
assert len(edges)==135 and edges[(9100,5)][2]==517517
assert edges[(9112,5)]==(7,-2,9113) and edges[(9113,4)]==(1,0,9112)
assert edges[(9113,3)]==(3,-2,9158) and edges[(9158,1)]==(3,-2,9113)
assert 'pandora' in rooms[9112].lower() and 'lokpan' in rooms[9113].lower()
resets=mount['reset_commands'];assert len(resets)==113 and collections.Counter(r['command'] for r in resets)=={'D':8,'O':9,'M':58,'F':7,'E':20,'G':10,'R':1}
assert len((ROOT/'areas/zon/mount.zon').read_text().splitlines())==148
for v,want in ((9107,[[0,9107,1,9138,100,0,0,0]]),(9123,[[0,9123,1,9152,100,0,0,0]]),(9136,[[0,9136,2,9100,100,0,0,0],[0,9136,2,9163,100,0,0,0]])):
    assert [r['arguments'] for r in resets if r['command']=='M' and r['arguments'][1]==v]==want
parent=room=None;stock=[]
for r in resets:
    a=r['arguments']
    if r['command'] in ('M','F'):parent,room=a[1],a[3]
    elif r['command'] in ('E','G'):stock.append((r['command'],a[1],parent,room,a[2],a[3] if r['command']=='E' else 0,a[4]))
for item,mob,where in ((9107,9124,9123),(9117,9110,9141),(9118,9114,9147),(9119,9125,9155),(9120,9119,9150),(9127,9100,9112),(9128,9133,9160),(359,9110,9141),(55444,9110,9141)):
    assert [s for s in stock if s[1]==item]==[('G',item,mob,where,1,0,100)]
assert [r['arguments'][1:5] for r in resets if r['command']=='O' and r['arguments'][1]==9106]==[[9106,1,9145,100]]
assert objvalues(objects[9106])[0]==13 and objvalues(objects[9106])[7]==16385 and objvalues(objects[9107])[7]==16385
assert all(objvalues(objects[v])[6]&4096 for v in (9117,9118,9119,9120))
assert objvalues(objects[9127])[0]==16 and objvalues(objects[9128])[0]==13
doors={(r['arguments'][1],r['arguments'][2]):r['arguments'][3] for r in resets if r['command']=='D'}
assert doors[(9112,5)]==doors[(9113,3)]==doors[(9158,1)]==2 and doors[(9113,4)]==1
routes={9101:(9103,9114),9102:(9114,9103),9104:(9138,9139),9105:(9139,9138),9125:(9158,9159),9126:(9159,9158)}
for v,(src,dest) in routes.items():
    values=objvalues(objects[v]);assert values[0]==25 and values[11:14]==[dest,7,-1] and not values[7]&1
    assert [r['arguments'][1:5] for r in resets if r['command']=='O' and r['arguments'][1]==v]==[[v,1,src,100]]
def mountain_reachable(use_objects=True,exclude=()):
    found={9100};pending=[9100]
    while pending:
        src=pending.pop();destinations=[dest for (v,d),(_,_,dest) in edges.items() if v==src and (v,d) not in exclude]
        if use_objects:destinations += [dest for where,dest in routes.values() if where==src]
        for dest in destinations:
            if dest in rooms and dest not in found:found.add(dest);pending.append(dest)
    return found
assert set(rooms)-mountain_reachable()=={9157,9165,9166}
assert len(mountain_reachable(False))==15 and 9158 in mountain_reachable(False) and 9114 not in mountain_reachable(False)
assert len(mountain_reachable(exclude={(9112,5)}))==12
assert all(r['arguments'][1] in objects or r['arguments'][1] in (72,359,371,55444) for r in resets if r['command'] in ('O','P','G','E'))
assert all(r['arguments'][1] in mobiles for r in resets if r['command'] in ('M','F','R'))
assign=(ROOT/'src/specs/specs.assign.c').read_text();db=(ROOT/'src/world/db.c').read_text()
assert not re.search(r'(?:real_mobile0|real_object0|real_room0)\(91\d\d\)',assign)
assert re.search(r'real_object0\(72\)[^;]*spell_pool',assign) and re.search(r'real_object0\(359\)[^;]*epic_stone',assign)
assert not any(int(b.split('~',4)[4].strip().split()[0])&(1|32768|8388608|2147483648) for b in mobiles.values())
command=(ROOT/'src/cmd/actcomm.c').read_text();at=command.index('void check_magic_doors(');magic=command[at:command.index('\n}',at)+2]
assert 'key == -2' in magic and 'EX_LOCKED' in magic and 'EX_CLOSED' not in magic
assert 'isname(word, arg1)' in magic and 'EX_SECRET' in magic and 'check_magic_doors(ch, argument + i)' in command
travel_source=(ROOT/'src/magic/spell_travel.c').read_text();travel=travel_source[travel_source.index('bool check_item_teleport('):]
at=travel_source.index('void teleport_to(');movement=travel_source[at:travel_source.index('\n}',at)+2]
assert 'teleport_to(ch, to_room' in travel and 'ITEM_TELEPORT' in travel and 'char_to_room(ch, to_room, 0)' in movement
epic=(ROOT/'src/world/epic.c').read_text();absorb=epic[epic.index('void epic_stone_absorb('):epic.index('int epic_stone(',epic.index('void epic_stone_absorb('))]
assert 'tobj = tobj->next_content' in absorb and absorb.index('extract_obj(tobj)')<absorb.rindex('tobj->affected[0]')
assert 'zone_touch' in epic and 'epic_publish_zone_touch' in epic
assert [r['arguments'][1:5] for r in resets if r['command']=='R']==[[9128,1,9147,100]]
legacy=dawndale_bodies(pathlib.Path(inventory_items[371]['source']).stem,'obj')[371]
assert objvalues(legacy)[0]==10 and objvalues(legacy)[12:15]==[0,0,0] and not objvalues(legacy)[7]&1 and re.search(r'A\s+8\s+46',legacy)
all_native=inventory_module.native_blocks(ROOT)
for essence,giver,rewards,proof in ((71228,71236,[71230],[71228]),(87588,87598,[87591],[87587,87588,87589,87590])):
    b=next(b for b in all_native if b['kind']=='Q' and b['giver_vnum']==giver and ('I',essence) in [tuple(x) for x in b['give']])
    assert [tuple(x) for x in b['give']]==[('I',v) for v in proof] and [tuple(x) for x in b['receive']]==[('I',v) for v in rewards] and not b['disappear']
for item,mob,where in ((71228,71248,71326),(87588,87602,87673)):
    area=pathlib.Path(inventory_mobs[mob]['source']).stem
    foreign=inventory_module.area_evidence(ROOT,area);parent=room=None;matched=[]
    for r in foreign['reset_commands']:
        a=r['arguments']
        if r['command'] in ('M','F'):parent,room=a[1],a[3]
        elif r['command']=='G' and a[1]==item:matched.append((parent,room,a[2],a[4]))
    assert matched==[(mob,where,1,100)]
assert 'necklace' in inventory_items[71248]['name'].lower() and 'bodyguard' in inventory_mobs[71248]['name'].lower()
assert not any(re.match(r'[OPGE]\s+-?\d+\s+(?:9106|9107|9117|9118|9119|9120)\s',line) for area in catalog_module.zone_registry(ROOT) if area['source_area']!='mount' for line in (ROOT/'areas/zon'/(area['source_area']+'.zon')).read_text().splitlines())
guard=db[db.index('static bool reset_command_issues_item('):db.index('/* execute the reset command table')]
assert all("case '"+c+"':" in guard for c in 'AOPGE') and re.search(r'economic_gameplay_authority::active\(\)\s*&&\s*reset_command_issues_item\(ZCMD.command\)\)\s*\{\s*last_cmd = 0;\s*continue;',db)
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==91]
assert len(units)==3 and all(u['achievement'] and u['daily_candidate'] for u in units)
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[s['summary'] for s in mapping['stories']]).lower()
for phrase in ('pandora','lokpan','four exact','two tribute rewards','one history','active, ready accounting'):assert phrase in guidance,phrase


# Cloud Giant Kingdom: exact multiset proof, alternative stock and separate effective training.
cloud=inventory_module.area_evidence(ROOT,'cldgt')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='cldgt')
blocks={r['block']['giver_vnum']:r['block'] for r in cloud['requests']}
expected={99501:(7,[99509]*5+[99510],99503,False),99520:(36,list(range(99518,99524)),99516,True),99548:(68,[32490,26614,402],405,False)}
assert set(blocks)==set(expected)
for giver,(line,proof,reward,departure) in expected.items():
    b=blocks[giver]
    assert (b['line'],b['giver_vnum'],b['disappear'])==(line,giver,departure)
    assert [tuple(x) for x in b['give']]==[('I',v) for v in proof] and [tuple(x) for x in b['receive']]==[('I',reward)]
    assert b['binding']['completion_key']=='give='+','.join('I:'+str(v) for v in sorted(proof))+f';receive=I:{reward};disappear={int(departure)}'
assert all(q['definition']['repeatable'] and q['definition']['daily_eligible'] and not q['definition']['daily_exclusion'] for q in cloud['requests'])
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
assert len(mapping['stories'])==3 and sum(len(s['steps']) for s in mapping['stories'])==16 and sum(t.get('optional',False) for s in mapping['stories'] for t in s['steps'])==13
for s in mapping['stories']:
    b=next(b for b in blocks.values() if s['contracts']==[b['binding']])
    need=collections.Counter(v for _,v in b['give'])
    rows={t['item_vnums'][0]:t['count'] for t in s['steps'] if t['kind']=='carried_item'}
    assert all(rows[v]==count for v,count in need.items())
    assert set(rows)==set(need)|({99505} if b['giver_vnum'] in (99501,99548) else set())
    assert all(t.get('optional',False) for t in s['steps'][:-1]) and s['steps'][-1]['kind']=='completion' and s['steps'][-1]['contracts']==[b['binding']]
assert [(s['id'],len(s['steps'])) for s in mapping['stories']]==[('five-pelts-and-strap',4),('six-scalps-for-anne',7),('legacy-charisma-scroll',5)]
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={99501,99520,99548,99508,99516,99517,99522,99503,99505,99507,99524,99525,99527,99529,99528,99526,99530}
assert len(cloud['dialogue'])==4 and sum(len(c['topics']) for c in contacts.values())==13
for v,c in contacts.items():
    assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
    assert c['topics']==[w for d in cloud['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert contacts[99501]['topics']==['yeti'] and contacts[99508]['topics']==['giants','city','giant','cloud']
assert contacts[99520]['topics']==['hi','hello','greetings','salutations','scalp','evils','evil','revenge'] and contacts[99548]['topics']==[]
qst=(ROOT/'areas/qst/cldgt.qst').read_text()
assert len(qst.splitlines())==76 and len(re.findall(r'^Q$',qst,re.M))==3 and len(re.findall(r'^M$',qst,re.M))==6 and 'qc_action 45~' in qst and 'qc_action 48~' in qst
assert (cloud['zone']['zone_number'],cloud['zone']['first_vnum'],cloud['zone']['last_vnum'],cloud['zone']['reset_mode'])==(995,99500,99680,1)
rooms=dawndale_bodies('cldgt','wld');objects=dawndale_bodies('cldgt','obj');mobiles=dawndale_bodies('cldgt','mob')
assert (len(rooms),len(mobiles),len(objects))==(181,63,43) and sorted(rooms)==list(range(99500,99681))
edges={(v,int(m[1])):(int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)}
assert len(edges)==391 and edges[(99500,2)][2]==553314 and edges[(99536,2)][2]==99562
assert [edges[k] for k in ((99538,0),(99561,2))]==[(3,99505,99561),(6,99505,99538)]
resets=cloud['reset_commands'];assert len(resets)==323 and collections.Counter(r['command'] for r in resets)=={'D':86,'O':1,'M':177,'E':26,'G':24,'F':9}
assert len((ROOT/'areas/zon/cldgt.zon').read_text().splitlines())==592
for v,where in ((99501,99537),(99520,99529),(99548,99607)):
    assert [r['arguments'] for r in resets if r['command']=='M' and r['arguments'][1]==v]==[[0,v,1,where,100,0,0,0]]
parent=room=None;stock=[]
for r in resets:
    a=r['arguments']
    if r['command'] in ('M','F'):parent,room=a[1],a[3]
    elif r['command'] in ('E','G'):stock.append((r['command'],a[1],parent,room,a[2],a[3] if r['command']=='E' else 0,a[4]))
assert [s for s in stock if s[1]==99509]==[('G',99509,v,where,5,0,100) for v,where in ((99503,99555),(99503,99560),(99505,99574),(99505,99576),(99503,99578))]
assert [s for s in stock if s[1]==99510]==[('E',99510,99507,99561,1,13,100)]
assert [s for s in stock if s[1]==99505]==[('G',99505,99516,99518,2,0,100),('G',99505,99508,99562,2,0,100)]
scalps={99518:[('G',99518,99524,99585,1,0,100)],99519:[('G',99519,99517,99516,2,0,100),('G',99519,99525,99585,2,0,100)],99520:[('G',99520,99527,99585,1,0,100)],99521:[('E',99521,99522,99580,2,18,100),('E',99521,99528,99587,2,18,100)],99522:[('G',99522,99526,99590,1,0,100)],99523:[('G',99523,99529,99587,1,0,100)]}
for v,want in scalps.items():assert [s for s in stock if s[1]==v]==want and objvalues(objects[v])[6]&4096
assert objvalues(objects[99518])[6]&32768 and objvalues(objects[99510])[7]&2048
assert objvalues(objects[99505])[0]==18 and objvalues(objects[99505])[12]==100 and objvalues(objects[99505])[6]&8388608
doors={(r['arguments'][1],r['arguments'][2]):r['arguments'][3] for r in resets if r['command']=='D'}
assert doors[(99538,0)]==doors[(99561,2)]==2
assert [(v,d,edges[(v,d)][2]) for (v,d),state in doors.items() if state&4]==[(99584,5,99585),(99586,5,99587),(99589,5,99590)]
assert all(doors[(v,4)]==1 for v in (99585,99587,99590))
def cloud_reachable(exclude=()):
    found={99500};pending=[99500]
    while pending:
        src=pending.pop()
        for (v,d),(_,_,dest) in edges.items():
            if v==src and (v,d) not in exclude and dest in rooms and dest not in found:found.add(dest);pending.append(dest)
    return found
assert cloud_reachable()==set(rooms)
before_boulder=cloud_reachable({(99538,0),(99561,2)})
assert len(before_boulder)==91 and 99562 in before_boulder and 99561 not in before_boulder and 99607 not in before_boulder
sectors={v:int(b.split('~',2)[2].strip().splitlines()[0].split()[2]) for v,b in rooms.items()}
assert {v for v,s in sectors.items() if s==8}=={99591,99595,99600}
assert all(int(rooms[v].split('~',2)[2].strip().splitlines()[0].split()[1])&131072 for v in range(99653,99660))
assert all(objvalues(b)[0] not in (25,29) for b in objects.values())
assert [r['arguments'][1] for r in resets if r['command']=='O']==[99524] and objvalues(objects[99524])[0]==13 and objvalues(objects[99524])[7]==0
assert all(r['arguments'][1] in objects for r in resets if r['command'] in ('O','P','G','E')) and all(r['arguments'][1] in mobiles for r in resets if r['command'] in ('M','F'))
for v in (402,405):assert v in inventory_items
legacy=dawndale_bodies(pathlib.Path(inventory_items[405]['source']).stem,'obj')[405]
assert objvalues(legacy)[0]==13 and objvalues(legacy)[11:19]==[0]*8
outside=[b for b in inventory_module.native_blocks(ROOT) if b['source']!='areas/qst/cldgt.qst' and b['kind'] in ('Q','QA') and any(kind=='I' and v in (32490,26614,402) for kind,v in b['give'])]
assert len(outside)==10 and len([b for b in outside if ('I',402) in [tuple(x) for x in b['give']]])==8
for area,item,source_mob,where,command,slot in (('bctdl',32490,32420,32469,'G',0),('negplane',26614,26642,26859,'E',16)):
    foreign=inventory_module.area_evidence(ROOT,area);parent=room=None;matched=[]
    for r in foreign['reset_commands']:
        a=r['arguments']
        if r['command'] in ('M','F'):parent,room=a[1],a[3]
        elif r['command'] in ('E','G') and a[1]==item:matched.append((r['command'],parent,room,a[2],a[3] if r['command']=='E' else 0,a[4]))
    assert matched==[(command,source_mob,where,1,slot,100)]
assert not any(re.match(r'[OPGE]\s+-?\d+\s+402\s',line) for area in catalog_module.zone_registry(ROOT) for line in (ROOT/'areas/zon'/(area['source_area']+'.zon')).read_text().splitlines())
assign=(ROOT/'src/specs/specs.assign.c').read_text();db=(ROOT/'src/world/db.c').read_text()
assert not re.search(r'(?:real_mobile0|real_object0|real_room0)\((?:995\d\d|996\d\d|402|405)\)',assign)
assert 'state &=\n\t\t3;' in db and 'if (state == 2)' in db and 'EX_PICKABLE' in db and 'if (state == 3)' in db and 'EX_PICKPROOF' in db
assert 'if (IS_ACT(mob, ACT_TEACHER) && !mob_index[nr].func.mob)' in db and 'mob_index[nr].func.mob = teacher;' in db
teacher_flags={v for v,b in mobiles.items() if int(b.split('~',4)[4].strip().split()[0])&32768};assert teacher_flags=={99520}
skills=(ROOT/'src/classes/epic_skills.c').read_text();assert '{ 99548, SKILL_EMPOWER_SONG, 0, 100, 0, 0 }' in skills
teaching=skills[skills.index('int teacher('):skills.index('int epic_teacher(')]
assert 'cmd != CMD_ASK' in teaching and 'strstr(arg, "level")' in teaching and 'GET_CLASS(pl, ch->player.m_class)' in teaching
epic=skills[skills.index('int epic_teacher('):skills.index('int epic_teacher(')+13000]
assert 'CMD_PRACTICE' in epic and 'Epic skill purchases are unavailable while economic accounting is active.' in epic
assert epic.index('economic_gameplay_authority::active()')<epic.index('epic_transaction_submit') and '405' not in epic
startup=(ROOT/'src/world/epic.c').read_text();startup=startup[startup.index('void epic_initialization('):]
assert 'epic_teachers[i].vnum' in startup and 'func.mob = epic_teacher;' in startup
dispatch=(ROOT/'src/cmd/interp.c').read_text();dispatch=dispatch[dispatch.index('bool special('):dispatch.index('bool special(')+2500]
assert '.func.mob)(k, ch, cmd, arg)' in dispatch and '.qst_func)(k, ch, cmd, arg)' in dispatch and 'ACT_SPEC' not in dispatch
read_command=(ROOT/'src/cmd/actinf.c').read_text();read_command=read_command[read_command.index('void do_read('):read_command.index('void do_read(')+500]
assert 'do_look' in read_command and 'SKILL_EMPOWER_SONG' not in read_command
guard=db[db.index('static bool reset_command_issues_item('):db.index('/* execute the reset command table')]
assert all("case '"+c+"':" in guard for c in 'AOPGE') and re.search(r'economic_gameplay_authority::active\(\)\s*&&\s*reset_command_issues_item\(ZCMD.command\)\)\s*\{\s*last_cmd = 0;\s*continue;',db)
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==995]
assert len(units)==3 and all(u['achievement'] and u['daily_candidate'] for u in units)
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[s['summary'] for s in mapping['stories']]).lower()
for phrase in ('five exact','six exact','shared','tablet producer','empower song','active, ready accounting'):assert phrase in guidance,phrase


# Myrloch Vale: independent returns, alternative keys, shared mechanisms and floor loot.
myrloch=inventory_module.area_evidence(ROOT,'myrloch_vale')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='myrloch_vale')
blocks={r['block']['give'][0][1]:r['block'] for r in myrloch['requests']}
expected={26417:(33,26411,26409,False),26420:(40,26411,26421,False),26433:(47,26411,26434,True),26418:(72,26417,26406,True)}
assert set(blocks)==set(expected)
for proof,(line,giver,reward,departure) in expected.items():
    b=blocks[proof]
    assert (b['line'],b['giver_vnum'],b['disappear'])==(line,giver,departure)
    assert [tuple(x) for x in b['give']]==[('I',proof)] and [tuple(x) for x in b['receive']]==[('I',reward)]
    assert b['binding']['completion_key']==f'give=I:{proof};receive=I:{reward};disappear={int(departure)}'
assert all(q['definition']['repeatable'] and q['definition']['daily_eligible'] and not q['definition']['daily_exclusion'] for q in myrloch['requests'])
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
assert len(mapping['stories'])==4 and sum(len(s['steps']) for s in mapping['stories'])==11 and sum(t.get('optional',False) for s in mapping['stories'] for t in s['steps'])==7
for s in mapping['stories']:
    b=next(b for b in blocks.values() if s['contracts']==[b['binding']])
    assert s['steps'][0]['item_vnums']==[b['give'][0][1]] and s['steps'][0]['count']==1 and s['steps'][0]['optional']
    assert s['steps'][-1]['kind']=='completion' and s['steps'][-1]['contracts']==[b['binding']]
assert [(s['id'],[t['item_vnums'][0] for t in s['steps'] if t['kind']=='carried_item']) for s in mapping['stories']]==[('return-old-key',[26417]),('return-old-bracelet',[26420]),('return-old-dagger',[26433]),('return-warlord-head',[26418,26409,26412,26411])]
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={26411,26417,26402,26413,26400,26401,26406,26407,26408,26410,26409}
assert len(myrloch['dialogue'])==4 and sum(len(c['topics']) for c in contacts.values())==6
for v,c in contacts.items():
    assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
    assert c['topics']==[w for d in myrloch['dialogue'] if d['giver_vnum']==v and not d['body'][0].startswith('qc_unblock ') for w in d['body'][0].rstrip('~').split()]
assert contacts[26411]['topics']==['quest','quests','head','warlord','fire'] and contacts[26417]['topics']==['chest']
assert all(not c['topics'] for v,c in contacts.items() if v not in (26411,26417))
qst=(ROOT/'areas/qst/myrloch_vale.qst').read_text()
assert len(qst.splitlines())==82 and len(re.findall(r'^Q$',qst,re.M))==4 and len(re.findall(r'^M$',qst,re.M))==5
assert 'qc_action 50~' in qst and 'qc_unblock 26555 north~' in qst
assert (myrloch['zone']['zone_number'],myrloch['zone']['first_vnum'],myrloch['zone']['last_vnum'],myrloch['zone']['reset_mode'])==(264,26341,26578,1)
rooms=dawndale_bodies('myrloch_vale','wld');objects=dawndale_bodies('myrloch_vale','obj');mobiles=dawndale_bodies('myrloch_vale','mob')
assert (len(rooms),len(mobiles),len(objects))==(178,18,45) and sorted(rooms)==list(range(26401,26579))
edges={(v,int(m[1])):(int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)}
assert len(edges)==373
assert [edges[k] for k in ((26492,5),(26493,4),(26516,1),(26517,3),(26530,0),(26531,2))]==[(2,26409,26493),(2,26409,26492),(3,26412,26517),(3,26412,26516),(3,26411,26531),(3,26411,26530)]
assert edges[(26539,0)][2]==26545 and edges[(26545,2)][2]==26540 and edges[(26559,2)][2]==26558 and edges[(26467,4)][2]==26425
resets=myrloch['reset_commands'];assert len(resets)==184 and collections.Counter(r['command'] for r in resets)=={'D':18,'O':18,'P':3,'M':84,'E':45,'G':16}
for v,where in ((26411,26578),(26417,26434),(26402,26555)):
    assert [r['arguments'] for r in resets if r['command']=='M' and r['arguments'][1]==v]==[[0,v,1,where,100,0,0,0]]
for v,where in ((26420,26471),(26433,26472),(26417,26492),(26405,26434),(26442,26515)):
    assert [r['arguments'] for r in resets if r['command']=='O' and r['arguments'][1]==v]==[[0,v,1,where,100,0,0,0]]
assert [r['arguments'] for r in resets if r['command']=='P']==[[1,26419,1,26405,100,0,0,0],[1,26550,1,26405,100,0,0,0],[1,26441,1,26442,100,0,0,0]]
parent=room=None;stock=[]
for r in resets:
    a=r['arguments']
    if r['command'] in ('M','F'):parent,room=a[1],a[3]
    elif r['command'] in ('E','G'):stock.append((r['command'],a[1],parent,room,a[2],a[3] if r['command']=='E' else 0,a[4]))
for item,mob,where in ((26409,26413,26480),(26412,26400,26514),(26443,26400,26515),(26411,26401,26530),(26418,26402,26555),(358,26402,26555),(55455,26402,26555)):
    assert [s for s in stock if s[1]==item]==[('G',item,mob,where,1,0,100)]
assert [s for s in stock if s[1]==26438]==[('E',26438,26402,26555,1,11,100)]
for v in (26420,26433,26417):
    values=objvalues(objects[v]);assert values[6]&4096 and values[7]&1 and values[8]&32768
for v in (26406,26409,26411,26412,26417,26443):
    values=objvalues(objects[v]);assert values[0]==18 and values[6]&8388608 and values[12]==100
assert objvalues(objects[26405])[0]==15 and objvalues(objects[26405])[7]==0 and objvalues(objects[26405])[11:15]==[500,31,26406,250]
assert objvalues(objects[26442])[11:15]==[5,15,26443,250]
teleports=[(r['arguments'][1],r['arguments'][3],objvalues(objects[r['arguments'][1]])[11:14]) for r in resets if r['command']=='O' and objvalues(objects[r['arguments'][1]])[0]==25]
assert teleports==[(26400,26413,[26414,320,-1]),(26401,26425,[26452,320,-1]),(26402,26425,[26451,320,-1]),(26403,26425,[26453,320,-1]),(26404,26425,[26454,320,-1]),(26407,26440,[26309,7,-1])]
assert objvalues(objects[26437])[0]==25 and objvalues(objects[26437])[11:14]==[26425,7,-1] and objvalues(objects[26437])[6]&4096
assert not any(r['command'] in 'OPGE' and r['arguments'][1]==26437 for r in resets)
switches=[(r['arguments'][1],r['arguments'][3],objvalues(objects[r['arguments'][1]])[11:15]) for r in resets if r['command']=='O' and objvalues(objects[r['arguments'][1]])[0]==29]
assert switches==[(26413,26547,[340,26540,0,0]),(26414,26549,[340,26534,1,0]),(26415,26551,[340,26534,0,0]),(26416,26553,[340,26540,3,0])]
reverse={0:2,1:3,2:0,3:1,4:5,5:4}
for v,where,(_,target,d,_) in switches:
    assert objvalues(objects[v])[6]&4096 and edges[(edges[(target,d)][2],reverse[d])][2]==target
blocked={(r['arguments'][1],r['arguments'][2]) for r in resets if r['command']=='D' and r['arguments'][3]&8}
assert blocked=={(26534,0),(26534,1),(26540,0),(26540,3),(26555,0)}
def myrloch_reachable(blocked=(),travel=True):
    found={26401};pending=[26401]
    while pending:
        src=pending.pop();targets=[dest for (v,d),(_,_,dest) in edges.items() if v==src and (v,d) not in blocked]
        if travel:targets += [values[0] for _,v,values in teleports if v==src]
        for dest in targets:
            if dest in rooms and dest not in found:found.add(dest);pending.append(dest)
    return found
assert myrloch_reachable(travel=False)==set(range(26401,26414)) and myrloch_reachable()==set(rooms)
current=set(blocked)
for expected_switch,size in ((26414,157),(26415,161),(26416,172),(26413,176)):
    reachable=myrloch_reachable(current);assert len(reachable)==size
    available=[s for s in switches if s[1] in reachable and (s[2][1],s[2][2]) in current]
    assert [s[0] for s in available]==[expected_switch]
    current.remove((available[0][2][1],available[0][2][2]))
    if expected_switch==26415:assert 26555 in myrloch_reachable(current)
assert set(rooms)-myrloch_reachable(current)=={26558,26559}
current.remove((26555,0));assert myrloch_reachable(current)==set(rooms)
assign=(ROOT/'src/specs/specs.assign.c').read_text();db=(ROOT/'src/world/db.c').read_text()
assert 'mob_index[real_mobile0(26402)].func.mob = unblock_on_death;' in assign and 'obj_index[real_object0(358)].func.obj = epic_stone;' in assign
assert 'if (mob_index[nr].func.mob && !IS_SET(mob->specials.act, ACT_SPEC))' in db and 'SET_BIT(mob->specials.act, ACT_SPEC);' in db
assert 'obj->type == ITEM_SWITCH && !obj_index[nr].func.obj' in db
flags=int(mobiles[26402].split('~',4)[4].strip().split()[0]);assert flags&8388608 and not flags&1
death=(ROOT/'src/specs/specs.exit_barriers.c').read_text();death=death[death.index('int unblock_on_death('):death.index('int unblock_on_death(')+2400]
assert 'QC_UNBLOCK' in death and 'CMD_DEATH' in death and 'EX_BLOCKED' in death and '/*pl*/' in death and 'make_corpse' not in death
fight=(ROOT/'src/combat/fight.c').read_text();assert '(*mob_index[GET_RNUM(ch)].func.mob)(ch, killer, CMD_DEATH, 0);' in fight
extract=(ROOT/'src/world/handler.c').read_text();extract=extract[extract.index('void extract_char(P_char ch)'):]
assert 'obj = unequip_char(ch, l)' in extract and 'obj_from_char(obj);' in extract and extract.count('obj_to_room(obj, ch->in_room);')>=2 and 'ITEM_TRANSIENT' in extract
for v in (26418,26438):assert not objvalues(objects[v])[6]&524288
for v in (358,55455):assert not objvalues(dawndale_bodies(pathlib.Path(inventory_items[v]['source']).stem,'obj')[v])[6]&524288
travel=(ROOT/'src/magic/spell_travel.c').read_text();travel=travel[travel.index('bool check_item_teleport('):]
assert 'ITEM_TELEPORT' in travel and 'teleport_to' in travel and 'obj->value[1] == cmd' in travel
assert 'check_item_teleport(exec_char, argument + begin + look_at, cmd)' in (ROOT/'src/cmd/interp.c').read_text()
epic=(ROOT/'src/world/epic.c').read_text();epic=epic[epic.index('int epic_stone('):]
assert 'zone_touch_transaction_submit' in epic and 'stone_uid' in epic and 'CMD_TOUCH' in epic
outside=inventory_module.area_evidence(ROOT,'caves_skelenak')
assert [(r['block']['line'],r['block']['giver_vnum'],r['block']['receive'],r['block']['disappear']) for r in outside['requests'] if r['block']['give']==[('I',26438)]]==[(155,4038,[('I',4029)],False)]
guard=db[db.index('static bool reset_command_issues_item('):db.index('/* execute the reset command table')]
assert all("case '"+c+"':" in guard for c in 'AOPGE')
assert re.search(r'economic_gameplay_authority::active\(\)\s*&&\s*reset_command_issues_item\(ZCMD.command\)\)\s*\{\s*last_cmd = 0;\s*continue;',db)
capture=(ROOT/'src/player/player_snapshot_capture.c').read_text()
assert 'ownership.state == item_custody_state::active' in capture and 'omit_norent && IS_SET(object->extra_flags, ITEM_NORENT) && !active_durable_custody' in capture
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==264]
assert len(units)==4 and all(u['achievement'] and u['daily_candidate'] for u in units)
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[s['summary'] for s in mapping['stories']]).lower()
for phrase in ('master leave','red flame','room floor','shared','stairway','active, ready accounting'):assert phrase in guidance,phrase


# Bandit Camp: independent rescue returns, locked sources and supplied insignias.
bandit=inventory_module.area_evidence(ROOT,'banditca')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='banditca')
blocks={r['block']['give'][0][1]:r['block'] for r in bandit['requests']}
expected={14831:(38,14807,'E',10000,True),14826:(150,14820,'I',14827,False),14821:(158,14820,'C',5000,False),14828:(174,14824,'E',125000,True)}
assert set(blocks)==set(expected)
for proof,(line,giver,kind,reward,departure) in expected.items():
    b=blocks[proof]
    assert (b['line'],b['giver_vnum'],b['disappear'])==(line,giver,departure)
    assert [tuple(x) for x in b['give']]==[('I',proof)] and [tuple(x) for x in b['receive']]==[(kind,reward)]
    assert b['binding']['completion_key']==f'give=I:{proof};receive={kind}:{reward};disappear={int(departure)}'
assert all(q['definition']['repeatable'] and q['definition']['daily_eligible'] and not q['definition']['daily_exclusion'] for q in bandit['requests'])
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
assert len(mapping['stories'])==4 and sum(len(s['steps']) for s in mapping['stories'])==10 and sum(t.get('optional',False) for s in mapping['stories'] for t in s['steps'])==6
for s in mapping['stories']:
    b=next(b for b in blocks.values() if s['contracts']==[b['binding']])
    assert s['steps'][0]['item_vnums']==[b['give'][0][1]] and s['steps'][0]['count']==1 and s['steps'][0]['optional']
    assert s['steps'][-1]['kind']=='completion' and s['steps'][-1]['contracts']==[b['binding']]
assert [(s['id'],[t['item_vnums'][0] for t in s['steps'] if t['kind']=='carried_item']) for s in mapping['stories']]==[('free-slave-groups',[14831]),('perrins-son-ring',[14826,14833]),('disrupt-inner-circle',[14821]),('free-perrins-daughter',[14828,14822])]
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={14820,14807,14824,14823,14814,14813,14811,14826,14817,14822,14825,14805,14806,14819}
assert len(bandit['dialogue'])==4 and sum(len(c['topics']) for c in contacts.values())==8
for v,c in contacts.items():
    assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
    assert c['topics']==[w for d in bandit['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert contacts[14820]['topics']==['hi','hello','quest','help','hail','boy','girl','kill'] and all(not c['topics'] for v,c in contacts.items() if v!=14820)
qst=(ROOT/'areas/qst/banditca.qst').read_text()
assert len(qst.splitlines())==188 and len(re.findall(r'^MA\nqc_action \d+~',qst,re.M))==14 and len(re.findall(r'^Q$',qst,re.M))==4 and len(re.findall(r'^M$',qst,re.M))==4
assert (bandit['zone']['zone_number'],bandit['zone']['first_vnum'],bandit['zone']['last_vnum'],bandit['zone']['reset_mode'])==(148,14700,14926,2)
rooms=dawndale_bodies('banditca','wld');objects=dawndale_bodies('banditca','obj');mobiles=dawndale_bodies('banditca','mob')
assert (len(rooms),len(mobiles),len(objects))==(127,30,36) and sorted(rooms)==list(range(14800,14927))
edges={(v,int(m[1])):(int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)}
assert len(edges)==311
assert edges[(14833,1)]==(2,0,14834) and edges[(14834,3)]==(2,0,14833)
assert edges[(14806,0)]==(0,0,14914) and edges[(14914,2)]==(0,0,14806)
assert [edges[k] for k in ((14814,5),(14816,4),(14818,4),(14859,5))]==[(5,0,14816),(1,0,14814),(1,0,14859),(5,0,14818)]
def bandit_reachable(blocked=()):
    found={14800};pending=[14800]
    while pending:
        v=pending.pop()
        for (src,d),(_,_,dest) in edges.items():
            if src==v and (src,d) not in blocked and dest in rooms and dest not in found:
                found.add(dest);pending.append(dest)
    return found
assert bandit_reachable()==set(rooms)
assert set(rooms)-bandit_reachable({(14833,1),(14834,3)})=={14832,14833,14842,14843}
resets=bandit['reset_commands'];assert len(resets)==234 and collections.Counter(r['command'] for r in resets)=={'D':30,'O':39,'P':21,'M':92,'F':6,'E':27,'G':19}
for v,where,cap in ((14807,(14835,14845),2),(14820,(14808,),1),(14824,(14843,),1)):
    assert [r['arguments'] for r in resets if r['command']=='M' and r['arguments'][1]==v]==[[0,v,cap,room,100,0,0,0] for room in where]
for v,where in ((14820,14842),(14832,14864)):
    assert [r['arguments'] for r in resets if r['command']=='O' and r['arguments'][1]==v]==[[0,v,1,where,100,0,0,0]]
assert [r['arguments'] for r in resets if r['command']=='P' and r['arguments'][1] in (14822,14828,14826,14834)]==[[1,14822,1,14820,100,0,0,0],[1,14828,1,14820,100,0,0,0],[1,14834,1,14832,100,0,0,0],[1,14826,1,14832,70,0,0,0]]
parent=room=None;stock=[]
for r in resets:
    a=r['arguments']
    if r['command'] in ('M','F'):parent,room=a[1],a[3]
    elif r['command'] in ('E','G'):stock.append((r['command'],a[1],parent,room,a[2],a[3] if r['command']=='E' else 0,a[4]))
assert [s for s in stock if s[1]==14831]==[('E',14831,14823,14835,2,18,100),('E',14831,14823,14845,2,18,100)]
assert [s for s in stock if s[1]==14833]==[('G',14833,14814,14864,1,0,100)]
assert [s for s in stock if s[1]==14821]==[('E',14821,m,r,12,24,100) for m,r in ((14811,14830),(14823,14835),(14826,14836),(14817,14863),(14814,14864),(14817,14866),(14822,14867),(14825,14877),(14805,14891),(14806,14897),(14806,14907),(14819,14920))]
assert len({s[2] for s in stock if s[1]==14821})==10
foreign=inventory_module.area_evidence(ROOT,'tharnadian_ruin')['reset_commands'];parent=room=None;outside=[]
for r in foreign:
    a=r['arguments']
    if r['command'] in ('M','F'):parent,room=a[1],a[3]
    elif r['command']=='G' and a[1]==14821:outside.append((parent,room,a[2],a[4]))
assert outside==[(5519,5508,1,100)]
for room,d in ((14814,5),(14816,4),(14818,4),(14859,5)):
    assert [r['arguments'][3] for r in resets if r['command']=='D' and r['arguments'][1:3]==[room,d]]==[1]
desk=objvalues(objects[14820]);chest=objvalues(objects[14832]);ring=objvalues(objects[14826]);insignia=objvalues(objects[14821])
assert desk[0]==chest[0]==15 and desk[7]==0 and desk[11:15]==[100,13,0,100] and chest[11:15]==[100,13,14833,100]
assert ring[0]==8 and ring[7]&1 and ring[8]&32768
assert insignia[0]==13 and insignia[7]==2097153 and insignia[6]&8 and insignia[8]&32768
for v in (14828,14831,14833):
    values=objvalues(objects[v]);assert values[0]==18 and values[7]&1 and values[6]&8388608
    assert values[12]==(100 if v==14833 else 0)
assert not bandit['special_assignments'] and not [v for v,b in mobiles.items() if int(b.split('~',4)[4].strip().split()[0])&32768]
assert any(s[1]==67262 and s[2]==14813 for s in stock) and 'mace' in inventory_items[67262]['name'].lower()
quest=(ROOT/'src/world/quest.c').read_text();db=(ROOT/'src/world/db.c').read_text();moves=(ROOT/'src/cmd/actmove.c').read_text()
durable=quest[quest.index('static bool submit_durable_quest_offering('):quest.index('void tell_quest(')]
assert 'used = used || roots[index] == item' in durable and 'goal->goal_type != QUEST_GOAL_ITEM' in durable
assert 'completion = completion->next, ++completion_index' in durable
ambient=quest[quest.index('bool execute_quest_routine('):quest.index('bool execute_quest_routine(')+1200]
assert 'cmd == CMD_NONE' in ambient and 'QC_ACTION' in ambient and 'TO_ROOM' in ambient and 'record_completion' not in ambient
assert 'zone_story_quest_runtime::encountered' in quest
guard=db[db.index('static bool reset_command_issues_item('):db.index('/* execute the reset command table')]
assert all("case '"+c+"':" in guard for c in 'AOPGE') and 'reset_command_issues_item(ZCMD.command)' in db
assert re.search(r'economic_gameplay_authority::active\(\)\s*&&\s*reset_command_issues_item\(ZCMD.command\)\)\s*\{\s*last_cmd = 0;\s*continue;',db)
assert re.search(r'state\s*&=\s*3;',db) and 'ZCMD.arg3 & 0x04' in db and 'EX_SECRET' in db
unlock=moves[moves.index('void do_unlock('):moves.index('void do_pick(')]
assert unlock.index('REMOVE_BIT(obj->value[1], CONT_LOCKED)')<unlock.index('break_key(ch, key_obj)') and 'number(0, 99) < key_obj->value[1]' in unlock
capture=(ROOT/'src/player/player_snapshot_capture.c').read_text()
assert 'ownership.state == item_custody_state::active' in capture and 'omit_norent && IS_SET(object->extra_flags, ITEM_NORENT) && !active_durable_custody' in capture and 'outcome=durable_norent_included' in capture
assert 'halfling' in objects[14822].lower() and 'serpentine dagger' in objects[14822].lower()
assert 'goblin' in inventory_mobs[93001]['keywords']
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==148]
assert len(units)==4 and all(u['achievement'] and u['daily_candidate'] for u in units)
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[s['summary'] for s in mapping['stories']]).lower()
for phrase in ('locked','chance','supplied','two slave groups','diary','auction','active, ready accounting'):assert phrase in guidance,phrase


# Maze of Undead Army: exact choices, five-item returns, keyed access and availability.
maze=inventory_module.area_evidence(ROOT,'maze_are')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='maze_are')
blocks={r['block']['receive'][0][1]:r['block'] for r in maze['requests']}
expected={94005:(33,94018,[94002,94003,94004],True),94011:(46,94018,[94002,94004,94004],False),94013:(55,94018,[94002,94002,94003],False),94015:(64,94018,[94004,94004,94004],False),94017:(73,94018,[94003,94004,94004],False),94020:(96,94035,[94024]*5,False),94014:(113,94043,[94001]*5,True)}
assert set(blocks)==set(expected)
for reward,(line,giver,ingredients,departure) in expected.items():
    b=blocks[reward]
    assert (b['line'],b['giver_vnum'],b['disappear'])==(line,giver,departure)
    assert [tuple(x) for x in b['give']]==[('I',v) for v in ingredients] and [tuple(x) for x in b['receive']]==[('I',reward)]
    assert b['binding']['completion_key']=='give='+','.join('I:'+str(v) for v in sorted(ingredients))+';receive=I:'+str(reward)+';disappear='+str(int(departure))
assert all(q['definition']['repeatable'] and q['definition']['daily_eligible'] and not q['definition']['daily_exclusion'] for q in maze['requests'])
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
assert len(mapping['stories'])==7 and sum(len(s['steps']) for s in mapping['stories'])==21 and sum(t.get('optional',False) for s in mapping['stories'] for t in s['steps'])==14
for s in mapping['stories']:
    b=next(b for b in blocks.values() if s['contracts']==[b['binding']])
    quantities=collections.Counter(v for kind,v in b['give'])
    material=[t for t in s['steps'] if t['kind']=='carried_item' and t['item_vnums'][0] in quantities]
    assert [(t['item_vnums'][0],t['count'],t['optional']) for t in material]==[(v,n,True) for v,n in sorted(quantities.items())]
    assert len([t for t in s['steps'] if t['kind']=='completion' and t['contracts']==[b['binding']]])==1
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={94017,94043,94018,94035,94020,94019,94015,94010,94039,94041}
assert len(maze['dialogue'])==4 and sum(len(c['topics']) for c in contacts.values())==9
for v,c in contacts.items():
    assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
    assert c['topics']==[w for d in maze['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert contacts[94043]['topics']==[] and contacts[94017]['topics']==['hi','hello','potion']
assert (maze['zone']['zone_number'],maze['zone']['first_vnum'],maze['zone']['last_vnum'],maze['zone']['reset_mode'])==(940,93953,94177,1)
rooms=dawndale_bodies('maze_are','wld');objects=dawndale_bodies('maze_are','obj');mobiles=dawndale_bodies('maze_are','mob')
assert (len(rooms),len(mobiles),len(objects))==(177,43,25) and sorted(rooms)==list(range(94001,94178))
edges={(v,int(m[1])):(int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)}
assert len(edges)==395 and edges[(94126,5)]==(7,94020,94137)
def maze_reachable(blocked=()):
    found={94177};pending=[94177]
    while pending:
        v=pending.pop()
        for (src,d),(state,key,dest) in edges.items():
            if src==v and (src,d) not in blocked and dest in rooms and dest not in found:
                found.add(dest);pending.append(dest)
    return found
reachable=maze_reachable()
assert set(rooms)-reachable=={94117,94118,94119,94121}
assert reachable-maze_reachable({(94126,5)})=={94120,94122,94136,94137,94138}
assert not [edge for edge,values in edges.items() if edge[0]==94118]
assert {(src,d) for (src,d),(_,_,dest) in edges.items() if dest==94118}=={(94117,3),(94119,6),(94121,7)}
resets=maze['reset_commands'];assert len(resets)==150 and collections.Counter(r['command'] for r in resets)=={'D':20,'O':9,'P':4,'M':99,'E':10,'G':7,'F':1}
assert [r['arguments'] for r in resets if r['command']=='O' and r['arguments'][1]==94001]==[[0,94001,5,v,100,0,0,0] for v in (94009,94013,94046,94062,94093)]
parent=room=None;stock=[]
for r in resets:
    args=r['arguments']
    if r['command'] in ('M','F'): parent,room=args[1],args[3]
    elif r['command']=='G':stock.append((args[1],parent,room,args[2],args[4]))
assert [s for s in stock if s[0] in (94002,94003,94004)]==[(94002,94020,94117,1,100),(94004,94019,94119,1,100),(94003,94015,94121,1,100)]
assert [s for s in stock if s[0]==94024]==[(94024,94010,94095,5,100),(94024,94017,94115,5,100),(94024,94039,94163,5,100),(94024,94041,94176,5,100)]
assert [r['arguments'] for r in resets if r['command']=='O' and r['arguments'][1]==94024]==[[0,94024,5,94141,100,0,0,0]]
assert [r['arguments'] for r in resets if r['command']=='F']==[[1,94018,1,94115,100,0,0,0]]
for v in (94017,94043,94035):
    assert [r['arguments'] for r in resets if r['command']=='M' and r['arguments'][1]==v]==[[0,v,1,94139 if v==94035 else 94115,100,0,0,0]]
assert [r['arguments'] for r in resets if r['command']=='D' and r['arguments'][1] in (94126,94137)]==[[0,94126,5,6,100,0,0,0],[0,94137,4,1,100,0,0,0]]
assert [r['arguments'] for r in resets if r['command']=='O' and r['arguments'][1] in (94023,94019)]==[[0,94023,1,94120,100,0,0,0],[0,94019,1,94132,100,0,0,0]]
assert [r['arguments'] for r in resets if r['command']=='P']==[[1,94021,1,94019,100,0,0,0],[1,94022,1,94019,100,0,0,0],[1,94006,2,94019,100,0,0,0],[1,94025,1,94019,100,0,0,0]]
dust=objvalues(objects[94001]);finger=objvalues(objects[94024]);gold=objvalues(objects[94020]);black=objvalues(objects[94023]);sarc=objvalues(objects[94019])
assert dust[0]==finger[0]==13 and dust[6]&4096 and not finger[6]&4096
assert all(v[7]&1 and v[8]&32768 for v in (dust,finger))
assert gold[0]==black[0]==18 and gold[12]==0 and black[12]==100
assert sarc[0]==15 and sarc[7]==0 and sarc[11:15]==[150,29,94023,150]
assert objvalues(objects[94025])[0]==12
for v in (94002,94003,94004):
    values=objvalues(objects[v]);assert values[0]==11 and values[7]&1 and values[8]&32768 and not values[6]&4096
assert not maze['special_assignments'] and not [v for v,b in mobiles.items() if int(b.split('~',4)[4].strip().split()[0])&32768]
assert all(not int(mobiles[v].split('~',4)[4].strip().split()[0])&2 for v in (94020,94019,94015))
quest=(ROOT/'src/world/quest.c').read_text();db=(ROOT/'src/world/db.c').read_text();moves=(ROOT/'src/cmd/actmove.c').read_text();search_source=(ROOT/'src/cmd/actobj.c').read_text()
durable=quest[quest.index('static bool submit_durable_quest_offering('):quest.index('void tell_quest(')]
assert 'completion = completion->next, ++completion_index' in durable and 'used = used || roots[index] == item' in durable and 'goal->goal_type != QUEST_GOAL_ITEM' in durable
assert 'qcp->next = quest_index[number_of_quests].quest_complete;' in quest
guard=db[db.index('static bool reset_command_issues_item('):db.index('/* execute the reset command table')]
assert all("case '"+c+"':" in guard for c in 'AOPGE') and 'reset_command_issues_item(ZCMD.command)' in db
assert re.search(r'economic_gameplay_authority::active\(\)\s*&&\s*reset_command_issues_item\(ZCMD.command\)\)\s*\{\s*last_cmd = 0;\s*continue;',db)
assert 'mob = last_mob = tmp_mob = last_mob_followable = NULL;' in db and 'add_follower(mob, last_mob_followable)' in db
assert 'EX_PICKPROOF' in db and 'REMOVE_BIT(k->extra_flags, ITEM_SECRET)' in search_source
unlock=moves[moves.index('void do_unlock('):moves.index('void do_pick(')]
assert unlock.index('REMOVE_BIT(obj->value[1], CONT_LOCKED)')<unlock.index('break_key(ch, key_obj)')
assert 'number(0, 99) < key_obj->value[1]' in unlock and 'EX_SECRET' in unlock
assert 'if (!committed)' in moves and 'Your key cracks, but remains intact.' in moves and 'publish_key_break, economic_source_kind::intentional_destruction' in moves
epic=(ROOT/'src/classes/epic_skills.c').read_text();boot=(ROOT/'src/net/comm.c').read_text();world_epic=(ROOT/'src/world/epic.c').read_text();alchemy=(ROOT/'src/classes/npc_alchemist.c').read_text()
assert '{ 94017, SKILL_SPELLBIND, 0, 100, 0, SKILL_ENCHANT, 0 }' in epic
assert re.search(r'^\s*//\s*\{EPIC_REWARD_SKILL, SKILL_SPELLBIND,',epic,re.M)
teacher=epic[epic.index('int epic_teacher('):]
assert teacher.index('if (pReward == NULL)')<teacher.index('coins_cost =') and 'economic_gameplay_authority::active()' in teacher
assert 'epic_initialization();' in boot and 'func.mob = epic_teacher;' in world_epic
spawn=alchemy[alchemy.index('void npc_alchemist_world_spawn('):]
assert spawn.index('economic_gameplay_authority::active()')<spawn.index('read_object(VOBJ_POISON_VIALS')
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==940]
assert len(units)==7 and all(u['achievement'] and u['daily_candidate'] for u in units)
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[s['summary'] for s in mapping['stories']]).lower()
for phrase in ('search','five','supplied','unavailable','only the three','spellbind','black key','active, ready accounting'):assert phrase in guidance,phrase


# Vargan: hidden exact quantities, unsupported mixed fee and retained history.
vargan=inventory_module.area_evidence(ROOT,'vargan')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='vargan')
blocks=[r['block'] for r in vargan['requests']]
assert len(blocks)==1 and (blocks[0]['line'],blocks[0]['giver_vnum'],blocks[0]['disappear'])==(26,2104,False)
assert [tuple(x) for x in blocks[0]['give']]==[('I',2126),('I',2126),('I',2127),('C',40000)] and [tuple(x) for x in blocks[0]['receive']]==[('I',2128)]
assert blocks[0]['binding']['completion_key']=='give=C:40000,I:2126,I:2126,I:2127;receive=I:2128;disappear=0'
assert mapping['stories'][0]['contracts']==[blocks[0]['binding']] and len(mapping['stories'])==1
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
assert mapping['stories'][0]['id']=='norkons-stolen-armor' and len(mapping['stories'][0]['steps'])==3
assert [(s['item_vnums'],s['count'],s['optional']) for s in mapping['stories'][0]['steps'][:-1]]==[([2126],2,True),([2127],1,True)]
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert len(contacts)==11 and len(vargan['dialogue'])==5
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for d in vargan['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert sum(len(c['topics']) for c in contacts.values())==6 and 'armor, betrayed, pieces and weld' in contacts[2104]['description']
assert (vargan['zone']['zone_number'],vargan['zone']['first_vnum'],vargan['zone']['last_vnum'],vargan['zone']['reset_mode'])==(21,2089,2193,2)
definition=vargan['requests'][0]['definition'];assert definition['repeatable'] and not definition['daily_eligible'] and definition['daily_exclusion']=='Unsupported durable offering'
rooms=dawndale_bodies('vargan','wld');objects=dawndale_bodies('vargan','obj');mobiles=dawndale_bodies('vargan','mob')
assert (len(rooms),len(mobiles),len(objects))==(94,11,30) and sorted(rooms)==list(range(2100,2194))
assert len(re.findall(r'\bD\d+\s+[^~]*~[^~]*~\s*-?\d+\s+-?\d+\s+-?\d+',''.join(rooms.values()),re.S))==239
assert collections.Counter(r['command'] for r in vargan['reset_commands'])=={'D':4,'O':5,'P':1,'M':35,'G':8,'E':17}
assert [r['arguments'] for r in vargan['reset_commands'] if r['command']=='O' and r['arguments'][1] in (2126,2127)]==[[0,2126,2,2127,100,0,0,0],[0,2127,1,2152,100,0,0,0],[0,2126,2,2160,100,0,0,0]]
assert [r['arguments'] for r in vargan['reset_commands'] if r['command']=='M' and r['arguments'][1]==2104]==[[0,2104,1,2163,100,0,0,0]]
for v in (2126,2127):
 values=objvalues(objects[v]);assert values[0]==13 and values[6]&4096 and values[7]&1 and values[8]&32768
forge=objvalues(objects[2100]);assert forge[0]==15 and forge[7]==0 and forge[11:15]==[0,5,0,100]
assert [r['arguments'] for r in vargan['reset_commands'] if r['command']=='P']==[[1,2101,1,2100,100,0,0,0]]
assert objvalues(objects[2101])[0]==0 and objvalues(objects[2103])[0]==13 and objvalues(objects[2103])[7]==0
edges={(v,int(m[1])):(int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)}
for v,d,target in ((2127,3,2128),(2128,1,2127),(2151,1,2152),(2152,3,2151)):assert edges[(v,d)]==(5,0,target)
assert all(r['arguments'][3]==0 for r in vargan['reset_commands'] if r['command']=='D')
for v,d,target in ((2112,5,2115),(2115,5,2116),(2116,0,2117),(2182,5,7800)):assert edges[(v,d)][2]==target
for v in (2115,2116):assert int(rooms[v].split('~',2)[2].strip().splitlines()[0].split()[1])&8192
assert all(not re.search(r'^\s*[CF]\s+\d',b,re.M) for b in rooms.values())
assert 'south' in rooms[2107].split('D0',1)[1].split('~',1)[0] and (2135,3) in edges and (2135,1) not in edges
assert 'DK 0 4' in mobiles[2107]
assert not vargan['special_assignments'] and not [v for v,b in mobiles.items() if int(b.split('~',4)[4].strip().split()[0])&32768]
quest=(ROOT/'src/world/quest.c').read_text();objects_source=(ROOT/'src/cmd/actobj.c').read_text();visibility=(ROOT/'src/core/utility.c').read_text();defs=(ROOT/'src/core/defines.h').read_text();utils=(ROOT/'src/core/utils.h').read_text()
durable=quest[quest.index('static bool submit_durable_quest_offering('):quest.index('void tell_quest(')]
assert 'goal->goal_type != QUEST_GOAL_ITEM' in durable and 'used = used || roots[index] == item' in durable and 'if (!supported || !count)' in durable
assert 'This quest cannot accept offerings right now.' in quest and 'GET_MONEY(mob) < gp->number' in quest and 'SUB_MONEY(mob, gp->number, 0)' in quest
search=objects_source[objects_source.index('void do_search('):objects_source.index('void do_apply_poison(')]
assert 'REMOVE_BIT(k->extra_flags, ITEM_SECRET)' in search and 'CAN_SEE_OBJ(ch, k)' in search and 'SET_BIT(k->extra_flags, ITEM_SECRET)' in search and '(!found_something || IS_TRUSTED(ch))' in search
assert 'if (IS_SET((obj)->extra_flags, ITEM_SECRET))' in visibility and 'GET_PLATINUM(ch) * 1000' in visibility
assert '#define ITEM_SECRET BIT_13' in defs and '#define CONT_CLOSED BIT_3' in defs and '#define RACE_DRAGONKIN 56' in defs
assert '#define IS_BEHOLDER(ch) ((GET_RACE(ch) == RACE_BEHOLDER))' in utils
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==21];assert len(units)==1 and units[0]['achievement'] and not units[0]['daily_candidate']
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[s['summary'] for s in mapping['stories']]).lower()
for phrase in ('search','two','supplied','40 platinum','unavailable','restock','loose','active, ready accounting'):assert phrase in guidance,phrase


# Purple Worm: repeated quantities, nested source, actual route and spawn availability.
pworm=inventory_module.area_evidence(ROOT,'pworm')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='pworm')
blocks=[r['block'] for r in pworm['requests']]
assert len(blocks)==1 and (blocks[0]['line'],blocks[0]['giver_vnum'],blocks[0]['disappear'])==(31,42506,False)
assert [tuple(x) for x in blocks[0]['give']]==[('I',42502)]+[('I',42500)]*7 and [tuple(x) for x in blocks[0]['receive']]==[('I',42504)]
assert blocks[0]['binding']['completion_key']=='give=I:42500,I:42500,I:42500,I:42500,I:42500,I:42500,I:42500,I:42502;receive=I:42504;disappear=0'
assert mapping['stories'][0]['contracts']==[blocks[0]['binding']] and len(mapping['stories'])==1
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
assert mapping['stories'][0]['id']=='family-amulet-and-seven-hides' and len(mapping['stories'][0]['steps'])==3
assert [(s['item_vnums'],s['count'],s['optional']) for s in mapping['stories'][0]['steps'][:-1]]==[([42502],1,True),([42500],7,True)]
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert len(contacts)==11 and len(pworm['dialogue'])==5
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for d in pworm['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert sum(len(c['topics']) for c in contacts.values())==13
assert (pworm['zone']['zone_number'],pworm['zone']['first_vnum'],pworm['zone']['last_vnum'],pworm['zone']['reset_mode'])==(425,42375,42598,1)
assert [(r['definition']['repeatable'],r['definition']['daily_eligible']) for r in pworm['requests']]==[(True,True)]
rooms=dawndale_bodies('pworm','wld');objects=dawndale_bodies('pworm','obj');mobiles=dawndale_bodies('pworm','mob')
assert (len(rooms),len(mobiles),len(objects))==(98,11,15) and sorted(rooms)==[v for v in range(42500,42599) if v!=42597]
assert len(re.findall(r'\bD\d+\s+[^~]*~[^~]*~\s*-?\d+\s+-?\d+\s+-?\d+',''.join(rooms.values()),re.S))==216
assert collections.Counter(r['command'] for r in pworm['reset_commands'])=={'D':6,'M':62,'G':9,'P':2,'E':9}
parents={};last=None
for r in pworm['reset_commands']:
 if r['command'] in ('M','F','R'):last=r
 if r['command'] in ('G','E'):parents[r['line']]=last
hides=[r for r in pworm['reset_commands'] if r['command']=='G' and r['arguments'][1]==42500]
assert len(hides)==7 and all(r['arguments']==[1,42500,7,0,100,0,0,0] for r in hides)
assert sorted(parents[r['line']]['arguments'][3] for r in hides)==[42536,42537,42589,42590,42591,42592,42593]
assert all(parents[r['line']]['arguments'][1:3]==[42505,7] for r in hides)
corpse=next(r for r in pworm['reset_commands'] if r['command']=='G' and r['arguments'][1]==42503)
assert corpse['arguments']==[1,42503,1,0,100,0,0,0] and parents[corpse['line']]['arguments']==[0,42505,7,42593,100,0,0,0]
assert [r['arguments'] for r in pworm['reset_commands'] if r['command']=='P']==[[1,42502,1,42503,100,0,0,0],[1,42515,1,42503,100,0,0,0]]
assert objvalues(objects[42503])[11:19]==[0,0,0,10000,0,0,0,0] and objvalues(objects[42503])[0]==15
assert int(objects[42503].split('~',4)[4].strip().splitlines()[0].split()[3])==0
assert [(r['arguments'][1],r['arguments'][3],r['arguments'][4]) for r in pworm['reset_commands'] if r['command']=='M' and r['arguments'][1] in (42506,42508,42509)]==[(42506,42594,65),(42508,42594,65),(42509,42594,65)]
edges={(v,int(m[1])):(int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)}
assert edges[(42551,8)][2]==42589 and (42551,0) not in edges
for v,d,target in ((42589,1,42536),(42536,1,42537),(42537,9,42590),(42590,9,42591),(42591,2,42592),(42592,2,42593),(42593,7,42554)):assert edges[(v,d)][2]==target
assert not any(v==42589 and target[2]==42551 for (v,d),target in edges.items()) and not any(v==42595 for v,d in edges)
assert [(r['arguments'][1],r['arguments'][2],r['arguments'][3]) for r in pworm['reset_commands'] if r['command']=='D']==[(42500,0,1),(42501,2,1),(42510,7,1),(42511,8,1),(42563,9,1),(42564,6,1)]
assert edges[(42510,7)][:2]==(5,0) and edges[(42563,9)][:2]==(5,0)
assert not pworm['special_assignments'] and not [v for v,b in mobiles.items() if int(b.split('~',4)[4].strip().split()[0])&32768]
db=(ROOT/'src/world/db.c').read_text();events=(ROOT/'src/world/events.c').read_text();boot=(ROOT/'src/world/new_events.c').read_text();defs=(ROOT/'src/core/defines.h').read_text()
branch=db[db.index("case 'M': /* read a mobile */"):db.index("case 'O': /* load an object to room */")]
assert 'ZCMD.arg4 == 100' in branch and 'force_item_repop' in branch and 'ZCMD.arg4 > number(0, 99)' in branch and 'char_to_room(mob, ZCMD.arg3, -2)' in branch
assert 'reset_zone(zone, 0);' in events and '::is_empty(zone)' in events and 'reset_zone(j, 2);' in boot
assert '#define DIR_NORTHEAST 8' in defs and '#define DIR_SOUTHEAST 9' in defs
assign=(ROOT/'src/specs/specs.assign.c').read_text();combat=(ROOT/'src/combat/mobcombat.c').read_text();mobact=(ROOT/'src/mob/mobact.c').read_text()
assert sorted(int(v) for v in re.findall(r'mob_index\[real_mobile0\((\d+)\)\]\.func\.mob = purple_worm;',assign))==[4480,131232,700004]
racial=combat[combat.index('void PwormCombat('):combat.index('int GenMobCombat(')]
assert 'spell_corrosive_blast' in racial and 'char_to_room' not in racial and 'die(' not in racial
assert 'if (IS_PWORM(ch))' in mobact and 'PwormCombat(ch, victim);' in combat
assert len([r for r in inventory_module.area_evidence(ROOT,'alatorin')['reset_commands'] if r['command']=='M' and r['arguments'][1]==42501])==9
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==425];assert len(units)==1 and units[0]['achievement'] and units[0]['daily_candidate']
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[s['summary'] for s in mapping['stories']]).lower()
for phrase in ('seven','supplied','northeast','ordinary resets','mode1','loose','active, ready accounting'):assert phrase in guidance,phrase


# Prison of Fort Boyard: five exact preparations, shared access and separate touch/reset authority.
prisonb=inventory_module.area_evidence(ROOT,'prisonb')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='prisonb')
blocks=[r['block'] for r in prisonb['requests']]
assert len(blocks)==1 and (blocks[0]['line'],blocks[0]['giver_vnum'],blocks[0]['disappear'])==(43,43010,False)
assert blocks[0]['binding']['completion_key']=='give=I:43003,I:43006,I:43008,I:43011,I:43012;receive=I:43015;disappear=0'
assert mapping['stories'][0]['contracts']==[blocks[0]['binding']] and len(mapping['stories'])==1
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
assert mapping['stories'][0]['id']=='five-scales-for-cloak' and len(mapping['stories'][0]['steps'])==6
assert [(s['item_vnums'],s['count'],s['optional']) for s in mapping['stories'][0]['steps'][:-1]]==[([v],1,True) for v in (43003,43006,43008,43011,43012)]
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert len(contacts)==16 and len(prisonb['dialogue'])==5
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for d in prisonb['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert sum(len(c['topics']) for c in contacts.values())==9
assert (prisonb['zone']['zone_number'],prisonb['zone']['first_vnum'],prisonb['zone']['last_vnum'],prisonb['zone']['reset_mode'])==(430,43000,43051,0)
assert [(r['definition']['repeatable'],r['definition']['daily_eligible']) for r in prisonb['requests']]==[(True,True)]
rooms=dawndale_bodies('prisonb','wld');objects=dawndale_bodies('prisonb','obj');mobiles=dawndale_bodies('prisonb','mob')
assert (len(rooms),len(mobiles),len(objects))==(52,25,38) and sorted(rooms)==list(range(43000,43052))
assert len(re.findall(r'\bD\d+\s+[^~]*~[^~]*~\s*-?\d+\s+-?\d+\s+-?\d+',''.join(rooms.values()),re.S))==108
assert collections.Counter(r['command'] for r in prisonb['reset_commands'])=={'D':44,'O':15,'P':61,'M':44,'E':38,'G':10}
parents={};last=None
for r in prisonb['reset_commands']:
 if r['command'] in ('M','F','R'):last=r
 if r['command'] in ('G','E'):parents[r['line']]=last
for item,parent,room in ((43003,43001,43008),(43006,43003,43012),(43008,43005,43018),(43011,43006,43017),(43012,43007,43016),(43000,43000,43004),(43020,43020,43023)):
 assert any(r['command']=='G' and r['arguments'][1:3]==[item,1] and parents[r['line']]['arguments'][1]==parent and parents[r['line']]['arguments'][3]==room for r in prisonb['reset_commands'] if r['command'] in ('G','E'))
assert objvalues(objects[43009])[11:15]==[340,43016,0,0] and objvalues(objects[43010])[11:15]==[340,43014,0,0]
assert objvalues(objects[43014])[11:15]==[43020,7,-1,0] and objvalues(objects[43023])[11:15]==[38106,264,-1,0]
assert objvalues(objects[43024])[11:15]==[100,13,0,100]
assert '3 43000 43005' in rooms[43004] and '3 43020 43037' in rooms[43026]
assert '8 0 43015' in rooms[43014] and '8 0 43018' in rooms[43016]
assert (ROOT/'areas/zon/fortb.zon').read_text().count('O 0 43014 1 38106 100')==1
assert not prisonb['special_assignments'] and not [v for v,b in mobiles.items() if int(b.split('~',4)[4].strip().split()[0])&32768]
assign=(ROOT/'src/specs/specs.assign.c').read_text();db=(ROOT/'src/world/db.c').read_text();switch=(ROOT/'src/specs/specs.object.c').read_text();epic=(ROOT/'src/world/epic.c').read_text();interp=(ROOT/'src/cmd/interp.h').read_text()
assert 'obj_index[real_object0(359)].func.obj = epic_stone;' in assign
assert 'obj->type == ITEM_SWITCH && !obj_index[nr].func.obj' in db and 'obj_index[nr].func.obj = item_switch;' in db
assert '#define CMD_PULL 340' in interp and '#define CMD_JUMP 264' in interp and '#define CMD_ENTER 7' in interp
assert 'obj->value[0] != cmd' in switch and 'EX_BLOCKED' in switch and 'Nothing happens.' in switch
assert 'zone_touch_transaction_submit(touch)' in epic and 'touch.participant_pids[i]' in epic and '!zone_table[real_zone0(zone_number)].reset_mode' in epic
assert 'This blue dragon was captured' in mobiles[43006] and 'This green dragon was captured' in mobiles[43007]
assert objects[43031].split('~',4)[3].strip()=='' and list(re.finditer(r'\bD(\d+)',rooms[43051]))[0][1]=='2'
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==430];assert len(units)==1 and units[0]['achievement'] and units[0]['daily_candidate']
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[s['summary'] for s in mapping['stories']]).lower()
for phrase in ('together','supplied','pull','jump','reset mode0','epic','loose','active, ready accounting'):assert phrase in guidance,phrase


# Moonshae Island: suggested progression, alternate orb and effective legacy availability.
moonshae=inventory_module.area_evidence(ROOT,'moonshae')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='moonshae')
blocks=[r['block'] for r in moonshae['requests']]
assert [(b['line'],b['giver_vnum'],b['disappear']) for b in blocks]==[(18,26208,False),(43,26221,True)]
assert [b['binding']['completion_key'] for b in blocks]==['give=I:26233;receive=E:99000,I:26235;disappear=0','give=I:26212;receive=I:26209;disappear=1']
assert [s['contracts'] for s in mapping['stories']]==[[b['binding']] for b in blocks]
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
assert [s['id'] for s in mapping['stories']]==['return-lost-sword','moonwell-proof-for-brigit'] and [len(s['steps']) for s in mapping['stories']]==[2,3]
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert len(contacts)==12 and len(moonshae['dialogue'])==5
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for d in moonshae['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert sum(len(c['topics']) for c in contacts.values())==14
assert (moonshae['zone']['zone_number'],moonshae['zone']['first_vnum'],moonshae['zone']['last_vnum'],moonshae['zone']['reset_mode'])==(262,26135,26340,2)
assert [(r['definition']['repeatable'],r['definition']['daily_eligible']) for r in moonshae['requests']]==[(True,True),(True,True)]
rooms=dawndale_bodies('moonshae','wld');objects=dawndale_bodies('moonshae','obj');mobiles=dawndale_bodies('moonshae','mob')
assert (len(rooms),len(mobiles),len(objects))==(127,27,40)
assert len(re.findall(r'\bD\d+\s+[^~]*~[^~]*~\s*-?\d+\s+-?\d+\s+-?\d+',''.join(rooms.values()),re.S))==266
assert sorted(set(range(26200,26341))-set(rooms))==[26254,26255,26321,26322,26323,26324,26325,26326,26327,26328,26329,26330,26334,26335]
assert collections.Counter(r['command'] for r in moonshae['reset_commands'])=={'D':26,'O':4,'M':77,'G':18,'E':40}
assert len((ROOT/'areas/zon/moonshae.zon').read_text().splitlines())==254
parents={};last=None
for r in moonshae['reset_commands']:
 if r['command'] in ('M','F','R'):last=r
 if r['command'] in ('G','E'):parents[r['line']]=last
for item,parent,room,c in ((26212,26204,26297,'G'),(26233,26214,26270,'G'),(26209,26214,26270,'E')):
 assert any(r['command']==c and r['arguments'][1]==item and parents[r['line']]['arguments'][1]==parent and parents[r['line']]['arguments'][3]==room for r in moonshae['reset_commands'] if r['command'] in ('G','E'))
assert '5 26209 26287' in rooms[26258] and '3 26209 26258' in rooms[26287]
assert '2 -2 26231' in rooms[26227] and '2 -2 26250' in rooms[26249] and '2 -2 26311' in rooms[26310]
assert '0 0 26285' in rooms[26285] and 11100 not in inventory_items and inventory_items[11300]['source']=='areas/obj/otower.obj'
assert [(a['kind'],a['vnum'],a['function']) for a in moonshae['special_assignments']]==[('mob',v,'sister_knight') for v in range(26218,26223)]
assert not [v for v,b in mobiles.items() if int(b.split('~',4)[4].strip().split()[0])&65536]
assign=(ROOT/'src/specs/specs.assign.c').read_text();local=(ROOT/'src/specs/specs.moonshae.c').read_text();db=(ROOT/'src/world/db.c').read_text();speech=(ROOT/'src/cmd/actcomm.c').read_text();fishing=(ROOT/'src/economy/tradeskill.c').read_text()
assert '//  obj_index[real_object0(26233)].func.obj = cymric_hugh;' in assign
assert 'ticket_taker' not in assign and 'cc_fisherffolk' not in assign and 'cc_female_ffolk' not in assign
assert 'sister_knight, NULL, 0, 0' in local and 'shout_and_hunt(ch, 100' in local
assert 'if (EXIT(ch, door) && (EXIT(ch, door)->key == -2)' in speech and 'check_magic_doors(ch, argument + i);' in speech
assert 'world[room].dir_option[door] = NULL;' in db and "zone_table[zone].cmd[comm].command = '!';" in db
assert '(obj_index[ZCMD.arg1].number < ZCMD.arg2 && ZCMD.arg4 == 100)' in db and 'force_item_repop' in db
assert 'O 0 26233 1 66251 20' in (ROOT/'areas/zon/cityruin.zon').read_text()
assert 'virtual_number == 26200' in fishing and 'grant_tradeskill_item(ch, fish);' in fishing and 'economic_source_kind::crafting' in fishing
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==262];assert len(units)==2 and all(u['achievement'] and u['daily_candidate'] for u in units)
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[s['summary'] for s in mapping['stories']]).lower()
for phrase in ('independent','foreman','supplied','loose','departs','locked','fishing','active, ready accounting'):assert phrase in guidance,phrase


# Ixxillikor: exact legacy proof, unavailable purchase and computed training.
ixxillikor=inventory_module.area_evidence(ROOT,'ixxillikor')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='ixxillikor')
blocks=[r['block'] for r in ixxillikor['requests']]
assert [(b['line'],b['giver_vnum'],b['disappear']) for b in blocks]==[(10,4203,False),(47,4224,False)]
assert [b['binding']['completion_key'] for b in blocks]==['give=I:402,I:26614,I:32490;receive=I:408;disappear=0','give=C:10000;receive=I:35708;disappear=0']
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/ixxillikor.qst' and b['kind'] in ('Q','QA')]
assert [b['line'] for b in raw]==[10,42,47] and raw[1]['binding']==raw[2]['binding']
assert raw[1]['give']==[('C',10000)] and raw[1]['receive']==[('I',35708)]
assert 35708 not in inventory_items and all(v in inventory_items for v in (402,408,32490,26614))
assert [s['contracts'] for s in mapping['stories']]==[[b['binding']] for b in blocks]
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
assert [s['id'] for s in mapping['stories']]==['legacy-power-scroll','legacy-auction-purchase']
assert [len(s['steps']) for s in mapping['stories']]==[4,1] and all(t['kind']=='completion' for t in mapping['stories'][1]['steps'])
assert {t['item_vnums'][0] for t in mapping['stories'][0]['steps'] if t['kind']=='carried_item'}=={32490,26614,402}
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert len(contacts)==12 and len(ixxillikor['dialogue'])==5
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for d in ixxillikor['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert sum(len(c['topics']) for c in contacts.values())==7 and contacts[4203]['topics']==[] and contacts[4264]['topics']==[]
assert (ixxillikor['zone']['zone_number'],ixxillikor['zone']['first_vnum'],ixxillikor['zone']['last_vnum'],ixxillikor['zone']['reset_mode'])==(42,4154,4398,2)
assert [(r['definition']['repeatable'],r['definition']['daily_eligible']) for r in ixxillikor['requests']]==[(True,True),(True,False)]
rooms=dawndale_bodies('ixxillikor','wld');objects=dawndale_bodies('ixxillikor','obj');mobiles=dawndale_bodies('ixxillikor','mob')
assert (len(rooms),len(mobiles),len(objects))==(199,69,28)
assert set(rooms)==set(range(4200,4399)) and set(objects)==set(range(4200,4228)) and set(mobiles)==set(range(4200,4269))
assert len(re.findall(r'\bD\d+\s+[^~]*~[^~]*~\s*-?\d+\s+-?\d+\s+-?\d+',''.join(rooms.values()),re.S))==456
assert collections.Counter(r['command'] for r in ixxillikor['reset_commands'])=={'D':30,'O':7,'M':194,'E':3,'G':16,'F':2}
assert len((ROOT/'areas/zon/ixxillikor.zon').read_text().splitlines())==353
parents={};last=None
for r in ixxillikor['reset_commands']:
 if r['command'] in ('M','F','R'):last=r
 if r['command'] in ('G','E'):parents[r['line']]=last
assert any(r['command']=='G' and r['arguments'][1]==4226 and parents[r['line']]['arguments'][1]==4257 and parents[r['line']]['arguments'][3]==4225 for r in ixxillikor['reset_commands'] if r['command'] in ('G','E'))
assert '2 4226 4226' in rooms[4225] and '1 0 4225' in rooms[4226]
assert any(r['command']=='M' and r['arguments'][1]==4208 and r['arguments'][3]==4332 for r in ixxillikor['reset_commands'])
assert any(r['command']=='F' and r['arguments'][1]==4254 and r['arguments'][3]==4332 for r in ixxillikor['reset_commands'])
assert not [r for r in ixxillikor['reset_commands'] if r['command'] in ('M','F') and r['arguments'][1] in (4246,4247,4248)]
assert not ixxillikor['special_assignments'] and not [v for v,b in mobiles.items() if int(b.split('~',4)[4].strip().split()[0])&65536]
assert all(objvalues(b)[0] not in (25,29) for b in objects.values())
scroll=dawndale_bodies('heavens','obj')[408];assert objvalues(scroll)[0]==13 and objvalues(scroll)[11:19]==[0]*8
quest=(ROOT/'src/world/quest.c').read_text();epic=(ROOT/'src/world/epic.c').read_text();skills=(ROOT/'src/classes/epic_skills.c').read_text();db=(ROOT/'src/world/db.c').read_text();actions=(ROOT/'src/cmd/actoth.c').read_text()
legacy_give=quest[quest.index('int quester('):] if 'int quester(' in quest else quest[quest.index('quester(P_char'):]
assert legacy_give.index('if (economic_gameplay_authority::active())') < legacy_give.index('do_give(pl, arg, -4)')
assert 'This quest cannot accept offerings right now.' in legacy_give
assert 'mob_index[quest_index[count].quester].qst_func = quester;' in quest
assert 'mob_index[real_mobile(epic_teachers[i].vnum)].func.mob = epic_teacher;' in epic
assert '{ 4203, SKILL_EPIC_POWER,' in skills and '{ 4208, SKILL_SPATIAL_FOCUS,' in skills
assert 'Epic skill purchases are unavailable while economic accounting is active.' in skills
assert 'state &=\n\t\t3;' in db and 'if (state == 2)' in db and 'exit_info = EX_ISDOOR;' in db
assert 'if (scroll->type != ITEM_SCROLL)' in actions
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==42];assert len(units)==2 and all(u['achievement'] for u in units) and sum(u['daily_candidate'] for u in units)==1
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[s['summary'] for s in mapping['stories']]).lower()
for phrase in ('currently unavailable','historical','supplied','loose','tablet','no_mob','ambient','active, ready accounting'):assert phrase in guidance,phrase


# Apocalypse Castle: distinct skulls, shared access and departing story-only return.
apocalypse=inventory_module.area_evidence(ROOT,'4horse')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='4horse')
blocks=sorted((r['block'] for r in apocalypse['requests']),key=lambda b:b['line'])
assert [(b['line'],b['giver_vnum'],b['disappear']) for b in blocks]==[(27,34502,False),(52,34503,True)]
assert [b['give'] for b in blocks]==[[('I',34548),('I',34549),('I',34550),('I',34551)],[('I',34505)]]
assert [b['receive'] for b in blocks]==[[('I',34552)],[('I',34527)]]
assert [s['contracts'] for s in mapping['stories']]==[[b['binding']] for b in blocks]
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
assert [s['id'] for s in mapping['stories']]==['four-horsemen-proof','lost-diamond-bracelet']
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert len(contacts)==12 and len(apocalypse['dialogue'])==5
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for d in apocalypse['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert sum(len(c['topics']) for c in contacts.values())==7
assert (apocalypse['zone']['zone_number'],apocalypse['zone']['first_vnum'],apocalypse['zone']['last_vnum'],apocalypse['zone']['reset_mode'])==(345,34500,34811,0)
assert [(r['definition']['repeatable'],r['definition']['daily_eligible']) for r in apocalypse['requests']]==[(True,True),(False,False)]
rooms=dawndale_bodies('4horse','wld');objects=dawndale_bodies('4horse','obj');mobiles=dawndale_bodies('4horse','mob')
assert (len(rooms),len(mobiles),len(objects))==(312,71,72)
assert set(rooms)==set(range(34500,34812)) and set(objects)==set(range(34500,34572)) and set(mobiles)==set(range(34500,34571))
assert len(re.findall(r'\bD\d+\s+[^~]*~[^~]*~\s*-?\d+\s+-?\d+\s+-?\d+',''.join(rooms.values()),re.S))==777
assert collections.Counter(r['command'] for r in apocalypse['reset_commands'])=={'D':50,'O':18,'P':5,'M':247,'E':74,'G':14,'F':10}
assert len((ROOT/'areas/zon/4horse.zon').read_text().splitlines())==579
parents={};last=None
for r in apocalypse['reset_commands']:
 if r['command'] in ('M','F','R'):last=r
 if r['command'] in ('G','E'):parents[r['line']]=last
for item,parent,room in ((34548,34547,34799),(34549,34544,34791),(34550,34545,34783),(34551,34546,34775),(34571,34569,34562),(34502,34501,34502),(34535,34537,34643),(34536,34520,34634),(34542,34543,34654)):
 assert any(r['command']=='G' and r['arguments'][1]==item and parents[r['line']]['arguments'][1]==parent and parents[r['line']]['arguments'][3]==room for r in apocalypse['reset_commands'] if r['command'] in ('G','E')),(item,parent,room)
assert any(r['command']=='P' and r['arguments'][1]==34505 and r['arguments'][3]==34504 for r in apocalypse['reset_commands'])
assert any(r['command']=='O' and r['arguments'][1]==34504 and r['arguments'][3]==34564 for r in apocalypse['reset_commands'])
assert '7 34571 34564' in rooms[34562] and '3 34571 34562' in rooms[34564]
assert '3 34552 34804' in rooms[34503] and '3 34552 34503' in rooms[34804]
for v,values in ((34503,[34517,7,-1,0]),(34521,[34503,181,-1,0]),(34569,[34573,270,-1,0]),(34570,[274,34568,0,0])):assert objvalues(objects[v])[11:15]==values
assert objvalues(objects[34570])[0]==29 and objvalues(objects[34570])[6]&2
assert '9 0 34644' in rooms[34568]
assert [(a['kind'],a['vnum'],a['function']) for a in apocalypse['special_assignments']]==[('obj',34545,'mankiller'),('obj',34559,'brainripper')]
assert not [v for v,b in mobiles.items() if int(b.split('~',4)[4].strip().split()[0])&65536]
assert set(r['arguments'][1] for r in apocalypse['reset_commands'] if r['command'] in ('O','P','G','E'))-set(objects)=={360,51401}
assign=(ROOT/'src/specs/specs.assign.c').read_text();assert 'obj_index[real_object0(360)].func.obj = epic_stone;' in assign
assert 'skill_beacon' not in assign and '{ 34804,' in (ROOT/'src/specs/specs.character_progression.c').read_text()
travel=(ROOT/'src/magic/spell_travel.c').read_text();switch=(ROOT/'src/specs/specs.object.c').read_text();epic=(ROOT/'src/world/epic.c').read_text()
assert 'obj->value[1] != cmd' in travel and 'obj->value[0] != cmd' in switch and 'if (cmd == CMD_TOUCH && IS_PC(ch))' in epic
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==345];assert len(units)==2 and all(u['achievement'] for u in units) and sum(u['daily_candidate'] for u in units)==1
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[s['summary'] for s in mapping['stories']]).lower()
for phrase in ('enter','dream','push','shove','supplied','loose','daily','departs','active, ready accounting'):assert phrase in guidance,phrase


# Shady Grove: preserved returns, exact source parentage and contextual services.
shady=inventory_module.area_evidence(ROOT,'shady')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='shady')
blocks=sorted((r['block'] for r in shady['requests']),key=lambda b:b['line'])
assert [(b['line'],b['giver_vnum'],b['disappear']) for b in blocks]==[(11,97510,False),(36,97545,True),(61,97548,False)]
assert [b['give'] for b in blocks]==[[('I',97514)],[('I',97582)],[('I',97572)]]
assert [b['receive'] for b in blocks]==[[('I',97570),('C',10000)],[('E',2100)],[('E',5200)]]
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,2,'complete') and not mapping['exclusions']
assert [s['id'] for s in mapping['stories']]==['request-97510-4963db257809','request-97545-85d3fa59aeaf','request-97548-13949b297677']
assert [s['contracts'] for s in mapping['stories']]==[[b['binding']] for b in blocks] and all(s['category']=='request' for s in mapping['stories'])
for s,b in zip(mapping['stories'],blocks):
 assert [t['id'] for t in s['steps']]==['item-1','turn-in']
 assert [(t['item_vnums'],t['count'],t['optional']) for t in s['steps'][:-1]]==[([b['give'][0][1]],1,True)]
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert len(contacts)==14 and len(shady['dialogue'])==5
assert [contacts[v]['keyword'] for v in (97510,97537,97545,97548)]==['high','old','troll','lyren']
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for d in shady['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert sum(len(c['topics']) for c in contacts.values())==11
assert (shady['zone']['zone_number'],shady['zone']['first_vnum'],shady['zone']['last_vnum'],shady['zone']['reset_mode'])==(975,97424,97758,2)
rooms=dawndale_bodies('shady','wld');objects=dawndale_bodies('shady','obj');mobiles=dawndale_bodies('shady','mob')
assert (len(rooms),len(mobiles),len(objects))==(258,106,90)
assert set(objects)==set(range(97500,97590)) and set(mobiles)==set(range(97500,97606))
assert len(re.findall(r'\bD\d+\s+[^~]*~[^~]*~\s*-?\d+\s+-?\d+\s+-?\d+',''.join(rooms.values()),re.S))==561
assert collections.Counter(r['command'] for r in shady['reset_commands'])=={'D':84,'O':31,'M':357,'E':234,'G':119,'F':28,'R':8}
reset=(ROOT/'areas/zon/shady.zon').read_text().splitlines();assert len(reset)==1125 and reset[-1]=='S'
parents={};last=None
for r in shady['reset_commands']:
 if r['command'] in ('M','F','R'):last=r
 if r['command'] in ('G','E'):parents[r['line']]=last
selected=[(r,parents[r['line']]) for r in shady['reset_commands'] if r['command'] in ('G','E') and r['arguments'][1] in (97514,97582,97572,97538,97535,97564,97570)]
for item,command,parent,room in ((97514,'E',97511,97502),(97582,'G',97557,97729),(97572,'E',97572,97700),(97538,'G',97500,97523),(97535,'G',97511,97502),(97564,'E',97552,97673),(97570,'E',97605,97703)):
 assert any(r['command']==command and r['arguments'][1]==item and p['arguments'][1]==parent and p['arguments'][3]==room for r,p in selected),(item,command,parent,room)
assert [r['arguments'][3] for r in shady['reset_commands'] if r['command']=='M' and r['arguments'][1]==97545]==[97580]
assert '2 97538 97523' in rooms[97522] and '2 97538 97522' in rooms[97523]
assert '2 97535 97503' in rooms[97502] and '2 97535 97502' in rooms[97503]
assert '3 97564 97726' in rooms[97673] and '1 0 97673' in rooms[97726]
assert [(a['kind'],a['vnum'],a['function']) for a in shady['special_assignments']]==[('mob',97545,'troll_slave'),('mob',97509,'stray_dog'),('mob',97534,'hardworking_fisherman'),('mob',97554,'orcish_jailkeeper'),('mob',97539,'orcish_woman'),('mob',97540,'world_quest'),('mob',97540,'world_quest'),('room',97757,'pet_shops'),('room',97663,'inn')]
assert not [v for v,b in mobiles.items() if int(b.split('~',4)[4].strip().split()[0])&65536]
for v,keyword in ((97509,'skunk'),(97529,'dog'),(97534,'baker'),(97532,'fisherman'),(97554,'guard'),(97552,'jailkeeper'),(97539,'mage'),(97537,'woman')):assert keyword in inventory_mobs[v]['keywords']
assert not any(z['source_area']=='shady2' for z in catalog_module.zone_registry(ROOT))
missing={r['arguments'][1] for r in shady['reset_commands'] if r['command'] in ('G','E','O','P') and r['arguments'][1] not in inventory_items};assert missing=={6070,6109,6110}
shops=(ROOT/'areas/shp/shady.shp').read_text();assert len(re.findall(r'^#\d+~',shops,re.M))==12
bakery=re.search(r'^#97534~([\s\S]*?)(?=^#\d+~|\Z)',shops,re.M)[1]
assert all('\n'+str(v)+'\n' in bakery for v in missing)
shop=(ROOT/'src/economy/shop.c').read_text();db=(ROOT/'src/world/db.c').read_text();mail=(ROOT/'src/cmd/mail.c').read_text();pet=(ROOT/'src/specs/specs.room.c').read_text()
assert 'Shop trades and services are unavailable while economic accounting is active.' in shop and 'SHOP_FUNC(shop) = mob_index[keeper].func.mob;' in shop
assert "zone_table[zone].cmd[comm].command = '!';" in db and 'IS_ACT(mob, ACT_TEACHER) && !mob_index[nr].func.mob' in db
postmaster=re.sub(r'/\*[\s\S]*?\*/','',mail.split('int postmaster[',1)[1].split('};',1)[0]);assert '97583' in postmaster
assert 'int pet_shops(' in pet and 'int gc_portal(' in (ROOT/'src/specs/specs.heavens.c').read_text()
native_prose=inventory_module.plain(' '.join(blocks[0]['body']));assert '1000 gold coins' in native_prose
outside=[b for b in inventory_module.native_blocks(ROOT) if b.get('giver_vnum')==83302 and b['kind']=='QA' and ('I',97549) in b['give']]
assert len(outside)==1 and outside[0]['receive']==[('C',35000)] and not outside[0]['disappear']
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[s['summary'] for s in mapping['stories']]).lower()
for phrase in ('supplied','need not','loose','child','accepts','cell key','daily','currently unavailable'):assert phrase in guidance,phrase
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==975];assert len(units)==3 and all(u['achievement'] and u['daily_candidate'] for u in units)


# Kimordril: preserved errands, lookalike potato, shared stock and effective services.
kimordril=inventory_module.area_evidence(ROOT,'kimordril')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='kimordril')
blocks=sorted((r['block'] for r in kimordril['requests']),key=lambda b:b['line'])
assert [(b['line'],b['giver_vnum'],b['disappear']) for b in blocks]==[(20,95508,True),(44,95512,False),(49,95512,False),(54,95512,False)]
assert [b['give'] for b in blocks]==[[('I',95506),('I',95508),('I',95507)],[('I',95512)],[('I',95513)],[('I',95514)]]
assert [b['receive'] for b in blocks]==[[('C',60),('E',100)],[('C',30)],[('C',35)],[('C',20)]]
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,2,'complete') and not mapping['exclusions']
assert [s['id'] for s in mapping['stories']]==['request-95508-58bfef7fc357','request-95512-17faf4eac94d','request-95512-6d0c1a6c969d','request-95512-64ea923287af']
assert [s['contracts'] for s in mapping['stories']]==[[b['binding']] for b in blocks] and all(s['category']=='request' for s in mapping['stories'])
assert [len(s['steps']) for s in mapping['stories']]==[4,2,2,2]
for s,b in zip(mapping['stories'],blocks):
 assert [(t['item_vnums'],t['count'],t['optional']) for t in s['steps'][:-1]]==[([v],n,True) for v,n in sorted(collections.Counter(v for k,v in b['give']).items())]
 assert [t['id'] for t in s['steps']]==['item-'+str(i) for i in range(1,len(s['steps']))]+['turn-in']
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert len(contacts)==14 and len(kimordril['dialogue'])==5
assert contacts[95508]['keyword']=='busy' and contacts[95512]['keyword']=='dirty'
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for d in kimordril['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert sum(len(c['topics']) for c in contacts.values())==14
assert (kimordril['zone']['zone_number'],kimordril['zone']['first_vnum'],kimordril['zone']['last_vnum'],kimordril['zone']['reset_mode'])==(955,95431,95658,2)
rooms=dawndale_bodies('kimordril','wld');objects=dawndale_bodies('kimordril','obj');mobiles=dawndale_bodies('kimordril','mob')
assert set(rooms)==set(range(95500,95659)) and set(objects)==set(range(95500,95549)) and set(mobiles)==set(range(95500,95560))
assert len(re.findall(r'\bD\d+\s+[^~]*~[^~]*~\s*-?\d+\s+-?\d+\s+-?\d+',''.join(rooms.values()),re.S))==351
assert collections.Counter(r['command'] for r in kimordril['reset_commands'])=={'D':38,'O':15,'P':15,'M':126,'E':42,'R':4,'F':4,'G':53}
reset=(ROOT/'areas/zon/kimordril.zon').read_text().splitlines();assert len(reset)==422 and reset[-1]=='S'
assert sum(r['command']=='P' and r['arguments'][1]==95506 and r['arguments'][3]==95504 for r in kimordril['reset_commands'])==3
assert [r['arguments'][3] for r in kimordril['reset_commands'] if r['command']=='M' and r['arguments'][1]==95508]==[95625,95639]
assert objects[95506].split('~')[:2]==objects[95510].split('~')[:2] and objvalues(objects[95506])[8]!=objvalues(objects[95510])[8]
assert objvalues(objects[95500])[0]==29 and objvalues(objects[95500])[11:15]==[270,95574,2,0] and objvalues(objects[95501])[11:15]==[270,95598,0,0]
assert '8 0 95598' in rooms[95574] and '8 0 95574' in rooms[95598] and '3 95548 95644' in rooms[95643] and '3 95548 95643' in rooms[95644]
assert [(a['kind'],a['vnum'],a['function']) for a in kimordril['special_assignments']]==[('mob',95517,'world_quest'),('mob',95506,'archer'),('mob',95503,'money_changer'),('mob',95535,'kimordril_shout'),('room',95569,'inn')]
assert [v for v,b in mobiles.items() if int(b.split('~',4)[4].strip().split()[0])&65536]==[95535]
shop=(ROOT/'src/economy/shop.c').read_text();db=(ROOT/'src/world/db.c').read_text();mail=(ROOT/'src/cmd/mail.c').read_text();general=(ROOT/'src/specs/specs.kimordril.c').read_text()
assert 'Shop trades and services are unavailable while economic accounting is active.' in shop and 'SHOP_FUNC(shop) = mob_index[keeper].func.mob;' in shop
assert 'IS_ACT(mob, ACT_TEACHER) && !mob_index[nr].func.mob' in db and 'int helpers[] = { 95505, 95532, 0 };' in general
postmaster=re.sub(r'/\*[\s\S]*?\*/','',mail.split('int postmaster[',1)[1].split('};',1)[0])
assert '95552' not in postmaster and all(str(v) in postmaster for v in (6097,16695,97583,73))
assert 'for (i = 0; postmaster[i] != -1; i++)' in mail
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[s['summary'] for s in mapping['stories']])
for phrase in ('two potatoes look identical','currently unavailable','not every animal instance','mother who accepts','brown','black','supplied','loose','daily'):assert phrase in guidance.lower(),phrase
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==955];assert len(units)==4 and all(u['achievement'] and u['daily_candidate'] for u in units)


# Cerberus: exact quantities, alternative trades and effective dispatch/supply limits.
cerebusp=inventory_module.area_evidence(ROOT,'cerebusp')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='cerebusp')
blocks=sorted((r['block'] for r in cerebusp['requests']),key=lambda b:b['line'])
assert [(b['line'],b['giver_vnum']) for b in blocks]==[(2,22006),(11,22006),(20,22006),(50,22007),(55,22007),(60,22007),(84,22029),(92,22029),(100,22048)]
assert all(b['kind']=='Q' and not b['disappear'] for b in blocks)
assert [b['give'] for b in blocks]==[[('I',v) for v in vs] for vs in ((22013,22015,22015,22017,22017),(22014,22014,22014,22014,22017),(22013,22015,22014,22014,22016),(22021,),(22004,),(22045,),(22023,22022,22025),(22046,22047),(22038,22039,22040,22041))]
assert [b['receive'] for b in blocks]==[[('I',v)] for v in (22028,22061,22062,22009,22009,22009,22024,22048,22042)]
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
assert len(mapping['stories'])==9 and [s['contracts'] for s in mapping['stories']]==[[b['binding']] for b in blocks]
assert [len(s['steps']) for s in mapping['stories']]==[4,3,5,2,2,2,4,3,5]
for s,b in zip(mapping['stories'],blocks):
 proof=collections.Counter(v for k,v in b['give']);steps=s['steps'][:-1]
 assert [(t['item_vnums'],t['count'],t['optional']) for t in steps]==[([v],count,True) for v,count in sorted(proof.items())]
 assert all(t['kind']=='carried_item' for t in steps) and s['category']=='story' and s['steps'][-1]['contracts']==s['contracts']
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert len(contacts)==17 and len(cerebusp['dialogue'])==5
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for d in cerebusp['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert sum(len(c['topics']) for c in contacts.values())==5 and not contacts[22006]['topics']
assert (cerebusp['zone']['zone_number'],cerebusp['zone']['first_vnum'],cerebusp['zone']['last_vnum'],cerebusp['zone']['reset_mode'])==(220,21938,22155,1)
rooms=dawndale_bodies('cerebusp','wld');objects=dawndale_bodies('cerebusp','obj');mobiles=dawndale_bodies('cerebusp','mob')
assert set(rooms)==set(range(22000,22156)) and set(objects)==set(range(22000,22072)) and set(mobiles)==set(range(22000,22083))
assert len(re.findall(r'\bD\d+\s+[^~]*~[^~]*~\s*-?\d+\s+-?\d+\s+-?\d+',''.join(rooms.values()),re.S))==323
assert collections.Counter(r['command'] for r in cerebusp['reset_commands'])=={'D':36,'O':60,'P':4,'M':244,'G':22,'F':12,'E':13}
reset=(ROOT/'areas/zon/cerebusp.zon').read_text().splitlines();assert len(reset)==510 and reset[-1]=='S'
assert reset[371].startswith('M 0 22024 1 22072 ')
assert all(objvalues(objects[v])[6]&4096 for v in (22013,22014,22015,22022,22023,22025,22038,22039,22040,22041,22046,22047))
assert objects[22023].split('~')[0]==objects[22047].split('~')[0] and objects[22023].split('~')[1]!=objects[22047].split('~')[1]
assert objvalues(objects[22024])[11:15]==[0,20,0,0] and objvalues(objects[22009])[11:15]==[0,0,0,0] and objvalues(objects[22042])[11:15]==[0,100,0,0]
portals={22002:22022,22003:22013,22005:22045,22006:22043,22010:40787,22033:22089,22034:22066,22043:22120,22044:22019}
assert all(objvalues(objects[v])[0]==25 and objvalues(objects[v])[11:15]==[dest,7,-1,0] for v,dest in portals.items())
assert '7 22024 22030' in rooms[22027] and '7 22009 22043' in rooms[22042] and '7 22042 22071' in rooms[22053]
assert [(a['kind'],a['vnum'],a['function']) for a in cerebusp['special_assignments']]==[('mob',22024,'cerberus_load'),('obj',22063,'master_set'),('obj',22070,'revenant_helm')]
flags=int(mobiles[22024].split('~',4)[4].strip().split()[0]);assert flags==8391692 and flags&4 and not flags&(1<<24)
fight=(ROOT/'src/combat/fight.c').read_text();worm=(ROOT/'src/specs/specs.winterhaven.c').read_text();assign=(ROOT/'src/specs/specs.assign.c').read_text();shop=(ROOT/'src/economy/shop.c').read_text();crown=(ROOT/'src/specs/specs.cerebusp.c').read_text()
assert 'IS_NPC(ch) && (ch->specials.act & ACT_SPEC_DIE)' in fight and '(*mob_index[GET_RNUM(ch)].func.mob)(ch, killer, CMD_DEATH, 0)' in fight
assert 'switch (number(0, 15))' in worm and 'switch (number(8, 15))' in worm and 'extract_char(ch)' in worm
assert 'Shop trades and services are unavailable while economic accounting is active.' in shop and 'if (refuse_unported_shop_mutation(ch))' in shop
assert (ROOT/'areas/shp/cerebusp.shp').read_text().startswith('#22026~\nN\n22016\n22017\n0\n')
assert '//  obj_index[real_object0(22032)].func.obj = warmace_puredark;' in assign and objvalues(objects[22032])[16:19]==[323461596,50,30]
assert 'add_event(event_revenant_crown' in crown and 'ch->player.race = RACE_REVENANT' in crown
assert re.search(r'T\s+2 0 0 0',dawndale_bodies('wh','obj')[55452])
quest_source=(ROOT/'src/world/quest.c').read_text()
load_q=quest_source.split("case 'Q':",1)[1].split("case 'D':",1)[0]
assert 'qcp->next = quest_index[number_of_quests].quest_complete;' in load_q and 'quest_index[number_of_quests].quest_complete = qcp;' in load_q
durable=quest_source.split('static bool submit_durable_quest_offering(',1)[1].split('int quester(',1)[0]
assert 'OBJ_VNUM(offering) == goal->number' in durable and 'if (!matching)' in durable and 'for (P_obj item = actor->carrying;' in durable
assert 'if (!complete)' in durable and 'return true;' in durable
assert [b['receive'][0][1] for b in reversed(blocks[:3])]==[22062,22061,22028]
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[s['summary'] for s in mapping['stories']])
for phrase in ('SEARCH','one treasure','four','currently unavailable','accounting','supplied','loose','builder review','choosing a journal card does not select the trade','several recipes'):assert phrase.lower() in guidance.lower(),phrase
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==220];assert len(units)==9 and all(u['achievement'] and u['daily_candidate'] for u in units)


# Carthapia: hidden coin, three identical-looking distinct scales, switches and training.
prison=inventory_module.area_evidence(ROOT,'prison')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='prison')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
blocks=[r['block'] for r in prison['requests']]
assert [(b['kind'],b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in blocks]==[('Q',20,7304,[('I',7372)],[('C',15000)],False),('Q',47,7306,[('I',7342),('I',7343),('I',7344)],[('I',7345)],False)]
assert len(prison['dialogue'])==6 and len(mapping['stories'])==2 and len(mapping['contacts'])==9
assert [s['contracts'] for s in mapping['stories']]==[[b['binding']] for b in blocks]
assert [len(s['steps']) for s in mapping['stories']]==[2,4]
assert [(t['item_vnums'],t['count'],t['optional']) for s in mapping['stories'] for t in s['steps'] if t['kind']=='carried_item']==[([v],1,True) for v in (7372,7342,7343,7344)]
assert all(s['category']=='story' and s['steps'][-1]['kind']=='completion' and s['steps'][-1]['contracts']==s['contracts'] for s in mapping['stories'])
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert set(contacts)=={7304,7306,7353,7351,7333,7332,7354,7357,7365}
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for d in prison['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert sum(len(c['topics']) for c in contacts.values())==8 and contacts[7306]['topics']==['hello','craft','talents','material']
assert (prison['zone']['zone_number'],prison['zone']['first_vnum'],prison['zone']['last_vnum'],prison['zone']['reset_mode'])==(73,7254,7498,1)
rooms=dawndale_bodies('prison','wld');objects=dawndale_bodies('prison','obj');mobiles=dawndale_bodies('prison','mob')
assert set(rooms)==set(range(7300,7499)) and set(objects)==set(range(7300,7378)) and set(mobiles)==set(range(7300,7372))
assert len(re.findall(r'\bD\d+\s+[^~]*~[^~]*~\s*-?\d+\s+-?\d+\s+-?\d+', ''.join(rooms.values()),re.S))==455
assert collections.Counter(r['command'] for r in prison['reset_commands'])=={'D':76,'O':32,'P':3,'M':107,'E':171,'F':20,'G':13}
reset=(ROOT/'areas/zon/prison.zon').read_text().splitlines();assert len(reset)==661 and reset[-1]=='S'
assert reset[312].startswith('O 0 7349 1 7353 ') and reset[314].startswith('P 1 7372 1 7349 ')
assert reset[365].startswith('M 0 7304 1 7301 ') and reset[367].startswith('M 0 7306 1 7302 ')
assert reset[595].startswith('M 0 7353 1 7443 ') and all(reset[596+i].startswith('G 1 '+str(7342+i)+' 1 ') for i in range(3))
assert objvalues(objects[7372])[6]&4096 and all(objvalues(objects[v])[6]&4096 and objvalues(objects[v])[8]&32768 for v in (7342,7343,7344))
assert len({tuple(objects[v].split('~')[:3]) for v in (7342,7343,7344)})==1
assert objvalues(objects[7333])[11:15]==[100,29,7334,100] and objvalues(objects[7349])[11:15]==[400,0,0,0]
assert objvalues(objects[7354])[0]==29 and objvalues(objects[7354])[11:15]==[341,7358,3,0]
assert objvalues(objects[7355])[0]==29 and objvalues(objects[7355])[11:15]==[341,7482,3,0]
portals={7336:[7386,7,-1,0],7337:[7465,7,-1,0],7351:[7479,7,-1,0],7359:[7484,7,-1,0],7360:[7486,7,-1,0],7361:[7488,7,-1,0],7362:[7490,7,-1,0],7363:[7492,7,-1,0],7364:[7494,7,-1,0],7371:[7497,7,-1,0]}
assert all(objvalues(objects[v])[0]==25 and objvalues(objects[v])[11:15]==values for v,values in portals.items())
assert re.findall(r'\bD(\d+)\s+[^~]*~[^~]*~\s*(-?\d+)\s*(-?\d+)\s*(-?\d+)',rooms[7422],re.S)==[('0','0','0','7421')]
assert 'Freedom lies upward' in rooms[7422] and '3 7377 7498' in rooms[7481] and '1 0 7481' in rooms[7498]
assert [(a['kind'],a['vnum'],a['function']) for a in prison['special_assignments']]==[('mob',7333,'warden_shout'),('obj',7365,'flaming_axe_of_azer'),('obj',7371,'nexus')]
assign=(ROOT/'src/specs/specs.assign.c').read_text();native=(ROOT/'src/specs/specs.prison.c').read_text();under=(ROOT/'src/specs/specs.underworld.c').read_text();db=(ROOT/'src/world/db.c').read_text();training=(ROOT/'src/classes/epic_skills.c').read_text();epic=(ROOT/'src/world/epic.c').read_text();switch=(ROOT/'src/specs/specs.object.c').read_text();search=(ROOT/'src/cmd/actobj.c').read_text();trap=(ROOT/'src/combat/trap.c').read_text()
assert 'obj_index[real_object0(358)].func.obj = epic_stone' in assign and '{ 7357, SKILL_KI_STRIKE, 0, 100, 0, 0, 0 }' in training
assert 'mob_index[real_mobile(epic_teachers[i].vnum)].func.mob = epic_teacher' in epic and 'Epic skill purchases are unavailable while economic accounting is active.' in training
assert 'cmd != CMD_PRACTICE' in training and 'SKILL_KI_STRIKE, 100, 100, 1000000, CLASS_MONK' in training
assert 'int dam = cmd / 1000;' in native and 'number(0, 29)' in native and 'read_mobile(AZER, VIRTUAL)' in native and 'shout_and_hunt(ch, 100' in native
assert 'to_room = number(0, top_of_world)' in under and '!IS_SURFACE_MAP(to_room)' in under
assert 'obj->type == ITEM_SWITCH && !obj_index[nr].func.obj' in db and 'obj_index[nr].func.obj = item_switch' in db
assert 'obj->value[0] != cmd' in switch and 'EX_BLOCKED' in switch and 'k = k->contains' in search and 'REMOVE_BIT(k->extra_flags, ITEM_SECRET)' in search
assert 'if (!obj->trap_charge)' in trap and re.search(r'T\s+2 0 0 0',dawndale_bodies('wh','obj')[55183])
assert objvalues(objects[7368])[16:19]==[33,50,30]
guidance=' '.join(mapping['orientation']+[c['description'] for c in mapping['contacts']]+[t['hint'] for s in mapping['stories'] for t in s['steps']])
for phrase in ('SEARCH','TUG','PRACTICE ki strike','three','hidden','random','currently unavailable','supplied','loose','active, ready accounting'):assert phrase.lower() in guidance.lower(),phrase
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==73];assert len(units)==2 and all(u['achievement'] and u['daily_candidate'] for u in units)


# Negative Material Plane: two departing returns, four stars, two scrolls and ordered callbacks.
negplane=inventory_module.area_evidence(ROOT,'negplane')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='negplane')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
blocks=[r['block'] for r in negplane['requests']]
assert len(blocks)==2
assert (blocks[0]['kind'],blocks[0]['line'],blocks[0]['giver_vnum'],blocks[0]['give'],blocks[0]['receive'],blocks[0]['disappear'])==('Q',24,26608,[('I',v) for v in (26616,26609,26611,26619,26638,26643)],[('I',26644),('I',26603)],True)
assert (blocks[1]['kind'],blocks[1]['line'],blocks[1]['giver_vnum'],blocks[1]['give'],blocks[1]['receive'],blocks[1]['disappear'])==('Q',98,26644,[('I',26614)],[('C',500000),('E',750000),('I',26662),('I',26667)],True)
assert len(negplane['dialogue'])==6 and len(mapping['stories'])==2 and len(mapping['contacts'])==10
assert [s['contracts'] for s in mapping['stories']]==[[b['binding']] for b in blocks]
assert [len(s['steps']) for s in mapping['stories']]==[7,2]
assert [(t['item_vnums'],t['count'],t['optional']) for s in mapping['stories'] for t in s['steps'] if t['kind']=='carried_item']==[([v],1,True) for v in (26616,26609,26611,26619,26638,26643,26614)]
assert all(s['category']=='story' and s['steps'][-1]['kind']=='completion' and s['steps'][-1]['contracts']==s['contracts'] for s in mapping['stories'])
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert set(contacts)=={26608,26644,26602,26614,26618,26630,26622,26621,26635,26642}
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for d in negplane['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert sum(len(c['topics']) for c in contacts.values())==9 and contacts[26644]['topics']==['hi','hello','force','dark','hope']
assert (negplane['zone']['zone_number'],negplane['zone']['first_vnum'],negplane['zone']['last_vnum'],negplane['zone']['reset_mode'])==(266,26579,26870,0)
rooms=dawndale_bodies('negplane','wld');objects=dawndale_bodies('negplane','obj');mobiles=dawndale_bodies('negplane','mob')
assert set(rooms)==set(range(26600,26871))-{26660} and set(objects)==set(range(26600,26669)) and set(mobiles)==set(range(26600,26646))
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==999 and {(26757,1,7,26603,26760),(26760,3,3,26603,26757),(26779,0,3,26602,26791),(26791,1,7,26632,26793),(26830,1,7,26645,26831),(26835,2,4,26645,26818),(26859,2,3,26667,26860),(26860,0,3,26667,26859)}<=set(edges)
assert not any(26861<=target<=26869 and not 26861<=room<=26869 for room,_,_,_,target in edges)
rs=negplane['reset_commands'];assert len(rs)==463 and collections.Counter(r['command'] for r in rs)=={'D':38,'O':36,'P':7,'M':222,'G':88,'E':65,'F':7}
assert sorted((r['arguments'][1],r['arguments'][3]) for r in rs if r['command']=='M' and r['arguments'][1] in (26608,26644,26642))==[(26608,26600),(26642,26859),(26644,26857)]
assert sorted((r['arguments'][1],r['arguments'][3]) for r in rs if r['command']=='O' and r['arguments'][1] in (26619,26624,26625,65,360))==[(65,26860),(360,26860),(26619,26831),(26624,26770),(26625,26770)]
assert [r['arguments'] for r in rs if r['command']=='P' and r['arguments'][1]==26643]==[[1,26643,1,26624,100,0,0,0]]
assert objvalues(objects[26624])[11:15]==[1000,29,26623,1000] and '6 0 1 100' in objects[26625]
portals={26600:[26706,7,-1,0],26601:[26601,7,-1,0],26605:[26870,120,-1,0],26606:[26649,120,-1,0],26612:[26755,166,-1,0],26613:[26643,166,-1,0],26636:[26780,7,-1,0],26637:[26628,7,-1,0],26640:[26819,7,-1,0],26641:[26818,7,-1,0],26655:[26600,7,-1,0]}
assert all(objvalues(objects[v])[0]==25 and objvalues(objects[v])[11:15]==values for v,values in portals.items())
assert objects[26612].split('~')[0].strip()=='boulder charred large volcanic' and objects[26613].split('~')[0].strip()=='frozen ash pile'
assert all(objvalues(objects[v])[12]==100 for v in (26602,26603,26623,26632,26667)) and objvalues(objects[26645])[12]==0
assert 'potions' in '\n'.join(blocks[1]['body']) and ('I',26661) not in blocks[1]['receive'] and ('I',26667) in blocks[1]['receive']
assign=(ROOT/'src/specs/specs.assign.c').read_text();native=(ROOT/'src/specs/specs.negplane.c').read_text();db=(ROOT/'src/world/db.c').read_text();quest=(ROOT/'src/world/quest.c').read_text();fight=(ROOT/'src/combat/fight.c').read_text()
assert [(a['kind'],a['vnum'],a['function']) for a in negplane['special_assignments']]==[('mob',26603,'neg_pocket'),('obj',26665,'elvenkind_cloak'),('obj',26653,'artifact_stone'),('obj',26662,'orb_of_destruction'),('obj',26621,'sanguine'),('obj',26662,'neg_orb'),('obj',26666,'transp_tow_misty_gloves')]
effective={ (a['kind'],a['vnum']):a['function'] for a in negplane['special_assignments']};assert len(effective)==6 and effective[('obj',26662)]=='neg_orb'
assert assign.index('obj_index[real_object0(26662)].func.obj = orb_of_destruction')<assign.index('obj_index[real_object0(26662)].func.obj = neg_orb')
assert 'cmd == CMD_DEATH' in native and 'dam = 250;' in native and 'GET_HIT(vict) -= dam;' in native and 'cmd == CMD_RUB' in native
assert 'data.victim = ch;' in fight and 'invoke_object_special(item, victim, CMD_GOTNUKED' in fight
assert 'state &=' in db and '3; // only grab first two bits' in db and 'extract_char(mob);' in quest
assert 'obj_index[real_object0(360)].func.obj = epic_stone' in assign and 'obj_index[real_object0(65)].func.obj = stat_pool_int' in assign and 'obj_index[real_object0(67243)].func.obj = living_necroplasm' in assign and 'obj_index[real_object0(67281)].func.obj = stalker_cloak' in assign
assert objvalues(objects[26615])[16:19]==[26010,40,30] and objvalues(objects[26620])[16:19]==[103,50,30] and objvalues(objects[26644])[16:19]==[107,40,30]
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==266]
assert len(units)==2 and sum(u['achievement'] for u in units)==2 and sum(u['daily_candidate'] for u in units)==0


# Voluntown: six family fragments, independent strand return, Mary lead, traps and imported stone.
voluntown=inventory_module.area_evidence(ROOT,'Voluntown')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='Voluntown')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
blocks=[r['block'] for r in voluntown['requests']]
assert len(blocks)==2
assert (blocks[0]['kind'],blocks[0]['line'],blocks[0]['giver_vnum'],blocks[0]['give'],blocks[0]['receive'],blocks[0]['disappear'])==('QA',64,142400,[('I',v) for v in range(142404,142410)],[('I',142443)],False)
assert (blocks[1]['kind'],blocks[1]['line'],blocks[1]['giver_vnum'],blocks[1]['give'],blocks[1]['receive'],blocks[1]['disappear'])==('QA',77,142400,[('I',142450)],[('I',142451)],False)
assert not voluntown['special_assignments'] and len(voluntown['dialogue'])==6
assert len(mapping['stories'])==2 and len(mapping['contacts'])==10
assert [s['contracts'] for s in mapping['stories']]==[[b['binding']] for b in blocks]
assert [len(s['steps']) for s in mapping['stories']]==[7,2]
assert [(t['item_vnums'],t['count'],t['optional']) for s in mapping['stories'] for t in s['steps'] if t['kind']=='carried_item']==[([v],1,True) for v in [*range(142404,142410),142450]]
assert all(s['category']=='story' and s['steps'][-1]['kind']=='completion' and s['steps'][-1]['contracts']==s['contracts'] for s in mapping['stories'])
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert set(contacts)=={142400,142401,142402,142408,142410,142417,142420,142422,142425,142427}
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for d in voluntown['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert sum(len(c['topics']) for c in contacts.values())==9 and contacts[142401]['topics']==['drakenstone']
assert (voluntown['zone']['zone_number'],voluntown['zone']['first_vnum'],voluntown['zone']['last_vnum'],voluntown['zone']['reset_mode'])==(1424,142284,142532,0)
rooms=dawndale_bodies('Voluntown','wld');objects=dawndale_bodies('Voluntown','obj');mobiles=dawndale_bodies('Voluntown','mob')
assert set(rooms)==set(range(142400,142533)) and set(objects)==set(range(142400,142452)) and set(mobiles)==set(range(142400,142430))
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==286 and {(142400,2,0,0,97132),(142400,3,3,97018,142401),(142401,3,0,97018,142402),(142431,0,3,142443,142438),(142451,2,3,142400,142452),(142452,0,3,142400,142451)}<=set(edges)
rs=voluntown['reset_commands'];assert len(rs)==185 and collections.Counter(r['command'] for r in rs)=={'D':22,'O':15,'P':14,'M':79,'E':33,'G':12,'F':10}
assert sorted((r['arguments'][1],r['arguments'][3]) for r in rs if r['command']=='P' and 142404<=r['arguments'][1]<=142409)==[(142404,142401),(142405,142415),(142406,142420),(142407,142427),(142408,142432),(142409,142438)]
assert sorted((r['arguments'][1],r['arguments'][3]) for r in rs if r['command']=='O' and r['arguments'][1] in (142401,142415,142420,142427,142432,142438,142450))==[(142401,142481),(142415,142469),(142420,142492),(142427,142508),(142432,142520),(142438,142530),(142450,142452)]
assert [(v,objvalues(objects[v])[12:14]) for v in (142401,142415,142420,142427,142432,142438)]==[(142401,[29,142402]),(142415,[29,142418]),(142420,[29,142421]),(142427,[29,142428]),(142432,[29,142433]),(142438,[0,142439])]
assert '512 0 0 60' in objects[142401] and '516 0 1 60' in objects[142420] and '512 9 1 60' in objects[142438]
assert all(objvalues(objects[v])[0]==18 and objects[v].split('~')[0].strip()=='key fragment' for v in range(142404,142410))
assert all(objvalues(objects[v])[16]>0 for v in (142419,142445,142449))
assert '_id_' in objects[142419] and '_id_name_' not in objects[142419]
quest=(ROOT/'src/world/quest.c').read_text();trap=(ROOT/'src/combat/trap.c').read_text();epic=(ROOT/'src/world/epic.c').read_text();assign=(ROOT/'src/specs/specs.assign.c').read_text()
assert "qcp->echoAll = (letterStrn[1] == 'A')" in quest and 'TRAP_DAM_SLASH 8' in trap and 'case TRAP_DAM' in trap and 'case 9:' not in trap
assert 'obj_index[real_object0(359)].func.obj = epic_stone' in assign and 'zone_touch_transaction_submit(touch)' in epic
assert 'if (result.reset_requested)' in epic and 'epic_publish_zone_touch(result)' in epic
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==1424]
assert len(units)==2 and sum(u['achievement'] for u in units)==2 and sum(u['daily_candidate'] for u in units)==2


# Firesworn: exact four essences, three outputs, generic combat and qualified continuation/access gaps.
firesworn=inventory_module.area_evidence(ROOT,'firesworn_altar')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='firesworn_altar')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
b=firesworn['requests'][0]['block'];assert len(firesworn['requests'])==1
assert (b['kind'],b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear'])==('QA',93,135114,[('I',v) for v in range(135106,135110)],[('I',135110),('I',135119),('I',135120)],False)
assert b['body']==['~'] and not firesworn['special_assignments']
assert len(mapping['stories'])==1 and len(mapping['contacts'])==8
s=mapping['stories'][0];assert s['category']=='story' and s['contracts']==[b['binding']] and len(s['steps'])==5
assert [(t['item_vnums'],t['count'],t['optional']) for t in s['steps'] if t['kind']=='carried_item']==[([v],1,True) for v in range(135106,135110)]
assert s['steps'][-1]['kind']=='completion' and s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional',False)
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert set(contacts)=={135114,135103,135104,135105,135106,135109,135110,135117}
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for d in firesworn['dialogue'] if d['giver_vnum']==v for w in d['body'][0].rstrip('~').split()]
assert sum(len(c['topics']) for c in contacts.values())==12 and len(firesworn['dialogue'])==7
assert (firesworn['zone']['zone_number'],firesworn['zone']['first_vnum'],firesworn['zone']['last_vnum'],firesworn['zone']['reset_mode'])==(1351,135049,135171,0)
rooms=dawndale_bodies('firesworn_altar','wld');objects=dawndale_bodies('firesworn_altar','obj');mobiles=dawndale_bodies('firesworn_altar','mob')
assert set(rooms)==set(range(135101,135172)) and set(objects)==set(range(135101,135134)) and set(mobiles)==set(range(135101,135122))
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==259 and {(135108,0,3,135110,135109),(135122,0,3,135110,135123),(135130,4,3,135133,135131),(135171,4,4,0,135108)}<=set(edges)
assert not any(key==135130 for room,direction,flags,key,destination in edges)
rs=firesworn['reset_commands'];assert len(rs)==112 and collections.Counter(r['command'] for r in rs)=={'D':18,'O':10,'P':1,'M':45,'G':12,'E':24,'F':2}
parent=None;proof=[]
for r in rs:
 if r['command'] in ('M','F','R'):parent=r['arguments']
 if r['command']=='G' and 135106<=r['arguments'][1]<=135109:proof.append((r['arguments'][1],parent[1],parent[3],r['arguments'][2],r['arguments'][4]))
assert sorted(proof)==[(135106,135103,135112,1,100),(135107,135104,135115,1,100),(135108,135105,135118,1,100),(135109,135106,135121,1,100)]
assert objvalues(objects[135101])[11:15]==[135157,7,-1,0] and objvalues(objects[135131])[11:15]==[135171,7,-1,0]
assert all(objvalues(objects[v])[0]==13 and objvalues(objects[v])[11:19]==[0]*8 for v in (135114,135115,135117))
assert all(objvalues(objects[v])[0]==5 and objvalues(objects[v])[16]>0 for v in (135112,135113,135116,135118,135121,135122))
# QA's A changes echo visibility; generic packed combat is separate from an accepted quest.
quest=(ROOT/'src/world/quest.c').read_text();conv=(ROOT/'src/mob/mobconv.c').read_text();death=(ROOT/'src/combat/fight.c').read_text();travel=(ROOT/'src/magic/spell_travel.c').read_text()
assert "qcp->echoAll = (letterStrn[1] == 'A')" in quest and 'isname("_spec1_", GET_NAME(ch))' in conv
assert 'obj_to_room(o, corpse->loc.room)' in death and 'IS_NPC(ch) && IS_NOCORPSE(ch)' in death
assert 'FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM' in travel and 'obj->value[1] != cmd' in travel
weapon=(ROOT/'src/combat/attack_effects.c').read_text();assert 'selected_packed_weapon_action(obj, ch, victim)' in weapon and 'obj->value[5] % 1000' in weapon
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==1351]
assert len(units)==1 and sum(u['achievement'] for u in units)==1 and sum(u['daily_candidate'] for u in units)==1


# Great Shaboath: information proof, four distinct essences and qualified computed/custom behavior.
shabo=inventory_module.area_evidence(ROOT,'shabo')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='shabo')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
raw=sorted((r['block'] for r in shabo['requests']),key=lambda b:b['line'])
assert [(b['kind'],b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[('Q',41,32827,[('I',32812)],[],False),('Q',87,32878,[('I',v) for v in range(32842,32846)],[('I',32847)],True)]
assert len(mapping['stories'])==2 and len(mapping['contacts'])==10 and sum(len(s['steps']) for s in mapping['stories'])==7
for s,b,goods in zip(mapping['stories'],raw,[[32812],list(range(32842,32846))]):
 assert s['category']=='story' and s['contracts']==[b['binding']]
 assert [(t['item_vnums'],t['count'],t['optional']) for t in s['steps'] if t['kind']=='carried_item']==[([v],1,True) for v in goods]
 assert s['steps'][-1]['kind']=='completion' and s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional',False)
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert set(contacts)=={32827,32878,32842,32864,32867,32870,32873,32803,32840,32843}
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for m in shabo['dialogue'] if m['giver_vnum']==v for w in m['body'][0].rstrip('~').split()]
assert contacts[32827]['keyword']=='filthy' and contacts[32842]['keyword']=='pallistren'
assert sum(len(c['topics']) for c in contacts.values())==18 and len(shabo['dialogue'])==7
assert (shabo['zone']['zone_number'],shabo['zone']['first_vnum'],shabo['zone']['last_vnum'],shabo['zone']['reset_mode'])==(328,32711,32919,0)
rooms=dawndale_bodies('shabo','wld');objects=dawndale_bodies('shabo','obj');mobiles=dawndale_bodies('shabo','mob')
assert set(rooms)==set(range(32800,32804))|set(range(32808,32920)) and set(objects)==set(range(32800,32869))|{32870} and set(mobiles)==set(range(32800,32883))
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==243 and {(32852,0,3,-2,32897),(32897,2,3,-2,32852),(32914,4,7,32847,32915),(32915,5,7,32847,32914),(32895,4,13,0,32896)}<=set(edges)
rs=shabo['reset_commands'];assert len(rs)==299 and collections.Counter(r['command'] for r in rs)=={'D':64,'O':18,'P':8,'M':137,'E':26,'G':34,'F':12}
parent=None;proof=[]
for r in rs:
 if r['command'] in ('M','F','R'):parent=r['arguments']
 if r['command']=='G' and 32842<=r['arguments'][1]<=32845:proof.append((r['arguments'][1],parent[1],parent[3],r['arguments'][2],r['arguments'][4]))
assert sorted(proof)==[(32842,32864,32909,1,100),(32843,32867,32912,1,100),(32844,32870,32906,1,100),(32845,32873,32903,1,100)]
assert any(r['command']=='O' and r['arguments'][1:4]==[32812,1,32845] for r in rs) and any(r['command']=='M' and r['arguments'][1:4]==[32878,1,32914] for r in rs)
assert objvalues(objects[32828])[0]==29 and objvalues(objects[32828])[11:15]==[270,32895,4,0]
assign=(ROOT/'src/specs/specs.assign.c').read_text();source=(ROOT/'src/specs/specs.shabo.c').read_text();speech=(ROOT/'src/cmd/actcomm.c').read_text();switch=(ROOT/'src/specs/specs.object.c').read_text();interp=(ROOT/'src/cmd/interp.h').read_text()
assert len(shabo['special_assignments'])==28
for low,high,handler in [(32907,32909,'shaboath_alternation_tower'),(32910,32911,'shaboath_necromancy_tower'),(32901,32902,'shaboath_enchantment_tower')]:
 assert re.search(r'for \(x = '+str(low)+r'; x <= '+str(high)+r'; x\+\+\)\s*\{\s*world\[real_room0\(x\)\].funct = '+handler,assign)
assert '#define CMD_PUSH 270' in interp and '#define CMD_TOUCH 320' in interp
assert '(EXIT(ch, door)->key == -2)' in speech and 'check_magic_doors(ch, argument + i);' in speech
assert 'REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED)' in speech and 'REMOVE_BIT(world[in_room].dir_option[door]->exit_info, EX_BLOCKED)' in switch
assert 'door double darlakanand~' in rooms[32852] and 'door double darkaland~' in rooms[32897]
# Qualify source gaps; these assertions do not execute or repair custom encounters.
palle=source[source.index('int shabo_palle('):source.index('int shabo_derro_savant(')]
assert 'static bool askedquestion = FALSE' in palle and 'static int timerr = 0' in palle and palle.count('read_mobile(32847, VIRTUAL)')==2
isname=(ROOT/'src/world/handler.c').read_text();assert 'if (!str || !namelist)\n\t\treturn FALSE;' in isname
petre=source[source.index('int shabo_petre('):source.index('int shabo_evilpetre(')]
assert 'return FALSE; // TRUE;' in petre and '!ch || !IS_AWAKE(ch) || cmd' in petre and 'extract_char(i)' not in petre
necro=source[source.index('int shaboath_necromancy_tower('):source.index('int shaboath_enchantment_tower(')]
assert 'GET_ITEM_TYPE(obj) == ITEM_CORPSE' in necro and 'obj_to_room(obj, real_room(NECROMANCER_BOSS_ROOM))' in necro
assert 'new_affect' in source and 'TAG_RACE_CHANGE' in source and 'af->modifier = GET_RACE(tch)' in source
events=(ROOT/'src/world/events.c').read_text();assert '(room->funct)(room->number, 0, 0, 0)' in events
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==328]
assert len(units)==2 and sum(u['achievement'] for u in units)==2 and sum(u['daily_candidate'] for u in units)==1
defs={r['definition']['giver_vnum']:r['definition'] for r in shabo['requests']}
assert defs[32827]['daily_eligible'] and not defs[32878]['daily_eligible'] and defs[32878]['daily_exclusion']=='Story-only quest'


# Aravne: distinct proof, independent royal return and qualified dynamic/custom sources.
aravne=inventory_module.area_evidence(ROOT,'clfhaven')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='clfhaven')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
raw=sorted((r['block'] for r in aravne['requests']),key=lambda b:b['line'])
expected=[('Q',32,21514,[('I',v) for v in range(21649,21658)],[('I',21663),('E',250000)],False),('Q',51,21514,[('I',v) for v in range(21658,21661)],[('I',21661),('I',21662),('E',300000)],False),('Q',83,21535,[('I',32490),('I',26614),('I',402)],[('I',409)],False)]
assert [(b['kind'],b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==expected
assert len(mapping['stories'])==3 and len(mapping['contacts'])==8 and sum(len(s['steps']) for s in mapping['stories'])==18
for s,b,goods in zip(mapping['stories'],raw,[list(range(21649,21658)),list(range(21658,21661)),[32490,26614,402]]):
 assert s['category']=='story' and s['contracts']==[b['binding']]
 assert [(t['item_vnums'],t['count'],t['optional']) for t in s['steps'] if t['kind']=='carried_item']==[([v],1,True) for v in goods]
 assert s['steps'][-1]['kind']=='completion' and s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional',False)
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert set(contacts)=={21514,21535,21549,21673,21595,21645,21650,21667}
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for m in aravne['dialogue'] if m['giver_vnum']==v for w in m['body'][0].rstrip('~').split()]
assert contacts[21514]['keyword']=='anesenthe' and contacts[21549]['keyword']=='marooned'
assert sum(len(c['topics']) for c in contacts.values())==15 and len(aravne['dialogue'])==7
assert aravne['zone']['zone_number']==215 and aravne['zone']['first_vnum']==21437 and aravne['zone']['last_vnum']==21937 and aravne['zone']['reset_mode']==1
rooms=dawndale_bodies('clfhaven','wld');objects=dawndale_bodies('clfhaven','obj');mobiles=dawndale_bodies('clfhaven','mob')
assert set(rooms)==set(range(21500,21938)) and set(objects)==set(range(21500,21664))|{21673} and set(mobiles)==set(range(21500,21674))
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==923 and {(21743,0,2,21559,21745),(21745,2,2,0,21743),(21851,0,3,21613,21856),(21856,2,3,21613,21851),(21865,3,3,21620,21874),(21874,1,3,21620,21865),(21930,0,3,21636,21931),(21931,2,3,21636,21930),(21936,0,3,21663,21937),(21937,2,3,21663,21936)}<=set(edges)
rs=aravne['reset_commands'];assert len(rs)==962 and collections.Counter(r['command'] for r in rs)=={'D':108,'O':27,'M':537,'E':187,'G':70,'F':31,'P':2}
parent=None;proof=[]
for r in rs:
 if r['command'] in ('M','F','R'):parent=r['arguments']
 if r['command']=='G' and 21649<=r['arguments'][1]<=21660:proof.append((r['arguments'][1],parent[1],parent[3],r['arguments'][2],r['arguments'][4]))
assert sorted(proof)==[(21649,21510,21510,10,100),(21650,21555,21652,10,100),(21651,21556,21657,10,100),(21652,21596,21744,10,100),(21653,21651,21875,10,100),(21654,21646,21857,10,100),(21655,21663,21929,10,100),(21656,21664,21929,10,100),(21657,21643,21849,10,100),(21658,21670,21937,10,100),(21659,21671,21937,10,100),(21660,21672,21937,10,100)]
assert any(r['command']=='M' and r['arguments'][1:4]==[21673,1,21568] for r in rs)
assert objvalues(objects[21673])[0]==13
# Dynamic full-boot binding is separate from the legacy scroll contract and literal audit.
teachers=(ROOT/'src/classes/epic_skills.c').read_text();epic=(ROOT/'src/world/epic.c').read_text();comm=(ROOT/'src/net/comm.c').read_text()
assert '{ 21535, SKILL_EPIC_LUCK, 0, 100, 0, 0, 0 }' in teachers
assert '{ EPIC_REWARD_SKILL, SKILL_EPIC_LUCK, 100, 50, 500000, 0 }' in teachers
assert 'mob_index[real_mobile(epic_teachers[i].vnum)].func.mob = epic_teacher' in epic and 'epic_initialization();' in comm
assert 'Epic skill purchases are unavailable while economic accounting is active.' in teachers
assert re.search(r'if \(cmd != CMD_PRACTICE\)',teachers) and 'SKILL_CREATE("epic luck", SKILL_EPIC_LUCK' in teachers
# Enabled death callback produces same-number sand, but local placement/UID is not admitted ownership.
assign=(ROOT/'src/specs/specs.assign.c').read_text();death=(ROOT/'src/specs/specs.winterhaven.c').read_text();db=(ROOT/'src/world/db.c').read_text();fight=(ROOT/'src/combat/fight.c').read_text();movement=(ROOT/'src/cmd/actobj.c').read_text()
assert 'mob_index[real_mobile0(21673)].func.mob = wh_corpse_to_object;' in assign
assert {(a['vnum'],a['function']) for a in aravne['special_assignments']}=={(21673,'wh_corpse_to_object'),(21549,'llyren'),(21611,'inn')}
proc=death[death.index('int wh_corpse_to_object('):death.index('int wh_corpse_to_object(')+750]
assert 'cmd == CMD_DEATH' in proc and 'read_object(GET_VNUM(ch), VIRTUAL)' in proc and 'obj_to_room(obj, ch->in_room)' in proc
assert 'SECS_PER_MUD_DAY / PULSE_MOBILE * WAIT_SEC' in proc and '8783880' in mobiles[21673]
assert 'persistence_next_item_uid()' in db and 'OBJ_RFLAG_CREATION_CANDIDATE' in db
assert 'func.mob)(ch, killer, CMD_DEATH, 0)' in fight and 'item_get_source_owner(ch, o_obj, s_obj, &source)' in movement
smith=inventory_module.area_evidence(ROOT,'airshipgrave');lens=next(r['block'] for r in smith['requests'] if r['block']['giver_vnum']==77543)
assert lens['line']==159 and lens['give']==[('I',21673),('C',25000)] and lens['receive']==[('I',77566)] and not lens['disappear']
assert re.search(r'^M 0 77543 1 77603 100', (ROOT/'areas/zon/airshipgrave.zon').read_text(),re.M)
porter=(ROOT/'src/specs/specs.clfhaven.c').read_text();handler=porter[porter.index('int llyren('):]
assert handler.index('economic_gameplay_authority::active()')<handler.index('transact(pl, NULL, ch, cost)')
assert '50 * 1000' in handler and '500 * 1000' in handler and '175 * 1000' in handler
assert 'else if (OBJ_WORN(container))' in handler and 'owner = t_obj->loc.wearing;' in handler
assert 'else if (OBJ_CARRIED(container))' in handler and 'owner = t_obj->loc.carrying;' in handler
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==215]
assert len(units)==3 and sum(u['achievement'] for u in units)==3 and sum(u['daily_candidate'] for u in units)==3


# Tharnadian Ruin: cross-zone exact proof, independent returns and alternative recipients.
tharn=inventory_module.area_evidence(ROOT,'tharnadian_ruin')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='tharnadian_ruin')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete') and not mapping['exclusions']
raw=sorted((r['block'] for r in tharn['requests']),key=lambda b:b['line'])
five=[('I',1375),('I',66225),('I',66226),('I',66227),('I',66230)]
expected=[('Q',57,5506,[('I',66201)],[('I',5504)],False),('Q',71,5506,five,[('I',66200),('E',275000),('C',300000)],False),('Q',87,5514,[('I',5513)],[('E',75000)],True),('Q',131,5528,five,[('I',66200),('C',300000),('E',275000)],False),('QA',143,5528,[('I',66201)],[('I',5504)],True)]
assert len(raw)==5 and [(b['kind'],b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==expected
assert len(mapping['stories'])==3 and len(mapping['contacts'])==5 and sum(len(s['steps']) for s in mapping['stories'])==10
for s,indices,goods in zip(mapping['stories'],[(1,3),(0,4),(2,)],[[1375,66225,66226,66227,66230],[66201],[5513]]):
 assert s['category']=='story' and s['contracts']==[raw[i]['binding'] for i in indices]
 assert [(t['item_vnums'],t['count'],t['optional']) for t in s['steps'] if t['kind']=='carried_item']==[([v],1,True) for v in goods]
 assert s['steps'][-1]['kind']=='completion' and s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional',False)
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert set(contacts)=={5506,5514,5528,5503,5520}
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for m in tharn['dialogue'] if m['giver_vnum']==v for w in m['body'][0].rstrip('~').split()]
assert sum(len(c['topics']) for c in contacts.values())==16 and len(tharn['dialogue'])==7
assert tharn['zone']['zone_number']==55 and tharn['zone']['first_vnum']==5418 and tharn['zone']['last_vnum']==5678 and tharn['zone']['reset_mode']==1
rooms=dawndale_bodies('tharnadian_ruin','wld');objects=dawndale_bodies('tharnadian_ruin','obj');mobiles=dawndale_bodies('tharnadian_ruin','mob')
assert set(rooms)==set(range(5500,5679)) and set(objects)==set(range(5500,5540)) and set(mobiles)==set(range(5500,5529))
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==371 and {(5511,5,5,0,5528),(5528,4,1,0,5511),(5540,0,3,82110,5541),(5541,2,3,82110,5540),(5671,1,3,5527,5672),(5672,3,3,5528,5671),(5501,3,0,0,78400),(5588,3,0,0,66200),(5616,0,0,0,1300)}<=set(edges)
rs=tharn['reset_commands'];assert len(rs)==238 and collections.Counter(r['command'] for r in rs)=={'D':26,'O':52,'P':5,'M':101,'E':14,'G':6,'F':34}
parent=None;goods=[]
for r in rs:
 if r['command'] in ('M','F','R'):parent=r['arguments']
 if r['command']=='G' and r['arguments'][1] in (5505,5534,5521,5527,5528):goods.append((r['arguments'][1],parent[1],parent[3]))
assert goods==[(5534,5503,5544),(5505,5506,5568),(5521,5520,5661),(5527,5524,5671),(5528,5524,5675)]
assert any(r['command']=='P' and r['arguments'][1:4]==[5504,1,5505] for r in rs)
assert any(r['command']=='O' and r['arguments'][1:4]==[5513,1,5599] for r in rs)
assert objvalues(objects[5501])[11:15]==[1000,29,5534,1000] and objvalues(objects[5520])[11:15]==[1000,29,5521,1000]
assert objvalues(objects[5505])[11:15]==[10,5,0,10] and objvalues(objects[5539])[0]==25 and objvalues(objects[5539])[11:15]==[568439,7,-1,0]
assert objvalues(objects[5513])[0]==13 and objvalues(objects[5527])[0]==18 and objvalues(objects[5528])[0]==18
assert not (ROOT/'areas/shp/tharnadian_ruin.shp').exists()
# Literal archaeology is qualified by compiler guards, not mistaken for live gates.
assign=(ROOT/'src/specs/specs.assign.c').read_text().splitlines();off=set();stack=[]
for n,line in enumerate(assign,1):
 if re.match(r'\s*#if\s+0\b',line):stack.append(True)
 elif re.match(r'\s*#if(?:def|ndef)?\b',line):stack.append(False)
 elif re.match(r'\s*#endif\b',line):stack.pop()
 if any(stack):off.add(n)
assert len(tharn['special_assignments'])==30 and sum(a['line'] in off for a in tharn['special_assignments'])==29
assert [(a['vnum'],a['function']) for a in tharn['special_assignments'] if a['line'] not in off]==[(5503,'undead_howl')]
howl=(ROOT/'src/specs/specs.tharnadian_ruin.c').read_text();interp=(ROOT/'src/cmd/interp.h').read_text();spawn=(ROOT/'src/world/db.c').read_text();mobact=(ROOT/'src/mob/mobact.c').read_text()
assert re.search(r'if \(cmd == CMD_SET_PERIODIC\)\s*return FALSE;',howl) and re.search(r'if \(cmd\)\s*return FALSE;',howl)
assert '#define CMD_MOB_COMBAT -102' in interp and '#define CMD_PERIODIC 0' in interp
assert 'func.mob)(ch, 0, CMD_MOB_COMBAT, 0)' in mobact and 'func.mob)(mob, NULL, CMD_SET_PERIODIC,' in spawn
assert 'number(0, 99) > 10' in howl and 'SAVING_FEAR' in howl and 'GET_OPPONENT(ch)' in howl
# Floor proof and guarded directions are separate from a personal kill or clearance history.
cityrooms=dawndale_bodies('cityruin','wld');cityz=(ROOT/'areas/zon/cityruin.zon').read_text();bz=(ROOT/'areas/zon/braddistock.zon').read_text();oz=(ROOT/'areas/zon/outpost.zon').read_text()
assert re.search(r'^O 0 66201 1 66251 100',cityz,re.M) and re.search(r'^M 0 66203 1 66250 100',cityz,re.M)
assert re.search(r'gate~\s*3 66200 66251',cityrooms[66250]) and re.search(r'gate~\s*3 66200 66250',cityrooms[66251])
br=inventory_module.area_evidence(ROOT,'braddistock');brraw=sorted((r['block'] for r in br['requests']),key=lambda b:b['line'])
assert [(b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in brraw]==[(1314,[('I',1340)],[('I',1375),('E',150000)],True),(1316,[('I',1334)],[('I',1340)],True)]
assert re.search(r'^G 1 1334 1 0 100',bz,re.M) and re.search(r'^M 0 82120 1 82123 100',oz,re.M) and re.search(r'^G 1 82110 1 0 100',oz,re.M)
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==55]
assert len(units)==3 and sum(u['achievement'] for u in units)==3 and sum(u['daily_candidate'] for u in units)==3


# Gagga-Jobo: exact repeated craft materials, paid-service classification and mixed-recipe dispatch blockers.
goblincave=inventory_module.area_evidence(ROOT,'goblincave')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='goblincave')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
raw=sorted((r['block'] for r in goblincave['requests']),key=lambda b:b['line']);assert len(raw)==6 and not mapping['exclusions']
expected=[(26,19005,[('I',19006),('C',1000)],19007,'service'),(32,19005,[('I',19006)]*2+[('C',2000)],19008,'service'),(39,19005,[('I',19006)]*3,19009,'request'),(46,19005,[('I',19006)]*4+[('C',10000)],19010,'service'),(67,19006,[('I',19011),('I',19013),('I',19006),('C',100000)],19012,'service'),(75,19006,[('I',19011)]*2,19014,'request')]
assert len(mapping['stories'])==6 and len(mapping['contacts'])==4 and sum(len(s['steps']) for s in mapping['stories'])==14
for b,s,(line,giver,give,reward,category) in zip(raw,mapping['stories'],expected):
 assert (b['kind'],b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear'])==('Q',line,giver,give,[('I',reward)],False)
 assert s['category']==category and s['contracts']==[b['binding']]
 items={t['item_vnums'][0]:t['count'] for t in s['steps'] if t['kind']=='carried_item'}
 assert items==dict(collections.Counter(v for k,v in give if k=='I')) and all(t.get('optional',False) for t in s['steps'] if t['kind']=='carried_item')
 assert s['steps'][-1]['kind']=='completion' and s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional',False)
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert set(contacts)=={19000,19001,19005,19006} and contacts[19005]['keyword']=='leather'
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for m in goblincave['dialogue'] if m['giver_vnum']==v for w in m['body'][0].rstrip('~').split()]
assert len(goblincave['dialogue'])==7 and sum(len(c['topics']) for c in contacts.values())==11
assert 'leatherworker&n' in inventory_mobs[19005]['keywords'] and 'leatherworker' not in inventory_mobs[19005]['keywords'] and 'leather' in inventory_mobs[19007]['keywords']
assert goblincave['zone']['zone_number']==190 and goblincave['zone']['first_vnum']==18905 and goblincave['zone']['last_vnum']==19022 and goblincave['zone']['reset_mode']==2
rs=goblincave['reset_commands'];assert len(rs)==65 and collections.Counter(r['command'] for r in rs)=={'D':4,'O':2,'P':10,'M':23,'E':4,'G':22}
rooms=dawndale_bodies('goblincave','wld');objects=dawndale_bodies('goblincave','obj');mobiles=dawndale_bodies('goblincave','mob')
assert set(rooms)==set(range(19000,19023)) and set(objects)==set(range(19000,19015)) and set(mobiles)==set(range(19000,19010))
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==45 and {(19010,5,1,0,19012),(19012,4,1,0,19010),(19017,1,1,0,19018),(19018,3,1,0,19017)}<=set(edges)
assert [(r['arguments'][1],r['arguments'][2],r['arguments'][3]) for r in rs if r['command']=='D']==[(19010,5,1),(19012,4,1),(19017,1,1),(19018,3,1)]
assert [r['arguments'][3] for r in rs if r['command']=='M' and r['arguments'][1]==19000]==[19018,19019,19019,19020,19021,19022]
assert [r['arguments'][3] for r in rs if r['command']=='M' and r['arguments'][1]==19001]==list(range(19012,19017))
parent=None;sources=[];products=[]
for r in rs:
 if r['command'] in ('M','F','R'):parent=r['arguments']
 if r['command']=='G' and r['arguments'][1] in (19006,19011,19013):sources.append((r['arguments'][1],r['arguments'][2],parent[1],parent[3]))
 if r['command']=='G' and r['arguments'][1] in (19007,19008,19009,19010,19014):products.append((r['arguments'][1],parent[1],parent[3]))
assert collections.Counter(v for v,cap,mob,room in sources)=={19006:6,19011:6,19013:5}
assert all((cap,mob)==((5,19001) if v==19013 else (6,19000)) for v,cap,mob,room in sources)
assert products==[(19007,19005,19008),(19008,19005,19008),(19009,19005,19008),(19010,19005,19008),(19014,19006,19011)]
assert not (ROOT/'areas/shp/goblincave.shp').exists() and not goblincave['special_assignments']
assert objvalues(objects[19001])[0]==15 and objvalues(objects[19001])[11:15]==[50,0,0,50]
assert objvalues(objects[19002])[0]==15 and objvalues(objects[19002])[11:15]==[100,1,0,100]
assert '\n14 20\n' in objects[19007] and '#define APPLY_MOVE 14' in (ROOT/'src/core/defines.h').read_text()
engine=(ROOT/'src/world/quest.c').read_text();selection=engine[engine.index('static bool submit_durable_quest_offering('):engine.index('void tell_quest(')]
assert 'goal->goal_type != QUEST_GOAL_ITEM' in selection and re.search(r'if \(!supported \|\| !count\)\s*\{\s*unsupported_goal = true;\s*break;',selection)
assert 'qcp->next = quest_index[number_of_quests].quest_complete;' in engine and re.search(r'\.quest_complete->give\s*=\s*gp;',engine)
assert list(reversed([b['line'] for b in raw if b['giver_vnum']==19005]))==[46,39,32,26] and raw[3]['give'][-1]==('C',10000)
assert 'This quest cannot accept offerings right now.' in engine
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==190]
assert len(units)==6 and sum(u['achievement'] for u in units)==2 and sum(u['daily_candidate'] for u in units)==2
assert [r['block']['line'] for r in goblincave['requests'] if r['definition']['daily_eligible']]==[39,75]


# Strathor Valley: foreign supplied proof, periodic song and balanced shared-access candidates.
stormht=inventory_module.area_evidence(ROOT,'stormht')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='stormht')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==1 and len(mapping['contacts'])==8 and not mapping['exclusions']
raw=stormht['requests'][0]['block'];assert len(stormht['requests'])==1
assert (raw['kind'],raw['line'],raw['giver_vnum'],raw['give'],raw['receive'],raw['disappear'])==('QA',63,30871,[('I',40073)],[('I',30846),('E',60000)],False)
story=mapping['stories'][0];assert story['id']=='sultan-proof-for-the-crown' and story['category']=='story' and story['contracts']==[raw['binding']]
assert [(t['id'],t['item_vnums'],t['count'],t['optional']) for t in story['steps'] if t['kind']=='carried_item']==[('sultan-sword',[40073],1,True)]
assert len(story['steps'])==2 and story['steps'][-1]['kind']=='completion' and story['steps'][-1]['contracts']==story['contracts'] and not story['steps'][-1].get('optional',False)
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert set(contacts)=={30814,30820,30821,30831,30833,30852,30868,30871}
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for m in stormht['dialogue'] if m['giver_vnum']==v for w in m['body'][0].rstrip('~').split()]
assert len(stormht['dialogue'])==8 and sum(len(c['topics']) for c in contacts.values())==16 and not contacts[30868]['topics']
qst=(ROOT/'areas/qst/stormht.qst').read_text();assert 'qc_action 200~' in qst and 'Woodseer' in qst and 'Jarus' in qst
assert stormht['zone']['zone_number']==308 and stormht['zone']['first_vnum']==30715 and stormht['zone']['last_vnum']==30937 and stormht['zone']['reset_mode']==2
rs=stormht['reset_commands'];assert len(rs)==336 and collections.Counter(r['command'] for r in rs)=={'M':224,'D':36,'O':26,'E':25,'G':16,'P':7,'F':2}
rooms=dawndale_bodies('stormht','wld');objects=dawndale_bodies('stormht','obj');mobiles=dawndale_bodies('stormht','mob')
assert set(rooms)==set(range(30800,30935))|{30937} and set(mobiles)==set(range(30800,30873)) and set(objects)==set(range(30800,30852))|{30853,30854}
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==297 and (30884,2,5,0,30885) in edges and (30916,2,8,0,30934) in edges and (30934,0,0,0,30916) in edges
assert [(r['arguments'][1],r['arguments'][2],r['arguments'][3]) for r in rs if r['command']=='D' and (r['arguments'][1],r['arguments'][2]) in ((30884,2),(30916,2),(30934,0))]==[(30884,2,5),(30916,2,8),(30934,0,0)]
assert objvalues(objects[30812])[0]==29 and objvalues(objects[30812])[11:14]==[340,30884,2]
assert objvalues(objects[30825])[0]==29 and objvalues(objects[30825])[11:14]==[340,30916,2] and 'push' in rooms[30923].lower()
assert (30848,3,1,0,30851) in edges and 'runes' in rooms[30848].lower()
assert (30845,0,2,30841,30849) in edges and (30849,2,1,0,30845) in edges
assert objvalues(objects[30850])[0]==15 and objvalues(objects[30850])[11:15]==[400,9,30851,20]
assert objvalues(objects[30830])[0]==15 and objvalues(objects[30830])[11:15]==[600,29,0,600]
assert objvalues(objects[30845])[0]==5 and objvalues(objects[30845])[16:19]==[263030,50,18]
parent=None;selected=[]
for r in rs:
 if r['command'] in ('M','F','R'):parent=r['arguments']
 if r['command'] in ('G','E') and r['arguments'][1] in (30841,30845,30851,30854):selected.append((r['command'],r['arguments'][1],parent[1],parent[3]))
assert selected==[('G',30841,30812,30845),('E',30845,30871,30926),('G',30851,30871,30926),('G',30854,30861,30931)]
assert any(r['command']=='M' and r['arguments'][1:5]==[30868,1,30822,50] for r in rs)
foreign=inventory_module.area_evidence(ROOT,'nizari');parent=None;sword_sources=[]
for r in foreign['reset_commands']:
 if r['command'] in ('M','F','R'):parent=r['arguments']
 if r['command']=='E' and r['arguments'][1]==40073:sword_sources.append((r['arguments'][2],r['arguments'][3],parent[1],parent[3]))
assert sword_sources==[(1,16,40073,40198)]
assert len(re.findall(r'^#\d+~',(ROOT/'areas/shp/stormht.shp').read_text(),re.M))==3
assert {(a['kind'],a['vnum'],a['function']) for a in stormht['special_assignments']}=={('mob',30831,'world_quest')}
engine=(ROOT/'src/world/quest.c').read_text();assert 'QC_ACTION' in engine and 'has_quest_ask' in engine
switch=(ROOT/'src/specs/specs.object.c').read_text();assert 'obj->value[0] != cmd' in switch and 'EX_BLOCKED' in switch
spells=(ROOT/'src/magic/spells.h').read_text();assert '#define SPELL_LIGHTNING_BOLT 30' in spells and '#define SPELL_BLUR 263' in spells
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==308];assert len(units)==1 and units[0]['achievement'] and units[0]['daily_candidate']


# Khildarak Stronghold: stable schema2 upgrade, ground proof, shared controls and balanced reference gaps.
khildarak=inventory_module.area_evidence(ROOT,'khildarak')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='khildarak')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,2,'complete')
assert len(mapping['stories'])==2 and len(mapping['contacts'])==6 and not mapping['exclusions']
blocks=inventory_module.native_blocks(ROOT)
raw=sorted((b for b in blocks if b['source']=='areas/qst/khildarak.qst' and b['kind'] in ('Q','QA')),key=lambda b:b['line'])
assert [(b['kind'],b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[('Q',36,17118,[('I',17074)],[('C',5000)],False),('Q',106,17248,[('I',17022)],[('I',17020)],False)]
assert [s['id'] for s in mapping['stories']]==['request-17118-e5eeeb25247d','request-17248-effe52f75b66']
for s,b in zip(mapping['stories'],raw):
 assert s['category']=='story' and s['contracts']==[b['binding']]
 assert [(t['id'],t['item_vnums'],t['count'],t['optional']) for t in s['steps'] if t['kind']=='carried_item']==[('item-1',[b['give'][0][1]],1,True)]
 assert s['steps'][-1]['id']=='turn-in' and s['steps'][-1]['kind']=='completion' and s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional',False)
assert sum(len(s['steps']) for s in mapping['stories'])==4
contacts={c['mob_vnum']:c for c in mapping['contacts']};assert set(contacts)=={17022,17059,17118,17199,17247,17248}
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for m in khildarak['dialogue'] if m['giver_vnum']==v for w in m['body'][0].rstrip('~').split()]
assert len(khildarak['dialogue'])==8 and sum(len(c['topics']) for c in contacts.values())==22 and 'retreive' in contacts[17118]['topics']
assert khildarak['zone']['zone_number']==170 and khildarak['zone']['first_vnum']==16961 and khildarak['zone']['last_vnum']==17750 and khildarak['zone']['reset_mode']==2
rs=khildarak['reset_commands'];assert len(rs)==1621
assert collections.Counter(r['command'] for r in rs)=={'D':528,'O':61,'P':5,'M':639,'R':27,'E':38,'G':232,'F':91}
rooms=dawndale_bodies('khildarak','wld');objects=dawndale_bodies('khildarak','obj');mobiles=dawndale_bodies('khildarak','mob')
assert set(rooms)==set(range(17000,17751)) and set(mobiles)==set(range(17000,17266)) and set(objects)==set(range(17000,17098))
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==1582 and (17537,5,0,0,17637) in edges and (17637,4,0,0,17537) in edges
assert not any(r['command']=='D' and (r['arguments'][1],r['arguments'][2]) in ((17537,5),(17637,4)) for r in rs)
assert [(r['arguments'][2],r['arguments'][3]) for r in rs if r['command']=='O' and r['arguments'][1]==17074]==[(2,17344),(2,17348)]
assert [(r['arguments'][2],r['arguments'][3]) for r in rs if r['command']=='O' and r['arguments'][1]==17022]==[(1,17648)]
assert [(r['arguments'][2],r['arguments'][3]) for r in rs if r['command']=='M' and r['arguments'][1]==17247]==[(3,17643)]*3
assert [r['arguments'][3] for r in rs if r['command']=='M' and r['arguments'][1]==17199]==[17343,17345,17347]
assert not any(r['command'] in ('G','E','O','P') and r['arguments'][1] in (17014,17015) for r in rs)
for item,cmd,room,direction in ((17004,66,17070,5),(17006,270,17332,5),(17014,274,17537,5),(17015,274,17637,4)):
 assert objvalues(objects[item])[0]==29 and objvalues(objects[item])[11:14]==[cmd,room,direction]
 assert any(v==room and d==direction for v,d,f,k,t in edges)
assert objvalues(objects[17019])[0]==15 and objvalues(objects[17019])[11:15]==[50,29,17020,250] and objvalues(objects[17020])[0]==18
assert any(r['command']=='O' and r['arguments'][1:5]==[17019,1,17160,100] for r in rs)
assert any(r['command']=='P' and r['arguments'][1:5]==[17021,1,17019,100] for r in rs)
assert any(r['command']=='M' and r['arguments'][1:4]==[17271,10,17271] for r in rs) and 17271 not in inventory_mobs
parent=None;missing_stock=[]
for r in rs:
 if r['command'] in ('M','F','R'):parent=r['arguments']
 if r['command']=='G' and r['arguments'][1] in (6070,6109,6110):missing_stock.append((r['arguments'][1],parent[1],parent[3]))
assert missing_stock==[(6070,17003,17281),(6109,17003,17281),(6110,17003,17281)]
assert all(v not in inventory_items for v in (6070,6109,6110))
assert len(re.findall(r'^#\d+~',(ROOT/'areas/shp/khildarak.shp').read_text(),re.M))==27
assignments={(a['kind'],a['vnum'],a['function']) for a in khildarak['special_assignments']}
assert len(assignments)==19 and {('mob',17247,'guild_guard'),('mob',17199,'devour'),('mob',17022,'world_quest'),('obj',17021,'khildarak_warhammer')}<=assignments
priest_krakens=next(b for b in khildarak['dialogue'] if b['giver_vnum']==17248 and b['body'][0].startswith('kraken '))
assert 'I cannot be sure' in ' '.join(priest_krakens['body'])
interp=(ROOT/'src/cmd/interp.c').read_text();assert 'IS_NPC(k) && IS_AWAKE(k)' in interp and '!IS_IMMOBILE(k)' in interp
h=(ROOT/'src/cmd/interp.h').read_text();assert '#define CMD_REMOVE 66' in h and '#define CMD_SHOVE 274' in h and '#define CMD_PUSH 270' in h
db=(ROOT/'src/world/db.c').read_text();assert '3; // only grab first two bits' in db and "case 'R': /* last mob loaded with M/F command will mount this */" in db
service=(ROOT/'src/classes/drannak.c').read_text();assert '#define SHARDS_FOR_ORB 3' in service and 'craft_recipe_discipline::harvester' in service and 'item_movement_transaction_submit_craft' in service
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==170]
assert len(units)==2 and sum(u['achievement'] for u in units)==2 and sum(u['daily_candidate'] for u in units)==2


# Labyrinth of No Return: independent exact bundles, follower sources, controls and bounded map discrepancy.
labyrinth=inventory_module.area_evidence(ROOT,'labyrinth')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='labyrinth')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==3 and len(mapping['contacts'])==8 and not mapping['exclusions']
blocks=inventory_module.native_blocks(ROOT)
raw=sorted((b for b in blocks if b['source']=='areas/qst/labyrinth.qst' and b['kind'] in ('Q','QA')),key=lambda b:b['line'])
assert [(b['kind'],b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[('QA',24,5024,[('I',5014)],[('I',5073),('C',24000),('E',24000)],True),('Q',85,5028,[('I',5001),('I',5002),('I',5003),('I',5005),('I',5006),('I',5007),('I',5008),('I',5009),('I',5020)],[('I',5031),('E',50000)],False),('Q',117,5055,[('I',5061),('I',5053),('I',5058)],[('I',5069),('I',5070)],True)]
assert [s['id'] for s in mapping['stories']]==['adventurer-dusty-map','knight-nine-proofs','vadatorn-minotaur-trio']
for s,b in zip(mapping['stories'],raw):
 assert s['category']=='story' and s['contracts']==[b['binding']]
 material=[t for t in s['steps'] if t['kind']=='carried_item']
 assert [(t['item_vnums'],t['count'],t['optional']) for t in material]==[([v],1,True) for k,v in b['give']]
 assert s['steps'][-1]['kind']=='completion' and s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional',False)
assert sum(len(s['steps']) for s in mapping['stories'])==16
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={5000,5006,5019,5024,5028,5047,5048,5055}
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for m in labyrinth['dialogue'] if m['giver_vnum']==v for w in m['body'][0].rstrip('~').split()]
assert len(labyrinth['dialogue'])==8 and sum(len(c['topics']) for c in contacts.values())==31
assert labyrinth['zone']['zone_number']==50 and labyrinth['zone']['first_vnum']==4969 and labyrinth['zone']['last_vnum']==5295 and labyrinth['zone']['reset_mode']==1
rs=labyrinth['reset_commands'];assert len(rs)==282
assert collections.Counter(r['command'] for r in rs)=={'D':14,'O':28,'P':7,'M':127,'G':62,'E':38,'F':6}
rooms=dawndale_bodies('labyrinth','wld');objects=dawndale_bodies('labyrinth','obj');mobiles=dawndale_bodies('labyrinth','mob')
assert set(rooms)==set(range(5000,5296)) and set(mobiles)==set(range(5000,5070)) and set(objects)==set(range(5000,5077))
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==602 and [(v,d,t) for v,d,f,k,t in edges if t not in rooms]==[(5000,4,20971)]
for edge in ((5000,3,15,0,5295),(5295,1,3,5057,5000),(5134,1,7,5057,5135),(5135,3,2,5057,5134),(5000,0,0,0,5008),(5008,0,0,0,5019)):assert edge in edges
assert not any(v==5019 and d==0 for v,d,f,k,t in edges) and '3 north, 4 west' in objects[5014]
parent=None;gear={}
for r in rs:
 cmd,a=r['command'],r['arguments']
 if cmd in ('M','F','R'):parent=(cmd,a)
 if cmd in ('E','G'):gear.setdefault((cmd,a[1]),[]).append((a,parent))
for cmd,item,carrier,room,kind,slot in (('E',5014,5019,5189,'M',18),('G',5002,5003,5131,'M',0),('G',5006,5008,5248,'F',0),('G',5007,5009,5248,'F',0),('G',5008,5010,5248,'M',0),('G',5009,5011,5248,'F',0),('G',5061,5048,5082,'M',0),('G',5053,5000,5143,'M',0),('G',5058,5047,5241,'M',0),('G',5057,5006,5134,'M',0),('G',5011,5016,5129,'M',0)):
 assert [(a[2:5],p[0],p[1][1],p[1][3]) for a,p in gear[(cmd,item)]]==[([1,slot,100],kind,carrier,room)]
assert [(p[1][1],p[1][3]) for a,p in gear[('G',5001)]]==[(5001,5005),(5002,5014),(5031,5133)]
assert [(p[1][1],p[1][3]) for a,p in gear[('G',5003)]]==[(5004,5077),(5032,5086),(5004,5107),(5032,5251)]
assert [(p[1][1],p[1][3]) for a,p in gear[('G',5005)]]==[(5007,5019),(5060,5125),(5034,5226)]
for item,cap,container in ((5020,1,5019),(5016,1,5067)):
 assert any(r['command']=='P' and r['arguments'][1:5]==[item,cap,container,100] for r in rs)
for item,room in ((5042,5171),(5043,5155),(5044,5248),(5046,5134),(5047,5134),(359,5003),(5019,5239),(5035,5003)):
 assert any(r['command']=='O' and r['arguments'][1:5]==[item,1,room,100] for r in rs)
for item,values in ((5042,[340,5276,0,0]),(5043,[340,5235,1,0]),(5044,[340,5002,1,0]),(5046,[270,5134,3,0]),(5047,[10,5134,0,0])):
 assert objvalues(objects[item])[0]==29 and objvalues(objects[item])[11:15]==values
 assert any(v==values[1] and d==values[2] for v,d,f,k,t in edges)
assert objvalues(objects[5047])[7]==0 and objvalues(objects[5019])[0]==15 and objvalues(objects[5019])[7]==0
assert not labyrinth['special_assignments'] and not (ROOT/'areas/shp/labyrinth.shp').exists()
foreign=next(b for b in blocks if b['source']=='areas/qst/tower.qst' and b['kind']=='QA' and b['line']==15)
assert foreign['giver_vnum']==9321 and foreign['give']==[('I',5011),('I',5016),('I',5035)] and foreign['receive']==[('E',18100),('I',9375),('C',53000)] and foreign['disappear']
h=(ROOT/'src/cmd/interp.h').read_text();assert '#define CMD_PULL 340' in h and '#define CMD_PUSH 270' in h and '#define CMD_GET 10' in h
db=(ROOT/'src/world/db.c').read_text();assert 'state &=' in db and '3; // only grab first two bits' in db
q=(ROOT/'src/world/quest.c').read_text();assert "qmp->echoAll = (letterStrn[1] == 'A')" in q and "qcp->echoAll = (letterStrn[1] == 'A')" in q
assert 'zone_story_quest_runtime::encountered(pl, ch)' in q
assert 'obj_index[real_object0(359)].func.obj = epic_stone' in (ROOT/'src/specs/specs.assign.c').read_text()
assert 'zone_touch_transaction_submit(touch)' in (ROOT/'src/world/epic.c').read_text()
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==50]
assert len(units)==3 and sum(u['achievement'] for u in units)==3 and sum(u['daily_candidate'] for u in units)==3


# Southern Coastal Highway: cross-zone optional history, independent paired returns and exact controls.
highway=inventory_module.area_evidence(ROOT,'highway')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='highway')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==3 and len(mapping['contacts'])==10 and not mapping['exclusions']
blocks=inventory_module.native_blocks(ROOT)
raw=sorted((b for b in blocks if b['source']=='areas/qst/highway.qst' and b['kind'] in ('Q','QA')),key=lambda b:b['line'])
assert [(b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[(19,41315,[('I',41398)],[('E',2500),('I',41405)],True),(75,41360,[('I',41348)],[('E',10000),('C',10000),('I',41417)],False),(90,41360,[('I',41415),('I',41416)],[('I',41419)],False)]
assert [s['id'] for s in mapping['stories']]==['morlanthra-apprentice-ring','magnamus-emerald-chalice','magnamus-harpy-parts']
victor=next(b for b in blocks if b['source']=='areas/qst/bastine.qst' and b['kind']=='Q' and b['line']==187)
assert victor['give']==[('I',41397)] and victor['receive']==[('I',41394),('I',41398)] and not victor['disappear']
assert mapping['stories'][0]['steps'][0]['kind']=='completion' and mapping['stories'][0]['steps'][0]['optional'] and mapping['stories'][0]['steps'][0]['contracts']==[victor['binding']]
for s,b in zip(mapping['stories'],raw):
 assert s['category']=='story' and s['contracts']==[b['binding']]
 material=[t for t in s['steps'] if t['kind']=='carried_item']
 assert [(t['item_vnums'],t['count'],t['optional']) for t in material]==[([v],1,True) for k,v in b['give']]
 assert s['steps'][-1]['kind']=='completion' and s['steps'][-1]['contracts']==s['contracts'] and not s['steps'][-1].get('optional',False)
assert sum(len(s['steps']) for s in mapping['stories'])==8
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={41302,41306,41313,41314,41315,41338,41355,41358,41360,41361}
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for m in highway['dialogue'] if m['giver_vnum']==v for w in m['body'][0].rstrip('~').split()]
assert len(highway['dialogue'])==8 and sum(len(c['topics']) for c in contacts.values())==25
assert highway['zone']['zone_number']==413 and highway['zone']['first_vnum']==41225 and highway['zone']['last_vnum']==41838 and highway['zone']['reset_mode']==2
rs=highway['reset_commands'];assert len(rs)==296
assert collections.Counter(r['command'] for r in rs)=={'D':36,'O':25,'P':12,'M':130,'E':82,'G':9,'R':2}
rooms=dawndale_bodies('highway','wld');objects=dawndale_bodies('highway','obj');mobiles=dawndale_bodies('highway','mob')
assert set(rooms)==set(range(41300,41839)) and set(mobiles)==set(range(41300,41364)) and set(objects)==set(range(41300,41418))|{41419,41421}
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==1411
assert [(v,d,t) for v,d,f,k,t in edges if t not in rooms]==[(41316,0,42137),(41488,5,-1),(41585,3,42152),(41618,1,7600),(41732,1,531250),(41837,0,530849)]
for edge in ((41690,5,6,41414,41691),(41691,4,2,41414,41690),(41701,3,2,41394,41702),(41702,1,1,-1,41701),(41693,0,7,41396,41703),(41703,0,3,41395,41705),(41705,2,1,0,41703),(41705,0,3,41343,41706),(41706,2,2,41343,41705),(41559,5,9,0,41584),(41584,3,9,0,41583)):assert edge in edges
parent=None;gear={}
for r in rs:
 cmd,a=r['command'],r['arguments']
 if cmd in ('M','F','R'):parent=(cmd,a)
 if cmd in ('E','G'):gear.setdefault((cmd,a[1]),[]).append((a,parent))
for cmd,item,cap,slot,chance,carrier,room in (('G',41414,1,0,100,41306,41690),('E',41397,1,2,100,41355,41701),('G',41396,1,0,100,41313,41702),('G',41395,1,0,100,41314,41704),('G',41416,1,0,20,41358,41838),('E',41411,1,12,100,41302,41724)):
 assert [(a[2:5],p[0],p[1][1],p[1][3]) for a,p in gear[(cmd,item)]]==[([cap,slot,chance],'M',carrier,room)]
assert [(a[2:5],p[0],p[1][1],p[1][3]) for a,p in gear[('E',41316)]]==[([1,19,100],'R',41347,41810)]
assert len(gear[('G',41315)])==2 and all(p[0]=='R' and p[1][1]==41347 for a,p in gear[('G',41315)])
for item,room,chance in ((41348,41540,100),(41343,41540,100),(41415,41838,10),(41412,41561,100),(41351,41559,100)):
 assert any(r['command']=='O' and r['arguments'][1:5]==[item,1,room,chance] for r in rs)
assert objvalues(objects[41412])[0]==29 and objvalues(objects[41412])[11:15]==[259,41584,3,0]
assert objvalues(objects[41351])[0]==29 and objvalues(objects[41351])[11:15]==[176,41559,5,0]
assert objvalues(objects[41349])[0]==13 and objvalues(objects[41349])[7]==0
assert objvalues(objects[41363])[0]==15 and objvalues(objects[41363])[12:14]==[15,41364]
assert objvalues(objects[41353])[12:14]==[5,0]
assert re.search(r'\bT\s+516\s+9\s+1\s+50',objects[41353]) and re.search(r'\bT\s+3140\s+10\s+-1\s+100',objects[41361])
assert {(a['kind'],a['vnum'],a['function']) for a in highway['special_assignments']}=={('obj',41304,'kearonor_hide'),('obj',41349,'hewards_mystical_organ'),('obj',41350,'wand_of_wonder')}
assert not (ROOT/'areas/shp/highway.shp').exists()
for zone in catalog['zones']:
 assert not re.search(r'^\s*[OPEG]\s+\d+\s+41349\s',(ROOT/f"areas/zon/{zone['source_area']}.zon").read_text(encoding='utf8'),re.M)
assert not any(b['kind'] in ('Q','QA') and ('I',41349) in b['receive'] for b in blocks)
h=(ROOT/'src/cmd/interp.h').read_text();assert '#define CMD_PRAY 176' in h and '#define CMD_PLAY 254' in h and '#define CMD_RUB 259' in h
special=(ROOT/'src/specs/specs.highway.c').read_text()
organ=special.split('int hewards_mystical_organ(',1)[1].split('int amethyst_orb(',1)[0]
assert 'static int working = FALSE' in organ and 'CMD_PLAY' in organ and 'POS_SITTING' in organ and 'CMD_LOOKAFAR' in organ
assert 'begin_wonder_action' in special and 'int kearonor_hide(' in special
trap=(ROOT/'src/combat/trap.c').read_text();assert '#define TRAP_DAM_SLASH 8' in trap and not re.search(r'^#define TRAP_DAM_\w+ (?:9|10)\b',trap,re.M)
assert {41915,41917} <= set(dawndale_bodies('mir','obj'))
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==413]
assert len(units)==3 and sum(u['achievement'] for u in units)==3 and sum(u['daily_candidate'] for u in units)==3

# Caves of Mt. Skelenak: independent rewardless tests, paired returns and native topic limits.
caves_skelenak=inventory_module.area_evidence(ROOT,'caves_skelenak')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='caves_skelenak')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==9 and len(mapping['contacts'])==8 and not mapping['exclusions']
blocks=inventory_module.native_blocks(ROOT)
raw=sorted((b for b in blocks if b['source']=='areas/qst/caves_skelenak.qst' and b['kind'] in ('Q','QA')),key=lambda b:b['line'])
expected=[(36,4027,[('I',4021)],[],False),(49,4027,[('I',4022)],[],False),(63,4027,[('I',4023)],[],False),(143,4038,[('I',4005),('I',4025)],[('I',4030)],False),(155,4038,[('I',26438)],[('I',4029)],False),(166,4038,[('I',16071)],[('C',100000)],False),(172,4038,[('I',20604)],[('C',150000)],False),(178,4038,[('I',4016),('I',4017)],[('I',4028)],False),(192,4038,[('I',26013)],[('I',4031)],True)]
assert [(b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==expected
assert [s['id'] for s in mapping['stories']]==['goortok-silver-disc','goortok-platinum-disc','goortok-strange-vial','monk-tribal-leaders','monk-warlord-shield','monk-marble-eye','monk-glass-eye','monk-pyrohydra-bracelets','monk-troll-king-sphere']
for s,b in zip(mapping['stories'],raw):
 assert s['category']=='story' and s['contracts']==[b['binding']]
 assert [(t['item_vnums'],t['count'],t['optional'],t['kind']) for t in s['steps'][:-1]]==[([v],1,True,'carried_item') for kind,v in b['give']]
 assert s['steps'][-1]['kind']=='completion' and s['steps'][-1]['contracts']==s['contracts']
assert sum(len(s['steps']) for s in mapping['stories'])==20
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={4009,4019,4026,4027,4032,4034,4037,4038}
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for m in caves_skelenak['dialogue'] if m['giver_vnum']==v for w in m['body'][0].rstrip('~').split()]
assert contacts[4027]['topics']==['moralon']
assert contacts[4038]['topics']==['greetings','hello','hi','solace','rock','tyrrany','warlord','quest','eyes','blind','eye','help','task','glass','eyeball','eyeballs']
assert sum(len(c['topics']) for c in contacts.values())==17 and len(caves_skelenak['dialogue'])==8
messages=[b for b in blocks if b['source']=='areas/qst/caves_skelenak.qst' and b['kind']=='M']
assert len(messages)==9 and sum(len(b['body'][0].rstrip('~').split()) for b in messages)==18
assert [(b['giver_vnum'],b['body'][0].rstrip('~')) for b in messages if "'" in b['body'][0]]==[(4027,"gid'nama")]
assert "gid'nama" not in json.dumps(mapping,ensure_ascii=False) and all("'" not in topic for c in contacts.values() for topic in c['topics'])
assert caves_skelenak['zone']['reset_mode']==2 and caves_skelenak['zone']['first_vnum']==3975 and caves_skelenak['zone']['last_vnum']==4153
rs=caves_skelenak['reset_commands']; assert len(rs)==160
assert collections.Counter(r['command'] for r in rs)=={'D':16,'O':10,'P':2,'M':116,'E':10,'G':6}
rooms=dawndale_bodies('caves_skelenak','wld');objects=dawndale_bodies('caves_skelenak','obj');mobiles=dawndale_bodies('caves_skelenak','mob')
assert set(rooms)==set(range(4001,4124))|set(range(4125,4154)) and set(objects)==set(range(4001,4032)) and set(mobiles)==set(range(4001,4039))
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==324 and [(v,d,t) for v,d,f,k,t in edges if t not in rooms]==[(4001,2,607420),(4093,5,709664),(4111,1,153645),(4125,3,557269),(4130,1,556876),(4131,0,556876),(4152,4,69088)]
for edge in ((4075,1,1,0,4076),(4076,3,1,0,4075),(4076,5,2,4004,4090),(4090,4,2,4004,4076),(4063,5,1,0,4064),(4064,4,1,0,4063),(4116,0,1,0,4117),(4117,2,1,0,4116),(4148,1,5,0,4149),(4149,3,5,0,4148),(4153,5,0,0,4110)): assert edge in edges
assert not any(e[4]==4153 for e in edges) and [e[1] for e in edges if e[0]==4153]==[5]
assert objvalues(objects[4027])[0]==25 and objvalues(objects[4027])[7]==0 and objvalues(objects[4027])[11:15]==[4153,555,-1,0]
assert objvalues(objects[4004])[0]==18 and objvalues(objects[4004])[11:13]==[2,100]
assert objvalues(objects[4020])[0]==15 and objvalues(objects[4020])[12:14]==[0,-1]
current=None; gear={}
for r in rs:
 c,a=r['command'],r['arguments']
 if c in ('M','F'): current=(c,a)
 if c in ('E','G'): gear.setdefault((c,a[1]),[]).append((a,current))
for cmd,item,cap,slot,carrier,room in (('E',4005,2,16,4019,4021),('E',4025,1,14,4009,4049),('G',4016,1,0,4034,4119),('G',4017,1,0,4034,4119),('E',4004,3,18,4027,4076)):
 assert [(a[2:5],p[0],p[1][1],p[1][3]) for a,p in gear[(cmd,item)]]==[([cap,slot,100],'M',carrier,room)]
assert any(r['command']=='O' and r['arguments'][1:5]==[4020,1,4107,100] for r in rs)
assert any(r['command']=='P' and r['arguments'][1:5]==[4021,1,4020,100] for r in rs)
assert any(r['command']=='O' and r['arguments'][1:5]==[4027,1,4110,100] for r in rs)
assert any(r['command']=='M' and r['arguments'][1:5]==[4038,1,4153,100] for r in rs)
assert {(a['kind'],a['vnum'],a['function']) for a in caves_skelenak['special_assignments']}=={('mob',4070,'piercer'),('mob',4120,'guild_guard')}
assert 4070 not in inventory_mobs and 4120 not in inventory_mobs and 4026 in inventory_mobs
db=(ROOT/'src/world/db.c').read_text(encoding='utf8')
assert 'economic_gameplay_authority::active() &&\n\t\t    reset_command_issues_item(ZCMD.command)' in db
real_mobile=db.split('int real_mobile0(',1)[1].split('int real_mobile(',1)[0]; assert 'return (0);' in real_mobile
assign=(ROOT/'src/specs/specs.assign.c').read_text(encoding='utf8')
for v,fn in ((4070,'piercer'),(4120,'guild_guard')): assert re.search(r'mob_index\[real_mobile0\('+str(v)+r'\)\]\.func\.mob\s*=\s*'+fn,assign)
interp_h=(ROOT/'src/cmd/interp.h').read_text(encoding='utf8'); assert '#define CMD_SACK 555' in interp_h and '#define CMD_CLIMB 556' in interp_h
interp=(ROOT/'src/cmd/interp.c').read_text(encoding='utf8'); assert re.search(r'^\s*//\s*CMD_N\(CMD_SACK',interp,re.M) and 'CMD_CLIMB' in interp
travel=(ROOT/'src/magic/spell_travel.c').read_text(encoding='utf8'); assert 'value[1] == cmd' in travel
quest=(ROOT/'src/world/quest.c').read_text(encoding='utf8')
ask=quest.split('int quester(',1)[1].split('if (cmd == CMD_GIVE)',1)[0]
assert 'isname(Gbuf1, qmp->key_words)' in ask and 'zone_story_quest_runtime::encountered(pl, ch);' in ask
assert 'record_completion' not in ask and 'successful_attempts' not in ask
assert 'reward_count' in quest.split('static bool capture_quest_offering_continuation(',1)[1].split('static ',1)[0]
assert not (ROOT/'areas/shp/caves_skelenak.shp').exists()
for zone in catalog['zones']:
 zon=(ROOT/f"areas/zon/{zone['source_area']}.zon").read_text(encoding='utf8')
 assert not re.search(r'^\s*[OPEG]\s+\d+\s+(?:4022|4023|4792)\s',zon,re.M)
assert not any(b['kind'] in ('Q','QA') and any(k=='I' and v in (4022,4023,4792) for k,v in b['receive']) for b in blocks)
desk=dawndale_bodies('pineholl','obj')[16072]; skulls=dawndale_bodies('elftomb','obj')[20603]
assert objvalues(desk)[0]==15 and objvalues(desk)[12:14]==[13,0] and re.search(r'\bT\s+516\s+4\s+1\s+30',desk)
assert objvalues(skulls)[0]==15 and objvalues(skulls)[12:14]==[1,0]
for area,cmd,v,cap,parent in (('pineholl','P',16071,1,16072),('elftomb','P',20604,1,20603),('swamp_two','G',26013,1,0),('myrloch_vale','E',26438,1,11)):
 assert re.search(r'^\s*'+cmd+r'\s+1\s+'+str(v)+r'\s+'+str(cap)+r'\s+'+str(parent)+r'\s+100\b',(ROOT/f'areas/zon/{area}.zon').read_text(encoding='utf8'),re.M)
assert 'qc_unblock 26555 north~' in (ROOT/'areas/qst/myrloch_vale.qst').read_text(encoding='utf8')
assert 'REMOVE_BIT(world[rroom].dir_option[dir]->exit_info, EX_BLOCKED);' in (ROOT/'src/specs/specs.exit_barriers.c').read_text(encoding='utf8')
assert 153645 not in dawndale_bodies('surface','wld') and 153645 in dawndale_bodies('Duris3','wld')
assert 'Moralon' in dawndale_bodies('northern_wilderness','wld')[12243] and 'Calim' in rooms[4067]
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==40]
assert len(units)==9 and sum(u['achievement'] for u in units)==9 and sum(u['daily_candidate'] for u in units)==9

# The Temple to Skrentherlog: exact blood, pickable chests and untracked custom outcomes.
yuan_ti=inventory_module.area_evidence(ROOT,'yuan_ti')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='yuan_ti')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==1 and len(mapping['contacts'])==8 and not mapping['exclusions']
blocks=inventory_module.native_blocks(ROOT)
raw=[b for b in blocks if b['source']=='areas/qst/yuan_ti.qst' and b['kind'] in ('Q','QA')]
assert [(b['kind'],b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[('Q',51,80517,[('I',80570)],[('I',80572)],True)]
story=mapping['stories'][0]
assert story['id']=='ranger-skrentherlog-blood' and story['category']=='story' and story['contracts']==[raw[0]['binding']]
assert len(story['steps'])==2 and story['steps'][0]['kind']=='carried_item' and story['steps'][0]['item_vnums']==[80570] and story['steps'][0]['count']==1 and story['steps'][0]['optional']
assert story['steps'][1]['kind']=='completion' and story['steps'][1]['contracts']==story['contracts']
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={80516,80517,80524,80526,80530,80534,80538,80539} and sum(len(c['topics']) for c in contacts.values())==14
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for m in yuan_ti['dialogue'] if m['giver_vnum']==v for w in m['body'][0].rstrip('~').split()]
assert contacts[80517]['topics']==['yuan-ti','yuan','shaman','cure','god','skrentherlog']
assert contacts[80534]['topics']==['die','death','stone','priest','priests','destroy','god','skrentherlog']
assert not any(contacts[v]['topics'] for v in contacts if v not in (80517,80534))
qst=(ROOT/'areas/qst/yuan_ti.qst').read_text(encoding='utf8')
assert set(map(int,re.findall(r'^#(\d+)',qst,re.M)))=={80517,80534}
assert collections.Counter(re.findall(r'^(MA|QA|M|Q)$',qst,re.M))=={'M':9,'Q':1}
assert yuan_ti['zone']['reset_mode']==0 and yuan_ti['zone']['first_vnum']==80470 and yuan_ti['zone']['last_vnum']==80599
rs=yuan_ti['reset_commands']; assert len(rs)==172
assert collections.Counter(r['command'] for r in rs)=={'D':16,'O':49,'P':20,'M':62,'E':15,'G':9,'F':1}
assert len(yuan_ti['mobs'])==41 and len(yuan_ti['items'])==82
assert {(a['kind'],a['vnum'],a['function']) for a in yuan_ti['special_assignments']}=={('obj',80556,'drowcrusher'),('obj',80579,'squelcher'),('obj',80569,'dragonarmor')}
rooms=dawndale_bodies('yuan_ti','wld');objects=dawndale_bodies('yuan_ti','obj');mobiles=dawndale_bodies('yuan_ti','mob')
assert set(rooms)==set(range(80500,80555))|{80599} and set(objects)==set(range(80500,80582)) and set(mobiles)==set(range(80500,80541))
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==104 and [(v,d,t) for v,d,f,k,t in edges if t not in rooms]==[(80554,2,80671)]
for edge in ((80542,1,3,80561,80543),(80543,3,3,80561,80542),(80544,1,7,80581,80545),(80545,3,1,0,80544),(80546,4,4,0,80547),(80500,2,0,0,80554),(80554,0,0,0,80500)): assert edge in edges
for v,d,t in ((80547,2,80548),(80548,4,80549),(80549,2,80550),(80550,2,80551),(80551,2,80552),(80552,2,80553),(80553,2,80554)): assert any(e[0]==v and e[1]==d and e[4]==t for e in edges)
assert not any(r['command']=='D' and r['arguments'][1]==80546 for r in rs)
for v,f,k in ((80532,29,80533),(80548,13,0),(80549,13,0),(80550,31,80551)):
 vals=objvalues(objects[v]); assert vals[0]==15 and vals[12:14]==[f,k]
assert objvalues(objects[80561])[0]==18 and objvalues(objects[80561])[7]==16389 and re.search(r'\bA\s+17\s+-8',objects[80561])
assert objvalues(objects[80563])[0]==13 and objvalues(objects[80563])[7]==0
current=None; gear={}; containers=[]
for r in rs:
 c,a=r['command'],r['arguments']
 if c in ('M','F'): current=(c,a)
 if c in ('E','G'): gear.setdefault((c,a[1]),[]).append((a,current))
 if c=='P': containers.append((a[1],a[3]))
assert [(a[2:5],p[0],p[1][1],p[1][3]) for a,p in gear[('G',80570)]]==[([1,0,100],'M',80538,80544)]
for item,carrier,room in ((80551,80516,80520),(80544,80530,80538),(80560,80530,80538),(80533,80524,80531),(80581,80538,80544),(55177,80538,80544)):
 assert [(a[2:5],p[0],p[1][1],p[1][3]) for a,p in gear[('G',item)]]==[([1,0,100],'M',carrier,room)]
assert (80556,80549) in containers and (80561,80550) in containers
assert rs[-1]['command']=='F' and rs[-1]['arguments'][1:5]==[80539,1,80544,100]
assert any(r['command']=='M' and r['arguments'][1:5]==[80517,1,80520,30] for r in rs)
assert any(r['command']=='O' and r['arguments'][1:5]==[359,1,80546,100] for r in rs)
assert not any(r['command']=='M' and r['arguments'][1]==80527 or r['command'] in ('O','P','E','G') and r['arguments'][1] in (80512,80557) for r in rs)
spec=(ROOT/'src/specs/specs.zalrix.c').read_text(encoding='utf8')
crusher=spec.split('int drowcrusher(',1)[1].split('int dragonarmor(',1)[0]
active=re.sub(r'/\*.*?\*/|//[^\n]*','',crusher,flags=re.S)
for token in ('cmd != CMD_HIT','OBJ_WORN(obj)','obj->loc.wearing->equipment[WIELD] != obj','VROOM_YUANTI_TREASUREROOM','FIND_OBJ_ROOM | FIND_NO_TRACKS','VOBJ_YUANTI_CRUSHERSTONE','unequip_char(ch, WIELD);','extract_obj(obj, TRUE);','extract_obj(obj2, TRUE);','while ((ch2 = world[from_room].people) != NULL)','char_from_room(ch2);','char_to_room(ch2, to_room, -1);'): assert token in active
assert 'economic_gameplay_authority' not in active and 'stop_fighting' not in active and 'zone_story_quest' not in active
handler=(ROOT/'src/world/handler.c').read_text(encoding='utf8'); assert "This deliberately does not retire the object's item_current_owner row." in handler
db=(ROOT/'src/world/db.c').read_text(encoding='utf8')
assert 'economic_gameplay_authority::active() &&\n\t\t    reset_command_issues_item(ZCMD.command)' in db
assert 'ZCMD.arg4 == 100' in db.split("case 'M': /* read a mobile */",1)[1].split("case 'O':",1)[0]
assert re.search(r'state\s*&=\s*3;',db.split('void setup_dir(',1)[1].split('void ',1)[0])
renew=db.split('void no_reset_zone_reset(',1)[1].split('static ',1)[0]
assert "SELECT reset_perc FROM zones WHERE id = '%d'" in renew and 'zone_table[zone_number].number' in renew
assert 'no_reset_zone_reset(zone);' in (ROOT/'src/world/events.c').read_text(encoding='utf8')
epic=(ROOT/'src/world/epic.c').read_text(encoding='utf8'); assert 'touch.reset_requested = touch.record_zone &&' in epic and '!zone_table[real_zone0(zone_number)].reset_mode' in epic
assert ' WHERE number=' in (ROOT/'src/world/zone_touch_repository.c').read_text(encoding='utf8')
flee=(ROOT/'src/cmd/actoff.c').read_text(encoding='utf8'); assert '#define DRAGONSLAYER_VNUM 80569' in flee and 'DRAGONSLAYER_VNUM' in flee.split('void do_flee(',1)[1]
dragon=spec.split('int dragonarmor(',1)[1].split('int squelcher(',1)[0]; assert 'RACE_DRAGONKIN' in dragon and 'SPLDAM_BREATH' in dragon and 'IS_NPC(Dragon_Mob)' not in dragon
squelcher=spec.split('int squelcher(',1)[1]; assert 'number(1, 100) < 6' in squelcher and 'spell_silence(50' in squelcher
assert not (ROOT/'areas/shp/yuan_ti.shp').exists()
foreign=[b for b in blocks if b['source']!='areas/qst/yuan_ti.qst' and b['kind'] in ('Q','QA') and ('I',55177) in b['give']]
assert [(b['source'],b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in foreign]==[('areas/qst/wh.qst',2881,55206,[('I',55177)],[('I',55362),('C',1000000),('I',55033)],False)]
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==805]
assert len(units)==1 and sum(u['achievement'] for u in units)==1 and sum(u['daily_candidate'] for u in units)==0

# Grumbar's Domain: counted planar granite, exact lash and hazardous ground access.
earthp=inventory_module.area_evidence(ROOT,'earthp')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='earthp')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==2 and len(mapping['contacts'])==6 and not mapping['exclusions']
blocks=inventory_module.native_blocks(ROOT)
raw=sorted((b for b in blocks if b['source']=='areas/qst/earthp.qst' and b['kind'] in ('Q','QA')),key=lambda b:b['line'])
assert [(b['kind'],b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[
 ('QA',65,131214,[('I',400104)]*10,[('I',131211)],True),
 ('QA',178,131236,[('I',131227)],[('I',131232),('E',200000)],True)]
assert {tuple(sorted(b.items())) for s in mapping['stories'] for b in s['contracts']}=={tuple(sorted(b['binding'].items())) for b in raw}
assert [s['id'] for s in mapping['stories']]==['sunnis-planar-granite','thulum-golden-lash']
assert all(s['category']=='story' and len(s['contracts'])==1 and s['steps'][-1]['contracts']==s['contracts'] for s in mapping['stories'])
assert [[(t['item_vnums'],t['count']) for t in s['steps'][:-1]] for s in mapping['stories']]==[[([400104],10)],[([131227],1)]]
assert all(t['kind']=='carried_item' and t['optional'] for s in mapping['stories'] for t in s['steps'][:-1])
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={131200,131214,131217,131230,131235,131236} and sum(len(c['topics']) for c in contacts.values())==28
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for m in earthp['dialogue'] if m['giver_vnum']==v for w in m['body'][0].rstrip('~').split()]
assert [len(contacts[v]['topics']) for v in (131214,131236)]==[10,18]
assert not any(contacts[v]['topics'] for v in (131200,131217,131230,131235))
qst=(ROOT/'areas/qst/earthp.qst').read_text(encoding='utf8')
assert set(map(int,re.findall(r'^#(\d+)',qst,re.M)))=={131200,131214,131217,131235,131236}
assert collections.Counter(re.findall(r'^(MA|QA|M|Q)$',qst,re.M))=={'MA':9,'QA':2,'M':7}
rs=earthp['reset_commands'];assert earthp['zone']['reset_mode']==1 and len(rs)==543
assert collections.Counter(r['command'] for r in rs)=={'D':14,'O':7,'M':160,'E':162,'F':104,'G':96}
assert len(earthp['mobs'])==37 and len(earthp['items'])==33
assert {(a['kind'],a['vnum'],a['function']) for a in earthp['special_assignments']}=={('mob',131232,'purple_worm')}
rooms=dawndale_bodies('earthp','wld');objects=dawndale_bodies('earthp','obj');mobiles=dawndale_bodies('earthp','mob')
assert set(rooms)==set(range(131200,131399)) and set(objects)==set(range(131200,131233)) and set(mobiles)==set(range(131200,131237))
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==794 and {(v,d,t) for v,d,f,k,t in edges if t not in rooms}=={(131200,4,23908)}
for edge in ((131362,5,8,0,131366),(131366,4,0,0,131362),(131366,1,3,131211,131364),(131364,3,3,131211,131366),(131396,5,0,0,131379)):assert edge in edges
for args in ([0,131362,5,8,100,0,0,0],[0,131366,1,2,100,0,0,0],[0,131364,3,1,100,0,0,0]):assert any(r['command']=='D' and r['arguments']==args for r in rs)
for giver,room in ((131214,131366),(131236,131376),(131230,131376),(131213,131362),(131223,131384)):
 assert any(r['command']=='M' and r['arguments'][1:5]==[giver,1,room,100] for r in rs)
for v in (131236,131230):assert not (int(mobiles[v].split('~',4)[4].split()[0])&2)
leader=current=None;shard_sources=[];gear_sources={};signatures=set()
for r in rs:
 c,a=r['command'],tuple(r['arguments'])
 if c=='M':leader=current=('M',a)
 elif c=='F':current=('F',a)
 signatures.add((c,a,current if c in ('E','G') else None,leader if c in ('F','E','G') else None))
 if c in ('E','G'):
  gear_sources.setdefault((c,a[1]),[]).append((a,current,leader))
  if c=='G' and a[1]==400104:shard_sources.append((a,current))
assert len(signatures)==337 and len({(r['command'],tuple(r['arguments'])) for r in rs})==225
assert len(shard_sources)==18 and collections.Counter(t[1][1] for a,t in shard_sources)=={131208:6,131209:6,131210:6}
assert all(a[2:5]==(18,0,100) and t[0]=='M' for a,t in shard_sources)
assert [(a[1:5],t[1][1],t[1][3]) for a,t,m in gear_sources[('E',131227)]]==[((131227,1,16,100),131230,131376)]
for v,carrier,room in ((131210,131213,131362),(131223,131223,131384)):
 assert [(a[2:5],t[1][1],t[1][3]) for a,t,m in gear_sources[('G',v)]]==[((1,0,100),carrier,room)]
assert objvalues(objects[131210])[0]==29 and objvalues(objects[131210])[7]==0 and objvalues(objects[131210])[11:15]==[270,131362,5,1]
assert re.search(r'\bT\s+261\s+4\s+1\s+32767',objects[131210])
assert objvalues(objects[131223])[0]==25 and objvalues(objects[131223])[7]==0 and objvalues(objects[131223])[11:15]==[131396,7,-1,0]
assert objvalues(objects[131211])[0]==18 and objvalues(objects[131211])[11:15]==[0,0,0,0]
assert objvalues(objects[131232])[0]==5 and 'pick' in objects[131232].split('~')[0] and re.search(r'\bA\s+14\s+60',objects[131232]) and re.search(r'\bA\s+55\s+5',objects[131232])
assert {r['arguments'][3] for r in rs if r['command']=='O' and r['arguments'][1]==434}=={131370,131371,131372,131373,131395,131396}
assert len(gear_sources[('G',193)])==7
reset=(ROOT/'src/world/db.c').read_text(encoding='utf8')
assert 'economic_gameplay_authority::active() &&\n\t\t    reset_command_issues_item(ZCMD.command)' in reset
follow=reset.split("case 'F': /* follow last mob M loaded */",1)[1].split("case 'R':",1)[0]
assert 'mob = read_mobile(ZCMD.arg1, REAL)' in follow and 'add_follower(mob, last_mob_followable);' in follow
assert 'ZCMD.arg4 == 100' in reset.split("case 'M': /* read a mobile */",1)[1].split("case 'O':",1)[0]
switch=(ROOT/'src/specs/specs.object.c').read_text(encoding='utf8').split('int item_switch(',1)[1].split('/* \'Mayhem\'',1)[0]
assert 'FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM' in switch and 'EX_BLOCKED' in switch
mining=(ROOT/'src/economy/mining.c').read_text(encoding='utf8')
assert '#define IS_MINING_PICK(obj) (isname("pick", obj->name) && obj->type == ITEM_WEAPON)' in mining
assert 'Mining is unavailable while item accounting is active.' in mining and 'Mining stopped while item accounting is active.' in mining
assert 'Salvage is unavailable while item accounting is active.' in (ROOT/'src/item/salvage.c').read_text(encoding='utf8')
assert 'Refining is unavailable while economic accounting is active.' in (ROOT/'src/economy/tradeskill.c').read_text(encoding='utf8')
trap=(ROOT/'src/combat/trap.c').read_text(encoding='utf8')
assert '#define TRAP_EFF_DOWN BIT_9' in trap and 'numdice = level / 100;' in trap and 'numsides = level % 100;' in trap and 'obj->trap_charge--;' in trap
assert re.search(r'if \(checkmovetrap\(ch, exitnumb\)\)\s*\{\s*return 0;', (ROOT/'src/cmd/actmove.c').read_text(encoding='utf8'))
assert all('\nee ' in mobiles[v] for v in (131213,131223))
assert not (ROOT/'areas/shp/earthp.shp').exists()
foreign=sorted((b for b in blocks if b['source']!='areas/qst/earthp.qst' and b['kind'] in ('Q','QA') and ('I',400104) in b['give']),key=lambda b:b['line'])
assert [(b['source'],b['line'],b['giver_vnum'],b['give'],b['receive']) for b in foreign]==[('areas/qst/alatorin.qst',583,83140,[('I',5),('I',400104),('I',400104)],[('I',83686)]),('areas/qst/alatorin.qst',2114,83150,[('I',400104)],[('I',83245)])]
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==1312]
assert len(units)==2 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==2

# Neverwind Valley: three same-kind rods, four distinct eggs and contextual letter delivery.
pyramid=inventory_module.area_evidence(ROOT,'pyramid')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='pyramid')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==3 and len(mapping['contacts'])==4 and not mapping['exclusions']
blocks=inventory_module.native_blocks(ROOT)
raw=sorted((b for b in blocks if b['source']=='areas/qst/pyramid.qst' and b['kind'] in ('Q','QA')),key=lambda b:b['line'])
assert [(b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[
 (30,20400,[('I',20400),('I',20400),('I',20400)],[('I',20418)],True),
 (47,20409,[('I',20419)],[('I',20412)],True),
 (84,20413,[('I',20403),('I',20404),('I',20405),('I',20406)],[('I',20417)],True)]
assert {tuple(sorted(b.items())) for s in mapping['stories'] for b in s['contracts']}=={tuple(sorted(b['binding'].items())) for b in raw}
assert [s['id'] for s in mapping['stories']]==['drinstan-three-kings','goar-apology-letter','maern-four-eggs']
assert all(s['category']=='story' and len(s['contracts'])==1 and s['steps'][-1]['contracts']==s['contracts'] for s in mapping['stories'])
assert [[(t['item_vnums'],t['count']) for t in s['steps'][:-1]] for s in mapping['stories']]==[[([20400],3)],[([20419],1)],[([20403],1),([20404],1),([20405],1),([20406],1)]]
assert all(t['kind']=='carried_item' and t['optional'] for s in mapping['stories'] for t in s['steps'][:-1])
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert {v:c['topics'] for v,c in contacts.items()}=={20400:['hello','yes','pyramid','language','favor','task'],20409:['father'],20410:['crying','yes'],20413:['birds','yes']}
assert sum(len(c['topics']) for c in contacts.values())==11
for v,c in contacts.items():
 assert c['name']==inventory_mobs[v]['name'] and c['keyword'] in inventory_mobs[v]['keywords']
 assert c['topics']==[w for m in pyramid['dialogue'] if m['giver_vnum']==v for w in m['body'][0].rstrip('~').split()]
assert not any(b['giver_vnum']==20410 for b in raw)
rs=pyramid['reset_commands'];assert pyramid['zone']['reset_mode']==2 and len(rs)==155
assert collections.Counter(r['command'] for r in rs)=={'D':14,'O':8,'P':8,'M':90,'E':27,'G':8}
assert len(pyramid['mobs'])==20 and len(pyramid['items'])==23 and not pyramid['special_assignments']
rooms=dawndale_bodies('pyramid','wld');objects=dawndale_bodies('pyramid','obj')
assert set(rooms)==set(range(20400,20599)) and set(objects)==set(range(20400,20423))
assert not (ROOT/'areas/shp/pyramid.shp').exists()
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==555
assert {(v,d,t) for v,d,f,k,t in edges if t not in rooms}=={(20400,2,528459),(20466,0,525659),(20466,1,526060)}
assert (20491,0,3,-2,20478) in edges and (20478,2,1,0,20491) in edges
assert 'double dunan~' in rooms[20491]
for args in ([0,20491,0,2,100,0,0,0],[0,20478,2,1,100,0,0,0]):assert any(r['command']=='D' and r['arguments']==args for r in rs)
for giver,room in ((20400,20437),(20409,20596),(20410,20489),(20413,20484),(20411,20592)):
 assert any(r['command']=='M' and r['arguments'][1:5]==[giver,1,room,100] for r in rs)
mobile_bodies=dawndale_bodies('pyramid','mob')
for v in (20409,20411):assert not (int(mobile_bodies[v].split('~',4)[4].split()[0]) & 2)
for parent,cap,room in ((20401,3,20477),(20401,3,20587),(20401,3,20590),(20420,1,20489),(20402,4,20498),(20402,4,20499),(20402,4,20500),(20402,4,20501)):
 assert any(r['command']=='O' and r['arguments'][1:5]==[parent,cap,room,100] for r in rs)
assert collections.Counter(tuple(r['arguments'][1:5]) for r in rs if r['command']=='P')=={(20400,3,20401,100):3,(20419,1,20420,100):1,(20403,1,20402,100):1,(20404,1,20402,100):1,(20405,1,20402,100):1,(20406,1,20402,100):1}
for v in (20401,20420):assert objvalues(objects[v])[0]==15 and objvalues(objects[v])[12:14]==[5,0]
assert objvalues(objects[20402])[0]==15 and objvalues(objects[20402])[12]==0
assert objvalues(objects[20418])[0]==4 and objvalues(objects[20418])[11:15]==[40,1,1,28]
assert objvalues(objects[20419])[0]==16 and '\nE\nletter~' in objects[20419]
assert all(objvalues(b)[0] not in (18,25,29) for b in objects.values())
for v in range(20403,20407):assert objects[v].split('~',1)[0]=='egg'
for source,target in ((20439,20498),(20411,20499),(20425,20500),(20453,20501)):
 assert (source,4,0,0,target) in edges and re.search(r'\bF\s+10\b',rooms[target])
for source,direction,target in ((20596,1,20598),(20598,1,20469),(20596,2,20597),(20598,2,20597),(20592,1,20593),(20593,1,20595),(20595,1,20591)):
 assert (source,direction,0,0,target) in edges
assert not any(v in (20594,20597) for v,d,f,k,t in edges)
assert int(rooms[20591].split('~',2)[2].split()[1])&131072
assert int(rooms[20509].split('~',2)[2].split()[1])&262144
assert 'North, North, East, South, East, North.' in rooms[20437]
comm=(ROOT/'src/cmd/actcomm.c').read_text(encoding='utf8')
password=comm.split('void check_magic_doors(',1)[1].split('void do_petition(',1)[0]
assert 'EXIT(ch, door)->key == -2' in password and 'isname(word, arg1)' in password
assert 'REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);' in password and 'EX_CLOSED' not in password
assert 'back->to_room == ch->in_room' in password and 'REMOVE_BIT(back->exit_info, EX_LOCKED);' in password
assert 'check_magic_doors(ch, argument + i);' in comm
info=(ROOT/'src/cmd/actinf.c').read_text(encoding='utf8')
read_note=info.split('void do_read(',1)[1].split('void do_examine(',1)[0]
assert 'at %s' in read_note and 'do_look(ch, buf, -4);' in read_note
assert 'arg2, tmp_object->ex_description' in info
reset=(ROOT/'src/world/db.c').read_text(encoding='utf8').split("case 'P': /* object to object */",1)[1].split("case 'G':",1)[0]
assert 'obj_to = get_obj_num(ZCMD.arg3);' in reset and 'obj_to_obj(obj, obj_to);' in reset
handler=(ROOT/'src/world/handler.c').read_text(encoding='utf8').split('P_obj get_obj_num(',1)[1].split('P_char get_char_room(',1)[0]
assert 'for (i = object_list; i; i = i->next)' in handler and 'i->R_num == nr' in handler
innate=(ROOT/'src/classes/innates.c').read_text(encoding='utf8')
assert 'ADD_RACIAL_INNATE(INNATE_SHIFT_PRIME, RACE_GITHZERAI, 1);' in innate
shift=innate.split('void do_shift_prime(P_char ch,',1)[1].split('void do_blast(',1)[0]
assert '20484' in shift and 'GET_RACE(ch) == RACE_GITHZERAI' in shift and 'check_innate_time(ch, INNATE_SHIFT_PRIME)' in shift and 'char_to_room(ch, r_room, -1)' in shift
foreign=[b for b in blocks if b['source']!='areas/qst/pyramid.qst' and b['kind'] in ('Q','QA') and any(k=='I' and v in objects for k,v in b['give']+b['receive'])]
assert not foreign
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==204]
assert len(units)==3 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==3

# Deep Ravine: staged access, paired foreign relics and accounting-refused paid key.
minopass=inventory_module.area_evidence(ROOT,'minopass')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='minopass')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==4 and len(mapping['contacts'])==7 and not mapping['exclusions']
blocks=inventory_module.native_blocks(ROOT)
raw=sorted((b for b in blocks if b['source']=='areas/qst/minopass.qst' and b['kind'] in ('Q','QA')),key=lambda b:b['line'])
assert [(b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[
 (38,94704,[('I',94723)],[('I',94724)],False),
 (67,94705,[('I',94715)],[('I',94716),('I',94727),('I',94727),('I',94727)],True),
 (130,94738,[('I',88807),('I',4402)],[('I',94726),('E',250000)],True),
 (174,94757,[('C',10000)],[('I',94729)],False)]
assert {tuple(sorted(b.items())) for s in mapping['stories'] for b in s['contracts']}=={tuple(sorted(b['binding'].items())) for b in raw}
assert [s['id'] for s in mapping['stories']]==['oesh-spore-truth','contractor-signet-ring','dwarf-paired-relics','nahasp-grate-key']
assert [s['category'] for s in mapping['stories']]==['story','story','story','service']
assert [[t['item_vnums'] for t in s['steps'][:-1]] for s in mapping['stories']]==[[[94723]],[[94715]],[[88807],[4402]],[]]
assert all(t['kind']=='carried_item' and t['optional'] and t['count']==1 for s in mapping['stories'] for t in s['steps'][:-1])
assert all(s['steps'][-1]['contracts']==s['contracts'] for s in mapping['stories'])
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={94704,94705,94706,94707,94738,94757,94762} and sum(len(c['topics']) for c in contacts.values())==23
for v,c in contacts.items():
 assert c['keyword'] in minopass['mobs'][v]['keywords']
 assert c['topics']==[w for m in minopass['dialogue'] if m['giver_vnum']==v for w in m['body'][0].rstrip('~').split()]
assert contacts[94704]['keyword']=='oesh' and contacts[94705]['keyword']=='contracter'
assert not contacts[94707]['topics'] and not contacts[94762]['topics']
assert 'unavailable while accounting is active' in mapping['stories'][-1]['summary']
assert 'no permanent revival' in mapping['stories'][2]['summary'].lower()
definitions={r['block']['line']:r['definition'] for r in minopass['requests']}
assert all(definitions[n]['daily_eligible'] for n in (38,67,130))
assert definitions[174]['eligible_for_zone_completion'] and not definitions[174]['daily_eligible'] and definitions[174]['daily_exclusion']=='No repeatable item offering'
rs=minopass['reset_commands'];assert minopass['zone']['reset_mode']==1 and len(rs)==347
assert collections.Counter(r['command'] for r in rs)=={'D':28,'O':129,'M':161,'G':20,'E':9}
assert len(minopass['mobs'])==63 and len(minopass['items'])==44 and not minopass['special_assignments']
rooms=dawndale_bodies('minopass','wld');objects=dawndale_bodies('minopass','obj')
assert set(rooms)==set(range(94700,94879))-{94728} and len(objects)==44
assert not (ROOT/'areas/shp/minopass.shp').exists()
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==404
for edge in ((94726,5,2,94729,94748),(94819,0,3,94724,94820),(94845,0,3,94716,94846)):
 assert edge in edges
assert {(v,d,t) for v,d,f,k,t in edges if t not in rooms}=={(94700,2,606867),(94721,0,604472),(94721,1,604873),(94818,1,832822),(94818,2,833221),(94878,2,584060)}
for item,cmd,target,direction in ((94700,270,94714,1),(94708,270,94785,5),(94709,340,94780,1),(94713,270,94840,5),(94714,270,94842,4)):
 assert objvalues(objects[item])[0]==29 and objvalues(objects[item])[11:14]==[cmd,target,direction]
assert any(r['command']=='O' and r['arguments'][1:5]==[94709,1,94790,100] for r in rs)
for item,location,target in ((94705,94771,94772),(94706,94772,94771),(94717,94820,94821),(94718,94821,94820),(94719,94846,94847),(94743,94847,94846),(94741,94791,94792),(94742,94792,94791)):
 assert objvalues(objects[item])[0]==25 and objvalues(objects[item])[11:15]==[target,7,-1,0]
 assert any(r['command']=='O' and r['arguments'][1:5]==[item,1,location,100] for r in rs)
spores=[r['arguments'][2:5] for r in rs if r['command']=='O' and r['arguments'][1]==94723]
assert collections.Counter(tuple(a) for a in spores)=={(26,94855,100):13,(26,94862,100):13}
assert any(r['command']=='O' and r['arguments'][1:5]==[94715,1,94843,100] for r in rs)
for giver,room in ((94704,94819),(94705,94843),(94738,94872),(94757,94726)):
 assert any(r['command']=='M' and r['arguments'][1:5]==[giver,1,room,100] for r in rs)
for v in (94872,94877):assert int(rooms[v].split('~',2)[2].split()[1]) & 131072
switch=(ROOT/'src/specs/specs.object.c').read_text(encoding='utf8').split('int item_switch(',1)[1].split("/* 'Mayhem'",1)[0]
assert 'obj->loc.room != in_room' in switch and 'You hear a rumbling sound in the distance.' in switch
assert switch.index('if (IS_SET(world[in_room].dir_option[door]->exit_info, EX_SECRET))')<switch.index('REMOVE_BIT(world[back].dir_option[(int)rev_dir[door]]->exit_info, EX_BLOCKED);')
action=(ROOT/'src/world/quest.c').read_text(encoding='utf8').split('bool execute_quest_routine(',1)[1].split('int binary_search(',1)[0]
assert 'act(qdata->message, FALSE, ch, 0, 0, TO_ROOM);' in action and 'quest_completion(' not in action
teleport=(ROOT/'src/magic/spell_travel.c').read_text(encoding='utf8').split('bool check_item_teleport(',1)[1].split('\n}',1)[0]
assert 'obj->value[1] != cmd' in teleport and 'teleport_to(ch, to_room, 0);' in teleport
assert 'GET_ALIGN' not in teleport and 'ALIGN_GOOD' not in teleport
related=set(objects)|{v for b in raw for k,v in b['give']+b['receive'] if k=='I'}
foreign=[b for b in blocks if b['source']!='areas/qst/minopass.qst' and b['kind'] in ('Q','QA') and any(k=='I' and v in related for k,v in b['give']+b['receive'])]
assert {(b['source'],b['giver_vnum'],b['line']) for b in foreign}=={('areas/qst/underworld.qst',4401,18),('areas/qst/menden.qst',88816,19),('areas/qst/alatorin.qst',83519,7425)}
assert all(b['binding'] not in [c for s in mapping['stories'] for c in s['contracts']] for b in foreign)
for area,giver,relic,room in (('underworld',4401,4402,4610),('menden',88816,88807,88860)):
 reset=(ROOT/('areas/zon/'+area+'.zon')).read_text(encoding='utf8')
 assert re.search(r'^M 0 '+str(giver)+r' 1 '+str(room)+r'[^\n]*\nG 1 '+str(relic)+r' 1 ',reset,re.M)
berronar=next(b for b in foreign if b['giver_vnum']==83519)
assert berronar['give']==[('I',83626)] and berronar['receive']==[('I',83626),('I',94726),('I',400234)] and berronar['disappear']
owner=next(m for m in catalog['story_mappings'] if m['source_area']=='alatorin')
assert any(berronar['binding'] in s['contracts'] for s in owner['stories'])
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==947]
assert len(units)==4 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==3

# Twin Towers: competing exact hearts, priest ANY alternatives and shared access lifecycle.
ttowers=inventory_module.area_evidence(ROOT,'ttowers')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='ttowers')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==5 and len(mapping['contacts'])==9 and not mapping['exclusions']
blocks=inventory_module.native_blocks(ROOT)
raw=sorted((b for b in blocks if b['source']=='areas/qst/ttowers.qst' and b['kind']=='Q'),key=lambda b:b['line'])
assert [(b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[
 (14,13206,[('I',13223)],[('C',30000)],False),
 (29,13207,[('I',13223)],[('C',30000)],False),
 (54,13213,[('I',13221),('I',13222)],[('C',100000)],False),
 (80,13216,[('I',13221)],[('I',13224)],False),
 (85,13216,[('I',13223)],[('I',13224)],False),
 (90,13216,[('I',13222)],[('I',13224)],False),
 (112,13229,[('I',13221),('I',13222),('I',13223)],[('I',13236),('E',100000)],True)]
assert {tuple(sorted(b.items())) for s in mapping['stories'] for b in s['contracts']}=={tuple(sorted(b['binding'].items())) for b in raw}
assert [s['id'] for s in mapping['stories']]==['blaevyna-lyena-heart','mixt-lyena-heart','lyena-two-hearts','priest-one-heart','talfyn-three-hearts']
assert [len(s['contracts']) for s in mapping['stories']]==[1,1,1,3,1]
assert [[t['item_vnums'] for t in s['steps'][:-1]] for s in mapping['stories']]==[[[13223]],[[13223]],[[13221],[13222]],[[13221,13223,13222]],[[13221],[13222],[13223]]]
for s in mapping['stories']:
 assert s['category']=='story' and s['steps'][-1]['contracts']==s['contracts']
 assert all(t['kind']=='carried_item' and t['optional'] and t['count']==1 for t in s['steps'][:-1])
assert sum(len(s['steps'])-1 for s in mapping['stories'])==8
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={13202,13206,13207,13209,13213,13214,13216,13219,13229}
assert len(ttowers['dialogue'])==9 and sum(len(c['topics']) for c in contacts.values())==17
for v,c in contacts.items():
 assert c['keyword'] in ttowers['mobs'][v]['keywords']
 assert c['topics']==[w for m in ttowers['dialogue'] if m['giver_vnum']==v for w in m['body'][0].rstrip('~').split()]
assert not contacts[13219]['topics'] and 'unavailable while accounting is active' in contacts[13219]['description']
assert 'one alternative' in mapping['stories'][3]['summary'].lower() and 'no revival' in mapping['stories'][3]['summary'].lower()
rs=ttowers['reset_commands'];assert ttowers['zone']['reset_mode']==1 and len(rs)==156
assert collections.Counter(r['command'] for r in rs)=={'D':34,'O':10,'P':1,'M':87,'E':17,'G':7}
parent=None;stock=[]
for r in rs:
 c,a=r['command'],r['arguments']
 if c=='M':parent=(a[1],a[3])
 if c in ('G','E'):stock.append((c,a[1],parent,a[2],a[4]))
for v,mob,room in [(13221,13207,13228),(13222,13206,13229),(13223,13213,13258),(13203,13207,13228)]:assert ('G',v,(mob,room),1,100) in stock
assert any(r['command']=='M' and r['arguments'][1:5]==[13229,1,13231,20] for r in rs)
assert any(r['command']=='M' and r['arguments'][1:5]==[13219,1,13277,100] for r in rs)
objects=dawndale_bodies('ttowers','obj');rooms=dawndale_bodies('ttowers','wld');mobs=dawndale_bodies('ttowers','mob')
assert (len(objects),len(rooms),len(mobs))==(37,99,30)
for v in (13221,13222,13223):assert objvalues(objects[v])[0]==13 and objvalues(objects[v])[7]==(1|16384)
for v,room,direction,command in [(13200,13201,0,270),(13202,13220,2,270),(13204,13231,2,270),(13205,13242,0,270),(13207,13252,3,270),(13208,13248,2,340),(13209,13257,3,270)]:
 assert objvalues(objects[v])[0]==29 and objvalues(objects[v])[11:14]==[command,room,direction]
 assert any(r['command']=='O' and r['arguments'][1:5]==[v,1,room,100] for r in rs)
assert objvalues(objects[13228])[0]==13 and objvalues(objects[13206])[0]==15
assert any(r['command']=='P' and r['arguments'][1:5]==[13213,1,13206,100] for r in rs)
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==211 and (13231,2,9,0,13237) in edges and (13230,2,2,13203,13231) in edges
assert {(v,d,t) for v,d,f,k,t in edges if t not in rooms}=={(13200,2,13570),(13230,0,13571),(13259,1,13569),(13297,3,30167)}
assert any(r['command']=='D' and r['arguments'][1:4]==[13231,2,1] for r in rs)
db=(ROOT/'src/world/db.c').read_text(encoding='utf8')
dreset=db.split("case 'D': /* set state of door */",1)[1].split("case '!':",1)[0]
assert 'if (ZCMD.arg3 & 0x08)' in dreset and 'EX_BLOCKED' in dreset and 'REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,\n\t\t\t\t\t\t   EX_BLOCKED)' not in dreset
assert not re.search(r'REMOVE_BIT\([^;]*EX_BLOCKED',dreset,re.S)
assert not ttowers['special_assignments']
epic=(ROOT/'src/classes/epic_skills.c').read_text(encoding='utf8')
assert re.search(r'\{\s*13219,\s*SKILL_JIN_TOUCH,\s*0,\s*100,\s*0,\s*0,\s*0\s*\}',epic)
assert re.search(r'\{\s*EPIC_REWARD_SKILL,\s*SKILL_JIN_TOUCH,\s*100,\s*75,\s*750000,\s*CLASS_MONK\s*\}',epic)
teacher=epic.split('int epic_teacher(',1)[1]
assert teacher.index('economic_gameplay_authority::active()')<teacher.index('epic_transaction_submit(pl, -epics_cost')
assert 'Epic skill purchases are unavailable while economic accounting is active.' in teacher
init=(ROOT/'src/world/epic.c').read_text(encoding='utf8').split('void epic_initialization()',1)[1].split('int stat_shops',1)[0]
assert 'mob_index[real_mobile(epic_teachers[i].vnum)].func.mob = epic_teacher;' in init
assert 'epic_initialization();' in (ROOT/'src/net/comm.c').read_text(encoding='utf8')
foreign=[b for b in blocks if b['source']!='areas/qst/ttowers.qst' and b['kind'] in ('Q','QA') and any(k=='I' and v in objects for k,v in b['give']+b['receive'])]
assert {(b['source'],b['giver_vnum'],b['line']) for b in foreign}=={('areas/qst/newhaven.qst',35286,331),('areas/qst/wh.qst',55101,916),('areas/qst/alatorin.qst',83337,5526)}
for b in foreign:
 owner=next(m for m in catalog['story_mappings'] if 'areas/qst/'+m['source_area']+'.qst'==b['source'])
 assert any(b['binding'] in s['contracts'] for s in owner['stories'])
for area,giver,item,reward in [('tower',9321,5011,9375),('suntmpl',82407,82406,82405)]:
 assert any(b['source']=='areas/qst/'+area+'.qst' and b['giver_vnum']==giver and ('I',item) in b['give'] and ('I',reward) in b['receive'] for b in blocks)
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==132]
assert len(units)==5 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==5

# Village of Refugees: exact paid-service refusal, native sources and actual access actions.
ruins=inventory_module.area_evidence(ROOT,'ruins')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='ruins')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==2 and len(mapping['contacts'])==3 and not mapping['exclusions']
raw=sorted((b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/ruins.qst' and b['kind']=='Q'),key=lambda b:b['line'])
assert [(b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[
 (16,98615,[('I',98601),('I',98602),('I',98603),('I',98604),('C',1000)],[('I',98605)],False),
 (113,98624,[('I',98642)],[('I',98643),('C',20000),('I',98644),('I',98645)],True)]
assert [s['id'] for s in mapping['stories']]==['anguinel-four-feather-ring','farmer-beholder-proof']
assert [s['category'] for s in mapping['stories']]==['service','story']
for story,b in zip(mapping['stories'],raw):
 assert story['contracts']==story['steps'][-1]['contracts']==[b['binding']]
 assert [(t['item_vnums'],t['count']) for t in story['steps'][:-1]]==[([v],1) for k,v in b['give'] if k=='I']
 assert all(t['kind']=='carried_item' and t['optional'] for t in story['steps'][:-1])
assert 'unavailable while accounting is active' in mapping['stories'][0]['summary']
assert '1,000 copper' in mapping['stories'][0]['summary'] and '10-platinum' in mapping['stories'][0]['summary']
assert len(ruins['dialogue'])==11 and sum(len(c['topics']) for c in mapping['contacts'])==17
for contact in mapping['contacts']:
 assert contact['keyword'] in ruins['mobs'][contact['mob_vnum']]['keywords']
 assert contact['topics']==[w for b in ruins['dialogue'] if b['giver_vnum']==contact['mob_vnum'] for w in b['body'][0].rstrip('~').split()]
rs=ruins['reset_commands'];assert ruins['zone']['reset_mode']==1 and len(rs)==202
assert collections.Counter(r['command'] for r in rs)=={'D':12,'O':20,'P':16,'M':108,'G':15,'E':16,'F':15}
parent=None;stock=[]
for r in rs:
 c,a=r['command'],r['arguments']
 if c in ('M','F'):parent=(c,a[1],a[3])
 if c in ('G','E'):stock.append((c,a[1],parent,a[2],a[4]))
for v,mob,room in [(98602,98603,98602),(98603,98605,98602),(98604,98606,98601),(98642,98637,98643),(98635,98637,98643),(98610,98611,98614)]:
 assert ('G',v,('M',mob,room),1,100) in stock
assert not any(v==98601 and p[1]==98604 for c,v,p,n,chance in stock)
assert any(r['command']=='P' and r['arguments'][1:5]==[98601,1,98600,100] for r in rs)
for v,room in [(98600,98604),(98607,98607),(98667,98610),(98631,98609),(98612,98615),(98626,98628)]:
 assert any(r['command']=='O' and r['arguments'][1:5]==[v,1,room,100] for r in rs)
for v,room in [(98615,98619),(98624,98663),(98616,98628),(98637,98643),(98638,98643)]:
 assert any(r['command']=='M' and r['arguments'][1:5]==[v,1,room,100] for r in rs)
assert ('E',98606,('M',98607,98610),1,100) in stock
assert ('E',98658,('F',98641,98633),5,100) in stock
objects=dawndale_bodies('ruins','obj');rooms=dawndale_bodies('ruins','wld');mobs=dawndale_bodies('ruins','mob')
assert (len(objects),len(rooms),len(mobs))==(68,67,44)
assert objvalues(objects[98601])[0]==11 and objvalues(objects[98642])[0]==12
assert objvalues(objects[98620])[0]==24 and objvalues(objects[98617])[0]==19
assert objvalues(objects[98607])[0]==objvalues(objects[98667])[0]==29
assert objvalues(objects[98607])[11:15]==[340,98607,3,0] and objvalues(objects[98667])[11:15]==[340,98610,1,0]
assert objvalues(objects[98631])[0]==25 and objvalues(objects[98631])[11:15]==[98629,7,-1,0]
assert objvalues(objects[98612])[11:15]==[150,29,98610,0]
assert all(objvalues(objects[v])[13]==98635 for v in (98632,98633,98634))
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==142 and (98607,3,9,98607,98610) in edges and (98610,1,9,98667,98607) in edges
assert {(v,d,t) for v,d,f,k,t in edges if t not in rooms}=={(98600,0,627239),(98600,1,627640)}
assert not ruins['special_assignments']
quest=(ROOT/'src/world/quest.c').read_text(encoding='utf8')
durable=quest.split('static bool submit_durable_quest_offering(',1)[1].split('\nvoid tell_quest(',1)[0]
assert 'goal->goal_type != QUEST_GOAL_ITEM' in durable and 'supported = false;' in durable
assert 'This quest cannot accept that durable item safely.' in durable
quester=quest.split('\nint quester(',1)[1].split('\nint quest_sort_comp',1)[0]
assert 'economic_gameplay_authority::active()' in quester and 'This quest cannot accept offerings right now.' in quester
assert quester.index('economic_gameplay_authority::active()')<quester.index('do_give(pl, arg, -4)')
assert '10 &+WPlatinum' in '\n'.join(ruins['dialogue'][0]['body'])
assert 'p = amount / 1000;' in (ROOT/'src/core/utility.c').read_text(encoding='utf8')
foreign=next(b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/newhaven.qst' and b['giver_vnum']==35286 and b['kind']=='Q')
assert foreign['line']==331 and foreign['give']==[('I',98606),('I',13221),('C',100000)] and foreign['receive']==[('I',35224)] and not foreign['disappear']
nh=next(m for m in catalog['story_mappings'] if m['source_area']=='newhaven');assert nh['revision']==2
collar=next(s for s in nh['stories'] if s['id']=='vulgaris-veldian-collar');assert collar['category']=='service' and collar['contracts']==[foreign['binding']]
assert 'PULL TABLE in the ruined shack' in collar['steps'][0]['hint'] and 'starts inside the alcove' not in collar['steps'][0]['hint']
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==986]
assert len(units)==2 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==1
assert not any(d['daily_eligible'] for d in catalog['definitions'] if d['giver_vnum']==98615)

# Woodseer: independent exact returns, declared sources, services and foreign receipt ownership.
woodseer=inventory_module.area_evidence(ROOT,'woodseer')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='woodseer')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,2,'complete')
assert len(mapping['stories'])==len(mapping['contacts'])==7 and not mapping['exclusions']
raw=sorted((b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/woodseer.qst' and b['kind']=='Q'),key=lambda b:b['line'])
assert [(b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[
 (20,16530,[('I',16526)],[('C',1000)],True),
 (45,16599,[('I',16545)],[('C',500)],False),
 (68,16614,[('I',16561)],[('C',500)],False),
 (81,16625,[('I',16546)],[('I',16576)],True),
 (100,16632,[('I',16547)],[('I',16579)],False),
 (122,16637,[('I',16544)],[('C',500)],False),
 (140,16688,[('I',16543)],[('I',16610),('C',500)],False)]
assert [s['id'] for s in mapping['stories']]==['request-16530-319885a24a7b','request-16599-5ebb15ae5a54','request-16614-89a5ffaadffe','request-16625-06a52409e119','request-16632-f03bbd8043ee','request-16637-cf5f291bd407','request-16688-dcf40d71c0ed']
for story,b in zip(mapping['stories'],raw):
 assert story['category']=='story' and story['contracts']==story['steps'][-1]['contracts']==[b['binding']]
 assert len(story['steps'])==2 and story['steps'][0]['kind']=='carried_item' and story['steps'][0]['optional']
 assert story['steps'][0]['item_vnums']==[b['give'][0][1]] and story['steps'][0]['count']==1
 assert [s['id'] for s in story['steps']]==['item-1','turn-in']
assert len(woodseer['dialogue'])==11 and sum(len(c['topics']) for c in mapping['contacts'])==22
for contact in mapping['contacts']:
 assert contact['topics']==[w for b in woodseer['dialogue'] if b['giver_vnum']==contact['mob_vnum'] for w in b['body'][0].rstrip('~').split()]
assert woodseer['zone']['reset_mode']==2 and all(d['repeatable'] and not d['prerequisites'] for d in catalog['definitions'] if d['zone_number']==165)
rs=woodseer['reset_commands'];assert len(rs)==1186 and collections.Counter(r['command'] for r in rs)=={'D':46,'O':90,'P':6,'M':608,'E':272,'G':164}
parent=None;stock=[]
for r in rs:
 c,a=r['command'],r['arguments']
 if c in ('M','F'):parent=(c,a[1],a[3])
 if c in ('G','E'):stock.append((c,a[1],parent,a[2],a[4]))
for mob,room in [(16530,16658),(16599,16799),(16614,16734),(16625,16686),(16632,16680),(16637,16665),(16688,16556),(16553,16633)]:
 assert any(r['command']=='M' and r['arguments'][1:5]==[mob,1,room,100] for r in rs)
assert any(r['command']=='O' and r['arguments'][1:5]==[16526,1,16587,100] for r in rs)
assert any(r['command']=='O' and r['arguments'][1:5]==[16547,4,16586,100] for r in rs)
for item,mob,cap,rooms in [(16545,16581,4,[16584,16584,16585,16585]),(16546,16582,1,[16585]),(16544,16572,8,[16562,16563,16570,16574,16627,16631,16643,16656]),(16561,16615,6,[16539,16850,16854,16857,16859,16861]),(16543,16571,3,[16563,16847])]:
 actual=[p[2] for cmd,v,p,n,chance in stock if cmd=='G' and v==item and p[1]==mob and n==cap and chance==100]
 assert sorted(actual)==sorted(rooms),(item,actual)
assert not any(v==16545 and p[1]==16580 or v==16543 and p[1]==16570 for cmd,v,p,n,chance in stock)
objects=dawndale_bodies('woodseer','obj');rooms=dawndale_bodies('woodseer','wld');mobs=dawndale_bodies('woodseer','mob')
assert (len(objects),len(rooms),len(mobs))==(215,453,249)
assert objvalues(objects[16526])[0]==13 and objvalues(objects[16649])[0]==32
assert all(objvalues(objects[v])[0]==19 for v in (16543,16545,16610,16635))
assert objvalues(objects[16547])[0]==13 and objvalues(objects[16904])[0]==15 and objvalues(objects[16904])[7]==0
assert 'coins' in '\n'.join(raw[3]['body']) and raw[3]['receive']==[('I',16576)]
assert 'DeLoran' in objects[16600] and not any(('I',16600) in b['give'] for b in raw)
assert ('E',16600,('M',16663,16618),1,100) in stock
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==1041 and (16570,5,5,-1,16587) in edges and (16500,4,0,-1,154581) in edges
assert (16915,4,0,0,16501) in edges
guard_births=[r['arguments'][3] for r in rs if r['command']=='M' and r['arguments'][1]==16501]
assert len(guard_births)==17 and 16501 not in guard_births
assert objvalues(dawndale_bodies('tower','obj')[9316])[0]==5
assert not any(objvalues(b)[0]==25 for b in objects.values())
registered_objects={v for z in catalog['zones'] for v in dawndale_bodies(z['source_area'],'obj')}
registered_rooms={v for z in catalog['zones'] for v in dawndale_bodies(z['source_area'],'wld')}
assert not {6070,6109,6110}&registered_objects and 154581 not in registered_rooms
assert 154581 in dawndale_bodies('Duris3','wld')
shops=(ROOT/'areas/shp/woodseer.shp').read_text(encoding='utf8')
assert len(re.findall(r'^#\d+~',shops,re.M))==20 and all(str(v) in shops for v in (6070,6109,6110,16649,16610,16635))
foreign=next(b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/surfacemini.qst' and b['giver_vnum']==97909 and b['kind']=='QA')
assert foreign['line']==302 and foreign['give']==[('I',16635),('I',3003)] and foreign['receive']==[('E',15000),('C',16000)] and foreign['disappear']
assert all(b['binding']!=foreign['binding'] for b in raw)
assert any(foreign['binding'] in s['contracts'] for m in catalog['story_mappings'] if m['source_area']=='surfacemini' for s in m['stories'])
assign=(ROOT/'src/specs/specs.assign.c').read_text(encoding='utf8');assert assign.count('mob_index[real_mobile0(16553)].func.mob = world_quest;')==5
artifact=(ROOT/'src/specs/specs.artifacts.c').read_text(encoding='utf8').split('int artifact_invisible(',1)[1].split('\nint ',1)[0];assert 'OBJ_WORN(obj)' in artifact
guards=(ROOT/'src/specs/specs.guards.c').read_text(encoding='utf8').split('int guild_guard(',1)[1].split('int guardian(',1)[0];assert 'case 16501:' in guards and '9316' in guards and 'GET_BIRTHPLACE(ch)' in guards
loader=(ROOT/'src/world/db.c').read_text(encoding='utf8');assert "zone_table[zone].cmd[comm].command = '!'" in loader and 'world[room].dir_option[door] = NULL;' in loader
events=(ROOT/'src/world/events.c').read_text(encoding='utf8');assert 'zone_table[zone].reset_mode == 2 || ::is_empty(zone)' in events
shop=(ROOT/'src/economy/shop.c').read_text(encoding='utf8');assert 'SHOP_FUNC(shop) = mob_index[keeper].func.mob;' in shop and 'real_object(temp)' in shop
service=(ROOT/'src/specs/specs.room.c').read_text(encoding='utf8');assert 'Pet purchases are unavailable while active accounting is enabled.' in service and 'Pet rentals are unavailable while active accounting is enabled.' in service
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==165];assert len(units)==7 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==7

# The Forgotten Mansion: exact independent requests, shared spoken access and foreign ownership.
mansion=inventory_module.area_evidence(ROOT,'mansion')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='mansion')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==3 and len(mapping['contacts'])==4 and not mapping['exclusions']
raw=sorted((b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/mansion.qst' and b['kind'] in ('Q','QA')),key=lambda b:b['line'])
assert [(b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[
 (48,3505,[('I',3505)],[('I',3512),('I',3511)],False),
 (70,3506,[('I',3509)],[('I',3513)],False),
 (126,3518,[('I',3529),('I',3539)],[('I',3540),('E',250000)],False)]
for story,b in zip(mapping['stories'],raw):
 assert story['category']=='story' and story['contracts']==story['steps'][-1]['contracts']==[b['binding']]
 assert [(s['item_vnums'][0],s['count']) for s in story['steps'][:-1]]==list(collections.Counter(v for k,v in b['give'] if k=='I').items())
 assert all(s['kind']=='carried_item' and s['optional'] for s in story['steps'][:-1])
assert sum(len(c['topics']) for c in mapping['contacts'])==25 and len(mansion['dialogue'])==12
for contact in mapping['contacts']:
 assert contact['topics']==[word for b in mansion['dialogue'] if b['giver_vnum']==contact['mob_vnum'] for word in b['body'][0].rstrip('~').split()]
assert mansion['zone']['reset_mode']==1 and all(d['repeatable'] for d in catalog['definitions'] if d['zone_number']==35)
rs=mansion['reset_commands'];assert len(rs)==247 and collections.Counter(r['command'] for r in rs)=={'D':38,'O':10,'P':4,'M':147,'E':36,'G':12}
parent=None;stock=[]
for r in rs:
 c,a=r['command'],r['arguments']
 if c in ('M','F'):parent=(c,a[1],a[3])
 if c in ('G','E'):stock.append((c,a[1],parent,a[2],a[4]))
for v,mob,room in [(3529,3512,3662),(3521,3512,3662),(3539,3524,3669),(67203,3524,3669),(3547,3515,3593),(3548,3539,3673),(3549,3538,3674),(3551,3533,3541),(55415,3541,3675)]:
 assert ('G',v,('M',mob,room),1,100) in stock
for item,container,room in [(3502,3501,3508),(3505,3504,3538),(3509,3510,3618),(3520,3519,3659)]:
 assert any(r['command']=='O' and r['arguments']==[0,container,1,room,100,0,0,0] for r in rs)
 assert any(r['command']=='P' and r['arguments']==[1,item,1,container,100,0,0,0] for r in rs)
for mob,room in [(3504,3502),(3505,3520),(3506,3576),(3518,3664)]:
 assert any(r['command']=='M' and r['arguments']==[0,mob,1,room,100,0,0,0] for r in rs)
assert any(r['command']=='O' and r['arguments']==[0,359,1,3675,100,0,0,0] for r in rs)
objects=dawndale_bodies('mansion','obj');rooms=dawndale_bodies('mansion','wld');mobs=dawndale_bodies('mansion','mob')
assert (len(objects),min(objects),max(objects),len(rooms),min(rooms),max(rooms),len(mobs))==(54,3500,3553,176,3500,3675,42)
assert objvalues(objects[3504])[0]==15 and objvalues(objects[3504])[11:15]==[100,5,0,100]
assert objvalues(objects[3501])[0]==15 and objvalues(objects[3501])[11:15]==[50,5,0,0]
assert objvalues(objects[3510])[0]==15 and objvalues(objects[3510])[11:15]==[50,0,0,50]
assert objvalues(objects[3529])[0]==objvalues(objects[3539])[0]==8
assert all(objvalues(objects[v])[12]==100 for v in (3502,3520,3521,3540,3547,3548,3549))
assert 'Rise of the' in objects[3505] and 'Phoenix' in objects[3505] and 'bishop Gaultair' in objects[3511]
assert 'wristwear' in objects[3513] and objvalues(objects[3513])[7]&4096
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==390 and [x for x in edges if x[-1] not in rooms]==[(3500,3,0,0,588620)]
assert [x for x in edges if x[3]==-2]==[(3515,1,7,-2,3540),(3540,3,7,-2,3515)]
assert 'wall stone door englehardt' in rooms[3515] and 'wall stone door englehardt' in rooms[3540]
assert all(any(r['command']=='D' and r['arguments'][1:4]==[v,d,6] for r in rs) for v,d in [(3515,1),(3540,3)])
assert (3660,6,7,3521,3665) in edges and (3664,2,3,3540,3671) in edges
assert (3673,2,3,3548,3674) in edges and (3674,2,3,3549,3675) in edges
assert 'crying' in rooms[3576] and 'the crying stops' in '\n'.join(raw[1]['body'])
assert not any(objvalues(body)[0]==25 for body in objects.values()) and not (ROOT/'areas/shp/mansion.shp').exists()
whrooms=dawndale_bodies('wh','wld');whobjects=dawndale_bodies('wh','obj')
assert re.search(r'D6\s+~\s*~\s*0\s+0\s+3675',whrooms[55634])
memory=next(b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/wh.qst' and b['giver_vnum']==55135 and b['kind']=='Q')
assert memory['give']==[('I',55415)] and memory['receive']==[('I',55362),('C',1000000),('I',55033)]
assert all(b['binding']!=memory['binding'] for b in raw)
assert objvalues(whobjects[55278])[0]==13 and objvalues(whobjects[55326])[11:14]==[1000,29,55327]
say=(ROOT/'src/cmd/actcomm.c').read_text(encoding='utf8');magic=say.split('void check_magic_doors(',1)[1].split('void do_petition(',1)[0]
assert 'EXIT(ch, door)->key == -2' in magic and 'isname(word, arg1)' in magic
assert 'REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED)' in magic and 'EX_SECRET' in magic and 'rev_dir[door]' in magic
assert 'check_magic_doors(ch, argument + i);' in say
defines=(ROOT/'src/core/defines.h').read_text(encoding='utf8');assert '#define DIR_NORTHWEST 6' in defines and '#define DIR_NORTHEAST 8' in defines
assign=(ROOT/'src/specs/specs.assign.c').read_text(encoding='utf8')
assert 'obj_index[real_object0(67203)].func.obj = vapor;' in assign and 'obj_index[real_object0(359)].func.obj = epic_stone;' in assign and 'obj_index[real_object0(55362)].func.obj = attribute_scroll;' in assign
vapor=(ROOT/'src/specs/specs.unique.c').read_text(encoding='utf8').split('int vapor(',1)[1].split('int vigor_mask(',1)[0]
assert 'CMD_SAY' in vapor and 'CMD_GOTHIT' in vapor and 'ITEM_NODROP' in vapor and 'spell_globe' in vapor
scroll=(ROOT/'src/specs/specs.winterhaven.c').read_text(encoding='utf8').split('int attribute_scroll(',1)[1].split('int earring_powers(',1)[0]
assert 'isname(arg, "scroll")' in scroll and scroll.index('extract_obj(obj, TRUE)')<scroll.index('read_object(number(55352, 55360)')
epic=(ROOT/'src/world/epic.c').read_text(encoding='utf8');assert 'zone_touch_transaction_submit(touch)' in epic and 'result.recovered_claim' in epic and 'telemetry_runtime_game_encounter_complete(' in epic
random=(ROOT/'src/item/randomeq.c').read_text(encoding='utf8');assert '3524, //  8 Tyrlos' in random and 'highdrop_mobs[i]' in random.split('/* Not worth the time to calculate.',1)[1].split('*/',1)[0]
loader=(ROOT/'src/world/db.c').read_text(encoding='utf8');assert 'economic_gameplay_authority::active() &&\n\t\t    reset_command_issues_item(ZCMD.command)' in loader
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==35];assert len(units)==3 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==3

# The Battlefield: distinct roots, independent trophies and guarded custom services.
battle=inventory_module.area_evidence(ROOT,'battlefi')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='battlefi')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==7 and len(mapping['contacts'])==6 and not mapping['exclusions']
raw=sorted((b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/battlefi.qst' and b['kind'] in ('Q','QA')),key=lambda b:b['line'])
assert [(b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[
 (7,66400,[('I',66400)],[('I',66462)],False),
 (22,66418,[('I',66427),('I',66427)],[('I',66426)],False),
 (67,66428,[('I',66415),('I',66416),('I',66425),('I',66417)],[('I',66419)],True),
 (84,66451,[('I',66430)],[('I',66458)],False),
 (114,66464,[('I',66451)],[('I',66452)],False),
 (119,66464,[('I',66453)],[('I',66454)],False),
 (124,66464,[('I',66455)],[('I',66456)],False)]
for story,b in zip(mapping['stories'],raw):
 assert story['contracts']==story['steps'][-1]['contracts']==[b['binding']]
 assert [(s['item_vnums'][0],s['count']) for s in story['steps'][:-1]]==list(collections.Counter(v for k,v in b['give'] if k=='I').items())
 assert all(s['kind']=='carried_item' and s['optional'] for s in story['steps'][:-1])
assert [s['category'] for s in mapping['stories']]==['story','service','story','story','story','story','story']
assert sum(len(c['topics']) for c in mapping['contacts'])==36 and len(battle['dialogue'])==12
assert next(c for c in mapping['contacts'] if c['mob_vnum']==66410)['topics']==[]
assert battle['zone']['reset_mode']==1 and all(d['repeatable'] for d in catalog['definitions'] if d['zone_number']==664)
rs=battle['reset_commands'];assert len(rs)==260 and collections.Counter(r['command'] for r in rs)=={'D':42,'O':17,'P':1,'M':152,'F':4,'G':20,'E':24}
parent=None;stock=[]
for r in rs:
 c,a=r['command'],r['arguments']
 if c in ('M','F'):parent=(c,a[1],a[3])
 if c in ('G','E'):stock.append((c,a[1],parent,a[2],a[4]))
assert ('G',66408,('F',66403,66401),1,100) in stock and not any(x[1]==66408 and x[2][1]==66471 for x in stock)
assert [x for x in stock if x[1]==66427]==[('G',66427,('M',66431,v),2,100) for v in (66407,66408)]
assert ('G',66400,('M',66401,66496),1,100) in stock
assert ('G',66414,('M',66427,66457),1,100) in stock
assert ('E',66455,('M',66465,66497),1,100) in stock
assert ('G',66451,('M',66458,66498),1,100) in stock and ('G',66453,('M',66462,66504),1,100) in stock
assert any(r['command']=='P' and r['arguments']==[1,66416,1,66413,100,0,0,0] for r in rs)
objects=dawndale_bodies('battlefi','obj');rooms=dawndale_bodies('battlefi','wld');mobs=dawndale_bodies('battlefi','mob')
assert (len(objects),min(objects),max(objects),len(rooms),min(rooms),max(rooms),len(mobs))==(63,66400,66462,163,66400,66562,72)
assert objvalues(objects[66400])[0]==19 and objvalues(objects[66400])[11]==24
assert objvalues(objects[66413])[0]==15 and objvalues(objects[66413])[11:15]==[99,13,66414,99] and not (objvalues(objects[66413])[12]&16)
assert objvalues(objects[66408])[12]==100 and objvalues(objects[66414])[12]==0
assert objvalues(objects[66419])[0]==5 and 'flail' in objects[66419] and 'Righteous' in re.sub(r'&(?:\+[A-Za-z]|[A-Za-z0-9])','',objects[66419])
assert [(v,objvalues(objects[v])[0],objvalues(objects[v])[11:14]) for v in (66403,66404,66441,66448,66443,66444,66445,66446)]==[(66403,25,[66448,7,-1]),(66404,25,[66541,7,-1]),(66441,25,[66535,320,-1]),(66448,25,[66534,320,-1]),(66443,25,[66465,320,-1]),(66444,25,[66465,7,-1]),(66445,25,[66465,320,-1]),(66446,25,[66562,7,-1])]
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==320 and not any(x[3]==-2 for x in edges) and [x for x in edges if x[-1] not in rooms]==[(66400,2,0,0,624087)]
assert 66355 not in rooms and not (ROOT/'areas/shp/battlefi.shp').exists()
trade=(ROOT/'src/economy/tradeskill.c').read_text(encoding='utf8');assign=(ROOT/'src/specs/specs.assign.c').read_text(encoding='utf8')
smith=trade.split('int smith(',1)[1].split('void initialize_tradeskills()',1)[0]
assert '{ 66410, { 127, 128, 129, 130, 88, 89, 90, 91, -1 } }' in trade
assert smith.index('economic_gameplay_authority::active()')<smith.index('choice = atoi(arg)')<smith.index('ch->carrying')
assert 'choice > i' in smith and 'sdata->items[choice - 1]' in smith and 'SUB_MONEY(pl, price, 0) != 0' in smith
assert smith.index('grant_tradeskill_item(pl, tobj)')<smith.index('extract_obj(needed_ore[j], TRUE)')
assert 'initialize_tradeskills();' in (ROOT/'src/net/comm.c').read_text(encoding='utf8') and 'mob_index[real_mobile0(smith_array[i].vnum)].func.mob = smith;' in trade
assert 'world[real_room0(66355)].funct = undead_inn;' in assign and 'obj_index[real_object0(66419)].func.obj = righteous_blade;' in assign
loader=(ROOT/'src/world/db.c').read_text(encoding='utf8');zero=loader.split('int real_room0(',1)[1].split('int real_room(',1)[0]
assert 'return (0);' in zero and 'reset_command_issues_item(ZCMD.command)' in loader
mining=(ROOT/'src/economy/mining.c').read_text(encoding='utf8');assert 'return (ore_type * 3) + ore_size + LOWEST_ORE_VNUM;' in mining and 'Mining is unavailable while item accounting is active.' in mining
heavens=dawndale_bodies('heavens','obj');assert all(v in heavens for v in (194,220,221,223,224,226,231,232,1255))
vulm=next(b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/verspin.qst' and b['line']==120)
assert vulm['give']==[('I',28146)] and vulm['receive']==[('I',223),('I',224),('I',225),('E',50000)]
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==664];assert len(units)==7 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==6

# The Great Realm of Duris: quantity, one retiring choice and custom boundaries.
realm=inventory_module.area_evidence(ROOT,'connectorzones')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='connectorzones')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==6 and len(mapping['contacts'])==8 and not mapping['exclusions']
raw=sorted((b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/connectorzones.qst' and b['kind'] in ('Q','QA')),key=lambda b:b['line'])
assert [(b['line'],b['kind'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[
 (9,'Q',53637,[('I',53625)],[('C',20000)],True),
 (31,'Q',53640,[('I',53622)]*4+[('C',10000)],[('I',53623)],False),
 (52,'Q',53658,[('I',32490),('I',26614),('I',402)],[('I',410)],False),
 (72,'Q',53700,[('I',53643),('I',53644)],[('I',53646)],True),
 (171,'Q',53704,[('I',16210),('I',16269),('I',16224),('I',16249),('I',16232)],[('I',53650),('I',53660)],True),
 (188,'Q',53704,[('I',16210),('I',16269),('I',16248),('I',16249),('I',16234)],[('I',53652),('I',53660)],True),
 (205,'Q',53704,[('I',16210),('I',16269),('I',16244),('I',16235),('I',16224)],[('I',53651),('I',53660)],True),
 (241,'Q',53720,[('I',330),('I',332)],[('I',53667),('I',53667)],False)]
groups=((0,),(1,),(2,),(3,),(4,5,6),(7,))
for story,indices in zip(mapping['stories'],groups):
 assert story['contracts']==story['steps'][-1]['contracts']==[raw[i]['binding'] for i in indices]
 quantities=collections.Counter()
 for i in indices:
  q=collections.Counter(item for kind,item in raw[i]['give'] if kind=='I')
  for v,n in q.items():quantities[v]=max(quantities[v],n)
 assert [(t['item_vnums'][0],t['count']) for t in story['steps'][:-1]]==list(quantities.items())
 assert all(t['kind']=='carried_item' and t['optional'] for t in story['steps'][:-1])
assert sum(len(c['topics']) for c in mapping['contacts'])==28 and len(realm['dialogue'])==12
assert next(c for c in mapping['contacts'] if c['mob_vnum']==53658)['topics']==[]
assert next(c for c in mapping['contacts'] if c['mob_vnum']==53670)['topics']==['quest','map','abandon','resign']
rs=realm['reset_commands'];assert len(rs)==492 and collections.Counter(r['command'] for r in rs)=={'D':88,'O':23,'P':4,'M':331,'E':23,'G':19,'F':4}
parent=None;stock=[]
for r in rs:
 a=r['arguments'];c=r['command']
 if c in ('M','F'):parent=(a[1],a[3])
 if c in ('G','E'):stock.append((c,a[1],parent,a[2],a[4]))
assert [r for r in stock if r[1]==53622]==[('G',53622,(53639,v),4,100) for v in (53919,53922,53928,53930)]
assert ('G',53644,(53695,54140),1,100) in stock and ('G',53643,(53692,54162),1,100) in stock
assert any(r['command']=='P' and r['arguments']==[1,53625,1,53624,100,0,0,0] for r in rs)
objects=dawndale_bodies('connectorzones','obj');rooms=dawndale_bodies('connectorzones','wld');mobiles=dawndale_bodies('connectorzones','mob')
assert (len(objects),min(objects),max(objects))==(70,53600,53669) and (len(rooms),min(rooms),max(rooms))==(646,53600,54245) and len(mobiles)==121
assert objvalues(objects[53646])[0]==8 and objvalues(objects[53664])[0]==3 and objvalues(objects[53664])[11:15]==[53,6,6,417]
assert objvalues(objects[53667])[11:17]==[35,8,8,0,0,3] and objvalues(objects[53668])[11:17]==[35,8,8,0,2,2]
assert [(v,objvalues(objects[v])[0],objvalues(objects[v])[11:14]) for v in (53635,53636,53655,53656,53665,53666,53669)]==[(53635,25,[54086,7,-1]),(53636,25,[54084,7,-1]),(53655,25,[54218,7,-1]),(53656,25,[54217,7,-1]),(53665,25,[54240,7,-1]),(53666,25,[559633,7,-1]),(53669,25,[43268,7,-1])]
assert {v:objvalues(objects[v])[12] for v in (53608,53618,53647,53648,53649)}=={53608:0,53618:100,53647:100,53648:0,53649:100}
assert [(v,objvalues(objects[v])[11:15]) for v in (53612,53613,53629,53630,53653,53654)]==[(53612,[270,53717,0,1]),(53613,[270,53718,2,1]),(53629,[270,53996,1,1]),(53630,[270,53997,3,1]),(53653,[270,54193,5,0]),(53654,[270,54194,5,1])]
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==1406 and len([edge for edge in edges if edge[-1] not in rooms])==105
assert not (ROOT/'areas/shp/connectorzones.shp').exists()
foreign=[b for b in inventory_module.native_blocks(ROOT) if b['kind'] in ('Q','QA')]
finishing=[b for b in foreign if b['giver_vnum']==97907 and any(k=='I' and v in (53646,53650,53651,53652) for k,v in b['give'])]
assert len(finishing)==4 and all(collections.Counter(b['give'])[('I',400280)]==3 and ('I',97921) in b['give'] for b in finishing)
mystic=next(b for b in foreign if b['source']=='areas/qst/surface.qst' and b['line']==71)
assert mystic['give']==[('I',500028),('I',500029),('I',500030),('I',500031),('I',500032)] and mystic['receive']==[('I',500033),('I',500034)]
assert objvalues(dawndale_bodies('surface','obj')[500034])[0]==13
assert 'crab' in dawndale_bodies('heavens','obj')[330] and 'shrimp' in dawndale_bodies('heavens','obj')[332]
assert objvalues(dawndale_bodies('heavens','obj')[410])[0]==13
epic=(ROOT/'src/classes/epic_skills.c').read_text(encoding='utf8');boot=(ROOT/'src/world/epic.c').read_text(encoding='utf8');comm=(ROOT/'src/net/comm.c').read_text(encoding='utf8')
assert '{ 53658, SKILL_EPIC_WISDOM' in epic and 'mob_index[real_mobile(epic_teachers[i].vnum)].func.mob = epic_teacher;' in boot and 'epic_initialization();' in comm
policy=(ROOT/'src/world/world_quest_policy.c').read_text(encoding='utf8');assert re.search(r'WORLD_QUEST_EXPLICIT_DENY_ZONES\[\]\s*=\s*\{[^}]*\b536\b',policy,re.S)
proc=(ROOT/'src/specs/specs.world_quest.c').read_text(encoding='utf8');utility=(ROOT/'src/core/utility.c').read_text(encoding='utf8')
assert 'world_quest_refund_payment' in proc and 'ADD_MONEY' in proc and 'economic_gameplay_authority::active()' in utility
missions=(ROOT/'src/world/world_quest.c').read_text(encoding='utf8');ask=missions.split('void quest_ask(',1)[1].split('void quest_kill(',1)[0]
assert 'quest_mob_vnum' in ask and 'FIND_AND_ASK' in ask and 'quest_full_reward' in ask and 'message' not in ask.split('wizlog',1)[0]
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==536]
assert len(units)==6 and sum(u['achievement'] for u in units)==sum(u['daily_candidate'] for u in units)==4
assert realm['zone']['reset_mode']==2 and all(d['repeatable'] for d in catalog['definitions'] if d['zone_number']==536)

# Tribal Oasis: independent services, exact proof bundle and carried rescue object.
oasis=inventory_module.area_evidence(ROOT,'oasis')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='oasis')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==9 and len(mapping['contacts'])==9 and not mapping['exclusions']
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/oasis.qst' and b['kind'] in ('Q','QA')]
assert [(b['line'],b['kind'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[
 (11,'Q',78006,[('I',32490),('I',26614),('I',402)],[('I',407)],False),
 (62,'QA',78010,[('I',78030),('I',78063)],[('I',78066),('I',78067)],False),
 (103,'Q',78024,[('C',100000)],[('I',78007)],False),
 (126,'Q',78030,[('I',78015)],[('I',78016)],False),
 (132,'Q',78030,[('I',78019),('I',78041),('I',78018)],[('I',78040)],False),
 (141,'Q',78030,[('I',78017),('I',78018)],[('I',78042)],False),
 (147,'Q',78030,[('I',78041),('I',78019),('I',78017)],[('I',78043)],False),
 (156,'Q',78030,[('I',78044),('I',78017)],[('I',78045)],False),
 (192,'Q',78058,[('I',78057)],[('I',78068)],False)]
for story,b in zip(mapping['stories'],raw):
 assert story['contracts']==story['steps'][-1]['contracts']==[b['binding']]
 assert story['category']==('story' if b['line'] in (11,62,192) else 'service')
 assert [t['item_vnums'] for t in story['steps'][:-1]]==[[v] for k,v in b['give'] if k=='I']
 assert all(t['kind']=='carried_item' and t['optional'] and t['count']==1 and 'gift' in t['hint'] for t in story['steps'][:-1])
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert len(oasis['dialogue'])==12 and sum(len(c['topics']) for c in contacts.values())==34 and contacts[78006]['topics']==[]
for d in oasis['dialogue']:assert set(d['body'][0].rstrip('~').split())<=set(contacts[d['giver_vnum']]['topics'])
rooms=dawndale_bodies('oasis','wld');objects=dawndale_bodies('oasis','obj');mobiles=dawndale_bodies('oasis','mob')
assert set(rooms)==set(range(78000,78304)) and set(objects)==set(range(78000,78069)) and set(mobiles)==set(range(78000,78065))
assert (oasis['zone']['zone_number'],oasis['zone']['first_vnum'],oasis['zone']['last_vnum'],oasis['zone']['reset_mode'])==(780,77947,78303,1)
rs=oasis['reset_commands'];assert len(rs)==381 and collections.Counter(r['command'] for r in rs)=={'D':18,'O':32,'P':2,'M':268,'F':5,'G':23,'E':33}
for item,cap,places in ((78044,1,[78046]),(78017,2,[78047,78047]),(78018,2,[78048,78048]),(78019,2,[78048,78048]),(78015,3,[78119,78137,78161]),(78041,2,[78173,78271]),(78057,1,[78279])):
 stock=[r['arguments'] for r in rs if r['command']=='O' and r['arguments'][1]==item]
 assert sorted(x[3] for x in stock)==places and all(x[2]==cap and x[4]==100 for x in stock)
sources={};parent=None
for row in rs:
 if row['command'] in ('M','F'):parent=row['arguments']
 if row['command'] in ('G','E'):sources.setdefault(row['arguments'][1],[]).append((row['command'],parent[1],parent[3],row['arguments'][2],row['arguments'][3],row['arguments'][4]))
for item,mob,room in ((78026,78041,78169),(78030,78051,78200),(78063,78062,78303),(78006,78025,78093),(78008,78026,78096),(78009,78027,78098),(78010,78028,78100),(78011,78029,78102)):
 assert sources[item]==[('G',mob,room,1,0,100)]
assert objvalues(objects[78057])[0]==8 and objvalues(objects[78057])[7]==1 and objvalues(objects[78057])[19]==65
for v in (78006,78007,78008,78009,78010,78011):assert objvalues(objects[v])[0]==18 and objvalues(objects[v])[12]==0
assert objvalues(objects[78026])[0]==18 and objvalues(objects[78026])[12]==100
assert {v:objvalues(objects[v])[11:16] for v in (78016,78040,78042,78043,78045)}=={78016:[51,66,66,201,10],78040:[51,29,213,180,10],78042:[25,4,14,34,100],78043:[25,28,39,73,15],78045:[55,41,106,73,0]}
assert objvalues(objects[78027])[0]==8 and objvalues(objects[78027])[7]==0 and all(objvalues(b)[0]!=25 for b in objects.values())
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==685 and (78075,5,4,0,78105) in edges and (78194,1,3,78026,78195) in edges and (78295,0,3,-2,78303) in edges
assert 'tomb mortazoth' in rooms[78295] and 'tomb mortazoth' in rooms[78303] and not re.search(r'^D\d',rooms[78203],re.M)
assert [(v,t) for v,di,flags,key,t in edges if t not in rooms]==[(78000,611692)] and '0 0 78000' in dawndale_bodies('surface','wld')[611692]
states=[r['arguments'][3] for r in rs if r['command']=='D'];assert states.count(2)==16 and states.count(1)==2
assert not any(re.search(r'^[FC]\s+',b,re.M) for b in rooms.values()) and not (ROOT/'areas/shp/oasis.shp').exists()
assert {r['arguments'][1] for r in rs if r['command'] in ('O','P','G','E')}-set(objects)=={359,55180}
assert len([r for r in rs if r['command']=='F' and r['arguments']==[1,78053,3,78202,100,0,0,0]])==3
assert int(mobiles[78006].split('~',4)[4].split()[0])==2122 and not (2122&32768)
comm=(ROOT/'src/cmd/actcomm.c').read_text(encoding='utf8');magic=comm.split('void check_magic_doors(',1)[1].split('void do_',1)[0]
assert '->key == -2' in magic and 'EX_LOCKED' in magic and 'last' in magic and 'check_magic_doors(ch, argument + i);' in comm
assert 'REMOVE_BIT' in magic and 'EX_CLOSED' not in magic
epic=(ROOT/'src/classes/epic_skills.c').read_text(encoding='utf8');assign=(ROOT/'src/specs/specs.assign.c').read_text(encoding='utf8')
assert '{ 78006, SKILL_ENCHANT' in epic and '//  {EPIC_REWARD_SKILL, SKILL_ENCHANT' in epic and 'pReward == NULL' in epic
assert 'real_mobile0(46)].func.mob = epic_teacher' in assign and 'real_mobile0(78006)' not in assign
assert objvalues(dawndale_bodies('heavens','obj')[407])[0]==13 and objvalues(dawndale_bodies('heavens','obj')[407])[11:19]==[0]*8
assert not re.search(r'^[OPGE] \d+ 402 ',(ROOT/'areas/zon/heavens.zon').read_text(encoding='utf8'),re.M)
quest=(ROOT/'src/world/quest.c').read_text(encoding='utf8');assert 'const bool giving_coins = isdigit(*temparg);' in quest and 'This quest cannot accept offerings right now.' in quest
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==780]
assert len(units)==9 and sum(u['achievement'] for u in units)==3 and sum(u['daily_candidate'] for u in units)==3

# Pharr Valley: actual recipe, reward, actor retirement and runtime ship scope.
pharrvly=inventory_module.area_evidence(ROOT,'pharrvly')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='pharrvly')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==2 and len(mapping['contacts'])==2 and not mapping['exclusions']
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/pharrvly.qst' and b['kind'] in ('Q','QA')]
assert [(b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[
 (87,40201,[('I',40208),('I',40209),('I',40210)],[('E',2500)],True),
 (100,40201,[('I',40213)],[('E',4000)],False)]
assert {tuple(sorted(b.items())) for s in mapping['stories'] for b in s['contracts']}=={tuple(sorted(b['binding'].items())) for b in raw}
for story,b in zip(mapping['stories'],raw):
 assert story['category']=='story' and story['steps'][-1]['contracts']==story['contracts']==[b['binding']]
 assert [t['item_vnums'] for t in story['steps'][:-1]]==[[v] for k,v in b['give']]
 assert all(t['kind']=='carried_item' and t['optional'] and t['count']==1 and 'gift' in t['hint'] for t in story['steps'][:-1])
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={40200,40201} and sum(len(c['topics']) for c in contacts.values())==22 and len(pharrvly['dialogue'])==14
for d in pharrvly['dialogue']:assert d['kind']=='M' and set(d['body'][0].rstrip('~').split())<=set(contacts[d['giver_vnum']]['topics'])
assert contacts[40200]['keyword']=='curator' and contacts[40201]['keyword']=='farmer'
rooms=dawndale_bodies('pharrvly','wld');objects=dawndale_bodies('pharrvly','obj');mobiles=dawndale_bodies('pharrvly','mob')
assert set(rooms)==set(range(40201,40390)) and set(objects)==set(range(40201,40227)) and set(mobiles)==set(range(40200,40275))
assert (pharrvly['zone']['zone_number'],pharrvly['zone']['first_vnum'],pharrvly['zone']['last_vnum'],pharrvly['zone']['reset_mode'])==(402,40199,40389,2)
rs=pharrvly['reset_commands'];assert len(rs)==375 and collections.Counter(r['command'] for r in rs)=={'D':28,'O':187,'P':3,'M':155,'G':2}
sources={};parent=None
for row in rs:
 if row['command']=='M':parent=row['arguments']
 if row['command'] in ('G','E'):sources.setdefault(row['arguments'][1],[]).append((row['command'],parent[1],parent[3],row['arguments'][2],row['arguments'][3],row['arguments'][4]))
assert sources=={40213:[('G',40215,40381,1,0,100)],67255:[('G',40215,40381,1,0,100)]}
for v,cap,number in ((40208,10,10),(40209,11,11),(40210,5,4),(40201,4,4)):
 stock=[r['arguments'] for r in rs if r['command']=='O' and r['arguments'][1]==v]
 assert len(stock)==number and all(x[2]==cap and x[4]==100 for x in stock)
assert {(r['arguments'][1],r['arguments'][2],r['arguments'][3],r['arguments'][4]) for r in rs if r['command']=='P'}=={(40210,5,40201,100),(40203,1,40204,100),(40211,1,40202,100)}
assert {r['arguments'][3] for r in rs if r['command']=='O' and r['arguments'][1]==40210}=={40270,40276,40308,40333}
for mob,room in ((40200,40250),(40201,40281),(40215,40381)):
 assert [r['arguments'] for r in rs if r['command']=='M' and r['arguments'][1]==mob]==[[0,mob,1,room,100,0,0,0]]
assert {r['arguments'][1] for r in rs if r['command']=='M'}==set(range(40200,40220))
assert int(mobiles[40200].split('~',4)[4].split()[0])==8 and int(mobiles[40201].split('~',4)[4].split()[0])==2058
for v in (40208,40209):assert objvalues(objects[v])[0]==19 and objvalues(objects[v])[11:15]==[5,0,0,0]
assert objvalues(objects[40210])[0]==15 and objvalues(objects[40210])[11:15]==[10,5,-1,100]
assert objvalues(objects[40202])[11:15]==[200,15,40203,250] and objvalues(objects[40204])[11:15]==[10,5,-1,250]
assert objvalues(objects[40203])[0]==18 and objvalues(objects[40203])[11:15]==[8,0,0,0]
assert objvalues(objects[40211])[0]==12 and objvalues(objects[40212])[0]==17 and objvalues(objects[40213])[0]==37
assert objvalues(objects[40214])[0]==29 and objvalues(objects[40214])[11:15]==[270,40223,1,0]
assert 'parchment scroll' in raw[0]['text'] if 'text' in raw[0] else 'parchment scroll' in (ROOT/'areas/qst/pharrvly.qst').read_text(encoding='utf8')
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==408 and (40223,1,8,0,40384) in edges and (40384,3,0,0,40223) in edges
states={(r['arguments'][1],r['arguments'][2]):r['arguments'][3] for r in rs if r['command']=='D'}
assert len(states)==28 and states[(40223,1)]==8 and states[(40384,3)]==0 and list(states.values()).count(1)==26
assert {v for v,b in rooms.items() if re.search(r'^F\s+10\s*$',b,re.M)}=={40373,40374}
assert not any(re.search(r'^C\s+',b,re.M) for b in rooms.values()) and int(rooms[40382].split('~',2)[2].split()[1])&262144
assert [(v,to) for v,di,flags,key,to in edges if to not in rooms]==[(40300,2006),(40389,28500)]
assert '0 0 40300' in dawndale_bodies('valley_crushk','wld')[2006] and '0 0 40389' in dawndale_bodies('pods','wld')[28500]
assert not (ROOT/'areas/shp/pharrvly.shp').exists() and all(objvalues(b)[0]!=25 for b in objects.values())
imports={r['arguments'][1] for r in rs if r['command'] in ('O','P','G','E')} - set(objects)
assert imports=={821,822,823,826,40072,67255}
assert not [b for b in inventory_module.native_blocks(ROOT) if b['source']!='areas/qst/pharrvly.qst' and any(k=='I' and v in objects for k,v in b['give']+b['receive'])]
ship=(ROOT/'src/ships/ship_npc.c').read_text(encoding='utf8')
crew=ship.split('NPCShipCrewData npcShipCrewData[] =',1)[1].split('/*',1)[0]
assert set(map(int,re.findall(r'\b402\d{2}\b',crew)))==set(range(40215,40275))
assert 'void assign_ship_crew_funcs()' in ship and 'zone_table[world[ch->in_room].zone].number != 600 && !IS_FIGHTING(ch)' in ship
treasure=ship.split('P_obj load_treasure_chest(',1)[1].split('void apply_zone_modifier',1)[0]
assert treasure.index('if (economic_gameplay_authority::active())')<treasure.index('read_object(r_num, REAL)')
assert 'dn_ex->key = 40225;' in ship and 'up_ex->key = 40225;' in ship and 'AUTOMATONS_MOONSTONE_CORE' in ship
switch=(ROOT/'src/specs/specs.object.c').read_text(encoding='utf8').split('int item_switch(',1)[1].split('int labelas(',1)[0]
assert 'obj->type != ITEM_SWITCH || obj->value[0] != cmd' in switch and 'REMOVE_BIT(world[in_room].dir_option[door]->exit_info, EX_BLOCKED);' in switch
assert 'REMOVE_BIT(world[back].dir_option[(int)rev_dir[door]]->exit_info, EX_BLOCKED);' in switch
assert '#define CMD_PUSH 270' in (ROOT/'src/cmd/interp.h').read_text(encoding='utf8') and 'obj->type == ITEM_SWITCH && !obj_index[nr].func.obj' in (ROOT/'src/world/db.c').read_text(encoding='utf8')
quest=(ROOT/'src/world/quest.c').read_text(encoding='utf8');offering=quest.split('static bool submit_durable_quest_offering(',1)[1].split('void tell_quest(',1)[0]
assert 'for (P_obj item = actor->carrying;' in offering and 'item_movement_transaction_submit_batch(' in offering and 'item_transfer_reason::quest_turnin' in offering
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==402];assert len(units)==2 and all(u['achievement'] and u['daily_candidate'] for u in units)

# Mistywood: source methods, optional referrals and retirement differ from narrative outcomes.
mistywood=inventory_module.area_evidence(ROOT,'mistywood')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='mistywood')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==3 and len(mapping['contacts'])==5 and not mapping['exclusions']
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/mistywood.qst' and b['kind'] in ('Q','QA')]
assert [(b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[
 (32,95019,[('I',95000)],[('C',15000),('E',25000)],False),
 (58,95020,[('I',95001)],[('I',95004)],False),
 (113,95022,[('I',95003)],[('I',95002),('E',10000)],True)]
assert {tuple(sorted(b.items())) for s in mapping['stories'] for b in s['contracts']}=={tuple(sorted(b['binding'].items())) for b in raw}
for story,b in zip(mapping['stories'],raw):
 assert story['category']=='story' and story['steps'][-1]['contracts']==story['contracts']==[b['binding']]
 assert len(story['steps'])==2
 t=story['steps'][0];assert t['kind']=='carried_item' and t['optional'] and t['count']==1 and t['item_vnums']==[b['give'][0][1]] and 'gift' in t['hint']
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={95019,95020,95021,95022,95023} and sum(len(c['topics']) for c in contacts.values())==37 and len(mistywood['dialogue'])==14
for d in mistywood['dialogue']:assert d['kind']=='M' and set(d['body'][0].rstrip('~').split())<=set(contacts[d['giver_vnum']]['topics'])
assert contacts[95019]['keyword']=='riliatar' and contacts[95021]['keyword']=='daumis' and contacts[95023]['keyword']=='korred'
rooms=dawndale_bodies('mistywood','wld');objects=dawndale_bodies('mistywood','obj');mobiles=dawndale_bodies('mistywood','mob')
assert set(rooms)==set(range(95000,95203)) and set(objects)==set(range(95000,95030)) and set(mobiles)==set(range(95000,95034))
assert (mistywood['zone']['zone_number'],mistywood['zone']['first_vnum'],mistywood['zone']['last_vnum'],mistywood['zone']['reset_mode'])==(950,94993,95202,2)
rs=mistywood['reset_commands'];assert len(rs)==171 and collections.Counter(r['command'] for r in rs)=={'D':10,'O':8,'P':7,'M':122,'E':18,'G':6}
sources={};parent=None
for row in rs:
 if row['command']=='M':parent=row['arguments']
 if row['command'] in ('G','E'):sources.setdefault(row['arguments'][1],[]).append((row['command'],parent[1],parent[3],row['arguments'][2],row['arguments'][3],row['arguments'][4]))
assert sources[95000]==[('E',95005,95108,1,3,100)] and 95001 not in sources and 95003 not in sources
assert [r['arguments'] for r in rs if r['command']=='O' and r['arguments'][1]==95001]==[[0,95001,1,95075,90,0,0,0]]
assert [r['arguments'] for r in rs if r['command']=='O' and r['arguments'][1]==95003]==[[0,95003,1,95080,100,0,0,0]]
for mob,room in ((95002,95075),(95005,95108),(95019,95170),(95020,95023),(95021,95100),(95022,95059),(95023,95073),(95004,95047)):
 assert [r['arguments'] for r in rs if r['command']=='M' and r['arguments'][1]==mob]==[[0,mob,1,room,100,0,0,0]]
flags=int(mobiles[95002].split('~',4)[4].split()[0]);assert flags&2 and flags&4
flags=int(mobiles[95020].split('~',4)[4].split()[0]);assert flags==33585160 and not flags&2 and not flags&32768
for v in (95019,95021,95022,95023):assert int(mobiles[v].split('~',4)[4].split()[0])&2
club=objvalues(objects[95001]);assert club[0]==5 and club[7]==8193 and club[6]==4202496 and club[19:22]==[21,5,100]
assert objvalues(objects[95003])[0]==19 and objvalues(objects[95003])[11:19]==[5,0,0,1,0,0,0,0]
assert objvalues(objects[95022])[0]==18 and objvalues(objects[95022])[11:15]==[95022,0,0,0] and sources[95022]==[('G',95004,95047,1,0,100)]
assert objvalues(objects[95011])[0]==15 and objvalues(objects[95017])[11:15]==objvalues(objects[95021])[11:15]==[200,5,-1,250]
assert all(objvalues(b)[0]!=25 for b in objects.values()) and not (ROOT/'areas/shp/mistywood.shp').exists()
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==580
for edge in ((95044,3,3,0,95045),(95045,1,3,0,95044),(95047,4,3,95022,95048),(95048,5,3,95022,95047),(95072,1,1,1,95073),(95073,3,1,1,95072),(95101,2,1,1,95102),(95102,0,5,1,95101),(95169,1,3,0,95170),(95170,3,3,0,95169),(95045,5,0,0,30162)):assert edge in edges
states={(r['arguments'][1],r['arguments'][2]):r['arguments'][3] for r in rs if r['command']=='D'}
assert states=={(95044,3):1,(95045,1):1,(95047,4):2,(95048,5):1,(95072,1):5,(95073,3):1,(95101,2):1,(95102,0):5,(95169,1):1,(95170,3):1}
assert not any(re.search(r'^[FC]\s+',b,re.M) for b in rooms.values())
for v in (95059,95060):assert int(rooms[v].split('~',2)[2].split()[1])&262144
assert '0 0 95045' in dawndale_bodies('underworld3','wld')[30162]
assert '0 0 95048' in dawndale_bodies('pineholl','wld')[16164] and '0 0 95100' in dawndale_bodies('krimman','wld')[16486]
boundary=[x for x in edges if x[4] not in rooms];assert len(boundary)==39
missing=[x for x in boundary if x[4]>=200000];assert len(missing)==38 and len({x[4] for x in missing})==36
world_ids=set()
for zone in catalog['zones']:world_ids.update(dawndale_bodies(zone['source_area'],'wld'))
assert not any(x[4] in world_ids for x in missing)
assert not mistywood['special_assignments']
assert not [b for b in inventory_module.native_blocks(ROOT) if b['source']!='areas/qst/mistywood.qst' and any(k=='I' and v in objects for k,v in b['give']+b['receive'])]
assert all(r['arguments'][1] in objects for r in rs if r['command'] in ('O','P','G','E')) and all(r['arguments'][1] in mobiles for r in rs if r['command'] in ('M','F'))
native=(ROOT/'src/world/quest.c').read_text();db=(ROOT/'src/world/db.c').read_text();mobact=(ROOT/'src/mob/mobact.c').read_text();food=(ROOT/'src/cmd/actobj.c').read_text();maker=(ROOT/'areas/src/wld/make_wld.c').read_text()
assert 'if (!mob || !completion->disappear)' in native and 'extract_char(mob)' in native
assert 'read_object' in db and 'reset_command_issues_item(ZCMD.command)' in db and 'economic_gameplay_authority::active()' in db
assert 'to_room = real_room0(world[room].dir_option[door]->to_room)' in db and 'world[room].dir_option[door] = NULL' in db and '(world + mid)->number == virt' in db and 'fputs(buf, all_wld)' in maker
assert 'CheckEqWorthUsing(ch, received)' in mobact and 'should_teacher_move(ch)' in mobact and 'IS_SET(ch->specials.act, ACT_SCAVENGER)' in mobact
assert 'if (temp->value[3] > 0)' in food and 'hit_reg = -temp->value[3] - hit_regen(ch, TRUE)' in food and 'extract_obj(temp)' in food
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==950];assert len(units)==3 and all(u['achievement'] and u['daily_candidate'] for u in units)

# Arcaneum: exact same-name bundles, paid admission and staged source/recipient limits.
library=inventory_module.area_evidence(ROOT,'library')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='library')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==6 and len(mapping['contacts'])==11 and not mapping['exclusions']
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/library.qst' and b['kind'] in ('Q','QA')]
assert [(b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[
 (10,402000,[('C',10000)],[('I',402003)],False),
 (21,402010,[('I',402004),('I',402005),('I',402007),('I',402006),('I',402008),('I',402009)],[('E',10000)],False),
 (37,402018,[('I',402020),('I',402022),('I',402023),('I',402024),('I',402026),('I',402028),('I',402027)],[('E',100000)],False),
 (72,402027,[('I',402029)],[('C',50000)],False),
 (85,402037,[('I',v) for v in range(402041,402049)],[('I',402049)],True),
 (137,402061,[('I',v) for v in (402011,402012,402015,402016,402017,402018,402019,402050,402051,402067)],[('I',402089)],True)]
assert {tuple(sorted(b.items())) for s in mapping['stories'] for b in s['contracts']}=={tuple(sorted(b['binding'].items())) for b in raw}
for story in mapping['stories']:
 b=next(b for b in raw if story['contracts']==[b['binding']])
 assert story['category']==('service' if b['line']==10 else 'story')
 assert story['steps'][-1]['contracts']==story['contracts']
 assert [(t['item_vnums'],t['count']) for t in story['steps'][:-1]]==[([v],n) for v,n in collections.Counter(v for k,v in b['give'] if k=='I').items()]
 assert all(t['optional'] and t['kind']=='carried_item' and 'gift' in t['hint'] for t in story['steps'][:-1])
assert sum(len(s['steps'])-1 for s in mapping['stories'])==32
dream=mapping['stories'][4]
assert len({t['text'] for t in dream['steps'][:-1]})==8 and len(dream['steps'])==9
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={402000,402010,402018,402026,402027,402037,402041,402058,402059,402060,402061}
assert sum(len(c['topics']) for c in contacts.values())==31 and len(library['dialogue'])==14
for d in library['dialogue']: assert set(d['body'][0].rstrip('~').split())<=set(contacts[d['giver_vnum']]['topics'])
assert contacts[402041]['keyword']=='hermit' and contacts[402037]['keyword']=='gazdiel'
rooms=dawndale_bodies('library','wld'); objects=dawndale_bodies('library','obj'); mobiles=dawndale_bodies('library','mob')
assert set(rooms)==set(range(402000,402150)) and set(objects)==set(range(402000,402092)) and set(mobiles)==set(range(402000,402063))
assert (library['zone']['zone_number'],library['zone']['first_vnum'],library['zone']['last_vnum'],library['zone']['reset_mode'])==(4020,401000,402149,1)
rs=library['reset_commands']; assert len(rs)==366 and collections.Counter(r['command'] for r in rs)=={'D':86,'O':46,'P':12,'M':153,'G':42,'E':27}
sources={}; parent=None
for row in rs:
 if row['command']=='M': parent=row['arguments']
 if row['command'] in ('G','E'): sources.setdefault(row['arguments'][1],[]).append((row['command'],parent[1],parent[3],row['arguments'][2],row['arguments'][3],row['arguments'][4]))
for item,mob,room in ((402004,402001,402009),(402006,402002,402009),(402005,402003,402009),(402009,402004,402010),(402008,402005,402010),(402007,402006,402010),(402026,402024,402057),(402024,402023,402058),(402023,402022,402060),(402028,402025,402060),(402022,402021,402061),(402027,402020,402061),(402020,402019,402062),(402039,402036,402093)):
 assert sources[item]==[('G',mob,room,1,0,100)],(item,sources[item])
for item in (402011,402012,402015,402016,402017,402018,402019,402050,402051,402067):
 assert [r['arguments'] for r in rs if r['command']=='P' and r['arguments'][1]==item]==[[1,item,1,402010,100,0,0,0]]
assert [r['arguments'] for r in rs if r['command']=='P' and r['arguments'][1]==402029]==[[1,402029,1,402030,100,0,0,0]]
assert objvalues(objects[402030])[11:15]==[300,13,0,300] and objvalues(objects[402031])[8]==0
assert objvalues(objects[402000])[0]==17 and objvalues(objects[402000])[11:15]==[30,30,28,0]
for v,target in ((402037,402073),(402038,402072)): assert objvalues(objects[v])[0]==25 and objvalues(objects[v])[11:15]==[target,7,-1,0]
for v in (402003,402039): assert objvalues(objects[v])[0]==18 and objvalues(objects[v])[12]==100
fragments={402043:402101,402042:402102,402041:402103,402044:402104,402048:402105,402046:402106,402047:402107,402045:402108}
for item,room in fragments.items():
 values=objvalues(objects[item]); assert values[0]==8 and values[19:21]==[0,0]
 assert [r['arguments'] for r in rs if r['command']=='O' and r['arguments'][1]==item]==[[0,item,1,room,100,0,0,0]]
assert len({re.sub(r'&(?:\+[A-Za-z]|[A-Za-z0-9])','',objects[v]) for v in fragments})==1
assert [re.findall(r'&\+([A-Za-z])',objects[v].split('~')[1]) for v in range(402041,402049)]==[[c,c.upper(),c,c.upper()] for c in 'wyrmbcg']+[['L','w','L','w']]
assert [r['arguments'] for r in rs if r['command']=='M' and r['arguments'][1]==402035]==[[0,402035,1,402109,100,0,0,0]]
assert [r['arguments'] for r in rs if r['command']=='M' and r['arguments'][1]==402061]==[[0,402061,1,402148,100,0,0,0]]
for v in (402035,402061):
 flags=int(mobiles[v].split('~',4)[4].split()[0]); assert flags&4 and not flags&2 and not flags&32768
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==410
assert [(d,t) for v,d,f,k,t in edges if v==402148]==[(d,402147 if d==2 else 402149) for d in range(10)]
assert not any(v==402149 for v,d,f,k,t in edges)
assert (402147,5,0,0,402002) in edges
for v in (402147,402148,402149):
 flags=int(rooms[v].split('~',2)[2].split()[1]); assert not flags&4 and not flags&512
assert not any(v not in range(402101,402110) and t in range(402101,402110) for v,d,f,k,t in edges)
for edge in ((402007,1,3,402003,402008),(402008,3,3,0,402007),(402073,0,7,402039,402100),(402100,2,7,402039,402073),(402059,0,5,0,402062),(402115,5,4,0,402007)): assert edge in edges
door_states={(r['arguments'][1],r['arguments'][2]):r['arguments'][3] for r in rs if r['command']=='D'}
assert door_states[402007,1]==door_states[402008,3]==2 and door_states[402073,0]==door_states[402100,2]==6
assert door_states[402059,0]==1 and (402115,5) not in door_states
assert {v:int(re.search(r'^F\s+(-?\d+)',b,re.M)[1]) for v,b in rooms.items() if re.search(r'^F\s+(-?\d+)',b,re.M)}=={402051:60,402052:70,402071:50,402116:60}
assert {(v,d,f,k,t) for v,d,f,k,t in edges if t not in rooms}=={(402000,0,0,0,813342)}
assert '0 0 402000' in dawndale_bodies('underdark','wld')[813342] and not library['special_assignments']
assert not [b for b in inventory_module.native_blocks(ROOT) if b['source']!='areas/qst/library.qst' and any(k=='I' and v in objects for k,v in b['give']+b['receive'])]
mobact=(ROOT/'src/mob/mobact.c').read_text(); travel=(ROOT/'src/magic/spell_travel.c').read_text(); native=(ROOT/'src/world/quest.c').read_text(); db=(ROOT/'src/world/db.c').read_text(); hold=(ROOT/'src/cmd/actobj.c').read_text()
assert 'max = 1' in mobact and 'obj->cost * 100' in mobact and 'should_teacher_move(ch)' in mobact and '!IS_ROOM(EXIT(ch, door)->to_room, ROOM_NO_MOB)' in mobact
assert 'obj_to = get_obj_num(ZCMD.arg3)' in db and 'mob_index[nr].func.mob = teacher' in db
assert 'obj->value[1] != cmd' in travel and 'if (obj->value[2] > 0)' in travel and 'char_to_room(ch, to_room, 0)' in travel
assert 'const bool giving_coins = isdigit(*temparg)' in native and 'This quest cannot accept offerings right now.' in native
assert '!CAN_WEAR(obj_object, ITEM_HOLD)' in hold
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==4020]
assert len(units)==6 and sum(u['achievement'] for u in units)==5 and sum(u['daily_candidate'] for u in units)==5
assert not units[0]['achievement'] and not units[0]['daily_candidate']

# Yerdonia: independent retiring requests, follower-owned proofs and competing hearts.
raxthan=inventory_module.area_evidence(ROOT,'raxthan')
mapping=next(m for m in catalog['story_mappings'] if m['source_area']=='raxthan')
assert (mapping['schema_version'],mapping['revision'],mapping['coverage'])==(3,1,'complete')
assert len(mapping['stories'])==10 and len(mapping['contacts'])==9 and not mapping['exclusions']
raw=[b for b in inventory_module.native_blocks(ROOT) if b['source']=='areas/qst/raxthan.qst' and b['kind'] in ('Q','QA')]
assert [(b['line'],b['giver_vnum'],b['give'],b['receive'],b['disappear']) for b in raw]==[
 (15,42903,[('I',42926)],[('C',15000),('E',25000)],True),
 (66,42912,[('I',42931)],[('I',42930)],True),
 (78,42912,[('I',42934)],[('I',42938)],True),
 (111,42915,[('I',42957)],[('I',42955),('E',100000)],True),
 (194,42938,[('I',42939),('I',42940)],[('I',42935),('E',250000)],True),
 (242,42939,[('I',42937)],[('I',42936),('E',50000)],True),
 (284,42941,[('I',42942),('I',42949)],[('I',42941),('E',50000)],True),
 (329,42944,[('I',42940),('I',42948)],[('I',42947),('E',60000)],True),
 (371,42945,[('I',42956)],[('I',42952),('E',25000)],True),
 (398,42946,[('I',42927)]*3,[('I',42953),('E',25000)],True)]
assert {tuple(sorted(b.items())) for s in mapping['stories'] for b in s['contracts']}=={tuple(sorted(b['binding'].items())) for b in raw}
for story in mapping['stories']:
 b=next(b for b in raw if story['contracts']==[b['binding']])
 assert story['category']=='story' and story['steps'][-1]['contracts']==story['contracts']
 assert [(t['item_vnums'],t['count']) for t in story['steps'][:-1]]==[([v],n) for v,n in collections.Counter(v for k,v in b['give']).items()]
 assert all(t['optional'] and t['kind']=='carried_item' and 'gift' in t['hint'] for t in story['steps'][:-1])
 assert len(story['contracts'])==1 and len(story['steps'])==len(collections.Counter(b['give']))+1
assert sum(len(s['steps'])-1 for s in mapping['stories'])==13
contacts={c['mob_vnum']:c for c in mapping['contacts']}
assert set(contacts)=={42903,42912,42915,42938,42939,42941,42944,42945,42946} and sum(len(c['topics']) for c in contacts.values())==25
assert len(raxthan['dialogue'])==14
for d in raxthan['dialogue']:assert set(d['body'][0].rstrip('~').split())<=set(contacts[d['giver_vnum']]['topics'])
assert contacts[42944]['keyword']=='trin' and contacts[42946]['keyword']=='grobklarn'
rooms=dawndale_bodies('raxthan','wld');objects=dawndale_bodies('raxthan','obj');mobiles=dawndale_bodies('raxthan','mob')
assert set(rooms)==set(range(42900,43000)) and set(objects)==set(range(42900,42959)) and set(mobiles)==set(range(42900,42964))
assert (raxthan['zone']['zone_number'],raxthan['zone']['first_vnum'],raxthan['zone']['last_vnum'],raxthan['zone']['reset_mode'])==(429,42599,42999,1)
rs=raxthan['reset_commands'];assert len(rs)==305
assert collections.Counter(r['command'] for r in rs)=={'D':4,'O':9,'M':177,'E':57,'G':44,'P':10,'F':4}
sources={};parent=None
for row in rs:
 if row['command'] in ('M','F'):parent=row['arguments']
 if row['command'] in ('G','E'):sources.setdefault(row['arguments'][1],[]).append((row['command'],parent[1],parent[3],row['arguments'][2],row['arguments'][3],row['arguments'][4]))
for item,mob,room,kind,slot in ((42931,42900,42949,'G',0),(42940,42934,42949,'G',0),(42934,42933,42987,'G',0),(42957,42950,42992,'G',0),(42939,42902,42994,'G',0),(42937,42940,42983,'G',0),(42942,42935,42970,'G',0),(42949,42942,42997,'G',0),(42948,42943,42966,'G',0),(42956,42949,42984,'E',16)):
 assert sources[item]==[(kind,mob,room,1,slot,100)],(item,sources[item])
assert sources[42925]==[('E',42913,42924,1,27,100)]
assert [r['arguments'] for r in rs if r['command']=='P' and r['arguments'][1]==42926]==[[1,42926,9,42925,100,0,0,0]]*9
assert sorted(r['arguments'][3] for r in rs if r['command']=='O' and r['arguments'][1]==42927)==[42901,42907,42916,42929,42932,42942,42943]
assert all(r['arguments'][2:3]==[7] and r['arguments'][4]==100 for r in rs if r['command']=='O' and r['arguments'][1]==42927)
for v in (42954,42958):assert objvalues(objects[v])[0]==29 and objvalues(objects[v])[8]==0
assert objvalues(objects[42954])[11:15]==[320,42943,0,0] and objvalues(objects[42958])[11:15]==[320,42944,2,0]
assert objvalues(objects[42930])[0]==9 and 'leggings' in objects[42930]
assert 'heart noctule' in objects[42940] and 'spine darthus' in objects[42956]
assert not any(objvalues(b)[0]==25 for b in objects.values()) and not raxthan['special_assignments']
edges=[(v,int(m[1]),int(m[4]),int(m[5]),int(m[6])) for v,b in rooms.items() for m in re.finditer(r'\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)',b,re.S)]
assert len(edges)==227
for edge in ((42943,0,8,0,42944),(42944,2,1,0,42943),(42940,2,5,0,42945),(42945,0,5,0,42940)):assert edge in edges
assert {(r['arguments'][1],r['arguments'][2],r['arguments'][3]) for r in rs if r['command']=='D'}=={(42940,2,1),(42945,0,1),(42943,0,8),(42944,2,1)}
assert {v:int(re.search(r'^F\s+(-?\d+)',b,re.M)[1]) for v,b in rooms.items() if re.search(r'^F\s+(-?\d+)',b,re.M)}=={42969:69,42982:83,42986:53,42993:81,42995:9,42996:18,42997:33}
assert {(v,d,f,k,t) for v,d,f,k,t in edges if t not in rooms}=={(42980,1,0,0,595852),(42980,2,0,0,596251),(42999,2,0,0,749737)}
assert '0 0 42980' in dawndale_bodies('surface','wld')[595852] and '0 0 42980' in dawndale_bodies('surface','wld')[596251]
assert '0 0 42999' in dawndale_bodies('underdark','wld')[749737]
assert sources[42953]==[('G',42963,42912,999,0,100)] and sources[6122]==[('G',42963,42912,999,0,100)]
assert 6122 not in inventory_module.inventory(ROOT)[0]
foreign=[b for b in inventory_module.native_blocks(ROOT) if ('I',42948) in b['give'] and b['source']!='areas/qst/raxthan.qst']
assert len(foreign)==1 and (foreign[0]['line'],foreign[0]['giver_vnum'],foreign[0]['give'],foreign[0]['receive'],foreign[0]['disappear'])==(6574,83406,[('I',20253),('I',42948)],[('I',83447),('C',100000),('E',85000)],True)
db=(ROOT/'src/world/db.c').read_text();switch=(ROOT/'src/specs/specs.object.c').read_text();fall=(ROOT/'src/world/falling.c').read_text();native=(ROOT/'src/world/quest.c').read_text()
assert 'world[room_nr].chance_fall = tmp' in db and 'ZCMD.arg3 & 0x04' in db and 'ZCMD.arg3 & 0x08' in db
assert '!IS_SET(world[in_room].dir_option[door]->exit_info, EX_BLOCKED)' in switch and 'REMOVE_BIT(world[in_room].dir_option[door]->exit_info, EX_BLOCKED)' in switch
assert 'add_follower(mob, last_mob_followable)' in db and 'tmp_mob = mob' in db
assert 'const bool entered = char_to_room(ch, new_room, -2)' in fall and 'falling_climb_catches' in fall and 'AFF_LEVITATE' in fall
assert 'act(completion->disappear_message' in native and 'extract_char(mob)' in native
units=[u for u in catalog_module.story_units(catalog) if u['zone_number']==429]
assert len(units)==10 and all(u['achievement'] and u['daily_candidate'] for u in units)
assert 'retires' in mapping['stories'][1]['summary'] and 'together' in mapping['stories'][7]['summary']

for area in ("twin_towers_forest", "newbie2", "newbie", "braddistock", "breale", "elvish", "krimman", "bastine", "pineholl", "quietus", "torg", "solonar", "wh", "smokev", "caertannad", "bs", "moria", "clwcvrn", "long", "blackpearl", "ravenloft2", "barovia", "tikitt", "jade", "savannah", "alatorin", "newhaven", "realm", "verspin", "shipy", "cosmic", "surface", "tharnadia", "minizones", "torrhan", "gold_hal", "ashrumite", "hall", "sarmiz", "delwyn", "divhome", "halfcut", "scorchvalley", "court", "snogres", "airshipgrave", "juiblex", "surfacemini", "nexus", "crakkaro", "roguerai", "desolate", "rftjngle", "trnsptow", "airp", "hunt", "tribal", "lornecro", "brass", "lortower", "mushroom_caverns", "smoke", "fishermans_wharf", "nlakes", "kobold", "troll_caves", "centaur_zone", "opalphoenix", "mira", "surfacekeeps", "icecrag", "cloister", "willem", "ixarkon", "mntcastl", "tundra", "fields_between", "goblinht", "ceothia", "brad", "desert", "ceopast", "basin_wa", "crypt", "val", "harrow", "mountaintracks", "shortc", "lavcav", "nomads", "undermountain", "desolateinv", "spshold", "harpyht", "herders", "jotun", "temple", "pods", "citadel", "element", "earth", "githzer", "worms", "ravenloft", "barovia2", "werrun", "newhope", "raxthan", "library", "mistywood", "pharrvly", "oasis", "connectorzones", "battlefi", "mansion", "woodseer", "ruins", "ttowers", "minopass", "pyramid", "earthp", "yuan_ti", "caves_skelenak", "highway", "labyrinth", "khildarak", "stormht", "goblincave", "tharnadian_ruin", "clfhaven", "shabo", "firesworn_altar", "Voluntown", "negplane", "prison", "cerebusp", "kimordril", "shady", "4horse", "ixxillikor", "moonshae", "prisonb", "pworm", "vargan", "maze_are", "banditca", "myrloch_vale", "cldgt", "mount", "tiamat", "obcita", "mist_chasm", "kastle", "malch"):
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
