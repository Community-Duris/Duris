-- Preserve the equipment position in the existing item custody authority and
-- the immutable transition row. Zero means carried or outside player equipment.
SET @item_equipment_slot_exists = (
    SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='item_current_owner'
      AND column_name='equipment_slot'
);
SET @item_equipment_slot_sql = IF(@item_equipment_slot_exists=0,
    'ALTER TABLE item_current_owner ADD COLUMN equipment_slot SMALLINT UNSIGNED NOT NULL DEFAULT 0',
    'SELECT 1');
PREPARE item_equipment_slot_stmt FROM @item_equipment_slot_sql;
EXECUTE item_equipment_slot_stmt;
DEALLOCATE PREPARE item_equipment_slot_stmt;

SET @baseline_equipment_slot_exists = (
    SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='item_ownership_baseline'
      AND column_name='equipment_slot'
);
SET @baseline_equipment_slot_sql = IF(@baseline_equipment_slot_exists=0,
    'ALTER TABLE item_ownership_baseline ADD COLUMN equipment_slot SMALLINT UNSIGNED NOT NULL DEFAULT 0',
    'SELECT 1');
PREPARE baseline_equipment_slot_stmt FROM @baseline_equipment_slot_sql;
EXECUTE baseline_equipment_slot_stmt;
DEALLOCATE PREPARE baseline_equipment_slot_stmt;

SET @ledger_from_equipment_slot_exists = (
    SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='item_ownership_ledger'
      AND column_name='from_equipment_slot'
);
SET @ledger_from_equipment_slot_sql = IF(@ledger_from_equipment_slot_exists=0,
    'ALTER TABLE item_ownership_ledger ADD COLUMN from_equipment_slot SMALLINT UNSIGNED NOT NULL DEFAULT 0',
    'SELECT 1');
PREPARE ledger_from_equipment_slot_stmt FROM @ledger_from_equipment_slot_sql;
EXECUTE ledger_from_equipment_slot_stmt;
DEALLOCATE PREPARE ledger_from_equipment_slot_stmt;

SET @ledger_to_equipment_slot_exists = (
    SELECT COUNT(*) FROM information_schema.columns
    WHERE table_schema=DATABASE() AND table_name='item_ownership_ledger'
      AND column_name='to_equipment_slot'
);
SET @ledger_to_equipment_slot_sql = IF(@ledger_to_equipment_slot_exists=0,
    'ALTER TABLE item_ownership_ledger ADD COLUMN to_equipment_slot SMALLINT UNSIGNED NOT NULL DEFAULT 0',
    'SELECT 1');
PREPARE ledger_to_equipment_slot_stmt FROM @ledger_to_equipment_slot_sql;
EXECUTE ledger_to_equipment_slot_stmt;
DEALLOCATE PREPARE ledger_to_equipment_slot_stmt;
