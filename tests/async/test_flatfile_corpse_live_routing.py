#!/usr/bin/env python3
"""Source contracts for live flat-file corpse lifecycle routing."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
FILES = (ROOT / "src/files.c").read_text()
COMM = (ROOT / "src/comm.c").read_text()
FIGHT = (ROOT / "src/fight.c").read_text()
ACTOBJ = (ROOT / "src/actobj.c").read_text()


def body(source: str, signature: str, next_signature: str) -> str:
    start = source.index(signature)
    end = source.index(next_signature, start)
    return source[start:end]


write_corpse = body(FILES, "void writeCorpse(P_obj corpse)",
                    "void persistence_refresh_restored_corpse")
purge_corpse = body(FILES, "void PurgeCorpseFile(P_obj corpse)", "ush_int getShort")
restore_corpses = body(FILES, "void restoreCorpses(void)", "/** Pet only functions")

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

print("[PASS] live corpse save, remove, restore, and revision routing are wired")
