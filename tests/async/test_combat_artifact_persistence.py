#!/usr/bin/env python3
"""Combat/artifact persistence contracts and native artifact-repair lifetime checks."""

from _paths import SRC, extract_function
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
fight_text = (SRC / "fight.c").read_text()
sql_text = (SRC / "sql.c").read_text()
sql_header = (SRC / "sql.h").read_text()
artifact_text = (SRC / "artifact.c").read_text()


def function(text: str, signature: str, next_signature: str) -> str:
    start = text.rfind(signature)
    assert start >= 0, signature
    opening = text.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


add_frags = extract_function("fight.c", "void AddFrags(P_char ch, P_char victim)\n{")
assert "submit_pvp_outcome(ch, victim, true)" in add_frags
for forbidden in ("sql_modify_frags", "redis_invalidate_fraglist", "ADD_MONEY", "epic_frag"):
    assert forbidden not in add_frags
assert "combat_outcome_transaction_submit(payload, combat_outcome_committed, &operation_id)" in fight_text
print("[PASS] combat mutations publish only through the transactional outcome command")

assert "bool sql_get_bind_data(int vnum, int *owner_pid, int *timer);" in sql_header
bind_lookup = function(
    sql_text,
    "bool sql_get_bind_data(int vnum, int *owner_pid, int *timer)\n{",
    "void sql_update_bind_data",
)
query = 'qry("SELECT owner_pid, timer FROM artifact_bind WHERE vnum = %d", vnum)'
query_failure = bind_lookup[bind_lookup.index(f"if (!{query})"):bind_lookup.index(
    "MYSQL_RES *res"
)]
allocation_failure = bind_lookup[bind_lookup.index("if (!res)"):bind_lookup.index(
    "if (mysql_num_rows"
)]
malformed_failure = bind_lookup[bind_lookup.index("if (!row ||"):bind_lookup.index(
    "*owner_pid = parsed_owner_pid;"
)]
checks = {
    "rejects invalid output pointers": "if (!owner_pid || !timer)" in bind_lookup,
    "initializes owner before query": bind_lookup.index("*owner_pid = 0;")
    < bind_lookup.index(query),
    "initializes timer before query": bind_lookup.index("*timer = 0;")
    < bind_lookup.index(query),
    "initializes provided outputs before pointer rejection": bind_lookup.index(
        "*timer = 0;"
    )
    < bind_lookup.index("if (!owner_pid || !timer)"),
    "query failure is explicit": "failed to read from database" in query_failure
    and "return false;" in query_failure,
    "allocation failure is checked": "if (!res)" in bind_lookup
    and "mysql_store_result failed" in allocation_failure
    and "return false;" in allocation_failure,
    "no row is successful defaults": "if (mysql_num_rows(res) < 1)" in bind_lookup
    and "mysql_free_result(res);\n\t\treturn true;" in bind_lookup,
    "row fetch is checked": "if (!row ||" in bind_lookup,
    "both columns are strictly parsed": "sql_parse_bind_int(row[0]" in bind_lookup
    and "sql_parse_bind_int(row[1]" in bind_lookup,
    "malformed row is explicit failure": "malformed database row" in malformed_failure
    and "return false;" in malformed_failure,
    "values publish atomically": bind_lookup.index("int parsed_owner_pid = 0;")
    < bind_lookup.index("*owner_pid = parsed_owner_pid;")
    and bind_lookup.index("int parsed_timer = 0;")
    < bind_lookup.index("*timer = parsed_timer;"),
    "valid row returns success after publication": bind_lookup.index(
        "*timer = parsed_timer;"
    )
    < bind_lookup.rindex("return true;"),
}
for label, passed in checks.items():
    print(f"[{'PASS' if passed else 'FAIL'}] bind lookup: {label}")
assert all(checks.values())

parser = function(sql_text, "static bool sql_parse_bind_int", "bool sql_get_bind_data")
assert "!isdigit((unsigned char)*digit)" in parser
assert "errno == ERANGE" in parser
assert "parsed < INT_MIN || parsed > INT_MAX" in parser
assert "*result = (int)parsed;" in parser
print("[PASS] malformed and out-of-range bind integers cannot publish")

