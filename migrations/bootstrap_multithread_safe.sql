-- Safe consolidated DurisMUD migration for new or existing databases
-- This file is idempotent and non-destructive:
-- it uses information_schema guards and idempotent DDL where supported, and it
-- avoids DROP TABLE, DROP DATABASE, TRUNCATE, and column-drop operations.

-- ============================================================================
-- FILE: run_this_one.sql
-- ============================================================================
-- durismud pfile-to-db combined migration
-- safe for production, idempotent, run multiple times safely

-- accounts and players

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
);

CREATE TABLE IF NOT EXISTS account_characters (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    account_name VARCHAR(50) NOT NULL,
    char_name VARCHAR(64) NOT NULL,
    pid INT UNSIGNED DEFAULT NULL,
    login_count BIGINT UNSIGNED DEFAULT 0,
    last_login BIGINT DEFAULT 0,
    blocked TINYINT DEFAULT 0,
    racewar TINYINT DEFAULT 0,
    deleted_at DATETIME DEFAULT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (account_name) REFERENCES accounts(account_name) ON DELETE CASCADE,
    UNIQUE KEY idx_char_name_unique (char_name),
    UNIQUE KEY acct_char (account_name, char_name),
    INDEX idx_account_name (account_name),
    INDEX idx_char_name (char_name),
    INDEX account_active (account_name, deleted_at)
);

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
    active TINYINT(1) NOT NULL DEFAULT 1,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (pid),
    INDEX idx_name (name),
    INDEX idx_account_name (account_name)
);


-- account related

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
);

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
);

CREATE TABLE IF NOT EXISTS kingdom_land (
    id INT AUTO_INCREMENT PRIMARY KEY,
    kingdom_id INT NOT NULL,  -- no fk, comes from filesystem
    start_vnum INT DEFAULT 0,
    end_vnum INT DEFAULT 0,
    type CHAR(1) DEFAULT 'r',
    INDEX idx_kingdom_id (kingdom_id)
);

CREATE TABLE IF NOT EXISTS player_recipes (
    id INT AUTO_INCREMENT PRIMARY KEY,
    pid INT NOT NULL,
    recipe_vnum INT NOT NULL,
    learned_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_recipe (pid, recipe_vnum)
);

CREATE TABLE IF NOT EXISTS player_shapechanges (
    id INT AUTO_INCREMENT PRIMARY KEY,
    pid INT NOT NULL,
    mob_vnum INT NOT NULL,
    times_researched INT DEFAULT 0,
    last_researched BIGINT DEFAULT 0,
    last_shapechanged BIGINT DEFAULT 0,
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_mob (pid, mob_vnum)
);

CREATE TABLE IF NOT EXISTS corpses (
    id INT AUTO_INCREMENT PRIMARY KEY,
    player_name VARCHAR(50) NOT NULL,
    save_id BIGINT NOT NULL,
    room_vnum INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
	short_descr VARCHAR(512) DEFAULT NULL,
    description TEXT DEFAULT NULL,
    INDEX idx_player_name (player_name),
    UNIQUE KEY uk_player_saveid (player_name, save_id)
);

CREATE TABLE IF NOT EXISTS shopkeepers (
    id INT AUTO_INCREMENT PRIMARY KEY,
    shop_id INT NOT NULL UNIQUE,
    mob_vnum INT DEFAULT 0,
    room_vnum INT DEFAULT 0,
    save_time BIGINT DEFAULT 0,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_shop_id (shop_id)
);

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
);


-- race/class lookups (populated by game server)

CREATE TABLE IF NOT EXISTS races (
    id INT UNSIGNED PRIMARY KEY,
    name VARCHAR(64) NOT NULL,
    short_name VARCHAR(32),
    ansi_name VARCHAR(128),
    abbrev VARCHAR(4),
    racewar TINYINT DEFAULT 0,
    playable TINYINT DEFAULT 0
);

CREATE TABLE IF NOT EXISTS classes (
    id INT UNSIGNED PRIMARY KEY,
    name VARCHAR(64) NOT NULL,
    ansi_name VARCHAR(128),
    short_name VARCHAR(8),
    menu_char CHAR(1)
);

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


-- player arrays

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
);

CREATE TABLE IF NOT EXISTS player_languages (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    tongue_id TINYINT UNSIGNED NOT NULL,
    proficiency TINYINT UNSIGNED DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_tongue (pid, tongue_id),
    CONSTRAINT fk_player_languages FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
);

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
);

CREATE TABLE IF NOT EXISTS player_timers (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    timer_id TINYINT UNSIGNED NOT NULL,
    timer_value BIGINT DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_timer (pid, timer_id),
    CONSTRAINT fk_player_timers FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS player_undead_slots (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    circle TINYINT UNSIGNED NOT NULL,
    slots TINYINT DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_circle (pid, circle),
    CONSTRAINT fk_player_undead_slots FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS player_forged_items (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    forge_index SMALLINT UNSIGNED NOT NULL,
    item_vnum INT DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_forge (pid, forge_index),
    CONSTRAINT fk_player_forged_items FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS player_granted_cmds (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    cmd_num INT NOT NULL,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_cmd (pid, cmd_num),
    CONSTRAINT fk_player_granted_cmds FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
);


-- player affects and items

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
);

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
    wear_flags INT DEFAULT NULL,
    item_type TINYINT DEFAULT NULL,
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
    bitvector1 BIGINT UNSIGNED DEFAULT NULL,
    bitvector2 BIGINT UNSIGNED DEFAULT NULL,
    bitvector3 BIGINT UNSIGNED DEFAULT NULL,
    bitvector4 BIGINT UNSIGNED DEFAULT NULL,
    bitvector5 BIGINT UNSIGNED DEFAULT NULL,
    item_material TINYINT DEFAULT NULL,
    obj_uid BIGINT UNSIGNED DEFAULT NULL,
    item_condition SMALLINT DEFAULT 100,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    INDEX idx_container_id (container_id),
    INDEX idx_obj_uid (obj_uid),
    CONSTRAINT fk_player_items FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE,
    CONSTRAINT fk_player_items_container FOREIGN KEY (container_id) REFERENCES player_items(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS player_item_affects (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_item_id (item_id),
    CONSTRAINT fk_player_item_affects FOREIGN KEY (item_id) REFERENCES player_items(id) ON DELETE CASCADE
);

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
);

CREATE TABLE IF NOT EXISTS player_spellbooks (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    mob_vnum INT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_mob (pid, mob_vnum),
    CONSTRAINT fk_player_spellbooks FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
);


-- item storage

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
    wear_flags INT DEFAULT NULL,
    item_type TINYINT DEFAULT NULL,
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
    bitvector1 BIGINT UNSIGNED DEFAULT NULL,
    bitvector2 BIGINT UNSIGNED DEFAULT NULL,
    bitvector3 BIGINT UNSIGNED DEFAULT NULL,
    bitvector4 BIGINT UNSIGNED DEFAULT NULL,
    bitvector5 BIGINT UNSIGNED DEFAULT NULL,
    item_material TINYINT DEFAULT NULL,
    obj_uid BIGINT UNSIGNED DEFAULT NULL,
    item_condition SMALLINT DEFAULT 100,
    FOREIGN KEY (corpse_id) REFERENCES corpses(id) ON DELETE CASCADE,
    FOREIGN KEY (container_id) REFERENCES corpse_items(id) ON DELETE CASCADE,
    INDEX idx_corpse_id (corpse_id),
    INDEX idx_vnum (vnum),
    INDEX idx_obj_uid (obj_uid)
);

CREATE TABLE IF NOT EXISTS corpse_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES corpse_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
);

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
    wear_flags INT DEFAULT NULL,
    item_type TINYINT DEFAULT NULL,
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
    bitvector1 BIGINT UNSIGNED DEFAULT NULL,
    bitvector2 BIGINT UNSIGNED DEFAULT NULL,
    bitvector3 BIGINT UNSIGNED DEFAULT NULL,
    bitvector4 BIGINT UNSIGNED DEFAULT NULL,
    bitvector5 BIGINT UNSIGNED DEFAULT NULL,
    item_material TINYINT DEFAULT NULL,
    obj_uid BIGINT UNSIGNED DEFAULT NULL,
    FOREIGN KEY (shopkeeper_id) REFERENCES shopkeepers(id) ON DELETE CASCADE,
    FOREIGN KEY (container_id) REFERENCES shopkeeper_items(id) ON DELETE CASCADE,
    INDEX idx_shopkeeper_id (shopkeeper_id),
    INDEX idx_vnum (vnum),
    INDEX idx_obj_uid (obj_uid)
);

CREATE TABLE IF NOT EXISTS shopkeeper_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES shopkeeper_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
);

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
    item_material TINYINT DEFAULT NULL,
    unique_id INT UNSIGNED DEFAULT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (container_id) REFERENCES saved_items(id) ON DELETE CASCADE,
    INDEX idx_room_vnum (room_vnum),
    INDEX idx_vnum (vnum)
);

CREATE TABLE IF NOT EXISTS saved_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES saved_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
);

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
);

CREATE TABLE IF NOT EXISTS siege_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES siege_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
);


-- lockers

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
);

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
    item_material TINYINT DEFAULT NULL,
    obj_uid BIGINT UNSIGNED DEFAULT NULL,
    item_condition SMALLINT DEFAULT 100,
    FOREIGN KEY (locker_id) REFERENCES lockers(id) ON DELETE CASCADE,
    FOREIGN KEY (container_id) REFERENCES locker_items(id) ON DELETE CASCADE,
    INDEX idx_locker_id (locker_id),
    INDEX idx_vnum (vnum),
    INDEX idx_obj_uid (obj_uid)
);

CREATE TABLE IF NOT EXISTS locker_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES locker_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
);


-- account_characters extra columns
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

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'deleted_at');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN deleted_at DATETIME DEFAULT NULL',
    'SELECT "deleted_at already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND index_name = 'account_active');
SET @sql = IF(@idx_exists = 0,
    'CREATE INDEX account_active ON account_characters(account_name, deleted_at)',
    'SELECT "account_active already exists"');
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


-- ships

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
);

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
);


-- guilds

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
);

CREATE TABLE IF NOT EXISTS guild_ranks (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    guild_id INT UNSIGNED NOT NULL,
    rank_index TINYINT NOT NULL,
    title VARCHAR(100) NOT NULL DEFAULT '',
    CONSTRAINT fk_guild_ranks_guild FOREIGN KEY (guild_id) REFERENCES guilds(id) ON DELETE CASCADE,
    UNIQUE KEY uk_guild_ranks_index (guild_id, rank_index)
);

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
);

-- online_status col if missing
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


-- player_data fixes

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


-- unique constraints for char names
-- safe version: create the uniqueness constraint only when duplicates are not present
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


-- hardcore hall of fame

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_data' AND column_name = 'killed_by');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_data ADD COLUMN killed_by VARCHAR(64) DEFAULT NULL AFTER numb_deaths',
    'SELECT "killed_by already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;


-- unique keys for upsert
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_languages' AND index_name = 'uk_pid_tongue');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_languages ADD UNIQUE KEY uk_pid_tongue (pid, tongue_id)',
    'SELECT "uk_pid_tongue already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_intros' AND index_name = 'uk_pid_intro');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_intros ADD UNIQUE KEY uk_pid_intro (pid, intro_index)',
    'SELECT "uk_pid_intro already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_timers' AND index_name = 'uk_pid_timer');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_timers ADD UNIQUE KEY uk_pid_timer (pid, timer_id)',
    'SELECT "uk_pid_timer already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_undead_slots' AND index_name = 'uk_pid_circle');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_undead_slots ADD UNIQUE KEY uk_pid_circle (pid, circle)',
    'SELECT "uk_pid_circle already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_forged_items' AND index_name = 'uk_pid_forge');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_forged_items ADD UNIQUE KEY uk_pid_forge (pid, forge_index)',
    'SELECT "uk_pid_forge already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_granted_cmds' AND index_name = 'uk_pid_cmd');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_granted_cmds ADD UNIQUE KEY uk_pid_cmd (pid, cmd_num)',
    'SELECT "uk_pid_cmd already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_skills' AND index_name = 'uk_pid_skill');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_skills ADD UNIQUE KEY uk_pid_skill (pid, skill_id)',
    'SELECT "uk_pid_skill already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;


