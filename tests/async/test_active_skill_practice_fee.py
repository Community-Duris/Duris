#!/usr/bin/env python3
"""Check priced skill lessons cannot follow a refused wallet debit."""

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
SOURCE = (ROOT / "src/guild/guild.c").read_text(encoding="utf-8")


def function_body(signature: str) -> str:
    start = SOURCE.index(signature)
    opening = SOURCE.index("{", start)
    depth = 0
    for end in range(opening, len(SOURCE)):
        depth += (SOURCE[end] == "{") - (SOURCE[end] == "}")
        if depth == 0:
            return SOURCE[start:end + 1]
    raise AssertionError(f"unterminated {signature}")


class ActiveSkillPracticeFee(unittest.TestCase):
    def test_paid_lesson_refuses_before_learning_and_free_paths_remain(self) -> None:
        body = function_body("void do_practice(P_char ch, char *arg, int cmd)\n{")
        lesson = body.index("practice_fee_paid(ch, cost)")
        self.assertLess(body.index("if (!*arg && teacher)"), lesson)
        self.assertLess(body.index('if (!str_cmp(arg, "all"))'), lesson)
        self.assertLess(body.index("if (!meming_cl || !IS_SPELL(skl))"), lesson)
        self.assertLess(lesson, body.index("ch->only.pc->skills[i].learned += 1"))
        self.assertLess(lesson, body.index('"You practice'))
        self.assertLess(lesson, body.index("do_teach(teacher"))

    def test_production_fee_guard_requires_successful_debit(self) -> None:
        compiler = shlex.split(os.environ.get("CXX", "g++"))
        if not compiler or not shutil.which(compiler[0]):
            self.skipTest("C++ compiler unavailable")
        helper = function_body("static bool practice_fee_paid(")
        harness = r'''
#include <cassert>
#include <cstring>
struct Character { int cash = 100; };
using P_char = Character *;
bool active_epoch = false, fail_debit = false;
int debits = 0, refusals = 0;
namespace economic_gameplay_authority { bool active() { return active_epoch; } }
void send_to_char(const char *message, P_char) {
    assert(std::strstr(message, "unavailable while active accounting"));
    ++refusals;
}
int debit(P_char ch, int amount) {
    ++debits;
    if (fail_debit) return -1;
    ch->cash -= amount;
    return 0;
}
#define SUB_MONEY(ch, amount, kind) debit(ch, amount)
''' + helper + r'''
int main() {
    Character student;
    active_epoch = true;
    assert(!practice_fee_paid(&student, 10));
    assert(refusals == 1 && debits == 0 && student.cash == 100);
    assert(practice_fee_paid(&student, 0));
    assert(refusals == 1 && debits == 0 && student.cash == 100);
    assert(!practice_fee_paid(&student, -1));
    active_epoch = false;
    fail_debit = true;
    assert(!practice_fee_paid(&student, 10));
    assert(debits == 1 && student.cash == 100);
    fail_debit = false;
    assert(practice_fee_paid(&student, 10));
    assert(debits == 2 && student.cash == 90);
}
'''
        with tempfile.TemporaryDirectory(prefix="active-practice-fee-") as directory:
            program = Path(directory) / "practice.cpp"
            binary = Path(directory) / "practice"
            program.write_text(harness, encoding="utf-8")
            subprocess.run([*compiler, "-std=c++20", "-O0", str(program), "-o", str(binary)],
                           check=True, timeout=60)
            subprocess.run([str(binary)], check=True, timeout=10)

    def test_writer_policy_requires_active_refusal(self) -> None:
        matrix = json.loads((ROOT / "docs/persistence/economy_accounting/writer_coverage_matrix.json")
                            .read_text(encoding="utf-8"))
        route = next(route for route in matrix["routes"] if route["id"] == "guild.practice_fee")
        self.assertTrue(route["blocking_policy_after_activation"]["must_block_on_activation"])
        self.assertFalse(route["double_entry_evidence"]["unified_operation_postings_observed"])


if __name__ == "__main__":
    unittest.main()
