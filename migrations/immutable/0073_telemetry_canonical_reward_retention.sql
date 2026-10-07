-- Independent reward definition 2: private exact native bank source cuts.
-- Additive and re-runnable. No gameplay authority or report publication.
-- Register in immutable history only after disposable schema qualification.

CREATE TABLE IF NOT EXISTS telemetry_reward_cut_v2 (
    cut_id BINARY(32) NOT NULL,
    definition_version INT UNSIGNED NOT NULL,
    captured_utc_usec BIGINT UNSIGNED NOT NULL,
    source_digest BINARY(32) NOT NULL,
    selected_count INT UNSIGNED NOT NULL,
    source_count INT UNSIGNED NOT NULL,
    reserved_bytes INT UNSIGNED NOT NULL,
    sealed TINYINT UNSIGNED NOT NULL,
    discovery_bucket TINYINT UNSIGNED NULL,
    discovery_cursor BINARY(16) NULL,
    discovery_limit SMALLINT UNSIGNED NULL,
    discovery_through BINARY(16) NULL,
    discovery_upper BINARY(16) NULL,
    PRIMARY KEY (cut_id),
    CONSTRAINT chk_reward2_cut_identity CHECK (definition_version = 2 AND cut_id = source_digest AND captured_utc_usec > 0),
    CONSTRAINT chk_reward2_cut_bounds CHECK (selected_count <= 16384 AND source_count <= 16384 AND reserved_bytes BETWEEN 4096 AND 33554432),
    CONSTRAINT chk_reward2_cut_sealed CHECK (sealed BETWEEN 0 AND 1),
    CONSTRAINT chk_reward2_cut_discovery CHECK ((discovery_bucket IS NULL AND discovery_cursor IS NULL AND discovery_limit IS NULL AND discovery_through IS NULL AND discovery_upper IS NULL) OR (discovery_bucket IS NOT NULL AND discovery_cursor IS NOT NULL AND discovery_limit BETWEEN 1 AND 666 AND discovery_upper IS NOT NULL AND (discovery_cursor = UNHEX(REPEAT(CHAR(48),32)) OR ORD(SUBSTRING(discovery_cursor,1,1)) = discovery_bucket) AND (discovery_upper = UNHEX(REPEAT(CHAR(48),32)) OR ORD(SUBSTRING(discovery_upper,1,1)) = discovery_bucket) AND (discovery_through IS NULL OR (discovery_upper = discovery_through AND discovery_cursor <= discovery_through))))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_reward_selection_v2 (
    cut_id BINARY(32) NOT NULL,
    selection_index SMALLINT UNSIGNED NOT NULL,
    operation_id BINARY(16) NOT NULL,
    PRIMARY KEY (cut_id,selection_index),
    UNIQUE KEY uq_reward2_selection_operation (cut_id,operation_id),
    CONSTRAINT fk_reward2_selection_cut FOREIGN KEY (cut_id) REFERENCES telemetry_reward_cut_v2 (cut_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_reward2_selection_index CHECK (selection_index < 16384),
    CONSTRAINT chk_reward2_selection_operation CHECK (operation_id <> UNHEX(REPEAT(CHAR(48),32)))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_reward_source_v2 (
    cut_id BINARY(32) NOT NULL,
    source_index SMALLINT UNSIGNED NOT NULL,
    source_table VARBINARY(64) NOT NULL,
    source_key VARBINARY(128) NOT NULL,
    payload VARBINARY(8192) NOT NULL,
    payload_digest BINARY(32) NOT NULL,
    PRIMARY KEY (cut_id,source_index),
    UNIQUE KEY uq_reward2_source_identity (cut_id,source_table,source_key),
    KEY idx_reward2_source_payloads (source_table,source_key,payload_digest,cut_id),
    CONSTRAINT fk_reward2_source_cut FOREIGN KEY (cut_id) REFERENCES telemetry_reward_cut_v2 (cut_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_reward2_source_index CHECK (source_index < 16384),
    CONSTRAINT chk_reward2_source_key CHECK (OCTET_LENGTH(source_table) BETWEEN 1 AND 64 AND OCTET_LENGTH(source_key) BETWEEN 1 AND 128),
    CONSTRAINT chk_reward2_source_payload CHECK (OCTET_LENGTH(payload) BETWEEN 1 AND 8192)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TRIGGER IF EXISTS telemetry_reward2_cut_insert;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_cut_insert BEFORE INSERT ON telemetry_reward_cut_v2 FOR EACH ROW
BEGIN
    IF NEW.sealed <> 0 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical reward cut must start unsealed';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_cut_update;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_cut_update BEFORE UPDATE ON telemetry_reward_cut_v2 FOR EACH ROW
BEGIN
    IF OLD.sealed <> 0 OR NEW.sealed <> 1 OR NOT (NEW.cut_id <=> OLD.cut_id) OR NOT (NEW.definition_version <=> OLD.definition_version) OR NOT (NEW.captured_utc_usec <=> OLD.captured_utc_usec) OR NOT (NEW.source_digest <=> OLD.source_digest) OR NOT (NEW.selected_count <=> OLD.selected_count) OR NOT (NEW.source_count <=> OLD.source_count) OR NOT (NEW.reserved_bytes <=> OLD.reserved_bytes) OR NOT (NEW.discovery_bucket <=> OLD.discovery_bucket) OR NOT (NEW.discovery_cursor <=> OLD.discovery_cursor) OR NOT (NEW.discovery_limit <=> OLD.discovery_limit) OR NOT (NEW.discovery_through <=> OLD.discovery_through) OR NOT (NEW.discovery_upper <=> OLD.discovery_upper) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical reward cut permits only exact sealing';
    END IF;
    IF (SELECT COUNT(*) FROM telemetry_reward_selection_v2 WHERE cut_id = OLD.cut_id) <> OLD.selected_count OR (SELECT COUNT(*) FROM telemetry_reward_source_v2 WHERE cut_id = OLD.cut_id) <> OLD.source_count THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical reward cut evidence is incomplete';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_cut_delete;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_cut_delete BEFORE DELETE ON telemetry_reward_cut_v2 FOR EACH ROW
BEGIN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical reward cut is immutable';
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_selection_insert;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_selection_insert BEFORE INSERT ON telemetry_reward_selection_v2 FOR EACH ROW
BEGIN
    IF NOT EXISTS (SELECT 1 FROM telemetry_reward_cut_v2 WHERE cut_id = NEW.cut_id AND sealed = 0 AND NEW.selection_index < selected_count) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical selection requires an unsealed cut';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_source_insert;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_source_insert BEFORE INSERT ON telemetry_reward_source_v2 FOR EACH ROW
BEGIN
    IF NOT EXISTS (SELECT 1 FROM telemetry_reward_cut_v2 WHERE cut_id = NEW.cut_id AND sealed = 0 AND NEW.source_index < source_count) OR NOT (NEW.payload_digest <=> UNHEX(SHA2(NEW.payload,256))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical source requires exact bytes and an unsealed cut';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_selection_update;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_selection_update BEFORE UPDATE ON telemetry_reward_selection_v2 FOR EACH ROW
BEGIN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical selection is immutable';
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_selection_delete;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_selection_delete BEFORE DELETE ON telemetry_reward_selection_v2 FOR EACH ROW
BEGIN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical selection is immutable';
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_source_update;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_source_update BEFORE UPDATE ON telemetry_reward_source_v2 FOR EACH ROW
BEGIN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical source is immutable';
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_source_delete;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_source_delete BEFORE DELETE ON telemetry_reward_source_v2 FOR EACH ROW
BEGIN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical source is immutable';
END$$
DELIMITER ;
