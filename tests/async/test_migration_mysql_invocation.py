#!/usr/bin/env python3
"""Contract checks for shell-safe migration database invocation."""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "migrations" / "run_migration.sh").read_text(encoding="utf-8", errors="replace")

declared_total = int(re.search(r"^TOTAL=(\d+)$", source, re.MULTILINE).group(1))
step_calls = sum(
    len(re.findall(rf'^{function} "', source, re.MULTILINE))
    for function in ("run_sql", "run_sql_file", "run_check", "convert_tables_to_charset")
)
manual_steps = len(re.findall(r"^\s*STEP=\$\(\(STEP \+ 1\)\)$", source, re.MULTILINE)) - 4

checks = {
    "password is passed through MYSQL_PWD": 'MYSQL_PWD="$DB_PASSWD"' in source,
    "password environment is exported": "export MYSQL_PWD" in source,
    "mysql arguments are represented as an array": "MYSQL=(mysql -h" in source,
    "array invocation is quoted": '"${MYSQL[@]}"' in source,
    "legacy shell command string is gone": "MYSQL_CMD=" not in source,
    "legacy unquoted execution is gone": "$MYSQL_CMD" not in source,
    "displayed migration total matches executable steps": declared_total == step_calls + manual_steps,
}

for name, ok in checks.items():
    print(("PASS" if ok else "FAIL") + ": " + name)

raise SystemExit(0 if all(checks.values()) else 1)
