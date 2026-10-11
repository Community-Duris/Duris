#include "combat/chaos_pouch_ledger.h"

#include "core/structs.h"
#include "core/utils.h"
#include "world/vnum.obj.h"
#include "economy/tradeskill.h"
#include "player/player_snapshot_codec.h"

#include <array>
#include <algorithm>
#include <functional>
#include <initializer_list>
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

namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
using pouch_reserve_fn = bool (*)(size_t, void *) noexcept;
bool pouch_add(size_t &value, size_t extra) noexcept
{
	if (extra > SIZE_MAX - value)
		return false;
	value += extra;
	return true;
}
template <typename T> bool pouch_vector_heap(const std::vector<T> &value, size_t &bytes) noexcept
{
	return value.capacity() <= SIZE_MAX / sizeof(T) &&
	       pouch_add(bytes, value.capacity() * sizeof(T));
}
bool pouch_string_heap(const std::string &value, size_t &bytes) noexcept
{
	return value.capacity() <= 15 ||
	       (value.capacity() < SIZE_MAX && pouch_add(bytes, value.capacity() + 1));
}
bool pouch_string_copy(const std::string &value, size_t &bytes) noexcept
{
	// GCC13 copy construction _M_construct(forward): fresh exact length.
	return value.size() <= 15 ||
	       (value.size() < SIZE_MAX && pouch_add(bytes, value.size() + 1));
}
template <typename T> bool pouch_vector_copy(const std::vector<T> &value, size_t &bytes) noexcept
{
	return value.size() <= SIZE_MAX / sizeof(T) && pouch_add(bytes, value.size() * sizeof(T));
}
bool pouch_description_heap(const player_item_extra_description_snapshot &value, size_t &bytes,
			    bool copied = false) noexcept
{
	return (copied ? pouch_string_copy(value.keyword, bytes) :
			 pouch_string_heap(value.keyword, bytes)) &&
	       (copied ? pouch_string_copy(value.description, bytes) :
			 pouch_string_heap(value.description, bytes)) &&
	       (copied ? pouch_vector_copy(value.spell_ids, bytes) :
			 pouch_vector_heap(value.spell_ids, bytes));
}
bool pouch_item_heap(const player_item_snapshot &item, size_t &bytes, bool copied = false) noexcept
{
	size_t actual = 0;
	return (copied ? player_item_snapshot_fresh_copy_request_bytes(item, &actual) :
			 player_item_snapshot_current_heap_bytes(item, &actual)) &&
	       pouch_add(bytes, actual);
}
constexpr size_t pouch_allocator_frames =
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
constexpr size_t pouch_copy_frames =
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
constexpr size_t pouch_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t pouch_default_frames =
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
constexpr size_t pouch_vector_frames =
	pouch_allocator_frames + pouch_copy_frames + pouch_relocate_frames + pouch_default_frames +
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
constexpr size_t pouch_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + pouch_allocator_frames;

// Nontrivial row/description construction and destruction are source scopes,
// not heap metadata. The nested member objects already live in sizeof(row).
constexpr size_t pouch_nontrivial_frames =
	// default_n_1<false>: first/n/cur/return; _Construct/addressof/placement.
	3 * sizeof(void *) + sizeof(size_t) + 5 * sizeof(void *) + sizeof(size_t) +
	// Actual aggregate row and description this, four row strings and two
	// description strings: string()/allocator hider/use-local-data/set-length.
	2 * sizeof(void *) +
	6 * (6 * sizeof(void *) + sizeof(size_t) + sizeof(char) + sizeof(std::allocator<char>)) +
	// row/description nested vector()/Vector_base()/Vector_impl()/data() and
	// allocator return carriers. Three source member vector types.
	3 * (5 * sizeof(void *) + sizeof(std::allocator<int32_t>)) +
	// Nontrivial _Destroy range/aux::__destroy/destroy_at/__addressof; actual
	// row/description destructor this then six string destructors/dispose/
	// _M_is_local/_M_destroy and three nested vector destroy/deallocate scopes.
	8 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(void *) +
	6 * (5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) + 3 * pouch_allocator_frames;
// Fitting _M_replace calls _M_disjunct(this,s). Both actual less pointer
// temporaries can coexist through the full || expression; their operator()
// has this/x/y/result and is_constant_evaluated result. Data/size queries.
constexpr size_t pouch_disjunct_frames = 2 * sizeof(void *) + sizeof(bool) +
					 2 * sizeof(std::less<const char *>) +
					 2 * (3 * sizeof(void *) + 2 * sizeof(bool)) +
					 2 * (2 * sizeof(void *)) + sizeof(void *) + sizeof(size_t);
constexpr size_t pouch_string_frames =
	pouch_disjunct_frames +
	// assign(s,n): this/s/n/ref-return; _M_replace(this,pos,len1,s,len2),
	// old_size/new_size/p/how_much/ref-return, actual length checks/queries.
	3 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + 5 * sizeof(size_t) +
	6 * (sizeof(void *) + sizeof(size_t)) + sizeof(bool) +
	// _M_mutate(this,pos,len1,s,len2), how_much/new_capacity/r;
	// _M_create(this,capacityref,oldcapacity), max_size, allocation return.
	3 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(void *) + sizeof(size_t) +
	pouch_allocator_frames +
	// _S_copy(d,s,n), traits::copy(s1,s2,n) returned pointer and memcopy
	// argument/result carriers; one-character assign reference/char scopes.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(void *) + sizeof(char) +
	// old block dispose/destroy plus data/capacity/set-length and final NUL.
	6 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) + sizeof(char);
// Genuine vector(n,value,allocator) constructor scopes, before fill:
// vector this/n/value-reference/allocator-reference and default allocator;
// _S_check_init_len n/a/result and its _Tp allocator copy; _Vector_base
// this/n/a, _Vector_impl this/a and allocator copy, _Vector_impl_data this;
// _M_create_storage this/n. Existing allocator profile owns _S_max_size.
constexpr size_t pouch_size_constructor_frames =
	3 * sizeof(void *) + sizeof(size_t) + sizeof(std::allocator<size_t>) + sizeof(void *) +
	2 * sizeof(size_t) + sizeof(std::allocator<size_t>) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) +
	sizeof(size_t);
struct pouch_decode_workspace;

constexpr size_t pouch_copy_constructor_frames =
	// Non-ledger extra-description copy this/source and its two strings.
	2 * sizeof(void *) +
	// basic_string(copy)/_M_construct(forward): this/source/allocator,
	// begin/end/distance/capacity and actual local construction guard.
	2 * (8 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::allocator<char>)) +
	// The description spell vector copy constructor/_Vector_base/_M_create_storage
	// and uninitialized_copy_a scopes, with actual allocator/result carriers.
	(11 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::allocator<int32_t>));
// Genuine vector(initializer_list,allocator) path: stl_vector.h:678,
// _M_range_initialize(forward):1687. The by-value __l is separate from the
// caller's singleton object and its backing row; tags are actual objects.
constexpr size_t pouch_singleton_constructor_frames =
	// vector this, __l, allocator reference and default allocator; actual
	// random-access tag argument converted to the forward tag parameter.
	2 * sizeof(void *) + sizeof(std::initializer_list<player_item_snapshot>) +
	sizeof(std::allocator<player_item_snapshot>) + sizeof(std::random_access_iterator_tag) +
	sizeof(std::forward_iterator_tag) +
	// _Vector_base(a), _Vector_impl(a), allocator copy and impl-data default.
	3 * (2 * sizeof(void *)) + sizeof(void *) +
	// initializer_list private constructor this/array/length; begin/end
	// this/result, nested end->begin and size this/size-result.
	2 * sizeof(void *) + sizeof(size_t) + 3 * (2 * sizeof(void *)) + sizeof(void *) +
	sizeof(size_t) +
	// _M_range_initialize this/first/last, actual __n. Its forward tag was
	// counted above. distance/__distance parameters/results remain owned
	// by pouch_copy_frames, as does genuine uninitialized_copy_a.
	3 * sizeof(void *) + sizeof(size_t) +
	// _S_check_init_len n/a/result, real _Tp_alloc_type(a) temporary and
	// allocator-copy this/source. _S_max_size and allocate are already
	// pouch_allocator_frames; no fill constructor substitutes this path.
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(std::allocator<player_item_snapshot>) +
	2 * sizeof(void *) +
	// Two real _M_get_Tp_allocator this/reference-result calls.
	2 * (2 * sizeof(void *));
// Exact callers above the already-owned _M_mutate/_M_create/copy/length
// chain. Keep these additional carriers separate from the pinned common
// assign/_M_replace profile; the unchanged allocation simulation is shared.
constexpr size_t pouch_append_caller_frames =
	// operator+(string&&,string&&): lhs/rhs, real __use_rhs and __size,
	// actual is_always_equal temporary. Returned strings are owned by
	// pouch_record_request, not charged a second time here.
	2 * sizeof(void *) + sizeof(bool) + sizeof(size_t) +
	sizeof(std::allocator_traits<std::allocator<char>>::is_always_equal) +
	// operator+(string&&,const char*): lhs/rhs; append(const string&)
	// this/str/reference-result; append(const char*) this/s/__n/ref-result;
	// append(const char*,n) this/s/n/reference-result.
	2 * sizeof(void *) + 3 * sizeof(void *) + 3 * sizeof(void *) + sizeof(size_t) +
	3 * sizeof(void *) + sizeof(size_t) +
	// _M_append this/s/n, real __len, reference-result. Its nested mutation,
	// checks, copy, data/capacity and set-length carriers remain in the
	// complete common pouch_string_frames above.
	3 * sizeof(void *) + 2 * sizeof(size_t);
constexpr size_t pouch_decimal_frames =
	// to_string(unsigned long/long long) val and returned string, char fill
	// constructor arguments; __to_chars_len's value/base/n/b2/b3/b4.
	sizeof(uint64_t) + sizeof(std::string) + sizeof(size_t) + sizeof(char) + sizeof(uint64_t) +
	4 * sizeof(unsigned int) + sizeof(unsigned long) +
	// __to_chars_10_impl first/len/val, local digits[201], pos and num.
	sizeof(void *) + sizeof(unsigned int) + sizeof(uint64_t) + 201 * sizeof(char) +
	2 * sizeof(unsigned int) +
	// Original integer from_chars<unsigned long>: first/last/value/base,
	// result/start/sign/val/valid; decimal alnum first/last/val/base/bits/c,
	// alnum_to_val input/result; raise_and_add val/base/c plus overflow temp.
	3 * sizeof(void *) + sizeof(int) + sizeof(std::from_chars_result) + sizeof(void *) +
	2 * sizeof(uint64_t) + sizeof(bool) + 3 * sizeof(void *) + 3 * sizeof(int) +
	sizeof(unsigned char) + sizeof(bool) + 2 * sizeof(unsigned char) + sizeof(void *) +
	sizeof(int) + sizeof(unsigned char) + sizeof(uint64_t) +
	// decimal __bit_width(unsigned) -> __countl_zero(unsigned), value/result
	// and source-defined unsigned digits plus real branch return carriers.
	3 * sizeof(unsigned int) + 6 * sizeof(int);
constexpr size_t pouch_library_frames =
	pouch_allocator_frames + pouch_copy_frames + pouch_relocate_frames + pouch_default_frames +
	pouch_vector_frames + pouch_move_frames + pouch_nontrivial_frames + pouch_string_frames +
	pouch_size_constructor_frames + pouch_copy_constructor_frames + pouch_decimal_frames +
	pouch_append_caller_frames;
