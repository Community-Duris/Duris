#!/usr/bin/env python3
from pathlib import Path
import sys

text = Path(__file__).resolve().parents[2].joinpath('src/copyover.c').read_text()
first = text.find('if (!do_save_silent(d->character, RENT_CRASH))')
second = text.find('if (!do_save_silent(d->character, RENT_CRASH))', first + 1)
abort_msg = text.find('copyover: save failed for %s, aborting copyover')
rollback = text.find('sql_rollback();', first)
notify = text.find('notify_copyover_failure("\\r\\n*** Copyover FAILED - reconnect. ***\\r\\n");', first)

print(f'first={first} second={second} rollback={rollback} notify={notify} abort_msg={abort_msg}')

ok = True
if min(first, second, rollback, notify, abort_msg) == -1:
    print('copyover save failure guard missing')
    ok = False

sys.exit(0 if ok else 1)
