-- Phase 02 replay-safe auction lifecycle, claims, custody, and reconciliation evidence.
-- Additive and re-runnable. Existing auctions remain legacy until explicitly reconciled.

SET @auction_revision_missing=(SELECT COUNT(*)=0 FROM information_schema.columns
 WHERE table_schema=DATABASE() AND table_name='auctions' AND column_name='auction_revision');
SET @auction_revision_sql=IF(@auction_revision_missing,
 'ALTER TABLE auctions ADD COLUMN auction_revision BIGINT UNSIGNED NOT NULL DEFAULT 0',
 'SELECT 1 INTO @auction_revision_unchanged');
PREPARE auction_revision_stmt FROM @auction_revision_sql;
EXECUTE auction_revision_stmt;
DEALLOCATE PREPARE auction_revision_stmt;

SET @auction_custody_missing=(SELECT COUNT(*)=0 FROM information_schema.columns
 WHERE table_schema=DATABASE() AND table_name='auctions' AND column_name='custody_state');
SET @auction_custody_sql=IF(@auction_custody_missing,
 'ALTER TABLE auctions ADD COLUMN custody_state TINYINT UNSIGNED NOT NULL DEFAULT 0',
 'SELECT 1 INTO @auction_custody_unchanged');
PREPARE auction_custody_stmt FROM @auction_custody_sql;
EXECUTE auction_custody_stmt;
DEALLOCATE PREPARE auction_custody_stmt;

SET @auction_listing_op_missing=(SELECT COUNT(*)=0 FROM information_schema.columns
 WHERE table_schema=DATABASE() AND table_name='auctions' AND column_name='listing_operation_id');
SET @auction_listing_op_sql=IF(@auction_listing_op_missing,
 'ALTER TABLE auctions ADD COLUMN listing_operation_id BINARY(16) NULL',
 'SELECT 1 INTO @auction_listing_op_unchanged');
PREPARE auction_listing_op_stmt FROM @auction_listing_op_sql;
EXECUTE auction_listing_op_stmt;
DEALLOCATE PREPARE auction_listing_op_stmt;

SET @auction_listing_op_index_missing=(SELECT COUNT(*)=0 FROM information_schema.statistics
 WHERE table_schema=DATABASE() AND table_name='auctions' AND index_name='uq_auction_listing_operation');
SET @auction_listing_op_index_sql=IF(@auction_listing_op_index_missing,
 'ALTER TABLE auctions ADD UNIQUE KEY uq_auction_listing_operation(listing_operation_id)',
 'SELECT 1 INTO @auction_listing_op_index_unchanged');
PREPARE auction_listing_op_index_stmt FROM @auction_listing_op_index_sql;
EXECUTE auction_listing_op_index_stmt;
DEALLOCATE PREPARE auction_listing_op_index_stmt;

SET @auction_money_revision_missing=(SELECT COUNT(*)=0 FROM information_schema.columns
 WHERE table_schema=DATABASE() AND table_name='auction_money_pickups' AND column_name='claim_revision');
SET @auction_money_revision_sql=IF(@auction_money_revision_missing,
 'ALTER TABLE auction_money_pickups ADD COLUMN claim_revision BIGINT UNSIGNED NOT NULL DEFAULT 0',
 'SELECT 1 INTO @auction_money_revision_unchanged');
PREPARE auction_money_revision_stmt FROM @auction_money_revision_sql;
EXECUTE auction_money_revision_stmt;
DEALLOCATE PREPARE auction_money_revision_stmt;

