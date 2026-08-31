#include "redis/redis_report_cache.h"

#include "config.h"
#include "db.h"
#include "structs.h"
#include "epic.h"
#include "combat/frag_cap_config.h"
#include "prototypes.h"
#include "redis/redis_cache_store.h"
#include "redis/redis_command_observability.h"
#include "redis/redis_connection.h"
#include "redis/redis_key_registry.h"
#include "redis/redis_namespace.h"
#include "persistence/report_cache_codec.h"
#include "magic/spells.h"
#include "sql/sql.h"
#include "utility.h"
#include "utils.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#ifndef __NO_REDIS__
#include <hiredis/hiredis.h>
#endif

struct zone_random_data
{
	int zone;
	int races[10];
	int proc_spells[3][2];
};

extern struct zone_random_data zones_random_data[];
extern Skill skills[];
extern void get_level_cap_info(long *max_frags, int *racewar, int *level, time_t *next_update);
extern const racewar_struct racewar_color[];

namespace
{
constexpr int named_report_cache_ttl_seconds = 86400;
constexpr int fraglist_cache_ttl_seconds = 900;
constexpr int epic_zones_cache_ttl_seconds = 900;
constexpr int artifact_cache_ttl_seconds = 900;
constexpr int max_frag_size = 10;

bool report_cache_enabled = false;
char cache_prefix[160] = {};
char cache_pattern[160] = {};
char named_key[160] = {};
char fraglist_key[160] = {};
char epic_zones_key[160] = {};
char artifact_keys[6][160] = {};

bool season_key(const char *key_namespace, uint64_t epoch, const char *suffix, char *output,
		size_t output_size)
{
	return redis_namespace_season_key(key_namespace, epoch, suffix, output, output_size);
}

const char *resolve_key(const char *key)
{
	if (!key)
		return NULL;
	if (!strcmp(key, REDIS_CACHE_NAMED))
		return named_key;
	if (!strcmp(key, REDIS_CACHE_FRAGLIST))
		return fraglist_key;
	if (!strcmp(key, REDIS_CACHE_EPIC_ZONES))
		return epic_zones_key;
	const size_t prefix_size = strlen(cache_prefix);
	return prefix_size && !strncmp(key, cache_prefix, prefix_size) && key[prefix_size] ? key :
											     NULL;
}

bool cache_set_ex(const char *key, int seconds, const char *value)
{
#ifdef __NO_REDIS__
	(void)key;
	(void)seconds;
	(void)value;
	return false;
#else
	const char *resolved = resolve_key(key);
	if (!report_cache_enabled || !resolved || !value || seconds <= 0)
		return false;
	return redis_cache_store_set(resolved, value, seconds);
#endif
}

char *cache_get(const char *key)
{
#ifdef __NO_REDIS__
	(void)key;
	return NULL;
#else
	const char *resolved = resolve_key(key);
	if (!report_cache_enabled || !resolved)
		return NULL;
	return redis_cache_store_get(resolved);
#endif
}

bool cache_delete(const char *key)
{
#ifdef __NO_REDIS__
	(void)key;
	return false;
#else
	const char *resolved = resolve_key(key);
	if (!report_cache_enabled || !resolved)
		return false;
	return redis_cache_store_delete(resolved);
#endif
}

const char *artifact_key(int type, bool godlist)
{
	if (type < 1 || type > 3)
		return NULL;
	return artifact_keys[(type - 1) * 2 + (godlist ? 1 : 0)];
}

#ifndef __NO_REDIS__
redisReply *cache_prime_command(redisContext *context, const char *script, const char **keys)
{
	const uint64_t started_usec = redis_observability_now_usec();
	redisReply *reply = NULL;
	if (context && !context->err)
		reply = static_cast<redisReply *>(
			redisCommand(context, "EVAL %b 6 %s %s %s %s %s %s", script, strlen(script),
				     keys[0], keys[1], keys[2], keys[3], keys[4], keys[5]));
	const uint64_t finished_usec = redis_observability_now_usec();
	const uint64_t duration_usec =
		finished_usec >= started_usec ? finished_usec - started_usec : 0;
	redis_shared_command_outcome outcome = REDIS_SHARED_OUTCOME_SUCCESS;
	if (!context || context->err)
		outcome = redis_command_outcome(context, false);
	else if (!reply)
		outcome = REDIS_SHARED_OUTCOME_NO_REPLY;
	else if (reply->type == REDIS_REPLY_ERROR)
		outcome = REDIS_SHARED_OUTCOME_ERROR_REPLY;
	redis_shared_command_observability_record(
		REDIS_SHARED_SCOPE_CACHE, REDIS_SHARED_COMMAND_SCRIPT, outcome, duration_usec);
	if (outcome == REDIS_SHARED_OUTCOME_SUCCESS)
		return reply;
	if (reply)
		freeReplyObject(reply);
	return NULL;
}

void prime_artifact_caches(const redis_connection_settings *connection)
{
	redisContext *context = redis_connection_open(connection);
	if (!context || context->err)
	{
		redis_shared_command_observability_record(REDIS_SHARED_SCOPE_CACHE,
							  REDIS_SHARED_COMMAND_SCRIPT,
							  redis_command_outcome(context, false), 0);
		if (context)
			redisFree(context);
		return;
	}
	constexpr const char *script = "local result={} for index,key in ipairs(KEYS) do "
				       "result[index*2-1]=redis.call('GET',key) "
				       "result[index*2]=redis.call('PTTL',key) end return result";
	const char *keys[6] = {};
	for (size_t index = 0; index < 6; ++index)
		keys[index] = artifact_keys[index];
	redisReply *reply = cache_prime_command(context, script, keys);
	if (!reply || reply->type != REDIS_REPLY_ARRAY || reply->elements != 12)
	{
		if (reply)
			freeReplyObject(reply);
		redisFree(context);
		return;
	}
	for (size_t index = 0; index < 6; ++index)
	{
		redisReply *value = reply->element[index * 2];
		redisReply *ttl = reply->element[index * 2 + 1];
		if (!value || value->type != REDIS_REPLY_STRING || !value->str || !ttl ||
		    ttl->type != REDIS_REPLY_INTEGER || ttl->integer <= 0)
			continue;
		const long long seconds = std::min((ttl->integer + 999) / 1000, 900LL);
		redis_cache_store_seed(keys[index], value->str, static_cast<int>(seconds));
	}
	freeReplyObject(reply);
	redisFree(context);
}
#endif

char *generate_named_report(void)
{
	char *output = static_cast<char *>(malloc(MAX_STRING_LENGTH * 4));
	if (!output)
		return NULL;

	output[0] = '\0';
	char buffer[MAX_STRING_LENGTH];

	strcat(output, "&+YCurrent listing of spells granted by named sets by zone.&n\n");
	strcat(output, "&+Y-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=&n\n\n");
	strcat(output, "  &+MNotes&n: &+W*&n if a zone isn't listed, sets still grant hitpoints\n");
	strcat(output,
	       "         &+W*&n caster level of the spell(s) is based on number of items\n");
	strcat(output, "           going over set requirements will increase caster level\n");
	strcat(output, "         &+W*&n &+Gthese&n spells have a cooldown of 1 minute\n");
	strcat(output, "           &+ythese&n spells have a cooldown of 5 minutes\n\n");
	strcat(output,
	       "&+Y ZONE NAME                                        &+W|&+B SPELLS GRANTED &+W(&+Ypieces required&n&+W)&n\n");
	strcat(output,
	       "&+W-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=&n\n");

	for (int i = 0; zones_random_data[i].zone; i++)
	{
		int zone_id = real_zone(zones_random_data[i].zone);
		if (zone_id <= 0)
			continue;

		const char *zone_name = zone_table[zone_id].name;
		snprintf(buffer, sizeof(buffer), " %s    &+W|&n ",
			 pad_ansi(zone_name, 45, FALSE).c_str());

		if (zones_random_data[i].proc_spells[0][0] == 0)
		{
			strcat(buffer, "&+LNONE&n");
			strcat(buffer, "\n");
			strcat(output, buffer);
			continue;
		}

		for (int x = 0; x < 3; x++)
		{
			if (zones_random_data[i].proc_spells[x][0] != 0 &&
			    zones_random_data[i].proc_spells[x][1] <= MAX_AFFECT_TYPES)
			{
				char buf[256];
				const char *spell_color = "&+B";

				if (zones_random_data[i].proc_spells[x][1] == SPELL_STONE_SKIN ||
				    zones_random_data[i].proc_spells[x][1] == SPELL_INVIGORATE)
					spell_color = "&+G";
				else if (zones_random_data[i].proc_spells[x][1] ==
					 SPELL_CONJURE_ELEMENTAL)
					spell_color = "&+y";

				snprintf(buf, sizeof(buf), "%s%s%s &+W(&+Y%d&+W)&n",
					 x != 0 ? "&+W,&n " : "", spell_color,
					 skills[zones_random_data[i].proc_spells[x][1]].name,
					 zones_random_data[i].proc_spells[x][0]);
				strcat(buffer, buf);
			}
		}
		strcat(buffer, "\n");
		strcat(output, buffer);
	}

	return output;
}

char *generate_fraglist_cache_payload(void)
{
#ifdef __NO_REDIS__
	return NULL;
#else
	char *output = static_cast<char *>(malloc(65536));
	if (!output)
		return NULL;

	output[0] = '\0';
	char buf[2048], name[256], prefix[1024];
	int frags, count;
	float fragnum;
	int cap_level, cap_racewar, cap_others;
	long cap_frags;
	time_t cap_deadline;
	MYSQL_RES *res;
	MYSQL_ROW row;

	get_level_cap_info(&cap_frags, &cap_racewar, &cap_level, &cap_deadline);
	if (cap_frags < 0 || cap_racewar < 0 || cap_racewar >= MAX_RACEWAR + 2 || cap_deadline <= 0)
	{
		free(output);
		return NULL;
	}
	const struct frag_cap_config *cap_config = frag_cap_config_get();
	if (!cap_config)
	{
		free(output);
		return NULL;
	}
	cap_others = std::clamp(cap_level, cap_config->cap_floor_level,
				std::min(cap_config->cap_maximum_level, MAXLVLMORTAL));

	snprintf(
		prefix, sizeof prefix,
		"&+YFrag Level Cap:&+w %d - &+%c%s&n, &+w%d&N - Others, &+YTop Frag Amount: &+w%d.%02d\n"
		"&+YTimer:&+w ",
		cap_level, racewar_color[cap_racewar].color, racewar_color[cap_racewar].name,
		cap_others, static_cast<int>(cap_frags / 100), static_cast<int>(cap_frags % 100));
	snprintf(output, 65536, " &+YFrags needed:&+w %.2f&n\n\n&+WTop Fraggers\n\n",
		 frag_cap_config_frags_for_level(cap_level + 1));

	res = db_query("SELECT char_name, total_frags FROM frag_leaderboard "
		       "WHERE deleted_at IS NULL ORDER BY total_frags DESC LIMIT %d",
		       max_frag_size);
	if (res)
	{
		count = 0;
		while ((row = mysql_fetch_row(res)) && count < max_frag_size)
		{
			if (row[0] && row[1])
			{
				strlcpy(name, row[0], sizeof name);
				name[0] = toupper(name[0]);
				frags = atoi(row[1]);
				fragnum = frags / 100.0;
				snprintf(buf, sizeof(buf), "   &+Y%-30s             &+R% 6.2f\r\n",
					 name, fragnum);
				strcat(output, buf);
				count++;
			}
		}
		mysql_free_result(res);

		while (count < max_frag_size)
		{
			snprintf(buf, sizeof(buf), "   &+Y%-30s             &+R% 6.2f\r\n",
				 "Nobody", 0.0);
			strcat(output, buf);
			count++;
		}
	}

	strcat(output, "\r\n\r\n&+LLowest Fraggers\r\n\r\n");

	res = db_query("SELECT char_name, total_frags FROM frag_leaderboard "
		       "WHERE deleted_at IS NULL ORDER BY total_frags ASC LIMIT %d",
		       max_frag_size);
	if (res)
	{
		count = 0;
		while ((row = mysql_fetch_row(res)) && count < max_frag_size)
		{
			if (row[0] && row[1])
			{
				strlcpy(name, row[0], sizeof name);
				name[0] = toupper(name[0]);
				frags = atoi(row[1]);
				fragnum = frags / 100.0;
				snprintf(buf, sizeof(buf), "   &+Y%-30s             &+R% 6.2f\r\n",
					 name, fragnum);
				strcat(output, buf);
				count++;
			}
		}
		mysql_free_result(res);

		while (count < max_frag_size)
		{
			snprintf(buf, sizeof(buf), "   &+Y%-30s             &+R% 6.2f\r\n",
				 "Nobody", 0.0);
			strcat(output, buf);
			count++;
		}
	}

	strcat(output, "\r\n");
	char *payload = report_cache_countdown_encode(prefix, output,
						      static_cast<uint64_t>(time(NULL)),
						      static_cast<uint64_t>(cap_deadline));
	free(output);
	return payload;
#endif
}
} // namespace

