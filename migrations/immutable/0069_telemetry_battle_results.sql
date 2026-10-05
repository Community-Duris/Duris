-- Definition-1 typed participant and supported-objective evidence, family 14.
-- Earlier raw families, migrations and reviewed inventories remain sealed.

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_boot_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_boot_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_boundary');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_process_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_process_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_boot_id');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_sequence'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_sequence BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_process_id');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_parent_sequence'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_parent_sequence BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_sequence');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_environment_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_environment_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_parent_sequence');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_season_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_season_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_environment_id');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_config_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_config_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_season_id');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_classifier_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_classifier_version INT UNSIGNED NULL DEFAULT NULL AFTER bout_config_id');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_policy_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_policy_version INT UNSIGNED NULL DEFAULT NULL AFTER bout_classifier_version');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_scope_zone_vnum'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_scope_zone_vnum INT NULL DEFAULT NULL AFTER bout_policy_version');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_scope_group_key'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_scope_group_key BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_scope_zone_vnum');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_build_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_build_version INT UNSIGNED NULL DEFAULT NULL AFTER bout_scope_group_key');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_content_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_content_version INT UNSIGNED NULL DEFAULT NULL AFTER bout_build_version');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_actor_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_actor_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_content_version');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_actor_pid'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_actor_pid INT NULL DEFAULT NULL AFTER bout_source_actor_id');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_owner_subject_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_owner_subject_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_source_actor_pid');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_actor_kind'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_actor_kind TINYINT UNSIGNED NULL DEFAULT NULL AFTER bout_source_owner_subject_id');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_power_band'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_power_band SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bout_source_actor_kind');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_session_boot_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_session_boot_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_source_power_band');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_session_process_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_session_process_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_source_session_boot_id');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_session_seq'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_session_seq BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_source_session_process_id');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_level_band'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_level_band SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bout_source_session_seq');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_class_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_class_id SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bout_source_level_band');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_race_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_race_id SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bout_source_class_id');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_faction_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_faction_id SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bout_source_race_id');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_zone_vnum'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_zone_vnum INT NULL DEFAULT NULL AFTER bout_source_faction_id');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_group_size'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_group_size INT UNSIGNED NULL DEFAULT NULL AFTER bout_source_zone_vnum');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_group_key'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_group_key BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_source_group_size');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_group_revision'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_group_revision SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bout_source_group_key');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_context_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_context_version SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bout_source_group_revision');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_quality_flags'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_quality_flags INT UNSIGNED NULL DEFAULT NULL AFTER bout_source_context_version');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_battle_seq'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_battle_seq BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_source_quality_flags');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_association_revision'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_association_revision BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_source_battle_seq');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_association_fact_sequence'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_association_fact_sequence INT UNSIGNED NULL DEFAULT NULL AFTER bout_source_association_revision');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_target_actor_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_target_actor_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_source_association_fact_sequence');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_target_actor_pid'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_target_actor_pid INT NULL DEFAULT NULL AFTER bout_target_actor_id');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_target_owner_subject_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_target_owner_subject_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_target_actor_pid');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_target_actor_kind'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_target_actor_kind TINYINT UNSIGNED NULL DEFAULT NULL AFTER bout_target_owner_subject_id');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_target_power_band'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_target_power_band SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bout_target_actor_kind');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_target_session_boot_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_target_session_boot_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_target_power_band');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_target_session_process_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_target_session_process_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_target_session_boot_id');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_target_session_seq'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_target_session_seq BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_target_session_process_id');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_target_level_band'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_target_level_band SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bout_target_session_seq');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_target_class_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_target_class_id SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bout_target_level_band');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_target_race_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_target_race_id SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bout_target_class_id');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_target_faction_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_target_faction_id SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bout_target_race_id');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_target_zone_vnum'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_target_zone_vnum INT NULL DEFAULT NULL AFTER bout_target_faction_id');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_target_group_size'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_target_group_size INT UNSIGNED NULL DEFAULT NULL AFTER bout_target_zone_vnum');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_target_group_key'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_target_group_key BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_target_group_size');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_target_group_revision'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_target_group_revision SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bout_target_group_key');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_target_context_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_target_context_version SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bout_target_group_revision');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_target_quality_flags'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_target_quality_flags INT UNSIGNED NULL DEFAULT NULL AFTER bout_target_context_version');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_target_battle_seq'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_target_battle_seq BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_target_quality_flags');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_target_association_revision'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_target_association_revision BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_target_battle_seq');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_target_association_fact_sequence'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_target_association_fact_sequence INT UNSIGNED NULL DEFAULT NULL AFTER bout_target_association_revision');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_start_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_start_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_target_association_fact_sequence');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_start_utc_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_start_utc_usec BIGINT NULL DEFAULT NULL AFTER bout_start_usec');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_at_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_at_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_start_utc_usec');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_at_utc_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_at_utc_usec BIGINT NULL DEFAULT NULL AFTER bout_at_usec');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_operation_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_operation_id BINARY(16) NULL DEFAULT NULL AFTER bout_at_utc_usec');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_object_uid'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_object_uid BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_operation_id');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_proof_window_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_proof_window_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER bout_source_object_uid');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_credited_zone_vnum'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_credited_zone_vnum INT NULL DEFAULT NULL AFTER bout_proof_window_usec');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_from_room_vnum'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_from_room_vnum INT NULL DEFAULT NULL AFTER bout_credited_zone_vnum');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_to_room_vnum'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_to_room_vnum INT NULL DEFAULT NULL AFTER bout_from_room_vnum');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_participant_count'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_participant_count SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bout_to_room_vnum');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_source_payload_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_source_payload_version SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bout_participant_count');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_definition_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_definition_version SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bout_source_payload_version');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_producer_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_producer_version SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bout_definition_version');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_flags'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_flags SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bout_producer_version');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_kind'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_kind TINYINT UNSIGNED NULL DEFAULT NULL AFTER bout_flags');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_authority'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_authority TINYINT UNSIGNED NULL DEFAULT NULL AFTER bout_kind');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_reason'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_reason TINYINT UNSIGNED NULL DEFAULT NULL AFTER bout_authority');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bout_quality_flags'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bout_quality_flags INT UNSIGNED NULL DEFAULT NULL AFTER bout_reason');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

