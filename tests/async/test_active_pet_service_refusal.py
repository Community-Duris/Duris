#!/usr/bin/env python3
"""Exercise the production patrol and pet service callbacks across activation."""

from __future__ import annotations

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
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#define TRUE 1
#define FALSE 0
#define CMD_SET_PERIODIC 1
#define CMD_LIST 2
#define CMD_BUY 3
#define CMD_RENT 4
#define CMD_OFFER 5
#define CMD_GIVE 6
#define MAX_INPUT_LENGTH 1024
#define MAX_STRING_LENGTH 1024
#define REAL 1
#define VIRTUAL 2
#define TO_ROOM 1
#define TO_CHAR 2
#define PET_NOCASH 1
#define CLASS_WARRIOR 1
#define CLASS_NONE 0
#define STRUNG_KEYS 1
#define STRUNG_DESC1 2
#define STRUNG_DESC2 4
#define STRUNG_DESC3 8
#define LOG_DEBUG 1
#define AVATAR 2
struct Character {
    int in_room = 0, cash = 5000, exp = 100, level = 10, rnum = 1, vnum = 1, id = 7;
    Character *next_in_room = nullptr, *master = nullptr;
    void *carrying = nullptr;
    const char *name = "pet";
    struct { const char *short_descr = "a pet"; int m_class = CLASS_WARRIOR; } player;
};
struct Object {
    int value[4] = {100, 7, 100, 0};
    int str_mask = 0, cost = 0;
    char *name = nullptr, *short_description = nullptr, *description = nullptr;
};
struct Room { Character *people = nullptr; int number = 100; } world[3];
using P_char = Character *;
using P_obj = Object *;
bool active_epoch = false;
int refusals = 0, debits = 0, mobile_reads = 0, publications = 0;
int pet_setups = 0, followers = 0, pet_writes = 0, object_reads = 0;
int object_removals = 0, object_extractions = 0, mount_extractions = 0;
Character stock, mount, created;
Object claim_ticket;
namespace economic_gameplay_authority { bool active() { return active_epoch; } }
#define IS_ALIVE(ch) ((ch) != nullptr)
#define GET_EXP(ch) ((ch)->exp)
#define GET_LEVEL(ch) ((ch)->level)
#define GET_VNUM(ch) ((ch)->vnum)
#define GET_RNUM(ch) ((ch)->rnum)
#define GET_MONEY(ch) ((ch)->cash)
#define GET_NAME(ch) ((ch)->name)
#define GET_MASTER(ch) ((ch)->master)
#define GET_IDNUM(ch) ((ch)->id)
#define IS_FIGHTING(ch) false
#define SUB_MONEY(ch, amount, kind) do { ++debits; (ch)->cash -= (amount); } while (false)
char *one_argument(char *input, char *output) {
    while (*input == ' ') ++input;
    while (*input && *input != ' ') *output++ = *input++;
    *output = '\0';
    return input;
}
void half_chop(char *input, char *first, char *second) {
    one_argument(input, first);
    *second = '\0';
}
P_char get_char_room(const char *, int) { return &stock; }
P_char get_char_room_vis(P_char, const char *) { return &mount; }
P_obj get_obj_in_list_vis(P_char, const char *, void *) { return &claim_ticket; }
int count_patrol(int) { return 0; }
int count_pets(P_char) { return 0; }
int mount_rent_cost(P_char pet) { return GET_LEVEL(pet) * 100; }
const char *coin_stringv(int) { return "coins"; }
std::string pad_ansi(const char *name, int, int) { return name; }
void send_to_char(const char *message, P_char) {
    if (std::strstr(message, "unavailable")) ++refusals;
}
P_char read_mobile(int, int) { ++mobile_reads; return &created; }
void char_to_room(P_char, int, int) { ++publications; }
void do_flee(P_char, int, int) {}
void mobPatrol_SetupNew(P_char) { ++pet_setups; }
void setup_pet(P_char, P_char, int, int) { ++pet_setups; }
void add_follower(P_char, P_char) { ++followers; }
bool isname(const char *, const char *) { return false; }
void act(const char *, int, P_char, P_obj, P_char, int) {}
void wizlog(int, const char *, ...) {}
void logit(int, const char *, ...) {}
int writePet(P_char) { ++pet_writes; return TRUE; }
P_obj read_object(int, int) { ++object_reads; return &claim_ticket; }
char *str_dup(const char *text) { return strdup(text); }
void obj_to_char(P_obj, P_char) { ++publications; }
void extract_char(P_char) { ++mount_extractions; }
void obj_from_char(P_obj) { ++object_removals; }
void extract_obj(P_obj, int) { ++object_extractions; }
'''

MAIN = r'''
int main() {
    Character player;
    player.level = 50;
    mount.master = &player;
    world[1].people = &stock;
    active_epoch = true;
    char listing[] = "";
    assert(patrol_shops(0, &player, CMD_LIST, listing) == TRUE);
    assert(pet_shops(0, &player, CMD_LIST, listing) == TRUE);
    assert(refusals == 0);
    char patrol[] = "pet";
    assert(patrol_shops(0, &player, CMD_BUY, patrol) == TRUE);
    char pet[] = "pet";
    assert(pet_shops(0, &player, CMD_BUY, pet) == TRUE);
    char offer[] = "mount";
    assert(pet_shops(0, &player, CMD_OFFER, offer) == TRUE);
    assert(refusals == 2);
    char rent[] = "mount";
    assert(pet_shops(0, &player, CMD_RENT, rent) == TRUE);
    char give[] = "ticket keeper";
    assert(pet_shops(0, &player, CMD_GIVE, give) == TRUE);
    assert(refusals == 4 && player.cash == 5000);
    assert(debits == 0 && mobile_reads == 0 && publications == 0);
    assert(pet_setups == 0 && followers == 0 && pet_writes == 0);
    assert(object_reads == 0 && object_removals == 0);
    assert(object_extractions == 0 && mount_extractions == 0);

    active_epoch = false;
    char legacy_patrol[] = "pet";
    assert(patrol_shops(0, &player, CMD_BUY, legacy_patrol) == TRUE);
    char legacy_pet[] = "pet";
    assert(pet_shops(0, &player, CMD_BUY, legacy_pet) == TRUE);
    char legacy_rent[] = "mount";
    assert(pet_shops(0, &player, CMD_RENT, legacy_rent) == TRUE);
    char legacy_give[] = "ticket keeper";
    assert(pet_shops(0, &player, CMD_GIVE, legacy_give) == TRUE);
    assert(refusals == 4 && debits == 4 && player.cash == 2150);
    assert(mobile_reads == 2 && pet_setups == 2 && followers == 1);
    assert(pet_writes == 1 && object_reads == 1 && publications == 3);
    assert(object_removals == 1 && object_extractions == 1 && mount_extractions == 1);
}
'''


def function_body(source: str, signature: str) -> str:
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 0
    for end in range(opening, len(source)):
        depth += (source[end] == "{") - (source[end] == "}")
        if depth == 0:
            return source[start:end + 1]
    raise AssertionError(f"unterminated {signature}")


class ActivePetServiceRefusal(unittest.TestCase):
    def test_services_refuse_before_mutation_and_legacy_paths_still_work(self) -> None:
        source = (ROOT / "src/specs/specs.room.c").read_text(encoding="utf-8")
        patrol = function_body(source, "int patrol_shops(")
        pet = function_body(source, "int pet_shops(")
        self.assertLess(patrol.index("economic_gameplay_authority::active()"),
                        patrol.index("read_mobile("))
        for effect in ("read_mobile(", "SUB_MONEY(", "writePet(", "obj_from_char("):
            self.assertLess(pet.index("economic_gameplay_authority::active()"),
                            pet.index(effect))
        compiler = shlex.split(os.environ.get("CXX", "g++"))
        if not compiler or not shutil.which(compiler[0]):
            self.skipTest("C++ compiler unavailable")
        with tempfile.TemporaryDirectory(prefix="active-pet-service-") as directory:
            program = Path(directory) / "pet_service.cpp"
            binary = Path(directory) / "pet_service"
            program.write_text(PRELUDE + patrol + pet + MAIN, encoding="utf-8")
            subprocess.run([*compiler, "-std=c++20", "-O0", str(program), "-o", str(binary)],
                           check=True, timeout=60)
            subprocess.run([str(binary)], check=True, timeout=10)


if __name__ == "__main__":
    unittest.main()