bool redis_report_cache_configure(const char *key_namespace, uint64_t epoch)
{
	report_cache_enabled = false;
	if (!key_namespace || !*key_namespace || !epoch ||
	    !season_key(key_namespace, epoch, "cache:", cache_prefix, sizeof cache_prefix) ||
	    !season_key(key_namespace, epoch, REDIS_CACHE_PATTERN, cache_pattern,
			sizeof cache_pattern) ||
	    !season_key(key_namespace, epoch, REDIS_CACHE_NAMED, named_key, sizeof named_key) ||
	    !season_key(key_namespace, epoch, REDIS_CACHE_FRAGLIST, fraglist_key,
			sizeof fraglist_key) ||
	    !season_key(key_namespace, epoch, REDIS_CACHE_EPIC_ZONES, epic_zones_key,
			sizeof epic_zones_key))
		return false;
	for (int type = 1; type <= 3; ++type)
		for (int view = 0; view <= 1; ++view)
		{
			char suffix[64];
			const int written = snprintf(suffix, sizeof suffix,
						     REDIS_CACHE_ARTIFACT_FORMAT, type, view);
			const size_t index = static_cast<size_t>((type - 1) * 2 + view);
			if (written <= 0 || static_cast<size_t>(written) >= sizeof suffix ||
			    !season_key(key_namespace, epoch, suffix, artifact_keys[index],
					sizeof artifact_keys[index]))
				return false;
		}
	return true;
}

