#include "item/lockpick_retirement_continuation.h"
#include "item/item_transfer_command.h"
#include "player/player_snapshot_codec.h"
#include "core/defines.h"
#include <climits>
#include <new>

namespace
{
bool valid(const lockpick_retirement_terms &terms) noexcept
{
	return terms.branch >= lockpick_retirement_branch::failed_container &&
	       terms.branch <= lockpick_retirement_branch::wear && terms.item_uid &&
	       terms.item_uid != UINT64_MAX && terms.actor_pid && terms.actor_pid <= INT32_MAX &&
	       terms.item_vnum > 0;
}
uint64_t read_le(std::span<const uint8_t> bytes, size_t offset, size_t count) noexcept
{
	uint64_t value = 0;
	for (size_t index = 0; index < count; ++index)
		value |= static_cast<uint64_t>(bytes[offset + index]) << (index * 8);
	return value;
}
void write_le(std::vector<uint8_t> &bytes, size_t offset, size_t count, uint64_t value) noexcept
{
	for (size_t index = 0; index < count; ++index)
		bytes[offset + index] = static_cast<uint8_t>(value >> (index * 8));
}
}

bool lockpick_retirement_encode(const lockpick_retirement_terms &terms,
				std::vector<uint8_t> *output) noexcept
try
{
	if (!output || !valid(terms))
		return false;
	std::vector<uint8_t> encoded(LOCKPICK_RETIREMENT_CONTINUATION_BYTES, 0);
	encoded[0] = 1;
	encoded[1] = static_cast<uint8_t>(terms.branch);
	write_le(encoded, 4, 8, terms.item_uid);
	write_le(encoded, 12, 4, terms.actor_pid);
	write_le(encoded, 16, 4, static_cast<uint32_t>(terms.item_vnum));
	*output = std::move(encoded);
	return true;
}
catch (const std::bad_alloc &)
{
	return false;
}

bool lockpick_retirement_decode(std::span<const uint8_t> bytes,
				lockpick_retirement_terms *output) noexcept
{
	if (!output || bytes.size() != LOCKPICK_RETIREMENT_CONTINUATION_BYTES || bytes[0] != 1 ||
	    bytes[2] || bytes[3])
		return false;
	lockpick_retirement_terms decoded;
	decoded.branch = static_cast<lockpick_retirement_branch>(bytes[1]);
	decoded.item_uid = read_le(bytes, 4, 8);
	decoded.actor_pid = static_cast<uint32_t>(read_le(bytes, 12, 4));
	const auto vnum = read_le(bytes, 16, 4);
	if (vnum > INT32_MAX)
		return false;
	decoded.item_vnum = static_cast<int32_t>(vnum);
	if (!valid(decoded))
		return false;
	*output = decoded;
	return true;
}

bool lockpick_retirement_payload_valid(const item_transfer_payload &payload) noexcept
try
{
	lockpick_retirement_terms terms;
	if (!lockpick_retirement_decode(payload.continuation.data, &terms) ||
	    payload.reason != item_transfer_reason::destruction || payload.multi_root ||
	    payload.item_count != 1 || payload.from_owner.type != item_owner_type::player ||
	    payload.from_owner.id != terms.actor_pid || payload.from_owner.context_id ||
	    payload.to_owner.type != item_owner_type::destruction || payload.to_owner.id ||
	    payload.to_owner.context_id || payload.selected_item_uid != terms.item_uid ||
	    payload.reason_id != terms.item_vnum || payload.logical_source_id ||
	    payload.target_root_item_uid || payload.target_parent_item_uid ||
	    payload.expected_target_parent_revision || payload.corpse.present ||
	    payload.collector.present || payload.native_mobile.present ||
	    payload.native_recovery.present || !payload.item_blob_size ||
	    payload.item_blob_size > payload.item_blob.size())
		return false;
	const auto &item = payload.items[0];
	if (item.item_uid != terms.item_uid || item.root_item_uid != terms.item_uid ||
	    item.parent_item_uid || item.vnum != terms.item_vnum ||
	    item.expected_state != item_custody_state::active || !item.expected_item_revision ||
	    item.expected_item_revision == UINT64_MAX)
		return false;
	std::vector<player_item_snapshot> literal;
	if (player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					     &literal) != player_snapshot_codec_result::ok ||
	    literal.size() != 1)
		return false;
	const auto &pick = literal.front();
	return pick.object_uid == terms.item_uid && pick.vnum == terms.item_vnum &&
	       pick.type == ITEM_PICK && pick.equipment_slot == HOLD + 1 &&
	       pick.parent_index == PLAYER_SNAPSHOT_NO_PARENT;
}
catch (...)
{
	return false;
}
namespace
{
constexpr size_t lockpick_bound_allocator_frames =
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

// Actual nontrivial row/description _Destroy/aux/destroy_at/addressof chain,
// member string destructors/dispose/destroy and nested vector deallocation.
constexpr size_t lockpick_bound_destructor_frames =
	8 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(void *) +
	6 * (5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) +
	3 * lockpick_bound_allocator_frames;

bool lockpick_bound_add(size_t &bytes, size_t extra) noexcept
{
	if (extra > SIZE_MAX - bytes)
		return false;
	bytes += extra;
	return true;
}
bool lockpick_bound_peak(bool (*reserve)(size_t, void *) noexcept, void *context,
			 size_t caller_live, size_t heap) noexcept
{
	constexpr size_t own = 3 * sizeof(void *) + 3 * sizeof(size_t) + 2 * sizeof(bool);
	return lockpick_bound_add(caller_live, heap) && lockpick_bound_add(caller_live, own) &&
	       reserve && reserve(caller_live, context);
}
} // namespace

