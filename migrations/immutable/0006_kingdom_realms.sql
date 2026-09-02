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
-- 3 rings reverting. The table is not yet part of the runtime boot contract:
-- kingdom_initialize() disables kingdoms for the boot when it cannot be read.

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
