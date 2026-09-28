-- Additive economic evidence and retained identity metadata.
-- No baseline, active epoch, account mapping or gameplay value is seeded here.
-- Provisional 0031 after canonical telemetry 0030; recheck allocation before merge.

CREATE TABLE IF NOT EXISTS economic_epoch (
    lineage BINARY(16) NOT NULL,
    epoch BINARY(16) NOT NULL,
    ordinal BIGINT UNSIGNED NOT NULL,
    predecessor BINARY(16) NULL,
    transition_kind SMALLINT UNSIGNED NOT NULL,
    transition_digest BINARY(32) NOT NULL,
    creating_operation_id BINARY(16) NOT NULL,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (lineage,epoch),
    UNIQUE KEY uq_economic_epoch_ordinal (lineage,ordinal),
    CONSTRAINT chk_economic_epoch_ordinal CHECK (ordinal > 0),
    CONSTRAINT chk_economic_epoch_transition CHECK (transition_kind > 0),
    CONSTRAINT chk_economic_epoch_predecessor CHECK (predecessor IS NULL OR predecessor <> epoch),
    CONSTRAINT economic_epoch_predecessor_fk FOREIGN KEY (lineage,predecessor)
        REFERENCES economic_epoch(lineage,epoch) ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT economic_epoch_operation_fk FOREIGN KEY (creating_operation_id)
        REFERENCES critical_operation_inbox(operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS economic_lineage_state (
    lineage BINARY(16) NOT NULL,
    active_epoch BINARY(16) NULL,
    revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (lineage),
    CONSTRAINT economic_lineage_epoch_fk FOREIGN KEY (lineage,active_epoch)
        REFERENCES economic_epoch(lineage,epoch) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS economic_account_mapping (
    mapping_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    lineage BINARY(16) NOT NULL,
    account_kind SMALLINT UNSIGNED NOT NULL,
    context_id BIGINT UNSIGNED NOT NULL DEFAULT 0,
    backend_kind TINYINT UNSIGNED NOT NULL,
    locator_kind SMALLINT UNSIGNED NOT NULL,
    native_id BIGINT UNSIGNED NOT NULL,
    active_native_id BIGINT UNSIGNED NULL,
    creating_operation_id BINARY(16) NOT NULL,
    retiring_operation_id BINARY(16) NULL,
    revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (mapping_id),
    UNIQUE KEY uq_economic_active_mapping
        (lineage,backend_kind,locator_kind,account_kind,context_id,active_native_id),
    KEY idx_economic_retained_mapping
        (lineage,backend_kind,locator_kind,native_id,mapping_id),
    CONSTRAINT chk_economic_mapping_kind CHECK (account_kind BETWEEN 1 AND 6),
    CONSTRAINT chk_economic_mapping_backend CHECK (backend_kind IN (1,2)),
    CONSTRAINT chk_economic_mapping_locator CHECK (locator_kind > 0 AND native_id > 0),
    CONSTRAINT chk_economic_mapping_active CHECK (
        (active_native_id IS NOT NULL AND active_native_id = native_id AND retiring_operation_id IS NULL)
        OR (active_native_id IS NULL AND retiring_operation_id IS NOT NULL)),
    CONSTRAINT economic_mapping_lineage_fk FOREIGN KEY (lineage)
        REFERENCES economic_lineage_state(lineage) ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT economic_mapping_creation_fk FOREIGN KEY (creating_operation_id)
        REFERENCES critical_operation_inbox(operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT economic_mapping_retirement_fk FOREIGN KEY (retiring_operation_id)
        REFERENCES critical_operation_inbox(operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS economic_accounting_operation (
    operation_id BINARY(16) NOT NULL,
    lineage BINARY(16) NOT NULL,
    epoch BINARY(16) NOT NULL,
    original_operation_id BINARY(16) NULL,
    accounting_version SMALLINT UNSIGNED NOT NULL,
    writer_id INT UNSIGNED NOT NULL,
    policy_version INT UNSIGNED NOT NULL,
    compiler_version INT UNSIGNED NOT NULL,
    actor_kind TINYINT UNSIGNED NOT NULL,
    actor_id BIGINT UNSIGNED NOT NULL,
    reason SMALLINT UNSIGNED NOT NULL,
    source_event BINARY(48) NULL,
    intent_digest BINARY(32) NOT NULL,
    domain_digest BINARY(32) NOT NULL,
    plan_digest BINARY(32) NULL,
    canonical_intent VARBINARY(8192) NOT NULL,
    canonical_plan MEDIUMBLOB NULL,
    outcome TINYINT UNSIGNED NOT NULL,
    result_code INT UNSIGNED NOT NULL,
    account_count SMALLINT UNSIGNED NOT NULL,
    posting_count SMALLINT UNSIGNED NOT NULL,
    child_count SMALLINT UNSIGNED NOT NULL,
    item_event_count SMALLINT UNSIGNED NOT NULL,
    before_witness_count SMALLINT UNSIGNED NOT NULL,
    after_witness_count SMALLINT UNSIGNED NOT NULL,
    recorded_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (operation_id),
    KEY idx_economic_operation_reason (lineage,epoch,reason,recorded_at,operation_id),
    KEY idx_economic_operation_original (original_operation_id,operation_id),
    UNIQUE KEY uq_economic_operation_source (operation_id,lineage,source_event,outcome),
    CONSTRAINT chk_economic_operation_version CHECK
        (accounting_version = 1 AND writer_id > 0 AND policy_version > 0 AND compiler_version > 0),
    CONSTRAINT chk_economic_operation_actor CHECK (actor_kind IN (1,2) AND actor_id > 0),
    CONSTRAINT chk_economic_operation_reason CHECK (reason BETWEEN 1 AND 42),
    CONSTRAINT chk_economic_operation_intent CHECK (OCTET_LENGTH(canonical_intent) BETWEEN 256 AND 8192),
    CONSTRAINT chk_economic_operation_counts CHECK
        (account_count <= 3072 AND posting_count <= 6144 AND child_count <= 64
         AND item_event_count <= 3000 AND before_witness_count <= 6000 AND after_witness_count <= 6000),
    CONSTRAINT chk_economic_operation_outcome CHECK (
        (outcome = 1 AND result_code = 0 AND plan_digest IS NOT NULL
         AND canonical_plan IS NOT NULL AND OCTET_LENGTH(canonical_plan) BETWEEN 256 AND 4194304)
        OR (outcome = 2 AND result_code <> 0 AND plan_digest IS NULL AND canonical_plan IS NULL
            AND account_count = 0 AND posting_count = 0 AND child_count = 0
            AND item_event_count = 0 AND before_witness_count = 0 AND after_witness_count = 0)),
    CONSTRAINT economic_operation_inbox_fk FOREIGN KEY (operation_id)
        REFERENCES critical_operation_inbox(operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT economic_operation_epoch_fk FOREIGN KEY (lineage,epoch)
        REFERENCES economic_epoch(lineage,epoch) ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT economic_operation_original_fk FOREIGN KEY (original_operation_id)
        REFERENCES critical_operation_inbox(operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS economic_accounting_account_effect (
    operation_id BINARY(16) NOT NULL,
    account_index SMALLINT UNSIGNED NOT NULL,
    account_key BINARY(40) NOT NULL,
    before_copper BIGINT NOT NULL,
    before_silver BIGINT NOT NULL,
    before_gold BIGINT NOT NULL,
    before_platinum BIGINT NOT NULL,
    after_copper BIGINT NOT NULL,
    after_silver BIGINT NOT NULL,
    after_gold BIGINT NOT NULL,
    after_platinum BIGINT NOT NULL,
    before_revision BIGINT UNSIGNED NOT NULL,
    after_revision BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (operation_id,account_index),
    UNIQUE KEY uq_economic_effect_account (operation_id,account_key),
    KEY idx_economic_effect_history (account_key,operation_id),
    CONSTRAINT chk_economic_effect_index CHECK (account_index < 3072),
    CONSTRAINT economic_effect_operation_fk FOREIGN KEY (operation_id)
        REFERENCES economic_accounting_operation(operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS economic_accounting_coin_posting (
    operation_id BINARY(16) NOT NULL,
    line_index SMALLINT UNSIGNED NOT NULL,
    event_index INT UNSIGNED NOT NULL,
    account_index SMALLINT UNSIGNED NOT NULL,
    child_index SMALLINT UNSIGNED NOT NULL,
    delta_copper BIGINT NOT NULL,
    delta_silver BIGINT NOT NULL,
    delta_gold BIGINT NOT NULL,
    delta_platinum BIGINT NOT NULL,
    copper_value BIGINT NOT NULL,
    PRIMARY KEY (operation_id,line_index),
    UNIQUE KEY uq_economic_posting_event (operation_id,event_index),
    CONSTRAINT chk_economic_posting_indexes CHECK
        (line_index < 6144 AND event_index = line_index AND account_index < 3072 AND child_index <= 64),
    CONSTRAINT economic_posting_account_fk FOREIGN KEY (operation_id,account_index)
        REFERENCES economic_accounting_account_effect(operation_id,account_index)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS economic_accounting_child (
    operation_id BINARY(16) NOT NULL,
    child_index SMALLINT UNSIGNED NOT NULL,
    child_operation_id BINARY(16) NOT NULL,
    domain_id INT UNSIGNED NOT NULL,
    discriminator BIGINT UNSIGNED NOT NULL,
    parent_index SMALLINT UNSIGNED NOT NULL,
    relationship SMALLINT UNSIGNED NOT NULL,
    receipt_operation_id BINARY(16) NULL,
    PRIMARY KEY (operation_id,child_index),
    UNIQUE KEY uq_economic_child_operation (child_operation_id),
    CONSTRAINT chk_economic_child_indexes CHECK
        (child_index BETWEEN 1 AND 64 AND parent_index < child_index),
    CONSTRAINT chk_economic_child_relationship CHECK
        (relationship = 1 AND domain_id > 0 AND child_operation_id <> operation_id),
    CONSTRAINT chk_economic_child_receipt CHECK
        (receipt_operation_id IS NULL OR receipt_operation_id = child_operation_id),
    CONSTRAINT economic_child_root_fk FOREIGN KEY (operation_id)
        REFERENCES economic_accounting_operation(operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT economic_child_receipt_fk FOREIGN KEY (receipt_operation_id)
        REFERENCES critical_operation_inbox(operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Bind the item reference's indexed UID/revision to the original immutable event.
-- This additive index does not rewrite or duplicate the existing ledger.
SET @economic_item_reference_index = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema=DATABASE() AND table_name='item_ownership_ledger'
      AND index_name='uq_item_ledger_accounting_reference');
SET @economic_item_reference_sql = IF(@economic_item_reference_index=0,
    'CREATE UNIQUE INDEX uq_item_ledger_accounting_reference ON item_ownership_ledger(operation_id,event_index,item_uid,item_revision)',
    'SELECT 1');
PREPARE economic_item_reference_stmt FROM @economic_item_reference_sql;
EXECUTE economic_item_reference_stmt;
DEALLOCATE PREPARE economic_item_reference_stmt;

CREATE TABLE IF NOT EXISTS economic_accounting_item_reference (
    operation_id BINARY(16) NOT NULL,
    line_index SMALLINT UNSIGNED NOT NULL,
    event_index INT UNSIGNED NOT NULL,
    child_index SMALLINT UNSIGNED NOT NULL,
    item_uid BIGINT UNSIGNED NOT NULL,
    before_revision BIGINT UNSIGNED NOT NULL,
    after_revision BIGINT UNSIGNED NOT NULL,
    legacy_operation_id BINARY(16) NOT NULL,
    legacy_event_index SMALLINT UNSIGNED NOT NULL,
    PRIMARY KEY (operation_id,line_index),
    UNIQUE KEY uq_economic_item_event (operation_id,event_index),
    UNIQUE KEY uq_economic_item_legacy (legacy_operation_id,legacy_event_index),
    KEY idx_economic_item_history (item_uid,after_revision),
    CONSTRAINT chk_economic_item_indexes CHECK
        (line_index < 3000 AND event_index = line_index AND child_index <= 64
         AND item_uid > 0 AND after_revision > before_revision),
    CONSTRAINT economic_item_operation_fk FOREIGN KEY (operation_id)
        REFERENCES economic_accounting_operation(operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT economic_item_legacy_fk FOREIGN KEY (legacy_operation_id,legacy_event_index,item_uid,after_revision)
        REFERENCES item_ownership_ledger(operation_id,event_index,item_uid,item_revision) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS economic_accounting_source_claim (
    lineage BINARY(16) NOT NULL,
    source_event BINARY(48) NOT NULL,
    operation_id BINARY(16) NOT NULL,
    outcome TINYINT UNSIGNED NOT NULL DEFAULT 1,
    PRIMARY KEY (lineage,source_event),
    UNIQUE KEY uq_economic_source_operation (operation_id),
    CONSTRAINT chk_economic_source_success CHECK (outcome = 1),
    CONSTRAINT economic_source_operation_fk FOREIGN KEY (operation_id,lineage,source_event,outcome)
        REFERENCES economic_accounting_operation(operation_id,lineage,source_event,outcome)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
