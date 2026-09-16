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
    result_code INT NOT NULL,
    durable_revision BIGINT UNSIGNED NOT NULL,
    result_payload VARBINARY(4096) NOT NULL,
    committed_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (operation_id)
) ENGINE=InnoDB;

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
