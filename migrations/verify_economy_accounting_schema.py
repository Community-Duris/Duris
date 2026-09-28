#!/usr/bin/env python3
"""Read-only exact accounting metadata verification; never loads checkout .env."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys

TABLES = (
    'economic_epoch', 'economic_lineage_state', 'economic_account_mapping',
    'economic_accounting_operation', 'economic_accounting_account_effect',
    'economic_accounting_coin_posting', 'economic_accounting_child',
    'economic_accounting_item_reference', 'economic_accounting_source_claim',
)
# Measured from the reviewed schema on disposable engines; not input-controlled.
EXPECTED = {
    'mysql8': 'f79cff9303ff365c38e419636a09d7f02e85f84ca948ba61dfbcc14c3dabbc2c',
    'mariadb10_11': '53a2a5a828352ea57072329086b013c38731b85e799a2f7dc083f6850a19da7d',
}

class VerificationError(Exception):
    pass

class Client:
    def __init__(self):
        for key in ('DB_HOST', 'DB_USER', 'DB_PASSWD', 'DB_NAME'):
            if not os.environ.get(key):
                raise VerificationError('explicit database environment is required')
        if not re.fullmatch(r'[A-Za-z0-9_]{1,64}', os.environ['DB_NAME']):
            raise VerificationError('invalid database name')
        port = os.environ.get('DB_PORT', '3306')
        if not port.isdigit() or not 1 <= int(port) <= 65535:
            raise VerificationError('invalid database port')
        help_result = subprocess.run(['mysql', '--no-defaults', '--help'], capture_output=True, text=True)
        host = os.environ['DB_HOST']
        socket = os.environ.get('DB_SOCKET')
        if socket:
            if not Path(socket).is_absolute():
                raise VerificationError('database socket must be absolute')
            connection = ['--protocol=socket', '--socket='+socket]
        else:
            connection = ['--protocol=tcp', '-h', host, '-P', port]
            if host in ('127.0.0.1','localhost','::1'):
                connection += ['--ssl-mode=PREFERRED' if '--ssl-mode' in help_result.stdout else '--skip-ssl']
            else:
                certificate = os.environ.get('DB_SSL_CA','')
                if os.environ.get('DB_TLS') != 'TRUE' or not Path(certificate).is_file():
                    raise VerificationError('remote schema verification requires TLS and a CA file')
                if '--ssl-mode' in help_result.stdout:
                    connection += ['--ssl-mode=VERIFY_IDENTITY','--ssl-ca='+certificate]
                elif '--ssl-verify-server-cert' in help_result.stdout:
                    connection += ['--ssl-verify-server-cert','--ssl-ca='+certificate]
                else:
                    raise VerificationError('database client cannot verify remote identity')
        self.args = ['mysql', '--no-defaults', '--connect-timeout=10', *connection,
                     '-u', os.environ['DB_USER'], '-N', '-B', '--raw', os.environ['DB_NAME']]
        self.env = {**os.environ, 'MYSQL_PWD': os.environ['DB_PASSWD']}
    def sql(self, query):
        try:
            result = subprocess.run(self.args, input=query, text=True, capture_output=True, env=self.env, timeout=30)
        except subprocess.TimeoutExpired:
            raise VerificationError('database metadata query timed out') from None
        if result.returncode:
            raise VerificationError('database statement failed')
        return result.stdout

def fingerprint(client, tables=TABLES, item_reference_index=True):
    names = ','.join("'" + name + "'" for name in tables)
    legacy_index = " OR (table_name='item_ownership_ledger' AND index_name='uq_item_ledger_accounting_reference')" if item_reference_index else ""
    # Include unsignedness, nullability, lengths, defaults, generated/update
    # attributes, ordered indexes, foreign keys and CHECK expressions. Row counts,
    # AUTO_INCREMENT counters and timestamps are data and are deliberately absent.
    query = f"""
SELECT CONCAT('T',CHAR(9),table_name,CHAR(9),engine,CHAR(9),table_collation)
FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type='BASE TABLE' AND table_name IN ({names})
UNION ALL
SELECT CONCAT('C',CHAR(9),table_name,CHAR(9),column_name,CHAR(9),ordinal_position,
 CHAR(9),column_type,CHAR(9),is_nullable,CHAR(9),COALESCE(character_maximum_length,0),
 CHAR(9),COALESCE(numeric_precision,0),CHAR(9),COALESCE(numeric_scale,0),
 CHAR(9),COALESCE(datetime_precision,0),CHAR(9),COALESCE(column_default,'<NULL>'),CHAR(9),extra)
FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name IN ({names})
UNION ALL
SELECT CONCAT('I',CHAR(9),table_name,CHAR(9),index_name,CHAR(9),non_unique,CHAR(9),seq_in_index,
 CHAR(9),column_name,CHAR(9),COALESCE(sub_part,0),CHAR(9),index_type)
FROM information_schema.statistics WHERE table_schema=DATABASE() AND (table_name IN ({names}){legacy_index})
UNION ALL
SELECT CONCAT('F',CHAR(9),k.table_name,CHAR(9),k.constraint_name,CHAR(9),k.column_name,
 CHAR(9),k.referenced_table_name,CHAR(9),k.referenced_column_name,CHAR(9),k.ordinal_position,
 CHAR(9),r.update_rule,CHAR(9),r.delete_rule)
FROM information_schema.key_column_usage k JOIN information_schema.referential_constraints r
 ON r.constraint_schema=k.constraint_schema AND r.constraint_name=k.constraint_name
WHERE k.constraint_schema=DATABASE() AND k.table_name IN ({names}) AND k.referenced_table_name IS NOT NULL
UNION ALL
SELECT CONCAT('K',CHAR(9),t.table_name,CHAR(9),t.constraint_name,CHAR(9),c.check_clause)
FROM information_schema.table_constraints t JOIN information_schema.check_constraints c
 ON c.constraint_schema=t.constraint_schema AND c.constraint_name=t.constraint_name
WHERE t.constraint_schema=DATABASE() AND t.table_name IN ({names}) AND t.constraint_type='CHECK'
ORDER BY 1;
"""
    rows = client.sql(query)
    if sum(line.startswith('T\t') for line in rows.splitlines()) != len(tables):
        raise VerificationError('accounting table inventory is incomplete')
    # MySQL exposes enforcement independently of the check expression.
    version = client.sql('SELECT VERSION();').strip()
    if 'MariaDB' in version and version.startswith('10.11.'):
        engine = 'mariadb10_11'
    elif 'MariaDB' not in version and version.startswith('8.0.'):
        engine = 'mysql8'
        rows += client.sql(f"SELECT CONCAT('E',CHAR(9),table_name,CHAR(9),constraint_name,CHAR(9),enforced) FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name IN ({names}) AND constraint_type='CHECK' ORDER BY 1;")
    else:
        raise VerificationError('unsupported database engine for accounting schema')
    return engine, hashlib.sha256(rows.encode()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--print-fingerprint', action='store_true', help='measure metadata without claiming validation')
    args = parser.parse_args()
    engine, actual = fingerprint(Client())
    if args.print_fingerprint:
        print(engine + ' ' + actual)
        return
    if actual != EXPECTED[engine]:
        raise VerificationError('accounting schema metadata fingerprint mismatch')
    print('accounting schema verified: 9 InnoDB tables, exact columns, indexes, foreign keys and checks')

if __name__ == '__main__':
    try:
        main()
    except (VerificationError, OSError) as error:
        print('accounting schema verification failed: ' + str(error), file=sys.stderr)
        raise SystemExit(1)
