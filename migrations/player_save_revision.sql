-- Add the durable monotonic identity used by the revisioned player checkpoint pipeline.
-- Existing and newly created rows start at revision zero until a guarded worker apply.
SET @save_revision_exists = (
    SELECT COUNT(*)
    FROM information_schema.columns
    WHERE table_schema = DATABASE()
      AND table_name = 'player_data'
      AND column_name = 'save_revision'
);
SET @save_revision_sql = IF(
    @save_revision_exists = 0,
    'ALTER TABLE player_data ADD COLUMN save_revision BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER last_save',
    'SELECT 1'
);
PREPARE save_revision_stmt FROM @save_revision_sql;
EXECUTE save_revision_stmt;
DEALLOCATE PREPARE save_revision_stmt;
