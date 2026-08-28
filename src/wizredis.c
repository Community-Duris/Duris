// wiz command for redis status and cache management

#include "prototypes.h"
#include "structs.h"
#include "interp.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "redis.h"
#include "redis_cache_store.h"
#include "redis_donation_worker.h"
#include "redis_floor_store.h"
#include "redis_key_registry.h"
#include "redis_presence_worker.h"
#include "world_recovery_pipeline.h"

static const char *world_worker_state(const world_recovery_health *world)
{
	if (!redis_world_state_enabled)
		return "OFF";
	if (!world->initialized)
		return "READY";
	if (world->capture_active)
		return "CAPTURE";
	if (world->worker_busy || world->queued_generations)
		return "PUBLISH";
	return "IDLE";
}

static void redis_status_simple(P_char ch)
{
	char buf[MAX_STRING_LENGTH];
	int pos = 0;

	pos += snprintf(buf + pos, sizeof(buf) - pos,
			"&+gRedis Status&n &+L(local telemetry; Redis is not queried)&n\r\n");

	const world_recovery_health world = world_recovery_pipeline_health_copy();
	pos += snprintf(buf + pos, sizeof(buf) - pos,
			"  &+cworld_worker&n     &+Y%-9s&n published=%llu failures=%llu\r\n",
			world_worker_state(&world), (unsigned long long)world.published,
			(unsigned long long)world.publish_failures);

	// revisioned player save queue
	int dirty = get_dirty_player_count();
	pos += snprintf(buf + pos, sizeof(buf) - pos,
			"  &+cplayer_queue&n     &+Y%-5d&n    pending saves\r\n", dirty);

	const redis_presence_worker_health presence = redis_presence_worker_health_copy();
	pos += snprintf(buf + pos, sizeof(buf) - pos,
			"  &+cpresence_worker&n  %s%-9s&n queued=%zu active=%zu dropped=%llu\r\n",
			presence.connected ? "&+G" : "&+Y",
			!presence.initialized ? "OFF" :
			presence.connected    ? "HEALTHY" :
						"BACKOFF",
			presence.queued, presence.active_sessions,
			(unsigned long long)presence.dropped);
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
	const redis_floor_store_health floor = redis_floor_store_health_copy();
	pos += snprintf(buf + pos, sizeof(buf) - pos,
			"  &+cfloor_worker&n     %s%-9s&n queued=%zu/%zuB dropped=%llu\r\n",
			floor.connected ? "&+G" : "&+Y",
			!floor.initialized ? "OFF" :
			floor.paused	   ? "BARRIER" :
			floor.connected	   ? "HEALTHY" :
					     "BACKOFF",
			floor.queued_batches, floor.queued_bytes,
			(unsigned long long)floor.dropped_batches);
	const redis_donation_worker_health donation = redis_donation_worker_health_copy();
	pos += snprintf(
		buf + pos, sizeof(buf) - pos,
		"  &+cdonation_worker&n  %s%-9s&n queued=%zu rejected=%llu dropped=%llu\r\n",
		donation.connected ? "&+G" : "&+Y",
		!donation.initialized ? "OFF" :
		donation.connected    ? "HEALTHY" :
					"BACKOFF",
		donation.queued, (unsigned long long)donation.rejected,
		(unsigned long long)donation.dropped);

	send_to_char(buf, ch);
}

