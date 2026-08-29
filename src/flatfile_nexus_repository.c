#include "flatfile_nexus_repository.h"

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
constexpr std::array<uint8_t, 8> nexus_magic = { 'D', 'U', 'R', 'N', 'E', 'X', 'U', 'S' };
constexpr uint32_t nexus_version = 1;
constexpr size_t nexus_file_maximum_bytes = 8 * 1024;
constexpr const char *nexus_filename = "nexus_stones";
constexpr const char *nexus_lock_filename = "nexus_stones.lock";

struct nexus_catalog
{
	uint64_t revision = 0;
	std::vector<flatfile_nexus_record> records;
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

std::string metadata_directory(const std::string &root)
{
	return root + "/metadata";
}

bool valid_record(const flatfile_nexus_record &record)
{
	return record.id > 0 && !record.name.empty() &&
	       record.name.size() <= FLATFILE_NEXUS_NAME_MAX_BYTES &&
	       record.name.find('\0') == std::string::npos &&
	       record.name.find('\n') == std::string::npos &&
	       record.name.find('\r') == std::string::npos && record.room_vnum >= 0 &&
	       record.align >= -3 && record.align <= 3 && record.stat_affect >= -1 &&
	       record.stat_affect <= 255 && record.affect_amount >= -1000000 &&
	       record.affect_amount <= 1000000 && record.last_touched_at >= 0 &&
	       record.last_touched_at <= INT32_MAX && record.bonus >= 0 && record.bonus <= 5;
}

bool valid_records(const std::vector<flatfile_nexus_record> &records)
{
	if (records.size() > FLATFILE_NEXUS_MAX_RECORDS)
		return false;
	for (size_t index = 0; index < records.size(); ++index)
	{
		if (!valid_record(records[index]))
			return false;
		if (index && records[index - 1].id >= records[index].id)
			return false;
	}
	return true;
}

bool encode_catalog(const nexus_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !catalog.revision || !valid_records(catalog.records))
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.records.size());
	for (const auto &record : catalog.records)
	{
		payload.number(record.id);
		payload.number<uint32_t>(record.name.size());
		payload.raw(reinterpret_cast<const uint8_t *>(record.name.data()),
			    record.name.size());
		payload.number(record.room_vnum);
		payload.number(record.align);
		payload.number(record.stat_affect);
		payload.number(record.affect_amount);
		payload.number(record.last_touched_at);
		payload.number(record.bonus);
	}
	if (!payload.valid || payload.bytes.size() > nexus_file_maximum_bytes)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload.bytes.data(), payload.bytes.size(), digest.data());
	encoder file;
	file.raw(nexus_magic.data(), nexus_magic.size());
	file.number(nexus_version);
	file.number<uint32_t>(payload.bytes.size());
	file.number(catalog.revision);
	file.raw(digest.data(), digest.size());
	file.raw(payload.bytes.data(), payload.bytes.size());
	if (!file.valid || file.bytes.size() > nexus_file_maximum_bytes)
		return false;
	*bytes = std::move(file.bytes);
	return true;
}

bool decode_catalog(const std::vector<uint8_t> &bytes, nexus_catalog *catalog)
{
	constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	if (!catalog || bytes.size() < header_size ||
	    memcmp(bytes.data(), nexus_magic.data(), nexus_magic.size()))
		return false;
	decoder header{ bytes.data() + nexus_magic.size(), bytes.size() - nexus_magic.size() };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&revision) || version != nexus_version || !revision ||
	    payload_size != bytes.size() - header_size)
		return false;
	const uint8_t *expected_digest = bytes.data() + 24;
	const uint8_t *payload_bytes = bytes.data() + header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload_bytes, payload_size, digest.data());
	if (CRYPTO_memcmp(expected_digest, digest.data(), digest.size()))
		return false;
	decoder payload{ payload_bytes, payload_size };
	uint32_t count = 0;
	if (!payload.number(&count) || count > FLATFILE_NEXUS_MAX_RECORDS)
		return false;
	nexus_catalog decoded;
	decoded.revision = revision;
	try
	{
		decoded.records.resize(count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	for (auto &record : decoded.records)
	{
		uint32_t name_size = 0;
		if (!payload.number(&record.id) || !payload.number(&name_size) || !name_size ||
		    name_size > FLATFILE_NEXUS_NAME_MAX_BYTES ||
		    payload.size - payload.offset < name_size)
			return false;
		try
		{
			record.name.assign(
				reinterpret_cast<const char *>(payload.data + payload.offset),
				name_size);
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
		payload.offset += name_size;
		if (!payload.number(&record.room_vnum) || !payload.number(&record.align) ||
		    !payload.number(&record.stat_affect) ||
		    !payload.number(&record.affect_amount) ||
		    !payload.number(&record.last_touched_at) || !payload.number(&record.bonus))
			return false;
	}
	if (payload.offset != payload.size || !valid_records(decoded.records))
		return false;
	*catalog = std::move(decoded);
	return true;
}

flatfile_nexus_result load_catalog(const std::string &root, nexus_catalog *catalog,
				   std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto read = flatfile_read(metadata_directory(root), nexus_filename,
					nexus_file_maximum_bytes, &bytes, error);
	if (read == flatfile_read_result::not_found)
		return flatfile_nexus_result::not_found;
	if (read == flatfile_read_result::io_error)
		return flatfile_nexus_result::io_error;
	if (read != flatfile_read_result::ok || !decode_catalog(bytes, catalog))
	{
		if (error && error->empty())
			*error = "nexus stone authority is corrupt";
		return flatfile_nexus_result::invalid;
	}
	return flatfile_nexus_result::ok;
}

flatfile_nexus_result publish(const std::string &root, nexus_catalog *catalog, std::string *error)
{
	if (!catalog || catalog->revision == std::numeric_limits<uint64_t>::max())
		return flatfile_nexus_result::invalid;
	++catalog->revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(*catalog, &bytes))
		return flatfile_nexus_result::invalid;
	return flatfile_atomic_write(metadata_directory(root), nexus_filename, bytes, error) ?
		       flatfile_nexus_result::ok :
		       flatfile_nexus_result::io_error;
}

flatfile_nexus_result acquire(const std::string &root, int *lock_fd, std::string *error)
{
	if (root.empty() || !lock_fd ||
	    !flatfile_lock_acquire(metadata_directory(root), nexus_lock_filename, lock_fd, error))
		return flatfile_nexus_result::io_error;
	return flatfile_nexus_result::ok;
}
} // namespace

