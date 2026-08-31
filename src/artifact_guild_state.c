#include "artifact_guild_state.h"

#include "assocs.h"
#include "db.h"
#include "epic.h"
#include "nexus_stones.h"
#include "prototypes.h"
#include "sql/sql.h"
#include "spells.h"
#include "utils.h"

#include <algorithm>
#include <cerrno>
#include <ctime>
#include <mysql.h>
#include <unordered_map>

namespace
{
struct cached_artifact
{
	int64_t timer;
	int32_t bind_owner_pid;
	int64_t bind_timer;
	uint64_t revision;
};

std::unordered_map<int32_t, cached_artifact> artifacts;
std::unordered_map<uint32_t, uint64_t> guild_revisions;
bool hydrated = false;

#ifndef __NO_MYSQL__
bool parse_i64(const char *text, int64_t *value)
{
	if (!text || !value)
		return false;
	char *end = nullptr;
	errno = 0;
	const long long parsed = strtoll(text, &end, 10);
	if (errno || !end || *end)
		return false;
	*value = parsed;
	return true;
}

bool parse_u64(const char *text, uint64_t *value)
{
	if (!text || !value)
		return false;
	char *end = nullptr;
	errno = 0;
	const unsigned long long parsed = strtoull(text, &end, 10);
	if (errno || !end || *end)
		return false;
	*value = parsed;
	return true;
}
#endif

int artifact_feed_seconds(P_char character, int epics, int epic_type)
{
	int seconds = epics * get_property("artifact.feeding.epic.point.seconds", 3600);
	switch (epic_type)
	{
	case EPIC_ZONE:
		seconds = static_cast<int>(seconds *
					   get_property("artifact.feeding.epic.typeMod.zone", 1.0));
		break;
	case EPIC_PVP:
		seconds = static_cast<int>(seconds *
					   get_property("artifact.feeding.epic.typeMod.pvp", 2.0));
		break;
	case EPIC_SHIP_PVP:
		seconds = static_cast<int>(
			seconds * get_property("artifact.feeding.epic.typeMod.pvpShip", 2.0));
		break;
	case EPIC_ELITE_MOB:
		seconds = static_cast<int>(
			seconds * get_property("artifact.feeding.epic.typeMod.eliteMob", 1.0));
		break;
	case EPIC_QUEST:
		seconds = static_cast<int>(
			seconds * get_property("artifact.feeding.epic.typeMod.quest", 1.0));
		break;
	case EPIC_RANDOM_ZONE:
		seconds = static_cast<int>(
			seconds * get_property("artifact.feeding.epic.typeMod.randomZone", 1.0));
		break;
	case EPIC_NEXUS_STONE:
		seconds = static_cast<int>(
			seconds * get_property("artifact.feeding.epic.typeMod.nexusStone", 1.0));
		break;
	case EPIC_BOON:
		seconds = static_cast<int>(
			seconds * get_property("artifact.feeding.epic.typeMod.boon", 0.25));
		break;
	case EPIC_STRAHDME:
	case EPIC_RANDOMMOB:
		break;
	default:
		seconds = 0;
		break;
	}
	if (affected_by_spell(character, TAG_PLR_RECENT_FRAG))
		seconds = (seconds * 3) / 2;
	return seconds;
}
} // namespace

bool artifact_guild_state_hydrate(void)
{
#ifdef __NO_MYSQL__
	return false;
#else
	std::unordered_map<int32_t, cached_artifact> next_artifacts;
	std::unordered_map<uint32_t, uint64_t> next_guilds;
	if (!qry("SELECT vnum,timer_epoch,bind_owner_pid,bind_timer_epoch,revision FROM "
		 "artifact_domain_state ORDER BY vnum"))
		return false;
	MYSQL_RES *rows = mysql_store_result(DB);
	if (!rows)
		return false;
	MYSQL_ROW row = nullptr;
	while ((row = mysql_fetch_row(rows)))
	{
		int64_t vnum = 0, timer = 0, owner = 0, bind_timer = 0;
		uint64_t revision = 0;
		if (!parse_i64(row[0], &vnum) || !parse_i64(row[1], &timer) ||
		    !parse_i64(row[2], &owner) || !parse_i64(row[3], &bind_timer) ||
		    !parse_u64(row[4], &revision) || vnum <= 0 || vnum > INT32_MAX ||
		    owner < INT32_MIN || owner > INT32_MAX)
		{
			mysql_free_result(rows);
			return false;
		}
		next_artifacts.emplace(static_cast<int32_t>(vnum),
				       cached_artifact{ timer, static_cast<int32_t>(owner),
							bind_timer, revision });
	}
	mysql_free_result(rows);
	if (!qry("SELECT id,outcome_revision FROM guilds ORDER BY id"))
		return false;
	rows = mysql_store_result(DB);
	if (!rows)
		return false;
	while ((row = mysql_fetch_row(rows)))
	{
		uint64_t guild_id = 0, revision = 0;
		if (!parse_u64(row[0], &guild_id) || !parse_u64(row[1], &revision) || !guild_id ||
		    guild_id > UINT32_MAX)
		{
			mysql_free_result(rows);
			return false;
		}
		next_guilds.emplace(static_cast<uint32_t>(guild_id), revision);
	}
	mysql_free_result(rows);
	artifacts.swap(next_artifacts);
	guild_revisions.swap(next_guilds);
	hydrated = true;
	return true;
#endif
}

