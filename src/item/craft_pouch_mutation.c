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
#include <initializer_list>
namespace
{
using pouch_wire_reserve_fn = bool (*)(size_t, void *) noexcept;
bool pouch_wire_add(size_t &bytes, size_t extra) noexcept
{
	if (extra > SIZE_MAX - bytes)
		return false;
	bytes += extra;
	return true;
}
constexpr size_t pouch_wire_allocator_frames =
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
constexpr size_t pouch_wire_copy_frames =
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
constexpr size_t pouch_wire_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t pouch_wire_default_frames =
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
constexpr size_t pouch_wire_vector_frames =
	pouch_wire_allocator_frames + pouch_wire_copy_frames + pouch_wire_relocate_frames +
	pouch_wire_default_frames +
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
constexpr size_t pouch_wire_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + pouch_wire_allocator_frames;

// Genuine default vector/base/impl/data and allocator/new_allocator this
// carriers. The vector object's fields are already included in its owner.
constexpr size_t pouch_wire_empty_constructor_frames = 6 * sizeof(void *);
// vector(n,a), actual default allocator temporary, _S_check_init_len n/a/return
// and allocator copy, _Vector_base this/n/a, _Vector_impl this/a/copy,
// _Vector_impl_data this and _M_create_storage this/n. The existing vector
// allocator profile owns _S_max_size and allocation scopes.
constexpr size_t pouch_wire_size_constructor_frames =
	2 * sizeof(void *) + sizeof(size_t) + sizeof(std::allocator<uint8_t>) + sizeof(void *) +
	2 * sizeof(size_t) + sizeof(std::allocator<uint8_t>) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) +
	sizeof(size_t);

constexpr size_t pouch_wire_pure_frames =
	// Original valid_usage/number/append parameters, index/offset/value,
	// results; span constructor/query/subscript and vector size/element scopes.
	7 * sizeof(void *) + 8 * sizeof(size_t) + 3 * sizeof(uint64_t) + sizeof(bool) +
	sizeof(std::span<const uint8_t>) + sizeof(std::span<const chaos_material_pouch_usage>) +
	12 * (sizeof(void *) + sizeof(size_t));
constexpr size_t pouch_wire_equal_frames =
	// Four-iterator equal/__equal4, both actual length differences and nested
	// equal/aux/aux1/equal<true>/memcmp plus iterator/reference queries.
	2 * (4 * sizeof(void *) + sizeof(bool)) + 2 * sizeof(ptrdiff_t) +
	4 * (3 * sizeof(void *) + sizeof(bool)) + 8 * (2 * sizeof(void *)) + sizeof(size_t) +
	2 * (2 * sizeof(void *) + sizeof(size_t) + sizeof(int));
// Complete actual singleton initializer/vector range construction. This is
// distinct from size-constructor and reserve profiles already present.
constexpr size_t pouch_wire_initializer_frames =
	// initializer_list private ctor this/array/len; begin/end/size queries
	// and their returned pointer/size values on the original vector ctor.
	2 * sizeof(void *) + sizeof(size_t) + 3 * (sizeof(void *) + sizeof(void *)) +
	2 * sizeof(size_t) +
	// vector(list,a) this/list-by-value/allocator reference/default allocator;
	// _Vector_base(a), Impl(a), Impl_data this and allocator copy.
	2 * sizeof(void *) + sizeof(std::initializer_list<player_item_snapshot>) +
	2 * sizeof(std::allocator<player_item_snapshot>) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(void *) +
	// _M_range_initialize this/first/last/forward tag and real n;
	// _S_check_init_len n/a/result/allocator copy (max_size in old profile).
	3 * sizeof(void *) + sizeof(std::forward_iterator_tag) + sizeof(size_t) + sizeof(void *) +
	2 * sizeof(size_t) + sizeof(std::allocator<player_item_snapshot>) + 2 * sizeof(void *) +
	// Extra OUTER row __do_uninit_copy this-free first/last/result/cur/return,
	// _Construct(location,source)/addressof/forward/placement-new closure.
	// The independent INNER description loop remains in shared row profile.
	5 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(size_t);
