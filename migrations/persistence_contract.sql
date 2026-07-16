-- Canonical upgrade contract for asynchronous persistence and transactional auctions.
-- This file is additive/idempotent and intentionally does not touch Redis.

CREATE TABLE IF NOT EXISTS persistence_item_events (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
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
    dedupe_key VARCHAR(64) DEFAULT NULL,
    PRIMARY KEY (id),
    UNIQUE KEY uq_item_dedupe (dedupe_key),
    KEY idx_item_uid_ts (item_uid, ts_usec, id),
    KEY idx_event_type_created (event_type, created_at)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS persistence_scalar_events (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
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
    PRIMARY KEY (id),
    UNIQUE KEY uq_scalar_dedupe (dedupe_key),
    KEY idx_scalar_event_key (event_type, event_key),
    KEY idx_scalar_zone_time (zone_number, touched_at)
) ENGINE=InnoDB;

-- CREATE TABLE IF NOT EXISTS cannot repair an interrupted deployment. Guard every
-- runtime-required column independently so any partial state converges on replay.
-- If id itself is missing, remove an incompatible legacy primary key first so
-- the guarded AUTO_INCREMENT PRIMARY KEY add can succeed on a populated table.
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='persistence_item_events' AND column_name='id') = 0 AND (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='persistence_item_events' AND index_name='PRIMARY') > 0, 'ALTER TABLE persistence_item_events DROP PRIMARY KEY', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='persistence_scalar_events' AND column_name='id') = 0 AND (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='persistence_scalar_events' AND index_name='PRIMARY') > 0, 'ALTER TABLE persistence_scalar_events DROP PRIMARY KEY', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'id') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY FIRST', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'ts_usec') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN ts_usec BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER id', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'event_type') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN event_type VARCHAR(64) NOT NULL DEFAULT \'\' AFTER ts_usec', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'item_uid') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN item_uid BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER event_type', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'vnum') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN vnum INT NOT NULL DEFAULT -1 AFTER item_uid', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'item') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN item VARCHAR(255) NOT NULL DEFAULT \'\' AFTER vnum', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'actor') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN actor VARCHAR(128) NOT NULL DEFAULT \'\' AFTER item', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'actor_id') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN actor_id INT NOT NULL DEFAULT -1 AFTER actor', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'source') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN source VARCHAR(255) NOT NULL DEFAULT \'\' AFTER actor_id', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'target') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN target VARCHAR(255) NOT NULL DEFAULT \'\' AFTER source', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'note') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN note VARCHAR(255) NOT NULL DEFAULT \'\' AFTER target', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'created_at') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP AFTER note', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND column_name = 'dedupe_key') = 0, 'ALTER TABLE persistence_item_events ADD COLUMN dedupe_key VARCHAR(64) DEFAULT NULL AFTER created_at', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'id') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY FIRST', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'event_type') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN event_type VARCHAR(64) NOT NULL DEFAULT \'\' AFTER id', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'event_key') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN event_key VARCHAR(255) NOT NULL DEFAULT \'\' AFTER event_type', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'boot_time') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN boot_time INT NOT NULL DEFAULT 0 AFTER event_key', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'touched_at') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN touched_at INT NOT NULL DEFAULT 0 AFTER boot_time', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'zone_number') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN zone_number INT NOT NULL DEFAULT 0 AFTER touched_at', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'toucher_pid') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN toucher_pid INT NOT NULL DEFAULT 0 AFTER zone_number', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'group_size') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN group_size INT NOT NULL DEFAULT 0 AFTER toucher_pid', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'epic_value') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN epic_value INT NOT NULL DEFAULT 0 AFTER group_size', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'alignment_delta') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN alignment_delta INT NOT NULL DEFAULT 0 AFTER epic_value', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'dedupe_key') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN dedupe_key VARCHAR(64) DEFAULT NULL AFTER alignment_delta', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND column_name = 'created_at') = 0, 'ALTER TABLE persistence_scalar_events ADD COLUMN created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP AFTER dedupe_key', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- Ensure AUTO_INCREMENT ids are backed by the canonical primary key before
-- exact column normalization. A wrong existing primary key is replaced atomically.
SET @pk_sig = (SELECT GROUP_CONCAT(CONCAT(column_name, ':', non_unique) ORDER BY seq_in_index) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='persistence_item_events' AND index_name='PRIMARY'); SET @sql = IF(@pk_sig IS NULL, 'ALTER TABLE persistence_item_events ADD PRIMARY KEY (id)', IF(@pk_sig <> 'id:0', 'ALTER TABLE persistence_item_events DROP PRIMARY KEY, ADD PRIMARY KEY (id)', 'SELECT 1 INTO @dummy')); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @pk_sig = (SELECT GROUP_CONCAT(CONCAT(column_name, ':', non_unique) ORDER BY seq_in_index) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='persistence_scalar_events' AND index_name='PRIMARY'); SET @sql = IF(@pk_sig IS NULL, 'ALTER TABLE persistence_scalar_events ADD PRIMARY KEY (id)', IF(@pk_sig <> 'id:0', 'ALTER TABLE persistence_scalar_events DROP PRIMARY KEY, ADD PRIMARY KEY (id)', 'SELECT 1 INTO @dummy')); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- Normalize definitions and ordering, not just names. This makes a partial or
-- hand-created deployment structurally identical to a clean contract install.
UPDATE persistence_item_events SET
    ts_usec=COALESCE(ts_usec,0), event_type=COALESCE(event_type,''),
    item_uid=COALESCE(item_uid,0), vnum=COALESCE(vnum,-1), item=COALESCE(item,''),
    actor=COALESCE(actor,''), actor_id=COALESCE(actor_id,-1), source=COALESCE(source,''),
    target=COALESCE(target,''), note=COALESCE(note,''), created_at=COALESCE(created_at,CURRENT_TIMESTAMP);
