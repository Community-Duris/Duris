#include "redis_world_store.h"
#include "redis_connection.h"
#include "redis_key_registry.h"

#include "world_recovery_pipeline.h"

#include <hiredis/hiredis.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/time.h>

namespace
{
struct world_keys
{
	char fence[128];
	char current[128];
	char timestamp[128];
	char sequence[128];
	char checksum[128];
	char complete[128];
	char floor_drops[128];
	char clean_shutdown[128];
};

constexpr const char *WORLD_PUBLISH_SCRIPT =
	"if redis.call('GET',KEYS[1])~=ARGV[1] then return 0 end "
	"local current=redis.call('GET',KEYS[2]) "
	"if ARGV[2]=='' then if current then return 0 end "
	"elseif current~=ARGV[2] then return 0 end "
	"redis.call('SET',KEYS[3],ARGV[3]) "
	"redis.call('EXPIRE',KEYS[3],ARGV[8]) "
	"redis.call('SET',KEYS[2],ARGV[4]) "
	"redis.call('SET',KEYS[4],ARGV[5]) "
	"redis.call('SET',KEYS[5],ARGV[4]) "
	"redis.call('SET',KEYS[6],ARGV[6]) "
	"redis.call('SET',KEYS[7],'1') "
	"redis.call('DEL',KEYS[8]) "
	"redis.call('PEXPIRE',KEYS[1],ARGV[7]) "
	"return 1";

bool format_key(char *buffer, size_t size, uint64_t epoch, const char *suffix)
{
	if (!buffer || size < 64 || !epoch || !suffix || !*suffix)
		return false;
	const int written =
		snprintf(buffer, size, REDIS_SEASON_KEY_FORMAT, (unsigned long long)epoch, suffix);
	return written > 0 && (size_t)written < size;
}

bool build_keys(const redis_world_store_config *config, world_keys *keys)
{
	return config && keys && config->season_epoch &&
	       format_key(keys->fence, sizeof keys->fence, config->season_epoch,
			  REDIS_WORLD_FENCE_SUFFIX) &&
	       format_key(keys->current, sizeof keys->current, config->season_epoch,
			  REDIS_WORLD_CURRENT_SUFFIX) &&
	       format_key(keys->timestamp, sizeof keys->timestamp, config->season_epoch,
			  REDIS_WORLD_TIMESTAMP_SUFFIX) &&
	       format_key(keys->sequence, sizeof keys->sequence, config->season_epoch,
			  REDIS_WORLD_SEQUENCE_SUFFIX) &&
	       format_key(keys->checksum, sizeof keys->checksum, config->season_epoch,
			  REDIS_WORLD_CHECKSUM_SUFFIX) &&
	       format_key(keys->complete, sizeof keys->complete, config->season_epoch,
			  REDIS_WORLD_COMPLETE_SUFFIX) &&
	       format_key(keys->floor_drops, sizeof keys->floor_drops, config->season_epoch,
			  REDIS_FLOOR_DROPS_SUFFIX) &&
	       format_key(keys->clean_shutdown, sizeof keys->clean_shutdown, config->season_epoch,
			  REDIS_WORLD_CLEAN_SHUTDOWN_SUFFIX);
}

redisContext *connect_bounded(const redis_world_store_config *config,
			      int minimum_command_timeout_msec = 0)
{
	return config ? redis_connection_open_with_timeout(config->connection,
							   minimum_command_timeout_msec) :
			nullptr;
}

redisReply *command(redisContext *context, const char *format, ...)
{
	if (!context || context->err || !format)
		return nullptr;
	va_list arguments;
	va_start(arguments, format);
	redisReply *reply = (redisReply *)redisvCommand(context, format, arguments);
	va_end(arguments);
	if (!reply || reply->type == REDIS_REPLY_ERROR)
	{
		if (reply)
			freeReplyObject(reply);
		return nullptr;
	}
	return reply;
}

bool status_ok(redisReply *reply)
{
	const bool result = reply && reply->type == REDIS_REPLY_STATUS && reply->str &&
			    strcmp(reply->str, "OK") == 0;
	if (reply)
		freeReplyObject(reply);
	return result;
}

bool token_matches(redisContext *context, const char *fence_key, const char *writer_token)
{
	redisReply *reply = command(context, "GET %s", fence_key);
	const bool matches = reply && reply->type == REDIS_REPLY_STRING && reply->str &&
			     reply->len == strlen(writer_token) &&
			     memcmp(reply->str, writer_token, reply->len) == 0;
	if (reply)
		freeReplyObject(reply);
	return matches;
}

void end_watch(redisContext *context, bool transaction_started)
{
	redisReply *reply = command(context, transaction_started ? "DISCARD" : "UNWATCH");
	if (reply)
		freeReplyObject(reply);
}
} // namespace

