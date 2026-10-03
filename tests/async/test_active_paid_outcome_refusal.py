#!/usr/bin/env python3
"""Exercise paid outcomes that still lack one durable money and gameplay root."""

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

STORMPORT_PRELUDE = r'''
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#define TRUE 1
#define FALSE 0
#define CMD_SET_PERIODIC 1
#define CMD_PERIODIC 2
#define CMD_ASK 3
#define MAX_STRING_LENGTH 1024
#define SPILL_BLOOD 100
#define NEXUS_STONE_LAST 10
#define STONE_ALIGN_GOOD 1
#define STONE_ALIGN_EVIL -1
#define TO_VICT 1
#define TO_NOTVICT 2
#define AVATAR 1
#define LOG_PLAYER 2
#define PLAYERLOG 3
struct Character { char name[32] = "player"; int cash = 20000000; };
struct Object { int align = 0; };
struct affected_type { int modifier = 1; };
using P_char = Character *;
using P_obj = Object *;
bool active_epoch = false;
int removals = 0, debits = 0, refusals = 0;
affected_type task;
namespace economic_gameplay_authority { bool active() { return active_epoch; } }
char *one_argument(char *input, char *output) {
    while (*input && std::isspace(static_cast<unsigned char>(*input))) ++input;
    while (*input && !std::isspace(static_cast<unsigned char>(*input))) *output++ = *input++;
    *output = '\0';
    return input;
}
int str_cmp(const char *a, const char *b) { return std::strcmp(a, b); }
#define CAN_SPEAK(ch) true
#define CAN_SEE(npc, ch) true
#define GET_MONEY(ch) ((ch)->cash)
#define GET_NAME(ch) ((ch)->name)
#define IS_RACEWAR_GOOD(ch) false
#define IS_RACEWAR_EVIL(ch) false
#define STONE_ALIGN(obj) ((obj)->align)
#define SUB_MONEY(ch, amount, kind) do { ++debits; (ch)->cash -= amount; } while (false)
void mobsay(P_char, const char *) {}
void send_to_char(const char *message, P_char) {
    if (std::strstr(message, "unavailable")) ++refusals;
}
affected_type *get_epic_task(P_char) { return &task; }
int get_property(const char *, int fallback) { return fallback; }
P_obj get_nexus_stone(int) { static Object stone; return &stone; }
void debug(const char *, ...) {}
const char *coin_stringv(int) { return "coins"; }
void act(const char *, int, P_char, void *, P_char, int) {}
void affect_remove(P_char, affected_type *) { ++removals; }
void wizlog(int, const char *, ...) {}
void logit(int, const char *, ...) {}
void sql_log(P_char, int, const char *, ...) {}
'''

STORMPORT_MAIN = r'''
int main() {
    Character priest, player;
    active_epoch = true;
    char task_request[] = "priest task";
    assert(clear_epic_task_spec(&priest, &player, CMD_ASK, task_request) == TRUE);
    assert(refusals == 0);
    char prayer_request[] = "priest prayer";
    assert(clear_epic_task_spec(&priest, &player, CMD_ASK, prayer_request) == TRUE);
    assert(refusals == 1 && removals == 0 && debits == 0);
    assert(player.cash == 20000000);
    active_epoch = false;
    char inactive_request[] = "priest prayer";
    assert(clear_epic_task_spec(&priest, &player, CMD_ASK, inactive_request) == TRUE);
    assert(removals == 1 && debits == 1 && player.cash == 10000000);
}
'''

PRAY_PRELUDE = r'''
#include <cassert>
#include <cstring>
#define TRUE 1
#define FALSE 0
#define CMD_SET_PERIODIC 1
#define CMD_PRAY 2
#define MAX_STRING_LENGTH 1024
#define TO_ROOM 1
#define TO_CHAR 2
struct Character { int in_room = 0; int cash = 500; const char *name = "Test"; };
struct extra_descr_data { const char *keyword = "item_for_Test"; extra_descr_data *next = nullptr; };
struct Object {
    Object *next_content = nullptr;
    extra_descr_data *ex_description = nullptr;
    int cost = 100;
};
struct Room { Object *contents = nullptr; } world[2];
using P_char = Character *;
using P_obj = Object *;
bool active_epoch = false;
int removals = 0, publications = 0, debits = 0, refusals = 0;
namespace economic_gameplay_authority { bool active() { return active_epoch; } }
#define GET_NAME(ch) ((ch)->name)
#define SUB_MONEY(ch, amount, kind) do { ++debits; (ch)->cash -= amount; } while (false)
int str_cmp(const char *a, const char *b) { return std::strcmp(a, b); }
void send_to_char(const char *, P_char) { ++refusals; }
void act(const char *, int, P_char, void *, void *, int) {}
void obj_from_room(P_obj object) { ++removals; world[1].contents = nullptr; }
void obj_to_room(P_obj object, int room) { ++publications; world[room].contents = object; }
'''

PRAY_MAIN = r'''
int main() {
    Character player;
    extra_descr_data description;
    Object item;
    item.ex_description = &description;
    active_epoch = true;
    assert(pray_for_items(0, &player, CMD_PRAY, nullptr) == FALSE);
    assert(refusals == 0);
    world[1].contents = &item;
    assert(pray_for_items(0, &player, CMD_PRAY, nullptr) == TRUE);
    assert(refusals == 1 && removals == 0 && publications == 0 && debits == 0);
    assert(world[1].contents == &item && player.cash == 500);
    active_epoch = false;
    assert(pray_for_items(0, &player, CMD_PRAY, nullptr) == TRUE);
    assert(removals == 1 && publications == 1 && debits == 1);
    assert(world[0].contents == &item && player.cash == 399);
}
'''

