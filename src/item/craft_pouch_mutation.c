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

// PRIVATE WHOLE COMPLETE POUCH SOURCE CANDIDATE; REVIEW/ROOT QUALIFICATION OPEN.
// PRIVATE reusable Source fragment for the unchanged original
// item_transfer_payload_clone_bounded. No endpoint, selection, or runtime edit.
// Pointers/references/results belong declared Source scopes, not emitted stack.
namespace
{
// GNU13 vector(copy) selected allocator/base/frontend: one real selected
// allocator object. No imaginary extra allocator objects or trivial iterator
// copy/move bodies. Const stored-pointer lvalues bind directly to constructor
// references. begin/end prvalues are the uninitialized-copy parameters.
template <class T> constexpr size_t item_payload_clone_leaf_vector_copy_source =
	(19 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::allocator<T>)) +
	(11 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(bool)) + 6 * sizeof(void *) +
	// Trivial uninitialized-copy backend; actual _Num is ptrdiff_t.
	// No bare builtin memmove callee, implicit trivial T assignment, or
	// invented normal-iterator copy/move call. Actual wrapper objects stay.
	46 * sizeof(void *) + sizeof(std::ptrdiff_t) + 4 * sizeof(bool) -
	// The two begin/end result objects ARE the two by-value parameters.
	2 * sizeof(void *);

template <class T> constexpr size_t item_payload_clone_leaf_vector_cleanup_source =
	// vector/base/nontrivial Impl/allocator receivers, Destroy allocator/range/
	// trivial aux, mutable allocator getter, deallocate wrappers/sized delete.
	// Two genuine predicate-wrapper bool results, no bare intrinsic callee.
	22 * sizeof(void *) + 5 * sizeof(size_t) + 2 * sizeof(bool);

template <class T> constexpr size_t item_payload_clone_leaf_vector_move_source =
	// Equal standard allocator assignment, genuine __move_storage bool;
	// constexpr propagation initializer has no extra runtime callee bool.
	// Real __tmp vector, returned allocator, swap-data three-pointer object.
	// Declared allocator/new_allocator const-copy assignments remain actual.
	40 * sizeof(void *) + sizeof(bool) + sizeof(std::true_type) + sizeof(std::vector<T>) +
	sizeof(std::allocator<T>);

template <class T> constexpr size_t item_payload_clone_leaf_vector_union_source =
	item_payload_clone_leaf_vector_copy_source<T> +
	item_payload_clone_leaf_vector_move_source<T> +
	item_payload_clone_leaf_vector_cleanup_source<T> -
	// Actual mutable get_Tp_allocator2P is shared move/cleanup. Copy/move
	// share allocator/new_allocator const-copy4P, Impl(a)2P, Impl_data1P,
	// const/mutable get_Tp_allocator4P. No old admission high-water borrowed.
	13 * sizeof(void *);

// Exact accepted GNU13 string(copy<char*>)+equal-allocator assignment+cleanup
// union. Mutable and const local_data pointer chains stay distinct. No bare
// memcpy/memcmp/strlen callee. Genuine declared predicate results stay.
// Tail runtime67P13N+C8B; separate Root-directed conservative init N+C.
// Copy increment45P10N+2D+forward/random tags+one selected allocator+B,
// after only actual shared tail descendants (local-data/stdmove/etc).
constexpr size_t item_payload_clone_leaf_string_source =
	112 * sizeof(void *) + 24 * sizeof(size_t) + 2 * sizeof(std::ptrdiff_t) +
	sizeof(std::forward_iterator_tag) + sizeof(std::random_access_iterator_tag) +
	sizeof(std::allocator<char>) + 2 * sizeof(char) + 9 * sizeof(bool);

constexpr size_t item_payload_clone_leaf_generated_source =
	// Ten actual enclosing call occurrences: payload/corpse/collector/
	// continuation/recovery/two forests/publication/cost context/projection.
	// These are ten actual sites, NOT ten distinct type specializations.
	// Copy this/source2P, genuine nontrivial move assignment this/source/ref3P,
	// nontrivial cleanup receiverP. Real payload stdmove arg/result2P below.
	// All scalar-only owners/native-money/attempt/arrays stay inline, with no
	// undeclared implicit trivial member-call scopes.
	10 * (2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *);

constexpr size_t item_payload_clone_leaf_member_source =
	item_payload_clone_leaf_generated_source + item_payload_clone_leaf_string_source +
	item_payload_clone_leaf_vector_union_source<uint64_t> +
	item_payload_clone_leaf_vector_union_source<uint8_t> +
	item_payload_clone_leaf_vector_union_source<native_quest_cost_attempt> -
	// Four real typed allocation/delete lanes (three vector types+char).
	// Global operator new and sized delete are the same declared boundaries.
	// Their real wrappers remain typed and owned, not replaced by this cut.
	3 * 2 * (sizeof(void *) + sizeof(size_t));

constexpr size_t item_payload_clone_leaf_helper_source =
	// Fresh public getter2P+N+B; policy genuine resultB; heap root2P+2B;
	// three vector-heap<T> signatures2P+N+2B each; string-heap2P+N+2B;
	// checked-add(size_t&,extra)P+N+B. Only fresh=true reaches this clone:
	// size() is already in corresponding copy/string family; capacity() is
	// not selected by this path. No CURRENT observer envelope substituted.
	13 * sizeof(void *) + 6 * sizeof(size_t) + 13 * sizeof(bool);

constexpr size_t item_payload_clone_leaf_candidate_inline =
	// The one actual private candidate DTO; input/output remain caller-owned.
	// This is owner INLINE, deliberately separate from the Source frame scopes.
	sizeof(item_transfer_payload);
constexpr size_t item_payload_clone_leaf_caller_source =
	// Four pointer/reference formals + actual bad_alloc catch-referenceP;
	// outer, constexpr own, peak, request are four declared size_t scopes;
	// genuine bool result. Candidate INLINE is named separately above.
	5 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_payload_clone_leaf_full_source = item_payload_clone_leaf_caller_source +
						       item_payload_clone_leaf_helper_source +
						       item_payload_clone_leaf_member_source;

constexpr size_t item_payload_clone_leaf_required_preentry =
	// Combined prospective request only; never silently rename DTO as Source.
	item_payload_clone_leaf_full_source + item_payload_clone_leaf_candidate_inline;
constexpr size_t item_payload_clone_leaf_matched_inline =
	// Original first own reserves this exact prospective candidate owner.
	item_payload_clone_leaf_candidate_inline;
constexpr size_t item_payload_clone_leaf_matched_source =
	// Only unambiguous original first own frame fields plus genuine ten
	// copy/move this/source4P + cleanupP groups. Candidate owner credit is
	// named separately; old conservative array/intrinsic/library/observation
	// allowances stay unchanged and UNCLAIMED. No whole envelope subtraction.
	4 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) +
	10 * (4 * sizeof(void *) + sizeof(void *));
constexpr size_t item_payload_clone_leaf_supplement_source =
	item_payload_clone_leaf_full_source - item_payload_clone_leaf_matched_source;

static_assert(std::is_trivially_copyable_v<native_quest_cost_attempt>);
static_assert(std::is_trivially_destructible_v<native_quest_cost_attempt>);
static_assert(item_payload_clone_leaf_vector_copy_source<uint8_t> ==
	      80 * sizeof(void *) + 9 * sizeof(size_t) + sizeof(std::ptrdiff_t) +
		      sizeof(std::allocator<uint8_t>) + 5 * sizeof(bool));
static_assert(item_payload_clone_leaf_supplement_source > 0);
static_assert(item_payload_clone_leaf_required_preentry -
		      (item_payload_clone_leaf_matched_source +
		       item_payload_clone_leaf_matched_inline) ==
	      item_payload_clone_leaf_supplement_source);
} // namespace

namespace
{
template <class T> constexpr size_t item_payload_default_vector_source =
	// vector/base/impl/data + selected allocator/new_allocator default receivers.
	6 * sizeof(void *);
constexpr size_t item_payload_default_string_local_source = 4 * 2 * sizeof(void *);
constexpr size_t item_payload_default_string_init_source =
	sizeof(void *) + sizeof(size_t) + sizeof(char) + sizeof(bool);
constexpr size_t item_payload_default_string_set_length_source =
	2 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(char) + sizeof(bool);
constexpr size_t item_payload_default_string_source =
	// String receiver; hider receiver/data/default-allocator reference; genuine
	// temporary allocator; allocator/new_allocator default and const-copy;
	// move reference/result; real temporary allocator cleanup receiver.
	// The new_allocator destructor declaration is PP-excluded in C++20.
	sizeof(void *) + 3 * sizeof(void *) + sizeof(std::allocator<char>) + 2 * sizeof(void *) +
	4 * sizeof(void *) + 2 * sizeof(void *) + sizeof(void *) +
	item_payload_default_string_local_source + item_payload_default_string_init_source +
	item_payload_default_string_set_length_source;
constexpr size_t item_payload_aggregate_default_source =
	// Six strings and six vectors are initialized sequentially, with no
	// callback or nested member initialization through these leaf bodies.
	// Four uint64 vectors reuse their completed typed lane. The byte/attempt
	// specializations remain separate. No default/copy/move aggregate receiver
	// is invented for recursive aggregate initialization of value = {}.
	item_payload_default_string_source + item_payload_default_vector_source<uint8_t> +
	item_payload_default_vector_source<uint64_t> +
	item_payload_default_vector_source<native_quest_cost_attempt>;
template <class T> constexpr size_t item_payload_cleanup_vector_source =
	// Actual vector/base/generated _Vector_impl/allocator cleanup receivers.
	// _Vector_impl is nontrivial because allocator has a declared destructor;
	// the new_allocator destructor declaration is PP-excluded in C++20.
	4 * sizeof(void *) +
	// _Destroy(first,last,allocator), _Destroy(first,last), trivial aux<true>.
	3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	// _M_get_Tp_allocator this/reference result; _M_deallocate, traits,
	// allocator and new_allocator pointer/count receivers; sized delete;
	// two actual predicate wrapper results: _Destroy and allocator deallocate.
	// Their calls are separate; no intrinsic result is invented. T is trivial.
	2 * sizeof(void *) + 4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) +
	sizeof(size_t) + 2 * sizeof(bool);
constexpr size_t item_payload_cleanup_string_source =
	// String/dispose/is-local/destroy receivers, destroy size and locality bool.
	4 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	// Actual const data and local_data/pointer_to/addressof/__addressof.
	2 * sizeof(void *) + 4 * 2 * sizeof(void *) +
	// _M_get_allocator receiver/reference result. String destroy directly
	// calls traits deallocate: it does not call vector _M_deallocate.
	2 * sizeof(void *) + 3 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) +
	sizeof(size_t) + sizeof(bool) +
	// Actual hider/allocator destruction receivers. new_allocator destructor
	// is PP-excluded in selected C++20; allocator predicate has one result.
	2 * sizeof(void *);
constexpr size_t item_payload_cleanup_source =
	// Ten reached generated nontrivial aggregate destructor receiver sites:
	// payload, corpse, collector, continuation, recovery, two forests,
	// publication, native-cost context and native-cost projection.
	10 * sizeof(void *) + item_payload_cleanup_string_source +
	item_payload_cleanup_vector_source<uint8_t> + item_payload_cleanup_vector_source<uint64_t> +
	item_payload_cleanup_vector_source<native_quest_cost_attempt>;
}

namespace
{
constexpr size_t item_payload_current_public_source =
	// Public value/output references, actual bytes local and bool result.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t item_payload_current_heap_source =
	// payload_clone_heap: borrowed value/bytes, fresh and returned bool.
	2 * sizeof(void *) + 2 * sizeof(bool);
template <class T> constexpr size_t item_payload_current_vector_heap_source =
	// Real value/bytes references, fresh, count local and bool result.
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool);
constexpr size_t item_payload_current_string_heap_source =
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool);
constexpr size_t item_payload_current_add_source =
	// Actual checked-add reference, amount and returned bool.
	sizeof(void *) + sizeof(size_t) + sizeof(bool);
template <class T> constexpr size_t item_payload_current_vector_capacity_source =
	// The selected ordinary vector<T>::capacity has this and scalar return;
	// pointer subtraction has no separate named ptrdiff_t temporary.
	sizeof(const std::vector<T> *) + sizeof(size_t);
constexpr size_t item_payload_current_string_capacity_source =
	// string::capacity this/result, _M_is_local this/bool; genuine const
	// _M_data, _M_local_data, pointer_to, addressof, __addressof ref/results.
	sizeof(const std::string *) + sizeof(size_t) + sizeof(const std::string *) + sizeof(bool) +
	2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *);
constexpr size_t item_payload_current_source =
	item_payload_current_public_source +
	// payload_clone_policy_supported has only its genuine bool result.
	sizeof(bool) + item_payload_current_heap_source + item_payload_current_add_source +
	item_payload_current_string_heap_source + item_payload_current_string_capacity_source +
	item_payload_current_vector_heap_source<uint8_t> +
	item_payload_current_vector_heap_source<uint64_t> +
	item_payload_current_vector_heap_source<native_quest_cost_attempt> +
	item_payload_current_vector_capacity_source<uint8_t> +
	item_payload_current_vector_capacity_source<uint64_t> +
	item_payload_current_vector_capacity_source<native_quest_cost_attempt>;
}

