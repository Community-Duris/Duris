-- Migration: Clean up legacy ghost and orphan rows in account_characters
-- Date: 2026-06-14
-- Purpose: Remove stale data that accumulated from the bugs fixed in Phase 10
--
-- Bug 1 (ghost characters): Characters were soft-deleted (deleted_at set) but
--   ON DUPLICATE KEY UPDATE never cleared deleted_at, so re-created characters
--   stayed invisible. The old soft-deleted rows remain.
-- Bug 2 (pid=0): Characters linked before player_data save got pid=0, then
--   ON DUPLICATE KEY UPDATE never propagated the real PID. The pid=0 rows
--   remain as orphans.

-- Remove rows where pid=0 (these are stale entries from characters linked
-- before their first save to player_data)
DELETE FROM `account_characters` WHERE `pid` = 0;

-- Remove soft-deleted rows that have a newer active row for the same
-- (account_name, char_name). Keeps only the most recent active row.
DELETE d
FROM `account_characters` d
INNER JOIN `account_characters` a ON
  d.account_name = a.account_name AND
  d.char_name = a.char_name AND
  d.id < a.id AND
  d.deleted_at IS NOT NULL AND
  a.deleted_at IS NULL;
