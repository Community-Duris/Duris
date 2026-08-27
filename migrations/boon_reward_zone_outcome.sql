-- Phase 02 idempotent boon trigger and zone-touch outcomes.
-- Additive and re-runnable; no legacy rows are rewritten.

CREATE TABLE IF NOT EXISTS boon_reward_outcome (
    operation_id BINARY(16) NOT NULL,
    pid INT UNSIGNED NOT NULL,
    option TINYINT UNSIGNED NOT NULL,
    event_value DOUBLE NOT NULL,
    entry_count SMALLINT UNSIGNED NOT NULL,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (operation_id),
    KEY idx_boon_reward_player_created (pid, created_at),
    CONSTRAINT boon_reward_operation_fk FOREIGN KEY (operation_id)
        REFERENCES critical_operation_inbox (operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS boon_reward_outcome_entry (
    operation_id BINARY(16) NOT NULL,
    entry_index SMALLINT UNSIGNED NOT NULL,
    boon_id INT NOT NULL,
    counter_after DOUBLE NOT NULL,
    completed TINYINT UNSIGNED NOT NULL,
    reward_type TINYINT UNSIGNED NOT NULL,
    reward_value DOUBLE NOT NULL,
    PRIMARY KEY (operation_id, entry_index),
    UNIQUE KEY uq_boon_reward_operation_boon (operation_id, boon_id),
    KEY idx_boon_reward_boon (boon_id, operation_id),
    CONSTRAINT boon_reward_entry_operation_fk FOREIGN KEY (operation_id)
        REFERENCES boon_reward_outcome (operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS zone_touch_outcome (
    operation_id BINARY(16) NOT NULL,
    zone_number INT NOT NULL,
    toucher_pid INT UNSIGNED NOT NULL,
    boot_time INT NOT NULL,
    touched_at INT NOT NULL,
    group_size SMALLINT UNSIGNED NOT NULL,
    epic_value INT NOT NULL,
    alignment_delta SMALLINT NOT NULL,
    reset_requested TINYINT UNSIGNED NOT NULL DEFAULT 0,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (operation_id),
    KEY idx_zone_touch_outcome_zone_created (zone_number, created_at),
    CONSTRAINT zone_touch_outcome_operation_fk FOREIGN KEY (operation_id)
        REFERENCES critical_operation_inbox (operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS zone_touch_outcome_participant (
    operation_id BINARY(16) NOT NULL,
    participant_index SMALLINT UNSIGNED NOT NULL,
    pid INT UNSIGNED NOT NULL,
    epic_value INT NOT NULL,
    PRIMARY KEY (operation_id, participant_index),
    UNIQUE KEY uq_zone_touch_operation_pid (operation_id, pid),
    KEY idx_zone_touch_participant_pid (pid, operation_id),
    CONSTRAINT zone_touch_participant_operation_fk FOREIGN KEY (operation_id)
        REFERENCES zone_touch_outcome (operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
