// redis dirty saves and world state persistence

#include "prototypes.h"
#include "structs.h"
#include "db.h"
#include "utility.h"
#include "utils.h"
#include "frag_cap_config.h"
#include "redis.h"
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/poll.h> /* local src/poll.h shadows <poll.h> via -I. */
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "config.h"
#include "copyover.h"
#include "donation_event.h"
#include "world_recovery_pipeline.h"
#include "epic.h"
#include "files.h"
#include "player_save_pipeline.h"
#include "player_save_worker.h"
#include "presence_policy.h"
#include "redis_cache_store.h"
#include "redis_presence_payload.h"
#include "redis_presence_worker.h"
#include "redis_world_store.h"
#include "spells.h"
#include "sql.h"
#include "sql_player.h"
#include "ships/ships.h"

#include <algorithm>
#include <chrono>
#include <new>
#include <string>
#include <thread>
#include <vector>

#ifndef __NO_MYSQL__
#include <cjson/cJSON.h>
#include <hiredis/hiredis.h>
#include <openssl/rand.h>
#endif

extern const int top_of_world;
extern int top_of_zone_table;
extern struct zone_data *zone_table;
extern struct room_data *world;
extern P_char character_list;
extern const struct race_names race_names_table[];
extern P_desc descriptor_list;

// ship object vnums defined in ships/ships.h

extern int _pwipe;
#ifdef __NO_MYSQL__
bool redis_clear_ship_snapshots(void)
{
	return true;
}
#endif

static redisContext *redis_ctx = NULL;
bool redis_enabled = false;
bool redis_donation_enabled = false;
bool redis_world_state_enabled = false;
int crash_recovery_boot = 0;

#define REDIS_WORLD_STATE_INTERVAL_DEFAULT 10
#define REDIS_WORLD_STATE_MAX_AGE_DEFAULT 300
#define REDIS_CONNECT_TIMEOUT_MSEC 250
#define REDIS_COMMAND_TIMEOUT_MSEC 100
#define REDIS_WORLD_DRAIN_TIMEOUT_MSEC 30000
#define REDIS_DONATION_MAX_MESSAGES_PER_PULSE 8
#define REDIS_DONATION_MAX_RECONNECT_DELAY 60
#define REDIS_PRESENCE_DRAIN_TIMEOUT_MSEC 1000
#define REDIS_CACHE_DRAIN_TIMEOUT_MSEC 1000

static int world_state_interval = REDIS_WORLD_STATE_INTERVAL_DEFAULT;
static int world_state_max_age = REDIS_WORLD_STATE_MAX_AGE_DEFAULT;
static bool world_recovery_quiesced = false;
static std::string world_writer_token;
static uint64_t world_writer_lease_msec = 0;
static uint64_t world_writer_epoch = 0;
static redisContext *donation_sub_ctx = NULL;
static volatile bool donation_sub_connected = false;
static unsigned int donation_reconnect_delay = 1;
static time_t donation_next_reconnect = 0;
static std::string donation_secret;
static std::vector<std::string> donation_seen_event_ids;
static void donation_sub_drop(const char *reason);
static bool redis_clear_floor_drops_checked(void);
static void redis_prime_artifact_caches(void);

static void donation_schedule_reconnect(void)
{
	donation_next_reconnect = time(NULL) + donation_reconnect_delay;
	donation_reconnect_delay = std::min(donation_reconnect_delay * 2,
					    (unsigned int)REDIS_DONATION_MAX_RECONNECT_DELAY);
}

#ifndef __NO_MYSQL__
static redisReply *redis_command(redisContext *ctx, const char *format, ...);

static void redis_log_command_failure(const char *outcome)
{
	static time_t last_log = 0;
	time_t now = time(NULL);
	if (now == last_log)
		return;
	last_log = now;
	logit(LOG_DEBUG, "redis command failed: outcome=%s", outcome);
}

static redisContext *redis_connect_bounded(const char *host, int port)
{
	struct timeval connect_timeout = { REDIS_CONNECT_TIMEOUT_MSEC / 1000,
					   (REDIS_CONNECT_TIMEOUT_MSEC % 1000) * 1000 };
	struct timeval command_timeout = { REDIS_COMMAND_TIMEOUT_MSEC / 1000,
					   (REDIS_COMMAND_TIMEOUT_MSEC % 1000) * 1000 };
	redisContext *ctx = redisConnectWithTimeout(host, port, connect_timeout);
	if (!ctx || ctx->err)
		return ctx;
	if (redisSetTimeout(ctx, command_timeout) != REDIS_OK)
	{
		redisFree(ctx);
		return NULL;
	}
	return ctx;
}

static redis_world_store_config redis_world_store_config_copy(void)
{
	redis_world_store_config config = {};
	config.host = getenv("REDIS_HOST");
	if (!config.host || !*config.host)
		config.host = "127.0.0.1";
	config.port = 6379;
	const char *port = getenv("REDIS_PORT");
	if (port && *port)
	{
		const int configured = atoi(port);
		if (configured > 0 && configured <= 65535)
			config.port = configured;
	}
	config.connect_timeout_msec = REDIS_CONNECT_TIMEOUT_MSEC;
	config.command_timeout_msec = REDIS_COMMAND_TIMEOUT_MSEC;
	config.season_epoch = world_writer_epoch ? world_writer_epoch : sql_season_epoch();
	return config;
}

static bool redis_epoch_key(char *buffer, size_t size, uint64_t epoch, const char *suffix)
{
	if (!buffer || size < 64 || !epoch || !suffix || !*suffix)
		return false;
	const int written =
		snprintf(buffer, size, "mud:season:%llu:%s", (unsigned long long)epoch, suffix);
	return written > 0 && (size_t)written < size;
}

bool redis_season_key(char *buffer, size_t size, const char *suffix)
{
	return redis_epoch_key(buffer, size, sql_season_epoch(), suffix);
}

static bool redis_world_generation_key(char *buffer, size_t size, uint64_t epoch, uint64_t sequence)
{
	char suffix[96];
	const int written = snprintf(suffix, sizeof suffix, "world_state:generation:%llu",
				     (unsigned long long)sequence);
	return written > 0 && (size_t)written < sizeof suffix &&
	       redis_epoch_key(buffer, size, epoch, suffix);
}

static bool redis_world_writer_token_create(void)
{
	unsigned char random[16] = {};
	if (RAND_bytes(random, sizeof(random)) != 1)
		return false;
	static const char hex[] = "0123456789abcdef";
	world_writer_token.resize(sizeof(random) * 2);
	for (size_t index = 0; index < sizeof(random); ++index)
	{
		world_writer_token[index * 2] = hex[random[index] >> 4];
		world_writer_token[index * 2 + 1] = hex[random[index] & 0x0f];
	}
	return true;
}

static bool redis_world_writer_fence_claim(void)
{
	if (!world_writer_token.empty())
	{
		const redis_world_store_config config = redis_world_store_config_copy();
		return redis_world_store_renew_fence(&config, world_writer_token.c_str(),
						     world_writer_lease_msec);
	}
	world_writer_epoch = sql_season_epoch();
	if (!world_writer_epoch)
		return false;
	if (!redis_world_writer_token_create())
	{
		world_writer_epoch = 0;
		return false;
	}
	world_writer_lease_msec = 10 * 60 * 1000;
	const redis_world_store_config config = redis_world_store_config_copy();
	if (redis_world_store_claim_fence(&config, world_writer_token.c_str(),
					  world_writer_lease_msec))
		return true;
	world_writer_token.clear();
	world_writer_lease_msec = 0;
	world_writer_epoch = 0;
	return false;
}

static bool redis_publish_world_generation(const unsigned char *data, size_t size,
					   const world_recovery_header *header, void * /*context*/)
{
	if (!data || !size || !header || world_writer_token.empty() || !world_writer_lease_msec)
		return false;
	const redis_world_store_config config = redis_world_store_config_copy();
	return redis_world_store_publish(&config, world_writer_token.c_str(),
					 world_writer_lease_msec, data, size, header->sequence,
					 header->timestamp, header->checksum);
}

static bool redis_world_recovery_ensure_initialized(void)
{
	if (world_recovery_quiesced)
		return false;
	if (world_recovery_pipeline_health_copy().initialized)
		return true;
	if (world_writer_token.empty() || !world_writer_lease_msec)
		return false;
	const redis_world_store_config config = redis_world_store_config_copy();
	if (!world_recovery_pipeline_init(redis_publish_world_generation, NULL))
	{
		redis_world_store_release_fence(&config, world_writer_token.c_str());
		world_writer_token.clear();
		world_writer_lease_msec = 0;
		world_writer_epoch = 0;
		return false;
	}
	if (redis_ctx)
	{
		char current_key[128];
		redisReply *sequence_reply =
			redis_epoch_key(current_key, sizeof current_key, world_writer_epoch,
					"world_state:current") ?
				(redisReply *)redis_command(redis_ctx, "GET %s", current_key) :
				NULL;
		if (sequence_reply && sequence_reply->type == REDIS_REPLY_STRING &&
		    sequence_reply->str)
			world_recovery_pipeline_set_sequence_floor(
				strtoull(sequence_reply->str, NULL, 10));
		if (sequence_reply)
			freeReplyObject(sequence_reply);
	}
	return true;
}

