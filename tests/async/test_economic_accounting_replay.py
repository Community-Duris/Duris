#!/usr/bin/env python3
"""Run admission, mixed-journal retention, and legacy replay regressions."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
work = ROOT / 'bin/tests/economic-accounting-replay'
work.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='run-', dir=work) as temporary, tempfile.TemporaryDirectory(prefix='duris-accounting-replay-') as runtime:
    executable = Path(temporary) / 'replay'
    command = shlex.split(os.environ.get('CXX', 'g++')) + [
        '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O1', '-g',
        '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie',
        '-pthread', '-I' + str(ROOT / 'src')]
    command += [str(ROOT / name) for name in (
        'tests/async/economic_accounting_replay_test.cpp',
        'src/economy/economic_accounting_intent.c', 'src/economy/economic_accounting_plan.c',
        'src/economy/economic_accounting_types.c', 'src/item/item_transfer_command.c',
        'src/persistence/critical_command.c', 'src/persistence/critical_command_journal.c',
        'src/persistence/critical_command_coordinator.c')]
    command += ['-lcrypto', '-lz', '-o', str(executable)]
    subprocess.run(command, check=True)
    environment = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    # The harness creates separate journals under this fresh runtime directory.
    subprocess.run([str(executable), runtime], env=environment, check=True, timeout=30)
