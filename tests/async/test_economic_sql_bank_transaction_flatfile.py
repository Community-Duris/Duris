#!/usr/bin/env python3
"""Structural bank routing and client-free refusal; no SQL connection is used."""
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
work=ROOT/'bin/tests/economic-sql-bank-flatfile';work.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='run-',dir=work) as temporary:
    executable=Path(temporary)/'test'
    files=['tests/async/economic_sql_bank_transaction_flatfile_test.cpp',
           'src/persistence/economic_sql_bank_transaction.c','src/economy/economic_currency_adapter.c',
           'src/economy/economic_accounting_intent.c','src/economy/economic_accounting_plan.c',
           'src/economy/economic_accounting_types.c','src/economy/currency_command.c',
           'src/persistence/critical_command.c','src/item/item_transfer_command.c']
    subprocess.run(['g++','-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-D__NO_MYSQL__',
                    '-Isrc/no_mysql','-Isrc']+files+['-lcrypto','-o',str(executable)],cwd=ROOT,check=True)
    subprocess.run([str(executable)],check=True)
