#include "economy/native_mobile_birth_accounting.h"

#include <algorithm>
#include <new>
#include <utility>

economic_accounting_error
native_mobile_birth_accounting_compile(const critical_command &command,
				       const economic_account_key &original_native_wallet,
				       economic_accounting_plan *output) noexcept
{
	using error = economic_accounting_error;
	if (!output)
		return error::corrupt_evidence;
	try
	{
		quest_mobile_native_image image;
		auto status = native_mobile_birth_command_decode(command, &image);
		if (status != error::ok)
			return status;
		if (!image.cash || image.cash->revision != 1)
			return error::corrupt_evidence; // Historical unknown cash is never zero.
		if (image.items.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS ||
		    image.items.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES)
			return error::capacity;

		economic_frozen_intent intent;
		status = economic_intent_decode(command.accounting_intent, &intent);
		if (status != error::ok)
			return status;
		economic_accounting_plan candidate;
		status = economic_intent_plan_metadata(command, intent, &candidate.metadata);
		if (status != error::ok)
			return status;

		// The original atomic owner supplies its created/locked mapping lifetime.
		// Native UID and mapping ID use distinct namespaces; never infer this key.
		if (!economic_account_key_valid(original_native_wallet) ||
		    original_native_wallet.lineage.bytes != candidate.metadata.lineage.bytes ||
		    original_native_wallet.kind != economic_account_kind::wallet ||
		    original_native_wallet.context_id != ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT)
			return error::invalid_identity;

		const auto &cash = image.cash->denominations.amount;
		economic_coin_vector delta{};
		status = economic_coin_delta({}, cash, &delta);
		if (status != error::ok)
			return status;
		candidate.accounts.push_back(
			{ original_native_wallet, {}, cash, 0, image.cash->revision });
		if (std::any_of(delta.begin(), delta.end(), [](int64_t part) { return part != 0; }))
		{
			economic_coin_vector opposite{};
			status = economic_coin_delta(cash, {}, &opposite);
			if (status != error::ok)
				return status;
			int64_t copper = 0;
			status = economic_coin_value(delta, &copper);
			if (status != error::ok)
				return status;
			candidate.accounts.push_back({ { candidate.metadata.lineage,
							 economic_account_kind::issuance, 1, 0 },
						       {},
						       {},
						       0,
						       0 });
			candidate.postings = { { 0, 0, 0, delta, copper },
					       { 1, 1, 0, opposite, -copper } };
		}
		// Known zero still creates the ordinary wallet at revision1. Its unchanged
		// balance needs no posting; an unreferenced zero issuance account is invalid.

		candidate.items_before.reserve(image.items.size());
		candidate.items_after.reserve(image.items.size());
		candidate.item_events.reserve(image.items.size());
		for (size_t index = 0; index < image.items.size(); ++index)
		{
			const auto &item = image.items[index];
			economic_item_position after{};
			after.owner = { item_owner_type::native_mobile,
					image.reference.mobile_instance_id, 0 };
			after.root_uid = item.object_uid;
			after.revision = 1;
			after.state = item_custody_state::active;
			after.equipment_slot = static_cast<uint16_t>(item.equipment_slot);
			if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			{
				if (item.parent_index < 0 ||
				    static_cast<size_t>(item.parent_index) >= index ||
				    item.equipment_slot)
					return error::topology;
				const size_t parent = static_cast<size_t>(item.parent_index);
				after.root_uid = candidate.items_after[parent].position.root_uid;
				after.parent_uid = image.items[parent].object_uid;
			}
			candidate.items_before.push_back({ item.object_uid, {} });
			candidate.items_after.push_back({ item.object_uid, after });
			// Event indices retain original equipment/carry DFS order. Shared
			// normalization sorts witnesses by UID without changing this order.
			candidate.item_events.push_back(
				{ static_cast<uint32_t>(index), 0, item.object_uid, {}, after });
		}
		status = economic_plan_normalize(&candidate);
		if (status != error::ok)
			return status;
		*output = std::move(candidate);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
}

#include <type_traits>

namespace
{
bool birth_compile_add(size_t &bytes, size_t added) noexcept
{
	if (added > SIZE_MAX - bytes)
		return false;
	bytes += added;
	return true;
}
bool birth_compile_rows(size_t &bytes, size_t count, size_t width) noexcept
{
	return (!width || count <= SIZE_MAX / width) && birth_compile_add(bytes, count * width);
}
bool birth_compile_admit(size_t base, size_t extra, bool (*reserve)(size_t, void *) noexcept,
			 void *context) noexcept
{
	return extra <= SIZE_MAX - base && reserve && reserve(base + extra, context);
}
bool birth_compile_plan_heap(const economic_accounting_plan &plan, size_t &bytes) noexcept
{
	bytes = 0;
	return birth_compile_rows(bytes, plan.accounts.capacity(),
				  sizeof(economic_account_effect)) &&
	       birth_compile_rows(bytes, plan.postings.capacity(), sizeof(economic_coin_posting)) &&
	       birth_compile_rows(bytes, plan.children.capacity(), sizeof(economic_child_link)) &&
	       birth_compile_rows(bytes, plan.items_before.capacity(),
				  sizeof(economic_item_snapshot)) &&
	       birth_compile_rows(bytes, plan.items_after.capacity(),
				  sizeof(economic_item_snapshot)) &&
	       birth_compile_rows(bytes, plan.item_events.capacity(), sizeof(economic_item_event));
}
bool birth_compile_account_push(size_t size, size_t capacity, size_t &extra) noexcept
{
	extra = sizeof(economic_account_effect);
	if (size != capacity)
		return true;
	const size_t growth = std::max(size, size_t{ 1 });
	return growth <= SIZE_MAX - size &&
	       birth_compile_rows(extra, size + growth, sizeof(economic_account_effect));
}
struct birth_compile_workspace
{
	quest_mobile_native_image image;
	economic_frozen_intent intent;
	economic_accounting_plan candidate;
	economic_coin_vector delta{};
	economic_accounting_plan_allocation_profile profile;
	std::span<const uint8_t> intent_wire;
	size_t image_heap = 0;
};
struct birth_compile_live
{
	birth_compile_workspace &work;
	size_t base;
	bool bytes(size_t &out) const noexcept
	{
		out = base;
		size_t plan_heap = 0;
		return birth_compile_add(out, work.image_heap) &&
		       birth_compile_add(out, work.intent.admission.facts.capacity()) &&
		       birth_compile_plan_heap(work.candidate, plan_heap) &&
		       birth_compile_add(out, plan_heap);
	}
};
}

economic_accounting_error native_mobile_birth_accounting_compile_v3_bounded(
	const critical_command &command, const economic_account_key &original_native_wallet,
	economic_accounting_plan *output, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live, size_t *retained_plan_heap_bytes) noexcept
{
	using error = economic_accounting_error;
	if (!output)
		return error::corrupt_evidence;
	// This companion serves the genuine cash-role projection; historical formats
	// still use the unchanged original compiler and are not claimed bounded here.
	if (command.payload_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION)
		return error::unresolved;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)original_native_wallet;
	(void)reserve;
	(void)context;
	(void)outer_live;
	(void)retained_plan_heap_bytes;
	return error::unresolved;
#else
	size_t base = outer_live;
	if (!birth_compile_add(base, sizeof(birth_compile_workspace)) ||
	    !birth_compile_add(base, sizeof(birth_compile_live)) ||
	    !birth_compile_admit(base, 0, reserve, context))
		return error::capacity;
	try
	{
		birth_compile_workspace work;
		birth_compile_live live{ work, base };
		auto &image = work.image;
		auto &intent = work.intent;
		auto &candidate = work.candidate;
		auto &delta = work.delta;
		error status;
		{
			// Original image-only decode discards recipes/constructor after complete
			// command proof. Keep that same lifetime rather than retain dead payloads.
			constexpr size_t decode_outputs =
				sizeof(std::vector<native_mobile_birth_item_recipe>) +
				sizeof(quest_mobile_native_constructor_recipe);
			if (!birth_compile_admit(base, decode_outputs, reserve, context))
				return error::capacity;
			std::vector<native_mobile_birth_item_recipe> recipes;
			quest_mobile_native_constructor_recipe constructor;
			size_t decode_outer = base;
			if (!birth_compile_add(decode_outer, decode_outputs))
				return error::capacity;
			status = native_mobile_birth_command_decode_bounded(
				command, &image, &recipes, &constructor, reserve, context,
				decode_outer, &work.image_heap);
		}
		if (status != error::ok)
			return status;
		if (!image.cash || image.cash->revision != 1)
			return error::corrupt_evidence;
		if (image.items.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS ||
		    image.items.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES)
			return error::capacity;
		size_t current = 0;
		if (!live.bytes(current) ||
		    !birth_compile_admit(current, sizeof(std::span<const uint8_t>), reserve,
					 context))
			return error::capacity;
		work.intent_wire = std::span<const uint8_t>(command.accounting_intent);
		status = economic_intent_decode_bounded(work.intent_wire, &intent, reserve, context,
							current);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		status = economic_intent_plan_metadata_bounded(command, intent, &candidate.metadata,
							       reserve, context, current);
		if (status != error::ok)
			return status;
		if (!economic_account_key_valid(original_native_wallet) ||
		    original_native_wallet.lineage.bytes != candidate.metadata.lineage.bytes ||
		    original_native_wallet.kind != economic_account_kind::wallet ||
		    original_native_wallet.context_id != ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT)
			return error::invalid_identity;
		const auto &cash = image.cash->denominations.amount;
		// Zero argument temporary and economic_coin_delta's own result coexist;
		// the output delta is already a named workspace object.
		if (!live.bytes(current) ||
		    !birth_compile_admit(current, 2 * sizeof(economic_coin_vector), reserve,
					 context))
			return error::capacity;
		status = economic_coin_delta({}, cash, &delta);
		if (status != error::ok)
			return status;
		size_t extra = 0;
		if (!live.bytes(current) ||
		    !birth_compile_account_push(candidate.accounts.size(),
						candidate.accounts.capacity(), extra) ||
		    !birth_compile_admit(current, extra, reserve, context))
			return error::capacity;
		candidate.accounts.push_back(
			{ original_native_wallet, {}, cash, 0, image.cash->revision });
		if (std::any_of(delta.begin(), delta.end(), [](int64_t part) { return part != 0; }))
		{
			if (!live.bytes(current) ||
			    !birth_compile_admit(current, 3 * sizeof(economic_coin_vector), reserve,
						 context))
				return error::capacity;
			economic_coin_vector opposite{};
			// The admitted local opposite survives the zero argument/callee result.
			status = economic_coin_delta(cash, {}, &opposite);
			if (status != error::ok)
				return status;
			int64_t copper = 0;
			status = economic_coin_value(delta, &copper);
			if (status != error::ok)
				return status;
			if (!live.bytes(current) || !birth_compile_add(current, sizeof(opposite)) ||
			    !birth_compile_account_push(candidate.accounts.size(),
							candidate.accounts.capacity(), extra) ||
			    !birth_compile_admit(current, extra, reserve, context))
				return error::capacity;
			candidate.accounts.push_back({ { candidate.metadata.lineage,
							 economic_account_kind::issuance, 1, 0 },
						       {},
						       {},
						       0,
						       0 });
			if (!live.bytes(current) || !birth_compile_add(current, sizeof(opposite)))
				return error::capacity;
			extra = sizeof(std::initializer_list<economic_coin_posting>) +
				sizeof(std::array<economic_coin_posting, 2>);
			if (!birth_compile_rows(extra, 2, sizeof(economic_coin_posting)) ||
			    !birth_compile_admit(current, extra, reserve, context))
				return error::capacity;
			candidate.postings = { { 0, 0, 0, delta, copper },
					       { 1, 1, 0, opposite, -copper } };
		}
		size_t item_requests = 0;
		const size_t item_temporary =
			image.items.empty() ?
				size_t{ 0 } :
				sizeof(economic_item_position) +
					std::max(sizeof(item_owner_identity),
						 std::max(sizeof(economic_item_snapshot),
							  sizeof(economic_item_event)));
		if (!birth_compile_rows(item_requests, image.items.size(),
					2 * sizeof(economic_item_snapshot) +
						sizeof(economic_item_event)) ||
		    !birth_compile_add(item_requests, item_temporary) || !live.bytes(current) ||
		    !birth_compile_admit(current, item_requests, reserve, context))
			return error::capacity;
		candidate.items_before.reserve(image.items.size());
		candidate.items_after.reserve(image.items.size());
		candidate.item_events.reserve(image.items.size());
		for (size_t index = 0; index < image.items.size(); ++index)
		{
			const auto &item = image.items[index];
			economic_item_position after{};
			after.owner = { item_owner_type::native_mobile,
					image.reference.mobile_instance_id, 0 };
			after.root_uid = item.object_uid;
			after.revision = 1;
			after.state = item_custody_state::active;
			after.equipment_slot = static_cast<uint16_t>(item.equipment_slot);
			if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			{
				if (item.parent_index < 0 ||
				    static_cast<size_t>(item.parent_index) >= index ||
				    item.equipment_slot)
					return error::topology;
				const size_t parent = static_cast<size_t>(item.parent_index);
				after.root_uid = candidate.items_after[parent].position.root_uid;
				after.parent_uid = image.items[parent].object_uid;
			}
			candidate.items_before.push_back({ item.object_uid, {} });
			candidate.items_after.push_back({ item.object_uid, after });
			candidate.item_events.push_back(
				{ static_cast<uint32_t>(index), 0, item.object_uid, {}, after });
		}
		if (!live.bytes(current) ||
		    !birth_compile_admit(current,
					 economic_plan_allocation_preflight_working_bytes(),
					 reserve, context))
			return error::capacity;
		status = economic_plan_allocation_preflight(candidate, &work.profile);
		if (status != error::ok)
			return status;
		if (!work.profile.storage_policy_supported ||
		    !birth_compile_admit(current, work.profile.normalize_working_bytes, reserve,
					 context))
			return error::capacity;
		status = economic_plan_normalize(&candidate);
		if (status != error::ok)
			return status;
		size_t retained = 0;
		if (!birth_compile_plan_heap(candidate, retained))
			return error::capacity;
		static_assert(std::is_nothrow_move_assignable_v<economic_accounting_plan>);
		*output = std::move(candidate);
		if (retained_plan_heap_bytes)
			*retained_plan_heap_bytes = retained;
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
#endif
}

#if defined(__linux__) && defined(__x86_64__) && defined(__LP64__) && !defined(_WIN32) &&         \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&           \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                           \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                        \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
namespace
{
constexpr size_t birth_fixed_native_mobile_birth_accounting_compile_profile_query_frame_bytes =
	native_mobile_birth_accounting_compile_profile_query_frame_bytes();
constexpr size_t birth_fixed_native_mobile_birth_command_general_decode_query_frame_bytes =
	native_mobile_birth_command_source_query_frame_bytes();
constexpr size_t birth_fixed_economic_intent_decode_source_query_frame_bytes =
	economic_intent_decode_source_query_frame_bytes();
constexpr size_t birth_fixed_economic_intent_plan_metadata_fixed_source_query_frame_bytes =
	economic_intent_plan_metadata_fixed_source_query_frame_bytes();
constexpr size_t birth_fixed_economic_plan_bounded_profile_query_frame_bytes =
	economic_plan_bounded_profile_query_frame_bytes();
}
#endif
#if defined(__linux__) && defined(__x86_64__) && defined(__LP64__) && !defined(_WIN32) &&         \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&           \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                           \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                        \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
namespace
{
struct general_birth_compile_live
{
	birth_compile_workspace &work;
	size_t base;
	bool bytes(size_t &out) const noexcept
	{
		out = base;
		size_t image_heap = 0, plan_heap = 0;
		return player_item_snapshot_list_current_heap_bytes(work.image.items,
								    &image_heap) &&
		       birth_compile_add(out, image_heap) &&
		       birth_compile_add(out, work.intent.admission.facts.capacity()) &&
		       birth_compile_plan_heap(work.candidate, plan_heap) &&
		       birth_compile_add(out, plan_heap);
	}
};
}

namespace
{
using birth_profile_getter = bool (*)(size_t *) noexcept;
bool general_birth_child_entry(const general_birth_compile_live &live, birth_profile_getter source,
			       birth_profile_getter initial, size_t query,
			       bool (*reserve)(size_t, void *) noexcept, void *context,
			       size_t &outer) noexcept
{
	size_t current = 0, source_bytes = 0, inline_bytes = 0, profile_request = query;
	if (!live.bytes(current) || !birth_compile_add(profile_request, query) ||
	    !birth_compile_admit(current, profile_request, reserve, context) ||
	    !source(&source_bytes) || !initial(&inline_bytes) ||
	    !birth_compile_add(source_bytes, inline_bytes) ||
	    !birth_compile_admit(current, source_bytes, reserve, context))
		return false;
	outer = current; // Prospective child SOURCE/inline is not retained twice.
	return true;
}
}

#endif
#if defined(__linux__) && defined(__x86_64__) && defined(__LP64__) && !defined(_WIN32) &&         \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&           \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                           \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                        \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
// PRIVATE draft. Genuine finite GNU13 SOURCE inventory, independent review
// pending. Each subtotal names selected installed bodies; allocator/emitted
// exception/libc tails are external qualification boundaries, not private heap.
namespace
{
constexpr size_t birth_source_pointer = sizeof(void *);
constexpr size_t birth_source_size = sizeof(size_t);
constexpr size_t birth_source_bool = sizeof(bool);
constexpr size_t birth_source_diff = sizeof(std::ptrdiff_t);

template <class Row> constexpr size_t birth_trivial_vector_lifetime_source() noexcept
{
	static_assert(std::is_trivially_destructible_v<Row>);
	using allocator = std::allocator<Row>;
	constexpr size_t P = sizeof(void *), N = sizeof(size_t), B = sizeof(bool);
	// vector/_Vector_base/_Vector_impl/allocator/new_allocator default ctors,
	// vector destructor -> _Destroy(first,last,allocator) -> _Destroy(first,last)
	// -> trivial _Destroy_aux::__destroy, then _Vector_base destructor and
	// _M_deallocate -> allocator_traits::deallocate -> allocator::deallocate
	// -> __new_allocator::deallocate. operator-delete args are boundary carriers.
	constexpr size_t constructors = 6 * P;
	constexpr size_t destruction = P + 3 * P + 2 * P + 2 * P + P + 3 * P;
	constexpr size_t deallocation = 4 * (2 * P + N) + (P + N);
	constexpr size_t base_getters = 2 * (2 * P) + P + B;
	// Real get_allocator/_M_get_Tp_allocator/allocator copy used by move
	// assignment, plus the temporary allocator's actual inline bytes.
	// get_allocator(this, allocator result), _M_get_Tp_allocator(this/ref),
	// allocator copy(this/source) and new_allocator copy(this/source).
	constexpr size_t allocator_copy = 7 * P + sizeof(allocator);
	return constructors + destruction + deallocation + base_getters + allocator_copy + N;
}

template <class Row> constexpr size_t birth_trivial_vector_move_source() noexcept
{
	static_assert(std::is_trivially_copyable_v<Row>);
	using allocator = std::allocator<Row>;
	using vector = std::vector<Row>;
	using base = std::_Vector_base<Row, allocator>;
	constexpr size_t P = sizeof(void *), N = sizeof(size_t), B = sizeof(bool);
	// operator=(vector&&) -> _M_move_assign(true_type): actual __tmp vector,
	// two _M_swap_data with actual _Vector_impl_data temporaries, their ctor
	// and three _M_copy_data calls each; then CXX20 direct __alloc_on_move.
	constexpr size_t assignment = 3 * P + B + 2 * P + sizeof(std::true_type) + sizeof(vector);
	// Real __tmp allocator constructor, not the default constructor: vector,
	// base, impl, allocator and new_allocator each(this/source), then data this.
	constexpr size_t temporary_constructor = 5 * (2 * P) + P;
	constexpr size_t swaps =
		2 * (2 * P + sizeof(typename base::_Vector_impl_data) + P + 3 * (2 * P));
	// CXX20 __alloc_on_move: two allocator references; std::move arg/ref
	// return; defaulted allocator copy assignment this/source/ref return and
	// its actual __new_allocator base's generated copy assignment same3P.
	constexpr size_t allocator_move = 2 * P + 2 * P + 3 * P + 3 * P;
	constexpr size_t forwards = 4 * (2 * P);
	return assignment + temporary_constructor + swaps + allocator_move + forwards +
	       birth_trivial_vector_lifetime_source<Row>() + N;
}

template <class Row> constexpr size_t birth_trivial_vector_mutation_source() noexcept
{
	static_assert(std::is_trivially_copyable_v<Row>);
	constexpr size_t P = sizeof(void *), N = sizeof(size_t), B = sizeof(bool),
			 D = sizeof(std::ptrdiff_t);
	// Selected vector reserve (__old_size,__tmp) and _M_realloc_insert
	// (__len,__old_start,__old_finish,__elems_before,__new_start,__new_finish).
	constexpr size_t reserve = 2 * P + 2 * N;
	constexpr size_t realloc_insert =
		8 * P + 2 * N + sizeof(typename std::vector<Row>::iterator);
	// _M_check_len __len, max_size->_S_max_size (__diffmax,__allocmax),
	// allocator_traits::max_size and std::min/max. Calls are finite and
	// sequential; static-call sum also dominates each nested phase.
	constexpr size_t length = 2 * P + 3 * N + 4 * (P + N) + P + 3 * N + P + N + 2 * (3 * P + B);
	// _M_allocate -> traits -> allocator -> new_allocator; actual default
	// hint argument included. _M_max_size, is_constant_evaluated, and boundary
	// operator-new argument/result are included, no imagined allocator header.
	static_assert(alignof(Row) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__);
	constexpr size_t allocation =
		2 * (2 * P + N) + (2 * P + N) + (3 * P + N) + (P + N) + B + (P + N);
	// allocator_traits::construct -> construct_at -> forwarding -> placement
	// new and actual row move ctor. Row ctor is trivial value field transfer.
	constexpr size_t construct = 3 * P + 2 * P + 3 * P + 2 * P + (2 * P + N) + 2 * P;
	// Selected non-bitwise __relocate_a -> three __niter_base calls ->
	// __relocate_a_1 loop -> __relocate_object_a -> addressof/construct/destroy.
	// Default-initialized economic rows are not is_trivial; do not falsely
	// claim the memmove overload. Both realloc halves and reserve use this.
	constexpr size_t relocate = 5 * P + 3 * B + 5 * P + 3 * (2 * P) + 6 * P + 3 * P +
				    2 * (2 * P) + construct + 2 * P + P;
	constexpr size_t destroy = 3 * P + 2 * P + 2 * P + 2 * P + P;
	constexpr size_t vector_access = 6 * (P + N) + 2 * (2 * P) + 2 * (2 * P) + 2 * P;
	constexpr size_t iteration = 18 * P + B; // authentic normal_iterator loop chain
	return reserve + realloc_insert + length + allocation + construct + 9 * P + 3 * relocate +
	       destroy + vector_access + iteration + birth_trivial_vector_lifetime_source<Row>() +
	       D + N;
}

constexpr size_t birth_posting_initializer_source() noexcept
{
	constexpr size_t P = sizeof(void *), N = sizeof(size_t), B = sizeof(bool),
			 D = sizeof(std::ptrdiff_t);
	// vector::operator=(initializer_list) -> begin/end -> _M_assign_aux
	// (__len,__tmp,__mid,__n), distance/category, advance/category, copy chain,
	// uninitialized_copy_a and fresh allocation/copy. The bytewise copy helper
	// includes __Num plus __assign_one for the one-row suffix branch.
	constexpr size_t assignment = 2 * P + sizeof(std::initializer_list<economic_coin_posting>) +
				      2 * (P + P) + 4 * P + 2 * N +
				      sizeof(std::forward_iterator_tag);
	constexpr size_t distance = 4 * P + 2 * D + sizeof(std::random_access_iterator_tag) +
				    2 * (P + sizeof(std::random_access_iterator_tag));
	constexpr size_t advance = 2 * P + 3 * D + sizeof(std::random_access_iterator_tag);
	constexpr size_t copy = 4 * P + 4 * P + 4 * P + 4 * P + D + 2 * P + 3 * (2 * P);
	constexpr size_t uninitialized = 4 * P + 4 * P + 4 * P + 4 * P + D + 2 * P;
	return assignment + distance + advance + copy + uninitialized + B +
	       birth_trivial_vector_mutation_source<economic_coin_posting>() + N;
}

constexpr size_t birth_scalar_source_frames() noexcept
{
	constexpr size_t P = sizeof(void *), N = sizeof(size_t), B = sizeof(bool),
			 D = sizeof(std::ptrdiff_t);
	constexpr size_t W = sizeof(__int128_t), I = sizeof(int64_t),
			 E = sizeof(economic_accounting_error);
	// Account-key validity -> operation-id-zero loop and kind validity switch.
	constexpr size_t identity =
		P + B + P + sizeof(uint8_t) + B + sizeof(economic_account_kind) + B;
	// Fixed-width std::array lineage compare: array begin/end+_S_ptr, equal
	// dispatch -> pointer __equal_aux/__equal_aux1/__equal<true> and memcmp
	// boundary carriers. No parallel/debug selection admitted.
	constexpr size_t equality = 2 * P + B + 4 * (2 * P) + 3 * P + B + 3 * P + B + 3 * P + B +
				    2 * P + B + 2 * P + N + sizeof(int);
	// Original coin_delta includes two nonnegative/all_of calls, two coin_value
	// calls, checked wide difference/narrow. The actual output result vector is
	// separately admitted physical inline in the unchanged semantic body.
	constexpr size_t value = 2 * P + W + N + E + 2 * P + 2 * P + W + P + B + 2 * I;
	constexpr size_t narrow = W + P + B + 2 * I;
	// Selected find_if/find_if_not random-access dispatch has trip_count and
	// iterator/category/predicate adapters; fixed loop unrolling is not item
	// count-dependent SOURCE. Empty predicates are bound by P and the actual
	// int64_t argument/bool result; iterator/result pointers are explicit.
	constexpr size_t predicate =
		3 * P + B + D + I + B + 4 * P + sizeof(std::random_access_iterator_tag) + 3 * P +
		B + 3 * P + B + 2 * P + sizeof(std::random_access_iterator_tag) + 4 * (P + I + B);
	constexpr size_t delta = 3 * P + I + N + E + 2 * predicate + 2 * value + narrow;
	constexpr size_t nonzero = predicate + 2 * (2 * P);
	// Actual optional bool/arrow/base payload/addressof leaves.
	constexpr size_t cash_access = 2 * (P + B) + 4 * (2 * P);
	// Scalar array operator[]/_S_ref and begin/end/_S_ptr shared by coin leaves.
	constexpr size_t array = 4 * (2 * P) + 2 * (2 * P + N);
	return identity + equality + 2 * delta + value + nonzero + cash_access + array + N;
}

constexpr size_t birth_general_local_source_frames() noexcept
{
	constexpr size_t P = sizeof(void *), N = sizeof(size_t), B = sizeof(bool),
			 E = sizeof(economic_accounting_error);
	// Full caller formals/locals and helper args/locals, excluding real workspace
	// and observer inline charged by initial_inline. Opposite/row/posting objects
	// are separately admitted before construction in the actual semantic body.
	// Six pointer formals + outer size_t; six actual image/intent/plan/delta/
	// cash/item references; twelve named size_t locals across genuine scopes.
	constexpr size_t caller = 12 * P + 13 * N + E + sizeof(int64_t);
	constexpr size_t arithmetic = (P + N + B) + (P + 2 * N + B) + (3 * P + 2 * N + B);
	constexpr size_t live = 2 * P + 2 * N + B;
	constexpr size_t plan_heap = 2 * P + B + 6 * (P + N) + 6 * (P + 2 * N + B);
	constexpr size_t account_request = 3 * N + P + B + 2 * (P + N + B);
	constexpr size_t child_preflight = 7 * P + 5 * N + B + 4 * (P + N + B) + 3 * P + 2 * N + B;
	constexpr size_t span = 2 * P + 2 * N + 2 * (P + N) + 2 * P;
	constexpr size_t metadata_move = 2 * P + 2 * P;
	return caller + arithmetic + live + plan_heap + account_request + child_preflight + span +
	       metadata_move + birth_scalar_source_frames() +
	       birth_trivial_vector_mutation_source<economic_account_effect>() +
	       birth_posting_initializer_source() +
	       2 * birth_trivial_vector_mutation_source<economic_item_snapshot>() +
	       birth_trivial_vector_mutation_source<economic_item_event>() +
	       birth_trivial_vector_lifetime_source<uint8_t>() +
	       birth_trivial_vector_lifetime_source<economic_child_link>() +
	       birth_trivial_vector_move_source<economic_account_effect>() +
	       birth_trivial_vector_move_source<economic_coin_posting>() +
	       birth_trivial_vector_move_source<economic_child_link>() +
	       2 * birth_trivial_vector_move_source<economic_item_snapshot>() +
	       birth_trivial_vector_move_source<economic_item_event>() + N;
}
}

#endif

bool native_mobile_birth_accounting_compile_own_source_frame_bytes(size_t *output) noexcept
{
#if !defined(__linux__) || !defined(__x86_64__) || !defined(__LP64__) || defined(_WIN32) ||      \
	!defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 ||                                  \
	!defined(_GLIBCXX_USE_CXX11_ABI) || !_GLIBCXX_USE_CXX11_ABI || __cplusplus != 202002L || \
	defined(_GLIBCXX_DEBUG) || defined(_GLIBCXX_ASSERTIONS) || defined(_GLIBCXX_PARALLEL) || \
	defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__) ||                         \
	(defined(_GLIBCXX_SANITIZE_VECTOR) && _GLIBCXX_SANITIZE_VECTOR != 0)
	(void)output;
	return false;
#else
	if (!output)
		return false;
	constexpr size_t local_source = birth_general_local_source_frames();
	size_t lifetime = 0, current = 0, total = local_source;
	if (!quest_mobile_native_image_lifetime_source_frame_bytes(&lifetime) ||
	    !quest_mobile_native_image_current_heap_source_frame_bytes(&current) ||
	    !birth_compile_add(total, lifetime) || !birth_compile_add(total, current) ||
	    !birth_compile_add(
		    total,
		    birth_fixed_native_mobile_birth_accounting_compile_profile_query_frame_bytes))
		return false;
	*output = total;
	return true;
#endif
}
bool native_mobile_birth_accounting_compile_initial_inline_bytes(size_t *output) noexcept
{
#if !defined(__linux__) || !defined(__x86_64__) || !defined(__LP64__) || defined(_WIN32) ||      \
	!defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 ||                                  \
	!defined(_GLIBCXX_USE_CXX11_ABI) || !_GLIBCXX_USE_CXX11_ABI || __cplusplus != 202002L || \
	defined(_GLIBCXX_DEBUG) || defined(_GLIBCXX_ASSERTIONS) || defined(_GLIBCXX_PARALLEL) || \
	defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__) ||                         \
	(defined(_GLIBCXX_SANITIZE_VECTOR) && _GLIBCXX_SANITIZE_VECTOR != 0)
	(void)output;
	return false;
