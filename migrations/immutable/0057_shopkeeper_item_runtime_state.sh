#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?}" "${DB_USER:?}" "${DB_PASSWD:?}" "${DB_NAME:?}"
export MYSQL_PWD="$DB_PASSWD"
if [[ -n "${DB_SOCKET:-}" ]]; then
    [[ "$DB_SOCKET" == /* ]] || { echo 'database socket must be absolute' >&2; exit 1; }
    CONNECTION=(--protocol=socket --socket="$DB_SOCKET")
else
    CONNECTION=(--protocol=tcp -h "$DB_HOST" -P "${DB_PORT:-3306}")
    help=$(mysql --no-defaults --help)
    if [[ "$DB_HOST" == 127.0.0.1 || "$DB_HOST" == localhost || "$DB_HOST" == ::1 ]]; then
        if [[ "$help" == *--ssl-mode* ]]; then CONNECTION+=(--ssl-mode=PREFERRED); else CONNECTION+=(--skip-ssl); fi
    else
        [[ "${DB_TLS:-}" == TRUE && -f "${DB_SSL_CA:-}" ]] || { echo 'remote verification requires TLS and a CA file' >&2; exit 1; }
        if [[ "$help" == *--ssl-mode* ]]; then
            CONNECTION+=(--ssl-mode=VERIFY_IDENTITY --ssl-ca="$DB_SSL_CA")
        elif [[ "$help" == *--ssl-verify-server-cert* ]]; then
            CONNECTION+=(--ssl-verify-server-cert --ssl-ca="$DB_SSL_CA")
        else
            echo 'database client cannot verify remote identity' >&2; exit 1
        fi
    fi
fi
MYSQL=(timeout 30 mysql --no-defaults --connect-timeout=10 "${CONNECTION[@]}" -u "$DB_USER" -N -B --raw "$DB_NAME")
shape=$("${MYSQL[@]}" -e "SELECT CONCAT(column_name,':',CASE WHEN data_type='int' THEN CONCAT(data_type,IF(column_type LIKE '%unsigned%',' unsigned','')) ELSE column_type END,':',is_nullable,':',COALESCE(column_default,'<null>'),':',extra) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='shopkeeper_item_runtime_state' ORDER BY ordinal_position;" | paste -sd'|' -)
[[ "$shape" == 'item_id:int unsigned:NO:<null>:|payload:mediumblob:NO:<null>:' ]] || { echo 'keeper runtime payload columns differ' >&2; exit 1; }
engine=$("${MYSQL[@]}" -e "SELECT CONCAT(engine,':',table_collation) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='shopkeeper_item_runtime_state';")
[[ "$engine" == 'InnoDB:utf8mb4_unicode_ci' ]] || { echo 'keeper runtime payload engine/collation differs' >&2; exit 1; }
indexes=$("${MYSQL[@]}" -e "SELECT CONCAT(index_name,':',non_unique,':',GROUP_CONCAT(column_name ORDER BY seq_in_index SEPARATOR ',')) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='shopkeeper_item_runtime_state' GROUP BY index_name,non_unique ORDER BY index_name;" | paste -sd'|' -)
[[ "$indexes" == 'PRIMARY:0:item_id' ]] || { echo 'keeper runtime payload indexes differ' >&2; exit 1; }
foreign_key_count=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.key_column_usage WHERE constraint_schema=DATABASE() AND table_name='shopkeeper_item_runtime_state' AND referenced_table_name IS NOT NULL;")
[[ "$foreign_key_count" == 1 ]] || { echo 'keeper runtime payload foreign key count differs' >&2; exit 1; }
foreign_key=$("${MYSQL[@]}" -e "SELECT CONCAT(k.constraint_name,':',k.column_name,':',k.referenced_table_name,':',k.referenced_column_name,':',r.update_rule,':',r.delete_rule,':',BINARY k.referenced_table_schema=BINARY DATABASE()) FROM information_schema.key_column_usage k JOIN information_schema.referential_constraints r ON r.constraint_schema=k.constraint_schema AND r.constraint_name=k.constraint_name WHERE k.constraint_schema=DATABASE() AND k.table_name='shopkeeper_item_runtime_state' AND k.referenced_table_name IS NOT NULL;")
[[ "$foreign_key" == 'fk_shopkeeper_item_runtime_state:item_id:shopkeeper_items:id:RESTRICT:CASCADE:1' ]] || { echo 'keeper runtime payload foreign key differs' >&2; exit 1; }
checks=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.table_constraints WHERE table_schema=DATABASE() AND table_name='shopkeeper_item_runtime_state' AND constraint_type='CHECK';")
[[ "$checks" == 0 ]] || { echo 'keeper runtime payload unexpected check constraints' >&2; exit 1; }
checkpoint_column=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='shopkeepers' AND BINARY column_name=BINARY 'runtime_payload_checkpoint_revision' AND ordinal_position=10 AND data_type='bigint' AND column_type IN ('bigint unsigned','bigint(20) unsigned') AND is_nullable='YES' AND numeric_precision=20 AND numeric_scale=0 AND character_maximum_length IS NULL AND datetime_precision IS NULL AND (column_default IS NULL OR (LOCATE('MariaDB',VERSION())>0 AND BINARY column_default=BINARY 'NULL')) AND extra='';")
[[ "$checkpoint_column" == 1 ]] || { echo 'keeper checkpoint marker column differs' >&2; exit 1; }
checkpoint_check=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.table_constraints t JOIN information_schema.check_constraints c ON c.constraint_schema=t.constraint_schema AND c.constraint_name=t.constraint_name WHERE t.constraint_schema=DATABASE() AND t.table_name='shopkeepers' AND BINARY t.constraint_name=BINARY 'chk_shopkeeper_runtime_checkpoint_revision' AND t.constraint_type='CHECK' AND LOWER(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(c.check_clause,CHAR(96),''),' ',''),CHAR(9),''),CHAR(10),''),CHAR(13),''),'(',''),')',''))='runtime_payload_checkpoint_revisionisnullorruntime_payload_checkpoint_revision>0andruntime_payload_checkpoint_revision<=shop_revision';")
[[ "$checkpoint_check" == 1 ]] || { echo 'keeper checkpoint marker check differs' >&2; exit 1; }
server_version=$("${MYSQL[@]}" -e 'SELECT VERSION();')
if [[ "$server_version" != *MariaDB* ]]; then
    checkpoint_enforced=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='shopkeepers' AND BINARY constraint_name=BINARY 'chk_shopkeeper_runtime_checkpoint_revision' AND constraint_type='CHECK' AND enforced='YES';")
    [[ "$checkpoint_enforced" == 1 ]] || { echo 'keeper checkpoint marker check is not enforced' >&2; exit 1; }
fi
printf 'exact keeper runtime payload and checkpoint marker schema verified\n'
