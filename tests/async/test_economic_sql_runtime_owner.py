#!/usr/bin/env python3
"""Real runtime owner lifetime/cleanup, with controlled SQL/guard interfaces."""
from pathlib import Path
import os
import subprocess
import tempfile
from _paths import ROOT

ENV = r'''
#ifndef OWNER_TEST_ENV_H
#define OWNER_TEST_ENV_H
#include <cassert>
#include <new>
#include <string>
#include <vector>
struct MYSQL { int id = 1; };
extern bool fail_open, fail_acquire, fail_bind, fail_recover, fail_constructor, locked;
extern int opened, closed, acquired, released, bound, recovered;
extern std::vector<std::string> events;
void mysql_close(MYSQL *);
#endif
'''
GUARD = r'''
#ifndef OWNER_TEST_GUARD_H
#define OWNER_TEST_GUARD_H
#include "owner_test_env.h"
class economic_sql_lifecycle_guard {
    bool held = false;
public:
    economic_sql_lifecycle_guard() { if (fail_constructor) throw std::bad_alloc(); }
    ~economic_sql_lifecycle_guard() {
        if (held) { assert(locked); locked = false; ++released; events.push_back("release"); }
    }
    static unsigned int acquire_runtime(MYSQL *conn, economic_sql_lifecycle_guard *out) noexcept {
        assert(conn && conn->id == 1 && !locked); ++acquired; events.push_back("acquire");
        if (fail_acquire) return 1;
        out->held = true; locked = true; return 0;
    }
};
#endif
'''
BIND = r'''
#include "owner_test_env.h"
inline bool duris_sql_exclusion_guard_bind_economic_runtime(MYSQL *conn) {
    assert(conn && conn->id == 1 && locked); ++bound; events.push_back("bind");
    return !fail_bind;
}
'''
LIFECYCLE = r'''
#include "owner_test_env.h"
class economic_sql_lifecycle_guard;
class economic_sql_accounting_lifecycle_transaction {
public:
    static unsigned int recover_runtime(MYSQL *conn, const economic_sql_lifecycle_guard &,
                                        bool *active) noexcept {
        assert(conn && conn->id == 1 && locked && active);
        ++recovered; *active = false; return fail_recover ? 5 : 0;
    }
};
'''
GAMEPLAY = r'''
class economic_gameplay_authority {
public:
    static bool active() noexcept { return false; }
    static void clear_sql_runtime() noexcept {}
};
'''
DRIVER = r'''
#include "owner_test_env.h"
#include "sql/sql_economic_runtime.h"
#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>
bool fail_open = false, fail_acquire = false, fail_bind = false, fail_recover = false,
     fail_constructor = false, locked = false;
int opened = 0, closed = 0, acquired = 0, released = 0, bound = 0, recovered = 0;
std::vector<std::string> events;
MYSQL *sql_open_configured_connection(unsigned long flags) {
    assert(flags == 0); ++opened; events.push_back("open");
    return fail_open ? nullptr : new MYSQL;
}
void mysql_close(MYSQL *conn) {
    assert(conn && !locked); ++closed; events.push_back("close"); delete conn;
}
void reset() {
    sql_economic_runtime_shutdown(); assert(!locked);
    fail_open = fail_acquire = fail_bind = fail_recover = fail_constructor = false;
    opened = closed = acquired = released = bound = recovered = 0; events.clear();
}
int main() {
    reset(); fail_open = true;
    assert(!sql_economic_runtime_start());
    assert(opened == 1 && acquired == 0 && bound == 0 && closed == 0);
    reset(); fail_acquire = true;
    assert(!sql_economic_runtime_start());
    assert(opened == 1 && acquired == 1 && released == 0 && closed == 1 && !locked);
    reset(); fail_bind = true;
    assert(!sql_economic_runtime_start());
    assert((events == std::vector<std::string>{"open", "acquire", "bind", "release", "close"}));
    reset(); fail_recover = true;
    assert(!sql_economic_runtime_start());
    assert(recovered == 1 && !locked && released == 1 && closed == 1);
    reset(); fail_constructor = true;
    assert(!sql_economic_runtime_start());
    assert(opened == 1 && acquired == 0 && closed == 1 && !locked);
    reset(); assert(sql_economic_runtime_start());
    assert(locked && opened == 1 && acquired == 1 && bound == 1 && recovered == 1 && closed == 0);
    assert(!sql_economic_runtime_start());
    assert(locked && opened == 1 && acquired == 1 && bound == 1 && closed == 0);
    pid_t child = fork(); assert(child >= 0);
    if (!child) {
        sql_economic_runtime_shutdown();
        assert(locked && released == 0 && closed == 0);
        _exit(0);
    }
    int status = 0; assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    assert(locked && released == 0 && closed == 0);
    sql_economic_runtime_shutdown();
    assert(!locked && released == 1 && closed == 1);
    assert((events == std::vector<std::string>{"open", "acquire", "bind", "release", "close"}));
    sql_economic_runtime_shutdown(); assert(released == 1 && closed == 1);
    std::puts("PASS: dedicated runtime owner retain, refusal, allocation cleanup, fork and release ordering");
}
'''
NO_MYSQL = r'''
#include "sql/sql_economic_runtime.h"
#include <cassert>
int main() { assert(!sql_economic_runtime_start()); sql_economic_runtime_shutdown(); }
'''

def main():
    with tempfile.TemporaryDirectory(prefix='duris-economic-runtime-owner-') as directory:
        path = Path(directory)
        for name, content in {'owner_test_env.h': ENV,
                              'persistence/economic_sql_lifecycle_guard.h': GUARD,
                              'persistence/economic_sql_accounting_lifecycle_transaction.h': LIFECYCLE,
                              'economy/economic_gameplay_authority.h': GAMEPLAY,
                              'sql/sql_exclusion_guard.h': BIND,
                              'owner.cpp': DRIVER, 'no_mysql.cpp': NO_MYSQL}.items():
            dest = path / name; dest.parent.mkdir(parents=True, exist_ok=True); dest.write_text(content)
        for label, defines in [('owner', []), ('no_mysql', ['-D__NO_MYSQL__'])]:
            exe = path / label
            subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++20', '-g', '-O0',
                            '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
                            '-fno-omit-frame-pointer', '-fno-pie', '-no-pie', *defines,
                            '-I'+str(path), '-I'+str(ROOT/'src'), str(path/(label+'.cpp')),
                            str(ROOT/'src/sql/sql_economic_runtime.c'), '-o', str(exe)],
                           check=True, cwd=ROOT, timeout=60)
            subprocess.run([str(exe)], check=True, timeout=30,
                           env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=1:halt_on_error=1',
                                'UBSAN_OPTIONS': 'halt_on_error=1'})

if __name__ == '__main__':
    main()
