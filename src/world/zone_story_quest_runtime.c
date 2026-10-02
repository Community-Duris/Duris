#include "world/zone_story_quest_runtime.h"

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/comm.h"
#include "ships/ships.h"
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

extern P_room world;
extern struct zone_data *zone_table;

namespace zone_story_quest_runtime
{
namespace
{
zone_story_quest_feature::service tracker;
bool tracker_ready = false;
unsigned temporary_placement_depth = 0;

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

bool save_persisted_state(std::string *error)
{
	const auto updates = tracker.changes_for_persistence();
	if (updates.values.empty() && !updates.replace)
		return true;
	const bool saved =
		persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY ?
			flatfile_zone_story_quest_records_save(persistence_mode_flatfile_root(),
							       content_revision(), updates,
							       error) ==
				flatfile_zone_story_quest_result::ok :
			sql_zone_story_quest_records_save(content_revision(), updates, error) ==
				sql_zone_story_quest_state_result::ok;
	if (saved)
		tracker.mark_persisted();
	return saved;
}

bool load_persisted_state(std::string *error)
{
	std::string encoded;
	bool legacy = false;
	if (persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY)
	{
		const auto loaded = flatfile_zone_story_quest_state_load(
			persistence_mode_flatfile_root(), content_revision(), &encoded, error,
			&legacy);
		if (loaded == flatfile_zone_story_quest_result::not_found)
			return true;
		if (loaded != flatfile_zone_story_quest_result::ok)
			return false;
	}
	else
	{
		const auto loaded = sql_zone_story_quest_state_load(content_revision(), &encoded,
								    error, &legacy);
		if (loaded == sql_zone_story_quest_state_result::not_found)
			return true;
		if (loaded != sql_zone_story_quest_state_result::ok)
			return false;
	}
	if (!tracker.deserialize_state(encoded, error))
		return false;
	if (legacy)
		return save_persisted_state(error);
	tracker.mark_persisted();
	return true;
}

uint32_t environment_season()
{
	const char *value = std::getenv("ZONE_STORY_SEASON_ID");
	if (!value || !*value)
		return 0;
	char *end = nullptr;
	errno = 0;
	const unsigned long parsed = std::strtoul(value, &end, 10);
	if (errno || end == value || *end || parsed == 0 ||
	    parsed > std::numeric_limits<uint32_t>::max())
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

std::string render_daily_surface(P_char player, bool colors, bool score, std::string *error)
{
	if (!ready() || !player || IS_NPC(player))
	{
		if (error)
			*error = "zone-story daily service is unavailable";
		return {};
	}
	/* Disabled daily quests are intentionally absent from every ordinary-player
	 * surface.  In particular, do not remember a character or create an
	 * assignment merely because score/quest was viewed. */
	if (!tracker.get_daily_policy().enabled)
		return {};
	const int64_t now = static_cast<int64_t>(time(NULL));
	return score ? tracker.render_daily_score(current_season_id(), GET_PID(player),
						  GET_LEVEL(player), GET_RACEWAR(player), now,
						  colors) :
		       tracker.render_daily(current_season_id(), GET_PID(player), GET_LEVEL(player),
					    GET_RACEWAR(player), now, colors);
}
} // namespace

temporary_placement::temporary_placement()
{
	++temporary_placement_depth;
}
temporary_placement::~temporary_placement()
{
	--temporary_placement_depth;
}

bool daily_eligible(P_char player, std::string_view id, int strongest, int64_t now)
{
	return ready() && player && IS_PC(player) && !IS_TRUSTED(player) &&
	       tracker.daily_eligible_for(current_season_id(), GET_PID(player), id,
					  GET_LEVEL(player), GET_RACEWAR(player), strongest, now);
}

std::string render_journal(P_char player, int32_t zone_number, bool daily_only, bool colors)
{
	if (!ready() || !player || !IS_PC(player))
		return "The quest journal is unavailable.\r\n";
	zone_story_quest_catalog::journal_inventory inventory;
	for (P_obj item = player->carrying; item; item = item->next_content)
		++inventory.carried[OBJ_VNUM(item)];
	for (int slot = 0; slot < MAX_WEAR; ++slot)
		if (player->equipment[slot])
			inventory.equipped.emplace(slot, OBJ_VNUM(player->equipment[slot]));
	return tracker.render_journal(current_season_id(), GET_PID(player), zone_number,
				      GET_LEVEL(player), GET_RACEWAR(player),
				      static_cast<int64_t>(time(nullptr)), daily_only, colors,
				      &inventory);
}

void arrived(P_char player)
{
	if (!ready() || temporary_placement_depth || !player || !IS_ALIVE(player) ||
	    !IS_PC(player) || IS_TRUSTED(player) || GET_PID(player) <= 0 || !player->desc ||
	    player->desc->connected != CON_PLAYING || player->desc->character != player ||
	    player->in_room < 0 || IS_SHIP_ROOM(player->in_room) ||
	    IS_ROOM(player->in_room, ROOM_ARENA))
		return;
	const int index = world[player->in_room].zone;
	const int32_t number = zone_table[index].number;
	if (tracker.has_discovered(current_season_id(), GET_PID(player), number))
		return;
	const auto undo = tracker.checkpoint_for(current_season_id(),
						 { static_cast<uint32_t>(GET_PID(player)) });
	std::string error;
	const auto discovered = tracker.discover_zone(current_season_id(), GET_PID(player), number,
						      world[player->in_room].number,
						      static_cast<int64_t>(time(NULL)), "arrival",
						      &error);
	if (discovered != zone_story_quest_feature::result::applied)
		return;
	if (!save_persisted_state(&error))
	{
		undo();
		static int64_t last_log = 0;
		const auto now = static_cast<int64_t>(time(NULL));
		if (now - last_log >= 60)
		{
			logit(LOG_DEBUG, "Zone discovery could not be saved: %s", error.c_str());
			last_log = now;
		}
		return;
	}
	send_to_char("Discovery achievement: ", player);
	send_to_char(zone_table[index].name, player);
	send_to_char(". Use 'quest zone <area>' to view its journal.\r\n", player);
}

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
	const auto undo = tracker.checkpoint_for(current_season_id(),
						 { static_cast<uint32_t>(GET_PID(player)) });
	if (!tracker.remember_character(current_season_id(), static_cast<uint32_t>(GET_PID(player)),
					GET_NAME(player), !IS_TRUSTED(player),
					static_cast<int>(GET_RACEWAR(player))))
		return true;
	if (save_persisted_state(error))
		return true;
	undo();
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
	return render_daily_surface(player, colors, false, error);
}

std::string render_daily_score(P_char player, bool colors, std::string *error)
{
	return render_daily_surface(player, colors, true, error);
}

bool record_authoritative_completion(std::string_view definition_id, int32_t zone_number,
				     uint32_t direct_completer_pid,
				     const std::vector<uint32_t> &credited_pids, int32_t room_vnum,
				     int64_t completed_at, std::string_view character_name,
				     int level, int racewar, bool party_context_known,
				     uint32_t party_size, int strongest_party_level,
				     std::string *error, std::string_view transaction_id,
				     const frozen_daily_context *daily)
{
	if (!ready())
		return fail(error, "zone-story quest runtime is not ready");
	if (definition_id.empty() || !direct_completer_pid || credited_pids.empty() ||
	    room_vnum <= 0 || completed_at <= 0)
		return fail(error, "invalid authoritative zone-story completion context");
	if (!transaction_id.empty())
	{
		zone_story_quest_tracking::completion_transaction existing;
		if (tracker.existing_transaction(transaction_id, &existing))
		{
			auto expected = credited_pids;
			auto stored = existing.credited_pids;
			std::sort(expected.begin(), expected.end());
			std::sort(stored.begin(), stored.end());
			if (existing.quest_definition_id != definition_id ||
			    existing.direct_completer_pid != direct_completer_pid ||
			    existing.room_vnum != room_vnum ||
			    existing.completed_at != completed_at || expected != stored ||
			    (daily && (existing.season_id != daily->season_id ||
				       existing.content_revision != daily->catalog_revision ||
				       existing.daily_policy_revision != daily->policy_revision ||
				       [&]()
				       {
					       auto stored_daily = existing.daily_credited_pids;
					       auto frozen_daily = daily->eligible_pids;
					       std::sort(stored_daily.begin(), stored_daily.end());
					       std::sort(frozen_daily.begin(), frozen_daily.end());
					       return stored_daily != frozen_daily;
				       }())))
				return fail(
					error,
					"replayed quest completion conflicts with its durable receipt");
			return true;
		}
	}
	const auto *definition = tracker.catalog().definitions.empty() ? nullptr : [&]()
	{
		for (const auto &candidate : tracker.catalog().definitions)
			if (candidate.definition_id == definition_id)
				return &candidate;
		return static_cast<const zone_story_quest_tracking::quest_definition *>(nullptr);
	}();
	if (!definition)
		return fail(error,
			    "zone-story completion definition is not in the production catalog");
	zone_story_quest_feature::completion_event event;
	event.transaction.schema_version =
		zone_story_quest_tracking::ZONE_STORY_QUEST_TRACKING_SCHEMA_VERSION;
	event.transaction.transaction_id = transaction_id.empty() ?
						   new_transaction_id(current_season_id(),
								      direct_completer_pid,
								      definition_id, completed_at) :
						   std::string(transaction_id);
	event.transaction.quest_definition_id = definition_id;
	event.transaction.zone_number = definition->zone_number;
	if (daily && (daily->catalog_revision != definition->content_revision ||
		      zone_number != definition->zone_number))
		return fail(error, "frozen completion catalog revision is unsupported");
	event.transaction.direct_completer_pid = direct_completer_pid;
	event.transaction.credited_pids = credited_pids;
	event.transaction.room_vnum = room_vnum;
	event.transaction.completed_at = completed_at;
	event.transaction.season_id = daily ? daily->season_id : current_season_id();
	event.transaction.daily_policy_revision = daily ? daily->policy_revision : 0;
	if (daily)
		event.transaction.daily_credited_pids = daily->eligible_pids;
	event.transaction.content_revision = definition->content_revision;
	event.character_name = character_name;
	event.level = level;
	event.racewar = racewar;
	event.party_context_known = party_context_known;
	event.party_size = party_size;
	event.strongest_party_level = strongest_party_level;
	const auto undo = tracker.checkpoint_for(event.transaction.season_id, credited_pids,
						 event.transaction.transaction_id);
	const auto recorded = tracker.record_completion(event, error);
	if (recorded != zone_story_quest_feature::result::applied &&
	    recorded != zone_story_quest_feature::result::already_applied)
		return false;
	if (!save_persisted_state(error))
	{
		undo();
		return false;
	}
	return true;
}

bool record_legacy_completion(struct char_data *player, const quest_complete_data *completion,
			      int32_t room_vnum, int64_t completed_at, std::string *error,
			      std::string_view transaction_id)
{
	if (!player || IS_NPC(player) || !completion)
		return fail(error, "legacy zone-story completion requires a player and Q block");
	if (IS_TRUSTED(player))
		return fail(error,
			    "immortal characters are not eligible for quest completion tracking");
	const std::string *definition_id =
		zone_story_quest_production::definition_id_for(completion);
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
			if (!member->ch || !IS_PC(member->ch) || IS_TRUSTED(member->ch) ||
			    member->ch->in_room != player->in_room)
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
			strongest_party_level =
				std::max(strongest_party_level, GET_LEVEL(member->ch));
		}
	}
	frozen_daily_context frozen;
	frozen.season_id = current_season_id();
	frozen.catalog_revision = content_revision();
	frozen.policy_revision = 1;
	if (transaction_id.empty())
	{
		for (uint32_t recipient : credited_pids)
		{
			P_char candidate = recipient == pid ? player : nullptr;
			for (group_list *member = player->group; !candidate && member;
			     member = member->next)
				if (member->ch &&
				    static_cast<uint32_t>(GET_PID(member->ch)) == recipient)
					candidate = member->ch;
			if (daily_eligible(candidate, *definition_id, strongest_party_level,
					   completed_at))
				frozen.eligible_pids.push_back(recipient);
		}
	}
	const uint32_t party_size = static_cast<uint32_t>(credited_pids.size());
	return record_authoritative_completion(*definition_id, zone_number, pid, credited_pids,
					       room_vnum, completed_at, GET_NAME(player),
					       GET_LEVEL(player), GET_RACEWAR(player), true,
					       party_size, strongest_party_level, error,
					       transaction_id,
					       transaction_id.empty() ? &frozen : nullptr);
}
} // namespace zone_story_quest_runtime