stub_start = sql_text.index("bool sql_get_bind_data(int vnum, int *owner_pid, int *timer)\n{")
stub_end = sql_text.index("bool sql_pwipe", stub_start)
stub = sql_text[stub_start:stub_end]
assert "*owner_pid = 0;" in stub
assert "*timer = 0;" in stub
assert "return false;" in stub
print("[PASS] no-MySQL bind lookup initializes outputs and reports failure")

assert artifact_text.count("sql_get_bind_data(") == 3
assert artifact_text.count("if (!sql_get_bind_data(") == 3
for caller, next_caller in (
    ("void artifact_switch_check", "void artifact_update_sql"),
    ("void artifact_feed_sql", "void poof_artifact"),
    ("void arti_fixit_sql", "void arti_sync_sql"),
):
    body = function(artifact_text, caller, next_caller)
    failure = body.index("if (!sql_get_bind_data(")
    failure_block = body[failure:body.index("}", failure) + 1]
    assert "return;" in failure_block or "continue;" in failure_block
print("[PASS] all artifact bind callers fail closed before ownership decisions")

repair = function(artifact_text, "void arti_fixit_sql(P_char ch)\n{", "// syncs all in-game")
repair_harness = r'''
#include "core/prototypes.h"
#include "core/utility.h"
#include "core/utils.h"
#include "world/db.h"
#include "sql/sql.h"
#include <array>
#include <cassert>
#include <cstdarg>
#include <unordered_set>
#include <vector>

// Run the production SQL command with isolated database/allocation boundaries.
#define ARTIFACT_ON_PC 3
static constexpr time_t now = 1700000000;
static time_t repair_time(time_t *) { return now; }
struct fixture_row { int vnum, location, owner; bool bind_ok, template_ok; };
static std::vector<fixture_row> rows;
static std::vector<std::array<std::string, 2>> sql_rows;
static size_t cursor;
static MYSQL_RES result{};
static bool result_open;
static std::unordered_set<P_obj> live;
static std::vector<int> reads, releases, bind_writes, timer_writes;
static int invalidations;
static std::string output;
MYSQL *DB = nullptr;

static const fixture_row &fixture(int vnum) {
    for (const auto &row : rows) if (row.vnum == vnum) return row;
    std::abort();
}
bool qry_at(persistence_query_site, const char *format, ...) {
    char text[512];
    va_list args; va_start(args, format);
    vsnprintf(text, sizeof(text), format, args); va_end(args);
    if (std::string(text) == "SELECT vnum, location FROM artifacts WHERE locType=3") {
        cursor = 0; result_open = true; return true;
    }
    unsigned long expiry = 0; int vnum = 0;
    assert(sscanf(text, "UPDATE artifacts SET timer = FROM_UNIXTIME(%lu), lastUpdate=SYSDATE() WHERE vnum = %d",
                  &expiry, &vnum) == 2);
    assert(expiry == now + ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY);
    timer_writes.push_back(vnum);
    return true;
}
MYSQL_RES *mysql_store_result(MYSQL *) { assert(result_open); return &result; }
my_ulonglong mysql_num_rows(MYSQL_RES *) { return sql_rows.size(); }
MYSQL_ROW mysql_fetch_row(MYSQL_RES *) {
    assert(result_open);
    if (cursor == sql_rows.size()) return nullptr;
    static char *columns[2];
    columns[0] = sql_rows[cursor][0].data(); columns[1] = sql_rows[cursor++][1].data();
    return columns;
}
void mysql_free_result(MYSQL_RES *) { assert(result_open); result_open = false; }
bool sql_get_bind_data(int vnum, int *owner, int *timer) {
    assert(!result_open);
    const auto &row = fixture(vnum);
    if (!row.bind_ok) return false;
    *owner = row.owner; *timer = 1; return true;
}
void sql_update_bind_data(int vnum, int *owner, int *timer) {
    assert(*owner == fixture(vnum).location && *timer == now);
    bind_writes.push_back(vnum);
}
static void arti_cache_invalidate() { ++invalidations; }
P_obj read_object(int vnum, int type) {
    assert(type == VIRTUAL); reads.push_back(vnum);
    if (!fixture(vnum).template_ok) return nullptr;
    auto *obj = new obj_data{};
    obj->R_num = vnum;
    obj->short_description = strdup("artifact description");
    live.insert(obj); return obj;
}
void extract_obj(P_obj obj, int gone_for_good) {
    assert(obj && !gone_for_good && live.erase(obj) == 1);
    releases.push_back(obj->R_num);
    free(obj->short_description); delete obj;
}
std::string pad_ansi(const char *text, int length, bool trim) {
    assert(length == 35 && trim); return text;
}
char *get_player_name_from_pid(int pid) { assert(pid == 20); static char name[] = "holder"; return name; }
void logit(const char *, const char *, ...) { std::abort(); }
void send_to_char(const char *text, P_char) { output += text; }
void send_to_char_f(P_char, const char *format, ...) {
    char text[512]; va_list args; va_start(args, format);
    vsnprintf(text, sizeof(text), format, args); va_end(args); output += text;
}

#define time repair_time
// INSERT_REPAIR
#undef time

static void run(std::vector<fixture_row> fixtures) {
    assert(live.empty()); rows = std::move(fixtures); sql_rows.clear();
    for (const auto &row : rows) sql_rows.push_back({std::to_string(row.vnum), std::to_string(row.location)});
    reads.clear(); releases.clear(); bind_writes.clear(); timer_writes.clear();
    invalidations = 0; output.clear();
    arti_fixit_sql(nullptr);
    assert(live.empty() && !result_open);
}
int main() {
    // Mismatched owner: retain the description through the report, then release once.
    run({{11, 20, 99, true, true}});
    assert(reads == std::vector<int>{11} && releases == reads);
    assert(bind_writes == reads && timer_writes == reads && invalidations == 1);
    assert(output.find("  1) 'artifact description&n'") != std::string::npos);
    assert(output.find("now owned by 'holder' 20.") != std::string::npos);

    // An already-correct binding still cleans up its display prototype without writing.
    run({{12, 20, 20, true, true}});
    assert(reads == std::vector<int>{12} && releases == reads);
    assert(bind_writes.empty() && timer_writes.empty() && invalidations == 0);
    assert(output == "All artifact bind_data are up to date.\n\r");

    // A missing prototype uses the existing NULL report without extracting nullptr.
    run({{14, 20, 99, true, false}});
    assert(reads == std::vector<int>{14} && releases.empty());
    assert(bind_writes == reads && timer_writes == reads && invalidations == 1);
    assert(output.find("  1) 'NULL&n'") != std::string::npos);
    run({{15, 20, 20, true, false}});
    assert(reads == std::vector<int>{15} && releases.empty());
    assert(bind_writes.empty() && timer_writes.empty() && invalidations == 0);

    // Failed binding lookup must not allocate a prototype or start a repair.
    run({{13, 20, 99, false, true}});
    assert(reads.empty() && releases.empty() && bind_writes.empty() && timer_writes.empty());
    assert(invalidations == 0 && output.find("Skipped artifact 13: bind lookup failed.") != std::string::npos);

    // Row iteration must keep lifetimes separate and count only repaired bindings.
    run({{11, 20, 99, true, true}, {12, 20, 20, true, true},
         {13, 20, 99, false, true}, {14, 20, 99, true, false}, {16, 20, 99, true, true}});
    assert(reads == (std::vector<int>{11, 12, 14, 16}));
    assert(releases == (std::vector<int>{11, 12, 16}));
    assert(bind_writes == (std::vector<int>{11, 14, 16}) && timer_writes == bind_writes);
    assert(invalidations == 3 && output.find("  3) 'artifact description&n'") != std::string::npos);
    puts("[PASS] six native SQL artifact-repair lifetime scenarios (ASan/UBSan)");
}
'''
build_root = root / "bin" / "tests"
build_root.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="artifact-repair-", dir=build_root) as directory:
    work = Path(directory)
    source = work / "repair.cpp"
    source.write_text(repair_harness.replace("// INSERT_REPAIR", repair))
    binary = work / "repair"
    mysql_cflags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
    subprocess.run([
        os.environ.get("CXX", "g++"), "-std=c++20", "-O1", "-g",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-no-pie",
        "-I", str(root / "src"), *mysql_cflags, str(source), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True, env=dict(
        os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
        UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1",
    ))
print("combat and artifact persistence source contracts and native repair checks passed")
