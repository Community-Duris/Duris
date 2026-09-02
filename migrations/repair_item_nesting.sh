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
MYSQL_SSL=()
if [[ "$DB_HOST" != "localhost" && "$DB_HOST" != "127.0.0.1" && "$DB_HOST" != "::1" ]]; then
  [[ "${DB_TLS:-}" == "TRUE" && -f "${DB_SSL_CA:-}" ]] || {
    echo 'remote item nesting repair requires TLS and a CA file' >&2
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
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" "$DB_NAME")

EVIDENCE_SQL=$(cat <<'SQL'
CREATE TEMPORARY TABLE item_nesting_candidates (
    source_table VARCHAR(32) NOT NULL,
    source_row_id BIGINT UNSIGNED NOT NULL,
    item_uid BIGINT UNSIGNED NOT NULL,
    parent_uid BIGINT UNSIGNED NULL,
    owner_type TINYINT UNSIGNED NOT NULL,
    owner_id BIGINT UNSIGNED NOT NULL,
    owner_context_id BIGINT UNSIGNED NOT NULL,
    broken_parent TINYINT UNSIGNED NOT NULL,
    PRIMARY KEY(source_table,source_row_id),
    KEY idx_nesting_candidate_uid(item_uid),
    KEY idx_nesting_candidate_parent(parent_uid)
) ENGINE=InnoDB;

INSERT INTO item_nesting_candidates
SELECT 'player_items',i.id,i.obj_uid,p.obj_uid,1,i.pid,0,
       IF(i.container_id IS NOT NULL AND p.obj_uid IS NULL,1,0)
FROM player_items i LEFT JOIN player_items p ON p.id=i.container_id
WHERE i.obj_uid>0;
INSERT INTO item_nesting_candidates
SELECT 'corpse_items',i.id,i.obj_uid,p.obj_uid,4,
       ((CAST(pd.pid AS UNSIGNED) << 32) | (CAST(c.save_id AS UNSIGNED) & 4294967295)),0,
       IF(i.container_id IS NOT NULL AND p.obj_uid IS NULL,1,0)
FROM corpse_items i JOIN corpses c ON c.id=i.corpse_id
JOIN player_data pd ON LOWER(pd.name)=LOWER(c.player_name)
LEFT JOIN corpse_items p ON p.id=i.container_id
WHERE i.obj_uid>0;
INSERT INTO item_nesting_candidates
SELECT 'locker_items',i.id,i.obj_uid,p.obj_uid,5,i.locker_id,
       COALESCE(i.chest_id,public_chest.id,0),
       IF(i.container_id IS NOT NULL AND p.obj_uid IS NULL,1,0)
FROM locker_items i LEFT JOIN locker_items p ON p.id=i.container_id
LEFT JOIN private_chests public_chest
  ON public_chest.locker_id=i.locker_id AND public_chest.is_public=1
WHERE i.obj_uid>0;
INSERT INTO item_nesting_candidates
SELECT 'account_locker_items',i.id,i.obj_uid,p.obj_uid,5,i.chest_id,0,
       IF(i.container_id IS NOT NULL AND p.obj_uid IS NULL,1,0)
FROM account_locker_items i LEFT JOIN account_locker_items p ON p.id=i.container_id
WHERE i.obj_uid>0;
INSERT INTO item_nesting_candidates
SELECT 'saved_items',i.id,i.obj_uid,p.obj_uid,3,CAST(i.room_vnum AS UNSIGNED),0,
       IF(i.container_id IS NOT NULL AND p.obj_uid IS NULL,1,0)
FROM saved_items i LEFT JOIN saved_items p ON p.id=i.container_id
WHERE i.obj_uid>0;

-- Preserve every candidate until ambiguity and quarantine checks have run.
-- INSERT IGNORE here would silently select whichever duplicate UID appeared first.
CREATE TEMPORARY TABLE item_nesting_evidence (
    item_uid BIGINT UNSIGNED NOT NULL PRIMARY KEY,
    parent_uid BIGINT UNSIGNED NULL,
    owner_type TINYINT UNSIGNED NOT NULL,
    owner_id BIGINT UNSIGNED NOT NULL,
    owner_context_id BIGINT UNSIGNED NOT NULL
) ENGINE=InnoDB AS
SELECT candidate.item_uid,MIN(candidate.parent_uid) AS parent_uid,
       MIN(candidate.owner_type) AS owner_type,MIN(candidate.owner_id) AS owner_id,
       MIN(candidate.owner_context_id) AS owner_context_id
FROM item_nesting_candidates candidate
LEFT JOIN (
  SELECT DISTINCT item_uid FROM item_ownership_quarantine
  WHERE repaired_at IS NULL AND item_uid>0
) quarantined_item ON quarantined_item.item_uid=candidate.item_uid
LEFT JOIN (
  SELECT DISTINCT item_uid FROM item_ownership_quarantine
  WHERE repaired_at IS NULL AND item_uid>0
) quarantined_parent ON quarantined_parent.item_uid=candidate.parent_uid
WHERE candidate.broken_parent=0
  AND quarantined_item.item_uid IS NULL
  AND quarantined_parent.item_uid IS NULL
GROUP BY candidate.item_uid
HAVING COUNT(DISTINCT COALESCE(candidate.parent_uid,0))=1
   AND COUNT(DISTINCT CONCAT_WS(':',candidate.owner_type,candidate.owner_id,
                                candidate.owner_context_id))=1;

-- Resolve the root represented by saved custody.  A row without exactly one
-- reachable root is unsafe evidence: it is cyclic, over-depth, or names a
-- parent excluded above.
CREATE TEMPORARY TABLE item_nesting_evidence_walk ENGINE=InnoDB AS
WITH RECURSIVE ancestry(start_uid,current_uid,parent_uid,depth,path,cycle_detected) AS (
  SELECT item_uid,item_uid,parent_uid,0,
         CAST(CONCAT(',',item_uid,',') AS CHAR(1536)),CAST(0 AS UNSIGNED)
  FROM item_nesting_evidence
  UNION ALL
  SELECT ancestry.start_uid,parent.item_uid,parent.parent_uid,ancestry.depth+1,
         CONCAT(ancestry.path,parent.item_uid,','),
         CAST(LOCATE(CONCAT(',',parent.item_uid,','),ancestry.path)>0 AS UNSIGNED)
  FROM ancestry JOIN item_nesting_evidence parent ON parent.item_uid=ancestry.parent_uid
  WHERE ancestry.cycle_detected=0 AND ancestry.depth<32
)
SELECT * FROM ancestry;

CREATE TEMPORARY TABLE item_nesting_evidence_roots (
    item_uid BIGINT UNSIGNED NOT NULL PRIMARY KEY,
    root_uid BIGINT UNSIGNED NOT NULL
) ENGINE=InnoDB AS
SELECT start_uid AS item_uid,
       MAX(CASE WHEN parent_uid IS NULL THEN current_uid ELSE 0 END) AS root_uid
FROM item_nesting_evidence_walk
GROUP BY start_uid
HAVING SUM(CASE WHEN parent_uid IS NULL THEN 1 ELSE 0 END)=1
   AND MAX(cycle_detected)=0;

CREATE TEMPORARY TABLE item_nesting_evidence_invalid (
    item_uid BIGINT UNSIGNED NOT NULL PRIMARY KEY
) ENGINE=InnoDB AS
SELECT evidence.item_uid
FROM item_nesting_evidence evidence
LEFT JOIN item_nesting_evidence_roots roots ON roots.item_uid=evidence.item_uid
WHERE roots.item_uid IS NULL;

-- Only rows whose owner already matches on both sides are repairable: a
-- disagreement about the owner is a custody conflict for the baseline
-- quarantine to resolve, not a nesting drift.
CREATE TEMPORARY TABLE item_nesting_drift (
    item_uid BIGINT UNSIGNED NOT NULL PRIMARY KEY,
    parent_uid BIGINT UNSIGNED NULL,
    root_uid BIGINT UNSIGNED NOT NULL
) ENGINE=InnoDB AS
SELECT evidence.item_uid,evidence.parent_uid,roots.root_uid
FROM item_nesting_evidence evidence
JOIN item_nesting_evidence_roots roots ON roots.item_uid=evidence.item_uid
JOIN item_current_owner current_item ON current_item.item_uid=evidence.item_uid
LEFT JOIN item_current_owner parent_item ON parent_item.item_uid=evidence.parent_uid
WHERE current_item.state=1
  AND current_item.owner_type=evidence.owner_type
  AND current_item.owner_id=evidence.owner_id
  AND current_item.owner_context_id=evidence.owner_context_id
  AND (evidence.parent_uid IS NULL OR
       (parent_item.state=1
        AND parent_item.owner_type=current_item.owner_type
        AND parent_item.owner_id=current_item.owner_id
        AND parent_item.owner_context_id=current_item.owner_context_id))
  AND (COALESCE(current_item.parent_item_uid,0)<>COALESCE(evidence.parent_uid,0)
       OR current_item.root_item_uid<>roots.root_uid);
SQL
)

if [[ "$CHECK_ONLY" == 1 ]]; then
  check_output=$("${MYSQL[@]}" -N -B <<SQL
$EVIDENCE_SQL
SELECT COUNT(*) FROM item_nesting_evidence_invalid;
SELECT COUNT(*) FROM item_nesting_drift;
SQL
)
  mapfile -t check_result <<< "$check_output"
  if [[ "${#check_result[@]}" != 2 || ! "${check_result[0]}" =~ ^[0-9]+$ ||
        ! "${check_result[1]}" =~ ^[0-9]+$ ]]; then
    echo 'item nesting check returned malformed database output' >&2
    exit 1
  fi
  if [[ "${check_result[0]}" != 0 ]]; then
    echo 'item nesting evidence does not resolve to exactly one acyclic root' >&2
    exit 1
  fi
  drift="${check_result[1]}"
  printf 'nesting_mismatch=%s\n' "$drift"
  [[ "$drift" == 0 ]]
  exit $?
fi

"${MYSQL[@]}" <<SQL
START TRANSACTION;
$EVIDENCE_SQL
SELECT COUNT(*) AS nesting_rows_to_repair FROM item_nesting_drift;

-- Validate the complete graph that would exist after applying the proposed
-- parents.  This catches a proposed parent that is a descendant, a missing or
-- inactive ancestor, and a chain that cannot reach one root.
CREATE TEMPORARY TABLE item_nesting_proposed (
    item_uid BIGINT UNSIGNED NOT NULL PRIMARY KEY,
    parent_uid BIGINT UNSIGNED NULL
) ENGINE=InnoDB AS
SELECT current_item.item_uid,
       CASE WHEN drift.item_uid IS NOT NULL THEN drift.parent_uid
            ELSE current_item.parent_item_uid END AS parent_uid
FROM item_current_owner current_item
LEFT JOIN item_nesting_drift drift ON drift.item_uid=current_item.item_uid
WHERE current_item.state=1;

CREATE TEMPORARY TABLE item_nesting_affected (
    item_uid BIGINT UNSIGNED NOT NULL PRIMARY KEY
) ENGINE=InnoDB AS
WITH RECURSIVE subtree(item_uid) AS (
  SELECT item_uid FROM item_nesting_drift
  UNION DISTINCT
  SELECT child.item_uid
  FROM subtree JOIN item_nesting_proposed child ON child.parent_uid=subtree.item_uid
)
SELECT item_uid FROM subtree;

CREATE TEMPORARY TABLE item_nesting_proposed_walk ENGINE=InnoDB AS
WITH RECURSIVE ancestry(start_uid,current_uid,parent_uid,depth,path,cycle_detected) AS (
  SELECT affected.item_uid,proposed.item_uid,proposed.parent_uid,0,
         CAST(CONCAT(',',proposed.item_uid,',') AS CHAR(1536)),CAST(0 AS UNSIGNED)
  FROM item_nesting_affected affected
  JOIN item_nesting_proposed proposed ON proposed.item_uid=affected.item_uid
  UNION ALL
  SELECT ancestry.start_uid,parent.item_uid,parent.parent_uid,ancestry.depth+1,
         CONCAT(ancestry.path,parent.item_uid,','),
         CAST(LOCATE(CONCAT(',',parent.item_uid,','),ancestry.path)>0 AS UNSIGNED)
  FROM ancestry JOIN item_nesting_proposed parent ON parent.item_uid=ancestry.parent_uid
  WHERE ancestry.cycle_detected=0 AND ancestry.depth<32
)
SELECT * FROM ancestry;

CREATE TEMPORARY TABLE item_nesting_proposed_roots (
    item_uid BIGINT UNSIGNED NOT NULL PRIMARY KEY,
    root_uid BIGINT UNSIGNED NOT NULL
) ENGINE=InnoDB AS
SELECT start_uid AS item_uid,
       MAX(CASE WHEN parent_uid IS NULL THEN current_uid ELSE 0 END) AS root_uid
FROM item_nesting_proposed_walk
GROUP BY start_uid
HAVING SUM(CASE WHEN parent_uid IS NULL THEN 1 ELSE 0 END)=1
   AND MAX(cycle_detected)=0;

CREATE TEMPORARY TABLE item_nesting_proposed_invalid (
    item_uid BIGINT UNSIGNED NOT NULL PRIMARY KEY
) ENGINE=InnoDB AS
SELECT affected.item_uid
FROM item_nesting_affected affected
LEFT JOIN item_nesting_proposed_roots roots ON roots.item_uid=affected.item_uid
WHERE roots.item_uid IS NULL;

SELECT 'unsafe_nesting_evidence' AS repair_error,item_uid
FROM item_nesting_evidence_invalid
UNION ALL
SELECT 'unsafe_proposed_parent_chain',item_uid
FROM item_nesting_proposed_invalid
LIMIT 20;

-- A duplicate-key error aborts this connection before either update whenever
-- one of the validation tables contains a row.  Closing the connection rolls
-- the transaction back even when the server does not enforce CHECK clauses.
CREATE TEMPORARY TABLE item_nesting_repair_guard (
    guard_id TINYINT UNSIGNED NOT NULL PRIMARY KEY
) ENGINE=InnoDB;
INSERT INTO item_nesting_repair_guard VALUES(1);
INSERT INTO item_nesting_repair_guard
SELECT 1 FROM item_nesting_evidence_invalid
UNION ALL
SELECT 1 FROM item_nesting_proposed_invalid
LIMIT 1;

UPDATE item_current_owner current_item JOIN item_nesting_drift drift
  ON drift.item_uid=current_item.item_uid
SET current_item.parent_item_uid=drift.parent_uid;

-- A repaired item's descendants inherited its stale root, so publish the root
-- already resolved from the validated proposed graph for every affected row.
UPDATE item_current_owner current_item JOIN item_nesting_proposed_roots resolved
  ON resolved.item_uid=current_item.item_uid
SET current_item.root_item_uid=resolved.root_uid
WHERE current_item.root_item_uid<>resolved.root_uid;
COMMIT;
SQL

"$SCRIPT_DIR/repair_item_nesting.sh" --check
