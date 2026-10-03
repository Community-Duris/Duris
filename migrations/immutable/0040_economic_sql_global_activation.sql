-- Global cutover decision, separate from 0036's wallet-root qualification receipt.
-- This migration selects no epoch and creates no decision. A lifecycle owner must
-- write this row and economic_lineage_state.active_epoch in one transaction.
CREATE TABLE IF NOT EXISTS economic_sql_global_activation (
    lineage BINARY(16) NOT NULL,
    epoch BINARY(16) NOT NULL,
    installation_operation_id BINARY(16) NOT NULL,
    baseline_operation_id BINARY(16) NOT NULL,
    manifest_digest BINARY(32) NOT NULL,
    audit_digest BINARY(32) NOT NULL,
    route_count BIGINT UNSIGNED NOT NULL,
    verified_route_count BIGINT UNSIGNED NOT NULL,
    unclassified_route_count BIGINT UNSIGNED NOT NULL,
    state TINYINT UNSIGNED NOT NULL,
    revision BIGINT UNSIGNED NOT NULL,
    decided_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    updated_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)
        ON UPDATE CURRENT_TIMESTAMP(6),
    PRIMARY KEY (lineage),
    KEY idx_economic_sql_global_activation_installation (installation_operation_id),
    KEY idx_economic_sql_global_activation_epoch (lineage,epoch),
    KEY idx_economic_sql_global_activation_baseline (baseline_operation_id),
    CONSTRAINT ck_economic_sql_global_activation_evidence CHECK (
        manifest_digest <> 0x0000000000000000000000000000000000000000000000000000000000000000
        AND audit_digest <> 0x0000000000000000000000000000000000000000000000000000000000000000
        AND route_count > 0 AND verified_route_count = route_count
        AND unclassified_route_count = 0),
    CONSTRAINT ck_economic_sql_global_activation_state
        CHECK (state IN (1,2) AND revision > 0),
    CONSTRAINT fk_economic_sql_global_activation_installation
        FOREIGN KEY (installation_operation_id)
        REFERENCES economic_sql_lifecycle_installation(operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT fk_economic_sql_global_activation_epoch FOREIGN KEY (lineage,epoch)
        REFERENCES economic_epoch(lineage,epoch) ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT fk_economic_sql_global_activation_baseline FOREIGN KEY (baseline_operation_id)
        REFERENCES economic_baseline_witness(operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
