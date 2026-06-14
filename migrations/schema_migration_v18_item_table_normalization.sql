-- schema_migration_v18_item_table_normalization.sql
-- Consolidates three previously-planned separate migrations into one:
--   (a) Extend v17 item_type + wear_flags to all item tables (§10.6)
--   (b) Add bitvector1-5 to item tables that lack them (§10.7)
--   (c) Create extra_descr tables for item tables that lack them (§10.8)
--
-- All statements are idempotent (use IF NOT EXISTS / information_schema checks).
-- Runs on every boot via cycle_mud.sh, safe for repeated execution.


-- ============================================================================
-- Part 1: item_type column — present on player_items + corpse_items (v17),
--         missing from locker_items, shopkeeper_items, saved_items,
--         siege_items, account_locker_items, player_pet_items (§10.6)
-- ============================================================================

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items ADD COLUMN item_type INT NOT NULL DEFAULT 0',
    'SELECT "locker_items.item_type already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'shopkeeper_items' AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE shopkeeper_items ADD COLUMN item_type INT NOT NULL DEFAULT 0',
    'SELECT "shopkeeper_items.item_type already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'saved_items' AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE saved_items ADD COLUMN item_type INT NOT NULL DEFAULT 0',
    'SELECT "saved_items.item_type already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'siege_items' AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE siege_items ADD COLUMN item_type INT NOT NULL DEFAULT 0',
    'SELECT "siege_items.item_type already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_locker_items' AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_locker_items ADD COLUMN item_type INT NOT NULL DEFAULT 0',
    'SELECT "account_locker_items.item_type already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_pet_items ADD COLUMN item_type INT NOT NULL DEFAULT 0',
    'SELECT "player_pet_items.item_type already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;


-- ============================================================================
-- Part 2: wear_flags column — present only on player_items (v17),
--         missing from corpse_items, locker_items, shopkeeper_items,
--         saved_items, siege_items, account_locker_items, player_pet_items (§10.6)
-- ============================================================================

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpse_items ADD COLUMN wear_flags INT UNSIGNED NOT NULL DEFAULT 0',
    'SELECT "corpse_items.wear_flags already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items ADD COLUMN wear_flags INT UNSIGNED NOT NULL DEFAULT 0',
    'SELECT "locker_items.wear_flags already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'shopkeeper_items' AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE shopkeeper_items ADD COLUMN wear_flags INT UNSIGNED NOT NULL DEFAULT 0',
    'SELECT "shopkeeper_items.wear_flags already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'saved_items' AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE saved_items ADD COLUMN wear_flags INT UNSIGNED NOT NULL DEFAULT 0',
    'SELECT "saved_items.wear_flags already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'siege_items' AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE siege_items ADD COLUMN wear_flags INT UNSIGNED NOT NULL DEFAULT 0',
    'SELECT "siege_items.wear_flags already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_locker_items' AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_locker_items ADD COLUMN wear_flags INT UNSIGNED NOT NULL DEFAULT 0',
    'SELECT "account_locker_items.wear_flags already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_pet_items ADD COLUMN wear_flags INT UNSIGNED NOT NULL DEFAULT 0',
    'SELECT "player_pet_items.wear_flags already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;


-- ============================================================================
-- Part 3: bitvector1-5 columns — present on player_items, locker_items (v14),
--         and account_locker_items (v14).
--         Missing from corpse_items, shopkeeper_items, saved_items,
--         siege_items, player_pet_items (§10.7)
-- ============================================================================

-- helper: add a bitvector column to a table if missing
-- Using inline repetition (Mysql 5.7 compatible — no reusable procedures in idempotent migration)

-- ---- corpse_items ----

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpse_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "corpse_items.bitvector1 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpse_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "corpse_items.bitvector2 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpse_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "corpse_items.bitvector3 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpse_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "corpse_items.bitvector4 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpse_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "corpse_items.bitvector5 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- ---- shopkeeper_items ----

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'shopkeeper_items' AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE shopkeeper_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "shopkeeper_items.bitvector1 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'shopkeeper_items' AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE shopkeeper_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "shopkeeper_items.bitvector2 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'shopkeeper_items' AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE shopkeeper_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "shopkeeper_items.bitvector3 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'shopkeeper_items' AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE shopkeeper_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "shopkeeper_items.bitvector4 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'shopkeeper_items' AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE shopkeeper_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "shopkeeper_items.bitvector5 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- ---- saved_items ----

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'saved_items' AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE saved_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "saved_items.bitvector1 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'saved_items' AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE saved_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "saved_items.bitvector2 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'saved_items' AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE saved_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "saved_items.bitvector3 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'saved_items' AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE saved_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "saved_items.bitvector4 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'saved_items' AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE saved_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "saved_items.bitvector5 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- ---- siege_items ----

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'siege_items' AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE siege_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "siege_items.bitvector1 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'siege_items' AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE siege_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "siege_items.bitvector2 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'siege_items' AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE siege_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "siege_items.bitvector3 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'siege_items' AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE siege_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "siege_items.bitvector4 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'siege_items' AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE siege_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "siege_items.bitvector5 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- ---- player_pet_items ----

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_pet_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "player_pet_items.bitvector1 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_pet_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "player_pet_items.bitvector2 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_pet_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "player_pet_items.bitvector3 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_pet_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "player_pet_items.bitvector4 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_pet_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "player_pet_items.bitvector5 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;


-- ============================================================================
-- Part 4: extra_descr tables — present for player_items, corpse_items,
--         locker_items, player_pet_items.
--         Missing for shopkeeper_items, saved_items, siege_items,
--         account_locker_items (§10.8)
-- ============================================================================

CREATE TABLE IF NOT EXISTS shopkeeper_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_shopkeeper_item_ed FOREIGN KEY (item_id)
    REFERENCES shopkeeper_items(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS saved_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_saved_item_ed FOREIGN KEY (item_id)
    REFERENCES saved_items(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS siege_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_siege_item_ed FOREIGN KEY (item_id)
    REFERENCES siege_items(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS account_locker_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_account_locker_item_ed FOREIGN KEY (item_id)
    REFERENCES account_locker_items(id) ON DELETE CASCADE
);


-- ============================================================================
-- Summary:
--   item_type added to:      locker_items, shopkeeper_items, saved_items,
--                            siege_items, account_locker_items, player_pet_items
--   wear_flags added to:     corpse_items, locker_items, shopkeeper_items,
--                            saved_items, siege_items, account_locker_items,
--                            player_pet_items
--   bitvector1-5 added to:   corpse_items, shopkeeper_items, saved_items,
--                            siege_items, player_pet_items
--   extra_descr tables for:  shopkeeper_items, saved_items, siege_items,
--                            account_locker_items
--
-- After this migration, ALL 8 item tables have a uniform column set:
--   player_items, corpse_items, locker_items, account_locker_items,
--   shopkeeper_items, saved_items, siege_items, player_pet_items
-- ============================================================================
