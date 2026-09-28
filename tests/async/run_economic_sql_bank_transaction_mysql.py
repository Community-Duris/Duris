#!/usr/bin/env python3
"""Compile/run the typed SQL bank journey in an explicitly disposable schema."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
if (os.environ.get('ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA') != '1' or
    os.environ.get('DB_HOST') != '127.0.0.1' or os.environ.get('DB_SOCKET') or
    not re.fullmatch(r'economic_schema_test_[A-Za-z0-9_]+',os.environ.get('DB_NAME',''))):
    raise SystemExit('explicit disposable loopback schema required')
# Reuse the maintained legacy repository link set, changing only the test driver.
script = (ROOT/'tests/async/run_currency_transaction_schema_mysql.sh').read_text()
chunk = script.split('g++ -std=c++20',1)[1].split('"$ROOT/bin/tests/currency_transaction_mysql_harness"',1)[0]
files = re.findall(r'(?:tests|src)/[A-Za-z0-9_/.-]+\.(?:cpp|c)',chunk)[1:]
files += ['tests/async/economic_sql_bank_transaction_mysql_harness.cpp',
          'src/persistence/economic_accounting_repository.c',
          'src/persistence/economic_sql_bank_transaction.c',
          'src/economy/economic_currency_adapter.c','src/economy/economic_accounting_types.c',
          'src/economy/economic_accounting_plan.c','src/economy/economic_accounting_intent.c']
files = list(dict.fromkeys(files))
work = ROOT/'bin/tests/economic-sql-bank'
work.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='run-',dir=work) as temporary:
    executable = Path(temporary)/'bank'
    flags = ['g++','-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-pthread','-O1','-g',
             '-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie','-Isrc']
    flags += shlex.split(subprocess.check_output(['mysql_config','--cflags'],text=True))
    flags += ['-Wl,--wrap=mysql_real_query,--wrap=mysql_errno']
    flags += files + shlex.split(subprocess.check_output(['mysql_config','--libs'],text=True))
    subprocess.run(flags+['-lcrypto','-o',str(executable)],cwd=ROOT,check=True)
    subprocess.run([str(executable)],cwd=ROOT,check=True,
                   env=dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                            UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'))
