-- 0062: additive retained consumption and explicit new baseline origin contract.
-- Original source allocations/legacy whole links and EAB1/EAB2 bytes are unchanged.
-- No origins, balances, mappings, epochs or activation decisions are backfilled.
CREATE TABLE IF NOT EXISTS economic_pending_claim_consumption (
    spending_operation_id BINARY(16) NOT NULL,
    source_operation_id BINARY(16) NOT NULL,
    source_slot SMALLINT UNSIGNED NOT NULL,
    amount BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (spending_operation_id,source_operation_id,source_slot),
    KEY idx_economic_pending_consumption_source (source_operation_id,source_slot),
    CONSTRAINT chk_economic_pending_consumption CHECK (source_slot>0 AND amount>0),
    CONSTRAINT economic_pending_consumption_operation_fk
        FOREIGN KEY (spending_operation_id)
        REFERENCES economic_accounting_operation(operation_id)
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    CONSTRAINT economic_pending_consumption_source_fk
        FOREIGN KEY (source_operation_id,source_slot)
        REFERENCES economic_pending_claim_source(source_operation_id,source_slot)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
SET @claim_origin_sql=IF(
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
      AND table_name='economic_baseline_witness' AND column_name='claim_origin_version')=0,
    'ALTER TABLE economic_baseline_witness ADD COLUMN claim_origin_version SMALLINT UNSIGNED NULL DEFAULT NULL AFTER command_accepted_at_usec, ADD CONSTRAINT ck_economic_baseline_claim_origin CHECK (claim_origin_version IS NULL OR claim_origin_version=1)',
    IF((SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE()
          AND table_name='economic_baseline_witness' AND BINARY column_name=BINARY 'claim_origin_version'
          AND ordinal_position=11 AND data_type='smallint'
          AND column_type IN ('smallint unsigned','smallint(5) unsigned')
          AND is_nullable='YES' AND numeric_precision=5 AND numeric_scale=0
          AND (column_default IS NULL OR (LOCATE('MariaDB',VERSION())>0
               AND BINARY column_default=BINARY 'NULL')) AND extra='')=1
       AND (SELECT COUNT(*) FROM information_schema.table_constraints t
          JOIN information_schema.check_constraints c ON c.constraint_schema=t.constraint_schema
            AND c.constraint_name=t.constraint_name WHERE t.constraint_schema=DATABASE()
          AND t.table_name='economic_baseline_witness'
          AND BINARY t.constraint_name=BINARY 'ck_economic_baseline_claim_origin'
          AND t.constraint_type='CHECK'
          AND LOWER(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(c.check_clause,
              CHAR(96),''),' ',''),CHAR(9),''),CHAR(10),''),CHAR(13),''),'(',''),')',''))=
              'claim_origin_versionisnullorclaim_origin_version=1')=1,
       'SELECT 1','DURIS_0062_REFUSE_DIVERGENT_CLAIM_ORIGIN_SHAPE'));
PREPARE claim_origin_stmt FROM @claim_origin_sql;
EXECUTE claim_origin_stmt;
DEALLOCATE PREPARE claim_origin_stmt;
