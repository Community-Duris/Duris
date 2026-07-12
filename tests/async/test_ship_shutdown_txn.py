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
    if 'start transaction failed' not in text:
        print('missing transaction failure log')
        ok = False
    else:
        failure_pos = text.find('start transaction failed')
        visitor_pos = text.find('ShipVisitor svs', failure_pos)
        return_pos = text.find('return;', failure_pos, visitor_pos)
        if return_pos == -1:
            print('ship shutdown continues after transaction start failure')
            ok = False
    if text.find('if(!write_ship(ship) && !IS_NPC_SHIP(ship) && SHIP_LOADED(ship))', start) == -1:
        print('ship shutdown write failure guard missing')
        ok = False
    if text.find('panic_corruption("shutdown_ships", "write_ship failed after rollback")', start) == -1:
        print('ship shutdown failure escalation missing')
        ok = False

sys.exit(0 if ok else 1)