ALTER TABLE persistence_item_events
    MODIFY COLUMN id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT FIRST,
    MODIFY COLUMN ts_usec BIGINT UNSIGNED NOT NULL AFTER id,
    MODIFY COLUMN event_type VARCHAR(64) NOT NULL DEFAULT '' AFTER ts_usec,
    MODIFY COLUMN item_uid BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER event_type,
    MODIFY COLUMN vnum INT NOT NULL DEFAULT -1 AFTER item_uid,
    MODIFY COLUMN item VARCHAR(255) NOT NULL DEFAULT '' AFTER vnum,
    MODIFY COLUMN actor VARCHAR(128) NOT NULL DEFAULT '' AFTER item,
    MODIFY COLUMN actor_id INT NOT NULL DEFAULT -1 AFTER actor,
    MODIFY COLUMN source VARCHAR(255) NOT NULL DEFAULT '' AFTER actor_id,
    MODIFY COLUMN target VARCHAR(255) NOT NULL DEFAULT '' AFTER source,
    MODIFY COLUMN note VARCHAR(255) NOT NULL DEFAULT '' AFTER target,
    MODIFY COLUMN created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP AFTER note,
    MODIFY COLUMN dedupe_key VARCHAR(64) DEFAULT NULL AFTER created_at;

UPDATE persistence_scalar_events SET
    event_type=COALESCE(event_type,''), event_key=COALESCE(event_key,''),
    boot_time=COALESCE(boot_time,0), touched_at=COALESCE(touched_at,0),
    zone_number=COALESCE(zone_number,0), toucher_pid=COALESCE(toucher_pid,0),
    group_size=COALESCE(group_size,0), epic_value=COALESCE(epic_value,0),
    alignment_delta=COALESCE(alignment_delta,0), created_at=COALESCE(created_at,CURRENT_TIMESTAMP);
ALTER TABLE persistence_scalar_events
    MODIFY COLUMN id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT FIRST,
    MODIFY COLUMN event_type VARCHAR(64) NOT NULL DEFAULT '' AFTER id,
    MODIFY COLUMN event_key VARCHAR(255) NOT NULL DEFAULT '' AFTER event_type,
    MODIFY COLUMN boot_time INT NOT NULL DEFAULT 0 AFTER event_key,
    MODIFY COLUMN touched_at INT NOT NULL DEFAULT 0 AFTER boot_time,
    MODIFY COLUMN zone_number INT NOT NULL DEFAULT 0 AFTER touched_at,
    MODIFY COLUMN toucher_pid INT NOT NULL DEFAULT 0 AFTER zone_number,
    MODIFY COLUMN group_size INT NOT NULL DEFAULT 0 AFTER toucher_pid,
    MODIFY COLUMN epic_value INT NOT NULL DEFAULT 0 AFTER group_size,
    MODIFY COLUMN alignment_delta INT NOT NULL DEFAULT 0 AFTER epic_value,
    MODIFY COLUMN dedupe_key VARCHAR(64) DEFAULT NULL AFTER alignment_delta,
    MODIFY COLUMN created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP AFTER dedupe_key;

