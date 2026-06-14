-- obj_uid and item_condition for duplication prevention
-- schema migration v11
--
-- Idempotency: all ALTER TABLEs are guarded with information_schema checks.

-- player_items: rename unique_id to obj_uid, change to bigint, add condition
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_items' AND column_name = 'obj_uid');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_items CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "obj_uid already exists on player_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_items' AND column_name = 'item_condition');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_items ADD COLUMN item_condition SMALLINT DEFAULT 100 AFTER obj_uid',
    'SELECT "item_condition already exists on player_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_items' AND index_name = 'idx_obj_uid');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_items ADD INDEX idx_obj_uid (obj_uid)',
    'SELECT "idx_obj_uid already exists on player_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- corpse_items: same changes
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'obj_uid');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpse_items CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "obj_uid already exists on corpse_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'item_condition');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpse_items ADD COLUMN item_condition SMALLINT DEFAULT 100 AFTER obj_uid',
    'SELECT "item_condition already exists on corpse_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND index_name = 'idx_obj_uid');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE corpse_items ADD INDEX idx_obj_uid (obj_uid)',
    'SELECT "idx_obj_uid already exists on corpse_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- locker_items: same changes
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'obj_uid');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "obj_uid already exists on locker_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'item_condition');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items ADD COLUMN item_condition SMALLINT DEFAULT 100 AFTER obj_uid',
    'SELECT "item_condition already exists on locker_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND index_name = 'idx_obj_uid');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE locker_items ADD INDEX idx_obj_uid (obj_uid)',
    'SELECT "idx_obj_uid already exists on locker_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_pet_items: add obj_uid and condition (no unique_id existed)
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND column_name = 'obj_uid');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_pet_items ADD COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL AFTER action_descr',
    'SELECT "obj_uid already exists on player_pet_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND column_name = 'item_condition');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_pet_items ADD COLUMN item_condition SMALLINT DEFAULT 100 AFTER obj_uid',
    'SELECT "item_condition already exists on player_pet_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND index_name = 'idx_obj_uid');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_pet_items ADD INDEX idx_obj_uid (obj_uid)',
    'SELECT "idx_obj_uid already exists on player_pet_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