struct pouch_wire_budget
{
	pouch_wire_reserve_fn reserve;
	void *context;
	size_t outer;
	size_t frames;
	const craft_pouch_mutation *candidate = nullptr;
	const std::vector<player_item_snapshot> *before = nullptr, *after = nullptr;
	const std::vector<uint8_t> *bytes = nullptr, *before_bytes = nullptr,
				   *after_bytes = nullptr;
	template <typename T> bool heap(const std::vector<T> &value, size_t &total) const noexcept
	{
		return value.capacity() <= SIZE_MAX / sizeof(T) &&
		       pouch_wire_add(total, value.capacity() * sizeof(T));
	}
	bool item(const player_item_snapshot &value, size_t &total) const noexcept
	{
		size_t actual = 0;
		return player_item_snapshot_current_heap_bytes(value, &actual) &&
		       pouch_wire_add(total, actual);
	}
	bool items(const std::vector<player_item_snapshot> &value, size_t &total) const noexcept
	{
		if (!heap(value, total))
			return false;
		for (const auto &row : value)
			if (!item(row, total))
				return false;
		return true;
	}
	bool prefix(size_t &output, size_t extra = 0) const noexcept
	{
		size_t total = outer;
		constexpr size_t observation = 15 * sizeof(void *) + 8 * sizeof(size_t) +
					       6 * sizeof(bool) +
					       8 * (sizeof(void *) + sizeof(size_t));
		if (!pouch_wire_add(total, sizeof(*this)) || !pouch_wire_add(total, frames) ||
		    (candidate &&
		     (!heap(candidate->usage, total) || !item(candidate->before, total) ||
		      !item(candidate->after, total))) ||
		    (before && !items(*before, total)) || (after && !items(*after, total)) ||
		    (bytes && !heap(*bytes, total)) ||
		    (before_bytes && !heap(*before_bytes, total)) ||
		    (after_bytes && !heap(*after_bytes, total)) ||
		    !pouch_wire_add(total, observation) ||
		    !pouch_wire_add(total, player_item_snapshot_copy_frame_bytes()) ||
		    !pouch_wire_add(total, extra))
			return false;
		output = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
	template <typename T>
	bool fresh(size_t count, size_t extra = pouch_wire_vector_frames) const noexcept
	{
		constexpr size_t own = sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool);
		size_t request = extra;
		return count <= SIZE_MAX / sizeof(T) &&
		       pouch_wire_add(request, count * sizeof(T)) && pouch_wire_add(request, own) &&
		       peak(request);
	}
	template <typename T> bool growth(const std::vector<T> &value, size_t count,
					  size_t caller_frames = 0) const noexcept
	{
		size_t request = pouch_wire_vector_frames;
		constexpr size_t own = 2 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool);
		if (count > value.max_size() - value.size())
			return false;
		if (value.size() + count > value.capacity())
		{
			size_t next = value.size();
			if (!pouch_wire_add(next, std::max(value.size(), count)) ||
			    next > value.max_size())
				next = value.max_size();
			if (next > SIZE_MAX / sizeof(T) ||
			    !pouch_wire_add(request, next * sizeof(T)))
				return false;
		}
		return pouch_wire_add(request, own) && pouch_wire_add(request, caller_frames) &&
		       peak(request);
	}
};
bool pouch_wire_append(pouch_wire_budget &owner, std::vector<uint8_t> &out, uint64_t value,
		       size_t size)
{
	constexpr size_t own = 2 * sizeof(void *) + sizeof(uint64_t) + 2 * sizeof(size_t) +
			       sizeof(bool) + sizeof(uint8_t);
	for (size_t index = 0; index < size; ++index)
	{
		if (!owner.growth(out, 1, own))
			return false;
		out.push_back(static_cast<uint8_t>(value >> (index * 8)));
	}
	return true;
}
player_snapshot_codec_result pouch_wire_singleton(const player_item_snapshot &source,
						  std::vector<uint8_t> *output,
						  pouch_wire_reserve_fn reserve, void *context,
						  size_t outer)
{
	// The original encode({source}) creates two distinct row copies: actual
	// const initializer-list backing, then its actual one-row vector element.
	constexpr size_t frame = sizeof(std::initializer_list<player_item_snapshot>) +
				 sizeof(std::vector<player_item_snapshot>) +
				 2 * sizeof(player_item_snapshot) + 4 * sizeof(void *) +
				 5 * sizeof(size_t) + sizeof(player_snapshot_codec_result) +
				 sizeof(bool) + pouch_wire_vector_frames +
				 pouch_wire_size_constructor_frames + pouch_wire_initializer_frames;
	size_t total = outer, request = 0;
	if (!reserve || !pouch_wire_add(total, frame) ||
	    !pouch_wire_add(total, player_item_snapshot_copy_frame_bytes()) ||
	    !reserve(total, context) ||
	    !player_item_snapshot_fresh_copy_request_bytes(source, &request) ||
	    !pouch_wire_add(total, request) || !pouch_wire_add(total, request) ||
	    !reserve(total, context))
		return player_snapshot_codec_result::allocation_failure;
	const std::initializer_list<player_item_snapshot> singleton{ source };
	const std::vector<player_item_snapshot> items(singleton);
	// The actual vector allocation replaces the prospectively owned inline
	// row term; its fresh element/string/vector requests were preadmitted.
	size_t current = outer, actual = 0;
	if (!pouch_wire_add(current, frame - sizeof(player_item_snapshot)) ||
	    !pouch_wire_add(current, player_item_snapshot_copy_frame_bytes()) ||
	    items.capacity() > SIZE_MAX / sizeof(player_item_snapshot) ||
	    !pouch_wire_add(current, items.capacity() * sizeof(player_item_snapshot)) ||
	    !player_item_snapshot_current_heap_bytes(*singleton.begin(), &actual) ||
	    !pouch_wire_add(current, actual) ||
	    !player_item_snapshot_current_heap_bytes(items.front(), &actual) ||
	    !pouch_wire_add(current, actual))
		return player_snapshot_codec_result::allocation_failure;
	return player_item_snapshot_list_encode_bounded(items, output, reserve, context, current);
}
} // namespace
bool craft_pouch_mutation_encode_bounded(const craft_pouch_mutation &mutation,
					 std::vector<uint8_t> *encoded,
					 bool (*reserve)(size_t, void *) noexcept, void *context,
					 size_t outer) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)reserve;
	(void)context;
	(void)outer;
	return false;
