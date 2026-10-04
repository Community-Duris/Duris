-- Independent incident review schema v2 for record families 1..9.
-- Sealed v1 inventories and published report definitions 1/2 retain their meaning.
-- Balance definitions 3+ reuse the existing scoped publication snapshot tables.
CREATE TABLE IF NOT EXISTS telemetry_incident_registry_v2 (
    environment_id BIGINT UNSIGNED NOT NULL,
    season_id BIGINT UNSIGNED NOT NULL,
    registry_version BIGINT UNSIGNED NOT NULL,
    previous_registry_version BIGINT UNSIGNED NOT NULL,
    reviewed_from_utc_usec BIGINT NULL,
    reviewed_through_utc_usec BIGINT NULL,
    reviewer_token BINARY(32) NOT NULL,
    review_evidence_digest BINARY(32) NOT NULL,
    packet_digest BINARY(32) NOT NULL,
    incident_count SMALLINT UNSIGNED NOT NULL,
    PRIMARY KEY (environment_id,season_id,registry_version),
    CONSTRAINT chk_incident_registry_version_v2 CHECK (environment_id <> 0 AND season_id <> 0 AND registry_version <> 0 AND previous_registry_version < registry_version AND registry_version - previous_registry_version = 1),
    CONSTRAINT chk_incident_registry_count_v2 CHECK (incident_count <= 64),
    CONSTRAINT chk_incident_registry_utc_v2 CHECK ((reviewed_from_utc_usec IS NULL OR CAST(reviewed_from_utc_usec AS UNSIGNED) <> 9223372036854775808) AND (reviewed_through_utc_usec IS NULL OR CAST(reviewed_through_utc_usec AS UNSIGNED) <> 9223372036854775808)),
    CONSTRAINT chk_incident_registry_time_v2 CHECK (reviewed_from_utc_usec IS NULL OR reviewed_through_utc_usec IS NULL OR reviewed_from_utc_usec <= reviewed_through_utc_usec)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_incident_v2 (
    environment_id BIGINT UNSIGNED NOT NULL,
    season_id BIGINT UNSIGNED NOT NULL,
    registry_version BIGINT UNSIGNED NOT NULL,
    incident_id BIGINT UNSIGNED NOT NULL,
    producer_boot_id BIGINT UNSIGNED NULL,
    producer_process_id BIGINT UNSIGNED NULL,
    start_utc_usec BIGINT NULL,
    end_utc_usec BIGINT NULL,
    record_kind_mask BIGINT UNSIGNED NOT NULL,
    first_record_seq BIGINT UNSIGNED NULL,
    last_record_seq BIGINT UNSIGNED NULL,
    fix_reference_digest BINARY(32) NULL,
    verified_boot_id BIGINT UNSIGNED NULL,
    verified_process_id BIGINT UNSIGNED NULL,
    verified_record_seq BIGINT UNSIGNED NULL,
    verified_record_kind TINYINT UNSIGNED NULL,
    verified_occurrence_utc_usec BIGINT NULL,
    backlog_disposition TINYINT UNSIGNED NOT NULL,
    observation_provenance TINYINT UNSIGNED NOT NULL,
    evidence_digest BINARY(32) NOT NULL,
    status TINYINT UNSIGNED NOT NULL,
    PRIMARY KEY (environment_id,season_id,registry_version,incident_id),
    CONSTRAINT fk_incident_registry_v2 FOREIGN KEY (environment_id,season_id,registry_version) REFERENCES telemetry_incident_registry_v2 (environment_id,season_id,registry_version) ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT chk_incident_identity_v2 CHECK (incident_id <> 0 AND ((producer_boot_id IS NULL AND producer_process_id IS NULL AND first_record_seq IS NULL AND last_record_seq IS NULL) OR (producer_boot_id IS NOT NULL AND producer_process_id IS NOT NULL AND producer_boot_id > 0 AND producer_process_id > 0))),
    CONSTRAINT chk_incident_utc_v2 CHECK ((start_utc_usec IS NULL OR CAST(start_utc_usec AS UNSIGNED) <> 9223372036854775808) AND (end_utc_usec IS NULL OR CAST(end_utc_usec AS UNSIGNED) <> 9223372036854775808) AND (verified_occurrence_utc_usec IS NULL OR CAST(verified_occurrence_utc_usec AS UNSIGNED) <> 9223372036854775808)),
    CONSTRAINT chk_incident_time_v2 CHECK (start_utc_usec IS NULL OR end_utc_usec IS NULL OR start_utc_usec <= end_utc_usec),
    CONSTRAINT chk_incident_sequence_v2 CHECK ((first_record_seq IS NULL OR first_record_seq > 0) AND (last_record_seq IS NULL OR last_record_seq > 0) AND (first_record_seq IS NULL OR last_record_seq IS NULL OR first_record_seq <= last_record_seq)),
    CONSTRAINT chk_incident_family_v2 CHECK (record_kind_mask BETWEEN 2 AND 1022 AND (record_kind_mask & 1) = 0),
    CONSTRAINT chk_incident_verification_v2 CHECK ((verified_boot_id IS NULL AND verified_process_id IS NULL AND verified_record_seq IS NULL AND verified_record_kind IS NULL AND verified_occurrence_utc_usec IS NULL) OR (fix_reference_digest IS NOT NULL AND verified_boot_id IS NOT NULL AND verified_process_id IS NOT NULL AND verified_record_seq IS NOT NULL AND verified_record_kind IS NOT NULL AND verified_boot_id > 0 AND verified_process_id > 0 AND verified_record_seq > 0 AND verified_record_kind BETWEEN 1 AND 9 AND (record_kind_mask & (1 << verified_record_kind)) <> 0)),
    CONSTRAINT chk_incident_disposition_v2 CHECK (backlog_disposition BETWEEN 0 AND 5 AND observation_provenance BETWEEN 1 AND 3 AND status BETWEEN 1 AND 2)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
