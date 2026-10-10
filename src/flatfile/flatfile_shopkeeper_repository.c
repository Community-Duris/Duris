#include "flatfile/flatfile_shopkeeper_repository.h"

#include "flatfile/flatfile_authority_transaction.h"
#include "flatfile/flatfile_store.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <limits>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <set>
#include <type_traits>
#include <unordered_set>

namespace
{
constexpr std::array<uint8_t, 8> catalog_magic = { 'D', 'U', 'R', 'S', 'H', 'O', 'P', 0 };
constexpr uint32_t catalog_version = 2;
constexpr size_t catalog_maximum_bytes = 256 * 1024 * 1024;
constexpr size_t shopkeeper_maximum = 262144;
constexpr size_t affect_maximum = 4096;
constexpr int16_t equipment_slot_maximum = 255;
constexpr const char *catalog_filename = "shopkeeper_catalog";

// Exact existing v2 serialized framing/field widths, never sizeof(record/padding).
constexpr size_t initial_checkpoint_header_bytes = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
constexpr size_t initial_checkpoint_record_bytes = 4 + 4 + 4 + 8 + 8 + 8 + 1 + 4;
constexpr size_t initial_checkpoint_affect_bytes = 4 + 4 + 4 + 4 + 5 * 8;
constexpr size_t initial_checkpoint_overhead_bytes =
	initial_checkpoint_header_bytes + 4 + initial_checkpoint_record_bytes + 4;
constexpr size_t initial_checkpoint_maximum_bytes =
	initial_checkpoint_overhead_bytes + affect_maximum * initial_checkpoint_affect_bytes +
	PLAYER_SNAPSHOT_MAX_BYTES;
static_assert(initial_checkpoint_maximum_bytes <= catalog_maximum_bytes);

struct shopkeeper_catalog
{
	uint64_t revision = 1;
	std::vector<flatfile_shopkeeper_record> records;
};

struct encoder
{
	std::vector<uint8_t> bytes;
	bool valid = true;

	template <typename T> void number(T value)
	{
		if (!valid)
			return;
		using U = std::make_unsigned_t<T>;
		U bits = static_cast<U>(value);
		try
		{
			for (size_t index = 0; index < sizeof(T); ++index)
			{
				bytes.push_back(static_cast<uint8_t>(bits & 0xff));
				bits >>= 8;
			}
		}
		catch (const std::bad_alloc &)
		{
			valid = false;
		}
	}

	void raw(const uint8_t *data, size_t size)
	{
		if (!valid || (!data && size) || bytes.size() > catalog_maximum_bytes ||
		    size > catalog_maximum_bytes - bytes.size())
		{
			valid = false;
			return;
		}
		try
		{
			bytes.insert(bytes.end(), data, data + size);
		}
		catch (const std::bad_alloc &)
		{
			valid = false;
		}
	}
};

struct decoder
{
	const uint8_t *data;
	size_t size;
	size_t offset = 0;

	template <typename T> bool number(T *value)
	{
		if (!value || offset > size || size - offset < sizeof(T))
			return false;
		using U = std::make_unsigned_t<T>;
		U bits = 0;
		for (size_t index = 0; index < sizeof(T); ++index)
			bits |= static_cast<U>(data[offset++]) << (index * 8);
		*value = static_cast<T>(bits);
		return true;
	}

