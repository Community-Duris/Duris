// redis dirty saves and world state persistence

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

#include "structs.h"
#include "sql.h"
#include "utils.h"
#include "prototypes.h"
#include "sql_player.h"
#include "redis.h"
#include "files.h"
#include "config.h"
#include "copyover.h"
#include "db.h"
#include "spells.h"
#include "utility.h"
#include "epic.h"

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

// ship object vnums defined in ships/ships.h

static redisContext *redis_ctx = NULL;
bool redis_enabled = false;
bool redis_world_state_enabled = false;
int crash_recovery_boot = 0;

#define REDIS_FLUSH_INTERVAL (5 * WAIT_SEC)
#define REDIS_WORLD_STATE_INTERVAL_DEFAULT 30
#define REDIS_WORLD_STATE_MAX_AGE_DEFAULT 300

static int world_state_interval = REDIS_WORLD_STATE_INTERVAL_DEFAULT;
static int world_state_max_age = REDIS_WORLD_STATE_MAX_AGE_DEFAULT;
static volatile pid_t world_state_save_pid = 0;
static volatile pid_t dirty_flush_pid = 0;

#define DIRTY_FLUSH_THRESHOLD 0

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

  // load obj_uid counter from redis
  redis_load_obj_uid_counter();

  return true;
#endif
}

