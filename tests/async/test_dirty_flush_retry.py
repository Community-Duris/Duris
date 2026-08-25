#!/usr/bin/env python3
from pathlib import Path
import sys
from contract_text import contains, find, index

root = Path(__file__).resolve().parents[2]
text = (root / 'src/redis.c').read_text()

wait_block = find(text, 'if (dirty_flush_pid > 0)')
clear_on_success = find(text, 'redisCommand(redis_ctx, "DEL %s", inflight_key)', wait_block)
keep_on_fail = find(text, 'restoring dirty set for retry', wait_block)
pre_fork_region = text[find(text, '// fork for async save'):find(text, 'if (pid < 0)', text.find('// fork for async save'))]
pre_fork_del = contains(pre_fork_region, 'DEL mud:dirty_players')
child_block = find(text, 'if (pid == 0)')
child_all_ok = find(text, 'bool all_ok = true;', child_block)
child_save_check = find(text, 'if (!sql_save_player(ch, RENT_CRASH, get_room_vnum(ch)))', child_block)
child_exit = find(text, '_exit(all_ok ? 0 : 1);', child_block)

checks = [
    ("waitpid block exists", wait_block != -1),
    ("dirty set is cleared only after a successful child exit", clear_on_success != -1 and keep_on_fail != -1 and clear_on_success < keep_on_fail),
    ("dirty set is no longer cleared before fork", not pre_fork_del),
    ("child tracks overall save success", child_all_ok != -1 and child_save_check != -1 and child_exit != -1 and child_all_ok < child_exit),
]

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

if failed:
    print("\nFailed checks:")
    for name in failed:
        print(f"- {name}")
    sys.exit(1)

print("\nDirty flush retry semantics look correct.")
