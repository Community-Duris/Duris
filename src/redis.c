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
#include "presence_policy.h"
#include "redis_cache_store.h"
#include "redis_command_observability.h"
#include "redis_connection.h"
#include "redis_donation_worker.h"
#include "redis_floor_store.h"
#include "redis_key_registry.h"
#include "redis_namespace.h"
#include "redis_presence_payload.h"
#include "redis_presence_worker.h"
#include "report_cache_codec.h"
#include "redis_world_store.h"
#include "world_recovery_codec.h"
#include "spells.h"
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
static redis_connection_settings *redis_settings = NULL;
static redis_connection_settings *redis_presence_settings = NULL;
static redis_connection_settings *redis_cache_settings = NULL;
static redis_connection_settings *redis_donation_settings = NULL;
static redis_connection_settings *redis_maintenance_settings = NULL;
bool redis_enabled = false;
bool redis_donation_enabled = false;
bool redis_world_state_enabled = false;
int crash_recovery_boot = 0;
int clean_restart_recovery_boot = 0;

#define REDIS_WORLD_STATE_INTERVAL_DEFAULT 10
#define REDIS_WORLD_STATE_MAX_AGE_DEFAULT 300
#define REDIS_CONNECT_TIMEOUT_MSEC 250
#define REDIS_COMMAND_TIMEOUT_MSEC 100
#define REDIS_WORLD_DRAIN_TIMEOUT_MSEC 30000
#define REDIS_DONATION_MAX_MESSAGES_PER_PULSE 8
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
static bool world_recovery_materialization_active = false;
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
static char redis_cache_prefix[160] = {};
static char redis_cache_pattern[160] = {};
static char redis_cache_named_key[160] = {};
static char redis_cache_fraglist_key[160] = {};
static char redis_cache_epic_zones_key[160] = {};
static char redis_cache_artifact_keys[6][160] = {};
static bool redis_clear_floor_drops_checked(void);
static void redis_prime_artifact_caches(void);

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
	config.connection = redis_settings;
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
	    !redis_epoch_key(redis_cache_prefix, sizeof redis_cache_prefix, epoch, "cache:") ||
	    !redis_epoch_key(redis_cache_pattern, sizeof redis_cache_pattern, epoch,
			     REDIS_CACHE_PATTERN) ||
	    !redis_epoch_key(redis_cache_named_key, sizeof redis_cache_named_key, epoch,
			     REDIS_CACHE_NAMED) ||
	    !redis_epoch_key(redis_cache_fraglist_key, sizeof redis_cache_fraglist_key, epoch,
			     REDIS_CACHE_FRAGLIST) ||
	    !redis_epoch_key(redis_cache_epic_zones_key, sizeof redis_cache_epic_zones_key, epoch,
			     REDIS_CACHE_EPIC_ZONES))
		return false;
	for (int type = 1; type <= 3; ++type)
		for (int view = 0; view <= 1; ++view)
		{
			char suffix[64];
			const int written = snprintf(suffix, sizeof suffix,
						     REDIS_CACHE_ARTIFACT_FORMAT, type, view);
			const size_t index = static_cast<size_t>((type - 1) * 2 + view);
			if (written <= 0 || (size_t)written >= sizeof suffix ||
			    !redis_epoch_key(redis_cache_artifact_keys[index],
					     sizeof redis_cache_artifact_keys[index], epoch,
					     suffix))
				return false;
		}
	redis_runtime_epoch = epoch;
	return true;
}

