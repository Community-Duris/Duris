-- schema migration v8: add unique constraints for character names
--
-- Idempotency: DELETE uses a self-join to remove duplicates (safe re-run).
-- ALTER TABLEs are guarded with information_schema checks.

-- first, clean up any existing duplicates (keep lowest pid)
DELETE ac1 FROM account_characters ac1
INNER JOIN account_characters ac2
WHERE ac1.char_name = ac2.char_name
  AND ac1.pid > ac2.pid;

-- add unique constraint on char_name (character names must be unique)
SET @key_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'account_characters'
    AND index_name = 'idx_char_name_unique');
SET @sql = IF(@key_exists = 0,
    'ALTER TABLE account_characters ADD UNIQUE INDEX idx_char_name_unique (char_name)',
    'SELECT "idx_char_name_unique already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- add unique constraint on player_data.name
SET @key_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_data'
    AND index_name = 'idx_player_name_unique');
SET @sql = IF(@key_exists = 0,
    'ALTER TABLE player_data ADD UNIQUE INDEX idx_player_name_unique (name)',
    'SELECT "idx_player_name_unique already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
