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
