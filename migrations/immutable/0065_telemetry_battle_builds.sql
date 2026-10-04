-- Definition-1 selected battle build point observations, record family 12.
-- Earlier record fields, sealed reports and incident schemas retain their meaning.

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_battle_boot_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_battle_boot_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bc_end_reason');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_battle_process_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_battle_process_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bctx_battle_boot_id');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_battle_seq'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_battle_seq BIGINT UNSIGNED NULL DEFAULT NULL AFTER bctx_battle_process_id');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_environment_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_environment_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bctx_battle_seq');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_season_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_season_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bctx_environment_id');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_config_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_config_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bctx_season_id');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_actor_id'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_actor_id BIGINT UNSIGNED NULL DEFAULT NULL AFTER bctx_config_id');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_sequence'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_sequence BIGINT UNSIGNED NULL DEFAULT NULL AFTER bctx_actor_id');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_association_revision'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_association_revision BIGINT UNSIGNED NULL DEFAULT NULL AFTER bctx_sequence');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_at_monotonic_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_at_monotonic_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER bctx_association_revision');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_at_utc_usec'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_at_utc_usec BIGINT NULL DEFAULT NULL AFTER bctx_at_monotonic_usec');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_association_fact_sequence'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_association_fact_sequence INT UNSIGNED NULL DEFAULT NULL AFTER bctx_at_utc_usec');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_definition_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_definition_version SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bctx_association_fact_sequence');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_native_context_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_native_context_version SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bctx_definition_version');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_boundary'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_boundary TINYINT UNSIGNED NULL DEFAULT NULL AFTER bctx_native_context_version');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_status'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_status TINYINT UNSIGNED NULL DEFAULT NULL AFTER bctx_boundary');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_actor_kind'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_actor_kind TINYINT UNSIGNED NULL DEFAULT NULL AFTER bctx_status');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_build_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_build_version INT UNSIGNED NULL DEFAULT NULL AFTER bctx_actor_kind');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_content_version'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_content_version INT UNSIGNED NULL DEFAULT NULL AFTER bctx_build_version');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_available'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_available INT UNSIGNED NULL DEFAULT NULL AFTER bctx_content_version');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_context_quality'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_context_quality INT UNSIGNED NULL DEFAULT NULL AFTER bctx_available');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_quality_flags'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_quality_flags INT UNSIGNED NULL DEFAULT NULL AFTER bctx_context_quality');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_primary_class_mask'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_primary_class_mask INT UNSIGNED NULL DEFAULT NULL AFTER bctx_quality_flags');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_secondary_class_mask'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_secondary_class_mask INT UNSIGNED NULL DEFAULT NULL AFTER bctx_primary_class_mask');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_level'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_level SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bctx_secondary_class_mask');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_race'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_race SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bctx_level');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_faction'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_faction SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bctx_race');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_specialization'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_specialization TINYINT UNSIGNED NULL DEFAULT NULL AFTER bctx_faction');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_base_str'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_base_str SMALLINT NULL DEFAULT NULL AFTER bctx_specialization');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_base_dex'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_base_dex SMALLINT NULL DEFAULT NULL AFTER bctx_base_str');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_base_agi'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_base_agi SMALLINT NULL DEFAULT NULL AFTER bctx_base_dex');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_base_con'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_base_con SMALLINT NULL DEFAULT NULL AFTER bctx_base_agi');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_base_pow'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_base_pow SMALLINT NULL DEFAULT NULL AFTER bctx_base_con');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_base_int'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_base_int SMALLINT NULL DEFAULT NULL AFTER bctx_base_pow');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_base_wis'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_base_wis SMALLINT NULL DEFAULT NULL AFTER bctx_base_int');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_base_cha'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_base_cha SMALLINT NULL DEFAULT NULL AFTER bctx_base_wis');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_base_kar'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_base_kar SMALLINT NULL DEFAULT NULL AFTER bctx_base_cha');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_base_luk'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_base_luk SMALLINT NULL DEFAULT NULL AFTER bctx_base_kar');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_str'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_str SMALLINT NULL DEFAULT NULL AFTER bctx_base_luk');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_dex'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_dex SMALLINT NULL DEFAULT NULL AFTER bctx_effective_str');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_agi'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_agi SMALLINT NULL DEFAULT NULL AFTER bctx_effective_dex');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_con'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_con SMALLINT NULL DEFAULT NULL AFTER bctx_effective_agi');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_pow'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_pow SMALLINT NULL DEFAULT NULL AFTER bctx_effective_con');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_int'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_int SMALLINT NULL DEFAULT NULL AFTER bctx_effective_pow');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_wis'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_wis SMALLINT NULL DEFAULT NULL AFTER bctx_effective_int');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_cha'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_cha SMALLINT NULL DEFAULT NULL AFTER bctx_effective_wis');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_kar'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_kar SMALLINT NULL DEFAULT NULL AFTER bctx_effective_cha');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_luk'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_luk SMALLINT NULL DEFAULT NULL AFTER bctx_effective_kar');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_base_hit'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_base_hit INT NULL DEFAULT NULL AFTER bctx_effective_luk');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_base_mana'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_base_mana INT NULL DEFAULT NULL AFTER bctx_base_hit');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_base_vitality'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_base_vitality INT NULL DEFAULT NULL AFTER bctx_base_mana');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_base_ward'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_base_ward INT NULL DEFAULT NULL AFTER bctx_base_vitality');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_hit'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_hit INT NULL DEFAULT NULL AFTER bctx_base_ward');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_mana'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_mana INT NULL DEFAULT NULL AFTER bctx_effective_hit');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_vitality'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_vitality INT NULL DEFAULT NULL AFTER bctx_effective_mana');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_ward'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_ward INT NULL DEFAULT NULL AFTER bctx_effective_vitality');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_current_hit'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_current_hit INT NULL DEFAULT NULL AFTER bctx_effective_ward');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_current_mana'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_current_mana INT NULL DEFAULT NULL AFTER bctx_current_hit');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_current_vitality'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_current_vitality INT NULL DEFAULT NULL AFTER bctx_current_mana');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_current_ward'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_current_ward INT NULL DEFAULT NULL AFTER bctx_current_vitality');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_base_armor'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_base_armor INT NULL DEFAULT NULL AFTER bctx_current_ward');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_base_hitroll'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_base_hitroll INT NULL DEFAULT NULL AFTER bctx_base_armor');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_base_damroll'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_base_damroll INT NULL DEFAULT NULL AFTER bctx_base_hitroll');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_armor'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_armor INT NULL DEFAULT NULL AFTER bctx_base_damroll');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_hitroll'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_hitroll INT NULL DEFAULT NULL AFTER bctx_effective_armor');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_damroll'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_damroll INT NULL DEFAULT NULL AFTER bctx_effective_hitroll');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_saving_para'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_saving_para TINYINT NULL DEFAULT NULL AFTER bctx_effective_damroll');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_saving_rod'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_saving_rod TINYINT NULL DEFAULT NULL AFTER bctx_saving_para');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_saving_fear'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_saving_fear TINYINT NULL DEFAULT NULL AFTER bctx_saving_rod');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_saving_breath'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_saving_breath TINYINT NULL DEFAULT NULL AFTER bctx_saving_fear');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_saving_spell'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_saving_spell TINYINT NULL DEFAULT NULL AFTER bctx_saving_breath');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_flags_1'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_flags_1 BIGINT UNSIGNED NULL DEFAULT NULL AFTER bctx_saving_spell');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_flags_2'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_flags_2 BIGINT UNSIGNED NULL DEFAULT NULL AFTER bctx_effective_flags_1');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_flags_3'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_flags_3 BIGINT UNSIGNED NULL DEFAULT NULL AFTER bctx_effective_flags_2');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_flags_4'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_flags_4 BIGINT UNSIGNED NULL DEFAULT NULL AFTER bctx_effective_flags_3');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_effective_flags_5'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_effective_flags_5 BIGINT UNSIGNED NULL DEFAULT NULL AFTER bctx_effective_flags_4');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_equipment_flags_1'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_equipment_flags_1 BIGINT UNSIGNED NULL DEFAULT NULL AFTER bctx_effective_flags_5');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_equipment_flags_2'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_equipment_flags_2 BIGINT UNSIGNED NULL DEFAULT NULL AFTER bctx_equipment_flags_1');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_equipment_flags_3'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_equipment_flags_3 BIGINT UNSIGNED NULL DEFAULT NULL AFTER bctx_equipment_flags_2');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_equipment_flags_4'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_equipment_flags_4 BIGINT UNSIGNED NULL DEFAULT NULL AFTER bctx_equipment_flags_3');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_equipment_flags_5'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_equipment_flags_5 BIGINT UNSIGNED NULL DEFAULT NULL AFTER bctx_equipment_flags_4');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_equipment_hit'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_equipment_hit INT NULL DEFAULT NULL AFTER bctx_equipment_flags_5');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_equipment_mana'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_equipment_mana INT NULL DEFAULT NULL AFTER bctx_equipment_hit');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_equipment_armor'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_equipment_armor INT NULL DEFAULT NULL AFTER bctx_equipment_mana');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_equipment_hitroll'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_equipment_hitroll INT NULL DEFAULT NULL AFTER bctx_equipment_armor');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_equipment_damroll'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_equipment_damroll INT NULL DEFAULT NULL AFTER bctx_equipment_hitroll');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_equipment_occupied_slots_count'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_equipment_occupied_slots_count TINYINT UNSIGNED NULL DEFAULT NULL AFTER bctx_equipment_damroll');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_equipment_melee_weapons_count'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_equipment_melee_weapons_count TINYINT UNSIGNED NULL DEFAULT NULL AFTER bctx_equipment_occupied_slots_count');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_equipment_ranged_weapons_count'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_equipment_ranged_weapons_count TINYINT UNSIGNED NULL DEFAULT NULL AFTER bctx_equipment_melee_weapons_count');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_equipment_shields_count'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_equipment_shields_count TINYINT UNSIGNED NULL DEFAULT NULL AFTER bctx_equipment_ranged_weapons_count');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_equipment_armor_count'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_equipment_armor_count TINYINT UNSIGNED NULL DEFAULT NULL AFTER bctx_equipment_shields_count');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_equipment_other_items_count'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_equipment_other_items_count TINYINT UNSIGNED NULL DEFAULT NULL AFTER bctx_equipment_armor_count');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_equipment_dynamic_affects_count'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_equipment_dynamic_affects_count TINYINT UNSIGNED NULL DEFAULT NULL AFTER bctx_equipment_other_items_count');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_equipment_digest'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_equipment_digest BINARY(32) NULL DEFAULT NULL AFTER bctx_equipment_dynamic_affects_count');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_epic_catalog_skills'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_epic_catalog_skills SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bctx_equipment_digest');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_epic_learned_skills'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_epic_learned_skills SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bctx_epic_catalog_skills');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_epic_digest'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_epic_digest BINARY(32) NULL DEFAULT NULL AFTER bctx_epic_learned_skills');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_affect_nodes'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_affect_nodes SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bctx_epic_digest');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_offensive_modifier_nodes'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_offensive_modifier_nodes SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bctx_affect_nodes');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_armor_modifier_nodes'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_armor_modifier_nodes SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bctx_offensive_modifier_nodes');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_resource_modifier_nodes'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_resource_modifier_nodes SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bctx_armor_modifier_nodes');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_unapplied_nodes'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_unapplied_nodes SMALLINT UNSIGNED NULL DEFAULT NULL AFTER bctx_resource_modifier_nodes');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_affects_complete'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_affects_complete TINYINT UNSIGNED NULL DEFAULT NULL AFTER bctx_unapplied_nodes');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_arena_membership'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_arena_membership TINYINT UNSIGNED NULL DEFAULT NULL AFTER bctx_affects_complete');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_arena_room'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_arena_room TINYINT UNSIGNED NULL DEFAULT NULL AFTER bctx_arena_membership');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_arena_enabled'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_arena_enabled TINYINT UNSIGNED NULL DEFAULT NULL AFTER bctx_arena_room');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_arena_type'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_arena_type TINYINT UNSIGNED NULL DEFAULT NULL AFTER bctx_arena_enabled');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_arena_stage'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_arena_stage TINYINT UNSIGNED NULL DEFAULT NULL AFTER bctx_arena_type');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_arena_team'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_arena_team TINYINT UNSIGNED NULL DEFAULT NULL AFTER bctx_arena_stage');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name='bctx_arena_player_flags'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN bctx_arena_player_flags INT NULL DEFAULT NULL AFTER bctx_arena_team');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND index_name='uq_telemetry_battle_build'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD UNIQUE KEY uq_telemetry_battle_build (bctx_battle_boot_id,bctx_battle_process_id,bctx_sequence)');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_interval' AND constraint_name='chk_telemetry_bctx_payload'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD CONSTRAINT chk_telemetry_bctx_payload CHECK ((record_kind=12 AND bctx_battle_boot_id IS NOT NULL AND bctx_battle_process_id IS NOT NULL AND bctx_battle_seq IS NOT NULL AND bctx_environment_id IS NOT NULL AND bctx_season_id IS NOT NULL AND bctx_config_id IS NOT NULL AND bctx_actor_id IS NOT NULL AND bctx_sequence IS NOT NULL AND bctx_association_revision IS NOT NULL AND bctx_at_monotonic_usec IS NOT NULL AND bctx_at_utc_usec IS NOT NULL AND bctx_association_fact_sequence IS NOT NULL AND bctx_definition_version IS NOT NULL AND bctx_native_context_version IS NOT NULL AND bctx_boundary IS NOT NULL AND bctx_status IS NOT NULL AND bctx_actor_kind IS NOT NULL AND bctx_build_version IS NOT NULL AND bctx_content_version IS NOT NULL AND bctx_available IS NOT NULL AND bctx_context_quality IS NOT NULL AND bctx_quality_flags IS NOT NULL AND bctx_primary_class_mask IS NOT NULL AND bctx_secondary_class_mask IS NOT NULL AND bctx_level IS NOT NULL AND bctx_race IS NOT NULL AND bctx_faction IS NOT NULL AND bctx_specialization IS NOT NULL AND bctx_base_str IS NOT NULL AND bctx_base_dex IS NOT NULL AND bctx_base_agi IS NOT NULL AND bctx_base_con IS NOT NULL AND bctx_base_pow IS NOT NULL AND bctx_base_int IS NOT NULL AND bctx_base_wis IS NOT NULL AND bctx_base_cha IS NOT NULL AND bctx_base_kar IS NOT NULL AND bctx_base_luk IS NOT NULL AND bctx_effective_str IS NOT NULL AND bctx_effective_dex IS NOT NULL AND bctx_effective_agi IS NOT NULL AND bctx_effective_con IS NOT NULL AND bctx_effective_pow IS NOT NULL AND bctx_effective_int IS NOT NULL AND bctx_effective_wis IS NOT NULL AND bctx_effective_cha IS NOT NULL AND bctx_effective_kar IS NOT NULL AND bctx_effective_luk IS NOT NULL AND bctx_base_hit IS NOT NULL AND bctx_base_mana IS NOT NULL AND bctx_base_vitality IS NOT NULL AND bctx_base_ward IS NOT NULL AND bctx_effective_hit IS NOT NULL AND bctx_effective_mana IS NOT NULL AND bctx_effective_vitality IS NOT NULL AND bctx_effective_ward IS NOT NULL AND bctx_current_hit IS NOT NULL AND bctx_current_mana IS NOT NULL AND bctx_current_vitality IS NOT NULL AND bctx_current_ward IS NOT NULL AND bctx_base_armor IS NOT NULL AND bctx_base_hitroll IS NOT NULL AND bctx_base_damroll IS NOT NULL AND bctx_effective_armor IS NOT NULL AND bctx_effective_hitroll IS NOT NULL AND bctx_effective_damroll IS NOT NULL AND bctx_saving_para IS NOT NULL AND bctx_saving_rod IS NOT NULL AND bctx_saving_fear IS NOT NULL AND bctx_saving_breath IS NOT NULL AND bctx_saving_spell IS NOT NULL AND bctx_effective_flags_1 IS NOT NULL AND bctx_effective_flags_2 IS NOT NULL AND bctx_effective_flags_3 IS NOT NULL AND bctx_effective_flags_4 IS NOT NULL AND bctx_effective_flags_5 IS NOT NULL AND bctx_equipment_flags_1 IS NOT NULL AND bctx_equipment_flags_2 IS NOT NULL AND bctx_equipment_flags_3 IS NOT NULL AND bctx_equipment_flags_4 IS NOT NULL AND bctx_equipment_flags_5 IS NOT NULL AND bctx_equipment_hit IS NOT NULL AND bctx_equipment_mana IS NOT NULL AND bctx_equipment_armor IS NOT NULL AND bctx_equipment_hitroll IS NOT NULL AND bctx_equipment_damroll IS NOT NULL AND bctx_equipment_occupied_slots_count IS NOT NULL AND bctx_equipment_melee_weapons_count IS NOT NULL AND bctx_equipment_ranged_weapons_count IS NOT NULL AND bctx_equipment_shields_count IS NOT NULL AND bctx_equipment_armor_count IS NOT NULL AND bctx_equipment_other_items_count IS NOT NULL AND bctx_equipment_dynamic_affects_count IS NOT NULL AND bctx_equipment_digest IS NOT NULL AND bctx_epic_catalog_skills IS NOT NULL AND bctx_epic_learned_skills IS NOT NULL AND bctx_epic_digest IS NOT NULL AND bctx_affect_nodes IS NOT NULL AND bctx_offensive_modifier_nodes IS NOT NULL AND bctx_armor_modifier_nodes IS NOT NULL AND bctx_resource_modifier_nodes IS NOT NULL AND bctx_unapplied_nodes IS NOT NULL AND bctx_affects_complete IS NOT NULL AND bctx_arena_membership IS NOT NULL AND bctx_arena_room IS NOT NULL AND bctx_arena_enabled IS NOT NULL AND bctx_arena_type IS NOT NULL AND bctx_arena_stage IS NOT NULL AND bctx_arena_team IS NOT NULL AND bctx_arena_player_flags IS NOT NULL) OR (record_kind<>12 AND bctx_battle_boot_id IS NULL AND bctx_battle_process_id IS NULL AND bctx_battle_seq IS NULL AND bctx_environment_id IS NULL AND bctx_season_id IS NULL AND bctx_config_id IS NULL AND bctx_actor_id IS NULL AND bctx_sequence IS NULL AND bctx_association_revision IS NULL AND bctx_at_monotonic_usec IS NULL AND bctx_at_utc_usec IS NULL AND bctx_association_fact_sequence IS NULL AND bctx_definition_version IS NULL AND bctx_native_context_version IS NULL AND bctx_boundary IS NULL AND bctx_status IS NULL AND bctx_actor_kind IS NULL AND bctx_build_version IS NULL AND bctx_content_version IS NULL AND bctx_available IS NULL AND bctx_context_quality IS NULL AND bctx_quality_flags IS NULL AND bctx_primary_class_mask IS NULL AND bctx_secondary_class_mask IS NULL AND bctx_level IS NULL AND bctx_race IS NULL AND bctx_faction IS NULL AND bctx_specialization IS NULL AND bctx_base_str IS NULL AND bctx_base_dex IS NULL AND bctx_base_agi IS NULL AND bctx_base_con IS NULL AND bctx_base_pow IS NULL AND bctx_base_int IS NULL AND bctx_base_wis IS NULL AND bctx_base_cha IS NULL AND bctx_base_kar IS NULL AND bctx_base_luk IS NULL AND bctx_effective_str IS NULL AND bctx_effective_dex IS NULL AND bctx_effective_agi IS NULL AND bctx_effective_con IS NULL AND bctx_effective_pow IS NULL AND bctx_effective_int IS NULL AND bctx_effective_wis IS NULL AND bctx_effective_cha IS NULL AND bctx_effective_kar IS NULL AND bctx_effective_luk IS NULL AND bctx_base_hit IS NULL AND bctx_base_mana IS NULL AND bctx_base_vitality IS NULL AND bctx_base_ward IS NULL AND bctx_effective_hit IS NULL AND bctx_effective_mana IS NULL AND bctx_effective_vitality IS NULL AND bctx_effective_ward IS NULL AND bctx_current_hit IS NULL AND bctx_current_mana IS NULL AND bctx_current_vitality IS NULL AND bctx_current_ward IS NULL AND bctx_base_armor IS NULL AND bctx_base_hitroll IS NULL AND bctx_base_damroll IS NULL AND bctx_effective_armor IS NULL AND bctx_effective_hitroll IS NULL AND bctx_effective_damroll IS NULL AND bctx_saving_para IS NULL AND bctx_saving_rod IS NULL AND bctx_saving_fear IS NULL AND bctx_saving_breath IS NULL AND bctx_saving_spell IS NULL AND bctx_effective_flags_1 IS NULL AND bctx_effective_flags_2 IS NULL AND bctx_effective_flags_3 IS NULL AND bctx_effective_flags_4 IS NULL AND bctx_effective_flags_5 IS NULL AND bctx_equipment_flags_1 IS NULL AND bctx_equipment_flags_2 IS NULL AND bctx_equipment_flags_3 IS NULL AND bctx_equipment_flags_4 IS NULL AND bctx_equipment_flags_5 IS NULL AND bctx_equipment_hit IS NULL AND bctx_equipment_mana IS NULL AND bctx_equipment_armor IS NULL AND bctx_equipment_hitroll IS NULL AND bctx_equipment_damroll IS NULL AND bctx_equipment_occupied_slots_count IS NULL AND bctx_equipment_melee_weapons_count IS NULL AND bctx_equipment_ranged_weapons_count IS NULL AND bctx_equipment_shields_count IS NULL AND bctx_equipment_armor_count IS NULL AND bctx_equipment_other_items_count IS NULL AND bctx_equipment_dynamic_affects_count IS NULL AND bctx_equipment_digest IS NULL AND bctx_epic_catalog_skills IS NULL AND bctx_epic_learned_skills IS NULL AND bctx_epic_digest IS NULL AND bctx_affect_nodes IS NULL AND bctx_offensive_modifier_nodes IS NULL AND bctx_armor_modifier_nodes IS NULL AND bctx_resource_modifier_nodes IS NULL AND bctx_unapplied_nodes IS NULL AND bctx_affects_complete IS NULL AND bctx_arena_membership IS NULL AND bctx_arena_room IS NULL AND bctx_arena_enabled IS NULL AND bctx_arena_type IS NULL AND bctx_arena_stage IS NULL AND bctx_arena_team IS NULL AND bctx_arena_player_flags IS NULL))');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_interval' AND constraint_name='chk_telemetry_bctx_binding'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD CONSTRAINT chk_telemetry_bctx_binding CHECK (record_kind<>12 OR (bctx_battle_boot_id=boot_id AND bctx_battle_process_id=process_id AND bctx_battle_boot_id>0 AND bctx_battle_process_id>0 AND bctx_battle_seq>0 AND bctx_sequence>0 AND bctx_at_utc_usec=occurrence_utc_usec AND bctx_environment_id>0 AND bctx_season_id>0 AND bctx_association_revision>0 AND bctx_association_fact_sequence>0 AND bctx_definition_version=1 AND bctx_native_context_version=1 AND bctx_available<=1023 AND bctx_context_quality<=255 AND bctx_quality_flags<=1023 AND bctx_boundary BETWEEN 1 AND 8 AND ((bctx_actor_kind=1 AND bctx_actor_id BETWEEN 1 AND 2147483647) OR (bctx_actor_kind IN (2,3) AND (bctx_actor_id & 9223372036854775808)<>0 AND (bctx_actor_id & 9223372036854775807)>0)) AND ((bctx_boundary=8 AND bctx_config_id=0 AND bctx_build_version=0 AND bctx_content_version=0) OR (bctx_boundary<>8 AND bctx_config_id>0 AND bctx_build_version>0 AND bctx_content_version>0))))');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_interval' AND constraint_name='chk_telemetry_bctx_status'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD CONSTRAINT chk_telemetry_bctx_status CHECK (record_kind<>12 OR ((bctx_status=1 AND bctx_available>0 AND bctx_boundary BETWEEN 1 AND 5 AND (bctx_context_quality & 128)<>0) OR (bctx_status=2 AND bctx_boundary BETWEEN 6 AND 8 AND (bctx_quality_flags & 1)<>0 AND (bctx_available=0 AND bctx_primary_class_mask=0 AND bctx_secondary_class_mask=0 AND bctx_level=0 AND bctx_race=0 AND bctx_faction=0 AND bctx_specialization=0 AND bctx_base_str=0 AND bctx_base_dex=0 AND bctx_base_agi=0 AND bctx_base_con=0 AND bctx_base_pow=0 AND bctx_base_int=0 AND bctx_base_wis=0 AND bctx_base_cha=0 AND bctx_base_kar=0 AND bctx_base_luk=0 AND bctx_effective_str=0 AND bctx_effective_dex=0 AND bctx_effective_agi=0 AND bctx_effective_con=0 AND bctx_effective_pow=0 AND bctx_effective_int=0 AND bctx_effective_wis=0 AND bctx_effective_cha=0 AND bctx_effective_kar=0 AND bctx_effective_luk=0 AND bctx_base_hit=0 AND bctx_base_mana=0 AND bctx_base_vitality=0 AND bctx_base_ward=0 AND bctx_effective_hit=0 AND bctx_effective_mana=0 AND bctx_effective_vitality=0 AND bctx_effective_ward=0 AND bctx_current_hit=0 AND bctx_current_mana=0 AND bctx_current_vitality=0 AND bctx_current_ward=0 AND bctx_base_armor=0 AND bctx_base_hitroll=0 AND bctx_base_damroll=0 AND bctx_effective_armor=0 AND bctx_effective_hitroll=0 AND bctx_effective_damroll=0 AND bctx_saving_para=0 AND bctx_saving_rod=0 AND bctx_saving_fear=0 AND bctx_saving_breath=0 AND bctx_saving_spell=0 AND bctx_effective_flags_1=0 AND bctx_effective_flags_2=0 AND bctx_effective_flags_3=0 AND bctx_effective_flags_4=0 AND bctx_effective_flags_5=0 AND bctx_equipment_flags_1=0 AND bctx_equipment_flags_2=0 AND bctx_equipment_flags_3=0 AND bctx_equipment_flags_4=0 AND bctx_equipment_flags_5=0 AND bctx_equipment_hit=0 AND bctx_equipment_mana=0 AND bctx_equipment_armor=0 AND bctx_equipment_hitroll=0 AND bctx_equipment_damroll=0 AND bctx_equipment_occupied_slots_count=0 AND bctx_equipment_melee_weapons_count=0 AND bctx_equipment_ranged_weapons_count=0 AND bctx_equipment_shields_count=0 AND bctx_equipment_armor_count=0 AND bctx_equipment_other_items_count=0 AND bctx_equipment_dynamic_affects_count=0 AND bctx_equipment_digest=UNHEX(REPEAT(CHAR(48),64)) AND bctx_epic_catalog_skills=0 AND bctx_epic_learned_skills=0 AND bctx_epic_digest=UNHEX(REPEAT(CHAR(48),64)) AND bctx_affect_nodes=0 AND bctx_offensive_modifier_nodes=0 AND bctx_armor_modifier_nodes=0 AND bctx_resource_modifier_nodes=0 AND bctx_unapplied_nodes=0 AND bctx_affects_complete=0 AND bctx_arena_membership=0 AND bctx_arena_room=0 AND bctx_arena_enabled=0 AND bctx_arena_type=0 AND bctx_arena_stage=0 AND bctx_arena_team=0 AND bctx_arena_player_flags=0))))');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_interval' AND constraint_name='chk_telemetry_bctx_values'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD CONSTRAINT chk_telemetry_bctx_values CHECK (record_kind<>12 OR (((bctx_available & 1)<>0 OR (bctx_base_str=0 AND bctx_base_dex=0 AND bctx_base_agi=0 AND bctx_base_con=0 AND bctx_base_pow=0 AND bctx_base_int=0 AND bctx_base_wis=0 AND bctx_base_cha=0 AND bctx_base_kar=0 AND bctx_base_luk=0 AND bctx_base_hit=0 AND bctx_base_mana=0 AND bctx_base_vitality=0 AND bctx_base_ward=0 AND bctx_base_armor=0 AND bctx_base_hitroll=0 AND bctx_base_damroll=0)) AND ((bctx_available & 2)<>0 OR (bctx_effective_str=0 AND bctx_effective_dex=0 AND bctx_effective_agi=0 AND bctx_effective_con=0 AND bctx_effective_pow=0 AND bctx_effective_int=0 AND bctx_effective_wis=0 AND bctx_effective_cha=0 AND bctx_effective_kar=0 AND bctx_effective_luk=0 AND bctx_effective_hit=0 AND bctx_effective_mana=0 AND bctx_effective_vitality=0 AND bctx_effective_ward=0 AND bctx_effective_armor=0 AND bctx_effective_hitroll=0 AND bctx_effective_damroll=0)) AND ((bctx_available & 4)<>0 OR (bctx_current_hit=0 AND bctx_current_mana=0 AND bctx_current_vitality=0 AND bctx_current_ward=0)) AND ((bctx_available & 8)<>0 OR (bctx_saving_para=0 AND bctx_saving_rod=0 AND bctx_saving_fear=0 AND bctx_saving_breath=0 AND bctx_saving_spell=0)) AND ((bctx_available & 16)<>0 OR (bctx_effective_flags_1=0 AND bctx_effective_flags_2=0 AND bctx_effective_flags_3=0 AND bctx_effective_flags_4=0 AND bctx_effective_flags_5=0))))');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_interval' AND constraint_name='chk_telemetry_bctx_equipment'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD CONSTRAINT chk_telemetry_bctx_equipment CHECK (record_kind<>12 OR (((bctx_available & 32)=0 AND (bctx_equipment_flags_1=0 AND bctx_equipment_flags_2=0 AND bctx_equipment_flags_3=0 AND bctx_equipment_flags_4=0 AND bctx_equipment_flags_5=0 AND bctx_equipment_hit=0 AND bctx_equipment_mana=0 AND bctx_equipment_armor=0 AND bctx_equipment_hitroll=0 AND bctx_equipment_damroll=0 AND bctx_equipment_occupied_slots_count=0 AND bctx_equipment_melee_weapons_count=0 AND bctx_equipment_ranged_weapons_count=0 AND bctx_equipment_shields_count=0 AND bctx_equipment_armor_count=0 AND bctx_equipment_other_items_count=0 AND bctx_equipment_dynamic_affects_count=0 AND bctx_equipment_digest=UNHEX(REPEAT(CHAR(48),64)))) OR ((bctx_available & 32)<>0 AND bctx_equipment_occupied_slots_count<=43 AND bctx_equipment_dynamic_affects_count<=bctx_equipment_occupied_slots_count AND bctx_equipment_melee_weapons_count+bctx_equipment_ranged_weapons_count+bctx_equipment_shields_count+bctx_equipment_armor_count+bctx_equipment_other_items_count=bctx_equipment_occupied_slots_count AND bctx_equipment_digest<>UNHEX(REPEAT(CHAR(48),64)) AND (bctx_context_quality & 2)=0)))');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_interval' AND constraint_name='chk_telemetry_bctx_epics'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD CONSTRAINT chk_telemetry_bctx_epics CHECK (record_kind<>12 OR (((bctx_available & 64)=0 AND (bctx_epic_catalog_skills=0 AND bctx_epic_learned_skills=0 AND bctx_epic_digest=UNHEX(REPEAT(CHAR(48),64)))) OR ((bctx_available & 64)<>0 AND bctx_actor_kind=1 AND bctx_epic_catalog_skills BETWEEN 1 AND 309 AND bctx_epic_learned_skills<=bctx_epic_catalog_skills AND bctx_epic_digest<>UNHEX(REPEAT(CHAR(48),64)) AND (bctx_context_quality & 1)=0)))');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_interval' AND constraint_name='chk_telemetry_bctx_affects'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD CONSTRAINT chk_telemetry_bctx_affects CHECK (record_kind<>12 OR (((bctx_available & 128)=0 AND (bctx_affect_nodes=0 AND bctx_offensive_modifier_nodes=0 AND bctx_armor_modifier_nodes=0 AND bctx_resource_modifier_nodes=0 AND bctx_unapplied_nodes=0 AND bctx_affects_complete=0)) OR ((bctx_available & 128)<>0 AND bctx_affect_nodes<=64 AND bctx_unapplied_nodes<=bctx_affect_nodes AND bctx_offensive_modifier_nodes+bctx_armor_modifier_nodes+bctx_resource_modifier_nodes+bctx_unapplied_nodes<=bctx_affect_nodes AND ((bctx_affects_complete=1 AND (bctx_context_quality & 12)=0) OR (bctx_affects_complete=0 AND (bctx_context_quality & 12)<>0)) AND ((bctx_context_quality & 4)=0 OR bctx_affect_nodes=64) AND ((bctx_context_quality & 8)=0 OR bctx_affect_nodes>0))))');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_interval' AND constraint_name='chk_telemetry_bctx_arena'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD CONSTRAINT chk_telemetry_bctx_arena CHECK (record_kind<>12 OR (bctx_arena_room<=1 AND ((bctx_available & 256)<>0 OR bctx_arena_room=0) AND ((bctx_available & 256)=0 OR (bctx_context_quality & 64)=0) AND bctx_arena_enabled<=1 AND bctx_arena_type<=5 AND bctx_arena_stage<=5 AND (((bctx_available & 512)<>0 AND (bctx_context_quality & 48)=0 AND ((bctx_arena_membership=1 AND bctx_arena_team=0 AND bctx_arena_player_flags=0) OR (bctx_arena_membership=2 AND bctx_actor_kind=1 AND bctx_arena_team BETWEEN 1 AND 3))) OR ((bctx_available & 512)=0 AND (bctx_arena_membership=0 OR (bctx_arena_membership=3 AND (bctx_context_quality & 32)<>0)) AND bctx_arena_team=0 AND bctx_arena_player_flags=0))))');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

