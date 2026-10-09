#ifndef CRAFT_RECIPE_CONTINUATION_H
#define CRAFT_RECIPE_CONTINUATION_H

#include "item/item_transfer_command.h"
#include "guild/skill_notch.h"

#include <climits>
#include <algorithm>
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
	// Version 3 only. Refinement awards neither XP nor a skill notch and
	// creates no player_craft_progression obligation.
	refine = 7,
};

struct craft_refine_wallet_cost
{
	uint64_t wallet_mapping_id = 0;
	uint64_t before_revision = 0, after_revision = 0;
	std::array<int32_t, 4> before = {}, after = {};
	bool operator==(const craft_refine_wallet_cost &) const = default;
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
	uint32_t refine_material_vnum = 0, refine_roll = 0, refine_ore_count = 0;
	uint32_t refine_last_ore_vnum = 0;
	craft_refine_wallet_cost refine_cost;
	// Original inventory order, before extraction; payload entries remain sorted.
	std::vector<uint64_t> refine_root_order;
	std::string refine_material_name;
	// Version 4 ordinary Craft/Forge: actual native skill BEFORE and frozen notch.
	bool frozen_progression = false;
	uint32_t progression_skill_before = 0;
};

inline bool craft_refine_succeeded(const craft_recipe_continuation &terms)
{
	return terms.refine_roll <=
	       61 + (terms.refine_ore_count != 1 ? 10 : terms.refine_last_ore_vnum - 194);
}

inline bool craft_refine_cost_valid(const craft_refine_wallet_cost &cost)
{
	if (!cost.wallet_mapping_id || cost.before_revision == UINT64_MAX ||
	    cost.after_revision != cost.before_revision + 1)
		return false;
	constexpr std::array<int64_t, 4> units = { 1, 10, 100, 1000 };
	int64_t value = 0;
	for (size_t i = 0; i < 4; ++i)
	{
		if (cost.before[i] < 0 || cost.after[i] < 0)
			return false;
		value += int64_t(cost.before[i]) * units[i];
	}
	if (value < 50000)
		return false;
	value -= 50000;
	// The existing PC SUB_MONEY payment normalizes the remaining wallet.
	for (size_t i = 4; i-- > 0;)
	{
		if (value / units[i] > INT32_MAX || cost.after[i] != value / units[i])
			return false;
		value %= units[i];
	}
	return true;
}

