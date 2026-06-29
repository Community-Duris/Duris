#!/usr/bin/env python3
from pathlib import Path
import sys

text = Path(__file__).resolve().parents[2].joinpath('src/actwiz.c').read_text()
count = text.count('if (!do_save_silent(victim, 1))')
log = text.count('logit(LOG_WIZ, "Failed to save %s after wizard flag change.", GET_NAME(victim));')

print(f'count={count} log={log}')

ok = True
if count < 3 or log < 3:
    print('missing wrapped do_save_silent call(s) in actwiz.c')
    ok = False

sys.exit(0 if ok else 1)
