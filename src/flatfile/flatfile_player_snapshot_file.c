#include "flatfile/flatfile_player_snapshot_file.h"
#include "flatfile/flatfile_native_mobile_birth_ordinary_physical.h"
#include "economy/native_mobile_birth_cash_role_command.h"
#include <algorithm>
#include <cerrno>
#include <charconv>
#include <dirent.h>
#include <fcntl.h>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

#include "flatfile/flatfile_store.h"
#include "player/player_snapshot_codec.h"

#include <cstring>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>
#include <utility>

namespace
{
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

}

namespace flatfile_player_snapshot_file
{
std::string player_directory(const std::string &root)
{
	return root + "/players";
}

std::string player_filename(int32_t pid)
{
	return std::to_string(pid) + ".snapshot";
}

std::string death_directory(const std::string &root)
{
	return root + "/player-deaths";
}

std::string death_filename(int32_t pid, uint64_t revision)
{
	return std::to_string(pid) + "-" + std::to_string(revision) + ".death";
}
}

using namespace flatfile_player_snapshot_file;

bool flatfile_player_snapshot_encode_file(const player_snapshot &snapshot,
					  std::vector<uint8_t> *bytes)
{
	if (!bytes || snapshot.components != PLAYER_CHECKPOINT_COMPONENT_ALL)
		return false;
	try
	{
		player_snapshot normalized = snapshot;
		normalized.encoded_size_bound = PLAYER_SNAPSHOT_MAX_BYTES;
		std::vector<uint8_t> payload;
		if (player_snapshot_encode(normalized, &payload) !=
		    player_snapshot_codec_result::ok)
			return false;
		normalized.encoded_size_bound = payload.size();
		if (player_snapshot_encode(normalized, &payload) !=
		    player_snapshot_codec_result::ok)
			return false;
		std::vector<uint8_t> candidate;
		candidate.reserve(payload.size() + 68);
		candidate.insert(candidate.end(), player_magic.begin(), player_magic.end());
		auto number = [&](uint64_t value, size_t size)
		{
			for (size_t index = 0; index < size; ++index)
				candidate.push_back(static_cast<uint8_t>(value >> (index * 8)));
		};
		number(player_file_version, 4);
		number(payload.size(), 4);
		number(static_cast<uint32_t>(normalized.pid), 4);
		number(normalized.revision, 8);
		number(normalized.components, 8);
		std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
		SHA256(payload.data(), payload.size(), digest.data());
		candidate.insert(candidate.end(), digest.begin(), digest.end());
		candidate.insert(candidate.end(), payload.begin(), payload.end());
		if (candidate.size() > player_file_maximum)
			return false;
		*bytes = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

flatfile_player_load_result flatfile_player_snapshot_read(const std::string &root, int32_t pid,
							  player_snapshot *snapshot,
							  std::string *error)
{
	return flatfile_player_snapshot_read_file(player_directory(root), player_filename(pid), pid,
						  snapshot, error);
}

flatfile_player_load_result
flatfile_player_snapshot_read_file(const std::string &directory, const std::string &filename,
				   int32_t pid, player_snapshot *snapshot, std::string *error)
{
	if (pid <= 0 || !snapshot)
		return flatfile_player_load_result::invalid;
	std::vector<uint8_t> bytes;
	const flatfile_read_result read =
		flatfile_read(directory, filename, player_file_maximum, &bytes, error);
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

unsigned int flatfile_native_mobile_birth_ordinary_player_physical_storage::verify_locked(
	const std::string &root, const flatfile_identity_lock &identity_lock,
	const flatfile_authority_lock &authority_lock,
	const critical_native_recovery_envelope &original,
	flatfile_native_mobile_birth_ordinary_player_physical_absence *output,
	std::string *error) noexcept
{
	if (root.empty() || !output || !identity_lock.matches(root) ||
	    !authority_lock.matches(root))
		return EINVAL;
	try
	{
		if (!native_mobile_birth_cash_role_recovery_valid(original))
			return EINVAL;
		quest_mobile_native_image image;
		std::vector<native_mobile_birth_item_recipe> recipes;
		native_mobile_birth_cash_role_recipe role;
		economic_frozen_intent intent;
		if (native_mobile_birth_cash_role_command_decode(original.command, &image, &recipes,
								 &role) !=
			    economic_accounting_error::ok ||
		    role.role != native_mobile_birth_cash_role::ordinary_wallet ||
		    economic_intent_decode(original.command.accounting_intent, &intent) !=
			    economic_accounting_error::ok ||
		    economic_intent_verify_binding(original.command, intent) !=
			    economic_accounting_error::ok ||
		    !intent.admission.metadata.source_event)
			return EINVAL;
		std::vector<uint64_t> born;
		born.reserve(image.items.size());
		for (const auto &literal : image.items)
			born.push_back(literal.object_uid);
		std::sort(born.begin(), born.end());
		if (std::adjacent_find(born.begin(), born.end()) != born.end() ||
		    (!born.empty() && !born.front()))
			return EINVAL;
		flatfile_native_mobile_birth_ordinary_identity_current identities;
		const auto identity_status =
			flatfile_native_mobile_birth_ordinary_identity_storage::read_locked(
				root, identity_lock, authority_lock, &identities, error);
		if (identity_status != flatfile_identity_result::ok)
			return identity_status == flatfile_identity_result::io_error ? EIO : EILSEQ;
		flatfile_native_mobile_birth_ordinary_player_physical_absence observed;
		observed.identity_catalog_revision = identities.catalog_revision;
		observed.identities = identities.records.size();
		for (const auto &identity : identities.records)
			if (!identity.active)
				++observed.retired_identities;

		const std::string directory = flatfile_player_snapshot_file::player_directory(root);
		std::vector<int32_t> pids;
		{
			// Enumerate actual canonical files, not only current/active identities.
			// The original writer holds authority through snapshot rename; do not
			// acquire player locks under authority and reverse its lock order.
			const int descriptor = open(
				directory.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
			if (descriptor < 0)
				return errno ? static_cast<unsigned int>(errno) : EIO;
			struct stat metadata
			{
			};
			if (fstat(descriptor, &metadata) < 0)
			{
				const int saved = errno;
				close(descriptor);
				return saved ? static_cast<unsigned int>(saved) : EIO;
			}
			if (!S_ISDIR(metadata.st_mode) || metadata.st_uid != geteuid() ||
			    (metadata.st_mode & 0077))
			{
				close(descriptor);
				return EILSEQ;
			}
			DIR *stream = fdopendir(descriptor);
			if (!stream)
			{
				const int saved = errno;
				close(descriptor);
				return saved ? static_cast<unsigned int>(saved) : EIO;
			}
			struct directory_owner
			{
				DIR *stream;
				~directory_owner() { closedir(stream); }
			} owned{ stream };
			for (;;)
			{
				errno = 0;
				const auto *entry = readdir(stream);
				if (!entry)
				{
					if (errno)
						return static_cast<unsigned int>(errno);
					break;
				}
				const std::string_view name(entry->d_name);
				constexpr std::string_view suffix = ".snapshot";
				if (!name.ends_with(suffix))
					continue; // Original lock/atomic temporary names are not snapshots.
				const auto numeric = name.substr(0, name.size() - suffix.size());
				int32_t pid = 0;
				const auto parsed = std::from_chars(
					numeric.data(), numeric.data() + numeric.size(), pid);
				if (parsed.ec != std::errc{} ||
				    parsed.ptr != numeric.data() + numeric.size() || pid <= 0 ||
				    flatfile_player_snapshot_file::player_filename(pid) != name)
					return EILSEQ;
				pids.push_back(pid);
			}
		}
		std::sort(pids.begin(), pids.end());
		if (std::adjacent_find(pids.begin(), pids.end()) != pids.end())
			return EILSEQ;
		for (int32_t pid : pids)
		{
			player_snapshot snapshot{};
			const auto loaded = flatfile_player_snapshot_read_file(
				directory, flatfile_player_snapshot_file::player_filename(pid), pid,
				&snapshot, error);
			// An enumerated canonical file disappearing under this genuine freeze
			// is not a harmless missing identity or an accepted empty inventory.
			if (loaded != flatfile_player_load_result::ok)
				return loaded == flatfile_player_load_result::io_error ?
					       EIO :
					       (loaded == flatfile_player_load_result::not_found ?
							ENOENT :
							EILSEQ);
			++observed.snapshots;
			const auto identity = std::lower_bound(identities.records.begin(),
							       identities.records.end(), pid,
							       [](const auto &value, int32_t sought)
							       { return value.pid < sought; });
			if (identity == identities.records.end() || identity->pid != pid)
				++observed.unindexed_snapshots;
			for (const auto &item : snapshot.items)
			{
				if (std::binary_search(born.begin(), born.end(), item.object_uid))
					return EEXIST;
				++observed.inventory_rows;
			}
			for (const auto &pet : snapshot.pets)
				for (const auto &item : pet.items)
				{
					if (std::binary_search(born.begin(), born.end(),
							       item.object_uid))
						return EEXIST;
					++observed.pet_item_rows;
				}
		}
		if (!identity_lock.matches(root) || !authority_lock.matches(root))
			return EINVAL;
		*output = observed;
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
}
