// wiz command for redis status and cache management

#include "prototypes.h"
#include "structs.h"
#include "interp.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include "redis.h"
#include "redis_cache_store.h"
#include "redis_presence_worker.h"

// helper to format time ago
static void format_time_ago(time_t ts, char *buf, size_t len)
{
	if (ts == 0)
	{
		snprintf(buf, len, "never");
		return;
	}

	time_t now = time(NULL);
	long diff = now - ts;

	if (diff < 0)
		diff = 0;

	if (diff < 60)
		snprintf(buf, len, "%ld sec ago", diff);
	else if (diff < 3600)
		snprintf(buf, len, "%ld min ago", diff / 60);
	else if (diff < 86400)
		snprintf(buf, len, "%ld hr ago", diff / 3600);
	else
		snprintf(buf, len, "%ld day ago", diff / 86400);
}

// helper to format ttl remaining
static void format_ttl(long ttl, char *buf, size_t len)
{
	if (ttl <= 0)
	{
		snprintf(buf, len, "expired");
		return;
	}

	long mins = ttl / 60;
	long secs = ttl % 60;
	snprintf(buf, len, "%ld:%02ld", mins, secs);
}

static void redis_status_simple(P_char ch)
{
	char buf[MAX_STRING_LENGTH];
	char time_buf[64];
	int pos = 0;

	pos += snprintf(buf + pos, sizeof(buf) - pos, "&+gRedis Status&n\r\n");

	// world state
	time_t ws_ts = redis_world_state_timestamp();
	bool is_valid = redis_has_world_state();

	format_time_ago(ws_ts, time_buf, sizeof(time_buf));
	pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cworld_state&n      %s%-5s&n    %s\r\n",
			is_valid ? "&+G" : "&+R", is_valid ? "VALID" : "NONE",
			ws_ts > 0 ? time_buf : "");

	// revisioned player save queue
	int dirty = get_dirty_player_count();
	pos += snprintf(buf + pos, sizeof(buf) - pos,
			"  &+cplayer_queue&n     &+Y%-5d&n    pending saves\r\n", dirty);

	const redis_presence_worker_health presence = redis_presence_worker_health_copy();
	pos += snprintf(buf + pos, sizeof(buf) - pos,
			"  &+cpresence_worker&n  %s%-9s&n queued=%zu dropped=%llu\r\n",
			presence.connected ? "&+G" : "&+Y",
			!presence.initialized ? "OFF" :
			presence.connected    ? "HEALTHY" :
						"BACKOFF",
			presence.queued, (unsigned long long)presence.dropped);
	const redis_cache_store_health cache = redis_cache_store_health_copy();
	pos += snprintf(
		buf + pos, sizeof(buf) - pos,
		"  &+ccache_worker&n     %s%-9s&n queued=%zu/%zuB local=%zu dropped=%llu\r\n",
		cache.connected ? "&+G" : "&+Y",
		!cache.initialized ? "OFF" :
		cache.connected	   ? "HEALTHY" :
				     "BACKOFF",
		cache.queued, cache.queued_bytes, cache.local_entries,
		(unsigned long long)cache.dropped);

	// floor drops
	char floor_key[128];
	long floor_count = redis_season_key(floor_key, sizeof floor_key, "floor_drops") ?
				   redis_hlen(floor_key) :
				   -1;
	pos += snprintf(buf + pos, sizeof(buf) - pos,
			"  &+cfloor_drops&n      &+Y%-5ld&n    objects\r\n", floor_count);

	pos += snprintf(buf + pos, sizeof(buf) - pos, "\r\n&+gCaches&n\r\n");

	// artifacts - just check if any exist
	bool arti_cached = redis_key_exists("mud:cache:artifact:1:0") ||
			   redis_key_exists("mud:cache:artifact:2:0") ||
			   redis_key_exists("mud:cache:artifact:3:0");
	pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cartifacts&n        %s%s&n\r\n",
			arti_cached ? "&+G" : "&+R", arti_cached ? "CACHED" : "CLEAR");

	// fraglist
	bool frag_cached = redis_key_exists("mud:cache:fraglist");
	pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cfraglist&n         %s%s&n\r\n",
			frag_cached ? "&+G" : "&+R", frag_cached ? "CACHED" : "CLEAR");

	// epic zones
	bool epic_cached = redis_key_exists("mud:cache:epic_zones");
	long epic_ttl = redis_get_ttl("mud:cache:epic_zones");
	if (epic_cached && epic_ttl > 0)
	{
		format_ttl(epic_ttl, time_buf, sizeof(time_buf));
		pos += snprintf(buf + pos, sizeof(buf) - pos,
				"  &+cepic_zones&n       &+GCACHED&n   expires %s\r\n", time_buf);
	}
	else
	{
		pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cepic_zones&n       %s%s&n\r\n",
				epic_cached ? "&+G" : "&+R", epic_cached ? "CACHED" : "CLEAR");
	}

	// named
	bool named_cached = redis_key_exists("mud:cache:named");
	pos += snprintf(buf + pos, sizeof(buf) - pos, "  &+cnamed&n            %s%s&n\r\n",
			named_cached ? "&+G" : "&+R", named_cached ? "CACHED" : "CLEAR");

	send_to_char(buf, ch);
}

