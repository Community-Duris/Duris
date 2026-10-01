#include "combat/chaos_pouch_ledger.h"

#include "core/structs.h"
#include "core/utils.h"
#include "world/vnum.obj.h"
#include "economy/tradeskill.h"
#include "player/player_snapshot_codec.h"

#include <array>
#include <charconv>
#include <limits>
#include <new>
#include <string_view>
#include <utility>

namespace
{
constexpr std::string_view ledger_prefix = "CHAOS_POUCH_LEDGER_";
constexpr size_t salvage_count = HIGHEST_MAT_VNUM - LOWEST_MAT_VNUM + 1;
constexpr size_t material_count = salvage_count + ENCRUST_VNUM_END - ENCRUST_VNUM_BEGIN + 1;

struct score
{
	uint64_t generated = 0;
	uint64_t collected = 0;
	bool operator==(const score &) const = default;
};

using scores = std::array<score, material_count>;

bool read_number(std::string_view *remaining, char delimiter, uint64_t *value)
{
	if (!remaining || !value || remaining->empty())
		return false;
	const char *begin = remaining->data();
	const char *end = begin + remaining->size();
	const auto parsed = std::from_chars(begin, end, *value);
	if (parsed.ec != std::errc() || parsed.ptr == end || *parsed.ptr != delimiter)
		return false;
	remaining->remove_prefix(static_cast<size_t>(parsed.ptr - begin) + 1);
	return true;
}

bool read_scores(const player_item_snapshot &item, scores *result)
{
	*result = {};
	std::array<const player_item_extra_description_snapshot *,
		   CHAOS_MATERIAL_POUCH_LEDGER_MAX_CHUNKS>
		chunks = {};
	size_t chunk_count = 0;
	for (const auto &description : item.extra_descriptions)
	{
		std::string_view key(description.keyword);
		if (!key.starts_with(ledger_prefix))
			continue;
		key.remove_prefix(ledger_prefix.size());
		if (key.empty() || (key.size() > 1 && key.front() == '0'))
			return false;
		uint64_t index = 0;
		const auto parsed = std::from_chars(key.data(), key.data() + key.size(), index);
		if (parsed.ec != std::errc() || parsed.ptr != key.data() + key.size() ||
		    index >= chunks.size() || chunks[index] || description.spellbook ||
		    !description.spell_ids.empty() ||
		    description.description.size() > CHAOS_MATERIAL_POUCH_LEDGER_CHUNK_BYTES)
			return false;
		chunks[index] = &description;
		++chunk_count;
	}
	std::array<bool, material_count> seen = {};
	for (size_t chunk = 0; chunk < chunk_count; ++chunk)
	{
		if (!chunks[chunk])
			return false;
		std::string_view remaining(chunks[chunk]->description);
		while (!remaining.empty())
		{
			uint64_t index = 0, generated = 0, collected = 0;
			if (!read_number(&remaining, ':', &index) ||
			    !read_number(&remaining, ':', &generated) ||
			    !read_number(&remaining, ';', &collected) || index >= result->size() ||
			    seen[index])
				return false;
			seen[index] = true;
			(*result)[index] = { generated, collected };
		}
	}
	return true;
}

bool material_index(int vnum, size_t *index)
{
	if (vnum >= LOWEST_MAT_VNUM && vnum <= HIGHEST_MAT_VNUM)
		*index = static_cast<size_t>(vnum - LOWEST_MAT_VNUM);
	else if (vnum >= ENCRUST_VNUM_BEGIN && vnum <= ENCRUST_VNUM_END)
		*index = salvage_count + static_cast<size_t>(vnum - ENCRUST_VNUM_BEGIN);
	else
		return false;
	return true;
}

bool write_scores(const scores &values, player_item_snapshot *item)
{
	std::vector<player_item_extra_description_snapshot> descriptions;
	for (const auto &description : item->extra_descriptions)
		if (!std::string_view(description.keyword).starts_with(ledger_prefix))
			descriptions.push_back(description);
	std::vector<std::string> chunks;
	std::string current;
	for (size_t index = 0; index < values.size(); ++index)
	{
		if (!values[index].generated && !values[index].collected)
			continue;
		const std::string record = std::to_string(index) + ":" +
					   std::to_string(values[index].generated) + ":" +
					   std::to_string(values[index].collected) + ";";
		if (current.size() + record.size() > CHAOS_MATERIAL_POUCH_LEDGER_CHUNK_BYTES)
		{
			chunks.push_back(std::move(current));
			current.clear();
		}
		current += record;
	}
	if (!current.empty())
		chunks.push_back(std::move(current));
	if (chunks.size() > CHAOS_MATERIAL_POUCH_LEDGER_MAX_CHUNKS)
		return false;
	for (size_t index = 0; index < chunks.size(); ++index)
		descriptions.push_back({ std::string(ledger_prefix) + std::to_string(index),
					 std::move(chunks[index]),
					 false,
					 {} });
	item->extra_descriptions = std::move(descriptions);
	item->string_mask |= STRUNG_EDESC;
	return true;
}
} // namespace

