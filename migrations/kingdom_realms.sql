-- kingdom_realms.sql
-- Creates the kingdom code's one persistent table.
--
-- Additive, guarded and re-runnable, per the repository's migration rules:
-- it creates nothing that already exists and drops nothing at all.
--
-- WHY ONLY ONE TABLE, AND NO PER-SQUARE ROWS.
-- A realm's territory is a SINGLE INTEGER. Squares are claimed in a fixed
-- order and a ring must complete before the next opens, so a realm owns
-- exactly claim indices 1..highest_claim and nothing else. There is no
-- ownership set to store, no 80 rows per realm, and losing a ring to unpaid
-- upkeep is just lowering that integer to a ring boundary. Anything that
-- wanted per-square rows would be storing a value it can already derive.
--
-- The column list here MUST match kingdom_realm_columns in
-- src/kingdom/kingdom_db.c, which drives both the SELECT and the INSERT from
-- one string so the two cannot drift. The loader reads the columns
-- positionally (row[0]..row[10]), so ORDER IS PART OF THE CONTRACT.

set @tbl_exists = (select count(*) from information_schema.tables
    where table_schema = database() and table_name = 'kingdom_realms');

set @sql = if(@tbl_exists = 0,
'create table kingdom_realms (
    -- the owning guild. One realm per guild, so this is the key.
    -- NOTE: guild ids are REUSED -- found_asc() hands out the lowest free id
    -- -- so a realm whose guild is deleted must be removed here, or the next
    -- guild to take that id silently inherits its land. The engine does that
    -- through kingdom_on_guild_deleted().
    assoc_id            int             not null,

    realm_id            int             not null default 0,

    -- The map room the guildhall entrance stands on: the centre of the 9x9
    -- footprint. Stored as a VNUM, not an rnum -- rnums are assigned at boot
    -- and are not stable across reboots.
    hall_vnum           int             not null default 0,

    -- 0..80. The whole territory record: the realm owns claims 1..this.
    highest_claim       int             not null default 0,

    -- Harvested stores. Non-withdrawable by design: they may be spent on the
    -- realm but never taken back out, which is why they cannot live in the
    -- guild coin treasury.
    res_mineral         bigint          not null default 0,
    res_wood            bigint          not null default 0,
    res_fibre           bigint          not null default 0,
    res_water           bigint          not null default 0,

    -- Unix time upkeep is settled through.
    upkeep_paid_through bigint          not null default 0,

    -- The arrears ladder: 0 current, 1 guards gone, 2 nodes dormant,
    -- 3 rings reverting.
    arrears             int             not null default 0,
    missed_cycles       int             not null default 0,

    primary key (assoc_id)
) engine=innodb default charset=utf8mb4',
'select ''kingdom_realms already exists''');

prepare stmt from @sql;
execute stmt;
deallocate prepare stmt;
