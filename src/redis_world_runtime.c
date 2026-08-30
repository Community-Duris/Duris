#include "redis_world_runtime.h"

#include "prototypes.h"
#include "redis_command_observability.h"
#include "redis_connection.h"
#include "redis_floor_runtime.h"
#include "redis_floor_store.h"
#include "redis_key_registry.h"
#include "redis_namespace.h"
#include "redis_world_store.h"
#include "world_recovery_codec.h"
#include "world_recovery_pipeline.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cerrno>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <strings.h>
#include <thread>
#include <vector>

#ifndef __NO_REDIS__
#include <hiredis/hiredis.h>
#include <openssl/rand.h>
#endif

namespace
{
constexpr int WORLD_STATE_INTERVAL_DEFAULT = 10;
constexpr int WORLD_STATE_MAX_AGE_DEFAULT = 300;
constexpr uint64_t WORLD_DRAIN_TIMEOUT_MSEC = 30000;
constexpr uint64_t FLOOR_DRAIN_TIMEOUT_MSEC = 1000;

redisContext *world_context = NULL;
const redis_connection_settings *world_connection = NULL;
bool world_enabled = false;
bool recovery_boot = false;
bool clean_restart_boot = false;
int world_state_interval = WORLD_STATE_INTERVAL_DEFAULT;
int world_state_max_age = WORLD_STATE_MAX_AGE_DEFAULT;
bool world_recovery_quiesced = false;
std::string world_writer_token;
std::string world_authentication_secret;
std::string world_previous_authentication_secret;
std::string world_key_namespace;
uint64_t world_writer_lease_msec = 0;
uint64_t world_writer_epoch = 0;
uint64_t world_runtime_epoch = 0;
bool world_floor_barrier_waiting = false;
bool world_floor_handoff_active = false;
uint64_t clean_shutdown_sequence = 0;
uint64_t world_sequence_floor = 0;

#ifndef __NO_REDIS__
void redis_clear_world_authentication_secrets()
{
	std::fill(world_authentication_secret.begin(), world_authentication_secret.end(), '\0');
	std::fill(world_previous_authentication_secret.begin(),
		  world_previous_authentication_secret.end(), '\0');
	world_authentication_secret.clear();
	world_previous_authentication_secret.clear();
}

bool redis_configure_world_authentication()
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
		world_authentication_secret.assign(current, current_size);
		if (previous_size)
			world_previous_authentication_secret.assign(previous, previous_size);
	}
	catch (const std::bad_alloc &)
	{
		redis_clear_world_authentication_secrets();
		return false;
	}
	return true;
}

bool redis_epoch_key(char *buffer, size_t size, uint64_t epoch, const char *suffix)
{
	return redis_namespace_season_key(world_key_namespace.c_str(), epoch, suffix, buffer, size);
}

redis_world_store_config redis_world_store_config_copy()
{
	redis_world_store_config config = {};
	config.connection = world_connection;
	config.key_namespace = world_key_namespace.c_str();
	config.authentication_secret = world_authentication_secret.c_str();
	config.previous_authentication_secret =
		world_previous_authentication_secret.empty() ?
			NULL :
			world_previous_authentication_secret.c_str();
	config.season_epoch = world_writer_epoch ? world_writer_epoch : world_runtime_epoch;
	config.generation_ttl_seconds = std::max<uint64_t>(3600, world_state_max_age * 4);
	return config;
}

void redis_log_command_failure(const char *outcome)
{
	static time_t last_log = 0;
	const time_t now = time(NULL);
	if (now == last_log)
		return;
	last_log = now;
	logit(LOG_DEBUG, "redis redis_command failed: outcome=%s", outcome);
}

