#ifndef PLAYER_SAVE_JOURNAL_H
#define PLAYER_SAVE_JOURNAL_H

#include "player/player_save_worker.h"

#include <cstddef>
#include <cstdint>

constexpr size_t PLAYER_SAVE_JOURNAL_MAX_BYTES = 256 * 1024 * 1024;
// Admission bound for new writes. The reader still scans every byte of a
// legacy journal (up to MAX_BYTES), including records beyond this bound.
constexpr size_t PLAYER_SAVE_JOURNAL_MAX_RECORDS = 16384;
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
	quarantined_pid,
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
	bool record_limit_exceeded;
};

bool player_save_journal_init(const char *directory,
			      size_t quota_bytes = PLAYER_SAVE_JOURNAL_MAX_BYTES);
void player_save_journal_shutdown(void);
// True for a durably quarantined PID, and for every PID while an attempted
// journal initialization has failed.  The latter keeps synchronous bypasses
// closed when the durable fence itself cannot be established.
bool player_save_journal_pid_quarantined(int pid);
player_save_journal_result player_save_journal_append(const player_snapshot &snapshot);
// Preserve a pre-fence queued capture for an already quarantined PID. Success
// is archive durability only: never active replay eligibility or a DB receipt.
player_save_journal_result player_save_journal_archive_quarantined(const player_snapshot &snapshot);
player_save_journal_result player_save_journal_checkpoint(int pid,
							  player_revision_t durable_revision);
player_save_journal_result player_save_journal_replay(player_save_apply_fn apply, void *context);
player_save_journal_health player_save_journal_health_copy(void);

bool player_save_journal_worker_append(const player_snapshot &snapshot, void *context);
bool player_save_journal_worker_ack(const player_snapshot &snapshot,
				    player_revision_t durable_revision, void *context);
// Preserve every unresolved frame for this PID before any completion can
// reopen admission. Archive failure fences all admission; it never ACKs.
void player_save_journal_worker_terminal(const player_snapshot &snapshot, void *context) noexcept;

#endif