-- pets

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
);

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
  `wear_flags` INT DEFAULT NULL,
  `item_type` TINYINT DEFAULT NULL,
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
  `bitvector1` BIGINT UNSIGNED DEFAULT NULL,
  `bitvector2` BIGINT UNSIGNED DEFAULT NULL,
  `bitvector3` BIGINT UNSIGNED DEFAULT NULL,
  `bitvector4` BIGINT UNSIGNED DEFAULT NULL,
  `bitvector5` BIGINT UNSIGNED DEFAULT NULL,
  `item_material` TINYINT DEFAULT NULL,
  `obj_uid` BIGINT UNSIGNED DEFAULT NULL,
  `item_condition` SMALLINT DEFAULT 100,
  PRIMARY KEY (`id`),
  KEY `idx_pet_id` (`pet_id`),
  KEY `idx_container_id` (`container_id`),
  KEY `idx_obj_uid` (`obj_uid`),
  CONSTRAINT `fk_pet_items_pet` FOREIGN KEY (`pet_id`)
    REFERENCES `player_pets` (`id`) ON DELETE CASCADE,
  CONSTRAINT `fk_pet_items_container` FOREIGN KEY (`container_id`)
    REFERENCES `player_pet_items` (`id`) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS `player_pet_item_affects` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `item_id` INT UNSIGNED NOT NULL,
  `location` TINYINT UNSIGNED DEFAULT 0,
  `modifier` INT DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `fk_pet_item_affects` FOREIGN KEY (`item_id`)
    REFERENCES `player_pet_items` (`id`) ON DELETE CASCADE
);


-- zone payouts (optional, only if zones table exists)
DROP PROCEDURE IF EXISTS update_zone_payouts;

DELIMITER //
CREATE PROCEDURE update_zone_payouts()
BEGIN
    DECLARE tbl_exists INT DEFAULT 0;
    SELECT COUNT(*) INTO tbl_exists FROM information_schema.tables
        WHERE table_schema = DATABASE() AND table_name = 'zones';

    IF tbl_exists > 0 THEN
        -- disable group size penalty for now
        UPDATE zones SET suggested_group_size = 100 WHERE epic_type != '0';

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


-- item extra descriptions (spellbooks etc)

CREATE TABLE IF NOT EXISTS player_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_player_item_ed FOREIGN KEY (item_id)
    REFERENCES player_items(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS player_pet_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_pet_item_ed FOREIGN KEY (item_id)
    REFERENCES player_pet_items(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS corpse_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_corpse_item_ed FOREIGN KEY (item_id)
    REFERENCES corpse_items(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS locker_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_locker_item_ed FOREIGN KEY (item_id)
    REFERENCES locker_items(id) ON DELETE CASCADE
);

-- obj_uid for item duplication prevention
DELIMITER //

CREATE PROCEDURE add_obj_uid_columns()
BEGIN
    -- player_items
    IF EXISTS (SELECT 1 FROM information_schema.columns
               WHERE table_schema = DATABASE()
               AND table_name = 'player_items'
               AND column_name = 'unique_id') THEN
        ALTER TABLE player_items CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL;
    ELSEIF NOT EXISTS (SELECT 1 FROM information_schema.columns
                       WHERE table_schema = DATABASE()
                       AND table_name = 'player_items'
                       AND column_name = 'obj_uid') THEN
        ALTER TABLE player_items ADD COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_items'
                   AND column_name = 'item_condition') THEN
        ALTER TABLE player_items ADD COLUMN item_condition SMALLINT DEFAULT 100;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.statistics
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_items'
                   AND index_name = 'idx_obj_uid') THEN
        ALTER TABLE player_items ADD INDEX idx_obj_uid (obj_uid);
    END IF;

    -- corpse_items
    IF EXISTS (SELECT 1 FROM information_schema.columns
               WHERE table_schema = DATABASE()
               AND table_name = 'corpse_items'
               AND column_name = 'unique_id') THEN
        ALTER TABLE corpse_items CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL;
    ELSEIF NOT EXISTS (SELECT 1 FROM information_schema.columns
                       WHERE table_schema = DATABASE()
                       AND table_name = 'corpse_items'
                       AND column_name = 'obj_uid') THEN
        ALTER TABLE corpse_items ADD COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'corpse_items'
                   AND column_name = 'item_condition') THEN
        ALTER TABLE corpse_items ADD COLUMN item_condition SMALLINT DEFAULT 100;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.statistics
                   WHERE table_schema = DATABASE()
                   AND table_name = 'corpse_items'
                   AND index_name = 'idx_obj_uid') THEN
        ALTER TABLE corpse_items ADD INDEX idx_obj_uid (obj_uid);
    END IF;

    -- locker_items
    IF EXISTS (SELECT 1 FROM information_schema.columns
               WHERE table_schema = DATABASE()
               AND table_name = 'locker_items'
               AND column_name = 'unique_id') THEN
        ALTER TABLE locker_items CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL;
    ELSEIF NOT EXISTS (SELECT 1 FROM information_schema.columns
                       WHERE table_schema = DATABASE()
                       AND table_name = 'locker_items'
                       AND column_name = 'obj_uid') THEN
        ALTER TABLE locker_items ADD COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'locker_items'
                   AND column_name = 'item_condition') THEN
        ALTER TABLE locker_items ADD COLUMN item_condition SMALLINT DEFAULT 100;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.statistics
                   WHERE table_schema = DATABASE()
                   AND table_name = 'locker_items'
                   AND index_name = 'idx_obj_uid') THEN
        ALTER TABLE locker_items ADD INDEX idx_obj_uid (obj_uid);
    END IF;

    -- player_pet_items
    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_pet_items'
                   AND column_name = 'obj_uid') THEN
        ALTER TABLE player_pet_items ADD COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_pet_items'
                   AND column_name = 'item_condition') THEN
        ALTER TABLE player_pet_items ADD COLUMN item_condition SMALLINT DEFAULT 100;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.statistics
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_pet_items'
                   AND index_name = 'idx_obj_uid') THEN
        ALTER TABLE player_pet_items ADD INDEX idx_obj_uid (obj_uid);
    END IF;
END //

DELIMITER ;

CALL add_obj_uid_columns();
DROP PROCEDURE IF EXISTS add_obj_uid_columns;


-- account lockers

CREATE TABLE IF NOT EXISTS account_lockers (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    account_name VARCHAR(50) NOT NULL UNIQUE,
    racewar TINYINT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (account_name) REFERENCES accounts(account_name) ON DELETE CASCADE,
    INDEX idx_account_name (account_name)
);

CREATE TABLE IF NOT EXISTS locker_chests (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    keyword VARCHAR(64) NOT NULL,
    keyword_hash VARCHAR(64) DEFAULT NULL,
    is_public TINYINT(1) DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (locker_id) REFERENCES account_lockers(id) ON DELETE CASCADE,
    UNIQUE KEY uk_locker_keyword (locker_id, keyword),
    INDEX idx_locker_id (locker_id)
);

CREATE TABLE IF NOT EXISTS account_locker_items (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    chest_id INT UNSIGNED NOT NULL,
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
    item_material TINYINT DEFAULT NULL,
    obj_uid BIGINT UNSIGNED DEFAULT NULL,
    item_condition SMALLINT DEFAULT 100,
    FOREIGN KEY (chest_id) REFERENCES locker_chests(id) ON DELETE CASCADE,
    FOREIGN KEY (container_id) REFERENCES account_locker_items(id) ON DELETE CASCADE,
    INDEX idx_chest_id (chest_id),
    INDEX idx_vnum (vnum),
    INDEX idx_obj_uid (obj_uid)
);

CREATE TABLE IF NOT EXISTS account_locker_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES account_locker_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
);

CREATE TABLE IF NOT EXISTS account_locker_access (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    visitor_account VARCHAR(50) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (locker_id) REFERENCES account_lockers(id) ON DELETE CASCADE,
    UNIQUE KEY uk_locker_visitor (locker_id, visitor_account),
    INDEX idx_visitor (visitor_account)
);

CREATE TABLE IF NOT EXISTS locker_activity_log (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    account_name VARCHAR(50) NOT NULL,
    char_name VARCHAR(64) NOT NULL,
    action_type ENUM('enter', 'leave', 'chest_open', 'chest_fail', 'kicked', 'chest_create', 'chest_delete', 'item_put', 'item_get') NOT NULL,
    chest_keyword VARCHAR(64) DEFAULT NULL,
    details VARCHAR(255) DEFAULT NULL,
    logged_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (locker_id) REFERENCES account_lockers(id) ON DELETE CASCADE,
    INDEX idx_locker_id (locker_id),
    INDEX idx_logged_at (logged_at)
);

CREATE TABLE IF NOT EXISTS locker_kickouts (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    account_name VARCHAR(50) NOT NULL,
    fail_count TINYINT UNSIGNED DEFAULT 0,
    kicked_until TIMESTAMP NULL DEFAULT NULL,
    last_fail TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (locker_id) REFERENCES account_lockers(id) ON DELETE CASCADE,
    UNIQUE KEY uk_locker_account (locker_id, account_name)
);

CREATE TABLE IF NOT EXISTS locker_session_state (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    account_name VARCHAR(50) NOT NULL,
    chest_id INT UNSIGNED NOT NULL,
    opened_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (locker_id) REFERENCES account_lockers(id) ON DELETE CASCADE,
    FOREIGN KEY (chest_id) REFERENCES locker_chests(id) ON DELETE CASCADE,
    UNIQUE KEY uk_session (locker_id, account_name, chest_id)
);


-- migrate char lockers to account lockers (non-destructive)

-- sync account_name
UPDATE player_data pd
JOIN account_characters ac ON pd.pid = ac.pid
SET pd.account_name = ac.account_name
WHERE pd.account_name IS NULL OR pd.account_name = '';

-- create account lockers
INSERT IGNORE INTO lockers (locker_name, racewar, race)
SELECT DISTINCT CONCAT('account.', LOWER(ac.account_name), '.', ac.racewar, '.locker'), ac.racewar, 0
FROM account_characters ac
JOIN lockers l ON LOWER(SUBSTRING_INDEX(l.locker_name, '.locker', 1)) = LOWER(ac.char_name)
WHERE ac.account_name IS NOT NULL AND ac.account_name != ''
  AND l.locker_name LIKE '%.locker'
  AND l.locker_name NOT LIKE 'guild.%'
  AND l.locker_name NOT LIKE 'account.%';

-- copy items
INSERT INTO locker_items (locker_id, vnum, container_id, quantity, weight, cost, timer,
    extra_flags, value0, value1, value2, value3, value4, value5, value6, value7,
    name, short_descr, description, action_descr, obj_uid, item_condition)
SELECT
    acct_locker.id,
    src.vnum,
    NULL,
    src.quantity, src.weight, src.cost, src.timer,
    src.extra_flags, src.value0, src.value1, src.value2, src.value3,
    src.value4, src.value5, src.value6, src.value7,
    src.name, src.short_descr, src.description, src.action_descr, src.obj_uid, src.item_condition
FROM locker_items src
JOIN lockers char_locker ON src.locker_id = char_locker.id
JOIN account_characters ac ON LOWER(SUBSTRING_INDEX(char_locker.locker_name, '.locker', 1)) = LOWER(ac.char_name)
JOIN lockers acct_locker ON acct_locker.locker_name = CONCAT('account.', LOWER(ac.account_name), '.', ac.racewar, '.locker')
WHERE char_locker.locker_name LIKE '%.locker'
  AND char_locker.locker_name NOT LIKE 'guild.%'
  AND char_locker.locker_name NOT LIKE 'account.%'
  AND src.vnum != 173
  AND (src.obj_uid IS NULL OR src.obj_uid NOT IN (
      SELECT obj_uid FROM locker_items WHERE locker_id = acct_locker.id AND obj_uid IS NOT NULL
  ));

-- copy affects
INSERT INTO locker_item_affects (item_id, location, modifier)
SELECT new_item.id, lia.location, lia.modifier
FROM locker_item_affects lia
JOIN locker_items old_item ON lia.item_id = old_item.id
JOIN lockers char_locker ON old_item.locker_id = char_locker.id
JOIN account_characters ac ON LOWER(SUBSTRING_INDEX(char_locker.locker_name, '.locker', 1)) = LOWER(ac.char_name)
JOIN lockers acct_locker ON acct_locker.locker_name = CONCAT('account.', LOWER(ac.account_name), '.', ac.racewar, '.locker')
JOIN locker_items new_item ON old_item.obj_uid = new_item.obj_uid AND new_item.locker_id = acct_locker.id
WHERE char_locker.locker_name LIKE '%.locker'
  AND char_locker.locker_name NOT LIKE 'guild.%'
  AND char_locker.locker_name NOT LIKE 'account.%'
  AND old_item.obj_uid IS NOT NULL
  AND NOT EXISTS (
      SELECT 1 FROM locker_item_affects WHERE item_id = new_item.id AND location = lia.location
  );


-- account banks

CREATE TABLE IF NOT EXISTS account_banks (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    account_name VARCHAR(50) NOT NULL,
    racewar TINYINT NOT NULL DEFAULT 0,
    bank_copper BIGINT UNSIGNED DEFAULT 0,
    bank_silver BIGINT UNSIGNED DEFAULT 0,
    bank_gold BIGINT UNSIGNED DEFAULT 0,
    bank_platinum BIGINT UNSIGNED DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (account_name) REFERENCES accounts(account_name) ON DELETE CASCADE,
    UNIQUE KEY uk_account_racewar (account_name, racewar),
    INDEX idx_account_name (account_name)
);

