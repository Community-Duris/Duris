#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/../.env"

MYSQL_CMD="mysql -h$DB_HOST -u$DB_USER -p$DB_PASSWD $DB_NAME"

STEP=0
TOTAL=54
FAILED=0

run_sql() {
    local desc="$1"
    local sql="$2"
    STEP=$((STEP + 1))
    printf "[%2d/%d] %s... " "$STEP" "$TOTAL" "$desc"

    local tmpfile=$(mktemp)
    echo "$sql" > "$tmpfile"

    local err_file=$(mktemp)
    if $MYSQL_CMD < "$tmpfile" 2>"$err_file"; then
        echo "ok"
    else
        echo "FAILED"
        head -20 "$err_file"
        FAILED=$((FAILED + 1))
    fi
    rm -f "$err_file"
    rm -f "$tmpfile"
}

convert_tables_to_charset() {
    local desc="$1"
    local with_collation="$2"
    STEP=$((STEP + 1))
    printf "[%2d/%d] %s... " "$STEP" "$TOTAL" "$desc"

    local db_charset=""
    local db_collation=""
    local tables=""
    local table_failed=0

    if ! db_charset=$($MYSQL_CMD -N -e "SELECT DEFAULT_CHARACTER_SET_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME=DATABASE();" 2>/dev/null); then
        echo "FAILED"
        FAILED=$((FAILED + 1))
        return 1
    fi

    if [ "$with_collation" = "1" ]; then
        if ! db_collation=$($MYSQL_CMD -N -e "SELECT DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME=DATABASE();" 2>/dev/null); then
            echo "FAILED"
            FAILED=$((FAILED + 1))
            return 1
        fi
    fi

    if ! tables=$($MYSQL_CMD -N -e "SELECT table_name FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type='BASE TABLE';" 2>/dev/null); then
        echo "FAILED"
        FAILED=$((FAILED + 1))
        return 1
    fi

    if [ -n "$tables" ] && [ -n "$db_charset" ] && { [ "$with_collation" != "1" ] || [ -n "$db_collation" ]; }; then
        while IFS= read -r t; do
            [ -z "$t" ] && continue
            local err_file
            err_file=$(mktemp)
            if [ "$with_collation" = "1" ]; then
                if $MYSQL_CMD -e "SET FOREIGN_KEY_CHECKS=0; ALTER TABLE \`$t\` CONVERT TO CHARACTER SET $db_charset COLLATE $db_collation; SET FOREIGN_KEY_CHECKS=1;" >/dev/null 2>"$err_file"; then
                    :
                else
                    if [ "$table_failed" -eq 0 ]; then
                        echo "FAILED"
                    fi
                    table_failed=1
                    head -20 "$err_file"
                fi
            else
                if $MYSQL_CMD -e "SET FOREIGN_KEY_CHECKS=0; ALTER TABLE \`$t\` CONVERT TO CHARACTER SET $db_charset; SET FOREIGN_KEY_CHECKS=1;" >/dev/null 2>"$err_file"; then
                    :
                else
                    if [ "$table_failed" -eq 0 ]; then
                        echo "FAILED"
                    fi
                    table_failed=1
                    head -20 "$err_file"
                fi
            fi
            rm -f "$err_file"
        done <<EOF
$tables
EOF
    fi

    if [ "$table_failed" -ne 0 ]; then
        FAILED=$((FAILED + 1))
        return 1
    fi

    echo "ok"
}

