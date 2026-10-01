#ifndef DURIS_TELEMETRY_ENCOUNTER_H
#define DURIS_TELEMETRY_ENCOUNTER_H

#include "telemetry/telemetry_types.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

/*
 * Encounter capture is deliberately a small, fixed-memory state machine.  It
 * describes an observed run attempt; it never owns a character, SQL handle,
 * string, or gameplay authority.  The runtime supplies an immutable event
 * sink when it wants the state machine's value-only events on the queue.
 */
inline constexpr std::size_t TELEMETRY_ENCOUNTER_MAX_ACTIVE = 128U;
inline constexpr std::size_t TELEMETRY_ENCOUNTER_MAX_PARTICIPANTS = 64U;
inline constexpr std::size_t TELEMETRY_ENCOUNTER_MAX_EVENTS_PER_RUN = 256U;
inline constexpr std::size_t TELEMETRY_ENCOUNTER_TERMINAL_CACHE = 64U;

enum class telemetry_encounter_update_outcome : std::uint8_t
{
	accepted = 0,
	joined_existing = 1,
	idempotent = 2,
	duplicate_conflict = 3,
	not_found = 4,
	capacity_full = 5,
	invalid = 6,
	sink_rejected = 7,
	bounded_overflow = 8,
};

/* The engine emits the same fixed value that the queue stores. */
using telemetry_encounter_event = telemetry_encounter_payload;

using telemetry_encounter_event_sink = bool (*)(void *context,
						const telemetry_encounter_event &event) noexcept;

struct telemetry_encounter_participant_state
{
	std::uint8_t occupied;
	std::uint8_t active;
	std::uint16_t reserved;
	telemetry_encounter_participant participant;
	telemetry_monotonic_usec segment_start_usec;
	telemetry_monotonic_usec last_observed_usec;
	telemetry_duration_usec participant_usec;
};

struct telemetry_encounter_slot
{
	std::uint8_t occupied;
	std::uint8_t terminal;
	std::uint16_t participant_count;
	std::uint16_t expected_credit_count;
	std::uint16_t event_count;
	telemetry_encounter_id encounter;
	telemetry_encounter_mode mode;
	telemetry_encounter_outcome outcome;
	std::uint8_t reserved[2];
	telemetry_encounter_source source;
	telemetry_monotonic_usec start_monotonic_usec;
	telemetry_utc_usec start_utc_usec;
	telemetry_quality_mask quality_flags;
	telemetry_encounter_participant_state participants[TELEMETRY_ENCOUNTER_MAX_PARTICIPANTS];
};

struct telemetry_encounter_terminal
{
	std::uint8_t occupied;
	std::uint8_t reserved[7];
	telemetry_encounter_id encounter;
	telemetry_encounter_outcome outcome;
};

struct telemetry_encounter_state
{
	telemetry_encounter_slot slots[TELEMETRY_ENCOUNTER_MAX_ACTIVE];
	telemetry_encounter_terminal terminal[TELEMETRY_ENCOUNTER_TERMINAL_CACHE];
	std::uint16_t terminal_next;
	std::uint16_t reserved;
};

struct telemetry_encounter_update
{
	telemetry_encounter_update_outcome outcome;
	std::uint8_t events_attempted;
	std::uint8_t events_accepted;
	std::uint8_t participants_active;
	telemetry_quality_mask quality_flags;
	telemetry_encounter_id encounter;
};

void telemetry_encounter_state_init(telemetry_encounter_state *state) noexcept;

telemetry_encounter_update
telemetry_encounter_begin(telemetry_encounter_state *state, telemetry_encounter_id encounter,
			  telemetry_encounter_source source, telemetry_encounter_mode mode,
			  telemetry_encounter_participant participant,
			  telemetry_monotonic_usec at_monotonic_usec,
			  telemetry_utc_usec at_utc_usec, telemetry_encounter_event_sink sink,
			  void *sink_context) noexcept;

telemetry_encounter_update
telemetry_encounter_join_group(telemetry_encounter_state *state, telemetry_encounter_source source,
			       telemetry_encounter_participant participant,
			       telemetry_monotonic_usec at_monotonic_usec,
			       telemetry_utc_usec at_utc_usec, telemetry_encounter_event_sink sink,
			       void *sink_context) noexcept;

telemetry_encounter_update telemetry_encounter_observe(telemetry_encounter_state *state,
						       telemetry_encounter_source source,
						       telemetry_encounter_participant participant,
						       telemetry_monotonic_usec at_monotonic_usec,
						       telemetry_utc_usec at_utc_usec,
						       telemetry_encounter_event_sink sink,
						       void *sink_context) noexcept;

telemetry_encounter_update telemetry_encounter_leave(telemetry_encounter_state *state,
						     telemetry_encounter_participant participant,
						     telemetry_encounter_outcome outcome,
						     telemetry_monotonic_usec at_monotonic_usec,
						     telemetry_utc_usec at_utc_usec,
						     telemetry_encounter_event_sink sink,
						     void *sink_context) noexcept;

telemetry_encounter_update
telemetry_encounter_close(telemetry_encounter_state *state, telemetry_encounter_id encounter,
			  telemetry_encounter_outcome outcome, std::uint16_t expected_credit_count,
			  telemetry_monotonic_usec at_monotonic_usec,
			  telemetry_utc_usec at_utc_usec, telemetry_encounter_event_sink sink,
			  void *sink_context) noexcept;

telemetry_encounter_update telemetry_encounter_close_for_participant(
	telemetry_encounter_state *state, telemetry_encounter_participant participant,
	telemetry_encounter_outcome outcome, std::uint16_t expected_credit_count,
	telemetry_monotonic_usec at_monotonic_usec, telemetry_utc_usec at_utc_usec,
	telemetry_encounter_event_sink sink, void *sink_context) noexcept;

telemetry_encounter_update telemetry_encounter_close_all(telemetry_encounter_state *state,
							 telemetry_encounter_outcome outcome,
							 telemetry_monotonic_usec at_monotonic_usec,
							 telemetry_utc_usec at_utc_usec,
							 telemetry_encounter_event_sink sink,
							 void *sink_context) noexcept;

static_assert(std::is_trivially_copyable_v<telemetry_encounter_event>);
static_assert(std::is_standard_layout_v<telemetry_encounter_event>);
static_assert(std::is_trivially_copyable_v<telemetry_encounter_state>);

#endif