	bool byte_vector(std::vector<uint8_t> *value, size_t maximum)
	{
		uint32_t length = 0;
		if (!value || !number(&length) || !length || length > maximum || offset > size ||
		    size - offset < length)
			return false;
		try
		{
			value->assign(data + offset, data + offset + length);
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
		offset += length;
		return true;
	}
};

std::string domains_directory(const std::string &root)
{
	return root + "/domains";
}

bool record_less(const flatfile_shopkeeper_record &left, const flatfile_shopkeeper_record &right)
{
	return left.shop_id < right.shop_id;
}

bool affect_less(const flatfile_shopkeeper_affect_record &left,
		 const flatfile_shopkeeper_affect_record &right)
{
	if (left.type != right.type)
		return left.type < right.type;
	if (left.location != right.location)
		return left.location < right.location;
	if (left.modifier != right.modifier)
		return left.modifier < right.modifier;
	if (left.duration != right.duration)
		return left.duration < right.duration;
	return left.bitvectors < right.bitvectors;
}

bool valid_items(const std::vector<player_item_snapshot> &items,
		 std::unordered_set<uint64_t> *item_uids)
{
	std::vector<uint8_t> encoded;
	if (!item_uids ||
	    player_item_snapshot_list_encode(items, &encoded) != player_snapshot_codec_result::ok)
		return false;
	std::set<int16_t> equipment_slots;
	for (const auto &item : items)
	{
		if (!item.object_uid || item.vnum <= 0 || item.equipment_slot < 0 ||
		    item.equipment_slot > equipment_slot_maximum ||
		    (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT && item.equipment_slot != 0) ||
		    (item.equipment_slot > 0 &&
		     !equipment_slots.insert(item.equipment_slot).second) ||
		    !item_uids->insert(item.object_uid).second)
			return false;
	}
	return true;
}

bool valid_catalog(const shopkeeper_catalog &catalog)
{
	if (!catalog.revision || catalog.records.size() > shopkeeper_maximum ||
	    !std::is_sorted(catalog.records.begin(), catalog.records.end(), record_less))
		return false;
	std::unordered_set<uint64_t> item_uids;
	try
	{
		for (size_t index = 0; index < catalog.records.size(); ++index)
		{
			const auto &record = catalog.records[index];
			if (record.mob_vnum <= 0 || record.room_vnum <= 0 || record.saved_at < 0 ||
			    record.cash < -1 || record.cash > std::numeric_limits<int>::max() ||
			    !record.revision || record.affects.size() > affect_maximum ||
			    (index && !record_less(catalog.records[index - 1], record)) ||
			    !std::is_sorted(record.affects.begin(), record.affects.end(),
					    affect_less) ||
			    !valid_items(record.items, &item_uids))
				return false;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

bool encode_items(encoder &out, const std::vector<player_item_snapshot> &items)
{
	std::vector<uint8_t> encoded;
	if (player_item_snapshot_list_encode(items, &encoded) != player_snapshot_codec_result::ok ||
	    encoded.size() > UINT32_MAX)
		return false;
	out.number<uint32_t>(encoded.size());
	out.raw(encoded.data(), encoded.size());
	return out.valid;
}

bool decode_items(decoder &in, std::vector<player_item_snapshot> *items)
{
	std::vector<uint8_t> encoded;
	return in.byte_vector(&encoded, PLAYER_SNAPSHOT_MAX_BYTES) &&
	       player_item_snapshot_list_decode(encoded.data(), encoded.size(), items) ==
		       player_snapshot_codec_result::ok;
}

bool encode_catalog(const shopkeeper_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !valid_catalog(catalog))
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.records.size());
	for (const auto &record : catalog.records)
	{
		payload.number(record.shop_id);
		payload.number(record.mob_vnum);
		payload.number(record.room_vnum);
		payload.number(record.saved_at);
		payload.number(record.revision);
		payload.number(record.cash);
		payload.number<uint8_t>(record.roaming ? 1 : 0);
		payload.number<uint32_t>(record.affects.size());
		for (const auto &affect : record.affects)
		{
			payload.number(affect.type);
			payload.number(affect.duration);
			payload.number(affect.modifier);
			payload.number(affect.location);
			for (uint64_t bitvector : affect.bitvectors)
				payload.number(bitvector);
		}
		if (!encode_items(payload, record.items))
			return false;
	}
	if (!payload.valid || payload.bytes.size() > catalog_maximum_bytes)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload.bytes.data(), payload.bytes.size(), digest.data());
	encoder file;
	file.raw(catalog_magic.data(), catalog_magic.size());
	file.number(catalog_version);
	file.number<uint32_t>(payload.bytes.size());
	file.number(catalog.revision);
	file.raw(digest.data(), digest.size());
	file.raw(payload.bytes.data(), payload.bytes.size());
	if (!file.valid || file.bytes.size() > catalog_maximum_bytes)
		return false;
	*bytes = std::move(file.bytes);
	return true;
}

bool decode_catalog(const std::vector<uint8_t> &bytes, shopkeeper_catalog *catalog)
{
	constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	if (!catalog || bytes.size() < header_size ||
	    memcmp(bytes.data(), catalog_magic.data(), catalog_magic.size()))
		return false;
	decoder header{ bytes.data() + 8, bytes.size() - 8 };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&revision) || (version != 1 && version != catalog_version) ||
	    !revision || payload_size != bytes.size() - header_size)
		return false;
	const uint8_t *payload_bytes = bytes.data() + header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload_bytes, payload_size, digest.data());
	if (CRYPTO_memcmp(bytes.data() + 24, digest.data(), digest.size()))
		return false;
	decoder payload{ payload_bytes, payload_size };
	uint32_t count = 0;
	if (!payload.number(&count) || count > shopkeeper_maximum)
		return false;
	shopkeeper_catalog decoded;
	decoded.revision = revision;
	try
	{
		decoded.records.resize(count);
		for (auto &record : decoded.records)
		{
			uint32_t affect_count = 0;
			uint8_t roaming = 0;
			if (!payload.number(&record.shop_id) || !payload.number(&record.mob_vnum) ||
			    !payload.number(&record.room_vnum) ||
			    !payload.number(&record.saved_at) ||
			    !payload.number(&record.revision) ||
			    (version == catalog_version && !payload.number(&record.cash)) ||
			    (version == catalog_version && !payload.number(&roaming)) ||
			    roaming > 1 || !payload.number(&affect_count) ||
			    affect_count > affect_maximum)
				return false;
			if (version == 1)
				record.cash = -1;
			record.roaming = roaming != 0;
			record.affects.resize(affect_count);
			for (auto &affect : record.affects)
			{
				if (!payload.number(&affect.type) ||
				    !payload.number(&affect.duration) ||
				    !payload.number(&affect.modifier) ||
				    !payload.number(&affect.location))
					return false;
				for (uint64_t &bitvector : affect.bitvectors)
					if (!payload.number(&bitvector))
						return false;
			}
			if (!decode_items(payload, &record.items))
				return false;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (payload.offset != payload.size || !valid_catalog(decoded))
		return false;
	*catalog = std::move(decoded);
	return true;
}

// Preflight before decode_catalog can resize records/affects/item byte vectors.
// The original decoder still verifies checksum, all values and forest framing.
bool initial_checkpoint_framing(const std::vector<uint8_t> &bytes) noexcept
{
	if (bytes.size() < initial_checkpoint_overhead_bytes ||
	    bytes.size() > initial_checkpoint_maximum_bytes ||
	    memcmp(bytes.data(), catalog_magic.data(), catalog_magic.size()))
		return false;
	decoder header{ bytes.data() + catalog_magic.size(), bytes.size() - catalog_magic.size() };
	uint32_t version = 0, payload_size = 0;
	uint64_t framing_revision = 0;
	if (!header.number(&version) || version != catalog_version ||
	    !header.number(&payload_size) ||
	    payload_size != bytes.size() - initial_checkpoint_header_bytes ||
	    !header.number(&framing_revision) || framing_revision != 1)
		return false;
	decoder payload{ bytes.data() + initial_checkpoint_header_bytes, payload_size };
	uint32_t count = 0, shop = 0, affects = 0;
	int32_t mobile = 0, room = 0;
	int64_t saved_at = 0, cash = 0;
	uint64_t record_revision = 0;
	uint8_t roaming = 0;
	if (!payload.number(&count) || count != 1 || !payload.number(&shop) ||
	    !payload.number(&mobile) || mobile <= 0 || !payload.number(&room) || room <= 0 ||
	    !payload.number(&saved_at) || saved_at < 0 || !payload.number(&record_revision) ||
	    record_revision != 1 || !payload.number(&cash) || cash < 0 ||
	    cash > std::numeric_limits<int>::max() || !payload.number(&roaming) || roaming > 1 ||
	    !payload.number(&affects) || affects > affect_maximum)
		return false;
	const size_t affect_bytes = static_cast<size_t>(affects) * initial_checkpoint_affect_bytes;
	if (payload.offset > payload.size || affect_bytes > payload.size - payload.offset)
		return false;
	payload.offset += affect_bytes;
	uint32_t item_bytes = 0;
	return payload.number(&item_bytes) && item_bytes &&
	       item_bytes <= PLAYER_SNAPSHOT_MAX_BYTES && payload.offset <= payload.size &&
	       item_bytes == payload.size - payload.offset;
}

flatfile_shopkeeper_result recover(const std::string &root, const flatfile_authority_lock &lock,
				   std::string *error)
{
	const auto result = flatfile_authority_transaction_recover(root, lock, error);
	if (result == flatfile_authority_transaction_result::ok)
		return flatfile_shopkeeper_result::ok;
	return result == flatfile_authority_transaction_result::io_error ?
		       flatfile_shopkeeper_result::io_error :
		       flatfile_shopkeeper_result::invalid;
}

flatfile_shopkeeper_result load_catalog(const std::string &root, shopkeeper_catalog *catalog,
					std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(domains_directory(root), catalog_filename,
					  catalog_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_shopkeeper_result::not_found;
	if (loaded == flatfile_read_result::io_error)
		return flatfile_shopkeeper_result::io_error;
	if (loaded != flatfile_read_result::ok || !decode_catalog(bytes, catalog))
	{
		if (error && error->empty())
			*error = "shopkeeper catalog is corrupt";
		return flatfile_shopkeeper_result::invalid;
	}
	return flatfile_shopkeeper_result::ok;
}

bool catalog_equal(shopkeeper_catalog left, shopkeeper_catalog right)
{
	left.revision = 1;
	right.revision = 1;
	std::vector<uint8_t> left_bytes, right_bytes;
	return encode_catalog(left, &left_bytes) && encode_catalog(right, &right_bytes) &&
	       left_bytes == right_bytes;
}

bool trade_items_match_payload(const shop_trade_payload &payload,
			       const std::vector<player_item_snapshot> &items)
{
	if (items.size() != payload.item_count)
		return false;
	size_t root_count = 0;
	for (size_t index = 0; index < items.size(); ++index)
	{
		const auto &item = items[index];
		if (!item.object_uid || item.vnum <= 0 || item.equipment_slot != 0)
			return false;
		uint64_t parent_uid = 0;
		if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
		{
			++root_count;
			if (item.object_uid != payload.selected_item_uid)
				return false;
		}
		else if (item.parent_index < 0 || item.parent_index >= static_cast<int32_t>(index))
			return false;
		else
			parent_uid = items[item.parent_index].object_uid;
		auto expected = std::lower_bound(
			payload.items.begin(), payload.items.begin() + payload.item_count,
			item.object_uid, [](const shop_trade_item_entry &candidate, uint64_t uid)
			{ return candidate.item_uid < uid; });
		if (expected == payload.items.begin() + payload.item_count ||
		    expected->item_uid != item.object_uid ||
		    expected->root_item_uid != payload.selected_item_uid ||
		    expected->parent_item_uid != parent_uid || expected->vnum != item.vnum)
			return false;
	}
	return root_count == 1;
}

bool select_subtree(const std::vector<player_item_snapshot> &items, uint64_t root_uid,
		    std::vector<player_item_snapshot> *selected, std::vector<bool> *selected_rows)
{
	if (!selected || !selected_rows)
		return false;
	auto root = std::find_if(items.begin(), items.end(),
				 [&](const auto &item) { return item.object_uid == root_uid; });
	if (root == items.end() || root->parent_index != PLAYER_SNAPSHOT_NO_PARENT)
		return false;
	try
	{
		selected->clear();
		selected_rows->assign(items.size(), false);
		std::vector<int32_t> remap(items.size(), PLAYER_SNAPSHOT_NO_PARENT);
		for (size_t index = 0; index < items.size(); ++index)
		{
			const bool include = items[index].object_uid == root_uid ||
					     (items[index].parent_index >= 0 &&
					      (*selected_rows)[items[index].parent_index]);
			if (!include)
				continue;
			player_item_snapshot copy = items[index];
			copy.parent_index = items[index].object_uid == root_uid ?
						    PLAYER_SNAPSHOT_NO_PARENT :
						    remap[items[index].parent_index];
			if (copy.parent_index < PLAYER_SNAPSHOT_NO_PARENT)
				return false;
			remap[index] = static_cast<int32_t>(selected->size());
			(*selected_rows)[index] = true;
			selected->push_back(std::move(copy));
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return !selected->empty();
}

bool remove_rows(std::vector<player_item_snapshot> *items, const std::vector<bool> &removed)
{
	if (!items || removed.size() != items->size())
		return false;
	try
	{
		std::vector<player_item_snapshot> kept;
		std::vector<int32_t> remap(items->size(), PLAYER_SNAPSHOT_NO_PARENT);
		kept.reserve(items->size());
		for (size_t index = 0; index < items->size(); ++index)
		{
			if (removed[index])
				continue;
			player_item_snapshot copy = (*items)[index];
			if (copy.parent_index >= 0)
			{
				if (removed[copy.parent_index] ||
				    remap[copy.parent_index] == PLAYER_SNAPSHOT_NO_PARENT)
					return false;
				copy.parent_index = remap[copy.parent_index];
			}
			remap[index] = static_cast<int32_t>(kept.size());
			kept.push_back(std::move(copy));
		}
		*items = std::move(kept);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

bool append_trade_items(std::vector<player_item_snapshot> *items,
			const std::vector<player_item_snapshot> &added)
{
	if (!items || items->size() > PLAYER_SNAPSHOT_MAX_OBJECTS - added.size())
		return false;
	const int32_t offset = static_cast<int32_t>(items->size());
	try
	{
		items->reserve(items->size() + added.size());
		for (auto item : added)
		{
			if (item.parent_index >= 0)
				item.parent_index += offset;
			items->push_back(std::move(item));
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}
// The bounded path uses one exact-capacity UID array and a fixed equipment
// bitmap. The original encoder remains mandatory for each item forest.
bool valid_catalog_bounded_scratch(const shopkeeper_catalog &catalog, size_t expected_item_count)
{
	if (!catalog.revision || catalog.records.size() > shopkeeper_maximum ||
	    !std::is_sorted(catalog.records.begin(), catalog.records.end(), record_less))
		return false;
	std::vector<uint64_t> item_uids;
	item_uids.reserve(expected_item_count);
	for (size_t index = 0; index < catalog.records.size(); ++index)
	{
		const auto &record = catalog.records[index];
		if (record.mob_vnum <= 0 || record.room_vnum <= 0 || record.saved_at < 0 ||
		    record.cash < -1 || record.cash > std::numeric_limits<int>::max() ||
		    !record.revision || record.affects.size() > affect_maximum ||
		    (index && !record_less(catalog.records[index - 1], record)) ||
		    !std::is_sorted(record.affects.begin(), record.affects.end(), affect_less))
			return false;
		std::vector<uint8_t> encoded;
		if (player_item_snapshot_list_encode(record.items, &encoded) !=
		    player_snapshot_codec_result::ok)
			return false;
		std::array<bool, equipment_slot_maximum + 1> equipment_slots{};
		for (const auto &item : record.items)
		{
			if (!item.object_uid || item.vnum <= 0 || item.equipment_slot < 0 ||
			    item.equipment_slot > equipment_slot_maximum ||
			    (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT &&
			     item.equipment_slot != 0) ||
			    (item.equipment_slot > 0 && equipment_slots[item.equipment_slot]) ||
			    item_uids.size() >= expected_item_count)
				return false;
			if (item.equipment_slot > 0)
				equipment_slots[item.equipment_slot] = true;
			item_uids.push_back(item.object_uid);
		}
	}
	if (item_uids.size() != expected_item_count)
		return false;
	std::sort(item_uids.begin(), item_uids.end());
	return std::adjacent_find(item_uids.begin(), item_uids.end()) == item_uids.end();
}

bool decode_catalog_bounded_scratch(const std::vector<uint8_t> &bytes, shopkeeper_catalog *catalog,
				    size_t expected_item_count)
{
	constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	if (!catalog || bytes.size() < header_size ||
	    memcmp(bytes.data(), catalog_magic.data(), catalog_magic.size()))
		return false;
	decoder header{ bytes.data() + 8, bytes.size() - 8 };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&revision) || (version != 1 && version != catalog_version) ||
	    !revision || payload_size != bytes.size() - header_size)
		return false;
	const uint8_t *payload_bytes = bytes.data() + header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	if (!SHA256(payload_bytes, payload_size, digest.data()))
		return false;
	if (CRYPTO_memcmp(bytes.data() + 24, digest.data(), digest.size()))
		return false;
	decoder payload{ payload_bytes, payload_size };
	uint32_t count = 0;
	if (!payload.number(&count) || count > shopkeeper_maximum)
		return false;
	shopkeeper_catalog decoded;
	decoded.revision = revision;
	try
	{
		decoded.records.resize(count);
		for (auto &record : decoded.records)
		{
			uint32_t affect_count = 0;
			uint8_t roaming = 0;
			if (!payload.number(&record.shop_id) || !payload.number(&record.mob_vnum) ||
			    !payload.number(&record.room_vnum) ||
			    !payload.number(&record.saved_at) ||
			    !payload.number(&record.revision) ||
			    (version == catalog_version && !payload.number(&record.cash)) ||
			    (version == catalog_version && !payload.number(&roaming)) ||
			    roaming > 1 || !payload.number(&affect_count) ||
			    affect_count > affect_maximum)
				return false;
			if (version == 1)
				record.cash = -1;
			record.roaming = roaming != 0;
			record.affects.resize(affect_count);
			for (auto &affect : record.affects)
			{
				if (!payload.number(&affect.type) ||
				    !payload.number(&affect.duration) ||
				    !payload.number(&affect.modifier) ||
				    !payload.number(&affect.location))
					return false;
				for (uint64_t &bitvector : affect.bitvectors)
					if (!payload.number(&bitvector))
						return false;
			}
			uint32_t item_bytes = 0;
			if (!payload.number(&item_bytes) || !item_bytes ||
			    item_bytes > PLAYER_SNAPSHOT_MAX_BYTES ||
			    payload.offset > payload.size ||
			    item_bytes > payload.size - payload.offset ||
			    player_item_snapshot_list_decode(payload.data + payload.offset,
							     item_bytes, &record.items) !=
				    player_snapshot_codec_result::ok)
				return false;
			payload.offset += item_bytes;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (payload.offset != payload.size ||
	    !valid_catalog_bounded_scratch(decoded, expected_item_count))
		return false;
	*catalog = std::move(decoded);
	return true;
}

// Resource origin is retained explicitly through semantic bool predicates.
// Legacy decoders/validators below keep their original behavior for all callers.
enum class shop_resource_status : uint8_t
{
	invalid,
	allocation_failure,
	capacity_exceeded,
	unsupported
};
struct shop_resource_state
{
	shop_resource_status status = shop_resource_status::invalid;
	bool codec(player_snapshot_codec_result value) noexcept
	{
		if (value == player_snapshot_codec_result::ok)
			return true;
		if (value == player_snapshot_codec_result::allocation_failure)
			status = shop_resource_status::allocation_failure;
		else if (value == player_snapshot_codec_result::limit_exceeded)
			status = shop_resource_status::capacity_exceeded;
		// Unknown serialized versions remain semantic corruption.
		return false;
	}
	bool capacity() noexcept
	{
		status = shop_resource_status::capacity_exceeded;
		return false;
	}
	bool policy(bool supported) noexcept
	{
		if (supported)
			return true;
		status = shop_resource_status::unsupported;
		return false;
	}
	flatfile_shopkeeper_result failure() const noexcept
	{
		if (status == shop_resource_status::allocation_failure)
			errno = ENOMEM;
		else if (status == shop_resource_status::capacity_exceeded)
			errno = ENOBUFS;
		else if (status == shop_resource_status::unsupported)
			errno = ENOTSUP;
		else
		{
			errno = EBADMSG;
			return flatfile_shopkeeper_result::invalid;
		}
		return flatfile_shopkeeper_result::io_error;
	}
};
bool shop_valid_catalog_resource(const shopkeeper_catalog &catalog, size_t expected_item_count,
				 shop_resource_state &resource)
{
	if (!catalog.revision || catalog.records.size() > shopkeeper_maximum ||
	    !std::is_sorted(catalog.records.begin(), catalog.records.end(), record_less))
		return false;
	std::vector<uint64_t> item_uids;
	item_uids.reserve(expected_item_count);
	for (size_t index = 0; index < catalog.records.size(); ++index)
	{
		const auto &record = catalog.records[index];
		if (record.mob_vnum <= 0 || record.room_vnum <= 0 || record.saved_at < 0 ||
		    record.cash < -1 || record.cash > std::numeric_limits<int>::max() ||
		    !record.revision || record.affects.size() > affect_maximum ||
		    (index && !record_less(catalog.records[index - 1], record)) ||
		    !std::is_sorted(record.affects.begin(), record.affects.end(), affect_less))
			return false;
		std::vector<uint8_t> encoded;
		if (!resource.codec(player_item_snapshot_list_encode(record.items, &encoded)))
			return false;
		std::array<bool, equipment_slot_maximum + 1> equipment_slots{};
		for (const auto &item : record.items)
		{
			if (!item.object_uid || item.vnum <= 0 || item.equipment_slot < 0 ||
			    item.equipment_slot > equipment_slot_maximum ||
			    (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT &&
			     item.equipment_slot != 0) ||
			    (item.equipment_slot > 0 && equipment_slots[item.equipment_slot]) ||
			    item_uids.size() >= expected_item_count)
				return false;
			if (item.equipment_slot > 0)
				equipment_slots[item.equipment_slot] = true;
			item_uids.push_back(item.object_uid);
		}
	}
	if (item_uids.size() != expected_item_count)
		return false;
	std::sort(item_uids.begin(), item_uids.end());
	return std::adjacent_find(item_uids.begin(), item_uids.end()) == item_uids.end();
}
bool shop_decode_catalog_resource(const std::vector<uint8_t> &bytes, shopkeeper_catalog *catalog,
				  size_t expected_item_count, shop_resource_state &resource)
{
	constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	if (!catalog || bytes.size() < header_size ||
	    memcmp(bytes.data(), catalog_magic.data(), catalog_magic.size()))
		return false;
	decoder header{ bytes.data() + 8, bytes.size() - 8 };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&revision) || (version != 1 && version != catalog_version) ||
	    !revision || payload_size != bytes.size() - header_size)
		return false;
	const uint8_t *payload_bytes = bytes.data() + header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	if (!SHA256(payload_bytes, payload_size, digest.data()))
		return false;
	if (CRYPTO_memcmp(bytes.data() + 24, digest.data(), digest.size()))
		return false;
	decoder payload{ payload_bytes, payload_size };
	uint32_t count = 0;
	if (!payload.number(&count) || count > shopkeeper_maximum)
		return false;
	shopkeeper_catalog decoded;
	decoded.revision = revision;
	try
	{
		decoded.records.resize(count);
		for (auto &record : decoded.records)
		{
			uint32_t affect_count = 0;
			uint8_t roaming = 0;
			if (!payload.number(&record.shop_id) || !payload.number(&record.mob_vnum) ||
			    !payload.number(&record.room_vnum) ||
			    !payload.number(&record.saved_at) ||
			    !payload.number(&record.revision) ||
			    (version == catalog_version && !payload.number(&record.cash)) ||
			    (version == catalog_version && !payload.number(&roaming)) ||
			    roaming > 1 || !payload.number(&affect_count) ||
			    affect_count > affect_maximum)
				return false;
			if (version == 1)
				record.cash = -1;
			record.roaming = roaming != 0;
			record.affects.resize(affect_count);
			for (auto &affect : record.affects)
			{
				if (!payload.number(&affect.type) ||
				    !payload.number(&affect.duration) ||
				    !payload.number(&affect.modifier) ||
				    !payload.number(&affect.location))
					return false;
				for (uint64_t &bitvector : affect.bitvectors)
					if (!payload.number(&bitvector))
						return false;
			}
			uint32_t item_bytes = 0;
			if (!payload.number(&item_bytes) || !item_bytes ||
			    item_bytes > PLAYER_SNAPSHOT_MAX_BYTES ||
			    payload.offset > payload.size ||
			    item_bytes > payload.size - payload.offset ||
			    !resource.codec(player_item_snapshot_list_decode(
				    payload.data + payload.offset, item_bytes, &record.items)))
				return false;
			payload.offset += item_bytes;
		}
	}
	catch (const std::bad_alloc &)
	{
		resource.status = shop_resource_status::allocation_failure;
		return false;
	}
	if (payload.offset != payload.size ||
	    !shop_valid_catalog_resource(decoded, expected_item_count, resource))
		return false;
	*catalog = std::move(decoded);
	return true;
}
bool shop_catalog_preflight_resource(const uint8_t *encoded, size_t encoded_size,
				     flatfile_shopkeeper_catalog_allocation_profile *output,
				     shop_resource_state &resource) noexcept
{
	if (!encoded || !output ||
	    encoded_size < initial_checkpoint_header_bytes + sizeof(uint32_t) ||
	    encoded_size > catalog_maximum_bytes ||
	    memcmp(encoded, catalog_magic.data(), catalog_magic.size()))
		return false;
	decoder header{ encoded + catalog_magic.size(), encoded_size - catalog_magic.size() };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || (version != 1 && version != catalog_version) ||
	    !header.number(&payload_size) ||
	    payload_size != encoded_size - initial_checkpoint_header_bytes ||
	    !header.number(&revision) || !revision)
		return false;
	// Sizing only: actual decode_catalog still authenticates the digest.
	const uint8_t *payload_bytes = encoded + initial_checkpoint_header_bytes;
	decoder payload{ payload_bytes, payload_size };
	uint32_t count = 0;
	if (!payload.number(&count) || count > shopkeeper_maximum)
		return false;
	const auto add = [&resource](size_t &total, size_t amount) noexcept
	{
		if (amount > SIZE_MAX - total)
			return resource.capacity();
		total += amount;
		return true;
	};
	const auto product = [&resource](size_t count_value, size_t width, size_t *result) noexcept
	{
		if (!result || (width && count_value > SIZE_MAX / width))
			return resource.capacity();
		*result = count_value * width;
		return true;
	};
	flatfile_shopkeeper_catalog_allocation_profile profile;
	profile.record_count = count;
	profile.decoded_catalog_payload_bytes = sizeof(shopkeeper_catalog);
	profile.canonical_catalog_bytes = initial_checkpoint_header_bytes + sizeof(uint32_t);
#if defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI
	profile.fresh_decode_storage_policy_supported = true;
#endif
	size_t row_bytes = 0;
	if (!product(count, sizeof(flatfile_shopkeeper_record), &row_bytes) ||
	    !add(profile.decoded_catalog_payload_bytes, row_bytes))
		return false;
	for (uint32_t index = 0; index < count; ++index)
	{
		uint32_t shop = 0, affect_count = 0, item_bytes = 0;
		int32_t mobile = 0, room = 0;
		int64_t saved_at = 0, cash = -1;
		uint64_t record_revision = 0;
		uint8_t roaming = 0;
		if (!payload.number(&shop) || !payload.number(&mobile) || !payload.number(&room) ||
		    !payload.number(&saved_at) || !payload.number(&record_revision) ||
		    (version == catalog_version &&
		     (!payload.number(&cash) || !payload.number(&roaming))) ||
		    roaming > 1 || !payload.number(&affect_count) || affect_count > affect_maximum)
			return false;
		size_t affect_wire_bytes = 0, affect_payload_bytes = 0;
		if (!product(affect_count, initial_checkpoint_affect_bytes, &affect_wire_bytes) ||
		    !product(affect_count, sizeof(flatfile_shopkeeper_affect_record),
			     &affect_payload_bytes) ||
		    payload.offset > payload.size ||
		    affect_wire_bytes > payload.size - payload.offset)
			return false;
		payload.offset += affect_wire_bytes;
		if (!payload.number(&item_bytes) || !item_bytes ||
		    item_bytes > PLAYER_SNAPSHOT_MAX_BYTES || payload.offset > payload.size ||
		    item_bytes > payload.size - payload.offset)
			return false;
		player_item_snapshot_list_allocation_profile items;
		if (!resource.codec(player_item_snapshot_list_preflight(
			    payload.data + payload.offset, item_bytes, &items)) ||
		    items.decoded_payload_bytes < sizeof(std::vector<player_item_snapshot>))
			return false;
		if (!resource.policy(items.canonical_encoder_storage_policy_supported))
			return false;
		size_t validation = items.canonical_encoded_capacity_bytes;
		if (!add(validation, items.decoded_payload_bytes) ||
		    !add(validation, items.relationship_scratch_bytes) ||
		    !items.item_codec_decoder_object_bytes ||
		    !add(validation, items.item_codec_decoder_object_bytes) ||
		    !add(validation, sizeof(std::vector<player_item_snapshot>)))
			return false;
		validation = std::max(validation, items.canonical_encoded_reallocation_peak_bytes);
		if (!add(validation, items.canonical_encoder_object_bytes))
			return false;
		profile.largest_item_roundtrip_scratch_bytes =
			std::max(profile.largest_item_roundtrip_scratch_bytes, validation);
		// Parent record contains its retained vector head; the decoder's
		// separate local item vector, decoder object and depth vector coexist.
		size_t direct_decode = items.relationship_scratch_bytes;
		if (!add(direct_decode, sizeof(std::vector<player_item_snapshot>)) ||
		    !add(direct_decode, items.item_codec_decoder_object_bytes))
			return false;
		profile.largest_item_decode_scratch_bytes =
			std::max(profile.largest_item_decode_scratch_bytes, direct_decode);
		payload.offset += item_bytes;
		if (!add(profile.item_count, items.item_count) ||
		    !add(profile.decoded_catalog_payload_bytes, affect_payload_bytes) ||
		    !add(profile.decoded_catalog_payload_bytes,
			 items.decoded_payload_bytes - sizeof(std::vector<player_item_snapshot>)) ||
		    !add(profile.canonical_catalog_bytes, initial_checkpoint_record_bytes) ||
		    !add(profile.canonical_catalog_bytes, affect_wire_bytes) ||
		    !add(profile.canonical_catalog_bytes, sizeof(uint32_t)) ||
		    !add(profile.canonical_catalog_bytes, items.canonical_encoded_bytes))
			return false;
		profile.largest_item_blob_bytes =
			std::max(profile.largest_item_blob_bytes, static_cast<size_t>(item_bytes));
		profile.largest_item_decode_payload_bytes = std::max(
			profile.largest_item_decode_payload_bytes, items.decoded_payload_bytes);
		profile.largest_item_relationship_scratch_bytes =
			std::max(profile.largest_item_relationship_scratch_bytes,
				 items.relationship_scratch_bytes);
		profile.largest_item_canonical_bytes = std::max(
			profile.largest_item_canonical_bytes, items.canonical_encoded_bytes);
		profile.fresh_decode_storage_policy_supported =
			profile.fresh_decode_storage_policy_supported &&
			items.fresh_decode_storage_policy_supported;
	}
	if (payload.offset != payload.size)
		return false;
	*output = profile;
	return true;
}

bool initial_keeper_budget_add(size_t &total, size_t amount) noexcept
{
	if (amount > SIZE_MAX - total)
		return false;
	total += amount;
	return true;
}

// Writes exactly the original catalog v2 scalar/raw byte sequence into one
// pre-sized vector. The caller has already authenticated the bounded catalog's
// complete original predicates; do not reintroduce ordinary hash/tree scratch.
struct initial_keeper_canonical_writer
{
	uint8_t *data;
	size_t size, offset = 0;
	template <typename T> bool number(T value) noexcept
	{
		if (offset > size || sizeof(T) > size - offset)
			return false;
		using U = std::make_unsigned_t<T>;
		U bits = static_cast<U>(value);
		for (size_t i = 0; i < sizeof(T); ++i)
		{
			data[offset++] = static_cast<uint8_t>(bits & 255);
			bits >>= 8;
		}
		return true;
	}
	bool raw(const uint8_t *bytes, size_t length) noexcept
	{
		if ((!bytes && length) || offset > size || length > size - offset)
			return false;
		if (length)
			memcpy(data + offset, bytes, length);
		offset += length;
		return true;
	}
};

bool initial_keeper_canonical_bounded_scratch(const shopkeeper_catalog &catalog,
					      size_t original_size, size_t original_item_bytes,
					      std::vector<uint8_t> *output)
{
	if (!output || catalog.revision != 1 || catalog.records.size() != 1 ||
	    original_size < initial_checkpoint_overhead_bytes ||
	    original_size > initial_checkpoint_maximum_bytes)
		return false;
	const auto &record = catalog.records.front();
	if (record.revision != 1 || record.cash < 0 ||
	    record.cash > std::numeric_limits<int>::max() || record.affects.size() > affect_maximum)
		return false;
	std::vector<uint8_t> items;
	// Original item encoder includes original relationships and complete semantic
	// decode roundtrip. Its exact capacity/reallocation peak was admitted by caller.
	if (player_item_snapshot_list_encode(record.items, &items) !=
		    player_snapshot_codec_result::ok ||
	    items.size() != original_item_bytes)
		return false;
	std::vector<uint8_t> canonical;
	canonical.reserve(original_size); // Exact fresh request under pinned policy.
	canonical.resize(original_size);
	initial_keeper_canonical_writer file{ canonical.data(), canonical.size() };
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest{};
	if (!file.raw(catalog_magic.data(), catalog_magic.size()) ||
	    !file.number<uint32_t>(catalog_version) ||
	    !file.number<uint32_t>(original_size - initial_checkpoint_header_bytes) ||
	    !file.number<uint64_t>(catalog.revision) || !file.raw(digest.data(), digest.size()) ||
	    !file.number<uint32_t>(1) || !file.number(record.shop_id) ||
	    !file.number(record.mob_vnum) || !file.number(record.room_vnum) ||
	    !file.number(record.saved_at) || !file.number(record.revision) ||
	    !file.number(record.cash) || !file.number<uint8_t>(record.roaming ? 1 : 0) ||
	    !file.number<uint32_t>(record.affects.size()))
		return false;
	for (const auto &affect : record.affects)
	{
		if (!file.number(affect.type) || !file.number(affect.duration) ||
		    !file.number(affect.modifier) || !file.number(affect.location))
			return false;
		for (uint64_t bits : affect.bitvectors)
			if (!file.number(bits))
				return false;
	}
	if (!file.number<uint32_t>(items.size()) || !file.raw(items.data(), items.size()) ||
	    file.offset != file.size ||
	    !SHA256(canonical.data() + initial_checkpoint_header_bytes,
		    canonical.size() - initial_checkpoint_header_bytes, digest.data()))
		return false;
	std::copy(digest.begin(), digest.end(), canonical.begin() + 24);
	output->swap(canonical);
	return true;
}

} // namespace

bool flatfile_shopkeeper_initial_checkpoint_encode(const flatfile_shopkeeper_record &record,
						   std::vector<uint8_t> *output) noexcept
{
	if (!output || record.revision != 1 || record.cash < 0 ||
	    record.cash > std::numeric_limits<int>::max() ||
	    record.affects.size() > affect_maximum ||
	    record.items.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return false;
	try
	{
		// Use the original item codec's real limit before copying the record.
		std::vector<uint8_t> bounded_items;
		if (player_item_snapshot_list_encode(record.items, &bounded_items) !=
			    player_snapshot_codec_result::ok ||
		    bounded_items.size() > PLAYER_SNAPSHOT_MAX_BYTES)
			return false;
		shopkeeper_catalog framing;
		framing.revision = 1; // Canonical framing only, never actual whole-catalog state.
		framing.records.emplace_back();
		auto &original = framing.records.front();
		original.shop_id = record.shop_id;
		original.mob_vnum = record.mob_vnum;
		original.room_vnum = record.room_vnum;
		original.saved_at = record.saved_at;
		original.revision = record.revision;
		original.cash = record.cash;
		original.roaming = record.roaming;
		original.affects = record.affects;
		// Decode only the bounded existing wire representation, preserving every
		// encoded item value without copying strings excluded by its original mask.
		if (player_item_snapshot_list_decode(bounded_items.data(), bounded_items.size(),
						     &original.items) !=
		    player_snapshot_codec_result::ok)
			return false;
		std::vector<uint8_t> canonical_items;
		if (player_item_snapshot_list_encode(original.items, &canonical_items) !=
			    player_snapshot_codec_result::ok ||
		    canonical_items != bounded_items)
			return false;
		std::sort(original.affects.begin(), original.affects.end(), affect_less);
		std::vector<uint8_t> encoded;
		if (!encode_catalog(framing, &encoded) || !initial_checkpoint_framing(encoded))
			return false;
		output->swap(encoded);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool flatfile_shopkeeper_initial_checkpoint_decode(const std::vector<uint8_t> &bytes,
						   flatfile_shopkeeper_record *output) noexcept
{
	if (!output || !initial_checkpoint_framing(bytes))
		return false;
	try
	{
		shopkeeper_catalog framing;
		if (!decode_catalog(bytes, &framing) || framing.revision != 1 ||
		    framing.records.size() != 1 || framing.records.front().revision != 1 ||
		    framing.records.front().cash < 0)
			return false;
		std::vector<uint8_t> canonical;
		if (!encode_catalog(framing, &canonical) || canonical != bytes)
			return false;
		static_assert(std::is_nothrow_move_assignable_v<flatfile_shopkeeper_record>);
		*output = std::move(framing.records.front());
		return true;
	}
	catch (...)
	{
		return false;
	}
}

flatfile_shopkeeper_result flatfile_shopkeeper_initial_checkpoint_decode_bounded(
	const std::vector<uint8_t> &bytes, flatfile_shopkeeper_record *output,
	flatfile_scratch_reserve_fn reserve_scratch_peak, void *context,
	size_t outer_live_scratch) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	errno = ENOTSUP;
	return flatfile_shopkeeper_result::io_error;
#else
	if (!output || !reserve_scratch_peak)
	{
		errno = EINVAL;
		return flatfile_shopkeeper_result::invalid;
	}
	// Admit actual named inline scan objects before inspecting the nested item
	// wire. No sizing step calls SHA or an allocating decoder/container method.
	constexpr size_t caller_fixed = sizeof(shopkeeper_catalog) +
					sizeof(flatfile_shopkeeper_catalog_allocation_profile) +
					sizeof(player_item_snapshot_list_allocation_profile) +
					sizeof(std::vector<uint8_t>);
	constexpr size_t scan_extra = sizeof(flatfile_shopkeeper_catalog_allocation_profile) +
				      2 * sizeof(player_item_snapshot_list_allocation_profile) +
				      2 * sizeof(decoder);
	size_t base = outer_live_scratch, scan_live = 0;
	if (!initial_keeper_budget_add(base, caller_fixed))
	{
		errno = ENOBUFS;
		return flatfile_shopkeeper_result::io_error;
	}
	scan_live = base;
	if (!initial_keeper_budget_add(scan_live, scan_extra) ||
	    !initial_keeper_budget_add(scan_live,
				       player_item_snapshot_list_decoder_object_bytes()) ||
	    !reserve_scratch_peak(scan_live, context))
	{
		errno = ENOBUFS;
		return flatfile_shopkeeper_result::io_error;
	}
	if (!initial_checkpoint_framing(bytes))
	{
		errno = EBADMSG;
		return flatfile_shopkeeper_result::invalid;
	}
	flatfile_shopkeeper_catalog_allocation_profile profile;
	player_item_snapshot_list_allocation_profile items;
	if (!flatfile_shopkeeper_catalog_preflight(bytes.data(), bytes.size(), &profile) ||
	    profile.record_count != 1 || profile.canonical_catalog_bytes != bytes.size() ||
	    profile.largest_item_blob_bytes > bytes.size() ||
	    player_item_snapshot_list_preflight(
		    bytes.data() + bytes.size() - profile.largest_item_blob_bytes,
		    profile.largest_item_blob_bytes, &items) != player_snapshot_codec_result::ok)
	{
		errno = EBADMSG;
		return flatfile_shopkeeper_result::invalid;
	}
	if (!profile.fresh_decode_storage_policy_supported ||
	    !items.fresh_decode_storage_policy_supported ||
	    !items.canonical_encoder_storage_policy_supported)
	{
		errno = ENOTSUP;
		return flatfile_shopkeeper_result::io_error;
	}
	// The local decoded catalog and its complete retained rows/AF/item payload
	// are included in profile; caller framing's separate object is in base.
	constexpr size_t decode_fixed =
		2 * sizeof(decoder) + sizeof(std::array<uint8_t, SHA256_DIGEST_LENGTH>) +
		sizeof(std::vector<uint64_t>) + sizeof(std::vector<uint8_t>) +
		sizeof(std::array<bool, equipment_slot_maximum + 1>);
	size_t validation = profile.largest_item_roundtrip_scratch_bytes;
	if (profile.item_count > SIZE_MAX / sizeof(uint64_t) ||
	    !initial_keeper_budget_add(validation, profile.item_count * sizeof(uint64_t)))
	{
		errno = ENOBUFS;
		return flatfile_shopkeeper_result::io_error;
	}
	size_t decode_live = base;
	if (!initial_keeper_budget_add(decode_live, profile.decoded_catalog_payload_bytes) ||
	    !initial_keeper_budget_add(decode_live, decode_fixed) ||
	    !initial_keeper_budget_add(
		    decode_live, std::max(validation, profile.largest_item_decode_scratch_bytes)) ||
	    !reserve_scratch_peak(decode_live, context))
	{
		errno = ENOBUFS;
		return flatfile_shopkeeper_result::io_error;
	}
	try
	{
		shopkeeper_catalog framing;
		if (!decode_catalog_bounded_scratch(bytes, &framing, profile.item_count) ||
		    framing.revision != 1 || framing.records.size() != 1 ||
		    framing.records.front().revision != 1 || framing.records.front().cash < 0)
		{
			errno = EBADMSG;
			return flatfile_shopkeeper_result::invalid;
		}
		// Canonical writer uses original item encoder, then one exact-sized file
		// vector; no whole-catalog payload encoder, hash/tree or affects clone.
		constexpr size_t canonical_fixed =
			2 * sizeof(std::vector<uint8_t>) + sizeof(initial_keeper_canonical_writer) +
			sizeof(std::array<uint8_t, SHA256_DIGEST_LENGTH>);
		size_t canonical_payload = items.canonical_encoded_capacity_bytes;
		size_t canonical_live = base;
		if (!initial_keeper_budget_add(canonical_payload, bytes.size()) ||
		    profile.decoded_catalog_payload_bytes < sizeof(shopkeeper_catalog) ||
		    !initial_keeper_budget_add(canonical_live,
					       profile.decoded_catalog_payload_bytes -
						       sizeof(shopkeeper_catalog)) ||
		    !initial_keeper_budget_add(canonical_live, canonical_fixed) ||
		    !initial_keeper_budget_add(
			    canonical_live,
			    std::max(canonical_payload,
				     profile.largest_item_roundtrip_scratch_bytes)) ||
		    !reserve_scratch_peak(canonical_live, context))
		{
			errno = ENOBUFS;
			return flatfile_shopkeeper_result::io_error;
		}
		std::vector<uint8_t> canonical;
		if (!initial_keeper_canonical_bounded_scratch(
			    framing, bytes.size(), profile.largest_item_blob_bytes, &canonical) ||
		    canonical != bytes)
		{
			errno = EBADMSG;
			return flatfile_shopkeeper_result::invalid;
		}
		static_assert(std::is_nothrow_move_assignable_v<flatfile_shopkeeper_record>);
		*output = std::move(framing.records.front());
		return flatfile_shopkeeper_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return flatfile_shopkeeper_result::io_error;
	}
	catch (...)
	{
		errno = EOVERFLOW;
		return flatfile_shopkeeper_result::io_error;
	}
#endif
}

bool flatfile_shopkeeper_catalog_preflight(
	const uint8_t *encoded, size_t encoded_size,
	flatfile_shopkeeper_catalog_allocation_profile *output) noexcept
{
	if (!encoded || !output ||
	    encoded_size < initial_checkpoint_header_bytes + sizeof(uint32_t) ||
	    encoded_size > catalog_maximum_bytes ||
	    memcmp(encoded, catalog_magic.data(), catalog_magic.size()))
		return false;
	decoder header{ encoded + catalog_magic.size(), encoded_size - catalog_magic.size() };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || (version != 1 && version != catalog_version) ||
	    !header.number(&payload_size) ||
	    payload_size != encoded_size - initial_checkpoint_header_bytes ||
	    !header.number(&revision) || !revision)
		return false;
	// Sizing only: actual decode_catalog still authenticates the digest.
	const uint8_t *payload_bytes = encoded + initial_checkpoint_header_bytes;
	decoder payload{ payload_bytes, payload_size };
	uint32_t count = 0;
	if (!payload.number(&count) || count > shopkeeper_maximum)
		return false;
	const auto add = [](size_t &total, size_t amount) noexcept
	{
		if (amount > SIZE_MAX - total)
			return false;
		total += amount;
		return true;
	};
	const auto product = [](size_t count_value, size_t width, size_t *result) noexcept
	{
		if (!result || (width && count_value > SIZE_MAX / width))
			return false;
		*result = count_value * width;
		return true;
	};
	flatfile_shopkeeper_catalog_allocation_profile profile;
	profile.record_count = count;
	profile.decoded_catalog_payload_bytes = sizeof(shopkeeper_catalog);
	profile.canonical_catalog_bytes = initial_checkpoint_header_bytes + sizeof(uint32_t);
#if defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI
	profile.fresh_decode_storage_policy_supported = true;
#endif
	size_t row_bytes = 0;
	if (!product(count, sizeof(flatfile_shopkeeper_record), &row_bytes) ||
	    !add(profile.decoded_catalog_payload_bytes, row_bytes))
		return false;
	for (uint32_t index = 0; index < count; ++index)
	{
		uint32_t shop = 0, affect_count = 0, item_bytes = 0;
		int32_t mobile = 0, room = 0;
		int64_t saved_at = 0, cash = -1;
		uint64_t record_revision = 0;
		uint8_t roaming = 0;
		if (!payload.number(&shop) || !payload.number(&mobile) || !payload.number(&room) ||
		    !payload.number(&saved_at) || !payload.number(&record_revision) ||
		    (version == catalog_version &&
		     (!payload.number(&cash) || !payload.number(&roaming))) ||
		    roaming > 1 || !payload.number(&affect_count) || affect_count > affect_maximum)
			return false;
		size_t affect_wire_bytes = 0, affect_payload_bytes = 0;
		if (!product(affect_count, initial_checkpoint_affect_bytes, &affect_wire_bytes) ||
		    !product(affect_count, sizeof(flatfile_shopkeeper_affect_record),
			     &affect_payload_bytes) ||
		    payload.offset > payload.size ||
		    affect_wire_bytes > payload.size - payload.offset)
			return false;
		payload.offset += affect_wire_bytes;
		if (!payload.number(&item_bytes) || !item_bytes ||
		    item_bytes > PLAYER_SNAPSHOT_MAX_BYTES || payload.offset > payload.size ||
		    item_bytes > payload.size - payload.offset)
			return false;
		player_item_snapshot_list_allocation_profile items;
		if (player_item_snapshot_list_preflight(payload.data + payload.offset, item_bytes,
							&items) !=
			    player_snapshot_codec_result::ok ||
		    items.decoded_payload_bytes < sizeof(std::vector<player_item_snapshot>))
			return false;
		if (!items.canonical_encoder_storage_policy_supported)
			return false;
		size_t validation = items.canonical_encoded_capacity_bytes;
		if (!add(validation, items.decoded_payload_bytes) ||
		    !add(validation, items.relationship_scratch_bytes) ||
		    !items.item_codec_decoder_object_bytes ||
		    !add(validation, items.item_codec_decoder_object_bytes) ||
		    !add(validation, sizeof(std::vector<player_item_snapshot>)))
			return false;
		validation = std::max(validation, items.canonical_encoded_reallocation_peak_bytes);
		if (!add(validation, items.canonical_encoder_object_bytes))
			return false;
		profile.largest_item_roundtrip_scratch_bytes =
			std::max(profile.largest_item_roundtrip_scratch_bytes, validation);
		// Parent record contains its retained vector head; the decoder's
		// separate local item vector, decoder object and depth vector coexist.
		size_t direct_decode = items.relationship_scratch_bytes;
		if (!add(direct_decode, sizeof(std::vector<player_item_snapshot>)) ||
		    !add(direct_decode, items.item_codec_decoder_object_bytes))
			return false;
		profile.largest_item_decode_scratch_bytes =
			std::max(profile.largest_item_decode_scratch_bytes, direct_decode);
		payload.offset += item_bytes;
		if (!add(profile.item_count, items.item_count) ||
		    !add(profile.decoded_catalog_payload_bytes, affect_payload_bytes) ||
		    !add(profile.decoded_catalog_payload_bytes,
			 items.decoded_payload_bytes - sizeof(std::vector<player_item_snapshot>)) ||
		    !add(profile.canonical_catalog_bytes, initial_checkpoint_record_bytes) ||
		    !add(profile.canonical_catalog_bytes, affect_wire_bytes) ||
		    !add(profile.canonical_catalog_bytes, sizeof(uint32_t)) ||
		    !add(profile.canonical_catalog_bytes, items.canonical_encoded_bytes))
			return false;
		profile.largest_item_blob_bytes =
			std::max(profile.largest_item_blob_bytes, static_cast<size_t>(item_bytes));
		profile.largest_item_decode_payload_bytes = std::max(
			profile.largest_item_decode_payload_bytes, items.decoded_payload_bytes);
		profile.largest_item_relationship_scratch_bytes =
			std::max(profile.largest_item_relationship_scratch_bytes,
				 items.relationship_scratch_bytes);
		profile.largest_item_canonical_bytes = std::max(
			profile.largest_item_canonical_bytes, items.canonical_encoded_bytes);
		profile.fresh_decode_storage_policy_supported =
			profile.fresh_decode_storage_policy_supported &&
			items.fresh_decode_storage_policy_supported;
	}
	if (payload.offset != payload.size)
		return false;
	*output = profile;
	return true;
}

flatfile_shopkeeper_result
flatfile_shopkeeper_establish(const std::string &root,
			      const std::vector<flatfile_shopkeeper_record> &records,
			      std::string *error)
{
	if (root.empty())
		return flatfile_shopkeeper_result::invalid;
	shopkeeper_catalog candidate;
	try
	{
		candidate.records = records;
		for (auto &record : candidate.records)
			std::sort(record.affects.begin(), record.affects.end(), affect_less);
		std::sort(candidate.records.begin(), candidate.records.end(), record_less);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_shopkeeper_result::io_error;
	}
	if (!valid_catalog(candidate))
		return flatfile_shopkeeper_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_shopkeeper_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_shopkeeper_result::ok)
		return recovered;
	shopkeeper_catalog existing;
	const auto loaded = load_catalog(root, &existing, error);
	if (loaded == flatfile_shopkeeper_result::ok)
		return catalog_equal(existing, candidate) ?
			       flatfile_shopkeeper_result::already_exists :
			       flatfile_shopkeeper_result::invalid;
	if (loaded != flatfile_shopkeeper_result::not_found)
		return loaded;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(candidate, &encoded))
		return flatfile_shopkeeper_result::invalid;
	if (!flatfile_atomic_write(domains_directory(root), catalog_filename, encoded, error))
		return flatfile_shopkeeper_result::io_error;
	return flatfile_shopkeeper_result::ok;
}

flatfile_shopkeeper_result
flatfile_shopkeeper_list(const std::string &root, std::vector<flatfile_shopkeeper_record> *records,
			 std::string *error)
{
	if (root.empty() || !records)
		return flatfile_shopkeeper_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_shopkeeper_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_shopkeeper_result::ok)
		return recovered;
	shopkeeper_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_shopkeeper_result::ok)
		return loaded;
	*records = std::move(catalog.records);
	return flatfile_shopkeeper_result::ok;
}

flatfile_shopkeeper_result
flatfile_shopkeeper_list_locked(const std::string &root, const flatfile_authority_lock &lock,
				std::vector<flatfile_shopkeeper_record> *records,
				std::string *error)
{
	if (root.empty() || !records || !lock.matches(root))
		return flatfile_shopkeeper_result::invalid;
	try
	{
		shopkeeper_catalog catalog;
		const auto loaded = load_catalog(root, &catalog, error);
		if (loaded != flatfile_shopkeeper_result::ok)
			return loaded;
		*records = std::move(catalog.records);
		return flatfile_shopkeeper_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_shopkeeper_result::io_error;
	}
}

flatfile_shopkeeper_result
flatfile_shopkeeper_read_trade_after_image(const flatfile_authority_after_image &image,
					   uint32_t shop_id, flatfile_shopkeeper_record *record,
					   std::string * /*error*/)
{
	if (!record || image.filename != catalog_filename || image.bytes.empty() ||
	    image.bytes.size() > catalog_maximum_bytes)
		return flatfile_shopkeeper_result::invalid;
	try
	{
		shopkeeper_catalog catalog;
		if (!decode_catalog(image.bytes, &catalog))
			return flatfile_shopkeeper_result::invalid;
		// prepare_trade emits the original canonical current-version catalog.
		// Reject legacy/alternate bytes rather than treating them as a proposal.
		std::vector<uint8_t> canonical;
		if (!encode_catalog(catalog, &canonical) || canonical != image.bytes)
			return flatfile_shopkeeper_result::invalid;
		auto selected =
			std::lower_bound(catalog.records.begin(), catalog.records.end(), shop_id,
					 [](const flatfile_shopkeeper_record &candidate,
					    uint32_t id) { return candidate.shop_id < id; });
		if (selected == catalog.records.end() || selected->shop_id != shop_id)
			return flatfile_shopkeeper_result::not_found;
		if (selected->cash < 0)
			return flatfile_shopkeeper_result::invalid;
		*record = std::move(*selected);
		return flatfile_shopkeeper_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_shopkeeper_result::io_error;
	}
}

flatfile_shopkeeper_result flatfile_shopkeeper_replace(const std::string &root,
						       const flatfile_shopkeeper_record &record,
						       uint64_t expected_revision,
						       std::string *error)
{
	if (root.empty() || !expected_revision ||
	    expected_revision == std::numeric_limits<uint64_t>::max() ||
	    record.revision != expected_revision + 1)
		return flatfile_shopkeeper_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_shopkeeper_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_shopkeeper_result::ok)
		return recovered;
	shopkeeper_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_shopkeeper_result::ok)
		return loaded;
	auto existing = std::lower_bound(catalog.records.begin(), catalog.records.end(), record,
					 [](const flatfile_shopkeeper_record &candidate,
					    const flatfile_shopkeeper_record &value)
					 { return record_less(candidate, value); });
	if (existing == catalog.records.end() || existing->shop_id != record.shop_id)
		return flatfile_shopkeeper_result::not_found;
	if (existing->revision != expected_revision)
		return flatfile_shopkeeper_result::stale;
	try
	{
		*existing = record;
		std::sort(existing->affects.begin(), existing->affects.end(), affect_less);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_shopkeeper_result::io_error;
	}
	if (!valid_catalog(catalog) || catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_shopkeeper_result::invalid;
	++catalog.revision;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(catalog, &encoded))
		return flatfile_shopkeeper_result::invalid;
	if (!flatfile_atomic_write(domains_directory(root), catalog_filename, encoded, error))
		return flatfile_shopkeeper_result::io_error;
	return flatfile_shopkeeper_result::ok;
}

