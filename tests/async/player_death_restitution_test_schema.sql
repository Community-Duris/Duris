CREATE DATABASE IF NOT EXISTS duris_issue_331_test;
USE duris_issue_331_test;

CREATE TABLE critical_operation_inbox (
    operation_id BINARY(16) NOT NULL,
    command_hash BINARY(32) NOT NULL,
    keys_hash BINARY(32) NOT NULL,
    command_type TINYINT UNSIGNED NOT NULL,
    schema_version INT UNSIGNED NOT NULL,
    payload_version INT UNSIGNED NOT NULL,
    status TINYINT UNSIGNED NOT NULL,
    result_code INT NOT NULL DEFAULT 0,
    durable_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    result_payload VARBINARY(4096) NOT NULL,
    committed_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (operation_id)
) ENGINE=InnoDB;

CREATE TABLE critical_outbox (
    outbox_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    operation_id BINARY(16) NOT NULL,
    event_index SMALLINT UNSIGNED NOT NULL,
    destination SMALLINT UNSIGNED NOT NULL,
    event_type SMALLINT UNSIGNED NOT NULL,
    payload_version SMALLINT UNSIGNED NOT NULL,
    payload BLOB NOT NULL,
    status TINYINT UNSIGNED NOT NULL DEFAULT 0,
    attempt_count SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    next_attempt_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    delivered_at TIMESTAMP(6) NULL,
    dead_lettered_at TIMESTAMP(6) NULL,
    last_error_code INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (outbox_id), UNIQUE KEY uq_critical_outbox_operation_event (operation_id,event_index)
) ENGINE=InnoDB;

