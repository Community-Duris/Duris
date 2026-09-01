#!/usr/bin/env bash
#
# Repair item_current_owner nesting that disagrees with the saved container
# linkage.
#
# Before the durable single-item put landed, `put <item> <container>` into a
# container the actor already owned moved the object live without submitting an
# ownership transfer.  The saved custody rows recorded the nesting, the ledger
# did not, and the ledger is what capture() checks before a give or drop and
# what player load rebuilds nesting from.  A container filled that way can
# therefore never be given or dropped, and spills its contents on the next
# login, until its rows are reconciled.
#
# Additive and re-runnable: it only rewrites parent_item_uid and root_item_uid,
# only where the ledger already agrees with the custody row about the owner, and
# it never touches ownership or item_revision (revisions are ledger-derived and
# reconcile_item_ownership.sh checks them against the event count).
#
# Usage: repair_item_nesting.sh [--check]
#        --check reports the drift count and exits non-zero if any remains.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
# shellcheck disable=SC1091
source "$PROJECT_ROOT/.env"
environment_name="${ENVIRONMENT:-${APP_ENV:-}}"
[[ "${environment_name,,}" =~ (dev|local|test) ]] || { echo 'refusing item nesting repair: environment is not development/local/test' >&2; exit 1; }
[[ "${DB_NAME,,}" =~ (dev|local|test) ]] || { echo 'refusing item nesting repair: database name is not development/local/test' >&2; exit 1; }

CHECK_ONLY=0
[[ "${1:-}" == "--check" ]] && CHECK_ONLY=1

export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" "$DB_NAME")

EVIDENCE_SQL=$(cat <<'SQL'
CREATE TEMPORARY TABLE item_nesting_evidence (
    item_uid BIGINT UNSIGNED NOT NULL PRIMARY KEY,
    parent_uid BIGINT UNSIGNED NULL,
    owner_type TINYINT UNSIGNED NOT NULL,
    owner_id BIGINT UNSIGNED NOT NULL,
    owner_context_id BIGINT UNSIGNED NOT NULL
) ENGINE=InnoDB;

INSERT IGNORE INTO item_nesting_evidence
SELECT i.obj_uid,p.obj_uid,1,i.pid,0
FROM player_items i LEFT JOIN player_items p ON p.id=i.container_id
WHERE i.obj_uid>0;
INSERT IGNORE INTO item_nesting_evidence
SELECT i.obj_uid,p.obj_uid,4,
       ((CAST(pd.pid AS UNSIGNED) << 32) | (CAST(c.save_id AS UNSIGNED) & 4294967295)),0
FROM corpse_items i JOIN corpses c ON c.id=i.corpse_id
JOIN player_data pd ON LOWER(pd.name)=LOWER(c.player_name)
LEFT JOIN corpse_items p ON p.id=i.container_id
WHERE i.obj_uid>0;
INSERT IGNORE INTO item_nesting_evidence
SELECT i.obj_uid,p.obj_uid,5,i.locker_id,COALESCE(i.chest_id,public_chest.id,0)
FROM locker_items i LEFT JOIN locker_items p ON p.id=i.container_id
LEFT JOIN private_chests public_chest
  ON public_chest.locker_id=i.locker_id AND public_chest.is_public=1
WHERE i.obj_uid>0;
INSERT IGNORE INTO item_nesting_evidence
SELECT i.obj_uid,p.obj_uid,5,i.chest_id,0
FROM account_locker_items i LEFT JOIN account_locker_items p ON p.id=i.container_id
WHERE i.obj_uid>0;
INSERT IGNORE INTO item_nesting_evidence
SELECT i.obj_uid,p.obj_uid,3,CAST(i.room_vnum AS UNSIGNED),0
FROM saved_items i LEFT JOIN saved_items p ON p.id=i.container_id
WHERE i.obj_uid>0;

-- Only rows whose owner already matches on both sides are repairable: a
-- disagreement about the owner is a custody conflict for the baseline
-- quarantine to resolve, not a nesting drift.
CREATE TEMPORARY TABLE item_nesting_drift (
    item_uid BIGINT UNSIGNED NOT NULL PRIMARY KEY,
    parent_uid BIGINT UNSIGNED NOT NULL
) ENGINE=InnoDB AS
SELECT evidence.item_uid,evidence.parent_uid
FROM item_nesting_evidence evidence
JOIN item_current_owner current_item ON current_item.item_uid=evidence.item_uid
JOIN item_current_owner parent_item ON parent_item.item_uid=evidence.parent_uid
WHERE evidence.parent_uid IS NOT NULL AND evidence.parent_uid>0
  AND current_item.state=1 AND parent_item.state=1
  AND COALESCE(current_item.parent_item_uid,0)<>evidence.parent_uid
  AND current_item.owner_type=evidence.owner_type
  AND current_item.owner_id=evidence.owner_id
  AND current_item.owner_context_id=evidence.owner_context_id
  AND parent_item.owner_type=current_item.owner_type
  AND parent_item.owner_id=current_item.owner_id
  AND parent_item.owner_context_id=current_item.owner_context_id;
SQL
)

