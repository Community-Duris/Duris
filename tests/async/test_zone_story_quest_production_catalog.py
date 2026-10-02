#!/usr/bin/env python3

import importlib.util
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
rows, _, _ = inventory_module.inventory(ROOT)
assert len(rows) == 350 and sum(bool(r["requests"]) for r in rows) == 221
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
for area in ("twin_towers_forest", "newbie2", "newbie", "braddistock", "breale", "elvish", "krimman", "bastine", "pineholl", "quietus", "torg", "solonar", "wh", "smokev", "caertannad", "bs", "moria", "clwcvrn", "long", "blackpearl"):
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
