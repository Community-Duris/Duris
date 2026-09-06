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
