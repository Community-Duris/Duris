#include "flatfile/flatfile_artifact_repository.h"

#include "flatfile/flatfile_store.h"
#include "player/player_snapshot_codec.h"

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
	    exact_items.front().object_uid != item_transfer_result_root(payload) ||
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
			     (((payload.reason == item_transfer_reason::player_drop ||
				item_transfer_forced_weapon_drop(payload.reason)) &&
			       !payload.target_parent_item_uid) ||
			      (payload.reason == item_transfer_reason::player_put &&
			       payload.target_parent_item_uid));
	const bool withdraw = payload.from_owner.type == item_owner_type::room &&
			      payload.to_owner.type == item_owner_type::player &&
			      payload.reason == item_transfer_reason::player_get &&
			      !payload.target_parent_item_uid;
	const bool create = payload.from_owner.type == item_owner_type::system &&
			    payload.to_owner.type == item_owner_type::room &&
			    payload.reason == item_transfer_reason::creation &&
			    !payload.target_parent_item_uid;
	const bool destroy = payload.from_owner.type == item_owner_type::room &&
			     payload.to_owner.type == item_owner_type::destruction &&
			     payload.reason == item_transfer_reason::destruction &&
			     !payload.target_parent_item_uid;
	const bool reparent = item_owner_identity_equal(payload.from_owner, payload.to_owner) &&
			      payload.from_owner.type == item_owner_type::room &&
			      payload.reason == item_transfer_reason::operator_repair &&
			      !payload.target_parent_item_uid;
	if (static_cast<unsigned int>(deposit) + static_cast<unsigned int>(withdraw) +
		    static_cast<unsigned int>(create) + static_cast<unsigned int>(destroy) +
		    static_cast<unsigned int>(reparent) !=
	    1)
		return flatfile_artifact_result::invalid;
	const uint64_t player_id = deposit ? payload.from_owner.id : payload.to_owner.id;
	const uint64_t room_id = deposit || create ? payload.to_owner.id : payload.from_owner.id;
	if (((deposit || withdraw) && (!player_id || player_id > INT32_MAX)) || !room_id ||
	    room_id > INT32_MAX || payload.from_owner.context_id || payload.to_owner.context_id)
		return flatfile_artifact_result::invalid;
	std::vector<player_item_snapshot> exact_items;
	if (player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					     &exact_items) != player_snapshot_codec_result::ok ||
	    exact_items.size() != payload.item_count || exact_items.empty() ||
	    exact_items.front().object_uid != item_transfer_result_root(payload) ||
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
	if (create)
		return flatfile_artifact_result::conflict;
	if (reparent)
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
		if (destroy)
		{
			record.owned = false;
			record.location_type = FLATFILE_ARTIFACT_NOT_IN_GAME;
			record.location = -1;
			record.bind_owner_pid = -1;
			record.bind_timer = 0;
		}
		else
		{
			record.location_type = deposit ? FLATFILE_ARTIFACT_ON_GROUND :
							 FLATFILE_ARTIFACT_ON_PLAYER;
			record.location = static_cast<int32_t>(deposit ? room_id : player_id);
		}
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

static flatfile_artifact_result flatfile_artifact_prepare_corpse_disposition(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t corpse_pid,
	int32_t room_vnum, uint32_t player_pid, uint64_t accepted_at_usec, bool destroy,
	const std::vector<player_item_snapshot> &items,
	flatfile_artifact_transfer_mutation *mutation, std::string *error)
{
	if (root.empty() || !lock.matches(root) || !mutation || !corpse_pid ||
	    corpse_pid > INT32_MAX || player_pid > INT32_MAX ||
	    static_cast<unsigned int>(destroy) + static_cast<unsigned int>(room_vnum > 0) +
			    static_cast<unsigned int>(player_pid > 0) !=
		    1 ||
	    accepted_at_usec / 1000000 > INT64_MAX)
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
		if (destroy)
		{
			record.owned = false;
			record.location_type = FLATFILE_ARTIFACT_NOT_IN_GAME;
			record.location = -1;
			record.bind_owner_pid = -1;
			record.bind_timer = 0;
		}
		else if (room_vnum > 0)
		{
			record.location_type = FLATFILE_ARTIFACT_ON_GROUND;
			record.location = room_vnum;
		}
		else
		{
			record.location_type = FLATFILE_ARTIFACT_ON_PLAYER;
			record.location = static_cast<int32_t>(player_pid);
		}
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
	return flatfile_artifact_prepare_corpse_disposition(root, lock, corpse_pid, room_vnum, 0,
							    accepted_at_usec, false, items,
							    mutation, error);
}

flatfile_artifact_result flatfile_artifact_prepare_corpse_destruction(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t corpse_pid,
	uint64_t accepted_at_usec, const std::vector<player_item_snapshot> &items,
	flatfile_artifact_transfer_mutation *mutation, std::string *error)
{
	return flatfile_artifact_prepare_corpse_disposition(
		root, lock, corpse_pid, 0, 0, accepted_at_usec, true, items, mutation, error);
}

flatfile_artifact_result flatfile_artifact_prepare_corpse_resurrection(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t corpse_pid,
	uint32_t player_pid, uint64_t accepted_at_usec,
	const std::vector<player_item_snapshot> &items,
	flatfile_artifact_transfer_mutation *mutation, std::string *error)
{
	return flatfile_artifact_prepare_corpse_disposition(root, lock, corpse_pid, 0, player_pid,
							    accepted_at_usec, false, items,
							    mutation, error);
}

#include <cerrno>

namespace
{
struct native_artifact_budget
{
	flatfile_scratch_reserve_fn reserve;
	void *context;
	size_t base;
	bool admit(size_t extra) const noexcept
	{
		// Actual this/extra, callback's size/context and boolean result carriers.
		constexpr size_t call_carriers = sizeof(native_artifact_budget *) +
						 2 * sizeof(size_t) + sizeof(void *) +
						 2 * sizeof(bool);
		if (extra > SIZE_MAX - call_carriers || extra + call_carriers > SIZE_MAX - base ||
		    !reserve || !reserve(base + extra + call_carriers, context))
		{
			errno = ENOBUFS;
			return false;
		}
		return true;
	}
};
bool native_artifact_policy() noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&  \
	_GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG) && defined(__linux__) &&       \
	defined(__x86_64__) && defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR == 3 && \
	defined(OPENSSL_VERSION_MINOR) && OPENSSL_VERSION_MINOR == 0 &&                        \
	defined(OPENSSL_VERSION_PATCH) && OPENSSL_VERSION_PATCH == 13 &&                       \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	return sizeof(void *) == 8 && sizeof(size_t) == 8 && sizeof(SHA_LONG) == 4;
#else
	return false;
