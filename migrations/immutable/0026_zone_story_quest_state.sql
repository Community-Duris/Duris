-- Durable aggregate state for the production zone/story quest feature.
--
-- The serialized document contains the idempotent completion fact index,
-- character/PID names, daily assignments and reward ledger, deletion
-- tombstones, and telemetry observations.  The application validates the
-- document checksum in flat-file mode and the schema/version/revision fields
-- here provide the same boot fence in SQL mode.

CREATE TABLE IF NOT EXISTS zone_story_quest_state (
    state_id TINYINT UNSIGNED NOT NULL,
    state_version INT UNSIGNED NOT NULL,
    catalog_revision INT UNSIGNED NOT NULL,
    state_blob MEDIUMTEXT NOT NULL,
    updated_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)
        ON UPDATE CURRENT_TIMESTAMP(6),
    PRIMARY KEY (state_id),
    CONSTRAINT chk_zone_story_quest_state_singleton CHECK (state_id = 1),
    CONSTRAINT chk_zone_story_quest_state_version CHECK (state_version = 1),
    CONSTRAINT chk_zone_story_quest_state_revision CHECK (catalog_revision > 0)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
