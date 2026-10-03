-- Archived player_affects uniqueness excludes bitvectors and custom messages.
-- Staging has genuine duplicate signatures with distinct bitvectors; the current
-- repository captures and saves each affect as a separate row. Never dedupe DML.
DROP PROCEDURE IF EXISTS duris_archive_affect_index;
DELIMITER //
CREATE PROCEDURE duris_archive_affect_index()
BEGIN
    DECLARE table_rows INT DEFAULT 0;
    DECLARE support_rows INT DEFAULT 0;
    DECLARE support_correct INT DEFAULT 0;
    DECLARE legacy_rows INT DEFAULT 0;
    DECLARE legacy_correct INT DEFAULT 0;

    SELECT COUNT(*) INTO table_rows FROM information_schema.tables
     WHERE table_schema=DATABASE() AND table_name='player_affects'
       AND table_type='BASE TABLE' AND engine='InnoDB';
    SELECT COUNT(*), COALESCE(SUM(seq_in_index=1 AND column_name='pid' AND
           non_unique=1 AND sub_part IS NULL AND collation='A' AND
           index_type='BTREE'),0)
      INTO support_rows, support_correct
      FROM information_schema.statistics
     WHERE table_schema=DATABASE() AND table_name='player_affects'
       AND index_name='idx_pid';
    SELECT COUNT(*), COALESCE(SUM(
           (seq_in_index=1 AND column_name='pid') OR
           (seq_in_index=2 AND column_name='type') OR
           (seq_in_index=3 AND column_name='duration') OR
           (seq_in_index=4 AND column_name='flags') OR
           (seq_in_index=5 AND column_name='modifier') OR
           (seq_in_index=6 AND column_name='location') OR
           (seq_in_index=7 AND column_name='level')),0)
      INTO legacy_rows, legacy_correct
      FROM information_schema.statistics
     WHERE table_schema=DATABASE() AND table_name='player_affects'
       AND index_name='uk_pid_type_dur_flags_mod_loc_lvl'
       AND non_unique=0 AND sub_part IS NULL AND collation='A'
       AND index_type='BTREE';
    -- The row-count check below also rejects a same-named index with different
    -- uniqueness, prefix, collation, or index type (not counted above).
    IF table_rows <> 1 OR support_rows <> 1 OR support_correct <> 1 OR
       (legacy_rows <> 0 AND (legacy_rows <> 7 OR legacy_correct <> 7)) OR
       (SELECT COUNT(*) FROM information_schema.statistics
         WHERE table_schema=DATABASE() AND table_name='player_affects'
           AND index_name='uk_pid_type_dur_flags_mod_loc_lvl') <> legacy_rows THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='unexpected player_affects table or index definitions';
    END IF;
    IF EXISTS (
        SELECT 1 FROM information_schema.key_column_usage
         WHERE referenced_table_schema=DATABASE()
           AND referenced_table_name='player_affects'
    ) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='player_affects index may support inbound foreign key';
    END IF;
    -- idx_pid remains as the independent supporting index for the outbound
    -- fk_player_affects(pid) -> player_data(pid). Dropping only the historical
    -- unique index cannot remove, merge, or rewrite an affect row.
    IF legacy_rows=7 THEN
        ALTER TABLE player_affects DROP INDEX uk_pid_type_dur_flags_mod_loc_lvl;
    END IF;
END //
DELIMITER ;
CALL duris_archive_affect_index();
DROP PROCEDURE duris_archive_affect_index;
