-- Migration: add item_material column to the 7 item tables
-- Phase 3.6: persist obj->material as a diff-from-prototype value
--   NULL = item matches its prototype's material (load code uses proto)
--   non-NULL = item has a custom material override
--
-- The shared helper sql_format_item_diff_fields_and_free_proto() in
-- src/sql_player.c writes "NULL" when the item's material matches its
-- prototype, or the numeric value otherwise.
--
-- This migration adds the column to the same 7 tables that received the
-- v19 diff columns (item_type, wear_flags, bitvector1-5):
--   corpse_items, locker_items, shopkeeper_items, siege_items,
--   saved_items, player_pet_items, account_locker_items
--
-- Idempotency: all ALTER TABLEs are guarded with information_schema checks.

-- corpse_items
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpse_items ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5',
    'SELECT "item_material already exists on corpse_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- locker_items
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5',
    'SELECT "item_material already exists on locker_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- shopkeeper_items
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'shopkeeper_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE shopkeeper_items ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5',
    'SELECT "item_material already exists on shopkeeper_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- siege_items
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'siege_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE siege_items ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5',
    'SELECT "item_material already exists on siege_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- saved_items
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'saved_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE saved_items ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5',
    'SELECT "item_material already exists on saved_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_pet_items
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_pet_items ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5',
    'SELECT "item_material already exists on player_pet_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- account_locker_items
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_locker_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_locker_items ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5',
    'SELECT "item_material already exists on account_locker_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
