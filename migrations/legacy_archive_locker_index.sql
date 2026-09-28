-- Converge historical account_lockers(account_name,racewar) uniqueness to the
-- fresh-schema one-locker-per-account contract. Never merge or delete rows.
DROP PROCEDURE IF EXISTS duris_archive_locker_index;
DELIMITER //
CREATE PROCEDURE duris_archive_locker_index()
BEGIN
    DECLARE account_columns INT DEFAULT 0;
    DECLARE account_definition INT DEFAULT 0;
    DECLARE support_rows INT DEFAULT 0;
    DECLARE support_correct INT DEFAULT 0;
    DECLARE canonical_rows INT DEFAULT 0;
    DECLARE canonical_correct INT DEFAULT 0;
    DECLARE legacy_rows INT DEFAULT 0;
    DECLARE legacy_correct INT DEFAULT 0;

    SELECT COUNT(*), COALESCE(SUM(column_type='varchar(50)' AND
           is_nullable='NO' AND character_set_name='utf8mb4' AND
           collation_name='utf8mb4_unicode_ci'),0)
      INTO account_columns, account_definition
      FROM information_schema.columns
     WHERE table_schema=DATABASE() AND table_name='account_lockers'
       AND column_name='account_name';
    IF account_columns <> 1 OR account_definition <> 1 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='unexpected account_lockers account_name definition';
    END IF;
    SELECT COUNT(*), COALESCE(SUM(seq_in_index=1 AND
           column_name='account_name' AND non_unique=1 AND
           sub_part IS NULL AND collation='A' AND index_type='BTREE'),0)
      INTO support_rows, support_correct
      FROM information_schema.statistics
     WHERE table_schema=DATABASE() AND table_name='account_lockers'
       AND index_name='idx_account_name';
    SELECT COUNT(*), COALESCE(SUM(seq_in_index=1 AND
           column_name='account_name' AND non_unique=0 AND
           sub_part IS NULL AND collation='A' AND index_type='BTREE'),0)
      INTO canonical_rows, canonical_correct
      FROM information_schema.statistics
     WHERE table_schema=DATABASE() AND table_name='account_lockers'
       AND index_name='account_name';
    SELECT COUNT(*), COALESCE(SUM((seq_in_index=1 AND
           column_name='account_name' AND non_unique=0 AND
           sub_part IS NULL AND collation='A' AND index_type='BTREE') OR
           (seq_in_index=2 AND column_name='racewar' AND non_unique=0 AND
           sub_part IS NULL AND collation='A' AND index_type='BTREE')),0)
      INTO legacy_rows, legacy_correct
      FROM information_schema.statistics
     WHERE table_schema=DATABASE() AND table_name='account_lockers'
       AND index_name='uk_account_racewar';
    IF support_rows <> 1 OR support_correct <> 1 OR
       (canonical_rows <> 0 AND (canonical_rows <> 1 OR canonical_correct <> 1)) OR
       (legacy_rows <> 0 AND (legacy_rows <> 2 OR legacy_correct <> 2)) OR
       (canonical_rows=0 AND legacy_rows=0) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='unexpected account_lockers index definitions';
    END IF;
    IF EXISTS (
        SELECT 1 FROM information_schema.key_column_usage
         WHERE referenced_table_schema=DATABASE()
           AND referenced_table_name='account_lockers'
           AND referenced_column_name IN ('account_name','racewar')
    ) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='locker account index supports an inbound foreign key';
    END IF;
    IF EXISTS (
        SELECT 1 FROM account_lockers
         GROUP BY account_name HAVING COUNT(*) > 1
    ) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='duplicate account_lockers account_name; no changes made';
    END IF;
    -- The independent idx_account_name stays for the outbound accounts FK.
    -- Add first: duplicate races fail here, before the legacy key is removed.
    IF canonical_rows=0 THEN
        ALTER TABLE account_lockers ADD UNIQUE KEY account_name (account_name);
    END IF;
    SELECT COUNT(*), COALESCE(SUM(seq_in_index=1 AND
           column_name='account_name' AND non_unique=0 AND
           sub_part IS NULL AND collation='A' AND index_type='BTREE'),0)
      INTO canonical_rows, canonical_correct
      FROM information_schema.statistics
     WHERE table_schema=DATABASE() AND table_name='account_lockers'
       AND index_name='account_name';
    IF canonical_rows <> 1 OR canonical_correct <> 1 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='canonical locker unique key not established';
    END IF;
    IF legacy_rows=2 THEN
        ALTER TABLE account_lockers DROP INDEX uk_account_racewar;
    END IF;
END //
DELIMITER ;
CALL duris_archive_locker_index();
DROP PROCEDURE duris_archive_locker_index;
