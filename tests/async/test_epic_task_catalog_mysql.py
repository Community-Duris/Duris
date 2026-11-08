#!/usr/bin/env python3
"""Real SQL boot and paged catalogue reads, including retry and malformed rows."""
import os
from pathlib import Path
import shlex
import subprocess
import uuid

from _paths import ROOT, extract_function, rel

HARNESS = r'''
#include "world/epic_task_catalog.h"
#include "persistence/maintenance_scheduler.h"
#include "sql/sql.h"
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>

MYSQL *DB = nullptr;
static unsigned logs = 0;
void logit(const char *, const char *, ...) { ++logs; }
int real_zone0(int n) { return n == 999 ? -1 : n; }
int number(int from, int) { return from; }
uint64_t persistence_observability_now_usec() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
bool qry_at(persistence_query_site, const char *format, ...) {
    char sql[512]; va_list args; va_start(args,format);
    vsnprintf(sql,sizeof(sql),format,args); va_end(args);
    return mysql_query(DB,sql)==0;
}
// PRODUCTION_FUNCTIONS
void sql(const std::string &text) { assert(mysql_query(DB,text.c_str())==0); }
maintenance_result page(uint64_t cursor=0) {
    maintenance_request request = {};
    request.work_id=123; request.job_id=maintenance_job_id::epic_task_catalog;
    request.cursor=cursor; request.row_budget=256;
    request.deadline_usec=persistence_observability_now_usec()+5000000;
    return execute_catalog(DB,request);
}
int main() {
    DB=mysql_init(nullptr); assert(DB);
    assert(mysql_real_connect(DB,getenv("TEST_DB_HOST"),getenv("TEST_DB_USER"),
        getenv("TEST_DB_PASSWORD"),getenv("TEST_DB_NAME"),atoi(getenv("TEST_DB_PORT")),nullptr,0));
    sql("CREATE TABLE zones(number BIGINT NULL,task_zone TINYINT DEFAULT 1) ENGINE=InnoDB");
    assert(!epic_task_catalog_refresh());
    assert(std::string(epic_task_catalog_last_refresh())=="empty_task_flags");
    assert(!epic_task_catalog_ready());
    const auto first_logs=logs;
    assert(!epic_task_catalog_refresh() && logs==first_logs);
    std::string values="INSERT INTO zones(number) VALUES";
    for(int n=1;n<=300;++n) values+=(n==1?"":",")+std::string("(")+std::to_string(n)+")";
    sql(values);
    assert(epic_task_catalog_refresh() && epic_task_catalog_size()==300);

    auto first=page(); assert(first.outcome==maintenance_outcome::more && first.value_count==256);
    epic_task_catalog_complete(first);
    sql("RENAME TABLE zones TO temporarily_unavailable_zones");
    auto failed=page(first.next_cursor);
    assert(failed.outcome==maintenance_outcome::retryable_failure && failed.error_code);
    epic_task_catalog_complete(failed);
    sql("RENAME TABLE temporarily_unavailable_zones TO zones");
    auto last=page(first.next_cursor);
    assert(last.outcome==maintenance_outcome::complete && last.value_count==44);
    epic_task_catalog_complete(last);
    assert(epic_task_catalog_size()==300);
    sql("DELETE FROM zones");
    epic_task_catalog_complete(page());
    assert(epic_task_catalog_size()==300 && epic_task_catalog_ready());
    assert(std::string(epic_task_catalog_last_refresh())=="empty_task_flags");
    sql(values);

    // The old number>cursor query hid the second 256 at a page boundary.
    sql("INSERT INTO zones(number) VALUES(256)");
    auto duplicate=page();
    assert(duplicate.outcome==maintenance_outcome::permanent_failure && duplicate.error_code==EINVAL);
    epic_task_catalog_complete(duplicate);
    assert(epic_task_catalog_size()==300);
    assert(!epic_task_catalog_refresh());
    assert(std::string(epic_task_catalog_last_refresh())=="duplicate_or_unordered_zone");
    sql("DELETE FROM zones WHERE number=256 LIMIT 1");

    for(const char *bad : {"0","-1","NULL","2147483648","999"}) {
        sql(std::string("INSERT INTO zones(number) VALUES(")+bad+")");
        assert(!epic_task_catalog_refresh());
        auto result=page();
        epic_task_catalog_complete(result);
        if(result.outcome==maintenance_outcome::more) {
            result=page(result.next_cursor);
            epic_task_catalog_complete(result);
        }
        assert(epic_task_catalog_size()==300);
        assert(std::string(epic_task_catalog_last_refresh())!="ready");
        sql("DELETE FROM zones WHERE number IS NULL OR number<=0 OR number>300");
    }
    assert(epic_task_catalog_refresh());
    sql("DROP TABLE zones");
    assert(!epic_task_catalog_refresh());
    assert(std::string(epic_task_catalog_last_refresh())=="database_query_failed");
    assert(epic_task_catalog_size()==300);
    mysql_close(DB);
}
'''

if os.getenv('TEST_DB_HOST') not in ('127.0.0.1', 'localhost'):
    raise SystemExit('Run under tests/async/with_disposable_mariadb.sh')
database = 'task_catalog_' + uuid.uuid4().hex[:12]
env = dict(os.environ, MYSQL_PWD=os.environ['TEST_DB_PASSWORD'], TEST_DB_NAME=database)
client = ['mysql', '--protocol=tcp', '-h', env['TEST_DB_HOST'], '-P', env['TEST_DB_PORT'],
          '-u', env['TEST_DB_USER']]
build = ROOT / 'bin/tests/epic-task-catalog'
build.mkdir(parents=True, exist_ok=True)
functions = '\n'.join(extract_function('maintenance_repository.c', signature) for signature in (
    'maintenance_result failure(', 'bool before_deadline(', 'bool parse_positive(',
    'bool query_values(', 'std::string limit_sql(', 'maintenance_result execute_catalog('))
source = build / 'regression.cpp'
source.write_text(HARNESS.replace('// PRODUCTION_FUNCTIONS', functions))
binary = build / 'regression'
flags = shlex.split(subprocess.check_output(['mysql_config', '--cflags', '--libs'], text=True))
subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Isrc', str(source),
                rel('epic_task_catalog.c'), rel('gameplay_read_state.c'), *flags,
                '-o', str(binary)], cwd=ROOT, check=True)
subprocess.run(client + ['-e', f'CREATE DATABASE {database}'], env=env, check=True)
try:
    subprocess.run([str(binary)], env=env, check=True, timeout=30)
finally:
    subprocess.run(client + ['-e', f'DROP DATABASE {database}'], env=env, check=True)
print('catalogue SQL: boot, paged retry, duplicate boundaries, invalid rows and snapshot retention PASS')
