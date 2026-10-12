"""Selected unchanged V5 helpers; runtime imports are Root-only."""
import hashlib,json,os,re,shlex,signal,subprocess,time
from pathlib import Path
sha=lambda value:hashlib.sha256(value).hexdigest()

def file_sha(path):
    value = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for data in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(data)
    return value.hexdigest()

def linker_aliases(work):
    aliases = work / "linker-aliases"
    aliases.mkdir()
    records = {}
    for name in ("libm.so", "libc.so"):
        installed = Path("/usr/lib/x86_64-linux-gnu") / name
        before = installed.read_bytes()
        after, count = re.subn(rb"(?<!\S)/lib/x86_64-linux-gnu/", b"/usr/lib/x86_64-linux-gnu/", before)
        assert b"/usr/usr/" not in after
        original_paths = re.findall(rb"(?<!\S)/usr/lib/x86_64-linux-gnu/[^\s()]+", before)
        assert all(token in after for token in original_paths)
        targets = re.findall(rb"(?<!\S)/[A-Za-z0-9_][^\s()]+", after)
        assert targets
        genuine = {}
        for token in targets:
            path = Path(token.decode("ascii"))
            with path.open("rb") as stream:
                magic = stream.read(8)
            assert magic[:4] == b"\x7fELF" or magic == b"!<arch>\n"
            genuine[str(path)] = file_sha(path)
        (aliases / name).write_bytes(after)
        records[name] = dict(installed=str(installed), original_sha256=sha(before),
                             private_sha256=file_sha(aliases / name), replacements=count, genuine_targets=genuine)
    return aliases, records

def io(path):
    p=Path(path).resolve();return Path('\\\\?\\'+str(p)) if os.name=='nt' else p

def dump(path,value):
    with path.open('x',encoding='utf-8',newline='\n') as stream:
        json.dump(value,stream,indent=2);stream.write('\n')

def pin(path):return dict(sha256=file_sha(path),bytes=Path(path).stat().st_size)

def manifest_check(directory,expected):
    data=io(directory/'MANIFEST.json').read_bytes();assert sha(data)==expected
    rows=json.loads(data)
    if isinstance(rows,dict):rows=rows['members']
    if isinstance(rows,dict):rows=[dict(path=name,**row) for name,row in rows.items()]
    for row in rows:
        path=(directory/row['path']).resolve();assert path.is_relative_to(directory.resolve())
        assert pin(io(path))==dict(sha256=row['sha256'],bytes=row['bytes']),row['path']
    return rows

def received(path,expected):
    assert file_sha(path)==expected
    value=json.loads(path.read_bytes())
    assert value.get('result',value.get('status','')).startswith('PASS_FINITE_')
    assert value.get('findings')==[]
    return value

def process_group_cleanup(process,grace=10):
    group=process.pid;events=[]
    try:os.killpg(group,signal.SIGTERM);events.append('SIGTERM')
    except ProcessLookupError:return dict(events=events,group_absent=True)
    deadline=time.monotonic()+grace
    while time.monotonic()<deadline:
        process.poll()
        try:os.killpg(group,0)
        except ProcessLookupError:return dict(events=events,group_absent=True)
        time.sleep(.05)
    try:os.killpg(group,signal.SIGKILL);events.append('SIGKILL')
    except ProcessLookupError:pass
    try:process.wait(timeout=10)
    except subprocess.TimeoutExpired:events.append('LEADER_WAIT_TIMEOUT')
    # Zombies may briefly retain the group number; no live descendant is accepted silently.
    try:os.killpg(group,0);absent=False
    except ProcessLookupError:absent=True
    return dict(events=events,group_absent=absent)

def execute(command,cwd,env,log,timeout):
    started=time.monotonic();row=dict(command=command,cwd=str(cwd),timeout_seconds=timeout,status='STARTED')
    dump(log.with_suffix('.started.json'),row)
    process=None
    try:
        with log.open('xb') as stream:
            process=subprocess.Popen(command,cwd=cwd,env=env,stdout=stream,stderr=subprocess.STDOUT,start_new_session=True)
            try:
                code=process.wait(timeout=timeout);row.update(exit_code=code,status='PASS' if code==0 else 'FAIL')
            except subprocess.TimeoutExpired:
                row.update(exit_code=None,status='TIMEOUT',cleanup=process_group_cleanup(process))
    except BaseException as exc:
        row.update(status='FAIL',exception_type=type(exc).__name__,failure=str(exc)[:2000])
        if process and process.poll() is None:row['cleanup']=process_group_cleanup(process)
        raise
    finally:
        row.update(elapsed_seconds=time.monotonic()-started,log_sha256=file_sha(log) if log.exists() else None)
        dump(log.with_suffix('.json'),row)
    return row

