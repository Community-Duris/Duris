#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
# shellcheck disable=SC1091
source "$PROJECT_ROOT/.env"
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
read_scalar() { "${MYSQL[@]}" -e "$1"; }

artifact_mismatch=$(read_scalar 'SELECT COUNT(*) FROM artifact_domain_state s JOIN artifact_domain_baseline b ON b.vnum=s.vnum LEFT JOIN (SELECT vnum,SUM(timer_delta) delta FROM artifact_delta_ledger GROUP BY vnum) l ON l.vnum=s.vnum WHERE b.opening_timer_epoch+COALESCE(l.delta,0)<>s.timer_epoch;')
legacy_mismatch=$(read_scalar "SELECT COUNT(*) FROM artifact_domain_state s JOIN artifacts a ON a.vnum=s.vnum WHERE s.timer_epoch<>COALESCE(UNIX_TIMESTAMP(a.timer),0) OR s.bind_owner_pid<>COALESCE((SELECT owner_pid FROM artifact_bind b WHERE b.vnum=s.vnum),0) OR s.bind_timer_epoch<>COALESCE((SELECT timer FROM artifact_bind b WHERE b.vnum=s.vnum),0);")
guild_mismatch=$(read_scalar 'SELECT COUNT(*) FROM guild_outcome_ledger l JOIN guilds g ON g.id=l.guild_id WHERE l.guild_revision=g.outcome_revision AND (l.prestige_after<>g.prestige OR l.construction_after<>g.construction);')
echo "artifact_ledger_mismatches=$artifact_mismatch artifact_legacy_mismatches=$legacy_mismatch guild_latest_mismatches=$guild_mismatch"
[[ "$artifact_mismatch" == "0" && "$legacy_mismatch" == "0" && "$guild_mismatch" == "0" ]]
