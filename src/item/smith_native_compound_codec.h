#ifndef SMITH_NATIVE_COMPOUND_CODEC_H
#define SMITH_NATIVE_COMPOUND_CODEC_H

#include "item/smith_native_compound.h"
#include "economy/economic_accounting_types.h"
#include "player/player_snapshot_codec.h"
#include "core/defines.h"

#include <algorithm>
#include <bit>
#include <span>
#include <type_traits>
#include <utility>

// Value envelope only. Decoding grants no source claim, factory ownership,
// checkpoint, admission, transaction, publication or ACK capability.
constexpr size_t SMITH_NATIVE_COMPOUND_HEADER_BYTES = 424;
constexpr std::array<uint8_t, 8> SMITH_NATIVE_COMPOUND_MAGIC{ 'S', 'M', 'I', 'T', 'H', '1', 0, 0 };

namespace smith_native_compound_codec_detail
{
struct reader
{
	std::span<const uint8_t> bytes;
	size_t offset = 0;
	bool good = true;

	std::span<const uint8_t> take(size_t count) noexcept
	{
		if (!good || offset > bytes.size() || count > bytes.size() - offset)
		{
			good = false;
			return {};
		}
		auto part = bytes.subspan(offset, count);
		offset += count;
		return part;
	}

	template <typename T> T integer() noexcept
	{
		using U = std::make_unsigned_t<T>;
		const auto part = take(sizeof(T));
		uint64_t value = 0;
		for (size_t i = 0; i < part.size(); ++i)
			value |= uint64_t(part[i]) << (8 * i);
		if constexpr (std::is_signed_v<T>)
			return std::bit_cast<T>(static_cast<U>(value));
		else
			return static_cast<T>(value);
	}

