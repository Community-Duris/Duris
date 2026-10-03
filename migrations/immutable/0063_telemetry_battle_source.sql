-- Exact selected source and cursor checkpoint for bounded battle publication.
-- Preparation alone does not activate a battle report or publish a generation.

CREATE TABLE IF NOT EXISTS telemetry_battle_source (
    definition_version INT UNSIGNED NOT NULL,
    generation BIGINT UNSIGNED NOT NULL,
    environment_id BIGINT UNSIGNED NOT NULL,
    season_id BIGINT UNSIGNED NOT NULL,
    input_origin BIGINT UNSIGNED NOT NULL,
    input_watermark BIGINT UNSIGNED NOT NULL,
    source_fact_count INT UNSIGNED NOT NULL,
    source_digest BINARY(32) NOT NULL,
    ownership_count INT UNSIGNED NOT NULL,
    association_count INT UNSIGNED NOT NULL,
    contribution_count INT UNSIGNED NOT NULL,
    quality_flags BIGINT UNSIGNED NOT NULL,
    publication_complete TINYINT UNSIGNED NOT NULL,
    PRIMARY KEY (definition_version,generation,environment_id,season_id),
    CONSTRAINT fk_battle_source_identity FOREIGN KEY (definition_version,generation,environment_id,season_id) REFERENCES telemetry_generation_identity (definition_version,generation,environment_id,season_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT fk_battle_source_state FOREIGN KEY (definition_version,generation,environment_id,season_id) REFERENCES telemetry_rollup_state (definition_version,generation,environment_id,season_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_battle_source_scope CHECK (definition_version = 5 AND generation > 0 AND environment_id > 0 AND season_id > 0),
    CONSTRAINT chk_battle_source_counts CHECK (input_origin <= input_watermark AND source_fact_count <= 16384 AND source_fact_count <= CAST(input_watermark AS DECIMAL(21,0)) - CAST(input_origin AS DECIMAL(21,0)) AND source_fact_count = ownership_count + association_count + contribution_count),
    CONSTRAINT chk_battle_source_complete CHECK (publication_complete BETWEEN 0 AND 1),
    CONSTRAINT chk_battle_source_quality CHECK ((quality_flags & 18446744073172745216) = 0)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_battle_input (
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
    UNIQUE KEY uq_battle_input_replay (definition_version,generation,environment_id,season_id,boot_id,process_id,record_seq),
    CONSTRAINT fk_battle_input_source FOREIGN KEY (definition_version,generation,environment_id,season_id) REFERENCES telemetry_battle_source (definition_version,generation,environment_id,season_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_battle_input_scope CHECK (definition_version = 5 AND generation > 0 AND environment_id > 0 AND season_id > 0),
    CONSTRAINT chk_battle_input_receipt CHECK (ingest_id > 0 AND boot_id > 0 AND process_id > 0 AND record_seq > 0),
    CONSTRAINT chk_battle_input_kind CHECK (record_kind IN (9,10,11)),
    CONSTRAINT chk_battle_input_payload CHECK (OCTET_LENGTH(payload) BETWEEN 1 AND 8192)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
