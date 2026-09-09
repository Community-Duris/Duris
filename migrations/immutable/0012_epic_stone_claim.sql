-- One durable reward claim per stone incarnation (globally allocated object UID).
CREATE TABLE IF NOT EXISTS epic_stone_claim (
    stone_uid BIGINT UNSIGNED NOT NULL,
    operation_id BINARY(16) NOT NULL,
    PRIMARY KEY (stone_uid),
    KEY idx_epic_stone_claim_operation (operation_id),
    CONSTRAINT epic_stone_claim_operation_fk FOREIGN KEY (operation_id)
        REFERENCES critical_operation_inbox (operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