void redis_cleanup(void)
{
#ifndef __NO_MYSQL__
  if (redis_ctx)
  {
    // save obj_uid counter before disconnect
    redis_save_obj_uid_counter();
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

void redis_save_obj_uid_counter(void)
{
#ifndef __NO_MYSQL__
  if (!redis_enabled || !redis_ctx)
    return;

  redisReply *reply = (redisReply *)redisCommand(redis_ctx, "SET mud:next_obj_uid %lu", next_obj_uid);
  if (reply)
    freeReplyObject(reply);
#endif
}

void redis_load_obj_uid_counter(void)
{
#ifndef __NO_MYSQL__
  if (!redis_enabled || !redis_ctx)
    return;

  redisReply *reply = (redisReply *)redisCommand(redis_ctx, "GET mud:next_obj_uid");
  if (reply && reply->type == REDIS_REPLY_STRING && reply->str)
  {
    unsigned long loaded = strtoul(reply->str, NULL, 10);
    if (loaded > next_obj_uid)
    {
      next_obj_uid = loaded;
      logit(LOG_SYS, "redis: loaded obj_uid counter = %lu", next_obj_uid);
    }
  }
  if (reply)
    freeReplyObject(reply);
#endif
}

void redis_log_floor_pickup(unsigned long obj_uid)
{
#ifndef __NO_MYSQL__
  if (!redis_enabled || !redis_ctx || obj_uid == 0)
    return;

  redisReply *reply = (redisReply *)redisCommand(redis_ctx, "SADD mud:floor_pickups %lu", obj_uid);
  if (reply)
    freeReplyObject(reply);
#endif
}

bool redis_check_floor_pickup(unsigned long obj_uid)
{
#ifndef __NO_MYSQL__
  if (!redis_enabled || !redis_ctx || obj_uid == 0)
    return false;

  redisReply *reply = (redisReply *)redisCommand(redis_ctx, "SISMEMBER mud:floor_pickups %lu", obj_uid);
  bool found = (reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
  if (reply)
    freeReplyObject(reply);
  return found;
#else
  return false;
#endif
}

void redis_clear_floor_pickups(void)
{
#ifndef __NO_MYSQL__
  if (!redis_enabled || !redis_ctx)
    return;

  redisReply *reply = (redisReply *)redisCommand(redis_ctx, "DEL mud:floor_pickups");
  if (reply)
    freeReplyObject(reply);
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
        sql_save_player(ch, RENT_CRASH, 0);
      return;
    }
  }

  redisReply *reply = (redisReply *)redisCommand(redis_ctx, "SADD mud:dirty_players %d", pid);
  if (!reply)
  {
    P_char ch = find_player_by_pid(pid);
    if (ch && IS_PC(ch))
      sql_save_player(ch, RENT_CRASH, 0);
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

  // skip if previous async save still running
  if (dirty_flush_pid > 0)
  {
    int status;
    pid_t result = waitpid(dirty_flush_pid, &status, WNOHANG);
    if (result == 0)
      return;
    dirty_flush_pid = 0;
  }

  if (!redis_ctx || redis_ctx->err)
  {
    if (!redis_reconnect())
    {
      redis_enabled = false;
      return;
    }
  }

  redisReply *reply = (redisReply *)redisCommand(redis_ctx, "SMEMBERS mud:dirty_players");
  if (!reply)
    return;

  if (reply->type != REDIS_REPLY_ARRAY || reply->elements == 0)
  {
    freeReplyObject(reply);
    return;
  }

  // copy pids to array
  int count = (int)reply->elements;
  int *pids = (int *)malloc(count * sizeof(int));
  if (!pids)
  {
    freeReplyObject(reply);
    return;
  }

  int valid = 0;
  for (size_t i = 0; i < reply->elements; i++)
  {
    if (reply->element[i]->type == REDIS_REPLY_STRING)
    {
      int pid = atoi(reply->element[i]->str);
      // only keep pids of players actually online
      P_char ch = find_player_by_pid(pid);
      if (ch && IS_PC(ch))
        pids[valid++] = pid;
    }
  }
  freeReplyObject(reply);

  if (valid == 0)
  {
    free(pids);
    return;
  }

  // clear dirty set now so stale entries don't accumulate
  redisReply *del = (redisReply *)redisCommand(redis_ctx, "DEL mud:dirty_players");
  if (del)
    freeReplyObject(del);

  // small batch - save sync
  if (valid <= DIRTY_FLUSH_THRESHOLD)
  {
    for (int i = 0; i < valid; i++)
    {
      P_char ch = find_player_by_pid(pids[i]);
      if (ch && IS_PC(ch))
        sql_save_player(ch, RENT_CRASH, get_room_vnum(ch));
    }
    free(pids);
    return;
  }

  // fork for async save
  logit(LOG_SYS, "flush_dirty: saving %d online players async", valid);
  pid_t pid = fork();
  if (pid < 0)
  {
    // fork failed, fallback to sync
    logit(LOG_SYS, "flush_dirty: fork failed, saving sync");
    for (int i = 0; i < valid; i++)
    {
      P_char ch = find_player_by_pid(pids[i]);
      if (ch && IS_PC(ch))
        sql_save_player(ch, RENT_CRASH, get_room_vnum(ch));
    }
    free(pids);
    return;
  }

  if (pid == 0)
  {
    // child - create own mysql connection
    MYSQL *child_conn = sql_create_child_connection();
    if (!child_conn)
    {
      free(pids);
      _exit(1);
    }

    sql_reset_for_child(child_conn);

    for (int i = 0; i < valid; i++)
    {
      P_char ch = find_player_by_pid(pids[i]);
      if (ch && IS_PC(ch))
        sql_save_player(ch, RENT_CRASH, get_room_vnum(ch));
    }

    mysql_close(child_conn);
    free(pids);
    _exit(0);
  }

  // parent
  dirty_flush_pid = pid;
  free(pids);
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

// sync version, called by forked child - needs own redis connection
static bool redis_save_world_state_sync(void)
{
#ifdef __NO_MYSQL__
  return false;
#else
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

  redisContext *ctx = redisConnect(redis_host, redis_port);
  if (!ctx || ctx->err)
  {
    if (ctx)
      redisFree(ctx);
    return false;
  }

  redisReply *valid_reply = (redisReply *)redisCommand(ctx, "SET mud:world_state:valid 0");
  if (valid_reply)
    freeReplyObject(valid_reply);

  int num_mobs, num_objs, num_rooms;
  copyover_count_items(&num_mobs, &num_objs, &num_rooms);
  int num_zones = top_of_zone_table + 1;

  size_t estimated_size = sizeof(struct copyover_header) +
                          (size_t)num_mobs * (sizeof(struct copyover_mob) + 64 * sizeof(struct copyover_affect) + 256 * sizeof(struct copyover_obj_content)) +
                          (size_t)num_objs * (sizeof(struct copyover_obj) + 64 * sizeof(struct copyover_obj_content)) +
                          (size_t)num_rooms * sizeof(struct copyover_room) +
                          (size_t)num_zones * sizeof(struct zone_age_entry);

  size_t buffer_size = estimated_size + (estimated_size / 4); // 25% headroom
  char *buffer = (char *)malloc(buffer_size);
  if (!buffer)
  {
    redisFree(ctx);
    return false;
  }

  size_t offset = 0;

  struct copyover_header header;
  memset(&header, 0, sizeof(header));
  memcpy(header.magic, COPYOVER_MAGIC, 4);
  header.version = COPYOVER_VERSION;
  header.timestamp = time(NULL);
  header.num_descriptors = 0;
  header.num_mobs = num_mobs;
  header.num_objects = num_objs;
  header.num_rooms = num_rooms;
  header.num_combat = 0;
  header.num_zones = num_zones;

  memcpy(buffer + offset, &header, sizeof(header));
  offset += sizeof(header);

  int mobs_written = 0;
  for (P_char ch = character_list; ch; ch = ch->next)
  {
    if (IS_NPC(ch) && ch->in_room >= 0 && !IS_PC_PET(ch))
    {
      int written = copyover_write_mob_to_buffer(ch, buffer + offset, buffer_size - offset);
      if (written < 0)
      {
        free(buffer);
        redisFree(ctx);
        return false;
      }
      offset += written;
      mobs_written++;
    }
  }

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
        free(buffer);
        redisFree(ctx);
        return false;
      }
      offset += written;
      objs_written++;
    }
  }

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
          free(buffer);
          redisFree(ctx);
          return false;
        }
        offset += written;
        doors_written++;
      }
    }
  }

  int zones_written = 0;
  for (int z = 0; z <= top_of_zone_table; z++)
  {
    int written = copyover_write_zone_age_to_buffer(z, buffer + offset, buffer_size - offset);
    if (written < 0)
    {
      free(buffer);
      redisFree(ctx);
      return false;
    }
    offset += written;
    zones_written++;
  }

  header.num_mobs = mobs_written;
  header.num_objects = objs_written;
  header.num_rooms = doors_written;
  header.num_zones = zones_written;
  memcpy(buffer, &header, sizeof(header));

  redisReply *reply = (redisReply *)redisCommand(ctx, "SET mud:world_state %b", buffer, offset);
  free(buffer);

  if (!reply)
  {
    redisFree(ctx);
    return false;
  }
  freeReplyObject(reply);

  char timestamp_str[32];
  snprintf(timestamp_str, sizeof(timestamp_str), "%ld", (long)time(NULL));
  reply = (redisReply *)redisCommand(ctx, "SET mud:world_state:timestamp %s", timestamp_str);
  if (reply)
    freeReplyObject(reply);

  reply = (redisReply *)redisCommand(ctx, "SET mud:world_state:valid 1");
  if (reply)
    freeReplyObject(reply);

  redisFree(ctx);
  return true;
