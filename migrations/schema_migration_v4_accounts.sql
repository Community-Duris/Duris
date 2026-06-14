-- account_characters pfile columns
--
-- Idempotency: all ALTER TABLEs guarded with information_schema checks.
-- CREATE INDEX wrapped with IF NOT EXISTS equivalent.

-- login_count
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'login_count');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN login_count BIGINT UNSIGNED DEFAULT 0',
    'SELECT "login_count already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- last_login
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'last_login');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN last_login BIGINT DEFAULT 0',
    'SELECT "last_login already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- blocked
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'blocked');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN blocked TINYINT DEFAULT 0',
    'SELECT "blocked already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- racewar
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'racewar');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN racewar TINYINT DEFAULT 0',
    'SELECT "racewar already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- index on (account_name, racewar)
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'account_characters'
    AND index_name = 'idx_account_racewar');
SET @sql = IF(@idx_exists = 0,
    'CREATE INDEX idx_account_racewar ON account_characters(account_name, racewar)',
    'SELECT "idx_account_racewar already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
