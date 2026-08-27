#!/usr/bin/env python3
"""Regression test for the stuck command gate (PLR2_WAIT that never comes down).

CAN_ACT() gates the input queue in comm.c: while PLR2_WAIT is set, nothing the
player types is even dequeued, so commands vanish silently.  The bit is only
ever cleared by event_wait(), which CharWait() schedules through add_event() --
and add_event() refuses some requests (negative delay, dead ch).  When that
happened the player could not act again for the rest of the session.

Verifies:
1. CharWait() clamps a negative delay instead of handing it to add_event().
2. CharWait() atomically replaces a shorter event_wait with a longer one.
3. CharWait() only publishes the gate bit and absolute deadline after the
   scheduler accepts the request.
4. comm.c self-heals a gate with no event_wait scheduled OR past its deadline.
5. The event wheel does not strand due events: authoritative ordering keeps old
   debt ahead of future work, and deferral reinserts through the same path.
"""

from pathlib import Path
import re
import sys
from contract_text import contains

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
        "CharWait atomically replaces an existing event_wait",
        contains(char_wait, "nevent_replace(nevent_handle_from_event(e), event_wait, delay")
    ))
    checks.append((
        "CharWait schedules a new event_wait when none exists",
        contains(char_wait, "scheduled = add_event(event_wait, delay, ch, NULL, NULL, 0, NULL, 0);")
    ))
    checks.append((
        "CharWait clears a newly requested gate when scheduling fails",
        contains(char_wait, "if (!scheduled)") and
        contains(char_wait, "if (!e)\n\t\t\tREMOVE_BIT(ch->specials.act2, PLR2_WAIT);")
    ))
    checks.append((
        "CharWait publishes the gate only after scheduling succeeds",
        char_wait.find("if (!scheduled)") < char_wait.find("SET_BIT(ch->specials.act2, PLR2_WAIT);")
    ))
else:
    checks.append(("CharWait function present", False))

checks.append((
    "CharWait records the accepted event's absolute gate deadline",
    contains(events, "ch->specials.wait_until_pulse = scheduled.handle.event->due_tick")
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

gate = re.search(r"if \(t_ch && !CAN_ACT\(t_ch\).*?\n\n\t\t\t/\*.*?if \(\(!t_ch \|\|\s*\(t_ch &&\s*\(+CAN_ACT\(t_ch\)", comm, re.S)
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
        "deferral leaves future absolute deadlines untouched",
        contains(defer_body, "event->due_tick > ne_event_tick") and
        not contains(defer_body, "event->timer")
    ))
    checks.append((
        "deferral no longer stops at the first not-due event",
        not contains(defer_body, "future_head")
    ))
    checks.append((
        "deferral keeps both bucket ends consistent",
        contains(defer_body, "nevent_unlink_schedule(event);") and
        contains(defer_body, "nevent_link_schedule(event, static_cast<int>(next_bucket));")
    ))
else:
    checks.append(("nevent_defer_suffix present", False))

checks.append((
    "due and aged work sort ahead of future work without ad hoc promotion",
    contains(new_events, "left->due_tick < right->due_tick") and
    contains(new_events, "left_priority > right_priority") and
    not contains(new_events, "nevent_promote_overdue_event")
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