// Private finite GNU13/C++20 leaf. No public API or maintained selection.
// Rows, parent named vectors/lists, capacities and string heaps are physical
// caller owners, separately retained. This is declared Source, not stack use.
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
constexpr size_t pouch_complete_snapshot_observation_own_source =
	// request<DA,ED,i32>: value/ref2P, fresh+return2B, countN each.
	3 * (2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool)) +
	// string request has the same declared signature/count/results.
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) +
	// row request: row/bytes2P, fresh/result2B; actual hidden ED range
	// reference, begin/end iterator objects, description reference4P.
	6 * sizeof(void *) + 2 * sizeof(bool) +
	// CURRENT and fresh roots each row/output2P, bytesN, bool result.
	2 * (2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	// Actual policy returnB; item_list_add bytes-ref/value/returnP+N+B.
	sizeof(bool) + sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_complete_snapshot_observation_vector_query_source =
	// Actual size/capacity for three distinct DA/ED/int32 vector types.
	6 * (sizeof(void *) + sizeof(size_t));
constexpr size_t pouch_complete_snapshot_observation_string_query_source =
	// Public size/capacity receiver/result; capacity reaches is_local,
	// _M_data getter and const local_data/pointer_to/addressof/__addressof.
	// The four row/two ED strings share the same completed string family.
	2 * (sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) +
	4 * (2 * sizeof(void *));
constexpr size_t pouch_complete_snapshot_observation_ed_iterator_source =
	// Const ED begin/end receivers2P, pointer-ref constructor2P, ++2P,
	// dereference2P, comparison2P+B and base2P. The two hidden iterator
	// destinations belong to row request above; no second result object.
	// Stored pointer lvalues bind directly through qualification-compatible
	// const references. No same-type implicit trivial copy/dtor bodies.
	12 * sizeof(void *) + sizeof(bool);
constexpr size_t pouch_complete_snapshot_observation_source =
	pouch_complete_snapshot_observation_own_source +
	pouch_complete_snapshot_observation_vector_query_source +
	pouch_complete_snapshot_observation_string_query_source +
	pouch_complete_snapshot_observation_ed_iterator_source;
static_assert(pouch_complete_snapshot_observation_source ==
	      50 * sizeof(void *) + 15 * sizeof(size_t) + 16 * sizeof(bool));

constexpr size_t pouch_complete_singleton_vector_constructor_source =
	// vector(list,a): receiver/ref2P, actual by-value list formal. Public
	// default allocator object and actual allocator/newallocator default
	// receivers2P; selected allocator cleanupP (no C++20 newallocator dtor).
	2 * sizeof(void *) + sizeof(std::initializer_list<player_item_snapshot>) +
	sizeof(std::allocator<player_item_snapshot>) + 3 * sizeof(void *) +
	// Base(a)/Impl(a)/allocator-copy/newallocator-copy2P each, data defaultP.
	9 * sizeof(void *) +
	// Actual initializer_list private pointer/count constructor2P+N;
	// member begin/end2P each and sizeP+N. Its by-value formal above is real,
	// but initializer_list declares no same-type copy or destructor body.
	7 * sizeof(void *) + 2 * sizeof(size_t) +
	// range_initialize receiver/raw first/last3P, nN, forward-tag formal
	// and actual random-access argument object. No size/fill constructor.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(std::forward_iterator_tag) +
	sizeof(std::random_access_iterator_tag) +
	// check_init_len allocator-ref/n/returnP+2N and actual copied allocator
	// temporary; its const-copy and cleanup signatures are shared above.
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(std::allocator<player_item_snapshot>) +
	// distance/raw __distance/category:5P, two difference scalar returns,
	// random-access __distance tag formal. No pointer iterator wrappers.
	5 * sizeof(void *) + 2 * sizeof(std::ptrdiff_t) + sizeof(std::random_access_iterator_tag) +
	// Mutable get_Tp_allocator receiver/reference, one completed family.
	2 * sizeof(void *);
constexpr size_t pouch_complete_singleton_vector_allocation_source =
	// M_allocate2P+N, traits allocate2P+N, allocator allocate2P+N+B,
	// newallocator allocate3P+N (including default hint), maxP+N,
	// selected ordinary operator new pointer-resultP/countN.
	11 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_complete_singleton_vector_internal_max_source =
	// Actual C++20 S_max allocator-refP, returnedN and two local N;
	// allocator_traits<allocator<row>>::max_size allocator-refP/resultN.
	// Selected traits computes size_t(-1)/sizeof(row) directly. It does
	// NOT call newallocator::_M_max_size or require a sharing subtraction.
	2 * sizeof(void *) + 4 * sizeof(size_t);
constexpr size_t pouch_complete_singleton_vector_min_source =
	// Actual S_max invokes min<size_t>(diffmax,allocmax): two const refs
	// and returned const ref3P. Real min family, never an old pool credit.
	3 * sizeof(void *);
constexpr size_t pouch_complete_singleton_raw_row_copy_backend_source =
	// stdallocator uninitialized_copy_a5P+B; uninitialized_copy4P+two
	// actual const bool locals; __uninit_copy<false>4P; do_uninit_copy5P;
	// Construct2P+B; addressof2P; forward<const row&>2P; placement2P+N.
	// Nontrivial row short-circuits check_constructible; no runtime result
	// for that compile-time selector or an undeclared builtin-copy callee.
	26 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(bool);
constexpr size_t pouch_complete_singleton_vector_cleanup_source =
	// Actual vector/base/impl/allocator receivers4P. Data and C++20
	// newallocator have no selected declared destructor bodies.
	4 * sizeof(void *) +
	// Destroy(first,last,allocator)3P, Destroy(first,last)2P+B,
	// false aux raw first/last2P; mutable allocator getter2P.
	3 * sizeof(void *) + 2 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) +
	2 * sizeof(void *) +
	// Deallocate vector/traits/allocator/newallocator each2P+N,
	// actual sized deleteP+N, deallocator constant-evaluation queryB.
	4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	// Raw-row aux reaches addressof2P, Destroy(pointer)P, destroy_atP,
	// and generated nontrivial row destructor receiverP. Member cleanup
	// graphs belong to the separate actual row leaf, never to this sizeof.
	5 * sizeof(void *);
constexpr size_t pouch_complete_singleton_const_front_source =
	// The actual pouch items is CONST, selecting const front2P/const beginP,
	// const iterator pointer-ref ctor2P/deref2P and materialized iteratorP.
	// Actual begin result occupies that receiver object; no second object.
	8 * sizeof(void *);
constexpr size_t pouch_complete_singleton_vector_generated_row_source =
	// Both real row copies reach the same completed generated COPY family.
	// Row-member strings/DA/ED/int32 COPY/lifetimes are separately composed.
	2 * sizeof(void *);
constexpr size_t pouch_complete_singleton_vector_source =
	pouch_complete_singleton_vector_constructor_source +
	pouch_complete_singleton_vector_allocation_source +
	pouch_complete_singleton_vector_internal_max_source +
	pouch_complete_singleton_vector_min_source +
	pouch_complete_singleton_raw_row_copy_backend_source +
	pouch_complete_singleton_vector_cleanup_source +
	pouch_complete_singleton_const_front_source +
	pouch_complete_singleton_vector_generated_row_source +
	// Actual items.capacity receiver/size result; no vector::max_size call.
	sizeof(void *) + sizeof(size_t);
static_assert(pouch_complete_singleton_vector_source ==
	      112 * sizeof(void *) + 22 * sizeof(size_t) + 2 * sizeof(std::ptrdiff_t) +
		      sizeof(std::forward_iterator_tag) +
		      2 * sizeof(std::random_access_iterator_tag) +
		      2 * sizeof(std::allocator<player_item_snapshot>) +
		      sizeof(std::initializer_list<player_item_snapshot>) + 7 * sizeof(bool));
constexpr size_t pouch_complete_singleton_vector_row_family_shared_source =
	// ONLY if actual parent separately retains its genuine row leaf's
	// COPY2P/destructorP (Quest's selected row-and-mutation own17P does):
	// these same typed family scopes are shared. No child admission credit.
	3 * sizeof(void *);
#endif
} // namespace

// PRIVATE selected pouch row-member vector Source; review pending.
// Exact matching ledger-v7 leaves only; no writer growth/insertion/row-move
// constructor/whole-list lifetime or historical unused fixed profile credit.
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
template <class T> constexpr size_t pouch_full_copy_vector_frontend_source =
	19 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::allocator<T>);

template <class T> constexpr size_t pouch_full_copy_vector_allocation_source =
	11 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(bool);

constexpr size_t pouch_full_copy_vector_iterator_entry_source =
	// begin/end receivers and actual result destinations4P; declared
	// pointer constructor2P. Qualification-compatible stored pointer lvalues
	// bind directly: no conversion temporary or undeclared trivial copy body.
	6 * sizeof(void *);

constexpr size_t pouch_full_copy_vector_trivial_backend_source =
	// __miter_base parameter/result objects keep their genuine owners.
	// No undeclared implicit trivial iterator move-constructor callee scope.
	// Bare builtin memmove declares no Source callee fields. Actual copy
	// wrapper formals, __assign_one and named _Num local remain owned.
	46 * sizeof(void *) + sizeof(std::ptrdiff_t) + 4 * sizeof(bool);

constexpr size_t pouch_full_copy_vector_ed_backend_source =
	34 * sizeof(void *) + sizeof(size_t) + 5 * sizeof(bool);

template <class T> constexpr size_t pouch_full_vector_deallocate_source =
	// _M_deallocate, allocator_traits::deallocate, allocator::deallocate,
	// new_allocator::deallocate each owns this/alloc, pointer, count.
	4 * (2 * sizeof(void *) + sizeof(size_t)) +
	// Actual sized operator delete pointer/count. Const-evaluation alternate
	// unsized delete is not selected; single helper bool is actual query.
	sizeof(void *) + sizeof(size_t) + sizeof(bool);

template <class T> constexpr size_t pouch_full_vector_cleanup_source =
	// Actual vector, Vector_base, generated nontrivial Vector_impl and
	// allocator destructor receivers. Impl_data has no declared destructor;
	// new_allocator destructor is excluded by the C++20 preprocessor path.
	// Neither contributes a runtime selected destructor scope.
	4 * sizeof(void *) +
	// _Destroy(first,last,allocator), _Destroy(first,last) and selected aux:
	// no invented cursor local: aux<false> increments its first parameter.
	3 * sizeof(void *) + 2 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) +
	// Mutable get_Tp_allocator receiver/reference result.
	2 * sizeof(void *) + pouch_full_vector_deallocate_source<T> +
	(std::is_trivially_destructible_v<T> ?
		 0 :
		 // Nontrivial aux reaches __addressof, _Destroy(pointer), destroy_at
		 // and the genuine T destructor receiver. Member cleanup is separate;
		 // raw-pointer ++ and == use no iterator object/helper family.
		 2 * sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(void *));

template <class T> constexpr size_t pouch_full_vector_tail_controller_source =
	// operator=(vector&&): this, source, returned ref, __move_storage.
	3 * sizeof(void *) + sizeof(bool) +
	// The traits query is used only in the constexpr initializer of
	// __move_storage. Its runtime callee-result scope is not selected; the
	// genuine named __move_storage bool above remains. std::move is actual.
	2 * sizeof(void *) +
	// _M_move_assign(true): this, source, actual by-value true_type;
	// genuine __tmp vector plus get_allocator's returned allocator temporary.
	2 * sizeof(void *) + sizeof(std::true_type) + sizeof(std::vector<T>) +
	sizeof(std::allocator<T>) +
	// get_allocator(const) this; const get_Tp_allocator this/ref-result;
	// allocator const-copy this/source and new_allocator const-copy2P.
	sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	// vector(a), Vector_base(a), Vector_impl(a): each this/allocator ref.
	// Allocator/new_allocator copy is the same selected const-copy family
	// above. Impl_data() receiver is a separate selected initializer scope.
	3 * (2 * sizeof(void *)) + sizeof(void *) +
	// swap_data this/source and genuine three-pointer data temporary.
	// Impl_data default already owned above; copy_data this/source is an
	// actual single typed helper family shared by the three calls/two swaps.
	2 * sizeof(void *) + 3 * sizeof(T *) + 2 * sizeof(void *) +
	// Mutable get_Tp_allocator is a separate overload; __alloc_on_move refs,
	// std::move(allocator) argument/result, allocator and new_allocator
	// selected generated CONST COPY assignments each this/source/returned-ref.
	// Both allocator types declare const-copy assignment and user copy/dtor;
	// std::move(__two) binds their const reference; no move-assignment exists.
	2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) + 3 * sizeof(void *) +
	3 * sizeof(void *);
template <class T> constexpr size_t pouch_complete_row_trivial_copy_vector_source =
	pouch_full_copy_vector_frontend_source<T> + pouch_full_copy_vector_allocation_source<T> +
	pouch_full_copy_vector_iterator_entry_source +
	pouch_full_copy_vector_trivial_backend_source -
	// begin/end CI class-prvalue results are exactly the two by-value
	// uninitialized_copy_a Input parameters, not separate physical objects.
	2 * sizeof(typename std::vector<T>::const_iterator);
constexpr size_t pouch_complete_row_ed_copy_vector_source =
	pouch_full_copy_vector_frontend_source<player_item_extra_description_snapshot> +
	pouch_full_copy_vector_allocation_source<player_item_extra_description_snapshot> +
	pouch_full_copy_vector_iterator_entry_source + pouch_full_copy_vector_ed_backend_source -
	2 * sizeof(std::vector<player_item_extra_description_snapshot>::const_iterator);
constexpr size_t pouch_complete_row_copy_vectors_source =
	pouch_complete_row_trivial_copy_vector_source<player_item_dynamic_affect_snapshot> +
	pouch_complete_row_ed_copy_vector_source +
	pouch_complete_row_trivial_copy_vector_source<int32_t>;
