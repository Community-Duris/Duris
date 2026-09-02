-- Immutable migration 0006: add the kingdom code's one persistent table. A
-- realm's territory is a single integer: squares are claimed in a fixed order
-- and a ring must complete before the next opens, so a realm owns exactly
-- claim indices 1..highest_claim and there is no per-square ownership set to
-- store. Losing a ring to unpaid upkeep lowers that integer to a ring boundary.
-- One realm per guild, keyed by the owning association id; association ids
-- are reused, so the engine removes the row when its guild is deleted.
-- The column list and ORDER match kingdom_realm_columns in
-- src/kingdom/kingdom_db.c, which reads rows positionally (row[0]..row[10]).
-- Harvested stores are spendable on the realm but never withdrawn, which is
-- why they do not live in the guild coin treasury. upkeep_paid_through is a
-- Unix time; arrears is the ladder 0 current, 1 guards gone, 2 nodes dormant,
-- 3 rings reverting. The TABLE is not yet in the boot contract's table list, so
-- kingdom_initialize() disables kingdoms for the boot when it cannot be read.
-- That is not permission to skip this migration: the boot gate in src/sql/sql.c
-- requires the 0006 ledger row and applied count 6, so a database left at head
-- 0005 refuses to boot.

CREATE TABLE IF NOT EXISTS kingdom_realms (
    assoc_id INT NOT NULL,
    realm_id INT NOT NULL DEFAULT 0,
    hall_vnum INT NOT NULL DEFAULT 0,
    highest_claim INT NOT NULL DEFAULT 0,
    res_mineral BIGINT NOT NULL DEFAULT 0,
    res_wood BIGINT NOT NULL DEFAULT 0,
    res_fibre BIGINT NOT NULL DEFAULT 0,
    res_water BIGINT NOT NULL DEFAULT 0,
    upkeep_paid_through BIGINT NOT NULL DEFAULT 0,
    arrears INT NOT NULL DEFAULT 0,
    missed_cycles INT NOT NULL DEFAULT 0,
    PRIMARY KEY (assoc_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- A database that already ran the pre-registration migrations/kingdom_realms.sql
-- (deleted when this migration was registered) has the table with the server's
-- default collation for utf8mb4 -- utf8mb4_0900_ai_ci on MySQL 8,
-- utf8mb4_general_ci on MariaDB 10.11 -- because that file carried no COLLATE
-- clause. CREATE TABLE IF NOT EXISTS is a no-op there, so without this step the
-- verifier's table_collation check would fail on that database forever.
-- Converge it. Every column is an integer, so CONVERT TO rewrites no character
-- data and no row value changes. On a table already at utf8mb4_unicode_ci the
-- guard selects the no-op branch and issues no ALTER at all, which is what keeps
-- this file exactly re-runnable.
SET @kingdom_realms_collation_drift = (
    SELECT COUNT(*)
    FROM information_schema.tables
    WHERE table_schema = DATABASE()
      AND table_name = 'kingdom_realms'
      AND table_collation <> 'utf8mb4_unicode_ci'
);
SET @kingdom_realms_collation_sql = IF(
    @kingdom_realms_collation_drift = 0,
    'SELECT 1',
    'ALTER TABLE kingdom_realms CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci'
);
PREPARE kingdom_realms_collation_stmt FROM @kingdom_realms_collation_sql;
EXECUTE kingdom_realms_collation_stmt;
DEALLOCATE PREPARE kingdom_realms_collation_stmt;