-- migrate player banks to account banks
REPLACE INTO account_banks (account_name, racewar, bank_copper, bank_silver, bank_gold, bank_platinum)
SELECT
    ac.account_name,
    ac.racewar,
    SUM(pd.bank_copper),
    SUM(pd.bank_silver),
    SUM(pd.bank_gold),
    SUM(pd.bank_platinum)
FROM account_characters ac
JOIN player_data pd ON ac.pid = pd.pid
WHERE pd.bank_copper > 0 OR pd.bank_silver > 0 OR pd.bank_gold > 0 OR pd.bank_platinum > 0
GROUP BY ac.account_name, ac.racewar;


-- private chests (links to lockers table)

CREATE TABLE IF NOT EXISTS private_chests (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    chest_name VARCHAR(32) NOT NULL,
    password_hash VARCHAR(64) DEFAULT NULL,
    is_public TINYINT(1) DEFAULT 0,
    sort_config TEXT DEFAULT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (locker_id) REFERENCES lockers(id) ON DELETE CASCADE,
    UNIQUE KEY uk_locker_chest (locker_id, chest_name),
    INDEX idx_locker_id (locker_id)
);

-- add chest_id to locker_items
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'chest_id');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items ADD COLUMN chest_id INT UNSIGNED DEFAULT NULL AFTER locker_id',
    'SELECT "chest_id already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- activity log for locker owner
CREATE TABLE IF NOT EXISTS private_chest_log (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    chest_id INT UNSIGNED DEFAULT NULL,
    char_name VARCHAR(64) NOT NULL,
    action_type ENUM('open', 'close', 'put', 'get', 'fail') NOT NULL,
    item_short VARCHAR(256) DEFAULT NULL,
    logged_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (locker_id) REFERENCES lockers(id) ON DELETE CASCADE,
    INDEX idx_locker_id (locker_id),
    INDEX idx_logged_at (logged_at)
);

-- create default public chest for existing lockers
INSERT IGNORE INTO private_chests (locker_id, chest_name, is_public)
SELECT id, 'public', 1
FROM lockers
WHERE locker_name LIKE 'account.%';


-- kofi donations
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'accounts' AND column_name = 'total_donated');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE accounts ADD COLUMN total_donated DECIMAL(10,2) DEFAULT 0.00',
    'SELECT "total_donated already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;


-- polls

CREATE TABLE IF NOT EXISTS polls (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    question VARCHAR(512) NOT NULL,
    created_by VARCHAR(32) NOT NULL,
    created_at INT NOT NULL DEFAULT 0,
    expires_at INT NOT NULL DEFAULT 0,
    is_active TINYINT(1) NOT NULL DEFAULT 1,
    multi_select TINYINT(1) NOT NULL DEFAULT 0,
    max_choices INT NOT NULL DEFAULT 1
);

CREATE TABLE IF NOT EXISTS poll_options (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    poll_id INT UNSIGNED NOT NULL,
    option_num INT NOT NULL,
    option_text VARCHAR(256) NOT NULL,
    INDEX idx_poll_id (poll_id)
);

CREATE TABLE IF NOT EXISTS poll_votes (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    poll_id INT UNSIGNED NOT NULL,
    account_name VARCHAR(64) NOT NULL,
    option_id INT UNSIGNED NOT NULL,
    voted_at INT NOT NULL DEFAULT 0,
    char_name VARCHAR(32) NOT NULL,
    UNIQUE KEY uk_poll_account_option (poll_id, account_name, option_id),
    INDEX idx_poll_id (poll_id),
    INDEX idx_account_name (account_name)
);


-- done

-- ============================================================================
-- FILE: schema_migration_v3_lockers.sql
-- ============================================================================
-- durismud locker tables schema
-- player lockers and guild lockers

-- ============================================================================
-- lockers table (replaces .locker pfiles)
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


-- ============================================================================
-- locker items (normalized, same pattern as other item tables)
-- ============================================================================

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
-- FILE: schema_migration_v4_accounts.sql
-- ============================================================================
-- account_characters pfile columns
--
-- Idempotency: all ALTER TABLEs guarded with information_schema checks.
-- CREATE INDEX wrapped with IF NOT EXISTS equivalent.

-- login_count
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'login_count');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN login_count BIGINT UNSIGNED DEFAULT 0',
    'SELECT "login_count already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- last_login
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'last_login');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN last_login BIGINT DEFAULT 0',
    'SELECT "last_login already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- blocked
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'blocked');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN blocked TINYINT DEFAULT 0',
    'SELECT "blocked already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- racewar
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'racewar');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN racewar TINYINT DEFAULT 0',
    'SELECT "racewar already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- index on (account_name, racewar)
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'account_characters'
    AND index_name = 'idx_account_racewar');
SET @sql = IF(@idx_exists = 0,
    'CREATE INDEX idx_account_racewar ON account_characters(account_name, racewar)',
    'SELECT "idx_account_racewar already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- ============================================================================
-- FILE: retired_schema_migration_v5_ships.sql
-- ============================================================================
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

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'ships' AND index_name = 'idx_ships_owner_pid');
SET @sql = IF(@idx_exists = 0,
    'CREATE INDEX idx_ships_owner_pid ON ships(owner_pid)',
    'SELECT "idx_ships_owner_pid already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

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

create table if not exists ship_armor (
    id int unsigned auto_increment primary key,
    ship_id int unsigned not null,
    side tinyint not null,
    armor int default 0,
    internal int default 0,
    constraint fk_ship_armor_ship foreign key (ship_id) references ships(id) on delete cascade,
    unique key uk_ship_armor (ship_id, side)
) engine=innodb;

create table if not exists ship_crew (
    id int unsigned auto_increment primary key,
    ship_id int unsigned not null,
    crew_index int default 0,
    sail_skill int default 0,
    guns_skill int default 0,
    rpar_skill int default 0,
    sail_chief int default 0,
    guns_chief int default 0,
    rpar_chief int default 0,
    constraint fk_ship_crew_ship foreign key (ship_id) references ships(id) on delete cascade,
    unique key uk_ship_crew (ship_id)
) engine=innodb;

-- ============================================================================
-- FILE: retired_schema_migration_v6_guilds.sql
-- ============================================================================
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
    player_name varchar(64) not null,
    player_pid int unsigned default null,
    bits int unsigned not null default 0,
    debt int unsigned not null default 0,
    online_status tinyint not null default 0,
    constraint fk_guild_members_guild foreign key (guild_id) references guilds(id) on delete cascade,
    unique key uk_guild_members_name (guild_id, player_name)
) engine=innodb;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'guild_members' AND index_name = 'idx_guild_members_name');
SET @sql = IF(@idx_exists = 0,
    'CREATE INDEX idx_guild_members_name ON guild_members(player_name)',
    'SELECT "idx_guild_members_name already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;


-- ============================================================================
-- FILE: bootstrap_base_boons_outposts.sql
-- ============================================================================
-- boons tables

create table if not exists boons (
    id int auto_increment primary key,
    time int not null default 0,
    duration int not null default 0,
    racewar int not null default 0,
    type int not null default 0,
    opt int not null default 0,
    criteria decimal(10,2) not null default 0.00,
    criteria2 decimal(10,2) not null default 0.00,
    bonus decimal(10,2) not null default 0.00,
    bonus2 decimal(10,2) not null default 0.00,
    random int not null default 0,
    author varchar(20) default null,
    active int not null default 0,
    pid int not null default 0,
    rpt int not null default 0
) engine=innodb;

create table if not exists boons_progress (
    id int auto_increment primary key,
    boonid int not null default 0,
    pid int not null default 0,
    counter decimal(10,2) not null default 0.00
) engine=innodb;

create table if not exists boons_shop (
    id int auto_increment primary key,
    pid int not null default 0,
    points int not null default 0,
    stats int not null default 0,
    unique key uk_pid (pid)
) engine=innodb;

-- outposts tables

create table if not exists outposts (
    id int not null,
    owner_id int not null default 0,
    level int not null default 1,
    walls int not null default 0,
    archers int not null default 0,
    resources int not null default 0,
    applied_resources int not null default 100000,
    hitpoints int not null default 0,
    territory int not null default 0,
    portal_room int not null default 0,
    golems int not null default 0,
    meurtriere int not null default 0,
    scouts int not null default 0,
    primary key (id)
) engine=innodb;

-- additional runtime tables aligned from run_migration.sh

create table if not exists artifacts (
    vnum int not null,
    owned char(1) not null,
    locType int not null default 1,
    location int not null,
    timer datetime default null,
    type int not null,
    lastUpdate datetime default null,
    primary key (vnum)
) engine=innodb;

create table if not exists artifacts_mortal (
    vnum int not null,
    owned char(1) not null,
    locType int not null,
    location int not null,
    timer datetime default null,
    type int not null,
    primary key (vnum)
) engine=innodb;

create table if not exists artifact_bind (
    vnum int not null primary key,
    owner_pid int default null,
    timer int default null
) engine=innodb;

create table if not exists alliances (
    id int auto_increment primary key,
    created_at datetime default null,
    forging_assoc_id int not null,
    joining_assoc_id int not null,
    tribute_owed int not null default 0
) engine=innodb;

create table if not exists associations (
    id int not null primary key,
    name varchar(255) not null default '',
    prestige int not null default 0,
    active tinyint(1) not null default 1,
    wood int not null default 0,
    stone int not null default 0,
    construction_points int not null default 0,
    over_max int not null default 0
) engine=innodb;

create table if not exists guild_transactions (
    id int unsigned auto_increment primary key,
    soc_id int unsigned not null default 0,
    date int not null default 0,
    transaction_info varchar(255) not null default '',
    index idx_soc_id (soc_id)
) engine=innodb;

create table if not exists guildhall_rooms (
    id int auto_increment primary key,
    guildhall_id int not null default 0,
    vnum int not null default 0,
    type int not null default 0,
    value0 int unsigned not null default 0,
    value1 int unsigned not null default 0,
    value2 int unsigned not null default 0,
    value3 int unsigned not null default 0,
    value4 int unsigned not null default 0,
    value5 int unsigned not null default 0,
    value6 int unsigned not null default 0,
    value7 int unsigned not null default 0,
    exit0 int not null default 0,
    exit1 int not null default 0,
    exit2 int not null default 0,
    exit3 int not null default 0,
    exit4 int not null default 0,
    exit5 int not null default 0,
    exit6 int not null default 0,
    exit7 int not null default 0,
    exit8 int not null default 0,
    exit9 int not null,
    name varchar(255) not null,
    index idx_vnum (vnum),
    index idx_guildhall_id (guildhall_id)
) engine=innodb;

create table if not exists guildhalls (
    id int auto_increment primary key,
    assoc_id int not null default 0,
    type int not null default 0,
    outside_vnum int not null default 0,
    racewar int not null default 0,
    index idx_assoc_id (assoc_id)
) engine=innodb;

create table if not exists ship_cargo_market_mods (
    type varchar(255) not null default '',
    port_id int not null default -1,
    cargo_type int not null default -1,
    modifier float not null default 0,
    key type_port_id_cargo_type (type, port_id, cargo_type)
) engine=innodb;

create table if not exists ship_cargo_prices (
    type varchar(255) not null default '',
    port_id int not null default -1,
    cargo_type int not null default -1,
    price int not null default 0,
    key type_port_id_cargo_type (type, port_id, cargo_type)
) engine=innodb;

create table if not exists shop_trophy (
    id int auto_increment primary key,
    item int not null default 0,
    value int not null default 0,
    seller int not null default 0,
    timestamp timestamp not null default current_timestamp on update current_timestamp
) engine=innodb;

create table if not exists statistics (
    id int auto_increment primary key,
    date int not null default 0,
    goods_count int not null default 0,
    evils_count int not null default 0,
    illithids_count int not null default 0,
    undeads_count int not null default 0,
    gods_count int not null default 0,
    in_guildhall_count int not null default 0,
    sum_goods_levels int not null default 0,
    sum_evils_levels int not null default 0,
    sum_illithids_levels int not null default 0,
    sum_undeads_levels int not null default 0,
    unique_ips_count int not null default 0
) engine=innodb;

create table if not exists timers (
    name varchar(255) not null default '',
    date int not null default 0,
    primary key (name)
) engine=innodb;

create table if not exists world_quest_accomplished (
    id int unsigned auto_increment primary key,
    pid varchar(45) not null default '',
    timestamp timestamp not null default current_timestamp on update current_timestamp,
    quest_giver int unsigned not null default 0,
    player_name varchar(45) not null default '',
    player_level int unsigned not null default 0,
    quest_target int not null default 0,
    reward_vnum int not null default 0,
    reward_desc varchar(255) not null default ''
) engine=innodb;

create table if not exists zones (
    id int auto_increment primary key,
    number int default null,
    name varchar(100) not null default '',
    epic_type int not null default 0,
    frequency_mod float not null default 1,
    zone_freq_mod float not null default 1,
    epic_level int not null default 0,
    task_zone tinyint(1) not null default 0,
    quest_zone tinyint(1) not null default 0,
    trophy_zone tinyint(1) not null default 1,
    suggested_group_size int not null default 1,
    epic_payout int not null default 0,
    difficulty int not null default 0,
    randoms_zone tinyint(1) not null default 1,
    alignment int not null default 0,
    last_touch timestamp null default null,
    reset_perc int default 0,
    stonecount int not null default 1,
    index idx_number (number)
) engine=innodb;

create table if not exists zone_touches (
    id int auto_increment primary key,
    boot_time timestamp null default null,
    zone_number int default null,
    touched_at timestamp null default null,
    toucher_pid int default null,
    group_size int default null,
    epic_value int default null,
    alignment_delta int default null,
    index idx_zone_number (zone_number)
) engine=innodb;

create table if not exists zone_trophy (
    pid bigint not null default 0,
    zone_number int not null default 0,
    exp int not null default 0,
    primary key (pid, zone_number),
    index idx_pid (pid),
    index idx_zone_number (zone_number),
    index idx_exp (exp)
) engine=innodb;

create table if not exists locker_access (
    owner varchar(255) not null,
    visitor varchar(255) not null,
    primary key (owner, visitor)
) engine=innodb;

create table if not exists ctf_data (
    id int auto_increment primary key,
    time timestamp null default null,
    pid int not null default 0,
    type int not null default 0,
    flagtype int not null default 0,
    racewar int not null default 0
) engine=innodb;

create table if not exists epic_bonus (
    pid int not null,
    type int not null default 0,
    time datetime default null,
    unique key uk_pid (pid)
) engine=innodb;

create table if not exists epic_gain (
    id int unsigned auto_increment primary key,
    pid bigint not null default 0,
    time datetime not null,
    type int not null default 0,
    type_id int not null default 0,
    epics int not null default 0,
    index idx_pid (pid)
) engine=innodb;

create table if not exists eq_drop (
    id int unsigned auto_increment primary key,
    date timestamp not null default current_timestamp on update current_timestamp,
    vnum int unsigned not null default 0,
    pid_looter bigint unsigned not null default 0,
    room_id int unsigned not null default 0,
    index idx_vnum (vnum)
) engine=innodb;

create table if not exists racewar_stat_mods (
    racewar int not null default 0,
    Str int not null default 0,
    Dex int not null default 0,
    Agi int not null default 0,
    Con int not null default 0,
    Pow int not null default 0,
    Intl int not null default 0,
    Wis int not null default 0,
    Cha int not null default 0,
    Kar int not null default 0,
    Luc int not null default 0
) engine=innodb;

create table if not exists categories (
    id int auto_increment primary key,
    name varchar(255) default null,
    `desc` varchar(255) default null
) engine=innodb;

create table if not exists changes (
    id int auto_increment primary key,
    history_id int default null,
    history_text text,
    history_title varchar(255) default null,
    history_category_id int default null,
    new_text text,
    new_title varchar(255) default null,
    new_category_id int default null,
    timestamp datetime default null,
    action varchar(255) default null,
    ip_number varchar(255) default null
) engine=innodb;

create table if not exists ip_info (
    pid bigint not null default 0,
    last_ip varchar(50) not null default 'none',
    last_connect datetime null default null,
    last_disconnect datetime null default null,
    racewar_side int not null default 0,
    primary key (pid)
) engine=innodb;

create table if not exists items (
    vnum int unsigned not null default 0,
    short_desc varchar(100) not null default '',
    obj_stat text not null,
    num_sold int not null default 0,
    avg_sell_price int not null default 0,
    primary key (vnum)
) engine=innodb;

create table if not exists level_cap (
    id int auto_increment primary key,
    most_frags float not null default 0,
    racewar_leader int not null default 0,
    level int not null default 25,
    next_update datetime default current_timestamp
) engine=innodb;

create table if not exists log_entries (
    id int unsigned auto_increment primary key,
    date datetime not null,
    kind varchar(255) not null default '',
    player_name varchar(255) not null default '',
    pid int not null default 0,
    ip_address varchar(15) not null default '',
    room_vnum int not null default 0,
    zone_number int not null default 0,
    message varchar(255) not null default '',
    index idx_date (date),
    index idx_kind (kind),
    index idx_player_name (player_name),
    index idx_pid (pid),
    index idx_ip_address (ip_address),
    index idx_room_vnum (room_vnum),
    index idx_zone_number (zone_number)
) engine=innodb;

create table if not exists mud_info (
    name varchar(255) not null,
    content text not null,
    primary key (name)
) engine=innodb;

create table if not exists multiplay_whitelist (
    id int auto_increment primary key,
    pattern varchar(255) not null,
    admin varchar(255) not null,
    description varchar(255) not null,
    created_on date default null,
    player varchar(255) not null
) engine=innodb;

create table if not exists nexus_stones (
    id int auto_increment primary key,
    name varchar(255) not null default '',
    room_vnum int not null default 0,
    align int not null default 0,
    stat_affect int not null default -1,
    affect_amount int not null default 0,
    last_touched_at timestamp null default null,
    bonus int not null default 0
) engine=innodb;

create table if not exists offline_messages (
    id int auto_increment primary key,
    pid bigint not null default 0,
    sender varchar(255) not null default '',
    message text not null,
    sent_at datetime not null default current_timestamp,
    read_at datetime default null,
    index idx_pid (pid)
) engine=innodb;

create table if not exists pages (
    id int auto_increment primary key,
    title varchar(255) default null,
    text text,
    last_update datetime default null,
    last_update_by varchar(255) default null,
    category_id int default null,
    ip_number varchar(255) default null
) engine=innodb;

create table if not exists ping (
    id bigint auto_increment primary key,
    timestamp datetime not null,
    url varchar(100) not null default '',
    ip varchar(100) not null default '',
    seq bigint not null default 0,
    time int not null default 0
) engine=innodb;

create table if not exists pkill_event (
    id int unsigned auto_increment primary key,
    stamp datetime not null,
    room_vnum int not null default 0,
    room_name text not null,
    tweeted tinyint(1) not null default 0
) engine=innodb;

create table if not exists pkill_info (
    id bigint unsigned auto_increment primary key,
    event_id int unsigned not null default 0,
    pid bigint not null default 0,
    level int not null default 0,
    pk_type text not null,
    equip text not null,
    log text,
    inroom int not null default 0,
    leader int default null,
    player_description varchar(255),
    index idx_event_id (event_id),
    index idx_pid (pid)
) engine=innodb;

create table if not exists prepstatment_duris_sql (
    id int unsigned auto_increment primary key,
    `desc` text not null,
    `sql` text not null
) engine=innodb;

create table if not exists progress (
    id int unsigned auto_increment primary key,
    pid bigint not null default 0,
    var_type int not null default 1,
    stamp datetime not null,
    delta int not null default 0,
    index idx_pid (pid),
    index idx_var_type (var_type)
) engine=innodb;

-- ============================================================================
-- FILE: schema_migration_v7_player_fixes.sql
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

-- ============================================================================
-- FILE: schema_migration_v8_hardcore.sql
-- ============================================================================
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

-- ============================================================================
-- FILE: schema_migration_v8_unique.sql
-- ============================================================================
-- schema migration v8: add unique constraints for character names
--
-- Safe version: add the unique index only if it does not already exist.
-- If duplicates exist, the migration will fail rather than deleting data.
--
-- add unique constraint on char_name (character names must be unique)
SET @key_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'account_characters'
    AND index_name = 'idx_char_name_unique');