constexpr size_t pouch_complete_row_member_default_vectors_source =
	// Parent before/after={} directly default only DA/ED row members.
	// Nested spell vector is COPY-constructed with copied ED elements;
	// ED defaults in the bounded list decoder belong to that child.
	// Each selected vector/base/impl/data/allocator/newallocator six receivers.
	2 * (6 * sizeof(void *));
constexpr size_t pouch_complete_row_member_cleanup_vectors_source =
	pouch_full_vector_cleanup_source<player_item_dynamic_affect_snapshot> +
	pouch_full_vector_cleanup_source<player_item_extra_description_snapshot> +
	pouch_full_vector_cleanup_source<int32_t>;
constexpr size_t pouch_complete_row_member_move_vectors_source =
	pouch_full_vector_tail_controller_source<player_item_dynamic_affect_snapshot> +
	pouch_full_vector_cleanup_source<player_item_dynamic_affect_snapshot> +
	pouch_full_vector_tail_controller_source<player_item_extra_description_snapshot> +
	pouch_full_vector_cleanup_source<player_item_extra_description_snapshot> +
	// ED's old vector contents may be nonempty at final output transfer;
	// its string/spell descendants are separately retained by parent graph.
	pouch_full_vector_cleanup_source<int32_t>;
constexpr size_t pouch_complete_row_member_vectors_selected_source =
	pouch_complete_row_copy_vectors_source + pouch_complete_row_member_default_vectors_source +
	pouch_complete_row_member_cleanup_vectors_source +
	pouch_complete_row_member_move_vectors_source;
constexpr size_t pouch_complete_row_and_mutation_nontrivial_own_source =
	// Generated actual nontrivial row COPY2P/move-assignment3P/destructorP;
	// generated ED COPY2P (ED destructor already in vector cleanup above).
	// Genuine mutation defaultP/move-assignment3P/destructorP, plus caller
	// std::move<row>2P and std::move<mutation>2P typed scopes. Row brace{}
	// sites have no row-default constructor; arrays/DA have no invented
	// undeclared implicit trivial callable body. All physical DTOs separate.
	17 * sizeof(void *);
#endif
} // namespace

// PRIVATE selected snapshot string lifetime Source; review pending.
// Actual rows copy/move/default/destroy their four strings; ED copies and
// destroys its two strings. These are matching genuine string leaves only,
// not whole ledger writer/row/vector Source or legacy profile credit.
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
constexpr size_t pouch_full_string_tail_operator_source =
	// operator=(string&&): this/source/returned-ref, equal_allocs, genuine
	// heap-transfer __data and __capacity. Different-allocator/deep-assign
	// and early destroy branches are unreachable with std::allocator<char>.
	4 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	// actual always_equal and propagate-on-move getter result families.
	2 * sizeof(bool) +
	// get_allocator mutable this/reference; alloc_on_move references and
	// std::move allocator args/result; allocator/new_allocator const-copy
	// assignment this/source/ref-return each. No allocator move-assignment.
	2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) + 2 * (3 * sizeof(void *)) +
	// Actual local-source guard addressof argument/returned pointer.
	2 * sizeof(void *) +
	// String data setter, length/capacity setters, and clear receiver.
	2 * sizeof(void *) + 2 * (sizeof(void *) + sizeof(size_t)) + sizeof(void *);

constexpr size_t pouch_full_string_local_const_source =
	// local_data const -> pointer_to -> addressof -> __addressof.
	4 * (2 * sizeof(void *));

constexpr size_t pouch_full_string_tail_query_source =
	// _M_is_local this/return, _M_data getter this/pointer-return, const local;
	// size and length are separate actual public functions.
	sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + pouch_full_string_local_const_source +
	2 * (sizeof(void *) + sizeof(size_t));

constexpr size_t pouch_full_string_local_mutable_source = 4 * (2 * sizeof(void *));

constexpr size_t pouch_full_string_tail_use_local_source =
	// use_local_data this/pointer return, init_local_buf this and exactly one
	// genuine std::is_constant_evaluated bool. Root requires the separately
	// conservative owning-body loop index/char allowance despite its runtime
	// constant-evaluation branch being false; this is no emitted-stack claim.
	2 * sizeof(void *) + sizeof(void *) + sizeof(bool) + sizeof(size_t) + sizeof(char) +
	pouch_full_string_local_mutable_source;

constexpr size_t pouch_full_string_set_length_source =
	// _M_set_length and _M_length this/n; _M_data this/result;
	// char_traits::assign destination/source refs, actual NUL temporary and
	// one const-evaluation query. Shared assign family also serves _S_copy1.
	2 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(char) + sizeof(bool);

constexpr size_t pouch_full_string_runtime_copy_source =
	// _S_copy destination/source/count; traits::copy destination/source/count
	// and returned pointer. Bare builtin memcpy has no declared Source
	// callee fields; retain only _S_copy and traits::copy owning declarations.
	2 * sizeof(void *) + sizeof(size_t) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(bool);

constexpr size_t pouch_full_string_tail_raw_operator_source =
	pouch_full_string_tail_operator_source + pouch_full_string_tail_query_source +
	pouch_full_string_tail_use_local_source + pouch_full_string_set_length_source +
	pouch_full_string_runtime_copy_source;

constexpr size_t pouch_full_string_tail_cleanup_source =
	// Destructor/dispose/islocal/destroy, allocator-hider and allocator
	// destruction, get_allocator/data/local chain and selected char
	// traits/allocator/newallocator/sized-delete. No newallocator C++20 dtor.
	25 * sizeof(void *) + 5 * sizeof(size_t) + 2 * sizeof(bool);

constexpr size_t pouch_full_string_tail_runtime_union_source =
	pouch_full_string_tail_raw_operator_source - 3 * sizeof(void *) - sizeof(size_t) -
	sizeof(size_t) - sizeof(char) + pouch_full_string_tail_cleanup_source -
	// Exact move/cleanup shared family is islocal(P+B), data2P,
	// const-local8P and get_allocator2P =13P+B. No unrelated margin.
	13 * sizeof(void *) - sizeof(bool);

constexpr size_t pouch_full_string_tail_conservative_init_source =
	// Root-required exact named owning-body allowance, retained once globally.
	sizeof(size_t) + sizeof(char);

constexpr size_t pouch_full_string_tail_union_source =
	pouch_full_string_tail_runtime_union_source +
	pouch_full_string_tail_conservative_init_source;

constexpr size_t pouch_full_string_copy_increment_source =
	// Additions over the authentic selected string-tail union ONLY.
	// string(copy) instantiates _M_construct<char*> and mutable-char
	// _S_copy_chars; it cannot share const-char iterator specializations.
	// One materialized selected allocator object, not old hypothetical3A.
	45 * sizeof(void *) + 10 * sizeof(size_t) + 2 * sizeof(std::ptrdiff_t) +
	sizeof(std::forward_iterator_tag) + sizeof(std::random_access_iterator_tag) +
	sizeof(std::allocator<char>) + sizeof(bool);

constexpr size_t pouch_full_string_copy_tail_union_source =
	pouch_full_string_tail_union_source + pouch_full_string_copy_increment_source;
constexpr size_t pouch_complete_snapshot_string_selected_source =
	pouch_full_string_copy_tail_union_source +
	// Two mutable allocator getters coexist through actual alloc_on_move.
	// The imported tail names one; own the second authentic occurrence2P.
	duris_craft_recipe_bounded_detail::complete_recipe_string_mutable_allocator_source +
	duris_craft_recipe_bounded_detail::member_string_default_source;
#endif
} // namespace

// Private exact encoder-selected string COPY+cleanup. Independently traced
// declarations, not ambiguous combined tail/copy_increment numerical credit.
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
constexpr size_t pouch_complete_string_copy_frontend_source =
	// copy this/source2P; mutable local-data+pointer/address chain8P;
	// const allocator getter2P; ext selectP; traits selectP; allocator and
	// newallocator const-copy constructors4P; rvalue hider3P; stdmove2P.
	// One selected allocator object aliases the traits/ext prvalue chain.
	23 * sizeof(void *) + sizeof(std::allocator<char>);
constexpr size_t pouch_complete_string_copy_construct_source =
	// _M_construct<char*> this/beg/end/Guard4P,dnewN,F; guard ctor2P/dtorP;
	// distance2P+D, categoryP+R,__distance2P+D,char* copy_chars3P.
	// The category prvalue is __distance's actual by-value R parameter.
	15 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(std::ptrdiff_t) +
	sizeof(std::forward_iterator_tag) + sizeof(std::random_access_iterator_tag);
constexpr size_t pouch_complete_string_copy_allocation_source =
	// create3P+N,max_sizeP+N,traits maxP+N,S_allocate3P+N,traits allocate2P+N,
	// allocator allocate2P+N+B,newallocator allocate3P+N,_M_max_sizeP+N,
	// actual declared operator new pointer resultP/countN.
	// Const getter is in frontend; mutable getter is in reached cleanup.
	17 * sizeof(void *) + 9 * sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_complete_string_copy_remaining_source =
	// data setter2P,capacity setterP+N,init-localP+B plus retained conservative
	// body N/C (not runtime execution evidence); set_length6P+2N+C+B after
	// only actual data getter2P is shared with cleanup; runtime S_copy/traits
	// copy5P+2N+B and public lengthP+N. Bare memcpy has no callee Source.
	16 * sizeof(void *) + 7 * sizeof(size_t) + 2 * sizeof(char) + 3 * sizeof(bool);
constexpr size_t pouch_complete_string_copy_cleanup_selected_source =
	pouch_complete_string_copy_frontend_source + pouch_complete_string_copy_construct_source +
	pouch_complete_string_copy_allocation_source + pouch_complete_string_copy_remaining_source +
	pouch_full_string_tail_cleanup_source;
static_assert(pouch_complete_string_copy_cleanup_selected_source ==
	      96 * sizeof(void *) + 22 * sizeof(size_t) + 2 * sizeof(std::ptrdiff_t) +
		      sizeof(std::forward_iterator_tag) + sizeof(std::random_access_iterator_tag) +
		      sizeof(std::allocator<char>) + 2 * sizeof(char) + 6 * sizeof(bool));
#endif
} // namespace

// PRIVATE review-pending selected usage-vector Source. These named
// graphs instantiate actual matching GNU13 declarations; no whole recipe
// encoder/decoder Source is used as credit. No original API/body changes.
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
using pouch_complete_usage = chaos_material_pouch_usage;
using pouch_complete_usage_vector = std::vector<pouch_complete_usage>;
using pouch_complete_usage_CI = pouch_complete_usage_vector::const_iterator;
namespace pouch_recipe_source = duris_craft_recipe_bounded_detail;
constexpr size_t pouch_complete_usage_default_source =
	// Explicit/defaulted vector/base/impl/data/allocator/newallocator
	// constructor receiver families. Physical vector/subobjects stay in DTO.
	6 * sizeof(void *);
constexpr size_t pouch_complete_usage_cleanup_source =
	pouch_recipe_source::member_vector_cleanup_source<pouch_complete_usage>();
constexpr size_t pouch_complete_usage_reserve_source =
	// Fresh candidate size/capacity0; count>0 is validated before entry.
	// Zero-count _S_relocate/a/a1 still owns formals/niter/count, but its
	// count>0 constant-evaluation predicate call is not reached. Null old
	// allocation reaches only _M_deallocate(this,p,n), no lower delete chain.
	pouch_recipe_source::complete_recipe_vector_reserve_own_source<pouch_complete_usage> +
	pouch_recipe_source::complete_recipe_vector_max_size_source<pouch_complete_usage> +
	pouch_recipe_source::complete_recipe_vector_capacity_source<pouch_complete_usage> +
	pouch_recipe_source::complete_recipe_vector_size_source<pouch_complete_usage> +
	pouch_recipe_source::complete_recipe_vector_allocate_source<pouch_complete_usage> +
	pouch_recipe_source::complete_recipe_vector_mutable_allocator_source<pouch_complete_usage> +
	21 * sizeof(void *) + sizeof(std::ptrdiff_t) + 2 * sizeof(void *) + sizeof(size_t);
constexpr size_t pouch_complete_usage_push_source =
	// Original push_back({vnum,count}) binds its actual aggregate temporary.
	// The implicit trivial aggregate move has no declared callable body.
	// Its object storage is separately owned at the original caller site.
	// Reserve(count), then exactly count pushes: each fitting branch is
	// selected. There is no realloc/check_len/allocation/relocation/delete
	// at these push occurrences. Actual emplace returns back()->end()->minus.
	pouch_recipe_source::complete_recipe_vector_push_own_source<pouch_complete_usage> +
	pouch_recipe_source::complete_recipe_scalar_move_source<pouch_complete_usage> +
	pouch_recipe_source::complete_recipe_vector_emplace_own_source<pouch_complete_usage> +
	pouch_recipe_source::complete_recipe_scalar_forward_source<pouch_complete_usage> +
	pouch_recipe_source::complete_recipe_vector_single_construct_source<pouch_complete_usage> +
	sizeof(void *) + sizeof(pouch_complete_usage_vector::iterator) +
	pouch_recipe_source::complete_recipe_normal_pointer_constructor_source<
		pouch_complete_usage_vector::iterator> +
	pouch_recipe_source::complete_recipe_vector_back_own_source<pouch_complete_usage> +
	pouch_recipe_source::complete_recipe_normal_offset_own_source<
		pouch_complete_usage_vector::iterator> +
	pouch_recipe_source::complete_recipe_normal_pointer_constructor_source<
		pouch_complete_usage_vector::iterator> +
	pouch_recipe_source::complete_recipe_normal_dereference_source<
		pouch_complete_usage_vector::iterator>;
constexpr size_t pouch_complete_usage_move_assignment_source =
	pouch_recipe_source::complete_recipe_vector_move_selected_source<pouch_complete_usage>;
