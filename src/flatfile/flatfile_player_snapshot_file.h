#ifndef DURIS_FLATFILE_PLAYER_SNAPSHOT_FILE_H
#define DURIS_FLATFILE_PLAYER_SNAPSHOT_FILE_H

#include "player/player_snapshot.h"

enum class flatfile_player_load_result
{
	ok,
	not_found,
	invalid,
	io_error
};

namespace flatfile_player_snapshot_file
{
constexpr uint32_t player_file_version = 1;
constexpr std::array<uint8_t, 8> player_magic = { 'D', 'U', 'R', 'P', 'L', 'Y', 'R', 0 };
constexpr size_t player_file_maximum = PLAYER_SNAPSHOT_MAX_BYTES + 128;
std::string player_directory(const std::string &root);
std::string player_filename(int32_t pid);
}

// Read one immutable, atomically published file without taking player locks.
// A coin transaction holds authority while checking its legacy pile baseline;
// taking a player lock there would reverse the snapshot writer's lock order.
flatfile_player_load_result flatfile_player_snapshot_read(const std::string &root, int32_t pid,
							  player_snapshot *snapshot,
							  std::string *error);

#endif
