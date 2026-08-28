#ifndef __REDIS_H__
#define __REDIS_H__

#include "structs.h"
#include <stdbool.h>

struct ShipData;

extern bool redis_enabled;

bool redis_init(void);
void redis_cleanup(void);

// Legacy floor-pickup key cleanup. New pickup entries are not created.
void redis_clear_floor_pickups(void);

// Administrative floor-state cleanup. Runtime staging is in redis_floor_runtime.h.
void redis_clear_floor_drops(void);

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

bool redis_clear_pwipe_state(void);
bool redis_validate_pwipe_state(void);
bool redis_season_key(char *buffer, size_t size, const char *suffix);

// Legacy ship snapshot cleanup. MySQL is the only ship read authority.
void redis_invalidate_ship_snapshot(const char *owner_name);
bool redis_clear_ship_snapshots(void);

// wiz command
void do_redis(P_char ch, char *argument, int cmd);

#endif
