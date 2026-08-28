#ifndef REDIS_REPORT_CACHE_H
#define REDIS_REPORT_CACHE_H

#include <stddef.h>
#include <stdint.h>

struct redis_connection_settings;

bool redis_report_cache_configure(const char *key_namespace, uint64_t epoch);
bool redis_report_cache_start(const struct redis_connection_settings *connection);
void redis_report_cache_cancel(void);
bool redis_report_cache_shutdown(uint64_t timeout_msec);
void redis_report_cache_reset(void);
bool redis_report_cache_enabled(void);
const char *redis_report_cache_pattern(void);

void redis_cache_named_report(void);
char *redis_get_named_report(void);
bool redis_invalidate_named_report(void);

void redis_cache_fraglist(void);
char *redis_get_fraglist(void);
bool redis_invalidate_fraglist(void);

void redis_cache_epic_zones(void);
void redis_cache_epic_zones_output(const char *output);
char *redis_get_epic_zones(void);
bool redis_invalidate_epic_zones(void);

void redis_cache_artifact_list(int type, bool godlist, const char *json);
char *redis_get_artifact_list(int type, bool godlist);
bool redis_invalidate_artifact_list(int type, bool godlist);
bool redis_invalidate_artifact_cache(void);

#endif
