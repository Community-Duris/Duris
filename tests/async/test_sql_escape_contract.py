#!/usr/bin/env python3
"""Contract checks for the thread-safe, correctly sized SQL escape helper."""
from _paths import SRC
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (SRC / "sql.c").read_text(encoding="utf-8", errors="replace")

checks = {
    "no shared escape buffer": "static char buff[MAX_STRING_LENGTH];\n\tmysql_real_escape_string(DB, buff, str, strlen(str));" not in source,
    "null input and DB are rejected": "if (!str || !DB)" in source,
    "worst-case expansion is bounded": "(string().max_size() - 1) / 2" in source,
    "owned dynamic storage is allocated": "escaped.assign(len * 2 + 1, '\\0')" in source,
    "connector result length is honored": "escaped.resize(escaped_len)" in source,
}

for name, ok in checks.items():
    print(("PASS" if ok else "FAIL") + ": " + name)

raise SystemExit(0 if all(checks.values()) else 1)
