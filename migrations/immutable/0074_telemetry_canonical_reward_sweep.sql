-- Private fair reward-definition-2 discovery progress. Not a commit watermark.
-- Draft additive migration; register only after disposable qualification.

CREATE TABLE IF NOT EXISTS telemetry_reward_sweep_v2 (
    scan_id BINARY(16) NOT NULL,
    definition_version INT UNSIGNED NOT NULL,
    revision BIGINT UNSIGNED NOT NULL,
    next_bucket TINYINT UNSIGNED NOT NULL,
    provisional TINYINT UNSIGNED NOT NULL,
    PRIMARY KEY (scan_id),
    CONSTRAINT chk_reward2_sweep_definition CHECK (definition_version = 2 AND provisional = 1)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_reward_sweep_bucket_v2 (
    scan_id BINARY(16) NOT NULL,
    bucket TINYINT UNSIGNED NOT NULL,
    cursor_operation BINARY(16) NOT NULL,
    pass_upper BINARY(16) NULL,
    completed_passes BIGINT UNSIGNED NOT NULL,
    failures BIGINT UNSIGNED NOT NULL,
    last_revision BIGINT UNSIGNED NOT NULL,
    last_cut BINARY(32) NULL,
    PRIMARY KEY (scan_id,bucket),
    KEY idx_reward2_bucket_cut (last_cut),
    CONSTRAINT fk_reward2_bucket_scan FOREIGN KEY (scan_id) REFERENCES telemetry_reward_sweep_v2 (scan_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT fk_reward2_bucket_cut FOREIGN KEY (last_cut) REFERENCES telemetry_reward_cut_v2 (cut_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_reward2_bucket_range CHECK ((cursor_operation = UNHEX(REPEAT(CHAR(48),32)) AND pass_upper IS NULL) OR (pass_upper IS NOT NULL AND cursor_operation <= pass_upper AND ORD(SUBSTRING(cursor_operation,1,1)) = bucket AND ORD(SUBSTRING(pass_upper,1,1)) = bucket))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_reward_sweep_step_v2 (
    scan_id BINARY(16) NOT NULL,
    revision BIGINT UNSIGNED NOT NULL,
    bucket TINYINT UNSIGNED NOT NULL,
    status TINYINT UNSIGNED NOT NULL,
    attempted_utc_usec BIGINT UNSIGNED NOT NULL,
    cut_id BINARY(32) NULL,
    payload VARBINARY(8192) NOT NULL,
    payload_digest BINARY(32) NOT NULL,
    PRIMARY KEY (scan_id,revision),
    KEY idx_reward2_step_cut (cut_id),
    CONSTRAINT fk_reward2_step_scan FOREIGN KEY (scan_id) REFERENCES telemetry_reward_sweep_v2 (scan_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT fk_reward2_step_cut FOREIGN KEY (cut_id) REFERENCES telemetry_reward_cut_v2 (cut_id) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_reward2_step_revision CHECK (revision > 0 AND attempted_utc_usec > 0),
    CONSTRAINT chk_reward2_step_status CHECK ((status BETWEEN 1 AND 2 AND cut_id IS NOT NULL) OR (status BETWEEN 3 AND 6 AND cut_id IS NULL)),
    CONSTRAINT chk_reward2_step_payload CHECK (OCTET_LENGTH(payload) BETWEEN 1 AND 8192)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TRIGGER IF EXISTS telemetry_reward2_sweep_insert;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_sweep_insert BEFORE INSERT ON telemetry_reward_sweep_v2 FOR EACH ROW
BEGIN
    IF NEW.revision <> 0 OR NEW.next_bucket <> 0 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical sweep requires its initial position';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_sweep_update;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_sweep_update BEFORE UPDATE ON telemetry_reward_sweep_v2 FOR EACH ROW
BEGIN
    IF NOT (NEW.scan_id <=> OLD.scan_id) OR NOT (NEW.definition_version <=> OLD.definition_version) OR NEW.provisional <> 1 OR NEW.revision <> OLD.revision + 1 OR NEW.next_bucket <> MOD(OLD.next_bucket + 1,256) OR NOT EXISTS (SELECT 1 FROM telemetry_reward_sweep_step_v2 p JOIN telemetry_reward_sweep_bucket_v2 b ON b.scan_id = p.scan_id AND b.bucket = p.bucket WHERE p.scan_id = OLD.scan_id AND p.revision = NEW.revision AND p.bucket = OLD.next_bucket AND b.last_revision = NEW.revision) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical sweep requires an exact durable fair step';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_sweep_delete;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_sweep_delete BEFORE DELETE ON telemetry_reward_sweep_v2 FOR EACH ROW
BEGIN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical sweep history is retained';
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_bucket_insert;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_bucket_insert BEFORE INSERT ON telemetry_reward_sweep_bucket_v2 FOR EACH ROW
BEGIN
    IF NEW.cursor_operation <> UNHEX(REPEAT(CHAR(48),32)) OR NEW.pass_upper IS NOT NULL OR NEW.completed_passes <> 0 OR NEW.failures <> 0 OR NEW.last_revision <> 0 OR NEW.last_cut IS NOT NULL OR NOT EXISTS (SELECT 1 FROM telemetry_reward_sweep_v2 WHERE scan_id = NEW.scan_id AND revision = 0 AND next_bucket = 0) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical bucket requires an initial sweep';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_bucket_update;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_bucket_update BEFORE UPDATE ON telemetry_reward_sweep_bucket_v2 FOR EACH ROW
BEGIN
    DECLARE step_status INT;
    DECLARE step_cut BINARY(32);
    IF NOT (NEW.scan_id <=> OLD.scan_id) OR NEW.bucket <> OLD.bucket OR NEW.last_revision <= OLD.last_revision OR NOT EXISTS (SELECT 1 FROM telemetry_reward_sweep_v2 s JOIN telemetry_reward_sweep_step_v2 p ON p.scan_id = s.scan_id AND p.revision = s.revision + 1 WHERE s.scan_id = OLD.scan_id AND s.next_bucket = OLD.bucket AND p.bucket = OLD.bucket AND p.revision = NEW.last_revision) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical bucket requires an exact pending step';
    END IF;
    SELECT status,cut_id INTO step_status,step_cut FROM telemetry_reward_sweep_step_v2 WHERE scan_id = OLD.scan_id AND revision = NEW.last_revision;
    IF step_status <= 2 THEN
        IF NOT (NEW.last_cut <=> step_cut) OR NEW.failures <> OLD.failures OR NEW.completed_passes <> OLD.completed_passes + (NEW.cursor_operation = UNHEX(REPEAT(CHAR(48),32))) OR (NEW.cursor_operation <> UNHEX(REPEAT(CHAR(48),32)) AND NEW.cursor_operation <= OLD.cursor_operation) OR (OLD.pass_upper IS NOT NULL AND NEW.pass_upper IS NOT NULL AND NOT (OLD.pass_upper <=> NEW.pass_upper)) THEN
            SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical captured page cannot skip its range';
        END IF;
    ELSE
        IF NOT (NEW.cursor_operation <=> OLD.cursor_operation) OR NOT (NEW.pass_upper <=> OLD.pass_upper) OR NOT (NEW.last_cut <=> OLD.last_cut) OR NEW.completed_passes <> OLD.completed_passes OR NEW.failures <> OLD.failures + 1 THEN
            SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical refused page preserves its cursor';
        END IF;
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_bucket_delete;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_bucket_delete BEFORE DELETE ON telemetry_reward_sweep_bucket_v2 FOR EACH ROW
BEGIN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical bucket history is retained';
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_step_insert;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_step_insert BEFORE INSERT ON telemetry_reward_sweep_step_v2 FOR EACH ROW
BEGIN
    IF NOT (NEW.payload_digest <=> UNHEX(SHA2(NEW.payload,256))) OR NOT EXISTS (SELECT 1 FROM telemetry_reward_sweep_v2 WHERE scan_id = NEW.scan_id AND revision + 1 = NEW.revision AND next_bucket = NEW.bucket) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical step requires exact payload and current position';
    END IF;
    IF NEW.cut_id IS NOT NULL AND NOT EXISTS (SELECT 1 FROM telemetry_reward_cut_v2 c JOIN telemetry_reward_sweep_bucket_v2 b ON b.scan_id = NEW.scan_id AND b.bucket = NEW.bucket WHERE c.cut_id = NEW.cut_id AND c.sealed = 1 AND c.captured_utc_usec = NEW.attempted_utc_usec AND c.discovery_bucket = NEW.bucket AND c.discovery_cursor = b.cursor_operation AND (c.discovery_through <=> b.pass_upper)) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical step requires its sealed discovery cut';
    END IF;
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_step_update;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_step_update BEFORE UPDATE ON telemetry_reward_sweep_step_v2 FOR EACH ROW
BEGIN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical step is immutable';
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS telemetry_reward2_step_delete;
DELIMITER $$
CREATE TRIGGER telemetry_reward2_step_delete BEFORE DELETE ON telemetry_reward_sweep_step_v2 FOR EACH ROW
BEGIN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'canonical step is immutable';
END$$
DELIMITER ;
