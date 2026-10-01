-- A corpse-derived pet identity remains stable while player_pets.id is a
-- projection key for nested player_pet_items rows. NULL preserves legacy pets.
SET @pet_uid_exists = (SELECT COUNT(*) FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='player_pets' AND COLUMN_NAME='pet_uid');
SET @pet_uid_sql = IF(@pet_uid_exists=0,
  'ALTER TABLE player_pets ADD COLUMN pet_uid BIGINT UNSIGNED NULL', 'SELECT 1');
PREPARE pet_uid_stmt FROM @pet_uid_sql;
EXECUTE pet_uid_stmt;
DEALLOCATE PREPARE pet_uid_stmt;

SET @pet_uid_index_exists = (SELECT COUNT(*) FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='player_pets'
    AND INDEX_NAME='uq_player_pets_pet_uid');
SET @pet_uid_index_sql = IF(@pet_uid_index_exists=0,
  'CREATE UNIQUE INDEX uq_player_pets_pet_uid ON player_pets(pet_uid)', 'SELECT 1');
PREPARE pet_uid_index_stmt FROM @pet_uid_index_sql;
EXECUTE pet_uid_index_stmt;
DEALLOCATE PREPARE pet_uid_index_stmt;

-- Pet custody is a distinct durable owner. Keep the three ownership
-- constraints in sync; this replacement is safe to repeat after a partial DDL run.
ALTER TABLE item_owner_revision
  DROP CONSTRAINT chk_item_owner_revision_type,
  ADD CONSTRAINT chk_item_owner_revision_type CHECK (owner_type BETWEEN 1 AND 11);
ALTER TABLE item_current_owner
  DROP CONSTRAINT chk_item_current_owner_type,
  ADD CONSTRAINT chk_item_current_owner_type CHECK (owner_type BETWEEN 1 AND 11);
ALTER TABLE item_ownership_baseline
  DROP CONSTRAINT chk_item_baseline_owner_type,
  ADD CONSTRAINT chk_item_baseline_owner_type CHECK (owner_type BETWEEN 1 AND 11);
