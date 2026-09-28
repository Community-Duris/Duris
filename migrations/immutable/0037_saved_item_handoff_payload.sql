-- Bind the acknowledged handoff to both complete saved-item snapshots.
-- Existing receipts predate this evidence. NULL keeps those receipts visible
-- so recovery can defer them instead of trusting an unverifiable payload.
SET @source_payload_exists = (
    SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema=DATABASE()
      AND table_name='saved_item_recovery_handoff'
      AND column_name='source_payload_digest'
);
SET @source_payload_sql = IF(@source_payload_exists=0,
    'ALTER TABLE saved_item_recovery_handoff ADD COLUMN source_payload_digest BINARY(32) NULL',
    'SELECT 1');
PREPARE source_payload_stmt FROM @source_payload_sql;
EXECUTE source_payload_stmt;
DEALLOCATE PREPARE source_payload_stmt;

SET @destination_payload_exists = (
    SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema=DATABASE()
      AND table_name='saved_item_recovery_handoff'
      AND column_name='destination_payload_digest'
);
SET @destination_payload_sql = IF(@destination_payload_exists=0,
    'ALTER TABLE saved_item_recovery_handoff ADD COLUMN destination_payload_digest BINARY(32) NULL',
    'SELECT 1');
PREPARE destination_payload_stmt FROM @destination_payload_sql;
EXECUTE destination_payload_stmt;
DEALLOCATE PREPARE destination_payload_stmt;