#endif
}

// forks child to avoid blocking main loop
bool redis_save_world_state(void)
{
#ifdef __NO_MYSQL__
  return false;
#else
  if (!redis_enabled || !redis_world_state_enabled)
    return false;

  if (world_state_save_pid > 0)
  {
    int status;
    pid_t result = waitpid(world_state_save_pid, &status, WNOHANG);
    if (result == 0)
    {
      // still running
      logit(LOG_DEBUG, "redis: world state save still in progress, skipping");
      return true;
    }
    world_state_save_pid = 0;
  }

  int num_mobs, num_objs, num_rooms;
  copyover_count_items(&num_mobs, &num_objs, &num_rooms);
  int num_zones = top_of_zone_table + 1;

  logit(LOG_SYS, "redis: saving world state (async, %d mobs, %d objs, %d doors, %d zones)",
        num_mobs, num_objs, num_rooms, num_zones);

  pid_t pid = fork();
  if (pid < 0)
  {
    logit(LOG_SYS, "redis: fork failed for world state save");
    return false;
  }

  if (pid == 0)
  {
    bool success = redis_save_world_state_sync();
    _exit(success ? 0 : 1);
  }

  world_state_save_pid = pid;
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
  int objs_skipped = 0;
  for (int i = 0; i < header.num_objects; i++)
  {
    // peek at obj_uid to check if it was picked up before crash
    struct copyover_obj peek_entry;
    if (len - offset >= sizeof(peek_entry))
    {
      memcpy(&peek_entry, buffer + offset, sizeof(peek_entry));
      if (peek_entry.obj_uid > 0 && redis_check_floor_pickup(peek_entry.obj_uid))
      {
        // skip this object - it was picked up before crash
        logit(LOG_SYS, "redis: skipping picked up floor item uid %lu vnum %d",
              peek_entry.obj_uid, peek_entry.vnum);
        // consume the bytes for this object entry and its contents
        offset += sizeof(peek_entry);
        offset += peek_entry.num_contents * sizeof(struct copyover_obj_content);
        objs_skipped++;
        continue;
      }
    }

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

  // clear pickup log after restore
  if (objs_skipped > 0)
    logit(LOG_SYS, "redis: skipped %d items that were picked up before crash", objs_skipped);
  redis_clear_floor_pickups();

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

bool redis_cache_set(const char *key, const char *value)
{
#ifdef __NO_MYSQL__
  return false;
#else
  if (!redis_enabled || !redis_ctx || !key || !value)
    return false;

  redisReply *reply = (redisReply *)redisCommand(redis_ctx, "SET %s %b", key, value, strlen(value));
  if (!reply)
    return false;

  bool ok = (reply->type == REDIS_REPLY_STATUS);
  freeReplyObject(reply);
  return ok;
#endif
}

bool redis_cache_set_ex(const char *key, int seconds, const char *value)
{
#ifdef __NO_MYSQL__
  return false;
#else
  if (!redis_enabled || !redis_ctx || !key || !value || seconds <= 0)
    return false;

  redisReply *reply = (redisReply *)redisCommand(redis_ctx, "SETEX %s %d %b", key, seconds, value, strlen(value));
  if (!reply)
    return false;

  bool ok = (reply->type == REDIS_REPLY_STATUS);
  freeReplyObject(reply);
  return ok;
#endif
}

char *redis_cache_get(const char *key)
{
#ifdef __NO_MYSQL__
  return NULL;
#else
  if (!redis_enabled || !redis_ctx || !key)
    return NULL;

  redisReply *reply = (redisReply *)redisCommand(redis_ctx, "GET %s", key);
  if (!reply)
    return NULL;

  char *result = NULL;
  if (reply->type == REDIS_REPLY_STRING && reply->str)
    result = strdup(reply->str);

  freeReplyObject(reply);
  return result;
#endif
}

void redis_cache_del(const char *key)
{
#ifndef __NO_MYSQL__
  if (!redis_enabled || !redis_ctx || !key)
    return;

  redisReply *reply = (redisReply *)redisCommand(redis_ctx, "DEL %s", key);
  if (reply)
    freeReplyObject(reply);
#endif
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
  strcat(output, "         &+W*&n caster level of the spell(s) is based on number of items\n");
  strcat(output, "           going over set requirements will increase caster level\n");
  strcat(output, "         &+W*&n &+Gthese&n spells have a cooldown of 1 minute\n");
  strcat(output, "           &+ythese&n spells have a cooldown of 5 minutes\n\n");
  strcat(output, "&+Y ZONE NAME                                        &+W|&+B SPELLS GRANTED &+W(&+Ypieces required&n&+W)&n\n");
  strcat(output, "&+W-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=&n\n");

  for (int i = 0; zones_random_data[i].zone; i++)
  {
    int zone_id = real_zone(zones_random_data[i].zone);
    if (zone_id <= 0)
      continue;

    const char *zone_name = zone_table[zone_id].name;
    snprintf(buffer, sizeof(buffer), " %s    &+W|&n ", pad_ansi(zone_name, 45, FALSE).c_str());

    if (zones_random_data[i].proc_spells[0][0] == 0)
    {
      strcat(buffer, "&+LNONE&n");
      strcat(buffer, "\n");
      strcat(output, buffer);
      continue;
    }

    for (int x = 0; x < 3; x++)
    {
      if (zones_random_data[i].proc_spells[x][0] != 0 && zones_random_data[i].proc_spells[x][1] <= MAX_AFFECT_TYPES)
      {
        char buf[256];
        const char *spellColor = "&+B";

        if (zones_random_data[i].proc_spells[x][1] == SPELL_STONE_SKIN ||
            zones_random_data[i].proc_spells[x][1] == SPELL_INVIGORATE)
          spellColor = "&+G";
        else if (zones_random_data[i].proc_spells[x][1] == SPELL_CONJURE_ELEMENTAL)
          spellColor = "&+y";

        snprintf(buf, sizeof(buf), "%s%s%s &+W(&+Y%d&+W)&n",
                 x != 0 ? "&+W,&n " : "",
                 spellColor,
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
  if (!redis_enabled)
    return;

  char *report = generate_named_report();
  if (report)
  {
    redis_cache_set("mud:cache:named", report);
    free(report);
    logit(LOG_SYS, "redis: cached named report");
  }
#endif
}

char *redis_get_named_report(void)
{
  return redis_cache_get("mud:cache:named");
}

// fraglist cache
extern void get_level_cap_info(long *max_frags, int *racewar, int *level, time_t *next_update);
extern int sql_level_cap(int racewar_side);
extern const racewar_struct racewar_color[];

#define MAX_FRAG_SIZE 10

static char *generate_fraglist_output(void)
{
#ifdef __NO_MYSQL__
  return NULL;
#else
  char *output = (char *)malloc(65536);
  if (!output)
    return NULL;

  output[0] = '\0';
  char buf[2048], name[256];
  int frags, count;
  float fragnum;
  int cap_level, cap_racewar, cap_others;
  long cap_frags;
  time_t cap_timer;
  int days, hours, mins, secs;
  MYSQL_RES *res;
  MYSQL_ROW row;

  get_level_cap_info(&cap_frags, &cap_racewar, &cap_level, &cap_timer);
  cap_others = sql_level_cap((cap_racewar == RACEWAR_GOOD) ? RACEWAR_EVIL : RACEWAR_GOOD);
  cap_timer -= time(NULL);

  if (cap_timer <= 0)
  {
    secs = mins = hours = days = 0;
  }
  else
  {
    secs = cap_timer % 60;
    cap_timer /= 60;
    mins = cap_timer % 60;
    cap_timer /= 60;
    hours = cap_timer % 24;
    cap_timer /= 24;
    days = cap_timer;
  }

  snprintf(output, 65536,
    "&+YFrag Level Cap:&+w %d - &+%c%s&n, &+w%d&N - Others, &+YTop Frag Amount: &+w%d.%02d\n"
    "&+YTimer:&+w %02d:%02d:%02d:%02d &+YFrags needed:&+w %.2f&n\n\n&+WTop Fraggers\n\n",
    cap_level, racewar_color[cap_racewar].color, racewar_color[cap_racewar].name,
    cap_others, (int)(cap_frags / 100), (int)(cap_frags % 100),
    days, hours, mins, secs, LEVEL_TO_FRAGS(cap_level + 1));

  // query top fraggers (no filter)
  res = db_query("SELECT char_name, total_frags FROM frag_leaderboard "
                 "WHERE deleted_at IS NULL ORDER BY total_frags DESC LIMIT %d", MAX_FRAG_SIZE);
  if (res)
  {
    count = 0;
    while ((row = mysql_fetch_row(res)) && count < MAX_FRAG_SIZE)
    {
      if (row[0] && row[1])
      {
        strncpy(name, row[0], sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        name[0] = toupper(name[0]);
        frags = atoi(row[1]);
        fragnum = frags / 100.0;
        snprintf(buf, sizeof(buf), "   &+Y%-30s             &+R% 6.2f\r\n", name, fragnum);
        strcat(output, buf);
        count++;
      }
    }
    mysql_free_result(res);

    while (count < MAX_FRAG_SIZE)
    {
      snprintf(buf, sizeof(buf), "   &+Y%-30s             &+R% 6.2f\r\n", "Nobody", 0.0);
      strcat(output, buf);
      count++;
    }
  }

  strcat(output, "\r\n\r\n&+LLowest Fraggers\r\n\r\n");

  // query lowest fraggers
  res = db_query("SELECT char_name, total_frags FROM frag_leaderboard "
                 "WHERE deleted_at IS NULL ORDER BY total_frags ASC LIMIT %d", MAX_FRAG_SIZE);
  if (res)
  {
    count = 0;
    while ((row = mysql_fetch_row(res)) && count < MAX_FRAG_SIZE)
    {
      if (row[0] && row[1])
      {
        strncpy(name, row[0], sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        name[0] = toupper(name[0]);
        frags = atoi(row[1]);
        fragnum = frags / 100.0;
        snprintf(buf, sizeof(buf), "   &+Y%-30s             &+R% 6.2f\r\n", name, fragnum);
        strcat(output, buf);
        count++;
      }
    }
    mysql_free_result(res);

    while (count < MAX_FRAG_SIZE)
    {
      snprintf(buf, sizeof(buf), "   &+Y%-30s             &+R% 6.2f\r\n", "Nobody", 0.0);
      strcat(output, buf);
      count++;
    }
  }

  strcat(output, "\r\n");
  return output;
#endif
}

void redis_cache_fraglist(void)
{
#ifndef __NO_MYSQL__
  if (!redis_enabled)
    return;

  char *output = generate_fraglist_output();
  if (output)
  {
    redis_cache_set("mud:cache:fraglist", output);
    free(output);
    logit(LOG_SYS, "redis: cached fraglist");
  }
#endif
}

char *redis_get_fraglist(void)
{
  return redis_cache_get("mud:cache:fraglist");
}

void redis_invalidate_fraglist(void)
{
  redis_cache_del("mud:cache:fraglist");
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
    redis_cache_set_ex("mud:cache:epic_zones", EPIC_ZONES_CACHE_TTL, output);
    free(output);
    logit(LOG_SYS, "redis: cached epic zones");
  }
#endif
}

char *redis_get_epic_zones(void)
{
  return redis_cache_get("mud:cache:epic_zones");
}

void redis_invalidate_epic_zones(void)
{
  redis_cache_del("mud:cache:epic_zones");
}

// online players list for web
void redis_player_online(P_char ch)
{
#ifndef __NO_MYSQL__
  if (!redis_enabled || !redis_ctx || !ch || IS_NPC(ch))
    return;

  char json[512];
  snprintf(json, sizeof(json),
    "{\"name\":\"%s\",\"level\":%d,\"class\":%lu,\"race\":%d,\"racewar\":%d}",
    GET_NAME(ch),
    GET_LEVEL(ch),
    (unsigned long)ch->player.m_class,
    GET_RACE(ch),
    GET_RACEWAR(ch));

  redisReply *reply = (redisReply *)redisCommand(redis_ctx,
    "HSET mud:online %d %b", GET_PID(ch), json, strlen(json));
  if (reply)
    freeReplyObject(reply);
#endif
}

void redis_player_offline(P_char ch)
{
#ifndef __NO_MYSQL__
  if (!redis_enabled || !redis_ctx || !ch || IS_NPC(ch))
    return;

  redisReply *reply = (redisReply *)redisCommand(redis_ctx,
    "HDEL mud:online %d", GET_PID(ch));
  if (reply)
    freeReplyObject(reply);
#endif
}

void redis_clear_online_players(void)
{
#ifndef __NO_MYSQL__
  if (!redis_enabled || !redis_ctx)
    return;

  redisReply *reply = (redisReply *)redisCommand(redis_ctx, "DEL mud:online");
  if (reply)
    freeReplyObject(reply);
#endif
}
