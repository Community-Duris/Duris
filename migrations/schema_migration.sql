-- durismud pfile to database migration schema
-- generated: 2025-12-30
--
-- note: this database is shared with durismud website
-- many tables already exist - this file only creates missing tables
--
-- existing tables (do not recreate):
--   accounts, account_characters, ships, ship_slots,
--   guilds, guild_members, guild_titles, player_*

-- ============================================================================
-- account extension: ip tracking
-- ============================================================================

CREATE TABLE IF NOT EXISTS account_ips (
    id INT AUTO_INCREMENT PRIMARY KEY,
    account_name VARCHAR(50) NOT NULL,        -- fk to accounts.account_name
    hostname VARCHAR(255),
    ip_address VARCHAR(45),                   -- ipv6 compatible
    count BIGINT UNSIGNED DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (account_name) REFERENCES accounts(account_name) ON DELETE CASCADE,
    INDEX idx_account_name (account_name),
    INDEX idx_ip_address (ip_address),
    UNIQUE KEY uk_account_ip (account_name, ip_address)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


-- ============================================================================
-- town/kingdom tables
-- ============================================================================

CREATE TABLE IF NOT EXISTS towns (
    id INT AUTO_INCREMENT PRIMARY KEY,
    zone_filename VARCHAR(100) NOT NULL,
    resources INT DEFAULT 0,
    defense INT DEFAULT 0,
    offense INT DEFAULT 0,
    deploy_guard TINYINT DEFAULT 0,
    guard_vnum INT DEFAULT 0,
    guard_max INT DEFAULT 0,
    guard_load_room INT DEFAULT 0,
    deploy_cavalry TINYINT DEFAULT 0,
    cavalry_vnum INT DEFAULT 0,
    cavalry_max INT DEFAULT 0,
    cavalry_load_room INT DEFAULT 0,
    deploy_portals TINYINT DEFAULT 0,
    portal_vnum INT DEFAULT 0,
    portal_load_room INT DEFAULT 0,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY uk_zone_filename (zone_filename)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS siege_objects (
    id INT AUTO_INCREMENT PRIMARY KEY,
    room_number INT NOT NULL,
    object_data BLOB,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_room_number (room_number)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS kingdom_land (
    id INT AUTO_INCREMENT PRIMARY KEY,
    kingdom_id INT NOT NULL,
    start_vnum INT DEFAULT 0,
    end_vnum INT DEFAULT 0,
    type CHAR(1) DEFAULT 'r',                 -- h/r/u
    INDEX idx_kingdom_id (kingdom_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


-- ============================================================================
-- player extension: tradeskill recipes
-- ============================================================================

CREATE TABLE IF NOT EXISTS player_recipes (
    id INT AUTO_INCREMENT PRIMARY KEY,
    pid INT NOT NULL,
    recipe_vnum INT NOT NULL,
    learned_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_recipe (pid, recipe_vnum)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


-- ============================================================================
-- player extension: shapechange forms
-- ============================================================================

CREATE TABLE IF NOT EXISTS player_shapechanges (
    id INT AUTO_INCREMENT PRIMARY KEY,
    pid INT NOT NULL,
    mob_vnum INT NOT NULL,
    times_researched INT DEFAULT 0,
    last_researched BIGINT DEFAULT 0,
    last_shapechanged BIGINT DEFAULT 0,
    INDEX idx_pid (pid),
    UNIQUE KEY uk_pid_mob (pid, mob_vnum)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


-- ============================================================================
-- corpse tables
-- ============================================================================

CREATE TABLE IF NOT EXISTS corpses (
    id INT AUTO_INCREMENT PRIMARY KEY,
    player_name VARCHAR(50) NOT NULL,
    save_id BIGINT NOT NULL,
    room_vnum INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_player_name (player_name),
    UNIQUE KEY uk_player_saveid (player_name, save_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS corpse_items (
    id INT AUTO_INCREMENT PRIMARY KEY,
    corpse_id INT NOT NULL,
    item_vnum INT NOT NULL,
    container_id INT DEFAULT NULL,
    item_data BLOB,
    FOREIGN KEY (corpse_id) REFERENCES corpses(id) ON DELETE CASCADE,
    INDEX idx_corpse_id (corpse_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


-- ============================================================================
-- shopkeeper tables
-- ============================================================================

CREATE TABLE IF NOT EXISTS shopkeepers (
    id INT AUTO_INCREMENT PRIMARY KEY,
    shop_id INT NOT NULL UNIQUE,
    mob_vnum INT DEFAULT 0,
    room_vnum INT DEFAULT 0,
    save_time BIGINT DEFAULT 0,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_shop_id (shop_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS shopkeeper_affects (
    id INT AUTO_INCREMENT PRIMARY KEY,
    shopkeeper_id INT NOT NULL,
    type INT DEFAULT 0,
    duration INT DEFAULT 0,
    modifier INT DEFAULT 0,
    location INT DEFAULT 0,
    bitvector1 BIGINT UNSIGNED DEFAULT 0,
    bitvector2 BIGINT UNSIGNED DEFAULT 0,
    bitvector3 BIGINT UNSIGNED DEFAULT 0,
    bitvector4 BIGINT UNSIGNED DEFAULT 0,
    bitvector5 BIGINT UNSIGNED DEFAULT 0,
	item_material TINYINT DEFAULT NULL,
    FOREIGN KEY (shopkeeper_id) REFERENCES shopkeepers(id) ON DELETE CASCADE,
    INDEX idx_shopkeeper_id (shopkeeper_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS shopkeeper_items (
    id INT AUTO_INCREMENT PRIMARY KEY,
    shopkeeper_id INT NOT NULL,
    item_vnum INT NOT NULL,
    slot INT DEFAULT -1,
    container_id INT DEFAULT NULL,
    item_data BLOB,
    FOREIGN KEY (shopkeeper_id) REFERENCES shopkeepers(id) ON DELETE CASCADE,
    INDEX idx_shopkeeper_id (shopkeeper_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


-- ============================================================================
-- saved items table
-- ============================================================================

CREATE TABLE IF NOT EXISTS saved_items (
    id INT AUTO_INCREMENT PRIMARY KEY,
    item_key VARCHAR(100) NOT NULL UNIQUE,
    room_vnum INT DEFAULT 0,
    item_vnum INT NOT NULL,
    item_data BLOB,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_item_key (item_key),
    INDEX idx_room_vnum (room_vnum)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


-- ============================================================================
-- summary: 12 new tables
-- ============================================================================
-- account_ips         - track unique ips per account
-- towns               - town/kingdom siege data
-- siege_objects       - siege object persistence
-- kingdom_land        - kingdom territory mapping
-- player_recipes      - tradeskill recipes learned
-- player_shapechanges - druid shapechange forms
-- corpses             - player corpse persistence
-- corpse_items        - items inside corpses
-- shopkeepers         - persistent shopkeeper state
-- shopkeeper_affects  - shopkeeper spell effects
-- shopkeeper_items    - shopkeeper inventory
-- saved_items         - persistent world items
-- ============================================================================
