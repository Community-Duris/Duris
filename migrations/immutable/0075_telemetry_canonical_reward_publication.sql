-- Independent canonical reward definition 2: exact retained generation publication.
-- Additive schema; no gameplay authority or earlier report changes.

CREATE TABLE IF NOT EXISTS telemetry_reward_generation_v2 (
    generation_id BINARY(16) NOT NULL,
    definition_version INT UNSIGNED NOT NULL,
    snapshot_utc_usec BIGINT UNSIGNED NOT NULL,
    cut_count SMALLINT UNSIGNED NOT NULL,
    binding_count SMALLINT UNSIGNED NOT NULL,
    event_count SMALLINT UNSIGNED NOT NULL,
    health_count SMALLINT UNSIGNED NOT NULL,
    reserved_bytes INT UNSIGNED NOT NULL,
    evidence_digest BINARY(32) NOT NULL,
    projection_digest BINARY(32) NOT NULL,
    health_evidence_digest BINARY(32) NOT NULL,
    health_projection_digest BINARY(32) NOT NULL,
    coverage_payload VARBINARY(8192) NOT NULL,
    coverage_digest BINARY(32) NOT NULL,
    phase TINYINT UNSIGNED NOT NULL,
    PRIMARY KEY (generation_id),
    CONSTRAINT chk_reward2_generation_version CHECK (definition_version = 2 AND snapshot_utc_usec > 0),
    CONSTRAINT chk_reward2_generation_counts CHECK (cut_count BETWEEN 1 AND 256 AND binding_count BETWEEN cut_count AND 16384 AND event_count <= 2000 AND reserved_bytes BETWEEN 4096 AND 33554432),
    CONSTRAINT chk_reward2_generation_phase CHECK (phase BETWEEN 0 AND 2),
    CONSTRAINT chk_reward2_generation_health CHECK (health_count IN (0,257)),
    CONSTRAINT chk_reward2_generation_payload CHECK (OCTET_LENGTH(coverage_payload) BETWEEN 1 AND 8192)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_reward_binding_v2 (
    generation_id BINARY(16) NOT NULL,
    binding_index SMALLINT UNSIGNED NOT NULL,
    binding_kind TINYINT UNSIGNED NOT NULL,
    cut_id BINARY(32) NOT NULL,
    source_index SMALLINT UNSIGNED NULL,
    payload_digest BINARY(32) NOT NULL,
    PRIMARY KEY (generation_id,binding_index),
    KEY idx_reward2_binding_cut (cut_id),
    KEY idx_reward2_binding_source (cut_id,source_index),
    CONSTRAINT fk_reward2_binding_generation FOREIGN KEY (generation_id) REFERENCES telemetry_reward_generation_v2 (generation_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT fk_reward2_binding_cut FOREIGN KEY (cut_id) REFERENCES telemetry_reward_cut_v2 (cut_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT fk_reward2_binding_source FOREIGN KEY (cut_id,source_index) REFERENCES telemetry_reward_source_v2 (cut_id,source_index) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_reward2_binding_index CHECK (binding_index < 16384),
    CONSTRAINT chk_reward2_binding_kind CHECK ((binding_kind = 1 AND source_index IS NULL AND payload_digest = cut_id) OR (binding_kind = 2 AND source_index IS NOT NULL AND source_index < 16384))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_reward_event_private_v2 (
    generation_id BINARY(16) NOT NULL,
    event_index SMALLINT UNSIGNED NOT NULL,
    payload VARBINARY(8192) NOT NULL,
    payload_digest BINARY(32) NOT NULL,
    PRIMARY KEY (generation_id,event_index),
    CONSTRAINT fk_reward2_private_event_generation FOREIGN KEY (generation_id) REFERENCES telemetry_reward_generation_v2 (generation_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_reward2_private_event_index CHECK (event_index < 2000),
    CONSTRAINT chk_reward2_private_event_payload CHECK (OCTET_LENGTH(payload) BETWEEN 1 AND 8192)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_reward_coverage_v2 (
    generation_id BINARY(16) NOT NULL,
    definition_version INT UNSIGNED NOT NULL,
    snapshot_utc_usec BIGINT UNSIGNED NOT NULL,
    binding_count SMALLINT UNSIGNED NOT NULL,
    event_count SMALLINT UNSIGNED NOT NULL,
    health_count SMALLINT UNSIGNED NOT NULL,
    evidence_digest BINARY(32) NOT NULL,
    projection_digest BINARY(32) NOT NULL,
    health_evidence_digest BINARY(32) NOT NULL,
    health_projection_digest BINARY(32) NOT NULL,
    coverage_payload VARBINARY(8192) NOT NULL,
    coverage_digest BINARY(32) NOT NULL,
    complete TINYINT UNSIGNED NOT NULL,
    PRIMARY KEY (generation_id),
    CONSTRAINT fk_reward2_coverage_generation FOREIGN KEY (generation_id) REFERENCES telemetry_reward_generation_v2 (generation_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_reward2_coverage_version CHECK (definition_version = 2 AND snapshot_utc_usec > 0 AND event_count <= 2000 AND binding_count BETWEEN 1 AND 16384),
    CONSTRAINT chk_reward2_coverage_complete CHECK (complete BETWEEN 0 AND 1),
    CONSTRAINT chk_reward2_coverage_health CHECK (health_count IN (0,257)),
    CONSTRAINT chk_reward2_coverage_payload CHECK (OCTET_LENGTH(coverage_payload) BETWEEN 1 AND 8192)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_reward_event_v2 (
    generation_id BINARY(16) NOT NULL,
    event_index SMALLINT UNSIGNED NOT NULL,
    payload VARBINARY(8192) NOT NULL,
    payload_digest BINARY(32) NOT NULL,
    PRIMARY KEY (generation_id,event_index),
    CONSTRAINT fk_reward2_public_event_coverage FOREIGN KEY (generation_id) REFERENCES telemetry_reward_coverage_v2 (generation_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_reward2_public_event_index CHECK (event_index < 2000),
    CONSTRAINT chk_reward2_public_event_payload CHECK (OCTET_LENGTH(payload) BETWEEN 1 AND 8192)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_reward_health_private_v2 (
    generation_id BINARY(16) NOT NULL,
    health_index SMALLINT UNSIGNED NOT NULL,
    payload VARBINARY(8192) NOT NULL,
    payload_digest BINARY(32) NOT NULL,
    projection_payload VARBINARY(8192) NOT NULL,
    projection_digest BINARY(32) NOT NULL,
    PRIMARY KEY (generation_id,health_index),
    CONSTRAINT fk_reward2_private_health_generation FOREIGN KEY (generation_id) REFERENCES telemetry_reward_generation_v2 (generation_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_reward2_private_health_index CHECK (health_index < 257),
    CONSTRAINT chk_reward2_private_health_payload CHECK (OCTET_LENGTH(payload) BETWEEN 1 AND 8192),
    CONSTRAINT chk_reward2_private_health_projection CHECK (OCTET_LENGTH(projection_payload) BETWEEN 1 AND 8192)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_reward_health_v2 (
    generation_id BINARY(16) NOT NULL,
    health_index SMALLINT UNSIGNED NOT NULL,
    payload VARBINARY(8192) NOT NULL,
    payload_digest BINARY(32) NOT NULL,
    PRIMARY KEY (generation_id,health_index),
    CONSTRAINT fk_reward2_public_health_generation FOREIGN KEY (generation_id) REFERENCES telemetry_reward_coverage_v2 (generation_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_reward2_public_health_index CHECK (health_index < 257),
    CONSTRAINT chk_reward2_public_health_payload CHECK (OCTET_LENGTH(payload) BETWEEN 1 AND 8192)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TRIGGER IF EXISTS telemetry_reward2_generation_insert;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_generation_insert BEFORE INSERT ON telemetry_reward_generation_v2 FOR EACH ROW
BEGIN
    IF NEW.phase <> 0 OR NOT (NEW.coverage_digest <=> UNHEX(SHA2(NEW.coverage_payload,256))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'reward generation requires exact initial metadata';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_generation_update;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_generation_update BEFORE UPDATE ON telemetry_reward_generation_v2 FOR EACH ROW
BEGIN
    IF NOT (NEW.generation_id <=> OLD.generation_id) OR NOT (NEW.definition_version <=> OLD.definition_version) OR NOT (NEW.snapshot_utc_usec <=> OLD.snapshot_utc_usec) OR NOT (NEW.cut_count <=> OLD.cut_count) OR NOT (NEW.binding_count <=> OLD.binding_count) OR NOT (NEW.event_count <=> OLD.event_count) OR NOT (NEW.health_count <=> OLD.health_count) OR NOT (NEW.reserved_bytes <=> OLD.reserved_bytes) OR NOT (NEW.evidence_digest <=> OLD.evidence_digest) OR NOT (NEW.projection_digest <=> OLD.projection_digest) OR NOT (NEW.health_evidence_digest <=> OLD.health_evidence_digest) OR NOT (NEW.health_projection_digest <=> OLD.health_projection_digest) OR NOT (NEW.coverage_payload <=> OLD.coverage_payload) OR NOT (NEW.coverage_digest <=> OLD.coverage_digest) OR NEW.phase <> OLD.phase + 1 OR (OLD.phase = 0 AND ((SELECT COUNT(*) FROM telemetry_reward_binding_v2 WHERE generation_id = OLD.generation_id) <> OLD.binding_count OR (SELECT COUNT(*) FROM telemetry_reward_event_private_v2 WHERE generation_id = OLD.generation_id) <> OLD.event_count OR (SELECT COUNT(*) FROM telemetry_reward_health_private_v2 WHERE generation_id = OLD.generation_id) <> OLD.health_count)) OR (OLD.phase = 1 AND (NOT EXISTS (SELECT 1 FROM telemetry_reward_coverage_v2 WHERE generation_id = OLD.generation_id AND complete = 0) OR (SELECT COUNT(*) FROM telemetry_reward_event_v2 WHERE generation_id = OLD.generation_id) <> OLD.event_count OR (SELECT COUNT(*) FROM telemetry_reward_health_v2 WHERE generation_id = OLD.generation_id) <> OLD.health_count)) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'reward generation permits exact sealing and publication only';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_generation_delete;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_generation_delete BEFORE DELETE ON telemetry_reward_generation_v2 FOR EACH ROW
BEGIN
    IF 1 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'reward generation is immutable';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_binding_insert;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_binding_insert BEFORE INSERT ON telemetry_reward_binding_v2 FOR EACH ROW
BEGIN
    IF NOT EXISTS (SELECT 1 FROM telemetry_reward_generation_v2 WHERE generation_id = NEW.generation_id AND phase = 0 AND NEW.binding_index < binding_count) OR NOT EXISTS (SELECT 1 FROM telemetry_reward_cut_v2 WHERE cut_id = NEW.cut_id AND sealed = 1) OR (NEW.binding_kind = 2 AND NOT EXISTS (SELECT 1 FROM telemetry_reward_source_v2 WHERE cut_id = NEW.cut_id AND source_index = NEW.source_index AND payload_digest = NEW.payload_digest)) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'reward binding requires exact sealed evidence';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_private_event_insert;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_private_event_insert BEFORE INSERT ON telemetry_reward_event_private_v2 FOR EACH ROW
BEGIN
    IF NOT EXISTS (SELECT 1 FROM telemetry_reward_generation_v2 WHERE generation_id = NEW.generation_id AND phase = 0 AND NEW.event_index < event_count) OR NOT (NEW.payload_digest <=> UNHEX(SHA2(NEW.payload,256))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'reward projection requires exact bytes and an unsealed generation';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_coverage_insert;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_coverage_insert BEFORE INSERT ON telemetry_reward_coverage_v2 FOR EACH ROW
BEGIN
    IF NEW.complete <> 0 OR NOT EXISTS (SELECT 1 FROM telemetry_reward_generation_v2 WHERE generation_id = NEW.generation_id AND phase = 1 AND generation_id <=> NEW.generation_id AND definition_version <=> NEW.definition_version AND snapshot_utc_usec <=> NEW.snapshot_utc_usec AND binding_count <=> NEW.binding_count AND event_count <=> NEW.event_count AND health_count <=> NEW.health_count AND evidence_digest <=> NEW.evidence_digest AND projection_digest <=> NEW.projection_digest AND health_evidence_digest <=> NEW.health_evidence_digest AND health_projection_digest <=> NEW.health_projection_digest AND coverage_payload <=> NEW.coverage_payload AND coverage_digest <=> NEW.coverage_digest) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'reward public coverage requires exact sealed metadata';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_coverage_update;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_coverage_update BEFORE UPDATE ON telemetry_reward_coverage_v2 FOR EACH ROW
BEGIN
    IF NOT (NEW.generation_id <=> OLD.generation_id) OR NOT (NEW.definition_version <=> OLD.definition_version) OR NOT (NEW.snapshot_utc_usec <=> OLD.snapshot_utc_usec) OR NOT (NEW.binding_count <=> OLD.binding_count) OR NOT (NEW.event_count <=> OLD.event_count) OR NOT (NEW.health_count <=> OLD.health_count) OR NOT (NEW.evidence_digest <=> OLD.evidence_digest) OR NOT (NEW.projection_digest <=> OLD.projection_digest) OR NOT (NEW.health_evidence_digest <=> OLD.health_evidence_digest) OR NOT (NEW.health_projection_digest <=> OLD.health_projection_digest) OR NOT (NEW.coverage_payload <=> OLD.coverage_payload) OR NOT (NEW.coverage_digest <=> OLD.coverage_digest) OR OLD.complete <> 0 OR NEW.complete <> 1 OR NOT EXISTS (SELECT 1 FROM telemetry_reward_generation_v2 WHERE generation_id = OLD.generation_id AND phase = 2) OR (SELECT COUNT(*) FROM telemetry_reward_event_v2 WHERE generation_id = OLD.generation_id) <> OLD.event_count OR (SELECT COUNT(*) FROM telemetry_reward_health_v2 WHERE generation_id = OLD.generation_id) <> OLD.health_count THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'reward public coverage permits exact completion only';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_public_event_insert;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_public_event_insert BEFORE INSERT ON telemetry_reward_event_v2 FOR EACH ROW
BEGIN
    IF NOT EXISTS (SELECT 1 FROM telemetry_reward_coverage_v2 c JOIN telemetry_reward_generation_v2 g ON g.generation_id = c.generation_id WHERE c.generation_id = NEW.generation_id AND c.complete = 0 AND g.phase = 1 AND NEW.event_index < c.event_count) OR NOT EXISTS (SELECT 1 FROM telemetry_reward_event_private_v2 WHERE generation_id = NEW.generation_id AND event_index = NEW.event_index AND payload = NEW.payload AND payload_digest = NEW.payload_digest) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'reward public event requires the exact sealed projection';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_binding_update;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_binding_update BEFORE UPDATE ON telemetry_reward_binding_v2 FOR EACH ROW
BEGIN
    IF 1 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'reward evidence is immutable';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_binding_delete;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_binding_delete BEFORE DELETE ON telemetry_reward_binding_v2 FOR EACH ROW
BEGIN
    IF 1 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'reward evidence is immutable';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_private_event_update;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_private_event_update BEFORE UPDATE ON telemetry_reward_event_private_v2 FOR EACH ROW
BEGIN
    IF 1 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'reward evidence is immutable';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_private_event_delete;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_private_event_delete BEFORE DELETE ON telemetry_reward_event_private_v2 FOR EACH ROW
BEGIN
    IF 1 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'reward evidence is immutable';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_coverage_delete;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_coverage_delete BEFORE DELETE ON telemetry_reward_coverage_v2 FOR EACH ROW
BEGIN
    IF 1 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'reward evidence is immutable';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_public_event_update;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_public_event_update BEFORE UPDATE ON telemetry_reward_event_v2 FOR EACH ROW
BEGIN
    IF 1 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'reward evidence is immutable';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_public_event_delete;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_public_event_delete BEFORE DELETE ON telemetry_reward_event_v2 FOR EACH ROW
BEGIN
    IF 1 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'reward evidence is immutable';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_private_health_insert;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_private_health_insert BEFORE INSERT ON telemetry_reward_health_private_v2 FOR EACH ROW
BEGIN
    IF NOT EXISTS (SELECT 1 FROM telemetry_reward_generation_v2 WHERE generation_id = NEW.generation_id AND phase = 0 AND NEW.health_index < health_count) OR NOT (NEW.payload_digest <=> UNHEX(SHA2(NEW.payload,256))) OR NOT (NEW.projection_digest <=> UNHEX(SHA2(NEW.projection_payload,256))) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'reward health requires exact bytes and an unsealed generation';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_public_health_insert;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_public_health_insert BEFORE INSERT ON telemetry_reward_health_v2 FOR EACH ROW
BEGIN
    IF NOT EXISTS (SELECT 1 FROM telemetry_reward_coverage_v2 c JOIN telemetry_reward_generation_v2 g ON g.generation_id = c.generation_id WHERE c.generation_id = NEW.generation_id AND c.complete = 0 AND g.phase = 1 AND NEW.health_index < c.health_count) OR NOT EXISTS (SELECT 1 FROM telemetry_reward_health_private_v2 WHERE generation_id = NEW.generation_id AND health_index = NEW.health_index AND projection_payload = NEW.payload AND projection_digest = NEW.payload_digest) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'reward health projection requires the exact sealed projection';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_private_health_update;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_private_health_update BEFORE UPDATE ON telemetry_reward_health_private_v2 FOR EACH ROW
BEGIN
    IF 1 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'reward health is immutable';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_private_health_delete;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_private_health_delete BEFORE DELETE ON telemetry_reward_health_private_v2 FOR EACH ROW
BEGIN
    IF 1 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'reward health is immutable';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_public_health_update;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_public_health_update BEFORE UPDATE ON telemetry_reward_health_v2 FOR EACH ROW
BEGIN
    IF 1 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'reward health is immutable';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_public_health_delete;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_public_health_delete BEFORE DELETE ON telemetry_reward_health_v2 FOR EACH ROW
BEGIN
    IF 1 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'reward health is immutable';
    END IF;
END$$
DELIMITER ;
