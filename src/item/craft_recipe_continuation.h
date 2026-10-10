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

namespace duris_craft_recipe_bounded_detail
{
using reserve_fn = bool (*)(size_t, void *) noexcept;
inline bool add(size_t &bytes, size_t extra) noexcept
{
	if (extra > SIZE_MAX - bytes)
		return false;
	bytes += extra;
	return true;
}
constexpr size_t allocator_frames =
	// _M_allocate, allocator_traits::allocate, allocator::allocate (C++20):
	// each this/allocator reference, n and returned pointer; new_allocator
	// adds its genuine hint pointer; operator new n and returned pointer.
	3 * (2 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	sizeof(void *) + sizeof(size_t) +
	// _M_deallocate/traits/allocator/new_allocator: allocator/this+p+n,
	// then sized operator delete p+n. Trivial element _Destroy closures.
	4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t) +
	(3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *)) +
	// vector max_size/_S_max_size/traits max_size/new_allocator::_M_max_size
	// references/results and actual diffmax/allocmax locals. C++20 allocator
	// has no max_size member; that inactive C++17 branch is not counted.
	4 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(size_t) +
	// traits::construct -> construct_at -> forward -> placement-new; all
	// constructor arguments here are real references to trivial values.
	3 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t);
constexpr size_t copy_frames =
	// __uninitialized_move_if_noexcept_a and __uninitialized_copy_a: 3
	// iterators+allocator-reference+returned iterator each. Runtime ordinary
	// uninitialized_copy's two boolean locals and __uninit_copy carrier.
	2 * (4 * sizeof(void *) + sizeof(void *)) + 3 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(bool) + 3 * sizeof(void *) + sizeof(void *) +
	// copy/copy_move_a/a1/a2/copy_m, each3 iterator params+return; real
	// miter/niter/wrap/assign_one and memmove argument/result scopes.
	5 * (3 * sizeof(void *) + sizeof(void *)) + 2 * (sizeof(void *) + sizeof(void *)) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) +
	// distance/__distance and normal-iterator subtraction/base/dereference/
	// ++/comparison/constructor source parameter/return scopes.
	2 * (2 * sizeof(void *) + sizeof(std::ptrdiff_t)) + sizeof(char) +
	6 * (2 * sizeof(void *)) + sizeof(std::ptrdiff_t) + sizeof(bool) +
	// Fitting forward insert reaches advance(__mid,__elems_after), even zero.
	// advance: iterator-reference, size_t n, real local difference_type __d;
	// __iterator_category: iterator-reference and actual returned RA tag;
	// __advance: iterator-reference, difference n and by-value RA tag;
	// actual += this/n/reference-return, plus source ++/-- alternatives.
	sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + sizeof(void *) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 2 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	4 * sizeof(void *);
constexpr size_t relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t default_frames =
	// Runtime default_n_a/default_n/default_n_1<true>: real first/n/allocator
	// reference, can_fill and val locals, actual returned pointer carriers.
	(3 * sizeof(void *) + sizeof(size_t)) +
	(2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	(3 * sizeof(void *) + sizeof(size_t)) +
	// _Construct's real location plus placement-new n/location/result.
	sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	// fill_n/__fill_n_a<random_access>: first/n/value/result/tag;
	// __size_to_integer argument/result; __fill_a/__fill_a1 scalar __tmp.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(char) + 2 * sizeof(size_t) +
	2 * (3 * sizeof(void *)) + sizeof(uint64_t);
constexpr size_t vector_frames =
	allocator_frames + copy_frames + relocate_frames + default_frames +
	// reserve this/n/old_size/tmp; assign public/forward-aux and exact
	// _M_allocate_and_copy's this/n/first/last/result/returned pointer.
	2 * sizeof(void *) + 2 * sizeof(size_t) + 7 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(char) + 5 * sizeof(void *) + sizeof(size_t) +
	// push_back/emplace_back and real realloc_insert old/new start/finish,
	// len/elems_before/position/forward value reference; _M_check_len.
	2 * sizeof(void *) + 3 * sizeof(void *) + 7 * sizeof(void *) + 2 * sizeof(size_t) +
	2 * sizeof(void *) + 3 * sizeof(size_t) +
	// C++20 forward insert public/range-insert (no old dispatch), offset/elems_after/
	// len/old-start/finish/mid/new-start/finish/iterator return/tag scopes.
	15 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(char) +
	// default_append's n/size/navail/len and real old/new/destroy pointers.
	5 * sizeof(void *) + 4 * sizeof(size_t) +
	// begin/end/cbegin/size/capacity/get-allocator declared carriers and
	// iterator-category/std::max arguments/results on the real call paths.
	7 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(char) + 3 * sizeof(void *);
constexpr size_t move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + allocator_frames;

// Genuine default vector/base/impl/data and allocator/new_allocator this
// carriers. The vector object's fields are already included in its owner.
constexpr size_t empty_constructor_frames = 6 * sizeof(void *);
// vector(n,a), actual default allocator temporary, _S_check_init_len n/a/return
// and allocator copy, _Vector_base this/n/a, _Vector_impl this/a/copy,
// _Vector_impl_data this and _M_create_storage this/n. The existing vector
// allocator profile owns _S_max_size and allocation scopes.
constexpr size_t size_constructor_frames =
	2 * sizeof(void *) + sizeof(size_t) + sizeof(std::allocator<uint8_t>) + sizeof(void *) +
	2 * sizeof(size_t) + sizeof(std::allocator<uint8_t>) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) +
	sizeof(size_t);

