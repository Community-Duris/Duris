#ifndef REDIS_PRESENCE_WORKER_H
#define REDIS_PRESENCE_WORKER_H

#include <stddef.h>
#include <stdint.h>

constexpr size_t REDIS_PRESENCE_QUEUE_CAPACITY = 1024;
constexpr size_t REDIS_PRESENCE_MAX_PAYLOAD_BYTES = 4096;
constexpr unsigned int REDIS_PRESENCE_MAX_COMMAND_ATTEMPTS = 3;

struct redis_presence_worker_config
{
	const char *host;
	int port;
	int connect_timeout_msec;
	int command_timeout_msec;
};

struct redis_presence_worker_health
{
	uint64_t submitted;
	uint64_t completed;
	uint64_t command_failures;
	uint64_t reconnects;
	uint64_t dropped;
	size_t queued;
	size_t high_water;
	bool initialized;
	bool connected;
	bool busy;
};

bool redis_presence_worker_init(const struct redis_presence_worker_config *config);
bool redis_presence_worker_submit_online(int pid, const char *json, bool publish_event);
bool redis_presence_worker_submit_offline(int pid, bool publish_event);
bool redis_presence_worker_submit_clear(void);
bool redis_presence_worker_drain(uint64_t timeout_msec);
bool redis_presence_worker_shutdown(uint64_t timeout_msec);
void redis_presence_worker_cancel(void);
struct redis_presence_worker_health redis_presence_worker_health_copy(void);
void redis_presence_worker_reset_for_tests(void);

#endif
