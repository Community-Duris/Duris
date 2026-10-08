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
# Read-only exact native constructor-origin metadata. No receipts are invented.
version=$("${MYSQL[@]}" -e 'SELECT VERSION();')
if [[ "$version" == 10.11.*MariaDB* ]]; then
    checks=$("${MYSQL[@]}" -e 'SELECT @@SESSION.check_constraint_checks;')
    [[ "$checks" == 1 ]] || { echo 'native birth origin checks disabled' >&2; exit 1; }
elif [[ "$version" != 8.0.* || "$version" == *MariaDB* ]]; then
    echo 'unsupported native birth origin schema engine' >&2; exit 1
fi
query=$(cat <<'DURIS_NATIVE_BIRTH_ORIGIN_SCHEMA_SQL'
SELECT (
        (SELECT COUNT(*) FROM information_schema.tables
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin'
              AND table_type='BASE TABLE' AND engine='InnoDB' AND auto_increment IS NULL)=1
        AND (SELECT COUNT(*) FROM information_schema.columns
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin')=4
        AND (SELECT COUNT(*) FROM information_schema.columns
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin'
              AND is_nullable='NO' AND column_default IS NULL AND extra=''
              AND ((ordinal_position IN (1,3) AND numeric_precision=20 AND numeric_scale=0
                    AND column_name=IF(ordinal_position=1,'mobile_instance_id','publication_revision')
                    AND data_type='bigint' AND column_type IN ('bigint unsigned','bigint(20) unsigned'))
                OR (ordinal_position=2 AND column_name='birth_operation' AND data_type='binary'
                    AND character_maximum_length=16 AND character_octet_length=16)
                OR (ordinal_position=4 AND column_name='canonical_origin' AND data_type='longblob')))=4
        AND (SELECT COUNT(*) FROM information_schema.statistics
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin')=2
        AND (SELECT COUNT(*) FROM information_schema.statistics
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin'
              AND non_unique=0 AND seq_in_index=1 AND sub_part IS NULL
              AND index_type='BTREE' AND collation='A'
              AND ((index_name='PRIMARY' AND column_name='mobile_instance_id')
                OR (index_name='uq_native_birth_origin_operation' AND column_name='birth_operation')))=2
        AND (SELECT COUNT(*) FROM information_schema.table_constraints
            WHERE constraint_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin')=5
        AND (SELECT COUNT(*) FROM information_schema.table_constraints
            WHERE constraint_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin'
              AND ((constraint_name='PRIMARY' AND constraint_type='PRIMARY KEY')
                OR (constraint_name='uq_native_birth_origin_operation' AND constraint_type='UNIQUE')
                OR (constraint_name IN ('fk_native_birth_origin_lifetime','fk_native_birth_origin_inbox')
                    AND constraint_type='FOREIGN KEY')
                OR (constraint_name='ck_native_birth_origin_bound' AND constraint_type='CHECK')))=5
        AND (SELECT COUNT(*) FROM information_schema.key_column_usage
            WHERE constraint_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin')=4
        AND (SELECT COUNT(*) FROM information_schema.key_column_usage
            WHERE constraint_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin'
              AND ordinal_position=1
              AND ((constraint_name='PRIMARY' AND column_name='mobile_instance_id'
                    AND referenced_table_name IS NULL)
                OR (constraint_name='uq_native_birth_origin_operation' AND column_name='birth_operation'
                    AND referenced_table_name IS NULL)
                OR (constraint_name='fk_native_birth_origin_lifetime' AND column_name='mobile_instance_id'
                    AND referenced_table_schema=DATABASE() AND referenced_table_name='quest_mobile_native'
                    AND referenced_column_name='mobile_instance_id' AND position_in_unique_constraint=1)
                OR (constraint_name='fk_native_birth_origin_inbox' AND column_name='birth_operation'
                    AND referenced_table_schema=DATABASE() AND referenced_table_name='critical_operation_inbox'
                    AND referenced_column_name='operation_id' AND position_in_unique_constraint=1)))=4
        AND (SELECT COUNT(*) FROM information_schema.referential_constraints
            WHERE constraint_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin'
              AND unique_constraint_schema=DATABASE() AND unique_constraint_name='PRIMARY'
              AND update_rule='RESTRICT' AND delete_rule='RESTRICT'
              AND ((constraint_name='fk_native_birth_origin_lifetime' AND referenced_table_name='quest_mobile_native')
                OR (constraint_name='fk_native_birth_origin_inbox' AND referenced_table_name='critical_operation_inbox')))=2
        AND (SELECT COUNT(*) FROM information_schema.check_constraints
            WHERE constraint_schema=DATABASE() AND constraint_name='ck_native_birth_origin_bound'
              AND REPLACE(LOWER(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(check_clause,
                  CHAR(96),''),' ',''),CHAR(9),''),CHAR(10),''),CHAR(13),''),'(',''),')','')),
                  'octet_length','length')=
                  'mobile_instance_id>0andpublication_revision>0andbirth_operation<>0x00000000000000000000000000000000andlengthcanonical_origin>0andlengthcanonical_origin<=33554432')=1
        AND (SELECT COUNT(*) FROM information_schema.partitions
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin'
              AND partition_name IS NOT NULL)=0
);
DURIS_NATIVE_BIRTH_ORIGIN_SCHEMA_SQL
)
shape=$("${MYSQL[@]}" -e "$query")
[[ "$shape" == 1 ]] || { echo 'native birth origin schema differs' >&2; exit 1; }
if [[ "$version" != *MariaDB* ]]; then
    enforced=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin' AND constraint_name='ck_native_birth_origin_bound' AND constraint_type='CHECK' AND enforced='YES';")
    [[ "$enforced" == 1 ]] || { echo 'native birth origin checks not enforced' >&2; exit 1; }
fi
printf 'exact native birth origin schema verified\n'
