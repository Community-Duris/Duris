#!/usr/bin/env python3
"""Execute bounded baseline genesis preparation in both build modes."""
import os
import hashlib
import struct
from pathlib import Path
import shlex
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
def reference():
    ident=lambda n: bytes([n])+bytes(15)
    digest=lambda n: bytes([n])+bytes(31)
    key=lambda kind,authority,context: ident(1)+struct.pack('<HHQQ4x',1,kind,authority,context)
    intent=struct.pack('<H',1)+ident(1)+ident(2)+ident(3)+struct.pack('<QQ',7,8)
    intent+=key(9,9001,0)+digest(11)+digest(12)+struct.pack('<II',6,3)
    domain=b''
    for n in range(1,7):
        domain+=key(n,1000+n,1 if n==2 else 0)+struct.pack('<4qQ',n,2,3,4,n-1)+digest(20+n)
    for uid,owner,owner_id,root,parent,revision,state,source in ((1,1,7,1,0,0,1,31),(2,1,7,1,1,8,3,31),(3,8,0,1,1,9,2,32)):
        domain+=struct.pack('<QB5QB',uid,owner,owner_id,0,root,parent,revision,state)+digest(source)
    result=''
    for name,tag,value in (('INTENT',b'DURIS-ECONOMIC-BASELINE-INTENT-V1\0',intent),('DOMAIN',b'DURIS-ECONOMIC-BASELINE-DOMAIN-V1\0',domain)):
        result+='const economic_digest REFERENCE_'+name+'={'+','.join(map(str,hashlib.sha256(tag+value).digest()))+'};\n'
    witness=b'EAB1'+struct.pack('<HHII',1,192,1128,0)+ident(1)+ident(2)+ident(3)
    witness+=struct.pack('<QQ',7,8)+key(9,9001,0)+digest(11)+digest(12)+struct.pack('<II',6,3)
    assert len(witness)==192
    for n in range(1,7):
        witness+=key(n,1000+n,1 if n==2 else 0)+struct.pack('<4qQ',n,2,3,4,n-1)+digest(20+n)
    for uid,owner,owner_id,root,parent,revision,state,source in ((1,1,7,1,0,0,1,31),(2,1,7,1,1,8,3,31),(3,8,0,1,1,9,2,32)):
        witness+=struct.pack('<QBB6x5Q',uid,owner,state,owner_id,0,root,parent,revision)+digest(source)
    assert len(witness)==1128
    result+='const std::vector<uint8_t> REFERENCE_WITNESS={'+','.join(map(str,witness))+'};\n'
    payload=b'EBC1'+struct.pack('<HHII',1,48,len(witness),0)+hashlib.sha256(witness).digest()
    result+='const std::vector<uint8_t> REFERENCE_COMMAND_PAYLOAD={'+','.join(map(str,payload))+'};\n'
    return result

with tempfile.TemporaryDirectory(prefix='duris-baseline-') as temporary:
    (Path(temporary)/'baseline_reference.inc').write_text(reference())
    for mode in ('sql','client-free'):
        executable=Path(temporary)/mode
        command=shlex.split(os.environ.get('CXX','g++'))+['-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-O1','-g',
            '-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie','-I'+str(ROOT/'src'),'-I'+temporary]
        if mode=='client-free':command += ['-D__NO_MYSQL__', '-I'+str(ROOT/'src/no_mysql')]
        command += [str(ROOT/name) for name in ('tests/async/economic_baseline_adapter_test.cpp',
            'src/economy/economic_baseline_command.c','src/economy/economic_baseline_adapter.c','src/economy/economic_baseline_codec.c','src/economy/economic_accounting_intent.c',
            'src/economy/economic_accounting_plan.c','src/economy/economic_accounting_types.c',
            'src/economy/economic_command_admission.c','src/economy/economic_currency_adapter.c',
            'src/persistence/critical_command_coordinator.c','src/persistence/critical_command_journal.c',
            'src/flatfile/flatfile_accounting_store.c','src/flatfile/flatfile_authority_transaction.c','src/flatfile/flatfile_store.c',
            'src/economy/currency_command.c','src/persistence/critical_command.c','src/item/item_transfer_command.c')]
        command+=['-Wl,--wrap=_Znwm,--wrap=_Znam','-lcrypto','-lz','-pthread','-o',str(executable)]
        subprocess.run(command,check=True)
        environment=dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
        subprocess.run([str(executable), str(Path(temporary)/(mode+'-journal'))],check=True,env=environment,timeout=120)
        print(mode+' baseline preparation passed',flush=True)
