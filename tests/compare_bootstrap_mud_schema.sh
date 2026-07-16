#!/bin/sh
# Read-only structural comparison for MUD tables defined by the fresh bootstrap.
# The bootstrap schema is the allow-list: website/Knex-only tables are excluded.
set -eu

: "${MIGRATED_DB:?set the archive-restored database after run_migration.sh}"
: "${BOOTSTRAP_DB:?set the fresh database loaded from bootstrap_multithread_safe.sql}"
: "${MYSQL_HOST:?set MySQL host}"
: "${MYSQL_USER:?set MySQL user}"

MYSQL_PORT=${MYSQL_PORT:-3306}
mysql_query() {
  mysql -N -B -h "$MYSQL_HOST" -P "$MYSQL_PORT" -u "$MYSQL_USER" "$@"
}
if [ -n "${MYSQL_PWD:-}" ]; then
  export MYSQL_PWD
fi

workdir=$(mktemp -d)
trap 'rm -rf "$workdir"' EXIT HUP INT TERM

# Table names present in the bootstrap define the MUD-only comparison scope.
mysql_query "$BOOTSTRAP_DB" -e \
  "SELECT table_name FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type='BASE TABLE' ORDER BY table_name" \
  > "$workdir/tables.txt"

sql_list=''
while IFS= read -r name; do
  [ -n "$name" ] || continue
  if [ -n "$sql_list" ]; then
    sql_list="$sql_list,"
  fi
  sql_list="${sql_list}'$name'"
done < "$workdir/tables.txt"

signature() {
  db=$1
  mysql_query "$db" -e "
SELECT CONCAT('TABLE|', table_name, '|', engine, '|', table_collation)
FROM information_schema.tables
WHERE table_schema=DATABASE() AND table_type='BASE TABLE' AND table_name IN ($sql_list)
UNION ALL
SELECT CONCAT('COLUMN|', table_name, '|', ordinal_position, '|', column_name, '|', column_type, '|', is_nullable, '|', COALESCE(column_default, '<NULL>'), '|', extra, '|', COALESCE(character_set_name, ''), '|', COALESCE(collation_name, ''))
FROM information_schema.columns
WHERE table_schema=DATABASE() AND table_name IN ($sql_list)
UNION ALL
SELECT CONCAT('INDEX|', table_name, '|', index_name, '|', non_unique, '|', seq_in_index, '|', COALESCE(column_name, ''), '|', COALESCE(collation, ''), '|', COALESCE(sub_part, ''))
FROM information_schema.statistics
WHERE table_schema=DATABASE() AND table_name IN ($sql_list)
UNION ALL
SELECT CONCAT('FK|', kcu.table_name, '|', kcu.constraint_name, '|', kcu.column_name, '|', kcu.referenced_table_name, '|', kcu.referenced_column_name, '|', rc.update_rule, '|', rc.delete_rule)
FROM information_schema.key_column_usage kcu
JOIN information_schema.referential_constraints rc
  ON rc.constraint_schema=kcu.constraint_schema
 AND rc.table_name=kcu.table_name
 AND rc.constraint_name=kcu.constraint_name
WHERE kcu.constraint_schema=DATABASE() AND kcu.table_name IN ($sql_list) AND kcu.referenced_table_name IS NOT NULL
" | LC_ALL=C sort
}

signature "$BOOTSTRAP_DB" > "$workdir/bootstrap.sig"
signature "$MIGRATED_DB" > "$workdir/migrated.sig"

if ! diff -u "$workdir/bootstrap.sig" "$workdir/migrated.sig" > "$workdir/diff"; then
  printf '%s\n' 'bootstrap MUD schema differs from migrated production schema:' >&2
  count=0
  while IFS= read -r line; do
    printf '%s\n' "$line"
    count=$((count + 1))
    [ "$count" -lt 80 ] || break
  done < "$workdir/diff"
  if [ "$(wc -l < "$workdir/diff")" -gt 80 ]; then
    printf '%s\n' '... diff truncated; full diff is available in the temporary test directory'
  fi
  exit 1
fi

printf '%s\n' 'bootstrap MUD schema equivalence: ok'
