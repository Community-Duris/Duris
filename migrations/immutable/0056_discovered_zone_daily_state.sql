-- Retain v1 documents while allowing discovery and frozen daily receipts in v2.
-- No player facts are rewritten by this migration; conversion occurs after a
-- validated application load. Replaying the migration preserves the document.
SET @zsq_drop_check = IF(VERSION() LIKE '%MariaDB%',
    'ALTER TABLE zone_story_quest_state DROP CONSTRAINT chk_zone_story_quest_state_version',
    'ALTER TABLE zone_story_quest_state DROP CHECK chk_zone_story_quest_state_version');
SET @zsq_check_exists = (
    SELECT COUNT(*) FROM information_schema.table_constraints
    WHERE constraint_schema=DATABASE() AND table_name='zone_story_quest_state'
      AND constraint_name='chk_zone_story_quest_state_version' AND constraint_type='CHECK'
);
SET @zsq_drop_check = IF(@zsq_check_exists > 0, @zsq_drop_check, 'SELECT 1');
PREPARE zsq_statement FROM @zsq_drop_check;
EXECUTE zsq_statement;
DEALLOCATE PREPARE zsq_statement;
ALTER TABLE zone_story_quest_state
    ADD CONSTRAINT chk_zone_story_quest_state_version CHECK (state_version IN (1,2));

-- Reuse the existing MEDIUMTEXT table as 255 bounded record buckets. Legacy
-- aggregate documents remain legal only in row 1 until application conversion.
SET @zsq_drop_check = IF(VERSION() LIKE '%MariaDB%',
    'ALTER TABLE zone_story_quest_state DROP CONSTRAINT chk_zone_story_quest_state_singleton',
    'ALTER TABLE zone_story_quest_state DROP CHECK chk_zone_story_quest_state_singleton');
SET @zsq_check_exists = (
    SELECT COUNT(*) FROM information_schema.table_constraints
    WHERE constraint_schema=DATABASE() AND table_name='zone_story_quest_state'
      AND constraint_name='chk_zone_story_quest_state_singleton' AND constraint_type='CHECK'
);
SET @zsq_drop_check = IF(@zsq_check_exists > 0, @zsq_drop_check, 'SELECT 1');
PREPARE zsq_statement FROM @zsq_drop_check;
EXECUTE zsq_statement;
DEALLOCATE PREPARE zsq_statement;
ALTER TABLE zone_story_quest_state ADD CONSTRAINT chk_zone_story_quest_state_singleton
    CHECK (state_id >= 1 AND (state_version = 2 OR state_id = 1));
