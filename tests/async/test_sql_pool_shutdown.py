#!/usr/bin/env python3
from pathlib import Path
import sys

text = Path(__file__).resolve().parents[2].joinpath('src/sql_pool.c').read_text()
wait = text.find('pthread_cond_wait(&pool_cond, &pool_mutex);')
shutdown_guard = text.find('if (!pool)', wait)
next_wait = text.find('pthread_cond_wait(&pool_cond, &pool_mutex);', wait + 1)
return_null = text.find('return NULL;', shutdown_guard)

print(f'wait={wait} shutdown_guard={shutdown_guard} return_null={return_null} next_wait={next_wait}')

ok = True
if min(wait, shutdown_guard, return_null) == -1:
    print('missing acquire/shutdown guard snippet')
    ok = False
elif not (wait < shutdown_guard < return_null):
    print('shutdown guard is not in the wake-up path')
    ok = False
elif next_wait != -1 and not (shutdown_guard < next_wait):
    print('unexpected ordering around cond-wait loop')
    ok = False

sys.exit(0 if ok else 1)
