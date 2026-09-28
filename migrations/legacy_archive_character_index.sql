-- The archived UNIQUE(account_name,char_name) adds no identity protection
-- beyond the canonical UNIQUE(char_name); drop the redundant key, never rows.
DROP PROCEDURE IF EXISTS duris_archive_character_index;
DELIMITER //
CREATE PROCEDURE duris_archive_character_index()
BEGIN
    DECLARE table_rows INT DEFAULT 0;
    DECLARE canonical_rows INT DEFAULT 0;
    DECLARE canonical_correct INT DEFAULT 0;
    DECLARE legacy_rows INT DEFAULT 0;
    DECLARE legacy_correct INT DEFAULT 0;

    SELECT COUNT(*) INTO table_rows FROM information_schema.tables
     WHERE table_schema=DATABASE() AND table_name='account_characters'
       AND table_type='BASE TABLE' AND engine='InnoDB';
    SELECT COUNT(*), COALESCE(SUM(seq_in_index=1 AND
           column_name='char_name' AND non_unique=0 AND sub_part IS NULL AND
           collation='A' AND index_type='BTREE'),0)
      INTO canonical_rows, canonical_correct
      FROM information_schema.statistics
     WHERE table_schema=DATABASE() AND table_name='account_characters'
       AND index_name='idx_char_name_unique';
    SELECT COUNT(*), COALESCE(SUM(
           (seq_in_index=1 AND column_name='account_name') OR
           (seq_in_index=2 AND column_name='char_name')),0)
      INTO legacy_rows, legacy_correct
      FROM information_schema.statistics
     WHERE table_schema=DATABASE() AND table_name='account_characters'
       AND index_name='acct_char' AND non_unique=0 AND sub_part IS NULL
       AND collation='A' AND index_type='BTREE';
    IF table_rows <> 1 OR canonical_rows <> 1 OR canonical_correct <> 1 OR
       (legacy_rows <> 0 AND (legacy_rows <> 2 OR legacy_correct <> 2)) OR
       (SELECT COUNT(*) FROM information_schema.statistics
         WHERE table_schema=DATABASE() AND table_name='account_characters'
           AND index_name='acct_char') <> legacy_rows THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='unexpected account_characters index definitions';
    END IF;
    IF EXISTS (
        SELECT 1 FROM information_schema.key_column_usage
         WHERE referenced_table_schema=DATABASE()
           AND referenced_table_name='account_characters'
    ) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='account_characters index may support inbound foreign key';
    END IF;
    IF legacy_rows=2 THEN
        ALTER TABLE account_characters DROP INDEX acct_char;
    END IF;
END //
DELIMITER ;
CALL duris_archive_character_index();
DROP PROCEDURE duris_archive_character_index;
