#include "telemetry/telemetry_control.h"

#include <bit>
#include <limits>
#include <string_view>
#include <type_traits>

namespace
{
using observation = telemetry_control_observation;
using actor_context = telemetry_control_actor_context;
using association = telemetry_control_association;
using kind = telemetry_control_kind;
using family = telemetry_control_family;
using result = telemetry_control_result;
using boundary = telemetry_control_boundary;
using state = telemetry_control_state;
using entry = telemetry_control_target_state;
using outcome = telemetry_control_outcome;
using update = telemetry_control_update;

template <std::size_t Width, bool Signed, typename T>
void encode_value(T value, std::uint8_t *&bytes) noexcept
{
	static_assert(sizeof(T) == Width);
	static_assert(std::is_signed_v<T> == Signed);
	const auto number = static_cast<std::uint64_t>(value);
	for (std::size_t index = 0U; index < Width; ++index)
		*bytes++ = static_cast<std::uint8_t>(number >> ((Width - 1U - index) * 8U));
}

template <std::size_t Width, bool Signed, typename T>
void decode_value(T &value, const std::uint8_t *&bytes) noexcept
{
	static_assert(sizeof(T) == Width);
	static_assert(std::is_signed_v<T> == Signed);
	std::uint64_t number = 0U;
	for (std::size_t index = 0U; index < Width; ++index)
		number = (number << 8U) | *bytes++;
	if constexpr (Signed)
		value = std::bit_cast<T>(static_cast<std::make_unsigned_t<T>>(number));
	else
		value = static_cast<T>(number);
}
} // namespace

bool telemetry_control_observation_same_key(const observation &a, const observation &b) noexcept
{
	return a.sequence != 0U && a.sequence == b.sequence &&
	       telemetry_producer_id_is_valid(a.producer) &&
	       a.producer.boot_id == b.producer.boot_id &&
	       a.producer.process_id == b.producer.process_id;
}

bool telemetry_control_observation_equal(const observation &a, const observation &b) noexcept
{
	if (!telemetry_control_observation_is_valid(a) ||
	    !telemetry_control_observation_is_valid(b))
		return false;
#define TELEMETRY_CONTROL_FIELD(name, member, width, signed_value) \
	if (a.member != b.member)                                  \
		return false;
#include "telemetry/telemetry_control_fields.inc"
#undef TELEMETRY_CONTROL_FIELD
	return true;
}

bool telemetry_control_observation_encode(const observation &v, std::uint8_t *bytes,
					  std::size_t length) noexcept
{
	if (!bytes || length != TELEMETRY_CONTROL_WIRE_BYTES ||
	    !telemetry_control_observation_is_valid(v))
		return false;
#define TELEMETRY_CONTROL_FIELD(name, member, width, signed_value) \
	encode_value<width, signed_value>(v.member, bytes);
#include "telemetry/telemetry_control_fields.inc"
#undef TELEMETRY_CONTROL_FIELD
	return true;
}

bool telemetry_control_observation_decode(const std::uint8_t *bytes, std::size_t length,
					  observation *output) noexcept
{
	if (!output)
		return false;
	*output = {};
	if (!bytes || length != TELEMETRY_CONTROL_WIRE_BYTES)
		return false;
	observation v{};
#define TELEMETRY_CONTROL_FIELD(name, member, width, signed_value) \
	decode_value<width, signed_value>(v.member, bytes);
#include "telemetry/telemetry_control_fields.inc"
#undef TELEMETRY_CONTROL_FIELD
	if (!telemetry_control_observation_is_valid(v))
		return false;
	*output = v;
	return true;
}