struct pouch_read_frame
{
	// read_scores: *result = {}; creates a full scores temporary distinct
	// from prepare_frame::values, alive through the array assignment.
	scores reset_temporary;
	// Generated array and score-entry assignment this/source references;
	// entry fields live inside the array, not a second score allocation.
	const void *array_assignment_this, *array_assignment_source;
	const void *entry_assignment_this, *entry_assignment_source;
	std::array<const player_item_extra_description_snapshot *,
		   CHAOS_MATERIAL_POUCH_LEDGER_MAX_CHUNKS>
		chunks;
	std::array<bool, material_count> seen;
	std::string_view key, remaining;
	std::from_chars_result parsed;
	uint64_t index, generated, collected;
	size_t chunk_count, chunk;
	const char *begin, *end;
};
struct pouch_budget
{
	pouch_reserve_fn reserve;
	void *context;
	size_t outer;
	bool admit(size_t extra) const noexcept
	{
		return extra <= SIZE_MAX - outer && reserve(outer + extra, context);
	}
};
template <typename T> bool pouch_push_request(const std::vector<T> &value, size_t &extra) noexcept
{
	if (value.size() < value.capacity())
		return true;
	// Exact original vector push _M_check_len(1): size+max(size,1).
	size_t capacity = value.size();
	if (!pouch_add(capacity, std::max(value.size(), size_t{ 1 })) ||
	    capacity > value.max_size() || capacity > SIZE_MAX / sizeof(T))
		return false;
	return pouch_add(extra, capacity * sizeof(T));
}
bool pouch_append_request(size_t length, size_t capacity, size_t added, size_t &new_capacity,
			  size_t &request) noexcept
{
	if (!pouch_add(length, added))
		return false;
	new_capacity = capacity;
	if (length <= capacity)
		return true;
	// basic_string::_M_create exact doubling branch, including old buffer
	// which remains in CURRENT until its actual dispose.
	if (capacity > SIZE_MAX / 2)
		return false;
	new_capacity = std::max(length, capacity * 2);
	return new_capacity < SIZE_MAX && pouch_add(request, new_capacity + 1);
}
size_t pouch_digits(uint64_t value) noexcept
{
	size_t count = 1;
	while (value >= 10)
	{
		value /= 10;
		++count;
	}
	return count;
}
bool pouch_record_request(size_t index, const score &value, size_t &extra) noexcept
{
	const size_t index_digits = pouch_digits(index), generated = pouch_digits(value.generated),
		     collected = pouch_digits(value.collected);
	size_t numeric = 0;
	for (const size_t length : { index_digits, generated, collected })
		if (length > 15 && !pouch_add(numeric, length + 1))
			return false;
	size_t length = index_digits, capacity = std::max(size_t{ 15 }, index_digits), peak = 0;
	// Every decimal rhs has fresh capacity=max(15,length). Rvalue+rvalue
	// cannot choose rhs when wanted>lhs.capacity: wanted>rhs.length and a
	// short rhs's capacity15 is no larger than lhs.capacity. Original lhs
	// therefore receives each append in this complete original expression.
	for (const size_t amount : { size_t{ 1 }, generated, size_t{ 1 }, collected, size_t{ 1 } })
	{
		size_t request = capacity > 15 ? capacity + 1 : 0, next = capacity;
		if (!pouch_append_request(length, capacity, amount, next, request) ||
		    !pouch_add(length, amount))
			return false;
		peak = std::max(peak, request);
		capacity = next;
	}
	return pouch_add(extra, numeric) && pouch_add(extra, peak) &&
	       // Three to_string and five operator+ returned string objects can
	       // survive the full expression; moved-from objects retain no heap.
	       pouch_add(extra, 8 * sizeof(std::string));
}
struct pouch_write_frame
{
	std::vector<player_item_extra_description_snapshot> descriptions;
	std::vector<std::string> chunks;
	std::string current;
};
bool pouch_writer_live(const player_item_snapshot &item, const pouch_write_frame &work,
		       const std::string *record, size_t &live) noexcept
{
	if (!pouch_item_heap(item, live) || !pouch_vector_heap(work.descriptions, live) ||
	    !pouch_vector_heap(work.chunks, live) || !pouch_string_heap(work.current, live))
		return false;
	for (const auto &description : work.descriptions)
		if (!pouch_description_heap(description, live))
			return false;
	for (const auto &chunk : work.chunks)
		if (!pouch_string_heap(chunk, live))
			return false;
	return !record || pouch_string_heap(*record, live);
}
bool pouch_write_scores_bounded(const scores &values, player_item_snapshot *item,
				pouch_reserve_fn reserve, void *context, size_t outer)
{
	// Caller outer owns inline candidate but excludes its mutable heap. Every
	// fresh reserve observes actual candidate/current/chunk/description heaps.
	constexpr size_t own =
		sizeof(pouch_write_frame) + sizeof(pouch_budget) + sizeof(std::string) +
		sizeof(player_item_extra_description_snapshot) + sizeof(size_t) +
		// Exact local request scanner arrays and scalar/view/iterator carriers:
		sizeof(std::array<size_t, 3>) + sizeof(std::array<size_t, 5>) +
		2 * sizeof(std::initializer_list<size_t>) + 12 * sizeof(size_t) +
		4 * sizeof(void *) + sizeof(std::string_view) + pouch_library_frames;
	pouch_budget budget{ reserve, context, outer };
	size_t initial = own;
	if (!pouch_item_heap(*item, initial) || !budget.admit(initial))
		return false;
	pouch_write_frame work;
	size_t closure_bytes = 0;
	const auto admit = [&](size_t extra, const std::string *record = nullptr) noexcept
	{
		size_t live = own + closure_bytes;
		return pouch_writer_live(*item, work, record, live) && pouch_add(live, extra) &&
		       budget.admit(live);
	};
	closure_bytes = sizeof(admit);
	for (const auto &description : item->extra_descriptions)
		if (!std::string_view(description.keyword).starts_with(ledger_prefix))
		{
			size_t extra = 0;
			if (!pouch_push_request(work.descriptions, extra) ||
			    !pouch_description_heap(description, extra, true) || !admit(extra))
				return false;
			work.descriptions.push_back(description);
		}
	for (size_t index = 0; index < values.size(); ++index)
	{
		if (!values[index].generated && !values[index].collected)
			continue;
		size_t extra = 0;
		if (!pouch_record_request(index, values[index], extra) || !admit(extra))
			return false;
		const std::string record = std::to_string(index) + ":" +
					   std::to_string(values[index].generated) + ":" +
					   std::to_string(values[index].collected) + ";";
		if (work.current.size() + record.size() > CHAOS_MATERIAL_POUCH_LEDGER_CHUNK_BYTES)
		{
			extra = 0;
			if (!pouch_push_request(work.chunks, extra) || !admit(extra, &record))
				return false;
			work.chunks.push_back(std::move(work.current));
			work.current.clear();
		}
		extra = 0;
		size_t capacity = 0;
		if (!pouch_append_request(work.current.size(), work.current.capacity(),
					  record.size(), capacity, extra) ||
		    !admit(extra, &record))
			return false;
		work.current += record;
	}
	if (!work.current.empty())
	{
		size_t extra = 0;
		if (!pouch_push_request(work.chunks, extra) || !admit(extra))
			return false;
		work.chunks.push_back(std::move(work.current));
	}
	if (work.chunks.size() > CHAOS_MATERIAL_POUCH_LEDGER_MAX_CHUNKS)
		return false;
	for (size_t index = 0; index < work.chunks.size(); ++index)
	{
		const size_t digits = pouch_digits(index);
		size_t keyword = ledger_prefix.size() > 15 ? ledger_prefix.size() + 1 : 0;
		size_t next = std::max(size_t{ 15 }, ledger_prefix.size());
		if (!pouch_append_request(ledger_prefix.size(), next, digits, next, keyword))
			return false;
		size_t extra = keyword;
		// Full keyword expression and aggregate are admitted before the real
		// chunk move; no callback can observe that move as an extra heap owner.
		if (!pouch_add(extra, 2 * sizeof(std::string)) ||
		    !pouch_push_request(work.descriptions, extra) || !admit(extra))
			return false;
		work.descriptions.push_back({ std::string(ledger_prefix) + std::to_string(index),
					      std::move(work.chunks[index]),
					      false,
					      {} });
	}
	if (!admit(0))
		return false;
	item->extra_descriptions = std::move(work.descriptions);
	item->string_mask |= STRUNG_EDESC;
	return true;
}
player_snapshot_codec_result pouch_encode_singleton_bounded(const player_item_snapshot &item,
							    std::vector<uint8_t> *encoded,
							    pouch_reserve_fn reserve, void *context,
							    size_t outer, bool *refused)
{
	size_t own = sizeof(std::initializer_list<player_item_snapshot>) +
		     sizeof(std::vector<player_item_snapshot>) + sizeof(pouch_budget) +
		     2 * sizeof(player_item_snapshot) + pouch_library_frames +
		     pouch_singleton_constructor_frames;
	if (!pouch_add(own, player_item_snapshot_copy_frame_bytes()))
	{
		*refused = true;
		return player_snapshot_codec_result::limit_exceeded;
	}
	pouch_budget budget{ reserve, context, outer };
	size_t copies = 0;
	if (!pouch_item_heap(item, copies, true) || copies > (SIZE_MAX - own) / 2 ||
	    !budget.admit(own + 2 * copies))
	{
		*refused = true;
		return player_snapshot_codec_result::limit_exceeded;
	}
	// Two original genuine deep copies: const initializer-list backing item
	// and its vector element. Both live until after the codec call finishes.
	const std::initializer_list<player_item_snapshot> singleton{ item };
	std::vector<player_item_snapshot> items(singleton);
	size_t live = own;
	if (!pouch_item_heap(*singleton.begin(), live) || !pouch_vector_heap(items, live) ||
	    !pouch_item_heap(items.front(), live) || !pouch_add(live, outer))
	{
		*refused = true;
		return player_snapshot_codec_result::limit_exceeded;
	}
	// own already includes the vector element's inline row, so its actual
	// one-element vector allocation replaces that one reserved row term.
	live -= sizeof(player_item_snapshot);
	struct relay_context
	{
		pouch_reserve_fn reserve;
		void *context;
		bool *refused;
	} relay{ reserve, context, refused };
	const auto relay_fn = +[](size_t bytes, void *opaque) noexcept
	{
		auto &state = *static_cast<relay_context *>(opaque);
		const bool allowed = state.reserve(bytes, state.context);
		if (!allowed)
			*state.refused = true;
		return allowed;
	};
	if (!pouch_add(live, sizeof(relay) + sizeof(relay_fn)))
	{
		*refused = true;
		return player_snapshot_codec_result::limit_exceeded;
	}
	return player_item_snapshot_list_encode_bounded(items, encoded, relay_fn, &relay, live);
}
#endif
} // namespace

chaos_pouch_ledger_result chaos_pouch_ledger_prepare_bounded(
	const player_item_snapshot &before, std::span<const chaos_material_pouch_usage> usage,
	chaos_pouch_usage_mode mode, player_item_snapshot *after,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	using result = chaos_pouch_ledger_result;
	if (!after || !reserve || !before.object_uid || before.vnum != VOBJ_CHAOS_CRAFT_POUCH ||
	    usage.empty() || usage.size() > material_count ||
	    (mode != chaos_pouch_usage_mode::generated &&
	     mode != chaos_pouch_usage_mode::collected))
		return result::invalid;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)context;
	(void)outer;
	return result::capacity;
#else
	struct prepare_frame
	{
		std::vector<uint8_t> encoded;
		scores values;
	};
	constexpr size_t own = sizeof(prepare_frame) + sizeof(pouch_budget) + pouch_library_frames +
			       3 * sizeof(size_t) + sizeof(bool) +
			       sizeof(player_snapshot_codec_result) + 2 * sizeof(void *) +
			       sizeof(std::span<const chaos_material_pouch_usage>) +
			       sizeof(chaos_pouch_usage_mode) + sizeof(pouch_reserve_fn) +
			       sizeof(void *) + sizeof(size_t);
	pouch_budget budget{ reserve, context, outer };
	if (!budget.admit(own))
		return result::capacity;
	try
	{
		prepare_frame work;
		size_t current = outer;
		if (!pouch_add(current, own))
			return result::capacity;
		bool refused = false;
		const auto first = pouch_encode_singleton_bounded(before, &work.encoded, reserve,
								  context, current, &refused);
		if (first != player_snapshot_codec_result::ok)
			return refused || first == player_snapshot_codec_result::allocation_failure ||
					       first == player_snapshot_codec_result::
								unsupported_version ?
				       result::capacity :
				       result::invalid;
		current = own;
		if (!pouch_vector_heap(work.encoded, current) ||
		    !pouch_add(current, sizeof(pouch_read_frame)) || !budget.admit(current))
			return result::capacity;
		if (!read_scores(before, &work.values))
			return result::invalid;
		for (const auto &used : usage)
		{
			size_t index = 0;
			if (!used.count || !material_index(used.vnum, &index))
				return result::invalid;
			uint64_t &count = mode == chaos_pouch_usage_mode::generated ?
						  work.values[index].generated :
						  work.values[index].collected;
			if (count > std::numeric_limits<uint64_t>::max() - used.count)
				return result::overflow;
			count += used.count;
		}
		current = own + sizeof(player_item_snapshot);
		if (!pouch_vector_heap(work.encoded, current) || !budget.admit(current))
			return result::capacity;
		player_item_snapshot candidate;
		current = outer;
		if (!pouch_add(current, own + sizeof(candidate)) ||
		    !pouch_vector_heap(work.encoded, current) ||
		    player_item_snapshot_clone_bounded(before, &candidate, reserve, context,
						       current) != player_snapshot_codec_result::ok)
			return result::capacity;
		// Nested writer observes the mutable candidate heap itself. Caller
		// outer retains the original inputs, prior output, score frame, encoded
		// bytes and candidate inline exactly once, and excludes that heap.
		current = outer;
		if (!pouch_add(current, own + sizeof(candidate)) ||
		    !pouch_vector_heap(work.encoded, current) ||
		    !pouch_write_scores_bounded(work.values, &candidate, reserve, context, current))
			return result::capacity;
		current = outer;
		if (!pouch_add(current, own + sizeof(candidate)) ||
		    !pouch_item_heap(candidate, current) ||
		    !pouch_vector_heap(work.encoded, current))
			return result::capacity;
		refused = false;
		if (pouch_encode_singleton_bounded(candidate, &work.encoded, reserve, context,
						   current,
						   &refused) != player_snapshot_codec_result::ok)
			return result::capacity;
		// Original full successful transfer. No callback or allocating work
		// follows it; prior output stays alive until this genuine move.
		*after = std::move(candidate);
		return result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return result::capacity;
	}
	catch (...)
	{
		return result::invalid;
	}
