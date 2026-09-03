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
-- The guard reads the current default rather than the table name, so a database
-- that already carries the contract issues no ALTER at all and the step stays
-- exactly re-runnable. MySQL reports the converged default as CURRENT_TIMESTAMP
-- and MariaDB as current_timestamp(); the prefix match accepts both.

DELIMITER //
CREATE PROCEDURE migrate_0007_pkill_event_stamp()
BEGIN
    DECLARE legacy_default INT DEFAULT 0;

    SELECT COUNT(*) INTO legacy_default
    FROM information_schema.columns
    WHERE table_schema = DATABASE()
      AND table_name = 'pkill_event'
      AND column_name = 'stamp'
      AND (column_default IS NULL
           OR UPPER(column_default) NOT LIKE 'CURRENT_TIMESTAMP%');

    IF legacy_default = 1 THEN
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
