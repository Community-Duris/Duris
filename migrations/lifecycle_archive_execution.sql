-- Phase 03 lifecycle archive execution metadata and same-database archive envelope.
-- Additive and re-runnable. The canonical pending policy does not enable mutation.
-- action: 1=archive, 2=purge, 3=pseudonymize, 4=cascade, 5=restore_tombstone.
-- status: 1=planned, 2=paused, 3=copying, 4=copied, 5=verified,
--         6=finalizing, 7=completed, 8=failed_or_blocked.

CREATE TABLE IF NOT EXISTS lifecycle_archive_jobs (
    job_id BINARY(16) NOT NULL,
    job_key BINARY(32) NOT NULL,
    policy_id VARCHAR(64) NOT NULL,
    policy_schema_version INT UNSIGNED NOT NULL,
    manifest_checksum BINARY(32) NOT NULL,
    store_id VARCHAR(128) NOT NULL,
    action TINYINT UNSIGNED NOT NULL,
    dry_run TINYINT UNSIGNED NOT NULL DEFAULT 1,
    target_environment VARCHAR(16) NOT NULL,
    approval_reference VARCHAR(128) NOT NULL,
    status TINYINT UNSIGNED NOT NULL,
    source_cursor VARBINARY(512) NOT NULL,
    source_upper_bound VARBINARY(512) NOT NULL,
    row_budget SMALLINT UNSIGNED NOT NULL,
    byte_budget INT UNSIGNED NOT NULL,
    time_budget_usec INT UNSIGNED NOT NULL,
    attempt_count INT UNSIGNED NOT NULL DEFAULT 0,
    source_count BIGINT UNSIGNED NOT NULL DEFAULT 0,
    archive_count BIGINT UNSIGNED NOT NULL DEFAULT 0,
    source_checksum BINARY(32) NULL,
    archive_checksum BINARY(32) NULL,
    reconciliation_before TINYINT UNSIGNED NOT NULL DEFAULT 0,
    reconciliation_after TINYINT UNSIGNED NOT NULL DEFAULT 0,
    last_error_code INT UNSIGNED NOT NULL DEFAULT 0,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    updated_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)
        ON UPDATE CURRENT_TIMESTAMP(6),
    completed_at TIMESTAMP(6) NULL DEFAULT NULL,
    PRIMARY KEY (job_id),
    UNIQUE KEY uq_lifecycle_archive_job_key (job_key),
    KEY idx_lifecycle_archive_job_claim (status, updated_at, job_id),
    KEY idx_lifecycle_archive_job_store (store_id, status, job_id),
    CONSTRAINT chk_lifecycle_archive_job_action CHECK (action BETWEEN 1 AND 5),
    CONSTRAINT chk_lifecycle_archive_job_status CHECK (status BETWEEN 1 AND 8),
    CONSTRAINT chk_lifecycle_archive_job_dry_run CHECK (dry_run IN (0,1)),
    CONSTRAINT chk_lifecycle_archive_job_budgets CHECK (
        row_budget BETWEEN 1 AND 256 AND
        byte_budget BETWEEN 1 AND 1048576 AND
        time_budget_usec BETWEEN 1 AND 500000
    )
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS lifecycle_archive_batches (
    batch_id BINARY(16) NOT NULL,
    batch_key BINARY(32) NOT NULL,
    job_id BINARY(16) NOT NULL,
    sequence_number BIGINT UNSIGNED NOT NULL,
    status TINYINT UNSIGNED NOT NULL,
    cursor_start VARBINARY(512) NOT NULL,
    cursor_end VARBINARY(512) NOT NULL,
    source_count INT UNSIGNED NOT NULL DEFAULT 0,
    archive_count INT UNSIGNED NOT NULL DEFAULT 0,
    source_bytes INT UNSIGNED NOT NULL DEFAULT 0,
    source_checksum BINARY(32) NULL,
    archive_checksum BINARY(32) NULL,
    reconciliation_before TINYINT UNSIGNED NOT NULL DEFAULT 0,
    reconciliation_after TINYINT UNSIGNED NOT NULL DEFAULT 0,
    attempt_count INT UNSIGNED NOT NULL DEFAULT 0,
    last_error_code INT UNSIGNED NOT NULL DEFAULT 0,
    started_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    verified_at TIMESTAMP(6) NULL DEFAULT NULL,
    completed_at TIMESTAMP(6) NULL DEFAULT NULL,
    PRIMARY KEY (batch_id),
    UNIQUE KEY uq_lifecycle_archive_batch_key (batch_key),
    UNIQUE KEY uq_lifecycle_archive_batch_sequence (job_id, sequence_number),
    UNIQUE KEY uq_lifecycle_archive_batch_job_id (job_id, batch_id),
    KEY idx_lifecycle_archive_batch_resume (job_id, status, sequence_number),
    CONSTRAINT lifecycle_archive_batch_job_fk FOREIGN KEY (job_id)
        REFERENCES lifecycle_archive_jobs (job_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT chk_lifecycle_archive_batch_status CHECK (status BETWEEN 1 AND 8)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS lifecycle_archive_rows (
    batch_id BINARY(16) NOT NULL,
    source_key VARBINARY(512) NOT NULL,
    source_checksum BINARY(32) NOT NULL,
    payload LONGBLOB NOT NULL,
    payload_bytes INT UNSIGNED NOT NULL,
    archived_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (batch_id, source_key),
    CONSTRAINT lifecycle_archive_row_batch_fk FOREIGN KEY (batch_id)
        REFERENCES lifecycle_archive_batches (batch_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT chk_lifecycle_archive_row_size CHECK (
        payload_bytes BETWEEN 1 AND 1048576
    )
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS lifecycle_archive_evidence (
    evidence_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    job_id BINARY(16) NOT NULL,
    batch_id BINARY(16) NULL,
    event_type TINYINT UNSIGNED NOT NULL,
    status TINYINT UNSIGNED NOT NULL,
    row_count INT UNSIGNED NOT NULL DEFAULT 0,
    byte_count INT UNSIGNED NOT NULL DEFAULT 0,
    error_code INT UNSIGNED NOT NULL DEFAULT 0,
    occurred_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (evidence_id),
    KEY idx_lifecycle_archive_evidence_job (job_id, evidence_id),
    CONSTRAINT lifecycle_archive_evidence_job_fk FOREIGN KEY (job_id)
        REFERENCES lifecycle_archive_jobs (job_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT lifecycle_archive_evidence_batch_fk FOREIGN KEY (job_id, batch_id)
        REFERENCES lifecycle_archive_batches (job_id, batch_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT chk_lifecycle_archive_evidence_event CHECK (event_type BETWEEN 1 AND 8),
    CONSTRAINT chk_lifecycle_archive_evidence_status CHECK (status BETWEEN 1 AND 8)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
