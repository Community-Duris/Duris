-- Revision authority for atomic corpse disposal and restart-safe completion receipts.
-- Existing corpse rows begin at revision 1; all later saves and lifecycle mutations
-- advance the per-corpse and singleton catalog revisions while holding row locks.

SET @corpse_revision_sql = IF(EXISTS(SELECT 1 FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='corpses' AND column_name='corpse_revision'),
    'SELECT 1',
    'ALTER TABLE corpses ADD COLUMN corpse_revision BIGINT UNSIGNED NOT NULL DEFAULT 1 AFTER save_id');
PREPARE corpse_revision_stmt FROM @corpse_revision_sql;
EXECUTE corpse_revision_stmt;
DEALLOCATE PREPARE corpse_revision_stmt;

SET @corpse_owner_index_sql = IF(EXISTS(SELECT 1 FROM information_schema.statistics
    WHERE table_schema=DATABASE() AND table_name='corpses' AND index_name='idx_corpse_owner_save'),
    'SELECT 1',
    'ALTER TABLE corpses ADD INDEX idx_corpse_owner_save(value3,save_id)');
PREPARE corpse_owner_index_stmt FROM @corpse_owner_index_sql;
EXECUTE corpse_owner_index_stmt;
DEALLOCATE PREPARE corpse_owner_index_stmt;

CREATE TABLE IF NOT EXISTS corpse_catalog_state (
    state_id TINYINT UNSIGNED NOT NULL,
    catalog_revision BIGINT UNSIGNED NOT NULL DEFAULT 1,
    updated_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)
        ON UPDATE CURRENT_TIMESTAMP(6),
    PRIMARY KEY (state_id),
    CONSTRAINT chk_corpse_catalog_singleton CHECK (state_id = 1),
    CONSTRAINT chk_corpse_catalog_revision CHECK (catalog_revision > 0)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

INSERT IGNORE INTO corpse_catalog_state(state_id,catalog_revision) VALUES(1,1);
