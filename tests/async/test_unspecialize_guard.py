#!/usr/bin/env python3
"""Verify that unspecialize denies invalid state before charging epic points."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, SRC, extract_function
from _source_contract import function_body


source = (SRC / "specializations.c").read_text(encoding="utf-8", errors="replace")
body = function_body(source, r"\bvoid\s+unspecialize\s*\(")
assert body is not None, "unspecialize definition is missing"
guard_at = body.index("if (!IS_SPECIALIZED(ch))")
denial_at = body.index("Water Goddess", guard_at)
return_at = body.index("return;", denial_at)
charge_at = body.index("epic_transaction_submit", guard_at)
assert return_at < charge_at, "non-specialized guard must return before epic charge"


PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "net/comm.h"
#include "world/epic.h"
#include "world/epic_transaction.h"
#include <cassert>
#include <cstdio>

static int message_count = 0;
static int submissions = 0;

void send_to_char(const char *, P_char) { ++message_count; }
void act(const char *, int, P_char, P_obj, void *, int) {}
void unspecialize_committed(P_char, bool, const epic_command_result &, unsigned int,
                            const uint8_t *, size_t) {}
bool epic_transaction_submit(P_char, int64_t, epic_reason_type, int64_t, uint16_t,
                             critical_source_site, critical_deadline_class,
                             epic_completion_fn, const void *, size_t) {
    ++submissions;
    return true;
}
'''

DRIVER = r'''
int main() {
    char_data non_specialized{};
    pc_only_data non_specialized_pc{};
    non_specialized.only.pc = &non_specialized_pc;
    non_specialized_pc.epics = 10;
    non_specialized.player.spec = 0;
    unspecialize(&non_specialized, nullptr);
    assert(submissions == 0);
    assert(non_specialized.player.spec == 0);
    assert(message_count == 1);

    char_data specialized{};
    pc_only_data specialized_pc{};
    specialized.only.pc = &specialized_pc;
    specialized_pc.epics = 10;
    specialized.player.spec = 1;
    unspecialize(&specialized, nullptr);
    assert(submissions == 1);

    std::puts("unspecialize invalid-state charge guard regression passed");
}
'''


with tempfile.TemporaryDirectory(prefix="duris-unspecialize-") as directory:
    directory_path = Path(directory)
    harness = directory_path / "harness.cpp"
    binary = directory_path / "harness"
    harness.write_text(
        PRELUDE + extract_function("specializations.c", "void unspecialize(P_char ch,") + DRIVER,
        encoding="utf-8",
    )
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsanitize=address,undefined",
            "-I" + str(ROOT / "src"),
            str(harness),
            "-o",
            str(binary),
        ],
        check=True,
    )
    subprocess.run([str(binary)], check=True)
