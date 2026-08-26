#!/usr/bin/env python3
"""Regression test for the stuck command gate (PLR2_WAIT that never comes down).

CAN_ACT() gates the input queue in comm.c: while PLR2_WAIT is set, nothing the
player types is even dequeued, so commands vanish silently.  The bit is only
ever cleared by event_wait(), which CharWait() schedules through add_event() --
and add_event() refuses some requests (negative delay, dead ch).  When that
happened the player could not act again for the rest of the session.

Verifies:
1. CharWait() clamps a negative delay instead of handing it to add_event().
2. CharWait() clears PLR2_WAIT when the event did not get scheduled.
3. CharWait() records an absolute deadline for the gate.
4. comm.c self-heals a gate with no event_wait scheduled OR past its deadline.
5. The event wheel does not strand due events, and overdue events can be promoted
   even on a pulse that has already spent its callback budget.
"""

from pathlib import Path
import re
import sys
from contract_text import contains, count

ROOT = Path(__file__).resolve().parents[2]
events = (ROOT / "src" / "events.c").read_text(encoding="utf-8", errors="replace")
comm = (ROOT / "src" / "comm.c").read_text(encoding="utf-8", errors="replace")
structs = (ROOT / "src" / "structs.h").read_text(encoding="utf-8", errors="replace")
new_events = (ROOT / "src" / "new_events.c").read_text(encoding="utf-8", errors="replace")

checks = []

m = re.search(r"void CharWait\(P_char ch, int delay\)\s*\{.*?\n\}", events, re.S)
if m:
    char_wait = m.group(0)
    checks.append((
        "CharWait clamps a negative delay",
        contains(char_wait, "if (delay < 0)") and contains(char_wait, "delay = 0;")
    ))
    checks.append((
        "CharWait clears PLR2_WAIT when event_wait was not scheduled",
        contains(char_wait, "!CAN_ACT(ch) && !get_scheduled(ch, event_wait)") and
        contains(char_wait, "REMOVE_BIT(ch->specials.act2, PLR2_WAIT);")
    ))
    checks.append((
        "CharWait still schedules event_wait",
        contains(char_wait, "add_event(event_wait, delay, ch, 0, 0, 0, 0, 0);")
    ))
else:
    checks.append(("CharWait function present", False))

checks.append((
    "CharWait records an absolute gate deadline",
    contains(events, "ch->specials.wait_until_pulse = ne_event_tick")
))
checks.append((
    "CharWait logs absurd delays so the caller can be found",
    contains(events, "delay > PULSES_IN_TICK")
))

checks.append((
    "comm.c self-heals a stuck command gate before reading input",
    contains(comm, "!get_scheduled(t_ch, event_wait) || ne_event_tick > t_ch->specials.wait_until_pulse") and
    contains(comm, "REMOVE_BIT(t_ch->specials.act2, PLR2_WAIT);")
))
checks.append((
    "comm.c logs the stuck gate so the cause can be traced",
    contains(comm, "command gate: clearing stuck PLR2_WAIT on")
))
checks.append((
    "comm.c declares event_wait",
    contains(comm, "extern void event_wait(P_char, P_char, P_obj, void *);")
))

gate = re.search(r"if \(t_ch && !CAN_ACT\(t_ch\).*?\n\n\t\t\tif \(\(!t_ch \|\|\s*\(t_ch && \(+CAN_ACT\(t_ch\)", comm, re.S)
checks.append((
    "self-heal runs before the CAN_ACT input gate",
    gate is not None
))

checks.append((
    "char_special_data carries the runtime gate deadline",
    contains(structs, "unsigned long long wait_until_pulse;")
))

defer = re.search(
    r"static long nevent_defer_suffix\(P_nevent deferred_head, long \*new_debt\)\s*\{.*?\n\}",
    new_events,
    re.S,
)
if defer:
    defer_body = defer.group(0)
    checks.append((
        "deferral credits the revolution to events it skips",
        contains(defer_body, "event->timer--;")
    ))
    checks.append((
        "deferral no longer stops at the first not-due event",
        not contains(defer_body, "future_head")
    ))
    checks.append((
        "deferral keeps both bucket ends consistent",
        contains(defer_body, "ne_schedule_tail[pulse] = event->prev_sched;") and
        contains(defer_body, "ne_schedule[next_pulse] = moved_head;")
    ))
else:
    checks.append(("nevent_defer_suffix present", False))

checks.append((
    "overdue event promotion is not blocked by an exhausted callback cap",
    not contains(new_events, "(max_callbacks <= 0 || executed < max_callbacks) && !priority_promotion_used") and
    count(new_events, "!priority_promotion_used && nevent_promote_overdue_event") == 2
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