#endif
}

chaos_pouch_ledger_result chaos_pouch_ledger_verify_bounded(
	const player_item_snapshot &before, const player_item_snapshot &after,
	std::span<const chaos_material_pouch_usage> usage, chaos_pouch_usage_mode mode,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	using result = chaos_pouch_ledger_result;
	if (!reserve)
		return result::invalid;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)before;
	(void)after;
	(void)usage;
	(void)mode;
	(void)context;
	(void)outer;
	return result::capacity;
#else
	struct verify_frame
	{
		player_item_snapshot expected;
		std::vector<uint8_t> expected_bytes, actual_bytes;
	};
	constexpr size_t own = sizeof(verify_frame) + sizeof(pouch_budget) + pouch_library_frames +
			       3 * sizeof(size_t) + sizeof(bool) +
			       sizeof(player_snapshot_codec_result) + 2 * sizeof(void *) +
			       sizeof(std::span<const chaos_material_pouch_usage>) +
			       sizeof(chaos_pouch_usage_mode) + sizeof(pouch_reserve_fn) +
			       sizeof(void *) + sizeof(size_t);
	pouch_budget budget{ reserve, context, outer };
	if (!budget.admit(own))
		return result::capacity;
	try
	{
		verify_frame work;
		size_t current = outer;
		if (!pouch_add(current, own))
			return result::capacity;
		const auto prepared = chaos_pouch_ledger_prepare_bounded(
			before, usage, mode, &work.expected, reserve, context, current);
		if (prepared != result::ok)
			return prepared;
		current = outer;
		if (!pouch_add(current, own) || !pouch_item_heap(work.expected, current))
			return result::capacity;
		bool refused = false;
		const auto encoded_expected = pouch_encode_singleton_bounded(
			work.expected, &work.expected_bytes, reserve, context, current, &refused);
		if (encoded_expected != player_snapshot_codec_result::ok)
			return refused ||
					       encoded_expected == player_snapshot_codec_result::
									   allocation_failure ||
					       encoded_expected == player_snapshot_codec_result::
									   unsupported_version ?
				       result::capacity :
				       result::invalid;
		current = outer;
		if (!pouch_add(current, own) || !pouch_item_heap(work.expected, current) ||
		    !pouch_vector_heap(work.expected_bytes, current))
			return result::capacity;
		refused = false;
		const auto encoded_actual = pouch_encode_singleton_bounded(
			after, &work.actual_bytes, reserve, context, current, &refused);
		if (encoded_actual != player_snapshot_codec_result::ok)
			return refused ||
					       encoded_actual == player_snapshot_codec_result::
									 allocation_failure ||
					       encoded_actual == player_snapshot_codec_result::
									 unsupported_version ?
				       result::capacity :
				       result::invalid;
		return work.expected_bytes == work.actual_bytes ? result::ok : result::invalid;
	}
	catch (const std::bad_alloc &)
	{
		return result::capacity;
	}
	catch (...)
	{
		return result::invalid;
	}
#endif
}

// Genuine controller present before the original first admit callback. The
// later prepare_frame/verify_frame and all Source/library work are separate;
// this is not the mixed original own allowance or any retained heap baseline.
bool chaos_pouch_ledger_prepare_initial_inline_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	if (!output)
		return false;
	*output = sizeof(pouch_budget);
	return true;
#else
	(void)output;
	return false;
#endif
}
bool chaos_pouch_ledger_verify_initial_inline_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	if (!output)
		return false;
	*output = sizeof(pouch_budget);
	return true;
#else
	(void)output;
	return false;
#endif
}

// Private typed decimal leaf inventory. This does not select a new ledger
// path or export a complete prepare/verify Source contract. The old decimal
// allowance and every original request are preserved. Capture the matching
// numeric_traits control and independently match its old credits before use.
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
constexpr size_t pouch_decimal_bit_width_own_source =
	// std::__bit_width<unsigned>: argument, actual __digits-typed _Nd,
	// and its declared int return (not an unsigned result).
	sizeof(unsigned int) + sizeof(decltype(__gnu_cxx::__int_traits<unsigned int>::__digits)) +
	sizeof(int);
constexpr size_t pouch_decimal_countl_zero_own_source =
	// unsigned argument; _Nd, _Nd_ull, _Nd_ul and _Nd_u real constants;
	// selected unsigned branch's __diff; actual int return. Discarded
	// wider-type branch variables and builtin implementation are excluded.
	sizeof(unsigned int) + sizeof(decltype(__gnu_cxx::__int_traits<unsigned int>::__digits)) +
	sizeof(decltype(__gnu_cxx::__int_traits<unsigned long long>::__digits)) +
	sizeof(decltype(__gnu_cxx::__int_traits<unsigned long>::__digits)) +
	sizeof(decltype(__gnu_cxx::__int_traits<unsigned int>::__digits)) + 2 * sizeof(int);
constexpr size_t pouch_decimal_bit_source =
	pouch_decimal_bit_width_own_source + pouch_decimal_countl_zero_own_source;
constexpr size_t pouch_decimal_raise_and_add_own_source =
	// uint64_t reference, base, digit and genuine bool return. The two
	// overflow builtins do not declare an unsigned overflow temporary.
	sizeof(uint64_t *) + sizeof(int) + sizeof(unsigned char) + sizeof(bool);
#endif
}

namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
// PRIVATE WORK: real ledger-local helper families, lower projections OPEN.
// P/N/B/C explicitly refer to actual pointer/ref, size_t, bool and char carriers.
constexpr size_t pouch_full_checked_add_source = sizeof(size_t *) + sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_full_admit_source = sizeof(pouch_budget *) + sizeof(size_t) + sizeof(bool);
template <class T> constexpr size_t pouch_full_vector_heap_source =
	// value/bytes refs and bool; actual capacity this/result. Checked-add
	// owns its separately named family. T=ED,string,byte,int32 are distinct.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
template <class T> constexpr size_t pouch_full_vector_copy_source =
	// value/bytes refs and bool; actual size this/result; no allocator graph.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_full_string_heap_source =
	// value/bytes refs + bool. Actual const capacity branch/helper projection
	// is separately matched to selected common string/current scopes.
	2 * sizeof(void *) + sizeof(bool);
constexpr size_t pouch_full_string_copy_source = 2 * sizeof(void *) + sizeof(bool);
constexpr size_t pouch_full_description_heap_source = 2 * sizeof(void *) + 2 * sizeof(bool);
constexpr size_t pouch_full_item_heap_source =
	// item/bytes refs, copied, local actual, bool; named CURRENT/fresh child
	// Source and actual getters are independent of this wrapper signature.
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool);
template <class T> constexpr size_t pouch_full_push_request_source =
	// value/extra refs, actual capacity local, bool; size_t{1} bound const-ref
	// temporary is real. Vector size/capacity/max_size and max/ref-result
	// families are separately projected, never inferred from pooled bytes.
	2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_full_append_request_source =
	// three scalar args, new_capacity/request refs and bool; capacity*2
	// bound to max's const-ref is a distinct physical size_t temporary.
	2 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_full_digits_source = sizeof(uint64_t) + 2 * sizeof(size_t);
constexpr size_t pouch_full_writer_live_source =
	// item/work/record/live refs, description and chunk aliases, plus each
	// actual range's hidden range/begin/end. Bool result is own.
	12 * sizeof(void *) + sizeof(bool);
constexpr size_t pouch_full_writer_admit_operator_source =
	// lambda receiver, extra and record formals, local live, bool result.
	2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_full_singleton_relay_operator_source =
	// actual lambda receiver, opaque and state ref, bytes argument,
	// allowed local and bool result. Generated thunk is a separate family.
	3 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool);
constexpr size_t pouch_full_singleton_relay_thunk_source =
	// Captureless function-pointer thunk: opaque/bytes formals, bool return
	// and actual empty closure value on which operator() is invoked.
	sizeof(void *) + sizeof(size_t) + sizeof(bool) + sizeof(char);
constexpr size_t pouch_full_singleton_relay_conversion_source =
	// Captureless lambda conversion: receiver and returned function pointer.
	2 * sizeof(void *);

// Missing-vs-old-phase credit mapping, actual scalar scanner arrays/loop
// temporaries, generated callbacks and full public body scopes remain OPEN.
// Do not export or label this finite partial work as a complete provider.

// PRIVATE WORK: finite reader mapping only, not a complete ledger provider.
// Read-only typed handoff from critical_codec_source_reviewer, 2026-10-10.
constexpr size_t pouch_full_reader_missing_source =
	// read_number's separate parsed result; read_scores' second lexical index;
	// actual {generated,collected} score temporary; unmatched formals/range/
	// real caller objects. No implicit trivial score/array assignment
	// body is charged. Old assignment this/source4P remains in the original
	// child allowance but is UNCLAIMED here; padding is never new credit.
	sizeof(std::from_chars_result) + sizeof(uint64_t) + sizeof(score) + 8 * sizeof(void *) +
	sizeof(char) + 2 * sizeof(bool);
constexpr size_t pouch_full_reader_iterator_source =
	// Const extra-description selected begin/end/declared pointer ctor/
	// == (C++20 rewritten !=)/base/deref/increment. No inactive != or3way.
	12 * sizeof(void *) + sizeof(bool) +
	// Spell-ID empty() with its actual two returned const iterators; no
	// deref/increment. Distinct int32 typed family from extra-descriptions.
	11 * sizeof(void *) + 2 * sizeof(bool);
constexpr size_t pouch_full_reader_array_source =
	// Three nonconst chunks/seen/scores subscripts, direct2P+N each.
	// chunks.size and scores.size eachP+N; no begin/data/array relation.
	8 * sizeof(void *) + 5 * sizeof(size_t);
constexpr size_t pouch_full_shared_string_view_source =
	// String→SV, genuine data/_M_data/size; pointer/count/copy SV ctors,
	// size/empty/front/data/remove_prefix; starts_with→substr/min/check→
	// ==→compare→_S_compare→char_traits compare; bare builtin memcmp
	// has no separately declared Source callee parameters or result.
	// Four SV actual by-value formal owners; return aggregates occupy real
	// caller destinations. One const query bool, not an extra intrinsic bool.
	// No string length/tcc search or nontrivial destructor is selected.
	26 * sizeof(void *) + 15 * sizeof(size_t) + sizeof(std::ptrdiff_t) + 4 * sizeof(int) +
	4 * sizeof(bool) + 4 * sizeof(std::string_view);
constexpr size_t pouch_full_reader_decimal_missing_source =
	// Actual from_chars sign is int, not spare old uint64. countl_zero's
	// selected __diff and int return + raise_and_add bool return are new.
	// Old alleged overflow uint64 temp and excess unsigned remain unused.
	3 * sizeof(int) + sizeof(bool);

// Reader caller complete real old matched fields, only for the full inventory.
// score==/array== are native-apply/counters paths and are never imported here.
constexpr size_t pouch_full_reader_caller_source =
	sizeof(scores) +
	sizeof(std::array<const player_item_extra_description_snapshot *,
			  CHAOS_MATERIAL_POUCH_LEDGER_MAX_CHUNKS>) +
	sizeof(std::array<bool, material_count>) + 2 * sizeof(std::string_view) +
	2 * sizeof(std::from_chars_result) + 4 * sizeof(uint64_t) + 2 * sizeof(size_t) +
	sizeof(score) + 10 * sizeof(void *) + sizeof(char) + 2 * sizeof(bool);

// Complete helper Source is not silently substituted for unrelated old writer
// string mutations, decoder totals or hypothetical array equality.

// PRIVATE exact selected row projection; strings/vectors/credit validation OPEN.
// All scalar members remain inside their actual row. No new physical row and
// no public-list move-constructor/decoder/lifetime total is borrowed here.
constexpr size_t pouch_full_row_array_copy_constructor_source =
	// Actual five array<T,N> types: values<int32,8>, timers<int64,6>,
	// bitvectors<uint64,5>, affects<array<int16,2>,4>, inner<int16,2>.
	// These trivial aggregate array types declare no copy constructor
	// body. Their inline bytes remain in each real row; no new callee scope.
	0;
constexpr size_t pouch_full_row_move_assignment_own_source =
	// Genuine nontrivial row receiver/source/returned-reference only.
	// Five trivial member-array assignments declare no new callee bodies.
	3 * sizeof(void *);
constexpr size_t pouch_full_row_move_assignment_final_tail_source =
	// Parent final transfer is after the clone child has returned. Child
	// clone copy/move allowance is not retained: the genuine nontrivial
	// row receiver/source/returned-reference is owned here, without credit
	// from that ended preflight. std::move is a distinct caller scope.
	pouch_full_row_move_assignment_own_source + 2 * sizeof(void *);
