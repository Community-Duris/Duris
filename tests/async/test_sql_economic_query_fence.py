#!/usr/bin/env python3
"""The actual main SQL executor must not issue a query after authority loss."""
from pathlib import Path
import os
import subprocess
import tempfile
from _paths import ROOT, source

PREFIX = r'''
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <strings.h>
struct MYSQL {} database;
MYSQL *DB = &database;
struct persistence_query_site { const char *file; const char *function; int line; };
bool permit = true;
int executed = 0, drained = 0, probed = 0, panicked = 0;
void sql_clear_results_on(MYSQL *conn) { assert(conn == DB); ++drained; }
bool duris_sql_exclusion_guard_allows(MYSQL *conn) {
    assert(conn == DB); ++probed; return permit;
}
void sql_trace_panic() { ++panicked; }
int sql_current_context() { return 0; }
bool sql_observed_execute_at(MYSQL *conn, persistence_query_site, int,
                            const char *, size_t, uint64_t *) {
    assert(conn == DB); ++executed; return true;
}
'''
SUFFIX = r'''
int main() {
    persistence_query_site site{"test", "mutation", 1};
    permit = false;
    assert(!sql_trace_exec_at(site, "write", "UPDATE probe SET v=1", 20, true, false));
    assert(probed == 1 && drained == 1 && executed == 0);
    permit = true;
    assert(sql_trace_exec_at(site, "write", "UPDATE probe SET v=1", 20, true, true));
    assert(probed == 2 && drained == 3 && executed == 1);
    permit = false;
    const int before_cleanup = executed;
    assert(!sql_trace_exec_at(site, "commit", "COMMIT", 6, true, false));
    assert(executed == before_cleanup);
    assert(sql_trace_exec_at(site, "rollback", "ROLLBACK", 8, true, false));
    assert(sql_trace_exec_at(site, "rollback", "rollback;", 9, true, false));
    assert(executed == before_cleanup + 2);
    const char stacked[] = "ROLLBACK; UPDATE probe SET v=1";
    assert(!sql_trace_exec_at(site, "stacked", stacked, sizeof(stacked)-1, true, false));
    assert(!sql_trace_exec_at(site, "savepoint", "ROLLBACK TO x", 13, true, false));
    assert(executed == before_cleanup + 2);
    const int before_missing = probed;
    DB = nullptr;
    assert(!sql_trace_exec_at(site, "write", "UPDATE probe SET v=1", 20, true, false));
    assert(probed == before_missing && executed == before_cleanup + 2);
    std::puts("PASS: real main-query executor refuses lost runtime SQL authority");
}
'''

def main():
    text = source('sql/sql.c').read_text()
    start = text.rindex('bool sql_trace_exec_at(')
    depth = 0
    body = None
    for end in range(text.index('{', start), len(text)):
        depth += (text[end] == '{') - (text[end] == '}')
        if depth == 0:
            body = text[start:end + 1]
            break
    assert body is not None, 'main SQL executor not found'
    with tempfile.TemporaryDirectory(prefix='duris-economic-query-fence-') as directory:
        cpp = Path(directory) / 'fence.cpp'; exe = Path(directory) / 'fence'
        cpp.write_text(PREFIX + body + SUFFIX)
        subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++20', '-Wall', '-Wextra',
                        '-Werror', '-g', '-O0', '-fsanitize=address,undefined',
                        '-fno-pie', '-no-pie', str(cpp), '-o', str(exe)], check=True,
                       cwd=ROOT, timeout=60)
        subprocess.run([str(exe)], check=True, timeout=30)

if __name__ == '__main__':
    main()
