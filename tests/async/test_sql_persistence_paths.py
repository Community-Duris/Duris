#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
checks = []

sql_c = (ROOT / "src/sql.c").read_text()
sql_pool_c = (ROOT / "src/sql_pool.c").read_text()
sql_player_c = (ROOT / "src/sql_player.c").read_text()
account_c = (ROOT / "src/account.c").read_text()

checks.append(("sql.c defines sql_persistence_db_name", "const char *sql_persistence_db_name(void)" in sql_c))
checks.append(("sql_pool.c uses sql_persistence_db_name in mysql_real_connect", "sql_persistence_db_name()" in sql_pool_c and "mysql_real_connect" in sql_pool_c))
checks.append(("sql_player.c uses sql_persistence_db_name in mysql_real_connect", "sql_persistence_db_name()" in sql_player_c and "mysql_real_connect" in sql_player_c))
checks.append(("sql_pool.c no longer hardcodes DB_NAME in mysql_real_connect", "mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASSWD, DB_NAME" not in sql_pool_c))
checks.append(("sql_player.c no longer hardcodes DB_NAME in mysql_real_connect", "mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASSWD, DB_NAME" not in sql_player_c))
checks.append(("sql_save_account wraps account/ips/characters in a transaction", "bool own_txn = false;" in sql_player_c and "sql_save_account: failed to save characters" in sql_player_c and "if (own_txn && !sql_commit())" in sql_player_c))
checks.append(("sql_save_account_characters fails hard on insert errors", "sql_save_account_characters" in sql_player_c and "if (!ok)" in sql_player_c and "sql_rollback();" in sql_player_c))
checks.append(("write_unique_ip logs failed ip saves", "write_unique_ip: failed to save IPs" in account_c and "if (!sql_save_account_ips" in account_c))
checks.append(("sql_restore_saved_items rewrites roots if final delete fails", "attempting to rewrite loaded items" in sql_player_c and "failed to rewrite" in sql_player_c and "struct restored_saved_item" in sql_player_c))

release_fn = re.search(r"void sql_pool_release\(MYSQL \*conn\)\n\{.*?\n\}", sql_pool_c, re.S)
if not release_fn:
    checks.append(("sql_pool_release function found", False))
else:
    body = release_fn.group(0)
    lock_pos = body.find("pthread_mutex_lock(&pool_mutex);")
    check_pos = body.find("if (!pool)")
    checks.append(("sql_pool_release locks before checking pool pointer", lock_pos != -1 and check_pos != -1 and lock_pos < check_pos))

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

if failed:
    print("\nFailed checks:")
    for name in failed:
        print(f"- {name}")
    sys.exit(1)

print("\nAll SQL persistence path checks passed.")
