-- Phase 02 revisioned artifact feed/bind and guild award outcomes.
-- Additive and re-runnable. Baseline population is a separate guarded action.

SET @guild_revision_missing = (SELECT COUNT(*) = 0 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='guilds' AND column_name='outcome_revision');
SET @guild_revision_sql = IF(@guild_revision_missing,
    'ALTER TABLE guilds ADD COLUMN outcome_revision BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER construction',
    'SELECT 1 INTO @guild_revision_unchanged');
PREPARE guild_revision_stmt FROM @guild_revision_sql;
EXECUTE guild_revision_stmt;
DEALLOCATE PREPARE guild_revision_stmt;

CREATE TABLE IF NOT EXISTS artifact_domain_state (
    vnum INT NOT NULL,
    owned TINYINT UNSIGNED NOT NULL DEFAULT 0,
    loc_type TINYINT UNSIGNED NOT NULL DEFAULT 1,
    location INT NOT NULL DEFAULT 0,
    timer_epoch BIGINT NOT NULL DEFAULT 0,
    artifact_type TINYINT UNSIGNED NOT NULL DEFAULT 0,
    bind_owner_pid INT NOT NULL DEFAULT 0,
    bind_timer_epoch BIGINT NOT NULL DEFAULT 0,
    item_uid BIGINT UNSIGNED NULL,
    item_revision BIGINT UNSIGNED NULL,
    revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    updated_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6) ON UPDATE CURRENT_TIMESTAMP(6),
    PRIMARY KEY (vnum),
    KEY idx_artifact_domain_owner (loc_type, location, vnum),
    KEY idx_artifact_domain_item (item_uid),
    CONSTRAINT artifact_domain_item_fk FOREIGN KEY (item_uid)
        REFERENCES item_current_owner (item_uid) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS artifact_domain_baseline (
    vnum INT NOT NULL,
    opening_timer_epoch BIGINT NOT NULL,
    opening_bind_owner_pid INT NOT NULL,
    opening_bind_timer_epoch BIGINT NOT NULL,
    opening_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    captured_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (vnum)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS artifact_guild_outcome (
    operation_id BINARY(16) NOT NULL,
    parent_operation_id BINARY(16) NOT NULL,
    actor_pid INT UNSIGNED NOT NULL,
    guild_id INT UNSIGNED NOT NULL DEFAULT 0,
    prestige_delta BIGINT NOT NULL DEFAULT 0,
    construction_delta BIGINT NOT NULL DEFAULT 0,
    artifact_count SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    guild_prestige_after BIGINT UNSIGNED NOT NULL DEFAULT 0,
    guild_construction_after BIGINT UNSIGNED NOT NULL DEFAULT 0,
    guild_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (operation_id),
    UNIQUE KEY uq_artifact_guild_parent_actor (parent_operation_id, actor_pid),
    KEY idx_artifact_guild_actor_created (actor_pid, created_at),
    CONSTRAINT artifact_guild_operation_fk FOREIGN KEY (operation_id)
        REFERENCES critical_operation_inbox (operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT artifact_guild_parent_fk FOREIGN KEY (parent_operation_id)
        REFERENCES critical_operation_inbox (operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS artifact_guild_outcome_delta (
    operation_id BINARY(16) NOT NULL,
    artifact_index SMALLINT UNSIGNED NOT NULL,
    vnum INT NOT NULL,
    flags TINYINT UNSIGNED NOT NULL,
    timer_before BIGINT NOT NULL,
    timer_after BIGINT NOT NULL,
    bind_owner_before INT NOT NULL,
    bind_owner_after INT NOT NULL,
    bind_timer_before BIGINT NOT NULL,
    bind_timer_after BIGINT NOT NULL,
    revision BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (operation_id, artifact_index),
    UNIQUE KEY uq_artifact_outcome_vnum (operation_id, vnum),
    CONSTRAINT artifact_outcome_delta_fk FOREIGN KEY (operation_id)
        REFERENCES artifact_guild_outcome (operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS artifact_delta_ledger (
    operation_id BINARY(16) NOT NULL,
    artifact_index SMALLINT UNSIGNED NOT NULL,
    vnum INT NOT NULL,
    timer_delta BIGINT NOT NULL,
    bind_owner_pid INT NOT NULL,
    bind_timer_epoch BIGINT NOT NULL,
    revision BIGINT UNSIGNED NOT NULL,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (operation_id, artifact_index),
    UNIQUE KEY uq_artifact_delta_revision (vnum, revision),
    KEY idx_artifact_delta_created (vnum, created_at),
    CONSTRAINT artifact_delta_operation_fk FOREIGN KEY (operation_id)
        REFERENCES artifact_guild_outcome (operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS guild_outcome_ledger (
    operation_id BINARY(16) NOT NULL,
    guild_id INT UNSIGNED NOT NULL,
    prestige_delta BIGINT NOT NULL,
    construction_delta BIGINT NOT NULL,
    prestige_after BIGINT UNSIGNED NOT NULL,
    construction_after BIGINT UNSIGNED NOT NULL,
    guild_revision BIGINT UNSIGNED NOT NULL,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (operation_id),
    UNIQUE KEY uq_guild_outcome_revision (guild_id, guild_revision),
    KEY idx_guild_outcome_created (guild_id, created_at),
    CONSTRAINT guild_outcome_operation_fk FOREIGN KEY (operation_id)
        REFERENCES artifact_guild_outcome (operation_id) ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
