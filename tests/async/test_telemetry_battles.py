#!/usr/bin/env python3
"""Qualify bounded battle association, configuration cuts and censored lifetimes."""

import argparse
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main(sanitize=False):
    artifacts = ROOT / 'bin/tests'
    artifacts.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='telemetry-battle-', dir=artifacts) as directory:
        executable = Path(directory) / 'telemetry-battle'
        command = ['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-pedantic',
                   '-I', str(ROOT / 'src'),
                   str(ROOT / 'tests/async/telemetry_battle_harness.cc'),
                   str(ROOT / 'src/telemetry/telemetry_battle.c'),
                   str(ROOT / 'src/telemetry/telemetry_battle_contract.c'), '-o', str(executable)]
        if sanitize:
            command += ['-g', '-fno-omit-frame-pointer', '-fsanitize=address,undefined']
        subprocess.run(command, check=True, timeout=120)
        subprocess.run([str(executable)], check=True, timeout=30)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    main(parser.parse_args().sanitize)
