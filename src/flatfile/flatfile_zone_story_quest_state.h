#ifndef DURIS_FLATFILE_ZONE_STORY_QUEST_STATE_H
#define DURIS_FLATFILE_ZONE_STORY_QUEST_STATE_H

#include "flatfile/flatfile_authority_transaction.h"

#include <cstdint>
#include <string>

enum class flatfile_zone_story_quest_result
{
	ok,
	not_found,
	invalid,
	corrupt,
	io_error,
	unchanged,
};

flatfile_zone_story_quest_result
flatfile_zone_story_quest_state_load(const char *root, uint32_t expected_catalog_revision,
				     std::string *state, std::string *error = nullptr);
flatfile_zone_story_quest_result flatfile_zone_story_quest_state_save(const char *root,
								      uint32_t catalog_revision,
								      const std::string &state,
								      std::string *error = nullptr);

/* Borrow the character deletion lock; stage alias/history erasure in its journal. */
flatfile_zone_story_quest_result flatfile_zone_story_quest_state_prepare_player_remove(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t catalog_revision,
	uint32_t pid, flatfile_authority_operation *operation, std::string *error = nullptr);

#endif