	critical_operation_id id() noexcept
	{
		critical_operation_id value{};
		const auto part = take(value.bytes.size());
		if (good)
			std::copy(part.begin(), part.end(), value.bytes.begin());
		return value;
	}
};

template <typename T> void put(std::vector<uint8_t> &bytes, T value)
{
	using U = std::make_unsigned_t<T>;
	U bits = static_cast<U>(value);
	for (size_t i = 0; i < sizeof(T); ++i)
	{
		bytes.push_back(static_cast<uint8_t>(bits & 0xff));
		bits >>= 8;
	}
}

// Same complete-literal/contiguous-DFS rule as the original native image.
// Generic snapshot validity alone permits reopening an earlier sibling.
inline bool literal_forest(std::span<const player_item_snapshot> items) noexcept
{
	constexpr uint8_t literal_mask = STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3;
	std::array<int32_t, PLAYER_SNAPSHOT_MAX_DEPTH> path{};
	size_t depth = 0;
	for (size_t i = 0; i < items.size(); ++i)
	{
		const auto &item = items[i];
		if (item.vnum < 0 || item.string_mask != literal_mask)
			return false;
		if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
		{
			depth = 1;
			path[0] = static_cast<int32_t>(i);
		}
		else
		{
			if (item.parent_index < 0 || size_t(item.parent_index) >= i)
				return false;
			while (depth && path[depth - 1] != item.parent_index)
				--depth;
			if (!depth || depth == path.size())
				return false;
			path[depth++] = static_cast<int32_t>(i);
		}
	}
	return true;
}

inline bool shape(const smith_native_compound_terms &terms)
{
	const auto &native = terms.native_before;
	if (critical_operation_id_is_zero(terms.operation_id) ||
	    terms.source.kind != economic_source_kind::crafting ||
	    !economic_source_event_valid(terms.source) || !terms.player_pid ||
	    terms.player_pid > INT32_MAX || terms.expected_player_item_revision == UINT64_MAX ||
	    !quest_mobile_native_reference_valid(native.reference) ||
	    native.reference.stock_revision == UINT64_MAX ||
	    native.reference.mobile_revision == UINT64_MAX || !native.wallet_mapping_id ||
	    !native.cash_revision || critical_operation_id_is_zero(native.lineage) ||
	    critical_operation_id_is_zero(native.birth_epoch) ||
	    std::any_of(native.denominations.begin(), native.denominations.end(),
			[](int64_t value) { return value < 0; }) ||
	    !smith_native_wallet_cost_valid(terms.player_wallet, terms.ore_count, terms.fee) ||
	    !terms.original_menu_choice || terms.selected_root_order.size() != terms.ore_count ||
	    terms.selected_native_items.empty() || terms.frozen_outputs.empty() ||
	    terms.selected_native_items.size() > ITEM_TRANSFER_MAX_ITEMS ||
	    terms.frozen_outputs.size() >
		    ITEM_TRANSFER_MAX_ITEMS - terms.selected_native_items.size() ||
	    terms.selected_custody.size() != terms.selected_native_items.size())
		return false;
	for (size_t i = 0; i < terms.selected_custody.size(); ++i)
	{
		const auto &row = terms.selected_custody[i];
		if (!row.item_uid || row.item_uid == UINT64_MAX || !row.root_item_uid ||
		    !row.expected_item_revision || row.expected_item_revision == UINT64_MAX ||
		    row.expected_state != item_custody_state::active ||
		    (i && terms.selected_custody[i - 1].item_uid >= row.item_uid))
			return false;
	}
	for (size_t i = 0; i < terms.selected_root_order.size(); ++i)
	{
		if (!terms.selected_root_order[i])
			return false;
		for (size_t j = 0; j < i; ++j)
			if (terms.selected_root_order[j] == terms.selected_root_order[i])
				return false;
	}
	if (!literal_forest(terms.selected_native_items) || !literal_forest(terms.frozen_outputs))
		return false;
	std::array<bool, 5> seen_roots{};
	std::vector<uint64_t> seen;
	seen.reserve(terms.selected_native_items.size() + terms.frozen_outputs.size());
	uint64_t current_root = 0;
	bool first_material_seen = false;
	for (size_t i = 0; i < terms.selected_native_items.size(); ++i)
	{
		const auto &item = terms.selected_native_items[i];
		uint64_t parent = 0;
		if (item.equipment_slot)
			return false;
		if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
		{
			current_root = item.object_uid;
			auto root = std::find(terms.selected_root_order.begin(),
					      terms.selected_root_order.end(), current_root);
			if (root == terms.selected_root_order.end())
				return false;
			const size_t index = root - terms.selected_root_order.begin();
			if (seen_roots[index])
				return false;
			seen_roots[index] = true;
			if (!index)
			{
				if (item.material != terms.first_ore_material)
					return false;
				first_material_seen = true;
			}
		}
		else
		{
			if (item.parent_index < 0 || size_t(item.parent_index) >= i)
				return false;
			parent = terms.selected_native_items[item.parent_index].object_uid;
		}
		auto entry = std::lower_bound(terms.selected_custody.begin(),
					      terms.selected_custody.end(), item.object_uid,
					      [](const auto &row, uint64_t uid)
					      { return row.item_uid < uid; });
		if (entry == terms.selected_custody.end() || entry->item_uid != item.object_uid ||
		    entry->root_item_uid != current_root || entry->parent_item_uid != parent ||
		    entry->vnum != item.vnum)
			return false;
		if (parent)
		{
			auto parent_entry = std::lower_bound(terms.selected_custody.begin(),
							     terms.selected_custody.end(), parent,
							     [](const auto &row, uint64_t uid)
							     { return row.item_uid < uid; });
			if (parent_entry == terms.selected_custody.end() ||
			    parent_entry->item_uid != parent ||
			    parent_entry->root_item_uid != current_root)
				return false;
		}
		seen.push_back(item.object_uid);
	}
	if (!first_material_seen ||
	    !std::all_of(seen_roots.begin(), seen_roots.begin() + terms.ore_count,
			 [](bool value) { return value; }))
		return false;
	for (size_t i = 0; i < terms.frozen_outputs.size(); ++i)
	{
		const auto &item = terms.frozen_outputs[i];
		if (!item.object_uid || item.object_uid == UINT64_MAX || item.equipment_slot ||
		    (!i ? item.parent_index != PLAYER_SNAPSHOT_NO_PARENT || item.vnum != 1255 ||
				     item.material != terms.first_ore_material :
			  item.parent_index < 0 || size_t(item.parent_index) >= i))
			return false;
		seen.push_back(item.object_uid);
	}
	std::sort(seen.begin(), seen.end());
	return std::adjacent_find(seen.begin(), seen.end()) == seen.end();
}
} // namespace smith_native_compound_codec_detail

