#include "guild/artifact_guild_state.h"

#include "guild/artifact_feed_rates.h"
#include "guild/assocs.h"
#include "world/db.h"
#include "world/epic.h"
#include "economy/nexus_stones.h"
#include "core/prototypes.h"
#include "world/difficulty.h"
#include "sql/sql.h"
#include "magic/spells.h"
#include "core/utils.h"

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

/*
 * seconds fed = epics x point seconds x the source's rate (artifact_feed_rates.c)
 *               x the artifact-feeding difficulty dial
 *               x 1.5 within epic.frag.thrill.duration of a frag
 */
int artifact_feed_seconds(P_char character, int epics, int epic_type)
{
	const double rate = artifact_feed_type_mod(epic_type);
	if (epics <= 0 || rate <= 0.0)
		return 0;
	int seconds =
		static_cast<int>(static_cast<double>(epics) * artifact_feed_point_seconds() * rate);
	seconds = difficulty_scale_int(seconds, difficulty_multiplier(DIFFICULTY_ARTIFACT_FEEDING));
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
		// PvP fills the timer to the full ARTIFACT_BLOOD_DAYS; every other source only
		// up to the non-PvP ceiling (artifact.feeding.nonPvp.ceilingHours).
		const int64_t maximum =
			static_cast<int64_t>(time(nullptr)) +
			(artifact_feed_is_pvp(epic_type) ?
				 static_cast<int64_t>(ARTIFACT_BLOOD_DAYS) * SECS_PER_REAL_DAY :
				 artifact_feed_nonpvp_ceiling_seconds());
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
			// A ceiling caps the feed; it never lowers a timer already above it.
			if (target > maximum)
				target = std::max(state->second.timer, maximum);
			if (target < 0)
				target = 0;
			if (target <= state->second.timer)
				continue;
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
