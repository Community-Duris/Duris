#include "economy/native_quest_cost.h"

#include <climits>
#include <bit>
#include <algorithm>
#include <new>
#include <utility>

namespace
{
constexpr std::array<int64_t, 4> units{ 1, 10, 100, 1000 };
bool native_value(const std::array<int64_t, 4> &cash, int64_t *value) noexcept
{
	int64_t total = 0;
	for (size_t i = 0; i < cash.size(); ++i)
	{
		if (cash[i] < 0 || cash[i] > INT_MAX || cash[i] > (INT_MAX - total) / units[i])
			return false;
		total += cash[i] * units[i];
	}
	*value = total;
	return true;
}
void spend(std::array<int64_t, 4> &cash, int64_t amount) noexcept
{
	if (amount > cash[0])
	{
		amount -= cash[0];
		cash[0] = 0;
	}
	else
	{
		cash[0] -= amount;
		return;
	}
	for (size_t i = 1; i < cash.size() && amount > 0; ++i)
	{
		const int64_t available = cash[i] * units[i];
		if (amount >= available)
		{
			amount -= available;
			cash[i] = 0;
		}
		else
		{
			const int64_t removed = amount / units[i] + 1;
			cash[i] -= removed;
			amount -= removed * units[i];
		}
	}
	// Actual ADD_MONEY decomposes negative remainder from largest coin down.
	if (amount < 0)
	{
		int64_t change = -amount;
		for (size_t i = cash.size(); i-- > 0;)
		{
			cash[i] += change / units[i];
			change %= units[i];
		}
	}
}
}

native_quest_cost_projection_result
native_quest_cost_project(const std::array<int64_t, 4> &before, uint64_t revision,
			  std::span<const native_quest_cost_requirement> requirements,
			  native_quest_cost_projection *output) noexcept
{
	using result = native_quest_cost_projection_result;
	int64_t value = 0;
	if (!output || !revision || !native_value(before, &value))
		return result::invalid;
	try
	{
		native_quest_cost_projection candidate;
		candidate.before = candidate.after = before;
		candidate.before_revision = candidate.after_revision = revision;
		candidate.attempts.reserve(requirements.size());
		bool changed = false;
		for (size_t i = 0; i < requirements.size(); ++i)
		{
			if (i && requirements[i - 1].slot >= requirements[i].slot)
				return result::invalid;
			native_quest_cost_attempt attempt;
			attempt.requirement = requirements[i];
			attempt.before = candidate.after;
			if (requirements[i].copper <= 0)
				attempt.outcome = native_quest_cost_attempt_outcome::nonpositive;
			else if (requirements[i].copper > value)
				attempt.outcome = native_quest_cost_attempt_outcome::insufficient;
			else
			{
				spend(candidate.after, requirements[i].copper);
				attempt.outcome = native_quest_cost_attempt_outcome::charged;
				changed = true;
			}
			attempt.after = candidate.after;
			if (!native_value(candidate.after, &value))
				return result::overflow;
			candidate.attempts.push_back(std::move(attempt));
		}
		if (changed)
		{
			if (revision == UINT64_MAX)
				return result::overflow;
			++candidate.after_revision;
		}
		*output = std::move(candidate);
		return result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return result::allocation_failure;
	}
	catch (...)
	{
		return result::invalid;
	}
}

namespace
{
using cost_result = native_quest_cost_projection_result;
constexpr size_t max_attempts =
	(CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - NATIVE_QUEST_COST_HEADER_BYTES) /
	NATIVE_QUEST_COST_ATTEMPT_BYTES;
void put_cost(uint8_t *out, uint64_t value, size_t size) noexcept
{
	for (size_t i = 0; i < size; ++i)
		out[i] = static_cast<uint8_t>(value >> (8 * i));
}
uint64_t get_cost(const uint8_t *in, size_t size) noexcept
{
	uint64_t value = 0;
	for (size_t i = 0; i < size; ++i)
		value |= static_cast<uint64_t>(in[i]) << (8 * i);
	return value;
}
bool read_cost_cash(const uint8_t *in, std::array<int64_t, 4> &cash) noexcept
{
	for (size_t i = 0; i < cash.size(); ++i)
	{
		const uint64_t value = get_cost(in + i * 8, 8);
		if (value > INT_MAX)
			return false;
		cash[i] = static_cast<int64_t>(value);
	}
	return true;
}
void put_cost_cash(uint8_t *out, const std::array<int64_t, 4> &cash) noexcept
{
	for (size_t i = 0; i < cash.size(); ++i)
		put_cost(out + i * 8, static_cast<uint64_t>(cash[i]), 8);
}
cost_result verify_cost_projection(const native_quest_cost_projection &value)
{
	if (value.attempts.size() > max_attempts)
		return cost_result::overflow;
	std::vector<native_quest_cost_requirement> requirements;
	requirements.reserve(value.attempts.size());
	for (const auto &attempt : value.attempts)
		requirements.push_back(attempt.requirement);
	native_quest_cost_projection expected;
	auto status = native_quest_cost_project(value.before, value.before_revision, requirements,
						&expected);
	return status != cost_result::ok ? status :
	       expected == value	 ? cost_result::ok :
					   cost_result::invalid;
}
}

native_quest_cost_projection_result
native_quest_cost_projection_encode(const native_quest_cost_projection &value,
				    std::vector<uint8_t> *output) noexcept
{
	if (!output)
		return cost_result::invalid;
	try
	{
		auto status = verify_cost_projection(value);
		if (status != cost_result::ok)
			return status;
		std::vector<uint8_t> bytes(NATIVE_QUEST_COST_HEADER_BYTES +
					   value.attempts.size() * NATIVE_QUEST_COST_ATTEMPT_BYTES);
		bytes[0] = 'N';
		bytes[1] = 'Q';
		bytes[2] = 'C';
		bytes[3] = '1';
		put_cost(bytes.data() + 4, 1, 2);
		put_cost(bytes.data() + 8, value.attempts.size(), 4);
		put_cost(bytes.data() + 16, value.before_revision, 8);
		put_cost(bytes.data() + 24, value.after_revision, 8);
		put_cost_cash(bytes.data() + 32, value.before);
		put_cost_cash(bytes.data() + 64, value.after);
		for (size_t i = 0; i < value.attempts.size(); ++i)
		{
			auto *row = bytes.data() + NATIVE_QUEST_COST_HEADER_BYTES +
				    i * NATIVE_QUEST_COST_ATTEMPT_BYTES;
			const auto &attempt = value.attempts[i];
			put_cost(row, attempt.requirement.slot, 4);
			put_cost(row + 4, std::bit_cast<uint32_t>(attempt.requirement.copper), 4);
			row[8] = static_cast<uint8_t>(attempt.outcome);
			put_cost_cash(row + 16, attempt.before);
			put_cost_cash(row + 48, attempt.after);
		}
		*output = std::move(bytes);
		return cost_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return cost_result::allocation_failure;
	}
	catch (...)
	{
		return cost_result::invalid;
	}
}