inline bool craft_refine_terms_valid(const craft_recipe_continuation &terms)
{
	if (terms.discipline != craft_recipe_discipline::refine || terms.frozen_progression ||
	    terms.progression_skill_before || !terms.player_pid || terms.player_pid > INT32_MAX ||
	    terms.experience || !terms.output_uid || !terms.pouch_mutation.empty() ||
	    terms.notch.learned_before || terms.notch.learned_after || terms.notch.timer_tag ||
	    terms.notch.timer_duration || terms.refine_material_vnum < 400000 ||
	    terms.refine_material_vnum > 400208 ||
	    terms.recipe_vnum != terms.refine_material_vnum + 1 || !terms.refine_roll ||
	    terms.refine_roll > 100 || terms.refine_ore_count > ITEM_TRANSFER_MAX_ITEMS - 2 ||
	    terms.refine_root_order.size() != terms.refine_ore_count + 2 ||
	    terms.refine_material_name.empty() || terms.refine_material_name.size() >= 512 ||
	    terms.refine_material_name.find('\0') != std::string::npos ||
	    (terms.refine_ore_count ?
		     terms.refine_last_ore_vnum < 194 || terms.refine_last_ore_vnum > 233 :
		     terms.refine_last_ore_vnum != 0) ||
	    terms.output_count != (craft_refine_succeeded(terms) ? 1u : 0u) ||
	    (terms.refine_ore_count == 1 ? terms.refine_cost != craft_refine_wallet_cost{} :
					   !craft_refine_cost_valid(terms.refine_cost)))
		return false;
	for (size_t i = 0; i < terms.refine_root_order.size(); ++i)
	{
		if (!terms.refine_root_order[i])
			return false;
		for (size_t j = 0; j < i; ++j)
			if (terms.refine_root_order[i] == terms.refine_root_order[j])
				return false;
	}
	return true;
}

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
	if (terms.discipline == craft_recipe_discipline::refine)
	{
		if (!encoded || !craft_refine_terms_valid(terms))
			return false;
		try
		{
			std::vector<uint8_t> bytes(128, 0);
			auto put = [&](size_t offset, uint64_t value, size_t width)
			{
				for (size_t i = 0; i < width; ++i)
					bytes[offset + i] = static_cast<uint8_t>(value >> (8 * i));
			};
			put(0, 3, 4);
			put(4, terms.player_pid, 4);
			put(8, 7, 4);
			put(16, terms.recipe_vnum, 4);
			put(20, terms.output_uid, 8);
			put(32, terms.output_count, 4);
			put(36, terms.refine_material_vnum, 4);
			put(40, terms.refine_roll, 4);
			put(44, terms.refine_ore_count, 4);
			put(48, terms.refine_last_ore_vnum, 4);
			put(52, terms.refine_root_order.size(), 4);
			put(56, terms.refine_cost.wallet_mapping_id, 8);
			put(64, terms.refine_cost.before_revision, 8);
			put(72, terms.refine_cost.after_revision, 8);
			for (size_t i = 0; i < 4; ++i)
			{
				put(80 + 4 * i, static_cast<uint32_t>(terms.refine_cost.before[i]),
				    4);
				put(96 + 4 * i, static_cast<uint32_t>(terms.refine_cost.after[i]),
				    4);
			}
			put(112, terms.refine_material_name.size(), 4);
			for (uint64_t uid : terms.refine_root_order)
				for (size_t i = 0; i < 8; ++i)
					bytes.push_back(static_cast<uint8_t>(uid >> (8 * i)));
			bytes.insert(bytes.end(), terms.refine_material_name.begin(),
				     terms.refine_material_name.end());
			if (bytes.size() > CRAFT_RECIPE_CONTINUATION_MAX_BYTES)
				return false;
			*encoded = std::move(bytes);
			return true;
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
	}
	if (terms.refine_material_vnum || terms.refine_roll || terms.refine_ore_count ||
	    terms.refine_last_ore_vnum || terms.refine_cost != craft_refine_wallet_cost{} ||
	    !terms.refine_root_order.empty() || !terms.refine_material_name.empty())
		return false;
	if ((terms.frozen_progression ?
		     (terms.discipline != craft_recipe_discipline::craft &&
		      terms.discipline != craft_recipe_discipline::forge) ||
			     terms.output_count != 1 || terms.progression_skill_before > 255 ||
			     (terms.notch.learned_before != terms.progression_skill_before &&
			      (terms.notch.learned_before || terms.notch.learned_after ||
			       terms.notch.timer_tag || terms.notch.timer_duration)) :
		     terms.progression_skill_before != 0))
		return false;
	if (!encoded || !terms.player_pid || terms.player_pid > INT32_MAX ||
	    (terms.discipline != craft_recipe_discipline::craft &&
	     terms.discipline != craft_recipe_discipline::forge &&
	     !craft_recipe_is_alchemy(terms.discipline)) ||
	    terms.experience > INT32_MAX || !terms.recipe_vnum || terms.recipe_vnum > INT32_MAX ||
	    !terms.output_uid || terms.notch.learned_before > 255 ||
	    terms.notch.learned_after > 255 || terms.notch.timer_tag > INT32_MAX ||
	    terms.notch.timer_duration > INT32_MAX ||
	    (!terms.notch.timer_tag && terms.notch.timer_duration) ||
	    (terms.discipline != craft_recipe_discipline::poison && !terms.frozen_progression &&
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
		number(terms.frozen_progression ? 4 : alchemy ? 2 : 1, 4);
		number(terms.player_pid, 4);
		number(static_cast<uint32_t>(terms.discipline), 4);
		number(terms.experience, 4);
		number(terms.recipe_vnum, 4);
		number(terms.output_uid, 8);
		number(terms.pouch_mutation.size(), 4);
		if (alchemy || terms.frozen_progression)
		{
			number(terms.frozen_progression ? terms.progression_skill_before :
							  terms.output_count,
			       4);
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
	if (version == 3)
	{
		if (bytes.size() < 128 || number(8, 4) != 7 || number(12, 4) || number(28, 4) ||
		    number(52, 4) > ITEM_TRANSFER_MAX_ITEMS || number(112, 4) >= 512 ||
		    bytes.size() != 128 + number(52, 4) * 8 + number(112, 4) || number(116, 4) ||
		    number(120, 8))
			return false;
		try
		{
			craft_recipe_continuation candidate;
			candidate.player_pid = static_cast<uint32_t>(number(4, 4));
			candidate.discipline = craft_recipe_discipline::refine;
			candidate.recipe_vnum = static_cast<uint32_t>(number(16, 4));
			candidate.output_uid = number(20, 8);
			candidate.output_count = static_cast<uint32_t>(number(32, 4));
			candidate.refine_material_vnum = static_cast<uint32_t>(number(36, 4));
			candidate.refine_roll = static_cast<uint32_t>(number(40, 4));
			candidate.refine_ore_count = static_cast<uint32_t>(number(44, 4));
			candidate.refine_last_ore_vnum = static_cast<uint32_t>(number(48, 4));
			candidate.refine_cost.wallet_mapping_id = number(56, 8);
			candidate.refine_cost.before_revision = number(64, 8);
			candidate.refine_cost.after_revision = number(72, 8);
			for (size_t i = 0; i < 4; ++i)
			{
				const auto before = number(80 + 4 * i, 4),
					   after = number(96 + 4 * i, 4);
				if (before > INT32_MAX || after > INT32_MAX)
					return false;
				candidate.refine_cost.before[i] = static_cast<int32_t>(before);
				candidate.refine_cost.after[i] = static_cast<int32_t>(after);
			}
			for (size_t i = 0; i < number(52, 4); ++i)
				candidate.refine_root_order.push_back(number(128 + 8 * i, 8));
			candidate.refine_material_name.assign(
				bytes.begin() + 128 + number(52, 4) * 8, bytes.end());
			std::vector<uint8_t> canonical;
			if (!craft_recipe_continuation_encode(candidate, &canonical) ||
			    !std::equal(canonical.begin(), canonical.end(), bytes.begin(),
					bytes.end()))
				return false;
			*terms = std::move(candidate);
			return true;
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
	}
	const size_t header =
		CRAFT_RECIPE_CONTINUATION_HEADER_BYTES + (version == 2 || version == 4 ? 20 : 0);
	if ((version != 1 && version != 2 && version != 4) || bytes.size() < header ||
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
		if (version == 2 || version == 4)
		{
			candidate.frozen_progression = version == 4;
			candidate.output_count =
				version == 4 ? 1 : static_cast<uint32_t>(number(32, 4));
			candidate.progression_skill_before =
				version == 4 ? static_cast<uint32_t>(number(32, 4)) : 0;
			candidate.notch = { static_cast<uint32_t>(number(36, 4)),
					    static_cast<uint32_t>(number(40, 4)),
					    static_cast<uint32_t>(number(44, 4)),
					    static_cast<uint32_t>(number(48, 4)) };
		}
		if ((version == 2) != craft_recipe_is_alchemy(candidate.discipline))
			return false;
		candidate.pouch_mutation.assign(bytes.begin() + header, bytes.end());
		std::vector<uint8_t> canonical;
		if (!craft_recipe_continuation_encode(candidate, &canonical) ||
		    (version == 4 &&
		     !std::equal(canonical.begin(), canonical.end(), bytes.begin(), bytes.end())))
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
	if (terms.discipline == craft_recipe_discipline::refine)
	{
		if (!craft_refine_terms_valid(terms) ||
		    payload.reason != item_transfer_reason::craft || !payload.multi_root ||
		    payload.from_owner.type != item_owner_type::player ||
		    payload.from_owner.id != terms.player_pid || payload.from_owner.context_id ||
		    !item_owner_identity_equal(payload.from_owner, payload.to_owner) ||
		    payload.reason_id != terms.recipe_vnum ||
		    payload.selected_item_uid != terms.output_uid ||
		    (terms.output_count ? !payload.item_blob_size : payload.item_blob_size != 0))
			return false;
		size_t materials = 0, ores = 0, roots = 0;
		uint32_t last_ore = 0;
		for (uint64_t uid : terms.refine_root_order)
		{
			const item_transfer_entry *entry = nullptr;
			for (size_t i = 0; i < payload.item_count; ++i)
				if (payload.items[i].item_uid == uid)
					entry = &payload.items[i];
			if (!entry || entry->root_item_uid != uid || entry->parent_item_uid)
				return false;
			if (entry->vnum == static_cast<int32_t>(terms.refine_material_vnum))
				++materials;
			else if (entry->vnum >= 194 && entry->vnum <= 233)
			{
				++ores;
				last_ore = static_cast<uint32_t>(entry->vnum);
			}
			else
				return false;
		}
		for (size_t i = 0; i < payload.item_count; ++i)
			if (!payload.items[i].parent_item_uid)
				++roots;
		return materials == 2 && ores == terms.refine_ore_count &&
		       last_ore == terms.refine_last_ore_vnum &&
		       roots == terms.refine_root_order.size();
	}
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

inline bool craft_refine_from_payload(const item_transfer_payload &payload,
				      craft_recipe_continuation *terms)
{
	return terms &&
	       payload.continuation.kind == item_transfer_continuation_kind::craft_recipe &&
	       craft_recipe_continuation_decode(payload.continuation.data, terms) &&
	       terms->discipline == craft_recipe_discipline::refine &&
	       craft_recipe_continuation_matches(*terms, payload);
}

#endif