bool redis_world_store_claim_fence(const struct redis_world_store_config *config,
				   const char *writer_token, uint64_t lease_msec)
{
	world_keys keys = {};
	if (!writer_token || !*writer_token || !lease_msec || !build_keys(config, &keys))
		return false;
	redisContext *context = connect_bounded(config);
	if (!context || context->err)
	{
		if (context)
			redisFree(context);
		return false;
	}
	const bool claimed =
		status_ok(command(context, "SET %s %b NX PX %llu", keys.fence, writer_token,
				  strlen(writer_token), (unsigned long long)lease_msec));
	redisFree(context);
	return claimed;
}

bool redis_world_store_renew_fence(const struct redis_world_store_config *config,
				   const char *writer_token, uint64_t lease_msec)
{
	world_keys keys = {};
	if (!writer_token || !*writer_token || !lease_msec || !build_keys(config, &keys))
		return false;
	redisContext *context = connect_bounded(config);
	if (!context || context->err)
	{
		if (context)
			redisFree(context);
		return false;
	}
	bool transaction_started = false;
	bool renewed = status_ok(command(context, "WATCH %s", keys.fence)) &&
		       token_matches(context, keys.fence, writer_token) &&
		       status_ok(command(context, "MULTI"));
	if (renewed)
	{
		transaction_started = true;
		redisReply *queued = command(context, "PEXPIRE %s %llu", keys.fence,
					     (unsigned long long)lease_msec);
		renewed = queued && queued->type == REDIS_REPLY_STATUS;
		if (queued)
			freeReplyObject(queued);
	}
	if (renewed)
	{
		redisReply *reply = command(context, "EXEC");
		renewed = reply && reply->type == REDIS_REPLY_ARRAY && reply->elements == 1 &&
			  reply->element[0] && reply->element[0]->type == REDIS_REPLY_INTEGER &&
			  reply->element[0]->integer == 1;
		if (reply)
			freeReplyObject(reply);
	}
	else
		end_watch(context, transaction_started);
	redisFree(context);
	return renewed;
}

bool redis_world_store_release_fence(const struct redis_world_store_config *config,
				     const char *writer_token)
{
	world_keys keys = {};
	if (!writer_token || !*writer_token || !build_keys(config, &keys))
		return false;
	redisContext *context = connect_bounded(config);
	if (!context || context->err)
	{
		if (context)
			redisFree(context);
		return false;
	}
	bool transaction_started = false;
	bool released = status_ok(command(context, "WATCH %s", keys.fence)) &&
			token_matches(context, keys.fence, writer_token) &&
			status_ok(command(context, "MULTI"));
	if (released)
	{
		transaction_started = true;
		redisReply *queued = command(context, "DEL %s", keys.fence);
		released = queued && queued->type == REDIS_REPLY_STATUS;
		if (queued)
			freeReplyObject(queued);
	}
	if (released)
	{
		redisReply *reply = command(context, "EXEC");
		released = reply && reply->type == REDIS_REPLY_ARRAY && reply->elements == 1 &&
			   reply->element[0] && reply->element[0]->type == REDIS_REPLY_INTEGER &&
			   reply->element[0]->integer == 1;
		if (reply)
			freeReplyObject(reply);
	}
	else
		end_watch(context, transaction_started);
	redisFree(context);
	return released;
}

