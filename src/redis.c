// redis dirty saves

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "structs.h"
#include "utils.h"
#include "prototypes.h"
#include "sql_player.h"
#include "redis.h"
#include "files.h"
#include "config.h"

#ifndef __NO_MYSQL__
#include <hiredis/hiredis.h>
#endif

extern const int top_of_world;
extern struct room_data *world;

static redisContext *redis_ctx = NULL;
bool redis_enabled = false;

#define REDIS_FLUSH_INTERVAL (5 * WAIT_SEC)

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

  redis_ctx = redisConnect(redis_host, redis_port);
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
    return true;
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

  redis_ctx = redisConnect(redis_host, redis_port);
  if (!redis_ctx)
  {
    logit(LOG_SYS, "redis: failed to allocate context");
    redis_enabled = false;
    return false;
  }

  if (redis_ctx->err)
  {
    logit(LOG_SYS, "redis connect failed: %s", redis_ctx->errstr);
    redisFree(redis_ctx);
    redis_ctx = NULL;
    redis_enabled = false;
    return false;
  }

  redis_enabled = true;
  logit(LOG_SYS, "redis connected to %s:%d, dirty saves enabled", redis_host, redis_port);

  // note: flush event scheduled in ne_init_events() after event system is ready

  return true;
#endif
}

void redis_cleanup(void)
{
#ifndef __NO_MYSQL__
  if (redis_ctx)
  {
    redisFree(redis_ctx);
    redis_ctx = NULL;
  }
  redis_enabled = false;
#endif
}

bool redis_ping(void)
{
#ifndef __NO_MYSQL__
  if (!redis_enabled || !redis_ctx)
    return false;

  redisReply *reply = (redisReply *)redisCommand(redis_ctx, "PING");
  if (!reply)
  {
    logit(LOG_DEBUG, "redis ping failed: no reply");
    return false;
  }

  bool success = (reply->type == REDIS_REPLY_STATUS &&
                  reply->str && strcasecmp(reply->str, "PONG") == 0);
  freeReplyObject(reply);
  return success;
#else
  return false;
#endif
}

void mark_player_dirty(int pid)
{
#ifndef __NO_MYSQL__
  if (!redis_enabled || pid <= 0)
    return;

  if (!redis_ctx || redis_ctx->err)
  {
    if (!redis_reconnect())
    {
      redis_enabled = false;
      P_char ch = find_player_by_pid(pid);
      if (ch && IS_PC(ch))
        sql_save_player(ch, RENT_CRASH, get_room_vnum(ch));
      return;
    }
  }

  redisReply *reply = (redisReply *)redisCommand(redis_ctx, "SADD mud:dirty_players %d", pid);
  if (!reply)
  {
    P_char ch = find_player_by_pid(pid);
    if (ch && IS_PC(ch))
      sql_save_player(ch, RENT_CRASH, get_room_vnum(ch));
    return;
  }
  freeReplyObject(reply);
#endif
}

void flush_dirty_players(void)
{
#ifndef __NO_MYSQL__
  if (!redis_enabled)
    return;

  if (!redis_ctx || redis_ctx->err)
  {
    if (!redis_reconnect())
    {
      redis_enabled = false;
      return;
    }
  }

  // smembers first, del after saves - no data loss on disconnect
  redisReply *reply = (redisReply *)redisCommand(redis_ctx, "SMEMBERS mud:dirty_players");
  if (!reply)
    return;

  if (reply->type != REDIS_REPLY_ARRAY)
  {
    freeReplyObject(reply);
    return;
  }

  int saved_count = 0;
  size_t i;
  for (i = 0; i < reply->elements; i++)
  {
    if (reply->element[i]->type != REDIS_REPLY_STRING)
      continue;

    int pid = atoi(reply->element[i]->str);
    P_char ch = find_player_by_pid(pid);
    if (ch && IS_PC(ch))
    {
      if (sql_save_player(ch, RENT_CRASH, get_room_vnum(ch)))
        saved_count++;
      else
        logit(LOG_DEBUG, "flush_dirty_players: failed to save pid %d", pid);
    }
  }

  freeReplyObject(reply);

  if (saved_count > 0)
  {
    redisReply *del_reply = (redisReply *)redisCommand(redis_ctx, "DEL mud:dirty_players");
    if (del_reply)
      freeReplyObject(del_reply);
  }
#endif
}

int get_dirty_player_count(void)
{
#ifndef __NO_MYSQL__
  if (!redis_enabled || !redis_ctx)
    return 0;

  redisReply *reply = (redisReply *)redisCommand(redis_ctx, "SCARD mud:dirty_players");
  if (!reply)
    return 0;

  int count = 0;
  if (reply->type == REDIS_REPLY_INTEGER)
    count = (int)reply->integer;

  freeReplyObject(reply);
  return count;
#else
  return 0;
#endif
}

void event_flush_dirty_players(P_char ch, P_char victim, P_obj obj, void *data)
{
  flush_dirty_players();

  if (redis_enabled)
    add_event(event_flush_dirty_players, REDIS_FLUSH_INTERVAL, NULL, NULL, NULL, 0, NULL, 0);
}
