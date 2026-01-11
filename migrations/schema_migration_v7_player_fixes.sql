-- schema_migration_v7_player_fixes.sql
-- adds missing player fields for existing databases
-- act3: surname/achievement flags
-- last_room: room vnum where player was saved
--
-- note: player_db_schema.sql already has these, this is for existing dbs only
-- note: will error if columns already exist - that's ok, just ignore

-- act3 contains player surname bits (serf, commoner, etc) and achievement flags
set @col_exists = (select count(*) from information_schema.columns
    where table_schema = database() and table_name = 'player_data' and column_name = 'act3');
set @sql = if(@col_exists = 0,
    'alter table player_data add column act3 bigint unsigned default 0 after act2',
    'select "act3 already exists"');
prepare stmt from @sql;
execute stmt;
deallocate prepare stmt;

-- last_room is the room vnum where player was saved
set @col_exists = (select count(*) from information_schema.columns
    where table_schema = database() and table_name = 'player_data' and column_name = 'last_room');
set @sql = if(@col_exists = 0,
    'alter table player_data add column last_room int default 0 after orig_birthplace',
    'select "last_room already exists"');
prepare stmt from @sql;
execute stmt;
deallocate prepare stmt;
