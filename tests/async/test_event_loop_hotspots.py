#!/usr/bin/env python3
"""Regression test for the two worst single-callback stalls in the event loop.

generic_char_event() swept every character in the game in one callback, costing
~18ms (measured avg over 38 samples) -- most of a pulse's 25ms event budget, which
pushed every other job in that pulse late.

sql_trace_enabled() set its result to true in BOTH branches, so SQL tracing was
always on: two log lines, each an open/append/close, for every query the game runs,
even with SQL_TRACE explicitly set to off.  That was the bulk of the 24ms spent in
event_write_statistic (the INSERT itself measures ~1.3ms).

Verifies:
1. generic_char_event splits its sweep into stable slices and its registry-owned
   cadence uses the matching fraction of the period, so each character is still
   visited once per full period.
2. The mob sanity check still runs on every pass.
3. sql_trace_enabled() only turns tracing on when the environment asks for it.
"""

from _paths import SRC
from pathlib import Path
import re
import sys
from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]
handler = (SRC / "handler.c").read_text(encoding="utf-8", errors="replace")
events = (SRC / "new_events.c").read_text(encoding="utf-8", errors="replace")
sql = (SRC / "sql.c").read_text(encoding="utf-8", errors="replace")

checks = []

checks.append((
    "generic_char_event declares slices and a period",
    contains(handler, "#define GENERIC_CHAR_EVENT_SLICES") and contains(handler, "#define GENERIC_CHAR_EVENT_PERIOD")
))
checks.append((
    "slice comes from a stable per-character hash",
    contains(handler, "static unsigned int char_sweep_slice(P_char c)") and contains(handler, "(uintptr_t)c")
))

m = re.search(r"void generic_char_event\([^)]*\)\s*\{.*?\n\}", handler, re.S)
if m:
    body = m.group(0)
    checks.append((
        "sweep advances a phase and skips characters outside it",
        contains(body, "generic_char_event_phase++ % GENERIC_CHAR_EVENT_SLICES") and
        contains(body, "if (char_sweep_slice(i) != phase)")
    ))
    checks.append((
        "mob sanity check still runs on every pass",
        body.index("without only.npc struct") < body.index("char_sweep_slice(i) != phase")
    ))
    checks.append((
        "registry cadence preserves the period / slices interval",
        "add_event(generic_char_event" not in body and
        contains(events, '"generic-character-sweep", generic_char_event, 20 * WAIT_SEC') and
        contains(events, "5 * WAIT_SEC, nevent_periodic_policy::fixed_delay")
    ))
else:
    checks.append(("generic_char_event present", False))

m = re.search(r"static bool sql_trace_enabled\(void\)\s*\{.*?\n\}", sql, re.S)
if m:
    body = m.group(0)
    checks.append((
        "sql_trace_enabled has no unconditional enable branch",
        body.count("on = true;") == 1
    ))
    checks.append((
        "sql_trace_enabled defaults to off",
        contains(body, "bool        on  = false;")
    ))
    checks.append((
        "sql_trace_enabled still honours SQL_TRACE",
        contains(body, 'getenv("SQL_TRACE")')
    ))
else:
    checks.append(("sql_trace_enabled present", False))

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

if failed:
    print("\nFailed regression checks:")
    for name in failed:
        print(f"- {name}")
    sys.exit(1)

print("\nAll event loop hotspot checks passed successfully.")
