#!/usr/bin/env python3
"""Regression test for the stuck command gate (PLR2_WAIT with no event_wait).

CAN_ACT() gates the input queue in comm.c: while PLR2_WAIT is set, nothing the
player types is even dequeued, so commands vanish silently.  The bit is only
ever cleared by event_wait(), which CharWait() schedules through add_event() --
and add_event() refuses some requests (negative delay, dead ch).  When that
happened the player could not act again for the rest of the session.

Verifies:
1. CharWait() clamps a negative delay instead of handing it to add_event().
2. CharWait() clears PLR2_WAIT when the event did not get scheduled.
3. comm.c self-heals a gate that is up with no event_wait scheduled, and logs it.
"""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
events = (ROOT / "src" / "events.c").read_text(encoding="utf-8", errors="replace")
comm = (ROOT / "src" / "comm.c").read_text(encoding="utf-8", errors="replace")

checks = []

m = re.search(r"void CharWait\(P_char ch, int delay\)\s*\{.*?\n\}", events, re.S)
if m:
    char_wait = m.group(0)
    checks.append((
        "CharWait clamps a negative delay",
        "if (delay < 0)" in char_wait and "delay = 0;" in char_wait
    ))
    checks.append((
        "CharWait clears PLR2_WAIT when event_wait was not scheduled",
        "!CAN_ACT(ch) && !get_scheduled(ch, event_wait)" in char_wait and
        "REMOVE_BIT(ch->specials.act2, PLR2_WAIT);" in char_wait
    ))
    checks.append((
        "CharWait still schedules event_wait",
        "add_event(event_wait, delay, ch, 0, 0, 0, 0, 0);" in char_wait
    ))
else:
    checks.append(("CharWait function present", False))

checks.append((
    "comm.c self-heals a stuck command gate before reading input",
    "!CAN_ACT(t_ch) && !get_scheduled(t_ch, event_wait)" in comm and
    "REMOVE_BIT(t_ch->specials.act2, PLR2_WAIT);" in comm
))
checks.append((
    "comm.c logs the stuck gate so the cause can be traced",
    "had PLR2_WAIT set with no event_wait scheduled" in comm
))
checks.append((
    "comm.c declares event_wait",
    "extern void event_wait(P_char, P_char, P_obj, void *);" in comm
))

gate = re.search(r"if \(t_ch && !CAN_ACT\(t_ch\).*?\n\n\t\t\tif \(\(!t_ch \|\| \(t_ch && \(CAN_ACT\(t_ch\)", comm, re.S)
checks.append((
    "self-heal runs before the CAN_ACT input gate",
    gate is not None
))

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

if failed:
    print("\nFailed regression checks:")
    for name in failed:
        print(f"- {name}")
    sys.exit(1)

print("\nAll command gate recovery checks passed successfully.")
