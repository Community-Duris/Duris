#include "telemetry/telemetry_progression.h"

#include <limits>

namespace
{

telemetry_progression_result result_with(telemetry_progression_outcome outcome) noexcept
{
	telemetry_progression_result result{};
	result.outcome = outcome;
	return result;
}

void saturating_increment(std::uint64_t &value) noexcept
{
	if (value != std::numeric_limits<std::uint64_t>::max())
		++value;
}

bool producer_matches(const telemetry_record_key &key,
		      const telemetry_producer_id &producer) noexcept
{
	return key.producer.boot_id == producer.boot_id &&
	       key.producer.process_id == producer.process_id;
}

std::int64_t signed_delta_or_zero(std::int64_t before, std::int64_t after) noexcept
{
	if (after >= before)
	{
		const std::uint64_t delta =
			static_cast<std::uint64_t>(after) - static_cast<std::uint64_t>(before);
		return delta <= static_cast<std::uint64_t>(
					std::numeric_limits<std::int64_t>::max()) ?
			       static_cast<std::int64_t>(delta) :
			       0;
	}
	const std::uint64_t delta =
		static_cast<std::uint64_t>(before) - static_cast<std::uint64_t>(after);
	return delta <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ?
		       -static_cast<std::int64_t>(delta) :
		       0;
}

} // namespace

telemetry_progression_observation telemetry_progression_make_experience(
	telemetry_progression_source source, telemetry_progression_reason reason,
	std::int64_t requested_xp, std::int64_t computed_xp, std::int64_t before_exp,
	std::int64_t after_exp, std::uint16_t level, std::uint32_t modifier_flags,
	telemetry_quality_mask quality_flags) noexcept
{
	telemetry_progression_observation observation{};
	observation.kind = telemetry_progression_kind::experience_observed;
	observation.source = source;
	observation.reason = reason;
	observation.observation_status = telemetry_progression_observation_status::observed_mutable;
	observation.modifier_flags = modifier_flags;
	observation.requested_xp = requested_xp;
	observation.computed_xp = computed_xp;
	observation.applied_xp = signed_delta_or_zero(before_exp, after_exp);
	observation.before_exp = before_exp;
	observation.after_exp = after_exp;
	observation.before_level = level;
	observation.after_level = level;
	observation.quality_flags = quality_flags;
	return observation;
}

telemetry_progression_observation telemetry_progression_make_level_transition(
	telemetry_progression_kind kind, telemetry_progression_source source,
	telemetry_progression_reason reason, std::uint16_t before_level, std::uint16_t after_level,
	std::uint64_t threshold_xp, std::uint32_t modifier_flags,
	telemetry_quality_mask quality_flags) noexcept
{
	telemetry_progression_observation observation{};
	observation.kind = kind;
	observation.source = source;
	observation.reason = reason;
	observation.observation_status = telemetry_progression_observation_status::observed_mutable;
	observation.modifier_flags = modifier_flags;
	observation.before_level = before_level;
	observation.after_level = after_level;
	observation.threshold_xp = threshold_xp;
	observation.quality_flags = quality_flags;
	return observation;
}

telemetry_progression_outcome
telemetry_progression_state_init(telemetry_progression_state *state,
				 const telemetry_progression_state_config *config) noexcept
{
	if (state == nullptr || config == nullptr ||
	    !telemetry_producer_id_is_valid(config->producer) || config->sink.emit == nullptr ||
	    config->key_allocator.next == nullptr)
		return telemetry_progression_outcome::invalid;
	*state = {};
	state->producer = config->producer;
	state->sink = config->sink;
	state->key_allocator = config->key_allocator;
	state->initialized = 1U;
	return telemetry_progression_outcome::accepted;
}

void telemetry_progression_state_reset(telemetry_progression_state *state) noexcept
{
	if (state != nullptr)
		*state = {};
}

telemetry_progression_result telemetry_progression_state_record(
	telemetry_progression_state *state, const telemetry_session_ref &session,
	const telemetry_connection_id &connection, telemetry_monotonic_usec at_monotonic_usec,
	telemetry_utc_usec at_utc_usec, const telemetry_dimensions &dimensions,
	telemetry_config_id config_id, std::uint32_t classifier_version,
	std::uint32_t policy_version, telemetry_progression_observation observation) noexcept
{
	if (state == nullptr || state->initialized == 0U ||
	    !telemetry_session_ref_is_valid(session) ||
	    !telemetry_connection_reference_is_valid(connection) ||
	    !telemetry_dimensions_are_valid(dimensions) || config_id == TELEMETRY_UNKNOWN_ID ||
	    classifier_version == 0U || policy_version == 0U ||
	    !telemetry_progression_observation_is_valid(observation))
		return result_with(telemetry_progression_outcome::invalid);

	telemetry_record record{};
	record.header.schema_version = TELEMETRY_SCHEMA_VERSION;
	record.header.kind = telemetry_record_kind::progression;
	record.header.occurrence_utc_usec = at_utc_usec;
	telemetry_progression_payload &payload = record.payload.progression;
	payload.session = session;
	payload.connection = connection;
	payload.at_monotonic_usec = at_monotonic_usec;
	payload.at_utc_usec = at_utc_usec;
	payload.kind = observation.kind;
	payload.source = observation.source;
	payload.reason = observation.reason;
	payload.observation_status = observation.observation_status;
	payload.modifier_flags = observation.modifier_flags;
	payload.requested_xp = observation.requested_xp;
	payload.computed_xp = observation.computed_xp;
	payload.applied_xp = observation.applied_xp;
	payload.before_exp = observation.before_exp;
	payload.after_exp = observation.after_exp;
	payload.before_level = observation.before_level;
	payload.after_level = observation.after_level;
	payload.reserved = 0U;
	payload.threshold_xp = observation.threshold_xp;
	payload.dimensions = dimensions;
	payload.config_id = config_id;
	payload.classifier_version = classifier_version;
	payload.policy_version = policy_version;
	payload.quality_flags = observation.quality_flags;
	if (!telemetry_progression_payload_is_valid(payload))
		return result_with(telemetry_progression_outcome::invalid);

	telemetry_record_key key{};
	if (!state->key_allocator.next(state->key_allocator.context,
				       telemetry_record_kind::progression, &key) ||
	    !telemetry_record_key_is_valid(key) || !producer_matches(key, state->producer))
	{
		saturating_increment(state->dropped_total);
		telemetry_progression_result result =
			result_with(telemetry_progression_outcome::allocator_exhausted);
		result.quality_flags = TELEMETRY_QUALITY_SEQUENCE_GAP;
		return result;
	}
	record.header.key = key;
	if (!telemetry_record_is_valid(record))
	{
		saturating_increment(state->dropped_total);
		return result_with(telemetry_progression_outcome::invalid);
	}
	telemetry_progression_result result =
		result_with(state->sink.emit(state->sink.context, &record) ?
				    telemetry_progression_outcome::accepted :
				    telemetry_progression_outcome::sink_rejected);
	result.record = key;
	result.quality_flags = payload.quality_flags;
	if (result.outcome == telemetry_progression_outcome::accepted)
		saturating_increment(state->accepted_total);
	else
	{
		saturating_increment(state->dropped_total);
		result.quality_flags |= TELEMETRY_QUALITY_QUEUE_DROP;
	}
	return result;
}
