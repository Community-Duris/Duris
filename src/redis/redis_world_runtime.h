#ifndef REDIS_WORLD_RUNTIME_H
#define REDIS_WORLD_RUNTIME_H

#include "core/structs.h"

#include <stdint.h>

struct redis_connection_settings;

struct redis_world_runtime_config
{
	const struct redis_connection_settings *connection;
	const char *key_namespace;
	uint64_t season_epoch;
	const char *host;
	int port;
	bool unix_socket;
};

bool redis_world_runtime_start(const struct redis_world_runtime_config *config);
void redis_world_runtime_shutdown(bool pwipe);
bool redis_world_runtime_enabled(void);
bool redis_world_recovery_boot_active(void);
void redis_world_recovery_boot_set(bool active);
bool redis_world_clean_restart_boot(void);
void redis_world_recovery_boot_clear(void);

bool redis_save_world_state(void);
bool redis_load_world_state(void);
bool redis_has_world_state(void);
bool redis_consume_world_state(void);
void event_save_world_state(P_char ch, P_char victim, P_obj obj, void *data);
void redis_world_recovery_pulse(void);
bool redis_world_recovery_drain(uint64_t timeout_msec);
bool redis_world_recovery_quiesce(void);

#endif
