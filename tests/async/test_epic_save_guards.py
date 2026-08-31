#!/usr/bin/env python3
from _paths import SRC
from pathlib import Path
import sys

text = (SRC / "epic.c").read_text()
reset_guard = text.count('logit(LOG_WIZ, "Failed to save %s after epic reset refund.", GET_NAME(t_ch));')
refund_guard = text.count('logit(LOG_WIZ, "Failed to save %s after epic skill refund.", GET_NAME(ch));')
wrapped_tch = text.count('if (!do_save_silent(t_ch, 1))')
wrapped_ch = text.count('if (!do_save_silent(ch, 1))')
print(f'reset_guard={reset_guard} refund_guard={refund_guard} wrapped_tch={wrapped_tch} wrapped_ch={wrapped_ch}')

ok = True
if reset_guard < 2 or refund_guard < 1 or wrapped_tch < 2 or wrapped_ch < 1:
    print('missing epic save guard(s)')
    ok = False

sys.exit(0 if ok else 1)
