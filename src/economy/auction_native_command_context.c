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
