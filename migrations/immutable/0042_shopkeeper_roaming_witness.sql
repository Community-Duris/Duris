-- Existing keeper snapshots do not prove the shop's roaming policy. Leave the
-- value unknown until a successful shopkeeper checkpoint captures configuration.
SET @shopkeeper_roaming_exists = (
    SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'shopkeepers'
      AND column_name = 'keeper_roaming'
);
SET @shopkeeper_roaming_ddl = IF(
    @shopkeeper_roaming_exists = 0,
    'ALTER TABLE shopkeepers ADD COLUMN keeper_roaming TINYINT UNSIGNED NULL DEFAULT NULL AFTER shop_revision',
    'SELECT 1'
);
PREPARE shopkeeper_roaming_stmt FROM @shopkeeper_roaming_ddl;
EXECUTE shopkeeper_roaming_stmt;
DEALLOCATE PREPARE shopkeeper_roaming_stmt;
