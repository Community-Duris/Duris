#!/usr/bin/env python3
"""Pirate boarders land in the target ship's own rooms.

NPCShipAI::board_target() loaded boarders at `target->bridge + room_no`, which
assumes the target's interior rooms have consecutive vnums.  They do not have to:
set_ship_physical_layout() claims each room first-free from the zone-600 pool,
and NPC ships spawning and despawning fragment that pool.  A boarder could then
be loaded into another ship's room, or into an idle pool room with no exits.

This runs the real board_target() against a target whose rooms are scattered
through the pool, and checks that every boarder is loaded into one of them.
"""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, extract_function

HARNESS = r'''
#include "core/prototypes.h"
#include "core/structs.h"
#include "ships/ship_npc_ai.h"
#include "ships/ship_npc.h"
#include "ships/ships.h"

#include <cassert>
#include <cstdio>
#include <set>
#include <vector>

static std::vector<int> boarded_rooms;
static int stolen = 0, draws = 0;
static char_data grunt;

NPCShipAI::NPCShipAI(P_ship s, P_char ch) : ship(s), debug_char(ch) {}
void NPCShipAI::steal_target_cargo() { ++stolen; }
void act_to_all_in_ship(P_ship, const char *) {}
int number(int low, int high) { return low + draws++ % (high - low + 1); }
P_char load_npc_ship_crew_member(P_ship, int room_no, int, int)
{
	boarded_rooms.push_back(room_no);
	return &grunt;
}

''' + extract_function("ship_npc_ai.c", "void NPCShipAI::board_target()") + r'''

int main()
{
	// A seven-room hull whose rooms were claimed from a fragmented pool.
	const int vnums[] = { 60010, 60400, 60011, 60233, 60012, 60090, 60015 };
	ShipData target = {};
	target.room_count = 7;
	for (int i = 0; i < target.room_count; i++)
		SHIP_ROOM_NUM(&target, i) = vnums[i];
	target.bridge = SHIP_ROOM_NUM(&target, 0);

	NPCShipCrewData crew = {};
	crew.level = 1; // boards three quarters of the rooms
	crew.outer_grunts[0] = 5001;
	crew.outer_grunts[1] = 5002;

	ShipData pirate = {};
	pirate.target = &target;
	NPCShipAI ai(&pirate);
	ai.type = NPC_AI_PIRATE;
	ai.crew_data = &crew;
	ai.did_board = nullptr;

	ai.board_target();

	const std::set<int> rooms(vnums, vnums + 7);
	assert(boarded_rooms.size() == 5);
	assert(boarded_rooms.front() == target.bridge);
	for (int room : boarded_rooms)
		assert(rooms.count(room));
	assert(std::set<int>(boarded_rooms.begin(), boarded_rooms.end()).size() > 2);
	assert(ai.did_board == &target && stolen == 1 && ai.mode == NPC_AI_LEAVING);

	puts("pirate boarders land only in the target ship's rooms");
	return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-ship-board-") as directory:
    path = Path(directory)
    (path / "test.cpp").write_text(HARNESS, encoding="utf-8")
    subprocess.run(
        ["g++", "-std=c++20", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
         "-Isrc", "-Itests/async", "-I/usr/include/mysql",
         str(path / "test.cpp"), "-o", str(path / "test")],
        cwd=ROOT, check=True)
    subprocess.run([str(path / "test")], check=True)
