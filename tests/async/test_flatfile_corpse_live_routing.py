#!/usr/bin/env python3
"""Source contracts for live flat-file corpse lifecycle routing."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
FILES = (ROOT / "src/files.c").read_text()
COMM = (ROOT / "src/comm.c").read_text()
FIGHT = (ROOT / "src/fight.c").read_text()
ACTOBJ = (ROOT / "src/actobj.c").read_text()
HANDLER = (ROOT / "src/handler.c").read_text()
MOBILE_SPECS = (ROOT / "src/specs.mobile.c").read_text()
UNDERMOUNTAIN_SPECS = (ROOT / "src/specs.undermountain.c").read_text()
VERZANAN_SPECS = (ROOT / "src/specs.verzanan.c").read_text()
LOHRR_SPECS = (ROOT / "src/specs.lohrr.c").read_text()
MAGIC = (ROOT / "src/magic.c").read_text()
NECROMANCY = (ROOT / "src/necromancy.c").read_text()


def body(source: str, signature: str, next_signature: str) -> str:
    start = source.index(signature)
    end = source.index(next_signature, start)
    return source[start:end]


write_corpse = body(FILES, "void writeCorpse(P_obj corpse)",
                    "void persistence_refresh_restored_corpse")
purge_corpse = body(FILES, "void PurgeCorpseFile(P_obj corpse)", "ush_int getShort")
restore_corpses = body(FILES, "void restoreCorpses(void)", "/** Pet only functions")
decay = body(HANDLER, "void Decay(P_obj obj)", "void update_char_objects")
deferred_release = body(HANDLER, "bool persistence_defer_corpse_room_release(",
                        "void Decay(P_obj obj)")
release_publication = body(HANDLER, "void publish_corpse_release(",
                           "bool submit_corpse_release")
nested_publication = body(HANDLER, "void publish_corpse_nested_release(",
                          "bool submit_corpse_nested_release")
nested_submission = body(HANDLER, "bool submit_corpse_nested_release(",
                         "bool submit_corpse_destruction")
destruction_publication = body(HANDLER, "void publish_corpse_destruction(",
                               "bool submit_corpse_destruction")
deferred_destruction = body(HANDLER, "bool persistence_defer_corpse_destruction(",
                            "void Decay(P_obj obj)")
deferred_compaction = body(HANDLER, "bool persistence_defer_corpse_compaction(",
                           "bool persistence_defer_corpse_destruction(")
get_item = body(ACTOBJ, "void get(P_char ch, P_obj o_obj, P_obj s_obj, int showit)",
                "int fight_in_room")
put_item = body(ACTOBJ, "bool put(P_char ch, P_obj o_obj, P_obj s_obj, int showit)",
                "void do_give")
devour = body(MOBILE_SPECS, "int devour(", "void event_tentacles")
dog_one = body(VERZANAN_SPECS, "int dog_one(", "int dog_two(")
dog_two = body(VERZANAN_SPECS, "int dog_two(", "int drunk_one(")
lightning_sword = body(UNDERMOUNTAIN_SPECS, "int lightning_sword(",
                       "int um_goblin_leader(")
flying_dagger = body(UNDERMOUNTAIN_SPECS, "int flying_dagger(", "int ochre_jelly(")
ochre_jelly = body(UNDERMOUNTAIN_SPECS, "int ochre_jelly(", "int animated_sword(")
very_angry = LOHRR_SPECS[LOHRR_SPECS.index("int very_angry_npc("):]
unmaking = body(MAGIC, "void spell_unmaking(", "void spell_enchant_weapon(")
wall_of_bones = body(NECROMANCY, "void spell_wall_of_bones(",
                     "void spell_compact_corpse(")
compact_corpse = NECROMANCY[NECROMANCY.index("void spell_compact_corpse("):]
resurrect = body(MAGIC, "void spell_resurrect(", "void spell_preserve(")
lesser_resurrect = body(MAGIC, "void spell_lesser_resurrect(",
                        "void spell_mass_invisibility(")
resurrection_publication = body(HANDLER, "void publish_corpse_resurrection(",
                                "void continue_corpse_resurrection(")
raise_publication = body(HANDLER, "void publish_corpse_raise(",
                         "P_obj find_resurrection_item(")
raise_undead = body(NECROMANCY, "void raise_undead(", "#undef UNDEAD_TYPES")
call_titan = body(NECROMANCY, "void spell_call_titan(", "struct SavedCorpseData")
create_dracolich = body(NECROMANCY, "void spell_create_dracolich(",
                        "void spell_create_golem(")
create_golem = body(NECROMANCY, "void create_golem(", "void spell_call_avatar(")
call_avatar = body(NECROMANCY, "void spell_call_avatar(",
                   "void spell_create_greater_dracolich(")
create_greater_dracolich = body(NECROMANCY, "void spell_create_greater_dracolich(",
                                "void do_exhume(")
raise_completion = body(NECROMANCY, "void complete_corpse_raise_after_commit(",
                        "void spell_create_dracolich(")

assert "PERSISTENCE_MODE_FLATFILE_PRIMARY" in write_corpse
assert "stage_corpse_lifecycle" in write_corpse
assert "corpse_lifecycle_action::upsert" in write_corpse
assert "corpse_lifecycle_action::remove" in write_corpse
assert write_corpse.index("PERSISTENCE_MODE_FLATFILE_PRIMARY") < write_corpse.index(
    "sql_save_corpse")
assert "PERSISTENCE_MODE_FLATFILE_PRIMARY" in purge_corpse
assert "skip_corpse_save" in purge_corpse
assert "corpse_lifecycle_action::remove" in purge_corpse
assert "flatfile_corpse_restore_catalog" in restore_corpses
assert "fatal_boot_error" in restore_corpses

assert "corpse_lifecycle_transaction_handle_completions" in COMM
assert "corpse_lifecycle_transaction_pulse();" in COMM
assert COMM.index("corpse_lifecycle_transaction_pulse();") < COMM.index(
    "critical_command_coordinator_pulse(critical_completions")
assert "corpse_lifecycle_transaction_note_item_transfer" in FIGHT
assert "corpse_lifecycle_transaction_note_item_transfer" in ACTOBJ
assert "persistence_defer_corpse_room_release(obj)" in decay
assert decay.index("persistence_defer_corpse_room_release(obj)") < decay.index(
    "if (OBJ_ROOM(obj))")
assert "PERSISTENCE_MODE_FLATFILE_PRIMARY" in deferred_release
assert "corpse_lifecycle_transaction_busy" in deferred_release
assert "submit_corpse_release(corpse)" in deferred_release
assert "OBJ_INSIDE(corpse) ? submit_corpse_nested_release(corpse)" in deferred_release
assert "corpse_lifecycle_transaction_release(payload, publish_corpse_release)" in HANDLER
assert "item_ownership_runtime_apply_corpse_release" in release_publication
assert release_publication.index("item_ownership_runtime_apply_corpse_release") < \
       release_publication.index("obj_from_obj(item)")
assert release_publication.index("obj_from_obj(item)") < \
       release_publication.index("extract_obj(corpse, TRUE)")
assert "corpse_lifecycle_action::release_nested" in nested_submission
assert "item_owner_type::player" in nested_submission
assert "item_owner_type::room" in nested_submission
assert "target_root_item_uid" in nested_submission
assert "target_parent_item_uid" in nested_submission
assert "expected_target_parent_revision" in nested_submission
assert "corpse_lifecycle_transaction_release(payload, publish_corpse_nested_release)" in \
       nested_submission
assert "item_ownership_runtime_apply_corpse_nested_release" in nested_publication
assert nested_publication.index("item_ownership_runtime_apply_corpse_nested_release") < \
       nested_publication.index("obj_from_obj(item)")
assert nested_publication.index("obj_to_obj(item, parent)") < \
       nested_publication.index("extract_obj(corpse, TRUE)")
assert "discard_corpse_release_money" in nested_publication
assert "writeCharacter(carrier, RENT_CRASH" in nested_publication
assert "corpse_lifecycle_transaction_destroy" in HANDLER
assert "item_ownership_runtime_apply_corpse_destruction" in destruction_publication
assert destruction_publication.index("item_ownership_runtime_apply_corpse_destruction") < \
       destruction_publication.index("extract_obj(corpse, TRUE)")
assert "PERSISTENCE_MODE_FLATFILE_PRIMARY" in deferred_destruction
assert "submit_corpse_destruction(corpse)" in deferred_destruction
assert "persistence_defer_corpse_destruction(corpse)" in very_angry
assert very_angry.index("persistence_defer_corpse_destruction(corpse)") < \
       very_angry.index("extract_obj(corpse, TRUE)")
assert "persistence_defer_corpse_unmaking(obj, ch, level, clevel)" in unmaking
assert unmaking.index("persistence_defer_corpse_unmaking(obj, ch, level, clevel)") < \
       unmaking.index("obj_from_obj(cobj)")
assert "corpse_unmakings" in HANDLER
assert "caster->runtime_id" in HANDLER
assert "live_room != room" in release_publication
assert release_publication.index("live_room != room") < \
       release_publication.index("item_ownership_runtime_apply_corpse_release")
assert HANDLER.index("item_ownership_runtime_apply_corpse_release") < \
       HANDLER.index("GET_HIT(caster) =")
assert "persistence_defer_corpse_wall_of_bones(corpse, ch, level, exit_dir)" in wall_of_bones
assert wall_of_bones.index("persistence_defer_corpse_wall_of_bones") < \
       wall_of_bones.index("complete_corpse_wall_of_bones")
assert wall_of_bones.index("persistence_defer_corpse_wall_of_bones") < \
       wall_of_bones.index("obj_from_obj(obj_in_corpse)")
assert "corpse_walls" in HANDLER
assert release_publication.index("item_ownership_runtime_apply_corpse_release") < \
       release_publication.index("complete_corpse_wall_of_bones")
assert "persistence_defer_corpse_compaction(obj, ch)" in compact_corpse
assert compact_corpse.index("persistence_defer_corpse_compaction") < \
       compact_corpse.index("obj_from_obj(content)")
assert "corpse_compactions" in HANDLER
assert deferred_compaction.index("read_object(VOBJ_PILE_BONES, VIRTUAL)") < \
       deferred_compaction.index("submit_corpse_release(corpse)")
assert "extract_obj(pile)" in deferred_compaction
assert release_publication.index("item_ownership_runtime_apply_corpse_release") < \
       release_publication.index("obj_to_room(compact_pile, room)")
assert "corpse_lifecycle_transaction_busy" in get_item
assert "corpse_lifecycle_transaction_busy" in put_item
for resurrection_spell in (resurrect, lesser_resurrect):
    assert "persistence_defer_corpse_resurrection" in resurrection_spell
    assert resurrection_spell.index("persistence_defer_corpse_resurrection") < \
           resurrection_spell.index("stop_fighting(t_ch)")
assert "corpse_lifecycle_transaction_resurrect" in HANDLER
assert "item_ownership_runtime_apply_corpse_resurrection" in resurrection_publication
assert resurrection_publication.index("item_ownership_runtime_apply_corpse_resurrection") < \
       resurrection_publication.index("complete_player_resurrection_after_commit")
assert HANDLER.index("item_movement_transaction_submit(",
                     HANDLER.index("void continue_corpse_resurrection(")) < \
       HANDLER.index("corpse_lifecycle_transaction_resurrect(",
                     HANDLER.index("void continue_corpse_resurrection("))
for raise_spell in (raise_undead, call_titan, create_dracolich, create_golem,
                    call_avatar, create_greater_dracolich):
    assert "persistence_defer_corpse_raise" in raise_spell
    assert raise_spell.index("persistence_defer_corpse_raise") < \
           raise_spell.index("char_to_room")
    assert raise_spell.index("persistence_defer_corpse_raise") < \
           raise_spell.index("create_saved_corpse")
assert "corpse_lifecycle_transaction_raise_follower" in HANDLER
assert "item_ownership_runtime_apply_corpse_raise" in raise_publication
assert raise_publication.index("item_ownership_runtime_apply_corpse_raise") < \
       raise_publication.index("complete_corpse_raise_after_commit")
assert "discard_nested_money" in raise_completion
assert "writeCharacter(caster, RENT_CRASH" in raise_completion

for release_caller, first_mutation in (
        (devour, "obj_from_obj(temp)"),
        (dog_one, "obj_from_obj(temp)"),
        (dog_two, "obj_from_obj(temp)"),
        (lightning_sword, "obj_from_obj(temp)"),
        (flying_dagger, "obj_from_obj(temp)"),
        (ochre_jelly, "obj_from_obj(temp)")):
    assert "persistence_defer_corpse_room_release" in release_caller
    assert release_caller.index("persistence_defer_corpse_room_release") < \
           release_caller.index(first_mutation)

print("[PASS] live corpse save, remove, release, effects, destruction, restore, and revision routing are wired")
