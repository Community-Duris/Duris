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
# This verifier only inspects metadata; it never sources .env or changes rows.
query=$(cat <<'DURIS_QUEST_MOBILE_SCHEMA_SQL'
SELECT (
    (SELECT COUNT(*) FROM information_schema.tables
        WHERE table_schema=DATABASE() AND table_name='quest_mobile_native'
          AND table_type='BASE TABLE' AND engine='InnoDB' AND auto_increment IS NULL) = 1
    AND (SELECT COUNT(*) FROM information_schema.columns
        WHERE table_schema=DATABASE() AND table_name='quest_mobile_native') = 5
    AND (SELECT COUNT(*) FROM information_schema.columns
        WHERE table_schema=DATABASE() AND table_name='quest_mobile_native'
          AND is_nullable='NO' AND column_default IS NULL AND extra=''
          AND column_type NOT LIKE '%zerofill%'
          AND ((ordinal_position=1 AND column_name='mobile_instance_id'
                AND data_type='bigint' AND column_type LIKE '%unsigned%')
            OR (ordinal_position=2 AND column_name='mobile_revision'
                AND data_type='bigint' AND column_type LIKE '%unsigned%')
            OR (ordinal_position=3 AND column_name='stock_revision'
                AND data_type='bigint' AND column_type LIKE '%unsigned%')
            OR (ordinal_position=4 AND column_name='lifetime_state'
                AND data_type='tinyint' AND column_type LIKE '%unsigned%')
            OR (ordinal_position=5 AND column_name='canonical_image'
                AND data_type='mediumblob'))) = 5
    AND (SELECT COUNT(*) FROM information_schema.statistics
        WHERE table_schema=DATABASE() AND table_name='quest_mobile_native') = 1
    AND (SELECT COUNT(*) FROM information_schema.statistics
        WHERE table_schema=DATABASE() AND table_name='quest_mobile_native'
          AND index_name='PRIMARY' AND non_unique=0 AND seq_in_index=1
          AND column_name='mobile_instance_id' AND sub_part IS NULL
          AND index_type='BTREE' AND collation='A') = 1
    AND (SELECT COUNT(*) FROM information_schema.table_constraints
        WHERE table_schema=DATABASE() AND table_name='quest_mobile_native') = 1
    AND (SELECT COUNT(*) FROM information_schema.table_constraints
        WHERE table_schema=DATABASE() AND table_name='quest_mobile_native'
          AND constraint_name='PRIMARY' AND constraint_type='PRIMARY KEY') = 1
    AND (SELECT COUNT(*) FROM information_schema.key_column_usage
        WHERE table_schema=DATABASE() AND table_name='quest_mobile_native') = 1
    AND (SELECT COUNT(*) FROM information_schema.key_column_usage
        WHERE table_schema=DATABASE() AND table_name='quest_mobile_native'
          AND constraint_name='PRIMARY' AND column_name='mobile_instance_id'
          AND ordinal_position=1 AND referenced_table_name IS NULL) = 1
    AND (SELECT COUNT(*) FROM information_schema.partitions
        WHERE table_schema=DATABASE() AND table_name='quest_mobile_native'
          AND partition_name IS NOT NULL) = 0
);
DURIS_QUEST_MOBILE_SCHEMA_SQL
)
shape=$("${MYSQL[@]}" -e "$query")
[[ "$shape" == 1 ]] || { echo 'quest mobile native schema differs' >&2; exit 1; }
printf 'exact quest mobile native schema verified\n'
