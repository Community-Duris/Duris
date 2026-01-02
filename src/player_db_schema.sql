-- player database schema for durismud
-- replaces binary pfile storage with mysql
-- created for feature/pfile-to-db migration

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ============================================================================
-- accounts
-- matches existing web schema (user_profiles, user_profile_stats use account_name)
-- ============================================================================

DROP TABLE IF EXISTS `accounts`;
CREATE TABLE `accounts` (
  `account_name` VARCHAR(50) NOT NULL,  -- matches web schema
  `email` VARCHAR(255) DEFAULT NULL,
  `password` VARCHAR(128) NOT NULL,  -- bcrypt hash, 60 chars
  `confirmation_code` VARCHAR(64) DEFAULT NULL,
  `confirmed` TINYINT(1) DEFAULT 0,
  `confirmation_sent` TINYINT(1) DEFAULT 0,
  `blocked` TINYINT(1) DEFAULT 0,
  `last_login` BIGINT DEFAULT 0,
  `last_good_char` BIGINT DEFAULT 0,  -- timestamp of last good racewar char played
  `last_evil_char` BIGINT DEFAULT 0,  -- timestamp of last evil racewar char played
  `flags1` BIGINT UNSIGNED DEFAULT 0,
  `flags2` BIGINT UNSIGNED DEFAULT 0,
  `flags3` BIGINT UNSIGNED DEFAULT 0,
  `flags4` BIGINT UNSIGNED DEFAULT 0,
  `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`account_name`),
  KEY `email` (`email`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- note: account_ips functionality covered by account_login_history (web table)
-- note: account_players functionality covered by account_characters (web table)
-- we'll use existing web tables instead of duplicating

-- ============================================================================
-- player core data
-- ============================================================================

DROP TABLE IF EXISTS `player_data`;
CREATE TABLE `player_data` (
  `pid` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(64) NOT NULL,
  `account_name` VARCHAR(50) DEFAULT NULL,  -- links to accounts table
  `short_descr` VARCHAR(512) DEFAULT NULL,
  `long_descr` TEXT DEFAULT NULL,
  `description` TEXT DEFAULT NULL,
  `title` VARCHAR(512) DEFAULT NULL,

  -- class/race/level
  `m_class` INT UNSIGNED DEFAULT 0,
  `secondary_class` INT UNSIGNED DEFAULT 0,
  `spec` TINYINT UNSIGNED DEFAULT 0,
  `race` TINYINT UNSIGNED DEFAULT 0,
  `racewar` TINYINT UNSIGNED DEFAULT 0,
  `level` TINYINT UNSIGNED DEFAULT 1,
  `sex` TINYINT UNSIGNED DEFAULT 0,

  -- physical
  `weight` SMALLINT UNSIGNED DEFAULT 0,
  `height` SMALLINT UNSIGNED DEFAULT 0,
  `size` TINYINT DEFAULT 0,

  -- location
  `hometown` INT DEFAULT 0,
  `birthplace` INT DEFAULT 0,
  `orig_birthplace` INT DEFAULT 0,
  `last_room` INT DEFAULT 0,  -- room saved in

  -- time
  `birth_time` BIGINT DEFAULT 0,
  `played_time` INT DEFAULT 0,  -- seconds played
  `last_save` BIGINT DEFAULT 0,
  `perm_aging` SMALLINT DEFAULT 0,

  -- base stats
  `base_str` TINYINT DEFAULT 0,
  `base_dex` TINYINT DEFAULT 0,
  `base_agi` TINYINT DEFAULT 0,
  `base_con` TINYINT DEFAULT 0,
  `base_pow` TINYINT DEFAULT 0,
  `base_int` TINYINT DEFAULT 0,
  `base_wis` TINYINT DEFAULT 0,
  `base_cha` TINYINT DEFAULT 0,
  `base_kar` TINYINT DEFAULT 0,
  `base_luk` TINYINT DEFAULT 0,

  -- points (int to handle overflow from legacy unsigned shorts)
  `mana` INT DEFAULT 0,
  `base_mana` INT DEFAULT 0,
  `hit_diff` INT DEFAULT 0,  -- max_hit - hit, stored as difference
  `base_hit` INT DEFAULT 0,
  `vitality` INT DEFAULT 0,
  `base_vitality` INT DEFAULT 0,
  `spells_memmed_extra` TINYINT DEFAULT 0,  -- spells_memmed[MAX_CIRCLE]

  -- money (can be billions)
  `copper` BIGINT DEFAULT 0,
  `silver` BIGINT DEFAULT 0,
  `gold` BIGINT DEFAULT 0,
  `platinum` BIGINT DEFAULT 0,
  `bank_copper` BIGINT DEFAULT 0,
  `bank_silver` BIGINT DEFAULT 0,
  `bank_gold` BIGINT DEFAULT 0,
  `bank_platinum` BIGINT DEFAULT 0,

  -- experience
  `exp` BIGINT DEFAULT 0,
  `epics` BIGINT DEFAULT 0,
  `epic_skill_points` BIGINT DEFAULT 0,
  `skillpoints` INT DEFAULT 0,
  `spell_bind_used` BIGINT DEFAULT 0,

  -- flags
  `act` BIGINT UNSIGNED DEFAULT 0,
  `act2` BIGINT UNSIGNED DEFAULT 0,
  `vote` BIGINT UNSIGNED DEFAULT 0,
  `alignment` INT DEFAULT 0,

  -- guild
  `prestige` SMALLINT DEFAULT 0,
  `assoc_id` SMALLINT UNSIGNED DEFAULT 0,
  `guild_status` INT UNSIGNED DEFAULT 0,
  `time_left_guild` BIGINT DEFAULT 0,
  `nb_left_guild` TINYINT DEFAULT 0,
  `time_unspecced` BIGINT DEFAULT 0,

  -- pvp
  `frags` BIGINT DEFAULT 0,
  `oldfrags` BIGINT DEFAULT 0,
  `numb_deaths` BIGINT UNSIGNED DEFAULT 0,

  -- conditions (hunger, thirst, etc)
  `condition_0` TINYINT DEFAULT 0,
  `condition_1` TINYINT DEFAULT 0,
  `condition_2` TINYINT DEFAULT 0,
  `condition_3` TINYINT DEFAULT 0,
  `condition_4` TINYINT DEFAULT 0,

  -- immortal stuff
  `poof_in` VARCHAR(512) DEFAULT NULL,
  `poof_out` VARCHAR(512) DEFAULT NULL,
  `poof_in_sound` VARCHAR(512) DEFAULT NULL,
  `poof_out_sound` VARCHAR(512) DEFAULT NULL,
  `echo_toggle` TINYINT UNSIGNED DEFAULT 0,
  `prompt` SMALLINT UNSIGNED DEFAULT 0,
  `wiz_invis` BIGINT DEFAULT 0,
  `law_flags` BIGINT UNSIGNED DEFAULT 0,
  `wimpy` SMALLINT DEFAULT 0,
  `aggressive` SMALLINT DEFAULT -1,
  `highest_level` TINYINT UNSIGNED DEFAULT 0,
  `screen_length` TINYINT UNSIGNED DEFAULT 24,

  -- quest data
  `quest_active` INT DEFAULT 0,
  `quest_mob_vnum` INT DEFAULT 0,
  `quest_type` INT DEFAULT 0,
  `quest_accomplished` INT DEFAULT 0,
  `quest_started` INT DEFAULT 0,
  `quest_zone_number` INT DEFAULT 0,
  `quest_giver` INT DEFAULT 0,
  `quest_level` INT DEFAULT 0,
  `quest_receiver` INT DEFAULT 0,
  `quest_shares_left` INT DEFAULT 0,
  `quest_kill_how_many` INT DEFAULT 0,
  `quest_kill_original` INT DEFAULT 0,
  `quest_map_room` INT DEFAULT 0,
  `quest_map_bought` INT DEFAULT 0,

  -- misc
  `last_ip` BIGINT UNSIGNED DEFAULT 0,

  `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

  PRIMARY KEY (`pid`),
  UNIQUE KEY `name` (`name`),
  KEY `account_name` (`account_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- player arrays (skills, languages, intros, etc)
-- ============================================================================

-- skills: only store non-zero entries (max ~2000 skills but most chars have <500)
DROP TABLE IF EXISTS `player_skills`;
CREATE TABLE `player_skills` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `pid` INT UNSIGNED NOT NULL,
  `skill_id` SMALLINT UNSIGNED NOT NULL,
  `learned` TINYINT UNSIGNED DEFAULT 0,
  `taught` TINYINT UNSIGNED DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `pid` (`pid`),
  UNIQUE KEY `pid_skill` (`pid`, `skill_id`),
  CONSTRAINT `fk_player_skills` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- languages: MAX_TONGUE = 29
DROP TABLE IF EXISTS `player_languages`;
CREATE TABLE `player_languages` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `pid` INT UNSIGNED NOT NULL,
  `tongue_id` TINYINT UNSIGNED NOT NULL,
  `proficiency` TINYINT UNSIGNED DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `pid` (`pid`),
  UNIQUE KEY `pid_tongue` (`pid`, `tongue_id`),
  CONSTRAINT `fk_player_languages` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- intros: MAX_INTRO = 150
DROP TABLE IF EXISTS `player_intros`;
CREATE TABLE `player_intros` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `pid` INT UNSIGNED NOT NULL,
  `intro_index` TINYINT UNSIGNED NOT NULL,
  `intro_pid` INT DEFAULT 0,  -- pid of introduced player
  `intro_time` BIGINT UNSIGNED DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `pid` (`pid`),
  UNIQUE KEY `pid_index` (`pid`, `intro_index`),
  CONSTRAINT `fk_player_intros` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- pc timers: NUMB_PC_TIMERS = 10
DROP TABLE IF EXISTS `player_timers`;
CREATE TABLE `player_timers` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `pid` INT UNSIGNED NOT NULL,
  `timer_id` TINYINT UNSIGNED NOT NULL,
  `timer_value` BIGINT DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `pid` (`pid`),
  UNIQUE KEY `pid_timer` (`pid`, `timer_id`),
  CONSTRAINT `fk_player_timers` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- undead spell slots: MAX_CIRCLE + 1 = 13
DROP TABLE IF EXISTS `player_undead_slots`;
CREATE TABLE `player_undead_slots` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `pid` INT UNSIGNED NOT NULL,
  `circle` TINYINT UNSIGNED NOT NULL,
  `slots` TINYINT DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `pid` (`pid`),
  UNIQUE KEY `pid_circle` (`pid`, `circle`),
  CONSTRAINT `fk_player_undead_slots` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- forged items: MAX_FORGE_ITEMS = 1000, only store non-zero entries
DROP TABLE IF EXISTS `player_forged_items`;
CREATE TABLE `player_forged_items` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `pid` INT UNSIGNED NOT NULL,
  `forge_index` SMALLINT UNSIGNED NOT NULL,
  `item_vnum` INT DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `pid` (`pid`),
  UNIQUE KEY `pid_forge` (`pid`, `forge_index`),
  CONSTRAINT `fk_player_forged_items` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- granted commands
DROP TABLE IF EXISTS `player_granted_cmds`;
CREATE TABLE `player_granted_cmds` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `pid` INT UNSIGNED NOT NULL,
  `cmd_num` INT NOT NULL,
  PRIMARY KEY (`id`),
  KEY `pid` (`pid`),
  UNIQUE KEY `pid_cmd` (`pid`, `cmd_num`),
  CONSTRAINT `fk_player_granted_cmds` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- player affects (spell effects)
-- ============================================================================

DROP TABLE IF EXISTS `player_affects`;
CREATE TABLE `player_affects` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `pid` INT UNSIGNED NOT NULL,
  `type` SMALLINT NOT NULL,  -- spell/skill number
  `duration` INT DEFAULT 0,
  `flags` SMALLINT UNSIGNED DEFAULT 0,
  `modifier` INT DEFAULT 0,
  `location` TINYINT UNSIGNED DEFAULT 0,
  `level` SMALLINT UNSIGNED DEFAULT 0,
  `bitvector1` BIGINT DEFAULT 0,  -- signed to handle legacy data
  `bitvector2` BIGINT DEFAULT 0,
  `bitvector3` BIGINT DEFAULT 0,
  `bitvector4` BIGINT DEFAULT 0,
  `bitvector5` BIGINT DEFAULT 0,
  `custom_msg_char` TEXT DEFAULT NULL,
  `custom_msg_room` TEXT DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `pid` (`pid`),
  CONSTRAINT `fk_player_affects` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- player items (inventory + equipment)
-- ============================================================================

DROP TABLE IF EXISTS `player_items`;
CREATE TABLE `player_items` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `pid` INT UNSIGNED NOT NULL,
  `vnum` INT NOT NULL,
  `equip_slot` TINYINT DEFAULT 0,  -- 0=inventory, 1-42=equipment slot
  `container_id` INT UNSIGNED DEFAULT NULL,  -- self-reference for nested containers
  `quantity` SMALLINT UNSIGNED DEFAULT 1,

  -- item properties that can be modified from prototype
  `weight` INT DEFAULT 0,
  `cost` INT DEFAULT 0,
  `timer` INT DEFAULT -1,
  `extra_flags` BIGINT UNSIGNED DEFAULT 0,

  -- item values (type-dependent)
  `value0` INT DEFAULT 0,
  `value1` INT DEFAULT 0,
  `value2` INT DEFAULT 0,
  `value3` INT DEFAULT 0,
  `value4` INT DEFAULT 0,
  `value5` INT DEFAULT 0,
  `value6` INT DEFAULT 0,
  `value7` INT DEFAULT 0,

  -- strung/modified strings
  `name` VARCHAR(512) DEFAULT NULL,  -- null = use prototype
  `short_descr` VARCHAR(512) DEFAULT NULL,
  `description` TEXT DEFAULT NULL,
  `action_descr` TEXT DEFAULT NULL,

  -- for unique items
  `unique_id` INT UNSIGNED DEFAULT NULL,

  PRIMARY KEY (`id`),
  KEY `pid` (`pid`),
  KEY `container_id` (`container_id`),
  CONSTRAINT `fk_player_items` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE,
  CONSTRAINT `fk_player_items_container` FOREIGN KEY (`container_id`) REFERENCES `player_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- item affects (custom modifiers on items)
DROP TABLE IF EXISTS `player_item_affects`;
CREATE TABLE `player_item_affects` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `item_id` INT UNSIGNED NOT NULL,
  `location` TINYINT UNSIGNED DEFAULT 0,
  `modifier` INT DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `item_id` (`item_id`),
  CONSTRAINT `fk_player_item_affects` FOREIGN KEY (`item_id`) REFERENCES `player_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- player witnesses
-- ============================================================================

DROP TABLE IF EXISTS `player_witnesses`;
CREATE TABLE `player_witnesses` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `pid` INT UNSIGNED NOT NULL,
  `crime` TINYINT UNSIGNED DEFAULT 0,  -- crime type witnessed
  `room_vnum` INT DEFAULT 0,
  `attacker_name` VARCHAR(64) DEFAULT NULL,
  `victim_name` VARCHAR(64) DEFAULT NULL,
  `witness_time` BIGINT DEFAULT 0,  -- when it happened
  PRIMARY KEY (`id`),
  KEY `pid` (`pid`),
  CONSTRAINT `fk_player_witnesses` FOREIGN KEY (`pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- storage lockers
-- ============================================================================

DROP TABLE IF EXISTS `lockers`;
CREATE TABLE `lockers` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `locker_name` VARCHAR(128) NOT NULL,    -- e.g. "Playername.locker" or "guild.123.locker"
  `owner_pid` INT UNSIGNED DEFAULT NULL,  -- for personal lockers
  `owner_assoc_id` INT DEFAULT NULL,       -- for guild lockers
  `racewar` TINYINT DEFAULT 0,
  `race` TINYINT DEFAULT 0,
  `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `locker_name` (`locker_name`),
  KEY `owner_pid` (`owner_pid`),
  KEY `owner_assoc_id` (`owner_assoc_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

DROP TABLE IF EXISTS `locker_items`;
CREATE TABLE `locker_items` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `locker_id` INT UNSIGNED NOT NULL,
  `vnum` INT NOT NULL,
  `container_id` INT UNSIGNED DEFAULT NULL,
  `quantity` SMALLINT UNSIGNED DEFAULT 1,
  `weight` INT DEFAULT 0,
  `cost` INT DEFAULT 0,
  `timer` INT DEFAULT -1,
  `extra_flags` BIGINT UNSIGNED DEFAULT 0,
  `value0` INT DEFAULT 0,
  `value1` INT DEFAULT 0,
  `value2` INT DEFAULT 0,
  `value3` INT DEFAULT 0,
  `value4` INT DEFAULT 0,
  `value5` INT DEFAULT 0,
  `value6` INT DEFAULT 0,
  `value7` INT DEFAULT 0,
  `name` VARCHAR(512) DEFAULT NULL,
  `short_descr` VARCHAR(512) DEFAULT NULL,
  `description` TEXT DEFAULT NULL,
  `action_descr` TEXT DEFAULT NULL,
  `unique_id` INT UNSIGNED DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `locker_id` (`locker_id`),
  KEY `container_id` (`container_id`),
  CONSTRAINT `fk_locker_items` FOREIGN KEY (`locker_id`) REFERENCES `lockers` (`id`) ON DELETE CASCADE,
  CONSTRAINT `fk_locker_items_container` FOREIGN KEY (`container_id`) REFERENCES `locker_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

DROP TABLE IF EXISTS `locker_item_affects`;
CREATE TABLE `locker_item_affects` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `item_id` INT UNSIGNED NOT NULL,
  `location` TINYINT UNSIGNED DEFAULT 0,
  `modifier` INT DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `item_id` (`item_id`),
  CONSTRAINT `fk_locker_item_affects` FOREIGN KEY (`item_id`) REFERENCES `locker_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- locker access permissions (who can access which lockers)
DROP TABLE IF EXISTS `locker_access`;
CREATE TABLE `locker_access` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `locker_id` INT UNSIGNED NOT NULL,
  `pid` INT UNSIGNED NOT NULL,  -- player who has access
  `access_level` TINYINT DEFAULT 1,  -- 1=view, 2=withdraw, 3=deposit, 4=full
  `granted_by_pid` INT UNSIGNED DEFAULT NULL,
  `granted_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `locker_id` (`locker_id`),
  KEY `pid` (`pid`),
  UNIQUE KEY `locker_pid` (`locker_id`, `pid`),
  CONSTRAINT `fk_locker_access` FOREIGN KEY (`locker_id`) REFERENCES `lockers` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- player ships
-- ============================================================================

DROP TABLE IF EXISTS `ships`;
CREATE TABLE `ships` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `owner_pid` INT UNSIGNED DEFAULT NULL,
  `owner_name` VARCHAR(64) NOT NULL,  -- for migration, eventually use pid only
  `ship_name` VARCHAR(128) DEFAULT NULL,
  `ship_class` TINYINT UNSIGNED DEFAULT 0,
  `frags` INT DEFAULT 0,
  `anchor_room` INT DEFAULT 0,
  `time_played` INT DEFAULT 0,
  `mainsail` INT DEFAULT 0,
  `race` TINYINT DEFAULT 0,
  `money` INT DEFAULT 0,
  `flags` BIGINT UNSIGNED DEFAULT 0,
  -- armor for 4 arcs (fore, port, rear, starboard)
  `armor_fore` INT DEFAULT 0,
  `armor_port` INT DEFAULT 0,
  `armor_rear` INT DEFAULT 0,
  `armor_star` INT DEFAULT 0,
  `internal_fore` INT DEFAULT 0,
  `internal_port` INT DEFAULT 0,
  `internal_rear` INT DEFAULT 0,
  `internal_star` INT DEFAULT 0,
  -- crew data
  `crew_index` INT DEFAULT 0,
  `crew_sail_skill` INT DEFAULT 0,  -- stored as skill * 1000
  `crew_guns_skill` INT DEFAULT 0,
  `crew_rpar_skill` INT DEFAULT 0,
  `crew_sail_chief` INT DEFAULT 0,
  `crew_guns_chief` INT DEFAULT 0,
  `crew_rpar_chief` INT DEFAULT 0,
  -- bonuses
  `maxspeed_bonus` INT DEFAULT 0,
  `capacity_bonus` INT DEFAULT 0,
  `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `owner_pid` (`owner_pid`),
  UNIQUE KEY `owner_name` (`owner_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

DROP TABLE IF EXISTS `ship_slots`;
CREATE TABLE `ship_slots` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `ship_id` INT UNSIGNED NOT NULL,
  `slot_index` TINYINT UNSIGNED NOT NULL,  -- 0 to maxslots-1
  `slot_type` TINYINT DEFAULT 0,  -- weapon, cargo, equipment, ammo, etc
  `item_index` INT DEFAULT 0,     -- weapon/cargo/equipment type index
  `position` TINYINT DEFAULT 0,   -- fore/port/rear/star/hold
  `timer` INT DEFAULT 0,
  `val0` INT DEFAULT 0,
  `val1` INT DEFAULT 0,
  `val2` INT DEFAULT 0,
  `val3` INT DEFAULT 0,
  `val4` INT DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `ship_id` (`ship_id`),
  UNIQUE KEY `ship_slot` (`ship_id`, `slot_index`),
  CONSTRAINT `fk_ship_slots` FOREIGN KEY (`ship_id`) REFERENCES `ships` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- guilds/associations
-- ============================================================================

DROP TABLE IF EXISTS `guilds`;
CREATE TABLE `guilds` (
  `id` INT UNSIGNED NOT NULL,  -- matches assoc_id, not auto-increment
  `name` VARCHAR(128) NOT NULL,
  `racewar` TINYINT UNSIGNED DEFAULT 0,
  `bits` INT UNSIGNED DEFAULT 0,
  `prestige` BIGINT UNSIGNED DEFAULT 0,
  `construction` BIGINT UNSIGNED DEFAULT 0,
  `platinum` INT UNSIGNED DEFAULT 0,
  `gold` INT UNSIGNED DEFAULT 0,
  `silver` INT UNSIGNED DEFAULT 0,
  `copper` INT UNSIGNED DEFAULT 0,
  `frags` BIGINT DEFAULT 0,
  `top_frags` BIGINT DEFAULT 0,
  `topfragger` VARCHAR(64) DEFAULT NULL,
  `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

DROP TABLE IF EXISTS `guild_titles`;
CREATE TABLE `guild_titles` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `guild_id` INT UNSIGNED NOT NULL,
  `rank` TINYINT UNSIGNED NOT NULL,  -- 0 to asc_num_ranks-1
  `title` VARCHAR(128) DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `guild_rank` (`guild_id`, `rank`),
  CONSTRAINT `fk_guild_titles` FOREIGN KEY (`guild_id`) REFERENCES `guilds` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

DROP TABLE IF EXISTS `guild_members`;
CREATE TABLE `guild_members` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `guild_id` INT UNSIGNED NOT NULL,
  `player_name` VARCHAR(64) NOT NULL,
  `player_pid` INT UNSIGNED DEFAULT NULL,  -- link to player_data after migration
  `bits` INT UNSIGNED DEFAULT 0,
  `debt` INT UNSIGNED DEFAULT 0,  -- in copper
  PRIMARY KEY (`id`),
  KEY `guild_id` (`guild_id`),
  UNIQUE KEY `guild_player` (`guild_id`, `player_name`),
  CONSTRAINT `fk_guild_members` FOREIGN KEY (`guild_id`) REFERENCES `guilds` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ============================================================================
-- player spellbooks (conjurable mobs)
-- ============================================================================

DROP TABLE IF EXISTS `player_spellbooks`;
CREATE TABLE `player_spellbooks` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `pid` INT UNSIGNED NOT NULL,
  `mob_vnum` INT NOT NULL,
  `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `pid` (`pid`),
  UNIQUE KEY `pid_mob` (`pid`, `mob_vnum`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

SET FOREIGN_KEY_CHECKS = 1;

-- ============================================================================
-- notes
-- ============================================================================
--
-- column sizing follows these guidelines (avoid legacy mistakes):
-- - player names: varchar(64) - current max ~20 but leave room
-- - ip addresses: varchar(45) - ipv6 needs 45 chars
-- - descriptions: text - no artificial limits
-- - flags/bitvectors: bigint unsigned - 64-bit
-- - money/exp: bigint - players can have billions
-- - timestamps: bigint - unix timestamps, not datetime
--
-- skills table only stores non-zero entries to save space
-- (most chars have <500 skills out of 2000 possible)
--
-- forged items table only stores non-zero entries
-- (most chars learn <100 recipes out of 1000 possible)
--
-- item container hierarchy uses self-referencing foreign key
-- (container_id points to parent item's id)
--
