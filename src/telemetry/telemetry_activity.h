#ifndef DURIS_TELEMETRY_ACTIVITY_H
#define DURIS_TELEMETRY_ACTIVITY_H

#include "telemetry/telemetry_session.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

/*
 * Pure #266 activity/context accounting.  The state and every service are
 * caller-owned.  Borrowed service callbacks are invoked synchronously and are
 * never copied into records; no game callback, game pointer, raw command, SQL
 * handle or allocation is retained.  The key allocator is deliberately the
 * exact allocator shared with #264.
 */
inline constexpr std::size_t TELEMETRY_ACTIVITY_STATE_MAX_SLOTS = 256U;
inline constexpr std::size_t TELEMETRY_ACTIVITY_CONFIG_MAX = 64U;
inline constexpr telemetry_duration_usec TELEMETRY_ACTIVITY_MINUTE_USEC = 60'000'000ULL;
inline constexpr telemetry_duration_usec TELEMETRY_ACTIVITY_INTERVAL_DEFAULT_USEC =
	TELEMETRY_INTERVAL_USEC_PROPOSAL;
inline constexpr telemetry_duration_usec TELEMETRY_ACTIVITY_ACTIVE_WINDOW_DEFAULT_USEC =
	TELEMETRY_ACTIVE_WINDOW_USEC_PROPOSAL;
inline constexpr std::uint32_t TELEMETRY_ACTIVITY_SEGMENTS_DEFAULT =
	TELEMETRY_CONTEXT_SEGMENTS_PER_MINUTE_PROPOSAL;
inline constexpr std::uint32_t TELEMETRY_ACTIVITY_MAX_PIECES_PER_OPERATION = 8U;

/* Context assertions are a value-only snapshot; precedence is resolved below. */
inline constexpr std::uint32_t TELEMETRY_ACTIVITY_CONTEXT_COMBAT = 1U << 0;
inline constexpr std::uint32_t TELEMETRY_ACTIVITY_CONTEXT_TRAVEL = 1U << 1;
inline constexpr std::uint32_t TELEMETRY_ACTIVITY_CONTEXT_CRAFTING = 1U << 2;
inline constexpr std::uint32_t TELEMETRY_ACTIVITY_CONTEXT_SOCIAL = 1U << 3;
inline constexpr std::uint32_t TELEMETRY_ACTIVITY_CONTEXT_ADMINISTRATION = 1U << 4;
inline constexpr std::uint32_t TELEMETRY_ACTIVITY_CONTEXT_OTHER = 1U << 5;
inline constexpr std::uint32_t TELEMETRY_ACTIVITY_CONTEXT_NONE = 1U << 6;
inline constexpr std::uint32_t TELEMETRY_ACTIVITY_CONTEXT_UNKNOWN = 1U << 7;
inline constexpr std::uint32_t TELEMETRY_ACTIVITY_CONTEXT_FLAGS_KNOWN =
	TELEMETRY_ACTIVITY_CONTEXT_COMBAT | TELEMETRY_ACTIVITY_CONTEXT_TRAVEL |
	TELEMETRY_ACTIVITY_CONTEXT_CRAFTING | TELEMETRY_ACTIVITY_CONTEXT_SOCIAL |
	TELEMETRY_ACTIVITY_CONTEXT_ADMINISTRATION | TELEMETRY_ACTIVITY_CONTEXT_OTHER |
	TELEMETRY_ACTIVITY_CONTEXT_NONE | TELEMETRY_ACTIVITY_CONTEXT_UNKNOWN;

enum class telemetry_activity_evidence_kind : std::uint8_t
{
	unknown = 0,
	player_action = 1,
	movement = 2,
	interaction = 3,
	communication = 4,
	combat_participation = 5,
	automatic_combat = 6,
	reading = 7,
	following = 8,
	recovery = 9,
	social = 10,
	afk = 11,
	spam = 12,
	keepalive = 13,
	autonomous_event = 14,
	staff_force = 15,
	linkdead = 16,
};

