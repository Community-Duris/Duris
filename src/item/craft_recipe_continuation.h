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

// Genuine initial inline only for the existing bounded entry. This is a
// separate preentry object query, not the complete SOURCE or heap allowance.
// Caller input/prior output remains outer-owned; original entry and every
// supported version/continuation branch are unchanged.
inline bool craft_recipe_continuation_encode_initial_inline_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	if (!output)
		return false;
	*output = sizeof(duris_craft_recipe_bounded_detail::budget);
	return true;
#else
	(void)output;
	return false;
#endif
}
constexpr size_t craft_recipe_continuation_encode_initial_query_frame_bytes() noexcept
{
	// Getter output pointer and returned bool; result is caller-owned.
	return sizeof(size_t *) + sizeof(bool);
}
inline bool craft_recipe_continuation_decode_initial_inline_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	if (!output)
		return false;
	*output = sizeof(duris_craft_recipe_bounded_detail::budget);
	return true;
#else
	(void)output;
	return false;
#endif
}
constexpr size_t craft_recipe_continuation_decode_initial_query_frame_bytes() noexcept
{
	// Getter output pointer and returned bool; result is caller-owned.
	return sizeof(size_t *) + sizeof(bool);
}

// Unselected named member-lifetime Source ledger. This is only the reached
// craft_recipe_continuation member graph, not a full encoder/decoder profile.
// Object inline and every actual capacity remain owned at their original sites.
namespace duris_craft_recipe_bounded_detail
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
constexpr size_t source_P = sizeof(void *);
constexpr size_t source_N = sizeof(size_t);
constexpr size_t source_B = sizeof(bool);
template <class T> constexpr size_t member_vector_cleanup_source() noexcept
{
	// Actual vector/base/generated _Vector_impl/allocator cleanup receivers.
	// _Vector_impl is nontrivial; new_allocator destruction is excluded in C++20.
	// Matching byte/u64 leaves are the reviewed payload cleanup specializations.
	return 4 * source_P +
	       // _Destroy first/last/allocator, first/last and trivial aux first/last.
	       3 * source_P + 2 * source_P + 2 * source_P +
	       // get allocator; vector _M_deallocate, traits, allocator/new_allocator;
	       // sized delete. The two bools are real separate predicate wrappers.
	       2 * source_P + 4 * (2 * source_P + source_N) + source_P + source_N + 2 * source_B;
}
template <class T> constexpr size_t member_vector_move_assignment_source() noexcept
{
	using V = std::vector<T>;
	using A = std::allocator<T>;
	// operator this/source/reference result, real move_storage local and
	// propagate policy returned bool, std::move<vector> ref/result, actual
	// _M_move_assign this/source/by-value trivial true_type tag.
	constexpr size_t entry = 7 * source_P + 2 * source_B + sizeof(std::true_type);
	// _M_move_assign's real retained tmp and get_allocator's returned A;
	// get_allocator this and const get-Tp-allocator this/reference result.
	constexpr size_t retained_and_get = sizeof(V) + sizeof(A) + 3 * source_P;
	// const-A vector/base/impl constructors this/allocator refs, selected
	// allocator/new_allocator const-copy receiver/ref and data default ctor.
	// Actual allocator copy lanes also serve get_allocator sequentially.
	constexpr size_t construction = 11 * source_P;
	// _M_swap_data this/source, real three-pointer tmp, _M_copy_data this/ref.
	// Data default constructor is already named in construction above.
	// Three copy calls and both swap invocations return sequentially, with
	// no callback, recursion or lower reentry; their actual lane is reused.
	constexpr size_t swap = 7 * source_P;
	// alloc_on_move refs, std::move<allocator> ref/result, generated allocator
	// and new_allocator assignments this/source/reference results.
	constexpr size_t allocator_move = 10 * source_P;
	// Selected cleanup owns the mutable get-Tp-allocator receiver/reference
	// lane (also called before alloc_on_move) and allocator destructor lane
	// (also used for the returned A temporary). Those calls finish before
	// the later vector cleanup, and their own wrapper/local scopes above
	// remain separately inventoried. No trivial destructor is invented.
	return entry + retained_and_get + construction + swap + allocator_move +
	       member_vector_cleanup_source<T>();
}
// Mutable local-data, pointer_to, addressof and __addressof actual lanes.
constexpr size_t member_string_local_source = 4 * (2 * source_P);
constexpr size_t member_string_init_source = source_P + source_N + sizeof(char) + source_B;
constexpr size_t member_string_set_length_source =
	2 * (source_P + source_N) + 2 * source_P + 2 * source_P + sizeof(char) + source_B;
constexpr size_t member_string_default_source =
	// basic_string/hider this, pointer and allocator reference, actual default
	// allocator object, default/copy allocator/new_allocator and std::move.
	source_P + 3 * source_P + sizeof(std::allocator<char>) + 2 * source_P + 4 * source_P +
	2 * source_P +
	// Genuine temporary allocator cleanup receiver; new_allocator dtor is
	// PP-excluded in selected C++20.
	source_P + member_string_local_source + member_string_init_source +
	member_string_set_length_source;
constexpr size_t member_string_destroy_source =
	// Actual string/dispose/is-local/destroy receiver/size/result scopes.
	4 * source_P + source_N + source_B + 2 * source_P + member_string_local_source +
	// get-allocator and direct traits/allocator/new_allocator deallocation;
	// string does not call vector _M_deallocate. One real deallocate predicate.
	2 * source_P + 3 * (2 * source_P + source_N) + source_P + source_N + source_B +
	// Real hider and allocator destruction; no C++20 new_allocator dtor.
	2 * source_P;
// Complete selected char string move-assignment lower declaration graph.
// These are genuine source scopes; object storage/capacity is separately owned.
constexpr size_t member_string_move_entry_source =
	// operator this/source/reference result, equal_allocs local; reachable
	// nonlocal __data pointer and __capacity size local.
	4 * source_P + source_N + source_B;
constexpr size_t member_string_move_allocator_source =
	// Actual always_equal and propagate wrapper bool results. noexcept-only
	// nothrow_move is not executed. Alloc_on_move refs, allocator std::move
	// ref/result, generated allocator/base assignment this/source/results.
	2 * source_B + 10 * source_P +
	// Mutable get_allocator this/reference result, reused sequentially.
	2 * source_P;
constexpr size_t member_string_move_const_locality_source =
	// is_local this/bool; const data this/pointer; const local_data,
	// pointer_to, addressof and __addressof actual argument/result scopes.
	11 * source_P + source_B;
constexpr size_t member_string_move_queries_source =
	// Distinct size() and length() receivers/returned size values; self-move
	// check calls actual __addressof<string> ref/result, not a char lane.
	2 * (source_P + source_N) + 2 * source_P;
constexpr size_t member_string_move_copy_source =
	// _S_copy destination/source/count; traits<char>::copy destination/source,
	// count/returned char pointer. Its real __is_constant_evaluated bool
	// wrapper is already owned by the sequential assign lane below.
	2 * source_P + source_N + 3 * source_P + source_N;
constexpr size_t member_string_move_setters_source =
	// Data setter this/pointer and capacity setter this/size. The length
	// setter's actual scope is owned by the set-length lane below.
	2 * source_P + source_P + source_N;
constexpr size_t member_string_move_local_route_source =
	// use_local_data this/returned pointer, real init wrapper and selected
	// mutable local-data/pointer_to/addressof/__addressof subtree. Root's
	// conservative init loop index/char is retained, not executed evidence.
	2 * source_P + member_string_init_source + member_string_local_source;
constexpr size_t member_string_move_clear_source =
	// clear this; set_length this/count; length setter this/count;
	// assign references, real zero-character temporary and one actual
	// __is_constant_evaluated wrapper result. Const data getter was already
	// owned by locality above and returns before these sequential calls.
	source_P + 2 * (source_P + source_N) + 2 * source_P + sizeof(char) + source_B;
constexpr size_t member_string_move_assignment_source =
	member_string_move_entry_source + member_string_move_allocator_source +
	member_string_move_const_locality_source + member_string_move_queries_source +
	member_string_move_copy_source + member_string_move_setters_source +
	member_string_move_local_route_source + member_string_move_clear_source;
constexpr size_t member_lifetime_source =
	// Distinct byte and uint64_t vector instantiations, counted once per named
	// reached lane, not multiplied by row/heap-object counts. Canonical wire
	// vector uses the same byte specialization; old storage is separately owned.
	2 * (6 * source_P) + member_vector_cleanup_source<uint8_t>() +
	member_vector_cleanup_source<uint64_t>() + member_vector_move_assignment_source<uint8_t>() +
	member_vector_move_assignment_source<uint64_t>() + member_string_default_source +
	member_string_move_assignment_source + member_string_destroy_source +
	// Generated recipe default constructor this and its plain-default
	// refine_cost constructor this. notch = {} is recursive aggregate
	// initialization, without an invented constructor receiver.
	2 * source_P +
	// Actual nontrivial recipe assignment owns this/source/reference result.
	// Notch, wallet cost and member arrays have only undeclared implicit
	// trivial assignments: no new callable Source scopes are introduced.
	3 * source_P +
	// Only recipe destructor is nontrivial. notch/cost/fixed-array members
	// have trivial destruction and no callable destructor receiver body.
	source_P;
#endif
} // namespace duris_craft_recipe_bounded_detail

// Unselected exact own declaration ledger of the original recipe budget.
// Library/vector/string/callback and whole encoder/decoder joins are separate.
namespace duris_craft_recipe_bounded_detail
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
constexpr size_t budget_add_own_source = sizeof(size_t *) + sizeof(size_t) + sizeof(bool);
template <class T> constexpr size_t budget_heap_own_source =
	sizeof(const budget *) + sizeof(const std::vector<T> *) + sizeof(size_t *) + sizeof(bool);
