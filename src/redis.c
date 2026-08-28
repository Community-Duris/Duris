// Redis subsystem composition and lifecycle.

#include "prototypes.h"
#include "redis_lifecycle.h"
#include "redis_world_runtime.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "redis_command_observability.h"
#include "redis_donation_worker.h"
#include "redis_donation_runtime.h"
#include "redis_floor_runtime.h"
#include "redis_key_registry.h"
#include "redis_maintenance.h"
#include "redis_namespace.h"
#include "redis_presence_runtime.h"
#include "redis_presence_worker.h"
#include "redis_report_cache.h"
#include "redis_runtime_config.h"
#include "sql.h"

extern int _pwipe;

static redis_runtime_connections redis_connections = {};
static bool redis_enabled = false;

#define REDIS_PRESENCE_DRAIN_TIMEOUT_MSEC 1000
#define REDIS_CACHE_DRAIN_TIMEOUT_MSEC 1000
static uint64_t redis_runtime_epoch = 0;
static char redis_key_namespace[64] = {};
static char redis_presence_current_key[160] = {};
static char redis_presence_session_prefix[160] = {};
static char redis_presence_session_pattern[160] = {};
static char redis_presence_retry_prefix[160] = {};
static char redis_presence_retry_pattern[160] = {};
static char redis_presence_event_channel[160] = {};
static char redis_donation_channel[160] = {};

bool redis_runtime_enabled(void)
{
	return redis_enabled;
}

static bool redis_epoch_key(char *buffer, size_t size, uint64_t epoch, const char *suffix)
{
	return redis_namespace_season_key(redis_key_namespace, epoch, suffix, buffer, size);
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

static bool redis_configure_namespace(void)
{
	return redis_namespace_validate(getenv("REDIS_NAMESPACE"), getenv("ENVIRONMENT"),
					redis_key_namespace, sizeof redis_key_namespace);
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
		redis_runtime_epoch = 0;
		redis_key_namespace[0] = '\0';
		return true;
	}
	redis_enabled = true;
	redis_shared_command_observability_set_enabled(true);
	redis_runtime_epoch = 0;
	redis_key_namespace[0] = '\0';
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

	const redis_world_runtime_config world_config = {
		redis_connections.world, redis_key_namespace,	 redis_runtime_epoch,
		redis_connections.host,	 redis_connections.port, redis_connections.unix_socket,
	};
	if (!redis_world_runtime_start(&world_config))
		logit(LOG_SYS, "redis: world runtime configuration failed; recovery disabled");

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
	redis_presence_runtime_set_enabled(false);
	redis_presence_worker_cancel();
	redis_report_cache_cancel();
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
	redis_donation_worker_shutdown();
	redis_presence_runtime_set_enabled(false);
	if (!redis_presence_worker_shutdown(REDIS_PRESENCE_DRAIN_TIMEOUT_MSEC))
		logit(LOG_SYS, "redis: presence worker drain timed out during shutdown");
	if (!redis_report_cache_shutdown(REDIS_CACHE_DRAIN_TIMEOUT_MSEC))
		logit(LOG_SYS, "redis: cache worker drain timed out during shutdown");
	redis_world_runtime_shutdown(_pwipe != 0);
	redis_runtime_connections_destroy(&redis_connections);
	redis_donation_runtime_set_enabled(false);
	redis_enabled = false;
	redis_shared_command_observability_set_enabled(false);
#endif
}