SET @telemetry_result_sql = IF(EXISTS(SELECT 1 FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND index_name='uq_telemetry_battle_result'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD UNIQUE KEY uq_telemetry_battle_result (bout_boot_id,bout_process_id,bout_sequence)');
PREPARE telemetry_result_statement FROM @telemetry_result_sql;
EXECUTE telemetry_result_statement;
DEALLOCATE PREPARE telemetry_result_statement;

-- Keep validation outside MariaDB table-definition metadata.
DELIMITER //
CREATE TRIGGER IF NOT EXISTS telemetry_battle_result_insert BEFORE INSERT ON telemetry_interval FOR EACH ROW
BEGIN
    IF NOT (((NEW.bout_boot_id IS NULL)+(NEW.bout_process_id IS NULL)+(NEW.bout_sequence IS NULL)+(NEW.bout_parent_sequence IS NULL)+(NEW.bout_environment_id IS NULL)+(NEW.bout_season_id IS NULL)+(NEW.bout_config_id IS NULL)+(NEW.bout_classifier_version IS NULL)+(NEW.bout_policy_version IS NULL)+(NEW.bout_scope_zone_vnum IS NULL)+(NEW.bout_scope_group_key IS NULL)+(NEW.bout_build_version IS NULL)+(NEW.bout_content_version IS NULL)+(NEW.bout_source_actor_id IS NULL)+(NEW.bout_source_actor_pid IS NULL)+(NEW.bout_source_owner_subject_id IS NULL)+(NEW.bout_source_actor_kind IS NULL)+(NEW.bout_source_power_band IS NULL)+(NEW.bout_source_session_boot_id IS NULL)+(NEW.bout_source_session_process_id IS NULL)+(NEW.bout_source_session_seq IS NULL)+(NEW.bout_source_level_band IS NULL)+(NEW.bout_source_class_id IS NULL)+(NEW.bout_source_race_id IS NULL)+(NEW.bout_source_faction_id IS NULL)+(NEW.bout_source_zone_vnum IS NULL)+(NEW.bout_source_group_size IS NULL)+(NEW.bout_source_group_key IS NULL)+(NEW.bout_source_group_revision IS NULL)+(NEW.bout_source_context_version IS NULL)+(NEW.bout_source_quality_flags IS NULL)+(NEW.bout_source_battle_seq IS NULL)+(NEW.bout_source_association_revision IS NULL)+(NEW.bout_source_association_fact_sequence IS NULL)+(NEW.bout_target_actor_id IS NULL)+(NEW.bout_target_actor_pid IS NULL)+(NEW.bout_target_owner_subject_id IS NULL)+(NEW.bout_target_actor_kind IS NULL)+(NEW.bout_target_power_band IS NULL)+(NEW.bout_target_session_boot_id IS NULL)+(NEW.bout_target_session_process_id IS NULL)+(NEW.bout_target_session_seq IS NULL)+(NEW.bout_target_level_band IS NULL)+(NEW.bout_target_class_id IS NULL)+(NEW.bout_target_race_id IS NULL)+(NEW.bout_target_faction_id IS NULL)+(NEW.bout_target_zone_vnum IS NULL)+(NEW.bout_target_group_size IS NULL)+(NEW.bout_target_group_key IS NULL)+(NEW.bout_target_group_revision IS NULL)+(NEW.bout_target_context_version IS NULL)+(NEW.bout_target_quality_flags IS NULL)+(NEW.bout_target_battle_seq IS NULL)+(NEW.bout_target_association_revision IS NULL)+(NEW.bout_target_association_fact_sequence IS NULL)+(NEW.bout_start_usec IS NULL)+(NEW.bout_start_utc_usec IS NULL)+(NEW.bout_at_usec IS NULL)+(NEW.bout_at_utc_usec IS NULL)+(NEW.bout_operation_id IS NULL)+(NEW.bout_source_object_uid IS NULL)+(NEW.bout_proof_window_usec IS NULL)+(NEW.bout_credited_zone_vnum IS NULL)+(NEW.bout_from_room_vnum IS NULL)+(NEW.bout_to_room_vnum IS NULL)+(NEW.bout_participant_count IS NULL)+(NEW.bout_source_payload_version IS NULL)+(NEW.bout_definition_version IS NULL)+(NEW.bout_producer_version IS NULL)+(NEW.bout_flags IS NULL)+(NEW.bout_kind IS NULL)+(NEW.bout_authority IS NULL)+(NEW.bout_reason IS NULL)+(NEW.bout_quality_flags IS NULL))=(NEW.record_kind<>14)*74) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_bout_payload';
    END IF;
    IF NOT (NEW.record_kind<>14 OR (NEW.bout_boot_id=NEW.boot_id AND NEW.bout_process_id=NEW.process_id AND NEW.bout_at_utc_usec=NEW.occurrence_utc_usec AND NEW.bout_boot_id>0 AND NEW.bout_process_id>0 AND NEW.bout_sequence>0 AND NEW.bout_parent_sequence<NEW.bout_sequence AND NEW.bout_environment_id>0 AND NEW.bout_season_id>0 AND NEW.bout_scope_zone_vnum=-1 AND NEW.bout_scope_group_key=0 AND NEW.bout_definition_version=1 AND NEW.bout_producer_version=1 AND NEW.bout_quality_flags<=1023 AND (NEW.bout_quality_flags & (NEW.bout_source_quality_flags | NEW.bout_target_quality_flags))=(NEW.bout_source_quality_flags | NEW.bout_target_quality_flags) AND NEW.bout_flags<=511 AND NEW.bout_start_usec<=NEW.bout_at_usec AND NEW.bout_kind BETWEEN 1 AND 8 AND NEW.bout_authority BETWEEN 1 AND 9 AND NEW.bout_reason<=14 AND NEW.bout_credited_zone_vnum>=-1 AND NEW.bout_from_room_vnum>=-1 AND NEW.bout_to_room_vnum>=-1)) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_bout_binding';
    END IF;
    IF NOT (NEW.record_kind<>14 OR (((NEW.bout_kind IN (7,8) AND NEW.bout_reason=9) AND NEW.bout_config_id=0 AND NEW.bout_classifier_version=0 AND NEW.bout_policy_version=0 AND NEW.bout_build_version=0 AND NEW.bout_content_version=0) OR (NOT (NEW.bout_kind IN (7,8) AND NEW.bout_reason=9) AND NEW.bout_config_id>0 AND NEW.bout_classifier_version>0 AND NEW.bout_policy_version>0 AND NEW.bout_build_version>0 AND NEW.bout_content_version>0))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_bout_config';
    END IF;
    IF NOT (NEW.record_kind<>14 OR ((((NEW.bout_target_actor_kind=1 AND NEW.bout_target_actor_id=NEW.bout_target_actor_pid AND NEW.bout_target_actor_pid BETWEEN 1 AND 2147483647 AND NEW.bout_target_owner_subject_id=NEW.bout_target_actor_id) OR (NEW.bout_target_actor_kind IN (2,3) AND (NEW.bout_target_actor_id & 9223372036854775808)<>0 AND (NEW.bout_target_actor_id & 9223372036854775807)>0 AND NEW.bout_target_actor_pid=-1 AND ((NEW.bout_target_actor_kind=2 AND NEW.bout_target_owner_subject_id BETWEEN 1 AND 2147483647) OR (NEW.bout_target_actor_kind=3 AND NEW.bout_target_owner_subject_id=0)))) AND (((NEW.bout_target_group_key & 9223372036854775808)<>0 AND (NEW.bout_target_group_key & 9223372036854775807)>0 AND NEW.bout_target_group_revision>0) OR ((NEW.bout_target_group_key & 9223372036854775808)=0 AND NEW.bout_target_group_revision=0)) AND ((NEW.bout_target_session_boot_id=0 AND NEW.bout_target_session_process_id=0 AND NEW.bout_target_session_seq=0) OR (NEW.bout_target_actor_kind=1 AND NEW.bout_target_session_boot_id>0 AND NEW.bout_target_session_process_id>0 AND NEW.bout_target_session_seq>0)) AND NEW.bout_target_context_version=1 AND NEW.bout_target_zone_vnum>=-1 AND NEW.bout_target_quality_flags<=1023) AND ((NEW.bout_source_actor_id=0 AND NEW.bout_source_actor_pid=0 AND NEW.bout_source_owner_subject_id=0 AND NEW.bout_source_actor_kind=0 AND NEW.bout_source_power_band=0 AND NEW.bout_source_session_boot_id=0 AND NEW.bout_source_session_process_id=0 AND NEW.bout_source_session_seq=0 AND NEW.bout_source_level_band=0 AND NEW.bout_source_class_id=0 AND NEW.bout_source_race_id=0 AND NEW.bout_source_faction_id=0 AND NEW.bout_source_zone_vnum=0 AND NEW.bout_source_group_size=0 AND NEW.bout_source_group_key=0 AND NEW.bout_source_group_revision=0 AND NEW.bout_source_context_version=0 AND NEW.bout_source_quality_flags=0) OR (((NEW.bout_source_actor_kind=1 AND NEW.bout_source_actor_id=NEW.bout_source_actor_pid AND NEW.bout_source_actor_pid BETWEEN 1 AND 2147483647 AND NEW.bout_source_owner_subject_id=NEW.bout_source_actor_id) OR (NEW.bout_source_actor_kind IN (2,3) AND (NEW.bout_source_actor_id & 9223372036854775808)<>0 AND (NEW.bout_source_actor_id & 9223372036854775807)>0 AND NEW.bout_source_actor_pid=-1 AND ((NEW.bout_source_actor_kind=2 AND NEW.bout_source_owner_subject_id BETWEEN 1 AND 2147483647) OR (NEW.bout_source_actor_kind=3 AND NEW.bout_source_owner_subject_id=0)))) AND (((NEW.bout_source_group_key & 9223372036854775808)<>0 AND (NEW.bout_source_group_key & 9223372036854775807)>0 AND NEW.bout_source_group_revision>0) OR ((NEW.bout_source_group_key & 9223372036854775808)=0 AND NEW.bout_source_group_revision=0)) AND ((NEW.bout_source_session_boot_id=0 AND NEW.bout_source_session_process_id=0 AND NEW.bout_source_session_seq=0) OR (NEW.bout_source_actor_kind=1 AND NEW.bout_source_session_boot_id>0 AND NEW.bout_source_session_process_id>0 AND NEW.bout_source_session_seq>0)) AND NEW.bout_source_context_version=1 AND NEW.bout_source_zone_vnum>=-1 AND NEW.bout_source_quality_flags<=1023)))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_bout_actors';
    END IF;
    IF NOT (NEW.record_kind<>14 OR (((NEW.bout_source_battle_seq=0 AND NEW.bout_source_association_revision=0 AND NEW.bout_source_association_fact_sequence=0) OR (NEW.bout_source_battle_seq>0 AND NEW.bout_source_association_revision>0 AND NEW.bout_source_association_fact_sequence>0)) AND ((NEW.bout_target_battle_seq=0 AND NEW.bout_target_association_revision=0 AND NEW.bout_target_association_fact_sequence=0) OR (NEW.bout_target_battle_seq>0 AND NEW.bout_target_association_revision>0 AND NEW.bout_target_association_fact_sequence>0)) AND (NOT (NEW.bout_source_actor_id=0 AND NEW.bout_source_actor_pid=0 AND NEW.bout_source_owner_subject_id=0 AND NEW.bout_source_actor_kind=0 AND NEW.bout_source_power_band=0 AND NEW.bout_source_session_boot_id=0 AND NEW.bout_source_session_process_id=0 AND NEW.bout_source_session_seq=0 AND NEW.bout_source_level_band=0 AND NEW.bout_source_class_id=0 AND NEW.bout_source_race_id=0 AND NEW.bout_source_faction_id=0 AND NEW.bout_source_zone_vnum=0 AND NEW.bout_source_group_size=0 AND NEW.bout_source_group_key=0 AND NEW.bout_source_group_revision=0 AND NEW.bout_source_context_version=0 AND NEW.bout_source_quality_flags=0) OR (NEW.bout_source_battle_seq=0 AND NEW.bout_source_association_revision=0 AND NEW.bout_source_association_fact_sequence=0)))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_bout_association';
    END IF;
    IF NOT (NEW.record_kind<>14 OR (((NEW.bout_kind IN (5,6) OR (NEW.bout_kind=7 AND NEW.bout_authority BETWEEN 6 AND 8)) AND (NEW.bout_operation_id<>X'00000000000000000000000000000000' AND NEW.bout_credited_zone_vnum>=0 AND NEW.bout_participant_count BETWEEN 1 AND 15 AND (NEW.bout_source_actor_id=0 AND NEW.bout_source_actor_pid=0 AND NEW.bout_source_owner_subject_id=0 AND NEW.bout_source_actor_kind=0 AND NEW.bout_source_power_band=0 AND NEW.bout_source_session_boot_id=0 AND NEW.bout_source_session_process_id=0 AND NEW.bout_source_session_seq=0 AND NEW.bout_source_level_band=0 AND NEW.bout_source_class_id=0 AND NEW.bout_source_race_id=0 AND NEW.bout_source_faction_id=0 AND NEW.bout_source_zone_vnum=0 AND NEW.bout_source_group_size=0 AND NEW.bout_source_group_key=0 AND NEW.bout_source_group_revision=0 AND NEW.bout_source_context_version=0 AND NEW.bout_source_quality_flags=0) AND NEW.bout_parent_sequence=0 AND NEW.bout_target_actor_kind=1 AND NEW.bout_proof_window_usec=0 AND NEW.bout_from_room_vnum=-1 AND NEW.bout_to_room_vnum=-1 AND (NEW.bout_flags & 65086)=0 AND ((NEW.bout_source_payload_version=1 AND NEW.bout_source_object_uid=0 AND (NEW.bout_flags & 128)<>0) OR (NEW.bout_source_payload_version=2 AND NEW.bout_source_object_uid>0)) AND ((NEW.bout_kind=7 AND NEW.bout_reason<>0 AND (NEW.bout_quality_flags & 1)<>0) OR (NEW.bout_reason=0 AND ((NEW.bout_kind=5 AND NEW.bout_authority=6 AND (NEW.bout_flags & 256)=0) OR (NEW.bout_kind=6 AND NEW.bout_authority IN (7,8))))))) OR (NOT (NEW.bout_kind IN (5,6) OR (NEW.bout_kind=7 AND NEW.bout_authority BETWEEN 6 AND 8)) AND (NEW.bout_operation_id=X'00000000000000000000000000000000' AND NEW.bout_source_object_uid=0 AND NEW.bout_participant_count=0 AND NEW.bout_source_payload_version=0 AND NEW.bout_credited_zone_vnum=-1 AND (NEW.bout_flags & 448)=0)))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_bout_objective';
    END IF;
    IF NOT (NEW.record_kind<>14 OR ((NEW.bout_kind IN (5,6) OR (NEW.bout_kind=7 AND NEW.bout_authority BETWEEN 6 AND 8)) OR (NEW.bout_kind IN (7,8) AND NEW.bout_reason<>0 AND (NEW.bout_quality_flags & 1)<>0 AND NEW.bout_proof_window_usec=0 AND (NEW.bout_flags & 56)=0 AND (NEW.bout_kind<>8 OR NEW.bout_parent_sequence>0)) OR (NOT (NEW.bout_kind IN (7,8)) AND (NEW.bout_reason=0 AND ((NEW.bout_kind=1 AND NEW.bout_authority=1 AND NEW.bout_parent_sequence=0 AND NEW.bout_proof_window_usec=0 AND (NEW.bout_flags & 65532)=0 AND NEW.bout_from_room_vnum=-1 AND NEW.bout_to_room_vnum=-1) OR ((NEW.bout_source_actor_id=0 AND NEW.bout_source_actor_pid=0 AND NEW.bout_source_owner_subject_id=0 AND NEW.bout_source_actor_kind=0 AND NEW.bout_source_power_band=0 AND NEW.bout_source_session_boot_id=0 AND NEW.bout_source_session_process_id=0 AND NEW.bout_source_session_seq=0 AND NEW.bout_source_level_band=0 AND NEW.bout_source_class_id=0 AND NEW.bout_source_race_id=0 AND NEW.bout_source_faction_id=0 AND NEW.bout_source_zone_vnum=0 AND NEW.bout_source_group_size=0 AND NEW.bout_source_group_key=0 AND NEW.bout_source_group_revision=0 AND NEW.bout_source_context_version=0 AND NEW.bout_source_quality_flags=0) AND (NEW.bout_source_battle_seq=0 AND NEW.bout_source_association_revision=0 AND NEW.bout_source_association_fact_sequence=0) AND (NEW.bout_flags & 2)=0 AND ((NEW.bout_kind=4 AND NEW.bout_authority=5 AND NEW.bout_parent_sequence>0 AND ((NEW.bout_flags & 4)<>0 AND NEW.bout_from_room_vnum>=0 AND NEW.bout_to_room_vnum>=0 AND NEW.bout_from_room_vnum<>NEW.bout_to_room_vnum) AND NEW.bout_proof_window_usec=30000000 AND NEW.bout_at_usec-NEW.bout_start_usec>=NEW.bout_proof_window_usec AND (NEW.bout_flags & 60)=60 AND NEW.bout_target_actor_kind=1 AND NOT (NEW.bout_target_battle_seq=0 AND NEW.bout_target_association_revision=0 AND NEW.bout_target_association_fact_sequence=0) AND NEW.bout_target_session_boot_id=NEW.bout_boot_id AND NEW.bout_target_session_process_id=NEW.bout_process_id AND NEW.bout_target_session_seq>0) OR (NEW.bout_parent_sequence=0 AND NEW.bout_proof_window_usec=0 AND (NEW.bout_flags & 65530)=0 AND ((NEW.bout_kind=2 AND NEW.bout_authority=2 AND ((NEW.bout_flags & 4)<>0 AND NEW.bout_from_room_vnum>=0 AND NEW.bout_to_room_vnum>=0 AND NEW.bout_from_room_vnum<>NEW.bout_to_room_vnum)) OR (NEW.bout_kind=3 AND ((NEW.bout_authority=3 AND ((NEW.bout_flags & 4)<>0 AND NEW.bout_from_room_vnum>=0 AND NEW.bout_to_room_vnum>=0 AND NEW.bout_from_room_vnum<>NEW.bout_to_room_vnum)) OR (NEW.bout_authority=4 AND (NEW.bout_flags & 4)=0 AND NEW.bout_from_room_vnum>=0 AND NEW.bout_from_room_vnum=NEW.bout_to_room_vnum)))))))))))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_bout_evidence';
    END IF;
    IF NOT (NEW.record_kind<>14 OR ((NEW.bout_quality_flags & 128)<>0 OR CAST(NEW.bout_start_utc_usec AS UNSIGNED)=9223372036854775808 OR CAST(NEW.bout_at_utc_usec AS UNSIGNED)=9223372036854775808 OR NEW.bout_start_utc_usec<=NEW.bout_at_utc_usec)) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_bout_utc';
    END IF;
    IF NOT (NEW.record_kind<>14 OR (COALESCE(NEW.environment_id,NEW.season_id,NEW.session_boot_id,NEW.session_process_id,NEW.session_seq,NEW.subject_id,NEW.pid,NEW.connection_boot_id,NEW.connection_process_id,NEW.connection_seq,NEW.start_monotonic_usec,NEW.end_monotonic_usec,NEW.start_utc_usec,NEW.end_utc_usec,NEW.duration_usec,NEW.category,NEW.context,NEW.context_quality,NEW.level_band,NEW.class_id,NEW.race_id,NEW.faction_id,NEW.zone_vnum,NEW.group_size,NEW.config_id,NEW.classifier_version,NEW.policy_version,NEW.quality_flags,NEW.lifecycle,NEW.end_reason,NEW.at_monotonic_usec,NEW.at_utc_usec,NEW.checkpoint_revision,NEW.connected_usec,NEW.active_usec,NEW.idle_usec,NEW.unknown_usec,NEW.resident_usec,NEW.linkdead_usec,NEW.gap_reason,NEW.first_missing_record_seq,NEW.last_missing_record_seq,NEW.dropped_records,NEW.config_revision,NEW.build_version,NEW.content_version,NEW.property_version,NEW.fingerprint,NEW.effective_utc_usec,NEW.interval_usec,NEW.checkpoint_interval_usec,NEW.active_window_usec,NEW.context_segments_per_minute,NEW.pulse_slot_count,NEW.backend,NEW.enabled,NEW.progression_kind,NEW.progression_source,NEW.progression_reason,NEW.progression_observation_status,NEW.progression_modifier_flags,NEW.progression_requested_xp,NEW.progression_computed_xp,NEW.progression_applied_xp,NEW.progression_before_exp,NEW.progression_after_exp,NEW.progression_before_level,NEW.progression_after_level,NEW.progression_threshold_xp,NEW.encounter_boot_id,NEW.encounter_process_id,NEW.encounter_seq,NEW.encounter_event,NEW.encounter_mode,NEW.encounter_outcome,NEW.encounter_revision,NEW.encounter_environment_id,NEW.encounter_season_id,NEW.encounter_config_id,NEW.encounter_classifier_version,NEW.encounter_policy_version,NEW.encounter_zone_vnum,NEW.encounter_group_key,NEW.encounter_participant_subject_id,NEW.encounter_participant_pid,NEW.encounter_start_monotonic_usec,NEW.encounter_start_utc_usec,NEW.elapsed_usec,NEW.participant_usec,NEW.participant_count,NEW.expected_credit_count,NEW.encounter_quality_flags,NEW.combat_encounter_boot_id,NEW.combat_encounter_process_id,NEW.combat_encounter_seq,NEW.combat_mode,NEW.combat_outcome,NEW.combat_revision,NEW.combat_environment_id,NEW.combat_season_id,NEW.combat_config_id,NEW.combat_classifier_version,NEW.combat_policy_version,NEW.combat_zone_vnum,NEW.combat_group_key,NEW.combat_actor_id,NEW.combat_actor_pid,NEW.combat_owner_subject_id,NEW.combat_actor_kind,NEW.combat_unique_player_count,NEW.combat_participant_count,NEW.combat_dropped_participant_count,NEW.combat_power_band,NEW.combat_opponent_power_band,NEW.combat_opponent_count,NEW.combat_modifier_flags,NEW.combat_start_monotonic_usec,NEW.combat_end_monotonic_usec,NEW.combat_start_utc_usec,NEW.combat_end_utc_usec,NEW.combat_damage_dealt,NEW.combat_damage_taken,NEW.combat_healing_attempted,NEW.combat_effective_healing,NEW.combat_overhealing,NEW.combat_control_applications,NEW.combat_casting_attempts,NEW.combat_casting_completions,NEW.combat_casting_aborts,NEW.combat_casting_elapsed_usec,NEW.combat_tanking_usec,NEW.combat_quality_flags,NEW.ownership_account_token,NEW.ownership_source,NEW.battle_boot_id,NEW.battle_process_id,NEW.battle_seq,NEW.battle_related_boot_id,NEW.battle_related_process_id,NEW.battle_related_seq,NEW.battle_environment_id,NEW.battle_season_id,NEW.battle_config_id,NEW.battle_classifier_version,NEW.battle_policy_version,NEW.battle_scope_zone_vnum,NEW.battle_scope_group_key,NEW.battle_revision,NEW.battle_fact_sequence,NEW.battle_fact_index,NEW.battle_fact_count,NEW.battle_definition_version,NEW.battle_fact_kind,NEW.battle_relation,NEW.battle_side_status,NEW.battle_mode,NEW.battle_close_reason,NEW.battle_actor_active,NEW.battle_actor_side,NEW.battle_end_censored,NEW.battle_actor_roles,NEW.battle_actor_count,NEW.battle_active_actor_count,NEW.battle_observed_owner_count,NEW.battle_dropped_actor_count,NEW.battle_actor_id,NEW.battle_actor_pid,NEW.battle_actor_owner_subject_id,NEW.battle_actor_kind,NEW.battle_actor_power_band,NEW.battle_actor_encounter_boot_id,NEW.battle_actor_encounter_process_id,NEW.battle_actor_encounter_seq,NEW.battle_actor_session_boot_id,NEW.battle_actor_session_process_id,NEW.battle_actor_session_seq,NEW.battle_actor_level_band,NEW.battle_actor_class_id,NEW.battle_actor_race_id,NEW.battle_actor_faction_id,NEW.battle_actor_zone_vnum,NEW.battle_actor_group_size,NEW.battle_actor_group_key,NEW.battle_actor_group_revision,NEW.battle_actor_context_version,NEW.battle_actor_quality_flags,NEW.battle_related_actor_id,NEW.battle_related_actor_kind,NEW.battle_present_usec,NEW.battle_contributor_usec,NEW.battle_pve_usec,NEW.battle_pvp_usec,NEW.battle_mixed_usec,NEW.battle_unknown_mode_usec,NEW.battle_outnumbered_owner_usec,NEW.battle_unknown_side_usec,NEW.battle_start_monotonic_usec,NEW.battle_at_monotonic_usec,NEW.battle_observed_through_monotonic_usec,NEW.battle_last_engagement_monotonic_usec,NEW.battle_inactivity_grace_usec,NEW.battle_at_utc_usec,NEW.battle_observed_through_utc_usec,NEW.battle_quality_flags,NEW.bc_battle_boot_id,NEW.bc_battle_process_id,NEW.bc_battle_seq,NEW.bc_environment_id,NEW.bc_season_id,NEW.bc_config_id,NEW.bc_classifier_version,NEW.bc_policy_version,NEW.bc_scope_zone_vnum,NEW.bc_scope_group_key,NEW.bc_actor_id,NEW.bc_actor_pid,NEW.bc_actor_owner_subject_id,NEW.bc_actor_kind,NEW.bc_actor_power_band,NEW.bc_actor_encounter_boot_id,NEW.bc_actor_encounter_process_id,NEW.bc_actor_encounter_seq,NEW.bc_actor_session_boot_id,NEW.bc_actor_session_process_id,NEW.bc_actor_session_seq,NEW.bc_actor_level_band,NEW.bc_actor_class_id,NEW.bc_actor_race_id,NEW.bc_actor_faction_id,NEW.bc_actor_zone_vnum,NEW.bc_actor_group_size,NEW.bc_actor_group_key,NEW.bc_actor_group_revision,NEW.bc_actor_context_version,NEW.bc_actor_quality_flags,NEW.bc_first_association_revision,NEW.bc_first_association_fact_sequence,NEW.bc_available_metrics,NEW.bc_side_status,NEW.bc_mode,NEW.bc_actor_side,NEW.bc_context_quality_flags,NEW.bc_segment_seq,NEW.bc_last_association_revision,NEW.bc_last_association_fact_sequence,NEW.bc_modifier_flags,NEW.bc_start_monotonic_usec,NEW.bc_start_utc_usec,NEW.bc_observed_through_monotonic_usec,NEW.bc_observed_through_utc_usec,NEW.bc_decision_monotonic_usec,NEW.bc_decision_utc_usec,NEW.bc_damage_dealt,NEW.bc_damage_taken,NEW.bc_healing_attempted,NEW.bc_effective_healing,NEW.bc_overhealing,NEW.bc_healing_received,NEW.bc_control_applications,NEW.bc_control_received,NEW.bc_casting_attempts,NEW.bc_casting_completions,NEW.bc_casting_aborts,NEW.bc_casting_unresolved,NEW.bc_casting_elapsed_usec,NEW.bc_engaged_target_usec,NEW.bc_quality_flags,NEW.bc_definition_version,NEW.bc_end_reason,NEW.bctx_battle_boot_id,NEW.bctx_battle_process_id,NEW.bctx_battle_seq,NEW.bctx_environment_id,NEW.bctx_season_id,NEW.bctx_config_id,NEW.bctx_actor_id,NEW.bctx_sequence,NEW.bctx_association_revision,NEW.bctx_at_monotonic_usec,NEW.bctx_at_utc_usec,NEW.bctx_association_fact_sequence,NEW.bctx_definition_version,NEW.bctx_native_context_version,NEW.bctx_boundary,NEW.bctx_status,NEW.bctx_actor_kind,NEW.bctx_build_version,NEW.bctx_content_version,NEW.bctx_available,NEW.bctx_context_quality,NEW.bctx_quality_flags,NEW.bctx_primary_class_mask,NEW.bctx_secondary_class_mask,NEW.bctx_level,NEW.bctx_race,NEW.bctx_faction,NEW.bctx_specialization,NEW.bctx_base_str,NEW.bctx_base_dex,NEW.bctx_base_agi,NEW.bctx_base_con,NEW.bctx_base_pow,NEW.bctx_base_int,NEW.bctx_base_wis,NEW.bctx_base_cha,NEW.bctx_base_kar,NEW.bctx_base_luk,NEW.bctx_effective_str,NEW.bctx_effective_dex,NEW.bctx_effective_agi,NEW.bctx_effective_con,NEW.bctx_effective_pow,NEW.bctx_effective_int,NEW.bctx_effective_wis,NEW.bctx_effective_cha,NEW.bctx_effective_kar,NEW.bctx_effective_luk,NEW.bctx_base_hit,NEW.bctx_base_mana,NEW.bctx_base_vitality,NEW.bctx_base_ward,NEW.bctx_effective_hit,NEW.bctx_effective_mana,NEW.bctx_effective_vitality,NEW.bctx_effective_ward,NEW.bctx_current_hit,NEW.bctx_current_mana,NEW.bctx_current_vitality,NEW.bctx_current_ward,NEW.bctx_base_armor,NEW.bctx_base_hitroll,NEW.bctx_base_damroll,NEW.bctx_effective_armor,NEW.bctx_effective_hitroll,NEW.bctx_effective_damroll,NEW.bctx_saving_para,NEW.bctx_saving_rod,NEW.bctx_saving_fear,NEW.bctx_saving_breath,NEW.bctx_saving_spell,NEW.bctx_effective_flags_1,NEW.bctx_effective_flags_2,NEW.bctx_effective_flags_3,NEW.bctx_effective_flags_4,NEW.bctx_effective_flags_5,NEW.bctx_equipment_flags_1,NEW.bctx_equipment_flags_2,NEW.bctx_equipment_flags_3,NEW.bctx_equipment_flags_4,NEW.bctx_equipment_flags_5,NEW.bctx_equipment_hit,NEW.bctx_equipment_mana,NEW.bctx_equipment_armor,NEW.bctx_equipment_hitroll,NEW.bctx_equipment_damroll,NEW.bctx_equipment_occupied_slots_count,NEW.bctx_equipment_melee_weapons_count,NEW.bctx_equipment_ranged_weapons_count,NEW.bctx_equipment_shields_count,NEW.bctx_equipment_armor_count,NEW.bctx_equipment_other_items_count,NEW.bctx_equipment_dynamic_affects_count,NEW.bctx_equipment_digest,NEW.bctx_epic_catalog_skills,NEW.bctx_epic_learned_skills,NEW.bctx_epic_digest,NEW.bctx_affect_nodes,NEW.bctx_offensive_modifier_nodes,NEW.bctx_armor_modifier_nodes,NEW.bctx_resource_modifier_nodes,NEW.bctx_unapplied_nodes,NEW.bctx_affects_complete,NEW.bctx_arena_membership,NEW.bctx_arena_room,NEW.bctx_arena_enabled,NEW.bctx_arena_type,NEW.bctx_arena_stage,NEW.bctx_arena_team,NEW.bctx_arena_player_flags,NEW.ctl_boot_id,NEW.ctl_process_id,NEW.ctl_sequence,NEW.ctl_previous_state_sequence,NEW.ctl_environment_id,NEW.ctl_season_id,NEW.ctl_config_id,NEW.ctl_classifier_version,NEW.ctl_policy_version,NEW.ctl_scope_zone_vnum,NEW.ctl_scope_group_key,NEW.ctl_build_version,NEW.ctl_content_version,NEW.ctl_source_actor_id,NEW.ctl_source_actor_pid,NEW.ctl_source_owner_subject_id,NEW.ctl_source_actor_kind,NEW.ctl_source_power_band,NEW.ctl_source_session_boot_id,NEW.ctl_source_session_process_id,NEW.ctl_source_session_seq,NEW.ctl_source_level_band,NEW.ctl_source_class_id,NEW.ctl_source_race_id,NEW.ctl_source_faction_id,NEW.ctl_source_zone_vnum,NEW.ctl_source_group_size,NEW.ctl_source_group_key,NEW.ctl_source_group_revision,NEW.ctl_source_context_version,NEW.ctl_source_quality_flags,NEW.ctl_source_battle_seq,NEW.ctl_source_association_revision,NEW.ctl_source_association_fact_sequence,NEW.ctl_target_actor_id,NEW.ctl_target_actor_pid,NEW.ctl_target_owner_subject_id,NEW.ctl_target_actor_kind,NEW.ctl_target_power_band,NEW.ctl_target_session_boot_id,NEW.ctl_target_session_process_id,NEW.ctl_target_session_seq,NEW.ctl_target_level_band,NEW.ctl_target_class_id,NEW.ctl_target_race_id,NEW.ctl_target_faction_id,NEW.ctl_target_zone_vnum,NEW.ctl_target_group_size,NEW.ctl_target_group_key,NEW.ctl_target_group_revision,NEW.ctl_target_context_version,NEW.ctl_target_quality_flags,NEW.ctl_target_battle_seq,NEW.ctl_target_association_revision,NEW.ctl_target_association_fact_sequence,NEW.ctl_last_target_association_revision,NEW.ctl_last_target_association_fact_sequence,NEW.ctl_start_usec,NEW.ctl_start_utc_usec,NEW.ctl_at_usec,NEW.ctl_at_utc_usec,NEW.ctl_decision_usec,NEW.ctl_decision_utc_usec,NEW.ctl_quality_flags,NEW.ctl_configured_ticks,NEW.ctl_definition_version,NEW.ctl_producer_version,NEW.ctl_flags,NEW.ctl_before_mask,NEW.ctl_after_mask,NEW.ctl_state_available,NEW.ctl_duration_coverage,NEW.ctl_kind,NEW.ctl_family,NEW.ctl_result,NEW.ctl_boundary) IS NULL)) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_bout_inactive';
    END IF;
END//
CREATE TRIGGER IF NOT EXISTS telemetry_battle_result_update BEFORE UPDATE ON telemetry_interval FOR EACH ROW
BEGIN
    IF NOT (((NEW.bout_boot_id IS NULL)+(NEW.bout_process_id IS NULL)+(NEW.bout_sequence IS NULL)+(NEW.bout_parent_sequence IS NULL)+(NEW.bout_environment_id IS NULL)+(NEW.bout_season_id IS NULL)+(NEW.bout_config_id IS NULL)+(NEW.bout_classifier_version IS NULL)+(NEW.bout_policy_version IS NULL)+(NEW.bout_scope_zone_vnum IS NULL)+(NEW.bout_scope_group_key IS NULL)+(NEW.bout_build_version IS NULL)+(NEW.bout_content_version IS NULL)+(NEW.bout_source_actor_id IS NULL)+(NEW.bout_source_actor_pid IS NULL)+(NEW.bout_source_owner_subject_id IS NULL)+(NEW.bout_source_actor_kind IS NULL)+(NEW.bout_source_power_band IS NULL)+(NEW.bout_source_session_boot_id IS NULL)+(NEW.bout_source_session_process_id IS NULL)+(NEW.bout_source_session_seq IS NULL)+(NEW.bout_source_level_band IS NULL)+(NEW.bout_source_class_id IS NULL)+(NEW.bout_source_race_id IS NULL)+(NEW.bout_source_faction_id IS NULL)+(NEW.bout_source_zone_vnum IS NULL)+(NEW.bout_source_group_size IS NULL)+(NEW.bout_source_group_key IS NULL)+(NEW.bout_source_group_revision IS NULL)+(NEW.bout_source_context_version IS NULL)+(NEW.bout_source_quality_flags IS NULL)+(NEW.bout_source_battle_seq IS NULL)+(NEW.bout_source_association_revision IS NULL)+(NEW.bout_source_association_fact_sequence IS NULL)+(NEW.bout_target_actor_id IS NULL)+(NEW.bout_target_actor_pid IS NULL)+(NEW.bout_target_owner_subject_id IS NULL)+(NEW.bout_target_actor_kind IS NULL)+(NEW.bout_target_power_band IS NULL)+(NEW.bout_target_session_boot_id IS NULL)+(NEW.bout_target_session_process_id IS NULL)+(NEW.bout_target_session_seq IS NULL)+(NEW.bout_target_level_band IS NULL)+(NEW.bout_target_class_id IS NULL)+(NEW.bout_target_race_id IS NULL)+(NEW.bout_target_faction_id IS NULL)+(NEW.bout_target_zone_vnum IS NULL)+(NEW.bout_target_group_size IS NULL)+(NEW.bout_target_group_key IS NULL)+(NEW.bout_target_group_revision IS NULL)+(NEW.bout_target_context_version IS NULL)+(NEW.bout_target_quality_flags IS NULL)+(NEW.bout_target_battle_seq IS NULL)+(NEW.bout_target_association_revision IS NULL)+(NEW.bout_target_association_fact_sequence IS NULL)+(NEW.bout_start_usec IS NULL)+(NEW.bout_start_utc_usec IS NULL)+(NEW.bout_at_usec IS NULL)+(NEW.bout_at_utc_usec IS NULL)+(NEW.bout_operation_id IS NULL)+(NEW.bout_source_object_uid IS NULL)+(NEW.bout_proof_window_usec IS NULL)+(NEW.bout_credited_zone_vnum IS NULL)+(NEW.bout_from_room_vnum IS NULL)+(NEW.bout_to_room_vnum IS NULL)+(NEW.bout_participant_count IS NULL)+(NEW.bout_source_payload_version IS NULL)+(NEW.bout_definition_version IS NULL)+(NEW.bout_producer_version IS NULL)+(NEW.bout_flags IS NULL)+(NEW.bout_kind IS NULL)+(NEW.bout_authority IS NULL)+(NEW.bout_reason IS NULL)+(NEW.bout_quality_flags IS NULL))=(NEW.record_kind<>14)*74) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_bout_payload';
    END IF;
    IF NOT (NEW.record_kind<>14 OR (NEW.bout_boot_id=NEW.boot_id AND NEW.bout_process_id=NEW.process_id AND NEW.bout_at_utc_usec=NEW.occurrence_utc_usec AND NEW.bout_boot_id>0 AND NEW.bout_process_id>0 AND NEW.bout_sequence>0 AND NEW.bout_parent_sequence<NEW.bout_sequence AND NEW.bout_environment_id>0 AND NEW.bout_season_id>0 AND NEW.bout_scope_zone_vnum=-1 AND NEW.bout_scope_group_key=0 AND NEW.bout_definition_version=1 AND NEW.bout_producer_version=1 AND NEW.bout_quality_flags<=1023 AND (NEW.bout_quality_flags & (NEW.bout_source_quality_flags | NEW.bout_target_quality_flags))=(NEW.bout_source_quality_flags | NEW.bout_target_quality_flags) AND NEW.bout_flags<=511 AND NEW.bout_start_usec<=NEW.bout_at_usec AND NEW.bout_kind BETWEEN 1 AND 8 AND NEW.bout_authority BETWEEN 1 AND 9 AND NEW.bout_reason<=14 AND NEW.bout_credited_zone_vnum>=-1 AND NEW.bout_from_room_vnum>=-1 AND NEW.bout_to_room_vnum>=-1)) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_bout_binding';
    END IF;
    IF NOT (NEW.record_kind<>14 OR (((NEW.bout_kind IN (7,8) AND NEW.bout_reason=9) AND NEW.bout_config_id=0 AND NEW.bout_classifier_version=0 AND NEW.bout_policy_version=0 AND NEW.bout_build_version=0 AND NEW.bout_content_version=0) OR (NOT (NEW.bout_kind IN (7,8) AND NEW.bout_reason=9) AND NEW.bout_config_id>0 AND NEW.bout_classifier_version>0 AND NEW.bout_policy_version>0 AND NEW.bout_build_version>0 AND NEW.bout_content_version>0))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_bout_config';
    END IF;
    IF NOT (NEW.record_kind<>14 OR ((((NEW.bout_target_actor_kind=1 AND NEW.bout_target_actor_id=NEW.bout_target_actor_pid AND NEW.bout_target_actor_pid BETWEEN 1 AND 2147483647 AND NEW.bout_target_owner_subject_id=NEW.bout_target_actor_id) OR (NEW.bout_target_actor_kind IN (2,3) AND (NEW.bout_target_actor_id & 9223372036854775808)<>0 AND (NEW.bout_target_actor_id & 9223372036854775807)>0 AND NEW.bout_target_actor_pid=-1 AND ((NEW.bout_target_actor_kind=2 AND NEW.bout_target_owner_subject_id BETWEEN 1 AND 2147483647) OR (NEW.bout_target_actor_kind=3 AND NEW.bout_target_owner_subject_id=0)))) AND (((NEW.bout_target_group_key & 9223372036854775808)<>0 AND (NEW.bout_target_group_key & 9223372036854775807)>0 AND NEW.bout_target_group_revision>0) OR ((NEW.bout_target_group_key & 9223372036854775808)=0 AND NEW.bout_target_group_revision=0)) AND ((NEW.bout_target_session_boot_id=0 AND NEW.bout_target_session_process_id=0 AND NEW.bout_target_session_seq=0) OR (NEW.bout_target_actor_kind=1 AND NEW.bout_target_session_boot_id>0 AND NEW.bout_target_session_process_id>0 AND NEW.bout_target_session_seq>0)) AND NEW.bout_target_context_version=1 AND NEW.bout_target_zone_vnum>=-1 AND NEW.bout_target_quality_flags<=1023) AND ((NEW.bout_source_actor_id=0 AND NEW.bout_source_actor_pid=0 AND NEW.bout_source_owner_subject_id=0 AND NEW.bout_source_actor_kind=0 AND NEW.bout_source_power_band=0 AND NEW.bout_source_session_boot_id=0 AND NEW.bout_source_session_process_id=0 AND NEW.bout_source_session_seq=0 AND NEW.bout_source_level_band=0 AND NEW.bout_source_class_id=0 AND NEW.bout_source_race_id=0 AND NEW.bout_source_faction_id=0 AND NEW.bout_source_zone_vnum=0 AND NEW.bout_source_group_size=0 AND NEW.bout_source_group_key=0 AND NEW.bout_source_group_revision=0 AND NEW.bout_source_context_version=0 AND NEW.bout_source_quality_flags=0) OR (((NEW.bout_source_actor_kind=1 AND NEW.bout_source_actor_id=NEW.bout_source_actor_pid AND NEW.bout_source_actor_pid BETWEEN 1 AND 2147483647 AND NEW.bout_source_owner_subject_id=NEW.bout_source_actor_id) OR (NEW.bout_source_actor_kind IN (2,3) AND (NEW.bout_source_actor_id & 9223372036854775808)<>0 AND (NEW.bout_source_actor_id & 9223372036854775807)>0 AND NEW.bout_source_actor_pid=-1 AND ((NEW.bout_source_actor_kind=2 AND NEW.bout_source_owner_subject_id BETWEEN 1 AND 2147483647) OR (NEW.bout_source_actor_kind=3 AND NEW.bout_source_owner_subject_id=0)))) AND (((NEW.bout_source_group_key & 9223372036854775808)<>0 AND (NEW.bout_source_group_key & 9223372036854775807)>0 AND NEW.bout_source_group_revision>0) OR ((NEW.bout_source_group_key & 9223372036854775808)=0 AND NEW.bout_source_group_revision=0)) AND ((NEW.bout_source_session_boot_id=0 AND NEW.bout_source_session_process_id=0 AND NEW.bout_source_session_seq=0) OR (NEW.bout_source_actor_kind=1 AND NEW.bout_source_session_boot_id>0 AND NEW.bout_source_session_process_id>0 AND NEW.bout_source_session_seq>0)) AND NEW.bout_source_context_version=1 AND NEW.bout_source_zone_vnum>=-1 AND NEW.bout_source_quality_flags<=1023)))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_bout_actors';
    END IF;
    IF NOT (NEW.record_kind<>14 OR (((NEW.bout_source_battle_seq=0 AND NEW.bout_source_association_revision=0 AND NEW.bout_source_association_fact_sequence=0) OR (NEW.bout_source_battle_seq>0 AND NEW.bout_source_association_revision>0 AND NEW.bout_source_association_fact_sequence>0)) AND ((NEW.bout_target_battle_seq=0 AND NEW.bout_target_association_revision=0 AND NEW.bout_target_association_fact_sequence=0) OR (NEW.bout_target_battle_seq>0 AND NEW.bout_target_association_revision>0 AND NEW.bout_target_association_fact_sequence>0)) AND (NOT (NEW.bout_source_actor_id=0 AND NEW.bout_source_actor_pid=0 AND NEW.bout_source_owner_subject_id=0 AND NEW.bout_source_actor_kind=0 AND NEW.bout_source_power_band=0 AND NEW.bout_source_session_boot_id=0 AND NEW.bout_source_session_process_id=0 AND NEW.bout_source_session_seq=0 AND NEW.bout_source_level_band=0 AND NEW.bout_source_class_id=0 AND NEW.bout_source_race_id=0 AND NEW.bout_source_faction_id=0 AND NEW.bout_source_zone_vnum=0 AND NEW.bout_source_group_size=0 AND NEW.bout_source_group_key=0 AND NEW.bout_source_group_revision=0 AND NEW.bout_source_context_version=0 AND NEW.bout_source_quality_flags=0) OR (NEW.bout_source_battle_seq=0 AND NEW.bout_source_association_revision=0 AND NEW.bout_source_association_fact_sequence=0)))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_bout_association';
    END IF;
    IF NOT (NEW.record_kind<>14 OR (((NEW.bout_kind IN (5,6) OR (NEW.bout_kind=7 AND NEW.bout_authority BETWEEN 6 AND 8)) AND (NEW.bout_operation_id<>X'00000000000000000000000000000000' AND NEW.bout_credited_zone_vnum>=0 AND NEW.bout_participant_count BETWEEN 1 AND 15 AND (NEW.bout_source_actor_id=0 AND NEW.bout_source_actor_pid=0 AND NEW.bout_source_owner_subject_id=0 AND NEW.bout_source_actor_kind=0 AND NEW.bout_source_power_band=0 AND NEW.bout_source_session_boot_id=0 AND NEW.bout_source_session_process_id=0 AND NEW.bout_source_session_seq=0 AND NEW.bout_source_level_band=0 AND NEW.bout_source_class_id=0 AND NEW.bout_source_race_id=0 AND NEW.bout_source_faction_id=0 AND NEW.bout_source_zone_vnum=0 AND NEW.bout_source_group_size=0 AND NEW.bout_source_group_key=0 AND NEW.bout_source_group_revision=0 AND NEW.bout_source_context_version=0 AND NEW.bout_source_quality_flags=0) AND NEW.bout_parent_sequence=0 AND NEW.bout_target_actor_kind=1 AND NEW.bout_proof_window_usec=0 AND NEW.bout_from_room_vnum=-1 AND NEW.bout_to_room_vnum=-1 AND (NEW.bout_flags & 65086)=0 AND ((NEW.bout_source_payload_version=1 AND NEW.bout_source_object_uid=0 AND (NEW.bout_flags & 128)<>0) OR (NEW.bout_source_payload_version=2 AND NEW.bout_source_object_uid>0)) AND ((NEW.bout_kind=7 AND NEW.bout_reason<>0 AND (NEW.bout_quality_flags & 1)<>0) OR (NEW.bout_reason=0 AND ((NEW.bout_kind=5 AND NEW.bout_authority=6 AND (NEW.bout_flags & 256)=0) OR (NEW.bout_kind=6 AND NEW.bout_authority IN (7,8))))))) OR (NOT (NEW.bout_kind IN (5,6) OR (NEW.bout_kind=7 AND NEW.bout_authority BETWEEN 6 AND 8)) AND (NEW.bout_operation_id=X'00000000000000000000000000000000' AND NEW.bout_source_object_uid=0 AND NEW.bout_participant_count=0 AND NEW.bout_source_payload_version=0 AND NEW.bout_credited_zone_vnum=-1 AND (NEW.bout_flags & 448)=0)))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_bout_objective';
    END IF;
    IF NOT (NEW.record_kind<>14 OR ((NEW.bout_kind IN (5,6) OR (NEW.bout_kind=7 AND NEW.bout_authority BETWEEN 6 AND 8)) OR (NEW.bout_kind IN (7,8) AND NEW.bout_reason<>0 AND (NEW.bout_quality_flags & 1)<>0 AND NEW.bout_proof_window_usec=0 AND (NEW.bout_flags & 56)=0 AND (NEW.bout_kind<>8 OR NEW.bout_parent_sequence>0)) OR (NOT (NEW.bout_kind IN (7,8)) AND (NEW.bout_reason=0 AND ((NEW.bout_kind=1 AND NEW.bout_authority=1 AND NEW.bout_parent_sequence=0 AND NEW.bout_proof_window_usec=0 AND (NEW.bout_flags & 65532)=0 AND NEW.bout_from_room_vnum=-1 AND NEW.bout_to_room_vnum=-1) OR ((NEW.bout_source_actor_id=0 AND NEW.bout_source_actor_pid=0 AND NEW.bout_source_owner_subject_id=0 AND NEW.bout_source_actor_kind=0 AND NEW.bout_source_power_band=0 AND NEW.bout_source_session_boot_id=0 AND NEW.bout_source_session_process_id=0 AND NEW.bout_source_session_seq=0 AND NEW.bout_source_level_band=0 AND NEW.bout_source_class_id=0 AND NEW.bout_source_race_id=0 AND NEW.bout_source_faction_id=0 AND NEW.bout_source_zone_vnum=0 AND NEW.bout_source_group_size=0 AND NEW.bout_source_group_key=0 AND NEW.bout_source_group_revision=0 AND NEW.bout_source_context_version=0 AND NEW.bout_source_quality_flags=0) AND (NEW.bout_source_battle_seq=0 AND NEW.bout_source_association_revision=0 AND NEW.bout_source_association_fact_sequence=0) AND (NEW.bout_flags & 2)=0 AND ((NEW.bout_kind=4 AND NEW.bout_authority=5 AND NEW.bout_parent_sequence>0 AND ((NEW.bout_flags & 4)<>0 AND NEW.bout_from_room_vnum>=0 AND NEW.bout_to_room_vnum>=0 AND NEW.bout_from_room_vnum<>NEW.bout_to_room_vnum) AND NEW.bout_proof_window_usec=30000000 AND NEW.bout_at_usec-NEW.bout_start_usec>=NEW.bout_proof_window_usec AND (NEW.bout_flags & 60)=60 AND NEW.bout_target_actor_kind=1 AND NOT (NEW.bout_target_battle_seq=0 AND NEW.bout_target_association_revision=0 AND NEW.bout_target_association_fact_sequence=0) AND NEW.bout_target_session_boot_id=NEW.bout_boot_id AND NEW.bout_target_session_process_id=NEW.bout_process_id AND NEW.bout_target_session_seq>0) OR (NEW.bout_parent_sequence=0 AND NEW.bout_proof_window_usec=0 AND (NEW.bout_flags & 65530)=0 AND ((NEW.bout_kind=2 AND NEW.bout_authority=2 AND ((NEW.bout_flags & 4)<>0 AND NEW.bout_from_room_vnum>=0 AND NEW.bout_to_room_vnum>=0 AND NEW.bout_from_room_vnum<>NEW.bout_to_room_vnum)) OR (NEW.bout_kind=3 AND ((NEW.bout_authority=3 AND ((NEW.bout_flags & 4)<>0 AND NEW.bout_from_room_vnum>=0 AND NEW.bout_to_room_vnum>=0 AND NEW.bout_from_room_vnum<>NEW.bout_to_room_vnum)) OR (NEW.bout_authority=4 AND (NEW.bout_flags & 4)=0 AND NEW.bout_from_room_vnum>=0 AND NEW.bout_from_room_vnum=NEW.bout_to_room_vnum)))))))))))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_bout_evidence';
    END IF;
    IF NOT (NEW.record_kind<>14 OR ((NEW.bout_quality_flags & 128)<>0 OR CAST(NEW.bout_start_utc_usec AS UNSIGNED)=9223372036854775808 OR CAST(NEW.bout_at_utc_usec AS UNSIGNED)=9223372036854775808 OR NEW.bout_start_utc_usec<=NEW.bout_at_utc_usec)) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_bout_utc';
    END IF;
    IF NOT (NEW.record_kind<>14 OR (COALESCE(NEW.environment_id,NEW.season_id,NEW.session_boot_id,NEW.session_process_id,NEW.session_seq,NEW.subject_id,NEW.pid,NEW.connection_boot_id,NEW.connection_process_id,NEW.connection_seq,NEW.start_monotonic_usec,NEW.end_monotonic_usec,NEW.start_utc_usec,NEW.end_utc_usec,NEW.duration_usec,NEW.category,NEW.context,NEW.context_quality,NEW.level_band,NEW.class_id,NEW.race_id,NEW.faction_id,NEW.zone_vnum,NEW.group_size,NEW.config_id,NEW.classifier_version,NEW.policy_version,NEW.quality_flags,NEW.lifecycle,NEW.end_reason,NEW.at_monotonic_usec,NEW.at_utc_usec,NEW.checkpoint_revision,NEW.connected_usec,NEW.active_usec,NEW.idle_usec,NEW.unknown_usec,NEW.resident_usec,NEW.linkdead_usec,NEW.gap_reason,NEW.first_missing_record_seq,NEW.last_missing_record_seq,NEW.dropped_records,NEW.config_revision,NEW.build_version,NEW.content_version,NEW.property_version,NEW.fingerprint,NEW.effective_utc_usec,NEW.interval_usec,NEW.checkpoint_interval_usec,NEW.active_window_usec,NEW.context_segments_per_minute,NEW.pulse_slot_count,NEW.backend,NEW.enabled,NEW.progression_kind,NEW.progression_source,NEW.progression_reason,NEW.progression_observation_status,NEW.progression_modifier_flags,NEW.progression_requested_xp,NEW.progression_computed_xp,NEW.progression_applied_xp,NEW.progression_before_exp,NEW.progression_after_exp,NEW.progression_before_level,NEW.progression_after_level,NEW.progression_threshold_xp,NEW.encounter_boot_id,NEW.encounter_process_id,NEW.encounter_seq,NEW.encounter_event,NEW.encounter_mode,NEW.encounter_outcome,NEW.encounter_revision,NEW.encounter_environment_id,NEW.encounter_season_id,NEW.encounter_config_id,NEW.encounter_classifier_version,NEW.encounter_policy_version,NEW.encounter_zone_vnum,NEW.encounter_group_key,NEW.encounter_participant_subject_id,NEW.encounter_participant_pid,NEW.encounter_start_monotonic_usec,NEW.encounter_start_utc_usec,NEW.elapsed_usec,NEW.participant_usec,NEW.participant_count,NEW.expected_credit_count,NEW.encounter_quality_flags,NEW.combat_encounter_boot_id,NEW.combat_encounter_process_id,NEW.combat_encounter_seq,NEW.combat_mode,NEW.combat_outcome,NEW.combat_revision,NEW.combat_environment_id,NEW.combat_season_id,NEW.combat_config_id,NEW.combat_classifier_version,NEW.combat_policy_version,NEW.combat_zone_vnum,NEW.combat_group_key,NEW.combat_actor_id,NEW.combat_actor_pid,NEW.combat_owner_subject_id,NEW.combat_actor_kind,NEW.combat_unique_player_count,NEW.combat_participant_count,NEW.combat_dropped_participant_count,NEW.combat_power_band,NEW.combat_opponent_power_band,NEW.combat_opponent_count,NEW.combat_modifier_flags,NEW.combat_start_monotonic_usec,NEW.combat_end_monotonic_usec,NEW.combat_start_utc_usec,NEW.combat_end_utc_usec,NEW.combat_damage_dealt,NEW.combat_damage_taken,NEW.combat_healing_attempted,NEW.combat_effective_healing,NEW.combat_overhealing,NEW.combat_control_applications,NEW.combat_casting_attempts,NEW.combat_casting_completions,NEW.combat_casting_aborts,NEW.combat_casting_elapsed_usec,NEW.combat_tanking_usec,NEW.combat_quality_flags,NEW.ownership_account_token,NEW.ownership_source,NEW.battle_boot_id,NEW.battle_process_id,NEW.battle_seq,NEW.battle_related_boot_id,NEW.battle_related_process_id,NEW.battle_related_seq,NEW.battle_environment_id,NEW.battle_season_id,NEW.battle_config_id,NEW.battle_classifier_version,NEW.battle_policy_version,NEW.battle_scope_zone_vnum,NEW.battle_scope_group_key,NEW.battle_revision,NEW.battle_fact_sequence,NEW.battle_fact_index,NEW.battle_fact_count,NEW.battle_definition_version,NEW.battle_fact_kind,NEW.battle_relation,NEW.battle_side_status,NEW.battle_mode,NEW.battle_close_reason,NEW.battle_actor_active,NEW.battle_actor_side,NEW.battle_end_censored,NEW.battle_actor_roles,NEW.battle_actor_count,NEW.battle_active_actor_count,NEW.battle_observed_owner_count,NEW.battle_dropped_actor_count,NEW.battle_actor_id,NEW.battle_actor_pid,NEW.battle_actor_owner_subject_id,NEW.battle_actor_kind,NEW.battle_actor_power_band,NEW.battle_actor_encounter_boot_id,NEW.battle_actor_encounter_process_id,NEW.battle_actor_encounter_seq,NEW.battle_actor_session_boot_id,NEW.battle_actor_session_process_id,NEW.battle_actor_session_seq,NEW.battle_actor_level_band,NEW.battle_actor_class_id,NEW.battle_actor_race_id,NEW.battle_actor_faction_id,NEW.battle_actor_zone_vnum,NEW.battle_actor_group_size,NEW.battle_actor_group_key,NEW.battle_actor_group_revision,NEW.battle_actor_context_version,NEW.battle_actor_quality_flags,NEW.battle_related_actor_id,NEW.battle_related_actor_kind,NEW.battle_present_usec,NEW.battle_contributor_usec,NEW.battle_pve_usec,NEW.battle_pvp_usec,NEW.battle_mixed_usec,NEW.battle_unknown_mode_usec,NEW.battle_outnumbered_owner_usec,NEW.battle_unknown_side_usec,NEW.battle_start_monotonic_usec,NEW.battle_at_monotonic_usec,NEW.battle_observed_through_monotonic_usec,NEW.battle_last_engagement_monotonic_usec,NEW.battle_inactivity_grace_usec,NEW.battle_at_utc_usec,NEW.battle_observed_through_utc_usec,NEW.battle_quality_flags,NEW.bc_battle_boot_id,NEW.bc_battle_process_id,NEW.bc_battle_seq,NEW.bc_environment_id,NEW.bc_season_id,NEW.bc_config_id,NEW.bc_classifier_version,NEW.bc_policy_version,NEW.bc_scope_zone_vnum,NEW.bc_scope_group_key,NEW.bc_actor_id,NEW.bc_actor_pid,NEW.bc_actor_owner_subject_id,NEW.bc_actor_kind,NEW.bc_actor_power_band,NEW.bc_actor_encounter_boot_id,NEW.bc_actor_encounter_process_id,NEW.bc_actor_encounter_seq,NEW.bc_actor_session_boot_id,NEW.bc_actor_session_process_id,NEW.bc_actor_session_seq,NEW.bc_actor_level_band,NEW.bc_actor_class_id,NEW.bc_actor_race_id,NEW.bc_actor_faction_id,NEW.bc_actor_zone_vnum,NEW.bc_actor_group_size,NEW.bc_actor_group_key,NEW.bc_actor_group_revision,NEW.bc_actor_context_version,NEW.bc_actor_quality_flags,NEW.bc_first_association_revision,NEW.bc_first_association_fact_sequence,NEW.bc_available_metrics,NEW.bc_side_status,NEW.bc_mode,NEW.bc_actor_side,NEW.bc_context_quality_flags,NEW.bc_segment_seq,NEW.bc_last_association_revision,NEW.bc_last_association_fact_sequence,NEW.bc_modifier_flags,NEW.bc_start_monotonic_usec,NEW.bc_start_utc_usec,NEW.bc_observed_through_monotonic_usec,NEW.bc_observed_through_utc_usec,NEW.bc_decision_monotonic_usec,NEW.bc_decision_utc_usec,NEW.bc_damage_dealt,NEW.bc_damage_taken,NEW.bc_healing_attempted,NEW.bc_effective_healing,NEW.bc_overhealing,NEW.bc_healing_received,NEW.bc_control_applications,NEW.bc_control_received,NEW.bc_casting_attempts,NEW.bc_casting_completions,NEW.bc_casting_aborts,NEW.bc_casting_unresolved,NEW.bc_casting_elapsed_usec,NEW.bc_engaged_target_usec,NEW.bc_quality_flags,NEW.bc_definition_version,NEW.bc_end_reason,NEW.bctx_battle_boot_id,NEW.bctx_battle_process_id,NEW.bctx_battle_seq,NEW.bctx_environment_id,NEW.bctx_season_id,NEW.bctx_config_id,NEW.bctx_actor_id,NEW.bctx_sequence,NEW.bctx_association_revision,NEW.bctx_at_monotonic_usec,NEW.bctx_at_utc_usec,NEW.bctx_association_fact_sequence,NEW.bctx_definition_version,NEW.bctx_native_context_version,NEW.bctx_boundary,NEW.bctx_status,NEW.bctx_actor_kind,NEW.bctx_build_version,NEW.bctx_content_version,NEW.bctx_available,NEW.bctx_context_quality,NEW.bctx_quality_flags,NEW.bctx_primary_class_mask,NEW.bctx_secondary_class_mask,NEW.bctx_level,NEW.bctx_race,NEW.bctx_faction,NEW.bctx_specialization,NEW.bctx_base_str,NEW.bctx_base_dex,NEW.bctx_base_agi,NEW.bctx_base_con,NEW.bctx_base_pow,NEW.bctx_base_int,NEW.bctx_base_wis,NEW.bctx_base_cha,NEW.bctx_base_kar,NEW.bctx_base_luk,NEW.bctx_effective_str,NEW.bctx_effective_dex,NEW.bctx_effective_agi,NEW.bctx_effective_con,NEW.bctx_effective_pow,NEW.bctx_effective_int,NEW.bctx_effective_wis,NEW.bctx_effective_cha,NEW.bctx_effective_kar,NEW.bctx_effective_luk,NEW.bctx_base_hit,NEW.bctx_base_mana,NEW.bctx_base_vitality,NEW.bctx_base_ward,NEW.bctx_effective_hit,NEW.bctx_effective_mana,NEW.bctx_effective_vitality,NEW.bctx_effective_ward,NEW.bctx_current_hit,NEW.bctx_current_mana,NEW.bctx_current_vitality,NEW.bctx_current_ward,NEW.bctx_base_armor,NEW.bctx_base_hitroll,NEW.bctx_base_damroll,NEW.bctx_effective_armor,NEW.bctx_effective_hitroll,NEW.bctx_effective_damroll,NEW.bctx_saving_para,NEW.bctx_saving_rod,NEW.bctx_saving_fear,NEW.bctx_saving_breath,NEW.bctx_saving_spell,NEW.bctx_effective_flags_1,NEW.bctx_effective_flags_2,NEW.bctx_effective_flags_3,NEW.bctx_effective_flags_4,NEW.bctx_effective_flags_5,NEW.bctx_equipment_flags_1,NEW.bctx_equipment_flags_2,NEW.bctx_equipment_flags_3,NEW.bctx_equipment_flags_4,NEW.bctx_equipment_flags_5,NEW.bctx_equipment_hit,NEW.bctx_equipment_mana,NEW.bctx_equipment_armor,NEW.bctx_equipment_hitroll,NEW.bctx_equipment_damroll,NEW.bctx_equipment_occupied_slots_count,NEW.bctx_equipment_melee_weapons_count,NEW.bctx_equipment_ranged_weapons_count,NEW.bctx_equipment_shields_count,NEW.bctx_equipment_armor_count,NEW.bctx_equipment_other_items_count,NEW.bctx_equipment_dynamic_affects_count,NEW.bctx_equipment_digest,NEW.bctx_epic_catalog_skills,NEW.bctx_epic_learned_skills,NEW.bctx_epic_digest,NEW.bctx_affect_nodes,NEW.bctx_offensive_modifier_nodes,NEW.bctx_armor_modifier_nodes,NEW.bctx_resource_modifier_nodes,NEW.bctx_unapplied_nodes,NEW.bctx_affects_complete,NEW.bctx_arena_membership,NEW.bctx_arena_room,NEW.bctx_arena_enabled,NEW.bctx_arena_type,NEW.bctx_arena_stage,NEW.bctx_arena_team,NEW.bctx_arena_player_flags,NEW.ctl_boot_id,NEW.ctl_process_id,NEW.ctl_sequence,NEW.ctl_previous_state_sequence,NEW.ctl_environment_id,NEW.ctl_season_id,NEW.ctl_config_id,NEW.ctl_classifier_version,NEW.ctl_policy_version,NEW.ctl_scope_zone_vnum,NEW.ctl_scope_group_key,NEW.ctl_build_version,NEW.ctl_content_version,NEW.ctl_source_actor_id,NEW.ctl_source_actor_pid,NEW.ctl_source_owner_subject_id,NEW.ctl_source_actor_kind,NEW.ctl_source_power_band,NEW.ctl_source_session_boot_id,NEW.ctl_source_session_process_id,NEW.ctl_source_session_seq,NEW.ctl_source_level_band,NEW.ctl_source_class_id,NEW.ctl_source_race_id,NEW.ctl_source_faction_id,NEW.ctl_source_zone_vnum,NEW.ctl_source_group_size,NEW.ctl_source_group_key,NEW.ctl_source_group_revision,NEW.ctl_source_context_version,NEW.ctl_source_quality_flags,NEW.ctl_source_battle_seq,NEW.ctl_source_association_revision,NEW.ctl_source_association_fact_sequence,NEW.ctl_target_actor_id,NEW.ctl_target_actor_pid,NEW.ctl_target_owner_subject_id,NEW.ctl_target_actor_kind,NEW.ctl_target_power_band,NEW.ctl_target_session_boot_id,NEW.ctl_target_session_process_id,NEW.ctl_target_session_seq,NEW.ctl_target_level_band,NEW.ctl_target_class_id,NEW.ctl_target_race_id,NEW.ctl_target_faction_id,NEW.ctl_target_zone_vnum,NEW.ctl_target_group_size,NEW.ctl_target_group_key,NEW.ctl_target_group_revision,NEW.ctl_target_context_version,NEW.ctl_target_quality_flags,NEW.ctl_target_battle_seq,NEW.ctl_target_association_revision,NEW.ctl_target_association_fact_sequence,NEW.ctl_last_target_association_revision,NEW.ctl_last_target_association_fact_sequence,NEW.ctl_start_usec,NEW.ctl_start_utc_usec,NEW.ctl_at_usec,NEW.ctl_at_utc_usec,NEW.ctl_decision_usec,NEW.ctl_decision_utc_usec,NEW.ctl_quality_flags,NEW.ctl_configured_ticks,NEW.ctl_definition_version,NEW.ctl_producer_version,NEW.ctl_flags,NEW.ctl_before_mask,NEW.ctl_after_mask,NEW.ctl_state_available,NEW.ctl_duration_coverage,NEW.ctl_kind,NEW.ctl_family,NEW.ctl_result,NEW.ctl_boundary) IS NULL)) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_bout_inactive';
    END IF;
END//
DELIMITER ;

-- Independent reviewed incident schema 7 includes typed-result loss.
CREATE TABLE IF NOT EXISTS telemetry_incident_registry_v7 (
    environment_id BIGINT UNSIGNED NOT NULL,
    season_id BIGINT UNSIGNED NOT NULL,
    registry_version BIGINT UNSIGNED NOT NULL,
    previous_registry_version BIGINT UNSIGNED NOT NULL,
    reviewed_from_utc_usec BIGINT NULL,
    reviewed_through_utc_usec BIGINT NULL,
    reviewer_token BINARY(32) NOT NULL,
    review_evidence_digest BINARY(32) NOT NULL,
    packet_digest BINARY(32) NOT NULL,
    incident_count SMALLINT UNSIGNED NOT NULL,
    PRIMARY KEY (environment_id,season_id,registry_version),
    CONSTRAINT chk_incident_registry_version_v7 CHECK (environment_id <> 0 AND season_id <> 0 AND registry_version <> 0 AND previous_registry_version < registry_version AND registry_version - previous_registry_version = 1),
    CONSTRAINT chk_incident_registry_count_v7 CHECK (incident_count <= 64),
    CONSTRAINT chk_incident_registry_utc_v7 CHECK ((reviewed_from_utc_usec IS NULL OR CAST(reviewed_from_utc_usec AS UNSIGNED) <> 9223372036854775808) AND (reviewed_through_utc_usec IS NULL OR CAST(reviewed_through_utc_usec AS UNSIGNED) <> 9223372036854775808)),
    CONSTRAINT chk_incident_registry_time_v7 CHECK (reviewed_from_utc_usec IS NULL OR reviewed_through_utc_usec IS NULL OR reviewed_from_utc_usec <= reviewed_through_utc_usec)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_incident_v7 (
    environment_id BIGINT UNSIGNED NOT NULL,
    season_id BIGINT UNSIGNED NOT NULL,
    registry_version BIGINT UNSIGNED NOT NULL,
    incident_id BIGINT UNSIGNED NOT NULL,
    producer_boot_id BIGINT UNSIGNED NULL,
    producer_process_id BIGINT UNSIGNED NULL,
    start_utc_usec BIGINT NULL,
    end_utc_usec BIGINT NULL,
    record_kind_mask BIGINT UNSIGNED NOT NULL,
    first_record_seq BIGINT UNSIGNED NULL,
    last_record_seq BIGINT UNSIGNED NULL,
    fix_reference_digest BINARY(32) NULL,
    verified_boot_id BIGINT UNSIGNED NULL,
    verified_process_id BIGINT UNSIGNED NULL,
    verified_record_seq BIGINT UNSIGNED NULL,
    verified_record_kind TINYINT UNSIGNED NULL,
    verified_occurrence_utc_usec BIGINT NULL,
    backlog_disposition TINYINT UNSIGNED NOT NULL,
    observation_provenance TINYINT UNSIGNED NOT NULL,
    evidence_digest BINARY(32) NOT NULL,
    status TINYINT UNSIGNED NOT NULL,
    PRIMARY KEY (environment_id,season_id,registry_version,incident_id),
    CONSTRAINT fk_incident_registry_v7 FOREIGN KEY (environment_id,season_id,registry_version) REFERENCES telemetry_incident_registry_v7 (environment_id,season_id,registry_version) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_incident_identity_v7 CHECK (incident_id <> 0 AND ((producer_boot_id IS NULL AND producer_process_id IS NULL AND first_record_seq IS NULL AND last_record_seq IS NULL) OR (producer_boot_id IS NOT NULL AND producer_process_id IS NOT NULL AND producer_boot_id > 0 AND producer_process_id > 0))),
    CONSTRAINT chk_incident_utc_v7 CHECK ((start_utc_usec IS NULL OR CAST(start_utc_usec AS UNSIGNED) <> 9223372036854775808) AND (end_utc_usec IS NULL OR CAST(end_utc_usec AS UNSIGNED) <> 9223372036854775808) AND (verified_occurrence_utc_usec IS NULL OR CAST(verified_occurrence_utc_usec AS UNSIGNED) <> 9223372036854775808)),
    CONSTRAINT chk_incident_time_v7 CHECK (start_utc_usec IS NULL OR end_utc_usec IS NULL OR start_utc_usec <= end_utc_usec),
    CONSTRAINT chk_incident_sequence_v7 CHECK ((first_record_seq IS NULL OR first_record_seq > 0) AND (last_record_seq IS NULL OR last_record_seq > 0) AND (first_record_seq IS NULL OR last_record_seq IS NULL OR first_record_seq <= last_record_seq)),
    CONSTRAINT chk_incident_family_v7 CHECK (record_kind_mask BETWEEN 2 AND 32766 AND (record_kind_mask & 1) = 0),
    CONSTRAINT chk_incident_verification_v7 CHECK ((verified_boot_id IS NULL AND verified_process_id IS NULL AND verified_record_seq IS NULL AND verified_record_kind IS NULL AND verified_occurrence_utc_usec IS NULL) OR (fix_reference_digest IS NOT NULL AND verified_boot_id IS NOT NULL AND verified_process_id IS NOT NULL AND verified_record_seq IS NOT NULL AND verified_record_kind IS NOT NULL AND verified_boot_id > 0 AND verified_process_id > 0 AND verified_record_seq > 0 AND verified_record_kind BETWEEN 1 AND 14 AND (record_kind_mask & (1 << verified_record_kind)) <> 0)),
    CONSTRAINT chk_incident_disposition_v7 CHECK (backlog_disposition BETWEEN 0 AND 5 AND observation_provenance BETWEEN 1 AND 3 AND status BETWEEN 1 AND 2)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
