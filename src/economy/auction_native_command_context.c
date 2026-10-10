#include "economy/auction_native_command_context.h"

#include "core/structs.h"

#include <algorithm>
#include <new>
#include <openssl/sha.h>
#include <type_traits>
#include <unordered_set>
#include <utility>

// Implemented by the separate original retained/publication owner. This narrow
// declaration avoids pulling its private world/staging header into the codec.
bool auction_native_expected_player_forest(const auction_command_payload &,
					   std::span<const player_item_snapshot>,
					   std::span<const player_item_snapshot>, bool rejected,
					   uint32_t original_actor_level,
					   std::vector<player_item_snapshot> *) noexcept;

namespace
{
using error = economic_accounting_error;
constexpr uint32_t trailer_magic = 0x32434e41; // ANC2
constexpr uint32_t footer_magic = 0x32454e41; // ANE2
constexpr size_t trailer_header_bytes = 128; // Fixed fields plus BEFORE UID count.
constexpr size_t footer_bytes = 12;

template <typename T> void append(std::vector<uint8_t> &bytes, T value)
{
	using U = std::make_unsigned_t<T>;
	for (size_t i = 0; i < sizeof(T); ++i)
		bytes.push_back(static_cast<uint8_t>(static_cast<U>(value) >> (8 * i)));
}
template <typename T> bool read(std::span<const uint8_t> bytes, size_t &offset, T &value)
{
	if (offset > bytes.size() || sizeof(T) > bytes.size() - offset)
		return false;
	using U = std::make_unsigned_t<T>;
	U result = 0;
	for (size_t i = 0; i < sizeof(T); ++i)
		result |= static_cast<U>(bytes[offset++]) << (8 * i);
	value = static_cast<T>(result);
	return true;
}

error codec_error(player_snapshot_codec_result result) noexcept
{
	if (result == player_snapshot_codec_result::ok)
		return error::ok;
	return result == player_snapshot_codec_result::allocation_failure ||
			       result == player_snapshot_codec_result::limit_exceeded ?
		       error::capacity :
		       error::corrupt_evidence;
}

// Bound caller-owned nested values before copying them into the original codec.
// Original saved string policy and contiguous equipment/inventory DFS are kept;
// the generic saved-item codec alone permits repeated UIDs and non-DFS order.
error canonical(std::span<const player_item_snapshot> items, std::vector<uint8_t> &bytes,
		bool full_literal = false)
{
	if (items.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return error::capacity;
	size_t rows = items.size(), text_bytes = 0;
	auto text = [&](const std::string &value)
	{
		if (value.size() > PLAYER_SNAPSHOT_MAX_STRING_BYTES ||
		    value.size() > PLAYER_SNAPSHOT_MAX_BYTES - text_bytes ||
		    value.find('\0') != std::string::npos)
			return false;
		text_bytes += value.size();
		return true;
	};
	std::unordered_set<uint64_t> identities;
	std::array<size_t, PLAYER_SNAPSHOT_MAX_DEPTH> ancestors{};
	size_t depth = 0;
	int16_t last_equipment = 0;
	bool inventory = false;
	for (size_t index = 0; index < items.size(); ++index)
	{
		const auto &item = items[index];
		if (!item.object_uid || item.object_uid == UINT64_MAX || item.vnum < 0 ||
		    item.string_mask > 15 || (full_literal && item.string_mask != 15) ||
		    !identities.insert(item.object_uid).second || !text(item.name) ||
		    !text(item.short_description) || !text(item.description) ||
		    !text(item.action_description))
			return error::corrupt_evidence;
		if (item.dynamic_affects.size() > PLAYER_SNAPSHOT_MAX_ROWS - rows)
			return error::capacity;
		rows += item.dynamic_affects.size();
		if (item.extra_descriptions.size() > PLAYER_SNAPSHOT_MAX_ROWS - rows)
			return error::capacity;
		rows += item.extra_descriptions.size();
		for (const auto &extra : item.extra_descriptions)
		{
			if (!text(extra.keyword) || !text(extra.description))
				return error::corrupt_evidence;
			if (extra.spell_ids.size() > PLAYER_SNAPSHOT_MAX_ROWS - rows)
				return error::capacity;
			rows += extra.spell_ids.size();
		}
		if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
		{
			if (item.equipment_slot < 0 || item.equipment_slot > MAX_WEAR ||
			    (item.equipment_slot &&
			     (inventory || item.equipment_slot <= last_equipment)))
				return error::topology;
			if (item.equipment_slot)
				last_equipment = item.equipment_slot;
			else
				inventory = true;
			depth = 1;
			ancestors[0] = index;
		}
		else
		{
			if (item.parent_index < 0 ||
			    item.parent_index >= static_cast<int32_t>(index) || item.equipment_slot)
				return error::topology;
			while (depth &&
			       ancestors[depth - 1] != static_cast<size_t>(item.parent_index))
				--depth;
			if (!depth || depth == ancestors.size())
				return error::topology;
			ancestors[depth++] = index;
		}
	}
	return codec_error(player_item_snapshot_list_encode(
		std::vector<player_item_snapshot>(items.begin(), items.end()), &bytes));
}

bool digest(std::span<const uint8_t> bytes, std::array<uint8_t, 32> &output) noexcept
{
	return SHA256(bytes.data(), bytes.size(), output.data()) != nullptr;
}

error shape(const auction_command_payload &payload, std::span<const player_item_snapshot> selected,
	    uint32_t level, uint64_t revision)
{
	const bool background = payload.action == auction_action::finalize ||
				payload.action == auction_action::remove;
	if (background)
		return !payload.actor_pid && !level && !revision && selected.empty() &&
				       !payload.item_count ?
			       error::ok :
			       error::invalid_identity;
	if (!payload.actor_pid || !level || level > 255 || !revision)
		return error::invalid_identity;
	const bool item_action = payload.action == auction_action::list ||
				 payload.action == auction_action::claim_item;
	if (!item_action)
		return (payload.action == auction_action::bid ||
			payload.action == auction_action::claim_money) &&
				       selected.empty() && !payload.item_count ?
			       error::ok :
			       error::invalid_identity;
	if (selected.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return error::capacity;
	if (selected.empty() || !payload.item_count ||
	    payload.item_count > AUCTION_COMMAND_MAX_ITEMS)
		return error::invalid_identity;
	size_t root = 0;
	for (size_t index = 0; index < selected.size(); ++index)
	{
		const auto &item = selected[index];
		if (item.equipment_slot)
			return error::invalid_identity;
		if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
		{
			if (root >= payload.item_count ||
			    item.object_uid != payload.items[root].item_uid ||
			    item.vnum != payload.items[root].vnum || item.vnum != selected[0].vnum)
				return error::invalid_identity;
			++root;
		}
	}
	return root == payload.item_count ? error::ok : error::invalid_identity;
}

bool decode_base(const critical_command &command, std::span<const uint8_t> bytes,
		 auction_command_payload &payload)
{
	critical_command original = command;
	original.payload_version = AUCTION_COMMAND_PAYLOAD_VERSION;
	original.payload.assign(bytes.begin(), bytes.end());
	std::vector<uint8_t> roundtrip;
	return (command.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION ?
			auction_command_decode_native_base(command, bytes, &payload) :
			auction_command_decode_payload(original, &payload)) &&
	       auction_command_encode_payload(payload, &roundtrip) &&
	       roundtrip.size() == bytes.size() &&
	       std::equal(roundtrip.begin(), roundtrip.end(), bytes.begin());
}

bool envelope_shape_valid(const critical_command &command)
{
	// Original envelope validation requires the coordinator's nonzero admission
	// time. A local projection checks its structural bounds for fresh preparation;
	// neither the caller's timestamp nor admission authority is changed.
	critical_command projection = command;
	if (!projection.accepted_at_usec)
		projection.accepted_at_usec = 1;
	return critical_command_envelope_valid(projection);
}
} // namespace

economic_accounting_error
auction_native_command_bind(critical_command *command, std::span<const player_item_snapshot> before,
			    std::span<const player_item_snapshot> after,
			    std::span<const player_item_snapshot> selected_literals,
			    uint32_t original_level, uint64_t acknowledged_save_revision) noexcept
{
	if (!command || command->schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    command->payload_version != AUCTION_COMMAND_PAYLOAD_VERSION ||
	    command->accepted_at_usec || !command->accounting_intent.empty() ||
	    command->publication_required)
		return error::invalid_version;
	try
	{
		if (!envelope_shape_valid(*command))
			return error::corrupt_evidence;
		auction_command_payload payload{};
		if (!decode_base(*command, command->payload, payload))
			return error::corrupt_evidence;
		const auto shaped = shape(payload, selected_literals, original_level,
					  acknowledged_save_revision);
		if (shaped != error::ok)
			return shaped;
		std::vector<uint8_t> before_bytes, after_bytes, selected_bytes;
		for (const auto &[items, bytes] :
		     { std::pair{ before, &before_bytes }, std::pair{ after, &after_bytes },
		       std::pair{ selected_literals, &selected_bytes } })
		{
			const auto result = canonical(items, *bytes, bytes == &selected_bytes);
			if (result != error::ok)
				return result;
		}
		if (!payload.actor_pid && (!before.empty() || !after.empty()))
			return error::invalid_identity;
		if (payload.action == auction_action::list)
			for (size_t start = 0; start < selected_literals.size();)
			{
				const auto &selected = selected_literals[start];
				const auto found = std::find_if(
					before.begin(), before.end(), [&](const auto &item)
					{ return item.object_uid == selected.object_uid; });
				if (found == before.end() ||
				    found->parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
				    found->equipment_slot)
					return error::topology;
				size_t end = start + 1;
				while (end < selected_literals.size() &&
				       selected_literals[end].parent_index !=
					       PLAYER_SNAPSHOT_NO_PARENT)
					++end;
				std::vector<player_item_snapshot> actual_tree, remaining;
				const auto extracted = player_item_snapshot_extract_subtree(
					std::vector<player_item_snapshot>(before.begin(),
									  before.end()),
					selected.object_uid, &actual_tree, &remaining);
				if (extracted != player_snapshot_codec_result::ok)
					return codec_error(extracted);
				std::vector<player_item_snapshot> supplied_tree(
					selected_literals.begin() + start,
					selected_literals.begin() + end);
				for (auto &item : supplied_tree)
					if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
						item.parent_index -= static_cast<int32_t>(start);
				std::vector<uint8_t> actual_bytes, supplied_bytes;
				// Root's acknowledged capture retains every selected subtree in
				// full. Compare exact complete images; omitted strings are never
				// inferred from a later template or another selected root.
				if (canonical(actual_tree, actual_bytes, true) != error::ok ||
				    canonical(supplied_tree, supplied_bytes, true) != error::ok ||
				    actual_bytes != supplied_bytes)
					return error::payload_conflict;
				start = end;
			}
		std::vector<player_item_snapshot> expected_after;
		std::vector<uint8_t> expected_bytes;
		if (!auction_native_expected_player_forest(payload, before, selected_literals,
							   false, original_level,
							   &expected_after) ||
		    canonical(expected_after, expected_bytes) != error::ok ||
		    expected_bytes != after_bytes)
			return error::topology;
		std::array<uint8_t, 32> before_digest{}, after_digest{}, selected_digest{};
		if (!digest(before_bytes, before_digest) || !digest(after_bytes, after_digest) ||
		    !digest(selected_bytes, selected_digest))
			return error::unresolved;
		const auto selected_nodes = static_cast<uint32_t>(selected_literals.size());
		const auto selected_roots = static_cast<uint16_t>(std::count_if(
			selected_literals.begin(), selected_literals.end(), [](const auto &item)
			{ return item.parent_index == PLAYER_SNAPSHOT_NO_PARENT; }));
		const auto base_size = command->payload.size();
		const auto trailer_size = trailer_header_bytes + before.size() * sizeof(uint64_t);
		if (base_size > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - footer_bytes ||
		    trailer_size > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - footer_bytes - base_size)
			return error::capacity;
		critical_command candidate = *command;
		candidate.payload.reserve(base_size + trailer_size + footer_bytes);
		append(candidate.payload, trailer_magic);
		append(candidate.payload, AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION);
		append(candidate.payload, uint16_t{ 0 });
		append(candidate.payload, original_level);
		append(candidate.payload, acknowledged_save_revision);
		candidate.payload.insert(candidate.payload.end(), before_digest.begin(),
					 before_digest.end());
		candidate.payload.insert(candidate.payload.end(), after_digest.begin(),
					 after_digest.end());
		candidate.payload.insert(candidate.payload.end(), selected_digest.begin(),
					 selected_digest.end());
		append(candidate.payload, selected_nodes);
		append(candidate.payload, selected_roots);
		append(candidate.payload, uint16_t{ 0 });
		append(candidate.payload, static_cast<uint32_t>(before.size()));
		for (const auto &item : before)
			append(candidate.payload, item.object_uid);
		append(candidate.payload, static_cast<uint32_t>(base_size));
		append(candidate.payload, static_cast<uint32_t>(trailer_size));
		append(candidate.payload, footer_magic);
		candidate.payload_version = AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION;
		if (!envelope_shape_valid(candidate))
			return error::corrupt_evidence;
		*command = std::move(candidate);
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

economic_accounting_error
auction_native_command_decode(const critical_command &command,
			      auction_native_command_context *output) noexcept
{
	if (!output || command.type != critical_command_type::auction ||
	    command.payload_version != AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION ||
	    (command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION &&
	     command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION) ||
	    (command.schema_version == CRITICAL_COMMAND_SCHEMA_VERSION &&
	     (command.publication_required || !command.accounting_intent.empty())))
		return error::invalid_version;
	try
	{
		if (!envelope_shape_valid(command) ||
		    command.payload.size() < trailer_header_bytes + footer_bytes + 1)
			return error::corrupt_evidence;
		const std::span<const uint8_t> bytes(command.payload);
		size_t offset = bytes.size() - footer_bytes;
		uint32_t base_size = 0, trailer_size = 0, magic = 0;
		if (!read(bytes, offset, base_size) || !read(bytes, offset, trailer_size) ||
		    !read(bytes, offset, magic) || magic != footer_magic || !base_size ||
		    base_size > bytes.size() - footer_bytes ||
		    trailer_size != bytes.size() - footer_bytes - base_size ||
		    trailer_size < trailer_header_bytes ||
		    trailer_size >
			    trailer_header_bytes + PLAYER_SNAPSHOT_MAX_OBJECTS * sizeof(uint64_t))
			return error::corrupt_evidence;
		auction_native_command_context candidate;
		if (!decode_base(command, bytes.first(base_size), candidate.payload))
			return error::corrupt_evidence;
		const auto trailer = bytes.subspan(base_size, trailer_size);
		offset = 0;
		uint16_t version = 0, reserved = 0;
		if (!read(trailer, offset, magic) || magic != trailer_magic ||
		    !read(trailer, offset, version) ||
		    version != AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION ||
		    !read(trailer, offset, reserved) || reserved ||
		    !read(trailer, offset, candidate.original_level) ||
		    !read(trailer, offset, candidate.acknowledged_save_revision))
			return error::corrupt_evidence;
		for (auto *value : { &candidate.before_digest, &candidate.after_digest,
				     &candidate.selected_digest })
		{
			if (trailer.size() - offset < value->size())
				return error::corrupt_evidence;
			std::copy_n(trailer.begin() + offset, value->size(), value->begin());
			offset += value->size();
		}
		if (!read(trailer, offset, candidate.selected_node_count) ||
		    !read(trailer, offset, candidate.selected_root_count) ||
		    !read(trailer, offset, reserved) || reserved)
			return error::corrupt_evidence;
		uint32_t before_count = 0;
		if (!read(trailer, offset, before_count) ||
		    before_count > PLAYER_SNAPSHOT_MAX_OBJECTS ||
		    trailer.size() - offset != static_cast<size_t>(before_count) * sizeof(uint64_t))
			return error::corrupt_evidence;
		std::unordered_set<uint64_t> before_identities;
		candidate.before_item_uids.reserve(before_count);
		for (uint32_t i = 0; i < before_count; ++i)
		{
			uint64_t uid = 0;
			if (!read(trailer, offset, uid) || !uid || uid == UINT64_MAX ||
			    !before_identities.insert(uid).second)
				return error::corrupt_evidence;
			candidate.before_item_uids.push_back(uid);
		}
		if (offset != trailer.size())
			return error::corrupt_evidence;
		const auto &payload = candidate.payload;
		const bool background = payload.action == auction_action::finalize ||
					payload.action == auction_action::remove;
		const bool items = payload.action == auction_action::list ||
				   payload.action == auction_action::claim_item;
		if ((background && (payload.actor_pid || candidate.original_level ||
				    candidate.acknowledged_save_revision ||
				    !candidate.before_item_uids.empty())) ||
		    (!background &&
		     (!payload.actor_pid || !candidate.original_level ||
		      candidate.original_level > 255 || !candidate.acknowledged_save_revision)))
			return error::invalid_identity;
		if (items)
		{
			if (!payload.item_count ||
			    candidate.selected_root_count != payload.item_count ||
			    candidate.selected_root_count > AUCTION_COMMAND_MAX_ITEMS ||
			    candidate.selected_node_count < candidate.selected_root_count ||
			    candidate.selected_node_count > PLAYER_SNAPSHOT_MAX_OBJECTS)
				return error::invalid_identity;
			for (size_t i = 0; i < payload.item_count; ++i)
				if (!payload.items[i].item_uid ||
				    payload.items[i].item_uid == UINT64_MAX ||
				    payload.items[i].vnum != payload.items[0].vnum)
					return error::invalid_identity;
			const auto item_fences = std::count_if(
				command.keys.begin(), command.keys.end(), [](const auto &key)
				{ return key.type == critical_entity_type::item; });
			if (item_fences > candidate.selected_node_count ||
			    (command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
			     item_fences != candidate.selected_node_count))
				return error::invalid_identity;
		}
		else if (payload.item_count || candidate.selected_node_count ||
			 candidate.selected_root_count ||
			 (!background && payload.action != auction_action::bid &&
			  payload.action != auction_action::claim_money))
			return error::invalid_identity;
		if ((candidate.payload.action == auction_action::bid ||
		     candidate.payload.action == auction_action::claim_money) &&
		    candidate.before_digest != candidate.after_digest)
			return error::topology;
		if (!items)
		{
			std::vector<uint8_t> empty_bytes;
			std::array<uint8_t, 32> empty_digest{};
			if (canonical({}, empty_bytes) != error::ok ||
			    !digest(empty_bytes, empty_digest) ||
			    candidate.selected_digest != empty_digest ||
			    (background && (candidate.before_digest != empty_digest ||
					    candidate.after_digest != empty_digest)))
				return error::corrupt_evidence;
		}
		candidate.base_v1_payload.assign(bytes.begin(), bytes.begin() + base_size);
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

#if defined(__linux__) && defined(__x86_64__) && __cplusplus == 202002L &&                        \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&    \
	!defined(_GLIBCXX_PARALLEL) && !defined(_GLIBCXX_SANITIZE_VECTOR)

namespace
{
constexpr size_t auction_codec_allocator_frames =
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
constexpr size_t auction_codec_copy_frames =
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
constexpr size_t auction_codec_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t auction_codec_default_frames =
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
constexpr size_t auction_codec_vector_frames =
	auction_codec_allocator_frames + auction_codec_copy_frames + auction_codec_relocate_frames +
	auction_codec_default_frames +
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
constexpr size_t auction_codec_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + auction_codec_allocator_frames;
constexpr size_t auction_codec_vector_constructor_frames =
	2 * sizeof(void *) + 3 * sizeof(std::allocator<uint8_t>) + 2 * sizeof(void *) +
	sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(size_t) +
	8 * (sizeof(void *) + sizeof(size_t)) + auction_codec_vector_frames;

// Genuine additional selected typed library scopes beside the vector's
// reserve/forward-insert profile. The owning inline DTOs remain in the actual
// enclosing object sizes, rather than a fabricated encoded-envelope baseline.
static_assert(std::is_trivially_copyable_v<economic_source_event>);
static_assert(std::is_trivially_destructible_v<economic_source_event>);
static_assert(std::is_trivially_copyable_v<auction_command_payload>);
static_assert(std::is_trivially_copyable_v<economic_account_key>);
constexpr size_t auction_codec_optional_frames =
	// Actual metadata/frozen/admission default/generated move/copy member
	// functions: this/source refs; optional/_Optional_base/_payload/_Storage
	// default constructors and trivial storage destructor this carriers.
	6 * sizeof(void *) + 5 * sizeof(void *) + sizeof(void *) +
	// source_event assignment operator=(T&&): this/u/ref-result; real
	// is_engaged/get/construct wrappers, payload _M_construct and forward.
	3 * sizeof(void *) + (sizeof(void *) + sizeof(bool)) + 2 * (2 * sizeof(void *)) +
	2 * sizeof(void *) + 2 * sizeof(void *) +
	// __addressof -> _Construct -> forward -> placement new -> trivial
	// economic_source_event generated move this/source. No extra DTO copy.
	2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(void *) +
	sizeof(size_t) + sizeof(void *) + 2 * sizeof(void *) +
	// Actual optional operator bool/operator->/base get/payload get and
	// addressof parameter and reference/pointer/bool result carriers.
	2 * (sizeof(void *) + sizeof(bool)) + 3 * (2 * sizeof(void *));
constexpr size_t auction_codec_equal_frames =
	// array/vector operator== actual lhs/rhs and returned bool; genuine
	// container size/begin/end and array_traits::_S_ptr pointer returns.
	2 * sizeof(void *) + sizeof(bool) + 2 * (sizeof(void *) + sizeof(size_t)) +
	6 * (2 * sizeof(void *)) +
	// equal/__equal_aux/__equal_aux1/__equal<true>::equal each three
	// iterator arguments and returned bool; __simple and __len locals;
	// niter_base calls and __memcmp's genuine pointers/length/int result.
	4 * (3 * sizeof(void *) + sizeof(bool)) + sizeof(bool) + sizeof(size_t) +
	3 * (2 * sizeof(void *)) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(int) +
	// Actual normal_iterator copied argument/ctor/base source carriers.
	4 * (2 * sizeof(void *));
constexpr size_t auction_codec_copy_n_frames =
	// Original copy_n(count literal16) owns first/count/result/__n2/result,
	// __size_to_integer(int), iterator_category and __copy_n<RA> tag.
	3 * sizeof(void *) + 2 * sizeof(int) + 2 * sizeof(int) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + 3 * sizeof(void *) + sizeof(int) +
	sizeof(std::random_access_iterator_tag);
constexpr size_t auction_codec_scalar_source_frames =
	// Original read lambda (facts-reference capture/this, offset,width,
	// byte index,value/result); typed read_number alternatives; span data,
	// index,size/constructor parameters and returned pointer/reference.
	2 * sizeof(void *) + 3 * sizeof(size_t) + 2 * sizeof(uint64_t) +
	sizeof(std::span<const uint8_t>) + 2 * sizeof(size_t) + 2 * sizeof(uint64_t) +
	4 * (sizeof(void *) + sizeof(size_t) + sizeof(void *)) +
	// Actual account/empty/key_valid/kind_valid helper params/results,
	// operation_id_equal two refs/result and zero's byte range loop.
	3 * sizeof(void *) + sizeof(economic_account_kind) + sizeof(uint64_t) + sizeof(bool) +
	2 * (sizeof(void *) + sizeof(bool)) + sizeof(economic_account_kind) + sizeof(bool) +
	2 * sizeof(void *) + sizeof(bool) + 4 * sizeof(void *) + sizeof(uint8_t) + sizeof(bool) +
	// Original append_u64/u32/u16 and native_fact_append<T> pointer/value/
	// index/byte result lifetimes, plus initializer-list begin/end/size.
	4 * (sizeof(void *) + sizeof(uint64_t) + sizeof(size_t) + sizeof(uint8_t)) +
	3 * (sizeof(void *) + sizeof(void *)) + sizeof(std::initializer_list<uint64_t>) +
	// Original metadata assignment generated function this/source and the
	// returned source_for event temporary, optional typed path above.
	2 * sizeof(void *) + sizeof(economic_source_event) + auction_codec_optional_frames +
	auction_codec_equal_frames + auction_codec_copy_n_frames;
bool auction_codec_add(size_t &total, size_t value) noexcept
{
	if (value > SIZE_MAX - total)
		return false;
	total += value;
	return true;
}
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
class auction_native_uid_set final : public std::__uset_hashtable<uint64_t>
{
	using table_type = std::__uset_hashtable<uint64_t>;
	using actual_node = std::__detail::_Hash_node<
		uint64_t, std::__cache_default<uint64_t, std::hash<uint64_t>>::value>;

    public:
	using table_type::table_type;
	bool current_heap(size_t *out) const noexcept
	{
		if (!out || !this->bucket_count())
			return false;
		size_t bytes = 0;
		if (this->bucket_count() > 1 &&
		    (this->bucket_count() > SIZE_MAX / sizeof(std::__detail::_Hash_node_base *) ||
		     !auction_codec_add(bytes, this->bucket_count() *
						       sizeof(std::__detail::_Hash_node_base *))))
			return false;
		if (this->size() > SIZE_MAX / sizeof(actual_node) ||
		    !auction_codec_add(bytes, this->size() * sizeof(actual_node)))
			return false;
		*out = bytes;
		return true;
	}
	bool prospective_insert(uint64_t uid, size_t *out) const noexcept
	{
		if (!out || this->size() == this->max_size())
			return false;
		if (this->find(uid) != this->end())
		{
			*out = 0;
			return true;
		}
		size_t bytes = sizeof(actual_node);
		auto policy = this->__rehash_policy();
		const auto next = policy._M_need_rehash(this->bucket_count(), this->size(), 1);
		if (next.first && next.second > 1 &&
		    (next.second > SIZE_MAX / sizeof(std::__detail::_Hash_node_base *) ||
		     !auction_codec_add(bytes,
					next.second * sizeof(std::__detail::_Hash_node_base *))))
			return false;
		*out = bytes;
		return true;
	}
};
static_assert(sizeof(auction_native_uid_set) == sizeof(std::unordered_set<uint64_t>));
static_assert(alignof(auction_native_uid_set) == alignof(std::unordered_set<uint64_t>));
static_assert(
	std::is_same_v<auction_native_uid_set::iterator, std::unordered_set<uint64_t>::iterator>);
#else
using auction_native_uid_set = std::unordered_set<uint64_t>;
#endif

bool auction_native_query_prepare(bool (*reserve)(size_t, void *) noexcept, void *context,
				  size_t outer) noexcept
{
	// Public decoder signature/return, genuine eight uid-profile size_t
	// locals/return, prepare signature/local and add/callback signatures.
	size_t total = outer;
	return reserve &&
	       auction_codec_add(total, 8 * sizeof(void *) + 13 * sizeof(size_t) +
						sizeof(economic_accounting_error) +
						3 * sizeof(bool)) &&
	       reserve(total, context);
}
struct auction_native_decode_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const critical_command *projection = nullptr;
	const auction_native_command_context *candidate = nullptr;
	const std::vector<uint8_t> *roundtrip = nullptr, *empty = nullptr;
	const auction_native_uid_set *uids = nullptr;
	bool denied = false;
	static bool forward(size_t amount, void *opaque) noexcept
	{
		auto &b = *static_cast<auction_native_decode_budget *>(opaque);
		if (b.denied || !b.reserve || !b.reserve(amount, b.context))
		{
			b.denied = true;
			return false;
		}
		return true;
	}
	bool prefix(size_t &bytes, size_t extra = 0) noexcept
	{
		bytes = outer;
		size_t heap = 0;
		const bool observed =
			auction_codec_add(bytes, sizeof(*this)) &&
			auction_codec_add(bytes, frames) && auction_codec_add(bytes, extra) &&
			(!projection || (critical_command_current_heap_bytes(*projection, &heap) &&
					 auction_codec_add(bytes, heap))) &&
			(!candidate ||
			 (auction_codec_add(bytes, candidate->base_v1_payload.capacity()) &&
			  candidate->before_item_uids.capacity() <= SIZE_MAX / sizeof(uint64_t) &&
			  auction_codec_add(bytes, candidate->before_item_uids.capacity() *
							   sizeof(uint64_t)))) &&
			(!roundtrip || auction_codec_add(bytes, roundtrip->capacity())) &&
			(!empty || auction_codec_add(bytes, empty->capacity())) &&
			(!uids || (uids->current_heap(&heap) && auction_codec_add(bytes, heap)));
		if (!observed)
			denied = true;
		return observed;
	}
	bool peak(size_t extra = 0) noexcept
	{
		size_t bytes = 0;
		if (!prefix(bytes, extra) || !forward(bytes, this))
		{
			denied = true;
			return false;
		}
		return true;
	}
};
bool auction_native_decode_policy() noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	return sizeof(void *) == 8 && sizeof(size_t) == 8;
#else
	return false;
#endif
}
bool auction_native_envelope_shape_bounded(const critical_command &command,
					   auction_native_decode_budget &budget)
{
	size_t request = 0;
	if (!critical_command_fresh_copy_request_bytes(command, &request) ||
	    !auction_codec_add(request, critical_command_copy_frame_bytes()) ||
	    !budget.peak(request))
		return false;
	critical_command projection = command;
	budget.projection = &projection;
	if (!projection.accepted_at_usec)
		projection.accepted_at_usec = 1;
	const bool valid = budget.peak(critical_command_valid_frame_bytes()) &&
			   critical_command_envelope_valid(projection);
	budget.projection = nullptr;
	return valid;
}
bool auction_native_decode_base_bounded(const critical_command &command,
					std::span<const uint8_t> bytes,
					auction_command_payload &payload,
					auction_native_decode_budget &budget)
{
	size_t request = 0, nested = 0;
	if (!critical_command_fresh_copy_request_bytes(command, &request) ||
	    !auction_codec_add(request, critical_command_copy_frame_bytes()) ||
	    !budget.peak(request))
		return false;
	critical_command original = command;
	budget.projection = &original;
	original.payload_version = AUCTION_COMMAND_PAYLOAD_VERSION;
	if (bytes.size() > original.payload.capacity() && !budget.peak(bytes.size()))
		return false;
	original.payload.assign(bytes.begin(), bytes.end());
	std::vector<uint8_t> roundtrip;
	budget.roundtrip = &roundtrip;
	const bool valid =
		budget.prefix(nested) &&
		(command.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION ?
			 auction_command_decode_native_base_bounded(
				 command, bytes, &payload, auction_native_decode_budget::forward,
				 &budget, nested) :
			 auction_command_decode_payload_bounded(
				 original, &payload, auction_native_decode_budget::forward, &budget,
				 nested)) &&
		budget.prefix(nested) &&
		auction_command_encode_payload_bounded(payload, &roundtrip,
						       auction_native_decode_budget::forward,
						       &budget, nested) &&
		roundtrip.size() == bytes.size() &&
		std::equal(roundtrip.begin(), roundtrip.end(), bytes.begin());
	budget.roundtrip = nullptr;
	budget.projection = nullptr;
	return valid;
}
bool auction_native_empty_canonical_bounded(std::vector<uint8_t> &bytes,
					    auction_native_decode_budget &budget)
{
	// This actual call site passes the empty forest. The original canonical
	// text/UID/ancestry loop is empty, then encodes a fresh empty vector.
	const size_t frames = sizeof(std::unordered_set<uint64_t>) +
			      sizeof(std::array<size_t, PLAYER_SNAPSHOT_MAX_DEPTH>) +
			      sizeof(std::vector<player_item_snapshot>) + 12 * sizeof(void *) +
			      8 * sizeof(size_t) + 6 * sizeof(bool) + sizeof(int16_t) +
			      auction_codec_vector_constructor_frames;
	size_t nested = 0;
	if (!budget.prefix(nested, frames) ||
	    !auction_native_decode_budget::forward(nested, &budget))
		return false;
	const std::vector<player_item_snapshot> copy;
	return player_item_snapshot_list_encode_bounded(
		       copy, &bytes, auction_native_decode_budget::forward, &budget, nested) ==
	       player_snapshot_codec_result::ok;
}
}

namespace
{
constexpr size_t auction_digest_sha_assembly_frames =
	2 * 4 * 64 + 4 * sizeof(void *) + 6 * sizeof(uint64_t) + (256 * 4 - 1) + 2 * sizeof(void *);
constexpr size_t auction_digest_sha_c_small_frames =
	16 * sizeof(unsigned int) + 12 * sizeof(unsigned int) + sizeof(unsigned int) + sizeof(int) +
	sizeof(const uint8_t *);
constexpr size_t auction_digest_sha_c_normal_frames = 16 * sizeof(unsigned int) +
						      11 * sizeof(unsigned int) + 2 * sizeof(int) +
						      2 * sizeof(void *);
constexpr size_t auction_digest_sha_init_frames = sizeof(void *) + sizeof(int);
constexpr size_t auction_digest_sha_update_frames = 2 * sizeof(void *) + sizeof(size_t) +
						    2 * sizeof(void *) + sizeof(unsigned int) +
						    sizeof(size_t) + sizeof(int);
constexpr size_t auction_digest_sha_final_frames = 3 * sizeof(void *) + sizeof(size_t) +
						   sizeof(unsigned long) + sizeof(unsigned int) +
						   sizeof(int);
[[maybe_unused]] constexpr size_t auction_digest_sha_frames =
	std::max(auction_digest_sha_assembly_frames,
		 std::max(auction_digest_sha_c_small_frames, auction_digest_sha_c_normal_frames)) +
	std::max(auction_digest_sha_init_frames,
		 std::max(auction_digest_sha_update_frames, auction_digest_sha_final_frames));

template <class Budget> bool auction_digest_fixed_bounded(std::span<const uint8_t> bytes,
							  std::array<uint8_t, 32> &output,
							  Budget &budget) noexcept
{
#if defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR == 3 &&      \
	defined(OPENSSL_VERSION_MINOR) && OPENSSL_VERSION_MINOR == 0 &&  \
	defined(OPENSSL_VERSION_PATCH) && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	if (!budget.peak(sizeof(SHA256_CTX) + auction_digest_sha_frames +
			 sizeof(std::span<const uint8_t>) + 3 * sizeof(void *) + 4 * sizeof(bool)))
		return false;
	// Original auction digest has no tag or domain prefix. Hash exactly the
	// same full canonical forest bytes, using caller-owned fixed SHA state.
	SHA256_CTX digest_context;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	const bool ok = SHA256_Init(&digest_context) == 1 &&
			SHA256_Update(&digest_context, bytes.data(), bytes.size()) == 1 &&
			SHA256_Final(output.data(), &digest_context) == 1;
#pragma GCC diagnostic pop
	return ok;
#else
	(void)bytes;
	(void)output;
	budget.denied = true;
	return false;
#endif
}

}
size_t auction_native_command_uid_set_source_frame_bytes() noexcept
{
	using iterator = auction_native_uid_set::iterator;
	// Genuine uint64 table find/insert and unique auxiliary scopes. Default
	// hash<uint64> has no cached string hash or string constructor/move.
	constexpr size_t lookup = 5 * (2 * sizeof(void *) + sizeof(void *)) + 4 * sizeof(void *) +
				  sizeof(iterator) + 2 * sizeof(size_t) + 2 * sizeof(void *) +
				  sizeof(iterator) + sizeof(std::pair<iterator, bool>);
	// Unique-node insertion, saved policy state, rehash pair and forwarding
	// into unique rehash's declared bucket/node pointer and scalar carriers.
	constexpr size_t rehash = 3 * sizeof(void *) + 3 * sizeof(size_t) +
				  sizeof(std::pair<bool, size_t>) + sizeof(iterator) +
				  2 * sizeof(void *) + sizeof(size_t) + sizeof(char) +
				  4 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(char);
	constexpr size_t allocation = 9 * sizeof(void *) + 4 * sizeof(size_t) +
				      2 * sizeof(std::allocator<void *>) + 3 * sizeof(void *) +
				      sizeof(size_t) + 5 * sizeof(void *) + 10 * sizeof(void *) +
				      sizeof(size_t);
	// Key extraction/hash/equal/bucket modulus/next/value pointers, actual
	// uint64 hash parameter/return and equal_to row references/this/result.
	constexpr size_t predicates = 6 * sizeof(void *) + 3 * sizeof(size_t) +
				      10 * (2 * sizeof(void *) + sizeof(void *)) +
				      2 * sizeof(bool) + sizeof(void *) + 2 * sizeof(uint64_t) +
				      3 * sizeof(void *) + sizeof(bool);
	// Full original table clear and node/bucket deallocation through allocator
	// traits. Native codec keeps the table through output move then destroys it.
	constexpr size_t cleanup = 10 * sizeof(void *) + 5 * sizeof(size_t) + 4 * sizeof(void *) +
				   auction_codec_allocator_frames;
	// Actual pure table request observer: this/uid/out/bytes/policy copy,
	// _M_need_rehash's declared inputs/returned pair and find/end wrappers.
	constexpr size_t observer = 8 * sizeof(void *) + 8 * sizeof(size_t) + 5 * sizeof(bool) +
				    sizeof(uint64_t) + sizeof(std::__detail::_Prime_rehash_policy) +
				    sizeof(std::pair<bool, size_t>) +
				    4 * (sizeof(void *) + sizeof(size_t));
	// Full authenticated GCC13.3 out-of-line prime policy body, not merely
	// its declaration: need_rehash owns this/three counts/returned pair/min;
	// next_bkt owns this/n/return/n_primes/last_prime/next_bkt. Static prime
	// and fast-bucket tables are library readonly objects, not private heap.
	constexpr size_t prime_policy =
		sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::pair<bool, size_t>) +
		sizeof(double) + 3 * sizeof(void *) + 3 * sizeof(size_t) +
		// Both actual std::max<size_t> closures and converted scalar inputs;
		// floor declarations contribute their double input/returned value.
		2 * (3 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) + 4 * sizeof(double) +
		// Real pair<bool,size_t> forwarding constructor and both forward
		// argument/return reference leaves, scalar constructor inputs.
		7 * sizeof(void *) + sizeof(bool) + sizeof(size_t) +
		// lower_bound<const unsigned long*,size_t> params/return and genuine
		// __iter_less_val returned empty adapter/default-this carrier.
		5 * sizeof(void *) + sizeof(__gnu_cxx::__ops::_Iter_less_val) +
		// __lower_bound params/return, comp, len/half/middle.
		5 * sizeof(void *) + 2 * sizeof(std::ptrdiff_t) +
		sizeof(__gnu_cxx::__ops::_Iter_less_val) +
		// distance -> random-access __distance params/tag/returned distance.
		4 * sizeof(void *) + 2 * sizeof(std::ptrdiff_t) +
		sizeof(std::random_access_iterator_tag) +
		// Both actual __iterator_category signatures and returned RA tags.
		2 * (sizeof(void *) + sizeof(std::random_access_iterator_tag)) +
		// advance's reference/n/local __d and RA __advance's ref/n/tag.
		2 * sizeof(void *) + 3 * sizeof(std::ptrdiff_t) +
		sizeof(std::random_access_iterator_tag) +
		// _Iter_less_val::operator(): this/actual ulong pointer/value ref/bool.
		3 * sizeof(void *) + sizeof(bool);
	return lookup + rehash + allocation + predicates + cleanup + observer + prime_policy;
}

economic_accounting_error auction_native_command_decode_bounded(
	const critical_command &command, auction_native_command_context *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	if (!auction_native_query_prepare(reserve, context, outer))
		return error::capacity;
	const size_t frames = sizeof(auction_native_command_context) + sizeof(critical_command) +
			      sizeof(auction_native_uid_set) + 2 * sizeof(std::vector<uint8_t>) +
			      sizeof(std::array<uint8_t, 32>) +
			      3 * sizeof(std::span<const uint8_t>) + 23 * sizeof(void *) +
			      12 * sizeof(size_t) + 8 * sizeof(bool) + 4 * sizeof(uint32_t) +
			      3 * sizeof(uint16_t) + sizeof(uint64_t) +
			      auction_codec_vector_frames + auction_codec_move_frames +
			      auction_codec_vector_constructor_frames +
			      auction_native_command_uid_set_source_frame_bytes();
	auction_native_decode_budget budget{ reserve, context, outer, frames };
	if (!auction_native_decode_policy() || !budget.peak())
		return error::capacity;
	if (!output || command.type != critical_command_type::auction ||
	    command.payload_version != AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION ||
	    (command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION &&
	     command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION) ||
	    (command.schema_version == CRITICAL_COMMAND_SCHEMA_VERSION &&
	     (command.publication_required || !command.accounting_intent.empty())))
		return error::invalid_version;
	try
	{
		if (!auction_native_envelope_shape_bounded(command, budget) ||
		    command.payload.size() < trailer_header_bytes + footer_bytes + 1)
			return budget.denied ? error::capacity : error::corrupt_evidence;
		const std::span<const uint8_t> bytes(command.payload);
		size_t offset = bytes.size() - footer_bytes;
		uint32_t base_size = 0, trailer_size = 0, magic = 0;
		if (!read(bytes, offset, base_size) || !read(bytes, offset, trailer_size) ||
		    !read(bytes, offset, magic) || magic != footer_magic || !base_size ||
		    base_size > bytes.size() - footer_bytes ||
		    trailer_size != bytes.size() - footer_bytes - base_size ||
		    trailer_size < trailer_header_bytes ||
		    trailer_size >
			    trailer_header_bytes + PLAYER_SNAPSHOT_MAX_OBJECTS * sizeof(uint64_t))
			return budget.denied ? error::capacity : error::corrupt_evidence;
		auction_native_command_context candidate;
		budget.candidate = &candidate;
		if (!auction_native_decode_base_bounded(command, bytes.first(base_size),
							candidate.payload, budget))
			return budget.denied ? error::capacity : error::corrupt_evidence;
		const auto trailer = bytes.subspan(base_size, trailer_size);
		offset = 0;
		uint16_t version = 0, reserved = 0;
		if (!read(trailer, offset, magic) || magic != trailer_magic ||
		    !read(trailer, offset, version) ||
		    version != AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION ||
		    !read(trailer, offset, reserved) || reserved ||
		    !read(trailer, offset, candidate.original_level) ||
		    !read(trailer, offset, candidate.acknowledged_save_revision))
			return budget.denied ? error::capacity : error::corrupt_evidence;
		for (auto *value : { &candidate.before_digest, &candidate.after_digest,
				     &candidate.selected_digest })
		{
			if (trailer.size() - offset < value->size())
				return budget.denied ? error::capacity : error::corrupt_evidence;
			std::copy_n(trailer.begin() + offset, value->size(), value->begin());
			offset += value->size();
		}
		if (!read(trailer, offset, candidate.selected_node_count) ||
		    !read(trailer, offset, candidate.selected_root_count) ||
		    !read(trailer, offset, reserved) || reserved)
			return budget.denied ? error::capacity : error::corrupt_evidence;
		uint32_t before_count = 0;
		if (!read(trailer, offset, before_count) ||
		    before_count > PLAYER_SNAPSHOT_MAX_OBJECTS ||
		    trailer.size() - offset != static_cast<size_t>(before_count) * sizeof(uint64_t))
			return budget.denied ? error::capacity : error::corrupt_evidence;
		auction_native_uid_set before_identities;
		budget.uids = &before_identities;
		if (!budget.peak(static_cast<size_t>(before_count) * sizeof(uint64_t)))
			return error::capacity;
		candidate.before_item_uids.reserve(before_count);
		for (uint32_t i = 0; i < before_count; ++i)
		{
			uint64_t uid = 0;
			if (!read(trailer, offset, uid) || !uid || uid == UINT64_MAX ||
			    ([&]() {
                    size_t request=0;
                    return !before_identities.prospective_insert(uid,&request) ||
                        !budget.peak(request) || !before_identities.insert(uid).second;
                }()))
				return budget.denied ? error::capacity : error::corrupt_evidence;
			candidate.before_item_uids.push_back(uid);
		}
		if (offset != trailer.size())
			return budget.denied ? error::capacity : error::corrupt_evidence;
		const auto &payload = candidate.payload;
		const bool background = payload.action == auction_action::finalize ||
					payload.action == auction_action::remove;
		const bool items = payload.action == auction_action::list ||
				   payload.action == auction_action::claim_item;
		if ((background && (payload.actor_pid || candidate.original_level ||
				    candidate.acknowledged_save_revision ||
				    !candidate.before_item_uids.empty())) ||
		    (!background &&
		     (!payload.actor_pid || !candidate.original_level ||
		      candidate.original_level > 255 || !candidate.acknowledged_save_revision)))
			return budget.denied ? error::capacity : error::invalid_identity;
		if (items)
		{
			if (!payload.item_count ||
			    candidate.selected_root_count != payload.item_count ||
			    candidate.selected_root_count > AUCTION_COMMAND_MAX_ITEMS ||
			    candidate.selected_node_count < candidate.selected_root_count ||
			    candidate.selected_node_count > PLAYER_SNAPSHOT_MAX_OBJECTS)
				return budget.denied ? error::capacity : error::invalid_identity;
			for (size_t i = 0; i < payload.item_count; ++i)
				if (!payload.items[i].item_uid ||
				    payload.items[i].item_uid == UINT64_MAX ||
				    payload.items[i].vnum != payload.items[0].vnum)
					return budget.denied ? error::capacity :
							       error::invalid_identity;
			const auto item_fences = std::count_if(
				command.keys.begin(), command.keys.end(), [](const auto &key)
				{ return key.type == critical_entity_type::item; });
			if (item_fences > candidate.selected_node_count ||
			    (command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
			     item_fences != candidate.selected_node_count))
				return budget.denied ? error::capacity : error::invalid_identity;
		}
		else if (payload.item_count || candidate.selected_node_count ||
			 candidate.selected_root_count ||
			 (!background && payload.action != auction_action::bid &&
			  payload.action != auction_action::claim_money))
			return budget.denied ? error::capacity : error::invalid_identity;
		if ((candidate.payload.action == auction_action::bid ||
		     candidate.payload.action == auction_action::claim_money) &&
		    candidate.before_digest != candidate.after_digest)
			return error::topology;
		if (!items)
		{
			std::vector<uint8_t> empty_bytes;
			budget.empty = &empty_bytes;
			std::array<uint8_t, 32> empty_digest{};
			if (!auction_native_empty_canonical_bounded(empty_bytes, budget) ||
			    !auction_digest_fixed_bounded(empty_bytes, empty_digest, budget) ||
			    candidate.selected_digest != empty_digest ||
			    (background && (candidate.before_digest != empty_digest ||
					    candidate.after_digest != empty_digest)))
				return budget.denied ? error::capacity : error::corrupt_evidence;
			budget.empty = nullptr;
		}
		if (!budget.peak(base_size))
			return error::capacity;
		candidate.base_v1_payload.assign(bytes.begin(), bytes.begin() + base_size);
		*output = std::move(candidate);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return budget.denied ? error::capacity : error::corrupt_evidence;
	}
}

#else
size_t auction_native_command_uid_set_source_frame_bytes() noexcept
{
	return 0;
}
economic_accounting_error auction_native_command_decode_bounded(const critical_command &,
								auction_native_command_context *,
								bool (*)(size_t, void *) noexcept,
								void *, size_t) noexcept
{
	return economic_accounting_error::capacity;
}
#endif
