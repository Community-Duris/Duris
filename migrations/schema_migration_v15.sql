-- create corpse_item_extra_descr and locker_item_extra_descr tables and add description columns to corpse table
-- schema migration v15


CREATE TABLE IF NOT EXISTS corpse_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_corpse_item_ed FOREIGN KEY (item_id)
    REFERENCES corpse_items(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS locker_item_extra_descr (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT,
  item_id INT UNSIGNED NOT NULL,
  keyword VARCHAR(255) NOT NULL,
  description TEXT,
  PRIMARY KEY (id),
  INDEX idx_item_id (item_id),
  CONSTRAINT fk_locker_item_ed FOREIGN KEY (item_id)
    REFERENCES locker_items(id) ON DELETE CASCADE
);

ALTER TABLE corpses
  ADD COLUMN short_descr VARCHAR(512) DEFAULT NULL AFTER created_at,
  ADD COLUMN description TEXT DEFAULT NULL AFTER short_descr;

ALTER TABLE player_data 
  CONVERT TO CHARACTER SET utf8mb4 
  COLLATE utf8mb4_0900_ai_ci;