#ifndef WORLD_RECOVERY_PIPELINE_H
#define WORLD_RECOVERY_PIPELINE_H

#include "structs.h"

#include <stddef.h>
#include <stdint.h>

constexpr uint32_t WORLD_RECOVERY_SCHEMA_VERSION = 7;
constexpr size_t WORLD_RECOVERY_MAX_BYTES = 64 * 1024 * 1024;
constexpr size_t WORLD_RECOVERY_MAX_RECORD_BYTES = 256 * 1024;
constexpr size_t WORLD_RECOVERY_CAPTURE_RECORD_BUDGET = 64;
constexpr uint64_t WORLD_RECOVERY_CAPTURE_TIME_BUDGET_USEC = 2000;
constexpr size_t WORLD_RECOVERY_QUEUE_CAPACITY = 2;
constexpr unsigned int WORLD_RECOVERY_MAX_RETRIES = 3;

struct world_recovery_header
{
	char magic[4];
	uint32_t schema_version;
	uint32_t header_size;
	uint64_t sequence;
	int64_t timestamp;
	uint64_t payload_size;
	uint32_t checksum;
	uint32_t mob_count;
	uint32_t object_count;
	uint32_t door_count;
	uint32_t zone_count;
	uint8_t complete;
	uint8_t reserved[7];
};

struct world_recovery_completion
{
	uint64_t sequence;
	bool published;
	unsigned int attempts;
};

struct world_recovery_health
{
	uint64_t requested;
	uint64_t coalesced;
	uint64_t capture_failures;
	uint64_t captured_records;
	uint64_t captured_bytes;
	uint64_t submitted;
	uint64_t published;
	uint64_t publish_failures;
	uint64_t stale_completions;
	uint64_t last_submitted_sequence;
	uint64_t last_acknowledged_sequence;
	uint64_t last_published_bytes;
	uint64_t high_water_bytes;
	uint64_t capture_age_msec;
	uint64_t worker_runtime_msec;
	uint64_t queued_generations;
	bool initialized;
	bool capture_active;
	bool worker_running;
	bool worker_busy;
};

typedef bool (*world_recovery_publish_fn)(const unsigned char *data, size_t size,
					  const world_recovery_header *header, void *context);

bool world_recovery_pipeline_init(world_recovery_publish_fn publish, void *context);
bool world_recovery_pipeline_set_sequence_floor(uint64_t durable_sequence);
void world_recovery_pipeline_shutdown(void);
void world_recovery_pipeline_cancel(void);
bool world_recovery_pipeline_request(void);
void world_recovery_pipeline_pulse(void);
bool world_recovery_pipeline_take_completion(world_recovery_completion *completion);
bool world_recovery_pipeline_drain(uint64_t timeout_msec);
bool world_recovery_pipeline_busy(void);
world_recovery_health world_recovery_pipeline_health_copy(void);

bool world_recovery_validate(const unsigned char *data, size_t size, int max_age_seconds,
			     uint64_t minimum_sequence, world_recovery_header *header_out);
bool world_recovery_restore(const unsigned char *data, size_t size, int max_age_seconds,
			    uint64_t minimum_sequence, world_recovery_header *header_out);

void world_recovery_capture_forget_character(P_char ch);
void world_recovery_capture_forget_object(P_obj obj);
void world_recovery_pipeline_reset_for_tests(void);

#endif