constexpr size_t budget_prefix_own_source =
	// this/result, extra, current, observation constexpr and return bool.
	sizeof(const budget *) + sizeof(size_t *) + 3 * sizeof(size_t) + sizeof(bool);
constexpr size_t budget_peak_own_source =
	// this, extra/current and returned bool; callback graph is separately owned.
	sizeof(const budget *) + 2 * sizeof(size_t) + sizeof(bool);
template <class T> constexpr size_t budget_growth_own_source =
	// this/value, count/own constexpr/request/next and returned bool.
	sizeof(const budget *) + sizeof(const std::vector<T> *) + 4 * sizeof(size_t) + sizeof(bool);
constexpr size_t budget_fresh_byte_own_source =
	// this, count/extra/own constexpr/request and returned bool.
	sizeof(const budget *) + 4 * sizeof(size_t) + sizeof(bool);
constexpr size_t budget_text_own_source =
	// this/value/data, length/own constexpr/request/next/capacity and bool.
	sizeof(const budget *) + sizeof(std::string *) + sizeof(const char *) + 5 * sizeof(size_t) +
	sizeof(bool);
[[maybe_unused]] constexpr size_t budget_named_own_source =
	budget_add_own_source + budget_heap_own_source<uint8_t> + budget_heap_own_source<uint64_t> +
	budget_prefix_own_source + budget_peak_own_source + budget_growth_own_source<uint8_t> +
	budget_growth_own_source<uint64_t> + budget_fresh_byte_own_source + budget_text_own_source;
// Exact old declarations already retain growth<T>'s whole 2P+4N+B group.
// Fresh byte's old P+3N+B covers count, extra and request; text's old
// 3P+4N+B covers length, request, next and capacity. Each also declares its
// own automatic constexpr size_t, a distinct source object missing there.
// This is only that verified missing own pair, not the whole supplement.
[[maybe_unused]] constexpr size_t budget_allocation_own_missing_source = 2 * sizeof(size_t);
#endif
} // namespace duris_craft_recipe_bounded_detail

// Finite selected default/cleanup corrections only. Root's conservative
// init_local_buf loop index/char above stay declared Source allowance, not
// runtime execution. The older move-assignment and generated aggregate groups
// still require their own actual selected-body/credit review; member_lifetime
// is unselected and is not a complete recipe encoder/decoder Source contract.

// Private prospective equivalent fixed counterpart bodies. Original APIs and
// all original versions/algorithms above remain byte-exact. These bodies are
// not selectable as complete until their full Source admission is finished.
namespace duris_craft_recipe_bounded_detail
{
// The fixed companion's frames must own complete static Source plus its
// exact inline DTO envelope once. Only actual prospective new capacity is
// added in growth/fresh/text. This body is UNSELECTED until that full frame
// provider is joined; no old mixed library allowance is relabeled complete.
struct complete_recipe_budget
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
		if (!add(current, sizeof(*this)) || !add(current, frames) ||
		    (candidate &&
		     (!heap(candidate->pouch_mutation, current) ||
		      !heap(candidate->refine_root_order, current) ||
		      (candidate->refine_material_name.capacity() > 15 &&
		       (candidate->refine_material_name.capacity() == SIZE_MAX ||
			!add(current, candidate->refine_material_name.capacity() + 1))))) ||
		    (bytes && !heap(*bytes, current)) || !add(current, extra))
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
		size_t request = 0;
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
	template <typename T> bool fresh(size_t count) const noexcept
	{
		size_t request = 0;
		return count <= SIZE_MAX / sizeof(T) && add(request, count * sizeof(T)) &&
		       peak(request);
	}
	bool text(std::string &value, const char *data, size_t length) const
	{
		size_t request = 0;
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

struct complete_recipe_put
{
	std::vector<uint8_t> &bytes;
	void operator()(size_t offset, uint64_t value, size_t width) const
	{
		for (size_t i = 0; i < width; ++i)
			bytes[offset + i] = static_cast<uint8_t>(value >> (8 * i));
	}
};
struct complete_recipe_number
{
	complete_recipe_budget &owner;
	std::vector<uint8_t> &bytes;
	bool operator()(uint64_t value, size_t size) const
	{
		for (size_t index = 0; index < size; ++index)
		{
			if (!owner.growth(bytes, 1))
				return false;
			bytes.push_back(static_cast<uint8_t>(value >> (8 * index)));
		}
		return true;
	}
};
struct complete_recipe_read
{
	const std::span<const uint8_t> &bytes;
	uint64_t operator()(size_t offset, size_t size) const
	{
		uint64_t value = 0;
		for (size_t index = 0; index < size; ++index)
			value |= static_cast<uint64_t>(bytes[offset + index]) << (index * 8);
		return value;
	}
};
constexpr size_t complete_recipe_put_capture_source = sizeof(complete_recipe_put);
constexpr size_t complete_recipe_number_capture_source = sizeof(complete_recipe_number);
constexpr size_t complete_recipe_read_capture_source = sizeof(complete_recipe_read);
} // namespace duris_craft_recipe_bounded_detail

// Private actual fixed own Source rows; complete library/whole-frame join OPEN.
namespace duris_craft_recipe_bounded_detail
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
constexpr size_t complete_recipe_encode_wrapper_source =
	// terms/encoded/reserve/context4P; outer and actual frames2N;
	// two original catch references2P; bool result. Actual budget object
	// and wire vector object have separate initial/inline ownership.
	6 * source_P + 2 * source_N + source_B;
constexpr size_t complete_recipe_put_operator_source =
	// actual named callable this, offset/width/i, uint64 value; void result.
	source_P + 3 * source_N + sizeof(uint64_t);
constexpr size_t complete_recipe_number_operator_source =
	// actual callable this, value, size/index and bool result.
	source_P + 2 * source_N + sizeof(uint64_t) + source_B;
constexpr size_t complete_recipe_encode_loops_source =
	// Actual four-entry loop i and UID-byte inner i; UID loop's const range
	// reference and genuine begin/end iterator locals stay live through its
	// nested byte loop. Iterator constructors/operators are lower Source.
	2 * source_N + source_P + 2 * sizeof(std::vector<uint64_t>::const_iterator) +
	sizeof(uint64_t) +
	// Ordinary alchemy local bool. Named capture objects are actual sizeof,
	// never assumed old-lambda capture representations.
	source_B + sizeof(complete_recipe_put) + sizeof(complete_recipe_number);
constexpr size_t complete_recipe_decode_wrapper_source =
	// Actual by-value span; terms/reserve/context3P and two catch refs2P;
	// outer/frames/header/two nested locals/two loop indices7N;
	// version and array before/after uint64 locals; bool return.
	sizeof(std::span<const uint8_t>) + 5 * source_P + 7 * source_N + 3 * sizeof(uint64_t) +
	source_B +
	// Genuine aggregate notch temporary at the original assignment site.
	sizeof(skill_notch_outcome) + sizeof(complete_recipe_read);
constexpr size_t complete_recipe_read_operator_source =
	// actual callable this, offset/size/index, local uint64 value and returned
	// uint64 carrier (possibly retained through a later push_back call).
	source_P + 3 * source_N + 2 * sizeof(uint64_t);
template <class T> constexpr size_t complete_recipe_budget_heap_own_source =
	sizeof(const complete_recipe_budget *) + sizeof(const std::vector<T> *) + sizeof(size_t *) +
	source_B;
constexpr size_t complete_recipe_budget_prefix_own_source = 2 * source_P + 2 * source_N + source_B;
constexpr size_t complete_recipe_budget_peak_own_source = source_P + 2 * source_N + source_B;
template <class T> constexpr size_t complete_recipe_budget_growth_own_source =
	2 * source_P + 3 * source_N + source_B;
constexpr size_t complete_recipe_budget_fresh_own_source = source_P + 2 * source_N + source_B;
constexpr size_t complete_recipe_budget_text_own_source = 3 * source_P + 4 * source_N + source_B;
constexpr size_t complete_recipe_refine_succeeded_own_source = source_P + source_B;
constexpr size_t complete_recipe_refine_cost_valid_own_source =
	// cost ref; actual constexpr units array, value int64, both i declarations;
	// bool result. Array accessor/equality children are separately mapped.
	source_P + sizeof(std::array<int64_t, 4>) + sizeof(int64_t) + 2 * source_N + source_B;
constexpr size_t complete_recipe_refine_terms_valid_own_source = source_P + 2 * source_N + source_B;
constexpr size_t complete_recipe_alchemy_own_source = sizeof(craft_recipe_discipline) + source_B;
constexpr size_t complete_recipe_refine_cost_compare_own_source =
	// Actual defaulted member operator== this/other/bool result. Rewritten !=
	// calls it; no invented separate != wrapper. Its actual Cost{} temporary
	// storage is separately retained through this comparator at reached sites.
	2 * source_P + source_B + sizeof(craft_refine_wallet_cost);
#endif
} // namespace duris_craft_recipe_bounded_detail

// Actual selected pure-validation lower scopes. No complete public contract yet.
namespace duris_craft_recipe_bounded_detail
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
constexpr size_t complete_recipe_array_const_i32_access_source = 2 * source_P + source_N;
constexpr size_t complete_recipe_array_mutable_i32_access_source = 2 * source_P + source_N;
constexpr size_t complete_recipe_array_const_i64_access_source = 2 * source_P + source_N;
constexpr size_t complete_recipe_string_size_source = source_P + source_N;
constexpr size_t complete_recipe_string_empty_source = source_P + source_B;
constexpr size_t complete_recipe_string_const_data_source = 2 * source_P;
constexpr size_t complete_recipe_string_find_own_source =
	// this, char argument, pos, ret, size, data, n, p and returned size.
	3 * source_P + 5 * source_N + sizeof(char);
