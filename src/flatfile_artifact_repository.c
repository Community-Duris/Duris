#include "flatfile_artifact_repository.h"

#include "flatfile_store.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>

namespace
{
constexpr std::array<uint8_t, 8> catalog_magic = { 'D', 'U', 'R', 'A', 'R', 'T', 'F', 0 };
constexpr uint32_t catalog_version = 1;
constexpr size_t catalog_maximum_bytes = 64 * 1024 * 1024;
constexpr size_t record_maximum = 1048576;
constexpr const char *catalog_filename = "artifact_catalog";

struct artifact_catalog
{
	uint64_t revision = 1;
	std::vector<flatfile_artifact_record> records;
};

struct encoder
{
	std::vector<uint8_t> bytes;
	bool valid = true;

	template <typename T> void number(T value)
	{
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
		if (!valid || (!data && size))
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
		if (!value || size - offset < sizeof(T))
			return false;
		using U = std::make_unsigned_t<T>;
		U bits = 0;
		for (size_t index = 0; index < sizeof(T); ++index)
			bits |= static_cast<U>(data[offset++]) << (index * 8);
		*value = static_cast<T>(bits);
		return true;
	}
};

std::string domains_directory(const std::string &root)
{
	return root + "/domains";
}

bool record_less(const flatfile_artifact_record &left, const flatfile_artifact_record &right)
{
	return left.vnum < right.vnum;
}

bool valid_record(const flatfile_artifact_record &record)
{
	return record.vnum > 0 && record.location_type >= FLATFILE_ARTIFACT_NOT_IN_GAME &&
	       record.location_type <= FLATFILE_ARTIFACT_ON_CORPSE && record.timer >= 0 &&
	       record.type >= 1 && record.type <= 3 && record.last_update >= 0 &&
	       record.bind_owner_pid >= -1 && record.bind_timer >= 0 && record.revision;
}

bool valid_records(const std::vector<flatfile_artifact_record> &records)
{
	if (records.size() > record_maximum ||
	    !std::is_sorted(records.begin(), records.end(), record_less))
		return false;
	for (size_t index = 0; index < records.size(); ++index)
		if (!valid_record(records[index]) ||
		    (index && records[index - 1].vnum == records[index].vnum))
			return false;
	return true;
}

bool encode_catalog(const artifact_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !catalog.revision || !valid_records(catalog.records))
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.records.size());
	for (const auto &record : catalog.records)
	{
		payload.number(record.vnum);
		payload.number<uint8_t>(record.owned ? 1 : 0);
		payload.number(record.location_type);
		payload.number(record.location);
		payload.number(record.timer);
		payload.number(record.type);
		payload.number(record.last_update);
		payload.number(record.bind_owner_pid);
		payload.number(record.bind_timer);
		payload.number(record.revision);
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

bool decode_catalog(const std::vector<uint8_t> &bytes, artifact_catalog *catalog)
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
	artifact_catalog decoded;
	decoded.revision = revision;
	uint32_t count = 0;
	if (!payload.number(&count) || count > record_maximum)
		return false;
	try
	{
		decoded.records.resize(count);
		for (auto &record : decoded.records)
		{
			uint8_t owned = 0;
			if (!payload.number(&record.vnum) || !payload.number(&owned) || owned > 1 ||
			    !payload.number(&record.location_type) ||
			    !payload.number(&record.location) || !payload.number(&record.timer) ||
			    !payload.number(&record.type) || !payload.number(&record.last_update) ||
			    !payload.number(&record.bind_owner_pid) ||
			    !payload.number(&record.bind_timer) ||
			    !payload.number(&record.revision))
				return false;
			record.owned = owned != 0;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (payload.offset != payload.size || !valid_records(decoded.records))
		return false;
	*catalog = std::move(decoded);
	return true;
}

flatfile_artifact_result recover(const std::string &root, const flatfile_authority_lock &lock,
				 std::string *error)
{
	const auto result = flatfile_authority_transaction_recover(root, lock, error);
	if (result == flatfile_authority_transaction_result::ok)
		return flatfile_artifact_result::ok;
	return result == flatfile_authority_transaction_result::io_error ?
		       flatfile_artifact_result::io_error :
		       flatfile_artifact_result::invalid;
}

flatfile_artifact_result load_catalog(const std::string &root, artifact_catalog *catalog,
				      std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(domains_directory(root), catalog_filename,
					  catalog_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_artifact_result::not_found;
	if (loaded == flatfile_read_result::io_error)
		return flatfile_artifact_result::io_error;
	if (loaded != flatfile_read_result::ok || !decode_catalog(bytes, catalog))
	{
		if (error && error->empty())
			*error = "artifact catalog is corrupt";
		return flatfile_artifact_result::invalid;
	}
	return flatfile_artifact_result::ok;
}
} // namespace

flatfile_artifact_result
flatfile_artifact_establish(const std::string &root,
			    const std::vector<flatfile_artifact_record> &records,
			    std::string *error)
{
	if (root.empty())
		return flatfile_artifact_result::invalid;
	artifact_catalog candidate;
	try
	{
		candidate.records = records;
		std::sort(candidate.records.begin(), candidate.records.end(), record_less);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_artifact_result::io_error;
	}
	if (!valid_records(candidate.records))
		return flatfile_artifact_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_artifact_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_artifact_result::ok)
		return recovered;
	artifact_catalog existing;
	const auto loaded = load_catalog(root, &existing, error);
	if (loaded == flatfile_artifact_result::ok)
		return existing.records == candidate.records ?
			       flatfile_artifact_result::already_exists :
			       flatfile_artifact_result::invalid;
	if (loaded != flatfile_artifact_result::not_found)
		return loaded;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(candidate, &bytes))
		return flatfile_artifact_result::invalid;
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_artifact_result::ok :
		       flatfile_artifact_result::io_error;
}

