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
assert "if (!is_owner)\n\t{\n\t\tif (!sql_verify_chest_password" in source
assert "strcpy(name, esc_locker_name);" not in source
# locker_access_canAccess now uses db_query() instead of qry() + mysql_store_result
assert "MYSQL_RES *res = db_query(\"%s\", query);" in source
assert "if (!res)" in source
# locker_access_show and locker_access_count still use qry() + mysql_store_result
assert source.count("mysql_store_result(DB)") >= 2
assert "count = mysql_num_rows(res);" in source
assert "if (mysql_num_rows(res) >= 1)" in source
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
