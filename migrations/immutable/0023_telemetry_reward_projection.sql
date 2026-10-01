-- Project committed reward authorities and bounded reconciliation state into
-- analytics.  The source ledgers/outcomes remain authoritative; this
-- migration adds no gameplay writes, outbox delivery state, or source locks.
-- Additive and re-runnable.

SET @reward_currency_scan_index_missing = (SELECT COUNT(*) = 0
    FROM information_schema.statistics WHERE table_schema=DATABASE()
    AND table_name='currency_ledger' AND index_name='idx_currency_created_operation');
SET @reward_currency_scan_index_sql = IF(@reward_currency_scan_index_missing,
    'ALTER TABLE currency_ledger ADD KEY idx_currency_created_operation (created_at, operation_id)',
    'SELECT 1 INTO @reward_currency_scan_index_unchanged');
PREPARE reward_currency_scan_index_stmt FROM @reward_currency_scan_index_sql;
EXECUTE reward_currency_scan_index_stmt;
DEALLOCATE PREPARE reward_currency_scan_index_stmt;

SET @reward_epic_scan_index_missing = (SELECT COUNT(*) = 0
    FROM information_schema.statistics WHERE table_schema=DATABASE()
    AND table_name='epic_ledger' AND index_name='idx_epic_created_operation');
SET @reward_epic_scan_index_sql = IF(@reward_epic_scan_index_missing,
    'ALTER TABLE epic_ledger ADD KEY idx_epic_created_operation (created_at, operation_id)',
    'SELECT 1 INTO @reward_epic_scan_index_unchanged');
PREPARE reward_epic_scan_index_stmt FROM @reward_epic_scan_index_sql;
EXECUTE reward_epic_scan_index_stmt;
DEALLOCATE PREPARE reward_epic_scan_index_stmt;

SET @reward_frag_scan_index_missing = (SELECT COUNT(*) = 0
    FROM information_schema.statistics WHERE table_schema=DATABASE()
    AND table_name='combat_frag_ledger' AND index_name='idx_combat_frag_created_operation');
SET @reward_frag_scan_index_sql = IF(@reward_frag_scan_index_missing,
    'ALTER TABLE combat_frag_ledger ADD KEY idx_combat_frag_created_operation (created_at, operation_id, participant_index, pid)',
    'SELECT 1 INTO @reward_frag_scan_index_unchanged');
PREPARE reward_frag_scan_index_stmt FROM @reward_frag_scan_index_sql;
EXECUTE reward_frag_scan_index_stmt;
DEALLOCATE PREPARE reward_frag_scan_index_stmt;

SET @reward_combat_scan_index_missing = (SELECT COUNT(*) = 0
    FROM information_schema.statistics WHERE table_schema=DATABASE()
    AND table_name='combat_outcome' AND index_name='idx_combat_outcome_created_operation');
SET @reward_combat_scan_index_sql = IF(@reward_combat_scan_index_missing,
    'ALTER TABLE combat_outcome ADD KEY idx_combat_outcome_created_operation (created_at, operation_id)',
    'SELECT 1 INTO @reward_combat_scan_index_unchanged');
PREPARE reward_combat_scan_index_stmt FROM @reward_combat_scan_index_sql;
EXECUTE reward_combat_scan_index_stmt;
DEALLOCATE PREPARE reward_combat_scan_index_stmt;

SET @reward_boon_scan_index_missing = (SELECT COUNT(*) = 0
    FROM information_schema.statistics WHERE table_schema=DATABASE()
    AND table_name='boon_reward_outcome' AND index_name='idx_boon_reward_created_operation');
SET @reward_boon_scan_index_sql = IF(@reward_boon_scan_index_missing,
    'ALTER TABLE boon_reward_outcome ADD KEY idx_boon_reward_created_operation (created_at, operation_id)',
    'SELECT 1 INTO @reward_boon_scan_index_unchanged');
PREPARE reward_boon_scan_index_stmt FROM @reward_boon_scan_index_sql;
EXECUTE reward_boon_scan_index_stmt;
DEALLOCATE PREPARE reward_boon_scan_index_stmt;

SET @reward_zone_scan_index_missing = (SELECT COUNT(*) = 0
    FROM information_schema.statistics WHERE table_schema=DATABASE()
    AND table_name='zone_touch_outcome' AND index_name='idx_zone_touch_outcome_created_operation');
SET @reward_zone_scan_index_sql = IF(@reward_zone_scan_index_missing,
    'ALTER TABLE zone_touch_outcome ADD KEY idx_zone_touch_outcome_created_operation (created_at, operation_id)',
    'SELECT 1 INTO @reward_zone_scan_index_unchanged');
PREPARE reward_zone_scan_index_stmt FROM @reward_zone_scan_index_sql;
EXECUTE reward_zone_scan_index_stmt;
DEALLOCATE PREPARE reward_zone_scan_index_stmt;

