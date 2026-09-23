-- Persistent state for renewable, damage-limited spell wards.
-- Each guard keeps this migration safe for databases that were partially
-- upgraded before the full ward feature was deployed.

SET @ward_sql = IF(
    (SELECT COUNT(*) FROM information_schema.columns
       WHERE table_schema = DATABASE()
         AND table_name = 'player_affects'
         AND column_name = 'ward_source_uid') = 0,
    'ALTER TABLE player_affects ADD COLUMN ward_source_uid BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER custom_msg_room',
    'SELECT 1');
PREPARE ward_stmt FROM @ward_sql;
EXECUTE ward_stmt;
DEALLOCATE PREPARE ward_stmt;

SET @ward_sql = IF(
    (SELECT COUNT(*) FROM information_schema.columns
       WHERE table_schema = DATABASE()
         AND table_name = 'player_affects'
         AND column_name = 'ward_full_duration') = 0,
    'ALTER TABLE player_affects ADD COLUMN ward_full_duration INT NOT NULL DEFAULT 0 AFTER ward_source_uid',
    'SELECT 1');
PREPARE ward_stmt FROM @ward_sql;
EXECUTE ward_stmt;
DEALLOCATE PREPARE ward_stmt;

SET @ward_sql = IF(
    (SELECT COUNT(*) FROM information_schema.columns
       WHERE table_schema = DATABASE()
         AND table_name = 'player_affects'
         AND column_name = 'ward_capacity') = 0,
    'ALTER TABLE player_affects ADD COLUMN ward_capacity BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER ward_full_duration',
    'SELECT 1');
PREPARE ward_stmt FROM @ward_sql;
EXECUTE ward_stmt;
DEALLOCATE PREPARE ward_stmt;

SET @ward_sql = IF(
    (SELECT COUNT(*) FROM information_schema.columns
       WHERE table_schema = DATABASE()
         AND table_name = 'player_affects'
         AND column_name = 'ward_capacity_max') = 0,
    'ALTER TABLE player_affects ADD COLUMN ward_capacity_max BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER ward_capacity',
    'SELECT 1');
PREPARE ward_stmt FROM @ward_sql;
EXECUTE ward_stmt;
DEALLOCATE PREPARE ward_stmt;

SET @ward_sql = IF(
    (SELECT COUNT(*) FROM information_schema.columns
       WHERE table_schema = DATABASE()
         AND table_name = 'player_affects'
         AND column_name = 'ward_refresh_remaining') = 0,
    'ALTER TABLE player_affects ADD COLUMN ward_refresh_remaining INT NOT NULL DEFAULT 0 AFTER ward_capacity_max',
    'SELECT 1');
PREPARE ward_stmt FROM @ward_sql;
EXECUTE ward_stmt;
DEALLOCATE PREPARE ward_stmt;

SET @ward_sql = IF(
    (SELECT COUNT(*) FROM information_schema.columns
       WHERE table_schema = DATABASE()
         AND table_name = 'player_affects'
         AND column_name = 'ward_source_type') = 0,
    'ALTER TABLE player_affects ADD COLUMN ward_source_type TINYINT UNSIGNED NOT NULL DEFAULT 0 AFTER ward_refresh_remaining',
    'SELECT 1');
PREPARE ward_stmt FROM @ward_sql;
EXECUTE ward_stmt;
DEALLOCATE PREPARE ward_stmt;

SET @ward_sql = IF(
    (SELECT COUNT(*) FROM information_schema.columns
       WHERE table_schema = DATABASE()
         AND table_name = 'player_affects'
         AND column_name = 'ward_source_worn') = 0,
    'ALTER TABLE player_affects ADD COLUMN ward_source_worn TINYINT UNSIGNED NOT NULL DEFAULT 0 AFTER ward_source_type',
    'SELECT 1');
PREPARE ward_stmt FROM @ward_sql;
EXECUTE ward_stmt;
DEALLOCATE PREPARE ward_stmt;

SET @ward_sql = IF(
    (SELECT COUNT(*) FROM information_schema.columns
       WHERE table_schema = DATABASE()
         AND table_name = 'player_affects'
         AND column_name = 'ward_active') = 0,
    'ALTER TABLE player_affects ADD COLUMN ward_active TINYINT UNSIGNED NOT NULL DEFAULT 0 AFTER ward_source_worn',
    'SELECT 1');
PREPARE ward_stmt FROM @ward_sql;
EXECUTE ward_stmt;
DEALLOCATE PREPARE ward_stmt;
