#ifndef ZONE_STORY_QUEST_RUNTIME_H
#define ZONE_STORY_QUEST_RUNTIME_H

#include "world/zone_story_quest_feature.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

struct char_data;
struct quest_complete_data;

namespace zone_story_quest_runtime
{
bool bootstrap(std::string *error = nullptr);
bool ready();
uint32_t current_season_id();
uint32_t content_revision();
zone_story_quest_feature::service *service();
bool persist(std::string *error = nullptr);
bool remember_character(struct char_data *player, std::string *error = nullptr);
bool erase_character(uint32_t pid, std::string *error = nullptr);
std::string render_daily(struct char_data *player, bool colors, std::string *error = nullptr);

/* The caller supplies the exact recipient set captured at the completion
 * boundary.  This function never scans the room or group later. */
bool record_authoritative_completion(std::string_view definition_id, int32_t zone_number,
				     uint32_t direct_completer_pid,
				     const std::vector<uint32_t> &credited_pids, int32_t room_vnum,
				     int64_t completed_at, std::string_view character_name,
				     int level, int racewar, bool party_context_known,
				     uint32_t party_size, int strongest_party_level,
				     std::string *error = nullptr,
				     std::string_view transaction_id = {});

/* Legacy static Q completion hook: snapshot the same-room PC group at the
 * completion boundary. The direct player remains the leadership recipient;
 * later group changes cannot alter the recorded recipient set. */
bool record_legacy_completion(struct char_data *player, const quest_complete_data *completion,
			      int32_t room_vnum, int64_t completed_at,
			      std::string *error = nullptr);
} // namespace zone_story_quest_runtime

#endif
