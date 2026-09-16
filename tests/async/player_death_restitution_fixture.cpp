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

critical_operation_id operation(uint8_t base)
{
	critical_operation_id id = {};
	for (size_t index = 0; index < id.bytes.size(); ++index)
		id.bytes[index] = static_cast<uint8_t>(base + index);
	return id;
}

player_item_snapshot item(uint64_t uid, int32_t vnum, int32_t parent, const char *name)
{
	player_item_snapshot value = {};
	value.parent_index = parent;
	value.equipment_slot = -1;
	value.object_uid = uid;
	value.generated_key = static_cast<int64_t>(uid + 9000);
	value.vnum = vnum;
	value.type = 4;
	value.string_mask = 15;
	value.name = name;
	value.short_description = std::string("short-") + name;
	value.description = std::string("description-") + name;
	value.action_description = std::string("action-") + name;
	value.values = { 11, 12, 13, 14, 15, 16, 17, 18 };
	value.timers = { 1700000000, 1700000001, 1700000002, 1700000003, 1700000004, 1700000005 };
	value.wear_flags = 21;
	value.extra_flags = 22;
	value.anti_flags = 23;
	value.anti2_flags = 24;
	value.extra2_flags = 25;
	value.weight = 26;
	value.material = 7;
	value.cost = 28;
	value.condition = 87;
	value.craftsmanship = 29;
	value.bitvectors = { 31, 32, 33, 34, 35 };
	value.affects = { { { 36, 37 }, { 0, 0 }, { 0, 0 }, { 0, 0 } } };
	value.dynamic_affects.push_back({ 38, 39, 40 });
	value.extra_descriptions.push_back({ "runes", "glowing", false, {} });
	return value;
}

player_death_custody_snapshot custody(uint64_t uid, uint64_t root, uint64_t parent, int32_t vnum)
{
	player_death_custody_snapshot row = {};
	row.item = { uid, root, parent, 10, vnum, item_custody_state::active };
	row.owner = { item_owner_type::player, 42, 0 };
	row.owner_revision = 5;
	return row;
}

player_snapshot death_snapshot(uint64_t revision, uint8_t operation_base, bool full)
{
	player_snapshot snapshot = {};
	snapshot.schema_version = PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
	snapshot.pid = 42;
	snapshot.revision = revision;
	snapshot.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
	snapshot.save_intent = RENT_DEATH;
	snapshot.encoded_size_bound = PLAYER_SNAPSHOT_MAX_BYTES;
	snapshot.death.emplace();
	snapshot.death->operation_id = operation(operation_base);
	snapshot.death->corpse_room_vnum = 1234;
	snapshot.death->wallet_revision = 9;
	snapshot.death->wallet_before = full ? std::array<int32_t, 4>{ 1, 2, 3, 4 } :
					       std::array<int32_t, 4>{ 0, 0, 0, 0 };
	snapshot.death->wallet_pile_uid = full ? 7000 : 0;
	player_item_snapshot corpse = {};
	corpse.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	corpse.object_uid = full ? 900 : 901;
	corpse.vnum = VOBJ_CORPSE;
	corpse.type = ITEM_CORPSE;
	corpse.string_mask = 1;
	corpse.name = "corpse";
	corpse.values[CORPSE_PID] = 42;
	corpse.values[CORPSE_SAVEID] = static_cast<int32_t>(revision);
	corpse.values[CORPSE_FLAGS] = PC_CORPSE;
	snapshot.death->corpse.push_back(corpse);
	if (!full)
		return snapshot;
	player_item_snapshot wallet = item(7000, VOBJ_COINS, 0, "coins");
	wallet.type = ITEM_MONEY;
	wallet.values = { 1, 2, 3, 4, 0, 0, 0, 0 };
	snapshot.death->corpse.push_back(wallet);
	snapshot.death->corpse.push_back(item(1000, 100, 0, "container-A"));
	snapshot.death->corpse.push_back(item(1001, 101, 2, "container-B"));
	snapshot.death->corpse.push_back(item(1002, 102, 3, "child-C"));
	snapshot.death->corpse.push_back(item(1003, 103, 2, "unique gloves"));
	player_item_snapshot artifact = item(1004, 104, 0, "artifact blade");
	artifact.extra_flags = UINT32_C(1) << 29;
	artifact.timers = { 1700000100, 1700000101, 1700000102, 1700000103, 1700000104, 1700000105 };
	artifact.parent_index = 2;
	snapshot.death->corpse.push_back(artifact);
	snapshot.death->custody.push_back(custody(1000, 1000, 0, 100));
	snapshot.death->custody.push_back(custody(1001, 1000, 1000, 101));
	snapshot.death->custody.push_back(custody(1002, 1000, 1000, 102));
	snapshot.death->custody.push_back(custody(1003, 1000, 1000, 103));
	snapshot.death->custody.push_back(custody(1004, 1000, 1000, 104));
	snapshot.death->custody.push_back(custody(7000, 7000, 0, VOBJ_COINS));
	snapshot.death->custody.push_back(custody(1999, 1999, 0, 999));
	return snapshot;
}
}

int main()
{
	for (const auto &case_data :
	     { std::pair<uint64_t, std::pair<uint8_t, bool>>{ 7, { 0xa0, true } },
	       { 8, { 0xb0, false } } })
	{
		std::vector<uint8_t> encoded;
		const auto snapshot = death_snapshot(case_data.first, case_data.second.first,
						     case_data.second.second);
		if (player_snapshot_encode(snapshot, &encoded) != player_snapshot_codec_result::ok)
		{
			std::cerr << "encode failed "
				  << static_cast<int>(player_snapshot_encode(snapshot, &encoded))
				  << '\n';
			return 2;
		}
		std::cout << hex(encoded) << '\n';
	}
	return 0;
}