flatfile_nexus_result flatfile_nexus_establish(const std::string &root,
					       const std::vector<flatfile_nexus_record> &records,
					       std::string *error)
{
	if (!valid_records(records))
		return flatfile_nexus_result::invalid;
	int lock_fd = -1;
	const auto locked = acquire(root, &lock_fd, error);
	if (locked != flatfile_nexus_result::ok)
		return locked;
	nexus_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded == flatfile_nexus_result::ok)
	{
		flatfile_lock_release(lock_fd);
		return flatfile_nexus_result::unchanged;
	}
	if (loaded != flatfile_nexus_result::not_found)
	{
		flatfile_lock_release(lock_fd);
		return loaded;
	}
	try
	{
		catalog.records = records;
	}
	catch (const std::bad_alloc &)
	{
		flatfile_lock_release(lock_fd);
		return flatfile_nexus_result::io_error;
	}
	const auto result = publish(root, &catalog, error);
	flatfile_lock_release(lock_fd);
	return result;
}

flatfile_nexus_result flatfile_nexus_list(const std::string &root,
					  std::vector<flatfile_nexus_record> *records,
					  std::string *error)
{
	if (!records)
		return flatfile_nexus_result::invalid;
	int lock_fd = -1;
	const auto locked = acquire(root, &lock_fd, error);
	if (locked != flatfile_nexus_result::ok)
		return locked;
	nexus_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded == flatfile_nexus_result::ok)
	{
		try
		{
			*records = catalog.records;
		}
		catch (const std::bad_alloc &)
		{
			flatfile_lock_release(lock_fd);
			return flatfile_nexus_result::io_error;
		}
	}
	flatfile_lock_release(lock_fd);
	return loaded;
}

flatfile_nexus_result flatfile_nexus_find(const std::string &root, int32_t id,
					  flatfile_nexus_record *record, std::string *error)
{
	if (!record || id <= 0)
		return flatfile_nexus_result::invalid;
	std::vector<flatfile_nexus_record> records;
	const auto listed = flatfile_nexus_list(root, &records, error);
	if (listed != flatfile_nexus_result::ok)
		return listed;
	const auto found = std::lower_bound(records.begin(), records.end(), id,
					    [](const auto &candidate, int32_t value)
					    { return candidate.id < value; });
	if (found == records.end() || found->id != id)
		return flatfile_nexus_result::not_found;
	try
	{
		*record = *found;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_nexus_result::io_error;
	}
	return flatfile_nexus_result::ok;
}

flatfile_nexus_result flatfile_nexus_update_state(const std::string &root, int32_t id,
						  int32_t align, int64_t last_touched_at,
						  std::string *error)
{
	if (id <= 0 || align < -3 || align > 3 || last_touched_at < 0 ||
	    last_touched_at > INT32_MAX)
		return flatfile_nexus_result::invalid;
	int lock_fd = -1;
	const auto locked = acquire(root, &lock_fd, error);
	if (locked != flatfile_nexus_result::ok)
		return locked;
	nexus_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_nexus_result::ok)
	{
		flatfile_lock_release(lock_fd);
		return loaded;
	}
	auto found = std::lower_bound(catalog.records.begin(), catalog.records.end(), id,
				      [](const auto &candidate, int32_t value)
				      { return candidate.id < value; });
	if (found == catalog.records.end() || found->id != id)
	{
		flatfile_lock_release(lock_fd);
		return flatfile_nexus_result::not_found;
	}
	if (found->align == align && found->last_touched_at == last_touched_at)
	{
		flatfile_lock_release(lock_fd);
		return flatfile_nexus_result::unchanged;
	}
	found->align = align;
	found->last_touched_at = last_touched_at;
	const auto result = publish(root, &catalog, error);
	flatfile_lock_release(lock_fd);
	return result;
}
