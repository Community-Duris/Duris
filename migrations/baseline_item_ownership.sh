#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
# shellcheck disable=SC1091
source "$PROJECT_ROOT/.env"
environment_name="${ENVIRONMENT:-${APP_ENV:-}}"
[[ "${environment_name,,}" =~ (dev|local|test) ]] || { echo 'refusing item baseline: environment is not development/local/test' >&2; exit 1; }
[[ "${DB_NAME,,}" =~ (dev|local|test) ]] || { echo 'refusing item baseline: database name is not development/local/test' >&2; exit 1; }
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" "$DB_NAME")

"${MYSQL[@]}" <<'SQL'
START TRANSACTION;
CREATE TEMPORARY TABLE item_custody_evidence (
    source_table VARCHAR(32) NOT NULL, source_row_id BIGINT UNSIGNED NOT NULL,
    item_uid BIGINT UNSIGNED NOT NULL, parent_uid BIGINT UNSIGNED NULL,
    root_uid BIGINT UNSIGNED NOT NULL, owner_type TINYINT UNSIGNED NOT NULL,
    owner_id BIGINT UNSIGNED NOT NULL, owner_context_id BIGINT UNSIGNED NOT NULL,
    vnum INT NOT NULL, broken_parent TINYINT UNSIGNED NOT NULL,
    PRIMARY KEY(source_table,source_row_id), KEY idx_evidence_uid(item_uid)
) ENGINE=InnoDB;

INSERT INTO item_custody_evidence
SELECT 'player_items',i.id,COALESCE(i.obj_uid,0),p.obj_uid,COALESCE(i.obj_uid,0),1,i.pid,0,i.vnum,
       IF(i.container_id IS NOT NULL AND p.obj_uid IS NULL,1,0)
FROM player_items i LEFT JOIN player_items p ON p.id=i.container_id;
INSERT INTO item_custody_evidence
SELECT 'corpse_items',i.id,COALESCE(i.obj_uid,0),p.obj_uid,COALESCE(i.obj_uid,0),4,
       ((CAST(pd.pid AS UNSIGNED) << 32) | (CAST(c.save_id AS UNSIGNED) & 4294967295)),0,i.vnum,
       IF(i.container_id IS NOT NULL AND p.obj_uid IS NULL,1,0)
FROM corpse_items i JOIN corpses c ON c.id=i.corpse_id
JOIN player_data pd ON LOWER(pd.name)=LOWER(c.player_name)
LEFT JOIN corpse_items p ON p.id=i.container_id;
INSERT INTO item_custody_evidence
SELECT 'locker_items',i.id,COALESCE(i.obj_uid,0),p.obj_uid,COALESCE(i.obj_uid,0),5,i.locker_id,
       COALESCE(i.chest_id,public_chest.id,0),i.vnum,
       IF(i.container_id IS NOT NULL AND p.obj_uid IS NULL,1,0)
FROM locker_items i LEFT JOIN locker_items p ON p.id=i.container_id
LEFT JOIN private_chests public_chest
  ON public_chest.locker_id=i.locker_id AND public_chest.is_public=1;
INSERT INTO item_custody_evidence
SELECT 'account_locker_items',i.id,COALESCE(i.obj_uid,0),p.obj_uid,COALESCE(i.obj_uid,0),5,i.chest_id,0,i.vnum,
       IF(i.container_id IS NOT NULL AND p.obj_uid IS NULL,1,0)
FROM account_locker_items i LEFT JOIN account_locker_items p ON p.id=i.container_id;
INSERT INTO item_custody_evidence
SELECT 'saved_items',i.id,COALESCE(i.obj_uid,0),p.obj_uid,COALESCE(i.obj_uid,0),3,CAST(i.room_vnum AS UNSIGNED),0,i.vnum,
       IF(i.container_id IS NOT NULL AND p.obj_uid IS NULL,1,0)
FROM saved_items i LEFT JOIN saved_items p ON p.id=i.container_id;
INSERT INTO item_custody_evidence
SELECT 'auctions',id,0,NULL,0,6,CAST(id AS UNSIGNED),0,obj_vnum,0
FROM auctions WHERE status='OPEN';
INSERT INTO item_custody_evidence
SELECT 'auction_item_pickups',id,0,NULL,0,6,CAST(id AS UNSIGNED),1,0,0
FROM auction_item_pickups WHERE retrieved=0;

INSERT IGNORE INTO item_ownership_quarantine(item_uid,source_table,source_row_id,conflict_code,evidence)
SELECT item_uid,source_table,source_row_id,3,'legacy custody row has no stable item UID'
FROM item_custody_evidence WHERE item_uid=0;
INSERT IGNORE INTO item_ownership_quarantine(item_uid,source_table,source_row_id,conflict_code,evidence)
SELECT item_uid,source_table,source_row_id,2,'container link has no stable parent item UID'
FROM item_custody_evidence WHERE item_uid>0 AND broken_parent=1;
INSERT IGNORE INTO item_ownership_quarantine(item_uid,source_table,source_row_id,conflict_code,evidence)
SELECT evidence.item_uid,evidence.source_table,evidence.source_row_id,1,
       'stable item UID occurs in more than one retained custody row'
FROM item_custody_evidence evidence
JOIN (SELECT item_uid FROM item_custody_evidence WHERE item_uid>0 GROUP BY item_uid HAVING COUNT(*)>1) duplicate
  ON duplicate.item_uid=evidence.item_uid;
