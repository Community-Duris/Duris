-- migration v11: remove players_core, add active column to player_data
-- this migration adds the active column to player_data and removes the players_core table

-- step 1: add active column to player_data if it doesn't exist
set @col_exists = (select count(*) from information_schema.columns
    where table_schema = database() and table_name = 'player_data' and column_name = 'active');

set @add_col = if(@col_exists = 0,
    'alter table player_data add column active tinyint(1) not null default 1 after last_ip',
    'select "active column already exists"');

prepare stmt from @add_col;
execute stmt;
deallocate prepare stmt;

-- step 2: set all existing players as active
update player_data set active = 1 where active is null or active = 0;

-- step 3: drop players_core table (optional - uncomment when ready)
-- drop table if exists players_core;
