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
