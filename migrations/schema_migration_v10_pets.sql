-- pet persistence: save charmed pets across crashes
-- schema migration v10

-- pet data (one row per pet)
CREATE TABLE IF NOT EXISTS `player_pets` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `owner_pid` INT UNSIGNED NOT NULL,
  `mob_vnum` INT NOT NULL,
  `pet_order` TINYINT DEFAULT 0,
  `hit` INT DEFAULT 0,
  `max_hit` INT DEFAULT 0,
  `mana` INT DEFAULT 0,
  `max_mana` INT DEFAULT 0,
  `vitality` INT DEFAULT 0,
  `max_vitality` INT DEFAULT 0,
  `charm_duration` INT DEFAULT -1,
  `room_vnum` INT DEFAULT 0,
  `saved_at` BIGINT DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `idx_owner_pid` (`owner_pid`),
  CONSTRAINT `fk_player_pets_owner` FOREIGN KEY (`owner_pid`)
    REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- pet equipment and inventory
CREATE TABLE IF NOT EXISTS `player_pet_items` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `pet_id` INT UNSIGNED NOT NULL,
  `vnum` INT NOT NULL,
  `equip_slot` TINYINT DEFAULT 0,
  `container_id` INT UNSIGNED DEFAULT NULL,
  `weight` INT DEFAULT 0,
  `cost` INT DEFAULT 0,
  `timer` INT DEFAULT -1,
  `extra_flags` BIGINT UNSIGNED DEFAULT 0,
  `value0` INT DEFAULT 0,
  `value1` INT DEFAULT 0,
  `value2` INT DEFAULT 0,
  `value3` INT DEFAULT 0,
  `value4` INT DEFAULT 0,
  `value5` INT DEFAULT 0,
  `value6` INT DEFAULT 0,
  `value7` INT DEFAULT 0,
  `name` VARCHAR(512) DEFAULT NULL,
  `short_descr` VARCHAR(512) DEFAULT NULL,
  `description` TEXT DEFAULT NULL,
  `action_descr` TEXT DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_pet_id` (`pet_id`),
  KEY `idx_container_id` (`container_id`),
  CONSTRAINT `fk_pet_items_pet` FOREIGN KEY (`pet_id`)
    REFERENCES `player_pets` (`id`) ON DELETE CASCADE,
  CONSTRAINT `fk_pet_items_container` FOREIGN KEY (`container_id`)
    REFERENCES `player_pet_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- pet item affects (stat mods)
CREATE TABLE IF NOT EXISTS `player_pet_item_affects` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `item_id` INT UNSIGNED NOT NULL,
  `location` TINYINT UNSIGNED DEFAULT 0,
  `modifier` INT DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `fk_pet_item_affects` FOREIGN KEY (`item_id`)
    REFERENCES `player_pet_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
