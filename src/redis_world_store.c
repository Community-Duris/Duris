#include "redis_world_store.h"

#include <hiredis/hiredis.h>

#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>

namespace
{
redisContext *connect_bounded(const redis_world_store_config *config)
{
	if (!config || !config->host || !*config->host || config->port <= 0 ||
	    config->port > 65535 || config->connect_timeout_msec <= 0 ||
	    config->command_timeout_msec <= 0)
		return nullptr;
	struct timeval connect_timeout = { config->connect_timeout_msec / 1000,
					   (config->connect_timeout_msec % 1000) * 1000 };
	struct timeval command_timeout = { config->command_timeout_msec / 1000,
					   (config->command_timeout_msec % 1000) * 1000 };
	redisContext *context =
		redisConnectWithTimeout(config->host, config->port, connect_timeout);
	if (!context || context->err)
		return context;
	if (redisSetTimeout(context, command_timeout) != REDIS_OK)
	{
		redisFree(context);
		return nullptr;
	}
	return context;
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

bool token_matches(redisContext *context, const char *writer_token)
{
	redisReply *reply = command(context, "GET mud:world_state:writer_fence");
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
	if (!writer_token || !*writer_token || !lease_msec)
		return false;
	redisContext *context = connect_bounded(config);
	if (!context || context->err)
	{
		if (context)
			redisFree(context);
		return false;
	}
	const bool claimed = status_ok(
		command(context, "SET mud:world_state:writer_fence %b NX PX %llu", writer_token,
			strlen(writer_token), (unsigned long long)lease_msec));
	redisFree(context);
	return claimed;
}

bool redis_world_store_renew_fence(const struct redis_world_store_config *config,
				   const char *writer_token, uint64_t lease_msec)
{
	if (!writer_token || !*writer_token || !lease_msec)
		return false;
	redisContext *context = connect_bounded(config);
	if (!context || context->err)
	{
		if (context)
			redisFree(context);
		return false;
	}
	bool transaction_started = false;
	bool renewed = status_ok(command(context, "WATCH mud:world_state:writer_fence")) &&
		       token_matches(context, writer_token) && status_ok(command(context, "MULTI"));
	if (renewed)
	{
		transaction_started = true;
		redisReply *queued = command(context, "PEXPIRE mud:world_state:writer_fence %llu",
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
	if (!writer_token || !*writer_token)
		return false;
	redisContext *context = connect_bounded(config);
	if (!context || context->err)
	{
		if (context)
			redisFree(context);
		return false;
	}
	bool transaction_started = false;
	bool released = status_ok(command(context, "WATCH mud:world_state:writer_fence")) &&
			token_matches(context, writer_token) &&
			status_ok(command(context, "MULTI"));
	if (released)
	{
		transaction_started = true;
		redisReply *queued = command(context, "DEL mud:world_state:writer_fence");
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

bool redis_world_store_publish(const struct redis_world_store_config *config,
			       const char *writer_token, uint64_t lease_msec,
			       const unsigned char *data, size_t size, uint64_t sequence,
			       int64_t timestamp, uint32_t checksum)
{
	if (!writer_token || !*writer_token || !lease_msec || !data || !size || !sequence ||
	    timestamp <= 0)
		return false;
	redisContext *context = connect_bounded(config);
	if (!context || context->err)
	{
		if (context)
			redisFree(context);
		return false;
	}

	bool transaction_started = false;
	bool valid = status_ok(command(context, "WATCH mud:world_state:writer_fence")) &&
		     token_matches(context, writer_token);
	uint64_t previous_sequence = 0;
	if (valid)
	{
		redisReply *reply = command(context, "GET mud:world_state:current");
		if (reply && reply->type == REDIS_REPLY_STRING && reply->str)
			previous_sequence = strtoull(reply->str, nullptr, 10);
		if (reply)
			freeReplyObject(reply);
		valid = status_ok(command(context, "MULTI"));
		transaction_started = valid;
	}

	const char *commands[] = {
		"SET mud:world_state:generation:%llu %b",
		"SET mud:world_state:current %llu",
		"SET mud:world_state:timestamp %lld",
		"SET mud:world_state:sequence %llu",
		"SET mud:world_state:checksum %u",
		"SET mud:world_state:complete 1",
		"DEL mud:floor_drops",
		"PEXPIRE mud:world_state:writer_fence %llu",
	};
	redisReply *queued[8] = {};
	if (valid)
	{
		queued[0] = command(context, commands[0], (unsigned long long)sequence, data, size);
		queued[1] = command(context, commands[1], (unsigned long long)sequence);
		queued[2] = command(context, commands[2], (long long)timestamp);
		queued[3] = command(context, commands[3], (unsigned long long)sequence);
		queued[4] = command(context, commands[4], checksum);
		queued[5] = command(context, commands[5]);
		queued[6] = command(context, commands[6]);
		queued[7] = command(context, commands[7], (unsigned long long)lease_msec);
		for (redisReply *reply : queued)
			if (!reply || reply->type != REDIS_REPLY_STATUS)
				valid = false;
		for (redisReply *reply : queued)
			if (reply)
				freeReplyObject(reply);
	}

	if (valid)
	{
		redisReply *reply = command(context, "EXEC");
		valid = reply && reply->type == REDIS_REPLY_ARRAY && reply->elements == 8;
		if (valid)
			for (size_t index = 0; index < reply->elements; ++index)
				if (!reply->element[index] ||
				    reply->element[index]->type == REDIS_REPLY_ERROR)
					valid = false;
		if (reply)
			freeReplyObject(reply);
	}
	else
		end_watch(context, transaction_started);

	if (valid && previous_sequence && previous_sequence != sequence)
	{
		redisReply *reply = command(context, "DEL mud:world_state:generation:%llu",
					    (unsigned long long)previous_sequence);
		if (reply)
			freeReplyObject(reply);
	}
	redisFree(context);
	return valid;
}
