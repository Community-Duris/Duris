#!/usr/bin/env bash
set -euo pipefail

# Print the supported protected classification and apply forms.
usage() {
    echo 'usage: repair_missing_combat_baselines.sh --classify /absolute/private/artifact.tsv' >&2
    echo '   or: repair_missing_combat_baselines.sh --apply /absolute/private/artifact.tsv SHA256' >&2
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
if [[ "$DB_HOST" != "localhost" && "$DB_HOST" != "127.0.0.1" && "$DB_HOST" != "::1" ]]; then
    [[ "${DB_TLS:-}" == "TRUE" && -f "${DB_SSL_CA:-}" ]] || {
        echo 'remote combat baseline repair requires TLS and a CA file' >&2
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

artifact_parent="$(dirname "$artifact")"
[[ -d "$artifact_parent" && ! -L "$artifact_parent" ]] || {
    echo 'classification artifact parent must be an existing directory' >&2
    exit 2
}
parent_mode=$(stat -c '%a' "$artifact_parent")
parent_uid=$(stat -c '%u' "$artifact_parent")
[[ "$parent_mode" =~ ^[0-7]+$ && "$parent_uid" == "$(id -u)" ]] || {
    echo 'classification artifact parent has invalid ownership or mode' >&2
    exit 2
}
(( (8#$parent_mode & 077) == 0 )) || {
    echo 'classification artifact parent must not grant group or other access' >&2
    exit 2
}

classification_query="
SELECT eligible.pid,eligible.frags,eligible.frag_revision,
       COUNT(ledger.pid),COALESCE(SUM(ledger.delta),0),
       CASE
         WHEN COUNT(ledger.pid)=0 AND eligible.frag_revision=0 THEN 'safe_no_history'
         WHEN COUNT(ledger.pid)>0 THEN 'ledger_history_requires_review'
         ELSE 'revision_without_ledger'
       END,
       eligible.frags-COALESCE(SUM(ledger.delta),0),
       CASE WHEN eligible.frag_revision>=COUNT(ledger.pid)
            THEN eligible.frag_revision-COUNT(ledger.pid) ELSE -1 END
FROM (
    SELECT DISTINCT player.pid,player.frags,player.frag_revision
    FROM player_data player
    JOIN account_characters mapping ON mapping.pid=player.pid
    WHERE player.active=1 AND mapping.deleted_at IS NULL AND mapping.blocked=0
) eligible
LEFT JOIN combat_frag_baseline baseline ON baseline.pid=eligible.pid
LEFT JOIN combat_frag_ledger ledger ON ledger.pid=eligible.pid
WHERE baseline.pid IS NULL
GROUP BY eligible.pid,eligible.frags,eligible.frag_revision
ORDER BY eligible.pid;"

if [[ "$mode" == --classify ]]; then
    [[ ! -e "$artifact" ]] || {
        echo 'refusing to overwrite an existing classification artifact' >&2
        exit 2
    }
    umask 077
    temporary=$(mktemp "$artifact_parent/.combat-baseline-classification.XXXXXX")
    trap 'rm -f "$temporary"' EXIT
    printf '# pid\tcurrent_frags\tcurrent_revision\tledger_rows\tledger_delta\tclass\tproposed_opening_frags\tproposed_opening_revision\n' >"$temporary"
    if ! "${MYSQL[@]}" -e "$classification_query" >>"$temporary"; then
        echo 'combat baseline classification query failed' >&2
        exit 2
    fi
    chmod 600 "$temporary"
    mv "$temporary" "$artifact"
    trap - EXIT
    total=$(awk 'BEGIN { n=0 } !/^#/ { n++ } END { print n }' "$artifact")
    safe=$(awk '$6 == "safe_no_history" { n++ } END { print n+0 }' "$artifact")
    history=$(awk '$6 == "ledger_history_requires_review" { n++ } END { print n+0 }' "$artifact")
    revision=$(awk '$6 == "revision_without_ledger" { n++ } END { print n+0 }' "$artifact")
    printf 'missing_combat_baselines=%s\nsafe_no_history=%s\n' "$total" "$safe"
    printf 'ledger_history_requires_review=%s\nrevision_without_ledger=%s\n' \
        "$history" "$revision"
    printf 'artifact_sha256=%s\n' "$(sha256sum "$artifact" | cut -d' ' -f1)"
    exit 0
fi

environment_name="${ENVIRONMENT:-${APP_ENV:-}}"
[[ "${environment_name,,}" =~ (dev|local|test) && "${DB_NAME,,}" =~ (dev|local|test) ]] || {
    echo 'refusing combat baseline repair outside development/local/test' >&2
    exit 1
}
[[ "${WRITERS_QUIESCED:-}" == TRUE && -n "${COMBAT_BASELINE_BACKUP_ID:-}" ]] || {
    echo 'repair requires WRITERS_QUIESCED=TRUE and COMBAT_BASELINE_BACKUP_ID' >&2
    exit 1
}
rollback_evidence="${COMBAT_BASELINE_ROLLBACK_EVIDENCE:-}"
rollback_reviewed_sha="${COMBAT_BASELINE_ROLLBACK_SHA256:-}"
[[ "$rollback_evidence" == /* && "$rollback_reviewed_sha" =~ ^[0-9a-f]{64}$ &&
   "$(dirname "$rollback_evidence")" == "$artifact_parent" &&
   -f "$rollback_evidence" && ! -L "$rollback_evidence" ]] || {
    echo 'repair requires a protected rollback-evidence file and reviewed checksum' >&2
    exit 1
}
rollback_mode=$(stat -c '%a' "$rollback_evidence")
rollback_uid=$(stat -c '%u' "$rollback_evidence")
(( (8#$rollback_mode & 077) == 0 )) && [[ "$rollback_uid" == "$(id -u)" ]] || {
    echo 'rollback evidence must be owner-only' >&2
    exit 2
}
rollback_sha=$(sha256sum "$rollback_evidence" | cut -d' ' -f1)
[[ "$rollback_sha" == "$rollback_reviewed_sha" ]] || {
    echo 'rollback evidence checksum does not match reviewed checksum' >&2
    exit 1
}
[[ -f "$artifact" && ! -L "$artifact" ]] || {
    echo 'classification artifact must be a regular non-symlink file' >&2
    exit 2
}
artifact_mode=$(stat -c '%a' "$artifact")
artifact_uid=$(stat -c '%u' "$artifact")
(( (8#$artifact_mode & 077) == 0 )) && [[ "$artifact_uid" == "$(id -u)" ]] || {
    echo 'classification artifact must be owner-only' >&2
    exit 2
}
actual_sha=$(sha256sum "$artifact" | cut -d' ' -f1)
[[ "$actual_sha" == "$3" ]] || {
    echo 'classification artifact checksum does not match reviewed checksum' >&2
    exit 1
}

approved_sql=""
approved_count=0
while IFS=$'\t' read -r pid current_frags current_revision ledger_rows ledger_delta \
    category opening_frags opening_revision extra; do
    [[ "$pid" == '# pid' ]] && continue
    [[ -z "$pid" ]] && continue
    [[ -z "${extra:-}" && "$pid" =~ ^[0-9]+$ && "$current_frags" =~ ^-?[0-9]+$ &&
       "$current_revision" =~ ^[0-9]+$ && "$ledger_rows" =~ ^[0-9]+$ &&
       "$ledger_delta" =~ ^-?[0-9]+$ && "$opening_frags" =~ ^-?[0-9]+$ &&
       "$opening_revision" =~ ^[0-9]+$ ]] || {
        echo 'classification artifact contains malformed data' >&2
        exit 2
    }
    [[ "$category" == safe_no_history && "$current_revision" == 0 &&
       "$ledger_rows" == 0 && "$ledger_delta" == 0 &&
       "$opening_frags" == "$current_frags" && "$opening_revision" == 0 ]] || {
        echo 'classification contains history or ambiguity requiring separate reviewed DML' >&2
        exit 1
    }
    row="SELECT $pid pid,$current_frags current_frags,$current_revision current_revision,$opening_frags opening_frags,$opening_revision opening_revision"
    if [[ -n "$approved_sql" ]]; then
        approved_sql+=" UNION ALL "
    fi
    approved_sql+="$row"
    approved_count=$((approved_count + 1))
done <"$artifact"

reconciliation_predicate="
NOT EXISTS (
  SELECT 1 FROM player_data player
  JOIN account_characters mapping ON mapping.pid=player.pid
  LEFT JOIN currency_wallet_baseline wallet ON wallet.pid=player.pid
  LEFT JOIN epic_balance_baseline epic ON epic.pid=player.pid
  LEFT JOIN combat_frag_baseline combat ON combat.pid=player.pid
  WHERE player.active=1 AND mapping.deleted_at IS NULL AND mapping.blocked=0
    AND (wallet.pid IS NULL OR epic.pid IS NULL OR combat.pid IS NULL))
AND NOT EXISTS (
  SELECT 1 FROM player_data player
  JOIN combat_frag_baseline baseline ON baseline.pid=player.pid
  LEFT JOIN (SELECT pid,SUM(delta) delta FROM combat_frag_ledger GROUP BY pid) ledger
    ON ledger.pid=player.pid
  WHERE baseline.opening_frags+COALESCE(ledger.delta,0)<>player.frags)
AND NOT EXISTS (
  SELECT 1 FROM combat_frag_ledger ledger JOIN player_data player ON player.pid=ledger.pid
  WHERE ledger.frag_revision=player.frag_revision AND ledger.frags_after<>player.frags)
AND NOT EXISTS (
  SELECT 1 FROM player_data player LEFT JOIN currency_wallet_baseline baseline
    ON baseline.pid=player.pid WHERE baseline.pid IS NULL)
AND NOT EXISTS (
  SELECT 1 FROM account_banks bank LEFT JOIN currency_bank_baseline baseline
    ON baseline.bank_id=bank.id WHERE baseline.bank_id IS NULL)
AND NOT EXISTS (
  SELECT 1 FROM player_data player
  JOIN currency_wallet_baseline baseline ON baseline.pid=player.pid
  LEFT JOIN (SELECT pid,SUM(wallet_delta_copper) copper,
                    SUM(wallet_delta_silver) silver,SUM(wallet_delta_gold) gold,
                    SUM(wallet_delta_platinum) platinum
             FROM currency_ledger GROUP BY pid) ledger ON ledger.pid=player.pid
  WHERE baseline.opening_copper+COALESCE(ledger.copper,0)<>player.copper
     OR baseline.opening_silver+COALESCE(ledger.silver,0)<>player.silver
     OR baseline.opening_gold+COALESCE(ledger.gold,0)<>player.gold
     OR baseline.opening_platinum+COALESCE(ledger.platinum,0)<>player.platinum)
AND NOT EXISTS (
  SELECT 1 FROM account_banks bank
  JOIN currency_bank_baseline baseline ON baseline.bank_id=bank.id
  LEFT JOIN (SELECT bank_id,SUM(bank_delta_copper) copper,
                    SUM(bank_delta_silver) silver,SUM(bank_delta_gold) gold,
                    SUM(bank_delta_platinum) platinum
             FROM currency_ledger GROUP BY bank_id) ledger ON ledger.bank_id=bank.id
  WHERE baseline.opening_copper+COALESCE(ledger.copper,0)<>bank.bank_copper
     OR baseline.opening_silver+COALESCE(ledger.silver,0)<>bank.bank_silver
     OR baseline.opening_gold+COALESCE(ledger.gold,0)<>bank.bank_gold
     OR baseline.opening_platinum+COALESCE(ledger.platinum,0)<>bank.bank_platinum)
AND NOT EXISTS (
  SELECT 1 FROM player_data player LEFT JOIN epic_balance_baseline baseline
    ON baseline.pid=player.pid WHERE baseline.pid IS NULL)
AND NOT EXISTS (
  SELECT 1 FROM player_data player JOIN epic_balance_baseline baseline
    ON baseline.pid=player.pid
  LEFT JOIN (SELECT pid,SUM(delta) delta FROM epic_ledger GROUP BY pid) ledger
    ON ledger.pid=player.pid
  WHERE baseline.opening_balance+COALESCE(ledger.delta,0)<>player.epics)
AND NOT EXISTS (
  SELECT 1 FROM epic_ledger ledger JOIN player_data player ON player.pid=ledger.pid
  WHERE ledger.epic_revision=player.epic_revision AND ledger.balance_after<>player.epics)
AND (SELECT COUNT(*) FROM information_schema.referential_constraints
     WHERE constraint_schema=DATABASE() AND update_rule='RESTRICT' AND delete_rule='RESTRICT'
       AND constraint_name IN ('combat_outcome_operation_fk','combat_participant_operation_fk',
         'combat_frag_operation_fk','currency_ledger_operation_fk','epic_ledger_operation_fk',
         'item_current_parent_fk','item_ownership_operation_fk'))=7"

if (( approved_count > 0 )); then
    apply_sql="CREATE TEMPORARY TABLE combat_baseline_apply_guard(ok TINYINT CHECK(ok=1));
START TRANSACTION;
INSERT INTO combat_frag_baseline(pid,opening_frags,opening_revision)
SELECT approved.pid,approved.opening_frags,approved.opening_revision
FROM ($approved_sql) approved
JOIN player_data player ON player.pid=approved.pid
LEFT JOIN combat_frag_baseline baseline ON baseline.pid=approved.pid
WHERE baseline.pid IS NULL AND player.frags=approved.current_frags
  AND player.frag_revision=approved.current_revision
  AND NOT EXISTS (SELECT 1 FROM combat_frag_ledger ledger WHERE ledger.pid=approved.pid);
INSERT INTO combat_baseline_apply_guard(ok)
SELECT COUNT(*)=$approved_count AND $reconciliation_predicate
FROM combat_frag_baseline baseline
JOIN ($approved_sql) approved ON approved.pid=baseline.pid
WHERE baseline.opening_frags=approved.opening_frags
  AND baseline.opening_revision=approved.opening_revision;
COMMIT;"
    if ! "${MYSQL[@]}" -e "$apply_sql" >/dev/null; then
        echo 'combat baseline repair rolled back because locked state did not match review' >&2
        exit 1
    fi
fi

readiness_output=$("$SCRIPT_DIR/check_character_baseline_readiness.sh") || {
    printf '%s\n' "$readiness_output"
    echo 'baseline repair applied, but full character readiness still fails' >&2
    exit 1
}
reconciliation_query="SELECT
(SELECT COUNT(*) FROM player_data player
 JOIN combat_frag_baseline baseline ON baseline.pid=player.pid
 LEFT JOIN (SELECT pid,SUM(delta) delta FROM combat_frag_ledger GROUP BY pid) ledger
   ON ledger.pid=player.pid
 WHERE baseline.opening_frags+COALESCE(ledger.delta,0)<>player.frags),
(SELECT COUNT(*) FROM combat_frag_ledger ledger JOIN player_data player ON player.pid=ledger.pid
 WHERE ledger.frag_revision=player.frag_revision AND ledger.frags_after<>player.frags),
(SELECT COUNT(*) FROM player_data player LEFT JOIN currency_wallet_baseline baseline
   ON baseline.pid=player.pid WHERE baseline.pid IS NULL),
(SELECT COUNT(*) FROM account_banks bank LEFT JOIN currency_bank_baseline baseline
   ON baseline.bank_id=bank.id WHERE baseline.bank_id IS NULL),
(SELECT COUNT(*) FROM player_data player JOIN currency_wallet_baseline baseline
   ON baseline.pid=player.pid
 LEFT JOIN (SELECT pid,SUM(wallet_delta_copper) copper,SUM(wallet_delta_silver) silver,
                   SUM(wallet_delta_gold) gold,SUM(wallet_delta_platinum) platinum
            FROM currency_ledger GROUP BY pid) ledger ON ledger.pid=player.pid
 WHERE baseline.opening_copper+COALESCE(ledger.copper,0)<>player.copper
    OR baseline.opening_silver+COALESCE(ledger.silver,0)<>player.silver
    OR baseline.opening_gold+COALESCE(ledger.gold,0)<>player.gold
    OR baseline.opening_platinum+COALESCE(ledger.platinum,0)<>player.platinum),
(SELECT COUNT(*) FROM account_banks bank JOIN currency_bank_baseline baseline
   ON baseline.bank_id=bank.id
 LEFT JOIN (SELECT bank_id,SUM(bank_delta_copper) copper,SUM(bank_delta_silver) silver,
                   SUM(bank_delta_gold) gold,SUM(bank_delta_platinum) platinum
            FROM currency_ledger GROUP BY bank_id) ledger ON ledger.bank_id=bank.id
 WHERE baseline.opening_copper+COALESCE(ledger.copper,0)<>bank.bank_copper
    OR baseline.opening_silver+COALESCE(ledger.silver,0)<>bank.bank_silver
    OR baseline.opening_gold+COALESCE(ledger.gold,0)<>bank.bank_gold
    OR baseline.opening_platinum+COALESCE(ledger.platinum,0)<>bank.bank_platinum),
(SELECT COUNT(*) FROM player_data player LEFT JOIN epic_balance_baseline baseline
   ON baseline.pid=player.pid WHERE baseline.pid IS NULL),
(SELECT COUNT(*) FROM player_data player JOIN epic_balance_baseline baseline
   ON baseline.pid=player.pid
 LEFT JOIN (SELECT pid,SUM(delta) delta FROM epic_ledger GROUP BY pid) ledger
   ON ledger.pid=player.pid
 WHERE baseline.opening_balance+COALESCE(ledger.delta,0)<>player.epics),
(SELECT COUNT(*) FROM epic_ledger ledger JOIN player_data player ON player.pid=ledger.pid
 WHERE ledger.epic_revision=player.epic_revision AND ledger.balance_after<>player.epics),
(SELECT COUNT(*)<>7 FROM information_schema.referential_constraints
 WHERE constraint_schema=DATABASE() AND update_rule='RESTRICT' AND delete_rule='RESTRICT'
   AND constraint_name IN ('combat_outcome_operation_fk','combat_participant_operation_fk',
     'combat_frag_operation_fk','currency_ledger_operation_fk','epic_ledger_operation_fk',
     'item_current_parent_fk','item_ownership_operation_fk'));"
if ! reconciliation_output=$("${MYSQL[@]}" -e "$reconciliation_query"); then
    echo 'full combat, currency, epic, item, and FK reconciliation query failed' >&2
    exit 1
fi
IFS=$'\t' read -r combat_mismatch combat_latest wallet_missing bank_missing \
    wallet_mismatch bank_mismatch epic_missing epic_mismatch epic_latest fk_invalid \
    <<<"$reconciliation_output"
for value in "$combat_mismatch" "$combat_latest" "$wallet_missing" "$bank_missing" \
    "$wallet_mismatch" "$bank_mismatch" "$epic_missing" "$epic_mismatch" \
    "$epic_latest" "$fk_invalid"; do
    [[ "$value" =~ ^[0-9]+$ ]] || {
        echo 'full baseline reconciliation returned malformed aggregate output' >&2
        exit 1
    }
    [[ "$value" == 0 ]] || {
        echo 'full baseline reconciliation did not pass' >&2
        exit 1
    }
done
receipt="$artifact.applied"
umask 077
receipt_temporary=$(mktemp "$artifact_parent/.combat-baseline-receipt.XXXXXX")
{
    printf 'artifact_sha256=%s\n' "$actual_sha"
    printf 'backup_id_sha256=%s\n' \
        "$(printf '%s' "$COMBAT_BASELINE_BACKUP_ID" | sha256sum | cut -d' ' -f1)"
    printf 'rollback_evidence_sha256=%s\n' "$rollback_sha"
    printf 'approved_rows=%s\n' "$approved_count"
    printf 'combat_mismatch=%s\ncombat_latest_mismatch=%s\n' \
        "$combat_mismatch" "$combat_latest"
    printf 'wallet_missing=%s\nbank_missing=%s\nwallet_mismatch=%s\nbank_mismatch=%s\n' \
        "$wallet_missing" "$bank_missing" "$wallet_mismatch" "$bank_mismatch"
    printf 'epic_missing=%s\nepic_mismatch=%s\nepic_latest_mismatch=%s\nfk_invalid=%s\n' \
        "$epic_missing" "$epic_mismatch" "$epic_latest" "$fk_invalid"
    printf '%s\n' "$readiness_output"
} >"$receipt_temporary"
chmod 600 "$receipt_temporary"
mv "$receipt_temporary" "$receipt"
printf '%s\n' "$readiness_output"
printf 'approved_rows=%s\nreceipt_sha256=%s\n' "$approved_count" \
    "$(sha256sum "$receipt" | cut -d' ' -f1)"
