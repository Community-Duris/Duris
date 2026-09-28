#!/usr/bin/env python3
"""Run private SQL baseline retention against explicitly disposable engines."""
import os
import argparse
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--client-free-only', action='store_true', help='compile and run refusal checks without a database')
parser.add_argument('--concurrency-rounds', type=int, choices=range(1,101), metavar='1..100', help='run only repeated SQL concurrency qualification')
options = parser.parse_args()
if options.client_free_only and options.concurrency_rounds:
    parser.error('client-free and SQL concurrency modes cannot be combined')
if not options.client_free_only and (os.environ.get('ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA')!='1' or os.environ.get('DB_HOST')!='127.0.0.1' or os.environ.get('DB_SOCKET') or not re.fullmatch(r'economic_schema_test_[A-Za-z0-9_]+',os.environ.get('DB_NAME',''))):
    raise SystemExit('explicit disposable loopback schema required')
files=['tests/async/economic_sql_baseline_transaction_test.cpp','src/persistence/economic_sql_baseline_transaction.c','src/economy/economic_baseline_command.c','src/economy/economic_baseline_adapter.c','src/economy/economic_baseline_codec.c','src/economy/economic_accounting_intent.c','src/economy/economic_accounting_plan.c','src/economy/economic_accounting_types.c','src/persistence/critical_command.c','src/item/item_transfer_command.c']
with tempfile.TemporaryDirectory(prefix='duris-sql-baseline-') as temporary:
    for mode in (('client-free',) if options.client_free_only else (('sql',) if options.concurrency_rounds else ('sql','client-free'))):
        executable=Path(temporary)/mode
        flags=['g++','-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-pthread','-O1','-g','-DDURIS_ECONOMIC_SQL_BASELINE_TEST','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie','-Isrc']
        if mode=='sql':flags+=shlex.split(subprocess.check_output(['mysql_config','--cflags'],text=True))+['-Wl,--wrap=mysql_real_query,--wrap=mysql_errno,--wrap=_Znwm,--wrap=_Znam']
        else:flags+=['-D__NO_MYSQL__','-Isrc/no_mysql']
        flags+=files
        if mode=='sql':flags+=shlex.split(subprocess.check_output(['mysql_config','--libs'],text=True))
        subprocess.run(flags+['-lcrypto','-o',str(executable)],cwd=ROOT,check=True)
        environment=dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
        environment.pop('DURIS_BASELINE_CONCURRENCY_ROUNDS',None)
        if options.concurrency_rounds:
            environment['DURIS_BASELINE_CONCURRENCY_ROUNDS']=str(options.concurrency_rounds)
        subprocess.run([str(executable)],cwd=ROOT,env=environment,check=True,timeout=900)
        print(mode+' SQL baseline transaction passed',flush=True)
