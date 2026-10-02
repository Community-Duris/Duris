#ifndef PLAYER_SAVE_JOURNAL_H
#define PLAYER_SAVE_JOURNAL_H

#include "player/player_save_worker.h"

#include <cstddef>
#include <cstdint>
#include <array>
#include <string>
#include <vector>

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

// Resident metadata only: try-lock, no initialization, decoding, replay or I/O.
struct player_save_journal_diagnostic
{
	player_save_journal_health health = {};
	bool available = false, global_fence = false, pid_fence = false, policy_fence = false;
	size_t archived_frames = 0;
	uint64_t archived_bytes = 0;
	bool recovery_prepared = false, recovery_resolved = false, recovery_revoked = false;
	player_revision_t replacement_revision = 0;
};
player_save_journal_diagnostic player_save_journal_diagnostic_copy(int pid);

bool player_save_journal_worker_append(const player_snapshot &snapshot, void *context);
bool player_save_journal_worker_ack(const player_snapshot &snapshot,
				    player_revision_t durable_revision, void *context);
// Preserve every unresolved frame for this PID before any completion can
// reopen admission. Archive failure fences all admission; it never ACKs.
void player_save_journal_worker_terminal(const player_snapshot &snapshot, void *context) noexcept;

// Recovery is a separate, listener-free owner. These records preserve the exact
// request and proof before that owner may touch authority; they are never live
// save ACKs. Ordinary admission remains fenced while a record is prepared.
struct player_save_recovery_record
{
	std::array<uint8_t, 16> identity = {};
	uint32_t backend = 0; // 1: flat-file, 2: SQL
	std::string backend_identity;
	std::string account_name;
	std::array<uint8_t, 32> archive_digest = {};
	player_snapshot baseline;
	player_snapshot replacement;
	std::vector<std::vector<uint8_t>> creation_commands;
	std::vector<uint8_t> authority_evidence;
};

player_save_journal_result
player_save_journal_recovery_inspect(int pid, std::vector<player_snapshot> *frames,
				     std::array<uint8_t, 32> *digest);
player_save_journal_result
player_save_journal_recovery_prepare(const player_save_recovery_record &record);
player_save_journal_result
player_save_journal_recovery_read(int pid, player_save_recovery_record *record, bool *resolved);
bool player_save_journal_recovery_matches(const player_save_recovery_record &record);
bool player_save_journal_recovery_fingerprint(const player_save_recovery_record &record,
					      std::array<uint8_t, 32> *digest);
bool player_save_journal_resolved_recoveries(std::vector<player_save_recovery_record> *records);
// Only the native recovery owner supplies this proof after exact read-back. A
// revision-only result or an unverified operation receipt must not call resolve.
using player_save_recovery_verify_fn = bool (*)(const player_save_recovery_record &, void *);
player_save_journal_result
player_save_journal_recovery_resolve(const player_save_recovery_record &record,
				     player_save_recovery_verify_fn verify, void *context);

#endif
