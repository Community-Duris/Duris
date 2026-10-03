-- Preserve dynamic item state when a player sells an item into shop custody.
SET @shopkeeper_item_properties_exists = (
    SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'shopkeeper_items'
      AND column_name = 'item_properties'
);
SET @shopkeeper_item_properties_ddl = IF(
    @shopkeeper_item_properties_exists = 0,
    'ALTER TABLE shopkeeper_items ADD COLUMN item_properties MEDIUMTEXT CHARACTER SET ascii DEFAULT NULL',
    'SELECT 1'
);
PREPARE shopkeeper_item_properties_stmt FROM @shopkeeper_item_properties_ddl;
EXECUTE shopkeeper_item_properties_stmt;
DEALLOCATE PREPARE shopkeeper_item_properties_stmt;
