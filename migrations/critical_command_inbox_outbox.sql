-- Phase 02 operation dedupe, generic test mutation, and transactional outbox.
-- Additive and re-runnable. Existing databases must be verified after application.

CREATE TABLE IF NOT EXISTS critical_operation_inbox (
    operation_id BINARY(16) NOT NULL,
    command_hash BINARY(32) NOT NULL,
    keys_hash BINARY(32) NOT NULL,
    command_type SMALLINT UNSIGNED NOT NULL,
    schema_version INT UNSIGNED NOT NULL,
    payload_version SMALLINT UNSIGNED NOT NULL,
    status TINYINT UNSIGNED NOT NULL,
    result_code INT UNSIGNED NOT NULL DEFAULT 0,
    durable_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    result_payload VARBINARY(4096) NOT NULL,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    committed_at TIMESTAMP(6) NULL DEFAULT NULL,
    PRIMARY KEY (operation_id),
    KEY idx_critical_inbox_status_created (status, created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS critical_test_state (
    entity_type TINYINT UNSIGNED NOT NULL,
    entity_id BIGINT UNSIGNED NOT NULL,
    value BIGINT NOT NULL DEFAULT 0,
    revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    updated_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)
        ON UPDATE CURRENT_TIMESTAMP(6),
    PRIMARY KEY (entity_type, entity_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS critical_outbox (
    outbox_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    operation_id BINARY(16) NOT NULL,
    event_index SMALLINT UNSIGNED NOT NULL,
    destination SMALLINT UNSIGNED NOT NULL,
    event_type SMALLINT UNSIGNED NOT NULL,
    payload_version SMALLINT UNSIGNED NOT NULL,
    payload BLOB NOT NULL,
    status TINYINT UNSIGNED NOT NULL DEFAULT 0,
    attempt_count SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    next_attempt_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    delivered_at TIMESTAMP(6) NULL DEFAULT NULL,
    dead_lettered_at TIMESTAMP(6) NULL DEFAULT NULL,
    last_error_code INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (outbox_id),
    UNIQUE KEY uq_critical_outbox_operation_event (operation_id, event_index),
    KEY idx_critical_outbox_claim (status, next_attempt_at, outbox_id),
    KEY idx_critical_outbox_age (status, created_at),
    CONSTRAINT critical_outbox_operation_fk FOREIGN KEY (operation_id)
        REFERENCES critical_operation_inbox (operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS critical_outbox_delivery_dedupe (
    consumer_id SMALLINT UNSIGNED NOT NULL,
    outbox_id BIGINT UNSIGNED NOT NULL,
    delivered_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (consumer_id, outbox_id),
    CONSTRAINT critical_outbox_delivery_fk FOREIGN KEY (outbox_id)
        REFERENCES critical_outbox (outbox_id)
        ON UPDATE RESTRICT ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
