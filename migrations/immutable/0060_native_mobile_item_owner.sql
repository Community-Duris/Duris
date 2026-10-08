-- Immutable migration 0060: admit native_mobile (owner type 12) custody.
-- Amend only the three named ranges installed by immutable 0028. Existing
-- columns, NULL distinctions, keys, other checks and populated rows are retained.
-- Accept exact 11/12 shapes so retry after any completed ALTER remains valid.
DROP PROCEDURE IF EXISTS duris_verify_native_mobile_owner_types;
DELIMITER //
CREATE PROCEDURE duris_verify_native_mobile_owner_types(IN allow_previous BOOLEAN)
BEGIN
    IF NOT (
        (SELECT COUNT(*) FROM information_schema.columns
            WHERE table_schema=DATABASE()
              AND table_name IN ('item_owner_revision', 'item_current_owner', 'item_ownership_baseline')
              AND column_name='owner_type' AND data_type='tinyint'
              AND column_type LIKE '%unsigned%' AND is_nullable='NO') = 3
        AND (SELECT COUNT(*) FROM information_schema.table_constraints t
            JOIN information_schema.check_constraints c
              ON c.constraint_schema=t.constraint_schema AND c.constraint_name=t.constraint_name
            WHERE t.table_schema=DATABASE() AND t.constraint_schema=DATABASE()
              AND t.constraint_type='CHECK'
              AND ((t.table_name='item_owner_revision' AND t.constraint_name='chk_item_owner_revision_type')
                OR (t.table_name='item_current_owner' AND t.constraint_name='chk_item_current_owner_type')
                OR (t.table_name='item_ownership_baseline' AND t.constraint_name='chk_item_baseline_owner_type'))
              AND (LOWER(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(c.check_clause,
                CHAR(96), ''), '(', ''), ')', ''), ' ', ''), CHAR(9), ''), CHAR(10), ''), CHAR(13), '')) = 'owner_typebetween1and12'
              OR (allow_previous AND LOWER(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(c.check_clause,
                CHAR(96), ''), '(', ''), ')', ''), ' ', ''), CHAR(9), ''), CHAR(10), ''), CHAR(13), '')) = 'owner_typebetween1and11'))) = 3
    ) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='incompatible native mobile item owner constraints';
    END IF;
    -- Use a prepared query so engine-specific enforcement metadata is never
    -- parsed on the other engine. Disabled enforcement is refused, not enabled.
    SET @duris_native_owner_enforced = NULL;
    SET @duris_native_owner_enforcement_sql = IF(LOCATE('MariaDB', VERSION()) > 0,
        'SELECT @@SESSION.check_constraint_checks INTO @duris_native_owner_enforced',
        'SELECT (COUNT(*) = 3) INTO @duris_native_owner_enforced FROM information_schema.table_constraints t WHERE t.table_schema=DATABASE() AND t.constraint_schema=DATABASE() AND t.constraint_type=''CHECK'' AND t.enforced=''YES'' AND ((t.table_name=''item_owner_revision'' AND t.constraint_name=''chk_item_owner_revision_type'') OR (t.table_name=''item_current_owner'' AND t.constraint_name=''chk_item_current_owner_type'') OR (t.table_name=''item_ownership_baseline'' AND t.constraint_name=''chk_item_baseline_owner_type''))');
    PREPARE native_owner_enforcement_stmt FROM @duris_native_owner_enforcement_sql;
    EXECUTE native_owner_enforcement_stmt;
    DEALLOCATE PREPARE native_owner_enforcement_stmt;
    IF @duris_native_owner_enforced IS NULL OR @duris_native_owner_enforced <> 1 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='native mobile item owner checks are not enforced';
    END IF;
END//
DELIMITER ;
CALL duris_verify_native_mobile_owner_types(TRUE);

-- Drop and replacement share each ALTER; no standalone unconstrained phase.
ALTER TABLE item_owner_revision
    DROP CONSTRAINT chk_item_owner_revision_type,
    ADD CONSTRAINT chk_item_owner_revision_type CHECK (owner_type BETWEEN 1 AND 12);
ALTER TABLE item_current_owner
    DROP CONSTRAINT chk_item_current_owner_type,
    ADD CONSTRAINT chk_item_current_owner_type CHECK (owner_type BETWEEN 1 AND 12);
ALTER TABLE item_ownership_baseline
    DROP CONSTRAINT chk_item_baseline_owner_type,
    ADD CONSTRAINT chk_item_baseline_owner_type CHECK (owner_type BETWEEN 1 AND 12);

CALL duris_verify_native_mobile_owner_types(FALSE);
DROP PROCEDURE duris_verify_native_mobile_owner_types;
