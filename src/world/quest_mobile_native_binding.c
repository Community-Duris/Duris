#include "world/quest_mobile_native_binding.h"
#include "world/quest_mobile_native_reference.h"
#include "world/quest_mobile_native.h"
#include "persistence/economic_sql_native_mobile_birth_transaction.h"

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <span>

extern P_index mob_index;
extern int top_of_mobt;

static_assert(QUEST_MOBILE_NATIVE_BINDING_BYTES == QUEST_MOBILE_NATIVE_REFERENCE_BYTES);

bool quest_mobile_native_reference_copy(const char_data *character,
					std::uint64_t expected_runtime_id,
					quest_mobile_native_reference *output) noexcept
{
	if (!nevent_require_game_thread("quest_mobile_native_reference_copy") || !character ||
	    !output || !expected_runtime_id ||
	    find_character_by_runtime_id(expected_runtime_id) != character)
		return false;
	// The original generation lookup proves the pointer is still live before
	// any access to storage that the character pool may have released or reused.
	if (character->runtime_id != expected_runtime_id || !IS_NPC(character) ||
	    !character->only.npc || !mob_index || character->only.npc->R_num < 0 ||
	    character->only.npc->R_num > top_of_mobt)
		return false;

	const std::span<const uint8_t> bytes(character->native_mobile_binding.encoded_reference_,
					     QUEST_MOBILE_NATIVE_BINDING_BYTES);
	quest_mobile_native_reference candidate;
	if (quest_mobile_native_reference_decode(bytes, &candidate) !=
	    player_snapshot_codec_result::ok)
		return false;
	// Check the observed live type only; it never supplies a missing reference.
	if (candidate.mobile_vnum != mob_index[character->only.npc->R_num].virtual_number)
		return false;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> canonical = {};
	if (quest_mobile_native_reference_encode(candidate, &canonical) !=
		    player_snapshot_codec_result::ok ||
	    !std::equal(canonical.begin(), canonical.end(), bytes.begin()))
		return false;
	*output = candidate;
	return true;
}

bool quest_mobile_native_publication_binding::advance(
	char_data *character, std::uint64_t expected_runtime_id,
	const quest_mobile_native_reference &before,
	const quest_mobile_native_reference &after) noexcept
{
	if (!nevent_is_game_thread() || before.mobile_revision == UINT64_MAX ||
	    before.stock_revision == UINT64_MAX)
		return false;
	// Every birth/source/type fact is unchanged; both revisions advance once.
	auto expected = before;
	++expected.mobile_revision;
	++expected.stock_revision;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> old_bytes{}, next{}, exact{};
	quest_mobile_native_reference observed;
	if (quest_mobile_native_reference_encode(before, &old_bytes) !=
		    player_snapshot_codec_result::ok ||
	    quest_mobile_native_reference_encode(after, &next) !=
		    player_snapshot_codec_result::ok ||
	    quest_mobile_native_reference_encode(expected, &exact) !=
		    player_snapshot_codec_result::ok ||
	    next != exact ||
	    !quest_mobile_native_reference_copy(character, expected_runtime_id, &observed))
		return false;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> current{};
	if (quest_mobile_native_reference_encode(observed, &current) !=
	    player_snapshot_codec_result::ok)
		return false;
	if (current == next)
		return true; // Exact same originally proven transition retry only.
	if (current != old_bytes)
		return false;
	// Nonallocating serialized value install, after every fallible comparison.
	std::copy(next.begin(), next.end(), character->native_mobile_binding.encoded_reference_);
	return true;
}

