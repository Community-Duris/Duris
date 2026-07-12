#!/usr/bin/env python3
"""
Standalone regression test for NULL mysql_store_result checks
added across 15 DB-touching source files.

Verifies that every mysql_store_result(DB) call in the reviewed files
is followed by a NULL check before the result is used.
"""

import re
import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(REPO, "src")

# Files where NULL checks were added
FILES_WITH_NULL_CHECKS = [
    "artifact.c",
    "boon.c",
    "outposts.c",
    "nexus_stones.c",
    "epic.c",
    "guildhall_db.c",
    "assocs.c",
    "ctf.c",
    "fight.c",
    "trophy.c",
    "wikihelp.c",
    "timers.c",
    "epic_bonus.c",
    "multiplay_whitelist.c",
    "alliances.c",
]

errors = []
total_checks_found = 0
total_store_result_calls = 0

for fname in FILES_WITH_NULL_CHECKS:
    fpath = os.path.join(SRC, fname)
    if not os.path.exists(fpath):
        errors.append(f"  {fname}: file not found")
        continue

    with open(fpath, "r", errors="replace") as f:
        lines = f.readlines()

    for i, line in enumerate(lines):
        if "mysql_store_result(DB)" not in line:
            continue

        total_store_result_calls += 1

        # Check if the call is already inside an if condition with NULL check
        # Pattern: if (!(var = mysql_store_result(DB))) or if ((var = mysql_store_result(DB)) != NULL)
        if "if" in line and ("NULL" in line or "!" in line):
            total_checks_found += 1
            continue

        # Extract the variable name being assigned
        m = re.search(r'(\w+)\s*=\s*mysql_store_result\(DB\)', line)
        if not m:
            # Could be inside an if condition already
            if "if" in line:
                total_checks_found += 1
                continue
            continue

        var_name = m.group(1)

        # Check the next 5 lines for a NULL check on this specific variable
        next_lines = "".join(lines[i + 1 : min(i + 6, len(lines))])
        patterns = [
            f"!{var_name}",
            f"{var_name} == NULL",
            f"{var_name} != NULL",
            f"if (!{var_name})",
            f"if ({var_name} == NULL)",
        ]

        if any(p in next_lines for p in patterns):
            total_checks_found += 1
        else:
            # Also check if the next line uses res with a guard like "if (res && ..."
            if f"if ({var_name} &&" in next_lines or f"if ({var_name}&&" in next_lines:
                total_checks_found += 1
            else:
                errors.append(f"  {fname}:{i+1}: mysql_store_result ({var_name}) without NULL check")

# Also verify that the NULL checks use LOG_DEBUG (not LOG_ERROR)
for fname in FILES_WITH_NULL_CHECKS:
    fpath = os.path.join(SRC, fname)
    if not os.path.exists(fpath):
        continue
    with open(fpath, "r", errors="replace") as f:
        content = f.read()
    if "LOG_ERROR" in content and "mysql_store_result failed" in content:
        errors.append(f"  {fname}: LOG_ERROR used instead of LOG_DEBUG in NULL check")

if errors:
    print("NULL mysql_store_result check failures:")
    for e in errors:
        print(e)
    sys.exit(1)
else:
    print(f"NULL mysql_store_result checks passed ({total_checks_found}/{total_store_result_calls} calls checked)")
    sys.exit(0)
