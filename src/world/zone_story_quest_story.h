#ifndef ZONE_STORY_QUEST_STORY_H
#define ZONE_STORY_QUEST_STORY_H

#include "world/zone_story_quest_catalog.h"

namespace zone_story_quest_story
{
/* Optional per-area sidecars. Malformed present files fail closed; absent
 * files leave the native catalog unchanged. Applying a file is atomic. */
bool apply(const std::string &json, std::string_view source_area,
	   zone_story_quest_catalog::catalog *catalog, std::string *error = nullptr);
bool load(zone_story_quest_catalog::catalog *catalog, std::string *error = nullptr,
	  const std::string &directory = "areas/story");
} // namespace zone_story_quest_story

#endif
