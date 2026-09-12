-- Preserve generated pet identity/lifetime and explicitly retained legacy assets.
-- Existing rows remain NULL/0 so the loader can classify them without guessing.
SET @pet_state_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='player_pets' AND column_name='restore_state'),
    'SELECT 1', 'ALTER TABLE player_pets ADD COLUMN restore_state MEDIUMTEXT NULL');
PREPARE pet_state_stmt FROM @pet_state_sql;
EXECUTE pet_state_stmt;
DEALLOCATE PREPARE pet_state_stmt;

SET @pet_hold_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='player_pets' AND column_name='hold_reason'),
    'SELECT 1', 'ALTER TABLE player_pets ADD COLUMN hold_reason INT UNSIGNED NOT NULL DEFAULT 0');
PREPARE pet_hold_stmt FROM @pet_hold_sql;
EXECUTE pet_hold_stmt;
DEALLOCATE PREPARE pet_hold_stmt;
