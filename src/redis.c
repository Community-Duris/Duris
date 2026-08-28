// redis dirty saves and world state persistence

#include "prototypes.h"
#include "structs.h"
#include "db.h"
#include "utility.h"
#include "utils.h"
#include "redis.h"
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "config.h"
#include "copyover.h"
#include "world_recovery_pipeline.h"
#include "files.h"
#include "redis_command_observability.h"
#include "redis_connection.h"
#include "redis_donation_worker.h"
#include "redis_donation_runtime.h"
#include "redis_floor_runtime.h"
#include "redis_floor_store.h"
#include "redis_key_registry.h"
#include "redis_maintenance.h"
#include "redis_namespace.h"
#include "redis_presence_runtime.h"
#include "redis_presence_worker.h"
#include "redis_report_cache.h"
#include "redis_runtime_config.h"
#include "redis_world_store.h"
#include "world_recovery_codec.h"
#include "sql.h"
#include "sql_player.h"
#include "ships/ships.h"

#include <algorithm>
#include <array>
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

// ship object vnums defined in ships/ships.h

extern int _pwipe;

static redisContext *redis_ctx = NULL;
static redis_runtime_connections redis_connections = {};
bool redis_enabled = false;
bool redis_world_state_enabled = false;
int crash_recovery_boot = 0;
int clean_restart_recovery_boot = 0;

#define REDIS_WORLD_STATE_INTERVAL_DEFAULT 10
#define REDIS_WORLD_STATE_MAX_AGE_DEFAULT 300
#define REDIS_WORLD_DRAIN_TIMEOUT_MSEC 30000
#define REDIS_PRESENCE_DRAIN_TIMEOUT_MSEC 1000
#define REDIS_CACHE_DRAIN_TIMEOUT_MSEC 1000
#define REDIS_FLOOR_DRAIN_TIMEOUT_MSEC 1000

static int world_state_interval = REDIS_WORLD_STATE_INTERVAL_DEFAULT;
static int world_state_max_age = REDIS_WORLD_STATE_MAX_AGE_DEFAULT;
static bool world_recovery_quiesced = false;
static std::string world_writer_token;
static std::string redis_world_authentication_secret;
static std::string redis_world_previous_authentication_secret;
static uint64_t world_writer_lease_msec = 0;
static uint64_t world_writer_epoch = 0;
static bool world_floor_barrier_waiting = false;
static bool world_floor_handoff_active = false;
static uint64_t clean_shutdown_sequence = 0;
static uint64_t world_sequence_floor = 0;
static uint64_t redis_runtime_epoch = 0;
static char redis_key_namespace[64] = {};
static char redis_presence_current_key[160] = {};
static char redis_presence_session_prefix[160] = {};
static char redis_presence_session_pattern[160] = {};
static char redis_presence_retry_prefix[160] = {};
static char redis_presence_retry_pattern[160] = {};
static char redis_presence_event_channel[160] = {};
static char redis_donation_channel[160] = {};
static bool redis_clear_floor_drops_checked(void);

#ifndef __NO_MYSQL__
static redisReply *redis_command(redis_shared_command_scope scope, redis_shared_command_kind kind,
				 redisContext *ctx, const char *format, ...);
static redisReply *redis_command_argv(redis_shared_command_scope scope,
				      redis_shared_command_kind kind, redisContext *ctx, int argc,
				      const char **arguments, const size_t *lengths);

static void redis_log_command_failure(const char *outcome)
{
	static time_t last_log = 0;
	time_t now = time(NULL);
	if (now == last_log)
		return;
	last_log = now;
	logit(LOG_DEBUG, "redis command failed: outcome=%s", outcome);
}

static redis_world_store_config redis_world_store_config_copy(void)
{
	redis_world_store_config config = {};
	config.connection = redis_connections.world;
	config.key_namespace = redis_key_namespace;
	config.authentication_secret = redis_world_authentication_secret.c_str();
	config.previous_authentication_secret =
		redis_world_previous_authentication_secret.empty() ?
			NULL :
			redis_world_previous_authentication_secret.c_str();
	config.season_epoch = world_writer_epoch ? world_writer_epoch : redis_runtime_epoch;
	config.generation_ttl_seconds = std::max<uint64_t>(3600, world_state_max_age * 4);
	return config;
}

static bool redis_epoch_key(char *buffer, size_t size, uint64_t epoch, const char *suffix)
{
	return redis_namespace_season_key(redis_key_namespace, epoch, suffix, buffer, size);
}

bool redis_season_key(char *buffer, size_t size, const char *suffix)
{
	return redis_epoch_key(buffer, size, redis_runtime_epoch, suffix);
}

