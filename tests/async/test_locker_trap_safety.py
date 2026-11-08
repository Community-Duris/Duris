#!/usr/bin/env python3
"""Exercise trap suppression within player lockers."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, SRC, extract_function, rel


trap_c = (SRC / "combat" / "trap.c").read_text(encoding="utf-8")
assert "is_in_locker" in trap_c
assert "is_locker_room" in trap_c

checkmovetrap = extract_function("combat/trap.c", "bool checkmovetrap(P_char ch, int dir)")
checkgetput = extract_function("combat/trap.c", "bool checkgetput(P_char ch, P_obj obj)")
checkopen = extract_function("combat/trap.c", "bool checkopen(P_char ch, P_obj obj)")
trapdamage = extract_function("combat/trap.c", "void trapdamage(P_char ch, P_obj obj)")

harness = r'''
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/comm.h"
#include "combat/damage.h"
#include "magic/spells.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

room_data rooms[3] = {};
P_room world = rooms;
const int top_of_world = 2;

static int damage_applied = 0;
static int sleep_applied = 0;

affected_type *affect_to_char(P_char, struct affected_type *af)
{
    if (af && af->bitvector == AFF_SLEEP)
        sleep_applied++;
    return af;
}

bool damage(P_char, P_char, double, int)
{
    damage_applied++;
    return true;
}

int spell_damage(P_char, P_char, double, int, uint, struct damage_messages *, int *)
{
    damage_applied++;
    return 10;
}

void act(const char *, int, P_char, void *, void *, int)
{
}

void act(const char *, int, P_char, P_obj, void *, int)
{
}

void send_to_char(const char *, P_char)
{
}

void send_to_room(const char *, int)
{
}

void spell_teleport(int, P_char, char *, int, P_char, P_obj)
{
}

P_char get_linked_char(P_char, unsigned short)
{
    return NULL;
}

int dice(int, int)
{
    return 10;
}

int number(int a, int)
{
    return a;
}

void stop_fighting(P_char)
{
}

#define TRAP_DAM_SLEEP 0
#define TRAP_DAM_TELEPORT 1
#define TRAP_DAM_FIRE 2
#define TRAP_DAM_COLD 3
#define TRAP_DAM_ACID 4
#define TRAP_DAM_ENERGY 5
#define TRAP_DAM_BLUNT 6
#define TRAP_DAM_PIERCE 7
#define TRAP_DAM_SLASH 8

#define TRAP_EFF_MOVE BIT_1
#define TRAP_EFF_OBJECT BIT_2
#define TRAP_EFF_ROOM BIT_3
#define TRAP_EFF_NORTH BIT_4
#define TRAP_EFF_EAST BIT_5
#define TRAP_EFF_SOUTH BIT_6
#define TRAP_EFF_WEST BIT_7
#define TRAP_EFF_UP BIT_8
#define TRAP_EFF_DOWN BIT_9
#define TRAP_EFF_OPEN BIT_10

static bool is_locker_room(int room)
{
	return room != NOWHERE && room >= 0 && room <= top_of_world && IS_ROOM(room, ROOM_LOCKER);
}

static bool is_in_locker(P_char ch, P_obj obj)
{
	if (ch && is_locker_room(ch->in_room))
		return true;
	for (P_obj cur = obj; cur; )
	{
		if (OBJ_ROOM(cur))
			return is_locker_room(cur->loc.room);
		if (cur->loc_p & (LOC_CARRIED | LOC_WORN))
		{
			P_char carrier = (cur->loc_p & LOC_CARRIED) ? cur->loc.carrying : cur->loc.wearing;
			return carrier && is_locker_room(carrier->in_room);
		}
		if (cur->loc_p & LOC_INSIDE)
			cur = cur->loc.inside;
		else
			break;
	}
	return false;
}
'''

harness += checkmovetrap + "\n" + checkgetput + "\n" + checkopen + "\n" + trapdamage + "\n"

harness += r'''
int main()
{
    // Room 0: ordinary room
    // Room 1: ROOM_LOCKER
    rooms[0].room_flags = 0;
    rooms[1].room_flags = ROOM_LOCKER;

    char_data actor = {};
    pc_only_data pc = {};
    actor.only.pc = &pc;
    actor.specials.act = 0;
    actor.in_room = 1; // in locker!

    obj_data trap_item = {};
    trap_item.trap_charge = 5;
    trap_item.trap_eff = TRAP_EFF_OBJECT | TRAP_EFF_OPEN;
    trap_item.trap_dam = TRAP_DAM_SLEEP;
    trap_item.loc_p = LOC_CARRIED;
    trap_item.loc.carrying = &actor;

    // In a locker, checkgetput and checkopen must be suppressed!
    assert(!checkgetput(&actor, &trap_item));
    assert(trap_item.trap_charge == 5);
    assert(sleep_applied == 0);

    assert(!checkopen(&actor, &trap_item));
    assert(trap_item.trap_charge == 5);
    assert(sleep_applied == 0);

    trapdamage(&actor, &trap_item);
    assert(trap_item.trap_charge == 5);
    assert(sleep_applied == 0);

    // Movement trap in locker
    trap_item.trap_eff = TRAP_EFF_MOVE | TRAP_EFF_NORTH;
    trap_item.loc_p = LOC_ROOM;
    trap_item.loc.room = 1;
    rooms[1].contents = &trap_item;
    assert(!checkmovetrap(&actor, 0));
    assert(trap_item.trap_charge == 5);

    // Now test outside locker (Room 0)
    actor.in_room = 0;
    trap_item.loc_p = LOC_CARRIED;
    trap_item.loc.carrying = &actor;
    trap_item.trap_eff = TRAP_EFF_OBJECT;
    assert(checkgetput(&actor, &trap_item));
    assert(trap_item.trap_charge == 4);
    assert(sleep_applied == 1);

    puts("locker trap safety regressions passed");
    return 0;
}
'''

build = ROOT / "bin" / "tests"
build.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="locker-trap-", dir=build) as directory:
    test_source = Path(directory) / "test.cpp"
    binary = Path(directory) / "test"
    test_source.write_text(harness, encoding="utf-8")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-I" + str(SRC),
            "-I/usr/include/mysql",
            str(test_source),
            "-o",
            str(binary),
        ],
        check=True,
    )
    subprocess.run([str(binary)], check=True)
