#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?}" "${DB_USER:?}" "${DB_PASSWD:?}" "${DB_NAME:?}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
shape=$("${MYSQL[@]}" -e "SELECT GROUP_CONCAT(CONCAT(column_name,':',column_type,':',is_nullable) ORDER BY ordinal_position SEPARATOR '|') FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='epic_stone_claim';")
[[ "$shape" == 'stone_uid:bigint unsigned:NO|operation_id:binary(16):NO' || "$shape" == 'stone_uid:bigint(20) unsigned:NO|operation_id:binary(16):NO' ]] || { echo 'FAILED: epic_stone_claim columns differ' >&2; exit 1; }
engine=$("${MYSQL[@]}" -e "SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='epic_stone_claim';")
primary=$("${MYSQL[@]}" -e "SELECT GROUP_CONCAT(column_name ORDER BY seq_in_index) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='epic_stone_claim' AND index_name='PRIMARY';")
fk=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.key_column_usage k JOIN information_schema.referential_constraints r USING(constraint_schema,constraint_name,table_name) WHERE k.constraint_schema=DATABASE() AND k.table_name='epic_stone_claim' AND k.column_name='operation_id' AND k.referenced_table_name='critical_operation_inbox' AND k.referenced_column_name='operation_id' AND r.update_rule='RESTRICT' AND r.delete_rule='RESTRICT';")
[[ "$engine" == InnoDB && "$primary" == stone_uid && "$fk" == 1 ]] || { echo 'FAILED: epic_stone_claim keys differ' >&2; exit 1; }
echo 'epic stone claim schema verified'
