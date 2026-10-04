#!/usr/bin/env python3
"""Execute production shutdown scheduling branches at an expired deadline."""

from _paths import extract_function
from pathlib import Path
import os
import shlex
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
function = extract_function("actwiz.c", "void timedShutdown(")
begin = function.index("time_t secs = shutdownData.reboot_time - time(0);")
end = function.index("// if the next warning timer isn't set", begin)
branches = function[begin:end]

HARNESS = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "cmd/interp.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

TimedShutdownData shutdownData;
static time_t clock_now = 1000;
static P_char issuer = nullptr;
static int scheduled_delay = 0;
static int restores = 0;
static time_t fixture_time(time_t *) { return clock_now; }

void do_restore(P_char ch, char *, int command) {
    assert(ch == issuer && GET_LEVEL(ch) == MAXLVL && command == CMD_RESTORE);
    ++restores;
}

nevent_schedule_result add_event(event_func, int delay, P_char, P_char, P_obj,
                                int, const void *, int) {
    scheduled_delay = delay;
    return {delay > 0 ? nevent_schedule_status::scheduled : nevent_schedule_status::negative_delay, {}};
}

#define time fixture_time
void timedShutdown(P_char, P_char, P_obj, void *) {
    @BRANCHES@
}
#undef time

int main() {
    char_data character = {};
    character.player.level = 12;
    for (bool online : {false, true}) {
        issuer = online ? &character : nullptr;
        for (int remaining : {-3, -1, 0, 1}) {
            shutdownData.reboot_time = clock_now + remaining;
            scheduled_delay = 0;
            restores = 0;
            timedShutdown(nullptr, nullptr, nullptr, nullptr);
            if (scheduled_delay <= 0 || scheduled_delay > WAIT_SEC) {
                std::fprintf(stderr, "FAIL: shutdown callback lost at deadline=%d online=%d delay=%d\n",
                             remaining, online, scheduled_delay);
                return 1;
            }
            assert(shutdownData.reboot_time == 0);
            assert(restores == (online ? 1 : 0) && character.player.level == 12);
        }
    }
    std::puts("PASS: production shutdown branches retain a positive follow-up at eight clock/session boundaries");
}
'''

(ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="shutdown-followup-", dir=ROOT / "bin/tests") as temporary:
    source = Path(temporary) / "probe.cpp"
    binary = Path(temporary) / "probe"
    source.write_text(HARNESS.replace("@BRANCHES@", branches))
    subprocess.run(shlex.split(os.environ.get("CXX", "g++")) + [
        "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-D__NO_MYSQL__",
        "-Isrc", "-Isrc/no_mysql", str(source), "-o", str(binary),
    ], cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=10)
