#include "flatfile_locker_repository.h"

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
constexpr std::array<uint8_t, 8> catalog_magic = { 'D', 'U', 'R', 'L', 'O', 'C', 'K', 0 };
constexpr uint32_t catalog_version = 1;
constexpr size_t catalog_maximum_bytes = 128 * 1024 * 1024;
constexpr size_t locker_maximum = 65536;
constexpr size_t chest_maximum = 262144;
constexpr size_t access_maximum = 1048576;
constexpr size_t locker_name_maximum = 100;
constexpr size_t chest_name_maximum = 32;
constexpr size_t password_hash_maximum = 64;
constexpr size_t access_name_maximum = 255;
constexpr size_t sort_config_maximum = 4096;
constexpr const char *catalog_filename = "locker_catalog";

struct locker_catalog
{
	uint64_t revision = 1;
	std::vector<flatfile_locker_record> lockers;
	std::vector<flatfile_locker_access_record> access;
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

	void string(const std::string &value, size_t maximum)
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

	bool string(std::string *value, size_t maximum)
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

	bool bytes(std::vector<uint8_t> *value, size_t maximum)
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

bool valid_name(const std::string &name, size_t maximum)
{
	if (name.empty() || name.size() > maximum)
		return false;
	for (unsigned char character : name)
		if (character < 0x21 || character > 0x7e)
			return false;
	return true;
}

bool locker_less(const flatfile_locker_record &left, const flatfile_locker_record &right)
{
	return left.locker_id < right.locker_id;
}

bool chest_less(const flatfile_locker_chest_record &left, const flatfile_locker_chest_record &right)
{
	return left.chest_id < right.chest_id;
}

bool access_less(const flatfile_locker_access_record &left,
		 const flatfile_locker_access_record &right)
{
	if (left.owner_name != right.owner_name)
		return left.owner_name < right.owner_name;
	return left.visitor_name < right.visitor_name;
}

bool valid_catalog(const locker_catalog &catalog)
{
	if (!catalog.revision || catalog.lockers.size() > locker_maximum ||
	    catalog.access.size() > access_maximum ||
	    !std::is_sorted(catalog.lockers.begin(), catalog.lockers.end(), locker_less) ||
	    !std::is_sorted(catalog.access.begin(), catalog.access.end(), access_less))
		return false;
	std::unordered_set<std::string> locker_names;
	std::unordered_set<uint32_t> chest_ids;
	std::unordered_set<uint64_t> item_uids;
	size_t chest_count = 0;
	try
	{
		locker_names.reserve(catalog.lockers.size());
		chest_ids.reserve(std::min(chest_maximum, catalog.lockers.size() * 2));
		for (size_t locker_index = 0; locker_index < catalog.lockers.size(); ++locker_index)
		{
			const auto &locker = catalog.lockers[locker_index];
			if (!locker.locker_id || !locker.revision ||
			    !valid_name(locker.locker_name, locker_name_maximum) ||
			    locker.locker_name != canonical_name(locker.locker_name) ||
			    locker.locker_name.rfind("account.", 0) == 0 ||
			    (locker.owner_pid > 0) == (locker.owner_assoc_id > 0) ||
			    locker.owner_pid < 0 || locker.owner_assoc_id < 0 ||
			    (locker_index &&
			     catalog.lockers[locker_index - 1].locker_id == locker.locker_id) ||
			    !locker_names.insert(locker.locker_name).second ||
			    !std::is_sorted(locker.chests.begin(), locker.chests.end(), chest_less))
				return false;
			if (locker.chests.empty() ||
			    locker.chests.size() > chest_maximum - chest_count)
				return false;
			chest_count += locker.chests.size();
			size_t public_count = 0;
			std::unordered_set<std::string> chest_names;
			chest_names.reserve(locker.chests.size());
			for (size_t chest_index = 0; chest_index < locker.chests.size();
			     ++chest_index)
			{
				const auto &chest = locker.chests[chest_index];
				if (!chest.chest_id || !chest.revision ||
				    !valid_name(chest.chest_name, chest_name_maximum) ||
				    chest.chest_name != canonical_name(chest.chest_name) ||
				    chest.password_hash.size() > password_hash_maximum ||
				    chest.sort_config.size() > sort_config_maximum ||
				    (chest.is_public && !chest.password_hash.empty()) ||
				    (chest_index &&
				     locker.chests[chest_index - 1].chest_id == chest.chest_id) ||
				    !chest_ids.insert(chest.chest_id).second ||
				    !chest_names.insert(chest.chest_name).second)
					return false;
				public_count += chest.is_public ? 1 : 0;
				std::vector<uint8_t> encoded_items;
				if (player_item_snapshot_list_encode(chest.items, &encoded_items) !=
				    player_snapshot_codec_result::ok)
					return false;
				for (const auto &item : chest.items)
					if (!item.object_uid || item.vnum <= 0 ||
					    item.equipment_slot != -1 ||
					    !item_uids.insert(item.object_uid).second)
						return false;
			}
			if (public_count != 1)
				return false;
		}
		for (size_t index = 0; index < catalog.access.size(); ++index)
		{
			const auto &entry = catalog.access[index];
			if (!entry.revision || !valid_name(entry.owner_name, access_name_maximum) ||
			    !valid_name(entry.visitor_name, access_name_maximum) ||
			    entry.owner_name != canonical_name(entry.owner_name) ||
			    entry.visitor_name != canonical_name(entry.visitor_name) ||
			    !locker_names.count(entry.owner_name) ||
			    (index && entry.owner_name == catalog.access[index - 1].owner_name &&
			     entry.visitor_name == catalog.access[index - 1].visitor_name))
				return false;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

bool encode_catalog(const locker_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !valid_catalog(catalog))
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.lockers.size());
	payload.number<uint32_t>(catalog.access.size());
	for (const auto &locker : catalog.lockers)
	{
		payload.number(locker.locker_id);
		payload.string(locker.locker_name, locker_name_maximum);
		payload.number(locker.owner_pid);
		payload.number(locker.owner_assoc_id);
		payload.number(locker.racewar);
		payload.number(locker.race);
		payload.number(locker.revision);
		payload.number<uint32_t>(locker.chests.size());
		for (const auto &chest : locker.chests)
		{
			std::vector<uint8_t> encoded_items;
			if (player_item_snapshot_list_encode(chest.items, &encoded_items) !=
			    player_snapshot_codec_result::ok)
				return false;
			payload.number(chest.chest_id);
			payload.string(chest.chest_name, chest_name_maximum);
			payload.string(chest.password_hash, password_hash_maximum);
			payload.number<uint8_t>(chest.is_public ? 1 : 0);
			payload.string(chest.sort_config, sort_config_maximum);
			payload.number(chest.revision);
			payload.number<uint32_t>(encoded_items.size());
			payload.raw(encoded_items.data(), encoded_items.size());
		}
	}
	for (const auto &entry : catalog.access)
	{
		payload.string(entry.owner_name, access_name_maximum);
		payload.string(entry.visitor_name, access_name_maximum);
		payload.number(entry.revision);
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

bool decode_catalog(const std::vector<uint8_t> &bytes, locker_catalog *catalog)
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
	uint32_t locker_count = 0, access_count = 0;
	if (!payload.number(&locker_count) || !payload.number(&access_count) ||
	    locker_count > locker_maximum || access_count > access_maximum)
		return false;
	locker_catalog decoded;
	decoded.revision = revision;
	size_t chest_count = 0;
	try
	{
		decoded.lockers.resize(locker_count);
		decoded.access.resize(access_count);
		for (auto &locker : decoded.lockers)
		{
			uint32_t count = 0;
			if (!payload.number(&locker.locker_id) ||
			    !payload.string(&locker.locker_name, locker_name_maximum) ||
			    !payload.number(&locker.owner_pid) ||
			    !payload.number(&locker.owner_assoc_id) ||
			    !payload.number(&locker.racewar) || !payload.number(&locker.race) ||
			    !payload.number(&locker.revision) || !payload.number(&count) ||
			    count > chest_maximum - chest_count)
				return false;
			chest_count += count;
			locker.chests.resize(count);
			for (auto &chest : locker.chests)
			{
				uint8_t is_public = 0;
				std::vector<uint8_t> encoded_items;
				if (!payload.number(&chest.chest_id) ||
				    !payload.string(&chest.chest_name, chest_name_maximum) ||
				    !payload.string(&chest.password_hash, password_hash_maximum) ||
				    !payload.number(&is_public) || is_public > 1 ||
				    !payload.string(&chest.sort_config, sort_config_maximum) ||
				    !payload.number(&chest.revision) ||
				    !payload.bytes(&encoded_items, PLAYER_SNAPSHOT_MAX_BYTES) ||
				    player_item_snapshot_list_decode(
					    encoded_items.data(), encoded_items.size(),
					    &chest.items) != player_snapshot_codec_result::ok)
					return false;
				chest.is_public = is_public != 0;
			}
		}
		for (auto &entry : decoded.access)
			if (!payload.string(&entry.owner_name, access_name_maximum) ||
			    !payload.string(&entry.visitor_name, access_name_maximum) ||
			    !payload.number(&entry.revision))
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

flatfile_locker_result recover(const std::string &root, const flatfile_authority_lock &lock,
			       std::string *error)
{
	const auto result = flatfile_authority_transaction_recover(root, lock, error);
	if (result == flatfile_authority_transaction_result::ok)
		return flatfile_locker_result::ok;
	return result == flatfile_authority_transaction_result::io_error ?
		       flatfile_locker_result::io_error :
		       flatfile_locker_result::invalid;
}

flatfile_locker_result load_catalog(const std::string &root, locker_catalog *catalog,
				    std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(domains_directory(root), catalog_filename,
					  catalog_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_locker_result::not_found;
	if (loaded == flatfile_read_result::io_error)
		return flatfile_locker_result::io_error;
	if (loaded != flatfile_read_result::ok || !decode_catalog(bytes, catalog))
	{
		if (error && error->empty())
			*error = "locker catalog is corrupt";
		return flatfile_locker_result::invalid;
	}
	return flatfile_locker_result::ok;
}

bool catalog_equal(locker_catalog left, locker_catalog right)
{
	left.revision = 1;
	right.revision = 1;
	std::vector<uint8_t> left_bytes, right_bytes;
	return encode_catalog(left, &left_bytes) && encode_catalog(right, &right_bytes) &&
	       left_bytes == right_bytes;
}
} // namespace

flatfile_locker_result flatfile_locker_establish(
	const std::string &root, const std::vector<flatfile_locker_record> &lockers,
	const std::vector<flatfile_locker_access_record> &access, std::string *error)
{
	if (root.empty())
		return flatfile_locker_result::invalid;
	locker_catalog candidate;
	try
	{
		candidate.lockers = lockers;
		candidate.access = access;
		for (auto &locker : candidate.lockers)
		{
			locker.locker_name = canonical_name(locker.locker_name);
			std::sort(locker.chests.begin(), locker.chests.end(), chest_less);
			for (auto &chest : locker.chests)
				chest.chest_name = canonical_name(chest.chest_name);
		}
		for (auto &entry : candidate.access)
		{
			entry.owner_name = canonical_name(entry.owner_name);
			entry.visitor_name = canonical_name(entry.visitor_name);
		}
		std::sort(candidate.lockers.begin(), candidate.lockers.end(), locker_less);
		std::sort(candidate.access.begin(), candidate.access.end(), access_less);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_locker_result::io_error;
	}
	if (!valid_catalog(candidate))
		return flatfile_locker_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_locker_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_locker_result::ok)
		return recovered;
	locker_catalog existing;
	const auto loaded = load_catalog(root, &existing, error);
	if (loaded == flatfile_locker_result::ok)
		return catalog_equal(existing, candidate) ? flatfile_locker_result::already_exists :
							    flatfile_locker_result::invalid;
	if (loaded != flatfile_locker_result::not_found)
		return loaded;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(candidate, &encoded))
		return flatfile_locker_result::invalid;
	if (!flatfile_atomic_write(domains_directory(root), catalog_filename, encoded, error))
		return flatfile_locker_result::io_error;
	return flatfile_locker_result::ok;
}

flatfile_locker_result flatfile_locker_list(const std::string &root,
					    std::vector<flatfile_locker_record> *lockers,
					    std::vector<flatfile_locker_access_record> *access,
					    std::string *error)
{
	if (root.empty() || !lockers || !access)
		return flatfile_locker_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_locker_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_locker_result::ok)
		return recovered;
	locker_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_locker_result::ok)
		return loaded;
	*lockers = std::move(catalog.lockers);
	*access = std::move(catalog.access);
	return flatfile_locker_result::ok;
}
