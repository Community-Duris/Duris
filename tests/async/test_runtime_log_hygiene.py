#!/usr/bin/env python3
"""Exercise command, peer, reset and shutdown diagnostics under ASan/UBSan.

Run the production command logger and shutdown callback. For the large connection
and world-reset functions, execute only their diagnostic branches with controlled
world indices and a failed peer lookup. No server, database or player data is used.
"""
import os
from pathlib import Path
import re
import subprocess
import tempfile

from _paths import ROOT, HARNESS_STUBS, extract_function, source

descriptor = extract_function("comm.c", "int new_descriptor(int s, int conn_type)")
peer_start = descriptor.index("\tif (getpeername(")
peer_failure = descriptor[peer_start:descriptor.index("\n\telse", peer_start)]
reset = extract_function("db.c", "void reset_zone(")
reset_logs = []
for command in "MFR":
    matches = re.findall(r'logit\(LOG_MOB,\s*"' + command + r' cmd not executed.*?;', reset, re.S)
    assert len(matches) == 1, command
    reset_logs.append(f"case '{command}': {matches[0]} break;")

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "world/events.h"
#include "net/comm.h"
#include "sql/sql.h"
#include "cmd/interp.h"
#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <netinet/in.h>

static room_data rooms[3]{};
P_room world = rooms;
extern const int top_of_world = 2;
static index_data mobs[2]{};
P_index mob_index = mobs;
P_desc descriptor_list = nullptr;
static FILE *cmdfile;
uint logcount = 0;
TimedShutdownData shutdownData{};
int shutdownflag, _reboot, _copyover, _autoboot, _pwipe;
char *shutdown_message = nullptr;
static std::vector<std::string> logged, broadcasts;
static int shutdown_records, sql_records;

void logit(const char *, const char *format, ...) {
    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    logged.emplace_back(buffer);
}
void send_to_all(const char *message) { broadcasts.emplace_back(message); }
void sql_log(P_char, const char *, const char *, ...) { ++sql_records; }
void write_shutdown_info(const char *issuer, const char *reason) {
    assert(!strcmp(issuer, shutdownData.IssuedBy));
    assert(!strcmp(reason, shutdownData.Reason));
    ++shutdown_records;
}
void __free(void *value, const char *, int) { free(value); }
char *str_dup(const char *text) { return strdup(text); }
void do_restore(P_char, char *, int) { assert(false); }
int persistence_prepare_pwipe() { assert(false); return false; }
int persistence_quarantine_fallback_events() { assert(false); return false; }
bool sql_pwipe(int) { assert(false); return false; }
bool sql_pwipe_crossed_boundary() { assert(false); return false; }
nevent_schedule_result add_event(event_func, int, P_char, P_char, P_obj, int,
                                const void *, int) { assert(false); return {}; }