constexpr size_t complete_recipe_traits_char_find_source =
	// s, n, a-reference and const char pointer return; one actual reached
	// std::__is_constant_evaluated Boolean. Runtime memchr is a builtin,
	// not a second C++ declaration or result-storage allowance.
	3 * source_P + source_N + source_B;
constexpr size_t complete_recipe_array_i32_equal_source =
	// array operator== one/two/B; const begin/end/data each this/result2P.
	2 * source_P + source_B + 6 * source_P +
	// equal, equal_aux, equal_aux1 and __equal<true>::equal retain their
	// genuine pointer formals/results through nested calls. aux1's __simple
	// is a declared Boolean; true equal owns len and its Boolean return.
	3 * source_P + source_B + 3 * source_P + source_B + 3 * source_P + 2 * source_B +
	3 * source_P + source_N + source_B +
	// Three actual pointer __niter_base calls have distinct returned argument
	// carriers retained together until equal_aux1. Price all three pairs.
	6 * source_P +
	// Actual std::__memcmp C++ wrapper: two pointers, count, returned int,
	// and real std::is_constant_evaluated result. No synthetic builtin frame.
	2 * source_P + source_N + sizeof(int) + source_B;
#endif
} // namespace duris_craft_recipe_bounded_detail

// Selected fresh-candidate char-string assignment declarations, not a public
// whole-recipe provider. Matching earlier lifetime/pure lanes remain named
// separately so final assembly cannot silently retain duplicate library Source.
namespace duris_craft_recipe_bounded_detail
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
constexpr size_t complete_recipe_string_assign_entry_source = 3 * source_P + source_N;
constexpr size_t complete_recipe_string_replace_own_source =
	// this, source, pos/len1/len2/old_size/new_size/how_much, p and ref result.
	// Runtime is_constant_evaluated uses the earlier actual default-init lane;
	// its consteval-only newp allocation/loop is not selected by this runtime.
	4 * source_P + 6 * source_N;
constexpr size_t complete_recipe_string_check_length_source = 2 * source_P + 2 * source_N;
constexpr size_t complete_recipe_string_disjunct_source =
	// this/source/B. Both genuine less temporary objects can overlap through
	// the full || expression; trivial objects have no invented ctor/dtor.
	2 * source_P + source_B + 2 * sizeof(std::less<const char *>);
constexpr size_t complete_recipe_string_less_source =
	// actual this/x/y/B; its __is_constant_evaluated result uses the matching
	// nonrecursive predicate lane already present in traits assign/cleanup.
	3 * source_P + source_B;
constexpr size_t complete_recipe_string_capacity_own_source = source_P + source_N;
constexpr size_t complete_recipe_string_mutate_own_source =
	// this/source/r, pos/len1/len2/how_much/new_capacity. Fresh candidate
	// length0 makes both old-content copy branches unreachable.
	3 * source_P + 5 * source_N;
constexpr size_t complete_recipe_string_create_own_source = 3 * source_P + source_N;
constexpr size_t complete_recipe_string_max_size_source =
	// string max_size this/N, const allocator getter this/ref and the real
	// std::allocator<char> specialization traits max_size alloc-ref/N.
	// No generic allocator max_size fallback or vector wrapper is selected.
	4 * source_P + 2 * source_N;
constexpr size_t complete_recipe_string_char_allocation_source =
	// _S_allocate allocator-ref/count/local-p/returned-p; actual traits
	// allocate alloc-ref/count/returned-p; allocator allocate this/count/p;
	// new_allocator allocate this/count/hint/returned-p; _M_max_size this/N;
	// real global operator new's declared count/returned pointer.
	3 * source_P + source_N + 2 * source_P + source_N + 2 * source_P + source_N + 3 * source_P +
	source_N + source_P + source_N + source_P + source_N;
constexpr size_t complete_recipe_string_mutable_allocator_source = 2 * source_P;
constexpr size_t complete_recipe_string_copy_source = 5 * source_P + 2 * source_N;
constexpr size_t complete_recipe_string_data_capacity_setters_source = 3 * source_P + source_N;
#endif
} // namespace duris_craft_recipe_bounded_detail

// Internal complete-child preentry adapter. EncoderSource must be bound only
// to the authenticated same-header complete prospective encoder constant.
inline bool craft_recipe_continuation_encode_complete_bounded(const craft_recipe_continuation &,
							      std::vector<uint8_t> *,
							      bool (*)(size_t, void *) noexcept,
							      void *, size_t) noexcept;

namespace duris_craft_recipe_bounded_detail
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
template <size_t EncoderSource>
inline bool complete_recipe_encode_child(complete_recipe_budget &owner,
					 const craft_recipe_continuation &terms,
					 std::vector<uint8_t> *encoded, reserve_fn reserve,
					 void *context) noexcept
{
	size_t nested = 0;
	size_t request = EncoderSource;
	if (!add(request, sizeof(complete_recipe_budget)) || !owner.prefix(nested, request) ||
	    !reserve || !reserve(nested, context) || !owner.prefix(nested, 0))
		return false;
	// Full Source plus actual initial was admitted transiently above. The
	// genuine parent-only prefix now lets the child own its Source/controller
	// once; no second child copy remains retained in outer.
	return craft_recipe_continuation_encode_complete_bounded(terms, encoded, reserve, context,
								 nested);
}
constexpr size_t complete_recipe_encoder_preentry_adapter_source =
	// Actual owner/terms/encoded/reserve/context formals, nested/request,
	// Boolean return. Same-header constexpr binding executes no getter.
	5 * source_P + 2 * source_N + source_B;
#endif
} // namespace duris_craft_recipe_bounded_detail

// Canonical byte/span equality actual rows. Typed library sharing with the
// vector mutation graphs must be proved before a complete union is selected.
namespace duris_craft_recipe_bounded_detail
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
using complete_recipe_wire_iterator = std::vector<uint8_t>::iterator;
using complete_recipe_input_iterator = std::span<const uint8_t>::iterator;
using complete_recipe_equal_ra = std::__and_<
	std::is_same<typename std::iterator_traits<complete_recipe_wire_iterator>::iterator_category,
		     std::random_access_iterator_tag>,
	std::is_same<typename std::iterator_traits<complete_recipe_input_iterator>::iterator_category,
		     std::random_access_iterator_tag>>;
constexpr size_t complete_recipe_equal4_entry_source = 2 * sizeof(complete_recipe_wire_iterator) +
						       2 * sizeof(complete_recipe_input_iterator) +
						       source_B;
constexpr size_t complete_recipe_equal4_helper_source =
	2 * sizeof(complete_recipe_wire_iterator) + 2 * sizeof(complete_recipe_input_iterator) +
	2 * sizeof(std::ptrdiff_t) + source_B +
	// Real _RAIters condition temporary and integral_constant conversion
	// this/B; no implicit trivial tag constructor/destructor is invented.
	sizeof(complete_recipe_equal_ra) + source_P + source_B;
constexpr size_t complete_recipe_equal3_entry_source = 2 * sizeof(complete_recipe_wire_iterator) +
						       sizeof(complete_recipe_input_iterator) +
						       source_B;
constexpr size_t complete_recipe_equal_aux_own_source = complete_recipe_equal3_entry_source;
template <class Iterator> constexpr size_t complete_recipe_distance_own_source =
	2 * sizeof(Iterator) + sizeof(std::ptrdiff_t);
template <class Iterator> constexpr size_t complete_recipe_ra_distance_own_source =
	2 * sizeof(Iterator) + sizeof(std::random_access_iterator_tag) + sizeof(std::ptrdiff_t);
template <class Iterator> constexpr size_t complete_recipe_iterator_category_source =
	source_P + sizeof(typename std::iterator_traits<Iterator>::iterator_category);
template <class Iterator> constexpr size_t complete_recipe_normal_subtract_own_source =
	2 * source_P + sizeof(typename std::iterator_traits<Iterator>::difference_type);
template <class Iterator> constexpr size_t complete_recipe_normal_base_source = 2 * source_P;
template <class Iterator> constexpr size_t complete_recipe_normal_niter_own_source =
	// Actual normal_iterator by-value formal and raw pointer return. base()
	// is a distinct named lower scope, not raw-input niter_base(pointer).
	sizeof(Iterator) + source_P;
constexpr size_t complete_recipe_byte_equal_aux1_source = 3 * source_P + 2 * source_B;
constexpr size_t complete_recipe_byte_equal_true_source = 3 * source_P + source_N + source_B;
constexpr size_t complete_recipe_byte_memcmp_source =
	// Genuine C++ std::__memcmp declaration, with real predicate result.
	// There is no invented builtin memcmp callee frame here.
	2 * source_P + source_N + sizeof(int) + source_B;
constexpr size_t complete_recipe_wire_begin_own_source =
	source_P + sizeof(complete_recipe_wire_iterator);
constexpr size_t complete_recipe_wire_end_own_source = complete_recipe_wire_begin_own_source;
constexpr size_t complete_recipe_span_begin_own_source =
	source_P + sizeof(complete_recipe_input_iterator);
constexpr size_t complete_recipe_span_end_own_source =
	// The actual _M_ptr + size() pointer prvalue binds to the normal-iterator
	// constructor's const pointer reference, materializing one real pointer.
	complete_recipe_span_begin_own_source + source_P;
constexpr size_t complete_recipe_span_size_source = source_P + source_N;
constexpr size_t complete_recipe_span_extent_source = source_P + source_N;
constexpr size_t complete_recipe_span_data_source = 2 * source_P;
constexpr size_t complete_recipe_span_subscript_source = 2 * source_P + source_N;
template <class Iterator> constexpr size_t complete_recipe_normal_pointer_constructor_source =
	// Actual explicitly defined pointer-const-reference constructor this/ref.
	// Mere iterator value copies have no invented implicit trivial ctor/dtor.
	2 * source_P;
