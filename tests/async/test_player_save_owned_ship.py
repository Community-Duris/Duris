#!/usr/bin/env python3
"""A player save saves the ship the player owns, not the one they stand in.

do_save_silent() also saves the character's ship.  It found that ship with
get_ship_from_char(), which returns whatever ship the character is standing
in.  A checkpoint therefore queued a save for someone else's ship, or an NPC
ship, and not for the owner's own ship while the owner was ashore.  The
fallback branch (a character without a pid, or a terminal save type) wrote
that ship synchronously and reported failure when the write failed.
write_ship() always refuses NPC ships, so standing aboard one failed the save
even though the character had been written.

This runs the real do_save_silent() against a fake ship registry.  Both
branches now use the loaded player ship the character owns.  The fallback
branch still reports a failed write of that ship, as
test_deferred_save_flush.py requires.  get_ship_from_char() is deliberately
not defined here.
"""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, extract_function

ACTOTH = "actoth.c"
FUNCTIONS = "\n\n".join(
    extract_function(ACTOTH, signature)
    for signature in (
        "static P_ship owned_player_ship(P_char ch)",
        "bool do_save_silent(P_char ch, int type)",
    )
)

HARNESS = r'''
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/safe_io.h"
#include "core/utils.h"
#include "player/player_save_pipeline.h"
#include "ships/ships.h"
#include "world/achievements.h"
#include "world/hardcore_config.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <strings.h>
#include <vector>

static room_data rooms[2] = {};
P_room world = rooms;

// --- ships: the character's own, another player's, and an NPC ship ------------
static ShipData own, other, npc;
static std::vector<P_ship> queued, written;
static bool fail_write = false;

P_ship get_ship_from_owner(char *name)
{
	for (P_ship ship : { &own, &other, &npc })
		if (ship->ownername && !strcasecmp(ship->ownername, name))
			return ship;
	return nullptr;
}
void queue_ship_save(P_ship ship, const char *) { queued.push_back(ship); }
int write_ship(P_ship ship)
{
	written.push_back(ship);
	return !IS_NPC_SHIP(ship) && SHIP_LOADED(ship) && !fail_write;
}

// --- the rest of a player save -------------------------------------------------
static int character_writes = 0;
bool player_save_pipeline_save_admitted(int) { return true; }
bool player_save_pipeline_is_nonterminal_type(int type) { return type == 1; }
player_save_pipeline_result player_save_pipeline_request(P_char, player_component_mask_t, int, int)
{
	return player_save_pipeline_result::queued;
}
int writeCharacter(P_char, int, int)
{
	++character_writes;
	return TRUE;
}
void checkHallOfFame(P_char, char *) {}
void checkLeaderBoard(P_char) {}
void update_achievements(P_char, P_char, int, int) {}
const struct hardcore_config *hardcore_config_get(void) { return nullptr; }
void send_to_char(const char *, P_char) {}
void logit(const char *, const char *, ...) {}
int panic_corruption_int(const char *, const char *, ...) { abort(); }
int IS_MORPH(P_char) { return FALSE; }
// Only for a connected descriptor's host file, and these characters have none.
void required_fscanf_impl(FILE *, int, const char *, int, const char *, ...) { abort(); }
void required_fgets_impl(char *, int, FILE *, const char *, int) { abort(); }

''' + FUNCTIONS + r'''

static char_data player;
static pc_only_data pc;
static char player_name[] = "Captain", other_name[] = "Stranger";

static void reset(int pid, bool owns_ship)
{
	own = ShipData{};
	other = ShipData{};
	npc = ShipData{};
	own.ownername = owns_ship ? player_name : nullptr;
	own.flags = LOADED;
	other.ownername = other_name;
	other.flags = LOADED;
	npc.race = NPCSHIP;
	npc.flags = LOADED;
	queued.clear();
	written.clear();
	fail_write = false;
	character_writes = 0;

	player = char_data{};
	pc = pc_only_data{};
	pc.pid = pid;
	player.only.pc = &pc;
	player.player.name = player_name;
	player.player.level = 50;
	player.in_room = 1;
}

int main()
{
	// Checkpoint: the owner's ship is queued, wherever the owner stands.
	reset(7, true);
	assert(do_save_silent(&player, 1));
	assert(queued.size() == 1 && queued[0] == &own && written.empty());

	// Checkpoint without a ship: nothing is queued.
	reset(7, false);
	assert(do_save_silent(&player, 1));
	assert(queued.empty() && written.empty());

	// Fallback save: the owner's ship is written, never the NPC ship.
	reset(0, true);
	assert(do_save_silent(&player, 1));
	assert(character_writes == 1);
	assert(written.size() == 1 && written[0] == &own);

	// Fallback save without a ship: the character alone is written.
	reset(0, false);
	assert(do_save_silent(&player, 1));
	assert(character_writes == 1 && written.empty());

	// A failed write of the owner's ship is still reported.
	reset(0, true);
	fail_write = true;
	assert(!do_save_silent(&player, 1));

	// A ship that never loaded is not written, and does not fail the save.
	reset(0, true);
	own.flags = 0;
	assert(do_save_silent(&player, 1));
	assert(written.empty());

	puts("player saves save the owned ship, not the one the player stands in");
	return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-owned-ship-save-") as directory:
    path = Path(directory)
    (path / "test.cpp").write_text(HARNESS, encoding="utf-8")
    subprocess.run(
        ["g++", "-std=c++20", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
         "-Isrc", "-Itests/async", "-I/usr/include/mysql",
         str(path / "test.cpp"), "-o", str(path / "test")],
        cwd=ROOT, check=True)
    subprocess.run([str(path / "test")], check=True)
