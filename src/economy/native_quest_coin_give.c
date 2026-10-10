#include "economy/native_quest_coin_give.h"

#include <climits>
#include <new>
#include <utility>

namespace
{
bool native_counts(const std::array<int64_t, 4> &cash) noexcept
{
	for (const int64_t count : cash)
		if (count < 0 || count > INT_MAX)
			return false;
	return true;
}
void put(std::vector<uint8_t> &bytes, size_t offset, uint64_t value, size_t count) noexcept
{
	for (size_t i = 0; i < count; ++i)
		bytes[offset + i] = static_cast<uint8_t>(value >> (i * 8));
}
uint64_t get(std::span<const uint8_t> bytes, size_t offset, size_t count) noexcept
{
	uint64_t value = 0;
	for (size_t i = 0; i < count; ++i)
		value |= static_cast<uint64_t>(bytes[offset + i]) << (i * 8);
	return value;
}
}

native_quest_coin_give_result native_quest_coin_give_project(
	const std::array<int64_t, 4> &player_before, uint64_t player_revision,
	const std::array<int64_t, 4> &mobile_before, uint64_t mobile_revision, uint8_t denomination,
	int32_t quantity, native_quest_coin_give_projection *output) noexcept
{
	using result = native_quest_coin_give_result;
	if (!output || !mobile_revision || denomination >= player_before.size() || quantity <= 0 ||
	    !native_counts(player_before) || !native_counts(mobile_before))
		return result::invalid;
	if (player_before[denomination] < quantity)
		return result::insufficient;
	constexpr std::array<int64_t, 4> units{ 1, 10, 100, 1000 };
	if (player_revision == UINT64_MAX || mobile_revision == UINT64_MAX ||
	    quantity > INT_MAX / units[denomination])
		return result::overflow;
	native_quest_coin_give_projection candidate;
	candidate.denomination = denomination;
	candidate.quantity = quantity;
	candidate.player_before_revision = player_revision;
	candidate.player_after_revision = player_revision + 1;
	candidate.mobile_before_revision = mobile_revision;
	candidate.mobile_after_revision = mobile_revision + 1;
	candidate.player_before = candidate.player_after = player_before;
	candidate.mobile_before = candidate.mobile_after = mobile_before;
	candidate.player_after[denomination] -= quantity;
	// Original src/cmd/actobj.c begin_coin_give_credit passes copper value
	// to src/core/utility.c ADD_MONEY, largest denomination first. It adds
	// normalized credit only; existing recipient cash is not exchanged.
	int64_t copper = static_cast<int64_t>(quantity) * units[denomination];
	for (size_t i = candidate.mobile_after.size(); i-- > 0;)
	{
		const int64_t added = copper / units[i];
		if (candidate.mobile_after[i] > INT_MAX - added)
			return result::overflow;
		candidate.mobile_after[i] += added;
		copper %= units[i];
	}
	*output = candidate;
	return result::ok;
}

native_quest_coin_give_result
native_quest_coin_give_encode(const native_quest_coin_give_projection &value,
			      std::vector<uint8_t> *output) noexcept
{
	using result = native_quest_coin_give_result;
	native_quest_coin_give_projection projected;
	if (!output ||
	    native_quest_coin_give_project(value.player_before, value.player_before_revision,
					   value.mobile_before, value.mobile_before_revision,
					   value.denomination, value.quantity,
					   &projected) != result::ok ||
	    projected != value)
		return result::invalid;
	try
	{
		std::vector<uint8_t> bytes(NATIVE_QUEST_COIN_GIVE_BYTES, 0);
		bytes[0] = 'N';
		bytes[1] = 'Q';
		bytes[2] = 'G';
		bytes[3] = '1';
		put(bytes, 4, 1, 2);
		bytes[6] = value.denomination;
		put(bytes, 8, static_cast<uint32_t>(value.quantity), 4);
		put(bytes, 16, value.player_before_revision, 8);
		put(bytes, 24, value.player_after_revision, 8);
		put(bytes, 32, value.mobile_before_revision, 8);
		put(bytes, 40, value.mobile_after_revision, 8);
		const std::array<const std::array<int64_t, 4> *, 4> arrays{ &value.player_before,
									    &value.player_after,
									    &value.mobile_before,
									    &value.mobile_after };
		for (size_t a = 0; a < arrays.size(); ++a)
			for (size_t i = 0; i < arrays[a]->size(); ++i)
				put(bytes, 48 + a * 32 + i * 8,
				    static_cast<uint64_t>((*arrays[a])[i]), 8);
		*output = std::move(bytes);
		return result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return result::allocation_failure;
	}
}

