-- Immutable migration 0059: native addressed quest-mobile image and stock.
-- IDs are reserved by the admitted birth owner; no AUTO_INCREMENT, backfill,
-- template adoption, or second item-custody catalog is created here.
-- The existing canonical image codec validates row contents.
CREATE TABLE IF NOT EXISTS quest_mobile_native (
    mobile_instance_id BIGINT UNSIGNED NOT NULL,
    mobile_revision BIGINT UNSIGNED NOT NULL,
    stock_revision BIGINT UNSIGNED NOT NULL,
    lifetime_state TINYINT UNSIGNED NOT NULL,
    canonical_image MEDIUMBLOB NOT NULL,
    PRIMARY KEY (mobile_instance_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP PROCEDURE IF EXISTS duris_verify_quest_mobile_native;
DELIMITER //
CREATE PROCEDURE duris_verify_quest_mobile_native()
BEGIN
    IF NOT (
        (SELECT COUNT(*) FROM information_schema.tables
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native'
              AND table_type='BASE TABLE' AND engine='InnoDB' AND auto_increment IS NULL) = 1
        AND (SELECT COUNT(*) FROM information_schema.columns
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native') = 5
        AND (SELECT COUNT(*) FROM information_schema.columns
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native'
              AND is_nullable='NO' AND column_default IS NULL AND extra=''
              AND column_type NOT LIKE '%zerofill%'
              AND ((ordinal_position=1 AND column_name='mobile_instance_id'
                    AND data_type='bigint' AND column_type LIKE '%unsigned%')
                OR (ordinal_position=2 AND column_name='mobile_revision'
                    AND data_type='bigint' AND column_type LIKE '%unsigned%')
                OR (ordinal_position=3 AND column_name='stock_revision'
                    AND data_type='bigint' AND column_type LIKE '%unsigned%')
                OR (ordinal_position=4 AND column_name='lifetime_state'
                    AND data_type='tinyint' AND column_type LIKE '%unsigned%')
                OR (ordinal_position=5 AND column_name='canonical_image'
                    AND data_type='mediumblob'))) = 5
        AND (SELECT COUNT(*) FROM information_schema.statistics
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native') = 1
        AND (SELECT COUNT(*) FROM information_schema.statistics
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native'
              AND index_name='PRIMARY' AND non_unique=0 AND seq_in_index=1
              AND column_name='mobile_instance_id' AND sub_part IS NULL
              AND index_type='BTREE' AND collation='A') = 1
        AND (SELECT COUNT(*) FROM information_schema.table_constraints
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native') = 1
        AND (SELECT COUNT(*) FROM information_schema.table_constraints
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native'
              AND constraint_name='PRIMARY' AND constraint_type='PRIMARY KEY') = 1
        AND (SELECT COUNT(*) FROM information_schema.key_column_usage
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native') = 1
        AND (SELECT COUNT(*) FROM information_schema.key_column_usage
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native'
              AND constraint_name='PRIMARY' AND column_name='mobile_instance_id'
              AND ordinal_position=1 AND referenced_table_name IS NULL) = 1
        AND (SELECT COUNT(*) FROM information_schema.partitions
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native'
              AND partition_name IS NOT NULL) = 0
    ) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='incompatible quest_mobile_native schema';
    END IF;
END//
DELIMITER ;
CALL duris_verify_quest_mobile_native();
DROP PROCEDURE duris_verify_quest_mobile_native;
