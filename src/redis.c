// redis dirty saves and world state persistence

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "structs.h"
#include "utils.h"
#include "prototypes.h"
#include "sql_player.h"
#include "redis.h"
#include "files.h"
#include "config.h"
#include "copyover.h"

#ifndef __NO_MYSQL__
#include <hiredis/hiredis.h>
#endif

extern const int top_of_world;
extern int top_of_zone_table;
extern struct zone_data *zone_table;
extern struct room_data *world;
extern P_char character_list;
extern P_obj object_list;
extern P_index obj_index;

// ship object vnums to skip
#define VOBJ_PANEL 30098
#define VOBJ_ALL_SHIPS 30099
#define VOBJ_CARGO_CRATE 30097

static redisContext *redis_ctx = NULL;
bool redis_enabled = false;
bool redis_world_state_enabled = false;
int crash_recovery_boot = 0;

#define REDIS_FLUSH_INTERVAL (5 * WAIT_SEC)
#define REDIS_WORLD_STATE_INTERVAL_DEFAULT 30
#define REDIS_WORLD_STATE_MAX_AGE_DEFAULT 300
// buffer size now dynamically allocated based on estimated world size

static int world_state_interval = REDIS_WORLD_STATE_INTERVAL_DEFAULT;
static int world_state_max_age = REDIS_WORLD_STATE_MAX_AGE_DEFAULT;

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

  // check for world state persistence
  const char *world_state_env = getenv("REDIS_WORLD_STATE");
  if (world_state_env && strcasecmp(world_state_env, "TRUE") == 0)
  {
    redis_world_state_enabled = true;

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

    logit(LOG_SYS, "redis world state enabled: interval=%ds, max_age=%ds",
          world_state_interval, world_state_max_age);
  }

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

// world state persistence functions