static void redis_status_detailed(P_char ch)
{
	char buf[MAX_STRING_LENGTH * 2];

	checked_snprintf(
		buf, sizeof(buf),
		"&+gRedis Status (detailed)&n\r\n"
		"&+LAll values are bounded local telemetry; Redis is not queried.&n\r\n\r\n");

	const redis_presence_worker_health presence = redis_presence_worker_health_copy();
	APPENDF(buf,
		"&+g[Presence Worker]&n\r\n"
		"  state=%s queued=%zu high_water=%zu active=%zu busy=%s\r\n"
		"  submitted=%llu completed=%llu dropped=%llu failures=%llu reconnects=%llu\r\n"
		"  lease_refreshes=%llu lease_failures=%llu\r\n\r\n",
		!presence.initialized ? "off" :
		presence.connected    ? "healthy" :
					"backoff",
		presence.queued, presence.high_water, presence.active_sessions,
		presence.busy ? "yes" : "no", (unsigned long long)presence.submitted,
		(unsigned long long)presence.completed, (unsigned long long)presence.dropped,
		(unsigned long long)presence.command_failures,
		(unsigned long long)presence.reconnects,
		(unsigned long long)presence.lease_refreshes,
		(unsigned long long)presence.lease_failures);
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
	const redis_floor_store_health floor = redis_floor_store_health_copy();
	APPENDF(buf,
		"&+g[Floor Worker]&n\r\n"
		"  state=%s queued=%zu bytes=%zu busy=%s barrier=%s\r\n"
		"  batches=%llu completed=%llu mutations=%llu dropped=%llu failures=%llu reconnects=%llu\r\n\r\n",
		!floor.initialized ? "off" :
		floor.paused	   ? "barrier" :
		floor.connected	   ? "healthy" :
				     "backoff",
		floor.queued_batches, floor.queued_bytes, floor.busy ? "yes" : "no",
		floor.barrier_requested ? "yes" : "no", (unsigned long long)floor.submitted_batches,
		(unsigned long long)floor.completed_batches,
		(unsigned long long)floor.completed_mutations,
		(unsigned long long)floor.dropped_batches,
		(unsigned long long)floor.command_failures, (unsigned long long)floor.reconnects);
	const redis_donation_worker_health donation = redis_donation_worker_health_copy();
	APPENDF(buf,
		"&+g[Donation Worker]&n\r\n"
		"  state=%s queued=%zu high_water=%zu\r\n"
		"  received=%llu validated=%llu rejected=%llu replayed=%llu dropped=%llu\r\n"
		"  connection_failures=%llu reconnects=%llu\r\n\r\n",
		!donation.initialized ? "off" :
		donation.connected    ? "healthy" :
					"backoff",
		donation.queued, donation.high_water, (unsigned long long)donation.received,
		(unsigned long long)donation.validated, (unsigned long long)donation.rejected,
		(unsigned long long)donation.replayed, (unsigned long long)donation.dropped,
		(unsigned long long)donation.connection_failures,
		(unsigned long long)donation.reconnects);

	const world_recovery_health world = world_recovery_pipeline_health_copy();
	APPENDF(buf,
		"&+g[World Recovery Pipeline]&n\r\n"
		"  state=%s capture=%s worker=%s busy=%s queued=%llu\r\n"
		"  requested=%llu coalesced=%llu submitted=%llu published=%llu failures=%llu\r\n"
		"  last_sequence=%llu acknowledged=%llu bytes=%llu high_water=%llu\r\n\r\n",
		world_worker_state(&world), world.capture_active ? "yes" : "no",
		world.worker_running ? "running" : "stopped", world.worker_busy ? "yes" : "no",
		(unsigned long long)world.queued_generations, (unsigned long long)world.requested,
		(unsigned long long)world.coalesced, (unsigned long long)world.submitted,
		(unsigned long long)world.published, (unsigned long long)world.publish_failures,
		(unsigned long long)world.last_submitted_sequence,
		(unsigned long long)world.last_acknowledged_sequence,
		(unsigned long long)world.last_published_bytes,
		(unsigned long long)world.high_water_bytes);

	const int dirty = get_dirty_player_count();
	APPENDF(buf,
		"&+g[Local State]&n\r\n"
		"  player_queue=%d cache_entries=%zu presence_sessions=%zu\r\n"
		"  Remote key counts, TTLs, and existence are intentionally not queried online.\r\n",
		dirty, cache.local_entries, presence.active_sessions);

	send_to_char(buf, ch);
}

static void redis_clear_cache(P_char ch, const char *cache)
{
	char buf[MAX_STRING_LENGTH];

	if (!*cache)
	{
		send_to_char("Usage: redis clear <cache>\r\n", ch);
		send_to_char("&+cOnline:&n artifacts, fraglist, epic, named\r\n", ch);
		send_to_char("&+cMaintenance only:&n world, floor, all\r\n", ch);
		return;
	}

	if (is_abbrev(cache, "world") || is_abbrev(cache, "floor"))
	{
		send_to_char(
			"&+RRefused online:&n recovery clears require a stopped server and the maintenance clear workflow.\r\n",
			ch);
		return;
	}

	if (is_abbrev(cache, "artifacts"))
	{
		if (redis_invalidate_artifact_cache())
			send_to_char(
				"&+GQueued:&n artifacts (6 variants) for background invalidation\r\n",
				ch);
		else
			send_to_char(
				"&+YPartial:&n local artifacts were cleared, but one or more background invalidations were rejected; retry.\r\n",
				ch);
		return;
	}

	if (is_abbrev(cache, "fraglist"))
	{
		if (redis_invalidate_fraglist())
			send_to_char("&+GQueued:&n fraglist for background invalidation\r\n", ch);
		else
			send_to_char(
				"&+RRejected:&n fraglist invalidation was not queued; retry after checking redis detailed.\r\n",
				ch);
		return;
	}

	if (is_abbrev(cache, "epic"))
	{
		if (redis_invalidate_epic_zones())
			send_to_char("&+GQueued:&n epic_zones for background invalidation\r\n", ch);
		else
			send_to_char(
				"&+RRejected:&n epic_zones invalidation was not queued; retry after checking redis detailed.\r\n",
				ch);
		return;
	}

	if (is_abbrev(cache, "named"))
	{
		if (redis_cache_del(REDIS_CACHE_NAMED))
			send_to_char("&+GQueued:&n named for background invalidation\r\n", ch);
		else
			send_to_char(
				"&+RRejected:&n named invalidation was not queued; retry after checking redis detailed.\r\n",
				ch);
		return;
	}

	snprintf(buf, sizeof(buf), "&+RUnknown cache:&n %s\r\n", cache);
	send_to_char(buf, ch);
	send_to_char("&+cOnline:&n artifacts, fraglist, epic, named\r\n", ch);
	send_to_char("&+cMaintenance only:&n world, floor, all\r\n", ch);
}

static void redis_clear_all(P_char ch, bool /*confirmed*/)
{
	send_to_char(
		"&+RRefused online:&n 'redis clear all' requires a stopped server and the maintenance clear workflow.\r\n",
		ch);
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
			send_to_char("&+cOnline:&n artifacts, fraglist, epic, named\r\n", ch);
			send_to_char("&+cMaintenance only:&n world, floor, all\r\n", ch);
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
