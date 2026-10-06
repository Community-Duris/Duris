"""Link exact original Make providers; execute private inactive world fixtures.

No fake RNG/class/UID/authority, private access, SQL activation or live services.
The original source and compiled provider directories are read-only mounts.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import resource
import shutil
import subprocess
import sys
import tempfile
import time

SOURCE_SHA = '51de87fb263df80e1aba5265f92600d6a6dfa438bc19934a5217142640285b8f'


def digest(path):
    value = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(block)
    return value.hexdigest()


def invoke(command, destination, *, cwd, env=None, timeout=None):
    started = time.monotonic()
    with destination.with_suffix('.stdout').open('wb') as stdout, \
            destination.with_suffix('.stderr').open('wb') as stderr:
        result = subprocess.run(command, cwd=cwd, env=env, stdout=stdout,
                                stderr=stderr, timeout=timeout, check=False)
    row = dict(command=command, exit=result.returncode,
               seconds=round(time.monotonic() - started, 3),
               stdout_sha256=digest(destination.with_suffix('.stdout')),
               stderr_sha256=digest(destination.with_suffix('.stderr')))
    destination.with_suffix('.json').write_text(json.dumps(row, indent=2) + '\n')
    if result.returncode:
        raise RuntimeError('Original-provider command failed; inspect owned ' + str(destination))
    return row


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source-root', type=Path, required=True)
    parser.add_argument('--providers-root', type=Path, required=True)
    parser.add_argument('--source-pins', type=Path, required=True)
    parser.add_argument('--artifacts-root', type=Path, required=True)
    args = parser.parse_args()
    source = args.source_root.resolve()
    providers = args.providers_root.resolve()
    raw = args.source_pins.read_bytes()
    if hashlib.sha256(raw).hexdigest() != SOURCE_SHA:
        raise RuntimeError('Original source manifest differs')
    source_pins = json.loads(raw)['files']
    export_raw = (providers / 'export-manifest.json').read_bytes()
    export_pins = {row['path']: row for row in json.loads(export_raw)['files']}
    records = json.loads((providers / 'tests/native-quest-major-builds/results.json').read_bytes())
    if not any(row['backend'] == 'flatfile' and row['status'] == 'PASS' and
               row['source_manifest_sha256'] == SOURCE_SHA for row in records):
        raise RuntimeError('Exact original production flatfile build proof required')
    # Source includes, serialized world inputs and original fixture helpers all
    # belong to the exact parent source archive; verify before import/compile.
    for name, pin in source_pins.items():
        path = source / name
        if pin['mode'] == '120000':
            if not path.is_symlink() or hashlib.sha256(os.readlink(path).encode()).hexdigest() != pin['sha256']:
                raise RuntimeError('Original declared link differs: ' + name)
        elif path.is_symlink() or digest(path) != pin['sha256']:
            raise RuntimeError('Original source differs: ' + name)
    args.artifacts_root.mkdir(parents=True, exist_ok=True)
    attempt = Path(tempfile.mkdtemp(prefix='actual-alchemist-', dir=args.artifacts_root))
    receipt = dict(status='RUNNING', source_manifest_sha256=SOURCE_SHA,
                   export_manifest_sha256=hashlib.sha256(export_raw).hexdigest(),
                   actual_owner_authority_proved=False, cold_sql_restore_proved=False,
                   production_accounting_activated=False, fixture_accounting_active=False,
                   full_plan3_complete=False, release_complete=False)
    receipt['driver_sha256'] = digest(Path(__file__))
    receipt['test_source_sha256'] = digest(Path(__file__).with_name('alchemist_actual_reset.cpp'))
    receipt_path = attempt / 'receipt.json'
    receipt_path.write_text(json.dumps(receipt, indent=2) + '\n')
    try:
        # Query the original Makefile's own object/flag/library contract without
        # building or changing the read-only source/provider object graph.
        query = attempt / 'query.mk'
        query.write_text('include ' + str(source / 'src/Makefile') + '\n'
                         '.PHONY: private-print\nprivate-print:\n'
                         '\t@echo OBJECTS=$(OBJS)\n\t@echo FLAGS=$(CFLAGS) $(INCLUDES)\n'
                         '\t@echo LIBS=$(LIBS)\n')
        query_result = subprocess.check_output([
            'make', '--no-print-directory', '-f', str(query), 'private-print',
            'PERSISTENCE_BACKEND=flatfile', 'BUILD_PROFILE=production'],
            cwd=source / 'src', text=True)
        (attempt / 'make-contract.txt').write_text(query_result)
        contract = dict(line.split('=', 1) for line in query_result.splitlines() if '=' in line)
        objects = []
        provider_rows = []
        for name in contract['OBJECTS'].split():
            relative = 'objects/server/flatfile/production/' + name
            path = providers / relative
            pin = export_pins[relative]
            if path.is_symlink() or digest(path) != pin['sha256']:
                raise RuntimeError('Original provider differs: ' + relative)
            objects.append(path)
            provider_rows.append(pin)
        (attempt / 'actual-provider-pins.json').write_text(json.dumps(provider_rows, indent=2) + '\n')
        # Keep all original implementation providers, including comm globals and
        # shutdown functions. Only the unrelated executable launcher entry name
        # changes in an owned copy; no gameplay symbol is replaced or wrapped.
        comm = providers / 'objects/server/flatfile/production/net/comm.o'
        renamed = attempt / 'comm-launcher-renamed.o'
        invoke(['objcopy', '--redefine-sym', 'main=duris_original_launcher_main',
                str(comm), str(renamed)], attempt / 'rename-launcher', cwd=source)
        objects = [renamed if path == comm else path for path in objects]
        import shlex
        flags = shlex.split(contract['FLAGS'])
        test_source = Path(__file__).with_name('alchemist_actual_reset.cpp')
        shutil.copyfile(test_source, attempt / 'tested-alchemist_actual_reset.cpp')
        test_object = attempt / 'alchemist_actual_reset.o'
        compile_row = invoke(['g++', *flags, '-c', str(test_source), '-o', str(test_object)],
                             attempt / 'compile', cwd=source / 'src')
        binary = attempt / 'alchemist_actual_reset'
        link_row = invoke(['g++', *shlex.split(contract['FLAGS'].split(' -I', 1)[0]),
                           '-rdynamic', '-g', '-o', str(binary), str(test_object),
                           *map(str, objects), *shlex.split(contract['LIBS'])],
                          attempt / 'link', cwd=source / 'src')
        sys.path.insert(0, str(source / 'tests/async'))
        import test_flatfile_combat_journey as journey
        outcomes = []
        for mode in ('present', 'missing'):
            root = attempt / mode
            runtime = root / 'runtime'
            state = root / 'state'
            runtime.mkdir(parents=True)
            state.mkdir(mode=0o700)
            (runtime / 'logs/log').mkdir(parents=True)
            (runtime / 'logs/player-log').mkdir(parents=True)
            # Original disposable mini-world builder; fixture files, not source.
            journey.make_fixture(runtime)
            for name in ('players', 'critical'):
                (runtime / 'journals' / name).mkdir(parents=True, mode=0o700)
            mobile_file = runtime / 'areas_mini/mini.mob'
            mobiles = mobile_file.read_text()
            for index in range(64):
                mobiles = mobiles.replace('$~',
                    f'#{22810 + index}\nrealalc{index:02d} alchemist~\n'
                    f'the fixture alchemist {index:02d}~\n'
                    f'The fixture alchemist {index:02d} stands here.\n~\n~\n'
                    '2 0 0 0 0 0 0 0 S\nPH 0 524288 -1\n'
                    '6 0 100 10d1+20 1d1+0\n0.0.0.0 0\n8 8 0\n$~')
            mobile_file.write_text(mobiles)
            object_file = runtime / 'areas_mini/mini.obj'
            objects_text = object_file.read_text()
            objects_text = re.sub(r'(?ms)^#102\n.*?(?=^#\d+\n|^\$~)', '', objects_text)
            if mode == 'present':
                vial = None
                for path in sorted((source / 'areas/obj').glob('*.obj')):
                    match = re.search(r'(?ms)^#102\n.*?(?=^#\d+\n|^\$~|\Z)', path.read_text())
                    if match:
                        vial = match.group(0)
                        break
                if vial is None:
                    raise RuntimeError('Original authored vial unavailable')
                objects_text = objects_text.replace('$~', vial + '$~')
            bag = ('#22900\nfixturebag container~\na fixture container~\n'
                   'A fixture container is here.~\n~\n'
                   '15 2 3 1 7 0 0 1 0 0 0\n10000 0 0 0 0 0 0 0\n1 0 100\n')
            objects_text = objects_text.replace('$~', bag + '$~')
            blocks = re.findall(r'(?ms)^#\d+\n.*?(?=^#\d+\n|^\$~)', objects_text)
            object_file.write_text(''.join(sorted(blocks, key=lambda block: int(block.split('\n', 1)[0][1:]))) + '$~\n')
            zone_file = runtime / 'areas_mini/mini.zon'
            zone = zone_file.read_text().split('\n', 4)
            resets = ''.join(f'M 0 {22810 + index} 1 22800 100 0 0 0\n' for index in range(64))
            zone_file.write_text('\n'.join(zone[:4]) + '\n' + resets + 'S\n$~\n')
            env = dict(PATH=os.environ.get('PATH', '/usr/bin:/bin'), ENVIRONMENT='local',
                       PERSISTENCE_MODE='flatfile-primary', FLATFILE_STATE_DIR=str(state),
                       PLAYER_SAVE_JOURNAL_DIR=str(runtime / 'journals/players'),
                       CRITICAL_COMMAND_JOURNAL_DIR=str(runtime / 'journals/critical'),
                       REDIS='FALSE', CHAOS_MUD='FALSE')
            # Preserve original native publication 64 MiB stack and 60s bound.
            soft, hard = resource.getrlimit(resource.RLIMIT_STACK)
            resource.setrlimit(resource.RLIMIT_STACK, (65536 * 1024, hard))
            row = invoke(['timeout', '--signal=TERM', '--kill-after=5s', '60s',
                          str(binary), mode], attempt / ('run-' + mode), cwd=runtime, env=env)
            outcomes.append(dict(mode=mode, **row))
        receipt.update(status='PASS_BOUNDED_ORIGINAL_INACTIVE_RESET_GRANT',
                       original_make_objects=len(objects), compile=compile_row,
                       link=link_row, outcomes=outcomes, binary_sha256=digest(binary),
                       test_source_sha256=digest(test_source), source_inputs_readonly=True,
                       providers_readonly=True, actual_class_rng_uid_factory=True,
                       current_latch_retry_only=True, final_forest_canonical_round_trip=True)
    except BaseException as failure:
        receipt.update(status='FAILED', reason=type(failure).__name__ + ': ' + str(failure))
        raise
    finally:
        receipt_path.write_text(json.dumps(receipt, indent=2) + '\n')
        print(json.dumps(dict(status=receipt['status'], receipt=str(receipt_path))), flush=True)


if __name__ == '__main__':
    main()
