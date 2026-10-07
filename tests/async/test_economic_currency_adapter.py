#!/usr/bin/env python3
"""Execute typed bank-transfer preparation and actual shared currency arithmetic."""
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from scripts.telemetry.canonical_reward_contract import qualify_bank_root, Disposition
from test_telemetry_reward_projection import native_bank_component_evidence
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
            'src/economy/currency_command.c','src/persistence/critical_command.c','src/item/item_transfer_command.c', "src/item/craft_pouch_mutation.c", "src/combat/chaos_pouch_ledger.c", 'src/player/player_snapshot_codec.c')]
        command+=['-lcrypto','-o',str(executable)]
        subprocess.run(command,check=True)
        environment=dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1',
                         DURIS_TELEMETRY_PLAN_EXPORT='1')
        result=subprocess.run([str(executable)],env=environment,capture_output=True,text=True)
        if result.returncode:
            print(result.stdout, end=''); print(result.stderr, end=''); result.check_returncode()
        lines=result.stdout.splitlines()
        rows=[native_bank_component_evidence(tuple(bytes.fromhex(part) for part in line.split()[1:]))
              for line in lines if line.startswith('BANK_HEX ')]
        if len(rows) != 4: raise AssertionError('native bank route export incomplete')
        expected={1:(Disposition.TRANSFER,137),2:(Disposition.TRANSFER,137),
                  3:(Disposition.OPENING,1_000_000_000),4:(Disposition.EARNED,100)}
        seen=set()
        for row in rows:
            event=qualify_bank_root(*row)
            writer=row[0]['writer_id']
            if (event.disposition,event.amount) != expected[writer]:
                raise AssertionError('canonical native component reward classification')
            seen.add(writer)
        if seen != set(expected): raise AssertionError('native bank route omitted')
        print(mode+': '+'\n'.join(line for line in lines if not line.startswith('BANK_HEX ')))
        print(mode+': four native component plans agree with canonical telemetry contract; SQL receipts are boundary fixtures')
