-- DurisMUD MUD-persistence bootstrap schema (authoritative)
-- Fresh database baseline. Existing databases must use migrations/run_migration.sh.

SET FOREIGN_KEY_CHECKS = 0;
SET sql_mode = '';

CREATE TABLE `account_banks` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `account_name` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `racewar` tinyint NOT NULL DEFAULT '0',
  `bank_copper` bigint unsigned DEFAULT '0',
  `bank_silver` bigint unsigned DEFAULT '0',
  `bank_gold` bigint unsigned DEFAULT '0',
  `bank_platinum` bigint unsigned DEFAULT '0',
  `bank_revision` bigint unsigned NOT NULL DEFAULT '0',
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_account_racewar` (`account_name`,`racewar`),
  KEY `idx_account_name` (`account_name`),
  CONSTRAINT `account_banks_ibfk_1` FOREIGN KEY (`account_name`) REFERENCES `accounts` (`account_name`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `account_characters` (
  `id` int NOT NULL AUTO_INCREMENT,
  `account_name` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT 'Account name from flat file system',
  `pid` bigint NOT NULL COMMENT 'Player ID - unique character identifier',
  `char_name` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT 'Character name',
  `created_at` datetime DEFAULT CURRENT_TIMESTAMP COMMENT 'When character was created',
  `deleted_at` datetime DEFAULT NULL COMMENT 'Soft delete - NULL means active',
  `email` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `last_ip` varchar(45) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `login_count` bigint unsigned DEFAULT '0',
  `last_login` timestamp NULL DEFAULT NULL,
  `blocked` tinyint DEFAULT '0',
  `racewar` tinyint DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `idx_char_name_unique` (`char_name`),
  KEY `account_name` (`account_name`),
  KEY `char_name` (`char_name`),
  KEY `deleted_at` (`deleted_at`),
  KEY `account_active` (`account_name`,`deleted_at`),
  KEY `idx_account_racewar` (`account_name`,`racewar`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Maps characters to accounts for web statistics';
CREATE TABLE `account_ips` (
  `id` int NOT NULL AUTO_INCREMENT,
  `account_name` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `hostname` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `ip_address` varchar(45) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `count` bigint unsigned DEFAULT '0',
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_account_ip` (`account_name`,`ip_address`),
  KEY `idx_account_name` (`account_name`),
  KEY `idx_ip_address` (`ip_address`),
  CONSTRAINT `account_ips_ibfk_1` FOREIGN KEY (`account_name`) REFERENCES `accounts` (`account_name`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `account_locker_access` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `locker_id` int unsigned NOT NULL,
  `visitor_account` varchar(50) COLLATE utf8mb4_unicode_ci NOT NULL,
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_locker_visitor` (`locker_id`,`visitor_account`),
  KEY `idx_visitor` (`visitor_account`),
  CONSTRAINT `account_locker_access_ibfk_1` FOREIGN KEY (`locker_id`) REFERENCES `account_lockers` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `account_locker_item_affects` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `item_id` int unsigned NOT NULL,
  `location` tinyint unsigned DEFAULT '0',
  `modifier` int DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `account_locker_item_affects_ibfk_1` FOREIGN KEY (`item_id`) REFERENCES `account_locker_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `account_locker_item_extra_descr` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `item_id` int unsigned NOT NULL,
  `keyword` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL,
  `description` text COLLATE utf8mb4_unicode_ci,
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `fk_account_locker_item_ed` FOREIGN KEY (`item_id`) REFERENCES `account_locker_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `account_locker_items` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `chest_id` int unsigned NOT NULL,
  `vnum` int NOT NULL,
  `container_id` int unsigned DEFAULT NULL,
  `quantity` smallint unsigned DEFAULT '1',
  `weight` int DEFAULT '0',
  `cost` int DEFAULT '0',
  `timer` int DEFAULT '-1',
  `extra_flags` bigint unsigned DEFAULT '0',
  `wear_flags` int DEFAULT NULL,
  `value0` int DEFAULT '0',
  `value1` int DEFAULT '0',
  `value2` int DEFAULT '0',
  `value3` int DEFAULT '0',
  `value4` int DEFAULT '0',
  `value5` int DEFAULT '0',
  `value6` int DEFAULT '0',
  `value7` int DEFAULT '0',
  `name` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `short_descr` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `description` text COLLATE utf8mb4_unicode_ci,
  `action_descr` text COLLATE utf8mb4_unicode_ci,
  `obj_uid` bigint unsigned DEFAULT NULL,
  `item_condition` smallint DEFAULT '100',
  `bitvector1` bigint unsigned DEFAULT NULL,
  `bitvector2` bigint unsigned DEFAULT NULL,
  `bitvector3` bigint unsigned DEFAULT NULL,
  `bitvector4` bigint unsigned DEFAULT NULL,
  `bitvector5` bigint unsigned DEFAULT NULL,
  `item_material` tinyint DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `container_id` (`container_id`),
  KEY `idx_chest_id` (`chest_id`),
  KEY `idx_vnum` (`vnum`),
  KEY `idx_obj_uid` (`obj_uid`),
  CONSTRAINT `account_locker_items_ibfk_1` FOREIGN KEY (`chest_id`) REFERENCES `locker_chests` (`id`) ON DELETE CASCADE,
  CONSTRAINT `account_locker_items_ibfk_2` FOREIGN KEY (`container_id`) REFERENCES `account_locker_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `account_lockers` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `account_name` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `racewar` tinyint DEFAULT '0',
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `account_name` (`account_name`),
  KEY `idx_account_name` (`account_name`),
  CONSTRAINT `account_lockers_ibfk_1` FOREIGN KEY (`account_name`) REFERENCES `accounts` (`account_name`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `accounts` (
  `account_name` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `email` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `last_login` timestamp NULL DEFAULT NULL,
  `password` varchar(128) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `confirmation_code` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `confirmed` tinyint(1) DEFAULT '0',
  `confirmation_sent` tinyint(1) DEFAULT '0',
  `blocked` tinyint(1) DEFAULT '0',
  `last_good_char` timestamp NULL DEFAULT NULL,
  `last_evil_char` timestamp NULL DEFAULT NULL,
  `flags1` bigint unsigned DEFAULT '0',
  `flags2` bigint unsigned DEFAULT '0',
  `flags3` bigint unsigned DEFAULT '0',
  `flags4` bigint unsigned DEFAULT '0',
  `total_donated` decimal(10,2) DEFAULT '0.00',
  PRIMARY KEY (`account_name`),
  KEY `idx_last_login` (`last_login` DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `account_bound_rewards` (
  `id` bigint unsigned NOT NULL AUTO_INCREMENT,
  `account_name` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `reward_vnum` int NOT NULL DEFAULT '36419',
  `template_version` smallint unsigned NOT NULL DEFAULT '0',
  `template_json` longtext CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
  `display_name` varchar(512) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `granted_by` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `expires_at` datetime DEFAULT NULL,
  `remaining_pwipes` int unsigned DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_account_bound_rewards_account` (`account_name`),
  KEY `idx_account_bound_rewards_vnum` (`reward_vnum`),
  KEY `idx_account_bound_rewards_expires` (`expires_at`),
  CONSTRAINT `account_bound_rewards_ibfk_1` FOREIGN KEY (`account_name`) REFERENCES `accounts` (`account_name`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `account_bound_reward_pwipe_state` (
  `id` tinyint unsigned NOT NULL,
  `last_processed_at` datetime DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO `account_bound_reward_pwipe_state` (`id`,`last_processed_at`) VALUES (1,NULL);
CREATE TABLE `account_bound_reward_summons` (
  `grant_id` bigint unsigned NOT NULL,
  `pid` int unsigned NOT NULL,
  `last_summoned_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `recovery_ready` tinyint(1) NOT NULL DEFAULT '0',
  PRIMARY KEY (`grant_id`,`pid`),
  KEY `idx_account_bound_reward_summons_pid` (`pid`),
  CONSTRAINT `account_bound_reward_summons_grant_fk` FOREIGN KEY (`grant_id`) REFERENCES `account_bound_rewards` (`id`) ON DELETE CASCADE,
  CONSTRAINT `account_bound_reward_summons_pid_fk` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `alliances` (
  `id` int NOT NULL AUTO_INCREMENT,
  `created_at` datetime DEFAULT NULL,
  `forging_assoc_id` int NOT NULL,
  `joining_assoc_id` int NOT NULL,
  `tribute_owed` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `artifact_bind` (
  `vnum` int NOT NULL,
  `owner_pid` int DEFAULT NULL,
  `timer` int DEFAULT NULL,
  PRIMARY KEY (`vnum`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `artifacts` (
  `vnum` int NOT NULL,
  `owned` varchar(10) COLLATE utf8mb4_unicode_ci DEFAULT 'N',
  `location` int DEFAULT NULL,
  `timer` datetime DEFAULT NULL,
  `type` int DEFAULT NULL,
  `lastUpdate` datetime DEFAULT NULL,
  `locType` int NOT NULL DEFAULT '1',
  PRIMARY KEY (`vnum`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `artifacts_mortal` (
  `vnum` int NOT NULL,
  `owned` varchar(10) COLLATE utf8mb4_unicode_ci DEFAULT 'N',
  `location` int DEFAULT NULL,
  `timer` datetime DEFAULT NULL,
  `type` int DEFAULT NULL,
  `lastUpdate` datetime DEFAULT NULL,
  `locType` int NOT NULL DEFAULT '1',
  PRIMARY KEY (`vnum`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `associations` (
  `id` int NOT NULL,
  `name` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `prestige` int NOT NULL DEFAULT '0',
  `active` tinyint(1) NOT NULL DEFAULT '1',
  `wood` int NOT NULL DEFAULT '0',
  `stone` int NOT NULL DEFAULT '0',
  `construction_points` int NOT NULL DEFAULT '0',
  `over_max` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `auction_bid_history` (
  `id` int NOT NULL AUTO_INCREMENT,
  `date` int NOT NULL DEFAULT '0',
  `auction_id` int NOT NULL DEFAULT '0',
  `bidder_pid` int NOT NULL DEFAULT '0',
  `bidder_name` varchar(32) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `bid_amount` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `auction_id` (`auction_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `auction_item_pickups` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `pid` int unsigned NOT NULL DEFAULT '0',
  `obj_blob_str` blob NOT NULL,
  `retrieved` tinyint(1) NOT NULL DEFAULT '0',
  `quantity` int NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`),
  KEY `pid` (`pid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `auction_money_pickups` (
  `pid` int unsigned NOT NULL DEFAULT '0',
  `money` int unsigned NOT NULL DEFAULT '0',
	`claim_revision` bigint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`pid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `auctions` (
  `id` int NOT NULL AUTO_INCREMENT,
  `seller_pid` int unsigned NOT NULL DEFAULT '0',
  `seller_name` varchar(32) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `status` enum('OPEN','CLOSED','REMOVED') COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'OPEN',
  `winning_bidder_pid` int NOT NULL DEFAULT '0',
  `winning_bidder_name` varchar(32) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `cur_price` int unsigned NOT NULL DEFAULT '0',
  `buy_price` int NOT NULL DEFAULT '0',
  `obj_short` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `obj_vnum` int NOT NULL DEFAULT '0',
  `obj_blob_str` blob NOT NULL,
  `id_keywords` varchar(1024) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `obj_info_text` mediumtext COLLATE utf8mb4_unicode_ci,
  `quantity` int NOT NULL DEFAULT '1',
  `start_time` timestamp NULL DEFAULT NULL,
  `end_time` timestamp NULL DEFAULT NULL,
	`auction_revision` bigint unsigned NOT NULL DEFAULT '0',
	`custody_state` tinyint unsigned NOT NULL DEFAULT '0',
	`listing_operation_id` binary(16) DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `seller_pid` (`seller_pid`),
	KEY `status` (`status`),
	UNIQUE KEY `uq_auction_listing_operation` (`listing_operation_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `boons` (
  `id` int NOT NULL AUTO_INCREMENT,
  `time` int NOT NULL DEFAULT '0',
  `duration` int NOT NULL DEFAULT '0',
  `racewar` int NOT NULL DEFAULT '0',
  `type` int NOT NULL DEFAULT '0',
  `opt` int NOT NULL DEFAULT '0',
  `criteria` decimal(10,2) NOT NULL DEFAULT '0.00',
  `criteria2` decimal(10,2) NOT NULL DEFAULT '0.00',
  `bonus` decimal(10,2) NOT NULL DEFAULT '0.00',
  `bonus2` decimal(10,2) NOT NULL DEFAULT '0.00',
  `random` int NOT NULL DEFAULT '0',
  `author` varchar(20) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `active` int NOT NULL DEFAULT '0',
  `pid` int NOT NULL DEFAULT '0',
  `rpt` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `boons_progress` (
  `id` int NOT NULL AUTO_INCREMENT,
  `boonid` int NOT NULL DEFAULT '0',
  `pid` int NOT NULL DEFAULT '0',
  `counter` decimal(10,2) NOT NULL DEFAULT '0.00',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `boons_shop` (
  `id` int NOT NULL AUTO_INCREMENT,
  `pid` int NOT NULL DEFAULT '0',
  `points` int NOT NULL DEFAULT '0',
  `stats` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `pid` (`pid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `categories` (
  `id` int NOT NULL AUTO_INCREMENT,
  `name` varchar(255) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `desc` varchar(255) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `changes` (
  `id` int NOT NULL AUTO_INCREMENT,
  `history_id` int DEFAULT NULL,
  `history_text` mediumtext COLLATE utf8mb4_unicode_ci,
  `history_title` varchar(255) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `history_category_id` int DEFAULT NULL,
  `new_text` mediumtext COLLATE utf8mb4_unicode_ci,
  `new_title` varchar(255) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `new_category_id` int DEFAULT NULL,
  `timestamp` datetime DEFAULT NULL,
  `action` varchar(255) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `ip_number` varchar(255) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `classes` (
  `id` int unsigned NOT NULL,
  `name` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL,
  `ansi_name` varchar(128) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `short_name` varchar(8) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `menu_char` char(1) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `corpse_item_affects` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `item_id` int unsigned NOT NULL,
  `location` tinyint unsigned DEFAULT '0',
  `modifier` int DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `corpse_item_affects_ibfk_1` FOREIGN KEY (`item_id`) REFERENCES `corpse_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `corpse_item_extra_descr` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `item_id` int unsigned NOT NULL,
  `keyword` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL,
  `description` text COLLATE utf8mb4_unicode_ci,
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `fk_corpse_item_ed` FOREIGN KEY (`item_id`) REFERENCES `corpse_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `corpse_items` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `corpse_id` int NOT NULL,
  `vnum` int NOT NULL,
  `item_type` tinyint DEFAULT '0',
  `container_id` int unsigned DEFAULT NULL,
  `quantity` smallint unsigned DEFAULT '1',
  `weight` int DEFAULT '0',
  `cost` int DEFAULT '0',
  `timer` int DEFAULT '-1',
  `extra_flags` bigint unsigned DEFAULT '0',
  `wear_flags` int DEFAULT NULL,
  `value0` int DEFAULT '0',
  `value1` int DEFAULT '0',
  `value2` int DEFAULT '0',
  `value3` int DEFAULT '0',
  `value4` int DEFAULT '0',
  `value5` int DEFAULT '0',
  `value6` int DEFAULT '0',
  `value7` int DEFAULT '0',
  `name` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `short_descr` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `description` text COLLATE utf8mb4_unicode_ci,
  `action_descr` text COLLATE utf8mb4_unicode_ci,
  `obj_uid` bigint unsigned DEFAULT NULL,
  `item_condition` smallint DEFAULT '100',
  `item_material` tinyint DEFAULT NULL,
  `bitvector1` bigint unsigned DEFAULT NULL,
  `bitvector2` bigint unsigned DEFAULT NULL,
  `bitvector3` bigint unsigned DEFAULT NULL,
  `bitvector4` bigint unsigned DEFAULT NULL,
  `bitvector5` bigint unsigned DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `container_id` (`container_id`),
  KEY `idx_corpse_id` (`corpse_id`),
  KEY `idx_vnum` (`vnum`),
  KEY `idx_obj_uid` (`obj_uid`),
  CONSTRAINT `corpse_items_ibfk_1` FOREIGN KEY (`corpse_id`) REFERENCES `corpses` (`id`) ON DELETE CASCADE,
  CONSTRAINT `corpse_items_ibfk_2` FOREIGN KEY (`container_id`) REFERENCES `corpse_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `corpses` (
  `id` int NOT NULL AUTO_INCREMENT,
  `player_name` varchar(50) COLLATE utf8mb4_unicode_ci NOT NULL,
  `save_id` bigint NOT NULL,
  `corpse_revision` bigint unsigned NOT NULL DEFAULT '1',
  `room_vnum` int DEFAULT '0',
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `short_descr` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `description` text COLLATE utf8mb4_unicode_ci,
  `name` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `weight` int DEFAULT NULL,
  `value0` int DEFAULT NULL,
  `value1` int DEFAULT NULL,
  `value2` int DEFAULT NULL,
  `value3` int DEFAULT NULL,
  `value4` int DEFAULT NULL,
  `value5` int DEFAULT NULL,
  `value7` int DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_player_saveid` (`player_name`,`save_id`),
  KEY `idx_player_name` (`player_name`),
  KEY `idx_corpse_owner_save` (`value3`,`save_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `corpse_catalog_state` (
  `state_id` tinyint unsigned NOT NULL,
  `catalog_revision` bigint unsigned NOT NULL DEFAULT '1',
  `updated_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6) ON UPDATE CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`state_id`),
  CONSTRAINT `chk_corpse_catalog_revision` CHECK (`catalog_revision` > 0),
  CONSTRAINT `chk_corpse_catalog_singleton` CHECK (`state_id` = 1)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO `corpse_catalog_state` (`state_id`,`catalog_revision`) VALUES (1,1);
CREATE TABLE `ctf_data` (
  `id` int NOT NULL AUTO_INCREMENT,
  `time` timestamp NULL DEFAULT NULL,
  `pid` int NOT NULL DEFAULT '0',
  `type` int NOT NULL DEFAULT '0',
  `flagtype` int NOT NULL DEFAULT '0',
  `racewar` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `epic_bonus` (
  `pid` int NOT NULL,
  `type` int NOT NULL DEFAULT '0',
  `time` datetime DEFAULT NULL,
  UNIQUE KEY `pid` (`pid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `epic_gain` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `pid` bigint NOT NULL DEFAULT '0',
  `time` datetime NOT NULL,
  `type` int NOT NULL DEFAULT '0',
  `type_id` int NOT NULL DEFAULT '0',
  `epics` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `pid_index` (`pid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `eq_drop` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `vnum` int unsigned NOT NULL DEFAULT '0',
  `pid_looter` bigint unsigned NOT NULL DEFAULT '0',
  `room_id` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `Index_2` (`vnum`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `frag_leaderboard` (
  `id` int NOT NULL AUTO_INCREMENT,
  `pid` bigint NOT NULL COMMENT 'Player ID - links to character',
  `account_name` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT 'Account name for account-level stats',
  `char_name` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT 'Character name for display',
  `total_frags` int NOT NULL DEFAULT '0' COMMENT 'Total frags (stored as int, divide by 100 for display)',
  `racewar` int NOT NULL COMMENT 'Racewar side (1=good, 2=evil, 3=undead, 4=illithid)',
  `race` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT NULL COMMENT 'Character race',
  `class` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT NULL COMMENT 'Character class',
  `level` int DEFAULT NULL COMMENT 'Character level',
  `deleted_at` datetime DEFAULT NULL COMMENT 'Soft delete - NULL means active',
  `last_updated` datetime DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last time record was updated',
  PRIMARY KEY (`id`),
  UNIQUE KEY `pid` (`pid`),
  KEY `char_name` (`char_name`),
  KEY `account_name` (`account_name`),
  KEY `total_frags_active` (`deleted_at`,`total_frags`),
  KEY `racewar_leaderboard` (`deleted_at`,`racewar`,`total_frags`),
  KEY `race_leaderboard` (`deleted_at`,`race`,`total_frags`),
  KEY `class_leaderboard` (`deleted_at`,`class`,`total_frags`),
  KEY `level_range` (`deleted_at`,`level`,`total_frags`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `guild_members` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `guild_id` int unsigned NOT NULL,
  `player_name` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `player_pid` int unsigned DEFAULT NULL,
  `bits` int unsigned NOT NULL DEFAULT '0',
  `debt` int unsigned NOT NULL DEFAULT '0',
  `online_status` tinyint NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_guild_members_name` (`guild_id`,`player_name`),
  KEY `idx_guild_members_name` (`player_name`),
  CONSTRAINT `fk_guild_members_guild` FOREIGN KEY (`guild_id`) REFERENCES `guilds` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `guild_ranks` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `guild_id` int unsigned NOT NULL,
  `rank_index` tinyint NOT NULL,
  `title` varchar(100) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_guild_ranks_index` (`guild_id`,`rank_index`),
  CONSTRAINT `fk_guild_ranks_guild` FOREIGN KEY (`guild_id`) REFERENCES `guilds` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `guild_transactions` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `soc_id` int unsigned NOT NULL DEFAULT '0',
  `date` int NOT NULL DEFAULT '0',
  `transaction_info` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  PRIMARY KEY (`id`),
  KEY `soc_id` (`soc_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `guildhall_rooms` (
  `id` int NOT NULL AUTO_INCREMENT,
  `guildhall_id` int NOT NULL DEFAULT '0',
  `vnum` int NOT NULL DEFAULT '0',
  `type` int NOT NULL DEFAULT '0',
  `value0` int unsigned NOT NULL DEFAULT '0',
  `value1` int unsigned NOT NULL DEFAULT '0',
  `value2` int unsigned NOT NULL DEFAULT '0',
  `value3` int unsigned NOT NULL DEFAULT '0',
  `value4` int unsigned NOT NULL DEFAULT '0',
  `value5` int unsigned NOT NULL DEFAULT '0',
  `value6` int unsigned NOT NULL DEFAULT '0',
  `value7` int unsigned NOT NULL DEFAULT '0',
  `exit0` int NOT NULL DEFAULT '0',
  `exit1` int NOT NULL DEFAULT '0',
  `exit2` int NOT NULL DEFAULT '0',
  `exit3` int NOT NULL DEFAULT '0',
  `exit4` int NOT NULL DEFAULT '0',
  `exit5` int NOT NULL DEFAULT '0',
  `exit6` int NOT NULL DEFAULT '0',
  `exit7` int NOT NULL DEFAULT '0',
  `exit8` int NOT NULL DEFAULT '0',
  `exit9` int NOT NULL,
  `name` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL,
  PRIMARY KEY (`id`),
  KEY `vnum` (`vnum`),
  KEY `guildhall_id` (`guildhall_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `guildhalls` (
  `id` int NOT NULL AUTO_INCREMENT,
  `assoc_id` int NOT NULL DEFAULT '0',
  `type` int NOT NULL DEFAULT '0',
  `outside_vnum` int NOT NULL DEFAULT '0',
  `racewar` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `assoc_id` (`assoc_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `guilds` (
  `id` int unsigned NOT NULL,
  `name` varchar(100) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `racewar` int unsigned NOT NULL DEFAULT '0',
  `bits` int unsigned NOT NULL DEFAULT '0',
  `prestige` bigint unsigned NOT NULL DEFAULT '0',
  `construction` bigint unsigned NOT NULL DEFAULT '0',
  `outcome_revision` bigint unsigned NOT NULL DEFAULT '0',
  `platinum` int unsigned NOT NULL DEFAULT '0',
  `gold` int unsigned NOT NULL DEFAULT '0',
  `silver` int unsigned NOT NULL DEFAULT '0',
  `copper` int unsigned NOT NULL DEFAULT '0',
  `frags` bigint NOT NULL DEFAULT '0',
  `top_frags` bigint NOT NULL DEFAULT '0',
  `topfragger` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `ip_info` (
  `pid` bigint NOT NULL DEFAULT '0',
  `last_ip` varchar(50) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'none',
  `last_connect` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `last_disconnect` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `racewar_side` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`pid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `items` (
  `vnum` int unsigned NOT NULL DEFAULT '0',
  `short_desc` varchar(100) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `obj_stat` mediumtext COLLATE utf8mb4_unicode_ci NOT NULL,
  `num_sold` int NOT NULL DEFAULT '0',
  `avg_sell_price` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`vnum`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `kingdom_land` (
  `id` int NOT NULL AUTO_INCREMENT,
  `kingdom_id` int NOT NULL,
  `start_vnum` int DEFAULT '0',
  `end_vnum` int DEFAULT '0',
  `type` char(1) COLLATE utf8mb4_unicode_ci DEFAULT 'r',
  PRIMARY KEY (`id`),
  KEY `idx_kingdom_id` (`kingdom_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `level_cap` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `most_frags` float NOT NULL,
  `racewar_leader` int NOT NULL,
  `level` int NOT NULL,
  `next_update` datetime NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO `level_cap` (`id`, `most_frags`, `racewar_leader`, `level`, `next_update`)
VALUES (1, 0, 2, 56, NOW());
CREATE TABLE `locker_access` (
  `owner` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL,
  `visitor` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL,
  PRIMARY KEY (`owner`,`visitor`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `locker_activity_log` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `locker_id` int unsigned NOT NULL,
  `account_name` varchar(50) COLLATE utf8mb4_unicode_ci NOT NULL,
  `char_name` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL,
  `action_type` int NOT NULL DEFAULT '1',
  `chest_keyword` varchar(64) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `details` varchar(255) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `logged_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_locker_id` (`locker_id`),
  KEY `idx_logged_at` (`logged_at`),
  CONSTRAINT `locker_activity_log_ibfk_1` FOREIGN KEY (`locker_id`) REFERENCES `account_lockers` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `locker_chests` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `locker_id` int unsigned NOT NULL,
  `keyword` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL,
  `keyword_hash` varchar(64) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `is_public` tinyint(1) DEFAULT '0',
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_locker_keyword` (`locker_id`,`keyword`),
  KEY `idx_locker_id` (`locker_id`),
  CONSTRAINT `locker_chests_ibfk_1` FOREIGN KEY (`locker_id`) REFERENCES `account_lockers` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `locker_item_affects` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `item_id` int unsigned NOT NULL,
  `location` tinyint unsigned DEFAULT '0',
  `modifier` int DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `locker_item_affects_ibfk_1` FOREIGN KEY (`item_id`) REFERENCES `locker_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `locker_item_extra_descr` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `item_id` int unsigned NOT NULL,
  `keyword` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL,
  `description` text COLLATE utf8mb4_unicode_ci,
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `fk_locker_item_ed` FOREIGN KEY (`item_id`) REFERENCES `locker_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `locker_items` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `locker_id` int unsigned NOT NULL,
  `chest_id` int unsigned DEFAULT NULL,
  `vnum` int NOT NULL,
  `container_id` int unsigned DEFAULT NULL,
  `quantity` smallint unsigned DEFAULT '1',
  `weight` int DEFAULT '0',
  `cost` int DEFAULT '0',
  `timer` int DEFAULT '-1',
  `extra_flags` bigint unsigned DEFAULT '0',
  `wear_flags` int DEFAULT NULL,
  `item_type` tinyint DEFAULT NULL,
  `value0` int DEFAULT '0',
  `value1` int DEFAULT '0',
  `value2` int DEFAULT '0',
  `value3` int DEFAULT '0',
  `value4` int DEFAULT '0',
  `value5` int DEFAULT '0',
  `value6` int DEFAULT '0',
  `value7` int DEFAULT '0',
  `name` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `short_descr` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `description` text COLLATE utf8mb4_unicode_ci,
  `action_descr` text COLLATE utf8mb4_unicode_ci,
  `obj_uid` bigint unsigned DEFAULT NULL,
  `item_condition` smallint DEFAULT '100',
  `bitvector1` bigint unsigned DEFAULT NULL,
  `bitvector2` bigint unsigned DEFAULT NULL,
  `bitvector3` bigint unsigned DEFAULT NULL,
  `bitvector4` bigint unsigned DEFAULT NULL,
  `bitvector5` bigint unsigned DEFAULT NULL,
  `item_material` tinyint DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `container_id` (`container_id`),
  KEY `idx_locker_id` (`locker_id`),
  KEY `idx_vnum` (`vnum`),
  KEY `idx_obj_uid` (`obj_uid`),
  CONSTRAINT `locker_items_ibfk_1` FOREIGN KEY (`locker_id`) REFERENCES `lockers` (`id`) ON DELETE CASCADE,
  CONSTRAINT `locker_items_ibfk_2` FOREIGN KEY (`container_id`) REFERENCES `locker_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `locker_kickouts` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `locker_id` int unsigned NOT NULL,
  `account_name` varchar(50) COLLATE utf8mb4_unicode_ci NOT NULL,
  `fail_count` tinyint unsigned DEFAULT '0',
  `kicked_until` timestamp NULL DEFAULT NULL,
  `last_fail` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_locker_account` (`locker_id`,`account_name`),
  CONSTRAINT `locker_kickouts_ibfk_1` FOREIGN KEY (`locker_id`) REFERENCES `account_lockers` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `locker_session_state` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `locker_id` int unsigned NOT NULL,
  `account_name` varchar(50) COLLATE utf8mb4_unicode_ci NOT NULL,
  `chest_id` int unsigned NOT NULL,
  `opened_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_session` (`locker_id`,`account_name`,`chest_id`),
  KEY `chest_id` (`chest_id`),
  CONSTRAINT `locker_session_state_ibfk_1` FOREIGN KEY (`locker_id`) REFERENCES `account_lockers` (`id`) ON DELETE CASCADE,
  CONSTRAINT `locker_session_state_ibfk_2` FOREIGN KEY (`chest_id`) REFERENCES `locker_chests` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `lockers` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `locker_name` varchar(100) COLLATE utf8mb4_unicode_ci NOT NULL,
  `owner_pid` int DEFAULT NULL,
  `owner_assoc_id` int DEFAULT NULL,
  `racewar` tinyint DEFAULT '0',
  `race` tinyint DEFAULT '0',
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `locker_name` (`locker_name`),
  KEY `idx_owner_pid` (`owner_pid`),
  KEY `idx_owner_assoc_id` (`owner_assoc_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `log_entries` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `date` datetime NOT NULL,
  `kind` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `player_name` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `pid` int NOT NULL DEFAULT '0',
  `ip_address` varchar(15) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `room_vnum` int NOT NULL DEFAULT '0',
  `zone_number` int NOT NULL DEFAULT '0',
  `message` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  PRIMARY KEY (`id`),
  KEY `date_index` (`date`),
  KEY `kind_index` (`kind`),
  KEY `name_index` (`player_name`),
  KEY `pid_index` (`pid`),
  KEY `ip_address_index` (`ip_address`),
  KEY `room_vnum_index` (`room_vnum`),
  KEY `zone_id_index` (`zone_number`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `mud_info` (
  `name` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL,
  `content` mediumtext COLLATE utf8mb4_unicode_ci NOT NULL,
  PRIMARY KEY (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `multiplay_whitelist` (
  `id` int NOT NULL AUTO_INCREMENT,
  `pattern` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL,
  `admin` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL,
  `description` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL,
  `created_on` date DEFAULT NULL,
  `player` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `nexus_stones` (
  `id` int NOT NULL AUTO_INCREMENT,
  `name` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `room_vnum` int NOT NULL DEFAULT '0',
  `align` int NOT NULL DEFAULT '0',
  `stat_affect` int NOT NULL DEFAULT '-1',
  `affect_amount` int NOT NULL DEFAULT '0',
  `last_touched_at` timestamp NULL DEFAULT NULL,
  `bonus` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `offline_messages` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `date` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
  `pid` int NOT NULL DEFAULT '0',
  `message_id` binary(16) DEFAULT NULL,
  `message` mediumtext COLLATE utf8mb4_unicode_ci NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_offline_message_identity` (`pid`,`message_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `offline_message_receipts` (
  `pid` int NOT NULL,
  `message_id` binary(16) NOT NULL,
  `message` mediumtext COLLATE utf8mb4_unicode_ci NOT NULL,
  `status` tinyint unsigned NOT NULL DEFAULT '0',
  `attempt_count` smallint unsigned NOT NULL DEFAULT '0',
  `last_attempt_at` timestamp(6) NULL DEFAULT NULL,
  `created_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  `delivered_at` timestamp(6) NULL DEFAULT NULL,
  PRIMARY KEY (`pid`,`message_id`),
  KEY `idx_offline_message_receipt_pending` (`pid`,`status`,`last_attempt_at`,`created_at`,`message_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `outposts` (
  `id` int NOT NULL,
  `owner_id` int NOT NULL DEFAULT '0',
  `level` int NOT NULL DEFAULT '1',
  `walls` int NOT NULL DEFAULT '0',
  `archers` int NOT NULL DEFAULT '0',
  `resources` int NOT NULL DEFAULT '0',
  `applied_resources` int NOT NULL DEFAULT '100000',
  `hitpoints` int NOT NULL DEFAULT '0',
  `territory` int NOT NULL DEFAULT '0',
  `portal_room` int NOT NULL DEFAULT '0',
  `golems` int NOT NULL DEFAULT '0',
  `meurtriere` int NOT NULL DEFAULT '0',
  `scouts` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `pages` (
  `id` int NOT NULL AUTO_INCREMENT,
  `title` varchar(255) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `text` mediumtext COLLATE utf8mb4_unicode_ci,
  `last_update` datetime DEFAULT NULL,
  `last_update_by` varchar(255) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `category_id` int DEFAULT NULL,
  `ip_number` varchar(255) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `persistence_item_events` (
  `id` bigint unsigned NOT NULL AUTO_INCREMENT,
  `ts_usec` bigint unsigned NOT NULL,
  `event_type` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `item_uid` bigint unsigned NOT NULL DEFAULT '0',
  `vnum` int NOT NULL DEFAULT '-1',
  `item` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `actor` varchar(128) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `actor_id` int NOT NULL DEFAULT '-1',
  `source` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `target` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `note` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `dedupe_key` varchar(64) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_item_dedupe` (`dedupe_key`),
  KEY `idx_item_uid_ts` (`item_uid`,`ts_usec`,`id`),
  KEY `idx_event_type_created` (`event_type`,`created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `persistence_scalar_events` (
  `id` bigint unsigned NOT NULL AUTO_INCREMENT,
  `event_type` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `event_key` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `boot_time` int NOT NULL DEFAULT '0',
  `touched_at` int NOT NULL DEFAULT '0',
  `zone_number` int NOT NULL DEFAULT '0',
  `toucher_pid` int NOT NULL DEFAULT '0',
  `group_size` int NOT NULL DEFAULT '0',
  `epic_value` int NOT NULL DEFAULT '0',
  `alignment_delta` int NOT NULL DEFAULT '0',
  `dedupe_key` varchar(64) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_scalar_dedupe` (`dedupe_key`),
  KEY `idx_scalar_event_key` (`event_type`,`event_key`),
  KEY `idx_scalar_zone_time` (`zone_number`,`touched_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `ping` (
  `ID` bigint NOT NULL AUTO_INCREMENT,
  `TIMESTAMP` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
  `URL` varchar(100) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `IP` varchar(100) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `SEQ` bigint NOT NULL DEFAULT '0',
  `TIME` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `pkill_event` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `stamp` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
  `room_vnum` int NOT NULL DEFAULT '0',
  `room_name` mediumtext COLLATE utf8mb4_unicode_ci NOT NULL,
  `tweeted` tinyint(1) NOT NULL DEFAULT '0',
  `like_count` int NOT NULL DEFAULT '0',
  `comment_count` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `idx_event_likes` (`like_count`),
  KEY `idx_event_comments` (`comment_count`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `pkill_info` (
  `id` bigint unsigned NOT NULL AUTO_INCREMENT,
  `event_id` int unsigned NOT NULL DEFAULT '0',
  `pid` bigint NOT NULL DEFAULT '0',
  `level` int NOT NULL DEFAULT '0',
  `pk_type` mediumtext COLLATE utf8mb4_unicode_ci NOT NULL,
  `equip` mediumtext COLLATE utf8mb4_unicode_ci NOT NULL,
  `log` mediumtext COLLATE utf8mb4_unicode_ci,
  `inroom` int NOT NULL DEFAULT '0',
  `leader` int DEFAULT NULL,
  `player_description` varchar(255) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `part_of_name` (`event_id`),
  KEY `unique_index` (`pid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `player_affects` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `pid` int unsigned NOT NULL,
  `type` smallint NOT NULL,
  `duration` int DEFAULT '0',
  `flags` smallint unsigned DEFAULT '0',
  `modifier` int DEFAULT '0',
  `location` tinyint unsigned DEFAULT '0',
  `level` smallint unsigned DEFAULT '0',
  `bitvector1` bigint unsigned DEFAULT '0',
  `bitvector2` bigint unsigned DEFAULT '0',
  `bitvector3` bigint unsigned DEFAULT '0',
  `bitvector4` bigint unsigned DEFAULT '0',
  `bitvector5` bigint unsigned DEFAULT '0',
  `custom_msg_char` text COLLATE utf8mb4_unicode_ci,
  `custom_msg_room` text COLLATE utf8mb4_unicode_ci,
  PRIMARY KEY (`id`),
  KEY `idx_pid` (`pid`),
  CONSTRAINT `fk_player_affects` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `player_data` (
  `pid` int unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL,
  `account_name` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `short_descr` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `long_descr` text COLLATE utf8mb4_unicode_ci,
  `description` text COLLATE utf8mb4_unicode_ci,
  `title` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `m_class` int unsigned DEFAULT '0',
  `secondary_class` int unsigned DEFAULT '0',
  `spec` tinyint unsigned DEFAULT '0',
  `race` tinyint unsigned DEFAULT '0',
  `racewar` tinyint unsigned DEFAULT '0',
  `level` tinyint unsigned DEFAULT '1',
  `sex` tinyint unsigned DEFAULT '0',
  `weight` smallint unsigned DEFAULT '0',
  `height` smallint unsigned DEFAULT '0',
  `size` tinyint DEFAULT '0',
  `hometown` int DEFAULT '0',
  `birthplace` int DEFAULT '0',
  `orig_birthplace` int DEFAULT '0',
  `last_room` int DEFAULT '0',
  `birth_time` timestamp NULL DEFAULT NULL,
  `played_time` int DEFAULT '0',
  `last_save` timestamp NULL DEFAULT NULL,
  `save_revision` bigint unsigned NOT NULL DEFAULT '0',
  `perm_aging` smallint DEFAULT '0',
  `base_str` tinyint DEFAULT '0',
  `base_dex` tinyint DEFAULT '0',
  `base_agi` tinyint DEFAULT '0',
  `base_con` tinyint DEFAULT '0',
  `base_pow` tinyint DEFAULT '0',
  `base_int` tinyint DEFAULT '0',
  `base_wis` tinyint DEFAULT '0',
  `base_cha` tinyint DEFAULT '0',
  `base_kar` tinyint DEFAULT '0',
  `base_luk` tinyint DEFAULT '0',
  `mana` int DEFAULT '0',
  `base_mana` int DEFAULT '0',
  `hit_diff` int DEFAULT '0',
  `base_hit` int DEFAULT '0',
  `vitality` int DEFAULT '0',
  `base_vitality` int DEFAULT '0',
  `spells_memmed_extra` tinyint DEFAULT '0',
  `copper` bigint DEFAULT '0',
  `silver` bigint DEFAULT '0',
  `gold` bigint DEFAULT '0',
  `platinum` bigint DEFAULT '0',
  `wallet_revision` bigint unsigned NOT NULL DEFAULT '0',
  `bank_copper` bigint DEFAULT '0',
  `bank_silver` bigint DEFAULT '0',
  `bank_gold` bigint DEFAULT '0',
  `bank_platinum` bigint DEFAULT '0',
  `exp` bigint DEFAULT '0',
  `epics` bigint DEFAULT '0',
  `epic_revision` bigint unsigned NOT NULL DEFAULT '0',
  `epic_skill_points` bigint DEFAULT '0',
  `skillpoints` int DEFAULT '0',
  `spell_bind_used` bigint DEFAULT '0',
  `act` bigint unsigned DEFAULT '0',
  `act2` bigint unsigned DEFAULT '0',
  `act3` bigint unsigned DEFAULT '0',
  `vote` bigint unsigned DEFAULT '0',
  `alignment` int DEFAULT '0',
  `prestige` smallint DEFAULT '0',
  `assoc_id` smallint unsigned DEFAULT '0',
  `guild_status` int unsigned DEFAULT '0',
  `time_left_guild` timestamp NULL DEFAULT NULL,
  `nb_left_guild` tinyint DEFAULT '0',
  `time_unspecced` timestamp NULL DEFAULT NULL,
  `frags` bigint DEFAULT '0',
  `oldfrags` bigint DEFAULT '0',
  `frag_revision` bigint unsigned NOT NULL DEFAULT '0',
  `numb_deaths` bigint unsigned DEFAULT '0',
  `killed_by` varchar(64) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `condition_0` tinyint DEFAULT '0',
  `condition_1` tinyint DEFAULT '0',
  `condition_2` tinyint DEFAULT '0',
  `condition_3` tinyint DEFAULT '0',
  `condition_4` tinyint DEFAULT '0',
  `poof_in` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `poof_out` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `poof_in_sound` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `poof_out_sound` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `echo_toggle` tinyint unsigned DEFAULT '0',
  `prompt` smallint unsigned DEFAULT '0',
  `wiz_invis` bigint DEFAULT '0',
  `law_flags` bigint unsigned DEFAULT '0',
  `wimpy` smallint DEFAULT '0',
  `aggressive` smallint DEFAULT '-1',
  `highest_level` tinyint unsigned DEFAULT '0',
  `screen_length` tinyint unsigned DEFAULT '24',
  `quest_active` int DEFAULT '0',
  `quest_mob_vnum` int DEFAULT '0',
  `quest_type` int DEFAULT '0',
  `quest_accomplished` int DEFAULT '0',
  `quest_started` int DEFAULT '0',
  `quest_zone_number` int DEFAULT '0',
  `quest_giver` int DEFAULT '0',
  `quest_level` int DEFAULT '0',
  `quest_receiver` int DEFAULT '0',
  `quest_shares_left` int DEFAULT '0',
  `quest_kill_how_many` int DEFAULT '0',
  `quest_kill_original` int DEFAULT '0',
  `quest_map_room` int DEFAULT '0',
  `quest_map_bought` int DEFAULT '0',
  `last_ip` bigint unsigned DEFAULT '0',
  `active` tinyint(1) NOT NULL DEFAULT '1',
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`pid`),
  UNIQUE KEY `idx_player_name_unique` (`name`),
  KEY `idx_name` (`name`),
  KEY `idx_account_name` (`account_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `player_forged_items` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `pid` int unsigned NOT NULL,
  `forge_index` smallint unsigned NOT NULL,
  `item_vnum` int DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_pid_forge` (`pid`,`forge_index`),
  KEY `idx_pid` (`pid`),
  CONSTRAINT `fk_player_forged_items` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `player_granted_cmds` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `pid` int unsigned NOT NULL,
  `cmd_num` int NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_pid_cmd` (`pid`,`cmd_num`),
  KEY `idx_pid` (`pid`),
  CONSTRAINT `fk_player_granted_cmds` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `player_intros` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `pid` int unsigned NOT NULL,
  `intro_index` tinyint unsigned NOT NULL,
  `intro_pid` int DEFAULT '0',
  `intro_time` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_pid_intro` (`pid`,`intro_index`),
  KEY `idx_pid` (`pid`),
  CONSTRAINT `fk_player_intros` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `player_item_affects` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `item_id` int unsigned NOT NULL,
  `location` tinyint unsigned DEFAULT '0',
  `modifier` int DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `fk_player_item_affects` FOREIGN KEY (`item_id`) REFERENCES `player_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `player_item_extra_descr` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `item_id` int unsigned NOT NULL,
  `keyword` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL,
  `description` text COLLATE utf8mb4_unicode_ci,
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `fk_player_item_ed` FOREIGN KEY (`item_id`) REFERENCES `player_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `player_items` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `pid` int unsigned NOT NULL,
  `vnum` int NOT NULL,
  `equip_slot` tinyint DEFAULT '0',
  `container_id` int unsigned DEFAULT NULL,
  `quantity` smallint unsigned DEFAULT '1',
  `weight` int DEFAULT '0',
  `cost` int DEFAULT '0',
  `timer` int DEFAULT '-1',
  `extra_flags` bigint unsigned DEFAULT '0',
  `wear_flags` int DEFAULT NULL,
  `item_type` tinyint DEFAULT NULL,
  `value0` int DEFAULT '0',
  `value1` int DEFAULT '0',
  `value2` int DEFAULT '0',
  `value3` int DEFAULT '0',
  `value4` int DEFAULT '0',
  `value5` int DEFAULT '0',
  `value6` int DEFAULT '0',
  `value7` int DEFAULT '0',
  `name` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `short_descr` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `description` text COLLATE utf8mb4_unicode_ci,
  `action_descr` text COLLATE utf8mb4_unicode_ci,
  `bitvector1` bigint unsigned DEFAULT NULL,
  `bitvector2` bigint unsigned DEFAULT NULL,
  `bitvector3` bigint unsigned DEFAULT NULL,
  `bitvector4` bigint unsigned DEFAULT NULL,
  `bitvector5` bigint unsigned DEFAULT NULL,
  `obj_uid` bigint unsigned DEFAULT NULL,
  `item_condition` smallint DEFAULT '100',
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `item_material` tinyint DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_pid` (`pid`),
  KEY `idx_container_id` (`container_id`),
  KEY `idx_obj_uid` (`obj_uid`),
  CONSTRAINT `fk_player_items` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE,
  CONSTRAINT `fk_player_items_container` FOREIGN KEY (`container_id`) REFERENCES `player_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `player_languages` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `pid` int unsigned NOT NULL,
  `tongue_id` tinyint unsigned NOT NULL,
  `proficiency` tinyint unsigned DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_pid_tongue` (`pid`,`tongue_id`),
  KEY `idx_pid` (`pid`),
  CONSTRAINT `fk_player_languages` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `player_pet_item_affects` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `item_id` int unsigned NOT NULL,
  `location` tinyint unsigned DEFAULT '0',
  `modifier` int DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `fk_pet_item_affects` FOREIGN KEY (`item_id`) REFERENCES `player_pet_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `player_pet_item_extra_descr` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `item_id` int unsigned NOT NULL,
  `keyword` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL,
  `description` text COLLATE utf8mb4_unicode_ci,
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `fk_pet_item_ed` FOREIGN KEY (`item_id`) REFERENCES `player_pet_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `player_pet_items` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `pet_id` int unsigned NOT NULL,
  `vnum` int NOT NULL,
  `equip_slot` tinyint DEFAULT '0',
  `container_id` int unsigned DEFAULT NULL,
  `weight` int DEFAULT '0',
  `cost` int DEFAULT '0',
  `timer` int DEFAULT '-1',
  `extra_flags` bigint unsigned DEFAULT '0',
  `wear_flags` int DEFAULT NULL,
  `value0` int DEFAULT '0',
  `value1` int DEFAULT '0',
  `value2` int DEFAULT '0',
  `value3` int DEFAULT '0',
  `value4` int DEFAULT '0',
  `value5` int DEFAULT '0',
  `value6` int DEFAULT '0',
  `value7` int DEFAULT '0',
  `name` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `short_descr` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `description` text COLLATE utf8mb4_unicode_ci,
  `action_descr` text COLLATE utf8mb4_unicode_ci,
  `obj_uid` bigint unsigned DEFAULT NULL,
  `item_condition` smallint DEFAULT '100',
  `item_material` tinyint DEFAULT NULL,
  `item_type` tinyint DEFAULT NULL,
  `bitvector1` bigint unsigned DEFAULT NULL,
  `bitvector2` bigint unsigned DEFAULT NULL,
  `bitvector3` bigint unsigned DEFAULT NULL,
  `bitvector4` bigint unsigned DEFAULT NULL,
  `bitvector5` bigint unsigned DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_pet_id` (`pet_id`),
  KEY `idx_container_id` (`container_id`),
  KEY `idx_obj_uid` (`obj_uid`),
  CONSTRAINT `fk_pet_items_container` FOREIGN KEY (`container_id`) REFERENCES `player_pet_items` (`id`) ON DELETE CASCADE,
  CONSTRAINT `fk_pet_items_pet` FOREIGN KEY (`pet_id`) REFERENCES `player_pets` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `player_pets` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `owner_pid` int unsigned NOT NULL,
  `mob_vnum` int NOT NULL,
  `pet_order` tinyint DEFAULT '0',
  `hit` int DEFAULT '0',
  `max_hit` int DEFAULT '0',
  `mana` int DEFAULT '0',
  `max_mana` int DEFAULT '0',
  `vitality` int DEFAULT '0',
  `max_vitality` int DEFAULT '0',
  `charm_duration` int DEFAULT '-1',
  `room_vnum` int DEFAULT '0',
  `saved_at` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_owner_pid` (`owner_pid`),
  CONSTRAINT `fk_player_pets_owner` FOREIGN KEY (`owner_pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `player_recipes` (
  `id` int NOT NULL AUTO_INCREMENT,
  `pid` int NOT NULL,
  `recipe_vnum` int NOT NULL,
  `learned_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_pid_recipe` (`pid`,`recipe_vnum`),
  KEY `idx_pid` (`pid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `player_shapechanges` (
  `id` int NOT NULL AUTO_INCREMENT,
  `pid` int NOT NULL,
  `mob_vnum` int NOT NULL,
  `times_researched` int DEFAULT '0',
  `last_researched` timestamp NULL DEFAULT NULL,
  `last_shapechanged` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_pid_mob` (`pid`,`mob_vnum`),
  KEY `idx_pid` (`pid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `player_skills` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `pid` int unsigned NOT NULL,
  `skill_id` smallint unsigned NOT NULL,
  `learned` tinyint unsigned DEFAULT '0',
  `taught` tinyint unsigned DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_pid_skill` (`pid`,`skill_id`),
  KEY `idx_pid` (`pid`),
  CONSTRAINT `fk_player_skills` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `player_spellbooks` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `pid` int unsigned NOT NULL,
  `mob_vnum` int NOT NULL,
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_pid_mob` (`pid`,`mob_vnum`),
  KEY `idx_pid` (`pid`),
  CONSTRAINT `fk_player_spellbooks` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `player_timers` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `pid` int unsigned NOT NULL,
  `timer_id` tinyint unsigned NOT NULL,
  `timer_value` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_pid_timer` (`pid`,`timer_id`),
  KEY `idx_pid` (`pid`),
  CONSTRAINT `fk_player_timers` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `player_undead_slots` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `pid` int unsigned NOT NULL,
  `circle` tinyint unsigned NOT NULL,
  `slots` tinyint DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_pid_circle` (`pid`,`circle`),
  KEY `idx_pid` (`pid`),
  CONSTRAINT `fk_player_undead_slots` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `player_witnesses` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `pid` int unsigned NOT NULL,
  `crime` tinyint unsigned DEFAULT '0',
  `room_vnum` int DEFAULT '0',
  `attacker_name` varchar(64) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `victim_name` varchar(64) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `witness_time` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_pid` (`pid`),
  CONSTRAINT `fk_player_witnesses` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `poll_options` (
  `id` int NOT NULL AUTO_INCREMENT,
  `poll_id` int NOT NULL,
  `option_num` int NOT NULL,
  `option_text` varchar(256) COLLATE utf8mb4_unicode_ci NOT NULL,
  PRIMARY KEY (`id`),
  KEY `poll_id` (`poll_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `poll_votes` (
  `id` int NOT NULL AUTO_INCREMENT,
  `poll_id` int NOT NULL,
  `account_name` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL,
  `option_id` int NOT NULL,
  `voted_at` int NOT NULL DEFAULT '0',
  `char_name` varchar(32) COLLATE utf8mb4_unicode_ci NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `unique_vote` (`poll_id`,`account_name`,`option_id`),
  KEY `poll_id` (`poll_id`),
  KEY `account_name` (`account_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `polls` (
  `id` int NOT NULL AUTO_INCREMENT,
  `question` varchar(512) COLLATE utf8mb4_unicode_ci NOT NULL,
  `created_by` varchar(32) COLLATE utf8mb4_unicode_ci NOT NULL,
  `created_at` int NOT NULL DEFAULT '0',
  `expires_at` int NOT NULL DEFAULT '0',
  `is_active` tinyint(1) NOT NULL DEFAULT '1',
  `multi_select` tinyint(1) NOT NULL DEFAULT '0',
  `max_choices` int NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `prepstatement_duris_sql` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `description` text COLLATE utf8mb4_unicode_ci,
  `sql_code` text COLLATE utf8mb4_unicode_ci,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `private_chest_log` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `locker_id` int unsigned NOT NULL,
  `chest_id` int unsigned DEFAULT NULL,
  `char_name` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL,
  `action_type` int NOT NULL DEFAULT '1',
  `item_short` varchar(256) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `logged_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_locker_id` (`locker_id`),
  KEY `idx_logged_at` (`logged_at`),
  CONSTRAINT `private_chest_log_ibfk_1` FOREIGN KEY (`locker_id`) REFERENCES `lockers` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `private_chests` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `locker_id` int unsigned NOT NULL,
  `chest_name` varchar(32) COLLATE utf8mb4_unicode_ci NOT NULL,
  `password_hash` varchar(64) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `is_public` tinyint(1) DEFAULT '0',
  `sort_config` text COLLATE utf8mb4_unicode_ci,
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_locker_chest` (`locker_id`,`chest_name`),
  KEY `idx_locker_id` (`locker_id`),
  CONSTRAINT `private_chests_ibfk_1` FOREIGN KEY (`locker_id`) REFERENCES `lockers` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `progress` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `pid` bigint NOT NULL DEFAULT '0',
  `var_type` enum('FRAGS','EXP') COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'FRAGS',
  `stamp` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
  `delta` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `pid_index` (`pid`),
  KEY `index_enum` (`var_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `quest_trophy` (
  `id` int NOT NULL AUTO_INCREMENT,
  `mob_vnum` int NOT NULL DEFAULT '0',
  `pid` bigint NOT NULL DEFAULT '0',
  `type` int NOT NULL DEFAULT '0',
  `reward_value` int NOT NULL DEFAULT '0',
  `timestamp` datetime NOT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_mob_vnum` (`mob_vnum`),
  KEY `idx_pid` (`pid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `races` (
  `id` int unsigned NOT NULL,
  `name` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL,
  `short_name` varchar(32) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `ansi_name` varchar(128) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `abbrev` varchar(4) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `racewar` tinyint DEFAULT '0',
  `playable` tinyint DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `racewar_stat_mods` (
  `racewar` int NOT NULL DEFAULT '0',
  `Str` int NOT NULL DEFAULT '0',
  `Dex` int NOT NULL DEFAULT '0',
  `Agi` int NOT NULL DEFAULT '0',
  `Con` int NOT NULL DEFAULT '0',
  `Pow` int NOT NULL DEFAULT '0',
  `Intl` int NOT NULL DEFAULT '0',
  `Wis` int NOT NULL DEFAULT '0',
  `Cha` int NOT NULL DEFAULT '0',
  `Kar` int NOT NULL DEFAULT '0',
  `Luc` int NOT NULL DEFAULT '0'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `saved_item_affects` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `item_id` int unsigned NOT NULL,
  `location` tinyint unsigned DEFAULT '0',
  `modifier` int DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `saved_item_affects_ibfk_1` FOREIGN KEY (`item_id`) REFERENCES `saved_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `saved_item_extra_descr` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `item_id` int unsigned NOT NULL,
  `keyword` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL,
  `description` text COLLATE utf8mb4_unicode_ci,
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `fk_saved_item_ed` FOREIGN KEY (`item_id`) REFERENCES `saved_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `saved_items` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `item_key` varchar(100) COLLATE utf8mb4_unicode_ci NOT NULL,
  `room_vnum` int DEFAULT '0',
  `vnum` int NOT NULL,
  `container_id` int unsigned DEFAULT NULL,
  `quantity` smallint unsigned DEFAULT '1',
  `weight` int DEFAULT '0',
  `cost` int DEFAULT '0',
  `timer` int DEFAULT '-1',
  `extra_flags` bigint unsigned DEFAULT '0',
  `wear_flags` int DEFAULT NULL,
  `item_type` tinyint DEFAULT NULL,
  `value0` int DEFAULT '0',
  `value1` int DEFAULT '0',
  `value2` int DEFAULT '0',
  `value3` int DEFAULT '0',
  `value4` int DEFAULT '0',
  `value5` int DEFAULT '0',
  `value6` int DEFAULT '0',
  `value7` int DEFAULT '0',
  `name` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `short_descr` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `description` text COLLATE utf8mb4_unicode_ci,
  `action_descr` text COLLATE utf8mb4_unicode_ci,
  `obj_uid` bigint unsigned DEFAULT NULL,
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `item_material` tinyint DEFAULT NULL,
  `bitvector1` bigint unsigned DEFAULT NULL,
  `bitvector2` bigint unsigned DEFAULT NULL,
  `bitvector3` bigint unsigned DEFAULT NULL,
  `bitvector4` bigint unsigned DEFAULT NULL,
  `bitvector5` bigint unsigned DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `item_key` (`item_key`),
  KEY `container_id` (`container_id`),
  KEY `idx_room_vnum` (`room_vnum`),
  KEY `idx_vnum` (`vnum`),
  CONSTRAINT `saved_items_ibfk_1` FOREIGN KEY (`container_id`) REFERENCES `saved_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `ship_armor` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `ship_id` int unsigned NOT NULL,
  `side` tinyint NOT NULL,
  `armor` int DEFAULT '0',
  `internal` int DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_ship_armor` (`ship_id`,`side`),
  CONSTRAINT `fk_ship_armor_ship` FOREIGN KEY (`ship_id`) REFERENCES `ships` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `ship_cargo_market_mods` (
  `type` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `port_id` int NOT NULL DEFAULT '-1',
  `cargo_type` int NOT NULL DEFAULT '-1',
  `modifier` float NOT NULL DEFAULT '0',
  KEY `type_port_id_cargo_type` (`type`,`port_id`,`cargo_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `ship_cargo_prices` (
  `type` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `port_id` int NOT NULL DEFAULT '-1',
  `cargo_type` int NOT NULL DEFAULT '-1',
  `price` int NOT NULL DEFAULT '0',
  KEY `type_port_id_cargo_type` (`type`,`port_id`,`cargo_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `ship_crew` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `ship_id` int unsigned NOT NULL,
  `crew_index` int DEFAULT '0',
  `sail_skill` int DEFAULT '0',
  `guns_skill` int DEFAULT '0',
  `rpar_skill` int DEFAULT '0',
  `sail_chief` int DEFAULT '0',
  `guns_chief` int DEFAULT '0',
  `rpar_chief` int DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_ship_crew` (`ship_id`),
  CONSTRAINT `fk_ship_crew_ship` FOREIGN KEY (`ship_id`) REFERENCES `ships` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `ship_slots` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `ship_id` int unsigned NOT NULL,
  `slot_index` tinyint NOT NULL,
  `slot_type` int NOT NULL DEFAULT '0',
  `item_index` int NOT NULL DEFAULT '0',
  `position` int NOT NULL DEFAULT '0',
  `timer` int NOT NULL DEFAULT '0',
  `val0` int NOT NULL DEFAULT '0',
  `val1` int NOT NULL DEFAULT '0',
  `val2` int NOT NULL DEFAULT '0',
  `val3` int NOT NULL DEFAULT '0',
  `val4` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_ship_slots_index` (`ship_id`,`slot_index`),
  CONSTRAINT `fk_ship_slots_ship` FOREIGN KEY (`ship_id`) REFERENCES `ships` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `ships` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `owner_name` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL,
  `ship_name` varchar(128) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `ship_class` tinyint unsigned DEFAULT '0',
  `frags` int DEFAULT '0',
  `anchor_room` int DEFAULT '0',
  `time_played` int DEFAULT '0',
  `mainsail` int DEFAULT '0',
  `race` tinyint DEFAULT '0',
  `money` int DEFAULT '0',
  `flags` bigint unsigned DEFAULT '0',
  `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `owner_name` (`owner_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `shop_trophy` (
  `id` int NOT NULL AUTO_INCREMENT,
  `item` int NOT NULL DEFAULT '0',
  `value` int NOT NULL DEFAULT '0',
  `seller` int NOT NULL DEFAULT '0',
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `shopkeeper_affects` (
  `id` int NOT NULL AUTO_INCREMENT,
  `shopkeeper_id` int NOT NULL,
  `type` int DEFAULT '0',
  `duration` int DEFAULT '0',
  `modifier` int DEFAULT '0',
  `location` int DEFAULT '0',
  `bitvector1` bigint unsigned DEFAULT '0',
  `bitvector2` bigint unsigned DEFAULT '0',
  `bitvector3` bigint unsigned DEFAULT '0',
  `bitvector4` bigint unsigned DEFAULT '0',
  `bitvector5` bigint unsigned DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `idx_shopkeeper_id` (`shopkeeper_id`),
  CONSTRAINT `shopkeeper_affects_ibfk_1` FOREIGN KEY (`shopkeeper_id`) REFERENCES `shopkeepers` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `shopkeeper_item_affects` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `item_id` int unsigned NOT NULL,
  `location` tinyint unsigned DEFAULT '0',
  `modifier` int DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `shopkeeper_item_affects_ibfk_1` FOREIGN KEY (`item_id`) REFERENCES `shopkeeper_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `shopkeeper_item_extra_descr` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `item_id` int unsigned NOT NULL,
  `keyword` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL,
  `description` text COLLATE utf8mb4_unicode_ci,
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `fk_shopkeeper_item_ed` FOREIGN KEY (`item_id`) REFERENCES `shopkeeper_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `shopkeeper_items` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `shopkeeper_id` int NOT NULL,
  `vnum` int NOT NULL,
  `equip_slot` tinyint DEFAULT '0',
  `container_id` int unsigned DEFAULT NULL,
  `quantity` smallint unsigned DEFAULT '1',
  `weight` int DEFAULT '0',
  `cost` int DEFAULT '0',
  `timer` int DEFAULT '-1',
  `extra_flags` bigint unsigned DEFAULT '0',
  `wear_flags` int DEFAULT NULL,
  `item_type` tinyint DEFAULT NULL,
  `value0` int DEFAULT '0',
  `value1` int DEFAULT '0',
  `value2` int DEFAULT '0',
  `value3` int DEFAULT '0',
  `value4` int DEFAULT '0',
  `value5` int DEFAULT '0',
  `value6` int DEFAULT '0',
  `value7` int DEFAULT '0',
  `name` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `short_descr` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `description` text COLLATE utf8mb4_unicode_ci,
  `action_descr` text COLLATE utf8mb4_unicode_ci,
  `obj_uid` bigint unsigned DEFAULT NULL,
  `item_material` tinyint DEFAULT NULL,
  `bitvector1` bigint unsigned DEFAULT NULL,
  `bitvector2` bigint unsigned DEFAULT NULL,
  `bitvector3` bigint unsigned DEFAULT NULL,
  `bitvector4` bigint unsigned DEFAULT NULL,
  `bitvector5` bigint unsigned DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `container_id` (`container_id`),
  KEY `idx_shopkeeper_id` (`shopkeeper_id`),
  KEY `idx_vnum` (`vnum`),
  CONSTRAINT `shopkeeper_items_ibfk_1` FOREIGN KEY (`shopkeeper_id`) REFERENCES `shopkeepers` (`id`) ON DELETE CASCADE,
  CONSTRAINT `shopkeeper_items_ibfk_2` FOREIGN KEY (`container_id`) REFERENCES `shopkeeper_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `shopkeepers` (
  `id` int NOT NULL AUTO_INCREMENT,
  `shop_id` int NOT NULL,
  `mob_vnum` int DEFAULT '0',
  `room_vnum` int DEFAULT '0',
  `save_time` timestamp NULL DEFAULT NULL,
  `updated_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `shop_id` (`shop_id`),
  KEY `idx_shop_id` (`shop_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `siege_item_affects` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `item_id` int unsigned NOT NULL,
  `location` tinyint unsigned DEFAULT '0',
  `modifier` int DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `siege_item_affects_ibfk_1` FOREIGN KEY (`item_id`) REFERENCES `siege_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `siege_item_extra_descr` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `item_id` int unsigned NOT NULL,
  `keyword` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL,
  `description` text COLLATE utf8mb4_unicode_ci,
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `fk_siege_item_ed` FOREIGN KEY (`item_id`) REFERENCES `siege_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `siege_items` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `room_vnum` int NOT NULL,
  `vnum` int NOT NULL,
  `container_id` int unsigned DEFAULT NULL,
  `quantity` smallint unsigned DEFAULT '1',
  `weight` int DEFAULT '0',
  `cost` int DEFAULT '0',
  `timer` int DEFAULT '-1',
  `extra_flags` bigint unsigned DEFAULT '0',
  `wear_flags` int DEFAULT NULL,
  `value0` int DEFAULT '0',
  `value1` int DEFAULT '0',
  `value2` int DEFAULT '0',
  `value3` int DEFAULT '0',
  `value4` int DEFAULT '0',
  `value5` int DEFAULT '0',
  `value6` int DEFAULT '0',
  `value7` int DEFAULT '0',
  `name` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `short_descr` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `description` text COLLATE utf8mb4_unicode_ci,
  `action_descr` text COLLATE utf8mb4_unicode_ci,
  `obj_uid` bigint unsigned DEFAULT NULL,
  `updated_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `item_material` tinyint DEFAULT NULL,
  `item_type` tinyint DEFAULT NULL,
  `bitvector1` bigint unsigned DEFAULT NULL,
  `bitvector2` bigint unsigned DEFAULT NULL,
  `bitvector3` bigint unsigned DEFAULT NULL,
  `bitvector4` bigint unsigned DEFAULT NULL,
  `bitvector5` bigint unsigned DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `container_id` (`container_id`),
  KEY `idx_room_vnum` (`room_vnum`),
  KEY `idx_vnum` (`vnum`),
  CONSTRAINT `siege_items_ibfk_1` FOREIGN KEY (`container_id`) REFERENCES `siege_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `statistics` (
  `id` int NOT NULL AUTO_INCREMENT,
  `date` int NOT NULL DEFAULT '0',
  `goods_count` int NOT NULL DEFAULT '0',
  `evils_count` int NOT NULL DEFAULT '0',
  `illithids_count` int NOT NULL DEFAULT '0',
  `undeads_count` int NOT NULL DEFAULT '0',
  `gods_count` int NOT NULL DEFAULT '0',
  `in_guildhall_count` int NOT NULL DEFAULT '0',
  `sum_goods_levels` int NOT NULL DEFAULT '0',
  `sum_evils_levels` int NOT NULL DEFAULT '0',
  `sum_illithids_levels` int NOT NULL DEFAULT '0',
  `sum_undeads_levels` int NOT NULL DEFAULT '0',
  `unique_ips_count` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `timers` (
  `name` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `date` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `towns` (
  `id` int NOT NULL AUTO_INCREMENT,
  `zone_filename` varchar(100) COLLATE utf8mb4_unicode_ci NOT NULL,
  `resources` int DEFAULT '0',
  `defense` int DEFAULT '0',
  `offense` int DEFAULT '0',
  `deploy_guard` tinyint DEFAULT '0',
  `guard_vnum` int DEFAULT '0',
  `guard_max` int DEFAULT '0',
  `guard_load_room` int DEFAULT '0',
  `deploy_cavalry` tinyint DEFAULT '0',
  `cavalry_vnum` int DEFAULT '0',
  `cavalry_max` int DEFAULT '0',
  `cavalry_load_room` int DEFAULT '0',
  `deploy_portals` tinyint DEFAULT '0',
  `portal_vnum` int DEFAULT '0',
  `portal_load_room` int DEFAULT '0',
  `updated_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_zone_filename` (`zone_filename`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `world_quest_accomplished` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `pid` varchar(45) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `quest_giver` int unsigned NOT NULL DEFAULT '0',
  `player_name` varchar(45) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `player_level` int unsigned NOT NULL DEFAULT '0',
  `quest_target` int NOT NULL DEFAULT '0',
  `reward_vnum` int NOT NULL DEFAULT '0',
  `reward_desc` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `zone_touches` (
  `id` int NOT NULL AUTO_INCREMENT,
  `zone_number` int DEFAULT NULL,
  `toucher_pid` int DEFAULT NULL,
  `group_size` int DEFAULT NULL,
  `epic_value` int DEFAULT NULL,
  `alignment_delta` int DEFAULT NULL,
  `boot_time` timestamp NULL DEFAULT NULL,
  `touched_at` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `zone_number_index` (`zone_number`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `zone_trophy` (
  `pid` bigint NOT NULL DEFAULT '0',
  `zone_number` int NOT NULL DEFAULT '0',
  `exp` int NOT NULL DEFAULT '0',
  PRIMARY KEY (`pid`,`zone_number`),
  KEY `pid_index` (`pid`),
  KEY `zone_number` (`zone_number`),
  KEY `exp_index` (`exp`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `zones` (
  `id` int NOT NULL AUTO_INCREMENT,
  `number` int DEFAULT NULL,
  `name` varchar(100) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `epic_type` int NOT NULL DEFAULT '0',
  `frequency_mod` float NOT NULL DEFAULT '1',
  `zone_freq_mod` float NOT NULL DEFAULT '1',
  `epic_level` int NOT NULL DEFAULT '0',
  `task_zone` tinyint(1) NOT NULL DEFAULT '0',
  `quest_zone` tinyint(1) NOT NULL DEFAULT '0',
  `trophy_zone` tinyint(1) NOT NULL DEFAULT '1',
  `suggested_group_size` int NOT NULL DEFAULT '1',
  `epic_payout` int NOT NULL DEFAULT '0',
  `difficulty` int NOT NULL DEFAULT '0',
  `randoms_zone` tinyint(1) NOT NULL DEFAULT '1',
  `alignment` int NOT NULL DEFAULT '0',
  `reset_perc` int DEFAULT '0',
  `stonecount` int NOT NULL DEFAULT '1',
  `last_touch` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `number_index` (`number`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `mud_schema_migrations` (
  `migration_name` varchar(128) NOT NULL,
  `applied_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`migration_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `critical_operation_inbox` (
  `operation_id` binary(16) NOT NULL,
  `command_hash` binary(32) NOT NULL,
  `keys_hash` binary(32) NOT NULL,
  `command_type` smallint unsigned NOT NULL,
  `schema_version` int unsigned NOT NULL,
  `payload_version` smallint unsigned NOT NULL,
  `status` tinyint unsigned NOT NULL,
  `result_code` int unsigned NOT NULL DEFAULT '0',
  `failure_stage` smallint unsigned NOT NULL DEFAULT '0',
  `durable_revision` bigint unsigned NOT NULL DEFAULT '0',
  `result_payload` varbinary(4096) NOT NULL,
  `created_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  `committed_at` timestamp(6) NULL DEFAULT NULL,
  PRIMARY KEY (`operation_id`),
  KEY `idx_critical_inbox_status_created` (`status`,`created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `critical_test_state` (
  `entity_type` tinyint unsigned NOT NULL,
  `entity_id` bigint unsigned NOT NULL,
  `value` bigint NOT NULL DEFAULT '0',
  `revision` bigint unsigned NOT NULL DEFAULT '0',
  `updated_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6) ON UPDATE CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`entity_type`,`entity_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `critical_outbox` (
  `outbox_id` bigint unsigned NOT NULL AUTO_INCREMENT,
  `operation_id` binary(16) NOT NULL,
  `event_index` smallint unsigned NOT NULL,
  `destination` smallint unsigned NOT NULL,
  `event_type` smallint unsigned NOT NULL,
  `payload_version` smallint unsigned NOT NULL,
  `payload` blob NOT NULL,
  `status` tinyint unsigned NOT NULL DEFAULT '0',
  `attempt_count` smallint unsigned NOT NULL DEFAULT '0',
  `next_attempt_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  `created_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  `delivered_at` timestamp(6) NULL DEFAULT NULL,
  `dead_lettered_at` timestamp(6) NULL DEFAULT NULL,
  `last_error_code` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`outbox_id`),
  UNIQUE KEY `uq_critical_outbox_operation_event` (`operation_id`,`event_index`),
  KEY `idx_critical_outbox_claim` (`status`,`next_attempt_at`,`outbox_id`),
  KEY `idx_critical_outbox_age` (`status`,`created_at`),
  CONSTRAINT `critical_outbox_operation_fk` FOREIGN KEY (`operation_id`) REFERENCES `critical_operation_inbox` (`operation_id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `critical_outbox_delivery_dedupe` (
  `consumer_id` smallint unsigned NOT NULL,
  `outbox_id` bigint unsigned NOT NULL,
  `delivered_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`consumer_id`,`outbox_id`),
  CONSTRAINT `critical_outbox_delivery_fk` FOREIGN KEY (`outbox_id`) REFERENCES `critical_outbox` (`outbox_id`) ON DELETE CASCADE ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `epic_balance_baseline` (
  `pid` int unsigned NOT NULL,
  `opening_balance` bigint NOT NULL,
  `opening_revision` bigint unsigned NOT NULL DEFAULT '0',
  `captured_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`pid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `epic_ledger` (
  `operation_id` binary(16) NOT NULL,
  `pid` int unsigned NOT NULL,
  `delta` bigint NOT NULL,
  `balance_after` bigint NOT NULL,
  `epic_revision` bigint unsigned NOT NULL,
  `reason_type` smallint unsigned NOT NULL,
  `reason_id` bigint NOT NULL DEFAULT '0',
  `source_site` smallint unsigned NOT NULL,
  `created_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`operation_id`),
  UNIQUE KEY `uq_epic_ledger_pid_revision` (`pid`,`epic_revision`),
  KEY `idx_epic_ledger_pid_created` (`pid`,`created_at`),
  KEY `idx_epic_created_operation` (`created_at`,`operation_id`),
  KEY `idx_epic_ledger_reason_created` (`reason_type`,`created_at`),
  CONSTRAINT `epic_ledger_operation_fk` FOREIGN KEY (`operation_id`) REFERENCES `critical_operation_inbox` (`operation_id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `currency_wallet_baseline` (
  `pid` int unsigned NOT NULL,
  `opening_copper` bigint NOT NULL,
  `opening_silver` bigint NOT NULL,
  `opening_gold` bigint NOT NULL,
  `opening_platinum` bigint NOT NULL,
  `opening_revision` bigint unsigned NOT NULL DEFAULT '0',
  `captured_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`pid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `currency_bank_baseline` (
  `bank_id` int unsigned NOT NULL,
  `opening_copper` bigint unsigned NOT NULL,
  `opening_silver` bigint unsigned NOT NULL,
  `opening_gold` bigint unsigned NOT NULL,
  `opening_platinum` bigint unsigned NOT NULL,
  `opening_revision` bigint unsigned NOT NULL DEFAULT '0',
  `captured_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`bank_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `currency_ledger` (
  `operation_id` binary(16) NOT NULL,
  `pid` int unsigned NOT NULL,
  `bank_id` int unsigned NOT NULL,
  `wallet_delta_copper` bigint NOT NULL,
  `wallet_delta_silver` bigint NOT NULL,
  `wallet_delta_gold` bigint NOT NULL,
  `wallet_delta_platinum` bigint NOT NULL,
  `bank_delta_copper` bigint NOT NULL,
  `bank_delta_silver` bigint NOT NULL,
  `bank_delta_gold` bigint NOT NULL,
  `bank_delta_platinum` bigint NOT NULL,
  `wallet_after_copper` bigint NOT NULL,
  `wallet_after_silver` bigint NOT NULL,
  `wallet_after_gold` bigint NOT NULL,
  `wallet_after_platinum` bigint NOT NULL,
  `bank_after_copper` bigint unsigned NOT NULL,
  `bank_after_silver` bigint unsigned NOT NULL,
  `bank_after_gold` bigint unsigned NOT NULL,
  `bank_after_platinum` bigint unsigned NOT NULL,
  `wallet_revision` bigint unsigned NOT NULL,
  `bank_revision` bigint unsigned NOT NULL,
  `reason_type` smallint unsigned NOT NULL,
  `reason_id` bigint NOT NULL DEFAULT '0',
  `source_site` smallint unsigned NOT NULL,
  `created_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`operation_id`),
  UNIQUE KEY `uq_currency_wallet_revision` (`pid`,`wallet_revision`),
  UNIQUE KEY `uq_currency_bank_revision` (`bank_id`,`bank_revision`),
  KEY `idx_currency_pid_created` (`pid`,`created_at`),
  KEY `idx_currency_created_operation` (`created_at`,`operation_id`),
  KEY `idx_currency_bank_created` (`bank_id`,`created_at`),
  KEY `idx_currency_reason_created` (`reason_type`,`created_at`),
  CONSTRAINT `currency_ledger_operation_fk` FOREIGN KEY (`operation_id`) REFERENCES `critical_operation_inbox` (`operation_id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `item_uid_allocator` (
  `allocator_id` tinyint unsigned NOT NULL,
  `next_uid` bigint unsigned NOT NULL,
  `updated_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6) ON UPDATE CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`allocator_id`),
  CONSTRAINT `chk_item_uid_allocator_singleton` CHECK ((`allocator_id` = 1)),
  CONSTRAINT `chk_item_uid_allocator_nonzero` CHECK ((`next_uid` > 0))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO `item_uid_allocator` (`allocator_id`,`next_uid`) VALUES (1,1);
CREATE TABLE `item_owner_revision` (
  `owner_type` tinyint unsigned NOT NULL, `owner_id` bigint unsigned NOT NULL,
  `owner_context_id` bigint unsigned NOT NULL DEFAULT '0', `revision` bigint unsigned NOT NULL DEFAULT '0',
  `updated_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6) ON UPDATE CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`owner_type`,`owner_id`,`owner_context_id`), KEY `idx_item_owner_revision_updated` (`updated_at`),
  CONSTRAINT `chk_item_owner_revision_type` CHECK ((`owner_type` between 1 and 10))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `item_current_owner` (
  `item_uid` bigint unsigned NOT NULL, `root_item_uid` bigint unsigned NOT NULL,
  `parent_item_uid` bigint unsigned DEFAULT NULL, `owner_type` tinyint unsigned NOT NULL,
  `owner_id` bigint unsigned NOT NULL, `owner_context_id` bigint unsigned NOT NULL DEFAULT '0',
  `item_revision` bigint unsigned NOT NULL DEFAULT '0', `vnum` int NOT NULL DEFAULT '0',
  `state` tinyint unsigned NOT NULL DEFAULT '1',
  `coin_payload` mediumblob DEFAULT NULL,
  `updated_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6) ON UPDATE CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`item_uid`), KEY `idx_item_current_root_uid` (`root_item_uid`,`item_uid`),
  KEY `idx_item_current_owner` (`owner_type`,`owner_id`,`owner_context_id`,`item_uid`),
  KEY `idx_item_current_parent` (`parent_item_uid`),
  CONSTRAINT `chk_item_current_uid_nonzero` CHECK (((`item_uid` > 0) and (`root_item_uid` > 0))),
  CONSTRAINT `chk_item_current_owner_type` CHECK ((`owner_type` between 1 and 10)),
  CONSTRAINT `chk_item_current_state` CHECK ((`state` between 1 and 3)),
  CONSTRAINT `item_current_parent_fk` FOREIGN KEY (`parent_item_uid`) REFERENCES `item_current_owner` (`item_uid`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `item_ownership_baseline` (
  `item_uid` bigint unsigned NOT NULL, `root_item_uid` bigint unsigned NOT NULL,
  `parent_item_uid` bigint unsigned DEFAULT NULL, `owner_type` tinyint unsigned NOT NULL,
  `owner_id` bigint unsigned NOT NULL, `owner_context_id` bigint unsigned NOT NULL DEFAULT '0',
  `opening_item_revision` bigint unsigned NOT NULL DEFAULT '0', `vnum` int NOT NULL DEFAULT '0',
  `source_table` varchar(32) NOT NULL, `source_row_id` bigint unsigned NOT NULL,
  `captured_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6), PRIMARY KEY (`item_uid`),
  UNIQUE KEY `uq_item_baseline_source` (`source_table`,`source_row_id`),
  KEY `idx_item_baseline_owner` (`owner_type`,`owner_id`,`owner_context_id`),
  CONSTRAINT `chk_item_baseline_owner_type` CHECK ((`owner_type` between 1 and 10))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `item_ownership_quarantine` (
  `quarantine_id` bigint unsigned NOT NULL AUTO_INCREMENT, `item_uid` bigint unsigned NOT NULL,
  `source_table` varchar(32) NOT NULL, `source_row_id` bigint unsigned NOT NULL,
  `conflict_code` smallint unsigned NOT NULL, `evidence` varchar(255) NOT NULL,
  `detected_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6), `repaired_at` timestamp(6) NULL DEFAULT NULL,
  PRIMARY KEY (`quarantine_id`), UNIQUE KEY `uq_item_quarantine_evidence` (`item_uid`,`source_table`,`source_row_id`,`conflict_code`),
  KEY `idx_item_quarantine_open` (`repaired_at`,`item_uid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `item_ownership_ledger` (
  `operation_id` binary(16) NOT NULL, `event_index` smallint unsigned NOT NULL,
  `item_uid` bigint unsigned NOT NULL, `root_item_uid` bigint unsigned NOT NULL,
  `parent_item_uid` bigint unsigned DEFAULT NULL, `from_owner_type` tinyint unsigned NOT NULL,
  `from_owner_id` bigint unsigned NOT NULL, `from_owner_context_id` bigint unsigned NOT NULL,
  `to_owner_type` tinyint unsigned NOT NULL, `to_owner_id` bigint unsigned NOT NULL,
  `to_owner_context_id` bigint unsigned NOT NULL, `item_revision` bigint unsigned NOT NULL,
  `from_owner_revision` bigint unsigned NOT NULL, `to_owner_revision` bigint unsigned NOT NULL,
  `reason_type` smallint unsigned NOT NULL, `reason_id` bigint NOT NULL DEFAULT '0',
  `source_site` smallint unsigned NOT NULL, `created_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`operation_id`,`event_index`), UNIQUE KEY `uq_item_ledger_item_revision` (`item_uid`,`item_revision`),
  KEY `idx_item_ledger_item_created` (`item_uid`,`created_at`),
  KEY `idx_item_ledger_from_owner` (`from_owner_type`,`from_owner_id`,`from_owner_context_id`,`created_at`),
  KEY `idx_item_ledger_to_owner` (`to_owner_type`,`to_owner_id`,`to_owner_context_id`,`created_at`),
  CONSTRAINT `item_ownership_operation_fk` FOREIGN KEY (`operation_id`) REFERENCES `critical_operation_inbox` (`operation_id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `collector_catalog_state` (
  `state_id` tinyint unsigned NOT NULL, `catalog_revision` bigint unsigned NOT NULL DEFAULT '0',
  `next_listing` bigint unsigned NOT NULL DEFAULT '1',
  `updated_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6) ON UPDATE CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`state_id`),
  CONSTRAINT `chk_collector_catalog_singleton` CHECK ((`state_id` = 1)),
  CONSTRAINT `chk_collector_catalog_next_listing` CHECK ((`next_listing` > 0))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO `collector_catalog_state` (`state_id`,`catalog_revision`,`next_listing`) VALUES (1,0,1);
CREATE TABLE `collector_deaths` (
  `death_operation_id` binary(16) NOT NULL, `beneficiary_pid` int unsigned NOT NULL,
  `death_time` bigint unsigned NOT NULL,
  `collection_delay` bigint unsigned NOT NULL DEFAULT '43200',
  `sale_delay` bigint unsigned NOT NULL DEFAULT '86400',
  `holding_duration` bigint unsigned NOT NULL DEFAULT '604800',
  `price_percent` bigint unsigned NOT NULL DEFAULT '200',
  `minimum_value` bigint unsigned NOT NULL DEFAULT '100',
  `hint_state` tinyint unsigned NOT NULL DEFAULT '0',
  `hint_revision` bigint unsigned NOT NULL DEFAULT '0',
  `created_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  `updated_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6) ON UPDATE CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`death_operation_id`),
  UNIQUE KEY `uq_collector_death_identity` (`beneficiary_pid`,`death_time`),
  CONSTRAINT `chk_collector_death_beneficiary` CHECK ((`beneficiary_pid` > 0)),
  CONSTRAINT `chk_collector_death_time` CHECK ((`death_time` > 0)),
  CONSTRAINT `chk_collector_death_collection_delay` CHECK ((`collection_delay` > 0)),
  CONSTRAINT `chk_collector_death_sale_delay` CHECK ((`sale_delay` >= `collection_delay`)),
  CONSTRAINT `chk_collector_death_holding_duration` CHECK ((`holding_duration` > 0)),
  CONSTRAINT `chk_collector_death_price_percent` CHECK ((`price_percent` > 0)),
  CONSTRAINT `chk_collector_death_minimum_value` CHECK ((`minimum_value` > 0)),
  CONSTRAINT `chk_collector_hint_state` CHECK ((`hint_state` between 0 and 2))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `collector_listings` (
  `listing_id` bigint unsigned NOT NULL, `death_operation_id` binary(16) NOT NULL,
  `beneficiary_pid` int unsigned NOT NULL, `item_uid` bigint unsigned NOT NULL,
  `status` tinyint unsigned NOT NULL, `holding_paused` tinyint unsigned NOT NULL DEFAULT '0',
  `due_at` bigint unsigned DEFAULT NULL, `listing_revision` bigint unsigned NOT NULL,
  `item_revision` bigint unsigned NOT NULL, `price_value` bigint unsigned NOT NULL DEFAULT '0',
  `record_blob` varbinary(154) NOT NULL, `item_blob` mediumblob DEFAULT NULL,
  `created_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  `updated_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6) ON UPDATE CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`listing_id`), UNIQUE KEY `uq_collector_death_item` (`death_operation_id`,`item_uid`),
  KEY `idx_collector_item_history` (`item_uid`,`listing_id`),
  KEY `idx_collector_beneficiary` (`beneficiary_pid`,`status`,`listing_id`),
  KEY `idx_collector_due` (`status`,`holding_paused`,`due_at`,`listing_id`),
  CONSTRAINT `chk_collector_listing_id` CHECK ((`listing_id` > 0)),
  CONSTRAINT `chk_collector_listing_beneficiary` CHECK ((`beneficiary_pid` > 0)),
  CONSTRAINT `chk_collector_listing_item` CHECK ((`item_uid` > 0)),
  CONSTRAINT `chk_collector_listing_status` CHECK ((`status` between 1 and 6)),
  CONSTRAINT `chk_collector_listing_paused` CHECK ((`holding_paused` between 0 and 1)),
  CONSTRAINT `chk_collector_listing_revision` CHECK ((`listing_revision` > 0)),
  CONSTRAINT `chk_collector_listing_item_revision` CHECK ((`item_revision` > 0)),
  CONSTRAINT `chk_collector_record_size` CHECK ((octet_length(`record_blob`) = 154)),
  CONSTRAINT `chk_collector_item_blob_size` CHECK (((`item_blob` is null) or (octet_length(`item_blob`) between 1 and 131072))),
  CONSTRAINT `collector_listing_death_fk` FOREIGN KEY (`death_operation_id`) REFERENCES `collector_deaths` (`death_operation_id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `collector_ledger` (
  `operation_id` binary(16) NOT NULL, `listing_id` bigint unsigned NOT NULL,
  `action` tinyint unsigned NOT NULL, `catalog_revision` bigint unsigned NOT NULL,
  `listing_revision` bigint unsigned NOT NULL, `actor_pid` int unsigned NOT NULL DEFAULT '0',
  `item_uid` bigint unsigned NOT NULL, `value_delta` bigint NOT NULL DEFAULT '0',
  `closed_reason` tinyint unsigned NOT NULL DEFAULT '0', `source_site` smallint unsigned NOT NULL,
  `created_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`operation_id`,`listing_id`), KEY `idx_collector_ledger_listing` (`listing_id`,`listing_revision`),
  KEY `idx_collector_ledger_actor` (`actor_pid`,`created_at`),
  KEY `idx_collector_ledger_item` (`item_uid`,`created_at`),
  CONSTRAINT `chk_collector_ledger_listing` CHECK ((`listing_id` > 0)),
  CONSTRAINT `chk_collector_ledger_action` CHECK ((`action` between 1 and 7)),
  CONSTRAINT `chk_collector_ledger_catalog_revision` CHECK ((`catalog_revision` > 0)),
  CONSTRAINT `chk_collector_ledger_listing_revision` CHECK ((`listing_revision` > 0)),
  CONSTRAINT `chk_collector_ledger_item` CHECK ((`item_uid` > 0)),
  CONSTRAINT `chk_collector_ledger_reason` CHECK ((`closed_reason` between 0 and 7))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `collector_reconciliation_quarantine` (
  `quarantine_id` bigint unsigned NOT NULL AUTO_INCREMENT, `listing_id` bigint unsigned NOT NULL,
  `item_uid` bigint unsigned NOT NULL DEFAULT '0', `conflict_code` smallint unsigned NOT NULL,
  `evidence` varchar(255) NOT NULL, `detected_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  `repaired_at` timestamp(6) NULL DEFAULT NULL, PRIMARY KEY (`quarantine_id`),
  UNIQUE KEY `uq_collector_quarantine` (`listing_id`,`item_uid`,`conflict_code`),
  KEY `idx_collector_quarantine_open` (`repaired_at`,`listing_id`),
  CONSTRAINT `chk_collector_quarantine_listing` CHECK ((`listing_id` > 0)),
  CONSTRAINT `chk_collector_quarantine_conflict` CHECK ((`conflict_code` > 0)),
  CONSTRAINT `chk_collector_quarantine_evidence` CHECK ((char_length(`evidence`) > 0))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `auction_item_custody` (
  `auction_id` int unsigned NOT NULL, `slot` smallint unsigned NOT NULL,
  `item_uid` bigint unsigned NOT NULL, `item_revision` bigint unsigned NOT NULL,
  `vnum` int NOT NULL, `obj_blob` longblob NOT NULL, `claim_pid` int unsigned DEFAULT NULL,
  `claim_operation_id` binary(16) DEFAULT NULL, `claimed_at` timestamp(6) NULL DEFAULT NULL,
  `created_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`auction_id`,`slot`), UNIQUE KEY `uq_auction_custody_item` (`item_uid`),
  KEY `idx_auction_custody_claim_operation` (`claim_operation_id`),
  KEY `idx_auction_custody_claim` (`claim_pid`,`claimed_at`,`auction_id`),
  CONSTRAINT `auction_custody_item_fk` FOREIGN KEY (`item_uid`) REFERENCES `item_current_owner` (`item_uid`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `auction_ledger` (
  `operation_id` binary(16) NOT NULL, `event_type` tinyint unsigned NOT NULL,
  `auction_id` int unsigned NOT NULL, `auction_revision` bigint unsigned NOT NULL,
  `actor_pid` int unsigned NOT NULL DEFAULT '0', `counterparty_pid` int unsigned NOT NULL DEFAULT '0',
  `value_delta` bigint NOT NULL DEFAULT '0', `final_price` bigint NOT NULL DEFAULT '0',
  `item_count` smallint unsigned NOT NULL DEFAULT '0',
  `created_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6), PRIMARY KEY (`operation_id`),
  KEY `idx_auction_ledger_revision` (`auction_id`,`auction_revision`),
  KEY `idx_auction_ledger_actor` (`actor_pid`,`created_at`),
  CONSTRAINT `auction_ledger_operation_fk` FOREIGN KEY (`operation_id`) REFERENCES `critical_operation_inbox` (`operation_id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `auction_reconciliation_quarantine` (
  `quarantine_id` bigint unsigned NOT NULL AUTO_INCREMENT, `auction_id` int unsigned NOT NULL,
  `item_uid` bigint unsigned NOT NULL DEFAULT '0', `conflict_code` smallint unsigned NOT NULL,
  `evidence` varchar(255) NOT NULL, `detected_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  `repaired_at` timestamp(6) NULL DEFAULT NULL, PRIMARY KEY (`quarantine_id`),
  UNIQUE KEY `uq_auction_quarantine` (`auction_id`,`item_uid`,`conflict_code`),
  KEY `idx_auction_quarantine_open` (`repaired_at`,`auction_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `combat_frag_baseline` (
  `pid` int unsigned NOT NULL, `opening_frags` bigint NOT NULL,
  `opening_revision` bigint unsigned NOT NULL DEFAULT '0',
  `captured_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6), PRIMARY KEY (`pid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `combat_outcome` (
  `operation_id` binary(16) NOT NULL, `pkill_event_id` int unsigned NOT NULL,
  `victim_pid` int unsigned NOT NULL, `room_vnum` int NOT NULL,
  `participant_count` smallint unsigned NOT NULL,
  `created_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6), PRIMARY KEY (`operation_id`),
  UNIQUE KEY `uq_combat_pkill_event` (`pkill_event_id`),
  KEY `idx_combat_victim_created` (`victim_pid`,`created_at`),
  KEY `idx_combat_outcome_created_operation` (`created_at`,`operation_id`),
  CONSTRAINT `combat_outcome_operation_fk` FOREIGN KEY (`operation_id`) REFERENCES `critical_operation_inbox` (`operation_id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `combat_outcome_participant` (
  `operation_id` binary(16) NOT NULL, `participant_index` smallint unsigned NOT NULL,
  `pid` int unsigned NOT NULL, `role` tinyint unsigned NOT NULL, `flags` tinyint unsigned NOT NULL,
  `frag_delta` bigint NOT NULL, `epic_delta` bigint NOT NULL, `wallet_delta_copper` bigint NOT NULL,
  `frag_after` bigint NOT NULL, `frag_revision` bigint unsigned NOT NULL,
  `epic_revision` bigint unsigned NOT NULL, `wallet_revision` bigint unsigned NOT NULL,
  `bank_revision` bigint unsigned NOT NULL, PRIMARY KEY (`operation_id`,`participant_index`),
  UNIQUE KEY `uq_combat_participant` (`operation_id`,`pid`),
  KEY `idx_combat_participant_pid` (`pid`,`operation_id`),
  CONSTRAINT `combat_participant_operation_fk` FOREIGN KEY (`operation_id`) REFERENCES `combat_outcome` (`operation_id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `combat_frag_ledger` (
  `operation_id` binary(16) NOT NULL, `participant_index` smallint unsigned NOT NULL,
  `pid` int unsigned NOT NULL, `delta` bigint NOT NULL, `frags_after` bigint NOT NULL,
  `frag_revision` bigint unsigned NOT NULL,
  `created_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`operation_id`,`participant_index`),
  UNIQUE KEY `uq_combat_frag_pid_revision` (`pid`,`frag_revision`),
  KEY `idx_combat_frag_pid_created` (`pid`,`created_at`),
  KEY `idx_combat_frag_created_operation` (`created_at`,`operation_id`,`participant_index`,`pid`),
  CONSTRAINT `combat_frag_operation_fk` FOREIGN KEY (`operation_id`) REFERENCES `combat_outcome` (`operation_id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `artifact_domain_state` (
  `vnum` int NOT NULL, `owned` tinyint unsigned NOT NULL DEFAULT '0',
  `loc_type` tinyint unsigned NOT NULL DEFAULT '1', `location` int NOT NULL DEFAULT '0',
  `timer_epoch` bigint NOT NULL DEFAULT '0', `artifact_type` tinyint unsigned NOT NULL DEFAULT '0',
  `bind_owner_pid` int NOT NULL DEFAULT '0', `bind_timer_epoch` bigint NOT NULL DEFAULT '0',
  `item_uid` bigint unsigned DEFAULT NULL, `item_revision` bigint unsigned DEFAULT NULL,
  `revision` bigint unsigned NOT NULL DEFAULT '0',
  `updated_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6) ON UPDATE CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`vnum`), KEY `idx_artifact_domain_owner` (`loc_type`,`location`,`vnum`),
  KEY `idx_artifact_domain_item` (`item_uid`),
  CONSTRAINT `artifact_domain_item_fk` FOREIGN KEY (`item_uid`) REFERENCES `item_current_owner` (`item_uid`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `artifact_domain_baseline` (
  `vnum` int NOT NULL, `opening_timer_epoch` bigint NOT NULL,
  `opening_bind_owner_pid` int NOT NULL, `opening_bind_timer_epoch` bigint NOT NULL,
  `opening_revision` bigint unsigned NOT NULL DEFAULT '0',
  `captured_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6), PRIMARY KEY (`vnum`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `artifact_guild_outcome` (
  `operation_id` binary(16) NOT NULL, `parent_operation_id` binary(16) NOT NULL,
  `actor_pid` int unsigned NOT NULL, `guild_id` int unsigned NOT NULL DEFAULT '0',
  `prestige_delta` bigint NOT NULL DEFAULT '0', `construction_delta` bigint NOT NULL DEFAULT '0',
  `artifact_count` smallint unsigned NOT NULL DEFAULT '0',
  `guild_prestige_after` bigint unsigned NOT NULL DEFAULT '0',
  `guild_construction_after` bigint unsigned NOT NULL DEFAULT '0',
  `guild_revision` bigint unsigned NOT NULL DEFAULT '0',
  `created_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6), PRIMARY KEY (`operation_id`),
  UNIQUE KEY `uq_artifact_guild_parent_actor` (`parent_operation_id`,`actor_pid`),
  KEY `idx_artifact_guild_actor_created` (`actor_pid`,`created_at`),
  CONSTRAINT `artifact_guild_operation_fk` FOREIGN KEY (`operation_id`) REFERENCES `critical_operation_inbox` (`operation_id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `artifact_guild_parent_fk` FOREIGN KEY (`parent_operation_id`) REFERENCES `critical_operation_inbox` (`operation_id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `artifact_guild_outcome_delta` (
  `operation_id` binary(16) NOT NULL, `artifact_index` smallint unsigned NOT NULL,
  `vnum` int NOT NULL, `flags` tinyint unsigned NOT NULL, `timer_before` bigint NOT NULL,
  `timer_after` bigint NOT NULL, `bind_owner_before` int NOT NULL, `bind_owner_after` int NOT NULL,
  `bind_timer_before` bigint NOT NULL, `bind_timer_after` bigint NOT NULL,
  `revision` bigint unsigned NOT NULL, PRIMARY KEY (`operation_id`,`artifact_index`),
  UNIQUE KEY `uq_artifact_outcome_vnum` (`operation_id`,`vnum`),
  CONSTRAINT `artifact_outcome_delta_fk` FOREIGN KEY (`operation_id`) REFERENCES `artifact_guild_outcome` (`operation_id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `artifact_delta_ledger` (
  `operation_id` binary(16) NOT NULL, `artifact_index` smallint unsigned NOT NULL,
  `vnum` int NOT NULL, `timer_delta` bigint NOT NULL, `bind_owner_pid` int NOT NULL,
  `bind_timer_epoch` bigint NOT NULL, `revision` bigint unsigned NOT NULL,
  `created_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`operation_id`,`artifact_index`), UNIQUE KEY `uq_artifact_delta_revision` (`vnum`,`revision`),
  KEY `idx_artifact_delta_created` (`vnum`,`created_at`),
  CONSTRAINT `artifact_delta_operation_fk` FOREIGN KEY (`operation_id`) REFERENCES `artifact_guild_outcome` (`operation_id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `guild_outcome_ledger` (
  `operation_id` binary(16) NOT NULL, `guild_id` int unsigned NOT NULL,
  `prestige_delta` bigint NOT NULL, `construction_delta` bigint NOT NULL,
  `prestige_after` bigint unsigned NOT NULL, `construction_after` bigint unsigned NOT NULL,
  `guild_revision` bigint unsigned NOT NULL,
  `created_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6), PRIMARY KEY (`operation_id`),
  UNIQUE KEY `uq_guild_outcome_revision` (`guild_id`,`guild_revision`),
  KEY `idx_guild_outcome_created` (`guild_id`,`created_at`),
  CONSTRAINT `guild_outcome_operation_fk` FOREIGN KEY (`operation_id`) REFERENCES `artifact_guild_outcome` (`operation_id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `boon_reward_outcome` (
  `operation_id` binary(16) NOT NULL, `pid` int unsigned NOT NULL,
  `option` tinyint unsigned NOT NULL, `event_value` double NOT NULL,
  `entry_count` smallint unsigned NOT NULL,
  `created_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`operation_id`), KEY `idx_boon_reward_player_created` (`pid`,`created_at`),
  KEY `idx_boon_reward_created_operation` (`created_at`,`operation_id`),
  CONSTRAINT `boon_reward_operation_fk` FOREIGN KEY (`operation_id`) REFERENCES `critical_operation_inbox` (`operation_id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `boon_reward_outcome_entry` (
  `operation_id` binary(16) NOT NULL, `entry_index` smallint unsigned NOT NULL,
  `boon_id` int NOT NULL, `counter_after` double NOT NULL,
  `completed` tinyint unsigned NOT NULL, `reward_type` tinyint unsigned NOT NULL,
  `reward_value` double NOT NULL, PRIMARY KEY (`operation_id`,`entry_index`),
  UNIQUE KEY `uq_boon_reward_operation_boon` (`operation_id`,`boon_id`),
  KEY `idx_boon_reward_boon` (`boon_id`,`operation_id`),
  CONSTRAINT `boon_reward_entry_operation_fk` FOREIGN KEY (`operation_id`) REFERENCES `boon_reward_outcome` (`operation_id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `zone_touch_outcome` (
  `operation_id` binary(16) NOT NULL, `zone_number` int NOT NULL,
  `toucher_pid` int unsigned NOT NULL, `boot_time` int NOT NULL, `touched_at` int NOT NULL,
  `group_size` smallint unsigned NOT NULL, `epic_value` int NOT NULL,
  `alignment_delta` smallint NOT NULL, `reset_requested` tinyint unsigned NOT NULL DEFAULT '0',
  `created_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`operation_id`), KEY `idx_zone_touch_outcome_zone_created` (`zone_number`,`created_at`),
  KEY `idx_zone_touch_outcome_created_operation` (`created_at`,`operation_id`),
  CONSTRAINT `zone_touch_outcome_operation_fk` FOREIGN KEY (`operation_id`) REFERENCES `critical_operation_inbox` (`operation_id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `zone_touch_outcome_participant` (
  `operation_id` binary(16) NOT NULL, `participant_index` smallint unsigned NOT NULL,
  `pid` int unsigned NOT NULL, `epic_value` int NOT NULL,
  PRIMARY KEY (`operation_id`,`participant_index`),
  UNIQUE KEY `uq_zone_touch_operation_pid` (`operation_id`,`pid`),
  KEY `idx_zone_touch_participant_pid` (`pid`,`operation_id`),
  CONSTRAINT `zone_touch_participant_operation_fk` FOREIGN KEY (`operation_id`) REFERENCES `zone_touch_outcome` (`operation_id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `session_audit_outcome` (
  `operation_id` binary(16) NOT NULL, `pid` int unsigned NOT NULL,
  `event_type` tinyint unsigned NOT NULL, `occurred_at` timestamp NOT NULL,
  `created_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`operation_id`), KEY `idx_session_audit_player_time` (`pid`,`occurred_at`),
  KEY `idx_session_audit_event_time` (`event_type`,`occurred_at`),
  CONSTRAINT `session_audit_operation_fk` FOREIGN KEY (`operation_id`) REFERENCES `critical_operation_inbox` (`operation_id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `chk_session_audit_event` CHECK (`event_type` IN (1,2))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE `lifecycle_archive_jobs` (
  `job_id` binary(16) NOT NULL, `job_key` binary(32) NOT NULL,
  `policy_id` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL,
  `policy_schema_version` int unsigned NOT NULL, `manifest_checksum` binary(32) NOT NULL,
  `store_id` varchar(128) COLLATE utf8mb4_unicode_ci NOT NULL,
  `action` tinyint unsigned NOT NULL, `dry_run` tinyint unsigned NOT NULL DEFAULT '1',
  `target_environment` varchar(16) COLLATE utf8mb4_unicode_ci NOT NULL,
  `approval_reference` varchar(128) COLLATE utf8mb4_unicode_ci NOT NULL,
  `status` tinyint unsigned NOT NULL, `source_cursor` varbinary(512) NOT NULL,
  `source_upper_bound` varbinary(512) NOT NULL, `row_budget` smallint unsigned NOT NULL,
  `byte_budget` int unsigned NOT NULL, `time_budget_usec` int unsigned NOT NULL,
  `attempt_count` int unsigned NOT NULL DEFAULT '0',
  `source_count` bigint unsigned NOT NULL DEFAULT '0',
  `archive_count` bigint unsigned NOT NULL DEFAULT '0',
  `source_checksum` binary(32) DEFAULT NULL, `archive_checksum` binary(32) DEFAULT NULL,
  `reconciliation_before` tinyint unsigned NOT NULL DEFAULT '0',
  `reconciliation_after` tinyint unsigned NOT NULL DEFAULT '0',
  `last_error_code` int unsigned NOT NULL DEFAULT '0',
  `created_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  `updated_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6) ON UPDATE CURRENT_TIMESTAMP(6),
  `completed_at` timestamp(6) NULL DEFAULT NULL, PRIMARY KEY (`job_id`),
  UNIQUE KEY `uq_lifecycle_archive_job_key` (`job_key`),
  KEY `idx_lifecycle_archive_job_claim` (`status`,`updated_at`,`job_id`),
  KEY `idx_lifecycle_archive_job_store` (`store_id`,`status`,`job_id`),
  CONSTRAINT `chk_lifecycle_archive_job_action` CHECK (`action` between 1 and 5),
  CONSTRAINT `chk_lifecycle_archive_job_status` CHECK (`status` between 1 and 8),
  CONSTRAINT `chk_lifecycle_archive_job_dry_run` CHECK (`dry_run` in (0,1)),
  CONSTRAINT `chk_lifecycle_archive_job_budgets` CHECK ((`row_budget` between 1 and 256) and (`byte_budget` between 1 and 1048576) and (`time_budget_usec` between 1 and 500000))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `lifecycle_archive_batches` (
  `batch_id` binary(16) NOT NULL, `batch_key` binary(32) NOT NULL,
  `job_id` binary(16) NOT NULL, `sequence_number` bigint unsigned NOT NULL,
  `status` tinyint unsigned NOT NULL, `cursor_start` varbinary(512) NOT NULL,
  `cursor_end` varbinary(512) NOT NULL, `source_count` int unsigned NOT NULL DEFAULT '0',
  `archive_count` int unsigned NOT NULL DEFAULT '0',
  `source_bytes` int unsigned NOT NULL DEFAULT '0',
  `source_checksum` binary(32) DEFAULT NULL, `archive_checksum` binary(32) DEFAULT NULL,
  `reconciliation_before` tinyint unsigned NOT NULL DEFAULT '0',
  `reconciliation_after` tinyint unsigned NOT NULL DEFAULT '0',
  `attempt_count` int unsigned NOT NULL DEFAULT '0',
  `last_error_code` int unsigned NOT NULL DEFAULT '0',
  `started_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  `verified_at` timestamp(6) NULL DEFAULT NULL,
  `completed_at` timestamp(6) NULL DEFAULT NULL, PRIMARY KEY (`batch_id`),
  UNIQUE KEY `uq_lifecycle_archive_batch_key` (`batch_key`),
  UNIQUE KEY `uq_lifecycle_archive_batch_sequence` (`job_id`,`sequence_number`),
  UNIQUE KEY `uq_lifecycle_archive_batch_job_id` (`job_id`,`batch_id`),
  KEY `idx_lifecycle_archive_batch_resume` (`job_id`,`status`,`sequence_number`),
  CONSTRAINT `lifecycle_archive_batch_job_fk` FOREIGN KEY (`job_id`) REFERENCES `lifecycle_archive_jobs` (`job_id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `chk_lifecycle_archive_batch_status` CHECK (`status` between 1 and 8)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `lifecycle_archive_rows` (
  `batch_id` binary(16) NOT NULL, `source_key` varbinary(512) NOT NULL,
  `source_checksum` binary(32) NOT NULL, `payload` longblob NOT NULL,
  `payload_bytes` int unsigned NOT NULL,
  `archived_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`batch_id`,`source_key`),
  CONSTRAINT `lifecycle_archive_row_batch_fk` FOREIGN KEY (`batch_id`) REFERENCES `lifecycle_archive_batches` (`batch_id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `chk_lifecycle_archive_row_size` CHECK (`payload_bytes` between 1 and 1048576)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `lifecycle_archive_evidence` (
  `evidence_id` bigint unsigned NOT NULL AUTO_INCREMENT, `job_id` binary(16) NOT NULL,
  `batch_id` binary(16) DEFAULT NULL, `event_type` tinyint unsigned NOT NULL,
  `status` tinyint unsigned NOT NULL, `row_count` int unsigned NOT NULL DEFAULT '0',
  `byte_count` int unsigned NOT NULL DEFAULT '0',
  `error_code` int unsigned NOT NULL DEFAULT '0',
  `occurred_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`evidence_id`), KEY `idx_lifecycle_archive_evidence_job` (`job_id`,`evidence_id`),
  CONSTRAINT `lifecycle_archive_evidence_job_fk` FOREIGN KEY (`job_id`) REFERENCES `lifecycle_archive_jobs` (`job_id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `lifecycle_archive_evidence_batch_fk` FOREIGN KEY (`job_id`,`batch_id`) REFERENCES `lifecycle_archive_batches` (`job_id`,`batch_id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `chk_lifecycle_archive_evidence_event` CHECK (`event_type` between 1 and 8),
  CONSTRAINT `chk_lifecycle_archive_evidence_status` CHECK (`status` between 1 and 8)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `personal_data_export_requests` (
  `request_id` binary(16) NOT NULL, `request_key` binary(32) NOT NULL,
  `account_name` varchar(50) COLLATE utf8mb4_unicode_ci NOT NULL,
  `account_scope_hash` binary(32) NOT NULL,
  `policy_id` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL,
  `policy_schema_version` int unsigned NOT NULL, `manifest_checksum` binary(32) NOT NULL,
  `snapshot_id` binary(16) DEFAULT NULL, `status` tinyint unsigned NOT NULL,
  `attempt_count` smallint unsigned NOT NULL DEFAULT '0',
  `expected_sections` smallint unsigned NOT NULL,
  `completed_sections` smallint unsigned NOT NULL DEFAULT '0',
  `excluded_sections` smallint unsigned NOT NULL DEFAULT '0',
  `record_count` bigint unsigned NOT NULL DEFAULT '0',
  `package_bytes` bigint unsigned NOT NULL DEFAULT '0',
  `package_checksum` binary(32) DEFAULT NULL, `delivery_token_hash` binary(32) DEFAULT NULL,
  `last_error_code` int unsigned NOT NULL DEFAULT '0',
  `requested_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  `started_at` timestamp(6) NULL DEFAULT NULL, `completed_at` timestamp(6) NULL DEFAULT NULL,
  `cancelled_at` timestamp(6) NULL DEFAULT NULL, `expires_at` timestamp(6) NOT NULL,
  PRIMARY KEY (`request_id`), UNIQUE KEY `uq_personal_export_request_key` (`request_key`),
  KEY `idx_personal_export_account_rate` (`account_name`,`requested_at`,`request_id`),
  KEY `idx_personal_export_work` (`status`,`requested_at`,`request_id`),
  KEY `idx_personal_export_expiry` (`status`,`expires_at`,`request_id`),
  CONSTRAINT `chk_personal_export_status` CHECK (`status` between 1 and 9),
  CONSTRAINT `chk_personal_export_section_counts` CHECK ((`completed_sections` + `excluded_sections`) <= `expected_sections`),
  CONSTRAINT `chk_personal_export_expiry` CHECK (`expires_at` > `requested_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `personal_data_export_sections` (
  `request_id` binary(16) NOT NULL,
  `store_id` varchar(128) COLLATE utf8mb4_unicode_ci NOT NULL,
  `disposition` tinyint unsigned NOT NULL, `status` tinyint unsigned NOT NULL,
  `snapshot_id` binary(16) DEFAULT NULL, `record_count` bigint unsigned NOT NULL DEFAULT '0',
  `byte_count` bigint unsigned NOT NULL DEFAULT '0', `section_checksum` binary(32) DEFAULT NULL,
  `exclusion_code` smallint unsigned NOT NULL DEFAULT '0',
  `last_error_code` int unsigned NOT NULL DEFAULT '0',
  `completed_at` timestamp(6) NULL DEFAULT NULL, PRIMARY KEY (`request_id`,`store_id`),
  KEY `idx_personal_export_section_status` (`request_id`,`status`,`store_id`),
  CONSTRAINT `personal_export_section_request_fk` FOREIGN KEY (`request_id`) REFERENCES `personal_data_export_requests` (`request_id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `chk_personal_export_disposition` CHECK (`disposition` between 1 and 4),
  CONSTRAINT `chk_personal_export_section_status` CHECK (`status` between 1 and 9)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `personal_data_export_audit` (
  `audit_id` bigint unsigned NOT NULL AUTO_INCREMENT, `request_id` binary(16) NOT NULL,
  `event_type` tinyint unsigned NOT NULL, `status` tinyint unsigned NOT NULL,
  `section_count` smallint unsigned NOT NULL DEFAULT '0',
  `record_count` bigint unsigned NOT NULL DEFAULT '0',
  `byte_count` bigint unsigned NOT NULL DEFAULT '0',
  `error_code` int unsigned NOT NULL DEFAULT '0',
  `occurred_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`audit_id`), KEY `idx_personal_export_audit_request` (`request_id`,`audit_id`),
  CONSTRAINT `personal_export_audit_request_fk` FOREIGN KEY (`request_id`) REFERENCES `personal_data_export_requests` (`request_id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `chk_personal_export_audit_event` CHECK (`event_type` between 1 and 9),
  CONSTRAINT `chk_personal_export_audit_status` CHECK (`status` between 1 and 9)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `account_erasure_requests` (
  `request_id` binary(16) NOT NULL, `request_key` binary(32) NOT NULL,
  `account_scope_hash` binary(32) NOT NULL, `subject_token` binary(32) NOT NULL,
  `policy_id` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL,
  `policy_schema_version` int unsigned NOT NULL, `manifest_checksum` binary(32) NOT NULL,
  `status` tinyint unsigned NOT NULL, `fence_revision` bigint unsigned DEFAULT NULL,
  `expected_stores` smallint unsigned NOT NULL,
  `completed_stores` smallint unsigned NOT NULL DEFAULT '0',
  `retained_stores` smallint unsigned NOT NULL DEFAULT '0',
  `reconciliation_checksum` binary(32) DEFAULT NULL,
  `last_error_code` int unsigned NOT NULL DEFAULT '0',
  `requested_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  `confirmed_at` timestamp(6) NULL DEFAULT NULL,
  `fenced_at` timestamp(6) NULL DEFAULT NULL, `completed_at` timestamp(6) NULL DEFAULT NULL,
  `cancelled_at` timestamp(6) NULL DEFAULT NULL, PRIMARY KEY (`request_id`),
  UNIQUE KEY `uq_account_erasure_request_key` (`request_key`),
  KEY `idx_account_erasure_scope_rate` (`account_scope_hash`,`requested_at`,`request_id`),
  KEY `idx_account_erasure_work` (`status`,`requested_at`,`request_id`),
  CONSTRAINT `chk_account_erasure_status` CHECK (`status` between 1 and 10),
  CONSTRAINT `chk_account_erasure_counts` CHECK ((`completed_stores` + `retained_stores`) <= `expected_stores`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `account_erasure_stores` (
  `request_id` binary(16) NOT NULL, `store_id` varchar(128) COLLATE utf8mb4_unicode_ci NOT NULL,
  `action` tinyint unsigned NOT NULL, `status` tinyint unsigned NOT NULL,
  `sequence_number` smallint unsigned NOT NULL,
  `affected_count` bigint unsigned NOT NULL DEFAULT '0',
  `remaining_direct_identifiers` bigint unsigned NOT NULL DEFAULT '0',
  `evidence_checksum` binary(32) DEFAULT NULL, `last_error_code` int unsigned NOT NULL DEFAULT '0',
  `completed_at` timestamp(6) NULL DEFAULT NULL, PRIMARY KEY (`request_id`,`store_id`),
  UNIQUE KEY `uq_account_erasure_store_sequence` (`request_id`,`sequence_number`),
  KEY `idx_account_erasure_store_work` (`request_id`,`status`,`sequence_number`),
  CONSTRAINT `account_erasure_store_request_fk` FOREIGN KEY (`request_id`) REFERENCES `account_erasure_requests` (`request_id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `chk_account_erasure_action` CHECK (`action` between 1 and 6),
  CONSTRAINT `chk_account_erasure_store_status` CHECK (`status` between 1 and 10)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `account_erasure_evidence` (
  `evidence_id` bigint unsigned NOT NULL AUTO_INCREMENT, `request_id` binary(16) NOT NULL,
  `store_id` varchar(128) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `event_type` tinyint unsigned NOT NULL, `status` tinyint unsigned NOT NULL,
  `affected_count` bigint unsigned NOT NULL DEFAULT '0',
  `remaining_count` bigint unsigned NOT NULL DEFAULT '0',
  `error_code` int unsigned NOT NULL DEFAULT '0',
  `occurred_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`evidence_id`), KEY `idx_account_erasure_evidence_request` (`request_id`,`evidence_id`),
  CONSTRAINT `account_erasure_evidence_request_fk` FOREIGN KEY (`request_id`) REFERENCES `account_erasure_requests` (`request_id`) ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT `chk_account_erasure_evidence_event` CHECK (`event_type` between 1 and 10),
  CONSTRAINT `chk_account_erasure_evidence_status` CHECK (`status` between 1 and 10)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `account_erasure_tombstones` (
  `subject_token` binary(32) NOT NULL, `request_id` binary(16) NOT NULL,
  `account_scope_hash` binary(32) NOT NULL, `policy_id` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL,
  `policy_schema_version` int unsigned NOT NULL, `manifest_checksum` binary(32) NOT NULL,
  `completed_at` timestamp(6) NOT NULL, `last_restore_generation` binary(32) DEFAULT NULL,
  `restore_apply_count` int unsigned NOT NULL DEFAULT '0', PRIMARY KEY (`subject_token`),
  UNIQUE KEY `uq_account_erasure_tombstone_request` (`request_id`),
  KEY `idx_account_erasure_tombstone_scope` (`account_scope_hash`),
  CONSTRAINT `account_erasure_tombstone_request_fk` FOREIGN KEY (`request_id`) REFERENCES `account_erasure_requests` (`request_id`) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `mud_schema_baselines` (
  `baseline_id` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL,
  `baseline_kind` enum('fresh_bootstrap','verified_legacy_adoption') COLLATE utf8mb4_unicode_ci NOT NULL,
  `schema_fingerprint` binary(32) NOT NULL, `manifest_version` int unsigned NOT NULL,
  `runner_version` int unsigned NOT NULL,
  `adopted_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`baseline_id`), UNIQUE KEY `uq_mud_schema_baseline_kind` (`baseline_kind`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `mud_schema_history` (
  `migration_id` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL,
  `sequence_number` int unsigned NOT NULL,
  `description` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL,
  `apply_checksum` binary(32) NOT NULL, `verify_checksum` binary(32) NOT NULL,
  `compatibility` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL,
  `runner_version` int unsigned NOT NULL,
  `applied_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`migration_id`),
  UNIQUE KEY `uq_mud_schema_history_sequence` (`sequence_number`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE `mud_schema_migration_state` (
  `state_id` tinyint unsigned NOT NULL, `applied_count` int unsigned NOT NULL,
  `history_checksum` binary(32) NOT NULL,
  `updated_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6) ON UPDATE CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`state_id`),
  CONSTRAINT `chk_mud_schema_migration_state_id` CHECK (`state_id` = 1)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO `mud_schema_migration_state` (`state_id`,`applied_count`,`history_checksum`)
VALUES (1,0,UNHEX(SHA2('',256)));

-- One durable reward claim per stone incarnation (globally allocated object UID).
CREATE TABLE IF NOT EXISTS epic_stone_claim (
    stone_uid BIGINT UNSIGNED NOT NULL,
    operation_id BINARY(16) NOT NULL,
    PRIMARY KEY (stone_uid),
    KEY idx_epic_stone_claim_operation (operation_id),
    CONSTRAINT epic_stone_claim_operation_fk FOREIGN KEY (operation_id)
        REFERENCES critical_operation_inbox (operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Replay-safe rollup projections keep checkpoint totals separate from coverage.
CREATE TABLE IF NOT EXISTS `telemetry_rollup_session` (
  `definition_version` int unsigned NOT NULL,
  `generation` bigint unsigned NOT NULL,
  `environment_id` bigint unsigned NOT NULL,
  `season_id` bigint unsigned NOT NULL,
  `session_boot_id` bigint unsigned NOT NULL,
  `session_process_id` bigint unsigned NOT NULL,
  `session_seq` bigint unsigned NOT NULL,
  `subject_id` bigint unsigned NOT NULL,
  `pid` int NOT NULL,
  `latest_checkpoint_revision` bigint unsigned NOT NULL DEFAULT '0',
  `connected_usec` bigint unsigned NOT NULL DEFAULT '0',
  `active_usec` bigint unsigned NOT NULL DEFAULT '0',
  `idle_usec` bigint unsigned NOT NULL DEFAULT '0',
  `unknown_usec` bigint unsigned NOT NULL DEFAULT '0',
  `resident_usec` bigint unsigned NOT NULL DEFAULT '0',
  `linkdead_usec` bigint unsigned NOT NULL DEFAULT '0',
  `covered_connected_usec` bigint unsigned NOT NULL DEFAULT '0',
  `covered_active_usec` bigint unsigned NOT NULL DEFAULT '0',
  `covered_idle_usec` bigint unsigned NOT NULL DEFAULT '0',
  `covered_unknown_usec` bigint unsigned NOT NULL DEFAULT '0',
  `covered_resident_usec` bigint unsigned NOT NULL DEFAULT '0',
  `covered_linkdead_usec` bigint unsigned NOT NULL DEFAULT '0',
  `attributable_usec` bigint unsigned NOT NULL DEFAULT '0',
  `observed_intervals` bigint unsigned NOT NULL DEFAULT '0',
  `entered` tinyint unsigned NOT NULL DEFAULT '0',
  `exited` tinyint unsigned NOT NULL DEFAULT '0',
  `end_reason` tinyint unsigned NOT NULL DEFAULT '0',
  `quality_flags` int unsigned NOT NULL DEFAULT '0',
  `input_watermark` bigint unsigned NOT NULL DEFAULT '0',
  `provisional` tinyint unsigned NOT NULL DEFAULT '1',
  PRIMARY KEY (`definition_version`,`generation`,`environment_id`,`season_id`,`session_boot_id`,`session_process_id`,`session_seq`),
  KEY `idx_rollup_session_subject` (`definition_version`,`generation`,`environment_id`,`season_id`,`subject_id`,`session_boot_id`,`session_process_id`,`session_seq`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `telemetry_cohort_member` (
  `definition_version` int unsigned NOT NULL,
  `generation` bigint unsigned NOT NULL,
  `environment_id` bigint unsigned NOT NULL,
  `season_id` bigint unsigned NOT NULL,
  `utc_day` date NOT NULL,
  `level_band` smallint unsigned NOT NULL,
  `class_id` smallint unsigned NOT NULL,
  `race_id` smallint unsigned NOT NULL,
  `faction_id` smallint unsigned NOT NULL,
  `zone_vnum` int NOT NULL,
  `config_id` bigint unsigned NOT NULL,
  `category` tinyint unsigned NOT NULL,
  `membership_kind` tinyint unsigned NOT NULL,
  `subject_id` bigint unsigned NOT NULL,
  `session_boot_id` bigint unsigned NOT NULL,
  `session_process_id` bigint unsigned NOT NULL,
  `session_seq` bigint unsigned NOT NULL,
  `duration_usec` bigint unsigned NOT NULL DEFAULT '0',
  `attributable_usec` bigint unsigned NOT NULL DEFAULT '0',
  `observed_intervals` bigint unsigned NOT NULL DEFAULT '0',
  `quality_flags` int unsigned NOT NULL DEFAULT '0',
  `input_watermark` bigint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`definition_version`,`generation`,`environment_id`,`season_id`,`utc_day`,`level_band`,`class_id`,`race_id`,`faction_id`,`zone_vnum`,`config_id`,`category`,`subject_id`,`session_boot_id`,`session_process_id`,`session_seq`),
  CONSTRAINT `chk_telemetry_cohort_member_kind` CHECK ((`membership_kind` = 1 AND `session_boot_id` = 0 AND `session_process_id` = 0 AND `session_seq` = 0) OR (`membership_kind` = 2 AND `session_boot_id` <> 0 AND `session_process_id` <> 0 AND `session_seq` <> 0)),
  CONSTRAINT `chk_telemetry_cohort_member_subject` CHECK (`subject_id` <> 0)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Replay-safe committed reward projection.  Source ledgers remain authority;
-- outcome rows are context and are never added to reward totals here.
CREATE TABLE IF NOT EXISTS `telemetry_reward_projection` (
  `source_kind` tinyint unsigned NOT NULL,
  `operation_id` binary(16) NOT NULL,
  `entry_index` smallint unsigned NOT NULL,
  `participant_pid` int unsigned NOT NULL,
  `source_table` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL,
  `source_created_at` timestamp(6) NOT NULL,
  `authority_kind` tinyint unsigned NOT NULL,
  `reward_kind` tinyint unsigned NOT NULL DEFAULT '0',
  `gross_amount` bigint DEFAULT NULL,
  `net_amount` bigint DEFAULT NULL,
  `transfer_amount` bigint DEFAULT NULL,
  `parent_operation_id` binary(16) DEFAULT NULL,
  `economic_operation_id` binary(16) NOT NULL,
  `reason_type` smallint unsigned NOT NULL DEFAULT '0',
  `reason_id` bigint NOT NULL DEFAULT '0',
  `source_site` smallint unsigned NOT NULL DEFAULT '0',
  `is_transfer` tinyint unsigned NOT NULL DEFAULT '0',
  `is_creation` tinyint unsigned NOT NULL DEFAULT '0',
  `context_complete` tinyint unsigned NOT NULL DEFAULT '1',
  `status` tinyint unsigned NOT NULL,
  `quality_flags` int unsigned NOT NULL DEFAULT '0',
  `source_payload_digest` binary(32) NOT NULL,
  `cycle_id` bigint unsigned NOT NULL,
  `projected_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`source_kind`,`operation_id`,`entry_index`,`participant_pid`),
  KEY `idx_reward_projection_scan` (`source_kind`,`source_created_at`,`operation_id`,`entry_index`,`participant_pid`),
  KEY `idx_reward_projection_participant` (`participant_pid`,`source_created_at`,`source_kind`),
  KEY `idx_reward_projection_economic` (`economic_operation_id`,`reward_kind`,`source_kind`),
  KEY `idx_reward_projection_status` (`source_kind`,`status`,`source_created_at`),
  CONSTRAINT `chk_reward_projection_source_kind` CHECK (`source_kind` between 1 and 10),
  CONSTRAINT `chk_reward_projection_authority` CHECK (`authority_kind` between 0 and 2),
  CONSTRAINT `chk_reward_projection_reward` CHECK (`reward_kind` between 0 and 3),
  CONSTRAINT `chk_reward_projection_status` CHECK (`status` between 1 and 5),
  CONSTRAINT `chk_reward_projection_transfer` CHECK (`is_transfer` between 0 and 1),
  CONSTRAINT `chk_reward_projection_creation` CHECK (`is_creation` between 0 and 1),
  CONSTRAINT `chk_reward_projection_context_complete` CHECK (`context_complete` between 0 and 1),
  CONSTRAINT `chk_reward_projection_authoritative_amount` CHECK ((`authority_kind` <> 1) or (`reward_kind` <> 0 and `gross_amount` is not null and `net_amount` is not null)),
  CONSTRAINT `chk_reward_projection_context_amount` CHECK ((`authority_kind` <> 2) or (`gross_amount` is null and `net_amount` is null and `transfer_amount` is null)),
  CONSTRAINT `chk_reward_projection_transfer_creation` CHECK ((`is_transfer` = 0) or (`is_creation` = 0)),
  CONSTRAINT `chk_reward_projection_transfer_amount` CHECK ((`transfer_amount` is null) or (`is_transfer` = 1 and `transfer_amount` >= 0))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `telemetry_reward_projection_state` (
  `source_kind` tinyint unsigned NOT NULL,
  `cycle_id` bigint unsigned NOT NULL DEFAULT '0',
  `fast_cursor_created_at` timestamp(6) NULL DEFAULT NULL,
  `fast_cursor_operation_id` binary(16) NOT NULL,
  `fast_cursor_entry_index` smallint unsigned NOT NULL DEFAULT '0',
  `fast_cursor_participant_pid` int unsigned NOT NULL DEFAULT '0',
  `reconcile_cursor_created_at` timestamp(6) NULL DEFAULT NULL,
  `reconcile_cursor_operation_id` binary(16) NOT NULL,
  `reconcile_cursor_entry_index` smallint unsigned NOT NULL DEFAULT '0',
  `reconcile_cursor_participant_pid` int unsigned NOT NULL DEFAULT '0',
  `cycle_high_water` timestamp(6) NULL DEFAULT NULL,
  `retention_floor` timestamp(6) NULL DEFAULT NULL,
  `acknowledged_through` timestamp(6) NULL DEFAULT NULL,
  `backlog_rows` bigint unsigned NOT NULL DEFAULT '0',
  `quality_flags` int unsigned NOT NULL DEFAULT '0',
  `provisional` tinyint unsigned NOT NULL DEFAULT '1',
  `last_fast_started_at` timestamp(6) NULL DEFAULT NULL,
  `last_fast_completed_at` timestamp(6) NULL DEFAULT NULL,
  `last_reconcile_started_at` timestamp(6) NULL DEFAULT NULL,
  `last_reconcile_completed_at` timestamp(6) NULL DEFAULT NULL,
  `updated_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6) ON UPDATE CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`source_kind`),
  KEY `idx_reward_projection_state_reconcile` (`provisional`,`acknowledged_through`,`retention_floor`),
  CONSTRAINT `chk_reward_projection_state_source_kind` CHECK (`source_kind` between 1 and 10),
  CONSTRAINT `chk_reward_projection_state_provisional` CHECK (`provisional` between 0 and 1)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `player_death_restitution_receipt` (
  `restitution_id` binary(16) NOT NULL,
  `source_pid` int NOT NULL,
  `death_revision` bigint unsigned NOT NULL,
  `recipient_pid` int NOT NULL,
  `death_operation_id` binary(16) NOT NULL,
  `evidence_digest` binary(32) NOT NULL,
  `plan_digest` binary(32) NOT NULL,
  `status` tinyint unsigned NOT NULL DEFAULT '1',
  `actor` varchar(128) NOT NULL,
  `reason` varchar(255) NOT NULL,
  `candidate_count` smallint unsigned NOT NULL DEFAULT '0',
  `delivered_count` smallint unsigned NOT NULL DEFAULT '0',
  `unresolved_count` smallint unsigned NOT NULL DEFAULT '0',
  `created_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  `approved_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  `applied_at` timestamp(6) NULL DEFAULT NULL,
  `verified_at` timestamp(6) NULL DEFAULT NULL,
  PRIMARY KEY (`restitution_id`),
  UNIQUE KEY `uq_restitution_plan` (`source_pid`,`death_revision`,`recipient_pid`,`plan_digest`),
  KEY `idx_restitution_source` (`source_pid`,`death_revision`),
  KEY `idx_restitution_operation` (`death_operation_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `player_death_restitution_item` (
  `restitution_id` binary(16) NOT NULL,
  `item_uid` bigint unsigned NOT NULL,
  `source_root_item_uid` bigint unsigned NOT NULL DEFAULT '0',
  `source_parent_item_uid` bigint unsigned NOT NULL DEFAULT '0',
  `delivered_root_item_uid` bigint unsigned NOT NULL DEFAULT '0',
  `delivered_parent_item_uid` bigint unsigned NOT NULL DEFAULT '0',
  `source_item_revision` bigint unsigned NOT NULL DEFAULT '0',
  `delivered_item_revision` bigint unsigned NOT NULL DEFAULT '0',
  `vnum` int NOT NULL DEFAULT '0',
  `artifact_vnum` int NOT NULL DEFAULT '0',
  `disposition` tinyint unsigned NOT NULL,
  `classification` varchar(64) NOT NULL,
  `metadata_digest` binary(32) NULL,
  `metadata_payload` mediumblob NULL,
  `note` varchar(255) NOT NULL DEFAULT '',
  `artifact_loss_epoch` bigint unsigned NOT NULL DEFAULT '0',
  `artifact_source_timer_epoch` bigint unsigned NOT NULL DEFAULT '0',
  `artifact_usable_lifetime_seconds` bigint unsigned NOT NULL DEFAULT '0',
  `artifact_delivered_timer_epoch` bigint unsigned NOT NULL DEFAULT '0',
  `artifact_timing_basis` varchar(64) NOT NULL DEFAULT '',
  `artifact_compensation_reference` varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`restitution_id`,`item_uid`),
  KEY `idx_restitution_item_uid` (`item_uid`),
  CONSTRAINT `restitution_item_receipt_fk` FOREIGN KEY (`restitution_id`)
      REFERENCES `player_death_restitution_receipt` (`restitution_id`)
      ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `player_death_restitution_delivery` (
  `item_uid` bigint unsigned NOT NULL,
  `restitution_id` binary(16) NOT NULL,
  `source_pid` int NOT NULL,
  `death_revision` bigint unsigned NOT NULL,
  `recipient_pid` int NOT NULL,
  `source_item_revision` bigint unsigned NOT NULL,
  `delivered_item_revision` bigint unsigned NOT NULL,
  `delivered_item_id` int unsigned NOT NULL,
  `metadata_digest` binary(32) NOT NULL,
  `original_payload` mediumblob NOT NULL,
  `delivered_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`item_uid`),
  UNIQUE KEY `uq_restitution_delivery_receipt_item` (`restitution_id`,`item_uid`),
  KEY `idx_restitution_delivery_receipt` (`restitution_id`),
  KEY `idx_restitution_delivery_recipient` (`recipient_pid`,`item_uid`),
  CONSTRAINT `restitution_delivery_receipt_fk` FOREIGN KEY (`restitution_id`,`item_uid`)
      REFERENCES `player_death_restitution_item` (`restitution_id`,`item_uid`)
      ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `player_death_restitution_runtime` (
  `item_uid` bigint unsigned NOT NULL,
  `recipient_pid` int NOT NULL,
  `state_payload` mediumblob NOT NULL,
  `state_digest` binary(32) NOT NULL,
  `updated_at` timestamp(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)
      ON UPDATE CURRENT_TIMESTAMP(6),
  PRIMARY KEY (`item_uid`),
  KEY `idx_restitution_runtime_recipient` (`recipient_pid`,`item_uid`),
  CONSTRAINT `restitution_runtime_delivery_fk` FOREIGN KEY (`item_uid`)
      REFERENCES `player_death_restitution_delivery` (`item_uid`)
      ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

SET FOREIGN_KEY_CHECKS = 1;

-- Economic accounting storage; no activation or opening balances.
-- Additive economic evidence and retained identity metadata.
-- No baseline, active epoch, account mapping or gameplay value is seeded here.
-- Provisional 0031 after canonical telemetry 0030; recheck allocation before merge.

CREATE TABLE IF NOT EXISTS economic_epoch (
    lineage BINARY(16) NOT NULL,
    epoch BINARY(16) NOT NULL,
    ordinal BIGINT UNSIGNED NOT NULL,
    predecessor BINARY(16) NULL,
    transition_kind SMALLINT UNSIGNED NOT NULL,
    transition_digest BINARY(32) NOT NULL,
    creating_operation_id BINARY(16) NOT NULL,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (lineage,epoch),
    UNIQUE KEY uq_economic_epoch_ordinal (lineage,ordinal),
    CONSTRAINT chk_economic_epoch_ordinal CHECK (ordinal > 0),
    CONSTRAINT chk_economic_epoch_transition CHECK (transition_kind > 0),
    CONSTRAINT chk_economic_epoch_predecessor CHECK (predecessor IS NULL OR predecessor <> epoch),
    CONSTRAINT economic_epoch_predecessor_fk FOREIGN KEY (lineage,predecessor)
        REFERENCES economic_epoch(lineage,epoch) ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT economic_epoch_operation_fk FOREIGN KEY (creating_operation_id)
        REFERENCES critical_operation_inbox(operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS economic_lineage_state (
    lineage BINARY(16) NOT NULL,
    active_epoch BINARY(16) NULL,
    revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (lineage),
    CONSTRAINT economic_lineage_epoch_fk FOREIGN KEY (lineage,active_epoch)
        REFERENCES economic_epoch(lineage,epoch) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS economic_account_mapping (
    mapping_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    lineage BINARY(16) NOT NULL,
    account_kind SMALLINT UNSIGNED NOT NULL,
    context_id BIGINT UNSIGNED NOT NULL DEFAULT 0,
    backend_kind TINYINT UNSIGNED NOT NULL,
    locator_kind SMALLINT UNSIGNED NOT NULL,
    native_id BIGINT UNSIGNED NOT NULL,
    active_native_id BIGINT UNSIGNED NULL,
    creating_operation_id BINARY(16) NOT NULL,
    retiring_operation_id BINARY(16) NULL,
    revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (mapping_id),
    UNIQUE KEY uq_economic_active_mapping
        (lineage,backend_kind,locator_kind,account_kind,context_id,active_native_id),
    KEY idx_economic_retained_mapping
        (lineage,backend_kind,locator_kind,native_id,mapping_id),
    CONSTRAINT chk_economic_mapping_kind CHECK (account_kind BETWEEN 1 AND 6),
    CONSTRAINT chk_economic_mapping_backend CHECK (backend_kind IN (1,2)),
    CONSTRAINT chk_economic_mapping_locator CHECK (locator_kind > 0 AND native_id > 0),
    CONSTRAINT chk_economic_mapping_active CHECK (
        (active_native_id IS NOT NULL AND active_native_id = native_id AND retiring_operation_id IS NULL)
        OR (active_native_id IS NULL AND retiring_operation_id IS NOT NULL)),
    CONSTRAINT economic_mapping_lineage_fk FOREIGN KEY (lineage)
        REFERENCES economic_lineage_state(lineage) ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT economic_mapping_creation_fk FOREIGN KEY (creating_operation_id)
        REFERENCES critical_operation_inbox(operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT economic_mapping_retirement_fk FOREIGN KEY (retiring_operation_id)
        REFERENCES critical_operation_inbox(operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS economic_accounting_operation (
    operation_id BINARY(16) NOT NULL,
    lineage BINARY(16) NOT NULL,
    epoch BINARY(16) NOT NULL,
    original_operation_id BINARY(16) NULL,
    accounting_version SMALLINT UNSIGNED NOT NULL,
    writer_id INT UNSIGNED NOT NULL,
    policy_version INT UNSIGNED NOT NULL,
    compiler_version INT UNSIGNED NOT NULL,
    actor_kind TINYINT UNSIGNED NOT NULL,
    actor_id BIGINT UNSIGNED NOT NULL,
    reason SMALLINT UNSIGNED NOT NULL,
    source_event BINARY(48) NULL,
    intent_digest BINARY(32) NOT NULL,
    domain_digest BINARY(32) NOT NULL,
    plan_digest BINARY(32) NULL,
    canonical_intent VARBINARY(8192) NOT NULL,
    canonical_plan MEDIUMBLOB NULL,
    outcome TINYINT UNSIGNED NOT NULL,
    result_code INT UNSIGNED NOT NULL,
    account_count SMALLINT UNSIGNED NOT NULL,
    posting_count SMALLINT UNSIGNED NOT NULL,
    child_count SMALLINT UNSIGNED NOT NULL,
    item_event_count SMALLINT UNSIGNED NOT NULL,
    before_witness_count SMALLINT UNSIGNED NOT NULL,
    after_witness_count SMALLINT UNSIGNED NOT NULL,
    recorded_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (operation_id),
    KEY idx_economic_operation_reason (lineage,epoch,reason,recorded_at,operation_id),
    KEY idx_economic_operation_original (original_operation_id,operation_id),
    UNIQUE KEY uq_economic_operation_source (operation_id,lineage,source_event,outcome),
    CONSTRAINT chk_economic_operation_version CHECK
        (accounting_version = 1 AND writer_id > 0 AND policy_version > 0 AND compiler_version > 0),
    CONSTRAINT chk_economic_operation_actor CHECK (actor_kind IN (1,2) AND actor_id > 0),
    CONSTRAINT chk_economic_operation_reason CHECK (reason BETWEEN 1 AND 42),
    CONSTRAINT chk_economic_operation_intent CHECK (OCTET_LENGTH(canonical_intent) BETWEEN 256 AND 8192),
    CONSTRAINT chk_economic_operation_counts CHECK
        (account_count <= 3072 AND posting_count <= 6144 AND child_count <= 64
         AND item_event_count <= 3000 AND before_witness_count <= 6000 AND after_witness_count <= 6000),
    CONSTRAINT chk_economic_operation_outcome CHECK (
        (outcome = 1 AND result_code = 0 AND plan_digest IS NOT NULL
         AND canonical_plan IS NOT NULL AND OCTET_LENGTH(canonical_plan) BETWEEN 256 AND 4194304)
        OR (outcome = 2 AND result_code <> 0 AND plan_digest IS NULL AND canonical_plan IS NULL
            AND account_count = 0 AND posting_count = 0 AND child_count = 0
            AND item_event_count = 0 AND before_witness_count = 0 AND after_witness_count = 0)),
    CONSTRAINT economic_operation_inbox_fk FOREIGN KEY (operation_id)
        REFERENCES critical_operation_inbox(operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT economic_operation_epoch_fk FOREIGN KEY (lineage,epoch)
        REFERENCES economic_epoch(lineage,epoch) ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT economic_operation_original_fk FOREIGN KEY (original_operation_id)
        REFERENCES critical_operation_inbox(operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS economic_accounting_account_effect (
    operation_id BINARY(16) NOT NULL,
    account_index SMALLINT UNSIGNED NOT NULL,
    account_key BINARY(40) NOT NULL,
    before_copper BIGINT NOT NULL,
    before_silver BIGINT NOT NULL,
    before_gold BIGINT NOT NULL,
    before_platinum BIGINT NOT NULL,
    after_copper BIGINT NOT NULL,
    after_silver BIGINT NOT NULL,
    after_gold BIGINT NOT NULL,
    after_platinum BIGINT NOT NULL,
    before_revision BIGINT UNSIGNED NOT NULL,
    after_revision BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (operation_id,account_index),
    UNIQUE KEY uq_economic_effect_account (operation_id,account_key),
    KEY idx_economic_effect_history (account_key,operation_id),
    CONSTRAINT chk_economic_effect_index CHECK (account_index < 3072),
    CONSTRAINT economic_effect_operation_fk FOREIGN KEY (operation_id)
        REFERENCES economic_accounting_operation(operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS economic_accounting_coin_posting (
    operation_id BINARY(16) NOT NULL,
    line_index SMALLINT UNSIGNED NOT NULL,
    event_index INT UNSIGNED NOT NULL,
    account_index SMALLINT UNSIGNED NOT NULL,
    child_index SMALLINT UNSIGNED NOT NULL,
    delta_copper BIGINT NOT NULL,
    delta_silver BIGINT NOT NULL,
    delta_gold BIGINT NOT NULL,
    delta_platinum BIGINT NOT NULL,
    copper_value BIGINT NOT NULL,
    PRIMARY KEY (operation_id,line_index),
    UNIQUE KEY uq_economic_posting_event (operation_id,event_index),
    CONSTRAINT chk_economic_posting_indexes CHECK
        (line_index < 6144 AND event_index = line_index AND account_index < 3072 AND child_index <= 64),
    CONSTRAINT economic_posting_account_fk FOREIGN KEY (operation_id,account_index)
        REFERENCES economic_accounting_account_effect(operation_id,account_index)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS economic_accounting_child (
    operation_id BINARY(16) NOT NULL,
    child_index SMALLINT UNSIGNED NOT NULL,
    child_operation_id BINARY(16) NOT NULL,
    domain_id INT UNSIGNED NOT NULL,
    discriminator BIGINT UNSIGNED NOT NULL,
    parent_index SMALLINT UNSIGNED NOT NULL,
    relationship SMALLINT UNSIGNED NOT NULL,
    receipt_operation_id BINARY(16) NULL,
    PRIMARY KEY (operation_id,child_index),
    UNIQUE KEY uq_economic_child_operation (child_operation_id),
    CONSTRAINT chk_economic_child_indexes CHECK
        (child_index BETWEEN 1 AND 64 AND parent_index < child_index),
    CONSTRAINT chk_economic_child_relationship CHECK
        (relationship = 1 AND domain_id > 0 AND child_operation_id <> operation_id),
    CONSTRAINT chk_economic_child_receipt CHECK
        (receipt_operation_id IS NULL OR receipt_operation_id = child_operation_id),
    CONSTRAINT economic_child_root_fk FOREIGN KEY (operation_id)
        REFERENCES economic_accounting_operation(operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT economic_child_receipt_fk FOREIGN KEY (receipt_operation_id)
        REFERENCES critical_operation_inbox(operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Bind the item reference's indexed UID/revision to the original immutable event.
-- This additive index does not rewrite or duplicate the existing ledger.
SET @economic_item_reference_index = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema=DATABASE() AND table_name='item_ownership_ledger'
      AND index_name='uq_item_ledger_accounting_reference');
SET @economic_item_reference_sql = IF(@economic_item_reference_index=0,
    'CREATE UNIQUE INDEX uq_item_ledger_accounting_reference ON item_ownership_ledger(operation_id,event_index,item_uid,item_revision)',
    'SELECT 1');
PREPARE economic_item_reference_stmt FROM @economic_item_reference_sql;
EXECUTE economic_item_reference_stmt;
DEALLOCATE PREPARE economic_item_reference_stmt;

CREATE TABLE IF NOT EXISTS economic_accounting_item_reference (
    operation_id BINARY(16) NOT NULL,
    line_index SMALLINT UNSIGNED NOT NULL,
    event_index INT UNSIGNED NOT NULL,
    child_index SMALLINT UNSIGNED NOT NULL,
    item_uid BIGINT UNSIGNED NOT NULL,
    before_revision BIGINT UNSIGNED NOT NULL,
    after_revision BIGINT UNSIGNED NOT NULL,
    legacy_operation_id BINARY(16) NOT NULL,
    legacy_event_index SMALLINT UNSIGNED NOT NULL,
    PRIMARY KEY (operation_id,line_index),
    UNIQUE KEY uq_economic_item_event (operation_id,event_index),
    UNIQUE KEY uq_economic_item_legacy (legacy_operation_id,legacy_event_index),
    KEY idx_economic_item_history (item_uid,after_revision),
    CONSTRAINT chk_economic_item_indexes CHECK
        (line_index < 3000 AND event_index = line_index AND child_index <= 64
         AND item_uid > 0 AND after_revision > before_revision),
    CONSTRAINT economic_item_operation_fk FOREIGN KEY (operation_id)
        REFERENCES economic_accounting_operation(operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT economic_item_legacy_fk FOREIGN KEY (legacy_operation_id,legacy_event_index,item_uid,after_revision)
        REFERENCES item_ownership_ledger(operation_id,event_index,item_uid,item_revision) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS economic_accounting_source_claim (
    lineage BINARY(16) NOT NULL,
    source_event BINARY(48) NOT NULL,
    operation_id BINARY(16) NOT NULL,
    outcome TINYINT UNSIGNED NOT NULL DEFAULT 1,
    PRIMARY KEY (lineage,source_event),
    UNIQUE KEY uq_economic_source_operation (operation_id),
    CONSTRAINT chk_economic_source_success CHECK (outcome = 1),
    CONSTRAINT economic_source_operation_fk FOREIGN KEY (operation_id,lineage,source_event,outcome)
        REFERENCES economic_accounting_operation(operation_id,lineage,source_event,outcome)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Baseline witness and opening reservations.
-- Additive baseline retention only. No epochs, mappings, native balances or
-- activation state are created. Complete witnesses are verified by the typed
-- lifecycle transaction; SQL constraints do not authenticate a native snapshot.
CREATE TABLE IF NOT EXISTS economic_baseline_control (
    lineage BINARY(16) NOT NULL,
    epoch BINARY(16) NOT NULL,
    opening_account VARBINARY(40) NOT NULL,
    creating_operation_id BINARY(16) NOT NULL,
    revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    last_operation_id BINARY(16) NULL,
    PRIMARY KEY (lineage,epoch),
    CONSTRAINT ck_economic_baseline_opening CHECK (OCTET_LENGTH(opening_account)=40),
    CONSTRAINT ck_economic_baseline_control_revision CHECK (
        (revision=0 AND last_operation_id IS NULL) OR
        (revision>0 AND last_operation_id IS NOT NULL)),
    CONSTRAINT fk_economic_baseline_control_epoch FOREIGN KEY (lineage,epoch)
        REFERENCES economic_epoch(lineage,epoch) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT fk_economic_baseline_control_create FOREIGN KEY (creating_operation_id)
        REFERENCES critical_operation_inbox(operation_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT fk_economic_baseline_control_last FOREIGN KEY (last_operation_id)
        REFERENCES critical_operation_inbox(operation_id) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS economic_baseline_witness (
    operation_id BINARY(16) NOT NULL,
    lineage BINARY(16) NOT NULL,
    epoch BINARY(16) NOT NULL,
    book_revision BIGINT UNSIGNED NOT NULL,
    witness_version SMALLINT UNSIGNED NOT NULL,
    holding_count SMALLINT UNSIGNED NOT NULL,
    item_count SMALLINT UNSIGNED NOT NULL,
    witness_digest BINARY(32) NOT NULL,
    canonical_witness MEDIUMBLOB NOT NULL,
    PRIMARY KEY (operation_id),
    UNIQUE KEY uq_economic_baseline_witness_epoch (lineage,epoch,operation_id),
    UNIQUE KEY uq_economic_baseline_witness_revision (lineage,epoch,book_revision),
    CONSTRAINT ck_economic_baseline_witness_revision CHECK (book_revision>0),
    CONSTRAINT ck_economic_baseline_witness_shape CHECK (
        witness_version=1 AND holding_count<=3071 AND item_count<=6000 AND
        OCTET_LENGTH(canonical_witness)=192+112*holding_count+88*item_count AND
        SUBSTRING(canonical_witness,1,4)=X'45414231'),
    CONSTRAINT fk_economic_baseline_witness_control FOREIGN KEY (lineage,epoch)
        REFERENCES economic_baseline_control(lineage,epoch) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT fk_economic_baseline_witness_operation FOREIGN KEY (operation_id)
        REFERENCES economic_accounting_operation(operation_id) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Account lifetimes share one namespace across all ordinary kinds/contexts.
-- Item UIDs have a separate namespace. Neither native revision nor preparation
-- ID weakens per-epoch uniqueness; empty batches still retain a witness receipt.
CREATE TABLE IF NOT EXISTS economic_baseline_reservation (
    lineage BINARY(16) NOT NULL,
    epoch BINARY(16) NOT NULL,
    identity_kind TINYINT UNSIGNED NOT NULL,
    identity_id BIGINT UNSIGNED NOT NULL,
    operation_id BINARY(16) NOT NULL,
    PRIMARY KEY (lineage,epoch,identity_kind,identity_id),
    KEY idx_economic_baseline_reservation_operation (lineage,epoch,operation_id),
    CONSTRAINT ck_economic_baseline_reservation_identity CHECK (identity_kind IN (1,2) AND identity_id>0),
    CONSTRAINT fk_economic_baseline_reservation_witness FOREIGN KEY (lineage,epoch,operation_id)
        REFERENCES economic_baseline_witness(lineage,epoch,operation_id) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
