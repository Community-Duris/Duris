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
