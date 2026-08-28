#include "flatfile_world_item_repository.h"

#include "flatfile_authority_transaction.h"
#include "flatfile_store.h"
#include "player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>
#include <unordered_set>

namespace
{
constexpr std::array<uint8_t, 8> catalog_magic = { 'D', 'U', 'R', 'W', 'R', 'L', 'D', 0 };
constexpr uint32_t catalog_version = 1;
constexpr size_t catalog_maximum_bytes = 256 * 1024 * 1024;
constexpr size_t corpse_maximum = 262144;
constexpr size_t saved_item_maximum = 262144;
constexpr size_t name_maximum = 255;
constexpr size_t key_maximum = 100;
constexpr size_t short_description_maximum = 512;
constexpr size_t description_maximum = 64 * 1024;
constexpr size_t keywords_maximum = 512;
constexpr const char *catalog_filename = "world_item_catalog";

struct world_item_catalog
{
	uint64_t revision = 1;
	std::vector<flatfile_corpse_record> corpses;
	std::vector<flatfile_saved_world_item_record> saved_items;
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
		if (!size)
			return;
		try
		{
			bytes.insert(bytes.end(), data, data + size);
		}
		catch (const std::bad_alloc &)
		{
			valid = false;
		}
	}

	void text(const std::string &value, size_t maximum)
	{
		if (value.size() > maximum || value.size() > UINT32_MAX)
		{
			valid = false;
			return;
		}
		number<uint32_t>(value.size());
		raw(reinterpret_cast<const uint8_t *>(value.data()), value.size());
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

	bool text(std::string *value, size_t maximum)
	{
		uint32_t length = 0;
		if (!value || !number(&length) || length > maximum || offset > size ||
		    size - offset < length)
			return false;
		try
		{
			value->assign(reinterpret_cast<const char *>(data + offset), length);
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
		offset += length;
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

std::string canonical_name(const std::string &name)
{
	std::string canonical = name;
	for (char &character : canonical)
		character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
	return canonical;
}

bool valid_printable(const std::string &value, size_t maximum, bool required)
{
	if ((required && value.empty()) || value.size() > maximum)
		return false;
	for (unsigned char character : value)
		if (character < 0x20 || character == 0x7f)
			return false;
	return true;
}

bool corpse_less(const flatfile_corpse_record &left, const flatfile_corpse_record &right)
{
	if (left.owner_pid != right.owner_pid)
		return left.owner_pid < right.owner_pid;
	return left.save_id < right.save_id;
}

bool saved_item_less(const flatfile_saved_world_item_record &left,
		     const flatfile_saved_world_item_record &right)
{
	return left.item_key < right.item_key;
}

bool valid_item_list(const std::vector<player_item_snapshot> &items, bool require_one_root,
		     std::unordered_set<uint64_t> *item_uids)
{
	std::vector<uint8_t> encoded;
	if (!item_uids ||
	    player_item_snapshot_list_encode(items, &encoded) != player_snapshot_codec_result::ok)
		return false;
	size_t roots = 0;
	for (const auto &item : items)
	{
		roots += item.parent_index == PLAYER_SNAPSHOT_NO_PARENT ? 1 : 0;
		if (!item.object_uid || item.vnum <= 0 || item.equipment_slot != -1 ||
		    !item_uids->insert(item.object_uid).second)
			return false;
	}
	return !require_one_root || roots == 1;
}

bool valid_catalog(const world_item_catalog &catalog)
{
	if (!catalog.revision || catalog.corpses.size() > corpse_maximum ||
	    catalog.saved_items.size() > saved_item_maximum ||
	    !std::is_sorted(catalog.corpses.begin(), catalog.corpses.end(), corpse_less) ||
	    !std::is_sorted(catalog.saved_items.begin(), catalog.saved_items.end(),
			    saved_item_less))
		return false;
	std::unordered_set<std::string> owner_names;
	std::unordered_set<std::string> item_keys;
	std::unordered_set<uint64_t> item_uids;
	try
	{
		owner_names.reserve(catalog.corpses.size());
		item_keys.reserve(catalog.saved_items.size());
		for (size_t index = 0; index < catalog.corpses.size(); ++index)
		{
			const auto &corpse = catalog.corpses[index];
			const bool same_owner = index && catalog.corpses[index - 1].owner_pid ==
								 corpse.owner_pid;
			if (!corpse.owner_pid || !corpse.save_id || !corpse.revision ||
			    corpse.room_vnum < 0 ||
			    !valid_printable(corpse.owner_name, name_maximum, true) ||
			    corpse.owner_name != canonical_name(corpse.owner_name) ||
			    !valid_printable(corpse.short_description, short_description_maximum,
					     false) ||
			    !valid_printable(corpse.description, description_maximum, false) ||
			    !valid_printable(corpse.keywords, keywords_maximum, false) ||
			    (index && !corpse_less(catalog.corpses[index - 1], corpse)) ||
			    (same_owner &&
			     catalog.corpses[index - 1].owner_name != corpse.owner_name) ||
			    (!same_owner && !owner_names.insert(corpse.owner_name).second) ||
			    !valid_item_list(corpse.items, false, &item_uids))
				return false;
		}
		for (size_t index = 0; index < catalog.saved_items.size(); ++index)
		{
			const auto &saved = catalog.saved_items[index];
			if (!saved.revision || saved.room_vnum <= 0 ||
			    !valid_printable(saved.item_key, key_maximum, true) ||
			    saved.items.empty() ||
			    (index && !saved_item_less(catalog.saved_items[index - 1], saved)) ||
			    !item_keys.insert(saved.item_key).second ||
			    !valid_item_list(saved.items, true, &item_uids))
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

bool encode_catalog(const world_item_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !valid_catalog(catalog))
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.corpses.size());
	payload.number<uint32_t>(catalog.saved_items.size());
	for (const auto &corpse : catalog.corpses)
	{
		payload.number(corpse.owner_pid);
		payload.text(corpse.owner_name, name_maximum);
		payload.number(corpse.save_id);
		payload.number(corpse.room_vnum);
		payload.text(corpse.short_description, short_description_maximum);
		payload.text(corpse.description, description_maximum);
		payload.text(corpse.keywords, keywords_maximum);
		payload.number(corpse.weight);
		for (int32_t value : corpse.values)
			payload.number(value);
		payload.number(corpse.revision);
		if (!encode_items(payload, corpse.items))
			return false;
	}
	for (const auto &saved : catalog.saved_items)
	{
		payload.text(saved.item_key, key_maximum);
		payload.number(saved.room_vnum);
		payload.number(saved.revision);
		if (!encode_items(payload, saved.items))
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

bool decode_catalog(const std::vector<uint8_t> &bytes, world_item_catalog *catalog)
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
	uint32_t corpse_count = 0, saved_count = 0;
	if (!payload.number(&corpse_count) || !payload.number(&saved_count) ||
	    corpse_count > corpse_maximum || saved_count > saved_item_maximum)
		return false;
	world_item_catalog decoded;
	decoded.revision = revision;
	try
	{
		decoded.corpses.resize(corpse_count);
		for (auto &corpse : decoded.corpses)
		{
			if (!payload.number(&corpse.owner_pid) ||
			    !payload.text(&corpse.owner_name, name_maximum) ||
			    !payload.number(&corpse.save_id) ||
			    !payload.number(&corpse.room_vnum) ||
			    !payload.text(&corpse.short_description, short_description_maximum) ||
			    !payload.text(&corpse.description, description_maximum) ||
			    !payload.text(&corpse.keywords, keywords_maximum) ||
			    !payload.number(&corpse.weight))
				return false;
			for (int32_t &value : corpse.values)
				if (!payload.number(&value))
					return false;
			if (!payload.number(&corpse.revision) ||
			    !decode_items(payload, &corpse.items))
				return false;
		}
		decoded.saved_items.resize(saved_count);
		for (auto &saved : decoded.saved_items)
			if (!payload.text(&saved.item_key, key_maximum) ||
			    !payload.number(&saved.room_vnum) || !payload.number(&saved.revision) ||
			    !decode_items(payload, &saved.items))
				return false;
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

flatfile_world_item_result recover(const std::string &root, const flatfile_authority_lock &lock,
				   std::string *error)
{
	const auto result = flatfile_authority_transaction_recover(root, lock, error);
	if (result == flatfile_authority_transaction_result::ok)
		return flatfile_world_item_result::ok;
	return result == flatfile_authority_transaction_result::io_error ?
		       flatfile_world_item_result::io_error :
		       flatfile_world_item_result::invalid;
}

flatfile_world_item_result load_catalog(const std::string &root, world_item_catalog *catalog,
					std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(domains_directory(root), catalog_filename,
					  catalog_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_world_item_result::not_found;
	if (loaded == flatfile_read_result::io_error)
		return flatfile_world_item_result::io_error;
	if (loaded != flatfile_read_result::ok || !decode_catalog(bytes, catalog))
	{
		if (error && error->empty())
			*error = "world item catalog is corrupt";
		return flatfile_world_item_result::invalid;
	}
	return flatfile_world_item_result::ok;
}

bool catalog_equal(world_item_catalog left, world_item_catalog right)
{
	left.revision = 1;
	right.revision = 1;
	std::vector<uint8_t> left_bytes, right_bytes;
	return encode_catalog(left, &left_bytes) && encode_catalog(right, &right_bytes) &&
	       left_bytes == right_bytes;
}
} // namespace

flatfile_world_item_result flatfile_world_item_establish(
	const std::string &root, const std::vector<flatfile_corpse_record> &corpses,
	const std::vector<flatfile_saved_world_item_record> &saved_items, std::string *error)
{
	if (root.empty())
		return flatfile_world_item_result::invalid;
	world_item_catalog candidate;
	try
	{
		candidate.corpses = corpses;
		candidate.saved_items = saved_items;
		for (auto &corpse : candidate.corpses)
			corpse.owner_name = canonical_name(corpse.owner_name);
		std::sort(candidate.corpses.begin(), candidate.corpses.end(), corpse_less);
		std::sort(candidate.saved_items.begin(), candidate.saved_items.end(),
			  saved_item_less);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_world_item_result::io_error;
	}
	if (!valid_catalog(candidate))
		return flatfile_world_item_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_world_item_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_world_item_result::ok)
		return recovered;
	world_item_catalog existing;
	const auto loaded = load_catalog(root, &existing, error);
	if (loaded == flatfile_world_item_result::ok)
		return catalog_equal(existing, candidate) ?
			       flatfile_world_item_result::already_exists :
			       flatfile_world_item_result::invalid;
	if (loaded != flatfile_world_item_result::not_found)
		return loaded;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(candidate, &encoded))
		return flatfile_world_item_result::invalid;
	if (!flatfile_atomic_write(domains_directory(root), catalog_filename, encoded, error))
		return flatfile_world_item_result::io_error;
	return flatfile_world_item_result::ok;
}

flatfile_world_item_result
flatfile_world_item_list(const std::string &root, std::vector<flatfile_corpse_record> *corpses,
			 std::vector<flatfile_saved_world_item_record> *saved_items,
			 std::string *error)
{
	if (root.empty() || !corpses || !saved_items)
		return flatfile_world_item_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_world_item_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_world_item_result::ok)
		return recovered;
	world_item_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_world_item_result::ok)
		return loaded;
	*corpses = std::move(catalog.corpses);
	*saved_items = std::move(catalog.saved_items);
	return flatfile_world_item_result::ok;
}
