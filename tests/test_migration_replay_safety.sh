#!/bin/sh
# Runs the complete migration driver a second time against an already-upgraded
# disposable database and proves the account-locker conversion does not add rows.
set -eu

: "${MIGRATION_TEST_DB:?set the disposable database name}"
: "${MYSQL_HOST:?set MySQL host}"
: "${MYSQL_USER:?set MySQL user}"
: "${MIGRATION_SCRIPT:?set the configured run_migration.sh path}"

MYSQL_PORT=${MYSQL_PORT:-3306}
if [ -n "${MYSQL_PWD:-}" ]; then
  export MYSQL_PWD
fi

count_rows() {
  mysql -N -B -h "$MYSQL_HOST" -P "$MYSQL_PORT" -u "$MYSQL_USER" "$MIGRATION_TEST_DB" \
    -e "SELECT (SELECT COUNT(*) FROM locker_items) + (SELECT COUNT(*) FROM locker_item_affects)"
}

before=$(count_rows)
evidence_dir=${DURIS_MATRIX_ROW_EVIDENCE:-"$(dirname "$MIGRATION_SCRIPT")/../bin/migration-replay"}
mkdir -p "$evidence_dir"
replay_log=$(mktemp "$evidence_dir/migration-replay.XXXXXX.log")
replay_status=0
"$MIGRATION_SCRIPT" >"$replay_log" 2>&1 || replay_status=$?
python3 - "$replay_log" "$replay_status" <<'PY'
import os
from pathlib import Path
import sys

path = Path(sys.argv[1])
output = path.read_text(encoding="utf-8", errors="replace")
for name in ("MYSQL_PWD", "DB_PASSWD"):
    password = os.environ.get(name)
    if password:
        output = output.replace(password, "<redacted>")
path.write_text(output, encoding="utf-8")
if int(sys.argv[2]):
    print(output, file=sys.stderr, end="")
PY
printf 'migration replay evidence: %s\n' "$replay_log"
if [ "$replay_status" -ne 0 ]; then
  exit "$replay_status"
fi
after=$(count_rows)

if [ "$before" != "$after" ]; then
  printf 'migration replay changed locker row count: before=%s after=%s\n' "$before" "$after" >&2
  exit 1
fi

printf 'migration replay locker rows: unchanged (%s)\n' "$after"