static redisReply *redis_command(redisContext *ctx, const char *format, ...)
{
	if (!ctx)
	{
		redis_log_command_failure("unavailable");
		return NULL;
	}
	if (ctx->err)
	{
		redis_log_command_failure(ctx->err == REDIS_ERR_IO ? "timeout_or_io" :
								     "context_error");
		return NULL;
	}

	va_list args;
	va_start(args, format);
	redisReply *reply = (redisReply *)redisvCommand(ctx, format, args);
	va_end(args);

	if (!reply)
	{
		redis_log_command_failure(ctx->err == REDIS_ERR_IO ? "timeout_or_io" : "no_reply");
		return NULL;
	}
	if (reply->type == REDIS_REPLY_ERROR)
	{
		redis_log_command_failure("error_reply");
		freeReplyObject(reply);
		return NULL;
	}
	return reply;
}

static void redis_prime_artifact_caches(void)
{
	if (!redis_ctx || redis_ctx->err)
		return;
	constexpr const char *script = "local result={} for index,key in ipairs(KEYS) do "
				       "result[index*2-1]=redis.call('GET',key) "
				       "result[index*2]=redis.call('PTTL',key) end return result";
	constexpr const char *keys[] = {
		"mud:cache:artifact:1:0", "mud:cache:artifact:1:1", "mud:cache:artifact:2:0",
		"mud:cache:artifact:2:1", "mud:cache:artifact:3:0", "mud:cache:artifact:3:1",
	};
	redisReply *reply = (redisReply *)redis_command(
		redis_ctx,
		"EVAL %b 6 mud:cache:artifact:1:0 mud:cache:artifact:1:1 mud:cache:artifact:2:0 mud:cache:artifact:2:1 mud:cache:artifact:3:0 mud:cache:artifact:3:1",
		script, strlen(script));
	if (!reply || reply->type != REDIS_REPLY_ARRAY || reply->elements != 12)
	{
		if (reply)
			freeReplyObject(reply);
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
		redis_cache_store_seed(keys[index], value->str, (int)seconds);
	}
	freeReplyObject(reply);
}

static bool redis_append_command(redisContext *ctx, const char *format, ...)
{
	if (!ctx || ctx->err || !format)
	{
		redis_log_command_failure(!ctx ? "unavailable" : "context_error");
		return false;
	}
	va_list args;
	va_start(args, format);
	const int result = redisvAppendCommand(ctx, format, args);
	va_end(args);
	if (result != REDIS_OK)
		redis_log_command_failure(ctx->err == REDIS_ERR_IO ? "timeout_or_io" :
								     "append_failed");
	return result == REDIS_OK;
}

static bool redis_collect_integer_replies(redisContext *ctx, size_t count)
{
	bool valid = true;
	for (size_t index = 0; index < count; ++index)
	{
		void *raw_reply = NULL;
		if (!ctx || redisGetReply(ctx, &raw_reply) != REDIS_OK || !raw_reply)
		{
			redis_log_command_failure(
				ctx && ctx->err == REDIS_ERR_IO ? "timeout_or_io" : "no_reply");
			return false;
		}
		redisReply *reply = (redisReply *)raw_reply;
		if (reply->type != REDIS_REPLY_INTEGER)
		{
			redis_log_command_failure(reply->type == REDIS_REPLY_ERROR ?
							  "error_reply" :
							  "unexpected_reply");
			valid = false;
		}
		freeReplyObject(reply);
	}
	return valid;
}

#endif

/* Scan-and-delete with MATCH pattern. Fail closed if SCAN/DEL misshape. */
static bool redis_clear_scan_match(const char *pattern)
{
#ifndef __NO_MYSQL__
	char cursor[64] = "0";

	if (!pattern || !redis_enabled || !redis_ctx)
		return false;

	do
	{
		redisReply *scan = (redisReply *)redis_command(
			redis_ctx, "SCAN %s MATCH %s COUNT 256", cursor, pattern);
		if (!scan || scan->type != REDIS_REPLY_ARRAY || scan->elements != 2 ||
		    !scan->element[0] || !scan->element[1] || !scan->element[0]->str ||
		    scan->element[0]->type != REDIS_REPLY_STRING ||
		    scan->element[1]->type != REDIS_REPLY_ARRAY)
		{
			if (scan)
				freeReplyObject(scan);
			return false;
		}

		snprintf(cursor, sizeof(cursor), "%s", scan->element[0]->str);
		redisReply *keys = scan->element[1];
		for (size_t i = 0; i < keys->elements; i++)
		{
			redisReply *key = keys->element[i];
			if (!key || key->type != REDIS_REPLY_STRING || !key->str)
			{
				freeReplyObject(scan);
				return false;
			}
			redisReply *del = (redisReply *)redis_command(redis_ctx, "DEL %b", key->str,
								      key->len);
			if (!del ||
			    (del->type != REDIS_REPLY_INTEGER && del->type != REDIS_REPLY_NIL))
			{
				if (del)
					freeReplyObject(del);
				freeReplyObject(scan);
				return false;
			}
			freeReplyObject(del);
		}
		freeReplyObject(scan);
	} while (strcmp(cursor, "0") != 0);

	return true;
#else
	(void)pattern;
	return true;
#endif
}

static bool redis_delete_key_checked(const char *key)
{
#ifndef __NO_MYSQL__
	if (!key || !*key || !redis_enabled || !redis_ctx)
		return false;
	redisReply *reply = (redisReply *)redis_command(redis_ctx, "DEL %s", key);
	const bool deleted = reply && reply->type == REDIS_REPLY_INTEGER;
	if (reply)
		freeReplyObject(reply);
	return deleted;
#else
	(void)key;
	return true;
#endif
}

// rnum to vnum
static int get_room_vnum(P_char ch)
{
	if (!ch || ch->in_room < 0 || ch->in_room > top_of_world)
		return NOWHERE;
	return world[ch->in_room].number;
}

static bool redis_reconnect(void)
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (redis_ctx)
	{
		redisFree(redis_ctx);
		redis_ctx = NULL;
	}

	const char *redis_host = getenv("REDIS_HOST");
	if (!redis_host || !*redis_host)
		redis_host = "127.0.0.1";

	const char *redis_port_str = getenv("REDIS_PORT");
	int redis_port = 6379;
	if (redis_port_str && *redis_port_str)
	{
		redis_port = atoi(redis_port_str);
		if (redis_port <= 0 || redis_port > 65535)
			redis_port = 6379;
	}

	redis_ctx = redis_connect_bounded(redis_host, redis_port);
	if (!redis_ctx || redis_ctx->err)
	{
		if (redis_ctx)
		{
			redisFree(redis_ctx);
			redis_ctx = NULL;
		}
		return false;
	}
	logit(LOG_SYS, "redis reconnected to %s:%d", redis_host, redis_port);
	return true;
#endif
}

void event_flush_dirty_players(P_char ch, P_char victim, P_obj obj, void *data);

