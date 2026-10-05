-- Exact PRE/POST fingerprints measured on owned MySQL8 and MariaDB10.11 clones.
-- New0061 follows0058; immutable0032 and its historical v1 CHECK stay exact.
-- No historical row rewrite. Exact preimage -> one ALTER; exact post -> no-op.
SET @baseline_v2_engine = CASE WHEN VERSION() LIKE '10.11.%MariaDB%' THEN 'mariadb'
    WHEN VERSION() LIKE '8.0.%' AND LOCATE('MariaDB',VERSION())=0 THEN 'mysql' ELSE NULL END;
SET @baseline_v2_pre = CASE @baseline_v2_engine
    WHEN 'mysql' THEN '49fec7b149d1731df804f048b4eac966fb7013d652a446eb627638f93ad67ee9' WHEN 'mariadb' THEN '5e2b9758c2bb0776b13f007f991f2ddf74d9cb57612426270a2b54df30d9c409' ELSE NULL END;
SET @baseline_v2_post = CASE @baseline_v2_engine
    WHEN 'mysql' THEN '858f37fb10734428601d4a10bacb1892154d6fb77cd656fe8a90a004ef00016d' WHEN 'mariadb' THEN '31f31065ee4e95d08049748eb49372a71ab7569f2f775aaaa9b1dbebf0978272' ELSE NULL END;
SET @baseline_v2_sql = IF(@baseline_v2_engine IS NOT NULL
    AND OCTET_LENGTH(@baseline_v2_pre)=64 AND @baseline_v2_pre REGEXP '^[0-9a-f]{64}$'
    AND OCTET_LENGTH(@baseline_v2_post)=64 AND @baseline_v2_post REGEXP '^[0-9a-f]{64}$',
    'SELECT 1', 'DURIS_0061_REFUSE_UNMEASURED_OR_UNSUPPORTED_METADATA');
PREPARE baseline_v2_stmt FROM @baseline_v2_sql;
EXECUTE baseline_v2_stmt;
DEALLOCATE PREPARE baseline_v2_stmt;
SET @baseline_v2_sql = IF(@baseline_v2_engine='mariadb',
    'SELECT @@SESSION.check_constraint_checks INTO @baseline_v2_checks_enabled',
    'SELECT 1 INTO @baseline_v2_checks_enabled');
PREPARE baseline_v2_stmt FROM @baseline_v2_sql;
EXECUTE baseline_v2_stmt;
DEALLOCATE PREPARE baseline_v2_stmt;
SET @baseline_v2_sql = IF(@baseline_v2_checks_enabled=1,
    'SELECT 1', 'DURIS_0061_REFUSE_DISABLED_CHECKS');
PREPARE baseline_v2_stmt FROM @baseline_v2_sql;
EXECUTE baseline_v2_stmt;
DEALLOCATE PREPARE baseline_v2_stmt;
SET @baseline_v2_previous_concat = @@SESSION.group_concat_max_len;
SET SESSION group_concat_max_len=65536;
SET @baseline_v2_read = IF(@baseline_v2_engine='mysql',
'SELECT COUNT(*),COUNT(metadata_row),COALESCE(SUM(OCTET_LENGTH(metadata_row)+1),0),COALESCE(OCTET_LENGTH(CONCAT(GROUP_CONCAT(metadata_row ORDER BY phase,metadata_row SEPARATOR ''
''),CHAR(10))),0),SHA2(CONCAT(GROUP_CONCAT(metadata_row ORDER BY phase,metadata_row SEPARATOR ''
''),CHAR(10)),256) INTO @baseline_v2_row_count,@baseline_v2_nonnull_rows,@baseline_v2_expected_bytes,@baseline_v2_actual_bytes,@baseline_v2_actual FROM (SELECT 0 AS phase,metadata_row FROM (SELECT CONCAT(''T'',CHAR(9),table_name,CHAR(9),engine,CHAR(9),table_collation) AS metadata_row
FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type=''BASE TABLE'' AND table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation'')
UNION ALL
SELECT CONCAT(''C'',CHAR(9),table_name,CHAR(9),column_name,CHAR(9),ordinal_position,
 CHAR(9),column_type,CHAR(9),is_nullable,CHAR(9),COALESCE(character_maximum_length,0),
 CHAR(9),COALESCE(numeric_precision,0),CHAR(9),COALESCE(numeric_scale,0),
 CHAR(9),COALESCE(datetime_precision,0),CHAR(9),COALESCE(column_default,''<NULL>''),CHAR(9),extra)
FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation'')
UNION ALL
SELECT CONCAT(''I'',CHAR(9),table_name,CHAR(9),index_name,CHAR(9),non_unique,CHAR(9),seq_in_index,
 CHAR(9),column_name,CHAR(9),COALESCE(sub_part,0),CHAR(9),index_type)
