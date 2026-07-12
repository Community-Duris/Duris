#!/bin/bash

# Don't use set -e: the charset conversion functions can fail on legacy
# tables (MyISAM, latin1, invalid datetime defaults) without affecting
# the rest of the migration.  Individual run_sql calls track failures
# and report them at the end.
set +e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/.env"

MYSQL_CMD="mysql -h$DB_HOST -P${DB_PORT:-3306} -u$DB_USER -p$DB_PASSWD $DB_NAME"

STEP=0
TOTAL=109
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
                if $MYSQL_CMD -e "SET sql_mode=''; SET FOREIGN_KEY_CHECKS=0; ALTER TABLE \`$t\` CONVERT TO CHARACTER SET $db_charset COLLATE $db_collation; SET FOREIGN_KEY_CHECKS=1;" >/dev/null 2>"$err_file"; then
                    :
                else
                    if [ "$table_failed" -eq 0 ]; then
                        echo "FAILED"
                    fi
                    table_failed=1
                    head -20 "$err_file"
                fi
            else
                if $MYSQL_CMD -e "SET sql_mode=''; SET FOREIGN_KEY_CHECKS=0; ALTER TABLE \`$t\` CONVERT TO CHARACTER SET $db_charset; SET FOREIGN_KEY_CHECKS=1;" >/dev/null 2>"$err_file"; then
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
ALTER DATABASE \`$DB_NAME\` CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci;"

run_sql "fix invalid datetime defaults before engine conversion" "
SET sql_mode='';
UPDATE log_entries SET date='1970-01-01 00:00:01' WHERE date < '1970-01-01 00:00:01' OR date = '0000-00-00 00:00:00';
UPDATE offline_messages SET date='1970-01-01 00:00:01' WHERE date < '1970-01-01 00:00:01' OR date = '0000-00-00 00:00:00';
UPDATE ping SET TIMESTAMP='1970-01-01 00:00:01' WHERE TIMESTAMP < '1970-01-01 00:00:01' OR TIMESTAMP = '0000-00-00 00:00:00';
UPDATE pkill_event SET stamp='1970-01-01 00:00:01' WHERE stamp < '1970-01-01 00:00:01' OR stamp = '0000-00-00 00:00:00';
UPDATE progress SET stamp='1970-01-01 00:00:01' WHERE stamp < '1970-01-01 00:00:01' OR stamp = '0000-00-00 00:00:00';"

run_sql "convert legacy MyISAM tables to InnoDB" "
SET sql_mode='';
ALTER TABLE artifact_bind ENGINE=InnoDB;
ALTER TABLE artifacts ENGINE=InnoDB;
ALTER TABLE artifacts_mortal ENGINE=InnoDB;
ALTER TABLE boons ENGINE=InnoDB;
ALTER TABLE boons_progress ENGINE=InnoDB;
ALTER TABLE boons_shop ENGINE=InnoDB;
ALTER TABLE ctf_data ENGINE=InnoDB;
ALTER TABLE epic_bonus ENGINE=InnoDB;
ALTER TABLE epic_gain ENGINE=InnoDB;
ALTER TABLE guild_transactions ENGINE=InnoDB;
ALTER TABLE guildhall_rooms ENGINE=InnoDB;
ALTER TABLE guildhalls ENGINE=InnoDB;
ALTER TABLE ip_info ENGINE=InnoDB;
ALTER TABLE locker_access ENGINE=InnoDB;
ALTER TABLE mud_info ENGINE=InnoDB;
ALTER TABLE multiplay_whitelist ENGINE=InnoDB;
ALTER TABLE nexus_stones ENGINE=InnoDB;
ALTER TABLE offline_messages ENGINE=InnoDB;
ALTER TABLE outposts ENGINE=InnoDB;
ALTER TABLE ping ENGINE=InnoDB;
ALTER TABLE pkill_event ENGINE=InnoDB;
ALTER TABLE pkill_info ENGINE=InnoDB;
ALTER TABLE players_core ENGINE=InnoDB;
ALTER TABLE poll_options ENGINE=InnoDB;
ALTER TABLE poll_votes ENGINE=InnoDB;
ALTER TABLE polls ENGINE=InnoDB;
ALTER TABLE progress ENGINE=InnoDB;
ALTER TABLE racewar_stat_mods ENGINE=InnoDB;
ALTER TABLE ship_cargo_market_mods ENGINE=InnoDB;
ALTER TABLE ship_cargo_prices ENGINE=InnoDB;
ALTER TABLE shop_trophy ENGINE=InnoDB;"

convert_tables_to_charset "convert existing tables to database default" 1

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
    active TINYINT(1) NOT NULL DEFAULT 1,
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

run_sql "create frag leaderboard table" "
CREATE TABLE IF NOT EXISTS frag_leaderboard (
  id int(11) NOT NULL auto_increment,
  pid bigint(20) NOT NULL,
  account_name varchar(255) NOT NULL,
  char_name varchar(255) NOT NULL,
  total_frags int(11) NOT NULL DEFAULT 0,
  racewar int(11) NOT NULL,
  race varchar(50) DEFAULT NULL,
  class varchar(50) DEFAULT NULL,
  level int(11) DEFAULT NULL,
  deleted_at datetime NULL DEFAULT NULL,
  last_updated datetime DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  UNIQUE KEY pid (pid),
  KEY char_name (char_name),
  KEY account_name (account_name),
  KEY total_frags_active (deleted_at, total_frags),
  KEY racewar_leaderboard (deleted_at, racewar, total_frags),
  KEY race_leaderboard (deleted_at, race, total_frags),
  KEY class_leaderboard (deleted_at, class, total_frags),
  KEY level_range (deleted_at, level, total_frags)
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
    bitvector1 BIGINT UNSIGNED DEFAULT 0,
    bitvector2 BIGINT UNSIGNED DEFAULT 0,
    bitvector3 BIGINT UNSIGNED DEFAULT 0,
    bitvector4 BIGINT UNSIGNED DEFAULT 0,
    bitvector5 BIGINT UNSIGNED DEFAULT 0,
    custom_msg_char TEXT DEFAULT NULL,
    custom_msg_room TEXT DEFAULT NULL,
    PRIMARY KEY (id),
    INDEX idx_pid (pid),
    CONSTRAINT fk_player_affects FOREIGN KEY (pid) REFERENCES player_data(pid) ON DELETE CASCADE
);
UPDATE player_affects SET bitvector1 = 0 WHERE bitvector1 < 0;
UPDATE player_affects SET bitvector2 = 0 WHERE bitvector2 < 0;
UPDATE player_affects SET bitvector3 = 0 WHERE bitvector3 < 0;
UPDATE player_affects SET bitvector4 = 0 WHERE bitvector4 < 0;
UPDATE player_affects SET bitvector5 = 0 WHERE bitvector5 < 0;
ALTER TABLE player_affects
    MODIFY bitvector1 BIGINT UNSIGNED DEFAULT 0,
    MODIFY bitvector2 BIGINT UNSIGNED DEFAULT 0,
    MODIFY bitvector3 BIGINT UNSIGNED DEFAULT 0,
    MODIFY bitvector4 BIGINT UNSIGNED DEFAULT 0,
    MODIFY bitvector5 BIGINT UNSIGNED DEFAULT 0;
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
    wear_flags INT DEFAULT NULL,
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

run_sql "add item_type to corpse_items" "
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'item_type');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE corpse_items ADD COLUMN item_type TINYINT DEFAULT 0 AFTER vnum',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;"

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
    wear_flags INT DEFAULT NULL,
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
    wear_flags INT DEFAULT NULL,
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
    wear_flags INT DEFAULT NULL,
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
    wear_flags INT DEFAULT NULL,
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
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND column_name = 'deleted_at');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE account_characters ADD COLUMN deleted_at DATETIME DEFAULT NULL',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND index_name = 'account_active');
SET @sql = IF(@idx_exists = 0,
    'CREATE INDEX account_active ON account_characters(account_name, deleted_at)',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;"

run_sql "sync account_characters pid" "
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'account_characters' AND index_name = 'pid');
SET @sql = IF(@idx_exists > 0,
    'ALTER TABLE account_characters DROP INDEX pid',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

UPDATE account_characters ac
JOIN player_data pd ON LOWER(ac.char_name) = LOWER(pd.name)
SET ac.pid = pd.pid
WHERE ac.pid != pd.pid OR ac.pid IS NULL;"

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

run_sql "sync guild_members player_pid" "
UPDATE guild_members gm
JOIN player_data pd ON LOWER(gm.player_name) = LOWER(pd.name)
SET gm.player_pid = pd.pid
WHERE gm.player_pid = 0 OR gm.player_pid IS NULL;"

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
DEALLOCATE PREPARE stmt;

SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'player_data' AND column_name = 'active');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE player_data ADD COLUMN active TINYINT(1) NOT NULL DEFAULT 1 AFTER last_ip',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

UPDATE player_data SET active = 1 WHERE active = 0 OR active IS NULL;"

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
  wear_flags INT DEFAULT NULL,
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

    -- fix random eq with null item_type based on wear_flags
    -- ITEM_WEAPON (5): WIELD = 8192
    UPDATE player_items SET item_type = 5 WHERE item_type IS NULL AND (wear_flags & 8192) != 0;
    UPDATE locker_items SET item_type = 5 WHERE item_type IS NULL AND (wear_flags & 8192) != 0;
    UPDATE corpse_items SET item_type = 5 WHERE item_type IS NULL AND (wear_flags & 8192) != 0;
    -- ITEM_SHIELD (37): WEAR_SHIELD = 512
    UPDATE player_items SET item_type = 37 WHERE item_type IS NULL AND (wear_flags & 512) != 0;
    UPDATE locker_items SET item_type = 37 WHERE item_type IS NULL AND (wear_flags & 512) != 0;
    UPDATE corpse_items SET item_type = 37 WHERE item_type IS NULL AND (wear_flags & 512) != 0;
    -- ITEM_QUIVER (30): WEAR_QUIVER = 1048576
    UPDATE player_items SET item_type = 30 WHERE item_type IS NULL AND (wear_flags & 1048576) != 0;
    UPDATE locker_items SET item_type = 30 WHERE item_type IS NULL AND (wear_flags & 1048576) != 0;
    UPDATE corpse_items SET item_type = 30 WHERE item_type IS NULL AND (wear_flags & 1048576) != 0;
    -- ITEM_ARMOR (9): body/head/legs/feet/hands/arms/about/waist/horse_body/spider_body = 553651704
    UPDATE player_items SET item_type = 9 WHERE item_type IS NULL AND (wear_flags & 553651704) != 0;
    UPDATE locker_items SET item_type = 9 WHERE item_type IS NULL AND (wear_flags & 553651704) != 0;
    UPDATE corpse_items SET item_type = 9 WHERE item_type IS NULL AND (wear_flags & 553651704) != 0;

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

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'player_pet_items'
                   AND column_name = 'wear_flags') THEN
        ALTER TABLE player_pet_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER extra_flags;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'shopkeeper_items'
                   AND column_name = 'wear_flags') THEN
        ALTER TABLE shopkeeper_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER extra_flags;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'shopkeeper_items'
                   AND column_name = 'item_type') THEN
        ALTER TABLE shopkeeper_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER wear_flags;
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.columns
               WHERE table_schema = DATABASE()
               AND table_name = 'shopkeeper_items'
               AND column_name = 'unique_id') THEN
        ALTER TABLE shopkeeper_items CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL;
    ELSEIF NOT EXISTS (SELECT 1 FROM information_schema.columns
                       WHERE table_schema = DATABASE()
                       AND table_name = 'shopkeeper_items'
                       AND column_name = 'obj_uid') THEN
        ALTER TABLE shopkeeper_items ADD COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'saved_items'
                   AND column_name = 'wear_flags') THEN
        ALTER TABLE saved_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER extra_flags;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'saved_items'
                   AND column_name = 'item_type') THEN
        ALTER TABLE saved_items ADD COLUMN item_type TINYINT DEFAULT NULL AFTER wear_flags;
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.columns
               WHERE table_schema = DATABASE()
               AND table_name = 'saved_items'
               AND column_name = 'unique_id') THEN
        ALTER TABLE saved_items CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL;
    ELSEIF NOT EXISTS (SELECT 1 FROM information_schema.columns
                       WHERE table_schema = DATABASE()
                       AND table_name = 'saved_items'
                       AND column_name = 'obj_uid') THEN
        ALTER TABLE saved_items ADD COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL;
    END IF;

    IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                   WHERE table_schema = DATABASE()
                   AND table_name = 'siege_items'
                   AND column_name = 'wear_flags') THEN
        ALTER TABLE siege_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER extra_flags;
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.columns
               WHERE table_schema = DATABASE()
               AND table_name = 'siege_items'
               AND column_name = 'unique_id') THEN
        ALTER TABLE siege_items CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL;
    ELSEIF NOT EXISTS (SELECT 1 FROM information_schema.columns
                       WHERE table_schema = DATABASE()
                       AND table_name = 'siege_items'
                       AND column_name = 'obj_uid') THEN
        ALTER TABLE siege_items ADD COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL;
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.tables
               WHERE table_schema = DATABASE()
               AND table_name = 'account_locker_items') THEN
        IF NOT EXISTS (SELECT 1 FROM information_schema.columns
                       WHERE table_schema = DATABASE()
                       AND table_name = 'account_locker_items'
                       AND column_name = 'wear_flags') THEN
            ALTER TABLE account_locker_items ADD COLUMN wear_flags INT DEFAULT NULL AFTER extra_flags;
        END IF;
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
    wear_flags INT DEFAULT NULL,
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
    action_type INT NOT NULL DEFAULT 1,
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
    action_type INT NOT NULL DEFAULT 1,
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

run_sql "create temp mapping table for locker items" "
DROP TABLE IF EXISTS _locker_item_map;
CREATE TABLE _locker_item_map (
    old_id INT UNSIGNED NOT NULL,
    new_id INT UNSIGNED NOT NULL,
    old_container_id INT UNSIGNED DEFAULT NULL,
    PRIMARY KEY (old_id),
    INDEX idx_new_id (new_id),
    INDEX idx_old_container (old_container_id)
);"

run_sql "copy locker items to account lockers" "
INSERT INTO locker_items (locker_id, chest_id, vnum, container_id, quantity, weight, cost, timer,
    extra_flags, wear_flags, value0, value1, value2, value3, value4, value5, value6, value7,
    name, short_descr, description, action_descr, obj_uid, item_condition)
SELECT
    acct_locker.id,
    pc.id,
    src.vnum,
    NULL,
    src.quantity, src.weight, src.cost, src.timer,
    src.extra_flags, src.wear_flags, src.value0, src.value1, src.value2, src.value3,
    src.value4, src.value5, src.value6, src.value7,
    src.name, src.short_descr, src.description, src.action_descr, src.obj_uid, src.item_condition
FROM locker_items src
JOIN lockers char_locker ON src.locker_id = char_locker.id
JOIN account_characters ac ON LOWER(SUBSTRING_INDEX(char_locker.locker_name, '.locker', 1)) = LOWER(ac.char_name)
JOIN lockers acct_locker ON acct_locker.locker_name = CONCAT('account.', LOWER(ac.account_name), '.', ac.racewar, '.locker')
JOIN private_chests pc ON pc.locker_id = acct_locker.id AND pc.chest_name = 'public'
WHERE char_locker.locker_name LIKE '%.locker'
  AND char_locker.locker_name NOT LIKE 'guild.%'
  AND char_locker.locker_name NOT LIKE 'account.%'
  AND src.vnum != 173;"

run_sql "build locker item id mapping" "
INSERT INTO _locker_item_map (old_id, new_id, old_container_id)
SELECT src.id, new_item.id, src.container_id
FROM locker_items src
JOIN lockers char_locker ON src.locker_id = char_locker.id
JOIN account_characters ac ON LOWER(SUBSTRING_INDEX(char_locker.locker_name, '.locker', 1)) = LOWER(ac.char_name)
JOIN lockers acct_locker ON acct_locker.locker_name = CONCAT('account.', LOWER(ac.account_name), '.', ac.racewar, '.locker')
JOIN private_chests pc ON pc.locker_id = acct_locker.id AND pc.chest_name = 'public'
JOIN locker_items new_item ON new_item.locker_id = acct_locker.id
    AND new_item.chest_id = pc.id
    AND new_item.obj_uid = src.obj_uid
WHERE char_locker.locker_name LIKE '%.locker'
  AND char_locker.locker_name NOT LIKE 'guild.%'
  AND char_locker.locker_name NOT LIKE 'account.%'
  AND src.vnum != 173
  AND src.obj_uid IS NOT NULL
ON DUPLICATE KEY UPDATE new_id = new_id;"

run_sql "restore container hierarchy in account lockers" "
UPDATE locker_items new_item
JOIN _locker_item_map m ON m.new_id = new_item.id
JOIN _locker_item_map container_map ON container_map.old_id = m.old_container_id
SET new_item.container_id = container_map.new_id
WHERE m.old_container_id IS NOT NULL;"

run_sql "copy locker item affects" "
INSERT INTO locker_item_affects (item_id, location, modifier)
SELECT m.new_id, lia.location, lia.modifier
FROM locker_item_affects lia
JOIN _locker_item_map m ON m.old_id = lia.item_id
WHERE NOT EXISTS (
    SELECT 1 FROM locker_item_affects WHERE item_id = m.new_id AND location = lia.location
);"

run_sql "cleanup temp mapping table" "
DROP TABLE IF EXISTS _locker_item_map;"

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

# ============================================================================
# legacy tables from duris.sql - create if not exists, non-destructive
# ============================================================================

run_sql "create alliances table" "
CREATE TABLE IF NOT EXISTS alliances (
    id INT AUTO_INCREMENT PRIMARY KEY,
    created_at DATETIME DEFAULT NULL,
    forging_assoc_id INT NOT NULL,
    joining_assoc_id INT NOT NULL,
    tribute_owed INT NOT NULL DEFAULT 0
);"

run_sql "create artifact_bind table" "
CREATE TABLE IF NOT EXISTS artifact_bind (
    vnum INT NOT NULL PRIMARY KEY,
    owner_pid INT DEFAULT NULL,
    timer INT DEFAULT NULL
);"

run_sql "create associations table" "
CREATE TABLE IF NOT EXISTS associations (
    id INT NOT NULL PRIMARY KEY,
    name VARCHAR(255) NOT NULL DEFAULT '',
    prestige INT NOT NULL DEFAULT 0,
    active TINYINT(1) NOT NULL DEFAULT 1,
    wood INT NOT NULL DEFAULT 0,
    stone INT NOT NULL DEFAULT 0,
    construction_points INT NOT NULL DEFAULT 0,
    over_max INT NOT NULL DEFAULT 0
);"

run_sql "create auction tables" "
CREATE TABLE IF NOT EXISTS auction_bid_history (
    id INT AUTO_INCREMENT PRIMARY KEY,
    date INT NOT NULL DEFAULT 0,
    auction_id INT NOT NULL DEFAULT 0,
    bidder_pid INT NOT NULL DEFAULT 0,
    bidder_name VARCHAR(32) NOT NULL DEFAULT '',
    bid_amount INT NOT NULL DEFAULT 0,
    INDEX idx_auction_id (auction_id)
);
CREATE TABLE IF NOT EXISTS auction_item_pickups (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    pid INT UNSIGNED NOT NULL DEFAULT 0,
    obj_blob_str BLOB NOT NULL,
    retrieved TINYINT(1) NOT NULL DEFAULT 0,
    quantity INT NOT NULL DEFAULT 1,
    INDEX idx_pid (pid)
);
CREATE TABLE IF NOT EXISTS auction_money_pickups (
    pid INT UNSIGNED NOT NULL PRIMARY KEY,
    money INT UNSIGNED NOT NULL DEFAULT 0
);
CREATE TABLE IF NOT EXISTS auctions (
    id INT AUTO_INCREMENT PRIMARY KEY,
    seller_pid INT UNSIGNED NOT NULL DEFAULT 0,
    seller_name VARCHAR(32) NOT NULL DEFAULT '',
    start_time TIMESTAMP NULL DEFAULT NULL,
    end_time TIMESTAMP NULL DEFAULT NULL,
    status INT NOT NULL DEFAULT 1,
    winning_bidder_pid INT NOT NULL DEFAULT 0,
    winning_bidder_name VARCHAR(32) NOT NULL DEFAULT '',
    cur_price INT UNSIGNED NOT NULL DEFAULT 0,
    buy_price INT NOT NULL DEFAULT 0,
    obj_short VARCHAR(255) NOT NULL DEFAULT '',
    obj_vnum INT NOT NULL DEFAULT 0,
    obj_blob_str BLOB NOT NULL,
    id_keywords VARCHAR(1024) NOT NULL DEFAULT '',
    quantity INT NOT NULL DEFAULT 1,
    obj_info_text TEXT DEFAULT NULL,
    INDEX idx_seller_pid (seller_pid),
    INDEX idx_end_time (end_time),
    INDEX idx_status (status)
);"

run_sql "create boons tables" "
CREATE TABLE IF NOT EXISTS boons (
    id INT AUTO_INCREMENT PRIMARY KEY,
    time INT NOT NULL DEFAULT 0,
    duration INT NOT NULL DEFAULT 0,
    racewar INT NOT NULL DEFAULT 0,
    type INT NOT NULL DEFAULT 0,
    opt INT NOT NULL DEFAULT 0,
    criteria DECIMAL(10,2) NOT NULL DEFAULT 0.00,
    criteria2 DECIMAL(10,2) NOT NULL DEFAULT 0.00,
    bonus DECIMAL(10,2) NOT NULL DEFAULT 0.00,
    bonus2 DECIMAL(10,2) NOT NULL DEFAULT 0.00,
    random INT NOT NULL DEFAULT 0,
    author VARCHAR(20) DEFAULT NULL,
    active INT NOT NULL DEFAULT 0,
    pid INT NOT NULL DEFAULT 0,
    rpt INT NOT NULL DEFAULT 0
);
CREATE TABLE IF NOT EXISTS boons_progress (
    id INT AUTO_INCREMENT PRIMARY KEY,
    boonid INT NOT NULL DEFAULT 0,
    pid INT NOT NULL DEFAULT 0,
    counter DECIMAL(10,2) NOT NULL DEFAULT 0.00
);
CREATE TABLE IF NOT EXISTS boons_shop (
    id INT AUTO_INCREMENT PRIMARY KEY,
    pid INT NOT NULL DEFAULT 0,
    points INT NOT NULL DEFAULT 0,
    stats INT NOT NULL DEFAULT 0,
    UNIQUE KEY uk_pid (pid)
);"

run_sql "create categories table" "
CREATE TABLE IF NOT EXISTS categories (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(255) DEFAULT NULL,
    \`desc\` VARCHAR(255) DEFAULT NULL
);"

run_sql "create changes table" "
CREATE TABLE IF NOT EXISTS changes (
    id INT AUTO_INCREMENT PRIMARY KEY,
    history_id INT DEFAULT NULL,
    history_text TEXT,
    history_title VARCHAR(255) DEFAULT NULL,
    history_category_id INT DEFAULT NULL,
    new_text TEXT,
    new_title VARCHAR(255) DEFAULT NULL,
    new_category_id INT DEFAULT NULL,
    timestamp DATETIME DEFAULT NULL,
    action VARCHAR(255) DEFAULT NULL,
    ip_number VARCHAR(255) DEFAULT NULL
);"

run_sql "create ctf_data table" "
CREATE TABLE IF NOT EXISTS ctf_data (
    id INT AUTO_INCREMENT PRIMARY KEY,
    time TIMESTAMP NULL DEFAULT NULL,
    pid INT NOT NULL DEFAULT 0,
    type INT NOT NULL DEFAULT 0,
    flagtype INT NOT NULL DEFAULT 0,
    racewar INT NOT NULL DEFAULT 0
);"

run_sql "create epic tables" "
CREATE TABLE IF NOT EXISTS epic_bonus (
    pid INT NOT NULL,
    type INT NOT NULL DEFAULT 0,
    time DATETIME DEFAULT NULL,
    UNIQUE KEY uk_pid (pid)
);
CREATE TABLE IF NOT EXISTS epic_gain (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    pid BIGINT NOT NULL DEFAULT 0,
    time DATETIME NOT NULL,
    type INT NOT NULL DEFAULT 0,
    type_id INT NOT NULL DEFAULT 0,
    epics INT NOT NULL DEFAULT 0,
    INDEX idx_pid (pid)
);
CREATE TABLE IF NOT EXISTS eq_drop (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    date TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    vnum INT UNSIGNED NOT NULL DEFAULT 0,
    pid_looter BIGINT UNSIGNED NOT NULL DEFAULT 0,
    room_id INT UNSIGNED NOT NULL DEFAULT 0,
    INDEX idx_vnum (vnum)
);"

run_sql "create guildhall tables" "
CREATE TABLE IF NOT EXISTS guild_transactions (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    soc_id INT UNSIGNED NOT NULL DEFAULT 0,
    date INT NOT NULL DEFAULT 0,
    transaction_info VARCHAR(255) NOT NULL DEFAULT '',
    INDEX idx_soc_id (soc_id)
);
CREATE TABLE IF NOT EXISTS guildhall_rooms (
    id INT AUTO_INCREMENT PRIMARY KEY,
    guildhall_id INT NOT NULL DEFAULT 0,
    vnum INT NOT NULL DEFAULT 0,
    type INT NOT NULL DEFAULT 0,
    value0 INT UNSIGNED NOT NULL DEFAULT 0,
    value1 INT UNSIGNED NOT NULL DEFAULT 0,
    value2 INT UNSIGNED NOT NULL DEFAULT 0,
    value3 INT UNSIGNED NOT NULL DEFAULT 0,
    value4 INT UNSIGNED NOT NULL DEFAULT 0,
    value5 INT UNSIGNED NOT NULL DEFAULT 0,
    value6 INT UNSIGNED NOT NULL DEFAULT 0,
    value7 INT UNSIGNED NOT NULL DEFAULT 0,
    exit0 INT NOT NULL DEFAULT 0,
    exit1 INT NOT NULL DEFAULT 0,
    exit2 INT NOT NULL DEFAULT 0,
    exit3 INT NOT NULL DEFAULT 0,
    exit4 INT NOT NULL DEFAULT 0,
    exit5 INT NOT NULL DEFAULT 0,
    exit6 INT NOT NULL DEFAULT 0,
    exit7 INT NOT NULL DEFAULT 0,
    exit8 INT NOT NULL DEFAULT 0,
    exit9 INT NOT NULL,
    name VARCHAR(255) NOT NULL,
    INDEX idx_vnum (vnum),
    INDEX idx_guildhall_id (guildhall_id)
);
CREATE TABLE IF NOT EXISTS guildhalls (
    id INT AUTO_INCREMENT PRIMARY KEY,
    assoc_id INT NOT NULL DEFAULT 0,
    type INT NOT NULL DEFAULT 0,
    outside_vnum INT NOT NULL DEFAULT 0,
    racewar INT NOT NULL DEFAULT 0,
    INDEX idx_assoc_id (assoc_id)
);"

run_sql "create ip_info table" "
CREATE TABLE IF NOT EXISTS ip_info (
    pid BIGINT NOT NULL DEFAULT 0,
    last_ip VARCHAR(50) NOT NULL DEFAULT 'none',
    last_connect DATETIME NULL DEFAULT NULL,
    last_disconnect DATETIME NULL DEFAULT NULL,
    racewar_side INT NOT NULL DEFAULT 0,
    PRIMARY KEY (pid)
);"

run_sql "create items table" "
CREATE TABLE IF NOT EXISTS items (
    vnum INT UNSIGNED NOT NULL DEFAULT 0,
    short_desc VARCHAR(100) NOT NULL DEFAULT '',
    obj_stat TEXT NOT NULL,
    num_sold INT NOT NULL DEFAULT 0,
    avg_sell_price INT NOT NULL DEFAULT 0,
    PRIMARY KEY (vnum)
);"

run_sql "create level_cap table" "
CREATE TABLE IF NOT EXISTS level_cap (
    id INT AUTO_INCREMENT PRIMARY KEY,
    most_frags FLOAT NOT NULL DEFAULT 0,
    racewar_leader INT NOT NULL DEFAULT 0,
    level INT NOT NULL DEFAULT 25,
    next_update DATETIME DEFAULT CURRENT_TIMESTAMP
);
INSERT INTO level_cap (id, most_frags, racewar_leader, level, next_update)
SELECT 1, 0, 2, 56, NOW()
FROM DUAL WHERE NOT EXISTS (SELECT 1 FROM level_cap WHERE id = 1);"

run_sql "create log_entries table" "
CREATE TABLE IF NOT EXISTS log_entries (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    date DATETIME NOT NULL,
    kind VARCHAR(255) NOT NULL DEFAULT '',
    player_name VARCHAR(255) NOT NULL DEFAULT '',
    pid INT NOT NULL DEFAULT 0,
    ip_address VARCHAR(15) NOT NULL DEFAULT '',
    room_vnum INT NOT NULL DEFAULT 0,
    zone_number INT NOT NULL DEFAULT 0,
    message VARCHAR(255) NOT NULL DEFAULT '',
    INDEX idx_date (date),
    INDEX idx_kind (kind),
    INDEX idx_player_name (player_name),
    INDEX idx_pid (pid),
    INDEX idx_ip_address (ip_address),
    INDEX idx_room_vnum (room_vnum),
    INDEX idx_zone_number (zone_number)
);"

run_sql "create mud_info table" "
CREATE TABLE IF NOT EXISTS mud_info (
    name VARCHAR(255) NOT NULL,
    content TEXT NOT NULL,
    PRIMARY KEY (name)
);
INSERT INTO mud_info (name, content) VALUES
    ('motd', ''),
    ('wizmotd', ''),
    ('news', ''),
    ('rules', ''),
    ('credits', ''),
    ('info', ''),
    ('wizlist', ''),
    ('faq', '')
ON DUPLICATE KEY UPDATE name = name;"

run_sql "create multiplay_whitelist table" "
CREATE TABLE IF NOT EXISTS multiplay_whitelist (
    id INT AUTO_INCREMENT PRIMARY KEY,
    pattern VARCHAR(255) NOT NULL,
    admin VARCHAR(255) NOT NULL,
    description VARCHAR(255) NOT NULL,
    created_on DATE DEFAULT NULL,
    player VARCHAR(255) NOT NULL
);"

run_sql "create nexus_stones table" "
CREATE TABLE IF NOT EXISTS nexus_stones (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(255) NOT NULL DEFAULT '',
    room_vnum INT NOT NULL DEFAULT 0,
    align INT NOT NULL DEFAULT 0,
    stat_affect INT NOT NULL DEFAULT -1,
    affect_amount INT NOT NULL DEFAULT 0,
    last_touched_at TIMESTAMP NULL DEFAULT NULL,
    bonus INT NOT NULL DEFAULT 0
);"

run_sql "create offline_messages table" "
CREATE TABLE IF NOT EXISTS offline_messages (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    date DATETIME NOT NULL,
    pid INT NOT NULL DEFAULT 0,
    message TEXT NOT NULL
);"

run_sql "create outposts table" "
CREATE TABLE IF NOT EXISTS outposts (
    id INT NOT NULL,
    owner_id INT NOT NULL DEFAULT 0,
    level INT NOT NULL DEFAULT 1,
    walls INT NOT NULL DEFAULT 0,
    archers INT NOT NULL DEFAULT 0,
    resources INT NOT NULL DEFAULT 0,
    applied_resources INT NOT NULL DEFAULT 100000,
    hitpoints INT NOT NULL DEFAULT 0,
    territory INT NOT NULL DEFAULT 0,
    portal_room INT NOT NULL DEFAULT 0,
    golems INT NOT NULL DEFAULT 0,
    meurtriere INT NOT NULL DEFAULT 0,
    scouts INT NOT NULL DEFAULT 0,
    PRIMARY KEY (id)
);"

run_sql "create pages table" "
CREATE TABLE IF NOT EXISTS pages (
    id INT AUTO_INCREMENT PRIMARY KEY,
    title VARCHAR(255) DEFAULT NULL,
    text TEXT,
    last_update DATETIME DEFAULT NULL,
    last_update_by VARCHAR(255) DEFAULT NULL,
    category_id INT DEFAULT NULL,
    ip_number VARCHAR(255) DEFAULT NULL
);"

run_sql "create ping table" "
CREATE TABLE IF NOT EXISTS ping (
    ID BIGINT AUTO_INCREMENT PRIMARY KEY,
    TIMESTAMP DATETIME NOT NULL,
    URL VARCHAR(100) NOT NULL DEFAULT '',
    IP VARCHAR(100) NOT NULL DEFAULT '',
    SEQ BIGINT NOT NULL DEFAULT 0,
    TIME INT NOT NULL DEFAULT 0
);"

run_sql "create pkill tables" "
CREATE TABLE IF NOT EXISTS pkill_event (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    stamp DATETIME NOT NULL,
    room_vnum INT NOT NULL DEFAULT 0,
    room_name TEXT NOT NULL,
    tweeted TINYINT(1) NOT NULL DEFAULT 0
);
CREATE TABLE IF NOT EXISTS pkill_info (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    event_id INT UNSIGNED NOT NULL DEFAULT 0,
    pid BIGINT NOT NULL DEFAULT 0,
    level INT NOT NULL DEFAULT 0,
    pk_type TEXT NOT NULL,
    equip TEXT NOT NULL,
    log TEXT,
    inroom INT NOT NULL DEFAULT 0,
    leader INT DEFAULT NULL,
    player_description VARCHAR(255),
    INDEX idx_event_id (event_id),
    INDEX idx_pid (pid)
);"

run_sql "create prepstatement table" "
CREATE TABLE IF NOT EXISTS prepstatement_duris_sql (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    description TEXT DEFAULT NULL,
    sql_code TEXT DEFAULT NULL
);

SET @legacy_exists = (SELECT COUNT(*) FROM information_schema.tables
    WHERE table_schema = DATABASE() AND table_name = 'prepstatment_duris_sql');
SET @sql = IF(@legacy_exists = 1,
    CONCAT('INSERT IGNORE INTO prepstatement_duris_sql (id, description, sql_code) SELECT id, ', CHAR(96), 'desc', CHAR(96), ', ', CHAR(96), 'sql', CHAR(96), ' FROM prepstatment_duris_sql'),
    'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
"

run_sql "create progress table" "
CREATE TABLE IF NOT EXISTS progress (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    pid BIGINT NOT NULL DEFAULT 0,
    var_type INT NOT NULL DEFAULT 1,
    stamp DATETIME NOT NULL,
    delta INT NOT NULL DEFAULT 0,
    INDEX idx_pid (pid),
    INDEX idx_var_type (var_type)
);"

run_sql "create racewar_stat_mods table" "
CREATE TABLE IF NOT EXISTS racewar_stat_mods (
    racewar INT NOT NULL DEFAULT 0,
    Str INT NOT NULL DEFAULT 0,
    Dex INT NOT NULL DEFAULT 0,
    Agi INT NOT NULL DEFAULT 0,
    Con INT NOT NULL DEFAULT 0,
    Pow INT NOT NULL DEFAULT 0,
    Intl INT NOT NULL DEFAULT 0,
    Wis INT NOT NULL DEFAULT 0,
    Cha INT NOT NULL DEFAULT 0,
    Kar INT NOT NULL DEFAULT 0,
    Luc INT NOT NULL DEFAULT 0
);"

run_sql "create ship cargo tables" "
CREATE TABLE IF NOT EXISTS ship_cargo_market_mods (
    type VARCHAR(255) NOT NULL DEFAULT '',
    port_id INT NOT NULL DEFAULT -1,
    cargo_type INT NOT NULL DEFAULT -1,
    modifier FLOAT NOT NULL DEFAULT 0,
    INDEX idx_type_port_cargo (type, port_id, cargo_type)
);
CREATE TABLE IF NOT EXISTS ship_cargo_prices (
    type VARCHAR(255) NOT NULL DEFAULT '',
    port_id INT NOT NULL DEFAULT -1,
    cargo_type INT NOT NULL DEFAULT -1,
    price INT NOT NULL DEFAULT 0,
    INDEX idx_type_port_cargo (type, port_id, cargo_type)
);"

run_sql "create shop_trophy table" "
CREATE TABLE IF NOT EXISTS shop_trophy (
    id INT AUTO_INCREMENT PRIMARY KEY,
    item INT NOT NULL DEFAULT 0,
    value INT NOT NULL DEFAULT 0,
    seller INT NOT NULL DEFAULT 0,
    timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);"

run_sql "create quest_trophy table" "
CREATE TABLE IF NOT EXISTS quest_trophy (
    id INT AUTO_INCREMENT PRIMARY KEY,
    mob_vnum INT NOT NULL DEFAULT 0,
    pid BIGINT NOT NULL DEFAULT 0,
    type INT NOT NULL DEFAULT 0,
    reward_value INT NOT NULL DEFAULT 0,
    timestamp DATETIME NOT NULL,
    INDEX idx_mob_vnum (mob_vnum),
    INDEX idx_pid (pid)
);"

run_sql "create statistics table" "
CREATE TABLE IF NOT EXISTS statistics (
    id INT AUTO_INCREMENT PRIMARY KEY,
    date INT NOT NULL DEFAULT 0,
    goods_count INT NOT NULL DEFAULT 0,
    evils_count INT NOT NULL DEFAULT 0,
    illithids_count INT NOT NULL DEFAULT 0,
    undeads_count INT NOT NULL DEFAULT 0,
    gods_count INT NOT NULL DEFAULT 0,
    in_guildhall_count INT NOT NULL DEFAULT 0,
    sum_goods_levels INT NOT NULL DEFAULT 0,
    sum_evils_levels INT NOT NULL DEFAULT 0,
    sum_illithids_levels INT NOT NULL DEFAULT 0,
    sum_undeads_levels INT NOT NULL DEFAULT 0,
    unique_ips_count INT NOT NULL DEFAULT 0
);"

run_sql "create timers table" "
CREATE TABLE IF NOT EXISTS timers (
    name VARCHAR(255) NOT NULL DEFAULT '',
    date INT NOT NULL DEFAULT 0,
    PRIMARY KEY (name)
);"

run_sql "create world_quest_accomplished table" "
CREATE TABLE IF NOT EXISTS world_quest_accomplished (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    pid VARCHAR(45) NOT NULL DEFAULT '',
    timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    quest_giver INT UNSIGNED NOT NULL DEFAULT 0,
    player_name VARCHAR(45) NOT NULL DEFAULT '',
    player_level INT UNSIGNED NOT NULL DEFAULT 0,
    quest_target INT NOT NULL DEFAULT 0,
    reward_vnum INT NOT NULL DEFAULT 0,
    reward_desc VARCHAR(255) NOT NULL DEFAULT ''
);"

run_sql "create zone tables" "
CREATE TABLE IF NOT EXISTS zones (
    id INT AUTO_INCREMENT PRIMARY KEY,
    number INT DEFAULT NULL,
    name VARCHAR(100) NOT NULL DEFAULT '',
    epic_type INT NOT NULL DEFAULT 0,
    frequency_mod FLOAT NOT NULL DEFAULT 1,
    zone_freq_mod FLOAT NOT NULL DEFAULT 1,
    epic_level INT NOT NULL DEFAULT 0,
    task_zone TINYINT(1) NOT NULL DEFAULT 0,
    quest_zone TINYINT(1) NOT NULL DEFAULT 0,
    trophy_zone TINYINT(1) NOT NULL DEFAULT 1,
    suggested_group_size INT NOT NULL DEFAULT 1,
    epic_payout INT NOT NULL DEFAULT 0,
    difficulty INT NOT NULL DEFAULT 0,
    randoms_zone TINYINT(1) NOT NULL DEFAULT 1,
    alignment INT NOT NULL DEFAULT 0,
    last_touch TIMESTAMP NULL DEFAULT NULL,
    reset_perc INT DEFAULT 0,
    stonecount INT NOT NULL DEFAULT 1,
    INDEX idx_number (number)
);
CREATE TABLE IF NOT EXISTS zone_touches (
    id INT AUTO_INCREMENT PRIMARY KEY,
    boot_time TIMESTAMP NULL DEFAULT NULL,
    zone_number INT DEFAULT NULL,
    touched_at TIMESTAMP NULL DEFAULT NULL,
    toucher_pid INT DEFAULT NULL,
    group_size INT DEFAULT NULL,
    epic_value INT DEFAULT NULL,
    alignment_delta INT DEFAULT NULL,
    INDEX idx_zone_number (zone_number)
);
CREATE TABLE IF NOT EXISTS zone_trophy (
    pid BIGINT NOT NULL DEFAULT 0,
    zone_number INT NOT NULL DEFAULT 0,
    exp INT NOT NULL DEFAULT 0,
    PRIMARY KEY (pid, zone_number),
    INDEX idx_pid (pid),
    INDEX idx_zone_number (zone_number),
    INDEX idx_exp (exp)
);"

run_sql "create artifacts tables" "
CREATE TABLE IF NOT EXISTS artifacts (
    vnum INT NOT NULL,
    owned CHAR(1) NOT NULL,
    locType INT NOT NULL DEFAULT 1,
    location INT NOT NULL,
    timer DATETIME DEFAULT NULL,
    type INT NOT NULL,
    lastUpdate DATETIME DEFAULT NULL,
    PRIMARY KEY (vnum)
);
CREATE TABLE IF NOT EXISTS artifacts_mortal (
    vnum INT NOT NULL,
    owned CHAR(1) NOT NULL,
    locType INT NOT NULL,
    location INT NOT NULL,
    timer DATETIME DEFAULT NULL,
    type INT NOT NULL,
    PRIMARY KEY (vnum)
);
CREATE TABLE IF NOT EXISTS locker_access (
    owner VARCHAR(255) NOT NULL,
    visitor VARCHAR(255) NOT NULL,
    PRIMARY KEY (owner, visitor)
);"

# ============================================================================
# alter existing tables to fix column types for existing databases
# ============================================================================

run_sql "convert artifacts locType enum to int" "
DELIMITER //
CREATE PROCEDURE convert_artifacts_loctype()
BEGIN
    DECLARE col_type VARCHAR(64);
    SELECT DATA_TYPE INTO col_type FROM information_schema.columns
        WHERE table_schema = DATABASE() AND table_name = 'artifacts' AND column_name = 'locType';
    IF col_type = 'enum' THEN
        ALTER TABLE artifacts ADD COLUMN locType_new INT NOT NULL DEFAULT 1;
        UPDATE artifacts SET locType_new = CASE locType
            WHEN 'NotInGame' THEN 1 WHEN 'OnNPC' THEN 2 WHEN 'OnPC' THEN 3
            WHEN 'OnGround' THEN 4 WHEN 'OnCorpse' THEN 5 ELSE 1 END;
        ALTER TABLE artifacts DROP COLUMN locType;
        ALTER TABLE artifacts CHANGE COLUMN locType_new locType INT NOT NULL DEFAULT 1;
    END IF;
    SELECT DATA_TYPE INTO col_type FROM information_schema.columns
        WHERE table_schema = DATABASE() AND table_name = 'artifacts_mortal' AND column_name = 'locType';
    IF col_type = 'enum' THEN
        ALTER TABLE artifacts_mortal ADD COLUMN locType_new INT NOT NULL DEFAULT 1;
        UPDATE artifacts_mortal SET locType_new = CASE locType
            WHEN 'NotInGame' THEN 1 WHEN 'OnNPC' THEN 2 WHEN 'OnPC' THEN 3
            WHEN 'OnGround' THEN 4 WHEN 'OnCorpse' THEN 5 ELSE 1 END;
        ALTER TABLE artifacts_mortal DROP COLUMN locType;
        ALTER TABLE artifacts_mortal CHANGE COLUMN locType_new locType INT NOT NULL DEFAULT 1;
    END IF;
END //
DELIMITER ;
CALL convert_artifacts_loctype();
DROP PROCEDURE IF EXISTS convert_artifacts_loctype;"

run_sql "convert auctions timestamps" "
DELIMITER //
CREATE PROCEDURE convert_auctions_timestamps()
BEGIN
    DECLARE col_type VARCHAR(64);
    SELECT DATA_TYPE INTO col_type FROM information_schema.columns
        WHERE table_schema = DATABASE() AND table_name = 'auctions' AND column_name = 'start_time';
    IF col_type = 'int' THEN
        ALTER TABLE auctions ADD COLUMN start_time_new TIMESTAMP NULL DEFAULT NULL;
        ALTER TABLE auctions ADD COLUMN end_time_new TIMESTAMP NULL DEFAULT NULL;
        UPDATE auctions SET start_time_new = FROM_UNIXTIME(start_time) WHERE start_time > 0;
        UPDATE auctions SET end_time_new = FROM_UNIXTIME(end_time) WHERE end_time > 0;
        ALTER TABLE auctions DROP COLUMN start_time;
        ALTER TABLE auctions DROP COLUMN end_time;
        ALTER TABLE auctions CHANGE COLUMN start_time_new start_time TIMESTAMP NULL DEFAULT NULL;
        ALTER TABLE auctions CHANGE COLUMN end_time_new end_time TIMESTAMP NULL DEFAULT NULL;
    END IF;
END //
DELIMITER ;
CALL convert_auctions_timestamps();
DROP PROCEDURE IF EXISTS convert_auctions_timestamps;"

run_sql "convert ctf_data timestamps" "
SET @col_type = (SELECT DATA_TYPE FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'ctf_data' AND column_name = 'time');
SET @sql = IF(@col_type = 'int',
    'ALTER TABLE ctf_data MODIFY COLUMN time TIMESTAMP NULL DEFAULT NULL',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;"

run_sql "convert nexus_stones timestamps" "
SET @col_type = (SELECT DATA_TYPE FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'nexus_stones' AND column_name = 'last_touched_at');
SET @sql = IF(@col_type = 'int',
    'ALTER TABLE nexus_stones MODIFY COLUMN last_touched_at TIMESTAMP NULL DEFAULT NULL',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;"

run_sql "convert zones timestamps" "
DELIMITER //
CREATE PROCEDURE convert_zones_timestamps()
BEGIN
    DECLARE col_type VARCHAR(64);
    SELECT DATA_TYPE INTO col_type FROM information_schema.columns
        WHERE table_schema = DATABASE() AND table_name = 'zones' AND column_name = 'last_touch';
    IF col_type = 'int' THEN
        ALTER TABLE zones ADD COLUMN last_touch_new TIMESTAMP NULL DEFAULT NULL;
        UPDATE zones SET last_touch_new = FROM_UNIXTIME(last_touch) WHERE last_touch > 0;
        ALTER TABLE zones DROP COLUMN last_touch;
        ALTER TABLE zones CHANGE COLUMN last_touch_new last_touch TIMESTAMP NULL DEFAULT NULL;
    END IF;
END //
DELIMITER ;
CALL convert_zones_timestamps();
DROP PROCEDURE IF EXISTS convert_zones_timestamps;"

run_sql "convert zone_touches timestamps" "
DELIMITER //
CREATE PROCEDURE convert_zone_touches_timestamps()
BEGIN
    DECLARE col_type VARCHAR(64);
    SELECT DATA_TYPE INTO col_type FROM information_schema.columns
        WHERE table_schema = DATABASE() AND table_name = 'zone_touches' AND column_name = 'boot_time';
    IF col_type = 'int' THEN
        ALTER TABLE zone_touches ADD COLUMN boot_time_new TIMESTAMP NULL DEFAULT NULL;
        ALTER TABLE zone_touches ADD COLUMN touched_at_new TIMESTAMP NULL DEFAULT NULL;
        UPDATE zone_touches SET boot_time_new = FROM_UNIXTIME(boot_time) WHERE boot_time > 0;
        UPDATE zone_touches SET touched_at_new = FROM_UNIXTIME(touched_at) WHERE touched_at > 0;
        ALTER TABLE zone_touches DROP COLUMN boot_time;
        ALTER TABLE zone_touches DROP COLUMN touched_at;
        ALTER TABLE zone_touches CHANGE COLUMN boot_time_new boot_time TIMESTAMP NULL DEFAULT NULL;
        ALTER TABLE zone_touches CHANGE COLUMN touched_at_new touched_at TIMESTAMP NULL DEFAULT NULL;
    END IF;
END //
DELIMITER ;
CALL convert_zone_touches_timestamps();
DROP PROCEDURE IF EXISTS convert_zone_touches_timestamps;"

# additional item_material migrations needed by sql_save_player() and sql_load_player_items()
run_sql "add item_material columns to item tables" "
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'corpse_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0, 'ALTER TABLE corpse_items ADD COLUMN item_material TINYINT DEFAULT NULL', 'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'locker_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0, 'ALTER TABLE locker_items ADD COLUMN item_material TINYINT DEFAULT NULL', 'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'shopkeeper_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0, 'ALTER TABLE shopkeeper_items ADD COLUMN item_material TINYINT DEFAULT NULL', 'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'siege_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0, 'ALTER TABLE siege_items ADD COLUMN item_material TINYINT DEFAULT NULL', 'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'saved_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0, 'ALTER TABLE saved_items ADD COLUMN item_material TINYINT DEFAULT NULL', 'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'player_pet_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0, 'ALTER TABLE player_pet_items ADD COLUMN item_material TINYINT DEFAULT NULL', 'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'account_locker_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0, 'ALTER TABLE account_locker_items ADD COLUMN item_material TINYINT DEFAULT NULL', 'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'player_items' AND column_name = 'item_material');
SET @sql = IF(@col_exists = 0, 'ALTER TABLE player_items ADD COLUMN item_material TINYINT DEFAULT NULL', 'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;"

# epic_bonus/epic_gain pid remapping is now handled by the C migration tool
# (src-migrate/migrate_players.c) which has access to the old pids from pfiles

convert_tables_to_charset "ensure consistent collation on all tables" 1

# create persistence event tables required by async ownership and zone-touch paths
run_sql "create persistence event tables" "
CREATE TABLE IF NOT EXISTS persistence_item_events (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    ts_usec BIGINT UNSIGNED NOT NULL,
    event_type VARCHAR(64) NOT NULL DEFAULT '',
    item_uid BIGINT UNSIGNED NOT NULL DEFAULT 0,
    vnum INT NOT NULL DEFAULT -1,
    item VARCHAR(255) NOT NULL DEFAULT '',
    actor VARCHAR(128) NOT NULL DEFAULT '',
    actor_id INT NOT NULL DEFAULT -1,
    source VARCHAR(255) NOT NULL DEFAULT '',
    target VARCHAR(255) NOT NULL DEFAULT '',
    note VARCHAR(255) NOT NULL DEFAULT '',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_item_uid_ts (item_uid, ts_usec, id),
    INDEX idx_event_type_created (event_type, created_at)
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS persistence_scalar_events (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    event_type VARCHAR(64) NOT NULL DEFAULT '',
    event_key VARCHAR(255) NOT NULL DEFAULT '',
    boot_time INT NOT NULL DEFAULT 0,
    touched_at INT NOT NULL DEFAULT 0,
    zone_number INT NOT NULL DEFAULT 0,
    toucher_pid INT NOT NULL DEFAULT 0,
    group_size INT NOT NULL DEFAULT 0,
    epic_value INT NOT NULL DEFAULT 0,
    alignment_delta INT NOT NULL DEFAULT 0,
    dedupe_key VARCHAR(64) DEFAULT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_scalar_event_key (event_type, event_key),
    INDEX idx_scalar_zone_time (zone_number, touched_at),
    UNIQUE KEY uq_scalar_dedupe (dedupe_key)
) ENGINE=InnoDB;"

run_sql "add item event dedupe key" "
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'dedupe_key');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE persistence_item_events ADD COLUMN dedupe_key VARCHAR(64) DEFAULT NULL',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND index_name = 'uq_item_dedupe');
SET @sql = IF(@idx_exists = 0,
    'CREATE UNIQUE INDEX uq_item_dedupe ON persistence_item_events(dedupe_key)',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;"

run_sql "add scalar event dedupe key" "
SET @col_exists = (SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'dedupe_key');
SET @sql = IF(@col_exists = 0,
    'ALTER TABLE persistence_scalar_events ADD COLUMN dedupe_key VARCHAR(64) DEFAULT NULL',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND index_name = 'uq_scalar_dedupe');
SET @sql = IF(@idx_exists = 0,
    'CREATE UNIQUE INDEX uq_scalar_dedupe ON persistence_scalar_events(dedupe_key)',
    'SELECT 1 INTO @dummy');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;"

# Repair partially deployed persistence-event tables. CREATE TABLE IF NOT EXISTS
# does not alter an existing table, so add every runtime-required column/index
# conditionally before workers issue inserts or ownership queries.
run_sql "repair persistence event schema drift" "
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'id') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY FIRST', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'id') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY FIRST', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'ts_usec') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN ts_usec BIGINT UNSIGNED NOT NULL DEFAULT 0', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'event_type') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN event_type VARCHAR(64) NOT NULL DEFAULT \'\'', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'item_uid') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN item_uid BIGINT UNSIGNED NOT NULL DEFAULT 0', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'vnum') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN vnum INT NOT NULL DEFAULT -1', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'item') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN item VARCHAR(255) NOT NULL DEFAULT \'\'', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'actor') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN actor VARCHAR(128) NOT NULL DEFAULT \'\'', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'actor_id') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN actor_id INT NOT NULL DEFAULT -1', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'source') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN source VARCHAR(255) NOT NULL DEFAULT \'\'', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'target') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN target VARCHAR(255) NOT NULL DEFAULT \'\'', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'note') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN note VARCHAR(255) NOT NULL DEFAULT \'\'', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'created_at') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'event_type') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN event_type VARCHAR(64) NOT NULL DEFAULT \'\'', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'event_key') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN event_key VARCHAR(255) NOT NULL DEFAULT \'\'', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'boot_time') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN boot_time INT NOT NULL DEFAULT 0', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'touched_at') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN touched_at INT NOT NULL DEFAULT 0', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'zone_number') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN zone_number INT NOT NULL DEFAULT 0', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'toucher_pid') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN toucher_pid INT NOT NULL DEFAULT 0', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'group_size') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN group_size INT NOT NULL DEFAULT 0', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'epic_value') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN epic_value INT NOT NULL DEFAULT 0', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'alignment_delta') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN alignment_delta INT NOT NULL DEFAULT 0', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'created_at') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND index_name = 'idx_item_uid_ts'); SET @sql = IF(@idx_exists = 0, 'CREATE INDEX idx_item_uid_ts ON persistence_item_events(item_uid, ts_usec, id)', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND index_name = 'idx_event_type_created'); SET @sql = IF(@idx_exists = 0, 'CREATE INDEX idx_event_type_created ON persistence_item_events(event_type, created_at)', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND index_name = 'idx_scalar_event_key'); SET @sql = IF(@idx_exists = 0, 'CREATE INDEX idx_scalar_event_key ON persistence_scalar_events(event_type, event_key)', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND index_name = 'idx_scalar_zone_time'); SET @sql = IF(@idx_exists = 0, 'CREATE INDEX idx_scalar_zone_time ON persistence_scalar_events(zone_number, touched_at)', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;"

run_sql "convert auction tables to InnoDB" "
ALTER TABLE auction_bid_history ENGINE=InnoDB;
ALTER TABLE auction_item_pickups ENGINE=InnoDB;
ALTER TABLE auction_money_pickups ENGINE=InnoDB;
ALTER TABLE auctions ENGINE=InnoDB;
ALTER TABLE ship_cargo_prices ENGINE=InnoDB;
ALTER TABLE ship_cargo_market_mods ENGINE=InnoDB;"

# flush redis cache (migration invalidates all cached data)
STEP=$((STEP + 1))
printf "[%2d/%d] %s... " "$STEP" "$TOTAL" "flush redis cache"
if command -v redis-cli &> /dev/null; then
    redis-cli FLUSHDB > /dev/null 2>&1 && echo "ok" || echo "FAILED"
else
    echo "skipped (redis-cli not found)"
fi

echo ""
echo "done. $FAILED failures."
exit $FAILED
