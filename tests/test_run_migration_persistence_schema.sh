#!/bin/sh
# Integration regression probe: run after migrations/run_migration.sh against an
# isolated production-derived database.  It is deliberately read-only.
set -eu

: "${MIGRATION_TEST_DB:?set the isolated database name}"
: "${MYSQL_HOST:?set the MySQL host}"
: "${MYSQL_USER:?set the MySQL user}"
: "${MYSQL_PWD:?set the MySQL password}"

mysql_cmd="mysql -N -h $MYSQL_HOST -u $MYSQL_USER"
if [ -n "${MYSQL_PORT:-}" ]; then
    mysql_cmd="$mysql_cmd -P $MYSQL_PORT"
fi

assert_table() {
    table=$1
    actual=$(sh -c "$mysql_cmd \"$MIGRATION_TEST_DB\" -e \"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='$table'\"")
    [ "$actual" = "1" ] || {
        echo "missing required table: $table" >&2
        exit 1
    }
}

assert_column() {
    table=$1
    column=$2
    actual=$(sh -c "$mysql_cmd \"$MIGRATION_TEST_DB\" -e \"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='$table' AND column_name='$column'\"")
    [ "$actual" = "1" ] || {
        echo "missing required column: $table.$column" >&2
        exit 1
    }
}

for table in \
  corpse_item_extra_descr \
  account_locker_item_extra_descr \
    saved_item_extra_descr \
    shopkeeper_item_extra_descr \
    siege_item_extra_descr
do
    assert_table "$table"
done

for table in corpse_items player_pet_items shopkeeper_items saved_items siege_items
do
    for column in bitvector1 bitvector2 bitvector3 bitvector4 bitvector5
do
        assert_column "$table" "$column"
    done
done

assert_column player_pet_items item_type
assert_column siege_items item_type

echo "persistence migration schema: ok"
