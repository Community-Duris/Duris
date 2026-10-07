-- PRIVATE successor: published constructor evidence under the native lifetime owner.
-- Existing native images, UID/custody/source catalogs and sealed0059 are unchanged.
-- No historical constructor evidence is backfilled and no admission is activated.
CREATE TABLE IF NOT EXISTS quest_mobile_native_birth_origin (
    mobile_instance_id BIGINT UNSIGNED NOT NULL,
    birth_operation BINARY(16) NOT NULL,
    publication_revision BIGINT UNSIGNED NOT NULL,
    canonical_origin LONGBLOB NOT NULL,
    PRIMARY KEY (mobile_instance_id),
    UNIQUE KEY uq_native_birth_origin_operation (birth_operation),
    CONSTRAINT fk_native_birth_origin_lifetime
        FOREIGN KEY (mobile_instance_id) REFERENCES quest_mobile_native(mobile_instance_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT fk_native_birth_origin_inbox
        FOREIGN KEY (birth_operation) REFERENCES critical_operation_inbox(operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT ck_native_birth_origin_bound CHECK (
        mobile_instance_id>0 AND publication_revision>0
        AND birth_operation<>0x00000000000000000000000000000000
        AND OCTET_LENGTH(canonical_origin)>0
        AND OCTET_LENGTH(canonical_origin)<=33554432)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP PROCEDURE IF EXISTS duris_verify_native_birth_origin;
DELIMITER //
CREATE PROCEDURE duris_verify_native_birth_origin()
BEGIN
    IF NOT (
        (SELECT COUNT(*) FROM information_schema.tables
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin'
              AND table_type='BASE TABLE' AND engine='InnoDB' AND auto_increment IS NULL)=1
        AND (SELECT COUNT(*) FROM information_schema.columns
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin')=4
        AND (SELECT COUNT(*) FROM information_schema.columns
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin'
              AND is_nullable='NO' AND column_default IS NULL AND extra=''
              AND ((ordinal_position IN (1,3) AND numeric_precision=20 AND numeric_scale=0
                    AND column_name=IF(ordinal_position=1,'mobile_instance_id','publication_revision')
                    AND data_type='bigint' AND column_type IN ('bigint unsigned','bigint(20) unsigned'))
                OR (ordinal_position=2 AND column_name='birth_operation' AND data_type='binary'
                    AND character_maximum_length=16 AND character_octet_length=16)
                OR (ordinal_position=4 AND column_name='canonical_origin' AND data_type='longblob')))=4
        AND (SELECT COUNT(*) FROM information_schema.statistics
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin')=2
        AND (SELECT COUNT(*) FROM information_schema.statistics
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin'
              AND non_unique=0 AND seq_in_index=1 AND sub_part IS NULL
              AND index_type='BTREE' AND collation='A'
              AND ((index_name='PRIMARY' AND column_name='mobile_instance_id')
                OR (index_name='uq_native_birth_origin_operation' AND column_name='birth_operation')))=2
        AND (SELECT COUNT(*) FROM information_schema.table_constraints
            WHERE constraint_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin')=5
        AND (SELECT COUNT(*) FROM information_schema.table_constraints
            WHERE constraint_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin'
              AND ((constraint_name='PRIMARY' AND constraint_type='PRIMARY KEY')
                OR (constraint_name='uq_native_birth_origin_operation' AND constraint_type='UNIQUE')
                OR (constraint_name IN ('fk_native_birth_origin_lifetime','fk_native_birth_origin_inbox')
                    AND constraint_type='FOREIGN KEY')
                OR (constraint_name='ck_native_birth_origin_bound' AND constraint_type='CHECK')))=5
        AND (SELECT COUNT(*) FROM information_schema.key_column_usage
            WHERE constraint_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin')=4
        AND (SELECT COUNT(*) FROM information_schema.key_column_usage
            WHERE constraint_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin'
              AND ordinal_position=1
              AND ((constraint_name='PRIMARY' AND column_name='mobile_instance_id'
                    AND referenced_table_name IS NULL)
                OR (constraint_name='uq_native_birth_origin_operation' AND column_name='birth_operation'
                    AND referenced_table_name IS NULL)
                OR (constraint_name='fk_native_birth_origin_lifetime' AND column_name='mobile_instance_id'
                    AND referenced_table_schema=DATABASE() AND referenced_table_name='quest_mobile_native'
                    AND referenced_column_name='mobile_instance_id' AND position_in_unique_constraint=1)
                OR (constraint_name='fk_native_birth_origin_inbox' AND column_name='birth_operation'
                    AND referenced_table_schema=DATABASE() AND referenced_table_name='critical_operation_inbox'
                    AND referenced_column_name='operation_id' AND position_in_unique_constraint=1)))=4
        AND (SELECT COUNT(*) FROM information_schema.referential_constraints
            WHERE constraint_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin'
              AND unique_constraint_schema=DATABASE() AND unique_constraint_name='PRIMARY'
              AND update_rule='RESTRICT' AND delete_rule='RESTRICT'
              AND ((constraint_name='fk_native_birth_origin_lifetime' AND referenced_table_name='quest_mobile_native')
                OR (constraint_name='fk_native_birth_origin_inbox' AND referenced_table_name='critical_operation_inbox')))=2
        AND (SELECT COUNT(*) FROM information_schema.check_constraints
            WHERE constraint_schema=DATABASE() AND constraint_name='ck_native_birth_origin_bound'
              AND REPLACE(LOWER(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(check_clause,
                  CHAR(96),''),' ',''),CHAR(9),''),CHAR(10),''),CHAR(13),''),'(',''),')','')),
                  'octet_length','length')=
                  'mobile_instance_id>0andpublication_revision>0andbirth_operation<>0x00000000000000000000000000000000andlengthcanonical_origin>0andlengthcanonical_origin<=33554432')=1
        AND (SELECT COUNT(*) FROM information_schema.partitions
            WHERE table_schema=DATABASE() AND table_name='quest_mobile_native_birth_origin'
              AND partition_name IS NOT NULL)=0
    ) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='incompatible native birth origin schema';
    END IF;
END//
DELIMITER ;
CALL duris_verify_native_birth_origin();
DROP PROCEDURE duris_verify_native_birth_origin;
