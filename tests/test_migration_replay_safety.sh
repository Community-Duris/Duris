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
"$MIGRATION_SCRIPT" >/tmp/duris-migration-replay.log 2>&1
after=$(count_rows)

if [ "$before" != "$after" ]; then
  printf 'migration replay changed locker row count: before=%s after=%s\n' "$before" "$after" >&2
  exit 1
fi

printf 'migration replay locker rows: unchanged (%s)\n' "$after"
