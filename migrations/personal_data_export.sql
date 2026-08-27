-- Phase 03 authenticated personal-data export request and redacted evidence schema.
-- Additive and re-runnable. Raw credentials and exported values never enter these tables.

CREATE TABLE IF NOT EXISTS personal_data_export_requests (
    request_id BINARY(16) NOT NULL,
    request_key BINARY(32) NOT NULL,
    account_name VARCHAR(50) NOT NULL,
    account_scope_hash BINARY(32) NOT NULL,
    policy_id VARCHAR(64) NOT NULL,
    policy_schema_version INT UNSIGNED NOT NULL,
    manifest_checksum BINARY(32) NOT NULL,
    snapshot_id BINARY(16) NULL,
    status TINYINT UNSIGNED NOT NULL,
    attempt_count SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    expected_sections SMALLINT UNSIGNED NOT NULL,
    completed_sections SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    excluded_sections SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    record_count BIGINT UNSIGNED NOT NULL DEFAULT 0,
    package_bytes BIGINT UNSIGNED NOT NULL DEFAULT 0,
    package_checksum BINARY(32) NULL,
    delivery_token_hash BINARY(32) NULL,
    last_error_code INT UNSIGNED NOT NULL DEFAULT 0,
    requested_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    started_at TIMESTAMP(6) NULL DEFAULT NULL,
    completed_at TIMESTAMP(6) NULL DEFAULT NULL,
    cancelled_at TIMESTAMP(6) NULL DEFAULT NULL,
    expires_at TIMESTAMP(6) NOT NULL,
    PRIMARY KEY (request_id),
    UNIQUE KEY uq_personal_export_request_key (request_key),
    KEY idx_personal_export_account_rate (account_name, requested_at, request_id),
    KEY idx_personal_export_work (status, requested_at, request_id),
    KEY idx_personal_export_expiry (status, expires_at, request_id),
    CONSTRAINT chk_personal_export_status CHECK (status BETWEEN 1 AND 9),
    CONSTRAINT chk_personal_export_section_counts CHECK (
        completed_sections + excluded_sections <= expected_sections
    ),
    CONSTRAINT chk_personal_export_expiry CHECK (expires_at > requested_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS personal_data_export_sections (
    request_id BINARY(16) NOT NULL,
    store_id VARCHAR(128) NOT NULL,
    disposition TINYINT UNSIGNED NOT NULL,
    status TINYINT UNSIGNED NOT NULL,
    snapshot_id BINARY(16) NULL,
    record_count BIGINT UNSIGNED NOT NULL DEFAULT 0,
    byte_count BIGINT UNSIGNED NOT NULL DEFAULT 0,
    section_checksum BINARY(32) NULL,
    exclusion_code SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    last_error_code INT UNSIGNED NOT NULL DEFAULT 0,
    completed_at TIMESTAMP(6) NULL DEFAULT NULL,
    PRIMARY KEY (request_id, store_id),
    KEY idx_personal_export_section_status (request_id, status, store_id),
    CONSTRAINT personal_export_section_request_fk FOREIGN KEY (request_id)
        REFERENCES personal_data_export_requests (request_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT chk_personal_export_disposition CHECK (disposition BETWEEN 1 AND 4),
    CONSTRAINT chk_personal_export_section_status CHECK (status BETWEEN 1 AND 9)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS personal_data_export_audit (
    audit_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    request_id BINARY(16) NOT NULL,
    event_type TINYINT UNSIGNED NOT NULL,
    status TINYINT UNSIGNED NOT NULL,
    section_count SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    record_count BIGINT UNSIGNED NOT NULL DEFAULT 0,
    byte_count BIGINT UNSIGNED NOT NULL DEFAULT 0,
    error_code INT UNSIGNED NOT NULL DEFAULT 0,
    occurred_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (audit_id),
    KEY idx_personal_export_audit_request (request_id, audit_id),
    CONSTRAINT personal_export_audit_request_fk FOREIGN KEY (request_id)
        REFERENCES personal_data_export_requests (request_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT chk_personal_export_audit_event CHECK (event_type BETWEEN 1 AND 9),
    CONSTRAINT chk_personal_export_audit_status CHECK (status BETWEEN 1 AND 9)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
