-- Shared battle record family 10; earlier typed families remain unchanged.

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_boot_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_boot_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER ownership_source');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_process_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_process_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_boot_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_seq'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_seq BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_process_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_related_boot_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_related_boot_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_seq');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_related_process_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_related_process_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_related_boot_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_related_seq'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_related_seq BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_related_process_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_environment_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_environment_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_related_seq');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_season_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_season_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_environment_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_config_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_config_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_season_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_classifier_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_classifier_version INT UNSIGNED NULL DEFAULT NULL AFTER battle_config_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_policy_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_policy_version INT UNSIGNED NULL DEFAULT NULL AFTER battle_classifier_version');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_scope_zone_vnum'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_scope_zone_vnum INT NULL DEFAULT NULL AFTER battle_policy_version');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_scope_group_key'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_scope_group_key BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_scope_zone_vnum');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_revision'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_revision BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_scope_group_key');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_fact_sequence'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_fact_sequence INT UNSIGNED NULL DEFAULT NULL AFTER battle_revision');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_fact_index'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_fact_index SMALLINT UNSIGNED NULL DEFAULT NULL AFTER battle_fact_sequence');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_fact_count'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_fact_count SMALLINT UNSIGNED NULL DEFAULT NULL AFTER battle_fact_index');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_definition_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_definition_version SMALLINT UNSIGNED NULL DEFAULT NULL AFTER battle_fact_count');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_fact_kind'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_fact_kind TINYINT UNSIGNED NULL DEFAULT NULL AFTER battle_definition_version');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_relation'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_relation TINYINT UNSIGNED NULL DEFAULT NULL AFTER battle_fact_kind');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_side_status'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_side_status TINYINT UNSIGNED NULL DEFAULT NULL AFTER battle_relation');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_mode'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_mode TINYINT UNSIGNED NULL DEFAULT NULL AFTER battle_side_status');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_close_reason'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_close_reason TINYINT UNSIGNED NULL DEFAULT NULL AFTER battle_mode');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_active'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_active TINYINT UNSIGNED NULL DEFAULT NULL AFTER battle_close_reason');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_side'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_side TINYINT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_active');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_end_censored'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_end_censored TINYINT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_side');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_roles'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_roles SMALLINT UNSIGNED NULL DEFAULT NULL AFTER battle_end_censored');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_count'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_count SMALLINT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_roles');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_active_actor_count'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_active_actor_count SMALLINT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_count');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_observed_owner_count'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_observed_owner_count SMALLINT UNSIGNED NULL DEFAULT NULL AFTER battle_active_actor_count');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_dropped_actor_count'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_dropped_actor_count INT UNSIGNED NULL DEFAULT NULL AFTER battle_observed_owner_count');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_dropped_actor_count');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_pid'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_pid INT NULL DEFAULT NULL AFTER battle_actor_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_owner_subject_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_owner_subject_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_pid');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_kind'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_kind TINYINT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_owner_subject_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_power_band'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_power_band SMALLINT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_kind');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_encounter_boot_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_encounter_boot_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_power_band');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_encounter_process_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_encounter_process_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_encounter_boot_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_encounter_seq'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_encounter_seq BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_encounter_process_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_session_boot_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_session_boot_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_encounter_seq');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_session_process_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_session_process_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_session_boot_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_session_seq'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_session_seq BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_session_process_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_level_band'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_level_band SMALLINT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_session_seq');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_class_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_class_id SMALLINT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_level_band');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_race_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_race_id SMALLINT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_class_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_faction_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_faction_id SMALLINT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_race_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_zone_vnum'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_zone_vnum INT NULL DEFAULT NULL AFTER battle_actor_faction_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_group_size'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_group_size INT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_zone_vnum');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_group_key'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_group_key BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_group_size');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_group_revision'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_group_revision SMALLINT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_group_key');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_context_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_context_version SMALLINT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_group_revision');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_actor_quality_flags'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_actor_quality_flags INT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_context_version');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_related_actor_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_related_actor_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_actor_quality_flags');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_related_actor_kind'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_related_actor_kind TINYINT UNSIGNED NULL DEFAULT NULL AFTER battle_related_actor_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_present_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_present_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_related_actor_kind');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_contributor_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_contributor_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_present_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_pve_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_pve_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_contributor_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_pvp_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_pvp_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_pve_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_mixed_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_mixed_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_pvp_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_unknown_mode_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_unknown_mode_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_mixed_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_outnumbered_owner_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_outnumbered_owner_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_unknown_mode_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_unknown_side_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_unknown_side_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_outnumbered_owner_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_start_monotonic_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_start_monotonic_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_unknown_side_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_at_monotonic_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_at_monotonic_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_start_monotonic_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_observed_through_monotonic_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_observed_through_monotonic_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_at_monotonic_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_last_engagement_monotonic_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_last_engagement_monotonic_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_observed_through_monotonic_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_inactivity_grace_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_inactivity_grace_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_last_engagement_monotonic_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_at_utc_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_at_utc_usec BIGINT NULL DEFAULT NULL AFTER battle_inactivity_grace_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_observed_through_utc_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_observed_through_utc_usec BIGINT NULL DEFAULT NULL AFTER battle_at_utc_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='battle_quality_flags'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN battle_quality_flags INT UNSIGNED NULL DEFAULT NULL AFTER battle_observed_through_utc_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND index_name='uq_telemetry_battle_fact'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD UNIQUE KEY uq_telemetry_battle_fact (battle_boot_id,battle_process_id,battle_seq,battle_fact_sequence)');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_interval' AND constraint_name='chk_telemetry_battle_payload'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD CONSTRAINT chk_telemetry_battle_payload CHECK ((record_kind=10 AND battle_boot_id IS NOT NULL AND battle_process_id IS NOT NULL AND battle_seq IS NOT NULL AND battle_related_boot_id IS NOT NULL AND battle_related_process_id IS NOT NULL AND battle_related_seq IS NOT NULL AND battle_environment_id IS NOT NULL AND battle_season_id IS NOT NULL AND battle_config_id IS NOT NULL AND battle_classifier_version IS NOT NULL AND battle_policy_version IS NOT NULL AND battle_scope_zone_vnum IS NOT NULL AND battle_scope_group_key IS NOT NULL AND battle_revision IS NOT NULL AND battle_fact_sequence IS NOT NULL AND battle_fact_index IS NOT NULL AND battle_fact_count IS NOT NULL AND battle_definition_version IS NOT NULL AND battle_fact_kind IS NOT NULL AND battle_relation IS NOT NULL AND battle_side_status IS NOT NULL AND battle_mode IS NOT NULL AND battle_close_reason IS NOT NULL AND battle_actor_active IS NOT NULL AND battle_actor_side IS NOT NULL AND battle_end_censored IS NOT NULL AND battle_actor_roles IS NOT NULL AND battle_actor_count IS NOT NULL AND battle_active_actor_count IS NOT NULL AND battle_observed_owner_count IS NOT NULL AND battle_dropped_actor_count IS NOT NULL AND battle_actor_id IS NOT NULL AND battle_actor_pid IS NOT NULL AND battle_actor_owner_subject_id IS NOT NULL AND battle_actor_kind IS NOT NULL AND battle_actor_power_band IS NOT NULL AND battle_actor_encounter_boot_id IS NOT NULL AND battle_actor_encounter_process_id IS NOT NULL AND battle_actor_encounter_seq IS NOT NULL AND battle_actor_session_boot_id IS NOT NULL AND battle_actor_session_process_id IS NOT NULL AND battle_actor_session_seq IS NOT NULL AND battle_actor_level_band IS NOT NULL AND battle_actor_class_id IS NOT NULL AND battle_actor_race_id IS NOT NULL AND battle_actor_faction_id IS NOT NULL AND battle_actor_zone_vnum IS NOT NULL AND battle_actor_group_size IS NOT NULL AND battle_actor_group_key IS NOT NULL AND battle_actor_group_revision IS NOT NULL AND battle_actor_context_version IS NOT NULL AND battle_actor_quality_flags IS NOT NULL AND battle_related_actor_id IS NOT NULL AND battle_related_actor_kind IS NOT NULL AND battle_present_usec IS NOT NULL AND battle_contributor_usec IS NOT NULL AND battle_pve_usec IS NOT NULL AND battle_pvp_usec IS NOT NULL AND battle_mixed_usec IS NOT NULL AND battle_unknown_mode_usec IS NOT NULL AND battle_outnumbered_owner_usec IS NOT NULL AND battle_unknown_side_usec IS NOT NULL AND battle_start_monotonic_usec IS NOT NULL AND battle_at_monotonic_usec IS NOT NULL AND battle_observed_through_monotonic_usec IS NOT NULL AND battle_last_engagement_monotonic_usec IS NOT NULL AND battle_inactivity_grace_usec IS NOT NULL AND battle_at_utc_usec IS NOT NULL AND battle_observed_through_utc_usec IS NOT NULL AND battle_quality_flags IS NOT NULL) OR (record_kind<>10 AND battle_boot_id IS NULL AND battle_process_id IS NULL AND battle_seq IS NULL AND battle_related_boot_id IS NULL AND battle_related_process_id IS NULL AND battle_related_seq IS NULL AND battle_environment_id IS NULL AND battle_season_id IS NULL AND battle_config_id IS NULL AND battle_classifier_version IS NULL AND battle_policy_version IS NULL AND battle_scope_zone_vnum IS NULL AND battle_scope_group_key IS NULL AND battle_revision IS NULL AND battle_fact_sequence IS NULL AND battle_fact_index IS NULL AND battle_fact_count IS NULL AND battle_definition_version IS NULL AND battle_fact_kind IS NULL AND battle_relation IS NULL AND battle_side_status IS NULL AND battle_mode IS NULL AND battle_close_reason IS NULL AND battle_actor_active IS NULL AND battle_actor_side IS NULL AND battle_end_censored IS NULL AND battle_actor_roles IS NULL AND battle_actor_count IS NULL AND battle_active_actor_count IS NULL AND battle_observed_owner_count IS NULL AND battle_dropped_actor_count IS NULL AND battle_actor_id IS NULL AND battle_actor_pid IS NULL AND battle_actor_owner_subject_id IS NULL AND battle_actor_kind IS NULL AND battle_actor_power_band IS NULL AND battle_actor_encounter_boot_id IS NULL AND battle_actor_encounter_process_id IS NULL AND battle_actor_encounter_seq IS NULL AND battle_actor_session_boot_id IS NULL AND battle_actor_session_process_id IS NULL AND battle_actor_session_seq IS NULL AND battle_actor_level_band IS NULL AND battle_actor_class_id IS NULL AND battle_actor_race_id IS NULL AND battle_actor_faction_id IS NULL AND battle_actor_zone_vnum IS NULL AND battle_actor_group_size IS NULL AND battle_actor_group_key IS NULL AND battle_actor_group_revision IS NULL AND battle_actor_context_version IS NULL AND battle_actor_quality_flags IS NULL AND battle_related_actor_id IS NULL AND battle_related_actor_kind IS NULL AND battle_present_usec IS NULL AND battle_contributor_usec IS NULL AND battle_pve_usec IS NULL AND battle_pvp_usec IS NULL AND battle_mixed_usec IS NULL AND battle_unknown_mode_usec IS NULL AND battle_outnumbered_owner_usec IS NULL AND battle_unknown_side_usec IS NULL AND battle_start_monotonic_usec IS NULL AND battle_at_monotonic_usec IS NULL AND battle_observed_through_monotonic_usec IS NULL AND battle_last_engagement_monotonic_usec IS NULL AND battle_inactivity_grace_usec IS NULL AND battle_at_utc_usec IS NULL AND battle_observed_through_utc_usec IS NULL AND battle_quality_flags IS NULL))');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_interval' AND constraint_name='chk_telemetry_battle_binding'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD CONSTRAINT chk_telemetry_battle_binding CHECK (record_kind<>10 OR (battle_boot_id=boot_id AND battle_process_id=process_id AND battle_boot_id>0 AND battle_process_id>0 AND battle_seq>0 AND battle_at_utc_usec=occurrence_utc_usec AND battle_definition_version=1 AND battle_environment_id>0 AND battle_season_id>0 AND battle_config_id>0 AND battle_classifier_version>0 AND battle_policy_version>0 AND battle_scope_zone_vnum=-1 AND battle_scope_group_key=0 AND battle_revision>0 AND battle_fact_sequence>battle_fact_index AND battle_fact_index<battle_fact_count AND battle_actor_count BETWEEN 2 AND 64 AND battle_active_actor_count<=battle_actor_count AND battle_observed_owner_count<=battle_active_actor_count AND battle_fact_kind BETWEEN 1 AND 7 AND battle_side_status BETWEEN 1 AND 3 AND battle_mode BETWEEN 0 AND 3 AND battle_start_monotonic_usec<=battle_last_engagement_monotonic_usec AND battle_last_engagement_monotonic_usec<=battle_observed_through_monotonic_usec AND battle_observed_through_monotonic_usec<=battle_at_monotonic_usec AND battle_inactivity_grace_usec>0 AND battle_quality_flags<=1023))');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