flatfile_shopkeeper_result
flatfile_shopkeeper_prepare_trade(const std::string &root, const flatfile_authority_lock &lock,
				  const shop_trade_payload &payload,
				  flatfile_shopkeeper_trade_mutation *mutation,
				  unsigned int *result_code, std::string *error)
{
	if (root.empty() || !lock.matches(root) || !mutation || !result_code)
		return flatfile_shopkeeper_result::invalid;
	*mutation = {};
	*result_code = 0;
	std::vector<uint8_t> validated_payload;
	if (!shop_trade_command_encode_payload(payload, &validated_payload))
		return flatfile_shopkeeper_result::invalid;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_shopkeeper_result::ok)
		return recovered;
	std::vector<player_item_snapshot> trade_items;
	const auto decoded = player_item_snapshot_list_decode(payload.item_blob.data(),
							      payload.item_blob_size, &trade_items);
	if (decoded == player_snapshot_codec_result::allocation_failure)
		return flatfile_shopkeeper_result::io_error;
	if (decoded != player_snapshot_codec_result::ok ||
	    !trade_items_match_payload(payload, trade_items))
	{
		*result_code = EINVAL;
		return flatfile_shopkeeper_result::ok;
	}
	if (payload.action == shop_trade_action::sell_store &&
	    std::any_of(trade_items.begin(), trade_items.end(),
			[](const auto &item) { return !item.dynamic_affects.empty(); }))
	{
		*result_code = EOPNOTSUPP;
		return flatfile_shopkeeper_result::ok;
	}
	std::vector<uint8_t> canonical_blob;
	const auto encoded = player_item_snapshot_list_encode(trade_items, &canonical_blob);
	if (encoded == player_snapshot_codec_result::allocation_failure)
		return flatfile_shopkeeper_result::io_error;
	if (encoded != player_snapshot_codec_result::ok ||
	    canonical_blob.size() != payload.item_blob_size ||
	    !std::equal(canonical_blob.begin(), canonical_blob.end(), payload.item_blob.begin()))
	{
		*result_code = EINVAL;
		return flatfile_shopkeeper_result::ok;
	}
	shopkeeper_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_shopkeeper_result::ok)
		return loaded;
	auto record =
		std::lower_bound(catalog.records.begin(), catalog.records.end(), payload.shop_id,
				 [](const flatfile_shopkeeper_record &candidate, uint32_t shop_id)
				 { return candidate.shop_id < shop_id; });
	if (record == catalog.records.end() || record->shop_id != payload.shop_id)
	{
		*result_code = ENOENT;
		return flatfile_shopkeeper_result::ok;
	}
	if (record->revision != payload.expected_shop_revision)
	{
		*result_code = ESTALE;
		return flatfile_shopkeeper_result::ok;
	}
	if (payload.keeper_vnum && (record->mob_vnum != payload.keeper_vnum || record->cash < 0 ||
				    record->cash != payload.expected_keeper_cash ||
				    record->roaming != static_cast<bool>(payload.keeper_roaming)))
	{
		*result_code = ESTALE;
		return flatfile_shopkeeper_result::ok;
	}
	if (record->revision == std::numeric_limits<uint64_t>::max() ||
	    catalog.revision == std::numeric_limits<uint64_t>::max())
	{
		*result_code = ERANGE;
		return flatfile_shopkeeper_result::ok;
	}
	if (payload.action == shop_trade_action::buy_existing ||
	    payload.action == shop_trade_action::discard_invalid)
	{
		std::vector<player_item_snapshot> stored_items;
		std::vector<bool> selected_rows;
		if (!select_subtree(record->items, payload.stock_item_uid, &stored_items,
				    &selected_rows))
		{
			*result_code = ESTALE;
			return flatfile_shopkeeper_result::ok;
		}
		std::vector<uint8_t> stored_blob;
		if (player_item_snapshot_list_encode(stored_items, &stored_blob) !=
		    player_snapshot_codec_result::ok)
			return flatfile_shopkeeper_result::io_error;
		if (stored_blob != canonical_blob)
		{
			*result_code = ESTALE;
			return flatfile_shopkeeper_result::ok;
		}
		if (!remove_rows(&record->items, selected_rows))
			return flatfile_shopkeeper_result::io_error;
	}
	else if (payload.action == shop_trade_action::buy_produced)
	{
		auto stock = std::find_if(record->items.begin(), record->items.end(),
					  [&](const auto &item)
					  { return item.object_uid == payload.stock_item_uid; });
		if (stock == record->items.end() ||
		    stock->parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
		    stock->equipment_slot != 0 || stock->vnum != payload.stock_vnum ||
		    trade_items.front().vnum != payload.stock_vnum)
		{
			*result_code = ESTALE;
			return flatfile_shopkeeper_result::ok;
		}
	}
	else if (payload.action == shop_trade_action::sell_store)
	{
		if (!append_trade_items(&record->items, trade_items))
			return flatfile_shopkeeper_result::io_error;
	}
	else if (payload.action != shop_trade_action::sell_destroy)
	{
		*result_code = EINVAL;
		return flatfile_shopkeeper_result::ok;
	}
	if (payload.keeper_vnum && payload.action != shop_trade_action::discard_invalid)
	{
		if (payload.action == shop_trade_action::buy_existing ||
		    payload.action == shop_trade_action::buy_produced)
		{
			if (record->cash > std::numeric_limits<int>::max() - payload.price)
			{
				*result_code = ERANGE;
				return flatfile_shopkeeper_result::ok;
			}
			record->cash += payload.price;
		}
		else if (record->cash >= payload.price)
			record->cash -= payload.price;
		else if (record->roaming && record->mob_vnum != 11005)
		{
			*result_code = ENOSPC;
			return flatfile_shopkeeper_result::ok;
		}
	}
	++record->revision;
	++catalog.revision;
	if (!valid_catalog(catalog))
	{
		*result_code = EINVAL;
		return flatfile_shopkeeper_result::ok;
	}
	mutation->shop_revision = record->revision;
	mutation->keeper_cash = record->cash;
	mutation->after_image.filename = catalog_filename;
	if (!encode_catalog(catalog, &mutation->after_image.bytes))
		return flatfile_shopkeeper_result::io_error;
	return flatfile_shopkeeper_result::ok;
}