bool redis_save_world_state(void)
{
#ifdef __NO_MYSQL__
  return false;
#else
  if (!redis_enabled || !redis_world_state_enabled)
    return false;

  if (!redis_ctx || redis_ctx->err)
  {
    if (!redis_reconnect())
    {
      redis_enabled = false;
      return false;
    }
  }

  // mark as invalid during save
  redisReply *valid_reply = (redisReply *)redisCommand(redis_ctx, "SET mud:world_state:valid 0");
  if (valid_reply)
    freeReplyObject(valid_reply);

  // count items
  int num_mobs, num_objs, num_rooms;
  copyover_count_items(&num_mobs, &num_objs, &num_rooms);
  int num_zones = top_of_zone_table + 1;

  // estimate buffer size needed
  size_t estimated_size = sizeof(struct copyover_header) +
                          (size_t)num_mobs * (sizeof(struct copyover_mob) + 64 * sizeof(struct copyover_affect) + 256 * sizeof(struct copyover_obj_content)) +
                          (size_t)num_objs * (sizeof(struct copyover_obj) + 64 * sizeof(struct copyover_obj_content)) +
                          (size_t)num_rooms * sizeof(struct copyover_room) +
                          (size_t)num_zones * sizeof(struct zone_age_entry);

  // allocate buffer with 25% headroom for inventory/affects
  size_t buffer_size = estimated_size + (estimated_size / 4);
  char *buffer = (char *)malloc(buffer_size);
  if (!buffer)
  {
    logit(LOG_SYS, "redis: failed to allocate world state buffer (%zu bytes)", buffer_size);
    return false;
  }

  logit(LOG_SYS, "redis: saving world state (%d mobs, %d objs, %d doors, %d zones, %zu bytes)",
        num_mobs, num_objs, num_rooms, num_zones, buffer_size);

  size_t offset = 0;

  // write header
  struct copyover_header header;
  memset(&header, 0, sizeof(header));
  memcpy(header.magic, COPYOVER_MAGIC, 4);
  header.version = COPYOVER_VERSION;
  header.timestamp = time(NULL);
  header.num_descriptors = 0;  // not saving descriptors for crash recovery
  header.num_mobs = num_mobs;
  header.num_objects = num_objs;
  header.num_rooms = num_rooms;
  header.num_combat = 0;
  header.num_zones = num_zones;

  memcpy(buffer + offset, &header, sizeof(header));
  offset += sizeof(header);

  // write mobs
  int mobs_written = 0;
  for (P_char ch = character_list; ch; ch = ch->next)
  {
    if (IS_NPC(ch) && ch->in_room >= 0 && !IS_PC_PET(ch))
    {
      int written = copyover_write_mob_to_buffer(ch, buffer + offset, buffer_size - offset);
      if (written < 0)
      {
        logit(LOG_SYS, "redis: buffer overflow writing mob");
        free(buffer);
        return false;
      }
      offset += written;
      mobs_written++;
    }
  }

  // write objects
  int objs_written = 0;
  for (P_obj obj = object_list; obj; obj = obj->next)
  {
    if (OBJ_ROOM(obj))
    {
      int vnum = OBJ_VNUM(obj);
      if (vnum == VOBJ_PANEL || vnum == VOBJ_ALL_SHIPS || vnum == VOBJ_CARGO_CRATE)
        continue;

      int written = copyover_write_obj_to_buffer(obj, buffer + offset, buffer_size - offset);
      if (written < 0)
      {
        logit(LOG_SYS, "redis: buffer overflow writing obj");
        free(buffer);
        return false;
      }
      offset += written;
      objs_written++;
    }
  }

  // write doors
  int doors_written = 0;
  for (int room = 0; room <= top_of_world; room++)
  {
    for (int dir = 0; dir < NUM_EXITS; dir++)
    {
      if (world[room].dir_option[dir] &&
          IS_SET(world[room].dir_option[dir]->exit_info, EX_ISDOOR))
      {
        int written = copyover_write_door_to_buffer(room, dir, buffer + offset, buffer_size - offset);
        if (written < 0)
        {
          logit(LOG_SYS, "redis: buffer overflow writing door");
          free(buffer);
          return false;
        }
        offset += written;
        doors_written++;
      }
    }
  }

  // write zone ages
  int zones_written = 0;
  for (int z = 0; z <= top_of_zone_table; z++)
  {
    int written = copyover_write_zone_age_to_buffer(z, buffer + offset, buffer_size - offset);
    if (written < 0)
    {
      logit(LOG_SYS, "redis: buffer overflow writing zone age");
      free(buffer);
      return false;
    }
    offset += written;
    zones_written++;
  }

  // update header with actual counts (may differ from estimates)
  header.num_mobs = mobs_written;
  header.num_objects = objs_written;
  header.num_rooms = doors_written;
  header.num_zones = zones_written;
  memcpy(buffer, &header, sizeof(header));

  // save to redis as binary
  redisReply *reply = (redisReply *)redisCommand(redis_ctx, "SET mud:world_state %b", buffer, offset);
  free(buffer);

  if (!reply)
  {
    logit(LOG_SYS, "redis: failed to save world state");
    return false;
  }
  freeReplyObject(reply);

  // save timestamp
  char timestamp_str[32];
  snprintf(timestamp_str, sizeof(timestamp_str), "%ld", (long)time(NULL));
  reply = (redisReply *)redisCommand(redis_ctx, "SET mud:world_state:timestamp %s", timestamp_str);
  if (reply)
    freeReplyObject(reply);

  // mark as valid
  reply = (redisReply *)redisCommand(redis_ctx, "SET mud:world_state:valid 1");
  if (reply)
    freeReplyObject(reply);

  logit(LOG_DEBUG, "redis: saved world state (%zu bytes, %d mobs, %d objs, %d doors, %d zones)",
        offset, mobs_written, objs_written, doors_written, zones_written);

  return true;
#endif
}

bool redis_has_world_state(void)
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

  // check valid flag
  redisReply *reply = (redisReply *)redisCommand(redis_ctx, "GET mud:world_state:valid");
  if (!reply)
    return false;

  bool valid = false;
  if (reply->type == REDIS_REPLY_STRING && reply->str && strcmp(reply->str, "1") == 0)
    valid = true;

  freeReplyObject(reply);

  if (!valid)
    return false;

  // check timestamp
  reply = (redisReply *)redisCommand(redis_ctx, "GET mud:world_state:timestamp");
  if (!reply)
    return false;

  time_t saved_time = 0;
  if (reply->type == REDIS_REPLY_STRING && reply->str)
    saved_time = (time_t)atol(reply->str);

  freeReplyObject(reply);

  if (saved_time == 0)
    return false;

  time_t now = time(NULL);
  if (now - saved_time > world_state_max_age)
  {
    logit(LOG_SYS, "redis: world state too old (%ld seconds), ignoring", (long)(now - saved_time));
    redis_clear_world_state();
    return false;
  }

  // check data exists
  reply = (redisReply *)redisCommand(redis_ctx, "EXISTS mud:world_state");
  if (!reply)
    return false;

  bool exists = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
  freeReplyObject(reply);

  return exists;
#endif
}

