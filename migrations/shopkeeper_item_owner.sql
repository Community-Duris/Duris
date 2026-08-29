-- Permit the append-only shopkeeper custody type (9) in existing ownership tables.
-- Re-runnable: each named constraint is replaced with the same widened predicate.

DROP PROCEDURE IF EXISTS widen_item_owner_types_for_shopkeepers;
DELIMITER //
CREATE PROCEDURE widen_item_owner_types_for_shopkeepers()
BEGIN
    DECLARE constraint_exists INT DEFAULT 0;

    SELECT COUNT(*) INTO constraint_exists
      FROM information_schema.table_constraints
     WHERE constraint_schema=DATABASE() AND table_name='item_owner_revision'
       AND constraint_name='chk_item_owner_revision_type' AND constraint_type='CHECK';
    IF constraint_exists>0 THEN
        ALTER TABLE item_owner_revision DROP CONSTRAINT chk_item_owner_revision_type;
    END IF;
    ALTER TABLE item_owner_revision ADD CONSTRAINT chk_item_owner_revision_type
        CHECK (owner_type BETWEEN 1 AND 9);

    SELECT COUNT(*) INTO constraint_exists
      FROM information_schema.table_constraints
     WHERE constraint_schema=DATABASE() AND table_name='item_current_owner'
       AND constraint_name='chk_item_current_owner_type' AND constraint_type='CHECK';
    IF constraint_exists>0 THEN
        ALTER TABLE item_current_owner DROP CONSTRAINT chk_item_current_owner_type;
    END IF;
    ALTER TABLE item_current_owner ADD CONSTRAINT chk_item_current_owner_type
        CHECK (owner_type BETWEEN 1 AND 9);

    SELECT COUNT(*) INTO constraint_exists
      FROM information_schema.table_constraints
     WHERE constraint_schema=DATABASE() AND table_name='item_ownership_baseline'
       AND constraint_name='chk_item_baseline_owner_type' AND constraint_type='CHECK';
    IF constraint_exists>0 THEN
        ALTER TABLE item_ownership_baseline DROP CONSTRAINT chk_item_baseline_owner_type;
    END IF;
    ALTER TABLE item_ownership_baseline ADD CONSTRAINT chk_item_baseline_owner_type
        CHECK (owner_type BETWEEN 1 AND 9);
END//
DELIMITER ;
CALL widen_item_owner_types_for_shopkeepers();
DROP PROCEDURE widen_item_owner_types_for_shopkeepers;