native_quest_cost_projection_result
native_quest_cost_projection_decode(std::span<const uint8_t> bytes,
				    native_quest_cost_projection *output) noexcept
{
	if (!output || bytes.size() < NATIVE_QUEST_COST_HEADER_BYTES ||
	    bytes.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES || bytes[0] != 'N' ||
	    bytes[1] != 'Q' || bytes[2] != 'C' || bytes[3] != '1' ||
	    get_cost(bytes.data() + 4, 2) != 1 || get_cost(bytes.data() + 6, 2) ||
	    get_cost(bytes.data() + 12, 4))
		return cost_result::invalid;
	const auto count = get_cost(bytes.data() + 8, 4);
	if (count > max_attempts || bytes.size() != NATIVE_QUEST_COST_HEADER_BYTES +
							    count * NATIVE_QUEST_COST_ATTEMPT_BYTES)
		return cost_result::invalid;
	try
	{
		native_quest_cost_projection value;
		value.before_revision = get_cost(bytes.data() + 16, 8);
		value.after_revision = get_cost(bytes.data() + 24, 8);
		if (!read_cost_cash(bytes.data() + 32, value.before) ||
		    !read_cost_cash(bytes.data() + 64, value.after))
			return cost_result::invalid;
		value.attempts.reserve(static_cast<size_t>(count));
		for (size_t i = 0; i < count; ++i)
		{
			const auto *row = bytes.data() + NATIVE_QUEST_COST_HEADER_BYTES +
					  i * NATIVE_QUEST_COST_ATTEMPT_BYTES;
			if (std::any_of(row + 9, row + 16, [](uint8_t byte) { return byte != 0; }))
				return cost_result::invalid;
			native_quest_cost_attempt attempt;
			attempt.requirement.slot = static_cast<uint32_t>(get_cost(row, 4));
			attempt.requirement.copper =
				std::bit_cast<int32_t>(static_cast<uint32_t>(get_cost(row + 4, 4)));
			attempt.outcome = static_cast<native_quest_cost_attempt_outcome>(row[8]);
			if (!read_cost_cash(row + 16, attempt.before) ||
			    !read_cost_cash(row + 48, attempt.after))
				return cost_result::invalid;
			value.attempts.push_back(std::move(attempt));
		}
		auto status = verify_cost_projection(value);
		if (status != cost_result::ok)
			return status;
		*output = std::move(value);
		return cost_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return cost_result::allocation_failure;
	}
	catch (...)
	{
		return cost_result::invalid;
	}
}
namespace
{
using cost_bound_reserve_fn = bool (*)(size_t, void *) noexcept;
bool cost_bound_add(size_t &bytes, size_t extra) noexcept
{
	if (extra > SIZE_MAX - bytes)
		return false;
	bytes += extra;
	return true;
}
constexpr size_t cost_bound_allocator_frames =
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
constexpr size_t cost_bound_copy_frames =
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
constexpr size_t cost_bound_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t cost_bound_default_frames =
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
constexpr size_t cost_bound_vector_frames =
	cost_bound_allocator_frames + cost_bound_copy_frames + cost_bound_relocate_frames +
	cost_bound_default_frames +
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
constexpr size_t cost_bound_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + cost_bound_allocator_frames;

// Genuine default vector/base/impl/data and allocator/new_allocator this
// carriers. The vector object's fields are already included in its owner.
constexpr size_t cost_bound_empty_constructor_frames = 6 * sizeof(void *);
// vector(n,a), actual default allocator temporary, _S_check_init_len n/a/return
// and allocator copy, _Vector_base this/n/a, _Vector_impl this/a/copy,
// _Vector_impl_data this and _M_create_storage this/n. The existing vector
// allocator profile owns _S_max_size and allocation scopes.
constexpr size_t cost_bound_size_constructor_frames =
	2 * sizeof(void *) + sizeof(size_t) + sizeof(std::allocator<uint8_t>) + sizeof(void *) +
	2 * sizeof(size_t) + sizeof(std::allocator<uint8_t>) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) +
	sizeof(size_t);
// Explicit vector data/size queries, returned dynamic span, its genuine
// pointer/count constructor, extent constructor and to_address input/return.
constexpr size_t cost_bound_span_frames = sizeof(std::span<const native_quest_cost_requirement>) +
					  7 * sizeof(void *) + 3 * sizeof(size_t);
// Complete original native_value/spend/get/put/read/put_cash declared scopes;
// array/span/vector query references/results; bit_cast input reference/result.
constexpr size_t cost_bound_pure_frames =
	// native_value: cash/value references, total and index; result.
	2 * sizeof(void *) + sizeof(int64_t) + sizeof(size_t) + sizeof(bool) +
	// spend: cash reference, amount/available/removed/change, both indexes.
	sizeof(void *) + 4 * sizeof(int64_t) + 2 * sizeof(size_t) +
	// get_cost + put_cost: input/output, value, size/index, scalar return.
	2 * sizeof(void *) + 3 * sizeof(uint64_t) + 4 * sizeof(size_t) +
	// read_cost_cash + put_cost_cash: two references each, both indexes,
	// get result retained in read, bool return; bit_cast source/result.
	4 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(uint64_t) + sizeof(bool) +
	2 * sizeof(void *) + sizeof(uint32_t) + sizeof(int32_t) +
	12 * (sizeof(void *) + sizeof(size_t)) +
	sizeof(std::span<const native_quest_cost_requirement>);
// Whole original defaulted projection/attempt/requirement comparison closure;
// vector::== size/begin and equal/aux/aux1/equal<false>; normal iterator
// queries/returned iterators; the two array<int64_t,4> equality memcmp chains.
constexpr size_t cost_bound_compare_frames =
	3 * (2 * sizeof(void *) + sizeof(bool)) + 2 * sizeof(void *) +
	4 * (3 * sizeof(void *) + sizeof(bool)) + 10 * (2 * sizeof(void *)) +
	2 * (2 * sizeof(void *) + 3 * (2 * sizeof(void *)) +
	     4 * (3 * sizeof(void *) + sizeof(bool)) + 3 * (2 * sizeof(void *)) + sizeof(size_t) +
	     2 * (2 * sizeof(void *) + sizeof(size_t) + sizeof(int)));
// Actual any_of -> none_of -> find_if -> __find_if(random) chain, iterator
// parameters/returns, empty predicate/adapter carriers, trip_count, RA tag,
// __pred_iter/adapter construction/operator() and lambda byte/bool result.
constexpr size_t cost_bound_any_frames = 5 * (3 * sizeof(void *) + sizeof(bool)) +
					 3 * sizeof(void *) + sizeof(ptrdiff_t) + 6 * sizeof(bool) +
					 2 * sizeof(std::random_access_iterator_tag) +
					 4 * sizeof(void *) + sizeof(uint8_t) + 2 * sizeof(bool);
