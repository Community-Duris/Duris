#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
table=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='lookup_dataset_state' AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci';")
columns=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='lookup_dataset_state' AND ((column_name='dataset_name' AND data_type='varchar' AND character_maximum_length=32 AND is_nullable='NO') OR (column_name='dataset_version' AND data_type='int' AND numeric_precision=10 AND LOWER(column_type) LIKE '%unsigned' AND is_nullable='NO') OR (column_name='dataset_checksum' AND data_type='binary' AND character_maximum_length=32 AND is_nullable='NO') OR (column_name='race_count' AND data_type='smallint' AND numeric_precision=5 AND LOWER(column_type) LIKE '%unsigned' AND is_nullable='NO') OR (column_name='class_count' AND data_type='smallint' AND numeric_precision=5 AND LOWER(column_type) LIKE '%unsigned' AND is_nullable='NO') OR (column_name='published_at' AND data_type='timestamp' AND datetime_precision=6 AND is_nullable='NO' AND UPPER(column_default) LIKE 'CURRENT_TIMESTAMP%' AND LOWER(extra) LIKE '%on update%')); ")
primary=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='lookup_dataset_state' AND index_name='PRIMARY' AND column_name='dataset_name';")
check_count=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.table_constraints WHERE table_schema=DATABASE() AND table_name='lookup_dataset_state' AND constraint_name='chk_lookup_dataset_counts' AND constraint_type='CHECK';")
[[ "$table" == 1 && "$columns" == 6 && "$primary" == 1 && "$check_count" == 1 ]]
echo "lookup dataset state schema verified"
