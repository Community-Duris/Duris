#!/usr/bin/env python3
"""Safety and backend contracts for player-initiated account deletion."""

from _paths import SRC
from contract_text import contains, index


ACCOUNT = (SRC / "account.c").read_text(encoding="utf-8", errors="replace")
SQL_PLAYER = (SRC / "sql_player.c").read_text(encoding="utf-8", errors="replace")
FLAT_DELETE = (SRC / "flatfile_account_delete.c").read_text(
    encoding="utf-8", errors="replace"
)
GUILD = (SRC / "assocs.c").read_text(encoding="utf-8", errors="replace")
SHIP = (SRC / "ship_base.c").read_text(encoding="utf-8", errors="replace")


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


password = function_body(ACCOUNT, "void get_account_password(")
begin_delete = function_body(ACCOUNT, "void delete_account(")
confirm_delete = function_body(ACCOUNT, "void verify_delete_account(")
drain_guard = ACCOUNT[
    ACCOUNT.index("class account_deletion_drain_guard") : ACCOUNT.index(
        "void remove_deleted_account_runtime"
    )
]
sql_delete = function_body(SQL_PLAYER, "bool sql_delete_account(", last=True)
flat_delete = function_body(FLAT_DELETE, "flatfile_account_delete_result flatfile_account_delete(")
guild_forget = function_body(GUILD, "void forget_deleted_guild_member(")
ship_runtime_remove = function_body(SHIP, "void delete_ship_runtime(")

# The destructive flow reuses login's bcrypt/legacy verifier and protects password input.
assert "account_password_matches(d->account, arg)" in password
assert "account_password_matches(d->account, arg)" in begin_delete
assert begin_delete.index("echo_off(d)") < begin_delete.index("account_password_matches")

# A matching password alone is insufficient: the account name must match byte-for-byte.
assert "strcmp(arg, d->account->acct_name)" in confirm_delete
assert "strcasecmp(arg, d->account->acct_name)" not in confirm_delete

# The durable, non-cancellable fence precedes disconnection, drains, and backend mutation.
fence = confirm_delete.index("d->account->acct_blocked = ACCOUNT_BLOCK_DELETION")
fence_write = confirm_delete.index("write_account(d->account)", fence)
disconnect = confirm_delete.index("close_other_account_sessions(d)")
backend = confirm_delete.index("sql_delete_account(")
assert fence < fence_write < disconnect < backend
assert "if (fenced)" in confirm_delete and "cannot be cancelled" in confirm_delete
assert "acct_blocked == ACCOUNT_BLOCK_DELETION" in password
assert "display_account_deletion_confirmation(d, true)" in password
assert "account_deletion_locker_runtime_active(account_name, identities)" in confirm_delete

# Every asynchronous writer that can republish live account/character state is drained.
for call in (
    "maintenance_scheduler_quiesce()",
    "critical_command_coordinator_quiesce()",
    "critical_outbox_quiesce()",
    "persistence_flush_all_character_saves()",
    "player_save_pipeline_quiesce()",
    "player_save_pipeline_drain(3000)",
    "locker_async_drain(3000)",
):
    assert call in drain_guard
assert "drain_pending_ship_saves()" in confirm_delete
runtime_remove = function_body(ACCOUNT, "void remove_deleted_account_runtime(")
detach_character = runtime_remove.index("character->desc = NULL")
detach_descriptor = runtime_remove.index("character_desc->character = NULL")
close_descriptor = runtime_remove.index("close_socket(character_desc)")
extract_character = runtime_remove.index("extract_char_after_terminal_save(character)")
assert detach_character < detach_descriptor < close_descriptor < extract_character
assert contains(runtime_remove, "player_revision_forget(identity.pid)")
assert contains(runtime_remove, "redis_invalidate_ship_snapshot(identity.name.c_str())")
assert contains(runtime_remove, "forget_deleted_guild_member(identity.name.c_str())")
assert contains(runtime_remove, "delete_ship_runtime(identity.name.c_str())")
assert "delete_ship_by_owner(owner_name, false)" in ship_runtime_remove
assert "P_member *link = &guild->members" in guild_forget
assert "flatfile_association_list(root, &records, &error)" in guild_forget
assert "guild->frags.frags = durable->frags" in guild_forget
assert "guild->save()" not in guild_forget

# Compile-time backend selection prevents an accidental dual-authority delete.
assert "#ifndef __NO_MYSQL__" in confirm_delete
assert "#else" in confirm_delete
assert "flatfile_account_delete(" in confirm_delete

# MariaDB locks the fence and owns one transaction. Credentials are removed last,
# reconciled absent, and only then committed.
assert "if (sql_in_transaction())" in sql_delete
assert sql_delete.index("sql_begin_transaction()") < sql_delete.index("FOR UPDATE")
assert contains(sql_delete, "atoi(row[0]) != ACCOUNT_BLOCK_DELETION")
fence_lock = index(sql_delete, '"SELECT blocked FROM accounts')
missing_account = index(sql_delete, "if (!row)", fence_lock)
fence_check = index(sql_delete, "atoi(row[0]) != ACCOUNT_BLOCK_DELETION", missing_account)
already_deleted = sql_delete[missing_account:fence_check]
assert contains(already_deleted, "strtoull(row[0], NULL, 10) == 0")
assert contains(already_deleted, "if (!already_deleted) goto fail;")
assert contains(already_deleted, "return true;")
player_remove = sql_delete.index('"DELETE FROM player_data WHERE pid=%d"')
projection_remove = index(sql_delete, '{ "account_characters", "account_name" }')
credential_remove = sql_delete.index('"DELETE FROM accounts WHERE LOWER(account_name)')
reconcile = sql_delete.index('"SELECT (SELECT COUNT(*) FROM accounts', credential_remove)
commit = sql_delete.index("sql_commit()")
assert player_remove < projection_remove < credential_remove < reconcile < commit
assert contains(sql_delete, "mysql_affected_rows(DB) != 1")
assert "status=1" in sql_delete
assert "(owner_type=4 AND (owner_id >> 32)=%d)" in sql_delete
assert "(owner_type=5 AND owner_id IN" in sql_delete
assert "UPDATE guilds g JOIN guild_members gm" in sql_delete
assert sql_delete.index("UPDATE guilds g JOIN guild_members gm") < sql_delete.index(
    "DELETE FROM guild_members"
)

# Flat-file deletion tombstones every character first, then publishes identity and
# credential removal through the recoverable authority journal, credential last.
character_remove = flat_delete.index("flatfile_character_delete(")
identity_remove = flat_delete.index("flatfile_identity_prepare_sync_account(")
credential_prepare = flat_delete.index("flatfile_account_prepare_remove(")
authority_commit = flat_delete.index("flatfile_authority_transaction_commit_operations(")
credential_check = flat_delete.rindex("flatfile_account_exists(")
identity_check = flat_delete.rindex("flatfile_identity_list_account(")
assert character_remove < identity_remove < credential_prepare < authority_commit
assert authority_commit < credential_check < identity_check

# Success destroys the live session credential and closes the connection.
assert "d->account = free_account(d->account)" in confirm_delete
assert "STATE(d) = CON_FLUSH" in confirm_delete

print("account deletion safety contracts passed")