bool lockpick_retirement_payload_valid_bounded(const item_transfer_payload &payload,
					       bool (*reserve)(size_t, void *) noexcept,
					       void *context, size_t outer) noexcept
try
{
	// Original fixed typed20 decoder has no allocation; source scalars,
	// returned span, fixed decoded terms and real vector default constructor.
	constexpr size_t fixed =
		sizeof(lockpick_retirement_terms) + sizeof(std::vector<player_item_snapshot>) +
		6 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) +
		sizeof(lockpick_retirement_terms) + sizeof(std::span<const uint8_t>) +
		5 * sizeof(void *) + 3 * sizeof(size_t) + 2 * sizeof(uint64_t) + sizeof(bool) +
		// Span pointer/count/extent/to_address, span/array/vector queries,
		// default vector/base/impl/data/allocator construction.
		7 * sizeof(void *) + 3 * sizeof(size_t) + 8 * (sizeof(void *) + sizeof(size_t)) +
		6 * sizeof(void *) + lockpick_bound_destructor_frames;
	size_t caller_live = outer;
	if (!lockpick_bound_add(caller_live, fixed) ||
	    !lockpick_bound_peak(reserve, context, caller_live, 0))
		return false;
	lockpick_retirement_terms terms;
	if (!lockpick_retirement_decode(std::span<const uint8_t>(payload.continuation.data.data(),
								 payload.continuation.data.size()),
					&terms) ||
	    payload.reason != item_transfer_reason::destruction || payload.multi_root ||
	    payload.item_count != 1 || payload.from_owner.type != item_owner_type::player ||
	    payload.from_owner.id != terms.actor_pid || payload.from_owner.context_id ||
	    payload.to_owner.type != item_owner_type::destruction || payload.to_owner.id ||
	    payload.to_owner.context_id || payload.selected_item_uid != terms.item_uid ||
	    payload.reason_id != terms.item_vnum || payload.logical_source_id ||
	    payload.target_root_item_uid || payload.target_parent_item_uid ||
	    payload.expected_target_parent_revision || payload.corpse.present ||
	    payload.collector.present || payload.native_mobile.present ||
	    payload.native_recovery.present || !payload.item_blob_size ||
	    payload.item_blob_size > payload.item_blob.size())
		return false;
	const auto &item = payload.items[0];
	if (item.item_uid != terms.item_uid || item.root_item_uid != terms.item_uid ||
	    item.parent_item_uid || item.vnum != terms.item_vnum ||
	    item.expected_state != item_custody_state::active || !item.expected_item_revision ||
	    item.expected_item_revision == UINT64_MAX)
		return false;
	std::vector<player_item_snapshot> literal;
	size_t literal_heap = 0;
	if (player_item_snapshot_list_decode_bounded(
		    payload.item_blob.data(), payload.item_blob_size, &literal, reserve, context,
		    caller_live, &literal_heap) != player_snapshot_codec_result::ok ||
	    literal.size() != 1)
		return false;
	// The genuine decoder reports actual capacity/string/nested heaps after
	// the complete nonallocating transfer into this real unchanged local.
	if (!lockpick_bound_peak(reserve, context, caller_live, literal_heap))
		return false;
	const auto &pick = literal.front();
	return pick.object_uid == terms.item_uid && pick.vnum == terms.item_vnum &&
	       pick.type == ITEM_PICK && pick.equipment_slot == HOLD + 1 &&
	       pick.parent_index == PLAYER_SNAPSHOT_NO_PARENT;
}
catch (...)
{
	return false;
}

