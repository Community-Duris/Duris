#!/usr/bin/env python3
"""Real materialization admission must budget the serialized identity reads."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'src/player/player_load_materialize.c').read_text()
includes = source[:source.index('\nnamespace\n')]
start = source.index('bool valid_snapshot(')
end = source.index('\n// Remembers the characters', start)
admission = source[start:end]
main = r'''
#include <cstdio>
#include <cstdlib>

static void require(bool ok, const char *why)
{
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", why); std::exit(1); }
}

int main()
{
    player_load_result result = {};
    result.outcome = player_load_outcome::applied;
    result.pid = 7;
    result.snapshot.pid = result.pid;
    result.snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    result.snapshot.components = PLAYER_LOAD_SESSION03_COMPONENTS;
    result.read_components = PLAYER_LOAD_SESSION04_READS;
    result.snapshot.save_intent = RENT_CRASH;
    for (unsigned field = static_cast<unsigned>(player_status_field::class_primary);
         field <= static_cast<unsigned>(player_status_field::last_ip); ++field) {
        player_snapshot_integer entry = {};
        entry.field = static_cast<player_status_field>(field);
        result.snapshot.status_integers.push_back(entry);
    }
    for (unsigned field = static_cast<unsigned>(player_status_string_field::name);
         field <= static_cast<unsigned>(player_status_string_field::poof_out); ++field) {
        player_snapshot_string entry = {};
        entry.field = static_cast<player_status_string_field>(field);
        result.snapshot.status_strings.push_back(entry);
    }
    require(valid_snapshot(result), "healthy fixture is invalid before query accounting");
    result.metrics.query_count = PLAYER_LOAD_PID_QUERY_MAX;
    require(valid_snapshot(result), "healthy bounded PID load rejected after identity lock");
    result.metrics.query_count = PLAYER_LOAD_QUERY_MAX;
    require(valid_snapshot(result), "healthy bounded name load rejected after identity lookup");
    result.metrics.query_count = PLAYER_LOAD_QUERY_MAX + 1;
    require(!valid_snapshot(result), "unexpected extra query escaped the bounded admission gate");
    result.metrics.query_count = 29;
    ++result.snapshot.schema_version;
    require(!valid_snapshot(result), "query budget change bypassed schema validation");
    std::puts("PASS: actual materializer accepts bounded PID/name identity reads and rejects excess/schema drift");
}
'''
with tempfile.TemporaryDirectory(prefix='load-query-budget-') as directory:
    cpp = Path(directory) / 'probe.cpp'
    binary = Path(directory) / 'probe'
    cpp.write_text(includes + '\n' + admission + '\n' + main)
    flags = shlex.split(subprocess.check_output(['mysql_config', '--cflags'], text=True))
    subprocess.run([os.getenv('CXX', 'g++'), '-std=c++20', '-Wall', '-Wextra', '-I'+str(ROOT/'src'),
                    *flags, str(cpp), '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT, check=True)