bool redis_init(void)
{
#ifdef __NO_MYSQL__
	redis_enabled = false;
	return true;
#else
	const char *redis_env = getenv("REDIS");
	if (!redis_env || strcasecmp(redis_env, "TRUE") != 0)
	{
		logit(LOG_SYS, "redis disabled (set REDIS=TRUE in .env to enable)");
		redis_enabled = false;
		redis_donation_enabled = false;
		donation_secret.clear();
		return true;
	}
	redis_enabled = true;
	world_recovery_quiesced = false;
	world_writer_token.clear();
	world_writer_lease_msec = 0;
	world_writer_epoch = 0;
	redis_donation_enabled = false;
	donation_secret.clear();
	const char *donation_env = getenv("REDIS_DONATION_SUBSCRIBER");
	if (donation_env && strcasecmp(donation_env, "TRUE") == 0)
	{
		const char *secret = getenv("REDIS_DONATION_SECRET");
		if (secret && strlen(secret) >= 32)
		{
			redis_donation_enabled = true;
			donation_secret = secret;
		}
		else
			logit(LOG_SYS,
			      "redis: donation subscriber disabled; REDIS_DONATION_SECRET must be at least 32 bytes");
	}

	const char *redis_host = getenv("REDIS_HOST");
	if (!redis_host || !*redis_host)
		redis_host = "127.0.0.1";

	const char *redis_port_str = getenv("REDIS_PORT");
	int redis_port = 6379;
	if (redis_port_str && *redis_port_str)
	{
		redis_port = atoi(redis_port_str);
		if (redis_port <= 0 || redis_port > 65535)
			redis_port = 6379;
	}

	redis_ctx = redis_connect_bounded(redis_host, redis_port);
	if (!redis_ctx)
	{
		logit(LOG_SYS, "redis: failed to allocate context");
	}
	else if (redis_ctx->err)
	{
		logit(LOG_SYS, "redis connect failed: outcome=unavailable");
		redisFree(redis_ctx);
		redis_ctx = NULL;
	}
	else
	{
		logit(LOG_SYS, "redis connected to %s:%d", redis_host, redis_port);
	}

	const redis_presence_worker_config presence_config = { redis_host, redis_port,
							       REDIS_CONNECT_TIMEOUT_MSEC,
							       REDIS_COMMAND_TIMEOUT_MSEC };
	if (!redis_presence_worker_init(&presence_config))
		logit(LOG_SYS, "redis: presence worker unavailable; presence updates disabled");
	const redis_cache_store_config cache_config = { redis_host, redis_port,
							REDIS_CONNECT_TIMEOUT_MSEC,
							REDIS_COMMAND_TIMEOUT_MSEC };
	if (!redis_cache_store_init(&cache_config))
		logit(LOG_SYS, "redis: cache worker unavailable; report caches disabled");
	else
		redis_prime_artifact_caches();

	// check for world state persistence
	const char *world_state_env = getenv("REDIS_WORLD_STATE");
	if (world_state_env && strcasecmp(world_state_env, "TRUE") == 0)
	{
		redis_world_state_enabled = true;

		const char *interval_str = getenv("REDIS_WORLD_STATE_INTERVAL");
		if (interval_str && *interval_str)
		{
			int interval = atoi(interval_str);
			if (interval >= 5 && interval <= 300)
				world_state_interval = interval;
		}

		const char *max_age_str = getenv("REDIS_WORLD_STATE_MAX_AGE");
		if (max_age_str && *max_age_str)
		{
			int max_age = atoi(max_age_str);
			if (max_age >= 60 && max_age <= 3600)
				world_state_max_age = max_age;
		}

		logit(LOG_SYS, "redis world state enabled: interval=%ds, max_age=%ds",
		      world_state_interval, world_state_max_age);
	}

	// note: flush event scheduled in ne_init_events() after event system is ready

	if (redis_ctx)
	{
		if (redis_donation_enabled)
			redis_donation_subscribe_init();
		if (redis_world_state_enabled && !redis_world_writer_fence_claim())
		{
			world_recovery_quiesced = true;
			logit(LOG_SYS,
			      "redis: world publisher disabled; writer lease unavailable at boot");
		}
	}
	else if (redis_world_state_enabled)
		world_recovery_quiesced = true;

	return true;
#endif
}

bool redis_clear_pwipe_state(void)
{
	if (!redis_enabled)
		return true;
	redis_presence_worker_cancel();
	redis_cache_store_cancel();
	if ((!redis_ctx || redis_ctx->err) && !redis_reconnect())
		return false;

	if (!redis_clear_world_state())
		return false;
	return redis_clear_floor_drops_checked() && redis_delete_key_checked("mud:floor_drops") &&
	       redis_delete_key_checked("mud:floor_pickups") &&
	       redis_delete_key_checked("mud:online") &&
	       redis_clear_scan_match("mud:presence_op:*") &&
	       redis_clear_scan_match("mud:world_state:generation:*") &&
	       redis_delete_key_checked("mud:world_state:current") &&
	       redis_delete_key_checked("mud:world_state:timestamp") &&
	       redis_delete_key_checked("mud:world_state:sequence") &&
	       redis_delete_key_checked("mud:world_state:checksum") &&
	       redis_delete_key_checked("mud:world_state:complete") &&
	       redis_delete_key_checked("mud:world_state:writer_fence") &&
	       redis_clear_scan_match("mud:cache:*") && redis_clear_ship_snapshots();
}

bool redis_validate_pwipe_state(void)
{
#ifdef __NO_MYSQL__
	return true;
#else
	if (!redis_enabled)
		return true;
	const redis_world_store_config config = redis_world_store_config_copy();
	redisContext *context = redis_connect_bounded(config.host, config.port);
	if (!context || context->err)
	{
		if (context)
			redisFree(context);
		return false;
	}
	redisReply *reply = (redisReply *)redis_command(context, "PING");
	const bool ready = reply && reply->type == REDIS_REPLY_STATUS && reply->str &&
			   !strcmp(reply->str, "PONG");
	if (reply)
		freeReplyObject(reply);
	redisFree(context);
	return ready;
#endif
}

void redis_cleanup(void)
{
#ifndef __NO_MYSQL__
	if (!redis_presence_worker_shutdown(REDIS_PRESENCE_DRAIN_TIMEOUT_MSEC))
		logit(LOG_SYS, "redis: presence worker drain timed out during shutdown");
	if (!redis_cache_store_shutdown(REDIS_CACHE_DRAIN_TIMEOUT_MSEC))
		logit(LOG_SYS, "redis: cache worker drain timed out during shutdown");
	if (redis_world_state_enabled)
	{
		const bool drained = !world_recovery_pipeline_health_copy().initialized ||
				     redis_world_recovery_drain(REDIS_WORLD_DRAIN_TIMEOUT_MSEC);
		if (!drained)
			logit(LOG_SYS, "redis: world recovery drain timed out during shutdown");
		if (world_recovery_pipeline_health_copy().initialized)
		{
			if (drained)
				world_recovery_pipeline_shutdown();
			else
				world_recovery_pipeline_cancel();
		}
	}
	if (!world_writer_token.empty())
	{
		const redis_world_store_config config = redis_world_store_config_copy();
		redis_world_store_release_fence(&config, world_writer_token.c_str());
	}
	world_writer_token.clear();
	world_writer_lease_msec = 0;
	world_writer_epoch = 0;
	if (redis_ctx)
	{
		redisFree(redis_ctx);
		redis_ctx = NULL;
	}
	if (donation_sub_ctx)
	{
		redisFree(donation_sub_ctx);
		donation_sub_ctx = NULL;
	}
	donation_sub_connected = false;
	donation_next_reconnect = 0;
	donation_reconnect_delay = 1;
	donation_seen_event_ids.clear();
	donation_secret.clear();
	redis_donation_enabled = false;
	redis_enabled = false;
#endif
}

void redis_clear_floor_pickups(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx)
		return;

	redisReply *reply = (redisReply *)redis_command(redis_ctx, "DEL mud:floor_pickups");
	if (reply)
		freeReplyObject(reply);
#endif
}

#define MAX_FLOOR_DROP_BATCH 1024

static struct
{
	unsigned long uid;
	int vnum;
	int room_vnum;
	int type;
	int values[8];
	time_t timers[6];
	char *name;
	char *short_desc;
	char *long_desc;
	int content_vnums[16];
	int content_count;
} floor_drop_batch[MAX_FLOOR_DROP_BATCH];

static int floor_drop_batch_count = 0;
static unsigned long floor_drop_removes[MAX_FLOOR_DROP_BATCH];
static int floor_drop_remove_count = 0;

void redis_log_floor_drop(P_obj obj, int room_vnum)
{
	if (_pwipe)
		return;
#ifndef __NO_MYSQL__
	if (!redis_world_state_enabled || !obj || obj->obj_uid == 0)
		return;

	// skip old corpses (vnum 2, value[6] is timestamp) - older than 24h
	if (OBJ_VNUM(obj) == 2 && obj->value[6] > 0)
	{
		time_t corpse_time = (time_t)obj->value[6];
		if (time(NULL) - corpse_time > 86400)
			return;
	}

	if (floor_drop_batch_count >= MAX_FLOOR_DROP_BATCH)
	{
		redis_flush_floor_drops();
		if (floor_drop_batch_count >= MAX_FLOOR_DROP_BATCH)
		{
			logit(LOG_SYS, "redis: floor delta retry buffer is full");
			return;
		}
	}

	int idx = floor_drop_batch_count++;
	floor_drop_batch[idx].uid = obj->obj_uid;
	floor_drop_batch[idx].vnum = OBJ_VNUM(obj);
	floor_drop_batch[idx].room_vnum = room_vnum;
	floor_drop_batch[idx].type = obj->type;

	for (int i = 0; i < NUMB_OBJ_VALS; i++)
		floor_drop_batch[idx].values[i] = obj->value[i];

	for (int i = 0; i < 6; i++)
		floor_drop_batch[idx].timers[i] = obj->timer[i];

	floor_drop_batch[idx].name = (obj->name && obj->name[0]) ? str_dup(obj->name) : NULL;
	floor_drop_batch[idx].short_desc = (obj->short_description && obj->short_description[0]) ?
						   str_dup(obj->short_description) :
						   NULL;
	floor_drop_batch[idx].long_desc =
		(obj->description && obj->description[0]) ? str_dup(obj->description) : NULL;

	floor_drop_batch[idx].content_count = 0;
	P_obj content;
	for (content = obj->contains; content && floor_drop_batch[idx].content_count < 16;
	     content = content->next_content)
	{
		floor_drop_batch[idx].content_vnums[floor_drop_batch[idx].content_count++] =
			OBJ_VNUM(content);
	}
#endif
}