// Materialized uint32 argument bound to bit_cast<int32_t> const-reference.
// Its reference/returned int32 are Source; this separate object is INLINE.
constexpr size_t cost_bound_decode_bit_cast_temporary_inline = sizeof(uint32_t);
struct cost_bound_budget
{
	cost_bound_reserve_fn reserve;
	void *context;
	size_t outer;
	size_t frames;
	const native_quest_cost_projection *projection;
	const std::vector<native_quest_cost_requirement> *requirements;
	const std::vector<uint8_t> *bytes;
	template <typename T> bool heap(const std::vector<T> &value, size_t &total) const noexcept
	{
		return value.capacity() <= SIZE_MAX / sizeof(T) &&
		       cost_bound_add(total, value.capacity() * sizeof(T));
	}
	bool prefix(size_t &result, size_t extra) const noexcept
	{
		size_t current = outer;
		// Real census/prefix/heap/add call parameters/results and locals.
		constexpr size_t observation =
			12 * sizeof(void *) + 7 * sizeof(size_t) + 5 * sizeof(bool);
		if (!cost_bound_add(current, sizeof(*this)) || !cost_bound_add(current, frames) ||
		    (projection && !heap(projection->attempts, current)) ||
		    (requirements && !heap(*requirements, current)) ||
		    (bytes && !heap(*bytes, current)) || !cost_bound_add(current, observation) ||
		    !cost_bound_add(current, extra))
			return false;
		result = current;
		return true;
	}
	bool peak(size_t extra) const noexcept
	{
		size_t current = 0;
		return prefix(current, extra) && reserve && reserve(current, context);
	}
	template <typename T> bool request(size_t count, size_t source_frames) const noexcept
	{
		// Actual request's this/count/source_frames/request, returned bool;
		// source_frames prospectively owns STL even for fitting operations.
		size_t request = source_frames;
		constexpr size_t own = sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool);
		return count <= SIZE_MAX / sizeof(T) &&
		       cost_bound_add(request, count * sizeof(T)) && cost_bound_add(request, own) &&
		       peak(request);
	}
};
bool cost_bound_initial(cost_bound_reserve_fn reserve, void *context, size_t outer,
			size_t frames) noexcept
{
	constexpr size_t own = 2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);
	return cost_bound_add(outer, sizeof(cost_bound_budget)) && cost_bound_add(outer, frames) &&
	       cost_bound_add(outer, own) && reserve && reserve(outer, context);
}
} // namespace

native_quest_cost_projection_result
native_quest_cost_project_bounded(const std::array<int64_t, 4> &before, uint64_t revision,
				  std::span<const native_quest_cost_requirement> requirements,
				  native_quest_cost_projection *output,
				  bool (*reserve)(size_t, void *) noexcept, void *context,
				  size_t outer) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)before;
	(void)revision;
	(void)requirements;
	(void)output;
	(void)reserve;
	(void)context;
	(void)outer;
	return native_quest_cost_projection_result::invalid;
#else
	using result = native_quest_cost_projection_result;
	int64_t value = 0;
	if (!output || !revision || !native_value(before, &value))
		return result::invalid;
	try
	{
		constexpr size_t frames = sizeof(native_quest_cost_projection) +
					  sizeof(native_quest_cost_attempt) + 4 * sizeof(void *) +
					  sizeof(std::span<const native_quest_cost_requirement>) +
					  sizeof(uint64_t) + 3 * sizeof(size_t) + sizeof(int64_t) +
					  sizeof(bool) + sizeof(result) + cost_bound_pure_frames +
					  cost_bound_empty_constructor_frames;
		if (!cost_bound_initial(reserve, context, outer, frames))
			return result::allocation_failure;
		native_quest_cost_projection candidate;
		cost_bound_budget owner{ reserve,    context, outer,  frames,
					 &candidate, nullptr, nullptr };
		candidate.before = candidate.after = before;
		candidate.before_revision = candidate.after_revision = revision;
		if (requirements.size() > candidate.attempts.max_size())
			return result::invalid;
		if (!owner.request<native_quest_cost_attempt>(requirements.size(),
							      cost_bound_vector_frames))
			return result::allocation_failure;
		candidate.attempts.reserve(requirements.size());
		bool changed = false;
		for (size_t i = 0; i < requirements.size(); ++i)
		{
			if (i && requirements[i - 1].slot >= requirements[i].slot)
				return result::invalid;
			native_quest_cost_attempt attempt;
			attempt.requirement = requirements[i];
			attempt.before = candidate.after;
			if (requirements[i].copper <= 0)
				attempt.outcome = native_quest_cost_attempt_outcome::nonpositive;
			else if (requirements[i].copper > value)
				attempt.outcome = native_quest_cost_attempt_outcome::insufficient;
			else
			{
				spend(candidate.after, requirements[i].copper);
				attempt.outcome = native_quest_cost_attempt_outcome::charged;
				changed = true;
			}
			attempt.after = candidate.after;
			if (!native_value(candidate.after, &value))
				return result::overflow;
			if (!owner.peak(cost_bound_vector_frames))
				return result::allocation_failure;
			candidate.attempts.push_back(std::move(attempt));
		}
		if (changed)
		{
			if (revision == UINT64_MAX)
				return result::overflow;
			++candidate.after_revision;
		}
		if (!owner.peak(cost_bound_move_frames))
			return result::allocation_failure;
		*output = std::move(candidate);
		return result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return result::allocation_failure;
	}
	catch (...)
	{
		return result::invalid;
	}

#endif
}

namespace
{
cost_result cost_bound_verify(const native_quest_cost_projection &value,
			      bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer)
{
	if (value.attempts.size() > max_attempts)
		return cost_result::overflow;
	constexpr size_t frames = sizeof(std::vector<native_quest_cost_requirement>) +
				  sizeof(native_quest_cost_projection) + 4 * sizeof(void *) +
				  3 * sizeof(size_t) + sizeof(cost_result) +
				  cost_bound_compare_frames +
				  2 * cost_bound_empty_constructor_frames + cost_bound_span_frames;
	if (!cost_bound_initial(reserve, context, outer, frames))
		return cost_result::allocation_failure;
	std::vector<native_quest_cost_requirement> requirements;
	cost_bound_budget owner{ reserve, context, outer, frames, nullptr, &requirements, nullptr };
	if (!owner.request<native_quest_cost_requirement>(value.attempts.size(),
							  cost_bound_vector_frames))
		return cost_result::allocation_failure;
	requirements.reserve(value.attempts.size());
	for (const auto &attempt : value.attempts)
	{
		if (!owner.peak(cost_bound_vector_frames))
			return cost_result::allocation_failure;
		requirements.push_back(attempt.requirement);
	}
	native_quest_cost_projection expected;
	owner.projection = &expected;
	size_t nested = 0;
	if (!owner.prefix(nested, 0))
		return cost_result::allocation_failure;
	auto status =
		native_quest_cost_project_bounded(value.before, value.before_revision,
						  std::span<const native_quest_cost_requirement>(
							  requirements.data(), requirements.size()),
						  &expected, reserve, context, nested);
	if (status == cost_result::ok && !owner.peak(cost_bound_compare_frames))
		return cost_result::allocation_failure;
	return status != cost_result::ok ? status :
	       expected == value	 ? cost_result::ok :
					   cost_result::invalid;
}
}

native_quest_cost_projection_result native_quest_cost_projection_encode_bounded(
	const native_quest_cost_projection &value, std::vector<uint8_t> *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)value;
	(void)output;
	(void)reserve;
	(void)context;
	(void)outer;
	return native_quest_cost_projection_result::invalid;
