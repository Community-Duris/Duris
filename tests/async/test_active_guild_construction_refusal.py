#!/usr/bin/env python3
"""Exercise active guild construction refusal at the shared command dispatcher."""

from __future__ import annotations

import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
COMMANDS = ("guildhall", "room", "golem", "upgrade", "rename", "overmax")
ROUTES = ("guildhall.create_fee", "guildhall.room_fee", "guildhall.golem_fee",
          "guildhall.upgrade_fee", "guildhall.rename_fee", "guildhall.overmax_fee")

PRELUDE = r'''
#include <cassert>
#include <cctype>
#include <cstring>

#define MAX_STRING_LENGTH 1024
struct Character { bool leader = true; bool trusted = false; };
using P_char = Character *;
#define CAN_CONSTRUCT_CMD(ch) ((ch)->leader)
#define IS_TRUSTED(ch) ((ch)->trusted)
const char CONSTRUCT_SYNTAX[] = "construction help";
bool active_epoch = false;
int calls[6] = {};
int refusals = 0, help_messages = 0;
namespace economic_gameplay_authority { bool active() { return active_epoch; } }
void send_to_char(const char *message, P_char) {
    if (std::strstr(message, "unavailable while active accounting")) ++refusals;
    else ++help_messages;
}
char *one_argument(char *input, char *output) {
    while (*input && std::isspace(static_cast<unsigned char>(*input))) ++input;
    while (*input && !std::isspace(static_cast<unsigned char>(*input))) *output++ = *input++;
    *output = '\0';
    return input;
}
bool is_abbrev(const char *input, const char *command) {
    return std::strncmp(input, command, std::strlen(input)) == 0;
}
void do_construct_guildhall(P_char, char *) { ++calls[0]; }
void do_construct_room(P_char, char *) { ++calls[1]; }
void do_construct_golem(P_char, char *) { ++calls[2]; }
void do_construct_upgrade(P_char, char *) { ++calls[3]; }
void do_construct_rename(P_char, char *) { ++calls[4]; }
void do_construct_overmax(P_char, char *) { ++calls[5]; }
'''

MAIN = r'''
int main() {
    Character leader;
    const char *commands[] = {"guildhall", "room", "golem", "upgrade", "rename", "overmax"};
    active_epoch = true;
    for (const char *command : commands) {
        char input[64];
        std::strcpy(input, command);
        do_construct(&leader, input, 0);
    }
    for (int count : calls) assert(count == 0);
    assert(refusals == 6 && help_messages == 0);
    do_construct(&leader, nullptr, 0);
    char unknown[] = "unknown";
    do_construct(&leader, unknown, 0);
    assert(refusals == 6 && help_messages == 2);
    active_epoch = false;
    for (const char *command : commands) {
        char input[64];
        std::strcpy(input, command);
        do_construct(&leader, input, 0);
    }
    for (int count : calls) assert(count == 1);
}
'''


def function_body() -> str:
    source = (ROOT / "src/guild/guildhall_cmds.c").read_text(encoding="utf-8")
    start = source.index("void do_construct(P_char ch,")
    opening = source.index("{", start)
    depth = 0
    for end in range(opening, len(source)):
        depth += (source[end] == "{") - (source[end] == "}")
        if depth == 0:
            return source[start:end + 1]
    raise AssertionError("unterminated do_construct")


class ActiveGuildConstructionRefusal(unittest.TestCase):
    def test_dispatcher_refuses_all_six_paid_actions_before_calling_them(self) -> None:
        compiler = shlex.split(os.environ.get("CXX", "g++"))
        if not compiler or not shutil.which(compiler[0]):
            self.skipTest("C++ compiler unavailable")
        with tempfile.TemporaryDirectory(prefix="active-guild-construct-") as directory:
            program = Path(directory) / "construct.cpp"
            binary = Path(directory) / "construct"
            program.write_text(PRELUDE + function_body() + MAIN, encoding="utf-8")
            subprocess.run([*compiler, "-std=c++20", "-O0", str(program), "-o", str(binary)],
                           check=True, timeout=60)
            subprocess.run([str(binary)], check=True, timeout=10)

    def test_guard_covers_each_dispatch_before_mutation(self) -> None:
        body = function_body()
        guard = body.index("economic_gameplay_authority::active()")
        refusal = body.index("return;", guard)
        self.assertLess(body.index("if (!arg || !*arg)"), guard)
        for command in COMMANDS:
            self.assertIn(f'is_abbrev(buff, "{command}")', body[guard:refusal])
            self.assertLess(refusal, body.index(f"do_construct_{command}(ch, arg)"))

    def test_six_fee_routes_remain_blocked(self) -> None:
        matrix = json.loads((ROOT / "docs/persistence/economy_accounting/writer_coverage_matrix.json")
                            .read_text(encoding="utf-8"))
        routes = {route["id"]: route for route in matrix["routes"]}
        for route_id in ROUTES:
            with self.subTest(route=route_id):
                self.assertTrue(routes[route_id]["blocking_policy_after_activation"]
                                ["must_block_on_activation"])
                self.assertFalse(routes[route_id]["double_entry_evidence"]
                                 ["unified_operation_postings_observed"])


if __name__ == "__main__":
    unittest.main()
