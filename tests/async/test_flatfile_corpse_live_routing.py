#!/usr/bin/env python3
"""Source contracts for live flat-file corpse lifecycle routing."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
FILES = (ROOT / "src/files.c").read_text()
COMM = (ROOT / "src/comm.c").read_text()
FIGHT = (ROOT / "src/fight.c").read_text()
ACTOBJ = (ROOT / "src/actobj.c").read_text()
HANDLER = (ROOT / "src/handler.c").read_text()


def body(source: str, signature: str, next_signature: str) -> str:
    start = source.index(signature)
    end = source.index(next_signature, start)
    return source[start:end]


write_corpse = body(FILES, "void writeCorpse(P_obj corpse)",
                    "void persistence_refresh_restored_corpse")
purge_corpse = body(FILES, "void PurgeCorpseFile(P_obj corpse)", "ush_int getShort")
restore_corpses = body(FILES, "void restoreCorpses(void)", "/** Pet only functions")
decay = body(HANDLER, "void Decay(P_obj obj)", "void update_char_objects")
release_publication = body(HANDLER, "void publish_corpse_release(",
                           "bool submit_corpse_release")
get_item = body(ACTOBJ, "void get(P_char ch, P_obj o_obj, P_obj s_obj, int showit)",
                "int fight_in_room")
put_item = body(ACTOBJ, "bool put(P_char ch, P_obj o_obj, P_obj s_obj, int showit)",
                "void do_give")

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
assert "PERSISTENCE_MODE_FLATFILE_PRIMARY" in decay
assert "submit_corpse_release(obj)" in decay
assert decay.index("submit_corpse_release(obj)") < decay.index("if (OBJ_ROOM(obj))")
assert "corpse_lifecycle_transaction_release(payload, publish_corpse_release)" in HANDLER
assert "item_ownership_runtime_apply_corpse_release" in release_publication
assert release_publication.index("item_ownership_runtime_apply_corpse_release") < \
       release_publication.index("obj_from_obj(item)")
assert release_publication.index("obj_from_obj(item)") < \
       release_publication.index("extract_obj(corpse, TRUE)")
assert "corpse_lifecycle_transaction_busy" in get_item
assert "corpse_lifecycle_transaction_busy" in put_item

print("[PASS] live corpse save, remove, release, restore, and revision routing are wired")