#else
	if (!output)
		return cost_result::invalid;
	try
	{
		constexpr size_t frames = sizeof(std::vector<uint8_t>) + 5 * sizeof(void *) +
					  4 * sizeof(size_t) + sizeof(cost_result) +
					  cost_bound_pure_frames;
		if (!cost_bound_initial(reserve, context, outer, frames))
			return cost_result::allocation_failure;
		cost_bound_budget owner{
			reserve, context, outer, frames, nullptr, nullptr, nullptr
		};
		size_t nested = 0;
		if (!owner.prefix(nested, 0))
			return cost_result::allocation_failure;
		auto status = cost_bound_verify(value, reserve, context, nested);
		if (status != cost_result::ok)
			return status;
		const size_t encoded_size = NATIVE_QUEST_COST_HEADER_BYTES +
					    value.attempts.size() * NATIVE_QUEST_COST_ATTEMPT_BYTES;
		if (!owner.request<uint8_t>(encoded_size,
					    cost_bound_vector_frames +
						    cost_bound_size_constructor_frames))
			return cost_result::allocation_failure;
		std::vector<uint8_t> bytes(encoded_size);
		owner.bytes = &bytes;
		bytes[0] = 'N';
		bytes[1] = 'Q';
		bytes[2] = 'C';
		bytes[3] = '1';
		put_cost(bytes.data() + 4, 1, 2);
		put_cost(bytes.data() + 8, value.attempts.size(), 4);
		put_cost(bytes.data() + 16, value.before_revision, 8);
		put_cost(bytes.data() + 24, value.after_revision, 8);
		put_cost_cash(bytes.data() + 32, value.before);
		put_cost_cash(bytes.data() + 64, value.after);
		for (size_t i = 0; i < value.attempts.size(); ++i)
		{
			auto *row = bytes.data() + NATIVE_QUEST_COST_HEADER_BYTES +
				    i * NATIVE_QUEST_COST_ATTEMPT_BYTES;
			const auto &attempt = value.attempts[i];
			put_cost(row, attempt.requirement.slot, 4);
			put_cost(row + 4, std::bit_cast<uint32_t>(attempt.requirement.copper), 4);
			row[8] = static_cast<uint8_t>(attempt.outcome);
			put_cost_cash(row + 16, attempt.before);
			put_cost_cash(row + 48, attempt.after);
		}
		if (!owner.peak(cost_bound_move_frames))
			return cost_result::allocation_failure;
		*output = std::move(bytes);
		return cost_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return cost_result::allocation_failure;
	}
	catch (...)
	{
		return cost_result::invalid;
	}

#endif
}

native_quest_cost_projection_result native_quest_cost_projection_decode_bounded(
	std::span<const uint8_t> bytes, native_quest_cost_projection *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)bytes;
	(void)output;
	(void)reserve;
	(void)context;
	(void)outer;
	return native_quest_cost_projection_result::invalid;
#else
	if (!output || bytes.size() < NATIVE_QUEST_COST_HEADER_BYTES ||
	    bytes.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES || bytes[0] != 'N' ||
	    bytes[1] != 'Q' || bytes[2] != 'C' || bytes[3] != '1' ||
	    get_cost(bytes.data() + 4, 2) != 1 || get_cost(bytes.data() + 6, 2) ||
	    get_cost(bytes.data() + 12, 4))
		return cost_result::invalid;
	const auto count = get_cost(bytes.data() + 8, 4);
	if (count > max_attempts || bytes.size() != NATIVE_QUEST_COST_HEADER_BYTES +
							    count * NATIVE_QUEST_COST_ATTEMPT_BYTES)
		return cost_result::invalid;
	try
	{
		constexpr size_t frames =
			cost_bound_decode_bit_cast_temporary_inline +
			sizeof(native_quest_cost_projection) + sizeof(native_quest_cost_attempt) +
			sizeof(std::span<const uint8_t>) + 5 * sizeof(void *) + 5 * sizeof(size_t) +
			sizeof(uint64_t) + sizeof(cost_result) + cost_bound_pure_frames +
			cost_bound_any_frames + cost_bound_empty_constructor_frames;
		if (!cost_bound_initial(reserve, context, outer, frames))
			return cost_result::allocation_failure;
		native_quest_cost_projection value;
		cost_bound_budget owner{ reserve, context, outer, frames, &value, nullptr, nullptr };
		value.before_revision = get_cost(bytes.data() + 16, 8);
		value.after_revision = get_cost(bytes.data() + 24, 8);
		if (!read_cost_cash(bytes.data() + 32, value.before) ||
		    !read_cost_cash(bytes.data() + 64, value.after))
			return cost_result::invalid;
		if (!owner.request<native_quest_cost_attempt>(static_cast<size_t>(count),
							      cost_bound_vector_frames))
			return cost_result::allocation_failure;
		value.attempts.reserve(static_cast<size_t>(count));
		for (size_t i = 0; i < count; ++i)
		{
			const auto *row = bytes.data() + NATIVE_QUEST_COST_HEADER_BYTES +
					  i * NATIVE_QUEST_COST_ATTEMPT_BYTES;
			if (std::any_of(row + 9, row + 16, [](uint8_t byte) { return byte != 0; }))
				return cost_result::invalid;
			native_quest_cost_attempt attempt;
			attempt.requirement.slot = static_cast<uint32_t>(get_cost(row, 4));
			attempt.requirement.copper =
				std::bit_cast<int32_t>(static_cast<uint32_t>(get_cost(row + 4, 4)));
			attempt.outcome = static_cast<native_quest_cost_attempt_outcome>(row[8]);
			if (!read_cost_cash(row + 16, attempt.before) ||
			    !read_cost_cash(row + 48, attempt.after))
				return cost_result::invalid;
			if (!owner.peak(cost_bound_vector_frames))
				return cost_result::allocation_failure;
			value.attempts.push_back(std::move(attempt));
		}
		size_t nested = 0;
		if (!owner.prefix(nested, 0))
			return cost_result::allocation_failure;
		auto status = cost_bound_verify(value, reserve, context, nested);
		if (status != cost_result::ok)
			return status;
		if (!owner.peak(cost_bound_move_frames))
			return cost_result::allocation_failure;
		*output = std::move(value);
		return cost_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return cost_result::allocation_failure;
	}
	catch (...)
	{
		return cost_result::invalid;
	}

#endif
}

// Actual original project/encode/decode call cost_bound_initial before creating
// their private projection/vector/controller owners. The full SOURCE query
// separately admits pre-callback scalar/call scopes; initial owner inline is
// therefore zero, not the mixed original frames allowance or a cached baseline.
bool native_quest_cost_project_initial_inline_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	if (!output)
		return false;
	*output = 0;
	return true;
#else
	(void)output;
	return false;
#endif
}
bool native_quest_cost_projection_encode_initial_inline_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	if (!output)
		return false;
	*output = 0;
	return true;
#else
	(void)output;
	return false;
#endif
}
bool native_quest_cost_projection_decode_initial_inline_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	if (!output)
		return false;
	*output = 0;
	return true;
#else
	(void)output;
	return false;
#endif
}

