#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "world/quest_mobile_native.h"
#include "item/item_transfer_command.h"

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

player_snapshot_codec_result
quest_mobile_native_image_preflight(std::span<const uint8_t> bytes,
				    quest_mobile_native_image_allocation_profile *output) noexcept
{
	using result = player_snapshot_codec_result;
	if (!output)
		return result::invalid_value;
	if (bytes.size() < QUEST_MOBILE_NATIVE_IMAGE_OVERHEAD)
		return result::truncated;
	if (bytes.size() > PLAYER_SNAPSHOT_MAX_BYTES)
		return result::limit_exceeded;
	// Sizing accepts untrusted framing; never call OpenSSL before admission.
	if (!std::equal(image_magic.begin(), image_magic.end(), bytes.begin()))
		return result::invalid_value;
	size_t offset = image_magic.size();
	const uint16_t version = get<uint16_t>(bytes.data(), offset);
	if (version != QUEST_MOBILE_NATIVE_VERSION &&
	    version != QUEST_MOBILE_NATIVE_CASH_IMAGE_VERSION)
		return result::unsupported_version;
	if (version == QUEST_MOBILE_NATIVE_CASH_IMAGE_VERSION &&
	    bytes.size() < QUEST_MOBILE_NATIVE_CASH_IMAGE_OVERHEAD)
		return result::truncated;
	quest_mobile_native_image header;
	header.state = static_cast<quest_mobile_lifetime_state>(get<uint8_t>(bytes.data(), offset));
	if (get<uint8_t>(bytes.data(), offset) ||
	    get<uint32_t>(bytes.data(), offset) != bytes.size())
		return result::invalid_value;
	// Fixed original reference framing/value checks only. Reference/image
	// digests and exact canonical bytes are validated by the original decoder
	// AFTER the prospective C++ allocation reservation.
	const auto encoded_reference = bytes.subspan(offset, QUEST_MOBILE_NATIVE_REFERENCE_BYTES);
	constexpr std::array<uint8_t, 8> reference_magic = { 'Q', 'M', 'N', 'R', 'E', 'F', 0, 0 };
	if (!std::equal(reference_magic.begin(), reference_magic.end(), encoded_reference.begin()))
		return result::invalid_value;
	size_t ref_offset = reference_magic.size();
	if (get<uint16_t>(encoded_reference.data(), ref_offset) != QUEST_MOBILE_NATIVE_VERSION)
		return result::unsupported_version;
	header.reference.provenance = static_cast<quest_mobile_birth_provenance>(
		get<uint8_t>(encoded_reference.data(), ref_offset));
	if (get<uint8_t>(encoded_reference.data(), ref_offset) ||
	    get<uint32_t>(encoded_reference.data(), ref_offset) !=
		    QUEST_MOBILE_NATIVE_REFERENCE_BYTES)
		return result::invalid_value;
	header.reference.mobile_instance_id = get<uint64_t>(encoded_reference.data(), ref_offset);
	std::copy_n(encoded_reference.data() + ref_offset,
		    header.reference.birth_operation.bytes.size(),
		    header.reference.birth_operation.bytes.begin());
	ref_offset += header.reference.birth_operation.bytes.size();
	if (economic_source_event_decode(
		    encoded_reference.subspan(ref_offset, ECONOMIC_SOURCE_EVENT_BYTES),
		    &header.reference.birth_source) != economic_accounting_error::ok)
		return result::invalid_value;
	ref_offset += ECONOMIC_SOURCE_EVENT_BYTES;
	header.reference.mobile_vnum = get<int32_t>(encoded_reference.data(), ref_offset);
	header.reference.birthplace_vnum = get<int32_t>(encoded_reference.data(), ref_offset);
	header.reference.reset_zone_vnum = get<int32_t>(encoded_reference.data(), ref_offset);
	header.reference.mobile_revision = get<uint64_t>(encoded_reference.data(), ref_offset);
	header.reference.stock_revision = get<uint64_t>(encoded_reference.data(), ref_offset);
	if (ref_offset != QUEST_MOBILE_NATIVE_REFERENCE_BYTES - SHA256_DIGEST_LENGTH ||
	    !quest_mobile_native_reference_valid(header.reference))
		return result::invalid_value;
	offset += QUEST_MOBILE_NATIVE_REFERENCE_BYTES;
	std::copy_n(bytes.data() + offset, header.last_transition_operation.bytes.size(),
		    header.last_transition_operation.bytes.begin());
	offset += header.last_transition_operation.bytes.size();
	if (version == QUEST_MOBILE_NATIVE_CASH_IMAGE_VERSION)
	{
		header.cash.emplace();
		header.cash->revision = get<uint64_t>(bytes.data(), offset);
		for (auto &amount : header.cash->denominations.amount)
			amount = get<int64_t>(bytes.data(), offset);
	}
	if (!nonzero(header.last_transition_operation) || !cash_valid(header) ||
	    (header.state != quest_mobile_lifetime_state::live &&
	     header.state != quest_mobile_lifetime_state::retired))
		return result::invalid_value;
	const uint32_t length = get<uint32_t>(bytes.data(), offset);
	if (length != bytes.size() - offset - SHA256_DIGEST_LENGTH)
		return result::invalid_value;
	quest_mobile_native_image_allocation_profile profile;
	const auto code =
		player_item_snapshot_list_preflight(bytes.data() + offset, length, &profile.items);
	if (code != result::ok)
		return code;
	if (header.state == quest_mobile_lifetime_state::retired && profile.items.item_count)
		return result::invalid_value;
	profile.canonical_image_bytes = bytes.size();
	profile.storage_policy_supported = profile.items.fresh_decode_storage_policy_supported &&
					   profile.items.canonical_encoder_storage_policy_supported;
#if defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI
	if (profile.storage_policy_supported)
	{
		const auto add = [](size_t &total, size_t amount) noexcept
		{
			if (amount > SIZE_MAX - total)
				return false;
			total += amount;
			return true;
		};
		const auto product = [](size_t count, size_t width, size_t &out) noexcept
		{
			if (count && width > SIZE_MAX / count)
				return false;
			out = count * width;
			return true;
		};
		// Original forest_valid's actual default hash policy: one fresh node
		// precedes a possible rehash; old/new heap bucket arrays coexist.
		// _M_need_rehash is allocation-free scalar/table policy, not a set.
		using node = std::__detail::_Hash_node<
			uint64_t, std::__cache_default<uint64_t, std::hash<uint64_t>>::value>;
		std::__detail::_Prime_rehash_policy policy;
		size_t buckets = 1, heap_buckets = 0;
		for (size_t index = 0; index < profile.items.item_count; ++index)
		{
			size_t nodes = 0, old_buckets = 0, new_buckets = 0;
			if (!product(index + 1, sizeof(node), nodes) ||
			    !product(heap_buckets, sizeof(std::__detail::_Hash_node_base *),
				     old_buckets))
				return result::limit_exceeded;
			const auto rehash = policy._M_need_rehash(buckets, index, 1);
			if (rehash.first)
			{
				if (!product(rehash.second,
					     sizeof(std::__detail::_Hash_node_base *), new_buckets))
					return result::limit_exceeded;
				buckets = heap_buckets = rehash.second;
			}
			if (!add(nodes, old_buckets) || !add(nodes, new_buckets))
				return result::limit_exceeded;
			profile.forest_validation_heap_peak_bytes =
				std::max(profile.forest_validation_heap_peak_bytes, nodes);
		}
		profile.forest_validation_inline_bytes =
			sizeof(std::unordered_set<uint64_t>) +
			sizeof(std::array<int32_t, PLAYER_SNAPSHOT_MAX_DEPTH>);
		const auto &items = profile.items;
		if (items.decoded_payload_bytes < sizeof(std::vector<player_item_snapshot>))
			return result::invalid_value;
		const size_t item_payload =
			items.decoded_payload_bytes - sizeof(std::vector<player_item_snapshot>);
		profile.decoded_image_payload_bytes = sizeof(quest_mobile_native_image);
		if (!add(profile.decoded_image_payload_bytes, item_payload))
			return result::limit_exceeded;
		size_t item_validation = items.canonical_encoded_capacity_bytes;
		if (!add(item_validation, items.item_codec_decoder_object_bytes) ||
		    !add(item_validation, sizeof(std::vector<player_item_snapshot>)) ||
		    !add(item_validation, items.decoded_payload_bytes) ||
		    !add(item_validation, items.relationship_scratch_bytes))
			return result::limit_exceeded;
		size_t item_roundtrip = items.canonical_encoder_object_bytes;
		if (!add(item_roundtrip, std::max(items.canonical_encoded_reallocation_peak_bytes,
						  item_validation)))
			return result::limit_exceeded;
		item_roundtrip = std::max(item_roundtrip, items.relationship_scratch_bytes);
		constexpr size_t reference_bytes =
			sizeof(std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES>);
		size_t forest = reference_bytes, roundtrip = reference_bytes,
		       image_build = reference_bytes;
		if (!add(forest, profile.forest_validation_inline_bytes) ||
		    !add(forest, profile.forest_validation_heap_peak_bytes) ||
		    !add(roundtrip, sizeof(std::vector<uint8_t>)) ||
		    !add(roundtrip, item_roundtrip) ||
		    !add(image_build, sizeof(std::vector<uint8_t>)) ||
		    !add(image_build, items.canonical_encoded_capacity_bytes) ||
		    !add(image_build, sizeof(std::vector<uint8_t>)) ||
		    !add(image_build, profile.canonical_image_bytes))
			return result::limit_exceeded;
		profile.canonical_encode_working_bytes =
			std::max({ forest, roundtrip, image_build });
		// Original item decode owns a temporary vector before moving into image;
		// later image re-encode holds its own decoded image and output vector.
		size_t decode_items = sizeof(quest_mobile_native_image);
		size_t decode_canonical = profile.decoded_image_payload_bytes;
		if (!add(decode_items, items.item_codec_decoder_object_bytes) ||
		    !add(decode_items, items.decoded_payload_bytes) ||
		    !add(decode_items, items.relationship_scratch_bytes) ||
		    !add(decode_canonical, sizeof(std::vector<uint8_t>)) ||
		    !add(decode_canonical, profile.canonical_encode_working_bytes))
			return result::limit_exceeded;
		profile.decode_working_bytes = std::max(decode_items, decode_canonical);
	}
#else
	profile.storage_policy_supported = false;
#endif
	*output = profile;
	return result::ok;
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

namespace
{
// The compatibility image reader and items-only observer share the exact native
// full-forest audit. This helper supplies no transition, cash or source facts.
player_snapshot_capture_result
native_items_capture(P_char mob, const quest_mobile_native_reference &reference,
		     std::vector<player_item_snapshot> *output) noexcept
{
	if (!mob || !output || !IS_NPC(mob) || !mob->only.npc ||
	    !quest_mobile_native_reference_valid(reference) || !mob_index || GET_RNUM(mob) < 0 ||
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
		std::vector<player_item_snapshot> candidate;
		size_t bytes = sizeof(player_snapshot);
		for (int slot = 0; slot < MAX_WEAR; ++slot)
		{
			if (!mob->equipment[slot])
				continue;
			const auto result = append_tree(mob->equipment[slot],
							static_cast<int16_t>(slot + 1), candidate,
							bytes);
			if (result != player_snapshot_capture_result::ok)
				return result;
		}
		for (const obj_data *root = mob->carrying; root; root = root->next_content)
		{
			const auto result = append_tree(root, 0, candidate, bytes);
			if (result != player_snapshot_capture_result::ok)
				return result;
		}

		std::vector<uint8_t> canonical;
		const auto code = forest_valid(candidate);
		const auto result =
			code == player_snapshot_codec_result::ok ?
				player_item_snapshot_list_encode(candidate, &canonical) :
				code;
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
}

player_snapshot_capture_result
quest_mobile_native_items_observe(P_char mob, const quest_mobile_native_reference &reference,
				  std::vector<player_item_snapshot> *output) noexcept
{
	if (!nevent_is_game_thread())
		return player_snapshot_capture_result::invalid_identity;
	return native_items_capture(mob, reference, output);
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
		quest_mobile_native_image candidate;
		candidate.reference = reference;
		candidate.state = quest_mobile_lifetime_state::live;
		candidate.last_transition_operation = last_transition;
		const auto captured = native_items_capture(mob, reference, &candidate.items);
		if (captured != player_snapshot_capture_result::ok)
			return captured;
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

player_snapshot_codec_result
quest_mobile_native_items_transition(std::span<const player_item_snapshot> original_items,
				     const quest_mobile_native_reference &original_reference,
				     const item_transfer_payload &payload,
				     std::vector<player_item_snapshot> *after) noexcept
{
	if (!after || original_items.size() > PLAYER_SNAPSHOT_MAX_OBJECTS ||
	    !payload.native_mobile.present ||
	    (payload.native_mobile.action != item_native_mobile_action::acceptance &&
	     payload.native_mobile.action != item_native_mobile_action::consumption) ||
	    !payload.item_blob_size || payload.item_blob_size > payload.item_blob.size() ||
	    original_reference.mobile_revision == UINT64_MAX ||
	    original_reference.stock_revision == UINT64_MAX)
		return player_snapshot_codec_result::invalid_value;
	try
	{
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> expected{}, actual{};
		if (quest_mobile_native_reference_encode(original_reference, &actual) !=
			    player_snapshot_codec_result::ok ||
		    quest_mobile_native_reference_encode(payload.native_mobile.reference,
							 &expected) !=
			    player_snapshot_codec_result::ok ||
		    actual != expected)
			return player_snapshot_codec_result::invalid_value;
		std::vector<player_item_snapshot> before(original_items.begin(),
							 original_items.end());
		auto code = forest_valid(before);
		if (code != player_snapshot_codec_result::ok)
			return code;
		std::vector<uint8_t> encoded;
		code = player_item_snapshot_list_encode(before, &encoded);
		if (code != player_snapshot_codec_result::ok)
			return code;
		std::vector<player_item_snapshot> selected;
		code = player_item_snapshot_list_decode(payload.item_blob.data(),
							payload.item_blob_size, &selected);
		if (code != player_snapshot_codec_result::ok)
			return code;
		if (selected.empty() || selected.size() != payload.item_count ||
		    forest_valid(selected) != player_snapshot_codec_result::ok)
			return player_snapshot_codec_result::invalid_value;
		if (payload.native_mobile.action == item_native_mobile_action::acceptance &&
		    std::count_if(selected.begin(), selected.end(), [](const auto &item)
				  { return item.parent_index == PLAYER_SNAPSHOT_NO_PARENT; }) != 1)
			return player_snapshot_codec_result::invalid_value;
		std::vector<player_item_snapshot> candidate = before;
		const bool acceptance = payload.native_mobile.action ==
					item_native_mobile_action::acceptance;
		if (acceptance)
		{
			if (selected.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - before.size())
				return player_snapshot_codec_result::limit_exceeded;
			for (const auto &item : selected)
				if (std::any_of(before.begin(), before.end(), [&](const auto &old)
						{ return old.object_uid == item.object_uid; }))
					return player_snapshot_codec_result::invalid_value;
			// obj_to_char_checked inserts before the first carried root of the same
			// prototype, or at the carried head if none. Never group equipment roots.
			size_t inventory = before.size();
			size_t insertion = before.size();
			for (size_t i = 0; i < before.size(); ++i)
				if (before[i].parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
				    !before[i].equipment_slot)
				{
					if (inventory == before.size())
						inventory = i;
					if (before[i].vnum == selected[0].vnum)
					{
						insertion = i;
						break;
					}
				}
			if (insertion == before.size())
				insertion = inventory;
			for (auto &item : candidate)
				if (item.parent_index >= static_cast<int32_t>(insertion))
					item.parent_index += static_cast<int32_t>(selected.size());
			selected[0].equipment_slot = 0;
			for (auto &item : selected)
				if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
					item.parent_index += static_cast<int32_t>(insertion);
			candidate.insert(candidate.begin() + insertion, selected.begin(),
					 selected.end());
		}
		else
		{
			std::unordered_set<uint64_t> removed;
			for (const auto &item : selected)
				removed.insert(item.object_uid);
			std::vector<player_item_snapshot> observed, retained;
			std::vector<int32_t> selected_index(before.size(),
							    PLAYER_SNAPSHOT_NO_PARENT);
			std::vector<int32_t> retained_index(before.size(),
							    PLAYER_SNAPSHOT_NO_PARENT);
			for (size_t i = 0; i < before.size(); ++i)
			{
				auto item = before[i];
				const bool erase = removed.count(item.object_uid) != 0;
				const auto parent = item.parent_index;
				if (parent != PLAYER_SNAPSHOT_NO_PARENT &&
				    (removed.count(before[parent].object_uid) != 0) != erase)
					return player_snapshot_codec_result::
						invalid_value; // No partial subtree retirement.
				auto &indexes = erase ? selected_index : retained_index;
				auto &items = erase ? observed : retained;
				if (parent != PLAYER_SNAPSHOT_NO_PARENT)
					item.parent_index = indexes[parent];
				indexes[i] = static_cast<int32_t>(items.size());
				items.push_back(std::move(item));
			}
			std::vector<uint8_t> observed_bytes, selected_bytes;
			code = player_item_snapshot_list_encode(observed, &observed_bytes);
			if (code == player_snapshot_codec_result::ok)
				code = player_item_snapshot_list_encode(selected, &selected_bytes);
			if (code != player_snapshot_codec_result::ok)
				return code;
			if (observed_bytes != selected_bytes)
				return player_snapshot_codec_result::invalid_value;
			candidate = std::move(retained);
		}

		code = forest_valid(candidate);
		if (code == player_snapshot_codec_result::ok)
			code = player_item_snapshot_list_encode(candidate, &encoded);
		if (code != player_snapshot_codec_result::ok)
			return code;
		*after = std::move(candidate);
		return player_snapshot_codec_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
}

player_snapshot_codec_result quest_mobile_native_item_transition(
	const quest_mobile_native_image &before, const item_transfer_payload &payload,
	const critical_operation_id &operation, quest_mobile_native_image *after) noexcept
{
	if (!after || !nonzero(operation) || !payload.native_mobile.present ||
	    (payload.native_mobile.action != item_native_mobile_action::acceptance &&
	     payload.native_mobile.action != item_native_mobile_action::consumption) ||
	    !payload.item_blob_size || payload.item_blob_size > payload.item_blob.size() ||
	    before.state != quest_mobile_lifetime_state::live || !before.cash ||
	    before.reference.mobile_revision == UINT64_MAX ||
	    before.reference.stock_revision == UINT64_MAX)
		return player_snapshot_codec_result::invalid_value;
	try
	{
		std::vector<uint8_t> canonical;
		auto code = quest_mobile_native_image_encode(before, &canonical);
		if (code != player_snapshot_codec_result::ok)
			return code;
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> expected{}, actual{};
		if (quest_mobile_native_reference_encode(before.reference, &actual) !=
			    player_snapshot_codec_result::ok ||
		    quest_mobile_native_reference_encode(payload.native_mobile.reference,
							 &expected) !=
			    player_snapshot_codec_result::ok ||
		    actual != expected)
			return player_snapshot_codec_result::invalid_value;
		quest_mobile_native_image candidate = before;
		code = quest_mobile_native_items_transition(before.items, before.reference, payload,
							    &candidate.items);
		if (code != player_snapshot_codec_result::ok)
			return code;
		if (payload.native_cost.present)
		{
			std::vector<uint8_t> exact_cost;
			if (payload.native_mobile.action !=
				    item_native_mobile_action::consumption ||
			    !payload.native_cost.wallet_mapping_id ||
			    native_quest_cost_projection_encode(payload.native_cost.projection,
								&exact_cost) !=
				    native_quest_cost_projection_result::ok ||
			    payload.native_cost.projection.before_revision !=
				    before.cash->revision ||
			    payload.native_cost.projection.before !=
				    before.cash->denominations.amount)
				return player_snapshot_codec_result::invalid_value;
			candidate.cash->revision = payload.native_cost.projection.after_revision;
			candidate.cash->denominations.amount = payload.native_cost.projection.after;
		}
		++candidate.reference.mobile_revision;
		++candidate.reference.stock_revision;
		candidate.last_transition_operation = operation;
		code = quest_mobile_native_image_encode(candidate, &canonical);
		if (code != player_snapshot_codec_result::ok)
			return code;
		if (!quest_mobile_native_cash_transition_valid(&before, candidate))
			return player_snapshot_codec_result::invalid_value;
		*after = std::move(candidate);
		return player_snapshot_codec_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
}

player_snapshot_codec_result quest_mobile_native_fee_transition(
	const quest_mobile_native_image &before, const item_transfer_payload &payload,
	const critical_operation_id &operation, quest_mobile_native_image *output) noexcept
{
	using result = player_snapshot_codec_result;
	if (!output || !payload.native_cost.fee_only || !nonzero(operation) ||
	    before.state != quest_mobile_lifetime_state::live || !before.cash ||
	    before.reference.mobile_revision == UINT64_MAX ||
	    !(payload.native_recovery.present ?
		      item_transfer_native_mobile_recovery_shape_valid(payload) :
		      item_transfer_native_mobile_shape_valid(payload)))
		return result::invalid_value;
	try
	{
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> original{}, bound{};
		if (quest_mobile_native_reference_encode(before.reference, &original) !=
			    result::ok ||
		    quest_mobile_native_reference_encode(payload.native_mobile.reference, &bound) !=
			    result::ok ||
		    original != bound ||
		    before.cash->revision != payload.native_cost.projection.before_revision ||
		    before.cash->denominations.amount != payload.native_cost.projection.before)
			return result::invalid_value;
		std::vector<uint8_t> canonical;
		auto status = quest_mobile_native_image_encode(before, &canonical);
		if (status != result::ok)
			return status;
		auto candidate = before;
		candidate.cash->revision = payload.native_cost.projection.after_revision;
		candidate.cash->denominations.amount = payload.native_cost.projection.after;
		++candidate.reference.mobile_revision;
		candidate.last_transition_operation = operation;
		if (!quest_mobile_native_cash_transition_valid(&before, candidate))
			return result::invalid_value;
		status = quest_mobile_native_image_encode(candidate, &canonical);
		if (status != result::ok)
			return status;
		*output = std::move(candidate);
		return result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return result::allocation_failure;
	}
}

player_snapshot_codec_result quest_mobile_native_money_transition(
	const quest_mobile_native_image &before, const item_transfer_payload &payload,
	const critical_operation_id &operation, quest_mobile_native_image *output) noexcept
{
	using result = player_snapshot_codec_result;
	if (!output || !payload.native_money.present || !nonzero(operation) ||
	    before.state != quest_mobile_lifetime_state::live || !before.cash ||
	    before.reference.mobile_revision == UINT64_MAX ||
	    !(payload.native_recovery.present ?
		      item_transfer_native_mobile_recovery_shape_valid(payload) :
		      item_transfer_native_mobile_shape_valid(payload)))
		return result::invalid_value;
	try
	{
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> original{}, bound{};
		if (quest_mobile_native_reference_encode(before.reference, &original) !=
			    result::ok ||
		    quest_mobile_native_reference_encode(payload.native_mobile.reference, &bound) !=
			    result::ok ||
		    original != bound ||
		    before.cash->revision !=
			    payload.native_money.projection.mobile_before_revision ||
		    before.cash->denominations.amount !=
			    payload.native_money.projection.mobile_before)
			return result::invalid_value;
		std::vector<uint8_t> canonical;
		auto status = quest_mobile_native_image_encode(before, &canonical);
		if (status != result::ok)
			return status;
		auto candidate = before;
		candidate.cash->revision = payload.native_money.projection.mobile_after_revision;
		candidate.cash->denominations.amount = payload.native_money.projection.mobile_after;
		++candidate.reference.mobile_revision;
		candidate.last_transition_operation = operation;
		if (!quest_mobile_native_cash_transition_valid(&before, candidate))
			return result::invalid_value;
		status = quest_mobile_native_image_encode(candidate, &canonical);
		if (status != result::ok)
			return status;
		*output = std::move(candidate);
		return result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return result::allocation_failure;
	}
}

namespace
{
using native_image_reserve_fn = bool (*)(size_t, void *) noexcept;
bool native_image_add(size_t &value, size_t added) noexcept
{
	if (added > SIZE_MAX - value)
		return false;
	value += added;
	return true;
}
bool native_image_admit(size_t outer, size_t extra, native_image_reserve_fn reserve,
			void *context) noexcept
{
	return extra <= SIZE_MAX - outer && reserve && reserve(outer + extra, context);
}
#if defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI
constexpr size_t native_image_hash_scan_objects =
	sizeof(std::__detail::_Prime_rehash_policy) + sizeof(std::pair<bool, size_t>);
bool native_image_forest_peak(size_t count, size_t &peak) noexcept
{
	using node = std::__detail::_Hash_node<
		uint64_t, std::__cache_default<uint64_t, std::hash<uint64_t>>::value>;
	std::__detail::_Prime_rehash_policy policy;
	size_t buckets = 1, heap_buckets = 0;
	peak = 0;
	for (size_t index = 0; index < count; ++index)
	{
		if (index + 1 > SIZE_MAX / sizeof(node) ||
		    heap_buckets > SIZE_MAX / sizeof(std::__detail::_Hash_node_base *))
			return false;
		size_t request = (index + 1) * sizeof(node);
		if (!native_image_add(request,
				      heap_buckets * sizeof(std::__detail::_Hash_node_base *)))
			return false;
		const auto rehash = policy._M_need_rehash(buckets, index, 1);
		if (rehash.first)
		{
			if (rehash.second > SIZE_MAX / sizeof(std::__detail::_Hash_node_base *) ||
			    !native_image_add(request,
					      rehash.second *
						      sizeof(std::__detail::_Hash_node_base *)))
				return false;
			buckets = heap_buckets = rehash.second;
		}
		peak = std::max(peak, request);
	}
	return native_image_add(peak, sizeof(std::unordered_set<uint64_t>)) &&
	       native_image_add(peak, sizeof(std::array<int32_t, PLAYER_SNAPSHOT_MAX_DEPTH>));
}
#endif
} // namespace

namespace
{
struct native_image_encode_workspace
{
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> reference{};
	std::vector<uint8_t> blob, candidate;
};
struct native_image_decode_workspace
{
	quest_mobile_native_image candidate;
	std::vector<uint8_t> canonical;
	player_item_snapshot_list_allocation_profile items;
	std::span<const uint8_t> reference;
};
}

namespace
{
constexpr size_t native_image_fixed_sha_assembly_frames =
	2 * 4 * 64 + 4 * sizeof(void *) + 6 * sizeof(uint64_t) + (256 * 4 - 1) + 2 * sizeof(void *);
constexpr size_t native_image_fixed_sha_c_small_frames =
	16 * sizeof(unsigned int) + 12 * sizeof(unsigned int) + sizeof(unsigned int) + sizeof(int) +
	sizeof(void *);
constexpr size_t native_image_fixed_sha_c_normal_frames = 16 * sizeof(unsigned int) +
							  11 * sizeof(unsigned int) +
							  2 * sizeof(int) + 2 * sizeof(void *);
constexpr size_t native_image_fixed_sha_init_frames = sizeof(void *) + sizeof(int);
constexpr size_t native_image_fixed_sha_update_frames = 2 * sizeof(void *) + sizeof(size_t) +
							2 * sizeof(void *) + sizeof(unsigned int) +
							sizeof(size_t) + sizeof(int);
constexpr size_t native_image_fixed_sha_final_frames = 3 * sizeof(void *) + sizeof(size_t) +
						       sizeof(unsigned long) +
						       sizeof(unsigned int) + sizeof(int);
// Real SHA256_Init/Update/Final memcpy/memset call arguments and result;
// OPENSSL_cleanse(buf,len) and the x86_64 leaf's return address. C fallback
// cleanse's actual ptr/len/pointer-result carriers are included as well.
constexpr size_t native_image_fixed_sha_memory_frames =
	2 * sizeof(void *) + sizeof(size_t) + sizeof(int) + sizeof(void *);
constexpr size_t native_image_fixed_sha_cleanse_frames =
	// mem_clr.c ptr/len and loaded volatile function pointer remain live
	// through its authentic indirect memset leaf; asm fallback is smaller.
	2 * sizeof(void *) + sizeof(size_t) + native_image_fixed_sha_memory_frames;
constexpr size_t native_image_fixed_sha_block_frames =
	// C compression ctx/in/num plus its actual typed locals; assembly term
	// already includes its own real caller return address.
	std::max(native_image_fixed_sha_assembly_frames,
		 2 * sizeof(void *) + sizeof(size_t) +
			 std::max(native_image_fixed_sha_c_small_frames,
				  native_image_fixed_sha_c_normal_frames));
constexpr size_t native_image_fixed_sha_frames =
	std::max(native_image_fixed_sha_init_frames,
		 std::max(native_image_fixed_sha_update_frames,
			  native_image_fixed_sha_final_frames)) +
	std::max(native_image_fixed_sha_block_frames,
		 std::max(native_image_fixed_sha_memory_frames,
			  native_image_fixed_sha_cleanse_frames));
constexpr size_t native_image_fixed_control_frames =
	// admit outer/request/reserve/context/result; add total/bytes/result;
	// profile's bool result. Callback-private frames remain callback-owned.
	3 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(player_snapshot_codec_result) +
	2 * sizeof(bool);
constexpr size_t native_image_fixed_copy_frames =
	// copy/copy_move_a/a1/a2/copy_m actual three iterator args and return;
	// miter/niter/wrap, real length/Num and runtime memcpy/memmove args/result.
	5 * (3 * sizeof(void *) + sizeof(void *)) + 2 * (sizeof(void *) + sizeof(void *)) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) +
	// copy_n, actual n conversion, forward copy_n random access tag.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(std::random_access_iterator_tag) +
	2 * sizeof(size_t) +
	// Array/span begin/end/data/size, _S_ptr and subspan true declarations.
	8 * (2 * sizeof(void *)) + 4 * (sizeof(void *) + sizeof(size_t)) +
	sizeof(std::span<const uint8_t>) + 2 * sizeof(size_t) + sizeof(void *);
constexpr size_t native_image_fixed_equal_frames =
	4 * (3 * sizeof(void *) + sizeof(bool)) + sizeof(bool) + sizeof(size_t) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(int);
constexpr size_t native_image_fixed_valid_frames =
	// nonzero(id), original any_of->none_of->find_if->two __find_if calls:
	// empty closure params/returned wrapper and real RA trip count/tag.
	sizeof(void *) + sizeof(bool) + 3 * (2 * sizeof(void *) + sizeof(char) + sizeof(bool)) +
	2 * (3 * sizeof(void *) + sizeof(char)) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) +
	// __pred_iter/_Iter_pred source ctor/operator and lambda(this,byte)/result,
	// real move refs; array pointer begin/end needs no heap.
	6 * sizeof(void *) + 4 * sizeof(char) + 3 * sizeof(bool) + sizeof(uint8_t) +
	4 * (2 * sizeof(void *)) +
	// economic_source_event_valid and actual critical_operation_id_is_zero:
	// references/results, range begin/end/byte and fixed-array query scopes.
	2 * (sizeof(void *) + sizeof(bool)) + 2 * sizeof(void *) + sizeof(uint8_t) +
	4 * (2 * sizeof(void *));
constexpr size_t native_image_fixed_put_get_frames =
	// Original put/get: bytes/offset refs, largest value/bits, n and returned
	// integer/bit_cast input/result. These are fixed, allocation-free helpers.
	4 * sizeof(void *) + 5 * sizeof(uint64_t) + 2 * sizeof(size_t);
constexpr size_t native_image_fixed_memcmp_frames =
	// Pinned cpuid.c fallback: in_a/in_b/len/i/a/b/x + int result. Real
	// x86_64cpuid.pl CRYPTO_memcmp has no pushes/sub/spill or nested call;
	// only its true caller return address is an explicit assembly stack term.
	4 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(unsigned char) + sizeof(int) +
	sizeof(void *);
// Actual image entry locals/parameters and nested allocation-free operations.
// The complete sum is retained, rather than assuming helper bodies inline.
constexpr size_t native_image_encode_entry_frames =
	// image/output/reserve/context and outer/base/live/hash_live/forest/overhead/offset;
	4 * sizeof(void *) + 7 * sizeof(size_t) + sizeof(player_snapshot_codec_result) +
	// candidate reference, amount plus desugared cash range/begin/end;
	4 * sizeof(void *) + sizeof(int64_t) +
	// nonzero and cash_valid all_of: reference/result, predicate wrapper,
	// find_if_not normal-iterator trip/tag and bool result carriers;
	native_image_fixed_valid_frames + 4 * sizeof(void *) + 3 * sizeof(bool) +
	4 * (3 * sizeof(void *) + sizeof(char) + sizeof(bool)) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + sizeof(int64_t) +
	// Actual copy/copy_n, put and checked-admission helper scopes.
	native_image_fixed_copy_frames + native_image_fixed_put_get_frames +
	native_image_fixed_control_frames +
	// forest_valid input/index/item/path cursor, last slot and flags; its
	// actual set/path objects and dynamic heap remain forest_peak-owned.
	2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(int16_t) + sizeof(bool) +
	sizeof(player_snapshot_codec_result);
constexpr size_t native_image_decode_entry_frames =
	// bytes/output/reserve/context/retained pointer, outer/base/live/offset/
	// retained/decode_working, length/version/status/checksum status;
	5 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(uint32_t) + sizeof(uint16_t) +
	3 * sizeof(player_snapshot_codec_result) +
	// candidate reference and cash range/begin/end/current element;
	5 * sizeof(void *) + sizeof(int64_t) + native_image_fixed_equal_frames +
	native_image_fixed_copy_frames + native_image_fixed_put_get_frames +
	native_image_fixed_control_frames;
player_snapshot_codec_result
native_image_hash_fixed_bounded(const uint8_t *input, size_t length, uint8_t *output,
				bool (*reserve)(size_t, void *) noexcept, void *context,
				size_t outer) noexcept
{
	using result = player_snapshot_codec_result;
	if (!input || !output)
		return result::invalid_value;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG) && defined(__linux__) &&      \
	defined(__x86_64__) && !defined(_WIN32) && defined(OPENSSL_VERSION_MAJOR) &&          \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                       \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                       \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
	constexpr size_t frames = sizeof(SHA256_CTX) + native_image_fixed_sha_frames +
				  5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) +
				  sizeof(result) + native_image_fixed_control_frames;
	if (!reserve || sizeof(SHA_LONG) != 4 || sizeof(unsigned int) != 4 ||
	    sizeof(unsigned long) != 8 || sizeof(void *) != 8 || sizeof(size_t) != 8)
		return result::allocation_failure;
	if (frames > SIZE_MAX - outer)
		return result::limit_exceeded;
	if (!reserve(outer + frames, context))
		return result::allocation_failure;
	SHA256_CTX state;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	const bool hashed = SHA256_Init(&state) == 1 && SHA256_Update(&state, input, length) == 1 &&
			    SHA256_Final(output, &state) == 1;
#pragma GCC diagnostic pop
	return hashed ? result::ok : result::invalid_value;
#else
	(void)length;
	(void)reserve;
	(void)context;
	(void)outer;
	return result::allocation_failure;
#endif
}
player_snapshot_codec_result
native_image_checksum_fixed_bounded(std::span<const uint8_t> bytes,
				    bool (*reserve)(size_t, void *) noexcept, void *context,
				    size_t outer) noexcept
{
	using result = player_snapshot_codec_result;
	if (bytes.size() < SHA256_DIGEST_LENGTH)
		return result::invalid_value;
	constexpr size_t frames =
		sizeof(std::span<const uint8_t>) +
		sizeof(std::array<uint8_t, SHA256_DIGEST_LENGTH>) + 2 * sizeof(size_t) +
		3 * sizeof(void *) + 2 * sizeof(result) + native_image_fixed_memcmp_frames +
		native_image_fixed_copy_frames + native_image_fixed_control_frames;
	size_t base = outer;
	if (!native_image_add(base, frames))
		return result::limit_exceeded;
	if (!native_image_admit(base, 0, reserve, context))
		return result::allocation_failure;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest{};
	const size_t sealed = bytes.size() - digest.size();
	const auto code = native_image_hash_fixed_bounded(bytes.data(), sealed, digest.data(),
							  reserve, context, base);
	if (code != result::ok)
		return code;
	return CRYPTO_memcmp(digest.data(), bytes.data() + sealed, digest.size()) == 0 ?
		       result::ok :
		       result::invalid_value;
}
// The complete paired capture companion supplies this genuine capacity getter.
bool native_capture_item_heap(const std::vector<player_item_snapshot> &, size_t *) noexcept;
}
player_snapshot_codec_result quest_mobile_native_image_encode_bounded(
	const quest_mobile_native_image &image, std::vector<uint8_t> *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	using result = player_snapshot_codec_result;
	if (!output || !nonzero(image.last_transition_operation) || !cash_valid(image) ||
	    (image.state != quest_mobile_lifetime_state::live &&
	     image.state != quest_mobile_lifetime_state::retired) ||
	    (image.state == quest_mobile_lifetime_state::retired && !image.items.empty()))
		return result::invalid_value;
#if !defined(__GLIBCXX__) || !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || \
	!defined(_GLIBCXX_USE_CXX11_ABI) || !_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)reserve;
	(void)context;
	(void)outer_live;
	return result::unsupported_version;
#else
	size_t base = outer_live;
	if (!native_image_add(base, sizeof(native_image_encode_workspace) +
					    native_image_encode_entry_frames) ||
	    !native_image_admit(base, 0, reserve, context))
		return result::limit_exceeded;
	try
	{
		native_image_encode_workspace work;
		auto code = quest_mobile_native_reference_encode_bounded(
			image.reference, &work.reference, reserve, context, base);
		if (code != result::ok)
			return code;
		if (image.items.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
			return result::limit_exceeded;
		if (!native_image_admit(base, native_image_hash_scan_objects, reserve, context))
			return result::limit_exceeded;
		size_t forest = 0;
		if (!native_image_forest_peak(image.items.size(), forest) ||
		    !native_image_admit(base, forest, reserve, context))
			return result::limit_exceeded;
		code = forest_valid(image.items);
		if (code != result::ok)
			return code;
		code = player_item_snapshot_list_encode_bounded(image.items, &work.blob, reserve,
								context, base);
		if (code != result::ok)
			return code;
		const size_t overhead = image.cash ? QUEST_MOBILE_NATIVE_CASH_IMAGE_OVERHEAD :
						     QUEST_MOBILE_NATIVE_IMAGE_OVERHEAD;
		if (work.blob.size() > PLAYER_SNAPSHOT_MAX_BYTES - overhead)
			return result::limit_exceeded;
		size_t live = base;
		if (!native_image_add(live, work.blob.capacity()) ||
		    !native_image_admit(live, overhead + work.blob.size(), reserve, context))
			return result::limit_exceeded;
		work.candidate.assign(overhead + work.blob.size(), 0);
		auto &candidate = work.candidate;
		std::copy(image_magic.begin(), image_magic.end(), candidate.begin());
		size_t offset = image_magic.size();
		put<uint16_t>(candidate.data(), offset,
			      image.cash ? QUEST_MOBILE_NATIVE_CASH_IMAGE_VERSION :
					   QUEST_MOBILE_NATIVE_VERSION);
		put<uint8_t>(candidate.data(), offset, static_cast<uint8_t>(image.state));
		put<uint8_t>(candidate.data(), offset, 0);
		put<uint32_t>(candidate.data(), offset, candidate.size());
		std::copy(work.reference.begin(), work.reference.end(), candidate.begin() + offset);
		offset += work.reference.size();
		std::copy(image.last_transition_operation.bytes.begin(),
			  image.last_transition_operation.bytes.end(), candidate.begin() + offset);
		offset += image.last_transition_operation.bytes.size();
		if (image.cash)
		{
			put<uint64_t>(candidate.data(), offset, image.cash->revision);
			for (int64_t amount : image.cash->denominations.amount)
				put<int64_t>(candidate.data(), offset, amount);
		}
		put<uint32_t>(candidate.data(), offset, work.blob.size());
		std::copy(work.blob.begin(), work.blob.end(), candidate.begin() + offset);
		offset += work.blob.size();
		if (offset != candidate.size() - SHA256_DIGEST_LENGTH)
			return result::invalid_value;
		size_t hash_live = live;
		if (!native_image_add(hash_live, candidate.capacity()))
			return result::limit_exceeded;
		code = native_image_hash_fixed_bounded(candidate.data(), offset,
						       candidate.data() + offset, reserve, context,
						       hash_live);
		if (code != result::ok)
			return code;
		*output = std::move(candidate);
		return result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return result::allocation_failure;
	}
#endif
}

player_snapshot_codec_result quest_mobile_native_image_decode_bounded(
	const std::span<const uint8_t> &bytes, quest_mobile_native_image *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live,
	size_t *retained_image_heap_bytes) noexcept
{
	using result = player_snapshot_codec_result;
	if (!output)
		return result::invalid_value;
	if (bytes.size() < QUEST_MOBILE_NATIVE_IMAGE_OVERHEAD)
		return result::truncated;
	if (bytes.size() > PLAYER_SNAPSHOT_MAX_BYTES)
		return result::limit_exceeded;
#if !defined(__GLIBCXX__) || !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || \
	!defined(_GLIBCXX_USE_CXX11_ABI) || !_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)reserve;
	(void)context;
	(void)outer_live;
	(void)retained_image_heap_bytes;
	return result::unsupported_version;
#else
	if (!std::equal(image_magic.begin(), image_magic.end(), bytes.begin()))
		return result::invalid_value;
	size_t entry_live = outer_live;
	if (!native_image_add(entry_live, native_image_decode_entry_frames))
		return result::limit_exceeded;
	if (!native_image_admit(entry_live, 0, reserve, context))
		return result::allocation_failure;
	const auto checksum_code =
		native_image_checksum_fixed_bounded(bytes, reserve, context, entry_live);
	if (checksum_code != result::ok)
		return checksum_code;
	try
	{
		size_t offset = image_magic.size();
		const uint16_t version = get<uint16_t>(bytes.data(), offset);
		if (version != QUEST_MOBILE_NATIVE_VERSION &&
		    version != QUEST_MOBILE_NATIVE_CASH_IMAGE_VERSION)
			return result::unsupported_version;
		if (version == QUEST_MOBILE_NATIVE_CASH_IMAGE_VERSION &&
		    bytes.size() < QUEST_MOBILE_NATIVE_CASH_IMAGE_OVERHEAD)
			return result::truncated;
		size_t base = entry_live;
		if (!native_image_add(base, sizeof(native_image_decode_workspace)) ||
		    !native_image_admit(base, 0, reserve, context))
			return result::limit_exceeded;
		native_image_decode_workspace work;
		auto &candidate = work.candidate;
		candidate.state = static_cast<quest_mobile_lifetime_state>(
			get<uint8_t>(bytes.data(), offset));
		if (get<uint8_t>(bytes.data(), offset) ||
		    get<uint32_t>(bytes.data(), offset) != bytes.size())
			return result::invalid_value;
		if (!native_image_admit(base, sizeof(std::span<const uint8_t>), reserve, context))
			return result::limit_exceeded;
		work.reference = bytes.subspan(offset, QUEST_MOBILE_NATIVE_REFERENCE_BYTES);
		auto code = quest_mobile_native_reference_decode_bounded(
			work.reference, &candidate.reference, reserve, context, base);
		if (code != result::ok)
			return code;
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
			return result::invalid_value;
		if (!native_image_admit(base, player_item_snapshot_list_preflight_object_bytes(),
					reserve, context))
			return result::limit_exceeded;
		code = player_item_snapshot_list_preflight(bytes.data() + offset, length,
							   &work.items);
		if (code != result::ok)
			return code;
		if (!work.items.fresh_decode_storage_policy_supported)
			return result::unsupported_version;
		if (work.items.decoded_payload_bytes < sizeof(std::vector<player_item_snapshot>))
			return result::invalid_value;
		size_t decode_working = work.items.item_codec_decoder_object_bytes;
		if (!native_image_add(decode_working, work.items.decoded_payload_bytes) ||
		    !native_image_add(decode_working, work.items.relationship_scratch_bytes) ||
		    !native_image_admit(base, decode_working, reserve, context))
			return result::limit_exceeded;
		code = player_item_snapshot_list_decode(bytes.data() + offset, length,
							&candidate.items);
		if (code != result::ok)
			return code;
		size_t retained = 0;
		if (!native_capture_item_heap(candidate.items, &retained))
			return result::limit_exceeded;
		size_t live = base;
		if (!native_image_add(live, retained))
			return result::limit_exceeded;
		code = quest_mobile_native_image_encode_bounded(candidate, &work.canonical, reserve,
								context, live);
		if (code != result::ok)
			return code;
		if (work.canonical.size() != bytes.size() ||
		    !std::equal(work.canonical.begin(), work.canonical.end(), bytes.begin()))
			return result::invalid_value;
		static_assert(std::is_nothrow_move_assignable_v<quest_mobile_native_image>);
		*output = std::move(candidate);
		if (retained_image_heap_bytes)
			*retained_image_heap_bytes = retained;
		return result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return result::allocation_failure;
	}
#endif
}

namespace
{
using native_capture_reserve = bool (*)(size_t, void *) noexcept;

// Authentic complete allocation-free original validator and its captured
// libstdc++13/source-event scalar call inventory. Resource/profile failures
// have a typed result before selecting the exact original semantic validator.
constexpr size_t native_capture_reference_valid_frames =
	// nonzero(id), original any_of->none_of->find_if->two __find_if calls:
	// empty closure params/returned wrapper and real RA trip count/tag.
	sizeof(void *) + sizeof(bool) + 3 * (2 * sizeof(void *) + sizeof(char) + sizeof(bool)) +
	2 * (3 * sizeof(void *) + sizeof(char)) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) +
	// __pred_iter/_Iter_pred source ctor/operator and lambda(this,byte)/result,
	// real move refs; array pointer begin/end needs no heap.
	6 * sizeof(void *) + 4 * sizeof(char) + 3 * sizeof(bool) + sizeof(uint8_t) +
	4 * (2 * sizeof(void *)) +
	// economic_source_event_valid and actual critical_operation_id_is_zero:
	// references/results, range begin/end/byte and fixed-array query scopes.
	2 * (sizeof(void *) + sizeof(bool)) + 2 * sizeof(void *) + sizeof(uint8_t) +
	4 * (2 * sizeof(void *));
player_snapshot_capture_result
native_capture_reference_validate(const quest_mobile_native_reference &value,
				  native_capture_reserve reserve, void *context,
				  size_t outer) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG) && defined(__linux__) &&      \
	defined(__x86_64__) && !defined(_WIN32)
	constexpr size_t frames = native_capture_reference_valid_frames + 3 * sizeof(void *) +
				  sizeof(size_t) + sizeof(player_snapshot_capture_result) +
				  sizeof(bool);
	if (sizeof(void *) != 8 || sizeof(size_t) != 8 || !reserve || frames > SIZE_MAX - outer ||
	    !reserve(outer + frames, context))
		return player_snapshot_capture_result::limit_exceeded;
	return quest_mobile_native_reference_valid(value) ?
		       player_snapshot_capture_result::ok :
		       player_snapshot_capture_result::invalid_identity;
