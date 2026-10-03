#!/usr/bin/env python3
"""Execute the production selector; never connect to or mutate a database.

The native terminal owner is selectable ONLY by a TEST_MUD SQL build in an
explicitly disposable loopback combat fixture. Ordinary SQL and flatfile
selection must remain unchanged, including journal replay and worker setup.
"""
from pathlib import Path
import itertools
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'src/player/player_save_pipeline.c'
text = SOURCE.read_text()
start = text.index('player_save_apply_fn selected_snapshot_apply()')
end = text.index('\nstruct terminal_fence', start)
body = text[start:end]
assert text.count('player_save_journal_replay(selected_snapshot_apply(), nullptr)') == 1
assert 'player_save_worker_init(selected_snapshot_apply(),' in text

preamble = r'''
#include <cstdlib>
#include <cstring>
#include <iostream>
using player_save_apply_fn = void (*)();
void flatfile_player_snapshot_apply_selected() {}
void player_snapshot_repository_apply_from_pool() {}
void player_death_conflict_apply_from_pool() {}
'''
main = r'''
int main()
{
    const auto fn = selected_snapshot_apply();
    std::cout << (fn == flatfile_player_snapshot_apply_selected ? "flatfile" :
                  fn == player_snapshot_repository_apply_from_pool ? "ordinary" :
                  fn == player_death_conflict_apply_from_pool ? "native" : "invalid") << '\n';
}
'''
compiler = os.environ.get('CXX') or shutil.which('g++-14') or shutil.which('g++')
assert compiler, 'C++ compiler unavailable'
keys = ('DURIS_TEST_SQL_DEATH_CONFLICT_RECOVERY', 'TEST_DB_DISPOSABLE', 'DB_HOST', 'DB_NAME')
clean = {key: value for key, value in os.environ.items() if key not in keys}
valid: dict[str, str] = dict(zip(keys, ('1', '1', '127.0.0.1', 'corpse_journey_test_012345abcdef')))
cases: list[tuple[str, dict[str, str]]] = [('no-environment', {}), ('fully-qualified', valid)]
for key, values in (
    (keys[0], (None, '', '0', 'true', '01', '1\n')),
    (keys[1], (None, '', '0', 'true', '01', '1\n')),
    ('DB_HOST', (None, '', 'localhost', '::1', 'host.docker.internal', '127.0.0.1.evil')),
    ('DB_NAME', (None, '', 'duris', 'duris_prod', 'duris_dev', 'corpse_journey_test_',
                 'corpse_journey_test_012345abcde', 'corpse_journey_test_012345abcdef0',
                 'corpse_journey_test_012345abcdeg', 'corpse_journey_test_012345ABCDEf',
                 'corpse_journey_test_012345abcdef\n')),
):
    for value in values:
        case = valid.copy()
        case.pop(key) if value is None else case.update({key: value})
        cases.append((key + '=' + repr(value), case))

count = 0
with tempfile.TemporaryDirectory(prefix='death-owner-selector-') as directory:
    temporary = Path(directory)
    source = temporary / 'selector.cpp'
    source.write_text(preamble + body + main)
    for mysql, test_mud in itertools.product((True, False), repeat=2):
        output = temporary / ('selector-' + str(mysql) + '-' + str(test_mud))
        flags = ([] if mysql else ['-D__NO_MYSQL__']) + (['-DTEST_MUD'] if test_mud else [])
        subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', *flags,
                        str(source), '-o', str(output)], check=True)
        for label, environment in cases:
            valid_runtime = (environment.get(keys[0]) == '1' and
                             environment.get(keys[1]) == '1' and
                             environment.get('DB_HOST') in ('127.0.0.1', 'localhost') and
                             environment.get('DB_NAME') == valid['DB_NAME'])
            expected = 'flatfile' if not mysql else ('native' if test_mud and valid_runtime else 'ordinary')
            actual = subprocess.check_output([str(output)], env={**clean, **environment}, text=True).strip()
            assert actual == expected, (mysql, test_mud, label, expected, actual)
            count += 1
print(f'PASS: {count} real-selector cases; normal SQL/flatfile selection unchanged; no DB actions')
