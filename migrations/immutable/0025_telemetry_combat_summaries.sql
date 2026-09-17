-- Bounded combat contribution and casting summaries share the typed interval
-- stream. This is additive and rerunnable; it does not activate a producer or
-- perform a production migration by itself.

SET @combat_add_encounter_boot_id = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_encounter_boot_id') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_encounter_boot_id BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_encounter_boot_id_unchanged');
PREPARE combat_add_encounter_boot_id_stmt FROM @combat_add_encounter_boot_id;
EXECUTE combat_add_encounter_boot_id_stmt;
DEALLOCATE PREPARE combat_add_encounter_boot_id_stmt;

SET @combat_add_encounter_process_id = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_encounter_process_id') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_encounter_process_id BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_encounter_process_id_unchanged');
PREPARE combat_add_encounter_process_id_stmt FROM @combat_add_encounter_process_id;
EXECUTE combat_add_encounter_process_id_stmt;
DEALLOCATE PREPARE combat_add_encounter_process_id_stmt;

SET @combat_add_encounter_seq = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_encounter_seq') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_encounter_seq BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_encounter_seq_unchanged');
PREPARE combat_add_encounter_seq_stmt FROM @combat_add_encounter_seq;
EXECUTE combat_add_encounter_seq_stmt;
DEALLOCATE PREPARE combat_add_encounter_seq_stmt;

SET @combat_add_mode = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_mode') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_mode TINYINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_mode_unchanged');
PREPARE combat_add_mode_stmt FROM @combat_add_mode;
EXECUTE combat_add_mode_stmt;
DEALLOCATE PREPARE combat_add_mode_stmt;

SET @combat_add_outcome = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_outcome') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_outcome TINYINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_outcome_unchanged');
PREPARE combat_add_outcome_stmt FROM @combat_add_outcome;
EXECUTE combat_add_outcome_stmt;
DEALLOCATE PREPARE combat_add_outcome_stmt;

SET @combat_add_revision = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_revision') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_revision SMALLINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_revision_unchanged');
PREPARE combat_add_revision_stmt FROM @combat_add_revision;
EXECUTE combat_add_revision_stmt;
DEALLOCATE PREPARE combat_add_revision_stmt;

SET @combat_add_environment_id = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_environment_id') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_environment_id BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_environment_id_unchanged');
PREPARE combat_add_environment_id_stmt FROM @combat_add_environment_id;
EXECUTE combat_add_environment_id_stmt;
DEALLOCATE PREPARE combat_add_environment_id_stmt;

SET @combat_add_season_id = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_season_id') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_season_id BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_season_id_unchanged');
PREPARE combat_add_season_id_stmt FROM @combat_add_season_id;
EXECUTE combat_add_season_id_stmt;
DEALLOCATE PREPARE combat_add_season_id_stmt;

SET @combat_add_config_id = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_config_id') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_config_id BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_config_id_unchanged');
PREPARE combat_add_config_id_stmt FROM @combat_add_config_id;
EXECUTE combat_add_config_id_stmt;
DEALLOCATE PREPARE combat_add_config_id_stmt;

SET @combat_add_classifier_version = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_classifier_version') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_classifier_version INT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_classifier_version_unchanged');
PREPARE combat_add_classifier_version_stmt FROM @combat_add_classifier_version;
EXECUTE combat_add_classifier_version_stmt;
DEALLOCATE PREPARE combat_add_classifier_version_stmt;

SET @combat_add_policy_version = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_policy_version') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_policy_version INT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_policy_version_unchanged');
PREPARE combat_add_policy_version_stmt FROM @combat_add_policy_version;
EXECUTE combat_add_policy_version_stmt;
DEALLOCATE PREPARE combat_add_policy_version_stmt;

SET @combat_add_zone_vnum = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_zone_vnum') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_zone_vnum INT NULL', 'SELECT 1 INTO @combat_add_zone_vnum_unchanged');
PREPARE combat_add_zone_vnum_stmt FROM @combat_add_zone_vnum;
EXECUTE combat_add_zone_vnum_stmt;
DEALLOCATE PREPARE combat_add_zone_vnum_stmt;

SET @combat_add_group_key = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_group_key') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_group_key BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_group_key_unchanged');
PREPARE combat_add_group_key_stmt FROM @combat_add_group_key;
EXECUTE combat_add_group_key_stmt;
DEALLOCATE PREPARE combat_add_group_key_stmt;

SET @combat_add_actor_id = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_actor_id') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_actor_id BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_actor_id_unchanged');
PREPARE combat_add_actor_id_stmt FROM @combat_add_actor_id;
EXECUTE combat_add_actor_id_stmt;
DEALLOCATE PREPARE combat_add_actor_id_stmt;

SET @combat_add_actor_pid = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_actor_pid') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_actor_pid INT NULL', 'SELECT 1 INTO @combat_add_actor_pid_unchanged');
PREPARE combat_add_actor_pid_stmt FROM @combat_add_actor_pid;
EXECUTE combat_add_actor_pid_stmt;
DEALLOCATE PREPARE combat_add_actor_pid_stmt;

