#include "world/zone_story_quest_runtime.h"

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "flatfile/flatfile_zone_story_quest_state.h"
#include "persistence/persistence_mode.h"
#include "sql/sql.h"
#include "sql/zone_story_quest_state_repository.h"
#include "world/zone_story_quest_production.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <limits>
#include <string>
#include <time.h>

namespace zone_story_quest_runtime
{
namespace
{
zone_story_quest_feature::service tracker;
bool tracker_ready = false;
std::atomic<uint64_t> transaction_sequence{ 0 };

bool fail(std::string *error, const char *message)
{
	if (error)
		*error = message;
	return false;
}

bool environment_enabled(const char *name)
{
	const char *value = std::getenv(name);
	return value && (!str_cmp(value, "1") || !str_cmp(value, "true") ||
				 !str_cmp(value, "yes") || !str_cmp(value, "on"));
}

bool load_persisted_state(std::string *error)
{
	std::string encoded;
	if (persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY)
	{
		const auto loaded = flatfile_zone_story_quest_state_load(
			persistence_mode_flatfile_root(), content_revision(), &encoded, error);
		if (loaded == flatfile_zone_story_quest_result::not_found)
			return true;
		if (loaded != flatfile_zone_story_quest_result::ok)
			return false;
	}
	else
	{
		const auto loaded = sql_zone_story_quest_state_load(content_revision(), &encoded, error);
		if (loaded == sql_zone_story_quest_state_result::not_found)
			return true;
		if (loaded != sql_zone_story_quest_state_result::ok)
			return false;
	}
	return tracker.deserialize_state(encoded, error);
}

bool save_persisted_state(std::string *error)
{
	const std::string encoded = tracker.serialize_state(error);
	if (encoded.empty())
		return fail(error, "zone-story state serialization returned an empty document");
	if (persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY)
		return flatfile_zone_story_quest_state_save(persistence_mode_flatfile_root(),
											 content_revision(), encoded, error) ==
		       flatfile_zone_story_quest_result::ok;
	return sql_zone_story_quest_state_save(content_revision(), encoded, error) ==
	       sql_zone_story_quest_state_result::ok;
}

uint32_t environment_season()
{
	const char *value = std::getenv("ZONE_STORY_SEASON_ID");
	if (!value || !*value)
		return 0;
	char *end = nullptr;
	errno = 0;
	const unsigned long parsed = std::strtoul(value, &end, 10);
	if (errno || end == value || *end || parsed == 0 || parsed > std::numeric_limits<uint32_t>::max())
		return 0;
	return static_cast<uint32_t>(parsed);
}

std::string new_transaction_id(uint32_t season_id, uint32_t pid, std::string_view definition_id,
					int64_t completed_at)
{
	const uint64_t sequence = ++transaction_sequence;
	return "zone-story:" + std::to_string(season_id) + ":" + std::to_string(pid) + ":" +
	       std::string(definition_id) + ":" + std::to_string(completed_at) + ":" +
	       std::to_string(sequence);
}
} // namespace

uint32_t current_season_id()
{
	const uint64_t sql_epoch = sql_season_epoch();
	if (sql_epoch > 0 && sql_epoch <= std::numeric_limits<uint32_t>::max())
		return static_cast<uint32_t>(sql_epoch);
	const uint32_t configured = environment_season();
	return configured ? configured : 1;
}

uint32_t content_revision()
{
	return zone_story_quest_production::ZONE_STORY_QUEST_PRODUCTION_CONTENT_REVISION;
}

bool bootstrap(std::string *error)
{
	std::string production_error;
	if (!zone_story_quest_production::bootstrap(content_revision(), &production_error))
	{
		tracker_ready = false;
		if (error)
			*error = production_error;
		return false;
	}
	tracker = zone_story_quest_feature::service(zone_story_quest_production::runtime_catalog());
	zone_story_quest_feature::daily_policy policy;
	policy.enabled = environment_enabled("ZONE_STORY_DAILY_ENABLED");
	tracker.set_daily_policy(policy);
	if (!load_persisted_state(error))
	{
		tracker_ready = false;
		return false;
	}
	tracker_ready = true;
	if (error)
		error->clear();
	return true;
}

bool ready()
{
	return tracker_ready && zone_story_quest_production::ready();
}

zone_story_quest_feature::service *service()
{
	return ready() ? &tracker : nullptr;
}

bool persist(std::string *error)
{
	return ready() && save_persisted_state(error);
}

bool remember_character(P_char player, std::string *error)
{
	if (!ready() || !player || IS_NPC(player))
		return fail(error, "zone-story character identity is unavailable");
	const std::string before = tracker.serialize_state(error);
	tracker.remember_character(current_season_id(), static_cast<uint32_t>(GET_PID(player)),
					  GET_NAME(player));
	const std::string after = tracker.serialize_state(error);
	if (before == after)
		return true;
	if (save_persisted_state(error))
		return true;
	std::string restore_error;
	tracker.deserialize_state(before, &restore_error);
	return false;
}

bool erase_character(uint32_t pid, std::string *error)
{
	if (!ready() || !pid)
		return fail(error, "zone-story character identity is unavailable");
	const std::string before = tracker.serialize_state(error);
	if (!tracker.erase_character_all_seasons(pid, current_season_id()))
		return fail(error, "zone-story character identity is invalid");
	if (save_persisted_state(error))
		return true;
	std::string restore_error;
	tracker.deserialize_state(before, &restore_error);
	return false;
}

std::string render_daily(P_char player, bool colors, std::string *error)
{
	if (!ready() || !player || IS_NPC(player))
		return fail(error, "zone-story daily service is unavailable"),
		       std::string("Daily zone-story quests are unavailable until catalog/persistence boot completes.\r\n");
	const std::string before = tracker.serialize_state(error);
	tracker.remember_character(current_season_id(), static_cast<uint32_t>(GET_PID(player)),
					  GET_NAME(player));
	std::string output = tracker.render_daily(
		current_season_id(), static_cast<uint32_t>(GET_PID(player)), GET_LEVEL(player),
		GET_RACEWAR(player), static_cast<int64_t>(time(NULL)), colors);
	if (save_persisted_state(error))
		return output;
	std::string restore_error;
	tracker.deserialize_state(before, &restore_error);
	return "Daily zone-story quest state could not be saved; no assignment was committed.\r\n";
}

bool record_authoritative_completion(
	std::string_view definition_id, int32_t zone_number, uint32_t direct_completer_pid,
	const std::vector<uint32_t> &credited_pids, int32_t room_vnum, int64_t completed_at,
	std::string_view character_name, int level, int racewar, bool party_context_known,
	uint32_t party_size, int strongest_party_level, std::string *error,
	std::string_view transaction_id)
{
	if (!ready())
		return fail(error, "zone-story quest runtime is not ready");
	if (definition_id.empty() || !direct_completer_pid || credited_pids.empty() || room_vnum <= 0 ||
	    completed_at <= 0)
		return fail(error, "invalid authoritative zone-story completion context");
	const auto *definition = tracker.catalog().definitions.empty() ? nullptr : [&]() {
		for (const auto &candidate : tracker.catalog().definitions)
			if (candidate.definition_id == definition_id)
				return &candidate;
		return static_cast<const zone_story_quest_tracking::quest_definition *>(nullptr);
	}();
	if (!definition)
		return fail(error, "zone-story completion definition is not in the production catalog");
	zone_story_quest_feature::completion_event event;
	event.transaction.schema_version =
		zone_story_quest_tracking::ZONE_STORY_QUEST_TRACKING_SCHEMA_VERSION;
	event.transaction.transaction_id = transaction_id.empty() ?
						 new_transaction_id(current_season_id(), direct_completer_pid,
									definition_id, completed_at) :
						 std::string(transaction_id);
	event.transaction.quest_definition_id = definition_id;
	event.transaction.zone_number = zone_number;
	event.transaction.direct_completer_pid = direct_completer_pid;
	event.transaction.credited_pids = credited_pids;
	event.transaction.room_vnum = room_vnum;
	event.transaction.completed_at = completed_at;
	event.transaction.season_id = current_season_id();
	event.transaction.content_revision = definition->content_revision;
	event.character_name = character_name;
	event.level = level;
	event.racewar = racewar;
	event.party_context_known = party_context_known;
	event.party_size = party_size;
	event.strongest_party_level = strongest_party_level;
	const std::string before = tracker.serialize_state(error);
	const auto recorded = tracker.record_completion(event, error);
	if (recorded != zone_story_quest_feature::result::applied &&
	    recorded != zone_story_quest_feature::result::already_applied)
		return false;
	if (!save_persisted_state(error))
	{
		std::string restore_error;
		tracker.deserialize_state(before, &restore_error);
		return false;
	}
	return true;
}

bool record_legacy_completion(struct char_data *player, const quest_complete_data *completion,
				      int32_t room_vnum, int64_t completed_at, std::string *error)
{
	if (!player || IS_NPC(player) || !completion)
		return fail(error, "legacy zone-story completion requires a player and Q block");
	const std::string *definition_id = zone_story_quest_production::definition_id_for(completion);
	if (!definition_id)
		return fail(error, "legacy Q block is not bound to the production catalog");
	int32_t zone_number = 0;
	for (const auto &definition : tracker.catalog().definitions)
		if (definition.definition_id == *definition_id)
		{
			zone_number = definition.zone_number;
			break;
		}
	const uint32_t pid = static_cast<uint32_t>(GET_PID(player));
	std::vector<uint32_t> credited_pids{ pid };
	int strongest_party_level = GET_LEVEL(player);
	if (player->group)
	{
		for (group_list *member = player->group; member; member = member->next)
		{
			if (!member->ch || !IS_PC(member->ch) || member->ch->in_room != player->in_room)
				continue;
			const uint32_t member_pid = static_cast<uint32_t>(GET_PID(member->ch));
			if (!member_pid)
				continue;
			bool already_captured = false;
			for (const uint32_t captured_pid : credited_pids)
				if (captured_pid == member_pid)
				{
					already_captured = true;
					break;
				}
			if (already_captured)
				continue;
			credited_pids.push_back(member_pid);
			strongest_party_level = std::max(strongest_party_level, GET_LEVEL(member->ch));
		}
	}
	const uint32_t party_size = static_cast<uint32_t>(credited_pids.size());
	return record_authoritative_completion(*definition_id, zone_number, pid,
						credited_pids, room_vnum, completed_at,
						GET_NAME(player), GET_LEVEL(player), GET_RACEWAR(player), true,
						party_size, strongest_party_level, error);
}
} // namespace zone_story_quest_runtime
