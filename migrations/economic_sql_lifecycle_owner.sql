-- SQL wallet/shared-bank baseline installation receipt.
-- This records a durable maintenance cutover selection only; it deliberately
-- does not set economic_lineage_state.active_epoch or enable gameplay coverage.
CREATE TABLE IF NOT EXISTS economic_sql_lifecycle_installation (
    operation_id BINARY(16) NOT NULL,
    lineage BINARY(16) NOT NULL,
    epoch BINARY(16) NOT NULL,
    request_digest BINARY(32) NOT NULL,
    source_capture_digest BINARY(32) NOT NULL,
    native_boundary_digest BINARY(32) NOT NULL,
    baseline_operation_id BINARY(16) NULL,
    wallet_count INT UNSIGNED NOT NULL,
    bank_count INT UNSIGNED NOT NULL,
    phase TINYINT UNSIGNED NOT NULL,
    selected_epoch BINARY(16) NULL,
    revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (operation_id),
    UNIQUE KEY uq_economic_sql_lifecycle_lineage (lineage),
    UNIQUE KEY uq_economic_sql_lifecycle_epoch (lineage,epoch),
    CONSTRAINT ck_economic_sql_lifecycle_phase CHECK (
        (phase=1 AND selected_epoch IS NULL) OR
        (phase=2 AND selected_epoch IS NOT NULL AND selected_epoch=epoch AND baseline_operation_id IS NOT NULL)),
    CONSTRAINT ck_economic_sql_lifecycle_revision CHECK (revision<=1),
    CONSTRAINT fk_economic_sql_lifecycle_inbox FOREIGN KEY (operation_id)
        REFERENCES critical_operation_inbox(operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT fk_economic_sql_lifecycle_lineage FOREIGN KEY (lineage)
        REFERENCES economic_lineage_state(lineage) ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT fk_economic_sql_lifecycle_epoch FOREIGN KEY (lineage,epoch)
        REFERENCES economic_epoch(lineage,epoch) ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT fk_economic_sql_lifecycle_baseline FOREIGN KEY (baseline_operation_id)
        REFERENCES critical_operation_inbox(operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