-- Independent reviewed incident schema 3 includes shared battle loss.
CREATE TABLE IF NOT EXISTS telemetry_incident_registry_v3 (
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
    CONSTRAINT chk_incident_registry_version_v3 CHECK (environment_id <> 0 AND season_id <> 0 AND registry_version <> 0 AND previous_registry_version < registry_version AND registry_version - previous_registry_version = 1),
    CONSTRAINT chk_incident_registry_count_v3 CHECK (incident_count <= 64),
    CONSTRAINT chk_incident_registry_utc_v3 CHECK ((reviewed_from_utc_usec IS NULL OR CAST(reviewed_from_utc_usec AS UNSIGNED) <> 9223372036854775808) AND (reviewed_through_utc_usec IS NULL OR CAST(reviewed_through_utc_usec AS UNSIGNED) <> 9223372036854775808)),
    CONSTRAINT chk_incident_registry_time_v3 CHECK (reviewed_from_utc_usec IS NULL OR reviewed_through_utc_usec IS NULL OR reviewed_from_utc_usec <= reviewed_through_utc_usec)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_incident_v3 (
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
    CONSTRAINT fk_incident_registry_v3 FOREIGN KEY (environment_id,season_id,registry_version) REFERENCES telemetry_incident_registry_v3 (environment_id,season_id,registry_version) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_incident_identity_v3 CHECK (incident_id <> 0 AND ((producer_boot_id IS NULL AND producer_process_id IS NULL AND first_record_seq IS NULL AND last_record_seq IS NULL) OR (producer_boot_id IS NOT NULL AND producer_process_id IS NOT NULL AND producer_boot_id > 0 AND producer_process_id > 0))),
    CONSTRAINT chk_incident_utc_v3 CHECK ((start_utc_usec IS NULL OR CAST(start_utc_usec AS UNSIGNED) <> 9223372036854775808) AND (end_utc_usec IS NULL OR CAST(end_utc_usec AS UNSIGNED) <> 9223372036854775808) AND (verified_occurrence_utc_usec IS NULL OR CAST(verified_occurrence_utc_usec AS UNSIGNED) <> 9223372036854775808)),
    CONSTRAINT chk_incident_time_v3 CHECK (start_utc_usec IS NULL OR end_utc_usec IS NULL OR start_utc_usec <= end_utc_usec),
    CONSTRAINT chk_incident_sequence_v3 CHECK ((first_record_seq IS NULL OR first_record_seq > 0) AND (last_record_seq IS NULL OR last_record_seq > 0) AND (first_record_seq IS NULL OR last_record_seq IS NULL OR first_record_seq <= last_record_seq)),
    CONSTRAINT chk_incident_family_v3 CHECK (record_kind_mask BETWEEN 2 AND 2046 AND (record_kind_mask & 1) = 0),
    CONSTRAINT chk_incident_verification_v3 CHECK ((verified_boot_id IS NULL AND verified_process_id IS NULL AND verified_record_seq IS NULL AND verified_record_kind IS NULL AND verified_occurrence_utc_usec IS NULL) OR (fix_reference_digest IS NOT NULL AND verified_boot_id IS NOT NULL AND verified_process_id IS NOT NULL AND verified_record_seq IS NOT NULL AND verified_record_kind IS NOT NULL AND verified_boot_id > 0 AND verified_process_id > 0 AND verified_record_seq > 0 AND verified_record_kind BETWEEN 1 AND 10 AND (record_kind_mask & (1 << verified_record_kind)) <> 0)),
    CONSTRAINT chk_incident_disposition_v3 CHECK (backlog_disposition BETWEEN 0 AND 5 AND observation_provenance BETWEEN 1 AND 3 AND status BETWEEN 1 AND 2)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
