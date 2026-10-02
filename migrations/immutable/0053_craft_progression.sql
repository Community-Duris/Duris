-- Freeze recipe progression in the item root. Applying its player snapshot
-- advances applied_revision in the same native transaction as XP and skills.
CREATE TABLE IF NOT EXISTS player_craft_progression (
    operation_id BINARY(16) NOT NULL,
    pid INT UNSIGNED NOT NULL,
    discipline INT UNSIGNED NOT NULL,
    experience INT UNSIGNED NOT NULL,
    applied_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (operation_id),
    KEY idx_player_craft_progression_owner (pid, applied_revision, operation_id),
    CONSTRAINT ck_player_craft_progression_terms
        CHECK (pid > 0 AND discipline IN (1,2) AND experience <= 2147483647),
    CONSTRAINT fk_player_craft_progression_operation FOREIGN KEY (operation_id)
        REFERENCES critical_operation_inbox(operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