bool redis_report_cache_start(const redis_connection_settings *connection)
{
#ifdef __NO_REDIS__
	(void)connection;
	return false;
#else
	report_cache_enabled = false;
	if (!connection || !cache_pattern[0])
		return false;
	const redis_cache_store_config config = { connection };
	if (!redis_cache_store_init(&config))
		return false;
	report_cache_enabled = true;
	prime_artifact_caches(connection);
	return true;
#endif
}

void redis_report_cache_cancel(void)
{
	report_cache_enabled = false;
#ifndef __NO_REDIS__
	redis_cache_store_cancel();
#endif
}

bool redis_report_cache_shutdown(uint64_t timeout_msec)
{
	report_cache_enabled = false;
#ifdef __NO_REDIS__
	(void)timeout_msec;
	return true;
#else
	return redis_cache_store_shutdown(timeout_msec);
#endif
}

void redis_report_cache_reset(void)
{
	report_cache_enabled = false;
	cache_prefix[0] = '\0';
	cache_pattern[0] = '\0';
	named_key[0] = '\0';
	fraglist_key[0] = '\0';
	epic_zones_key[0] = '\0';
	for (auto &key : artifact_keys)
		key[0] = '\0';
}

bool redis_report_cache_enabled(void)
{
	return report_cache_enabled;
}