#include <type_traits>

namespace
{
template <class T> constexpr size_t lockpick_complete_vector_deallocate_source =
	// _M_deallocate, allocator_traits::deallocate, allocator::deallocate,
	// new_allocator::deallocate each owns this/alloc, pointer, count.
	4 * (2 * sizeof(void *) + sizeof(size_t)) +
	// Actual sized operator delete pointer/count. Const-evaluation alternate
	// unsized delete is not selected; single helper bool is actual query.
	sizeof(void *) + sizeof(size_t) + sizeof(bool);
template <class T> constexpr size_t lockpick_complete_vector_cleanup_source =
	// Actual vector, Vector_base, generated nontrivial Vector_impl and
	// allocator destructor receivers. Impl_data has no declared destructor;
	// new_allocator destructor is excluded by the C++20 preprocessor path.
	// Neither contributes a runtime selected destructor scope.
	4 * sizeof(void *) +
	// _Destroy(first,last,allocator), _Destroy(first,last) and selected aux:
	// no invented cursor local: aux<false> increments its first parameter.
	3 * sizeof(void *) + 2 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) +
	// Mutable get_Tp_allocator receiver/reference result.
	2 * sizeof(void *) + lockpick_complete_vector_deallocate_source<T> +
	(std::is_trivially_destructible_v<T> ?
		 0 :
		 // Nontrivial aux reaches __addressof, _Destroy(pointer), destroy_at
		 // and the genuine T destructor receiver. Member cleanup is separate;
		 // raw-pointer ++ and == use no iterator object/helper family.
		 2 * sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(void *));
constexpr size_t lockpick_complete_member_string_cleanup_source =
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

constexpr size_t lockpick_complete_literal_cleanup_source =
	// Four exact reached vector element types. Row/ED destructor receivers
	// are in their nontrivial vector backend; no row copy/default/move lane.
	lockpick_complete_vector_cleanup_source<player_item_snapshot> +
	lockpick_complete_vector_cleanup_source<player_item_dynamic_affect_snapshot> +
	lockpick_complete_vector_cleanup_source<player_item_extra_description_snapshot> +
	lockpick_complete_vector_cleanup_source<int32_t> +
	// Actual same string destructor family across row and nested ED fields.
	// Physical strings, rows and all capacities belong to the literal heap.
	lockpick_complete_member_string_cleanup_source;

constexpr size_t lockpick_complete_fixed_decode_source =
	// Original decode: by-value span, output, vnum and bool result. Its
	// real decoded DTO is private workspace, charged separately below.
	sizeof(std::span<const uint8_t>) + sizeof(void *) + sizeof(uint64_t) + sizeof(bool) +
	// read_le: by-value span, offset/count/index, value and scalar return.
	sizeof(std::span<const uint8_t>) + 3 * sizeof(size_t) + 2 * sizeof(uint64_t) +
	// valid terms reference/result; actual DMI-generated terms default ctor.
	sizeof(void *) + sizeof(bool) + sizeof(void *) +
	// Explicit defaulted span copy has receiver/source2P. Dynamic extent
	// has no declared copy callee. The read_le lvalue copy is reached;
	// caller prvalue->decode elides. Preserve explicit span destruction.
	2 * sizeof(void *) + sizeof(void *) + // explicit span destructor receiver
	// span::size and dynamic extent getter, plus span::operator[] this,
	// index/reference result and real ordinary constexpr-assert query bool.
	2 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(bool);

constexpr size_t lockpick_complete_list_child_own_source =
	// data/output/context/retained4P + reserveF; size/outer/query/source/
	// initial/supplement/request/child_outer8N and actual enum result.
	4 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 8 * sizeof(size_t) +
	sizeof(player_snapshot_codec_result);