static int failed_getpeername(int, sockaddr *, socklen_t *) { errno = ENOTCONN; return -1; }
'''

functions = []
for signature in ("void init_cmdlog(", "void close_cmdlog(", "void cmdlog("):
    functions.append(extract_function("debug.c", signature))
functions.append(extract_function("actwiz.c", "static const char *scheduled_shutdown_type_name("))
if "static void log_shutdown(" in source("actwiz.c").read_text():
    functions.append(extract_function("actwiz.c", "static void log_shutdown("))
functions.append(extract_function("actwiz.c", "void timedShutdown("))

DRIVER = r'''
static std::vector<std::string> command_lines() {
    close_cmdlog();
    std::ifstream stream("logs/log/cmd.debug");
    assert(stream.good());
    std::vector<std::string> lines;
    for (std::string line; std::getline(stream, line);) lines.push_back(line);
    return lines;
}
static char_data actor() {
    char_data ch{};
    ch.player.name = const_cast<char *>("LogQa");
    ch.in_room = 1;
    world[1].number = 1200;
    return ch;
}
static void short_commands() {
    auto ch = actor();
    char north[] = "n", south[] = "s", attack[] = "k target";
    init_cmdlog();
    cmdlog(&ch, north); cmdlog(&ch, south); cmdlog(&ch, attack);
    auto lines = command_lines();
    assert(lines.size() == 3 && logcount == 3);
    assert(lines[0].ends_with(":: [1] LogQa in 1200: n"));
    assert(lines[1].ends_with(":: [2] LogQa in 1200: s"));
    assert(lines[2].ends_with(":: [3] LogQa in 1200: k target"));
}
static void empty_commands() {
    auto ch = actor();
    char empty[1] = {'\0'};
    char stale[] = {'\0', 'x', '\0'};
    char look[] = "look";
    cmdlog(&ch, look); // A closed log still safely ignores commands.
    init_cmdlog();
    cmdlog(&ch, empty); cmdlog(&ch, stale);
    ch.specials.act = ACT_ISNPC;
    cmdlog(&ch, look);
    assert(command_lines().empty() && logcount == 0);
}
static void peer() {
    descriptor_data descriptor{};
    auto *newd = &descriptor;
    sockaddr_in6 sock{};
    socklen_t size = sizeof(sock);
    int desc = -1;
#define getpeername failed_getpeername
    /* PEER_FAILURE */
#undef getpeername
    assert(!strcmp(newd->host, "unknown"));
}
static void reset_message(char command, int room) {
    struct { int arg1, arg2, arg3, arg4; } args{1, 7, room, 25};
#define ZCMD args
    switch (command) {
        /* RESET_LOGS */
        default: assert(false);
    }
#undef ZCMD
}
static void resets() {
    mob_index[1].virtual_number = 8888;
    world[2].number = 599999;
    for (char command : {'M', 'F', 'R'}) {
        for (int room : {2, -1, 3}) {
            logged.clear();
            reset_message(command, room);
            assert(logged.size() == 1);
            const std::string expected = std::string(1, command) +
                " cmd not executed: mob 8888 in room " + (room == 2 ? "599999" : "-1") +
                ", limit 7, chance 25%";
            assert(logged[0] == expected);
        }
    }
}
static void shutdowns() {
    for (const auto &[kind, name] : std::vector<std::pair<int, const char *>>{
        {TimedShutdownData::OK, "Shutdown"}, {TimedShutdownData::REBOOT, "Reboot"},
        {TimedShutdownData::COPYOVER, "Copyover"}, {TimedShutdownData::AUTOREBOOT, "Auto-reboot"},
        {TimedShutdownData::AUTOREBOOT_COPYOVER, "Auto-reboot with copyover"}}) {
        logged.clear(); broadcasts.clear(); shutdown_records = sql_records = 0;
        shutdownflag = _reboot = _copyover = _autoboot = _pwipe = 0;
        shutdownData = {};
        shutdownData.eShutdownType = static_cast<decltype(shutdownData.eShutdownType)>(kind);
        strcpy(shutdownData.IssuedBy, "LogQa");
        strcpy(shutdownData.Reason, "operator's test at 25%");
        timedShutdown(nullptr, nullptr, nullptr, nullptr);
        assert(logged.size() == 1);
        assert(logged[0] == std::string(name) + " by LogQa: operator's test at 25%");
        assert(logged[0].find_first_of("\r\n\033") == std::string::npos);
        assert(sql_records == 0); // The request is already audited at scheduling time.
        assert(broadcasts.size() == 1 && broadcasts[0].find("\r\n") != std::string::npos);
        assert(shutdown_message && broadcasts[0] == shutdown_message);
        assert(shutdownflag == 1 && !_pwipe);
        assert(_reboot == (kind == TimedShutdownData::REBOOT));
        assert(_copyover == (kind == TimedShutdownData::COPYOVER ||
                              kind == TimedShutdownData::AUTOREBOOT_COPYOVER));
        assert(_autoboot == (kind == TimedShutdownData::AUTOREBOOT ||
                              kind == TimedShutdownData::AUTOREBOOT_COPYOVER));
        assert(shutdown_records == (kind == TimedShutdownData::OK ||
                                    kind == TimedShutdownData::REBOOT ||
                                    kind == TimedShutdownData::COPYOVER));
    }
    free(shutdown_message); shutdown_message = nullptr;
}
int main(int argc, char **argv) {
    assert(argc == 2);
    const std::string mode = argv[1];
    if (mode == "short-commands") short_commands();
    else if (mode == "empty-commands") empty_commands();
    else if (mode == "peer") peer();
    else if (mode == "resets") resets();
    else if (mode == "shutdowns") shutdowns();
    else assert(false);
    printf("runtime log hygiene passed: %s\n", argv[1]);
}
'''

driver = DRIVER.replace("/* PEER_FAILURE */", peer_failure).replace("/* RESET_LOGS */", "\n".join(reset_logs))
with tempfile.TemporaryDirectory(prefix="runtime-log-hygiene-") as directory:
    path = Path(directory)
    (path / "logs/log").mkdir(parents=True)
    harness, binary = path / "harness.cpp", path / "harness"
    harness.write_text(PRELUDE + "\n".join(functions) + driver)
    subprocess.run([
        "g++", "-std=c++20", "-g", "-O1", "-fsanitize=address,undefined",
        "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
        "-ffunction-sections", "-fdata-sections", "-I" + str(ROOT / "src"), "-I/usr/include/mysql",
        str(harness), str(HARNESS_STUBS), "-Wl,--gc-sections", "-o", str(binary),
    ], check=True, cwd=ROOT)
    failures = []
    for mode in ("short-commands", "empty-commands", "peer", "resets", "shutdowns"):
        result = subprocess.run([str(binary), mode], cwd=path, env={
            **os.environ, "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
            "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1",
        })
        if result.returncode:
            failures.append(mode)
    assert not failures, failures
