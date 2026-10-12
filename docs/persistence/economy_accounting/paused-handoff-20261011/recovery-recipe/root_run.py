"""Root-only direct isolated component; author never executes this helper."""
import argparse,hashlib,json,os,shutil,subprocess,sys,time
from pathlib import Path
HERE=Path(__file__).resolve().parent
assert hashlib.sha256((HERE/'trusted_scoped.py').read_bytes()).hexdigest()=='85c6c07ed2edea087e4878792a3a2c381ca7f11063590e5d088e11328a790644'
import trusted_scoped as t
p=argparse.ArgumentParser();p.add_argument('--repo',required=True);p.add_argument('--out',required=True);p.add_argument('--expect-manifest',required=True);a=p.parse_args()
repo=Path(a.repo).resolve();out=Path(a.out).resolve()
assert out.parent==repo/'tmp' and not out.exists() and not out.is_relative_to(HERE)
assert out.name=='recovery-context-currentf215-native-root-v3-20261011'
t.manifest_check(HERE,a.expect_manifest)
cfg=json.loads((HERE/'CONFIG.json').read_bytes())

# Prospective host supervision: logical file bytes and actual free space are
# separate measurements. Sampling is not an instantaneous disk reservation.
ARTIFACT_LIMIT=1024**3
FREE_RESERVE=512*1024**2
PREFLIGHT_FREE=ARTIFACT_LIMIT+FREE_RESERVE
STORAGE_SAMPLE_SECONDS=1.0

def storage_guard(preflight=False):
    owned=0
    for path in out.rglob('*'):
        if path.is_symlink() or (hasattr(path,'is_junction') and path.is_junction()):
            raise RuntimeError('Owned OUT contains a linked entry')
        if path.is_file():
            owned+=path.stat().st_size
    free=shutil.disk_usage(out).free
    sample=dict(owned_logical_bytes=owned,actual_free_bytes=free,
                logical_limit_bytes=ARTIFACT_LIMIT,free_reserve_bytes=FREE_RESERVE,
                preflight_free_bytes=PREFLIGHT_FREE,sample_interval_seconds=STORAGE_SAMPLE_SECONDS)
    if owned>ARTIFACT_LIMIT or free<(PREFLIGHT_FREE if preflight else FREE_RESERVE):
        raise RuntimeError('Host storage guard violation: '+json.dumps(sample,sort_keys=True))
    return sample
def guard():
    assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo).decode().strip()==cfg['revision']
    assert subprocess.check_output(['git','status','--porcelain=v1','-z','--untracked-files=normal'],cwd=repo).hex()==cfg['status_hex']
    assert subprocess.check_output(['git','diff','--cached','--name-only'],cwd=repo)==b''
    for row in cfg['inputs']+cfg['protected']+[cfg['runner'],cfg['provider_packet'],cfg['trusted_origin']]:
        path=repo/row['path'];assert not path.is_symlink() and path.is_file(),row['path']
        assert t.pin(path)=={'bytes':row['bytes'],'sha256':row['sha256']},row['path']
    t.manifest_check(HERE,a.expect_manifest)
guard();out.mkdir();(out/'native/artifacts').mkdir(parents=True)
container='duris-recovery-context-'+hashlib.sha256(str(out).encode()).hexdigest()[:12]
command=['docker','run','--rm','--pull=never','--name',container,'--network','none','--read-only','--cap-drop=ALL','--security-opt=no-new-privileges','--memory=8g','--cpus=2','--pids-limit=512','--tmpfs','/work:rw,exec,nosuid,size=10g','--tmpfs','/tmp:rw,exec,nosuid,size=1g','-v',str(repo)+':/root-source:ro','-v',str(HERE)+':/recipe:ro','-v',str(out/'native')+':/evidence','-v',str(out/'native/artifacts')+':/work/source/bin/tests','--entrypoint','python3',cfg['image'],'-B','/recipe/guest.py']
result=dict(status='STARTED',command=command,container=container,revision=cfg['revision'],recipe_manifest=a.expect_manifest,timeout_seconds=cfg['host_seconds'],scope=cfg['scope']);t.dump(out/'HOST-DISPATCH-STARTED.json',result)
process=None;start=time.monotonic()
try:
    with (out/'HOST-DISPATCH.log').open('xb') as log:
        result['storage_preflight']=storage_guard(preflight=True)
        process=subprocess.Popen(command,stdout=log,stderr=subprocess.STDOUT)
        while process.poll() is None:
            result['storage_last_live']=storage_guard()
            remaining=cfg['host_seconds']-(time.monotonic()-start)
            if remaining<=0:
                raise subprocess.TimeoutExpired(command,cfg['host_seconds'])
            time.sleep(min(STORAGE_SAMPLE_SECONDS,remaining))
        code=process.returncode
        # Final growth must not bypass the last live sample before real PASS.
        result['storage_before_pass']=storage_guard()
    result.update(exit_code=code,status='PASS_COMPONENT_ONLY' if code==0 else 'FAIL')
    assert code==0
    assert json.loads((out/'native/RESULTS.json').read_bytes())['status']=='PASS_RECOVERY_CONTEXT_COMPONENT_ONLY'
except BaseException as exc:
    result.update(status='FAIL',exception_type=type(exc).__name__,failure=str(exc)[:2500]);raise
finally:
    try:
        if process is not None:result['cleanup']=t.host_container_cleanup(container)
        else:result['cleanup']=dict(container_not_started=True,container=container)
    except BaseException as exc:result.update(status='FAIL',cleanup_failure=str(exc));raise
    finally:
        if process and process.poll() is None:process.kill();process.wait(timeout=15)
        try:guard();result['post_host_inputs_preserved']=True
        except BaseException as exc:result.update(status='FAIL',post_guard_failure=str(exc));raise
        finally:
            result['elapsed_seconds']=time.monotonic()-start
            result['log_sha256']=t.file_sha(out/'HOST-DISPATCH.log')
            try:result['storage_after_cleanup']=storage_guard()
            except BaseException as exc:
                result.update(status='FAIL',storage_after_cleanup_failure=str(exc))
                t.dump(out/'HOST-DISPATCH-RESULTS.json',result)
                raise
            t.dump(out/'HOST-DISPATCH-RESULTS.json',result)
            # Include the actual final record in total retained artifacts. No
            # more successful-run files are written after this final check.
            try:storage_guard()
            except BaseException as exc:
                result.update(status='FAIL',storage_after_record_failure=str(exc))
                t.dump(out/'HOST-DISPATCH-RESULTS.json',result)
                raise
