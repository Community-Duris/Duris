#include "flatfile_player_repository.h"

#include "flatfile_store.h"
#include "persistence_mode.h"
#include "player_snapshot_codec.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <limits>
#include <mutex>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
constexpr uint32_t player_file_version = 1;
constexpr std::array<uint8_t, 8> player_magic = { 'D', 'U', 'R', 'P', 'L', 'Y', 'R', 0 };
constexpr size_t player_file_maximum = PLAYER_SNAPSHOT_MAX_BYTES + 128;
std::mutex player_mutex;

struct encoder
{
	std::vector<uint8_t> bytes;

	template <typename T> void number(T value)
	{
		using unsigned_type = std::make_unsigned_t<T>;
		unsigned_type bits = static_cast<unsigned_type>(value);
		for (size_t index = 0; index < sizeof(T); ++index)
		{
			bytes.push_back(static_cast<uint8_t>(bits & 0xff));
			bits >>= 8;
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
		using unsigned_type = std::make_unsigned_t<T>;
		unsigned_type bits = 0;
		for (size_t index = 0; index < sizeof(T); ++index)
			bits |= static_cast<unsigned_type>(data[offset++]) << (index * 8);
		*value = static_cast<T>(bits);
		return true;
	}
};

struct authority_lock
{
	int fd = -1;
	~authority_lock() { flatfile_lock_release(fd); }
};

std::string player_directory(const std::string &root)
{
	return root + "/players";
}

std::string player_filename(int32_t pid)
{
	return std::to_string(pid) + ".snapshot";
}

std::string player_lock_filename(int32_t pid)
{
	return ".player-" + std::to_string(pid) + ".lock";
}

bool valid_snapshot(const player_snapshot &snapshot)
{
	return snapshot.schema_version == PLAYER_SNAPSHOT_SCHEMA_VERSION && snapshot.pid > 0 &&
	       snapshot.revision && snapshot.components &&
	       !(snapshot.components & ~PLAYER_CHECKPOINT_COMPONENT_ALL) &&
	       snapshot.encoded_size_bound &&
	       snapshot.encoded_size_bound <= PLAYER_SNAPSHOT_MAX_BYTES;
}

bool normalize_size(player_snapshot *snapshot, std::vector<uint8_t> *payload)
{
	if (!snapshot || !payload)
		return false;
	snapshot->encoded_size_bound = 1;
	if (player_snapshot_encode(*snapshot, payload) != player_snapshot_codec_result::ok)
		return false;
	snapshot->encoded_size_bound = payload->size();
	return player_snapshot_encode(*snapshot, payload) == player_snapshot_codec_result::ok;
}

bool encode_file(player_snapshot *snapshot, std::vector<uint8_t> *bytes)
{
	std::vector<uint8_t> payload;
	if (!bytes || !normalize_size(snapshot, &payload))
		return false;
	unsigned char digest[SHA256_DIGEST_LENGTH];
	SHA256(payload.data(), payload.size(), digest);
	encoder out;
	out.bytes.insert(out.bytes.end(), player_magic.begin(), player_magic.end());
	out.number<uint32_t>(player_file_version);
	out.number<uint32_t>(payload.size());
	out.number<int32_t>(snapshot->pid);
	out.number<uint64_t>(snapshot->revision);
	out.number<uint64_t>(snapshot->components);
	out.bytes.insert(out.bytes.end(), digest, digest + sizeof(digest));
	out.bytes.insert(out.bytes.end(), payload.begin(), payload.end());
	if (out.bytes.size() > player_file_maximum)
		return false;
	*bytes = std::move(out.bytes);
	return true;
}

flatfile_player_load_result load_unlocked(const std::string &root, int32_t pid,
					  player_snapshot *snapshot, std::string *error)
{
	if (pid <= 0 || !snapshot)
		return flatfile_player_load_result::invalid;
	std::vector<uint8_t> bytes;
	const flatfile_read_result read = flatfile_read(
		player_directory(root), player_filename(pid), player_file_maximum, &bytes, error);
	if (read == flatfile_read_result::not_found)
		return flatfile_player_load_result::not_found;
	if (read == flatfile_read_result::invalid)
		return flatfile_player_load_result::invalid;
	if (read != flatfile_read_result::ok)
		return flatfile_player_load_result::io_error;
	constexpr size_t header_size = player_magic.size() + sizeof(uint32_t) * 2 +
				       sizeof(int32_t) + sizeof(uint64_t) * 2 +
				       SHA256_DIGEST_LENGTH;
	if (bytes.size() < header_size ||
	    memcmp(bytes.data(), player_magic.data(), player_magic.size()))
		return flatfile_player_load_result::invalid;
	decoder header{ bytes.data() + player_magic.size(), bytes.size() - player_magic.size() };
	uint32_t version = 0, payload_size = 0;
	int32_t stored_pid = 0;
	uint64_t stored_revision = 0, stored_components = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&stored_pid) || !header.number(&stored_revision) ||
	    !header.number(&stored_components) || version != player_file_version ||
	    stored_pid != pid || !stored_revision ||
	    stored_components != PLAYER_CHECKPOINT_COMPONENT_ALL ||
	    payload_size != bytes.size() - header_size)
		return flatfile_player_load_result::invalid;
	const uint8_t *stored_digest = bytes.data() + player_magic.size() + sizeof(uint32_t) * 2 +
				       sizeof(int32_t) + sizeof(uint64_t) * 2;
	const uint8_t *payload = bytes.data() + header_size;
	unsigned char actual_digest[SHA256_DIGEST_LENGTH];
	SHA256(payload, payload_size, actual_digest);
	if (CRYPTO_memcmp(stored_digest, actual_digest, sizeof(actual_digest)))
		return flatfile_player_load_result::invalid;
	player_snapshot decoded = {};
	if (player_snapshot_decode(payload, payload_size, &decoded) !=
		    player_snapshot_codec_result::ok ||
	    decoded.pid != stored_pid || decoded.revision != stored_revision ||
	    decoded.components != stored_components)
		return flatfile_player_load_result::invalid;
	*snapshot = std::move(decoded);
	return flatfile_player_load_result::ok;
}

