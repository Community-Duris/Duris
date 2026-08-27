#!/usr/bin/env bash
set -euo pipefail

[[ "${1:-}" == "--apply" ]] || { echo 'usage: baseline_artifact_guild_state.sh --apply' >&2; exit 2; }
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
# shellcheck disable=SC1091
source "$PROJECT_ROOT/.env"
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
environment_name="${ENVIRONMENT:-${APP_ENV:-}}"
[[ "${environment_name,,}" =~ (dev|local|test) && "${DB_NAME,,}" =~ (dev|local|test) ]] || {
    echo 'refusing artifact/guild baseline: environment and database must be development/local/test' >&2
    exit 1
}
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
read_scalar() { "${MYSQL[@]}" -e "$1"; }

advanced=$(read_scalar 'SELECT (SELECT COUNT(*) FROM artifact_delta_ledger)+(SELECT COUNT(*) FROM guild_outcome_ledger)+(SELECT COUNT(*) FROM guilds WHERE outcome_revision<>0);')
if [[ "$advanced" != "0" ]]; then
    echo "artifact/guild authority already advanced; refusing baseline"
    exit 1
fi

"${MYSQL[@]}" -e "INSERT INTO artifact_domain_state(vnum,owned,loc_type,location,timer_epoch,artifact_type,bind_owner_pid,bind_timer_epoch) SELECT a.vnum,a.owned='Y',a.locType,COALESCE(a.location,0),COALESCE(UNIX_TIMESTAMP(a.timer),0),COALESCE(a.type,0),COALESCE(b.owner_pid,0),COALESCE(b.timer,0) FROM artifacts a LEFT JOIN artifact_bind b ON b.vnum=a.vnum ON DUPLICATE KEY UPDATE owned=VALUES(owned),loc_type=VALUES(loc_type),location=VALUES(location),timer_epoch=VALUES(timer_epoch),artifact_type=VALUES(artifact_type),bind_owner_pid=VALUES(bind_owner_pid),bind_timer_epoch=VALUES(bind_timer_epoch); INSERT IGNORE INTO artifact_domain_baseline(vnum,opening_timer_epoch,opening_bind_owner_pid,opening_bind_timer_epoch,opening_revision) SELECT vnum,timer_epoch,bind_owner_pid,bind_timer_epoch,revision FROM artifact_domain_state;"
