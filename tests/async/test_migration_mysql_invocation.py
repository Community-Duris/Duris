#!/usr/bin/env python3
"""Contract checks for shell-safe migration database invocation."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "migrations" / "run_migration.sh").read_text(encoding="utf-8", errors="replace")

checks = {
    "password is passed through MYSQL_PWD": 'MYSQL_PWD="$DB_PASSWD"' in source,
    "password environment is exported": "export MYSQL_PWD" in source,
    "mysql arguments are represented as an array": "MYSQL=(mysql -h" in source,
    "array invocation is quoted": '"${MYSQL[@]}"' in source,
    "legacy shell command string is gone": "MYSQL_CMD=" not in source,
    "legacy unquoted execution is gone": "$MYSQL_CMD" not in source,
}

for name, ok in checks.items():
    print(("PASS" if ok else "FAIL") + ": " + name)

raise SystemExit(0 if all(checks.values()) else 1)