#else

	constexpr size_t frames = 3 * sizeof(std::vector<uint8_t>) + 4 * sizeof(void *) +
				  4 * sizeof(size_t) + sizeof(bool) + pouch_wire_pure_frames +
				  3 * pouch_wire_empty_constructor_frames + pouch_wire_move_frames;
	pouch_wire_budget owner{ reserve, context, outer, frames };
	size_t prefix = 0;
	if (!owner.peak() || !owner.prefix(prefix))
		return false;

	if (!encoded || !valid_usage(mutation.usage) ||
	    mutation.before.parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
	    chaos_pouch_ledger_verify_bounded(mutation.before, mutation.after,
					      std::span<const chaos_material_pouch_usage>(
						      mutation.usage.data(), mutation.usage.size()),
					      mutation.mode, reserve, context,
					      prefix) != chaos_pouch_ledger_result::ok)
		return false;
	try
	{
		std::vector<uint8_t> before, after;
		owner.before_bytes = &before;
		owner.after_bytes = &after;
		if (!owner.prefix(prefix))
			return false;
		if (pouch_wire_singleton(mutation.before, &before, reserve, context, prefix) !=
			    player_snapshot_codec_result::ok ||
		    (!owner.prefix(prefix) ?
			     player_snapshot_codec_result::allocation_failure :
			     pouch_wire_singleton(mutation.after, &after, reserve, context,
						  prefix)) != player_snapshot_codec_result::ok ||
		    before.size() > CRAFT_POUCH_MUTATION_MAX_BYTES ||
		    after.size() > CRAFT_POUCH_MUTATION_MAX_BYTES)
			return false;
		const size_t total = header_bytes + mutation.usage.size() * usage_bytes +
				     before.size() + after.size();
		if (total > CRAFT_POUCH_MUTATION_MAX_BYTES)
			return false;
		std::vector<uint8_t> candidate;
		owner.bytes = &candidate;
		if (!owner.fresh<uint8_t>(total))
			return false;
		candidate.reserve(total);
		if (!pouch_wire_append(owner, candidate, version, 4))
			return false;
		if (!pouch_wire_append(owner, candidate, static_cast<uint32_t>(mutation.mode), 4))
			return false;
		if (!pouch_wire_append(owner, candidate, mutation.usage.size(), 4))
			return false;
		if (!pouch_wire_append(owner, candidate, before.size(), 4))
			return false;
		if (!pouch_wire_append(owner, candidate, after.size(), 4))
			return false;
		// Genuine used reference survives each nested append request.
		owner.frames += sizeof(void *);
		for (const auto &used : mutation.usage)
		{
			if (!pouch_wire_append(owner, candidate, static_cast<uint32_t>(used.vnum),
					       4))
				return false;
			if (!pouch_wire_append(owner, candidate, used.count, 8))
				return false;
		}
		owner.frames -= sizeof(void *);
		if (!owner.growth(candidate, before.size()))
			return false;
		candidate.insert(candidate.end(), before.begin(), before.end());
		if (!owner.growth(candidate, after.size()))
			return false;
		candidate.insert(candidate.end(), after.begin(), after.end());
		if (!owner.peak(pouch_wire_move_frames))
			return false;
		*encoded = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}

