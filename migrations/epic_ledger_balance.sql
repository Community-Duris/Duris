-- Phase 02 epic opening balance, immutable operation ledger, and domain revision.
-- Additive and re-runnable. Baseline population is a separate guarded action.

SET @epic_revision_missing = (SELECT COUNT(*) = 0 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='player_data' AND column_name='epic_revision');
SET @epic_revision_sql = IF(@epic_revision_missing,
    'ALTER TABLE player_data ADD COLUMN epic_revision BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER epics',
    'SELECT 1 INTO @epic_revision_unchanged');
PREPARE epic_revision_stmt FROM @epic_revision_sql;
EXECUTE epic_revision_stmt;
DEALLOCATE PREPARE epic_revision_stmt;

CREATE TABLE IF NOT EXISTS epic_balance_baseline (
    pid INT UNSIGNED NOT NULL,
    opening_balance BIGINT NOT NULL,
    opening_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    captured_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (pid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS epic_ledger (
    operation_id BINARY(16) NOT NULL,
    pid INT UNSIGNED NOT NULL,
    delta BIGINT NOT NULL,
    balance_after BIGINT NOT NULL,
    epic_revision BIGINT UNSIGNED NOT NULL,
    reason_type SMALLINT UNSIGNED NOT NULL,
    reason_id BIGINT NOT NULL DEFAULT 0,
    source_site SMALLINT UNSIGNED NOT NULL,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (operation_id),
    UNIQUE KEY uq_epic_ledger_pid_revision (pid, epic_revision),
    KEY idx_epic_ledger_pid_created (pid, created_at),
    KEY idx_epic_ledger_reason_created (reason_type, created_at),
    CONSTRAINT epic_ledger_operation_fk FOREIGN KEY (operation_id)
        REFERENCES critical_operation_inbox (operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