bool redis_flush_floor_drops(void)
{
#ifndef __NO_MYSQL__
	if (!redis_world_state_enabled)
		return true;
	if (world_recovery_pipeline_busy())
		return false;
	if (!redis_enabled || !redis_ctx)
		return false;

	if (floor_drop_batch_count == 0 && floor_drop_remove_count == 0)
		return true;
	char floor_key[128];
	if (!redis_season_key(floor_key, sizeof floor_key, "floor_drops"))
		return false;

	size_t appended = 0;
	bool queued = true;
	// Queue removes first, followed by replacement values. Hiredis sends the buffered
	// commands together when replies are collected, avoiding one round trip per delta.
	for (int i = 0; i < floor_drop_remove_count; i++)
	{
		if (!redis_append_command(redis_ctx, "HDEL %s %lu", floor_key,
					  floor_drop_removes[i]))
		{
			queued = false;
			break;
		}
		++appended;
	}

	// Queue adds only if every remove was accepted into the client output buffer.
	for (int i = 0; queued && i < floor_drop_batch_count; i++)
	{
		cJSON *o = cJSON_CreateObject();
		if (!o)
		{
			redis_collect_integer_replies(redis_ctx, appended);
			return false;
		}

		cJSON_AddNumberToObject(o, "uid", (double)floor_drop_batch[i].uid);
		cJSON_AddNumberToObject(o, "v", floor_drop_batch[i].vnum);
		cJSON_AddNumberToObject(o, "rm", floor_drop_batch[i].room_vnum);
		cJSON_AddNumberToObject(o, "tp", floor_drop_batch[i].type);

		for (int j = 0; j < 8; j++)
		{
			if (floor_drop_batch[i].values[j] != 0)
			{
				char key[16];
				snprintf(key, sizeof(key), "v%d", j);
				cJSON_AddNumberToObject(o, key, floor_drop_batch[i].values[j]);
			}
		}

		bool has_timer = false;
		for (int j = 0; j < 6; j++)
		{
			if (floor_drop_batch[i].timers[j] != 0)
			{
				has_timer = true;
				break;
			}
		}
		if (has_timer)
		{
			cJSON *tmr = cJSON_CreateArray();
			for (int j = 0; j < 6; j++)
				cJSON_AddItemToArray(
					tmr,
					cJSON_CreateNumber((double)floor_drop_batch[i].timers[j]));
			cJSON_AddItemToObject(o, "tmr", tmr);
		}

		if (floor_drop_batch[i].name)
			cJSON_AddStringToObject(o, "nm", floor_drop_batch[i].name);
		if (floor_drop_batch[i].short_desc)
			cJSON_AddStringToObject(o, "sd", floor_drop_batch[i].short_desc);
		if (floor_drop_batch[i].long_desc)
			cJSON_AddStringToObject(o, "ld", floor_drop_batch[i].long_desc);

		if (floor_drop_batch[i].content_count > 0)
		{
			cJSON *contents = cJSON_CreateArray();
			for (int j = 0; j < floor_drop_batch[i].content_count; j++)
				cJSON_AddItemToArray(
					contents,
					cJSON_CreateNumber(floor_drop_batch[i].content_vnums[j]));
			cJSON_AddItemToObject(o, "con", contents);
		}

		char *json_str = cJSON_PrintUnformatted(o);
		cJSON_Delete(o);
		if (!json_str)
		{
			redis_collect_integer_replies(redis_ctx, appended);
			return false;
		}

		queued = redis_append_command(redis_ctx, "HSET %s %lu %s", floor_key,
					      floor_drop_batch[i].uid, json_str);
		free(json_str);
		if (queued)
			++appended;
	}

	const bool flushed = redis_collect_integer_replies(redis_ctx, appended);
	if (!queued || !flushed ||
	    appended != (size_t)(floor_drop_remove_count + floor_drop_batch_count))
		return false;

	floor_drop_remove_count = 0;
	for (int i = 0; i < floor_drop_batch_count; i++)
	{
		if (floor_drop_batch[i].name)
			str_free(floor_drop_batch[i].name);
		if (floor_drop_batch[i].short_desc)
			str_free(floor_drop_batch[i].short_desc);
		if (floor_drop_batch[i].long_desc)
			str_free(floor_drop_batch[i].long_desc);
		floor_drop_batch[i].name = NULL;
		floor_drop_batch[i].short_desc = NULL;
		floor_drop_batch[i].long_desc = NULL;
	}
	floor_drop_batch_count = 0;
	return true;
#else
	return false;
#endif
}

void redis_remove_floor_drop(unsigned long obj_uid)
{
#ifndef __NO_MYSQL__
	if (!redis_world_state_enabled || obj_uid == 0)
		return;

	// check if it's in the pending batch - remove from there first
	for (int i = 0; i < floor_drop_batch_count; i++)
	{
		if (floor_drop_batch[i].uid == obj_uid)
		{
			if (floor_drop_batch[i].name)
				str_free(floor_drop_batch[i].name);
			if (floor_drop_batch[i].short_desc)
				str_free(floor_drop_batch[i].short_desc);
			if (floor_drop_batch[i].long_desc)
				str_free(floor_drop_batch[i].long_desc);

			// shift remaining entries
			for (int j = i; j < floor_drop_batch_count - 1; j++)
				floor_drop_batch[j] = floor_drop_batch[j + 1];
			floor_drop_batch_count--;
			return;
		}
	}

	// not in batch, queue for removal from redis
	if (floor_drop_remove_count < MAX_FLOOR_DROP_BATCH)
		floor_drop_removes[floor_drop_remove_count++] = obj_uid;
#endif
}

static bool redis_clear_floor_drops_checked(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx)
		return false;
	char floor_key[128];
	const uint64_t epoch = world_writer_epoch ? world_writer_epoch : sql_season_epoch();
	if (!redis_epoch_key(floor_key, sizeof floor_key, epoch, "floor_drops"))
		return false;

	redisReply *reply = (redisReply *)redis_command(redis_ctx, "DEL %s", floor_key);
	if (!reply || reply->type != REDIS_REPLY_INTEGER)
	{
		if (reply)
			freeReplyObject(reply);
		return false;
	}
	freeReplyObject(reply);
	return true;
#else
	return false;
#endif
}

void redis_clear_floor_drops(void)
{
	redis_clear_floor_drops_checked();
}

