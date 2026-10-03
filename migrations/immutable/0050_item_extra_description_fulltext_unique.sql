-- Replace the legacy 255-character description-prefix uniqueness with uniqueness
-- over the complete description value. SHA-256 keeps the indexed key within the
-- InnoDB limit while distinguishing descriptions that share the first 255 chars.
-- The generated digest is deterministic in both MySQL 8 and MariaDB 10.11.
-- Duplicate detection uses the same exact-byte digest as the final index.
-- Description collation must not reject distinct case/accent variants on replay.
-- Do not silently delete pre-existing rows. A duplicate complete value requires
-- explicit reconciliation; abort before any permanent DDL if one exists.
CREATE TEMPORARY TABLE item_descr_duplicate_guard (id INT NOT NULL PRIMARY KEY);
INSERT INTO item_descr_duplicate_guard VALUES (1);
INSERT INTO item_descr_duplicate_guard
SELECT 1 FROM (
    SELECT item_id, keyword, UNHEX(SHA2(COALESCE(description, ''), 256))
    FROM player_item_extra_descr
    GROUP BY item_id, keyword, UNHEX(SHA2(COALESCE(description, ''), 256))
    HAVING COUNT(*) > 1
    LIMIT 1
) AS player_conflicts;
INSERT INTO item_descr_duplicate_guard
SELECT 1 FROM (
    SELECT item_id, keyword, UNHEX(SHA2(COALESCE(description, ''), 256))
    FROM player_pet_item_extra_descr
    GROUP BY item_id, keyword, UNHEX(SHA2(COALESCE(description, ''), 256))
    HAVING COUNT(*) > 1
    LIMIT 1
) AS pet_conflicts;
DROP TEMPORARY TABLE item_descr_duplicate_guard;

SET @player_descr_hash_exists = (
    SELECT COUNT(*)
    FROM information_schema.columns
    WHERE table_schema = DATABASE()
      AND table_name = 'player_item_extra_descr'
      AND column_name = 'description_sha256'
);
SET @player_descr_hash_sql = IF(
    @player_descr_hash_exists = 0,
    'ALTER TABLE player_item_extra_descr ADD COLUMN description_sha256 BINARY(32) GENERATED ALWAYS AS (UNHEX(SHA2(COALESCE(description, ''''), 256))) STORED AFTER description',
    'SELECT 1 INTO @migration_noop'
);
PREPARE player_descr_hash_stmt FROM @player_descr_hash_sql;
EXECUTE player_descr_hash_stmt;
DEALLOCATE PREPARE player_descr_hash_stmt;

SET @pet_descr_hash_exists = (
    SELECT COUNT(*)
    FROM information_schema.columns
    WHERE table_schema = DATABASE()
      AND table_name = 'player_pet_item_extra_descr'
      AND column_name = 'description_sha256'
);
SET @pet_descr_hash_sql = IF(
    @pet_descr_hash_exists = 0,
    'ALTER TABLE player_pet_item_extra_descr ADD COLUMN description_sha256 BINARY(32) GENERATED ALWAYS AS (UNHEX(SHA2(COALESCE(description, ''''), 256))) STORED AFTER description',
    'SELECT 1 INTO @migration_noop'
);
PREPARE pet_descr_hash_stmt FROM @pet_descr_hash_sql;
EXECUTE pet_descr_hash_stmt;
DEALLOCATE PREPARE pet_descr_hash_stmt;

-- Existing duplicates are not automatically removed: they can be custody
-- evidence and must be explicitly reconciled outside this migration.
-- A same-named index is reusable only when its complete ordered definition matches.
SET @uk_item_descr_matches = (
    SELECT IF(
        COUNT(*) = 3
        AND MIN(non_unique) = 0
        AND MAX(non_unique) = 0
        AND GROUP_CONCAT(column_name ORDER BY seq_in_index SEPARATOR ',') =
            'item_id,keyword,description_sha256'
        AND SUM(sub_part IS NOT NULL) = 0,
        1,
        0
    )
    FROM information_schema.statistics
    WHERE table_schema = DATABASE()
      AND table_name = 'player_item_extra_descr'
      AND index_name = 'uk_item_descr'
);
SET @uk_item_descr_present = (
    SELECT COUNT(*)
    FROM information_schema.statistics
    WHERE table_schema = DATABASE()
      AND table_name = 'player_item_extra_descr'
      AND index_name = 'uk_item_descr'
);
SET @uk_item_descr_drop_sql = IF(
    @uk_item_descr_present > 0 AND @uk_item_descr_matches = 0,
    'ALTER TABLE player_item_extra_descr DROP INDEX uk_item_descr',
    'SELECT 1 INTO @migration_noop'
);
PREPARE uk_item_descr_drop_stmt FROM @uk_item_descr_drop_sql;
EXECUTE uk_item_descr_drop_stmt;
DEALLOCATE PREPARE uk_item_descr_drop_stmt;

