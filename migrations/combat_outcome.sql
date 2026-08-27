-- Phase 02 immutable PvP outcomes and revisioned frag authority.
-- Additive and re-runnable. Baseline population is a separate guarded action.

SET @frag_revision_missing = (SELECT COUNT(*) = 0 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='player_data' AND column_name='frag_revision');
SET @frag_revision_sql = IF(@frag_revision_missing,
    'ALTER TABLE player_data ADD COLUMN frag_revision BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER oldfrags',
    'SELECT 1 INTO @frag_revision_unchanged');
PREPARE frag_revision_stmt FROM @frag_revision_sql;
EXECUTE frag_revision_stmt;
DEALLOCATE PREPARE frag_revision_stmt;

CREATE TABLE IF NOT EXISTS combat_frag_baseline (
    pid INT UNSIGNED NOT NULL,
    opening_frags BIGINT NOT NULL,
    opening_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    captured_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (pid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS combat_outcome (
    operation_id BINARY(16) NOT NULL,
    pkill_event_id INT UNSIGNED NOT NULL,
    victim_pid INT UNSIGNED NOT NULL,
    room_vnum INT NOT NULL,
    participant_count SMALLINT UNSIGNED NOT NULL,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (operation_id),
    UNIQUE KEY uq_combat_pkill_event (pkill_event_id),
    KEY idx_combat_victim_created (victim_pid, created_at),
    CONSTRAINT combat_outcome_operation_fk FOREIGN KEY (operation_id)
        REFERENCES critical_operation_inbox (operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS combat_outcome_participant (
    operation_id BINARY(16) NOT NULL,
    participant_index SMALLINT UNSIGNED NOT NULL,
    pid INT UNSIGNED NOT NULL,
    role TINYINT UNSIGNED NOT NULL,
    flags TINYINT UNSIGNED NOT NULL,
    frag_delta BIGINT NOT NULL,
    epic_delta BIGINT NOT NULL,
    wallet_delta_copper BIGINT NOT NULL,
    frag_after BIGINT NOT NULL,
    frag_revision BIGINT UNSIGNED NOT NULL,
    epic_revision BIGINT UNSIGNED NOT NULL,
    wallet_revision BIGINT UNSIGNED NOT NULL,
    bank_revision BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (operation_id, participant_index),
    UNIQUE KEY uq_combat_participant (operation_id, pid),
    KEY idx_combat_participant_pid (pid, operation_id),
    CONSTRAINT combat_participant_operation_fk FOREIGN KEY (operation_id)
        REFERENCES combat_outcome (operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS combat_frag_ledger (
    operation_id BINARY(16) NOT NULL,
    participant_index SMALLINT UNSIGNED NOT NULL,
    pid INT UNSIGNED NOT NULL,
    delta BIGINT NOT NULL,
    frags_after BIGINT NOT NULL,
    frag_revision BIGINT UNSIGNED NOT NULL,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (operation_id, participant_index),
    UNIQUE KEY uq_combat_frag_pid_revision (pid, frag_revision),
    KEY idx_combat_frag_pid_created (pid, created_at),
    CONSTRAINT combat_frag_operation_fk FOREIGN KEY (operation_id)
        REFERENCES combat_outcome (operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