#else
	(void)value;
	(void)reserve;
	(void)context;
	(void)outer;
	return player_snapshot_capture_result::limit_exceeded;
#endif
}

struct native_capture_audit
{
	// Fixed original limits bound physical alias/UID detection before any tree
	// allocation. The bounded sibling preserves the original complete audit;
	// its storage admission precedes construction of this actual workspace.
	std::array<const obj_data *, PLAYER_SNAPSHOT_MAX_OBJECTS> objects{};
	std::array<uint64_t, PLAYER_SNAPSHOT_MAX_OBJECTS> uids{};
	size_t count = 0;
};
constexpr size_t native_capture_audit_frames =
	(PLAYER_SNAPSHOT_MAX_DEPTH + 1) * (4 * sizeof(void *) + 2 * sizeof(size_t) +
					   sizeof(player_snapshot_capture_result) + sizeof(bool));

player_snapshot_capture_result native_capture_audit_tree(const obj_data *object,
							 const obj_data *parent,
							 native_capture_audit &seen,
							 size_t depth) noexcept
{
	if (depth > PLAYER_SNAPSHOT_MAX_DEPTH || seen.count >= PLAYER_SNAPSHOT_MAX_OBJECTS)
		return player_snapshot_capture_result::limit_exceeded;
	for (size_t i = 0; i < seen.count; ++i)
		if (seen.objects[i] == object)
			return player_snapshot_capture_result::object_cycle;
	if (!object->obj_uid || object->obj_uid == UINT64_MAX ||
	    (parent && (object->loc_p != LOC_INSIDE || object->loc.inside != parent)))
		return player_snapshot_capture_result::malformed_source;
	for (size_t i = 0; i < seen.count; ++i)
		if (seen.uids[i] == object->obj_uid)
			return player_snapshot_capture_result::malformed_source;
	seen.objects[seen.count] = object;
	seen.uids[seen.count++] = object->obj_uid;
	for (const obj_data *child = object->contains; child; child = child->next_content)
	{
		const auto result = native_capture_audit_tree(child, object, seen, depth + 1);
		if (result != player_snapshot_capture_result::ok)
			return result;
	}
	return player_snapshot_capture_result::ok;
}

