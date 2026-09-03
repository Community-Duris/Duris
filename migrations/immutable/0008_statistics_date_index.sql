-- Immutable migration 0008: index statistics(date). The table is the population
-- time series and carried only its primary key, while every consumer filters an
-- epoch range on `date` and sorts chronologically; a snapshot of 256,292 rows
-- planned as a full scan plus filesort.
--
-- The index is additive and the table is append-only at roughly one row per
-- sampled hour, so the write cost is immaterial next to the read pattern.
--
-- MySQL 8 has no portable CREATE INDEX IF NOT EXISTS, so the guard reads
-- information_schema first. It matches the whole shape the verification step
-- asserts - a single non-unique entry on `date` at seq_in_index 1 - so a
-- database that already carries the index issues no ALTER and the step stays
-- exactly re-runnable, while an index of that name with a different shape is
-- dropped and rebuilt instead of leaving the contract unmet.
--
-- The procedure is dropped before it is created so a run that failed inside the
-- body, leaving the definition behind, can simply be retried.

DELIMITER //
DROP PROCEDURE IF EXISTS migrate_0008_statistics_date_index//
CREATE PROCEDURE migrate_0008_statistics_date_index()
BEGIN
    DECLARE index_entries INT DEFAULT 0;
    DECLARE contract_entries INT DEFAULT 0;

    SELECT COUNT(*), COALESCE(SUM(non_unique = 1 AND seq_in_index = 1 AND
                                  column_name = 'date'), 0)
      INTO index_entries, contract_entries
    FROM information_schema.statistics
    WHERE table_schema = DATABASE()
      AND table_name = 'statistics'
      AND index_name = 'idx_statistics_date';

    IF index_entries <> 1 OR contract_entries <> 1 THEN
        IF index_entries > 0 THEN
            ALTER TABLE statistics DROP KEY idx_statistics_date;
        END IF;
        ALTER TABLE statistics ADD KEY idx_statistics_date (`date`);
    END IF;
END//
CALL migrate_0008_statistics_date_index()//
DROP PROCEDURE migrate_0008_statistics_date_index//
DELIMITER ;
