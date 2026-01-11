-- durismud locker tables schema
-- player lockers and guild lockers

-- ============================================================================
-- lockers table (replaces .locker pfiles)
-- ============================================================================

CREATE TABLE IF NOT EXISTS lockers (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_name VARCHAR(100) NOT NULL UNIQUE,
    owner_pid INT DEFAULT NULL,
    owner_assoc_id INT DEFAULT NULL,
    racewar TINYINT DEFAULT 0,
    race TINYINT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_owner_pid (owner_pid),
    INDEX idx_owner_assoc_id (owner_assoc_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


-- ============================================================================
-- locker items (normalized, same pattern as other item tables)
-- ============================================================================

CREATE TABLE IF NOT EXISTS locker_items (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    vnum INT NOT NULL,
    container_id INT UNSIGNED DEFAULT NULL,
    quantity SMALLINT UNSIGNED DEFAULT 1,
    weight INT DEFAULT 0,
    cost INT DEFAULT 0,
    timer INT DEFAULT -1,
    extra_flags BIGINT UNSIGNED DEFAULT 0,
    value0 INT DEFAULT 0,
    value1 INT DEFAULT 0,
    value2 INT DEFAULT 0,
    value3 INT DEFAULT 0,
    value4 INT DEFAULT 0,
    value5 INT DEFAULT 0,
    value6 INT DEFAULT 0,
    value7 INT DEFAULT 0,
    name VARCHAR(512) DEFAULT NULL,
    short_descr VARCHAR(512) DEFAULT NULL,
    description TEXT DEFAULT NULL,
    action_descr TEXT DEFAULT NULL,
    unique_id INT UNSIGNED DEFAULT NULL,
    FOREIGN KEY (locker_id) REFERENCES lockers(id) ON DELETE CASCADE,
    FOREIGN KEY (container_id) REFERENCES locker_items(id) ON DELETE CASCADE,
    INDEX idx_locker_id (locker_id),
    INDEX idx_vnum (vnum)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS locker_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES locker_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
