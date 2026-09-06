-- Immutable migration 0011: durable death disposition and its custody evidence.
-- A corpse handoff the ledger refuses cannot be retried into the same refusal
-- forever. The terminal death record keeps the corpse identity and location, the
-- wallet the conversion never took, the complete refused item payload and the
-- custody rows that were disputed, outside the player's active inventory.
CREATE TABLE IF NOT EXISTS player_death_disposition (
    pid INT NOT NULL,
    save_revision BIGINT UNSIGNED NOT NULL,
    operation_id BINARY(16) NOT NULL,
    corpse_item_uid BIGINT UNSIGNED NOT NULL,
    corpse_room_vnum INT NOT NULL,
    wallet_revision BIGINT UNSIGNED NOT NULL,
    wallet_copper INT NOT NULL DEFAULT 0,
    wallet_silver INT NOT NULL DEFAULT 0,
    wallet_gold INT NOT NULL DEFAULT 0,
    wallet_platinum INT NOT NULL DEFAULT 0,
    wallet_pile_uid BIGINT UNSIGNED NOT NULL DEFAULT 0,
    payload MEDIUMBLOB NOT NULL,
    recorded_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (pid,save_revision),
    KEY idx_player_death_disposition_operation (operation_id),
    KEY idx_player_death_disposition_corpse (corpse_item_uid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS player_death_custody (
    pid INT NOT NULL,
    save_revision BIGINT UNSIGNED NOT NULL,
    item_uid BIGINT UNSIGNED NOT NULL,
    root_item_uid BIGINT UNSIGNED NOT NULL,
    parent_item_uid BIGINT UNSIGNED NOT NULL DEFAULT 0,
    item_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    vnum INT NOT NULL DEFAULT 0,
    state TINYINT UNSIGNED NOT NULL DEFAULT 0,
    owner_type TINYINT UNSIGNED NOT NULL DEFAULT 0,
    owner_id BIGINT UNSIGNED NOT NULL DEFAULT 0,
    owner_context_id BIGINT UNSIGNED NOT NULL DEFAULT 0,
    owner_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (pid,save_revision,item_uid),
    KEY idx_player_death_custody_item (item_uid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