#endif
}

bool craft_pouch_mutation_decode_bounded(std::span<const uint8_t> encoded,
					 craft_pouch_mutation *mutation,
					 bool (*reserve)(size_t, void *) noexcept, void *context,
					 size_t outer) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)reserve;
	(void)context;
	(void)outer;
	return false;
#else

	constexpr size_t frames =
		sizeof(craft_pouch_mutation) + 2 * sizeof(std::vector<player_item_snapshot>) +
		sizeof(std::vector<uint8_t>) + sizeof(std::span<const uint8_t>) +
		4 * sizeof(void *) + 8 * sizeof(size_t) + 3 * sizeof(uint64_t) +
		sizeof(chaos_material_pouch_usage) + sizeof(bool) + pouch_wire_pure_frames +
		pouch_wire_equal_frames + 3 * pouch_wire_empty_constructor_frames +
		3 * pouch_wire_move_frames;
	pouch_wire_budget owner{ reserve, context, outer, frames };
	size_t prefix = 0;
	if (!owner.peak())
		return false;

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
		owner.candidate = &candidate;
		if (!owner.fresh<chaos_material_pouch_usage>(count))
			return false;
		candidate.mode = static_cast<chaos_pouch_usage_mode>(mode);
		candidate.usage.reserve(count);
		for (size_t index = 0; index < count; ++index)
		{
			const size_t offset = header_bytes + index * usage_bytes;
			const uint64_t vnum = number(encoded, offset, 4);
			if (vnum > INT32_MAX)
				return false;
			if (!owner.growth(candidate.usage, 1))
				return false;
			candidate.usage.push_back(
				{ static_cast<int>(vnum), number(encoded, offset + 4, 8) });
		}
		const size_t before_offset = header_bytes + count * usage_bytes;
		std::vector<player_item_snapshot> before, after;
		owner.before = &before;
		owner.after = &after;
		if (!owner.prefix(prefix))
			return false;
		if (player_item_snapshot_list_decode_bounded(
			    encoded.data() + before_offset, before_size, &before, reserve, context,
			    prefix) != player_snapshot_codec_result::ok ||
		    (!owner.prefix(prefix) ? player_snapshot_codec_result::allocation_failure :
					     player_item_snapshot_list_decode_bounded(
						     encoded.data() + before_offset + before_size,
						     after_size, &after, reserve, context,
						     prefix)) != player_snapshot_codec_result::ok ||
		    before.size() != 1 || after.size() != 1)
			return false;
		if (!owner.peak(player_item_snapshot_copy_frame_bytes()))
			return false;
		candidate.before = std::move(before[0]);
		if (!owner.peak(player_item_snapshot_copy_frame_bytes()))
			return false;
		candidate.after = std::move(after[0]);
		std::vector<uint8_t> canonical;
		owner.bytes = &canonical;
		if (!owner.prefix(prefix))
			return false;
		if (!craft_pouch_mutation_encode_bounded(candidate, &canonical, reserve, context,
							 prefix) ||
		    (!owner.peak(pouch_wire_equal_frames) ||
		     !std::equal(canonical.begin(), canonical.end(), encoded.begin(),
				 encoded.end())))
			return false;
		if (!owner.peak(pouch_wire_move_frames + player_item_snapshot_copy_frame_bytes()))
			return false;
		*mutation = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}