#else
	if (!output)
		return false;
	*output = sizeof(birth_compile_workspace) + sizeof(general_birth_compile_live);
	return true;
#endif
}

economic_accounting_error native_mobile_birth_accounting_compile_bounded(
	const critical_command &command, const economic_account_key &original_native_wallet,
	economic_accounting_plan *output, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live, size_t *retained_plan_heap_bytes) noexcept
{
	using error = economic_accounting_error;
	if (!output)
		return error::corrupt_evidence;
#if !defined(__linux__) || !defined(__x86_64__) || !defined(__LP64__) || defined(_WIN32) ||      \
	!defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 ||                                  \
	!defined(_GLIBCXX_USE_CXX11_ABI) || !_GLIBCXX_USE_CXX11_ABI || __cplusplus != 202002L || \
	defined(_GLIBCXX_DEBUG) || defined(_GLIBCXX_ASSERTIONS) || defined(_GLIBCXX_PARALLEL) || \
	defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__) ||                         \
	(defined(_GLIBCXX_SANITIZE_VECTOR) && _GLIBCXX_SANITIZE_VECTOR != 0)
	(void)original_native_wallet;
	(void)reserve;
	(void)context;
	(void)outer_live;
	(void)retained_plan_heap_bytes;
	return error::unresolved;