constexpr size_t lockpick_complete_own_source =
	// New root payload/context/item/pick refs, reserveF, outer/fixed/
	// caller_live/literal_heap4N, bool return and support-helper bool.
	4 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 4 * sizeof(size_t) +
	2 * sizeof(bool) + lockpick_complete_fixed_decode_source +
	lockpick_complete_list_child_own_source +
	// Actual three published child getter declarations; scalar outputs are
	// already the named adapter locals. No mandatory constexpr runtime call.
	3 * player_item_snapshot_list_decode_bounded_source_query_frame_bytes() +
	// Unchanged peak helper reserveF/contextP/caller_live/heap/own3N/B,
	// checked-add reference/value/result, and callback declaration P/N/B.
	sizeof(bool (*)(size_t, void *) noexcept) + sizeof(void *) + 3 * sizeof(size_t) +
	sizeof(bool) + 2 * (sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	// Continuation const byte vector data+selected raw _M_data_ptr5P,
	// and size P+N. Its accessor family does not cover item_blob.
	5 * sizeof(void *) + sizeof(void *) + sizeof(size_t) +
	// Actual const fixed byte-array item_blob data receiver/result2P
	// and size receiver/resultP+N, distinct from the continuation vector.
	3 * sizeof(void *) + sizeof(size_t) +
	// Pointer/count span: ctor2P+N, extent ctorP+N, public to_address and
	// raw __to_address each pointer argument/result2P. Formal span above
	// is the guaranteed-elided result object, not a second temporary.
	7 * sizeof(void *) + 2 * sizeof(size_t) +
	// Actual const payload item-array index this/index/reference result.
	2 * sizeof(void *) + sizeof(size_t) +
	// Literal vector size P+N; mutable front/begin/pointer-ctor/dereference
	// seven declared pointer scopes plus the actual returned iterator.
	// Stored pointer binds directly, with no conversion temporary.
	sizeof(void *) + sizeof(size_t) + 7 * sizeof(void *) +
	sizeof(std::vector<player_item_snapshot>::iterator) +
	// Actual empty vector/base/impl/data/allocator/newallocator ctor receivers.
	6 * sizeof(void *) + lockpick_complete_literal_cleanup_source;

constexpr size_t lockpick_complete_workspace =
	// Root terms/literal and original fixed decoder's independent decoded
	// terms. All are constructed only after the new root's first admission;
	// sequential nonoverlapping DTOs are conservatively retained, no credit.
	2 * sizeof(lockpick_retirement_terms) + sizeof(std::vector<player_item_snapshot>);

bool lockpick_complete_policy() noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&  \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) && \
	!defined(_GLIBCXX_PARALLEL) && !defined(_GLIBCXX_PARALLEL_ASSERTIONS)
	return sizeof(void *) == 8 && sizeof(size_t) == 8 && sizeof(std::ptrdiff_t) == 8 &&
	       sizeof(std::span<const uint8_t>) == 2 * sizeof(void *) &&
	       sizeof(std::vector<player_item_snapshot>) == 3 * sizeof(void *) &&
	       sizeof(std::vector<player_item_snapshot>::iterator) == sizeof(void *) &&
	       sizeof(std::allocator<player_item_snapshot>) == 1 &&
	       sizeof(std::allocator<player_item_dynamic_affect_snapshot>) == 1 &&
	       sizeof(std::allocator<player_item_extra_description_snapshot>) == 1 &&
	       sizeof(std::allocator<int32_t>) == 1 && sizeof(std::allocator<char>) == 1;
#else
	return false;
#endif
}
player_snapshot_codec_result
lockpick_complete_list_decode_child(const uint8_t *data, size_t size,
				    std::vector<player_item_snapshot> *output,
				    bool (*reserve)(size_t, void *) noexcept, void *context,
				    size_t outer, size_t *retained_item_heap_bytes) noexcept
{
	constexpr size_t query =
		3 * player_item_snapshot_list_decode_bounded_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer;
	if (!reserve || !lockpick_bound_add(request, query) || !reserve(request, context) ||
	    !player_item_snapshot_list_decode_bounded_source_frame_bytes(&source) ||
	    !player_item_snapshot_list_decode_bounded_initial_inline_bytes(&initial) ||
	    !player_item_snapshot_list_decode_bounded_source_supplement_frame_bytes(&supplement))
		return player_snapshot_codec_result::limit_exceeded;
	request = outer;
	if (!lockpick_bound_add(request, source) || !lockpick_bound_add(request, initial) ||
	    !reserve(request, context))
		return player_snapshot_codec_result::limit_exceeded;
	size_t child_outer = outer;
	if (!lockpick_bound_add(child_outer, supplement))
		return player_snapshot_codec_result::limit_exceeded;
	return player_item_snapshot_list_decode_bounded(data, size, output, reserve, context,
							child_outer, retained_item_heap_bytes);
}

} // namespace

