#include "classes/npc_alchemist.h"
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "economy/economic_gameplay_authority.h"
#include "item/item_uid_allocator.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "persistence/persistence_log.h"
#include "persistence/persistence_mode.h"
#include "world/db.h"
#include "world/vnum.obj.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <unordered_set>
#include <vector>

// Full original server providers, with only the launcher main symbol renamed.
// No private access, test authority, SQL decision, RNG or allocator substitutes.
extern int mini_mode, no_specials, no_random, no_ferries;
extern P_char character_list;
extern P_obj object_list;
extern P_room world;
extern P_index mob_index;
extern int top_of_zone_table;

namespace
{
constexpr int FIRST_ALCHEMIST = 22810, ALCHEMISTS = 64, BAG = 22900;

uint64_t chance_seed(bool selected)
{
	// Choose a fixture input using the real RNG, then reseed immediately before
	// the actual original public chance. This is not a stream-equivalence gate.
	for (uint64_t seed = 1; seed != 10000; ++seed)
	{
		randomize(seed);
		if ((number(1, 100) <= 10) == selected)
			return seed;
	}
	assert(false);
	return 0;
}

size_t actual_vials(P_char actor)
{
	size_t count = 0;
	for (P_obj item = actor->carrying; item; item = item->next_content)
		if (OBJ_VNUM(item) == VOBJ_POISON_VIALS)
			++count;
	return count;
}

void assert_unique_live_uids()
{
	std::unordered_set<uint64_t> seen;
	for (P_obj item = object_list; item; item = item->next)
		assert(item->obj_uid && item->obj_uid != UINT64_MAX &&
		       seen.insert(item->obj_uid).second);
}

P_char actual_actor(int room)
{
	P_char actor = read_mobile(FIRST_ALCHEMIST, VIRTUAL);
	assert(actor && IS_NPC(actor) && actor->only.npc && GET_CLASS(actor, CLASS_ALCHEMIST) &&
	       !GET_MASTER(actor) && !actor->only.npc->summoned_instance &&
	       !actor->only.npc->alchemist_vial_roll_done && actor->in_room == NOWHERE &&
	       actor->runtime_id && find_character_by_runtime_id(actor->runtime_id) == actor);
	GET_BIRTHPLACE(actor) = world[room].number;
	char_to_room(actor, room, -2);
	assert(actor->in_room == room);
	return actor;
}

void retained_retry(P_char actor, uint64_t selected_seed)
{
	assert(actor->only.npc->alchemist_vial_roll_done);
	const auto before_uid = item_uid_allocator_remaining();
	const auto before = actual_vials(actor);
	for (int retry = 0; retry != 3; ++retry)
	{
		randomize(selected_seed);
		npc_alchemist_world_spawn(actor);
		assert(actor->only.npc->alchemist_vial_roll_done &&
		       item_uid_allocator_remaining() == before_uid &&
		       actual_vials(actor) == before);
	}
}

void actual_final_forest(P_char actor)
{
	P_obj vial = actor->carrying;
	assert(vial && OBJ_VNUM(vial) == VOBJ_POISON_VIALS && vial->obj_uid);
	const auto grant_uid = vial->obj_uid;
	P_obj bag = read_object(BAG, VIRTUAL);
	assert(bag && bag->type == ITEM_CONTAINER && bag->obj_uid != grant_uid);
	obj_to_char(bag, actor);
	obj_from_char(vial);
	obj_to_obj(vial, bag);
	assert(vial->loc_p == LOC_INSIDE && vial->loc.inside == bag && bag->contains == vial);

	// This is the actor's complete one-root inventory forest, checked through
	// actual reciprocal links. The public literal capture needs no birth/source
	// reference, admitted command or synthetic identity facts.
	assert(actor->carrying == bag && !bag->next_content && bag->loc_p == LOC_CARRIED &&
	       bag->loc.carrying == actor && !vial->next_content);
	std::vector<player_item_snapshot> observed;
	size_t estimated_bytes = 0;
	assert(player_item_snapshot_tree_capture_literal(actor->carrying, &observed,
							 &estimated_bytes) ==
	       player_snapshot_capture_result::ok);
	assert(estimated_bytes);
	assert(observed.size() == 2 && observed[0].object_uid == bag->obj_uid &&
	       observed[0].parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
	       observed[1].object_uid == grant_uid && observed[1].vnum == VOBJ_POISON_VIALS &&
	       observed[1].parent_index == 0 && !observed[1].equipment_slot);
	std::vector<uint8_t> encoded, round_trip;
	std::vector<player_item_snapshot> decoded;
	assert(player_item_snapshot_list_encode(observed, &encoded) ==
	       player_snapshot_codec_result::ok);
	assert(player_item_snapshot_list_decode(encoded.data(), encoded.size(), &decoded) ==
	       player_snapshot_codec_result::ok);
	assert(player_item_snapshot_list_encode(decoded, &round_trip) ==
		       player_snapshot_codec_result::ok &&
	       encoded == round_trip);
	assert_unique_live_uids();
	std::puts(
		"PASS actual granted UID survives later native nesting and canonical final-forest round trip");
}
}

