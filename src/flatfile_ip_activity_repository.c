#include "flatfile_ip_activity_repository.h"

#include "defines.h"
#include "flatfile_store.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cstdint>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <string>
#include <vector>

namespace
{
constexpr std::array<uint8_t, 8> activity_magic = { 'D', 'U', 'R', 'I', 'P', 'A', 'C', 'T' };
constexpr uint32_t activity_version = 1;
constexpr size_t activity_maximum_entries = 250000;
constexpr size_t activity_ip_maximum = 253;
constexpr size_t activity_maximum_bytes = 64 * 1024 * 1024;
constexpr size_t activity_header_size =
	activity_magic.size() + sizeof(uint32_t) * 2 + SHA256_DIGEST_LENGTH;
constexpr const char *activity_filename = "ip_activity";
constexpr const char *activity_lock_filename = ".ip-activity.lock";

enum class load_result
{
	ok,
	not_found,
	corrupt,
	io_error
};

void append_u32(std::vector<uint8_t> *bytes, uint32_t value)
{
	for (size_t offset = 0; offset < sizeof(value); ++offset)
	{
		bytes->push_back(static_cast<uint8_t>(value & 0xff));
		value >>= 8;
	}
}

void append_u64(std::vector<uint8_t> *bytes, uint64_t value)
{
	for (size_t offset = 0; offset < sizeof(value); ++offset)
	{
		bytes->push_back(static_cast<uint8_t>(value & 0xff));
		value >>= 8;
	}
}

bool read_u32(const uint8_t *data, size_t size, size_t *offset, uint32_t *value)
{
	if (!data || !offset || !value || *offset > size || size - *offset < sizeof(*value))
		return false;
	uint32_t decoded = 0;
	for (size_t index = 0; index < sizeof(decoded); ++index)
		decoded |= static_cast<uint32_t>(data[(*offset)++]) << (index * 8);
	*value = decoded;
	return true;
}

bool read_u64(const uint8_t *data, size_t size, size_t *offset, uint64_t *value)
{
	if (!data || !offset || !value || *offset > size || size - *offset < sizeof(*value))
		return false;
	uint64_t decoded = 0;
	for (size_t index = 0; index < sizeof(decoded); ++index)
		decoded |= static_cast<uint64_t>(data[(*offset)++]) << (index * 8);
	*value = decoded;
	return true;
}

bool valid_ip(const std::string &ip, bool allow_empty)
{
	if (ip.empty())
		return allow_empty;
	if (ip.size() > activity_ip_maximum)
		return false;
	for (const unsigned char byte : ip)
		if (byte < 0x21 || byte > 0x7e)
			return false;
	return true;
}

bool valid_record(const flatfile_ip_activity_record &record)
{
	return record.pid > 0 && record.last_connect >= 0 && record.last_disconnect >= 0 &&
	       record.racewar_side >= RACEWAR_NONE && record.racewar_side <= MAX_RACEWAR &&
	       valid_ip(record.ip, record.last_connect == 0);
}

bool valid_records(const std::vector<flatfile_ip_activity_record> &records)
{
	if (records.size() > activity_maximum_entries)
		return false;
	uint32_t previous_pid = 0;
	for (const auto &record : records)
	{
		if (!valid_record(record) || record.pid <= previous_pid)
			return false;
		previous_pid = record.pid;
	}
	return true;
}

bool encode_records(const std::vector<flatfile_ip_activity_record> &records,
		    std::vector<uint8_t> *bytes)
{
	if (!bytes || !valid_records(records))
		return false;
	try
	{
		std::vector<uint8_t> payload;
		payload.reserve(sizeof(uint32_t) + records.size() * 48);
		append_u32(&payload, static_cast<uint32_t>(records.size()));
		for (const auto &record : records)
		{
			append_u32(&payload, record.pid);
			append_u64(&payload, static_cast<uint64_t>(record.last_connect));
			append_u64(&payload, static_cast<uint64_t>(record.last_disconnect));
			append_u32(&payload, static_cast<uint32_t>(record.racewar_side));
			append_u32(&payload, static_cast<uint32_t>(record.ip.size()));
			payload.insert(payload.end(), record.ip.begin(), record.ip.end());
		}
		if (payload.size() > UINT32_MAX ||
		    payload.size() > activity_maximum_bytes - activity_header_size)
			return false;
		std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
		SHA256(payload.data(), payload.size(), digest.data());
		bytes->clear();
		bytes->reserve(activity_header_size + payload.size());
		bytes->insert(bytes->end(), activity_magic.begin(), activity_magic.end());
		append_u32(bytes, activity_version);
		append_u32(bytes, static_cast<uint32_t>(payload.size()));
		bytes->insert(bytes->end(), digest.begin(), digest.end());
		bytes->insert(bytes->end(), payload.begin(), payload.end());
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool decode_records(const std::vector<uint8_t> &bytes,
		    std::vector<flatfile_ip_activity_record> *records)
{
	if (!records || bytes.size() < activity_header_size ||
	    !std::equal(activity_magic.begin(), activity_magic.end(), bytes.begin()))
		return false;
	size_t header_offset = activity_magic.size();
	uint32_t version = 0, payload_size = 0;
	if (!read_u32(bytes.data(), bytes.size(), &header_offset, &version) ||
	    !read_u32(bytes.data(), bytes.size(), &header_offset, &payload_size) ||
	    version != activity_version || payload_size != bytes.size() - activity_header_size)
		return false;
	const uint8_t *expected_digest = bytes.data() + header_offset;
	header_offset += SHA256_DIGEST_LENGTH;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(bytes.data() + header_offset, payload_size, digest.data());
	if (CRYPTO_memcmp(expected_digest, digest.data(), digest.size()))
		return false;

	const uint8_t *payload = bytes.data() + header_offset;
	size_t offset = 0;
	uint32_t count = 0;
	if (!read_u32(payload, payload_size, &offset, &count) || count > activity_maximum_entries)
		return false;
	std::vector<flatfile_ip_activity_record> decoded;
	try
	{
		decoded.reserve(count);
		for (uint32_t index = 0; index < count; ++index)
		{
			uint32_t pid = 0, racewar_side = 0, ip_size = 0;
			uint64_t last_connect = 0, last_disconnect = 0;
			if (!read_u32(payload, payload_size, &offset, &pid) ||
			    !read_u64(payload, payload_size, &offset, &last_connect) ||
			    !read_u64(payload, payload_size, &offset, &last_disconnect) ||
			    !read_u32(payload, payload_size, &offset, &racewar_side) ||
			    !read_u32(payload, payload_size, &offset, &ip_size) ||
			    last_connect > INT64_MAX || last_disconnect > INT64_MAX ||
			    racewar_side > INT_MAX || ip_size > activity_ip_maximum ||
			    offset > payload_size || payload_size - offset < ip_size)
				return false;
			flatfile_ip_activity_record record;
			record.pid = pid;
			record.last_connect = static_cast<int64_t>(last_connect);
			record.last_disconnect = static_cast<int64_t>(last_disconnect);
			record.racewar_side = static_cast<int>(racewar_side);
			record.ip.assign(reinterpret_cast<const char *>(payload + offset), ip_size);
			offset += ip_size;
			decoded.push_back(std::move(record));
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (offset != payload_size || !valid_records(decoded))
		return false;
	*records = std::move(decoded);
	return true;
}

std::string activity_directory(const char *root)
{
	return root && *root ? std::string(root) + "/metadata" : std::string();
}

load_result load_records(const std::string &directory,
			 std::vector<flatfile_ip_activity_record> *records, std::string *error)
{
	if (directory.empty() || !records)
	{
		if (error)
			*error = "flat-file state root is unavailable";
		return load_result::io_error;
	}
	std::vector<uint8_t> bytes;
	const auto loaded =
		flatfile_read(directory, activity_filename, activity_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return load_result::not_found;
	if (loaded != flatfile_read_result::ok)
		return loaded == flatfile_read_result::io_error ? load_result::io_error :
								  load_result::corrupt;
	if (!decode_records(bytes, records))
	{
		if (error)
			*error = "IP activity record is corrupt";
		return load_result::corrupt;
	}
	return load_result::ok;
}

bool save_records(const std::string &directory,
		  const std::vector<flatfile_ip_activity_record> &records, std::string *error)
{
	std::vector<uint8_t> bytes;
	if (!encode_records(records, &bytes))
	{
		if (error)
			*error = "IP activity values exceed format limits";
		return false;
	}
	return flatfile_atomic_write(directory, activity_filename, bytes, error);
}

flatfile_ip_activity_result public_result(load_result result)
{
	switch (result)
	{
	case load_result::ok:
		return flatfile_ip_activity_result::ok;
	case load_result::not_found:
		return flatfile_ip_activity_result::not_found;
	case load_result::corrupt:
		return flatfile_ip_activity_result::corrupt;
	case load_result::io_error:
		return flatfile_ip_activity_result::io_error;
	}
	return flatfile_ip_activity_result::io_error;
}

template <typename Mutation>
flatfile_ip_activity_result mutate_records(const char *root, Mutation mutation, std::string *error)
{
	const std::string directory = activity_directory(root);
	if (directory.empty())
		return flatfile_ip_activity_result::invalid;
	int lock_fd = -1;
	if (!flatfile_lock_acquire(directory, activity_lock_filename, &lock_fd, error))
		return flatfile_ip_activity_result::io_error;
	std::vector<flatfile_ip_activity_record> records;
	const auto loaded = load_records(directory, &records, error);
	if (loaded == load_result::corrupt || loaded == load_result::io_error)
	{
		flatfile_lock_release(lock_fd);
		return public_result(loaded);
	}
	bool saved = false;
	try
	{
		const bool changed = mutation(&records);
		saved = !changed || save_records(directory, records, error);
	}
	catch (const std::bad_alloc &)
	{
		if (error)
			*error = "IP activity allocation failed";
	}
	flatfile_lock_release(lock_fd);
	return saved ? flatfile_ip_activity_result::ok : flatfile_ip_activity_result::io_error;
}
} // namespace

flatfile_ip_activity_result flatfile_ip_activity_connect(const char *root, uint32_t pid,
							 const char *ip, int racewar_side,
							 int64_t occurred_at, std::string *error)
{
	const std::string saved_ip = ip ? ip : "";
	if (!pid || occurred_at <= 0 || racewar_side < RACEWAR_NONE || racewar_side > MAX_RACEWAR ||
	    !valid_ip(saved_ip, false))
	{
		if (error)
			*error = "invalid IP connection activity";
		return flatfile_ip_activity_result::invalid;
	}
	return mutate_records(
		root,
		[&](auto *records)
		{
			auto found = std::lower_bound(records->begin(), records->end(), pid,
						      [](const auto &record, uint32_t wanted)
						      { return record.pid < wanted; });
			if (found == records->end() || found->pid != pid)
			{
				flatfile_ip_activity_record added;
				added.pid = pid;
				found = records->insert(found, std::move(added));
			}
			found->ip = saved_ip;
			found->last_connect = occurred_at;
			found->racewar_side = racewar_side;
			return true;
		},
		error);
}

flatfile_ip_activity_result flatfile_ip_activity_disconnect(const char *root, uint32_t pid,
							    int racewar_side, int64_t occurred_at,
							    std::string *error)
{
	if (!pid || occurred_at <= 0 || racewar_side < RACEWAR_NONE || racewar_side > MAX_RACEWAR)
	{
		if (error)
			*error = "invalid IP disconnect activity";
		return flatfile_ip_activity_result::invalid;
	}
	return mutate_records(
		root,
		[&](auto *records)
		{
			auto found = std::lower_bound(records->begin(), records->end(), pid,
						      [](const auto &record, uint32_t wanted)
						      { return record.pid < wanted; });
			if (found == records->end() || found->pid != pid)
			{
				flatfile_ip_activity_record added;
				added.pid = pid;
				found = records->insert(found, std::move(added));
			}
			found->last_disconnect = occurred_at;
			found->racewar_side = racewar_side;
			return true;
		},
		error);
}

flatfile_ip_activity_result flatfile_ip_activity_get(const char *root, uint32_t pid,
						     flatfile_ip_activity_record *record,
						     std::string *error)
{
	if (!pid || !record)
	{
		if (error)
			*error = "invalid IP activity lookup";
		return flatfile_ip_activity_result::invalid;
	}
	std::vector<flatfile_ip_activity_record> records;
	const auto loaded = load_records(activity_directory(root), &records, error);
	if (loaded != load_result::ok)
		return public_result(loaded);
	const auto found = std::lower_bound(records.begin(), records.end(), pid,
					    [](const auto &entry, uint32_t wanted)
					    { return entry.pid < wanted; });
	if (found == records.end() || found->pid != pid)
		return flatfile_ip_activity_result::not_found;
	*record = *found;
	return flatfile_ip_activity_result::ok;
}

flatfile_ip_activity_result flatfile_ip_activity_find_latest(const char *root, const char *ip,
							     flatfile_ip_activity_record *record,
							     std::string *error)
{
	if (!record || !valid_ip(ip ? std::string(ip) : std::string(), false))
	{
		if (error)
			*error = "invalid IP activity search";
		return flatfile_ip_activity_result::invalid;
	}
	std::vector<flatfile_ip_activity_record> records;
	const auto loaded = load_records(activity_directory(root), &records, error);
	if (loaded != load_result::ok)
		return public_result(loaded);
	const flatfile_ip_activity_record *latest = nullptr;
	for (const auto &candidate : records)
		if (candidate.ip == ip &&
		    (!latest || candidate.last_connect > latest->last_connect ||
		     (candidate.last_connect == latest->last_connect &&
		      candidate.pid > latest->pid)))
			latest = &candidate;
	if (!latest)
		return flatfile_ip_activity_result::not_found;
	*record = *latest;
	return flatfile_ip_activity_result::ok;
}

flatfile_ip_activity_result flatfile_ip_activity_reset_active(const char *root, int64_t occurred_at,
							      std::string *error)
{
	if (occurred_at <= 0)
	{
		if (error)
			*error = "invalid IP activity reset time";
		return flatfile_ip_activity_result::invalid;
	}
	std::vector<flatfile_ip_activity_record> existing;
	const auto loaded = load_records(activity_directory(root), &existing, error);
	if (loaded == load_result::not_found)
		return flatfile_ip_activity_result::ok;
	if (loaded != load_result::ok)
		return public_result(loaded);
	return mutate_records(
		root,
		[&](auto *records)
		{
			bool changed = false;
			for (auto &record : *records)
				if (record.last_connect > record.last_disconnect)
				{
					record.last_disconnect = occurred_at;
					changed = true;
				}
			return changed;
		},
		error);
}
