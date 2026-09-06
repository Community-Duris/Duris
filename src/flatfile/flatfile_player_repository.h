#ifndef DURIS_FLATFILE_PLAYER_REPOSITORY_H
#define DURIS_FLATFILE_PLAYER_REPOSITORY_H

#include "flatfile/flatfile_authority_transaction.h"
#include "flatfile/flatfile_player_snapshot_file.h"
#include "player/player_load_repository.h"
#include "player/player_save_worker.h"

#include <memory>
#include <string>

class flatfile_player_snapshot_lock
{
    public:
	flatfile_player_snapshot_lock() noexcept;
	~flatfile_player_snapshot_lock();
	flatfile_player_snapshot_lock(const flatfile_player_snapshot_lock &) = delete;
	flatfile_player_snapshot_lock &operator=(const flatfile_player_snapshot_lock &) = delete;

	bool acquire(const std::string &root, int32_t pid, std::string *error);
	bool matches(const std::string &root, int32_t pid) const;

    private:
	struct state;
	std::unique_ptr<state> state_;
	bool owns(const std::string &root, int32_t pid) const;
};

flatfile_player_load_result flatfile_player_snapshot_load(const std::string &root, int32_t pid,
							  player_snapshot *snapshot,
							  std::string *error);
player_load_result flatfile_player_load_repository_execute(const std::string &root,
							   const player_load_request &request);
player_load_result
flatfile_player_load_repository_execute_selected(const player_load_request &request, void *context);
player_save_apply_result flatfile_player_snapshot_apply(const std::string &root,
							const player_snapshot &snapshot,
							std::string *error);
player_save_apply_result flatfile_player_snapshot_apply_selected(const player_snapshot &snapshot,
								 void *context);
flatfile_player_load_result flatfile_player_snapshot_prepare_remove(
	const std::string &root, const flatfile_player_snapshot_lock &snapshot_lock,
	const flatfile_authority_lock &authority_lock, int32_t pid,
	flatfile_authority_operation *operation, std::string *error);

#endif
