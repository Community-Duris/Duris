#!/usr/bin/env python3
from _paths import SRC
from pathlib import Path
import sys

text = (SRC / "alliances.c").read_text()

start = text.find('void save_alliances()')
if start == -1:
    print('missing save_alliances')
    sys.exit(1)

own_txn = text.find('bool own_txn = false;', start)
begin_txn = text.find('sql_begin_transaction()', start)
delete_stmt = text.find('DELETE FROM alliances', start)
insert_stmt = text.find('INSERT INTO alliances', start)
commit_stmt = text.find('sql_commit()', start)
rollback_stmt = text.find('sql_rollback()', start)

print(f'start={start} own_txn={own_txn} begin_txn={begin_txn} delete={delete_stmt} insert={insert_stmt} commit={commit_stmt} rollback={rollback_stmt}')

ok = True
for label, value in [
    ('own_txn', own_txn),
    ('begin_txn', begin_txn),
    ('delete_stmt', delete_stmt),
    ('insert_stmt', insert_stmt),
    ('commit_stmt', commit_stmt),
    ('rollback_stmt', rollback_stmt),
]:
    if value == -1:
        print(f'missing {label}')
        ok = False

if ok:
    if not (start < own_txn < delete_stmt < insert_stmt < commit_stmt):
        print('transaction wrapper order is wrong')
        ok = False
    if rollback_stmt < delete_stmt:
        print('rollback handling not present in save_alliances failure path')
        ok = False

sys.exit(0 if ok else 1)