CREATE TABLE player_data (
    pid INT UNSIGNED NOT NULL AUTO_INCREMENT,
    name VARCHAR(64) NOT NULL,
    account_name VARCHAR(50) DEFAULT NULL,
    short_descr VARCHAR(512) DEFAULT NULL,
    long_descr TEXT,
    description TEXT,
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
    save_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
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
    wallet_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    bank_copper BIGINT DEFAULT 0,
    bank_silver BIGINT DEFAULT 0,
    bank_gold BIGINT DEFAULT 0,
    bank_platinum BIGINT DEFAULT 0,
    exp BIGINT DEFAULT 0,
    epics BIGINT DEFAULT 0,
    epic_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
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
    frag_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
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
    created_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (pid),
    UNIQUE KEY idx_player_name_unique (name),
    KEY idx_name (name),
    KEY idx_account_name (account_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

INSERT INTO player_data (pid,name,account_name,save_revision,racewar)
VALUES (42,'issue331-source-player','acct42',8,0);

CREATE TABLE item_owner_revision (
    owner_type TINYINT UNSIGNED NOT NULL,
    owner_id BIGINT UNSIGNED NOT NULL,
    owner_context_id BIGINT UNSIGNED NOT NULL,
    revision BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (owner_type,owner_id,owner_context_id)
) ENGINE=InnoDB;

CREATE TABLE item_current_owner (
    item_uid BIGINT UNSIGNED NOT NULL,
    root_item_uid BIGINT UNSIGNED NOT NULL,
    parent_item_uid BIGINT UNSIGNED NULL,
    owner_type TINYINT UNSIGNED NOT NULL,
    owner_id BIGINT UNSIGNED NOT NULL,
    owner_context_id BIGINT UNSIGNED NOT NULL,
    vnum INT NOT NULL,
    item_revision BIGINT UNSIGNED NOT NULL,
    state TINYINT UNSIGNED NOT NULL,
    PRIMARY KEY (item_uid),
    KEY idx_item_current_owner_parent (parent_item_uid)
) ENGINE=InnoDB;

CREATE TABLE item_ownership_ledger (
    operation_id BINARY(16) NOT NULL,
    event_index INT UNSIGNED NOT NULL,
    item_uid BIGINT UNSIGNED NOT NULL,
    root_item_uid BIGINT UNSIGNED NOT NULL,
    parent_item_uid BIGINT UNSIGNED NULL,
    from_owner_type TINYINT UNSIGNED NOT NULL,
    from_owner_id BIGINT UNSIGNED NOT NULL,
    from_owner_context_id BIGINT UNSIGNED NOT NULL,
    to_owner_type TINYINT UNSIGNED NOT NULL,
    to_owner_id BIGINT UNSIGNED NOT NULL,
    to_owner_context_id BIGINT UNSIGNED NOT NULL,
    item_revision BIGINT UNSIGNED NOT NULL,
    from_owner_revision BIGINT UNSIGNED NOT NULL,
    to_owner_revision BIGINT UNSIGNED NOT NULL,
    reason_type TINYINT UNSIGNED NOT NULL,
    reason_id BIGINT UNSIGNED NOT NULL,
    source_site TINYINT UNSIGNED NOT NULL,
    PRIMARY KEY (operation_id,event_index),
    UNIQUE KEY uq_item_ownership_ledger_item_revision (item_uid,item_revision)
) ENGINE=InnoDB;

CREATE TABLE player_death_disposition (
    pid INT NOT NULL,
    save_revision BIGINT UNSIGNED NOT NULL,
    operation_id BINARY(16) NOT NULL,
    corpse_item_uid BIGINT UNSIGNED NOT NULL,
    corpse_room_vnum INT NOT NULL,
    wallet_revision BIGINT UNSIGNED NOT NULL,
    wallet_copper INT NOT NULL DEFAULT 0,
    wallet_silver INT NOT NULL DEFAULT 0,
    wallet_gold INT NOT NULL DEFAULT 0,
    wallet_platinum INT NOT NULL DEFAULT 0,
    wallet_pile_uid BIGINT UNSIGNED NOT NULL DEFAULT 0,
    payload MEDIUMBLOB NOT NULL,
    recorded_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (pid,save_revision)
) ENGINE=InnoDB;

CREATE TABLE player_death_custody (
    pid INT NOT NULL,
    save_revision BIGINT UNSIGNED NOT NULL,
    item_uid BIGINT UNSIGNED NOT NULL,
    root_item_uid BIGINT UNSIGNED NOT NULL,
    parent_item_uid BIGINT UNSIGNED NOT NULL DEFAULT 0,
    item_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    vnum INT NOT NULL DEFAULT 0,
    state TINYINT UNSIGNED NOT NULL DEFAULT 0,
    owner_type TINYINT UNSIGNED NOT NULL DEFAULT 0,
    owner_id BIGINT UNSIGNED NOT NULL DEFAULT 0,
    owner_context_id BIGINT UNSIGNED NOT NULL DEFAULT 0,
    owner_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (pid,save_revision,item_uid),
    KEY idx_player_death_custody_item (item_uid)
) ENGINE=InnoDB;

CREATE TABLE player_items (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    pid INT NOT NULL,
    vnum INT NOT NULL,
    equip_slot INT DEFAULT NULL,
    container_id INT UNSIGNED DEFAULT NULL,
    quantity INT NOT NULL DEFAULT 1,
    weight INT NOT NULL DEFAULT 0,
    cost INT NOT NULL DEFAULT 0,
    timer INT DEFAULT NULL,
    extra_flags BIGINT DEFAULT NULL,
    wear_flags BIGINT DEFAULT NULL,
    item_type INT DEFAULT NULL,
    value0 INT DEFAULT NULL, value1 INT DEFAULT NULL, value2 INT DEFAULT NULL, value3 INT DEFAULT NULL,
    value4 INT DEFAULT NULL, value5 INT DEFAULT NULL, value6 INT DEFAULT NULL, value7 INT DEFAULT NULL,
    name TEXT NULL, short_descr TEXT NULL, description TEXT NULL, action_descr TEXT NULL,
    bitvector1 BIGINT DEFAULT NULL, bitvector2 BIGINT DEFAULT NULL, bitvector3 BIGINT DEFAULT NULL,
    bitvector4 BIGINT DEFAULT NULL, bitvector5 BIGINT DEFAULT NULL,
    item_material INT DEFAULT NULL,
    obj_uid BIGINT UNSIGNED NOT NULL,
    item_condition INT DEFAULT NULL,
    PRIMARY KEY (id), UNIQUE KEY uq_player_items_uid (obj_uid), KEY idx_player_items_pid (pid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE player_item_affects (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    item_id INT UNSIGNED NOT NULL,
    location INT NOT NULL,
    modifier INT NOT NULL,
    PRIMARY KEY (id), KEY idx_player_item_affects_item (item_id)
) ENGINE=InnoDB;

CREATE TABLE player_item_extra_descr (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    item_id INT UNSIGNED NOT NULL,
    keyword VARCHAR(255) NOT NULL,
    description TEXT NOT NULL,
    PRIMARY KEY (id), KEY idx_player_item_extra_descr_item (item_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE artifact_bind (
    vnum INT NOT NULL PRIMARY KEY,
    owner_pid INT DEFAULT NULL,
    timer INT DEFAULT NULL
) ENGINE=InnoDB;
CREATE TABLE artifacts (
    vnum INT NOT NULL PRIMARY KEY,
    owned VARCHAR(10) DEFAULT 'N',
    location INT DEFAULT NULL,
    timer DATETIME DEFAULT NULL,
    type INT DEFAULT NULL,
    lastUpdate DATETIME DEFAULT NULL,
    locType INT NOT NULL DEFAULT 1
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
CREATE TABLE artifacts_mortal LIKE artifacts;

CREATE TABLE artifact_domain_state (
    vnum INT NOT NULL PRIMARY KEY,
    owned TINYINT UNSIGNED NOT NULL DEFAULT 0,
    loc_type TINYINT UNSIGNED NOT NULL DEFAULT 1,
    location INT NOT NULL DEFAULT 0,
    timer_epoch BIGINT NOT NULL DEFAULT 0,
    artifact_type TINYINT UNSIGNED NOT NULL DEFAULT 0,
    bind_owner_pid INT NOT NULL DEFAULT 0,
    bind_timer_epoch BIGINT NOT NULL DEFAULT 0,
    item_uid BIGINT UNSIGNED NULL,
    item_revision BIGINT UNSIGNED NULL,
    revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    KEY idx_artifact_domain_item (item_uid)
) ENGINE=InnoDB;

CREATE TABLE artifact_domain_baseline (
    vnum INT NOT NULL PRIMARY KEY,
    opening_timer_epoch BIGINT NOT NULL,
    opening_bind_owner_pid INT NOT NULL,
    opening_bind_timer_epoch BIGINT NOT NULL,
    opening_revision BIGINT UNSIGNED NOT NULL DEFAULT 0
) ENGINE=InnoDB;