flatfile_artifact_result flatfile_artifact_list(const std::string &root,
						std::vector<flatfile_artifact_record> *records,
						std::string *error)
{
	if (root.empty() || !records)
		return flatfile_artifact_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_artifact_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_artifact_result::ok)
		return recovered;
	artifact_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_artifact_result::ok)
		return loaded;
	try
	{
		*records = catalog.records;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_artifact_result::io_error;
	}
	return flatfile_artifact_result::ok;
}

flatfile_artifact_result flatfile_artifact_prepare_player_release(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	flatfile_authority_operation *operation, std::string *error)
{
	if (root.empty() || !pid || !operation || !lock.matches(root))
		return flatfile_artifact_result::invalid;
	*operation = {};
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_artifact_result::ok)
		return recovered;
	artifact_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_artifact_result::ok)
		return loaded;
	for (const auto &record : catalog.records)
		if (record.location_type == FLATFILE_ARTIFACT_ON_CORPSE &&
		    record.location == static_cast<int32_t>(pid))
			return flatfile_artifact_result::conflict;
	bool changed = false;
	for (auto &record : catalog.records)
	{
		const bool held = record.location_type == FLATFILE_ARTIFACT_ON_PLAYER &&
				  record.location == static_cast<int32_t>(pid);
		const bool bound = record.bind_owner_pid == static_cast<int32_t>(pid);
		if (!held && !bound)
			continue;
		if (record.revision == std::numeric_limits<uint64_t>::max())
			return flatfile_artifact_result::invalid;
		if (held)
		{
			record.owned = false;
			record.location_type = FLATFILE_ARTIFACT_NOT_IN_GAME;
			record.location = 0;
			record.timer = 0;
		}
		if (bound)
		{
			record.bind_owner_pid = -1;
			record.bind_timer = 0;
		}
		++record.revision;
		changed = true;
	}
	if (!changed)
		return flatfile_artifact_result::unchanged;
	if (catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_artifact_result::invalid;
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_artifact_result::invalid;
	operation->store = flatfile_authority_store::domains;
	operation->kind = flatfile_authority_operation_kind::write;
	operation->filename = catalog_filename;
	operation->bytes = std::move(bytes);
	return flatfile_artifact_result::ok;
}
