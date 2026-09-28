#!/usr/bin/env python3
"""Execute typed bank-transfer preparation and actual shared currency arithmetic."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
work=ROOT/'bin/tests/economic-currency-adapter';work.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='run-',dir=work) as temporary:
    for mode in ('sql','flatfile'):
        executable=Path(temporary)/mode
        command=shlex.split(os.environ.get('CXX','g++'))+['-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-O1','-g',
            '-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie','-I'+str(ROOT/'src')]
        if mode=='flatfile':command.append('-D__NO_MYSQL__')
        command += [str(ROOT/name) for name in ('tests/async/economic_currency_adapter_test.cpp',
            'src/economy/economic_currency_adapter.c','src/economy/economic_accounting_intent.c',
            'src/economy/economic_accounting_plan.c','src/economy/economic_accounting_types.c',
            'src/economy/currency_command.c','src/persistence/critical_command.c','src/item/item_transfer_command.c')]
        command+=['-lcrypto','-o',str(executable)]
        subprocess.run(command,check=True)
        environment=dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
        subprocess.run([str(executable)],check=True,env=environment)
