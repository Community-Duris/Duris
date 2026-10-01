#include "item/craft_pouch_mutation.h"
#include "item/craft_recipe_continuation.h"
#include "player/player_snapshot_codec.h"
#include "world/vnum.obj.h"

#include <algorithm>
#include <new>
#include <map>
#include <utility>

namespace
{
constexpr uint32_t version = 1;
constexpr size_t header_bytes = 20;
constexpr size_t usage_bytes = 12;

void append(std::vector<uint8_t> *out, uint64_t value, size_t size)
{
	for (size_t index = 0; index < size; ++index)
		out->push_back(static_cast<uint8_t>(value >> (index * 8)));
}

uint64_t number(std::span<const uint8_t> input, size_t offset, size_t size)
{
	uint64_t value = 0;
	for (size_t index = 0; index < size; ++index)
		value |= static_cast<uint64_t>(input[offset + index]) << (index * 8);
	return value;
}

bool valid_usage(const std::vector<chaos_material_pouch_usage> &usage)
{
	if (usage.empty() || usage.size() > CRAFT_POUCH_MUTATION_MAX_MATERIALS)
		return false;
	for (size_t index = 0; index < usage.size(); ++index)
		if (usage[index].vnum <= 0 || !usage[index].count ||
		    (index && usage[index - 1].vnum >= usage[index].vnum))
			return false;
	return true;
}
} // namespace