constexpr size_t pouch_full_row_generated_cleanup_source =
	// Actual nontrivial row destructor receiver. The five trivial std::array
	// member types have no declared destructor body and contribute no
	// selected runtime destructor call scopes. Nontrivial row assignment3P
	// is distinct and preserved; member arrays remain actual inline storage.
	sizeof(void *);

// Actual row operator= dispatches four string move assignments and equal-
// allocator move assignments of dynamic-affect and ED vectors. Source object's
// old string heaps can be left in candidate until its real cleanup finishes;
// caller prior-output outer remains counted throughout this original tail.
// Standard equal string allocator makes unequal/deep-copy assign branches
// unreachable. Persistent pouch library credits are mapped only to their actual selected
// methods. The clone child's four string/two vector move groups are not
// persistent final-tail authority and cannot be credited here.

// No callable constructors/destructors are invented for aggregate zero-init.
// Precise generated lifetime convention and typed string/vector descendants
// require final finite review before this partial projection is composed.

// PRIVATE actual generated work-frame lifetime projection; finite review OPEN.
constexpr size_t pouch_full_prepare_frame_lifetime_own_source =
	// Local prepare_frame contains byte vector and scores. Its implicit
	// nontrivial default/cleanup receivers are genuine separate scopes.
	2 * sizeof(void *);
constexpr size_t pouch_full_verify_frame_lifetime_own_source =
	// Local verify_frame contains expected row plus two byte vectors.
	// Generated frame default/cleanup receivers only; member graphs shared.
	2 * sizeof(void *);
constexpr size_t pouch_full_writer_frame_lifetime_own_source =
	// Local writer frame: two distinct typed vectors plus string.
	// Generated frame default/cleanup receivers only.
	2 * sizeof(void *);
constexpr size_t pouch_full_row_default_own_source =
	// prepare candidate and verify expected invoke the same generated
	// nontrivial row default constructor. Its actual candidate/expected rows
	// are default-initialized; their aggregate fixed/scalar members add no
	// new declared trivial constructor bodies to this receiver projection.
	sizeof(void *);
constexpr size_t pouch_full_scores_default_source =
	// prepare_frame::values is DEFAULT-initialized, not the reader's {} reset.
	// Its implicit array<score> default constructor reaches score's default
	// member initializers through the implicit score default constructor.
	// Its nontrivial DMI default initialization is retained; reset {}
	// and implicit trivial score/array assignment add no new callee body.
	2 * sizeof(void *);
// Default vector<byte> has actual six receivers and cleanup22P+5N+2B;
// keep these exact typed helper families separate from DA/ED/int32 defaults.
// All frame member inline bytes are admitted by original own; Source helper
// definitions here do not make a second work/row/array/byte-vector object.
// The outer initial pouch_budget is handled by actual initial-inline getter,
// not reintroduced as a second physical initial object in this helper layer.

// PRIVATE selected span/iterator ledger branch projection, finite review OPEN.
// Parameter inline span object is genuinely named in original public own.
// Its helper scopes are not replaced by that object's sizeof.
constexpr size_t pouch_full_usage_span_method_source =
	// empty(this,bool), size(this,N), dynamic extent(this,N).
	3 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) +
	// span const begin/end receivers2P; returned iterators occupy the
	// caller hidden range destinations. end pointer-sum bound-reference
	// temporaryP is genuine; begin binds the pointer member itself.
	3 * sizeof(void *) +
	// normal_iterator<const usage*,span> ctor(this,source pointer-ref),
	// C++20 ==(left,right,bool), base(this,returnedpointer),
	// prefix++(this,returnedref), dereference(this,returnedref).
	2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) +
	2 * sizeof(void *) + 2 * sizeof(void *);
constexpr size_t pouch_full_usage_span_parameter_lifetime_source =
	// Defaulted span/dynamic-extent copy constructors receiver/source2P
	// each. span has an explicitly defaulted destructor at captured span244
	// (receiverP); dynamic extent has no declared trivial destructor body.
	// No pointer/count/to_address ctor is selected.
	5 * sizeof(void *);

// Prepare uses empty/size/const begin/end and range iteration. Verify only
// owns its additional real by-value span lifetime; nested prepare owns methods.
// No arbitrary vector or unrelated decoder Source profile is used as credit.

// PRIVATE UNACCEPTED selected string tail projection, GNU13/CXX11 ABI.
// Same equal standard allocator at final row assignment; Source helper-family
// credit and complete writer graph remain OPEN. No exported complete claim.
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
constexpr size_t pouch_full_string_local_mutable_source = 4 * (2 * sizeof(void *));
constexpr size_t pouch_full_string_tail_query_source =
	// _M_is_local this/return, _M_data getter this/pointer-return, const local;
	// size and length are separate actual public functions.
	sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + pouch_full_string_local_const_source +
	2 * (sizeof(void *) + sizeof(size_t));
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

// Four row strings reach the same actual selected std::string methods; own
// receiver/source call carriers are real. No row string move constructor is
// substituted for this operator. Old string mutation/disjunct pools can credit
// only exactly named shared size/data/set_length/copy/disposal definitions.
// String cleanup still owns destructor -> dispose/is_local -> destroy ->
// allocator traits deallocate plus allocator-hider/member destructor chain.
// Credit/family multiplicity requires finite review before full composition.

// Actual selected equal-allocator assignment + disposal union. These are
// source helper family scopes; six actual row/ED strings do not make six
// different std::string type specializations. Physical strings remain inline.
constexpr size_t pouch_full_string_tail_cleanup_source =
	// Destructor/dispose/islocal/destroy, allocator-hider and allocator
	// destruction, get_allocator/data/local chain and selected char
	// traits/allocator/newallocator/sized-delete. No newallocator C++20 dtor.
	25 * sizeof(void *) + 5 * sizeof(size_t) + 2 * sizeof(bool);
constexpr size_t pouch_full_string_tail_raw_operator_source =
	pouch_full_string_tail_operator_source + pouch_full_string_tail_query_source +
	pouch_full_string_tail_use_local_source + pouch_full_string_set_length_source +
	pouch_full_string_runtime_copy_source;
// The raw groups (local pointer-chain constants are named lower descendants)
// physically sum58P+10N+2C+7B including conservative init. This
// sum repeats _M_length(P+N) and _M_data getter2P; only the union below is used.
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
constexpr size_t pouch_full_string_tail_missing_source =
	// Genuine unique persistent old credit ONLY20P+9N+C+B:
	// nontrivial dtor/dispose/islocal/destroy4P+N+B;
	// _S_copy/traits-copy5P+2N plus traits-assign2P; old conservative
	// memcpy3P+N allowance is preserved in the child but UNCLAIMED here;
	// traits/allocator/newallocator/sizeddelete7P+4N (no vector_deallocate);
	// capacity setter and set_length2P+2N, actual NUL charC.
	// Ambiguous final data2P and six(P+N) query pool stay UNCLAIMED.
	pouch_full_string_tail_union_source -
	(20 * sizeof(void *) + 9 * sizeof(size_t) + sizeof(char) + sizeof(bool));

// Full runtime group union67P13N+C8B includes actual destructor/dispose/
// destroy, allocator-hider and C++20 allocator cleanup, char deallocation;
// pointer local chain and helper getter overlaps are counted once. It excludes
// the C++20-inactive new_allocator destructor, unmatched loop-local scopes in
// other templates, and all alternate unequal allocator/assign/disjunct paths.
// This closes only the selected final string-tail projection; the ledger full
// provider and writer/clone string constructor compositions remain OPEN.

static_assert(pouch_full_string_tail_runtime_union_source ==
	      67 * sizeof(void *) + 13 * sizeof(size_t) + sizeof(char) + 8 * sizeof(bool));
static_assert(pouch_full_string_tail_missing_source ==
	      47 * sizeof(void *) + 5 * sizeof(size_t) + sizeof(char) + 7 * sizeof(bool));

// PRIVATE complete selected decimal helper inventory, actual default base10.
constexpr size_t pouch_full_reader_from_chars_own_source =
	// first/last/value refs3P; base int; actual __resF,__sign int,
	// __val uint64,__start pointer,__valid bool. Returned aggregate occupies
	// caller parsed destination: no third aggregate invented for NRVO.
	4 * sizeof(void *) + 2 * sizeof(int) + sizeof(std::from_chars_result) + sizeof(uint64_t) +
	sizeof(bool);
constexpr size_t pouch_full_reader_alnum_own_source =
	// first ref,last ptr,val ref; base/bits_per_digit/unused_lower int3;
	// actual byte __c and bool return. Both calls share selected DecOnly=true.
	3 * sizeof(void *) + 3 * sizeof(int) + sizeof(unsigned char) + sizeof(bool);
constexpr size_t pouch_full_reader_alnum_to_val_source = 2 * sizeof(unsigned char);
constexpr size_t pouch_full_reader_decimal_source =
	pouch_full_reader_from_chars_own_source + pouch_full_reader_alnum_own_source +
	pouch_full_reader_alnum_to_val_source + pouch_decimal_bit_source +
	pouch_decimal_raise_and_add_own_source;
static_assert(pouch_full_reader_decimal_source ==
	      8 * sizeof(void *) + 14 * sizeof(int) + sizeof(std::from_chars_result) +
		      sizeof(uint64_t) + 2 * sizeof(unsigned int) + 4 * sizeof(unsigned char) +
		      3 * sizeof(bool));
// Actual literal/default base10 reaches only alnum<true>, no pow2/other-base
// parser, signed overflow temporary, wchar/std::isdigit/locale/table branch.
// Genuine old matched fields leave only3int+B residual already encoded in
// reader-source-work.inc. Its old spare2uint64+unsigned slots stay unused.
// The caller's read_number/read_scores parsedF and charconv __resF are distinct
// declared objects; return carrier is not a speculative extra aggregate.

// PRIVATE finite clone reviewer projection; complete ledger selection OPEN.
// GNU13 C++20 source families, captured exact controls; no emitted stack claim.
// No child high-water lends credit to a later final prepare transfer.
constexpr size_t pouch_full_clone_entry_missing_source =
	// outer/own/peak/request are four size_t vs three old own fields.
	// const bad_alloc catch-reference is an actual additional pointer.
	sizeof(size_t) + sizeof(void *);
constexpr size_t pouch_full_clone_generated_row_missing_source =
	// Old genuine row copy/move this+source4P only. Nontrivial row
	// returned refP and caller std::move argument/result2P are unmatched.
	// Trivial member arrays have inline storage, no declared callee bodies.
	3 * sizeof(void *);

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
constexpr size_t pouch_full_copy_vector_int32_source =
	pouch_full_copy_vector_frontend_source<int32_t> +
	pouch_full_copy_vector_allocation_source<int32_t> +
	pouch_full_copy_vector_iterator_entry_source +
	pouch_full_copy_vector_trivial_backend_source;
constexpr size_t pouch_full_copy_vector_dynamic_source =
	pouch_full_copy_vector_frontend_source<player_item_dynamic_affect_snapshot> +
	pouch_full_copy_vector_allocation_source<player_item_dynamic_affect_snapshot> +
	pouch_full_copy_vector_iterator_entry_source +
	pouch_full_copy_vector_trivial_backend_source;
// __assign_one and actual dynamic-affect object storage are retained.
// Scalar-only dynamic-affect declares no assignment callee body.
constexpr size_t pouch_full_copy_vector_ed_source =
	pouch_full_copy_vector_frontend_source<player_item_extra_description_snapshot> +
	pouch_full_copy_vector_allocation_source<player_item_extra_description_snapshot> +
	pouch_full_copy_vector_iterator_entry_source + pouch_full_copy_vector_ed_backend_source;
// Original conservative memmove allowance remains unchanged in old
// children. It is not claimed as a declared intrinsic match in these unused
// partial missing projections; their other real unmatched scopes stay fixed.
// Generated ED copy constructor2P and its two string/vector-i32 member-copy
// graphs are separately composed; no speculative member inline object added.
constexpr size_t pouch_full_copy_vector_int32_missing_source =
	2 * sizeof(void *) + 3 * sizeof(bool);
constexpr size_t pouch_full_copy_vector_dynamic_missing_source =
	2 * sizeof(void *) + 3 * sizeof(bool);
constexpr size_t pouch_full_copy_vector_ed_missing_source = 4 * sizeof(void *) + 3 * sizeof(bool);

constexpr size_t pouch_full_string_copy_increment_source =
	// Additions over the authentic selected string-tail union ONLY.
	// string(copy) instantiates _M_construct<char*> and mutable-char
	// _S_copy_chars; it cannot share const-char iterator specializations.
	// One materialized selected allocator object, not old hypothetical3A.
	45 * sizeof(void *) + 10 * sizeof(size_t) + 2 * sizeof(std::ptrdiff_t) +
	sizeof(std::forward_iterator_tag) + sizeof(std::random_access_iterator_tag) +
	sizeof(std::allocator<char>) + sizeof(bool);
