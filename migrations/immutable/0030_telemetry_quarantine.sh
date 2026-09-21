#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?}" "${DB_USER:?}" "${DB_PASSWD:?}" "${DB_NAME:?}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
scalar() { "${MYSQL[@]}" -e "$1"; }

shape=$(scalar "SELECT CONCAT(engine,':',table_collation) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='telemetry_quarantine' AND table_type='BASE TABLE';")
[[ "$shape" == "InnoDB:utf8mb4_unicode_ci" ]] || {
    echo "FAILED: telemetry_quarantine engine/collation differs: $shape" >&2
    exit 1
}

columns=$(scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_quarantine';")
[[ "$columns" == 13 ]] || {
    echo "FAILED: telemetry_quarantine column inventory differs: $columns" >&2
    exit 1
}

primary=$(scalar "SELECT GROUP_CONCAT(column_name ORDER BY seq_in_index) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='telemetry_quarantine' AND index_name='PRIMARY' AND non_unique=0;")
[[ "$primary" == "boot_id,process_id,record_seq" ]] || {
    echo "FAILED: telemetry_quarantine primary key differs: $primary" >&2
    exit 1
}

payload=$(scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_quarantine' AND ((column_name='payload_sha256' AND data_type='binary' AND character_maximum_length=32 AND is_nullable='NO') OR (column_name='record_payload' AND data_type='blob' AND is_nullable='NO') OR (column_name='recovery_state' AND data_type='tinyint' AND column_type LIKE '%unsigned' AND is_nullable='NO' AND column_default='0'));")
[[ "$payload" == 3 ]] || {
    echo "FAILED: telemetry_quarantine payload/recovery shape differs" >&2
    exit 1
}

printf 'telemetry quarantine migration verified: durable replay identity and payload shape are exact\n'
