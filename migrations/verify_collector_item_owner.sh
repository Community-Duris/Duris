#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
if [[ -z "${DB_HOST:-}" || -z "${DB_USER:-}" || -z "${DB_PASSWD:-}" || -z "${DB_NAME:-}" ]]; then
    if [[ -f "$PROJECT_ROOT/.env" ]]; then
        # shellcheck disable=SC1091
        source "$PROJECT_ROOT/.env"
    fi
fi
: "${DB_HOST:?DB_HOST is required}"
: "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}"
: "${DB_NAME:?DB_NAME is required}"

export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
read_scalar() { "${MYSQL[@]}" -e "$1"; }

# MariaDB may return duplicate metadata rows for a named CHECK. Count the
# three distinct constraints, then verify their behavior with sentinel writes.
collector_constraints=$(read_scalar "SELECT COUNT(DISTINCT constraint_name) FROM information_schema.check_constraints WHERE constraint_schema=DATABASE() AND constraint_name IN ('chk_item_owner_revision_type','chk_item_current_owner_type','chk_item_baseline_owner_type') AND LOWER(REPLACE(check_clause,CHAR(96),'')) REGEXP 'owner_type[[:space:]]+between[[:space:]]+1[[:space:]]+and[[:space:]]+10([^0-9]|$)';")
pet_constraints=$(read_scalar "SELECT COUNT(DISTINCT constraint_name) FROM information_schema.check_constraints WHERE constraint_schema=DATABASE() AND constraint_name IN ('chk_item_owner_revision_type','chk_item_current_owner_type','chk_item_baseline_owner_type') AND LOWER(REPLACE(check_clause,CHAR(96),'')) REGEXP 'owner_type[[:space:]]+between[[:space:]]+1[[:space:]]+and[[:space:]]+11([^0-9]|$)';")
pet_column=$(read_scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='player_pets' AND column_name='pet_uid';")
if [[ "$collector_constraints" == 3 && "$pet_constraints" == 0 ]]; then
    max_owner_type=10
elif [[ "$pet_constraints" == 3 && "$collector_constraints" == 0 && "$pet_column" == 1 ]]; then
    max_owner_type=11
else
    echo "FAILED: expected three uniform collector (10) or pet-enabled (11) ownership constraints" >&2
    exit 1
fi

sentinel_owner_id=18446744073709551614
"${MYSQL[@]}" -e "START TRANSACTION; DELETE FROM item_owner_revision WHERE owner_type=10 AND owner_id=$sentinel_owner_id AND owner_context_id=0; INSERT INTO item_owner_revision (owner_type,owner_id,owner_context_id,revision) VALUES (10,$sentinel_owner_id,0,0); ROLLBACK;"
unknown_owner_type=$((max_owner_type + 1))
if "${MYSQL[@]}" -e "START TRANSACTION; INSERT INTO item_owner_revision (owner_type,owner_id,owner_context_id,revision) VALUES ($unknown_owner_type,$sentinel_owner_id,0,0); ROLLBACK;" >/dev/null 2>&1; then
    echo "FAILED: ownership constraints accepted unknown owner type $unknown_owner_type" >&2
    exit 1
fi

printf 'collector item custody verified: 3 constraints accept type 10, max type %s, reject type %s\n' "$max_owner_type" "$unknown_owner_type"
