#!/usr/bin/env python3
from pathlib import Path
import sys

text = Path(__file__).resolve().parents[2].joinpath('src/ships/ship_base.c').read_text()
start = text.find('void shutdown_ships()')
if start == -1:
    print('missing shutdown_ships')
    sys.exit(1)

checks = {
    'sql_begin_transaction': text.find('sql_begin_transaction()', start),
    'sql_commit': text.find('sql_commit()', start),
    'sql_rollback': text.find('sql_rollback()', start),
    'mysql_start_transaction': text.find('mysql_real_query(DB, "START TRANSACTION"', start),
    'mysql_commit': text.find('mysql_real_query(DB, "COMMIT"', start),
    'mysql_rollback': text.find('mysql_real_query(DB, "ROLLBACK"', start),
}
for name, pos in checks.items():
    print(f'{name}={pos}')

ok = True
for name in ('sql_begin_transaction', 'sql_commit', 'sql_rollback'):
    if checks[name] == -1:
        print(f'missing {name}')
        ok = False
for name in ('mysql_start_transaction', 'mysql_commit', 'mysql_rollback'):
    if checks[name] != -1:
        print(f'unexpected raw mysql transaction call: {name}')
        ok = False

if ok:
    if not (checks['sql_begin_transaction'] < checks['sql_commit']):
        print('transaction order is wrong')
        ok = False
    if checks['sql_rollback'] > checks['sql_commit']:
        print('rollback handling should appear before or with commit handling')
        ok = False

sys.exit(0 if ok else 1)
