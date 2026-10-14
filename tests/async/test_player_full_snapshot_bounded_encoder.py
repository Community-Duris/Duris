#!/usr/bin/env python3
"""Source-bound whole-codec GNU13 production regression, run only by ROOT.

Build the original b1bd codec against its complete original provider/header tree
and independently emit golden bytes. Build source214's whole maintained codec
with the same harness and ordinary production flags; compare complete wires,
roundtrips, every callback denial and strong-output controls. Also compile/run
the unchanged actual item-codec harness. No stubs, extracted providers,
sanitizer-policy bypasses, Make edits, SQL, live ownership or gameplay claims.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import shlex
import shutil
import subprocess
import time


CANONICAL_BASE = 'b1bd868afc2bcf3e80f1837aec55674cb4052c4d'
SOURCE214 = 'e9f9e9b6bc4ead1d0309b43a2fe67fabf5008c80314b1b7c18217f082013d4b2'
SOURCE_MANIFEST_FILE = '7d675cd929e285e3fbb0e3e2880072373f75b56b7f42ee2ae77de471ed11244b'
REFERENCE_CLOSURE = 'ae29ab7a4c5056fab0b7940d29deeb9763cb5a5dee875a74baa9972eb5cb06b8'
SOURCE_CLOSURE = 'b9255c67789d4591309bd7c9d728394c8d30205545d05faadeaae94279e4de14'
REFERENCE_CODEC = '2e8265999eb8bc23e6c48ec68578684d946451ed0c83b62c6354305a2922b8b3'
REFERENCE_HEADER = '0837110a98385f1419bf9eb92fe874403532dbd49b11590e1111af32cc2e078d'
HARNESS = 'tests/async/player_full_snapshot_bounded_encoder_test.cpp'
RECIPE = 'tests/async/test_player_full_snapshot_bounded_encoder.py'
ITEM_HARNESS = 'tests/async/player_item_snapshot_codec_harness.cpp'
CODEC = 'src/player/player_snapshot_codec.c'
HEADER = 'src/player/player_snapshot_codec.h'
COMPILE_TIMEOUT = 300
BASELINE_TIMEOUT = 30
BOUNDED_TIMEOUT = 180
ITEM_TIMEOUT = 30
AGGREGATE_TIMEOUT = 600


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def identity(pins):
    rows = ''.join(f'{name}\t{sha}\n' for name, sha in sorted(pins.items()))
    return hashlib.sha256(rows.encode()).hexdigest()


def verify(root, pins):
    for name, sha in pins.items():
        candidate = (root / name).resolve()
        if not candidate.is_relative_to(root) or digest(candidate) != sha:
            raise RuntimeError(f'raw source pin mismatch: {name}')


def make_flags(text, name):
    # Read these exact maintained production assignments, including their full
    # continuation lines. No test-specific warning or Source-policy exceptions.
    logical = text.replace('\\\n', ' ')
    match = re.search(r'^' + re.escape(name) + r'\s*=\s*(.+)$', logical, re.MULTILINE)
    if not match:
        raise RuntimeError(f'maintained flag assignment missing: {name}')
    return shlex.split(match[1])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, required=True)
    parser.add_argument('--source-manifest', type=Path, required=True)
    parser.add_argument('--reference-root', type=Path, required=True)
    parser.add_argument('--reference-manifest', type=Path, required=True)
    parser.add_argument('--test-manifest', type=Path, required=True,
                        help='frozen SHOP delivery-receipt.json with both test postimage hashes')
    parser.add_argument('--evidence', type=Path, required=True,
                        help='new isolated D: test evidence directory (or direct container mount)')
    parser.add_argument('--bin-root', type=Path, required=True,
                        help='separate new D:/Dev/Builds/Duris/<job>/bin directory or direct container mount')
    args = parser.parse_args()
    source = args.source_root.resolve()
    reference = args.reference_root.resolve()
    evidence = args.evidence.resolve()
    binaries = args.bin_root.resolve()
    if (evidence.exists() or binaries.exists() or evidence.is_relative_to(source) or
            evidence.is_relative_to(reference) or binaries.is_relative_to(source) or
            binaries.is_relative_to(reference) or evidence.is_relative_to(binaries) or
            binaries.is_relative_to(evidence)):
        raise RuntimeError('build/evidence paths must be new, distinct and outside both immutable source roots')
    evidence.mkdir(parents=True)
    binaries.mkdir(parents=True)
    metadata = {
        'status': 'PREPARING', 'canonical_base': CANONICAL_BASE, 'source214': SOURCE214,
        'reference_closure': REFERENCE_CLOSURE, 'source_closure': SOURCE_CLOSURE,
        'limits_seconds': {'each_compile': COMPILE_TIMEOUT, 'baseline': BASELINE_TIMEOUT,
                           'bounded': BOUNDED_TIMEOUT, 'item': ITEM_TIMEOUT,
                           'aggregate': AGGREGATE_TIMEOUT},
        'steps': [], 'qualification': 'Whole original/current codec value regression only; '
                                     'no live owner, SQL, Source admission, activation or gameplay proof.',
        'bin_root': str(binaries), 'evidence_root': str(evidence),
    }
    pins = reference_pins = test_pins = None
    started = time.monotonic()
    deadline = started + AGGREGATE_TIMEOUT
    try:
        source_manifest = json.loads(args.source_manifest.read_text())
        reference_manifest = json.loads(args.reference_manifest.read_text())
        test_manifest = json.loads(args.test_manifest.read_text())
        if (digest(args.source_manifest) != SOURCE_MANIFEST_FILE or
                source_manifest['canonical_base'] != CANONICAL_BASE or
                source_manifest['manifest_sha256'] != SOURCE214 or
                identity(source_manifest['files']) != SOURCE214 or
                reference_manifest['canonical_base'] != CANONICAL_BASE):
            raise RuntimeError('wrong immutable canonical source identity')
        pins = {name: sha for name, sha in source_manifest['files'].items() if name.startswith('src/')}
        reference_pins = reference_manifest['files']
        if (identity(pins) != SOURCE_CLOSURE or identity(reference_pins) != REFERENCE_CLOSURE or
                reference_pins[CODEC] != REFERENCE_CODEC or reference_pins[HEADER] != REFERENCE_HEADER or
                set(pins) != set(reference_pins) or
                sorted(name for name in pins if pins[name] != reference_pins[name]) != [CODEC, HEADER]):
            raise RuntimeError('baseline/current complete provider closure or two-file boundary mismatch')
        verify(source, pins)
        verify(reference, reference_pins)
        # This is the original maintained item harness, not a new helper fixture.
        if digest(source / ITEM_HARNESS) != source_manifest['files'][ITEM_HARNESS]:
            raise RuntimeError('actual original item-codec harness changed')
        test_pins = {name: test_manifest['files'][name]['postimage_sha256'] for name in (HARNESS, RECIPE)}
        test_root = Path(__file__).resolve().parents[2]
        verify(test_root, test_pins)
        metadata.update({'source_manifest_sha256': digest(args.source_manifest),
                         'reference_manifest_sha256': digest(args.reference_manifest),
                         'test_manifest_sha256': digest(args.test_manifest),
                         'source_files': len(pins), 'reference_files': len(reference_pins),
                         'test_pins': test_pins, 'item_harness_sha256': digest(source / ITEM_HARNESS)})
        compiler = shutil.which(os.environ.get('CXX', 'g++-13'))
        if not compiler:
            raise RuntimeError('ordinary GNU13 compiler executable is unavailable')
        version = subprocess.check_output([compiler, '-dumpfullversion'], text=True, timeout=10).strip()
        machine = subprocess.check_output([compiler, '-dumpmachine'], text=True, timeout=10).strip()
        if not version.startswith('13.') or not machine.startswith('x86_64'):
            raise RuntimeError(f'unsupported ordinary production toolchain: {version} {machine}')
        makefile = (source / 'src/Makefile').read_text()
        flags = ['-g', '-std=c++20', *make_flags(makefile, 'HARDENING_FLAGS'),
                 *make_flags(makefile, 'WARNING_FLAGS'), '-D__NO_TESTS__', '-DDURIS_PERSISTENT_TRANSPORT']
        metadata.update({'compiler': compiler, 'compiler_version': version, 'compiler_machine': machine,
                         'maintained_makefile_sha256': digest(source / 'src/Makefile'),
                         'production_flags': flags})
        (evidence / 'dependency-closure.json').write_text(json.dumps({
            'canonical_base': CANONICAL_BASE, 'source214': SOURCE214,
            'reference': reference_pins, 'current': pins, 'test_pins': test_pins,
            'item_harness': {ITEM_HARNESS: digest(source / ITEM_HARNESS)},
        }, indent=2) + '\n')

        def step(name, command, timeout, cwd):
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError('aggregate regression deadline exceeded')
            effective = min(timeout, remaining)
            row = {'name': name, 'command': [str(value) for value in command],
                   'cwd': str(cwd), 'timeout_seconds': effective}
            metadata['steps'].append(row)
            begin = time.monotonic()
            try:
                with (evidence / (name + '.log')).open('wb') as log:
                    process = subprocess.run(command, cwd=cwd, stdout=log, stderr=subprocess.STDOUT,
                                             timeout=effective, check=False)
                row['exit'] = process.returncode
                if process.returncode:
                    raise RuntimeError(f'{name} failed; see its preserved log')
            except subprocess.TimeoutExpired:
                row['status'] = 'TIMEOUT'
                raise
            finally:
                row['seconds'] = time.monotonic() - begin

        harness = test_root / HARNESS
        golden = evidence / 'original-golden'
        baseline_binary = binaries / 'original_full_encoder'
        bounded_binary = binaries / 'bounded_full_encoder'
        item_binary = binaries / 'original_item_codec'

        def compile_command(root, cpp, binary, extra=()):
            # Compile the ENTIRE maintained translation unit. No provider
            # extraction, --gc-sections, link stubs or alternate implementations.
            return [compiler, *flags, *extra, '-I', str(root / 'src'),
                    '-I', str(root / 'tests/async'), '-I/usr/include/libxml2', '-I/usr/include/mysql',
                    '-x', 'c++', str(cpp), str(root / CODEC), '-o', str(binary)]

        step('compile_original_full', compile_command(reference, harness, baseline_binary,
                                                      ('-DDURIS_FULL_ENCODER_BASELINE=1',)),
             COMPILE_TIMEOUT, reference)
        step('emit_independent_original_golden', [baseline_binary, golden], BASELINE_TIMEOUT, reference)
        golden_pins = {path.name: digest(path) for path in sorted(golden.glob('*.bin'))}
        if len(golden_pins) != 26:
            raise RuntimeError('original independent golden set is incomplete')
        metadata['original_golden_pins'] = golden_pins
        metadata['baseline_binary_sha256'] = digest(baseline_binary)
        step('compile_bounded_full', compile_command(source, harness, bounded_binary), COMPILE_TIMEOUT, source)
        metadata['bounded_binary_sha256'] = digest(bounded_binary)
        step('complete_wire_and_all_denials', [bounded_binary, golden], BOUNDED_TIMEOUT, source)
        if golden_pins != {path.name: digest(path) for path in sorted(golden.glob('*.bin'))}:
            raise RuntimeError('independent original golden bytes changed during current test')
        step('compile_original_item_codec', compile_command(source, source / ITEM_HARNESS, item_binary),
             COMPILE_TIMEOUT, source)
        metadata['item_binary_sha256'] = digest(item_binary)
        step('actual_original_item_codec', [item_binary], ITEM_TIMEOUT, source)
        verify(source, pins)
        verify(reference, reference_pins)
        verify(test_root, test_pins)
        metadata['source_unchanged'] = True
        metadata['status'] = 'PASS_WHOLE_CODEC_INDEPENDENT_GOLDEN_AND_FINITE_CONTROLS'
    except Exception as failure:
        metadata['status'] = 'FAIL'
        metadata['error'] = f'{type(failure).__name__}: {failure}'
        raise
    finally:
        metadata['seconds'] = time.monotonic() - started
        (evidence / 'result.json').write_text(json.dumps(metadata, indent=2) + '\n')
        print(json.dumps({'status': metadata['status'], 'receipt': str(evidence / 'result.json')}))


if __name__ == '__main__':
    main()
