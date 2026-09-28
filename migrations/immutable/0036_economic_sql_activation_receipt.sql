-- Immutable migration 0036: receipt schema prerequisite only.
-- This creates no receipt, changes no active_epoch pointer, and does not seed or
-- alter installation, baseline, native, or gameplay data. Scope value 1 is the
-- fixed qualification_sql_wallet_root_v1 scope; no global scope is supported.
-- The lifecycle composite key is deliberately installed here, after immutable
-- 0033's exact historical verifier has run during ordered fresh replay.

DELIMITER //
DROP PROCEDURE IF EXISTS migrate_0036_activation_binding_index//
CREATE PROCEDURE migrate_0036_activation_binding_index()
BEGIN
    DECLARE index_entries INT DEFAULT 0;
    DECLARE matching_entries INT DEFAULT 0;

    SELECT COUNT(*), COALESCE(SUM(
        non_unique = 0 AND seq_in_index BETWEEN 1 AND 4 AND
        ((seq_in_index = 1 AND column_name = 'operation_id') OR
         (seq_in_index = 2 AND column_name = 'lineage') OR
         (seq_in_index = 3 AND column_name = 'epoch') OR
         (seq_in_index = 4 AND column_name = 'baseline_operation_id')) AND
        COALESCE(sub_part, 0) = 0 AND index_type = 'BTREE'), 0)
      INTO index_entries, matching_entries
    FROM information_schema.statistics
    WHERE table_schema = DATABASE()
      AND table_name = 'economic_sql_lifecycle_installation'
      AND index_name = 'uq_economic_sql_lifecycle_activation_binding';

    IF index_entries = 0 THEN
        ALTER TABLE economic_sql_lifecycle_installation
            ADD UNIQUE KEY uq_economic_sql_lifecycle_activation_binding
                (operation_id, lineage, epoch, baseline_operation_id);
    ELSEIF index_entries <> 4 OR matching_entries <> 4 THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = '0036 lifecycle activation binding index has malformed shape';
    END IF;
END//
CALL migrate_0036_activation_binding_index()//
DROP PROCEDURE migrate_0036_activation_binding_index//
DELIMITER ;

