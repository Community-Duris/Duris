-- RAM-authoritative boon purchases have a durable domain clock in both backends.
-- Additive and re-runnable; run before starting the RAM-authority binary.
SET @ram_stat_missing = (SELECT COUNT(*) = 0 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='player_data' AND column_name='base_stat_revision');
SET @ram_stat_sql = IF(@ram_stat_missing,
    'ALTER TABLE player_data ADD COLUMN base_stat_revision BIGINT UNSIGNED NOT NULL DEFAULT 0',
    'SELECT 1 INTO @ram_stat_unchanged');
PREPARE ram_stat_stmt FROM @ram_stat_sql;
EXECUTE ram_stat_stmt;
DEALLOCATE PREPARE ram_stat_stmt;

CREATE TABLE IF NOT EXISTS shop_trade_state (
    shop_id INT UNSIGNED NOT NULL PRIMARY KEY,
    revision BIGINT UNSIGNED NOT NULL DEFAULT 1
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT IGNORE INTO shop_trade_state(shop_id) SELECT shop_id FROM shopkeepers;
SET @ram_condition_missing = (SELECT COUNT(*) = 0 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='shopkeeper_items' AND column_name='item_condition');
SET @ram_condition_sql = IF(@ram_condition_missing,
    'ALTER TABLE shopkeeper_items ADD COLUMN item_condition SMALLINT NOT NULL DEFAULT 100',
    'SELECT 1 INTO @ram_condition_unchanged');
PREPARE ram_condition_stmt FROM @ram_condition_sql;
EXECUTE ram_condition_stmt;
DEALLOCATE PREPARE ram_condition_stmt;
CREATE TABLE IF NOT EXISTS shopkeeper_item_runtime_state (
    item_id INT UNSIGNED NOT NULL PRIMARY KEY,
    payload MEDIUMBLOB NOT NULL,
    CONSTRAINT fk_shopkeeper_runtime_item FOREIGN KEY (item_id) REFERENCES shopkeeper_items(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

SET @ram_saved_missing = (SELECT COUNT(*) = 0 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='saved_items' AND column_name='item_condition');
SET @ram_saved_sql = IF(@ram_saved_missing,
    'ALTER TABLE saved_items ADD COLUMN item_condition SMALLINT NOT NULL DEFAULT 100',
    'SELECT 1 INTO @ram_saved_unchanged');
PREPARE ram_saved_stmt FROM @ram_saved_sql;
EXECUTE ram_saved_stmt;
DEALLOCATE PREPARE ram_saved_stmt;
CREATE TABLE IF NOT EXISTS corpse_item_runtime_state (
    item_id INT UNSIGNED NOT NULL PRIMARY KEY,
    payload MEDIUMBLOB NOT NULL,
    CONSTRAINT fk_corpse_item_runtime FOREIGN KEY (item_id) REFERENCES corpse_items(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
CREATE TABLE IF NOT EXISTS saved_item_runtime_state (
    item_id INT UNSIGNED NOT NULL PRIMARY KEY,
    payload MEDIUMBLOB NOT NULL,
    CONSTRAINT fk_saved_item_runtime FOREIGN KEY (item_id) REFERENCES saved_items(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS player_pet_item_runtime_state (
    item_id INT UNSIGNED NOT NULL PRIMARY KEY,
    payload MEDIUMBLOB NOT NULL,
    CONSTRAINT fk_player_pet_item_runtime FOREIGN KEY (item_id) REFERENCES player_pet_items(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
