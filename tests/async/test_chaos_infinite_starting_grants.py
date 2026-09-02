#!/usr/bin/env python3
"""Contracts for Chaos infinite starting grants and compact material pouch."""

from __future__ import annotations

import re

from _paths import ROOT, source

NANNY = source("nanny.c").read_text(encoding="utf-8", errors="replace")
NANNY_COMPACT = re.sub(r"\s+", "", NANNY)
MATERIALS = source("chaos_materials.h").read_text(encoding="utf-8", errors="replace")
MATERIALS_C_PATH = ROOT / "src/combat/chaos_materials.c"
MATERIALS_C = MATERIALS_C_PATH.read_text(encoding="utf-8", errors="replace") if MATERIALS_C_PATH.exists() else ""
VNUM_OBJECT = source("vnum.obj.h").read_text(encoding="utf-8", errors="replace")
POUCH_OBJECT = (ROOT / "areas/obj/tradeskills.obj").read_text(
    encoding="latin1", errors="replace"
)
EPIC = source("epic_skills.c").read_text(encoding="utf-8", errors="replace")
EPIC_HEADER = source("epic_skills.h").read_text(encoding="utf-8", errors="replace")
EPIC_COMMAND = source("epic_command.h").read_text(encoding="utf-8", errors="replace")
EPIC_CODEC = source("epic_command.c").read_text(encoding="utf-8", errors="replace")
CURRENCY_COMMAND = source("currency_command.h").read_text(encoding="utf-8", errors="replace")
CURRENCY_CODEC = source("currency_command.c").read_text(encoding="utf-8", errors="replace")
MOVEMENT = source("item_movement_transaction.c").read_text(encoding="utf-8", errors="replace")
MOVEMENT_HEADER = source("item_movement_transaction.h").read_text(encoding="utf-8", errors="replace")
SHIP_SHOP = source("ship_shop.c").read_text(encoding="utf-8", errors="replace")
ACHIEVEMENTS = source("achievements.c").read_text(encoding="utf-8", errors="replace")
ACHIEVEMENTS_HEADER = source("achievements.h").read_text(encoding="utf-8", errors="replace")
CONFIG = source("chaos_config.c").read_text(encoding="utf-8", errors="replace")
CONFIG_HEADER = source("chaos_config.h").read_text(encoding="utf-8", errors="replace")
CURRENCY_TRANSACTION = source("currency_transaction.c").read_text(encoding="utf-8", errors="replace")
CURRENCY_TRANSACTION_HEADER = source("currency_transaction.h").read_text(encoding="utf-8", errors="replace")
CURRENCY_COMMAND = source("currency_command.c").read_text(encoding="utf-8", errors="replace")
WS_HANDLERS = source("ws_handlers.c").read_text(encoding="utf-8", errors="replace")
GUILD = source("guild.c").read_text(encoding="utf-8", errors="replace")
ENV_EXAMPLE = (ROOT / ".env.example").read_text(encoding="utf-8", errors="replace")

switches = (
    "CHAOS_STARTER_BONUSES",
    "CHAOS_STARTER_FRIGATE",
    "CHAOS_STARTER_EPIC_SKILLS",
    "CHAOS_STARTER_EPIC_POINTS",
    "CHAOS_STARTER_BANK_PLATINUM",
    "CHAOS_STARTER_MATERIALS",
)
for switch in switches:
    assert switch in CONFIG
    assert switch in ENV_EXAMPLE
for accessor in (
    "chaos_starter_bonuses_enabled",
    "chaos_starter_frigate_enabled",
    "chaos_starter_epic_skills_enabled",
    "chaos_starter_epic_points_enabled",
    "chaos_starter_bank_platinum_enabled",
    "chaos_starter_materials_enabled",
):
    assert accessor in CONFIG_HEADER