#else
	size_t base = outer_live, source = 0, initial = 0;
	if (!birth_compile_admit(
		    outer_live,
		    birth_fixed_native_mobile_birth_accounting_compile_profile_query_frame_bytes,
		    reserve, context) ||
	    !native_mobile_birth_accounting_compile_own_source_frame_bytes(&source) ||
	    !native_mobile_birth_accounting_compile_initial_inline_bytes(&initial) ||
	    !birth_compile_add(base, source) || !birth_compile_add(base, initial) ||
	    !birth_compile_admit(base, 0, reserve, context))
		return error::capacity;
	try
	{
		birth_compile_workspace work;
		general_birth_compile_live live{ work, base };
		auto &image = work.image;
		auto &intent = work.intent;
		auto &candidate = work.candidate;
		auto &delta = work.delta;
		error status;
		// Genuine general image-only decode owns the complete original version
		// routing, transient recipe/constructor proofs and their destruction.
		size_t child_outer = 0;
		if (!general_birth_child_entry(
			    live, native_mobile_birth_command_general_decode_source_frame_bytes,
			    native_mobile_birth_command_general_decode_initial_inline_bytes,
			    birth_fixed_native_mobile_birth_command_general_decode_query_frame_bytes,
			    reserve, context, child_outer))
			return error::capacity;
		status = native_mobile_birth_command_decode_bounded(
			command, &image, reserve, context, child_outer, &work.image_heap);
		if (status != error::ok)
			return status;
		if (!image.cash || image.cash->revision != 1)
			return error::corrupt_evidence;
		if (image.items.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS ||
		    image.items.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES)
			return error::capacity;
		size_t current = 0;
		if (!live.bytes(current) ||
		    !birth_compile_admit(current, sizeof(std::span<const uint8_t>), reserve,
					 context))
			return error::capacity;
		work.intent_wire = std::span<const uint8_t>(command.accounting_intent);
		if (!general_birth_child_entry(
			    live, economic_intent_decode_source_frame_bytes,
			    economic_intent_decode_initial_inline_bytes,
			    birth_fixed_economic_intent_decode_source_query_frame_bytes, reserve,
			    context, child_outer))
			return error::capacity;
		size_t decode_supplement = 0;
		if (!economic_intent_decode_source_supplement_frame_bytes(&decode_supplement) ||
		    !birth_compile_add(child_outer, decode_supplement) ||
		    !birth_compile_admit(child_outer, 0, reserve, context))
			return error::capacity;
		status = economic_intent_decode_bounded(work.intent_wire, &intent, reserve, context,
							child_outer);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		if (!general_birth_child_entry(
			    live, economic_intent_plan_metadata_fixed_source_frame_bytes,
			    economic_intent_plan_metadata_fixed_initial_inline_bytes,
			    birth_fixed_economic_intent_plan_metadata_fixed_source_query_frame_bytes,
			    reserve, context, child_outer))
			return error::capacity;
		status = economic_intent_plan_metadata_fixed_bounded(
			command, intent, &candidate.metadata, reserve, context, child_outer);
		if (status != error::ok)
			return status;
		if (!economic_account_key_valid(original_native_wallet) ||
		    original_native_wallet.lineage.bytes != candidate.metadata.lineage.bytes ||
		    original_native_wallet.kind != economic_account_kind::wallet ||
		    original_native_wallet.context_id != ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT)
			return error::invalid_identity;
		const auto &cash = image.cash->denominations.amount;
		// Zero argument temporary and economic_coin_delta's own result coexist;
		// the output delta is already a named workspace object.
		if (!live.bytes(current) ||
		    !birth_compile_admit(current, 2 * sizeof(economic_coin_vector), reserve,
					 context))
			return error::capacity;
		status = economic_coin_delta({}, cash, &delta);
		if (status != error::ok)
			return status;
		size_t extra = 0;
		if (!live.bytes(current) ||
		    !birth_compile_account_push(candidate.accounts.size(),
						candidate.accounts.capacity(), extra) ||
		    !birth_compile_admit(current, extra, reserve, context))
			return error::capacity;
		candidate.accounts.push_back(
			{ original_native_wallet, {}, cash, 0, image.cash->revision });
		if (std::any_of(delta.begin(), delta.end(), [](int64_t part) { return part != 0; }))
		{
			if (!live.bytes(current) ||
			    !birth_compile_admit(current, 3 * sizeof(economic_coin_vector), reserve,
						 context))
				return error::capacity;
			economic_coin_vector opposite{};
			// The admitted local opposite survives the zero argument/callee result.
			status = economic_coin_delta(cash, {}, &opposite);
			if (status != error::ok)
				return status;
			int64_t copper = 0;
			status = economic_coin_value(delta, &copper);
			if (status != error::ok)
				return status;
			if (!live.bytes(current) || !birth_compile_add(current, sizeof(opposite)) ||
			    !birth_compile_account_push(candidate.accounts.size(),
							candidate.accounts.capacity(), extra) ||
			    !birth_compile_admit(current, extra, reserve, context))
				return error::capacity;
			candidate.accounts.push_back({ { candidate.metadata.lineage,
							 economic_account_kind::issuance, 1, 0 },
						       {},
						       {},
						       0,
						       0 });
			if (!live.bytes(current) || !birth_compile_add(current, sizeof(opposite)))
				return error::capacity;
			extra = sizeof(std::initializer_list<economic_coin_posting>) +
				sizeof(std::array<economic_coin_posting, 2>);
			if (!birth_compile_rows(extra, 2, sizeof(economic_coin_posting)) ||
			    !birth_compile_admit(current, extra, reserve, context))
				return error::capacity;
			candidate.postings = { { 0, 0, 0, delta, copper },
					       { 1, 1, 0, opposite, -copper } };
		}
		size_t item_requests = 0;
		const size_t item_temporary =
			image.items.empty() ?
				size_t{ 0 } :
				sizeof(economic_item_position) +
					std::max(sizeof(item_owner_identity),
						 std::max(sizeof(economic_item_snapshot),
							  sizeof(economic_item_event)));
		if (!birth_compile_rows(item_requests, image.items.size(),
					2 * sizeof(economic_item_snapshot) +
						sizeof(economic_item_event)) ||
		    !birth_compile_add(item_requests, item_temporary) || !live.bytes(current) ||
		    !birth_compile_admit(current, item_requests, reserve, context))
			return error::capacity;
		candidate.items_before.reserve(image.items.size());
		candidate.items_after.reserve(image.items.size());
		candidate.item_events.reserve(image.items.size());
		for (size_t index = 0; index < image.items.size(); ++index)
		{
			const auto &item = image.items[index];
			economic_item_position after{};
			after.owner = { item_owner_type::native_mobile,
					image.reference.mobile_instance_id, 0 };
			after.root_uid = item.object_uid;
			after.revision = 1;
			after.state = item_custody_state::active;
			after.equipment_slot = static_cast<uint16_t>(item.equipment_slot);
			if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			{
				if (item.parent_index < 0 ||
				    static_cast<size_t>(item.parent_index) >= index ||
				    item.equipment_slot)
					return error::topology;
				const size_t parent = static_cast<size_t>(item.parent_index);
				after.root_uid = candidate.items_after[parent].position.root_uid;
				after.parent_uid = image.items[parent].object_uid;
			}
			candidate.items_before.push_back({ item.object_uid, {} });
			candidate.items_after.push_back({ item.object_uid, after });
			candidate.item_events.push_back(
				{ static_cast<uint32_t>(index), 0, item.object_uid, {}, after });
		}
		if (!live.bytes(current))
			return error::capacity;
		if (!general_birth_child_entry(
			    live, economic_plan_normalize_source_frame_bytes,
			    economic_plan_normalize_initial_inline_bytes,
			    birth_fixed_economic_plan_bounded_profile_query_frame_bytes, reserve,
			    context, child_outer))
			return error::capacity;
		status = economic_plan_normalize_bounded(&candidate, reserve, context, child_outer);
		if (status != error::ok)
			return status;
		size_t retained = 0;
		if (!birth_compile_plan_heap(candidate, retained))
			return error::capacity;
		static_assert(std::is_nothrow_move_assignable_v<economic_accounting_plan>);
		*output = std::move(candidate);
		if (retained_plan_heap_bytes)
			*retained_plan_heap_bytes = retained;
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
#endif
}
