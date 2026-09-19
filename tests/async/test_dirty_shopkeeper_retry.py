#!/usr/bin/env python3
"""Regression contracts for shopkeeper identity guards and dirty retries."""

from _paths import SRC
from pathlib import Path

sql = (SRC / "sql_player.c").read_text()
files = (SRC / "core/files.c").read_text()
comm = (SRC / "net/comm.c").read_text()

flush_start = sql.rindex("bool sql_save_dirty_shopkeepers(bool force)")
flush_end = sql.index("static P_obj sql_load_saved_item_contents", flush_start)
flush = sql[flush_start:flush_end]

save_start = sql.rindex("bool sql_save_shopkeeper(P_char ch, int shop_nr)")
save_end = sql.index("bool sql_delete_shopkeeper", save_start)
save = sql[save_start:save_end]

checks = {
    "dirty flush returns false while any dirty shop remains": (
        "return false;" in flush and "return true;" in flush
    ),
    "explicit save API is bool and takes shop identity": (
        "bool sql_save_dirty_shopkeepers(bool force)" in sql
        and "int writeShopKeeper(P_char ch, int shop_nr)" in files
        and "sql_save_shopkeeper(ch, shop_nr)" in files
    ),
    "validated identity is bound before persistence": (
        "validate_shopkeeper_save(ch, shop_nr)" in save
        and "bind_shopkeeper(ch, shop_nr)" in sql
    ),
    "failed attempts record bounded retry state": "shopkeeper_save_retry_record_failure(retry, now)" in flush,
    "failed attempts retain dirty state": "leaving_dirty=1" in sql,
    "successful attempts clear and reset state": (
        "shop_index[i].dirty = 0;" in flush
        and "shopkeeper_save_retry_reset(retry);" in flush
    ),
    "invalid keeper is not silently cleared": (
        "shopkeeper_save_reason::invalid_keeper" in sql
        and "shopkeeper_save_retry_record_failure(retry, now)" in flush
    ),
    "identity scan includes live character list": "for (P_char ch = character_list" in sql,
    "identity scan distinguishes roaming shops": "shop_index[shop_nr].shop_is_roaming" in sql,
    "pretransaction guard logs its exact reason": (
        "validate_shopkeeper_save(ch, shop_nr)" in save
        and "phase=pretransaction" in sql
        and "shopkeeper_save_reason_name(reason)" in sql
    ),
    "pretransaction guard checks configured keeper identity": (
        "GET_RNUM(ch) != shop_index[shop_nr].keeper" in sql
        and "shopkeeper_save_matches_room(ch, shop_nr)" in sql
        and "bind_shopkeeper(ch, shop_nr)" in sql
    ),
    "shutdown bypasses retry backoff": "save_dirty_shopkeepers(true)" in comm,
    "direct keeper lookup is bounds-checked": "shop_nr >= number_of_shops" in files,
    "direct save failure re-dirties the shop": (
        "shop_index[shop_nr].dirty = 1;" in files
        and "writeShopKeeper: shop=%d outcome=retry leaving_dirty=1" in files
    ),
    "direct save success clears retry state": (
        "shop_index[shop_nr].dirty = 0;" in files
        and "shopkeeper_save_retry_reset(&shop_index[shop_nr].dirty_save_retry);" in files
    ),
    "retry state is initialized when shop storage is allocated": (
        "bzero(&shop_index[0], sizeof(struct shop_data));" in (SRC / "economy/shop.c").read_text()
    ),
}

for label, passed in checks.items():
    print(f"[{'PASS' if passed else 'FAIL'}] {label}")

assert all(checks.values())
print("shopkeeper dirty-save identity and retry contracts passed")
