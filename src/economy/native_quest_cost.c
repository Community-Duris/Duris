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