FROM information_schema.statistics WHERE table_schema=DATABASE() AND (table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation''))
UNION ALL
SELECT CONCAT(''F'',CHAR(9),k.table_name,CHAR(9),k.constraint_name,CHAR(9),k.column_name,
 CHAR(9),k.referenced_table_name,CHAR(9),k.referenced_column_name,CHAR(9),k.ordinal_position,
 CHAR(9),r.update_rule,CHAR(9),r.delete_rule)
FROM information_schema.key_column_usage k JOIN information_schema.referential_constraints r
 ON r.constraint_schema=k.constraint_schema AND r.constraint_name=k.constraint_name
WHERE k.constraint_schema=DATABASE() AND k.table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation'') AND k.referenced_table_name IS NOT NULL
UNION ALL
SELECT CONCAT(''K'',CHAR(9),t.table_name,CHAR(9),t.constraint_name,CHAR(9),c.check_clause)
FROM information_schema.table_constraints t JOIN information_schema.check_constraints c
 ON c.constraint_schema=t.constraint_schema AND c.constraint_name=t.constraint_name
WHERE t.constraint_schema=DATABASE() AND t.table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation'') AND t.constraint_type=''CHECK'') AS ordinary
UNION ALL SELECT 1 AS phase,metadata_row FROM (SELECT CONCAT(''E'',CHAR(9),table_name,CHAR(9),constraint_name,CHAR(9),enforced) AS metadata_row FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation'') AND constraint_type=''CHECK'') AS enforcement) AS canonical_metadata',
'SELECT COUNT(*),COUNT(metadata_row),COALESCE(SUM(OCTET_LENGTH(metadata_row)+1),0),COALESCE(OCTET_LENGTH(CONCAT(GROUP_CONCAT(metadata_row ORDER BY phase,metadata_row SEPARATOR ''
''),CHAR(10))),0),SHA2(CONCAT(GROUP_CONCAT(metadata_row ORDER BY phase,metadata_row SEPARATOR ''
''),CHAR(10)),256) INTO @baseline_v2_row_count,@baseline_v2_nonnull_rows,@baseline_v2_expected_bytes,@baseline_v2_actual_bytes,@baseline_v2_actual FROM (SELECT 0 AS phase,metadata_row FROM (SELECT CONCAT(''T'',CHAR(9),table_name,CHAR(9),engine,CHAR(9),table_collation) AS metadata_row
FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type=''BASE TABLE'' AND table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation'')
UNION ALL
SELECT CONCAT(''C'',CHAR(9),table_name,CHAR(9),column_name,CHAR(9),ordinal_position,
 CHAR(9),column_type,CHAR(9),is_nullable,CHAR(9),COALESCE(character_maximum_length,0),
 CHAR(9),COALESCE(numeric_precision,0),CHAR(9),COALESCE(numeric_scale,0),
 CHAR(9),COALESCE(datetime_precision,0),CHAR(9),COALESCE(column_default,''<NULL>''),CHAR(9),extra)
FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation'')
UNION ALL
SELECT CONCAT(''I'',CHAR(9),table_name,CHAR(9),index_name,CHAR(9),non_unique,CHAR(9),seq_in_index,
 CHAR(9),column_name,CHAR(9),COALESCE(sub_part,0),CHAR(9),index_type)
FROM information_schema.statistics WHERE table_schema=DATABASE() AND (table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation''))
UNION ALL
SELECT CONCAT(''F'',CHAR(9),k.table_name,CHAR(9),k.constraint_name,CHAR(9),k.column_name,
 CHAR(9),k.referenced_table_name,CHAR(9),k.referenced_column_name,CHAR(9),k.ordinal_position,
 CHAR(9),r.update_rule,CHAR(9),r.delete_rule)
FROM information_schema.key_column_usage k JOIN information_schema.referential_constraints r
 ON r.constraint_schema=k.constraint_schema AND r.constraint_name=k.constraint_name