// Fitting _M_replace calls _M_disjunct(this,s). Both actual less pointer
// temporaries can coexist through the full || expression; their operator()
// has this/x/y/result and is_constant_evaluated result. Data/size queries.
constexpr size_t disjunct_frames = 2 * sizeof(void *) + sizeof(bool) +
				   2 * sizeof(std::less<const char *>) +
				   2 * (3 * sizeof(void *) + 2 * sizeof(bool)) +
				   2 * (2 * sizeof(void *)) + sizeof(void *) + sizeof(size_t);
constexpr size_t string_frames =
	disjunct_frames +
	// assign(s,n): this/s/n/ref-return; _M_replace(this,pos,len1,s,len2),
	// old_size/new_size/p/how_much/ref-return, actual length checks/queries.
	3 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + 5 * sizeof(size_t) +
	6 * (sizeof(void *) + sizeof(size_t)) + sizeof(bool) +
	// _M_mutate(this,pos,len1,s,len2), how_much/new_capacity/r;
	// _M_create(this,capacityref,oldcapacity), max_size, allocation return.
	3 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(void *) + sizeof(size_t) +
	allocator_frames +
	// _S_copy(d,s,n), traits::copy(s1,s2,n) returned pointer and memcopy
	// argument/result carriers; one-character assign reference/char scopes.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(void *) + sizeof(char) +
	// old block dispose/destroy plus data/capacity/set-length and final NUL.
	6 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) + sizeof(char);

// Actual one string default/move/destruction closure; private vectors use the
// genuine vector move/destruction profile. Fixed member state is in sizeof.
constexpr size_t string_lifetime_frames =
	6 * sizeof(void *) + sizeof(size_t) + sizeof(char) + sizeof(std::allocator<char>) +
	3 * sizeof(void *) + sizeof(bool) + sizeof(void *) + sizeof(size_t) + 6 * sizeof(void *) +
	2 * sizeof(std::allocator<char>) + 2 * sizeof(bool) +
	12 * (sizeof(void *) + sizeof(size_t)) + string_frames + 5 * sizeof(void *) +
	2 * sizeof(size_t) + sizeof(bool) + allocator_frames;