const char *redis_report_cache_pattern(void)
{
	return cache_pattern[0] ? cache_pattern : NULL;
}

void redis_cache_named_report(void)
{
#ifndef __NO_REDIS__
	if (!report_cache_enabled)
		return;
	char *report = generate_named_report();
	if (report)
	{
		cache_set_ex(REDIS_CACHE_NAMED, named_report_cache_ttl_seconds, report);
		free(report);
		logit(LOG_SYS, "redis: cached named report");
	}
#endif
}

char *redis_get_named_report(void)
{
	return cache_get(REDIS_CACHE_NAMED);
}

bool redis_invalidate_named_report(void)
{
	return cache_delete(REDIS_CACHE_NAMED);
}

void redis_cache_fraglist(void)
{
#ifndef __NO_REDIS__
	if (!report_cache_enabled)
		return;
	char *output = generate_fraglist_cache_payload();
	if (output)
	{
		cache_set_ex(REDIS_CACHE_FRAGLIST, fraglist_cache_ttl_seconds, output);
		free(output);
		logit(LOG_SYS, "redis: cached fraglist");
	}
#endif
}

char *redis_get_fraglist(void)
{
#ifdef __NO_REDIS__
	return NULL;
#else
	if (!report_cache_enabled)
		return NULL;
	const auto render = [](const char *payload, void *) -> char *
	{
		return report_cache_countdown_render(payload, static_cast<uint64_t>(time(NULL)),
						     fraglist_cache_ttl_seconds);
	};
	return redis_cache_store_transform(fraglist_key, render, NULL);
#endif
}