constexpr size_t pouch_complete_valid_usage_source =
	// valid_usage usage-ref/index/result, then its actual const empty/query
	// graph and const operator[]. Empty's two CI return objects are real
	// comparison operands, not copies or manufactured qualification temps.
	sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	pouch_recipe_source::complete_recipe_vector_empty_own_source<pouch_complete_usage> +
	pouch_recipe_source::complete_recipe_vector_const_begin_own_source<pouch_complete_usage> +
	pouch_recipe_source::complete_recipe_vector_const_end_own_source<pouch_complete_usage> +
	2 * pouch_recipe_source::complete_recipe_normal_pointer_constructor_source<
		    pouch_complete_usage_CI> +
	pouch_recipe_source::complete_recipe_normal_equal_own_source<pouch_complete_usage_CI> +
	2 * pouch_recipe_source::complete_recipe_normal_base_source<pouch_complete_usage_CI> +
	pouch_recipe_source::complete_recipe_vector_size_source<pouch_complete_usage> +
	pouch_recipe_source::complete_recipe_vector_const_subscript_source<pouch_complete_usage>;
#endif
} // namespace

// PRIVATE actual pouch encode candidate byte-wire Source, review pending.
// candidate.reserve(total) precedes every push and both inserts; total includes
// header+usage+both encoded rows. No reached push/insert reallocation branch.
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
namespace pouch_wire_source = duris_craft_recipe_bounded_detail;
using pouch_complete_byte_I = std::vector<uint8_t>::iterator;
using pouch_complete_byte_CI = std::vector<uint8_t>::const_iterator;
using pouch_complete_byte_M = std::move_iterator<uint8_t *>;
constexpr size_t pouch_complete_byte_fresh_reserve_source =
	// Same selected fresh0/cap0 positive reserve graph as usage-v2, typed byte.
	pouch_wire_source::complete_recipe_vector_reserve_own_source<uint8_t> +
	pouch_wire_source::complete_recipe_vector_max_size_source<uint8_t> +
	pouch_wire_source::complete_recipe_vector_capacity_source<uint8_t> +
	pouch_wire_source::complete_recipe_vector_size_source<uint8_t> +
	pouch_wire_source::complete_recipe_vector_allocate_source<uint8_t> +
	pouch_wire_source::complete_recipe_vector_mutable_allocator_source<uint8_t> +
	21 * sizeof(void *) + sizeof(std::ptrdiff_t) + 2 * sizeof(void *) + sizeof(size_t);
constexpr size_t pouch_complete_byte_fitting_push_source =
	pouch_wire_source::complete_recipe_vector_push_own_source<uint8_t> +
	pouch_wire_source::complete_recipe_scalar_move_source<uint8_t> +
	pouch_wire_source::complete_recipe_vector_emplace_own_source<uint8_t> +
	pouch_wire_source::complete_recipe_scalar_forward_source<uint8_t> +
	pouch_wire_source::complete_recipe_vector_single_construct_source<uint8_t> +
	sizeof(void *) + sizeof(pouch_complete_byte_I) +
	pouch_wire_source::complete_recipe_normal_pointer_constructor_source<pouch_complete_byte_I> +
	pouch_wire_source::complete_recipe_vector_back_own_source<uint8_t> +
	pouch_wire_source::complete_recipe_normal_offset_own_source<pouch_complete_byte_I> +
	pouch_wire_source::complete_recipe_normal_pointer_constructor_source<pouch_complete_byte_I> +
	pouch_wire_source::complete_recipe_normal_dereference_source<pouch_complete_byte_I>;

template <class Output> constexpr size_t pouch_complete_byte_selected_copy_source =
	// Input is actual mutable byte-vector I, not const-byte or raw-span input.
	pouch_wire_source::complete_recipe_copy_projected_source<pouch_complete_byte_I, Output> +
	2 * pouch_wire_source::complete_recipe_miter_argument_source<pouch_complete_byte_I> +
	pouch_wire_source::complete_recipe_copy_projected_source<pouch_complete_byte_I, Output> +
	2 * pouch_wire_source::complete_recipe_niter_graph_source<pouch_complete_byte_I> +
	pouch_wire_source::complete_recipe_niter_graph_source<Output> +
	pouch_wire_source::complete_recipe_raw_copy_a1_source<uint8_t *, uint8_t *> +
	pouch_wire_source::complete_recipe_raw_copy_a2_source<uint8_t *, uint8_t *> +
	pouch_wire_source::complete_recipe_byte_simple_copy_m_source +
	pouch_wire_source::complete_recipe_std_constant_evaluated_source +
	(std::is_pointer_v<Output> ?
		 pouch_wire_source::complete_recipe_raw_niter_wrap_source :
		 pouch_wire_source::complete_recipe_normal_niter_wrap_own_source<Output> +
			 pouch_wire_source::complete_recipe_niter_graph_source<Output> +
			 pouch_wire_source::complete_recipe_normal_offset_argument_source<Output> +
			 pouch_wire_source::complete_recipe_normal_pointer_constructor_source<
				 Output>);
// Actual list encode emits a uint32 count before the one row, hence both byte
// input ranges exceed1; fitting copy(first,mid,position) has length0. Neither
// actual false-copy occurrence calls __assign_one. Real wrappers/_Num remain.
constexpr size_t pouch_complete_byte_positive_uninitialized_copy_source =
	pouch_wire_source::complete_recipe_uninitialized_copy_a_source<pouch_complete_byte_I,
								       uint8_t *> +
	pouch_wire_source::complete_recipe_std_internal_constant_evaluated_source +
	pouch_wire_source::complete_recipe_uninitialized_copy_source<pouch_complete_byte_I,
								     uint8_t *> +
	pouch_wire_source::complete_recipe_uninitialized_copy_true_source<pouch_complete_byte_I,
									  uint8_t *> +
	pouch_complete_byte_selected_copy_source<uint8_t *>;
constexpr size_t pouch_complete_byte_zero_move_copy_source =
	pouch_wire_source::complete_recipe_copy_own_source<pouch_complete_byte_M, uint8_t *> +
	2 * (pouch_wire_source::complete_recipe_move_miter_own_source<uint8_t> +
	     pouch_wire_source::complete_recipe_move_iterator_base_source<uint8_t> +
	     pouch_wire_source::complete_recipe_generic_miter_source<uint8_t *>)+pouch_wire_source::
			complete_recipe_copy_move_a_own_source<uint8_t *, uint8_t *> +
	3 * pouch_wire_source::complete_recipe_raw_niter_source<uint8_t> +
	pouch_wire_source::complete_recipe_raw_copy_a1_source<uint8_t *, uint8_t *> +
	pouch_wire_source::complete_recipe_raw_copy_a2_source<uint8_t *, uint8_t *> +
	pouch_wire_source::complete_recipe_byte_simple_copy_m_source +
	pouch_wire_source::complete_recipe_std_constant_evaluated_source +
	pouch_wire_source::complete_recipe_raw_niter_wrap_source;
constexpr size_t pouch_complete_byte_zero_uninitialized_move_source =
	pouch_wire_source::complete_recipe_uninitialized_move_own_source<uint8_t> +
	2 * pouch_wire_source::complete_recipe_make_move_iterator_graph_source +
	pouch_wire_source::complete_recipe_uninitialized_copy_a_source<pouch_complete_byte_M,
								       uint8_t *> +
	pouch_wire_source::complete_recipe_std_internal_constant_evaluated_source +
	pouch_wire_source::complete_recipe_uninitialized_copy_source<pouch_complete_byte_M,
								     uint8_t *> +
	pouch_wire_source::complete_recipe_uninitialized_copy_true_source<pouch_complete_byte_M,
									  uint8_t *> +
	pouch_complete_byte_zero_move_copy_source;
constexpr size_t pouch_complete_byte_fitting_range_insert_own_source =
	// this + old_finish2P, position/first/last/mid4I, n/elems_after2N, F.
	2 * sizeof(void *) + 4 * sizeof(pouch_complete_byte_I) + 2 * sizeof(size_t) +
	sizeof(std::forward_iterator_tag);
constexpr size_t pouch_complete_byte_fitting_end_insert_source =
	pouch_wire_source::complete_recipe_byte_insert_own_source<pouch_complete_byte_I> +
	pouch_complete_byte_fitting_range_insert_own_source +
	pouch_wire_source::complete_recipe_byte_const_iterator_conversion_source +
	pouch_wire_source::complete_recipe_normal_base_source<pouch_complete_byte_I> +
	pouch_wire_source::complete_recipe_byte_cbegin_own_source +
	pouch_wire_source::complete_recipe_normal_pointer_constructor_source<pouch_complete_byte_CI> +
	pouch_wire_source::complete_recipe_normal_subtract_own_source<pouch_complete_byte_CI> +
	2 * pouch_wire_source::complete_recipe_normal_base_source<pouch_complete_byte_CI> +
	2 * (pouch_wire_source::complete_recipe_wire_begin_own_source +
	     pouch_wire_source::complete_recipe_normal_pointer_constructor_source<
		     pouch_complete_byte_I> +
	     pouch_wire_source::complete_recipe_normal_offset_argument_source<pouch_complete_byte_I> +
	     pouch_wire_source::complete_recipe_normal_pointer_constructor_source<
		     pouch_complete_byte_I>)+pouch_wire_source::
			complete_recipe_iterator_category_source<pouch_complete_byte_I> +
	pouch_wire_source::complete_recipe_normal_equal_own_source<pouch_complete_byte_I> +
	2 * pouch_wire_source::complete_recipe_normal_base_source<pouch_complete_byte_I> +
	pouch_wire_source::complete_recipe_normal_distance_graph_source<pouch_complete_byte_I> +
	pouch_wire_source::complete_recipe_normal_advance_graph_source<pouch_complete_byte_I> +
	pouch_complete_byte_positive_uninitialized_copy_source +
	pouch_complete_byte_zero_uninitialized_move_source +
	pouch_complete_byte_selected_copy_source<pouch_complete_byte_I> +
	// Two genuine mutable allocator references are retained through the
	// corresponding fitting uninitialized copy/move calls conservatively.
	2 * pouch_wire_source::complete_recipe_vector_mutable_allocator_source<uint8_t> +
	// Internal end()-position plus actual position.base() at zero move.
	pouch_wire_source::complete_recipe_wire_end_own_source +
	pouch_wire_source::complete_recipe_normal_pointer_constructor_source<pouch_complete_byte_I> +
	pouch_wire_source::complete_recipe_normal_subtract_own_source<pouch_complete_byte_I> +
	3 * pouch_wire_source::complete_recipe_normal_base_source<pouch_complete_byte_I> +
	// Actual caller end I is separate through Iâ†’CI conversion. Input begin/end
	// class prvalues directly initialize public insert's first/last formals.
	pouch_wire_source::complete_recipe_wire_end_own_source +
	pouch_wire_source::complete_recipe_normal_pointer_constructor_source<pouch_complete_byte_I> +
	2 * (sizeof(void *) + pouch_wire_source::complete_recipe_normal_pointer_constructor_source<
				      pouch_complete_byte_I>);
#endif
} // namespace

// PRIVATE whole selected map fixed Source candidate; review pending.
// Original APIs/map algorithms remain unchanged. Node heap and real erase
// recursion are dynamic caller-owned terms, never hidden in this fixed sum.
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
using pouch_complete_map = std::map<int, uint64_t>;
using pouch_complete_map_I = pouch_complete_map::iterator;
using pouch_complete_map_CI = pouch_complete_map::const_iterator;
using pouch_complete_map_K = std::less<int>;
using pouch_complete_map_S = std::_Select1st<pouch_complete_map::value_type>;
using pouch_complete_map_R = std::pair<std::_Rb_tree_node_base *, std::_Rb_tree_node_base *>;
using pouch_complete_map_Q1 = std::tuple<const int &>;
using pouch_complete_map_Q0 = std::tuple<>;
constexpr size_t pouch_map_P = sizeof(void *), pouch_map_N = sizeof(size_t),
		 pouch_map_B = sizeof(bool);
constexpr size_t pouch_complete_map_lookup_source =
	// Exact named lookup union from finite declaration trace80P4B+K+S.
	// Six pointer-shaped iterator objects are expressed with their actual
	// types: contains/find each two CIs; mutable at two Is. No invented
	// undeclared iterator COPY/MOVE or qualification-pointer temporary.
	74 * pouch_map_P + 4 * sizeof(pouch_complete_map_CI) + 2 * sizeof(pouch_complete_map_I) +
	4 * pouch_map_B + sizeof(pouch_complete_map_K) + sizeof(pouch_complete_map_S);
constexpr size_t pouch_complete_map_size_source = 2 * (pouch_map_P + pouch_map_N);
constexpr size_t pouch_complete_map_default_source =
	// map/tree/impl/nodeallocator/newallocator/keycompare/header/reset
	// receiver families. Subobjects already live inside sizeof(map).
	8 * pouch_map_P;
constexpr size_t pouch_complete_map_cleanup_source =
	// map/tree/nontrivial impl/allocator4P; drop2P,destroy2P,traits destroy2P,
	// destroy_at<pair>P; put+deallocate9P4N+B; mutable nodeallocator getter2P.
	// Mutable valptr6P and left/right/begin are named exact lookup families.
	// No C++20 newallocator, trivial header/keycompare/pair/node destructor.
	22 * pouch_map_P + 4 * pouch_map_N + pouch_map_B;
constexpr size_t pouch_complete_map_allocate_source =
	// get_node this/result2P; actual node traits/allocator/newallocator/max/
	// ordinary new9P5N+B. get_Node_allocator2P is already selected cleanup
	// getter lane, returns before each later nonrecursive allocation call.
	11 * pouch_map_P + 5 * pouch_map_N + pouch_map_B;