static const char *redis_resolve_cache_key(const char *key)
{
	if (!key)
		return NULL;
	if (!strcmp(key, REDIS_CACHE_NAMED))
		return redis_cache_named_key;
	if (!strcmp(key, REDIS_CACHE_FRAGLIST))
		return redis_cache_fraglist_key;
	if (!strcmp(key, REDIS_CACHE_EPIC_ZONES))
		return redis_cache_epic_zones_key;
	const size_t prefix_size = strlen(redis_cache_prefix);
	return prefix_size && !strncmp(key, redis_cache_prefix, prefix_size) && key[prefix_size] ?
		       key :
		       NULL;
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

static void redis_prime_artifact_caches(void)
{
	redisContext *context = redis_connection_open(redis_cache_settings);
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
		keys[index] = redis_cache_artifact_keys[index];
	redisReply *reply = (redisReply *)redis_command(REDIS_SHARED_SCOPE_CACHE,
							REDIS_SHARED_COMMAND_SCRIPT, context,
							"EVAL %b 6 %s %s %s %s %s %s", script,
							strlen(script), keys[0], keys[1], keys[2],
							keys[3], keys[4], keys[5]);
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
		redis_cache_store_seed(keys[index], value->str, (int)seconds);
	}
	freeReplyObject(reply);
	redisFree(context);
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

static bool redis_delete_key_checked(redis_shared_command_scope scope, const char *key)
{
#ifndef __NO_MYSQL__
	if (!key || !*key || !redis_enabled || !redis_ctx)
		return false;
	redisReply *reply = (redisReply *)redis_command(scope, REDIS_SHARED_COMMAND_WRITE,
							redis_ctx, "DEL %s", key);
	const bool deleted = reply && reply->type == REDIS_REPLY_INTEGER;
	if (reply)
		freeReplyObject(reply);
	if (!deleted)
		return false;
	reply = (redisReply *)redis_command(scope, REDIS_SHARED_COMMAND_READ, redis_ctx,
					    "EXISTS %s", key);
	const bool absent = reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 0;
	if (reply)
		freeReplyObject(reply);
	return absent;
#else
	(void)scope;
	(void)key;
	return true;
#endif
}

static bool redis_parse_number(const char *value, int minimum, int maximum, int fallback,
			       int *result)
{
	if (!result)
		return false;
	if (!value || !*value)
	{
		*result = fallback;
		return true;
	}
	errno = 0;
	char *end = NULL;
	const long parsed = strtol(value, &end, 10);
	if (errno || !end || *end || parsed < minimum || parsed > maximum)
		return false;
	*result = (int)parsed;
	return true;
}

static bool redis_host_is_loopback(const char *host)
{
	return host && (!strcasecmp(host, "localhost") || !strcmp(host, "127.0.0.1") ||
			!strcmp(host, "::1"));
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

struct redis_identity
{
	const char *username;
	const char *password;
};

static bool redis_resolve_identity(const char *username_name, const char *password_name,
				   bool production, redis_identity *identity)
{
	if (!username_name || !password_name || !identity)
		return false;
	const char *username = getenv(username_name);
	const char *password = getenv(password_name);
	const bool scoped = (username && *username) || (password && *password);
	if (scoped)
	{
		if (!username || !*username || !password || !*password)
			return false;
	}
	else
	{
		if (production)
			return false;
		username = getenv("REDIS_USERNAME");
		password = getenv("REDIS_PASSWORD");
	}
	identity->username = username;
	identity->password = password;
	return true;
}

static void redis_destroy_connection_settings(void)
{
	redis_connection_settings_destroy(redis_settings);
	redis_connection_settings_destroy(redis_presence_settings);
	redis_connection_settings_destroy(redis_cache_settings);
	redis_connection_settings_destroy(redis_donation_settings);
	redis_connection_settings_destroy(redis_maintenance_settings);
	redis_settings = NULL;
	redis_presence_settings = NULL;
	redis_cache_settings = NULL;
	redis_donation_settings = NULL;
	redis_maintenance_settings = NULL;
}

static bool redis_configure_connections(const char *host, int port, const char *unix_socket)
{
	int database = 0;
	if (!redis_parse_number(getenv("REDIS_DB"), 0, 255, 0, &database))
		return false;
	const char *tls_value = getenv("REDIS_TLS");
	const bool tls = tls_value && !strcasecmp(tls_value, "TRUE");
	if (tls_value && *tls_value && strcasecmp(tls_value, "TRUE") &&
	    strcasecmp(tls_value, "FALSE"))
		return false;
	const char *environment = getenv("ENVIRONMENT");
	const bool production = environment && !strcasecmp(environment, "production");
	const bool require_tls = (!unix_socket || !*unix_socket) && environment &&
				 !strcasecmp(environment, "production") &&
				 !redis_host_is_loopback(host);
	redis_identity world = {};
	redis_identity presence = {};
	redis_identity cache = {};
	redis_identity donation = {};
	redis_identity maintenance = {};
	if (!redis_resolve_identity("REDIS_WORLD_USERNAME", "REDIS_WORLD_PASSWORD", production,
				    &world) ||
	    !redis_resolve_identity("REDIS_PRESENCE_USERNAME", "REDIS_PRESENCE_PASSWORD",
				    production, &presence) ||
	    !redis_resolve_identity("REDIS_CACHE_USERNAME", "REDIS_CACHE_PASSWORD", production,
				    &cache) ||
	    !redis_resolve_identity("REDIS_MAINTENANCE_USERNAME", "REDIS_MAINTENANCE_PASSWORD",
				    production, &maintenance) ||
	    (redis_donation_enabled &&
	     !redis_resolve_identity("REDIS_DONATION_USERNAME", "REDIS_DONATION_PASSWORD",
				     production, &donation)))
		return false;
	if (production)
	{
		const char *usernames[5] = { world.username, presence.username, cache.username,
					     maintenance.username,
					     redis_donation_enabled ? donation.username : NULL };
		const size_t count = redis_donation_enabled ? 5 : 4;
		for (size_t left = 0; left < count; ++left)
			for (size_t right = left + 1; right < count; ++right)
				if (!usernames[left] || !*usernames[left] || !usernames[right] ||
				    !*usernames[right] ||
				    !strcmp(usernames[left], usernames[right]))
					return false;
	}
	redis_connection_options options = {
		host,
		port,
		REDIS_CONNECT_TIMEOUT_MSEC,
		REDIS_COMMAND_TIMEOUT_MSEC,
		database,
		world.username,
		world.password,
		tls,
		getenv("REDIS_CA_CERT"),
		getenv("REDIS_TLS_SERVER_NAME"),
		require_tls,
		unix_socket,
	};
	redis_connection_settings *world_settings = redis_connection_settings_create(&options);
	options.username = presence.username;
	options.password = presence.password;
	redis_connection_settings *presence_settings = redis_connection_settings_create(&options);
	options.username = cache.username;
	options.password = cache.password;
	redis_connection_settings *cache_settings = redis_connection_settings_create(&options);
	redis_connection_settings *donation_settings = NULL;
	if (redis_donation_enabled)
	{
		options.username = donation.username;
		options.password = donation.password;
		donation_settings = redis_connection_settings_create(&options);
	}
	options.username = maintenance.username;
	options.password = maintenance.password;
	redis_connection_settings *maintenance_settings =
		redis_connection_settings_create(&options);
	if (!world_settings || !presence_settings || !cache_settings ||
	    (redis_donation_enabled && !donation_settings) || !maintenance_settings)
	{
		redis_connection_settings_destroy(world_settings);
		redis_connection_settings_destroy(presence_settings);
		redis_connection_settings_destroy(cache_settings);
		redis_connection_settings_destroy(donation_settings);
		redis_connection_settings_destroy(maintenance_settings);
		return false;
	}
	redis_destroy_connection_settings();
	redis_settings = world_settings;
	redis_presence_settings = presence_settings;
	redis_cache_settings = cache_settings;
	redis_donation_settings = donation_settings;
	redis_maintenance_settings = maintenance_settings;
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

	redis_ctx = redis_connection_open(redis_settings);
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
	world_recovery_materialization_active = false;
	clean_shutdown_sequence = 0;
	world_sequence_floor = 0;
	clean_restart_recovery_boot = 0;
	redis_donation_enabled = false;
	const char *donation_secret = NULL;
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

	const char *redis_socket = getenv("REDIS_SOCKET");
	const bool use_socket = redis_socket && *redis_socket;
	const char *configured_host = getenv("REDIS_HOST");
	const char *configured_port = getenv("REDIS_PORT");
	if (use_socket &&
	    ((configured_host && *configured_host) || (configured_port && *configured_port)))
	{
		logit(LOG_SYS,
		      "redis: REDIS_SOCKET is mutually exclusive with REDIS_HOST and REDIS_PORT; Redis disabled");
		redis_enabled = false;
		return false;
	}
	const char *redis_host = use_socket ? NULL : configured_host;
	if (!use_socket && (!redis_host || !*redis_host))
		redis_host = "127.0.0.1";

	int redis_port = 0;
	if (!use_socket && !redis_parse_number(configured_port, 1, 65535, 6379, &redis_port))
	{
		logit(LOG_SYS, "redis: invalid REDIS_PORT; Redis disabled");
		redis_enabled = false;
		return false;
	}
	if (!redis_configure_connections(redis_host, redis_port, use_socket ? redis_socket : NULL))
	{
		logit(LOG_SYS, "redis: invalid connection security configuration; Redis disabled");
		redis_enabled = false;
		return false;
	}

	redis_ctx = redis_connection_open(redis_settings);
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
		if (use_socket)
			logit(LOG_SYS, "redis connected through configured Unix socket");
		else
			logit(LOG_SYS, "redis connected to %s:%d", redis_host, redis_port);
	}
	redis_shared_connection_observability_record(false, redis_ctx != NULL);

	const redis_presence_worker_config presence_config = {
		redis_presence_settings,
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
	const redis_cache_store_config cache_config = { redis_cache_settings };
	if (!redis_cache_store_init(&cache_config))
		logit(LOG_SYS, "redis: cache worker unavailable; report caches disabled");
	else
		redis_prime_artifact_caches();
	if (redis_donation_enabled)
	{
		const redis_donation_worker_config donation_config = { redis_donation_settings,
								       donation_secret,
								       redis_donation_channel };
		if (!redis_donation_worker_init(&donation_config))
		{
			redis_donation_enabled = false;
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
		const redis_floor_store_config floor_config = { redis_settings };
		if (redis_world_state_enabled && !redis_floor_store_init(&floor_config))
		{
			logit(LOG_SYS, "redis: floor worker unavailable; world recovery disabled");
			redis_world_state_enabled = false;
		}
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
	if (!redis_runtime_epoch)
		return false;
	redis_donation_worker_shutdown();
	redis_presence_worker_cancel();
	redis_cache_store_cancel();
	redis_floor_store_cancel();
	if (!redis_world_recovery_quiesce())
		return false;
	redisContext *maintenance = redis_connection_open(redis_maintenance_settings);
	if (!maintenance || maintenance->err)
	{
		redis_shared_command_observability_record(REDIS_SHARED_SCOPE_MAINTENANCE,
							  REDIS_SHARED_COMMAND_WRITE,
							  redis_command_outcome(maintenance, false),
							  0);
		if (maintenance)
			redisFree(maintenance);
		return false;
	}
	redisContext *world_context = redis_ctx;
	redis_ctx = maintenance;
	const bool cleared =
		redis_clear_world_state() && redis_clear_floor_drops_checked() &&
		redis_delete_key_checked(REDIS_SHARED_SCOPE_MAINTENANCE,
					 REDIS_LEGACY_FLOOR_DROPS) &&
		redis_delete_key_checked(REDIS_SHARED_SCOPE_MAINTENANCE,
					 REDIS_LEGACY_FLOOR_PICKUPS) &&
		redis_delete_key_checked(REDIS_SHARED_SCOPE_MAINTENANCE, REDIS_LEGACY_ONLINE) &&
		redis_delete_key_checked(REDIS_SHARED_SCOPE_MAINTENANCE,
					 redis_presence_current_key) &&
		redis_clear_scan_match(REDIS_SHARED_SCOPE_MAINTENANCE,
				       redis_presence_session_pattern) &&
		redis_clear_scan_match(REDIS_SHARED_SCOPE_MAINTENANCE,
				       redis_presence_retry_pattern) &&
		redis_delete_key_checked(REDIS_SHARED_SCOPE_MAINTENANCE,
					 REDIS_LEGACY_PRESENCE_CURRENT) &&
		redis_clear_scan_match(REDIS_SHARED_SCOPE_MAINTENANCE,
				       REDIS_LEGACY_PRESENCE_SESSION_PATTERN) &&
		redis_clear_scan_match(REDIS_SHARED_SCOPE_MAINTENANCE,
				       REDIS_LEGACY_PRESENCE_RETRY_PATTERN) &&
		redis_clear_scan_match(REDIS_SHARED_SCOPE_MAINTENANCE,
				       REDIS_LEGACY_WORLD_GENERATION_PATTERN) &&
		redis_delete_key_checked(REDIS_SHARED_SCOPE_MAINTENANCE,
					 REDIS_LEGACY_WORLD_CURRENT) &&
		redis_delete_key_checked(REDIS_SHARED_SCOPE_MAINTENANCE,
					 REDIS_LEGACY_WORLD_TIMESTAMP) &&
		redis_delete_key_checked(REDIS_SHARED_SCOPE_MAINTENANCE,
					 REDIS_LEGACY_WORLD_SEQUENCE) &&
		redis_delete_key_checked(REDIS_SHARED_SCOPE_MAINTENANCE,
					 REDIS_LEGACY_WORLD_CHECKSUM) &&
		redis_delete_key_checked(REDIS_SHARED_SCOPE_MAINTENANCE,
					 REDIS_LEGACY_WORLD_COMPLETE) &&
		redis_delete_key_checked(REDIS_SHARED_SCOPE_MAINTENANCE,
					 REDIS_LEGACY_WORLD_FENCE) &&
		redis_clear_scan_match(REDIS_SHARED_SCOPE_MAINTENANCE, redis_cache_pattern) &&
		redis_clear_scan_match(REDIS_SHARED_SCOPE_MAINTENANCE,
				       REDIS_LEGACY_CACHE_PATTERN) &&
		redis_clear_ship_snapshots();
	redis_ctx = world_context;
	redisFree(maintenance);
	return cleared;
}

bool redis_validate_pwipe_state(void)
{
#ifdef __NO_MYSQL__
	return true;
#else
	if (!redis_enabled)
		return true;
	redisContext *context = redis_connection_open(redis_maintenance_settings);
	redisReply *reply = (redisReply *)redis_command(REDIS_SHARED_SCOPE_MAINTENANCE,
							REDIS_SHARED_COMMAND_READ, context, "PING");
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
	bool world_recovery_drained = true;
	redis_donation_worker_shutdown();
	if (!redis_presence_worker_shutdown(REDIS_PRESENCE_DRAIN_TIMEOUT_MSEC))
		logit(LOG_SYS, "redis: presence worker drain timed out during shutdown");
	if (!redis_cache_store_shutdown(REDIS_CACHE_DRAIN_TIMEOUT_MSEC))
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
	world_recovery_materialization_active = false;
	clean_shutdown_sequence = 0;
	world_sequence_floor = 0;
	clean_restart_recovery_boot = 0;
	if (redis_ctx)
	{
		redisFree(redis_ctx);
		redis_ctx = NULL;
	}
	redis_destroy_connection_settings();
	redis_clear_world_authentication_secrets();
	redis_donation_enabled = false;
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

#define MAX_FLOOR_DROP_BATCH 1024
constexpr size_t FLOOR_DROP_RECORD_MAX_BYTES =
	sizeof(world_recovery_object_record) +
	WORLD_RECOVERY_MAX_ITEM_TREE * sizeof(world_recovery_item_snapshot);

static struct
{
	unsigned long uid;
	size_t record_size;
	unsigned char record[FLOOR_DROP_RECORD_MAX_BYTES];
} floor_drop_batch[MAX_FLOOR_DROP_BATCH];

static int floor_drop_batch_count = 0;
static unsigned long floor_drop_removes[MAX_FLOOR_DROP_BATCH];
static int floor_drop_remove_count = 0;

void redis_log_floor_drop(P_obj obj, int room_vnum)
{
	if (_pwipe)
		return;
#ifndef __NO_MYSQL__
	if (!redis_world_state_enabled || world_recovery_quiesced || !obj || obj->obj_uid == 0)
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

	const int index = floor_drop_batch_count;
	const int size = world_recovery_write_object_to_buffer(
		obj, room_vnum, reinterpret_cast<char *>(floor_drop_batch[index].record),
		sizeof(floor_drop_batch[index].record));
	if (size <= 0)
		return;
	floor_drop_batch[index].uid = obj->obj_uid;
	floor_drop_batch[index].record_size = static_cast<size_t>(size);
	++floor_drop_batch_count;
#endif
}

bool redis_flush_floor_drops(void)
{
#ifndef __NO_MYSQL__
	if (!redis_world_state_enabled || world_recovery_quiesced)
		return true;
	if (!redis_enabled || !redis_floor_store_health_copy().initialized)
		return false;

	if (floor_drop_batch_count == 0 && floor_drop_remove_count == 0)
		return true;
	char floor_key[128];
	char floor_index_key[128];
	if (!redis_season_key(floor_key, sizeof floor_key, REDIS_FLOOR_DROPS_SUFFIX) ||
	    !redis_season_key(floor_index_key, sizeof floor_index_key,
			      REDIS_FLOOR_DROP_INDEX_SUFFIX))
		return false;

	std::vector<redis_floor_mutation> mutations;
	try
	{
		mutations.reserve(floor_drop_remove_count + floor_drop_batch_count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	// Preserve remove-before-replacement ordering within one immutable worker batch.
	for (int i = 0; i < floor_drop_remove_count; i++)
		mutations.push_back({ floor_drop_removes[i], NULL, 0, true });

	for (int i = 0; i < floor_drop_batch_count; i++)
		mutations.push_back({ floor_drop_batch[i].uid, floor_drop_batch[i].record,
				      floor_drop_batch[i].record_size, false, true });

	if (!redis_floor_store_submit(floor_key, floor_index_key, mutations.data(),
				      mutations.size()))
		return false;

	floor_drop_remove_count = 0;
	floor_drop_batch_count = 0;
	return true;
#else
	return false;
#endif
}

void redis_remove_floor_drop(unsigned long obj_uid)
{
#ifndef __NO_MYSQL__
	if (!redis_world_state_enabled || world_recovery_quiesced ||
	    world_recovery_materialization_active || obj_uid == 0)
		return;

	// check if it's in the pending batch - remove from there first
	for (int i = 0; i < floor_drop_batch_count; i++)
	{
		if (floor_drop_batch[i].uid == obj_uid)
		{
			--floor_drop_batch_count;
			if (i != floor_drop_batch_count)
				floor_drop_batch[i] = floor_drop_batch[floor_drop_batch_count];
			return;
		}
	}

	// not in batch, queue for removal from redis
	if (floor_drop_remove_count < MAX_FLOOR_DROP_BATCH)
		floor_drop_removes[floor_drop_remove_count++] = obj_uid;
#endif
}

void redis_world_recovery_set_materializing(bool active)
{
	world_recovery_materialization_active = active;
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

bool redis_cache_set(const char *key, const char *value)
{
#ifdef __NO_MYSQL__
	return false;
#else
	const char *resolved = redis_resolve_cache_key(key);
	if (!redis_enabled || !resolved || !value)
		return false;
	return redis_cache_store_set(resolved, value, 0);
#endif
}

bool redis_cache_set_ex(const char *key, int seconds, const char *value)
{
#ifdef __NO_MYSQL__
	return false;
#else
	const char *resolved = redis_resolve_cache_key(key);
	if (!redis_enabled || !resolved || !value || seconds <= 0)
		return false;
	return redis_cache_store_set(resolved, value, seconds);
#endif
}

char *redis_cache_get(const char *key)
{
#ifdef __NO_MYSQL__
	return NULL;
#else
	const char *resolved = redis_resolve_cache_key(key);
	if (!redis_enabled || !resolved)
		return NULL;
	return redis_cache_store_get(resolved);
#endif
}

bool redis_cache_del(const char *key)
{
#ifndef __NO_MYSQL__
	const char *resolved = redis_resolve_cache_key(key);
	if (!redis_enabled || !resolved)
		return false;
	return redis_cache_store_delete(resolved);
#else
	return false;
#endif
}

#ifndef __NO_MYSQL__
static void redis_ship_cache_key(char *buf, size_t buf_size, const char *owner_name)
{
	snprintf(buf, buf_size, REDIS_SHIP_SNAPSHOT_FORMAT, owner_name ? owner_name : "");
}

void redis_invalidate_ship_snapshot(const char *owner_name)
{
	if (!owner_name)
		return;

	char key[256];
	redis_ship_cache_key(key, sizeof(key), owner_name);
	redis_cache_store_delete(key);
}

bool redis_clear_ship_snapshots(void)
{
	if (!redis_enabled)
		return true;
	if (!redis_ctx)
		return false;

	char cursor[64] = "0";
	do
	{
		redisReply *scan = (redisReply *)redis_command(REDIS_SHARED_SCOPE_MAINTENANCE,
							       REDIS_SHARED_COMMAND_SCAN, redis_ctx,
							       "SCAN %s MATCH %s COUNT 256", cursor,
							       REDIS_SHIP_SNAPSHOT_PATTERN);
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
			redisReply *del = (redisReply *)redis_command(
				REDIS_SHARED_SCOPE_MAINTENANCE, REDIS_SHARED_COMMAND_WRITE,
				redis_ctx, "DEL %b", key->str, key->len);
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

	return redis_scan_match_empty(REDIS_SHARED_SCOPE_MAINTENANCE, REDIS_SHIP_SNAPSHOT_PATTERN);
}
#endif

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

void redis_check_donation_messages(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || !redis_donation_enabled)
		return;
	for (int handled = 0; handled < REDIS_DONATION_MAX_MESSAGES_PER_PULSE; ++handled)
	{
		donation_event event = {};
		if (!redis_donation_worker_take(&event))
			break;
		broadcast_donation_nchat(&event);
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
	constexpr int named_report_cache_ttl_seconds = 86400;
	if (!redis_enabled)
		return;

	char *report = generate_named_report();
	if (report)
	{
		redis_cache_set_ex(REDIS_CACHE_NAMED, named_report_cache_ttl_seconds, report);
		free(report);
		logit(LOG_SYS, "redis: cached named report");
	}
#endif
}

char *redis_get_named_report(void)
{
	return redis_cache_get(REDIS_CACHE_NAMED);
}

// fraglist cache
extern void get_level_cap_info(long *max_frags, int *racewar, int *level, time_t *next_update);
extern const racewar_struct racewar_color[];

#define MAX_FRAG_SIZE 10
static constexpr int fraglist_cache_ttl_seconds = 900;

static char *generate_fraglist_cache_payload(void)
{
#ifdef __NO_MYSQL__
	return NULL;
#else
	char *output = (char *)malloc(65536);
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
	// Avoid a redundant lookup: both displayed caps derive from the same singleton row.
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
		cap_others, (int)(cap_frags / 100), (int)(cap_frags % 100));
	snprintf(output, 65536, " &+YFrags needed:&+w %.2f&n\n\n&+WTop Fraggers\n\n",
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
	char *payload = report_cache_countdown_encode(prefix, output,
						      static_cast<uint64_t>(time(NULL)),
						      static_cast<uint64_t>(cap_deadline));
	free(output);
	return payload;
#endif
}

void redis_cache_fraglist(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled)
		return;

	char *output = generate_fraglist_cache_payload();
	if (output)
	{
		redis_cache_set_ex(REDIS_CACHE_FRAGLIST, fraglist_cache_ttl_seconds, output);
		free(output);
		logit(LOG_SYS, "redis: cached fraglist");
	}
#endif
}

char *redis_get_fraglist(void)
{
	const auto render = [](const char *payload, void *) -> char *
	{
		return report_cache_countdown_render(payload, static_cast<uint64_t>(time(NULL)),
						     fraglist_cache_ttl_seconds);
	};
	return redis_cache_store_transform(redis_cache_fraglist_key, render, NULL);
}

bool redis_invalidate_fraglist(void)
{
	return redis_cache_del(REDIS_CACHE_FRAGLIST);
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
		redis_cache_set_ex(REDIS_CACHE_EPIC_ZONES, EPIC_ZONES_CACHE_TTL, output);
		free(output);
		logit(LOG_SYS, "redis: cached epic zones");
	}
#endif
}

char *redis_get_epic_zones(void)
{
	return redis_cache_get(REDIS_CACHE_EPIC_ZONES);
}

bool redis_invalidate_epic_zones(void)
{
	return redis_cache_del(REDIS_CACHE_EPIC_ZONES);
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
	if (type < 1 || type > 3)
		return NULL;
	return redis_cache_artifact_keys[(type - 1) * 2 + (godlist ? 1 : 0)];
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

bool redis_invalidate_artifact_list(int type, bool godlist)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled || type < 1 || type > 3)
		return false;
	return redis_cache_del(get_artifact_cache_key(type, godlist));
#else
	return false;
#endif
}

bool redis_invalidate_artifact_cache(void)
{
#ifndef __NO_MYSQL__
	if (!redis_enabled)
		return false;

	bool submitted = true;
	for (int t = 1; t <= 3; t++)
	{
		submitted = redis_cache_del(get_artifact_cache_key(t, false)) && submitted;
		submitted = redis_cache_del(get_artifact_cache_key(t, true)) && submitted;
	}
	return submitted;
#else
	return false;
#endif
}
