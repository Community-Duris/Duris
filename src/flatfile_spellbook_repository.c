#include "flatfile_spellbook_repository.h"

#include "flatfile_authority_transaction.h"
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
constexpr std::array<uint8_t, 8> catalog_magic = { 'D', 'U', 'R', 'S', 'P', 'B', 'K', 0 };
constexpr uint32_t catalog_version = 1;
constexpr size_t catalog_maximum_bytes = 256 * 1024 * 1024;
constexpr size_t player_maximum = 1048576;
constexpr size_t mobs_per_player_maximum = 65536;
constexpr const char *catalog_filename = "spellbook_catalog";

struct spellbook_catalog
{
	uint64_t revision = 1;
	std::vector<flatfile_spellbook_record> records;
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

bool record_less(const flatfile_spellbook_record &left, const flatfile_spellbook_record &right)
{
	return left.pid < right.pid;
}

bool valid_records(const std::vector<flatfile_spellbook_record> &records)
{
	if (records.size() > player_maximum ||
	    !std::is_sorted(records.begin(), records.end(), record_less))
		return false;
	for (size_t index = 0; index < records.size(); ++index)
	{
		const auto &record = records[index];
		if (!record.pid || record.mobs.size() > mobs_per_player_maximum ||
		    (index && records[index - 1].pid == record.pid) ||
		    !std::is_sorted(record.mobs.begin(), record.mobs.end()) ||
		    std::adjacent_find(record.mobs.begin(), record.mobs.end()) !=
			    record.mobs.end() ||
		    std::any_of(record.mobs.begin(), record.mobs.end(),
				[](int32_t spellbook) { return spellbook <= 0; }))
			return false;
	}
	return true;
}

bool encode_catalog(const spellbook_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !catalog.revision || !valid_records(catalog.records))
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.records.size());
	for (const auto &record : catalog.records)
	{
		payload.number(record.pid);
		payload.number<uint32_t>(record.mobs.size());
		for (int32_t spellbook : record.mobs)
			payload.number(spellbook);
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

bool decode_catalog(const std::vector<uint8_t> &bytes, spellbook_catalog *catalog)
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
	spellbook_catalog decoded;
	decoded.revision = revision;
	uint32_t count = 0;
	if (!payload.number(&count) || count > player_maximum)
		return false;
	try
	{
		decoded.records.resize(count);
		for (auto &record : decoded.records)
		{
			uint32_t spellbook_count = 0;
			if (!payload.number(&record.pid) || !payload.number(&spellbook_count) ||
			    spellbook_count > mobs_per_player_maximum)
				return false;
			record.mobs.resize(spellbook_count);
			for (int32_t &spellbook : record.mobs)
				if (!payload.number(&spellbook))
					return false;
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

flatfile_spellbook_result recover(const std::string &root, const flatfile_authority_lock &lock,
				  std::string *error)
{
	const auto result = flatfile_authority_transaction_recover(root, lock, error);
	if (result == flatfile_authority_transaction_result::ok)
		return flatfile_spellbook_result::ok;
	return result == flatfile_authority_transaction_result::io_error ?
		       flatfile_spellbook_result::io_error :
		       flatfile_spellbook_result::invalid;
}

flatfile_spellbook_result load_catalog(const std::string &root, spellbook_catalog *catalog,
				       std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(domains_directory(root), catalog_filename,
					  catalog_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_spellbook_result::not_found;
	if (loaded == flatfile_read_result::io_error)
		return flatfile_spellbook_result::io_error;
	if (loaded != flatfile_read_result::ok || !decode_catalog(bytes, catalog))
		return flatfile_spellbook_result::invalid;
	return flatfile_spellbook_result::ok;
}

flatfile_spellbook_record *find_record(spellbook_catalog *catalog, uint32_t pid)
{
	auto found = std::lower_bound(catalog->records.begin(), catalog->records.end(), pid,
				      [](const auto &record, uint32_t candidate)
				      { return record.pid < candidate; });
	return found != catalog->records.end() && found->pid == pid ? &*found : nullptr;
}

flatfile_spellbook_result publish(const std::string &root, spellbook_catalog &catalog,
				  std::string *error)
{
	if (catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_spellbook_result::invalid;
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_spellbook_result::invalid;
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_spellbook_result::ok :
		       flatfile_spellbook_result::io_error;
}
} // namespace

flatfile_spellbook_result
flatfile_spellbook_establish(const std::string &root,
			     const std::vector<flatfile_spellbook_record> &records,
			     std::string *error)
{
	if (root.empty())
		return flatfile_spellbook_result::invalid;
	spellbook_catalog candidate;
	try
	{
		candidate.records = records;
		for (auto &record : candidate.records)
		{
			std::sort(record.mobs.begin(), record.mobs.end());
			if (std::adjacent_find(record.mobs.begin(), record.mobs.end()) !=
			    record.mobs.end())
				return flatfile_spellbook_result::invalid;
		}
		std::sort(candidate.records.begin(), candidate.records.end(), record_less);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_spellbook_result::io_error;
	}
	if (!valid_records(candidate.records))
		return flatfile_spellbook_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_spellbook_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_spellbook_result::ok)
		return recovered;
	spellbook_catalog existing;
	const auto loaded = load_catalog(root, &existing, error);
	if (loaded == flatfile_spellbook_result::ok)
		return existing.records == candidate.records ?
			       flatfile_spellbook_result::already_exists :
			       flatfile_spellbook_result::invalid;
	if (loaded != flatfile_spellbook_result::not_found)
		return loaded;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(candidate, &bytes))
		return flatfile_spellbook_result::invalid;
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_spellbook_result::ok :
		       flatfile_spellbook_result::io_error;
}

flatfile_spellbook_result flatfile_spellbook_list(const std::string &root, uint32_t pid,
						  std::vector<int32_t> *mobs, std::string *error)
{
	if (root.empty() || !pid || !mobs)
		return flatfile_spellbook_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_spellbook_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_spellbook_result::ok)
		return recovered;
	spellbook_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_spellbook_result::ok)
		return loaded;
	try
	{
		if (const auto *record = find_record(&catalog, pid))
			*mobs = record->mobs;
		else
			mobs->clear();
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_spellbook_result::io_error;
	}
	return flatfile_spellbook_result::ok;
}