#include "flatfile/flatfile_shopkeeper_ownership.h"
#include "core/defines.h"
#include <unordered_map>

namespace
{
bool checkpoint_custody_equal(const flatfile_item_ownership_record &left,
			      const flatfile_item_ownership_record &right)
{
	return left.item_uid == right.item_uid && left.root_item_uid == right.root_item_uid &&
	       left.parent_item_uid == right.parent_item_uid &&
	       item_owner_identity_equal(left.owner, right.owner) &&
	       left.item_revision == right.item_revision && left.vnum == right.vnum &&
	       left.state == right.state && left.coin_payload == right.coin_payload &&
	       left.equipment_slot == right.equipment_slot;
}

// The coin payload uses its original independent one-item position. Compare
// complete canonical properties after authenticating position through custody.
bool checkpoint_coin_body_equal(player_item_snapshot left, player_item_snapshot right)
{
	left.parent_index = right.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	left.equipment_slot = right.equipment_slot = 0;
	std::vector<uint8_t> left_bytes, right_bytes;
	return player_item_snapshot_list_encode({ left }, &left_bytes) ==
		       player_snapshot_codec_result::ok &&
	       player_item_snapshot_list_encode({ right }, &right_bytes) ==
		       player_snapshot_codec_result::ok &&
	       left_bytes == right_bytes;
}
} // namespace

