#!/usr/bin/env python3
from _paths import SRC
from pathlib import Path
import sys
from contract_text import contains, find, index

text = (SRC / "ships/ship_base.c").read_text()
start = find(text, 'void shutdown_ships()')
if start == -1:
    print('missing shutdown_ships')
    sys.exit(1)

checks = {
    'sql_begin_transaction': find(text, 'sql_begin_transaction()', start),
    'sql_commit': find(text, 'sql_commit()', start),
    'sql_rollback': find(text, 'sql_rollback()', start),
    'mysql_start_transaction': find(text, 'mysql_real_query(DB, "START TRANSACTION"', start),
    'mysql_commit': find(text, 'mysql_real_query(DB, "COMMIT"', start),
    'mysql_rollback': find(text, 'mysql_real_query(DB, "ROLLBACK"', start),
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
    if not contains(text, 'start transaction failed'):
        print('missing transaction failure log')
        ok = False
    else:
        failure_pos = find(text, 'start transaction failed')
        visitor_pos = find(text, 'ShipVisitor svs', failure_pos)
        return_pos = find(text, 'return;', failure_pos, visitor_pos)
        if return_pos == -1:
            print('ship shutdown continues after transaction start failure')
            ok = False
    if find(text, 'if(!write_ship(ship) && !IS_NPC_SHIP(ship) && SHIP_LOADED(ship))', start) == -1:
        print('ship shutdown write failure guard missing')
        ok = False
    if find(text, 'panic_corruption("shutdown_ships", "write_ship failed after rollback")', start) == -1:
        print('ship shutdown failure escalation missing')
        ok = False

sys.exit(0 if ok else 1)