// Complete original refine validity/cost/success and alchemy pure closures,
// fixed comparison temporary, units/value/index and term root i/j loops;
// original array/vector/string/span queries and find(char) source scopes.
constexpr size_t pure_frames =
	sizeof(craft_refine_wallet_cost) + sizeof(std::array<int64_t, 4>) + 6 * sizeof(void *) +
	sizeof(int64_t) + 4 * sizeof(size_t) + 4 * sizeof(bool) +
	12 * (sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(char) +
	4 * sizeof(size_t) + 2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	sizeof(char) + 2 * sizeof(void *) + sizeof(int) + sizeof(size_t) +
	// Defaulted cost array equality/equal/aux/aux1/memcmp source scopes.
	2 * sizeof(void *) + sizeof(bool) + 3 * (2 * sizeof(void *)) +
	4 * (3 * sizeof(void *) + sizeof(bool)) + 3 * (2 * sizeof(void *)) + sizeof(size_t) +
	2 * (2 * sizeof(void *) + sizeof(size_t) + sizeof(int));
// Four-iterator equal path: equal/__equal4 first1/last1/first2/last2,
// both range lengths, actual raw-pointer/normal-iterator niter/query carriers,
// then equal/aux/aux1/equal<true>/memcmp. No allocation.
constexpr size_t equal_frames = 2 * (4 * sizeof(void *) + sizeof(bool)) + 2 * sizeof(ptrdiff_t) +
				4 * (3 * sizeof(void *) + sizeof(bool)) + 8 * (2 * sizeof(void *)) +
				sizeof(size_t) +
				2 * (2 * sizeof(void *) + sizeof(size_t) + sizeof(int));
struct budget
{
	reserve_fn reserve;
	void *context;
	size_t outer;
	size_t frames;
	const craft_recipe_continuation *candidate = nullptr;
	const std::vector<uint8_t> *bytes = nullptr;
	template <typename T> bool heap(const std::vector<T> &value, size_t &total) const noexcept
	{
		return value.capacity() <= SIZE_MAX / sizeof(T) &&
		       add(total, value.capacity() * sizeof(T));
	}
	bool prefix(size_t &result, size_t extra) const noexcept
	{
		size_t current = outer;
		constexpr size_t observation =
			13 * sizeof(void *) + 8 * sizeof(size_t) + 6 * sizeof(bool);
		if (!add(current, sizeof(*this)) || !add(current, frames) ||
		    (candidate &&
		     (!heap(candidate->pouch_mutation, current) ||
		      !heap(candidate->refine_root_order, current) ||
		      (candidate->refine_material_name.capacity() > 15 &&
		       (candidate->refine_material_name.capacity() == SIZE_MAX ||
			!add(current, candidate->refine_material_name.capacity() + 1))))) ||
		    (bytes && !heap(*bytes, current)) || !add(current, observation) ||
		    !add(current, extra))
			return false;
		result = current;
		return true;
	}
	bool peak(size_t extra) const noexcept
	{
		size_t current = 0;
		return prefix(current, extra) && reserve && reserve(current, context);
	}
	template <typename T> bool growth(const std::vector<T> &value, size_t count) const noexcept
	{
		constexpr size_t own = 2 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool);
		size_t request = vector_frames + own;
		if (count > value.max_size() - value.size())
			return false;
		if (value.size() + count > value.capacity())
		{
			size_t next = value.size();
			if (!add(next, std::max(value.size(), count)) || next > value.max_size())
				next = value.max_size();
			if (next > SIZE_MAX / sizeof(T) || !add(request, next * sizeof(T)))
				return false;
		}
		return peak(request);
	}
	template <typename T> bool fresh(size_t count, size_t extra) const noexcept
	{
		constexpr size_t own = sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool);
		size_t request = vector_frames + extra;
		return count <= SIZE_MAX / sizeof(T) && add(request, count * sizeof(T)) &&
		       add(request, own) && peak(request);
	}
	bool text(std::string &value, const char *data, size_t length) const
	{
		constexpr size_t own = 3 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool);
		size_t request = string_frames + own;
		if (length > value.capacity())
		{
			size_t next = length;
			const size_t capacity = value.capacity();
			if (capacity > SIZE_MAX / 2)
				return false;
			if (next < 2 * capacity)
				next = 2 * capacity;
			if (next > value.max_size())
				next = value.max_size();
			if (next == SIZE_MAX || !add(request, next + 1))
				return false;
		}
		if (!peak(request))
			return false;
		value.assign(data, length);
		return true;
	}
};
} // namespace duris_craft_recipe_bounded_detail

inline bool craft_recipe_continuation_encode_bounded(const craft_recipe_continuation &terms,
						     std::vector<uint8_t> *encoded,
						     bool (*reserve)(size_t, void *) noexcept,
						     void *context, size_t outer) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)terms;
	(void)encoded;
	(void)reserve;
	(void)context;
	(void)outer;
	return false;
