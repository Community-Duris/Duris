#!/usr/bin/env python3
"""Read-only source-corpus coverage using the production parser and word renderer.

Run from a Linux development checkout with the normal C++/cJSON dependencies.
No server, account, database, active-world counts, or traffic sampling is involved.
"""
import collections
import json
import math
from pathlib import Path
import re
import statistics
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tests/async"))
from scenery_fixture import CONFIG, scenery_harness

files = sorted((ROOT / "areas/wld").glob("*.wld"))
descriptions = []
oversized = 0
for path in files:
    # A room's first two tilde fields are its title and long description. Match
    # numeric record headers only; extra descriptions and exit text are excluded.
    data = path.read_bytes().replace(b"\r\n", b"\n")
    for record in re.finditer(rb"^#\d+\n[^~]*~\n?([^~]*)~", data, re.MULTILINE):
        description = record.group(1)
        if description.strip():
            if len(description) >= 65536 or b"\0" in description:
                oversized += 1
            else:
                descriptions.append(description)
with scenery_harness() as binary:
    result = subprocess.check_output([str(binary), str(CONFIG), "--audit"],
        input=b"\0".join(descriptions) + b"\0", timeout=180)
counts = [int(line) for line in result.splitlines()]
assert len(counts) == len(descriptions)
affected = sorted(count for count in counts if count)
print(json.dumps({
    "scope": "offline tracked-area source corpus; not active rooms or traffic",
    "palette_keys": len(json.loads(CONFIG.read_text())["dictionaries"]["scenery"]),
    "wld_files": len(files),
    "nonempty_description_records": len(descriptions),
    "distinct_description_strings": len(set(descriptions)),
    "distinct_stripped_description_strings": len({text.strip() for text in descriptions}),
    "oversized_or_nul_records_excluded": oversized,
    "eligible_occurrences": sum(counts),
    "affected_records": len(affected),
    "distinct_affected_descriptions": len({text for text, count in zip(descriptions, counts) if count}),
    "distinct_stripped_affected_descriptions": len({text.strip() for text, count in zip(descriptions, counts) if count}),
    "affected_record_median": statistics.median(affected) if affected else 0,
    "affected_record_p95_nearest_rank": affected[math.ceil(len(affected) * .95) - 1] if affected else 0,
    "affected_record_histogram": dict(sorted(collections.Counter(affected).items())),
}, indent=2))
