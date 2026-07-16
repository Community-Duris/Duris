#!/usr/bin/env python3
from pathlib import Path
import sys

text = Path(__file__).resolve().parents[2].joinpath('src/actobj.c').read_text()
if_guard = text.find('if (!do_save_silent(ch, 1))')
log = text.find('Failed to save %s after level mushroom')
extract = text.find('extract_obj(temp);', if_guard)
print(f'if_guard={if_guard} log={log} extract={extract}')

ok = True
if min(if_guard, log, extract) == -1:
    print('missing level mushroom save guard')
    ok = False
elif not (if_guard < log < extract):
    print('unexpected level mushroom save/log order')
    ok = False

sys.exit(0 if ok else 1)
