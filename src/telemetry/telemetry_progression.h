#ifndef DURIS_TELEMETRY_PROGRESSION_H
#define DURIS_TELEMETRY_PROGRESSION_H

#include "telemetry/telemetry_types.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

/*
 * Pure H progression capture.  This module accepts only value snapshots and
 * emits one bounded tagged fact; it retains no character, descriptor, SQL
 * handle, callback or unbounded source map.  Durable/reconciled status is
 * reserved for later recovery/ledger work; the gameplay adapter emits only
 * observed_mutable facts.
 */
struct telemetry_progression_observation
{
	telemetry_progression_kind kind;
	telemetry_progression_source source;
	telemetry_progression_reason reason;
	telemetry_progression_observation_status observation_status;
	std::uint32_t modifier_flags;
	std::int64_t requested_xp;
	std::int64_t computed_xp;
	std::int64_t applied_xp;
	std::int64_t before_exp;
	std::int64_t after_exp;
	std::uint16_t before_level;
	std::uint16_t after_level;
	std::uint32_t reserved;
	std::uint64_t threshold_xp;
	telemetry_quality_mask quality_flags;
};

enum class telemetry_progression_outcome : std::uint8_t
{
	accepted = 0,
	invalid = 1,
	sink_rejected = 2,
	allocator_exhausted = 3,
};

struct telemetry_progression_sink
{
	using emit_function = bool (*)(void *, const telemetry_record *) noexcept;

	emit_function emit;
	void *context;
};

struct telemetry_progression_record_key_allocator
{
	using next_function = bool (*)(void *, telemetry_record_kind,
				       telemetry_record_key *) noexcept;

	next_function next;
	void *context;
};

struct telemetry_progression_state_config
{
	telemetry_producer_id producer;
	telemetry_progression_sink sink;
	telemetry_progression_record_key_allocator key_allocator;
};

struct telemetry_progression_state
{
	telemetry_producer_id producer;
	telemetry_progression_sink sink;
	telemetry_progression_record_key_allocator key_allocator;
	std::uint8_t initialized;
	std::uint8_t reserved[7];
	std::uint64_t accepted_total;
	std::uint64_t dropped_total;
};

struct telemetry_progression_result
{
	telemetry_progression_outcome outcome;
	std::uint8_t reserved[3];
	telemetry_record_key record;
	telemetry_quality_mask quality_flags;
};

constexpr bool telemetry_progression_signed_delta_is_valid(std::int64_t before, std::int64_t after,
							   std::int64_t applied) noexcept
{
	return telemetry_signed_delta_is_valid(before, after, applied);
}

constexpr bool telemetry_progression_observation_is_valid(
	const telemetry_progression_observation &observation) noexcept
{
	if (!telemetry_progression_kind_is_valid(observation.kind) ||
	    !telemetry_progression_source_is_valid(observation.source) ||
	    !telemetry_progression_reason_is_valid(observation.reason) ||
	    !telemetry_progression_observation_status_is_valid(observation.observation_status) ||
	    !telemetry_progression_modifier_flags_are_valid(observation.modifier_flags) ||
	    observation.reserved != 0U ||
	    !telemetry_quality_mask_is_valid(observation.quality_flags))
		return false;
	if (observation.kind == telemetry_progression_kind::experience_observed)
		return observation.before_level == observation.after_level &&
		       observation.threshold_xp == 0U &&
		       telemetry_progression_signed_delta_is_valid(observation.before_exp,
								   observation.after_exp,
								   observation.applied_xp);
	if (observation.requested_xp != 0 || observation.computed_xp != 0 ||
	    observation.applied_xp != 0U || observation.before_exp != 0 ||
	    observation.after_exp != 0 || observation.before_level == observation.after_level)
		return false;
	if (observation.kind == telemetry_progression_kind::level_advanced)
		return observation.after_level > observation.before_level;
	return observation.kind == telemetry_progression_kind::level_lost &&
	       observation.after_level < observation.before_level;
}

telemetry_progression_observation telemetry_progression_make_experience(
	telemetry_progression_source source, telemetry_progression_reason reason,
	std::int64_t requested_xp, std::int64_t computed_xp, std::int64_t before_exp,
	std::int64_t after_exp, std::uint16_t level, std::uint32_t modifier_flags,
	telemetry_quality_mask quality_flags) noexcept;

telemetry_progression_observation telemetry_progression_make_level_transition(
	telemetry_progression_kind kind, telemetry_progression_source source,
	telemetry_progression_reason reason, std::uint16_t before_level, std::uint16_t after_level,
	std::uint64_t threshold_xp, std::uint32_t modifier_flags,
	telemetry_quality_mask quality_flags) noexcept;

telemetry_progression_outcome
telemetry_progression_state_init(telemetry_progression_state *state,
				 const telemetry_progression_state_config *config) noexcept;
void telemetry_progression_state_reset(telemetry_progression_state *state) noexcept;
telemetry_progression_result telemetry_progression_state_record(
	telemetry_progression_state *state, const telemetry_session_ref &session,
	const telemetry_connection_id &connection, telemetry_monotonic_usec at_monotonic_usec,
	telemetry_utc_usec at_utc_usec, const telemetry_dimensions &dimensions,
	telemetry_config_id config_id, std::uint32_t classifier_version,
	std::uint32_t policy_version, telemetry_progression_observation observation) noexcept;

static_assert(std::is_trivially_copyable_v<telemetry_progression_observation>);
static_assert(std::is_standard_layout_v<telemetry_progression_observation>);
static_assert(std::is_trivially_copyable_v<telemetry_progression_state>);
static_assert(std::is_trivially_copyable_v<telemetry_progression_result>);

#endif
