#ifndef SHOP_TRADE_WORLD_WITNESS_H
#define SHOP_TRADE_WORLD_WITNESS_H

#include "player/player_snapshot.h"
#include "economy/shop_trade_recovery_manifest.h"
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

// Original decoded v8 values and current native forests are expectations only.
// The observer neither authenticates them nor rebinds/adopts a loaded body.
struct shop_trade_world_cold_request
{
	const shop_trade_payload *original = nullptr;
	std::span<const player_item_snapshot> player_items;
	std::span<const player_item_snapshot> keeper_items;
	std::span<const shop_trade_world_uid_expectation> uid_locations;
};
struct shop_trade_world_cold_observation
{
	char_data *actor = nullptr;
	char_data *keeper = nullptr;
	uint64_t actor_runtime_id = 0;
	uint64_t keeper_runtime_id = 0;
	bool actor_present = false;
	bool keeper_present = false;
	// A disconnected body is present, but its account identity is unavailable.
	bool actor_account_available = false;
	bool actor_account_matched = false;
	bool player_current_match = false;
	bool keeper_current_match = false;
	bool uid_locations_current_match = false;
	bool target_present = false;
	size_t player_physical_item_count = 0;
	size_t keeper_physical_item_count = 0;
	std::vector<player_item_snapshot> player_items;
	std::vector<player_item_snapshot> keeper_items;
	// Only the original target_parent_item_uid, under the audited actor.
	std::vector<player_item_snapshot> target_items;
	// Original selected tree if physically held by either addressed body or
	// exactly detached. This is an observed literal, never a fabricated BEFORE.
	std::vector<player_item_snapshot> selected_items;
	// Actual role-specific bindings; absent bodies leave absent bindings, while
	// present-empty bodies have canonical empty bindings. Caller compares these
	// with the immutable original manifest before deciding any publication.
	shop_trade_recovery_manifest observed_bindings;
	// Request order, nullptr means globally absent. Never retain these pointers.
	std::vector<obj_data *> objects;
};
// Pure bounded game-thread observation, strong output. True can report a valid
// BEFORE placement with current_match=false, or a truly absent owner body.
// Explicit location::absent still requires complete global UID absence.
// Malformed/aliased/foreign graphs, identity conflict, codec/budget/allocation
// refusal return false. No SQL, native/runtime authority, mutation or ACK.
// Player strings use saved policy plus the original selected literal subtree
// only when actually on the actor; keeper and optional target are literal.
// Account-unavailable, transient runtime IDs and value bindings grant no new
// authority. Reobserve every pointer and graph after native callbacks.
bool shop_trade_world_cold_observe(const shop_trade_world_cold_request &,
				   shop_trade_world_cold_observation *) noexcept;

#endif
