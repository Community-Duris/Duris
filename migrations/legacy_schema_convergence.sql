-- Normalize legacy-upgrade metadata to the sealed fresh-bootstrap contract.
-- The legacy runner is re-runnable, so every conditional DDL operation must be too.

SET sql_mode='';

DROP PROCEDURE IF EXISTS duris_add_column_if_missing;
DROP PROCEDURE IF EXISTS duris_drop_column_if_present;
DROP PROCEDURE IF EXISTS duris_drop_index_if_present;
DROP PROCEDURE IF EXISTS duris_drop_fk_if_present;

DELIMITER //
CREATE PROCEDURE duris_add_column_if_missing(
    IN table_name_value VARCHAR(64),
    IN column_name_value VARCHAR(64),
    IN definition_value TEXT
)
BEGIN
    IF NOT EXISTS (
        SELECT 1 FROM information_schema.columns
        WHERE table_schema=DATABASE()
          AND table_name=table_name_value
          AND column_name=column_name_value
    ) THEN
        SET @ddl=CONCAT('ALTER TABLE `',table_name_value,'` ADD COLUMN `',
                        column_name_value,'` ',definition_value);
        PREPARE statement_handle FROM @ddl;
        EXECUTE statement_handle;
        DEALLOCATE PREPARE statement_handle;
    END IF;
END //

CREATE PROCEDURE duris_drop_column_if_present(
    IN table_name_value VARCHAR(64),
    IN column_name_value VARCHAR(64)
)
BEGIN
    IF EXISTS (
        SELECT 1 FROM information_schema.columns
        WHERE table_schema=DATABASE()
          AND table_name=table_name_value
          AND column_name=column_name_value
    ) THEN
        SET @ddl=CONCAT('ALTER TABLE `',table_name_value,'` DROP COLUMN `',
                        column_name_value,'`');
        PREPARE statement_handle FROM @ddl;
        EXECUTE statement_handle;
        DEALLOCATE PREPARE statement_handle;
    END IF;
END //

CREATE PROCEDURE duris_drop_index_if_present(
    IN table_name_value VARCHAR(64),
    IN index_name_value VARCHAR(64)
)
BEGIN
    IF EXISTS (
        SELECT 1 FROM information_schema.statistics
        WHERE table_schema=DATABASE()
          AND table_name=table_name_value
          AND index_name=index_name_value
    ) THEN
        SET @ddl=CONCAT('ALTER TABLE `',table_name_value,'` DROP INDEX `',
                        index_name_value,'`');
        PREPARE statement_handle FROM @ddl;
        EXECUTE statement_handle;
        DEALLOCATE PREPARE statement_handle;
    END IF;
END //

CREATE PROCEDURE duris_drop_fk_if_present(
    IN table_name_value VARCHAR(64),
    IN constraint_name_value VARCHAR(64)
)
BEGIN
    IF EXISTS (
        SELECT 1 FROM information_schema.referential_constraints
        WHERE constraint_schema=DATABASE()
          AND table_name=table_name_value
          AND constraint_name=constraint_name_value
    ) THEN
        SET @ddl=CONCAT('ALTER TABLE `',table_name_value,'` DROP FOREIGN KEY `',
                        constraint_name_value,'`');
        PREPARE statement_handle FROM @ddl;
        EXECUTE statement_handle;
        DEALLOCATE PREPARE statement_handle;
    END IF;
END //
DELIMITER ;

CALL duris_add_column_if_missing('account_characters','email',
    'VARCHAR(255) DEFAULT NULL AFTER `deleted_at`');
CALL duris_add_column_if_missing('account_characters','last_ip',
    'VARCHAR(45) DEFAULT NULL AFTER `email`');
