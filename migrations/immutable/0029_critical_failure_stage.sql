-- Record the bounded terminal-failure stage on critical operation receipts.
-- This is additive and safe to replay. A legacy upgrade or an earlier
-- compatibility patch may already have supplied the column; in that case the
-- immutable step records the already-present shape without changing rows.
SET @critical_failure_stage_sql = IF(EXISTS(
    SELECT 1 FROM information_schema.columns
    WHERE table_schema=DATABASE()
      AND table_name='critical_operation_inbox'
      AND column_name='failure_stage'),
    'SELECT 1',
    'ALTER TABLE critical_operation_inbox ADD COLUMN failure_stage SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER result_code');
PREPARE critical_failure_stage_stmt FROM @critical_failure_stage_sql;
EXECUTE critical_failure_stage_stmt;
DEALLOCATE PREPARE critical_failure_stage_stmt;