WHERE k.constraint_schema=DATABASE() AND k.table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation'') AND k.referenced_table_name IS NOT NULL
UNION ALL
SELECT CONCAT(''K'',CHAR(9),t.table_name,CHAR(9),t.constraint_name,CHAR(9),c.check_clause)
FROM information_schema.table_constraints t JOIN information_schema.check_constraints c
 ON c.constraint_schema=t.constraint_schema AND c.constraint_name=t.constraint_name
WHERE t.constraint_schema=DATABASE() AND t.table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation'') AND t.constraint_type=''CHECK'') AS ordinary) AS canonical_metadata');
PREPARE baseline_v2_stmt FROM @baseline_v2_read;
EXECUTE baseline_v2_stmt;
DEALLOCATE PREPARE baseline_v2_stmt;
-- Check complete bounded byte count as well as digest, never a truncated prefix.
SET @baseline_v2_complete = @baseline_v2_row_count BETWEEN 1 AND 4096
    AND @baseline_v2_nonnull_rows=@baseline_v2_row_count
    AND @baseline_v2_expected_bytes BETWEEN 1 AND 65536
    AND @baseline_v2_expected_bytes=@baseline_v2_actual_bytes;
SET @baseline_v2_alter = IF(@baseline_v2_engine='mysql',
'ALTER TABLE economic_baseline_witness DROP CHECK ck_economic_baseline_witness_shape, ADD CONSTRAINT ck_economic_baseline_witness_shape CHECK (holding_count<=3071 AND item_count<=6000 AND (
        (witness_version=1 AND OCTET_LENGTH(canonical_witness)=192+112*holding_count+88*item_count AND SUBSTRING(canonical_witness,1,4)=X''45414231'') OR
        (witness_version=2 AND OCTET_LENGTH(canonical_witness)=192+112*holding_count+96*item_count AND SUBSTRING(canonical_witness,1,4)=X''45414232'' AND SUBSTRING(canonical_witness,5,2)=X''0200'' AND SUBSTRING(canonical_witness,7,2)=X''C000'')))',
'ALTER TABLE economic_baseline_witness DROP CONSTRAINT ck_economic_baseline_witness_shape, ADD CONSTRAINT ck_economic_baseline_witness_shape CHECK (holding_count<=3071 AND item_count<=6000 AND (
        (witness_version=1 AND OCTET_LENGTH(canonical_witness)=192+112*holding_count+88*item_count AND SUBSTRING(canonical_witness,1,4)=X''45414231'') OR
        (witness_version=2 AND OCTET_LENGTH(canonical_witness)=192+112*holding_count+96*item_count AND SUBSTRING(canonical_witness,1,4)=X''45414232'' AND SUBSTRING(canonical_witness,5,2)=X''0200'' AND SUBSTRING(canonical_witness,7,2)=X''C000'')))');
SET @baseline_v2_sql = IF(@baseline_v2_complete AND BINARY @baseline_v2_actual=BINARY @baseline_v2_post,
    'SELECT 1', IF(@baseline_v2_complete AND BINARY @baseline_v2_actual=BINARY @baseline_v2_pre,
    @baseline_v2_alter, 'DURIS_0061_REFUSE_DIVERGENT_BASELINE_METADATA'));
PREPARE baseline_v2_stmt FROM @baseline_v2_sql;
EXECUTE baseline_v2_stmt;
DEALLOCATE PREPARE baseline_v2_stmt;
PREPARE baseline_v2_stmt FROM @baseline_v2_read;
EXECUTE baseline_v2_stmt;
DEALLOCATE PREPARE baseline_v2_stmt;
SET SESSION group_concat_max_len=@baseline_v2_previous_concat;
SET @baseline_v2_sql = IF(@baseline_v2_row_count BETWEEN 1 AND 4096
    AND @baseline_v2_nonnull_rows=@baseline_v2_row_count
    AND @baseline_v2_expected_bytes BETWEEN 1 AND 65536
    AND @baseline_v2_expected_bytes=@baseline_v2_actual_bytes
    AND BINARY @baseline_v2_actual=BINARY @baseline_v2_post,
    'SELECT 1', 'DURIS_0061_REFUSE_POST_ALTER_BASELINE_METADATA');
PREPARE baseline_v2_stmt FROM @baseline_v2_sql;
EXECUTE baseline_v2_stmt;
DEALLOCATE PREPARE baseline_v2_stmt;