int main(int argc, char **argv)
{
	assert(argc == 2 && (!std::strcmp(argv[1], "present") || !std::strcmp(argv[1], "missing")));
	const bool present = !std::strcmp(argv[1], "present");
	char error[2048]{};
	assert(persistence_mode_configure(error, sizeof(error)) &&
	       !persistence_mode_requires_mysql() && !economic_gameplay_authority::active());
	assert(item_uid_allocator_reserve(nullptr, ITEM_UID_BOOT_RESERVATION));
	assert(persistence_log_start(LOG_FILE, LOG_WIZ));
	mini_mode = no_specials = no_random = no_ferries = 1;
	nevent_bind_game_thread();
	initialize_properties();
	load_event_names();
	randomize(41);
	boot_db(1);
	assert(nevent_is_game_thread() && !economic_gameplay_authority::active());
	const int room = real_room(22800);
	assert(room > NOWHERE && top_of_zone_table == 0 &&
	       (real_object(VOBJ_POISON_VIALS) >= 0) == present);

	// Actual M branch, original loader/converter, original tail and original
	// public alchemist grant. No private owner is reached in inactive mode.
	// ne_init_events already executes the actual boot reset with force2. A
	// later ordinary reset must respect the original prototype limit1 rather
	// than create a second legitimate instance through an explicitly forced reset.
	const auto before_limited_reset = item_uid_allocator_remaining();
	randomize(83);
	reset_zone(0, 0);
	assert(item_uid_allocator_remaining() == before_limited_reset);
	size_t actual_reset_mobiles = 0, reset_vials = 0;
	for (P_char actor = character_list; actor; actor = actor->next)
	{
		if (!IS_NPC(actor) || GET_RNUM(actor) < 0)
			continue;
		const int vnum = mob_index[GET_RNUM(actor)].virtual_number;
		if (vnum < FIRST_ALCHEMIST || vnum >= FIRST_ALCHEMIST + ALCHEMISTS)
			continue;
		++actual_reset_mobiles;
		assert(GET_CLASS(actor, CLASS_ALCHEMIST) && actor->in_room == room &&
		       GET_BIRTHPLACE(actor) == 22800 && actor->only.npc->alchemist_vial_roll_done);
		reset_vials += actual_vials(actor);
	}
	assert(actual_reset_mobiles == ALCHEMISTS && (present ? reset_vials > 0 : !reset_vials));
	assert_unique_live_uids();
	std::printf(
		"PASS actual reset M loaded %zu real alchemists, vial prototype %s, grants %zu\n",
		actual_reset_mobiles, present ? "present" : "absent", reset_vials);

	const auto selected_seed = chance_seed(true), missed_seed = chance_seed(false);
	P_char missed = actual_actor(room);
	const auto before_miss = item_uid_allocator_remaining();
	randomize(missed_seed);
	npc_alchemist_world_spawn(missed);
	assert(missed->only.npc->alchemist_vial_roll_done && !missed->carrying &&
	       item_uid_allocator_remaining() == before_miss);
	retained_retry(missed, selected_seed);
	std::puts("PASS actual missed original chance retains latch and consumes no UID on retry");

	P_char selected = actual_actor(room);
	const auto before_grant = item_uid_allocator_remaining();
	randomize(selected_seed);
	npc_alchemist_world_spawn(selected);
	assert(selected->only.npc->alchemist_vial_roll_done &&
	       actual_vials(selected) == (present ? 1U : 0U));
	assert(item_uid_allocator_remaining() == before_grant - (present ? 1U : 0U));
	retained_retry(selected, selected_seed);
	if (present)
		actual_final_forest(selected);
	else
		std::puts(
			"PASS actual selected chance with missing prototype returns no grant/UID; retained latch prevents retry grant");
	assert(character_runtime_index_is_consistent());
	assert(!economic_gameplay_authority::active());
	assert(persistence_log_drain(1000));
	std::puts(
		"PASS bounded original-provider inactive reset/factory/grant integration; active owner and cold SQL proof pending");
	return 0;
}
