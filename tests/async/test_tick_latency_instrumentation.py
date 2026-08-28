#!/usr/bin/env python3
"""Regression test for the game loop's latency instrumentation.

Three defects made the tick diagnostics lie about where time went:

1. Every section was timed with clock(), which on POSIX reports CPU time summed
   across every thread in the process.  The MySQL worker pool and the Redis
   subscriber burned CPU concurrently with the tick, so their time landed in the
   loop's own numbers and produced "MUD TICK TOOK TOO LONG - loop time -
   1.310889 / aff/pts time - 1.289582" reports for ticks that never stalled.
2. connections/commands/prompts/activities/combat were derived from the
   PROFILE_START/PROFILE_END accumulators, which only move when `do_profile` is
   on.  With profiling off -- the normal case -- those five lines of the stall
   report were stale garbage, which is why aff/pts (one of the two sections
   timed directly) always looked like the culprit.
3. The periodic latency dump wrote to the absolute path
   /durismud/logs/latency_trace.log, so no trace file was ever produced from a
   checkout that does not live at /durismud.

Verifies the loop times itself against CLOCK_MONOTONIC and dumps into logs/.
"""

from pathlib import Path
import re
import sys

from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]
comm = (ROOT / "src" / "comm.c").read_text(encoding="utf-8", errors="replace")

checks = []

checks.append((
    "a monotonic clock helper exists",
    contains(comm, "static double loop_monotonic_seconds(void)") and
    contains(comm, "clock_gettime(CLOCK_MONOTONIC, &now)")
))

match = re.search(r"void game_loop\(int port, int sslport\)\s*\{.*?\n\}", comm, re.S)
if not match:
    checks.append(("game_loop present", False))
    loop = ""
else:
    loop = match.group(0)

if loop:
    checks.append((
        "the loop no longer measures itself with process CPU time",
        not contains(loop, "clock()") and not contains(loop, "CLOCKS_PER_SEC")
    ))
    checks.append((
        "every reported section is timed with the monotonic helper",
        all(contains(loop, f"double {name}_begin = loop_monotonic_seconds();")
            for name in ("loop_time", "connections", "commands", "prompts",
                         "activities", "combat", "ne_events",
                         "affect_and_points"))
    ))
    checks.append((
        "section timings no longer come from the do_profile accumulators",
        not any(contains(loop, f"{name}_profile_end - {name}_profile_beg")
                for name in ("connections", "commands", "prompts", "activities",
                             "combat"))
    ))
    checks.append((
        "the stall report still names every measured section",
        all(contains(loop, f'"  - {label} time - %f"')
            for label in ("connections", "activities", "combat", "commands",
                          "ne_events", "prompts", "aff/pts"))
    ))
    checks.append((
        "the stall report splits aff/pts into affect_update and point_update",
        all(contains(loop, f'"    - {label} time - %f"')
            for label in ("affect_update", "point_update"))
    ))
    checks.append((
        "the split timings are recorded in the latency trace",
        all(contains(loop, f'latency_trace_record("{name}"')
            for name in ("affect_update", "point_update"))
    ))
    checks.append((
        "the latency trace dump targets the repository's logs directory",
        contains(loop, 'fopen("logs/latency_trace.log", "a")') and
        not contains(loop, "/durismud/logs")
    ))

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

if failed:
    print("\nFailed regression checks:")
    for name in failed:
        print(f"- {name}")
    sys.exit(1)

print("\nAll tick latency instrumentation checks passed successfully.")