CALL duris_drop_fk_if_present('account_characters','account_characters_ibfk_1');
CALL duris_drop_index_if_present('account_characters','idx_account_name');
CALL duris_drop_index_if_present('account_characters','idx_char_name');
CALL duris_drop_index_if_present('account_characters','account_name');
CALL duris_drop_index_if_present('account_characters','char_name');
CALL duris_drop_index_if_present('account_characters','deleted_at');
ALTER TABLE account_characters
    MODIFY COLUMN id INT NOT NULL AUTO_INCREMENT FIRST,
    MODIFY COLUMN account_name VARCHAR(255) NOT NULL AFTER id,
    MODIFY COLUMN pid BIGINT NOT NULL AFTER account_name,
    MODIFY COLUMN char_name VARCHAR(255) NOT NULL AFTER pid,
    MODIFY COLUMN created_at DATETIME DEFAULT CURRENT_TIMESTAMP AFTER char_name,
    MODIFY COLUMN deleted_at DATETIME DEFAULT NULL AFTER created_at,
    MODIFY COLUMN email VARCHAR(255) DEFAULT NULL AFTER deleted_at,
    MODIFY COLUMN last_ip VARCHAR(45) DEFAULT NULL AFTER email,
    MODIFY COLUMN login_count BIGINT UNSIGNED DEFAULT 0 AFTER last_ip,
    MODIFY COLUMN last_login TIMESTAMP NULL DEFAULT NULL AFTER login_count,
    MODIFY COLUMN blocked TINYINT DEFAULT 0 AFTER last_login,
    MODIFY COLUMN racewar TINYINT DEFAULT 0 AFTER blocked,
    ADD KEY account_name (account_name),
    ADD KEY char_name (char_name),
    ADD KEY deleted_at (deleted_at),
    COMMENT='Maps characters to accounts for web statistics';

CALL duris_add_column_if_missing('account_locker_items','bitvector1',
    'BIGINT UNSIGNED DEFAULT NULL AFTER `item_condition`');
CALL duris_add_column_if_missing('account_locker_items','bitvector2',
    'BIGINT UNSIGNED DEFAULT NULL AFTER `bitvector1`');
CALL duris_add_column_if_missing('account_locker_items','bitvector3',
    'BIGINT UNSIGNED DEFAULT NULL AFTER `bitvector2`');
CALL duris_add_column_if_missing('account_locker_items','bitvector4',
    'BIGINT UNSIGNED DEFAULT NULL AFTER `bitvector3`');
CALL duris_add_column_if_missing('account_locker_items','bitvector5',
    'BIGINT UNSIGNED DEFAULT NULL AFTER `bitvector4`');
ALTER TABLE account_locker_items
    MODIFY COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL AFTER action_descr,
    MODIFY COLUMN item_condition SMALLINT DEFAULT 100 AFTER obj_uid,
    MODIFY COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER item_condition,
    MODIFY COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1,
    MODIFY COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2,
    MODIFY COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3,
    MODIFY COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4,
    MODIFY COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5;

CALL duris_drop_column_if_present('accounts','updated_at');
CALL duris_drop_index_if_present('accounts','idx_email');
CALL duris_drop_index_if_present('accounts','idx_last_login');
ALTER TABLE accounts
    MODIFY COLUMN account_name VARCHAR(50) NOT NULL FIRST,
    MODIFY COLUMN email VARCHAR(255) DEFAULT NULL AFTER account_name,
    MODIFY COLUMN created_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP AFTER email,
    MODIFY COLUMN last_login TIMESTAMP NULL DEFAULT NULL AFTER created_at,
    MODIFY COLUMN password VARCHAR(128) NOT NULL DEFAULT '' AFTER last_login,
    MODIFY COLUMN confirmation_code VARCHAR(64) DEFAULT NULL AFTER password,
    MODIFY COLUMN confirmed TINYINT(1) DEFAULT 0 AFTER confirmation_code,
    MODIFY COLUMN confirmation_sent TINYINT(1) DEFAULT 0 AFTER confirmed,
    MODIFY COLUMN blocked TINYINT(1) DEFAULT 0 AFTER confirmation_sent,
    MODIFY COLUMN last_good_char TIMESTAMP NULL DEFAULT NULL AFTER blocked,
    MODIFY COLUMN last_evil_char TIMESTAMP NULL DEFAULT NULL AFTER last_good_char,
    MODIFY COLUMN flags1 BIGINT UNSIGNED DEFAULT 0 AFTER last_evil_char,
    MODIFY COLUMN flags2 BIGINT UNSIGNED DEFAULT 0 AFTER flags1,
    MODIFY COLUMN flags3 BIGINT UNSIGNED DEFAULT 0 AFTER flags2,
    MODIFY COLUMN flags4 BIGINT UNSIGNED DEFAULT 0 AFTER flags3,
    MODIFY COLUMN total_donated DECIMAL(10,2) DEFAULT 0.00 AFTER flags4,
    ADD KEY idx_last_login (last_login DESC);

