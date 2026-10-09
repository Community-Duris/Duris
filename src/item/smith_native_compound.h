#ifndef SMITH_NATIVE_COMPOUND_H
#define SMITH_NATIVE_COMPOUND_H

#include "item/craft_recipe_continuation.h"
#include "player/player_snapshot.h"
#include "world/quest_mobile_native_binding.h"

#include <array>
#include <cstdint>
#include <vector>

// Reserved only for the original Smith compound. Existing transfer/craft/native
// support predicates remain unchanged until its real participant is connected.
constexpr uint16_t ITEM_TRANSFER_SMITH_NATIVE_COMPOUND_PAYLOAD_VERSION = 17;

// Same canonical PC wallet value layout, a different original price rule.
// Neither this DTO nor validation grants a mapping, source or payment permit.
using smith_native_wallet_cost = craft_refine_wallet_cost;

inline uint32_t smith_native_tier_fee(uint32_t ore_count) noexcept
{
	constexpr std::array<uint32_t, 5> fees{ 2000, 5000, 15000, 30000, 50000 };
	return ore_count >= 1 && ore_count <= fees.size() ? fees[ore_count - 1] : 0;
}

inline bool smith_native_wallet_cost_valid(const smith_native_wallet_cost &cost, uint32_t ore_count,
					   uint32_t fee) noexcept
{
	if (!fee || fee != smith_native_tier_fee(ore_count) || !cost.wallet_mapping_id ||
	    cost.before_revision == UINT64_MAX || cost.after_revision != cost.before_revision + 1)
		return false;
	constexpr std::array<int64_t, 4> units{ 1, 10, 100, 1000 };
	int64_t value = 0;
	for (size_t i = 0; i < units.size(); ++i)
	{
		if (cost.before[i] < 0 || cost.after[i] < 0)
			return false;
		value += int64_t(cost.before[i]) * units[i];
	}
	if (value < fee)
		return false;
	value -= fee;
	// Exact original PC SUB_MONEY normalization; no NPC cash changes.
	for (size_t i = units.size(); i-- > 0;)
	{
		if (value / units[i] > INT32_MAX || cost.after[i] != value / units[i])
			return false;
		value %= units[i];
	}
	return true;
}

// Caller-observed original values, never a sealed producer/publication capability.
// Native selected trees retain native forest order; custody entries separately
// use canonical UID order, and root order retains original ore selection order.
// Complete native/PC images, acknowledged PC save, admission lineage/epoch,
// actual runtime generations and the genuine factory callback/event closure
// belong to the independently bounded original recovery envelope/owner. Birth
// epoch and season are separate facts, never substitutes for admission identity.
struct smith_native_compound_terms
{
	critical_operation_id operation_id{};
	economic_source_event source{};
	uint64_t season_epoch = 0;
	uint32_t player_pid = 0;
	uint64_t expected_player_item_revision = 0;
	quest_mobile_native_cash_reference native_before{};
	smith_native_wallet_cost player_wallet{};
	uint32_t smith_catalog_index = 0, original_menu_choice = 0;
	uint32_t forge_catalog_index = 0, ore_count = 0, fee = 0;
	int32_t first_ore_material = 0;
	std::vector<item_transfer_entry> selected_custody;
	std::vector<player_item_snapshot> selected_native_items;
	std::vector<uint64_t> selected_root_order;
	std::vector<player_item_snapshot> frozen_outputs;
};

#endif
