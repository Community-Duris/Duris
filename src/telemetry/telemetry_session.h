#ifndef DURIS_TELEMETRY_SESSION_H
#define DURIS_TELEMETRY_SESSION_H

#include "telemetry/telemetry_runtime.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

/*
 * Private #264 state boundary.  The state is value-only and caller-owned: no
 * player, descriptor, game callback, SQL handle, thread, or filesystem object
 * is retained here.  The three service contexts are borrowed synchronously by
 * this module; a caller must keep them alive for the state lifetime and must
 * give #266 the same key allocator used here.
 */
inline constexpr std::size_t TELEMETRY_SESSION_STATE_MAX_SLOTS = 256U;
inline constexpr telemetry_duration_usec TELEMETRY_SESSION_RETIRE_AFTER_MAX_USEC =
	TELEMETRY_ACTIVE_WINDOW_USEC_MAX_PROPOSAL;

enum class telemetry_session_state_outcome : std::uint8_t
{
	accepted = 0,
	accepted_unclosed_recovery = 1,
	idempotent = 2,
	retired = 3,
	invalid = 4,
	not_found = 5,
	capacity_full = 6,
	clock_unavailable = 7,
	sink_rejected = 8,
	allocator_exhausted = 9,
	revision_exhausted = 10,
};

/* Return false rather than inventing a timestamp when the clock is unavailable. */
struct telemetry_session_clock
{
	using now_function = bool (*)(void *, telemetry_monotonic_usec *,
				      telemetry_utc_usec *) noexcept;

	now_function now;
	void *context;
};

/*
 * emit is called immediately with one copied, fixed-size value.  The session
 * module never queues this function or context.  A false return is a bounded
 * admission loss; state and absolute counters still advance.
 */
struct telemetry_session_sink
{
	using emit_function = bool (*)(void *, const telemetry_record *) noexcept;

	emit_function emit;
	void *context;
};

/*
 * The allocator is the coordination point shared by session control and #266
 * detail producers.  It must return a fresh replay key for the current process
 * producer; this module deliberately has no private record sequence counter.
 */
struct telemetry_session_record_key_allocator
{
	using next_function = bool (*)(void *, telemetry_record_kind,
				       telemetry_record_key *) noexcept;

	next_function next;
	void *context;
};

struct telemetry_session_state_config
{
	std::uint16_t max_slots;
	std::uint16_t reserved;
	telemetry_duration_usec detached_retire_after_usec;
	telemetry_producer_id producer;
	telemetry_session_clock clock;
	telemetry_session_sink sink;
	telemetry_session_record_key_allocator key_allocator;
};

struct telemetry_session_state_result
{
	telemetry_session_state_outcome outcome;
	std::uint16_t records_attempted;
	std::uint16_t records_accepted;
	std::uint16_t records_dropped;
	std::uint16_t slots_retired;
	telemetry_checkpoint_revision revision;
	telemetry_record_key first_record;
	telemetry_record_key last_record;
	telemetry_cumulative_counters cumulative;
	telemetry_quality_mask quality_flags;
};

struct telemetry_session_state_stats
{
	std::uint16_t capacity;
	std::uint16_t used_slots;
	std::uint16_t resident_slots;
	std::uint16_t closed_slots;
	std::uint64_t accepted_control_total;
	std::uint64_t dropped_control_total;
	std::uint64_t retired_session_total;
	std::uint64_t unclosed_tail_total;
	std::uint64_t revision_exhaustion_total;
};

struct telemetry_session_state_view
{
	telemetry_session_ref session;
	telemetry_connection_id connection;
	telemetry_cumulative_counters cumulative;
	telemetry_checkpoint_revision last_allocated_revision;
	telemetry_quality_mask quality_flags;
	std::uint8_t connected;
	std::uint8_t closed;
	std::uint8_t reserved[6];
};

enum class telemetry_session_slot_lifecycle : std::uint8_t
{
	empty = 0,
	resident = 1,
	closed = 2,
};

enum class telemetry_session_accounting_mode : std::uint8_t
{
	unknown = 0,
	active = 1,
	idle = 2,
	linkdead = 3,
};

/* Private fixed-capacity slot.  It contains no owned allocation. */
struct telemetry_session_slot
{
	telemetry_session_slot_lifecycle lifecycle;
	telemetry_session_accounting_mode mode;
	std::uint8_t connected;
	std::uint8_t has_last_counter;
	std::uint8_t has_last_transition;
	std::uint8_t has_last_exit;
	std::uint8_t has_last_checkpoint;
	std::uint8_t pending_control_drop;
	std::uint8_t has_resume_snapshot;
	std::uint8_t reserved;
	telemetry_session_ref session;
	telemetry_connection_id connection;
	telemetry_connection_id entry_connection;
	telemetry_dimensions dimensions;
	telemetry_config_id config_id;
	std::uint32_t classifier_version;
	std::uint32_t policy_version;
	telemetry_quality_mask quality_flags;
	telemetry_cumulative_counters cumulative;
	telemetry_monotonic_usec entry_monotonic_usec;
	telemetry_utc_usec entry_utc_usec;
	telemetry_monotonic_usec last_observed_monotonic_usec;
	telemetry_utc_usec last_observed_utc_usec;
	telemetry_monotonic_usec last_utc_monotonic_usec;
	telemetry_monotonic_usec detached_since_monotonic_usec;
	telemetry_checkpoint_revision last_allocated_revision;
	telemetry_connection_id last_checkpoint_connection;
	telemetry_cumulative_counters last_checkpoint_cumulative;
	telemetry_monotonic_usec last_checkpoint_monotonic_usec;
	telemetry_utc_usec last_checkpoint_utc_usec;
	telemetry_quality_mask last_checkpoint_quality_flags;
	telemetry_session_handoff resume_handoff_snapshot;
	telemetry_session_enter entry_snapshot;
	telemetry_counter_update last_counter_snapshot;
	telemetry_connection_transition last_transition_snapshot;
	telemetry_session_exit last_exit_snapshot;
};

