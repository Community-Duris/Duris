#include "redis_maintenance.h"

#include "redis_command_observability.h"
#include "redis_connection.h"
#include "redis_key_registry.h"
#include "redis_namespace.h"
#include "redis_ship_legacy.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#ifndef __NO_REDIS__
#include <hiredis/hiredis.h>
#endif

namespace
{
#ifndef __NO_REDIS__
redisReply *maintenance_command(redis_shared_command_kind kind, redisContext *context,
				const char *format, ...)
{
	const uint64_t started_usec = redis_observability_now_usec();
	redisReply *reply = NULL;
	if (context && !context->err)
	{
		va_list arguments;
		va_start(arguments, format);
		reply = static_cast<redisReply *>(redisvCommand(context, format, arguments));
		va_end(arguments);
	}
	const uint64_t finished_usec = redis_observability_now_usec();
	const uint64_t duration_usec =
		finished_usec >= started_usec ? finished_usec - started_usec : 0;
	redis_shared_command_outcome outcome = REDIS_SHARED_OUTCOME_SUCCESS;
	if (!context || context->err)
		outcome = redis_command_outcome(context, false);
	else if (!reply)
		outcome = REDIS_SHARED_OUTCOME_NO_REPLY;
	else if (reply->type == REDIS_REPLY_ERROR)
		outcome = REDIS_SHARED_OUTCOME_ERROR_REPLY;
	redis_shared_command_observability_record(REDIS_SHARED_SCOPE_MAINTENANCE, kind, outcome,
						  duration_usec);
	if (outcome == REDIS_SHARED_OUTCOME_SUCCESS)
		return reply;
	if (reply)
		freeReplyObject(reply);
	return NULL;
}

bool scan_match_empty(redisContext *context, const char *pattern)
{
	char cursor[64] = "0";
	if (!context || context->err || !pattern || !*pattern)
		return false;
	do
	{
		redisReply *scan = maintenance_command(REDIS_SHARED_COMMAND_SCAN, context,
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
		snprintf(cursor, sizeof cursor, "%s", scan->element[0]->str);
		const bool empty = scan->element[1]->elements == 0;
		freeReplyObject(scan);
		if (!empty)
			return false;
	} while (strcmp(cursor, "0"));
	return true;
}

bool clear_scan_match(redisContext *context, const char *pattern)
{
	char cursor[64] = "0";
	if (!context || context->err || !pattern || !*pattern)
		return false;
	do
	{
		redisReply *scan = maintenance_command(REDIS_SHARED_COMMAND_SCAN, context,
						       "SCAN %s MATCH %s COUNT 256", cursor,
						       pattern);
		if (!scan || scan->type != REDIS_REPLY_ARRAY || scan->elements != 2 ||
		    !scan->element[0] || !scan->element[1] || !scan->element[0]->str ||
		    scan->element[0]->type != REDIS_REPLY_STRING ||
		    scan->element[1]->type != REDIS_REPLY_ARRAY)
		{
			if (scan)
				freeReplyObject(scan);
			return false;
		}
		snprintf(cursor, sizeof cursor, "%s", scan->element[0]->str);
		redisReply *keys = scan->element[1];
		for (size_t index = 0; index < keys->elements; ++index)
		{
			redisReply *key = keys->element[index];
			if (!key || key->type != REDIS_REPLY_STRING || !key->str)
			{
				freeReplyObject(scan);
				return false;
			}
			redisReply *deleted = maintenance_command(
				REDIS_SHARED_COMMAND_WRITE, context, "DEL %b", key->str, key->len);
			if (!deleted || (deleted->type != REDIS_REPLY_INTEGER &&
					 deleted->type != REDIS_REPLY_NIL))
			{
				if (deleted)
					freeReplyObject(deleted);
				freeReplyObject(scan);
				return false;
			}
			freeReplyObject(deleted);
		}
		freeReplyObject(scan);
	} while (strcmp(cursor, "0"));
	return scan_match_empty(context, pattern);
}

bool delete_key_checked(redisContext *context, const char *key)
{
	if (!context || context->err || !key || !*key)
		return false;
	redisReply *reply = maintenance_command(REDIS_SHARED_COMMAND_WRITE, context, "DEL %s", key);
	const bool deleted = reply && reply->type == REDIS_REPLY_INTEGER;
	if (reply)
		freeReplyObject(reply);
	if (!deleted)
		return false;
	reply = maintenance_command(REDIS_SHARED_COMMAND_READ, context, "EXISTS %s", key);
	const bool absent = reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 0;
	if (reply)
		freeReplyObject(reply);
	return absent;
}

bool delete_pair_checked(redisContext *context, const char *first, const char *second)
{
	redisReply *reply = maintenance_command(REDIS_SHARED_COMMAND_WRITE, context, "DEL %s %s",
						first, second);
	const bool deleted = reply && reply->type == REDIS_REPLY_INTEGER;
	if (reply)
		freeReplyObject(reply);
	if (!deleted)
		return false;
	reply = maintenance_command(REDIS_SHARED_COMMAND_READ, context, "EXISTS %s %s", first,
				    second);
	const bool absent = reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 0;
	if (reply)
		freeReplyObject(reply);
	return absent;
}

bool scoped_key(const redis_maintenance_config *config, const char *suffix, char *buffer,
		size_t size)
{
	return config && redis_namespace_season_key(config->key_namespace, config->season_epoch,
						    suffix, buffer, size);
}

bool config_valid(const redis_maintenance_config *config)
{
	return config && config->connection && config->key_namespace && *config->key_namespace &&
	       config->season_epoch && config->presence_current_key &&
	       *config->presence_current_key && config->presence_session_pattern &&
	       *config->presence_session_pattern && config->presence_retry_pattern &&
	       *config->presence_retry_pattern && config->report_cache_pattern &&
	       *config->report_cache_pattern;
}
#endif
} // namespace

bool redis_maintenance_clear(const redis_maintenance_config *config)
{
#ifdef __NO_REDIS__
	(void)config;
	return true;
#else
	if (!config_valid(config))
		return false;
	char world_generation_pattern[160];
	char world_current_key[128];
	char world_timestamp_key[128];
	char world_sequence_key[128];
	char world_checksum_key[128];
	char world_complete_key[128];
	char world_clean_shutdown_key[128];
	char floor_drops_key[128];
	char floor_drop_index_key[128];
	if (!scoped_key(config, REDIS_WORLD_GENERATION_PATTERN, world_generation_pattern,
			sizeof world_generation_pattern) ||
	    !scoped_key(config, REDIS_WORLD_CURRENT_SUFFIX, world_current_key,
			sizeof world_current_key) ||
	    !scoped_key(config, REDIS_WORLD_TIMESTAMP_SUFFIX, world_timestamp_key,
			sizeof world_timestamp_key) ||
	    !scoped_key(config, REDIS_WORLD_SEQUENCE_SUFFIX, world_sequence_key,
			sizeof world_sequence_key) ||
	    !scoped_key(config, REDIS_WORLD_CHECKSUM_SUFFIX, world_checksum_key,
			sizeof world_checksum_key) ||
	    !scoped_key(config, REDIS_WORLD_COMPLETE_SUFFIX, world_complete_key,
			sizeof world_complete_key) ||
	    !scoped_key(config, REDIS_WORLD_CLEAN_SHUTDOWN_SUFFIX, world_clean_shutdown_key,
			sizeof world_clean_shutdown_key) ||
	    !scoped_key(config, REDIS_FLOOR_DROPS_SUFFIX, floor_drops_key,
			sizeof floor_drops_key) ||
	    !scoped_key(config, REDIS_FLOOR_DROP_INDEX_SUFFIX, floor_drop_index_key,
			sizeof floor_drop_index_key))
		return false;

	redisContext *context = redis_connection_open(config->connection);
	if (!context || context->err)
	{
		redis_shared_command_observability_record(REDIS_SHARED_SCOPE_MAINTENANCE,
							  REDIS_SHARED_COMMAND_WRITE,
							  redis_command_outcome(context, false), 0);
		if (context)
			redisFree(context);
		return false;
	}
	const bool cleared = clear_scan_match(context, world_generation_pattern) &&
			     delete_key_checked(context, world_current_key) &&
			     delete_key_checked(context, world_timestamp_key) &&
			     delete_key_checked(context, world_sequence_key) &&
			     delete_key_checked(context, world_checksum_key) &&
			     delete_key_checked(context, world_complete_key) &&
			     delete_key_checked(context, world_clean_shutdown_key) &&
			     delete_pair_checked(context, floor_drops_key, floor_drop_index_key) &&
			     delete_key_checked(context, REDIS_LEGACY_FLOOR_DROPS) &&
			     delete_key_checked(context, REDIS_LEGACY_FLOOR_PICKUPS) &&
			     delete_key_checked(context, REDIS_LEGACY_ONLINE) &&
			     delete_key_checked(context, config->presence_current_key) &&
			     clear_scan_match(context, config->presence_session_pattern) &&
			     clear_scan_match(context, config->presence_retry_pattern) &&
			     delete_key_checked(context, REDIS_LEGACY_PRESENCE_CURRENT) &&
			     clear_scan_match(context, REDIS_LEGACY_PRESENCE_SESSION_PATTERN) &&
			     clear_scan_match(context, REDIS_LEGACY_PRESENCE_RETRY_PATTERN) &&
			     clear_scan_match(context, REDIS_LEGACY_WORLD_GENERATION_PATTERN) &&
			     delete_key_checked(context, REDIS_LEGACY_WORLD_CURRENT) &&
			     delete_key_checked(context, REDIS_LEGACY_WORLD_TIMESTAMP) &&
			     delete_key_checked(context, REDIS_LEGACY_WORLD_SEQUENCE) &&
			     delete_key_checked(context, REDIS_LEGACY_WORLD_CHECKSUM) &&
			     delete_key_checked(context, REDIS_LEGACY_WORLD_COMPLETE) &&
			     delete_key_checked(context, REDIS_LEGACY_WORLD_FENCE) &&
			     clear_scan_match(context, config->report_cache_pattern) &&
			     clear_scan_match(context, REDIS_LEGACY_CACHE_PATTERN) &&
			     redis_clear_ship_snapshots(context);
	redisFree(context);
	return cleared;
#endif
}

bool redis_maintenance_validate(const redis_maintenance_config *config)
{
#ifdef __NO_REDIS__
	(void)config;
	return true;
#else
	if (!config_valid(config))
		return false;
	redisContext *context = redis_connection_open(config->connection);
	redisReply *reply = maintenance_command(REDIS_SHARED_COMMAND_READ, context, "PING");
	const bool ready = reply && reply->type == REDIS_REPLY_STATUS && reply->str &&
			   !strcmp(reply->str, "PONG");
	if (reply)
		freeReplyObject(reply);
	if (context)
		redisFree(context);
	return ready;
#endif
}
