-- Persist secondary item flags and the complete dynamic-affect tuple for player items.
-- NULL remains the legacy-row form; the versioned ASCII-hex payload is validated by
-- the player snapshot codec before it is applied to an in-memory object.
SET @item_properties_exists = (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA=DATABASE()
    AND TABLE_NAME='player_items'
    AND COLUMN_NAME='item_properties'
);
SET @item_properties_sql = IF(
  @item_properties_exists=0,
  'ALTER TABLE player_items ADD COLUMN item_properties MEDIUMTEXT CHARACTER SET ascii DEFAULT NULL',
  'SELECT 1'
);
PREPARE item_properties_stmt FROM @item_properties_sql;
EXECUTE item_properties_stmt;
DEALLOCATE PREPARE item_properties_stmt;
