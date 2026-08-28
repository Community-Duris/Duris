#ifndef DURIS_FLATFILE_BOON_REPOSITORY_H
#define DURIS_FLATFILE_BOON_REPOSITORY_H

#include "boon_reward_command.h"
#include "boon_shop_command.h"
#include "flatfile_authority_transaction.h"
#include "critical_command_coordinator.h"

#include <cstdint>
#include <string>
#include <vector>

struct flatfile_boon_definition
{
	uint32_t id = 0;
	int64_t start_time = 0;
	int64_t duration = 0;
	uint8_t racewar = 0;
	uint8_t type = 0;
	uint8_t option = 0;
	double criteria = 0;
	double criteria2 = 0;
	double bonus = 0;
	double bonus2 = 0;
	bool random = false;
	bool active = false;
	uint32_t target_pid = 0;
	bool repeat = false;
	std::string author;
};

struct flatfile_boon_player_projection
{
	int64_t points = 0;
	int64_t stats = 0;
};

struct flatfile_boon_pending_reward
{
	critical_operation_id operation_id = {};
	uint32_t pid = 0;
	double event_data = 0;
	boon_reward_result result = {};
};

enum class flatfile_boon_result
{
	ok,
	not_found,
	invalid,
	io_error,
	already_exists,
};

flatfile_boon_result
flatfile_boon_establish(const std::string &root,
			const std::vector<flatfile_boon_definition> &definitions,
			std::string *error);
flatfile_boon_result flatfile_boon_create(const std::string &root,
					  flatfile_boon_definition *definition, std::string *error);
flatfile_boon_result flatfile_boon_deactivate(const std::string &root, uint32_t boon_id,
					      std::string *error);
flatfile_boon_result flatfile_boon_extend(const std::string &root, uint32_t boon_id,
					  int64_t extend_minutes, int64_t now,
					  const std::string &author, bool *was_active,
					  std::string *error);
flatfile_boon_result
flatfile_boon_load_definitions(const std::string &root,
			       std::vector<flatfile_boon_definition> *definitions,
			       std::string *error);
flatfile_boon_result flatfile_boon_load_progress(const std::string &root, uint32_t boon_id,
						 uint32_t pid, double *counter, std::string *error);
flatfile_boon_result flatfile_boon_load_player(const std::string &root, uint32_t pid,
					       flatfile_boon_player_projection *player,
					       std::string *error);
flatfile_boon_result flatfile_boon_find_pending_reward(const std::string &root, uint32_t pid,
						       flatfile_boon_pending_reward *reward,
						       std::string *error);
flatfile_boon_result flatfile_boon_acknowledge_reward(const std::string &root,
						      const critical_operation_id &operation_id,
						      std::string *error);
flatfile_boon_result flatfile_boon_prepare_player_remove(const std::string &root,
							 const flatfile_authority_lock &lock,
							 uint32_t pid,
							 flatfile_authority_operation *operation,
							 std::string *error);
critical_apply_result flatfile_boon_repository_apply(const std::string &root,
						     const critical_command &command);
critical_apply_result flatfile_boon_shop_repository_apply(const std::string &root,
							  const critical_command &command);

#endif
