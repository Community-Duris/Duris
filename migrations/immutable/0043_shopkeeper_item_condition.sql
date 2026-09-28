-- Old keeper item snapshots do not prove their runtime condition. Preserve
-- that uncertainty until a successful shopkeeper checkpoint writes the value.
SET @shopkeeper_item_condition_exists = (
    SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema = DATABASE() AND table_name = 'shopkeeper_items'
      AND column_name = 'item_condition'
);
SET @shopkeeper_item_condition_ddl = IF(
    @shopkeeper_item_condition_exists = 0,
    'ALTER TABLE shopkeeper_items ADD COLUMN item_condition SMALLINT NULL DEFAULT NULL AFTER obj_uid',
    'SELECT 1'
);
PREPARE shopkeeper_item_condition_stmt FROM @shopkeeper_item_condition_ddl;
EXECUTE shopkeeper_item_condition_stmt;
DEALLOCATE PREPARE shopkeeper_item_condition_stmt;
