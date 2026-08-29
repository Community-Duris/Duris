#include "flatfile_frag_leaderboard_repository.h"

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
constexpr std::array<uint8_t, 8> catalog_magic = { 'D', 'U', 'R', 'F', 'R', 'A', 'G', 0 };
constexpr uint32_t catalog_version = 1;
constexpr size_t catalog_maximum_bytes = 256 * 1024 * 1024;
constexpr size_t record_maximum = 1048576;
constexpr size_t account_maximum = 255;
constexpr size_t character_maximum = 255;
constexpr size_t category_maximum = 50;
constexpr const char *catalog_filename = "frag_leaderboard";

struct frag_catalog
{
	uint64_t revision = 1;
	std::vector<flatfile_frag_leaderboard_record> records;
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

	void text(const std::string &value)
	{
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
		if (!value || size - offset < sizeof(T))
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
		if (!value || !number(&length) || length > maximum || size - offset < length)
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
		return value->find('\0') == std::string::npos;
	}
};

std::string domains_directory(const std::string &root)
{
	return root + "/domains";
}

bool record_less(const flatfile_frag_leaderboard_record &left,
		 const flatfile_frag_leaderboard_record &right)
{
	return left.pid < right.pid;
}

bool valid_text(const std::string &value, size_t maximum, bool required)
{
	return (!required || !value.empty()) && value.size() <= maximum &&
	       value.find('\0') == std::string::npos;
}

bool valid_record(const flatfile_frag_leaderboard_record &record)
{
	return record.pid && valid_text(record.account_name, account_maximum, true) &&
	       valid_text(record.character_name, character_maximum, true) &&
	       valid_text(record.race_name, category_maximum, false) &&
	       valid_text(record.class_name, category_maximum, false) && record.deleted_at >= 0 &&
	       record.last_updated >= 0 && record.revision;
}

bool valid_records(const std::vector<flatfile_frag_leaderboard_record> &records)
{
	if (records.size() > record_maximum ||
	    !std::is_sorted(records.begin(), records.end(), record_less))
		return false;
	for (size_t index = 0; index < records.size(); ++index)
		if (!valid_record(records[index]) ||
		    (index && records[index - 1].pid == records[index].pid))
			return false;
	return true;
}

bool encode_catalog(const frag_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !catalog.revision || !valid_records(catalog.records))
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.records.size());
	for (const auto &record : catalog.records)
	{
		payload.number(record.pid);
		payload.text(record.account_name);
		payload.text(record.character_name);
		payload.number(record.total_frags);
		payload.number(record.racewar);
		payload.text(record.race_name);
		payload.text(record.class_name);
		payload.number(record.level);
		payload.number(record.deleted_at);
		payload.number(record.last_updated);
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

bool decode_catalog(const std::vector<uint8_t> &bytes, frag_catalog *catalog)
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
	frag_catalog decoded;
	decoded.revision = revision;
	uint32_t count = 0;
	if (!payload.number(&count) || count > record_maximum)
		return false;
	try
	{
		decoded.records.resize(count);
		for (auto &record : decoded.records)
			if (!payload.number(&record.pid) ||
			    !payload.text(&record.account_name, account_maximum) ||
			    !payload.text(&record.character_name, character_maximum) ||
			    !payload.number(&record.total_frags) ||
			    !payload.number(&record.racewar) ||
			    !payload.text(&record.race_name, category_maximum) ||
			    !payload.text(&record.class_name, category_maximum) ||
			    !payload.number(&record.level) || !payload.number(&record.deleted_at) ||
			    !payload.number(&record.last_updated) ||
			    !payload.number(&record.revision))
				return false;
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

flatfile_frag_leaderboard_result recover(const std::string &root,
					 const flatfile_authority_lock &lock, std::string *error)
{
	const auto result = flatfile_authority_transaction_recover(root, lock, error);
	if (result == flatfile_authority_transaction_result::ok)
		return flatfile_frag_leaderboard_result::ok;
	return result == flatfile_authority_transaction_result::io_error ?
		       flatfile_frag_leaderboard_result::io_error :
		       flatfile_frag_leaderboard_result::invalid;
}

flatfile_frag_leaderboard_result load_catalog(const std::string &root, frag_catalog *catalog,
					      std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(domains_directory(root), catalog_filename,
					  catalog_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_frag_leaderboard_result::not_found;
	if (loaded == flatfile_read_result::io_error)
		return flatfile_frag_leaderboard_result::io_error;
	if (loaded != flatfile_read_result::ok || !decode_catalog(bytes, catalog))
	{
		if (error && error->empty())
			*error = "frag leaderboard is corrupt";
		return flatfile_frag_leaderboard_result::invalid;
	}
	return flatfile_frag_leaderboard_result::ok;
}

flatfile_frag_leaderboard_record *find_record(frag_catalog *catalog, uint32_t pid)
{
	auto found = std::lower_bound(catalog->records.begin(), catalog->records.end(), pid,
				      [](const auto &record, uint32_t candidate)
				      { return record.pid < candidate; });
	return found != catalog->records.end() && found->pid == pid ? &*found : nullptr;
}

flatfile_frag_leaderboard_result publish(const std::string &root, frag_catalog *catalog,
					 std::string *error)
{
	if (!catalog || catalog->revision == std::numeric_limits<uint64_t>::max())
		return flatfile_frag_leaderboard_result::invalid;
	++catalog->revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(*catalog, &bytes))
		return flatfile_frag_leaderboard_result::invalid;
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_frag_leaderboard_result::ok :
		       flatfile_frag_leaderboard_result::io_error;
}
} // namespace

flatfile_frag_leaderboard_result
flatfile_frag_leaderboard_establish(const std::string &root,
				    const std::vector<flatfile_frag_leaderboard_record> &records,
				    std::string *error)
{
	if (root.empty())
		return flatfile_frag_leaderboard_result::invalid;
	frag_catalog candidate;
	try
	{
		candidate.records = records;
		std::sort(candidate.records.begin(), candidate.records.end(), record_less);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_frag_leaderboard_result::io_error;
	}
	if (!valid_records(candidate.records))
		return flatfile_frag_leaderboard_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_frag_leaderboard_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_frag_leaderboard_result::ok)
		return recovered;
	frag_catalog existing;
	const auto loaded = load_catalog(root, &existing, error);
	if (loaded == flatfile_frag_leaderboard_result::ok)
		return existing.records == candidate.records ?
			       flatfile_frag_leaderboard_result::already_exists :
			       flatfile_frag_leaderboard_result::invalid;
	if (loaded != flatfile_frag_leaderboard_result::not_found)
		return loaded;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(candidate, &bytes))
		return flatfile_frag_leaderboard_result::invalid;
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_frag_leaderboard_result::ok :
		       flatfile_frag_leaderboard_result::io_error;
}

