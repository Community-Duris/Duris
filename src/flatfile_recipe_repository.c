#include "flatfile_recipe_repository.h"

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
constexpr std::array<uint8_t, 8> catalog_magic = { 'D', 'U', 'R', 'R', 'C', 'P', 'E', 0 };
constexpr uint32_t catalog_version = 1;
constexpr size_t catalog_maximum_bytes = 256 * 1024 * 1024;
constexpr size_t player_maximum = 1048576;
constexpr size_t recipes_per_player_maximum = 65536;
constexpr const char *catalog_filename = "recipe_catalog";

struct recipe_catalog
{
	uint64_t revision = 1;
	std::vector<flatfile_recipe_record> records;
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

bool record_less(const flatfile_recipe_record &left, const flatfile_recipe_record &right)
{
	return left.pid < right.pid;
}

bool valid_records(const std::vector<flatfile_recipe_record> &records)
{
	if (records.size() > player_maximum ||
	    !std::is_sorted(records.begin(), records.end(), record_less))
		return false;
	for (size_t index = 0; index < records.size(); ++index)
	{
		const auto &record = records[index];
		if (!record.pid || record.recipes.size() > recipes_per_player_maximum ||
		    (index && records[index - 1].pid == record.pid) ||
		    !std::is_sorted(record.recipes.begin(), record.recipes.end()) ||
		    std::adjacent_find(record.recipes.begin(), record.recipes.end()) !=
			    record.recipes.end() ||
		    std::any_of(record.recipes.begin(), record.recipes.end(),
				[](int32_t recipe) { return recipe <= 0; }))
			return false;
	}
	return true;
}

bool encode_catalog(const recipe_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !catalog.revision || !valid_records(catalog.records))
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.records.size());
	for (const auto &record : catalog.records)
	{
		payload.number(record.pid);
		payload.number<uint32_t>(record.recipes.size());
		for (int32_t recipe : record.recipes)
			payload.number(recipe);
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

bool decode_catalog(const std::vector<uint8_t> &bytes, recipe_catalog *catalog)
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
	recipe_catalog decoded;
	decoded.revision = revision;
	uint32_t count = 0;
	if (!payload.number(&count) || count > player_maximum)
		return false;
	try
	{
		decoded.records.resize(count);
		for (auto &record : decoded.records)
		{
			uint32_t recipe_count = 0;
			if (!payload.number(&record.pid) || !payload.number(&recipe_count) ||
			    recipe_count > recipes_per_player_maximum)
				return false;
			record.recipes.resize(recipe_count);
			for (int32_t &recipe : record.recipes)
				if (!payload.number(&recipe))
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

flatfile_recipe_result recover(const std::string &root, const flatfile_authority_lock &lock,
			       std::string *error)
{
	const auto result = flatfile_authority_transaction_recover(root, lock, error);
	if (result == flatfile_authority_transaction_result::ok)
		return flatfile_recipe_result::ok;
	return result == flatfile_authority_transaction_result::io_error ?
		       flatfile_recipe_result::io_error :
		       flatfile_recipe_result::invalid;
}

flatfile_recipe_result load_catalog(const std::string &root, recipe_catalog *catalog,
				    std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(domains_directory(root), catalog_filename,
					  catalog_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_recipe_result::not_found;
	if (loaded == flatfile_read_result::io_error)
		return flatfile_recipe_result::io_error;
	if (loaded != flatfile_read_result::ok || !decode_catalog(bytes, catalog))
		return flatfile_recipe_result::invalid;
	return flatfile_recipe_result::ok;
}

flatfile_recipe_record *find_record(recipe_catalog *catalog, uint32_t pid)
{
	auto found = std::lower_bound(catalog->records.begin(), catalog->records.end(), pid,
				      [](const auto &record, uint32_t candidate)
				      { return record.pid < candidate; });
	return found != catalog->records.end() && found->pid == pid ? &*found : nullptr;
}

flatfile_recipe_result publish(const std::string &root, recipe_catalog &catalog, std::string *error)
{
	if (catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_recipe_result::invalid;
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_recipe_result::invalid;
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_recipe_result::ok :
		       flatfile_recipe_result::io_error;
}
} // namespace

flatfile_recipe_result flatfile_recipe_establish(const std::string &root,
						 const std::vector<flatfile_recipe_record> &records,
						 std::string *error)
{
	if (root.empty())
		return flatfile_recipe_result::invalid;
	recipe_catalog candidate;
	try
	{
		candidate.records = records;
		for (auto &record : candidate.records)
		{
			std::sort(record.recipes.begin(), record.recipes.end());
			if (std::adjacent_find(record.recipes.begin(), record.recipes.end()) !=
			    record.recipes.end())
				return flatfile_recipe_result::invalid;
		}
		std::sort(candidate.records.begin(), candidate.records.end(), record_less);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_recipe_result::io_error;
	}
	if (!valid_records(candidate.records))
		return flatfile_recipe_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_recipe_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_recipe_result::ok)
		return recovered;
	recipe_catalog existing;
	const auto loaded = load_catalog(root, &existing, error);
	if (loaded == flatfile_recipe_result::ok)
		return existing.records == candidate.records ?
			       flatfile_recipe_result::already_exists :
			       flatfile_recipe_result::invalid;
	if (loaded != flatfile_recipe_result::not_found)
		return loaded;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(candidate, &bytes))
		return flatfile_recipe_result::invalid;
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_recipe_result::ok :
		       flatfile_recipe_result::io_error;
}

flatfile_recipe_result flatfile_recipe_list(const std::string &root, uint32_t pid,
					    std::vector<int32_t> *recipes, std::string *error)
{
	if (root.empty() || !pid || !recipes)
		return flatfile_recipe_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_recipe_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_recipe_result::ok)
		return recovered;
	recipe_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_recipe_result::ok)
		return loaded;
	try
	{
		if (const auto *record = find_record(&catalog, pid))
			*recipes = record->recipes;
		else
			recipes->clear();
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_recipe_result::io_error;
	}
	return flatfile_recipe_result::ok;
}

