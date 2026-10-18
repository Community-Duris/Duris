/****************************************************************************
 *
 *  File: player_save_worker.h                                  Part of Duris
 *  Usage: player save worker types, completions, and health
 *
 *  Copyright 1990, 1991 - see LICENSE for complete information
 *
 *  Additions since 2025 by the Duris maintainers and since 2026 by
 *    LuminariMUD maintainers are public domain (Unlicense, see LICENSE)
 *
 *  Created by: Duris, LuminariMUD, Zusuk                  Date: 2026-09-23
 *
 ****************************************************************************/

#ifndef PLAYER_SAVE_WORKER_H
#define PLAYER_SAVE_WORKER_H

#include "player/player_snapshot.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

/*
 * The one persistence writer. A single background thread applies every queued
 * save in the order it was captured: player saves (with their pets), corpse
 * saves, locker saves, saved room items, log_entries rows and critical commands.
 * A newer save of the same owner replaces its queued one when that one is the last
 * job queued; otherwise it is queued behind, so no job overtakes one it may rely on.
 * Each log row and each command is its own owner, so none replaces another.
 *
 * A lost connection is retried at the head of the queue. Any other failure is
 * reported through the completion and the job is dropped; the caller marks the
 * owner dirty so its next save carries the state again.
 */
constexpr unsigned int PLAYER_SAVE_WORKER_DEFAULT_THREADS = 1;
constexpr uint64_t PLAYER_SAVE_WORKER_MAX_AGE_MSEC = 5 * 60 * 1000;
constexpr uint64_t PLAYER_SAVE_WORKER_RETRY_INITIAL_MSEC = 100;
constexpr uint64_t PLAYER_SAVE_WORKER_RETRY_MAX_MSEC = 5000;
// Completed jobs retain a slot until the game consumes their completion. Payload
// bytes count queued and in-flight captures; completions no longer own a payload.
constexpr size_t PLAYER_SAVE_WORKER_MAX_PENDING_JOBS = 4096;
constexpr size_t PLAYER_SAVE_WORKER_MAX_PENDING_BYTES = 128 * 1024 * 1024;
// How long an interrupted writer gets to stop before shutdown goes on without it.
constexpr uint64_t PLAYER_SAVE_WORKER_STOP_GRACE_MSEC = 1000;
/* Application-specific repository error: a replacement player-item graph did
 * not exactly match authoritative active custody. */
constexpr unsigned int PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH = 10001;

enum class persistence_job_kind : uint8_t
{
	player,
	corpse,
	locker,
	saved_item,
	log,
	// A critical command (critical_command_coordinator.c), in capture order with the saves.
	critical,
	// A change to an account's bank, added to what the database holds.
	bank,
	// A shopkeeper's stock, keyed by shop number plus one.
	shopkeeper,
	// Statements or a read the game thread queued (sql_async.c); each is its own owner.
	sql,
	// Complete flat-file artifact/soul captures, on the same ordered writer.
	artifact,
	// Batched economic audit events outbox persistence.
	economy,
};

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
	// Identifier only, never SQL or values. Empty when the backend cannot name a field.
	std::array<char, 64> failed_field = {};
	bool rolled_back = false;
};

struct player_save_completion
{
	persistence_job_kind kind;
	uint64_t owner;
	int32_t pid;
	player_revision_t revision;
	player_component_mask_t components;
	player_save_apply_outcome outcome;
	player_revision_t durable_revision = 0;
	unsigned int error_code;
	unsigned int retry_count;
	uint64_t queued_at_usec;
	uint64_t started_at_usec;
	uint64_t completed_at_usec;
	std::array<char, 64> failed_field = {};
	bool rolled_back = false;
};

enum class player_save_submit_result : uint8_t
{
	accepted,
	replaced,
	invalid,
	unavailable,
	coalesced,
	stale,
	capacity_exceeded,
	revision_state_mismatch,
	worker_unavailable,
	journal_failure,
	durably_spilled,
};

struct player_save_worker_health
{
	uint64_t queued_jobs;
	uint64_t inflight_jobs;
	uint64_t completion_jobs;
	uint64_t queued_bytes;
	uint64_t inflight_bytes;
	uint64_t high_water_jobs;
	uint64_t high_water_bytes;
	uint64_t oldest_age_msec;
	bool age_limit_exceeded;
	uint64_t submitted;
	uint64_t replaced;
	uint64_t applied;
	uint64_t connection_retries;
	uint64_t failures;
	uint64_t max_capture_to_apply_usec;
	uint64_t max_apply_usec;
	uint64_t queued_pids = 0;
	uint64_t inflight_pids = 0;
	uint64_t high_water_pids = 0;
	uint64_t coalesced = 0;
	uint64_t stale = 0;
	uint64_t retryable_failures = 0;
	uint64_t terminal_failures = 0;
	uint64_t custody_payload_mismatches = 0;
	uint64_t retries_exhausted = 0;
	unsigned int worker_threads = 1;
	unsigned int running_workers = 1;
	uint64_t max_ack_latency_usec = 0;
	uint64_t max_revision_gap = 0;
	bool running;
	bool stop_pending;
};

using player_save_apply_fn = player_save_apply_result (*)(const player_snapshot &snapshot,
							  void *context);
// Writes one sealed corpse, locker, saved room item or log job on the writer thread.
using persistence_job_write_fn = std::function<player_save_apply_result()>;
using persistence_job_owner = std::pair<persistence_job_kind, uint64_t>;

bool player_save_worker_init(player_save_apply_fn apply, void *context);
// Stops after the job being written. If one is being written, `interrupt` (when given)
// is called to cut its database call short; the job then stays pending. A writer that
// has not stopped within PLAYER_SAVE_WORKER_STOP_GRACE_MSEC after that (a new
// connection being opened cannot be cut short) is left to finish on its own. Jobs
// still queued are not written: persistence_writer_pending_owners() names them all.
using player_save_journal_append_fn = bool (*)(const player_snapshot &snapshot, void *context);
using player_save_journal_ack_fn = bool (*)(int pid, player_revision_t revision, void *context);

inline bool player_save_worker_set_journal_hooks(player_save_journal_append_fn append,
						 player_save_journal_ack_fn acknowledge,
						 void *context)
{
	(void)context;
	if (append && !acknowledge)
		return false;
	return true;
}

void player_save_worker_shutdown(void (*interrupt)(void) = nullptr);
player_save_submit_result player_save_worker_submit(player_snapshot snapshot);
player_save_submit_result player_save_worker_submit_retained(player_snapshot *snapshot);
// Capacity refusal returns unavailable with errno=ENOSPC, preserving admitted jobs.
player_save_submit_result persistence_writer_submit(persistence_job_kind kind, uint64_t owner,
						    size_t bytes, persistence_job_write_fn write);
size_t player_save_worker_pulse(player_save_completion *completions_out, size_t capacity);
// True while this owner has a save queued or being written.
bool player_save_worker_pid_pending(int pid);
bool persistence_writer_pending(persistence_job_kind kind, uint64_t owner);
// Blocks until nothing is queued or being written, or the timeout passes.
bool persistence_writer_wait_idle(uint64_t timeout_msec);
std::vector<persistence_job_owner> persistence_writer_pending_owners(void);
const char *persistence_job_kind_name(persistence_job_kind kind);
player_save_worker_health player_save_worker_health_copy(void);
void player_save_worker_reset_for_tests(void);

#endif
