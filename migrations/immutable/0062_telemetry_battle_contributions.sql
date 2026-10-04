-- Shared battle contribution record family 11; earlier typed families remain unchanged.

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_battle_boot_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_battle_boot_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER battle_quality_flags');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_battle_process_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_battle_process_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_battle_boot_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_battle_seq'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_battle_seq BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_battle_process_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_environment_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_environment_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_battle_seq');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_season_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_season_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_environment_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_config_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_config_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_season_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_classifier_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_classifier_version INT UNSIGNED NULL DEFAULT NULL AFTER bc_config_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_policy_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_policy_version INT UNSIGNED NULL DEFAULT NULL AFTER bc_classifier_version');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_scope_zone_vnum'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_scope_zone_vnum INT NULL DEFAULT NULL AFTER bc_policy_version');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_scope_group_key'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_scope_group_key BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_scope_zone_vnum');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_scope_group_key');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_pid'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_pid INT NULL DEFAULT NULL AFTER bc_actor_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_owner_subject_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_owner_subject_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_actor_pid');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_kind'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_kind TINYINT UNSIGNED NULL DEFAULT NULL AFTER bc_actor_owner_subject_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_power_band'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_power_band SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bc_actor_kind');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_encounter_boot_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_encounter_boot_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_actor_power_band');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_encounter_process_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_encounter_process_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_actor_encounter_boot_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_encounter_seq'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_encounter_seq BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_actor_encounter_process_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_session_boot_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_session_boot_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_actor_encounter_seq');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_session_process_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_session_process_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_actor_session_boot_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_session_seq'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_session_seq BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_actor_session_process_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_level_band'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_level_band SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bc_actor_session_seq');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_class_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_class_id SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bc_actor_level_band');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_race_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_race_id SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bc_actor_class_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_faction_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_faction_id SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bc_actor_race_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_zone_vnum'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_zone_vnum INT NULL DEFAULT NULL AFTER bc_actor_faction_id');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_group_size'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_group_size INT UNSIGNED NULL DEFAULT NULL AFTER bc_actor_zone_vnum');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_group_key'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_group_key BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_actor_group_size');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_group_revision'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_group_revision SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bc_actor_group_key');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_context_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_context_version SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bc_actor_group_revision');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_quality_flags'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_quality_flags INT UNSIGNED NULL DEFAULT NULL AFTER bc_actor_context_version');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_first_association_revision'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_first_association_revision BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_actor_quality_flags');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_first_association_fact_sequence'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_first_association_fact_sequence INT UNSIGNED NULL DEFAULT NULL AFTER bc_first_association_revision');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_available_metrics'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_available_metrics INT UNSIGNED NULL DEFAULT NULL AFTER bc_first_association_fact_sequence');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_side_status'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_side_status TINYINT UNSIGNED NULL DEFAULT NULL AFTER bc_available_metrics');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_mode'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_mode TINYINT UNSIGNED NULL DEFAULT NULL AFTER bc_side_status');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_actor_side'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_actor_side TINYINT UNSIGNED NULL DEFAULT NULL AFTER bc_mode');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_context_quality_flags'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_context_quality_flags INT UNSIGNED NULL DEFAULT NULL AFTER bc_actor_side');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_segment_seq'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_segment_seq BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_context_quality_flags');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_last_association_revision'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_last_association_revision BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_segment_seq');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_last_association_fact_sequence'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_last_association_fact_sequence INT UNSIGNED NULL DEFAULT NULL AFTER bc_last_association_revision');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_modifier_flags'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_modifier_flags INT UNSIGNED NULL DEFAULT NULL AFTER bc_last_association_fact_sequence');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_start_monotonic_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_start_monotonic_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_modifier_flags');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_start_utc_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_start_utc_usec BIGINT NULL DEFAULT NULL AFTER bc_start_monotonic_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_observed_through_monotonic_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_observed_through_monotonic_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_start_utc_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_observed_through_utc_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_observed_through_utc_usec BIGINT NULL DEFAULT NULL AFTER bc_observed_through_monotonic_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_decision_monotonic_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_decision_monotonic_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_observed_through_utc_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_decision_utc_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_decision_utc_usec BIGINT NULL DEFAULT NULL AFTER bc_decision_monotonic_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_damage_dealt'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_damage_dealt BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_decision_utc_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_damage_taken'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_damage_taken BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_damage_dealt');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_healing_attempted'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_healing_attempted BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_damage_taken');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_effective_healing'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_effective_healing BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_healing_attempted');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_overhealing'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_overhealing BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_effective_healing');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_healing_received'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_healing_received BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_overhealing');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_control_applications'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_control_applications BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_healing_received');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_control_received'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_control_received BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_control_applications');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_casting_attempts'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_casting_attempts BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_control_received');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_casting_completions'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_casting_completions BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_casting_attempts');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_casting_aborts'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_casting_aborts BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_casting_completions');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_casting_unresolved'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_casting_unresolved BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_casting_aborts');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_casting_elapsed_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_casting_elapsed_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_casting_unresolved');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_engaged_target_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_engaged_target_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_casting_elapsed_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_quality_flags'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_quality_flags INT UNSIGNED NULL DEFAULT NULL AFTER bc_engaged_target_usec');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_definition_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_definition_version SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bc_quality_flags');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bc_end_reason'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bc_end_reason TINYINT UNSIGNED NULL DEFAULT NULL AFTER bc_definition_version');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND index_name='uq_telemetry_battle_contribution'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD UNIQUE KEY uq_telemetry_battle_contribution (bc_battle_boot_id,bc_battle_process_id,bc_segment_seq)');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_interval' AND constraint_name='chk_telemetry_bc_payload'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD CONSTRAINT chk_telemetry_bc_payload CHECK ((record_kind=11 AND bc_battle_boot_id IS NOT NULL AND bc_battle_process_id IS NOT NULL AND bc_battle_seq IS NOT NULL AND bc_environment_id IS NOT NULL AND bc_season_id IS NOT NULL AND bc_config_id IS NOT NULL AND bc_classifier_version IS NOT NULL AND bc_policy_version IS NOT NULL AND bc_scope_zone_vnum IS NOT NULL AND bc_scope_group_key IS NOT NULL AND bc_actor_id IS NOT NULL AND bc_actor_pid IS NOT NULL AND bc_actor_owner_subject_id IS NOT NULL AND bc_actor_kind IS NOT NULL AND bc_actor_power_band IS NOT NULL AND bc_actor_encounter_boot_id IS NOT NULL AND bc_actor_encounter_process_id IS NOT NULL AND bc_actor_encounter_seq IS NOT NULL AND bc_actor_session_boot_id IS NOT NULL AND bc_actor_session_process_id IS NOT NULL AND bc_actor_session_seq IS NOT NULL AND bc_actor_level_band IS NOT NULL AND bc_actor_class_id IS NOT NULL AND bc_actor_race_id IS NOT NULL AND bc_actor_faction_id IS NOT NULL AND bc_actor_zone_vnum IS NOT NULL AND bc_actor_group_size IS NOT NULL AND bc_actor_group_key IS NOT NULL AND bc_actor_group_revision IS NOT NULL AND bc_actor_context_version IS NOT NULL AND bc_actor_quality_flags IS NOT NULL AND bc_first_association_revision IS NOT NULL AND bc_first_association_fact_sequence IS NOT NULL AND bc_available_metrics IS NOT NULL AND bc_side_status IS NOT NULL AND bc_mode IS NOT NULL AND bc_actor_side IS NOT NULL AND bc_context_quality_flags IS NOT NULL AND bc_segment_seq IS NOT NULL AND bc_last_association_revision IS NOT NULL AND bc_last_association_fact_sequence IS NOT NULL AND bc_modifier_flags IS NOT NULL AND bc_start_monotonic_usec IS NOT NULL AND bc_start_utc_usec IS NOT NULL AND bc_observed_through_monotonic_usec IS NOT NULL AND bc_observed_through_utc_usec IS NOT NULL AND bc_decision_monotonic_usec IS NOT NULL AND bc_decision_utc_usec IS NOT NULL AND bc_damage_dealt IS NOT NULL AND bc_damage_taken IS NOT NULL AND bc_healing_attempted IS NOT NULL AND bc_effective_healing IS NOT NULL AND bc_overhealing IS NOT NULL AND bc_healing_received IS NOT NULL AND bc_control_applications IS NOT NULL AND bc_control_received IS NOT NULL AND bc_casting_attempts IS NOT NULL AND bc_casting_completions IS NOT NULL AND bc_casting_aborts IS NOT NULL AND bc_casting_unresolved IS NOT NULL AND bc_casting_elapsed_usec IS NOT NULL AND bc_engaged_target_usec IS NOT NULL AND bc_quality_flags IS NOT NULL AND bc_definition_version IS NOT NULL AND bc_end_reason IS NOT NULL) OR (record_kind<>11 AND bc_battle_boot_id IS NULL AND bc_battle_process_id IS NULL AND bc_battle_seq IS NULL AND bc_environment_id IS NULL AND bc_season_id IS NULL AND bc_config_id IS NULL AND bc_classifier_version IS NULL AND bc_policy_version IS NULL AND bc_scope_zone_vnum IS NULL AND bc_scope_group_key IS NULL AND bc_actor_id IS NULL AND bc_actor_pid IS NULL AND bc_actor_owner_subject_id IS NULL AND bc_actor_kind IS NULL AND bc_actor_power_band IS NULL AND bc_actor_encounter_boot_id IS NULL AND bc_actor_encounter_process_id IS NULL AND bc_actor_encounter_seq IS NULL AND bc_actor_session_boot_id IS NULL AND bc_actor_session_process_id IS NULL AND bc_actor_session_seq IS NULL AND bc_actor_level_band IS NULL AND bc_actor_class_id IS NULL AND bc_actor_race_id IS NULL AND bc_actor_faction_id IS NULL AND bc_actor_zone_vnum IS NULL AND bc_actor_group_size IS NULL AND bc_actor_group_key IS NULL AND bc_actor_group_revision IS NULL AND bc_actor_context_version IS NULL AND bc_actor_quality_flags IS NULL AND bc_first_association_revision IS NULL AND bc_first_association_fact_sequence IS NULL AND bc_available_metrics IS NULL AND bc_side_status IS NULL AND bc_mode IS NULL AND bc_actor_side IS NULL AND bc_context_quality_flags IS NULL AND bc_segment_seq IS NULL AND bc_last_association_revision IS NULL AND bc_last_association_fact_sequence IS NULL AND bc_modifier_flags IS NULL AND bc_start_monotonic_usec IS NULL AND bc_start_utc_usec IS NULL AND bc_observed_through_monotonic_usec IS NULL AND bc_observed_through_utc_usec IS NULL AND bc_decision_monotonic_usec IS NULL AND bc_decision_utc_usec IS NULL AND bc_damage_dealt IS NULL AND bc_damage_taken IS NULL AND bc_healing_attempted IS NULL AND bc_effective_healing IS NULL AND bc_overhealing IS NULL AND bc_healing_received IS NULL AND bc_control_applications IS NULL AND bc_control_received IS NULL AND bc_casting_attempts IS NULL AND bc_casting_completions IS NULL AND bc_casting_aborts IS NULL AND bc_casting_unresolved IS NULL AND bc_casting_elapsed_usec IS NULL AND bc_engaged_target_usec IS NULL AND bc_quality_flags IS NULL AND bc_definition_version IS NULL AND bc_end_reason IS NULL))');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_interval' AND constraint_name='chk_telemetry_bc_binding'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD CONSTRAINT chk_telemetry_bc_binding CHECK (record_kind<>11 OR (bc_battle_boot_id=boot_id AND bc_battle_process_id=process_id AND bc_battle_boot_id>0 AND bc_battle_process_id>0 AND bc_battle_seq>0 AND bc_segment_seq>0 AND bc_decision_utc_usec=occurrence_utc_usec AND bc_definition_version=1 AND bc_end_reason BETWEEN 1 AND 4 AND bc_environment_id>0 AND bc_season_id>0 AND bc_config_id>0 AND bc_classifier_version>0 AND bc_policy_version>0 AND bc_scope_zone_vnum=-1 AND bc_scope_group_key=0 AND bc_first_association_revision>0 AND bc_first_association_revision<=bc_last_association_revision AND bc_first_association_fact_sequence>0 AND bc_first_association_fact_sequence<=bc_last_association_fact_sequence AND (bc_first_association_fact_sequence<>bc_last_association_fact_sequence OR bc_first_association_revision=bc_last_association_revision) AND bc_start_monotonic_usec<=bc_observed_through_monotonic_usec AND bc_observed_through_monotonic_usec<=bc_decision_monotonic_usec AND bc_available_metrics<=31 AND bc_quality_flags<=1023 AND (bc_context_quality_flags & bc_quality_flags)=bc_context_quality_flags AND (bc_actor_quality_flags & bc_context_quality_flags)=bc_actor_quality_flags AND bc_modifier_flags<=255 AND bc_side_status BETWEEN 1 AND 3 AND bc_mode BETWEEN 0 AND 3 AND ((bc_side_status=1 AND bc_actor_side BETWEEN 1 AND 2 AND (bc_context_quality_flags & 658)=0) OR (bc_side_status<>1 AND bc_actor_side=0)) AND (bc_end_reason<>4 OR (bc_quality_flags & 17)=17) AND ((bc_quality_flags & 128)<>0 OR ((bc_start_utc_usec=-9223372036854775808 OR bc_observed_through_utc_usec=-9223372036854775808 OR bc_start_utc_usec<=bc_observed_through_utc_usec) AND (bc_observed_through_utc_usec=-9223372036854775808 OR bc_decision_utc_usec=-9223372036854775808 OR bc_observed_through_utc_usec<=bc_decision_utc_usec)))))');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_interval' AND constraint_name='chk_telemetry_bc_actor'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD CONSTRAINT chk_telemetry_bc_actor CHECK (record_kind<>11 OR (bc_actor_owner_subject_id<=2147483647 AND bc_actor_id>0 AND bc_actor_context_version=1 AND bc_actor_zone_vnum>=-1 AND ((bc_actor_kind=1 AND bc_actor_id=bc_actor_pid AND bc_actor_pid>0 AND bc_actor_owner_subject_id=bc_actor_id) OR (bc_actor_kind IN (2,3) AND (bc_actor_id & 9223372036854775808)<>0 AND (bc_actor_id & 9223372036854775807)>0 AND bc_actor_pid=-1 AND ((bc_actor_kind=2 AND bc_actor_owner_subject_id>0) OR (bc_actor_kind=3 AND bc_actor_owner_subject_id=0)))) AND (((bc_actor_group_key & 9223372036854775808)=0 AND bc_actor_group_revision=0) OR ((bc_actor_group_key & 9223372036854775808)<>0 AND (bc_actor_group_key & 9223372036854775807)>0 AND bc_actor_group_revision>0)) AND ((bc_actor_encounter_boot_id=0 AND bc_actor_encounter_process_id=0 AND bc_actor_encounter_seq=0) OR (bc_actor_encounter_boot_id=bc_battle_boot_id AND bc_actor_encounter_process_id=bc_battle_process_id AND bc_actor_encounter_seq>0)) AND ((bc_actor_session_boot_id=0 AND bc_actor_session_process_id=0 AND bc_actor_session_seq=0) OR (bc_actor_kind=1 AND bc_actor_session_boot_id>0 AND bc_actor_session_process_id>0 AND bc_actor_session_seq>0))))');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