#endif
}
// Same installed low-level SHA256 leaf as the native-origin companion. These
// fixed workspace/assembly/C fallback terms refer to that retained source pin;
// no EVP context or implicit digest allocation is used.
int native_artifact_hash(const uint8_t *data, size_t size, uint8_t *digest,
			 const native_artifact_budget &budget, size_t retained) noexcept
{
#if defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR == 3 &&      \
	defined(OPENSSL_VERSION_MINOR) && OPENSSL_VERSION_MINOR == 0 &&  \
	defined(OPENSSL_VERSION_PATCH) && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	constexpr size_t assembly = 2 * 4 * 64 + 4 * sizeof(void *) + 6 * sizeof(uint64_t) +
				    (256 * 4 - 1) + 2 * sizeof(void *);
	constexpr size_t c_small = 16 * sizeof(unsigned) + 12 * sizeof(unsigned) +
				   sizeof(unsigned) + sizeof(int) + sizeof(void *);
	constexpr size_t c_normal = 16 * sizeof(unsigned) + 11 * sizeof(unsigned) +
				    2 * sizeof(int) + 2 * sizeof(void *);
	constexpr size_t update =
		4 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(unsigned) + sizeof(int);
	constexpr size_t final = 3 * sizeof(void *) + sizeof(size_t) + sizeof(unsigned long) +
				 sizeof(unsigned) + sizeof(int);
	constexpr size_t sha_frames =
		std::max(assembly, std::max(c_small, c_normal)) +
		std::max(sizeof(void *) + sizeof(int), std::max(update, final));
	// Own data/digest/budget reference, length AND retained parameter, return
	// status and real valid bool coexist with the admitted SHA workspace.
	constexpr size_t objects = sizeof(SHA256_CTX) + 3 * sizeof(void *) + 2 * sizeof(size_t) +
				   sizeof(int) + sizeof(bool) + sha_frames;
	if (retained > SIZE_MAX - objects || !budget.admit(retained + objects))
		return ENOBUFS;
	SHA256_CTX state;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	const bool valid = SHA256_Init(&state) == 1 && SHA256_Update(&state, data, size) == 1 &&
			   SHA256_Final(digest, &state) == 1;
#pragma GCC diagnostic pop
	return valid ? 0 : EIO;
#else
	(void)data;
	(void)size;
	(void)digest;
	(void)budget;
	(void)retained;
	return ENOTSUP;
#endif
}
int native_artifact_load(const std::string &root, const flatfile_authority_lock &lock,
			 artifact_catalog *catalog, const native_artifact_budget &budget) noexcept
{
	if (root.empty() || !lock.matches(root) || !catalog)
		return EINVAL;
	// Actual load arguments (four references/pointers) and recovered result
	// coexist with the nested original recovery; no working struct substitutes
	// for these source-declared carriers.
	constexpr size_t recovery_carriers =
		4 * sizeof(void *) + sizeof(flatfile_authority_transaction_result);
	if (recovery_carriers > SIZE_MAX - budget.base)
		return ENOBUFS;
	const auto recovered = flatfile_authority_transaction_recover_bounded(
		root, lock, budget.reserve, budget.context, budget.base + recovery_carriers);
	if (recovered != flatfile_authority_transaction_result::ok)
		return errno == ENOBUFS ? ENOBUFS : errno == ENOMEM ? ENOMEM : EIO;
	struct work
	{
		std::string directory;
		std::string name;
		std::vector<uint8_t> bytes;
		artifact_catalog candidate;
		std::array<uint8_t, SHA256_DIGEST_LENGTH> digest{};
		uint32_t version = 0, payload_size = 0, count = 0;
		uint64_t revision = 0;
	};
	if (root.size() > SIZE_MAX - 8)
		return ENOBUFS;
	const size_t directory_size = root.size() + 8;
	// Empty GCC13 string assign allocates max(requested,2*15)+1 beyond SSO.
	const size_t directory_request =
		directory_size > 15 ? std::max(directory_size, size_t(30)) + 1 : 0;
	constexpr size_t name_request = sizeof("artifact_catalog") - 1 > 15 ? 31 : 0;
	// Four argument carriers; recovered/loaded results; directory_size,
	// directory_request, retained, directory_actual and record_request; both
	// actual decoder objects, payload pointer and hash return; real range
	// reference/begin/end/record and owned byte. The decoder::number largest
	// instantiation contributes this/value pointers, uint64 bits/index/bool.
	constexpr size_t load_carriers =
		4 * sizeof(void *) + sizeof(flatfile_authority_transaction_result) +
		sizeof(flatfile_read_result) + 5 * sizeof(size_t) + 2 * sizeof(decoder) +
		sizeof(const uint8_t *) + sizeof(int) + 2 * sizeof(void *) +
		2 * sizeof(std::vector<flatfile_artifact_record>::iterator) + sizeof(uint8_t) +
		2 * sizeof(void *) + sizeof(uint64_t) + sizeof(size_t) + sizeof(bool);
	size_t retained = sizeof(work) + load_carriers;
	if (directory_request > SIZE_MAX - retained ||
	    name_request > SIZE_MAX - retained - directory_request)
		return ENOBUFS;
	retained += directory_request + name_request;
	if (!budget.admit(retained))
		return ENOBUFS;
	try
	{
		work w;
		w.directory.reserve(directory_size);
		w.directory.assign(root);
		w.directory.append("/domains");
		w.name.assign(catalog_filename);
		const size_t directory_actual =
			w.directory.capacity() > 15 ? w.directory.capacity() + 1 : 0;
		if (directory_actual > directory_request)
			return EOVERFLOW;
		const auto loaded = flatfile_read_bounded(w.directory, w.name,
							  catalog_maximum_bytes, &w.bytes,
							  budget.reserve, budget.context,
							  budget.base + retained);
		if (loaded == flatfile_read_result::not_found)
			return ENOENT;
		if (loaded != flatfile_read_result::ok)
			return loaded == flatfile_read_result::invalid ? EBADMSG :
			       errno				       ? errno :
									 EIO;
		if (w.bytes.capacity() > SIZE_MAX - retained)
			return ENOBUFS;
		retained += w.bytes.capacity();
		constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
		if (w.bytes.size() < header_size || memcmp(w.bytes.data(), catalog_magic.data(), 8))
			return EBADMSG;
		decoder header{ w.bytes.data() + 8, w.bytes.size() - 8 };
		if (!header.number(&w.version) || !header.number(&w.payload_size) ||
		    !header.number(&w.revision) || w.version != catalog_version || !w.revision ||
		    w.payload_size != w.bytes.size() - header_size)
			return EBADMSG;
		const uint8_t *payload_bytes = w.bytes.data() + header_size;
		decoder payload{ payload_bytes, w.payload_size };
		if (!payload.number(&w.count) || w.count > record_maximum ||
		    w.payload_size != 4 + size_t(w.count) * 53)
			return EBADMSG;
		const int hashed = native_artifact_hash(payload_bytes, w.payload_size,
							w.digest.data(), budget, retained);
		if (hashed)
			return hashed;
		if (CRYPTO_memcmp(w.bytes.data() + 24, w.digest.data(), w.digest.size()))
			return EBADMSG;
		const size_t record_request = size_t(w.count) * sizeof(flatfile_artifact_record);
		if (record_request > SIZE_MAX - retained ||
		    !budget.admit(retained + record_request))
			return ENOBUFS;
		w.candidate.revision = w.revision;
		w.candidate.records.reserve(w.count);
		w.candidate.records.resize(w.count);
		for (auto &record : w.candidate.records)
		{
			uint8_t owned = 0;
			if (!payload.number(&record.vnum) || !payload.number(&owned) || owned > 1 ||
			    !payload.number(&record.location_type) ||
			    !payload.number(&record.location) || !payload.number(&record.timer) ||
			    !payload.number(&record.type) || !payload.number(&record.last_update) ||
			    !payload.number(&record.bind_owner_pid) ||
			    !payload.number(&record.bind_timer) ||
			    !payload.number(&record.revision))
				return EBADMSG;
			record.owned = owned != 0;
		}
		if (payload.offset != payload.size || !valid_records(w.candidate.records))
			return EBADMSG;
		*catalog = std::move(w.candidate);
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EOVERFLOW;
	}
}
template <typename T>
void native_artifact_number(std::vector<uint8_t> &bytes, size_t &offset, T value) noexcept
{
	using U = std::make_unsigned_t<T>;
	U bits = static_cast<U>(value);
	for (size_t index = 0; index < sizeof(T); ++index)
	{
		bytes[offset++] = static_cast<uint8_t>(bits & 0xff);
		bits >>= 8;
	}
}
int native_artifact_encode(const artifact_catalog &catalog, std::vector<uint8_t> *output,
			   const native_artifact_budget &budget) noexcept
{
	if (!output || !catalog.revision || !valid_records(catalog.records))
		return EBADMSG;
	struct work
	{
		std::vector<uint8_t> payload, file;
		std::array<uint8_t, SHA256_DIGEST_LENGTH> digest{};
		size_t offset = 0;
	};
	constexpr size_t header = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	const size_t payload_size = 4 + catalog.records.size() * 53;
	const size_t file_size = header + payload_size;
	if (file_size > catalog_maximum_bytes)
		return EBADMSG;
	// Actual catalog/output/budget reference carriers, payload_size/file_size/
	// retained, hash result, record range and header index/byte ranges. The
	// largest native_artifact_number<T> contributes vector/offset references,
	// uint64 value/bits, index and void return (no payload storage allocation).
	constexpr size_t encode_carriers =
		3 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(int) + 2 * sizeof(void *) +
		2 * sizeof(std::vector<flatfile_artifact_record>::const_iterator) + sizeof(size_t) +
		2 * (3 * sizeof(void *) + sizeof(uint8_t)) + 2 * sizeof(void *) +
		2 * sizeof(uint64_t) + sizeof(size_t);
	const size_t retained = sizeof(work) + encode_carriers + payload_size + file_size;
	if (!budget.admit(retained))
		return ENOBUFS;
	try
	{
		work w;
		w.payload.reserve(payload_size);
		w.payload.resize(payload_size);
		w.file.reserve(file_size);
		w.file.resize(file_size);
		native_artifact_number<uint32_t>(w.payload, w.offset, catalog.records.size());
		for (const auto &record : catalog.records)
		{
			native_artifact_number(w.payload, w.offset, record.vnum);
			native_artifact_number<uint8_t>(w.payload, w.offset, record.owned ? 1 : 0);
			native_artifact_number(w.payload, w.offset, record.location_type);
			native_artifact_number(w.payload, w.offset, record.location);
			native_artifact_number(w.payload, w.offset, record.timer);
			native_artifact_number(w.payload, w.offset, record.type);
			native_artifact_number(w.payload, w.offset, record.last_update);
			native_artifact_number(w.payload, w.offset, record.bind_owner_pid);
			native_artifact_number(w.payload, w.offset, record.bind_timer);
			native_artifact_number(w.payload, w.offset, record.revision);
		}
		const int hashed = native_artifact_hash(w.payload.data(), w.payload.size(),
							w.digest.data(), budget, retained);
		if (hashed)
			return hashed;
		for (size_t index = 0; index < catalog_magic.size(); ++index)
			w.file[index] = catalog_magic[index];
		w.offset = 8;
		native_artifact_number(w.file, w.offset, catalog_version);
		native_artifact_number<uint32_t>(w.file, w.offset, payload_size);
		native_artifact_number(w.file, w.offset, catalog.revision);
		for (uint8_t byte : w.digest)
			w.file[w.offset++] = byte;
		for (uint8_t byte : w.payload)
			w.file[w.offset++] = byte;
		output->swap(w.file);
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EOVERFLOW;
	}
}
} // namespace