bool redis_invalidate_fraglist(void)
{
	return cache_delete(REDIS_CACHE_FRAGLIST);
}

void redis_cache_epic_zones(void)
{
#ifndef __NO_REDIS__
	if (!report_cache_enabled)
		return;
	char *output = generate_epic_zones_output();
	if (output)
	{
		redis_cache_epic_zones_output(output);
		free(output);
		logit(LOG_SYS, "redis: cached epic zones");
	}
#endif
}

void redis_cache_epic_zones_output(const char *output)
{
	cache_set_ex(REDIS_CACHE_EPIC_ZONES, epic_zones_cache_ttl_seconds, output);
}

char *redis_get_epic_zones(void)
{
	return cache_get(REDIS_CACHE_EPIC_ZONES);
}

bool redis_invalidate_epic_zones(void)
{
	return cache_delete(REDIS_CACHE_EPIC_ZONES);
}

void redis_cache_artifact_list(int type, bool godlist, const char *json)
{
#ifndef __NO_REDIS__
	if (!report_cache_enabled || type < 1 || type > 3 || !json)
		return;
	cache_set_ex(artifact_key(type, godlist), artifact_cache_ttl_seconds, json);
#else
	(void)type;
	(void)godlist;
	(void)json;
#endif
}

char *redis_get_artifact_list(int type, bool godlist)
{
	return cache_get(artifact_key(type, godlist));
}

bool redis_invalidate_artifact_list(int type, bool godlist)
{
#ifdef __NO_REDIS__
	(void)type;
	(void)godlist;
	return false;
#else
	if (!report_cache_enabled || type < 1 || type > 3)
		return false;
	return cache_delete(artifact_key(type, godlist));
#endif
}

bool redis_invalidate_artifact_cache(void)
{
#ifdef __NO_REDIS__
	return false;
#else
	if (!report_cache_enabled)
		return false;
	bool submitted = true;
	for (int type = 1; type <= 3; ++type)
	{
		submitted = cache_delete(artifact_key(type, false)) && submitted;
		submitted = cache_delete(artifact_key(type, true)) && submitted;
	}
	return submitted;
#endif
}
