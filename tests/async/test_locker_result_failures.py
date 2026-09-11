#!/usr/bin/env python3
"""Regression checks for fail-closed locker result handling."""
from _paths import SRC
from pathlib import Path

source = (SRC / "storage_lockers.c").read_text()

assert "int name_len = snprintf(name, sizeof(name), \"%s\", esc_locker_name);" in source
assert "(size_t)name_len >= sizeof(name)" in source
assert "locker_require_owner" in source
assert "locker_require_active_user" not in source
assert "Only the locker owner can manage access." in source
assert "bool is_owner = locker_char && esc_locker_name_matches_player" in source
# Non-owner opens must pass the async verification and credential recheck.
opening = source[source.index("static int locker_opencmd(P_char ch, char *arg)\n{"):source.index("static int locker_closecmd(P_char ch, char * /*arg*/)\n{")]
assert "sql_verify_chest_password" not in opening
assert "if (is_owner)" in opening
assert "if (!sql_get_chest_password_hash(chest_id, &hash))" in opening
lookup_failure = opening[opening.index("if (!sql_get_chest_password_hash"):opening.index("if (!hash || !arg2[0])")]
assert "finish(ch->desc, 0, nullptr);" in lookup_failure
assert "return TRUE;" in lookup_failure
assert "finish(ch->desc, !hash && !arg2[0], nullptr);" in opening
assert "password_async_start(" in opening
assert "password_work_submit(arg2, hash, nullptr, 1, 1)" in opening
assert "valid && sql_finish_chest_password(chest_id, expected.c_str()," in opening
assert "sql_get_chest_id(locker_id, name.c_str()) != chest_id" in opening
assert opening.index("if (!valid)") < opening.index("locker->SetCurrentChestId(chest_id)")
failed = opening[opening.index("if (!valid)"):opening.index("locker->SetCurrentChestId(chest_id)")]
assert "CHEST_ACTION_FAIL" in failed and "return;" in failed
assert "if (!submitted)" in opening
assert "Password service is busy; try again later." in opening
assert "strcpy(name, esc_locker_name);" not in source
# locker_access_canAccess now uses db_query() instead of qry() + mysql_store_result
assert "MYSQL_RES *res = db_query(\"%s\", query);" in source
assert "if (!res)" in source
# locker_access_show and locker_access_count still use qry() + mysql_store_result
assert source.count("mysql_store_result(DB)") >= 2
assert "count = mysql_num_rows(res);" in source
assert "if (mysql_num_rows(res) >= 1)" in source
# Personal-locker ownership is a stable PID/racewar decision. It is checked
# before visitor grants for both idle and actively occupied lockers.
assert "static bool locker_access_canEnter(P_char locker, P_char visitor)" in source
assert "sql_locker_owner_can_access(GET_NAME(locker), GET_PID(visitor)" in source
assert source.count("!locker_access_canEnter(") == 2
sql_player = (SRC / "sql_player.c").read_text()
owner_access = sql_player[sql_player.index("bool sql_locker_owner_can_access") :]
assert "locker->owner_pid != owner_pid" in owner_access
assert "locker->owner_assoc_id" in owner_access
assert "identity.active && !identity.blocked && identity.racewar == racewar" in owner_access
mariadb_owner_access = owner_access[owner_access.index("bool sql_locker_owner_can_access", 1) :]
assert "JOIN account_characters ac ON ac.pid=l.owner_pid" in mariadb_owner_access
assert "l.owner_pid=%d" in mariadb_owner_access
assert "l.owner_assoc_id IS NULL" in mariadb_owner_access
assert "ac.racewar=l.racewar" in mariadb_owner_access
assert "ac.blocked=0" in mariadb_owner_access
assert "ac.deleted_at IS NULL" in mariadb_owner_access
# addAccess and remAccess now return bool
assert "static bool locker_access_remAccess" in source
assert "static bool locker_access_addAccess" in source
# callers check return values
assert "if (locker_access_addAccess(chLocker, arg2))" in source
assert "if (locker_access_remAccess(chLocker, arg2))" in source
# guild locker authorization
assert "locker_is_guild_member" in source
assert "bool is_guild_member = locker_is_guild_member(pLocker, ch);" in source
# PFileToLocker puts items on room floor, not invisible locker char
assert "obj_to_room(tmp_object, m_realRoom);" in source
# UnsortedChest rejects containers (in header)
header = (SRC / "storage_lockers.h").read_text()
assert "obj->type == ITEM_CONTAINER" in header
# Deferred terminal save
assert "event_deferredTerminalSave" in source
# CanAddAccount checks racewar
assert "AND racewar = %d" in source

print("locker result failure checks passed")