int flatfile_artifact_get_bounded(const std::string &root, const flatfile_authority_lock &lock,
				  int32_t vnum, flatfile_artifact_record *record,
				  flatfile_scratch_reserve_fn reserve, void *context,
				  size_t outer) noexcept
{
	if (!record || vnum <= 0 || !reserve)
		return EINVAL;
	if (!native_artifact_policy())
		return ENOTSUP;
	artifact_catalog catalog;
	// Real own arguments, budget object, loaded/result status and subsequent
	// original range reference, const iterators and element reference. Sum the
	// finite declared scopes conservatively before nested calls.
	constexpr size_t get_carriers = 5 * sizeof(void *) + sizeof(int32_t) + sizeof(size_t) +
					sizeof(native_artifact_budget) + 2 * sizeof(int) +
					2 * sizeof(void *) +
					2 * sizeof(std::vector<flatfile_artifact_record>::iterator);
	if (sizeof(catalog) > SIZE_MAX - get_carriers ||
	    sizeof(catalog) + get_carriers > SIZE_MAX - outer)
		return ENOBUFS;
	native_artifact_budget budget{ reserve, context, outer + sizeof(catalog) + get_carriers };
	if (!budget.admit(0))
		return ENOBUFS;
	const int loaded = native_artifact_load(root, lock, &catalog, budget);
	if (loaded)
		return loaded;
	for (const auto &candidate : catalog.records)
		if (candidate.vnum == vnum)
		{
			*record = candidate;
			return 0;
		}
	return ENOENT;
}

