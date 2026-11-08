-- Preserve item state absent from the canonical scalar/metadata columns.
-- Rows follow the physical player_items row through transfers and are replaced
-- with the same snapshot transaction at save. No legacy data is overwritten.
CREATE TABLE IF NOT EXISTS player_item_runtime_state (
 item_id INT UNSIGNED NOT NULL,
 payload MEDIUMBLOB NOT NULL,
 PRIMARY KEY (item_id),
 CONSTRAINT fk_player_item_runtime_state FOREIGN KEY (item_id)
  REFERENCES player_items(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