constexpr size_t pouch_full_string_copy_increment_matched_source =
	// Constructor20P+N+2D+tags+A after exact local-data overlap with tail.
	// Creation/allocation13P+7N; ambiguous string-max query pool unclaimed.
	33 * sizeof(void *) + 8 * sizeof(size_t) + 2 * sizeof(std::ptrdiff_t) +
	sizeof(std::forward_iterator_tag) + sizeof(std::random_access_iterator_tag) +
	sizeof(std::allocator<char>);
constexpr size_t pouch_full_string_copy_increment_missing_source =
	pouch_full_string_copy_increment_source - pouch_full_string_copy_increment_matched_source;
constexpr size_t pouch_full_string_copy_tail_union_source =
	pouch_full_string_tail_union_source + pouch_full_string_copy_increment_source;
constexpr size_t pouch_full_string_copy_tail_missing_source =
	pouch_full_string_tail_missing_source + pouch_full_string_copy_increment_missing_source;
static_assert(pouch_full_string_copy_increment_missing_source ==
	      12 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool));
static_assert(pouch_full_string_copy_tail_missing_source ==
	      59 * sizeof(void *) + 7 * sizeof(size_t) + sizeof(char) + 8 * sizeof(bool));

// Global phase partition remains explicit: above missing-vector projections
// apply against each corresponding original clone group once. Copy/final
// controller shared frontend11P per DA/ED and int32 cleanup getter2P must
// be deducted only from full unions; do not subtract old slots twice.
// Clone nontrivial row assignment3P plus caller std::move2P is distinct
// from the final prepare transfer; trivial arrays add no callee scopes,
// but Source family composition may retain shared definition once where law
// genuinely covers both; no earlier child admission is persistent authority.

// PRIVATE genuine vector<row>(initializer_list) selected Source, review OPEN.
constexpr size_t pouch_full_singleton_vector_constructor_source =
	// vector receiver, allocator ref, actual by-value initializer_list;
	// actual public default-arg allocator object and default receivers2P.
	2 * sizeof(void *) + sizeof(std::initializer_list<player_item_snapshot>) +
	sizeof(std::allocator<player_item_snapshot>) + 2 * sizeof(void *) +
	// Base(a)/Impl(a)/allocator/newallocator const-copy each2P; data defaultP.
	9 * sizeof(void *) +
	// Actual LI pointer/count constructor and begin/end/size families7P2N;
	// actual same-type by-value LI copy ctor this/source2P.
	7 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(void *) +
	// range_initialize(this,first,last), named n, actual forward param tag,
	// converted random-access argument object. No fill/size constructor.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(std::forward_iterator_tag) +
	sizeof(std::random_access_iterator_tag) +
	// S_check_init_len allocator ref,n,returnedN; actual copied allocator
	// temporary at S_max_size call (const-copy family already Base/Impl).
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(std::allocator<player_item_snapshot>) +
	// Raw const-row distance/__distance/category families; returned category
	// tag occupies actual distance tag parameter, no third returned tag.
	5 * sizeof(void *) + 2 * sizeof(std::ptrdiff_t) + sizeof(std::random_access_iterator_tag) +
	// Genuine mutable get_Tp_allocator receiver/ref result, shared calls.
	2 * sizeof(void *);
constexpr size_t pouch_full_singleton_raw_row_copy_backend_source =
	// uninitialized_copy_a5P+B, uninitialized_copy4P+2B, uninit_false4P,
	// do_uninit_copy5P, Construct2P+B, addressof2P,forward2P,placement2P+N.
	// All actual range iterators are RAW const-row*/row*, no iterator helpers.
	26 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(bool);
// Generated row COPY receiver/source2P plus actual five array constructors,
// string/DA/ED/member COPY and cleanup graphs are shared selected row family.
// Typed row-vector allocation11P6N+B and internal S_max descendant
// union3P5N share newallocMax(P+N). Public vector::max_size is NOT called.
// Typed row-vector cleanup27P5N2B includes the selected row destructor receiver;
// shared row-member destructor leaves must not be multiplied or replaced with
// an opaque player-list lifetime profile. Root tests native/heap assumptions.
// Scalar/public/callback Source and original two genuine backing/element row
// deep copies remain independent. Old singleton own prices both row inline;
// one actual row-vector allocation replaces EXACTLY its one reserved row term.
// Old phase-specific row-copy allowance may credit matching selected scopes
// only while that singleton is active; it cannot lend later prepare tail.

constexpr size_t pouch_full_singleton_internal_max_source =
	// S_max/traits max/newallocator max actual receivers3P and return3N
	// plus genuine diffmax/allocmax locals2N. No public vector max scope.
	3 * sizeof(void *) + 5 * sizeof(size_t);
constexpr size_t pouch_full_singleton_front_source =
	// items.front() actual nonconst path: front2P,begin receiverP,pointer
	// normal-iterator ctor2P,deref2P and actual begin iterator destinationP.
	// No const conversion/generated copy/move/other iterator operations.
	8 * sizeof(void *);
constexpr size_t pouch_full_singleton_vector_frontend_missing_source =
	// Full frontend33P5N2D+F+2R+2A+LI. Exact original constructor
	// frontend unique22P5N+F+R+2A+LI; raw-row distance subsection4P2D+R
	// is explicitly assigned to this phase, giving total26P5N2D+F+2R+2A+LI.
	// Discard repeated old LIbegin/allocator-copy/getter allowances, no pool.
	7 * sizeof(void *);

// PRIVATE actual original entry/local carrier map; global provider OPEN.
// No struct padding, unused size_t pool or prior child high-water credit.
constexpr size_t pouch_full_prepare_caller_missing_source =
	// Usage hidden range/begin/end/used4P, count referenceP, bad_alloc catchP;
	// actual return ledger-result and two compared codec-result destinations.
	6 * sizeof(void *) + sizeof(chaos_pouch_ledger_result) +
	2 * sizeof(player_snapshot_codec_result);
constexpr size_t pouch_full_verify_caller_missing_source =
	// Catch refP, nested prepared ledger-result, actual own return result,
	// second encoded codec-result. Old spareN never credits an enum.
	sizeof(void *) + 2 * sizeof(chaos_pouch_ledger_result) +
	sizeof(player_snapshot_codec_result);
constexpr size_t pouch_full_singleton_caller_missing_source =
	// item/encoded/reserve/context/refused5P, outerN; own/copies/live3N.
	// All are original named declarations, absent the old singleton own.
	// Relay struct3P + function pointerP are explicitly admitted separately
	// before encode and are not charged a second time as residual objects.
	5 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(player_snapshot_codec_result);
constexpr size_t pouch_full_material_index_source =
	// Genuine vnum int/index pointer formal and bool return, no native state.
	sizeof(int) + sizeof(size_t *) + sizeof(bool);
constexpr size_t pouch_full_usage_numeric_limit_source =
	// Actual numeric_limits<uint64_t>::max returned value at overflow check.
	sizeof(uint64_t);
// Preparation and verification frame inline bytes are admitted already by
// their unchanged own sizes; frame construction/cleanup call families and
// member helper descendants must be named separately in complete composition.
// Before/after/input/output/usage/prior heaps/callback descendants are outer-
// owned exactly once. None of these formulas reintroduces those foreign heaps.

// PRIVATE UNACCEPTED typed finite projection: equal standard allocator only.
// Each T specialization below describes the actually selected final controller.
// Vector element cleanup/deallocation, generated allocator/constant lifetimes
// and persistent old-credit projection remain separately OPEN. This is not a
// complete ledger export and is not admitted or selected anywhere.
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

// Dynamic-affect and ED instantiate distinct controllers at final row move.
// ED is also the writer's final descriptions transfer, same selected T family.
// There is no per-row move construction in either equal-allocator assignment.
// A helper may be shared only inside a real retained parent Source allowance;
// clone/singleton high-water child admissions are not persistent tail credit.

// PRIVATE UNACCEPTED selected vector cleanup; no source export/selection.
// Captured GNU13 allocator.h203/stl_vector367,733/alloc_traits945/
// stl_construct149,169,182 and new_allocator100,104 actual branches.
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

// Dynamic-affect and int32 use aux<true>; ED, item and string use aux<false>.
// ED's two strings and spell vector are distinct real nested member cleanup,
// not per-row heap charged twice. Parent prior output heaps remain outer-owned
// until actual controller tmp destruction finishes. Shared selected get_Tp
// scopes need one exact retained credit; no whole allocator pool subtraction.
// Constant query/family multiplicity and generated destructor credit still
// require finite review; this is not a complete lifetime provider.

// PRIVATE finite reviewer handoff, not complete ledger acceptance.
// Scope: persistent PREPARE pouch_library_frames after clone/writer return.
// No ended child high-water is credited. Exactly ONE old move lane is ED;
// dynamic-affect's distinct full controller has no second old move lane.
constexpr size_t pouch_full_final_vector_tail_source =
	// Two controllers share ONLY their exact typed mutable getter with
	// each corresponding cleanup: deduct 2P per T from full sums.
	pouch_full_vector_tail_controller_source<player_item_dynamic_affect_snapshot> +
	pouch_full_vector_cleanup_source<player_item_dynamic_affect_snapshot> - 2 * sizeof(void *) +
	pouch_full_vector_tail_controller_source<player_item_extra_description_snapshot> +
	pouch_full_vector_cleanup_source<player_item_extra_description_snapshot> -
	2 * sizeof(void *) +
	// ED entry owns actual nested int32 spell-vector cleanup. No spell move
	// controller is called at the final equal-allocator row assignment.
	pouch_full_vector_cleanup_source<int32_t>;
constexpr size_t pouch_full_final_vector_tail_missing_source =
	// Full dynamic/ED/spell tail147P+15N+8B+2*(tag+V+A).
	// Genuine persistent matched fields ONLY:
	// one ED controller14P+B+tag+V+A; one ED temp vector destructorP;
	// dynamic/int32 each exact trivial Destroy7P+deallocate9P+5N;
	// ED only allocator/range Destroy5P+deallocate9P+5N: its
	// aux<false>2P is distinct from allocator subtotal's aux<true>.
	// ED nontrivial aux<false>2P/addressof2P/DestroyP/destroy_atP/
	// receiverP=7P come from the named old nontrivial subsection.
	// Combined selected cleanup credit still53P+15N (32+14+7).
	// No allocation/max_size/construct/default/unused margin is subtracted.
	79 * sizeof(void *) + 7 * sizeof(bool) + sizeof(std::true_type) +
	sizeof(std::vector<player_item_dynamic_affect_snapshot>) +
	sizeof(std::allocator<player_item_dynamic_affect_snapshot>);

// Generated nontrivial row destructor receiverP is matched to its real old
// nontrivial row counterpart separately. Its five trivial arrays add no
// selected destructor scopes. Nontrivial row assignment3P+caller move2P
// remain independent; ED nested string cleanup is NOT covered by this leaf.
// Global ledger partition must reserve the above old slots exactly once.

// PRIVATE WORK, NOT A COMPLETE/EXPORTED/ACCEPTED PROVIDER.
// Actual selected unsigned-long conversion on the supported LP64 target.
// Original to_string uses one __str/return object under NRVO; actual caller
// expression string objects remain the old pouch_record_request inline credit.
constexpr size_t pouch_full_writer_to_string_source = sizeof(unsigned long) + sizeof(std::string);
constexpr size_t pouch_full_writer_decimal_length_source =
	// __to_chars_len: value, int base, unsigned n/b2/b3, ulong b4,
	// genuine unsigned result. The old4U+UL maps these exact fields,
	// while its uint64 value is a separate genuine old argument credit.
	sizeof(uint64_t) + sizeof(int) + 4 * sizeof(unsigned int) + sizeof(unsigned long);
constexpr size_t pouch_full_writer_decimal_digits_source =
	// __to_chars_10_impl: first,len,val,digits201,pos; both lexical
	// const auto __num are uint64_t (__val-derived), NOT unsigned int.
	sizeof(char *) + sizeof(unsigned int) + sizeof(uint64_t) + 201 * sizeof(char) +
	sizeof(unsigned int) + 2 * sizeof(uint64_t);
constexpr size_t pouch_full_writer_decimal_missing_source =
	// Old decimal allowance has no selected length-helper int base.
	// Old twoU after digits only credit actual posU once. Its otherU
	// has no matching uint64 num and is not lent to either lexical num.
	sizeof(int) + 2 * sizeof(uint64_t);

// Fill-string constructor closure and selected scalar array/view/lifetime
// descendants remain OPEN. Do not compose this file as a full provider.

