#ifndef REDIS_DONATION_WORKER_H
#define REDIS_DONATION_WORKER_H

#include "donation_event.h"

#include <stddef.h>
#include <stdint.h>

struct redis_connection_settings;

constexpr size_t REDIS_DONATION_QUEUE_CAPACITY = 64;
constexpr size_t REDIS_DONATION_REPLAY_CAPACITY = 256;
constexpr size_t REDIS_DONATION_WORK_BATCH = 32;
constexpr unsigned int REDIS_DONATION_MAX_RECONNECT_DELAY_SECONDS = 60;

struct redis_donation_worker_config
{
	const struct redis_connection_settings *connection;
	const char *secret;
	const char *channel;
};

struct redis_donation_worker_health
{
	uint64_t received;
	uint64_t validated;
	uint64_t rejected;
	uint64_t replayed;
	uint64_t dropped;
	uint64_t connection_failures;
	uint64_t reconnects;
	size_t queued;
	size_t high_water;
	bool initialized;
	bool connected;
};

bool redis_donation_worker_init(const struct redis_donation_worker_config *config);
bool redis_donation_worker_take(struct donation_event *event);
void redis_donation_worker_shutdown(void);
struct redis_donation_worker_health redis_donation_worker_health_copy(void);
void redis_donation_worker_reset_for_tests(void);

#endif