artifact_guild_capture_status
artifact_guild_state_capture(P_char character, int epics, int epic_type,
			     const critical_operation_id &parent_operation_id,
			     artifact_guild_payload *payload)
{
	if (!character || IS_NPC(character) || !payload || epics <= 0 || !hydrated ||
	    critical_operation_id_is_zero(parent_operation_id))
		return artifact_guild_capture_status::unavailable;
	*payload = {};
	payload->parent_operation_id = parent_operation_id;
	payload->actor_pid = static_cast<uint32_t>(GET_PID(character));

	Guild *guild = GET_ASSOC(character);
	if (guild && epics >= static_cast<int>(get_property("prestige.epicsMinimum", 4.0)))
	{
		int members = 1;
		for (struct group_list *entry = character->group; entry; entry = entry->next)
			if (entry->ch != character && IS_PC(entry->ch) &&
			    entry->ch->in_room == character->in_room &&
			    GET_ASSOC(entry->ch) == guild)
				++members;
		if (members >=
		    static_cast<int>(get_property("prestige.guildedInGroupMinimum", 3.0)))
		{
			const uint32_t guild_id = guild->get_id();
			auto revision = guild_revisions.find(guild_id);
			if (revision == guild_revisions.end())
				return artifact_guild_capture_status::unavailable;
			int prestige =
				(epic_type == EPIC_PVP || epic_type == EPIC_SHIP_PVP) ?
					static_cast<int>(get_property("prestige.gain.pvp", 20)) :
					static_cast<int>(get_property("prestige.gain.default", 10));
			prestige = std::max(0, check_nexus_bonus(character, prestige,
								 NEXUS_BONUS_PRESTIGE));
			const uint64_t notch = static_cast<uint64_t>(std::max(
				1, get_property("prestige.constructionPoints.notch", 100)));
			payload->guild_id = guild_id;
			payload->expected_guild_revision = revision->second;
			payload->prestige_delta = prestige;
			payload->construction_delta =
				static_cast<int64_t>((guild->get_prestige() + prestige) / notch -
						     guild->get_prestige() / notch);
		}
	}

	if (!IS_TRUSTED(character))
	{
		const int feed_seconds = artifact_feed_seconds(character, epics, epic_type);
		const int64_t maximum = static_cast<int64_t>(time(nullptr)) +
					ARTIFACT_BLOOD_DAYS * SECS_PER_REAL_DAY;
		for (int slot = 0; slot < MAX_WEAR && feed_seconds &&
				   payload->artifact_count < payload->artifacts.size();
		     ++slot)
		{
			P_obj object = character->equipment[slot];
			if (!object || !IS_ARTIFACT(object))
				continue;
			const int32_t vnum = OBJ_VNUM(object);
			auto state = artifacts.find(vnum);
			if (state == artifacts.end())
				return artifact_guild_capture_status::unavailable;
			const bool soul_check = epic_type == EPIC_PVP || epic_type == EPIC_SHIP_PVP;
			if (soul_check && state->second.bind_owner_pid != -1 &&
			    state->second.bind_owner_pid != GET_PID(character))
				continue;
			int64_t target = state->second.timer + feed_seconds;
			if (target > maximum)
				target = maximum;
			if (target < 0)
				target = 0;
			auto &delta = payload->artifacts[payload->artifact_count++];
			delta = { vnum,
				  ARTIFACT_DELTA_FEED,
				  state->second.revision,
				  state->second.timer,
				  target,
				  state->second.bind_owner_pid,
				  state->second.bind_owner_pid,
				  state->second.bind_timer,
				  state->second.bind_timer };
		}
	}
	if (!payload->guild_id && !payload->artifact_count)
		return artifact_guild_capture_status::no_effect;
	return artifact_guild_capture_status::ready;
}

void artifact_guild_state_publish(const artifact_guild_result &result)
{
	for (size_t index = 0; index < result.artifact_count; ++index)
	{
		const auto &entry = result.artifacts[index];
		artifacts[entry.vnum] = { entry.timer, entry.bind_owner_pid, entry.bind_timer,
					  entry.revision };
	}
	if (result.guild_id)
	{
		guild_revisions[result.guild_id] = result.guild_revision;
		Guild *guild = get_guild_from_id(static_cast<int>(result.guild_id));
		if (guild)
			guild->publish_outcome_totals(result.prestige, result.construction);
	}
}

bool artifact_guild_state_ready(void)
{
	return hydrated;
}

void artifact_guild_state_reset_for_tests(void)
{
	artifacts.clear();
	guild_revisions.clear();
	hydrated = false;
}
