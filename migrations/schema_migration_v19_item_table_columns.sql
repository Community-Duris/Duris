-- ===================================================================
-- schema_migration_v19_item_table_columns.sql
--
-- Phase 3.5 (consolidated schema migration):
-- Adds item_type, wear_flags, bitvector1-5 columns to the 7 item
-- storage tables that were missing them.  These columns exist on
-- player_items (since v17) and are referenced by the production
-- INSERT statements in src/sql_player.c, but the tables below were
-- never updated.  Without this migration, every save to these tables
-- fails with MySQL error 1054 (Unknown column 'item_type' in 'field
-- list') for the two tables that already reference them, and
-- silently loses wear/encrusted-affect data for the rest.
--
-- Column types match player_items exactly:
--   item_type   TINYINT          DEFAULT NULL
--   wear_flags  INT              DEFAULT NULL
--   bitvector1  BIGINT UNSIGNED  DEFAULT NULL
--   bitvector2  BIGINT UNSIGNED  DEFAULT NULL
--   bitvector3  BIGINT UNSIGNED  DEFAULT NULL
--   bitvector4  BIGINT UNSIGNED  DEFAULT NULL
--   bitvector5  BIGINT UNSIGNED  DEFAULT NULL
--
-- The 7 tables covered:
--   corpse_items, locker_items, shopkeeper_items, siege_items,
--   saved_items, player_pet_items, account_locker_items
--
-- Idempotency: each ALTER is wrapped in a prepared statement that
-- checks information_schema.columns first.  If the column already
-- exists, the prepared statement executes `DO 0` as a no-op.  This
-- makes the migration safe to re-run on a database that has already
-- been upgraded (the cycle_mud.sh wrapper also uses `|| true` as a
-- belt-and-suspenders guard).
-- ===================================================================

-- -------------------------------------------------------------------
-- corpse_items
-- -------------------------------------------------------------------
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'corpse_items'
                     AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE corpse_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER extra_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'corpse_items'
                     AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE corpse_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER item_type',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'corpse_items'
                     AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE corpse_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER wear_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'corpse_items'
                     AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE corpse_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'corpse_items'
                     AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE corpse_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'corpse_items'
                     AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE corpse_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'corpse_items'
                     AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE corpse_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- -------------------------------------------------------------------
-- locker_items
-- -------------------------------------------------------------------
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'locker_items'
                     AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE locker_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER extra_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'locker_items'
                     AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE locker_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER item_type',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'locker_items'
                     AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE locker_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER wear_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'locker_items'
                     AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE locker_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'locker_items'
                     AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE locker_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'locker_items'
                     AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE locker_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'locker_items'
                     AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE locker_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- -------------------------------------------------------------------
-- shopkeeper_items
-- -------------------------------------------------------------------
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'shopkeeper_items'
                     AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE shopkeeper_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER extra_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'shopkeeper_items'
                     AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE shopkeeper_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER item_type',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'shopkeeper_items'
                     AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE shopkeeper_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER wear_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'shopkeeper_items'
                     AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE shopkeeper_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'shopkeeper_items'
                     AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE shopkeeper_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'shopkeeper_items'
                     AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE shopkeeper_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'shopkeeper_items'
                     AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE shopkeeper_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- -------------------------------------------------------------------
-- siege_items
-- -------------------------------------------------------------------
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'siege_items'
                     AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE siege_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER extra_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'siege_items'
                     AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE siege_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER item_type',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'siege_items'
                     AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE siege_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER wear_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'siege_items'
                     AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE siege_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'siege_items'
                     AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE siege_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'siege_items'
                     AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE siege_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'siege_items'
                     AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE siege_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- -------------------------------------------------------------------
-- saved_items
-- -------------------------------------------------------------------
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'saved_items'
                     AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE saved_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER extra_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'saved_items'
                     AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE saved_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER item_type',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'saved_items'
                     AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE saved_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER wear_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'saved_items'
                     AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE saved_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'saved_items'
                     AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE saved_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'saved_items'
                     AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE saved_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'saved_items'
                     AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE saved_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- -------------------------------------------------------------------
-- player_pet_items
-- -------------------------------------------------------------------
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'player_pet_items'
                     AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE player_pet_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER extra_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'player_pet_items'
                     AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE player_pet_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER item_type',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'player_pet_items'
                     AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE player_pet_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER wear_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'player_pet_items'
                     AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE player_pet_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'player_pet_items'
                     AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE player_pet_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'player_pet_items'
                     AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE player_pet_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'player_pet_items'
                     AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE player_pet_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- -------------------------------------------------------------------
-- account_locker_items
-- -------------------------------------------------------------------
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'account_locker_items'
                     AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE account_locker_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER extra_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'account_locker_items'
                     AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE account_locker_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER item_type',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'account_locker_items'
                     AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE account_locker_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER wear_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'account_locker_items'
                     AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE account_locker_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'account_locker_items'
                     AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE account_locker_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'account_locker_items'
                     AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE account_locker_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'account_locker_items'
                     AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE account_locker_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