flatfile_spellbook_result flatfile_spellbook_contains(const std::string &root, uint32_t pid,
						      int32_t mob_vnum, bool *contains,
						      std::string *error)
{
	if (!contains || mob_vnum <= 0)
		return flatfile_spellbook_result::invalid;
	std::vector<int32_t> mobs;
	const auto loaded = flatfile_spellbook_list(root, pid, &mobs, error);
	if (loaded == flatfile_spellbook_result::ok)
		*contains = std::binary_search(mobs.begin(), mobs.end(), mob_vnum);
	return loaded;
}

flatfile_spellbook_result flatfile_spellbook_add(const std::string &root, uint32_t pid,
						 int32_t mob_vnum, std::string *error)
{
	if (root.empty() || !pid || mob_vnum <= 0)
		return flatfile_spellbook_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_spellbook_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_spellbook_result::ok)
		return recovered;
	spellbook_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_spellbook_result::ok)
		return loaded;
	flatfile_spellbook_record *record = find_record(&catalog, pid);
	try
	{
		if (!record)
		{
			if (catalog.records.size() >= player_maximum)
				return flatfile_spellbook_result::invalid;
			catalog.records.push_back({ pid, {} });
			std::sort(catalog.records.begin(), catalog.records.end(), record_less);
			record = find_record(&catalog, pid);
		}
		auto at = std::lower_bound(record->mobs.begin(), record->mobs.end(), mob_vnum);
		if (at != record->mobs.end() && *at == mob_vnum)
			return flatfile_spellbook_result::ok;
		if (record->mobs.size() >= mobs_per_player_maximum)
			return flatfile_spellbook_result::invalid;
		record->mobs.insert(at, mob_vnum);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_spellbook_result::io_error;
	}
	return publish(root, catalog, error);
}

flatfile_spellbook_result flatfile_spellbook_remove(const std::string &root, uint32_t pid,
						    int32_t mob_vnum, std::string *error)
{
	if (root.empty() || !pid || mob_vnum <= 0)
		return flatfile_spellbook_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_spellbook_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_spellbook_result::ok)
		return recovered;
	spellbook_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_spellbook_result::ok)
		return loaded;
	flatfile_spellbook_record *record = find_record(&catalog, pid);
	if (!record)
		return flatfile_spellbook_result::ok;
	auto found = std::lower_bound(record->mobs.begin(), record->mobs.end(), mob_vnum);
	if (found == record->mobs.end() || *found != mob_vnum)
		return flatfile_spellbook_result::ok;
	record->mobs.erase(found);
	if (record->mobs.empty())
	{
		auto empty = std::lower_bound(catalog.records.begin(), catalog.records.end(), pid,
					      [](const auto &candidate, uint32_t value)
					      { return candidate.pid < value; });
		catalog.records.erase(empty);
	}
	return publish(root, catalog, error);
}

flatfile_spellbook_result flatfile_spellbook_clear(const std::string &root, uint32_t pid,
						   std::string *error)
{
	if (root.empty() || !pid)
		return flatfile_spellbook_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_spellbook_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_spellbook_result::ok)
		return recovered;
	spellbook_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_spellbook_result::ok)
		return loaded;
	auto found = std::lower_bound(catalog.records.begin(), catalog.records.end(), pid,
				      [](const auto &record, uint32_t candidate)
				      { return record.pid < candidate; });
	if (found == catalog.records.end() || found->pid != pid)
		return flatfile_spellbook_result::ok;
	catalog.records.erase(found);
	return publish(root, catalog, error);
}

flatfile_spellbook_result flatfile_spellbook_prepare_clear(const std::string &root,
							   const flatfile_authority_lock &lock,
							   uint32_t pid,
							   flatfile_authority_operation *operation,
							   std::string *error)
{
	if (root.empty() || !pid || !operation || !lock.matches(root))
		return flatfile_spellbook_result::invalid;
	*operation = {};
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_spellbook_result::ok)
		return recovered;
	spellbook_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_spellbook_result::ok)
		return loaded;
	auto found = std::lower_bound(catalog.records.begin(), catalog.records.end(), pid,
				      [](const auto &record, uint32_t candidate)
				      { return record.pid < candidate; });
	if (found == catalog.records.end() || found->pid != pid)
		return flatfile_spellbook_result::not_found;
	if (catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_spellbook_result::invalid;
	catalog.records.erase(found);
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_spellbook_result::invalid;
	operation->store = flatfile_authority_store::domains;
	operation->kind = flatfile_authority_operation_kind::write;
	operation->filename = catalog_filename;
	operation->bytes = std::move(bytes);
	return flatfile_spellbook_result::ok;
}
