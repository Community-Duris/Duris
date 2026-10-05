#include "world/quest_mobile_native.h"

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"

#include <algorithm>
#include <bit>
#include <climits>
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
constexpr std::array<uint8_t, 8> image_magic = { 'Q', 'M', 'N', 'I', 'M', 'G', 0, 0 };
constexpr uint8_t literal_mask = STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3;

bool nonzero(const critical_operation_id &id)
{
	return std::any_of(id.bytes.begin(), id.bytes.end(),
			   [](uint8_t value) { return value != 0; });
}

bool cash_valid(const quest_mobile_native_image &image) noexcept
{
	if (!image.cash)
		return true; // Historical absence remains unknown.
	if (!image.cash->revision)
		return false;
	return std::all_of(
		image.cash->denominations.amount.begin(), image.cash->denominations.amount.end(),
		[&](int64_t amount)
		{
			return amount >= 0 && amount <= INT_MAX &&
			       (image.state != quest_mobile_lifetime_state::retired || amount == 0);
		});
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

player_snapshot_codec_result
quest_mobile_native_image_encode(const quest_mobile_native_image &image,
				 std::vector<uint8_t> *output) noexcept
{
	if (!output || !nonzero(image.last_transition_operation) || !cash_valid(image) ||
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
		const size_t overhead = image.cash ? QUEST_MOBILE_NATIVE_CASH_IMAGE_OVERHEAD :
						     QUEST_MOBILE_NATIVE_IMAGE_OVERHEAD;
		if (blob.size() > PLAYER_SNAPSHOT_MAX_BYTES - overhead)
			return player_snapshot_codec_result::limit_exceeded;
		std::vector<uint8_t> candidate(overhead + blob.size(), 0);
		std::copy(image_magic.begin(), image_magic.end(), candidate.begin());
		size_t offset = image_magic.size();
		put<uint16_t>(candidate.data(), offset,
			      image.cash ? QUEST_MOBILE_NATIVE_CASH_IMAGE_VERSION :
					   QUEST_MOBILE_NATIVE_VERSION);
		put<uint8_t>(candidate.data(), offset, static_cast<uint8_t>(image.state));
		put<uint8_t>(candidate.data(), offset, 0);
		put<uint32_t>(candidate.data(), offset, candidate.size());
		std::copy(reference.begin(), reference.end(), candidate.begin() + offset);
		offset += reference.size();
		std::copy(image.last_transition_operation.bytes.begin(),
			  image.last_transition_operation.bytes.end(), candidate.begin() + offset);
		offset += image.last_transition_operation.bytes.size();
		if (image.cash)
		{
			put<uint64_t>(candidate.data(), offset, image.cash->revision);
			for (int64_t amount : image.cash->denominations.amount)
				put<int64_t>(candidate.data(), offset, amount);
		}
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
		const uint16_t version = get<uint16_t>(bytes.data(), offset);
		if (version != QUEST_MOBILE_NATIVE_VERSION &&
		    version != QUEST_MOBILE_NATIVE_CASH_IMAGE_VERSION)
			return player_snapshot_codec_result::unsupported_version;
		if (version == QUEST_MOBILE_NATIVE_CASH_IMAGE_VERSION &&
		    bytes.size() < QUEST_MOBILE_NATIVE_CASH_IMAGE_OVERHEAD)
			return player_snapshot_codec_result::truncated;
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
		if (version == QUEST_MOBILE_NATIVE_CASH_IMAGE_VERSION)
		{
			candidate.cash.emplace();
			candidate.cash->revision = get<uint64_t>(bytes.data(), offset);
			for (auto &amount : candidate.cash->denominations.amount)
				amount = get<int64_t>(bytes.data(), offset);
		}
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

bool quest_mobile_native_cash_transition_valid(const quest_mobile_native_image *before,
					       const quest_mobile_native_image &after) noexcept
{
	if (!after.cash || !cash_valid(after))
		return false;
	if (!before)
		return after.state == quest_mobile_lifetime_state::live &&
		       after.cash->revision == 1 && after.reference.mobile_revision == 1;
	if (!before->cash || !cash_valid(*before))
		return false;
	if (before->cash->denominations.amount == after.cash->denominations.amount)
		return after.cash->revision == before->cash->revision &&
		       after.reference.mobile_revision >= before->reference.mobile_revision;
	return before->cash->revision != UINT64_MAX &&
	       before->reference.mobile_revision != UINT64_MAX &&
	       after.cash->revision == before->cash->revision + 1 &&
	       after.reference.mobile_revision == before->reference.mobile_revision + 1;
}

player_snapshot_capture_result
quest_mobile_native_capture(P_char mob, const quest_mobile_native_reference &reference,
			    quest_mobile_lifetime_state state,
			    const critical_operation_id &last_transition,
			    quest_mobile_native_image *output) noexcept
{
	if (state != quest_mobile_lifetime_state::live || !mob || !output || !IS_NPC(mob) ||
	    !mob->only.npc || !quest_mobile_native_reference_valid(reference) ||
	    !nonzero(last_transition) || !mob_index || GET_RNUM(mob) < 0 ||
	    GET_RNUM(mob) > top_of_mobt ||
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

player_snapshot_capture_result
quest_mobile_native_capture(P_char mob, const quest_mobile_native_reference &reference,
			    quest_mobile_lifetime_state state,
			    const critical_operation_id &last_transition, uint64_t cash_revision,
			    quest_mobile_native_image *output) noexcept
{
	if (!output || !cash_revision)
		return player_snapshot_capture_result::invalid_identity;
	quest_mobile_native_image candidate;
	const auto captured =
		quest_mobile_native_capture(mob, reference, state, last_transition, &candidate);
	if (captured != player_snapshot_capture_result::ok)
		return captured;
	try
	{
		candidate.cash.emplace();
		candidate.cash->revision = cash_revision;
		candidate.cash->denominations.amount = { GET_COPPER(mob), GET_SILVER(mob),
							 GET_GOLD(mob), GET_PLATINUM(mob) };
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
