-- PRIVATE additive reset issuance origin: exact original command plus typed48.
-- Insert in the original successful SQL root before COMMIT. No backfill.
-- Original admission stores only issuance evidence. A NULL terminal remains unknown.
-- Retain the original completed ZRR1 BODY before journal retirement; no backfill.
-- Current custody may move or retire; only the original inbox is a FK owner.
CREATE TABLE IF NOT EXISTS zone_reset_item_birth_origin (
    root_item_uid BIGINT UNSIGNED NOT NULL,
    birth_operation BINARY(16) NOT NULL,
    room_revision BIGINT UNSIGNED NOT NULL,
    canonical_origin LONGBLOB NOT NULL,
    PRIMARY KEY (root_item_uid),
    UNIQUE KEY uq_zone_reset_item_origin_operation (birth_operation),
    CONSTRAINT fk_zone_reset_item_origin_inbox
        FOREIGN KEY (birth_operation) REFERENCES critical_operation_inbox(operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT ck_zone_reset_item_origin_bound CHECK (
        root_item_uid>0 AND room_revision>0
        AND birth_operation<>0x00000000000000000000000000000000
        AND OCTET_LENGTH(canonical_origin)>0
        AND OCTET_LENGTH(canonical_origin)<=524352)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP PROCEDURE IF EXISTS duris_verify_zone_reset_item_origin;
DELIMITER //
CREATE PROCEDURE duris_verify_zone_reset_item_origin()
BEGIN
    -- Only the exact unpublished four-column predecessor may be extended.
    IF (
        (SELECT COUNT(*) FROM information_schema.tables
            WHERE table_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND table_type='BASE TABLE' AND engine='InnoDB' AND auto_increment IS NULL)=1
        AND (SELECT COUNT(*) FROM information_schema.columns
            WHERE table_schema=DATABASE() AND table_name='zone_reset_item_birth_origin')=4
        AND (SELECT COUNT(*) FROM information_schema.columns
            WHERE table_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND is_nullable='NO' AND column_default IS NULL AND extra=''
              AND ((ordinal_position IN (1,3) AND numeric_precision=20 AND numeric_scale=0
                    AND column_name=IF(ordinal_position=1,'root_item_uid','room_revision')
                    AND data_type='bigint' AND column_type IN ('bigint unsigned','bigint(20) unsigned'))
                OR (ordinal_position=2 AND column_name='birth_operation' AND data_type='binary'
                    AND character_maximum_length=16 AND character_octet_length=16)
                OR (ordinal_position=4 AND column_name='canonical_origin' AND data_type='longblob')))=4
        AND (SELECT COUNT(*) FROM information_schema.statistics
            WHERE table_schema=DATABASE() AND table_name='zone_reset_item_birth_origin')=2
        AND (SELECT COUNT(*) FROM information_schema.statistics
            WHERE table_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND non_unique=0 AND seq_in_index=1 AND sub_part IS NULL
              AND index_type='BTREE' AND collation='A'
              AND ((index_name='PRIMARY' AND column_name='root_item_uid')
                OR (index_name='uq_zone_reset_item_origin_operation' AND column_name='birth_operation')))=2
        AND (SELECT COUNT(*) FROM information_schema.table_constraints
            WHERE constraint_schema=DATABASE() AND table_name='zone_reset_item_birth_origin')=4
        AND (SELECT COUNT(*) FROM information_schema.table_constraints
            WHERE constraint_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND ((constraint_name='PRIMARY' AND constraint_type='PRIMARY KEY')
                OR (constraint_name='uq_zone_reset_item_origin_operation' AND constraint_type='UNIQUE')
                OR (constraint_name='fk_zone_reset_item_origin_inbox'
                    AND constraint_type='FOREIGN KEY')
                OR (constraint_name='ck_zone_reset_item_origin_bound' AND constraint_type='CHECK')))=4
        AND (SELECT COUNT(*) FROM information_schema.key_column_usage
            WHERE constraint_schema=DATABASE() AND table_name='zone_reset_item_birth_origin')=3
        AND (SELECT COUNT(*) FROM information_schema.key_column_usage
            WHERE constraint_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND ordinal_position=1
              AND ((constraint_name='PRIMARY' AND column_name='root_item_uid'
                    AND referenced_table_name IS NULL)
                OR (constraint_name='uq_zone_reset_item_origin_operation' AND column_name='birth_operation'
                    AND referenced_table_name IS NULL)
                OR (constraint_name='fk_zone_reset_item_origin_inbox' AND column_name='birth_operation'
                    AND referenced_table_schema=DATABASE() AND referenced_table_name='critical_operation_inbox'
                    AND referenced_column_name='operation_id' AND position_in_unique_constraint=1)))=3
        AND (SELECT COUNT(*) FROM information_schema.referential_constraints
            WHERE constraint_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND unique_constraint_schema=DATABASE() AND unique_constraint_name='PRIMARY'
              AND update_rule='RESTRICT' AND delete_rule='RESTRICT'
              AND constraint_name='fk_zone_reset_item_origin_inbox'
              AND referenced_table_name='critical_operation_inbox')=1
        AND (SELECT COUNT(*) FROM information_schema.check_constraints
            WHERE constraint_schema=DATABASE() AND constraint_name='ck_zone_reset_item_origin_bound'
              AND REPLACE(LOWER(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(check_clause,
                  CHAR(96),''),' ',''),CHAR(9),''),CHAR(10),''),CHAR(13),''),'(',''),')','')),
                  'octet_length','length')=
                  'root_item_uid>0androom_revision>0andbirth_operation<>0x00000000000000000000000000000000andlengthcanonical_origin>0andlengthcanonical_origin<=524352')=1
        AND (SELECT COUNT(*) FROM information_schema.partitions
            WHERE table_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND partition_name IS NOT NULL)=0
    ) THEN
        ALTER TABLE zone_reset_item_birth_origin
            ADD COLUMN terminal_publication_context LONGBLOB NULL,
            ADD CONSTRAINT ck_zone_reset_item_terminal_bound CHECK (
                terminal_publication_context IS NULL OR (
                    OCTET_LENGTH(terminal_publication_context)>0
                    AND OCTET_LENGTH(terminal_publication_context)<=33554432));
    ELSEIF NOT (
        (SELECT COUNT(*) FROM information_schema.tables
            WHERE table_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND table_type='BASE TABLE' AND engine='InnoDB' AND auto_increment IS NULL)=1
        AND (SELECT COUNT(*) FROM information_schema.columns
            WHERE table_schema=DATABASE() AND table_name='zone_reset_item_birth_origin')=5
        AND (SELECT COUNT(*) FROM information_schema.columns
            WHERE table_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND is_nullable='NO' AND column_default IS NULL AND extra=''
              AND ((ordinal_position IN (1,3) AND numeric_precision=20 AND numeric_scale=0
                    AND column_name=IF(ordinal_position=1,'root_item_uid','room_revision')
                    AND data_type='bigint' AND column_type IN ('bigint unsigned','bigint(20) unsigned'))
                OR (ordinal_position=2 AND column_name='birth_operation' AND data_type='binary'
                    AND character_maximum_length=16 AND character_octet_length=16)
                OR (ordinal_position=4 AND column_name='canonical_origin' AND data_type='longblob')))=4
        AND (SELECT COUNT(*) FROM information_schema.columns
            WHERE table_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND ordinal_position=5 AND column_name='terminal_publication_context'
              AND data_type='longblob' AND is_nullable='YES'
              AND (column_default IS NULL OR (VERSION() LIKE '10.11.%MariaDB%'
                   AND BINARY column_default = BINARY 'NULL')) AND extra='')=1
        AND (SELECT COUNT(*) FROM information_schema.statistics
            WHERE table_schema=DATABASE() AND table_name='zone_reset_item_birth_origin')=2
        AND (SELECT COUNT(*) FROM information_schema.statistics
            WHERE table_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND non_unique=0 AND seq_in_index=1 AND sub_part IS NULL
              AND index_type='BTREE' AND collation='A'
              AND ((index_name='PRIMARY' AND column_name='root_item_uid')
                OR (index_name='uq_zone_reset_item_origin_operation' AND column_name='birth_operation')))=2
        AND (SELECT COUNT(*) FROM information_schema.table_constraints
            WHERE constraint_schema=DATABASE() AND table_name='zone_reset_item_birth_origin')=5
        AND (SELECT COUNT(*) FROM information_schema.table_constraints
            WHERE constraint_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND ((constraint_name='PRIMARY' AND constraint_type='PRIMARY KEY')
                OR (constraint_name='uq_zone_reset_item_origin_operation' AND constraint_type='UNIQUE')
                OR (constraint_name='fk_zone_reset_item_origin_inbox'
                    AND constraint_type='FOREIGN KEY')
                OR (constraint_name='ck_zone_reset_item_origin_bound' AND constraint_type='CHECK')
                OR (constraint_name='ck_zone_reset_item_terminal_bound' AND constraint_type='CHECK')))=5
        AND (SELECT COUNT(*) FROM information_schema.key_column_usage
            WHERE constraint_schema=DATABASE() AND table_name='zone_reset_item_birth_origin')=3
        AND (SELECT COUNT(*) FROM information_schema.key_column_usage
            WHERE constraint_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND ordinal_position=1
              AND ((constraint_name='PRIMARY' AND column_name='root_item_uid'
                    AND referenced_table_name IS NULL)
                OR (constraint_name='uq_zone_reset_item_origin_operation' AND column_name='birth_operation'
                    AND referenced_table_name IS NULL)
                OR (constraint_name='fk_zone_reset_item_origin_inbox' AND column_name='birth_operation'
                    AND referenced_table_schema=DATABASE() AND referenced_table_name='critical_operation_inbox'
                    AND referenced_column_name='operation_id' AND position_in_unique_constraint=1)))=3
        AND (SELECT COUNT(*) FROM information_schema.referential_constraints
            WHERE constraint_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND unique_constraint_schema=DATABASE() AND unique_constraint_name='PRIMARY'
              AND update_rule='RESTRICT' AND delete_rule='RESTRICT'
              AND constraint_name='fk_zone_reset_item_origin_inbox'
              AND referenced_table_name='critical_operation_inbox')=1
        AND (SELECT COUNT(*) FROM information_schema.check_constraints
            WHERE constraint_schema=DATABASE() AND constraint_name='ck_zone_reset_item_origin_bound'
              AND REPLACE(LOWER(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(check_clause,
                  CHAR(96),''),' ',''),CHAR(9),''),CHAR(10),''),CHAR(13),''),'(',''),')','')),
                  'octet_length','length')=
                  'root_item_uid>0androom_revision>0andbirth_operation<>0x00000000000000000000000000000000andlengthcanonical_origin>0andlengthcanonical_origin<=524352')=1
        AND (SELECT COUNT(*) FROM information_schema.check_constraints
            WHERE constraint_schema=DATABASE() AND constraint_name='ck_zone_reset_item_terminal_bound'
              AND REPLACE(LOWER(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(check_clause,
                  CHAR(96),''),' ',''),CHAR(9),''),CHAR(10),''),CHAR(13),''),'(',''),')','')),
                  'octet_length','length')=
                  'terminal_publication_contextisnullorlengthterminal_publication_context>0andlengthterminal_publication_context<=33554432')=1
        AND (SELECT COUNT(*) FROM information_schema.partitions
            WHERE table_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND partition_name IS NOT NULL)=0
    ) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='incompatible zone reset item origin schema';
    END IF;
    IF NOT (
        (SELECT COUNT(*) FROM information_schema.tables
            WHERE table_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND table_type='BASE TABLE' AND engine='InnoDB' AND auto_increment IS NULL)=1
        AND (SELECT COUNT(*) FROM information_schema.columns
            WHERE table_schema=DATABASE() AND table_name='zone_reset_item_birth_origin')=5
        AND (SELECT COUNT(*) FROM information_schema.columns
            WHERE table_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND is_nullable='NO' AND column_default IS NULL AND extra=''
              AND ((ordinal_position IN (1,3) AND numeric_precision=20 AND numeric_scale=0
                    AND column_name=IF(ordinal_position=1,'root_item_uid','room_revision')
                    AND data_type='bigint' AND column_type IN ('bigint unsigned','bigint(20) unsigned'))
                OR (ordinal_position=2 AND column_name='birth_operation' AND data_type='binary'
                    AND character_maximum_length=16 AND character_octet_length=16)
                OR (ordinal_position=4 AND column_name='canonical_origin' AND data_type='longblob')))=4
        AND (SELECT COUNT(*) FROM information_schema.columns
            WHERE table_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND ordinal_position=5 AND column_name='terminal_publication_context'
              AND data_type='longblob' AND is_nullable='YES'
              AND (column_default IS NULL OR (VERSION() LIKE '10.11.%MariaDB%'
                   AND BINARY column_default = BINARY 'NULL')) AND extra='')=1
        AND (SELECT COUNT(*) FROM information_schema.statistics
            WHERE table_schema=DATABASE() AND table_name='zone_reset_item_birth_origin')=2
        AND (SELECT COUNT(*) FROM information_schema.statistics
            WHERE table_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND non_unique=0 AND seq_in_index=1 AND sub_part IS NULL
              AND index_type='BTREE' AND collation='A'
              AND ((index_name='PRIMARY' AND column_name='root_item_uid')
                OR (index_name='uq_zone_reset_item_origin_operation' AND column_name='birth_operation')))=2
        AND (SELECT COUNT(*) FROM information_schema.table_constraints
            WHERE constraint_schema=DATABASE() AND table_name='zone_reset_item_birth_origin')=5
        AND (SELECT COUNT(*) FROM information_schema.table_constraints
            WHERE constraint_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND ((constraint_name='PRIMARY' AND constraint_type='PRIMARY KEY')
                OR (constraint_name='uq_zone_reset_item_origin_operation' AND constraint_type='UNIQUE')
                OR (constraint_name='fk_zone_reset_item_origin_inbox'
                    AND constraint_type='FOREIGN KEY')
                OR (constraint_name='ck_zone_reset_item_origin_bound' AND constraint_type='CHECK')
                OR (constraint_name='ck_zone_reset_item_terminal_bound' AND constraint_type='CHECK')))=5
        AND (SELECT COUNT(*) FROM information_schema.key_column_usage
            WHERE constraint_schema=DATABASE() AND table_name='zone_reset_item_birth_origin')=3
        AND (SELECT COUNT(*) FROM information_schema.key_column_usage
            WHERE constraint_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND ordinal_position=1
              AND ((constraint_name='PRIMARY' AND column_name='root_item_uid'
                    AND referenced_table_name IS NULL)
                OR (constraint_name='uq_zone_reset_item_origin_operation' AND column_name='birth_operation'
                    AND referenced_table_name IS NULL)
                OR (constraint_name='fk_zone_reset_item_origin_inbox' AND column_name='birth_operation'
                    AND referenced_table_schema=DATABASE() AND referenced_table_name='critical_operation_inbox'
                    AND referenced_column_name='operation_id' AND position_in_unique_constraint=1)))=3
        AND (SELECT COUNT(*) FROM information_schema.referential_constraints
            WHERE constraint_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND unique_constraint_schema=DATABASE() AND unique_constraint_name='PRIMARY'
              AND update_rule='RESTRICT' AND delete_rule='RESTRICT'
              AND constraint_name='fk_zone_reset_item_origin_inbox'
              AND referenced_table_name='critical_operation_inbox')=1
        AND (SELECT COUNT(*) FROM information_schema.check_constraints
            WHERE constraint_schema=DATABASE() AND constraint_name='ck_zone_reset_item_origin_bound'
              AND REPLACE(LOWER(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(check_clause,
                  CHAR(96),''),' ',''),CHAR(9),''),CHAR(10),''),CHAR(13),''),'(',''),')','')),
                  'octet_length','length')=
                  'root_item_uid>0androom_revision>0andbirth_operation<>0x00000000000000000000000000000000andlengthcanonical_origin>0andlengthcanonical_origin<=524352')=1
        AND (SELECT COUNT(*) FROM information_schema.check_constraints
            WHERE constraint_schema=DATABASE() AND constraint_name='ck_zone_reset_item_terminal_bound'
              AND REPLACE(LOWER(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(check_clause,
                  CHAR(96),''),' ',''),CHAR(9),''),CHAR(10),''),CHAR(13),''),'(',''),')','')),
                  'octet_length','length')=
                  'terminal_publication_contextisnullorlengthterminal_publication_context>0andlengthterminal_publication_context<=33554432')=1
        AND (SELECT COUNT(*) FROM information_schema.partitions
            WHERE table_schema=DATABASE() AND table_name='zone_reset_item_birth_origin'
              AND partition_name IS NOT NULL)=0
    ) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='incompatible zone reset item terminal schema';
    END IF;
END//
DELIMITER ;
CALL duris_verify_zone_reset_item_origin();
DROP PROCEDURE duris_verify_zone_reset_item_origin;
