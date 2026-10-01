-- Telemetry encounter lifecycle facts share the append-only typed stream.
-- This is additive and rerunnable; no gameplay, balance, or production load
-- is performed by this migration.

SET @encounter_add_boot_id = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='encounter_boot_id') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN encounter_boot_id BIGINT UNSIGNED NULL',
    'SELECT 1 INTO @encounter_add_boot_id_unchanged');
PREPARE encounter_add_boot_id_stmt FROM @encounter_add_boot_id;
EXECUTE encounter_add_boot_id_stmt;
DEALLOCATE PREPARE encounter_add_boot_id_stmt;

SET @encounter_add_process_id = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='encounter_process_id') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN encounter_process_id BIGINT UNSIGNED NULL',
    'SELECT 1 INTO @encounter_add_process_id_unchanged');
PREPARE encounter_add_process_id_stmt FROM @encounter_add_process_id;
EXECUTE encounter_add_process_id_stmt;
DEALLOCATE PREPARE encounter_add_process_id_stmt;

SET @encounter_add_seq = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='encounter_seq') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN encounter_seq BIGINT UNSIGNED NULL',
    'SELECT 1 INTO @encounter_add_seq_unchanged');
PREPARE encounter_add_seq_stmt FROM @encounter_add_seq;
EXECUTE encounter_add_seq_stmt;
DEALLOCATE PREPARE encounter_add_seq_stmt;

SET @encounter_add_event = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='encounter_event') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN encounter_event TINYINT UNSIGNED NULL',
    'SELECT 1 INTO @encounter_add_event_unchanged');
PREPARE encounter_add_event_stmt FROM @encounter_add_event;
EXECUTE encounter_add_event_stmt;
DEALLOCATE PREPARE encounter_add_event_stmt;

SET @encounter_add_mode = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='encounter_mode') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN encounter_mode TINYINT UNSIGNED NULL',
    'SELECT 1 INTO @encounter_add_mode_unchanged');
PREPARE encounter_add_mode_stmt FROM @encounter_add_mode;
EXECUTE encounter_add_mode_stmt;
DEALLOCATE PREPARE encounter_add_mode_stmt;

SET @encounter_add_outcome = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='encounter_outcome') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN encounter_outcome TINYINT UNSIGNED NULL',
    'SELECT 1 INTO @encounter_add_outcome_unchanged');
PREPARE encounter_add_outcome_stmt FROM @encounter_add_outcome;
EXECUTE encounter_add_outcome_stmt;
DEALLOCATE PREPARE encounter_add_outcome_stmt;

SET @encounter_add_revision = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='encounter_revision') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN encounter_revision SMALLINT UNSIGNED NULL',
    'SELECT 1 INTO @encounter_add_revision_unchanged');
PREPARE encounter_add_revision_stmt FROM @encounter_add_revision;
EXECUTE encounter_add_revision_stmt;
DEALLOCATE PREPARE encounter_add_revision_stmt;

SET @encounter_add_environment_id = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='encounter_environment_id') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN encounter_environment_id BIGINT UNSIGNED NULL',
    'SELECT 1 INTO @encounter_add_environment_id_unchanged');
PREPARE encounter_add_environment_id_stmt FROM @encounter_add_environment_id;
EXECUTE encounter_add_environment_id_stmt;
DEALLOCATE PREPARE encounter_add_environment_id_stmt;

SET @encounter_add_season_id = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='encounter_season_id') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN encounter_season_id BIGINT UNSIGNED NULL',
    'SELECT 1 INTO @encounter_add_season_id_unchanged');
PREPARE encounter_add_season_id_stmt FROM @encounter_add_season_id;
EXECUTE encounter_add_season_id_stmt;
DEALLOCATE PREPARE encounter_add_season_id_stmt;

SET @encounter_add_config_id = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='encounter_config_id') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN encounter_config_id BIGINT UNSIGNED NULL',
    'SELECT 1 INTO @encounter_add_config_id_unchanged');
