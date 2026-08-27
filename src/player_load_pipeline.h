#ifndef PLAYER_LOAD_PIPELINE_H
#define PLAYER_LOAD_PIPELINE_H

#include "player_load_repository.h"

#include <cstddef>
#include <cstdint>

constexpr size_t PLAYER_LOAD_MAX_PENDING = 256;
constexpr size_t PLAYER_LOAD_MAX_COMPLETIONS = 256;

enum class player_load_submit_outcome : uint8_t
{
	accepted,
	duplicate,
	invalid,
	capacity_exceeded,
	unavailable,
};

struct player_load_pipeline_health
{
	uint64_t queued = 0;
	uint64_t inflight = 0;
	uint64_t completions = 0;
	uint64_t high_water = 0;
	uint64_t submitted = 0;
	uint64_t cancelled = 0;
	uint64_t stale = 0;
	uint64_t applied = 0;
	uint64_t retryable_failures = 0;
	uint64_t component_failures = 0;
	uint64_t limit_exceeded = 0;
	uint64_t timed_out = 0;
	uint64_t oldest_age_msec = 0;
	uint64_t last_completion_latency_usec = 0;
	uint64_t max_completion_latency_usec = 0;
	uint64_t last_transaction_usec = 0;
	uint64_t last_snapshot_bytes = 0;
	uint64_t last_snapshot_age_sec = 0;
	uint32_t last_query_count = 0;
	uint32_t last_row_count = 0;
	bool running = false;
	bool stop_pending = false;
};

using player_load_execute_fn = player_load_result (*)(const player_load_request &, void *);

bool player_load_pipeline_init(player_load_execute_fn execute = nullptr, void *context = nullptr);
uint64_t player_load_pipeline_next_request_id(void);
void player_load_pipeline_shutdown(void);
player_load_submit_outcome player_load_pipeline_submit(player_load_request request);
bool player_load_pipeline_cancel(uint64_t request_id);
size_t player_load_pipeline_pulse(player_load_result *results_out, size_t capacity);
bool player_load_pipeline_wait(player_load_request request, player_load_result *result_out,
			       uint64_t timeout_msec);
player_load_pipeline_health player_load_pipeline_health_copy(void);
void player_load_pipeline_note_stale(void);
void player_load_pipeline_reset_for_tests(void);

#endif
