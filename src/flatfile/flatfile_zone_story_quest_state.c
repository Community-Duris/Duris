#include "flatfile/flatfile_zone_story_quest_state.h"

#include "flatfile/flatfile_store.h"
#include "world/zone_story_quest_feature.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cerrno>
#include <new>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <string>
#include <vector>

namespace
{
constexpr std::array<uint8_t, 8> state_magic = { 'D', 'U', 'R', 'Z', 'Q', 'S', 'T', '1' };
constexpr uint32_t state_version = 2;
constexpr size_t state_digest_size = SHA256_DIGEST_LENGTH;
constexpr size_t state_header_size = state_magic.size() + sizeof(uint32_t) + sizeof(uint32_t) +
				     sizeof(uint64_t) + state_digest_size;
constexpr size_t state_maximum_bytes = 64U * 1024U * 1024U;

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

std::string state_directory(const char *root)
{
	return root && *root ? std::string(root) + "/domains" : std::string();
}
}

flatfile_zone_story_quest_result legacy_state_load(const char *root,
						   uint32_t expected_catalog_revision,
						   std::string *state, std::string *error)
{
	if (!root || !*root || !expected_catalog_revision || !state)
	{
		if (error)
			*error = "invalid zone-story flat-file state location";
		return flatfile_zone_story_quest_result::invalid;
	}
	std::vector<uint8_t> bytes;
	const auto loaded = flatfile_read(state_directory(root), "zone-story-quests.state",
					  state_maximum_bytes, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_zone_story_quest_result::not_found;
	if (loaded != flatfile_read_result::ok)
		return loaded == flatfile_read_result::invalid ?
			       flatfile_zone_story_quest_result::invalid :
			       flatfile_zone_story_quest_result::io_error;
	if (bytes.size() < state_header_size ||
	    !std::equal(state_magic.begin(), state_magic.end(), bytes.begin()) ||
	    (read_u32(bytes.data() + state_magic.size()) != state_version &&
	     read_u32(bytes.data() + state_magic.size()) != 1))
	{
		if (error)
			*error = "zone-story flat-file state header is corrupt";
		return flatfile_zone_story_quest_result::corrupt;
	}
	const uint32_t catalog_revision =
		read_u32(bytes.data() + state_magic.size() + sizeof(uint32_t));
	if (catalog_revision != expected_catalog_revision &&
	    !(catalog_revision == 1 && expected_catalog_revision == 2))
	{
		if (error)
			*error = "zone-story flat-file state catalog revision is stale";
		return flatfile_zone_story_quest_result::corrupt;
	}
	const uint64_t payload_size =
		read_u64(bytes.data() + state_magic.size() + sizeof(uint32_t) + sizeof(uint32_t));
	if (payload_size > state_maximum_bytes - state_header_size ||
	    bytes.size() != state_header_size + static_cast<size_t>(payload_size))
	{
		if (error)
			*error = "zone-story flat-file state length is corrupt";
		return flatfile_zone_story_quest_result::corrupt;
	}
	const uint8_t *expected_digest = bytes.data() + state_magic.size() + sizeof(uint32_t) +
					 sizeof(uint32_t) + sizeof(uint64_t);
	const uint8_t *payload = bytes.data() + state_header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload, static_cast<size_t>(payload_size), digest.data());
	if (CRYPTO_memcmp(expected_digest, digest.data(), digest.size()))
	{
		if (error)
			*error = "zone-story flat-file state checksum is invalid";
		return flatfile_zone_story_quest_result::corrupt;
	}
	state->assign(reinterpret_cast<const char *>(payload), static_cast<size_t>(payload_size));
	return flatfile_zone_story_quest_result::ok;
}