SET @telemetry_battle_sql = IF(EXISTS(SELECT 1 FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_interval' AND constraint_name='chk_telemetry_bc_counters'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD CONSTRAINT chk_telemetry_bc_counters CHECK (record_kind<>11 OR (((bc_available_metrics & 1)<>0 OR (bc_damage_dealt=0 AND bc_damage_taken=0)) AND ((bc_available_metrics & 2)<>0 OR (bc_healing_attempted=0 AND bc_effective_healing=0 AND bc_overhealing=0 AND bc_healing_received=0)) AND ((bc_available_metrics & 4)<>0 OR (bc_control_applications=0 AND bc_control_received=0)) AND ((bc_available_metrics & 8)<>0 OR (bc_casting_attempts=0 AND bc_casting_completions=0 AND bc_casting_aborts=0 AND bc_casting_unresolved=0 AND bc_casting_elapsed_usec=0)) AND ((bc_available_metrics & 16)<>0 OR bc_engaged_target_usec=0) AND bc_effective_healing<=bc_healing_attempted AND bc_overhealing<=bc_healing_attempted AND ((bc_quality_flags & 512)<>0 OR CAST(bc_effective_healing AS DECIMAL(21,0))+CAST(bc_overhealing AS DECIMAL(21,0))=bc_healing_attempted) AND CAST(bc_casting_completions AS DECIMAL(21,0))+CAST(bc_casting_aborts AS DECIMAL(21,0))+bc_casting_unresolved=bc_casting_attempts AND bc_casting_unresolved<=1 AND (bc_casting_attempts>0 OR bc_casting_elapsed_usec=0) AND (bc_casting_unresolved=0 OR (bc_quality_flags & 64)<>0) AND (bc_control_applications=0 OR (bc_modifier_flags & 32)<>0) AND bc_casting_elapsed_usec+CAST(bc_start_monotonic_usec AS DECIMAL(21,0))<=bc_observed_through_monotonic_usec AND bc_engaged_target_usec+CAST(bc_start_monotonic_usec AS DECIMAL(21,0))<=bc_observed_through_monotonic_usec))');
PREPARE telemetry_battle_statement FROM @telemetry_battle_sql;
EXECUTE telemetry_battle_statement;
DEALLOCATE PREPARE telemetry_battle_statement;

-- Independent reviewed incident schema 4 includes contribution loss.
CREATE TABLE IF NOT EXISTS telemetry_incident_registry_v4 (
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
    CONSTRAINT chk_incident_registry_version_v4 CHECK (environment_id <> 0 AND season_id <> 0 AND registry_version <> 0 AND previous_registry_version < registry_version AND registry_version - previous_registry_version = 1),
    CONSTRAINT chk_incident_registry_count_v4 CHECK (incident_count <= 64),
    CONSTRAINT chk_incident_registry_utc_v4 CHECK ((reviewed_from_utc_usec IS NULL OR CAST(reviewed_from_utc_usec AS UNSIGNED) <> 9223372036854775808) AND (reviewed_through_utc_usec IS NULL OR CAST(reviewed_through_utc_usec AS UNSIGNED) <> 9223372036854775808)),
    CONSTRAINT chk_incident_registry_time_v4 CHECK (reviewed_from_utc_usec IS NULL OR reviewed_through_utc_usec IS NULL OR reviewed_from_utc_usec <= reviewed_through_utc_usec)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_incident_v4 (
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
    CONSTRAINT fk_incident_registry_v4 FOREIGN KEY (environment_id,season_id,registry_version) REFERENCES telemetry_incident_registry_v4 (environment_id,season_id,registry_version) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_incident_identity_v4 CHECK (incident_id <> 0 AND ((producer_boot_id IS NULL AND producer_process_id IS NULL AND first_record_seq IS NULL AND last_record_seq IS NULL) OR (producer_boot_id IS NOT NULL AND producer_process_id IS NOT NULL AND producer_boot_id > 0 AND producer_process_id > 0))),
    CONSTRAINT chk_incident_utc_v4 CHECK ((start_utc_usec IS NULL OR CAST(start_utc_usec AS UNSIGNED) <> 9223372036854775808) AND (end_utc_usec IS NULL OR CAST(end_utc_usec AS UNSIGNED) <> 9223372036854775808) AND (verified_occurrence_utc_usec IS NULL OR CAST(verified_occurrence_utc_usec AS UNSIGNED) <> 9223372036854775808)),
    CONSTRAINT chk_incident_time_v4 CHECK (start_utc_usec IS NULL OR end_utc_usec IS NULL OR start_utc_usec <= end_utc_usec),
    CONSTRAINT chk_incident_sequence_v4 CHECK ((first_record_seq IS NULL OR first_record_seq > 0) AND (last_record_seq IS NULL OR last_record_seq > 0) AND (first_record_seq IS NULL OR last_record_seq IS NULL OR first_record_seq <= last_record_seq)),
    CONSTRAINT chk_incident_family_v4 CHECK (record_kind_mask BETWEEN 2 AND 4094 AND (record_kind_mask & 1) = 0),
    CONSTRAINT chk_incident_verification_v4 CHECK ((verified_boot_id IS NULL AND verified_process_id IS NULL AND verified_record_seq IS NULL AND verified_record_kind IS NULL AND verified_occurrence_utc_usec IS NULL) OR (fix_reference_digest IS NOT NULL AND verified_boot_id IS NOT NULL AND verified_process_id IS NOT NULL AND verified_record_seq IS NOT NULL AND verified_record_kind IS NOT NULL AND verified_boot_id > 0 AND verified_process_id > 0 AND verified_record_seq > 0 AND verified_record_kind BETWEEN 1 AND 11 AND (record_kind_mask & (1 << verified_record_kind)) <> 0)),
    CONSTRAINT chk_incident_disposition_v4 CHECK (backlog_disposition BETWEEN 0 AND 5 AND observation_provenance BETWEEN 1 AND 3 AND status BETWEEN 1 AND 2)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
