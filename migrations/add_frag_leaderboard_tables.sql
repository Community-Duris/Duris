-- Migration: Add Frag Leaderboard Tables for Web Statistics
-- Date: 2025-11-07 - Arih
-- Purpose: Hybrid approach - MUD uses flat files, web uses database
--
-- This migration adds two tables:
-- 1. account_characters: Maps characters to accounts with soft delete
-- 2. frag_leaderboard: Denormalized leaderboard data for fast web queries
--
-- NO foreign keys to avoid cascading delete issues and maintain MUD stability

-- ============================================================================
-- Table: account_characters
-- Purpose: Track which characters belong to which accounts
-- ============================================================================
CREATE TABLE IF NOT EXISTS `account_characters` (
  `id` int(11) NOT NULL auto_increment,
  `account_name` varchar(255) NOT NULL COMMENT 'Account name from flat file system',
  `pid` bigint(20) NOT NULL COMMENT 'Player ID - unique character identifier',
  `char_name` varchar(255) NOT NULL COMMENT 'Character name',
  `created_at` datetime DEFAULT CURRENT_TIMESTAMP COMMENT 'When character was created',
  `deleted_at` datetime NULL DEFAULT NULL COMMENT 'Soft delete - NULL means active',
  PRIMARY KEY (`id`),
  UNIQUE KEY `pid` (`pid`),
  KEY `account_name` (`account_name`),
  KEY `char_name` (`char_name`),
  KEY `deleted_at` (`deleted_at`),
  KEY `account_active` (`account_name`, `deleted_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Maps characters to accounts for web statistics';

-- ============================================================================
-- Table: frag_leaderboard
-- Purpose: Denormalized frag leaderboard for fast web queries
-- ============================================================================
CREATE TABLE IF NOT EXISTS `frag_leaderboard` (
  `id` int(11) NOT NULL auto_increment,
  `pid` bigint(20) NOT NULL COMMENT 'Player ID - links to character',
  `account_name` varchar(255) NOT NULL COMMENT 'Account name for account-level stats',
  `char_name` varchar(255) NOT NULL COMMENT 'Character name for display',
  `total_frags` int(11) NOT NULL DEFAULT 0 COMMENT 'Total frags (stored as int, divide by 100 for display)',
  `racewar` int(11) NOT NULL COMMENT 'Racewar side (1=good, 2=evil, 3=undead, 4=illithid)',
  `race` varchar(50) DEFAULT NULL COMMENT 'Character race',
  `class` varchar(50) DEFAULT NULL COMMENT 'Character class',
  `level` int(11) DEFAULT NULL COMMENT 'Character level',
  `deleted_at` datetime NULL DEFAULT NULL COMMENT 'Soft delete - NULL means active',
  `last_updated` datetime DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last time record was updated',
  PRIMARY KEY (`id`),
  UNIQUE KEY `pid` (`pid`),
  KEY `char_name` (`char_name`),
  KEY `account_name` (`account_name`),
  KEY `total_frags_active` (`deleted_at`, `total_frags`),
  KEY `racewar_leaderboard` (`deleted_at`, `racewar`, `total_frags`),
  KEY `race_leaderboard` (`deleted_at`, `race`, `total_frags`),
  KEY `class_leaderboard` (`deleted_at`, `class`, `total_frags`),
  KEY `level_range` (`deleted_at`, `level`, `total_frags`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Denormalized frag leaderboard for web statistics';

-- ============================================================================
-- Sample Queries for Web Frontend
-- ============================================================================

-- Top 100 overall (active characters only)
-- SELECT char_name, total_frags / 100.0 AS frags, racewar, race, class, level
-- FROM frag_leaderboard
-- WHERE deleted_at IS NULL
-- ORDER BY total_frags DESC
-- LIMIT 100;

-- Top 50 by racewar (e.g., good=1, evil=2)
-- SELECT char_name, total_frags / 100.0 AS frags, race, class, level
-- FROM frag_leaderboard
-- WHERE deleted_at IS NULL AND racewar = 1
-- ORDER BY total_frags DESC
-- LIMIT 50;

-- Top 50 by race (e.g., 'drow_elf')
-- SELECT char_name, total_frags / 100.0 AS frags, class, level
-- FROM frag_leaderboard
-- WHERE deleted_at IS NULL AND race = 'drow_elf'
-- ORDER BY total_frags DESC
-- LIMIT 50;

-- Top 50 by class (e.g., 'necromancer')
-- SELECT char_name, total_frags / 100.0 AS frags, race, level
-- FROM frag_leaderboard
-- WHERE deleted_at IS NULL AND class = 'necromancer'
-- ORDER BY total_frags DESC
-- LIMIT 50;

-- All characters for an account (including deleted)
-- SELECT char_name, total_frags / 100.0 AS frags, racewar, race, class, level,
--        deleted_at, last_updated
-- FROM frag_leaderboard
-- WHERE account_name = 'Resakse'
-- ORDER BY total_frags DESC;

-- Frag distribution by class
-- SELECT class, AVG(total_frags / 100.0) AS avg_frags,
--        MIN(total_frags / 100.0) AS min_frags,
--        MAX(total_frags / 100.0) AS max_frags,
--        COUNT(*) AS char_count
-- FROM frag_leaderboard
-- WHERE deleted_at IS NULL
-- GROUP BY class
-- ORDER BY avg_frags DESC;

-- Players who crossed milestone (e.g., 1000 frags)
-- SELECT char_name, total_frags / 100.0 AS frags, account_name, race, class
-- FROM frag_leaderboard
-- WHERE deleted_at IS NULL AND total_frags >= 100000
-- ORDER BY total_frags DESC;

-- Monthly frag gains (using existing progress table)
-- SELECT p.pid, fl.char_name, SUM(p.delta) / 100.0 AS frags_gained
-- FROM progress p
-- JOIN frag_leaderboard fl ON p.pid = fl.pid
-- WHERE p.var_type = 'FRAGS'
--   AND p.stamp >= DATE_SUB(NOW(), INTERVAL 1 MONTH)
--   AND fl.deleted_at IS NULL
-- GROUP BY p.pid, fl.char_name
-- ORDER BY frags_gained DESC
-- LIMIT 50;
