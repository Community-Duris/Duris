#include "flatfile_artifact_repository.h"

#include "flatfile_store.h"
#include "player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
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
constexpr uint32_t artifact_extra_flag = 1U << 28;

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

flatfile_artifact_result flatfile_artifact_ensure(const std::string &root, std::string *error)
{
	if (root.empty())
		return flatfile_artifact_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_artifact_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_artifact_result::ok)
		return recovered;
	artifact_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded == flatfile_artifact_result::ok)
		return flatfile_artifact_result::already_exists;
	if (loaded != flatfile_artifact_result::not_found)
		return loaded;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
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

flatfile_artifact_result flatfile_artifact_get(const std::string &root, int32_t vnum,
					       flatfile_artifact_record *record, std::string *error)
{
	if (record)
		*record = {};
	if (root.empty() || vnum <= 0 || !record)
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
	const auto found = std::lower_bound(catalog.records.begin(), catalog.records.end(), vnum,
					    [](const flatfile_artifact_record &candidate,
					       int32_t sought) { return candidate.vnum < sought; });
	if (found == catalog.records.end() || found->vnum != vnum)
		return flatfile_artifact_result::not_found;
	*record = *found;
	return flatfile_artifact_result::ok;
}

flatfile_artifact_result flatfile_artifact_erase(const std::string &root, int32_t vnum,
						 std::string *error)
{
	if (root.empty() || vnum <= 0)
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
	const auto found = std::lower_bound(catalog.records.begin(), catalog.records.end(), vnum,
					    [](const flatfile_artifact_record &candidate,
					       int32_t sought) { return candidate.vnum < sought; });
	if (found == catalog.records.end() || found->vnum != vnum)
		return flatfile_artifact_result::not_found;
	if (catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_artifact_result::invalid;
	catalog.records.erase(found);
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_artifact_result::invalid;
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_artifact_result::ok :
		       flatfile_artifact_result::io_error;
}

flatfile_artifact_result flatfile_artifact_gameplay_update(const std::string &root, int32_t vnum,
							   bool owned, int32_t location_type,
							   int32_t location, int64_t timer,
							   int32_t type, int64_t last_update,
							   std::string *error)
{
	if (root.empty() || vnum <= 0 || location_type < FLATFILE_ARTIFACT_NOT_IN_GAME ||
	    location_type > FLATFILE_ARTIFACT_ON_CORPSE || timer < 0 || type < 1 || type > 3 ||
	    last_update < 0)
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
	auto found = std::lower_bound(catalog.records.begin(), catalog.records.end(), vnum,
				      [](const flatfile_artifact_record &candidate, int32_t sought)
				      { return candidate.vnum < sought; });
	if (found != catalog.records.end() && found->vnum == vnum)
	{
		if (found->owned == owned && found->location_type == location_type &&
		    found->location == location && found->timer == timer && found->type == type &&
		    found->last_update == last_update)
			return flatfile_artifact_result::unchanged;
		if (found->revision == std::numeric_limits<uint64_t>::max())
			return flatfile_artifact_result::invalid;
		found->owned = owned;
		found->location_type = location_type;
		found->location = location;
		found->timer = timer;
		found->type = type;
		found->last_update = last_update;
		++found->revision;
	}
	else
	{
		const flatfile_artifact_record inserted = { vnum,  owned, location_type, location,
							    timer, type,  last_update,	 0,
							    0,	   1 };
		try
		{
			catalog.records.insert(found, inserted);
		}
		catch (const std::bad_alloc &)
		{
			return flatfile_artifact_result::io_error;
		}
	}
	if (catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_artifact_result::invalid;
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_artifact_result::invalid;
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_artifact_result::ok :
		       flatfile_artifact_result::io_error;
}

flatfile_artifact_result flatfile_artifact_remove_owned(const std::string &root, int32_t vnum,
							int32_t corpse_pid, int32_t type,
							int64_t last_update, std::string *error)
{
	if (root.empty() || vnum <= 0 || type < 1 || type > 3 || last_update < 0)
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
	auto found = std::lower_bound(catalog.records.begin(), catalog.records.end(), vnum,
				      [](const flatfile_artifact_record &candidate, int32_t sought)
				      { return candidate.vnum < sought; });
	if (found == catalog.records.end() || found->vnum != vnum)
	{
		if (corpse_pid <= 0)
			return flatfile_artifact_result::unchanged;
		const flatfile_artifact_record inserted = { vnum,
							    true,
							    FLATFILE_ARTIFACT_ON_CORPSE,
							    corpse_pid,
							    0,
							    type,
							    last_update,
							    -1,
							    0,
							    1 };
		try
		{
			catalog.records.insert(found, inserted);
		}
		catch (const std::bad_alloc &)
		{
			return flatfile_artifact_result::io_error;
		}
	}
	else
	{
		const bool on_corpse = corpse_pid > 0;
		const int32_t location_type = on_corpse ? FLATFILE_ARTIFACT_ON_CORPSE :
							  FLATFILE_ARTIFACT_NOT_IN_GAME;
		const int32_t location = on_corpse ? corpse_pid : -1;
		if (found->owned == on_corpse && found->location_type == location_type &&
		    found->location == location && found->last_update == last_update &&
		    found->bind_owner_pid == -1 && found->bind_timer == 0)
			return flatfile_artifact_result::unchanged;
		if (found->revision == std::numeric_limits<uint64_t>::max())
			return flatfile_artifact_result::invalid;
		found->owned = on_corpse;
		found->location_type = location_type;
		found->location = location;
		found->last_update = last_update;
		found->bind_owner_pid = -1;
		found->bind_timer = 0;
		++found->revision;
	}
	if (catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_artifact_result::invalid;
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_artifact_result::invalid;
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_artifact_result::ok :
		       flatfile_artifact_result::io_error;
}

flatfile_artifact_result flatfile_artifact_extend_timer(const std::string &root, int32_t vnum,
							int64_t minimum_timer, int64_t last_update,
							std::string *error)
{
	if (root.empty() || vnum <= 0 || minimum_timer <= 0 || last_update < 0)
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
	auto found = std::lower_bound(catalog.records.begin(), catalog.records.end(), vnum,
				      [](const flatfile_artifact_record &candidate, int32_t sought)
				      { return candidate.vnum < sought; });
	if (found == catalog.records.end() || found->vnum != vnum)
		return flatfile_artifact_result::not_found;
	const int64_t timer = std::max(found->timer, minimum_timer);
	if (found->timer == timer && found->last_update == last_update)
		return flatfile_artifact_result::unchanged;
	if (found->revision == std::numeric_limits<uint64_t>::max() ||
	    catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_artifact_result::invalid;
	found->timer = timer;
	found->last_update = last_update;
	++found->revision;
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_artifact_result::invalid;
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_artifact_result::ok :
		       flatfile_artifact_result::io_error;
}

flatfile_artifact_result flatfile_artifact_find_next_expired(const std::string &root,
							     int32_t after_vnum, int64_t now,
							     flatfile_artifact_record *record,
							     std::string *error)
{
	if (record)
		*record = {};
	if (root.empty() || after_vnum < 0 || now < 0 || !record)
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
	const auto first =
		std::upper_bound(catalog.records.begin(), catalog.records.end(), after_vnum,
				 [](int32_t sought, const flatfile_artifact_record &candidate)
				 { return sought < candidate.vnum; });
	const auto found = std::find_if(
		first, catalog.records.end(), [=](const flatfile_artifact_record &candidate)
		{ return candidate.owned && candidate.timer > 0 && candidate.timer < now; });
	if (found == catalog.records.end())
		return flatfile_artifact_result::not_found;
	*record = *found;
	return flatfile_artifact_result::ok;
}

flatfile_artifact_result flatfile_artifact_expire(const std::string &root, int32_t vnum,
						  int64_t now, std::string *error)
{
	if (root.empty() || vnum <= 0 || now < 0)
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
	auto found = std::lower_bound(catalog.records.begin(), catalog.records.end(), vnum,
				      [](const flatfile_artifact_record &candidate, int32_t sought)
				      { return candidate.vnum < sought; });
	if (found == catalog.records.end() || found->vnum != vnum)
		return flatfile_artifact_result::not_found;
	if (!found->owned || found->timer <= 0 || found->timer >= now)
		return flatfile_artifact_result::unchanged;
	if (found->revision == std::numeric_limits<uint64_t>::max() ||
	    catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_artifact_result::invalid;
	found->owned = false;
	found->location_type = FLATFILE_ARTIFACT_NOT_IN_GAME;
	found->location = -1;
	found->timer = 0;
	found->last_update = now;
	++found->revision;
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_artifact_result::invalid;
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_artifact_result::ok :
		       flatfile_artifact_result::io_error;
}

flatfile_artifact_result
flatfile_artifact_war_owners(const std::string &root, int32_t after_pid, size_t maximum,
			     std::vector<flatfile_artifact_war_owner> *owners, std::string *error)
{
	if (owners)
		owners->clear();
	if (root.empty() || after_pid < 0 || !maximum || maximum > record_maximum || !owners)
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
		std::map<int32_t, flatfile_artifact_war_owner> grouped;
		for (const auto &record : catalog.records)
		{
			if (record.location_type != FLATFILE_ARTIFACT_ON_PLAYER ||
			    record.location <= after_pid)
				continue;
			auto &owner = grouped[record.location];
			owner.pid = record.location;
			++owner.total;
			if (record.type == 1)
				++owner.major;
			else if (record.type == 2)
				++owner.unique;
			else
				++owner.ioun;
		}
		owners->reserve(std::min(maximum, grouped.size()));
		for (const auto &[pid, owner] : grouped)
		{
			(void)pid;
			if (owner.major <= 1 && owner.unique <= 1 && owner.ioun <= 1)
				continue;
			owners->push_back(owner);
			if (owners->size() == maximum)
				break;
		}
	}
	catch (const std::bad_alloc &)
	{
		owners->clear();
		return flatfile_artifact_result::io_error;
	}
	return flatfile_artifact_result::ok;
}

flatfile_artifact_result flatfile_artifact_apply_war_burn(const std::string &root, int32_t pid,
							  int64_t now, double retained,
							  int64_t last_update, std::string *error)
{
	if (root.empty() || pid <= 0 || now < 0 || !std::isfinite(retained) || retained < 0.0 ||
	    retained > 1.0 || last_update < 0)
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
	bool changed = false;
	for (auto &record : catalog.records)
	{
		if (record.location_type != FLATFILE_ARTIFACT_ON_PLAYER || record.location != pid ||
		    record.timer <= now)
			continue;
		if (record.revision == std::numeric_limits<uint64_t>::max())
			return flatfile_artifact_result::invalid;
		const int64_t remaining = record.timer - now;
		const int64_t reduced = static_cast<int64_t>(
			std::floor(static_cast<long double>(remaining) * retained));
		record.timer = now + reduced;
		record.last_update = last_update;
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
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_artifact_result::ok :
		       flatfile_artifact_result::io_error;
}

flatfile_artifact_result flatfile_artifact_bind_get(const std::string &root, int32_t vnum,
						    int32_t *owner_pid, int64_t *timer,
						    std::string *error)
{
	if (owner_pid)
		*owner_pid = 0;
	if (timer)
		*timer = 0;
	if (root.empty() || vnum <= 0 || !owner_pid || !timer)
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
	const auto found = std::lower_bound(catalog.records.begin(), catalog.records.end(), vnum,
					    [](const flatfile_artifact_record &record,
					       int32_t sought) { return record.vnum < sought; });
	if (found == catalog.records.end() || found->vnum != vnum)
		return flatfile_artifact_result::not_found;
	*owner_pid = found->bind_owner_pid;
	*timer = found->bind_timer;
	return flatfile_artifact_result::ok;
}

flatfile_artifact_result flatfile_artifact_bind_update(const std::string &root, int32_t vnum,
						       int32_t owner_pid, int64_t timer,
						       std::string *error)
{
	if (root.empty() || vnum <= 0 || owner_pid < -1 || timer < 0)
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
	const auto found = std::lower_bound(catalog.records.begin(), catalog.records.end(), vnum,
					    [](const flatfile_artifact_record &record,
					       int32_t sought) { return record.vnum < sought; });
	if (found == catalog.records.end() || found->vnum != vnum)
		return flatfile_artifact_result::not_found;
	if (found->bind_owner_pid == owner_pid && found->bind_timer == timer)
		return flatfile_artifact_result::unchanged;
	if (found->revision == std::numeric_limits<uint64_t>::max() ||
	    catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_artifact_result::invalid;
	found->bind_owner_pid = owner_pid;
	found->bind_timer = timer;
	++found->revision;
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_artifact_result::invalid;
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_artifact_result::ok :
		       flatfile_artifact_result::io_error;
}

flatfile_artifact_result flatfile_artifact_bind_reset_all(const std::string &root,
							  std::string *error)
{
	if (root.empty())
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
	bool changed = false;
	for (auto &record : catalog.records)
	{
		if (record.bind_owner_pid == -1 && record.bind_timer == 0)
			continue;
		if (record.revision == std::numeric_limits<uint64_t>::max())
			return flatfile_artifact_result::invalid;
		record.bind_owner_pid = -1;
		record.bind_timer = 0;
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
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_artifact_result::ok :
		       flatfile_artifact_result::io_error;
}

flatfile_artifact_result
flatfile_artifact_repair_player_binding(const std::string &root, int32_t vnum,
					int64_t artifact_timer, int64_t bind_timer,
					int64_t last_update, std::string *error)
{
	if (root.empty() || vnum <= 0 || artifact_timer <= 0 || bind_timer < 0 || last_update < 0)
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
	auto found = std::lower_bound(catalog.records.begin(), catalog.records.end(), vnum,
				      [](const flatfile_artifact_record &candidate, int32_t sought)
				      { return candidate.vnum < sought; });
	if (found == catalog.records.end() || found->vnum != vnum)
		return flatfile_artifact_result::not_found;
	if (found->location_type != FLATFILE_ARTIFACT_ON_PLAYER || found->location <= 0)
		return flatfile_artifact_result::conflict;
	if (found->bind_owner_pid == found->location)
		return flatfile_artifact_result::unchanged;
	if (found->revision == std::numeric_limits<uint64_t>::max() ||
	    catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_artifact_result::invalid;
	found->timer = artifact_timer;
	found->last_update = last_update;
	found->bind_owner_pid = found->location;
	found->bind_timer = bind_timer;
	++found->revision;
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_artifact_result::invalid;
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_artifact_result::ok :
		       flatfile_artifact_result::io_error;
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
	bool changed = false;
	for (auto &record : catalog.records)
	{
		const bool held = (record.location_type == FLATFILE_ARTIFACT_ON_PLAYER ||
				   record.location_type == FLATFILE_ARTIFACT_ON_CORPSE) &&
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

flatfile_artifact_result flatfile_artifact_release_player(const std::string &root, uint32_t pid,
							  std::string *error)
{
	if (root.empty() || !pid)
		return flatfile_artifact_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_artifact_result::io_error;
	flatfile_authority_operation operation;
	const auto prepared =
		flatfile_artifact_prepare_player_release(root, lock, pid, &operation, error);
	if (prepared != flatfile_artifact_result::ok)
		return prepared;
	const auto committed =
		flatfile_authority_transaction_commit_operations(root, lock, { operation }, error);
	if (committed == flatfile_authority_transaction_result::ok)
		return flatfile_artifact_result::ok;
	return committed == flatfile_authority_transaction_result::io_error ?
		       flatfile_artifact_result::io_error :
		       flatfile_artifact_result::invalid;
}

flatfile_artifact_result flatfile_artifact_reconcile_players(
	const std::string &root, const std::vector<flatfile_artifact_player_item> &items,
	int64_t reconciled_at, flatfile_artifact_reconcile_result *result, std::string *error)
{
	if (result)
		*result = {};
	if (root.empty() || reconciled_at < 0 || !result)
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
	std::map<int32_t, int32_t> player_by_vnum;
	for (const auto &item : items)
	{
		if (item.vnum <= 0 || item.pid <= 0)
			return flatfile_artifact_result::invalid;
		const auto artifact =
			std::lower_bound(catalog.records.begin(), catalog.records.end(), item.vnum,
					 [](const flatfile_artifact_record &candidate,
					    int32_t sought) { return candidate.vnum < sought; });
		if (artifact == catalog.records.end() || artifact->vnum != item.vnum)
			continue;
		if (!player_by_vnum.emplace(item.vnum, item.pid).second)
			return flatfile_artifact_result::conflict;
	}
	bool changed = false;
	for (auto &record : catalog.records)
	{
		const bool held = record.location_type == FLATFILE_ARTIFACT_ON_PLAYER ||
				  record.location_type == FLATFILE_ARTIFACT_ON_CORPSE;
		if (held)
			++result->cleared;
		const auto player = player_by_vnum.find(record.vnum);
		if (player != player_by_vnum.end())
			++result->updated;
		flatfile_artifact_record desired = record;
		if (player != player_by_vnum.end())
		{
			desired.owned = true;
			desired.location_type = FLATFILE_ARTIFACT_ON_PLAYER;
			desired.location = player->second;
			desired.last_update = reconciled_at;
			desired.bind_owner_pid = player->second;
			desired.bind_timer = reconciled_at;
		}
		else
		{
			if (held)
			{
				desired.owned = false;
				desired.location_type = FLATFILE_ARTIFACT_NOT_IN_GAME;
				desired.location = 0;
				desired.last_update = reconciled_at;
			}
			desired.bind_owner_pid = -1;
			desired.bind_timer = 0;
		}
		if (desired == record)
			continue;
		if (record.revision == std::numeric_limits<uint64_t>::max())
			return flatfile_artifact_result::invalid;
		desired.revision = record.revision + 1;
		record = desired;
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
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_artifact_result::ok :
		       flatfile_artifact_result::io_error;
}

flatfile_artifact_result flatfile_artifact_prepare_corpse_transfer(
	const std::string &root, const flatfile_authority_lock &lock,
	const item_transfer_payload &payload, uint64_t accepted_at_usec,
	flatfile_artifact_transfer_mutation *mutation, std::string *error)
{
	constexpr int64_t cross_race_feed_seconds = 5 * 24 * 60 * 60;
	constexpr size_t corpse_racewar_value_index = 5;
	if (root.empty() || !lock.matches(root) || !mutation || !payload.item_count ||
	    payload.item_count > ITEM_TRANSFER_MAX_ITEMS || accepted_at_usec / 1000000 > INT64_MAX)
		return flatfile_artifact_result::invalid;
	*mutation = {};
	const bool create = payload.from_owner.type == item_owner_type::player &&
			    payload.to_owner.type == item_owner_type::corpse &&
			    payload.reason == item_transfer_reason::corpse_create;
	const bool loot = payload.from_owner.type == item_owner_type::corpse &&
			  payload.to_owner.type == item_owner_type::player &&
			  payload.reason == item_transfer_reason::corpse_loot;
	if (create == loot)
		return flatfile_artifact_result::invalid;
	std::vector<player_item_snapshot> exact_items;
	if (!payload.item_blob_size || payload.item_blob_size > payload.item_blob.size() ||
	    player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					     &exact_items) != player_snapshot_codec_result::ok ||
	    exact_items.size() != payload.item_count || exact_items.empty() ||
	    exact_items.front().object_uid != payload.selected_item_uid ||
	    !std::all_of(exact_items.begin(), exact_items.end(),
			 [&](const auto &item)
			 {
				 return std::any_of(payload.items.begin(),
						    payload.items.begin() + payload.item_count,
						    [&](const auto &entry) {
							    return entry.item_uid ==
									   item.object_uid &&
								   entry.vnum == item.vnum;
						    });
			 }) ||
	    !std::all_of(payload.items.begin(), payload.items.begin() + payload.item_count,
			 [&](const auto &entry)
			 {
				 return std::count_if(exact_items.begin(), exact_items.end(),
						      [&](const auto &item) {
							      return entry.item_uid ==
									     item.object_uid &&
								     entry.vnum == item.vnum;
						      }) == 1;
			 }))
		return flatfile_artifact_result::invalid;
	const item_owner_identity &corpse_owner = create ? payload.to_owner : payload.from_owner;
	const uint32_t corpse_pid = static_cast<uint32_t>(corpse_owner.id >> 32);
	const uint32_t corpse_save_id = static_cast<uint32_t>(corpse_owner.id);
	const uint64_t player_id = create ? payload.from_owner.id : payload.to_owner.id;
	if (!corpse_pid || corpse_pid > INT32_MAX || !corpse_save_id || !player_id ||
	    corpse_owner.id != item_corpse_owner_id(corpse_pid, corpse_save_id) ||
	    player_id > INT32_MAX ||
	    (payload.corpse.present && (payload.corpse.actor_racewar > 4 ||
					payload.corpse.values[corpse_racewar_value_index] < 0 ||
					payload.corpse.values[corpse_racewar_value_index] > 4)))
		return flatfile_artifact_result::invalid;
	artifact_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_artifact_result::ok)
		return loaded;
	for (const auto &item : exact_items)
	{
		if (!(item.extra_flags & artifact_extra_flag))
			continue;
		flatfile_artifact_record key = {};
		key.vnum = item.vnum;
		const auto record = std::lower_bound(catalog.records.begin(), catalog.records.end(),
						     key, record_less);
		if (record == catalog.records.end() || record->vnum != key.vnum)
			return flatfile_artifact_result::conflict;
	}
	const int64_t event_time = static_cast<int64_t>(accepted_at_usec / 1000000);
	bool changed = false;
	for (auto &record : catalog.records)
	{
		const bool selected = std::any_of(payload.items.begin(),
						  payload.items.begin() + payload.item_count,
						  [&](const auto &item)
						  { return item.vnum == record.vnum; });
		if (!selected)
			continue;
		if (!payload.corpse.present)
			return flatfile_artifact_result::conflict;
		const int32_t expected_location =
			static_cast<int32_t>(create ? player_id : corpse_pid);
		const int32_t expected_type = create ? FLATFILE_ARTIFACT_ON_PLAYER :
						       FLATFILE_ARTIFACT_ON_CORPSE;
		if (!record.owned || record.location_type != expected_type ||
		    record.location != expected_location || record.revision == UINT64_MAX)
			return flatfile_artifact_result::conflict;
		record.owned = true;
		record.location_type = create ? FLATFILE_ARTIFACT_ON_CORPSE :
						FLATFILE_ARTIFACT_ON_PLAYER;
		record.location = static_cast<int32_t>(create ? corpse_pid : player_id);
		record.last_update = event_time;
		if (create)
		{
			record.bind_owner_pid = -1;
			record.bind_timer = 0;
		}
		else
		{
			const int32_t corpse_racewar =
				payload.corpse.values[corpse_racewar_value_index];
			if (corpse_racewar && payload.corpse.actor_racewar != corpse_racewar)
			{
				if (event_time > INT64_MAX - cross_race_feed_seconds)
					return flatfile_artifact_result::invalid;
				record.bind_owner_pid = -1;
				record.bind_timer = event_time;
				record.timer = std::max(record.timer,
							event_time + cross_race_feed_seconds);
			}
		}
		++record.revision;
		changed = true;
	}
	if (!changed)
		return flatfile_artifact_result::unchanged;
	if (catalog.revision == UINT64_MAX)
		return flatfile_artifact_result::invalid;
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_artifact_result::invalid;
	mutation->after_image = { catalog_filename, std::move(bytes) };
	return flatfile_artifact_result::ok;
}

flatfile_artifact_result flatfile_artifact_prepare_room_transfer(
	const std::string &root, const flatfile_authority_lock &lock,
	const item_transfer_payload &payload, uint64_t accepted_at_usec,
	flatfile_artifact_transfer_mutation *mutation, std::string *error)
{
	if (root.empty() || !lock.matches(root) || !mutation || !payload.item_count ||
	    payload.item_count > ITEM_TRANSFER_MAX_ITEMS || !payload.item_blob_size ||
	    payload.item_blob_size > payload.item_blob.size() ||
	    accepted_at_usec / 1000000 > INT64_MAX)
		return flatfile_artifact_result::invalid;
	*mutation = {};
	const bool deposit = payload.from_owner.type == item_owner_type::player &&
			     payload.to_owner.type == item_owner_type::room &&
			     ((payload.reason == item_transfer_reason::player_drop &&
			       !payload.target_parent_item_uid) ||
			      (payload.reason == item_transfer_reason::player_put &&
			       payload.target_parent_item_uid));
	const bool withdraw = payload.from_owner.type == item_owner_type::room &&
			      payload.to_owner.type == item_owner_type::player &&
			      payload.reason == item_transfer_reason::player_get &&
			      !payload.target_parent_item_uid;
	if (deposit == withdraw)
		return flatfile_artifact_result::invalid;
	const uint64_t player_id = deposit ? payload.from_owner.id : payload.to_owner.id;
	const uint64_t room_id = deposit ? payload.to_owner.id : payload.from_owner.id;
	if (!player_id || player_id > INT32_MAX || !room_id || room_id > INT32_MAX ||
	    payload.from_owner.context_id || payload.to_owner.context_id)
		return flatfile_artifact_result::invalid;
	std::vector<player_item_snapshot> exact_items;
	if (player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					     &exact_items) != player_snapshot_codec_result::ok ||
	    exact_items.size() != payload.item_count || exact_items.empty() ||
	    exact_items.front().object_uid != payload.selected_item_uid ||
	    !std::all_of(exact_items.begin(), exact_items.end(),
			 [&](const auto &item)
			 {
				 return std::count_if(payload.items.begin(),
						      payload.items.begin() + payload.item_count,
						      [&](const auto &entry) {
							      return entry.item_uid ==
									     item.object_uid &&
								     entry.vnum == item.vnum;
						      }) == 1;
			 }) ||
	    !std::all_of(payload.items.begin(), payload.items.begin() + payload.item_count,
			 [&](const auto &entry)
			 {
				 return std::count_if(exact_items.begin(), exact_items.end(),
						      [&](const auto &item) {
							      return entry.item_uid ==
									     item.object_uid &&
								     entry.vnum == item.vnum;
						      }) == 1;
			 }))
		return flatfile_artifact_result::invalid;
	if (std::none_of(exact_items.begin(), exact_items.end(),
			 [](const auto &item) { return item.extra_flags & artifact_extra_flag; }))
		return flatfile_artifact_result::unchanged;
	artifact_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_artifact_result::ok)
		return loaded;
	for (size_t index = 0; index < exact_items.size(); ++index)
	{
		const auto &item = exact_items[index];
		if (!(item.extra_flags & artifact_extra_flag))
			continue;
		if (std::any_of(exact_items.begin(), exact_items.begin() + index,
				[&](const auto &prior) {
					return (prior.extra_flags & artifact_extra_flag) &&
					       prior.vnum == item.vnum;
				}))
			return flatfile_artifact_result::conflict;
		flatfile_artifact_record key = {};
		key.vnum = item.vnum;
		const auto record = std::lower_bound(catalog.records.begin(), catalog.records.end(),
						     key, record_less);
		const int32_t expected_type = deposit ? FLATFILE_ARTIFACT_ON_PLAYER :
							FLATFILE_ARTIFACT_ON_GROUND;
		const int32_t expected_location =
			static_cast<int32_t>(deposit ? player_id : room_id);
		if (record == catalog.records.end() || record->vnum != item.vnum ||
		    !record->owned || record->location_type != expected_type ||
		    record->location != expected_location || record->revision == UINT64_MAX)
			return flatfile_artifact_result::conflict;
	}
	const int64_t event_time = static_cast<int64_t>(accepted_at_usec / 1000000);
	bool changed = false;
	for (auto &record : catalog.records)
	{
		const bool selected =
			std::any_of(exact_items.begin(), exact_items.end(),
				    [&](const auto &item) {
					    return (item.extra_flags & artifact_extra_flag) &&
						   item.vnum == record.vnum;
				    });
		if (!selected)
			continue;
		record.location_type = deposit ? FLATFILE_ARTIFACT_ON_GROUND :
						 FLATFILE_ARTIFACT_ON_PLAYER;
		record.location = static_cast<int32_t>(deposit ? room_id : player_id);
		record.last_update = event_time;
		++record.revision;
		changed = true;
	}
	if (!changed)
		return flatfile_artifact_result::unchanged;
	if (catalog.revision == UINT64_MAX)
		return flatfile_artifact_result::invalid;
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_artifact_result::invalid;
	mutation->after_image = { catalog_filename, std::move(bytes) };
	return flatfile_artifact_result::ok;
}

flatfile_artifact_result flatfile_artifact_prepare_corpse_release(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t corpse_pid,
	int32_t room_vnum, uint64_t accepted_at_usec,
	const std::vector<player_item_snapshot> &items,
	flatfile_artifact_transfer_mutation *mutation, std::string *error)
{
	if (root.empty() || !lock.matches(root) || !mutation || !corpse_pid ||
	    corpse_pid > INT32_MAX || room_vnum <= 0 || accepted_at_usec / 1000000 > INT64_MAX)
		return flatfile_artifact_result::invalid;
	*mutation = {};
	artifact_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_artifact_result::ok)
		return loaded;
	for (size_t index = 0; index < items.size(); ++index)
	{
		const auto &item = items[index];
		if (!(item.extra_flags & artifact_extra_flag))
			continue;
		if (std::any_of(items.begin(), items.begin() + index,
				[&](const auto &prior) {
					return (prior.extra_flags & artifact_extra_flag) &&
					       prior.vnum == item.vnum;
				}))
			return flatfile_artifact_result::conflict;
		flatfile_artifact_record key = {};
		key.vnum = item.vnum;
		const auto record = std::lower_bound(catalog.records.begin(), catalog.records.end(),
						     key, record_less);
		if (record == catalog.records.end() || record->vnum != key.vnum || !record->owned ||
		    record->location_type != FLATFILE_ARTIFACT_ON_CORPSE ||
		    record->location != static_cast<int32_t>(corpse_pid) ||
		    record->revision == UINT64_MAX)
			return flatfile_artifact_result::conflict;
	}
	const int64_t event_time = static_cast<int64_t>(accepted_at_usec / 1000000);
	bool changed = false;
	for (auto &record : catalog.records)
	{
		const bool selected = std::any_of(items.begin(), items.end(), [&](const auto &item)
						  { return item.vnum == record.vnum; });
		if (!selected)
			continue;
		if (!record.owned || record.location_type != FLATFILE_ARTIFACT_ON_CORPSE ||
		    record.location != static_cast<int32_t>(corpse_pid) ||
		    record.revision == UINT64_MAX)
			return flatfile_artifact_result::conflict;
		record.location_type = FLATFILE_ARTIFACT_ON_GROUND;
		record.location = room_vnum;
		record.last_update = event_time;
		++record.revision;
		changed = true;
	}
	if (!changed)
		return flatfile_artifact_result::unchanged;
	if (catalog.revision == UINT64_MAX)
		return flatfile_artifact_result::invalid;
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_artifact_result::invalid;
	mutation->after_image = { catalog_filename, std::move(bytes) };
	return flatfile_artifact_result::ok;
}