int redis_restore_floor_drops(void)
{
#ifndef __NO_MYSQL__
	if (!redis_world_state_enabled || !redis_enabled || !redis_ctx)
		return 0;
	char floor_key[128];
	if (!redis_season_key(floor_key, sizeof floor_key, "floor_drops"))
		return 0;

	redisReply *reply = (redisReply *)redis_command(redis_ctx, "HGETALL %s", floor_key);
	if (!reply || reply->type != REDIS_REPLY_ARRAY)
	{
		if (reply)
			freeReplyObject(reply);
		return 0;
	}

	int restored = 0, skipped = 0;

	// hgetall returns [key, val, key, val...]
	for (size_t i = 0; i + 1 < reply->elements; i += 2)
	{
		const char *uid_str = reply->element[i]->str;
		const char *json_str = reply->element[i + 1]->str;
		if (!uid_str || !json_str)
			continue;

		unsigned long uid = strtoul(uid_str, NULL, 10);
		if (uid == 0)
			continue;

		cJSON *obj_json = cJSON_Parse(json_str);
		if (!obj_json)
			continue;

		cJSON *v = cJSON_GetObjectItem(obj_json, "v");
		cJSON *rm = cJSON_GetObjectItem(obj_json, "rm");
		if (!v || !rm)
		{
			cJSON_Delete(obj_json);
			continue;
		}

		int vnum = v->valueint;
		int room_vnum = rm->valueint;
		int rnum = real_room(room_vnum);
		if (rnum < 0 || rnum > top_of_world)
		{
			cJSON_Delete(obj_json);
			continue;
		}
		char room_ref[32];
		snprintf(room_ref, sizeof(room_ref), "%d", room_vnum);
		if (!sql_persistence_item_owner_matches(uid, "room", room_ref,
							"redis_restore_floor_drops"))
		{
			skipped++;
			cJSON_Delete(obj_json);
			continue;
		}

		P_obj obj = read_object(vnum, VIRTUAL);
		if (!obj)
		{
			cJSON_Delete(obj_json);
			continue;
		}

		obj->obj_uid = uid;

		cJSON *tp = cJSON_GetObjectItem(obj_json, "tp");
		if (tp && cJSON_IsNumber(tp))
			obj->type = tp->valueint;

		for (int j = 0; j < NUMB_OBJ_VALS; j++)
		{
			char key[16];
			snprintf(key, sizeof(key), "v%d", j);
			cJSON *val = cJSON_GetObjectItem(obj_json, key);
			if (val && cJSON_IsNumber(val))
				obj->value[j] = val->valueint;
		}

		cJSON *tmr = cJSON_GetObjectItem(obj_json, "tmr");
		if (tmr && cJSON_IsArray(tmr))
		{
			int idx = 0;
			cJSON *t;
			cJSON_ArrayForEach(t, tmr)
			{
				if (idx < 6 && cJSON_IsNumber(t))
					obj->timer[idx] = (time_t)t->valuedouble;
				idx++;
			}
		}

		cJSON *nm = cJSON_GetObjectItem(obj_json, "nm");
		cJSON *sd = cJSON_GetObjectItem(obj_json, "sd");
		cJSON *ld = cJSON_GetObjectItem(obj_json, "ld");
		if (nm && cJSON_IsString(nm) && nm->valuestring[0])
		{
			if ((obj->str_mask & STRUNG_KEYS) && obj->name)
				str_free(obj->name);
			obj->name = str_dup(nm->valuestring);
			obj->str_mask |= STRUNG_KEYS;
		}
		if (sd && cJSON_IsString(sd) && sd->valuestring[0])
		{
			if ((obj->str_mask & STRUNG_DESC2) && obj->short_description)
				str_free(obj->short_description);
			obj->short_description = str_dup(sd->valuestring);
			obj->str_mask |= STRUNG_DESC2;
		}
		if (ld && cJSON_IsString(ld) && ld->valuestring[0])
		{
			if ((obj->str_mask & STRUNG_DESC1) && obj->description)
				str_free(obj->description);
			obj->description = str_dup(ld->valuestring);
			obj->str_mask |= STRUNG_DESC1;
		}

		obj_to_room(obj, rnum);

		cJSON *con = cJSON_GetObjectItem(obj_json, "con");
		if (con && cJSON_IsArray(con))
		{
			cJSON *cont_vnum;
			cJSON_ArrayForEach(cont_vnum, con)
			{
				if (!cJSON_IsNumber(cont_vnum))
					continue;
				int content_vnum = cont_vnum->valueint;
				if (content_vnum > 0)
				{
					P_obj content = read_object(content_vnum, VIRTUAL);
					if (content)
						obj_to_obj(content, obj);
				}
			}
		}

		cJSON_Delete(obj_json);
		restored++;
	}

	freeReplyObject(reply);

	if (skipped > 0)
		logit(LOG_SYS, "redis: floor drops: skipped %d non-authoritative items", skipped);
	if (restored > 0)
		logit(LOG_SYS, "redis: floor drops: restored %d items", restored);

	// dont clear floor_drops here - world_state restore needs to check against it
	// cleared after world_state restore completes

	return restored;
#else
	return 0;
#endif
}

void mark_player_dirty(int pid)
{
	if (_pwipe)
		return;
	mark_player_dirty_components(pid, PLAYER_CHECKPOINT_COMPONENT_ALL);
}

void mark_player_dirty_components(int pid, player_component_mask_t components)
{
	if (_pwipe)
		return;
	player_save_pipeline_mark(pid, components);
}

void flush_dirty_players(void)
{
	for (P_char ch = character_list; ch; ch = ch->next)
		if (IS_PC(ch) && GET_PID(ch) > 0)
			player_save_pipeline_checkpoint_dirty(ch, RENT_CRASH, get_room_vnum(ch));
}

int get_dirty_player_count(void)
{
	return static_cast<int>(player_save_pipeline_dirty_count());
}

struct persistence_dirty_save_snapshot redis_dirty_save_snapshot_copy(void)
{
	struct persistence_dirty_save_snapshot snapshot = {};
	const player_save_pipeline_health pipeline = player_save_pipeline_health_copy();
	const player_save_worker_health worker = player_save_worker_health_copy();
	snapshot.enabled = 1;
	snapshot.available = pipeline.initialized ? 1 : 0;
	snapshot.active_count = player_save_pipeline_dirty_count();
	snapshot.inflight_count = worker.inflight_pids;
	snapshot.inflight_oldest_age_msec = worker.oldest_age_msec;
	return snapshot;
}

void event_flush_dirty_players(P_char /*ch*/, P_char /*victim*/, P_obj /*obj*/, void * /*data*/)
{
	constexpr size_t DIRTY_PLAYER_BATCH_SIZE = 8;
	static std::vector<uint64_t> character_ids;
	static size_t cursor = 0;

	if (character_ids.empty())
	{
		try
		{
			for (P_char character = character_list; character;
			     character = character->next)
				if (IS_PC(character) && GET_PID(character) > 0 &&
				    character->runtime_id)
					character_ids.push_back(character->runtime_id);
		}
		catch (const std::bad_alloc &)
		{
			character_ids.clear();
			cursor = 0;
			nevent_periodic_retry_after(WAIT_SEC,
						    "dirty-player snapshot allocation failed");
			return;
		}
	}

	size_t processed = 0;
	while (cursor < character_ids.size() && processed < DIRTY_PLAYER_BATCH_SIZE)
	{
		P_char character = find_character_by_runtime_id(character_ids[cursor++]);
		processed++;
		if (character && IS_PC(character) && GET_PID(character) > 0)
			player_save_pipeline_checkpoint_dirty(character, RENT_CRASH,
							      get_room_vnum(character));
	}

	if (cursor < character_ids.size())
	{
		nevent_periodic_continue_after(1);
		return;
	}

	character_ids.clear();
	cursor = 0;
	if (redis_world_state_enabled)
		redis_flush_floor_drops();
}

bool redis_save_world_state(void)
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (!redis_enabled || !redis_world_state_enabled)
		return false;
	if (!redis_world_recovery_ensure_initialized())
	{
		logit(LOG_SYS, "redis: world recovery worker unavailable");
		return false;
	}
	redis_world_recovery_pulse();
	if (world_recovery_pipeline_busy())
		return true;
	if ((!redis_ctx || redis_ctx->err) && !redis_reconnect())
		return false;
	if (!redis_flush_floor_drops())
		return false;
	logit(LOG_SYS, "redis: starting bounded world recovery capture");
	return world_recovery_pipeline_request();
#endif
}

void redis_world_recovery_pulse(void)
{
#ifndef __NO_MYSQL__
	if (!redis_world_state_enabled)
		return;
	if (!world_recovery_pipeline_health_copy().initialized)
		return;
	world_recovery_pipeline_pulse();
	world_recovery_completion completion = {};
	while (world_recovery_pipeline_take_completion(&completion))
	{
		const world_recovery_health recovery = world_recovery_pipeline_health_copy();
		if (completion.published &&
		    completion.sequence == recovery.last_acknowledged_sequence)
		{
			logit(LOG_SYS,
			      "redis: world recovery generation and floor handoff acknowledged sequence=%llu attempts=%u",
			      (unsigned long long)completion.sequence, completion.attempts);
		}
		else if (!completion.published)
			logit(LOG_SYS,
			      "redis: world recovery generation publish failed sequence=%llu attempts=%u",
			      (unsigned long long)completion.sequence, completion.attempts);
	}
#endif
}

bool redis_world_recovery_drain(uint64_t timeout_msec)
{
#ifdef __NO_MYSQL__
	(void)timeout_msec;
	return true;
#else
	if (!redis_world_state_enabled)
		return true;
	if (!world_recovery_pipeline_health_copy().initialized)
		return true;
	const auto deadline =
		std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_msec);
	while (std::chrono::steady_clock::now() < deadline)
	{
		redis_world_recovery_pulse();
		if (!world_recovery_pipeline_busy())
			return true;
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return false;
#endif
}

bool redis_world_recovery_quiesce(void)
{
#ifdef __NO_MYSQL__
	return true;
#else
	world_recovery_quiesced = true;
	if (world_recovery_pipeline_health_copy().initialized)
		world_recovery_pipeline_cancel();
	return redis_world_writer_fence_claim();
#endif
}

