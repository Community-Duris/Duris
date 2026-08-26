#!/usr/bin/env python3
"""Replacement-row save contracts for player array-backed components."""

from pathlib import Path

root = Path(__file__).resolve().parents[2]
text = (root / "src/sql_player.c").read_text()

status_start = text.rfind("bool sql_save_player_status(P_char ch, int type, int room)\n{")
status_end = text.find("bool sql_save_player_skills(P_char ch)\n{", status_start)
status = text[status_start:status_end]

components = [
    ("timers", "player_timers", "NUMB_PC_TIMERS", "pc_timer[i] != 0"),
    ("undead spell slots", "player_undead_slots", "MAX_CIRCLE", "undead_spell_slots[i] != 0"),
    ("forged items", "player_forged_items", "MAX_FORGE_ITEMS", "learned_forged_list[i] != 0"),
    ("granted commands", "player_granted_cmds", "numb_gcmd", "numb_gcmd > 0"),
]

for index, (label, table, bound, nonzero) in enumerate(components):
    start = status.index(f"// {label} - batch delete then batch insert")
    if index + 1 < len(components):
        end = status.index(f"// {components[index + 1][0]} - batch delete then batch insert", start)
    else:
        replace_start = status.index("REPLACE INTO player_granted_cmds", start)
        end = status.index("\n\tfree(batch);\n\n\tif (own_txn)", replace_start)
    block = status[start:end]
    delete = f'sql_delete_player_subtable(pid, "{table}")'
    replace = f"REPLACE INTO {table}"
    insert_failure = block[block.index("if (has_data)"):]
    checks = {
        "checked delete": f"if (!{delete})" in block,
        "delete before replacement": block.index(delete) < block.index(replace),
        "delete failure frees batch": "free(batch);" in block[:block.index(replace)],
        "delete failure honors owner": "if (own_txn)" in block[:block.index(replace)] and
                                       "sql_rollback();" in block[:block.index(replace)],
        "delete failure returns false": "return false;" in block[:block.index(replace)],
        "current bound retained": bound in block,
        "zero filtering retained": nonzero in block,
        "empty insert suppressed": "if (has_data)" in block,
        "insert failure is checked": "if (!sql_run_query(batch))" in insert_failure,
        "insert failure frees batch": "free(batch);" in insert_failure,
        "insert failure honors owner": "if (own_txn)" in insert_failure and
                                       "sql_rollback();" in insert_failure,
        "insert failure returns false": "return false;" in insert_failure,
    }
    for check, passed in checks.items():
        print(f"[{'PASS' if passed else 'FAIL'}] {label}: {check}")
    assert all(checks.values()), label

# Existing replacement components remain unchanged and use the same checked helper.
for table in ("player_languages", "player_intros"):
    assert f'if (!sql_delete_player_subtable(pid, "{table}"))' in status

master_start = text.rfind("bool sql_save_player(P_char ch, int type, int room)\n{", 0, status_start)
master = text[master_start:status_start]
status_failure = master[master.index("if (!sql_save_player_status"):master.index(
    "if (!sql_save_player_skills"
)]
assert "sql_rollback();" in status_failure
assert "return false;" in status_failure

assert status.index("if (own_txn)\n\t{\n\t\tif (!sql_commit())") > status.index(
    'sql_delete_player_subtable(pid, "player_granted_cmds")'
)

print("player replacement-state source contracts passed")
