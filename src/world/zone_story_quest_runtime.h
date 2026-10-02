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
struct frozen_daily_context
{
	uint32_t season_id = 0;
	uint32_t catalog_revision = 0;
	uint32_t policy_revision = 0;
	std::vector<uint32_t> eligible_pids;
};
/* Temporary placements (map previews and remote inspection) never discover areas. */
class temporary_placement
{
    public:
	temporary_placement();
	~temporary_placement();
	temporary_placement(const temporary_placement &) = delete;
	temporary_placement &operator=(const temporary_placement &) = delete;
};
void arrived(struct char_data *player);
void encountered(struct char_data *player, struct char_data *npc);
bool daily_eligible(struct char_data *player, std::string_view definition_id,
		    int strongest_party_level, int64_t completed_at);
bool bootstrap(std::string *error = nullptr);
/* Loaded receipt/cleanup authority. Player surfaces and journey admission also
 * require active economic accounting; service() returns nullptr while inactive. */
bool ready();
uint32_t current_season_id();
uint32_t content_revision();
zone_story_quest_feature::service *service();
bool persist(std::string *error = nullptr);
bool remember_character(struct char_data *player, std::string *error = nullptr);
bool erase_character(uint32_t pid, std::string *error = nullptr);
std::string render_daily(struct char_data *player, bool colors, std::string *error = nullptr);
std::string render_daily_score(struct char_data *player, bool colors, std::string *error = nullptr);
std::string render_journal(struct char_data *player, int32_t zone_number, bool daily_only,
			   bool colors);

/* The caller supplies the exact recipient set captured at the completion
 * boundary.  This function never scans the room or group later. */
bool record_authoritative_completion(
	std::string_view definition_id, int32_t zone_number, uint32_t direct_completer_pid,
	const std::vector<uint32_t> &credited_pids, int32_t room_vnum, int64_t completed_at,
	std::string_view character_name, int level, int racewar, bool party_context_known,
	uint32_t party_size, int strongest_party_level, std::string *error = nullptr,
	std::string_view transaction_id = {}, const frozen_daily_context *daily = nullptr);

/* Legacy static Q completion hook: snapshot the same-room PC group at the
 * completion boundary. The direct player remains the leadership recipient;
 * later group changes cannot alter the recorded recipient set. */
bool record_legacy_completion(struct char_data *player, const quest_complete_data *completion,
			      int32_t room_vnum, int64_t completed_at, std::string *error = nullptr,
			      std::string_view transaction_id = {});
} // namespace zone_story_quest_runtime

#endif
