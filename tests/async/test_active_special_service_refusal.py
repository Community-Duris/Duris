#!/usr/bin/env python3
"""Pin paid special-procedure refusals before legacy cash and service effects."""

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

LLYREN_PRELUDE = r'''
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>

#define TRUE 1
#define FALSE 0
#define CMD_LIST 1
#define CMD_BUY 2
#define CMD_SAY 3
#define TO_CHAR 4
#define ARTIFACT_MAJOR 1
#define ARTIFACT_UNIQUE 2
#define ARTIFACT_IOUN 3

struct Character;
struct Object;
using P_char = Character *;
using P_obj = Object *;
struct Character {
    struct { const char *short_descr = "clerk"; } player;
    int cash[4] = {0, 0, 0, 0};
    bool pc = false;
};
struct Object {
    P_obj next = nullptr;
    int R_num = 0;
    const char *name = "";
    const char *short_description = "";
    struct { P_char wearing = nullptr; P_char carrying = nullptr;
             int room = 0; P_obj inside = nullptr; } loc;
    int placement = 0;
    bool artifact = false;
    bool ioun = false;
};
struct { int virtual_number; } obj_index[1] = {};
struct { const char *name; } world[1] = {{"test room"}};
P_obj object_list = nullptr;
#define IS_ARTIFACT(obj) ((obj)->artifact)
#define IS_IOUN(obj) ((obj)->ioun)
#define OBJ_WORN(obj) ((obj)->placement == 1)
#define OBJ_CARRIED(obj) ((obj)->placement == 2)
#define OBJ_ROOM(obj) ((obj)->placement == 3)
#define OBJ_INSIDE(obj) ((obj)->placement == 4)
#define OBJ_SHORT(obj) ((obj)->short_description)
#define IS_PC(ch) ((ch)->pc)
#define IS_PC_CORPSE(obj) false
#define GET_PLATINUM(ch) ((ch)->cash[3])
#define GET_GOLD(ch) ((ch)->cash[2])
#define GET_SILVER(ch) ((ch)->cash[1])
#define GET_COPPER(ch) ((ch)->cash[0])

bool active_epoch = false;
int payment_calls = 0, message_calls = 0;
namespace economic_gameplay_authority { bool active() { return active_epoch; } }
char *skip_spaces(char *text) {
    while (*text && std::isspace(static_cast<unsigned char>(*text))) ++text;
    return text;
}
bool is_abbrev(const char *value, const char *name) {
    return std::strncmp(value, name, std::strlen(value)) == 0;
}
char *writable_arg(const char *text) { return const_cast<char *>(text); }
void do_say(P_char, char *, int) {}
bool transact(P_char, void *, P_char, int) { ++payment_calls; return true; }
void send_to_char(const char *, P_char) { ++message_calls; }
void act(const char *, int, P_char, void *, P_char, int) {}
'''

LLYREN_MAIN = r'''
int main() {
    Character clerk, player;
    player.pc = true;
    clerk.cash[0] = 4;
    clerk.cash[3] = 7;
    char request[] = "unique";
    active_epoch = true;
    assert(llyren(&clerk, &player, CMD_LIST, request) == TRUE);
    assert(payment_calls == 0 && message_calls == 1);
    assert(clerk.cash[0] == 4 && clerk.cash[3] == 7);
    assert(llyren(&clerk, &player, CMD_BUY, request) == FALSE);
    assert(payment_calls == 0);
    active_epoch = false;
    assert(llyren(&clerk, &player, CMD_LIST, request) == TRUE);
    assert(payment_calls == 1);
    assert(clerk.cash[0] == 0 && clerk.cash[3] == 0);
}
'''


def function_body(path: str, signature: str) -> str:
    source = (ROOT / path).read_text(encoding="utf-8")
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 0
    for end in range(opening, len(source)):
        depth += (source[end] == "{") - (source[end] == "}")
        if depth == 0:
            return source[start : end + 1]
    raise AssertionError(f"unterminated function: {signature}")


