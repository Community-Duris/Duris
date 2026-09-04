#!/usr/bin/env bash
set -euo pipefail

expected_count=""
case "${1:-}" in
    "") ;;
    --expect-count)
        [[ "${2:-}" =~ ^[0-9]+$ && $# -eq 2 ]] || {
            echo 'usage: check_flatfile_account_locker_conversion.sh [--expect-count N]' >&2
            exit 2
        }
        expected_count="$2"
        ;;
    *)
        echo 'usage: check_flatfile_account_locker_conversion.sh [--expect-count N]' >&2
        exit 2
        ;;
esac

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
if [[ -z "${DB_HOST:-}" ]]; then
    # shellcheck disable=SC1091
    source "$PROJECT_ROOT/.env"
fi
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
MYSQL_SSL=()
if [[ "$DB_HOST" != "localhost" && "$DB_HOST" != "127.0.0.1" && "$DB_HOST" != "::1" ]]; then
    [[ "${DB_TLS:-}" == "TRUE" && -f "${DB_SSL_CA:-}" ]] || {
        echo 'remote account locker conversion check requires TLS and a CA file' >&2
        exit 2
    }
    if mysql --help 2>&1 | grep -q -- '--ssl-mode'; then
        MYSQL_SSL=(--ssl-mode=VERIFY_IDENTITY --ssl-ca="$DB_SSL_CA")
    elif mysql --help 2>&1 | grep -q -- '--ssl-verify-server-cert'; then
        MYSQL_SSL=(--ssl-ca="$DB_SSL_CA" --ssl-verify-server-cert)
    else
        echo 'database client cannot verify the remote server identity' >&2
        exit 2
    fi
fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")

# Read-only and aggregate-only: no account, character, locker, chest, or item identity
# is emitted. The owner map is built from authoritative accounts plus the finite
# racewar-side domain; a locker display name never creates an identity by itself.
query="
SELECT
    COUNT(*) AS account_lockers,
    COALESCE(SUM(owner_map.account_name IS NULL), 0) AS unmatched_typed_owner,
    COALESCE(SUM(COALESCE(l.owner_pid, 0) <> 0 OR
                 COALESCE(l.owner_assoc_id, 0) <> 0), 0) AS invalid_owner_fields,
    COALESCE(SUM((SELECT COUNT(*) FROM private_chests pc WHERE pc.locker_id=l.id) = 0 OR
                 (SELECT COUNT(*) FROM private_chests pc
                  WHERE pc.locker_id=l.id AND pc.is_public=1) <> 1 OR
                 EXISTS (SELECT 1 FROM private_chests pc
                         WHERE pc.locker_id=l.id AND
                               (pc.chest_name = '' OR
                                BINARY pc.chest_name <> BINARY LOWER(pc.chest_name) OR
                                OCTET_LENGTH(pc.password_hash) > 64 OR
                                OCTET_LENGTH(pc.sort_config) > 4096 OR
                                (pc.is_public=1 AND COALESCE(pc.password_hash, '') <> '')))), 0)
        AS invalid_chest_shape,
    COALESCE(SUM(EXISTS (
        SELECT 1 FROM item_current_owner current_item
        WHERE current_item.owner_type=5 AND current_item.owner_id=l.id AND
              current_item.state=1 AND
              (current_item.item_uid=0 OR
               (SELECT COUNT(*) FROM locker_items li
                JOIN private_chests pc
                  ON pc.id=li.chest_id AND pc.locker_id=l.id
                WHERE li.locker_id=l.id AND li.chest_id=current_item.owner_context_id AND
                      li.obj_uid=current_item.item_uid AND li.vnum > 0) <> 1))), 0)
        AS invalid_item_shape,
    COALESCE(SUM(EXISTS (
        SELECT 1 FROM item_current_owner current_item
        JOIN item_current_owner duplicate
          ON duplicate.item_uid=current_item.item_uid AND
             duplicate.state=1 AND
             (duplicate.owner_type<>current_item.owner_type OR
              duplicate.owner_id<>current_item.owner_id OR
              duplicate.owner_context_id<>current_item.owner_context_id)
        WHERE current_item.owner_type=5 AND current_item.owner_id=l.id AND
              current_item.state=1)), 0) AS duplicate_item_uid_lockers,
    (SELECT COUNT(*) FROM locker_access access_row
     LEFT JOIN lockers access_locker
       ON BINARY LOWER(access_locker.locker_name)=BINARY LOWER(access_row.owner)
     WHERE LOWER(access_row.owner) LIKE 'account.%' AND access_locker.id IS NULL)
        AS orphan_access
FROM lockers l
LEFT JOIN (
    SELECT LOWER(a.account_name) AS account_name, sides.racewar_side,
           LOWER(CONCAT('account.', a.account_name, '.', sides.racewar_side, '.locker'))
               AS locker_name
    FROM accounts a
    CROSS JOIN (
        SELECT 0 AS racewar_side UNION ALL SELECT 1 UNION ALL SELECT 2
        UNION ALL SELECT 3 UNION ALL SELECT 4
    ) sides
) owner_map
  ON BINARY owner_map.locker_name=BINARY LOWER(l.locker_name)
 AND owner_map.racewar_side=l.racewar
WHERE LOWER(l.locker_name) LIKE 'account.%';"

if ! result=$("${MYSQL[@]}" -e "$query"); then
    echo 'account locker conversion check failed to query authority' >&2
    exit 2
fi
IFS=$'\t' read -r account_lockers unmatched_typed_owner invalid_owner_fields \
    invalid_chest_shape invalid_item_shape duplicate_item_uid_lockers orphan_access <<<"$result"
for value in "$account_lockers" "$unmatched_typed_owner" "$invalid_owner_fields" \
    "$invalid_chest_shape" "$invalid_item_shape" "$duplicate_item_uid_lockers" \
    "$orphan_access"; do
    [[ "$value" =~ ^[0-9]+$ ]] || {
        echo 'account locker conversion check returned malformed aggregate output' >&2
        exit 2
    }
done

printf 'account_lockers=%s\nunmatched_typed_owner=%s\ninvalid_owner_fields=%s\n' \
    "$account_lockers" "$unmatched_typed_owner" "$invalid_owner_fields"
printf 'invalid_chest_shape=%s\ninvalid_item_shape=%s\nduplicate_item_uid_lockers=%s\n' \
    "$invalid_chest_shape" "$invalid_item_shape" "$duplicate_item_uid_lockers"
printf 'orphan_access=%s\n' "$orphan_access"

[[ -z "$expected_count" || "$account_lockers" == "$expected_count" ]] || exit 1
[[ "$unmatched_typed_owner" == 0 && "$invalid_owner_fields" == 0 &&
   "$invalid_chest_shape" == 0 && "$invalid_item_shape" == 0 &&
   "$duplicate_item_uid_lockers" == 0 && "$orphan_access" == 0 ]]