static void redis_status_detailed(P_char ch)
{
	char buf[MAX_STRING_LENGTH * 2];
	char time_buf[64];

	checked_snprintf(buf, sizeof(buf), "&+gRedis Status (detailed)&n\r\n\r\n");

	const redis_presence_worker_health presence = redis_presence_worker_health_copy();
	APPENDF(buf,
		"&+g[Presence Worker]&n\r\n"
		"  state=%s queued=%zu high_water=%zu busy=%s\r\n"
		"  submitted=%llu completed=%llu dropped=%llu failures=%llu reconnects=%llu\r\n\r\n",
		!presence.initialized ? "off" :
		presence.connected    ? "healthy" :
					"backoff",
		presence.queued, presence.high_water, presence.busy ? "yes" : "no",
		(unsigned long long)presence.submitted, (unsigned long long)presence.completed,
		(unsigned long long)presence.dropped, (unsigned long long)presence.command_failures,
		(unsigned long long)presence.reconnects);
	const redis_cache_store_health cache = redis_cache_store_health_copy();
	APPENDF(buf,
		"&+g[Cache Worker]&n\r\n"
		"  state=%s queued=%zu bytes=%zu local=%zu busy=%s\r\n"
		"  submitted=%llu completed=%llu coalesced=%llu dropped=%llu failures=%llu reconnects=%llu\r\n\r\n",
		!cache.initialized ? "off" :
		cache.connected	   ? "healthy" :
				     "backoff",
		cache.queued, cache.queued_bytes, cache.local_entries, cache.busy ? "yes" : "no",
		(unsigned long long)cache.submitted, (unsigned long long)cache.completed,
		(unsigned long long)cache.coalesced, (unsigned long long)cache.dropped,
		(unsigned long long)cache.command_failures, (unsigned long long)cache.reconnects);

	APPENDF(buf, "&+g[World Recovery]&n\r\n");

	// world state with full timestamp
	time_t ws_ts = redis_world_state_timestamp();
	bool is_valid = redis_has_world_state();

	if (ws_ts > 0)
	{
		struct tm *tm_info = localtime(&ws_ts);
		char date_buf[64];
		strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", tm_info);
		format_time_ago(ws_ts, time_buf, sizeof(time_buf));
		APPENDF(buf, "  &+cworld_state&n      %s%-5s&n    %s (%s)\r\n",
			is_valid ? "&+G" : "&+R", is_valid ? "VALID" : "NONE", date_buf, time_buf);
	}
	else
	{
		APPENDF(buf, "  &+cworld_state&n      &+RNONE&n\r\n");
	}

	// floor drops
	char floor_key[128];
	long floor_drops = redis_season_key(floor_key, sizeof floor_key, "floor_drops") ?
				   redis_hlen(floor_key) :
				   -1;
	APPENDF(buf, "  &+cfloor_drops&n      &+Y%-5ld&n    objects tracked\r\n", floor_drops);

	// floor pickups
	long floor_pickups = redis_scard("mud:floor_pickups");
	APPENDF(buf, "  &+cfloor_pickups&n    &+Y%-5ld&n    uids in dedup set\r\n", floor_pickups);

	// revisioned player save queue
	int dirty = get_dirty_player_count();
	APPENDF(buf, "  &+cplayer_queue&n     &+Y%-5d&n    pending async saves\r\n", dirty);

	APPENDF(buf, "\r\n&+g[Content Caches]&n\r\n");

	// artifacts - show all 6 variants
	APPENDF(buf, "  &+cartifacts&n\r\n");

	const char *arti_names[] = { "major", "unique", "ioun" };
	for (int t = 1; t <= 3; t++)
	{
		for (int g = 0; g <= 1; g++)
		{
			char key[64];
			snprintf(key, sizeof(key), "mud:cache:artifact:%d:%d", t, g);
			bool cached = redis_key_exists(key);
			APPENDF(buf, "    %s (%s)    %s%s&n\r\n", arti_names[t - 1],
				g ? "god" : "player", cached ? "&+G" : "&+R",
				cached ? "CACHED" : "CLEAR");
		}
	}

	// fraglist
	bool frag_cached = redis_key_exists("mud:cache:fraglist");
	APPENDF(buf, "  &+cfraglist&n         %s%s&n\r\n", frag_cached ? "&+G" : "&+R",
		frag_cached ? "CACHED" : "CLEAR");

	// epic zones with ttl
	bool epic_cached = redis_key_exists("mud:cache:epic_zones");
	long epic_ttl = redis_get_ttl("mud:cache:epic_zones");
	if (epic_cached && epic_ttl > 0)
	{
		format_ttl(epic_ttl, time_buf, sizeof(time_buf));
		APPENDF(buf, "  &+cepic_zones&n       &+GCACHED&n   ttl %s\r\n", time_buf);
	}
	else
	{
		APPENDF(buf, "  &+cepic_zones&n       %s%s&n\r\n", epic_cached ? "&+G" : "&+R",
			epic_cached ? "CACHED" : "CLEAR");
	}

	// named
	bool named_cached = redis_key_exists("mud:cache:named");
	APPENDF(buf, "  &+cnamed&n            %s%s&n\r\n", named_cached ? "&+G" : "&+R",
		named_cached ? "CACHED" : "CLEAR");

	send_to_char(buf, ch);
}