bool redis_has_world_state(void)
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (!redis_enabled || !redis_world_state_enabled)
		return false;

	if (!redis_ctx || redis_ctx->err)
	{
		if (!redis_reconnect())
			return false;
	}

	// The atomic current pointer and self-validating framed blob are authoritative.
	// Other metadata keys are operator diagnostics and cannot invalidate a good blob.
	const uint64_t epoch = sql_season_epoch();
	char current_key[128];
	if (!redis_epoch_key(current_key, sizeof current_key, epoch, "world_state:current"))
		return false;
	redisReply *reply = (redisReply *)redis_command(redis_ctx, "GET %s", current_key);
	if (!reply)
		return false;
	uint64_t sequence = 0;
	if (reply->type == REDIS_REPLY_STRING && reply->str)
		sequence = strtoull(reply->str, NULL, 10);
	freeReplyObject(reply);
	if (!sequence)
		return false;
	char generation_key[160];
	if (!redis_world_generation_key(generation_key, sizeof generation_key, epoch, sequence))
		return false;
	reply = (redisReply *)redis_command(redis_ctx, "GET %s", generation_key);
	if (!reply)
		return false;
	world_recovery_header header = {};
	const bool exists =
		reply->type == REDIS_REPLY_STRING && reply->str && reply->len > 0 &&
		world_recovery_validate(reinterpret_cast<const unsigned char *>(reply->str),
					reply->len, world_state_max_age, sequence, &header) &&
		header.sequence == sequence;
	freeReplyObject(reply);
	return exists;
#endif
}

time_t redis_world_state_timestamp(void)
{
#ifdef __NO_MYSQL__
	return 0;
#else
	if (!redis_enabled || !redis_ctx)
		return 0;
	char timestamp_key[128];
	if (!redis_season_key(timestamp_key, sizeof timestamp_key, "world_state:timestamp"))
		return 0;

	redisReply *reply = (redisReply *)redis_command(redis_ctx, "GET %s", timestamp_key);
	if (!reply)
		return 0;

	time_t ts = 0;
	if (reply->type == REDIS_REPLY_STRING && reply->str)
		ts = (time_t)atol(reply->str);

	freeReplyObject(reply);
	return ts;
#endif
}

bool redis_clear_world_state(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || ((!redis_ctx || redis_ctx->err) && !redis_reconnect()))
		return false;
	const bool quiesced = redis_world_recovery_quiesce();
	if (!quiesced)
		return false;
	char generation_pattern[160];
	char current_key[128];
	char timestamp_key[128];
	char sequence_key[128];
	char checksum_key[128];
	char complete_key[128];
	if (!redis_epoch_key(generation_pattern, sizeof generation_pattern, world_writer_epoch,
			     "world_state:generation:*") ||
	    !redis_epoch_key(current_key, sizeof current_key, world_writer_epoch,
			     "world_state:current") ||
	    !redis_epoch_key(timestamp_key, sizeof timestamp_key, world_writer_epoch,
			     "world_state:timestamp") ||
	    !redis_epoch_key(sequence_key, sizeof sequence_key, world_writer_epoch,
			     "world_state:sequence") ||
	    !redis_epoch_key(checksum_key, sizeof checksum_key, world_writer_epoch,
			     "world_state:checksum") ||
	    !redis_epoch_key(complete_key, sizeof complete_key, world_writer_epoch,
			     "world_state:complete"))
		return false;
	const bool generations_cleared = redis_clear_scan_match(generation_pattern);

	redisReply *reply = (redisReply *)redis_command(redis_ctx, "DEL %s %s %s %s %s",
							current_key, timestamp_key, sequence_key,
							checksum_key, complete_key);
	const bool metadata_cleared = reply && reply->type == REDIS_REPLY_INTEGER;
	if (reply)
		freeReplyObject(reply);

	if (quiesced && generations_cleared && metadata_cleared)
		logit(LOG_SYS, "redis: cleared and quiesced world recovery until restart");
	return quiesced && generations_cleared && metadata_cleared;
#else
	return false;
#endif
}

bool redis_load_world_state(void)
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (!redis_enabled || !redis_world_state_enabled)
		return false;

	if (!redis_ctx || redis_ctx->err)
	{
		if (!redis_reconnect())
			return false;
	}

	// restore floor drops first - has most recent data
	redis_restore_floor_drops();
	const uint64_t epoch = sql_season_epoch();
	char current_key[128];
	if (!redis_epoch_key(current_key, sizeof current_key, epoch, "world_state:current"))
		return false;
	redisReply *sequence_reply = (redisReply *)redis_command(redis_ctx, "GET %s", current_key);
	if (!sequence_reply)
		return false;
	uint64_t expected_sequence = 0;
	if (sequence_reply->type == REDIS_REPLY_STRING && sequence_reply->str)
		expected_sequence = strtoull(sequence_reply->str, NULL, 10);
	freeReplyObject(sequence_reply);
	if (!expected_sequence)
		return false;

	char generation_key[160];
	if (!redis_world_generation_key(generation_key, sizeof generation_key, epoch,
					expected_sequence))
		return false;
	redisReply *reply = (redisReply *)redis_command(redis_ctx, "GET %s", generation_key);
	if (!reply || redis_ctx->err)
	{
		if (reply)
			freeReplyObject(reply);
		return false;
	}

	if (reply->type != REDIS_REPLY_STRING || !reply->str || reply->len == 0)
	{
		freeReplyObject(reply);
		return false;
	}

	world_recovery_header header = {};
	const bool result =
		world_recovery_restore(reinterpret_cast<const unsigned char *>(reply->str),
				       reply->len, world_state_max_age, expected_sequence, &header);
	freeReplyObject(reply);
	if (!result || header.sequence != expected_sequence)
	{
		logit(LOG_SYS, "redis: rejected invalid world recovery generation");
		return false;
	}
	redis_clear_floor_pickups();
	redis_clear_floor_drops();
	logit(LOG_SYS,
	      "redis: restored world recovery generation sequence=%llu mobs=%u objs=%u doors=%u zones=%u",
	      (unsigned long long)header.sequence, header.mob_count, header.object_count,
	      header.door_count, header.zone_count);
	return true;
#endif
}

void event_save_world_state(P_char /*ch*/, P_char /*victim*/, P_obj /*obj*/, void * /*data*/)
{
	if (redis_enabled && redis_world_state_enabled)
	{
		if (!redis_save_world_state())
			nevent_periodic_mark_failure("world-state persistence did not complete");
	}
	else
		nevent_periodic_mark_failure("world-state persistence is disabled");
	nevent_periodic_next_after(world_state_interval * WAIT_SEC);
}

bool redis_cache_set(const char *key, const char *value)
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (!redis_enabled || !key || !value)
		return false;
	return redis_cache_store_set(key, value, 0);
#endif
}

bool redis_cache_set_ex(const char *key, int seconds, const char *value)
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (!redis_enabled || !key || !value || seconds <= 0)
		return false;
	return redis_cache_store_set(key, value, seconds);
#endif
}

char *redis_cache_get(const char *key)
{
#ifdef __NO_MYSQL__
	return NULL;
#else
	if (!redis_enabled || !key)
		return NULL;
	return redis_cache_store_get(key);
#endif
}

void redis_cache_del(const char *key)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !key)
		return;
	redis_cache_store_delete(key);
#endif
}

#ifndef __NO_MYSQL__
static void redis_ship_cache_key(char *buf, size_t buf_size, const char *owner_name)
{
	snprintf(buf, buf_size, "ship:snapshot:%s", owner_name ? owner_name : "");
}

void redis_invalidate_ship_snapshot(const char *owner_name)
{
	if (!owner_name)
		return;

	char key[256];
	redis_ship_cache_key(key, sizeof(key), owner_name);
	redis_cache_del(key);
}

bool redis_clear_ship_snapshots(void)
{
	if (!redis_enabled || !redis_ctx)
		return true;

	char cursor[64] = "0";
	do
	{
		redisReply *scan = (redisReply *)redis_command(
			redis_ctx, "SCAN %s MATCH ship:snapshot:* COUNT 256", cursor);
		if (!scan || scan->type != REDIS_REPLY_ARRAY || scan->elements != 2 ||
		    !scan->element[0] || !scan->element[1] || !scan->element[0]->str ||
		    scan->element[0]->type != REDIS_REPLY_STRING ||
		    scan->element[1]->type != REDIS_REPLY_ARRAY)
		{
			if (scan)
				freeReplyObject(scan);
			return false;
		}

		snprintf(cursor, sizeof(cursor), "%s", scan->element[0]->str);
		redisReply *keys = scan->element[1];
		for (size_t i = 0; i < keys->elements; i++)
		{
			redisReply *key = keys->element[i];
			if (!key || key->type != REDIS_REPLY_STRING || !key->str)
			{
				freeReplyObject(scan);
				return false;
			}
			redisReply *del = (redisReply *)redis_command(redis_ctx, "DEL %b", key->str,
								      key->len);
			if (!del ||
			    (del->type != REDIS_REPLY_INTEGER && del->type != REDIS_REPLY_NIL))
			{
				if (del)
					freeReplyObject(del);
				freeReplyObject(scan);
				return false;
			}
			freeReplyObject(del);
		}
		freeReplyObject(scan);
	} while (strcmp(cursor, "0") != 0);

	return true;
}
#endif

