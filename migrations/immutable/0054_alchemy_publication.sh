#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
bash "$ROOT/immutable/0053_craft_progression.sh"
export MYSQL_PWD="${DB_PASSWD:?}"
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
clause=$(timeout 30 mysql --no-defaults "${CONNECTION[@]}" -u "${DB_USER:?}" -N -B "${DB_NAME:?}" -e "SELECT c.check_clause FROM information_schema.check_constraints c JOIN information_schema.table_constraints t ON t.constraint_schema=c.constraint_schema AND t.constraint_name=c.constraint_name WHERE t.constraint_schema=DATABASE() AND t.table_name='player_craft_progression' AND t.constraint_name='ck_player_craft_progression_terms';")
normalized=$(printf '%s' "$clause" | tr '[:upper:]' '[:lower:]' | tr -d '\140()[:space:]')
[[ "$normalized" == 'pid>0anddisciplinein1,2,3,4,5,6andexperience<=2147483647anddisciplinein1,2orexperience=0' ]] || {
    echo 'alchemy publication receipt constraint differs' >&2; exit 1;
}
printf 'alchemy publication receipts verified\n'
