#!/usr/bin/env python3
"""Exercise the real reference writer on an explicitly disposable SQL schema."""
import importlib.util
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class ItemReferenceMysqlTest(unittest.TestCase):
    def test_real_statement_errors_and_reference_integrity(self):
        if os.environ.get('TEST_DB_DISPOSABLE') != '1':
            self.skipTest('explicit disposable SQL fixture required')
        if (os.environ.get('ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA') != '1' or
                os.environ.get('DB_HOST') != '127.0.0.1' or os.environ.get('DB_SOCKET') or
                not re.fullmatch(r'economic_schema_test_[A-Za-z0-9_]+', os.environ.get('DB_NAME', ''))):
            self.fail('explicit disposable loopback schema is required')
        spec = importlib.util.spec_from_file_location('schema_fixture',
            ROOT / 'tests/async/test_economic_accounting_schema_mysql.py')
        assert spec is not None and spec.loader is not None
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        fixture = module.AccountingSchemaTest()
        ledger = ('INSERT INTO item_ownership_ledger(operation_id,event_index,item_uid,root_item_uid,'
                  'from_owner_type,from_owner_id,from_owner_context_id,to_owner_type,to_owner_id,'
                  'to_owner_context_id,item_revision,from_owner_revision,to_owner_revision,reason_type,'
                  'source_site) VALUES(' + module.binary(3) + ',0,777,777,1,7,0,1,8,0,1,0,1,1,1);')
        fixture_sql = fixture.setup_epoch() + fixture.operation(2) + fixture.inbox(3) + ledger
        with tempfile.TemporaryDirectory(prefix='duris-item-reference-mysql-') as directory:
            binary = Path(directory) / 'probe'
            flags = shlex.split(subprocess.check_output(['mysql_config', '--cflags', '--libs'], text=True))
            subprocess.run([
                *shlex.split(os.environ.get('CXX', 'g++')), '-std=c++20', '-O1', '-g',
                '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-fsanitize=address,undefined',
                '-fno-omit-frame-pointer', '-fno-pie', '-no-pie', '-Isrc',
                'tests/async/economic_accounting_item_reference_mysql_harness.cpp',
                'src/item/economic_accounting_item_reference.c', '-o', str(binary), *flags,
            ], cwd=ROOT, check=True, timeout=120)
            subprocess.run([str(binary)], cwd=ROOT, input=fixture_sql, text=True, check=True,
                timeout=30, env=dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                                    UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'))


if __name__ == '__main__':
    unittest.main()