flatfile_shopkeeper_result flatfile_shopkeeper_source_checkpoint_storage::prepare_locked(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t shop_id,
	uint64_t expected_shop_revision, int32_t original_keeper_vnum, int64_t original_cash,
	bool original_roaming, std::span<const player_item_snapshot> original_keeper_literal,
	const std::vector<flatfile_item_ownership_record> &actual_keeper_custody,
	uint64_t actual_keeper_owner_revision, flatfile_shopkeeper_record *original_before,
	flatfile_shopkeeper_record *actual_after,
	flatfile_authority_after_image *complete_catalog_after, std::string *error)
{
	if (root.empty() || !lock.matches(root) || !original_before || !actual_after ||
	    original_before == actual_after || !complete_catalog_after || !expected_shop_revision ||
	    expected_shop_revision == UINT64_MAX || !actual_keeper_owner_revision ||
	    original_keeper_vnum <= 0 || original_cash < 0 ||
	    original_cash > std::numeric_limits<int>::max() ||
	    original_keeper_literal.size() > PLAYER_SNAPSHOT_MAX_OBJECTS ||
	    actual_keeper_custody.size() != original_keeper_literal.size())
		return flatfile_shopkeeper_result::invalid;
	try
	{
		shopkeeper_catalog catalog;
		const auto loaded = load_catalog(root, &catalog, error);
		if (loaded != flatfile_shopkeeper_result::ok)
			return loaded;
		auto selected =
			std::lower_bound(catalog.records.begin(), catalog.records.end(), shop_id,
					 [](const flatfile_shopkeeper_record &record, uint32_t id)
					 { return record.shop_id < id; });
		if (selected == catalog.records.end() || selected->shop_id != shop_id)
			return flatfile_shopkeeper_result::not_found;
		if (selected->revision != expected_shop_revision ||
		    selected->mob_vnum != original_keeper_vnum || selected->cash < 0 ||
		    selected->cash != original_cash || selected->roaming != original_roaming)
			return flatfile_shopkeeper_result::stale;
		if (catalog.revision == UINT64_MAX)
			return flatfile_shopkeeper_result::invalid;

		// Re-read the genuine active owner cut; an input vector or clock alone
		// cannot confer native catalog authority. Shop zero uses the real UID
		// namespace function, never the raw shop index as an owner ID.
		uint64_t owner_revision = 0;
		std::vector<flatfile_item_ownership_record> custody;
		const auto owned = flatfile_item_repository_load_owner_locked(
			root, lock, flatfile_shopkeeper_item_owner(shop_id), &owner_revision,
			&custody, error);
		if (owned != flatfile_item_repository_result::ok)
			return owned == flatfile_item_repository_result::io_error ?
				       flatfile_shopkeeper_result::io_error :
			       owned == flatfile_item_repository_result::not_found ?
				       flatfile_shopkeeper_result::not_found :
				       flatfile_shopkeeper_result::invalid;
		if (owner_revision != actual_keeper_owner_revision ||
		    custody.size() != actual_keeper_custody.size())
			return flatfile_shopkeeper_result::stale;
		std::unordered_map<uint64_t, const flatfile_item_ownership_record *> by_uid;
		for (const auto &entry : custody)
			if (!by_uid.emplace(entry.item_uid, &entry).second)
				return flatfile_shopkeeper_result::invalid;
		std::unordered_set<uint64_t> supplied;
		for (const auto &entry : actual_keeper_custody)
		{
			const auto found = by_uid.find(entry.item_uid);
			if (!supplied.insert(entry.item_uid).second || found == by_uid.end() ||
			    !checkpoint_custody_equal(entry, *found->second))
				return flatfile_shopkeeper_result::stale;
		}

		flatfile_shopkeeper_record before = *selected;
		// The original catalog decoder already proves global UID uniqueness.
		// Index its immutable BEFORE items once; legacy coins must not rescan
		// the complete forest for each current item under the authority lock.
		std::unordered_map<uint64_t, const player_item_snapshot *> before_by_uid;
		before_by_uid.reserve(before.items.size());
		for (const auto &item : before.items)
			before_by_uid.emplace(item.object_uid, &item);
		flatfile_shopkeeper_record after = before;
		after.items.assign(original_keeper_literal.begin(), original_keeper_literal.end());
		++after.revision;
		std::vector<player_load_item_identity> identities;
		const auto reconciled = flatfile_shopkeeper_reconcile_item_ownership(
			after, owner_revision, custody, &identities);
		if (reconciled != flatfile_shopkeeper_ownership_result::ok)
			return reconciled == flatfile_shopkeeper_ownership_result::io_error ?
				       flatfile_shopkeeper_result::io_error :
				       flatfile_shopkeeper_result::invalid;
		// The original reconciler proves root/parent/UID/vnum/revision/state.
		// Add its omitted exact position and current coin property comparisons;
		// general literal properties remain the original native owner's capture.
		for (const auto &item : after.items)
		{
			const auto &entry = *by_uid.at(item.object_uid);
			if (item.equipment_slot < 0 ||
			    static_cast<uint16_t>(item.equipment_slot) != entry.equipment_slot)
				return flatfile_shopkeeper_result::stale;
			if (item.type == ITEM_MONEY &&
			    std::any_of(item.values.begin(), item.values.begin() + 4,
					[](int32_t value) { return value < 0; }))
				return flatfile_shopkeeper_result::invalid;
			if (!entry.coin_payload.empty())
			{
				std::vector<player_item_snapshot> coins;
				if (player_item_snapshot_list_decode(
					    entry.coin_payload.data(), entry.coin_payload.size(),
					    &coins) != player_snapshot_codec_result::ok ||
				    coins.size() != 1 || item.type != ITEM_MONEY ||
				    !checkpoint_coin_body_equal(item, coins.front()))
					return flatfile_shopkeeper_result::stale;
			}
			else if (item.type == ITEM_MONEY)
			{
				const auto saved = before_by_uid.find(item.object_uid);
				if (saved == before_by_uid.end() ||
				    saved->second->type != ITEM_MONEY ||
				    !checkpoint_coin_body_equal(item, *saved->second))
					return flatfile_shopkeeper_result::stale;
			}
		}
		*selected = after;
		++catalog.revision;
		flatfile_authority_after_image image;
		image.filename = catalog_filename;
		if (!encode_catalog(catalog, &image.bytes))
			return flatfile_shopkeeper_result::invalid;
		// No throwing work remains: publish all three proposed values together.
		*original_before = std::move(before);
		*actual_after = std::move(after);
		*complete_catalog_after = std::move(image);
		return flatfile_shopkeeper_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_shopkeeper_result::io_error;
	}
}

