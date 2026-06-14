-- create corpse_item_extra_descr and locker_item_extra_descr tables and add description columns to corpse table
-- schema migration v15
--
-- Idempotency: CREATE TABLE uses IF NOT EXISTS. ALTER TABLEs guarded with
-- information_schema checks.

CREATE TABLE IF NOT EXISTS corpse_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_corpse_item_ed FOREIGN KEY (item_id)
    REFERENCES corpse_items(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS locker_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_locker_item_ed FOREIGN KEY (item_id)
    REFERENCES locker_items(id) ON DELETE CASCADE
);

-- corpses: short_descr
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpses' AND column_name = 'short_descr');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpses ADD COLUMN short_descr VARCHAR(512) DEFAULT NULL AFTER created_at',
    'SELECT "short_descr already exists on corpses"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- corpses: description
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpses' AND column_name = 'description');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpses ADD COLUMN description TEXT DEFAULT NULL AFTER short_descr',
    'SELECT "description already exists on corpses"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_data: CONVERT TO CHARACTER SET (idempotent - no-op if already utf8mb4)
-- Only run if not already utf8mb4_0900_ai_ci
SET @charset_ok = (SELECT COUNT(*) FROM information_schema.tables
    WHERE table_schema = DATABASE() AND table_name = 'player_data'
    AND table_collation = 'utf8mb4_0900_ai_ci');
SET @sql = IF(@charset_ok = 0,
    'ALTER TABLE player_data CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci',
    'SELECT "player_data already utf8mb4_0900_ai_ci"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
