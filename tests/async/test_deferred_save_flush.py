#!/usr/bin/env python3
from pathlib import Path
import sys

text = Path(__file__).resolve().parents[2].joinpath('src/actoth.c').read_text()

sig = text.find('bool do_save_silent(P_char ch, int type)')
ret_false = text.find('return false;', sig)
ret_true = text.find('return true;', sig)
flush_one = text.find('if (do_save_silent(ch, pending.type ? pending.type : 1))')
clear_one = text.find('memset(slot, 0, sizeof(*slot));', flush_one)
ship_guard = text.find('if (!write_ship(ship))', sig)
ship_return = text.find('return false;', ship_guard) if ship_guard != -1 else -1
flush_char = text.find('void persistence_flush_character_saves(P_char ch)')
flush_char_guard = text.find('bool saved = do_save_silent(ch, pending.type ? pending.type : 1);', flush_char)
flush_char_clear = text.find('memset(slot, 0, sizeof(*slot));', flush_char_guard)
flush_char_log = text.find('Deferred player save flush failed', flush_char_guard)
discard_missing = text.find('deferred_save_character_missing')
discard_missing_clear = text.find('memset(slot, 0, sizeof(*slot));', discard_missing)
discard_alive = text.find('deferred_save_character_not_alive')
discard_alive_clear = text.find('memset(slot, 0, sizeof(*slot));', discard_alive)
discard_global = text.find('deferred_save_discard_global')
discard_global_clear = text.find('memset(slot, 0, sizeof(*slot));', discard_global)
full_guard = text.find('deferred_save_full')
full_call = text.find('if (!do_save_silent(ch, type ? type : 1))', full_guard)
full_memset = text.find('memset(slot, 0, sizeof(*slot));', full_call, full_call + 200) if full_call != -1 else -1

print(f'sig={sig} ret_false={ret_false} ret_true={ret_true} flush_one={flush_one} clear_one={clear_one} ship_guard={ship_guard} ship_return={ship_return} flush_char={flush_char} flush_char_guard={flush_char_guard} flush_char_clear={flush_char_clear} flush_char_log={flush_char_log} discard_missing={discard_missing} discard_missing_clear={discard_missing_clear} discard_alive={discard_alive} discard_alive_clear={discard_alive_clear} discard_global={discard_global} discard_global_clear={discard_global_clear} full_guard={full_guard} full_call={full_call} full_memset={full_memset}')

ok = True
if min(sig, ret_false, ret_true, flush_one, clear_one, ship_guard, ship_return, flush_char, flush_char_guard, flush_char_clear, flush_char_log, discard_missing, discard_missing_clear, discard_alive, discard_alive_clear, discard_global, discard_global_clear, full_guard, full_call) == -1:
    print('missing expected deferred-save flush snippets')
    ok = False
else:
    if full_memset != -1 and full_memset > full_guard:
        print('full deferred-save path still clears a null slot')
        ok = False
    if not (flush_one < clear_one):
        print('global flush clears slot before successful save gate')
        ok = False
    if not (flush_char_guard < flush_char_clear):
        print('direct deferred flush clears slot before successful save gate')
        ok = False
    if not (flush_char_guard < flush_char_log):
        print('direct deferred flush does not log failure')
        ok = False
    if not (discard_missing < discard_missing_clear):
        print('missing-char deferred discard does not clear slot')
        ok = False
    if not (discard_alive < discard_alive_clear):
        print('not-alive deferred discard does not clear slot')
        ok = False
    if not (discard_global < discard_global_clear):
        print('global deferred discard does not clear slot')
        ok = False
    if not (ret_false < ret_true):
        print('save helper return flow looks wrong')
        ok = False
    if not (ship_guard < ship_return):
        print('ship save failure is not checked')
        ok = False

sys.exit(0 if ok else 1)
