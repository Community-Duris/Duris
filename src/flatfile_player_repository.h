#ifndef DURIS_FLATFILE_PLAYER_REPOSITORY_H
#define DURIS_FLATFILE_PLAYER_REPOSITORY_H

#include "player_save_worker.h"

#include <string>

enum class flatfile_player_load_result
{
	ok,
	not_found,
	invalid,
	io_error
};

flatfile_player_load_result flatfile_player_snapshot_load(const std::string &root, int32_t pid,
							  player_snapshot *snapshot,
							  std::string *error);
player_save_apply_result flatfile_player_snapshot_apply(const std::string &root,
							const player_snapshot &snapshot,
							std::string *error);
player_save_apply_result flatfile_player_snapshot_apply_selected(const player_snapshot &snapshot,
								 void *context);

#endif
