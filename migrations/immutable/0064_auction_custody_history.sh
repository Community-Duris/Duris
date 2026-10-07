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
# Metadata-only verifier; no credentials are sourced and no rows are changed.
query=$(cat <<'DURIS_AUCTION_CUSTODY_HISTORY_SQL'
SELECT ((SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='auction_item_custody' AND table_type='BASE TABLE' AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci') = 1
        AND (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='auction_item_custody' AND ordinal_position<=10) = 10
        AND (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='auction_item_custody' AND ordinal_position<=10 AND CONCAT(ordinal_position,':',column_name) IN ('1:auction_id','2:slot','3:item_uid','4:item_revision','5:vnum','6:obj_blob','7:claim_pid','8:claim_operation_id','9:claimed_at','10:created_at')) = 10
        AND (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='auction_item_custody' AND column_name='item_uid' AND data_type='bigint' AND LOWER(column_type) IN ('bigint unsigned','bigint(20) unsigned') AND is_nullable='NO' AND numeric_precision=20 AND numeric_scale=0) = 1
        AND (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='auction_item_custody' AND column_name='claimed_at' AND ordinal_position=9 AND data_type='timestamp' AND datetime_precision=6 AND is_nullable='YES' AND (column_default IS NULL OR LOWER(column_default)='null') AND COALESCE(extra,'')='') = 1
        AND (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='auction_item_custody' AND index_name='PRIMARY' AND non_unique=0 AND index_type='BTREE' AND sub_part IS NULL AND ((seq_in_index=1 AND column_name='auction_id') OR (seq_in_index=2 AND column_name='slot'))) = 2
        AND (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='auction_item_custody' AND index_name='idx_auction_custody_claim_operation' AND non_unique=1 AND index_type='BTREE' AND sub_part IS NULL AND ((seq_in_index=1 AND column_name='claim_operation_id'))) = 1
        AND (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='auction_item_custody' AND index_name='idx_auction_custody_claim' AND non_unique=1 AND index_type='BTREE' AND sub_part IS NULL AND ((seq_in_index=1 AND column_name='claim_pid') OR (seq_in_index=2 AND column_name='claimed_at') OR (seq_in_index=3 AND column_name='auction_id'))) = 3
        AND (SELECT COUNT(*) FROM information_schema.key_column_usage k JOIN information_schema.referential_constraints r ON r.constraint_schema=k.constraint_schema AND r.table_name=k.table_name AND r.constraint_name=k.constraint_name WHERE k.constraint_schema=DATABASE() AND k.table_schema=DATABASE() AND k.table_name='auction_item_custody' AND k.constraint_name='auction_custody_item_fk' AND k.column_name='item_uid' AND k.ordinal_position=1 AND k.position_in_unique_constraint=1 AND k.referenced_table_schema=DATABASE() AND k.referenced_table_name='item_current_owner' AND k.referenced_column_name='item_uid' AND r.update_rule='RESTRICT' AND r.delete_rule='RESTRICT') = 1
        AND (SELECT COUNT(*) FROM information_schema.key_column_usage WHERE table_schema=DATABASE() AND table_name='auction_item_custody' AND referenced_table_name IS NOT NULL) = 1
        AND (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='auction_item_custody' AND 1=1)=11 AND (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='auction_item_custody' AND column_name='active_item_uid' AND ordinal_position=11 AND data_type='bigint' AND LOWER(column_type) IN ('bigint unsigned','bigint(20) unsigned') AND is_nullable='YES' AND numeric_precision=20 AND numeric_scale=0 AND LOWER(extra)='stored generated' AND LOWER(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(generation_expression,CHAR(96),''),' ',''),CHAR(9),''),CHAR(10),''),CHAR(13),''),'(',''),')',''))='casewhenclaimed_atisnullthenitem_uidelsenullend')=1 AND (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='auction_item_custody' AND index_name='idx_auction_custody_item' AND non_unique=1 AND index_type='BTREE' AND sub_part IS NULL AND ((seq_in_index=1 AND column_name='item_uid'))) = 1 AND (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='auction_item_custody' AND index_name='uq_auction_custody_active_item' AND non_unique=0 AND index_type='BTREE' AND sub_part IS NULL AND ((seq_in_index=1 AND column_name='active_item_uid'))) = 1 AND (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='auction_item_custody' AND index_name IN ('idx_auction_custody_item','uq_auction_custody_active_item'))=2 AND (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='auction_item_custody' AND index_name='uq_auction_custody_item')=0 AND (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='auction_item_custody' AND 1=1)=8);
DURIS_AUCTION_CUSTODY_HISTORY_SQL
)
shape=$("${MYSQL[@]}" -e "$query")
[[ "$shape" == 1 ]] || { echo 'auction custody history metadata differs' >&2; exit 1; }
printf 'exact auction custody current uniqueness and retained history schema verified\n'
