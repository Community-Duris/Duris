#ifndef REDIS_FLOOR_STORE_H
#define REDIS_FLOOR_STORE_H

#include <stddef.h>
#include <stdint.h>

struct redis_connection_settings;

constexpr size_t REDIS_FLOOR_QUEUE_CAPACITY = 8;
constexpr size_t REDIS_FLOOR_QUEUE_MAX_BYTES = 16 * 1024 * 1024;
constexpr size_t REDIS_FLOOR_BATCH_CAPACITY = 2048;
constexpr size_t REDIS_FLOOR_KEY_MAX_BYTES = 128;
constexpr size_t REDIS_FLOOR_VALUE_MAX_BYTES = 256 * 1024;
constexpr unsigned int REDIS_FLOOR_MAX_COMMAND_ATTEMPTS = 3;

struct redis_floor_store_config
{
	const struct redis_connection_settings *connection;
};

struct redis_floor_mutation
{
	uint64_t uid;
	const unsigned char *value;
	size_t value_size;
	bool remove;
	bool encode_world_object = false;
};

struct redis_floor_store_health
{
	uint64_t submitted_batches;
	uint64_t completed_batches;
	uint64_t completed_mutations;
	uint64_t command_failures;
	uint64_t reconnects;
	uint64_t dropped_batches;
	size_t queued_batches;
	size_t queued_bytes;
	size_t high_water_batches;
	size_t high_water_bytes;
	bool initialized;
	bool connected;
	bool busy;
	bool barrier_requested;
	bool paused;
};

bool redis_floor_store_init(const struct redis_floor_store_config *config);
bool redis_floor_store_submit(const char *key, const struct redis_floor_mutation *mutations,
			      size_t count);
bool redis_floor_store_request_barrier(void);
bool redis_floor_store_take_barrier(bool *succeeded);
void redis_floor_store_resume(void);
bool redis_floor_store_drain(uint64_t timeout_msec);
bool redis_floor_store_shutdown(uint64_t timeout_msec);
void redis_floor_store_cancel(void);
struct redis_floor_store_health redis_floor_store_health_copy(void);
void redis_floor_store_reset_for_tests(void);

#endif
