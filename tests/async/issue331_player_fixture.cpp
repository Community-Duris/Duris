#include "player/player_snapshot_codec.h"
#include "core/files.h"
#include "core/structs.h"
#include "classes/necromancy.h"
#include "world/vnum.obj.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace
{
std::string hex(const std::vector<uint8_t> &bytes)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string out;
	out.reserve(bytes.size() * 2);
	for (uint8_t byte : bytes)
	{
		out.push_back(digits[byte >> 4]);
		out.push_back(digits[byte & 0x0f]);
	}
	return out;
}

critical_operation_id operation()
{
	critical_operation_id id = {};
	for (size_t index = 0; index < id.bytes.size(); ++index)
		id.bytes[index] = static_cast<uint8_t>(0xa0 + index);
	return id;
}

player_item_snapshot item(uint64_t uid, int32_t vnum, int32_t parent, int8_t type,
			  uint32_t wear_flags, uint32_t extra_flags, const char *name,
			  const char *short_description)
{
	player_item_snapshot value = {};
	value.parent_index = parent;
	value.equipment_slot = -1;
	value.object_uid = uid;
	value.generated_key = static_cast<int64_t>(uid + 9000);
	value.vnum = vnum;
	value.type = type;
	value.string_mask = 15;
	value.name = name;
	value.short_description = short_description;
	value.description =
		std::string(short_description) + " rests in the recovered custody tree.";
	value.action_description = "Recovered by the issue331 player journey.";
	value.values = { 0, 0, 0, 0, 0, 0, 0, 0 };
	value.timers = { 0, 0, 0, 0, 0, 0 };
	value.wear_flags = wear_flags;
	value.extra_flags = extra_flags;
	value.anti_flags = 0;
	value.anti2_flags = 0;
	value.extra2_flags = 0;
	value.weight = 1;
	value.material = 7;
	value.cost = 25;
	value.condition = 100;
	value.craftsmanship = 0;
	value.bitvectors = { 0, 0, 0, 0, 0 };
	value.affects = { { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } } };
	value.dynamic_affects.push_back({ 1, 2, 3 });
	value.extra_descriptions.push_back({ "recovered", "issue331 custody payload", false, {} });
	return value;
}

player_death_custody_snapshot custody(uint64_t uid, uint64_t root, uint64_t parent, int32_t vnum,
				      int pid, uint64_t owner_revision)
{
	player_death_custody_snapshot row = {};
	row.item = { uid, root, parent, 10, vnum, item_custody_state::active };
	row.owner = { item_owner_type::player, pid, 0 };
	row.owner_revision = owner_revision;
	return row;
}

player_snapshot death_snapshot(int pid, uint64_t owner_revision)
{
	player_snapshot snapshot = {};
	snapshot.schema_version = PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
	snapshot.pid = pid;
	snapshot.revision = 77;
	snapshot.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
	snapshot.save_intent = RENT_DEATH;
	snapshot.encoded_size_bound = PLAYER_SNAPSHOT_MAX_BYTES;
	snapshot.death.emplace();
	snapshot.death->operation_id = operation();
	snapshot.death->corpse_room_vnum = 22800;
	snapshot.death->wallet_revision = 9;
	snapshot.death->wallet_before = { 1, 2, 3, 4 };
	snapshot.death->wallet_pile_uid = 51006;

	player_item_snapshot corpse = {};
	corpse.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	corpse.object_uid = 50999;
	corpse.vnum = VOBJ_CORPSE;
	corpse.type = ITEM_CORPSE;
	corpse.string_mask = 1;
	corpse.name = "corpse";
	corpse.values[CORPSE_PID] = pid;
	corpse.values[CORPSE_SAVEID] = 77;
	corpse.values[CORPSE_FLAGS] = PC_CORPSE;
	snapshot.death->corpse.push_back(corpse);

	// The order is intentional: the bag precedes its nested banana so the
	// materializer can reconstruct the exact parent relationship.
	player_item_snapshot bag = item(51000, 391, 0, ITEM_CONTAINER, ITEM_TAKE, 0,
					"qabag recovered bag leather", "a recovered leather bag");
	bag.values = { 50, 1, 0, 100, 0, 0, 0, 0 };
	bag.material = 13;
	snapshot.death->corpse.push_back(bag);

	player_item_snapshot banana = item(51001, 15, 1, ITEM_FOOD, ITEM_TAKE, 0,
					   "qabanana recovered banana", "a recovered banana");
	banana.values = { 10, 1, 1, 0, 0, 0, 0, 0 };
	banana.material = 48;
	snapshot.death->corpse.push_back(banana);

	player_item_snapshot mace = item(51002, 677, 0, ITEM_WEAPON, ITEM_TAKE | ITEM_WIELD, 0,
					 "qamace recovered mace wooden", "a recovered wooden mace");
	mace.values = { 6, 1, 6, 7, 0, 0, 0, 0 };
	mace.material = 6;
	snapshot.death->corpse.push_back(mace);

	player_item_snapshot gloves = item(51003, 67259, 0, ITEM_ARMOR, ITEM_TAKE | ITEM_WEAR_HANDS,
					   ITEM_ARTIFACT, "qagloves unique recovered gloves",
					   "a unique pair of recovered gloves");
	gloves.values = { 3, 2, 10, 0, 0, 0, 0, 0 };
	gloves.material = 2;
	gloves.cost = 1000;
	gloves.condition = 100;
	snapshot.death->corpse.push_back(gloves);

	player_item_snapshot coins = item(51006, VOBJ_COINS, 0, ITEM_MONEY, ITEM_TAKE, 0, "coins",
					  "a pile of recovered coins");
	coins.values = { 1, 2, 3, 4, 0, 0, 0, 0 };
	snapshot.death->corpse.push_back(coins);

	// This UID remains in custody but intentionally has no serialized payload.
	snapshot.death->custody.push_back(custody(51000, 51000, 0, 391, pid, owner_revision));
	snapshot.death->custody.push_back(custody(51001, 51000, 51000, 15, pid, owner_revision));
	snapshot.death->custody.push_back(custody(51002, 51002, 0, 677, pid, owner_revision));
	snapshot.death->custody.push_back(custody(51003, 51003, 0, 67259, pid, owner_revision));
	snapshot.death->custody.push_back(
		custody(51006, 51006, 0, VOBJ_COINS, pid, owner_revision));
	snapshot.death->custody.push_back(custody(51005, 51005, 0, 102, pid, owner_revision));
	return snapshot;
}
}

int main(int argc, char **argv)
{
	if (argc != 3)
		return 2;
	const int pid = std::stoi(argv[1]);
	std::vector<uint8_t> encoded;
	const auto snapshot = death_snapshot(pid, std::stoull(argv[2]));
	const auto result = player_snapshot_encode(snapshot, &encoded);
	if (result != player_snapshot_codec_result::ok)
	{
		std::cerr << "issue331 fixture encode failed: " << static_cast<int>(result) << "\n";
		return 2;
	}
	std::cout << hex(encoded) << '\n';
	return 0;
}
