#include "flatfile/flatfile_zone_story_quest_state.h"

#include "flatfile/flatfile_store.h"
#include "world/zone_story_quest_feature.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cerrno>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <string>
#include <vector>

namespace
{
constexpr std::array<uint8_t, 8> state_magic = { 'D', 'U', 'R', 'Z', 'Q', 'S', 'T', '1' };
constexpr uint32_t state_version = 1;
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

static flatfile_zone_story_quest_result load_state(const char *root,
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
	    read_u32(bytes.data() + state_magic.size()) != state_version)
	{
		if (error)
			*error = "zone-story flat-file state header is corrupt";
		return flatfile_zone_story_quest_result::corrupt;
	}
	const uint32_t catalog_revision =
		read_u32(bytes.data() + state_magic.size() + sizeof(uint32_t));
	if (catalog_revision != expected_catalog_revision)
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
bool encode_state(uint32_t catalog_revision, const std::string &state, std::vector<uint8_t> *bytes)
{
	if (!catalog_revision || state.size() > state_maximum_bytes - state_header_size)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(reinterpret_cast<const uint8_t *>(state.data()), state.size(), digest.data());
	bytes->reserve(state_header_size + state.size());
	bytes->insert(bytes->end(), state_magic.begin(), state_magic.end());
	append_u32(bytes, state_version);
	append_u32(bytes, catalog_revision);
	append_u64(bytes, static_cast<uint64_t>(state.size()));
	bytes->insert(bytes->end(), digest.begin(), digest.end());
	bytes->insert(bytes->end(), state.begin(), state.end());
	return true;
}

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

struct state_lock
{
	int fd = -1;
	~state_lock() { flatfile_lock_release(fd); }
};
} // namespace

flatfile_zone_story_quest_result
flatfile_zone_story_quest_state_load(const char *root, uint32_t expected_catalog_revision,
				     std::string *state, std::string *error)
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
	return load_state(root, expected_catalog_revision, state, error);
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
	if (!root || !*root || !catalog_revision ||
	    state.size() > state_maximum_bytes - state_header_size)
		return flatfile_zone_story_quest_result::invalid;
	flatfile_authority_lock authority_lock;
	if (!authority_lock.acquire(root, error))
		return flatfile_zone_story_quest_result::io_error;
	const auto recovered = recover(root, authority_lock, error);
	if (recovered != flatfile_zone_story_quest_result::ok)
		return recovered;
	const std::string directory = state_directory(root);
	state_lock lock;
	if (!flatfile_lock_acquire(directory, ".zone-story-quests.lock", &lock.fd, error))
		return flatfile_zone_story_quest_result::io_error;
	std::vector<uint8_t> bytes;
	if (!encode_state(catalog_revision, state, &bytes))
		return flatfile_zone_story_quest_result::invalid;
	return flatfile_atomic_write(directory, "zone-story-quests.state", bytes, error) ?
		       flatfile_zone_story_quest_result::ok :
		       flatfile_zone_story_quest_result::io_error;
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
	const auto loaded = load_state(root.c_str(), catalog_revision, &before, error);
	if (loaded != flatfile_zone_story_quest_result::ok)
		return loaded;
	zone_story_quest_feature::service state;
	if (!state.deserialize_state(before, error) || !state.erase_character_all_seasons(pid, 1))
		return flatfile_zone_story_quest_result::corrupt;
	const std::string after = state.serialize_state(error);
	if (after.empty())
		return flatfile_zone_story_quest_result::io_error;
	if (before == after)
		return flatfile_zone_story_quest_result::unchanged;
	operation->store = flatfile_authority_store::domains;
	operation->kind = flatfile_authority_operation_kind::write;
	operation->filename = "zone-story-quests.state";
	if (!encode_state(catalog_revision, after, &operation->bytes))
		return flatfile_zone_story_quest_result::invalid;
	return flatfile_zone_story_quest_result::ok;
}
catch (const std::bad_alloc &)
{
	errno = ENOMEM;
	return flatfile_zone_story_quest_result::io_error;
}
