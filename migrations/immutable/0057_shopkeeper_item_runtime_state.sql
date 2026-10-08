-- This nullable whole-forest marker is native checkpoint completeness evidence.
-- It grants no item custody, issuance, enrollment or trade authority; no backfill.
SET @keeper_checkpoint_sql = IF(
 (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='shopkeepers' AND column_name='runtime_payload_checkpoint_revision')=0
 AND (SELECT COUNT(*) FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='shopkeepers' AND constraint_name='chk_shopkeeper_runtime_checkpoint_revision')=0,
 'ALTER TABLE shopkeepers ADD COLUMN runtime_payload_checkpoint_revision BIGINT UNSIGNED NULL DEFAULT NULL AFTER updated_at, ADD CONSTRAINT chk_shopkeeper_runtime_checkpoint_revision CHECK (runtime_payload_checkpoint_revision IS NULL OR (runtime_payload_checkpoint_revision>0 AND runtime_payload_checkpoint_revision<=shop_revision))',
 IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='shopkeepers' AND BINARY column_name=BINARY 'runtime_payload_checkpoint_revision' AND ordinal_position=10 AND data_type='bigint' AND column_type IN ('bigint unsigned','bigint(20) unsigned') AND is_nullable='YES' AND numeric_precision=20 AND numeric_scale=0 AND character_maximum_length IS NULL AND datetime_precision IS NULL AND (column_default IS NULL OR (LOCATE('MariaDB',VERSION())>0 AND BINARY column_default=BINARY 'NULL')) AND extra='')=1 AND (SELECT COUNT(*) FROM information_schema.table_constraints t JOIN information_schema.check_constraints c ON c.constraint_schema=t.constraint_schema AND c.constraint_name=t.constraint_name WHERE t.constraint_schema=DATABASE() AND t.table_name='shopkeepers' AND BINARY t.constraint_name=BINARY 'chk_shopkeeper_runtime_checkpoint_revision' AND t.constraint_type='CHECK' AND LOWER(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(c.check_clause,CHAR(96),''),' ',''),CHAR(9),''),CHAR(10),''),CHAR(13),''),'(',''),')',''))='runtime_payload_checkpoint_revisionisnullorruntime_payload_checkpoint_revision>0andruntime_payload_checkpoint_revision<=shop_revision')=1,
 'SELECT 1','DURIS_0057_REFUSE_DIVERGENT_KEEPER_CHECKPOINT_SHAPE'));
PREPARE keeper_checkpoint_stmt FROM @keeper_checkpoint_sql;
EXECUTE keeper_checkpoint_stmt;
DEALLOCATE PREPARE keeper_checkpoint_stmt;
SET @keeper_checkpoint_enforcement_sql = IF(LOCATE('MariaDB',VERSION())>0,
 'SELECT 1 INTO @keeper_checkpoint_enforced',
 'SELECT COUNT(*) INTO @keeper_checkpoint_enforced FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name=''shopkeepers'' AND constraint_name=''chk_shopkeeper_runtime_checkpoint_revision'' AND constraint_type=''CHECK'' AND enforced=''YES''');
PREPARE keeper_checkpoint_stmt FROM @keeper_checkpoint_enforcement_sql;
EXECUTE keeper_checkpoint_stmt;
DEALLOCATE PREPARE keeper_checkpoint_stmt;
SET @keeper_checkpoint_sql = IF(@keeper_checkpoint_enforced=1,
 'SELECT 1','DURIS_0057_REFUSE_UNENFORCED_KEEPER_CHECKPOINT_CHECK');
PREPARE keeper_checkpoint_stmt FROM @keeper_checkpoint_sql;
EXECUTE keeper_checkpoint_stmt;
DEALLOCATE PREPARE keeper_checkpoint_stmt;

-- Complete current keeper item payload is subordinate to the physical item row.
-- UID, keeper identity and custody remain in the existing native authorities.
-- No legacy payload is synthesized or rewritten by this additive migration.
CREATE TABLE IF NOT EXISTS shopkeeper_item_runtime_state (
 item_id INT UNSIGNED NOT NULL,
 payload MEDIUMBLOB NOT NULL,
 PRIMARY KEY (item_id),
 CONSTRAINT fk_shopkeeper_item_runtime_state FOREIGN KEY (item_id)
  REFERENCES shopkeeper_items(id) ON UPDATE RESTRICT ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
