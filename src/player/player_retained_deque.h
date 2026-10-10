#ifndef PLAYER_RETAINED_DEQUE_H
#define PLAYER_RETAINED_DEQUE_H

#include <cstddef>
#include <deque>
#include <limits>
#include <cstdint>
#include <initializer_list>
#include <vector>

// Genuine standard deque owner with a passive supported-library storage view.
// Every original operation, element/allocator type and constructor is inherited.
// No parallel capacity ledger, private-member override, cast or layout replica.
template <typename T> class player_retained_deque : public std::deque<T>
{
    public:
	using std::deque<T>::deque;
	using std::deque<T>::operator=;
	player_retained_deque() = default;
	player_retained_deque(const player_retained_deque &) = default;
	player_retained_deque(player_retained_deque &&) = default;
	player_retained_deque &operator=(const player_retained_deque &) = default;
	player_retained_deque &operator=(player_retained_deque &&) = default;
	// Excludes this owner's inline object and element-owned nested heaps.
	// Caller stabilizes the actual owner for the entire scan. Empty ordinary
	// deques retain their real allocated map and one node; moved-from null
	// storage has none. Strong scalar output, no callbacks/allocations.
	bool current_heap_bytes(size_t *output) const noexcept
	{
		if (!output)
			return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
		if (sizeof(void *) != 8 || sizeof(size_t) != 8)
			return false;
		using gnu_base = std::_Deque_base<T, std::allocator<T>>;
		// Explicit indirect base qualification bypasses deque's private
		// using-declaration; actual base storage is protected and belongs to
		// this genuine derived owner. It is not a borrowed deque cast.
		const auto &impl = this->gnu_base::_M_impl;
		if (!impl._M_map)
		{
			if (impl._M_map_size || impl._M_start._M_node || impl._M_finish._M_node)
				return false;
			*output = 0;
			return true;
		}
		if (!impl._M_map_size || !impl._M_start._M_node || !impl._M_finish._M_node ||
		    impl._M_start._M_node < impl._M_map ||
		    impl._M_finish._M_node < impl._M_start._M_node ||
		    impl._M_finish._M_node >= impl._M_map + impl._M_map_size)
			return false;
		const size_t maximum = std::numeric_limits<size_t>::max();
		const size_t nodes =
			static_cast<size_t>(impl._M_finish._M_node - impl._M_start._M_node) + 1;
		const size_t elements = std::__deque_buf_size(sizeof(T));
		if (!elements || impl._M_map_size > maximum / sizeof(T *) ||
		    elements > maximum / sizeof(T))
			return false;
		const size_t block = elements * sizeof(T);
		if (nodes > maximum / block)
			return false;
		const size_t map_bytes = impl._M_map_size * sizeof(T *);
		const size_t node_bytes = nodes * block;
		if (node_bytes > maximum - map_bytes)
			return false;
		*output = map_bytes + node_bytes;
		return true;
#else
		return false;
#endif
	}
};