CREATE TABLE IF NOT EXISTS economic_sql_activation_receipt (
    operation_id BINARY(16) NOT NULL,
    lineage BINARY(16) NOT NULL,
    epoch BINARY(16) NOT NULL,
    baseline_operation_id BINARY(16) NOT NULL,
    baseline_revision BIGINT UNSIGNED NOT NULL,
    source_capture_digest BINARY(32) NOT NULL,
    native_boundary_digest BINARY(32) NOT NULL,
    activation_scope TINYINT UNSIGNED NOT NULL,
    coverage_contract_version SMALLINT UNSIGNED NOT NULL,
    coverage_evidence_digest BINARY(32) NOT NULL,
    activation_digest BINARY(32) NOT NULL,
    receipt_version SMALLINT UNSIGNED NOT NULL,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (operation_id),
    UNIQUE KEY uq_economic_sql_activation_lineage (lineage),
    KEY idx_economic_sql_activation_install_binding
        (operation_id, lineage, epoch, baseline_operation_id),
    KEY idx_economic_sql_activation_epoch_revision
        (lineage, epoch, baseline_revision),
    KEY idx_economic_sql_activation_baseline_operation (baseline_operation_id),
    CONSTRAINT ck_economic_sql_activation_operation_nonzero
        CHECK (operation_id <> 0x00000000000000000000000000000000),
    CONSTRAINT ck_economic_sql_activation_lineage_nonzero
        CHECK (lineage <> 0x00000000000000000000000000000000),
    CONSTRAINT ck_economic_sql_activation_epoch_nonzero
        CHECK (epoch <> 0x00000000000000000000000000000000),
    CONSTRAINT ck_economic_sql_activation_baseline_operation_nonzero
        CHECK (baseline_operation_id <> 0x00000000000000000000000000000000),
    CONSTRAINT ck_economic_sql_activation_baseline_revision
        CHECK (baseline_revision > 0),
    CONSTRAINT ck_economic_sql_activation_scope
        CHECK (activation_scope = 1),
    CONSTRAINT ck_economic_sql_activation_coverage_version
        CHECK (coverage_contract_version = 1),
    CONSTRAINT ck_economic_sql_activation_receipt_version
        CHECK (receipt_version = 1),
    CONSTRAINT fk_economic_sql_activation_installation
        FOREIGN KEY (operation_id, lineage, epoch, baseline_operation_id)
        REFERENCES economic_sql_lifecycle_installation
            (operation_id, lineage, epoch, baseline_operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT fk_economic_sql_activation_epoch
        FOREIGN KEY (lineage, epoch)
        REFERENCES economic_epoch (lineage, epoch)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT fk_economic_sql_activation_baseline_revision
        FOREIGN KEY (lineage, epoch, baseline_revision)
        REFERENCES economic_baseline_witness (lineage, epoch, book_revision)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT fk_economic_sql_activation_operation_inbox
        FOREIGN KEY (operation_id)
        REFERENCES critical_operation_inbox (operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT fk_economic_sql_activation_baseline_inbox
        FOREIGN KEY (baseline_operation_id)
        REFERENCES critical_operation_inbox (operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- A fresh bootstrap carries the receipt columns and all local constraints, but
-- intentionally defers this one FK until 0036 installs its referenced index.
-- Older/pre-existing tables are treated the same way: add only when absent and
-- refuse an unexpected same-name constraint without rewriting it.
DELIMITER //
DROP PROCEDURE IF EXISTS migrate_0036_activation_installation_fk//
CREATE PROCEDURE migrate_0036_activation_installation_fk()
BEGIN
    DECLARE constraint_entries INT DEFAULT 0;
    DECLARE matching_entries INT DEFAULT 0;
    DECLARE other_installation_fks INT DEFAULT 0;

    SELECT COUNT(*), COALESCE(SUM(
        k.ordinal_position BETWEEN 1 AND 4 AND
        ((k.ordinal_position = 1 AND k.column_name = 'operation_id' AND
          k.referenced_column_name = 'operation_id') OR
         (k.ordinal_position = 2 AND k.column_name = 'lineage' AND
          k.referenced_column_name = 'lineage') OR
         (k.ordinal_position = 3 AND k.column_name = 'epoch' AND
          k.referenced_column_name = 'epoch') OR
         (k.ordinal_position = 4 AND k.column_name = 'baseline_operation_id' AND
          k.referenced_column_name = 'baseline_operation_id')) AND
        k.referenced_table_name = 'economic_sql_lifecycle_installation' AND
        k.referenced_table_schema = DATABASE() AND
        r.update_rule = 'RESTRICT' AND r.delete_rule = 'RESTRICT'), 0)
      INTO constraint_entries, matching_entries
    FROM information_schema.key_column_usage k
    JOIN information_schema.referential_constraints r
      ON r.constraint_schema = k.constraint_schema
     AND r.table_name = k.table_name
     AND r.constraint_name = k.constraint_name
    WHERE k.constraint_schema = DATABASE()
      AND k.table_name = 'economic_sql_activation_receipt'
      AND k.constraint_name = 'fk_economic_sql_activation_installation'
      AND k.referenced_table_name IS NOT NULL;

    IF constraint_entries = 0 THEN
        SELECT COUNT(*)
          INTO other_installation_fks
        FROM information_schema.key_column_usage
        WHERE constraint_schema = DATABASE()
          AND table_name = 'economic_sql_activation_receipt'
          AND referenced_table_name = 'economic_sql_lifecycle_installation'
          AND constraint_name <> 'fk_economic_sql_activation_installation';
        IF other_installation_fks <> 0 THEN
            SIGNAL SQLSTATE '45000'
                SET MESSAGE_TEXT = '0036 unexpected activation receipt installation foreign key';
        END IF;
        ALTER TABLE economic_sql_activation_receipt
            ADD CONSTRAINT fk_economic_sql_activation_installation
            FOREIGN KEY (operation_id, lineage, epoch, baseline_operation_id)
            REFERENCES economic_sql_lifecycle_installation
                (operation_id, lineage, epoch, baseline_operation_id)
            ON UPDATE RESTRICT ON DELETE RESTRICT;
    ELSEIF constraint_entries <> 4 OR matching_entries <> 4 THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = '0036 activation receipt installation foreign key has malformed shape';
    END IF;
END//
CALL migrate_0036_activation_installation_fk()//
DROP PROCEDURE migrate_0036_activation_installation_fk//
DELIMITER ;