namespace
{
constexpr size_t journal_header_size = state_header_size + sizeof(uint32_t);
constexpr size_t journal_digest_offset = journal_header_size - SHA256_DIGEST_LENGTH;
constexpr size_t compact_slack = 8U * 1024U * 1024U;
struct journal_cache
{
	std::string directory;
	zone_story_quest_state::records values;
	uint64_t valid_bytes = 0;
	uint64_t file_bytes = 0;
	size_t record_bytes = 0;
	bool journal = false;
	bool observed = false;
	struct stat identity = {};
};
journal_cache cache;

struct file_descriptor
{
	int fd = -1;
	~file_descriptor()
	{
		if (fd >= 0)
			close(fd);
	}
};
struct state_lock
{
	int fd = -1;
	~state_lock() { flatfile_lock_release(fd); }
};
flatfile_zone_story_quest_result recover(const std::string &root,
					 const flatfile_authority_lock &lock, std::string *error)
{
	const auto result = flatfile_authority_transaction_recover(root, lock, error);
	return result == flatfile_authority_transaction_result::ok ?
		       flatfile_zone_story_quest_result::ok :
	       result == flatfile_authority_transaction_result::io_error ?
		       flatfile_zone_story_quest_result::io_error :
		       flatfile_zone_story_quest_result::invalid;
}

bool read_at(int fd, uint64_t offset, uint8_t *data, size_t size)
{
	while (size)
	{
		const ssize_t count = pread(fd, data, size, static_cast<off_t>(offset));
		if (count < 0 && errno == EINTR)
			continue;
		if (count <= 0)
			return false;
		data += count;
		offset += count;
		size -= count;
	}
	return true;
}
bool write_bytes(int fd, const std::vector<uint8_t> &bytes)
{
	size_t offset = 0;
	while (offset < bytes.size())
	{
		const ssize_t count = write(fd, bytes.data() + offset, bytes.size() - offset);
		if (count < 0 && errno == EINTR)
			continue;
		if (count <= 0)
			return false;
		offset += count;
	}
	return true;
}
int open_state(const std::string &directory, int flags)
{
	const int parent = open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
	if (parent < 0)
		return -1;
	struct stat info = {};
	if (fstat(parent, &info) || info.st_uid != geteuid() || (info.st_mode & 0077))
	{
		close(parent);
		errno = EACCES;
		return -1;
	}
	const int fd = openat(parent, "zone-story-quests.state",
			      flags | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
	close(parent);
	return fd;
}
bool private_file(int fd, struct stat *info)
{
	return !fstat(fd, info) && S_ISREG(info->st_mode) && info->st_uid == geteuid() &&
	       info->st_nlink == 1 && !(info->st_mode & 0077) && info->st_size >= 0;
}
bool cache_matches(const std::string &directory)
{
	if (!cache.observed || cache.directory != directory)
		return false;
	file_descriptor file{ open_state(directory, O_RDONLY) };
	struct stat info = {};
	return file.fd >= 0 && private_file(file.fd, &info) &&
	       info.st_dev == cache.identity.st_dev && info.st_ino == cache.identity.st_ino &&
	       info.st_size == cache.identity.st_size &&
	       info.st_mtim.tv_sec == cache.identity.st_mtim.tv_sec &&
	       info.st_mtim.tv_nsec == cache.identity.st_mtim.tv_nsec;
}
void observe_cache_file()
{
	file_descriptor file{ open_state(cache.directory, O_RDONLY) };
	cache.observed = file.fd >= 0 && private_file(file.fd, &cache.identity);
}
std::vector<uint8_t> frame(uint32_t revision, const zone_story_quest_state::changes &updates)
{
	const std::string payload = zone_story_quest_state::encode(updates.values);
	std::vector<uint8_t> bytes(state_magic.begin(), state_magic.end());
	append_u32(&bytes, 3);
	append_u32(&bytes, revision);
	append_u64(&bytes, payload.size());
	append_u32(&bytes, updates.replace ? 1 : 0);
	bytes.insert(bytes.end(), payload.begin(), payload.end());
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(bytes.data(), bytes.size(), digest.data());
	bytes.insert(bytes.begin() + journal_digest_offset, digest.begin(), digest.end());
	return bytes;
}
size_t record_size(const std::string &key, const std::string &value)
{
	return 2 * (key.size() + value.size()) + 2;
}
void update_cache(const zone_story_quest_state::changes &updates)
{
	if (updates.replace)
	{
		cache.values.clear();
		cache.record_bytes = 0;
	}
	for (const auto &[key, value] : updates.values)
	{
		const auto old = cache.values.find(key);
		if (old != cache.values.end())
			cache.record_bytes -= record_size(old->first, old->second);
		if (value.empty())
			cache.values.erase(key);
		else
		{
			cache.values[key] = value;
			cache.record_bytes += record_size(key, value);
		}
	}
}
std::vector<uint8_t> snapshot(uint32_t revision, const zone_story_quest_state::records &values)
{
	// Multiple bounded frames form one snapshot, published by one atomic rename.
	std::vector<uint8_t> bytes;
	zone_story_quest_state::changes portion;
	portion.replace = true;
	size_t size = 7;
	for (const auto &[key, value] : values)
	{
		const size_t next = record_size(key, value);
		if (size + next > state_maximum_bytes && !portion.values.empty())
		{
			const auto encoded = frame(revision, portion);
			bytes.insert(bytes.end(), encoded.begin(), encoded.end());
			portion.values.clear();
			portion.replace = false;
			size = 7;
		}
		portion.values.emplace(key, value);
		size += next;
	}
	const auto encoded = frame(revision, portion);
	bytes.insert(bytes.end(), encoded.begin(), encoded.end());
	return bytes;
}
flatfile_zone_story_quest_result broken(std::string *error, const char *message)
{
	if (error)
		*error = message;
	return flatfile_zone_story_quest_result::corrupt;
}
}

static flatfile_zone_story_quest_result load_state(const char *root,
						   uint32_t expected_catalog_revision,
						   std::string *state, std::string *error,
						   bool *legacy)
{
	if (!root || !*root || !expected_catalog_revision || !state)
		return broken(error, "invalid zone-story journal request");
	cache = {};
	cache.directory = state_directory(root);
	file_descriptor file{ open_state(cache.directory, O_RDONLY) };
	const int fd = file.fd;
	if (fd < 0)
	{
		if (errno == ENOENT)
			return flatfile_zone_story_quest_result::not_found;
		return broken(error, "zone-story journal could not be opened");
	}
	struct stat info = {};
	std::array<uint8_t, journal_header_size> header = {};
	if (!private_file(fd, &info) || info.st_size < 12 || !read_at(fd, 0, header.data(), 12) ||
	    !std::equal(state_magic.begin(), state_magic.end(), header.begin()))
	{
		return broken(error, "invalid zone-story journal metadata or header");
	}
	if (read_u32(header.data() + 8) != 3)
	{
		const auto result =
			legacy_state_load(root, expected_catalog_revision, state, error);
		if (result == flatfile_zone_story_quest_result::ok)
		{
			update_cache({ zone_story_quest_state::split_document(*state), true });
			cache.file_bytes = info.st_size;
			cache.identity = info;
			cache.observed = true;
			if (legacy)
				*legacy = true;
		}
		return result;
	}
	cache.file_bytes = info.st_size;
	uint64_t offset = 0;
	bool first = true;
	while (offset < cache.file_bytes)
	{
		// An interrupted final append contributes no facts. A complete but bad
		// frame is corruption and prevents boot; it is never silently skipped.
		if (cache.file_bytes - offset < journal_header_size)
			break;
		if (!read_at(fd, offset, header.data(), header.size()) ||
		    !std::equal(state_magic.begin(), state_magic.end(), header.begin()) ||
		    read_u32(header.data() + 8) != 3)
		{
			return broken(error, "zone-story journal frame header is corrupt");
		}
		const auto revision = read_u32(header.data() + 12);
		const auto size = read_u64(header.data() + 16);
		const auto replace = read_u32(header.data() + 24);
		if (revision != expected_catalog_revision || size > state_maximum_bytes ||
		    replace > 1 || (first && !replace))
		{
			return broken(error, "zone-story journal frame contract is invalid");
		}
		if (size > cache.file_bytes - offset - journal_header_size)
			break;
		std::vector<uint8_t> checked(header.begin(),
					     header.begin() + journal_digest_offset);
		checked.resize(journal_digest_offset + size);
		if (!read_at(fd, offset + journal_header_size,
			     checked.data() + journal_digest_offset, size))
		{
			return broken(error, "zone-story journal read failed");
		}
		std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
		SHA256(checked.data(), checked.size(), digest.data());
		zone_story_quest_state::records values;
		if (CRYPTO_memcmp(digest.data(), header.data() + journal_digest_offset,
				  digest.size()) ||
		    !zone_story_quest_state::decode(
			    std::string_view(reinterpret_cast<const char *>(checked.data() +
									    journal_digest_offset),
					     size),
			    &values))
		{
			return broken(error,
				      "zone-story journal checksum or record encoding is corrupt");
		}
		update_cache({ std::move(values), replace != 0 });
		offset += journal_header_size + size;
		first = false;
	}
	if (first || !cache.values.count("meta"))
		return broken(error, "zone-story journal snapshot is missing");
	cache.valid_bytes = offset;
	cache.journal = true;
	cache.identity = info;
	cache.observed = true;
	if (legacy)
		*legacy = false;
	*state = zone_story_quest_state::document(cache.values);
	return flatfile_zone_story_quest_result::ok;
}

static flatfile_zone_story_quest_result
save_records_locked(const char *root, uint32_t catalog_revision,
		    const zone_story_quest_state::changes &requested, std::string *error)
{
	if (!root || !*root || !catalog_revision)
		return broken(error, "invalid zone-story journal location");
	const auto metadata = requested.values.find("meta");
	if ((metadata != requested.values.end() && metadata->second.empty()) ||
	    (requested.replace && metadata == requested.values.end()))
		return broken(error, "zone-story journal metadata is missing");
	const auto directory = state_directory(root);
	if (!cache_matches(directory))
	{
		std::string ignored;
		const auto loaded = load_state(root, catalog_revision, &ignored, error, nullptr);
		if (loaded != flatfile_zone_story_quest_result::ok &&
		    loaded != flatfile_zone_story_quest_result::not_found && !requested.replace)
			return loaded;
	}
	auto updates = requested;
	if (!updates.replace)
	{
		for (auto entry = updates.values.begin(); entry != updates.values.end();)
		{
			const auto old = cache.values.find(entry->first);
			if ((entry->second.empty() && old == cache.values.end()) ||
			    (old != cache.values.end() && old->second == entry->second))
				entry = updates.values.erase(entry);
			else
				++entry;
		}
		if (updates.values.empty())
			return flatfile_zone_story_quest_result::ok;
	}
	size_t update_size = 7;
	for (const auto &[key, value] : updates.values)
	{
		const auto bytes = record_size(key, value);
		if (bytes > state_maximum_bytes)
			return broken(error, "zone-story journal record is oversized");
		update_size += bytes;
	}
	if (!updates.replace && update_size > state_maximum_bytes)
		return broken(error, "zone-story journal update is oversized");
	state_lock state_file_lock;
	if (!flatfile_lock_acquire(directory, ".zone-story-quests.lock", &state_file_lock.fd,
				   error))
		return flatfile_zone_story_quest_result::io_error;
	bool saved = false;
	if (updates.replace || !cache.journal ||
	    cache.file_bytes + update_size > 2 * cache.record_bytes + compact_slack)
	{
		auto values = cache.values;
		zone_story_quest_state::apply(&values, updates);
		if (!values.count("meta"))
		{
			return broken(error, "zone-story journal metadata is missing");
		}
		const auto bytes = snapshot(catalog_revision, values);
		saved = flatfile_atomic_write(directory, "zone-story-quests.state", bytes, error);
		if (saved)
		{
			cache.valid_bytes = cache.file_bytes = bytes.size();
			cache.journal = true;
		}
	}
	else
	{
		const auto bytes = frame(catalog_revision, updates);
		file_descriptor file{ open_state(directory, O_RDWR) };
		const int fd = file.fd;
		struct stat info = {};
		if (fd >= 0 && private_file(fd, &info) &&
		    static_cast<uint64_t>(info.st_size) == cache.file_bytes &&
		    !ftruncate(fd, cache.valid_bytes) &&
		    lseek(fd, cache.valid_bytes, SEEK_SET) >= 0)
		{
			saved = write_bytes(fd, bytes) && !fdatasync(fd);
			if (saved)
				cache.valid_bytes = cache.file_bytes =
					cache.valid_bytes + bytes.size();
			else
			{
				const bool rolled_back = !ftruncate(fd, cache.valid_bytes) &&
							 !fdatasync(fd);
				if (!rolled_back)
					cache.journal = false;
				cache.file_bytes = cache.valid_bytes;
			}
		}
	}
	if (!saved)
	{
		if (error)
			*error = "zone-story journal write failed";
		return flatfile_zone_story_quest_result::io_error;
	}
	update_cache(updates);
	observe_cache_file();
	return flatfile_zone_story_quest_result::ok;
}

flatfile_zone_story_quest_result
flatfile_zone_story_quest_state_load(const char *root, uint32_t expected_catalog_revision,
				     std::string *state, std::string *error, bool *legacy)
try
{
	if (!root || !*root || !expected_catalog_revision || !state)
		return flatfile_zone_story_quest_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_zone_story_quest_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_zone_story_quest_result::ok)
		return recovered;
	return load_state(root, expected_catalog_revision, state, error, legacy);
}
catch (const std::bad_alloc &)
{
	errno = ENOMEM;
	return flatfile_zone_story_quest_result::io_error;
}

