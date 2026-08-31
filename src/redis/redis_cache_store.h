#ifndef REDIS_CACHE_STORE_H
#define REDIS_CACHE_STORE_H

#include <stddef.h>
#include <stdint.h>
#include "redis/redis_command_observability.h"

struct redis_connection_settings;

constexpr size_t REDIS_CACHE_QUEUE_CAPACITY = 64;
constexpr size_t REDIS_CACHE_QUEUE_MAX_BYTES = 4 * 1024 * 1024;
constexpr size_t REDIS_CACHE_LOCAL_CAPACITY = 32;
constexpr size_t REDIS_CACHE_MAX_KEY_BYTES = 128;
constexpr size_t REDIS_CACHE_MAX_VALUE_BYTES = 1024 * 1024;
constexpr unsigned int REDIS_CACHE_MAX_COMMAND_ATTEMPTS = 3;

struct redis_cache_store_config
{
	const struct redis_connection_settings *connection;
};

typedef char *(*redis_cache_store_transform_fn)(const char *value, void *context);

struct redis_cache_store_health
{
	uint64_t submitted;
	uint64_t completed;
	uint64_t coalesced;
	uint64_t command_failures;
	uint64_t connection_failures;
	uint64_t reconnects;
	uint64_t dropped;
	redis_worker_operation_health operations;
	size_t queued;
	size_t queued_bytes;
	size_t high_water;
	size_t high_water_bytes;
	size_t local_entries;
	bool initialized;
	bool connected;
	bool busy;
};

bool redis_cache_store_init(const struct redis_cache_store_config *config);
bool redis_cache_store_seed(const char *key, const char *value, int ttl_seconds);
bool redis_cache_store_set(const char *key, const char *value, int ttl_seconds);
char *redis_cache_store_get(const char *key);
char *redis_cache_store_transform(const char *key, redis_cache_store_transform_fn transform,
				  void *context);
bool redis_cache_store_delete(const char *key);
bool redis_cache_store_drain(uint64_t timeout_msec);
bool redis_cache_store_shutdown(uint64_t timeout_msec);
void redis_cache_store_cancel(void);
struct redis_cache_store_health redis_cache_store_health_copy(void);
void redis_cache_store_reset_for_tests(void);

#endif