constexpr size_t pouch_complete_map_subscript_source =
	// this/key/mapped-ref result; local i,end operand,RHS returned iterator;
	// actual key comparator and tuple1/tuple0 caller objects. The static
	// piecewise object is passed by ref: no caller temporary tag.
	3 * pouch_map_P + 3 * sizeof(pouch_complete_map_I) + sizeof(pouch_complete_map_K) +
	sizeof(pouch_complete_map_Q1) + sizeof(pouch_complete_map_Q0);
constexpr size_t pouch_complete_map_emplace_source =
	// this+three Args refs4P; positionCI; actual Auto_node(two pointers);
	// pair result __res. The helper's returned pair is this same object.
	6 * pouch_map_P + sizeof(pouch_complete_map_CI) + sizeof(pouch_complete_map_R);
constexpr size_t pouch_complete_map_auto_node_source =
	// ctor this/tree/3args5P, dtorP, key this/key-ref2P, insert thisP plus
	// its by-value pair and named __it. Pair copy ctor is explicitly declared.
	9 * pouch_map_P + sizeof(pouch_complete_map_R) + sizeof(pouch_complete_map_I);
constexpr size_t pouch_complete_map_node_construct_source =
	// create6P,construct5P,traits construct5P,construct_at5P, actual defaulted
	// __aligned_membuf receiverP, global placement count/location/result2P+N.
	// Three real Args forward specializations share their exact6P lane.
	30 * pouch_map_P + pouch_map_N;
constexpr size_t pouch_complete_map_tuple_pair_source =
	// Initial tuple<const int&> fixed tuple/impl/head ctor6P; later actual
	// xvalue tuple/impl/head MOVE6P; empty tuple explicit default receiverP.
	// Public piecewise pair thisP plus its by-value tag/two tuple objects;
	// delegate this/tuple refs3P plus two index-tag actual by-value objects;
	// get/helper/impl head/head base8P and forward<const int&>2P.
	27 * pouch_map_P + sizeof(std::piecewise_construct_t) + sizeof(pouch_complete_map_Q1) +
	sizeof(pouch_complete_map_Q0) + sizeof(std::_Index_tuple<0>) + sizeof(std::_Index_tuple<>);
constexpr size_t pouch_complete_map_hint_source =
	// this/key2P; const-position, mutable pos/before; two real null-pointer
	// materializations bound to fixed pair constructor ref parameters.
	4 * pouch_map_P + sizeof(pouch_complete_map_CI) + 2 * sizeof(pouch_complete_map_I);
constexpr size_t pouch_complete_map_empty_unique_source =
	// Empty-only fallback this/key/x/y4P; j and begin comparison destination;
	// compB. Nonempty search/decrement branches are not reached by this hint.
	4 * pouch_map_P + 2 * sizeof(pouch_complete_map_I) + pouch_map_B;
constexpr size_t pouch_complete_map_insert_rebalance_source =
	// insert_node this/x/p/z4P and insert_leftB; genuine library rebalance
	// three formals/root/xpp/TWO lexical y locals7P+B; local rotations3P each.
	// No exported rotation wrapper or rebalance_for_erase path is selected.
	17 * pouch_map_P + 2 * pouch_map_B;
constexpr size_t pouch_complete_map_position_pair_source =
	// Three distinct declared pair constructors3P each: fixed BasePtr refs,
	// forwarding BasePtr&/BasePtr&, empty fallback LinkPtr&/BasePtr&.
	// Real forward<BasePtr&>/forward<LinkPtr&>2P each; declared pair copy2P.
	15 * pouch_map_P;
constexpr size_t pouch_complete_map_insert_iterator_source =
	// Explicit mutable->const ctor2P, const_cast thisP, mutable --2P,
	// exported decrement2P/local decrement4P, tree beginP,left/rightmost4P.
	// Other pointer ctor/deref/equality/query families are exact lookup lanes.
	16 * pouch_map_P;
constexpr size_t pouch_complete_map_fixed_source =
	pouch_complete_map_lookup_source + pouch_complete_map_size_source +
	pouch_complete_map_default_source + pouch_complete_map_cleanup_source +
	pouch_complete_map_allocate_source + pouch_complete_map_subscript_source +
	pouch_complete_map_emplace_source + pouch_complete_map_auto_node_source +
	pouch_complete_map_node_construct_source + pouch_complete_map_tuple_pair_source +
	pouch_complete_map_hint_source + pouch_complete_map_empty_unique_source +
	pouch_complete_map_insert_rebalance_source + pouch_complete_map_position_pair_source +
	pouch_complete_map_insert_iterator_source;
constexpr size_t pouch_complete_map_erase_frame_source =
	// Actual recursive _M_erase this,x,y. At every callback callers retain
	// precisely the original n+1 frame bound; prospective absent node adds
	// one frame and its exact node heap before allocation. No fixed duplicate.
	3 * pouch_map_P;
#endif
} // namespace

// PRIVATE REVIEW-PENDING actual pure matches graph. This is separate
// from recipe encode/decode full Source: neither codec calls matches().
// Only named matching selected leaves are reused; no whole codec/unused
// alchemy, DTO lifetime or vector mutation inventory supplies spare credit.
namespace duris_craft_recipe_bounded_detail
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
using complete_recipe_matches_root_ci = std::vector<uint64_t>::const_iterator;
constexpr size_t complete_recipe_matches_own_source =
	// terms/payload, entry pointer and hidden range reference; materials,
	// ores,roots and two separate i declarations; last_ore and actual uid;
	// hidden begin/end iterators; bool result. The two returned class values
	// initialize those exact hidden iterator objects by mandatory elision.
	4 * source_P + 5 * source_N + sizeof(uint32_t) + sizeof(uint64_t) +
	2 * sizeof(complete_recipe_matches_root_ci) + source_B;
constexpr size_t complete_recipe_matches_root_iterator_source =
	// begin/end returned CI storage is already the hidden objects above.
	// Real getter receivers and explicitly declared pointer ctors remain.
	2 * (source_P +
	     complete_recipe_normal_pointer_constructor_source<
		     complete_recipe_matches_root_ci>)+complete_recipe_normal_equal_own_source<complete_recipe_matches_root_ci> +
	2 * complete_recipe_normal_base_source<complete_recipe_matches_root_ci> +
	complete_recipe_normal_preincrement_source<complete_recipe_matches_root_ci> +
	complete_recipe_normal_dereference_source<complete_recipe_matches_root_ci>;
constexpr size_t complete_recipe_matches_byte_empty_source =
	// Actual const empty()->begin/end()->operator==. Unlike the root range,
	// these returned CIs are separate real temporary comparison operands.
	complete_recipe_vector_empty_own_source<uint8_t> +
	complete_recipe_vector_const_begin_own_source<uint8_t> +
	complete_recipe_vector_const_end_own_source<uint8_t> +
	2 * complete_recipe_normal_pointer_constructor_source<complete_recipe_byte_const_iterator> +
	complete_recipe_normal_equal_own_source<complete_recipe_byte_const_iterator> +
	2 * complete_recipe_normal_base_source<complete_recipe_byte_const_iterator>;
constexpr size_t complete_recipe_matches_refine_validation_source =
	// Exactly refine_terms_valid's selected descendants; no is_alchemy,
	// u64 empty(), recipe/cost default construction or codec call is reached.
	complete_recipe_refine_succeeded_own_source + complete_recipe_refine_cost_valid_own_source +
	complete_recipe_refine_terms_valid_own_source +
	complete_recipe_refine_cost_compare_own_source +
	complete_recipe_array_const_i32_access_source +
	complete_recipe_array_const_i64_access_source + complete_recipe_array_i32_equal_source +
	complete_recipe_matches_byte_empty_source + complete_recipe_vector_size_source<uint64_t> +
	complete_recipe_vector_const_subscript_source<uint64_t> +
	complete_recipe_string_size_source + complete_recipe_string_empty_source +
	complete_recipe_string_find_own_source + complete_recipe_string_const_data_source +
	complete_recipe_traits_char_find_source;
constexpr size_t complete_recipe_matches_item_array_source =
	// Actual std::array<item_transfer_entry,...>::const operator[] direct
	// this/index/returned-reference; captured GNU13 directly indexes elems.
	// No old _AT_Type::_S_ref or array ctor/dtor is selected.
	2 * source_P + source_N;
constexpr size_t complete_recipe_matches_owner_identity_source =
	// Original item_owner_identity_equal(left,right): two refs/bool result.
	// Scalar field comparisons reach no extra C++ callable helper.
	2 * source_P + source_B;
constexpr size_t complete_recipe_matches_full_source =
	complete_recipe_matches_own_source + complete_recipe_matches_root_iterator_source +
	complete_recipe_matches_refine_validation_source +
	complete_recipe_matches_item_array_source + complete_recipe_matches_owner_identity_source;
#endif
} // namespace duris_craft_recipe_bounded_detail

// PRIVATE top-level declaration Source only, not full profiles.
// Prospective DTOs/controllers/heaps and reached child/library/query bodies
// remain separate; original bodies are unchanged and these entries unselected.
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
using pouch_complete_usage_const_iterator = std::vector<chaos_material_pouch_usage>::const_iterator;
constexpr size_t pouch_complete_encode_wrapper_own_source =
	// mutation/encoded/reserve/context4P; actual catch referenceP;
	// outer/constexpr frames/prefix/total4N; bool returned value.
	5 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_complete_encode_usage_loop_own_source =
	// Hidden actual range reference, begin/end CI, declared used reference.
	2 * sizeof(void *) + 2 * sizeof(pouch_complete_usage_const_iterator);
constexpr size_t pouch_complete_decode_wrapper_own_source =
	// By-value encoded span; mutation/reserve/context3P + catch referenceP.
	// outer/frames/prefix/count/before_size/after_size/index/offset/
	// before_offset9N; declared mode/vnum2U; returned bool.
	sizeof(std::span<const uint8_t>) + 4 * sizeof(void *) + 9 * sizeof(size_t) +
	2 * sizeof(uint64_t) + sizeof(bool);
constexpr size_t pouch_complete_payload_wrapper_own_source =
	// payload/mutation/reserve/context4P, two distinct catch references2P;
	// outer/frames/prefix/retained_count/index5N; returned bool.
	6 * sizeof(void *) + 5 * sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_complete_payload_loops_own_source =
	// Actual indexed entry ref, usage range ref/used ref + hidden begin/end.
	// candidate.usage is mutable despite the const used reference: its
	// actual hidden iterator type is I, distinct from encoder input CI.
	3 * sizeof(void *) + 2 * sizeof(std::vector<chaos_material_pouch_usage>::iterator);
constexpr size_t pouch_complete_append_own_source =
	// New owner/out2P, valueU, size/index2N, bool result; actual prvalue
	// byte argument bound to push's T&& stays genuine caller-owned storage.
	2 * sizeof(void *) + sizeof(uint64_t) + 2 * sizeof(size_t) + sizeof(bool) + sizeof(uint8_t);
constexpr size_t pouch_complete_number_own_source =
	// Original input span formal; offset/size/index3N; value + return2U.
	sizeof(std::span<const uint8_t>) + 3 * sizeof(size_t) + 2 * sizeof(uint64_t);
constexpr size_t pouch_complete_callback_interface_source =
	// Actual caller-owned callback receives bytes by value/context pointer
	// and returns bool. No callback implementation/outer-owned data invented.
	sizeof(size_t) + sizeof(void *) + sizeof(bool);
#endif
} // namespace

// Private actual reached controller/helper Source projection, not full entry.
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
namespace pouch_selected_recipe_source = duris_craft_recipe_bounded_detail;
constexpr size_t pouch_complete_checked_add_source = sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_complete_wire_heap_own_source =
	// this/value/total3P plus bool; typed vector capacity is separate.
	3 * sizeof(void *) + sizeof(bool);
constexpr size_t pouch_complete_wire_item_own_source =
	// this/value/total3P, named actualN, returned bool.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_complete_wire_items_own_source =
	// this/value/total3P plus hidden range and row references2P;
	// actual two const row iterator objects stay here, not primitive methods.
	5 * sizeof(void *) + 2 * sizeof(std::vector<player_item_snapshot>::const_iterator) +
	sizeof(bool);
constexpr size_t pouch_complete_wire_prefix_own_source =
	2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_complete_wire_peak_own_source =
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_complete_wire_fresh_own_source =
	sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_complete_wire_fitting_growth_own_source =
	// Existing growth's count/caller_frames/request3N are reached.
	// The separately declared nextN is inside its unselected realloc arm:
	// original encoder reserve(total) and decoder reserve(count) guarantee
	// all ensuing growth(...,1) checks fit, so it is not new runtime Source.
	2 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_complete_payload_prefix_own_source =
	2 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_complete_payload_peak_own_source =
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_complete_payload_insert_own_source =
	2 * sizeof(void *) + sizeof(size_t) + sizeof(int) + 2 * sizeof(bool);
constexpr size_t pouch_complete_payload_erase_own_source =
	// count/frame bytes2N and total-refP plus bool. Recursive erase frames
	// remain dynamic 3P*(n+1), charged by the unchanged actual prefix logic.
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_complete_snapshot_request_common_own_source =
	3 * (2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool)) + 2 * sizeof(void *) +
	sizeof(size_t) + 2 * sizeof(bool) + 6 * sizeof(void *) + 2 * sizeof(bool) + sizeof(bool) +
	sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_complete_snapshot_current_selected_source =
	pouch_complete_snapshot_request_common_own_source + 2 * sizeof(void *) + sizeof(size_t) +
	sizeof(bool) + 3 * (sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t) +
	sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + 4 * (2 * sizeof(void *)) +
	pouch_complete_snapshot_observation_ed_iterator_source;
