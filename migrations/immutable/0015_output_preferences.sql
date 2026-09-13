-- Per-character display choices share the revisioned player status checkpoint.
-- Empty values preserve the appearance of every existing character.
SET @output_preferences_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='player_data' AND column_name='output_preferences'),
    'SELECT 1', 'ALTER TABLE player_data ADD COLUMN output_preferences VARBINARY(512) NOT NULL DEFAULT ''''');
PREPARE output_preferences_stmt FROM @output_preferences_sql;
EXECUTE output_preferences_stmt;
DEALLOCATE PREPARE output_preferences_stmt;
