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

table=$("${MYSQL[@]}" -e "SELECT COUNT(*),COALESCE(SUM(engine='InnoDB' AND table_collation='utf8mb4_unicode_ci'),0) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='economic_pending_claim_source' AND table_type='BASE TABLE';")
[[ "$table" == $'1\t1' ]] || { echo "FAILED: pending claim source table differs: $table" >&2; exit 1; }

columns=$("${MYSQL[@]}" -e "SELECT COUNT(*),COALESCE(SUM(CASE WHEN
 (column_name='source_operation_id' AND data_type='binary' AND character_maximum_length=16 AND is_nullable='NO') OR
 (column_name='source_slot' AND data_type='smallint' AND column_type LIKE '%unsigned%' AND is_nullable='NO') OR
 (column_name='lineage' AND data_type='binary' AND character_maximum_length=16 AND is_nullable='NO') OR
 (column_name='claim_mapping_id' AND data_type='bigint' AND column_type LIKE '%unsigned%' AND is_nullable='NO') OR
 (column_name='beneficiary_pid' AND data_type='int' AND column_type LIKE '%unsigned%' AND is_nullable='NO') OR
 (column_name='amount' AND data_type='bigint' AND column_type LIKE '%unsigned%' AND is_nullable='NO') OR
 (column_name='claim_operation_id' AND data_type='binary' AND character_maximum_length=16 AND is_nullable='YES')
 THEN 1 ELSE 0 END),0) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='economic_pending_claim_source';")
[[ "$columns" == $'7\t7' ]] || { echo "FAILED: pending claim source columns differ: $columns" >&2; exit 1; }

indexes=$("${MYSQL[@]}" -e "SELECT COUNT(DISTINCT index_name),COALESCE(SUM(CASE WHEN
 (index_name='PRIMARY' AND ((seq_in_index=1 AND column_name='source_operation_id') OR (seq_in_index=2 AND column_name='source_slot')) AND non_unique=0) OR
 (index_name='idx_economic_pending_claim_open' AND ((seq_in_index=1 AND column_name='beneficiary_pid') OR (seq_in_index=2 AND column_name='claim_operation_id') OR (seq_in_index=3 AND column_name='source_operation_id') OR (seq_in_index=4 AND column_name='source_slot'))) OR
 (index_name='idx_economic_pending_claim_mapping' AND ((seq_in_index=1 AND column_name='lineage') OR (seq_in_index=2 AND column_name='claim_mapping_id') OR (seq_in_index=3 AND column_name='claim_operation_id'))) OR
 (index_name='idx_economic_pending_claim_consumed' AND seq_in_index=1 AND column_name='claim_operation_id')
 THEN 1 ELSE 0 END),0) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='economic_pending_claim_source';")
[[ "$indexes" == *$'\t10' ]] || { echo "FAILED: pending claim source indexes differ: $indexes" >&2; exit 1; }

foreign_keys=$("${MYSQL[@]}" -e "SELECT COUNT(*),COALESCE(SUM(CASE WHEN
 (constraint_name='economic_pending_source_operation_fk' AND column_name='source_operation_id' AND referenced_table_name='economic_accounting_operation' AND referenced_column_name='operation_id') OR
 (constraint_name='economic_pending_claim_mapping_fk' AND column_name='claim_mapping_id' AND referenced_table_name='economic_account_mapping' AND referenced_column_name='mapping_id') OR
 (constraint_name='economic_pending_claim_operation_fk' AND column_name='claim_operation_id' AND referenced_table_name='economic_accounting_operation' AND referenced_column_name='operation_id')
 THEN 1 ELSE 0 END),0) FROM information_schema.key_column_usage WHERE table_schema=DATABASE() AND table_name='economic_pending_claim_source' AND referenced_table_name IS NOT NULL;")
[[ "$foreign_keys" == $'3\t3' ]] || { echo "FAILED: pending claim source foreign keys differ: $foreign_keys" >&2; exit 1; }

checks=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='economic_pending_claim_source' AND constraint_type='CHECK' AND constraint_name='chk_economic_pending_claim_source';")
[[ "$checks" == 1 ]] || { echo "FAILED: pending claim source check differs: $checks" >&2; exit 1; }
printf 'pending claim source schema verified\n'
