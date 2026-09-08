-- Phase 02 durable item identity, current custody, revisions, and immutable ledger.
-- Additive and re-runnable. Legacy baseline population is a separate guarded action.

CREATE TABLE IF NOT EXISTS item_uid_allocator (
    allocator_id TINYINT UNSIGNED NOT NULL,
    next_uid BIGINT UNSIGNED NOT NULL,
    updated_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)
        ON UPDATE CURRENT_TIMESTAMP(6),
    PRIMARY KEY (allocator_id),
    CONSTRAINT chk_item_uid_allocator_singleton CHECK (allocator_id = 1),
    CONSTRAINT chk_item_uid_allocator_nonzero CHECK (next_uid > 0)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

INSERT IGNORE INTO item_uid_allocator(allocator_id,next_uid) VALUES(1,1);

CREATE TABLE IF NOT EXISTS item_owner_revision (
    owner_type TINYINT UNSIGNED NOT NULL,
    owner_id BIGINT UNSIGNED NOT NULL,
    owner_context_id BIGINT UNSIGNED NOT NULL DEFAULT 0,
    revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    updated_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)
        ON UPDATE CURRENT_TIMESTAMP(6),
    PRIMARY KEY (owner_type,owner_id,owner_context_id),
    KEY idx_item_owner_revision_updated (updated_at),
    CONSTRAINT chk_item_owner_revision_type CHECK (owner_type BETWEEN 1 AND 9)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS item_current_owner (
    item_uid BIGINT UNSIGNED NOT NULL,
    root_item_uid BIGINT UNSIGNED NOT NULL,
    parent_item_uid BIGINT UNSIGNED NULL,
    owner_type TINYINT UNSIGNED NOT NULL,
    owner_id BIGINT UNSIGNED NOT NULL,
    owner_context_id BIGINT UNSIGNED NOT NULL DEFAULT 0,
    item_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    vnum INT NOT NULL DEFAULT 0,
    state TINYINT UNSIGNED NOT NULL DEFAULT 1,
    updated_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)
        ON UPDATE CURRENT_TIMESTAMP(6),
    PRIMARY KEY (item_uid),
    KEY idx_item_current_root_uid (root_item_uid,item_uid),
    KEY idx_item_current_owner (owner_type,owner_id,owner_context_id,item_uid),
    KEY idx_item_current_parent (parent_item_uid),
    CONSTRAINT chk_item_current_uid_nonzero CHECK (item_uid > 0 AND root_item_uid > 0),
    CONSTRAINT chk_item_current_owner_type CHECK (owner_type BETWEEN 1 AND 9),
    CONSTRAINT chk_item_current_state CHECK (state BETWEEN 1 AND 3),
    CONSTRAINT item_current_parent_fk FOREIGN KEY (parent_item_uid)
        REFERENCES item_current_owner(item_uid) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Coin conversions commit their complete pile payload with custody. Player,
-- corpse, locker and floor snapshots are projections and can lag that commit.
SET @coin_payload_missing = (SELECT COUNT(*) = 0 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='item_current_owner'
      AND column_name='coin_payload');
SET @coin_payload_sql = IF(@coin_payload_missing,
    'ALTER TABLE item_current_owner ADD COLUMN coin_payload MEDIUMBLOB NULL AFTER state',
    'SELECT 1 INTO @coin_payload_unchanged');
PREPARE coin_payload_stmt FROM @coin_payload_sql;
EXECUTE coin_payload_stmt;
DEALLOCATE PREPARE coin_payload_stmt;

-- Existing columns must satisfy the same contract as a newly added column.
DROP PROCEDURE IF EXISTS duris_verify_coin_payload;
DELIMITER //
CREATE PROCEDURE duris_verify_coin_payload()
BEGIN
    IF (SELECT COUNT(*) FROM information_schema.columns
        WHERE table_schema=DATABASE() AND table_name='item_current_owner'
          AND column_name='coin_payload' AND data_type='mediumblob'
          AND is_nullable='YES' AND ordinal_position=10
          AND (column_default IS NULL OR column_default='NULL')) <> 1 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='incompatible item_current_owner.coin_payload';
    END IF;
END//
DELIMITER ;
CALL duris_verify_coin_payload();
DROP PROCEDURE duris_verify_coin_payload;


