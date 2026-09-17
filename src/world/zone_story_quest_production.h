#ifndef ZONE_STORY_QUEST_PRODUCTION_H
#define ZONE_STORY_QUEST_PRODUCTION_H

#include "world/zone_story_quest_catalog.h"

#include <cstdint>
#include <string>

struct quest_complete_data;

namespace zone_story_quest_production
{
constexpr uint32_t ZONE_STORY_QUEST_PRODUCTION_CONTENT_REVISION = 1;

/* Build the catalog from the booted static quest index.  The catalog is
 * intentionally separate from bartender/random world quest assignment. */
zone_story_quest_catalog::catalog build_runtime_catalog(uint32_t content_revision,
								std::string *error = nullptr);

/* Publish the boot-time catalog and bind each Q completion block to its stable
 * definition identity for the authoritative completion hook. */
bool bootstrap(uint32_t content_revision = ZONE_STORY_QUEST_PRODUCTION_CONTENT_REVISION,
	       std::string *error = nullptr);
const zone_story_quest_catalog::catalog &runtime_catalog();
const std::string *definition_id_for(const quest_complete_data *completion);
bool ready();

int zone_for_giver_vnum(int giver_vnum);
std::string canonical_completion_key(const quest_complete_data &completion);
} // namespace zone_story_quest_production

#endif
