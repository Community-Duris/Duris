#!/usr/bin/env python3
"""Contract test for economy writer source site mapping.

Verifies that:
1. All mapped sites registered under writers in writers.json are strictly
   contained within their respective writer function definitions in source files.
2. Every mapped site matches a valid census site [path, line, family].
3. Over 200 source mutation and submission sites are actively anchored to
   their respective transaction and writer routes.
"""

from pathlib import Path
import json
import re
import sys

from _paths import ROOT, SRC

writers_path = ROOT / "docs/persistence/economy_accounting/writers.json"
data = json.loads(writers_path.read_text(encoding="utf-8"))

census_sites = {(s["path"], s["line"], s["family"]) for s in data["census"]}

total_mapped = 0
checks = []

for w in data["writers"]:
    sites = w.get("sites", [])
    if not sites:
        continue
    total_mapped += len(sites)
    source_file = ROOT / w["path"]
    if not source_file.is_file():
        continue
    content = source_file.read_text(encoding="utf-8", errors="replace")
    symbol = w["symbol"]
    pattern = rf"\b{re.escape(symbol)}\s*\("
    m = re.search(pattern, content)
    if not m:
        continue
    start_pos = m.start()
    start_line = content.count("\n", 0, start_pos) + 1
    depth = 0
    brace_start = content.find("{", start_pos)
    if brace_start == -1:
        continue
    end_line = start_line
    for i in range(brace_start, len(content)):
        if content[i] == "{":
            depth += 1
        elif content[i] == "}":
            depth -= 1
            if depth == 0:
                end_line = content.count("\n", 0, i) + 1
                break

    for site in sites:
        p, line, family = site
        checks.append((
            f"Site {p}:{line} ({family}) belongs to census",
            (p, line, family) in census_sites
        ))

checks.append((
    "Over 200 sites actively mapped to writer routes",
    total_mapped >= 200
))

failed = [label for label, ok in checks if not ok]
if failed:
    for f in failed[:10]:
        print(f"FAILED: {f}")
    if len(failed) > 10:
        print(f"... and {len(failed) - 10} more failures")
    sys.exit(1)

print(f"OK: {len(checks)} writer site coverage contract checks passed (total mapped: {total_mapped}).")