CREATE TABLE IF NOT EXISTS auction_item_custody (
    auction_id INT UNSIGNED NOT NULL,
    slot SMALLINT UNSIGNED NOT NULL,
    item_uid BIGINT UNSIGNED NOT NULL,
    item_revision BIGINT UNSIGNED NOT NULL,
    vnum INT NOT NULL,
    obj_blob LONGBLOB NOT NULL,
    claim_pid INT UNSIGNED NULL,
    claim_operation_id BINARY(16) NULL,
    claimed_at TIMESTAMP(6) NULL,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (auction_id,slot),
    UNIQUE KEY uq_auction_custody_item(item_uid),
    KEY idx_auction_custody_claim_operation(claim_operation_id),
    KEY idx_auction_custody_claim(claim_pid,claimed_at,auction_id),
    CONSTRAINT auction_custody_item_fk FOREIGN KEY (item_uid)
        REFERENCES item_current_owner(item_uid) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS auction_ledger (
    operation_id BINARY(16) NOT NULL,
    event_type TINYINT UNSIGNED NOT NULL,
    auction_id INT UNSIGNED NOT NULL,
    auction_revision BIGINT UNSIGNED NOT NULL,
    actor_pid INT UNSIGNED NOT NULL DEFAULT 0,
    counterparty_pid INT UNSIGNED NOT NULL DEFAULT 0,
    value_delta BIGINT NOT NULL DEFAULT 0,
    final_price BIGINT NOT NULL DEFAULT 0,
    item_count SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (operation_id),
    KEY idx_auction_ledger_revision(auction_id,auction_revision),
    KEY idx_auction_ledger_actor(actor_pid,created_at),
    CONSTRAINT auction_ledger_operation_fk FOREIGN KEY (operation_id)
        REFERENCES critical_operation_inbox(operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS auction_reconciliation_quarantine (
    quarantine_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    auction_id INT UNSIGNED NOT NULL,
    item_uid BIGINT UNSIGNED NOT NULL DEFAULT 0,
    conflict_code SMALLINT UNSIGNED NOT NULL,
    evidence VARCHAR(255) NOT NULL,
    detected_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    repaired_at TIMESTAMP(6) NULL,
    PRIMARY KEY (quarantine_id),
    UNIQUE KEY uq_auction_quarantine(auction_id,item_uid,conflict_code),
    KEY idx_auction_quarantine_open(repaired_at,auction_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

INSERT IGNORE INTO auction_reconciliation_quarantine(auction_id,item_uid,conflict_code,evidence)
SELECT a.id,0,1,'legacy auction has no stable per-item custody identity'
FROM auctions a
WHERE a.status=1 AND a.custody_state=0;

SET @old_claim_op_unique=(SELECT COUNT(*) FROM information_schema.statistics
 WHERE table_schema=DATABASE() AND table_name='auction_item_custody'
 AND index_name='uq_auction_custody_claim_operation');
SET @drop_old_claim_op_sql=IF(@old_claim_op_unique>0,
 'ALTER TABLE auction_item_custody DROP INDEX uq_auction_custody_claim_operation',
 'SELECT 1 INTO @old_claim_op_unchanged');
PREPARE drop_old_claim_op_stmt FROM @drop_old_claim_op_sql;
EXECUTE drop_old_claim_op_stmt;
DEALLOCATE PREPARE drop_old_claim_op_stmt;
SET @claim_op_index_missing=(SELECT COUNT(*)=0 FROM information_schema.statistics
 WHERE table_schema=DATABASE() AND table_name='auction_item_custody'
 AND index_name='idx_auction_custody_claim_operation');
SET @claim_op_index_sql=IF(@claim_op_index_missing,
 'ALTER TABLE auction_item_custody ADD KEY idx_auction_custody_claim_operation(claim_operation_id)',
 'SELECT 1 INTO @claim_op_index_unchanged');
PREPARE claim_op_index_stmt FROM @claim_op_index_sql;
EXECUTE claim_op_index_stmt;
DEALLOCATE PREPARE claim_op_index_stmt;

SET @old_ledger_revision_unique=(SELECT COUNT(*) FROM information_schema.statistics
 WHERE table_schema=DATABASE() AND table_name='auction_ledger'
 AND index_name='uq_auction_ledger_revision');
SET @drop_old_ledger_revision_sql=IF(@old_ledger_revision_unique>0,
 'ALTER TABLE auction_ledger DROP INDEX uq_auction_ledger_revision',
 'SELECT 1 INTO @old_ledger_revision_unchanged');
PREPARE drop_old_ledger_revision_stmt FROM @drop_old_ledger_revision_sql;
EXECUTE drop_old_ledger_revision_stmt;
DEALLOCATE PREPARE drop_old_ledger_revision_stmt;
SET @ledger_revision_index_missing=(SELECT COUNT(*)=0 FROM information_schema.statistics
 WHERE table_schema=DATABASE() AND table_name='auction_ledger'
 AND index_name='idx_auction_ledger_revision');
SET @ledger_revision_index_sql=IF(@ledger_revision_index_missing,
 'ALTER TABLE auction_ledger ADD KEY idx_auction_ledger_revision(auction_id,auction_revision)',
 'SELECT 1 INTO @ledger_revision_index_unchanged');
PREPARE ledger_revision_index_stmt FROM @ledger_revision_index_sql;
EXECUTE ledger_revision_index_stmt;
DEALLOCATE PREPARE ledger_revision_index_stmt;
