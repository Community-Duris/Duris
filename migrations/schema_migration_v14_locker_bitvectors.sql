-- add bitvector columns to locker_items for encrusted item affects
-- also fixes existing encrusted items that lost their bitvectors during migration

-- step 1: add bitvector columns to locker_items
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "bitvector1 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "bitvector2 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "bitvector3 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "bitvector4 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "bitvector5 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- same for account_locker_items
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_locker_items' AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_locker_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "bitvector1 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_locker_items' AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_locker_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "bitvector2 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_locker_items' AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_locker_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "bitvector3 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_locker_items' AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_locker_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "bitvector4 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_locker_items' AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_locker_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "bitvector5 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- step 2: fix existing encrusted items
-- ITEM_ENCRUSTED = BIT_32 = 2147483648
-- value5 stores the spell type, which determines what bitvector to set
--
-- spell to bitvector mapping (from set_encrust_affect in randomeq.c):
-- SPELL_ACID_BLAST (116)               -> bitvector2 = AFF2_PROT_GAS | AFF2_PROT_ACID = 4096 | 8192 = 12288
-- SPELL_FIREBALL (26)                  -> bitvector1 = AFF_PROT_FIRE = 536870912
-- SPELL_MAGIC_MISSILE (32)             -> bitvector1 = AFF_PROTECT_GOOD = 65536
-- SPELL_CHILL_TOUCH (8)                -> bitvector2 = AFF2_PROT_COLD = 64
-- SPELL_NEGATIVE_CONCUSSION_BLAST (378)-> bitvector1 = AFF_MINOR_GLOBE = 64
-- SPELL_SHOCKING_GRASP (37)            -> bitvector1 = AFF_SLOW_POISON = 32768
-- SPELL_FLAMESTRIKE (21)               -> bitvector1 = AFF_SENSE_LIFE = 32
-- SPELL_BLINDNESS (4)                  -> bitvector2 = AFF2_DETECT_MAGIC = 16
-- SPELL_ENERGY_DRAIN (25)              -> bitvector1 = AFF_HASTE = 16

-- fix locker_items encrusted items
UPDATE locker_items
SET bitvector2 = COALESCE(bitvector2, 0) | 12288
WHERE (extra_flags & 2147483648) != 0 AND value5 = 116;

UPDATE locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 536870912
WHERE (extra_flags & 2147483648) != 0 AND value5 = 26;

UPDATE locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 65536
WHERE (extra_flags & 2147483648) != 0 AND value5 = 32;

UPDATE locker_items
SET bitvector2 = COALESCE(bitvector2, 0) | 64
WHERE (extra_flags & 2147483648) != 0 AND value5 = 8;

UPDATE locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 64
WHERE (extra_flags & 2147483648) != 0 AND value5 = 378;

UPDATE locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 32768
WHERE (extra_flags & 2147483648) != 0 AND value5 = 37;

UPDATE locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 32
WHERE (extra_flags & 2147483648) != 0 AND value5 = 21;

UPDATE locker_items
SET bitvector2 = COALESCE(bitvector2, 0) | 16
WHERE (extra_flags & 2147483648) != 0 AND value5 = 4;

UPDATE locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 16
WHERE (extra_flags & 2147483648) != 0 AND value5 = 25;

-- fix account_locker_items encrusted items
UPDATE account_locker_items
SET bitvector2 = COALESCE(bitvector2, 0) | 12288
WHERE (extra_flags & 2147483648) != 0 AND value5 = 116;

UPDATE account_locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 536870912
WHERE (extra_flags & 2147483648) != 0 AND value5 = 26;

UPDATE account_locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 65536
WHERE (extra_flags & 2147483648) != 0 AND value5 = 32;

UPDATE account_locker_items
SET bitvector2 = COALESCE(bitvector2, 0) | 64
WHERE (extra_flags & 2147483648) != 0 AND value5 = 8;

UPDATE account_locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 64
WHERE (extra_flags & 2147483648) != 0 AND value5 = 378;

UPDATE account_locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 32768
WHERE (extra_flags & 2147483648) != 0 AND value5 = 37;

