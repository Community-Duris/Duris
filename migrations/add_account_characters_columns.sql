-- Migration: Add missing columns and unique key to account_characters
-- Date: 2026-06-14
-- Purpose: Fix account/character lifecycle bugs (ghost characters, PID propagation)
--
-- The account_characters table was missing columns used by sql_save_account_characters
-- (login_count, last_login, blocked, racewar) and was missing a unique key on
-- (account_name, char_name) which is needed for the ON DUPLICATE KEY UPDATE to
-- correctly identify rows when the PID changes (pid=0 -> real PID after save, or
-- new PID after delete-recreate cycle).

-- Add missing columns (IF NOT EXISTS safe - won't fail if already present)
ALTER TABLE `account_characters`
  ADD COLUMN IF NOT EXISTS `login_count` bigint(20) unsigned DEFAULT 0 AFTER `char_name`,
  ADD COLUMN IF NOT EXISTS `last_login` datetime NULL DEFAULT NULL AFTER `login_count`,
  ADD COLUMN IF NOT EXISTS `blocked` int(11) DEFAULT 0 AFTER `last_login`,
  ADD COLUMN IF NOT EXISTS `racewar` int(11) DEFAULT 1 AFTER `blocked`;

-- Add unique key on (account_name, char_name) for natural row identification.
-- This is critical: without it, when a character is deleted and re-created
-- (new PID), the ON DUPLICATE KEY UPDATE in sql_save_account_characters won't
-- match the old row and will create a duplicate instead of clearing deleted_at.
ALTER TABLE `account_characters`
  ADD UNIQUE KEY IF NOT EXISTS `acct_char` (`account_name`, `char_name`);