ALTER TABLE artifacts
    MODIFY COLUMN vnum INT NOT NULL FIRST,
    MODIFY COLUMN owned VARCHAR(10) DEFAULT 'N' AFTER vnum,
    MODIFY COLUMN location INT DEFAULT NULL AFTER owned,
    MODIFY COLUMN timer DATETIME DEFAULT NULL AFTER location,
    MODIFY COLUMN type INT DEFAULT NULL AFTER timer,
    MODIFY COLUMN lastUpdate DATETIME DEFAULT NULL AFTER type,
    MODIFY COLUMN locType INT NOT NULL DEFAULT 1 AFTER lastUpdate;

CALL duris_add_column_if_missing('artifacts_mortal','lastUpdate',
    'DATETIME DEFAULT NULL AFTER `type`');
ALTER TABLE artifacts_mortal
    MODIFY COLUMN vnum INT NOT NULL FIRST,
    MODIFY COLUMN owned VARCHAR(10) DEFAULT 'N' AFTER vnum,
    MODIFY COLUMN location INT DEFAULT NULL AFTER owned,
    MODIFY COLUMN timer DATETIME DEFAULT NULL AFTER location,
    MODIFY COLUMN type INT DEFAULT NULL AFTER timer,
    MODIFY COLUMN lastUpdate DATETIME DEFAULT NULL AFTER type,
    MODIFY COLUMN locType INT NOT NULL DEFAULT 1 AFTER lastUpdate;

CALL duris_add_column_if_missing('auctions','obj_info_text',
    'MEDIUMTEXT DEFAULT NULL AFTER `id_keywords`');
CALL duris_drop_index_if_present('auctions','auction_end');
ALTER TABLE auctions
    MODIFY COLUMN id INT NOT NULL AUTO_INCREMENT FIRST,
    MODIFY COLUMN seller_pid INT UNSIGNED NOT NULL DEFAULT 0 AFTER id,
    MODIFY COLUMN seller_name VARCHAR(32) NOT NULL DEFAULT '' AFTER seller_pid,
    MODIFY COLUMN status ENUM('OPEN','CLOSED','REMOVED') NOT NULL DEFAULT 'OPEN' AFTER seller_name,
    MODIFY COLUMN winning_bidder_pid INT NOT NULL DEFAULT 0 AFTER status,
    MODIFY COLUMN winning_bidder_name VARCHAR(32) NOT NULL DEFAULT '' AFTER winning_bidder_pid,
    MODIFY COLUMN cur_price INT UNSIGNED NOT NULL DEFAULT 0 AFTER winning_bidder_name,
    MODIFY COLUMN buy_price INT NOT NULL DEFAULT 0 AFTER cur_price,
    MODIFY COLUMN obj_short VARCHAR(255) NOT NULL DEFAULT '' AFTER buy_price,
    MODIFY COLUMN obj_vnum INT NOT NULL DEFAULT 0 AFTER obj_short,
    MODIFY COLUMN obj_blob_str BLOB NOT NULL AFTER obj_vnum,
    MODIFY COLUMN id_keywords VARCHAR(1024) DEFAULT NULL AFTER obj_blob_str,
    MODIFY COLUMN obj_info_text MEDIUMTEXT DEFAULT NULL AFTER id_keywords,
    MODIFY COLUMN quantity INT NOT NULL DEFAULT 1 AFTER obj_info_text,
    MODIFY COLUMN start_time TIMESTAMP NULL DEFAULT NULL AFTER quantity,
    MODIFY COLUMN end_time TIMESTAMP NULL DEFAULT NULL AFTER start_time,
    MODIFY COLUMN auction_revision BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER end_time,
    MODIFY COLUMN custody_state TINYINT UNSIGNED NOT NULL DEFAULT 0 AFTER auction_revision,
    MODIFY COLUMN listing_operation_id BINARY(16) DEFAULT NULL AFTER custody_state;

ALTER TABLE ip_info
    MODIFY COLUMN last_connect TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP AFTER last_ip,
    MODIFY COLUMN last_disconnect TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP AFTER last_connect;

ALTER TABLE level_cap
    MODIFY COLUMN id INT UNSIGNED NOT NULL AUTO_INCREMENT FIRST,
    MODIFY COLUMN most_frags FLOAT NOT NULL AFTER id,
    MODIFY COLUMN racewar_leader INT NOT NULL AFTER most_frags,
    MODIFY COLUMN level INT NOT NULL AFTER racewar_leader,
    MODIFY COLUMN next_update DATETIME NOT NULL AFTER level;