enum class telemetry_activity_outcome : std::uint8_t
{
	accepted = 0,
	accepted_degraded = 1,
	idempotent = 2,
	invalid = 3,
	not_found = 4,
	capacity_full = 5,
	clock_unavailable = 6,
	sink_rejected = 7,
	allocator_exhausted = 8,
	config_unavailable = 9,
	config_capacity_full = 10,
	retired = 11,
};

enum class telemetry_activity_slot_lifecycle : std::uint8_t
{
	empty = 0,
	resident = 1,
	closed = 2,
};

enum class telemetry_activity_accounting_mode : std::uint8_t
{
	unknown = 0,
	active = 1,
	idle = 2,
	linkdead = 3,
};

struct telemetry_activity_clock
{
	using now_function = bool (*)(void *, telemetry_monotonic_usec *,
				      telemetry_utc_usec *) noexcept;

	now_function now;
	void *context;
};

struct telemetry_activity_sink
{
	using emit_function = bool (*)(void *, const telemetry_record *) noexcept;

	emit_function emit;
	void *context;
};

/* Alias, not a look-alike: #264 and #266 use one sequence authority. */
using telemetry_activity_record_key_allocator = telemetry_session_record_key_allocator;

struct telemetry_activity_state_config
{
	std::uint16_t max_slots;
	std::uint16_t pulse_slot_count;
	std::uint32_t context_segments_per_minute;
	std::uint16_t reserved;
	telemetry_duration_usec interval_usec;
	telemetry_duration_usec active_window_usec;
	telemetry_duration_usec detached_retire_after_usec;
	telemetry_monotonic_usec monotonic_anchor_usec;
	telemetry_producer_id producer;
	telemetry_activity_clock clock;
	telemetry_activity_sink sink;
	telemetry_activity_record_key_allocator key_allocator;
};

/* An immutable typed observation; no raw command or callback crosses this API. */
struct telemetry_activity_evidence
{
	telemetry_session_ref session;
	telemetry_connection_id connection;
	telemetry_monotonic_usec at_monotonic_usec;
	telemetry_utc_usec at_utc_usec;
	telemetry_activity_evidence_kind kind;
	std::uint8_t reserved[3];
	telemetry_quality_mask quality_flags;
};

struct telemetry_activity_context_snapshot
{
	telemetry_session_ref session;
	telemetry_connection_id connection;
	telemetry_monotonic_usec at_monotonic_usec;
	telemetry_utc_usec at_utc_usec;
	telemetry_dimensions dimensions;
	std::uint32_t context_flags;
	telemetry_config_id config_id;
	std::uint32_t classifier_version;
	std::uint32_t policy_version;
	telemetry_quality_mask quality_flags;
};

/* A result delta is independent of interval admission. #265 can pass it to #264. */
struct telemetry_activity_result
{
	telemetry_activity_outcome outcome;
	std::uint8_t has_delta;
	std::uint8_t reserved[2];
	std::uint16_t records_attempted;
	std::uint16_t records_accepted;
	std::uint16_t records_dropped;
	std::uint16_t intervals_sealed;
	std::uint16_t slots_retired;
	std::uint16_t reserved2;
	telemetry_counter_update delta;
	telemetry_cumulative_counters cumulative;
	telemetry_quality_mask quality_flags;
};

struct telemetry_activity_pulse_request
{
	telemetry_monotonic_usec now_monotonic_usec;
	telemetry_utc_usec occurrence_utc_usec;
	std::uint16_t slot;
	std::uint16_t reserved;
	// Required capacity: every resident slot in this cohort, even if not due.
	// Insufficient space rejects the pulse before consuming any elapsed time.
	// Consume returned deltas synchronously, regardless of detail-sink outcome.
	telemetry_counter_update *deltas;
	std::uint16_t delta_capacity;
	std::uint16_t reserved2;
};

struct telemetry_activity_pulse_result
{
	telemetry_activity_outcome outcome;
	std::uint8_t reserved[3];
	std::uint16_t sessions_considered;
	std::uint16_t intervals_sealed;
	std::uint16_t records_attempted;
	std::uint16_t records_accepted;
	std::uint16_t records_dropped;
	std::uint16_t deltas_written;
	std::uint16_t deltas_dropped;
	telemetry_quality_mask quality_flags;
};