#endif
}
#include <tuple>
namespace
{
using pouch_payload_map = std::map<int, uint64_t>;
using pouch_payload_node = std::_Rb_tree_node<pouch_payload_map::value_type>;
// Genuine allocation-free map lookup scopes: public contains/at/lower_bound,
// tree find/lower_bound and _M_lower_bound; key_compare, less(int), iterator
// construction/dereference/comparison, _S_key/_Select1st and tree queries.
constexpr size_t pouch_payload_lookup_frames =
	3 * 3 * sizeof(void *) + 3 * sizeof(void *) + 5 * sizeof(void *) + 4 * sizeof(void *) +
	5 * 2 * sizeof(void *) + 3 * sizeof(void *) + 3 * sizeof(void *) + sizeof(bool) +
	2 * (sizeof(void *) + sizeof(std::less<int>)) + 2 * sizeof(void *) +
	2 * 2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(bool) + 6 * sizeof(void *) +
	sizeof(size_t);
// Actual operator[](const int&) node construction and insertion chain. The
// default standard allocator requests ONE genuine node only for a missing key.
constexpr size_t pouch_payload_insert_frames =
	pouch_payload_lookup_frames +
	// map::operator[] this/k/i/result + real tuple temporaries/piecewise tag.
	4 * sizeof(void *) + sizeof(std::tuple<const int &>) + sizeof(std::tuple<>) +
	sizeof(std::piecewise_construct_t) +
	// _M_emplace_hint_unique this/position/3 arg refs, actual _Auto_node
	// two-pointer object, returned position pair and returned iterator.
	11 * sizeof(void *) +
	// _Auto_node ctor this/tree/3args; create_node this/3args/tmp/result;
	// construct_node this/node/3args; get_node/allocator/valptr queries.
	5 * sizeof(void *) + 6 * sizeof(void *) + 5 * sizeof(void *) + 3 * 2 * sizeof(void *) +
	// Node allocator/placement-new/construct_at and failed-node destruction;
	// triple piecewise args add real extra references to existing one-value
	// traits construct/construct_at/forward profiles.
	pouch_wire_allocator_frames + 2 * sizeof(void *) + 2 * sizeof(void *) + 4 * sizeof(void *) +
	// pair piecewise constructor this/tag/by-value tuple1/tuple2, delegate
	// this/tuple refs/index tags; get/get_helper/head/head_base, forward;
	// tuple<const int&>/Tuple_impl/Head_base and tuple<> constructors.
	sizeof(void *) + sizeof(std::piecewise_construct_t) + sizeof(std::tuple<const int &>) +
	sizeof(std::tuple<>) + 3 * sizeof(void *) + 2 * sizeof(char) + 4 * 2 * sizeof(void *) +
	2 * sizeof(void *) + 6 * sizeof(void *) + sizeof(void *) +
	// Hint lookup actual pos/before/after/returned pair and fallback unique
	// lookup x/y/j/comp; _Auto_node::_M_insert pair/it/result and insertion.
	8 * sizeof(void *) + 7 * sizeof(void *) + sizeof(bool) + 5 * sizeof(void *) +
	5 * sizeof(void *) + sizeof(bool) +
	// GCC13.3 tree.cc _Rb_tree_insert_and_rebalance: actual bool+3 pointer
	// params, root reference, xpp/y locals; nested rotate x/root/y. No heap.
	sizeof(bool) + 6 * sizeof(void *) + 3 * sizeof(void *) +
	// Actual returned position pair constructors and node guard destructor.
	3 * sizeof(void *) + sizeof(void *);
constexpr size_t pouch_payload_map_lifetime_frames =
	// map/tree/impl/allocator/key_compare/header/_M_reset construction and
	// destruction; drop_node/destroy_node/allocator destroy and sized delete.
	8 * sizeof(void *) + 2 * sizeof(std::allocator<pouch_payload_map::value_type>) +
	sizeof(std::less<int>) + 3 * 2 * sizeof(void *) + pouch_wire_allocator_frames;
// _M_erase source has this, x and local y; each actual recursion follows a
// distinct existing node's right child. Current n+1 is a source-proven finite
// upper bound including terminal empty call, not an opaque guessed stack cap.
bool pouch_payload_erase_frames(size_t nodes, size_t &total) noexcept
{
	constexpr size_t frame = 3 * sizeof(void *);
	return nodes < SIZE_MAX && nodes + 1 <= SIZE_MAX / frame &&
	       pouch_wire_add(total, (nodes + 1) * frame);
}
struct pouch_payload_budget
{
	pouch_wire_budget wire;
	const craft_recipe_continuation *recipe = nullptr;
	const item_transfer_payload *payload = nullptr;
	const pouch_payload_map *consumed = nullptr;
	bool prefix(size_t &output, size_t extra = 0) const noexcept
	{
		size_t total = 0, actual = 0;
		constexpr size_t observation =
			7 * sizeof(void *) + 6 * sizeof(size_t) + 3 * sizeof(bool);
		if (!wire.prefix(total) || !pouch_wire_add(total, sizeof(*this) - sizeof(wire)) ||
		    !pouch_wire_add(total, observation) ||
		    (recipe &&
		     (!wire.heap(recipe->pouch_mutation, total) ||
		      !wire.heap(recipe->refine_root_order, total) ||
		      (recipe->refine_material_name.capacity() > 15 &&
		       (recipe->refine_material_name.capacity() == SIZE_MAX ||
			!pouch_wire_add(total, recipe->refine_material_name.capacity() + 1))))) ||
		    (payload &&
		     (!item_transfer_payload_current_heap_bytes(*payload, &actual) ||
		      !pouch_wire_add(total, actual) ||
		      !pouch_wire_add(total, item_transfer_payload_copy_frame_bytes()))) ||
		    (consumed &&
		     (consumed->size() > SIZE_MAX / sizeof(pouch_payload_node) ||
		      !pouch_wire_add(total, consumed->size() * sizeof(pouch_payload_node)) ||
		      !pouch_payload_erase_frames(consumed->size(), total))) ||
		    !pouch_wire_add(total, extra))
			return false;
		output = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && wire.reserve && wire.reserve(total, wire.context);
	}
	bool insert(const pouch_payload_map &value, int key) const noexcept
	{
		// Authenticate the real currently absent/present key before forming
		// its exact original operator[] request, with true lookup frames.
		constexpr size_t own =
			2 * sizeof(void *) + sizeof(int) + sizeof(bool) + sizeof(size_t);
		if (!peak(pouch_payload_lookup_frames + own))
			return false;
		const bool missing = !value.contains(key);
		size_t extra = pouch_payload_insert_frames + own;
		if (missing && (!pouch_wire_add(extra, sizeof(pouch_payload_node)) ||
				!pouch_wire_add(extra, 3 * sizeof(void *))))
			return false;
		// Current n+1 erase recursion already lives in prefix. One new node
		// adds precisely one prospective recursive frame before allocation.
		return peak(extra);
	}
};
bool pouch_payload_reset(pouch_payload_budget &owner, craft_pouch_mutation *output) noexcept
{
	// Actual temporary compound mutation has two default rows and a usage
	// vector. Its true lifetime/default/move/destructor scopes are admitted
	// before construction and the exact original successful reset.
	if (!owner.peak(sizeof(craft_pouch_mutation) + pouch_wire_empty_constructor_frames +
			pouch_wire_move_frames + player_item_snapshot_copy_frame_bytes()))
		return false;
	*output = {};
	return true;
}
} // namespace
bool craft_pouch_mutation_from_payload_bounded(const item_transfer_payload &payload,
					       craft_pouch_mutation *mutation,
					       bool (*reserve)(size_t, void *) noexcept,
					       void *context, size_t outer) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)payload;
	(void)mutation;
	(void)reserve;
	(void)context;
	(void)outer;
	return false;