#endif
} // namespace duris_craft_recipe_bounded_detail

// Exact selected vector primitive declaration rows; caller/copy/fill/insert
// and complete owning unions remain separate. Only byte/u64 are reached here.
namespace duris_craft_recipe_bounded_detail
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
template <class T> constexpr size_t complete_recipe_vector_size_source = source_P + source_N;
template <class T> constexpr size_t complete_recipe_vector_capacity_source = source_P + source_N;
template <class T> constexpr size_t complete_recipe_vector_const_subscript_source =
	2 * source_P + source_N;
template <class T> constexpr size_t complete_recipe_vector_mutable_subscript_source =
	2 * source_P + source_N;
template <class T> constexpr size_t complete_recipe_vector_max_size_source =
	// max_size this/N, const allocator getter this/ref, _S_max_size allocref,
	// actual diffmax/allocmax and returnedN, specialized traits max_size
	// allocref/N, and min(first-ref,last-ref,returned-ref).
	8 * source_P + 5 * source_N;
template <class T> constexpr size_t complete_recipe_vector_allocate_source =
	// _M_allocate, traits allocate and allocator allocate each this-or-ref,
	// count and pointer return; new_allocator allocate this/count/hint/p;
	// _M_max_size this/N and actual ordinary operator new count/p return.
	// Actual allocator predicate B; no aligned scalar branch or builtin frame.
	11 * source_P + 6 * source_N + source_B;
template <class T> constexpr size_t complete_recipe_vector_single_construct_source =
	// traits construct allocref/location/argref; its forward ref/result;
	// construct_at location/argref/returned-p, its forward ref/result, and
	// actual declared placement new count/location/returned-p.
	12 * source_P + source_N;
template <class T> constexpr size_t complete_recipe_vector_relocate_source =
	// _S_relocate/__relocate_a/__relocate_a_1 each first/last/result/allocator
	// plus returned-p. Three raw niter_base formals/returned pointers remain
	// actual argument carriers; a1 owns count and real predicate B.
	// Ordinary builtin memmove has no invented C++ frame.
	21 * source_P + sizeof(std::ptrdiff_t) + source_B;
template <class T> constexpr size_t complete_recipe_vector_check_len_own_source =
	// this/message, n/len/returnedN. size(), max_size() and max() are
	// separately named reached descendants, not silent broad free credit.
	2 * source_P + 3 * source_N;
constexpr size_t complete_recipe_size_max_source = 3 * source_P;
constexpr size_t complete_recipe_size_min_source = 3 * source_P;
#endif
} // namespace duris_craft_recipe_bounded_detail

// Genuine vector<byte>(128,0) fill route. No default_n/default_append or
// invented scalar constructor/builtin memset callee is reached here.
namespace duris_craft_recipe_bounded_detail
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
constexpr size_t complete_recipe_byte_fill_constructor_own_source =
	// this/count/value-ref/allocator-ref; actual default allocator and zero
	// byte argument temporaries. Actual vector DTO is separately inline-owned.
	3 * source_P + source_N + sizeof(std::allocator<uint8_t>) + sizeof(uint8_t);
constexpr size_t complete_recipe_byte_check_init_len_source =
	// _S_check_init_len(n, allocator-ref) returns n; its real converted
	// _Tp_alloc_type(__a) temporary lives through the nested _S_max_size.
	source_P + 2 * source_N + sizeof(std::allocator<uint8_t>);
// The converted temporary invokes the actual allocator/new_allocator
// const-copy lane4P and allocator cleanupP. Reuse only these exact reached
// lanes already named by fill/base and temporary cleanup at sequential sites;
// the temporary OBJECT above is distinct from the public fill default A.
constexpr size_t complete_recipe_allocator_const_copy_source = 4 * source_P;
constexpr size_t complete_recipe_allocator_cleanup_source = source_P;
constexpr size_t complete_recipe_byte_fill_base_source =
	// Base(this,count,allocatorref), impl(this,allocatorref), allocator/base
	// const-copy this/source, actual data default receiver and create-storage
	// this/count. Actual allocation/max-size closures are separate primitives.
	2 * source_P + source_N + 2 * source_P + 4 * source_P + source_P + source_P + source_N;
constexpr size_t complete_recipe_byte_fill_allocator_temporary_source =
	// Explicit allocator/new_allocator default constructors and actual
	// allocator destructor. C++20 new_allocator destructor is PP-excluded.
	3 * source_P;
constexpr size_t complete_recipe_byte_fill_initialize_source = 2 * source_P + source_N;
constexpr size_t complete_recipe_byte_uninitialized_fill_a_source = 4 * source_P + source_N;
constexpr size_t complete_recipe_byte_uninitialized_fill_source =
	// first/count/value-ref/returned-p, actual __can_fill Boolean local.
	3 * source_P + source_N + source_B;
constexpr size_t complete_recipe_byte_uninitialized_fill_true_source = 3 * source_P + source_N;
constexpr size_t complete_recipe_byte_fill_n_source = 3 * source_P + source_N;
constexpr size_t complete_recipe_size_to_integer_source = 2 * source_N;
constexpr size_t complete_recipe_byte_fill_n_a_source =
	3 * source_P + source_N + sizeof(std::random_access_iterator_tag);
constexpr size_t complete_recipe_byte_fill_a_source = 3 * source_P;
constexpr size_t complete_recipe_byte_fill_a1_source =
	// Actual first/last/value-ref, typed byte __tmp and size_t __len.
	// Shared real std::is_constant_evaluated lane is joined once explicitly.
	3 * source_P + sizeof(uint8_t) + source_N;
constexpr size_t complete_recipe_std_constant_evaluated_source = source_B;
constexpr size_t complete_recipe_std_internal_constant_evaluated_source = source_B;
#endif
} // namespace duris_craft_recipe_bounded_detail

// Selected reserve and scalar-rvalue push declaration rows. Relocation is
// the true GNU13 constexpr-selected byte/u64 path; unused alternatives stay out.
namespace duris_craft_recipe_bounded_detail
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
template <class T> constexpr size_t complete_recipe_vector_reserve_own_source =
	// this/count, actual old_size and tmp allocation pointer.
	2 * source_P + 2 * source_N;
template <class T> constexpr size_t complete_recipe_vector_push_own_source = 2 * source_P;
template <class T> constexpr size_t complete_recipe_vector_emplace_own_source = 3 * source_P;
template <class T> constexpr size_t complete_recipe_scalar_move_source = 2 * source_P;
template <class T> constexpr size_t complete_recipe_scalar_forward_source = 2 * source_P;
template <class T> constexpr size_t complete_recipe_vector_realloc_insert_own_source =
	// this/argref, real position iterator, four old/new pointers, len and
	// elems_before. Typed iterator sizeof is never assumed pointer sizeof.
	6 * source_P + sizeof(typename std::vector<T>::iterator) + 2 * source_N;
template <class T> constexpr size_t complete_recipe_vector_back_own_source = 2 * source_P;
template <class Iterator> constexpr size_t complete_recipe_normal_offset_own_source =
	// this, difference, actual returned iterator and materialized pointer
	// argument bound to the lower explicit const-reference constructor.
	2 * source_P + sizeof(typename std::iterator_traits<Iterator>::difference_type) +
	sizeof(Iterator);
template <class Iterator> constexpr size_t complete_recipe_normal_dereference_source = 2 * source_P;
template <class T> constexpr size_t complete_recipe_vector_mutable_allocator_source = 2 * source_P;
constexpr size_t complete_recipe_byte_push_argument_source = sizeof(uint8_t);
// Decoder's number() returned uint64 carrier is already an actual retained
// read-operator Source value at push_back(number(...)); no second uint64
// argument object is manufactured beside that same returned scalar.
#endif
} // namespace duris_craft_recipe_bounded_detail

// Typed selected copy/move declaration rows. These are not a whole vector
// union and do not import unselected old normalizer/copy envelopes.
namespace duris_craft_recipe_bounded_detail
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
template <class Input, class Output> constexpr size_t complete_recipe_copy_own_source =
	2 * sizeof(Input) + 2 * sizeof(Output);
template <class Input, class Output> constexpr size_t complete_recipe_copy_move_a_own_source =
	2 * sizeof(Input) + 2 * sizeof(Output);
template <class Iterator> constexpr size_t complete_recipe_generic_miter_source =
	2 * sizeof(Iterator);
template <class T> constexpr size_t complete_recipe_move_miter_own_source =
	// Actual move_iterator by-value formal and returned raw pointer; its
	// C++20 const& base() returns const raw-iterator reference; raw miter runs.
	sizeof(std::move_iterator<T *>) + source_P;
template <class T> constexpr size_t complete_recipe_move_iterator_base_source = 2 * source_P;
template <class T> constexpr size_t complete_recipe_move_iterator_constructor_source = 2 * source_P;
template <class T> constexpr size_t complete_recipe_move_iterator_factory_own_source =
	source_P + sizeof(std::move_iterator<T *>);
template <class T> constexpr size_t complete_recipe_raw_niter_source = 2 * source_P;
constexpr size_t complete_recipe_raw_niter_wrap_source = 3 * source_P;
template <class Iterator> constexpr size_t complete_recipe_normal_niter_wrap_own_source =
	// Actual generic niter_wrap's from is by VALUE, not a reference.
	// res raw-pointer formal and returned typed normal iterator stay separate.
	2 * sizeof(Iterator) + source_P;
template <class Input, class Output> constexpr size_t complete_recipe_uninitialized_copy_a_source =
	2 * sizeof(Input) + 2 * sizeof(Output) + source_P;
