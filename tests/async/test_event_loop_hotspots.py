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
1. Character maintenance uses owned deadlines, with no population sweep.
2. The NPC sanity check remains five-second work and body work twenty-second work.
3. sql_trace_enabled() only turns tracing on when the environment asks for it.
4. A redundant drain after a legacy caller consumed its result does not enable a
   production trace burst for MySQL's commands-out-of-sync diagnostic.
"""

from _paths import SRC
from pathlib import Path
import re
import sys
from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]
handler = (SRC / "handler.c").read_text(encoding="utf-8", errors="replace")
maintenance = (SRC / "character_maintenance.c").read_text(encoding="utf-8")
events = (SRC / "new_events.c").read_text(encoding="utf-8", errors="replace")
sql = (SRC / "sql.c").read_text(encoding="utf-8", errors="replace")

checks = []

checks.append((
    "character maintenance preserves the body and NPC check periods",
    contains(maintenance, "BODY_PERIOD = 20 * WAIT_SEC") and contains(maintenance, "NPC_CHECK_PERIOD = 5 * WAIT_SEC")
))
checks.append((
    "deadlines spread using runtime identities",
    contains(maintenance, "character->runtime_id % BODY_PERIOD") and
    contains(maintenance, "character->runtime_id % NPC_CHECK_PERIOD")
))

m = re.search(r"void generic_char_event\([^)]*\)\s*\{.*?\n\}", maintenance, re.S)
if m:
    body = m.group(0)
    checks.append((
        "callback resolves one owned character without a population walk",
        "character_list" not in body and "find_character_by_runtime_id" not in body and
        contains(body, "current_nevent->owner_runtime_id != ch->runtime_id")
    ))
    checks.append((
        "mob sanity check precedes the body deadline",
        body.index("without only.npc struct") < body.index("ne_event_tick >= ch->character_maintenance_body_due")
    ))
    checks.append((
        "the full-population periodic sweep is removed",
        '"generic-character-sweep"' not in events and
        contains(events, "character_maintenance_init();") and "char_sweep_slice" not in handler
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

checks.append((
    "already-consumed MySQL results do not enable a trace burst",
    contains(sql, "mysql_errno(conn) == CR_COMMANDS_OUT_OF_SYNC") and
    contains(sql, 'sql_trace_log_drain(conn, "clear/already_consumed", false)') and
    sql.index("mysql_errno(conn) == CR_COMMANDS_OUT_OF_SYNC") <
    sql.index('sql_trace_log_drain(conn, "clear/error", true)')
))

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

if failed:
    print("\nFailed regression checks:")
    for name in failed:
        print(f"- {name}")
    sys.exit(1)

print("\nAll event loop hotspot checks passed successfully.")
