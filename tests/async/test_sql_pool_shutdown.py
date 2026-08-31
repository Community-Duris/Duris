#!/usr/bin/env python3
from _paths import SRC
from pathlib import Path
import sys

text = (SRC / "sql_pool.c").read_text()
acquire_start = text.index('MYSQL *sql_pool_acquire')
release_start = text.index('void sql_pool_release', acquire_start)
acquire = text[acquire_start:release_start]

checks = [
    ('acquire rejects a closing pool before waiting', 'if (!pool || pool_closing)' in acquire),
    ('acquire wakes and rejects a closing pool after waiting', acquire.count('if (!pool || pool_closing)') >= 2),
    ('shutdown marks the pool closing', 'pool_closing = 1' in text[text.index('void sql_pool_shutdown'):]),
    ('shutdown waits for borrowers before mysql_close',
     text.find('pthread_cond_wait(&pool_cond, &pool_mutex);', text.index('void sql_pool_shutdown'), text.index('mysql_close', text.index('void sql_pool_shutdown'))) != -1),
]

for name, ok in checks:
    print(f'{name}: {"ok" if ok else "FAIL"}')
    if not ok:
        sys.exit(1)
