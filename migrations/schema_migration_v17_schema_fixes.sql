-- schema_migration_v17_schema_fixes.sql
-- Adds missing columns to tables that the wip-async code expects but
-- which were not present in the base schema migration (run_this_one.sql).
--
-- Idempotency: all ALTER TABLEs are guarded with information_schema checks.

-- corpse_items: item_type
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpse_items ADD COLUMN item_type INT NOT NULL DEFAULT 0',
    'SELECT "item_type already exists on corpse_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_items: item_type
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_items' AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_items ADD COLUMN item_type INT NOT NULL DEFAULT 0',
    'SELECT "item_type already exists on player_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_items: wear_flags
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_items' AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_items ADD COLUMN wear_flags INT UNSIGNED NOT NULL DEFAULT 0',
    'SELECT "wear_flags already exists on player_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- auctions: obj_info_text
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'auctions' AND column_name = 'obj_info_text');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE auctions ADD COLUMN obj_info_text TEXT NULL',
    'SELECT "obj_info_text already exists on auctions"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