#else

	constexpr size_t frames = 4 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool) +
				  pouch_wire_pure_frames + pouch_payload_lookup_frames +
				  pouch_payload_map_lifetime_frames;
	pouch_payload_budget owner{ { reserve, context, outer, frames } };
	size_t prefix = 0;
	if (!owner.peak())
		return false;

	if (!mutation)
		return false;
	if (payload.continuation.kind == item_transfer_continuation_kind::craft_recipe)
	{
		owner.wire.frames += sizeof(craft_recipe_continuation) +
				     duris_craft_recipe_bounded_detail::pure_frames +
				     duris_craft_recipe_bounded_detail::string_lifetime_frames;
		if (!owner.peak())
			return false;
		craft_recipe_continuation recipe;
		owner.recipe = &recipe;
		if (!owner.prefix(prefix))
			return false;
		if (!craft_recipe_continuation_decode_bounded(
			    std::span<const uint8_t>(payload.continuation.data.data(),
						     payload.continuation.data.size()),
			    &recipe, reserve, context, prefix) ||
		    !craft_recipe_continuation_matches(recipe, payload))
			return false;
		if (recipe.pouch_mutation.empty())
		{
			return pouch_payload_reset(owner, mutation); // original empty recipe reset
		}
		try
		{
			owner.wire.frames += sizeof(item_transfer_payload) +
					     item_transfer_payload_copy_frame_bytes();
			if (!owner.peak())
				return false;
			item_transfer_payload pouch_payload;
			owner.payload = &pouch_payload;
			if (!owner.prefix(prefix) ||
			    !item_transfer_payload_clone_bounded(payload, &pouch_payload, reserve,
								 context, prefix))
				return false;
			pouch_payload.continuation.kind =
				item_transfer_continuation_kind::craft_pouch_usage;
			if (!owner.peak(pouch_wire_move_frames))
				return false;
			pouch_payload.continuation.data = std::move(recipe.pouch_mutation);
			return owner.prefix(prefix) &&
			       craft_pouch_mutation_from_payload_bounded(pouch_payload, mutation,
									 reserve, context, prefix);
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
		return pouch_payload_reset(owner, mutation); // original absent continuation reset
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
		owner.wire.frames += sizeof(craft_pouch_mutation) + sizeof(pouch_payload_map) +
				     pouch_wire_empty_constructor_frames +
				     player_item_snapshot_copy_frame_bytes();
		if (!owner.peak())
			return false;
		craft_pouch_mutation candidate;
		owner.wire.candidate = &candidate;
		if (!owner.prefix(prefix))
			return false;
		if (!craft_pouch_mutation_decode_bounded(
			    std::span<const uint8_t>(payload.continuation.data.data(),
						     payload.continuation.data.size()),
			    &candidate, reserve, context, prefix))
			return false;
		size_t retained_count = 0;
		std::map<int, uint64_t> consumed;
		owner.consumed = &consumed;
		// Actual entry/used references remain live through nested callbacks.
		owner.wire.frames += 2 * sizeof(void *);
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
				if (!owner.insert(consumed, entry.vnum))
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
			if (!owner.peak(pouch_payload_lookup_frames))
				return false;
			for (const auto &used : candidate.usage)
				if (!consumed.contains(used.vnum) ||
				    consumed.at(used.vnum) != used.count)
					return false;
		}
		owner.wire.frames -= 2 * sizeof(void *);
		if (!owner.peak(pouch_wire_move_frames + player_item_snapshot_copy_frame_bytes()))
			return false;
		*mutation = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}

#endif
}