template <class Input, class Output> constexpr size_t complete_recipe_uninitialized_copy_source =
	// Real __can_memmove and __assignable Boolean locals remain live into
	// __uninitialized_copy<true>; predicate is separately named shared lane.
	2 * sizeof(Input) + 2 * sizeof(Output) + 2 * source_B;
template <class Input, class Output>
constexpr size_t complete_recipe_uninitialized_copy_true_source =
	2 * sizeof(Input) + 2 * sizeof(Output);
template <class T> constexpr size_t complete_recipe_uninitialized_move_own_source = 5 * source_P;
template <class T> constexpr size_t complete_recipe_uninitialized_move_if_noexcept_own_source =
	5 * source_P;
template <class Input, class Output> constexpr size_t complete_recipe_raw_copy_a1_source =
	2 * sizeof(Input) + 2 * sizeof(Output);
template <class Input, class Output> constexpr size_t complete_recipe_raw_copy_a2_source =
	2 * sizeof(Input) + 2 * sizeof(Output);
constexpr size_t complete_recipe_byte_simple_copy_m_source =
	// Actual first/last/result/returned-p and ptrdiff _Num; Num==1 calls the
	// real assign_one wrapper below. Builtin memmove declares no new C++ frame.
	4 * source_P + sizeof(std::ptrdiff_t);
constexpr size_t complete_recipe_char_to_byte_scalar_copy_m_source =
	// Distinct char input/byte output types select false,false,RA generic
	// scalar copy and its actual difference_type __n. No byte memcpy lane.
	4 * source_P + sizeof(std::ptrdiff_t);
constexpr size_t complete_recipe_byte_assign_one_source = 2 * source_P;
constexpr size_t complete_recipe_byte_move_assign_one_source = 2 * source_P;
#endif
} // namespace duris_craft_recipe_bounded_detail

// Exact owning end-insert and fresh span-assign declarations, with typed
// iterator/tag widths. All reachable lower dependencies are separate rows.
namespace duris_craft_recipe_bounded_detail
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
template <class Input> constexpr size_t complete_recipe_byte_insert_own_source =
	// this, actual const-position, first/last, offset and returned iterator.
	source_P + sizeof(std::vector<uint8_t>::const_iterator) + 2 * sizeof(Input) +
	sizeof(std::ptrdiff_t) + sizeof(std::vector<uint8_t>::iterator);
template <class Input> constexpr size_t complete_recipe_byte_range_insert_own_source =
	// this/position/first/last/forward tag/n; fitting elems_after/old_finish/mid;
	// growing old_start/old_finish/len/new_start/new_finish. Conservative
	// union of genuine distinct named branch declarations, not generic margin.
	6 * source_P + sizeof(std::vector<uint8_t>::iterator) + 3 * sizeof(Input) + 3 * source_N +
	sizeof(std::forward_iterator_tag);
template <class Input> constexpr size_t complete_recipe_byte_assign_own_source =
	source_P + 2 * sizeof(Input);
template <class Input> constexpr size_t complete_recipe_byte_fresh_assign_aux_own_source =
	// this, first/last, forward tag, len and positive-length tmp. Fresh candidate
	// has size/capacity0; positive fitting mid/n/advance branch is unreachable.
	2 * source_P + 2 * sizeof(Input) + source_N + sizeof(std::forward_iterator_tag);
template <class Input> constexpr size_t complete_recipe_byte_allocate_and_copy_own_source =
	// this/count/first/last, result pointer and returned pointer.
	3 * source_P + 2 * sizeof(Input) + source_N;
constexpr size_t complete_recipe_byte_erase_at_end_own_source = 2 * source_P + source_N;
constexpr size_t complete_recipe_byte_cbegin_own_source =
	source_P + sizeof(std::vector<uint8_t>::const_iterator);
constexpr size_t complete_recipe_byte_const_iterator_conversion_source =
	// Actual explicitly defined normal-iterator conversion this/source ref;
	// source.base's receiver/reference return remains a distinct lower scope.
	2 * source_P;
template <class Iterator> constexpr size_t complete_recipe_normal_equal_own_source =
	// != rewrites the actual C++20 ==; no independent != function invented.
	2 * source_P + source_B;
template <class Iterator> constexpr size_t complete_recipe_zero_advance_own_source =
	// Actual input iterator-reference, size_t argument and ptrdiff local d.
	source_P + source_N + sizeof(std::ptrdiff_t);
template <class Iterator> constexpr size_t complete_recipe_ra_advance_own_source =
	source_P + sizeof(std::ptrdiff_t) + sizeof(std::random_access_iterator_tag);
template <class Iterator> constexpr size_t complete_recipe_normal_plus_equal_source =
	2 * source_P + sizeof(std::ptrdiff_t);
#endif
} // namespace duris_craft_recipe_bounded_detail

// Actual GNU13 material-name iterator is normal_iterator<const char*,string>.
// Raw const-char input exists only after its genuine niter/base strip.
namespace duris_craft_recipe_bounded_detail
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
using complete_recipe_name_iterator = std::string::const_iterator;
constexpr size_t complete_recipe_name_begin_own_source =
	// this and actual returned normal iterator. _M_data() pointer prvalue,
	// converted to const pointer, materializes at constructor const pointer&.
	2 * source_P + sizeof(complete_recipe_name_iterator);
constexpr size_t complete_recipe_name_end_own_source = complete_recipe_name_begin_own_source;
template <class T> constexpr size_t complete_recipe_vector_const_begin_own_source =
	// Pointer field lvalue is reference-compatible with constructor const
	// pointer reference; this is not the string's pointer-return prvalue.
	source_P + sizeof(typename std::vector<T>::const_iterator);
template <class T> constexpr size_t complete_recipe_vector_const_end_own_source =
	complete_recipe_vector_const_begin_own_source<T>;
template <class T> constexpr size_t complete_recipe_vector_empty_own_source = source_P + source_B;
template <class Iterator> constexpr size_t complete_recipe_normal_preincrement_source =
	2 * source_P;
// K participates in original name insert public/forward-range formals,
// iterator-category, distance/subtract/base, != rewritten==, advance(mid,0),
// normal +=, generic miter and normal niter before scalar char-to-byte copy.
// Therefore instantiate those actual typed rows with K, never const char*.
#endif
} // namespace duris_craft_recipe_bounded_detail

// Selected typed Source call graphs. These expressions conservatively retain
// distinct actual call occurrences; they never borrow unrelated old slots.
// A function/type/specialization is shared only with a named no-overlap proof.
// Complete entry union and runtime frame binding remain separate below.
namespace duris_craft_recipe_bounded_detail
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
using complete_recipe_byte_const_iterator = std::vector<uint8_t>::const_iterator;
using complete_recipe_byte_move_iterator = std::move_iterator<uint8_t *>;

// Mandatory class-prvalue result identities are projected at the exact
// call site. This is not optional NRVO or a subtraction from spare margins.
template <class Input, class Output> constexpr size_t complete_recipe_copy_projected_source =
	2 * sizeof(Input) + sizeof(Output) + (std::is_pointer_v<Output> ? sizeof(Output) : 0);
template <class Input> constexpr size_t complete_recipe_miter_argument_source =
	// A class result directly initializes copy_move_a's by-value Input.
	// Its object is owned by that formal; scalar pointer returns stay distinct.
	sizeof(Input) + (std::is_pointer_v<Input> ? sizeof(Input) : 0);
template <class Iterator> constexpr size_t complete_recipe_normal_offset_argument_source =
	// this, n and real pointer-arithmetic prvalue at ctor const pointer&.
	// Returned class directly initializes the named caller formal/result owner.
	2 * source_P + sizeof(typename std::iterator_traits<Iterator>::difference_type);

template <class Iterator> constexpr size_t complete_recipe_normal_distance_graph_source =
	complete_recipe_distance_own_source<Iterator> +
	complete_recipe_ra_distance_own_source<Iterator> +
	// Category RA prvalue is the exact __distance by-value tag object.
	source_P + complete_recipe_normal_subtract_own_source<Iterator> +
	2 * complete_recipe_normal_base_source<Iterator>;

template <class Iterator> constexpr size_t complete_recipe_normal_advance_graph_source =
	complete_recipe_zero_advance_own_source<Iterator> +
	// Category RA prvalue is the exact __advance by-value tag object.
	source_P + complete_recipe_ra_advance_own_source<Iterator> +
	complete_recipe_normal_plus_equal_source<Iterator>;

template <class Iterator> constexpr size_t complete_recipe_niter_graph_source =
	std::is_pointer_v<Iterator> ? complete_recipe_raw_niter_source<
					      typename std::iterator_traits<Iterator>::value_type> :
				      complete_recipe_normal_niter_own_source<Iterator> +
					      complete_recipe_normal_base_source<Iterator>;

