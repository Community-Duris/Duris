#!/usr/bin/env bash
set -euo pipefail
# Exact, read-only verifier for the additive activation-receipt schema.
# Never sources checkout .env and never writes to the database.
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
version=$("${MYSQL[@]}" -e 'SELECT VERSION();')
if [[ "$version" == 10.11.*MariaDB* ]]; then
    engine=mariadb
elif [[ "$version" == 8.0.* && "$version" != *MariaDB* ]]; then
    engine=mysql
else
    echo 'unsupported database engine for SQL activation receipt schema' >&2; exit 1
fi

if [[ "$engine" == mariadb ]]; then
    bigint_column_type='bigint(20) unsigned'
    tinyint_column_type='tinyint(3) unsigned'
    smallint_column_type='smallint(5) unsigned'
    created_at_extra=''
else
    bigint_column_type='bigint unsigned'
    tinyint_column_type='tinyint unsigned'
    smallint_column_type='smallint unsigned'
    created_at_extra='DEFAULT_GENERATED'
fi

expect_pair() {
    local label="$1" actual="$2" expected="$3"
    [[ "$actual" == "$expected" ]] || {
        printf 'activation receipt schema mismatch: %s expected=%q actual=%q\n' \
            "$label" "$expected" "$actual" >&2
        exit 1
    }
}

table_shape=$("${MYSQL[@]}" -e "SELECT COUNT(*),COALESCE(SUM(engine='InnoDB' AND table_collation='utf8mb4_unicode_ci'),0) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type='BASE TABLE' AND table_name='economic_sql_activation_receipt';")
expect_pair table-engine-collation "$table_shape" $'1\t1'

columns_shape=$("${MYSQL[@]}" -e "SELECT COUNT(*),COALESCE(SUM(CASE
 WHEN column_name='operation_id' AND ordinal_position=1 AND column_type='binary(16)' AND is_nullable='NO' AND column_default IS NULL AND extra=''
  OR column_name='lineage' AND ordinal_position=2 AND column_type='binary(16)' AND is_nullable='NO' AND column_default IS NULL AND extra=''
  OR column_name='epoch' AND ordinal_position=3 AND column_type='binary(16)' AND is_nullable='NO' AND column_default IS NULL AND extra=''
  OR column_name='baseline_operation_id' AND ordinal_position=4 AND column_type='binary(16)' AND is_nullable='NO' AND column_default IS NULL AND extra=''
  OR column_name='baseline_revision' AND ordinal_position=5 AND column_type='${bigint_column_type}' AND is_nullable='NO' AND column_default IS NULL AND extra=''
  OR column_name='source_capture_digest' AND ordinal_position=6 AND column_type='binary(32)' AND is_nullable='NO' AND column_default IS NULL AND extra=''
  OR column_name='native_boundary_digest' AND ordinal_position=7 AND column_type='binary(32)' AND is_nullable='NO' AND column_default IS NULL AND extra=''
  OR column_name='activation_scope' AND ordinal_position=8 AND column_type='${tinyint_column_type}' AND is_nullable='NO' AND column_default IS NULL AND extra=''
  OR column_name='coverage_contract_version' AND ordinal_position=9 AND column_type='${smallint_column_type}' AND is_nullable='NO' AND column_default IS NULL AND extra=''
  OR column_name='coverage_evidence_digest' AND ordinal_position=10 AND column_type='binary(32)' AND is_nullable='NO' AND column_default IS NULL AND extra=''
  OR column_name='activation_digest' AND ordinal_position=11 AND column_type='binary(32)' AND is_nullable='NO' AND column_default IS NULL AND extra=''
  OR column_name='receipt_version' AND ordinal_position=12 AND column_type='${smallint_column_type}' AND is_nullable='NO' AND column_default IS NULL AND extra=''
  OR column_name='created_at' AND ordinal_position=13 AND column_type='timestamp(6)' AND is_nullable='NO' AND LOWER(COALESCE(column_default,''))='current_timestamp(6)' AND extra='${created_at_extra}'
 THEN 1 ELSE 0 END),0) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='economic_sql_activation_receipt';")
expect_pair columns "$columns_shape" $'13\t13'

