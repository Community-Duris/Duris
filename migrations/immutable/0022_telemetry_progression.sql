-- Add the first post-foundation progression fact fields to the tagged stream.
-- The migration is additive and rerunnable; no gameplay or rollup table is
-- changed. Nullable fields remain absent for every pre-existing record kind.

SET @progression_kind_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='progression_kind'),
    'SELECT 1',
    'ALTER TABLE telemetry_interval ADD COLUMN progression_kind TINYINT UNSIGNED NULL AFTER enabled');
PREPARE progression_kind_stmt FROM @progression_kind_sql;
EXECUTE progression_kind_stmt;
DEALLOCATE PREPARE progression_kind_stmt;

SET @progression_source_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='progression_source'),
    'SELECT 1',
    'ALTER TABLE telemetry_interval ADD COLUMN progression_source TINYINT UNSIGNED NULL AFTER progression_kind');
PREPARE progression_source_stmt FROM @progression_source_sql;
EXECUTE progression_source_stmt;
DEALLOCATE PREPARE progression_source_stmt;

SET @progression_reason_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='progression_reason'),
    'SELECT 1',
    'ALTER TABLE telemetry_interval ADD COLUMN progression_reason TINYINT UNSIGNED NULL AFTER progression_source');
PREPARE progression_reason_stmt FROM @progression_reason_sql;
EXECUTE progression_reason_stmt;
DEALLOCATE PREPARE progression_reason_stmt;

SET @progression_observation_status_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='progression_observation_status'),
    'SELECT 1',
    'ALTER TABLE telemetry_interval ADD COLUMN progression_observation_status TINYINT UNSIGNED NULL AFTER progression_reason');
PREPARE progression_observation_status_stmt FROM @progression_observation_status_sql;
EXECUTE progression_observation_status_stmt;
DEALLOCATE PREPARE progression_observation_status_stmt;

SET @progression_modifier_flags_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='progression_modifier_flags'),
    'SELECT 1',
    'ALTER TABLE telemetry_interval ADD COLUMN progression_modifier_flags INT UNSIGNED NULL AFTER progression_observation_status');
PREPARE progression_modifier_flags_stmt FROM @progression_modifier_flags_sql;
EXECUTE progression_modifier_flags_stmt;
DEALLOCATE PREPARE progression_modifier_flags_stmt;

SET @progression_requested_xp_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='progression_requested_xp'),
    'SELECT 1',
    'ALTER TABLE telemetry_interval ADD COLUMN progression_requested_xp BIGINT NULL AFTER progression_modifier_flags');
PREPARE progression_requested_xp_stmt FROM @progression_requested_xp_sql;
EXECUTE progression_requested_xp_stmt;
DEALLOCATE PREPARE progression_requested_xp_stmt;

SET @progression_computed_xp_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='progression_computed_xp'),
    'SELECT 1',
    'ALTER TABLE telemetry_interval ADD COLUMN progression_computed_xp BIGINT NULL AFTER progression_requested_xp');
PREPARE progression_computed_xp_stmt FROM @progression_computed_xp_sql;
EXECUTE progression_computed_xp_stmt;
DEALLOCATE PREPARE progression_computed_xp_stmt;

SET @progression_applied_xp_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='progression_applied_xp'),
    'SELECT 1',
    'ALTER TABLE telemetry_interval ADD COLUMN progression_applied_xp BIGINT NULL AFTER progression_computed_xp');
PREPARE progression_applied_xp_stmt FROM @progression_applied_xp_sql;
EXECUTE progression_applied_xp_stmt;
DEALLOCATE PREPARE progression_applied_xp_stmt;

SET @progression_before_exp_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='progression_before_exp'),
    'SELECT 1',
    'ALTER TABLE telemetry_interval ADD COLUMN progression_before_exp BIGINT NULL AFTER progression_applied_xp');
PREPARE progression_before_exp_stmt FROM @progression_before_exp_sql;
EXECUTE progression_before_exp_stmt;
DEALLOCATE PREPARE progression_before_exp_stmt;

SET @progression_after_exp_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='progression_after_exp'),
    'SELECT 1',
    'ALTER TABLE telemetry_interval ADD COLUMN progression_after_exp BIGINT NULL AFTER progression_before_exp');
PREPARE progression_after_exp_stmt FROM @progression_after_exp_sql;
EXECUTE progression_after_exp_stmt;
DEALLOCATE PREPARE progression_after_exp_stmt;

SET @progression_before_level_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='progression_before_level'),
    'SELECT 1',
    'ALTER TABLE telemetry_interval ADD COLUMN progression_before_level SMALLINT UNSIGNED NULL AFTER progression_after_exp');
PREPARE progression_before_level_stmt FROM @progression_before_level_sql;
EXECUTE progression_before_level_stmt;
DEALLOCATE PREPARE progression_before_level_stmt;

SET @progression_after_level_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='progression_after_level'),
    'SELECT 1',
    'ALTER TABLE telemetry_interval ADD COLUMN progression_after_level SMALLINT UNSIGNED NULL AFTER progression_before_level');
PREPARE progression_after_level_stmt FROM @progression_after_level_sql;
EXECUTE progression_after_level_stmt;
DEALLOCATE PREPARE progression_after_level_stmt;

SET @progression_threshold_xp_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='progression_threshold_xp'),
    'SELECT 1',
    'ALTER TABLE telemetry_interval ADD COLUMN progression_threshold_xp BIGINT UNSIGNED NULL AFTER progression_after_level');
PREPARE progression_threshold_xp_stmt FROM @progression_threshold_xp_sql;
EXECUTE progression_threshold_xp_stmt;
DEALLOCATE PREPARE progression_threshold_xp_stmt;
