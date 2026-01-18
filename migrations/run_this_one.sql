-- durismud pfile-to-db combined migration
-- run this on a fresh database or existing durismud website database
-- self-contained: creates all required tables
--
-- ============================================================================
-- safe for production: all operations use IF NOT EXISTS / conditional checks
-- to avoid data loss on existing databases. can run multiple times safely.
-- ============================================================================

-- ============================================================================
-- base tables - accounts and players
-- ============================================================================

CREATE TABLE IF NOT EXISTS accounts (
    account_name VARCHAR(50) NOT NULL,
    email VARCHAR(255) DEFAULT NULL,
    password VARCHAR(128) NOT NULL,
    confirmation_code VARCHAR(64) DEFAULT NULL,
    confirmed TINYINT(1) DEFAULT 0,
    confirmation_sent TINYINT(1) DEFAULT 0,
    blocked TINYINT(1) DEFAULT 0,
    last_login BIGINT DEFAULT 0,
    last_good_char BIGINT DEFAULT 0,
    last_evil_char BIGINT DEFAULT 0,
    flags1 BIGINT UNSIGNED DEFAULT 0,
    flags2 BIGINT UNSIGNED DEFAULT 0,
    flags3 BIGINT UNSIGNED DEFAULT 0,
    flags4 BIGINT UNSIGNED DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (account_name),
    INDEX idx_email (email)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS account_characters (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    account_name VARCHAR(50) NOT NULL,
    char_name VARCHAR(64) NOT NULL,
    pid INT UNSIGNED DEFAULT NULL,
    login_count BIGINT UNSIGNED DEFAULT 0,
    last_login BIGINT DEFAULT 0,
    blocked TINYINT DEFAULT 0,
    racewar TINYINT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (account_name) REFERENCES accounts(account_name) ON DELETE CASCADE,
    INDEX idx_account_name (account_name),
    INDEX idx_char_name (char_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS player_data (
    pid INT UNSIGNED NOT NULL AUTO_INCREMENT,
    name VARCHAR(64) NOT NULL,
    account_name VARCHAR(50) DEFAULT NULL,
    short_descr VARCHAR(512) DEFAULT NULL,
    long_descr TEXT DEFAULT NULL,
    description TEXT DEFAULT NULL,
    title VARCHAR(512) DEFAULT NULL,
    m_class INT UNSIGNED DEFAULT 0,
    secondary_class INT UNSIGNED DEFAULT 0,
    spec TINYINT UNSIGNED DEFAULT 0,
    race TINYINT UNSIGNED DEFAULT 0,
    racewar TINYINT UNSIGNED DEFAULT 0,
    level TINYINT UNSIGNED DEFAULT 1,
    sex TINYINT UNSIGNED DEFAULT 0,
    weight SMALLINT UNSIGNED DEFAULT 0,
    height SMALLINT UNSIGNED DEFAULT 0,
    size TINYINT DEFAULT 0,
    hometown INT DEFAULT 0,
    birthplace INT DEFAULT 0,
    orig_birthplace INT DEFAULT 0,
    last_room INT DEFAULT 0,
    birth_time BIGINT DEFAULT 0,
    played_time INT DEFAULT 0,
    last_save BIGINT DEFAULT 0,
    perm_aging SMALLINT DEFAULT 0,
    base_str TINYINT DEFAULT 0,
    base_dex TINYINT DEFAULT 0,
    base_agi TINYINT DEFAULT 0,
    base_con TINYINT DEFAULT 0,
    base_pow TINYINT DEFAULT 0,
    base_int TINYINT DEFAULT 0,
    base_wis TINYINT DEFAULT 0,
    base_cha TINYINT DEFAULT 0,
    base_kar TINYINT DEFAULT 0,
    base_luk TINYINT DEFAULT 0,
    mana INT DEFAULT 0,
    base_mana INT DEFAULT 0,
    hit_diff INT DEFAULT 0,
    base_hit INT DEFAULT 0,
    vitality INT DEFAULT 0,
    base_vitality INT DEFAULT 0,
    spells_memmed_extra TINYINT DEFAULT 0,
    copper BIGINT DEFAULT 0,
    silver BIGINT DEFAULT 0,
    gold BIGINT DEFAULT 0,
    platinum BIGINT DEFAULT 0,
    bank_copper BIGINT DEFAULT 0,
    bank_silver BIGINT DEFAULT 0,
    bank_gold BIGINT DEFAULT 0,
    bank_platinum BIGINT DEFAULT 0,
    exp BIGINT DEFAULT 0,
    epics BIGINT DEFAULT 0,
    epic_skill_points BIGINT DEFAULT 0,
    skillpoints INT DEFAULT 0,
    spell_bind_used BIGINT DEFAULT 0,
    act BIGINT UNSIGNED DEFAULT 0,
    act2 BIGINT UNSIGNED DEFAULT 0,
    act3 BIGINT UNSIGNED DEFAULT 0,
    vote BIGINT UNSIGNED DEFAULT 0,
    alignment INT DEFAULT 0,
    prestige SMALLINT DEFAULT 0,
    assoc_id SMALLINT UNSIGNED DEFAULT 0,
    guild_status INT UNSIGNED DEFAULT 0,
    time_left_guild BIGINT DEFAULT 0,
    nb_left_guild TINYINT DEFAULT 0,
    time_unspecced BIGINT DEFAULT 0,
    frags BIGINT DEFAULT 0,
    oldfrags BIGINT DEFAULT 0,
    numb_deaths BIGINT UNSIGNED DEFAULT 0,
    killed_by VARCHAR(64) DEFAULT NULL,
    condition_0 TINYINT DEFAULT 0,
    condition_1 TINYINT DEFAULT 0,
    condition_2 TINYINT DEFAULT 0,
    condition_3 TINYINT DEFAULT 0,
    condition_4 TINYINT DEFAULT 0,
    poof_in VARCHAR(512) DEFAULT NULL,
    poof_out VARCHAR(512) DEFAULT NULL,
    poof_in_sound VARCHAR(512) DEFAULT NULL,
    poof_out_sound VARCHAR(512) DEFAULT NULL,
    echo_toggle TINYINT UNSIGNED DEFAULT 0,
    prompt SMALLINT UNSIGNED DEFAULT 0,
    wiz_invis BIGINT DEFAULT 0,
    law_flags BIGINT UNSIGNED DEFAULT 0,
    wimpy SMALLINT DEFAULT 0,
    aggressive SMALLINT DEFAULT -1,
    highest_level TINYINT UNSIGNED DEFAULT 0,
    screen_length TINYINT UNSIGNED DEFAULT 24,
    quest_active INT DEFAULT 0,
    quest_mob_vnum INT DEFAULT 0,
    quest_type INT DEFAULT 0,
    quest_accomplished INT DEFAULT 0,
    quest_started INT DEFAULT 0,
    quest_zone_number INT DEFAULT 0,
    quest_giver INT DEFAULT 0,
    quest_level INT DEFAULT 0,
    quest_receiver INT DEFAULT 0,
    quest_shares_left INT DEFAULT 0,
    quest_kill_how_many INT DEFAULT 0,
    quest_kill_original INT DEFAULT 0,
    quest_map_room INT DEFAULT 0,
    quest_map_bought INT DEFAULT 0,
    last_ip BIGINT UNSIGNED DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (pid),
    INDEX idx_name (name),
    INDEX idx_account_name (account_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


-- ============================================================================
-- account related tables
-- ============================================================================

CREATE TABLE IF NOT EXISTS account_ips (
    id INT AUTO_INCREMENT PRIMARY KEY,
    account_name VARCHAR(50) NOT NULL,
    hostname VARCHAR(255),
    ip_address VARCHAR(45),
    count BIGINT UNSIGNED DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (account_name) REFERENCES accounts(account_name) ON DELETE CASCADE,
    INDEX idx_account_name (account_name),
    INDEX idx_ip_address (ip_address),
    UNIQUE KEY uk_account_ip (account_name, ip_address)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS towns (
    id INT AUTO_INCREMENT PRIMARY KEY,
    zone_filename VARCHAR(100) NOT NULL,
    resources INT DEFAULT 0,
    defense INT DEFAULT 0,
    offense INT DEFAULT 0,
    deploy_guard TINYINT DEFAULT 0,
    guard_vnum INT DEFAULT 0,
    guard_max INT DEFAULT 0,
    guard_load_room INT DEFAULT 0,
    deploy_cavalry TINYINT DEFAULT 0,
    cavalry_vnum INT DEFAULT 0,
    cavalry_max INT DEFAULT 0,
    cavalry_load_room INT DEFAULT 0,
    deploy_portals TINYINT DEFAULT 0,
    portal_vnum INT DEFAULT 0,
    portal_load_room INT DEFAULT 0,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY uk_zone_filename (zone_filename)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS kingdom_land (
    id INT AUTO_INCREMENT PRIMARY KEY,
    kingdom_id INT NOT NULL,  -- no fk: values from filesystem kingdom.land file, 0 = unowned territory
    start_vnum INT DEFAULT 0,
    end_vnum INT DEFAULT 0,
    type CHAR(1) DEFAULT 'r',
    INDEX idx_kingdom_id (kingdom_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS player_recipes (
    id INT AUTO_INCREMENT PRIMARY KEY,
    pid INT NOT NULL,
    recipe_vnum INT NOT NULL,
    learned_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_recipe (pid, recipe_vnum)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS player_shapechanges (
    id INT AUTO_INCREMENT PRIMARY KEY,
    pid INT NOT NULL,
    mob_vnum INT NOT NULL,
    times_researched INT DEFAULT 0,
    last_researched BIGINT DEFAULT 0,
    last_shapechanged BIGINT DEFAULT 0,
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_mob (pid, mob_vnum)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS corpses (
    id INT AUTO_INCREMENT PRIMARY KEY,
    player_name VARCHAR(50) NOT NULL,
    save_id BIGINT NOT NULL,
    room_vnum INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_player_name (player_name),
    UNIQUE KEY uk_player_saveid (player_name, save_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS shopkeepers (
    id INT AUTO_INCREMENT PRIMARY KEY,
    shop_id INT NOT NULL UNIQUE,
    mob_vnum INT DEFAULT 0,
    room_vnum INT DEFAULT 0,
    save_time BIGINT DEFAULT 0,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_shop_id (shop_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS shopkeeper_affects (
    id INT AUTO_INCREMENT PRIMARY KEY,
    shopkeeper_id INT NOT NULL,
    type INT DEFAULT 0,
    duration INT DEFAULT 0,
    modifier INT DEFAULT 0,
    location INT DEFAULT 0,
    bitvector1 BIGINT UNSIGNED DEFAULT 0,
    bitvector2 BIGINT UNSIGNED DEFAULT 0,
    bitvector3 BIGINT UNSIGNED DEFAULT 0,
    bitvector4 BIGINT UNSIGNED DEFAULT 0,
    bitvector5 BIGINT UNSIGNED DEFAULT 0,
    FOREIGN KEY (shopkeeper_id) REFERENCES shopkeepers(id) ON DELETE CASCADE,
    INDEX idx_shopkeeper_id (shopkeeper_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


-- ============================================================================
-- schema_lookup_tables.sql - race/class lookup tables
-- ============================================================================

-- lookup tables: use CREATE IF NOT EXISTS to preserve any existing data
-- these are populated by the game server, not this migration

CREATE TABLE IF NOT EXISTS races (
    id INT UNSIGNED PRIMARY KEY,
    name VARCHAR(64) NOT NULL,
    short_name VARCHAR(32),
    ansi_name VARCHAR(128),
    abbrev VARCHAR(4),
    racewar TINYINT DEFAULT 0 COMMENT '0=neutral, 1=good, 2=evil',
    playable TINYINT DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS classes (
    id INT UNSIGNED PRIMARY KEY,
    name VARCHAR(64) NOT NULL,
    ansi_name VARCHAR(128),
    short_name VARCHAR(8),
    menu_char CHAR(1)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE OR REPLACE VIEW players_view AS
SELECT
    pd.pid,
    pd.name,
    pd.level,
    pd.race as race_id,
    r.ansi_name as race,
    pd.m_class as class_id,
    c.ansi_name as classname,
    pd.racewar,
    pd.assoc_id,
    pd.exp,
    pd.epics,
    pd.played_time as playtime,
    (pd.copper + pd.silver*10 + pd.gold*100 + pd.platinum*1000) as money,
    (pd.bank_copper + pd.bank_silver*10 + pd.bank_gold*100 + pd.bank_platinum*1000) as balance
FROM player_data pd
LEFT JOIN races r ON pd.race = r.id
LEFT JOIN classes c ON pd.m_class = c.id;


-- ============================================================================
-- player array tables - skills, languages, etc
-- ============================================================================

CREATE TABLE IF NOT EXISTS player_skills (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    skill_id SMALLINT UNSIGNED NOT NULL,
    learned TINYINT UNSIGNED DEFAULT 0,
    taught TINYINT UNSIGNED DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_skill (pid, skill_id),
    CONSTRAINT fk_player_skills FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS player_languages (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    tongue_id TINYINT UNSIGNED NOT NULL,
    proficiency TINYINT UNSIGNED DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_tongue (pid, tongue_id),
    CONSTRAINT fk_player_languages FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS player_intros (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    intro_index TINYINT UNSIGNED NOT NULL,
    intro_pid INT DEFAULT 0,
    intro_time BIGINT UNSIGNED DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_intro (pid, intro_index),
    CONSTRAINT fk_player_intros FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS player_timers (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    timer_id TINYINT UNSIGNED NOT NULL,
    timer_value BIGINT DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_timer (pid, timer_id),
    CONSTRAINT fk_player_timers FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS player_undead_slots (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    circle TINYINT UNSIGNED NOT NULL,
    slots TINYINT DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_circle (pid, circle),
    CONSTRAINT fk_player_undead_slots FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS player_forged_items (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    forge_index SMALLINT UNSIGNED NOT NULL,
    item_vnum INT DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_forge (pid, forge_index),
    CONSTRAINT fk_player_forged_items FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS player_granted_cmds (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    cmd_num INT NOT NULL,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_cmd (pid, cmd_num),
    CONSTRAINT fk_player_granted_cmds FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


-- ============================================================================
-- player affects and items
-- ============================================================================

CREATE TABLE IF NOT EXISTS player_affects (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    type SMALLINT NOT NULL,
    duration INT DEFAULT 0,
    flags SMALLINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    location TINYINT UNSIGNED DEFAULT 0,
    level SMALLINT UNSIGNED DEFAULT 0,
    bitvector1 BIGINT DEFAULT 0,
    bitvector2 BIGINT DEFAULT 0,
    bitvector3 BIGINT DEFAULT 0,
    bitvector4 BIGINT DEFAULT 0,
    bitvector5 BIGINT DEFAULT 0,
    custom_msg_char TEXT DEFAULT NULL,
    custom_msg_room TEXT DEFAULT NULL,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    CONSTRAINT fk_player_affects FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS player_items (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    vnum INT NOT NULL,
    equip_slot TINYINT DEFAULT 0,
    container_id INT UNSIGNED DEFAULT NULL,
    quantity SMALLINT UNSIGNED DEFAULT 1,
    weight INT DEFAULT 0,
    cost INT DEFAULT 0,
    timer INT DEFAULT -1,
    extra_flags BIGINT UNSIGNED DEFAULT 0,
    value0 INT DEFAULT 0,
    value1 INT DEFAULT 0,
    value2 INT DEFAULT 0,
    value3 INT DEFAULT 0,
    value4 INT DEFAULT 0,
    value5 INT DEFAULT 0,
    value6 INT DEFAULT 0,
    value7 INT DEFAULT 0,
    name VARCHAR(512) DEFAULT NULL,
    short_descr VARCHAR(512) DEFAULT NULL,
    description TEXT DEFAULT NULL,
    action_descr TEXT DEFAULT NULL,
    unique_id INT UNSIGNED DEFAULT NULL,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    INDEX idx_container_id (container_id),
    CONSTRAINT fk_player_items FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE,
    CONSTRAINT fk_player_items_container FOREIGN KEY (container_id) REFERENCES player_items(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS player_item_affects (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_item_id (item_id),
    CONSTRAINT fk_player_item_affects FOREIGN KEY (item_id) REFERENCES player_items(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS player_witnesses (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    crime TINYINT UNSIGNED DEFAULT 0,
    room_vnum INT DEFAULT 0,
    attacker_name VARCHAR(64) DEFAULT NULL,
    victim_name VARCHAR(64) DEFAULT NULL,
    witness_time BIGINT DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    CONSTRAINT fk_player_witnesses FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS player_spellbooks (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    mob_vnum INT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_mob (pid, mob_vnum),
    CONSTRAINT fk_player_spellbooks FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


-- ============================================================================
-- schema_migration_v2.sql - normalized item storage (no blobs)
-- ============================================================================

-- item tables: use CREATE IF NOT EXISTS to preserve existing player data
-- never drop these tables as they contain saved equipment/items

CREATE TABLE IF NOT EXISTS corpse_items (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    corpse_id INT NOT NULL,
    vnum INT NOT NULL,
    container_id INT UNSIGNED DEFAULT NULL,
    quantity SMALLINT UNSIGNED DEFAULT 1,
    weight INT DEFAULT 0,
    cost INT DEFAULT 0,
    timer INT DEFAULT -1,
    extra_flags BIGINT UNSIGNED DEFAULT 0,
    value0 INT DEFAULT 0,
    value1 INT DEFAULT 0,
    value2 INT DEFAULT 0,
    value3 INT DEFAULT 0,
    value4 INT DEFAULT 0,
    value5 INT DEFAULT 0,
    value6 INT DEFAULT 0,
    value7 INT DEFAULT 0,
    name VARCHAR(512) DEFAULT NULL,
    short_descr VARCHAR(512) DEFAULT NULL,
    description TEXT DEFAULT NULL,
    action_descr TEXT DEFAULT NULL,
    unique_id INT UNSIGNED DEFAULT NULL,
    FOREIGN KEY (corpse_id) REFERENCES corpses(id) ON DELETE CASCADE,
    FOREIGN KEY (container_id) REFERENCES corpse_items(id) ON DELETE CASCADE,
    INDEX idx_corpse_id (corpse_id),
    INDEX idx_vnum (vnum)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS corpse_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES corpse_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS shopkeeper_items (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    shopkeeper_id INT NOT NULL,
    vnum INT NOT NULL,
    equip_slot TINYINT DEFAULT 0,
    container_id INT UNSIGNED DEFAULT NULL,
    quantity SMALLINT UNSIGNED DEFAULT 1,
    weight INT DEFAULT 0,
    cost INT DEFAULT 0,
    timer INT DEFAULT -1,
    extra_flags BIGINT UNSIGNED DEFAULT 0,
    value0 INT DEFAULT 0,
    value1 INT DEFAULT 0,
    value2 INT DEFAULT 0,
    value3 INT DEFAULT 0,
    value4 INT DEFAULT 0,
    value5 INT DEFAULT 0,
    value6 INT DEFAULT 0,
    value7 INT DEFAULT 0,
    name VARCHAR(512) DEFAULT NULL,
    short_descr VARCHAR(512) DEFAULT NULL,
    description TEXT DEFAULT NULL,
    action_descr TEXT DEFAULT NULL,
    unique_id INT UNSIGNED DEFAULT NULL,
    FOREIGN KEY (shopkeeper_id) REFERENCES shopkeepers(id) ON DELETE CASCADE,
    FOREIGN KEY (container_id) REFERENCES shopkeeper_items(id) ON DELETE CASCADE,
    INDEX idx_shopkeeper_id (shopkeeper_id),
    INDEX idx_vnum (vnum)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS shopkeeper_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES shopkeeper_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS saved_items (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_key VARCHAR(100) NOT NULL UNIQUE,
    room_vnum INT DEFAULT 0,
    vnum INT NOT NULL,
    container_id INT UNSIGNED DEFAULT NULL,
    quantity SMALLINT UNSIGNED DEFAULT 1,
    weight INT DEFAULT 0,
    cost INT DEFAULT 0,
    timer INT DEFAULT -1,
    extra_flags BIGINT UNSIGNED DEFAULT 0,
    value0 INT DEFAULT 0,
    value1 INT DEFAULT 0,
    value2 INT DEFAULT 0,
    value3 INT DEFAULT 0,
    value4 INT DEFAULT 0,
    value5 INT DEFAULT 0,
    value6 INT DEFAULT 0,
    value7 INT DEFAULT 0,
    name VARCHAR(512) DEFAULT NULL,
    short_descr VARCHAR(512) DEFAULT NULL,
    description TEXT DEFAULT NULL,
    action_descr TEXT DEFAULT NULL,
    unique_id INT UNSIGNED DEFAULT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (container_id) REFERENCES saved_items(id) ON DELETE CASCADE,
    INDEX idx_room_vnum (room_vnum),
    INDEX idx_vnum (vnum)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS saved_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES saved_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS siege_items (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    room_vnum INT NOT NULL,
    vnum INT NOT NULL,
    container_id INT UNSIGNED DEFAULT NULL,
    quantity SMALLINT UNSIGNED DEFAULT 1,
    weight INT DEFAULT 0,
    cost INT DEFAULT 0,
    timer INT DEFAULT -1,
    extra_flags BIGINT UNSIGNED DEFAULT 0,
    value0 INT DEFAULT 0,
    value1 INT DEFAULT 0,
    value2 INT DEFAULT 0,
    value3 INT DEFAULT 0,
    value4 INT DEFAULT 0,
    value5 INT DEFAULT 0,
    value6 INT DEFAULT 0,
    value7 INT DEFAULT 0,
    name VARCHAR(512) DEFAULT NULL,
    short_descr VARCHAR(512) DEFAULT NULL,
    description TEXT DEFAULT NULL,
    action_descr TEXT DEFAULT NULL,
    unique_id INT UNSIGNED DEFAULT NULL,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (container_id) REFERENCES siege_items(id) ON DELETE CASCADE,
    INDEX idx_room_vnum (room_vnum),
    INDEX idx_vnum (vnum)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS siege_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES siege_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


-- ============================================================================
-- schema_migration_v3_lockers.sql - locker tables
-- ============================================================================

CREATE TABLE IF NOT EXISTS lockers (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_name VARCHAR(100) NOT NULL UNIQUE,
    owner_pid INT DEFAULT NULL,
    owner_assoc_id INT DEFAULT NULL,
    racewar TINYINT DEFAULT 0,
    race TINYINT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_owner_pid (owner_pid),
    INDEX idx_owner_assoc_id (owner_assoc_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS locker_items (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    vnum INT NOT NULL,
    container_id INT UNSIGNED DEFAULT NULL,
    quantity SMALLINT UNSIGNED DEFAULT 1,
    weight INT DEFAULT 0,
    cost INT DEFAULT 0,
    timer INT DEFAULT -1,
    extra_flags BIGINT UNSIGNED DEFAULT 0,
    value0 INT DEFAULT 0,
    value1 INT DEFAULT 0,
    value2 INT DEFAULT 0,
    value3 INT DEFAULT 0,
    value4 INT DEFAULT 0,
    value5 INT DEFAULT 0,
    value6 INT DEFAULT 0,
    value7 INT DEFAULT 0,
    name VARCHAR(512) DEFAULT NULL,
    short_descr VARCHAR(512) DEFAULT NULL,
    description TEXT DEFAULT NULL,
    action_descr TEXT DEFAULT NULL,
    unique_id INT UNSIGNED DEFAULT NULL,
    FOREIGN KEY (locker_id) REFERENCES lockers(id) ON DELETE CASCADE,
    FOREIGN KEY (container_id) REFERENCES locker_items(id) ON DELETE CASCADE,
    INDEX idx_locker_id (locker_id),
    INDEX idx_vnum (vnum)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS locker_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES locker_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


-- ============================================================================
-- schema_migration_v4_accounts.sql - account_characters columns
-- ============================================================================

-- add columns to account_characters if they don't exist (for existing databases)
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'login_count');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN login_count BIGINT UNSIGNED DEFAULT 0',
    'SELECT "login_count already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'last_login');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN last_login BIGINT DEFAULT 0',
    'SELECT "last_login already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'blocked');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN blocked TINYINT DEFAULT 0',
    'SELECT "blocked already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'racewar');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN racewar TINYINT DEFAULT 0',
    'SELECT "racewar already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND index_name = 'idx_account_racewar');
SET @sql = IF(@idx_exists = 0,
    'CREATE INDEX idx_account_racewar ON account_characters(account_name, racewar)',
    'SELECT "idx_account_racewar already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;


-- ============================================================================
-- schema_migration_v5_ships.sql - ship tables
-- ============================================================================

CREATE TABLE IF NOT EXISTS ships (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    owner_pid INT UNSIGNED DEFAULT NULL,
    owner_name VARCHAR(64) NOT NULL UNIQUE,
    ship_name VARCHAR(128) DEFAULT NULL,
    ship_class TINYINT UNSIGNED DEFAULT 0,
    frags INT DEFAULT 0,
    anchor_room INT DEFAULT 0,
    time_played INT DEFAULT 0,
    mainsail INT DEFAULT 0,
    race TINYINT DEFAULT 0,
    money INT DEFAULT 0,
    flags BIGINT UNSIGNED DEFAULT 0,
    armor_fore INT DEFAULT 0,
    armor_port INT DEFAULT 0,
    armor_rear INT DEFAULT 0,
    armor_star INT DEFAULT 0,
    internal_fore INT DEFAULT 0,
    internal_port INT DEFAULT 0,
    internal_rear INT DEFAULT 0,
    internal_star INT DEFAULT 0,
    crew_index INT DEFAULT 0,
    crew_sail_skill INT DEFAULT 0,
    crew_guns_skill INT DEFAULT 0,
    crew_rpar_skill INT DEFAULT 0,
    crew_sail_chief INT DEFAULT 0,
    crew_guns_chief INT DEFAULT 0,
    crew_rpar_chief INT DEFAULT 0,
    maxspeed_bonus INT DEFAULT 0,
    capacity_bonus INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'ships' AND index_name = 'idx_ships_owner_pid');
SET @sql = IF(@idx_exists = 0,
    'CREATE INDEX idx_ships_owner_pid ON ships(owner_pid)',
    'SELECT "idx_ships_owner_pid already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

CREATE TABLE IF NOT EXISTS ship_slots (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    ship_id INT UNSIGNED NOT NULL,
    slot_index TINYINT NOT NULL,
    slot_type INT NOT NULL DEFAULT 0,
    item_index INT NOT NULL DEFAULT 0,
    position INT NOT NULL DEFAULT 0,
    timer INT NOT NULL DEFAULT 0,
    val0 INT NOT NULL DEFAULT 0,
    val1 INT NOT NULL DEFAULT 0,
    val2 INT NOT NULL DEFAULT 0,
    val3 INT NOT NULL DEFAULT 0,
    val4 INT NOT NULL DEFAULT 0,
    CONSTRAINT fk_ship_slots_ship FOREIGN KEY (ship_id) REFERENCES ships(id) ON DELETE CASCADE,
    UNIQUE KEY uk_ship_slots_index (ship_id, slot_index)
) ENGINE=InnoDB;


-- ============================================================================
-- schema_migration_v6_guilds.sql - guild tables
-- ============================================================================

CREATE TABLE IF NOT EXISTS guilds (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    guild_id INT UNSIGNED NOT NULL UNIQUE,
    name VARCHAR(100) NOT NULL,
    racewar INT UNSIGNED NOT NULL DEFAULT 0,
    bits INT UNSIGNED NOT NULL DEFAULT 0,
    prestige BIGINT UNSIGNED NOT NULL DEFAULT 0,
    construction BIGINT UNSIGNED NOT NULL DEFAULT 0,
    platinum INT UNSIGNED NOT NULL DEFAULT 0,
    gold INT UNSIGNED NOT NULL DEFAULT 0,
    silver INT UNSIGNED NOT NULL DEFAULT 0,
    copper INT UNSIGNED NOT NULL DEFAULT 0,
    total_frags BIGINT NOT NULL DEFAULT 0,
    top_frags BIGINT NOT NULL DEFAULT 0,
    top_fragger VARCHAR(50) NOT NULL DEFAULT '',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS guild_ranks (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    guild_id INT UNSIGNED NOT NULL,
    rank_index TINYINT NOT NULL,
    title VARCHAR(100) NOT NULL DEFAULT '',
    CONSTRAINT fk_guild_ranks_guild FOREIGN KEY (guild_id) REFERENCES guilds(id) ON DELETE CASCADE,
    UNIQUE KEY uk_guild_ranks_index (guild_id, rank_index)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS guild_members (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    guild_id INT UNSIGNED NOT NULL,
    player_name VARCHAR(64) NOT NULL,
    player_pid INT UNSIGNED DEFAULT NULL,
    bits INT UNSIGNED NOT NULL DEFAULT 0,
    debt INT UNSIGNED NOT NULL DEFAULT 0,
    online_status TINYINT NOT NULL DEFAULT 0,
    CONSTRAINT fk_guild_members_guild FOREIGN KEY (guild_id) REFERENCES guilds(id) ON DELETE CASCADE,
    UNIQUE KEY uk_guild_members_name (guild_id, player_name)
) ENGINE=InnoDB;

-- add online_status column if missing (for existing databases)
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'guild_members' AND column_name = 'online_status');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE guild_members ADD COLUMN online_status TINYINT NOT NULL DEFAULT 0',
    'SELECT "online_status already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'guild_members' AND index_name = 'idx_guild_members_name');
SET @sql = IF(@idx_exists = 0,
    'CREATE INDEX idx_guild_members_name ON guild_members(player_name)',
    'SELECT "idx_guild_members_name already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;


-- ============================================================================
-- schema_migration_v7_player_fixes.sql - player_data columns
-- ============================================================================

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_data' AND column_name = 'act3');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_data ADD COLUMN act3 BIGINT UNSIGNED DEFAULT 0 AFTER act2',
    'SELECT "act3 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_data' AND column_name = 'last_room');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_data ADD COLUMN last_room INT DEFAULT 0 AFTER orig_birthplace',
    'SELECT "last_room already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;


-- ============================================================================
-- schema_migration_v8_unique.sql - unique constraints for char names
-- ============================================================================

-- clean up duplicates first (keep lowest pid)
DELETE ac1 FROM account_characters ac1
INNER JOIN account_characters ac2
WHERE ac1.char_name = ac2.char_name
  AND ac1.pid > ac2.pid;

-- add unique constraint on account_characters.char_name (if not exists)
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE()
    AND table_name = 'account_characters'
    AND index_name = 'idx_char_name_unique');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE account_characters ADD UNIQUE INDEX idx_char_name_unique (char_name)',
    'SELECT "idx_char_name_unique already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- add unique constraint on player_data.name (if not exists)
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE()
    AND table_name = 'player_data'
    AND index_name = 'idx_player_name_unique');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_data ADD UNIQUE INDEX idx_player_name_unique (name)',
    'SELECT "idx_player_name_unique already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;


-- ============================================================================
-- schema_migration_v8_hardcore.sql - hardcore hall of fame
-- ============================================================================

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_data' AND column_name = 'killed_by');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_data ADD COLUMN killed_by VARCHAR(64) DEFAULT NULL AFTER numb_deaths',
    'SELECT "killed_by already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;


-- ============================================================================
-- schema_migration_v9_dirty_saves.sql - unique keys for upsert pattern
-- ============================================================================

-- player_languages: uk_pid_tongue
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_languages' AND index_name = 'uk_pid_tongue');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_languages ADD UNIQUE KEY uk_pid_tongue (pid, tongue_id)',
    'SELECT "uk_pid_tongue already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- player_intros: uk_pid_intro
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_intros' AND index_name = 'uk_pid_intro');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_intros ADD UNIQUE KEY uk_pid_intro (pid, intro_index)',
    'SELECT "uk_pid_intro already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- player_timers: uk_pid_timer
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_timers' AND index_name = 'uk_pid_timer');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_timers ADD UNIQUE KEY uk_pid_timer (pid, timer_id)',
    'SELECT "uk_pid_timer already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- player_undead_slots: uk_pid_circle
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_undead_slots' AND index_name = 'uk_pid_circle');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_undead_slots ADD UNIQUE KEY uk_pid_circle (pid, circle)',
    'SELECT "uk_pid_circle already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- player_forged_items: uk_pid_forge
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_forged_items' AND index_name = 'uk_pid_forge');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_forged_items ADD UNIQUE KEY uk_pid_forge (pid, forge_index)',
    'SELECT "uk_pid_forge already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- player_granted_cmds: uk_pid_cmd
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_granted_cmds' AND index_name = 'uk_pid_cmd');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_granted_cmds ADD UNIQUE KEY uk_pid_cmd (pid, cmd_num)',
    'SELECT "uk_pid_cmd already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- player_skills: uk_pid_skill
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_skills' AND index_name = 'uk_pid_skill');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_skills ADD UNIQUE KEY uk_pid_skill (pid, skill_id)',
    'SELECT "uk_pid_skill already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;


-- ============================================================================
-- schema_migration_v10_pets.sql - pet persistence
-- ============================================================================

CREATE TABLE IF NOT EXISTS `player_pets` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `owner_pid` INT UNSIGNED NOT NULL,
  `mob_vnum` INT NOT NULL,
  `pet_order` TINYINT DEFAULT 0,
  `hit` INT DEFAULT 0,
  `max_hit` INT DEFAULT 0,
  `mana` INT DEFAULT 0,
  `max_mana` INT DEFAULT 0,
  `vitality` INT DEFAULT 0,
  `max_vitality` INT DEFAULT 0,
  `charm_duration` INT DEFAULT -1,
  `room_vnum` INT DEFAULT 0,
  `saved_at` BIGINT DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `idx_owner_pid` (`owner_pid`),
  CONSTRAINT `fk_player_pets_owner` FOREIGN KEY (`owner_pid`)
    REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `player_pet_items` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `pet_id` INT UNSIGNED NOT NULL,
  `vnum` INT NOT NULL,
  `equip_slot` TINYINT DEFAULT 0,
  `container_id` INT UNSIGNED DEFAULT NULL,
  `weight` INT DEFAULT 0,
  `cost` INT DEFAULT 0,
  `timer` INT DEFAULT -1,
  `extra_flags` BIGINT UNSIGNED DEFAULT 0,
  `value0` INT DEFAULT 0,
  `value1` INT DEFAULT 0,
  `value2` INT DEFAULT 0,
  `value3` INT DEFAULT 0,
  `value4` INT DEFAULT 0,
  `value5` INT DEFAULT 0,
  `value6` INT DEFAULT 0,
  `value7` INT DEFAULT 0,
  `name` VARCHAR(512) DEFAULT NULL,
  `short_descr` VARCHAR(512) DEFAULT NULL,
  `description` TEXT DEFAULT NULL,
  `action_descr` TEXT DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_pet_id` (`pet_id`),
  KEY `idx_container_id` (`container_id`),
  CONSTRAINT `fk_pet_items_pet` FOREIGN KEY (`pet_id`)
    REFERENCES `player_pets` (`id`) ON DELETE CASCADE,
  CONSTRAINT `fk_pet_items_container` FOREIGN KEY (`container_id`)
    REFERENCES `player_pet_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `player_pet_item_affects` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `item_id` INT UNSIGNED NOT NULL,
  `location` TINYINT UNSIGNED DEFAULT 0,
  `modifier` INT DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `fk_pet_item_affects` FOREIGN KEY (`item_id`)
    REFERENCES `player_pet_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


-- ============================================================================
-- epic-zone-payout.sql - zone payout data (optional)
-- ============================================================================

-- only run zone updates if zones table exists (may not exist on fresh install)
DROP PROCEDURE IF EXISTS update_zone_payouts;

DELIMITER //
CREATE PROCEDURE update_zone_payouts()
BEGIN
    DECLARE tbl_exists INT DEFAULT 0;
    SELECT COUNT(*) INTO tbl_exists FROM information_schema.tables
        WHERE table_schema = DATABASE() AND table_name = 'zones';

    IF tbl_exists > 0 THEN
        -- set group size to 100 to disable group size penalty until tuned
        UPDATE zones SET suggested_group_size = 100 WHERE epic_type != '0';

        -- zone payout values
        UPDATE zones SET epic_payout = 0 WHERE number = 1389;
        UPDATE zones SET epic_payout = 80 WHERE number IN (400, 93, 740, 14, 90, 383);
        UPDATE zones SET epic_payout = 90 WHERE number IN (264, 140, 823, 370, 38, 879, 113, 143);
        UPDATE zones SET epic_payout = 100 WHERE number IN (191, 342, 285, 67, 381, 27, 429, 805, 133, 183, 130, 666, 1320, 220, 755);
        UPDATE zones SET epic_payout = 110 WHERE number IN (758, 73, 824, 662, 664);
        UPDATE zones SET epic_payout = 120 WHERE number IN (430, 773, 490, 710);
        UPDATE zones SET epic_payout = 130 WHERE number IN (200, 766);
        UPDATE zones SET epic_payout = 150 WHERE number IN (760, 570, 91);
        UPDATE zones SET epic_payout = 175 WHERE number IN (318, 50);
        UPDATE zones SET epic_payout = 200 WHERE number IN (970, 920, 213);
        UPDATE zones SET epic_payout = 225 WHERE number IN (24, 244, 254, 197);
        UPDATE zones SET epic_payout = 250 WHERE number IN (151, 780, 412);
        UPDATE zones SET epic_payout = 260 WHERE number IN (87, 368);
        UPDATE zones SET epic_payout = 275 WHERE number IN (35, 448, 756, 261);
        UPDATE zones SET epic_payout = 285 WHERE number IN (419, 162);
        UPDATE zones SET epic_payout = 300 WHERE number IN (709, 238, 124);
        UPDATE zones SET epic_payout = 315 WHERE number IN (784, 831);
        UPDATE zones SET epic_payout = 325 WHERE number IN (386, 229, 289, 960);
        UPDATE zones SET epic_payout = 335 WHERE number = 441;
        UPDATE zones SET epic_payout = 345 WHERE number = 215;
        UPDATE zones SET epic_payout = 350 WHERE number IN (989, 315, 367, 1200, 1398, 232);
        UPDATE zones SET epic_payout = 400 WHERE number IN (328, 159, 435, 712, 326);
        UPDATE zones SET epic_payout = 425 WHERE number IN (910, 877, 777);
        UPDATE zones SET epic_payout = 450 WHERE number IN (883, 1316);
        UPDATE zones SET epic_payout = 500 WHERE number IN (814, 230, 1390);
        UPDATE zones SET epic_payout = 550 WHERE number IN (444, 588, 1424);
        UPDATE zones SET epic_payout = 600 WHERE number IN (1300, 68);
        UPDATE zones SET epic_payout = 650 WHERE number = 196;
        UPDATE zones SET epic_payout = 700 WHERE number IN (345, 257);
        UPDATE zones SET epic_payout = 800 WHERE number IN (266, 324);
        UPDATE zones SET epic_payout = 850 WHERE number = 4200;
        UPDATE zones SET epic_payout = 900 WHERE number IN (387, 455);
        UPDATE zones SET epic_payout = 950 WHERE number = 875;
        UPDATE zones SET epic_payout = 1000 WHERE number = 583;
    END IF;
END //
DELIMITER ;

CALL update_zone_payouts();
DROP PROCEDURE IF EXISTS update_zone_payouts;


-- ============================================================================
-- schema_migration_v11_extra_descr.sql - item extra descriptions (spellbooks)
-- ============================================================================

CREATE TABLE IF NOT EXISTS player_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_player_item_ed FOREIGN KEY (item_id)
    REFERENCES player_items(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS player_pet_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_pet_item_ed FOREIGN KEY (item_id)
    REFERENCES player_pet_items(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;


-- ============================================================================
-- done
-- ============================================================================