bool craft_pouch_mutation_encode(const craft_pouch_mutation &mutation,
				 std::vector<uint8_t> *encoded)
{
	if (!encoded || !valid_usage(mutation.usage) ||
	    mutation.before.parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
	    chaos_pouch_ledger_verify(mutation.before, mutation.after, mutation.usage,
				      mutation.mode) != chaos_pouch_ledger_result::ok)
		return false;
	try
	{
		std::vector<uint8_t> before, after;
		if (player_item_snapshot_list_encode({ mutation.before }, &before) !=
			    player_snapshot_codec_result::ok ||
		    player_item_snapshot_list_encode({ mutation.after }, &after) !=
			    player_snapshot_codec_result::ok ||
		    before.size() > CRAFT_POUCH_MUTATION_MAX_BYTES ||
		    after.size() > CRAFT_POUCH_MUTATION_MAX_BYTES)
			return false;
		const size_t total = header_bytes + mutation.usage.size() * usage_bytes +
				     before.size() + after.size();
		if (total > CRAFT_POUCH_MUTATION_MAX_BYTES)
			return false;
		std::vector<uint8_t> candidate;
		candidate.reserve(total);
		append(&candidate, version, 4);
		append(&candidate, static_cast<uint32_t>(mutation.mode), 4);
		append(&candidate, mutation.usage.size(), 4);
		append(&candidate, before.size(), 4);
		append(&candidate, after.size(), 4);
		for (const auto &used : mutation.usage)
		{
			append(&candidate, static_cast<uint32_t>(used.vnum), 4);
			append(&candidate, used.count, 8);
		}
		candidate.insert(candidate.end(), before.begin(), before.end());
		candidate.insert(candidate.end(), after.begin(), after.end());
		*encoded = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool craft_pouch_mutation_decode(std::span<const uint8_t> encoded, craft_pouch_mutation *mutation)
{
	if (!mutation || encoded.size() < header_bytes ||
	    encoded.size() > CRAFT_POUCH_MUTATION_MAX_BYTES || number(encoded, 0, 4) != version)
		return false;
	const uint64_t mode = number(encoded, 4, 4);
	const size_t count = number(encoded, 8, 4);
	const size_t before_size = number(encoded, 12, 4);
	const size_t after_size = number(encoded, 16, 4);
	if ((mode != static_cast<uint32_t>(chaos_pouch_usage_mode::generated) &&
	     mode != static_cast<uint32_t>(chaos_pouch_usage_mode::collected)) ||
	    !count || count > CRAFT_POUCH_MUTATION_MAX_MATERIALS ||
	    before_size > CRAFT_POUCH_MUTATION_MAX_BYTES ||
	    after_size > CRAFT_POUCH_MUTATION_MAX_BYTES ||
	    encoded.size() != header_bytes + count * usage_bytes + before_size + after_size)
		return false;
	try
	{
		craft_pouch_mutation candidate;
		candidate.mode = static_cast<chaos_pouch_usage_mode>(mode);
		candidate.usage.reserve(count);
		for (size_t index = 0; index < count; ++index)
		{
			const size_t offset = header_bytes + index * usage_bytes;
			const uint64_t vnum = number(encoded, offset, 4);
			if (vnum > INT32_MAX)
				return false;
			candidate.usage.push_back(
				{ static_cast<int>(vnum), number(encoded, offset + 4, 8) });
		}
		const size_t before_offset = header_bytes + count * usage_bytes;
		std::vector<player_item_snapshot> before, after;
		if (player_item_snapshot_list_decode(encoded.data() + before_offset, before_size,
						     &before) != player_snapshot_codec_result::ok ||
		    player_item_snapshot_list_decode(encoded.data() + before_offset + before_size,
						     after_size,
						     &after) != player_snapshot_codec_result::ok ||
		    before.size() != 1 || after.size() != 1)
			return false;
		candidate.before = std::move(before[0]);
		candidate.after = std::move(after[0]);
		std::vector<uint8_t> canonical;
		if (!craft_pouch_mutation_encode(candidate, &canonical) ||
		    !std::equal(canonical.begin(), canonical.end(), encoded.begin(), encoded.end()))
			return false;
		*mutation = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool craft_pouch_mutation_from_payload(const item_transfer_payload &payload,
				       craft_pouch_mutation *mutation)
{
	if (!mutation)
		return false;
	if (payload.continuation.kind == item_transfer_continuation_kind::craft_recipe)
	{
		craft_recipe_continuation recipe;
		if (!craft_recipe_continuation_decode(payload.continuation.data, &recipe) ||
		    !craft_recipe_continuation_matches(recipe, payload))
			return false;
		if (recipe.pouch_mutation.empty())
		{
			*mutation = {};
			return true;
		}
		try
		{
			item_transfer_payload pouch_payload = payload;
			pouch_payload.continuation.kind =
				item_transfer_continuation_kind::craft_pouch_usage;
			pouch_payload.continuation.data = std::move(recipe.pouch_mutation);
			return craft_pouch_mutation_from_payload(pouch_payload, mutation);
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
	}
	if (payload.continuation.kind == item_transfer_continuation_kind::none)
	{
		if (!payload.continuation.data.empty())
			return false;
		*mutation = {};
		return true;
	}
	if (payload.continuation.kind != item_transfer_continuation_kind::craft_pouch_usage ||
	    payload.reason != item_transfer_reason::craft || !payload.multi_root ||
	    !payload.from_owner.id || payload.from_owner.type != item_owner_type::player ||
	    payload.from_owner.context_id || payload.from_owner.type != payload.to_owner.type ||
	    payload.from_owner.id != payload.to_owner.id || payload.to_owner.context_id ||
	    payload.item_count < 2 || payload.item_count > ITEM_TRANSFER_MAX_ITEMS)
		return false;
	try
	{
		craft_pouch_mutation candidate;
		if (!craft_pouch_mutation_decode(payload.continuation.data, &candidate))
			return false;
		size_t retained_count = 0;
		std::map<int, uint64_t> consumed;
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const auto &entry = payload.items[index];
			if (entry.expected_state != item_custody_state::active ||
			    entry.expected_item_revision == UINT64_MAX)
				return false;
			if (entry.item_uid == candidate.before.object_uid)
			{
				if (entry.vnum != VOBJ_CHAOS_CRAFT_POUCH)
					return false;
				++retained_count;
			}
			else
			{
				if (entry.vnum == VOBJ_CHAOS_CRAFT_POUCH ||
				    entry.root_item_uid == candidate.before.object_uid ||
				    entry.parent_item_uid == candidate.before.object_uid)
					return false;
				++consumed[entry.vnum];
			}
		}
		if (retained_count != 1 || payload.selected_item_uid == candidate.before.object_uid)
			return false;
		if (candidate.mode == chaos_pouch_usage_mode::collected)
		{
			if (payload.item_blob_size || payload.reason_id != VOBJ_CHAOS_CRAFT_POUCH ||
			    consumed.size() != candidate.usage.size())
				return false;
			for (const auto &used : candidate.usage)
				if (!consumed.contains(used.vnum) ||
				    consumed.at(used.vnum) != used.count)
					return false;
		}
		*mutation = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
