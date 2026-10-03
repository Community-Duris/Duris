-- Exact immutable payloads for narrowly admitted, already-owned room drops.
-- Custody/topology remain solely in item_current_owner. No legacy row adoption,
-- baseline, activation, projection repair, or prior receipt rewrite occurs here.
CREATE TABLE IF NOT EXISTS sql_room_item_payload (
    item_uid BIGINT UNSIGNED NOT NULL,
    item_revision BIGINT UNSIGNED NOT NULL,
    payload_version SMALLINT UNSIGNED NOT NULL,
    operation_id BINARY(16) NOT NULL,
    season_epoch BIGINT UNSIGNED NOT NULL,
    payload MEDIUMBLOB NOT NULL,
    PRIMARY KEY (item_uid,item_revision),
    KEY idx_sql_room_item_operation (operation_id),
    CONSTRAINT ck_sql_room_item_payload CHECK
        (item_uid > 0 AND item_revision > 0 AND payload_version = 1 AND season_epoch > 0
         AND LENGTH(payload) > 0 AND LENGTH(payload) <= 131072),
    CONSTRAINT fk_sql_room_item_operation FOREIGN KEY (operation_id)
        REFERENCES critical_operation_inbox(operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP PROCEDURE IF EXISTS duris_verify_sql_room_item_payload;
DELIMITER //
CREATE PROCEDURE duris_verify_sql_room_item_payload()
BEGIN
    IF (SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE()
        AND table_name='sql_room_item_payload' AND engine='InnoDB') <> 1 OR
       (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
        AND table_name='sql_room_item_payload') <> 6 OR
       (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
        AND table_name='sql_room_item_payload' AND is_nullable='NO' AND
        ((ordinal_position=1 AND column_name='item_uid' AND data_type='bigint' AND column_type LIKE '%unsigned%') OR
         (ordinal_position=2 AND column_name='item_revision' AND data_type='bigint' AND column_type LIKE '%unsigned%') OR
         (ordinal_position=3 AND column_name='payload_version' AND data_type='smallint' AND column_type LIKE '%unsigned%') OR
         (ordinal_position=4 AND column_name='operation_id' AND data_type='binary' AND character_octet_length=16) OR
         (ordinal_position=5 AND column_name='season_epoch' AND data_type='bigint' AND column_type LIKE '%unsigned%') OR
         (ordinal_position=6 AND column_name='payload' AND data_type='mediumblob'))) <> 6 OR
       (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE()
        AND table_name='sql_room_item_payload') <> 3 OR
       (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE()
        AND table_name='sql_room_item_payload' AND non_unique=0 AND index_name='PRIMARY'
        AND ((seq_in_index=1 AND column_name='item_uid') OR
             (seq_in_index=2 AND column_name='item_revision'))) <> 2 OR
       (SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE()
        AND table_name='sql_room_item_payload' AND non_unique=1
        AND index_name='idx_sql_room_item_operation' AND seq_in_index=1
        AND column_name='operation_id') <> 1 OR
       (SELECT COUNT(*) FROM information_schema.table_constraints WHERE table_schema=DATABASE()
        AND table_name='sql_room_item_payload' AND constraint_type='FOREIGN KEY') <> 1 OR
       (SELECT COUNT(*) FROM information_schema.table_constraints WHERE table_schema=DATABASE()
        AND table_name='sql_room_item_payload' AND constraint_type='CHECK') <> 1 OR
       (SELECT COUNT(*) FROM information_schema.check_constraints c
        JOIN information_schema.table_constraints t ON t.constraint_schema=c.constraint_schema
          AND t.constraint_name=c.constraint_name
        WHERE t.table_schema=DATABASE() AND t.table_name='sql_room_item_payload'
          AND t.constraint_type='CHECK' AND t.constraint_name='ck_sql_room_item_payload'
          AND REPLACE(LOWER(REPLACE(REPLACE(REPLACE(REPLACE(c.check_clause,' ',''),'`',''),'(',''),')','')),'octet_length','length') =
          'item_uid>0anditem_revision>0andpayload_version=1andseason_epoch>0andlengthpayload>0andlengthpayload<=131072') <> 1 OR
       (SELECT COUNT(*) FROM information_schema.key_column_usage k
        JOIN information_schema.referential_constraints r
          ON r.constraint_schema=k.constraint_schema AND r.constraint_name=k.constraint_name
        WHERE k.table_schema=DATABASE() AND k.table_name='sql_room_item_payload'
          AND k.constraint_name='fk_sql_room_item_operation' AND k.column_name='operation_id'
          AND k.referenced_table_name='critical_operation_inbox' AND k.referenced_column_name='operation_id'
          AND r.update_rule='RESTRICT' AND r.delete_rule='RESTRICT') <> 1 THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='incompatible sql_room_item_payload schema';
    END IF;
END//
DELIMITER ;
CALL duris_verify_sql_room_item_payload();
DROP PROCEDURE duris_verify_sql_room_item_payload;