static bool redis_configure_epoch_surfaces(uint64_t epoch)
{
	if (!epoch ||
	    !redis_epoch_key(redis_presence_current_key, sizeof redis_presence_current_key, epoch,
			     REDIS_PRESENCE_CURRENT) ||
	    !redis_epoch_key(redis_presence_session_prefix, sizeof redis_presence_session_prefix,
			     epoch, REDIS_PRESENCE_SESSION_PREFIX) ||
	    !redis_epoch_key(redis_presence_session_pattern, sizeof redis_presence_session_pattern,
			     epoch, REDIS_PRESENCE_SESSION_PATTERN) ||
	    !redis_epoch_key(redis_presence_retry_prefix, sizeof redis_presence_retry_prefix, epoch,
			     REDIS_PRESENCE_RETRY_PREFIX) ||
	    !redis_epoch_key(redis_presence_retry_pattern, sizeof redis_presence_retry_pattern,
			     epoch, REDIS_PRESENCE_RETRY_PATTERN) ||
	    !redis_epoch_key(redis_presence_event_channel, sizeof redis_presence_event_channel,
			     epoch, REDIS_PRESENCE_EVENT_CHANNEL) ||
	    !redis_epoch_key(redis_donation_channel, sizeof redis_donation_channel, epoch,
			     REDIS_DONATION_CHANNEL) ||
	    !redis_floor_runtime_configure(redis_key_namespace, epoch) ||
	    !redis_report_cache_configure(redis_key_namespace, epoch))
		return false;
	redis_runtime_epoch = epoch;
	return true;
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
	world_writer_epoch = redis_runtime_epoch;
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
					   const world_recovery_header *header,
					   redis_shared_command_outcome *outcome,
					   void * /*context*/)
{
	if (!data || !size || !header || world_writer_token.empty() || !world_writer_lease_msec)
		return false;
	const redis_world_store_config config = redis_world_store_config_copy();
	return redis_world_store_publish_observed(&config, world_writer_token.c_str(),
						  world_writer_lease_msec, data, size,
						  header->sequence, header->timestamp,
						  header->checksum, outcome);
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
					REDIS_WORLD_CURRENT_SUFFIX) ?
				(redisReply *)redis_command(REDIS_SHARED_SCOPE_WORLD,
							    REDIS_SHARED_COMMAND_READ, redis_ctx,
							    "GET %s", current_key) :
				NULL;
		if (sequence_reply && sequence_reply->type == REDIS_REPLY_STRING &&
		    sequence_reply->str)
			world_sequence_floor = std::max<uint64_t>(
				world_sequence_floor, strtoull(sequence_reply->str, NULL, 10));
		if (sequence_reply)
			freeReplyObject(sequence_reply);
	}
	if (world_sequence_floor)
		world_recovery_pipeline_set_sequence_floor(world_sequence_floor);
	return true;
}

static redisReply *redis_command_finish(redis_shared_command_scope scope,
					redis_shared_command_kind kind, redisContext *ctx,
					uint64_t started_usec, redisReply *reply)
{
	const uint64_t finished_usec = persistence_observability_now_usec();
	const uint64_t duration_usec =
		finished_usec >= started_usec ? finished_usec - started_usec : 0;
	redis_shared_command_outcome outcome = REDIS_SHARED_OUTCOME_SUCCESS;
	const char *label = NULL;
	if (!ctx || ctx->err)
	{
		outcome = redis_command_outcome(ctx, false);
		label = outcome == REDIS_SHARED_OUTCOME_UNAVAILABLE ? "unavailable" :
			outcome == REDIS_SHARED_OUTCOME_TIMEOUT	    ? "timeout" :
								      "transport";
	}
	else if (!reply)
	{
		outcome = REDIS_SHARED_OUTCOME_NO_REPLY;
		label = "no_reply";
	}
	else if (reply->type == REDIS_REPLY_ERROR)
	{
		outcome = REDIS_SHARED_OUTCOME_ERROR_REPLY;
		label = "error_reply";
	}
	redis_shared_command_observability_record(scope, kind, outcome, duration_usec);
	if (outcome == REDIS_SHARED_OUTCOME_SUCCESS)
		return reply;
	redis_log_command_failure(label);
	if (reply)
		freeReplyObject(reply);
	return NULL;
}

static redisReply *redis_command(redis_shared_command_scope scope, redis_shared_command_kind kind,
				 redisContext *ctx, const char *format, ...)
{
	const uint64_t started_usec = persistence_observability_now_usec();
	redisReply *reply = NULL;
	if (ctx && !ctx->err)
	{
		va_list args;
		va_start(args, format);
		reply = (redisReply *)redisvCommand(ctx, format, args);
		va_end(args);
	}
	return redis_command_finish(scope, kind, ctx, started_usec, reply);
}

static redisReply *redis_command_argv(redis_shared_command_scope scope,
				      redis_shared_command_kind kind, redisContext *ctx, int argc,
				      const char **arguments, const size_t *lengths)
{
	const uint64_t started_usec = persistence_observability_now_usec();
	redisReply *reply = NULL;
	if (ctx && !ctx->err)
		reply = static_cast<redisReply *>(redisCommandArgv(ctx, argc, arguments, lengths));
	return redis_command_finish(scope, kind, ctx, started_usec, reply);
}

#endif

