-- ships tables

create table if not exists ships (
    id int unsigned auto_increment primary key,
    owner_pid int unsigned default null,
    owner_name varchar(64) not null unique,
    ship_name varchar(128) default null,
    ship_class tinyint unsigned default 0,
    frags int default 0,
    anchor_room int default 0,
    time_played int default 0,
    mainsail int default 0,
    race tinyint default 0,
    money int default 0,
    flags bigint unsigned default 0,
    armor_fore int default 0,
    armor_port int default 0,
    armor_rear int default 0,
    armor_star int default 0,
    internal_fore int default 0,
    internal_port int default 0,
    internal_rear int default 0,
    internal_star int default 0,
    crew_index int default 0,
    crew_sail_skill int default 0,
    crew_guns_skill int default 0,
    crew_rpar_skill int default 0,
    crew_sail_chief int default 0,
    crew_guns_chief int default 0,
    crew_rpar_chief int default 0,
    maxspeed_bonus int default 0,
    capacity_bonus int default 0,
    created_at timestamp default current_timestamp,
    updated_at timestamp default current_timestamp on update current_timestamp
) engine=innodb;

create index idx_ships_owner_pid on ships(owner_pid);

create table if not exists ship_slots (
    id int unsigned auto_increment primary key,
    ship_id int unsigned not null,
    slot_index tinyint not null,
    slot_type int not null default 0,
    item_index int not null default 0,
    position int not null default 0,
    timer int not null default 0,
    val0 int not null default 0,
    val1 int not null default 0,
    val2 int not null default 0,
    val3 int not null default 0,
    val4 int not null default 0,
    constraint fk_ship_slots_ship foreign key (ship_id) references ships(id) on delete cascade,
    unique key uk_ship_slots_index (ship_id, slot_index)
) engine=innodb;
