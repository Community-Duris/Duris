-- Normalize archive-derived item_material ordering to the fresh MUD schema.
-- No column is removed or added. Validate all four definitions before ALTERs,
-- and refuse an unexpected type/default/comment rather than rewriting values.
DROP PROCEDURE IF EXISTS duris_archive_material_order;
DELIMITER //
CREATE PROCEDURE duris_archive_material_order(
    IN target_table VARCHAR(64), IN anchor_column VARCHAR(64), IN apply_change TINYINT)
BEGIN
    DECLARE material_count INT DEFAULT 0;
    DECLARE material_position INT DEFAULT 0;
    DECLARE material_data_type VARCHAR(64) DEFAULT '';
    DECLARE material_column_type VARCHAR(64) DEFAULT '';
    DECLARE material_nullable VARCHAR(3) DEFAULT '';
    DECLARE non_null_defaults INT DEFAULT 0;
    DECLARE material_extra VARCHAR(255) DEFAULT '';
    DECLARE material_comment TEXT DEFAULT '';
    DECLARE anchor_count INT DEFAULT 0;
    DECLARE anchor_position INT DEFAULT 0;

    IF NOT ((target_table='corpse_items' AND anchor_column='item_condition') OR
            (target_table='player_pet_items' AND anchor_column='item_condition') OR
            (target_table='shopkeeper_items' AND anchor_column='obj_uid') OR
            (target_table='siege_items' AND anchor_column='updated_at')) OR
       apply_change NOT IN (0,1) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='unknown material-order contract';
    END IF;
    SELECT COUNT(*), COALESCE(MAX(ordinal_position),0),
           COALESCE(MAX(data_type),''), COALESCE(MAX(column_type),''),
           COALESCE(MAX(is_nullable),''),
           COALESCE(SUM(CASE WHEN column_default IS NULL OR
                                  column_default='NULL' THEN 0 ELSE 1 END),0),
           COALESCE(MAX(extra),''), COALESCE(MAX(column_comment),'')
      INTO material_count, material_position, material_data_type,
           material_column_type, material_nullable, non_null_defaults,
           material_extra, material_comment
      FROM information_schema.columns
     WHERE table_schema=DATABASE() AND table_name=target_table
       AND column_name='item_material';
    SELECT COUNT(*), COALESCE(MAX(ordinal_position),0)
      INTO anchor_count, anchor_position
      FROM information_schema.columns
     WHERE table_schema=DATABASE() AND table_name=target_table
       AND column_name=anchor_column;
    IF material_count <> 1 OR anchor_count <> 1 OR
       material_data_type <> 'tinyint' OR
       material_column_type NOT IN ('tinyint','tinyint(4)') OR
       material_nullable <> 'YES' OR non_null_defaults <> 0 OR
       material_extra <> '' OR material_comment <> '' THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='unexpected item_material definition or anchor';
    END IF;
    IF apply_change=1 AND material_position <> anchor_position+1 THEN
        SET @duris_material_order_sql = CONCAT(
            'ALTER TABLE `', target_table,
            '` MODIFY COLUMN `item_material` TINYINT DEFAULT NULL AFTER `',
            anchor_column, '`');
        PREPARE duris_material_order_stmt FROM @duris_material_order_sql;
        EXECUTE duris_material_order_stmt;
        DEALLOCATE PREPARE duris_material_order_stmt;
    END IF;
END //
DELIMITER ;
CALL duris_archive_material_order('corpse_items','item_condition',0);
CALL duris_archive_material_order('player_pet_items','item_condition',0);
CALL duris_archive_material_order('shopkeeper_items','obj_uid',0);
CALL duris_archive_material_order('siege_items','updated_at',0);
CALL duris_archive_material_order('corpse_items','item_condition',1);
CALL duris_archive_material_order('player_pet_items','item_condition',1);
CALL duris_archive_material_order('shopkeeper_items','obj_uid',1);
CALL duris_archive_material_order('siege_items','updated_at',1);
DROP PROCEDURE duris_archive_material_order;