int flatfile_artifact_gameplay_update_bounded(const std::string &root,
					      const flatfile_authority_lock &lock, int32_t vnum,
					      bool owned, int32_t location_type, int32_t location,
					      int64_t timer, int32_t type, int64_t last_update,
					      bool *returned, bool *succeeded,
					      flatfile_scratch_reserve_fn reserve, void *context,
					      size_t outer) noexcept
{
	if (!returned || !succeeded || !reserve || vnum <= 0 || location_type < 1 ||
	    location_type > 5 || timer < 0 || type < 1 || type > 3 || last_update < 0)
		return EINVAL;
	if (*returned || *succeeded)
		return EALREADY;
	if (!native_artifact_policy())
		return ENOTSUP;
	struct work
	{
		artifact_catalog catalog;
		std::vector<uint8_t> bytes;
		std::string directory, name;
		bool published = false;
	};
	// Both actual budget objects, own six pointer/reference arguments, four
	// int32 and two int64 args, owned/outer, status/write_error/return, heap,
	// next/request/path_length/path_request/atomic and actual found iterator.
	// The original insert's temporary record and lambda argument/return carriers
	// are included while old and fresh catalog vector storage coexist.
	constexpr size_t update_carriers =
		2 * sizeof(native_artifact_budget) + 6 * sizeof(void *) + 4 * sizeof(int32_t) +
		2 * sizeof(int64_t) + sizeof(bool) + sizeof(size_t) + 3 * sizeof(int) +
		6 * sizeof(size_t) + sizeof(std::vector<flatfile_artifact_record>::iterator) +
		sizeof(bool) + sizeof(flatfile_artifact_record) + sizeof(void *) + sizeof(int32_t) +
		sizeof(bool);
	if (sizeof(work) > SIZE_MAX - update_carriers ||
	    sizeof(work) + update_carriers > SIZE_MAX - outer)
		return ENOBUFS;
	native_artifact_budget budget{ reserve, context, outer + sizeof(work) + update_carriers };
	if (!budget.admit(0))
		return ENOBUFS;
	try
	{
		work w;
		int status = native_artifact_load(root, lock, &w.catalog, budget);
		if (status)
			return status;
		size_t heap = w.catalog.records.capacity() * sizeof(flatfile_artifact_record);
		auto found =
			std::lower_bound(w.catalog.records.begin(), w.catalog.records.end(), vnum,
					 [](const flatfile_artifact_record &candidate,
					    int32_t sought) { return candidate.vnum < sought; });
		if (found != w.catalog.records.end() && found->vnum == vnum)
		{
			if (found->owned == owned && found->location_type == location_type &&
			    found->location == location && found->timer == timer &&
			    found->type == type && found->last_update == last_update)
			{
				*returned = true;
				*succeeded = true;
				return 0;
			}
			if (found->revision == UINT64_MAX)
				return EBADMSG;
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
			const size_t next = w.catalog.records.size() +
					    std::max(w.catalog.records.size(), size_t(1));
			if (next > SIZE_MAX / sizeof(flatfile_artifact_record))
				return ENOBUFS;
			const size_t request = next * sizeof(flatfile_artifact_record);
			if (request > SIZE_MAX - heap || !budget.admit(heap + request))
				return ENOBUFS;
			w.catalog.records.insert(found, { vnum, owned, location_type, location,
							  timer, type, last_update, 0, 0, 1 });
			heap = w.catalog.records.capacity() * sizeof(flatfile_artifact_record);
		}
		if (w.catalog.revision == UINT64_MAX)
			return EBADMSG;
		++w.catalog.revision;
		native_artifact_budget encode_budget = budget;
		if (heap > SIZE_MAX - encode_budget.base)
			return ENOBUFS;
		encode_budget.base += heap;
		status = native_artifact_encode(w.catalog, &w.bytes, encode_budget);
		if (status)
			return status;
		if (w.bytes.capacity() > SIZE_MAX - heap)
			return ENOBUFS;
		heap += w.bytes.capacity();
		if (root.size() > SIZE_MAX - 8)
			return ENOBUFS;
		const size_t path_length = root.size() + 8;
		const size_t path_request =
			path_length > 15 ? std::max(path_length, size_t(30)) + 1 : 0;
		// catalog_filename is sixteen characters: fresh GCC13 SSO15 grows to
		// thirty plus its terminator. Admit it BEFORE assign and retain its actual
		// request in every following atomic writer prefix.
		constexpr size_t name_request = 31;
		if (path_request > SIZE_MAX - name_request ||
		    path_request + name_request > SIZE_MAX - heap ||
		    !budget.admit(heap + path_request + name_request))
			return ENOBUFS;
		w.directory.reserve(path_length);
		w.directory.assign(root);
		w.directory.append("/domains");
		w.name.assign(catalog_filename);
		heap += path_request + name_request;
		const size_t atomic = flatfile_atomic_write_working_bytes();
		if (atomic > SIZE_MAX - heap || !budget.admit(heap + atomic))
			return ENOBUFS;
		// Publish and latch its actual outcome before any allocating tail. A failed
		// fsync following rename is returned=true/succeeded=false and cannot replay.
		errno = 0;
		const bool written = flatfile_atomic_write_with_publication(
			w.directory, w.name, w.bytes, nullptr, &w.published);
		const int write_error = errno;
		*returned = true;
		*succeeded = written;
		return written ? 0 : write_error ? write_error : EIO;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EOVERFLOW;
	}
}