// Complete source-declared CURRENT observer call graph, pinned to the actual
// full snapshot functions and installed GCC13 headers captured with this slice.
// Values model declared signature/local/iterator/return carriers, not emitted
// stack or allocator metadata. Branch maxima keep sequential callees disjoint;
// parent carriers persist across their genuine nested child calls.
namespace player_retained_observer_source
{
constexpr size_t ptr = sizeof(void *), scalar = sizeof(size_t), bit = sizeof(bool);
constexpr size_t maximum(size_t a, size_t b) noexcept
{
	return a > b ? a : b;
}
constexpr size_t checked_add = ptr + scalar + bit;
constexpr size_t checked_array = ptr + 2 * scalar + bit + checked_add;
// vector capacity/size: this, size_type result and pointer-difference result.
constexpr size_t vector_query = ptr + scalar + sizeof(std::ptrdiff_t);
constexpr size_t vector_request = 2 * ptr + 2 * bit + scalar + maximum(vector_query, checked_add);
// string capacity -> _M_is_local -> _M_data/_M_local_data -> pointer_to ->
// addressof/__addressof. Each real function owns its this/ref and return.
constexpr size_t string_query =
	ptr + scalar + ptr + bit + maximum(2 * ptr, 2 * ptr + 2 * ptr + 2 * ptr + 2 * ptr);
constexpr size_t string_request = 2 * ptr + 2 * bit + scalar + maximum(string_query, checked_add);
// Optional bool and actual engaged get chains. operator-> additionally reaches
// std::addressof/__addressof; no optional payload copy or allocation is reached.
constexpr size_t optional_present = 2 * (ptr + bit);
constexpr size_t optional_value = 6 * ptr + optional_present;
constexpr size_t optional_arrow = 10 * ptr + optional_present;
// Vector normal_iterator range: __range and element refs, actual begin/end
// iterator objects; begin/end+constructor, ==/rewritten !=, ++ and * query scopes.
constexpr size_t normal_range_live = 2 * ptr + 2 * sizeof(std::vector<uint8_t>::const_iterator);
constexpr size_t normal_range_query =
	maximum(ptr + sizeof(std::vector<uint8_t>::const_iterator) + 2 * ptr,
		maximum(2 * ptr + 2 * bit, 2 * ptr));
constexpr size_t normal_range = normal_range_live + normal_range_query;
// array range uses genuine raw pointers. begin/end -> data -> _S_ptr;
// built-in comparison/increment do not construct a hidden iterator class.
constexpr size_t array_range_live = 4 * ptr;
constexpr size_t array_range_query = 6 * ptr + bit;
constexpr size_t array_range = array_range_live + array_range_query;
// Actual item row request, nested descriptions/spell vector, no clone/copy.
constexpr size_t row_request =
	2 * ptr + 2 * bit +
	maximum(maximum(string_request, vector_request),
		normal_range_live +
			maximum(normal_range_query, maximum(string_request, vector_request)));
constexpr size_t items_request =
	2 * ptr + bit +
	maximum(vector_request, normal_range_live + maximum(normal_range_query, row_request));
// Evidence columns, row-vector blocks and optional cell strings are nested
// ranges. Rows and cells coexist; columns are a separate sequential range.
constexpr size_t evidence_request =
	2 * ptr + bit +
	maximum(vector_request,
		maximum(normal_range_live + maximum(normal_range_query, string_request),
			normal_range_live +
				maximum(normal_range_query,
					maximum(vector_request,
						normal_range_live +
							maximum(normal_range_query,
								maximum(optional_present,
									optional_value +
										string_request))))));
// Full snapshot: status/affect/pet ranges are sequential; pet items nest one
// item range. Death/conflict reference carriers persist around their respective
// child observations, including all five sequential evidence-table calls.
constexpr size_t snapshot_request =
	2 * ptr + scalar + bit +
	maximum(bit,
		maximum(maximum(vector_request, string_request),
			maximum(normal_range_live + maximum(normal_range_query,
							    maximum(string_request, items_request)),
				maximum(optional_present,
					maximum(optional_value,
						ptr + maximum(items_request,
							      maximum(vector_request,
								      maximum(optional_present,
									      maximum(optional_value,
										      ptr + evidence_request)))))))));
// Exact unique_ptr::get -> __uniq_ptr_impl::_M_ptr -> get<0> -> __get_helper ->
// _Tuple_impl::_M_head -> _Head_base::_M_head, each this/ref + returned pointer.
constexpr size_t unique_get = 6 * (2 * ptr);
// Actual initializer-list job pair: two-pointer backing array, descriptor,
// __range ref, begin/end pointers and job pointer. end reaches begin and size.
constexpr size_t job_range_live = 2 * ptr + sizeof(std::initializer_list<void *>) + 4 * ptr;
constexpr size_t job_range_query = 5 * ptr + scalar;
// lock_guard object belongs to the worker method, below. Constructor/destructor
// signatures and mutex lock's real __e, gthread lock/unlock/active query, and
// pthread declaration boundary argument/result scopes are attributed here.
constexpr size_t leaf_lock_queries =
	2 * ptr + ptr + sizeof(int) + ptr + sizeof(int) + sizeof(int) + ptr + sizeof(int);
// No owning storage is discovered by these pure fixed-source constants.
} // namespace player_retained_observer_source

#endif
