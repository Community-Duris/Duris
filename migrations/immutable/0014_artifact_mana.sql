-- Physical-item resource authority is independent of owner snapshots. Keep rows
-- after extraction: reusing a stale snapshot must never reinitialize its UID.
CREATE TABLE IF NOT EXISTS artifact_mana (
    item_uid BIGINT UNSIGNED NOT NULL,
    profile_id BIGINT UNSIGNED NOT NULL,
    profile_revision BIGINT UNSIGNED NOT NULL,
    version BIGINT UNSIGNED NOT NULL,
    capacity BIGINT UNSIGNED NOT NULL,
    regeneration BIGINT UNSIGNED NOT NULL,
    reserve BIGINT UNSIGNED NOT NULL,
    settled_at BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (item_uid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
