-- Immutable migration 0002: guard against duplicate item metadata accumulating on the incremental
-- and equipment-only save paths (see docs/ongoing-projects/ongoing/character-creation-persistence-gap.md).
--
-- The load path treats an exact duplicate (item_id, keyword, description) as snapshot
-- corruption and refuses the character, so the write side must never be able to produce
-- one. This migration is additive and re-runnable: it deduplicates first, keeping the
-- lowest id of each group, then adds the unique key only if it is not already present.
--
-- description is a TEXT column, so the unique key uses a 255-byte prefix. Two rows whose
-- descriptions differ only past 255 bytes would collide; that is intentional - such rows
-- are duplicates for every practical purpose and the load path would reject neither.

DELETE ed FROM player_item_extra_descr ed
JOIN (
    SELECT MIN(id) AS keep_id, item_id, keyword, LEFT(COALESCE(description, ''), 255) AS descr_prefix
    FROM player_item_extra_descr
    GROUP BY item_id, keyword, descr_prefix
    HAVING COUNT(*) > 1
) dup
  ON dup.item_id = ed.item_id
 AND dup.keyword = ed.keyword
 AND LEFT(COALESCE(ed.description, ''), 255) = dup.descr_prefix
 AND ed.id > dup.keep_id;

SET @uk_item_descr_exists = (
    SELECT COUNT(*)
    FROM information_schema.statistics
    WHERE table_schema = DATABASE()
      AND table_name = 'player_item_extra_descr'
      AND index_name = 'uk_item_descr'
);
SET @uk_item_descr_sql = IF(
    @uk_item_descr_exists = 0,
    'ALTER TABLE player_item_extra_descr ADD UNIQUE KEY uk_item_descr (item_id, keyword, description(255))',
    'SELECT 1'
);
PREPARE uk_item_descr_stmt FROM @uk_item_descr_sql;
EXECUTE uk_item_descr_stmt;
DEALLOCATE PREPARE uk_item_descr_stmt;

DELETE ed FROM player_pet_item_extra_descr ed
JOIN (
    SELECT MIN(id) AS keep_id, item_id, keyword, LEFT(COALESCE(description, ''), 255) AS descr_prefix
    FROM player_pet_item_extra_descr
    GROUP BY item_id, keyword, descr_prefix
    HAVING COUNT(*) > 1
) dup
  ON dup.item_id = ed.item_id
 AND dup.keyword = ed.keyword
 AND LEFT(COALESCE(ed.description, ''), 255) = dup.descr_prefix
 AND ed.id > dup.keep_id;

SET @uk_pet_item_descr_exists = (
    SELECT COUNT(*)
    FROM information_schema.statistics
    WHERE table_schema = DATABASE()
      AND table_name = 'player_pet_item_extra_descr'
      AND index_name = 'uk_pet_item_descr'
);
SET @uk_pet_item_descr_sql = IF(
    @uk_pet_item_descr_exists = 0,
    'ALTER TABLE player_pet_item_extra_descr ADD UNIQUE KEY uk_pet_item_descr (item_id, keyword, description(255))',
    'SELECT 1'
);
PREPARE uk_pet_item_descr_stmt FROM @uk_pet_item_descr_sql;
EXECUTE uk_pet_item_descr_stmt;
DEALLOCATE PREPARE uk_pet_item_descr_stmt;

-- The same accumulation applies to item affects; the load path silently drops duplicate
-- (location, modifier) pairs there, but the rows still grow without bound.
DELETE ia FROM player_item_affects ia
JOIN (
    SELECT MIN(id) AS keep_id, item_id, location, modifier
    FROM player_item_affects
    GROUP BY item_id, location, modifier
    HAVING COUNT(*) > 1
) dup
  ON dup.item_id = ia.item_id
 AND dup.location = ia.location
 AND dup.modifier = ia.modifier
 AND ia.id > dup.keep_id;

SET @uk_item_affect_exists = (
    SELECT COUNT(*)
    FROM information_schema.statistics
    WHERE table_schema = DATABASE()
      AND table_name = 'player_item_affects'
      AND index_name = 'uk_item_affect'
);
SET @uk_item_affect_sql = IF(
    @uk_item_affect_exists = 0,
    'ALTER TABLE player_item_affects ADD UNIQUE KEY uk_item_affect (item_id, location, modifier)',
    'SELECT 1'
);
PREPARE uk_item_affect_stmt FROM @uk_item_affect_sql;
EXECUTE uk_item_affect_stmt;
DEALLOCATE PREPARE uk_item_affect_stmt;

DELETE ia FROM player_pet_item_affects ia
JOIN (
    SELECT MIN(id) AS keep_id, item_id, location, modifier
    FROM player_pet_item_affects
    GROUP BY item_id, location, modifier
    HAVING COUNT(*) > 1
) dup
  ON dup.item_id = ia.item_id
 AND dup.location = ia.location
 AND dup.modifier = ia.modifier
 AND ia.id > dup.keep_id;

SET @uk_pet_item_affect_exists = (
    SELECT COUNT(*)
    FROM information_schema.statistics
    WHERE table_schema = DATABASE()
      AND table_name = 'player_pet_item_affects'
      AND index_name = 'uk_pet_item_affect'
);
SET @uk_pet_item_affect_sql = IF(
    @uk_pet_item_affect_exists = 0,
    'ALTER TABLE player_pet_item_affects ADD UNIQUE KEY uk_pet_item_affect (item_id, location, modifier)',
    'SELECT 1'
);
PREPARE uk_pet_item_affect_stmt FROM @uk_pet_item_affect_sql;
EXECUTE uk_pet_item_affect_stmt;
DEALLOCATE PREPARE uk_pet_item_affect_stmt;
