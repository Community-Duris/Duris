-- Phase 02 wallet/account-bank baselines, immutable vector ledger, and revisions.
-- Additive and re-runnable. Baseline population is a separate guarded action.

SET @wallet_revision_missing = (SELECT COUNT(*) = 0 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='player_data' AND column_name='wallet_revision');
SET @wallet_revision_sql = IF(@wallet_revision_missing,
    'ALTER TABLE player_data ADD COLUMN wallet_revision BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER platinum',
    'SELECT 1 INTO @wallet_revision_unchanged');
PREPARE wallet_revision_stmt FROM @wallet_revision_sql;
EXECUTE wallet_revision_stmt;
DEALLOCATE PREPARE wallet_revision_stmt;

SET @bank_revision_missing = (SELECT COUNT(*) = 0 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='account_banks' AND column_name='bank_revision');
SET @bank_revision_sql = IF(@bank_revision_missing,
    'ALTER TABLE account_banks ADD COLUMN bank_revision BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER bank_platinum',
    'SELECT 1 INTO @bank_revision_unchanged');
PREPARE bank_revision_stmt FROM @bank_revision_sql;
EXECUTE bank_revision_stmt;
DEALLOCATE PREPARE bank_revision_stmt;

CREATE TABLE IF NOT EXISTS currency_wallet_baseline (
    pid INT UNSIGNED NOT NULL,
    opening_copper BIGINT NOT NULL,
    opening_silver BIGINT NOT NULL,
    opening_gold BIGINT NOT NULL,
    opening_platinum BIGINT NOT NULL,
    opening_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    captured_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (pid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS currency_bank_baseline (
    bank_id INT UNSIGNED NOT NULL,
    opening_copper BIGINT UNSIGNED NOT NULL,
    opening_silver BIGINT UNSIGNED NOT NULL,
    opening_gold BIGINT UNSIGNED NOT NULL,
    opening_platinum BIGINT UNSIGNED NOT NULL,
    opening_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    captured_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (bank_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS currency_ledger (
    operation_id BINARY(16) NOT NULL,
    pid INT UNSIGNED NOT NULL,
    bank_id INT UNSIGNED NOT NULL,
    wallet_delta_copper BIGINT NOT NULL,
    wallet_delta_silver BIGINT NOT NULL,
    wallet_delta_gold BIGINT NOT NULL,
    wallet_delta_platinum BIGINT NOT NULL,
    bank_delta_copper BIGINT NOT NULL,
    bank_delta_silver BIGINT NOT NULL,
    bank_delta_gold BIGINT NOT NULL,
    bank_delta_platinum BIGINT NOT NULL,
    wallet_after_copper BIGINT NOT NULL,
    wallet_after_silver BIGINT NOT NULL,
    wallet_after_gold BIGINT NOT NULL,
    wallet_after_platinum BIGINT NOT NULL,
    bank_after_copper BIGINT UNSIGNED NOT NULL,
    bank_after_silver BIGINT UNSIGNED NOT NULL,
    bank_after_gold BIGINT UNSIGNED NOT NULL,
    bank_after_platinum BIGINT UNSIGNED NOT NULL,
    wallet_revision BIGINT UNSIGNED NOT NULL,
    bank_revision BIGINT UNSIGNED NOT NULL,
    reason_type SMALLINT UNSIGNED NOT NULL,
    reason_id BIGINT NOT NULL DEFAULT 0,
    source_site SMALLINT UNSIGNED NOT NULL,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (operation_id),
    UNIQUE KEY uq_currency_wallet_revision (pid, wallet_revision),
    UNIQUE KEY uq_currency_bank_revision (bank_id, bank_revision),
    KEY idx_currency_pid_created (pid, created_at),
    KEY idx_currency_bank_created (bank_id, created_at),
    KEY idx_currency_reason_created (reason_type, created_at),
    CONSTRAINT currency_ledger_operation_fk FOREIGN KEY (operation_id)
        REFERENCES critical_operation_inbox (operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
