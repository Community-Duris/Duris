-- Persist the outer player-corpse object, not only the items inside it.
-- Each column is guarded independently so this file is safe to replay on
-- production-derived schemas with partial migrations.

SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'corpses' AND column_name = 'short_descr') = 0, 'ALTER TABLE corpses ADD COLUMN short_descr VARCHAR(512) DEFAULT NULL AFTER created_at', 'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'corpses' AND column_name = 'description') = 0, 'ALTER TABLE corpses ADD COLUMN description TEXT DEFAULT NULL AFTER short_descr', 'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'corpses' AND column_name = 'name') = 0, 'ALTER TABLE corpses ADD COLUMN name VARCHAR(512) DEFAULT NULL AFTER description', 'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'corpses' AND column_name = 'weight') = 0, 'ALTER TABLE corpses ADD COLUMN weight INT DEFAULT NULL AFTER name', 'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'corpses' AND column_name = 'value0') = 0, 'ALTER TABLE corpses ADD COLUMN value0 INT DEFAULT NULL AFTER weight', 'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'corpses' AND column_name = 'value1') = 0, 'ALTER TABLE corpses ADD COLUMN value1 INT DEFAULT NULL AFTER value0', 'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'corpses' AND column_name = 'value2') = 0, 'ALTER TABLE corpses ADD COLUMN value2 INT DEFAULT NULL AFTER value1', 'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'corpses' AND column_name = 'value3') = 0, 'ALTER TABLE corpses ADD COLUMN value3 INT DEFAULT NULL AFTER value2', 'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'corpses' AND column_name = 'value4') = 0, 'ALTER TABLE corpses ADD COLUMN value4 INT DEFAULT NULL AFTER value3', 'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'corpses' AND column_name = 'value5') = 0, 'ALTER TABLE corpses ADD COLUMN value5 INT DEFAULT NULL AFTER value4', 'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'corpses' AND column_name = 'value7') = 0, 'ALTER TABLE corpses ADD COLUMN value7 INT DEFAULT NULL AFTER value5', 'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- All rows in this table are player corpses. These two fields can be rebuilt
-- exactly for legacy rows; death-time level, XP, race, weight, and carved state
-- deliberately remain NULL when no authoritative historical value exists.
UPDATE corpses
SET name = CONCAT(player_name, ' corpse _pcorpse_')
WHERE name IS NULL OR name = '';

UPDATE corpses
SET value1 = 1
WHERE value1 IS NULL;

-- Repair only the signature produced by the shifted loader: a numeric item
-- condition became short_descr while the former short description became the
-- room description. A generic long description is safer than inventing the
-- player's death-time race.
UPDATE corpses
SET short_descr = CONCAT('the corpse of ', player_name),
    description = CONCAT('The corpse of ', player_name, ' is lying here.')
WHERE TRIM(short_descr) REGEXP '^[0-9]+$'
  AND LOWER(TRIM(description)) = LOWER(CONCAT('the corpse of ', player_name));