bool quest_mobile_native_cash_reference_copy(const char_data *character, std::uint64_t runtime,
					     quest_mobile_native_cash_reference *output) noexcept
{
	if (!output)
		return false;
	quest_mobile_native_cash_reference candidate;
	if (!quest_mobile_native_reference_copy(character, runtime, &candidate.reference))
		return false;
	const auto read = [](const uint8_t *bytes) noexcept
	{
		uint64_t value = 0;
		for (size_t i = 0; i < 8; ++i)
			value |= static_cast<uint64_t>(bytes[i]) << (8 * i);
		return value;
	};
	const auto &binding = character->native_mobile_binding;
	candidate.cash_revision = read(binding.cash_revision_);
	candidate.wallet_mapping_id = read(binding.wallet_mapping_id_);
	std::copy(std::begin(binding.lineage_), std::end(binding.lineage_),
		  candidate.lineage.bytes.begin());
	std::copy(std::begin(binding.birth_epoch_), std::end(binding.birth_epoch_),
		  candidate.birth_epoch.bytes.begin());
	if (!candidate.cash_revision || !candidate.wallet_mapping_id ||
	    critical_operation_id_is_zero(candidate.lineage) ||
	    critical_operation_id_is_zero(candidate.birth_epoch))
		return false;
	candidate.denominations = { GET_COPPER(character), GET_SILVER(character),
				    GET_GOLD(character), GET_PLATINUM(character) };
	if (std::any_of(candidate.denominations.begin(), candidate.denominations.end(),
			[](int64_t value) { return value < 0; }))
		return false;
	*output = candidate;
	return true;
}

bool quest_mobile_native_publication_binding::apply_cost(
	char_data *character, uint64_t runtime, const quest_mobile_native_cash_reference &before,
	const native_quest_cost_projection &cost,
	const quest_mobile_native_reference &after) noexcept
{
	if (!nevent_is_game_thread() || before.reference.mobile_revision == UINT64_MAX ||
	    before.reference.stock_revision == UINT64_MAX || !before.wallet_mapping_id ||
	    cost.before_revision != before.cash_revision || cost.before != before.denominations ||
	    cost.attempts.empty())
		return false;
	std::vector<uint8_t> canonical_cost;
	if (native_quest_cost_projection_encode(cost, &canonical_cost) !=
	    native_quest_cost_projection_result::ok)
		return false;
	auto expected = before.reference;
	++expected.mobile_revision;
	++expected.stock_revision;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> old{}, next{}, exact{}, current{};
	quest_mobile_native_cash_reference observed;
	if (quest_mobile_native_reference_encode(before.reference, &old) !=
		    player_snapshot_codec_result::ok ||
	    quest_mobile_native_reference_encode(after, &next) !=
		    player_snapshot_codec_result::ok ||
	    quest_mobile_native_reference_encode(expected, &exact) !=
		    player_snapshot_codec_result::ok ||
	    next != exact ||
	    !quest_mobile_native_cash_reference_copy(character, runtime, &observed) ||
	    observed.wallet_mapping_id != before.wallet_mapping_id ||
	    observed.lineage.bytes != before.lineage.bytes ||
	    observed.birth_epoch.bytes != before.birth_epoch.bytes ||
	    quest_mobile_native_reference_encode(observed.reference, &current) !=
		    player_snapshot_codec_result::ok)
		return false;
	if (current == next && observed.cash_revision == cost.after_revision &&
	    observed.denominations == cost.after)
		return true; // Only this exact originally proven returned action retry.
	if (current != old || observed.cash_revision != cost.before_revision ||
	    observed.denominations != cost.before)
		return false;
	// Every fallible observation, value check and allocation is complete.
	// The actual original native action journal owns started/unreturned crash
	// uncertainty; a partial cash/reference write grants no repeat or ACK.
	GET_COPPER(character) = static_cast<int>(cost.after[0]);
	GET_SILVER(character) = static_cast<int>(cost.after[1]);
	GET_GOLD(character) = static_cast<int>(cost.after[2]);
	GET_PLATINUM(character) = static_cast<int>(cost.after[3]);
	std::copy(next.begin(), next.end(), character->native_mobile_binding.encoded_reference_);
	for (size_t i = 0; i < 8; ++i)
		character->native_mobile_binding.cash_revision_[i] =
			static_cast<uint8_t>(cost.after_revision >> (8 * i));
	return true;
}

