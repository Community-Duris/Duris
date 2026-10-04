#!/usr/bin/env python3
"""Qualify disjoint battle contribution segments and explicit association bases."""

import argparse
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def compile_harness(executable, sanitize=False, optimize=False):
    command = ['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-pedantic',
               '-I', str(ROOT / 'src'),
               str(ROOT / 'tests/async/telemetry_battle_contribution_harness.cc'),
               str(ROOT / 'src/telemetry/telemetry_battle_contribution.c'),
               str(ROOT / 'src/telemetry/telemetry_control.c'),
               str(ROOT / 'src/telemetry/telemetry_battle.c'),
               str(ROOT / 'src/telemetry/telemetry_battle_contract.c'), '-o', str(executable)]
    if sanitize:
        command += ['-g', '-fno-omit-frame-pointer', '-fsanitize=address,undefined']
    if optimize:
        command += ['-O2']
    subprocess.run(command, check=True, timeout=120)


def main(sanitize=False):
    artifacts = ROOT / 'bin/tests'
    artifacts.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='telemetry-contributions-', dir=artifacts) as directory:
        executable = Path(directory) / 'telemetry-contributions'
        compile_harness(executable, sanitize)
        subprocess.run([str(executable)], check=True, timeout=60)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    main(parser.parse_args().sanitize)
