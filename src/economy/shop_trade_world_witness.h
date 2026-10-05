#ifndef SHOP_TRADE_WORLD_WITNESS_H
#define SHOP_TRADE_WORLD_WITNESS_H

#include "player/player_snapshot.h"
#include <span>

struct char_data;
struct obj_data;

enum class shop_trade_world_location : uint8_t
{
	absent,
	actor_inventory,
	actor_equipment,
	keeper_inventory,
	keeper_equipment,
	inside,
	detached
};
struct shop_trade_world_uid_expectation
{
	uint64_t uid = 0;
	shop_trade_world_location location = shop_trade_world_location::absent;
	uint64_t parent_uid = 0;
	int16_t equipment_slot = 0; // Existing snapshot convention: equipment slot + 1.
	int32_t native_item_id = -1; // -1 makes no row-ID assertion; zero is exact staged zero.
};
struct shop_trade_world_expectation
{
	uint32_t actor_pid = 0;
	uint64_t actor_runtime_id =
		0; // Zero requires complete PID-body absence and empty player list.
	uint64_t keeper_runtime_id = 0;
	uint32_t shop_id = 0;
	int32_t keeper_vnum = 0;
	std::span<const player_item_snapshot> player_items;
	std::span<const player_item_snapshot> keeper_items;
	std::span<const player_item_snapshot> detached_items;
	// Explicit original subtree policy, never inferred from incoming masks.
	// A selected subtree may be nested beneath a generic destination container.
	std::span<const uint64_t> literal_player_root_uids;
	std::span<const shop_trade_world_uid_expectation> uid_locations;
};
struct shop_trade_world_witness
{
	char_data *actor = nullptr;
	char_data *keeper = nullptr;
	// Complete bounded physical actor census, including saved-policy omitted
	// NORENT bodies. This is an observation, never a custody or ACK capability.
	size_t player_physical_item_count = 0;
	std::vector<player_item_snapshot> player_items;
	std::vector<player_item_snapshot> keeper_items;
	std::vector<player_item_snapshot> detached_items;
	// Same order as uid_locations; absent requirements produce nullptr.
	std::vector<obj_data *> objects;
};
// Already-bound game thread only. Strong output; no world mutation, native SQL,
// custody authority, native-handler-tail proof or ACK capability. Pointers are
// transient and must be reobserved after callbacks; never retain across pulses.
// Player list uses existing saved NORENT/string policies plus explicit literal
// roots; every omitted physical sibling is still included in the world census.
bool shop_trade_world_witness_observe(const shop_trade_world_expectation &,
				      shop_trade_world_witness *) noexcept;
// Pure feasibility check reuses the observer's native budget/DFS/wire rules.
// Original held values grant no custody, placement or publication authority.
bool shop_trade_world_player_values_supported(std::span<const player_item_snapshot>) noexcept;

struct shop_trade_payload;
// Pure conversion of already derived AFTER values to actual native insertion
// order (before first same-R_num sibling, otherwise head). Caller must first
// census these transient pointers under its original owner exclusions.
// Includes omitted NORENT physical anchors, preserves equipment/root/DFS order
// and existing bounds. No mutation, SQL, publication, handler-tail or ACK proof.
bool shop_trade_world_expected_player_order(const shop_trade_payload &, char_data *actor,
					    obj_data *selected, obj_data *destination,
					    const std::vector<player_item_snapshot> &values,
					    std::vector<player_item_snapshot> *output) noexcept;

// Same pure insertion conversion for a stored sale into the addressed NPC.
// Caller proves the original keeper runtime identity and full literal stock;
// VNUM and typed action checks here do not authenticate that native authority.
bool shop_trade_world_expected_keeper_order(const shop_trade_payload &, char_data *keeper,
					    obj_data *selected,
					    const std::vector<player_item_snapshot> &values,
					    std::vector<player_item_snapshot> *output) noexcept;

#endif
