-- Definition-1 typed control resolutions and target-state intervals, record family 13.
-- Earlier families, wire meanings and reviewed incident inventories remain sealed.

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_boot_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_boot_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bctx_arena_player_flags');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_process_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_process_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_boot_id');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_sequence'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_sequence BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_process_id');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_previous_state_sequence'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_previous_state_sequence BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_sequence');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_environment_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_environment_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_previous_state_sequence');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_season_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_season_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_environment_id');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_config_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_config_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_season_id');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_classifier_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_classifier_version INT UNSIGNED NULL DEFAULT NULL AFTER ctl_config_id');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_policy_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_policy_version INT UNSIGNED NULL DEFAULT NULL AFTER ctl_classifier_version');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_scope_zone_vnum'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_scope_zone_vnum INT NULL DEFAULT NULL AFTER ctl_policy_version');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_scope_group_key'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_scope_group_key BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_scope_zone_vnum');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_build_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_build_version INT UNSIGNED NULL DEFAULT NULL AFTER ctl_scope_group_key');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_content_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_content_version INT UNSIGNED NULL DEFAULT NULL AFTER ctl_build_version');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_source_actor_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_source_actor_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_content_version');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_source_actor_pid'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_source_actor_pid INT NULL DEFAULT NULL AFTER ctl_source_actor_id');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_source_owner_subject_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_source_owner_subject_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_source_actor_pid');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_source_actor_kind'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_source_actor_kind TINYINT UNSIGNED NULL DEFAULT NULL AFTER ctl_source_owner_subject_id');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_source_power_band'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_source_power_band SMALLINT UNSIGNED NULL DEFAULT NULL AFTER ctl_source_actor_kind');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_source_session_boot_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_source_session_boot_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_source_power_band');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_source_session_process_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_source_session_process_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_source_session_boot_id');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_source_session_seq'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_source_session_seq BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_source_session_process_id');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_source_level_band'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_source_level_band SMALLINT UNSIGNED NULL DEFAULT NULL AFTER ctl_source_session_seq');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_source_class_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_source_class_id SMALLINT UNSIGNED NULL DEFAULT NULL AFTER ctl_source_level_band');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_source_race_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_source_race_id SMALLINT UNSIGNED NULL DEFAULT NULL AFTER ctl_source_class_id');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_source_faction_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_source_faction_id SMALLINT UNSIGNED NULL DEFAULT NULL AFTER ctl_source_race_id');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_source_zone_vnum'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_source_zone_vnum INT NULL DEFAULT NULL AFTER ctl_source_faction_id');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_source_group_size'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_source_group_size INT UNSIGNED NULL DEFAULT NULL AFTER ctl_source_zone_vnum');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_source_group_key'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_source_group_key BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_source_group_size');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_source_group_revision'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_source_group_revision SMALLINT UNSIGNED NULL DEFAULT NULL AFTER ctl_source_group_key');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_source_context_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_source_context_version SMALLINT UNSIGNED NULL DEFAULT NULL AFTER ctl_source_group_revision');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_source_quality_flags'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_source_quality_flags INT UNSIGNED NULL DEFAULT NULL AFTER ctl_source_context_version');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_source_battle_seq'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_source_battle_seq BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_source_quality_flags');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_source_association_revision'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_source_association_revision BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_source_battle_seq');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_source_association_fact_sequence'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_source_association_fact_sequence INT UNSIGNED NULL DEFAULT NULL AFTER ctl_source_association_revision');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_target_actor_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_target_actor_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_source_association_fact_sequence');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_target_actor_pid'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_target_actor_pid INT NULL DEFAULT NULL AFTER ctl_target_actor_id');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_target_owner_subject_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_target_owner_subject_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_target_actor_pid');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_target_actor_kind'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_target_actor_kind TINYINT UNSIGNED NULL DEFAULT NULL AFTER ctl_target_owner_subject_id');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_target_power_band'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_target_power_band SMALLINT UNSIGNED NULL DEFAULT NULL AFTER ctl_target_actor_kind');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_target_session_boot_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_target_session_boot_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_target_power_band');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_target_session_process_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_target_session_process_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_target_session_boot_id');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_target_session_seq'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_target_session_seq BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_target_session_process_id');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_target_level_band'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_target_level_band SMALLINT UNSIGNED NULL DEFAULT NULL AFTER ctl_target_session_seq');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_target_class_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_target_class_id SMALLINT UNSIGNED NULL DEFAULT NULL AFTER ctl_target_level_band');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_target_race_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_target_race_id SMALLINT UNSIGNED NULL DEFAULT NULL AFTER ctl_target_class_id');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_target_faction_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_target_faction_id SMALLINT UNSIGNED NULL DEFAULT NULL AFTER ctl_target_race_id');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_target_zone_vnum'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_target_zone_vnum INT NULL DEFAULT NULL AFTER ctl_target_faction_id');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_target_group_size'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_target_group_size INT UNSIGNED NULL DEFAULT NULL AFTER ctl_target_zone_vnum');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_target_group_key'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_target_group_key BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_target_group_size');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_target_group_revision'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_target_group_revision SMALLINT UNSIGNED NULL DEFAULT NULL AFTER ctl_target_group_key');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_target_context_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_target_context_version SMALLINT UNSIGNED NULL DEFAULT NULL AFTER ctl_target_group_revision');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_target_quality_flags'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_target_quality_flags INT UNSIGNED NULL DEFAULT NULL AFTER ctl_target_context_version');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_target_battle_seq'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_target_battle_seq BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_target_quality_flags');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_target_association_revision'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_target_association_revision BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_target_battle_seq');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_target_association_fact_sequence'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_target_association_fact_sequence INT UNSIGNED NULL DEFAULT NULL AFTER ctl_target_association_revision');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_last_target_association_revision'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_last_target_association_revision BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_target_association_fact_sequence');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_last_target_association_fact_sequence'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_last_target_association_fact_sequence INT UNSIGNED NULL DEFAULT NULL AFTER ctl_last_target_association_revision');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_start_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_start_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_last_target_association_fact_sequence');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_start_utc_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_start_utc_usec BIGINT NULL DEFAULT NULL AFTER ctl_start_usec');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_at_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_at_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_start_utc_usec');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_at_utc_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_at_utc_usec BIGINT NULL DEFAULT NULL AFTER ctl_at_usec');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_decision_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_decision_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER ctl_at_utc_usec');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_decision_utc_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_decision_utc_usec BIGINT NULL DEFAULT NULL AFTER ctl_decision_usec');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_quality_flags'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_quality_flags INT UNSIGNED NULL DEFAULT NULL AFTER ctl_decision_utc_usec');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_configured_ticks'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_configured_ticks INT NULL DEFAULT NULL AFTER ctl_quality_flags');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_definition_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_definition_version SMALLINT UNSIGNED NULL DEFAULT NULL AFTER ctl_configured_ticks');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_producer_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_producer_version SMALLINT UNSIGNED NULL DEFAULT NULL AFTER ctl_definition_version');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_flags'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_flags SMALLINT UNSIGNED NULL DEFAULT NULL AFTER ctl_producer_version');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_before_mask'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_before_mask SMALLINT UNSIGNED NULL DEFAULT NULL AFTER ctl_flags');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_after_mask'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_after_mask SMALLINT UNSIGNED NULL DEFAULT NULL AFTER ctl_before_mask');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_state_available'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_state_available SMALLINT UNSIGNED NULL DEFAULT NULL AFTER ctl_after_mask');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_duration_coverage'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_duration_coverage SMALLINT UNSIGNED NULL DEFAULT NULL AFTER ctl_state_available');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_kind'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_kind TINYINT UNSIGNED NULL DEFAULT NULL AFTER ctl_duration_coverage');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_family'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_family TINYINT UNSIGNED NULL DEFAULT NULL AFTER ctl_kind');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_result'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_result TINYINT UNSIGNED NULL DEFAULT NULL AFTER ctl_family');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='ctl_boundary'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ctl_boundary TINYINT UNSIGNED NULL DEFAULT NULL AFTER ctl_result');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;

