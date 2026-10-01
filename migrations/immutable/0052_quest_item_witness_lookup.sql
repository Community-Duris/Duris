-- Recovery verifies immutable creation witnesses by frozen quest source identity.
-- Keep this non-unique: conflicting witnesses must remain visible to recovery.
SET @quest_item_witness_index_exists = (
    SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'item_ownership_ledger'
      AND index_name = 'idx_item_ledger_reason_source'
);
SET @quest_item_witness_index_sql = IF(
    @quest_item_witness_index_exists = 0,
    'ALTER TABLE item_ownership_ledger ADD KEY idx_item_ledger_reason_source (reason_type, reason_id)',
    'SELECT 1 INTO @migration_noop'
);
PREPARE quest_item_witness_index_stmt FROM @quest_item_witness_index_sql;
EXECUTE quest_item_witness_index_stmt;
DEALLOCATE PREPARE quest_item_witness_index_stmt;
