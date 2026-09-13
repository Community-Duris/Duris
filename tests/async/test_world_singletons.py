#!/usr/bin/env python3
"""Execute singleton reconciliation, transport movement and recovery wire round trips."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
transport = (ROOT / 'src/world/transport.c').read_text()
prefix = transport[:transport.index('// Handles list command')]
movement = transport[transport.index('int do_simple_move_skipping_procs'):]
copyover = (ROOT / 'src/persistence/copyover.c').read_text()
mob_codec = copyover[copyover.index('int copyover_write_mob_to_buffer'):copyover.index('int copyover_write_obj_to_buffer')]
mob_codec += copyover[copyover.index('P_char copyover_restore_mob_from_buffer'):copyover.index('P_obj copyover_restore_obj_from_buffer')]
harness = (ROOT / 'tests/async/world_singletons_harness.cpp').read_text()
harness = harness.replace('// TRANSPORT_PRODUCTION', prefix + '\n' + movement + '\n' + mob_codec)
build = ROOT / 'bin/tests'
build.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='world-singletons-', dir=build) as tmp:
    source = Path(tmp) / 'harness.cpp'
    source.write_text(harness)
    binary = Path(tmp) / 'harness'
    subprocess.run(['g++', '-std=c++20', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie',
                    '-I', str(ROOT / 'src'), '-I/usr/include/mysql', '-I/usr/include/libxml2',
                    str(source), str(ROOT / 'src/world/world_singletons.c'),
                    str(ROOT / 'src/world/world_recovery_codec.c'), '-lbsd', '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=1:halt_on_error=1'})

comm = (ROOT / 'src/net/comm.c').read_text()
loop = comm[comm.index('void game_loop(int port, int sslport)'):]
assert comm.count('initialize_transport();') == 2  # declaration and post-recovery call
assert loop.index('redis_world_recovery_boot_clear();') < loop.index('initialize_transport();')
assert loop.index('copyover_recover(') < loop.index('reconcile_shopkeepers(')
copyover = (ROOT / 'src/persistence/copyover.c').read_text()
assert 'header.version != 12' in copyover
assert 'offsetof(copyover_mob, transport)' in copyover
assert copyover.count('transport_capture(mob, &entry.transport);') == 2
assert copyover.count('transport_restore(mob, mob_entry.transport);') == 2
print('world singleton lifecycle and recovery compatibility passed')
