#!/usr/bin/env python3
from _paths import SRC
from pathlib import Path
import sys
from contract_text import find, index

text = (SRC / "ships/ship_base.c").read_text()

flush_fn = find(text, 'void flush_pending_ship_saves(void)')
loop = find(text, 'for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))', flush_fn)
load_ship = find(text, 'P_ship ship = svs;', loop)
skip_non_pending = find(text, 'if (!ship || !ship->save_pending)', loop)
skip_delay = find(text, 'if (ship->save_retry_after && ship->save_retry_after > now)', loop)
signature = find(text, 'unsigned long long current_signature = ship_save_signature(ship);', loop)
dedupe = find(text, 'if (current_signature == ship->save_saved_signature)', loop)
write = find(text, 'if (write_ship(ship))', loop)
clear_after_write = find(text, 'ship->save_pending         = false;', write)
set_retry = find(text, 'ship->save_retry_after = now + 1;', write)
queue_fn = find(text, 'void queue_ship_save(P_ship ship, const char *reason)')
queue_pending = find(text, 'if (!ship->save_pending)', queue_fn)
queue_retry_reset = find(text, 'ship->save_retry_after = 0;', queue_fn)
queue_refresh = find(text, 'Refreshed pending ship save', queue_fn)
owner_sig = find(text, 'SHIP_SIG_MIX_CSTR(ship->ownername);')

print(
    f'flush_fn={flush_fn} loop={loop} load_ship={load_ship} skip_non_pending={skip_non_pending} '
    f'skip_delay={skip_delay} signature={signature} dedupe={dedupe} write={write} '
    f'clear_after_write={clear_after_write} set_retry={set_retry} queue_fn={queue_fn} '
    f'queue_pending={queue_pending} queue_retry_reset={queue_retry_reset} queue_refresh={queue_refresh}'
)

ok = True
required = [flush_fn, loop, load_ship, skip_non_pending, skip_delay, signature, dedupe, write, clear_after_write, set_retry, queue_fn, queue_pending, queue_retry_reset, queue_refresh, owner_sig]
if any(v == -1 for v in required):
    print('missing expected ship queue deduplication snippet(s)')
    ok = False
else:
    if not (flush_fn < loop < load_ship < skip_non_pending < skip_delay < signature < dedupe < write < clear_after_write < set_retry):
        print('flush path order does not preserve per-ship deduplication before write')
        ok = False
    if not (queue_fn < queue_pending and queue_retry_reset > queue_pending):
        print('queue helper does not look like a per-ship pending marker')
        ok = False

sys.exit(0 if ok else 1)
