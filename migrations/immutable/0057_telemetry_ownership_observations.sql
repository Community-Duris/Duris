-- Authenticated, observed account ownership boundaries. Allocation is not an observation.
-- Token zero is explicit unavailable identity for kind 9; other kinds retain NULL.
SET @telemetry_ownership_sql = IF(EXISTS(
  SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE()
  AND table_name='telemetry_interval' AND column_name='ownership_account_token'),
  'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ownership_account_token BIGINT UNSIGNED NULL DEFAULT NULL AFTER combat_quality_flags');
PREPARE telemetry_ownership_statement FROM @telemetry_ownership_sql;
EXECUTE telemetry_ownership_statement;
DEALLOCATE PREPARE telemetry_ownership_statement;

SET @telemetry_ownership_sql = IF(EXISTS(
  SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE()
  AND table_name='telemetry_interval' AND column_name='ownership_source'),
  'SELECT 1', 'ALTER TABLE telemetry_interval ADD COLUMN ownership_source TINYINT UNSIGNED NULL DEFAULT NULL AFTER ownership_account_token');
PREPARE telemetry_ownership_statement FROM @telemetry_ownership_sql;
EXECUTE telemetry_ownership_statement;
DEALLOCATE PREPARE telemetry_ownership_statement;

SET @telemetry_ownership_sql = IF(EXISTS(
  SELECT 1 FROM information_schema.table_constraints WHERE constraint_schema=DATABASE()
  AND table_name='telemetry_interval' AND constraint_name='chk_telemetry_ownership_payload'),
  'SELECT 1', 'ALTER TABLE telemetry_interval ADD CONSTRAINT chk_telemetry_ownership_payload CHECK ((record_kind=9 AND ownership_account_token IS NOT NULL AND ownership_source IS NOT NULL AND ((ownership_source BETWEEN 1 AND 4 AND ownership_account_token<>0) OR (ownership_source=5 AND ownership_account_token=0))) OR (record_kind<>9 AND ownership_account_token IS NULL AND ownership_source IS NULL))');
PREPARE telemetry_ownership_statement FROM @telemetry_ownership_sql;
EXECUTE telemetry_ownership_statement;
DEALLOCATE PREPARE telemetry_ownership_statement;