flatfile_recipe_result flatfile_recipe_contains(const std::string &root, uint32_t pid,
						int32_t recipe_vnum, bool *contains,
						std::string *error)
{
	if (!contains || recipe_vnum <= 0)
		return flatfile_recipe_result::invalid;
	std::vector<int32_t> recipes;
	const auto loaded = flatfile_recipe_list(root, pid, &recipes, error);
	if (loaded == flatfile_recipe_result::ok)
		*contains = std::binary_search(recipes.begin(), recipes.end(), recipe_vnum);
	return loaded;
}

flatfile_recipe_result flatfile_recipe_add(const std::string &root, uint32_t pid,
					   int32_t recipe_vnum, std::string *error)
{
	if (root.empty() || !pid || recipe_vnum <= 0)
		return flatfile_recipe_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_recipe_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_recipe_result::ok)
		return recovered;
	recipe_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_recipe_result::ok)
		return loaded;
	flatfile_recipe_record *record = find_record(&catalog, pid);
	try
	{
		if (!record)
		{
			if (catalog.records.size() >= player_maximum)
				return flatfile_recipe_result::invalid;
			catalog.records.push_back({ pid, {} });
			std::sort(catalog.records.begin(), catalog.records.end(), record_less);
			record = find_record(&catalog, pid);
		}
		auto at = std::lower_bound(record->recipes.begin(), record->recipes.end(),
					   recipe_vnum);
		if (at != record->recipes.end() && *at == recipe_vnum)
			return flatfile_recipe_result::ok;
		if (record->recipes.size() >= recipes_per_player_maximum)
			return flatfile_recipe_result::invalid;
		record->recipes.insert(at, recipe_vnum);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_recipe_result::io_error;
	}
	return publish(root, catalog, error);
}

flatfile_recipe_result flatfile_recipe_clear(const std::string &root, uint32_t pid,
					     std::string *error)
{
	if (root.empty() || !pid)
		return flatfile_recipe_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_recipe_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_recipe_result::ok)
		return recovered;
	recipe_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_recipe_result::ok)
		return loaded;
	auto found = std::lower_bound(catalog.records.begin(), catalog.records.end(), pid,
				      [](const auto &record, uint32_t candidate)
				      { return record.pid < candidate; });
	if (found == catalog.records.end() || found->pid != pid)
		return flatfile_recipe_result::ok;
	catalog.records.erase(found);
	return publish(root, catalog, error);
}
