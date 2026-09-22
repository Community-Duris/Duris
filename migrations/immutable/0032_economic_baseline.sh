#!/usr/bin/env bash
set -euo pipefail
# Self-contained immutable metadata verifier. Never sources checkout .env.
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
    expected=778e7d3815bc4c66d2bb13072c9bc6689df9008a332bee02b5fd3c84548e0e3e
elif [[ "$version" == 8.0.* && "$version" != *MariaDB* ]]; then
    engine=mysql
    expected=f0551ebf630d1e18f4bdec863f239da3d974f783f3acafbc243483b8e67bf3bc
else
    echo 'unsupported database engine for baseline retention schema' >&2; exit 1
fi
query=$(cat <<'DURIS_METADATA_SQL'

SELECT CONCAT('T',CHAR(9),table_name,CHAR(9),engine,CHAR(9),table_collation)
FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type='BASE TABLE' AND table_name IN ('economic_baseline_control','economic_baseline_witness','economic_baseline_reservation')
UNION ALL
SELECT CONCAT('C',CHAR(9),table_name,CHAR(9),column_name,CHAR(9),ordinal_position,
 CHAR(9),column_type,CHAR(9),is_nullable,CHAR(9),COALESCE(character_maximum_length,0),
 CHAR(9),COALESCE(numeric_precision,0),CHAR(9),COALESCE(numeric_scale,0),
 CHAR(9),COALESCE(datetime_precision,0),CHAR(9),COALESCE(column_default,'<NULL>'),CHAR(9),extra)
FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name IN ('economic_baseline_control','economic_baseline_witness','economic_baseline_reservation')
UNION ALL
SELECT CONCAT('I',CHAR(9),table_name,CHAR(9),index_name,CHAR(9),non_unique,CHAR(9),seq_in_index,
 CHAR(9),column_name,CHAR(9),COALESCE(sub_part,0),CHAR(9),index_type)
FROM information_schema.statistics WHERE table_schema=DATABASE() AND (table_name IN ('economic_baseline_control','economic_baseline_witness','economic_baseline_reservation'))
UNION ALL
SELECT CONCAT('F',CHAR(9),k.table_name,CHAR(9),k.constraint_name,CHAR(9),k.column_name,
 CHAR(9),k.referenced_table_name,CHAR(9),k.referenced_column_name,CHAR(9),k.ordinal_position,
 CHAR(9),r.update_rule,CHAR(9),r.delete_rule)
FROM information_schema.key_column_usage k JOIN information_schema.referential_constraints r
 ON r.constraint_schema=k.constraint_schema AND r.constraint_name=k.constraint_name
WHERE k.constraint_schema=DATABASE() AND k.table_name IN ('economic_baseline_control','economic_baseline_witness','economic_baseline_reservation') AND k.referenced_table_name IS NOT NULL
UNION ALL
SELECT CONCAT('K',CHAR(9),t.table_name,CHAR(9),t.constraint_name,CHAR(9),c.check_clause)
FROM information_schema.table_constraints t JOIN information_schema.check_constraints c
 ON c.constraint_schema=t.constraint_schema AND c.constraint_name=t.constraint_name
WHERE t.constraint_schema=DATABASE() AND t.table_name IN ('economic_baseline_control','economic_baseline_witness','economic_baseline_reservation') AND t.constraint_type='CHECK'
ORDER BY 1;

DURIS_METADATA_SQL
)
enforced=$(cat <<'DURIS_ENFORCED_SQL'
SELECT CONCAT('E',CHAR(9),table_name,CHAR(9),constraint_name,CHAR(9),enforced) FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name IN ('economic_baseline_control','economic_baseline_witness','economic_baseline_reservation') AND constraint_type='CHECK' ORDER BY 1;
DURIS_ENFORCED_SQL
)
actual=$({ "${MYSQL[@]}" -e "$query"; if [[ "$engine" == mysql ]]; then "${MYSQL[@]}" -e "$enforced"; fi; } | sha256sum | cut -d' ' -f1)
[[ "$actual" == "$expected" ]] || { echo 'baseline retention schema metadata fingerprint mismatch' >&2; exit 1; }
echo 'baseline retention schema verified: 3 InnoDB tables, exact columns, indexes, foreign keys and checks'
