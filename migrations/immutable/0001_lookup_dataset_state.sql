-- Immutable migration 0001: versioned atomic race/class lookup publication state.
CREATE TABLE IF NOT EXISTS lookup_dataset_state (
    dataset_name VARCHAR(32) NOT NULL,
    dataset_version INT UNSIGNED NOT NULL,
    dataset_checksum BINARY(32) NOT NULL,
    race_count SMALLINT UNSIGNED NOT NULL,
    class_count SMALLINT UNSIGNED NOT NULL,
    published_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)
        ON UPDATE CURRENT_TIMESTAMP(6),
    PRIMARY KEY (dataset_name),
    CONSTRAINT chk_lookup_dataset_counts CHECK (race_count > 0 AND class_count > 0)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