constexpr size_t pouch_complete_snapshot_fresh_selected_source =
	pouch_complete_snapshot_request_common_own_source + 2 * sizeof(void *) + sizeof(size_t) +
	sizeof(bool) + 3 * (sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t) +
	pouch_complete_snapshot_observation_ed_iterator_source;
static_assert(pouch_complete_snapshot_current_selected_source ==
	      44 * sizeof(void *) + 10 * sizeof(size_t) + 15 * sizeof(bool));
static_assert(pouch_complete_snapshot_fresh_selected_source ==
	      33 * sizeof(void *) + 10 * sizeof(size_t) + 14 * sizeof(bool));
template <class Iterator> constexpr size_t pouch_complete_typed_range_methods_source =
	// Actual mutable/const vector begin and end receivers2P; two genuine
	// pointer constructor occurrences4P conservatively retained; ++ and
	// dereference2P each, operator==2P+B, two concurrent base refs4P.
	// Hidden range/begin/end/element objects belong to their actual caller.
	16 * sizeof(void *) + sizeof(bool);
constexpr size_t pouch_complete_number_selected_source =
	pouch_complete_number_own_source +
	// Captured span explicitly declares its same-type copy ctor=this/other.
	2 * sizeof(void *) + pouch_selected_recipe_source::complete_recipe_span_subscript_source;
template <class T> constexpr size_t pouch_complete_fitting_growth_query_source =
	pouch_selected_recipe_source::complete_recipe_vector_max_size_source<T> +
	pouch_selected_recipe_source::complete_recipe_vector_size_source<T> +
	pouch_selected_recipe_source::complete_recipe_vector_capacity_source<T>;
constexpr size_t pouch_complete_wire_encode_controller_source =
	// Encoder never registers candidate/row vectors: only byte heap observer
	// branch is selected. All item/row observer routines belong to singleton.
	pouch_complete_wire_heap_own_source +
	pouch_selected_recipe_source::complete_recipe_vector_capacity_source<uint8_t> +
	pouch_complete_wire_prefix_own_source + pouch_complete_wire_peak_own_source +
	pouch_complete_wire_fresh_own_source + pouch_complete_wire_fitting_growth_own_source +
	pouch_complete_fitting_growth_query_source<uint8_t> + pouch_complete_checked_add_source +
	pouch_complete_callback_interface_source;
constexpr size_t pouch_complete_wire_decode_controller_source =
	// Three actually instantiated heap<T> methods: byte, usage, outer row.
	3 * pouch_complete_wire_heap_own_source +
	pouch_selected_recipe_source::complete_recipe_vector_capacity_source<uint8_t> +
	pouch_selected_recipe_source::complete_recipe_vector_capacity_source<
		chaos_material_pouch_usage> +
	pouch_selected_recipe_source::complete_recipe_vector_capacity_source<player_item_snapshot> +
	pouch_complete_wire_item_own_source + pouch_complete_wire_items_own_source +
	pouch_complete_typed_range_methods_source<std::vector<player_item_snapshot>::const_iterator> +
	pouch_complete_snapshot_current_selected_source + pouch_complete_wire_prefix_own_source +
	pouch_complete_wire_peak_own_source + pouch_complete_wire_fresh_own_source +
	pouch_complete_wire_fitting_growth_own_source +
	pouch_complete_fitting_growth_query_source<chaos_material_pouch_usage> +
	pouch_complete_checked_add_source + pouch_complete_callback_interface_source;
#endif
} // namespace

// Private actual per-endpoint parent lifetimes. Source only; physical values,
// prior output heaps and two singleton copies remain in owning controllers.
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
namespace pouch_lifetime_recipe_source = duris_craft_recipe_bounded_detail;
constexpr size_t pouch_complete_encoder_row_member_source =
	// Encoder's two deep row copies reach strings and DA/ED/i32 COPY, then
	// cleanup. No parent row default/move or mutation lifetime is selected.
	pouch_complete_row_copy_vectors_source + pouch_complete_row_member_cleanup_vectors_source +
	pouch_complete_string_copy_cleanup_selected_source +
	// ED generated nontrivial COPY2P is separate. Outer singleton Source owns
	// row COPY2P/destructorP already; do not add those same family scopes here.
	2 * sizeof(void *);
constexpr size_t pouch_complete_decoder_mutation_value_source =
	// Actual plain mutation defaultP, generated mutation assignment3P/dtorP,
	// row move assignment3P/destructorP, and distinct stdmove<row>/<mutation>
	// argument/result2P each. Row before/after={} has no row-default receiver.
	13 * sizeof(void *) + pouch_complete_usage_default_source +
	pouch_complete_usage_cleanup_source + pouch_complete_usage_move_assignment_source +
	pouch_complete_row_member_default_vectors_source +
	pouch_complete_row_member_move_vectors_source +
	// Exact selected strings reach default, row move and cleanup, no COPY.
	pouch_full_string_tail_union_source +
	pouch_lifetime_recipe_source::complete_recipe_string_mutable_allocator_source +
	pouch_lifetime_recipe_source::member_string_default_source;
constexpr size_t pouch_complete_decoder_outer_row_vectors_source =
	// Parent before/after vectors default and cleanup; row members are in
	// selected mutation member Source above. No parent row COPY is reached.
	6 * sizeof(void *) + pouch_full_vector_cleanup_source<player_item_snapshot>;
constexpr size_t pouch_complete_parent_recipe_default_cleanup_source =
	// Actual recipe and unbraced refine_cost DMI default receivers2P, recipe
	// nontrivial cleanupP. No recipe move assignment or trivial field callee.
	3 * sizeof(void *) + 2 * (6 * sizeof(void *)) +
	pouch_lifetime_recipe_source::member_vector_cleanup_source<uint8_t>() +
	pouch_lifetime_recipe_source::member_vector_cleanup_source<uint64_t>() +
	pouch_lifetime_recipe_source::member_string_default_source +
	pouch_lifetime_recipe_source::member_string_destroy_source;
constexpr size_t pouch_complete_parent_payload_plain_default_source =
	// item_transfer_payload pouch_payload; is plain default, unlike Coin {}.
	// Ten genuine default receiver sites: payload, corpse, collector,
	// continuation, native_mobile, reference, recovery, two forests,
	// publication. Native cost/money brace DMIs add no enclosing receiver.
	10 * sizeof(void *) + item_payload_aggregate_default_source;
constexpr size_t pouch_complete_parent_payload_plain_lifetime_source =
	pouch_complete_parent_payload_plain_default_source + item_payload_cleanup_source;
constexpr size_t pouch_complete_parent_payload_continuation_move_source =
	// Original pouch_payload.continuation.data = stdmove(recipe.pouch_mutation).
	// Actual payload/pouch byte vectors, no invented aggregate assignment.
	pouch_lifetime_recipe_source::complete_recipe_vector_move_selected_source<uint8_t> +
	pouch_lifetime_recipe_source::complete_recipe_scalar_move_source<std::vector<uint8_t>>;
#endif
} // namespace

namespace
{
constexpr size_t pouch_complete_recipe_adapter_source =
	4 * sizeof(void *) + sizeof(std::span<const uint8_t>) + sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_complete_ledger_adapter_source =
	4 * sizeof(void *) + 5 * sizeof(size_t) + sizeof(chaos_pouch_ledger_result);
constexpr size_t pouch_complete_list_decode_adapter_own_source =
	// owner/bytes/output/reserve/context, size + source/supplement/initial/
	// prefix/request, returned codec enum. Output/prior rows remain caller-owned.
	5 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(player_snapshot_codec_result);
constexpr size_t pouch_complete_list_decode_adapter_query_source =
	// Actual three distinct lower getter invocations, all same authentic
	// published pure-query shape; result size_t variables live in own above.
	3 * player_item_snapshot_list_decode_bounded_source_query_frame_bytes();
constexpr size_t pouch_complete_payload_clone_adapter_own_source =
	// owner/source/output/reserve/context5P; prefix/request2N; bool result.
	5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_complete_encode_adapter_own_source =
	5 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_complete_decode_adapter_own_source =
	4 * sizeof(void *) + sizeof(std::span<const uint8_t>) + 3 * sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_complete_usage_payload_adapter_own_source =
	5 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool);
}

// PRIVATE complete retained-own composition, independent whole review OPEN.
// These constants exclude physical controllers/DTOs/heaps and all full nested
// self-owning child Source. They DO include actual adapter/profile queries.
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
namespace pouch_composition_recipe_source = duris_craft_recipe_bounded_detail;
constexpr size_t pouch_complete_span_pointer_count_constructor_source =
	// span(this,first,count)=2P+N, to_address(pointer)2P and internal raw
	// __to_address(pointer)2P, dynamic extent storage(this,count)=P+N.
	7 * sizeof(void *) + 2 * sizeof(size_t);
constexpr size_t pouch_complete_vector_span_caller_source =
	// Actual vector data result2P + sizeP+N; returned span object directly
	// initializes child by-value formal. No extra caller span DTO is invented.
	3 * sizeof(void *) + sizeof(size_t) + pouch_complete_span_pointer_count_constructor_source;
constexpr size_t pouch_complete_singleton_own_source =
	4 * sizeof(void *) + 8 * sizeof(size_t) + sizeof(player_snapshot_codec_result);
constexpr size_t pouch_complete_encode_retained_source =
	pouch_complete_encode_wrapper_own_source + pouch_complete_encode_usage_loop_own_source +
	pouch_complete_wire_encode_controller_source + pouch_complete_append_own_source +
	pouch_complete_valid_usage_source +
	pouch_complete_typed_range_methods_source<
		std::vector<chaos_material_pouch_usage>::const_iterator> +
	// Actual old ledger adapter/local/query graph plus actual usage span construction.
	pouch_complete_ledger_adapter_source +
	2 * chaos_pouch_ledger_verify_source_query_frame_bytes() +
	chaos_pouch_ledger_verify_initial_query_frame_bytes() +
	pouch_complete_vector_span_caller_source +
	// Actual singleton helper/query + both CURRENT/fresh roots and outer row
	// constructor/copy/cleanup; member row COPY/cleanup projected separately.
	pouch_complete_singleton_own_source +
	2 * player_item_snapshot_list_encode_source_query_frame_bytes() +
	pouch_complete_snapshot_observation_source + pouch_complete_singleton_vector_source +
	pouch_complete_encoder_row_member_source +
	// Actual byte vector default/cleanup/fresh reserve/fitting push/end insert,
	// successful output move and caller stdmove; no realloc-at-push inventory.
	6 * sizeof(void *) +
	pouch_composition_recipe_source::member_vector_cleanup_source<uint8_t>() +
	pouch_complete_byte_fresh_reserve_source + pouch_complete_byte_fitting_push_source +
	pouch_complete_byte_fitting_end_insert_source +
	pouch_composition_recipe_source::complete_recipe_vector_move_selected_source<uint8_t> +
	pouch_composition_recipe_source::complete_recipe_scalar_move_source<std::vector<uint8_t>> +
	// Byte row lengths, actual usage size. Other identical calls may retain
	// their separately matched Source occurrences conservatively.
	pouch_composition_recipe_source::complete_recipe_vector_size_source<uint8_t> +
	pouch_composition_recipe_source::complete_recipe_vector_size_source<
		chaos_material_pouch_usage>;
constexpr size_t pouch_complete_decode_retained_source =
	// Explicitly declared span<const byte> destructor, shared after return.
	sizeof(void *) + pouch_complete_decode_wrapper_own_source +
	pouch_complete_wire_decode_controller_source + pouch_complete_number_selected_source +
	pouch_composition_recipe_source::complete_recipe_span_size_source +
	pouch_composition_recipe_source::complete_recipe_span_extent_source +
	pouch_composition_recipe_source::complete_recipe_span_data_source +
	pouch_complete_usage_reserve_source + pouch_complete_usage_push_source +
	pouch_complete_decoder_mutation_value_source +
	pouch_complete_decoder_outer_row_vectors_source +
	pouch_complete_list_decode_adapter_own_source +
	pouch_complete_list_decode_adapter_query_source + pouch_complete_encode_adapter_own_source +
	craft_pouch_mutation_encode_complete_source_query_frame_bytes() +
	// Genuine scalar initial getter outP/returnedB.
	sizeof(void *) + sizeof(bool) +
	// before/after size and mutable operator[] plus data used at child binds.
	pouch_composition_recipe_source::complete_recipe_vector_size_source<player_item_snapshot> +
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(void *) +
	// Parent canonical byte vector default/cleanup and actual equal4 graph.
	6 * sizeof(void *) +
	pouch_composition_recipe_source::member_vector_cleanup_source<uint8_t>() +
	pouch_composition_recipe_source::complete_recipe_canonical_equal_graph_source;
constexpr size_t pouch_complete_payload_controller_selected_source =
	// No prefix row-vector/byte registration and no fresh/growth methods are
	// reached by this parent. Recipe heap probes instantiate byte/u64 heap;
	// candidate probes instantiate usage heap and CURRENT row helper.
	3 * pouch_complete_wire_heap_own_source +
	pouch_composition_recipe_source::complete_recipe_vector_capacity_source<uint8_t> +
	pouch_composition_recipe_source::complete_recipe_vector_capacity_source<uint64_t> +
	pouch_composition_recipe_source::complete_recipe_vector_capacity_source<
		chaos_material_pouch_usage> +
	pouch_complete_wire_item_own_source + pouch_complete_snapshot_current_selected_source +
	pouch_complete_wire_prefix_own_source + pouch_complete_payload_prefix_own_source +
	pouch_complete_payload_peak_own_source + pouch_complete_payload_insert_own_source +
	pouch_complete_payload_erase_own_source + pouch_complete_checked_add_source +
	pouch_complete_callback_interface_source + item_payload_current_source +
	// Recipe string capacity is the same actual public char-string capacity
	// family already retained by row CURRENT and payload CURRENT observers.
	pouch_complete_map_fixed_source;
