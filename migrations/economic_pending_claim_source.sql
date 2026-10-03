-- Source allocations for the shared auction_money_pickups aggregate. A producer
-- records one row in the same transaction as its native credit and accounting
-- root. A claim links every consumed source to its own accounting root.
CREATE TABLE IF NOT EXISTS economic_pending_claim_source (
    source_operation_id BINARY(16) NOT NULL,
    source_slot SMALLINT UNSIGNED NOT NULL,
    lineage BINARY(16) NOT NULL,
    claim_mapping_id BIGINT UNSIGNED NOT NULL,
    beneficiary_pid INT UNSIGNED NOT NULL,
    amount BIGINT UNSIGNED NOT NULL,
    claim_operation_id BINARY(16) NULL,
    PRIMARY KEY (source_operation_id, source_slot),
    KEY idx_economic_pending_claim_open
        (beneficiary_pid, claim_operation_id, source_operation_id, source_slot),
    KEY idx_economic_pending_claim_mapping
        (lineage, claim_mapping_id, claim_operation_id),
    KEY idx_economic_pending_claim_consumed (claim_operation_id),
    CONSTRAINT chk_economic_pending_claim_source
        CHECK (source_slot > 0 AND beneficiary_pid > 0 AND amount > 0),
    CONSTRAINT economic_pending_source_operation_fk
        FOREIGN KEY (source_operation_id)
        REFERENCES economic_accounting_operation(operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT economic_pending_claim_mapping_fk
        FOREIGN KEY (claim_mapping_id)
        REFERENCES economic_account_mapping(mapping_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT economic_pending_claim_operation_fk
        FOREIGN KEY (claim_operation_id)
        REFERENCES economic_accounting_operation(operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