// Complete typed native-cost Source derivation. Every term below names an
// actual selected declaration/control; private owners remain INLINE.
// Profiles are conservative source unions, not optimizer stack estimates.
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
namespace
{
using cost_source_padding_predicate = decltype([](uint8_t byte) { return byte != 0; });
using cost_source_padding_adapter = __gnu_cxx::__ops::_Iter_pred<cost_source_padding_predicate>;
static_assert(sizeof(cost_source_padding_predicate) == 1);
static_assert(sizeof(cost_source_padding_adapter) == 1);
static_assert(!std::is_trivial_v<native_quest_cost_requirement>);
static_assert(!std::is_trivial_v<native_quest_cost_attempt>);
static_assert(std::is_trivially_copyable_v<native_quest_cost_requirement>);
static_assert(std::is_trivially_copyable_v<native_quest_cost_attempt>);
constexpr size_t cost_complete_native_value =
	// cash/value, total, i, result; before/src/economy/native_quest_cost.c:12 native_value.
	(2 * sizeof(void *) + sizeof(int64_t) + sizeof(size_t) + sizeof(bool));
constexpr size_t cost_complete_spend =
	// cash, amount/available/removed/change, two i; before/src/economy/native_quest_cost.c:24 spend.
	(sizeof(void *) + 4 * sizeof(int64_t) + 2 * sizeof(size_t));
constexpr size_t cost_complete_get =
	// in, size/i, value and returned uint64; before/src/economy/native_quest_cost.c:132 get_cost.
	(sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(uint64_t));
constexpr size_t cost_complete_put =
	// out, value, size/i; before/src/economy/native_quest_cost.c:127 put_cost.
	(sizeof(void *) + sizeof(uint64_t) + 2 * sizeof(size_t));
constexpr size_t cost_complete_read_cash =
	// in/cash, i, value, result; before/src/economy/native_quest_cost.c:139 read_cost_cash.
	(2 * sizeof(void *) + sizeof(size_t) + sizeof(uint64_t) + sizeof(bool));
constexpr size_t cost_complete_put_cash =
	// out/cash, i; before/src/economy/native_quest_cost.c:150 put_cost_cash.
	(2 * sizeof(void *) + sizeof(size_t));
constexpr size_t cost_complete_array_access =
	// const cash/unit subscript this/index/reference; array:208-212 direct _M_elems.
	(2 * sizeof(void *) + sizeof(size_t)) +
	// mutable cash subscript this/index/reference; array:200-204 direct _M_elems.
	(2 * sizeof(void *) + sizeof(size_t)) +
	// cash size this/result; array:190-192.
	(sizeof(void *) + sizeof(size_t));
constexpr size_t cost_complete_requirement_span =
	// size this/result and dynamic extent this/result; span:248-250,95-97.
	(2 * (sizeof(void *) + sizeof(size_t))) +
	// subscript this/index/reference; span:284-288.
	(2 * sizeof(void *) + sizeof(size_t)) +
	// explicit span destructor receiver; external lvalue argument copy caller-owned; span:244 explicitly defaulted destructor; no dynamic-extent trivial dtor.
	(sizeof(void *));
constexpr size_t cost_complete_byte_span =
	// size this/result and dynamic extent this/result; span:248-250,95-97.
	(2 * (sizeof(void *) + sizeof(size_t))) +
	// subscript this/index/reference; span:284-288.
	(2 * sizeof(void *) + sizeof(size_t)) +
	// data this/pointer result; span:292-294.
	(2 * sizeof(void *)) +
	// explicit span destructor receiver; external lvalue argument copy caller-owned; span:244 explicitly defaulted destructor; no dynamic-extent trivial dtor.
	(sizeof(void *));
constexpr size_t cost_complete_span_construction =
	// span pointer/count constructor this/first/count; span:155-161.
	(2 * sizeof(void *) + sizeof(size_t)) +
	// dynamic extent ctor this/count; span:91-93.
	(sizeof(void *) + sizeof(size_t)) +
	// raw to_address and __to_address each pointer/result; bits/ptr_traits.h:250-251,212-216.
	(4 * sizeof(void *)) +
	// requirements vector data this/result, _M_data_ptr this/ptr/result; stl_vector.h:1255-1261,1988-1991.
	(5 * sizeof(void *)) +
	// requirements vector size this/result; stl_vector.h:989-991.
	(sizeof(void *) + sizeof(size_t));
constexpr size_t cost_complete_initial =
	// reserve/context, outer/frames/own, returned bool; before.c cost_bound_initial.
	(2 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool));
constexpr size_t cost_complete_request =
	// this, count/source_frames/request/own, returned bool; before.c cost_bound_budget::request.
	(sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool));
constexpr size_t cost_complete_observation =
	// add totalref/extra/result; before.c cost_bound_add.
	(sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	// prefix this/resultref, extra/current/observation, bool; before.c cost_bound_budget::prefix.
	(2 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool)) +
	// peak this, extra/current, result; before.c cost_bound_budget::peak.
	(sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) +
	// reserve callback prototype bytes/context/result only; before.c cost_bound_reserve_fn; callback body caller-owned.
	(sizeof(void *) + sizeof(size_t) + sizeof(bool));
constexpr size_t cost_complete_heap =
	// heap<T> this/vectorref/totalref/result; before.c cost_bound_budget::heap.
	(3 * sizeof(void *) + sizeof(bool)) +
	// capacity<T> this/size result; stl_vector.h:1073-1075.
	(sizeof(void *) + sizeof(size_t));
constexpr size_t cost_complete_project_own =
	// before/output/reserve/context, requirements span/revision, outer/value/frames/i/changed/catchref/result; before.c native_quest_cost_project_bounded; owners excluded.
	(5 * sizeof(void *) + sizeof(std::span<const native_quest_cost_requirement>) +
	 sizeof(uint64_t) + 3 * sizeof(size_t) + sizeof(int64_t) + sizeof(bool) +
	 sizeof(native_quest_cost_projection_result));
constexpr size_t cost_complete_verify_own =
	// value/reserve/context, outer/frames/nested, hidden range/begin/end/attemptref, status/result; before.c cost_bound_verify; owners excluded.
	(5 * sizeof(void *) + 3 * sizeof(size_t) +
	 2 * sizeof(std::vector<native_quest_cost_attempt>::const_iterator) +
	 2 * sizeof(native_quest_cost_projection_result));
constexpr size_t cost_complete_encode_own =
	// value/output/reserve/context/row/attemptref/catchref, outer/frames/nested/encoded_size/i, status/result; before.c encode_bounded; owners excluded.
	(7 * sizeof(void *) + 5 * sizeof(size_t) + 2 * sizeof(native_quest_cost_projection_result));
constexpr size_t cost_complete_decode_own =
	// bytes/output/reserve/context/row/catchref, outer/frames/i/nested, count, status/result; before.c decode_bounded; owners excluded.
	(sizeof(std::span<const uint8_t>) + 5 * sizeof(void *) + 4 * sizeof(size_t) +
	 sizeof(uint64_t) + 2 * sizeof(native_quest_cost_projection_result));
constexpr size_t cost_complete_allocator_access =
	// mutable and const allocator accessor this/reference result; stl_vector.h:298-309.
	(4 * sizeof(void *));
constexpr size_t cost_complete_max_size_wrapper =
	// max_size this/result; stl_vector.h:999.
	(sizeof(void *) + sizeof(size_t));
constexpr size_t cost_complete_max_size_leaf =
	// _S_max_size allocatorref/diffmax/allocmax/result; stl_vector.h:1916-1924.
	(sizeof(void *) + 3 * sizeof(size_t)) +
	// allocator_traits max_size allocatorref/result C++20; alloc_traits.h:571-577.
	(sizeof(void *) + sizeof(size_t)) +
	// min two const refs and returned const ref; stl_algobase.h:230-238.
	(3 * sizeof(void *));
constexpr size_t cost_complete_allocate =
	// _M_allocate this/n/result; traits allocate allocator/n/result; allocator allocate this/n/result; stl_vector.h:378-382; alloc_traits.h:481-482; allocator.h:181-191.
	(3 * (2 * sizeof(void *) + sizeof(size_t))) +
	// new_allocator allocate this/n/hint/result; new_allocator.h:120-153.
	(3 * sizeof(void *) + sizeof(size_t)) +
	// new_allocator _M_max_size this/result; new_allocator.h:_M_max_size.
	(sizeof(void *) + sizeof(size_t)) +
	// ordinary operator new size/result prototype; new header ordinary operator new; external body not claimed.
	(sizeof(void *) + sizeof(size_t)) +
	// allocator<T>::allocate own actual __is_constant_evaluated returned bool; allocator.h:183; c++config.h:540-550.
	(sizeof(bool));