bool quest_mobile_native_publication_binding::apply_money(
	char_data *player, uint64_t player_runtime, uint32_t player_pid, char_data *mobile,
	uint64_t mobile_runtime, const quest_mobile_native_cash_reference &before,
	const native_quest_coin_give_projection &money,
	const quest_mobile_native_reference &after) noexcept
{
	if (!nevent_is_game_thread() || !player || !player_runtime || !player_pid ||
	    find_character_by_runtime_id(player_runtime) != player ||
	    player->runtime_id != player_runtime || IS_NPC(player) || !player->only.pc ||
	    static_cast<uint64_t>(GET_PID(player)) != player_pid ||
	    before.reference.mobile_revision == UINT64_MAX || !before.wallet_mapping_id ||
	    money.mobile_before_revision != before.cash_revision ||
	    money.mobile_before != before.denominations)
		return false;
	native_quest_coin_give_projection projected;
	if (native_quest_coin_give_project(money.player_before, money.player_before_revision,
					   money.mobile_before, money.mobile_before_revision,
					   money.denomination, money.quantity,
					   &projected) != native_quest_coin_give_result::ok ||
	    projected != money)
		return false;
	auto expected = before.reference;
	++expected.mobile_revision;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> original{}, next{}, exact{},
		current{};
	quest_mobile_native_cash_reference observed;
	if (quest_mobile_native_reference_encode(before.reference, &original) !=
		    player_snapshot_codec_result::ok ||
	    quest_mobile_native_reference_encode(after, &next) !=
		    player_snapshot_codec_result::ok ||
	    quest_mobile_native_reference_encode(expected, &exact) !=
		    player_snapshot_codec_result::ok ||
	    next != exact ||
	    !quest_mobile_native_cash_reference_copy(mobile, mobile_runtime, &observed) ||
	    observed.wallet_mapping_id != before.wallet_mapping_id ||
	    observed.lineage.bytes != before.lineage.bytes ||
	    observed.birth_epoch.bytes != before.birth_epoch.bytes ||
	    quest_mobile_native_reference_encode(observed.reference, &current) !=
		    player_snapshot_codec_result::ok)
		return false;
	const std::array<int64_t, 4> player_cash{ GET_COPPER(player), GET_SILVER(player),
						  GET_GOLD(player), GET_PLATINUM(player) };
	if (current == next && observed.cash_revision == money.mobile_after_revision &&
	    observed.denominations == money.mobile_after && player_cash == money.player_after &&
	    player->only.pc->wallet_revision == money.player_after_revision)
		return true; // Exact returned action only; caller still owns its journal proof.
	if (current != original || observed.cash_revision != money.mobile_before_revision ||
	    observed.denominations != money.mobile_before || player_cash != money.player_before ||
	    player->only.pc->wallet_revision != money.player_before_revision)
		return false;
	// No fallible observation or allocation remains. Started/unreturned journal
	// uncertainty protects the combined action across any partial native write.
	GET_COPPER(player) = static_cast<int>(money.player_after[0]);
	GET_SILVER(player) = static_cast<int>(money.player_after[1]);
	GET_GOLD(player) = static_cast<int>(money.player_after[2]);
	GET_PLATINUM(player) = static_cast<int>(money.player_after[3]);
	player->only.pc->wallet_revision = money.player_after_revision;
	GET_COPPER(mobile) = static_cast<int>(money.mobile_after[0]);
	GET_SILVER(mobile) = static_cast<int>(money.mobile_after[1]);
	GET_GOLD(mobile) = static_cast<int>(money.mobile_after[2]);
	GET_PLATINUM(mobile) = static_cast<int>(money.mobile_after[3]);
	std::copy(next.begin(), next.end(), mobile->native_mobile_binding.encoded_reference_);
	for (size_t i = 0; i < 8; ++i)
		mobile->native_mobile_binding.cash_revision_[i] =
			static_cast<uint8_t>(money.mobile_after_revision >> (8 * i));
	return true;
}

