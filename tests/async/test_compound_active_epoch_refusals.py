#!/usr/bin/env python3
"""Keep unported compound commands ahead of their first native mutation."""

import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


def function(source: str, declaration: str) -> str:
    start = source.index(declaration)
    return source[start:source.index("\n}", start) + 2]


class CompoundActiveEpochRefusals(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.shop = (ROOT / "src/economy/shop.c").read_text()
        cls.crafting = (ROOT / "src/economy/crafting.c").read_text()
        cls.tradeskill = (ROOT / "src/economy/tradeskill.c").read_text()

    def test_shop_commands_refuse_before_secondary_procedures(self):
        route = function(self.shop, "int shop_keeper(P_char keeper, P_char ch, int cmd, char *arg)")
        guard = route.index("refuse_unported_shop_mutation(ch)")
        self.assertLess(guard, route.index("smith(keeper, ch, cmd, arg)"))
        self.assertLess(guard, route.index("SHOP_FUNC(shop_nr)"))
        for command in ("CMD_BUY", "CMD_SELL", "CMD_PERUSE", "CMD_REPAIR", "CMD_FORGE"):
            self.assertIn(command, route[:guard])

    def test_shop_guard_uses_the_runtime_epoch(self):
        guard = function(self.shop, "static bool refuse_unported_shop_mutation(P_char ch)")
        source = r'''
#include <cassert>
#include <string>
struct Character {};
using P_char = Character *;
struct economic_gameplay_authority {
    static bool enabled;
    static bool active() { return enabled; }
};
bool economic_gameplay_authority::enabled = false;
std::string message;
void send_to_char(const char *text, P_char) { message = text; }
''' + guard + r'''
int main() {
    Character player;
    assert(!refuse_unported_shop_mutation(&player) && message.empty());
    economic_gameplay_authority::enabled = true;
    assert(refuse_unported_shop_mutation(&player));
    assert(message.find("unavailable") != std::string::npos);
}
'''
        build = ROOT / "bin/tests"
        build.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="shop-active-gate-", dir=build) as temporary:
            cpp = Path(temporary) / "gate.cpp"
            binary = Path(temporary) / "gate"
            cpp.write_text(source)
            subprocess.run([os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                            str(cpp), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)

    def test_direct_shop_routes_refuse_before_mutation(self):
        for declaration, actor in (
            ("void shopping_buy(char *arg, P_char ch, P_char keeper, int shop_nr)", "ch"),
            ("void shopping_sell(char *arg, P_char ch, P_char keeper, int shop_nr)", "ch"),
            ("void shopping_peruse(char *arg, P_char ch, P_char keeper, int shop_nr)", "ch"),
            ("void shopping_repair(char *arg, P_char ch, P_char keeper, int shop_nr)", "ch"),
            ("bool transact(P_char from, P_obj merchandise, P_char to, int value)", "from"),
        ):
            with self.subTest(declaration=declaration):
                route = function(self.shop, declaration)
                self.assertRegex(route, re.escape(declaration) +
                                 r"\s*\{\s*if \(refuse_unported_shop_mutation\(" +
                                 actor + r"\)\)\s*return")

    def test_crafting_and_refining_refuse_before_inputs(self):
        craft = function(self.crafting,
                         "void crafting_handle_command(P_char ch, enum crafting_mode mode, char *argument)")
        self.assertLess(craft.index("economic_gameplay_authority::active()"),
                        craft.index("crafting_mode_enabled(mode)"))
        smith = function(self.tradeskill, "int smith(P_char ch, P_char pl, int cmd, char *arg)")
        self.assertLess(smith.index("economic_gameplay_authority::active()"),
                        smith.index("obj_from_char(tobj)"))
        refine = function(self.tradeskill, "void do_refine(P_char ch, char *arg, int /*cmd*/)")
        self.assertLess(refine.index("economic_gameplay_authority::active()"),
                        refine.index("argument_interpreter(arg, gbuf1, gbuf3)"))


if __name__ == "__main__":
    unittest.main()
