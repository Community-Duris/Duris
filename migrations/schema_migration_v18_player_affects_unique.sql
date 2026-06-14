-- schema_migration_v18_player_affects_unique.sql
-- Phase 3.4 follow-up: add UNIQUE KEY to player_affects so it can be
-- converted to REPLACE INTO like the other 7 array-save tables.
--
-- IMPORTANT: This migration MUST run before the production code is updated
-- to use REPLACE INTO. The order of operations is:
--   1. Deduplicate existing rows (keep the row with the lowest id per group)
--   2. Add the UNIQUE KEY (will fail if duplicates still exist)
--   3. Source code updated to use REPLACE INTO
--
-- Idempotency: the dedup DELETE is safe to run multiple times (no-op if no
-- duplicates). The ALTER TABLE will fail on second run because the key
-- already exists, but the `|| true` in cycle_mud.sh swallows that error.
--
-- The unique key includes all columns that can distinguish semantically
-- different affects: (pid, type, duration, flags, modifier, location, level).
-- bitvector1-5 are excluded because they are derived from the spell type
-- and are the same for the same spell. Two affects of the same type on the
-- same character with the same duration, flags, modifier, location, and
-- level are semantically identical and can be safely collapsed.

-- Step 1: Deduplicate existing rows. Keep the row with the lowest id for
-- each (pid, type, duration, flags, modifier, location, level) group. This
-- preserves the oldest row (which was inserted first and is most likely the
-- canonical one).
DELETE a FROM player_affects a
INNER JOIN player_affects b
  ON a.pid = b.pid
 AND a.type = b.type
 AND a.duration = b.duration
 AND a.flags = b.flags
 AND a.modifier = b.modifier
 AND a.location = b.location
 AND a.level = b.level
 AND a.id > b.id;

-- Step 2: Add the UNIQUE KEY. Uses the idempotent prepared-statement
-- pattern so the migration is safe to re-run.
SET @key_exists := (
    SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE()
      AND table_name = 'player_affects'
      AND index_name = 'uk_pid_type_dur_flags_mod_loc_lvl'
);

SET @sql := IF(@key_exists = 0,
    'ALTER TABLE player_affects ADD UNIQUE KEY uk_pid_type_dur_flags_mod_loc_lvl (pid, type, duration, flags, modifier, location, level)',
    'DO 0'  -- no-op
);

PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