receipt_indexes=$("${MYSQL[@]}" -e "SELECT COUNT(*),COALESCE(SUM(CASE WHEN
 (index_name='PRIMARY' AND non_unique=0 AND seq_in_index=1 AND column_name='operation_id') OR
 (index_name='uq_economic_sql_activation_lineage' AND non_unique=0 AND seq_in_index=1 AND column_name='lineage') OR
 (index_name='idx_economic_sql_activation_install_binding' AND non_unique=1 AND ((seq_in_index=1 AND column_name='operation_id') OR (seq_in_index=2 AND column_name='lineage') OR (seq_in_index=3 AND column_name='epoch') OR (seq_in_index=4 AND column_name='baseline_operation_id'))) OR
 (index_name='idx_economic_sql_activation_epoch_revision' AND non_unique=1 AND ((seq_in_index=1 AND column_name='lineage') OR (seq_in_index=2 AND column_name='epoch') OR (seq_in_index=3 AND column_name='baseline_revision'))) OR
 (index_name='idx_economic_sql_activation_baseline_operation' AND non_unique=1 AND seq_in_index=1 AND column_name='baseline_operation_id')
 THEN 1 ELSE 0 END),0),COALESCE(SUM(index_type='BTREE' AND COALESCE(sub_part,0)=0),0) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='economic_sql_activation_receipt';")
expect_pair receipt-indexes "$receipt_indexes" $'10\t10\t10'

binding_index=$("${MYSQL[@]}" -e "SELECT COUNT(*),COALESCE(SUM(non_unique=0 AND seq_in_index BETWEEN 1 AND 4 AND ((seq_in_index=1 AND column_name='operation_id') OR (seq_in_index=2 AND column_name='lineage') OR (seq_in_index=3 AND column_name='epoch') OR (seq_in_index=4 AND column_name='baseline_operation_id')) AND COALESCE(sub_part,0)=0 AND index_type='BTREE'),0) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='economic_sql_lifecycle_installation' AND index_name='uq_economic_sql_lifecycle_activation_binding';")
expect_pair lifecycle-composite-index "$binding_index" $'4\t4'

lifecycle_indexes=$("${MYSQL[@]}" -e "SELECT COUNT(*),COALESCE(SUM(CASE WHEN
 (index_name='PRIMARY' AND non_unique=0 AND seq_in_index=1 AND column_name='operation_id') OR
 (index_name='uq_economic_sql_lifecycle_lineage' AND non_unique=0 AND seq_in_index=1 AND column_name='lineage') OR
 (index_name='uq_economic_sql_lifecycle_epoch' AND non_unique=0 AND ((seq_in_index=1 AND column_name='lineage') OR (seq_in_index=2 AND column_name='epoch'))) OR
 (index_name='uq_economic_sql_lifecycle_activation_binding' AND non_unique=0 AND ((seq_in_index=1 AND column_name='operation_id') OR (seq_in_index=2 AND column_name='lineage') OR (seq_in_index=3 AND column_name='epoch') OR (seq_in_index=4 AND column_name='baseline_operation_id'))) OR
 (index_name='fk_economic_sql_lifecycle_baseline' AND non_unique=1 AND seq_in_index=1 AND column_name='baseline_operation_id')
 THEN 1 ELSE 0 END),0),COALESCE(SUM(index_type='BTREE' AND COALESCE(sub_part,0)=0),0) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='economic_sql_lifecycle_installation';")
expect_pair lifecycle-index-inventory "$lifecycle_indexes" $'9\t9\t9'

foreign_keys=$("${MYSQL[@]}" -e "SELECT COUNT(*),COALESCE(SUM(CASE WHEN
 (k.constraint_name='fk_economic_sql_activation_installation' AND k.ordinal_position BETWEEN 1 AND 4 AND ((k.ordinal_position=1 AND k.column_name='operation_id' AND k.referenced_column_name='operation_id') OR (k.ordinal_position=2 AND k.column_name='lineage' AND k.referenced_column_name='lineage') OR (k.ordinal_position=3 AND k.column_name='epoch' AND k.referenced_column_name='epoch') OR (k.ordinal_position=4 AND k.column_name='baseline_operation_id' AND k.referenced_column_name='baseline_operation_id')) AND k.referenced_table_name='economic_sql_lifecycle_installation' AND k.referenced_table_schema=DATABASE() AND r.update_rule='RESTRICT' AND r.delete_rule='RESTRICT') OR
 (k.constraint_name='fk_economic_sql_activation_epoch' AND k.ordinal_position BETWEEN 1 AND 2 AND ((k.ordinal_position=1 AND k.column_name='lineage' AND k.referenced_column_name='lineage') OR (k.ordinal_position=2 AND k.column_name='epoch' AND k.referenced_column_name='epoch')) AND k.referenced_table_name='economic_epoch' AND k.referenced_table_schema=DATABASE() AND r.update_rule='RESTRICT' AND r.delete_rule='RESTRICT') OR
 (k.constraint_name='fk_economic_sql_activation_baseline_revision' AND k.ordinal_position BETWEEN 1 AND 3 AND ((k.ordinal_position=1 AND k.column_name='lineage' AND k.referenced_column_name='lineage') OR (k.ordinal_position=2 AND k.column_name='epoch' AND k.referenced_column_name='epoch') OR (k.ordinal_position=3 AND k.column_name='baseline_revision' AND k.referenced_column_name='book_revision')) AND k.referenced_table_name='economic_baseline_witness' AND k.referenced_table_schema=DATABASE() AND r.update_rule='RESTRICT' AND r.delete_rule='RESTRICT') OR
 (k.constraint_name='fk_economic_sql_activation_operation_inbox' AND k.ordinal_position=1 AND k.column_name='operation_id' AND k.referenced_column_name='operation_id' AND k.referenced_table_name='critical_operation_inbox' AND k.referenced_table_schema=DATABASE() AND r.update_rule='RESTRICT' AND r.delete_rule='RESTRICT') OR
 (k.constraint_name='fk_economic_sql_activation_baseline_inbox' AND k.ordinal_position=1 AND k.column_name='baseline_operation_id' AND k.referenced_column_name='operation_id' AND k.referenced_table_name='critical_operation_inbox' AND k.referenced_table_schema=DATABASE() AND r.update_rule='RESTRICT' AND r.delete_rule='RESTRICT')
 THEN 1 ELSE 0 END),0) FROM information_schema.key_column_usage k JOIN information_schema.referential_constraints r ON r.constraint_schema=k.constraint_schema AND r.table_name=k.table_name AND r.constraint_name=k.constraint_name WHERE k.constraint_schema=DATABASE() AND k.table_name='economic_sql_activation_receipt' AND k.referenced_table_name IS NOT NULL;")