SET @combat_add_owner_subject_id = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_owner_subject_id') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_owner_subject_id BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_owner_subject_id_unchanged');
PREPARE combat_add_owner_subject_id_stmt FROM @combat_add_owner_subject_id;
EXECUTE combat_add_owner_subject_id_stmt;
DEALLOCATE PREPARE combat_add_owner_subject_id_stmt;

SET @combat_add_actor_kind = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_actor_kind') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_actor_kind TINYINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_actor_kind_unchanged');
PREPARE combat_add_actor_kind_stmt FROM @combat_add_actor_kind;
EXECUTE combat_add_actor_kind_stmt;
DEALLOCATE PREPARE combat_add_actor_kind_stmt;

SET @combat_add_unique_player_count = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_unique_player_count') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_unique_player_count SMALLINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_unique_player_count_unchanged');
PREPARE combat_add_unique_player_count_stmt FROM @combat_add_unique_player_count;
EXECUTE combat_add_unique_player_count_stmt;
DEALLOCATE PREPARE combat_add_unique_player_count_stmt;

SET @combat_add_participant_count = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_participant_count') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_participant_count SMALLINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_participant_count_unchanged');
PREPARE combat_add_participant_count_stmt FROM @combat_add_participant_count;
EXECUTE combat_add_participant_count_stmt;
DEALLOCATE PREPARE combat_add_participant_count_stmt;

SET @combat_add_dropped_participant_count = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_dropped_participant_count') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_dropped_participant_count SMALLINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_dropped_participant_count_unchanged');
PREPARE combat_add_dropped_participant_count_stmt FROM @combat_add_dropped_participant_count;
EXECUTE combat_add_dropped_participant_count_stmt;
DEALLOCATE PREPARE combat_add_dropped_participant_count_stmt;

SET @combat_add_power_band = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_power_band') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_power_band SMALLINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_power_band_unchanged');
PREPARE combat_add_power_band_stmt FROM @combat_add_power_band;
EXECUTE combat_add_power_band_stmt;
DEALLOCATE PREPARE combat_add_power_band_stmt;

SET @combat_add_opponent_power_band = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_opponent_power_band') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_opponent_power_band SMALLINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_opponent_power_band_unchanged');
PREPARE combat_add_opponent_power_band_stmt FROM @combat_add_opponent_power_band;
EXECUTE combat_add_opponent_power_band_stmt;
DEALLOCATE PREPARE combat_add_opponent_power_band_stmt;

SET @combat_add_opponent_count = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_opponent_count') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_opponent_count SMALLINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_opponent_count_unchanged');
PREPARE combat_add_opponent_count_stmt FROM @combat_add_opponent_count;
EXECUTE combat_add_opponent_count_stmt;
DEALLOCATE PREPARE combat_add_opponent_count_stmt;

SET @combat_add_modifier_flags = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_modifier_flags') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_modifier_flags INT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_modifier_flags_unchanged');
PREPARE combat_add_modifier_flags_stmt FROM @combat_add_modifier_flags;
EXECUTE combat_add_modifier_flags_stmt;
DEALLOCATE PREPARE combat_add_modifier_flags_stmt;

SET @combat_add_start_monotonic_usec = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_start_monotonic_usec') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_start_monotonic_usec BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_start_monotonic_usec_unchanged');
PREPARE combat_add_start_monotonic_usec_stmt FROM @combat_add_start_monotonic_usec;
EXECUTE combat_add_start_monotonic_usec_stmt;
DEALLOCATE PREPARE combat_add_start_monotonic_usec_stmt;

SET @combat_add_end_monotonic_usec = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_end_monotonic_usec') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_end_monotonic_usec BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_end_monotonic_usec_unchanged');
PREPARE combat_add_end_monotonic_usec_stmt FROM @combat_add_end_monotonic_usec;
EXECUTE combat_add_end_monotonic_usec_stmt;
DEALLOCATE PREPARE combat_add_end_monotonic_usec_stmt;

SET @combat_add_start_utc_usec = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_start_utc_usec') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_start_utc_usec BIGINT NULL', 'SELECT 1 INTO @combat_add_start_utc_usec_unchanged');
PREPARE combat_add_start_utc_usec_stmt FROM @combat_add_start_utc_usec;
EXECUTE combat_add_start_utc_usec_stmt;
DEALLOCATE PREPARE combat_add_start_utc_usec_stmt;

SET @combat_add_end_utc_usec = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_end_utc_usec') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_end_utc_usec BIGINT NULL', 'SELECT 1 INTO @combat_add_end_utc_usec_unchanged');
PREPARE combat_add_end_utc_usec_stmt FROM @combat_add_end_utc_usec;
EXECUTE combat_add_end_utc_usec_stmt;
DEALLOCATE PREPARE combat_add_end_utc_usec_stmt;

