#!/usr/bin/env python3
"""Check resolved-plan canonical bytes in both compilation modes under sanitizers."""
import os
import hashlib
from pathlib import Path
import shlex
import struct
import subprocess
import tempfile
import test_economic_accounting_types as fixtures

ROOT=Path(__file__).resolve().parents[2]

def reference():
    ident=lambda n:bytes([n])+bytes(15)
    header=b'EAP1'+struct.pack('<HH',1,0)+ident(1)+ident(2)+ident(3)+bytes(16)
    header+=struct.pack('<B3xQIIIH2x',1,7,1,1,1,2)
    header+=bytes(52)+bytes([11])+bytes(31)+bytes([12])+bytes(31)
    header+=struct.pack('<6I16x',2,2,0,0,0,0)
    assert len(header)==256
    account=lambda pid,before,after,br,ar:ident(1)+struct.pack('<HHQQ4x',1,1,pid,0)+struct.pack('<8q2Q',*before,*after,br,ar)
    result=header+account(1,[0,0,0,1],[3,6,8,0],4,5)+account(2,[0]*4,[7,3,1,0],8,9)
    result+=struct.pack('<IHH5q',0,0,0,3,6,8,-1,-137)+struct.pack('<IHH5q',1,1,0,7,3,1,0,137)
    assert len(result)==592
    command=b'CCM1'+struct.pack('<I',1)+ident(3)+struct.pack('<HHHBBQIII',3,1,1,1,0,1,2,2,3)
    command+=struct.pack('<B7xQ',1,7)+struct.pack('<B7xQ',2,8)
    command+=struct.pack('<B7xQQ',1,7,9)+struct.pack('<B7xQQ',2,8,10)+bytes([1,2,3])
    binding=hashlib.sha256(b'DURIS-ECONOMIC-COMMAND-V1\0'+command).digest()
    return ('const std::vector<uint8_t> REFERENCE_WALLET_PLAN={'+','.join(map(str,result))+'};\n'
            +'const economic_digest REFERENCE_COMMAND_DIGEST={'+','.join(map(str,binding))+'};\n')

def main():
    work=ROOT/'bin/tests/economic-accounting-plan';work.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='run-',dir=work) as temporary:
        temp=Path(temporary);(temp/'golden.inc').write_text(fixtures.goldens(include_plans=True));(temp/'reference.inc').write_text(reference())
        outputs=[]
        for mode in ('sql','flatfile'):
            executable=temp/('plan-'+mode)
            command=shlex.split(os.environ.get('CXX','g++'))+['-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-O1','-g',
                '-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie',
                '-I'+str(ROOT/'src'),'-I'+str(temp)]
            if mode=='flatfile':command.append('-D__NO_MYSQL__')
            command += [str(ROOT/name) for name in ('tests/async/economic_accounting_plan_test.cpp','src/economy/economic_accounting_plan.c',
                'src/economy/economic_accounting_types.c','src/persistence/critical_command.c','src/item/item_transfer_command.c')]
            command += ['-lcrypto','-o',str(executable)]
            subprocess.run(command,check=True)
            environment=dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
            result=subprocess.run([str(executable)],env=environment,capture_output=True,text=True)
            if result.returncode:
                print(result.stdout, end=""); print(result.stderr, end="")
                result.check_returncode()
            outputs.append(result.stdout);print(mode+': '+result.stdout.strip())
        if outputs[0]!=outputs[1]:raise AssertionError('backend compilation changed canonical results')

if __name__=='__main__':main()