constexpr size_t cost_complete_deallocate =
	// base/traits/allocator/new_allocator each this-or-allocator/p/n; stl_vector.h:386; alloc_traits.h:495; allocator.h:194; new_allocator.h:158.
	(4 * (2 * sizeof(void *) + sizeof(size_t))) +
	// ordinary sized operator delete p/n prototype; new header sized operator delete; external body not claimed.
	(sizeof(void *) + sizeof(size_t)) +
	// allocator<T>::deallocate own actual __is_constant_evaluated returned bool; allocator.h:196; c++config.h:540-550.
	(sizeof(bool));
constexpr size_t cost_complete_vector_default =
	// vector/base/impl/data/allocator/new_allocator default this receivers; stl_vector.h:526,315,137,99; allocator.h:163; new_allocator.h:80.
	(6 * sizeof(void *));
constexpr size_t cost_complete_vector_cleanup =
	// vector/base/impl/allocator selected nontrivial destructor receivers; stl_vector.h:733,367; generated nontrivial impl; allocator.h:176; no data or C++20 new_allocator dtor body.
	(4 * sizeof(void *)) +
	// allocator _Destroy first/last/allocator; plain _Destroy first/last; trivial __destroy first/last; alloc_traits.h:945-948; stl_construct.h:182-197,170-173.
	(7 * sizeof(void *)) +
	// allocator accessor this/ref; stl_vector.h:298-309.
	(2 * sizeof(void *)) +
	// _Destroy<T*> own actual __is_constant_evaluated returned bool; stl_construct.h:193; c++config.h:540-550.
	(sizeof(bool));
constexpr size_t cost_complete_reserve_empty =
	// reserve this/n/old_size/tmp; vector.tcc:68-100.
	(2 * sizeof(void *) + 2 * sizeof(size_t)) +
	// size and capacity each this/result; stl_vector.h:989-991,1073-1075.
	(2 * (sizeof(void *) + sizeof(size_t))) +
	// allocator accessor this/ref; stl_vector.h:298-303.
	(2 * sizeof(void *)) +
	// _S_relocate and __relocate_a each first/last/result/allocator/returned ptr; stl_vector.h:504; stl_uninitialized.h:1140.
	(10 * sizeof(void *)) +
	// three raw __niter_base input/returned pointer calls; stl_uninitialized.h:1147-1149; stl_algobase.h:316.
	(6 * sizeof(void *)) +
	// generic __relocate_a_1 first/last/result/allocator/cur/returned pointer, empty loop; stl_uninitialized.h:1092-1108; requirement and attempt is_trivial false.
	(6 * sizeof(void *)) +
	// base _M_deallocate this/oldnull/n; trait body not entered on empty old vector; vector.tcc:93; stl_vector.h:386-391.
	(2 * sizeof(void *) + sizeof(size_t));
constexpr size_t cost_complete_construct_one =
	// traits construct allocator/location/argument refs; alloc_traits.h:534-545.
	(3 * sizeof(void *)) +
	// construct_at location/argument/returned pointer; stl_construct.h:94-98.
	(3 * sizeof(void *)) +
	// forward argument ref/returned ref; move.h:77-94.
	(2 * sizeof(void *)) +
	// placement new size/location/result prototype; new header placement new.
	(2 * sizeof(void *) + sizeof(size_t));
constexpr size_t cost_complete_push_requirement_fits =
	// lvalue push_back this/argument ref; stl_vector.h:1281-1291.
	(2 * sizeof(void *)) +
	// requirement generated copy ctor this/source ref; native_quest_cost.h requirement generated.
	(2 * sizeof(void *));
constexpr size_t cost_complete_push_attempt_fits =
	// rvalue push_back this/arg and emplace this/arg/returned ref; stl_vector.h:1298; vector.tcc:112-128.
	(5 * sizeof(void *)) +
	// move argument/returned ref; move.h:105.
	(2 * sizeof(void *)) +
	// back this/returned ref and end this/returned normal iterator; stl_vector.h:1234,889.
	(4 * sizeof(void *)) +
	// normal iterator pointer-ref ctor this/source ref; dereference this/returned ref; stl_iterator.h:1075,1099.
	(4 * sizeof(void *)) +
	// normal iterator operator-minus this/difference/returned iterator, materialized underlying pointer arg; stl_iterator.h:1159-1162; same ctor already named.
	(3 * sizeof(void *) + sizeof(std::ptrdiff_t)) +
	// attempt/requirement/two cash-array generated move constructor this/source refs; native_quest_cost.h generated move + array generated move.
	(8 * sizeof(void *));
constexpr size_t cost_complete_generated_common =
	// projection default/nontrivial dtor, attempt/requirement nontrivial default ctor receivers; native_quest_cost.h generated selected bodies; no attempt/requirement trivial dtor.
	(4 * sizeof(void *));
constexpr size_t cost_complete_project_copy_assignments =
	// requirement copy assignment this/source/returned ref; native_quest_cost.h generated.
	(3 * sizeof(void *)) +
	// cash array copy assignment this/source/returned ref; array generated; same member scope used for both cash arrays.
	(3 * sizeof(void *));
constexpr size_t cost_complete_projection_move_assignments =
	// projection move assignment this/source/returned ref; cash-array move assignment this/source/returned ref; native_quest_cost.h generated + array generated.
	(6 * sizeof(void *));
constexpr size_t cost_complete_projection_move =
	// std::move<native_quest_cost_projection> actual argument/ref result; move.h:105; distinct from vector/attempt/allocator move instantiations.
	(2 * sizeof(void *));
constexpr size_t cost_complete_vector_move =
	// ext allocator traits _S_propagate_on_move_assign actual returned bool; distinct from local __move_storage; ext/alloc_traits.h:109-110 selected by stl_vector.h:769; true propagation short-circuits _S_always_equal.
	(sizeof(bool)) +
	// operator= this/source/returned ref and move_storage constexpr; stl_vector.h:766-773.
	(3 * sizeof(void *) + sizeof(bool)) +
	// _M_move_assign this/source and true_type parameter; stl_vector.h:1959-1965.
	(2 * sizeof(void *) + sizeof(std::true_type)) +
	// move vector argument/returned ref; move.h:105.
	(2 * sizeof(void *)) +
	// get_allocator this/allocator result; stl_vector.h:310-313.
	(sizeof(void *) + sizeof(std::allocator<uint8_t>)) +
	// allocator accessor this/ref; stl_vector.h:298-309.
	(2 * sizeof(void *)) +
	// vector(allocator), base(allocator), impl(allocator) each this/allocator ref; data default this; stl_vector.h:537,321,146,99.
	(7 * sizeof(void *)) +
	// allocator and new_allocator copy ctor this/source; allocator.h:166-168; new_allocator.h:83.
	(4 * sizeof(void *)) +
	// swap_data this/argument, copy_data this/argument, data explicit default receiver; stl_vector.h:122-130,113-119,99; no trivial data destructor body.
	(5 * sizeof(void *)) +
	// alloc_on_move two refs; allocator and new_allocator assignment each this/source/returned ref; move ref/result; alloc_traits.h:747-754; allocator.h:171; new_allocator.h:100; move.h:105.
	(10 * sizeof(void *));
