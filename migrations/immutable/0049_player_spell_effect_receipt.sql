-- Retain spell effects with an owning player snapshot revision so recovered
-- committed component commands can suppress an already persisted mutation.
CREATE TABLE IF NOT EXISTS player_spell_effect_receipt (
    pid INT UNSIGNED NOT NULL,
    operation_id BINARY(16) NOT NULL,
    effect_id INT UNSIGNED NOT NULL,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (pid, operation_id),
    KEY idx_player_spell_effect_receipt_age (pid, created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