-- Event tables share the clean bootstrap's transactional engine and collation.
SET @table_exact = (SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='persistence_item_events' AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci'); SET @sql = IF(@table_exact = 0, 'ALTER TABLE persistence_item_events ENGINE=InnoDB, CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @table_exact = (SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='persistence_scalar_events' AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci'); SET @sql = IF(@table_exact = 0, 'ALTER TABLE persistence_scalar_events ENGINE=InnoDB, CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- Named indexes must also have the canonical columns and uniqueness. Drop only
-- a same-named incompatible index; the guarded creates below restore it.
SET @idx_sig = (SELECT GROUP_CONCAT(CONCAT(column_name, ':', non_unique) ORDER BY seq_in_index) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='persistence_item_events' AND index_name='uq_item_dedupe'); SET @sql = IF(@idx_sig IS NOT NULL AND @idx_sig <> 'dedupe_key:0', 'DROP INDEX uq_item_dedupe ON persistence_item_events', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @idx_sig = (SELECT GROUP_CONCAT(CONCAT(column_name, ':', non_unique) ORDER BY seq_in_index) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='persistence_item_events' AND index_name='idx_item_uid_ts'); SET @sql = IF(@idx_sig IS NOT NULL AND @idx_sig <> 'item_uid:1,ts_usec:1,id:1', 'DROP INDEX idx_item_uid_ts ON persistence_item_events', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @idx_sig = (SELECT GROUP_CONCAT(CONCAT(column_name, ':', non_unique) ORDER BY seq_in_index) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='persistence_item_events' AND index_name='idx_event_type_created'); SET @sql = IF(@idx_sig IS NOT NULL AND @idx_sig <> 'event_type:1,created_at:1', 'DROP INDEX idx_event_type_created ON persistence_item_events', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @idx_sig = (SELECT GROUP_CONCAT(CONCAT(column_name, ':', non_unique) ORDER BY seq_in_index) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='persistence_scalar_events' AND index_name='uq_scalar_dedupe'); SET @sql = IF(@idx_sig IS NOT NULL AND @idx_sig <> 'dedupe_key:0', 'DROP INDEX uq_scalar_dedupe ON persistence_scalar_events', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @idx_sig = (SELECT GROUP_CONCAT(CONCAT(column_name, ':', non_unique) ORDER BY seq_in_index) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='persistence_scalar_events' AND index_name='idx_scalar_event_key'); SET @sql = IF(@idx_sig IS NOT NULL AND @idx_sig <> 'event_type:1,event_key:1', 'DROP INDEX idx_scalar_event_key ON persistence_scalar_events', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @idx_sig = (SELECT GROUP_CONCAT(CONCAT(column_name, ':', non_unique) ORDER BY seq_in_index) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='persistence_scalar_events' AND index_name='idx_scalar_zone_time'); SET @sql = IF(@idx_sig IS NOT NULL AND @idx_sig <> 'zone_number:1,touched_at:1', 'DROP INDEX idx_scalar_zone_time ON persistence_scalar_events', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND index_name = 'uq_item_dedupe'); SET @sql = IF(@idx_exists = 0, 'CREATE UNIQUE INDEX uq_item_dedupe ON persistence_item_events(dedupe_key)', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND index_name = 'idx_item_uid_ts'); SET @sql = IF(@idx_exists = 0, 'CREATE INDEX idx_item_uid_ts ON persistence_item_events(item_uid, ts_usec, id)', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema = DATABASE() AND table_name = 'persistence_item_events' AND index_name = 'idx_event_type_created'); SET @sql = IF(@idx_exists = 0, 'CREATE INDEX idx_event_type_created ON persistence_item_events(event_type, created_at)', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND index_name = 'uq_scalar_dedupe'); SET @sql = IF(@idx_exists = 0, 'CREATE UNIQUE INDEX uq_scalar_dedupe ON persistence_scalar_events(dedupe_key)', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND index_name = 'idx_scalar_event_key'); SET @sql = IF(@idx_exists = 0, 'CREATE INDEX idx_scalar_event_key ON persistence_scalar_events(event_type, event_key)', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @idx_exists = (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema = DATABASE() AND table_name = 'persistence_scalar_events' AND index_name = 'idx_scalar_zone_time'); SET @sql = IF(@idx_exists = 0, 'CREATE INDEX idx_scalar_zone_time ON persistence_scalar_events(zone_number, touched_at)', 'SELECT 1 INTO @dummy'); PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- The boot gate requires transactional auction settlement tables.
ALTER TABLE auction_bid_history ENGINE=InnoDB;
ALTER TABLE auction_item_pickups ENGINE=InnoDB;
ALTER TABLE auction_money_pickups ENGINE=InnoDB;
ALTER TABLE auctions ENGINE=InnoDB;
