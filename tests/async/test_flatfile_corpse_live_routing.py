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
destruction_publication = body(HANDLER, "void publish_corpse_destruction(",
                               "bool submit_corpse_destruction")
deferred_destruction = body(HANDLER, "bool persistence_defer_corpse_destruction(",
                            "void Decay(P_obj obj)")
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
assert "corpse_lifecycle_transaction_release(payload, publish_corpse_release)" in HANDLER
assert "item_ownership_runtime_apply_corpse_release" in release_publication
assert release_publication.index("item_ownership_runtime_apply_corpse_release") < \
       release_publication.index("obj_from_obj(item)")
assert release_publication.index("obj_from_obj(item)") < \
       release_publication.index("extract_obj(corpse, TRUE)")
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
assert "corpse_lifecycle_transaction_busy" in get_item
assert "corpse_lifecycle_transaction_busy" in put_item

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

print("[PASS] live corpse save, remove, release, unmaking, destruction, restore, and revision routing are wired")
