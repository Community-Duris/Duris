#include "world/quest_mobile_native.h"

#include "core/structs.h"
#include "core/utils.h"

#include <algorithm>
#include <bit>
#include <cstring>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>
#include <unordered_set>
#include <utility>

extern P_index mob_index;
extern int top_of_mobt;

namespace
{
constexpr std::array<uint8_t, 8> reference_magic = { 'Q', 'M', 'N', 'R', 'E', 'F', 0, 0 };
constexpr std::array<uint8_t, 8> image_magic = { 'Q', 'M', 'N', 'I', 'M', 'G', 0, 0 };
constexpr uint8_t literal_mask = STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3;

bool nonzero(const critical_operation_id &id)
{
	return std::any_of(id.bytes.begin(), id.bytes.end(),
			   [](uint8_t value) { return value != 0; });
}

bool reference_valid(const quest_mobile_native_reference &value)
{
	return value.mobile_instance_id && value.mobile_instance_id != UINT64_MAX &&
	       nonzero(value.birth_operation) && economic_source_event_valid(value.birth_source) &&
	       value.mobile_vnum >= 0 && value.mobile_revision && value.stock_revision &&
	       ((value.provenance == quest_mobile_birth_provenance::reset &&
		 value.reset_zone_vnum >= 0) ||
		(value.provenance == quest_mobile_birth_provenance::spawn &&
		 value.reset_zone_vnum == -1));
}

template <typename T> void put(uint8_t *bytes, size_t &offset, T value)
{
	using U = std::make_unsigned_t<T>;
	U bits = static_cast<U>(value);
	for (size_t n = 0; n < sizeof(T); ++n)
	{
		bytes[offset++] = static_cast<uint8_t>(bits & 0xff);
		bits >>= 8;
	}
}

template <typename T> T get(const uint8_t *bytes, size_t &offset)
{
	using U = std::make_unsigned_t<T>;
	U bits = 0;
	for (size_t n = 0; n < sizeof(T); ++n)
		bits |= static_cast<U>(bytes[offset++]) << (8 * n);
	return std::bit_cast<T>(bits);
}

bool checksum(std::span<const uint8_t> bytes)
{
	if (bytes.size() < SHA256_DIGEST_LENGTH)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	const size_t sealed = bytes.size() - digest.size();
	return SHA256(bytes.data(), sealed, digest.data()) &&
	       CRYPTO_memcmp(digest.data(), bytes.data() + sealed, digest.size()) == 0;
}

// Extra native restrictions beyond the generic item codec: complete literal
// identities, real slots and contiguous DFS (a later subtree cannot reopen an
// earlier sibling). Neither revisions nor custody are inferred from these rows.
player_snapshot_codec_result forest_valid(const std::vector<player_item_snapshot> &items)
{
	if (items.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return player_snapshot_codec_result::limit_exceeded;
	std::unordered_set<uint64_t> uids;
	std::array<int32_t, PLAYER_SNAPSHOT_MAX_DEPTH> path = {};
	size_t depth = 0;
	int16_t last_equipment_slot = 0;
	bool inventory_started = false;
	for (size_t index = 0; index < items.size(); ++index)
	{
		const auto &item = items[index];
		if (!item.object_uid || item.object_uid == UINT64_MAX ||
		    !uids.insert(item.object_uid).second || item.vnum < 0 ||
		    item.string_mask != literal_mask)
			return player_snapshot_codec_result::invalid_value;
		if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
		{
			if (item.equipment_slot < 0 || item.equipment_slot > MAX_WEAR)
				return player_snapshot_codec_result::invalid_value;
			if (item.equipment_slot)
			{
				if (inventory_started || item.equipment_slot <= last_equipment_slot)
					return player_snapshot_codec_result::invalid_value;
				last_equipment_slot = item.equipment_slot;
			}
			else
				inventory_started = true;
			depth = 1;
			path[0] = static_cast<int32_t>(index);
		}
		else
		{
			if (item.parent_index < 0 ||
			    item.parent_index >= static_cast<int32_t>(index) || item.equipment_slot)
				return player_snapshot_codec_result::invalid_value;
			while (depth && path[depth - 1] != item.parent_index)
				--depth;
			if (!depth)
				return player_snapshot_codec_result::invalid_value;
			if (depth == path.size())
				return player_snapshot_codec_result::limit_exceeded;
			path[depth++] = static_cast<int32_t>(index);
		}
	}
	return player_snapshot_codec_result::ok;
}

struct physical_audit
{
	std::unordered_set<const obj_data *> objects;
	std::unordered_set<uint64_t> uids;
};

player_snapshot_capture_result audit_tree(const obj_data *object, const obj_data *parent,
					  physical_audit &seen, size_t depth)
{
	if (depth > PLAYER_SNAPSHOT_MAX_DEPTH || seen.objects.size() >= PLAYER_SNAPSHOT_MAX_OBJECTS)
		return player_snapshot_capture_result::limit_exceeded;
	if (!seen.objects.insert(object).second)
		return player_snapshot_capture_result::object_cycle;
	if (!object->obj_uid || object->obj_uid == UINT64_MAX ||
	    !seen.uids.insert(object->obj_uid).second ||
	    (parent && (object->loc_p != LOC_INSIDE || object->loc.inside != parent)))
		return player_snapshot_capture_result::malformed_source;
	for (const obj_data *child = object->contains; child; child = child->next_content)
	{
		const auto result = audit_tree(child, object, seen, depth + 1);
		if (result != player_snapshot_capture_result::ok)
			return result;
	}
	return player_snapshot_capture_result::ok;
}

player_snapshot_capture_result append_tree(const obj_data *root, int16_t slot,
					   std::vector<player_item_snapshot> &items, size_t &bytes)
{
	std::vector<player_item_snapshot> tree;
	size_t estimate = 0;
	const auto result = player_item_snapshot_tree_capture_literal(const_cast<obj_data *>(root),
								      &tree, &estimate);
	if (result != player_snapshot_capture_result::ok)
		return result;
	if (tree.empty() || tree.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - items.size() ||
	    estimate < sizeof(player_snapshot) || bytes > PLAYER_SNAPSHOT_MAX_BYTES ||
	    estimate - sizeof(player_snapshot) > PLAYER_SNAPSHOT_MAX_BYTES - bytes)
		return player_snapshot_capture_result::limit_exceeded;
	// Each tree estimate includes one snapshot header; this complete forest
	// charges that common header once, following ordinary/keeper capture.
	bytes += estimate - sizeof(player_snapshot);
	const int32_t offset = static_cast<int32_t>(items.size());
	tree[0].equipment_slot = slot;
	for (auto &row : tree)
	{
		if (row.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			row.parent_index += offset;
		items.push_back(std::move(row));
	}
	return player_snapshot_capture_result::ok;
}
} // namespace

player_snapshot_codec_result quest_mobile_native_reference_encode(
	const quest_mobile_native_reference &value,
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> *output) noexcept
{
	if (!output || !reference_valid(value))
		return player_snapshot_codec_result::invalid_value;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> candidate = {};
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source = {};
	if (economic_source_event_encode(value.birth_source, &source) !=
	    economic_accounting_error::ok)
		return player_snapshot_codec_result::invalid_value;
	std::copy(reference_magic.begin(), reference_magic.end(), candidate.begin());
	size_t offset = reference_magic.size();
	put<uint16_t>(candidate.data(), offset, QUEST_MOBILE_NATIVE_VERSION);
	put<uint8_t>(candidate.data(), offset, static_cast<uint8_t>(value.provenance));
	put<uint8_t>(candidate.data(), offset, 0);
	put<uint32_t>(candidate.data(), offset, candidate.size());
	put<uint64_t>(candidate.data(), offset, value.mobile_instance_id);
	std::copy(value.birth_operation.bytes.begin(), value.birth_operation.bytes.end(),
		  candidate.begin() + offset);
	offset += value.birth_operation.bytes.size();
	std::copy(source.begin(), source.end(), candidate.begin() + offset);
	offset += source.size();
	put<int32_t>(candidate.data(), offset, value.mobile_vnum);
	put<int32_t>(candidate.data(), offset, value.birthplace_vnum);
	put<int32_t>(candidate.data(), offset, value.reset_zone_vnum);
	put<uint64_t>(candidate.data(), offset, value.mobile_revision);
	put<uint64_t>(candidate.data(), offset, value.stock_revision);
	if (offset != candidate.size() - SHA256_DIGEST_LENGTH ||
	    !SHA256(candidate.data(), offset, candidate.data() + offset))
		return player_snapshot_codec_result::invalid_value;
	*output = candidate;
	return player_snapshot_codec_result::ok;
}

player_snapshot_codec_result
quest_mobile_native_reference_decode(std::span<const uint8_t> bytes,
				     quest_mobile_native_reference *output) noexcept
{
	if (!output || bytes.size() != QUEST_MOBILE_NATIVE_REFERENCE_BYTES)
		return player_snapshot_codec_result::invalid_value;
	if (!std::equal(reference_magic.begin(), reference_magic.end(), bytes.begin()) ||
	    !checksum(bytes))
		return player_snapshot_codec_result::invalid_value;
	size_t offset = reference_magic.size();
	if (get<uint16_t>(bytes.data(), offset) != QUEST_MOBILE_NATIVE_VERSION)
		return player_snapshot_codec_result::unsupported_version;
	quest_mobile_native_reference candidate;
	candidate.provenance =
		static_cast<quest_mobile_birth_provenance>(get<uint8_t>(bytes.data(), offset));
	if (get<uint8_t>(bytes.data(), offset) ||
	    get<uint32_t>(bytes.data(), offset) != bytes.size())
		return player_snapshot_codec_result::invalid_value;
	candidate.mobile_instance_id = get<uint64_t>(bytes.data(), offset);
	std::copy_n(bytes.data() + offset, candidate.birth_operation.bytes.size(),
		    candidate.birth_operation.bytes.begin());
	offset += candidate.birth_operation.bytes.size();
	if (economic_source_event_decode(bytes.subspan(offset, ECONOMIC_SOURCE_EVENT_BYTES),
					 &candidate.birth_source) != economic_accounting_error::ok)
		return player_snapshot_codec_result::invalid_value;
	offset += ECONOMIC_SOURCE_EVENT_BYTES;
	candidate.mobile_vnum = get<int32_t>(bytes.data(), offset);
	candidate.birthplace_vnum = get<int32_t>(bytes.data(), offset);
	candidate.reset_zone_vnum = get<int32_t>(bytes.data(), offset);
	candidate.mobile_revision = get<uint64_t>(bytes.data(), offset);
	candidate.stock_revision = get<uint64_t>(bytes.data(), offset);
	if (!reference_valid(candidate))
		return player_snapshot_codec_result::invalid_value;
	*output = candidate;
	return player_snapshot_codec_result::ok;
}

player_snapshot_codec_result
quest_mobile_native_image_encode(const quest_mobile_native_image &image,
				 std::vector<uint8_t> *output) noexcept
{
	if (!output || !nonzero(image.last_transition_operation) ||
	    (image.state != quest_mobile_lifetime_state::live &&
	     image.state != quest_mobile_lifetime_state::retired) ||
	    (image.state == quest_mobile_lifetime_state::retired && !image.items.empty()))
		return player_snapshot_codec_result::invalid_value;
	try
	{
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> reference = {};
		auto result = quest_mobile_native_reference_encode(image.reference, &reference);
		if (result != player_snapshot_codec_result::ok)
			return result;
		result = forest_valid(image.items);
		if (result != player_snapshot_codec_result::ok)
			return result;
		std::vector<uint8_t> blob;
		result = player_item_snapshot_list_encode(image.items, &blob);
		if (result != player_snapshot_codec_result::ok)
			return result;
		if (blob.size() > PLAYER_SNAPSHOT_MAX_BYTES - QUEST_MOBILE_NATIVE_IMAGE_OVERHEAD)
			return player_snapshot_codec_result::limit_exceeded;
		std::vector<uint8_t> candidate(QUEST_MOBILE_NATIVE_IMAGE_OVERHEAD + blob.size(), 0);
		std::copy(image_magic.begin(), image_magic.end(), candidate.begin());
		size_t offset = image_magic.size();
		put<uint16_t>(candidate.data(), offset, QUEST_MOBILE_NATIVE_VERSION);
		put<uint8_t>(candidate.data(), offset, static_cast<uint8_t>(image.state));
		put<uint8_t>(candidate.data(), offset, 0);
		put<uint32_t>(candidate.data(), offset, candidate.size());
		std::copy(reference.begin(), reference.end(), candidate.begin() + offset);
		offset += reference.size();
		std::copy(image.last_transition_operation.bytes.begin(),
			  image.last_transition_operation.bytes.end(), candidate.begin() + offset);
		offset += image.last_transition_operation.bytes.size();
		put<uint32_t>(candidate.data(), offset, blob.size());
		std::copy(blob.begin(), blob.end(), candidate.begin() + offset);
		offset += blob.size();
		if (offset != candidate.size() - SHA256_DIGEST_LENGTH ||
		    !SHA256(candidate.data(), offset, candidate.data() + offset))
			return player_snapshot_codec_result::invalid_value;
		*output = std::move(candidate);
		return player_snapshot_codec_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
}

player_snapshot_codec_result
quest_mobile_native_image_decode(std::span<const uint8_t> bytes,
				 quest_mobile_native_image *output) noexcept
{
	if (!output)
		return player_snapshot_codec_result::invalid_value;
	if (bytes.size() < QUEST_MOBILE_NATIVE_IMAGE_OVERHEAD)
		return player_snapshot_codec_result::truncated;
	if (bytes.size() > PLAYER_SNAPSHOT_MAX_BYTES)
		return player_snapshot_codec_result::limit_exceeded;
	if (!std::equal(image_magic.begin(), image_magic.end(), bytes.begin()) || !checksum(bytes))
		return player_snapshot_codec_result::invalid_value;
	try
	{
		size_t offset = image_magic.size();
		if (get<uint16_t>(bytes.data(), offset) != QUEST_MOBILE_NATIVE_VERSION)
			return player_snapshot_codec_result::unsupported_version;
		quest_mobile_native_image candidate;
		candidate.state = static_cast<quest_mobile_lifetime_state>(
			get<uint8_t>(bytes.data(), offset));
		if (get<uint8_t>(bytes.data(), offset) ||
		    get<uint32_t>(bytes.data(), offset) != bytes.size())
			return player_snapshot_codec_result::invalid_value;
		auto result = quest_mobile_native_reference_decode(
			bytes.subspan(offset, QUEST_MOBILE_NATIVE_REFERENCE_BYTES),
			&candidate.reference);
		if (result != player_snapshot_codec_result::ok)
			return result;
		offset += QUEST_MOBILE_NATIVE_REFERENCE_BYTES;
		std::copy_n(bytes.data() + offset, candidate.last_transition_operation.bytes.size(),
			    candidate.last_transition_operation.bytes.begin());
		offset += candidate.last_transition_operation.bytes.size();
		const uint32_t length = get<uint32_t>(bytes.data(), offset);
		if (length != bytes.size() - offset - SHA256_DIGEST_LENGTH)
			return player_snapshot_codec_result::invalid_value;
		result = player_item_snapshot_list_decode(bytes.data() + offset, length,
							  &candidate.items);
		if (result != player_snapshot_codec_result::ok)
			return result;
		std::vector<uint8_t> canonical;
		result = quest_mobile_native_image_encode(candidate, &canonical);
		if (result != player_snapshot_codec_result::ok)
			return result;
		if (canonical.size() != bytes.size() ||
		    !std::equal(canonical.begin(), canonical.end(), bytes.begin()))
			return player_snapshot_codec_result::invalid_value;
		*output = std::move(candidate);
		return player_snapshot_codec_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
}

player_snapshot_capture_result
quest_mobile_native_capture(P_char mob, const quest_mobile_native_reference &reference,
			    quest_mobile_lifetime_state state,
			    const critical_operation_id &last_transition,
			    quest_mobile_native_image *output) noexcept
{
	if (state != quest_mobile_lifetime_state::live || !mob || !output || !IS_NPC(mob) ||
	    !mob->only.npc || !reference_valid(reference) || !nonzero(last_transition) ||
	    !mob_index || GET_RNUM(mob) < 0 || GET_RNUM(mob) > top_of_mobt ||
	    mob_index[GET_RNUM(mob)].virtual_number != reference.mobile_vnum ||
	    GET_BIRTHPLACE(mob) != reference.birthplace_vnum)
		return player_snapshot_capture_result::invalid_identity;
	try
	{
		physical_audit seen;
		// Audit the whole forest first, including cross-root aliases and exact
		// reciprocal location links. Capture cannot silently filter NORENT stock.
		for (int slot = 0; slot < MAX_WEAR; ++slot)
		{
			const obj_data *root = mob->equipment[slot];
			if (!root)
				continue;
			if (root->loc_p != LOC_WORN || root->loc.wearing != mob ||
			    root->next_content)
				return player_snapshot_capture_result::malformed_source;
			const auto result = audit_tree(root, nullptr, seen, 1);
			if (result != player_snapshot_capture_result::ok)
				return result;
		}
		for (const obj_data *root = mob->carrying; root; root = root->next_content)
		{
			if (root->loc_p != LOC_CARRIED || root->loc.carrying != mob)
				return player_snapshot_capture_result::malformed_source;
			const auto result = audit_tree(root, nullptr, seen, 1);
			if (result != player_snapshot_capture_result::ok)
				return result;
		}
		quest_mobile_native_image candidate;
		candidate.reference = reference;
		candidate.state = quest_mobile_lifetime_state::live;
		candidate.last_transition_operation = last_transition;
		size_t bytes = sizeof(player_snapshot);
		for (int slot = 0; slot < MAX_WEAR; ++slot)
		{
			if (!mob->equipment[slot])
				continue;
			const auto result = append_tree(mob->equipment[slot],
							static_cast<int16_t>(slot + 1),
							candidate.items, bytes);
			if (result != player_snapshot_capture_result::ok)
				return result;
		}
		for (const obj_data *root = mob->carrying; root; root = root->next_content)
		{
			const auto result = append_tree(root, 0, candidate.items, bytes);
			if (result != player_snapshot_capture_result::ok)
				return result;
		}
		std::vector<uint8_t> canonical;
		const auto result = quest_mobile_native_image_encode(candidate, &canonical);
		if (result == player_snapshot_codec_result::allocation_failure)
			return player_snapshot_capture_result::retryable_allocation_failure;
		if (result == player_snapshot_codec_result::limit_exceeded)
			return player_snapshot_capture_result::limit_exceeded;
		if (result != player_snapshot_codec_result::ok)
			return player_snapshot_capture_result::malformed_source;
		*output = std::move(candidate);
		return player_snapshot_capture_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_capture_result::retryable_allocation_failure;
	}
}
