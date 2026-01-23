-- obj_uid and item_condition for duplication prevention
-- schema migration v11

-- player_items: rename unique_id to obj_uid, change to bigint, add condition
ALTER TABLE player_items
  CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL,
  ADD COLUMN item_condition SMALLINT DEFAULT 100 AFTER obj_uid,
  ADD INDEX idx_obj_uid (obj_uid);

-- corpse_items: same changes
ALTER TABLE corpse_items
  CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL,
  ADD COLUMN item_condition SMALLINT DEFAULT 100 AFTER obj_uid,
  ADD INDEX idx_obj_uid (obj_uid);

-- locker_items: same changes
ALTER TABLE locker_items
  CHANGE COLUMN unique_id obj_uid BIGINT UNSIGNED DEFAULT NULL,
  ADD COLUMN item_condition SMALLINT DEFAULT 100 AFTER obj_uid,
  ADD INDEX idx_obj_uid (obj_uid);

-- player_pet_items: add obj_uid and condition (no unique_id existed)
ALTER TABLE player_pet_items
  ADD COLUMN obj_uid BIGINT UNSIGNED DEFAULT NULL AFTER action_descr,
  ADD COLUMN item_condition SMALLINT DEFAULT 100 AFTER obj_uid,
  ADD INDEX idx_obj_uid (obj_uid);
