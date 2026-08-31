#include "flatfile/flatfile_account_reward_summon_repository.h"

#include "flatfile/flatfile_store.h"

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
constexpr std::array<uint8_t, 8> catalog_magic = { 'D', 'U', 'R', 'S', 'U', 'M', 'N', 0 };
constexpr uint32_t catalog_version = 1;
constexpr size_t record_maximum = 1048576;
constexpr size_t catalog_maximum_bytes = 64 * 1024 * 1024;
constexpr const char *catalog_filename = "account_reward_summon_catalog";

struct summon_catalog
{
	uint64_t revision = 1;
	std::vector<flatfile_account_reward_summon_record> records;
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
};

std::string domains_directory(const std::string &root)
{
	return root + "/domains";
}

bool record_less(const flatfile_account_reward_summon_record &left,
		 const flatfile_account_reward_summon_record &right)
{
	if (left.grant_id != right.grant_id)
		return left.grant_id < right.grant_id;
	return left.pid < right.pid;
}

bool valid_catalog(const summon_catalog &catalog)
{
	if (!catalog.revision || catalog.records.size() > record_maximum ||
	    !std::is_sorted(catalog.records.begin(), catalog.records.end(), record_less))
		return false;
	for (size_t index = 0; index < catalog.records.size(); ++index)
	{
		const auto &record = catalog.records[index];
		if (!record.grant_id || !record.pid || record.last_summoned_at < 0 ||
		    !record.revision || (index && !record_less(catalog.records[index - 1], record)))
			return false;
	}
	return true;
}

bool encode_catalog(const summon_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !valid_catalog(catalog))
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.records.size());
	for (const auto &record : catalog.records)
	{
		payload.number(record.grant_id);
		payload.number(record.pid);
		payload.number(record.last_summoned_at);
		payload.number<uint8_t>(record.recovery_ready ? 1 : 0);
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

bool decode_catalog(const std::vector<uint8_t> &bytes, summon_catalog *catalog)
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
	if (!payload.number(&count) || count > record_maximum)
		return false;
	summon_catalog decoded;
	decoded.revision = revision;
	try
	{
		decoded.records.resize(count);
		for (auto &record : decoded.records)
		{
			uint8_t recovery_ready = 0;
			if (!payload.number(&record.grant_id) || !payload.number(&record.pid) ||
			    !payload.number(&record.last_summoned_at) ||
			    !payload.number(&recovery_ready) || recovery_ready > 1 ||
			    !payload.number(&record.revision))
				return false;
			record.recovery_ready = recovery_ready != 0;
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

flatfile_account_reward_summon_result
recover(const std::string &root, const flatfile_authority_lock &lock, std::string *error)
{
	const auto result = flatfile_authority_transaction_recover(root, lock, error);
	if (result == flatfile_authority_transaction_result::ok)
		return flatfile_account_reward_summon_result::ok;
	return result == flatfile_authority_transaction_result::io_error ?
		       flatfile_account_reward_summon_result::io_error :
		       flatfile_account_reward_summon_result::invalid;
}

flatfile_account_reward_summon_result load_catalog(const std::string &root, summon_catalog *catalog,
						   std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(domains_directory(root), catalog_filename,
					  catalog_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_account_reward_summon_result::not_found;
	if (loaded == flatfile_read_result::io_error)
		return flatfile_account_reward_summon_result::io_error;
	if (loaded != flatfile_read_result::ok || !decode_catalog(bytes, catalog))
	{
		if (error && error->empty())
			*error = "account reward summon catalog is corrupt";
		return flatfile_account_reward_summon_result::invalid;
	}
	return flatfile_account_reward_summon_result::ok;
}

bool catalog_equal(summon_catalog left, summon_catalog right)
{
	left.revision = 1;
	right.revision = 1;
	std::vector<uint8_t> left_bytes, right_bytes;
	return encode_catalog(left, &left_bytes) && encode_catalog(right, &right_bytes) &&
	       left_bytes == right_bytes;
}
} // namespace

flatfile_account_reward_summon_result flatfile_account_reward_summon_establish(
	const std::string &root, const std::vector<flatfile_account_reward_summon_record> &records,
	std::string *error)
{
	if (root.empty())
		return flatfile_account_reward_summon_result::invalid;
	summon_catalog candidate;
	try
	{
		candidate.records = records;
		std::sort(candidate.records.begin(), candidate.records.end(), record_less);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_account_reward_summon_result::io_error;
	}
	if (!valid_catalog(candidate))
		return flatfile_account_reward_summon_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_account_reward_summon_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_account_reward_summon_result::ok)
		return recovered;
	summon_catalog existing;
	const auto loaded = load_catalog(root, &existing, error);
	if (loaded == flatfile_account_reward_summon_result::ok)
		return catalog_equal(existing, candidate) ?
			       flatfile_account_reward_summon_result::already_exists :
			       flatfile_account_reward_summon_result::invalid;
	if (loaded != flatfile_account_reward_summon_result::not_found)
		return loaded;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(candidate, &encoded))
		return flatfile_account_reward_summon_result::invalid;
	if (!flatfile_atomic_write(domains_directory(root), catalog_filename, encoded, error))
		return flatfile_account_reward_summon_result::io_error;
	return flatfile_account_reward_summon_result::ok;
}

flatfile_account_reward_summon_result
flatfile_account_reward_summon_list(const std::string &root,
				    std::vector<flatfile_account_reward_summon_record> *records,
				    std::string *error)
{
	if (root.empty() || !records)
		return flatfile_account_reward_summon_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_account_reward_summon_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_account_reward_summon_result::ok)
		return recovered;
	summon_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_account_reward_summon_result::ok)
		return loaded;
	*records = std::move(catalog.records);
	return flatfile_account_reward_summon_result::ok;
}

flatfile_account_reward_summon_result flatfile_account_reward_summon_prepare_player_remove(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	flatfile_authority_operation *operation, std::string *error)
{
	if (root.empty() || !lock.matches(root) || !pid || !operation)
		return flatfile_account_reward_summon_result::invalid;
	*operation = {};
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_account_reward_summon_result::ok)
		return recovered;
	summon_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_account_reward_summon_result::ok)
		return loaded;
	if (catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_account_reward_summon_result::invalid;
	const auto original_size = catalog.records.size();
	catalog.records.erase(std::remove_if(catalog.records.begin(), catalog.records.end(),
					     [pid](const auto &record)
					     { return record.pid == pid; }),
			      catalog.records.end());
	if (catalog.records.size() == original_size)
		return flatfile_account_reward_summon_result::unchanged;
	++catalog.revision;
	std::vector<uint8_t> encoded;
	if (!encode_catalog(catalog, &encoded))
		return flatfile_account_reward_summon_result::invalid;
	operation->store = flatfile_authority_store::domains;
	operation->kind = flatfile_authority_operation_kind::write;
	operation->filename = catalog_filename;
	operation->bytes = std::move(encoded);
	return flatfile_account_reward_summon_result::ok;
}
