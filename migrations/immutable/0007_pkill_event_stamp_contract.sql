-- Immutable migration 0007: replace the legacy zero-date default on
-- pkill_event.stamp with an explicit portable contract. The column was created
-- as DATETIME NOT NULL DEFAULT '0000-00-00 00:00:00', which no longer loads
-- under a server that enables NO_ZERO_DATE and NO_ZERO_IN_DATE, and which
-- forced consumers to relax their session SQL mode to write the table at all.
--
-- Every current writer supplies the value explicitly with NOW(), so the default
-- is only a safety net; CURRENT_TIMESTAMP expresses that on both supported
-- engines. Rows written before strict mode was enabled may still hold the zero
-- value, and an ALTER that leaves them in place fails under NO_ZERO_DATE, so
-- they are first normalized to the epoch. That value carries the same absence
-- of information the zero date did, in a form both engines accept.
--
-- The guard reads the whole contract the verification step asserts - DATETIME,
-- NOT NULL, a current-timestamp default, and no surviving zero-date rows - so a
-- database that already carries it issues no ALTER and the step stays exactly
-- re-runnable, while a partially converged one (default already replaced but
-- rows or nullability still legacy) is still repaired instead of silently
-- skipped. MySQL reports the converged default as CURRENT_TIMESTAMP and MariaDB
-- as current_timestamp(); the prefix match accepts both.
--
-- The procedure is dropped before it is created so a run that failed inside the
-- body, leaving the definition behind, can simply be retried.

DELIMITER //
DROP PROCEDURE IF EXISTS migrate_0007_pkill_event_stamp//
CREATE PROCEDURE migrate_0007_pkill_event_stamp()
BEGIN
    DECLARE contract_met INT DEFAULT 0;
    DECLARE zero_rows INT DEFAULT 0;

    SELECT COUNT(*) INTO contract_met
    FROM information_schema.columns
    WHERE table_schema = DATABASE()
      AND table_name = 'pkill_event'
      AND column_name = 'stamp'
      AND data_type = 'datetime'
      AND is_nullable = 'NO'
      AND column_default IS NOT NULL
      AND UPPER(column_default) LIKE 'CURRENT_TIMESTAMP%';

    SELECT COUNT(*) INTO zero_rows
    FROM pkill_event
    WHERE YEAR(stamp) = 0;

    IF contract_met = 0 OR zero_rows > 0 THEN
        UPDATE pkill_event
           SET stamp = '1970-01-01 00:00:00'
         WHERE YEAR(stamp) = 0;

        ALTER TABLE pkill_event
            MODIFY stamp DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP;
    END IF;
END//
CALL migrate_0007_pkill_event_stamp()//
DROP PROCEDURE migrate_0007_pkill_event_stamp//
DELIMITER ;
