-- Preserve existing craft receipts and extend their closed discipline set for
-- poison, Encrust success/failure and Harvester publication. These operations
-- award no XP; their player save still acknowledges the committed publication.
SET @alchemy_drop_check = IF(
    EXISTS(SELECT 1 FROM information_schema.table_constraints
           WHERE constraint_schema=DATABASE() AND table_name='player_craft_progression'
             AND constraint_name='ck_player_craft_progression_terms'),
    IF(VERSION() LIKE '%MariaDB%',
       'ALTER TABLE player_craft_progression DROP CONSTRAINT ck_player_craft_progression_terms',
       'ALTER TABLE player_craft_progression DROP CHECK ck_player_craft_progression_terms'),
    'SELECT 1');
PREPARE alchemy_statement FROM @alchemy_drop_check;
EXECUTE alchemy_statement;
DEALLOCATE PREPARE alchemy_statement;
ALTER TABLE player_craft_progression
    ADD CONSTRAINT ck_player_craft_progression_terms
    CHECK (pid > 0 AND discipline IN (1,2,3,4,5,6) AND experience <= 2147483647
           AND (discipline IN (1,2) OR experience=0));