bool native_capture_item_heap(const std::vector<player_item_snapshot> &items,
			      size_t *output) noexcept
{
	if (!output || items.capacity() > SIZE_MAX / sizeof(player_item_snapshot))
		return false;
	size_t bytes = items.capacity() * sizeof(player_item_snapshot);
	const auto string_heap = [&](const std::string &value) noexcept
	{
		return value.capacity() <= 15 || (value.capacity() != SIZE_MAX &&
						  native_image_add(bytes, value.capacity() + 1));
	};
	for (const auto &item : items)
	{
		if (!string_heap(item.name) || !string_heap(item.short_description) ||
		    !string_heap(item.description) || !string_heap(item.action_description) ||
		    item.dynamic_affects.capacity() >
			    SIZE_MAX / sizeof(player_item_dynamic_affect_snapshot) ||
		    item.extra_descriptions.capacity() >
			    SIZE_MAX / sizeof(player_item_extra_description_snapshot) ||
		    !native_image_add(bytes, item.dynamic_affects.capacity() *
						     sizeof(player_item_dynamic_affect_snapshot)) ||
		    !native_image_add(bytes,
				      item.extra_descriptions.capacity() *
					      sizeof(player_item_extra_description_snapshot)))
			return false;
		for (const auto &extra : item.extra_descriptions)
			if (!string_heap(extra.keyword) || !string_heap(extra.description) ||
			    extra.spell_ids.capacity() > SIZE_MAX / sizeof(int32_t) ||
			    !native_image_add(bytes, extra.spell_ids.capacity() * sizeof(int32_t)))
				return false;
	}
	*output = bytes;
	return true;
}