#else

	using namespace duris_craft_recipe_bounded_detail;
	constexpr size_t frames = sizeof(std::vector<uint8_t>) + 5 * sizeof(void *) +
				  8 * sizeof(size_t) + 2 * sizeof(uint64_t) + 2 * sizeof(bool) +
				  pure_frames;
	budget owner{ reserve, context, outer, frames };
	if (!owner.peak(0))
		return false;

	if (terms.discipline == craft_recipe_discipline::refine)
	{
		if (!encoded || !craft_refine_terms_valid(terms))
			return false;
		try
		{
			if (!owner.fresh<uint8_t>(
				    128, size_constructor_frames + sizeof(void *) +
						 sizeof(uint8_t) +
						 4 * (3 * sizeof(void *) + sizeof(size_t)) +
						 sizeof(uint8_t) + sizeof(bool) + sizeof(char)))
				return false;
			std::vector<uint8_t> bytes(128, 0);
			owner.bytes = &bytes;
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
				{
					if (!owner.growth(bytes, 1))
						return false;
					bytes.push_back(static_cast<uint8_t>(uid >> (8 * i)));
				}
			if (!owner.growth(bytes, terms.refine_material_name.size()))
				return false;
			bytes.insert(bytes.end(), terms.refine_material_name.begin(),
				     terms.refine_material_name.end());
			if (bytes.size() > CRAFT_RECIPE_CONTINUATION_MAX_BYTES)
				return false;
			if (!owner.peak(move_frames))
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
		if (!owner.peak(empty_constructor_frames))
			return false;
		std::vector<uint8_t> bytes;
		owner.bytes = &bytes;
		if (!owner.growth(bytes, CRAFT_RECIPE_CONTINUATION_HEADER_BYTES +
						 terms.pouch_mutation.size()))
			return false;
		bytes.reserve(CRAFT_RECIPE_CONTINUATION_HEADER_BYTES + terms.pouch_mutation.size());
		auto number = [&](uint64_t value, size_t size) -> bool
		{
			for (size_t index = 0; index < size; ++index)
			{
				if (!owner.growth(bytes, 1))
					return false;
				bytes.push_back(static_cast<uint8_t>(value >> (8 * index)));
			}
			return true;
		};
		const bool alchemy = craft_recipe_is_alchemy(terms.discipline);
		if (!number(terms.frozen_progression ? 4 : alchemy ? 2 : 1, 4))
			return false;
		if (!number(terms.player_pid, 4))
			return false;
		if (!number(static_cast<uint32_t>(terms.discipline), 4))
			return false;
		if (!number(terms.experience, 4))
			return false;
		if (!number(terms.recipe_vnum, 4))
			return false;
		if (!number(terms.output_uid, 8))
			return false;
		if (!number(terms.pouch_mutation.size(), 4))
			return false;
		if (alchemy || terms.frozen_progression)
		{
			if (!number(terms.frozen_progression ? terms.progression_skill_before :
							       terms.output_count,
				    4))
				return false;
			if (!number(terms.notch.learned_before, 4))
				return false;
			if (!number(terms.notch.learned_after, 4))
				return false;
			if (!number(terms.notch.timer_tag, 4))
				return false;
			if (!number(terms.notch.timer_duration, 4))
				return false;
		}
		if (!owner.growth(bytes, terms.pouch_mutation.size()))
			return false;
		bytes.insert(bytes.end(), terms.pouch_mutation.begin(), terms.pouch_mutation.end());
		if (!owner.peak(move_frames))
			return false;
		*encoded = std::move(bytes);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}

#endif
}

inline bool craft_recipe_continuation_decode_bounded(std::span<const uint8_t> bytes,
						     craft_recipe_continuation *terms,
						     bool (*reserve)(size_t, void *) noexcept,
						     void *context, size_t outer) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)bytes;
	(void)terms;
	(void)reserve;
	(void)context;
	(void)outer;
	return false;
#else

	using namespace duris_craft_recipe_bounded_detail;
	constexpr size_t frames = sizeof(craft_recipe_continuation) + sizeof(std::vector<uint8_t>) +
				  sizeof(std::span<const uint8_t>) + 6 * sizeof(void *) +
				  8 * sizeof(size_t) + 4 * sizeof(uint64_t) + 3 * sizeof(bool) +
				  pure_frames + equal_frames + string_lifetime_frames +
				  2 * move_frames + 3 * empty_constructor_frames;
	budget owner{ reserve, context, outer, frames };
	if (!owner.peak(0))
		return false;

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
			owner.candidate = &candidate;
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
			{
				if (!owner.growth(candidate.refine_root_order, 1))
					return false;
				candidate.refine_root_order.push_back(number(128 + 8 * i, 8));
			}
			if (!owner.text(candidate.refine_material_name,
					reinterpret_cast<const char *>(bytes.data() + 128 +
								       number(52, 4) * 8),
					number(112, 4)))
				return false;
			std::vector<uint8_t> canonical;
			owner.bytes = &canonical;
			size_t nested = 0;
			if (!owner.prefix(nested, 0))
				return false;
			if (!craft_recipe_continuation_encode_bounded(candidate, &canonical,
								      reserve, context, nested) ||
			    !std::equal(canonical.begin(), canonical.end(), bytes.begin(),
					bytes.end()))
				return false;
			if (!owner.peak(0))
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
		owner.candidate = &candidate;
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
		if (!owner.growth(candidate.pouch_mutation, bytes.size() - header))
			return false;
		candidate.pouch_mutation.assign(bytes.begin() + header, bytes.end());
		std::vector<uint8_t> canonical;
		owner.bytes = &canonical;
		size_t nested = 0;
		if (!owner.prefix(nested, 0))
			return false;
		if (!craft_recipe_continuation_encode_bounded(candidate, &canonical, reserve,
							      context, nested) ||
		    (version == 4 &&
		     !std::equal(canonical.begin(), canonical.end(), bytes.begin(), bytes.end())))
			return false;
		if (!owner.peak(0))
			return false;
		*terms = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}

#endif
}

#endif
