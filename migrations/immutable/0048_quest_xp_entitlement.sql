-- Persist per-recipient quest XP independently from the turn-in player's
-- general reward obligation so every credited player can recover it on login.
CREATE TABLE IF NOT EXISTS quest_reward_xp_entitlement (
    offering_operation_id BINARY(16) NOT NULL,
    recipient_pid INT UNSIGNED NOT NULL,
    reward_index INT UNSIGNED NOT NULL,
    amount INT UNSIGNED NOT NULL,
    applied_at TIMESTAMP(6) NULL DEFAULT NULL,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (offering_operation_id, recipient_pid, reward_index),
    KEY idx_quest_reward_xp_pending (recipient_pid, applied_at, offering_operation_id),
    CONSTRAINT quest_reward_xp_offering_fk FOREIGN KEY (offering_operation_id)
        REFERENCES quest_reward_obligation (offering_operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
