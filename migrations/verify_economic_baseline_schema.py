#!/usr/bin/env python3
"""Read-only exact baseline retention metadata verification; no checkout .env."""
import argparse
import re
import sys
from verify_economy_accounting_schema import Client, VerificationError

TABLES = ('economic_baseline_control', 'economic_baseline_witness', 'economic_baseline_reservation')
HISTORICAL_0032_EXPECTED = {'mysql8': 'f0551ebf630d1e18f4bdec863f239da3d974f783f3acafbc243483b8e67bf3bc', 'mariadb10_11': '778e7d3815bc4c66d2bb13072c9bc6689df9008a332bee02b5fd3c84548e0e3e'}

# Current0062 metadata measured on both owned engines; original hashes stay historical.
EXPECTED = {'mysql8': 'fcda92ac8244729a41978dd4c1b1579732ace2a80cf3ab12fcfe3414f4494316', 'mariadb10_11': 'e305033606fbbd6c168c72990a5eceaa6c3c2e3673ff970e90c91c64214f4f95'}

# The accepted0061 canonical reader preserves phase ordering (ordinary T/C/I/F/K,
# then MySQL enforcement E), counts NULL rows, and refuses truncated GROUP_CONCAT.
METADATA_QUERY = """SET @baseline_v2_engine = CASE WHEN VERSION() LIKE '10.11.%MariaDB%' THEN 'mariadb'
    WHEN VERSION() LIKE '8.0.%' AND LOCATE('MariaDB',VERSION())=0 THEN 'mysql' ELSE NULL END;
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
SET SESSION group_concat_max_len=@baseline_v2_previous_concat;
SELECT @baseline_v2_row_count,@baseline_v2_nonnull_rows,@baseline_v2_expected_bytes,
       @baseline_v2_actual_bytes,@baseline_v2_actual;
"""

def fingerprint(client):
    version = client.sql('SELECT VERSION();').strip()
    if version.startswith('10.11.') and 'MariaDB' in version:
        engine = 'mariadb10_11'
        if client.sql('SELECT @@SESSION.check_constraint_checks;').strip() != '1':
            raise VerificationError('baseline equipment schema metadata fingerprint mismatch: disabled checks')
    elif version.startswith('8.0.') and 'MariaDB' not in version:
        engine = 'mysql8'
    else:
        raise VerificationError('unsupported database engine for baseline equipment schema')
    metadata = client.sql(METADATA_QUERY).strip()
    if not re.fullmatch(r'[0-9]+\t[0-9]+\t[0-9]+\t[0-9]+\t[0-9a-f]{64}', metadata):
        raise VerificationError('baseline equipment schema metadata fingerprint mismatch: invalid aggregate')
    rows, nonnull, expected_bytes, actual_bytes, actual = metadata.split('\t')
    if len(rows) > 4 or len(nonnull) > 4 or len(expected_bytes) > 5 or len(actual_bytes) > 5:
        raise VerificationError('baseline equipment schema metadata fingerprint mismatch: unbounded aggregate')
    if not (1 <= int(rows) <= 4096 and int(nonnull) == int(rows) and
            1 <= int(expected_bytes) <= 65536 and int(actual_bytes) == int(expected_bytes)):
        raise VerificationError('baseline equipment schema metadata fingerprint mismatch: NULL or truncated aggregate')
    return engine, actual

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--print-fingerprint', action='store_true')
    args = parser.parse_args()
    engine, actual = fingerprint(Client())
    if args.print_fingerprint:
        print(engine + ' ' + actual)
    elif EXPECTED[engine] is None:
        raise VerificationError('0062 baseline metadata awaits actual engine measurement')
    elif actual != EXPECTED[engine]:
        raise VerificationError('baseline retention metadata fingerprint mismatch')
    else:
        print('baseline retention schema verified: 3 InnoDB tables, exact metadata')

if __name__ == '__main__':
    try:
        main()
    except (VerificationError, OSError) as error:
        print('baseline schema verification failed: ' + str(error), file=sys.stderr)
        raise SystemExit(1)
