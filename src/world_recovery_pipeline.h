#ifndef WORLD_RECOVERY_PIPELINE_H
#define WORLD_RECOVERY_PIPELINE_H

#include "structs.h"
#include "redis_command_observability.h"

#include <stddef.h>
#include <stdint.h>

constexpr uint32_t WORLD_RECOVERY_SCHEMA_VERSION = 9;
constexpr size_t WORLD_RECOVERY_MAX_BYTES = 64 * 1024 * 1024;
constexpr size_t WORLD_RECOVERY_MAX_RECORD_BYTES = 256 * 1024;
constexpr size_t WORLD_RECOVERY_MAX_ITEM_TREE = 512;
constexpr size_t WORLD_RECOVERY_MAX_FLOOR_BYTES = 16 * 1024 * 1024;
constexpr size_t WORLD_RECOVERY_MAX_FLOOR_RECORDS = 32768;
constexpr size_t WORLD_RECOVERY_CAPTURE_RECORD_BUDGET = 1024;
constexpr uint64_t WORLD_RECOVERY_CAPTURE_TIME_BUDGET_USEC = 2000;
constexpr uint64_t WORLD_RECOVERY_CAPTURE_MAX_AGE_MSEC = 300000;
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

struct world_recovery_item_snapshot
{
	uint64_t item_uid;
	uint64_t root_item_uid;
	uint64_t parent_item_uid;
	int32_t vnum;
	int32_t type;
	int32_t values[8];
	int64_t timers[6];
	char name[80];
	char short_description[80];
	char description[160];
};

struct world_recovery_object_record
{
	int32_t room_vnum;
	uint32_t item_count;
};

static_assert(sizeof(world_recovery_object_record) +
		      WORLD_RECOVERY_MAX_ITEM_TREE * sizeof(world_recovery_item_snapshot) <=
	      WORLD_RECOVERY_MAX_RECORD_BYTES);

struct world_recovery_authority_item
{
	uint64_t item_uid;
	uint64_t root_item_uid;
	uint64_t parent_item_uid;
	int32_t room_vnum;
	int32_t vnum;
};

struct world_recovery_health
{
	uint64_t requested;
	uint64_t coalesced;
	uint64_t capture_failures;
	uint64_t capture_expirations;
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
	uint64_t last_capture_duration_msec;
	uint64_t worker_runtime_msec;
	redis_worker_operation_health publish_operations;
	uint64_t queued_generations;
	bool initialized;
	bool capture_active;
	bool worker_running;
	bool worker_busy;
};

typedef bool (*world_recovery_publish_fn)(const unsigned char *data, size_t size,
					  const world_recovery_header *header,
					  redis_shared_command_outcome *outcome, void *context);

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
bool world_recovery_capture_age_expired(uint64_t age_msec);

bool world_recovery_validate(const unsigned char *data, size_t size, int max_age_seconds,
			     uint64_t minimum_sequence, world_recovery_header *header_out);
bool world_recovery_restore(const unsigned char *data, size_t size, int max_age_seconds,
			    uint64_t minimum_sequence, world_recovery_header *header_out);
bool world_recovery_restore_with_floor(const unsigned char *data, size_t size, int max_age_seconds,
				       uint64_t minimum_sequence,
				       const unsigned char *const *floor_records,
				       const size_t *floor_record_sizes, size_t floor_record_count,
				       world_recovery_header *header_out);
int world_recovery_write_object_to_buffer(P_obj obj, int room_vnum, char *buf, size_t max_len);

void world_recovery_capture_forget_character(P_char ch);
void world_recovery_capture_forget_object(P_obj obj);
void world_recovery_pipeline_reset_for_tests(void);

#endif