native_quest_coin_give_result
native_quest_coin_give_decode(std::span<const uint8_t> bytes,
			      native_quest_coin_give_projection *output) noexcept
{
	using result = native_quest_coin_give_result;
	if (!output || bytes.size() != NATIVE_QUEST_COIN_GIVE_BYTES || bytes[0] != 'N' ||
	    bytes[1] != 'Q' || bytes[2] != 'G' || bytes[3] != '1' || get(bytes, 4, 2) != 1 ||
	    bytes[7] || get(bytes, 12, 4) || get(bytes, 8, 4) > INT_MAX)
		return result::invalid;
	native_quest_coin_give_projection value;
	value.denomination = bytes[6];
	value.quantity = static_cast<int32_t>(get(bytes, 8, 4));
	value.player_before_revision = get(bytes, 16, 8);
	value.player_after_revision = get(bytes, 24, 8);
	value.mobile_before_revision = get(bytes, 32, 8);
	value.mobile_after_revision = get(bytes, 40, 8);
	const std::array<std::array<int64_t, 4> *, 4> arrays{
		&value.player_before, &value.player_after, &value.mobile_before, &value.mobile_after
	};
	for (size_t a = 0; a < arrays.size(); ++a)
		for (size_t i = 0; i < arrays[a]->size(); ++i)
		{
			const uint64_t count = get(bytes, 48 + a * 32 + i * 8, 8);
			if (count > INT_MAX)
				return result::invalid;
			(*arrays[a])[i] = static_cast<int64_t>(count);
		}
	native_quest_coin_give_projection projected;
	if (native_quest_coin_give_project(value.player_before, value.player_before_revision,
					   value.mobile_before, value.mobile_before_revision,
					   value.denomination, value.quantity,
					   &projected) != result::ok ||
	    projected != value)
		return result::invalid;
	*output = value;
	return result::ok;
}
namespace
{
bool give_bound_add(size_t &bytes, size_t extra) noexcept
{
	if (extra > SIZE_MAX - bytes)
		return false;
	bytes += extra;
	return true;
}
constexpr size_t give_bound_allocator_frames =
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
constexpr size_t give_bound_copy_frames =
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
constexpr size_t give_bound_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t give_bound_default_frames =
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
constexpr size_t give_bound_vector_frames =
	give_bound_allocator_frames + give_bound_copy_frames + give_bound_relocate_frames +
	give_bound_default_frames +
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
constexpr size_t give_bound_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + give_bound_allocator_frames;

// Genuine default vector/base/impl/data and allocator/new_allocator this
// carriers. The vector object's fields are already included in its owner.
constexpr size_t give_bound_empty_constructor_frames = 6 * sizeof(void *);
// vector(n,a), actual default allocator temporary, _S_check_init_len n/a/return
// and allocator copy, _Vector_base this/n/a, _Vector_impl this/a/copy,
// _Vector_impl_data this and _M_create_storage this/n. The existing vector
// allocator profile owns _S_max_size and allocation scopes.
constexpr size_t give_bound_size_constructor_frames =
	2 * sizeof(void *) + sizeof(size_t) + sizeof(std::allocator<uint8_t>) + sizeof(void *) +
	2 * sizeof(size_t) + sizeof(std::allocator<uint8_t>) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) +
	sizeof(size_t);