SET @sql = IF(@key_exists = 0,
    'ALTER TABLE account_characters ADD UNIQUE INDEX idx_char_name_unique (char_name)',
    'SELECT "idx_char_name_unique already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- add unique constraint on player_data.name
SET @key_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_data'
    AND index_name = 'idx_player_name_unique');
SET @sql = IF(@key_exists = 0,
    'ALTER TABLE player_data ADD UNIQUE INDEX idx_player_name_unique (name)',
    'SELECT "idx_player_name_unique already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- ============================================================================
-- FILE: schema_migration_v9_dirty_saves.sql
-- ============================================================================
-- dirty saves support: add unique constraints for upsert pattern
-- run this before enabling redis dirty saves
--
-- Idempotency: all ALTER TABLEs are guarded with information_schema checks.

-- player_languages: unique on (pid, tongue_id)
SET @key_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_languages'
    AND index_name = 'uk_pid_tongue');
SET @sql = IF(@key_exists = 0,
    'ALTER TABLE player_languages ADD UNIQUE KEY uk_pid_tongue (pid, tongue_id)',
    'SELECT "uk_pid_tongue already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_intros: unique on (pid, intro_index)
SET @key_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_intros'
    AND index_name = 'uk_pid_intro');
SET @sql = IF(@key_exists = 0,
    'ALTER TABLE player_intros ADD UNIQUE KEY uk_pid_intro (pid, intro_index)',
    'SELECT "uk_pid_intro already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_timers: unique on (pid, timer_id)
SET @key_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_timers'
    AND index_name = 'uk_pid_timer');
SET @sql = IF(@key_exists = 0,
    'ALTER TABLE player_timers ADD UNIQUE KEY uk_pid_timer (pid, timer_id)',
    'SELECT "uk_pid_timer already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_undead_slots: unique on (pid, circle)
SET @key_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_undead_slots'
    AND index_name = 'uk_pid_circle');
SET @sql = IF(@key_exists = 0,
    'ALTER TABLE player_undead_slots ADD UNIQUE KEY uk_pid_circle (pid, circle)',
    'SELECT "uk_pid_circle already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_forged_items: unique on (pid, forge_index)
SET @key_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_forged_items'
    AND index_name = 'uk_pid_forge');
SET @sql = IF(@key_exists = 0,
    'ALTER TABLE player_forged_items ADD UNIQUE KEY uk_pid_forge (pid, forge_index)',
    'SELECT "uk_pid_forge already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_granted_cmds: unique on (pid, cmd_num)
SET @key_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_granted_cmds'
    AND index_name = 'uk_pid_cmd');
SET @sql = IF(@key_exists = 0,
    'ALTER TABLE player_granted_cmds ADD UNIQUE KEY uk_pid_cmd (pid, cmd_num)',
    'SELECT "uk_pid_cmd already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_skills: unique on (pid, skill_id)
SET @key_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_skills'
    AND index_name = 'uk_pid_skill');
SET @sql = IF(@key_exists = 0,
    'ALTER TABLE player_skills ADD UNIQUE KEY uk_pid_skill (pid, skill_id)',
    'SELECT "uk_pid_skill already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- ============================================================================
-- FILE: schema_migration_v10_pets.sql
-- ============================================================================
-- pet persistence: save charmed pets across crashes
-- schema migration v10

-- pet data (one row per pet)
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

-- pet equipment and inventory
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

-- pet item affects (stat mods)
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
-- FILE: schema_migration_v11_obj_uid.sql
-- ============================================================================
-- obj_uid and item_condition for duplication prevention
-- schema migration v11
--
-- Idempotency: all ALTER TABLEs are guarded with information_schema checks.

-- player_items: rename unique_id to obj_uid, change to bigint, add condition
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_items' AND column_name = 'obj_uid');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_items CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "obj_uid already exists on player_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_items' AND column_name = 'item_condition');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_items ADD COLUMN item_condition SMALLINT DEFAULT 100 AFTER obj_uid',
    'SELECT "item_condition already exists on player_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_items' AND index_name = 'idx_obj_uid');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_items ADD INDEX idx_obj_uid (obj_uid)',
    'SELECT "idx_obj_uid already exists on player_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- corpse_items: same changes
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'obj_uid');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpse_items CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "obj_uid already exists on corpse_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'item_condition');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpse_items ADD COLUMN item_condition SMALLINT DEFAULT 100 AFTER obj_uid',
    'SELECT "item_condition already exists on corpse_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND index_name = 'idx_obj_uid');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE corpse_items ADD INDEX idx_obj_uid (obj_uid)',
    'SELECT "idx_obj_uid already exists on corpse_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- locker_items: same changes
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'obj_uid');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "obj_uid already exists on locker_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'item_condition');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items ADD COLUMN item_condition SMALLINT DEFAULT 100 AFTER obj_uid',
    'SELECT "item_condition already exists on locker_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND index_name = 'idx_obj_uid');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE locker_items ADD INDEX idx_obj_uid (obj_uid)',
    'SELECT "idx_obj_uid already exists on locker_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_pet_items: add obj_uid and condition (no unique_id existed)
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND column_name = 'obj_uid');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_pet_items ADD COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL AFTER action_descr',
    'SELECT "obj_uid already exists on player_pet_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND column_name = 'item_condition');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_pet_items ADD COLUMN item_condition SMALLINT DEFAULT 100 AFTER obj_uid',
    'SELECT "item_condition already exists on player_pet_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND index_name = 'idx_obj_uid');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_pet_items ADD INDEX idx_obj_uid (obj_uid)',
    'SELECT "idx_obj_uid already exists on player_pet_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- ============================================================================