// PRIVATE typed helper-family projection; finite independent review OPEN.
// Actual unchanged expected_bytes == actual_bytes, GNU13/C++20.
constexpr size_t pouch_full_verify_byte_equality_source =
	// vector== left/right refs/result; same vector<byte>::size family once.
	2 * sizeof(void *) + sizeof(bool) + sizeof(void *) + sizeof(size_t) +
	// const begin/end receivers2P and declared pointer constructor2P.
	// Stored pointer lvalues bind directly, without conversion temporaries.
	// Returned iterator expressions occupy equal's actual parameter owners.
	2 * sizeof(void *) + 2 * sizeof(void *) +
	// equal/equal_aux/equal_aux1/equal<true> each three real formals+B;
	// equal_aux1 simpleB and equal<true> actual lenN are named locals.
	4 * (3 * sizeof(void *) + sizeof(bool)) + sizeof(bool) + sizeof(size_t) +
	// Three same typed niter_base calls select ONE helper2P. Actual base2P
	// is retained; no undeclared implicit trivial COPY/MOVE callee.
	// Actual named parameter/local/result iterator objects are unchanged.
	2 * sizeof(void *) + 2 * sizeof(void *) +
	// Declared __memcmp owns its2P+N+int and constant queryB.
	// Bare builtin memcmp adds no declared Source callee fields.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(int) + sizeof(bool);
static_assert(pouch_full_verify_byte_equality_source ==
	      25 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(int) + 7 * sizeof(bool));
// Exactly three iterator expressions left.begin/left.end/right.begin are
// selected; no right.end or three-way operator. Physical vector/heap lifetime
// is original verify own/CURRENT. Shared family union is distinct from number
// of calls: no repeated size/constructor/niter family allowance is retained.
// Interface/full Source/supplement selection remains unaccepted until review.

// PRIVATE selected mutable ED/string push iterator graph, review OPEN.
template <class T> constexpr size_t pouch_full_writer_mutable_iterator_source =
	// begin/end receivers; iterator(pointer const&) constructor this/source.
	// Mutable T* member binding creates no const-pointer conversion temp.
	2 * sizeof(void *) + 2 * sizeof(void *) +
	// iterator subtraction left/right refs and returned difference; base
	// this/pointer result shared for subtraction and relocation calls.
	2 * sizeof(void *) + sizeof(std::ptrdiff_t) + 2 * sizeof(void *) +
	// Actual emplace return back(): receiver/ref-result2P, deref2P.
	// back() is *(end()-1): iterator member minus receiverP+nD, genuine
	// returned iterator destinationP and pointer-sum boundref temporaryP.
	2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(void *) + sizeof(std::ptrdiff_t) +
	2 * sizeof(void *) +
	// Genuine end() temporary in back()'s *(end()-1). Each distinct
	// realloc Args specialization owns its own begin() expression temporary
	// in position-begin in the realloc caller profile, not this family.
	sizeof(void *);
// These calls bind iterators to const refs or construct prvalue destinations;
// no generated same-type COPY/MOVE normal-iterator ctor is selected here.
// No prefix--/range iteration ++/==, const cbegin/end conversion, distance, advance,
// reverse iterator or forward-insert path is imported as unused Source.
// T=ED/string are distinct typed controller/iterator families. Physical
// vector outputs/work/prior heaps remain original-owned through failure and
// transfer. Global selected lower/controller sharing/old credits remain OPEN.

// PRIVATE UNACCEPTED actual writer vector<ED>/vector<string> growth graph.
// Both selected types are nontrivial and nothrow movable; bitwise/memmove
// relocation and the fallback uninitialized_move_if_noexcept are not selected.
template <class T> constexpr size_t pouch_full_writer_relocation_controller_source =
	// _S_relocate and __relocate_a first/last/result/allocator/return each5P;
	// selected __relocate_a_1 additionally declares real __cur (6P).
	2 * (5 * sizeof(void *)) + 6 * sizeof(void *) +
	// Three raw T* niter_base calls select ONE helper2P family.
	2 * sizeof(void *) +
	// __relocate_object_a dest/orig/allocator; std::move(T) refs/result;
	// construct allocator/location/value; construct_at location/value/return;
	// forward argument/reference return; placement new location/return/size.
	3 * sizeof(void *) + 2 * sizeof(void *) + 3 * sizeof(void *) + 3 * sizeof(void *) +
	2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	// Three __addressof calls select ONE T helper2P family.
	// allocator_traits::destroy allocator/location -> destroy_at location.
	2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(void *) +
	// Actual generated T move-constructor receiver/source and T destructor
	// receiver. Nested T member move/cleanup has separate typed graphs.
	2 * sizeof(void *) + sizeof(void *) +
	// Actual _S_use_relocate and selected _S_nothrow_relocate(true) returns;
	// true tag parameter and original move_insertable tag argument temporary.
	2 * sizeof(bool) + sizeof(std::true_type) +
	sizeof(std::__is_move_insertable<std::allocator<T>>);

// Full selected union41P+N+2B+two actual tags. Old named wrappers may
// credit17P only: _S_relocate5,relocate_a5,relocate_a1 five named args/result,
// and ONE niter helper2. Old repeated niter allowance is unused.
// Missing24P+N+2B+tags retains genuine cur and object construction/cleanup.
// Bitwise ptrdiff/memmove fields cannot credit this nontrivial path.
template <class T> constexpr size_t pouch_full_writer_relocation_missing_source =
	pouch_full_writer_relocation_controller_source<T> - 17 * sizeof(void *);
// For string, move constructor2P/destructorP overlap exact complete string
// family; deduct3P at union composition. ED move2P remains generated; its
// destructorP may overlap actual selected ED cleanup. Never subtract a whole
// vector/list/clone allowance or import clone's _Construct wrapper.

// PRIVATE actual writer default vector method families, finite review.
template <class T> constexpr size_t pouch_full_writer_vector_default_source =
	// Actual vector/Base/Impl/allocator/new_allocator/Impl_data receivers.
	// No returned/default-argument allocator temporary is constructed here.
	6 * sizeof(void *);
constexpr size_t pouch_full_writer_spell_vector_default_missing_source =
	// Old nested i32 default lane names five genuine receiver scopes;
	// new_allocator default receiver is missing. Old A remains unused.
	sizeof(void *);
constexpr size_t pouch_full_writer_string_vector_default_missing_source =
	// vector<string> chunks type has no corresponding one of the three
	// original DA/ED/i32 nested default lanes; full six receivers retained.
	pouch_full_writer_vector_default_source<std::string>;
// Aggregate ED initializes spell_ids by actual vector default construction;
// the later generated ED move reaches its distinct14P vector move ctor.
// Default/cleanup typed helper overlap must be unioned separately, not
// multiplied by physical ED members or credited from an ended clone child.

// PRIVATE finite selected writer/scanner map; complete STL credit OPEN.
constexpr size_t pouch_full_record_request_caller_missing_source =
	// Original own stores index formalN plus eleven actual record_request
	// localN fields (two lexical length vars included), two backing arrays
	// and two list objects, plus FOUR begin/end iterator carriers. Additional
	// value/extra formals+bool and two hidden range aliases are actual.
	// Index formal is already one of the twelve original scalar slots.
	4 * sizeof(void *) + sizeof(bool) +
	// Actual max(size_t{15}, index_digits) binds one distinct scalar temp;
	// max owns two const refs and its returned ref. Old12N are occupied.
	sizeof(size_t) + 3 * sizeof(void *) +
	// Actual initializer_list<size_t> private constructor(this,first,n),
	// begin/end receiver/result, size receiver/result: one selected helper
	// family shared by the TWO real lists. Physical lists remain old-owned.
	7 * sizeof(void *) + 2 * sizeof(size_t);
constexpr size_t pouch_full_writer_body_missing_source =
	// Writer formals values/item/reserve/context/outer and own bool return;
	// original oneN matches second index, leaving twelve genuine local Ns
	// (own/initial/closure/extra sites/capacity/digits/keyword/next/index).
	4 * sizeof(void *) + 13 * sizeof(size_t) + sizeof(bool) +
	// First ED range hidden range/begin/end plus description ref. This
	// caller range is distinct from record_request's two list loops whose
	// four begin/end carriers consumed old own4P; old SV is writer temporary.
	4 * sizeof(void *) +
	// Third-loop std::max(size_t{15}, ledger_prefix.size()) bound temporary.
	sizeof(size_t);
constexpr size_t pouch_full_writer_const_score_index_source =
	// Actual const scores[] is distinct from reader nonconst scores[].
	2 * sizeof(void *) + sizeof(size_t);
constexpr size_t pouch_full_writer_string_vector_index_source =
	// Actual vector<string>[] receiver/index/reference return.
	2 * sizeof(void *) + sizeof(size_t);

// Source of shared std::max and size methods is owned exactly once where
// selected. Argument/result physical destinations above are caller-owned.
// Const ED iterator graph is same authentic reader family; const string
// iterator family for writer_live is distinct. Mutable vector insertion
// iterators/constructors and ED/string push/relocation closure remain OPEN.
// All original request arrays, arithmetic/operators and allocation peaks
// remain byte-unchanged; these are private source declarations only.

// PRIVATE selected C++20 writer vector push frontends, full composition OPEN.
// Actual stl_vector1276/1293 and vector.tcc112/446. Nontrivial relocation
// separately selects the true branch; no unused default_append/forward insert.
constexpr size_t pouch_full_writer_ed_const_push_source =
	// push_back(const ED&): receiver/reference. No emplace_back for this call.
	2 * sizeof(void *);
template <class T> constexpr size_t pouch_full_writer_rvalue_push_source =
	// push_back(T&&) receiver/reference; std::move(T) argument/ref-result;
	// emplace_back<T> receiver/reference/returned-reference.
	2 * sizeof(void *) + 2 * sizeof(void *) + 3 * sizeof(void *);
template <class T> constexpr size_t pouch_full_writer_realloc_insert_caller_source =
	// Actual this/position/argument and old_start/old_finish/new_start/
	// new_finish4P; len and elems_before2N; begin() expression iterator
	// destinationP at position-begin. Each Args signature is distinct.
	8 * sizeof(void *) + 2 * sizeof(size_t);
template <class T> constexpr size_t pouch_full_writer_check_len_source =
	// Receiver and diagnostic-string reference, n/len/returned size; max
	// actual const-reference helper is separately shared size_t family.
	2 * sizeof(void *) + 3 * sizeof(size_t);
template <class T> constexpr size_t pouch_full_writer_max_size_source =
	// vector max, S_max, traits max, newallocator max actual receivers4P;
	// each returned size4N and diffmax/allocmax2N. Actual min3P separately.
	4 * sizeof(void *) + 6 * sizeof(size_t);
template <class T> constexpr size_t pouch_full_writer_direct_construct_source =
	// allocator_traits::construct3P -> runtime construct_at3P -> forward2P
	// -> placement-new2P+N. const-ED and rvalue-ED/string are distinct Args.
	// Constructor receiver/source2P belongs the genuine value family.
	10 * sizeof(void *) + sizeof(size_t);
// At realloc entry end()'s result occupies the actual position argument;
// begin()'s iterator in position-begin and back()'s end()/--/* have real
// returned iterator temporaries. Actual iterator/family lifetime mapping
// must be composed with this caller inventory before any full export.
// Nested failure cleanup is the selected T family; retain actual partial
// copy unwind. Do not import clone's _Construct as direct insertion.

template <class T> constexpr size_t pouch_full_writer_const_allocator_getter_source =
	// Actual public max_size() calls const M_get_Tp_allocator2P. ED can
	// share its exact copy frontend getter; vector<string> must explicitly
	// own this family if no real composed controller already owns it.
	2 * sizeof(void *);
// Rvalue direct_construct10P+N and std::move<T>2P already reside in genuine
// relocation<T> controller union. Do not append those families again. Const-ED
// direct construction has distinct Args=const ED& and is retained separately.

// PRIVATE actual original writer append/operator family union; review OPEN.
constexpr size_t pouch_full_writer_append_caller_source =
	// rvalue+rvalue plus: lhs/rhs2P,use_rhsB,sizeN,actual always_equal tag;
	// rvalue+constchar*: lhs/rhs2P; append(string)3P;
	// append(ptr)3P+N; append(ptr,n)3P+N; M_append3P+2N.
	16 * sizeof(void *) + 5 * sizeof(size_t) + sizeof(bool) +
	sizeof(std::allocator_traits<std::allocator<char>>::is_always_equal);
constexpr size_t pouch_full_writer_append_new_caller_source =
	// Actual operator+=(const string&) this/input/returned-reference.
	// Integral-constant always_equal conversion this/returnedbool.
	3 * sizeof(void *) + sizeof(void *) + sizeof(bool);
constexpr size_t pouch_full_writer_append_mutation_source =
	// _M_mutate receiver,pos,len1,s,len2,how_much,new_capacity,r.
	// All real fields exactly match old mutation subsection once.
	3 * sizeof(void *) + 5 * sizeof(size_t);
constexpr size_t pouch_full_writer_append_length_check_source =
	// _M_check_length(this,n1,n2,diagnostic); descendants max/size shared.
	2 * sizeof(void *) + 2 * sizeof(size_t);
constexpr size_t pouch_full_writer_append_cstring_length_source =
	// char_traits<char>::length input/returnedN and own constant query B;
	// bare builtin strlen adds no declared callee input/return fields.
	// Constant-evaluation alternate generic-character loop is not selected.
	sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_full_writer_string_capacity_query_source =
	// Genuine const capacity receiver/returned size; is_local lower already
	// full string tail. CURRENT may own the same exact getter, not new heap.
	sizeof(void *) + sizeof(size_t);