void redis_donation_subscribe_init(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_donation_enabled || donation_secret.empty() ||
	    time(NULL) < donation_next_reconnect)
		return;

	const char *redis_host = getenv("REDIS_HOST");
	if (!redis_host || !*redis_host)
		redis_host = "127.0.0.1";

	const char *redis_port_str = getenv("REDIS_PORT");
	int redis_port = 6379;
	if (redis_port_str && *redis_port_str)
	{
		redis_port = atoi(redis_port_str);
		if (redis_port <= 0 || redis_port > 65535)
			redis_port = 6379;
	}

	donation_sub_ctx = redis_connect_bounded(redis_host, redis_port);
	if (!donation_sub_ctx || donation_sub_ctx->err)
	{
		if (donation_sub_ctx)
		{
			redisFree(donation_sub_ctx);
			donation_sub_ctx = NULL;
		}
		donation_schedule_reconnect();
		logit(LOG_SYS, "redis: donation subscriber failed to connect; retry delayed");
		return;
	}

	redisReply *reply = (redisReply *)redis_command(donation_sub_ctx, "SUBSCRIBE mud:nchat");
	if (!reply || reply->type != REDIS_REPLY_ARRAY)
	{
		if (reply)
			freeReplyObject(reply);
		donation_sub_drop("subscribe failed");
		return;
	}
	freeReplyObject(reply);

	donation_sub_connected = true;
	donation_next_reconnect = 0;
	donation_reconnect_delay = 1;
	logit(LOG_SYS, "redis: donation subscriber connected to mud:nchat");
#endif
}

static void broadcast_donation_nchat(const struct donation_event *event)
{
	char buf[MAX_STRING_LENGTH];
	P_desc i;
	P_char to;
	const double amount = (double)event->amount_cents / 100.0;

	if (event->is_public)
	{
		if (event->message[0])
			snprintf(buf, sizeof(buf),
				 "&+Y%s&n&+m donated &+W%.2f %s&n&+m: &+w'%s'&n\n",
				 event->character_name, amount, event->currency, event->message);
		else
			snprintf(buf, sizeof(buf), "&+Y%s&n&+m donated &+W%.2f %s&n&+m!&n\n",
				 event->character_name, amount, event->currency);
	}
	else
	{
		if (event->message[0])
			snprintf(buf, sizeof(buf),
				 "&+Yan anonymous donor&n&+m gave &+W%.2f %s&n&+m: &+w'%s'&n\n",
				 amount, event->currency, event->message);
		else
			snprintf(buf, sizeof(buf),
				 "&+Yan anonymous donor&n&+m gave &+W%.2f %s&n&+m!&n\n", amount,
				 event->currency);
	}

	for (i = descriptor_list; i; i = i->next)
	{
		if (i->connected || !(to = i->character))
			continue;
		if (IS_NPC(to) || !PLR2_FLAGGED(to, PLR2_NCHAT))
			continue;
		send_to_char(buf, to);
	}

	logit(LOG_SYS, "donation: event=%s donor=%s amount=%.2f currency=%s", event->event_id,
	      event->is_public ? event->character_name : "anonymous", amount, event->currency);
}

static void handle_donation_reply(redisReply *reply)
{
#ifndef __NO_MYSQL__
	if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 3)
	{
		if (reply->element[0]->type == REDIS_REPLY_STRING &&
		    strcmp(reply->element[0]->str, "message") == 0 &&
		    reply->element[2]->type == REDIS_REPLY_STRING)
		{
			struct donation_event event = {};
			if (!donation_event_decode(reply->element[2]->str, reply->element[2]->len,
						   donation_secret.c_str(), time(NULL), &event))
				return;

			const std::string event_id(event.event_id);
			if (std::find(donation_seen_event_ids.begin(),
				      donation_seen_event_ids.end(),
				      event_id) != donation_seen_event_ids.end())
				return;
			if (donation_seen_event_ids.size() >= 256)
				donation_seen_event_ids.erase(donation_seen_event_ids.begin());
			donation_seen_event_ids.push_back(event_id);
			broadcast_donation_nchat(&event);
		}
	}
#endif
}

static void donation_sub_drop(const char *reason)
{
#ifndef __NO_MYSQL__
	donation_schedule_reconnect();
	logit(LOG_SYS, "redis: donation subscriber error: %s; retry delayed", reason);
	donation_sub_connected = false;
	if (donation_sub_ctx)
	{
		redisFree(donation_sub_ctx);
		donation_sub_ctx = NULL;
	}
#endif
}

void redis_check_donation_messages(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_donation_enabled)
		return;

	// attempt reconnect if disconnected
	if (!donation_sub_connected || !donation_sub_ctx)
	{
		redis_donation_subscribe_init();
		if (!donation_sub_connected)
			return;
	}

	/* The subscriber socket is blocking with a 100ms timeout, so calling
	   redisGetReply() unconditionally stalls the whole game loop for that
	   timeout on every idle pulse.  Only touch the socket when it already
	   has data, then drain the complete replies the reader holds. */
	struct pollfd pfd;

	pfd.fd = donation_sub_ctx->fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	if (poll(&pfd, 1, 0) < 0)
	{
		if (errno == EINTR || errno == EAGAIN)
			return;
		donation_sub_drop(strerror(errno));
		return;
	}

	if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
	{
		donation_sub_drop("subscriber socket closed");
		return;
	}

	if (pfd.revents & POLLIN)
	{
		if (redisBufferRead(donation_sub_ctx) != REDIS_OK)
		{
			donation_sub_drop(donation_sub_ctx->errstr);
			return;
		}
	}

	for (int handled = 0; handled < REDIS_DONATION_MAX_MESSAGES_PER_PULSE; ++handled)
	{
		redisReply *reply = NULL;

		if (redisGetReplyFromReader(donation_sub_ctx, (void **)&reply) != REDIS_OK)
		{
			donation_sub_drop(donation_sub_ctx->errstr);
			return;
		}

		if (!reply)
			break;

		handle_donation_reply(reply);
		freeReplyObject(reply);
	}
#endif
}

void event_check_donation_messages(P_char /*ch*/, P_char /*victim*/, P_obj /*obj*/, void * /*data*/)
{
	redis_check_donation_messages();
}

// forward declare from random.mob.c
struct zone_random_data
{
	int zone;
	int races[10];
	int proc_spells[3][2];
};
extern struct zone_random_data zones_random_data[];
extern Skill skills[];