static bool redis_scan_match_empty(redis_shared_command_scope scope, const char *pattern)
{
#ifndef __NO_MYSQL__
	char cursor[64] = "0";
	if (!pattern || !redis_enabled || !redis_ctx)
		return false;
	do
	{
		redisReply *scan =
			(redisReply *)redis_command(scope, REDIS_SHARED_COMMAND_SCAN, redis_ctx,
						    "SCAN %s MATCH %s COUNT 1", cursor, pattern);
		if (!scan || scan->type != REDIS_REPLY_ARRAY || scan->elements != 2 ||
		    !scan->element[0] || !scan->element[1] || !scan->element[0]->str ||
		    scan->element[0]->type != REDIS_REPLY_STRING ||
		    scan->element[1]->type != REDIS_REPLY_ARRAY)
		{
			if (scan)
				freeReplyObject(scan);
			return false;
		}
		redisReply *keys = scan->element[1];
		const bool empty = keys->elements == 0;
		snprintf(cursor, sizeof(cursor), "%s", scan->element[0]->str);
		freeReplyObject(scan);
		if (!empty)
			return false;
	} while (strcmp(cursor, "0") != 0);
	return true;
#else
	(void)scope;
	(void)pattern;
	return true;
#endif
}

/* Scan-and-delete with MATCH pattern and verify an empty postcondition. */
static bool redis_clear_scan_match(redis_shared_command_scope scope, const char *pattern)
{
#ifndef __NO_MYSQL__
	char cursor[64] = "0";

	if (!pattern || !redis_enabled || !redis_ctx)
		return false;

	do
	{
		redisReply *scan =
			(redisReply *)redis_command(scope, REDIS_SHARED_COMMAND_SCAN, redis_ctx,
						    "SCAN %s MATCH %s COUNT 256", cursor, pattern);
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
			redisReply *del = (redisReply *)redis_command(scope,
								      REDIS_SHARED_COMMAND_WRITE,
								      redis_ctx, "DEL %b", key->str,
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

	return redis_scan_match_empty(scope, pattern);
#else
	(void)scope;
	(void)pattern;
	return true;
#endif
}

static bool redis_configure_namespace(void)
{
	return redis_namespace_validate(getenv("REDIS_NAMESPACE"), getenv("ENVIRONMENT"),
					redis_key_namespace, sizeof redis_key_namespace);
}

static void redis_clear_world_authentication_secrets(void)
{
	std::fill(redis_world_authentication_secret.begin(),
		  redis_world_authentication_secret.end(), '\0');
	std::fill(redis_world_previous_authentication_secret.begin(),
		  redis_world_previous_authentication_secret.end(), '\0');
	redis_world_authentication_secret.clear();
	redis_world_previous_authentication_secret.clear();
}

static bool redis_configure_world_authentication(void)
{
	const char *current = getenv("REDIS_WORLD_STATE_SECRET");
	const char *previous = getenv("REDIS_WORLD_STATE_SECRET_PREVIOUS");
	const size_t current_size = current ? strlen(current) : 0;
	const size_t previous_size = previous ? strlen(previous) : 0;
	if (current_size < 32 || current_size > 256 ||
	    (previous_size && (previous_size < 32 || previous_size > 256)))
		return false;
	try
	{
		redis_world_authentication_secret.assign(current, current_size);
		if (previous_size)
			redis_world_previous_authentication_secret.assign(previous, previous_size);
	}
	catch (const std::bad_alloc &)
	{
		redis_clear_world_authentication_secrets();
		return false;
	}
	return true;
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

	redis_ctx = redis_connection_open(redis_connections.world);
	if (!redis_ctx || redis_ctx->err)
	{
		if (redis_ctx)
		{
			redisFree(redis_ctx);
			redis_ctx = NULL;
		}
		redis_shared_connection_observability_record(true, false);
		return false;
	}
	redis_shared_connection_observability_record(true, true);
	logit(LOG_SYS, "redis reconnected");
	return true;
#endif
}

bool redis_init(void)
{
	redis_shared_command_observability_reset(false);
	redis_donation_runtime_set_enabled(false);
	redis_floor_runtime_reset();
	redis_presence_runtime_set_enabled(false);
	redis_report_cache_reset();
#ifdef __NO_MYSQL__
	redis_enabled = false;
	return true;
#else
	const char *redis_env = getenv("REDIS");
	if (!redis_env || strcasecmp(redis_env, "TRUE") != 0)
	{
		logit(LOG_SYS, "redis disabled (set REDIS=TRUE in .env to enable)");
		redis_enabled = false;
		redis_donation_runtime_set_enabled(false);
		redis_world_state_enabled = false;
		redis_runtime_epoch = 0;
		redis_key_namespace[0] = '\0';
		redis_clear_world_authentication_secrets();
		return true;
	}
	redis_enabled = true;
	redis_shared_command_observability_set_enabled(true);
	redis_world_state_enabled = false;
	redis_runtime_epoch = 0;
	redis_key_namespace[0] = '\0';
	redis_clear_world_authentication_secrets();
	if (!redis_configure_namespace())
	{
		logit(LOG_SYS,
		      "redis: REDIS_NAMESPACE must match duris:<ENVIRONMENT>:<deployment>; Redis disabled");
		redis_enabled = false;
		return false;
	}
	if (!redis_configure_epoch_surfaces(sql_season_epoch()))
	{
		logit(LOG_SYS, "redis: active SQL season epoch unavailable; Redis disabled");
		redis_enabled = false;
		return false;
	}
	world_recovery_quiesced = false;
	world_writer_token.clear();
	world_writer_lease_msec = 0;
	world_writer_epoch = 0;
	clean_shutdown_sequence = 0;
	world_sequence_floor = 0;
	clean_restart_recovery_boot = 0;
	redis_donation_runtime_set_enabled(false);
	const char *donation_secret = NULL;
	const char *donation_env = getenv("REDIS_DONATION_SUBSCRIBER");
	if (donation_env && strcasecmp(donation_env, "TRUE") == 0)
	{
		const char *secret = getenv("REDIS_DONATION_SECRET");
		if (secret && strlen(secret) >= 32)
		{
			redis_donation_runtime_set_enabled(true);
			donation_secret = secret;
		}
		else
			logit(LOG_SYS,
			      "redis: donation subscriber disabled; REDIS_DONATION_SECRET must be at least 32 bytes");
	}

	if (!redis_runtime_connections_configure(redis_donation_runtime_enabled(),
						 &redis_connections))
	{
		logit(LOG_SYS, "redis: invalid connection security configuration; Redis disabled");
		redis_enabled = false;
		return false;
	}

	redis_ctx = redis_connection_open(redis_connections.world);
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
		if (redis_connections.unix_socket)
			logit(LOG_SYS, "redis connected through configured Unix socket");
		else
			logit(LOG_SYS, "redis connected to %s:%d", redis_connections.host,
			      redis_connections.port);
	}
	redis_shared_connection_observability_record(false, redis_ctx != NULL);

	const redis_presence_worker_config presence_config = {
		redis_connections.presence,
		REDIS_PRESENCE_SESSION_TTL_SECONDS,
		REDIS_PRESENCE_HEARTBEAT_INTERVAL_SECONDS * 1000,
		redis_presence_current_key,
		redis_presence_session_prefix,
		redis_presence_retry_prefix,
		redis_presence_event_channel,
		REDIS_LEGACY_ONLINE
	};
	if (!redis_presence_worker_init(&presence_config))
		logit(LOG_SYS, "redis: presence worker unavailable; presence updates disabled");
	else
		redis_presence_runtime_set_enabled(true);
	if (!redis_report_cache_start(redis_connections.cache))
		logit(LOG_SYS, "redis: cache worker unavailable; report caches disabled");
	if (redis_donation_runtime_enabled())
	{
		const redis_donation_worker_config donation_config = { redis_connections.donation,
								       donation_secret,
								       redis_donation_channel };
		if (!redis_donation_worker_init(&donation_config))
		{
			redis_donation_runtime_set_enabled(false);
			logit(LOG_SYS,
			      "redis: donation worker unavailable; donation subscriber disabled");
		}
	}

	// check for world state persistence
	const char *world_state_env = getenv("REDIS_WORLD_STATE");
	if (world_state_env && strcasecmp(world_state_env, "TRUE") == 0)
	{
		redis_world_state_enabled = true;
		if (!redis_configure_world_authentication())
		{
			logit(LOG_SYS,
			      "redis: world recovery disabled; REDIS_WORLD_STATE_SECRET must be 32-256 bytes");
			redis_world_state_enabled = false;
		}

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

		if (redis_world_state_enabled)
			logit(LOG_SYS, "redis world state enabled: interval=%ds, max_age=%ds",
			      world_state_interval, world_state_max_age);
		const redis_floor_store_config floor_config = { redis_connections.world };
		if (redis_world_state_enabled && !redis_floor_store_init(&floor_config))
		{
			logit(LOG_SYS, "redis: floor worker unavailable; world recovery disabled");
			redis_world_state_enabled = false;
		}
		if (redis_world_state_enabled)
			redis_floor_runtime_set_enabled(true);
		if (redis_world_state_enabled)
		{
			const redis_world_store_config world_config =
				redis_world_store_config_copy();
			clean_shutdown_sequence =
				redis_world_store_consume_clean_shutdown(&world_config);
		}
	}

	// note: flush event scheduled in ne_init_events() after event system is ready

	if (redis_ctx)
	{
		if (redis_world_state_enabled && !redis_world_writer_fence_claim())
		{
			world_recovery_quiesced = true;
			redis_floor_runtime_set_quiesced(true);
			logit(LOG_SYS,
			      "redis: world publisher disabled; writer lease unavailable at boot");
		}
	}
	else if (redis_world_state_enabled)
	{
		world_recovery_quiesced = true;
		redis_floor_runtime_set_quiesced(true);
	}

	return true;
#endif
}

bool redis_clear_pwipe_state(void)
{
	if (!redis_enabled)
		return true;
	if (!redis_runtime_epoch)
		return false;
	redis_donation_worker_shutdown();
	redis_floor_runtime_set_enabled(false);
	redis_floor_runtime_set_quiesced(true);
	redis_presence_runtime_set_enabled(false);
	redis_presence_worker_cancel();
	redis_report_cache_cancel();
	redis_floor_store_cancel();
	if (!redis_world_recovery_quiesce())
		return false;
	const redis_maintenance_config config = {
		redis_connections.maintenance,
		redis_key_namespace,
		redis_runtime_epoch,
		redis_presence_current_key,
		redis_presence_session_pattern,
		redis_presence_retry_pattern,
		redis_report_cache_pattern(),
	};
	return redis_maintenance_clear(&config);
}

bool redis_validate_pwipe_state(void)
{
	if (!redis_enabled)
		return true;
	const redis_maintenance_config config = {
		redis_connections.maintenance,
		redis_key_namespace,
		redis_runtime_epoch,
		redis_presence_current_key,
		redis_presence_session_pattern,
		redis_presence_retry_pattern,
		redis_report_cache_pattern(),
	};
	return redis_maintenance_validate(&config);
}

void redis_cleanup(void)
{
#ifndef __NO_MYSQL__
	bool world_recovery_drained = true;
	redis_donation_worker_shutdown();
	redis_floor_runtime_set_enabled(false);
	redis_floor_runtime_set_quiesced(true);
	redis_presence_runtime_set_enabled(false);
	if (!redis_presence_worker_shutdown(REDIS_PRESENCE_DRAIN_TIMEOUT_MSEC))
		logit(LOG_SYS, "redis: presence worker drain timed out during shutdown");
	if (!redis_report_cache_shutdown(REDIS_CACHE_DRAIN_TIMEOUT_MSEC))
		logit(LOG_SYS, "redis: cache worker drain timed out during shutdown");
	if (redis_world_state_enabled)
	{
		world_recovery_drained = !world_recovery_pipeline_health_copy().initialized ||
					 redis_world_recovery_drain(REDIS_WORLD_DRAIN_TIMEOUT_MSEC);
		if (!world_recovery_drained)
			logit(LOG_SYS, "redis: world recovery drain timed out during shutdown");
		if (world_recovery_pipeline_health_copy().initialized)
		{
			if (world_recovery_drained)
				world_recovery_pipeline_shutdown();
			else
				world_recovery_pipeline_cancel();
		}
	}
	const bool floor_drained = redis_floor_store_shutdown(REDIS_FLOOR_DRAIN_TIMEOUT_MSEC);
	if (!floor_drained)
		logit(LOG_SYS, "redis: floor worker drain timed out during shutdown");
	if (!_pwipe && !world_recovery_quiesced && world_recovery_drained && floor_drained &&
	    redis_world_state_enabled && !world_writer_token.empty())
	{
		const redis_world_store_config config = redis_world_store_config_copy();
		if (!redis_world_store_mark_clean_shutdown(&config, world_writer_token.c_str()))
			logit(LOG_SYS, "redis: clean shutdown recovery marker was not recorded");
	}
	if (!world_writer_token.empty())
	{
		const redis_world_store_config config = redis_world_store_config_copy();
		redis_world_store_release_fence(&config, world_writer_token.c_str());
	}
	world_writer_token.clear();
	world_writer_lease_msec = 0;
	world_writer_epoch = 0;
	clean_shutdown_sequence = 0;
	world_sequence_floor = 0;
	clean_restart_recovery_boot = 0;
	if (redis_ctx)
	{
		redisFree(redis_ctx);
		redis_ctx = NULL;
	}
	redis_runtime_connections_destroy(&redis_connections);
	redis_clear_world_authentication_secrets();
	redis_donation_runtime_set_enabled(false);
	redis_floor_runtime_set_enabled(false);
	redis_floor_runtime_set_quiesced(true);
	redis_world_state_enabled = false;
	redis_enabled = false;
	redis_shared_command_observability_set_enabled(false);
#endif
}

void redis_clear_floor_pickups(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx)
		return;

	redisReply *reply = (redisReply *)redis_command(REDIS_SHARED_SCOPE_FLOOR,
							REDIS_SHARED_COMMAND_WRITE, redis_ctx,
							"DEL mud:floor_pickups");
	if (reply)
		freeReplyObject(reply);
#endif
}

