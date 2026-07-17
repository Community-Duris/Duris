-- Account-wide divine reward assignment.
-- One row per account and reward vnum makes repeated assignment idempotent
-- while allowing multiple distinct divine claims per account.
-- This file is both the missing-table migration and the replay-safe repair contract
-- for partially deployed existing databases. Keep it structurally identical to
-- account_bound_rewards in bootstrap_multithread_safe.sql.
CREATE TABLE IF NOT EXISTS account_bound_rewards (
    account_name VARCHAR(50) NOT NULL,
    reward_vnum INT NOT NULL DEFAULT 36419,
    granted_by VARCHAR(50) NOT NULL DEFAULT '',
    created_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (account_name, reward_vnum),
    KEY idx_account_bound_rewards_vnum (reward_vnum),
    CONSTRAINT account_bound_rewards_ibfk_1 FOREIGN KEY (account_name)
        REFERENCES accounts(account_name) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- A same-named but structurally wrong FK must not block column or engine repair.
SET @account_bound_rewards_fk_named := (
    SELECT COUNT(*) FROM information_schema.table_constraints
    WHERE constraint_schema = DATABASE()
      AND table_name = 'account_bound_rewards'
      AND constraint_name = 'account_bound_rewards_ibfk_1'
      AND constraint_type = 'FOREIGN KEY'
);
SET @account_bound_rewards_fk_exact := (
    SELECT COUNT(*)
    FROM information_schema.key_column_usage kcu
    JOIN information_schema.referential_constraints rc
      ON rc.constraint_schema = kcu.constraint_schema
     AND rc.table_name = kcu.table_name
     AND rc.constraint_name = kcu.constraint_name
    WHERE kcu.constraint_schema = DATABASE()
      AND kcu.table_name = 'account_bound_rewards'
      AND kcu.constraint_name = 'account_bound_rewards_ibfk_1'
      AND kcu.column_name = 'account_name'
      AND kcu.referenced_table_name = 'accounts'
      AND kcu.referenced_column_name = 'account_name'
      AND rc.update_rule = 'NO ACTION'
      AND rc.delete_rule = 'CASCADE'
);
SET @account_bound_rewards_sql := IF(
    @account_bound_rewards_fk_named = 1 AND @account_bound_rewards_fk_exact = 0,
    'ALTER TABLE account_bound_rewards DROP FOREIGN KEY account_bound_rewards_ibfk_1',
    'DO 1'
);
PREPARE account_bound_rewards_stmt FROM @account_bound_rewards_sql;
EXECUTE account_bound_rewards_stmt;
DEALLOCATE PREPARE account_bound_rewards_stmt;

-- Repair the table storage contract before adding the FK. This preserves rows.
SET @account_bound_rewards_sql := IF(
    (SELECT COUNT(*) FROM information_schema.tables
     WHERE table_schema = DATABASE()
       AND table_name = 'account_bound_rewards'
       AND engine = 'InnoDB'
       AND table_collation = 'utf8mb4_unicode_ci') = 1,
    'DO 1',
    'ALTER TABLE account_bound_rewards ENGINE=InnoDB, CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci'
);
PREPARE account_bound_rewards_stmt FROM @account_bound_rewards_sql;
EXECUTE account_bound_rewards_stmt;
DEALLOCATE PREPARE account_bound_rewards_stmt;

-- Guard every column independently so interrupted deployments converge.
SET @account_bound_rewards_sql := IF(
    (SELECT COUNT(*) FROM information_schema.columns
     WHERE table_schema = DATABASE() AND table_name = 'account_bound_rewards'
       AND column_name = 'account_name') = 0,
    'ALTER TABLE account_bound_rewards ADD COLUMN account_name VARCHAR(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL FIRST',
    'DO 1'
);
PREPARE account_bound_rewards_stmt FROM @account_bound_rewards_sql;
EXECUTE account_bound_rewards_stmt;
DEALLOCATE PREPARE account_bound_rewards_stmt;

SET @account_bound_rewards_sql := IF(
    (SELECT COUNT(*) FROM information_schema.columns
     WHERE table_schema = DATABASE() AND table_name = 'account_bound_rewards'
       AND ordinal_position = 1 AND column_name = 'account_name'
       AND column_type = 'varchar(50)' AND is_nullable = 'NO'
       AND character_set_name = 'utf8mb4' AND collation_name = 'utf8mb4_unicode_ci') = 1,
    'DO 1',
    'ALTER TABLE account_bound_rewards MODIFY COLUMN account_name VARCHAR(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL FIRST'
);
PREPARE account_bound_rewards_stmt FROM @account_bound_rewards_sql;
EXECUTE account_bound_rewards_stmt;
DEALLOCATE PREPARE account_bound_rewards_stmt;

SET @account_bound_rewards_sql := IF(
    (SELECT COUNT(*) FROM information_schema.columns
     WHERE table_schema = DATABASE() AND table_name = 'account_bound_rewards'
       AND column_name = 'reward_vnum') = 0,
    'ALTER TABLE account_bound_rewards ADD COLUMN reward_vnum INT NOT NULL DEFAULT 36419 AFTER account_name',
    'DO 1'
);
PREPARE account_bound_rewards_stmt FROM @account_bound_rewards_sql;
EXECUTE account_bound_rewards_stmt;
DEALLOCATE PREPARE account_bound_rewards_stmt;

-- Preserve valid positive assignments; normalize only unusable legacy values.
UPDATE account_bound_rewards
SET reward_vnum = 36419
WHERE reward_vnum IS NULL
   OR CAST(reward_vnum AS CHAR) NOT REGEXP '^[0-9]+$'
   OR CAST(reward_vnum AS UNSIGNED) = 0;

SET @account_bound_rewards_sql := IF(
    (SELECT COUNT(*) FROM information_schema.columns
     WHERE table_schema = DATABASE() AND table_name = 'account_bound_rewards'
       AND ordinal_position = 2 AND column_name = 'reward_vnum'
       AND column_type = 'int' AND is_nullable = 'NO'
       AND column_default = '36419') = 1,
    'DO 1',
    'ALTER TABLE account_bound_rewards MODIFY COLUMN reward_vnum INT NOT NULL DEFAULT 36419 AFTER account_name'
);
PREPARE account_bound_rewards_stmt FROM @account_bound_rewards_sql;
EXECUTE account_bound_rewards_stmt;
DEALLOCATE PREPARE account_bound_rewards_stmt;

SET @account_bound_rewards_sql := IF(
    (SELECT COUNT(*) FROM information_schema.columns
     WHERE table_schema = DATABASE() AND table_name = 'account_bound_rewards'
       AND column_name = 'granted_by') = 0,
    'ALTER TABLE account_bound_rewards ADD COLUMN granted_by VARCHAR(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '''' AFTER reward_vnum',
    'DO 1'
);
PREPARE account_bound_rewards_stmt FROM @account_bound_rewards_sql;
EXECUTE account_bound_rewards_stmt;
DEALLOCATE PREPARE account_bound_rewards_stmt;
UPDATE account_bound_rewards SET granted_by = '' WHERE granted_by IS NULL;
SET @account_bound_rewards_sql := IF(
    (SELECT COUNT(*) FROM information_schema.columns
     WHERE table_schema = DATABASE() AND table_name = 'account_bound_rewards'
       AND ordinal_position = 3 AND column_name = 'granted_by'
       AND column_type = 'varchar(50)' AND is_nullable = 'NO'
       AND column_default = '' AND character_set_name = 'utf8mb4'
       AND collation_name = 'utf8mb4_unicode_ci') = 1,
    'DO 1',
    'ALTER TABLE account_bound_rewards MODIFY COLUMN granted_by VARCHAR(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '''' AFTER reward_vnum'
);
PREPARE account_bound_rewards_stmt FROM @account_bound_rewards_sql;
EXECUTE account_bound_rewards_stmt;
DEALLOCATE PREPARE account_bound_rewards_stmt;

SET @account_bound_rewards_sql := IF(
    (SELECT COUNT(*) FROM information_schema.columns
     WHERE table_schema = DATABASE() AND table_name = 'account_bound_rewards'
       AND column_name = 'created_at') = 0,
    'ALTER TABLE account_bound_rewards ADD COLUMN created_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP AFTER granted_by',
    'DO 1'
);
PREPARE account_bound_rewards_stmt FROM @account_bound_rewards_sql;
EXECUTE account_bound_rewards_stmt;
DEALLOCATE PREPARE account_bound_rewards_stmt;
SET @account_bound_rewards_sql := IF(
    (SELECT COUNT(*) FROM information_schema.columns
     WHERE table_schema = DATABASE() AND table_name = 'account_bound_rewards'
       AND ordinal_position = 4 AND column_name = 'created_at'
       AND data_type = 'timestamp' AND is_nullable = 'YES'
       AND UPPER(column_default) = 'CURRENT_TIMESTAMP') = 1,
    'DO 1',
    'ALTER TABLE account_bound_rewards MODIFY COLUMN created_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP AFTER granted_by'
);
PREPARE account_bound_rewards_stmt FROM @account_bound_rewards_sql;
EXECUTE account_bound_rewards_stmt;
DEALLOCATE PREPARE account_bound_rewards_stmt;

SET @account_bound_rewards_sql := IF(
    (SELECT COUNT(*) FROM information_schema.columns
     WHERE table_schema = DATABASE() AND table_name = 'account_bound_rewards'
       AND column_name = 'updated_at') = 0,
    'ALTER TABLE account_bound_rewards ADD COLUMN updated_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP AFTER created_at',
    'DO 1'
);
PREPARE account_bound_rewards_stmt FROM @account_bound_rewards_sql;
EXECUTE account_bound_rewards_stmt;
DEALLOCATE PREPARE account_bound_rewards_stmt;
SET @account_bound_rewards_sql := IF(
    (SELECT COUNT(*) FROM information_schema.columns
     WHERE table_schema = DATABASE() AND table_name = 'account_bound_rewards'
       AND ordinal_position = 5 AND column_name = 'updated_at'
       AND data_type = 'timestamp' AND is_nullable = 'YES'
       AND UPPER(column_default) = 'CURRENT_TIMESTAMP'
       AND LOWER(extra) LIKE '%on update current_timestamp%') = 1,
    'DO 1',
    'ALTER TABLE account_bound_rewards MODIFY COLUMN updated_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP AFTER created_at'
);
PREPARE account_bound_rewards_stmt FROM @account_bound_rewards_sql;
EXECUTE account_bound_rewards_stmt;
DEALLOCATE PREPARE account_bound_rewards_stmt;

-- Restore the exact primary-key and reward lookup index signatures.
SET @account_bound_rewards_pk_exists := (
    SELECT COUNT(DISTINCT index_name) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'account_bound_rewards'
      AND index_name = 'PRIMARY'
);
SET @account_bound_rewards_pk_exact := (
    SELECT COUNT(*) FROM (
        SELECT index_name,
               GROUP_CONCAT(column_name ORDER BY seq_in_index) AS columns_signature
        FROM information_schema.statistics
        WHERE table_schema = DATABASE() AND table_name = 'account_bound_rewards'
          AND index_name = 'PRIMARY'
        GROUP BY index_name
        HAVING columns_signature = 'account_name,reward_vnum'
    ) AS exact_account_bound_rewards_pk
);
SET @account_bound_rewards_sql := IF(
    @account_bound_rewards_pk_exact = 1,
    'DO 1',
    IF(@account_bound_rewards_pk_exists = 1,
       'ALTER TABLE account_bound_rewards DROP PRIMARY KEY, ADD PRIMARY KEY (account_name, reward_vnum)',
       'ALTER TABLE account_bound_rewards ADD PRIMARY KEY (account_name, reward_vnum)')
);
PREPARE account_bound_rewards_stmt FROM @account_bound_rewards_sql;
EXECUTE account_bound_rewards_stmt;
DEALLOCATE PREPARE account_bound_rewards_stmt;

SET @account_bound_rewards_index_exists := (
    SELECT COUNT(DISTINCT index_name) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'account_bound_rewards'
      AND index_name = 'idx_account_bound_rewards_vnum'
);
SET @account_bound_rewards_index_exact := (
    SELECT COUNT(*) FROM (
        SELECT index_name, non_unique,
               GROUP_CONCAT(column_name ORDER BY seq_in_index) AS columns_signature
        FROM information_schema.statistics
        WHERE table_schema = DATABASE() AND table_name = 'account_bound_rewards'
          AND index_name = 'idx_account_bound_rewards_vnum'
        GROUP BY index_name, non_unique
        HAVING non_unique = 1 AND columns_signature = 'reward_vnum'
    ) AS exact_account_bound_rewards_index
);
SET @account_bound_rewards_sql := IF(
    @account_bound_rewards_index_exact = 1,
    'DO 1',
    IF(@account_bound_rewards_index_exists = 1,
       'ALTER TABLE account_bound_rewards DROP INDEX idx_account_bound_rewards_vnum, ADD INDEX idx_account_bound_rewards_vnum (reward_vnum)',
       'ALTER TABLE account_bound_rewards ADD INDEX idx_account_bound_rewards_vnum (reward_vnum)')
);
PREPARE account_bound_rewards_stmt FROM @account_bound_rewards_sql;
EXECUTE account_bound_rewards_stmt;
DEALLOCATE PREPARE account_bound_rewards_stmt;

-- Add the canonical cascade FK only after both sides have compatible structure.
SET @account_bound_rewards_fk_exact := (
    SELECT COUNT(*)
    FROM information_schema.key_column_usage kcu
    JOIN information_schema.referential_constraints rc
      ON rc.constraint_schema = kcu.constraint_schema
     AND rc.table_name = kcu.table_name
     AND rc.constraint_name = kcu.constraint_name
    WHERE kcu.constraint_schema = DATABASE()
      AND kcu.table_name = 'account_bound_rewards'
      AND kcu.constraint_name = 'account_bound_rewards_ibfk_1'
      AND kcu.column_name = 'account_name'
      AND kcu.referenced_table_name = 'accounts'
      AND kcu.referenced_column_name = 'account_name'
      AND rc.update_rule = 'NO ACTION'
      AND rc.delete_rule = 'CASCADE'
);
SET @account_bound_rewards_sql := IF(
    @account_bound_rewards_fk_exact = 1,
    'DO 1',
    'ALTER TABLE account_bound_rewards ADD CONSTRAINT account_bound_rewards_ibfk_1 FOREIGN KEY (account_name) REFERENCES accounts(account_name) ON DELETE CASCADE'
);
PREPARE account_bound_rewards_stmt FROM @account_bound_rewards_sql;
EXECUTE account_bound_rewards_stmt;
DEALLOCATE PREPARE account_bound_rewards_stmt;