inline player_snapshot_codec_result
smith_native_compound_encode(const smith_native_compound_terms &terms,
			     std::vector<uint8_t> *output) noexcept
{
	using namespace smith_native_compound_codec_detail;
	if (!output)
		return player_snapshot_codec_result::invalid_value;
	try
	{
		if (!shape(terms))
			return player_snapshot_codec_result::invalid_value;
		std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source{};
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> reference{};
		std::vector<uint8_t> inputs, outputs;
		if (economic_source_event_encode(terms.source, &source) !=
			    economic_accounting_error::ok ||
		    quest_mobile_native_reference_encode(terms.native_before.reference,
							 &reference) !=
			    player_snapshot_codec_result::ok)
			return player_snapshot_codec_result::invalid_value;
		auto status =
			player_item_snapshot_list_encode(terms.selected_native_items, &inputs);
		if (status != player_snapshot_codec_result::ok)
			return status;
		status = player_item_snapshot_list_encode(terms.frozen_outputs, &outputs);
		if (status != player_snapshot_codec_result::ok)
			return status;
		if (inputs.size() > ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES ||
		    outputs.size() > ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES)
			return player_snapshot_codec_result::limit_exceeded;
		const size_t total = SMITH_NATIVE_COMPOUND_HEADER_BYTES +
				     terms.selected_custody.size() * ITEM_TRANSFER_ENTRY_BYTES +
				     terms.selected_root_order.size() * sizeof(uint64_t) +
				     inputs.size() + outputs.size();
		if (total > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
			return player_snapshot_codec_result::limit_exceeded;
		std::vector<uint8_t> bytes;
		bytes.reserve(total);
		auto append = [&](const auto &part)
		{ bytes.insert(bytes.end(), part.begin(), part.end()); };
		append(SMITH_NATIVE_COMPOUND_MAGIC);
		put<uint16_t>(bytes, ITEM_TRANSFER_SMITH_NATIVE_COMPOUND_PAYLOAD_VERSION);
		put<uint16_t>(bytes, 0);
		put<uint32_t>(bytes, total);
		append(terms.operation_id.bytes);
		append(source);
		put<uint64_t>(bytes, terms.season_epoch);
		put<uint32_t>(bytes, terms.player_pid);
		put<uint64_t>(bytes, terms.expected_player_item_revision);
		append(reference);
		const auto &native = terms.native_before;
		append(native.lineage.bytes);
		append(native.birth_epoch.bytes);
		put<uint64_t>(bytes, native.wallet_mapping_id);
		put<uint64_t>(bytes, native.cash_revision);
		for (auto value : native.denominations)
			put<int64_t>(bytes, value);
		const auto &wallet = terms.player_wallet;
		put<uint64_t>(bytes, wallet.wallet_mapping_id);
		put<uint64_t>(bytes, wallet.before_revision);
		put<uint64_t>(bytes, wallet.after_revision);
		for (auto value : wallet.before)
			put<int32_t>(bytes, value);
		for (auto value : wallet.after)
			put<int32_t>(bytes, value);
		put<uint32_t>(bytes, terms.smith_catalog_index);
		put<uint32_t>(bytes, terms.original_menu_choice);
		put<uint32_t>(bytes, terms.forge_catalog_index);
		put<uint32_t>(bytes, terms.ore_count);
		put<uint32_t>(bytes, terms.fee);
		put<int32_t>(bytes, terms.first_ore_material);
		put<uint32_t>(bytes, terms.selected_custody.size());
		put<uint32_t>(bytes, terms.selected_root_order.size());
		put<uint32_t>(bytes, inputs.size());
		put<uint32_t>(bytes, outputs.size());
		if (bytes.size() != SMITH_NATIVE_COMPOUND_HEADER_BYTES)
			return player_snapshot_codec_result::invalid_value;
		for (const auto &entry : terms.selected_custody)
		{
			put<uint64_t>(bytes, entry.item_uid);
			put<uint64_t>(bytes, entry.root_item_uid);
			put<uint64_t>(bytes, entry.parent_item_uid);
			put<uint64_t>(bytes, entry.expected_item_revision);
			put<int32_t>(bytes, entry.vnum);
			put<uint8_t>(bytes, static_cast<uint8_t>(entry.expected_state));
			put<uint8_t>(bytes, 0);
			put<uint16_t>(bytes, 0);
		}
		for (uint64_t uid : terms.selected_root_order)
			put<uint64_t>(bytes, uid);
		append(inputs);
		append(outputs);
		if (bytes.size() != total)
			return player_snapshot_codec_result::invalid_value;
		*output = std::move(bytes);
		return player_snapshot_codec_result::ok;
	}
	catch (...)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
}

inline player_snapshot_codec_result
smith_native_compound_decode(std::span<const uint8_t> bytes,
			     smith_native_compound_terms *output) noexcept
{
	using namespace smith_native_compound_codec_detail;
	if (!output || bytes.size() < SMITH_NATIVE_COMPOUND_HEADER_BYTES ||
	    bytes.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return player_snapshot_codec_result::invalid_value;
	try
	{
		reader in{ bytes };
		const auto magic = in.take(SMITH_NATIVE_COMPOUND_MAGIC.size());
		if (!std::equal(magic.begin(), magic.end(), SMITH_NATIVE_COMPOUND_MAGIC.begin()) ||
		    in.integer<uint16_t>() != ITEM_TRANSFER_SMITH_NATIVE_COMPOUND_PAYLOAD_VERSION ||
		    in.integer<uint16_t>() || in.integer<uint32_t>() != bytes.size())
			return player_snapshot_codec_result::invalid_value;
		smith_native_compound_terms terms;
		terms.operation_id = in.id();
		if (economic_source_event_decode(in.take(ECONOMIC_SOURCE_EVENT_BYTES),
						 &terms.source) != economic_accounting_error::ok)
			return player_snapshot_codec_result::invalid_value;
		terms.season_epoch = in.integer<uint64_t>();
		terms.player_pid = in.integer<uint32_t>();
		terms.expected_player_item_revision = in.integer<uint64_t>();
		auto &native = terms.native_before;
		if (quest_mobile_native_reference_decode(
			    in.take(QUEST_MOBILE_NATIVE_REFERENCE_BYTES), &native.reference) !=
		    player_snapshot_codec_result::ok)
			return player_snapshot_codec_result::invalid_value;
		native.lineage = in.id();
		native.birth_epoch = in.id();
		native.wallet_mapping_id = in.integer<uint64_t>();
		native.cash_revision = in.integer<uint64_t>();
		for (auto &value : native.denominations)
			value = in.integer<int64_t>();
		auto &wallet = terms.player_wallet;
		wallet.wallet_mapping_id = in.integer<uint64_t>();
		wallet.before_revision = in.integer<uint64_t>();
		wallet.after_revision = in.integer<uint64_t>();
		for (auto &value : wallet.before)
			value = in.integer<int32_t>();
		for (auto &value : wallet.after)
			value = in.integer<int32_t>();
		terms.smith_catalog_index = in.integer<uint32_t>();
		terms.original_menu_choice = in.integer<uint32_t>();
		terms.forge_catalog_index = in.integer<uint32_t>();
		terms.ore_count = in.integer<uint32_t>();
		terms.fee = in.integer<uint32_t>();
		terms.first_ore_material = in.integer<int32_t>();
		const auto count = in.integer<uint32_t>();
		const auto roots = in.integer<uint32_t>();
		const auto input_bytes = in.integer<uint32_t>();
		const auto output_bytes = in.integer<uint32_t>();
		if (!in.good || in.offset != SMITH_NATIVE_COMPOUND_HEADER_BYTES || !count ||
		    count > ITEM_TRANSFER_MAX_ITEMS || roots < 1 || roots > 5 ||
		    roots != terms.ore_count || !input_bytes ||
		    input_bytes > ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES || !output_bytes ||
		    output_bytes > ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES ||
		    bytes.size() - in.offset != size_t(count) * ITEM_TRANSFER_ENTRY_BYTES +
							size_t(roots) * sizeof(uint64_t) +
							input_bytes + output_bytes)
			return player_snapshot_codec_result::invalid_value;
		terms.selected_custody.reserve(count);
		for (size_t i = 0; i < count; ++i)
		{
			item_transfer_entry entry{};
			entry.item_uid = in.integer<uint64_t>();
			entry.root_item_uid = in.integer<uint64_t>();
			entry.parent_item_uid = in.integer<uint64_t>();
			entry.expected_item_revision = in.integer<uint64_t>();
			entry.vnum = in.integer<int32_t>();
			entry.expected_state =
				static_cast<item_custody_state>(in.integer<uint8_t>());
			if (in.integer<uint8_t>() || in.integer<uint16_t>())
				return player_snapshot_codec_result::invalid_value;
			terms.selected_custody.push_back(entry);
		}
		terms.selected_root_order.reserve(roots);
		for (size_t i = 0; i < roots; ++i)
			terms.selected_root_order.push_back(in.integer<uint64_t>());
		const auto inputs = in.take(input_bytes);
		const auto outputs = in.take(output_bytes);
		if (!in.good || in.offset != bytes.size())
			return player_snapshot_codec_result::invalid_value;
		auto status = player_item_snapshot_list_decode(inputs.data(), inputs.size(),
							       &terms.selected_native_items);
		if (status != player_snapshot_codec_result::ok)
			return status;
		status = player_item_snapshot_list_decode(outputs.data(), outputs.size(),
							  &terms.frozen_outputs);
		if (status != player_snapshot_codec_result::ok)
			return status;
		if (!shape(terms))
			return player_snapshot_codec_result::invalid_value;
		*output = std::move(terms);
		return player_snapshot_codec_result::ok;
	}
	catch (...)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
}

#endif
