#!/usr/bin/env python3
"""Exercise the NPC rename admission guard before identity and wallet effects."""

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

PRELUDE = r'''
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>

#define TRUE 1
#define FALSE 0
#define CMD_SET_PERIODIC 1
#define CMD_ASK 2
#define MAX_STRING_LENGTH 1024
#define AVATAR 1
#define LOG_PLAYER 2
#define PLAYERLOG 3
#define SEX_MALE 1

struct Character {
    struct { const char *name = "clerk"; const char *short_descr = "clerk"; } player;
    char name[64] = "Oldname";
    int cash = 6000000;
};
using P_char = Character *;
#define IS_ALIVE(ch) ((ch) != nullptr)
#define CAN_SPEAK(ch) true
#define CAN_SEE(npc, ch) true
#define GET_MONEY(ch) ((ch)->cash)
#define GET_NAME(ch) ((ch)->name)
#define GET_SEX(ch) SEX_MALE
#define SUB_MONEY(ch, amount, kind) do { ++debit_calls; (ch)->cash -= (amount); } while (false)

bool active_epoch = false;
int rename_calls = 0, ship_calls = 0, debit_calls = 0, message_calls = 0;
namespace economic_gameplay_authority { bool active() { return active_epoch; } }
char *one_argument(char *input, char *output) {
    while (*input && std::isspace(static_cast<unsigned char>(*input))) ++input;
    while (*input && !std::isspace(static_cast<unsigned char>(*input))) *output++ = *input++;
    *output = '\0';
    return input;
}
int str_cmp(const char *a, const char *b) { return std::strcmp(a, b); }
void send_to_char(const char *, P_char) { ++message_calls; }
void mobsay(P_char, const char *) {}
int get_property(const char *, int fallback) { return fallback; }
const char *coin_stringv(int) { return "coins"; }
bool rename_character(P_char ch, char *, char *new_name) {
    ++rename_calls;
    std::snprintf(ch->name, sizeof(ch->name), "%s", new_name);
    return true;
}
bool rename_ship_owner(char *, char *) { ++ship_calls; return true; }
void wizlog(int, const char *, ...) {}
void logit(int, const char *, ...) {}
void sql_log(P_char, int, const char *, ...) {}
'''

MAIN = r'''
int main() {
    Character clerk, player;
    char active_request[] = "clerk rename Newname";
    active_epoch = true;
    assert(mob_do_rename_hook(&clerk, &player, CMD_ASK, active_request) == TRUE);
    assert(rename_calls == 0 && ship_calls == 0 && debit_calls == 0);
    assert(std::strcmp(player.name, "Oldname") == 0 && player.cash == 6000000);
    assert(message_calls == 1);
    char inactive_request[] = "clerk rename Newname";
    active_epoch = false;
    assert(mob_do_rename_hook(&clerk, &player, CMD_ASK, inactive_request) == TRUE);
    assert(rename_calls == 1 && ship_calls == 1 && debit_calls == 1);
    assert(std::strcmp(player.name, "Newname") == 0 && player.cash == 1000000);
}
'''


def function_body() -> str:
    source = (ROOT / "src/net/modify.c").read_text(encoding="utf-8")
    start = source.index("int mob_do_rename_hook(")
    opening = source.index("{", start)
    depth = 0
    for end in range(opening, len(source)):
        depth += (source[end] == "{") - (source[end] == "}")
        if depth == 0:
            return source[start:end + 1]
    raise AssertionError("unterminated mob_do_rename_hook")


class ActivePaidRenameRefusal(unittest.TestCase):
    def test_active_refusal_executes_before_identity_and_cash_changes(self) -> None:
        compiler = shlex.split(os.environ.get("CXX", "g++"))
        if not compiler or not shutil.which(compiler[0]):
            self.skipTest("C++ compiler unavailable")
        with tempfile.TemporaryDirectory(prefix="active-rename-") as directory:
            program = Path(directory) / "rename.cpp"
            binary = Path(directory) / "rename"
            program.write_text(PRELUDE + function_body() + MAIN, encoding="utf-8")
            subprocess.run([*compiler, "-std=c++20", "-O0", str(program), "-o", str(binary)],
                           check=True, timeout=60)
            subprocess.run([str(binary)], check=True, timeout=10)

    def test_guard_precedes_price_identity_and_debit(self) -> None:
        body = function_body()
        guard = body.index("economic_gameplay_authority::active()")
        refusal = body.index("return TRUE;", guard)
        self.assertLess(body.index("if (!*new_name)"), guard)
        self.assertLess(body.index("if (!CAN_SEE(npc, ch))"), guard)
        for effect in ("get_property(\"mobspecs.rename.price\"", "rename_character(ch, old_name",
                       "rename_ship_owner(old_name", "SUB_MONEY(ch, renamePrice"):
            self.assertLess(refusal, body.index(effect))

    def test_audit_keeps_unrooted_rename_blocked(self) -> None:
        matrix = json.loads((ROOT / "docs/persistence/economy_accounting/writer_coverage_matrix.json")
                            .read_text(encoding="utf-8"))
        route = next(row for row in matrix["routes"] if row["id"] == "service.mob_rename_fee")
        self.assertTrue(route["blocking_policy_after_activation"]["must_block_on_activation"])
        self.assertFalse(route["double_entry_evidence"]["unified_operation_postings_observed"])


if __name__ == "__main__":
    unittest.main()