constexpr size_t cost_complete_byte_size_ctor =
	// default allocator and new_allocator constructor receivers; stl_vector.h:552 default allocator_type(); allocator.h:163; new_allocator.h:87.
	(2 * sizeof(void *)) +
	// vector n/a ctor this/n/a; check_init n/a/result; stl_vector.h:552-558,1907-1913.
	(3 * sizeof(void *) + 3 * sizeof(size_t)) +
	// base n/a ctor this/n/a; impl a ctor this/a; data default receiver; storage this/n; stl_vector.h:333-335,146,99,396-401.
	(6 * sizeof(void *) + 2 * sizeof(size_t)) +
	// allocator and new_allocator copy ctor this/source; allocator.h:166; new_allocator.h:83.
	(4 * sizeof(void *)) +
	// allocator temporary selected destructor receiver; allocator.h:176; C++20 new_allocator has no declared destructor body.
	(sizeof(void *)) +
	// default_initialize this/n and allocator getter this/ref; stl_vector.h:1715-1721.
	(3 * sizeof(void *) + sizeof(size_t)) +
	// default_n_a first/n/allocatorref/result; stl_uninitialized.h:778-780.
	(3 * sizeof(void *) + sizeof(size_t)) +
	// default_n first/n/can_fill/result; stl_uninitialized.h:697-713.
	(2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	// default_n_1 true first/n/val/result; stl_uninitialized.h:652-665.
	(3 * sizeof(void *) + sizeof(size_t)) +
	// addressof inputref/result; move.h __addressof.
	(2 * sizeof(void *)) +
	// _Construct location and placement new size/location/result; stl_construct.h:109-118; new placement.
	(3 * sizeof(void *) + sizeof(size_t)) +
	// fill_n first/n/value-ref/result; stl_algobase.h:1141-1158.
	(3 * sizeof(void *) + sizeof(size_t)) +
	// size_to_integer input/result; stl_algobase.h:1015-1020 selected unsigned long.
	(2 * sizeof(size_t)) +
	// iterator_category iteratorref/returned RA tag; stl_iterator_base_types.h:239.
	(sizeof(void *) + sizeof(std::random_access_iterator_tag)) +
	// fill_n_a first/n/value-ref/tag/result; stl_algobase.h:1111-1126.
	(3 * sizeof(void *) + sizeof(size_t) + sizeof(std::random_access_iterator_tag)) +
	// fill_a first/last/value-ref; byte fill_a1 first/last/value-ref/tmp/len; stl_algobase.h:968;945-956 byte memset selected.
	(6 * sizeof(void *) + sizeof(uint8_t) + sizeof(size_t)) +
	// memset dst/int/len/result prototype; builtin memset actual arguments/results; host body not claimed.
	(2 * sizeof(void *) + sizeof(int) + sizeof(size_t)) +
	// _Construct<byte> own actual __is_constant_evaluated returned bool; stl_construct.h:111; c++config.h:540-550.
	(sizeof(bool)) +
	// __uninitialized_default_n<byte*,size_t> own actual is_constant_evaluated returned bool; stl_uninitialized.h:702; type_traits:3649-3655.
	(sizeof(bool)) +
	// __fill_a1<byte> own actual is_constant_evaluated returned bool; stl_algobase.h:950; type_traits:3649-3655.
	(sizeof(bool));
constexpr size_t cost_complete_byte_queries =
	// mutable data this/pointer result and _M_data_ptr this/ptr/result; stl_vector.h:1255-1261,1988.
	(5 * sizeof(void *)) +
	// mutable subscript this/index/ref; stl_vector.h:1126-1132.
	(2 * sizeof(void *) + sizeof(size_t));
constexpr size_t cost_complete_attempt_queries =
	// const size this/result; const subscript this/index/ref; stl_vector.h:989,1146-1152.
	(3 * sizeof(void *) + 2 * sizeof(size_t)) +
	// const begin/end each this/returned normal iterator; stl_vector.h:879,899.
	(2 * (sizeof(void *) + sizeof(std::vector<native_quest_cost_attempt>::const_iterator))) +
	// const begin/end each materialized const attempt pointer bound to iterator const-reference ctor; stl_vector.h:879,899 T* to const T*; stl_iterator.h:1075.
	(2 * sizeof(void *)) +
	// normal iterator pointer-ref ctor this/source ref, dereference this/result, increment this/result, equal lhs/rhs/result, base this/result; stl_iterator.h:1075,1099,1108,1191,1167.
	(10 * sizeof(void *) + sizeof(bool));
constexpr size_t cost_complete_comparison =
	// defaulted projection/attempt/requirement equality each this/other-ref/bool; native_quest_cost.h defaulted equality.
	(3 * (2 * sizeof(void *) + sizeof(bool))) +
	// cash-array equality refs/bool; array:297.
	(2 * sizeof(void *) + sizeof(bool)) +
	// cash-array begin/end/begin and their actual data calls, receivers/returns; array:128-145,276-282.
	(12 * sizeof(void *)) +
	// vector<attempt> equality two refs/bool; stl_vector.h:2043-2046.
	(2 * sizeof(void *) + sizeof(bool)) +
	// attempt equal/aux/aux1/false each first/last/first2/bool; aux1 simple bool; stl_algobase.h:1546,1225,1210,1161.
	(4 * (3 * sizeof(void *) + sizeof(bool)) + sizeof(bool)) +
	// three normal __niter_base by-value iterator/pointer result and their base this/ref-result; stl_iterator.h:1357-1360; stl_algobase.h:1227-1229.
	(3 *
	 (sizeof(std::vector<native_quest_cost_attempt>::const_iterator) + 3 * sizeof(void *))) +
	// cash equal/aux/aux1/true each three ptrs/result, simple bool and len; stl_algobase.h:1546,1225,1210,1178.
	(4 * (3 * sizeof(void *) + sizeof(bool)) + sizeof(bool) + sizeof(size_t)) +
	// three cash raw __niter_base input/return; stl_algobase.h:316.
	(6 * sizeof(void *)) +
	// cash __memcmp pointers/count/int result and builtin memcmp pointers/count/int result; stl_algobase.h:93-111.
	(2 * (2 * sizeof(void *) + sizeof(size_t) + sizeof(int))) +
	// __memcmp<cash> own actual is_constant_evaluated returned bool; stl_algobase.h:100; type_traits:3649-3655.
	(sizeof(bool));
constexpr size_t cost_complete_any_padding =
	// any_of and none_of first/last/predicate/bool; stl_algo.h:495,476.
	(2 * (2 * sizeof(void *) + sizeof(cost_source_padding_predicate) + sizeof(bool))) +
	// find_if first/last/predicate/returned pointer; stl_algo.h:3914-3925.
	(3 * sizeof(void *) + sizeof(cost_source_padding_predicate)) +
	// three-arg find_if first/last/adapter/returned pointer; stl_algobase.h:2115.
	(3 * sizeof(void *) + sizeof(cost_source_padding_adapter)) +
	// RA find_if first/last/adapter/tag/trip_count/returned pointer; stl_algobase.h:2064-2110.
	(3 * sizeof(void *) + sizeof(cost_source_padding_adapter) +
	 sizeof(std::random_access_iterator_tag) + sizeof(std::ptrdiff_t)) +
	// iterator_category iterator-ref/returned RA; stl_iterator_base_types.h:239.
	(sizeof(void *) + sizeof(std::random_access_iterator_tag)) +
	// pred_iter pred/returned adapter; adapter ctor this/pred; predefined_ops.h:326,310.
	(2 * sizeof(cost_source_padding_predicate) + sizeof(cost_source_padding_adapter) +
	 sizeof(void *)) +
	// move predicate input/returned refs; adapter operator() this/iterator/bool; move.h:105; predefined_ops.h:318.
	(4 * sizeof(void *) + sizeof(bool)) +
	// lambda operator() this/byte/result; before.c decode padding lambda.
	(sizeof(void *) + sizeof(uint8_t) + sizeof(bool));
constexpr size_t cost_complete_bit_cast_encode =
	// const int32 source ref and uint32 returned value; bit:81-88.
	(sizeof(void *) + sizeof(uint32_t));
constexpr size_t cost_complete_bit_cast_decode =
	// const uint32 source ref and int32 returned value; bit:81-88.
	(sizeof(void *) + sizeof(int32_t));
constexpr size_t cost_complete_verify_generated =
	// expected projection generated default/dtor receivers; native_quest_cost.h generated; expected vector default and cleanup separate.
	(2 * sizeof(void *));
// Complete per-runtime source unions. Child Source remains a separate
// complete call graph while its parent controller/scalars remain live.
// Reserve is always on a fresh vector; each subsequent push fits.
constexpr size_t cost_complete_project =
	cost_complete_project_own + cost_complete_native_value + cost_complete_spend +
	cost_complete_array_access + cost_complete_requirement_span + cost_complete_initial +
	cost_complete_request + cost_complete_observation + cost_complete_heap +
	cost_complete_vector_default + cost_complete_reserve_empty +
	cost_complete_max_size_wrapper + cost_complete_max_size_leaf + cost_complete_allocate +
	cost_complete_construct_one + cost_complete_push_attempt_fits +
	cost_complete_generated_common + cost_complete_project_copy_assignments +
	cost_complete_projection_move_assignments + cost_complete_projection_move +
	cost_complete_vector_move + cost_complete_vector_cleanup + cost_complete_deallocate +
	cost_complete_allocator_access;
constexpr size_t cost_complete_verify =
	cost_complete_verify_own + cost_complete_initial + cost_complete_request +
	cost_complete_observation + 2 * cost_complete_heap + 2 * cost_complete_vector_default +
	cost_complete_reserve_empty + cost_complete_max_size_wrapper + cost_complete_max_size_leaf +
	cost_complete_allocate + cost_complete_construct_one + cost_complete_push_requirement_fits +
	2 * cost_complete_vector_cleanup + 2 * cost_complete_deallocate +
	cost_complete_allocator_access + cost_complete_verify_generated +
	cost_complete_attempt_queries + cost_complete_comparison + cost_complete_span_construction +
	cost_complete_project;
constexpr size_t cost_complete_encode =
	cost_complete_encode_own + cost_complete_initial + cost_complete_request +
	cost_complete_observation + cost_complete_heap + cost_complete_verify + cost_complete_put +
	cost_complete_put_cash + cost_complete_array_access + cost_complete_bit_cast_encode +
	cost_complete_byte_queries + cost_complete_attempt_queries + cost_complete_byte_size_ctor +
	cost_complete_max_size_leaf + cost_complete_allocate + cost_complete_vector_cleanup +
	cost_complete_deallocate + cost_complete_allocator_access + cost_complete_vector_move;
constexpr size_t cost_complete_decode =
	cost_complete_decode_own + cost_complete_initial + cost_complete_request +
	cost_complete_observation + cost_complete_heap + cost_complete_verify + cost_complete_get +
	cost_complete_read_cash + cost_complete_array_access + cost_complete_bit_cast_decode +
	cost_complete_byte_span + cost_complete_any_padding + cost_complete_vector_default +
	cost_complete_reserve_empty + cost_complete_max_size_wrapper + cost_complete_max_size_leaf +
	cost_complete_allocate + cost_complete_construct_one + cost_complete_push_attempt_fits +
	cost_complete_generated_common + cost_complete_projection_move_assignments +
	cost_complete_projection_move + cost_complete_vector_move + cost_complete_vector_cleanup +
	cost_complete_deallocate + cost_complete_allocator_access;
// Only typed Source fields continuously held by the matching original
// local frames are credited. Call-scoped observation/mutation allowances
// are never borrowed for whole-body cleanup or early-refusal lifetimes.
// Remaining ambiguous old slots are deliberately unused, never repurposed.
constexpr size_t cost_complete_project_credit =
	4 * sizeof(void *) + sizeof(std::span<const native_quest_cost_requirement>) +
	sizeof(uint64_t) + 3 * sizeof(size_t) + sizeof(int64_t) + sizeof(bool) +
	sizeof(native_quest_cost_projection_result) + cost_complete_native_value +
	cost_complete_spend + cost_complete_vector_default;
constexpr size_t cost_complete_verify_credit =
	4 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(native_quest_cost_projection_result) +
	2 * cost_complete_vector_default +
	// Actual original compare declarations: three defaulted equalities,
	// vector equality references, four std::equal/aux/aux1/false scopes.
	3 * (2 * sizeof(void *) + sizeof(bool)) + 2 * sizeof(void *) +
	4 * (3 * sizeof(void *) + sizeof(bool)) + cost_complete_project_credit;
constexpr size_t cost_complete_encode_credit =
	5 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(native_quest_cost_projection_result) +
	cost_complete_put + cost_complete_put_cash + cost_complete_bit_cast_encode +
	cost_complete_verify_credit;
constexpr size_t cost_complete_decode_credit =
	sizeof(std::span<const uint8_t>) + 5 * sizeof(void *) + 4 * sizeof(size_t) +
	sizeof(uint64_t) + sizeof(native_quest_cost_projection_result) + cost_complete_get +
	cost_complete_read_cash + cost_complete_bit_cast_decode + cost_complete_vector_default +
	cost_complete_verify_credit;
static_assert(cost_complete_project >= cost_complete_project_credit);
static_assert(cost_complete_encode >= cost_complete_encode_credit);
static_assert(cost_complete_decode >= cost_complete_decode_credit);
}
#endif
bool native_quest_cost_project_source_frame_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	if (!output)
		return false;
	*output = cost_complete_project;
	return true;
#else
	(void)output;
	return false;
#endif
}
bool native_quest_cost_project_source_supplement_frame_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	if (!output)
		return false;
	*output = cost_complete_project - cost_complete_project_credit;
	return true;
#else
	(void)output;
	return false;
#endif
}
bool native_quest_cost_projection_encode_source_frame_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	if (!output)
		return false;
	*output = cost_complete_encode;
	return true;
#else
	(void)output;
	return false;
#endif
}
bool native_quest_cost_projection_encode_source_supplement_frame_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	if (!output)
		return false;
	*output = cost_complete_encode - cost_complete_encode_credit;
	return true;
#else
	(void)output;
	return false;
#endif
}
bool native_quest_cost_projection_decode_source_frame_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	if (!output)
		return false;
	*output = cost_complete_decode;
	return true;
#else
	(void)output;
	return false;
#endif
}
bool native_quest_cost_projection_decode_source_supplement_frame_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	if (!output)
		return false;
	*output = cost_complete_decode - cost_complete_decode_credit;
	return true;
#else
	(void)output;
	return false;
#endif
}