flatfile_zone_story_quest_result
flatfile_zone_story_quest_records_save(const char *root, uint32_t catalog_revision,
				       const zone_story_quest_state::changes &updates,
				       std::string *error)
try
{
	if (!root || !*root || !catalog_revision)
		return flatfile_zone_story_quest_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_zone_story_quest_result::io_error;
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_zone_story_quest_result::ok)
		return recovered;
	return save_records_locked(root, catalog_revision, updates, error);
}
catch (const std::bad_alloc &)
{
	errno = ENOMEM;
	return flatfile_zone_story_quest_result::io_error;
}

flatfile_zone_story_quest_result flatfile_zone_story_quest_state_save(const char *root,
								      uint32_t catalog_revision,
								      const std::string &state,
								      std::string *error)
try
{
	return flatfile_zone_story_quest_records_save(
		root, catalog_revision, { zone_story_quest_state::split_document(state), true },
		error);
}
catch (const std::bad_alloc &)
{
	errno = ENOMEM;
	return flatfile_zone_story_quest_result::io_error;
}

flatfile_zone_story_quest_result flatfile_zone_story_quest_state_prepare_player_remove(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t catalog_revision,
	uint32_t pid, flatfile_authority_operation *operation, std::string *error)
try
{
	if (!operation || !pid || !catalog_revision || !lock.matches(root))
		return flatfile_zone_story_quest_result::invalid;
	*operation = {};
	const auto recovered = recover(root, lock, error);
	if (recovered != flatfile_zone_story_quest_result::ok)
		return recovered;
	std::string before;
	const auto loaded = load_state(root.c_str(), catalog_revision, &before, error, nullptr);
	if (loaded != flatfile_zone_story_quest_result::ok)
		return loaded;
	zone_story_quest_feature::service state;
	if (!state.deserialize_state(before, error) || !state.erase_character_all_seasons(pid, 1))
		return flatfile_zone_story_quest_result::corrupt;
	const std::string after = state.serialize_state(error);
	if (after.empty())
		return flatfile_zone_story_quest_result::io_error;
	const auto after_values = zone_story_quest_state::split_document(after);
	if (zone_story_quest_state::split_document(before) == after_values)
		return flatfile_zone_story_quest_result::unchanged;
	auto bytes = snapshot(catalog_revision, after_values);
	if (bytes.size() > flatfile_authority_transaction_maximum_bytes)
		return flatfile_zone_story_quest_result::invalid;
	operation->store = flatfile_authority_store::domains;
	operation->kind = flatfile_authority_operation_kind::write;
	operation->filename = "zone-story-quests.state";
	operation->bytes = std::move(bytes);
	return flatfile_zone_story_quest_result::ok;
}
catch (const std::bad_alloc &)
{
	errno = ENOMEM;
	return flatfile_zone_story_quest_result::io_error;
}
