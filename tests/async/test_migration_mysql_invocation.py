#!/usr/bin/env python3
"""Contract checks for shell-safe migration database invocation."""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "migrations" / "run_migration.sh").read_text(encoding="utf-8", errors="replace")

declared_total = int(re.search(r"^TOTAL=(\d+)$", source, re.MULTILINE).group(1))
step_functions = (
    "run_sql",
    "run_sql_file",
    "run_check",
    "convert_tables_to_charset",
    "convert_tables_to_innodb_if_present",
)
step_calls = sum(
    len(re.findall(rf'^{function} "', source, re.MULTILINE))
    for function in step_functions
)
manual_steps = (
    len(re.findall(r"^\s*STEP=\$\(\(STEP \+ 1\)\)$", source, re.MULTILINE))
    - len(step_functions)
)

checks = {
    "password is passed through MYSQL_PWD": 'MYSQL_PWD="$DB_PASSWD"' in source,
    "password environment is exported": "export MYSQL_PWD" in source,
    "mysql arguments are represented as an array": "MYSQL=(mysql -h" in source,
    "array invocation is quoted": '"${MYSQL[@]}"' in source,
    "isolated migration config can override repository env files": (
        'if [[ -n "${MIGRATION_ENV_FILE:-}" ]]' in source
        and 'source "$MIGRATION_ENV_FILE"' in source
    ),
    "legacy shell command string is gone": "MYSQL_CMD=" not in source,
    "legacy unquoted execution is gone": "$MYSQL_CMD" not in source,
    "optional legacy tables are checked before engine conversion": (
        "convert_tables_to_innodb_if_present()" in source
        and "FROM information_schema.tables" in source
        and "[ \"$table_exists\" = \"1\" ] || continue" in source
        and '"SET sql_mode=\'\'; ALTER TABLE \\`$table\\` ENGINE=InnoDB;"' in source
    ),
    "poll tables may be absent until their guarded creation step": (
        'convert_tables_to_innodb_if_present "convert legacy MyISAM tables to InnoDB"' in source
        and "poll_options poll_votes" in source
        and "polls progress" in source
    ),
    "displayed migration total matches executable steps": declared_total == step_calls + manual_steps,
}

for name, ok in checks.items():
    print(("PASS" if ok else "FAIL") + ": " + name)

raise SystemExit(0 if all(checks.values()) else 1)