struct telemetry_session_state
{
	std::uint16_t max_slots;
	std::uint16_t used_slots;
	std::uint16_t resident_slots;
	std::uint16_t closed_slots;
	std::uint8_t initialized;
	std::uint8_t pending_gap_has_scope;
	std::uint8_t reserved[6];
	telemetry_duration_usec detached_retire_after_usec;
	telemetry_producer_id producer;
	telemetry_session_clock clock;
	telemetry_session_sink sink;
	telemetry_session_record_key_allocator key_allocator;
	std::uint64_t pending_control_drops;
	telemetry_session_ref pending_gap_session;
	telemetry_connection_id pending_gap_connection;
	telemetry_quality_mask pending_gap_quality_flags;
	std::uint64_t accepted_control_total;
	std::uint64_t dropped_control_total;
	std::uint64_t retired_session_total;
	std::uint64_t unclosed_tail_total;
	std::uint64_t revision_exhaustion_total;
	// IDs are allocated at telemetry admission, not raw socket acceptance.
	// These watermarks survive slot reuse and never wrap within a producer.
	telemetry_connection_sequence last_connection_sequence;
	telemetry_session_sequence last_local_session_sequence;
	telemetry_session_slot slots[TELEMETRY_SESSION_STATE_MAX_SLOTS];
};

static_assert(std::is_trivially_copyable_v<telemetry_session_state_config>);
static_assert(std::is_trivially_copyable_v<telemetry_session_state_result>);
static_assert(std::is_trivially_copyable_v<telemetry_session_state_view>);
static_assert(std::is_trivially_copyable_v<telemetry_session_slot>);
static_assert(std::is_trivially_copyable_v<telemetry_session_state>);

telemetry_session_state_outcome
telemetry_session_state_init(telemetry_session_state *state,
			     const telemetry_session_state_config *config) noexcept;
void telemetry_session_state_reset(telemetry_session_state *state) noexcept;

telemetry_session_state_result
telemetry_session_state_enter(telemetry_session_state *state,
			      telemetry_session_enter enter) noexcept;
telemetry_session_state_result
telemetry_session_state_transition(telemetry_session_state *state,
				   telemetry_connection_transition transition) noexcept;
telemetry_session_state_result
telemetry_session_state_update_counters(telemetry_session_state *state,
					telemetry_counter_update update) noexcept;

/* Uses the injected clock and emits one absolute checkpoint at the observation. */
telemetry_session_state_result
telemetry_session_state_checkpoint(telemetry_session_state *state,
				   telemetry_session_ref session) noexcept;
/* Explicit timestamp form used by terminal exit and deterministic service tests. */
telemetry_session_state_result
telemetry_session_state_checkpoint_at(telemetry_session_state *state, telemetry_session_ref session,
				      telemetry_monotonic_usec at_monotonic_usec,
				      telemetry_utc_usec at_utc_usec) noexcept;

telemetry_session_state_result telemetry_session_state_exit(telemetry_session_state *state,
							    telemetry_session_exit exit) noexcept;

/*
 * Handoff accounts only through the old process' injected clock cut. Pending
 * control loss must first pass one bounded sink admission. Failure returns an
 * all-zero output, not a valid continuity token. The caller owns bounded queue
 * drain and incomplete-recovery policy; this call never blocks gameplay.
 */
telemetry_session_state_result
telemetry_session_state_handoff_copy(telemetry_session_state *state, telemetry_session_ref session,
				     telemetry_session_handoff *handoff) noexcept;
/* Shares the classifier's flush cut so a failed copyover can continue accounting. */
telemetry_session_state_result telemetry_session_state_handoff_copy_at(
	telemetry_session_state *state, telemetry_session_ref session,
	telemetry_session_handoff *handoff, telemetry_monotonic_usec at_monotonic_usec,
	telemetry_utc_usec at_utc_usec) noexcept;

/*
 * A valid handoff imports cumulative state and emits exactly one attached
 * lifecycle record.  An all-zero handoff is the explicit absent-handoff path:
 * it emits an unclosed zero-duration coverage gap and starts a new session from
 * entry, never a fabricated tail.  A malformed partial handoff is rejected.
 */
telemetry_session_state_result
telemetry_session_state_resume(telemetry_session_state *state,
			       telemetry_session_resume resume) noexcept;

/* Retires every detached slot whose finite age has elapsed; work is <= capacity. */
telemetry_session_state_result
telemetry_session_state_retire_expired(telemetry_session_state *state) noexcept;

bool telemetry_session_state_copy_view(const telemetry_session_state *state,
				       telemetry_session_ref session,
				       telemetry_session_state_view *view) noexcept;
telemetry_session_state_stats
telemetry_session_state_stats_copy(const telemetry_session_state *state) noexcept;

#endif