bool replace_items_together(player_component_mask_t components)
{
	const player_component_mask_t items = PLAYER_COMPONENT_EQUIPMENT |
					      PLAYER_COMPONENT_INVENTORY;
	return !(components & items) || (components & items) == items;
}

bool merge_snapshot(const player_snapshot &incoming, player_snapshot *materialized)
{
	if (!materialized || materialized->pid != incoming.pid ||
	    materialized->components != PLAYER_CHECKPOINT_COMPONENT_ALL ||
	    !replace_items_together(incoming.components))
		return false;
	materialized->revision = incoming.revision;
	materialized->save_intent = incoming.save_intent;
	materialized->room_vnum = incoming.room_vnum;
	materialized->recipes_are_external = incoming.recipes_are_external;
	if (incoming.components & PLAYER_COMPONENT_STATUS)
	{
		materialized->status_integers = incoming.status_integers;
		materialized->status_strings = incoming.status_strings;
		materialized->conditions = incoming.conditions;
		materialized->quest_values = incoming.quest_values;
	}
	if (incoming.components & PLAYER_COMPONENT_LANGUAGES)
		materialized->languages = incoming.languages;
	if (incoming.components & PLAYER_COMPONENT_INTRODUCTIONS)
		materialized->introductions = incoming.introductions;
	if (incoming.components & PLAYER_COMPONENT_TIMERS)
		materialized->timers = incoming.timers;
	if (incoming.components & PLAYER_COMPONENT_UNDEAD_SLOTS)
		materialized->undead_slots = incoming.undead_slots;
	if (incoming.components & PLAYER_COMPONENT_FORGED_ITEMS)
		materialized->forged_items = incoming.forged_items;
	if (incoming.components & PLAYER_COMPONENT_GRANTED_COMMANDS)
		materialized->granted_commands = incoming.granted_commands;
	if (incoming.components & PLAYER_COMPONENT_SKILLS)
		materialized->skills = incoming.skills;
	if (incoming.components & PLAYER_COMPONENT_AFFECTS)
		materialized->affects = incoming.affects;
	if (incoming.components & (PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY))
		materialized->items = incoming.items;
	if (incoming.components & PLAYER_COMPONENT_PETS)
		materialized->pets = incoming.pets;
	if (incoming.components & PLAYER_COMPONENT_SHAPECHANGES)
		materialized->shapes = incoming.shapes;
	if (incoming.components & PLAYER_COMPONENT_TROPHIES)
		materialized->trophies = incoming.trophies;
	materialized->components = PLAYER_CHECKPOINT_COMPONENT_ALL;
	return true;
}
} // namespace

flatfile_player_load_result flatfile_player_snapshot_load(const std::string &root, int32_t pid,
							  player_snapshot *snapshot,
							  std::string *error)
{
	std::lock_guard<std::mutex> guard(player_mutex);
	return load_unlocked(root, pid, snapshot, error);
}

player_save_apply_result flatfile_player_snapshot_apply(const std::string &root,
							const player_snapshot &snapshot,
							std::string *error)
{
	if (!valid_snapshot(snapshot) || !replace_items_together(snapshot.components))
		return { player_save_apply_outcome::terminal_failure, 0, EINVAL };
	std::lock_guard<std::mutex> guard(player_mutex);
	authority_lock authority;
	if (!flatfile_lock_acquire(player_directory(root), player_lock_filename(snapshot.pid),
				   &authority.fd, error))
		return { player_save_apply_outcome::retryable_failure, 0, EIO };
	player_snapshot materialized = {};
	const flatfile_player_load_result loaded =
		load_unlocked(root, snapshot.pid, &materialized, error);
	if (loaded == flatfile_player_load_result::invalid)
		return { player_save_apply_outcome::terminal_failure, 0, EILSEQ };
	if (loaded == flatfile_player_load_result::io_error)
		return { player_save_apply_outcome::retryable_failure, 0, EIO };
	if (loaded == flatfile_player_load_result::not_found)
	{
		if (snapshot.components != PLAYER_CHECKPOINT_COMPONENT_ALL)
			return { player_save_apply_outcome::terminal_failure, 0, ENOENT };
		materialized = snapshot;
	}
	else
	{
		if (materialized.revision >= snapshot.revision)
			return { materialized.revision == snapshot.revision ?
					 player_save_apply_outcome::already_applied :
					 player_save_apply_outcome::stale_revision,
				 materialized.revision, 0 };
		if (!merge_snapshot(snapshot, &materialized))
			return { player_save_apply_outcome::terminal_failure, materialized.revision,
				 EINVAL };
	}
	std::vector<uint8_t> bytes;
	if (!encode_file(&materialized, &bytes))
		return { player_save_apply_outcome::terminal_failure, 0, EINVAL };
	if (!flatfile_atomic_write(player_directory(root), player_filename(snapshot.pid), bytes,
				   error))
		return { player_save_apply_outcome::retryable_failure, 0, EIO };
	return { player_save_apply_outcome::applied, snapshot.revision, 0 };
}

player_save_apply_result flatfile_player_snapshot_apply_selected(const player_snapshot &snapshot,
								 void *context)
{
	(void)context;
	const char *root = persistence_mode_flatfile_root();
	if (!root)
		return { player_save_apply_outcome::terminal_failure, 0, EINVAL };
	std::string error;
	return flatfile_player_snapshot_apply(root, snapshot, &error);
}