bool lockpick_retirement_payload_valid_complete_bounded(const item_transfer_payload &payload,
							bool (*reserve)(size_t, void *) noexcept,
							void *context, size_t outer) noexcept
try
{
	if (!lockpick_complete_policy())
		return false;
	constexpr size_t fixed = lockpick_complete_own_source + lockpick_complete_workspace;
	size_t caller_live = outer;
	if (!lockpick_bound_add(caller_live, fixed) ||
	    !lockpick_bound_peak(reserve, context, caller_live, 0))
		return false;
	lockpick_retirement_terms terms;
	if (!lockpick_retirement_decode(std::span<const uint8_t>(payload.continuation.data.data(),
								 payload.continuation.data.size()),
					&terms) ||
	    payload.reason != item_transfer_reason::destruction || payload.multi_root ||
	    payload.item_count != 1 || payload.from_owner.type != item_owner_type::player ||
	    payload.from_owner.id != terms.actor_pid || payload.from_owner.context_id ||
	    payload.to_owner.type != item_owner_type::destruction || payload.to_owner.id ||
	    payload.to_owner.context_id || payload.selected_item_uid != terms.item_uid ||
	    payload.reason_id != terms.item_vnum || payload.logical_source_id ||
	    payload.target_root_item_uid || payload.target_parent_item_uid ||
	    payload.expected_target_parent_revision || payload.corpse.present ||
	    payload.collector.present || payload.native_mobile.present ||
	    payload.native_recovery.present || !payload.item_blob_size ||
	    payload.item_blob_size > payload.item_blob.size())
		return false;
	const auto &item = payload.items[0];
	if (item.item_uid != terms.item_uid || item.root_item_uid != terms.item_uid ||
	    item.parent_item_uid || item.vnum != terms.item_vnum ||
	    item.expected_state != item_custody_state::active || !item.expected_item_revision ||
	    item.expected_item_revision == UINT64_MAX)
		return false;
	std::vector<player_item_snapshot> literal;
	size_t literal_heap = 0;
	if (lockpick_complete_list_decode_child(
		    payload.item_blob.data(), payload.item_blob_size, &literal, reserve, context,
		    caller_live, &literal_heap) != player_snapshot_codec_result::ok ||
	    literal.size() != 1)
		return false;
	// The genuine decoder reports actual capacity/string/nested heaps after
	// the complete nonallocating transfer into this real unchanged local.
	if (!lockpick_bound_peak(reserve, context, caller_live, literal_heap))
		return false;
	const auto &pick = literal.front();
	return pick.object_uid == terms.item_uid && pick.vnum == terms.item_vnum &&
	       pick.type == ITEM_PICK && pick.equipment_slot == HOLD + 1 &&
	       pick.parent_index == PLAYER_SNAPSHOT_NO_PARENT;
}
catch (...)
{
	return false;
}

bool lockpick_retirement_payload_valid_complete_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !lockpick_complete_policy())
		return false;
	size_t child = 0;
	if (!player_item_snapshot_list_decode_bounded_source_frame_bytes(&child))
		return false;
	size_t total = lockpick_complete_own_source;
	if (!lockpick_bound_add(total, child))
		return false;
	*output = total;
	return true;
}
bool lockpick_retirement_payload_valid_complete_source_supplement_frame_bytes(
	size_t *output) noexcept
{
	if (!output || !lockpick_complete_policy())
		return false;
	// New root admits its complete own Source before constructing workspace;
	// local adapter owns transient full/initial and retains actual child supp.
	*output = 0;
	return true;
}
bool lockpick_retirement_payload_valid_complete_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !lockpick_complete_policy())
		return false;
	// First root reserve precedes terms/literal/fixed-decoder construction.
	*output = 0;
	return true;
}
