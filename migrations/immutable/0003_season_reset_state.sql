-- Immutable migration 0003: establish a durable, monotonic season identity and
-- an irreversible reset fence. The singleton begins in the active state for the
-- existing season. Runtime reset code advances the epoch and changes status to
-- resetting in one transaction before issuing any destructive season mutation.

CREATE TABLE IF NOT EXISTS season_reset_state (
    state_id TINYINT UNSIGNED NOT NULL,
    season_epoch BIGINT UNSIGNED NOT NULL,
    reset_status ENUM('active', 'resetting') NOT NULL DEFAULT 'active',
    reset_started_at DATETIME(6) NULL DEFAULT NULL,
    reset_completed_at DATETIME(6) NULL DEFAULT NULL,
    PRIMARY KEY (state_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

INSERT INTO season_reset_state (
    state_id,
    season_epoch,
    reset_status,
    reset_started_at,
    reset_completed_at
) VALUES (1, 1, 'active', NULL, NULL)
ON DUPLICATE KEY UPDATE state_id = VALUES(state_id);