redisReply *redis_command_finish(redis_shared_command_scope scope, redis_shared_command_kind kind,
				 uint64_t started_usec, redisReply *reply)
{
	const uint64_t finished_usec = redis_observability_now_usec();
	const uint64_t duration_usec =
		finished_usec >= started_usec ? finished_usec - started_usec : 0;
	redis_shared_command_outcome outcome = REDIS_SHARED_OUTCOME_SUCCESS;
	const char *label = NULL;
	if (!world_context || world_context->err)
	{
		outcome = redis_command_outcome(world_context, false);
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

redisReply *redis_command(redis_shared_command_scope scope, redis_shared_command_kind kind,
			  const char *format, ...)
{
	const uint64_t started_usec = redis_observability_now_usec();
	redisReply *reply = NULL;
	if (world_context && !world_context->err)
	{
		va_list arguments;
		va_start(arguments, format);
		reply = static_cast<redisReply *>(redisvCommand(world_context, format, arguments));
		va_end(arguments);
	}
	return redis_command_finish(scope, kind, started_usec, reply);
}

redisReply *redis_command_argv(redis_shared_command_scope scope, redis_shared_command_kind kind,
			       int argc, const char **arguments, const size_t *lengths)
{
	const uint64_t started_usec = redis_observability_now_usec();
	redisReply *reply = NULL;
	if (world_context && !world_context->err)
		reply = static_cast<redisReply *>(
			redisCommandArgv(world_context, argc, arguments, lengths));
	return redis_command_finish(scope, kind, started_usec, reply);
}

bool redis_reconnect()
{
	if (world_context)
	{
		redisFree(world_context);
		world_context = NULL;
	}
	world_context = redis_connection_open(world_connection);
	if (!world_context || world_context->err)
	{
		if (world_context)
		{
			redisFree(world_context);
			world_context = NULL;
		}
		redis_shared_connection_observability_record(true, false);
		return false;
	}
	redis_shared_connection_observability_record(true, true);
	logit(LOG_SYS, "redis reconnected");
	return true;
}

bool redis_world_writer_token_create()
{
	unsigned char random[16] = {};
	if (RAND_bytes(random, sizeof random) != 1)
		return false;
	static const char hex[] = "0123456789abcdef";
	world_writer_token.resize(sizeof random * 2);
	for (size_t index = 0; index < sizeof random; ++index)
	{
		world_writer_token[index * 2] = hex[random[index] >> 4];
		world_writer_token[index * 2 + 1] = hex[random[index] & 0x0f];
	}
	return true;
}

bool redis_world_writer_fence_claim()
{
	if (!world_writer_token.empty())
	{
		const redis_world_store_config config = redis_world_store_config_copy();
		return redis_world_store_renew_fence(&config, world_writer_token.c_str(),
						     world_writer_lease_msec);
	}
	world_writer_epoch = world_runtime_epoch;
	if (!world_writer_epoch || !redis_world_writer_token_create())
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

bool redis_publish_world_generation(const unsigned char *data, size_t size,
				    const world_recovery_header *header,
				    redis_shared_command_outcome *outcome, void * /*context*/)
{
	if (!data || !size || !header || world_writer_token.empty() || !world_writer_lease_msec)
		return false;
	const redis_world_store_config config = redis_world_store_config_copy();
	return redis_world_store_publish_observed(&config, world_writer_token.c_str(),
						  world_writer_lease_msec, data, size,
						  header->sequence, header->timestamp,
						  header->checksum, outcome);
}

bool redis_world_recovery_ensure_initialized()
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
	if (world_context)
	{
		char current_key[128];
		redisReply *sequence_reply =
			redis_epoch_key(current_key, sizeof current_key, world_writer_epoch,
					REDIS_WORLD_CURRENT_SUFFIX) ?
				redis_command(REDIS_SHARED_SCOPE_WORLD, REDIS_SHARED_COMMAND_READ,
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

void redis_clear_floor_pickups()
{
	if (!world_context)
		return;
	redisReply *reply = redis_command(REDIS_SHARED_SCOPE_FLOOR, REDIS_SHARED_COMMAND_WRITE,
					  "DEL %s", REDIS_LEGACY_FLOOR_PICKUPS);
	if (reply)
		freeReplyObject(reply);
}

bool redis_clear_floor_drops()
{
	if (!world_context)
		return false;
	char floor_key[128];
	char floor_index_key[128];
	const uint64_t epoch = world_writer_epoch ? world_writer_epoch : world_runtime_epoch;
	if (!redis_epoch_key(floor_key, sizeof floor_key, epoch, REDIS_FLOOR_DROPS_SUFFIX) ||
	    !redis_epoch_key(floor_index_key, sizeof floor_index_key, epoch,
			     REDIS_FLOOR_DROP_INDEX_SUFFIX))
		return false;
	redisReply *reply = redis_command(REDIS_SHARED_SCOPE_FLOOR, REDIS_SHARED_COMMAND_WRITE,
					  "DEL %s %s", floor_key, floor_index_key);
	if (!reply || reply->type != REDIS_REPLY_INTEGER)
	{
		if (reply)
			freeReplyObject(reply);
		return false;
	}
	freeReplyObject(reply);
	reply = redis_command(REDIS_SHARED_SCOPE_FLOOR, REDIS_SHARED_COMMAND_READ, "EXISTS %s %s",
			      floor_key, floor_index_key);
	const bool absent = reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 0;
	if (reply)
		freeReplyObject(reply);
	return absent;
}

bool redis_read_floor_records(std::vector<std::vector<unsigned char>> *records,
			      size_t maximum_bytes)
{
	if (!records || !world_enabled || !world_context)
		return false;
	char floor_key[128];
	char floor_index_key[128];
	if (!redis_epoch_key(floor_key, sizeof floor_key, world_runtime_epoch,
			     REDIS_FLOOR_DROPS_SUFFIX) ||
	    !redis_epoch_key(floor_index_key, sizeof floor_index_key, world_runtime_epoch,
			     REDIS_FLOOR_DROP_INDEX_SUFFIX))
		return false;
	redisReply *index_count_reply = redis_command(
		REDIS_SHARED_SCOPE_FLOOR, REDIS_SHARED_COMMAND_READ, "ZCARD %s", floor_index_key);
	redisReply *hash_count_reply = redis_command(
		REDIS_SHARED_SCOPE_FLOOR, REDIS_SHARED_COMMAND_READ, "HLEN %s", floor_key);
	const bool counts_valid =
		index_count_reply && index_count_reply->type == REDIS_REPLY_INTEGER &&
		index_count_reply->integer >= 0 && hash_count_reply &&
		hash_count_reply->type == REDIS_REPLY_INTEGER && hash_count_reply->integer >= 0 &&
		index_count_reply->integer == hash_count_reply->integer &&
		static_cast<uint64_t>(index_count_reply->integer) <=
			WORLD_RECOVERY_MAX_FLOOR_RECORDS;
	const size_t record_count = counts_valid ? static_cast<size_t>(index_count_reply->integer) :
						   0;
	const long long index_count = index_count_reply && index_count_reply->type ==
								   REDIS_REPLY_INTEGER ?
					      index_count_reply->integer :
					      -1;
	const long long hash_count = hash_count_reply &&
						     hash_count_reply->type == REDIS_REPLY_INTEGER ?
					     hash_count_reply->integer :
					     -1;
	if (index_count_reply)
		freeReplyObject(index_count_reply);
	if (hash_count_reply)
		freeReplyObject(hash_count_reply);
	if (!counts_valid || (record_count && !maximum_bytes) ||
	    maximum_bytes > WORLD_RECOVERY_MAX_FLOOR_BYTES)
	{
		logit(LOG_SYS,
		      "redis: world recovery floor counts rejected index=%lld hash=%lld budget=%zu",
		      index_count, hash_count, maximum_bytes);
		return false;
	}
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
		redisReply *fields = redis_command(REDIS_SHARED_SCOPE_FLOOR,
						   REDIS_SHARED_COMMAND_READ, "ZRANGE %s %lld %lld",
						   floor_index_key, static_cast<long long>(first),
						   static_cast<long long>(first + count - 1));
		if (!fields || fields->type != REDIS_REPLY_ARRAY || fields->elements != count)
		{
			logit(LOG_SYS,
			      "redis: world recovery floor index page rejected first=%zu count=%zu",
			      first, count);
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
						    REDIS_SHARED_COMMAND_READ,
						    static_cast<int>(count + 2), arguments.data(),
						    lengths.data());
		if (!values || values->type != REDIS_REPLY_ARRAY || values->elements != count)
		{
			logit(LOG_SYS,
			      "redis: world recovery floor values page rejected first=%zu count=%zu",
			      first, count);
			valid = false;
		}
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
			{
				logit(LOG_SYS,
				      "redis: world recovery floor value rejected first=%zu index=%zu uid=%llu bytes=%zu root=%llu",
				      first, index, (unsigned long long)field_uids[index],
				      value && value->str ? value->len : 0,
				      (unsigned long long)root_uid);
				break;
			}
			const size_t record_size = value->len - WORLD_RECOVERY_FLOOR_PREFIX_BYTES;
			if (record_size > maximum_bytes - aggregate_size)
			{
				logit(LOG_SYS,
				      "redis: world recovery floor budget exceeded record=%zu aggregate=%zu budget=%zu",
				      record_size, aggregate_size, maximum_bytes);
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
	index_count_reply = redis_command(REDIS_SHARED_SCOPE_FLOOR, REDIS_SHARED_COMMAND_READ,
					  "ZCARD %s", floor_index_key);
	hash_count_reply = redis_command(REDIS_SHARED_SCOPE_FLOOR, REDIS_SHARED_COMMAND_READ,
					 "HLEN %s", floor_key);
	const bool stable = index_count_reply && index_count_reply->type == REDIS_REPLY_INTEGER &&
			    hash_count_reply && hash_count_reply->type == REDIS_REPLY_INTEGER &&
			    index_count_reply->integer == static_cast<long long>(record_count) &&
			    hash_count_reply->integer == static_cast<long long>(record_count);
	const long long final_index_count = index_count_reply && index_count_reply->type ==
									 REDIS_REPLY_INTEGER ?
						    index_count_reply->integer :
						    -1;
	const long long final_hash_count = hash_count_reply && hash_count_reply->type ==
								       REDIS_REPLY_INTEGER ?
						   hash_count_reply->integer :
						   -1;
	if (index_count_reply)
		freeReplyObject(index_count_reply);
	if (hash_count_reply)
		freeReplyObject(hash_count_reply);
	if (!stable)
	{
		logit(LOG_SYS,
		      "redis: world recovery floor changed during read expected=%zu index=%lld hash=%lld",
		      record_count, final_index_count, final_hash_count);
		records->clear();
	}
	return stable;
}
#endif
} // namespace

bool redis_world_runtime_start(const redis_world_runtime_config *config)
{
#ifdef __NO_REDIS__
	(void)config;
	return true;
#else
	if (!config || !config->connection || !config->key_namespace || !*config->key_namespace ||
	    !config->season_epoch)
		return false;
	world_connection = config->connection;
	world_runtime_epoch = config->season_epoch;
	try
	{
		world_key_namespace.assign(config->key_namespace);
	}
	catch (const std::bad_alloc &)
	{
		world_connection = NULL;
		world_runtime_epoch = 0;
		return false;
	}
	world_enabled = false;
	recovery_boot = false;
	clean_restart_boot = false;
	world_state_interval = WORLD_STATE_INTERVAL_DEFAULT;
	world_state_max_age = WORLD_STATE_MAX_AGE_DEFAULT;
	world_recovery_quiesced = false;
	world_writer_token.clear();
	world_writer_lease_msec = 0;
	world_writer_epoch = 0;
	world_floor_barrier_waiting = false;
	world_floor_handoff_active = false;
	clean_shutdown_sequence = 0;
	world_sequence_floor = 0;
	redis_clear_world_authentication_secrets();

	world_context = redis_connection_open(world_connection);
	if (!world_context)
		logit(LOG_SYS, "redis: failed to allocate context");
	else if (world_context->err)
	{
		logit(LOG_SYS, "redis connect failed: outcome=unavailable");
		redisFree(world_context);
		world_context = NULL;
	}
	else if (config->unix_socket)
		logit(LOG_SYS, "redis connected through configured Unix socket");
	else
		logit(LOG_SYS, "redis connected to %s:%d", config->host, config->port);
	redis_shared_connection_observability_record(false, world_context != NULL);

	const char *world_state_env = getenv("REDIS_WORLD_STATE");
	if (world_state_env && strcasecmp(world_state_env, "TRUE") == 0)
	{
		world_enabled = true;
		if (!redis_configure_world_authentication())
		{
			logit(LOG_SYS,
			      "redis: world recovery disabled; REDIS_WORLD_STATE_SECRET must be 32-256 bytes");
			world_enabled = false;
		}
		const char *interval_text = getenv("REDIS_WORLD_STATE_INTERVAL");
		if (interval_text && *interval_text)
		{
			const int interval = atoi(interval_text);
			if (interval >= 5 && interval <= 300)
				world_state_interval = interval;
		}
		const char *max_age_text = getenv("REDIS_WORLD_STATE_MAX_AGE");
		if (max_age_text && *max_age_text)
		{
			const int max_age = atoi(max_age_text);
			if (max_age >= 60 && max_age <= 3600)
				world_state_max_age = max_age;
		}
		if (world_enabled)
			logit(LOG_SYS, "redis world state enabled: interval=%ds, max_age=%ds",
			      world_state_interval, world_state_max_age);
		const redis_floor_store_config floor_config = { world_connection };
		if (world_enabled && !redis_floor_store_init(&floor_config))
		{
			logit(LOG_SYS, "redis: floor worker unavailable; world recovery disabled");
			world_enabled = false;
		}
		if (world_enabled)
			redis_floor_runtime_set_enabled(true);
		if (world_enabled)
		{
			const redis_world_store_config world_config =
				redis_world_store_config_copy();
			clean_shutdown_sequence =
				redis_world_store_consume_clean_shutdown(&world_config);
		}
	}
	if (world_context)
	{
		if (world_enabled && !redis_world_writer_fence_claim())
		{
			world_recovery_quiesced = true;
			redis_floor_runtime_set_quiesced(true);
			logit(LOG_SYS,
			      "redis: world publisher disabled; writer lease unavailable at boot");
		}
	}
	else if (world_enabled)
	{
		world_recovery_quiesced = true;
		redis_floor_runtime_set_quiesced(true);
	}
	return true;
#endif
}

void redis_world_runtime_shutdown(bool pwipe)
{
#ifndef __NO_REDIS__
	bool recovery_drained = true;
	redis_floor_runtime_set_enabled(false);
	redis_floor_runtime_set_quiesced(true);
	if (world_enabled)
	{
		recovery_drained = !world_recovery_pipeline_health_copy().initialized ||
				   redis_world_recovery_drain(WORLD_DRAIN_TIMEOUT_MSEC);
		if (!recovery_drained)
			logit(LOG_SYS, "redis: world recovery drain timed out during shutdown");
		if (world_recovery_pipeline_health_copy().initialized)
		{
			if (recovery_drained)
				world_recovery_pipeline_shutdown();
			else
				world_recovery_pipeline_cancel();
		}
	}
	const bool floor_drained = redis_floor_store_shutdown(FLOOR_DRAIN_TIMEOUT_MSEC);
	if (!floor_drained)
		logit(LOG_SYS, "redis: floor worker drain timed out during shutdown");
	if (!pwipe && !world_recovery_quiesced && recovery_drained && floor_drained &&
	    world_enabled && !world_writer_token.empty())
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
	recovery_boot = false;
	clean_restart_boot = false;
	if (world_context)
	{
		redisFree(world_context);
		world_context = NULL;
	}
	redis_clear_world_authentication_secrets();
	world_key_namespace.clear();
	world_connection = NULL;
	world_runtime_epoch = 0;
	world_enabled = false;
#else
	(void)pwipe;
#endif
}

bool redis_world_runtime_enabled(void)
{
	return world_enabled;
}

bool redis_world_recovery_boot_active(void)
{
	return recovery_boot;
}

void redis_world_recovery_boot_set(bool active)
{
	recovery_boot = active;
}

bool redis_world_clean_restart_boot(void)
{
	return clean_restart_boot;
}

void redis_world_recovery_boot_clear(void)
{
	recovery_boot = false;
	clean_restart_boot = false;
}

bool redis_save_world_state(void)
{
#ifdef __NO_REDIS__
	return false;
#else
	if (!world_enabled)
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
	if (!redis_flush_floor_drops() || !redis_floor_store_request_barrier())
		return false;
	world_floor_barrier_waiting = true;
	return true;
#endif
}

void redis_world_recovery_pulse(void)
{
#ifndef __NO_REDIS__
	if (!world_enabled || !world_recovery_pipeline_health_copy().initialized)
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
			logit(LOG_SYS,
			      "redis: world recovery generation and floor handoff acknowledged sequence=%llu attempts=%u",
			      (unsigned long long)completion.sequence, completion.attempts);
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
#ifdef __NO_REDIS__
	(void)timeout_msec;
	return true;
#else
	if (!world_enabled || !world_recovery_pipeline_health_copy().initialized)
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
#ifdef __NO_REDIS__
	return true;
#else
	world_recovery_quiesced = true;
	redis_floor_runtime_set_quiesced(true);
	redis_floor_store_cancel();
	world_floor_barrier_waiting = false;
	world_floor_handoff_active = false;
	if (world_recovery_pipeline_health_copy().initialized)
		world_recovery_pipeline_cancel();
	return !world_enabled || redis_world_writer_fence_claim();
#endif
}

bool redis_has_world_state(void)
{
#ifdef __NO_REDIS__
	return false;
#else
	if (!world_enabled)
	{
		clean_restart_boot = false;
		return false;
	}
	const uint64_t candidate_clean_sequence = clean_shutdown_sequence;
	clean_shutdown_sequence = 0;
	clean_restart_boot = false;
	if ((!world_context || world_context->err) && !redis_reconnect())
		return false;
	char current_key[128];
	if (!redis_epoch_key(current_key, sizeof current_key, world_runtime_epoch,
			     REDIS_WORLD_CURRENT_SUFFIX))
		return false;
	redisReply *reply = redis_command(REDIS_SHARED_SCOPE_WORLD, REDIS_SHARED_COMMAND_READ,
					  "GET %s", current_key);
	if (!reply)
		return false;
	uint64_t sequence = 0;
	if (reply->type == REDIS_REPLY_STRING && reply->str)
		sequence = strtoull(reply->str, NULL, 10);
	freeReplyObject(reply);
	if (!sequence)
		return false;
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
	clean_restart_boot = exists && candidate_clean_sequence == sequence;
	return exists;
#endif
}

bool redis_consume_world_state(void)
{
#ifdef __NO_REDIS__
	return false;
#else
	if (!world_enabled || !world_context || world_context->err)
		return false;
	char current_key[128];
	if (!redis_epoch_key(current_key, sizeof current_key, world_runtime_epoch,
			     REDIS_WORLD_CURRENT_SUFFIX))
		return false;
	redisReply *current = redis_command(REDIS_SHARED_SCOPE_WORLD, REDIS_SHARED_COMMAND_READ,
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
#ifdef __NO_REDIS__
	return false;
#else
	if (!world_enabled || ((!world_context || world_context->err) && !redis_reconnect()))
	{
		logit(LOG_SYS, "redis: world recovery load unavailable");
		return false;
	}
	char current_key[128];
	if (!redis_epoch_key(current_key, sizeof current_key, world_runtime_epoch,
			     REDIS_WORLD_CURRENT_SUFFIX))
	{
		logit(LOG_SYS, "redis: world recovery current key rejected");
		return false;
	}
	redisReply *sequence_reply = redis_command(
		REDIS_SHARED_SCOPE_WORLD, REDIS_SHARED_COMMAND_READ, "GET %s", current_key);
	if (!sequence_reply)
	{
		logit(LOG_SYS, "redis: world recovery current sequence read failed");
		return false;
	}
	uint64_t expected_sequence = 0;
	if (sequence_reply->type == REDIS_REPLY_STRING && sequence_reply->str)
		expected_sequence = strtoull(sequence_reply->str, NULL, 10);
	freeReplyObject(sequence_reply);
	if (!expected_sequence)
	{
		logit(LOG_SYS, "redis: world recovery current sequence rejected");
		return false;
	}
	std::vector<unsigned char> generation;
	const redis_world_store_config config = redis_world_store_config_copy();
	if (!redis_world_store_read_generation(&config, expected_sequence, &generation))
	{
		logit(LOG_SYS, "redis: world recovery generation read failed sequence=%llu",
		      (unsigned long long)expected_sequence);
		return false;
	}
	std::vector<std::vector<unsigned char>> floor_records;
	const size_t floor_budget = generation.size() <= WORLD_RECOVERY_MAX_BYTES ?
					    std::min(WORLD_RECOVERY_MAX_FLOOR_BYTES,
						     WORLD_RECOVERY_MAX_BYTES - generation.size()) :
					    0;
	if (!redis_read_floor_records(&floor_records, floor_budget))
	{
		logit(LOG_SYS, "redis: world recovery floor read failed sequence=%llu budget=%zu",
		      (unsigned long long)expected_sequence, floor_budget);
		return false;
	}
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
		logit(LOG_SYS, "redis: world recovery floor metadata allocation failed");
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
	if (world_enabled)
	{
		if (!redis_save_world_state())
			nevent_periodic_mark_failure("world-state persistence did not complete");
	}
	else
		nevent_periodic_mark_failure("world-state persistence is disabled");
	nevent_periodic_next_after(world_state_interval * WAIT_SEC);
}