CALL duris_add_column_if_missing('locker_items','bitvector1',
    'BIGINT UNSIGNED DEFAULT NULL AFTER `item_condition`');
CALL duris_add_column_if_missing('locker_items','bitvector2',
    'BIGINT UNSIGNED DEFAULT NULL AFTER `bitvector1`');
CALL duris_add_column_if_missing('locker_items','bitvector3',
    'BIGINT UNSIGNED DEFAULT NULL AFTER `bitvector2`');
CALL duris_add_column_if_missing('locker_items','bitvector4',
    'BIGINT UNSIGNED DEFAULT NULL AFTER `bitvector3`');
CALL duris_add_column_if_missing('locker_items','bitvector5',
    'BIGINT UNSIGNED DEFAULT NULL AFTER `bitvector4`');
ALTER TABLE locker_items
    MODIFY COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL AFTER action_descr,
    MODIFY COLUMN item_condition SMALLINT DEFAULT 100 AFTER obj_uid,
    MODIFY COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER item_condition,
    MODIFY COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1,
    MODIFY COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2,
    MODIFY COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3,
    MODIFY COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4,
    MODIFY COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5;

CALL duris_add_column_if_missing('pkill_event','like_count',
    'INT NOT NULL DEFAULT 0 AFTER `tweeted`');
CALL duris_add_column_if_missing('pkill_event','comment_count',
    'INT NOT NULL DEFAULT 0 AFTER `like_count`');
CALL duris_drop_index_if_present('pkill_event','idx_event_likes');
CALL duris_drop_index_if_present('pkill_event','idx_event_comments');
ALTER TABLE pkill_event
    MODIFY COLUMN like_count INT NOT NULL DEFAULT 0 AFTER tweeted,
    MODIFY COLUMN comment_count INT NOT NULL DEFAULT 0 AFTER like_count,
    ADD KEY idx_event_likes (like_count),
    ADD KEY idx_event_comments (comment_count);

CALL duris_add_column_if_missing('player_items','created_at',
    'TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP AFTER `item_condition`');
ALTER TABLE player_items
    MODIFY COLUMN created_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP AFTER item_condition,
    MODIFY COLUMN item_material TINYINT DEFAULT NULL AFTER created_at;

CALL duris_drop_index_if_present('poll_options','idx_poll_id');
CALL duris_drop_index_if_present('poll_options','poll_id');
ALTER TABLE poll_options
    MODIFY COLUMN id INT NOT NULL AUTO_INCREMENT FIRST,
    MODIFY COLUMN poll_id INT NOT NULL AFTER id,
    ADD KEY poll_id (poll_id);

CALL duris_drop_index_if_present('poll_votes','idx_account_name');
CALL duris_drop_index_if_present('poll_votes','idx_poll_id');
CALL duris_drop_index_if_present('poll_votes','account_name');
CALL duris_drop_index_if_present('poll_votes','poll_id');
CALL duris_drop_index_if_present('poll_votes','uk_poll_account_option');
CALL duris_drop_index_if_present('poll_votes','unique_vote');
ALTER TABLE poll_votes
    MODIFY COLUMN id INT NOT NULL AUTO_INCREMENT FIRST,
    MODIFY COLUMN poll_id INT NOT NULL AFTER id,
    MODIFY COLUMN account_name VARCHAR(64) NOT NULL AFTER poll_id,
    MODIFY COLUMN option_id INT NOT NULL AFTER account_name,
    MODIFY COLUMN voted_at INT NOT NULL DEFAULT 0 AFTER option_id,
    MODIFY COLUMN char_name VARCHAR(32) NOT NULL AFTER voted_at,
    ADD UNIQUE KEY unique_vote (poll_id,account_name,option_id),
    ADD KEY poll_id (poll_id),
    ADD KEY account_name (account_name);

ALTER TABLE polls
    MODIFY COLUMN id INT NOT NULL AUTO_INCREMENT FIRST,
    MODIFY COLUMN created_at INT NOT NULL DEFAULT 0 AFTER created_by,
    MODIFY COLUMN expires_at INT NOT NULL DEFAULT 0 AFTER created_at;

ALTER TABLE prepstatement_duris_sql
    MODIFY COLUMN description TEXT DEFAULT NULL AFTER id,
    MODIFY COLUMN sql_code TEXT DEFAULT NULL AFTER description;

