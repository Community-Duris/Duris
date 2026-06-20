#!/usr/bin/env python3
from pathlib import Path
import sys

text = Path(__file__).resolve().parents[2].joinpath('src/sql_player.c').read_text()

inc_start = text.find('if (use_incremental)')
inc_return = text.find('\t\treturn true;', inc_start)
inc_clear_eq = text.find('REMOVE_BIT(ch->runtime_flags, CHAR_RFLAG_DIRTY_EQUIPMENT);', inc_start)
inc_clear_inv = text.find('REMOVE_BIT(ch->runtime_flags, CHAR_RFLAG_DIRTY_INVENTORY);', inc_start)

batch_start = text.find('bool success = sql_save_player_items_batch_all(pid, ch, save_equipment, save_inventory);')
batch_return = text.find('\treturn success;', batch_start)
batch_clear_eq = text.find('REMOVE_BIT(ch->runtime_flags, CHAR_RFLAG_DIRTY_EQUIPMENT);', batch_start)
batch_clear_inv = text.find('REMOVE_BIT(ch->runtime_flags, CHAR_RFLAG_DIRTY_INVENTORY);', batch_start)

print(f'inc_start={inc_start} inc_return={inc_return} inc_clear_eq={inc_clear_eq} inc_clear_inv={inc_clear_inv}')
print(f'batch_start={batch_start} batch_return={batch_return} batch_clear_eq={batch_clear_eq} batch_clear_inv={batch_clear_inv}')

ok = True
if min(inc_start, inc_return, inc_clear_eq, inc_clear_inv, batch_start, batch_return, batch_clear_eq, batch_clear_inv) == -1:
    print('missing one or more required snippets')
    ok = False
else:
    if not (inc_start < inc_clear_eq < inc_return and inc_start < inc_clear_inv < inc_return):
        print('incremental dirty-bit clears are not inside the success path')
        ok = False
    if not (batch_start < batch_clear_eq < batch_return and batch_start < batch_clear_inv < batch_return):
        print('batch dirty-bit clears are not inside the success path')
        ok = False

sys.exit(0 if ok else 1)
