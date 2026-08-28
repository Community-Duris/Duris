#include "flatfile_world_quest_history.h"

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
constexpr std::array<uint8_t, 8> history_magic = { 'D', 'U', 'R', 'W', 'Q', 'H', 'S', 'T' };
constexpr uint32_t history_version = 1;
constexpr size_t history_maximum_entries = 100000;
constexpr size_t history_entry_size = sizeof(uint32_t) * 2 + sizeof(uint64_t);
constexpr size_t history_header_size =
	history_magic.size() + sizeof(uint32_t) * 2 + SHA256_DIGEST_LENGTH;
constexpr size_t history_maximum_bytes =
	history_header_size + history_maximum_entries * history_entry_size;

struct history_entry
{
	int quest_target = 0;
	int player_level = 0;
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
	return entry.quest_target > 0 && entry.player_level >= 0 &&
	       entry.player_level <= UCHAR_MAX && entry.occurred_at > 0;
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
			append_u32(&payload, static_cast<uint32_t>(entry.quest_target));
			append_u32(&payload, static_cast<uint32_t>(entry.player_level));
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
			const uint32_t quest_target = read_u32(encoded);
			const uint32_t player_level = read_u32(encoded + sizeof(uint32_t));
			const uint64_t occurred_at = read_u64(encoded + sizeof(uint32_t) * 2);
			if (quest_target > INT_MAX || player_level > INT_MAX ||
			    occurred_at > INT64_MAX)
				return false;
			history_entry entry{ static_cast<int>(quest_target),
					     static_cast<int>(player_level),
					     static_cast<int64_t>(occurred_at) };
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
	return root && *root ? std::string(root) + "/players" : std::string();
}

std::string history_filename(uint32_t pid)
{
	return std::to_string(pid) + ".world-quests";
}

std::string history_lock_filename(uint32_t pid)
{
	return ".player-" + std::to_string(pid) + "-world-quests.lock";
}

load_result load_history(const std::string &directory, uint32_t pid,
			 std::vector<history_entry> *history, std::string *error)
{
	if (directory.empty() || !pid || !history)
	{
		if (error)
			*error = "invalid world-quest history location";
		return load_result::io_error;
	}
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(directory, history_filename(pid), history_maximum_bytes,
					  &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return load_result::not_found;
	if (loaded != flatfile_read_result::ok)
		return loaded == flatfile_read_result::io_error ? load_result::io_error :
								  load_result::corrupt;
	if (!decode_history(bytes, history))
	{
		if (error)
			*error = "world-quest history is corrupt";
		return load_result::corrupt;
	}
	return load_result::ok;
}

flatfile_world_quest_result public_result(load_result result)
{
	switch (result)
	{
	case load_result::ok:
		return flatfile_world_quest_result::ok;
	case load_result::not_found:
		return flatfile_world_quest_result::not_found;
	case load_result::corrupt:
		return flatfile_world_quest_result::corrupt;
	case load_result::io_error:
		return flatfile_world_quest_result::io_error;
	}
	return flatfile_world_quest_result::io_error;
}
} // namespace

flatfile_world_quest_result flatfile_world_quest_record(const char *root, uint32_t pid,
							int quest_target, int player_level,
							int64_t occurred_at, std::string *error)
{
	history_entry added{ quest_target, player_level, occurred_at };
	const std::string directory = history_directory(root);
	if (!pid || directory.empty() || !valid_entry(added))
	{
		if (error)
			*error = "invalid world-quest completion";
		return flatfile_world_quest_result::invalid;
	}
	int lock_fd = -1;
	if (!flatfile_lock_acquire(directory, history_lock_filename(pid), &lock_fd, error))
		return flatfile_world_quest_result::io_error;
	std::vector<history_entry> history;
	const auto loaded = load_history(directory, pid, &history, error);
	if (loaded == load_result::corrupt || loaded == load_result::io_error)
	{
		flatfile_lock_release(lock_fd);
		return public_result(loaded);
	}
	bool saved = false;
	try
	{
		if (history.size() < history_maximum_entries)
		{
			history.push_back(added);
			std::vector<uint8_t> bytes;
			saved = encode_history(history, &bytes) &&
				flatfile_atomic_write(directory, history_filename(pid), bytes,
						      error);
		}
		else if (error)
			*error = "world-quest history limit reached";
	}
	catch (const std::bad_alloc &)
	{
		if (error)
			*error = "world-quest history allocation failed";
	}
	flatfile_lock_release(lock_fd);
	return saved ? flatfile_world_quest_result::ok : flatfile_world_quest_result::io_error;
}

flatfile_world_quest_result flatfile_world_quest_completed(const char *root, uint32_t pid,
							   int quest_target, bool *completed,
							   std::string *error)
{
	if (!pid || quest_target <= 0 || !completed)
		return flatfile_world_quest_result::invalid;
	*completed = false;
	std::vector<history_entry> history;
	const auto loaded = load_history(history_directory(root), pid, &history, error);
	if (loaded == load_result::not_found)
		return flatfile_world_quest_result::ok;
	if (loaded != load_result::ok)
		return public_result(loaded);
	*completed = std::any_of(history.begin(), history.end(), [quest_target](const auto &entry)
				 { return entry.quest_target == quest_target; });
	return flatfile_world_quest_result::ok;
}

flatfile_world_quest_result flatfile_world_quest_count_day(const char *root, uint32_t pid,
							   int player_level, int64_t now,
							   int *count, std::string *error)
{
	if (!pid || player_level < 0 || player_level > UCHAR_MAX || now <= 0 || !count)
		return flatfile_world_quest_result::invalid;
	*count = 0;
	std::vector<history_entry> history;
	const auto loaded = load_history(history_directory(root), pid, &history, error);
	if (loaded == load_result::not_found)
		return flatfile_world_quest_result::ok;
	if (loaded != load_result::ok)
		return public_result(loaded);
	const int64_t current_day = now / (60 * 60 * 24);
	for (const auto &entry : history)
		if (entry.occurred_at / (60 * 60 * 24) == current_day &&
		    (player_level >= 50 || entry.player_level == player_level))
			++*count;
	return flatfile_world_quest_result::ok;
}

flatfile_world_quest_result
flatfile_world_quest_prepare_remove(const std::string &root, const flatfile_authority_lock &lock,
				    uint32_t pid, flatfile_authority_operation *operation,
				    std::string *error)
{
	if (!operation || !pid || !lock.matches(root))
		return flatfile_world_quest_result::invalid;
	*operation = {};
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_world_quest_result::io_error :
			       flatfile_world_quest_result::invalid;
	std::vector<history_entry> history;
	const auto loaded = load_history(history_directory(root.c_str()), pid, &history, error);
	if (loaded != load_result::ok)
		return public_result(loaded);
	operation->store = flatfile_authority_store::players;
	operation->kind = flatfile_authority_operation_kind::remove;
	operation->filename = history_filename(pid);
	return flatfile_world_quest_result::ok;
}
