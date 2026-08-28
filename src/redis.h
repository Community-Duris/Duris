#ifndef __REDIS_H__
#define __REDIS_H__

#include "structs.h"
#include <stdbool.h>

struct ShipData;

extern bool redis_enabled;
extern bool redis_donation_enabled;

bool redis_init(void);
void redis_cleanup(void);

// Legacy floor-pickup key cleanup. New pickup entries are not created.
void redis_clear_floor_pickups(void);

// floor drop tracking for crash recovery
void redis_log_floor_drop(P_obj obj, int room_vnum);
void redis_remove_floor_drop(unsigned long obj_uid);
void redis_clear_floor_drops(void);
bool redis_flush_floor_drops(void);

// world state persistence for crash recovery
extern bool redis_world_state_enabled;
extern int crash_recovery_boot;
extern int clean_restart_recovery_boot;

bool redis_save_world_state(void);
bool redis_load_world_state(void);
bool redis_has_world_state(void);
bool redis_consume_world_state(void);
bool redis_clear_world_state(void);
void event_save_world_state(P_char ch, P_char victim, P_obj obj, void *data);
void redis_world_recovery_pulse(void);
bool redis_world_recovery_drain(uint64_t timeout_msec);
bool redis_world_recovery_quiesce(void);
void redis_world_recovery_set_materializing(bool active);

// cache helpers
bool redis_cache_set(const char *key, const char *value);
bool redis_cache_set_ex(const char *key, int seconds, const char *value);
char *redis_cache_get(const char *key);
bool redis_cache_del(const char *key);
void redis_check_donation_messages(void);
void event_check_donation_messages(P_char ch, P_char victim, P_obj obj, void *data);

// named command cache
void redis_cache_named_report(void);
char *redis_get_named_report(void);

// frag command cache
void redis_cache_fraglist(void);
char *redis_get_fraglist(void);
bool redis_invalidate_fraglist(void);

// epic zones command cache
void redis_cache_epic_zones(void);
char *redis_get_epic_zones(void);
bool redis_invalidate_epic_zones(void);

// arti cache
void redis_cache_artifact_list(int type, bool godlist, const char *json);
char *redis_get_artifact_list(int type, bool godlist);
bool redis_invalidate_artifact_list(int type, bool godlist);
bool redis_invalidate_artifact_cache(void);

// online players list for web
void redis_player_online(P_char ch);
void redis_player_offline(P_char ch);
void redis_clear_online_players(void);

bool redis_clear_pwipe_state(void);
bool redis_validate_pwipe_state(void);
bool redis_season_key(char *buffer, size_t size, const char *suffix);

// Legacy ship snapshot cleanup. MySQL is the only ship read authority.
void redis_invalidate_ship_snapshot(const char *owner_name);
bool redis_clear_ship_snapshots(void);

// wiz command
void do_redis(P_char ch, char *argument, int cmd);

#endif