struct telemetry_activity_state_stats
{
	std::uint16_t capacity;
	std::uint16_t used_slots;
	std::uint16_t resident_slots;
	std::uint16_t closed_slots;
	std::uint64_t admitted_config_total;
	std::uint64_t accepted_detail_total;
	std::uint64_t dropped_detail_total;
	std::uint64_t accepted_control_total;
	std::uint64_t dropped_control_total;
	std::uint64_t suppressed_detail_total;
	std::uint64_t context_overflow_total;
	std::uint64_t clock_discontinuity_total;
	std::uint64_t retired_session_total;
};

struct telemetry_activity_state_view
{
	telemetry_session_ref session;
	telemetry_connection_id connection;
	telemetry_cumulative_counters cumulative;
	telemetry_dimensions dimensions;
	telemetry_config_id config_id;
	std::uint32_t classifier_version;
	std::uint32_t policy_version;
	telemetry_monotonic_usec interval_start_monotonic_usec;
	telemetry_monotonic_usec active_deadline_monotonic_usec;
	std::uint32_t context_segments_used;
	telemetry_interval_category category;
	telemetry_activity_context context;
	telemetry_context_quality context_quality;
	telemetry_quality_mask quality_flags;
	std::uint8_t connected;
	std::uint8_t closed;
	std::uint8_t detail_ready;
	std::uint8_t context_overflow;
	std::uint8_t reserved[4];
};

struct telemetry_activity_config_entry
{
	std::uint8_t used;
	std::uint8_t reserved[7];
	telemetry_config_snapshot snapshot;
};

struct telemetry_activity_pending_gap
{
	std::uint8_t pending;
	std::uint8_t scoped;
	std::uint8_t sequence_known;
	std::uint8_t duration_known;
	std::uint8_t detail_loss;
	std::uint8_t control_loss;
	std::uint8_t reserved[2];
	telemetry_gap_reason reason;
	telemetry_session_ref session;
	telemetry_connection_id connection;
	telemetry_monotonic_usec start_monotonic_usec;
	telemetry_monotonic_usec end_monotonic_usec;
	telemetry_utc_usec start_utc_usec;
	telemetry_utc_usec end_utc_usec;
	telemetry_record_sequence first_missing_record_seq;
	telemetry_record_sequence last_missing_record_seq;
	telemetry_duration_usec duration_usec;
	std::uint64_t dropped_records;
	telemetry_quality_mask quality_flags;
};

struct telemetry_activity_slot
{
	telemetry_activity_slot_lifecycle lifecycle;
	telemetry_activity_accounting_mode mode;
	std::uint8_t connected;
	std::uint8_t detail_ready;
	std::uint8_t context_overflow;
	std::uint8_t has_utc_anchor;
	std::uint8_t has_last_evidence;
	std::uint8_t has_last_context;
	std::uint8_t has_last_transition;
	std::uint8_t has_last_exit;
	std::uint8_t utc_discontinuity_pending;
	std::uint16_t context_segments_used;
	std::uint64_t context_bucket;
	std::uint32_t segment_cap;
	telemetry_duration_usec interval_usec;
	telemetry_duration_usec active_window_usec;
	telemetry_session_ref session;
	telemetry_connection_id connection;
	telemetry_dimensions dimensions;
	telemetry_activity_context context;
	telemetry_context_quality context_quality;
	std::uint8_t context_reserved;
	telemetry_config_id config_id;
	std::uint32_t classifier_version;
	std::uint32_t policy_version;
	telemetry_quality_mask quality_flags;
	telemetry_cumulative_counters cumulative;
	telemetry_monotonic_usec interval_start_monotonic_usec;
	telemetry_utc_usec interval_start_utc_usec;
	telemetry_monotonic_usec last_observed_monotonic_usec;
	telemetry_utc_usec last_observed_utc_usec;
	telemetry_monotonic_usec active_deadline_monotonic_usec;
	telemetry_monotonic_usec detached_since_monotonic_usec;
	telemetry_activity_evidence last_evidence;
	telemetry_activity_context_snapshot last_context;
	telemetry_connection_transition last_transition;
	telemetry_session_exit last_exit;
	telemetry_session_enter entry_snapshot;
};

