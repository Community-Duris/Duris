#!/usr/bin/env python3
from pathlib import Path
import sys

text = Path(__file__).resolve().parents[2].joinpath('src/sql_player.c').read_text()

save_player_start = text.find('bool sql_save_player(P_char ch, int type, int room)')
items_start = text.find('bool sql_save_player_items(P_char ch)')
items_return = text.find('\treturn true;', items_start)
player_commit = text.find('if (!sql_commit())', save_player_start)
player_clear = text.find('clear_player_dirty_container_flags(ch);', save_player_start)
player_dirty_eq = text.find('REMOVE_BIT(ch->runtime_flags, CHAR_RFLAG_DIRTY_EQUIPMENT);', save_player_start)
player_dirty_inv = text.find('REMOVE_BIT(ch->runtime_flags, CHAR_RFLAG_DIRTY_INVENTORY);', save_player_start)
items_clear = text.find('clear_player_dirty_container_flags(ch);', items_start, items_return)
items_dirty_eq = text.find('REMOVE_BIT(ch->runtime_flags, CHAR_RFLAG_DIRTY_EQUIPMENT);', items_start, items_return)
items_dirty_inv = text.find('REMOVE_BIT(ch->runtime_flags, CHAR_RFLAG_DIRTY_INVENTORY);', items_start, items_return)

print(f'save_player_start={save_player_start} player_commit={player_commit} player_clear={player_clear}')
print(f'items_start={items_start} items_return={items_return} items_clear={items_clear}')

ok = True
for label, value in [
    ('sql_save_player found', save_player_start),
    ('sql_save_player_items found', items_start),
    ('sql_save_player commit found', player_commit),
    ('sql_save_player clear found', player_clear),
]:
    if value == -1:
        print(f'missing {label}')
        ok = False

if ok:
    if not (save_player_start < player_commit < player_clear):
        print('dirty container clear is not after sql_save_player commit')
        ok = False
    if items_clear != -1 or items_dirty_eq != -1 or items_dirty_inv != -1:
        print('dirty container clears still appear inside sql_save_player_items')
        ok = False

sys.exit(0 if ok else 1)