bool redis_world_store_mark_clean_shutdown(const struct redis_world_store_config *config,
					   const char *writer_token)
{
	world_keys keys = {};
	if (!writer_token || !*writer_token || !config || !config->generation_ttl_seconds ||
	    !build_keys(config, &keys))
		return false;
	redisContext *context = connect_bounded(config);
	if (!context || context->err)
	{
		if (context)
			redisFree(context);
		return false;
	}
	constexpr const char *script = "if redis.call('GET',KEYS[1])~=ARGV[1] then return 0 end "
				       "local current=redis.call('GET',KEYS[2]) "
				       "if not current then redis.call('DEL',KEYS[3]) return 0 end "
				       "redis.call('SET',KEYS[3],current,'EX',ARGV[2]) return 1";
	redisReply *reply = command(context, "EVAL %b 3 %s %s %s %b %llu", script, strlen(script),
				    keys.fence, keys.current, keys.clean_shutdown, writer_token,
				    strlen(writer_token),
				    (unsigned long long)config->generation_ttl_seconds);
	const bool marked = reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 1;
	if (reply)
		freeReplyObject(reply);
	redisFree(context);
	return marked;
}

uint64_t redis_world_store_consume_clean_shutdown(const struct redis_world_store_config *config)
{
	world_keys keys = {};
	if (!build_keys(config, &keys))
		return 0;
	redisContext *context = connect_bounded(config);
	if (!context || context->err)
	{
		if (context)
			redisFree(context);
		return 0;
	}
	constexpr const char *script =
		"local value=redis.call('GET',KEYS[1]) redis.call('DEL',KEYS[1]) return value";
	redisReply *reply =
		command(context, "EVAL %b 1 %s", script, strlen(script), keys.clean_shutdown);
	uint64_t sequence = 0;
	if (reply && reply->type == REDIS_REPLY_STRING && reply->str)
		sequence = strtoull(reply->str, nullptr, 10);
	if (reply)
		freeReplyObject(reply);
	redisFree(context);
	return sequence;
}

bool redis_world_store_consume_generation(const struct redis_world_store_config *config,
					  const char *writer_token, uint64_t sequence)
{
	world_keys keys = {};
	char generation[160];
	char suffix[96];
	const int suffix_length = snprintf(suffix, sizeof suffix, REDIS_WORLD_GENERATION_FORMAT,
					   (unsigned long long)sequence);
	if (!writer_token || !*writer_token || !sequence || !build_keys(config, &keys) ||
	    suffix_length <= 0 || (size_t)suffix_length >= sizeof suffix ||
	    !format_key(generation, sizeof generation, config->season_epoch, suffix))
		return false;
	redisContext *context = connect_bounded(config);
	if (!context || context->err)
	{
		if (context)
			redisFree(context);
		return false;
	}
	constexpr const char *script =
		"if redis.call('GET',KEYS[1])~=ARGV[1] then return 0 end "
		"if redis.call('GET',KEYS[2])~=ARGV[2] then return 0 end "
		"redis.call('DEL',KEYS[2],KEYS[3],KEYS[4],KEYS[5],KEYS[6],KEYS[7],KEYS[8]) "
		"return 1";
	redisReply *reply = command(context, "EVAL %b 8 %s %s %s %s %s %s %s %s %b %llu", script,
				    strlen(script), keys.fence, keys.current, generation,
				    keys.timestamp, keys.sequence, keys.checksum, keys.complete,
				    keys.clean_shutdown, writer_token, strlen(writer_token),
				    (unsigned long long)sequence);
	const bool consumed = reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 1;
	if (reply)
		freeReplyObject(reply);
	redisFree(context);
	return consumed;
}

