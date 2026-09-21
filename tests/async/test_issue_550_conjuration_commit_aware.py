#!/usr/bin/env python3
"""Contract checks for commit-aware conjured weapons and replacements."""

from _paths import extract_function, source


MAGIC = source("magic.c").read_text(encoding="utf-8", errors="replace")
SKILLS = source("classes/new_skills.c").read_text(encoding="utf-8", errors="replace")


def function_body(text: str, signature: str) -> str:
    start = text.index(signature)
    brace = text.index("{", start)
    depth = 0
    for index in range(brace, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start : index + 1]
    raise AssertionError(f"unterminated function: {signature}")


assert '"item/item_movement_transaction.h"' in MAGIC
assert '"item/item_movement_transaction.h"' in SKILLS

weapon_submit = function_body(MAGIC, "static bool submit_conjured_weapon(")
weapon_callback = function_body(MAGIC, "static void conjured_weapon_grant_completed(")
weapon_effect = function_body(MAGIC, "static void conjured_weapon_publish_effect(")
for signature in (
    "void spell_ensis_unguis(",
    "void spell_lancea_cineralae(",
    "void spell_simulacrum_anguis(",
):
    spell = function_body(MAGIC, signature)
    assert "submit_conjured_weapon" in spell
    assert "obj_to_char" not in spell
    assert "spell_damage" not in spell

assert "item_creation_grant_submit_to_player_with_completion" in weapon_submit
assert "conjured_weapon_grant_completed" in weapon_submit
assert weapon_submit.index("item_creation_grant_submit_to_player_with_completion") < weapon_submit.index(
    "extract_obj"
)
assert "item_uid" in weapon_callback and "actor_pid" in weapon_callback
assert "self_damage" in weapon_callback
assert "if (!committed)" in weapon_callback
assert "result.root_item_uid != context.item_uid" in weapon_callback
assert "OBJ_CARRIED_BY(blade, actor)" in weapon_callback
assert "conjured_weapon_vnum(context.kind) < 0" in weapon_callback
assert "OBJ_VNUM(blade) != conjured_weapon_vnum(context.kind)" in weapon_callback
assert weapon_callback.index("magic_find_object_by_uid") < weapon_callback.index(
    "conjured_weapon_publish_effect"
)
assert "spell_damage" in weapon_effect
assert "no health was spent" in weapon_callback

reload = function_body(MAGIC, "\nvoid load_soulbind(P_char ch)\n{")
reload_callback = function_body(MAGIC, "static void soulbind_reload_completed(")
assert "item_creation_grant_submit_to_player_with_completion" in reload
assert "soulbind_reload_completed" in reload
assert "existing soulbound item was kept" in reload
assert "result.root_item_uid != context.item_uid" in reload_callback
assert "OBJ_VNUM(replacement) != context.item_vnum" in reload_callback
assert "remove_soulbind_except" in reload_callback
assert reload_callback.index("magic_find_object_by_uid") < reload_callback.index(
    "remove_soulbind_except"
)
assert reload_callback.index("if (!committed)") < reload_callback.index(
    "remove_soulbind_except"
)

soulbind_command = function_body(MAGIC, "void do_soulbind(")
reload_block_start = soulbind_command.index(
    "if (has_soulbind(victim) != 0 && !replace_existing)"
)
reload_block_end = soulbind_command.index("// If victim doesn't have soulbind", reload_block_start)
reload_block = soulbind_command[reload_block_start:reload_block_end]
assert "remove_soulbind(victim)" not in reload_block
assert reload_block.index("load_soulbind(victim)") < reload_block.index("return")

replacement_submit = function_body(SKILLS, "static bool submit_summoned_replacement(")
replacement_callback = function_body(SKILLS, "static void summoned_replacement_completed(")
for signature in ("void event_summon_book(", "void event_summon_totem("):
    event = function_body(SKILLS, signature)
    assert "submit_summoned_replacement" in event
    assert "extract_obj" not in event
    assert "obj_to_char" not in event

assert "item_creation_grant_submit_to_player_with_completion" in replacement_submit
assert "summoned_replacement_completed" in replacement_submit
assert replacement_submit.index("item_creation_grant_submit_to_player_with_completion") < replacement_submit.index(
    "extract_obj"
)
assert "if (!committed)" in replacement_callback
assert "result.root_item_uid != context.item_uid" in replacement_callback
assert "summoned_replacement_kind_valid(context.kind)" in replacement_callback
assert "new_skills_find_object_by_uid" in replacement_callback
assert replacement_callback.index("new_skills_find_object_by_uid") < replacement_callback.index(
    "retire_other_summoned_items"
)
assert replacement_callback.index("retire_other_summoned_items") < replacement_callback.index(
    "materializes"
)
assert "existing spellbook was kept" in replacement_callback
assert "existing totem was kept" in replacement_callback

retire = function_body(SKILLS, "static void retire_other_summoned_items(")
assert "P_obj next = object->next" in retire
assert "object->obj_uid != keep_uid" in retire
assert "OBJ_VNUM(object) == 417" in function_body(
    SKILLS, "static bool summoned_totem_matches("
)

print("Issue 550 commit-aware conjuration contract passed.")