// Each copy specialization has actual Input/Output formals and retained
// returned carriers. True byte move and false const-byte copy remain distinct
// instantiations even when their typed expressions have equal numeric sizes.
template <bool Move, class Input, class Output, class RawInput>
constexpr size_t complete_recipe_copy_graph_source =
	complete_recipe_copy_projected_source<Input, Output> +
	// Public copy strips two miter arguments before entering copy_move_a.
	2 * complete_recipe_miter_argument_source<Input> +
	complete_recipe_copy_projected_source<Input, Output> +
	// Actual first/last/output niter calls; all three results are retained
	// together through a1/a2. These are three real declared call occurrences.
	2 * complete_recipe_niter_graph_source<Input> + complete_recipe_niter_graph_source<Output> +
	complete_recipe_raw_copy_a1_source<RawInput, uint8_t *> +
	complete_recipe_raw_copy_a2_source<RawInput, uint8_t *> +
	// Different source element char selects scalar false,false,RA copy.
	(std::is_same_v<RawInput, const char *> ?
		 complete_recipe_char_to_byte_scalar_copy_m_source :
		 complete_recipe_byte_simple_copy_m_source +
			 (Move ? complete_recipe_byte_move_assign_one_source :
				 complete_recipe_byte_assign_one_source)) +
	complete_recipe_std_constant_evaluated_source +
	// Pointer output selects same-type raw wrap. Normal output really calls
	// normal wrap, niter_base(from), operator+(difference) and its constructor.
	(std::is_pointer_v<Output> ?
		 complete_recipe_raw_niter_wrap_source :
		 complete_recipe_normal_niter_wrap_own_source<Output> +
			 complete_recipe_niter_graph_source<Output> +
			 // operator+ result is the same niter_wrap/public copy result.
			 complete_recipe_normal_offset_argument_source<Output> +
			 complete_recipe_normal_pointer_constructor_source<Output>);

// std::copy(M,M,byte*) strips move iterators to raw byte pointers. Its
// copy_move_a<true> therefore has pointer Input, unlike the public copy.
constexpr size_t complete_recipe_byte_move_copy_graph_source =
	complete_recipe_copy_own_source<complete_recipe_byte_move_iterator, uint8_t *> +
	2 * (complete_recipe_move_miter_own_source<uint8_t> +
	     complete_recipe_move_iterator_base_source<uint8_t> +
	     complete_recipe_generic_miter_source<
		     uint8_t *>)+complete_recipe_copy_move_a_own_source<uint8_t *, uint8_t *> +
	3 * complete_recipe_raw_niter_source<uint8_t> +
	complete_recipe_raw_copy_a1_source<uint8_t *, uint8_t *> +
	complete_recipe_raw_copy_a2_source<uint8_t *, uint8_t *> +
	complete_recipe_byte_simple_copy_m_source + complete_recipe_byte_move_assign_one_source +
	complete_recipe_std_constant_evaluated_source + complete_recipe_raw_niter_wrap_source;

template <class Input, class RawInput>
constexpr size_t complete_recipe_uninitialized_copy_graph_source =
	complete_recipe_uninitialized_copy_a_source<Input, uint8_t *> +
	complete_recipe_std_internal_constant_evaluated_source +
	complete_recipe_uninitialized_copy_source<Input, uint8_t *> +
	complete_recipe_uninitialized_copy_true_source<Input, uint8_t *> +
	complete_recipe_copy_graph_source<false, Input, uint8_t *, RawInput>;

// make_move_iterator and make_move_if_noexcept_iterator are distinct actual
// makers. Each returns M and constructs M; the ctor std::move(pointer) and
// maker std::move(pointer) are actual separately declared invocation scopes.
constexpr size_t complete_recipe_make_move_iterator_graph_source =
	// Maker M result is the exact uninitialized_copy_a Input formal.
	source_P + complete_recipe_scalar_move_source<uint8_t *> +
	complete_recipe_move_iterator_constructor_source<uint8_t> +
	complete_recipe_scalar_move_source<uint8_t *>;
constexpr size_t complete_recipe_make_move_if_noexcept_iterator_graph_source =
	// Maker M result is the exact uninitialized_copy_a Input formal.
	source_P + complete_recipe_move_iterator_constructor_source<uint8_t> +
	complete_recipe_scalar_move_source<uint8_t *>;

constexpr size_t complete_recipe_uninitialized_move_graph_source =
	complete_recipe_uninitialized_move_own_source<uint8_t> +
	2 * complete_recipe_make_move_iterator_graph_source +
	complete_recipe_uninitialized_copy_a_source<complete_recipe_byte_move_iterator, uint8_t *> +
	complete_recipe_std_internal_constant_evaluated_source +
	complete_recipe_uninitialized_copy_source<complete_recipe_byte_move_iterator, uint8_t *> +
	complete_recipe_uninitialized_copy_true_source<complete_recipe_byte_move_iterator,
						       uint8_t *> +
	complete_recipe_byte_move_copy_graph_source;
constexpr size_t complete_recipe_uninitialized_move_if_noexcept_graph_source =
	complete_recipe_uninitialized_move_if_noexcept_own_source<uint8_t> +
	2 * complete_recipe_make_move_if_noexcept_iterator_graph_source +
	complete_recipe_uninitialized_copy_a_source<complete_recipe_byte_move_iterator, uint8_t *> +
	complete_recipe_std_internal_constant_evaluated_source +
	complete_recipe_uninitialized_copy_source<complete_recipe_byte_move_iterator, uint8_t *> +
	complete_recipe_uninitialized_copy_true_source<complete_recipe_byte_move_iterator,
						       uint8_t *> +
	complete_recipe_byte_move_copy_graph_source;

template <class T> constexpr size_t complete_recipe_vector_destroy_range_source =
	// Allocator-reference wrapper3P, range wrapper2P and trivial aux2P;
	// genuine __is_constant_evaluated is still a named result, no element dtor.
	7 * source_P + complete_recipe_std_internal_constant_evaluated_source;
template <class T> constexpr size_t complete_recipe_vector_deallocate_graph_source =
	// get allocator2P; vector/traits/allocator/newallocator each2P+N;
	// actual sized delete declarationP+N and genuine allocator predicate.
	11 * source_P + 5 * source_N + complete_recipe_std_internal_constant_evaluated_source;

template <class T> constexpr size_t complete_recipe_vector_reserve_graph_source =
	complete_recipe_vector_reserve_own_source<T> + complete_recipe_vector_max_size_source<T> +
	complete_recipe_vector_capacity_source<T> + complete_recipe_vector_size_source<T> +
	complete_recipe_vector_allocate_source<T> +
	complete_recipe_vector_mutable_allocator_source<T> +
	complete_recipe_vector_relocate_source<T> +
	complete_recipe_vector_deallocate_graph_source<T>;

template <class T> constexpr size_t complete_recipe_vector_push_graph_source =
	complete_recipe_vector_push_own_source<T> + complete_recipe_scalar_move_source<T> +
	complete_recipe_vector_emplace_own_source<T> + complete_recipe_scalar_forward_source<T> +
	complete_recipe_vector_single_construct_source<T> +
	complete_recipe_vector_realloc_insert_own_source<T> +
	complete_recipe_vector_check_len_own_source<T> + complete_recipe_vector_max_size_source<T> +
	complete_recipe_vector_size_source<T> + complete_recipe_size_max_source +
	complete_recipe_vector_allocate_source<T> +
	complete_recipe_vector_mutable_allocator_source<T> +
	complete_recipe_vector_relocate_source<T> +
	complete_recipe_vector_deallocate_graph_source<T> +
	// Actual emplace end()/position base(), and back()->end()->operator-.
	// end() argument to realloc_insert is the same position formal I.
	source_P +
	// back() end() result is a separate implicit-object operand to minus.
	source_P + sizeof(typename std::vector<T>::iterator) +
	2 * complete_recipe_normal_pointer_constructor_source<typename std::vector<T>::iterator> +
	complete_recipe_normal_base_source<typename std::vector<T>::iterator> +
	complete_recipe_vector_back_own_source<T> +
	complete_recipe_normal_offset_own_source<typename std::vector<T>::iterator> +
	complete_recipe_normal_pointer_constructor_source<typename std::vector<T>::iterator> +
	complete_recipe_normal_dereference_source<typename std::vector<T>::iterator>;

template <class Input, class RawInput>
constexpr size_t complete_recipe_byte_end_insert_graph_source =
	complete_recipe_byte_insert_own_source<Input> +
	complete_recipe_byte_range_insert_own_source<Input> +
	complete_recipe_byte_const_iterator_conversion_source +
	complete_recipe_normal_base_source<complete_recipe_wire_iterator> +
	complete_recipe_byte_cbegin_own_source +
	complete_recipe_normal_pointer_constructor_source<complete_recipe_byte_const_iterator> +
	complete_recipe_normal_subtract_own_source<complete_recipe_byte_const_iterator> +
	2 * complete_recipe_normal_base_source<complete_recipe_byte_const_iterator> +
	// Both public begin()+offset calls own real begin implicit-object
	// operands. Their offset results are respectively range position I
	// and the public returned I, already owned by the two actual wrappers.
	2 * (complete_recipe_wire_begin_own_source +
	     complete_recipe_normal_pointer_constructor_source<complete_recipe_wire_iterator> +
	     complete_recipe_normal_offset_argument_source<complete_recipe_wire_iterator> +
	     complete_recipe_normal_pointer_constructor_source<
		     complete_recipe_wire_iterator>)+complete_recipe_iterator_category_source<Input> +
	complete_recipe_normal_equal_own_source<Input> +
	2 * complete_recipe_normal_base_source<Input> +
	complete_recipe_normal_distance_graph_source<Input> +
	// Fitting end insert calls advance(mid,0), still real declared wrappers;
	// the old byte move range is empty but its wrappers are reached.
	complete_recipe_normal_advance_graph_source<Input> +
	complete_recipe_uninitialized_copy_graph_source<Input, RawInput> +
	complete_recipe_uninitialized_move_graph_source +
	complete_recipe_copy_graph_source<false, Input, complete_recipe_wire_iterator, RawInput> +
	// Growing branch has two sequential distinct move-if-noexcept calls;
	// retaining each genuine occurrence is conservative, not object-count fiction.
	2 * complete_recipe_uninitialized_move_if_noexcept_graph_source +
	complete_recipe_vector_check_len_own_source<uint8_t> +
	complete_recipe_vector_max_size_source<uint8_t> +
	complete_recipe_vector_size_source<uint8_t> + complete_recipe_size_max_source +
	complete_recipe_vector_allocate_source<uint8_t> +
	complete_recipe_vector_mutable_allocator_source<uint8_t> +
	complete_recipe_vector_destroy_range_source<uint8_t> +
	complete_recipe_vector_deallocate_graph_source<uint8_t> +
	// Internal fitting end subtraction, three position.base() occurrences;
	// caller end argument is separately priced by its actual entry expression.
	complete_recipe_wire_end_own_source +
	complete_recipe_normal_pointer_constructor_source<complete_recipe_wire_iterator> +
	complete_recipe_normal_subtract_own_source<complete_recipe_wire_iterator> +
	5 * complete_recipe_normal_base_source<complete_recipe_wire_iterator>;

