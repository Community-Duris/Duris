#!/usr/bin/env python3
from _paths import SRC
from pathlib import Path
import sys
from contract_text import find, index

text = (SRC / "actset.c").read_text()
first = find(text, 'if (!do_save_silent(ppl, 1))')
second = find(text, 'if (!do_save_silent(ppl, 1))', first + 1)
log = find(text, 'logit(LOG_WIZ, "Failed to save %s after set command.", GET_NAME(ppl));')

print(f'first={first} second={second} log={log}')

ok = True
if first == -1 or second == -1 or log == -1:
    print('missing wrapped do_save_silent call(s) in actset.c')
    ok = False

sys.exit(0 if ok else 1)
