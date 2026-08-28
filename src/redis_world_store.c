#include "redis_world_store.h"
#include "redis_connection.h"
#include "redis_key_registry.h"

#include "world_recovery_pipeline.h"

#include <hiredis/hiredis.h>

#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <sys/time.h>
#include <vector>

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
	char floor_drop_index[128];
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
	"redis.call('DEL',KEYS[8],KEYS[9]) "
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
	       format_key(keys->floor_drop_index, sizeof keys->floor_drop_index,
			  config->season_epoch, REDIS_FLOOR_DROP_INDEX_SUFFIX) &&
	       format_key(keys->clean_shutdown, sizeof keys->clean_shutdown, config->season_epoch,
			  REDIS_WORLD_CLEAN_SHUTDOWN_SUFFIX);
}

bool generation_key(char *buffer, size_t size, const redis_world_store_config *config,
		    uint64_t sequence)
{
	char suffix[96];
	const int written = snprintf(suffix, sizeof suffix, REDIS_WORLD_GENERATION_FORMAT,
				     (unsigned long long)sequence);
	return config && sequence && written > 0 && (size_t)written < sizeof suffix &&
	       format_key(buffer, size, config->season_epoch, suffix);
}

bool generation_chunk_key(char *buffer, size_t size, const redis_world_store_config *config,
			  uint64_t sequence, const char *upload_token, unsigned int index)
{
	char suffix[128];
	if (!upload_token || strlen(upload_token) != 32)
		return false;
	const int written = snprintf(suffix, sizeof suffix, REDIS_WORLD_GENERATION_CHUNK_FORMAT,
				     (unsigned long long)sequence, upload_token, index);
	return config && sequence && index < REDIS_WORLD_GENERATION_MAX_CHUNKS && written > 0 &&
	       (size_t)written < sizeof suffix &&
	       format_key(buffer, size, config->season_epoch, suffix);
}

void put_u32(unsigned char *output, uint32_t value)
{
	for (size_t index = 0; index < 4; ++index)
		output[index] = static_cast<unsigned char>(value >> (index * 8));
}

void put_u64(unsigned char *output, uint64_t value)
{
	for (size_t index = 0; index < 8; ++index)
		output[index] = static_cast<unsigned char>(value >> (index * 8));
}

uint32_t get_u32(const unsigned char *data)
{
	uint32_t value = 0;
	for (size_t index = 0; index < 4; ++index)
		value |= static_cast<uint32_t>(data[index]) << (index * 8);
	return value;
}

uint64_t get_u64(const unsigned char *data)
{
	uint64_t value = 0;
	for (size_t index = 0; index < 8; ++index)
		value |= static_cast<uint64_t>(data[index]) << (index * 8);
	return value;
}

bool encode_manifest(unsigned char *output, size_t output_size, size_t generation_size,
		     const char *upload_token)
{
	if (!output || output_size < REDIS_WORLD_GENERATION_MANIFEST_BYTES || !generation_size ||
	    generation_size > WORLD_RECOVERY_MAX_BYTES || !upload_token ||
	    strlen(upload_token) != 32)
		return false;
	const size_t chunks = (generation_size + REDIS_WORLD_GENERATION_CHUNK_BYTES - 1) /
			      REDIS_WORLD_GENERATION_CHUNK_BYTES;
	if (!chunks || chunks > REDIS_WORLD_GENERATION_MAX_CHUNKS)
		return false;
	memcpy(output, "WRG1", 4);
	put_u32(output + 4, 1);
	put_u64(output + 8, generation_size);
	put_u32(output + 16, static_cast<uint32_t>(chunks));
	put_u32(output + 20, REDIS_WORLD_GENERATION_CHUNK_BYTES);
	memcpy(output + 24, upload_token, 32);
	return true;
}

