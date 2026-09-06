-- Immutable migration 0010: preserve committed coin amounts and item payloads.
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
