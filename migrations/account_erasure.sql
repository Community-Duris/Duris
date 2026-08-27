-- Phase 03 account erasure request, per-store evidence, and restore tombstones.
-- Additive and re-runnable. Subject names, credentials, and erased values are excluded.

CREATE TABLE IF NOT EXISTS account_erasure_requests (
    request_id BINARY(16) NOT NULL,
    request_key BINARY(32) NOT NULL,
    account_scope_hash BINARY(32) NOT NULL,
    subject_token BINARY(32) NOT NULL,
    policy_id VARCHAR(64) NOT NULL,
    policy_schema_version INT UNSIGNED NOT NULL,
    manifest_checksum BINARY(32) NOT NULL,
    status TINYINT UNSIGNED NOT NULL,
    fence_revision BIGINT UNSIGNED NULL,
    expected_stores SMALLINT UNSIGNED NOT NULL,
    completed_stores SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    retained_stores SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    reconciliation_checksum BINARY(32) NULL,
    last_error_code INT UNSIGNED NOT NULL DEFAULT 0,
    requested_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    confirmed_at TIMESTAMP(6) NULL DEFAULT NULL,
    fenced_at TIMESTAMP(6) NULL DEFAULT NULL,
    completed_at TIMESTAMP(6) NULL DEFAULT NULL,
    cancelled_at TIMESTAMP(6) NULL DEFAULT NULL,
    PRIMARY KEY (request_id),
    UNIQUE KEY uq_account_erasure_request_key (request_key),
    KEY idx_account_erasure_scope_rate (account_scope_hash, requested_at, request_id),
    KEY idx_account_erasure_work (status, requested_at, request_id),
    CONSTRAINT chk_account_erasure_status CHECK (status BETWEEN 1 AND 10),
    CONSTRAINT chk_account_erasure_counts CHECK (
        completed_stores + retained_stores <= expected_stores
    )
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS account_erasure_stores (
    request_id BINARY(16) NOT NULL,
    store_id VARCHAR(128) NOT NULL,
    action TINYINT UNSIGNED NOT NULL,
    status TINYINT UNSIGNED NOT NULL,
    sequence_number SMALLINT UNSIGNED NOT NULL,
    affected_count BIGINT UNSIGNED NOT NULL DEFAULT 0,
    remaining_direct_identifiers BIGINT UNSIGNED NOT NULL DEFAULT 0,
    evidence_checksum BINARY(32) NULL,
    last_error_code INT UNSIGNED NOT NULL DEFAULT 0,
    completed_at TIMESTAMP(6) NULL DEFAULT NULL,
    PRIMARY KEY (request_id, store_id),
    UNIQUE KEY uq_account_erasure_store_sequence (request_id, sequence_number),
    KEY idx_account_erasure_store_work (request_id, status, sequence_number),
    CONSTRAINT account_erasure_store_request_fk FOREIGN KEY (request_id)
        REFERENCES account_erasure_requests (request_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT chk_account_erasure_action CHECK (action BETWEEN 1 AND 6),
    CONSTRAINT chk_account_erasure_store_status CHECK (status BETWEEN 1 AND 10)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS account_erasure_evidence (
    evidence_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    request_id BINARY(16) NOT NULL,
    store_id VARCHAR(128) NULL,
    event_type TINYINT UNSIGNED NOT NULL,
    status TINYINT UNSIGNED NOT NULL,
    affected_count BIGINT UNSIGNED NOT NULL DEFAULT 0,
    remaining_count BIGINT UNSIGNED NOT NULL DEFAULT 0,
    error_code INT UNSIGNED NOT NULL DEFAULT 0,
    occurred_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (evidence_id),
    KEY idx_account_erasure_evidence_request (request_id, evidence_id),
    CONSTRAINT account_erasure_evidence_request_fk FOREIGN KEY (request_id)
        REFERENCES account_erasure_requests (request_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT chk_account_erasure_evidence_event CHECK (event_type BETWEEN 1 AND 10),
    CONSTRAINT chk_account_erasure_evidence_status CHECK (status BETWEEN 1 AND 10)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS account_erasure_tombstones (
    subject_token BINARY(32) NOT NULL,
    request_id BINARY(16) NOT NULL,
    account_scope_hash BINARY(32) NOT NULL,
    policy_id VARCHAR(64) NOT NULL,
    policy_schema_version INT UNSIGNED NOT NULL,
    manifest_checksum BINARY(32) NOT NULL,
    completed_at TIMESTAMP(6) NOT NULL,
    last_restore_generation BINARY(32) NULL,
    restore_apply_count INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (subject_token),
    UNIQUE KEY uq_account_erasure_tombstone_request (request_id),
    KEY idx_account_erasure_tombstone_scope (account_scope_hash),
    CONSTRAINT account_erasure_tombstone_request_fk FOREIGN KEY (request_id)
        REFERENCES account_erasure_requests (request_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