SET @telemetry_build_sql = IF(EXISTS(SELECT 1 FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_interval' AND constraint_name='chk_telemetry_bctx_inactive'), 'SELECT 1', 'ALTER TABLE telemetry_interval ADD CONSTRAINT chk_telemetry_bctx_inactive CHECK (record_kind<>12 OR (environment_id IS NULL AND season_id IS NULL AND session_boot_id IS NULL AND session_process_id IS NULL AND session_seq IS NULL AND subject_id IS NULL AND pid IS NULL AND connection_boot_id IS NULL AND connection_process_id IS NULL AND connection_seq IS NULL AND start_monotonic_usec IS NULL AND end_monotonic_usec IS NULL AND start_utc_usec IS NULL AND end_utc_usec IS NULL AND duration_usec IS NULL AND category IS NULL AND context IS NULL AND context_quality IS NULL AND level_band IS NULL AND class_id IS NULL AND race_id IS NULL AND faction_id IS NULL AND zone_vnum IS NULL AND group_size IS NULL AND config_id IS NULL AND classifier_version IS NULL AND policy_version IS NULL AND quality_flags IS NULL AND lifecycle IS NULL AND end_reason IS NULL AND at_monotonic_usec IS NULL AND at_utc_usec IS NULL AND checkpoint_revision IS NULL AND connected_usec IS NULL AND active_usec IS NULL AND idle_usec IS NULL AND unknown_usec IS NULL AND resident_usec IS NULL AND linkdead_usec IS NULL AND gap_reason IS NULL AND first_missing_record_seq IS NULL AND last_missing_record_seq IS NULL AND dropped_records IS NULL AND config_revision IS NULL AND build_version IS NULL AND content_version IS NULL AND property_version IS NULL AND fingerprint IS NULL AND effective_utc_usec IS NULL AND interval_usec IS NULL AND checkpoint_interval_usec IS NULL AND active_window_usec IS NULL AND context_segments_per_minute IS NULL AND pulse_slot_count IS NULL AND backend IS NULL AND enabled IS NULL AND progression_kind IS NULL AND progression_source IS NULL AND progression_reason IS NULL AND progression_observation_status IS NULL AND progression_modifier_flags IS NULL AND progression_requested_xp IS NULL AND progression_computed_xp IS NULL AND progression_applied_xp IS NULL AND progression_before_exp IS NULL AND progression_after_exp IS NULL AND progression_before_level IS NULL AND progression_after_level IS NULL AND progression_threshold_xp IS NULL AND encounter_boot_id IS NULL AND encounter_process_id IS NULL AND encounter_seq IS NULL AND encounter_event IS NULL AND encounter_mode IS NULL AND encounter_outcome IS NULL AND encounter_revision IS NULL AND encounter_environment_id IS NULL AND encounter_season_id IS NULL AND encounter_config_id IS NULL AND encounter_classifier_version IS NULL AND encounter_policy_version IS NULL AND encounter_zone_vnum IS NULL AND encounter_group_key IS NULL AND encounter_participant_subject_id IS NULL AND encounter_participant_pid IS NULL AND encounter_start_monotonic_usec IS NULL AND encounter_start_utc_usec IS NULL AND elapsed_usec IS NULL AND participant_usec IS NULL AND participant_count IS NULL AND expected_credit_count IS NULL AND encounter_quality_flags IS NULL AND combat_encounter_boot_id IS NULL AND combat_encounter_process_id IS NULL AND combat_encounter_seq IS NULL AND combat_mode IS NULL AND combat_outcome IS NULL AND combat_revision IS NULL AND combat_environment_id IS NULL AND combat_season_id IS NULL AND combat_config_id IS NULL AND combat_classifier_version IS NULL AND combat_policy_version IS NULL AND combat_zone_vnum IS NULL AND combat_group_key IS NULL AND combat_actor_id IS NULL AND combat_actor_pid IS NULL AND combat_owner_subject_id IS NULL AND combat_actor_kind IS NULL AND combat_unique_player_count IS NULL AND combat_participant_count IS NULL AND combat_dropped_participant_count IS NULL AND combat_power_band IS NULL AND combat_opponent_power_band IS NULL AND combat_opponent_count IS NULL AND combat_modifier_flags IS NULL AND combat_start_monotonic_usec IS NULL AND combat_end_monotonic_usec IS NULL AND combat_start_utc_usec IS NULL AND combat_end_utc_usec IS NULL AND combat_damage_dealt IS NULL AND combat_damage_taken IS NULL AND combat_healing_attempted IS NULL AND combat_effective_healing IS NULL AND combat_overhealing IS NULL AND combat_control_applications IS NULL AND combat_casting_attempts IS NULL AND combat_casting_completions IS NULL AND combat_casting_aborts IS NULL AND combat_casting_elapsed_usec IS NULL AND combat_tanking_usec IS NULL AND combat_quality_flags IS NULL AND ownership_account_token IS NULL AND ownership_source IS NULL AND battle_boot_id IS NULL AND battle_process_id IS NULL AND battle_seq IS NULL AND battle_related_boot_id IS NULL AND battle_related_process_id IS NULL AND battle_related_seq IS NULL AND battle_environment_id IS NULL AND battle_season_id IS NULL AND battle_config_id IS NULL AND battle_classifier_version IS NULL AND battle_policy_version IS NULL AND battle_scope_zone_vnum IS NULL AND battle_scope_group_key IS NULL AND battle_revision IS NULL AND battle_fact_sequence IS NULL AND battle_fact_index IS NULL AND battle_fact_count IS NULL AND battle_definition_version IS NULL AND battle_fact_kind IS NULL AND battle_relation IS NULL AND battle_side_status IS NULL AND battle_mode IS NULL AND battle_close_reason IS NULL AND battle_actor_active IS NULL AND battle_actor_side IS NULL AND battle_end_censored IS NULL AND battle_actor_roles IS NULL AND battle_actor_count IS NULL AND battle_active_actor_count IS NULL AND battle_observed_owner_count IS NULL AND battle_dropped_actor_count IS NULL AND battle_actor_id IS NULL AND battle_actor_pid IS NULL AND battle_actor_owner_subject_id IS NULL AND battle_actor_kind IS NULL AND battle_actor_power_band IS NULL AND battle_actor_encounter_boot_id IS NULL AND battle_actor_encounter_process_id IS NULL AND battle_actor_encounter_seq IS NULL AND battle_actor_session_boot_id IS NULL AND battle_actor_session_process_id IS NULL AND battle_actor_session_seq IS NULL AND battle_actor_level_band IS NULL AND battle_actor_class_id IS NULL AND battle_actor_race_id IS NULL AND battle_actor_faction_id IS NULL AND battle_actor_zone_vnum IS NULL AND battle_actor_group_size IS NULL AND battle_actor_group_key IS NULL AND battle_actor_group_revision IS NULL AND battle_actor_context_version IS NULL AND battle_actor_quality_flags IS NULL AND battle_related_actor_id IS NULL AND battle_related_actor_kind IS NULL AND battle_present_usec IS NULL AND battle_contributor_usec IS NULL AND battle_pve_usec IS NULL AND battle_pvp_usec IS NULL AND battle_mixed_usec IS NULL AND battle_unknown_mode_usec IS NULL AND battle_outnumbered_owner_usec IS NULL AND battle_unknown_side_usec IS NULL AND battle_start_monotonic_usec IS NULL AND battle_at_monotonic_usec IS NULL AND battle_observed_through_monotonic_usec IS NULL AND battle_last_engagement_monotonic_usec IS NULL AND battle_inactivity_grace_usec IS NULL AND battle_at_utc_usec IS NULL AND battle_observed_through_utc_usec IS NULL AND battle_quality_flags IS NULL AND bc_battle_boot_id IS NULL AND bc_battle_process_id IS NULL AND bc_battle_seq IS NULL AND bc_environment_id IS NULL AND bc_season_id IS NULL AND bc_config_id IS NULL AND bc_classifier_version IS NULL AND bc_policy_version IS NULL AND bc_scope_zone_vnum IS NULL AND bc_scope_group_key IS NULL AND bc_actor_id IS NULL AND bc_actor_pid IS NULL AND bc_actor_owner_subject_id IS NULL AND bc_actor_kind IS NULL AND bc_actor_power_band IS NULL AND bc_actor_encounter_boot_id IS NULL AND bc_actor_encounter_process_id IS NULL AND bc_actor_encounter_seq IS NULL AND bc_actor_session_boot_id IS NULL AND bc_actor_session_process_id IS NULL AND bc_actor_session_seq IS NULL AND bc_actor_level_band IS NULL AND bc_actor_class_id IS NULL AND bc_actor_race_id IS NULL AND bc_actor_faction_id IS NULL AND bc_actor_zone_vnum IS NULL AND bc_actor_group_size IS NULL AND bc_actor_group_key IS NULL AND bc_actor_group_revision IS NULL AND bc_actor_context_version IS NULL AND bc_actor_quality_flags IS NULL AND bc_first_association_revision IS NULL AND bc_first_association_fact_sequence IS NULL AND bc_available_metrics IS NULL AND bc_side_status IS NULL AND bc_mode IS NULL AND bc_actor_side IS NULL AND bc_context_quality_flags IS NULL AND bc_segment_seq IS NULL AND bc_last_association_revision IS NULL AND bc_last_association_fact_sequence IS NULL AND bc_modifier_flags IS NULL AND bc_start_monotonic_usec IS NULL AND bc_start_utc_usec IS NULL AND bc_observed_through_monotonic_usec IS NULL AND bc_observed_through_utc_usec IS NULL AND bc_decision_monotonic_usec IS NULL AND bc_decision_utc_usec IS NULL AND bc_damage_dealt IS NULL AND bc_damage_taken IS NULL AND bc_healing_attempted IS NULL AND bc_effective_healing IS NULL AND bc_overhealing IS NULL AND bc_healing_received IS NULL AND bc_control_applications IS NULL AND bc_control_received IS NULL AND bc_casting_attempts IS NULL AND bc_casting_completions IS NULL AND bc_casting_aborts IS NULL AND bc_casting_unresolved IS NULL AND bc_casting_elapsed_usec IS NULL AND bc_engaged_target_usec IS NULL AND bc_quality_flags IS NULL AND bc_definition_version IS NULL AND bc_end_reason IS NULL))');
PREPARE telemetry_build_statement FROM @telemetry_build_sql;
EXECUTE telemetry_build_statement;
DEALLOCATE PREPARE telemetry_build_statement;

-- Independent reviewed incident schema 5 includes build observation loss.
CREATE TABLE IF NOT EXISTS telemetry_incident_registry_v5 (
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
    CONSTRAINT chk_incident_registry_version_v5 CHECK (environment_id <> 0 AND season_id <> 0 AND registry_version <> 0 AND previous_registry_version < registry_version AND registry_version - previous_registry_version = 1),
    CONSTRAINT chk_incident_registry_count_v5 CHECK (incident_count <= 64),
    CONSTRAINT chk_incident_registry_utc_v5 CHECK ((reviewed_from_utc_usec IS NULL OR CAST(reviewed_from_utc_usec AS UNSIGNED) <> 9223372036854775808) AND (reviewed_through_utc_usec IS NULL OR CAST(reviewed_through_utc_usec AS UNSIGNED) <> 9223372036854775808)),
    CONSTRAINT chk_incident_registry_time_v5 CHECK (reviewed_from_utc_usec IS NULL OR reviewed_through_utc_usec IS NULL OR reviewed_from_utc_usec <= reviewed_through_utc_usec)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_incident_v5 (
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
    CONSTRAINT fk_incident_registry_v5 FOREIGN KEY (environment_id,season_id,registry_version) REFERENCES telemetry_incident_registry_v5 (environment_id,season_id,registry_version) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_incident_identity_v5 CHECK (incident_id <> 0 AND ((producer_boot_id IS NULL AND producer_process_id IS NULL AND first_record_seq IS NULL AND last_record_seq IS NULL) OR (producer_boot_id IS NOT NULL AND producer_process_id IS NOT NULL AND producer_boot_id > 0 AND producer_process_id > 0))),
    CONSTRAINT chk_incident_utc_v5 CHECK ((start_utc_usec IS NULL OR CAST(start_utc_usec AS UNSIGNED) <> 9223372036854775808) AND (end_utc_usec IS NULL OR CAST(end_utc_usec AS UNSIGNED) <> 9223372036854775808) AND (verified_occurrence_utc_usec IS NULL OR CAST(verified_occurrence_utc_usec AS UNSIGNED) <> 9223372036854775808)),
    CONSTRAINT chk_incident_time_v5 CHECK (start_utc_usec IS NULL OR end_utc_usec IS NULL OR start_utc_usec <= end_utc_usec),
    CONSTRAINT chk_incident_sequence_v5 CHECK ((first_record_seq IS NULL OR first_record_seq > 0) AND (last_record_seq IS NULL OR last_record_seq > 0) AND (first_record_seq IS NULL OR last_record_seq IS NULL OR first_record_seq <= last_record_seq)),
    CONSTRAINT chk_incident_family_v5 CHECK (record_kind_mask BETWEEN 2 AND 8190 AND (record_kind_mask & 1) = 0),
    CONSTRAINT chk_incident_verification_v5 CHECK ((verified_boot_id IS NULL AND verified_process_id IS NULL AND verified_record_seq IS NULL AND verified_record_kind IS NULL AND verified_occurrence_utc_usec IS NULL) OR (fix_reference_digest IS NOT NULL AND verified_boot_id IS NOT NULL AND verified_process_id IS NOT NULL AND verified_record_seq IS NOT NULL AND verified_record_kind IS NOT NULL AND verified_boot_id > 0 AND verified_process_id > 0 AND verified_record_seq > 0 AND verified_record_kind BETWEEN 1 AND 12 AND (record_kind_mask & (1 << verified_record_kind)) <> 0)),
    CONSTRAINT chk_incident_disposition_v5 CHECK (backlog_disposition BETWEEN 0 AND 5 AND observation_provenance BETWEEN 1 AND 3 AND status BETWEEN 1 AND 2)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
