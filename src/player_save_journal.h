#ifndef PLAYER_SAVE_JOURNAL_H
#define PLAYER_SAVE_JOURNAL_H

#include "player_save_worker.h"

#include <cstddef>
#include <cstdint>

constexpr size_t PLAYER_SAVE_JOURNAL_MAX_BYTES = 256 * 1024 * 1024;
constexpr size_t PLAYER_SAVE_JOURNAL_MAX_RECORDS = 4096;
constexpr uint64_t PLAYER_SAVE_JOURNAL_MAX_AGE_MSEC = 7ULL * 24 * 60 * 60 * 1000;

enum class player_save_journal_result : uint8_t
{
	ok,
	not_initialized,
	invalid_path,
	unsafe_permissions,
	encode_failure,
	io_failure,
	quota_exceeded,
	corrupt_data,
	replay_blocked,
};

struct player_save_journal_health
{
	uint64_t bytes;
	uint64_t records;
	uint64_t oldest_age_msec;
	uint64_t appended;
	uint64_t append_failures;
	uint64_t checkpoints;
	uint64_t checkpoint_failures;
	uint64_t replayed;
	uint64_t duplicates;
	uint64_t corrupt_records;
	uint64_t unsupported_records;
	uint64_t quarantined_bytes;
	uint64_t backpressure;
	bool initialized;
	bool quota_exceeded;
	bool age_limit_exceeded;
};

bool player_save_journal_init(const char *directory,
			      size_t quota_bytes = PLAYER_SAVE_JOURNAL_MAX_BYTES);
void player_save_journal_shutdown(void);
player_save_journal_result player_save_journal_append(const player_snapshot &snapshot);
player_save_journal_result player_save_journal_checkpoint(int pid,
							  player_revision_t durable_revision);
player_save_journal_result player_save_journal_replay(player_save_apply_fn apply, void *context);
player_save_journal_health player_save_journal_health_copy(void);

bool player_save_journal_worker_append(const player_snapshot &snapshot, void *context);
bool player_save_journal_worker_ack(int pid, player_revision_t revision, void *context);

#endif
