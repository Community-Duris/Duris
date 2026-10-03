-- Existing snapshots have no captured cash. NULL keeps that distinction until
-- the next successful shopkeeper checkpoint; no historical value is invented.
SET @shopkeeper_cash_exists = (
    SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'shopkeepers'
      AND column_name = 'cash'
);
SET @shopkeeper_cash_ddl = IF(
    @shopkeeper_cash_exists = 0,
    'ALTER TABLE shopkeepers ADD COLUMN cash INT NULL DEFAULT NULL AFTER save_time',
    'SELECT 1'
);
PREPARE shopkeeper_cash_stmt FROM @shopkeeper_cash_ddl;
EXECUTE shopkeeper_cash_stmt;
DEALLOCATE PREPARE shopkeeper_cash_stmt;

-- Stable shopkeeper IDs let the treasury mapping retain one shop lifetime.
SET @shopkeeper_revision_exists = (
    SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'shopkeepers'
      AND column_name = 'shop_revision'
);
SET @shopkeeper_revision_ddl = IF(
    @shopkeeper_revision_exists = 0,
    'ALTER TABLE shopkeepers ADD COLUMN shop_revision BIGINT UNSIGNED NOT NULL DEFAULT 1 AFTER cash',
    'SELECT 1'
);
PREPARE shopkeeper_revision_stmt FROM @shopkeeper_revision_ddl;
EXECUTE shopkeeper_revision_stmt;
DEALLOCATE PREPARE shopkeeper_revision_stmt;