SET @combat_add_damage_dealt = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_damage_dealt') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_damage_dealt BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_damage_dealt_unchanged');
PREPARE combat_add_damage_dealt_stmt FROM @combat_add_damage_dealt;
EXECUTE combat_add_damage_dealt_stmt;
DEALLOCATE PREPARE combat_add_damage_dealt_stmt;

SET @combat_add_damage_taken = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_damage_taken') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_damage_taken BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_damage_taken_unchanged');
PREPARE combat_add_damage_taken_stmt FROM @combat_add_damage_taken;
EXECUTE combat_add_damage_taken_stmt;
DEALLOCATE PREPARE combat_add_damage_taken_stmt;

SET @combat_add_healing_attempted = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_healing_attempted') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_healing_attempted BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_healing_attempted_unchanged');
PREPARE combat_add_healing_attempted_stmt FROM @combat_add_healing_attempted;
EXECUTE combat_add_healing_attempted_stmt;
DEALLOCATE PREPARE combat_add_healing_attempted_stmt;

SET @combat_add_effective_healing = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_effective_healing') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_effective_healing BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_effective_healing_unchanged');
PREPARE combat_add_effective_healing_stmt FROM @combat_add_effective_healing;
EXECUTE combat_add_effective_healing_stmt;
DEALLOCATE PREPARE combat_add_effective_healing_stmt;

SET @combat_add_overhealing = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_overhealing') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_overhealing BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_overhealing_unchanged');
PREPARE combat_add_overhealing_stmt FROM @combat_add_overhealing;
EXECUTE combat_add_overhealing_stmt;
DEALLOCATE PREPARE combat_add_overhealing_stmt;

SET @combat_add_control_applications = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_control_applications') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_control_applications BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_control_applications_unchanged');
PREPARE combat_add_control_applications_stmt FROM @combat_add_control_applications;
EXECUTE combat_add_control_applications_stmt;
DEALLOCATE PREPARE combat_add_control_applications_stmt;

SET @combat_add_casting_attempts = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_casting_attempts') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_casting_attempts BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_casting_attempts_unchanged');
PREPARE combat_add_casting_attempts_stmt FROM @combat_add_casting_attempts;
EXECUTE combat_add_casting_attempts_stmt;
DEALLOCATE PREPARE combat_add_casting_attempts_stmt;

SET @combat_add_casting_completions = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_casting_completions') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_casting_completions BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_casting_completions_unchanged');
PREPARE combat_add_casting_completions_stmt FROM @combat_add_casting_completions;
EXECUTE combat_add_casting_completions_stmt;
DEALLOCATE PREPARE combat_add_casting_completions_stmt;

SET @combat_add_casting_aborts = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_casting_aborts') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_casting_aborts BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_casting_aborts_unchanged');
PREPARE combat_add_casting_aborts_stmt FROM @combat_add_casting_aborts;
EXECUTE combat_add_casting_aborts_stmt;
DEALLOCATE PREPARE combat_add_casting_aborts_stmt;

SET @combat_add_casting_elapsed_usec = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_casting_elapsed_usec') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_casting_elapsed_usec BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_casting_elapsed_usec_unchanged');
PREPARE combat_add_casting_elapsed_usec_stmt FROM @combat_add_casting_elapsed_usec;
EXECUTE combat_add_casting_elapsed_usec_stmt;
DEALLOCATE PREPARE combat_add_casting_elapsed_usec_stmt;

SET @combat_add_tanking_usec = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_tanking_usec') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_tanking_usec BIGINT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_tanking_usec_unchanged');
PREPARE combat_add_tanking_usec_stmt FROM @combat_add_tanking_usec;
EXECUTE combat_add_tanking_usec_stmt;
DEALLOCATE PREPARE combat_add_tanking_usec_stmt;

SET @combat_add_quality_flags = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='combat_quality_flags') = 0, 'ALTER TABLE telemetry_interval ADD COLUMN combat_quality_flags INT UNSIGNED NULL', 'SELECT 1 INTO @combat_add_quality_flags_unchanged');
PREPARE combat_add_quality_flags_stmt FROM @combat_add_quality_flags;
EXECUTE combat_add_quality_flags_stmt;
DEALLOCATE PREPARE combat_add_quality_flags_stmt;

SET @combat_summary_index = IF((SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND index_name='uq_telemetry_combat_summary') = 0, 'ALTER TABLE telemetry_interval ADD UNIQUE KEY uq_telemetry_combat_summary (combat_encounter_boot_id,combat_encounter_process_id,combat_encounter_seq,combat_actor_kind,combat_actor_id,combat_actor_pid,combat_revision)', 'SELECT 1 INTO @combat_summary_index_unchanged');
PREPARE combat_summary_index_stmt FROM @combat_summary_index;
EXECUTE combat_summary_index_stmt;
DEALLOCATE PREPARE combat_summary_index_stmt;