static bool redis_clear_floor_drops_checked(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_ctx)
		return false;
	char floor_key[128];
	char floor_index_key[128];
	const uint64_t epoch = world_writer_epoch ? world_writer_epoch : redis_runtime_epoch;
	if (!redis_epoch_key(floor_key, sizeof floor_key, epoch, REDIS_FLOOR_DROPS_SUFFIX) ||
	    !redis_epoch_key(floor_index_key, sizeof floor_index_key, epoch,
			     REDIS_FLOOR_DROP_INDEX_SUFFIX))
		return false;

	redisReply *reply = (redisReply *)redis_command(REDIS_SHARED_SCOPE_FLOOR,
							REDIS_SHARED_COMMAND_WRITE, redis_ctx,
							"DEL %s %s", floor_key, floor_index_key);
	if (!reply || reply->type != REDIS_REPLY_INTEGER)
	{
		if (reply)
			freeReplyObject(reply);
		return false;
	}
	freeReplyObject(reply);
	reply = (redisReply *)redis_command(REDIS_SHARED_SCOPE_FLOOR, REDIS_SHARED_COMMAND_READ,
					    redis_ctx, "EXISTS %s %s", floor_key, floor_index_key);
	const bool absent = reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 0;
	if (reply)
		freeReplyObject(reply);
	return absent;