baseline = NANNY.index("if (!writeCharacter(ch, 2, NOWHERE))")
ledgers = NANNY.index("schedule_chaos_starting_ledgers(ch);")
kit = NANNY.index("load_chaos_new_character_kit(ch);")
chaos_level = NANNY.index("advance_to_level(ch, 56);")
skill_grant = NANNY.index("grant_epic_skills_without_specialization(ch);")
tattoo_init = NANNY.index("grant_chaos_tattoo_achievement(ch);")
post_entry_save = NANNY.index("if (!writeCharacter(ch, 1, NOWHERE))")
assert baseline < kit < ledgers
assert chaos_level < skill_grant < tattoo_init < post_entry_save
assert "if (new_character && chaos_starter_epic_skills_enabled())" in NANNY
assert "if (new_character && chaos_starter_frigate_enabled())" in NANNY
assert "if (!ch || !chaos_starter_bonuses_enabled())" in NANNY

assert "epic_transaction_submit_identified(ch,operation_id,20000" in NANNY_COMPACT
assert "bank_delta.amount[3]=1000000;" in NANNY_COMPACT
assert "currency_transaction_submit_identified(ch,operation_id,{},bank_delta" in NANNY_COMPACT
assert "critical_source_site::login" in NANNY
assert "critical_deadline_class::recovery" in NANNY

assert "if (!ch || IS_NPC(ch) || IS_SPECIALIZED(ch))" in EPIC
assert "find_epic_teacher_for_skill" in EPIC
assert "reward.classes && !IS_SET(reward.classes, ch->player.m_class)" in EPIC
assert "teacher.deny_skill && GET_CHAR_SKILL(ch, teacher.deny_skill)" in EPIC
assert "teacher.pre_requisite" in EPIC
assert "ch->only.pc->skills[reward.value].learned = ceiling;" in EPIC
assert "grant_epic_skills_without_specialization" in EPIC_HEADER

assert "chaos_starter_reward" in EPIC_COMMAND
assert "reason <= epic_reason_type::chaos_starter_reward" in EPIC_CODEC
assert "chaos_starter_reward" in CURRENCY_COMMAND
assert "reason <= currency_reason_type::chaos_starter_reward" in CURRENCY_CODEC

assert "chaos_starter_frigate_enabled" in SHIP_SHOP
assert "const int tattoo_reward_hull = chaos_frigate_reward ? SH_FRIGATE : SH_SLOOP;" in SHIP_SHOP
assert "const int tattoo_discount = chaos_frigate_reward ? SHIPTYPE_COST(tattoo_reward_hull) :" in SHIP_SHOP
assert "AIP_FREESLOOP" in ACHIEVEMENTS
assert "chaos_frigate ?" in ACHIEVEMENTS
assert "A free frigate" in ACHIEVEMENTS
assert "A free sloop" in ACHIEVEMENTS

