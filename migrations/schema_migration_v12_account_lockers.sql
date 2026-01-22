-- account locker system migration
-- replaces character-based lockers with account-based lockers containing keyword-protected chests

-- ============================================================================
-- account lockers table (one per account)
-- ============================================================================

CREATE TABLE IF NOT EXISTS account_lockers (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    account_name VARCHAR(50) NOT NULL UNIQUE,
    racewar TINYINT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (account_name) REFERENCES accounts(account_name) ON DELETE CASCADE,
    INDEX idx_account_name (account_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================================
-- locker chests (public + private keyword-protected)
-- ============================================================================

CREATE TABLE IF NOT EXISTS locker_chests (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    keyword VARCHAR(64) NOT NULL,
    keyword_hash VARCHAR(64) DEFAULT NULL,
    is_public TINYINT(1) DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (locker_id) REFERENCES account_lockers(id) ON DELETE CASCADE,
    UNIQUE KEY uk_locker_keyword (locker_id, keyword),
    INDEX idx_locker_id (locker_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================================
-- account locker items (belongs to chests, not directly to lockers)
-- ============================================================================

CREATE TABLE IF NOT EXISTS account_locker_items (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    chest_id INT UNSIGNED NOT NULL,
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
    obj_uid BIGINT UNSIGNED DEFAULT NULL,
    item_condition SMALLINT DEFAULT 100,
    FOREIGN KEY (chest_id) REFERENCES locker_chests(id) ON DELETE CASCADE,
    FOREIGN KEY (container_id) REFERENCES account_locker_items(id) ON DELETE CASCADE,
    INDEX idx_chest_id (chest_id),
    INDEX idx_vnum (vnum),
    INDEX idx_obj_uid (obj_uid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================================
-- account locker item affects
-- ============================================================================

CREATE TABLE IF NOT EXISTS account_locker_item_affects (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    item_id INT UNSIGNED NOT NULL,
    location TINYINT UNSIGNED DEFAULT 0,
    modifier INT DEFAULT 0,
    FOREIGN KEY (item_id) REFERENCES account_locker_items(id) ON DELETE CASCADE,
    INDEX idx_item_id (item_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================================
-- account locker access sharing (account-to-account)
-- ============================================================================

CREATE TABLE IF NOT EXISTS account_locker_access (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    visitor_account VARCHAR(50) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (locker_id) REFERENCES account_lockers(id) ON DELETE CASCADE,
    UNIQUE KEY uk_locker_visitor (locker_id, visitor_account),
    INDEX idx_visitor (visitor_account)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================================
-- activity log (last 50 entries per locker)
-- ============================================================================

CREATE TABLE IF NOT EXISTS locker_activity_log (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    account_name VARCHAR(50) NOT NULL,
    char_name VARCHAR(64) NOT NULL,
    action_type ENUM('enter', 'leave', 'chest_open', 'chest_fail', 'kicked', 'chest_create', 'chest_delete', 'item_put', 'item_get') NOT NULL,
    chest_keyword VARCHAR(64) DEFAULT NULL,
    details VARCHAR(255) DEFAULT NULL,
    logged_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (locker_id) REFERENCES account_lockers(id) ON DELETE CASCADE,
    INDEX idx_locker_id (locker_id),
    INDEX idx_logged_at (logged_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================================
-- kickout tracking (rate limiting for keyword failures)
-- ============================================================================

CREATE TABLE IF NOT EXISTS locker_kickouts (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    account_name VARCHAR(50) NOT NULL,
    fail_count TINYINT UNSIGNED DEFAULT 0,
    kicked_until TIMESTAMP NULL DEFAULT NULL,
    last_fail TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (locker_id) REFERENCES account_lockers(id) ON DELETE CASCADE,
    UNIQUE KEY uk_locker_account (locker_id, account_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================================
-- session state for open chests (tracks which private chests are visible)
-- ============================================================================

CREATE TABLE IF NOT EXISTS locker_session_state (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    locker_id INT UNSIGNED NOT NULL,
    account_name VARCHAR(50) NOT NULL,
    chest_id INT UNSIGNED NOT NULL,
    opened_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (locker_id) REFERENCES account_lockers(id) ON DELETE CASCADE,
    FOREIGN KEY (chest_id) REFERENCES locker_chests(id) ON DELETE CASCADE,
    UNIQUE KEY uk_session (locker_id, account_name, chest_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
