-- Durable quarantine for record-specific telemetry storage failures. The
-- original fixed-size record is retained for controlled replay; ordinary
-- health diagnostics expose only its producer/key/kind, failure class and
-- numeric error. This table is additive and CREATE IF NOT EXISTS is rerunnable.
CREATE TABLE IF NOT EXISTS telemetry_quarantine (
  boot_id BIGINT UNSIGNED NOT NULL,
  process_id BIGINT UNSIGNED NOT NULL,
  record_seq BIGINT UNSIGNED NOT NULL,
  schema_version SMALLINT UNSIGNED NOT NULL,
  record_kind TINYINT UNSIGNED NOT NULL,
  failure_class TINYINT UNSIGNED NOT NULL,
  sql_error_code INT UNSIGNED NOT NULL,
  gap_reason TINYINT UNSIGNED NOT NULL,
  quality_flags INT UNSIGNED NOT NULL,
  payload_sha256 BINARY(32) NOT NULL,
  record_payload BLOB NOT NULL,
  quarantined_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
  recovery_state TINYINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (boot_id,process_id,record_seq),
  KEY idx_telemetry_quarantine_recovery (recovery_state,quarantined_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