namespace
{
update changed(outcome result = outcome::accepted) noexcept
{
	return { result, 0U, 0U, 0U };
}

bool scoped(const state &s, const observation &v) noexcept
{
	return v.producer.boot_id == s.producer.boot_id &&
	       v.producer.process_id == s.producer.process_id &&
	       v.scope.environment_id == s.environment_id && v.scope.season_id == s.season_id;
}

bool same_target(const entry &e, telemetry_battle_actor_key key) noexcept
{
	return e.occupied && e.value.target.actor.actor_id == key.id &&
	       e.value.target.actor.kind == key.kind;
}

entry *find_target(state &s, telemetry_battle_actor_key key) noexcept
{
	for (auto &e : s.targets)
		if (same_target(e, key))
			return &e;
	return nullptr;
}

constexpr bool context_field(std::string_view name) noexcept
{
	return (name.starts_with("ctl_target_") && name != "ctl_target_association_revision" &&
		name != "ctl_target_association_fact_sequence") ||
	       name.starts_with("ctl_scope_") || name == "ctl_environment_id" ||
	       name == "ctl_season_id" || name == "ctl_config_id" ||
	       name == "ctl_classifier_version" || name == "ctl_policy_version" ||
	       name == "ctl_build_version" || name == "ctl_content_version" ||
	       name == "ctl_state_available" || name == "ctl_duration_coverage";
}

bool same_context(const observation &a, const observation &b) noexcept
{
#define TELEMETRY_CONTROL_FIELD(name, member, width, signed_value) \
	if constexpr (context_field(#name))                        \
		if (a.member != b.member)                          \
			return false;
#include "telemetry/telemetry_control_fields.inc"
#undef TELEMETRY_CONTROL_FIELD
	return true;
}

void emit(state &s, observation &v, telemetry_control_sink sink, void *opaque, update &r) noexcept
{
	if (s.next_sequence == 0U)
	{
		r.outcome = outcome::sequence_exhausted;
		r.quality_flags |= TELEMETRY_QUALITY_CARDINALITY_OVERFLOW;
		return;
	}
	v.sequence = s.next_sequence;
	if (!telemetry_control_observation_is_valid(v))
	{
		r.outcome = outcome::invalid;
		return;
	}
	s.next_sequence =
		v.sequence == std::numeric_limits<telemetry_sequence>::max() ? 0U : v.sequence + 1U;
	++r.rows_attempted;
	if (sink(opaque, v))
		++r.rows_accepted;
	else
	{
		r.outcome = outcome::sink_rejected;
		r.quality_flags |= TELEMETRY_QUALITY_QUEUE_DROP;
	}
}

observation fresh_entry(const observation &point) noexcept
{
	auto v = point;
	v.kind = kind::state_entry;
	v.boundary = boundary::context_changed;
	v.previous_state_sequence = 0U;
	v.start_usec = v.at_usec;
	v.start_utc_usec = v.at_utc_usec;
	v.decision_usec = v.at_usec;
	v.decision_utc_usec = v.at_utc_usec;
	v.before_mask = v.after_mask;
	return v;
}

void begin(state &s, entry &e, observation point, telemetry_control_sink sink, void *opaque,
	   update &r) noexcept
{
	const auto accepted_before = r.rows_accepted;
	const auto attempted_before = r.rows_attempted;
	emit(s, point, sink, opaque, r);
	if (r.rows_attempted == attempted_before)
		return;
	e = { point, 1U, static_cast<std::uint8_t>(r.rows_accepted == accepted_before) };
}

void seal(state &s, entry &e, const observation &next, bool actual_transition,
	  telemetry_control_boundary reason, telemetry_control_sink sink, void *opaque,
	  update &r) noexcept
{
	auto v = e.value;
	v.kind = kind::state_interval;
	v.previous_state_sequence = e.value.sequence;
	v.boundary = reason;
	v.decision_usec = next.decision_usec;
	v.decision_utc_usec = next.decision_utc_usec;
	if (actual_transition)
	{
		v.at_usec = next.at_usec;
		v.at_utc_usec = next.at_utc_usec;
		v.after_mask = next.after_mask;
		v.last_target_association_revision = next.target_association.revision;
		v.last_target_association_fact_sequence = next.target_association.fact_sequence;
		v.quality_flags |= next.quality_flags;
	}
	if (v.at_usec != v.decision_usec)
		v.quality_flags |= TELEMETRY_QUALITY_UNCLOSED_TAIL;
	if ((v.start_utc_usec != TELEMETRY_UTC_UNKNOWN && v.at_utc_usec != TELEMETRY_UTC_UNKNOWN &&
	     v.at_utc_usec < v.start_utc_usec) ||
	    (v.at_utc_usec != TELEMETRY_UTC_UNKNOWN &&
	     v.decision_utc_usec != TELEMETRY_UTC_UNKNOWN && v.decision_utc_usec < v.at_utc_usec))
		v.quality_flags |= TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
	const auto before = r.rows_accepted;
	emit(s, v, sink, opaque, r);
	if (actual_transition && r.rows_attempted != 0U)
	{
		e.value = fresh_entry(next);
		e.value.sequence = v.sequence;
		e.delivery_gap = r.rows_accepted == before;
	}
}

void gap(state &s, const observation &point, telemetry_control_boundary reason,
	 telemetry_control_sink sink, void *opaque, update &r) noexcept
{
	auto v = point;
	v.kind = kind::source_gap;
	v.boundary = reason;
	v.previous_state_sequence = 0U;
	v.before_mask = v.after_mask = v.state_available = v.duration_coverage = 0U;
	v.quality_flags |= TELEMETRY_QUALITY_CONTEXT_UNKNOWN;
	if (reason == boundary::capacity_refused)
		v.quality_flags |= TELEMETRY_QUALITY_CARDINALITY_OVERFLOW;
	if (reason == boundary::configuration_unavailable)
	{
		v.scope.config_id = 0U;
		v.scope.classifier_version = v.scope.policy_version = 0U;
		v.build_version = v.content_version = 0U;
	}
	emit(s, v, sink, opaque, r);
}
} // namespace

bool telemetry_control_state_init(state *s, telemetry_producer_id producer,
				  telemetry_environment_id environment,
				  telemetry_season_id season) noexcept
{
	if (!s)
		return false;
	*s = {};
	if (!telemetry_producer_id_is_valid(producer) || environment == 0U || season == 0U)
		return false;
	s->producer = producer;
	s->environment_id = environment;
	s->season_id = season;
	s->next_sequence = 1U;
	s->initialized = 1U;
	return true;
}

update telemetry_control_observe(state *s, const observation &raw_input,
				 telemetry_control_sink sink, void *opaque) noexcept
{
	auto input = raw_input;
	if (!s || !s->initialized || !sink || input.sequence != 0U ||
	    input.previous_state_sequence != 0U || !scoped(*s, input) ||
	    (input.kind != kind::resolution && input.kind != kind::state_entry &&
	     input.kind != kind::source_gap))
		return changed(outcome::invalid);
	if (s->next_sequence == 0U)
	{
		auto r = changed(outcome::sequence_exhausted);
		r.quality_flags = TELEMETRY_QUALITY_CARDINALITY_OVERFLOW;
		return r;
	}
	auto probe = input;
	probe.sequence = s->next_sequence;
	if (!telemetry_control_observation_is_valid(probe))
		return changed(outcome::invalid);
	if (input.decision_usec < s->latest_decision_usec)
	{
		auto r = changed(outcome::invalid);
		r.quality_flags = TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
		return r;
	}
	auto r = changed();
	if (input.kind == kind::resolution)
	{
		s->latest_decision_usec = input.decision_usec;
		auto v = input;
		emit(*s, v, sink, opaque, r);
		return r;
	}
	const telemetry_battle_actor_key key{ input.target.actor.actor_id,
					      input.target.actor.kind };
	auto *e = find_target(*s, key);
	if (e &&
	    (input.at_usec < e->value.at_usec ||
	     (input.target_association.battle_sequence ==
		      e->value.target_association.battle_sequence &&
	      (input.target_association.revision < e->value.last_target_association_revision ||
	       input.target_association.fact_sequence <
		       e->value.last_target_association_fact_sequence ||
	       (input.target_association.fact_sequence ==
			e->value.last_target_association_fact_sequence &&
		(input.target_association.revision != e->value.last_target_association_revision ||
		 (input.kind != kind::source_gap && !same_context(e->value, input))))))))
		return changed(outcome::invalid);
	if (e && e->value.at_utc_usec != TELEMETRY_UTC_UNKNOWN &&
	    input.at_utc_usec != TELEMETRY_UTC_UNKNOWN && input.at_utc_usec < e->value.at_utc_usec)
		input.quality_flags |= TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
	s->latest_decision_usec = input.decision_usec;
	if (input.kind == kind::source_gap)
	{
		if (e && !e->delivery_gap)
			seal(*s, *e, input, false, input.boundary, sink, opaque, r);
		gap(*s, input, input.boundary, sink, opaque, r);
		if (e)
			*e = {};
		return r;
	}
	if (!e)
	{
		for (auto &candidate : s->targets)
			if (!candidate.occupied)
			{
				e = &candidate;
				break;
			}
		if (!e)
		{
			gap(*s, input, boundary::capacity_refused, sink, opaque, r);
			if (r.outcome == outcome::accepted)
				r.outcome = outcome::capacity_full;
			r.quality_flags |= TELEMETRY_QUALITY_CARDINALITY_OVERFLOW;
			return r;
		}
		begin(*s, *e, input, sink, opaque, r);
		return r;
	}
	if (e->delivery_gap)
	{
		gap(*s, input, boundary::source_unavailable, sink, opaque, r);
		begin(*s, *e, fresh_entry(input), sink, opaque, r);
		return r;
	}
	if (!same_context(e->value, input))
	{
		seal(*s, *e, input, false, boundary::context_changed, sink, opaque, r);
		begin(*s, *e, fresh_entry(input), sink, opaque, r);
		return r;
	}
	if (e->value.after_mask != input.after_mask)
	{
		seal(*s, *e, input, true, boundary::state_changed, sink, opaque, r);
		return r;
	}
	e->value.at_usec = input.at_usec;
	e->value.at_utc_usec = input.at_utc_usec;
	e->value.decision_usec = input.decision_usec;
	e->value.decision_utc_usec = input.decision_utc_usec;
	e->value.last_target_association_revision = input.target_association.revision;
	e->value.last_target_association_fact_sequence = input.target_association.fact_sequence;
	e->value.quality_flags |= input.quality_flags;
	if (e->value.start_utc_usec != TELEMETRY_UTC_UNKNOWN &&
	    input.at_utc_usec != TELEMETRY_UTC_UNKNOWN &&
	    input.at_utc_usec < e->value.start_utc_usec)
		e->value.quality_flags |= TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
	r.outcome = outcome::idempotent;
	return r;
}

update telemetry_control_close(state *s, telemetry_battle_actor_key key,
			       telemetry_monotonic_usec decision, telemetry_utc_usec utc,
			       telemetry_control_boundary reason, telemetry_control_sink sink,
			       void *opaque) noexcept
{
	if (!s || !s->initialized || !sink || !telemetry_battle_actor_key_is_valid(key) ||
	    (reason != boundary::actor_left && reason != boundary::battle_ended) ||
	    decision < s->latest_decision_usec)
		return changed(outcome::invalid);
	auto *e = find_target(*s, key);
	if (!e)
		return changed(outcome::not_found);
	auto r = changed();
	auto next = e->value;
	next.decision_usec = decision;
	next.decision_utc_usec = utc;
	s->latest_decision_usec = decision;
	if (e->delivery_gap)
		gap(*s, next, boundary::source_unavailable, sink, opaque, r);
	else
		seal(*s, *e, next, false, reason, sink, opaque, r);
	*e = {};
	return r;
}
