#ifndef REDIS_MAINTENANCE_H
#define REDIS_MAINTENANCE_H

#include <stdint.h>

struct redis_connection_settings;

struct redis_maintenance_config
{
	const struct redis_connection_settings *connection;
	const char *key_namespace;
	uint64_t season_epoch;
	const char *presence_current_key;
	const char *presence_session_pattern;
	const char *presence_retry_pattern;
	const char *report_cache_pattern;
};

// Stopped-server season reset workflow. The caller must quiesce runtime writers first.
bool redis_maintenance_clear(const struct redis_maintenance_config *config);
bool redis_maintenance_validate(const struct redis_maintenance_config *config);

bool redis_clear_pwipe_state(void);
bool redis_validate_pwipe_state(void);

#endif