run_sql "set database to server default" "
ALTER DATABASE \`$DB_NAME\` CHARACTER SET = utf8mb4;"

convert_tables_to_charset "convert existing tables to database default" 0

run_sql "create accounts table" "
CREATE TABLE IF NOT EXISTS accounts (
    account_name VARCHAR(50) NOT NULL,
    email VARCHAR(255) DEFAULT NULL,
    password VARCHAR(128) NOT NULL,
    confirmation_code VARCHAR(64) DEFAULT NULL,
    confirmed TINYINT(1) DEFAULT 0,
    confirmation_sent TINYINT(1) DEFAULT 0,
    blocked TINYINT(1) DEFAULT 0,
    last_login TIMESTAMP NULL DEFAULT NULL,
    last_good_char TIMESTAMP NULL DEFAULT NULL,
    last_evil_char TIMESTAMP NULL DEFAULT NULL,
    flags1 BIGINT UNSIGNED DEFAULT 0,
    flags2 BIGINT UNSIGNED DEFAULT 0,
    flags3 BIGINT UNSIGNED DEFAULT 0,
    flags4 BIGINT UNSIGNED DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (account_name),
    INDEX idx_email (email)
);

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'accounts' AND column_name = 'email');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE accounts ADD COLUMN email VARCHAR(255) DEFAULT NULL AFTER account_name',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'accounts' AND column_name = 'password');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE accounts ADD COLUMN password VARCHAR(128) NOT NULL DEFAULT \\'\\'',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'accounts' AND column_name = 'confirmation_code');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE accounts ADD COLUMN confirmation_code VARCHAR(64) DEFAULT NULL',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'accounts' AND column_name = 'confirmed');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE accounts ADD COLUMN confirmed TINYINT(1) DEFAULT 0',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'accounts' AND column_name = 'confirmation_sent');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE accounts ADD COLUMN confirmation_sent TINYINT(1) DEFAULT 0',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'accounts' AND column_name = 'blocked');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE accounts ADD COLUMN blocked TINYINT(1) DEFAULT 0',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'accounts' AND column_name = 'last_login');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE accounts ADD COLUMN last_login TIMESTAMP NULL DEFAULT NULL',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_type = (SELECT DATA_TYPE FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'accounts' AND column_name = 'last_login');
SET @sql = IF(@col_type NOT IN ('bigint', 'int'),
    'ALTER TABLE accounts MODIFY COLUMN last_login TIMESTAMP NULL DEFAULT NULL',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'accounts' AND column_name = 'last_good_char');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE accounts ADD COLUMN last_good_char TIMESTAMP NULL DEFAULT NULL',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_type = (SELECT DATA_TYPE FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'accounts' AND column_name = 'last_good_char');
SET @sql = IF(@col_type IS NOT NULL AND @col_type NOT IN ('bigint', 'int'),
    'ALTER TABLE accounts MODIFY COLUMN last_good_char TIMESTAMP NULL DEFAULT NULL',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'accounts' AND column_name = 'last_evil_char');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE accounts ADD COLUMN last_evil_char TIMESTAMP NULL DEFAULT NULL',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_type = (SELECT DATA_TYPE FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'accounts' AND column_name = 'last_evil_char');
SET @sql = IF(@col_type IS NOT NULL AND @col_type NOT IN ('bigint', 'int'),
    'ALTER TABLE accounts MODIFY COLUMN last_evil_char TIMESTAMP NULL DEFAULT NULL',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'accounts' AND column_name = 'flags1');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE accounts ADD COLUMN flags1 BIGINT UNSIGNED DEFAULT 0',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'accounts' AND column_name = 'flags2');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE accounts ADD COLUMN flags2 BIGINT UNSIGNED DEFAULT 0',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'accounts' AND column_name = 'flags3');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE accounts ADD COLUMN flags3 BIGINT UNSIGNED DEFAULT 0',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'accounts' AND column_name = 'flags4');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE accounts ADD COLUMN flags4 BIGINT UNSIGNED DEFAULT 0',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;"

run_sql "create account_characters table" "
CREATE TABLE IF NOT EXISTS account_characters (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    account_name VARCHAR(50) NOT NULL,
    char_name VARCHAR(64) NOT NULL,
    pid INT UNSIGNED DEFAULT NULL,
    login_count BIGINT UNSIGNED DEFAULT 0,
    last_login TIMESTAMP NULL DEFAULT NULL,
    blocked TINYINT DEFAULT 0,
    racewar TINYINT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (account_name) REFERENCES accounts(account_name) ON DELETE CASCADE,
    INDEX idx_account_name (account_name),
    INDEX idx_char_name (char_name)
);"

run_sql "create player_data table" "
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
    birth_time TIMESTAMP NULL DEFAULT NULL,
    played_time INT DEFAULT 0,
    last_save TIMESTAMP NULL DEFAULT NULL,
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
    time_left_guild TIMESTAMP NULL DEFAULT NULL,
    nb_left_guild TINYINT DEFAULT 0,
    time_unspecced TIMESTAMP NULL DEFAULT NULL,
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
);"

run_sql "create account_ips table" "
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
);"

run_sql "create towns table" "
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
);"

run_sql "create kingdom_land table" "
CREATE TABLE IF NOT EXISTS kingdom_land (
    id INT AUTO_INCREMENT PRIMARY KEY,
    kingdom_id INT NOT NULL,
    start_vnum INT DEFAULT 0,
    end_vnum INT DEFAULT 0,
    type CHAR(1) DEFAULT 'r',
    INDEX idx_kingdom_id (kingdom_id)
);"

run_sql "create player_recipes table" "
CREATE TABLE IF NOT EXISTS player_recipes (
    id INT AUTO_INCREMENT PRIMARY KEY,
    pid INT NOT NULL,
    recipe_vnum INT NOT NULL,
    learned_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_recipe (pid, recipe_vnum)
);"

run_sql "create player_shapechanges table" "
CREATE TABLE IF NOT EXISTS player_shapechanges (
    id INT AUTO_INCREMENT PRIMARY KEY,
    pid INT NOT NULL,
    mob_vnum INT NOT NULL,
    times_researched INT DEFAULT 0,
    last_researched TIMESTAMP NULL DEFAULT NULL,
    last_shapechanged TIMESTAMP NULL DEFAULT NULL,
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_mob (pid, mob_vnum)
);"

run_sql "create corpses table" "
CREATE TABLE IF NOT EXISTS corpses (
    id INT AUTO_INCREMENT PRIMARY KEY,
    player_name VARCHAR(50) NOT NULL,
    save_id BIGINT NOT NULL,
    room_vnum INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_player_name (player_name),
    UNIQUE KEY uk_player_saveid (player_name, save_id)
);"

run_sql "create shopkeepers table" "
CREATE TABLE IF NOT EXISTS shopkeepers (
    id INT AUTO_INCREMENT PRIMARY KEY,
    shop_id INT NOT NULL UNIQUE,
    mob_vnum INT DEFAULT 0,
    room_vnum INT DEFAULT 0,
    save_time TIMESTAMP NULL DEFAULT NULL,
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
);"

run_sql "create races and classes tables" "
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
);"

run_sql "create players_view" "
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
LEFT JOIN classes c ON pd.m_class = c.id;"

run_sql "create player array tables" "
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
    intro_time TIMESTAMP NULL DEFAULT NULL,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_intro (pid, intro_index),
    CONSTRAINT fk_player_intros FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS player_timers (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT UNSIGNED NOT NULL,
    timer_id TINYINT UNSIGNED NOT NULL,
    timer_value TIMESTAMP NULL DEFAULT NULL,
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
);"

run_sql "create player affects and items tables" "
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
    witness_time TIMESTAMP NULL DEFAULT NULL,
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
);"

run_sql "create corpse_items tables" "
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
);
CREATE TABLE IF NOT EXISTS corpse_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES corpse_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
);"

run_sql "create shopkeeper_items tables" "
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
);
CREATE TABLE IF NOT EXISTS shopkeeper_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES shopkeeper_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
);"

run_sql "create saved_items tables" "
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
);
CREATE TABLE IF NOT EXISTS saved_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES saved_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
);"

run_sql "create siege_items tables" "
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
);"

run_sql "create lockers tables" "
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
    unique_id INT UNSIGNED DEFAULT NULL,
    FOREIGN KEY (locker_id) REFERENCES lockers(id) ON DELETE CASCADE,
    FOREIGN KEY (container_id) REFERENCES locker_items(id) ON DELETE CASCADE,
    INDEX idx_locker_id (locker_id),
    INDEX idx_vnum (vnum)
);
CREATE TABLE IF NOT EXISTS locker_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES locker_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
);"

run_sql "add account_characters columns" "
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'login_count');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN login_count BIGINT UNSIGNED DEFAULT 0',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'last_login');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN last_login TIMESTAMP NULL DEFAULT NULL',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'blocked');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN blocked TINYINT DEFAULT 0',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'racewar');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN racewar TINYINT DEFAULT 0',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND index_name = 'idx_account_racewar');
SET @sql = IF(@idx_exists = 0,
    'CREATE INDEX idx_account_racewar ON account_characters(account_name, racewar)',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;"

run_sql "create ships tables" "
CREATE TABLE IF NOT EXISTS ships (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
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
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'ships' AND column_name = 'race');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE ships ADD COLUMN race TINYINT DEFAULT 0',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'ships' AND column_name = 'money');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE ships ADD COLUMN money INT DEFAULT 0',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'ships' AND column_name = 'flags');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE ships ADD COLUMN flags BIGINT UNSIGNED DEFAULT 0',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

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

CREATE TABLE IF NOT EXISTS ship_armor (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    ship_id INT UNSIGNED NOT NULL,
    side TINYINT NOT NULL,
    armor INT DEFAULT 0,
    internal INT DEFAULT 0,
    CONSTRAINT fk_ship_armor_ship FOREIGN KEY (ship_id) REFERENCES ships(id) ON DELETE CASCADE,
    UNIQUE KEY uk_ship_armor (ship_id, side)
);

CREATE TABLE IF NOT EXISTS ship_crew (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    ship_id INT UNSIGNED NOT NULL,
    crew_index INT DEFAULT 0,
    sail_skill INT DEFAULT 0,
    guns_skill INT DEFAULT 0,
    rpar_skill INT DEFAULT 0,
    sail_chief INT DEFAULT 0,
    guns_chief INT DEFAULT 0,
    rpar_chief INT DEFAULT 0,
    CONSTRAINT fk_ship_crew_ship FOREIGN KEY (ship_id) REFERENCES ships(id) ON DELETE CASCADE,
    UNIQUE KEY uk_ship_crew (ship_id)
);"

run_sql "create guilds tables" "
CREATE TABLE IF NOT EXISTS guilds (
    id INT UNSIGNED PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    racewar INT UNSIGNED NOT NULL DEFAULT 0,
    bits INT UNSIGNED NOT NULL DEFAULT 0,
    prestige BIGINT UNSIGNED NOT NULL DEFAULT 0,
    construction BIGINT UNSIGNED NOT NULL DEFAULT 0,
    platinum INT UNSIGNED NOT NULL DEFAULT 0,
    gold INT UNSIGNED NOT NULL DEFAULT 0,
    silver INT UNSIGNED NOT NULL DEFAULT 0,
    copper INT UNSIGNED NOT NULL DEFAULT 0,
    frags BIGINT NOT NULL DEFAULT 0,
    top_frags BIGINT NOT NULL DEFAULT 0,
    topfragger VARCHAR(50) NOT NULL DEFAULT '',
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

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'guilds' AND column_name = 'guild_id');
SET @sql = IF(@col_exists > 0,
    'ALTER TABLE guilds DROP COLUMN guild_id',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'guilds' AND column_name = 'total_frags');
SET @sql = IF(@col_exists > 0,
    'ALTER TABLE guilds CHANGE COLUMN total_frags frags BIGINT NOT NULL DEFAULT 0',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'guilds' AND column_name = 'top_fragger');
SET @sql = IF(@col_exists > 0,
    'ALTER TABLE guilds CHANGE COLUMN top_fragger topfragger VARCHAR(50) NOT NULL DEFAULT \\'\\'',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'guilds' AND column_name = 'frags');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE guilds ADD COLUMN frags BIGINT NOT NULL DEFAULT 0',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'guilds' AND column_name = 'topfragger');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE guilds ADD COLUMN topfragger VARCHAR(50) NOT NULL DEFAULT \\'\\'',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;"

run_sql "add guild_members columns" "
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'guild_members' AND column_name = 'online_status');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE guild_members ADD COLUMN online_status TINYINT NOT NULL DEFAULT 0',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'guild_members' AND index_name = 'idx_guild_members_name');
SET @sql = IF(@idx_exists = 0,
    'CREATE INDEX idx_guild_members_name ON guild_members(player_name)',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;"

run_sql "add player_data columns" "
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_data' AND column_name = 'act3');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_data ADD COLUMN act3 BIGINT UNSIGNED DEFAULT 0 AFTER act2',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_data' AND column_name = 'last_room');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_data ADD COLUMN last_room INT DEFAULT 0 AFTER orig_birthplace',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;"

run_sql "add unique char name constraints" "
DELETE ac1 FROM account_characters ac1
INNER JOIN account_characters ac2
WHERE ac1.char_name = ac2.char_name
  AND ac1.pid > ac2.pid;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE()
    AND table_name = 'account_characters'
    AND index_name = 'idx_char_name_unique');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE account_characters ADD UNIQUE INDEX idx_char_name_unique (char_name)',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE()
    AND table_name = 'player_data'
    AND index_name = 'idx_player_name_unique');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_data ADD UNIQUE INDEX idx_player_name_unique (name)',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;"

run_sql "add killed_by column" "
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_data' AND column_name = 'killed_by');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_data ADD COLUMN killed_by VARCHAR(64) DEFAULT NULL AFTER numb_deaths',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;"

run_sql "add unique keys for upsert" "
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_languages' AND index_name = 'uk_pid_tongue');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_languages ADD UNIQUE KEY uk_pid_tongue (pid, tongue_id)',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_intros' AND index_name = 'uk_pid_intro');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_intros ADD UNIQUE KEY uk_pid_intro (pid, intro_index)',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_timers' AND index_name = 'uk_pid_timer');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_timers ADD UNIQUE KEY uk_pid_timer (pid, timer_id)',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_undead_slots' AND index_name = 'uk_pid_circle');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_undead_slots ADD UNIQUE KEY uk_pid_circle (pid, circle)',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_forged_items' AND index_name = 'uk_pid_forge');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_forged_items ADD UNIQUE KEY uk_pid_forge (pid, forge_index)',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_granted_cmds' AND index_name = 'uk_pid_cmd');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_granted_cmds ADD UNIQUE KEY uk_pid_cmd (pid, cmd_num)',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'player_skills' AND index_name = 'uk_pid_skill');
SET @sql = IF(@idx_exists = 0,
    'ALTER TABLE player_skills ADD UNIQUE KEY uk_pid_skill (pid, skill_id)',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;"

run_sql "create player_pets tables" "
CREATE TABLE IF NOT EXISTS player_pets (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  owner_pid INT UNSIGNED NOT NULL,
  mob_vnum INT NOT NULL,
  pet_order TINYINT DEFAULT 0,
  hit INT DEFAULT 0,
  max_hit INT DEFAULT 0,
  mana INT DEFAULT 0,
  max_mana INT DEFAULT 0,
  vitality INT DEFAULT 0,
  max_vitality INT DEFAULT 0,
  charm_duration INT DEFAULT -1,
  room_vnum INT DEFAULT 0,
  saved_at TIMESTAMP NULL DEFAULT NULL,
  PRIMARY KEY (id),
  KEY idx_owner_pid (owner_pid),
  CONSTRAINT fk_player_pets_owner FOREIGN KEY (owner_pid)
    REFERENCES player_data (pid) ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS player_pet_items (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  pet_id INT UNSIGNED NOT NULL,
  vnum INT NOT NULL,
  equip_slot TINYINT DEFAULT 0,
  container_id INT UNSIGNED DEFAULT NULL,
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
  PRIMARY KEY (id),
  KEY idx_pet_id (pet_id),
  KEY idx_container_id (container_id),
  CONSTRAINT fk_pet_items_pet FOREIGN KEY (pet_id)
    REFERENCES player_pets (id) ON DELETE CASCADE,
  CONSTRAINT fk_pet_items_container FOREIGN KEY (container_id)
    REFERENCES player_pet_items (id) ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS player_pet_item_affects (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  location TINYINT UNSIGNED DEFAULT 0,
  modifier INT DEFAULT 0,
  PRIMARY KEY (id),
  KEY idx_item_id (item_id),
  CONSTRAINT fk_pet_item_affects FOREIGN KEY (item_id)
    REFERENCES player_pet_items (id) ON DELETE CASCADE
);"

run_sql "update zone payouts" "
DROP PROCEDURE IF EXISTS update_zone_payouts;

DELIMITER //
CREATE PROCEDURE update_zone_payouts()
BEGIN
    DECLARE tbl_exists INT DEFAULT 0;
    SELECT COUNT(*) INTO tbl_exists FROM information_schema.tables
        WHERE table_schema = DATABASE() AND table_name = 'zones';

    IF tbl_exists > 0 THEN
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
DROP PROCEDURE IF EXISTS update_zone_payouts;"

run_sql "create item extra_descr tables" "
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
);"

run_sql "add obj_uid columns" "
DELIMITER //

CREATE PROCEDURE add_obj_uid_columns()
BEGIN
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

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_items'
                   AND column_name = 'wear_flags') THEN
        ALTER TABLE player_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER extra_flags;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_items'
                   AND column_name = 'item_type') THEN
        ALTER TABLE player_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER wear_flags;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_items'
                   AND column_name = 'bitvector1') THEN
        ALTER TABLE player_items ADD COLUMN bitvector1 BIGINT UNSIGNED DEFAULT NULL AFTER action_descr;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_items'
                   AND column_name = 'bitvector2') THEN
        ALTER TABLE player_items ADD COLUMN bitvector2 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector1;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_items'
                   AND column_name = 'bitvector3') THEN
        ALTER TABLE player_items ADD COLUMN bitvector3 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector2;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_items'
                   AND column_name = 'bitvector4') THEN
        ALTER TABLE player_items ADD COLUMN bitvector4 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector3;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_items'
                   AND column_name = 'bitvector5') THEN
        ALTER TABLE player_items ADD COLUMN bitvector5 BIGINT UNSIGNED DEFAULT NULL AFTER bitvector4;
    END IF;

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

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'corpse_items'
                   AND column_name = 'wear_flags') THEN
        ALTER TABLE corpse_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER extra_flags;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'corpse_items'
                   AND column_name = 'item_type') THEN
        ALTER TABLE corpse_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER wear_flags;
    END IF;

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

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'locker_items'
                   AND column_name = 'wear_flags') THEN
        ALTER TABLE locker_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER extra_flags;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'locker_items'
                   AND column_name = 'item_type') THEN
        ALTER TABLE locker_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER wear_flags;
    END IF;

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
DROP PROCEDURE IF EXISTS add_obj_uid_columns;"

run_sql "create account_lockers table" "
CREATE TABLE IF NOT EXISTS account_lockers (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    account_name VARCHAR(50) NOT NULL UNIQUE,
    racewar TINYINT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (account_name) REFERENCES accounts(account_name) ON DELETE CASCADE,
    INDEX idx_account_name (account_name)
);"

run_sql "create locker_chests table" "
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
);"

run_sql "create account_locker_items table" "
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
);"

run_sql "create account_locker_access table" "
CREATE TABLE IF NOT EXISTS account_locker_access (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    visitor_account VARCHAR(50) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (locker_id) REFERENCES account_lockers(id) ON DELETE CASCADE,
    UNIQUE KEY uk_locker_visitor (locker_id, visitor_account),
    INDEX idx_visitor (visitor_account)
);"

run_sql "create locker_activity_log table" "
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
);"

run_sql "create locker_kickouts table" "
CREATE TABLE IF NOT EXISTS locker_kickouts (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    account_name VARCHAR(50) NOT NULL,
    fail_count TINYINT UNSIGNED DEFAULT 0,
    kicked_until TIMESTAMP NULL DEFAULT NULL,
    last_fail TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (locker_id) REFERENCES account_lockers(id) ON DELETE CASCADE,
    UNIQUE KEY uk_locker_account (locker_id, account_name)
);"

run_sql "create locker_session_state table" "
CREATE TABLE IF NOT EXISTS locker_session_state (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    account_name VARCHAR(50) NOT NULL,
    chest_id INT UNSIGNED NOT NULL,
    opened_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (locker_id) REFERENCES account_lockers(id) ON DELETE CASCADE,
    FOREIGN KEY (chest_id) REFERENCES locker_chests(id) ON DELETE CASCADE,
    UNIQUE KEY uk_session (locker_id, account_name, chest_id)
);"

run_sql "sync player_data account_name" "
UPDATE player_data pd
JOIN account_characters ac ON pd.pid = ac.pid
SET pd.account_name = ac.account_name
WHERE pd.account_name IS NULL OR pd.account_name = '';"

run_sql "create account lockers from char lockers" "
INSERT IGNORE INTO lockers (locker_name, racewar, race)
SELECT DISTINCT CONCAT('account.', LOWER(ac.account_name), '.', ac.racewar, '.locker'), ac.racewar, 0
FROM account_characters ac
JOIN lockers l ON LOWER(SUBSTRING_INDEX(l.locker_name, '.locker', 1)) = LOWER(ac.char_name)
WHERE ac.account_name IS NOT NULL AND ac.account_name != ''
  AND l.locker_name LIKE '%.locker'
  AND l.locker_name NOT LIKE 'guild.%'
  AND l.locker_name NOT LIKE 'account.%';"

run_sql "copy locker items to account lockers" "
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
  ));"

run_sql "copy locker item affects" "
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
  );"

run_sql "create account_banks table" "
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
);"

run_sql "migrate player banks to account banks" "
INSERT IGNORE INTO account_banks (account_name, racewar, bank_copper, bank_silver, bank_gold, bank_platinum)
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
GROUP BY ac.account_name, ac.racewar;"

run_sql "create private_chests table" "
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
);"

run_sql "add locker_items chest_id column" "
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'chest_id');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE locker_items ADD COLUMN chest_id INT UNSIGNED DEFAULT NULL AFTER locker_id',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;"

run_sql "create private_chest_log table" "
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
);"

run_sql "create default public chests" "
INSERT IGNORE INTO private_chests (locker_id, chest_name, is_public)
SELECT id, 'public', 1
FROM lockers
WHERE locker_name LIKE 'account.%';"

run_sql "add total_donated column" "
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'accounts' AND column_name = 'total_donated');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE accounts ADD COLUMN total_donated DECIMAL(10,2) DEFAULT 0.00',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;"

run_sql "create polls tables" "
CREATE TABLE IF NOT EXISTS polls (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    question VARCHAR(512) NOT NULL,
    created_by VARCHAR(32) NOT NULL,
    created_at TIMESTAMP NULL DEFAULT NULL,
    expires_at TIMESTAMP NULL DEFAULT NULL,
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
    voted_at TIMESTAMP NULL DEFAULT NULL,
    char_name VARCHAR(32) NOT NULL,
    UNIQUE KEY uk_poll_account_option (poll_id, account_name, option_id),
    INDEX idx_poll_id (poll_id),
    INDEX idx_account_name (account_name)
);"

echo ""
echo "done. $FAILED failures."
exit $FAILED
