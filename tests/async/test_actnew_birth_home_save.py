#!/usr/bin/env python3
from pathlib import Path
import sys

text = Path(__file__).resolve().parents[2].joinpath('src/actnew.c').read_text()

old_assign = text.find('int old_hometown')
check = text.find('if (!do_save_silent(ch, 1))', old_assign)
restore = text.find('ch->player.hometown        = old_hometown;', check)
money = text.find('SUB_MONEY(ch, cost, 0);', check)
msg = text.find('Thank you for the payment and welcome to your new birth home', check)

print(f'old_assign={old_assign} check={check} restore={restore} money={money} msg={msg}')

ok = True
if min(old_assign, check, restore, money, msg) == -1:
    print('missing expected birth-home save gating snippets')
    ok = False
else:
    if not (check < restore < money < msg):
        print('birth-home save / payment ordering is wrong')
        ok = False

sys.exit(0 if ok else 1)
