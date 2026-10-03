#ifndef CRAFT_RECIPE_CONTINUATION_H
#define CRAFT_RECIPE_CONTINUATION_H

#include "item/item_transfer_command.h"
#include "guild/skill_notch.h"

#include <climits>
#include <new>
#include <span>
#include <vector>

enum class craft_recipe_discipline : uint32_t
{
	craft = 1,
	forge = 2,
	poison = 3,
	encrust = 4,
	encrust_failure = 5,
	harvester = 6,
};

struct craft_recipe_continuation
{
	uint32_t player_pid = 0;
	craft_recipe_discipline discipline = craft_recipe_discipline::craft;
	uint32_t experience = 0;
	uint32_t recipe_vnum = 0;
	uint64_t output_uid = 0;
	std::vector<uint8_t> pouch_mutation;
	uint32_t output_count = 0;
	skill_notch_outcome notch = {};
};

inline bool craft_recipe_is_alchemy(craft_recipe_discipline discipline)
{
	return discipline >= craft_recipe_discipline::poison &&
	       discipline <= craft_recipe_discipline::harvester;
}

constexpr size_t CRAFT_RECIPE_CONTINUATION_HEADER_BYTES = 32;
constexpr size_t CRAFT_RECIPE_CONTINUATION_MAX_BYTES =
	CRAFT_RECIPE_CONTINUATION_HEADER_BYTES + 20 + ITEM_TRANSFER_POUCH_CONTINUATION_MAX_BYTES;

inline bool craft_recipe_continuation_encode(const craft_recipe_continuation &terms,
					     std::vector<uint8_t> *encoded)
{
	if (!encoded || !terms.player_pid || terms.player_pid > INT32_MAX ||
	    (terms.discipline != craft_recipe_discipline::craft &&
	     terms.discipline != craft_recipe_discipline::forge &&
	     !craft_recipe_is_alchemy(terms.discipline)) ||
	    terms.experience > INT32_MAX || !terms.recipe_vnum || terms.recipe_vnum > INT32_MAX ||
	    !terms.output_uid || terms.notch.learned_before > 255 ||
	    terms.notch.learned_after > 255 || terms.notch.timer_tag > INT32_MAX ||
	    terms.notch.timer_duration > INT32_MAX ||
	    (!terms.notch.timer_tag && terms.notch.timer_duration) ||
	    (terms.discipline != craft_recipe_discipline::poison &&
	     (terms.notch.learned_before || terms.notch.learned_after || terms.notch.timer_tag ||
	      terms.notch.timer_duration)) ||
	    (craft_recipe_is_alchemy(terms.discipline) &&
	     (terms.experience ||
	      (terms.discipline == craft_recipe_discipline::poison ?
		       !terms.output_count || terms.output_count > 64 :
		       terms.output_count !=
			       (terms.discipline == craft_recipe_discipline::encrust_failure ?
					0u :
					1u)))) ||
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
		const bool alchemy = craft_recipe_is_alchemy(terms.discipline);
		number(alchemy ? 2 : 1, 4);
		number(terms.player_pid, 4);
		number(static_cast<uint32_t>(terms.discipline), 4);
		number(terms.experience, 4);
		number(terms.recipe_vnum, 4);
		number(terms.output_uid, 8);
		number(terms.pouch_mutation.size(), 4);
		if (alchemy)
		{
			number(terms.output_count, 4);
			number(terms.notch.learned_before, 4);
			number(terms.notch.learned_after, 4);
			number(terms.notch.timer_tag, 4);
			number(terms.notch.timer_duration, 4);
		}
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
	const uint64_t version = number(0, 4);
	const size_t header = CRAFT_RECIPE_CONTINUATION_HEADER_BYTES + (version == 2 ? 20 : 0);
	if ((version != 1 && version != 2) || bytes.size() < header ||
	    number(28, 4) != bytes.size() - header)
		return false;
	try
	{
		craft_recipe_continuation candidate;
		candidate.player_pid = static_cast<uint32_t>(number(4, 4));
		candidate.discipline = static_cast<craft_recipe_discipline>(number(8, 4));
		candidate.experience = static_cast<uint32_t>(number(12, 4));
		candidate.recipe_vnum = static_cast<uint32_t>(number(16, 4));
		candidate.output_uid = number(20, 8);
		if (version == 2)
		{
			candidate.output_count = static_cast<uint32_t>(number(32, 4));
			candidate.notch = { static_cast<uint32_t>(number(36, 4)),
					    static_cast<uint32_t>(number(40, 4)),
					    static_cast<uint32_t>(number(44, 4)),
					    static_cast<uint32_t>(number(48, 4)) };
		}
		if ((version == 2) != craft_recipe_is_alchemy(candidate.discipline))
			return false;
		candidate.pouch_mutation.assign(bytes.begin() + header, bytes.end());
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
	       payload.selected_item_uid == terms.output_uid &&
	       (terms.discipline == craft_recipe_discipline::encrust_failure ?
			payload.item_blob_size == 0 :
			payload.item_blob_size != 0);
}

#endif