def compile_profile(command,source,env,evidence,label,timeout):
    row=execute(command[:1]+['-n']+command[1:],source,env,evidence/(label+'.dry.log'),timeout)
    assert row['status']=='PASS'
    compiles=[shlex.split(line) for line in (evidence/(label+'.dry.log')).read_text().splitlines() if line.startswith('g++ ')]
    assert len(compiles)>=2
    for argv in compiles:
        assert '-std=c++20' in argv and '-Werror' in argv and '-Wunused-parameter' in argv and '-Wformat=2' in argv
        assert '-D__NO_TESTS__' in argv and '-DTEST_MUD' not in argv
    control=evidence/(label+'.macro-control.cc');control.write_text('#include <bits/c++config.h>\n#include <mysql/mysql.h>\n')
    flags=[];argv=compiles[0];i=1
    while i<len(argv):
        if argv[i] in ('-c','-o'):i+=2;continue
        if argv[i] in ('-MMD','-MP') or argv[i].endswith(('.c','.cc','.cpp')):i+=1;continue
        flags.append(argv[i]);i+=1
    row=execute(['g++',*flags,'-dM','-E','-x','c++',str(control)],source/'src',env,evidence/(label+'.macros.log'),timeout)
    assert row['status']=='PASS'
    text=(evidence/(label+'.macros.log')).read_text()
    assert '#define _GLIBCXX_RELEASE 13' in text and '#define _GLIBCXX_USE_CXX11_ABI 1' in text
    assert '#define TEST_MUD ' not in text and '#define _GLIBCXX_DEBUG ' not in text
    assert '#define __SANITIZE_ADDRESS__ ' not in text and '#define __SANITIZE_THREAD__ ' not in text
    assert not any(word.startswith('-fsanitize=') for word in argv)
    assert ('-D__NO_MYSQL__' in argv)==('PERSISTENCE_BACKEND=flatfile' in command)
    assert ('#define __NO_MYSQL__ ' in text)==('PERSISTENCE_BACKEND=flatfile' in command)
    for macro in ('_GLIBCXX_ASSERTIONS','_GLIBCXX_CONCEPT_CHECKS'):
        assert ('#define '+macro+' ' in text)==any('-D'+macro in word for word in command)
    return dict(compiles=len(compiles),macro_log_sha256=file_sha(evidence/(label+'.macros.log')))

def host_container_cleanup(container):
    rows=[]
    for command,timeout in ((['docker','stop','--time','30',container],45),(['docker','kill',container],15)):
        try:
            output=subprocess.run(command,capture_output=True,timeout=timeout)
            rows.append(dict(command=command,exit_code=output.returncode,stdout=output.stdout.decode(errors='replace')[:2000],stderr=output.stderr.decode(errors='replace')[:2000]))
        except subprocess.TimeoutExpired:rows.append(dict(command=command,status='TIMEOUT'))
        state=subprocess.run(['docker','inspect','--format','{{.State.Running}}',container],capture_output=True,timeout=15)
        detail=state.stderr.decode(errors='replace')
        absent_messages=tuple(prefix+container.casefold() for prefix in (
            'error: no such object: ', 'error: no such container: ',
            'error response from daemon: no such object: ',
            'error response from daemon: no such container: '))
        gone=state.returncode!=0 and detail.strip().casefold() in absent_messages
        stopped=state.returncode==0 and state.stdout.strip()==b'false'
        rows.append(dict(inspect_exit_code=state.returncode,running=state.stdout.decode(errors='replace').strip(),stderr=detail[:2000]))
        if gone or stopped:return dict(events=rows,container_not_running=True)
    raise RuntimeError('Container shutdown not verified: '+json.dumps(rows))