if [[ "$CHECK_ONLY" == 1 ]]; then
  drift=$("${MYSQL[@]}" -N -B <<SQL
$EVIDENCE_SQL
SELECT COUNT(*) FROM item_nesting_drift;
SQL
)
  printf 'nesting_mismatch=%s\n' "$drift"
  [[ "$drift" == 0 ]]
  exit $?
fi

"${MYSQL[@]}" <<SQL
START TRANSACTION;
$EVIDENCE_SQL
SELECT COUNT(*) AS nesting_rows_to_repair FROM item_nesting_drift;

UPDATE item_current_owner current_item JOIN item_nesting_drift drift
  ON drift.item_uid=current_item.item_uid
SET current_item.parent_item_uid=drift.parent_uid;

-- A repaired item's descendants inherited its stale root, so recompute roots
-- across each repaired subtree rather than only for the rows just rewritten.
CREATE TEMPORARY TABLE item_nesting_affected (
    item_uid BIGINT UNSIGNED NOT NULL PRIMARY KEY
) ENGINE=InnoDB AS
WITH RECURSIVE subtree(item_uid,depth) AS (
  SELECT item_uid,0 FROM item_nesting_drift
  UNION DISTINCT
  SELECT child.item_uid,subtree.depth+1
  FROM subtree JOIN item_current_owner child ON child.parent_item_uid=subtree.item_uid
  WHERE subtree.depth<32
)
SELECT DISTINCT item_uid FROM subtree;

CREATE TEMPORARY TABLE item_nesting_roots (
    item_uid BIGINT UNSIGNED NOT NULL PRIMARY KEY,
    root_uid BIGINT UNSIGNED NOT NULL
) ENGINE=InnoDB AS
WITH RECURSIVE ancestry(start_uid,current_uid,parent_uid,depth) AS (
  SELECT current_item.item_uid,current_item.item_uid,current_item.parent_item_uid,0
  FROM item_current_owner current_item
  JOIN item_nesting_affected affected ON affected.item_uid=current_item.item_uid
  UNION ALL
  SELECT ancestry.start_uid,parent_item.item_uid,parent_item.parent_item_uid,ancestry.depth+1
  FROM ancestry JOIN item_current_owner parent_item ON parent_item.item_uid=ancestry.parent_uid
  WHERE ancestry.depth<32
)
SELECT start_uid AS item_uid,current_uid AS root_uid FROM ancestry WHERE parent_uid IS NULL;

UPDATE item_current_owner current_item JOIN item_nesting_roots resolved
  ON resolved.item_uid=current_item.item_uid
SET current_item.root_item_uid=resolved.root_uid
WHERE current_item.root_item_uid<>resolved.root_uid;
COMMIT;
SQL

"$SCRIPT_DIR/repair_item_nesting.sh" --check
