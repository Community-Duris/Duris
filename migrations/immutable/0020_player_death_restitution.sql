-- Immutable migration 0020: audited, idempotent restitution receipts.
-- The SQL operator is intentionally narrow: only original, payload-backed item
-- identities can be delivered. Currency and unsupported artifact authority remain
-- classifications in the receipt, never automatic refunds.
CREATE TABLE IF NOT EXISTS player_death_restitution_receipt (
    restitution_id BINARY(16) NOT NULL,
    source_pid INT NOT NULL,
    death_revision BIGINT UNSIGNED NOT NULL,
    recipient_pid INT NOT NULL,
    death_operation_id BINARY(16) NOT NULL,
    evidence_digest BINARY(32) NOT NULL,
    plan_digest BINARY(32) NOT NULL,
    status TINYINT UNSIGNED NOT NULL DEFAULT 1,
    actor VARCHAR(128) NOT NULL,
    reason VARCHAR(255) NOT NULL,
    candidate_count SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    delivered_count SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    unresolved_count SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    approved_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    applied_at TIMESTAMP(6) NULL DEFAULT NULL,
    verified_at TIMESTAMP(6) NULL DEFAULT NULL,
    PRIMARY KEY (restitution_id),
    UNIQUE KEY uq_restitution_plan (source_pid,death_revision,recipient_pid,plan_digest),
    KEY idx_restitution_source (source_pid,death_revision),
    KEY idx_restitution_operation (death_operation_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS player_death_restitution_item (
    restitution_id BINARY(16) NOT NULL,
    item_uid BIGINT UNSIGNED NOT NULL,
    source_root_item_uid BIGINT UNSIGNED NOT NULL DEFAULT 0,
    source_parent_item_uid BIGINT UNSIGNED NOT NULL DEFAULT 0,
    delivered_root_item_uid BIGINT UNSIGNED NOT NULL DEFAULT 0,
    delivered_parent_item_uid BIGINT UNSIGNED NOT NULL DEFAULT 0,
    source_item_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    delivered_item_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    vnum INT NOT NULL DEFAULT 0,
    artifact_vnum INT NOT NULL DEFAULT 0,
    disposition TINYINT UNSIGNED NOT NULL,
    classification VARCHAR(64) NOT NULL,
    metadata_digest BINARY(32) NULL,
    metadata_payload MEDIUMBLOB NULL,
    note VARCHAR(255) NOT NULL DEFAULT '',
    artifact_loss_epoch BIGINT UNSIGNED NOT NULL DEFAULT 0,
    artifact_source_timer_epoch BIGINT UNSIGNED NOT NULL DEFAULT 0,
    artifact_usable_lifetime_seconds BIGINT UNSIGNED NOT NULL DEFAULT 0,
    artifact_delivered_timer_epoch BIGINT UNSIGNED NOT NULL DEFAULT 0,
    artifact_timing_basis VARCHAR(64) NOT NULL DEFAULT '',
    artifact_compensation_reference VARCHAR(255) NOT NULL DEFAULT '',
    PRIMARY KEY (restitution_id,item_uid),
    KEY idx_restitution_item_uid (item_uid),
    CONSTRAINT restitution_item_receipt_fk FOREIGN KEY (restitution_id)
        REFERENCES player_death_restitution_receipt(restitution_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- One physical UID can be referenced by several death revisions. This unique
-- guard is the cross-death idempotency boundary; a death revision is not enough.
CREATE TABLE IF NOT EXISTS player_death_restitution_delivery (
    item_uid BIGINT UNSIGNED NOT NULL,
    restitution_id BINARY(16) NOT NULL,
    source_pid INT NOT NULL,
    death_revision BIGINT UNSIGNED NOT NULL,
    recipient_pid INT NOT NULL,
    source_item_revision BIGINT UNSIGNED NOT NULL,
    delivered_item_revision BIGINT UNSIGNED NOT NULL,
    delivered_item_id INT UNSIGNED NOT NULL,
    metadata_digest BINARY(32) NOT NULL,
    original_payload MEDIUMBLOB NOT NULL,
    delivered_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (item_uid),
    UNIQUE KEY uq_restitution_delivery_receipt_item (restitution_id,item_uid),
    KEY idx_restitution_delivery_receipt (restitution_id),
    KEY idx_restitution_delivery_recipient (recipient_pid,item_uid),
    CONSTRAINT restitution_delivery_receipt_fk FOREIGN KEY (restitution_id,item_uid)
        REFERENCES player_death_restitution_item(restitution_id,item_uid)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- The original payload is immutable evidence. This small mutable companion is
-- updated by the normal SQL snapshot writer so reconnects retain current exact
-- generated state instead of replaying an old death snapshot after a later save.
CREATE TABLE IF NOT EXISTS player_death_restitution_runtime (
    item_uid BIGINT UNSIGNED NOT NULL,
    recipient_pid INT NOT NULL,
    state_payload MEDIUMBLOB NOT NULL,
    state_digest BINARY(32) NOT NULL,
    updated_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)
        ON UPDATE CURRENT_TIMESTAMP(6),
    PRIMARY KEY (item_uid),
    KEY idx_restitution_runtime_recipient (recipient_pid,item_uid),
    CONSTRAINT restitution_runtime_delivery_fk FOREIGN KEY (item_uid)
        REFERENCES player_death_restitution_delivery(item_uid)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
