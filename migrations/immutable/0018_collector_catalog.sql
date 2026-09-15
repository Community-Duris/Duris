-- Collector-of-Antiquities metadata, held payloads, due projections, and audit evidence.
-- The canonical record_blob is the versioned authority. Indexed columns are projections
-- validated and updated in the same transaction by the collector repository.

CREATE TABLE IF NOT EXISTS collector_catalog_state (
    state_id TINYINT UNSIGNED NOT NULL,
    catalog_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    next_listing BIGINT UNSIGNED NOT NULL DEFAULT 1,
    updated_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)
        ON UPDATE CURRENT_TIMESTAMP(6),
    PRIMARY KEY (state_id),
    CONSTRAINT chk_collector_catalog_singleton CHECK (state_id = 1),
    CONSTRAINT chk_collector_catalog_next_listing CHECK (next_listing > 0)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

INSERT IGNORE INTO collector_catalog_state(state_id,catalog_revision,next_listing)
VALUES(1,0,1);

CREATE TABLE IF NOT EXISTS collector_deaths (
    death_operation_id BINARY(16) NOT NULL,
    beneficiary_pid INT UNSIGNED NOT NULL,
    death_time BIGINT UNSIGNED NOT NULL,
    collection_delay BIGINT UNSIGNED NOT NULL DEFAULT 43200,
    sale_delay BIGINT UNSIGNED NOT NULL DEFAULT 86400,
    holding_duration BIGINT UNSIGNED NOT NULL DEFAULT 604800,
    price_percent BIGINT UNSIGNED NOT NULL DEFAULT 200,
    minimum_value BIGINT UNSIGNED NOT NULL DEFAULT 100,
    hint_state TINYINT UNSIGNED NOT NULL DEFAULT 0,
    hint_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    updated_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)
        ON UPDATE CURRENT_TIMESTAMP(6),
    PRIMARY KEY (death_operation_id),
    UNIQUE KEY uq_collector_death_identity (beneficiary_pid,death_time),
    CONSTRAINT chk_collector_death_beneficiary CHECK (beneficiary_pid > 0),
    CONSTRAINT chk_collector_death_time CHECK (death_time > 0),
    CONSTRAINT chk_collector_death_collection_delay CHECK (collection_delay > 0),
    CONSTRAINT chk_collector_death_sale_delay CHECK (sale_delay >= collection_delay),
    CONSTRAINT chk_collector_death_holding_duration CHECK (holding_duration > 0),
    CONSTRAINT chk_collector_death_price_percent CHECK (price_percent > 0),
    CONSTRAINT chk_collector_death_minimum_value CHECK (minimum_value > 0),
    CONSTRAINT chk_collector_hint_state CHECK (hint_state BETWEEN 0 AND 2)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS collector_listings (
    listing_id BIGINT UNSIGNED NOT NULL,
    death_operation_id BINARY(16) NOT NULL,
    beneficiary_pid INT UNSIGNED NOT NULL,
    item_uid BIGINT UNSIGNED NOT NULL,
    status TINYINT UNSIGNED NOT NULL,
    holding_paused TINYINT UNSIGNED NOT NULL DEFAULT 0,
    due_at BIGINT UNSIGNED NULL,
    listing_revision BIGINT UNSIGNED NOT NULL,
    item_revision BIGINT UNSIGNED NOT NULL,
    price_value BIGINT UNSIGNED NOT NULL DEFAULT 0,
    record_blob VARBINARY(154) NOT NULL,
    item_blob MEDIUMBLOB NULL,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    updated_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)
        ON UPDATE CURRENT_TIMESTAMP(6),
    PRIMARY KEY (listing_id),
    UNIQUE KEY uq_collector_death_item (death_operation_id,item_uid),
    KEY idx_collector_item_history (item_uid,listing_id),
    KEY idx_collector_beneficiary (beneficiary_pid,status,listing_id),
    KEY idx_collector_due (status,holding_paused,due_at,listing_id),
    CONSTRAINT chk_collector_listing_id CHECK (listing_id > 0),
    CONSTRAINT chk_collector_listing_beneficiary CHECK (beneficiary_pid > 0),
    CONSTRAINT chk_collector_listing_item CHECK (item_uid > 0),
    CONSTRAINT chk_collector_listing_status CHECK (status BETWEEN 1 AND 6),
    CONSTRAINT chk_collector_listing_paused CHECK (holding_paused BETWEEN 0 AND 1),
    CONSTRAINT chk_collector_listing_revision CHECK (listing_revision > 0),
    CONSTRAINT chk_collector_listing_item_revision CHECK (item_revision > 0),
    CONSTRAINT chk_collector_record_size CHECK (OCTET_LENGTH(record_blob) = 154),
    CONSTRAINT chk_collector_item_blob_size CHECK (
        item_blob IS NULL OR OCTET_LENGTH(item_blob) BETWEEN 1 AND 131072),
    CONSTRAINT collector_listing_death_fk FOREIGN KEY (death_operation_id)
        REFERENCES collector_deaths(death_operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS collector_ledger (
    operation_id BINARY(16) NOT NULL,
    listing_id BIGINT UNSIGNED NOT NULL,
    action TINYINT UNSIGNED NOT NULL,
    catalog_revision BIGINT UNSIGNED NOT NULL,
    listing_revision BIGINT UNSIGNED NOT NULL,
    actor_pid INT UNSIGNED NOT NULL DEFAULT 0,
    item_uid BIGINT UNSIGNED NOT NULL,
    value_delta BIGINT NOT NULL DEFAULT 0,
    closed_reason TINYINT UNSIGNED NOT NULL DEFAULT 0,
    source_site SMALLINT UNSIGNED NOT NULL,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (operation_id,listing_id),
    KEY idx_collector_ledger_listing (listing_id,listing_revision),
    KEY idx_collector_ledger_actor (actor_pid,created_at),
    KEY idx_collector_ledger_item (item_uid,created_at),
    CONSTRAINT chk_collector_ledger_listing CHECK (listing_id > 0),
    CONSTRAINT chk_collector_ledger_action CHECK (action BETWEEN 1 AND 7),
    CONSTRAINT chk_collector_ledger_catalog_revision CHECK (catalog_revision > 0),
    CONSTRAINT chk_collector_ledger_listing_revision CHECK (listing_revision > 0),
    CONSTRAINT chk_collector_ledger_item CHECK (item_uid > 0),
    CONSTRAINT chk_collector_ledger_reason CHECK (closed_reason BETWEEN 0 AND 7)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS collector_reconciliation_quarantine (
    quarantine_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    listing_id BIGINT UNSIGNED NOT NULL,
    item_uid BIGINT UNSIGNED NOT NULL DEFAULT 0,
    conflict_code SMALLINT UNSIGNED NOT NULL,
    evidence VARCHAR(255) NOT NULL,
    detected_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    repaired_at TIMESTAMP(6) NULL,
    PRIMARY KEY (quarantine_id),
    UNIQUE KEY uq_collector_quarantine (listing_id,item_uid,conflict_code),
    KEY idx_collector_quarantine_open (repaired_at,listing_id),
    CONSTRAINT chk_collector_quarantine_listing CHECK (listing_id > 0),
    CONSTRAINT chk_collector_quarantine_conflict CHECK (conflict_code > 0),
    CONSTRAINT chk_collector_quarantine_evidence CHECK (CHAR_LENGTH(evidence) > 0)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