expect_pair foreign-keys "$foreign_keys" $'11\t11'

checks=$("${MYSQL[@]}" -e "SELECT COUNT(*),COALESCE(SUM(CASE WHEN
 (t.constraint_name='ck_economic_sql_activation_operation_nonzero' AND LOWER(REPLACE(REPLACE(REPLACE(REPLACE(c.check_clause,CHAR(96),''),' ',''),'(',''),')',''))='operation_id<>0x00000000000000000000000000000000') OR
 (t.constraint_name='ck_economic_sql_activation_lineage_nonzero' AND LOWER(REPLACE(REPLACE(REPLACE(REPLACE(c.check_clause,CHAR(96),''),' ',''),'(',''),')',''))='lineage<>0x00000000000000000000000000000000') OR
 (t.constraint_name='ck_economic_sql_activation_epoch_nonzero' AND LOWER(REPLACE(REPLACE(REPLACE(REPLACE(c.check_clause,CHAR(96),''),' ',''),'(',''),')',''))='epoch<>0x00000000000000000000000000000000') OR
 (t.constraint_name='ck_economic_sql_activation_baseline_operation_nonzero' AND LOWER(REPLACE(REPLACE(REPLACE(REPLACE(c.check_clause,CHAR(96),''),' ',''),'(',''),')',''))='baseline_operation_id<>0x00000000000000000000000000000000') OR
 (t.constraint_name='ck_economic_sql_activation_baseline_revision' AND LOWER(REPLACE(REPLACE(REPLACE(REPLACE(c.check_clause,CHAR(96),''),' ',''),'(',''),')',''))='baseline_revision>0') OR
 (t.constraint_name='ck_economic_sql_activation_scope' AND LOWER(REPLACE(REPLACE(REPLACE(REPLACE(c.check_clause,CHAR(96),''),' ',''),'(',''),')',''))='activation_scope=1') OR
 (t.constraint_name='ck_economic_sql_activation_coverage_version' AND LOWER(REPLACE(REPLACE(REPLACE(REPLACE(c.check_clause,CHAR(96),''),' ',''),'(',''),')',''))='coverage_contract_version=1') OR
 (t.constraint_name='ck_economic_sql_activation_receipt_version' AND LOWER(REPLACE(REPLACE(REPLACE(REPLACE(c.check_clause,CHAR(96),''),' ',''),'(',''),')',''))='receipt_version=1')
 THEN 1 ELSE 0 END),0) FROM information_schema.table_constraints t JOIN information_schema.check_constraints c ON c.constraint_schema=t.constraint_schema AND c.constraint_name=t.constraint_name WHERE t.constraint_schema=DATABASE() AND t.table_name='economic_sql_activation_receipt' AND t.constraint_type='CHECK';")
expect_pair check-constraints "$checks" $'8\t8'
if [[ "$engine" == mysql ]]; then
    enforced=$("${MYSQL[@]}" -e "SELECT COUNT(*),COALESCE(SUM(enforced='YES'),0) FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='economic_sql_activation_receipt' AND constraint_type='CHECK';")
    expect_pair enforced-checks "$enforced" $'8\t8'
fi

echo 'SQL activation receipt schema verified: exact columns, indexes, composite lifecycle/epoch/witness/inbox foreign keys and enforced v1 checks'