bool redis_world_store_publish(const struct redis_world_store_config *config,
			       const char *writer_token, uint64_t lease_msec,
			       const unsigned char *data, size_t size, uint64_t sequence,
			       int64_t timestamp, uint32_t checksum)
{
	world_keys keys = {};
	char generation[160];
	char generation_suffix[96];
	const int suffix_length = snprintf(generation_suffix, sizeof generation_suffix,
					   REDIS_WORLD_GENERATION_FORMAT,
					   (unsigned long long)sequence);
	if (!writer_token || !*writer_token || !lease_msec || !data || !size ||
	    size > WORLD_RECOVERY_MAX_BYTES || !sequence || timestamp <= 0 || !config ||
	    !config->generation_ttl_seconds || !build_keys(config, &keys) || suffix_length <= 0 ||
	    (size_t)suffix_length >= sizeof generation_suffix ||
	    !format_key(generation, sizeof generation, config->season_epoch, generation_suffix))
		return false;
	constexpr size_t assumed_bytes_per_second = 16 * 1024 * 1024;
	constexpr int maximum_publish_timeout_msec = 5000;
	const uint64_t transfer_msec =
		(size * 1000ULL + assumed_bytes_per_second - 1) / assumed_bytes_per_second;
	const int publish_timeout_msec =
		std::min(maximum_publish_timeout_msec, static_cast<int>(transfer_msec + 100));
	redisContext *context = connect_bounded(config, publish_timeout_msec);
	if (!context || context->err)
	{
		if (context)
			redisFree(context);
		return false;
	}

	uint64_t previous_sequence = 0;
	redisReply *current_reply = command(context, "GET %s", keys.current);
	bool valid = current_reply &&
		     (current_reply->type == REDIS_REPLY_NIL ||
		      (current_reply->type == REDIS_REPLY_STRING && current_reply->str));
	if (valid && current_reply->type == REDIS_REPLY_STRING)
		previous_sequence = strtoull(current_reply->str, nullptr, 10);
	if (current_reply)
		freeReplyObject(current_reply);

	char expected[32] = {};
	const int expected_length = previous_sequence ?
					    snprintf(expected, sizeof expected, "%llu",
						     (unsigned long long)previous_sequence) :
					    0;
	if (expected_length < 0 || (size_t)expected_length >= sizeof expected)
		valid = false;
	if (valid)
	{
		redisReply *reply = command(
			context,
			"EVAL %b 8 %s %s %s %s %s %s %s %s %b %b %b %llu %lld %u %llu %llu",
			WORLD_PUBLISH_SCRIPT, strlen(WORLD_PUBLISH_SCRIPT), keys.fence,
			keys.current, generation, keys.timestamp, keys.sequence, keys.checksum,
			keys.complete, keys.floor_drops, writer_token, strlen(writer_token),
			expected, (size_t)expected_length, data, size, (unsigned long long)sequence,
			(long long)timestamp, checksum, (unsigned long long)lease_msec,
			(unsigned long long)config->generation_ttl_seconds);
		valid = reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 1;
		if (reply)
			freeReplyObject(reply);
	}

	if (valid && previous_sequence && previous_sequence != sequence)
	{
		char previous[160];
		char previous_suffix[96];
		const int previous_suffix_length = snprintf(previous_suffix, sizeof previous_suffix,
							    REDIS_WORLD_GENERATION_FORMAT,
							    (unsigned long long)previous_sequence);
		redisReply *reply =
			previous_suffix_length > 0 &&
					(size_t)previous_suffix_length < sizeof previous_suffix &&
					format_key(previous, sizeof previous, config->season_epoch,
						   previous_suffix) ?
				command(context, "DEL %s", previous) :
				nullptr;
		if (reply)
			freeReplyObject(reply);
	}
	redisFree(context);
	return valid;
}