CREATE TABLE IF NOT EXISTS telemetry_reward_projection (
    source_kind TINYINT UNSIGNED NOT NULL,
    operation_id BINARY(16) NOT NULL,
    entry_index SMALLINT UNSIGNED NOT NULL,
    participant_pid INT UNSIGNED NOT NULL,
    source_table VARCHAR(64) NOT NULL,
    source_created_at TIMESTAMP(6) NOT NULL,
    authority_kind TINYINT UNSIGNED NOT NULL,
    reward_kind TINYINT UNSIGNED NOT NULL DEFAULT 0,
    gross_amount BIGINT NULL,
    net_amount BIGINT NULL,
    transfer_amount BIGINT NULL,
    parent_operation_id BINARY(16) NULL,
    economic_operation_id BINARY(16) NOT NULL,
    reason_type SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    reason_id BIGINT NOT NULL DEFAULT 0,
    source_site SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    is_transfer TINYINT UNSIGNED NOT NULL DEFAULT 0,
    is_creation TINYINT UNSIGNED NOT NULL DEFAULT 0,
    context_complete TINYINT UNSIGNED NOT NULL DEFAULT 1,
    status TINYINT UNSIGNED NOT NULL,
    quality_flags INT UNSIGNED NOT NULL DEFAULT 0,
    source_payload_digest BINARY(32) NOT NULL,
    cycle_id BIGINT UNSIGNED NOT NULL,
    projected_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (source_kind, operation_id, entry_index, participant_pid),
    KEY idx_reward_projection_scan
        (source_kind, source_created_at, operation_id, entry_index, participant_pid),
    KEY idx_reward_projection_participant
        (participant_pid, source_created_at, source_kind),
    KEY idx_reward_projection_economic
        (economic_operation_id, reward_kind, source_kind),
    KEY idx_reward_projection_status
        (source_kind, status, source_created_at),
    CONSTRAINT chk_reward_projection_source_kind CHECK (source_kind BETWEEN 1 AND 10),
    CONSTRAINT chk_reward_projection_authority CHECK (authority_kind BETWEEN 0 AND 2),
    CONSTRAINT chk_reward_projection_reward CHECK (reward_kind BETWEEN 0 AND 3),
    CONSTRAINT chk_reward_projection_status CHECK (status BETWEEN 1 AND 5),
    CONSTRAINT chk_reward_projection_transfer CHECK (is_transfer BETWEEN 0 AND 1),
    CONSTRAINT chk_reward_projection_creation CHECK (is_creation BETWEEN 0 AND 1),
    CONSTRAINT chk_reward_projection_context_complete CHECK (context_complete BETWEEN 0 AND 1),
    CONSTRAINT chk_reward_projection_authoritative_amount CHECK
        ((authority_kind <> 1) OR
         (reward_kind <> 0 AND gross_amount IS NOT NULL AND net_amount IS NOT NULL)),
    CONSTRAINT chk_reward_projection_context_amount CHECK
        ((authority_kind <> 2) OR
         (gross_amount IS NULL AND net_amount IS NULL AND transfer_amount IS NULL)),
    CONSTRAINT chk_reward_projection_transfer_creation CHECK
        (is_transfer = 0 OR is_creation = 0),
    CONSTRAINT chk_reward_projection_transfer_amount CHECK
        (transfer_amount IS NULL OR (is_transfer = 1 AND transfer_amount >= 0))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_reward_projection_state (
    source_kind TINYINT UNSIGNED NOT NULL,
    cycle_id BIGINT UNSIGNED NOT NULL DEFAULT 0,
    fast_cursor_created_at TIMESTAMP(6) NULL,
    fast_cursor_operation_id BINARY(16) NOT NULL,
    fast_cursor_entry_index SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    fast_cursor_participant_pid INT UNSIGNED NOT NULL DEFAULT 0,
    reconcile_cursor_created_at TIMESTAMP(6) NULL,
    reconcile_cursor_operation_id BINARY(16) NOT NULL,
    reconcile_cursor_entry_index SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    reconcile_cursor_participant_pid INT UNSIGNED NOT NULL DEFAULT 0,
    cycle_high_water TIMESTAMP(6) NULL,
    retention_floor TIMESTAMP(6) NULL,
    acknowledged_through TIMESTAMP(6) NULL,
    backlog_rows BIGINT UNSIGNED NOT NULL DEFAULT 0,
    quality_flags INT UNSIGNED NOT NULL DEFAULT 0,
    provisional TINYINT UNSIGNED NOT NULL DEFAULT 1,
    last_fast_started_at TIMESTAMP(6) NULL,
    last_fast_completed_at TIMESTAMP(6) NULL,
    last_reconcile_started_at TIMESTAMP(6) NULL,
    last_reconcile_completed_at TIMESTAMP(6) NULL,
    updated_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)
        ON UPDATE CURRENT_TIMESTAMP(6),
    PRIMARY KEY (source_kind),
    KEY idx_reward_projection_state_reconcile
        (provisional, acknowledged_through, retention_floor),
    CONSTRAINT chk_reward_projection_state_source_kind CHECK (source_kind BETWEEN 1 AND 10),
    CONSTRAINT chk_reward_projection_state_provisional CHECK (provisional BETWEEN 0 AND 1)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
