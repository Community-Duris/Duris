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

// One allocation-free atomic projection of complete retained journal storage:
// mutex, strings and their allocated capacities, quota/health/native flag,
// full rewrite-attempt carrier, four vector capacities and both atomic carriers.
// SIZE_MAX means unavailable/overflow (also the initial value). This replaces
// the old rewrite term in the selected aggregate; never add both projections or
// duplicate startup metadata. It neither locks nor dereferences mutable storage,
// and grants no independent concurrent admission or journal readiness authority.
size_t critical_command_journal_persistent_storage_bytes() noexcept;

// Full original single native replacement: immutable command, exact expected
// body, revision+1 and nonregressing phase, complete mixed scan and exact
// uncertain postimage confirmation. Same callback/outer/current-retention
// contract as bounded retirement; persistent rewrite storage counted once
// separately. No callback/allocation after rename or durable promotion.
critical_command_journal_result critical_command_journal_replace_native_recovery_bounded(
	const critical_native_recovery_envelope &, const critical_native_recovery_envelope &,
	bool (*)(size_t, void *) noexcept, void *, size_t outer_live) noexcept;

// Genuine prospective startup companions only; original init/replay and their
// defaults remain unchanged. The startup owner supplies its authentic input,
// inline context and full ROOT/global scratch in outer_live, excluding metadata
// owned here and CURRENT coordinator storage. Its reserve must supply genuine
// same-lock CURRENT coordinator admission while the real init lock is held;
// no locking outside coordinator observer or journal reentry is allowed.
// Persistent rewrite uncertainty is counted once separately by the aggregate.
using critical_command_replay_bounded_fn = bool (*)(critical_command command,
						    void *original_context,
						    bool (*reserve)(size_t, void *) noexcept,
						    void *budget_context,
						    size_t outer_live) noexcept;
using critical_native_recovery_replay_bounded_fn = bool (*)(
	critical_native_recovery_envelope envelope, void *original_context,
	bool (*reserve)(size_t, void *) noexcept, void *budget_context, size_t outer_live) noexcept;

bool critical_command_journal_init_bounded(
	const char *directory, size_t quota_bytes, bool (*reserve)(size_t, void *) noexcept,
	void *budget_context, size_t outer_live,
	size_t *current_journal_metadata_bytes = nullptr) noexcept;

// Complete mixed admitted scan precedes every callback. Journal lock is released
// before callbacks exactly as ordinary replay. Each callback outer owns all
// retained mixed frame capacities, current native local/parameter carriers and
// moved heap once. The owning startup caller keeps genuine coordinator ownership
// across scan and callbacks; callback receivers must retain that complete prefix.
// These declarations select no startup path and grant no execution/ACK authority.
critical_command_journal_result critical_command_journal_replay_with_native_bounded(
	critical_command_replay_bounded_fn legacy_replay,
	critical_native_recovery_replay_bounded_fn native_replay, void *original_context,
	bool (*reserve)(size_t, void *) noexcept, void *budget_context, size_t outer_live,
	size_t *current_journal_metadata_bytes = nullptr) noexcept;

// Optional outputs above are genuine journal-held metadata snapshots even on
// failure; unavailable observations leave outputs unchanged. Metadata excludes
// the published rewrite carrier term already owned by the aggregate. Caller
// admits its output and actual provider frames, and retains J only during its
// genuine ownership scope. No initialized/readiness inference or lease follows.
class zone_reset_item_owner;
class critical_startup_journal_budget_owner final
{
	friend class zone_reset_item_owner;
	// Fresh passive observation for the later startup owner after its genuine
	// coordinator scope. Takes journal mutex: never call while journal is held,
	// never install in the pure seven-global observer. Caller admits actual
	// lock_guard/size_t observation frame plus its real output storage.
	static bool current_metadata_bytes(size_t *) noexcept;
};

// Pure source queries grant neither observation nor admission. Their own output
// pointer/result/query carriers must be admitted by the real caller first.
bool critical_command_journal_startup_projection_source_frame_bytes(size_t *) noexcept;
bool critical_command_journal_startup_source_frame_bytes(size_t *) noexcept;
// Pure prospective original initial workspace/lock/snapshot inline peak.
// The fixed query accessor below dominates its own genuine P+B query carriers.
// ROOT reserves this transiently with SOURCE before mutation; it is not retained
// in outer when the journal's original per-operation admissions own it.
bool critical_command_journal_startup_initial_inline_bytes(size_t *) noexcept;
// Genuine source-query output/return/three locals and the two nested pure
// source-provider signatures/returns plus both checked-add inputs/results. Caller
// additionally owns this constexpr accessor's returned size_t carrier.
constexpr size_t critical_command_journal_startup_source_query_frame_bytes() noexcept
{
	return sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) +
	       2 * (sizeof(void *) + sizeof(bool)) +
	       2 * (sizeof(void *) + sizeof(size_t) + sizeof(bool));
}
// First callback admits source scopes before an actual journal mutex/census.
// Second callback sees the genuinely published full J and excludes J from its
// incoming prefix; aggregate supplies it once. No health/file/generation change.
bool critical_command_journal_startup_persistent_projection_bounded(
	bool (*source_reserve)(size_t, void *) noexcept, void *source_context,
	bool (*physical_reserve)(size_t, void *) noexcept, void *physical_context,
	size_t outer_live, size_t *full_journal_storage_bytes) noexcept;
// Available only synchronously inside that exact reserve/context callback.
// Success yields the genuine metadata term owned in this particular prefix;
// subtract it once before full-J aggregate addition. It performs no observation,
// lock or cached-output lookup. Failure leaves output unchanged.
bool critical_command_journal_startup_owned_metadata(bool (*reserve)(size_t, void *) noexcept,
						     void *context, size_t *) noexcept;
// Complete original counterparts: no readiness/ACK/authority/default selection.
// The full journal source provider and real ROOT/caller frames remain retained
// across all callbacks. Caller input/output heaps stay in outer, except genuine
// locked metadata conveyed by the synchronous identity contract above.
bool critical_command_journal_init_physical_bounded(
	const char *directory, size_t quota_bytes, bool (*reserve)(size_t, void *) noexcept,
	void *budget_context, size_t outer_live,
	size_t *current_journal_metadata_bytes = nullptr) noexcept;
critical_command_journal_result critical_command_journal_replay_with_native_physical_bounded(
	critical_command_replay_bounded_fn, critical_native_recovery_replay_bounded_fn,
	void *original_context, bool (*reserve)(size_t, void *) noexcept, void *budget_context,
	size_t outer_live, size_t *current_journal_metadata_bytes = nullptr) noexcept;

#endif
