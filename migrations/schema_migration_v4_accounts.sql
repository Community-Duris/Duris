-- account_characters pfile columns

alter table account_characters
add column login_count bigint unsigned default 0,
add column last_login bigint default 0,
add column blocked tinyint default 0,
add column racewar tinyint default 0;

create index idx_account_racewar on account_characters(account_name, racewar);
