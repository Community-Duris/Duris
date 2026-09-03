-- Immutable migration 0009: the kingdom garrison roster.
--
-- Ruled 2026-09-04: guards stopped being a number derived from territory and
-- became individuals a guild BUYS, names a class for and promotes. Land still
-- sets the ceiling -- how many may stand (kingdom_guard_allowance) and how high
-- they may rise (kingdom_guard_level_cap, two levels per completed ring) -- but
-- nothing musters that was not paid for, so unlike the rest of the garrison
-- this is state that must survive a reboot.
--
-- ONE ROW PER GUARD, keyed by the owning association and the roster slot the
-- `kingdom roster` listing numbers guards by. Slots 0..15 are guards; slot 16
-- is the realm's single champion, which only a realm holding all eighty squares
-- may raise. guard_class is a CLASS_* bit from src/core/defines.h, level is the
-- rank the guild has paid for and which never falls. Association ids are
-- reused by found_asc(), so the engine deletes a guild's rows when the guild
-- goes, exactly as it does for kingdom_realms.
--
-- DELIBERATELY OUTSIDE runtime_table_sql_list. The runtime compatibility
-- fingerprint in src/core/runtime_compatibility_contract.h is a SHA-256 over
-- information_schema for the tables named in that list, computed against a live
-- MySQL 8 and a live MariaDB 10.11; adding a name to the list means
-- regenerating both fingerprints on real servers. This table is therefore left
-- out of it, which costs nothing at boot -- the count, engine and collation
-- checks all scope themselves to that same list -- and the module degrades the
-- way kingdom_realms already does when its table cannot be read: the roster
-- loads empty, no guards muster, and nothing else in the server is affected.
-- Adding it to the contract is a follow-up for whoever next has both databases
-- to hand.

CREATE TABLE IF NOT EXISTS kingdom_garrison (
    assoc_id INT NOT NULL,
    slot INT NOT NULL,
    guard_class INT NOT NULL DEFAULT 0,
    level INT NOT NULL DEFAULT 0,
    PRIMARY KEY (assoc_id, slot)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Converge a table that predates this migration on some database, for the same
-- reason 0006 carries the same guard: CREATE TABLE IF NOT EXISTS is a no-op
-- against an existing table, so a table created without a COLLATE clause would
-- keep the server's default collation forever and fail the verifier below.
-- Every column is an integer, so CONVERT TO rewrites no character data and no
-- row value changes; on a table already at utf8mb4_unicode_ci the guard picks
-- the no-op branch and issues no ALTER, which is what keeps this file exactly
-- re-runnable.
SET @kingdom_garrison_collation_drift = (
    SELECT COUNT(*)
    FROM information_schema.tables
    WHERE table_schema = DATABASE()
      AND table_name = 'kingdom_garrison'
      AND table_collation <> 'utf8mb4_unicode_ci'
);
SET @kingdom_garrison_collation_sql = IF(
    @kingdom_garrison_collation_drift = 0,
    'SELECT 1',
    'ALTER TABLE kingdom_garrison CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci'
);
PREPARE kingdom_garrison_collation_stmt FROM @kingdom_garrison_collation_sql;
EXECUTE kingdom_garrison_collation_stmt;
DEALLOCATE PREPARE kingdom_garrison_collation_stmt;
