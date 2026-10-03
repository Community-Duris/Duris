#!/usr/bin/env python3
"""Lock down the SQL deletion guard and its position in character deletion."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FILES = (ROOT / "src/core/files.c").read_text(encoding="utf-8")
SQL_PLAYER = (ROOT / "src/sql/sql_player.c").read_text(encoding="utf-8")
SQL_PLAYER_DELETION_H = (ROOT / "src/sql/sql_player_deletion.h").read_text(
    encoding="utf-8"
)
DEATH_CONFLICT = (ROOT / "src/player/player_death_conflict_repository.c").read_text(
    encoding="utf-8"
)


def function_body(source: str, signature: str, *, last: bool = False) -> str:
    start = source.rindex(signature) if last else source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for position in range(brace, len(source)):
        if source[position] == "{":
            depth += 1
        elif source[position] == "}":
            depth -= 1
            if depth == 0:
                return source[start : position + 1]
    raise AssertionError(f"unterminated function: {signature}")


remove_character = function_body(FILES, "character_delete_result delete_character_result(")
guard = function_body(SQL_PLAYER, "bool sql_player_deletion_guard(", last=True)
delete_player = function_body(SQL_PLAYER, "bool sql_delete_player(", last=True)
retain_conflict = function_body(
    DEATH_CONFLICT, "retain_death_conflict(MYSQL *connection,"
)

# The account-menu confirmation enters the actual deletion body; the DB guard
# must acquire its lock before soft-delete or any artifact/custody cleanup.
begin = remove_character.index("sql_begin_transaction()")
check = remove_character.index("sql_player_deletion_guard(")
soft_delete = remove_character.index("sql_soft_delete_character(")
assert begin < check < soft_delete
assert remove_character.index("if (!prepared)") > check
assert "sql_rollback()" in remove_character[check:]

# The exact-player lock serializes the evidence read with native retention;
# the nonlocking read is the first consistent read after acquiring that lock.
assert "!DB" in guard and "pid <= 0" in guard and "!sql_in_transaction()" in guard
assert "FROM player_data WHERE pid=" in guard and "FOR UPDATE" in guard
assert "FROM player_death_conflict_evidence WHERE pid=" in guard
assert "LIMIT 1" in guard and "LIMIT 1 FOR UPDATE" not in guard
assert "FOR UPDATE" in guard and "player_data WHERE pid=" in guard
assert "if (!result)" in guard and "return false;" in guard
assert SQL_PLAYER_DELETION_H.count("sql_player_deletion_guard") >= 1
retention_admission = retain_conflict.index("economic_sql_currency_writer_guard::acquire")
retention_player_lock = retain_conflict.index("SELECT save_revision FROM player_data WHERE pid=")
retention_insert = retain_conflict.index("INSERT INTO player_death_conflict_evidence")
assert retention_admission < retention_player_lock < retention_insert
assert "economic_sql_currency_writer_guard::acquire" not in guard
assert "writer.is_valid_for(DB)" in guard
assert remove_character.index("economic_sql_currency_writer_guard::acquire") < begin

# The physical DELETE boundary independently fails closed for callers that do
# not originate in delete_character_result(). The caller-owned outer transaction
# retains all cleanup changes for rollback on refusal.
assert "sql_player_deletion_guard" in SQL_PLAYER_DELETION_H
assert "if (!sql_player_deletion_guard(pid, *writer))" in delete_player
assert "character_deletion_guard_pid != pid" in delete_player
assert "!writer || !writer->is_valid_for(DB)" in delete_player
assert delete_player.index("if (!sql_player_deletion_guard(pid, *writer))") < delete_player.index('"DELETE FROM player_data')
assert "own_txn" in delete_player and "sql_rollback()" in delete_player

print("PASS: account character deletion locks and checks retained evidence before cleanup")