chaos_pouch_ledger_result
chaos_pouch_ledger_prepare(const player_item_snapshot &before,
			   std::span<const chaos_material_pouch_usage> usage,
			   chaos_pouch_usage_mode mode, player_item_snapshot *after)
{
	using result = chaos_pouch_ledger_result;
	if (!after || !before.object_uid || before.vnum != VOBJ_CHAOS_CRAFT_POUCH ||
	    usage.empty() || usage.size() > material_count ||
	    (mode != chaos_pouch_usage_mode::generated &&
	     mode != chaos_pouch_usage_mode::collected))
		return result::invalid;
	try
	{
		std::vector<uint8_t> encoded;
		if (player_item_snapshot_list_encode({ before }, &encoded) !=
		    player_snapshot_codec_result::ok)
			return result::invalid;
		scores values;
		if (!read_scores(before, &values))
			return result::invalid;
		for (const auto &used : usage)
		{
			size_t index = 0;
			if (!used.count || !material_index(used.vnum, &index))
				return result::invalid;
			uint64_t &count = mode == chaos_pouch_usage_mode::generated ?
						  values[index].generated :
						  values[index].collected;
			if (count > std::numeric_limits<uint64_t>::max() - used.count)
				return result::overflow;
			count += used.count;
		}
		player_item_snapshot candidate = before;
		if (!write_scores(values, &candidate) ||
		    player_item_snapshot_list_encode({ candidate }, &encoded) !=
			    player_snapshot_codec_result::ok)
			return result::capacity;
		*after = std::move(candidate);
		return result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return result::capacity;
	}
}

chaos_pouch_ledger_result
chaos_pouch_ledger_verify(const player_item_snapshot &before, const player_item_snapshot &after,
			  std::span<const chaos_material_pouch_usage> usage,
			  chaos_pouch_usage_mode mode)
{
	player_item_snapshot expected;
	const auto prepared = chaos_pouch_ledger_prepare(before, usage, mode, &expected);
	if (prepared != chaos_pouch_ledger_result::ok)
		return prepared;
	try
	{
		std::vector<uint8_t> expected_bytes, actual_bytes;
		if (player_item_snapshot_list_encode({ expected }, &expected_bytes) !=
			    player_snapshot_codec_result::ok ||
		    player_item_snapshot_list_encode({ after }, &actual_bytes) !=
			    player_snapshot_codec_result::ok ||
		    expected_bytes != actual_bytes)
			return chaos_pouch_ledger_result::invalid;
		return chaos_pouch_ledger_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return chaos_pouch_ledger_result::capacity;
	}
}

chaos_pouch_ledger_result chaos_pouch_ledger_apply_native(
	const player_item_snapshot &frozen_before, const player_item_snapshot &frozen_after,
	std::span<const chaos_material_pouch_usage> usage, chaos_pouch_usage_mode mode,
	const player_item_snapshot &native_before, player_item_snapshot *native_after)
{
	const auto verified = chaos_pouch_ledger_verify(frozen_before, frozen_after, usage, mode);
	if (verified != chaos_pouch_ledger_result::ok)
		return verified;
	if (!native_after || native_before.object_uid != frozen_before.object_uid ||
	    native_before.vnum != frozen_before.vnum)
		return chaos_pouch_ledger_result::invalid;
	scores captured, locked;
	if (!read_scores(frozen_before, &captured) || !read_scores(native_before, &locked) ||
	    captured != locked)
		return chaos_pouch_ledger_result::invalid;
	return chaos_pouch_ledger_prepare(native_before, usage, mode, native_after);
}

chaos_pouch_ledger_result chaos_pouch_ledger_overlay(const player_item_snapshot &ledger_source,
						     const player_item_snapshot &current,
						     player_item_snapshot *overlaid)
{
	using result = chaos_pouch_ledger_result;
	if (!overlaid || !current.object_uid || current.object_uid != ledger_source.object_uid ||
	    current.vnum != VOBJ_CHAOS_CRAFT_POUCH || current.vnum != ledger_source.vnum)
		return result::invalid;
	scores values, existing;
	if (!read_scores(ledger_source, &values) || !read_scores(current, &existing))
		return result::invalid;
	try
	{
		auto candidate = current;
		if (!write_scores(values, &candidate))
			return result::capacity;
		// Validate as a standalone snapshot but retain the caller's graph index.
		auto standalone = candidate;
		standalone.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		std::vector<uint8_t> encoded;
		if (player_item_snapshot_list_encode({ standalone }, &encoded) !=
		    player_snapshot_codec_result::ok)
			return result::invalid;
		*overlaid = std::move(candidate);
		return result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return result::capacity;
	}
}

bool chaos_pouch_ledger_counters_equal(const player_item_snapshot &left,
				       const player_item_snapshot &right)
{
	scores left_scores, right_scores;
	return left.object_uid && left.object_uid == right.object_uid &&
	       left.vnum == VOBJ_CHAOS_CRAFT_POUCH && left.vnum == right.vnum &&
	       read_scores(left, &left_scores) && read_scores(right, &right_scores) &&
	       left_scores == right_scores;
}