static void redis_clear_cache(P_char ch, const char *cache)
{
	char buf[MAX_STRING_LENGTH];

	if (!*cache)
	{
		send_to_char("Usage: redis clear <cache>\r\n", ch);
		send_to_char("&+cValid:&n world, floor, artifacts, fraglist, epic, named, all\r\n",
			     ch);
		return;
	}

	if (is_abbrev(cache, "world"))
	{
		if (redis_clear_world_state())
			send_to_char(
				"&+GCleared and quiesced:&n world_state (publishing resumes after restart)\r\n",
				ch);
		else
			send_to_char("&+RFailed:&n world_state was not safely cleared\r\n", ch);
		return;
	}

	if (is_abbrev(cache, "floor"))
	{
		if (!redis_world_recovery_quiesce())
		{
			send_to_char("&+RFailed:&n world publisher could not be safely fenced\r\n",
				     ch);
			return;
		}
		redis_clear_floor_drops();
		redis_clear_floor_pickups();
		send_to_char(
			"&+GCleared and quiesced:&n floor_drops, floor_pickups (publishing resumes after restart)\r\n",
			ch);
		return;
	}

	if (is_abbrev(cache, "artifacts"))
	{
		redis_invalidate_artifact_cache();
		send_to_char("&+GCleared:&n artifacts (6 variants)\r\n", ch);
		return;
	}

	if (is_abbrev(cache, "fraglist"))
	{
		redis_invalidate_fraglist();
		send_to_char("&+GCleared:&n fraglist\r\n", ch);
		return;
	}

	if (is_abbrev(cache, "epic"))
	{
		redis_invalidate_epic_zones();
		send_to_char("&+GCleared:&n epic_zones\r\n", ch);
		return;
	}

	if (is_abbrev(cache, "named"))
	{
		redis_cache_del("mud:cache:named");
		send_to_char("&+GCleared:&n named\r\n", ch);
		return;
	}

	snprintf(buf, sizeof(buf), "&+RUnknown cache:&n %s\r\n", cache);
	send_to_char(buf, ch);
	send_to_char("&+cValid:&n world, floor, artifacts, fraglist, epic, named, all\r\n", ch);
}

static void redis_clear_all(P_char ch, bool confirmed)
{
	if (!confirmed)
	{
		send_to_char("&+RUse 'redis clear all confirm' to clear all caches.&n\r\n", ch);
		return;
	}

	// world recovery
	if (!redis_clear_world_state())
	{
		send_to_char("&+RFailed:&n world recovery was not safely cleared\r\n", ch);
		return;
	}
	redis_clear_floor_drops();
	redis_clear_floor_pickups();

	send_to_char(
		"&+GCleared and quiesced:&n world_state, floor_drops, floor_pickups (publishing resumes after restart)\r\n",
		ch);

	// content caches
	redis_invalidate_artifact_cache();
	redis_invalidate_fraglist();
	redis_invalidate_epic_zones();
	redis_cache_del("mud:cache:named");

	send_to_char("&+GCleared:&n artifacts (6), fraglist, epic_zones, named\r\n", ch);
}

void do_redis(P_char ch, char *argument, int /*cmd*/)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];

	if (IS_NPC(ch))
		return;

	if (!redis_enabled)
	{
		send_to_char("Redis is not enabled.\r\n", ch);
		return;
	}

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	argument = one_argument(argument, arg3);

	// no args - simple status
	if (!*arg1)
	{
		redis_status_simple(ch);
		return;
	}

	// detailed
	if (is_abbrev(arg1, "detailed"))
	{
		redis_status_detailed(ch);
		return;
	}

	// clear
	if (is_abbrev(arg1, "clear"))
	{
		if (!*arg2)
		{
			send_to_char("Usage: redis clear <cache>\r\n", ch);
			send_to_char(
				"&+cValid:&n world, floor, artifacts, fraglist, epic, named, all\r\n",
				ch);
			return;
		}

		// all needs special handling
		if (is_abbrev(arg2, "all"))
		{
			redis_clear_all(ch, is_abbrev(arg3, "confirm"));
			return;
		}

		// single cache clear
		redis_clear_cache(ch, arg2);
		return;
	}

	// unknown
	send_to_char("Usage: redis [detailed | clear <cache>]\r\n", ch);
}