namespace
{
size_t native_artifact_record_vector_source_frames() noexcept
{
	using records = std::vector<flatfile_artifact_record>;
	using iterator = records::iterator;
	using const_iterator = records::const_iterator;
	using difference = records::difference_type;
	// Genuine record has default member initializers: it is NOT is_trivial,
	// although its copy/move/destruction are trivial and its move is noexcept.
	// GNU13 therefore takes element-wise __relocate_a_1, not bitwise memmove.
	static_assert(!std::is_trivial_v<flatfile_artifact_record> &&
		      std::is_trivially_copyable_v<flatfile_artifact_record> &&
		      std::is_nothrow_move_constructible_v<flatfile_artifact_record>);
	return sizeof(size_t) +
	       // insert(this,position,value,n) and spare-storage pos/Temporary_value:
	       // true temporary stores vector receiver plus one complete record value.
	       3 * sizeof(void *) + sizeof(const_iterator) + sizeof(size_t) + sizeof(iterator) +
	       sizeof(void *) + sizeof(flatfile_artifact_record) +
	       // _Temporary_value constructor/destructor/value/ptr and _M_insert_aux:
	       // receiver/position/arg, forward/move/allocator construction, complete
	       // move_backward iterators/length/assignment helpers, final assignment.
	       17 * sizeof(void *) + 4 * sizeof(iterator) + 5 * sizeof(flatfile_artifact_record *) +
	       2 * sizeof(difference) + sizeof(std::random_access_iterator_tag) +
	       5 * sizeof(void *) +
	       // _M_realloc_insert receiver/position/args/len/elems_before/four pointers.
	       2 * sizeof(void *) + sizeof(iterator) + 2 * sizeof(size_t) +
	       4 * sizeof(flatfile_artifact_record *) +
	       // Original resize -> _M_default_append receiver/n/size/navail/newlen/
	       // newstart/destroy_from. No separate reserve is substituted for resize.
	       2 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(flatfile_artifact_record *) +
	       // _M_check_len/max_size/_S_max_size/size/end/min/max and allocator.
	       7 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(difference) + sizeof(iterator) +
	       // _M_allocate/alloc_traits/allocator/new_allocator/operator-new.
	       5 * sizeof(void *) + 8 * sizeof(size_t) +
	       // __uninitialized_default_n_a -> default constructor loop/value init,
	       // construct_at/allocator construct/forward/addressof and rollback ranges.
	       15 * sizeof(void *) + 4 * sizeof(flatfile_artifact_record *) + 3 * sizeof(size_t) +
	       sizeof(bool) +
	       // _S_relocate/__relocate_a/__relocate_a_1: actual generic first/last/
	       // result/allocator, current result pointer and per-element original
	       // __relocate_object_a dest/original/allocator -> construct/destroy.
	       12 * sizeof(void *) + sizeof(flatfile_artifact_record *) + 3 * sizeof(void *) +
	       9 * sizeof(void *) +
	       // Both genuine vector destruction/move assignment and exception cleanup
	       // use allocator-aware _Destroy (trivial destructor), pointer ranges,
	       // _M_deallocate/alloc_traits/allocator/new_allocator and sized delete.
	       8 * sizeof(void *) + 4 * sizeof(flatfile_artifact_record *) + 2 * sizeof(iterator) +
	       3 * sizeof(size_t) + 8 * sizeof(void *) + 5 * sizeof(size_t) +
	       // vector move assignment _M_move_assign receiver/value/true tag and
	       // actual empty temporary vector, allocator exchange and data swap scopes.
	       sizeof(records) + 13 * sizeof(void *) + sizeof(std::true_type);
}
size_t native_artifact_validation_source_frames() noexcept
{
	using records = std::vector<flatfile_artifact_record>;
	using iterator = records::const_iterator;
	using comparator =
		bool (*)(const flatfile_artifact_record &, const flatfile_artifact_record &);
	using adapted = __gnu_cxx::__ops::_Iter_comp_iter<comparator>;
	static_assert(std::is_same_v<iterator, __gnu_cxx::__normal_iterator<
						       const flatfile_artifact_record *, records>>);
	// Exact valid_records -> is_sorted -> is_sorted_until -> __is_sorted_until
	// chain. There is one iterative next iterator, no recursion/count estimate.
	return sizeof(size_t) +
	       // valid_records records reference/index and its scalar return.
	       sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	       // is_sorted first,last,function pointer,bool result; is_sorted_until
	       // first,last,function pointer,iterator result; internal first,last,
	       // adapted comparator,next and iterator result.
	       9 * sizeof(iterator) + 2 * sizeof(comparator) + sizeof(adapted) + sizeof(bool) +
	       // __iter_comp_iter argument/result; _Iter_comp_iter constructor this/
	       // comparator, std::move input/result; operator() this/two iterators/bool.
	       4 * sizeof(comparator) + sizeof(adapted) + 4 * sizeof(void *) +
	       2 * sizeof(iterator) + sizeof(bool) +
	       // begin/end const receivers and iterator pointer/ref constructor scopes.
	       6 * sizeof(void *) + 2 * sizeof(iterator) +
	       // iterator equality references/result, prefix increment this/ref result,
	       // and both comparator dereferences this/reference result.
	       8 * sizeof(void *) + sizeof(bool) +
	       // record_less two const references/bool; valid_record const ref/bool.
	       3 * sizeof(void *) + 2 * sizeof(bool) +
	       // vector::size receiver/result, both operator[] receiver/index/reference
	       // and their bounds-check macro (disabled by the accepted nondebug build).
	       5 * sizeof(void *) + 3 * sizeof(size_t);
}
size_t native_artifact_byte_vector_source_frames() noexcept
{
	using bytes = std::vector<uint8_t>;
	using iterator = bytes::iterator;
	using pointer = bytes::pointer;
	using difference = bytes::difference_type;
	static_assert(std::is_trivial_v<uint8_t> &&
		      std::is_same_v<bytes::allocator_type, std::allocator<uint8_t>> &&
		      std::is_same_v<pointer, uint8_t *>);
	// Genuine byte vectors differ from record vectors: GNU13 selects bitwise
	// runtime relocation and trivial default fill. The two allocations themselves
	// remain owned by native_artifact_encode's payload_size/file_size admission.
	return sizeof(size_t) +
	       // reserve this/n/old_size/tmp; size/capacity/max_size/_S_max_size,
	       // allocator reference, ptrdiff maximum and std::min references/result.
	       10 * sizeof(void *) + 9 * sizeof(size_t) + sizeof(difference) +
	       // _M_allocate -> allocator_traits::allocate -> allocator::allocate ->
	       // new_allocator::allocate: actual receivers/count/hint/max checks,
	       // requested operator-new size and pointer return carriers.
	       10 * sizeof(void *) + 8 * sizeof(size_t) +
	       // _S_relocate -> __relocate_a -> __relocate_a_1 byte specialization:
	       // three pointer args/return and allocator ref per layer, genuine count;
	       // three __niter_base pointer args/results; memmove dest/src/byte count.
	       21 * sizeof(void *) + sizeof(difference) + sizeof(size_t) +
	       // resize this/new_size; _M_default_append this/n/size/navail. Here both
	       // actual local vectors start empty and reserve has returned before resize,
	       // so navail>=n; the allocating append branch is source-unreachable.
	       2 * sizeof(void *) + 4 * sizeof(size_t) +
	       // allocator-specialized __uninitialized_default_n_a -> default_n ->
	       // default_n_1<true>: first/count/allocator, first/count, first/count/
	       // genuine val pointer; addressof and runtime _Construct/placement-new.
	       // The constexpr-only construct_at branch is not reached at runtime.
	       12 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) +
	       // std::fill_n->__fill_n_a(random_access)->__fill_a->__fill_a1(byte):
	       // first/count/value/result, category object, first/last/value, true byte
	       // temporary/length and actual memset pointer/int/size call carriers.
	       12 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(difference) +
	       sizeof(std::random_access_iterator_tag) + sizeof(uint8_t) + sizeof(int) +
	       // fill_n's actual __size_to_integer argument/result and
	       // __iterator_category reference/result before the random-access branch.
	       2 * sizeof(size_t) + sizeof(void *) + sizeof(std::random_access_iterator_tag) +
	       // vector::swap this/value; _M_swap_data this/value/local impl_data,
	       // three _M_copy_data(this/value) calls; allocator getter refs,
	       // _S_on_swap->__alloc_on_swap(two refs), C++20 constexpr false
	       // propagation branch: no run-time false_type argument exists. Includes
	       // the real default impl_data constructor and equal-allocator operands.
	       3 * sizeof(pointer) + 19 * sizeof(void *) + sizeof(bool) +
	       // Both success and exception work destruction: vector/base/impl this,
	       // allocator-aware _Destroy ranges/allocator (trivial leaf), _M_deallocate
	       // pointer/count, allocator_traits/allocator/new_allocator deallocate and
	       // actual sized-delete pointer/count. No byte destructor callback.
	       17 * sizeof(void *) + 5 * sizeof(size_t) +
	       // Actual byte begin/end normal-iterator ctor, equality/increment/deref,
	       // size/data/operator[] receivers/indices/results (number/copy loops),
	       // digest array begin/end/data/operator[] scopes. Genuine GCC13 array
	       // returns its actual _M_elems directly; it has no _S_ref/_S_ptr helpers.
	       6 * sizeof(iterator) + 27 * sizeof(void *) + 5 * sizeof(size_t) + sizeof(bool) +
	       // Largest native_artifact_number<T> vector/offset references, value,
	       // unsigned bits and index; includes real uint8_t output conversion.
	       2 * sizeof(void *) + 2 * sizeof(uint64_t) + sizeof(size_t) + sizeof(uint8_t);
}
size_t native_artifact_error_heap(const std::string *error) noexcept
{
	return error && error->capacity() > 15 ? error->capacity() + 1 : 0;
}
bool native_artifact_refresh_error(native_artifact_budget &budget, size_t &owned,
				   const std::string *error) noexcept
{
	const size_t actual = native_artifact_error_heap(error);
	if (owned > budget.base || actual > SIZE_MAX - (budget.base - owned))
		return false;
	budget.base = budget.base - owned + actual;
	owned = actual;
	return true;
}
int native_artifact_corrupt_with_error(std::string *error, bool *completed,
				       native_artifact_budget &budget, size_t &owned,
				       size_t retained) noexcept
{
	// Preserve load_catalog's original conditional literal assignment. True
	// current error capacity is already in budget; the growth request is added
	// while old storage is still live, BEFORE the original assignment.
	constexpr size_t own_frames = 5 * sizeof(void *) + 4 * sizeof(size_t) + 7 * sizeof(void *) +
				      3 * sizeof(size_t) + sizeof(bool);
	const size_t string_frames = flatfile_diagnostic_string_source_frame_bytes();
	constexpr size_t query_frames = 6 * sizeof(void *) + 4 * sizeof(size_t);
	if (string_frames > SIZE_MAX - own_frames - query_frames)
		return ENOBUFS;
	const size_t frames = own_frames + query_frames + string_frames;
	const size_t length = sizeof("artifact catalog is corrupt") - 1;
	size_t request = 0;
	if (error && error->empty() && length > error->capacity())
	{
		if (error->capacity() > (SIZE_MAX - 1) / 2)
			return ENOBUFS;
		request = std::max(length, error->capacity() * 2) + 1;
	}
	if (request > SIZE_MAX - frames || retained > SIZE_MAX - frames - request ||
	    !budget.admit(retained + frames + request))
		return ENOBUFS;
	try
	{
		if (error && error->empty())
			*error = "artifact catalog is corrupt";
		if (!native_artifact_refresh_error(budget, owned, error))
			return ENOBUFS;
		*completed = true;
		return EBADMSG;
	}
	catch (...)
	{
		(void)native_artifact_refresh_error(budget, owned, error);
		return ENOMEM;
	}
}
int native_artifact_load_with_error(const std::string &root, const flatfile_authority_lock &lock,
				    artifact_catalog *catalog, std::string *error, bool *completed,
				    native_artifact_budget budget) noexcept
{
	if (root.empty() || !lock.matches(root) || !catalog || !completed)
		return EINVAL;
	// Actual load arguments (four references/pointers) and recovered result
	// coexist with the nested original recovery; no working struct substitutes
	// for these source-declared carriers.
	constexpr size_t recovery_carriers =
		4 * sizeof(void *) + sizeof(flatfile_authority_transaction_result);
	if (recovery_carriers > SIZE_MAX - budget.base)
		return ENOBUFS;
	// Actual error heap is already in the caller outer. Refresh after each
	// genuine error-capable child; never replace its concrete text with errno.
	size_t error_heap = native_artifact_error_heap(error);
	bool recovery_started = false, recovery_returned = false, read_returned = false;
	constexpr size_t own_error_frames = sizeof(native_artifact_budget) + 3 * sizeof(bool) +
					    sizeof(size_t) + 4 * sizeof(void *) +
					    3 * sizeof(size_t) + sizeof(bool);
	const size_t string_frames = flatfile_diagnostic_string_source_frame_bytes();
	constexpr size_t query_frames = 6 * sizeof(void *) + 10 * sizeof(size_t);
	if (string_frames > SIZE_MAX - own_error_frames - query_frames)
		return ENOBUFS;
	const size_t vector_frames = native_artifact_record_vector_source_frames();
	if (vector_frames > SIZE_MAX - own_error_frames - query_frames - string_frames)
		return ENOBUFS;
	const size_t validation_frames = native_artifact_validation_source_frames();
	if (validation_frames >
	    SIZE_MAX - own_error_frames - query_frames - string_frames - vector_frames)
		return ENOBUFS;
	// The real loaded byte vector also has an original cleanup interval.
	const size_t byte_frames = native_artifact_byte_vector_source_frames();
	if (byte_frames > SIZE_MAX - own_error_frames - query_frames - string_frames -
				  vector_frames - validation_frames)
		return ENOBUFS;
	const size_t error_frames = own_error_frames + query_frames + string_frames +
				    vector_frames + validation_frames + byte_frames;
	if (error_frames > SIZE_MAX - budget.base)
		return ENOBUFS;
	budget.base += error_frames;
	const auto recovered = flatfile_authority_transaction_recover_with_error_bounded(
		root, lock, error, &recovery_started, &recovery_returned, budget.reserve,
		budget.context, budget.base + recovery_carriers);
	if (!native_artifact_refresh_error(budget, error_heap, error))
		return ENOBUFS;
	if (!recovery_returned)
		return errno ? errno : ENOBUFS;
	if (recovered != flatfile_authority_transaction_result::ok)
	{
		*completed = true;
		return recovered == flatfile_authority_transaction_result::io_error ? EIO : EBADMSG;
	}
	struct work
	{
		std::string directory;
		std::string name;
		std::vector<uint8_t> bytes;
		artifact_catalog candidate;
		std::array<uint8_t, SHA256_DIGEST_LENGTH> digest{};
		uint32_t version = 0, payload_size = 0, count = 0;
		uint64_t revision = 0;
	};
	if (root.size() > SIZE_MAX - 8)
		return ENOBUFS;
	const size_t directory_size = root.size() + 8;
	// Empty GCC13 string assign allocates max(requested,2*15)+1 beyond SSO.
	const size_t directory_request =
		directory_size > 15 ? std::max(directory_size, size_t(30)) + 1 : 0;
	constexpr size_t name_request = sizeof("artifact_catalog") - 1 > 15 ? 31 : 0;
	// Four argument carriers; recovered/loaded results; directory_size,
	// directory_request, retained, directory_actual and record_request; both
	// actual decoder objects, payload pointer and hash return; real range
	// reference/begin/end/record and owned byte. The decoder::number largest
	// instantiation contributes this/value pointers, uint64 bits/index/bool.
	constexpr size_t load_carriers =
		4 * sizeof(void *) + sizeof(flatfile_authority_transaction_result) +
		sizeof(flatfile_read_result) + 5 * sizeof(size_t) + 2 * sizeof(decoder) +
		sizeof(const uint8_t *) + sizeof(int) + 2 * sizeof(void *) +
		2 * sizeof(std::vector<flatfile_artifact_record>::iterator) + sizeof(uint8_t) +
		2 * sizeof(void *) + sizeof(uint64_t) + sizeof(size_t) + sizeof(bool);
	size_t retained = sizeof(work) + load_carriers;
	if (directory_request > SIZE_MAX - retained ||
	    name_request > SIZE_MAX - retained - directory_request)
		return ENOBUFS;
	retained += directory_request + name_request;
	if (!budget.admit(retained))
		return ENOBUFS;
	try
	{
		work w;
		w.directory.reserve(directory_size);
		w.directory.assign(root);
		w.directory.append("/domains");
		w.name.assign(catalog_filename);
		const size_t directory_actual =
			w.directory.capacity() > 15 ? w.directory.capacity() + 1 : 0;
		if (directory_actual > directory_request)
			return EOVERFLOW;
		const auto loaded = flatfile_read_with_error_bounded(
			w.directory, w.name, catalog_maximum_bytes, &w.bytes, error, &read_returned,
			budget.reserve, budget.context, budget.base + retained);
		if (!native_artifact_refresh_error(budget, error_heap, error))
			return ENOBUFS;
		if (!read_returned)
			return errno ? errno : ENOBUFS;
		if (loaded == flatfile_read_result::not_found)
		{
			*completed = true;
			return ENOENT;
		}
		if (loaded == flatfile_read_result::io_error)
		{
			*completed = true;
			return EIO;
		}
		if (w.bytes.capacity() > SIZE_MAX - retained)
			return ENOBUFS;
		retained += w.bytes.capacity();
		if (loaded != flatfile_read_result::ok)
			return native_artifact_corrupt_with_error(error, completed, budget,
								  error_heap, retained);
		constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
		if (w.bytes.size() < header_size || memcmp(w.bytes.data(), catalog_magic.data(), 8))
			return native_artifact_corrupt_with_error(error, completed, budget,
								  error_heap, retained);
		decoder header{ w.bytes.data() + 8, w.bytes.size() - 8 };
		if (!header.number(&w.version) || !header.number(&w.payload_size) ||
		    !header.number(&w.revision) || w.version != catalog_version || !w.revision ||
		    w.payload_size != w.bytes.size() - header_size)
			return native_artifact_corrupt_with_error(error, completed, budget,
								  error_heap, retained);
		const uint8_t *payload_bytes = w.bytes.data() + header_size;
		const int hashed = native_artifact_hash(payload_bytes, w.payload_size,
							w.digest.data(), budget, retained);
		if (hashed)
			return hashed;
		if (CRYPTO_memcmp(w.bytes.data() + 24, w.digest.data(), w.digest.size()))
			return native_artifact_corrupt_with_error(error, completed, budget,
								  error_heap, retained);
		decoder payload{ payload_bytes, w.payload_size };
		if (!payload.number(&w.count) || w.count > record_maximum)
			return native_artifact_corrupt_with_error(error, completed, budget,
								  error_heap, retained);
		const size_t record_request = size_t(w.count) * sizeof(flatfile_artifact_record);
		if (record_request > SIZE_MAX - retained ||
		    !budget.admit(retained + record_request))
			return ENOBUFS;
		w.candidate.revision = w.revision;
		try
		{
			w.candidate.records.resize(w.count);
		}
		catch (const std::bad_alloc &)
		{
			if (w.candidate.records.capacity() >
			    (SIZE_MAX - retained) / sizeof(flatfile_artifact_record))
				return ENOBUFS;
			return native_artifact_corrupt_with_error(
				error, completed, budget, error_heap,
				retained + w.candidate.records.capacity() *
						   sizeof(flatfile_artifact_record));
		}
		if (w.candidate.records.capacity() >
		    (SIZE_MAX - retained) / sizeof(flatfile_artifact_record))
			return ENOBUFS;
		retained += w.candidate.records.capacity() * sizeof(flatfile_artifact_record);
		for (auto &record : w.candidate.records)
		{
			uint8_t owned = 0;
			if (!payload.number(&record.vnum) || !payload.number(&owned) || owned > 1 ||
			    !payload.number(&record.location_type) ||
			    !payload.number(&record.location) || !payload.number(&record.timer) ||
			    !payload.number(&record.type) || !payload.number(&record.last_update) ||
			    !payload.number(&record.bind_owner_pid) ||
			    !payload.number(&record.bind_timer) ||
			    !payload.number(&record.revision))
				return native_artifact_corrupt_with_error(error, completed, budget,
									  error_heap, retained);
			record.owned = owned != 0;
		}
		if (payload.offset != payload.size || !valid_records(w.candidate.records))
			return native_artifact_corrupt_with_error(error, completed, budget,
								  error_heap, retained);
		*catalog = std::move(w.candidate);
		*completed = true;
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EOVERFLOW;
	}
}
} // namespace
// Complete original remove-owned catalog mutation under the caller's genuine
// locally acquired authority lock. Diagnostic text capture remains a separate
// source obligation; this errno-style leaf does not replace original methods.
int flatfile_artifact_remove_owned_with_error_bounded(
	const std::string &root, const flatfile_authority_lock &lock, int32_t vnum,
	int32_t corpse_pid, int32_t type, int64_t last_update, std::string *error, bool *returned,
	bool *succeeded, flatfile_scratch_reserve_fn reserve, void *context, size_t outer) noexcept
{
	if (!returned || !succeeded || !reserve || returned == succeeded)
		return EINVAL;
	if (*returned || *succeeded)
		return EALREADY;
	if (!native_artifact_policy())
		return ENOTSUP;
	if (root.empty() || vnum <= 0 || type < 1 || type > 3 || last_update < 0)
	{
		*returned = true;
		*succeeded = false;
		return EINVAL; // Original semantic terminal, never a budget interruption.
	}
	struct work
	{
		artifact_catalog catalog;
		std::vector<uint8_t> bytes;
		std::string directory, name;
		bool published = false, loaded_returned = false, write_returned = false;
	};
	// Actual call arguments, both live budgets, found iterator, complete inserted
	// record, lambda/range/move carriers, status/errno, capacity/path counters.
	// The authentic caller-owned lock/root storage is exclusively in outer.
	constexpr size_t own_carriers =
		8 * sizeof(void *) + 3 * sizeof(int32_t) + sizeof(int64_t) + sizeof(size_t) +
		2 * sizeof(native_artifact_budget) +
		sizeof(std::vector<flatfile_artifact_record>::iterator) +
		sizeof(flatfile_artifact_record) + 5 * sizeof(bool) + 2 * sizeof(int32_t) +
		4 * sizeof(int) + 9 * sizeof(size_t) + sizeof(void *) + sizeof(int32_t) +
		sizeof(bool) + 6 * sizeof(void *) + 2 * sizeof(flatfile_artifact_record) +
		// Actual lower_bound normal-iterator/value/comparator wrapper,
		// __lower_bound first/last/value/comp/len/half/middle, distance and
		// random-access advance/iterator arithmetic/dereference source scopes.
		10 * sizeof(std::vector<flatfile_artifact_record>::iterator) + 11 * sizeof(void *) +
		3 * sizeof(std::ptrdiff_t) + 2 * sizeof(std::random_access_iterator_tag) +
		2 * sizeof(bool);
	const size_t string_frames = flatfile_diagnostic_string_source_frame_bytes();
	constexpr size_t query_frames = 6 * sizeof(void *) + 10 * sizeof(size_t);
	if (string_frames > SIZE_MAX - own_carriers - query_frames)
		return ENOBUFS;
	const size_t vector_frames = native_artifact_record_vector_source_frames();
	if (vector_frames > SIZE_MAX - own_carriers - query_frames - string_frames)
		return ENOBUFS;
	const size_t validation_frames = native_artifact_validation_source_frames();
	if (validation_frames >
	    SIZE_MAX - own_carriers - query_frames - string_frames - vector_frames)
		return ENOBUFS;
	const size_t byte_frames = native_artifact_byte_vector_source_frames();
	if (byte_frames > SIZE_MAX - own_carriers - query_frames - string_frames - vector_frames -
				  validation_frames)
		return ENOBUFS;
	const size_t carriers = own_carriers + query_frames + string_frames + vector_frames +
				validation_frames + byte_frames;
	if (sizeof(work) > SIZE_MAX - carriers || sizeof(work) + carriers > SIZE_MAX - outer)
		return ENOBUFS;
	native_artifact_budget budget{ reserve, context, outer + sizeof(work) + carriers };
	if (!budget.admit(0))
		return ENOBUFS;
	try
	{
		work w;
		size_t error_owned = native_artifact_error_heap(error);
		int status = native_artifact_load_with_error(root, lock, &w.catalog, error,
							     &w.loaded_returned, budget);
		if (!native_artifact_refresh_error(budget, error_owned, error))
			return ENOBUFS;
		if (status)
		{
			if (w.loaded_returned)
			{
				*returned = true;
				*succeeded = false;
			}
			return status;
		}
		size_t heap = w.catalog.records.capacity() * sizeof(flatfile_artifact_record);
		auto found =
			std::lower_bound(w.catalog.records.begin(), w.catalog.records.end(), vnum,
					 [](const flatfile_artifact_record &candidate,
					    int32_t sought) { return candidate.vnum < sought; });
		if (found == w.catalog.records.end() || found->vnum != vnum)
		{
			if (corpse_pid <= 0)
			{
				*returned = true;
				*succeeded = true;
				return 0;
			}
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
			// Installed GCC13 _M_check_len(1) is size + max(size,1). Its old
			// allocation remains live while the complete new vector is requested.
			const size_t size = w.catalog.records.size();
			if (size > SIZE_MAX - std::max(size, size_t(1)))
				return ENOBUFS;
			const size_t next = size + std::max(size, size_t(1));
			if (next > SIZE_MAX / sizeof(flatfile_artifact_record))
				return ENOBUFS;
			const size_t request = w.catalog.records.size() ==
							       w.catalog.records.capacity() ?
						       next * sizeof(flatfile_artifact_record) :
						       0;
			if (request > SIZE_MAX - heap || !budget.admit(heap + request))
				return ENOBUFS;
			try
			{
				w.catalog.records.insert(found, inserted);
			}
			catch (const std::bad_alloc &)
			{
				// Original remove-owned catches this exact insertion failure.
				*returned = true;
				*succeeded = false;
				return ENOMEM;
			}
			heap = w.catalog.records.capacity() * sizeof(flatfile_artifact_record);
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
			{
				*returned = true;
				*succeeded = true;
				return 0;
			}
			if (found->revision == UINT64_MAX)
			{
				*returned = true;
				*succeeded = false;
				return EBADMSG;
			}
			found->owned = on_corpse;
			found->location_type = location_type;
			found->location = location;
			found->last_update = last_update;
			found->bind_owner_pid = -1;
			found->bind_timer = 0;
			++found->revision;
		}
		if (w.catalog.revision == UINT64_MAX)
		{
			*returned = true;
			*succeeded = false;
			return EBADMSG;
		}
		++w.catalog.revision;
		native_artifact_budget encode_budget = budget;
		if (heap > SIZE_MAX - encode_budget.base)
			return ENOBUFS;
		encode_budget.base += heap;
		// Genuine validation runs before encode's own allocation admission.
		// Prospectively admit the caller-held validation/byte source profiles
		// plus actual catalog storage BEFORE entering that original helper.
		if (!encode_budget.admit(0))
			return ENOBUFS;
		status = native_artifact_encode(w.catalog, &w.bytes, encode_budget);
		if (status)
		{
			// Original encode_catalog catches allocation failure into its
			// invalid result, while explicit admission refusal is unfinished.
			if (status == EBADMSG || status == ENOMEM)
			{
				*returned = true;
				*succeeded = false;
			}
			return status;
		}
		if (w.bytes.capacity() > SIZE_MAX - heap || root.size() > SIZE_MAX - 8)
			return ENOBUFS;
		heap += w.bytes.capacity();
		const size_t path_length = root.size() + 8;
		const size_t path_request =
			path_length > 15 ? std::max(path_length, size_t(30)) + 1 : 0;
		constexpr size_t name_request = 31;
		if (path_request > SIZE_MAX - name_request ||
		    path_request + name_request > SIZE_MAX - heap ||
		    !budget.admit(heap + path_request + name_request))
			return ENOBUFS;
		w.directory.reserve(path_length);
		w.directory.assign(root);
		w.directory.append("/domains");
		w.name.assign(catalog_filename);
		// Retain actual live capacities for the writer, not just prospective
		// empty-string requests. Both were admitted before reserve/assign.
		const size_t directory_actual =
			w.directory.capacity() > 15 ? w.directory.capacity() + 1 : 0;
		const size_t name_actual = w.name.capacity() > 15 ? w.name.capacity() + 1 : 0;
		if (directory_actual > path_request || name_actual > name_request ||
		    directory_actual > SIZE_MAX - heap ||
		    name_actual > SIZE_MAX - heap - directory_actual)
			return EOVERFLOW;
		heap += directory_actual + name_actual;
		const size_t atomic = flatfile_atomic_write_working_bytes();
		if (atomic > SIZE_MAX - heap || !budget.admit(heap + atomic))
			return ENOBUFS;
		errno = 0;
		const bool written = flatfile_atomic_write_with_error_bounded(
			w.directory, w.name, w.bytes, error, &w.published, &w.write_returned,
			reserve, context, budget.base + heap);
		const int write_error = errno;
		if (!native_artifact_refresh_error(budget, error_owned, error))
			return ENOBUFS;
		if (!w.write_returned)
			return write_error ? write_error : ENOBUFS;
		*returned = true;
		*succeeded = written;
		return written ? 0 : write_error ? write_error : EIO;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EOVERFLOW;
	}
}
