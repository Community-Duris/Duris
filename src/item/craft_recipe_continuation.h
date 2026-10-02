#ifndef CRAFT_RECIPE_CONTINUATION_H
#define CRAFT_RECIPE_CONTINUATION_H

#include "item/item_transfer_command.h"

#include <climits>
#include <new>
#include <span>
#include <vector>

enum class craft_recipe_discipline : uint32_t
{
	craft = 1,
	forge = 2,
};

struct craft_recipe_continuation
{
	uint32_t player_pid = 0;
	craft_recipe_discipline discipline = craft_recipe_discipline::craft;
	uint32_t experience = 0;
	uint32_t recipe_vnum = 0;
	uint64_t output_uid = 0;
	std::vector<uint8_t> pouch_mutation;
};

constexpr size_t CRAFT_RECIPE_CONTINUATION_HEADER_BYTES = 32;
constexpr size_t CRAFT_RECIPE_CONTINUATION_MAX_BYTES =
	CRAFT_RECIPE_CONTINUATION_HEADER_BYTES + ITEM_TRANSFER_POUCH_CONTINUATION_MAX_BYTES;

inline bool craft_recipe_continuation_encode(const craft_recipe_continuation &terms,
					     std::vector<uint8_t> *encoded)
{
	if (!encoded || !terms.player_pid || terms.player_pid > INT32_MAX ||
	    (terms.discipline != craft_recipe_discipline::craft &&
	     terms.discipline != craft_recipe_discipline::forge) ||
	    terms.experience > INT32_MAX || !terms.recipe_vnum || terms.recipe_vnum > INT32_MAX ||
	    !terms.output_uid ||
	    terms.pouch_mutation.size() > ITEM_TRANSFER_POUCH_CONTINUATION_MAX_BYTES)
		return false;
	try
	{
		std::vector<uint8_t> bytes;
		bytes.reserve(CRAFT_RECIPE_CONTINUATION_HEADER_BYTES + terms.pouch_mutation.size());
		auto number = [&](uint64_t value, size_t size)
		{
			for (size_t index = 0; index < size; ++index)
				bytes.push_back(static_cast<uint8_t>(value >> (8 * index)));
		};
		number(1, 4);
		number(terms.player_pid, 4);
		number(static_cast<uint32_t>(terms.discipline), 4);
		number(terms.experience, 4);
		number(terms.recipe_vnum, 4);
		number(terms.output_uid, 8);
		number(terms.pouch_mutation.size(), 4);
		bytes.insert(bytes.end(), terms.pouch_mutation.begin(), terms.pouch_mutation.end());
		*encoded = std::move(bytes);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

inline bool craft_recipe_continuation_decode(std::span<const uint8_t> bytes,
					     craft_recipe_continuation *terms)
{
	if (!terms || bytes.size() < CRAFT_RECIPE_CONTINUATION_HEADER_BYTES ||
	    bytes.size() > CRAFT_RECIPE_CONTINUATION_MAX_BYTES)
		return false;
	auto number = [&](size_t offset, size_t size)
	{
		uint64_t value = 0;
		for (size_t index = 0; index < size; ++index)
			value |= static_cast<uint64_t>(bytes[offset + index]) << (index * 8);
		return value;
	};
	if (number(0, 4) != 1 ||
	    number(28, 4) != bytes.size() - CRAFT_RECIPE_CONTINUATION_HEADER_BYTES)
		return false;
	try
	{
		craft_recipe_continuation candidate;
		candidate.player_pid = static_cast<uint32_t>(number(4, 4));
		candidate.discipline = static_cast<craft_recipe_discipline>(number(8, 4));
		candidate.experience = static_cast<uint32_t>(number(12, 4));
		candidate.recipe_vnum = static_cast<uint32_t>(number(16, 4));
		candidate.output_uid = number(20, 8);
		candidate.pouch_mutation.assign(
			bytes.begin() + CRAFT_RECIPE_CONTINUATION_HEADER_BYTES, bytes.end());
		std::vector<uint8_t> canonical;
		if (!craft_recipe_continuation_encode(candidate, &canonical))
			return false;
		*terms = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

inline bool craft_recipe_continuation_matches(const craft_recipe_continuation &terms,
					      const item_transfer_payload &payload)
{
	return payload.reason == item_transfer_reason::craft && payload.multi_root &&
	       payload.from_owner.type == item_owner_type::player &&
	       payload.from_owner.id == terms.player_pid && !payload.from_owner.context_id &&
	       payload.to_owner.type == item_owner_type::player &&
	       payload.to_owner.id == terms.player_pid && !payload.to_owner.context_id &&
	       payload.reason_id == terms.recipe_vnum &&
	       payload.selected_item_uid == terms.output_uid && payload.item_blob_size != 0;
}

#endif
