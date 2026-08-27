#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
if [[ -z "${DB_HOST:-}" ]]; then
    # shellcheck disable=SC1091
    source "$PROJECT_ROOT/.env"
fi
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")

output=$("${MYSQL[@]}" -e "
SELECT 'boon_entry_count_mismatch', COUNT(*)
FROM boon_reward_outcome o
LEFT JOIN (SELECT operation_id,COUNT(*) count FROM boon_reward_outcome_entry GROUP BY operation_id) e USING(operation_id)
WHERE o.entry_count<>COALESCE(e.count,0)
UNION ALL
SELECT 'zone_participant_count_mismatch', COUNT(*)
FROM zone_touch_outcome o
LEFT JOIN (SELECT operation_id,COUNT(*) count FROM zone_touch_outcome_participant GROUP BY operation_id) p USING(operation_id)
WHERE o.group_size<>COALESCE(p.count,0)
UNION ALL
SELECT 'missing_committed_inbox', COUNT(*)
FROM (
  SELECT operation_id FROM boon_reward_outcome UNION ALL SELECT operation_id FROM zone_touch_outcome
) outcomes LEFT JOIN critical_operation_inbox i USING(operation_id)
WHERE i.operation_id IS NULL OR i.status<>1;")
printf '%s\n' "$output"
awk 'BEGIN { ok=1 } $2 != 0 { ok=0 } END { exit ok ? 0 : 1 }' <<< "$output"
