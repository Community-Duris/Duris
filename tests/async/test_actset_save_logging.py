#!/usr/bin/env python3
from pathlib import Path
import sys

text = Path(__file__).resolve().parents[2].joinpath('src/actset.c').read_text()
first = text.find('if (!do_save_silent(ppl, 1))')
second = text.find('if (!do_save_silent(ppl, 1))', first + 1)
log = text.find('logit(LOG_WIZ, "Failed to save %s after set command.", GET_NAME(ppl));')

print(f'first={first} second={second} log={log}')

ok = True
if first == -1 or second == -1 or log == -1:
    print('missing wrapped do_save_silent call(s) in actset.c')
    ok = False

sys.exit(0 if ok else 1)
