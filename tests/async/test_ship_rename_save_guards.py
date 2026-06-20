from pathlib import Path
import sys

text = Path(__file__).resolve().parents[2].joinpath('src/ships/ship_base.c').read_text()

rename_ship = text.find('bool rename_ship(P_char ch, char *owner_name, char *new_name)')
rename_guard = text.find('if (!write_ship(temp))', rename_ship)
rename_return = text.find('return FALSE;', rename_guard)
rename_unlink = text.find('unlink(buf);', rename_guard)

reown = text.find('bool rename_ship_owner(char *old_name, char *new_name)')
reown_guard = text.find('if (!write_ship(ship))', reown)
reown_return = text.find('return FALSE;', reown_guard)
reown_unlink = text.find('unlink(buf);', reown_guard)

print(f'rename_ship={rename_ship} rename_guard={rename_guard} rename_return={rename_return} rename_unlink={rename_unlink} reown={reown} reown_guard={reown_guard} reown_return={reown_return} reown_unlink={reown_unlink}')

ok = True
if min(rename_ship, rename_guard, rename_return, rename_unlink, reown, reown_guard, reown_return, reown_unlink) == -1:
    print('missing expected ship-rename save guards')
    ok = False
else:
    if not (rename_guard < rename_return < rename_unlink):
        print('rename_ship does not abort before unlink on write failure')
        ok = False
    if not (reown_guard < reown_return < reown_unlink):
        print('rename_ship_owner does not abort before unlink on write failure')
        ok = False

sys.exit(0 if ok else 1)
