#include "flatfile_shop_trophy_history.h"

#include "flatfile_store.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <string>
#include <vector>

namespace
{
constexpr std::array<uint8_t, 8> history_magic = { 'D', 'U', 'R', 'S', 'H', 'O', 'P', 'T' };
constexpr uint32_t history_version = 1;
constexpr size_t history_maximum_entries = 100000;
constexpr size_t history_entry_size = sizeof(uint32_t) * 3 + sizeof(uint64_t);
constexpr size_t history_header_size =
	history_magic.size() + sizeof(uint32_t) * 2 + SHA256_DIGEST_LENGTH;
constexpr size_t history_maximum_bytes =
	history_header_size + history_maximum_entries * history_entry_size;
constexpr int64_t seconds_per_day = 60 * 60 * 24;
constexpr int64_t trophy_days = 7;
constexpr const char *history_filename = "shop-trophy";
constexpr const char *history_lock_filename = ".shop-trophy.lock";

struct history_entry
{
	int item = 0;
	int value = 0;
	int seller = 0;
	int64_t occurred_at = 0;
};

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

uint32_t read_u32(const uint8_t *bytes)
{
	uint32_t value = 0;
	for (size_t offset = 0; offset < sizeof(value); ++offset)
		value |= static_cast<uint32_t>(bytes[offset]) << (offset * 8);
	return value;
}

uint64_t read_u64(const uint8_t *bytes)
{
	uint64_t value = 0;
	for (size_t offset = 0; offset < sizeof(value); ++offset)
		value |= static_cast<uint64_t>(bytes[offset]) << (offset * 8);
	return value;
}

bool valid_entry(const history_entry &entry)
{
	return entry.item >= 0 && entry.value > 0 && entry.seller >= 0 && entry.occurred_at > 0;
}

bool in_trophy_window(int64_t occurred_at, int64_t now)
{
	return now / seconds_per_day - occurred_at / seconds_per_day <= trophy_days;
}

bool encode_history(const std::vector<history_entry> &history, std::vector<uint8_t> *bytes)
{
	if (!bytes || history.size() > history_maximum_entries)
		return false;
	try
	{
		std::vector<uint8_t> payload;
		payload.reserve(history.size() * history_entry_size);
		for (const auto &entry : history)
		{
			if (!valid_entry(entry))
				return false;
			append_u32(&payload, static_cast<uint32_t>(entry.item));
			append_u32(&payload, static_cast<uint32_t>(entry.value));
			append_u32(&payload, static_cast<uint32_t>(entry.seller));
			append_u64(&payload, static_cast<uint64_t>(entry.occurred_at));
		}
		std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
		SHA256(payload.data(), payload.size(), digest.data());
		bytes->clear();
		bytes->reserve(history_header_size + payload.size());
		bytes->insert(bytes->end(), history_magic.begin(), history_magic.end());
		append_u32(bytes, history_version);
		append_u32(bytes, static_cast<uint32_t>(history.size()));
		bytes->insert(bytes->end(), digest.begin(), digest.end());
		bytes->insert(bytes->end(), payload.begin(), payload.end());
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool decode_history(const std::vector<uint8_t> &bytes, std::vector<history_entry> *history)
{
	if (!history || bytes.size() < history_header_size ||
	    !std::equal(history_magic.begin(), history_magic.end(), bytes.begin()) ||
	    read_u32(bytes.data() + history_magic.size()) != history_version)
		return false;
	const uint32_t count = read_u32(bytes.data() + history_magic.size() + sizeof(uint32_t));
	if (count > history_maximum_entries ||
	    bytes.size() != history_header_size + static_cast<size_t>(count) * history_entry_size)
		return false;
	const uint8_t *expected_digest = bytes.data() + history_magic.size() + sizeof(uint32_t) * 2;
	const uint8_t *payload = bytes.data() + history_header_size;
	const size_t payload_size = bytes.size() - history_header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload, payload_size, digest.data());
	if (CRYPTO_memcmp(expected_digest, digest.data(), digest.size()))
		return false;

	std::vector<history_entry> decoded;
	try
	{
		decoded.reserve(count);
		for (uint32_t index = 0; index < count; ++index)
		{
			const uint8_t *encoded =
				payload + static_cast<size_t>(index) * history_entry_size;
			history_entry entry{
				static_cast<int32_t>(read_u32(encoded)),
				static_cast<int32_t>(read_u32(encoded + sizeof(uint32_t))),
				static_cast<int32_t>(read_u32(encoded + sizeof(uint32_t) * 2)),
				static_cast<int64_t>(read_u64(encoded + sizeof(uint32_t) * 3))
			};
			if (!valid_entry(entry))
				return false;
			decoded.push_back(entry);
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	*history = std::move(decoded);
	return true;
}

std::string history_directory(const char *root)
{
	return root && *root ? std::string(root) + "/metadata" : std::string();
}

load_result load_history(const std::string &directory, std::vector<history_entry> *history,
			 std::string *error)
{
	if (directory.empty() || !history)
	{
		if (error)
			*error = "invalid shop-trophy history location";
		return load_result::io_error;
	}
	std::vector<uint8_t> bytes;
	const auto loaded =
		flatfile_read(directory, history_filename, history_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return load_result::not_found;
	if (loaded != flatfile_read_result::ok)
		return loaded == flatfile_read_result::io_error ? load_result::io_error :
								  load_result::corrupt;
	if (!decode_history(bytes, history))
	{
		if (error)
			*error = "shop-trophy history is corrupt";
		return load_result::corrupt;
	}
	return load_result::ok;
}

flatfile_shop_trophy_result public_result(load_result result)
{
	switch (result)
	{
	case load_result::ok:
	case load_result::not_found:
		return flatfile_shop_trophy_result::ok;
	case load_result::corrupt:
		return flatfile_shop_trophy_result::corrupt;
	case load_result::io_error:
		return flatfile_shop_trophy_result::io_error;
	}
	return flatfile_shop_trophy_result::io_error;
}
} // namespace

flatfile_shop_trophy_result flatfile_shop_trophy_record(const char *root, int item, int value,
							int seller, int64_t occurred_at,
							std::string *error)
{
	const history_entry added{ item, value, seller, occurred_at };
	const std::string directory = history_directory(root);
	if (directory.empty() || !valid_entry(added))
	{
		if (error)
			*error = "invalid shop sale history entry";
		return flatfile_shop_trophy_result::invalid;
	}
	int lock_fd = -1;
	if (!flatfile_lock_acquire(directory, history_lock_filename, &lock_fd, error))
		return flatfile_shop_trophy_result::io_error;
	std::vector<history_entry> history;
	const auto loaded = load_history(directory, &history, error);
	if (loaded == load_result::corrupt || loaded == load_result::io_error)
	{
		flatfile_lock_release(lock_fd);
		return public_result(loaded);
	}
	bool saved = false;
	try
	{
		history.erase(std::remove_if(history.begin(), history.end(),
					     [occurred_at](const history_entry &entry) {
						     return !in_trophy_window(entry.occurred_at,
									      occurred_at);
					     }),
			      history.end());
		if (history.size() < history_maximum_entries)
		{
			history.push_back(added);
			std::vector<uint8_t> bytes;
			saved = encode_history(history, &bytes) &&
				flatfile_atomic_write(directory, history_filename, bytes, error);
		}
		else if (error)
			*error = "shop-trophy history limit reached";
	}
	catch (const std::bad_alloc &)
	{
		if (error)
			*error = "shop-trophy history allocation failed";
	}
	flatfile_lock_release(lock_fd);
	return saved ? flatfile_shop_trophy_result::ok : flatfile_shop_trophy_result::io_error;
}

flatfile_shop_trophy_result flatfile_shop_trophy_count(const char *root, int item, int64_t now,
						       int *count, std::string *error)
{
	if (item < 0 || now <= 0 || !count)
		return flatfile_shop_trophy_result::invalid;
	*count = 0;
	std::vector<history_entry> history;
	const auto loaded = load_history(history_directory(root), &history, error);
	if (loaded == load_result::not_found)
		return flatfile_shop_trophy_result::ok;
	if (loaded != load_result::ok)
		return public_result(loaded);
	*count = static_cast<int>(std::count_if(
		history.begin(), history.end(), [item, now](const history_entry &entry)
		{ return entry.item == item && in_trophy_window(entry.occurred_at, now); }));
	return flatfile_shop_trophy_result::ok;
}
