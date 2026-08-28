#include "redis_ship_legacy.h"

#include "redis_cache_store.h"
#include "redis_command_observability.h"
#include "redis_key_registry.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#ifndef __NO_MYSQL__
#include <hiredis/hiredis.h>
#endif

namespace
{
#ifndef __NO_MYSQL__
redisReply *ship_command(redis_shared_command_kind kind, redisContext *context, const char *format,
			 ...)
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

bool ship_snapshot_scan_empty(redisContext *context)
{
	char cursor[64] = "0";
	do
	{
		redisReply *scan = ship_command(REDIS_SHARED_COMMAND_SCAN, context,
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
		snprintf(cursor, sizeof cursor, "%s", scan->element[0]->str);
		const bool empty = scan->element[1]->elements == 0;
		freeReplyObject(scan);
		if (!empty)
			return false;
	} while (strcmp(cursor, "0"));
	return true;
}
#endif
} // namespace

void redis_invalidate_ship_snapshot(const char *owner_name)
{
#ifndef __NO_MYSQL__
	if (!owner_name)
		return;
	char key[256];
	snprintf(key, sizeof key, REDIS_SHIP_SNAPSHOT_FORMAT, owner_name);
	redis_cache_store_delete(key);
#else
	(void)owner_name;
#endif
}

bool redis_clear_ship_snapshots(struct redisContext *context)
{
#ifdef __NO_MYSQL__
	(void)context;
	return true;
#else
	if (!context || context->err)
		return false;
	char cursor[64] = "0";
	do
	{
		redisReply *scan = ship_command(REDIS_SHARED_COMMAND_SCAN, context,
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
			redisReply *deleted = ship_command(REDIS_SHARED_COMMAND_WRITE, context,
							   "DEL %b", key->str, key->len);
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
	return ship_snapshot_scan_empty(context);
#endif
}