// Original pouch_append_caller_frames owns the actual16P+5N+B+tag group;
// old pouch_string_frames owns actual mutation3P+5N. Neither lends unused
// assign/_M_replace/disjunct or six unclaimed query fields to new scopes.
// All genuine append descendants _M_create/allocation/set_length/data/length/
// copy/dispose/capacity setters are shared retained string constructor/tail.
// Exact std::move<string> family is shared with typed writer rvalue push.
// Proven original RHS insert branch is unreachable: wanted>RHS fresh length,
// and RHS SSO capacity15 never exceeds LHS capacity when growth is needed.
// Keep original complete expression/arg order/copies/string bytes untouched.

constexpr size_t pouch_full_writer_append_union_source =
	pouch_full_writer_append_caller_source + pouch_full_writer_append_new_caller_source +
	pouch_full_writer_append_mutation_source + pouch_full_writer_append_length_check_source +
	pouch_full_writer_append_cstring_length_source +
	pouch_full_writer_string_capacity_query_source;
constexpr size_t pouch_full_writer_append_missing_source =
	// Exact unambiguous old append-caller and mutation groups ONLY.
	pouch_full_writer_append_union_source - pouch_full_writer_append_caller_source -
	pouch_full_writer_append_mutation_source;
static_assert(pouch_full_writer_append_missing_source ==
	      8 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(bool));

// PRIVATE selected writer constructor Source, common allocator/string leaves
// and genuine old credits still OPEN. No complete provider/export claim.
constexpr size_t pouch_full_fill_string_constructor_own_source =
	// string(n,c,a): this,n,c,allocator-ref. Returned/local string value
	// storage belongs to original to_string __str/caller destination.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(char);
constexpr size_t pouch_full_fill_string_construct_own_source =
	// _M_construct(n,c): receiver and two actual formals. Actual create/
	// capacity/local-init/data/set-length leaves are separately shared.
	sizeof(void *) + sizeof(size_t) + sizeof(char);
constexpr size_t pouch_full_fill_string_assign_source =
	// _S_assign(d,n,c) and char_traits<char>::assign(d,n,c) which returns d;
	// one constant-evaluation bool; runtime memset d,int value,n,result.
	sizeof(void *) + sizeof(size_t) + sizeof(char) + 2 * sizeof(void *) + sizeof(size_t) +
	sizeof(char) + sizeof(bool) + 2 * sizeof(void *) + sizeof(int) + sizeof(size_t);
constexpr size_t pouch_full_view_string_constructor_own_source =
	// string<const SV>(const SV&,a): receiver, input-ref, allocator-ref;
	// _S_to_string_view actual SV by-value formal; return occupies wrapper
	// argument destination. __sv_wrapper ctor receiver + by-value SV;
	// delegated string(wrapper,a) receiver/by-value wrapper/allocator-ref;
	// delegated string(char*,n,a) receiver,source,n,allocator-ref.
	3 * sizeof(void *) + sizeof(std::string_view) + sizeof(void *) + sizeof(std::string_view) +
	2 * sizeof(void *) + sizeof(std::string_view) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t pouch_full_append_equal_traits_conversion_source =
	// Existing old append Source owns genuine is_always_equal object.
	// integral_constant<bool,true>::operator bool owns receiver and result;
	// the discarded allocator comparison/get_allocator branch is not owned.
	sizeof(void *) + sizeof(bool);

// Preserved original record proof: for each appended RHS numeric string,
// wanted exceeds RHS length; SSO RHS capacity15 never exceeds LHS capacity.
// Thus __use_rhs is true, but RHS insert branch is never selected by these
// original operator+ expressions. No insert/allocator-inequality graph is lent.

// Selected writer-only constructor additions established against authentic
// basic_string.h188, string_view136 and basic_string.tcc222 controls.
constexpr size_t pouch_full_writer_const_allocator_hider_source =
	// this/data/const allocator-reference; overload distinct from rvalue hider.
	3 * sizeof(void *);
constexpr size_t pouch_full_writer_const_char_construct_source =
	// _M_construct<const char*> own receiver/beg/end/tag/dnew/guard;
	// _Guard ctor/destructor; distance/__distance/category and actual
	// const-char _S_copy_chars. None is mutable-char clone specialization.
	15 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(std::ptrdiff_t) +
	sizeof(std::forward_iterator_tag) + sizeof(std::random_access_iterator_tag);
constexpr size_t pouch_full_writer_constructor_default_allocator_source =
	// Real default-argument allocator object plus allocator/newallocator
	// default constructor receivers. Typed cleanup already common tail.
	sizeof(std::allocator<char>) + 2 * sizeof(void *);
// Explicit SV copy2P, data2P and size(P+N) are genuinely selected by wrapper
// and _S_to_string_view return. The actual shared reader string-view component
// already owns all three exact helper families; do not add those5P+N again.
// Full global Source/old-slot partition is OPEN: these additions do not claim
// the incomplete old writer copy-constructor group qualifies mutable or
// const-char construction, nor borrow completed clone high-water.

// PRIVATE UNACCEPTED selected member lifetime graph for writer ED/string.
// Actual GNU13 C++20 controls; no entire list-lifetime/decoder total imported.
template <class T> constexpr size_t pouch_full_vector_move_constructor_source =
	// Defaulted vector/base moves; actual Impl move, allocator/new_allocator
	// const-copy construction, Impl_data move: each this/source family2P.
	6 * (2 * sizeof(void *)) +
	// Impl invokes std::move(__x) twice with the SAME Impl-reference
	// specialization: retain its argument/result2P family only once.
	// Impl_data pointer() resets existing members, no new declared carrier.
	2 * sizeof(void *);
constexpr size_t pouch_full_string_default_constructor_source =
	// string receiver; hider this/data/allocator-ref; actual default allocator
	// object plus allocator/new_allocator default and const-copy receivers.
	sizeof(void *) + 3 * sizeof(void *) + sizeof(std::allocator<char>) + 2 * sizeof(void *) +
	2 * (2 * sizeof(void *)) +
	// Actual hider std::move allocator family; local pointer chain;
	// runtime init query; complete set-length/assign/char-NUL graph.
	2 * sizeof(void *) + pouch_full_string_local_mutable_source + sizeof(void *) +
	sizeof(bool) + sizeof(size_t) + sizeof(char) + pouch_full_string_set_length_source;
constexpr size_t pouch_full_string_move_constructor_source =
	// string this/source; rvalue-allocator hider this/data/allocator-ref,
	// actual allocator/new_allocator const copies; std::move allocator
	// refs/result family shared between constructor and hider, get_allocator.
	2 * sizeof(void *) + 3 * sizeof(void *) + 2 * (2 * sizeof(void *)) + 2 * sizeof(void *) +
	2 * sizeof(void *) + pouch_full_string_local_mutable_source + sizeof(void *) +
	sizeof(bool) + sizeof(size_t) + sizeof(char) +
	// is_local/data/const local plus actual source.length receiver/result.
	sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + pouch_full_string_local_const_source +
	sizeof(void *) + sizeof(size_t) +
	// data/capacity setters; source use-local receiver/result. Init/local
	// method families already above; destination _M_length is same selected
	// setter reached by the reset _M_set_length graph, not charged again.
	2 * sizeof(void *) + sizeof(void *) + sizeof(size_t) + 2 * sizeof(void *) +
	// Complete reset set-length except its identical _M_data query already
	// mapped above. Actual NUL/assign query and count formals remain.
	pouch_full_string_set_length_source - 2 * sizeof(void *) +
	// Direct traits::copy in local source branch, not _S_copy/char1 branch;
	// traits::copy owns its real formals/result/query. Bare builtin
	// memcpy introduces no separately declared Source callee fields.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t pouch_full_writer_ed_move_members_source =
	// ED generated move receiver/source lives in relocation controller or
	// allocator construction. Keyword/description are two physical member
	// objects already in ED, but select ONE std::string method-family union.
	// Spell-vector move is the genuine distinct selected typed descendant.
	pouch_full_string_move_constructor_source +
	pouch_full_vector_move_constructor_source<int32_t>;

// No implicit trivial new_allocator/Impl_data/array destructor body is
// invented. The string move constructor makes no allocator/heap request;
// physical member/string-return objects and prior heaps remain old-owned.
// Default/move/destroy shared helper Source dedup and old exact typed credits
// remain OPEN until coherent full-provider composition and finite review.

// The added init N+char is the Root-required separately labeled conservative
// owning-body allowance. Global Source union retains this identical helper
// once, not again for each constructor/tail use or each physical member.

// PRIVATE exact-family union PROPOSAL, independent review/credit OPEN.
// Existing selected copy+assignment+cleanup union already owns mutable and
// const local-data chains, allocator/newallocator const COPY constructors,
// rvalue allocator hider, std::move<allocator>, init+length+traits copy,
// getter/setter/allocation/query/disposal methods and conservative init N+C.
constexpr size_t pouch_full_writer_string_default_increment_source =
	// Remaining default-string receiverP and allocator/newallocator DEFAULT
	// constructor receivers2P. This real default-arg allocator object is
	// distinct from copy's materialized select-on-copy allocator value.
	3 * sizeof(void *) + sizeof(std::allocator<char>);
constexpr size_t pouch_full_writer_string_move_increment_source =
	// Move constructor this/source2P, all selected lower helpers shared with
	// actual retained string copy/tail union. Not a second full string graph.
	2 * sizeof(void *);
constexpr size_t pouch_full_writer_string_value_union_source =
	pouch_full_string_copy_tail_union_source +
	pouch_full_writer_string_default_increment_source +
	pouch_full_writer_string_move_increment_source;
constexpr size_t pouch_full_writer_string_constructor_union_source =
	pouch_full_writer_string_value_union_source +
	pouch_full_writer_const_allocator_hider_source +
	pouch_full_writer_const_char_construct_source +
	pouch_full_view_string_constructor_own_source +
	pouch_full_fill_string_constructor_own_source +
	pouch_full_fill_string_construct_own_source + pouch_full_fill_string_assign_source +
	// Distinct real default-argument allocator objects at fill to_string and
	// view keyword constructors; constructors themselves shared2P above.
	// View-default can coexist with RHS fill-default in keyword expression.
	2 * sizeof(std::allocator<char>);
// SV data/size/copy families reside in the actual shared string-view Source.
// Constructor default allocator default receivers2P already value union;
// two distinct fill/view materializations are owned explicitly above.
// Neither borrows ended copy allocator lifetime or creates returned extras.
// Physical returned strings/ED members/record eight original objects remain
// old-admitted; no guessed NRVO object is added. No original behavior changes.
// Selected constructor subtotal is not complete writer STL Source: append
// mutation/disjunct/check/string-plus caller and typed push/iterator graphs
// and exact old-credit partition are explicit remaining composition work.

// The original children remain unchanged. This is a conservative Source
// union, not emitted stack, heap capacity, an inventory substitute or a new
// admission algorithm. Only unambiguous original caller objects/fields below
// are credited; partial historical library allowances are deliberately unused.
// Initial is the one outer pouch_budget. Foreign inputs, usage/prior-output
// objects, alias heaps and callback descendants remain caller-owned once.
constexpr size_t pouch_full_prepare_caller_matched =
	sizeof(std::vector<uint8_t>) + sizeof(scores) + sizeof(player_item_snapshot) +
	4 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool) +
	sizeof(player_snapshot_codec_result) + sizeof(std::span<const chaos_material_pouch_usage>) +
	sizeof(chaos_pouch_usage_mode);
constexpr size_t pouch_full_prepare_caller =
	pouch_full_prepare_caller_matched + pouch_full_prepare_caller_missing_source;
constexpr size_t pouch_full_singleton_caller_matched =
	sizeof(std::initializer_list<player_item_snapshot>) +
	sizeof(std::vector<player_item_snapshot>) + sizeof(pouch_budget) +
	2 * sizeof(player_item_snapshot) + 4 * sizeof(void *);
constexpr size_t pouch_full_singleton_caller =
	pouch_full_singleton_caller_matched + pouch_full_singleton_caller_missing_source +
	// Original unary+ lambda conversion materializes its own empty closure.
	// The thunk closure is a distinct later Source object, owned in helpers.
	sizeof(char);
constexpr size_t pouch_full_clone_caller_matched = sizeof(player_item_snapshot) +
						   4 * sizeof(void *) + 3 * sizeof(size_t) +
						   sizeof(player_snapshot_codec_result);
constexpr size_t pouch_full_clone_caller =
	pouch_full_clone_caller_matched + pouch_full_clone_entry_missing_source;