flatfile_frag_leaderboard_result
flatfile_frag_leaderboard_list(const std::string &root,
			       std::vector<flatfile_frag_leaderboard_record> *records,
			       std::string *error)
{
	if (root.empty() || !records)
		return flatfile_frag_leaderboard_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_frag_leaderboard_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_frag_leaderboard_result::ok)
		return recovered;
	frag_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_frag_leaderboard_result::ok)
		return loaded;
	try
	{
		*records = catalog.records;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_frag_leaderboard_result::io_error;
	}
	return flatfile_frag_leaderboard_result::ok;
}

flatfile_frag_leaderboard_result
flatfile_frag_leaderboard_upsert(const std::string &root,
				 const flatfile_frag_leaderboard_record &record, std::string *error)
{
	if (root.empty() || !valid_record(record) || record.deleted_at)
		return flatfile_frag_leaderboard_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_frag_leaderboard_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_frag_leaderboard_result::ok)
		return recovered;
	frag_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_frag_leaderboard_result::ok)
		return loaded;
	flatfile_frag_leaderboard_record *stored = find_record(&catalog, record.pid);
	try
	{
		if (!stored)
		{
			if (catalog.records.size() >= record_maximum)
				return flatfile_frag_leaderboard_result::invalid;
			catalog.records.push_back(record);
			std::sort(catalog.records.begin(), catalog.records.end(), record_less);
		}
		else
		{
			if (stored->revision == std::numeric_limits<uint64_t>::max())
				return flatfile_frag_leaderboard_result::invalid;
			const uint64_t next_revision = stored->revision + 1;
			*stored = record;
			stored->revision = next_revision;
		}
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_frag_leaderboard_result::io_error;
	}
	return publish(root, &catalog, error);
}

flatfile_frag_leaderboard_result flatfile_frag_leaderboard_prepare_tombstone(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	int64_t deleted_at, flatfile_authority_operation *operation, std::string *error)
{
	if (root.empty() || !pid || deleted_at <= 0 || !operation || !lock.matches(root))
		return flatfile_frag_leaderboard_result::invalid;
	*operation = {};
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_frag_leaderboard_result::ok)
		return recovered;
	frag_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_frag_leaderboard_result::ok)
		return loaded;
	auto *record = find_record(&catalog, pid);
	if (!record || record->deleted_at)
		return flatfile_frag_leaderboard_result::unchanged;
	if (record->revision == std::numeric_limits<uint64_t>::max() ||
	    catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_frag_leaderboard_result::invalid;
	record->deleted_at = deleted_at;
	record->last_updated = deleted_at;
	++record->revision;
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_frag_leaderboard_result::invalid;
	operation->store = flatfile_authority_store::domains;
	operation->kind = flatfile_authority_operation_kind::write;
	operation->filename = catalog_filename;
	operation->bytes = std::move(bytes);
	return flatfile_frag_leaderboard_result::ok;
}