constexpr size_t complete_recipe_byte_fresh_span_assign_graph_source =
	complete_recipe_byte_assign_own_source<complete_recipe_input_iterator> +
	complete_recipe_byte_fresh_assign_aux_own_source<complete_recipe_input_iterator> +
	complete_recipe_iterator_category_source<complete_recipe_input_iterator> +
	complete_recipe_normal_distance_graph_source<complete_recipe_input_iterator> +
	complete_recipe_byte_check_init_len_source + complete_recipe_allocator_const_copy_source +
	complete_recipe_allocator_cleanup_source + complete_recipe_vector_max_size_source<uint8_t> +
	complete_recipe_byte_allocate_and_copy_own_source<complete_recipe_input_iterator> +
	complete_recipe_vector_allocate_source<uint8_t> +
	complete_recipe_vector_mutable_allocator_source<uint8_t> +
	complete_recipe_uninitialized_copy_graph_source<complete_recipe_input_iterator,
							const uint8_t *> +
	complete_recipe_vector_destroy_range_source<uint8_t> +
	complete_recipe_vector_deallocate_graph_source<uint8_t> +
	// len==0 calls std::copy(J,J,byte*) and erase at end on a fresh vector.
	complete_recipe_copy_graph_source<false, complete_recipe_input_iterator, uint8_t *,
					  const uint8_t *> +
	complete_recipe_byte_erase_at_end_own_source +
	complete_recipe_vector_destroy_range_source<uint8_t>;
#endif
} // namespace duris_craft_recipe_bounded_detail

// Complete selected entry Source assembly. This candidate remains pending
// independent whole-graph review; the original APIs above remain byte-exact.
namespace duris_craft_recipe_bounded_detail
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
constexpr size_t complete_recipe_callback_interface_source = source_P + source_N + source_B;
// Only the declared callback bytes/context/result interface is owned here.
// Actual caller callback body and borrowed input/output heaps remain outer-owned.
constexpr size_t complete_recipe_budget_common_source =
	budget_add_own_source + complete_recipe_budget_prefix_own_source +
	complete_recipe_budget_peak_own_source + complete_recipe_budget_heap_own_source<uint8_t> +
	complete_recipe_vector_capacity_source<uint8_t> + complete_recipe_callback_interface_source;
// Encoder candidate stays nullptr for its complete lifetime. Only decoder
// sets candidate, so these observer descendants belong to decoder-own alone.
constexpr size_t complete_recipe_budget_candidate_source =
	complete_recipe_budget_heap_own_source<uint64_t> +
	complete_recipe_vector_capacity_source<uint64_t> +
	complete_recipe_string_capacity_own_source + member_string_move_const_locality_source;
constexpr size_t complete_recipe_byte_growth_source =
	complete_recipe_budget_growth_own_source<uint8_t> +
	complete_recipe_vector_max_size_source<uint8_t> +
	complete_recipe_vector_size_source<uint8_t> +
	complete_recipe_vector_capacity_source<uint8_t> + complete_recipe_size_max_source;
constexpr size_t complete_recipe_u64_growth_source =
	complete_recipe_budget_growth_own_source<uint64_t> +
	complete_recipe_vector_max_size_source<uint64_t> +
	complete_recipe_vector_size_source<uint64_t> +
	complete_recipe_vector_capacity_source<uint64_t> + complete_recipe_size_max_source;
constexpr size_t complete_recipe_pure_selected_graph_source =
	complete_recipe_refine_succeeded_own_source + complete_recipe_refine_cost_valid_own_source +
	complete_recipe_refine_terms_valid_own_source + complete_recipe_alchemy_own_source +
	complete_recipe_refine_cost_compare_own_source +
	complete_recipe_array_const_i32_access_source +
	complete_recipe_array_const_i64_access_source + complete_recipe_array_i32_equal_source +
	complete_recipe_vector_empty_own_source<uint8_t> +
	complete_recipe_vector_size_source<uint64_t> +
	complete_recipe_vector_const_subscript_source<uint64_t> +
	complete_recipe_vector_empty_own_source<uint64_t> + complete_recipe_string_size_source +
	complete_recipe_string_empty_source + complete_recipe_string_find_own_source +
	complete_recipe_traits_char_find_source;

constexpr size_t complete_recipe_byte_fill_graph_source =
	complete_recipe_byte_fill_constructor_own_source +
	complete_recipe_byte_check_init_len_source + complete_recipe_allocator_const_copy_source +
	complete_recipe_allocator_cleanup_source + complete_recipe_vector_max_size_source<uint8_t> +
	complete_recipe_byte_fill_base_source +
	complete_recipe_byte_fill_allocator_temporary_source +
	complete_recipe_vector_allocate_source<uint8_t> +
	complete_recipe_vector_mutable_allocator_source<uint8_t> +
	complete_recipe_byte_fill_initialize_source +
	complete_recipe_byte_uninitialized_fill_a_source +
	complete_recipe_byte_uninitialized_fill_source +
	complete_recipe_byte_uninitialized_fill_true_source + complete_recipe_byte_fill_n_source +
	complete_recipe_size_to_integer_source + complete_recipe_byte_fill_n_a_source +
	// Category RA return directly initializes fill_n_a's already-owned tag.
	source_P + complete_recipe_byte_fill_a_source + complete_recipe_byte_fill_a1_source +
	complete_recipe_std_constant_evaluated_source +
	complete_recipe_std_internal_constant_evaluated_source;

template <class T> constexpr size_t complete_recipe_vector_move_selected_source =
	// Actual vector operator/source/result3P, declared __move_storage B,
	// internal std::move<vector>2P and _M_move_assign2P/by-value tag.
	// Policy helper solely initializes constexpr __move_storage: no new runtime
	// callee result is introduced; old original allowances remain unchanged.
	7 * source_P + source_B + sizeof(std::true_type) +
	// Real retained temporary V and returned A, get_allocator/const Tp getter.
	sizeof(std::vector<T>) + sizeof(std::allocator<T>) + 3 * source_P +
	// Selected const-A constructor/copy/data graph, actual swap/data locals,
	// alloc_on_move/generated assignments and exact temporary cleanup.
	11 * source_P + 7 * source_P + 10 * source_P +
	// Both mutable allocator getters precede alloc_on_move; two returned
	// references remain together through it. Price both genuine call scopes
	// explicitly instead of borrowing a Source lane from allocation/cleanup.
	2 * complete_recipe_vector_mutable_allocator_source<T> + member_vector_cleanup_source<T>();

constexpr size_t complete_recipe_encode_input_iterators_source =
	// Range begin/end CI results ARE the two hidden iterator locals already
	// owned by encode_loops. Getter receiver and genuine ctor scopes remain.
	2 * (source_P +
	     complete_recipe_normal_pointer_constructor_source<
		     std::vector<uint64_t>::
			     const_iterator>)+complete_recipe_normal_equal_own_source<std::vector<uint64_t>::
											      const_iterator> +
	2 * complete_recipe_normal_base_source<std::vector<uint64_t>::const_iterator> +
	complete_recipe_normal_preincrement_source<std::vector<uint64_t>::const_iterator> +
	complete_recipe_normal_dereference_source<std::vector<uint64_t>::const_iterator> +
	// K/C begin/end prvalues initialize actual insert by-value Input formals.
	// String pointer-return qualification materializations remain distinct.
	2 *
		(2 * source_P + complete_recipe_string_const_data_source +
		 complete_recipe_normal_pointer_constructor_source<complete_recipe_name_iterator>)+2 *
		(source_P + complete_recipe_normal_pointer_constructor_source<
				    complete_recipe_byte_const_iterator>)+
		// Both byte writer caller end() I arguments remain distinct temporaries
		// through I-to-CI conversion and public insert. They are not CI parameters.
		2 *
		(complete_recipe_wire_end_own_source +
		 complete_recipe_normal_pointer_constructor_source<complete_recipe_wire_iterator>);

constexpr size_t complete_recipe_encoder_retained_source =
	complete_recipe_encode_wrapper_source + complete_recipe_encode_loops_source +
	complete_recipe_put_operator_source + complete_recipe_number_operator_source +
	complete_recipe_budget_common_source + complete_recipe_byte_growth_source +
	complete_recipe_budget_fresh_own_source + complete_recipe_pure_selected_graph_source +
	complete_recipe_byte_fill_graph_source +
	complete_recipe_vector_reserve_graph_source<uint8_t> +
	complete_recipe_vector_push_graph_source<uint8_t> +
	complete_recipe_byte_push_argument_source +
	complete_recipe_vector_mutable_subscript_source<uint8_t> +
	complete_recipe_vector_size_source<uint8_t> +
	complete_recipe_byte_end_insert_graph_source<complete_recipe_name_iterator, const char *> +
	complete_recipe_byte_end_insert_graph_source<complete_recipe_byte_const_iterator,
						     const uint8_t *> +
	complete_recipe_encode_input_iterators_source +
	// Ordinary byte-vector default chain6P; actual moved local cleanup and
	// the move helper's distinct retained temporary cleanup are both reached.
	6 * source_P + complete_recipe_vector_move_selected_source<uint8_t> +
	member_vector_cleanup_source<uint8_t>() +
	complete_recipe_scalar_move_source<std::vector<uint8_t>>;