constexpr size_t pouch_complete_usage_getter_query_source =
	// Exact actual internal usage-only full getter expression, no runtime
	// execution of a hypothetical policy getter or cached baseline.
	sizeof(size_t *) + 2 * sizeof(size_t) + sizeof(bool) +
	craft_pouch_mutation_decode_complete_source_query_frame_bytes() + sizeof(void *) +
	sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_complete_from_payload_retained_source =
	// Explicitly declared span<const byte> destructor for actual parent adapters.
	sizeof(void *) + pouch_complete_payload_wrapper_own_source +
	pouch_complete_payload_loops_own_source +
	pouch_complete_payload_controller_selected_source +
	pouch_complete_typed_range_methods_source<std::vector<chaos_material_pouch_usage>::iterator> +
	pouch_complete_decoder_mutation_value_source +
	pouch_complete_parent_recipe_default_cleanup_source +
	pouch_complete_parent_payload_plain_lifetime_source +
	pouch_complete_parent_payload_continuation_move_source +
	// Recipe child adapter uses actual span formal plus declared span copy
	// when passing its lvalue onward; actual construction is separate.
	pouch_complete_recipe_adapter_source + 2 * sizeof(void *) +
	pouch_complete_vector_span_caller_source +
	pouch_composition_recipe_source::complete_recipe_matches_full_source +
	// Plain parent clone adapter owns own scalars only; old clone full and
	// candidate DTO are transient, genuine positive supplement retained.
	pouch_complete_payload_clone_adapter_own_source +
	pouch_complete_usage_payload_adapter_own_source + pouch_complete_usage_getter_query_source +
	sizeof(void *) + sizeof(bool) +
	// Nonrecipe nested complete decode adapter: actual span construction and
	// copy formal constructor, full getter query and scalar initial query.
	pouch_complete_decode_adapter_own_source + 2 * sizeof(void *) +
	pouch_complete_vector_span_caller_source +
	craft_pouch_mutation_decode_complete_source_query_frame_bytes() + sizeof(void *) +
	sizeof(bool) +
	// Reset helper this/output2P and resultB. Its actual mutation temporary
	// is prospective INLINE, with selected member lifetime already above.
	2 * sizeof(void *) + sizeof(bool) +
	// Actual const item entry array index, byte empty and usage size.
	2 * sizeof(void *) + sizeof(size_t) +
	pouch_composition_recipe_source::complete_recipe_matches_byte_empty_source +
	pouch_composition_recipe_source::complete_recipe_vector_size_source<
		chaos_material_pouch_usage>;
#endif
} // namespace

namespace
{
bool pouch_complete_usage_payload_source_frame_bytes(size_t *) noexcept;
}

// PRIVATE BODY/CONTROLLER JOIN ONLY: retained-own Source definitions and
// complete nested entry preflights are still required before selection.
// PRIVATE additive complete owning controllers, not selected yet.
// Entries supply their genuine complete retained-own Source plus actual live
// DTO inline in frames. Child full Source is admitted transiently before entry;
// only actual non-self-owned child supplement remains in child outer.
// Dynamic capacity/alias/node/erase ownership is exactly the original graph.
// Original old controllers and all old APIs are preserved byte-exact.
namespace
{
struct pouch_complete_wire_budget
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
		if (!pouch_wire_add(total, sizeof(*this)) || !pouch_wire_add(total, frames) ||
		    (candidate &&
		     (!heap(candidate->usage, total) || !item(candidate->before, total) ||
		      !item(candidate->after, total))) ||
		    (before && !items(*before, total)) || (after && !items(*after, total)) ||
		    (bytes && !heap(*bytes, total)) ||
		    (before_bytes && !heap(*before_bytes, total)) ||
		    (after_bytes && !heap(*after_bytes, total)) || !pouch_wire_add(total, extra))
			return false;
		output = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
	template <typename T> bool fresh(size_t count, size_t extra = 0) const noexcept
	{
		size_t request = extra;
		return count <= SIZE_MAX / sizeof(T) &&
		       pouch_wire_add(request, count * sizeof(T)) && peak(request);
	}
	template <typename T> bool growth(const std::vector<T> &value, size_t count,
					  size_t caller_frames = 0) const noexcept
	{
		size_t request = 0;
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
		return pouch_wire_add(request, caller_frames) && peak(request);
	}
};
struct pouch_complete_payload_budget
{
	pouch_complete_wire_budget wire;
	const craft_recipe_continuation *recipe = nullptr;
	const item_transfer_payload *payload = nullptr;
	const pouch_payload_map *consumed = nullptr;
	bool prefix(size_t &output, size_t extra = 0) const noexcept
	{
		size_t total = 0, actual = 0;
		if (!wire.prefix(total) || !pouch_wire_add(total, sizeof(*this) - sizeof(wire)) ||
		    (recipe &&
		     (!wire.heap(recipe->pouch_mutation, total) ||
		      !wire.heap(recipe->refine_root_order, total) ||
		      (recipe->refine_material_name.capacity() > 15 &&
		       (recipe->refine_material_name.capacity() == SIZE_MAX ||
			!pouch_wire_add(total, recipe->refine_material_name.capacity() + 1))))) ||
		    (payload && (!item_transfer_payload_current_heap_bytes(*payload, &actual) ||
				 !pouch_wire_add(total, actual))) ||
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
		if (!peak())
			return false;
		const bool missing = !value.contains(key);
		size_t extra = 0;
		if (missing && (!pouch_wire_add(extra, sizeof(pouch_payload_node)) ||
				!pouch_wire_add(extra, 3 * sizeof(void *))))
			return false;
		// Current n+1 erase recursion already lives in prefix. One new node
		// adds precisely one prospective recursive frame before allocation.
		return peak(extra);
	}
};
} // namespace

namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
// BODY MAPPING ONLY: complete pouch owning Source remains unsealed. This
// actual recipe child handoff is private and must not be selected independently.

bool pouch_complete_recipe_decode_child(pouch_complete_payload_budget &owner,
					std::span<const uint8_t> bytes,
					craft_recipe_continuation *recipe,
					pouch_wire_reserve_fn reserve, void *context) noexcept
{
	size_t prefix = 0;
	// Caller already admitted actual complete child Source+initial+this
	// adapter before materializing its span/formals. Only genuine parent and
	// adapter Source remains in outer; the recipe controller owns its Source.
	return owner.prefix(prefix) && craft_recipe_continuation_decode_complete_bounded(
					       bytes, recipe, reserve, context, prefix);
}

chaos_pouch_ledger_result pouch_complete_ledger_verify_child(pouch_complete_wire_budget &owner,
							     const craft_pouch_mutation &mutation,
							     pouch_wire_reserve_fn reserve,
							     void *context) noexcept
{
	size_t source = 0, supplement = 0, initial = 0, prefix = 0, request = 0;
	// Caller preadmits actual adapter and both complete pure getter graphs
	// before this entry. Query scalar outputs are these authentic named locals.
	if (!chaos_pouch_ledger_verify_source_frame_bytes(&source) ||
	    !chaos_pouch_ledger_verify_source_supplement_frame_bytes(&supplement) ||
	    !chaos_pouch_ledger_verify_initial_inline_bytes(&initial) ||
	    !pouch_wire_add(source, initial))
		return chaos_pouch_ledger_result::capacity;
	request = source;
	if (!owner.peak(request))
		return chaos_pouch_ledger_result::capacity;
	// Full+initial is transient. The unchanged old ledger owns its original
	// named reservations; retain only its actual missing positive supplement.
	request = supplement;
	if (!owner.prefix(prefix, request))
		return chaos_pouch_ledger_result::capacity;
	return chaos_pouch_ledger_verify_bounded(
		mutation.before, mutation.after,
		std::span<const chaos_material_pouch_usage>(mutation.usage.data(),
							    mutation.usage.size()),
		mutation.mode, reserve, context, prefix);
}
#endif
} // namespace

namespace
{
bool pouch_complete_wire_append(pouch_complete_wire_budget &owner, std::vector<uint8_t> &out,
				uint64_t value, size_t size)
{
	for (size_t index = 0; index < size; ++index)
	{
		if (!owner.growth(out, 1))
			return false;
		out.push_back(static_cast<uint8_t>(value >> (index * 8)));
	}
	return true;
}

player_snapshot_codec_result pouch_complete_wire_singleton(const player_item_snapshot &source,
							   std::vector<uint8_t> *output,
							   pouch_wire_reserve_fn reserve,
							   void *context, size_t outer)
{
	// Complete parent Source already owns this helper, matching row/string/
	// vector copies and queries. These are only its genuine live DTOs.
	constexpr size_t frame = sizeof(std::initializer_list<player_item_snapshot>) +
				 sizeof(std::vector<player_item_snapshot>) +
				 2 * sizeof(player_item_snapshot);
	size_t total = outer, request = 0;
	if (!reserve || !pouch_wire_add(total, frame) || !reserve(total, context) ||
	    !player_item_snapshot_fresh_copy_request_bytes(source, &request) ||
	    !pouch_wire_add(total, request) || !pouch_wire_add(total, request) ||
	    !reserve(total, context))
		return player_snapshot_codec_result::allocation_failure;
	const std::initializer_list<player_item_snapshot> singleton{ source };
	const std::vector<player_item_snapshot> items(singleton);
	// Vector's actual allocated row replaces its prospective inline row once.
	size_t current = outer, actual = 0;
	if (!pouch_wire_add(current, frame - sizeof(player_item_snapshot)) ||
	    items.capacity() > SIZE_MAX / sizeof(player_item_snapshot) ||
	    !pouch_wire_add(current, items.capacity() * sizeof(player_item_snapshot)) ||
	    !player_item_snapshot_current_heap_bytes(*singleton.begin(), &actual) ||
	    !pouch_wire_add(current, actual) ||
	    !player_item_snapshot_current_heap_bytes(items.front(), &actual) ||
	    !pouch_wire_add(current, actual))
		return player_snapshot_codec_result::allocation_failure;
	size_t child_source = 0, child_initial = 0;
	if (!player_item_snapshot_list_encode_source_frame_bytes(&child_source) ||
	    !player_item_snapshot_list_encode_initial_inline_bytes(&child_initial))
		return player_snapshot_codec_result::allocation_failure;
	request = current;
	if (!pouch_wire_add(request, child_source) || !pouch_wire_add(request, child_initial) ||
	    !reserve(request, context) || !pouch_wire_add(current, child_source))
		return player_snapshot_codec_result::allocation_failure;
	// Published encode Source is its actual retained supplement contract;
	// unlike bounded decode it exposes no different supplementary getter.
	return player_item_snapshot_list_encode_bounded(items, output, reserve, context, current);
}

bool pouch_complete_payload_reset(pouch_complete_payload_budget &owner,
				  craft_pouch_mutation *output) noexcept
{
	// Actual temporary compound mutation has two default rows and a usage
	// vector. Its true lifetime/default/move/destructor scopes are admitted
	// before construction and the exact original successful reset.
	if (!owner.peak(sizeof(craft_pouch_mutation)))
		return false;
	*output = {};
	return true;
}

} // namespace

// PRIVATE complete-controller list-child preentry adapter, unselected.
// Parent frames already retain exact adapter own plus getter-query scopes;
// full lower Source+initial is transient, and only its actual supplement
// remains across the unchanged original BOUNDED decoder (not unbounded API).
namespace
{
player_snapshot_codec_result
pouch_complete_list_decode_child(pouch_complete_wire_budget &owner, const uint8_t *bytes,
				 size_t size, std::vector<player_item_snapshot> *output,
				 pouch_wire_reserve_fn reserve, void *context) noexcept
{
	size_t source = 0, supplement = 0, initial = 0, prefix = 0, request = 0;
	if (!player_item_snapshot_list_decode_bounded_source_frame_bytes(&source) ||
	    !player_item_snapshot_list_decode_bounded_source_supplement_frame_bytes(&supplement) ||
	    !player_item_snapshot_list_decode_bounded_initial_inline_bytes(&initial))
		return player_snapshot_codec_result::allocation_failure;
	request = source;
	if (!pouch_wire_add(request, initial) || !owner.prefix(prefix, request) || !reserve ||
	    !reserve(prefix, context) || !owner.prefix(prefix, supplement))
		return player_snapshot_codec_result::allocation_failure;
	return player_item_snapshot_list_decode_bounded(bytes, size, output, reserve, context,
							prefix);
}

} // namespace

// PRIVATE clone preentry join for complete parent; full parent still OPEN.
namespace
{
bool pouch_complete_payload_clone_child(pouch_complete_payload_budget &owner,
					const item_transfer_payload &source,
					item_transfer_payload *output,
					pouch_wire_reserve_fn reserve, void *context) noexcept
{
	// Parent's retained-own Source already owns actual adapter scopes before
	// entry. Clone's candidate INLINE is separate from declared Source.
	size_t prefix = 0, request = item_payload_clone_leaf_required_preentry;
	if (!owner.peak(request) ||
	    !owner.prefix(prefix, item_payload_clone_leaf_supplement_source))
		return false;
	// The unchanged old child owns the exact candidate and named old Source;
	// only genuine missing supplement survives the child outer handoff.
	return item_transfer_payload_clone_bounded(source, output, reserve, context, prefix);
}

} // namespace

