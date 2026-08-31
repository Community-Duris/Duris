#!/usr/bin/env python3
from _paths import SRC
from pathlib import Path
import sys

text = (SRC / "actwiz.c").read_text()
count = text.count('if (!do_save_silent(victim, 1))')
log = text.count('logit(LOG_WIZ, "Failed to save %s after wizard flag change.", GET_NAME(victim));')

print(f'count={count} log={log}')

ok = True
if count < 2 or log < 2:
    print('missing wrapped do_save_silent call(s) in actwiz.c')
    ok = False

sys.exit(0 if ok else 1)
