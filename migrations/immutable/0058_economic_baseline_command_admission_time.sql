-- Add only the original typed command admission timestamp to retained witnesses.
-- Legacy rows remain NULL; no timestamp is inferred from receipts or clocks.
-- Column and CHECK are one ALTER. A preexisting divergent/partial shape refuses;
-- the immutable verifier also checks every original baseline metadata field.
SET @baseline_admission_sql = IF(
    (SELECT COUNT(*) FROM information_schema.columns
     WHERE table_schema=DATABASE() AND table_name='economic_baseline_witness'
       AND column_name='command_accepted_at_usec')=0,
    'ALTER TABLE economic_baseline_witness ADD COLUMN command_accepted_at_usec BIGINT UNSIGNED NULL DEFAULT NULL AFTER canonical_witness, ADD CONSTRAINT ck_economic_baseline_command_accepted_at_usec CHECK (command_accepted_at_usec IS NULL OR command_accepted_at_usec>0)',
    IF((SELECT COUNT(*) FROM information_schema.columns
     WHERE table_schema=DATABASE() AND table_name='economic_baseline_witness'
       AND BINARY column_name=BINARY 'command_accepted_at_usec'
       AND ordinal_position=10 AND data_type='bigint'
       AND column_type IN ('bigint unsigned','bigint(20) unsigned')
       AND is_nullable='YES' AND numeric_precision=20 AND numeric_scale=0
       AND character_maximum_length IS NULL AND datetime_precision IS NULL
       AND (column_default IS NULL OR
            (LOCATE('MariaDB',VERSION())>0 AND BINARY column_default=BINARY 'NULL'))
       AND extra='')=1 AND (SELECT COUNT(*) FROM information_schema.table_constraints t
     JOIN information_schema.check_constraints c
       ON c.constraint_schema=t.constraint_schema AND c.constraint_name=t.constraint_name
     WHERE t.constraint_schema=DATABASE() AND t.table_name='economic_baseline_witness'
       AND BINARY t.constraint_name=BINARY 'ck_economic_baseline_command_accepted_at_usec'
       AND t.constraint_type='CHECK' AND LOWER(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(c.check_clause,CHAR(96),''),' ',''),CHAR(9),''),CHAR(10),''),CHAR(13),''),'(',''),')',''))='command_accepted_at_usecisnullorcommand_accepted_at_usec>0')=1,
       'SELECT 1',
       'DURIS_0058_REFUSE_DIVERGENT_BASELINE_ADMISSION_SHAPE'));
-- The deliberate invalid statement above raises at PREPARE before mutation on
-- a divergent retry; it creates no helper routine/table or inferred row value.
PREPARE baseline_admission_stmt FROM @baseline_admission_sql;
EXECUTE baseline_admission_stmt;
DEALLOCATE PREPARE baseline_admission_stmt;

-- MySQL exposes CHECK enforcement independently; MariaDB does not expose that
-- column. Prepare only the selected engine's query, without altering enforcement.
SET @baseline_admission_enforcement_sql = IF(LOCATE('MariaDB',VERSION())>0,
    'SELECT 1 INTO @baseline_admission_enforced',
    'SELECT COUNT(*) INTO @baseline_admission_enforced FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name=''economic_baseline_witness'' AND constraint_name=''ck_economic_baseline_command_accepted_at_usec'' AND constraint_type=''CHECK'' AND enforced=''YES''');
PREPARE baseline_admission_stmt FROM @baseline_admission_enforcement_sql;
EXECUTE baseline_admission_stmt;
DEALLOCATE PREPARE baseline_admission_stmt;
SET @baseline_admission_sql = IF(@baseline_admission_enforced=1,
    'SELECT 1', 'DURIS_0058_REFUSE_UNENFORCED_BASELINE_ADMISSION_CHECK');
PREPARE baseline_admission_stmt FROM @baseline_admission_sql;
EXECUTE baseline_admission_stmt;
DEALLOCATE PREPARE baseline_admission_stmt;