// PRIVATE owning nested-child preflight; no full parent selection yet.
namespace
{
// Parent retained-own Source owns the real adapter and profile query scopes.
// Full prospective child Source+initial is transient; the new child owns its
// own Source after entry, so its parent outer carries no duplicated full total.
bool pouch_complete_encode_child(pouch_complete_wire_budget &owner,
				 const craft_pouch_mutation &input, std::vector<uint8_t> *output,
				 pouch_wire_reserve_fn reserve, void *context) noexcept
{
	size_t source = 0, initial = 0, prefix = 0;
	if (!craft_pouch_mutation_encode_complete_source_frame_bytes(&source) ||
	    !craft_pouch_mutation_encode_complete_initial_inline_bytes(&initial) ||
	    !pouch_wire_add(source, initial) || !owner.peak(source) || !owner.prefix(prefix))
		return false;
	return craft_pouch_mutation_encode_complete_bounded(input, output, reserve, context,
							    prefix);
}
bool pouch_complete_decode_child(pouch_complete_payload_budget &owner,
				 std::span<const uint8_t> input, craft_pouch_mutation *output,
				 pouch_wire_reserve_fn reserve, void *context) noexcept
{
	size_t source = 0, initial = 0, prefix = 0;
	if (!craft_pouch_mutation_decode_complete_source_frame_bytes(&source) ||
	    !craft_pouch_mutation_decode_complete_initial_inline_bytes(&initial) ||
	    !pouch_wire_add(source, initial) || !owner.peak(source) || !owner.prefix(prefix))
		return false;
	return craft_pouch_mutation_decode_complete_bounded(input, output, reserve, context,
							    prefix);
}
bool pouch_complete_usage_payload_child(pouch_complete_payload_budget &owner,
					const item_transfer_payload &input,
					craft_pouch_mutation *output, pouch_wire_reserve_fn reserve,
					void *context) noexcept
{
	size_t source = 0, initial = 0, prefix = 0;
	// This call is reached only after the original branch writes exactly
	// craft_pouch_usage. It cannot recurse into another recipe branch.
	// The internal prospective usage profile is genuine usage-own+decode;
	// it does not reserve another complete recipe branch merely by type.
	if (!pouch_complete_usage_payload_source_frame_bytes(&source) ||
	    !craft_pouch_mutation_from_payload_complete_initial_inline_bytes(&initial) ||
	    !pouch_wire_add(source, initial) || !owner.peak(source) || !owner.prefix(prefix))
		return false;
	return craft_pouch_mutation_from_payload_complete_bounded(input, output, reserve, context,
								  prefix);
}

} // namespace

bool craft_pouch_mutation_encode_complete_bounded(const craft_pouch_mutation &mutation,
						  std::vector<uint8_t> *encoded,
						  bool (*reserve)(size_t, void *) noexcept,
						  void *context, size_t outer) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)reserve;
	(void)context;
	(void)outer;
	return false;
#else

	constexpr size_t frames =
		pouch_complete_encode_retained_source + 3 * sizeof(std::vector<uint8_t>);
	pouch_complete_wire_budget owner{ reserve, context, outer, frames };
	size_t prefix = 0;
	if (!owner.peak() || !owner.prefix(prefix))
		return false;

	if (!encoded || !valid_usage(mutation.usage) ||
	    mutation.before.parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
	    (!owner.peak(0) ?
		     chaos_pouch_ledger_result::capacity :
		     pouch_complete_ledger_verify_child(owner, mutation, reserve, context)) !=
		    chaos_pouch_ledger_result::ok)
		return false;
	try
	{
		std::vector<uint8_t> before, after;
		owner.before_bytes = &before;
		owner.after_bytes = &after;
		if (!owner.prefix(prefix))
			return false;
		if (pouch_complete_wire_singleton(mutation.before, &before, reserve, context,
						  prefix) != player_snapshot_codec_result::ok ||
		    (!owner.prefix(prefix) ? player_snapshot_codec_result::allocation_failure :
					     pouch_complete_wire_singleton(
						     mutation.after, &after, reserve, context,
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
		if (!pouch_complete_wire_append(owner, candidate, version, 4))
			return false;
		if (!pouch_complete_wire_append(owner, candidate,
						static_cast<uint32_t>(mutation.mode), 4))
			return false;
		if (!pouch_complete_wire_append(owner, candidate, mutation.usage.size(), 4))
			return false;
		if (!pouch_complete_wire_append(owner, candidate, before.size(), 4))
			return false;
		if (!pouch_complete_wire_append(owner, candidate, after.size(), 4))
			return false;
		// Genuine used reference survives each nested append request.
		for (const auto &used : mutation.usage)
		{
			if (!pouch_complete_wire_append(owner, candidate,
							static_cast<uint32_t>(used.vnum), 4))
				return false;
			if (!pouch_complete_wire_append(owner, candidate, used.count, 8))
				return false;
		}
		if (!owner.growth(candidate, before.size()))
			return false;
		candidate.insert(candidate.end(), before.begin(), before.end());
		if (!owner.growth(candidate, after.size()))
			return false;
		candidate.insert(candidate.end(), after.begin(), after.end());
		if (!owner.peak())
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
bool craft_pouch_mutation_decode_complete_bounded(std::span<const uint8_t> encoded,
						  craft_pouch_mutation *mutation,
						  bool (*reserve)(size_t, void *) noexcept,
						  void *context, size_t outer) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)reserve;
	(void)context;
	(void)outer;
	return false;
#else

	constexpr size_t frames = pouch_complete_decode_retained_source +
				  sizeof(craft_pouch_mutation) +
				  2 * sizeof(std::vector<player_item_snapshot>) +
				  sizeof(std::vector<uint8_t>) + sizeof(chaos_material_pouch_usage);
	pouch_complete_wire_budget owner{ reserve, context, outer, frames };
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
		if (pouch_complete_list_decode_child(owner, encoded.data() + before_offset,
						     before_size, &before, reserve,
						     context) != player_snapshot_codec_result::ok ||
		    (!owner.prefix(prefix) ?
			     player_snapshot_codec_result::allocation_failure :
			     pouch_complete_list_decode_child(
				     owner, encoded.data() + before_offset + before_size,
				     after_size, &after, reserve, context)) !=
			    player_snapshot_codec_result::ok ||
		    before.size() != 1 || after.size() != 1)
			return false;
		if (!owner.peak())
			return false;
		candidate.before = std::move(before[0]);
		if (!owner.peak())
			return false;
		candidate.after = std::move(after[0]);
		std::vector<uint8_t> canonical;
		owner.bytes = &canonical;
		if (!owner.prefix(prefix))
			return false;
		if (!pouch_complete_encode_child(owner, candidate, &canonical, reserve, context) ||
		    (!owner.peak() || !std::equal(canonical.begin(), canonical.end(),
						  encoded.begin(), encoded.end())))
			return false;
		if (!owner.peak())
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
bool craft_pouch_mutation_from_payload_complete_bounded(const item_transfer_payload &payload,
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

	constexpr size_t frames = pouch_complete_from_payload_retained_source + 0;
	pouch_complete_payload_budget owner{ { reserve, context, outer, frames } };
	size_t prefix = 0;
	if (!owner.peak())
		return false;

	if (!mutation)
		return false;
	if (payload.continuation.kind == item_transfer_continuation_kind::craft_recipe)
	{
		owner.wire.frames += sizeof(craft_recipe_continuation);
		if (!owner.peak())
			return false;
		craft_recipe_continuation recipe;
		owner.recipe = &recipe;
		if (!owner.prefix(prefix))
			return false;
		if (!owner.peak(
			    duris_craft_recipe_bounded_detail::complete_recipe_decode_full_source +
			    sizeof(duris_craft_recipe_bounded_detail::complete_recipe_budget)) ||
		    !pouch_complete_recipe_decode_child(
			    owner,
			    std::span<const uint8_t>(payload.continuation.data.data(),
						     payload.continuation.data.size()),
			    &recipe, reserve, context) ||
		    !craft_recipe_continuation_matches(recipe, payload))
			return false;
		if (recipe.pouch_mutation.empty())
		{
			return pouch_complete_payload_reset(
				owner, mutation); // original empty recipe reset
		}
		try
		{
			owner.wire.frames += sizeof(item_transfer_payload);
			if (!owner.peak())
				return false;
			item_transfer_payload pouch_payload;
			owner.payload = &pouch_payload;
			if (!owner.prefix(prefix) ||
			    !pouch_complete_payload_clone_child(owner, payload, &pouch_payload,
								reserve, context))
				return false;
			pouch_payload.continuation.kind =
				item_transfer_continuation_kind::craft_pouch_usage;
			if (!owner.peak())
				return false;
			pouch_payload.continuation.data = std::move(recipe.pouch_mutation);
			return owner.prefix(prefix) &&
			       pouch_complete_usage_payload_child(owner, pouch_payload, mutation,
								  reserve, context);
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
		return pouch_complete_payload_reset(owner,
						    mutation); // original absent continuation reset
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
		owner.wire.frames += sizeof(craft_pouch_mutation) + sizeof(pouch_payload_map);
		if (!owner.peak())
			return false;
		craft_pouch_mutation candidate;
		owner.wire.candidate = &candidate;
		if (!owner.prefix(prefix))
			return false;
		if (!pouch_complete_decode_child(
			    owner,
			    std::span<const uint8_t>(payload.continuation.data.data(),
						     payload.continuation.data.size()),
			    &candidate, reserve, context))
			return false;
		size_t retained_count = 0;
		std::map<int, uint64_t> consumed;
		owner.consumed = &consumed;
		// Actual entry/used references remain live through nested callbacks.
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
			if (!owner.peak())
				return false;
			for (const auto &used : candidate.usage)
				if (!consumed.contains(used.vnum) ||
				    consumed.at(used.vnum) != used.count)
					return false;
		}
		if (!owner.peak())
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
namespace
{
bool pouch_complete_usage_payload_source_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	size_t child = 0, total = pouch_complete_from_payload_retained_source;
	// Runtime entry still initializes its unconditional retained-own constant.
	// The known usage kind omits only unreachable nested recipe/clone calls,
	// not any byte retained by that actual owner/controller entry.
	if (!craft_pouch_mutation_decode_complete_source_frame_bytes(&child) ||
	    !pouch_wire_add(total, child))
		return false;
	*bytes = total;
	return true;
#else
	return false;
#endif
}
constexpr size_t pouch_complete_usage_payload_source_query_frame_bytes() noexcept
{
	return sizeof(size_t *) + 2 * sizeof(size_t) + sizeof(bool) +
	       craft_pouch_mutation_decode_complete_source_query_frame_bytes() + sizeof(void *) +
	       sizeof(size_t) + sizeof(bool);
}
} // namespace
bool craft_pouch_mutation_encode_complete_source_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	size_t ledger = 0, list = 0, total = pouch_complete_encode_retained_source;
	if (!chaos_pouch_ledger_verify_source_frame_bytes(&ledger) ||
	    !player_item_snapshot_list_encode_source_frame_bytes(&list) ||
	    !pouch_wire_add(total, ledger) || !pouch_wire_add(total, list))
		return false;
	*bytes = total;
	return true;
#else
	return false;
#endif
}
bool craft_pouch_mutation_decode_complete_source_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	size_t list = 0, encoder = 0, total = pouch_complete_decode_retained_source;
	if (!player_item_snapshot_list_decode_bounded_source_frame_bytes(&list) ||
	    !craft_pouch_mutation_encode_complete_source_frame_bytes(&encoder) ||
	    !pouch_wire_add(total, list) || !pouch_wire_add(total, encoder))
		return false;
	// Actual decoder parent remains live through its canonical encoder.
	// Decoder controller retains only decode-own; nested encoder self-owns
	// encoder Source exactly once. Public prospective full composes both.
	*bytes = total;
	return true;
#else
	return false;
#endif
}
bool craft_pouch_mutation_from_payload_complete_source_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	size_t usage = 0,
	       total = pouch_complete_from_payload_retained_source +
		       duris_craft_recipe_bounded_detail::complete_recipe_decode_full_source +
		       item_payload_clone_leaf_full_source;
	if (!pouch_complete_usage_payload_source_frame_bytes(&usage) ||
	    !pouch_wire_add(total, usage))
		return false;
	// Recipe ancestor and actual second usage-kind invocation overlap.
	// Its own constant appears twice because these are two genuine owners.
	// Sequential recipe and clone Source closures are conservatively summed;
	// clone candidate DTO remains separately in required-preentry INLINE.
	*bytes = total;
	return true;
#else
	return false;
#endif
}

bool craft_pouch_mutation_encode_complete_source_supplement_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	*bytes = 0;
	return true;
#else
	return false;
#endif
}

bool craft_pouch_mutation_encode_complete_initial_inline_bytes(size_t *bytes) noexcept
{
	if (!bytes)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	*bytes = sizeof(pouch_complete_wire_budget);
	return true;
#else
	return false;
#endif
}

bool craft_pouch_mutation_decode_complete_source_supplement_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	*bytes = 0;
	return true;
#else
	return false;
#endif
}

bool craft_pouch_mutation_decode_complete_initial_inline_bytes(size_t *bytes) noexcept
{
	if (!bytes)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	*bytes = sizeof(pouch_complete_wire_budget);
	return true;
#else
	return false;
#endif
}

bool craft_pouch_mutation_from_payload_complete_source_supplement_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	*bytes = 0;
	return true;
#else
	return false;
#endif
}

bool craft_pouch_mutation_from_payload_complete_initial_inline_bytes(size_t *bytes) noexcept
{
	if (!bytes)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	*bytes = sizeof(pouch_complete_payload_budget);
	return true;
#else
	return false;
#endif
}