flatfile_shopkeeper_result
flatfile_shopkeeper_source_checkpoint_storage::read_current_catalog_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	flatfile_authority_after_image *original_catalog, std::string *error)
{
	if (root.empty() || !lock.matches(root) || !original_catalog)
		return flatfile_shopkeeper_result::invalid;
	try
	{
		flatfile_authority_after_image image;
		image.filename = catalog_filename;
		const auto loaded = flatfile_read(domains_directory(root), catalog_filename,
						  catalog_maximum_bytes, &image.bytes, error);
		if (loaded == flatfile_read_result::not_found)
			return flatfile_shopkeeper_result::not_found;
		if (loaded == flatfile_read_result::io_error)
			return flatfile_shopkeeper_result::io_error;
		shopkeeper_catalog catalog;
		if (loaded != flatfile_read_result::ok || !decode_catalog(image.bytes, &catalog))
		{
			if (error && error->empty())
				*error = "shopkeeper catalog is corrupt";
			return flatfile_shopkeeper_result::invalid;
		}
		// Original bounded/checksummed/versioned decoder validates the whole
		// catalog. Preserve its real raw representation and header revision.
		// This is a value, not journal/recovery/write/publication authority.
		*original_catalog = std::move(image);
		return flatfile_shopkeeper_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_shopkeeper_result::io_error;
	}
}