PREPARE encounter_add_config_id_stmt FROM @encounter_add_config_id;
EXECUTE encounter_add_config_id_stmt;
DEALLOCATE PREPARE encounter_add_config_id_stmt;

SET @encounter_add_classifier_version = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='encounter_classifier_version') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN encounter_classifier_version INT UNSIGNED NULL',
    'SELECT 1 INTO @encounter_add_classifier_version_unchanged');
PREPARE encounter_add_classifier_version_stmt FROM @encounter_add_classifier_version;
EXECUTE encounter_add_classifier_version_stmt;
DEALLOCATE PREPARE encounter_add_classifier_version_stmt;

SET @encounter_add_policy_version = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='encounter_policy_version') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN encounter_policy_version INT UNSIGNED NULL',
    'SELECT 1 INTO @encounter_add_policy_version_unchanged');
PREPARE encounter_add_policy_version_stmt FROM @encounter_add_policy_version;
EXECUTE encounter_add_policy_version_stmt;
DEALLOCATE PREPARE encounter_add_policy_version_stmt;

SET @encounter_add_zone_vnum = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='encounter_zone_vnum') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN encounter_zone_vnum INT NULL',
    'SELECT 1 INTO @encounter_add_zone_vnum_unchanged');
PREPARE encounter_add_zone_vnum_stmt FROM @encounter_add_zone_vnum;
EXECUTE encounter_add_zone_vnum_stmt;
DEALLOCATE PREPARE encounter_add_zone_vnum_stmt;

SET @encounter_add_group_key = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='encounter_group_key') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN encounter_group_key BIGINT UNSIGNED NULL',
    'SELECT 1 INTO @encounter_add_group_key_unchanged');
PREPARE encounter_add_group_key_stmt FROM @encounter_add_group_key;
EXECUTE encounter_add_group_key_stmt;
DEALLOCATE PREPARE encounter_add_group_key_stmt;

SET @encounter_add_participant_subject_id = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='encounter_participant_subject_id') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN encounter_participant_subject_id BIGINT UNSIGNED NULL',
    'SELECT 1 INTO @encounter_add_participant_subject_id_unchanged');
PREPARE encounter_add_participant_subject_id_stmt FROM @encounter_add_participant_subject_id;
EXECUTE encounter_add_participant_subject_id_stmt;
DEALLOCATE PREPARE encounter_add_participant_subject_id_stmt;

SET @encounter_add_participant_pid = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='encounter_participant_pid') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN encounter_participant_pid INT NULL',
    'SELECT 1 INTO @encounter_add_participant_pid_unchanged');
PREPARE encounter_add_participant_pid_stmt FROM @encounter_add_participant_pid;
EXECUTE encounter_add_participant_pid_stmt;
DEALLOCATE PREPARE encounter_add_participant_pid_stmt;

SET @encounter_add_at_monotonic_usec = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='at_monotonic_usec') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN at_monotonic_usec BIGINT UNSIGNED NULL',
    'SELECT 1 INTO @encounter_add_at_monotonic_usec_unchanged');
PREPARE encounter_add_at_monotonic_usec_stmt FROM @encounter_add_at_monotonic_usec;
EXECUTE encounter_add_at_monotonic_usec_stmt;
DEALLOCATE PREPARE encounter_add_at_monotonic_usec_stmt;

SET @encounter_add_at_utc_usec = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='at_utc_usec') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN at_utc_usec BIGINT NULL',
    'SELECT 1 INTO @encounter_add_at_utc_usec_unchanged');
PREPARE encounter_add_at_utc_usec_stmt FROM @encounter_add_at_utc_usec;
EXECUTE encounter_add_at_utc_usec_stmt;
DEALLOCATE PREPARE encounter_add_at_utc_usec_stmt;

