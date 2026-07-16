-- Account-wide divine reward assignment.
-- One row per account makes reassignment idempotent and prevents duplicate grants.
CREATE TABLE IF NOT EXISTS account_bound_rewards (
    account_name VARCHAR(50) NOT NULL,
    reward_vnum INT NOT NULL DEFAULT 36419,
    granted_by VARCHAR(50) NOT NULL DEFAULT '',
    created_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (account_name),
    KEY idx_account_bound_rewards_vnum (reward_vnum),
    CONSTRAINT account_bound_rewards_ibfk_1 FOREIGN KEY (account_name)
        REFERENCES accounts(account_name) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

SET @account_bound_rewards_fk_exists := (
    SELECT COUNT(*) FROM information_schema.TABLE_CONSTRAINTS
    WHERE CONSTRAINT_SCHEMA = DATABASE()
      AND TABLE_NAME = 'account_bound_rewards'
      AND CONSTRAINT_NAME = 'account_bound_rewards_ibfk_1'
      AND CONSTRAINT_TYPE = 'FOREIGN KEY'
);
SET @account_bound_rewards_fk_sql := IF(
    @account_bound_rewards_fk_exists = 0,
    'ALTER TABLE account_bound_rewards ADD CONSTRAINT account_bound_rewards_ibfk_1 FOREIGN KEY (account_name) REFERENCES accounts(account_name) ON DELETE CASCADE',
    'SELECT 1'
);
PREPARE account_bound_rewards_fk_stmt FROM @account_bound_rewards_fk_sql;
EXECUTE account_bound_rewards_fk_stmt;
DEALLOCATE PREPARE account_bound_rewards_fk_stmt;
