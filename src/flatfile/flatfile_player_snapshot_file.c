#include "flatfile/flatfile_player_snapshot_file.h"

#include "flatfile/flatfile_store.h"
#include "player/player_snapshot_codec.h"

#include <cstring>
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