CREATE TABLE IF NOT EXISTS item_ownership_baseline (
    item_uid BIGINT UNSIGNED NOT NULL,
    root_item_uid BIGINT UNSIGNED NOT NULL,
    parent_item_uid BIGINT UNSIGNED NULL,
    owner_type TINYINT UNSIGNED NOT NULL,
    owner_id BIGINT UNSIGNED NOT NULL,
    owner_context_id BIGINT UNSIGNED NOT NULL DEFAULT 0,
    opening_item_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    vnum INT NOT NULL DEFAULT 0,
    source_table VARCHAR(32) NOT NULL,
    source_row_id BIGINT UNSIGNED NOT NULL,
    captured_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (item_uid),
    UNIQUE KEY uq_item_baseline_source (source_table,source_row_id),
    KEY idx_item_baseline_owner (owner_type,owner_id,owner_context_id),
    CONSTRAINT chk_item_baseline_owner_type CHECK (owner_type BETWEEN 1 AND 9)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS item_ownership_quarantine (
    quarantine_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    item_uid BIGINT UNSIGNED NOT NULL,
    source_table VARCHAR(32) NOT NULL,
    source_row_id BIGINT UNSIGNED NOT NULL,
    conflict_code SMALLINT UNSIGNED NOT NULL,
    evidence VARCHAR(255) NOT NULL,
    detected_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    repaired_at TIMESTAMP(6) NULL,
    PRIMARY KEY (quarantine_id),
    UNIQUE KEY uq_item_quarantine_evidence (item_uid,source_table,source_row_id,conflict_code),
    KEY idx_item_quarantine_open (repaired_at,item_uid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS item_ownership_ledger (
    operation_id BINARY(16) NOT NULL,
    event_index SMALLINT UNSIGNED NOT NULL,
    item_uid BIGINT UNSIGNED NOT NULL,
    root_item_uid BIGINT UNSIGNED NOT NULL,
    parent_item_uid BIGINT UNSIGNED NULL,
    from_owner_type TINYINT UNSIGNED NOT NULL,
    from_owner_id BIGINT UNSIGNED NOT NULL,
    from_owner_context_id BIGINT UNSIGNED NOT NULL,
    to_owner_type TINYINT UNSIGNED NOT NULL,
    to_owner_id BIGINT UNSIGNED NOT NULL,
    to_owner_context_id BIGINT UNSIGNED NOT NULL,
    item_revision BIGINT UNSIGNED NOT NULL,
    from_owner_revision BIGINT UNSIGNED NOT NULL,
    to_owner_revision BIGINT UNSIGNED NOT NULL,
    reason_type SMALLINT UNSIGNED NOT NULL,
    reason_id BIGINT NOT NULL DEFAULT 0,
    source_site SMALLINT UNSIGNED NOT NULL,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (operation_id,event_index),
    UNIQUE KEY uq_item_ledger_item_revision (item_uid,item_revision),
    KEY idx_item_ledger_item_created (item_uid,created_at),
    KEY idx_item_ledger_from_owner (from_owner_type,from_owner_id,from_owner_context_id,created_at),
    KEY idx_item_ledger_to_owner (to_owner_type,to_owner_id,to_owner_context_id,created_at),
    CONSTRAINT item_ownership_operation_fk FOREIGN KEY (operation_id)
        REFERENCES critical_operation_inbox(operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Never reserve below an identity already present in retained custody storage. This
-- only advances the singleton and is safe to repeat.
UPDATE item_uid_allocator
SET next_uid=GREATEST(next_uid,(
    SELECT COALESCE(MAX(item_uid),0)+1 FROM (
        SELECT obj_uid AS item_uid FROM player_items WHERE obj_uid IS NOT NULL
        UNION ALL SELECT obj_uid FROM corpse_items WHERE obj_uid IS NOT NULL
        UNION ALL SELECT obj_uid FROM locker_items WHERE obj_uid IS NOT NULL
        UNION ALL SELECT obj_uid FROM account_locker_items WHERE obj_uid IS NOT NULL
        UNION ALL SELECT obj_uid FROM saved_items WHERE obj_uid IS NOT NULL
        UNION ALL SELECT obj_uid FROM player_pet_items WHERE obj_uid IS NOT NULL
        UNION ALL SELECT obj_uid FROM shopkeeper_items WHERE obj_uid IS NOT NULL
        UNION ALL SELECT obj_uid FROM siege_items WHERE obj_uid IS NOT NULL
        UNION ALL SELECT item_uid FROM item_current_owner
    ) retained_item_uids
)) WHERE allocator_id=1;
