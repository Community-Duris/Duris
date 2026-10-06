-- Definition 9 retained progression, milestone, rotation and portfolio evidence.
-- Earlier definitions and immutable generations retain their original meanings.
-- Original references do not advance the selected source cursor.
-- Public coverage/detail and source completion commit in the existing publication transaction.

CREATE TABLE IF NOT EXISTS telemetry_progression_source_v9 (
    definition_version INT UNSIGNED NOT NULL,
    generation BIGINT UNSIGNED NOT NULL,
    environment_id BIGINT UNSIGNED NOT NULL,
    season_id BIGINT UNSIGNED NOT NULL,
    input_origin BIGINT UNSIGNED NOT NULL,
    input_watermark BIGINT UNSIGNED NOT NULL,
    source_fact_count INT UNSIGNED NOT NULL,
    source_digest BINARY(32) NOT NULL,
    interval_count INT UNSIGNED NOT NULL,
    lifecycle_count INT UNSIGNED NOT NULL,
    progression_count INT UNSIGNED NOT NULL,
    ownership_count INT UNSIGNED NOT NULL,
    context_count INT UNSIGNED NOT NULL,
    configuration_chunk_count INT UNSIGNED NOT NULL,
    reference_count INT UNSIGNED NOT NULL,
    reference_digest BINARY(32) NOT NULL,
    quality_flags BIGINT UNSIGNED NOT NULL,
    publication_complete TINYINT UNSIGNED NOT NULL,
    PRIMARY KEY (definition_version,generation,environment_id,season_id),
    CONSTRAINT fk_progression9_source_identity FOREIGN KEY (definition_version,generation,environment_id,season_id) REFERENCES telemetry_generation_identity (definition_version,generation,environment_id,season_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT fk_progression9_source_state FOREIGN KEY (definition_version,generation,environment_id,season_id) REFERENCES telemetry_rollup_state (definition_version,generation,environment_id,season_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_progression9_source_scope CHECK (definition_version = 9 AND generation > 0 AND environment_id > 0 AND season_id > 0),
    CONSTRAINT chk_progression9_source_counts CHECK (input_origin <= input_watermark AND source_fact_count + reference_count <= 16384 AND source_fact_count <= CAST(input_watermark AS DECIMAL(21,0)) - CAST(input_origin AS DECIMAL(21,0)) AND source_fact_count = interval_count + lifecycle_count + progression_count + ownership_count + context_count + configuration_chunk_count),
    CONSTRAINT chk_progression9_source_quality CHECK ((quality_flags & 18446744073172745216) = 0),
    CONSTRAINT chk_progression9_source_complete CHECK (publication_complete BETWEEN 0 AND 1)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_progression_input_v9 (
    definition_version INT UNSIGNED NOT NULL,
    generation BIGINT UNSIGNED NOT NULL,
    environment_id BIGINT UNSIGNED NOT NULL,
    season_id BIGINT UNSIGNED NOT NULL,
    ingest_id BIGINT UNSIGNED NOT NULL,
    boot_id BIGINT UNSIGNED NOT NULL,
    process_id BIGINT UNSIGNED NOT NULL,
    record_seq BIGINT UNSIGNED NOT NULL,
    record_kind TINYINT UNSIGNED NOT NULL,
    payload VARBINARY(8192) NOT NULL,
    payload_digest BINARY(32) NOT NULL,
    PRIMARY KEY (definition_version,generation,environment_id,season_id,ingest_id),
    UNIQUE KEY uq_progression9_input_replay (definition_version,generation,environment_id,season_id,boot_id,process_id,record_seq),
    CONSTRAINT fk_progression9_input_source FOREIGN KEY (definition_version,generation,environment_id,season_id) REFERENCES telemetry_progression_source_v9 (definition_version,generation,environment_id,season_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_progression9_input_scope CHECK (definition_version = 9 AND generation > 0 AND environment_id > 0 AND season_id > 0),
    CONSTRAINT chk_progression9_input_receipt CHECK (ingest_id > 0 AND boot_id > 0 AND process_id > 0 AND record_seq > 0),
    CONSTRAINT chk_progression9_input_kind CHECK (record_kind IN (1,2,6,9,15,16)),
    CONSTRAINT chk_progression9_input_payload CHECK (OCTET_LENGTH(payload) BETWEEN 1 AND 8192)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_progression_reference_v9 (
    definition_version INT UNSIGNED NOT NULL,
    generation BIGINT UNSIGNED NOT NULL,
    environment_id BIGINT UNSIGNED NOT NULL,
    season_id BIGINT UNSIGNED NOT NULL,
    ingest_id BIGINT UNSIGNED NOT NULL,
    boot_id BIGINT UNSIGNED NOT NULL,
    process_id BIGINT UNSIGNED NOT NULL,
    record_seq BIGINT UNSIGNED NOT NULL,
    record_kind TINYINT UNSIGNED NOT NULL,
    payload VARBINARY(8192) NOT NULL,
    payload_digest BINARY(32) NOT NULL,
    PRIMARY KEY (definition_version,generation,environment_id,season_id,ingest_id),
    UNIQUE KEY uq_progression9_reference_replay (definition_version,generation,environment_id,season_id,boot_id,process_id,record_seq),
    CONSTRAINT fk_progression9_reference_source FOREIGN KEY (definition_version,generation,environment_id,season_id) REFERENCES telemetry_progression_source_v9 (definition_version,generation,environment_id,season_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_progression9_reference_scope CHECK (definition_version = 9 AND generation > 0 AND environment_id > 0 AND season_id > 0),
    CONSTRAINT chk_progression9_reference_receipt CHECK (ingest_id > 0 AND boot_id > 0 AND process_id > 0 AND record_seq > 0),
    CONSTRAINT chk_progression9_reference_kind CHECK (record_kind IN (1,2,6,9,16)),
    CONSTRAINT chk_progression9_reference_payload CHECK (OCTET_LENGTH(payload) BETWEEN 1 AND 8192)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_rollup_progression_coverage_v9 (
    definition_version INT UNSIGNED NOT NULL,
    generation BIGINT UNSIGNED NOT NULL,
    environment_id BIGINT UNSIGNED NOT NULL,
    season_id BIGINT UNSIGNED NOT NULL,
    input_origin BIGINT UNSIGNED NOT NULL,
    input_watermark BIGINT UNSIGNED NOT NULL,
    source_fact_count INT UNSIGNED NOT NULL,
    source_digest BINARY(32) NOT NULL,
    interval_count INT UNSIGNED NOT NULL,
    lifecycle_count INT UNSIGNED NOT NULL,
    progression_count INT UNSIGNED NOT NULL,
    ownership_count INT UNSIGNED NOT NULL,
    context_count INT UNSIGNED NOT NULL,
    configuration_chunk_count INT UNSIGNED NOT NULL,
    reference_count INT UNSIGNED NOT NULL,
    reference_digest BINARY(32) NOT NULL,
    quality_flags BIGINT UNSIGNED NOT NULL,
    publication_complete TINYINT UNSIGNED NOT NULL,
    snapshot_digest BINARY(32) NOT NULL,
    context_row_count INT UNSIGNED NOT NULL,
    milestone_row_count INT UNSIGNED NOT NULL,
    rotation_row_count INT UNSIGNED NOT NULL,
    effort_row_count INT UNSIGNED NOT NULL,
    portfolio_row_count INT UNSIGNED NOT NULL,
    unknown_context_count INT UNSIGNED NOT NULL,
    completed_milestone_count INT UNSIGNED NOT NULL,
    unfinished_milestone_count INT UNSIGNED NOT NULL,
    censored_milestone_count INT UNSIGNED NOT NULL,
    qualified_full_stage_count INT UNSIGNED NOT NULL,
    qualified_rate_cell_count INT UNSIGNED NOT NULL,
    PRIMARY KEY (definition_version,generation,environment_id,season_id),
    CONSTRAINT fk_progression9_coverage_source FOREIGN KEY (definition_version,generation,environment_id,season_id) REFERENCES telemetry_progression_source_v9 (definition_version,generation,environment_id,season_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_progression9_coverage_scope CHECK (definition_version = 9 AND generation > 0 AND environment_id > 0 AND season_id > 0),
    CONSTRAINT chk_progression9_coverage_counts CHECK (input_origin <= input_watermark AND source_fact_count + reference_count <= 16384 AND source_fact_count <= CAST(input_watermark AS DECIMAL(21,0)) - CAST(input_origin AS DECIMAL(21,0)) AND source_fact_count = interval_count + lifecycle_count + progression_count + ownership_count + context_count + configuration_chunk_count),
    CONSTRAINT chk_progression9_coverage_quality CHECK ((quality_flags & 18446744073172745216) = 0),
    CONSTRAINT chk_progression9_coverage_complete CHECK (publication_complete = 1),
    CONSTRAINT chk_progression9_coverage_rows CHECK (context_row_count + milestone_row_count + rotation_row_count + effort_row_count + portfolio_row_count <= 4096),
    CONSTRAINT chk_progression9_coverage_detail CHECK (context_row_count = context_count AND unknown_context_count <= context_row_count AND completed_milestone_count + unfinished_milestone_count + censored_milestone_count = milestone_row_count AND qualified_full_stage_count <= completed_milestone_count AND qualified_rate_cell_count <= portfolio_row_count)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_rollup_progression_row_v9 (
    definition_version INT UNSIGNED NOT NULL,
    generation BIGINT UNSIGNED NOT NULL,
    environment_id BIGINT UNSIGNED NOT NULL,
    season_id BIGINT UNSIGNED NOT NULL,
    row_kind TINYINT UNSIGNED NOT NULL,
    row_key BINARY(32) NOT NULL,
    payload VARBINARY(8192) NOT NULL,
    payload_digest BINARY(32) NOT NULL,
    quality_flags BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (definition_version,generation,environment_id,season_id,row_kind,row_key),
    CONSTRAINT fk_progression9_row_coverage FOREIGN KEY (definition_version,generation,environment_id,season_id) REFERENCES telemetry_rollup_progression_coverage_v9 (definition_version,generation,environment_id,season_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_progression9_row_scope CHECK (definition_version = 9 AND generation > 0 AND environment_id > 0 AND season_id > 0),
    CONSTRAINT chk_progression9_row_kind CHECK (row_kind BETWEEN 1 AND 5),
    CONSTRAINT chk_progression9_row_payload CHECK (OCTET_LENGTH(payload) BETWEEN 1 AND 8192),
    CONSTRAINT chk_progression9_row_quality CHECK ((quality_flags & 18446744073172745216) = 0)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TRIGGER IF EXISTS telemetry_progression9_source_insert;
DELIMITER $$
CREATE TRIGGER telemetry_progression9_source_insert BEFORE INSERT ON telemetry_progression_source_v9 FOR EACH ROW
BEGIN
    IF NEW.publication_complete <> 0 OR NOT EXISTS (SELECT 1 FROM telemetry_rollup_state s WHERE s.definition_version = NEW.definition_version AND s.generation = NEW.generation AND s.environment_id = NEW.environment_id AND s.season_id = NEW.season_id AND s.publication_status = 0) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'progression source requires building generation';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_progression9_source_update;
DELIMITER $$
CREATE TRIGGER telemetry_progression9_source_update BEFORE UPDATE ON telemetry_progression_source_v9 FOR EACH ROW
BEGIN
    IF OLD.publication_complete <> 0 OR NOT (NEW.definition_version <=> OLD.definition_version) OR NOT (NEW.generation <=> OLD.generation) OR NOT (NEW.environment_id <=> OLD.environment_id) OR NOT (NEW.season_id <=> OLD.season_id) OR NOT (NEW.input_origin <=> OLD.input_origin) OR NEW.input_watermark < OLD.input_watermark OR NEW.source_fact_count < OLD.source_fact_count OR NEW.interval_count < OLD.interval_count OR NEW.lifecycle_count < OLD.lifecycle_count OR NEW.progression_count < OLD.progression_count OR NEW.ownership_count < OLD.ownership_count OR NEW.context_count < OLD.context_count OR NEW.configuration_chunk_count < OLD.configuration_chunk_count OR NEW.reference_count < OLD.reference_count OR NOT EXISTS (SELECT 1 FROM telemetry_rollup_state s WHERE s.definition_version = NEW.definition_version AND s.generation = NEW.generation AND s.environment_id = NEW.environment_id AND s.season_id = NEW.season_id AND s.publication_status = 0) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'progression source is immutable or backward';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_progression9_source_delete;
DELIMITER $$
CREATE TRIGGER telemetry_progression9_source_delete BEFORE DELETE ON telemetry_progression_source_v9 FOR EACH ROW
BEGIN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'progression retained evidence is immutable';
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_progression9_input_insert;
DELIMITER $$
CREATE TRIGGER telemetry_progression9_input_insert BEFORE INSERT ON telemetry_progression_input_v9 FOR EACH ROW
BEGIN
    IF NOT EXISTS (SELECT 1 FROM telemetry_progression_source_v9 s WHERE s.definition_version = NEW.definition_version AND s.generation = NEW.generation AND s.environment_id = NEW.environment_id AND s.season_id = NEW.season_id AND s.publication_complete = 0) OR NOT EXISTS (SELECT 1 FROM telemetry_rollup_state s WHERE s.definition_version = NEW.definition_version AND s.generation = NEW.generation AND s.environment_id = NEW.environment_id AND s.season_id = NEW.season_id AND s.publication_status = 0) OR NOT (NEW.payload_digest <=> UNHEX(SHA2(NEW.payload,256))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'progression evidence requires building exact source';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_progression9_input_update;
DELIMITER $$
CREATE TRIGGER telemetry_progression9_input_update BEFORE UPDATE ON telemetry_progression_input_v9 FOR EACH ROW
BEGIN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'progression retained evidence is immutable';
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_progression9_input_delete;
DELIMITER $$
CREATE TRIGGER telemetry_progression9_input_delete BEFORE DELETE ON telemetry_progression_input_v9 FOR EACH ROW
BEGIN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'progression retained evidence is immutable';
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_progression9_reference_insert;
DELIMITER $$
CREATE TRIGGER telemetry_progression9_reference_insert BEFORE INSERT ON telemetry_progression_reference_v9 FOR EACH ROW
BEGIN
    IF NOT EXISTS (SELECT 1 FROM telemetry_progression_source_v9 s WHERE s.definition_version = NEW.definition_version AND s.generation = NEW.generation AND s.environment_id = NEW.environment_id AND s.season_id = NEW.season_id AND s.publication_complete = 0) OR NOT EXISTS (SELECT 1 FROM telemetry_rollup_state s WHERE s.definition_version = NEW.definition_version AND s.generation = NEW.generation AND s.environment_id = NEW.environment_id AND s.season_id = NEW.season_id AND s.publication_status = 0) OR NOT (NEW.payload_digest <=> UNHEX(SHA2(NEW.payload,256))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'progression evidence requires building exact source';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_progression9_reference_update;
DELIMITER $$
CREATE TRIGGER telemetry_progression9_reference_update BEFORE UPDATE ON telemetry_progression_reference_v9 FOR EACH ROW
BEGIN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'progression retained evidence is immutable';
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_progression9_reference_delete;
DELIMITER $$
CREATE TRIGGER telemetry_progression9_reference_delete BEFORE DELETE ON telemetry_progression_reference_v9 FOR EACH ROW
BEGIN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'progression retained evidence is immutable';
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_progression9_coverage_insert;
DELIMITER $$
CREATE TRIGGER telemetry_progression9_coverage_insert BEFORE INSERT ON telemetry_rollup_progression_coverage_v9 FOR EACH ROW
BEGIN
    IF NOT EXISTS (SELECT 1 FROM telemetry_progression_source_v9 s WHERE s.definition_version = NEW.definition_version AND s.generation = NEW.generation AND s.environment_id = NEW.environment_id AND s.season_id = NEW.season_id AND s.publication_complete = 0) OR NOT EXISTS (SELECT 1 FROM telemetry_rollup_state s WHERE s.definition_version = NEW.definition_version AND s.generation = NEW.generation AND s.environment_id = NEW.environment_id AND s.season_id = NEW.season_id AND s.publication_status = 0) OR NOT EXISTS (SELECT 1 FROM telemetry_progression_source_v9 s WHERE s.definition_version = NEW.definition_version AND s.generation = NEW.generation AND s.environment_id = NEW.environment_id AND s.season_id = NEW.season_id AND s.input_origin = NEW.input_origin AND s.input_watermark = NEW.input_watermark AND s.source_fact_count = NEW.source_fact_count AND s.source_digest = NEW.source_digest AND s.interval_count = NEW.interval_count AND s.lifecycle_count = NEW.lifecycle_count AND s.progression_count = NEW.progression_count AND s.ownership_count = NEW.ownership_count AND s.context_count = NEW.context_count AND s.configuration_chunk_count = NEW.configuration_chunk_count AND s.reference_count = NEW.reference_count AND s.reference_digest = NEW.reference_digest AND (s.quality_flags & NEW.quality_flags) = s.quality_flags) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'progression evidence requires building exact source';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_progression9_coverage_update;
DELIMITER $$
CREATE TRIGGER telemetry_progression9_coverage_update BEFORE UPDATE ON telemetry_rollup_progression_coverage_v9 FOR EACH ROW
BEGIN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'progression retained evidence is immutable';
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_progression9_coverage_delete;
DELIMITER $$
CREATE TRIGGER telemetry_progression9_coverage_delete BEFORE DELETE ON telemetry_rollup_progression_coverage_v9 FOR EACH ROW
BEGIN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'progression retained evidence is immutable';
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_progression9_row_insert;
DELIMITER $$
CREATE TRIGGER telemetry_progression9_row_insert BEFORE INSERT ON telemetry_rollup_progression_row_v9 FOR EACH ROW
BEGIN
    IF NOT EXISTS (SELECT 1 FROM telemetry_progression_source_v9 s WHERE s.definition_version = NEW.definition_version AND s.generation = NEW.generation AND s.environment_id = NEW.environment_id AND s.season_id = NEW.season_id AND s.publication_complete = 0) OR NOT EXISTS (SELECT 1 FROM telemetry_rollup_state s WHERE s.definition_version = NEW.definition_version AND s.generation = NEW.generation AND s.environment_id = NEW.environment_id AND s.season_id = NEW.season_id AND s.publication_status = 0) OR NOT (NEW.payload_digest <=> UNHEX(SHA2(NEW.payload,256))) OR NOT EXISTS (SELECT 1 FROM telemetry_rollup_progression_coverage_v9 s WHERE s.definition_version = NEW.definition_version AND s.generation = NEW.generation AND s.environment_id = NEW.environment_id AND s.season_id = NEW.season_id) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'progression evidence requires building exact source';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_progression9_row_update;
DELIMITER $$
CREATE TRIGGER telemetry_progression9_row_update BEFORE UPDATE ON telemetry_rollup_progression_row_v9 FOR EACH ROW
BEGIN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'progression retained evidence is immutable';
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_progression9_row_delete;
DELIMITER $$
CREATE TRIGGER telemetry_progression9_row_delete BEFORE DELETE ON telemetry_rollup_progression_row_v9 FOR EACH ROW
BEGIN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'progression retained evidence is immutable';
END$$
DELIMITER ;
