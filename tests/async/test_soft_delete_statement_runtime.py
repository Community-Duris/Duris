#!/usr/bin/env python3
"""Run the production soft-delete body, optionally against disposable MariaDB.

Default: deterministic statement/transaction failure injection under sanitizers.
--mariadb-fixture: root with no password at 127.0.0.1, database pr204_fixture.
Use only an isolated disposable server: this mode creates temporary tables only.
"""
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'src/sql/sql.c').read_text()
start = source.index('bool sql_soft_delete_character(long pid)')
brace = source.index('{', start)
depth = 0
for end in range(brace, len(source)):
    depth += (source[end] == '{') - (source[end] == '}')
    if not depth:
        body = source[start:end + 1]
        break

prelude = r'''
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <string>
#ifdef LIVE_DB
#include <mysql/mysql.h>
static MYSQL *DB;
#else
static void *DB = reinterpret_cast<void*>(1);
#endif
static bool in_tx=false, begin_ok=true, commit_ok=true;
static int calls=0, fail_at=0, begins=0, commits=0, rollbacks=0;
static std::string statements[2];
void checked_snprintf(char *out, size_t n, const char *fmt, ...) {
    va_list ap; va_start(ap,fmt); vsnprintf(out,n,fmt,ap); va_end(ap);
}
bool execute(const char *sql) {
#ifdef LIVE_DB
    return mysql_real_query(DB,sql,strlen(sql))==0;
#else
    (void)sql; return true;
#endif
}
bool sql_in_transaction() {return in_tx;}
bool sql_begin_transaction() {
    ++begins;
    if(!begin_ok || !execute("START TRANSACTION"))return false;
    in_tx=true; return true;
}
bool sql_commit() {
    ++commits;
    if(!commit_ok || !execute("COMMIT"))return false;
    in_tx=false; return true;
}
bool sql_rollback() {++rollbacks; bool ok=execute("ROLLBACK"); in_tx=false; return ok;}
bool sql_trace_exec(const char*, const char *sql,size_t n,bool drain,bool after) {
    assert(n==strlen(sql) && drain && !after);
    assert(calls<2); statements[calls]=sql;
    ++calls;
    if(calls==fail_at)return false;
    return execute(sql);
}
// Model the existing result-set API: successful UPDATE also returns nullptr.
void *db_query(const char *fmt, long pid) {
    char sql[256]; snprintf(sql,sizeof(sql),fmt,pid); execute(sql); return nullptr;
}
void reset() {
    in_tx=false; begin_ok=commit_ok=true;
    calls=fail_at=begins=commits=rollbacks=0;
}
'''
main = r'''
#ifdef LIVE_DB
long scalar(const char *query) {
    assert(execute(query)); MYSQL_RES *r=mysql_store_result(DB); assert(r);
    MYSQL_ROW row=mysql_fetch_row(r); assert(row); long value=std::stol(row[0]);
    mysql_free_result(r); return value;
}
void seed() {
    assert(execute("DELETE FROM account_characters"));
    assert(execute("DELETE FROM frag_leaderboard"));
    assert(execute("INSERT INTO account_characters VALUES(1,NULL),(2,NULL)"));
    assert(execute("INSERT INTO frag_leaderboard VALUES(1,NULL),(2,NULL)"));
}
#endif
int main() {
#ifdef LIVE_DB
    DB=mysql_init(nullptr);
    assert(mysql_real_connect(DB,"127.0.0.1","root","","pr204_fixture",3306,nullptr,0));
    assert(execute("CREATE TEMPORARY TABLE account_characters (pid BIGINT PRIMARY KEY, deleted_at DATETIME NULL) ENGINE=InnoDB"));
    assert(execute("CREATE TEMPORARY TABLE frag_leaderboard (pid BIGINT PRIMARY KEY, deleted_at DATETIME NULL) ENGINE=InnoDB"));
    seed();
#endif
    reset(); assert(sql_soft_delete_character(1));
    assert(calls==2 && begins==1 && commits==1 && rollbacks==0 && !in_tx);
    assert(statements[0].find("account_characters")!=std::string::npos);
    assert(statements[1].find("frag_leaderboard")!=std::string::npos);
#ifdef LIVE_DB
    assert(mysql_field_count(DB)==0 && mysql_store_result(DB)==nullptr);
    assert(scalar("SELECT COUNT(*) FROM account_characters WHERE pid=1 AND deleted_at IS NOT NULL")==1);
    assert(scalar("SELECT COUNT(*) FROM frag_leaderboard WHERE pid=1 AND deleted_at IS NOT NULL")==1);
    assert(scalar("SELECT COUNT(*) FROM account_characters WHERE pid=2 AND deleted_at IS NULL")==1);
    assert(scalar("SELECT COUNT(*) FROM frag_leaderboard WHERE pid=2 AND deleted_at IS NULL")==1);
#endif
    // Repeated tombstones and absent leaderboard rows are successful no-ops.
    reset(); assert(sql_soft_delete_character(1));
    reset(); assert(sql_soft_delete_character(999));
    for(bool outer:{false,true}) {
        for(int failure:{0,1,2}) {
#ifdef LIVE_DB
            seed();
#endif
            reset();
            if(outer)assert(sql_begin_transaction());
            begins=0; fail_at=failure;
            assert(sql_soft_delete_character(1)==(failure==0));
            assert(begins==(outer?0:1));
            assert(commits==(!outer && !failure ? 1:0));
            assert(rollbacks==(!outer && failure ? 1:0));
            if(outer) {assert(in_tx); assert(sql_rollback());}
#ifdef LIVE_DB
            const long active=(outer || failure) ? 2:1;
            assert(scalar("SELECT COUNT(*) FROM account_characters WHERE deleted_at IS NULL")==active);
            assert(scalar("SELECT COUNT(*) FROM frag_leaderboard WHERE deleted_at IS NULL")==active);
#endif
        }
    }
    reset(); begin_ok=false; assert(!sql_soft_delete_character(1)); assert(!calls);
    reset(); commit_ok=false; assert(!sql_soft_delete_character(1)); assert(rollbacks==1);
    reset(); assert(!sql_soft_delete_character(0)); assert(!sql_soft_delete_character(-1)); assert(!begins);
#ifdef LIVE_DB
    // A real statement error after the first successful UPDATE must roll it back.
    seed(); assert(execute("DROP TEMPORARY TABLE frag_leaderboard"));
    reset(); assert(!sql_soft_delete_character(1)); assert(rollbacks==1);
    assert(scalar("SELECT COUNT(*) FROM account_characters WHERE deleted_at IS NULL")==2);
    mysql_close(DB);
#endif
    DB=nullptr; reset(); assert(!sql_soft_delete_character(1)); assert(!begins);
    puts("PASS: production soft-delete statement success, no-op, failure and transaction ownership");
}
'''
live = '--mariadb-fixture' in sys.argv
build_root = ROOT / 'bin/tests'
build_root.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='soft-delete-', dir=build_root) as directory:
    cpp = Path(directory) / 'runtime.cpp'
    cpp.write_text(prelude + body + main)
    exe = Path(directory) / 'runtime'
    flags = ['-DLIVE_DB', '-lmariadb'] if live else []
    subprocess.run(['g++', '-std=c++20', '-g', '-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer', str(cpp), '-o', str(exe), *flags], check=True)
    subprocess.run([str(exe)], check=True)