assert "VOBJ_CHAOS_CRAFT_POUCH 400300" in VNUM_OBJECT
assert POUCH_OBJECT.count("#400300\n") == 1
assert "chaos craft pouch universal materials~" in POUCH_OBJECT
assert "a compact Chaos craft pouch~" in POUCH_OBJECT
assert "15 3 3 0 7 0 0 8404993" in POUCH_OBJECT
assert "chaos_material_pouch_is" in MATERIALS
assert "chaos_material_pouch_in" in MATERIALS
assert "chaos_material_pouch_available" in MATERIALS
assert "WEAR_ATTACH_BELT_1" in MATERIALS_C
assert "WEAR_ATTACH_BELT_3" in MATERIALS_C
assert "chaos_material_pouch_contents_description" in MATERIALS
assert "VOBJ_CHAOS_CRAFT_POUCH" in NANNY
assert "REMOVE_BIT(obj->extra_flags, ITEM_TRANSIENT)" in NANNY
assert "chaos_starter_materials_enabled()" in NANNY
assert "chaos_material_pouch_available" in source("crafting.c").read_text(
    encoding="utf-8", errors="replace"
)
CRAFTING = source("crafting.c").read_text(encoding="utf-8", errors="replace")
assert "!chaos_pouch && (invLowMats < numLowest || invHighMats < numHighest)" in CRAFTING
assert "!chaos_pouch && (numLowest > 0)" in CRAFTING
assert "!chaos_pouch && (invVnum == lowQualityMaterialVnum)" in CRAFTING
assert "chaos_material_pouch_available" in source("enhance.c").read_text(
    encoding="utf-8", errors="replace"
)
ENHANCE = source("enhance.c").read_text(encoding="utf-8", errors="replace")
assert "if (!chaos_material_pouch_available(ch))" in ENHANCE
assert "chaos_material_pouch_is(source)" in ENHANCE
assert "chaos_material_pouch_is(material)" in ENHANCE
SALVAGE = source("salvage.c").read_text(encoding="utf-8", errors="replace")
assert "chaos_material_pouch_is(temp)" in SALVAGE
SALCHEMIST = source("salchemist.c").read_text(encoding="utf-8", errors="replace")
assert "chaos_material_pouch_available" in SALCHEMIST
assert "chaos_material_pouch_is(item)" in SALCHEMIST
assert "chaos_material_pouch_is(jewel)" in SALCHEMIST
assert "read_object(static_cast<int>(jewel_vnum), VIRTUAL)" in SALCHEMIST
assert "virtual_jewel" in SALCHEMIST
CHAOS = source("chaos.c").read_text(encoding="utf-8", errors="replace")
ACTINF = source("actinf.c").read_text(encoding="utf-8", errors="replace")
ACTOBJ = source("actobj.c").read_text(encoding="utf-8", errors="replace")
MAKEFILE = (ROOT / "src/Makefile").read_text(encoding="utf-8", errors="replace")
assert "chaos_material_pouch_contents_description" in ACTINF
assert "chaos_material_pouch_is(tmp_object)" in ACTINF
assert "CHAOS_POUCH_LEDGER" in MATERIALS_C
assert "chaos_material_pouch_record_generated" in MATERIALS
assert "chaos_material_pouch_record_collected" in MATERIALS
assert "chaos_material_pouch_scoreboard" in MATERIALS
assert "chaos_material_pouch_collect_inventory" in ACTOBJ
assert "WEAR_ATTACH_BELT_1" in ACTOBJ
assert "WEAR_ATTACH_BELT_3" in ACTOBJ
assert "Chaos craft pouch generated" in MATERIALS_C
assert "std::sort" in MATERIALS_C
assert "combat/chaos_materials.o" in MAKEFILE
assert "#ifdef TEST_MUD" not in CHAOS
assert "chaos_test_commands_enabled" in CHAOS
assert "boot_enhancement_system" in CHAOS
assert "pouchseed" in CHAOS
assert "pouchgenerate" in CHAOS
assert "chaos_material_pouch_record_generated" in CHAOS
assert "chaos_material_pouch_find(ch)" in ENHANCE
assert "CHAOS_RESOURCE_" not in NANNY
assert "chaos_resource_" not in NANNY
assert "PLR3_CHAOS_STARTER_PENDING" not in NANNY
assert "item_creation_grant_submit_to_player_before_entry_with_completion" not in NANNY
assert "item_creation_grant_completion_fn" not in source("item_movement_transaction.h").read_text(
    encoding="utf-8", errors="replace"
)
SNAPSHOT = source("player_snapshot.h").read_text(encoding="utf-8", errors="replace")
assert "PLAYER_SNAPSHOT_MAX_BYTES = 4 * 1024 * 1024" in SNAPSHOT
assert "PLAYER_SNAPSHOT_MAX_ROWS = 8192" in SNAPSHOT
assert "PLAYER_SNAPSHOT_MAX_OBJECTS = 4096" in SNAPSHOT
assert "ITEM_TRANSFER_MAX_ITEMS = 3000" in source("item_transfer_command.h").read_text(
    encoding="utf-8", errors="replace"
)

print("Chaos infinite starting-grant contracts passed")