#else
	return false;
#endif
}

void redis_clear_floor_drops(void)
{
	redis_clear_floor_drops_checked();
}

static bool redis_read_floor_records(std::vector<std::vector<unsigned char>> *records,
				     size_t maximum_bytes)
{
#ifndef __NO_MYSQL__
	if (!records || !redis_world_state_enabled || !redis_enabled || !redis_ctx)
		return false;
	char floor_key[128];
	char floor_index_key[128];
	if (!redis_season_key(floor_key, sizeof floor_key, REDIS_FLOOR_DROPS_SUFFIX) ||
	    !redis_season_key(floor_index_key, sizeof floor_index_key,
			      REDIS_FLOOR_DROP_INDEX_SUFFIX))
		return false;
	redisReply *index_count_reply =
		(redisReply *)redis_command(REDIS_SHARED_SCOPE_FLOOR, REDIS_SHARED_COMMAND_READ,
					    redis_ctx, "ZCARD %s", floor_index_key);
	redisReply *hash_count_reply = (redisReply *)redis_command(REDIS_SHARED_SCOPE_FLOOR,
								   REDIS_SHARED_COMMAND_READ,
								   redis_ctx, "HLEN %s", floor_key);
	const bool counts_valid =
		index_count_reply && index_count_reply->type == REDIS_REPLY_INTEGER &&
		index_count_reply->integer >= 0 && hash_count_reply &&
		hash_count_reply->type == REDIS_REPLY_INTEGER && hash_count_reply->integer >= 0 &&
		index_count_reply->integer == hash_count_reply->integer &&
		static_cast<uint64_t>(index_count_reply->integer) <=
			WORLD_RECOVERY_MAX_FLOOR_RECORDS;
	const size_t record_count = counts_valid ? static_cast<size_t>(index_count_reply->integer) :
						   0;
	if (index_count_reply)
		freeReplyObject(index_count_reply);
	if (hash_count_reply)
		freeReplyObject(hash_count_reply);
	if (!counts_valid || (record_count && !maximum_bytes) ||
	    maximum_bytes > WORLD_RECOVERY_MAX_FLOOR_BYTES)
		return false;
	try
	{
		records->clear();
		records->reserve(record_count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	constexpr size_t page_size = 64;
	size_t aggregate_size = 0;
	for (size_t first = 0; first < record_count; first += page_size)
	{
		const size_t count = std::min(page_size, record_count - first);
		redisReply *fields = (redisReply *)redis_command(
			REDIS_SHARED_SCOPE_FLOOR, REDIS_SHARED_COMMAND_READ, redis_ctx,
			"ZRANGE %s %zu %zu", floor_index_key, first, first + count - 1);
		if (!fields || fields->type != REDIS_REPLY_ARRAY || fields->elements != count)
		{
			if (fields)
				freeReplyObject(fields);
			records->clear();
			return false;
		}
		std::array<const char *, page_size + 2> arguments = {};
		std::array<size_t, page_size + 2> lengths = {};
		std::array<uint64_t, page_size> field_uids = {};
		arguments[0] = "HMGET";
		lengths[0] = 5;
		arguments[1] = floor_key;
		lengths[1] = strlen(floor_key);
		bool valid = true;
		for (size_t index = 0; index < count; ++index)
		{
			const redisReply *field = fields->element[index];
			char *end = NULL;
			errno = 0;
			field_uids[index] = field && field->type == REDIS_REPLY_STRING &&
							    field->str ?
						    strtoull(field->str, &end, 10) :
						    0;
			valid = valid && field && field->type == REDIS_REPLY_STRING && field->str &&
				!errno && end && !*end && field_uids[index];
			arguments[index + 2] = field ? field->str : nullptr;
			lengths[index + 2] = field ? field->len : 0;
		}
		redisReply *values = nullptr;
		if (valid)
			values = redis_command_argv(REDIS_SHARED_SCOPE_FLOOR,
						    REDIS_SHARED_COMMAND_READ, redis_ctx,
						    static_cast<int>(count + 2), arguments.data(),
						    lengths.data());
		if (!values || values->type != REDIS_REPLY_ARRAY || values->elements != count)
			valid = false;
		for (size_t index = 0; valid && index < count; ++index)
		{
			const redisReply *value = values->element[index];
			uint64_t root_uid = 0;
			valid = value && value->type == REDIS_REPLY_STRING && value->str &&
				value->len > WORLD_RECOVERY_FLOOR_PREFIX_BYTES &&
				value->len <= WORLD_RECOVERY_FLOOR_PREFIX_BYTES +
						      WORLD_RECOVERY_MAX_RECORD_BYTES &&
				world_recovery_floor_object_root_uid(
					reinterpret_cast<const unsigned char *>(value->str),
					value->len, &root_uid) &&
				root_uid == field_uids[index];
			if (!valid)
				break;
			const size_t record_size = value->len - WORLD_RECOVERY_FLOOR_PREFIX_BYTES;
			if (record_size > maximum_bytes - aggregate_size)
			{
				valid = false;
				break;
			}
			std::vector<unsigned char> record;
			try
			{
				record.resize(record_size);
			}
			catch (const std::bad_alloc &)
			{
				valid = false;
				break;
			}
			memcpy(record.data(), value->str + WORLD_RECOVERY_FLOOR_PREFIX_BYTES,
			       record_size);
			aggregate_size += record_size;
			records->push_back(std::move(record));
		}
		if (values)
			freeReplyObject(values);
		freeReplyObject(fields);
		if (!valid)
		{
			records->clear();
			return false;
		}
	}
	index_count_reply = (redisReply *)redis_command(REDIS_SHARED_SCOPE_FLOOR,
							REDIS_SHARED_COMMAND_READ, redis_ctx,
							"ZCARD %s", floor_index_key);
	hash_count_reply = (redisReply *)redis_command(REDIS_SHARED_SCOPE_FLOOR,
						       REDIS_SHARED_COMMAND_READ, redis_ctx,
						       "HLEN %s", floor_key);
	const bool stable = index_count_reply && index_count_reply->type == REDIS_REPLY_INTEGER &&
			    hash_count_reply && hash_count_reply->type == REDIS_REPLY_INTEGER &&
			    index_count_reply->integer == static_cast<long long>(record_count) &&
			    hash_count_reply->integer == static_cast<long long>(record_count);
	if (index_count_reply)
		freeReplyObject(index_count_reply);
	if (hash_count_reply)
		freeReplyObject(hash_count_reply);
	if (!stable)
		records->clear();
	return stable;
#else
	(void)records;
	(void)maximum_bytes;
	return false;
#endif
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
	if (world_recovery_pipeline_busy() || world_floor_barrier_waiting ||
	    world_floor_handoff_active)
		return true;
	if (!redis_flush_floor_drops())
		return false;
	if (!redis_floor_store_request_barrier())
		return false;
	world_floor_barrier_waiting = true;
	return true;
#endif
}

void redis_world_recovery_pulse(void)
{
#ifndef __NO_MYSQL__
	if (!redis_world_state_enabled)
		return;
	if (!world_recovery_pipeline_health_copy().initialized)
		return;
	bool barrier_succeeded = false;
	if (world_floor_barrier_waiting && redis_floor_store_take_barrier(&barrier_succeeded))
	{
		world_floor_barrier_waiting = false;
		if (barrier_succeeded && world_recovery_pipeline_request())
		{
			world_floor_handoff_active = true;
			logit(LOG_SYS, "redis: starting bounded world recovery capture");
		}
		else
		{
			redis_floor_store_resume();
			logit(LOG_SYS, "redis: floor preflight failed; recovery capture deferred");
		}
	}
	world_recovery_pipeline_pulse();
	world_recovery_completion completion = {};
	while (world_recovery_pipeline_take_completion(&completion))
	{
		if (world_floor_handoff_active)
		{
			redis_floor_store_resume();
			world_floor_handoff_active = false;
		}
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
	if (world_floor_handoff_active && !world_recovery_pipeline_busy())
	{
		redis_floor_store_resume();
		world_floor_handoff_active = false;
		logit(LOG_SYS, "redis: recovery capture failed; floor publication resumed");
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
	redis_floor_runtime_set_quiesced(true);
	redis_floor_store_cancel();
	world_floor_barrier_waiting = false;
	world_floor_handoff_active = false;
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
	{
		clean_restart_recovery_boot = 0;
		return false;
	}
	const uint64_t candidate_clean_sequence = clean_shutdown_sequence;
	clean_shutdown_sequence = 0;
	clean_restart_recovery_boot = 0;

	if (!redis_ctx || redis_ctx->err)
	{
		if (!redis_reconnect())
			return false;
	}

	// The atomic current pointer and self-validating framed blob are authoritative.
	// Other metadata keys are operator diagnostics and cannot invalidate a good blob.
	const uint64_t epoch = redis_runtime_epoch;
	char current_key[128];
	if (!redis_epoch_key(current_key, sizeof current_key, epoch, REDIS_WORLD_CURRENT_SUFFIX))
		return false;
	redisReply *reply = (redisReply *)redis_command(REDIS_SHARED_SCOPE_WORLD,
							REDIS_SHARED_COMMAND_READ, redis_ctx,
							"GET %s", current_key);
	if (!reply)
		return false;
	uint64_t sequence = 0;
	if (reply->type == REDIS_REPLY_STRING && reply->str)
		sequence = strtoull(reply->str, NULL, 10);
	freeReplyObject(reply);
	if (!sequence)
	{
		clean_restart_recovery_boot = 0;
		return false;
	}
	std::vector<unsigned char> generation;
	const redis_world_store_config config = redis_world_store_config_copy();
	if (!redis_world_store_read_generation(&config, sequence, &generation))
		return false;
	world_recovery_header header = {};
	const bool exists = world_recovery_validate(generation.data(), generation.size(),
						    world_state_max_age, sequence, &header) &&
			    header.sequence == sequence;
	if (exists)
		world_sequence_floor = std::max(world_sequence_floor, sequence);
	clean_restart_recovery_boot = exists && candidate_clean_sequence == sequence ? 1 : 0;
	return exists;
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
	char clean_shutdown_key[128];
	if (!redis_epoch_key(generation_pattern, sizeof generation_pattern, world_writer_epoch,
			     REDIS_WORLD_GENERATION_PATTERN) ||
	    !redis_epoch_key(current_key, sizeof current_key, world_writer_epoch,
			     REDIS_WORLD_CURRENT_SUFFIX) ||
	    !redis_epoch_key(timestamp_key, sizeof timestamp_key, world_writer_epoch,
			     REDIS_WORLD_TIMESTAMP_SUFFIX) ||
	    !redis_epoch_key(sequence_key, sizeof sequence_key, world_writer_epoch,
			     REDIS_WORLD_SEQUENCE_SUFFIX) ||
	    !redis_epoch_key(checksum_key, sizeof checksum_key, world_writer_epoch,
			     REDIS_WORLD_CHECKSUM_SUFFIX) ||
	    !redis_epoch_key(complete_key, sizeof complete_key, world_writer_epoch,
			     REDIS_WORLD_COMPLETE_SUFFIX) ||
	    !redis_epoch_key(clean_shutdown_key, sizeof clean_shutdown_key, world_writer_epoch,
			     REDIS_WORLD_CLEAN_SHUTDOWN_SUFFIX))
		return false;
	const bool generations_cleared =
		redis_clear_scan_match(REDIS_SHARED_SCOPE_WORLD, generation_pattern);

	redisReply *reply = (redisReply *)redis_command(REDIS_SHARED_SCOPE_WORLD,
							REDIS_SHARED_COMMAND_WRITE, redis_ctx,
							"DEL %s %s %s %s %s %s", current_key,
							timestamp_key, sequence_key, checksum_key,
							complete_key, clean_shutdown_key);
	const bool metadata_cleared = reply && reply->type == REDIS_REPLY_INTEGER;
	if (reply)
		freeReplyObject(reply);
	reply = NULL;
	if (metadata_cleared)
	{
		reply = (redisReply *)redis_command(REDIS_SHARED_SCOPE_WORLD,
						    REDIS_SHARED_COMMAND_READ, redis_ctx,
						    "EXISTS %s %s %s %s %s %s", current_key,
						    timestamp_key, sequence_key, checksum_key,
						    complete_key, clean_shutdown_key);
	}
	const bool metadata_absent = reply && reply->type == REDIS_REPLY_INTEGER &&
				     reply->integer == 0;
	if (reply)
		freeReplyObject(reply);

	if (quiesced && generations_cleared && metadata_cleared && metadata_absent)
		logit(LOG_SYS, "redis: cleared and quiesced world recovery until restart");
	return quiesced && generations_cleared && metadata_cleared && metadata_absent;
#else
	return false;
#endif
}

bool redis_consume_world_state(void)
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (!redis_enabled || !redis_world_state_enabled || !redis_ctx || redis_ctx->err)
		return false;
	const uint64_t epoch = redis_runtime_epoch;
	char current_key[128];
	if (!redis_epoch_key(current_key, sizeof current_key, epoch, REDIS_WORLD_CURRENT_SUFFIX))
		return false;
	redisReply *current = (redisReply *)redis_command(REDIS_SHARED_SCOPE_WORLD,
							  REDIS_SHARED_COMMAND_READ, redis_ctx,
							  "GET %s", current_key);
	if (!current || current->type != REDIS_REPLY_STRING || !current->str)
	{
		if (current)
			freeReplyObject(current);
		return false;
	}
	const uint64_t sequence = strtoull(current->str, NULL, 10);
	freeReplyObject(current);
	const redis_world_store_config config = redis_world_store_config_copy();
	return redis_world_store_consume_generation(&config, world_writer_token.c_str(), sequence);
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

	const uint64_t epoch = redis_runtime_epoch;
	char current_key[128];
	if (!redis_epoch_key(current_key, sizeof current_key, epoch, REDIS_WORLD_CURRENT_SUFFIX))
		return false;
	redisReply *sequence_reply = (redisReply *)redis_command(REDIS_SHARED_SCOPE_WORLD,
								 REDIS_SHARED_COMMAND_READ,
								 redis_ctx, "GET %s", current_key);
	if (!sequence_reply)
		return false;
	uint64_t expected_sequence = 0;
	if (sequence_reply->type == REDIS_REPLY_STRING && sequence_reply->str)
		expected_sequence = strtoull(sequence_reply->str, NULL, 10);
	freeReplyObject(sequence_reply);
	if (!expected_sequence)
		return false;

	std::vector<unsigned char> generation;
	const redis_world_store_config config = redis_world_store_config_copy();
	if (!redis_world_store_read_generation(&config, expected_sequence, &generation))
		return false;

	std::vector<std::vector<unsigned char>> floor_records;
	const size_t floor_budget = generation.size() <= WORLD_RECOVERY_MAX_BYTES ?
					    std::min(WORLD_RECOVERY_MAX_FLOOR_BYTES,
						     WORLD_RECOVERY_MAX_BYTES - generation.size()) :
					    0;
	if (!redis_read_floor_records(&floor_records, floor_budget))
		return false;
	std::vector<const unsigned char *> floor_record_data;
	std::vector<size_t> floor_record_sizes;
	try
	{
		floor_record_data.reserve(floor_records.size());
		floor_record_sizes.reserve(floor_records.size());
		for (const std::vector<unsigned char> &record : floor_records)
		{
			floor_record_data.push_back(record.data());
			floor_record_sizes.push_back(record.size());
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	world_recovery_header header = {};
	const bool result = world_recovery_restore_with_floor(
		generation.data(), generation.size(), world_state_max_age, expected_sequence,
		floor_record_data.data(), floor_record_sizes.data(), floor_records.size(), &header);
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