class ActiveSpecialServiceRefusal(unittest.TestCase):
    def test_artifact_location_active_refusal_executes_without_payment(self) -> None:
        compiler = shlex.split(os.environ.get("CXX", "g++"))
        if not compiler or not shutil.which(compiler[0]):
            self.skipTest("C++ compiler unavailable")
        source = (LLYREN_PRELUDE + function_body("src/specs/specs.clfhaven.c", "int llyren(")
                  + LLYREN_MAIN)
        with tempfile.TemporaryDirectory(prefix="active-llyren-") as directory:
            program = Path(directory) / "llyren.cpp"
            binary = Path(directory) / "llyren"
            program.write_text(source, encoding="utf-8")
            subprocess.run([*compiler, "-std=c++20", "-O0", str(program), "-o", str(binary)],
                           check=True, timeout=60)
            subprocess.run([str(binary)], check=True, timeout=10)

    def test_artifact_location_purchase_refuses_before_payment_or_npc_cash_clear(self) -> None:
        body = function_body("src/specs/specs.clfhaven.c", "int llyren(")
        command = body.index("if (cmd != CMD_LIST)")
        guard = body.index("economic_gameplay_authority::active()")
        refusal = body.index("return TRUE;", guard)
        self.assertLess(command, guard)
        self.assertLess(refusal, body.index("transact(pl, NULL, ch, cost)"))
        self.assertLess(refusal, body.index("GET_PLATINUM(ch) = 0"))
        self.assertLess(refusal, body.index("for (t_obj = object_list"))

    def test_cleric_keeps_free_listings_but_refuses_purchases_before_spells(self) -> None:
        body = function_body("src/specs/specs.mobile.c", "int rentacleric(")
        buy = body.index("if (cmd == CMD_BUY)")
        has_spell = body.index("if (*buf)", buy)
        guard = body.index("economic_gameplay_authority::active()", has_spell)
        refusal = body.index("return TRUE;", guard)
        self.assertLess(buy, has_spell)
        self.assertLess(has_spell, guard)
        for mutation in ("transact(vict, NULL, ch, cost)", "GET_PLATINUM(ch) =",
                         "StopCasting(ch)", "MobCastSpell(ch, vict"):
            self.assertLess(refusal, body.index(mutation))
        self.assertIn("else if (cmd == CMD_LIST)", body[refusal:])
        self.assertIn("Here is a listing of the prices", body)

    def test_witch_keeps_free_listing_but_refuses_buy_before_affect(self) -> None:
        body = function_body("src/specs/specs.heavens.c", "int witch_doctor(")
        listing = body.index("if (cmd == CMD_LIST)")
        buy = body.index("if (cmd == CMD_BUY)")
        guard = body.index("economic_gameplay_authority::active()", buy)
        refusal = body.index("return TRUE;", guard)
        self.assertLess(listing, buy)
        self.assertLess(buy, guard)
        for mutation in ("memset(&af", "transact(customer, NULL, witch",
                         "affect_to_char(customer, &af)", "GET_PLATINUM(witch) = 0"):
            self.assertLess(refusal, body.index(mutation))

    def test_audit_keeps_paid_routes_blocked(self) -> None:
        matrix = json.loads((ROOT / "docs/persistence/economy_accounting/writer_coverage_matrix.json")
                            .read_text(encoding="utf-8"))
        routes = {route["id"]: route for route in matrix["routes"]}
        inventory = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json")
                               .read_text(encoding="utf-8"))
        writers = {row["id"]: row for row in inventory["writers"]}
        for route_id in ("special.llyren", "special.rentacleric", "special.witch_doctor"):
            with self.subTest(route=route_id):
                self.assertTrue(routes[route_id]["blocking_policy_after_activation"]
                                ["must_block_on_activation"])
                self.assertFalse(routes[route_id]["double_entry_evidence"]
                                 ["unified_operation_postings_observed"])
                self.assertIn("active", writers[route_id]["current_support"].lower())


if __name__ == "__main__":
    unittest.main()