flatfile_shopkeeper_result flatfile_shopkeeper_initial_catalog_storage::prepare_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const std::vector<uint8_t> &original_initial_checkpoint,
	flatfile_shopkeeper_initial_catalog_stage *output, std::string *error) noexcept
{
	if (root.empty() || !output || !lock.matches(root))
		return flatfile_shopkeeper_result::invalid;
	try
	{
		// Exact one-record v2 framing, revision1, observed cash, all affects
		// and full literal forest. No reconstruction from current templates.
		flatfile_shopkeeper_record initial;
		if (!flatfile_shopkeeper_initial_checkpoint_decode(original_initial_checkpoint,
								   &initial))
			return flatfile_shopkeeper_result::invalid;
		shopkeeper_catalog catalog;
		const auto loaded = load_catalog(root, &catalog, error);
		if (loaded != flatfile_shopkeeper_result::ok &&
		    loaded != flatfile_shopkeeper_result::not_found)
			return loaded;
		flatfile_shopkeeper_initial_catalog_stage stage;
		stage.catalog_before_present = loaded == flatfile_shopkeeper_result::ok;
		if (stage.catalog_before_present)
		{
			stage.catalog_before_revision = catalog.revision;
			if (catalog.revision == UINT64_MAX)
				return flatfile_shopkeeper_result::invalid;
		}
		// A genuinely absent file uses the SAME original establish clock1;
		// corruption, unknown read outcomes and an existing SHOP never do.
		auto selected = std::lower_bound(catalog.records.begin(), catalog.records.end(),
						 initial.shop_id,
						 [](const flatfile_shopkeeper_record &record,
						    uint32_t id) { return record.shop_id < id; });
		if (selected != catalog.records.end() && selected->shop_id == initial.shop_id)
			return flatfile_shopkeeper_result::already_exists;
		if (catalog.records.size() >= shopkeeper_maximum)
			return flatfile_shopkeeper_result::invalid;
		catalog.records.insert(selected, std::move(initial));
		if (stage.catalog_before_present)
			++catalog.revision;
		stage.catalog_after_revision = catalog.revision;
		stage.operation.store = flatfile_authority_store::domains;
		stage.operation.kind = flatfile_authority_operation_kind::write;
		stage.operation.filename = catalog_filename;
		// Original codec rechecks complete bounds, cross-keeper UID uniqueness,
		// EQ/INV forests and sorted affects. Unrelated v1 cash stays -1 in v2;
		// it is neither measured cash nor a guessed replacement balance.
		if (!encode_catalog(catalog, &stage.operation.bytes) || !lock.matches(root))
			return flatfile_shopkeeper_result::invalid;
		static_assert(
			std::is_nothrow_move_assignable_v<flatfile_shopkeeper_initial_catalog_stage>);
		*output = std::move(stage);
		return flatfile_shopkeeper_result::ok;
	}
	catch (...)
	{
		// No write or recovery has started; every allocation/exception refuses
		// while preserving the caller's entire previous proposal.
		return flatfile_shopkeeper_result::io_error;
	}
}