-- FILE: schema_migration_v11_remove_players_core.sql
-- ============================================================================
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

-- ============================================================================
-- FILE: schema_migration_v13_locker_sort.sql
-- ============================================================================
-- sorted chests are in-memory only, no database changes needed
-- this migration is kept for backwards compatibility
SELECT 1;

-- ============================================================================
-- FILE: schema_migration_v14_locker_bitvectors.sql
-- ============================================================================
-- add bitvector columns to locker_items for encrusted item affects
-- also fixes existing encrusted items that lost their bitvectors during migration

-- step 1: add bitvector columns to locker_items
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "bitvector1 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "bitvector2 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "bitvector3 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "bitvector4 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "bitvector5 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- same for account_locker_items
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_locker_items' AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_locker_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "bitvector1 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_locker_items' AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_locker_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "bitvector2 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_locker_items' AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_locker_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "bitvector3 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_locker_items' AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_locker_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "bitvector4 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_locker_items' AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_locker_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "bitvector5 already exists"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- step 2: fix existing encrusted items
-- ITEM_ENCRUSTED = BIT_32 = 2147483648
-- value5 stores the spell type, which determines what bitvector to set
--
-- spell to bitvector mapping (from set_encrust_affect in randomeq.c):
-- SPELL_ACID_BLAST (116)               -> bitvector2 = AFF2_PROT_GAS | AFF2_PROT_ACID = 4096 | 8192 = 12288
-- SPELL_FIREBALL (26)                  -> bitvector1 = AFF_PROT_FIRE = 536870912
-- SPELL_MAGIC_MISSILE (32)             -> bitvector1 = AFF_PROTECT_GOOD = 65536
-- SPELL_CHILL_TOUCH (8)                -> bitvector2 = AFF2_PROT_COLD = 64
-- SPELL_NEGATIVE_CONCUSSION_BLAST (378)-> bitvector1 = AFF_MINOR_GLOBE = 64
-- SPELL_SHOCKING_GRASP (37)            -> bitvector1 = AFF_SLOW_POISON = 32768
-- SPELL_FLAMESTRIKE (21)               -> bitvector1 = AFF_SENSE_LIFE = 32
-- SPELL_BLINDNESS (4)                  -> bitvector2 = AFF2_DETECT_MAGIC = 16
-- SPELL_ENERGY_DRAIN (25)              -> bitvector1 = AFF_HASTE = 16

-- fix locker_items encrusted items
UPDATE locker_items
SET bitvector2 = COALESCE(bitvector2, 0) | 12288
WHERE (extra_flags & 2147483648) != 0 AND value5 = 116;

UPDATE locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 536870912
WHERE (extra_flags & 2147483648) != 0 AND value5 = 26;

UPDATE locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 65536
WHERE (extra_flags & 2147483648) != 0 AND value5 = 32;

UPDATE locker_items
SET bitvector2 = COALESCE(bitvector2, 0) | 64
WHERE (extra_flags & 2147483648) != 0 AND value5 = 8;

UPDATE locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 64
WHERE (extra_flags & 2147483648) != 0 AND value5 = 378;

UPDATE locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 32768
WHERE (extra_flags & 2147483648) != 0 AND value5 = 37;

UPDATE locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 32
WHERE (extra_flags & 2147483648) != 0 AND value5 = 21;

UPDATE locker_items
SET bitvector2 = COALESCE(bitvector2, 0) | 16
WHERE (extra_flags & 2147483648) != 0 AND value5 = 4;

UPDATE locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 16
WHERE (extra_flags & 2147483648) != 0 AND value5 = 25;

-- fix account_locker_items encrusted items
UPDATE account_locker_items
SET bitvector2 = COALESCE(bitvector2, 0) | 12288
WHERE (extra_flags & 2147483648) != 0 AND value5 = 116;

UPDATE account_locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 536870912
WHERE (extra_flags & 2147483648) != 0 AND value5 = 26;

UPDATE account_locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 65536
WHERE (extra_flags & 2147483648) != 0 AND value5 = 32;

UPDATE account_locker_items
SET bitvector2 = COALESCE(bitvector2, 0) | 64
WHERE (extra_flags & 2147483648) != 0 AND value5 = 8;

UPDATE account_locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 64
WHERE (extra_flags & 2147483648) != 0 AND value5 = 378;

UPDATE account_locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 32768
WHERE (extra_flags & 2147483648) != 0 AND value5 = 37;

UPDATE account_locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 32
WHERE (extra_flags & 2147483648) != 0 AND value5 = 21;

UPDATE account_locker_items
SET bitvector2 = COALESCE(bitvector2, 0) | 16
WHERE (extra_flags & 2147483648) != 0 AND value5 = 4;

UPDATE account_locker_items
SET bitvector1 = COALESCE(bitvector1, 0) | 16
WHERE (extra_flags & 2147483648) != 0 AND value5 = 25;

-- fix weightless containers (vnums 32841, 19780, 96443 should have weight -3000)
UPDATE player_items SET weight = -3000 WHERE vnum IN (32841, 19780, 96443) AND (weight IS NULL OR weight >= 0);
UPDATE locker_items SET weight = -3000 WHERE vnum IN (32841, 19780, 96443) AND (weight IS NULL OR weight >= 0);
UPDATE account_locker_items SET weight = -3000 WHERE vnum IN (32841, 19780, 96443) AND (weight IS NULL OR weight >= 0);
UPDATE corpse_items SET weight = -3000 WHERE vnum IN (32841, 19780, 96443) AND (weight IS NULL OR weight >= 0);

-- fix player_items encrusted items (in case any were missed)
UPDATE player_items
SET bitvector2 = COALESCE(bitvector2, 0) | 12288
WHERE (extra_flags & 2147483648) != 0 AND value5 = 116 AND (bitvector2 IS NULL OR (bitvector2 & 12288) = 0);

UPDATE player_items
SET bitvector1 = COALESCE(bitvector1, 0) | 536870912
WHERE (extra_flags & 2147483648) != 0 AND value5 = 26 AND (bitvector1 IS NULL OR (bitvector1 & 536870912) = 0);

UPDATE player_items
SET bitvector1 = COALESCE(bitvector1, 0) | 65536
WHERE (extra_flags & 2147483648) != 0 AND value5 = 32 AND (bitvector1 IS NULL OR (bitvector1 & 65536) = 0);

UPDATE player_items
SET bitvector2 = COALESCE(bitvector2, 0) | 64
WHERE (extra_flags & 2147483648) != 0 AND value5 = 8 AND (bitvector2 IS NULL OR (bitvector2 & 64) = 0);

UPDATE player_items
SET bitvector1 = COALESCE(bitvector1, 0) | 64
WHERE (extra_flags & 2147483648) != 0 AND value5 = 378 AND (bitvector1 IS NULL OR (bitvector1 & 64) = 0);

UPDATE player_items
SET bitvector1 = COALESCE(bitvector1, 0) | 32768
WHERE (extra_flags & 2147483648) != 0 AND value5 = 37 AND (bitvector1 IS NULL OR (bitvector1 & 32768) = 0);

UPDATE player_items
SET bitvector1 = COALESCE(bitvector1, 0) | 32
WHERE (extra_flags & 2147483648) != 0 AND value5 = 21 AND (bitvector1 IS NULL OR (bitvector1 & 32) = 0);

UPDATE player_items
SET bitvector2 = COALESCE(bitvector2, 0) | 16
WHERE (extra_flags & 2147483648) != 0 AND value5 = 4 AND (bitvector2 IS NULL OR (bitvector2 & 16) = 0);

UPDATE player_items
SET bitvector1 = COALESCE(bitvector1, 0) | 16
WHERE (extra_flags & 2147483648) != 0 AND value5 = 25 AND (bitvector1 IS NULL OR (bitvector1 & 16) = 0);

-- show how many items were affected
SELECT 'weightless containers fixed (player_items):' as info, COUNT(*) as count
FROM player_items WHERE vnum IN (32841, 19780, 96443) AND weight = -3000;

SELECT 'player_items encrusted count:' as info, COUNT(*) as count
FROM player_items WHERE (extra_flags & 2147483648) != 0;

SELECT 'locker_items encrusted count:' as info, COUNT(*) as count
FROM locker_items WHERE (extra_flags & 2147483648) != 0;

SELECT 'account_locker_items encrusted count:' as info, COUNT(*) as count
FROM account_locker_items WHERE (extra_flags & 2147483648) != 0;

-- ============================================================================
-- FILE: schema_migration_v15.sql
-- ============================================================================
-- create corpse_item_extra_descr and locker_item_extra_descr tables and add description columns to corpse table
-- schema migration v15
--
-- Idempotency: CREATE TABLE uses IF NOT EXISTS. ALTER TABLEs guarded with
-- information_schema checks.

CREATE TABLE IF NOT EXISTS corpse_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_corpse_item_ed FOREIGN KEY (item_id)
    REFERENCES corpse_items(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS locker_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_locker_item_ed FOREIGN KEY (item_id)
    REFERENCES locker_items(id) ON DELETE CASCADE
);

-- corpses: short_descr
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpses' AND column_name = 'short_descr');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpses ADD COLUMN short_descr VARCHAR(512) DEFAULT NULL AFTER created_at',
    'SELECT "short_descr already exists on corpses"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- corpses: description
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpses' AND column_name = 'description');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpses ADD COLUMN description TEXT DEFAULT NULL AFTER short_descr',
    'SELECT "description already exists on corpses"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_data: CONVERT TO CHARACTER SET (idempotent - no-op if already utf8mb4)
-- Only run if not already utf8mb4_0900_ai_ci
SET @charset_ok = (SELECT COUNT(*) FROM information_schema.tables
    WHERE table_schema = DATABASE() AND table_name = 'player_data'
    AND table_collation = 'utf8mb4_0900_ai_ci');
