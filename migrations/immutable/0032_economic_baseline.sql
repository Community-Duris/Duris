-- Additive baseline retention only. No epochs, mappings, native balances or
-- activation state are created. Complete witnesses are verified by the typed
-- lifecycle transaction; SQL constraints do not authenticate a native snapshot.
CREATE TABLE IF NOT EXISTS economic_baseline_control (
    lineage BINARY(16) NOT NULL,
    epoch BINARY(16) NOT NULL,
    opening_account VARBINARY(40) NOT NULL,
    creating_operation_id BINARY(16) NOT NULL,
    revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    last_operation_id BINARY(16) NULL,
    PRIMARY KEY (lineage,epoch),
    CONSTRAINT ck_economic_baseline_opening CHECK (OCTET_LENGTH(opening_account)=40),
    CONSTRAINT ck_economic_baseline_control_revision CHECK (
        (revision=0 AND last_operation_id IS NULL) OR
        (revision>0 AND last_operation_id IS NOT NULL)),
    CONSTRAINT fk_economic_baseline_control_epoch FOREIGN KEY (lineage,epoch)
        REFERENCES economic_epoch(lineage,epoch) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT fk_economic_baseline_control_create FOREIGN KEY (creating_operation_id)
        REFERENCES critical_operation_inbox(operation_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT fk_economic_baseline_control_last FOREIGN KEY (last_operation_id)
        REFERENCES critical_operation_inbox(operation_id) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS economic_baseline_witness (
    operation_id BINARY(16) NOT NULL,
    lineage BINARY(16) NOT NULL,
    epoch BINARY(16) NOT NULL,
    book_revision BIGINT UNSIGNED NOT NULL,
    witness_version SMALLINT UNSIGNED NOT NULL,
    holding_count SMALLINT UNSIGNED NOT NULL,
    item_count SMALLINT UNSIGNED NOT NULL,
    witness_digest BINARY(32) NOT NULL,
    canonical_witness MEDIUMBLOB NOT NULL,
    PRIMARY KEY (operation_id),
    UNIQUE KEY uq_economic_baseline_witness_epoch (lineage,epoch,operation_id),
    UNIQUE KEY uq_economic_baseline_witness_revision (lineage,epoch,book_revision),
    CONSTRAINT ck_economic_baseline_witness_revision CHECK (book_revision>0),
    CONSTRAINT ck_economic_baseline_witness_shape CHECK (
        witness_version=1 AND holding_count<=3071 AND item_count<=6000 AND
        OCTET_LENGTH(canonical_witness)=192+112*holding_count+88*item_count AND
        SUBSTRING(canonical_witness,1,4)=X'45414231'),
    CONSTRAINT fk_economic_baseline_witness_control FOREIGN KEY (lineage,epoch)
        REFERENCES economic_baseline_control(lineage,epoch) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT fk_economic_baseline_witness_operation FOREIGN KEY (operation_id)
        REFERENCES economic_accounting_operation(operation_id) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Account lifetimes share one namespace across all ordinary kinds/contexts.
-- Item UIDs have a separate namespace. Neither native revision nor preparation
-- ID weakens per-epoch uniqueness; empty batches still retain a witness receipt.
CREATE TABLE IF NOT EXISTS economic_baseline_reservation (
    lineage BINARY(16) NOT NULL,
    epoch BINARY(16) NOT NULL,
    identity_kind TINYINT UNSIGNED NOT NULL,
    identity_id BIGINT UNSIGNED NOT NULL,
    operation_id BINARY(16) NOT NULL,
    PRIMARY KEY (lineage,epoch,identity_kind,identity_id),
    KEY idx_economic_baseline_reservation_operation (lineage,epoch,operation_id),
    CONSTRAINT ck_economic_baseline_reservation_identity CHECK (identity_kind IN (1,2) AND identity_id>0),
    CONSTRAINT fk_economic_baseline_reservation_witness FOREIGN KEY (lineage,epoch,operation_id)
        REFERENCES economic_baseline_witness(lineage,epoch,operation_id) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
