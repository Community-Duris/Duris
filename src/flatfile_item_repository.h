#ifndef DURIS_FLATFILE_ITEM_REPOSITORY_H
#define DURIS_FLATFILE_ITEM_REPOSITORY_H

#include "critical_command_coordinator.h"
#include "item_transfer_command.h"

#include <cstdint>
#include <string>
#include <vector>

struct flatfile_item_ownership_record
{
	uint64_t item_uid = 0;
	uint64_t root_item_uid = 0;
	uint64_t parent_item_uid = 0;
	item_owner_identity owner = { item_owner_type::unknown, 0, 0 };
	uint64_t item_revision = 0;
	int32_t vnum = 0;
	item_custody_state state = item_custody_state::absent;
};

enum class flatfile_item_repository_result
{
	ok,
	not_found,
	invalid,
	io_error
};

flatfile_item_repository_result flatfile_item_repository_load_owner(
	const std::string &root, const item_owner_identity &owner, uint64_t *owner_revision,
	std::vector<flatfile_item_ownership_record> *items, std::string *error);
critical_apply_result flatfile_item_repository_apply(const std::string &root,
						     const critical_command &command);
critical_apply_result
flatfile_critical_command_repository_apply_selected(const critical_command &command, void *context);

#endif
