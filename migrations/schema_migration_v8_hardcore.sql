-- schema_migration_v8_hardcore.sql
-- adds killed_by column for hardcore hall of fame
--
-- note: player_db_schema.sql will also be updated with this
-- note: will skip if column already exists

-- killed_by stores who killed a hardcore character (for hall of fame display)
set @col_exists = (select count(*) from information_schema.columns
    where table_schema = database() and table_name = 'player_data' and column_name = 'killed_by');
set @sql = if(@col_exists = 0,
    'alter table player_data add column killed_by varchar(64) default null after numb_deaths',
    'select "killed_by already exists"');
prepare stmt from @sql;
execute stmt;
deallocate prepare stmt;