CALL duris_drop_index_if_present('progress','index_var_type');
CALL duris_drop_index_if_present('progress','index_enum');
ALTER TABLE progress
    MODIFY COLUMN var_type ENUM('FRAGS','EXP') NOT NULL DEFAULT 'FRAGS' AFTER pid,
    MODIFY COLUMN stamp DATETIME NOT NULL DEFAULT '0000-00-00 00:00:00' AFTER var_type,
    ADD KEY index_enum (var_type);

ALTER TABLE saved_items
    MODIFY COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL AFTER action_descr,
    MODIFY COLUMN created_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP AFTER obj_uid,
    MODIFY COLUMN updated_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP AFTER created_at,
    MODIFY COLUMN item_material TINYINT DEFAULT NULL AFTER updated_at,
    MODIFY COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER item_material,
    MODIFY COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1,
    MODIFY COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2,
    MODIFY COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3,
    MODIFY COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4;

ALTER TABLE zone_touches
    MODIFY COLUMN zone_number INT DEFAULT NULL AFTER id,
    MODIFY COLUMN toucher_pid INT DEFAULT NULL AFTER zone_number,
    MODIFY COLUMN group_size INT DEFAULT NULL AFTER toucher_pid,
    MODIFY COLUMN epic_value INT DEFAULT NULL AFTER group_size,
    MODIFY COLUMN alignment_delta INT DEFAULT NULL AFTER epic_value,
    MODIFY COLUMN boot_time TIMESTAMP NULL DEFAULT NULL AFTER alignment_delta,
    MODIFY COLUMN touched_at TIMESTAMP NULL DEFAULT NULL AFTER boot_time;

ALTER TABLE zones
    MODIFY COLUMN id INT(11) NOT NULL AUTO_INCREMENT FIRST,
    MODIFY COLUMN number INT(11) DEFAULT NULL AFTER id,
    MODIFY COLUMN suggested_group_size INT(11) NOT NULL DEFAULT 1 AFTER trophy_zone,
    MODIFY COLUMN epic_payout INT(11) NOT NULL DEFAULT 0 AFTER suggested_group_size,
    MODIFY COLUMN difficulty INT(11) NOT NULL DEFAULT 0 AFTER epic_payout,
    MODIFY COLUMN reset_perc INT DEFAULT 0 AFTER alignment,
    MODIFY COLUMN stonecount INT NOT NULL DEFAULT 1 AFTER reset_perc,
    MODIFY COLUMN last_touch TIMESTAMP NULL DEFAULT NULL AFTER stonecount;

-- MariaDB retains integer display widths that MySQL 8 omits. Normalize the
-- handful of legacy widths whose declarations differ from the sealed baseline.
ALTER TABLE epic_bonus
    MODIFY COLUMN pid INT(11) NOT NULL FIRST,
    MODIFY COLUMN type INT(11) NOT NULL DEFAULT 0 AFTER pid;
ALTER TABLE log_entries
    MODIFY COLUMN pid INT(11) NOT NULL DEFAULT 0 AFTER player_name,
    MODIFY COLUMN room_vnum INT(11) NOT NULL DEFAULT 0 AFTER ip_address;
ALTER TABLE outposts
    MODIFY COLUMN meurtriere INT(11) NOT NULL DEFAULT 0 AFTER golems;
ALTER TABLE ping
    MODIFY COLUMN ID BIGINT(20) NOT NULL AUTO_INCREMENT FIRST;
ALTER TABLE world_quest_accomplished
    MODIFY COLUMN quest_target INT(11) NOT NULL DEFAULT 0 AFTER player_level,
    MODIFY COLUMN reward_vnum INT(11) NOT NULL DEFAULT 0 AFTER quest_target;
ALTER TABLE zone_touches
    MODIFY COLUMN id INT(11) NOT NULL AUTO_INCREMENT FIRST;