bool quest_mobile_native_publication_binding::restore_money_metadata(
	char_data *mobile, uint64_t runtime, const quest_mobile_native_image &current,
	const native_mobile_wallet_origin &origin) noexcept
{
	if (!nevent_is_game_thread() || !current.cash || !current.cash->revision ||
	    current.state != quest_mobile_lifetime_state::live || !origin.wallet_mapping_id ||
	    origin.mobile_instance_id != current.reference.mobile_instance_id ||
	    origin.birth_operation.bytes != current.reference.birth_operation.bytes ||
	    std::all_of(origin.lineage.bytes.begin(), origin.lineage.bytes.end(),
			[](uint8_t v) { return !v; }) ||
	    std::all_of(origin.birth_epoch.bytes.begin(), origin.birth_epoch.bytes.end(),
			[](uint8_t v) { return !v; }))
		return false;
	try
	{
		quest_mobile_native_reference observed;
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> exact{}, actual{};
		if (!quest_mobile_native_reference_copy(mobile, runtime, &observed) ||
		    quest_mobile_native_reference_encode(observed, &actual) !=
			    player_snapshot_codec_result::ok ||
		    quest_mobile_native_reference_encode(current.reference, &exact) !=
			    player_snapshot_codec_result::ok ||
		    actual != exact ||
		    current.cash->denominations.amount !=
			    std::array<int64_t, 4>{ GET_COPPER(mobile), GET_SILVER(mobile),
						    GET_GOLD(mobile), GET_PLATINUM(mobile) })
			return false;
		std::vector<player_item_snapshot> forest;
		std::vector<uint8_t> original, physical;
		if (quest_mobile_native_items_observe(mobile, observed, &forest) !=
			    player_snapshot_capture_result::ok ||
		    player_item_snapshot_list_encode(forest, &physical) !=
			    player_snapshot_codec_result::ok ||
		    player_item_snapshot_list_encode(current.items, &original) !=
			    player_snapshot_codec_result::ok ||
		    original != physical)
			return false;
		std::array<uint8_t, QUEST_MOBILE_NATIVE_CASH_BINDING_METADATA_BYTES> target{},
			existing{};
		const auto put = [](uint8_t *out, uint64_t value)
		{
			for (size_t i = 0; i < 8; ++i)
				out[i] = static_cast<uint8_t>(value >> (i * 8));
		};
		put(target.data(), current.cash->revision);
		put(target.data() + 8, origin.wallet_mapping_id);
		std::copy(origin.lineage.bytes.begin(), origin.lineage.bytes.end(),
			  target.begin() + 16);
		std::copy(origin.birth_epoch.bytes.begin(), origin.birth_epoch.bytes.end(),
			  target.begin() + 32);
		auto &binding = mobile->native_mobile_binding;
		std::copy(std::begin(binding.cash_revision_), std::end(binding.cash_revision_),
			  existing.begin());
		std::copy(std::begin(binding.wallet_mapping_id_),
			  std::end(binding.wallet_mapping_id_), existing.begin() + 8);
		std::copy(std::begin(binding.lineage_), std::end(binding.lineage_),
			  existing.begin() + 16);
		std::copy(std::begin(binding.birth_epoch_), std::end(binding.birth_epoch_),
			  existing.begin() + 32);
		if (existing == target)
			return true;
		if (!std::all_of(existing.begin(), existing.end(),
				 [](uint8_t value) { return !value; }))
			return false;
		// All authentic literal/canonical/origin comparisons precede nonallocating
		// writes. Refusal never repairs a partially installed/mixed wallet witness.
		std::copy_n(target.begin(), 8, binding.cash_revision_);
		std::copy_n(target.begin() + 8, 8, binding.wallet_mapping_id_);
		std::copy_n(target.begin() + 16, 16, binding.lineage_);
		std::copy_n(target.begin() + 32, 16, binding.birth_epoch_);
		return true;
	}
	catch (...)
	{
		return false;
	}
}
