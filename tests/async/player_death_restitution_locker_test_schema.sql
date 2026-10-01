USE duris_issue_331_test;

CREATE TABLE accounts (
    account_name VARCHAR(50) NOT NULL,
    PRIMARY KEY (account_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE account_characters (
    id INT NOT NULL AUTO_INCREMENT,
    account_name VARCHAR(255) NOT NULL,
    pid BIGINT NOT NULL,
    char_name VARCHAR(255) NOT NULL,
    racewar TINYINT NOT NULL DEFAULT 0,
    deleted_at DATETIME DEFAULT NULL,
    PRIMARY KEY (id),
    KEY idx_account_characters_pid (pid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE frag_leaderboard (
    id INT NOT NULL AUTO_INCREMENT,
    pid BIGINT NOT NULL,
    account_name VARCHAR(255) NOT NULL,
    char_name VARCHAR(255) NOT NULL,
    racewar INT NOT NULL,
    deleted_at DATETIME DEFAULT NULL,
    PRIMARY KEY (id),
    UNIQUE KEY uq_frag_leaderboard_pid (pid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE lockers (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    locker_name VARCHAR(100) NOT NULL,
    owner_pid INT DEFAULT NULL,
    owner_assoc_id INT DEFAULT NULL,
    racewar TINYINT DEFAULT 0,
    race TINYINT DEFAULT 0,
    PRIMARY KEY (id),
    UNIQUE KEY uq_lockers_name (locker_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE private_chests (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    locker_id INT UNSIGNED NOT NULL,
    chest_name VARCHAR(32) NOT NULL,
    password_hash VARCHAR(64) DEFAULT NULL,
    is_public TINYINT(1) DEFAULT 0,
    sort_config TEXT,
    PRIMARY KEY (id),
    UNIQUE KEY uq_private_chest (locker_id,chest_name),
    CONSTRAINT fk_private_chest_locker FOREIGN KEY (locker_id)
        REFERENCES lockers(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE locker_items (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    locker_id INT UNSIGNED NOT NULL,
    chest_id INT UNSIGNED DEFAULT NULL,
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
    description TEXT,
    action_descr TEXT,
    obj_uid BIGINT UNSIGNED DEFAULT NULL,
    item_condition SMALLINT DEFAULT 100,
    bitvector1 BIGINT UNSIGNED DEFAULT NULL,
    bitvector2 BIGINT UNSIGNED DEFAULT NULL,
    bitvector3 BIGINT UNSIGNED DEFAULT NULL,
    bitvector4 BIGINT UNSIGNED DEFAULT NULL,
    bitvector5 BIGINT UNSIGNED DEFAULT NULL,
    item_material TINYINT DEFAULT NULL,
    PRIMARY KEY (id),
    KEY idx_locker_items_uid (obj_uid),
    CONSTRAINT fk_locker_item_locker FOREIGN KEY (locker_id)
        REFERENCES lockers(id) ON DELETE CASCADE,
    CONSTRAINT fk_locker_item_container FOREIGN KEY (container_id)
        REFERENCES locker_items(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE locker_item_affects (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    PRIMARY KEY (id),
    CONSTRAINT fk_locker_item_affect FOREIGN KEY (item_id)
        REFERENCES locker_items(id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE locker_item_extra_descr (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    item_id INT UNSIGNED NOT NULL,
    keyword VARCHAR(255) NOT NULL,
    description TEXT,
    PRIMARY KEY (id),
    CONSTRAINT fk_locker_item_description FOREIGN KEY (item_id)
        REFERENCES locker_items(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE item_uid_allocator (
    allocator_id TINYINT UNSIGNED NOT NULL,
    next_uid BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (allocator_id)
) ENGINE=InnoDB;

CREATE TABLE corpse_items (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    obj_uid BIGINT UNSIGNED DEFAULT NULL,
    PRIMARY KEY (id), KEY idx_corpse_items_uid (obj_uid)
) ENGINE=InnoDB;
CREATE TABLE account_locker_items LIKE corpse_items;
CREATE TABLE saved_items LIKE corpse_items;
CREATE TABLE player_pet_items LIKE corpse_items;
CREATE TABLE shopkeeper_items LIKE corpse_items;
CREATE TABLE siege_items LIKE corpse_items;

DELETE FROM player_data WHERE pid=42;
INSERT INTO accounts(account_name) VALUES('acct42');
INSERT INTO frag_leaderboard(pid,account_name,char_name,racewar,deleted_at)
VALUES(42,'acct42','Deletedchar',1,'2026-09-20 01:02:03');
INSERT INTO item_uid_allocator(allocator_id,next_uid) VALUES(1,9000);
INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision)
VALUES(1,42,0,6);
INSERT INTO item_current_owner(
    item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,
    vnum,item_revision,state
) VALUES(100,100,NULL,1,42,0,677,11,3);
INSERT INTO player_death_disposition(
    pid,save_revision,operation_id,corpse_item_uid,corpse_room_vnum,wallet_revision,
    wallet_copper,wallet_silver,wallet_gold,wallet_platinum,wallet_pile_uid,payload,recorded_at
) VALUES(
    42,7,UNHEX(REPEAT('20',16)),999,3001,5,0,0,0,0,0,
    'death-payload',FROM_UNIXTIME(1700000000)
);
INSERT INTO player_death_custody(
    pid,save_revision,item_uid,root_item_uid,parent_item_uid,item_revision,vnum,state,
    owner_type,owner_id,owner_context_id,owner_revision
) VALUES(42,7,100,100,0,10,677,1,1,42,0,5);
