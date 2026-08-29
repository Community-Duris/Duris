#include "flatfile_shopkeeper_repository.h"

#include "flatfile_authority_transaction.h"
#include "flatfile_store.h"
#include "player_snapshot_codec.h"

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
constexpr uint32_t catalog_version = 1;
constexpr size_t catalog_maximum_bytes = 256 * 1024 * 1024;
constexpr size_t shopkeeper_maximum = 262144;
constexpr size_t affect_maximum = 4096;
constexpr int16_t equipment_slot_maximum = 255;
constexpr const char *catalog_filename = "shopkeeper_catalog";

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
	    !header.number(&revision) || version != catalog_version || !revision ||
	    payload_size != bytes.size() - header_size)
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
			if (!payload.number(&record.shop_id) || !payload.number(&record.mob_vnum) ||
			    !payload.number(&record.room_vnum) ||
			    !payload.number(&record.saved_at) ||
			    !payload.number(&record.revision) || !payload.number(&affect_count) ||
			    affect_count > affect_maximum)
				return false;
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
} // namespace

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
	++record->revision;
	++catalog.revision;
	if (!valid_catalog(catalog))
	{
		*result_code = EINVAL;
		return flatfile_shopkeeper_result::ok;
	}
	mutation->shop_revision = record->revision;
	mutation->after_image.filename = catalog_filename;
	if (!encode_catalog(catalog, &mutation->after_image.bytes))
		return flatfile_shopkeeper_result::io_error;
	return flatfile_shopkeeper_result::ok;
}