struct telemetry_activity_state
{
	std::uint16_t max_slots;
	std::uint16_t pulse_slot_count;
	std::uint32_t context_segments_per_minute;
	std::uint16_t reserved;
	std::uint8_t initialized;
	std::uint8_t process_anchor_set;
	telemetry_duration_usec interval_usec;
	telemetry_duration_usec active_window_usec;
	telemetry_duration_usec detached_retire_after_usec;
	telemetry_monotonic_usec monotonic_anchor_usec;
	telemetry_producer_id producer;
	telemetry_activity_clock clock;
	telemetry_activity_sink sink;
	telemetry_activity_record_key_allocator key_allocator;
	std::uint16_t used_slots;
	std::uint16_t resident_slots;
	std::uint16_t closed_slots;
	std::uint16_t reserved2;
	telemetry_connection_sequence last_connection_sequence;
	telemetry_activity_config_entry configs[TELEMETRY_ACTIVITY_CONFIG_MAX];
	telemetry_activity_pending_gap pending_gap;
	std::uint64_t admitted_config_total;
	std::uint64_t accepted_detail_total;
	std::uint64_t dropped_detail_total;
	std::uint64_t accepted_control_total;
	std::uint64_t dropped_control_total;
	std::uint64_t suppressed_detail_total;
	std::uint64_t context_overflow_total;
	std::uint64_t clock_discontinuity_total;
	std::uint64_t retired_session_total;
	telemetry_activity_slot slots[TELEMETRY_ACTIVITY_STATE_MAX_SLOTS];
};

constexpr bool
telemetry_activity_evidence_kind_is_valid(telemetry_activity_evidence_kind kind) noexcept
{
	return kind >= telemetry_activity_evidence_kind::player_action &&
	       kind <= telemetry_activity_evidence_kind::linkdead;
}

constexpr bool telemetry_activity_context_flags_are_valid(std::uint32_t flags) noexcept
{
	return (flags & ~TELEMETRY_ACTIVITY_CONTEXT_FLAGS_KNOWN) == 0U;
}

constexpr telemetry_activity_context
telemetry_activity_context_resolve(std::uint32_t flags) noexcept
{
	if ((flags & TELEMETRY_ACTIVITY_CONTEXT_COMBAT) != 0U)
		return telemetry_activity_context::combat;
	if ((flags & TELEMETRY_ACTIVITY_CONTEXT_TRAVEL) != 0U)
		return telemetry_activity_context::travel;
	if ((flags & TELEMETRY_ACTIVITY_CONTEXT_CRAFTING) != 0U)
		return telemetry_activity_context::crafting;
	if ((flags & TELEMETRY_ACTIVITY_CONTEXT_SOCIAL) != 0U)
		return telemetry_activity_context::social;
	if ((flags & TELEMETRY_ACTIVITY_CONTEXT_ADMINISTRATION) != 0U)
		return telemetry_activity_context::administration;
	if ((flags & TELEMETRY_ACTIVITY_CONTEXT_OTHER) != 0U)
		return telemetry_activity_context::other;
	if ((flags & TELEMETRY_ACTIVITY_CONTEXT_NONE) != 0U)
		return telemetry_activity_context::none;
	return telemetry_activity_context::unknown;
}

constexpr bool
telemetry_activity_evidence_is_valid(const telemetry_activity_evidence &evidence) noexcept
{
	if (!telemetry_session_ref_is_valid(evidence.session) ||
	    !telemetry_activity_evidence_kind_is_valid(evidence.kind) ||
	    evidence.reserved[0] != 0U || evidence.reserved[1] != 0U ||
	    evidence.reserved[2] != 0U || !telemetry_quality_mask_is_valid(evidence.quality_flags))
		return false;
	if (evidence.kind == telemetry_activity_evidence_kind::linkdead)
		return telemetry_connection_id_is_zero(evidence.connection);
	return telemetry_connection_id_is_valid(evidence.connection);
}