constexpr size_t pouch_full_writer_caller_matched =
	sizeof(pouch_write_frame) + sizeof(pouch_budget) +
	sizeof(player_item_extra_description_snapshot) +
	// Eight record-expression objects including the final record destination,
	// plus the two keyword expression temporaries. The final keyword is a
	// member of the already owned aggregate ED; no third returned object.
	10 * sizeof(std::string) + sizeof(std::array<size_t, 3>) + sizeof(std::array<size_t, 5>) +
	2 * sizeof(std::initializer_list<size_t>) + 13 * sizeof(size_t) + 8 * sizeof(void *) +
	sizeof(std::string_view);
constexpr size_t pouch_full_writer_caller_missing =
	// Writer own/range/formals + record_request own/range/formals. The max
	// and initializer-list helper families are separately owned below.
	12 * sizeof(void *) + 15 * sizeof(size_t) + 2 * sizeof(bool);
constexpr size_t pouch_full_writer_caller =
	pouch_full_writer_caller_matched + pouch_full_writer_caller_missing;

constexpr size_t pouch_full_observation_own =
	// Actual snapshot_clone vector request<DA,ED,i32> signatures/count/result;
	// string request signature/count/result; row request + const ED range;
	// two public CURRENT/fresh roots, policy return and item_list_add.
	// String/vector query and const ED iterator descendants live in the
	// genuine selected shared components, not the old observation inventory.
	19 * sizeof(void *) + 7 * sizeof(size_t) + 14 * sizeof(bool);
constexpr size_t pouch_full_ledger_helper_own =
	pouch_full_checked_add_source + pouch_full_admit_source +
	// heap<ED,string,byte,i32>, copy<i32>: signatures only. The actual
	// capacity/size helpers are shared with the selected vector components.
	5 * (2 * sizeof(void *) + sizeof(bool)) + pouch_full_string_heap_source +
	pouch_full_string_copy_source + pouch_full_description_heap_source +
	pouch_full_item_heap_source +
	pouch_full_push_request_source<player_item_extra_description_snapshot> +
	pouch_full_push_request_source<std::string> + pouch_full_append_request_source +
	pouch_full_digits_source + pouch_full_writer_live_source +
	pouch_full_writer_admit_operator_source + pouch_full_singleton_relay_operator_source +
	pouch_full_singleton_relay_thunk_source + pouch_full_singleton_relay_conversion_source +
	pouch_full_material_index_source + pouch_full_usage_numeric_limit_source +
	// std::max<size_t>, initializer_list<size_t> ctor/begin/end/size. The
	// two backing arrays and two physical lists are in writer caller storage.
	10 * sizeof(void *) + 2 * sizeof(size_t) + pouch_full_observation_own +
	// Actual singleton copy-frame getter result before construction; this is
	// only its scalar return, never a complete clone Source provider.
	sizeof(size_t);

constexpr size_t pouch_full_selected_vector_union =
	pouch_full_copy_vector_dynamic_source + pouch_full_copy_vector_ed_source +
	pouch_full_copy_vector_int32_source + pouch_full_final_vector_tail_source -
	// Exact copy/controller shared frontend11P per DA/ED; i32 cleanup's
	// mutable allocator getter2P. Do not subtract old credits here.
	32 * sizeof(void *) +
	// Three copy begin/end destinations are their uninitialized_copy_a
	// by-value parameters by guaranteed elision: deduct6P, not a new object.
	// ED copy backend __addressof2P shares its actual cleanup leaf.
	// DA/ED/i32 defaults add five new receiver families each; their data
	// default receiver is already in each genuine copy frontend.
	15 * sizeof(void *) +
	// ED's actual spell-vector move: its allocator/newallocator const copy
	// fourP are already copy frontend. Impl_data MOVE is distinct from default.
	pouch_full_vector_move_constructor_source<int32_t> - 4 * sizeof(void *) +
	// work.chunks is default/push-grown/destroyed, never vector-assigned.
	// Own only the actual default six receivers, genuine cleanup minus its
	// shared string destructor and __addressof<string>2P, plus actual max getter/allocation.
	pouch_full_writer_vector_default_source<std::string> +
	pouch_full_vector_cleanup_source<std::string> - 3 * sizeof(void *) +
	pouch_full_writer_const_allocator_getter_source<std::string> +
	pouch_full_copy_vector_allocation_source<std::string> +
	// Actual public size/capacity observations: DA/ED/i32 size is already
	// copy frontend; all three capacity and string size/capacity are new.
	5 * (sizeof(void *) + sizeof(size_t)) +
	// Writer public max and check_len, one per actual ED/string type. The
	// allocator/newallocator max P+N is shared with its real allocation path.
	pouch_full_writer_max_size_source<player_item_extra_description_snapshot> +
	pouch_full_writer_max_size_source<std::string> - 2 * (sizeof(void *) + sizeof(size_t)) +
	pouch_full_writer_check_len_source<player_item_extra_description_snapshot> +
	pouch_full_writer_check_len_source<std::string> +
	// ED const and rvalue, string rvalue: three distinct Args realloc callers.
	pouch_full_writer_ed_const_push_source +
	pouch_full_writer_rvalue_push_source<player_item_extra_description_snapshot> +
	pouch_full_writer_rvalue_push_source<std::string> - 4 * sizeof(void *) +
	2 * pouch_full_writer_realloc_insert_caller_source<player_item_extra_description_snapshot> +
	pouch_full_writer_realloc_insert_caller_source<std::string> +
	pouch_full_writer_mutable_iterator_source<player_item_extra_description_snapshot> +
	pouch_full_writer_mutable_iterator_source<std::string> +
	// Direct const ED construct has different Args. forward<const ED&> and
	// placement boundary already actual ED vector-copy backend: deduct2P here;
	// the global placement operator overlap is removed separately below.
	pouch_full_writer_direct_construct_source<player_item_extra_description_snapshot> -
	2 * sizeof(void *) +
	pouch_full_writer_relocation_controller_source<player_item_extra_description_snapshot> -
	// ED __addressof2P/destroy_atP/dtorP shared with real ED cleanup.
	4 * sizeof(void *) + pouch_full_writer_relocation_controller_source<std::string> -
	// String value move ctor2P/dtorP, plus string cleanup addressof2P/destroy_atP.
	6 * sizeof(void *) +
	// ED const range is already in actual copy graph. i32 empty adds only
	// its receiver/return, two returned destinations and actual == family.
	5 * sizeof(void *) + 2 * sizeof(bool) +
	// writer_live's const string range has its distinct genuine normal type.
	// Stored pointer lvalues bind directly: no qualification temporaries.
	12 * sizeof(void *) + sizeof(bool) + pouch_full_singleton_vector_constructor_source +
	pouch_full_singleton_raw_row_copy_backend_source +
	pouch_full_copy_vector_allocation_source<player_item_snapshot> +
	pouch_full_singleton_internal_max_source - (sizeof(void *) + sizeof(size_t)) +
	pouch_full_vector_cleanup_source<player_item_snapshot> - sizeof(void *) +
	// row destructor receiver above is supplied by the generated row family.
	pouch_full_singleton_front_source +
	// front's normal-iterator actual destination remains. Its begin/deref
	// signatures do not substitute for the raw-row constructor backend.
	// No separate bare builtin memmove callee was declared; both typed
	// copy families already exclude it, so there is no intrinsic sharing cut.
	0 +
	// Global operator new and sized delete: six real typed allocation/delete
	// chains including the common char-string family use one selected boundary.
	-5 * (sizeof(void *) + sizeof(size_t)) - 5 * (sizeof(void *) + sizeof(size_t)) +
	// Five actual placement boundaries (row/ED copy, const ED construct,
	// ED/string relocation) share the same operator new(size_t,void*) body.
	-4 * (2 * sizeof(void *) + sizeof(size_t));

constexpr size_t pouch_full_generated_value_source =
	2 * sizeof(void *) + pouch_full_row_array_copy_constructor_source +
	pouch_full_row_move_assignment_final_tail_source + pouch_full_row_generated_cleanup_source +
	pouch_full_row_default_own_source +
	// ED generated copy2P; generated move2P is already real relocation;
	// generated ED destructorP already vector ED cleanup.
	2 * sizeof(void *) + pouch_full_scores_default_source +
	pouch_full_prepare_frame_lifetime_own_source + pouch_full_writer_frame_lifetime_own_source;

constexpr size_t pouch_full_prepare_shared_source =
	pouch_full_ledger_helper_own + pouch_full_selected_vector_union +
	pouch_full_generated_value_source + pouch_full_reader_caller_source +
	pouch_full_reader_array_source + pouch_full_reader_decimal_source +
	pouch_full_shared_string_view_source + pouch_full_writer_string_constructor_union_source -
	// Exact shared string _M_data2P and size(P+N), not a whole query pool.
	(3 * sizeof(void *) + sizeof(size_t)) + pouch_full_writer_append_union_source +
	// to_string has a genuine named __str local. NRVO is optional, so
	// retain this Source object plus its existing expression destination;
	// the returned aggregate introduces no third string or extra heap.
	pouch_full_writer_to_string_source + pouch_full_writer_decimal_length_source +
	pouch_full_writer_decimal_digits_source +
	// Actual basic_string::empty and mutable operator[] at to_string str[0].
	// Their data/size descendants are already in the shared string union.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(bool) + pouch_full_usage_span_method_source +
	pouch_full_usage_span_parameter_lifetime_source +
	pouch_full_writer_const_score_index_source + pouch_full_writer_string_vector_index_source;
constexpr size_t pouch_full_prepare_own_source =
	pouch_full_prepare_shared_source + pouch_full_prepare_caller + pouch_full_singleton_caller +
	pouch_full_clone_caller + pouch_full_writer_caller;
constexpr size_t pouch_full_prepare_missing_own_source =
	// Deliberately conservative: only these exact original caller fields and
	// physical objects, plus reader's individually mapped real fields, are
	// claimed. Historical library/clone/singleton repeated partial inventories
	// are not treated as complete providers or as fungible spare bytes.
	pouch_full_prepare_shared_source -
	(pouch_full_reader_caller_source - pouch_full_reader_missing_source) +
	pouch_full_prepare_caller_missing_source + pouch_full_singleton_caller_missing_source +
	sizeof(char) + pouch_full_clone_entry_missing_source + pouch_full_writer_caller_missing;
constexpr size_t pouch_full_verify_caller_matched =
	sizeof(player_item_snapshot) + 2 * sizeof(std::vector<uint8_t>) + 4 * sizeof(void *) +
	3 * sizeof(size_t) + sizeof(bool) + sizeof(player_snapshot_codec_result) +
	sizeof(std::span<const chaos_material_pouch_usage>) + sizeof(chaos_pouch_usage_mode);
constexpr size_t pouch_full_verify_own_source =
	pouch_full_verify_caller_matched + pouch_full_verify_caller_missing_source +
	// Nested prepare has its own initial controller, distinct from verify's
	// outer initial object. All already composed method families shared once.
	sizeof(pouch_budget) + pouch_full_verify_frame_lifetime_own_source +
	pouch_full_verify_byte_equality_source;
// Actual __memcmp wrapper remains; no bare intrinsic sharing cut.
constexpr size_t pouch_full_verify_missing_own_source =
	pouch_full_verify_caller_missing_source + pouch_full_verify_frame_lifetime_own_source +
	pouch_full_verify_byte_equality_source;
// Actual __memcmp wrapper remains; no bare intrinsic sharing cut.
static_assert(pouch_full_prepare_missing_own_source > 0);
static_assert(pouch_full_prepare_missing_own_source < pouch_full_prepare_own_source);
static_assert(pouch_full_verify_missing_own_source < pouch_full_verify_own_source);

#endif
}
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
bool pouch_full_ledger_profile(bool verify, bool supplement, size_t *output) noexcept
{
	if (!output)
		return false;
	size_t encoded_source = 0;
	if (!player_item_snapshot_list_encode_source_frame_bytes(&encoded_source))
		return false;
	size_t result = supplement ? pouch_full_prepare_missing_own_source :
				     pouch_full_prepare_own_source;
	// Actual lower call is the selected list ENCODE provider. Row observation
	// is derived from its real roots above, without a copy/decode margin.
	if (!pouch_add(result, encoded_source))
		return false;
	if (verify && !pouch_add(result, supplement ? pouch_full_verify_missing_own_source :
						      pouch_full_verify_own_source))
		return false;
	// Final sole scalar transfer: null/unsupported/lower refusal/overflow leaves
	// the destination unchanged. Pure no-allocation/no-observation query.
	*output = result;
	return true;
}
#endif
} // namespace
bool chaos_pouch_ledger_prepare_source_frame_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	return pouch_full_ledger_profile(false, false, output);
#else
	(void)output;
	return false;
#endif
}
bool chaos_pouch_ledger_prepare_source_supplement_frame_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	return pouch_full_ledger_profile(false, true, output);
#else
	(void)output;
	return false;
#endif
}
bool chaos_pouch_ledger_verify_source_frame_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	return pouch_full_ledger_profile(true, false, output);
#else
	(void)output;
	return false;
#endif
}
bool chaos_pouch_ledger_verify_source_supplement_frame_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	return pouch_full_ledger_profile(true, true, output);
#else
	(void)output;
	return false;
#endif
}