flatfile_shopkeeper_result
flatfile_shopkeeper_list_locked_bounded(const std::string &root,
					const flatfile_authority_lock &lock,
					std::vector<flatfile_shopkeeper_record> *records,
					flatfile_scratch_reserve_fn reserve_scratch_peak,
					void *context, size_t outer_live_scratch) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	errno = ENOTSUP;
	return flatfile_shopkeeper_result::io_error;
#else
	if (root.empty() || !records || !reserve_scratch_peak || !lock.matches(root))
	{
		errno = EINVAL;
		return flatfile_shopkeeper_result::invalid;
	}
	shop_resource_state resource;
	const auto add = [](size_t &total, size_t amount) noexcept
	{
		if (amount > SIZE_MAX - total)
			return false;
		total += amount;
		return true;
	};
	// Explicit fixed storage for both reader and bounded decoder/validator
	// scopes. The catalog's inline object is already included in its profile.
	constexpr size_t fixed = sizeof(shop_resource_state) + 4 * sizeof(shop_resource_state *) +
				 2 * sizeof(player_snapshot_codec_result) +
				 2 * sizeof(std::string) + sizeof(std::vector<uint8_t>) +
				 2 * sizeof(flatfile_shopkeeper_catalog_allocation_profile) +
				 2 * sizeof(player_item_snapshot_list_allocation_profile) +
				 sizeof(shopkeeper_catalog) + sizeof(std::vector<uint64_t>) +
				 sizeof(std::vector<uint8_t>) +
				 sizeof(std::array<bool, equipment_slot_maximum + 1>) +
				 3 * sizeof(decoder) +
				 sizeof(std::array<uint8_t, SHA256_DIGEST_LENGTH>);
	size_t directory_size = root.size();
	size_t live = outer_live_scratch;
	if (!add(directory_size, sizeof("/domains") - 1) || !add(live, fixed) ||
	    !add(live, player_item_snapshot_list_decoder_object_bytes()) ||

	    (directory_size > 15 &&
	     (directory_size == SIZE_MAX || !add(live, directory_size + 1))) ||
	    !add(live, sizeof("shopkeeper_catalog")) || !reserve_scratch_peak(live, context))
	{
		errno = ENOBUFS;
		return flatfile_shopkeeper_result::io_error;
	}
	try
	{
		// Fresh length constructor requests exactly n+1 for non-SSO strings
		// under the pinned policy; no uncharged operator+ growth or root copy.
		std::string directory(directory_size, '\0');
		std::copy(root.begin(), root.end(), directory.begin());
		std::copy_n("/domains", sizeof("/domains") - 1, directory.begin() + root.size());
		const std::string filename(catalog_filename);
		std::vector<uint8_t> bytes;
		const auto read = flatfile_read_bounded(directory, filename, catalog_maximum_bytes,
							&bytes, reserve_scratch_peak, context,
							live);
		if (read == flatfile_read_result::not_found)
			return flatfile_shopkeeper_result::not_found;
		if (read == flatfile_read_result::io_error)
			return flatfile_shopkeeper_result::io_error;
		flatfile_shopkeeper_catalog_allocation_profile profile;
		if (read != flatfile_read_result::ok)
			return flatfile_shopkeeper_result::invalid;
		if (!shop_catalog_preflight_resource(bytes.data(), bytes.size(), &profile,
						     resource))
			return resource.failure();
		if (!profile.fresh_decode_storage_policy_supported)
		{
			errno = ENOTSUP;
			return flatfile_shopkeeper_result::io_error;
		}
		size_t validation = profile.largest_item_roundtrip_scratch_bytes;
		if (profile.item_count > SIZE_MAX / sizeof(uint64_t) ||
		    !add(validation, profile.item_count * sizeof(uint64_t)))
		{
			errno = ENOBUFS;
			return flatfile_shopkeeper_result::io_error;
		}
		const size_t scratch =
			std::max(validation, profile.largest_item_decode_scratch_bytes);
		if (!add(live, bytes.capacity()) ||
		    !add(live, profile.decoded_catalog_payload_bytes) || !add(live, scratch) ||
		    !reserve_scratch_peak(live, context))
		{
			errno = ENOBUFS;
			return flatfile_shopkeeper_result::io_error;
		}
		shopkeeper_catalog catalog;
		if (!shop_decode_catalog_resource(bytes, &catalog, profile.item_count, resource))
			return resource.failure();
		if (!lock.matches(root))
		{
			errno = EBADMSG;
			return flatfile_shopkeeper_result::invalid;
		}
		records->swap(catalog.records);
		return flatfile_shopkeeper_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return flatfile_shopkeeper_result::io_error;
	}
	catch (...)
	{
		errno = EOVERFLOW;
		return flatfile_shopkeeper_result::io_error;
	}
#endif
}
