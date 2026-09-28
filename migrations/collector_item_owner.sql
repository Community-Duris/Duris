-- Permit the append-only collector custody type (10) in existing ownership tables.
-- Re-runnable: widen an older check, but never narrow later pet custody (11)
-- or a wider authority already installed by an immutable migration.

DROP PROCEDURE IF EXISTS widen_item_owner_types_for_collector;
DELIMITER //
CREATE PROCEDURE widen_item_owner_types_for_collector()
BEGIN
    DECLARE constraint_exists INT DEFAULT 0;
    DECLARE wider_constraint INT DEFAULT 0;

    SELECT COUNT(*) INTO constraint_exists
      FROM information_schema.table_constraints
     WHERE constraint_schema=DATABASE() AND table_name='item_owner_revision'
       AND constraint_name='chk_item_owner_revision_type' AND constraint_type='CHECK';
    SELECT COUNT(*) INTO wider_constraint FROM information_schema.check_constraints
     WHERE constraint_schema=DATABASE() AND constraint_name='chk_item_owner_revision_type'
       AND LOWER(REPLACE(check_clause,CHAR(96),'')) REGEXP
           'owner_type[[:space:]]+between[[:space:]]+1[[:space:]]+and[[:space:]]+(1[1-9]|[2-9][0-9]+|1[0-9]{2,})([^0-9]|$)';
    IF wider_constraint=0 THEN
        IF constraint_exists>0 THEN
            ALTER TABLE item_owner_revision
                DROP CONSTRAINT chk_item_owner_revision_type,
                ADD CONSTRAINT chk_item_owner_revision_type
                    CHECK (owner_type BETWEEN 1 AND 10);
        ELSE
            ALTER TABLE item_owner_revision ADD CONSTRAINT chk_item_owner_revision_type
                CHECK (owner_type BETWEEN 1 AND 10);
        END IF;
    END IF;

    SELECT COUNT(*) INTO constraint_exists
      FROM information_schema.table_constraints
     WHERE constraint_schema=DATABASE() AND table_name='item_current_owner'
       AND constraint_name='chk_item_current_owner_type' AND constraint_type='CHECK';
    SELECT COUNT(*) INTO wider_constraint FROM information_schema.check_constraints
     WHERE constraint_schema=DATABASE() AND constraint_name='chk_item_current_owner_type'
       AND LOWER(REPLACE(check_clause,CHAR(96),'')) REGEXP
           'owner_type[[:space:]]+between[[:space:]]+1[[:space:]]+and[[:space:]]+(1[1-9]|[2-9][0-9]+|1[0-9]{2,})([^0-9]|$)';
    IF wider_constraint=0 THEN
        IF constraint_exists>0 THEN
            ALTER TABLE item_current_owner
                DROP CONSTRAINT chk_item_current_owner_type,
                ADD CONSTRAINT chk_item_current_owner_type
                    CHECK (owner_type BETWEEN 1 AND 10);
        ELSE
            ALTER TABLE item_current_owner ADD CONSTRAINT chk_item_current_owner_type
                CHECK (owner_type BETWEEN 1 AND 10);
        END IF;
    END IF;

    SELECT COUNT(*) INTO constraint_exists
      FROM information_schema.table_constraints
     WHERE constraint_schema=DATABASE() AND table_name='item_ownership_baseline'
       AND constraint_name='chk_item_baseline_owner_type' AND constraint_type='CHECK';
    SELECT COUNT(*) INTO wider_constraint FROM information_schema.check_constraints
     WHERE constraint_schema=DATABASE() AND constraint_name='chk_item_baseline_owner_type'
       AND LOWER(REPLACE(check_clause,CHAR(96),'')) REGEXP
           'owner_type[[:space:]]+between[[:space:]]+1[[:space:]]+and[[:space:]]+(1[1-9]|[2-9][0-9]+|1[0-9]{2,})([^0-9]|$)';
    IF wider_constraint=0 THEN
        IF constraint_exists>0 THEN
            ALTER TABLE item_ownership_baseline
                DROP CONSTRAINT chk_item_baseline_owner_type,
                ADD CONSTRAINT chk_item_baseline_owner_type
                    CHECK (owner_type BETWEEN 1 AND 10);
        ELSE
            ALTER TABLE item_ownership_baseline ADD CONSTRAINT chk_item_baseline_owner_type
                CHECK (owner_type BETWEEN 1 AND 10);
        END IF;
    END IF;
END//
DELIMITER ;
CALL widen_item_owner_types_for_collector();
DROP PROCEDURE widen_item_owner_types_for_collector;