SET @sql = IF(@charset_ok = 0,
    'ALTER TABLE player_data CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci',
    'SELECT "player_data already utf8mb4_0900_ai_ci"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- ============================================================================
-- FILE: schema_migration_v17_schema_fixes.sql
-- ============================================================================
-- schema_migration_v17_schema_fixes.sql
-- Adds missing columns to tables that the wip-async code expects but
-- which were not present in the base schema migration (run_this_one.sql).
--
-- Idempotency: all ALTER TABLEs are guarded with information_schema checks.

-- corpse_items: item_type
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpse_items ADD COLUMN item_type TINYINT DEFAULT NULL',
    'SELECT "item_type already exists on corpse_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_items: item_type
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_items' AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_items ADD COLUMN item_type TINYINT DEFAULT NULL',
    'SELECT "item_type already exists on player_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_items: wear_flags
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_items' AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_items ADD COLUMN wear_flags INT DEFAULT NULL',
    'SELECT "wear_flags already exists on player_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- auction tables (fresh installs and pre-existing databases)
CREATE TABLE IF NOT EXISTS `auction_bid_history` (
  `id` int(11) NOT NULL auto_increment,
  `date` int(11) NOT NULL default '0',
  `auction_id` int(11) NOT NULL default '0',
  `bidder_pid` int(11) NOT NULL default '0',
  `bidder_name` varchar(32) NOT NULL default '',
  `bid_amount` int(11) NOT NULL default '0',
  PRIMARY KEY (`id`),
  KEY `auction_id` (`auction_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `auction_item_pickups` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `pid` int(10) unsigned NOT NULL default '0',
  `obj_blob_str` blob NOT NULL,
  `retrieved` tinyint(1) NOT NULL default '0',
  `quantity` int(11) NOT NULL default '1',
  PRIMARY KEY (`id`),
  KEY `pid` (`pid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `auction_money_pickups` (
  `pid` int(10) unsigned NOT NULL default '0',
  `money` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY (`pid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `auctions` (
  `id` int(11) NOT NULL auto_increment,
  `seller_pid` int(10) unsigned NOT NULL default '0',
  `seller_name` varchar(32) NOT NULL default '',
  `start_time` TIMESTAMP NULL DEFAULT NULL,
  `end_time` TIMESTAMP NULL DEFAULT NULL,
  `status` int(11) NOT NULL default '1',
  `winning_bidder_pid` int(11) NOT NULL default '0',
  `winning_bidder_name` varchar(32) NOT NULL default '',
  `cur_price` int(10) unsigned NOT NULL default '0',
  `buy_price` int(11) NOT NULL default '0',
  `obj_short` varchar(255) NOT NULL default '',
  `obj_vnum` int(11) NOT NULL default '0',
  `obj_blob_str` blob NOT NULL,
  `id_keywords` varchar(1024) NOT NULL default '',
  `quantity` int(11) NOT NULL default '1',
  PRIMARY KEY (`id`),
  KEY `seller_pid` (`seller_pid`),
  KEY `auction_end` (`end_time`),
  KEY `status` (`status`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- auctions: obj_info_text
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'auctions' AND column_name = 'obj_info_text');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE auctions ADD COLUMN obj_info_text TEXT NULL',
    'SELECT "obj_info_text already exists on auctions"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- ============================================================================
-- FILE: schema_migration_v18_item_table_normalization.sql
-- ============================================================================
-- schema_migration_v18_item_table_normalization.sql
-- Consolidates three previously-planned separate migrations into one:
--   (a) Extend v17 item_type + wear_flags to all item tables (§10.6)
--   (b) Add bitvector1-5 to item tables that lack them (§10.7)
--   (c) Create extra_descr tables for item tables that lack them (§10.8)
--
-- All statements are idempotent (use IF NOT EXISTS / information_schema checks).
-- Runs on every boot via cycle_mud.sh, safe for repeated execution.


-- ============================================================================
-- Part 1: item_type column — present on player_items + corpse_items (v17),
--         missing from locker_items, shopkeeper_items, saved_items,
--         siege_items, account_locker_items, player_pet_items (§10.6)
-- ============================================================================

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items ADD COLUMN item_type TINYINT DEFAULT NULL',
    'SELECT "locker_items.item_type already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'shopkeeper_items' AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE shopkeeper_items ADD COLUMN item_type TINYINT DEFAULT NULL',
    'SELECT "shopkeeper_items.item_type already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'saved_items' AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE saved_items ADD COLUMN item_type TINYINT DEFAULT NULL',
    'SELECT "saved_items.item_type already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'siege_items' AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE siege_items ADD COLUMN item_type TINYINT DEFAULT NULL',
    'SELECT "siege_items.item_type already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_locker_items' AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_locker_items ADD COLUMN item_type TINYINT DEFAULT NULL',
    'SELECT "account_locker_items.item_type already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_pet_items ADD COLUMN item_type TINYINT DEFAULT NULL',
    'SELECT "player_pet_items.item_type already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;


-- ============================================================================
-- Part 2: wear_flags column — present only on player_items (v17),
--         missing from corpse_items, locker_items, shopkeeper_items,
--         saved_items, siege_items, account_locker_items, player_pet_items (§10.6)
-- ============================================================================

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpse_items ADD COLUMN wear_flags INT DEFAULT NULL',
    'SELECT "corpse_items.wear_flags already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items ADD COLUMN wear_flags INT DEFAULT NULL',
    'SELECT "locker_items.wear_flags already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'shopkeeper_items' AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE shopkeeper_items ADD COLUMN wear_flags INT DEFAULT NULL',
    'SELECT "shopkeeper_items.wear_flags already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'saved_items' AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE saved_items ADD COLUMN wear_flags INT DEFAULT NULL',
    'SELECT "saved_items.wear_flags already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'siege_items' AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE siege_items ADD COLUMN wear_flags INT DEFAULT NULL',
    'SELECT "siege_items.wear_flags already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_locker_items' AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_locker_items ADD COLUMN wear_flags INT DEFAULT NULL',
    'SELECT "account_locker_items.wear_flags already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_pet_items ADD COLUMN wear_flags INT DEFAULT NULL',
    'SELECT "player_pet_items.wear_flags already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;


-- ============================================================================
-- Part 3: bitvector1-5 columns — present on player_items, locker_items (v14),
--         and account_locker_items (v14).
--         Missing from corpse_items, shopkeeper_items, saved_items,
--         siege_items, player_pet_items (§10.7)
-- ============================================================================

-- helper: add a bitvector column to a table if missing
-- Using inline repetition (Mysql 5.7 compatible — no reusable procedures in idempotent migration)

-- ---- corpse_items ----

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpse_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "corpse_items.bitvector1 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpse_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "corpse_items.bitvector2 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpse_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "corpse_items.bitvector3 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpse_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "corpse_items.bitvector4 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpse_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "corpse_items.bitvector5 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- ---- shopkeeper_items ----

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'shopkeeper_items' AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE shopkeeper_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "shopkeeper_items.bitvector1 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'shopkeeper_items' AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE shopkeeper_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "shopkeeper_items.bitvector2 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'shopkeeper_items' AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE shopkeeper_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "shopkeeper_items.bitvector3 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'shopkeeper_items' AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE shopkeeper_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "shopkeeper_items.bitvector4 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'shopkeeper_items' AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE shopkeeper_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "shopkeeper_items.bitvector5 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- ---- saved_items ----

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'saved_items' AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE saved_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "saved_items.bitvector1 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'saved_items' AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE saved_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "saved_items.bitvector2 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'saved_items' AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE saved_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "saved_items.bitvector3 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'saved_items' AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE saved_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "saved_items.bitvector4 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'saved_items' AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE saved_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "saved_items.bitvector5 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- ---- siege_items ----

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'siege_items' AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE siege_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "siege_items.bitvector1 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'siege_items' AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE siege_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "siege_items.bitvector2 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'siege_items' AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE siege_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "siege_items.bitvector3 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'siege_items' AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE siege_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "siege_items.bitvector4 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'siege_items' AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE siege_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "siege_items.bitvector5 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- ---- player_pet_items ----

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_pet_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "player_pet_items.bitvector1 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_pet_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "player_pet_items.bitvector2 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_pet_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "player_pet_items.bitvector3 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_pet_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "player_pet_items.bitvector4 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_pet_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL',
    'SELECT "player_pet_items.bitvector5 already exists"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;


-- ============================================================================
-- Part 4: extra_descr tables — present for player_items, corpse_items,
--         locker_items, player_pet_items.
--         Missing for shopkeeper_items, saved_items, siege_items,
--         account_locker_items (§10.8)
-- ============================================================================

CREATE TABLE IF NOT EXISTS shopkeeper_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_shopkeeper_item_ed FOREIGN KEY (item_id)
    REFERENCES shopkeeper_items(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS saved_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_saved_item_ed FOREIGN KEY (item_id)
    REFERENCES saved_items(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS siege_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_siege_item_ed FOREIGN KEY (item_id)
    REFERENCES siege_items(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS account_locker_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_account_locker_item_ed FOREIGN KEY (item_id)
    REFERENCES account_locker_items(id) ON DELETE CASCADE
);


-- ============================================================================
-- Summary:
--   item_type added to:      locker_items, shopkeeper_items, saved_items,
--                            siege_items, account_locker_items, player_pet_items
--   wear_flags added to:     corpse_items, locker_items, shopkeeper_items,
--                            saved_items, siege_items, account_locker_items,
--                            player_pet_items
--   bitvector1-5 added to:   corpse_items, shopkeeper_items, saved_items,
--                            siege_items, player_pet_items
--   extra_descr tables for:  shopkeeper_items, saved_items, siege_items,
--                            account_locker_items
--
-- After this migration, ALL 8 item tables have a uniform column set:
--   player_items, corpse_items, locker_items, account_locker_items,
--   shopkeeper_items, saved_items, siege_items, player_pet_items
-- ============================================================================

-- ============================================================================
-- FILE: schema_migration_v18_player_affects_unique.sql
-- ============================================================================
-- schema_migration_v18_player_affects_unique.sql
-- Phase 3.4 follow-up: add UNIQUE KEY to player_affects so it can be
-- converted to REPLACE INTO like the other 7 array-save tables.
--
-- IMPORTANT: This migration MUST run before the production code is updated
-- to use REPLACE INTO. The order of operations is:
--   1. Deduplicate existing rows (keep the row with the lowest id per group)
--   2. Add the UNIQUE KEY (will fail if duplicates still exist)
--   3. Source code updated to use REPLACE INTO
--
-- Idempotency: the dedup DELETE is safe to run multiple times (no-op if no
-- duplicates). The ALTER TABLE will fail on second run because the key
-- already exists, but the `|| true` in cycle_mud.sh swallows that error.
--
-- The unique key includes all columns that can distinguish semantically
-- different affects: (pid, type, duration, flags, modifier, location, level).
-- bitvector1-5 are excluded because they are derived from the spell type
-- and are the same for the same spell. Two affects of the same type on the
-- same character with the same duration, flags, modifier, location, and
-- level are semantically identical and can be safely collapsed.

-- Step 1: Remove any existing duplicate rows, keeping the lowest id in each
-- duplicate group.
DELETE p1
FROM player_affects p1
INNER JOIN player_affects p2
        ON p1.pid = p2.pid
       AND p1.type = p2.type
       AND p1.duration = p2.duration
       AND p1.flags = p2.flags
       AND p1.modifier = p2.modifier
       AND p1.location = p2.location
       AND p1.level = p2.level
       AND p1.id > p2.id;

-- Step 2: Add the UNIQUE KEY. Uses the idempotent prepared-statement
-- pattern so the migration is safe to re-run.
-- Note: if duplicates already exist, this step will fail rather than deleting data.
SET @key_exists := (
    SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE()
      AND table_name = 'player_affects'
      AND index_name = 'uk_pid_type_dur_flags_mod_loc_lvl'
);

SET @sql := IF(@key_exists = 0,
    'ALTER TABLE player_affects ADD UNIQUE KEY uk_pid_type_dur_flags_mod_loc_lvl (pid, type, duration, flags, modifier, location, level)',
    'DO 0'  -- no-op
);

PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- ============================================================================
-- FILE: schema_migration_v19_item_table_columns.sql
-- ============================================================================
-- ===================================================================
-- schema_migration_v19_item_table_columns.sql
--
-- Phase 3.5 (consolidated schema migration):
-- Adds item_type, wear_flags, bitvector1-5 columns to the 7 item
-- storage tables that were missing them.  These columns exist on
-- player_items (since v17) and are referenced by the production
-- INSERT statements in src/sql_player.c, but the tables below were
-- never updated.  Without this migration, every save to these tables
-- fails with MySQL error 1054 (Unknown column 'item_type' in 'field
-- list') for the two tables that already reference them, and
-- silently loses wear/encrusted-affect data for the rest.
--
-- Column types match player_items exactly:
--   item_type   TINYINT          DEFAULT NULL
--   wear_flags  INT              DEFAULT NULL
--   bitvector1  BIGINT UNSIGNED  DEFAULT NULL
--   bitvector2  BIGINT UNSIGNED  DEFAULT NULL
--   bitvector3  BIGINT UNSIGNED  DEFAULT NULL
--   bitvector4  BIGINT UNSIGNED  DEFAULT NULL
--   bitvector5  BIGINT UNSIGNED  DEFAULT NULL
--
-- The 7 tables covered:
--   corpse_items, locker_items, shopkeeper_items, siege_items,
--   saved_items, player_pet_items, account_locker_items
--
-- Idempotency: each ALTER is wrapped in a prepared statement that
-- checks information_schema.columns first.  If the column already
-- exists, the prepared statement executes `DO 0` as a no-op.  This
-- makes the migration safe to re-run on a database that has already
-- been upgraded (the cycle_mud.sh wrapper also uses `|| true` as a
-- belt-and-suspenders guard).
-- ===================================================================

-- -------------------------------------------------------------------
-- corpse_items
-- -------------------------------------------------------------------
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'corpse_items'
                     AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE corpse_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER extra_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'corpse_items'
                     AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE corpse_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER item_type',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'corpse_items'
                     AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE corpse_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER wear_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'corpse_items'
                     AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE corpse_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'corpse_items'
                     AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE corpse_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'corpse_items'
                     AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE corpse_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'corpse_items'
                     AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE corpse_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- -------------------------------------------------------------------
-- locker_items
-- -------------------------------------------------------------------
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'locker_items'
                     AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE locker_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER extra_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'locker_items'
                     AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE locker_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER item_type',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'locker_items'
                     AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE locker_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER wear_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'locker_items'
                     AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE locker_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'locker_items'
                     AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE locker_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'locker_items'
                     AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE locker_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'locker_items'
                     AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE locker_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- -------------------------------------------------------------------
-- shopkeeper_items
-- -------------------------------------------------------------------
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'shopkeeper_items'
                     AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE shopkeeper_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER extra_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'shopkeeper_items'
                     AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE shopkeeper_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER item_type',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'shopkeeper_items'
                     AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE shopkeeper_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER wear_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'shopkeeper_items'
                     AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE shopkeeper_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'shopkeeper_items'
                     AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE shopkeeper_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'shopkeeper_items'
                     AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE shopkeeper_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'shopkeeper_items'
                     AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE shopkeeper_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- -------------------------------------------------------------------
-- siege_items
-- -------------------------------------------------------------------
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'siege_items'
                     AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE siege_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER extra_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'siege_items'
                     AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE siege_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER item_type',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'siege_items'
                     AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE siege_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER wear_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'siege_items'
                     AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE siege_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'siege_items'
                     AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE siege_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'siege_items'
                     AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE siege_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'siege_items'
                     AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE siege_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- -------------------------------------------------------------------
-- saved_items
-- -------------------------------------------------------------------
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'saved_items'
                     AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE saved_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER extra_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'saved_items'
                     AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE saved_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER item_type',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'saved_items'
                     AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE saved_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER wear_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'saved_items'
                     AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE saved_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'saved_items'
                     AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE saved_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'saved_items'
                     AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE saved_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'saved_items'
                     AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE saved_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- -------------------------------------------------------------------
-- player_pet_items
-- -------------------------------------------------------------------
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'player_pet_items'
                     AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE player_pet_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER extra_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'player_pet_items'
                     AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE player_pet_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER item_type',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'player_pet_items'
                     AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE player_pet_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER wear_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'player_pet_items'
                     AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE player_pet_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'player_pet_items'
                     AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE player_pet_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'player_pet_items'
                     AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE player_pet_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'player_pet_items'
                     AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE player_pet_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- -------------------------------------------------------------------
-- account_locker_items
-- -------------------------------------------------------------------
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'account_locker_items'
                     AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE account_locker_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER extra_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'account_locker_items'
                     AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE account_locker_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER item_type',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'account_locker_items'
                     AND column_name = 'bitvector1');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE account_locker_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER wear_flags',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'account_locker_items'
                     AND column_name = 'bitvector2');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE account_locker_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'account_locker_items'
                     AND column_name = 'bitvector3');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE account_locker_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'account_locker_items'
                     AND column_name = 'bitvector4');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE account_locker_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                     AND table_name = 'account_locker_items'
                     AND column_name = 'bitvector5');
SET @sql = IF(@col_exists = 0,
              'ALTER TABLE account_locker_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4',
              'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- ============================================================================
-- FILE: schema_migration_v20_item_material.sql
-- ============================================================================
-- Migration: add item_material column to the 7 item tables
-- Phase 3.6: persist obj->material as a diff-from-prototype value
--   NULL = item matches its prototype's material (load code uses proto)
--   non-NULL = item has a custom material override
--
-- The shared helper sql_format_item_diff_fields_and_free_proto() in
-- src/sql_player.c writes "NULL" when the item's material matches its
-- prototype, or the numeric value otherwise.
--
-- This migration adds the column to the same 7 tables that received the
-- v19 diff columns (item_type, wear_flags, bitvector1-5):
--   corpse_items, locker_items, shopkeeper_items, siege_items,
--   saved_items, player_pet_items, account_locker_items
--
-- Idempotency: all ALTER TABLEs are guarded with information_schema checks.

-- corpse_items
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpse_items ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5',
    'SELECT "item_material already exists on corpse_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- locker_items
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5',
    'SELECT "item_material already exists on locker_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- shopkeeper_items
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'shopkeeper_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE shopkeeper_items ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5',
    'SELECT "item_material already exists on shopkeeper_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- siege_items
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'siege_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE siege_items ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5',
    'SELECT "item_material already exists on siege_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- saved_items
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'saved_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE saved_items ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5',
    'SELECT "item_material already exists on saved_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- player_pet_items
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_pet_items ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5',
    'SELECT "item_material already exists on player_pet_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- account_locker_items
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_locker_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_locker_items ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5',
    'SELECT "item_material already exists on account_locker_items"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- ============================================================================
-- FILE: schema_migration_v21_player_item_material.sql
-- ============================================================================
-- Migration: add item_material to player_items
--
-- sql_save_player() and sql_load_player_items() now persist item_material
-- for player equipment/inventory. The base schema created by run_this_one.sql
-- predates that change, so old/new databases need this migration.
--
-- Idempotent: safe to re-run.

SET @col_exists = (
    SELECT COUNT(*)
    FROM information_schema.columns
    WHERE table_schema = DATABASE()
      AND table_name = 'player_items'
      AND column_name = 'item_material'
);

SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_items ADD COLUMN item_material TINYINT DEFAULT NULL AFTER bitvector5',
    'DO 0');

PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- ============================================================================
-- FILE: create_polls_tables.sql
-- ============================================================================
-- create polls tables if they don't exist
-- run with: mysql -u duris -p duris < migrations/create_polls_tables.sql

CREATE TABLE IF NOT EXISTS `polls` (
  `id` int(11) NOT NULL auto_increment,
  `question` varchar(512) NOT NULL,
  `created_by` varchar(32) NOT NULL,
  `created_at` int(11) NOT NULL default '0',
  `expires_at` int(11) NOT NULL default '0',
  `is_active` tinyint(1) NOT NULL default '1',
  `multi_select` tinyint(1) NOT NULL default '0',
  `max_choices` int(11) NOT NULL default '1',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `poll_options` (
  `id` int(11) NOT NULL auto_increment,
  `poll_id` int(11) NOT NULL,
  `option_num` int(11) NOT NULL,
  `option_text` varchar(256) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `poll_id` (`poll_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE IF NOT EXISTS `poll_votes` (
  `id` int(11) NOT NULL auto_increment,
  `poll_id` int(11) NOT NULL,
  `account_name` varchar(64) NOT NULL,
  `option_id` int(11) NOT NULL,
  `voted_at` int(11) NOT NULL default '0',
  `char_name` varchar(32) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `unique_vote` (`poll_id`, `account_name`, `option_id`),
  KEY `poll_id` (`poll_id`),
  KEY `account_name` (`account_name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- ============================================================================
-- FILE: epic-zone-payout.sql
-- ============================================================================
-- non-destructive, can be applied like:
-- mysql -uduris -pduris duris_dev <epic-zone-payout.sql

-- let's not have group size affect payout until we get the numbers right
UPDATE zones SET suggested_group_size = 100 WHERE epic_type != '0';

-- later on we might want to try something like this:
-- UPDATE zones SET suggested_group_size = 4 WHERE epic_type = '1';
-- UPDATE zones SET suggested_group_size = 8 WHERE epic_type = '2';
-- UPDATE zones SET suggested_group_size = 12 WHERE epic_type = '3';

-- adjusted values based on discord feedback, generally raised the low zones and lowered the high
UPDATE zones SET epic_payout = 0 WHERE number = 1389; -- ironstar (broken)
UPDATE zones SET epic_payout = 80 WHERE number = 400; -- nizari
UPDATE zones SET epic_payout = 80 WHERE number = 93; -- tower of high sorcery
UPDATE zones SET epic_payout = 80 WHERE number = 740; -- bloodstone keep
UPDATE zones SET epic_payout = 80 WHERE number = 14; -- kobold settlement
UPDATE zones SET epic_payout = 80 WHERE number = 90; -- orrak
UPDATE zones SET epic_payout = 80 WHERE number = 383; -- werrun
UPDATE zones SET epic_payout = 90 WHERE number = 264; -- myrloch vale
UPDATE zones SET epic_payout = 90 WHERE number = 140; -- faerie realm
UPDATE zones SET epic_payout = 90 WHERE number = 823; -- swamp lab
UPDATE zones SET epic_payout = 90 WHERE number = 370; -- crystalspyre
UPDATE zones SET epic_payout = 90 WHERE number = 38; -- elemental groves
UPDATE zones SET epic_payout = 90 WHERE number = 879; -- kelek
UPDATE zones SET epic_payout = 90 WHERE number = 113; -- outcast tower
UPDATE zones SET epic_payout = 90 WHERE number = 143; -- nakral
UPDATE zones SET epic_payout = 100 WHERE number = 191; -- high moor
UPDATE zones SET epic_payout = 100 WHERE number = 342; -- clan stoutdorf (drst)
UPDATE zones SET epic_payout = 100 WHERE number = 285; -- pharr valley
UPDATE zones SET epic_payout = 100 WHERE number = 67; -- court of the muse
UPDATE zones SET epic_payout = 100 WHERE number = 381; -- fort boyard
UPDATE zones SET epic_payout = 100 WHERE number = 27; -- headless
UPDATE zones SET epic_payout = 100 WHERE number = 429; -- drustl
UPDATE zones SET epic_payout = 100 WHERE number = 805; -- skrentherlog (yan-ti)
UPDATE zones SET epic_payout = 100 WHERE number = 133; -- caverns of armageddon
UPDATE zones SET epic_payout = 100 WHERE number = 183; -- temple of flames
UPDATE zones SET epic_payout = 100 WHERE number = 130; -- citadel
UPDATE zones SET epic_payout = 100 WHERE number = 666; -- torrhan
UPDATE zones SET epic_payout = 100 WHERE number = 1320; -- githyanki fortress
UPDATE zones SET epic_payout = 100 WHERE number = 220; -- pits of cerebus
UPDATE zones SET epic_payout = 100 WHERE number = 755; -- dark stone tower
UPDATE zones SET epic_payout = 110 WHERE number = 758; -- rogue plains
UPDATE zones SET epic_payout = 110 WHERE number = 73; -- carthapia
UPDATE zones SET epic_payout = 110 WHERE number = 824; -- temple of the sun
UPDATE zones SET epic_payout = 110 WHERE number = 662; -- tharn ruins
UPDATE zones SET epic_payout = 110 WHERE number = 664; -- battlefield
UPDATE zones SET epic_payout = 120 WHERE number = 430; -- boyard prison
UPDATE zones SET epic_payout = 120 WHERE number = 773; -- desolate under fire
UPDATE zones SET epic_payout = 120 WHERE number = 490; -- venan'trut
UPDATE zones SET epic_payout = 120 WHERE number = 710; -- fields between
UPDATE zones SET epic_payout = 130 WHERE number = 200; -- krethik keep
UPDATE zones SET epic_payout = 130 WHERE number = 766; -- jade empire
UPDATE zones SET epic_payout = 150 WHERE number = 760; -- ultarium
UPDATE zones SET epic_payout = 150 WHERE number = 570; -- trakkia
UPDATE zones SET epic_payout = 150 WHERE number = 91; -- mountain of the banished
UPDATE zones SET epic_payout = 175 WHERE number = 318; -- ice tower
UPDATE zones SET epic_payout = 175 WHERE number = 50; -- labyrinth
UPDATE zones SET epic_payout = 200 WHERE number = 970; -- icecrag castle
UPDATE zones SET epic_payout = 200 WHERE number = 920; -- undermountain
UPDATE zones SET epic_payout = 200 WHERE number = 213; -- mazzolin
UPDATE zones SET epic_payout = 225 WHERE number = 24; -- quintaragon
UPDATE zones SET epic_payout = 225 WHERE number = 244; -- plane of air
UPDATE zones SET epic_payout = 225 WHERE number = 254; -- plane of fire
UPDATE zones SET epic_payout = 225 WHERE number = 197; -- astral plane
UPDATE zones SET epic_payout = 250 WHERE number = 151; -- new cave city
UPDATE zones SET epic_payout = 250 WHERE number = 780; -- tribal oasis
UPDATE zones SET epic_payout = 250 WHERE number = 412; -- shadamehr
UPDATE zones SET epic_payout = 260 WHERE number = 87; -- gibberling
UPDATE zones SET epic_payout = 260 WHERE number = 368; -- domain
UPDATE zones SET epic_payout = 275 WHERE number = 35; -- forgotten mansion
UPDATE zones SET epic_payout = 275 WHERE number = 448; -- keep of evil
UPDATE zones SET epic_payout = 275 WHERE number = 756; -- obsidian citadel
UPDATE zones SET epic_payout = 275 WHERE number = 261; -- swamp troll
UPDATE zones SET epic_payout = 285 WHERE number = 419; -- forest of mir
UPDATE zones SET epic_payout = 285 WHERE number = 162; -- transparent tower
UPDATE zones SET epic_payout = 300 WHERE number = 709; -- hall of knighthood
UPDATE zones SET epic_payout = 300 WHERE number = 238; -- plane of earth
UPDATE zones SET epic_payout = 300 WHERE number = 124; -- ethereal plane
UPDATE zones SET epic_payout = 315 WHERE number = 784; -- tharn rifts
UPDATE zones SET epic_payout = 315 WHERE number = 831; -- alatorin
UPDATE zones SET epic_payout = 325 WHERE number = 386; -- sevenoaks
UPDATE zones SET epic_payout = 325 WHERE number = 229; -- ny'neth
UPDATE zones SET epic_payout = 325 WHERE number = 289; -- kingdom of torg
UPDATE zones SET epic_payout = 325 WHERE number = 960; -- jotunheim
UPDATE zones SET epic_payout = 335 WHERE number = 441; -- tikitzopl (51)
UPDATE zones SET epic_payout = 345 WHERE number = 215; -- aravne
UPDATE zones SET epic_payout = 350 WHERE number = 989; -- tezcat
UPDATE zones SET epic_payout = 350 WHERE number = 315; -- sea kingdom
UPDATE zones SET epic_payout = 350 WHERE number = 367; -- arachdrathos guilds
UPDATE zones SET epic_payout = 350 WHERE number = 1200; -- depths
UPDATE zones SET epic_payout = 350 WHERE number = 1398; -- smoke plane
UPDATE zones SET epic_payout = 350 WHERE number = 232; -- plane of water
UPDATE zones SET epic_payout = 400 WHERE number = 328; -- shaboath (51)
UPDATE zones SET epic_payout = 400 WHERE number = 159; -- pit of dragons
UPDATE zones SET epic_payout = 400 WHERE number = 435; -- temple of earth (52)
UPDATE zones SET epic_payout = 400 WHERE number = 712; -- scorched valley
UPDATE zones SET epic_payout = 400 WHERE number = 326; -- fortress of dreams
UPDATE zones SET epic_payout = 425 WHERE number = 910; -- barovia
UPDATE zones SET epic_payout = 425 WHERE number = 877; -- snow ogres
UPDATE zones SET epic_payout = 425 WHERE number = 777; -- hall of ancients
UPDATE zones SET epic_payout = 450 WHERE number = 883; -- charcoal palace (51)
UPDATE zones SET epic_payout = 450 WHERE number = 1316; -- tempest court
UPDATE zones SET epic_payout = 500 WHERE number = 814; -- ceothia (53)
UPDATE zones SET epic_payout = 500 WHERE number = 230; -- ny'neth 2
UPDATE zones SET epic_payout = 500 WHERE number = 1390; -- brass (52)
UPDATE zones SET epic_payout = 550 WHERE number = 444; -- githzerai stronghold (52)
UPDATE zones SET epic_payout = 550 WHERE number = 588; -- barovia 2
UPDATE zones SET epic_payout = 550 WHERE number = 1424; -- mausoleum
UPDATE zones SET epic_payout = 600 WHERE number = 1300; -- vecna (54)
UPDATE zones SET epic_payout = 600 WHERE number = 68; -- dragonnia
UPDATE zones SET epic_payout = 650 WHERE number = 196; -- tiamat (53)
UPDATE zones SET epic_payout = 700 WHERE number = 345; -- apocalypse castle (54)
UPDATE zones SET epic_payout = 700 WHERE number = 257; -- bahamut (54)
UPDATE zones SET epic_payout = 800 WHERE number = 266; -- negative plane (55)
UPDATE zones SET epic_payout = 800 WHERE number = 324; -- bronze citadel (55)
UPDATE zones SET epic_payout = 850 WHERE number = 4200; -- dreadnaught
UPDATE zones SET epic_payout = 900 WHERE number = 387; -- ny'neth 3 (56)
UPDATE zones SET epic_payout = 900 WHERE number = 455; -- celestia (56)
UPDATE zones SET epic_payout = 950 WHERE number = 875; -- 222 (56)
UPDATE zones SET epic_payout = 1000 WHERE number = 583; -- ravenloft (56)


-- ============================================================================
-- FILE: add_frag_leaderboard_tables.sql
-- ============================================================================
-- Migration: Add Frag Leaderboard Tables for Web Statistics
-- Date: 2025-11-07 - Arih
-- Purpose: Hybrid approach - MUD uses flat files, web uses database
--
-- This migration adds two tables:
-- 1. account_characters: Maps characters to accounts with soft delete
-- 2. frag_leaderboard: Denormalized leaderboard data for fast web queries
--
-- NO foreign keys to avoid cascading delete issues and maintain MUD stability

-- ============================================================================
-- Table: account_characters
-- Purpose: Track which characters belong to which accounts
-- ============================================================================
CREATE TABLE IF NOT EXISTS `account_characters` (
  `id` int(11) NOT NULL auto_increment,
  `account_name` varchar(255) NOT NULL COMMENT 'Account name from flat file system',
  `pid` bigint(20) NOT NULL COMMENT 'Player ID - unique character identifier',
  `char_name` varchar(255) NOT NULL COMMENT 'Character name',
  `created_at` datetime DEFAULT CURRENT_TIMESTAMP COMMENT 'When character was created',
  `deleted_at` datetime NULL DEFAULT NULL COMMENT 'Soft delete - NULL means active',
  PRIMARY KEY (`id`),
  UNIQUE KEY `pid` (`pid`),
  KEY `account_name` (`account_name`),
  KEY `char_name` (`char_name`),
  KEY `deleted_at` (`deleted_at`),
  KEY `account_active` (`account_name`, `deleted_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Maps characters to accounts for web statistics';

-- ============================================================================
-- Table: frag_leaderboard
-- Purpose: Denormalized frag leaderboard for fast web queries
-- ============================================================================
CREATE TABLE IF NOT EXISTS `frag_leaderboard` (
  `id` int(11) NOT NULL auto_increment,
  `pid` bigint(20) NOT NULL COMMENT 'Player ID - links to character',
  `account_name` varchar(255) NOT NULL COMMENT 'Account name for account-level stats',
  `char_name` varchar(255) NOT NULL COMMENT 'Character name for display',
  `total_frags` int(11) NOT NULL DEFAULT 0 COMMENT 'Total frags (stored as int, divide by 100 for display)',
  `racewar` int(11) NOT NULL COMMENT 'Racewar side (1=good, 2=evil, 3=undead, 4=illithid)',
  `race` varchar(50) DEFAULT NULL COMMENT 'Character race',
  `class` varchar(50) DEFAULT NULL COMMENT 'Character class',
  `level` int(11) DEFAULT NULL COMMENT 'Character level',
  `deleted_at` datetime NULL DEFAULT NULL COMMENT 'Soft delete - NULL means active',
  `last_updated` datetime DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last time record was updated',
  PRIMARY KEY (`id`),
  UNIQUE KEY `pid` (`pid`),
  KEY `char_name` (`char_name`),
  KEY `account_name` (`account_name`),
  KEY `total_frags_active` (`deleted_at`, `total_frags`),
  KEY `racewar_leaderboard` (`deleted_at`, `racewar`, `total_frags`),
  KEY `race_leaderboard` (`deleted_at`, `race`, `total_frags`),
  KEY `class_leaderboard` (`deleted_at`, `class`, `total_frags`),
  KEY `level_range` (`deleted_at`, `level`, `total_frags`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Denormalized frag leaderboard for web statistics';

-- ============================================================================
-- Sample Queries for Web Frontend
-- ============================================================================

-- Top 100 overall (active characters only)
-- SELECT char_name, total_frags / 100.0 AS frags, racewar, race, class, level
-- FROM frag_leaderboard
-- WHERE deleted_at IS NULL
-- ORDER BY total_frags DESC
-- LIMIT 100;

-- Top 50 by racewar (e.g., good=1, evil=2)
-- SELECT char_name, total_frags / 100.0 AS frags, race, class, level
-- FROM frag_leaderboard
-- WHERE deleted_at IS NULL AND racewar = 1
-- ORDER BY total_frags DESC
-- LIMIT 50;

-- Top 50 by race (e.g., 'drow_elf')
-- SELECT char_name, total_frags / 100.0 AS frags, class, level
-- FROM frag_leaderboard
-- WHERE deleted_at IS NULL AND race = 'drow_elf'
-- ORDER BY total_frags DESC
-- LIMIT 50;

-- Top 50 by class (e.g., 'necromancer')
-- SELECT char_name, total_frags / 100.0 AS frags, race, level
-- FROM frag_leaderboard
-- WHERE deleted_at IS NULL AND class = 'necromancer'
-- ORDER BY total_frags DESC
-- LIMIT 50;

-- All characters for an account (including deleted)
-- SELECT char_name, total_frags / 100.0 AS frags, racewar, race, class, level,
--        deleted_at, last_updated
-- FROM frag_leaderboard
-- WHERE account_name = 'Resakse'
-- ORDER BY total_frags DESC;

-- Frag distribution by class
-- SELECT class, AVG(total_frags / 100.0) AS avg_frags,
--        MIN(total_frags / 100.0) AS min_frags,
--        MAX(total_frags / 100.0) AS max_frags,
--        COUNT(*) AS char_count
-- FROM frag_leaderboard
-- WHERE deleted_at IS NULL
-- GROUP BY class
-- ORDER BY avg_frags DESC;

-- Players who crossed milestone (e.g., 1000 frags)
-- SELECT char_name, total_frags / 100.0 AS frags, account_name, race, class
-- FROM frag_leaderboard
-- WHERE deleted_at IS NULL AND total_frags >= 100000
-- ORDER BY total_frags DESC;

-- Monthly frag gains (using existing progress table)
-- SELECT p.pid, fl.char_name, SUM(p.delta) / 100.0 AS frags_gained
-- FROM progress p
-- JOIN frag_leaderboard fl ON p.pid = fl.pid
-- WHERE p.var_type = 'FRAGS'
--   AND p.stamp >= DATE_SUB(NOW(), INTERVAL 1 MONTH)
--   AND fl.deleted_at IS NULL
-- GROUP BY p.pid, fl.char_name
-- ORDER BY frags_gained DESC
-- LIMIT 50;

-- ============================================================================
-- FILE: add_account_characters_columns.sql
-- ============================================================================
-- Migration: Add missing columns and unique key to account_characters
-- Date: 2026-06-14
-- Purpose: Fix account/character lifecycle bugs (ghost characters, PID propagation)
--
-- The account_characters table was missing columns used by sql_save_account_characters
-- (login_count, last_login, blocked, racewar) and was missing a unique key on
-- (account_name, char_name) which is needed for the ON DUPLICATE KEY UPDATE to
-- correctly identify rows when the PID changes (pid=0 -> real PID after save, or
-- new PID after delete-recreate cycle).

-- Add missing columns using information_schema guards so this works on MySQL
-- versions that do not support ALTER TABLE ... ADD COLUMN IF NOT EXISTS.
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'login_count');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN login_count bigint(20) unsigned DEFAULT 0 AFTER char_name',
    'SELECT "login_count already exists on account_characters"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'last_login');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN last_login datetime NULL DEFAULT NULL AFTER login_count',
    'SELECT "last_login already exists on account_characters"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'blocked');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN blocked int(11) DEFAULT 0 AFTER last_login',
    'SELECT "blocked already exists on account_characters"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'racewar');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN racewar int(11) DEFAULT 0 AFTER blocked',
    'SELECT "racewar already exists on account_characters"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- Add unique key on (account_name, char_name) for natural row identification.
-- This is critical: without it, when a character is deleted and re-created
-- (new PID), the ON DUPLICATE KEY UPDATE in sql_save_account_characters won't
-- match the old row and will create a duplicate instead of clearing deleted_at.
SET @key_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND index_name = 'acct_char');
SET @sql = IF(@key_exists = 0,
    'ALTER TABLE account_characters ADD UNIQUE KEY acct_char (account_name, char_name)',
    'SELECT "acct_char already exists on account_characters"');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;


-- Ensure item_type and wear_flags columns are DEFAULT NULL (and not NOT NULL)
-- for locker_items, saved_items, and siege_items. This prevents save failures
-- when C code tries to insert NULL (when properties match their prototype).

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'item_type');
SET @sql = IF(@col_exists = 1,
              'ALTER TABLE locker_items MODIFY COLUMN item_type TINYINT DEFAULT NULL',
              'SELECT "locker_items.item_type not present to modify"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 1,
              'ALTER TABLE locker_items MODIFY COLUMN wear_flags INT DEFAULT NULL',
              'SELECT "locker_items.wear_flags not present to modify"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'saved_items' AND column_name = 'item_type');
SET @sql = IF(@col_exists = 1,
              'ALTER TABLE saved_items MODIFY COLUMN item_type TINYINT DEFAULT NULL',
              'SELECT "saved_items.item_type not present to modify"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'saved_items' AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 1,
              'ALTER TABLE saved_items MODIFY COLUMN wear_flags INT DEFAULT NULL',
              'SELECT "saved_items.wear_flags not present to modify"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'siege_items' AND column_name = 'item_type');
SET @sql = IF(@col_exists = 1,
              'ALTER TABLE siege_items MODIFY COLUMN item_type TINYINT DEFAULT NULL',
              'SELECT "siege_items.item_type not present to modify"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
                   WHERE table_schema = DATABASE() AND table_name = 'siege_items' AND column_name = 'wear_flags');
SET @sql = IF(@col_exists = 1,
              'ALTER TABLE siege_items MODIFY COLUMN wear_flags INT DEFAULT NULL',
              'SELECT "siege_items.wear_flags not present to modify"');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

