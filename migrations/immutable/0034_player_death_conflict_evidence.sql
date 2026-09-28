-- Additive, immutable forensic evidence for rejected death-save conflicts.
-- Payload bytes are v10 evidence only: never loadable inventory or a save receipt.
-- Rows remain unresolved; resolution is represented separately and is out of scope.
-- No player FK/cascade: evidence survives player deletion. Replay compares the
-- original request_hash and must never overwrite first-observed evidence.
CREATE TABLE IF NOT EXISTS player_death_conflict_evidence (
    operation_id BINARY(16) NOT NULL,
    pid INT NOT NULL,
    save_revision BIGINT UNSIGNED NOT NULL,
    source_revision BIGINT UNSIGNED NOT NULL,
    corpse_item_uid BIGINT UNSIGNED NOT NULL,
    request_hash BINARY(32) NOT NULL,
    payload_hash BINARY(32) NOT NULL,
    payload MEDIUMBLOB NOT NULL,
    recorded_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
    PRIMARY KEY (operation_id),
    UNIQUE KEY uq_death_conflict_revision (pid,save_revision),
    UNIQUE KEY uq_death_conflict_corpse (pid,corpse_item_uid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
