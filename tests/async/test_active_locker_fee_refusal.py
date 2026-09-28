#!/usr/bin/env python3
"""Check paid locker admission before room and private-chest effects."""

from __future__ import annotations

import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "src/item/storage_lockers.c").read_text(encoding="utf-8")


def function_body(signature: str) -> str:
    start = SOURCE.index(signature)
    opening = SOURCE.index("{", start)
    depth = 0
    for end in range(opening, len(SOURCE)):
        depth += (SOURCE[end] == "{") - (SOURCE[end] == "}")
        if depth == 0:
            return SOURCE[start:end + 1]
    raise AssertionError(f"unterminated {signature}")


class ActiveLockerFeeRefusal(unittest.TestCase):
    def test_paid_entry_refuses_before_loading_or_moving(self) -> None:
        body = function_body("int storage_locker_room_hook(int room, P_char ch, int cmd, char *arg)\n{")
        guard = body.index("locker_paid_service_refused(ch)")
        self.assertLess(body.index('str_cmp(enterWhat, "locker")'), guard)
        for effect in ("load_locker_char(", "create_new_locker(", "PFileToLocker(",
                       "char_from_room(ch)", "SUB_BALANCE(ch", "SUB_MONEY(ch"):
            with self.subTest(effect=effect):
                self.assertLess(guard, body.index(effect))

    def test_chest_refuses_before_hash_or_sql_and_rechecks_async_callback(self) -> None:
        body = function_body("static int locker_chestcmd(P_char ch, char *arg)\n{")
        creation = body.split('if (is_abbrev(arg1, "create"))', 1)[1].split(
            'if (is_abbrev(arg1, "delete"))', 1)[0]
        first_guard = creation.index("locker_paid_service_refused(ch)")
        callback = creation.index("auto finish =")
        second_guard = creation.index("locker_paid_service_refused(actor)")
        self.assertLess(first_guard, callback)
        self.assertLess(first_guard, creation.index("password_async_start("))
        self.assertLess(callback, second_guard)
        self.assertLess(creation.index("locker_current(actor)"), second_guard)
        self.assertLess(second_guard, creation.index("sql_create_private_chest_hashed("))
        self.assertLess(second_guard, creation.index("SUB_BALANCE(actor"))
        self.assertLess(second_guard, creation.index("SUB_MONEY(actor"))

    def test_production_guard_allows_inactive_and_refuses_active(self) -> None:
        compiler = shlex.split(os.environ.get("CXX", "g++"))
        if not compiler or not shutil.which(compiler[0]):
            self.skipTest("C++ compiler unavailable")
        helper = function_body("static bool locker_paid_service_refused(")
        harness = r'''
#include <cassert>
#include <cstring>
struct Character {};
using P_char = Character *;
bool active_epoch = false;
int refusals = 0;
namespace economic_gameplay_authority { bool active() { return active_epoch; } }
void send_to_char(const char *message, P_char) {
    assert(std::strstr(message, "unavailable while active accounting"));
    ++refusals;
}
''' + helper + r'''
int main() {
    Character player;
    assert(!locker_paid_service_refused(&player));
    assert(refusals == 0);
    active_epoch = true;
    assert(locker_paid_service_refused(&player));
    assert(refusals == 1);
    active_epoch = false;
    assert(!locker_paid_service_refused(&player));
    assert(refusals == 1);
}
'''
        with tempfile.TemporaryDirectory(prefix="active-locker-fee-") as directory:
            program = Path(directory) / "locker_fee.cpp"
            binary = Path(directory) / "locker_fee"
            program.write_text(harness, encoding="utf-8")
            subprocess.run([*compiler, "-std=c++20", "-O0", str(program), "-o", str(binary)],
                           check=True, timeout=60)
            subprocess.run([str(binary)], check=True, timeout=10)


if __name__ == "__main__":
    unittest.main()