player_snapshot_capture_result native_capture_map_codec(player_snapshot_codec_result code) noexcept
{
	if (code == player_snapshot_codec_result::allocation_failure)
		return player_snapshot_capture_result::retryable_allocation_failure;
	if (code == player_snapshot_codec_result::limit_exceeded)
		return player_snapshot_capture_result::limit_exceeded;
	return code == player_snapshot_codec_result::ok ?
		       player_snapshot_capture_result::ok :
		       player_snapshot_capture_result::malformed_source;
}

player_snapshot_capture_result native_capture_append_tree_bounded(
	const obj_data *root, int16_t slot, std::vector<player_item_snapshot> &items, size_t &bytes,
	native_capture_reserve reserve, void *context, size_t outer) noexcept
{
	constexpr size_t frames =
		10 * sizeof(void *) + 8 * sizeof(size_t) + sizeof(int16_t) +
		sizeof(std::vector<player_item_snapshot>) + sizeof(player_snapshot_capture_result) +
		2 * sizeof(std::vector<player_item_snapshot>::iterator) + sizeof(int32_t);
	size_t heap = 0, base = outer;
	if (!native_capture_item_heap(items, &heap) || !native_image_add(base, frames) ||
	    !native_image_add(base, heap) || !native_image_admit(base, 0, reserve, context))
		return player_snapshot_capture_result::limit_exceeded;
	try
	{
		std::vector<player_item_snapshot> tree;
		size_t estimate = 0, retained = 0;
		const auto result = player_item_snapshot_tree_capture_literal_bounded(
			const_cast<obj_data *>(root), &tree, &estimate, reserve, context, base,
			&retained);
		if (result != player_snapshot_capture_result::ok)
			return result;
		if (tree.empty() || tree.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - items.size() ||
		    estimate < sizeof(player_snapshot) || bytes > PLAYER_SNAPSHOT_MAX_BYTES ||
		    estimate - sizeof(player_snapshot) > PLAYER_SNAPSHOT_MAX_BYTES - bytes ||
		    tree.size() > items.capacity() - items.size())
			return player_snapshot_capture_result::limit_exceeded;
		bytes += estimate - sizeof(player_snapshot);
		const int32_t offset = static_cast<int32_t>(items.size());
		tree[0].equipment_slot = slot;
		// Candidate row capacity was admitted for the exact audited count before
		// any capture. Moves preserve existing literal allocations and order.
		for (auto &row : tree)
		{
			if (row.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
				row.parent_index += offset;
			items.push_back(std::move(row));
		}
		return player_snapshot_capture_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_capture_result::retryable_allocation_failure;
	}
}

player_snapshot_capture_result
native_items_capture_bounded(P_char mob, const quest_mobile_native_reference &reference,
			     std::vector<player_item_snapshot> *output,
			     native_capture_reserve reserve, void *context, size_t outer,
			     size_t *retained_output) noexcept
{
#if !defined(__GLIBCXX__) || !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || \
	!defined(_GLIBCXX_USE_CXX11_ABI) || !_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)mob;
	(void)reference;
	(void)output;
	(void)reserve;
	(void)context;
	(void)outer;
	(void)retained_output;
	return player_snapshot_capture_result::limit_exceeded;
#else
	constexpr size_t frames =
		sizeof(native_capture_audit) + native_capture_audit_frames +
		2 * sizeof(std::vector<uint8_t>) + sizeof(std::vector<player_item_snapshot>) +
		12 * sizeof(void *) + 8 * sizeof(size_t) + 2 * sizeof(int) +
		3 * sizeof(player_snapshot_capture_result) + sizeof(player_snapshot_codec_result);
	size_t base = outer;
	if (!native_image_add(base, frames) || !native_image_admit(base, 0, reserve, context))
		return player_snapshot_capture_result::limit_exceeded;
	const auto validated_reference =
		native_capture_reference_validate(reference, reserve, context, base);
	if (validated_reference != player_snapshot_capture_result::ok)
		return validated_reference;
	if (!mob || !output || !IS_NPC(mob) || !mob->only.npc ||

	    !mob_index || GET_RNUM(mob) < 0 || GET_RNUM(mob) > top_of_mobt ||
	    mob_index[GET_RNUM(mob)].virtual_number != reference.mobile_vnum ||
	    GET_BIRTHPLACE(mob) != reference.birthplace_vnum)
		return player_snapshot_capture_result::invalid_identity;
	try
	{
		native_capture_audit seen;
		for (int slot = 0; slot < MAX_WEAR; ++slot)
		{
			const obj_data *root = mob->equipment[slot];
			if (!root)
				continue;
			if (root->loc_p != LOC_WORN || root->loc.wearing != mob ||
			    root->next_content)
				return player_snapshot_capture_result::malformed_source;
			const auto result = native_capture_audit_tree(root, nullptr, seen, 1);
			if (result != player_snapshot_capture_result::ok)
				return result;
		}
		for (const obj_data *root = mob->carrying; root; root = root->next_content)
		{
			if (root->loc_p != LOC_CARRIED || root->loc.carrying != mob)
				return player_snapshot_capture_result::malformed_source;
			const auto result = native_capture_audit_tree(root, nullptr, seen, 1);
			if (result != player_snapshot_capture_result::ok)
				return result;
		}
		std::vector<player_item_snapshot> candidate;
		if (seen.count > SIZE_MAX / sizeof(player_item_snapshot) ||
		    !native_image_admit(base, seen.count * sizeof(player_item_snapshot), reserve,
					context))
			return player_snapshot_capture_result::limit_exceeded;
		candidate.reserve(seen.count);
		size_t bytes = sizeof(player_snapshot);
		for (int slot = 0; slot < MAX_WEAR; ++slot)
		{
			if (!mob->equipment[slot])
				continue;
			const auto result = native_capture_append_tree_bounded(
				mob->equipment[slot], static_cast<int16_t>(slot + 1), candidate,
				bytes, reserve, context, base);
			if (result != player_snapshot_capture_result::ok)
				return result;
		}
		for (const obj_data *root = mob->carrying; root; root = root->next_content)
		{
			const auto result = native_capture_append_tree_bounded(
				root, 0, candidate, bytes, reserve, context, base);
			if (result != player_snapshot_capture_result::ok)
				return result;
		}
		if (candidate.size() != seen.count)
			return player_snapshot_capture_result::malformed_source;
		size_t heap = 0, live = base;
		if (!native_capture_item_heap(candidate, &heap) || !native_image_add(live, heap))
			return player_snapshot_capture_result::limit_exceeded;
		std::vector<uint8_t> canonical;
		// The existing bounded image encoder applies the full original native
		// forest validation. Its reference/cash policy is checked by the public
		// capture; here retain the original complete generic item codec check.
		const auto code = player_item_snapshot_list_encode_bounded(candidate, &canonical,
									   reserve, context, live);
		const auto result = native_capture_map_codec(code);
		if (result != player_snapshot_capture_result::ok)
			return result;
		*output = std::move(candidate);
		if (retained_output)
			*retained_output = heap;
		return player_snapshot_capture_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_capture_result::retryable_allocation_failure;
	}
#endif
}
}

