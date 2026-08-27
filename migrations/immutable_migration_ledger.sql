-- Phase 03 immutable migration history and honest baseline adoption.
-- This ledger is separate from mud_schema_migrations data-copy markers.

CREATE TABLE IF NOT EXISTS mud_schema_baselines (
    baseline_id VARCHAR(64) NOT NULL,
    baseline_kind ENUM('fresh_bootstrap','verified_legacy_adoption') NOT NULL,
    schema_fingerprint BINARY(32) NOT NULL,
    manifest_version INT UNSIGNED NOT NULL,
    runner_version INT UNSIGNED NOT NULL,
    adopted_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (baseline_id),
    UNIQUE KEY uq_mud_schema_baseline_kind (baseline_kind)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS mud_schema_history (
    migration_id VARCHAR(64) NOT NULL,
    sequence_number INT UNSIGNED NOT NULL,
    description VARCHAR(255) NOT NULL,
    apply_checksum BINARY(32) NOT NULL,
    verify_checksum BINARY(32) NOT NULL,
    compatibility VARCHAR(64) NOT NULL,
    runner_version INT UNSIGNED NOT NULL,
    applied_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (migration_id),
    UNIQUE KEY uq_mud_schema_history_sequence (sequence_number)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS mud_schema_migration_state (
    state_id TINYINT UNSIGNED NOT NULL,
    applied_count INT UNSIGNED NOT NULL,
    history_checksum BINARY(32) NOT NULL,
    updated_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)
        ON UPDATE CURRENT_TIMESTAMP(6),
    PRIMARY KEY (state_id),
    CONSTRAINT chk_mud_schema_migration_state_id CHECK (state_id = 1)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

INSERT IGNORE INTO mud_schema_migration_state(state_id,applied_count,history_checksum)
VALUES(1,0,UNHEX(SHA2('',256)));
