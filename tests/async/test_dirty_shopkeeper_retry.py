#!/usr/bin/env python3
from pathlib import Path
import sys

text = Path(__file__).resolve().parents[2].joinpath('src/sql_player.c').read_text()

flush = text.find('void sql_save_dirty_shopkeepers(void)')
keeper_found = text.find('if (keeper)', flush)
keeper_saved = text.find('if (sql_save_shopkeeper(keeper, i))', keeper_found)
keeper_missing = text.find('keeper not found; keep dirty', flush)
keep_dirty = text.find('leaving dirty', keeper_missing)
clear_dirty = text.find('shop_index[i].dirty = 0;', keeper_missing)

checks = [
    ('flush function exists', flush != -1),
    ('keeper path still saves and clears dirty on success', keeper_found != -1 and keeper_saved != -1),
    ('missing keeper path now logs retry instead of clearing dirty', keeper_missing != -1 and keep_dirty != -1),
    ('missing keeper path no longer clears dirty', clear_dirty == -1),
]

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

if failed:
    print('\nFailed checks:')
    for name in failed:
        print(f'- {name}')
    sys.exit(1)

print('\nDirty shopkeeper retry semantics look correct.')