-- Immutable migration 0002 removes redundant item metadata before adding
-- uniqueness constraints. Keep the complete imported source rows outside the
-- runtime contract whenever that cleanup will collapse duplicates.
DROP PROCEDURE IF EXISTS duris_archive_duplicate_item_metadata;
DELIMITER //
CREATE PROCEDURE duris_archive_duplicate_item_metadata()
BEGIN
    IF EXISTS (
        SELECT 1
        FROM player_item_extra_descr
        GROUP BY item_id, keyword, LEFT(COALESCE(description, ''), 255)
        HAVING COUNT(*) > 1
        LIMIT 1
    ) THEN
        CREATE TABLE IF NOT EXISTS legacy_import_player_item_extra_descr
            LIKE player_item_extra_descr;
        INSERT IGNORE INTO legacy_import_player_item_extra_descr
        SELECT * FROM player_item_extra_descr;
    END IF;

    IF EXISTS (
        SELECT 1
        FROM player_pet_item_extra_descr
        GROUP BY item_id, keyword, LEFT(COALESCE(description, ''), 255)
        HAVING COUNT(*) > 1
        LIMIT 1
    ) THEN
        CREATE TABLE IF NOT EXISTS legacy_import_player_pet_item_extra_descr
            LIKE player_pet_item_extra_descr;
        INSERT IGNORE INTO legacy_import_player_pet_item_extra_descr
        SELECT * FROM player_pet_item_extra_descr;
    END IF;

    IF EXISTS (
        SELECT 1
        FROM player_item_affects
        GROUP BY item_id, location, modifier
        HAVING COUNT(*) > 1
        LIMIT 1
    ) THEN
        CREATE TABLE IF NOT EXISTS legacy_import_player_item_affects
            LIKE player_item_affects;
        INSERT IGNORE INTO legacy_import_player_item_affects
        SELECT * FROM player_item_affects;
    END IF;

    IF EXISTS (
        SELECT 1
        FROM player_pet_item_affects
        GROUP BY item_id, location, modifier
        HAVING COUNT(*) > 1
        LIMIT 1
    ) THEN
        CREATE TABLE IF NOT EXISTS legacy_import_player_pet_item_affects
            LIKE player_pet_item_affects;
        INSERT IGNORE INTO legacy_import_player_pet_item_affects
        SELECT * FROM player_pet_item_affects;
    END IF;
END//
DELIMITER ;
CALL duris_archive_duplicate_item_metadata();
DROP PROCEDURE duris_archive_duplicate_item_metadata;

-- Launcher-created server_reboots tables predate immutable migration 0004.
-- Preserve the exact legacy rows outside the runtime contract before filling
-- nullable/incompatible fields for the canonical projection.
DROP PROCEDURE IF EXISTS duris_prepare_legacy_server_reboots;
DELIMITER //
CREATE PROCEDURE duris_prepare_legacy_server_reboots()
BEGIN
    DECLARE legacy_shape INT DEFAULT 0;

    SELECT COUNT(*) INTO legacy_shape
    FROM information_schema.columns
    WHERE table_schema = DATABASE()
      AND table_name = 'server_reboots'
      AND column_name = 'id';

    IF legacy_shape = 1 THEN
        CREATE TABLE IF NOT EXISTS legacy_import_server_reboots (
            source_id BIGINT UNSIGNED NOT NULL,
            boot_time BIGINT NOT NULL,
            shutdown_time BIGINT DEFAULT NULL,
            uptime_seconds BIGINT DEFAULT NULL,
            shutdown_type VARCHAR(50) DEFAULT NULL,
            initiated_by VARCHAR(255) DEFAULT NULL,
            reason TEXT DEFAULT NULL,
            created_at DATETIME DEFAULT NULL,
            PRIMARY KEY (source_id)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

        INSERT IGNORE INTO legacy_import_server_reboots (
            source_id,
            boot_time,
            shutdown_time,
            uptime_seconds,
            shutdown_type,
            initiated_by,
            reason,
            created_at
        )
        SELECT
            id,
            boot_time,
            shutdown_time,
            uptime_seconds,
            shutdown_type,
            initiated_by,
            reason,
            created_at
        FROM server_reboots;

        UPDATE server_reboots
        SET uptime_seconds = GREATEST(
                COALESCE(uptime_seconds, COALESCE(shutdown_time, boot_time) - boot_time), 0),
            shutdown_time = GREATEST(COALESCE(shutdown_time, boot_time), 0),
            boot_time = GREATEST(boot_time, 0);
    END IF;
END//
DELIMITER ;
CALL duris_prepare_legacy_server_reboots();
DROP PROCEDURE duris_prepare_legacy_server_reboots;
DROP PROCEDURE IF EXISTS migrate_0004_server_reboots;

DROP PROCEDURE duris_add_column_if_missing;
DROP PROCEDURE duris_drop_column_if_present;
DROP PROCEDURE duris_drop_index_if_present;
DROP PROCEDURE duris_drop_fk_if_present;