SET @encounter_add_start_monotonic_usec = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='encounter_start_monotonic_usec') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN encounter_start_monotonic_usec BIGINT UNSIGNED NULL',
    'SELECT 1 INTO @encounter_add_start_monotonic_usec_unchanged');
PREPARE encounter_add_start_monotonic_usec_stmt FROM @encounter_add_start_monotonic_usec;
EXECUTE encounter_add_start_monotonic_usec_stmt;
DEALLOCATE PREPARE encounter_add_start_monotonic_usec_stmt;

SET @encounter_add_start_utc_usec = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='encounter_start_utc_usec') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN encounter_start_utc_usec BIGINT NULL',
    'SELECT 1 INTO @encounter_add_start_utc_usec_unchanged');
PREPARE encounter_add_start_utc_usec_stmt FROM @encounter_add_start_utc_usec;
EXECUTE encounter_add_start_utc_usec_stmt;
DEALLOCATE PREPARE encounter_add_start_utc_usec_stmt;

SET @encounter_add_elapsed_usec = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='elapsed_usec') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN elapsed_usec BIGINT UNSIGNED NULL',
    'SELECT 1 INTO @encounter_add_elapsed_usec_unchanged');
PREPARE encounter_add_elapsed_usec_stmt FROM @encounter_add_elapsed_usec;
EXECUTE encounter_add_elapsed_usec_stmt;
DEALLOCATE PREPARE encounter_add_elapsed_usec_stmt;

SET @encounter_add_participant_usec = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='participant_usec') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN participant_usec BIGINT UNSIGNED NULL',
    'SELECT 1 INTO @encounter_add_participant_usec_unchanged');
PREPARE encounter_add_participant_usec_stmt FROM @encounter_add_participant_usec;
EXECUTE encounter_add_participant_usec_stmt;
DEALLOCATE PREPARE encounter_add_participant_usec_stmt;

SET @encounter_add_participant_count = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='participant_count') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN participant_count SMALLINT UNSIGNED NULL',
    'SELECT 1 INTO @encounter_add_participant_count_unchanged');
PREPARE encounter_add_participant_count_stmt FROM @encounter_add_participant_count;
EXECUTE encounter_add_participant_count_stmt;
DEALLOCATE PREPARE encounter_add_participant_count_stmt;

SET @encounter_add_expected_credit_count = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='expected_credit_count') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN expected_credit_count SMALLINT UNSIGNED NULL',
    'SELECT 1 INTO @encounter_add_expected_credit_count_unchanged');
PREPARE encounter_add_expected_credit_count_stmt FROM @encounter_add_expected_credit_count;
EXECUTE encounter_add_expected_credit_count_stmt;
DEALLOCATE PREPARE encounter_add_expected_credit_count_stmt;

SET @encounter_add_quality_flags = IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND column_name='encounter_quality_flags') = 0,
    'ALTER TABLE telemetry_interval ADD COLUMN encounter_quality_flags INT UNSIGNED NULL',
    'SELECT 1 INTO @encounter_add_quality_flags_unchanged');
PREPARE encounter_add_quality_flags_stmt FROM @encounter_add_quality_flags;
EXECUTE encounter_add_quality_flags_stmt;
DEALLOCATE PREPARE encounter_add_quality_flags_stmt;

SET @encounter_event_index = IF(
    (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE()
     AND table_name='telemetry_interval' AND index_name='uq_telemetry_encounter_event') = 0,
    'ALTER TABLE telemetry_interval ADD UNIQUE KEY uq_telemetry_encounter_event (encounter_boot_id,encounter_process_id,encounter_seq,encounter_event,encounter_revision,encounter_participant_pid)',
    'SELECT 1 INTO @encounter_event_index_unchanged');
PREPARE encounter_event_index_stmt FROM @encounter_event_index;
EXECUTE encounter_event_index_stmt;
DEALLOCATE PREPARE encounter_event_index_stmt;
