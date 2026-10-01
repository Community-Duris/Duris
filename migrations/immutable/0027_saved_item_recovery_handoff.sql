-- A saved root and its descendants intentionally share item_key. The old
-- unique index allowed one root but rejected every nested child. This index
-- change preserves all rows and is rerunnable after an interrupted DDL.
SET @saved_key_unique = (
    SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'saved_items'
      AND index_name = 'item_key' AND non_unique = 0
);
SET @saved_key_sql = IF(@saved_key_unique > 0,
    'ALTER TABLE saved_items DROP INDEX item_key', 'DO 0');
PREPARE saved_key_statement FROM @saved_key_sql;
EXECUTE saved_key_statement;
DEALLOCATE PREPARE saved_key_statement;

SET @saved_key_index = (
    SELECT COUNT(*) FROM information_schema.statistics
    WHERE table_schema = DATABASE() AND table_name = 'saved_items'
      AND index_name = 'idx_saved_items_item_key'
);
SET @saved_key_sql = IF(@saved_key_index = 0,
    'ALTER TABLE saved_items ADD INDEX idx_saved_items_item_key (item_key)',
    'DO 0');
PREPARE saved_key_statement FROM @saved_key_sql;
EXECUTE saved_key_statement;
DEALLOCATE PREPARE saved_key_statement;

-- This receipt is the durable acknowledgment that a replacement graph was
-- committed. A later transaction may retire only its named source root,
-- whose descendants are removed by the existing self-FK cascade.
CREATE TABLE IF NOT EXISTS saved_item_recovery_handoff (
    season_epoch BIGINT UNSIGNED NOT NULL,
    source_root_id INT UNSIGNED NOT NULL,
    source_key VARCHAR(100) NOT NULL,
    source_uid BIGINT UNSIGNED NOT NULL,
    source_room_vnum INT NOT NULL,
    source_row_count INT UNSIGNED NOT NULL,
    source_id_digest BINARY(32) NOT NULL,
    destination_root_id INT UNSIGNED NOT NULL,
    destination_key VARCHAR(100) NOT NULL,
    acknowledged_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    retired_at TIMESTAMP(6) NULL DEFAULT NULL,
    PRIMARY KEY (season_epoch, source_root_id),
    KEY idx_saved_item_handoff_destination (season_epoch, destination_root_id),
    KEY idx_saved_item_handoff_uid (season_epoch, source_uid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