time_t redis_world_state_timestamp(void)
{
#ifdef __NO_MYSQL__
  return 0;
#else
  if (!redis_enabled || !redis_ctx)
    return 0;

  redisReply *reply = (redisReply *)redisCommand(redis_ctx, "GET mud:world_state:timestamp");
  if (!reply)
    return 0;

  time_t ts = 0;
  if (reply->type == REDIS_REPLY_STRING && reply->str)
    ts = (time_t)atol(reply->str);

  freeReplyObject(reply);
  return ts;
#endif
}

void redis_clear_world_state(void)
{
#ifndef __NO_MYSQL__
  if (!redis_enabled || !redis_ctx)
    return;

  redisReply *reply = (redisReply *)redisCommand(redis_ctx,
                                                  "DEL mud:world_state mud:world_state:timestamp mud:world_state:valid");
  if (reply)
    freeReplyObject(reply);

  logit(LOG_SYS, "redis: cleared world state snapshot");
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

  redisReply *reply = (redisReply *)redisCommand(redis_ctx, "GET mud:world_state");
  if (!reply || redis_ctx->err) {
    if (reply)
      freeReplyObject(reply);
    return false;
  }

  if (reply->type != REDIS_REPLY_STRING || !reply->str || reply->len == 0)
  {
    freeReplyObject(reply);
    return false;
  }

  const char *buffer = reply->str;
  size_t len = reply->len;
  size_t offset = 0;

  // read header
  if (len < sizeof(struct copyover_header))
  {
    freeReplyObject(reply);
    return false;
  }

  struct copyover_header header;
  memcpy(&header, buffer + offset, sizeof(header));
  offset += sizeof(header);

  if (memcmp(header.magic, COPYOVER_MAGIC, 4) != 0 ||
      header.version != COPYOVER_VERSION)
  {
    logit(LOG_SYS, "redis: world state version mismatch, ignoring");
    freeReplyObject(reply);
    redis_clear_world_state();
    return false;
  }

  logit(LOG_SYS, "redis: restoring world state (%d mobs, %d objs, %d doors, %d zones)",
        header.num_mobs, header.num_objects, header.num_rooms, header.num_zones);

  // restore mobs
  int mobs_restored = 0;
  for (int i = 0; i < header.num_mobs; i++)
  {
    size_t bytes_read = 0;
    P_char mob = copyover_restore_mob_from_buffer(buffer + offset, len - offset, &bytes_read);
    if (bytes_read == 0) {
      logit(LOG_SYS, "redis: unexpected end of data at mob %d/%d", i, header.num_mobs);
      break;
    }
    offset += bytes_read;
    if (mob)
      mobs_restored++;
  }

  // restore objects
  int objs_restored = 0;
  for (int i = 0; i < header.num_objects; i++)
  {
    size_t bytes_read = 0;
    P_obj obj = copyover_restore_obj_from_buffer(buffer + offset, len - offset, &bytes_read);
    if (bytes_read == 0) {
      logit(LOG_SYS, "redis: unexpected end of data at obj %d/%d", i, header.num_objects);
      break;
    }
    offset += bytes_read;
    if (obj)
      objs_restored++;
  }

  // restore doors
  int doors_restored = 0;
  for (int i = 0; i < header.num_rooms; i++)
  {
    size_t bytes_read = 0;
    int result = copyover_restore_door_from_buffer(buffer + offset, len - offset, &bytes_read);
    if (bytes_read == 0) {
      logit(LOG_SYS, "redis: unexpected end of data at door %d/%d", i, header.num_rooms);
      break;
    }
    offset += bytes_read;
    if (result == 0)
      doors_restored++;
  }

  // restore zone ages
  int zones_restored = 0;
  for (int i = 0; i < header.num_zones; i++)
  {
    size_t bytes_read = 0;
    int result = copyover_restore_zone_age_from_buffer(buffer + offset, len - offset, &bytes_read);
    if (bytes_read == 0) {
      logit(LOG_SYS, "redis: unexpected end of data at zone %d/%d", i, header.num_zones);
      break;
    }
    offset += bytes_read;
    if (result == 0)
      zones_restored++;
  }

  freeReplyObject(reply);

  logit(LOG_SYS, "redis: world state restored (%d mobs, %d objs, %d doors, %d zones)",
        mobs_restored, objs_restored, doors_restored, zones_restored);

  return true;
#endif
}

void event_save_world_state(P_char ch, P_char victim, P_obj obj, void *data)
{
  if (redis_enabled && redis_world_state_enabled) {
    redis_save_world_state();
    add_event(event_save_world_state, world_state_interval * WAIT_SEC, NULL, NULL, NULL, 0, NULL, 0);
  }
}
