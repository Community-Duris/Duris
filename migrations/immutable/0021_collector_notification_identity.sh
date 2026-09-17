#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}" "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
MYSQL_BIN=${MYSQL_BIN:-mysql}
if "$MYSQL_BIN" --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=("$MYSQL_BIN" "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
check() {
    local label="$1 $2" expected=$3 sql=$4 actual
    actual=$("${MYSQL[@]}" -e "SET SESSION group_concat_max_len=16384; $sql")
    if [[ "$actual" != "$expected" ]]; then
        printf 'FAILED: %s differs\nexpected: %s\nactual: %s\n' "$label" "$expected" "$actual" >&2
        exit 1
    fi
}
check offline_messages engine 'InnoDB:utf8mb4_unicode_ci' \
    "SELECT CONCAT(engine,':',table_collation) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='offline_messages';"
check offline_messages columns \
    'id:int unsigned:NO:<null>:auto_increment|date:datetime:NO:0000-00-00 00:00:00:|pid:int:NO:0:|message_id:binary(16):YES:<null>:|message:mediumtext:NO:<null>:' \
    "SELECT GROUP_CONCAT(CONCAT(column_name,':',REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(LOWER(column_type),'bigint(20)','bigint'),'int(10)','int'),'int(11)','int'),'smallint(5)','smallint'),'tinyint(3)','tinyint'),':',is_nullable,':',COALESCE(NULLIF(LOWER(TRIM(BOTH CHAR(39) FROM column_default)),'null'),'<null>'),':',TRIM(REPLACE(LOWER(extra),'default_generated',''))) ORDER BY ordinal_position SEPARATOR '|') FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='offline_messages';"
check offline_messages indexes \
    'PRIMARY:0:id:BTREE|uq_offline_message_identity:0:pid,message_id:BTREE' \
    "SELECT GROUP_CONCAT(CONCAT(index_name,':',non_unique,':',cols,':',index_type) ORDER BY BINARY index_name SEPARATOR '|') FROM (SELECT index_name,non_unique,index_type,GROUP_CONCAT(column_name ORDER BY seq_in_index) cols FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='offline_messages' GROUP BY index_name,non_unique,index_type) s;"
check offline_messages foreign_keys '' \
    "SELECT COALESCE(GROUP_CONCAT(CONCAT(constraint_name,':',column_name,':',referenced_table_name,':',referenced_column_name) ORDER BY BINARY constraint_name SEPARATOR '|'),'') FROM information_schema.key_column_usage WHERE constraint_schema=DATABASE() AND table_name='offline_messages' AND referenced_table_name IS NOT NULL;"
check offline_message_receipts engine 'InnoDB:utf8mb4_unicode_ci' \
    "SELECT CONCAT(engine,':',table_collation) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='offline_message_receipts';"
check offline_message_receipts columns \
    'pid:int:NO:<null>:|message_id:binary(16):NO:<null>:|message:mediumtext:NO:<null>:|status:tinyint unsigned:NO:0:|attempt_count:smallint unsigned:NO:0:|last_attempt_at:timestamp(6):YES:<null>:|created_at:timestamp(6):NO:current_timestamp(6):|delivered_at:timestamp(6):YES:<null>:' \
    "SELECT GROUP_CONCAT(CONCAT(column_name,':',REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(LOWER(column_type),'bigint(20)','bigint'),'int(10)','int'),'int(11)','int'),'smallint(5)','smallint'),'tinyint(3)','tinyint'),':',is_nullable,':',COALESCE(NULLIF(LOWER(TRIM(BOTH CHAR(39) FROM column_default)),'null'),'<null>'),':',TRIM(REPLACE(LOWER(extra),'default_generated',''))) ORDER BY ordinal_position SEPARATOR '|') FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='offline_message_receipts';"
check offline_message_receipts indexes \
    'PRIMARY:0:pid,message_id:BTREE|idx_offline_message_receipt_pending:1:pid,status,last_attempt_at,created_at,message_id:BTREE' \
    "SELECT GROUP_CONCAT(CONCAT(index_name,':',non_unique,':',cols,':',index_type) ORDER BY BINARY index_name SEPARATOR '|') FROM (SELECT index_name,non_unique,index_type,GROUP_CONCAT(column_name ORDER BY seq_in_index) cols FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='offline_message_receipts' GROUP BY index_name,non_unique,index_type) s;"
check offline_message_receipts foreign_keys '' \
    "SELECT COALESCE(GROUP_CONCAT(CONCAT(constraint_name,':',column_name,':',referenced_table_name,':',referenced_column_name) ORDER BY BINARY constraint_name SEPARATOR '|'),'') FROM information_schema.key_column_usage WHERE constraint_schema=DATABASE() AND table_name='offline_message_receipts' AND referenced_table_name IS NOT NULL;"
printf 'collector notification identity schema verified\n'