HOME_PRELUDE = r'''
#include <cassert>
#include <cstdio>
#include <cstring>
#define TRUE 1
#define FALSE 0
#define MAX_STRING_LENGTH 1024
#define ROOM_SAFE 1
#define ZONE_TOWN 2
#define JUSTICE_EVILHOME 4
#define JUSTICE_GOODHOME 8
#define TO_CHAR 1
#define MINLVLIMMORTAL 1
struct Character {
    int in_room = 0;
    int cash = 60000;
    struct { int hometown = 1; int birthplace = 2; int orig_birthplace = 3; } player;
};
struct Room { int zone = 0; int number = 1000; } world[1];
struct Zone { int flags = ZONE_TOWN; } zone_table[1];
struct Hometown { int flags = 0; } hometowns[1];
using P_char = Character *;
bool active_epoch = false;
int saves = 0, debits = 0, refusals = 0;
namespace economic_gameplay_authority { bool active() { return active_epoch; } }
int get_property(const char *, double fallback) { return static_cast<int>(fallback); }
#define IS_NPC(ch) false
#define IS_ROOM(room, flag) false
#define IS_ILLITHID(ch) false
#define GET_LEVEL(ch) 30
#define IS_SET(bits, flag) (((bits) & (flag)) != 0)
#define IS_RACEWAR_GOOD(ch) true
#define IS_RACEWAR_EVIL(ch) false
#define VNUM2TOWN(vnum) 1
#define GET_MONEY(ch) ((ch)->cash)
#define SUB_MONEY(ch, amount, kind) do { ++debits; (ch)->cash -= amount; } while (false)
void send_to_char(const char *message, P_char) {
    if (std::strstr(message, "unavailable")) ++refusals;
}
void act(const char *, int, P_char, void *, void *, int) {}
void wizlog(int, const char *, ...) {}
bool do_save_silent(P_char, int) { ++saves; return true; }
'''

HOME_MAIN = r'''
int main() {
    Character player;
    active_epoch = true;
    do_home(&player, nullptr, 0);
    assert(refusals == 1 && saves == 0 && debits == 0);
    assert(player.player.hometown == 1 && player.player.birthplace == 2 &&
           player.player.orig_birthplace == 3 && player.cash == 60000);
    active_epoch = false;
    do_home(&player, nullptr, 0);
    assert(saves == 1 && debits == 1 && player.cash == 10000);
    assert(player.player.hometown == 1000 && player.player.birthplace == 1000 &&
           player.player.orig_birthplace == 1000);
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
            return source[start:end + 1]
    raise AssertionError(f"unterminated {signature}")


def execute_harness(prelude: str, body: str, main: str) -> None:
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    if not compiler or not shutil.which(compiler[0]):
        raise unittest.SkipTest("C++ compiler unavailable")
    with tempfile.TemporaryDirectory(prefix="active-paid-outcome-") as directory:
        program = Path(directory) / "outcome.cpp"
        binary = Path(directory) / "outcome"
        program.write_text(prelude + body + main, encoding="utf-8")
        subprocess.run([*compiler, "-std=c++20", "-O0", str(program), "-o", str(binary)],
                       check=True, timeout=60)
        subprocess.run([str(binary)], check=True, timeout=10)


class ActivePaidOutcomeRefusal(unittest.TestCase):
    def test_epic_task_clear_refuses_before_affect_and_fee(self) -> None:
        body = function_body("src/specs/specs.stormport.c", "int clear_epic_task_spec(")
        guard = body.index("economic_gameplay_authority::active()")
        refusal = body.index("return TRUE;", guard)
        self.assertLess(body.index('!str_cmp(askFor, "prayer")'), guard)
        self.assertLess(refusal, body.index("affect_remove(ch, afp)"))
        self.assertLess(refusal, body.index("SUB_MONEY(ch, price, 0)"))
        execute_harness(STORMPORT_PRELUDE, body, STORMPORT_MAIN)

    def test_item_prayer_refuses_matching_stash_before_movement_and_fee(self) -> None:
        body = function_body("src/specs/specs.room.c", "int pray_for_items(")
        guard = body.index("economic_gameplay_authority::active()")
        refusal = body.index("return TRUE;", guard)
        self.assertLess(body.index("str_cmp(Gbuf4, ext->keyword) == 0"), guard)
        for effect in ("obj_from_room(obj)", "obj_to_room(obj, ch->in_room)",
                       "SUB_MONEY(ch, gold, 0)"):
            self.assertLess(refusal, body.index(effect))
        execute_harness(PRAY_PRELUDE, body, PRAY_MAIN)

    def test_home_fee_refuses_before_save_and_debit(self) -> None:
        body = function_body("src/cmd/actnew.c", "void do_home(")
        guard = body.index("economic_gameplay_authority::active()")
        refusal = body.index("return;", guard)
        for effect in ("ch->player.hometown = world[ch->in_room].number",
                       "do_save_silent(ch, 1)", "SUB_MONEY(ch, cost, 0)"):
            self.assertLess(refusal, body.index(effect))
        execute_harness(HOME_PRELUDE, body, HOME_MAIN)

    def test_audit_keeps_all_three_routes_blocked(self) -> None:
        matrix = json.loads((ROOT / "docs/persistence/economy_accounting/writer_coverage_matrix.json")
                            .read_text(encoding="utf-8"))
        routes = {route["id"]: route for route in matrix["routes"]}
        for route_id in ("special.clear_epic_task_fee", "special.pray_for_items_fee",
                         "housing.home_fee"):
            with self.subTest(route=route_id):
                self.assertTrue(routes[route_id]["blocking_policy_after_activation"]
                                ["must_block_on_activation"])
                self.assertFalse(routes[route_id]["double_entry_evidence"]
                                 ["unified_operation_postings_observed"])


if __name__ == "__main__":
    unittest.main()
