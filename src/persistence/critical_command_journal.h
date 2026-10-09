#ifndef CRITICAL_COMMAND_JOURNAL_H
#define CRITICAL_COMMAND_JOURNAL_H

#include "persistence/critical_command.h"

#include <cstddef>
#include <cstdint>
#include <vector>

constexpr size_t CRITICAL_COMMAND_JOURNAL_DEFAULT_QUOTA = 256 * 1024 * 1024;
constexpr size_t CRITICAL_COMMAND_JOURNAL_MAX_RECORDS = 4096;

enum class critical_command_journal_result : uint8_t
{
	ok,
	not_initialized,
	invalid,
	io_failure,
	unsafe_permissions,
	quota_exceeded,
	corrupt_data,
	replay_blocked,
	append_uncertain,
};

struct critical_command_journal_health
{
	uint64_t records;
	uint64_t bytes;
	uint64_t oldest_age_msec;
	uint64_t appends;
	uint64_t checkpoints;
	uint64_t replays;
	uint64_t duplicates;
	uint64_t corrupt_records;
	uint64_t io_failures;
	critical_command_journal_result last_result;
	bool quota_exceeded;
	bool append_uncertain;
	bool initialized;
};

using critical_command_replay_fn = bool (*)(critical_command command, void *context);

// This carrier retains original recovery context, never SQL/source/ACK authority.
// Command payload/encoding limits and the journal quota/record limits are unchanged.
constexpr size_t CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES = 32 * 1024 * 1024;

enum class critical_native_recovery_phase : uint8_t
{
	execution_pending = 1,
	continuation_pending = 2,
};

struct critical_native_recovery_envelope
{
	critical_command command;
	uint64_t revision = 0;
	critical_native_recovery_phase phase = critical_native_recovery_phase::execution_pending;
	std::vector<uint8_t> attachment;
};

using critical_native_recovery_replay_fn = bool (*)(critical_native_recovery_envelope envelope,
						    void *context);

// Initial revision 1/execution_pending; command and attachment share one fsync.
// replay_blocked means prior journal uncertainty prevented any append attempt.
// append_uncertain means this attempted append or its cleanup is uncertain.
critical_command_journal_result
critical_command_journal_append_native_recovery(const critical_native_recovery_envelope &envelope);
// Confirm only the exact initial frame after uncertain admission; partial,
// conflicting or absent records refuse. This grants no execution/ACK authority.
critical_command_journal_result
critical_command_journal_sync_native_recovery(const critical_native_recovery_envelope &expected);
// Exact immutable command/operation, revision + 1, execution -> execution/continuation
// or continuation -> continuation. A different current record is never overwritten.
critical_command_journal_result critical_command_journal_replace_native_recovery(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor);
// Exact existing continuation_pending record only; absence is not retirement proof.
critical_command_journal_result
critical_command_journal_retire_native_recovery(const critical_native_recovery_envelope &expected);
// Only the guarded save-owner ACK can retire an original held execution frame.
// Public generic/native continuation retirement remains unchanged.
class player_save_restored_publication_owner;
class critical_held_retirement_journal_owner final
{
	friend bool critical_command_coordinator_acknowledge_publication(
		player_save_restored_publication_owner &);
	static critical_command_journal_result
	retire_execution(const critical_native_recovery_envelope &);
};
// Full scan/validation precedes callbacks. The legacy callback never sees an envelope.
// Exact two-record continuation transition in the original atomic rewrite:
// replace parent (or delete it for terminal completion) and delete child together.
// Both exact bodies must be present in phase2; uncertainty confirms only this
// exact pair and complete attempted postimage. No absence, ACK, world/reward proof
// or parent-child authority follows. The original coordinator/domain owner must
// establish that binding and pin both operation lifetimes before calling.
critical_command_journal_result critical_command_journal_transition_native_pair(
	const critical_native_recovery_envelope &expected_parent,
	const critical_native_recovery_envelope &expected_child,
	const critical_native_recovery_envelope *parent_successor);

critical_command_journal_result
critical_command_journal_replay_with_native(critical_command_replay_fn legacy_replay,
					    critical_native_recovery_replay_fn native_replay,
					    void *context);

bool critical_command_journal_init(const char *directory,
				   size_t quota_bytes = CRITICAL_COMMAND_JOURNAL_DEFAULT_QUOTA);
void critical_command_journal_shutdown(void);
critical_command_journal_result critical_command_journal_append(const critical_command &command);
critical_command_journal_result critical_command_journal_sync(void);
critical_command_journal_result
critical_command_journal_checkpoint(const critical_operation_id &operation_id);
critical_command_journal_result critical_command_journal_replay(critical_command_replay_fn replay,
								void *context);
critical_command_journal_health critical_command_journal_health_copy(void);
const char *critical_command_journal_result_name(critical_command_journal_result result);
void critical_command_journal_reset_for_tests(void);

// Full original single native continuation retirement, with prospective owning
// mixed-journal decode and exact rewrite/uncertainty confirmation. Caller input,
// old outputs and inline context remain in outer. Callback must count the
// published persistent rewrite storage exactly once separately from outer and
// must not acquire journal/coordinator mutexes; it runs under journal mutex.
// No source/ACK/execution proof or absence-based retirement is granted.
// After return, drop the call scratch before refreshing aggregate retention:
// post-rename uncertainty promotes already-admitted vectors without allocation.
critical_command_journal_result
critical_command_journal_retire_native_recovery_bounded(const critical_native_recovery_envelope &,
							bool (*)(size_t, void *) noexcept, void *,
							size_t outer_live) noexcept;
// Allocation-free atomic visibility of actual persistent attempt storage.
// It is a census hook, not an independent concurrent admission protocol.
size_t critical_command_journal_native_rewrite_storage_bytes() noexcept;

#endif
