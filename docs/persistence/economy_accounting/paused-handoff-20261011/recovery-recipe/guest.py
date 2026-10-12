import hashlib,json,os,shlex,shutil,subprocess,sys,time
from pathlib import Path
assert hashlib.sha256(Path('/recipe/trusted_scoped.py').read_bytes()).hexdigest()=='85c6c07ed2edea087e4878792a3a2c381ca7f11063590e5d088e11328a790644'
sys.path.insert(0,'/recipe')
import trusted_scoped as t
cfg=json.loads(Path('/recipe/CONFIG.json').read_bytes())
root=Path('/root-source'); source=Path('/work/source'); evidence=Path('/evidence')
result={'status':'STARTED','scope':cfg['scope'],'production_source_authority':cfg['revision'],'runs':[]}
def verify(base,rows):
    for row in rows:
        p=base/row['path']
        assert p.is_file() and not p.is_symlink(),row['path']
        assert t.pin(p)=={'bytes':row['bytes'],'sha256':row['sha256']},row['path']
try:
    verify(root,cfg['inputs']+cfg['protected']+[cfg['runner'],cfg['provider_packet'],cfg['trusted_origin']])
    # Docker owns the nested artifact mount; no archive/extractor owns source here.
    source.mkdir(parents=True,exist_ok=True)
    assert (source/'bin/tests').is_dir() and not (source/'bin/tests').is_symlink()
    assert not list((source/'bin/tests').iterdir()),'artifact output must be fresh'
    for row in cfg['inputs']:
        p=source/row['path'];p.parent.mkdir(parents=True,exist_ok=True)
        p.write_bytes((root/row['path']).read_bytes())
        assert t.pin(p)=={'bytes':row['bytes'],'sha256':row['sha256']},row['path']
    selected=source/'tests/async/test_native_quest_recovery_context.py'
    selected.write_bytes((root/cfg['runner']['path']).read_bytes())
    assert t.file_sha(selected)==cfg['runner']['sha256']
    compiler=Path(cfg['compiler']['path'])
    assert t.file_sha(compiler)==cfg['compiler']['sha256']
    actual=subprocess.run([str(compiler),'-dumpfullversion'],capture_output=True,check=True,timeout=15)
    assert actual.stdout.decode().strip()==cfg['compiler']['version']
    aliases,alias_records=t.linker_aliases(Path('/work'))
    assert alias_records==cfg['expected_linker_aliases']
    result['compiler']=cfg['compiler'];result['linker_aliases']=alias_records
    env=dict(os.environ,CXX=shlex.join([str(compiler),'-L'+str(aliases)]),PYTHONDONTWRITEBYTECODE='1')
    run=t.execute(['python3','-B','-u','tests/async/test_native_quest_recovery_context.py'],source,env,evidence/'component.log',cfg['guest_seconds'])
    result['runs'].append(run)
    directories=list((source/'bin/tests').iterdir());assert len(directories)==1
    artifacts=directories[0]
    assert artifacts.is_dir() and not artifacts.is_symlink()
    receipt=json.loads((artifacts/'receipt.json').read_bytes())
    result['original_receipt']=receipt
    result['artifact_directory']=artifacts.name
    result['artifact_hashes']={p.relative_to(artifacts).as_posix():t.pin(p) for p in sorted(artifacts.rglob('*')) if p.is_file()}
    assert run['status']=='PASS',run
    assert receipt['status']=='passed_component_only' and receipt['source_unchanged']
    assert receipt['successful_compiles']==1 and receipt['runtime_attempts']==1
    assert receipt['runtime_timeout_seconds']==20 and receipt['runtime_exit_code']==0
    assert len(receipt['source_sha256'])==18
    result['status']='PASS_RECOVERY_CONTEXT_COMPONENT_ONLY'
except BaseException as exc:
    result.update(status='FAIL',exception_type=type(exc).__name__,failure=str(exc)[:2500])
    raise
finally:
    try:
        verify(root,cfg['inputs']+cfg['protected']+[cfg['runner'],cfg['provider_packet'],cfg['trusted_origin']])
        if source.exists():
            for row in cfg['inputs']:
                if row['path']=='tests/async/test_native_quest_recovery_context.py':continue
                p=source/row['path']
                if p.exists():assert t.pin(p)=={'bytes':row['bytes'],'sha256':row['sha256']},row['path']
        result['post_source_inputs_preserved']=True
    except BaseException as exc:
        result.update(status='FAIL',post_guard_failure=str(exc));raise
    finally:t.dump(evidence/'RESULTS.json',result)