// Whole genuine allocation-free native coin-give projection: candidate and
// units array, original parameters, copper/added/index and result; native_counts
// count/range/array query scopes. Inputs are caller-owned references.
constexpr size_t give_bound_project_frames =
	sizeof(native_quest_coin_give_projection) + sizeof(std::array<int64_t, 4>) +
	3 * sizeof(void *) + 2 * sizeof(uint64_t) + sizeof(uint8_t) + sizeof(int32_t) +
	2 * sizeof(int64_t) + sizeof(size_t) + sizeof(native_quest_coin_give_result) +
	3 * sizeof(void *) + sizeof(int64_t) + sizeof(bool) + 8 * (sizeof(void *) + sizeof(size_t));
// Full defaulted projection equality: this/other/result, four authentic array
// equality->equal/aux/aux1/equal<true>/niter/memcmp chains. This profile sums
// their distinct source call scopes, not emitted machine stack.
constexpr size_t give_bound_equal_frames =
	2 * sizeof(void *) + sizeof(bool) +
	4 * (2 * sizeof(void *) + sizeof(bool) + 3 * (2 * sizeof(void *)) +
	     4 * (3 * sizeof(void *) + sizeof(bool)) + 3 * (2 * sizeof(void *)) + sizeof(size_t) +
	     2 * (2 * sizeof(void *) + sizeof(size_t) + sizeof(int)));
// put/get actual parameter/return/scalar loop scopes, vector/span/array query
// calls; complete encoder projected + decode value/projected and pointer array.
constexpr size_t give_bound_wire_frames =
	2 * sizeof(std::span<const uint8_t>) + 2 * sizeof(void *) + 5 * sizeof(size_t) +
	3 * sizeof(uint64_t) + 12 * (sizeof(void *) + sizeof(size_t));
constexpr size_t give_bound_encode_frames =
	sizeof(native_quest_coin_give_projection) +
	sizeof(std::array<const std::array<int64_t, 4> *, 4>) + 5 * sizeof(void *) +
	4 * sizeof(size_t) + sizeof(native_quest_coin_give_result) + give_bound_project_frames +
	give_bound_equal_frames + give_bound_wire_frames;
constexpr size_t give_bound_decode_frames =
	2 * sizeof(native_quest_coin_give_projection) +
	sizeof(std::array<std::array<int64_t, 4> *, 4>) + 4 * sizeof(void *) + 4 * sizeof(size_t) +
	sizeof(uint64_t) + sizeof(native_quest_coin_give_result) + give_bound_project_frames +
	give_bound_equal_frames + give_bound_wire_frames;
// vector(n,zero,a): value reference/zero temporary plus actual fill scopes:
// _M_fill_initialize, __uninitialized_fill_n_a, uninitialized_fill_n and
// __uninitialized_fill_n<true>; val-copy, returned iterator and fill_n chain.
constexpr size_t give_bound_fill_frames =
	give_bound_size_constructor_frames + sizeof(void *) + sizeof(uint8_t) +
	4 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(uint8_t) + sizeof(bool) + sizeof(char);
bool give_bound_peak(bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer,
		     size_t fixed, size_t extra) noexcept
{
	// Genuine helper arguments/return and add reference/extra/result carriers,
	// reserve argument/result; outer itself is the current scalar total.
	constexpr size_t own = 4 * sizeof(void *) + 4 * sizeof(size_t) + 3 * sizeof(bool);
	return give_bound_add(outer, fixed) && give_bound_add(outer, extra) &&
	       give_bound_add(outer, own) && reserve && reserve(outer, context);
}
} // namespace

native_quest_coin_give_result native_quest_coin_give_encode_bounded(
	const native_quest_coin_give_projection &value, std::vector<uint8_t> *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)value;
	(void)output;
	(void)reserve;
	(void)context;
	(void)outer;
	return native_quest_coin_give_result::invalid;
