#ifndef PLAYER_SAVE_WORKER_H
#define PLAYER_SAVE_WORKER_H

#include "player/player_snapshot.h"

#include <cstddef>
#include <cstdint>

constexpr size_t PLAYER_SAVE_WORKER_MAX_PIDS = 256;
constexpr size_t PLAYER_SAVE_WORKER_MAX_RESULTS = 256;
constexpr size_t PLAYER_SAVE_WORKER_MAX_BYTES = 32 * 1024 * 1024;
constexpr uint64_t PLAYER_SAVE_WORKER_MAX_AGE_MSEC = 5 * 60 * 1000;
constexpr unsigned int PLAYER_SAVE_WORKER_MAX_RETRIES = 8;
constexpr unsigned int PLAYER_SAVE_WORKER_DEFAULT_THREADS = 2;

enum class player_save_apply_outcome : uint8_t
{
	applied,
	already_applied,
	stale_revision,
	retryable_failure,
	terminal_failure,
	ambiguous_commit,
};

struct player_save_apply_result
{
	player_save_apply_outcome outcome;
	player_revision_t durable_revision;
	unsigned int error_code;
};

struct player_save_completion
{
	int32_t pid;
	player_revision_t revision;
	player_component_mask_t components;
	player_save_apply_outcome outcome;
	player_revision_t durable_revision;
	unsigned int error_code;
	unsigned int retry_count;
	uint64_t queued_at_usec;
	uint64_t started_at_usec;
	uint64_t completed_at_usec;
};

enum class player_save_submit_result : uint8_t
{
	accepted,
	coalesced,
	invalid,
	stale,
	capacity_exceeded,
	revision_state_mismatch,
	worker_unavailable,
	journal_failure,
	durably_spilled,
};

struct player_save_worker_health
{
	uint64_t queued_pids;
	uint64_t inflight_pids;
	uint64_t queued_bytes;
	uint64_t high_water_pids;
	uint64_t high_water_bytes;
	uint64_t oldest_age_msec;
	bool age_limit_exceeded;
	uint64_t submitted;
	uint64_t coalesced;
	uint64_t applied;
	uint64_t stale;
	uint64_t retryable_failures;
	uint64_t terminal_failures;
	uint64_t retries_exhausted;
	uint64_t max_capture_to_apply_usec;
	uint64_t max_apply_usec;
	uint64_t max_ack_latency_usec;
	uint64_t max_revision_gap;
	unsigned int worker_threads;
	unsigned int running_workers;
	bool running;
	bool stop_pending;
};

using player_save_apply_fn = player_save_apply_result (*)(const player_snapshot &snapshot,
							  void *context);
using player_save_journal_append_fn = bool (*)(const player_snapshot &snapshot, void *context);
using player_save_journal_ack_fn = bool (*)(int pid, player_revision_t revision, void *context);

bool player_save_worker_init(player_save_apply_fn apply, void *context,
			     unsigned int worker_threads = PLAYER_SAVE_WORKER_DEFAULT_THREADS);
void player_save_worker_shutdown(void);
bool player_save_worker_set_journal_hooks(player_save_journal_append_fn append,
					  player_save_journal_ack_fn acknowledge, void *context);
player_save_submit_result player_save_worker_submit(player_snapshot snapshot);
player_save_submit_result player_save_worker_submit_retained(player_snapshot *snapshot);
size_t player_save_worker_pulse(player_save_completion *completions_out, size_t capacity);
player_save_worker_health player_save_worker_health_copy(void);
void player_save_worker_reset_for_tests(void);

#endif
