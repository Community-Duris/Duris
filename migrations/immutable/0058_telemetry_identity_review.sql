-- Reviewed cross-account associations are append-only under a restricted role.
-- Only an owner provisions reviewer authority; no game-loop identity lookup.
CREATE TABLE IF NOT EXISTS telemetry_identity_reviewer (
    environment_id BIGINT UNSIGNED NOT NULL,
    season_id BIGINT UNSIGNED NOT NULL,
    database_principal VARCHAR(384) COLLATE utf8mb4_bin NOT NULL,
    reviewer_token BINARY(32) NOT NULL,
    enabled TINYINT UNSIGNED NOT NULL,
    PRIMARY KEY (environment_id,season_id,database_principal),
    CONSTRAINT chk_identity_reviewer_scope CHECK (environment_id <> 0 AND season_id <> 0 AND LENGTH(database_principal) > 0),
    CONSTRAINT chk_identity_reviewer_authority CHECK (enabled IN (0,1) AND reviewer_token <> REPEAT(CHAR(0),32))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_identity_registry (
    environment_id BIGINT UNSIGNED NOT NULL,
    season_id BIGINT UNSIGNED NOT NULL,
    registry_version BIGINT UNSIGNED NOT NULL,
    previous_registry_version BIGINT UNSIGNED NOT NULL,
    previous_packet_digest BINARY(32) NULL,
    reviewed_from_utc_usec BIGINT NOT NULL,
    reviewed_through_utc_usec BIGINT NOT NULL,
    reviewed_at_utc_usec BIGINT NOT NULL,
    reviewer_token BINARY(32) NOT NULL,
    review_evidence_digest BINARY(32) NOT NULL,
    packet_digest BINARY(32) NOT NULL,
    association_count SMALLINT UNSIGNED NOT NULL,
    database_principal VARCHAR(384) COLLATE utf8mb4_bin NOT NULL,
    registered_at_utc_usec BIGINT NOT NULL,
    PRIMARY KEY (environment_id,season_id,registry_version),
    UNIQUE KEY uq_identity_registry_digest (environment_id,season_id,registry_version,packet_digest),
    CONSTRAINT chk_identity_registry_scope CHECK (environment_id <> 0 AND season_id <> 0 AND registry_version > previous_registry_version AND registry_version - previous_registry_version = 1 AND LENGTH(database_principal) > 0),
    CONSTRAINT chk_identity_registry_previous CHECK ((previous_registry_version = 0 AND previous_packet_digest IS NULL) OR (previous_registry_version > 0 AND previous_packet_digest IS NOT NULL AND previous_packet_digest <> REPEAT(CHAR(0),32))),
    CONSTRAINT chk_identity_registry_time CHECK (CAST(reviewed_from_utc_usec AS UNSIGNED) <> 9223372036854775808 AND reviewed_from_utc_usec < reviewed_through_utc_usec AND reviewed_through_utc_usec <= reviewed_at_utc_usec AND reviewed_at_utc_usec <= registered_at_utc_usec),
    CONSTRAINT chk_identity_registry_evidence CHECK (association_count <= 1024 AND reviewer_token <> REPEAT(CHAR(0),32) AND review_evidence_digest <> REPEAT(CHAR(0),32) AND packet_digest <> REPEAT(CHAR(0),32))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_identity_association (
    environment_id BIGINT UNSIGNED NOT NULL,
    season_id BIGINT UNSIGNED NOT NULL,
    registry_version BIGINT UNSIGNED NOT NULL,
    association_id BIGINT UNSIGNED NOT NULL,
    account_token BIGINT UNSIGNED NOT NULL,
    controller_token BIGINT UNSIGNED NULL,
    valid_from_utc_usec BIGINT NOT NULL,
    valid_through_utc_usec BIGINT NULL,
    status TINYINT UNSIGNED NOT NULL,
    provenance TINYINT UNSIGNED NOT NULL,
    evidence_digest BINARY(32) NOT NULL,
    PRIMARY KEY (environment_id,season_id,registry_version,association_id),
    KEY idx_identity_association_account (environment_id,season_id,account_token),
    CONSTRAINT fk_identity_association_registry FOREIGN KEY (environment_id,season_id,registry_version) REFERENCES telemetry_identity_registry (environment_id,season_id,registry_version) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT fk_identity_association_account FOREIGN KEY (environment_id,season_id,account_token) REFERENCES telemetry_account_token (environment_id,season_id,account_token) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_identity_association_identity CHECK (association_id <> 0 AND account_token <> 0 AND (controller_token IS NULL OR controller_token <> 0)),
    CONSTRAINT chk_identity_association_time CHECK (CAST(valid_from_utc_usec AS UNSIGNED) <> 9223372036854775808 AND (valid_through_utc_usec IS NULL OR valid_through_utc_usec > valid_from_utc_usec)),
    CONSTRAINT chk_identity_association_evidence CHECK (provenance BETWEEN 1 AND 3 AND evidence_digest <> REPEAT(CHAR(0),32)),
    CONSTRAINT chk_identity_association_status CHECK ((status = 1 AND controller_token IS NOT NULL) OR (status = 2 AND controller_token IS NULL) OR status = 3)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- A reservation pins review identity before a future balance generation builds.
-- It is metadata, not proof that effort or a balance report has been published.
CREATE TABLE IF NOT EXISTS telemetry_generation_identity (
    definition_version INT UNSIGNED NOT NULL,
    generation BIGINT UNSIGNED NOT NULL,
    environment_id BIGINT UNSIGNED NOT NULL,
    season_id BIGINT UNSIGNED NOT NULL,
    registry_version BIGINT UNSIGNED NULL,
    registry_digest BINARY(32) NULL,
    reviewed_from_utc_usec BIGINT NULL,
    reviewed_through_utc_usec BIGINT NULL,
    reviewed_at_utc_usec BIGINT NULL,
    association_count SMALLINT UNSIGNED NOT NULL,
    PRIMARY KEY (definition_version,generation,environment_id,season_id),
    KEY idx_generation_identity_registry (environment_id,season_id,registry_version,registry_digest),
    CONSTRAINT fk_generation_identity_registry FOREIGN KEY (environment_id,season_id,registry_version,registry_digest) REFERENCES telemetry_identity_registry (environment_id,season_id,registry_version,packet_digest) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_generation_identity_scope CHECK (definition_version >= 3 AND generation <> 0 AND environment_id <> 0 AND season_id <> 0),
    CONSTRAINT chk_generation_identity_review CHECK ((registry_version IS NULL AND registry_digest IS NULL AND reviewed_from_utc_usec IS NULL AND reviewed_through_utc_usec IS NULL AND reviewed_at_utc_usec IS NULL AND association_count = 0) OR (registry_version IS NOT NULL AND registry_version > 0 AND registry_digest IS NOT NULL AND registry_digest <> REPEAT(CHAR(0),32) AND reviewed_from_utc_usec IS NOT NULL AND CAST(reviewed_from_utc_usec AS UNSIGNED) <> 9223372036854775808 AND reviewed_through_utc_usec IS NOT NULL AND reviewed_at_utc_usec IS NOT NULL AND reviewed_from_utc_usec < reviewed_through_utc_usec AND reviewed_through_utc_usec <= reviewed_at_utc_usec AND association_count <= 1024))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