player_snapshot_capture_result quest_mobile_native_capture_bounded(
	P_char mob, const quest_mobile_native_reference &reference,
	quest_mobile_lifetime_state state, const critical_operation_id &last_transition,
	quest_mobile_native_image *output, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer, size_t *retained_image_heap) noexcept
{
	constexpr size_t frames =
		sizeof(quest_mobile_native_image) + sizeof(std::vector<uint8_t>) +
		8 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(quest_mobile_lifetime_state) +
		2 * sizeof(player_snapshot_capture_result) + sizeof(player_snapshot_codec_result);
	size_t base = outer;
	if (!native_image_add(base, frames) || !native_image_admit(base, 0, reserve, context))
		return player_snapshot_capture_result::limit_exceeded;
	const auto validated_reference =
		native_capture_reference_validate(reference, reserve, context, base);
	if (validated_reference != player_snapshot_capture_result::ok)
		return validated_reference;
	if (state != quest_mobile_lifetime_state::live || !mob || !output || !IS_NPC(mob) ||
	    !mob->only.npc || !nonzero(last_transition) || !mob_index || GET_RNUM(mob) < 0 ||
	    GET_RNUM(mob) > top_of_mobt ||
	    mob_index[GET_RNUM(mob)].virtual_number != reference.mobile_vnum ||
	    GET_BIRTHPLACE(mob) != reference.birthplace_vnum)
		return player_snapshot_capture_result::invalid_identity;
	try
	{
		quest_mobile_native_image candidate;
		candidate.reference = reference;
		candidate.state = quest_mobile_lifetime_state::live;
		candidate.last_transition_operation = last_transition;
		size_t heap = 0;
		const auto captured = native_items_capture_bounded(mob, reference, &candidate.items,
								   reserve, context, base, &heap);
		if (captured != player_snapshot_capture_result::ok)
			return captured;
		size_t live = base;
		if (!native_image_add(live, heap))
			return player_snapshot_capture_result::limit_exceeded;
		std::vector<uint8_t> canonical;
		const auto result = native_capture_map_codec(
			quest_mobile_native_image_encode_bounded(candidate, &canonical, reserve,
								 context, live));
		if (result != player_snapshot_capture_result::ok)
			return result;
		*output = std::move(candidate);
		if (retained_image_heap)
			*retained_image_heap = heap;
		return player_snapshot_capture_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_capture_result::retryable_allocation_failure;
	}
}

