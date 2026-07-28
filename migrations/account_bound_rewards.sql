-- Account-bound divine reward grants.
--
-- Version 0 rows are legacy vnum-only grants. Version 1 rows carry an exact,
-- versioned object snapshot in template_json. Stable grant IDs permit multiple
-- exact grants with the same account and vnum.
CREATE TABLE IF NOT EXISTS account_bound_rewards (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    account_name VARCHAR(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
    reward_vnum INT NOT NULL DEFAULT 36419,
    template_version SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    template_json LONGTEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL,
    display_name VARCHAR(512) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
    granted_by VARCHAR(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
    created_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    expires_at DATETIME NULL DEFAULT NULL,
    remaining_pwipes INT UNSIGNED NULL DEFAULT NULL,
    PRIMARY KEY (id),
    KEY idx_account_bound_rewards_account (account_name),
    KEY idx_account_bound_rewards_vnum (reward_vnum),
    KEY idx_account_bound_rewards_expires (expires_at),
    CONSTRAINT account_bound_rewards_ibfk_1 FOREIGN KEY (account_name)
        REFERENCES accounts(account_name) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Singleton timestamp guarding account-reward pwipe policy from duplicate or
-- accidentally repeated invocations. Legitimate pwipes are never closer than
-- the shortest calendar month, so 28 days is the idempotency window.
CREATE TABLE IF NOT EXISTS account_bound_reward_pwipe_state (
    id TINYINT UNSIGNED NOT NULL,
    last_processed_at DATETIME NULL DEFAULT NULL,
    PRIMARY KEY (id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP PROCEDURE IF EXISTS migrate_account_bound_rewards;
DELIMITER //
CREATE PROCEDURE migrate_account_bound_rewards()
BEGIN
    DECLARE pk_signature TEXT DEFAULT '';
    DECLARE fk_named INT DEFAULT 0;
    DECLARE fk_exact INT DEFAULT 0;
    DECLARE index_named INT DEFAULT 0;
    DECLARE index_exact INT DEFAULT 0;

    SELECT COUNT(*) INTO fk_named
      FROM information_schema.table_constraints
     WHERE constraint_schema=DATABASE() AND table_name='account_bound_rewards'
       AND constraint_name='account_bound_rewards_ibfk_1' AND constraint_type='FOREIGN KEY';
    SELECT COUNT(*) INTO fk_exact
      FROM information_schema.key_column_usage kcu
      JOIN information_schema.referential_constraints rc
        ON rc.constraint_schema=kcu.constraint_schema
       AND rc.table_name=kcu.table_name AND rc.constraint_name=kcu.constraint_name
     WHERE kcu.constraint_schema=DATABASE() AND kcu.table_name='account_bound_rewards'
       AND kcu.constraint_name='account_bound_rewards_ibfk_1'
       AND kcu.column_name='account_name' AND kcu.referenced_table_name='accounts'
       AND kcu.referenced_column_name='account_name'
       AND rc.update_rule='NO ACTION' AND rc.delete_rule='CASCADE';
    IF fk_named=1 AND fk_exact=0 THEN
        ALTER TABLE account_bound_rewards DROP FOREIGN KEY account_bound_rewards_ibfk_1;
    END IF;

    ALTER TABLE account_bound_rewards ENGINE=InnoDB,
        CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_rewards' AND column_name='account_name') THEN
        ALTER TABLE account_bound_rewards ADD COLUMN account_name VARCHAR(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL FIRST;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_rewards' AND column_name='reward_vnum') THEN
        ALTER TABLE account_bound_rewards ADD COLUMN reward_vnum INT NOT NULL DEFAULT 36419 AFTER account_name;
    END IF;
    UPDATE account_bound_rewards SET reward_vnum=36419 WHERE reward_vnum IS NULL OR CAST(reward_vnum AS CHAR) NOT REGEXP '^[0-9]+$' OR CAST(reward_vnum AS UNSIGNED)=0;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_rewards' AND column_name='template_version') THEN
        ALTER TABLE account_bound_rewards ADD COLUMN template_version SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER reward_vnum;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_rewards' AND column_name='template_json') THEN
        ALTER TABLE account_bound_rewards ADD COLUMN template_json LONGTEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL AFTER template_version;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_rewards' AND column_name='display_name') THEN
        ALTER TABLE account_bound_rewards ADD COLUMN display_name VARCHAR(512) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '' AFTER template_json;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_rewards' AND column_name='granted_by') THEN
        ALTER TABLE account_bound_rewards ADD COLUMN granted_by VARCHAR(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '' AFTER display_name;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_rewards' AND column_name='created_at') THEN
        ALTER TABLE account_bound_rewards ADD COLUMN created_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP AFTER granted_by;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_rewards' AND column_name='updated_at') THEN
        ALTER TABLE account_bound_rewards ADD COLUMN updated_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP AFTER created_at;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_rewards' AND column_name='expires_at') THEN
        ALTER TABLE account_bound_rewards ADD COLUMN expires_at DATETIME NULL DEFAULT NULL AFTER updated_at;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_rewards' AND column_name='remaining_pwipes') THEN
        ALTER TABLE account_bound_rewards ADD COLUMN remaining_pwipes INT UNSIGNED NULL DEFAULT NULL AFTER expires_at;
    END IF;

    UPDATE account_bound_rewards SET template_version=0 WHERE template_version IS NULL;
    UPDATE account_bound_rewards SET display_name='' WHERE display_name IS NULL;
    UPDATE account_bound_rewards SET granted_by='' WHERE granted_by IS NULL;
    UPDATE account_bound_rewards SET remaining_pwipes=NULL WHERE remaining_pwipes=0;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_rewards' AND column_name='id') THEN
        ALTER TABLE account_bound_rewards ADD COLUMN id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT UNIQUE FIRST;
    END IF;

    /* The legacy composite PRIMARY KEY may be the only supporting index for
     * the account_name foreign key.  Establish the target account index before
     * replacing that primary key, or MySQL correctly rejects the transition. */
    SELECT COUNT(*), COUNT(*)=1 AND MAX(non_unique)=1 AND MAX(columns_signature)='account_name'
      INTO index_named, index_exact
      FROM (
        SELECT non_unique, GROUP_CONCAT(column_name ORDER BY seq_in_index) columns_signature
          FROM information_schema.statistics
         WHERE table_schema=DATABASE() AND table_name='account_bound_rewards' AND index_name='idx_account_bound_rewards_account'
         GROUP BY non_unique
      ) x;
    IF index_named>0 AND index_exact=0 THEN ALTER TABLE account_bound_rewards DROP INDEX idx_account_bound_rewards_account; END IF;
    IF index_exact=0 THEN ALTER TABLE account_bound_rewards ADD INDEX idx_account_bound_rewards_account(account_name); END IF;

    SELECT COALESCE(GROUP_CONCAT(column_name ORDER BY seq_in_index),'') INTO pk_signature
      FROM information_schema.statistics
     WHERE table_schema=DATABASE() AND table_name='account_bound_rewards' AND index_name='PRIMARY';
    IF pk_signature <> 'id' THEN
        IF pk_signature <> '' THEN
            ALTER TABLE account_bound_rewards DROP PRIMARY KEY, ADD PRIMARY KEY (id);
        ELSE
            ALTER TABLE account_bound_rewards ADD PRIMARY KEY (id);
        END IF;
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='account_bound_rewards' AND index_name='id' AND index_name<>'PRIMARY') THEN
        ALTER TABLE account_bound_rewards DROP INDEX id;
    END IF;

    ALTER TABLE account_bound_rewards
        MODIFY COLUMN id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT FIRST,
        MODIFY COLUMN account_name VARCHAR(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL AFTER id,
        MODIFY COLUMN reward_vnum INT NOT NULL DEFAULT 36419 AFTER account_name,
        MODIFY COLUMN template_version SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER reward_vnum,
        MODIFY COLUMN template_json LONGTEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL AFTER template_version,
        MODIFY COLUMN display_name VARCHAR(512) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '' AFTER template_json,
        MODIFY COLUMN granted_by VARCHAR(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '' AFTER display_name,
        MODIFY COLUMN created_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP AFTER granted_by,
        MODIFY COLUMN updated_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP AFTER created_at,
        MODIFY COLUMN expires_at DATETIME NULL DEFAULT NULL AFTER updated_at,
        MODIFY COLUMN remaining_pwipes INT UNSIGNED NULL DEFAULT NULL AFTER expires_at;

    SELECT COUNT(*), COUNT(*)=1 AND MAX(non_unique)=1 AND MAX(columns_signature)='account_name'
      INTO index_named, index_exact
      FROM (
        SELECT non_unique, GROUP_CONCAT(column_name ORDER BY seq_in_index) columns_signature
          FROM information_schema.statistics
         WHERE table_schema=DATABASE() AND table_name='account_bound_rewards' AND index_name='idx_account_bound_rewards_account'
         GROUP BY non_unique
      ) x;
    IF index_named>0 AND index_exact=0 THEN ALTER TABLE account_bound_rewards DROP INDEX idx_account_bound_rewards_account; END IF;
    IF index_exact=0 THEN ALTER TABLE account_bound_rewards ADD INDEX idx_account_bound_rewards_account(account_name); END IF;

    SELECT COUNT(*), COUNT(*)=1 AND MAX(non_unique)=1 AND MAX(columns_signature)='reward_vnum'
      INTO index_named, index_exact
      FROM (
        SELECT non_unique, GROUP_CONCAT(column_name ORDER BY seq_in_index) columns_signature
          FROM information_schema.statistics
         WHERE table_schema=DATABASE() AND table_name='account_bound_rewards' AND index_name='idx_account_bound_rewards_vnum'
         GROUP BY non_unique
      ) x;
    IF index_named>0 AND index_exact=0 THEN ALTER TABLE account_bound_rewards DROP INDEX idx_account_bound_rewards_vnum; END IF;
    IF index_exact=0 THEN ALTER TABLE account_bound_rewards ADD INDEX idx_account_bound_rewards_vnum(reward_vnum); END IF;

    SELECT COUNT(*), COUNT(*)=1 AND MAX(non_unique)=1 AND MAX(columns_signature)='expires_at'
      INTO index_named, index_exact
      FROM (
        SELECT non_unique, GROUP_CONCAT(column_name ORDER BY seq_in_index) columns_signature
          FROM information_schema.statistics
         WHERE table_schema=DATABASE() AND table_name='account_bound_rewards' AND index_name='idx_account_bound_rewards_expires'
         GROUP BY non_unique
      ) x;
    IF index_named>0 AND index_exact=0 THEN ALTER TABLE account_bound_rewards DROP INDEX idx_account_bound_rewards_expires; END IF;
    IF index_exact=0 THEN ALTER TABLE account_bound_rewards ADD INDEX idx_account_bound_rewards_expires(expires_at); END IF;

    SELECT COUNT(*) INTO fk_exact
      FROM information_schema.key_column_usage kcu
      JOIN information_schema.referential_constraints rc
        ON rc.constraint_schema=kcu.constraint_schema
       AND rc.table_name=kcu.table_name AND rc.constraint_name=kcu.constraint_name
     WHERE kcu.constraint_schema=DATABASE() AND kcu.table_name='account_bound_rewards'
       AND kcu.constraint_name='account_bound_rewards_ibfk_1'
       AND kcu.column_name='account_name' AND kcu.referenced_table_name='accounts'
       AND kcu.referenced_column_name='account_name'
       AND rc.update_rule='NO ACTION' AND rc.delete_rule='CASCADE';
    IF fk_exact=0 THEN
        ALTER TABLE account_bound_rewards ADD CONSTRAINT account_bound_rewards_ibfk_1
            FOREIGN KEY (account_name) REFERENCES accounts(account_name) ON DELETE CASCADE;
    END IF;
END//
DELIMITER ;
CALL migrate_account_bound_rewards();
DROP PROCEDURE migrate_account_bound_rewards;

CREATE TABLE IF NOT EXISTS account_bound_reward_summons (
    grant_id BIGINT UNSIGNED NOT NULL,
    pid INT UNSIGNED NOT NULL,
    last_summoned_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    recovery_ready TINYINT(1) NOT NULL DEFAULT 0,
    PRIMARY KEY (grant_id, pid),
    KEY idx_account_bound_reward_summons_pid (pid),
    CONSTRAINT account_bound_reward_summons_grant_fk FOREIGN KEY (grant_id)
        REFERENCES account_bound_rewards(id) ON DELETE CASCADE,
    CONSTRAINT account_bound_reward_summons_pid_fk FOREIGN KEY (pid)
        REFERENCES player_data(pid) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


DROP PROCEDURE IF EXISTS migrate_account_bound_reward_summons;
DELIMITER //
CREATE PROCEDURE migrate_account_bound_reward_summons()
BEGIN
    DECLARE pk_signature TEXT DEFAULT '';
    DECLARE named_count INT DEFAULT 0;
    DECLARE exact_count INT DEFAULT 0;

    SELECT COUNT(*) INTO named_count FROM information_schema.table_constraints
     WHERE constraint_schema=DATABASE() AND table_name='account_bound_reward_summons'
       AND constraint_name='account_bound_reward_summons_grant_fk' AND constraint_type='FOREIGN KEY';
    SELECT COUNT(*) INTO exact_count FROM information_schema.key_column_usage kcu
      JOIN information_schema.referential_constraints rc ON rc.constraint_schema=kcu.constraint_schema AND rc.table_name=kcu.table_name AND rc.constraint_name=kcu.constraint_name
     WHERE kcu.constraint_schema=DATABASE() AND kcu.table_name='account_bound_reward_summons' AND kcu.constraint_name='account_bound_reward_summons_grant_fk'
       AND kcu.column_name='grant_id' AND kcu.referenced_table_name='account_bound_rewards' AND kcu.referenced_column_name='id' AND rc.delete_rule='CASCADE';
    IF named_count=1 AND exact_count=0 THEN ALTER TABLE account_bound_reward_summons DROP FOREIGN KEY account_bound_reward_summons_grant_fk; END IF;

    SELECT COUNT(*) INTO named_count FROM information_schema.table_constraints
     WHERE constraint_schema=DATABASE() AND table_name='account_bound_reward_summons'
       AND constraint_name='account_bound_reward_summons_pid_fk' AND constraint_type='FOREIGN KEY';
    SELECT COUNT(*) INTO exact_count FROM information_schema.key_column_usage kcu
      JOIN information_schema.referential_constraints rc ON rc.constraint_schema=kcu.constraint_schema AND rc.table_name=kcu.table_name AND rc.constraint_name=kcu.constraint_name
     WHERE kcu.constraint_schema=DATABASE() AND kcu.table_name='account_bound_reward_summons' AND kcu.constraint_name='account_bound_reward_summons_pid_fk'
       AND kcu.column_name='pid' AND kcu.referenced_table_name='player_data' AND kcu.referenced_column_name='pid' AND rc.delete_rule='CASCADE';
    IF named_count=1 AND exact_count=0 THEN ALTER TABLE account_bound_reward_summons DROP FOREIGN KEY account_bound_reward_summons_pid_fk; END IF;

    ALTER TABLE account_bound_reward_summons ENGINE=InnoDB, CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_reward_summons' AND column_name='grant_id') THEN
        ALTER TABLE account_bound_reward_summons ADD COLUMN grant_id BIGINT UNSIGNED NOT NULL FIRST;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_reward_summons' AND column_name='pid') THEN
        ALTER TABLE account_bound_reward_summons ADD COLUMN pid INT UNSIGNED NOT NULL AFTER grant_id;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_reward_summons' AND column_name='last_summoned_at') THEN
        ALTER TABLE account_bound_reward_summons ADD COLUMN last_summoned_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP AFTER pid;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_reward_summons' AND column_name='recovery_ready') THEN
        ALTER TABLE account_bound_reward_summons ADD COLUMN recovery_ready TINYINT(1) NOT NULL DEFAULT 0 AFTER last_summoned_at;
    END IF;
    ALTER TABLE account_bound_reward_summons
        MODIFY COLUMN grant_id BIGINT UNSIGNED NOT NULL FIRST,
        MODIFY COLUMN pid INT UNSIGNED NOT NULL AFTER grant_id,
        MODIFY COLUMN last_summoned_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP AFTER pid,
        MODIFY COLUMN recovery_ready TINYINT(1) NOT NULL DEFAULT 0 AFTER last_summoned_at;

    SELECT COALESCE(GROUP_CONCAT(column_name ORDER BY seq_in_index),'') INTO pk_signature
      FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='account_bound_reward_summons' AND index_name='PRIMARY';
    IF pk_signature<>'grant_id,pid' THEN
        IF pk_signature<>'' THEN ALTER TABLE account_bound_reward_summons DROP PRIMARY KEY, ADD PRIMARY KEY(grant_id,pid);
        ELSE ALTER TABLE account_bound_reward_summons ADD PRIMARY KEY(grant_id,pid); END IF;
    END IF;

    SELECT COUNT(*) INTO named_count FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='account_bound_reward_summons' AND index_name='idx_account_bound_reward_summons_pid';
    SELECT COUNT(*) INTO exact_count FROM (
        SELECT non_unique,GROUP_CONCAT(column_name ORDER BY seq_in_index) sig FROM information_schema.statistics
         WHERE table_schema=DATABASE() AND table_name='account_bound_reward_summons' AND index_name='idx_account_bound_reward_summons_pid' GROUP BY non_unique
         HAVING non_unique=1 AND sig='pid') x;
    IF named_count>0 AND exact_count=0 THEN ALTER TABLE account_bound_reward_summons DROP INDEX idx_account_bound_reward_summons_pid; END IF;
    IF exact_count=0 THEN ALTER TABLE account_bound_reward_summons ADD INDEX idx_account_bound_reward_summons_pid(pid); END IF;

    SELECT COUNT(*) INTO exact_count FROM information_schema.key_column_usage kcu
      JOIN information_schema.referential_constraints rc ON rc.constraint_schema=kcu.constraint_schema AND rc.table_name=kcu.table_name AND rc.constraint_name=kcu.constraint_name
     WHERE kcu.constraint_schema=DATABASE() AND kcu.table_name='account_bound_reward_summons' AND kcu.constraint_name='account_bound_reward_summons_grant_fk'
       AND kcu.column_name='grant_id' AND kcu.referenced_table_name='account_bound_rewards' AND kcu.referenced_column_name='id' AND rc.delete_rule='CASCADE';
    IF exact_count=0 THEN ALTER TABLE account_bound_reward_summons ADD CONSTRAINT account_bound_reward_summons_grant_fk FOREIGN KEY(grant_id) REFERENCES account_bound_rewards(id) ON DELETE CASCADE; END IF;

    SELECT COUNT(*) INTO exact_count FROM information_schema.key_column_usage kcu
      JOIN information_schema.referential_constraints rc ON rc.constraint_schema=kcu.constraint_schema AND rc.table_name=kcu.table_name AND rc.constraint_name=kcu.constraint_name
     WHERE kcu.constraint_schema=DATABASE() AND kcu.table_name='account_bound_reward_summons' AND kcu.constraint_name='account_bound_reward_summons_pid_fk'
       AND kcu.column_name='pid' AND kcu.referenced_table_name='player_data' AND kcu.referenced_column_name='pid' AND rc.delete_rule='CASCADE';
    IF exact_count=0 THEN ALTER TABLE account_bound_reward_summons ADD CONSTRAINT account_bound_reward_summons_pid_fk FOREIGN KEY(pid) REFERENCES player_data(pid) ON DELETE CASCADE; END IF;
END//
DELIMITER ;
CALL migrate_account_bound_reward_summons();
DROP PROCEDURE migrate_account_bound_reward_summons;

DROP PROCEDURE IF EXISTS migrate_account_bound_reward_pwipe_state;
DELIMITER //
CREATE PROCEDURE migrate_account_bound_reward_pwipe_state()
BEGIN
    DECLARE pk_signature TEXT DEFAULT '';
    DECLARE preserved_timestamp DATETIME DEFAULT NULL;

    ALTER TABLE account_bound_reward_pwipe_state ENGINE=InnoDB,
        CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_reward_pwipe_state' AND column_name='id') THEN
        ALTER TABLE account_bound_reward_pwipe_state ADD COLUMN id TINYINT UNSIGNED NULL FIRST;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_reward_pwipe_state' AND column_name='last_processed_at') THEN
        ALTER TABLE account_bound_reward_pwipe_state ADD COLUMN last_processed_at DATETIME NULL DEFAULT NULL AFTER id;
    END IF;

    SELECT MAX(last_processed_at) INTO preserved_timestamp FROM account_bound_reward_pwipe_state;
    DELETE FROM account_bound_reward_pwipe_state;
    ALTER TABLE account_bound_reward_pwipe_state
        MODIFY COLUMN id TINYINT UNSIGNED NOT NULL FIRST,
        MODIFY COLUMN last_processed_at DATETIME NULL DEFAULT NULL AFTER id;

    SELECT COALESCE(GROUP_CONCAT(column_name ORDER BY seq_in_index),'') INTO pk_signature
      FROM information_schema.statistics
     WHERE table_schema=DATABASE() AND table_name='account_bound_reward_pwipe_state' AND index_name='PRIMARY';
    IF pk_signature<>'id' THEN
        IF pk_signature<>'' THEN
            ALTER TABLE account_bound_reward_pwipe_state DROP PRIMARY KEY, ADD PRIMARY KEY(id);
        ELSE
            ALTER TABLE account_bound_reward_pwipe_state ADD PRIMARY KEY(id);
        END IF;
    END IF;

    INSERT INTO account_bound_reward_pwipe_state(id,last_processed_at) VALUES (1,preserved_timestamp);
END//
DELIMITER ;
CALL migrate_account_bound_reward_pwipe_state();
DROP PROCEDURE migrate_account_bound_reward_pwipe_state;