constexpr bool telemetry_activity_context_snapshot_is_valid(
	const telemetry_activity_context_snapshot &snapshot) noexcept
{
	return telemetry_session_ref_is_valid(snapshot.session) &&
	       telemetry_connection_id_is_valid(snapshot.connection) &&
	       telemetry_dimensions_are_valid(snapshot.dimensions) &&
	       telemetry_activity_context_flags_are_valid(snapshot.context_flags) &&
	       snapshot.config_id != TELEMETRY_UNKNOWN_ID && snapshot.classifier_version != 0U &&
	       snapshot.policy_version != 0U &&
	       telemetry_quality_mask_is_valid(snapshot.quality_flags);
}

constexpr bool
telemetry_activity_pulse_request_is_valid(const telemetry_activity_pulse_request &request) noexcept
{
	return request.reserved == 0U && request.reserved2 == 0U;
}

static_assert(std::is_trivially_copyable_v<telemetry_activity_state_config>);
static_assert(std::is_trivially_copyable_v<telemetry_activity_evidence>);
static_assert(std::is_trivially_copyable_v<telemetry_activity_context_snapshot>);
static_assert(std::is_trivially_copyable_v<telemetry_activity_result>);
static_assert(std::is_trivially_copyable_v<telemetry_activity_pulse_request>);
static_assert(std::is_trivially_copyable_v<telemetry_activity_slot>);
static_assert(std::is_trivially_copyable_v<telemetry_activity_state>);

telemetry_activity_outcome
telemetry_activity_state_init(telemetry_activity_state *state,
			      const telemetry_activity_state_config *config) noexcept;
void telemetry_activity_state_reset(telemetry_activity_state *state) noexcept;

/* Configuration must be admitted before an interval may reference its identity. */
telemetry_activity_result
telemetry_activity_state_publish_config(telemetry_activity_state *state,
					telemetry_config_snapshot config) noexcept;
bool telemetry_activity_state_config_is_admitted(const telemetry_activity_state *state,
						 telemetry_config_id config_id) noexcept;

telemetry_activity_result telemetry_activity_state_enter(telemetry_activity_state *state,
							 telemetry_session_enter enter) noexcept;
telemetry_activity_result
telemetry_activity_state_record_evidence(telemetry_activity_state *state,
					 telemetry_activity_evidence evidence) noexcept;
telemetry_activity_result
telemetry_activity_state_update_context(telemetry_activity_state *state,
					telemetry_activity_context_snapshot snapshot) noexcept;
telemetry_activity_result
telemetry_activity_state_transition(telemetry_activity_state *state,
				    telemetry_connection_transition transition) noexcept;
telemetry_activity_result
telemetry_activity_state_flush_at(telemetry_activity_state *state, telemetry_session_ref session,
				  telemetry_monotonic_usec at_monotonic_usec,
				  telemetry_utc_usec at_utc_usec) noexcept;
telemetry_activity_result telemetry_activity_state_flush(telemetry_activity_state *state,
							 telemetry_session_ref session) noexcept;
telemetry_activity_result telemetry_activity_state_exit(telemetry_activity_state *state,
							telemetry_session_exit exit) noexcept;
telemetry_activity_pulse_result
telemetry_activity_state_pulse(telemetry_activity_state *state,
			       telemetry_activity_pulse_request request) noexcept;
telemetry_activity_result
telemetry_activity_state_retire_expired(telemetry_activity_state *state) noexcept;

bool telemetry_activity_state_copy_view(const telemetry_activity_state *state,
					telemetry_session_ref session,
					telemetry_activity_state_view *view) noexcept;
telemetry_activity_state_stats
telemetry_activity_state_stats_copy(const telemetry_activity_state *state) noexcept;

#endif
