#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo 'usage: reconcile_coin_custody_pair.sh --classify /absolute/private/pair.tsv' >&2
    echo '   or: reconcile_coin_custody_pair.sh --apply /absolute/private/pair.tsv SHA256' >&2
    exit 2
}

mode="${1:-}"
artifact="${2:-}"
[[ "$mode" == --classify || "$mode" == --apply ]] || usage
[[ "$artifact" == /* ]] || usage
if [[ "$mode" == --classify ]]; then
    [[ $# -eq 2 ]] || usage
else
    [[ $# -eq 3 && "${3:-}" =~ ^[0-9a-f]{64}$ ]] || usage
fi

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
if [[ "$DB_HOST" != localhost && "$DB_HOST" != 127.0.0.1 && "$DB_HOST" != ::1 ]]; then
    [[ "${DB_TLS:-}" == TRUE && -f "${DB_SSL_CA:-}" ]] || {
        echo 'remote coin custody classification requires TLS and a CA file' >&2
        exit 1
    }
    if mysql --help 2>&1 | grep -q -- '--ssl-mode'; then
        MYSQL_SSL=(--ssl-mode=VERIFY_IDENTITY --ssl-ca="$DB_SSL_CA")
    elif mysql --help 2>&1 | grep -q -- '--ssl-verify-server-cert'; then
        MYSQL_SSL=(--ssl-ca="$DB_SSL_CA" --ssl-verify-server-cert)
    else
        echo 'database client cannot verify the remote server identity' >&2
        exit 1
    fi
fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")

artifact_parent="$(dirname "$artifact")"
[[ -d "$artifact_parent" && ! -L "$artifact_parent" ]] || {
    echo 'pair artifact parent must be an existing directory' >&2
    exit 2
}
parent_mode=$(stat -c '%a' "$artifact_parent")
parent_uid=$(stat -c '%u' "$artifact_parent")
[[ "$parent_mode" =~ ^[0-7]+$ && "$parent_uid" == "$(id -u)" ]] || {
    echo 'pair artifact parent has invalid ownership or mode' >&2
    exit 2
}
(( (8#$parent_mode & 077) == 0 )) || {
    echo 'pair artifact parent must not grant group or other access' >&2
    exit 2
}

pair_query="
SELECT payload.pid,payload.id,payload.obj_uid,old_owner.item_uid,payload.vnum,
       COALESCE(payload.container_id,0),COALESCE(parent_payload.obj_uid,0),
       old_owner.root_item_uid,COALESCE(old_owner.parent_item_uid,0),
       old_owner.item_revision,baseline.opening_item_revision,
       payload.value0,payload.value1,payload.value2,payload.value3
FROM player_items payload
LEFT JOIN item_current_owner new_owner ON new_owner.item_uid=payload.obj_uid
LEFT JOIN player_items parent_payload ON parent_payload.id=payload.container_id
LEFT JOIN item_current_owner parent_owner ON parent_owner.item_uid=parent_payload.obj_uid
JOIN item_current_owner old_owner
  ON old_owner.owner_type=1 AND old_owner.owner_id=payload.pid
 AND old_owner.owner_context_id=0 AND old_owner.state=1 AND old_owner.vnum=payload.vnum
JOIN item_ownership_baseline baseline
  ON baseline.item_uid=old_owner.item_uid
 AND baseline.owner_type=old_owner.owner_type
 AND baseline.owner_id=old_owner.owner_id
 AND baseline.owner_context_id=old_owner.owner_context_id
 AND baseline.vnum=old_owner.vnum
WHERE payload.vnum=3 AND (payload.item_type IS NULL OR payload.item_type=20)
  AND payload.obj_uid IS NOT NULL AND payload.obj_uid>0 AND new_owner.item_uid IS NULL
  AND NOT EXISTS (SELECT 1 FROM item_ownership_baseline new_baseline
                  WHERE new_baseline.item_uid=payload.obj_uid)
  AND NOT EXISTS (SELECT 1 FROM item_ownership_ledger new_ledger
                  WHERE new_ledger.item_uid=payload.obj_uid)
  AND old_owner.item_revision>=baseline.opening_item_revision
  AND NOT EXISTS (SELECT 1 FROM player_items existing
                  WHERE existing.obj_uid=old_owner.item_uid)
  AND NOT EXISTS (SELECT 1 FROM player_pet_items existing
                  WHERE existing.obj_uid=old_owner.item_uid)
  AND NOT EXISTS (SELECT 1 FROM locker_items existing
                  WHERE existing.obj_uid=old_owner.item_uid)
  AND NOT EXISTS (SELECT 1 FROM corpse_items existing
                  WHERE existing.obj_uid=old_owner.item_uid)
  AND NOT EXISTS (SELECT 1 FROM saved_items existing
                  WHERE existing.obj_uid=old_owner.item_uid)
  AND NOT EXISTS (SELECT 1 FROM account_locker_items existing
                  WHERE existing.obj_uid=old_owner.item_uid)
  AND NOT EXISTS (SELECT 1 FROM siege_items existing
                  WHERE existing.obj_uid=old_owner.item_uid)
  AND NOT EXISTS (SELECT 1 FROM shopkeeper_items existing
                  WHERE existing.obj_uid=old_owner.item_uid)
  AND NOT EXISTS (SELECT 1 FROM auction_item_custody existing
                  WHERE existing.item_uid=old_owner.item_uid)
  AND payload.container_id IS NOT NULL AND parent_payload.obj_uid IS NOT NULL
  AND old_owner.parent_item_uid=parent_payload.obj_uid
  AND parent_owner.root_item_uid=old_owner.root_item_uid
  AND parent_owner.owner_type=old_owner.owner_type
  AND parent_owner.owner_id=old_owner.owner_id
  AND parent_owner.owner_context_id=old_owner.owner_context_id
  AND parent_owner.state=1;
"

if [[ "$mode" == --classify ]]; then
    [[ ! -e "$artifact" ]] || {
        echo 'refusing to overwrite an existing pair artifact' >&2
        exit 2
    }
    umask 077
    temporary=$(mktemp "$artifact_parent/.coin-custody-pair.XXXXXX")
    trap 'rm -f "$temporary"' EXIT
    printf '# pid\tpayload_id\tnew_uid\told_uid\tvnum\tcontainer_id\tparent_uid\troot_uid\told_parent_uid\titem_revision\topening_revision\tvalue0\tvalue1\tvalue2\tvalue3\n' >"$temporary"
    if ! "${MYSQL[@]}" -e "$pair_query" >>"$temporary"; then
        echo 'coin custody pair classification query failed' >&2
        exit 2
    fi
    chmod 600 "$temporary"
    mv "$temporary" "$artifact"
    trap - EXIT
    pair_count=$(awk 'BEGIN { n=0 } !/^#/ { n++ } END { print n }' "$artifact")
    printf 'candidate_pairs=%s\nartifact_sha256=%s\n' "$pair_count" \
        "$(sha256sum "$artifact" | cut -d' ' -f1)"
    exit 0
fi

environment_name="${ENVIRONMENT:-${APP_ENV:-}}"
[[ "${environment_name,,}" =~ (dev|local|test) && "${DB_NAME,,}" =~ (dev|local|test) ]] || {
    echo 'refusing coin custody reconciliation outside development/local/test' >&2
    exit 1
}
[[ "${WRITERS_QUIESCED:-}" == TRUE && -n "${COIN_CUSTODY_BACKUP_ID:-}" ]] || {
    echo 'reconciliation requires WRITERS_QUIESCED=TRUE and COIN_CUSTODY_BACKUP_ID' >&2
    exit 1
}
[[ -f "$artifact" && ! -L "$artifact" ]] || {
    echo 'pair artifact must be a regular non-symlink file' >&2
    exit 2
}
artifact_mode=$(stat -c '%a' "$artifact")
artifact_uid=$(stat -c '%u' "$artifact")
(( (8#$artifact_mode & 077) == 0 )) && [[ "$artifact_uid" == "$(id -u)" ]] || {
    echo 'pair artifact must be owner-only' >&2
    exit 2
}
actual_sha=$(sha256sum "$artifact" | cut -d' ' -f1)
[[ "$actual_sha" == "$3" ]] || {
    echo 'pair artifact checksum does not match reviewed checksum' >&2
    exit 1
}

pair_count=0
while IFS=$'\t' read -r pid payload_id new_uid old_uid vnum container_id parent_uid \
    root_uid old_parent_uid item_revision opening_revision value0 value1 value2 value3 extra; do
    [[ "$pid" == '# pid' ]] && continue
    [[ -z "$pid" ]] && continue
    values=("$pid" "$payload_id" "$new_uid" "$old_uid" "$vnum" "$container_id" \
        "$parent_uid" "$root_uid" "$old_parent_uid" "$item_revision" \
        "$opening_revision")
    for value in "${values[@]}"; do
        [[ "$value" =~ ^[0-9]+$ ]] || {
            echo 'pair artifact contains malformed identity data' >&2
            exit 2
        }
    done
    for value in "$value0" "$value1" "$value2" "$value3"; do
        [[ "$value" =~ ^-?[0-9]+$ ]] || {
            echo 'pair artifact contains malformed money data' >&2
            exit 2
        }
    done
    [[ -z "${extra:-}" && "$vnum" == 3 && "$new_uid" != "$old_uid" &&
       "$container_id" != 0 && "$parent_uid" != 0 && "$old_parent_uid" == "$parent_uid" ]] || {
        echo 'pair artifact is not an exact physical-coin replacement pair' >&2
        exit 2
    }
    pair_count=$((pair_count + 1))
done <"$artifact"
[[ "$pair_count" == 1 ]] || {
    echo 'reconciliation requires exactly one reviewed owner/payload pair' >&2
    exit 1
}

apply_sql="CREATE TEMPORARY TABLE coin_custody_repair_guard(ok TINYINT CHECK(ok=1));
START TRANSACTION;
SELECT id FROM player_items WHERE id=$payload_id FOR UPDATE;
SELECT item_uid FROM item_current_owner WHERE item_uid IN ($old_uid,$new_uid) FOR UPDATE;
INSERT INTO coin_custody_repair_guard(ok)
SELECT COUNT(*)=1 FROM player_items payload
LEFT JOIN player_items parent_payload ON parent_payload.id=payload.container_id
WHERE payload.id=$payload_id AND payload.pid=$pid AND payload.obj_uid=$new_uid
  AND payload.vnum=$vnum AND (payload.item_type IS NULL OR payload.item_type=20)
  AND COALESCE(payload.container_id,0)=$container_id
  AND COALESCE(parent_payload.obj_uid,0)=$parent_uid
  AND payload.value0=$value0 AND payload.value1=$value1
  AND payload.value2=$value2 AND payload.value3=$value3;
INSERT INTO coin_custody_repair_guard(ok)
SELECT COUNT(*)=1 FROM item_current_owner owner
JOIN item_ownership_baseline baseline ON baseline.item_uid=owner.item_uid
JOIN item_current_owner parent ON parent.item_uid=$parent_uid
WHERE owner.item_uid=$old_uid AND owner.root_item_uid=$root_uid
  AND owner.parent_item_uid=$old_parent_uid
  AND owner.owner_type=1 AND owner.owner_id=$pid AND owner.owner_context_id=0
  AND owner.item_revision=$item_revision AND owner.vnum=$vnum AND owner.state=1
  AND parent.root_item_uid=owner.root_item_uid AND parent.owner_type=owner.owner_type
  AND parent.owner_id=owner.owner_id AND parent.owner_context_id=owner.owner_context_id
  AND parent.state=1
  AND baseline.opening_item_revision=$opening_revision
  AND baseline.owner_type=owner.owner_type AND baseline.owner_id=owner.owner_id
  AND baseline.owner_context_id=owner.owner_context_id AND baseline.vnum=owner.vnum;
INSERT INTO coin_custody_repair_guard(ok)
SELECT (SELECT COUNT(*) FROM item_current_owner WHERE item_uid=$new_uid)=0
   AND (SELECT COUNT(*) FROM item_ownership_baseline WHERE item_uid=$new_uid)=0
   AND (SELECT COUNT(*) FROM item_ownership_ledger WHERE item_uid=$new_uid)=0;
INSERT INTO coin_custody_repair_guard(ok)
SELECT (SELECT COUNT(*) FROM player_items WHERE obj_uid=$new_uid)=1
   AND (SELECT COUNT(*) FROM player_items WHERE obj_uid=$old_uid)=0
   AND (SELECT COUNT(*) FROM player_pet_items WHERE obj_uid IN ($old_uid,$new_uid))=0
   AND (SELECT COUNT(*) FROM locker_items WHERE obj_uid IN ($old_uid,$new_uid))=0
   AND (SELECT COUNT(*) FROM account_locker_items WHERE obj_uid IN ($old_uid,$new_uid))=0
   AND (SELECT COUNT(*) FROM corpse_items WHERE obj_uid IN ($old_uid,$new_uid))=0
   AND (SELECT COUNT(*) FROM saved_items WHERE obj_uid IN ($old_uid,$new_uid))=0
   AND (SELECT COUNT(*) FROM siege_items WHERE obj_uid IN ($old_uid,$new_uid))=0
   AND (SELECT COUNT(*) FROM shopkeeper_items WHERE obj_uid IN ($old_uid,$new_uid))=0
   AND (SELECT COUNT(*) FROM auction_item_custody
        WHERE item_uid IN ($old_uid,$new_uid))=0;
UPDATE player_items SET obj_uid=$old_uid
WHERE id=$payload_id AND pid=$pid AND obj_uid=$new_uid;
INSERT INTO coin_custody_repair_guard(ok) SELECT ROW_COUNT()=1;
INSERT INTO coin_custody_repair_guard(ok)
SELECT (SELECT COUNT(*) FROM player_items WHERE id=$payload_id AND obj_uid=$old_uid)=1
   AND (SELECT COUNT(*) FROM item_current_owner WHERE item_uid=$old_uid
        AND owner_type=1 AND owner_id=$pid AND state=1)=1
   AND (SELECT COUNT(*) FROM player_items WHERE obj_uid=$new_uid)=0
   AND (SELECT COUNT(*) FROM item_current_owner WHERE item_uid=$new_uid)=0;
COMMIT;"
if ! "${MYSQL[@]}" -e "$apply_sql" >/dev/null; then
    echo 'coin custody reconciliation rolled back because live state differed from review' >&2
    exit 1
fi

receipt="$artifact.applied"
umask 077
receipt_temporary=$(mktemp "$artifact_parent/.coin-custody-receipt.XXXXXX")
{
    printf 'artifact_sha256=%s\n' "$actual_sha"
    printf 'backup_id_sha256=%s\n' \
        "$(printf '%s' "$COIN_CUSTODY_BACKUP_ID" | sha256sum | cut -d' ' -f1)"
    printf 'reconciled_pairs=1\n'
} >"$receipt_temporary"
chmod 600 "$receipt_temporary"
mv "$receipt_temporary" "$receipt"
printf 'reconciled_pairs=1\nreceipt_sha256=%s\n' \
    "$(sha256sum "$receipt" | cut -d' ' -f1)"
