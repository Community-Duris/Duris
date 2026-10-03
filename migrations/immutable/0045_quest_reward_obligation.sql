-- Retain the exact reward terms with successful quest-offering custody.
-- Acknowledgement is separate from item publication and follows reward effects.
CREATE TABLE IF NOT EXISTS quest_reward_obligation (
    offering_operation_id BINARY(16) NOT NULL,
    player_pid INT UNSIGNED NOT NULL,
    continuation BLOB NOT NULL,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    acknowledged_at TIMESTAMP(6) NULL DEFAULT NULL,
    PRIMARY KEY (offering_operation_id),
    KEY idx_quest_reward_pending (player_pid, acknowledged_at, created_at),
    CONSTRAINT quest_reward_offering_fk FOREIGN KEY (offering_operation_id)
        REFERENCES critical_operation_inbox (operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
