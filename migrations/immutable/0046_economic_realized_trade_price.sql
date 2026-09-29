-- Preserve the committed copper price of a shop buy or sale on its accounting root.
SET @economic_realized_price_exists = (
    SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE()
      AND table_name = 'economic_accounting_operation'
      AND column_name = 'realized_price_copper'
);
SET @economic_realized_price_ddl = IF(
    @economic_realized_price_exists = 0,
    'ALTER TABLE economic_accounting_operation '
    'ADD COLUMN realized_price_copper BIGINT NULL DEFAULT NULL AFTER source_event',
    'SELECT 1'
);
PREPARE economic_realized_price_stmt FROM @economic_realized_price_ddl;
EXECUTE economic_realized_price_stmt;
DEALLOCATE PREPARE economic_realized_price_stmt;
