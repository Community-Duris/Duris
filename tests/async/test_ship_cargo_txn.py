#!/usr/bin/env python3
from pathlib import Path
import sys

text = Path(__file__).resolve().parents[2].joinpath('src/ships/ship_cargo.c').read_text()
start = text.find('int write_cargo()')
if start == -1:
    print('missing write_cargo')
    sys.exit(1)

checks = {
    'sql_begin_transaction': text.find('sql_begin_transaction()', start),
    'sql_commit': text.find('sql_commit()', start),
    'sql_rollback': text.find('sql_rollback()', start),
    'query_delete': text.find('delete from ship_cargo_market_mods; delete from ship_cargo_prices;', start),
}
for k, v in checks.items():
    print(f'{k}={v}')

ok = True
for name in ('sql_begin_transaction', 'sql_commit', 'sql_rollback', 'query_delete'):
    if checks[name] == -1:
        print(f'missing {name}')
        ok = False

if ok:
    if not (checks['sql_begin_transaction'] < checks['query_delete'] < checks['sql_commit']):
        print('transaction ordering broken')
        ok = False
    if checks['sql_rollback'] > checks['sql_commit']:
        print('rollback handling should appear before commit handling')
        ok = False

sys.exit(0 if ok else 1)
