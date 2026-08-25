-- guilds tables

create table if not exists guilds (
    id int unsigned auto_increment primary key,
    guild_id int unsigned not null unique,
    name varchar(100) not null,
    racewar int unsigned not null default 0,
    bits int unsigned not null default 0,
    prestige bigint unsigned not null default 0,
    construction bigint unsigned not null default 0,
    platinum int unsigned not null default 0,
    gold int unsigned not null default 0,
    silver int unsigned not null default 0,
    copper int unsigned not null default 0,
    total_frags bigint not null default 0,
    top_frags bigint not null default 0,
    top_fragger varchar(50) not null default '',
    created_at timestamp default current_timestamp,
    updated_at timestamp default current_timestamp on update current_timestamp
) engine=innodb;

create table if not exists guild_ranks (
    id int unsigned auto_increment primary key,
    guild_id int unsigned not null,
    rank_index tinyint not null,
    title varchar(100) not null default '',
    constraint fk_guild_ranks_guild foreign key (guild_id) references guilds(id) on delete cascade,
    unique key uk_guild_ranks_index (guild_id, rank_index)
) engine=innodb;

create table if not exists guild_members (
    id int unsigned auto_increment primary key,
    guild_id int unsigned not null,
    member_name varchar(50) not null,
    bits int unsigned not null default 0,
    debt int unsigned not null default 0,
    online_status tinyint not null default 0,
    constraint fk_guild_members_guild foreign key (guild_id) references guilds(id) on delete cascade,
    unique key uk_guild_members_name (guild_id, member_name)
) engine=innodb;

create index idx_guild_members_name on guild_members(member_name);