bool decode_manifest(const unsigned char *data, size_t size, size_t *generation_size,
		     size_t *chunk_count, char *upload_token, size_t upload_token_size)
{
	if (!data || size != REDIS_WORLD_GENERATION_MANIFEST_BYTES || !generation_size ||
	    !chunk_count || !upload_token || upload_token_size < 33 || memcmp(data, "WRG1", 4) ||
	    get_u32(data + 4) != 1 || get_u32(data + 20) != REDIS_WORLD_GENERATION_CHUNK_BYTES)
		return false;
	const uint64_t total = get_u64(data + 8);
	const uint32_t chunks = get_u32(data + 16);
	if (!total || total > WORLD_RECOVERY_MAX_BYTES || !chunks ||
	    chunks > REDIS_WORLD_GENERATION_MAX_CHUNKS ||
	    chunks != (total + REDIS_WORLD_GENERATION_CHUNK_BYTES - 1) /
			      REDIS_WORLD_GENERATION_CHUNK_BYTES)
		return false;
	*generation_size = static_cast<size_t>(total);
	*chunk_count = chunks;
	for (size_t index = 0; index < 32; ++index)
		if (!((data[24 + index] >= '0' && data[24 + index] <= '9') ||
		      (data[24 + index] >= 'a' && data[24 + index] <= 'f')))
			return false;
	memcpy(upload_token, data + 24, 32);
	upload_token[32] = '\0';
	return true;
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

redisReply *bounded_string(redisContext *context, const char *key, size_t maximum_bytes,
			   size_t expected_bytes = 0)
{
	if (!context || context->err || !key || !*key || !maximum_bytes ||
	    expected_bytes > maximum_bytes)
		return nullptr;
	redisReply *length = command(context, "STRLEN %s", key);
	if (!length || length->type != REDIS_REPLY_INTEGER || length->integer <= 0 ||
	    static_cast<uint64_t>(length->integer) > maximum_bytes ||
	    (expected_bytes && static_cast<size_t>(length->integer) != expected_bytes))
	{
		if (length)
			freeReplyObject(length);
		return nullptr;
	}
	const size_t exact_size = static_cast<size_t>(length->integer);
	freeReplyObject(length);
	redisReply *reply = command(context, "GET %s", key);
	if (!reply || reply->type != REDIS_REPLY_STRING || !reply->str || reply->len != exact_size)
	{
		if (reply)
			freeReplyObject(reply);
		return nullptr;
	}
	return reply;
}

bool delete_chunk_batch(redisContext *context, const redis_world_store_config *config,
			uint64_t sequence, const char *upload_token, size_t first, size_t count)
{
	constexpr size_t maximum_batch = 16;
	if (!context || !config || !sequence || !upload_token || !count || count > maximum_batch ||
	    first >= REDIS_WORLD_GENERATION_MAX_CHUNKS ||
	    count > REDIS_WORLD_GENERATION_MAX_CHUNKS - first)
		return false;
	std::array<std::array<char, 192>, maximum_batch> keys = {};
	std::array<const char *, maximum_batch + 1> arguments = {};
	std::array<size_t, maximum_batch + 1> lengths = {};
	arguments[0] = "DEL";
	lengths[0] = 3;
	for (size_t index = 0; index < count; ++index)
	{
		if (!generation_chunk_key(keys[index].data(), keys[index].size(), config, sequence,
					  upload_token, static_cast<unsigned int>(first + index)))
			return false;
		arguments[index + 1] = keys[index].data();
		lengths[index + 1] = strlen(keys[index].data());
	}
	redisReply *reply = static_cast<redisReply *>(redisCommandArgv(
		context, static_cast<int>(count + 1), arguments.data(), lengths.data()));
	const bool valid = reply && reply->type == REDIS_REPLY_INTEGER;
	if (reply)
		freeReplyObject(reply);
	return valid;
}

bool delete_generation_artifacts(redisContext *context, const redis_world_store_config *config,
				 uint64_t sequence, const char *upload_token, bool delete_manifest)
{
	if (!context || !config || !sequence || !upload_token)
		return false;
	bool valid = true;
	if (delete_manifest)
	{
		char manifest[160];
		valid = generation_key(manifest, sizeof manifest, config, sequence);
		if (valid)
		{
			redisReply *reply = command(context, "DEL %s", manifest);
			valid = reply && reply->type == REDIS_REPLY_INTEGER;
			if (reply)
				freeReplyObject(reply);
		}
	}
	for (size_t first = 0; first < REDIS_WORLD_GENERATION_MAX_CHUNKS; first += 16)
		valid = delete_chunk_batch(
				context, config, sequence, upload_token, first,
				std::min<size_t>(16, REDIS_WORLD_GENERATION_MAX_CHUNKS - first)) &&
			valid;
	return valid;
}

bool manifest_upload_token(redisContext *context, const redis_world_store_config *config,
			   uint64_t sequence, char *upload_token, size_t upload_token_size)
{
	char key[160];
	if (!context || !config || !sequence || !upload_token || upload_token_size < 33 ||
	    !generation_key(key, sizeof key, config, sequence))
		return false;
	redisReply *manifest = bounded_string(context, key, REDIS_WORLD_GENERATION_MANIFEST_BYTES,
					      REDIS_WORLD_GENERATION_MANIFEST_BYTES);
	size_t generation_size = 0;
	size_t chunk_count = 0;
	const bool valid = manifest &&
			   decode_manifest(reinterpret_cast<const unsigned char *>(manifest->str),
					   manifest->len, &generation_size, &chunk_count,
					   upload_token, upload_token_size);
	if (manifest)
		freeReplyObject(manifest);
	return valid;
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
	if (!writer_token || !*writer_token || !sequence || !build_keys(config, &keys) ||
	    !generation_key(generation, sizeof generation, config, sequence))
		return false;
	redisContext *context = connect_bounded(config);
	if (!context || context->err)
	{
		if (context)
			redisFree(context);
		return false;
	}
	char upload_token[33] = {};
	if (!manifest_upload_token(context, config, sequence, upload_token, sizeof upload_token))
	{
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
	if (consumed)
		delete_generation_artifacts(context, config, sequence, upload_token, false);
	redisFree(context);
	return consumed;
}

bool redis_world_store_read_generation(const struct redis_world_store_config *config,
				       uint64_t sequence, std::vector<unsigned char> *generation)
{
	char manifest_key[160];
	if (!config || !sequence || !generation ||
	    !generation_key(manifest_key, sizeof manifest_key, config, sequence))
		return false;
	redisContext *context = connect_bounded(config, 500);
	if (!context || context->err)
	{
		if (context)
			redisFree(context);
		return false;
	}
	redisReply *manifest = bounded_string(context, manifest_key,
					      REDIS_WORLD_GENERATION_MANIFEST_BYTES,
					      REDIS_WORLD_GENERATION_MANIFEST_BYTES);
	size_t generation_size = 0;
	size_t chunk_count = 0;
	char upload_token[33] = {};
	bool valid = manifest &&
		     decode_manifest(reinterpret_cast<const unsigned char *>(manifest->str),
				     manifest->len, &generation_size, &chunk_count, upload_token,
				     sizeof upload_token);
	if (manifest)
		freeReplyObject(manifest);
	try
	{
		if (valid)
			generation->assign(generation_size, 0);
		else
			generation->clear();
	}
	catch (const std::bad_alloc &)
	{
		valid = false;
		generation->clear();
	}
	for (size_t index = 0; valid && index < chunk_count; ++index)
	{
		char chunk_key[192];
		const size_t offset = index * REDIS_WORLD_GENERATION_CHUNK_BYTES;
		const size_t expected =
			std::min(REDIS_WORLD_GENERATION_CHUNK_BYTES, generation_size - offset);
		if (!generation_chunk_key(chunk_key, sizeof chunk_key, config, sequence,
					  upload_token, static_cast<unsigned int>(index)))
		{
			valid = false;
			break;
		}
		redisReply *chunk = bounded_string(context, chunk_key,
						   REDIS_WORLD_GENERATION_CHUNK_BYTES, expected);
		if (!chunk)
		{
			valid = false;
			break;
		}
		memcpy(generation->data() + offset, chunk->str, expected);
		freeReplyObject(chunk);
	}
	redisFree(context);
	if (!valid)
		generation->clear();
	return valid;
}

bool redis_world_store_publish(const struct redis_world_store_config *config,
			       const char *writer_token, uint64_t lease_msec,
			       const unsigned char *data, size_t size, uint64_t sequence,
			       int64_t timestamp, uint32_t checksum)
{
	world_keys keys = {};
	char generation[160];
	if (!writer_token || !*writer_token || !lease_msec || !data || !size ||
	    size > WORLD_RECOVERY_MAX_BYTES || !sequence || timestamp <= 0 || !config ||
	    !config->generation_ttl_seconds || !build_keys(config, &keys) ||
	    !generation_key(generation, sizeof generation, config, sequence))
		return false;
	redisContext *context = connect_bounded(config, 500);
	if (!context || context->err)
	{
		if (context)
			redisFree(context);
		return false;
	}

	uint64_t previous_sequence = 0;
	redisReply *current_reply = command(context, "GET %s", keys.current);
	bool valid = current_reply && token_matches(context, keys.fence, writer_token) &&
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
	std::array<unsigned char, REDIS_WORLD_GENERATION_MANIFEST_BYTES> manifest = {};
	valid = valid && encode_manifest(manifest.data(), manifest.size(), size, writer_token);
	const size_t chunk_count = (size + REDIS_WORLD_GENERATION_CHUNK_BYTES - 1) /
				   REDIS_WORLD_GENERATION_CHUNK_BYTES;
	for (size_t index = 0; valid && index < chunk_count; ++index)
	{
		char chunk_key[192];
		const size_t offset = index * REDIS_WORLD_GENERATION_CHUNK_BYTES;
		const size_t chunk_size =
			std::min(REDIS_WORLD_GENERATION_CHUNK_BYTES, size - offset);
		valid = generation_chunk_key(chunk_key, sizeof chunk_key, config, sequence,
					     writer_token, static_cast<unsigned int>(index)) &&
			status_ok(command(context, "SET %s %b EX %llu", chunk_key, data + offset,
					  chunk_size,
					  (unsigned long long)config->generation_ttl_seconds));
	}
	if (valid)
	{
		redisReply *reply = command(
			context,
			"EVAL %b 9 %s %s %s %s %s %s %s %s %s %b %b %b %llu %lld %u %llu %llu",
			WORLD_PUBLISH_SCRIPT, strlen(WORLD_PUBLISH_SCRIPT), keys.fence,
			keys.current, generation, keys.timestamp, keys.sequence, keys.checksum,
			keys.complete, keys.floor_drops, keys.floor_drop_index, writer_token,
			strlen(writer_token), expected, (size_t)expected_length, manifest.data(),
			manifest.size(), (unsigned long long)sequence, (long long)timestamp,
			checksum, (unsigned long long)lease_msec,
			(unsigned long long)config->generation_ttl_seconds);
		valid = reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 1;
		if (reply)
			freeReplyObject(reply);
	}
	if (!valid)
		delete_generation_artifacts(context, config, sequence, writer_token, false);

	if (valid && previous_sequence && previous_sequence != sequence)
	{
		char previous_upload_token[33] = {};
		if (manifest_upload_token(context, config, previous_sequence, previous_upload_token,
					  sizeof previous_upload_token))
			delete_generation_artifacts(context, config, previous_sequence,
						    previous_upload_token, true);
	}
	redisFree(context);
	return valid;
}