UPDATE account_locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 32
WHERE (extra_flags & 2147483648) != 0 AND value5 = 21;

UPDATE account_locker_items
SET bitvector2 = COALESCE(bitvector2, 0) | 16
WHERE (extra_flags & 2147483648) != 0 AND value5 = 4;

UPDATE account_locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 16
WHERE (extra_flags & 2147483648) != 0 AND value5 = 25;

-- fix weightless containers (vnums 32841, 19780, 96443 should have weight -3000)
UPDATE player_items SET weight = -3000 WHERE vnum IN (32841, 19780, 96443) AND (weight IS NULL OR weight >= 0);
UPDATE locker_items SET weight = -3000 WHERE vnum IN (32841, 19780, 96443) AND (weight IS NULL OR weight >= 0);
UPDATE account_locker_items SET weight = -3000 WHERE vnum IN (32841, 19780, 96443) AND (weight IS NULL OR weight >= 0);
UPDATE corpse_items SET weight = -3000 WHERE vnum IN (32841, 19780, 96443) AND (weight IS NULL OR weight >= 0);

-- fix player_items encrusted items (in case any were missed)
UPDATE player_items
SET bitvector2 = COALESCE(bitvector2, 0) | 12288
WHERE (extra_flags & 2147483648) != 0 AND value5 = 116 AND (bitvector2 IS NULL OR (bitvector2 & 12288) = 0);

UPDATE player_items
SET bitvector1 = COALESCE(bitvector1, 0) | 536870912
WHERE (extra_flags & 2147483648) != 0 AND value5 = 26 AND (bitvector1 IS NULL OR (bitvector1 & 536870912) = 0);

UPDATE player_items
SET bitvector1 = COALESCE(bitvector1, 0) | 65536
WHERE (extra_flags & 2147483648) != 0 AND value5 = 32 AND (bitvector1 IS NULL OR (bitvector1 & 65536) = 0);

UPDATE player_items
SET bitvector2 = COALESCE(bitvector2, 0) | 64
WHERE (extra_flags & 2147483648) != 0 AND value5 = 8 AND (bitvector2 IS NULL OR (bitvector2 & 64) = 0);

UPDATE player_items
SET bitvector1 = COALESCE(bitvector1, 0) | 64
WHERE (extra_flags & 2147483648) != 0 AND value5 = 378 AND (bitvector1 IS NULL OR (bitvector1 & 64) = 0);

UPDATE player_items
SET bitvector1 = COALESCE(bitvector1, 0) | 32768
WHERE (extra_flags & 2147483648) != 0 AND value5 = 37 AND (bitvector1 IS NULL OR (bitvector1 & 32768) = 0);

UPDATE player_items
SET bitvector1 = COALESCE(bitvector1, 0) | 32
WHERE (extra_flags & 2147483648) != 0 AND value5 = 21 AND (bitvector1 IS NULL OR (bitvector1 & 32) = 0);

UPDATE player_items
SET bitvector2 = COALESCE(bitvector2, 0) | 16
WHERE (extra_flags & 2147483648) != 0 AND value5 = 4 AND (bitvector2 IS NULL OR (bitvector2 & 16) = 0);

UPDATE player_items
SET bitvector1 = COALESCE(bitvector1, 0) | 16
WHERE (extra_flags & 2147483648) != 0 AND value5 = 25 AND (bitvector1 IS NULL OR (bitvector1 & 16) = 0);

-- show how many items were affected
SELECT 'weightless containers fixed (player_items):' as info, COUNT(*) as count
FROM player_items WHERE vnum IN (32841, 19780, 96443) AND weight = -3000;

SELECT 'player_items encrusted count:' as info, COUNT(*) as count
FROM player_items WHERE (extra_flags & 2147483648) != 0;

SELECT 'locker_items encrusted count:' as info, COUNT(*) as count
FROM locker_items WHERE (extra_flags & 2147483648) != 0;

SELECT 'account_locker_items encrusted count:' as info, COUNT(*) as count
FROM account_locker_items WHERE (extra_flags & 2147483648) != 0;