static char *generate_named_report(void)
{
	char *output = (char *)malloc(MAX_STRING_LENGTH * 4);
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
				const char *spellColor = "&+B";

				if (zones_random_data[i].proc_spells[x][1] == SPELL_STONE_SKIN ||
				    zones_random_data[i].proc_spells[x][1] == SPELL_INVIGORATE)
					spellColor = "&+G";
				else if (zones_random_data[i].proc_spells[x][1] ==
					 SPELL_CONJURE_ELEMENTAL)
					spellColor = "&+y";

				snprintf(buf, sizeof(buf), "%s%s%s &+W(&+Y%d&+W)&n",
					 x != 0 ? "&+W,&n " : "", spellColor,
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

void redis_cache_named_report(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled)
		return;

	char *report = generate_named_report();
	if (report)
	{
		redis_cache_set("mud:cache:named", report);
		free(report);
		logit(LOG_SYS, "redis: cached named report");
	}
#endif
}

char *redis_get_named_report(void)
{
	return redis_cache_get("mud:cache:named");
}

// fraglist cache
extern void get_level_cap_info(long *max_frags, int *racewar, int *level, time_t *next_update);
extern int sql_level_cap(int racewar_side);
extern const racewar_struct racewar_color[];

#define MAX_FRAG_SIZE 10

static char *generate_fraglist_output(void)
{
#ifdef __NO_MYSQL__
	return NULL;
#else
	char *output = (char *)malloc(65536);
	if (!output)
		return NULL;

	output[0] = '\0';
	char buf[2048], name[256];
	int frags, count;
	float fragnum;
	int cap_level, cap_racewar, cap_others;
	long cap_frags;
	time_t cap_timer;
	int days, hours, mins, secs;
	MYSQL_RES *res;
	MYSQL_ROW row;

	get_level_cap_info(&cap_frags, &cap_racewar, &cap_level, &cap_timer);
	cap_others = sql_level_cap((cap_racewar == RACEWAR_GOOD) ? RACEWAR_EVIL : RACEWAR_GOOD);
	cap_timer -= time(NULL);

	if (cap_timer <= 0)
	{
		secs = mins = hours = days = 0;
	}
	else
	{
		secs = cap_timer % 60;
		cap_timer /= 60;
		mins = cap_timer % 60;
		cap_timer /= 60;
		hours = cap_timer % 24;
		cap_timer /= 24;
		days = cap_timer;
	}

	snprintf(
		output, 65536,
		"&+YFrag Level Cap:&+w %d - &+%c%s&n, &+w%d&N - Others, &+YTop Frag Amount: &+w%d.%02d\n"
		"&+YTimer:&+w %02d:%02d:%02d:%02d &+YFrags needed:&+w %.2f&n\n\n&+WTop Fraggers\n\n",
		cap_level, racewar_color[cap_racewar].color, racewar_color[cap_racewar].name,
		cap_others, (int)(cap_frags / 100), (int)(cap_frags % 100), days, hours, mins, secs,
		frag_cap_config_frags_for_level(cap_level + 1));

	// query top fraggers (no filter)
	res = db_query("SELECT char_name, total_frags FROM frag_leaderboard "
		       "WHERE deleted_at IS NULL ORDER BY total_frags DESC LIMIT %d",
		       MAX_FRAG_SIZE);
	if (res)
	{
		count = 0;
		while ((row = mysql_fetch_row(res)) && count < MAX_FRAG_SIZE)
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

		while (count < MAX_FRAG_SIZE)
		{
			snprintf(buf, sizeof(buf), "   &+Y%-30s             &+R% 6.2f\r\n",
				 "Nobody", 0.0);
			strcat(output, buf);
			count++;
		}
	}

	strcat(output, "\r\n\r\n&+LLowest Fraggers\r\n\r\n");

	// query lowest fraggers
	res = db_query("SELECT char_name, total_frags FROM frag_leaderboard "
		       "WHERE deleted_at IS NULL ORDER BY total_frags ASC LIMIT %d",
		       MAX_FRAG_SIZE);
	if (res)
	{
		count = 0;
		while ((row = mysql_fetch_row(res)) && count < MAX_FRAG_SIZE)
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

		while (count < MAX_FRAG_SIZE)
		{
			snprintf(buf, sizeof(buf), "   &+Y%-30s             &+R% 6.2f\r\n",
				 "Nobody", 0.0);
			strcat(output, buf);
			count++;
		}
	}

	strcat(output, "\r\n");
	return output;
#endif
}

void redis_cache_fraglist(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled)
		return;

	char *output = generate_fraglist_output();
	if (output)
	{
		redis_cache_set("mud:cache:fraglist", output);
		free(output);
		logit(LOG_SYS, "redis: cached fraglist");
	}
#endif
}

char *redis_get_fraglist(void)
{
	return redis_cache_get("mud:cache:fraglist");
}

void redis_invalidate_fraglist(void)
{
	redis_cache_del("mud:cache:fraglist");
}

// epic zones cache - 15 min ttl for alignment display
#define EPIC_ZONES_CACHE_TTL 900

void redis_cache_epic_zones(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled)
		return;

	char *output = generate_epic_zones_output();
	if (output)
	{
		redis_cache_set_ex("mud:cache:epic_zones", EPIC_ZONES_CACHE_TTL, output);
		free(output);
		logit(LOG_SYS, "redis: cached epic zones");
	}
#endif
}

char *redis_get_epic_zones(void)
{
	return redis_cache_get("mud:cache:epic_zones");
}

void redis_invalidate_epic_zones(void)
{
	redis_cache_del("mud:cache:epic_zones");
}

// online players list for web
void redis_player_online(P_char ch)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !ch || IS_NPC(ch))
		return;
	if (!durisweb_presence_character_visible(ch))
	{
		redis_presence_worker_submit_offline(GET_PID(ch), false);
		return;
	}

	const char *account = get_account_name_safe(ch);
	const char *race_str = race_names_table[GET_RACE(ch)].ansi;
	const char *class_str = get_class_name(ch, NULL);
	const char *ip = (ch->desc && ch->desc->host[0]) ? ch->desc->host : "";
	const char *client = (ch->desc && ch->desc->client_name[0]) ? ch->desc->client_name : "";
	const char *client_ver =
		(ch->desc && ch->desc->client_version[0]) ? ch->desc->client_version : "";
	time_t login_time = ch->player.time.logon;

	const redis_presence_fields fields = { GET_NAME(ch),
					       account,
					       race_str,
					       class_str,
					       ip,
					       client,
					       client_ver,
					       GET_LEVEL(ch),
					       GET_RACEWAR(ch),
					       static_cast<int64_t>(login_time),
					       durisweb_private_presence_enabled() };
	char *json = redis_presence_payload_encode(fields);
	if (!json)
		return;

	redis_presence_worker_submit_online(GET_PID(ch), json, true);
	free(json);
#endif
}

void redis_player_offline(P_char ch)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !ch || IS_NPC(ch))
		return;
	redis_presence_worker_submit_offline(GET_PID(ch), durisweb_presence_character_visible(ch));
#endif
}

void redis_clear_online_players(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled)
		return;
	redis_presence_worker_submit_clear();
#endif
}

// arti cache
static const char *get_artifact_cache_key(int type, bool godlist)
{
	static char key[64];
	snprintf(key, sizeof(key), "mud:cache:artifact:%d:%d", type, godlist ? 1 : 0);
	return key;
}

void redis_cache_artifact_list(int type, bool godlist, const char *json)
{
#ifndef __NO_MYSQL__
	constexpr int ARTIFACT_CACHE_TTL_SECONDS = 900;
	if (!redis_enabled || type < 1 || type > 3 || !json)
		return;
	redis_cache_set_ex(get_artifact_cache_key(type, godlist), ARTIFACT_CACHE_TTL_SECONDS, json);
#endif
}

char *redis_get_artifact_list(int type, bool godlist)
{
	return redis_cache_get(get_artifact_cache_key(type, godlist));
}

void redis_invalidate_artifact_list(int type, bool godlist)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || type < 1 || type > 3)
		return;
	redis_cache_del(get_artifact_cache_key(type, godlist));
#endif
}

void redis_invalidate_artifact_cache(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled)
		return;

	for (int t = 1; t <= 3; t++)
	{
		redis_cache_del(get_artifact_cache_key(t, false));
		redis_cache_del(get_artifact_cache_key(t, true));
	}
#endif
}

// generic helpers for wiz command

bool redis_key_exists(const char *key)
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (!redis_enabled || !redis_ctx || !key)
		return false;

	redisReply *reply = (redisReply *)redis_command(redis_ctx, "EXISTS %s", key);
	if (!reply)
		return false;

	bool exists = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
	freeReplyObject(reply);
	return exists;
#endif
}

long redis_get_ttl(const char *key)
{
#ifdef __NO_MYSQL__
	return -1;
#else
	if (!redis_enabled || !redis_ctx || !key)
		return -1;

	redisReply *reply = (redisReply *)redis_command(redis_ctx, "TTL %s", key);
	if (!reply)
		return -1;

	long ttl = -1;
	if (reply->type == REDIS_REPLY_INTEGER)
		ttl = (long)reply->integer;

	freeReplyObject(reply);
	return ttl;
#endif
}

long redis_hlen(const char *key)
{
#ifdef __NO_MYSQL__
	return 0;
#else
	if (!redis_enabled || !redis_ctx || !key)
		return 0;

	redisReply *reply = (redisReply *)redis_command(redis_ctx, "HLEN %s", key);
	if (!reply)
		return 0;

	long len = 0;
	if (reply->type == REDIS_REPLY_INTEGER)
		len = (long)reply->integer;

	freeReplyObject(reply);
	return len;
#endif
}

long redis_scard(const char *key)
{
#ifdef __NO_MYSQL__
	return 0;
#else
	if (!redis_enabled || !redis_ctx || !key)
		return 0;

	redisReply *reply = (redisReply *)redis_command(redis_ctx, "SCARD %s", key);
	if (!reply)
		return 0;

	long card = 0;
	if (reply->type == REDIS_REPLY_INTEGER)
		card = (long)reply->integer;

	freeReplyObject(reply);
	return card;
#endif
}