#else
	using result = native_quest_coin_give_result;
	if (!give_bound_peak(reserve, context, outer, give_bound_encode_frames, 0))
		return result::allocation_failure;
	native_quest_coin_give_projection projected;
	if (!output ||
	    native_quest_coin_give_project(value.player_before, value.player_before_revision,
					   value.mobile_before, value.mobile_before_revision,
					   value.denomination, value.quantity,
					   &projected) != result::ok ||
	    projected != value)
		return result::invalid;
	try
	{
		if (!give_bound_peak(reserve, context, outer, give_bound_encode_frames,
				     sizeof(std::vector<uint8_t>) + NATIVE_QUEST_COIN_GIVE_BYTES +
					     give_bound_vector_frames + give_bound_fill_frames))
			return result::allocation_failure;
		std::vector<uint8_t> bytes(NATIVE_QUEST_COIN_GIVE_BYTES, 0);
		bytes[0] = 'N';
		bytes[1] = 'Q';
		bytes[2] = 'G';
		bytes[3] = '1';
		put(bytes, 4, 1, 2);
		bytes[6] = value.denomination;
		put(bytes, 8, static_cast<uint32_t>(value.quantity), 4);
		put(bytes, 16, value.player_before_revision, 8);
		put(bytes, 24, value.player_after_revision, 8);
		put(bytes, 32, value.mobile_before_revision, 8);
		put(bytes, 40, value.mobile_after_revision, 8);
		const std::array<const std::array<int64_t, 4> *, 4> arrays{ &value.player_before,
									    &value.player_after,
									    &value.mobile_before,
									    &value.mobile_after };
		for (size_t a = 0; a < arrays.size(); ++a)
			for (size_t i = 0; i < arrays[a]->size(); ++i)
				put(bytes, 48 + a * 32 + i * 8,
				    static_cast<uint64_t>((*arrays[a])[i]), 8);
		if (!give_bound_peak(reserve, context, outer, give_bound_encode_frames,
				     sizeof(bytes) + bytes.capacity() + give_bound_move_frames))
			return result::allocation_failure;
		*output = std::move(bytes);
		return result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return result::allocation_failure;
	}

#endif
}

native_quest_coin_give_result native_quest_coin_give_decode_bounded(
	std::span<const uint8_t> bytes, native_quest_coin_give_projection *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)bytes;
	(void)output;
	(void)reserve;
	(void)context;
	(void)outer;
	return native_quest_coin_give_result::invalid;
#else
	using result = native_quest_coin_give_result;
	if (!output || bytes.size() != NATIVE_QUEST_COIN_GIVE_BYTES || bytes[0] != 'N' ||
	    bytes[1] != 'Q' || bytes[2] != 'G' || bytes[3] != '1' || get(bytes, 4, 2) != 1 ||
	    bytes[7] || get(bytes, 12, 4) || get(bytes, 8, 4) > INT_MAX)
		return result::invalid;
	if (!give_bound_peak(reserve, context, outer, give_bound_decode_frames, 0))
		return result::allocation_failure;
	native_quest_coin_give_projection value;
	value.denomination = bytes[6];
	value.quantity = static_cast<int32_t>(get(bytes, 8, 4));
	value.player_before_revision = get(bytes, 16, 8);
	value.player_after_revision = get(bytes, 24, 8);
	value.mobile_before_revision = get(bytes, 32, 8);
	value.mobile_after_revision = get(bytes, 40, 8);
	const std::array<std::array<int64_t, 4> *, 4> arrays{
		&value.player_before, &value.player_after, &value.mobile_before, &value.mobile_after
	};
	for (size_t a = 0; a < arrays.size(); ++a)
		for (size_t i = 0; i < arrays[a]->size(); ++i)
		{
			const uint64_t count = get(bytes, 48 + a * 32 + i * 8, 8);
			if (count > INT_MAX)
				return result::invalid;
			(*arrays[a])[i] = static_cast<int64_t>(count);
		}
	native_quest_coin_give_projection projected;
	if (native_quest_coin_give_project(value.player_before, value.player_before_revision,
					   value.mobile_before, value.mobile_before_revision,
					   value.denomination, value.quantity,
					   &projected) != result::ok ||
	    projected != value)
		return result::invalid;
	*output = value;
	return result::ok;

#endif
}
