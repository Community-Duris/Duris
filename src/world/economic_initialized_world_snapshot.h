#ifndef ECONOMIC_INITIALIZED_WORLD_SNAPSHOT_H
#define ECONOMIC_INITIALIZED_WORLD_SNAPSHOT_H

#include "persistence/economic_sql_source_snapshot.h"
#include "player/player_snapshot.h"
#include "world/quest_mobile_native_binding.h"

#include <optional>
#include <string>
#include <vector>

constexpr uint32_t ECONOMIC_WORLD_NO_INDEX = UINT32_MAX;
enum class economic_world_item_location : uint8_t
{
	room,
	inside,
	carried,
	equipment,
};
struct economic_world_item_observation
{
	// Complete literal forest order, parent-before-child. No saved NORENT filter.
	player_item_snapshot literal;
	economic_world_item_location location = {};
	// Room/body/item vector index according to location; no native pointer.
	uint32_t owner_index = ECONOMIC_WORLD_NO_INDEX;
	uint32_t root_index = ECONOMIC_WORLD_NO_INDEX;
};
struct economic_world_room_observation
{
	int32_t room_rnum = -1, room_vnum = 0;
	uint32_t zone_rnum = 0;
	int32_t zone_vnum = -1;
	// Actual native sibling/person order, including present-empty rooms.
	std::vector<uint32_t> roots, people;
};
struct economic_world_body_observation
{
	bool npc = false, globally_registered = false;
	uint64_t runtime_id = 0;
	// Raw PID/rnum facts; zero PID or unavailable prototype never becomes identity.
	int32_t player_pid = 0, mobile_rnum = -1, native_pet_id = 0;
	std::optional<int32_t> mobile_vnum;
	int32_t shop_id = -1;
	uint64_t durable_pet_uid = 0;
	uint32_t durable_pet_owner_pid = 0;
	int32_t room_rnum = -1;
	uint32_t room_index = ECONOMIC_WORLD_NO_INDEX;
	uint32_t descriptor_index = ECONOMIC_WORLD_NO_INDEX;
	uint32_t original_body_index = ECONOMIC_WORLD_NO_INDEX;
	uint32_t player_body_index = ECONOMIC_WORLD_NO_INDEX;
	uint64_t wallet_revision = 0;
	std::array<int64_t, 4> cash{};
	std::vector<uint32_t> equipment_roots, inventory_roots;
	// Failed copy is unobserved, not proof that private metadata is absent/valid.
	// A legacy NPC receives no manufactured mobile/PID/wallet identity.
	std::optional<quest_mobile_native_reference> native_reference;
	std::optional<quest_mobile_native_cash_reference> native_cash_reference;
};
struct economic_world_descriptor_observation
{
	uint32_t character_index = ECONOMIC_WORLD_NO_INDEX;
	uint32_t original_index = ECONOMIC_WORLD_NO_INDEX;
	// Preserve actual account availability even for a switched original body.
	std::optional<std::string> account_name;
};
struct economic_world_bank_projection
{
	uint32_t body_index = ECONOMIC_WORLD_NO_INDEX;
	uint8_t racewar = 0;
	// Actual descriptor account only; disconnected identity remains unavailable.
	// Private evidence; never print names or balances in diagnostics.
	std::optional<std::string> account_name;
	std::array<int64_t, 4> denominations{};
	uint64_t revision = 0;
};
struct economic_initialized_world_snapshot
{
	uint32_t version = 1;
	std::vector<economic_world_room_observation> rooms;
	std::vector<economic_world_body_observation> bodies;
	std::vector<economic_world_descriptor_observation> descriptors;
	std::vector<economic_world_item_observation> items;
	// Complete object_list order mapped to owning item values, never pointers.
	std::vector<uint32_t> object_registry_order;
	// Per-PC cached bank PROJECTIONS, not distinct balances. Never sum these rows.
	// Shared account/racewar identity and SQL comparison belong to normalization.
	std::vector<economic_world_bank_projection> bank_projections;
	// Rows: room/body/descriptor/item/bank projection and literal affect/extra/spell.
	// Cells count retained primitive/array values and strings; bytes use fixed
	// primitive widths and exact string lengths. NULL/absent fields count zero
	// bytes. Vector/allocator overhead is additional, bounded by original limits.
	uint64_t rows = 0, cells = 0, cell_bytes = 0;
};

// Complete bounded game-thread observation. Return 0 only for the complete
// reciprocal world/registry/closure; malformed, detached, UID-less, cyclic,
// duplicate or over-budget graphs refuse with output unchanged. Every PC/NPC
// cash denomination, including zero, is retained; bank projections stay separate.
// Shared original capture ceilings remain unchanged. Temporary literal tree
// storage additionally obeys the original player snapshot capture bounds.
// This API provides no initialized-world/boot/copyover/activation, custody, SQL,
// mutation or ACK authority. Root's real lifecycle owner must authenticate the
// successful boot cut and maintain its validity across later gameplay changes.
unsigned int
economic_initialized_world_snapshot_capture(const economic_sql_source_limits &,
					    economic_initialized_world_snapshot *) noexcept;

#endif
