#ifndef DURIS_FLATFILE_ITEM_REPOSITORY_H
#define DURIS_FLATFILE_ITEM_REPOSITORY_H

#include "auction_command.h"
#include "flatfile_authority_transaction.h"
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

enum class flatfile_item_baseline_result
{
	applied,
	already_applied,
	conflict,
	invalid,
	io_error
};

struct flatfile_item_auction_mutation
{
	uint64_t player_owner_revision = 0;
	uint64_t auction_owner_revision = 0;
	uint16_t item_count = 0;
	std::array<uint64_t, AUCTION_COMMAND_MAX_ITEMS> item_uids = {};
	std::array<uint64_t, AUCTION_COMMAND_MAX_ITEMS> item_revisions = {};
	flatfile_authority_after_image after_image;
};

flatfile_item_repository_result flatfile_item_repository_load_owner(
	const std::string &root, const item_owner_identity &owner, uint64_t *owner_revision,
	std::vector<flatfile_item_ownership_record> *items, std::string *error);
flatfile_item_baseline_result
flatfile_item_repository_establish_owner(const std::string &root, const item_owner_identity &owner,
					 const std::vector<flatfile_item_ownership_record> &items,
					 std::string *error);
critical_apply_result flatfile_item_repository_apply(const std::string &root,
						     const critical_command &command);
flatfile_item_repository_result flatfile_item_repository_prepare_auction_transfer(
	const std::string &root, const flatfile_authority_lock &lock,
	const auction_command_payload &payload, uint32_t auction_id, bool to_auction,
	flatfile_item_auction_mutation *mutation, unsigned int *result_code, std::string *error);
critical_apply_result
flatfile_critical_command_repository_apply_selected(const critical_command &command, void *context);

#endif
