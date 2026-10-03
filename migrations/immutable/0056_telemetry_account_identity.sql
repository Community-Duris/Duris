-- Private current-account binding and retained opaque lifetime/scoped tokens.
-- Allocation is account-load preparation, not historical authentication evidence.
CREATE TABLE IF NOT EXISTS telemetry_account_lifetime (
    lifetime_id BIGINT UNSIGNED NOT NULL,
    account_name VARCHAR(50) NULL,
    PRIMARY KEY (lifetime_id),
    UNIQUE KEY uq_telemetry_lifetime_account (account_name),
    CONSTRAINT chk_telemetry_lifetime_nonzero CHECK (lifetime_id <> 0),
    CONSTRAINT fk_telemetry_lifetime_account FOREIGN KEY (account_name) REFERENCES accounts (account_name) ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS telemetry_account_token (
    environment_id BIGINT UNSIGNED NOT NULL,
    season_id BIGINT UNSIGNED NOT NULL,
    account_token BIGINT UNSIGNED NOT NULL,
    lifetime_id BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (environment_id,season_id,account_token),
    UNIQUE KEY uq_telemetry_token_lifetime (environment_id,season_id,lifetime_id),
    KEY idx_telemetry_token_lifetime (lifetime_id),
    CONSTRAINT chk_telemetry_token_nonzero CHECK (environment_id <> 0 AND season_id <> 0 AND account_token <> 0 AND lifetime_id <> 0),
    CONSTRAINT fk_telemetry_token_lifetime FOREIGN KEY (lifetime_id) REFERENCES telemetry_account_lifetime (lifetime_id) ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
