-- Artifact control is revisioned configuration, not mutable object metadata.
-- The game can use the file catalog in flatfile mode; this schema is the
-- durable SQL authority used by the deployment that needs shared operators,
-- audit history, and idempotent publish requests.
CREATE TABLE IF NOT EXISTS artifact_config_revision (
  revision_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  revision_no BIGINT UNSIGNED NOT NULL,
  catalog_hash CHAR(16) NOT NULL,
  source_kind VARCHAR(16) NOT NULL,
  document LONGTEXT NOT NULL,
  published_by_pid INT NULL,
  published_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (revision_id),
  UNIQUE KEY uq_artifact_config_revision_no (revision_no),
  UNIQUE KEY uq_artifact_config_revision_hash (catalog_hash),
  CONSTRAINT chk_artifact_config_revision_source CHECK (source_kind IN ('file', 'sql', 'import')),
  CONSTRAINT chk_artifact_config_revision_json CHECK (JSON_VALID(document))
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS artifact_config_head (
  singleton_id TINYINT UNSIGNED NOT NULL,
  active_revision_no BIGINT UNSIGNED NOT NULL,
  active_catalog_hash CHAR(16) NOT NULL,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (singleton_id),
  CONSTRAINT chk_artifact_config_head_singleton CHECK (singleton_id = 1)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS artifact_config_draft (
  draft_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  base_revision_no BIGINT UNSIGNED NOT NULL,
  draft_hash CHAR(16) NOT NULL,
  document LONGTEXT NOT NULL,
  editor_pid INT NULL,
  state VARCHAR(16) NOT NULL DEFAULT 'open',
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (draft_id),
  KEY ix_artifact_config_draft_state (state, updated_at),
  CONSTRAINT chk_artifact_config_draft_state CHECK (state IN ('open', 'submitted', 'discarded')),
  CONSTRAINT chk_artifact_config_draft_json CHECK (JSON_VALID(document))
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS artifact_config_publish_audit (
  audit_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  revision_no BIGINT UNSIGNED NOT NULL,
  draft_id BIGINT UNSIGNED NULL,
  operator_pid INT NULL,
  action VARCHAR(16) NOT NULL,
  reason VARCHAR(255) NOT NULL DEFAULT '',
  catalog_hash CHAR(16) NOT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (audit_id),
  KEY ix_artifact_config_audit_revision (revision_no),
  CONSTRAINT chk_artifact_config_audit_action CHECK (action IN ('publish', 'import', 'rollback', 'discard'))
) ENGINE=InnoDB;

-- One row per artifact and holder class is the query-friendly projection of
-- the JSON catalog.  The JSON revision remains the immutable source of truth.
CREATE TABLE IF NOT EXISTS artifact_instance_policy (
  revision_no BIGINT UNSIGNED NOT NULL,
  vnum INT NOT NULL,
  holder_kind VARCHAR(16) NOT NULL,
  uniqueness_family VARCHAR(64) NOT NULL,
  variant_id VARCHAR(64) NOT NULL,
  enabled TINYINT(1) NOT NULL DEFAULT 1,
  load_chance SMALLINT UNSIGNED NOT NULL DEFAULT 100,
  load_limit SMALLINT UNSIGNED NOT NULL DEFAULT 1,
  initial_lifetime_seconds INT UNSIGNED NOT NULL DEFAULT 864000,
  max_remaining_lifetime_seconds INT UNSIGNED NOT NULL DEFAULT 864000,
  load_room INT NOT NULL DEFAULT 0,
  load_mob INT NOT NULL DEFAULT 0,
  load_slot SMALLINT NOT NULL DEFAULT -1,
  PRIMARY KEY (revision_no, vnum, holder_kind),
  KEY ix_artifact_instance_policy_vnum (vnum, holder_kind),
  CONSTRAINT chk_artifact_instance_policy_holder CHECK (holder_kind IN ('player', 'wild_npc', 'controlled_npc')),
  CONSTRAINT chk_artifact_instance_policy_chance CHECK (load_chance BETWEEN 0 AND 100),
  CONSTRAINT chk_artifact_instance_policy_limit CHECK (load_limit > 0),
  CONSTRAINT chk_artifact_instance_policy_lifetime CHECK (initial_lifetime_seconds <= 31536000 AND max_remaining_lifetime_seconds <= 31536000)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS artifact_power_policy (
  revision_no BIGINT UNSIGNED NOT NULL,
  vnum INT NOT NULL,
  power_id VARCHAR(64) NOT NULL,
  enabled TINYINT(1) NOT NULL DEFAULT 1,
  chance_numerator INT UNSIGNED NOT NULL DEFAULT 1,
  chance_denominator INT UNSIGNED NOT NULL DEFAULT 1,
  cooldown_seconds INT UNSIGNED NOT NULL DEFAULT 0,
  windup_pulses SMALLINT UNSIGNED NOT NULL DEFAULT 0,
  mana_cost_milliunits INT UNSIGNED NOT NULL DEFAULT 0,
  power_level TINYINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (revision_no, vnum, power_id),
  CONSTRAINT chk_artifact_power_policy_chance CHECK (chance_denominator > 0 AND chance_numerator <= chance_denominator),
  CONSTRAINT chk_artifact_power_policy_limits CHECK (cooldown_seconds <= 604800 AND windup_pulses <= 600 AND power_level <= 60)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS artifact_control_request (
  request_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  idempotency_key CHAR(36) NOT NULL,
  command VARCHAR(32) NOT NULL,
  vnum INT NULL,
  holder_kind VARCHAR(16) NULL,
  payload LONGTEXT NOT NULL,
  requested_by_pid INT NULL,
  state VARCHAR(16) NOT NULL DEFAULT 'pending',
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (request_id),
  UNIQUE KEY uq_artifact_control_request_key (idempotency_key),
  KEY ix_artifact_control_request_state (state, created_at),
  CONSTRAINT chk_artifact_control_request_payload CHECK (JSON_VALID(payload)),
  CONSTRAINT chk_artifact_control_request_state CHECK (state IN ('pending', 'applied', 'rejected', 'expired')),
  CONSTRAINT chk_artifact_control_request_command CHECK (command IN ('validate', 'publish', 'rollback', 'discard', 'set_mode', 'set_enabled')),
  CONSTRAINT chk_artifact_control_request_holder CHECK (holder_kind IS NULL OR holder_kind IN ('player', 'wild_npc', 'controlled_npc'))
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS artifact_control_result (
  request_id BIGINT UNSIGNED NOT NULL,
  result_code VARCHAR(32) NOT NULL,
  message VARCHAR(255) NOT NULL DEFAULT '',
  active_revision_no BIGINT UNSIGNED NULL,
  active_catalog_hash CHAR(16) NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (request_id),
  CONSTRAINT chk_artifact_control_result_code CHECK (result_code IN ('ok', 'invalid', 'conflict', 'forbidden', 'not_found', 'unavailable'))
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS artifact_control_lease (
  singleton_id TINYINT UNSIGNED NOT NULL,
  holder_token CHAR(36) NULL,
  holder_pid INT NULL,
  expires_at TIMESTAMP NULL,
  PRIMARY KEY (singleton_id),
  CONSTRAINT chk_artifact_control_lease_singleton CHECK (singleton_id = 1)
) ENGINE=InnoDB;

-- SQL operators receive an idempotent request inbox instead of UPDATE access
-- to the head, revision, or gameplay-state tables. The runtime worker owns the
-- transition from pending to applied/rejected and writes the result receipt.
DELIMITER //
DROP PROCEDURE IF EXISTS artifact_control_request_submit//
CREATE PROCEDURE artifact_control_request_submit(
  IN p_idempotency_key CHAR(36),
  IN p_command VARCHAR(32),
  IN p_vnum INT,
  IN p_holder_kind VARCHAR(16),
  IN p_payload LONGTEXT,
  IN p_requested_by_pid INT
)
SQL SECURITY INVOKER
BEGIN
  DECLARE existing_request BIGINT UNSIGNED DEFAULT NULL;
  IF p_idempotency_key IS NULL OR p_idempotency_key = '' OR NOT JSON_VALID(p_payload) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'invalid artifact control request';
  END IF;
  SELECT request_id INTO existing_request
    FROM artifact_control_request
   WHERE idempotency_key = p_idempotency_key
   LIMIT 1;
  IF existing_request IS NULL THEN
    INSERT INTO artifact_control_request(
      idempotency_key, command, vnum, holder_kind, payload, requested_by_pid)
    VALUES(p_idempotency_key, p_command, p_vnum, p_holder_kind, p_payload, p_requested_by_pid);
    SET existing_request = LAST_INSERT_ID();
  END IF;
  SELECT existing_request AS request_id;
END//

DROP PROCEDURE IF EXISTS artifact_control_request_status//
CREATE PROCEDURE artifact_control_request_status(IN p_request_id BIGINT UNSIGNED)
SQL SECURITY INVOKER
BEGIN
  SELECT r.request_id, r.idempotency_key, r.command, r.vnum, r.holder_kind,
         r.state, r.created_at, o.result_code, o.message,
         o.active_revision_no, o.active_catalog_hash
    FROM artifact_control_request r
    LEFT JOIN artifact_control_result o ON o.request_id = r.request_id
   WHERE r.request_id = p_request_id;
END//
DELIMITER ;

CREATE OR REPLACE VIEW artifact_control_active AS
SELECT p.revision_no, p.vnum, p.holder_kind, p.uniqueness_family, p.variant_id,
       p.enabled, p.load_chance, p.load_limit, p.initial_lifetime_seconds,
       p.max_remaining_lifetime_seconds, p.load_room, p.load_mob, p.load_slot,
       h.active_catalog_hash
  FROM artifact_instance_policy p
  JOIN artifact_config_head h ON h.singleton_id = 1
 WHERE p.revision_no = h.active_revision_no;

INSERT INTO artifact_config_head(singleton_id, active_revision_no, active_catalog_hash)
VALUES (1, 0, '')
ON DUPLICATE KEY UPDATE singleton_id = VALUES(singleton_id);