SET @telemetry_control_sql = IF(EXISTS(SELECT 1 FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND index_name='uq_telemetry_control'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD UNIQUE KEY uq_telemetry_control (ctl_boot_id,ctl_process_id,ctl_sequence)');
PREPARE telemetry_control_statement FROM @telemetry_control_sql;
EXECUTE telemetry_control_statement;
DEALLOCATE PREPARE telemetry_control_statement;









-- Validation lives outside MariaDB's bounded table-definition metadata.
DELIMITER //
CREATE TRIGGER IF NOT EXISTS telemetry_control_insert BEFORE INSERT ON telemetry_interval FOR EACH ROW
BEGIN
    IF NOT (((NEW.ctl_boot_id IS NULL)+(NEW.ctl_process_id IS NULL)+(NEW.ctl_sequence IS NULL)+(NEW.ctl_previous_state_sequence IS NULL)+(NEW.ctl_environment_id IS NULL)+(NEW.ctl_season_id IS NULL)+(NEW.ctl_config_id IS NULL)+(NEW.ctl_classifier_version IS NULL)+(NEW.ctl_policy_version IS NULL)+(NEW.ctl_scope_zone_vnum IS NULL)+(NEW.ctl_scope_group_key IS NULL)+(NEW.ctl_build_version IS NULL)+(NEW.ctl_content_version IS NULL)+(NEW.ctl_source_actor_id IS NULL)+(NEW.ctl_source_actor_pid IS NULL)+(NEW.ctl_source_owner_subject_id IS NULL)+(NEW.ctl_source_actor_kind IS NULL)+(NEW.ctl_source_power_band IS NULL)+(NEW.ctl_source_session_boot_id IS NULL)+(NEW.ctl_source_session_process_id IS NULL)+(NEW.ctl_source_session_seq IS NULL)+(NEW.ctl_source_level_band IS NULL)+(NEW.ctl_source_class_id IS NULL)+(NEW.ctl_source_race_id IS NULL)+(NEW.ctl_source_faction_id IS NULL)+(NEW.ctl_source_zone_vnum IS NULL)+(NEW.ctl_source_group_size IS NULL)+(NEW.ctl_source_group_key IS NULL)+(NEW.ctl_source_group_revision IS NULL)+(NEW.ctl_source_context_version IS NULL)+(NEW.ctl_source_quality_flags IS NULL)+(NEW.ctl_source_battle_seq IS NULL)+(NEW.ctl_source_association_revision IS NULL)+(NEW.ctl_source_association_fact_sequence IS NULL)+(NEW.ctl_target_actor_id IS NULL)+(NEW.ctl_target_actor_pid IS NULL)+(NEW.ctl_target_owner_subject_id IS NULL)+(NEW.ctl_target_actor_kind IS NULL)+(NEW.ctl_target_power_band IS NULL)+(NEW.ctl_target_session_boot_id IS NULL)+(NEW.ctl_target_session_process_id IS NULL)+(NEW.ctl_target_session_seq IS NULL)+(NEW.ctl_target_level_band IS NULL)+(NEW.ctl_target_class_id IS NULL)+(NEW.ctl_target_race_id IS NULL)+(NEW.ctl_target_faction_id IS NULL)+(NEW.ctl_target_zone_vnum IS NULL)+(NEW.ctl_target_group_size IS NULL)+(NEW.ctl_target_group_key IS NULL)+(NEW.ctl_target_group_revision IS NULL)+(NEW.ctl_target_context_version IS NULL)+(NEW.ctl_target_quality_flags IS NULL)+(NEW.ctl_target_battle_seq IS NULL)+(NEW.ctl_target_association_revision IS NULL)+(NEW.ctl_target_association_fact_sequence IS NULL)+(NEW.ctl_last_target_association_revision IS NULL)+(NEW.ctl_last_target_association_fact_sequence IS NULL)+(NEW.ctl_start_usec IS NULL)+(NEW.ctl_start_utc_usec IS NULL)+(NEW.ctl_at_usec IS NULL)+(NEW.ctl_at_utc_usec IS NULL)+(NEW.ctl_decision_usec IS NULL)+(NEW.ctl_decision_utc_usec IS NULL)+(NEW.ctl_quality_flags IS NULL)+(NEW.ctl_configured_ticks IS NULL)+(NEW.ctl_definition_version IS NULL)+(NEW.ctl_producer_version IS NULL)+(NEW.ctl_flags IS NULL)+(NEW.ctl_before_mask IS NULL)+(NEW.ctl_after_mask IS NULL)+(NEW.ctl_state_available IS NULL)+(NEW.ctl_duration_coverage IS NULL)+(NEW.ctl_kind IS NULL)+(NEW.ctl_family IS NULL)+(NEW.ctl_result IS NULL)+(NEW.ctl_boundary IS NULL))=(NEW.record_kind<>13)*76) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_ctl_payload';
    END IF;
    IF NOT (NEW.record_kind<>13 OR (NEW.ctl_boot_id=NEW.boot_id AND NEW.ctl_process_id=NEW.process_id AND NEW.ctl_decision_utc_usec=NEW.occurrence_utc_usec AND NEW.ctl_boot_id>0 AND NEW.ctl_process_id>0 AND NEW.ctl_sequence>0 AND NEW.ctl_environment_id>0 AND NEW.ctl_season_id>0 AND NEW.ctl_scope_zone_vnum=-1 AND NEW.ctl_scope_group_key=0 AND NEW.ctl_definition_version=1 AND NEW.ctl_producer_version=1 AND NEW.ctl_quality_flags<=1023 AND NEW.ctl_flags<=63 AND NEW.ctl_before_mask<=255 AND NEW.ctl_after_mask<=255 AND NEW.ctl_state_available<=255 AND NEW.ctl_duration_coverage<=255 AND NEW.ctl_start_usec<=NEW.ctl_at_usec AND NEW.ctl_at_usec<=NEW.ctl_decision_usec AND NEW.ctl_previous_state_sequence<NEW.ctl_sequence AND NEW.ctl_boundary BETWEEN 1 AND 11 AND ((NEW.ctl_quality_flags & (NEW.ctl_source_quality_flags | NEW.ctl_target_quality_flags))=(NEW.ctl_source_quality_flags | NEW.ctl_target_quality_flags)) AND ((NEW.ctl_kind=4 AND NEW.ctl_boundary=8 AND NEW.ctl_config_id=0 AND NEW.ctl_classifier_version=0 AND NEW.ctl_policy_version=0 AND NEW.ctl_build_version=0 AND NEW.ctl_content_version=0) OR (NOT (NEW.ctl_kind=4 AND NEW.ctl_boundary=8) AND NEW.ctl_config_id>0 AND NEW.ctl_classifier_version>0 AND NEW.ctl_policy_version>0 AND NEW.ctl_build_version>0 AND NEW.ctl_content_version>0)))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_ctl_binding';
    END IF;
    IF NOT (NEW.record_kind<>13 OR ((((NEW.ctl_target_actor_kind=1 AND NEW.ctl_target_actor_id=NEW.ctl_target_actor_pid AND NEW.ctl_target_actor_pid BETWEEN 1 AND 2147483647 AND NEW.ctl_target_owner_subject_id=NEW.ctl_target_actor_id) OR (NEW.ctl_target_actor_kind IN (2,3) AND (NEW.ctl_target_actor_id & 9223372036854775808)<>0 AND (NEW.ctl_target_actor_id & 9223372036854775807)>0 AND NEW.ctl_target_actor_pid=-1 AND ((NEW.ctl_target_actor_kind=2 AND NEW.ctl_target_owner_subject_id BETWEEN 1 AND 2147483647) OR (NEW.ctl_target_actor_kind=3 AND NEW.ctl_target_owner_subject_id=0)))) AND (((NEW.ctl_target_group_key & 9223372036854775808)<>0 AND (NEW.ctl_target_group_key & 9223372036854775807)>0 AND NEW.ctl_target_group_revision>0) OR ((NEW.ctl_target_group_key & 9223372036854775808)=0 AND NEW.ctl_target_group_revision=0)) AND ((NEW.ctl_target_session_boot_id=0 AND NEW.ctl_target_session_process_id=0 AND NEW.ctl_target_session_seq=0) OR (NEW.ctl_target_actor_kind=1 AND NEW.ctl_target_session_boot_id>0 AND NEW.ctl_target_session_process_id>0 AND NEW.ctl_target_session_seq>0)) AND NEW.ctl_target_context_version=1 AND NEW.ctl_target_zone_vnum>=-1 AND NEW.ctl_target_quality_flags<=1023) AND ((NEW.ctl_kind=1 AND (((NEW.ctl_source_actor_kind=1 AND NEW.ctl_source_actor_id=NEW.ctl_source_actor_pid AND NEW.ctl_source_actor_pid BETWEEN 1 AND 2147483647 AND NEW.ctl_source_owner_subject_id=NEW.ctl_source_actor_id) OR (NEW.ctl_source_actor_kind IN (2,3) AND (NEW.ctl_source_actor_id & 9223372036854775808)<>0 AND (NEW.ctl_source_actor_id & 9223372036854775807)>0 AND NEW.ctl_source_actor_pid=-1 AND ((NEW.ctl_source_actor_kind=2 AND NEW.ctl_source_owner_subject_id BETWEEN 1 AND 2147483647) OR (NEW.ctl_source_actor_kind=3 AND NEW.ctl_source_owner_subject_id=0)))) AND (((NEW.ctl_source_group_key & 9223372036854775808)<>0 AND (NEW.ctl_source_group_key & 9223372036854775807)>0 AND NEW.ctl_source_group_revision>0) OR ((NEW.ctl_source_group_key & 9223372036854775808)=0 AND NEW.ctl_source_group_revision=0)) AND ((NEW.ctl_source_session_boot_id=0 AND NEW.ctl_source_session_process_id=0 AND NEW.ctl_source_session_seq=0) OR (NEW.ctl_source_actor_kind=1 AND NEW.ctl_source_session_boot_id>0 AND NEW.ctl_source_session_process_id>0 AND NEW.ctl_source_session_seq>0)) AND NEW.ctl_source_context_version=1 AND NEW.ctl_source_zone_vnum>=-1 AND NEW.ctl_source_quality_flags<=1023)) OR (NEW.ctl_kind<>1 AND NEW.ctl_source_actor_id=0 AND NEW.ctl_source_actor_pid=0 AND NEW.ctl_source_owner_subject_id=0 AND NEW.ctl_source_actor_kind=0 AND NEW.ctl_source_power_band=0 AND NEW.ctl_source_session_boot_id=0 AND NEW.ctl_source_session_process_id=0 AND NEW.ctl_source_session_seq=0 AND NEW.ctl_source_level_band=0 AND NEW.ctl_source_class_id=0 AND NEW.ctl_source_race_id=0 AND NEW.ctl_source_faction_id=0 AND NEW.ctl_source_zone_vnum=0 AND NEW.ctl_source_group_size=0 AND NEW.ctl_source_group_key=0 AND NEW.ctl_source_group_revision=0 AND NEW.ctl_source_context_version=0 AND NEW.ctl_source_quality_flags=0 AND NEW.ctl_source_battle_seq=0 AND NEW.ctl_source_association_revision=0 AND NEW.ctl_source_association_fact_sequence=0)))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_ctl_actors';
    END IF;
    IF NOT (NEW.record_kind<>13 OR (((NEW.ctl_source_battle_seq=0 AND NEW.ctl_source_association_revision=0 AND NEW.ctl_source_association_fact_sequence=0) OR (NEW.ctl_source_battle_seq>0 AND NEW.ctl_source_association_revision>0 AND NEW.ctl_source_association_fact_sequence>0)) AND ((NEW.ctl_target_battle_seq=0 AND NEW.ctl_target_association_revision=0 AND NEW.ctl_target_association_fact_sequence=0) OR (NEW.ctl_target_battle_seq>0 AND NEW.ctl_target_association_revision>0 AND NEW.ctl_target_association_fact_sequence>0)) AND ((NEW.ctl_target_battle_seq=0 AND NEW.ctl_last_target_association_revision=0 AND NEW.ctl_last_target_association_fact_sequence=0) OR (NEW.ctl_target_battle_seq>0 AND NEW.ctl_last_target_association_revision>=NEW.ctl_target_association_revision AND NEW.ctl_last_target_association_fact_sequence>=NEW.ctl_target_association_fact_sequence AND (NEW.ctl_last_target_association_fact_sequence<>NEW.ctl_target_association_fact_sequence OR NEW.ctl_last_target_association_revision=NEW.ctl_target_association_revision))))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_ctl_association';
    END IF;
    IF NOT (NEW.record_kind<>13 OR NEW.ctl_kind<>1 OR (NEW.ctl_start_usec=NEW.ctl_at_usec AND NEW.ctl_at_usec=NEW.ctl_decision_usec AND NEW.ctl_start_utc_usec=NEW.ctl_at_utc_usec AND NEW.ctl_at_utc_usec=NEW.ctl_decision_utc_usec AND NEW.ctl_last_target_association_revision=NEW.ctl_target_association_revision AND NEW.ctl_last_target_association_fact_sequence=NEW.ctl_target_association_fact_sequence AND NEW.ctl_previous_state_sequence=0 AND NEW.ctl_boundary=11 AND NEW.ctl_family BETWEEN 1 AND 8 AND NEW.ctl_result BETWEEN 1 AND 14 AND NEW.ctl_state_available=255 AND NEW.ctl_duration_coverage=0 AND (((NEW.ctl_flags & 32)<>0 AND NEW.ctl_source_actor_id=NEW.ctl_target_actor_id AND NEW.ctl_source_actor_kind=NEW.ctl_target_actor_kind) OR ((NEW.ctl_flags & 32)=0 AND (NEW.ctl_source_actor_id<>NEW.ctl_target_actor_id OR NEW.ctl_source_actor_kind<>NEW.ctl_target_actor_kind))) AND ((NEW.ctl_result<>1 AND NEW.ctl_configured_ticks=0 AND (NEW.ctl_flags & 28)=0) OR (NEW.ctl_result=1 AND (NEW.ctl_after_mask & CASE WHEN NEW.ctl_family=8 THEN 136 ELSE (1 << (NEW.ctl_family-1)) END)<>0 AND ((NEW.ctl_flags & 8)=0 OR (NEW.ctl_family=6 AND (NEW.ctl_before_mask & 32)<>0)) AND ((NEW.ctl_flags & 16)=0 OR NEW.ctl_family=2) AND ((NEW.ctl_flags & 4)=0 OR NEW.ctl_family IN (2,3,6)))))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_ctl_operation';
    END IF;
    IF NOT (NEW.record_kind<>13 OR (NEW.ctl_kind BETWEEN 1 AND 4 AND (NEW.ctl_kind=1 OR (NEW.ctl_family=0 AND NEW.ctl_result=0 AND NEW.ctl_flags=0 AND NEW.ctl_configured_ticks=0 AND NEW.ctl_boundary<>11 AND ((NEW.ctl_kind=2 AND NEW.ctl_start_usec=NEW.ctl_at_usec AND NEW.ctl_at_usec=NEW.ctl_decision_usec AND NEW.ctl_start_utc_usec=NEW.ctl_at_utc_usec AND NEW.ctl_at_utc_usec=NEW.ctl_decision_utc_usec AND NEW.ctl_last_target_association_revision=NEW.ctl_target_association_revision AND NEW.ctl_last_target_association_fact_sequence=NEW.ctl_target_association_fact_sequence AND NEW.ctl_state_available=255 AND (NEW.ctl_duration_coverage=255 OR (NEW.ctl_quality_flags & 1)<>0) AND NEW.ctl_target_battle_seq>0 AND NEW.ctl_previous_state_sequence=0 AND NEW.ctl_before_mask=NEW.ctl_after_mask AND NEW.ctl_boundary IN (1,4)) OR (NEW.ctl_kind=3 AND NEW.ctl_state_available=255 AND (NEW.ctl_duration_coverage=255 OR (NEW.ctl_quality_flags & 1)<>0) AND NEW.ctl_target_battle_seq>0 AND NEW.ctl_previous_state_sequence>0 AND NEW.ctl_boundary BETWEEN 2 AND 10 AND (NEW.ctl_at_usec=NEW.ctl_decision_usec OR (NEW.ctl_quality_flags & 64)<>0)) OR (NEW.ctl_kind=4 AND NEW.ctl_state_available=0 AND NEW.ctl_duration_coverage=0 AND NEW.ctl_before_mask=0 AND NEW.ctl_after_mask=0 AND (NEW.ctl_quality_flags & 1)<>0 AND NEW.ctl_boundary>=7)))))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_ctl_state';
    END IF;
    IF NOT (NEW.record_kind<>13 OR (NEW.ctl_quality_flags & 128)<>0 OR ((CAST(NEW.ctl_start_utc_usec AS UNSIGNED)=9223372036854775808 OR CAST(NEW.ctl_at_utc_usec AS UNSIGNED)=9223372036854775808 OR NEW.ctl_start_utc_usec<=NEW.ctl_at_utc_usec) AND (CAST(NEW.ctl_at_utc_usec AS UNSIGNED)=9223372036854775808 OR CAST(NEW.ctl_decision_utc_usec AS UNSIGNED)=9223372036854775808 OR NEW.ctl_at_utc_usec<=NEW.ctl_decision_utc_usec))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_ctl_utc';
    END IF;
    IF NOT (NEW.record_kind<>13 OR COALESCE(NEW.environment_id,NEW.season_id,NEW.session_boot_id,NEW.session_process_id,NEW.session_seq,NEW.subject_id,NEW.pid,NEW.connection_boot_id,NEW.connection_process_id,NEW.connection_seq,NEW.start_monotonic_usec,NEW.end_monotonic_usec,NEW.start_utc_usec,NEW.end_utc_usec,NEW.duration_usec,NEW.category,NEW.context,NEW.context_quality,NEW.level_band,NEW.class_id,NEW.race_id,NEW.faction_id,NEW.zone_vnum,NEW.group_size,NEW.config_id,NEW.classifier_version,NEW.policy_version,NEW.quality_flags,NEW.lifecycle,NEW.end_reason,NEW.at_monotonic_usec,NEW.at_utc_usec,NEW.checkpoint_revision,NEW.connected_usec,NEW.active_usec,NEW.idle_usec,NEW.unknown_usec,NEW.resident_usec,NEW.linkdead_usec,NEW.gap_reason,NEW.first_missing_record_seq,NEW.last_missing_record_seq,NEW.dropped_records,NEW.config_revision,NEW.build_version,NEW.content_version,NEW.property_version,NEW.fingerprint,NEW.effective_utc_usec,NEW.interval_usec,NEW.checkpoint_interval_usec,NEW.active_window_usec,NEW.context_segments_per_minute,NEW.pulse_slot_count,NEW.backend,NEW.enabled,NEW.progression_kind,NEW.progression_source,NEW.progression_reason,NEW.progression_observation_status,NEW.progression_modifier_flags,NEW.progression_requested_xp,NEW.progression_computed_xp,NEW.progression_applied_xp,NEW.progression_before_exp,NEW.progression_after_exp,NEW.progression_before_level,NEW.progression_after_level,NEW.progression_threshold_xp,NEW.encounter_boot_id,NEW.encounter_process_id,NEW.encounter_seq,NEW.encounter_event,NEW.encounter_mode,NEW.encounter_outcome,NEW.encounter_revision,NEW.encounter_environment_id,NEW.encounter_season_id,NEW.encounter_config_id,NEW.encounter_classifier_version,NEW.encounter_policy_version,NEW.encounter_zone_vnum,NEW.encounter_group_key,NEW.encounter_participant_subject_id,NEW.encounter_participant_pid,NEW.encounter_start_monotonic_usec,NEW.encounter_start_utc_usec,NEW.elapsed_usec,NEW.participant_usec,NEW.participant_count,NEW.expected_credit_count,NEW.encounter_quality_flags,NEW.combat_encounter_boot_id,NEW.combat_encounter_process_id,NEW.combat_encounter_seq,NEW.combat_mode,NEW.combat_outcome,NEW.combat_revision,NEW.combat_environment_id,NEW.combat_season_id,NEW.combat_config_id,NEW.combat_classifier_version,NEW.combat_policy_version,NEW.combat_zone_vnum,NEW.combat_group_key,NEW.combat_actor_id,NEW.combat_actor_pid,NEW.combat_owner_subject_id,NEW.combat_actor_kind,NEW.combat_unique_player_count,NEW.combat_participant_count,NEW.combat_dropped_participant_count,NEW.combat_power_band,NEW.combat_opponent_power_band,NEW.combat_opponent_count,NEW.combat_modifier_flags,NEW.combat_start_monotonic_usec,NEW.combat_end_monotonic_usec,NEW.combat_start_utc_usec,NEW.combat_end_utc_usec,NEW.combat_damage_dealt,NEW.combat_damage_taken,NEW.combat_healing_attempted,NEW.combat_effective_healing,NEW.combat_overhealing,NEW.combat_control_applications,NEW.combat_casting_attempts,NEW.combat_casting_completions,NEW.combat_casting_aborts,NEW.combat_casting_elapsed_usec,NEW.combat_tanking_usec,NEW.combat_quality_flags) IS NULL) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_ctl_inactive';
    END IF;
END//
CREATE TRIGGER IF NOT EXISTS telemetry_control_update BEFORE UPDATE ON telemetry_interval FOR EACH ROW
BEGIN
    IF NOT (((NEW.ctl_boot_id IS NULL)+(NEW.ctl_process_id IS NULL)+(NEW.ctl_sequence IS NULL)+(NEW.ctl_previous_state_sequence IS NULL)+(NEW.ctl_environment_id IS NULL)+(NEW.ctl_season_id IS NULL)+(NEW.ctl_config_id IS NULL)+(NEW.ctl_classifier_version IS NULL)+(NEW.ctl_policy_version IS NULL)+(NEW.ctl_scope_zone_vnum IS NULL)+(NEW.ctl_scope_group_key IS NULL)+(NEW.ctl_build_version IS NULL)+(NEW.ctl_content_version IS NULL)+(NEW.ctl_source_actor_id IS NULL)+(NEW.ctl_source_actor_pid IS NULL)+(NEW.ctl_source_owner_subject_id IS NULL)+(NEW.ctl_source_actor_kind IS NULL)+(NEW.ctl_source_power_band IS NULL)+(NEW.ctl_source_session_boot_id IS NULL)+(NEW.ctl_source_session_process_id IS NULL)+(NEW.ctl_source_session_seq IS NULL)+(NEW.ctl_source_level_band IS NULL)+(NEW.ctl_source_class_id IS NULL)+(NEW.ctl_source_race_id IS NULL)+(NEW.ctl_source_faction_id IS NULL)+(NEW.ctl_source_zone_vnum IS NULL)+(NEW.ctl_source_group_size IS NULL)+(NEW.ctl_source_group_key IS NULL)+(NEW.ctl_source_group_revision IS NULL)+(NEW.ctl_source_context_version IS NULL)+(NEW.ctl_source_quality_flags IS NULL)+(NEW.ctl_source_battle_seq IS NULL)+(NEW.ctl_source_association_revision IS NULL)+(NEW.ctl_source_association_fact_sequence IS NULL)+(NEW.ctl_target_actor_id IS NULL)+(NEW.ctl_target_actor_pid IS NULL)+(NEW.ctl_target_owner_subject_id IS NULL)+(NEW.ctl_target_actor_kind IS NULL)+(NEW.ctl_target_power_band IS NULL)+(NEW.ctl_target_session_boot_id IS NULL)+(NEW.ctl_target_session_process_id IS NULL)+(NEW.ctl_target_session_seq IS NULL)+(NEW.ctl_target_level_band IS NULL)+(NEW.ctl_target_class_id IS NULL)+(NEW.ctl_target_race_id IS NULL)+(NEW.ctl_target_faction_id IS NULL)+(NEW.ctl_target_zone_vnum IS NULL)+(NEW.ctl_target_group_size IS NULL)+(NEW.ctl_target_group_key IS NULL)+(NEW.ctl_target_group_revision IS NULL)+(NEW.ctl_target_context_version IS NULL)+(NEW.ctl_target_quality_flags IS NULL)+(NEW.ctl_target_battle_seq IS NULL)+(NEW.ctl_target_association_revision IS NULL)+(NEW.ctl_target_association_fact_sequence IS NULL)+(NEW.ctl_last_target_association_revision IS NULL)+(NEW.ctl_last_target_association_fact_sequence IS NULL)+(NEW.ctl_start_usec IS NULL)+(NEW.ctl_start_utc_usec IS NULL)+(NEW.ctl_at_usec IS NULL)+(NEW.ctl_at_utc_usec IS NULL)+(NEW.ctl_decision_usec IS NULL)+(NEW.ctl_decision_utc_usec IS NULL)+(NEW.ctl_quality_flags IS NULL)+(NEW.ctl_configured_ticks IS NULL)+(NEW.ctl_definition_version IS NULL)+(NEW.ctl_producer_version IS NULL)+(NEW.ctl_flags IS NULL)+(NEW.ctl_before_mask IS NULL)+(NEW.ctl_after_mask IS NULL)+(NEW.ctl_state_available IS NULL)+(NEW.ctl_duration_coverage IS NULL)+(NEW.ctl_kind IS NULL)+(NEW.ctl_family IS NULL)+(NEW.ctl_result IS NULL)+(NEW.ctl_boundary IS NULL))=(NEW.record_kind<>13)*76) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_ctl_payload';
    END IF;
    IF NOT (NEW.record_kind<>13 OR (NEW.ctl_boot_id=NEW.boot_id AND NEW.ctl_process_id=NEW.process_id AND NEW.ctl_decision_utc_usec=NEW.occurrence_utc_usec AND NEW.ctl_boot_id>0 AND NEW.ctl_process_id>0 AND NEW.ctl_sequence>0 AND NEW.ctl_environment_id>0 AND NEW.ctl_season_id>0 AND NEW.ctl_scope_zone_vnum=-1 AND NEW.ctl_scope_group_key=0 AND NEW.ctl_definition_version=1 AND NEW.ctl_producer_version=1 AND NEW.ctl_quality_flags<=1023 AND NEW.ctl_flags<=63 AND NEW.ctl_before_mask<=255 AND NEW.ctl_after_mask<=255 AND NEW.ctl_state_available<=255 AND NEW.ctl_duration_coverage<=255 AND NEW.ctl_start_usec<=NEW.ctl_at_usec AND NEW.ctl_at_usec<=NEW.ctl_decision_usec AND NEW.ctl_previous_state_sequence<NEW.ctl_sequence AND NEW.ctl_boundary BETWEEN 1 AND 11 AND ((NEW.ctl_quality_flags & (NEW.ctl_source_quality_flags | NEW.ctl_target_quality_flags))=(NEW.ctl_source_quality_flags | NEW.ctl_target_quality_flags)) AND ((NEW.ctl_kind=4 AND NEW.ctl_boundary=8 AND NEW.ctl_config_id=0 AND NEW.ctl_classifier_version=0 AND NEW.ctl_policy_version=0 AND NEW.ctl_build_version=0 AND NEW.ctl_content_version=0) OR (NOT (NEW.ctl_kind=4 AND NEW.ctl_boundary=8) AND NEW.ctl_config_id>0 AND NEW.ctl_classifier_version>0 AND NEW.ctl_policy_version>0 AND NEW.ctl_build_version>0 AND NEW.ctl_content_version>0)))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_ctl_binding';
    END IF;
    IF NOT (NEW.record_kind<>13 OR ((((NEW.ctl_target_actor_kind=1 AND NEW.ctl_target_actor_id=NEW.ctl_target_actor_pid AND NEW.ctl_target_actor_pid BETWEEN 1 AND 2147483647 AND NEW.ctl_target_owner_subject_id=NEW.ctl_target_actor_id) OR (NEW.ctl_target_actor_kind IN (2,3) AND (NEW.ctl_target_actor_id & 9223372036854775808)<>0 AND (NEW.ctl_target_actor_id & 9223372036854775807)>0 AND NEW.ctl_target_actor_pid=-1 AND ((NEW.ctl_target_actor_kind=2 AND NEW.ctl_target_owner_subject_id BETWEEN 1 AND 2147483647) OR (NEW.ctl_target_actor_kind=3 AND NEW.ctl_target_owner_subject_id=0)))) AND (((NEW.ctl_target_group_key & 9223372036854775808)<>0 AND (NEW.ctl_target_group_key & 9223372036854775807)>0 AND NEW.ctl_target_group_revision>0) OR ((NEW.ctl_target_group_key & 9223372036854775808)=0 AND NEW.ctl_target_group_revision=0)) AND ((NEW.ctl_target_session_boot_id=0 AND NEW.ctl_target_session_process_id=0 AND NEW.ctl_target_session_seq=0) OR (NEW.ctl_target_actor_kind=1 AND NEW.ctl_target_session_boot_id>0 AND NEW.ctl_target_session_process_id>0 AND NEW.ctl_target_session_seq>0)) AND NEW.ctl_target_context_version=1 AND NEW.ctl_target_zone_vnum>=-1 AND NEW.ctl_target_quality_flags<=1023) AND ((NEW.ctl_kind=1 AND (((NEW.ctl_source_actor_kind=1 AND NEW.ctl_source_actor_id=NEW.ctl_source_actor_pid AND NEW.ctl_source_actor_pid BETWEEN 1 AND 2147483647 AND NEW.ctl_source_owner_subject_id=NEW.ctl_source_actor_id) OR (NEW.ctl_source_actor_kind IN (2,3) AND (NEW.ctl_source_actor_id & 9223372036854775808)<>0 AND (NEW.ctl_source_actor_id & 9223372036854775807)>0 AND NEW.ctl_source_actor_pid=-1 AND ((NEW.ctl_source_actor_kind=2 AND NEW.ctl_source_owner_subject_id BETWEEN 1 AND 2147483647) OR (NEW.ctl_source_actor_kind=3 AND NEW.ctl_source_owner_subject_id=0)))) AND (((NEW.ctl_source_group_key & 9223372036854775808)<>0 AND (NEW.ctl_source_group_key & 9223372036854775807)>0 AND NEW.ctl_source_group_revision>0) OR ((NEW.ctl_source_group_key & 9223372036854775808)=0 AND NEW.ctl_source_group_revision=0)) AND ((NEW.ctl_source_session_boot_id=0 AND NEW.ctl_source_session_process_id=0 AND NEW.ctl_source_session_seq=0) OR (NEW.ctl_source_actor_kind=1 AND NEW.ctl_source_session_boot_id>0 AND NEW.ctl_source_session_process_id>0 AND NEW.ctl_source_session_seq>0)) AND NEW.ctl_source_context_version=1 AND NEW.ctl_source_zone_vnum>=-1 AND NEW.ctl_source_quality_flags<=1023)) OR (NEW.ctl_kind<>1 AND NEW.ctl_source_actor_id=0 AND NEW.ctl_source_actor_pid=0 AND NEW.ctl_source_owner_subject_id=0 AND NEW.ctl_source_actor_kind=0 AND NEW.ctl_source_power_band=0 AND NEW.ctl_source_session_boot_id=0 AND NEW.ctl_source_session_process_id=0 AND NEW.ctl_source_session_seq=0 AND NEW.ctl_source_level_band=0 AND NEW.ctl_source_class_id=0 AND NEW.ctl_source_race_id=0 AND NEW.ctl_source_faction_id=0 AND NEW.ctl_source_zone_vnum=0 AND NEW.ctl_source_group_size=0 AND NEW.ctl_source_group_key=0 AND NEW.ctl_source_group_revision=0 AND NEW.ctl_source_context_version=0 AND NEW.ctl_source_quality_flags=0 AND NEW.ctl_source_battle_seq=0 AND NEW.ctl_source_association_revision=0 AND NEW.ctl_source_association_fact_sequence=0)))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_ctl_actors';
    END IF;
    IF NOT (NEW.record_kind<>13 OR (((NEW.ctl_source_battle_seq=0 AND NEW.ctl_source_association_revision=0 AND NEW.ctl_source_association_fact_sequence=0) OR (NEW.ctl_source_battle_seq>0 AND NEW.ctl_source_association_revision>0 AND NEW.ctl_source_association_fact_sequence>0)) AND ((NEW.ctl_target_battle_seq=0 AND NEW.ctl_target_association_revision=0 AND NEW.ctl_target_association_fact_sequence=0) OR (NEW.ctl_target_battle_seq>0 AND NEW.ctl_target_association_revision>0 AND NEW.ctl_target_association_fact_sequence>0)) AND ((NEW.ctl_target_battle_seq=0 AND NEW.ctl_last_target_association_revision=0 AND NEW.ctl_last_target_association_fact_sequence=0) OR (NEW.ctl_target_battle_seq>0 AND NEW.ctl_last_target_association_revision>=NEW.ctl_target_association_revision AND NEW.ctl_last_target_association_fact_sequence>=NEW.ctl_target_association_fact_sequence AND (NEW.ctl_last_target_association_fact_sequence<>NEW.ctl_target_association_fact_sequence OR NEW.ctl_last_target_association_revision=NEW.ctl_target_association_revision))))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_ctl_association';
    END IF;
    IF NOT (NEW.record_kind<>13 OR NEW.ctl_kind<>1 OR (NEW.ctl_start_usec=NEW.ctl_at_usec AND NEW.ctl_at_usec=NEW.ctl_decision_usec AND NEW.ctl_start_utc_usec=NEW.ctl_at_utc_usec AND NEW.ctl_at_utc_usec=NEW.ctl_decision_utc_usec AND NEW.ctl_last_target_association_revision=NEW.ctl_target_association_revision AND NEW.ctl_last_target_association_fact_sequence=NEW.ctl_target_association_fact_sequence AND NEW.ctl_previous_state_sequence=0 AND NEW.ctl_boundary=11 AND NEW.ctl_family BETWEEN 1 AND 8 AND NEW.ctl_result BETWEEN 1 AND 14 AND NEW.ctl_state_available=255 AND NEW.ctl_duration_coverage=0 AND (((NEW.ctl_flags & 32)<>0 AND NEW.ctl_source_actor_id=NEW.ctl_target_actor_id AND NEW.ctl_source_actor_kind=NEW.ctl_target_actor_kind) OR ((NEW.ctl_flags & 32)=0 AND (NEW.ctl_source_actor_id<>NEW.ctl_target_actor_id OR NEW.ctl_source_actor_kind<>NEW.ctl_target_actor_kind))) AND ((NEW.ctl_result<>1 AND NEW.ctl_configured_ticks=0 AND (NEW.ctl_flags & 28)=0) OR (NEW.ctl_result=1 AND (NEW.ctl_after_mask & CASE WHEN NEW.ctl_family=8 THEN 136 ELSE (1 << (NEW.ctl_family-1)) END)<>0 AND ((NEW.ctl_flags & 8)=0 OR (NEW.ctl_family=6 AND (NEW.ctl_before_mask & 32)<>0)) AND ((NEW.ctl_flags & 16)=0 OR NEW.ctl_family=2) AND ((NEW.ctl_flags & 4)=0 OR NEW.ctl_family IN (2,3,6)))))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_ctl_operation';
    END IF;
    IF NOT (NEW.record_kind<>13 OR (NEW.ctl_kind BETWEEN 1 AND 4 AND (NEW.ctl_kind=1 OR (NEW.ctl_family=0 AND NEW.ctl_result=0 AND NEW.ctl_flags=0 AND NEW.ctl_configured_ticks=0 AND NEW.ctl_boundary<>11 AND ((NEW.ctl_kind=2 AND NEW.ctl_start_usec=NEW.ctl_at_usec AND NEW.ctl_at_usec=NEW.ctl_decision_usec AND NEW.ctl_start_utc_usec=NEW.ctl_at_utc_usec AND NEW.ctl_at_utc_usec=NEW.ctl_decision_utc_usec AND NEW.ctl_last_target_association_revision=NEW.ctl_target_association_revision AND NEW.ctl_last_target_association_fact_sequence=NEW.ctl_target_association_fact_sequence AND NEW.ctl_state_available=255 AND (NEW.ctl_duration_coverage=255 OR (NEW.ctl_quality_flags & 1)<>0) AND NEW.ctl_target_battle_seq>0 AND NEW.ctl_previous_state_sequence=0 AND NEW.ctl_before_mask=NEW.ctl_after_mask AND NEW.ctl_boundary IN (1,4)) OR (NEW.ctl_kind=3 AND NEW.ctl_state_available=255 AND (NEW.ctl_duration_coverage=255 OR (NEW.ctl_quality_flags & 1)<>0) AND NEW.ctl_target_battle_seq>0 AND NEW.ctl_previous_state_sequence>0 AND NEW.ctl_boundary BETWEEN 2 AND 10 AND (NEW.ctl_at_usec=NEW.ctl_decision_usec OR (NEW.ctl_quality_flags & 64)<>0)) OR (NEW.ctl_kind=4 AND NEW.ctl_state_available=0 AND NEW.ctl_duration_coverage=0 AND NEW.ctl_before_mask=0 AND NEW.ctl_after_mask=0 AND (NEW.ctl_quality_flags & 1)<>0 AND NEW.ctl_boundary>=7)))))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_ctl_state';
    END IF;
    IF NOT (NEW.record_kind<>13 OR (NEW.ctl_quality_flags & 128)<>0 OR ((CAST(NEW.ctl_start_utc_usec AS UNSIGNED)=9223372036854775808 OR CAST(NEW.ctl_at_utc_usec AS UNSIGNED)=9223372036854775808 OR NEW.ctl_start_utc_usec<=NEW.ctl_at_utc_usec) AND (CAST(NEW.ctl_at_utc_usec AS UNSIGNED)=9223372036854775808 OR CAST(NEW.ctl_decision_utc_usec AS UNSIGNED)=9223372036854775808 OR NEW.ctl_at_utc_usec<=NEW.ctl_decision_utc_usec))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_ctl_utc';
    END IF;
    IF NOT (NEW.record_kind<>13 OR COALESCE(NEW.environment_id,NEW.season_id,NEW.session_boot_id,NEW.session_process_id,NEW.session_seq,NEW.subject_id,NEW.pid,NEW.connection_boot_id,NEW.connection_process_id,NEW.connection_seq,NEW.start_monotonic_usec,NEW.end_monotonic_usec,NEW.start_utc_usec,NEW.end_utc_usec,NEW.duration_usec,NEW.category,NEW.context,NEW.context_quality,NEW.level_band,NEW.class_id,NEW.race_id,NEW.faction_id,NEW.zone_vnum,NEW.group_size,NEW.config_id,NEW.classifier_version,NEW.policy_version,NEW.quality_flags,NEW.lifecycle,NEW.end_reason,NEW.at_monotonic_usec,NEW.at_utc_usec,NEW.checkpoint_revision,NEW.connected_usec,NEW.active_usec,NEW.idle_usec,NEW.unknown_usec,NEW.resident_usec,NEW.linkdead_usec,NEW.gap_reason,NEW.first_missing_record_seq,NEW.last_missing_record_seq,NEW.dropped_records,NEW.config_revision,NEW.build_version,NEW.content_version,NEW.property_version,NEW.fingerprint,NEW.effective_utc_usec,NEW.interval_usec,NEW.checkpoint_interval_usec,NEW.active_window_usec,NEW.context_segments_per_minute,NEW.pulse_slot_count,NEW.backend,NEW.enabled,NEW.progression_kind,NEW.progression_source,NEW.progression_reason,NEW.progression_observation_status,NEW.progression_modifier_flags,NEW.progression_requested_xp,NEW.progression_computed_xp,NEW.progression_applied_xp,NEW.progression_before_exp,NEW.progression_after_exp,NEW.progression_before_level,NEW.progression_after_level,NEW.progression_threshold_xp,NEW.encounter_boot_id,NEW.encounter_process_id,NEW.encounter_seq,NEW.encounter_event,NEW.encounter_mode,NEW.encounter_outcome,NEW.encounter_revision,NEW.encounter_environment_id,NEW.encounter_season_id,NEW.encounter_config_id,NEW.encounter_classifier_version,NEW.encounter_policy_version,NEW.encounter_zone_vnum,NEW.encounter_group_key,NEW.encounter_participant_subject_id,NEW.encounter_participant_pid,NEW.encounter_start_monotonic_usec,NEW.encounter_start_utc_usec,NEW.elapsed_usec,NEW.participant_usec,NEW.participant_count,NEW.expected_credit_count,NEW.encounter_quality_flags,NEW.combat_encounter_boot_id,NEW.combat_encounter_process_id,NEW.combat_encounter_seq,NEW.combat_mode,NEW.combat_outcome,NEW.combat_revision,NEW.combat_environment_id,NEW.combat_season_id,NEW.combat_config_id,NEW.combat_classifier_version,NEW.combat_policy_version,NEW.combat_zone_vnum,NEW.combat_group_key,NEW.combat_actor_id,NEW.combat_actor_pid,NEW.combat_owner_subject_id,NEW.combat_actor_kind,NEW.combat_unique_player_count,NEW.combat_participant_count,NEW.combat_dropped_participant_count,NEW.combat_power_band,NEW.combat_opponent_power_band,NEW.combat_opponent_count,NEW.combat_modifier_flags,NEW.combat_start_monotonic_usec,NEW.combat_end_monotonic_usec,NEW.combat_start_utc_usec,NEW.combat_end_utc_usec,NEW.combat_damage_dealt,NEW.combat_damage_taken,NEW.combat_healing_attempted,NEW.combat_effective_healing,NEW.combat_overhealing,NEW.combat_control_applications,NEW.combat_casting_attempts,NEW.combat_casting_completions,NEW.combat_casting_aborts,NEW.combat_casting_elapsed_usec,NEW.combat_tanking_usec,NEW.combat_quality_flags) IS NULL) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'chk_telemetry_ctl_inactive';
    END IF;
END//
DELIMITER ;

-- Independent reviewed incident schema 6 includes typed-control loss.
CREATE TABLE IF NOT EXISTS telemetry_incident_registry_v6 (
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
    CONSTRAINT chk_incident_registry_version_v6 CHECK (environment_id <> 0 AND season_id <> 0 AND registry_version <> 0 AND previous_registry_version < registry_version AND registry_version - previous_registry_version = 1),
    CONSTRAINT chk_incident_registry_count_v6 CHECK (incident_count <= 64),
    CONSTRAINT chk_incident_registry_utc_v6 CHECK ((reviewed_from_utc_usec IS NULL OR CAST(reviewed_from_utc_usec AS UNSIGNED) <> 9223372036854775808) AND (reviewed_through_utc_usec IS NULL OR CAST(reviewed_through_utc_usec AS UNSIGNED) <> 9223372036854775808)),
    CONSTRAINT chk_incident_registry_time_v6 CHECK (reviewed_from_utc_usec IS NULL OR reviewed_through_utc_usec IS NULL OR reviewed_from_utc_usec <= reviewed_through_utc_usec)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_incident_v6 (
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
    CONSTRAINT fk_incident_registry_v6 FOREIGN KEY (environment_id,season_id,registry_version) REFERENCES telemetry_incident_registry_v6 (environment_id,season_id,registry_version) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_incident_identity_v6 CHECK (incident_id <> 0 AND ((producer_boot_id IS NULL AND producer_process_id IS NULL AND first_record_seq IS NULL AND last_record_seq IS NULL) OR (producer_boot_id IS NOT NULL AND producer_process_id IS NOT NULL AND producer_boot_id > 0 AND producer_process_id > 0))),
    CONSTRAINT chk_incident_utc_v6 CHECK ((start_utc_usec IS NULL OR CAST(start_utc_usec AS UNSIGNED) <> 9223372036854775808) AND (end_utc_usec IS NULL OR CAST(end_utc_usec AS UNSIGNED) <> 9223372036854775808) AND (verified_occurrence_utc_usec IS NULL OR CAST(verified_occurrence_utc_usec AS UNSIGNED) <> 9223372036854775808)),
    CONSTRAINT chk_incident_time_v6 CHECK (start_utc_usec IS NULL OR end_utc_usec IS NULL OR start_utc_usec <= end_utc_usec),
    CONSTRAINT chk_incident_sequence_v6 CHECK ((first_record_seq IS NULL OR first_record_seq > 0) AND (last_record_seq IS NULL OR last_record_seq > 0) AND (first_record_seq IS NULL OR last_record_seq IS NULL OR first_record_seq <= last_record_seq)),
    CONSTRAINT chk_incident_family_v6 CHECK (record_kind_mask BETWEEN 2 AND 16382 AND (record_kind_mask & 1) = 0),
    CONSTRAINT chk_incident_verification_v6 CHECK ((verified_boot_id IS NULL AND verified_process_id IS NULL AND verified_record_seq IS NULL AND verified_record_kind IS NULL AND verified_occurrence_utc_usec IS NULL) OR (fix_reference_digest IS NOT NULL AND verified_boot_id IS NOT NULL AND verified_process_id IS NOT NULL AND verified_record_seq IS NOT NULL AND verified_record_kind IS NOT NULL AND verified_boot_id > 0 AND verified_process_id > 0 AND verified_record_seq > 0 AND verified_record_kind BETWEEN 1 AND 13 AND (record_kind_mask & (1 << verified_record_kind)) <> 0)),
    CONSTRAINT chk_incident_disposition_v6 CHECK (backlog_disposition BETWEEN 0 AND 5 AND observation_provenance BETWEEN 1 AND 3 AND status BETWEEN 1 AND 2)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