SET @uk_item_descr_matches = (
    SELECT IF(
        COUNT(*) = 3
        AND MIN(non_unique) = 0
        AND MAX(non_unique) = 0
        AND GROUP_CONCAT(column_name ORDER BY seq_in_index SEPARATOR ',') =
            'item_id,keyword,description_sha256'
        AND SUM(sub_part IS NOT NULL) = 0,
        1,
        0
    )
    FROM information_schema.statistics
    WHERE table_schema = DATABASE()
      AND table_name = 'player_item_extra_descr'
      AND index_name = 'uk_item_descr'
);
SET @uk_item_descr_add_sql = IF(
    @uk_item_descr_matches = 0,
    'ALTER TABLE player_item_extra_descr ADD UNIQUE KEY uk_item_descr (item_id, keyword, description_sha256)',
    'SELECT 1 INTO @migration_noop'
);
PREPARE uk_item_descr_add_stmt FROM @uk_item_descr_add_sql;
EXECUTE uk_item_descr_add_stmt;
DEALLOCATE PREPARE uk_item_descr_add_stmt;

SET @uk_pet_item_descr_matches = (
    SELECT IF(
        COUNT(*) = 3
        AND MIN(non_unique) = 0
        AND MAX(non_unique) = 0
        AND GROUP_CONCAT(column_name ORDER BY seq_in_index SEPARATOR ',') =
            'item_id,keyword,description_sha256'
        AND SUM(sub_part IS NOT NULL) = 0,
        1,
        0
    )
    FROM information_schema.statistics
    WHERE table_schema = DATABASE()
      AND table_name = 'player_pet_item_extra_descr'
      AND index_name = 'uk_pet_item_descr'
);
SET @uk_pet_item_descr_present = (
    SELECT COUNT(*)
    FROM information_schema.statistics
    WHERE table_schema = DATABASE()
      AND table_name = 'player_pet_item_extra_descr'
      AND index_name = 'uk_pet_item_descr'
);
SET @uk_pet_item_descr_drop_sql = IF(
    @uk_pet_item_descr_present > 0 AND @uk_pet_item_descr_matches = 0,
    'ALTER TABLE player_pet_item_extra_descr DROP INDEX uk_pet_item_descr',
    'SELECT 1 INTO @migration_noop'
);
PREPARE uk_pet_item_descr_drop_stmt FROM @uk_pet_item_descr_drop_sql;
EXECUTE uk_pet_item_descr_drop_stmt;
DEALLOCATE PREPARE uk_pet_item_descr_drop_stmt;

SET @uk_pet_item_descr_matches = (
    SELECT IF(
        COUNT(*) = 3
        AND MIN(non_unique) = 0
        AND MAX(non_unique) = 0
        AND GROUP_CONCAT(column_name ORDER BY seq_in_index SEPARATOR ',') =
            'item_id,keyword,description_sha256'
        AND SUM(sub_part IS NOT NULL) = 0,
        1,
        0
    )
    FROM information_schema.statistics
    WHERE table_schema = DATABASE()
      AND table_name = 'player_pet_item_extra_descr'
      AND index_name = 'uk_pet_item_descr'
);
SET @uk_pet_item_descr_add_sql = IF(
    @uk_pet_item_descr_matches = 0,
    'ALTER TABLE player_pet_item_extra_descr ADD UNIQUE KEY uk_pet_item_descr (item_id, keyword, description_sha256)',
    'SELECT 1 INTO @migration_noop'
);
PREPARE uk_pet_item_descr_add_stmt FROM @uk_pet_item_descr_add_sql;
EXECUTE uk_pet_item_descr_add_stmt;
DEALLOCATE PREPARE uk_pet_item_descr_add_stmt;