player_snapshot_capture_result
quest_mobile_native_capture_bounded(P_char mob, const quest_mobile_native_reference &reference,
				    quest_mobile_lifetime_state state,
				    const critical_operation_id &last_transition,
				    uint64_t cash_revision, quest_mobile_native_image *output,
				    bool (*reserve)(size_t, void *) noexcept, void *context,
				    size_t outer, size_t *retained_image_heap) noexcept
{
	if (!output || !cash_revision)
		return player_snapshot_capture_result::invalid_identity;
	constexpr size_t frames = sizeof(quest_mobile_native_image) + sizeof(std::vector<uint8_t>) +
				  8 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(uint64_t) +
				  sizeof(quest_mobile_lifetime_state) +
				  2 * sizeof(player_snapshot_capture_result);
	size_t base = outer;
	if (!native_image_add(base, frames) || !native_image_admit(base, 0, reserve, context))
		return player_snapshot_capture_result::limit_exceeded;
	quest_mobile_native_image candidate;
	size_t heap = 0;
	const auto captured = quest_mobile_native_capture_bounded(
		mob, reference, state, last_transition, &candidate, reserve, context, base, &heap);
	if (captured != player_snapshot_capture_result::ok)
		return captured;
	try
	{
		candidate.cash.emplace();
		candidate.cash->revision = cash_revision;
		candidate.cash->denominations.amount = { GET_COPPER(mob), GET_SILVER(mob),
							 GET_GOLD(mob), GET_PLATINUM(mob) };
		size_t live = base;
		if (!native_image_add(live, heap))
			return player_snapshot_capture_result::limit_exceeded;
		std::vector<uint8_t> canonical;
		const auto result = native_capture_map_codec(
			quest_mobile_native_image_encode_bounded(candidate, &canonical, reserve,
								 context, live));
		if (result != player_snapshot_capture_result::ok)
			return result;
		*output = std::move(candidate);
		if (retained_image_heap)
			*retained_image_heap = heap;
		return player_snapshot_capture_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_capture_result::retryable_allocation_failure;
	}
}