constexpr size_t complete_recipe_fresh_string_graph_source =
	complete_recipe_budget_text_own_source + complete_recipe_string_capacity_own_source +
	complete_recipe_string_max_size_source + complete_recipe_string_assign_entry_source +
	complete_recipe_string_replace_own_source + complete_recipe_string_check_length_source +
	complete_recipe_string_disjunct_source + 2 * complete_recipe_string_less_source +
	complete_recipe_string_mutate_own_source + complete_recipe_string_create_own_source +
	complete_recipe_string_char_allocation_source +
	complete_recipe_string_mutable_allocator_source + complete_recipe_string_copy_source +
	complete_recipe_string_data_capacity_setters_source +
	// Actual size/data/locality/length/traits-assign wrappers are contained in
	// reached default/move/cleanup lanes below; identical typed calls return
	// before fresh assign and cannot reenter them or call a reserve callback.
	member_string_default_source + member_string_move_assignment_source +
	// String move allocator graph already names one mutable getter2P.
	// The second actual returned reference coexists at alloc_on_move; own
	// that second call explicitly rather than rely on fresh allocation credit.
	complete_recipe_string_mutable_allocator_source + member_string_destroy_source;

constexpr size_t complete_recipe_canonical_equal_graph_source =
	complete_recipe_equal4_entry_source + complete_recipe_equal4_helper_source +
	complete_recipe_normal_distance_graph_source<complete_recipe_wire_iterator> +
	complete_recipe_normal_distance_graph_source<complete_recipe_input_iterator> +
	complete_recipe_equal3_entry_source + complete_recipe_equal_aux_own_source +
	2 * complete_recipe_niter_graph_source<complete_recipe_wire_iterator> +
	complete_recipe_niter_graph_source<complete_recipe_input_iterator> +
	complete_recipe_byte_equal_aux1_source + complete_recipe_byte_equal_true_source +
	complete_recipe_byte_memcmp_source +
	// Four caller iterator results directly initialize equal4's four formal
	// class objects. No second result I/J is created at their getters.
	2 *
		(source_P +
		 complete_recipe_normal_pointer_constructor_source<complete_recipe_wire_iterator>)+2 *
		(source_P +
		 complete_recipe_normal_pointer_constructor_source<complete_recipe_input_iterator>)+
		// Actual span end pointer addition prvalue and size/extent child scopes.
		source_P +
	complete_recipe_span_size_source + complete_recipe_span_extent_source;

constexpr size_t complete_recipe_decoder_retained_source =
	// The two old nested locals move into the real preentry adapter below.
	// Actual wrapper Source is span +5P+5N+3U+B+notch-temp+read capture.
	sizeof(std::span<const uint8_t>) + 5 * source_P + 5 * source_N + 3 * sizeof(uint64_t) +
	source_B + sizeof(skill_notch_outcome) + sizeof(complete_recipe_read) +
	complete_recipe_read_operator_source + complete_recipe_budget_common_source +
	complete_recipe_budget_candidate_source + complete_recipe_byte_growth_source +
	complete_recipe_u64_growth_source + complete_recipe_alchemy_own_source +
	complete_recipe_encoder_preentry_adapter_source + complete_recipe_span_size_source +
	complete_recipe_span_extent_source + complete_recipe_span_data_source +
	complete_recipe_span_subscript_source + complete_recipe_array_mutable_i32_access_source +
	complete_recipe_vector_push_graph_source<uint64_t> +
	complete_recipe_byte_fresh_span_assign_graph_source +
	// Fresh assign caller J begin()+header/end results ARE its by-value
	// first/last parameters; begin's separate implicit-object J stays live
	// through operator+. Actual pointer-arithmetic ctor prvalues stay owned.
	complete_recipe_span_begin_own_source +
	complete_recipe_normal_pointer_constructor_source<complete_recipe_input_iterator> +
	complete_recipe_normal_offset_argument_source<complete_recipe_input_iterator> +
	complete_recipe_normal_pointer_constructor_source<complete_recipe_input_iterator> +
	source_P + source_P + complete_recipe_span_size_source +
	complete_recipe_span_extent_source +
	complete_recipe_normal_pointer_constructor_source<complete_recipe_input_iterator> +
	complete_recipe_fresh_string_graph_source + complete_recipe_canonical_equal_graph_source +
	// Exact candidate byte/u64 default, selected member moves and cleanup.
	2 * (6 * source_P) + complete_recipe_vector_move_selected_source<uint8_t> +
	complete_recipe_vector_move_selected_source<uint64_t> +
	member_vector_cleanup_source<uint8_t>() + member_vector_cleanup_source<uint64_t>() +
	// Genuine recipe/cost DMI default2P, nontrivial recipe assignment3P,
	// and recipe cleanupP. Trivial notch/wallet/array assignments have no
	// declared callable body; physical members/notch temporary stay owned.
	2 * source_P + 3 * source_P + source_P +
	complete_recipe_scalar_move_source<craft_recipe_continuation>;

// Public prospective decode Source includes the actual encoder overlap.
// The decoder controller retains decoder-own only; the nested encoder then
// owns its complete Source exactly once through its own controller.
constexpr size_t complete_recipe_encode_full_source = complete_recipe_encoder_retained_source;
constexpr size_t complete_recipe_decode_full_source =
	complete_recipe_decoder_retained_source + complete_recipe_encode_full_source;
#endif
} // namespace duris_craft_recipe_bounded_detail
inline bool craft_recipe_continuation_encode_complete_bounded(
	const craft_recipe_continuation &terms, std::vector<uint8_t> *encoded,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
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
	constexpr size_t frames =
		sizeof(std::vector<uint8_t>) + complete_recipe_encoder_retained_source;
	complete_recipe_budget owner{ reserve, context, outer, frames };
	if (!owner.peak(0))
		return false;

	if (terms.discipline == craft_recipe_discipline::refine)
	{
		if (!encoded || !craft_refine_terms_valid(terms))
			return false;
		try
		{
			if (!owner.fresh<uint8_t>(128))
				return false;
			std::vector<uint8_t> bytes(128, 0);
			owner.bytes = &bytes;
			complete_recipe_put put{ bytes };
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
			if (!owner.peak(0))
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
		if (!owner.peak(0))
			return false;
		std::vector<uint8_t> bytes;
		owner.bytes = &bytes;
		if (!owner.growth(bytes, CRAFT_RECIPE_CONTINUATION_HEADER_BYTES +
						 terms.pouch_mutation.size()))
			return false;
		bytes.reserve(CRAFT_RECIPE_CONTINUATION_HEADER_BYTES + terms.pouch_mutation.size());
		complete_recipe_number number{ owner, bytes };
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
		if (!owner.peak(0))
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

inline bool craft_recipe_continuation_decode_complete_bounded(
	std::span<const uint8_t> bytes, craft_recipe_continuation *terms,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
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
				  complete_recipe_decoder_retained_source;
	complete_recipe_budget owner{ reserve, context, outer, frames };
	if (!owner.peak(0))
		return false;

	if (!terms || bytes.size() < CRAFT_RECIPE_CONTINUATION_HEADER_BYTES ||
	    bytes.size() > CRAFT_RECIPE_CONTINUATION_MAX_BYTES)
		return false;
	complete_recipe_read number{ bytes };
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
			if (!complete_recipe_encode_child<complete_recipe_encode_full_source>(
				    owner, candidate, &canonical, reserve, context) ||
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
		if (!complete_recipe_encode_child<complete_recipe_encode_full_source>(
			    owner, candidate, &canonical, reserve, context) ||
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

inline bool craft_recipe_continuation_encode_complete_source_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	using namespace duris_craft_recipe_bounded_detail;
	*bytes = complete_recipe_encode_full_source;
	return true;
#else
	return false;
#endif
}

inline bool
craft_recipe_continuation_encode_complete_source_supplement_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	using namespace duris_craft_recipe_bounded_detail;
	*bytes = 0;
	return true;
#else
	return false;
#endif
}

inline bool craft_recipe_continuation_encode_complete_initial_inline_bytes(size_t *bytes) noexcept
{
	if (!bytes)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	using namespace duris_craft_recipe_bounded_detail;
	*bytes = sizeof(duris_craft_recipe_bounded_detail::complete_recipe_budget);
	return true;
#else
	return false;
#endif
}

constexpr size_t craft_recipe_continuation_encode_complete_source_query_frame_bytes() noexcept
{
	// Each actual scalar getter output pointer/returned Boolean only;
	// Source and initial result size_t objects are owned by its caller.
	return sizeof(size_t *) + sizeof(bool);
}

inline bool craft_recipe_continuation_decode_complete_source_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	using namespace duris_craft_recipe_bounded_detail;
	*bytes = complete_recipe_decode_full_source;
	return true;
#else
	return false;
#endif
}

inline bool
craft_recipe_continuation_decode_complete_source_supplement_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	using namespace duris_craft_recipe_bounded_detail;
	*bytes = 0;
	return true;
#else
	return false;
#endif
}

inline bool craft_recipe_continuation_decode_complete_initial_inline_bytes(size_t *bytes) noexcept
{
	if (!bytes)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	using namespace duris_craft_recipe_bounded_detail;
	*bytes = sizeof(duris_craft_recipe_bounded_detail::complete_recipe_budget);
	return true;
#else
	return false;
#endif
}

constexpr size_t craft_recipe_continuation_decode_complete_source_query_frame_bytes() noexcept
{
	// Each actual scalar getter output pointer/returned Boolean only;
	// Source and initial result size_t objects are owned by its caller.
	return sizeof(size_t *) + sizeof(bool);
}
#endif