INSERT IGNORE INTO item_ownership_quarantine(item_uid,source_table,source_row_id,conflict_code,evidence)
SELECT child.item_uid,child.source_table,child.source_row_id,2,
       'container parent identity is ambiguous across retained custody rows'
FROM item_custody_evidence child
JOIN (SELECT item_uid FROM item_custody_evidence WHERE item_uid>0 GROUP BY item_uid HAVING COUNT(*)>1) duplicate_parent
  ON duplicate_parent.item_uid=child.parent_uid
WHERE child.item_uid>0;
INSERT IGNORE INTO item_ownership_quarantine(item_uid,source_table,source_row_id,conflict_code,evidence)
SELECT evidence.item_uid,evidence.source_table,evidence.source_row_id,4,
       'legacy custody conflicts with an existing authoritative owner row'
FROM item_custody_evidence evidence JOIN item_current_owner current_item
  ON current_item.item_uid=evidence.item_uid
WHERE evidence.item_uid>0 AND
      (current_item.owner_type<>evidence.owner_type OR
       current_item.owner_id<>evidence.owner_id OR
       current_item.owner_context_id<>evidence.owner_context_id OR
       COALESCE(current_item.parent_item_uid,0)<>COALESCE(evidence.parent_uid,0) OR
       current_item.vnum<>evidence.vnum);

CREATE TEMPORARY TABLE tainted_item_uids (
    item_uid BIGINT UNSIGNED NOT NULL PRIMARY KEY
) ENGINE=InnoDB AS
WITH RECURSIVE tainted(item_uid,depth) AS (
  SELECT DISTINCT item_uid,0 FROM item_ownership_quarantine
  WHERE repaired_at IS NULL AND item_uid>0
  UNION DISTINCT
  SELECT child.item_uid,tainted.depth+1
  FROM tainted JOIN item_custody_evidence child ON child.parent_uid=tainted.item_uid
  WHERE tainted.depth<32
)
SELECT DISTINCT item_uid FROM tainted;
INSERT IGNORE INTO item_ownership_quarantine(item_uid,source_table,source_row_id,conflict_code,evidence)
SELECT child.item_uid,child.source_table,child.source_row_id,2,
       'container ancestry reaches quarantined custody evidence'
FROM item_custody_evidence child JOIN tainted_item_uids tainted
  ON tainted.item_uid=child.item_uid
LEFT JOIN item_ownership_quarantine existing
  ON existing.item_uid=child.item_uid AND existing.repaired_at IS NULL
WHERE child.item_uid>0 AND existing.quarantine_id IS NULL;

CREATE TEMPORARY TABLE unambiguous_item_evidence LIKE item_custody_evidence;
INSERT INTO unambiguous_item_evidence
SELECT evidence.* FROM item_custody_evidence evidence
LEFT JOIN item_ownership_quarantine quarantine
  ON quarantine.item_uid=evidence.item_uid AND quarantine.repaired_at IS NULL
WHERE evidence.item_uid>0 AND evidence.broken_parent=0 AND quarantine.quarantine_id IS NULL;

CREATE TEMPORARY TABLE resolved_item_roots (
    item_uid BIGINT UNSIGNED NOT NULL PRIMARY KEY,
    root_uid BIGINT UNSIGNED NOT NULL
) ENGINE=InnoDB AS
WITH RECURSIVE ancestry(start_uid,current_uid,parent_uid,depth) AS (
  SELECT item_uid,item_uid,parent_uid,0 FROM unambiguous_item_evidence
  UNION ALL
  SELECT ancestry.start_uid,parent.item_uid,parent.parent_uid,ancestry.depth+1
  FROM ancestry JOIN unambiguous_item_evidence parent
    ON parent.item_uid=ancestry.parent_uid
  WHERE ancestry.depth<32
)
SELECT start_uid AS item_uid,current_uid AS root_uid
FROM ancestry WHERE parent_uid IS NULL;
UPDATE unambiguous_item_evidence evidence JOIN resolved_item_roots resolved
  ON resolved.item_uid=evidence.item_uid
SET evidence.root_uid=resolved.root_uid;

INSERT IGNORE INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision)
SELECT DISTINCT owner_type,owner_id,owner_context_id,0 FROM unambiguous_item_evidence;
INSERT IGNORE INTO item_ownership_baseline(item_uid,root_item_uid,parent_item_uid,owner_type,
    owner_id,owner_context_id,opening_item_revision,vnum,source_table,source_row_id)
SELECT item_uid,root_uid,parent_uid,owner_type,owner_id,owner_context_id,0,vnum,source_table,source_row_id
FROM unambiguous_item_evidence ORDER BY item_uid;
INSERT IGNORE INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,
    owner_context_id,item_revision,vnum,state)
SELECT item_uid,root_uid,NULL,owner_type,owner_id,owner_context_id,0,vnum,1
FROM unambiguous_item_evidence ORDER BY item_uid;
UPDATE item_current_owner current_item JOIN unambiguous_item_evidence evidence
  ON evidence.item_uid=current_item.item_uid
SET current_item.parent_item_uid=evidence.parent_uid
WHERE current_item.parent_item_uid IS NULL AND evidence.parent_uid IS NOT NULL;
COMMIT;
SELECT COUNT(*) AS authoritative_items FROM item_current_owner;
SELECT COUNT(*) AS open_quarantine_rows FROM item_ownership_quarantine WHERE repaired_at IS NULL;
SQL
