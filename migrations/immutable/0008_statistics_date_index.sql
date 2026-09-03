-- Immutable migration 0008: index statistics(date). The table is the population
-- time series and carried only its primary key, while every consumer filters an
-- epoch range on `date` and sorts chronologically; a snapshot of 256,292 rows
-- planned as a full scan plus filesort.
--
-- The index is additive and the table is append-only at roughly one row per
-- sampled hour, so the write cost is immaterial next to the read pattern.
--
-- MySQL 8 has no portable CREATE INDEX IF NOT EXISTS, so the guard reads
-- information_schema first and issues no ALTER when the index is already
-- present, which keeps the step exactly re-runnable.

DELIMITER //
CREATE PROCEDURE migrate_0008_statistics_date_index()
BEGIN
    DECLARE index_present INT DEFAULT 0;

    SELECT COUNT(*) INTO index_present
    FROM information_schema.statistics
    WHERE table_schema = DATABASE()
      AND table_name = 'statistics'
      AND index_name = 'idx_statistics_date';

    IF index_present = 0 THEN
        ALTER TABLE statistics ADD KEY idx_statistics_date (`date`);
    END IF;
END//
CALL migrate_0008_statistics_date_index()//
DROP PROCEDURE migrate_0008_statistics_date_index//
DELIMITER ;
