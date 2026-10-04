#include "telemetry/telemetry_battle_contribution.h"

#include <bit>
#include <cstring>
#include <limits>

namespace
{
using outcome = telemetry_battle_contribution_outcome;
using context = telemetry_battle_contribution_context;
using entry = telemetry_battle_contribution_actor_state;
using slot = telemetry_battle_contribution_slot;
using state = telemetry_battle_contribution_state;
using update = telemetry_battle_contribution_update;
using cut = telemetry_battle_contribution_cut;
using end = telemetry_battle_contribution_end;
struct located
{
	slot *owner;
	entry *actor;
};

bool same_producer(telemetry_producer_id a, telemetry_producer_id b) noexcept
{
	return a.boot_id == b.boot_id && a.process_id == b.process_id;
}
bool same_battle(telemetry_battle_id a, telemetry_battle_id b) noexcept
{
	return same_producer(a.producer, b.producer) && a.sequence == b.sequence;
}
bool same_context(const context &a, const context &b) noexcept
{
	if (a.available_metrics != b.available_metrics || a.quality_flags != b.quality_flags)
		return false;
	/* Reuse canonical named context fields, never ABI/padding comparison.
	 * Advancing an association receipt alone does not split identical context. */
	telemetry_battle_fact left{}, right{};
	left.battle = a.battle;
	right.battle = b.battle;
	left.scope = a.scope;
	right.scope = b.scope;
	left.actor = a.actor;
	right.actor = b.actor;
	left.mode = a.mode;
	right.mode = b.mode;
	left.side_status = a.side_status;
	right.side_status = b.side_status;
	left.side = a.side;
	right.side = b.side;
#define TELEMETRY_BATTLE_FIELD(name, member, width, signed_value) \
	if (left.member != right.member)                          \
		return false;
#include "telemetry/telemetry_battle_fields.inc"
#undef TELEMETRY_BATTLE_FIELD
	return true;
}
bool valid_end(end reason) noexcept
{
	return reason >= end::context_changed && reason <= end::source_gap;
}
bool valid_cut(cut point) noexcept
{
	return point.decision_usec >= point.observed_usec;
}
cut at(telemetry_monotonic_usec time, telemetry_utc_usec utc) noexcept
{
	return { time, utc, time, utc };
}
update result(outcome value = outcome::accepted) noexcept
{
	update r{};
	r.outcome = value;
	return r;
}
void add(std::uint64_t &value, std::uint64_t amount, telemetry_quality_mask &quality) noexcept
{
	if (amount > std::numeric_limits<std::uint64_t>::max() - value)
	{
		value = std::numeric_limits<std::uint64_t>::max();
		quality |= TELEMETRY_QUALITY_CARDINALITY_OVERFLOW;
	}
	else
		value += amount;
}
slot *find_battle(state &s, telemetry_battle_id battle) noexcept
{
	for (auto &v : s.slots)
		if (v.occupied && same_battle(v.battle, battle))
			return &v;
	return nullptr;
}
update capacity_refused(state &s, telemetry_battle_id battle) noexcept
{
	auto r = result(outcome::capacity_full);
	r.quality_flags = s.quality_flags;
	if (const auto *owner = find_battle(s, battle))
		r.quality_flags |= owner->quality_flags;
	return r;
}
located find_actor(state &s, telemetry_id actor) noexcept
{
	for (auto &v : s.slots)
		if (v.occupied)
			for (auto &a : v.actors)
				if (a.occupied && a.value.context.actor.actor.actor_id == actor)
					return { &v, &a };
	return {};
}
bool ready(state *s, telemetry_battle_contribution_sink sink) noexcept
{
	return s && s->initialized && sink;
}
bool admitted_context(const state &s, const context &c, std::uint32_t metric) noexcept
{
	return telemetry_battle_contribution_context_is_valid(c) &&
	       (c.available_metrics & metric) == metric &&
	       same_producer(s.producer, c.battle.producer) &&
	       c.scope.environment_id == s.environment_id && c.scope.season_id == s.season_id;
}
bool ordered_ref(const located &found, const context &c) noexcept
{
	if (!found.actor || !same_battle(found.owner->battle, c.battle))
		return true;
	const auto &v = found.actor->value;
	if (c.association_revision < v.last_association_revision ||
	    c.association_fact_sequence < v.last_association_fact_sequence)
		return false;
	if (c.association_fact_sequence == v.last_association_fact_sequence)
		return c.association_revision == v.last_association_revision &&
		       same_context(v.context, c);
	return true;
}
bool ordered_time(state &s, telemetry_monotonic_usec time, telemetry_utc_usec utc) noexcept
{
	if (time < s.latest_usec)
	{
		s.quality_flags |= TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
		return false;
	}
	if (utc != TELEMETRY_UTC_UNKNOWN && s.latest_utc_usec != TELEMETRY_UTC_UNKNOWN &&
	    utc < s.latest_utc_usec)
		s.quality_flags |= TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
	return true;
}
void remember_time(state &s, telemetry_monotonic_usec time, telemetry_utc_usec utc) noexcept
{
	s.latest_usec = time;
	s.latest_utc_usec = utc;
	if (utc == TELEMETRY_UTC_UNKNOWN)
		s.quality_flags |= TELEMETRY_QUALITY_CONTEXT_UNKNOWN;
}
void release(located found) noexcept
{
	*found.actor = {};
	if (--found.owner->actor_count == 0U)
		*found.owner = {};
}
void seal(state &s, located found, cut point, end reason, telemetry_battle_contribution_sink sink,
	  void *opaque, update &r) noexcept
{
	auto &a = *found.actor;
	auto &v = a.value;
	v.cut = point;
	v.end_reason = reason;
	v.quality_flags |= found.owner->quality_flags | s.quality_flags;
	if (reason == end::source_gap)
		v.quality_flags |= TELEMETRY_QUALITY_CONTEXT_UNKNOWN | TELEMETRY_QUALITY_QUEUE_DROP;
	if (a.casting)
	{
		add(v.counters.casting_elapsed_usec, point.observed_usec - a.casting_start_usec,
		    v.quality_flags);
		++v.counters.casting_unresolved;
		v.quality_flags |= TELEMETRY_QUALITY_UNCLOSED_TAIL;
	}
	if (a.engaged)
		add(v.counters.engaged_target_usec, point.observed_usec - a.engagement_start_usec,
		    v.quality_flags);
	if ((v.start_utc_usec != TELEMETRY_UTC_UNKNOWN &&
	     point.observed_utc_usec != TELEMETRY_UTC_UNKNOWN &&
	     point.observed_utc_usec < v.start_utc_usec) ||
	    (point.observed_utc_usec != TELEMETRY_UTC_UNKNOWN &&
	     point.decision_utc_usec != TELEMETRY_UTC_UNKNOWN &&
	     point.decision_utc_usec < point.observed_utc_usec))
		v.quality_flags |= TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
	++r.rows_attempted;
	r.quality_flags |= v.quality_flags;
	if (sink(opaque, v))
		++r.rows_accepted;
	else
	{
		r.outcome = outcome::sink_rejected;
		r.quality_flags |= TELEMETRY_QUALITY_QUEUE_DROP;
		/* A missing finalized segment cannot be repaired by later totals. */
		s.quality_flags |= TELEMETRY_QUALITY_QUEUE_DROP;
	}
	release(found);
}
bool can_allocate(state &s, const context &first, const context *second, const located &a,
		  const located &b) noexcept
{
	const bool distinct = second && second->actor.actor.actor_id != first.actor.actor.actor_id;
	const auto fresh =
		static_cast<unsigned>(!a.actor || !same_context(a.actor->value.context, first)) +
		static_cast<unsigned>(distinct &&
				      (!b.actor || !same_context(b.actor->value.context, *second)));
	if (fresh &&
	    (!s.next_sequence ||
	     fresh - 1U > std::numeric_limits<telemetry_sequence>::max() - s.next_sequence))
	{
		s.quality_flags |= TELEMETRY_QUALITY_CONTEXT_OVERFLOW;
		return false;
	}
	auto *destination = find_battle(s, first.battle);
	if (destination)
	{
		const unsigned needed =
			static_cast<unsigned>(!a.actor || a.owner != destination) +
			static_cast<unsigned>(distinct && (!b.actor || b.owner != destination));
		if (destination->actor_count + needed <= TELEMETRY_BATTLE_CONTRIBUTION_MAX_ACTORS)
			return true;
		destination->quality_flags |= TELEMETRY_QUALITY_CARDINALITY_OVERFLOW;
		return false;
	}
	for (const auto &v : s.slots)
		if (!v.occupied)
			return true;
	/* Changed actors may free their old slot before the new stream starts. */
	for (const auto &v : s.slots)
	{
		const unsigned moving = static_cast<unsigned>(a.owner == &v) +
					static_cast<unsigned>(distinct && b.owner == &v);
		if (v.occupied && v.actor_count == moving)
			return true;
	}
	s.quality_flags |= TELEMETRY_QUALITY_CARDINALITY_OVERFLOW;
	return false;
}
entry *begin(state &s, const context &c, telemetry_monotonic_usec time,
	     telemetry_utc_usec utc) noexcept
{
	auto *owner = find_battle(s, c.battle);
	if (!owner)
		for (auto &v : s.slots)
			if (!v.occupied)
			{
				owner = &v;
				owner->occupied = 1U;
				owner->battle = c.battle;
				break;
			}
	if (!owner)
		return nullptr; // preflight guarantees space
	for (auto &a : owner->actors)
		if (!a.occupied)
		{
			a = {};
			a.occupied = 1U;
			a.value.context = c;
			a.value.sequence = s.next_sequence;
			s.next_sequence =
				s.next_sequence == std::numeric_limits<telemetry_sequence>::max() ?
					0U :
					s.next_sequence + 1U;
			a.value.definition_version = TELEMETRY_BATTLE_CONTRIBUTION_VERSION;
			a.value.start_usec = time;
			a.value.start_utc_usec = utc;
			a.value.quality_flags = c.quality_flags | c.actor.quality_flags |
						owner->quality_flags | s.quality_flags;
			a.last_observed_usec = time;
			a.value.last_association_revision = c.association_revision;
			a.value.last_association_fact_sequence = c.association_fact_sequence;
			++owner->actor_count;
			return &a;
		}
	return nullptr;
}
void observed(entry &a, const context &c, telemetry_monotonic_usec time) noexcept
{
	a.last_observed_usec = time;
	a.value.last_association_revision = c.association_revision;
	a.value.last_association_fact_sequence = c.association_fact_sequence;
}
entry *prepare(state &s, const context &c, located prior, telemetry_monotonic_usec time,
	       telemetry_utc_usec utc, telemetry_battle_contribution_sink sink, void *opaque,
	       update &r) noexcept
{
	if (prior.actor && !same_context(prior.actor->value.context, c))
	{
		seal(s, prior, at(time, utc), end::context_changed, sink, opaque, r);
		prior = {};
	}
	auto *value = prior.actor ? prior.actor : begin(s, c, time, utc);
	if (value)
		observed(*value, c, time);
	return value;
}
enum class pair_kind
{
	damage,
	healing,
	control
};
update pair_event(state *s, const context &first, const context &second, std::uint64_t amount,
		  std::uint64_t effective, telemetry_monotonic_usec time, telemetry_utc_usec utc,
		  std::uint32_t modifiers, telemetry_battle_contribution_sink sink, void *opaque,
		  pair_kind kind) noexcept
{
	const auto metric = kind == pair_kind::damage  ? TELEMETRY_BC_DAMAGE :
			    kind == pair_kind::healing ? TELEMETRY_BC_HEALING :
							 TELEMETRY_BC_CONTROL;
	if (!ready(s, sink) || !admitted_context(*s, first, metric) ||
	    !admitted_context(*s, second, metric) || !same_battle(first.battle, second.battle) ||
	    first.scope.config_id != second.scope.config_id ||
	    first.scope.classifier_version != second.scope.classifier_version ||
	    first.scope.policy_version != second.scope.policy_version ||
	    !telemetry_combat_modifier_flags_are_valid(modifiers) || effective > amount ||
	    !ordered_time(*s, time, utc))
		return result(outcome::invalid);
	const bool same_actor = first.actor.actor.actor_id == second.actor.actor.actor_id;
	if (same_actor && (!same_context(first, second) ||
			   first.association_revision != second.association_revision ||
			   first.association_fact_sequence != second.association_fact_sequence))
		return result(outcome::invalid);
	if (!amount)
		return result(outcome::idempotent);
	const auto a = find_actor(*s, first.actor.actor.actor_id),
		   b = find_actor(*s, second.actor.actor.actor_id);
	if (!ordered_ref(a, first) || !ordered_ref(b, second))
		return result(outcome::invalid);
	if (!can_allocate(*s, first, &second, a, b))
		return capacity_refused(*s, first.battle);
	remember_time(*s, time, utc);
	auto r = result();
	/* Preflight admits both sides together. Release both changed segments before
	 * creating either replacement: only the target's old slot may be reusable. */
	if (a.actor && !same_context(a.actor->value.context, first))
		seal(*s, a, at(time, utc), end::context_changed, sink, opaque, r);
	if (!same_actor)
	{
		const auto prior_target = find_actor(*s, second.actor.actor.actor_id);
		if (prior_target.actor && !same_context(prior_target.actor->value.context, second))
			seal(*s, prior_target, at(time, utc), end::context_changed, sink, opaque,
			     r);
	}
	auto *source = prepare(*s, first, find_actor(*s, first.actor.actor.actor_id), time, utc,
			       sink, opaque, r);
	auto *target = same_actor ? source :
				    prepare(*s, second, find_actor(*s, second.actor.actor.actor_id),
					    time, utc, sink, opaque, r);
	if (!source || !target)
	{
		s->quality_flags |= TELEMETRY_QUALITY_CARDINALITY_OVERFLOW;
		r.quality_flags |= s->quality_flags;
		if (r.outcome != outcome::sink_rejected)
			r.outcome = outcome::capacity_full;
		return r;
	}
	source->value.modifier_flags |= modifiers;
	target->value.modifier_flags |= modifiers;
	source->value.quality_flags |= s->quality_flags;
	target->value.quality_flags |= s->quality_flags;
	auto &x = source->value.counters;
	auto &y = target->value.counters;
	if (kind == pair_kind::damage)
	{
		add(x.damage_dealt, amount, source->value.quality_flags);
		add(y.damage_taken, amount, target->value.quality_flags);
	}
	else if (kind == pair_kind::healing)
	{
		add(x.healing_attempted, amount, source->value.quality_flags);
		add(x.effective_healing, effective, source->value.quality_flags);
		add(x.overhealing, amount - effective, source->value.quality_flags);
		add(y.healing_received, effective, target->value.quality_flags);
	}
	else
	{
		source->value.modifier_flags |= TELEMETRY_COMBAT_MODIFIER_CONTROL;
		add(x.control_applications, amount, source->value.quality_flags);
		add(y.control_received, amount, target->value.quality_flags);
	}
	r.quality_flags |= source->value.quality_flags | target->value.quality_flags;
	return r;
}
update actor_event(state *s, const context &c, telemetry_monotonic_usec time,
		   telemetry_utc_usec utc, telemetry_battle_contribution_sink sink, void *opaque,
		   std::uint32_t metric, unsigned action,
		   const telemetry_battle_actor_key *target) noexcept
{
	if (!ready(s, sink) || !admitted_context(*s, c, metric) || !ordered_time(*s, time, utc) ||
	    (target && (!telemetry_battle_actor_key_is_valid(*target) ||
			target->id == c.actor.actor.actor_id)))
		return result(outcome::invalid);
	const auto old = find_actor(*s, c.actor.actor.actor_id);
	if (!ordered_ref(old, c))
		return result(outcome::invalid);
	auto r = result();
	const bool terminal = action == 1U || action == 2U || (action == 3U && !target);
	if (terminal && (!old.actor || !same_context(old.actor->value.context, c)))
	{
		remember_time(*s, time, utc);
		if (old.actor)
			seal(*s, old, at(time, utc), end::context_changed, sink, opaque, r);
		if (r.outcome != outcome::sink_rejected)
			r.outcome = outcome::not_found;
		return r;
	}
	if (!can_allocate(*s, c, nullptr, old, {}))
		return capacity_refused(*s, c.battle);
	remember_time(*s, time, utc);
	auto *a = prepare(*s, c, old, time, utc, sink, opaque, r);
	if (!a)
		return result(outcome::capacity_full);
	auto &v = a->value;
	v.quality_flags |= s->quality_flags;
	r.quality_flags |= v.quality_flags;
	if (action == 0U)
	{
		if (a->casting)
		{
			r.outcome = outcome::idempotent;
			return r;
		}
		if (v.counters.casting_attempts == std::numeric_limits<std::uint64_t>::max())
		{
			v.quality_flags |= TELEMETRY_QUALITY_CARDINALITY_OVERFLOW;
			r.quality_flags |= v.quality_flags;
			r.outcome = outcome::capacity_full;
			return r;
		}
		++v.counters.casting_attempts;
		a->casting = 1U;
		a->casting_start_usec = time;
	}
	else if (action == 1U || action == 2U)
	{
		if (!a->casting)
		{
			r.outcome = outcome::not_found;
			return r;
		}
		if (action == 1U)
			++v.counters.casting_completions;
		else
			++v.counters.casting_aborts;
		add(v.counters.casting_elapsed_usec, time - a->casting_start_usec, v.quality_flags);
		a->casting = 0U;
	}
	else
	{
		if (a->engaged && target && a->engagement_target.id == target->id &&
		    a->engagement_target.kind == target->kind)
		{
			r.outcome = outcome::idempotent;
			return r;
		}
		if (a->engaged)
			add(v.counters.engaged_target_usec, time - a->engagement_start_usec,
			    v.quality_flags);
		a->engaged = target ? 1U : 0U;
		a->engagement_target = target ? *target : telemetry_battle_actor_key{};
		a->engagement_start_usec = time;
	}
	r.quality_flags |= v.quality_flags;
	return r;
}
} // namespace

bool telemetry_battle_contribution_same_key(const telemetry_battle_contribution_payload &a,
					    const telemetry_battle_contribution_payload &b) noexcept
{
	return same_producer(a.context.battle.producer, b.context.battle.producer) &&
	       a.sequence == b.sequence;
}
bool telemetry_battle_contribution_equal(const telemetry_battle_contribution_payload &a,
					 const telemetry_battle_contribution_payload &b) noexcept
{
	if (!telemetry_battle_contribution_payload_is_valid(a) ||
	    !telemetry_battle_contribution_payload_is_valid(b))
		return false;
#define TELEMETRY_BC_FIELD(name, member, width, signed_value) \
	if (a.member != b.member)                             \
		return false;
#include "telemetry/telemetry_battle_contribution_fields.inc"
#undef TELEMETRY_BC_FIELD
	return true;
}
bool telemetry_battle_contribution_encode(const telemetry_battle_contribution_payload &value,
					  std::uint8_t *bytes, std::size_t length) noexcept
{
	if (!bytes || length != TELEMETRY_BATTLE_CONTRIBUTION_WIRE_BYTES ||
	    !telemetry_battle_contribution_payload_is_valid(value))
		return false;
	auto put = [&]<std::size_t Width, bool Signed, typename T>(T number)
	{
		static_assert(sizeof(T) == Width && std::is_signed_v<T> == Signed);
		const auto bits = static_cast<std::uint64_t>(number);
		for (std::size_t index = 0U; index < Width; ++index)
			*bytes++ = static_cast<std::uint8_t>(bits >> ((Width - 1U - index) * 8U));
	};
#define TELEMETRY_BC_FIELD(name, member, width, signed_value) \
	put.operator()<width, signed_value>(value.member);
#include "telemetry/telemetry_battle_contribution_fields.inc"
#undef TELEMETRY_BC_FIELD
	return true;
}
bool telemetry_battle_contribution_decode(const std::uint8_t *bytes, std::size_t length,
					  telemetry_battle_contribution_payload *value) noexcept
{
	if (!value)
		return false;
	*value = {};
	if (!bytes || length != TELEMETRY_BATTLE_CONTRIBUTION_WIRE_BYTES)
		return false;
	telemetry_battle_contribution_payload decoded{};
	auto get = [&]<std::size_t Width, bool Signed, typename T>(T &number)
	{
		static_assert(sizeof(T) == Width && std::is_signed_v<T> == Signed);
		std::uint64_t bits = 0U;
		for (std::size_t index = 0U; index < Width; ++index)
			bits = (bits << 8U) | *bytes++;
		if constexpr (Signed)
			number = std::bit_cast<T>(static_cast<std::make_unsigned_t<T>>(bits));
		else
			number = static_cast<T>(bits);
	};
#define TELEMETRY_BC_FIELD(name, member, width, signed_value) \
	get.operator()<width, signed_value>(decoded.member);
#include "telemetry/telemetry_battle_contribution_fields.inc"
#undef TELEMETRY_BC_FIELD
	if (!telemetry_battle_contribution_payload_is_valid(decoded))
		return false;
	*value = decoded;
	return true;
}
bool telemetry_battle_contribution_state_init(telemetry_battle_contribution_state *s,
					      telemetry_producer_id producer,
					      telemetry_id environment,
					      telemetry_id season) noexcept
{
	if (!s || !telemetry_producer_id_is_valid(producer) || !environment || !season)
		return false;
	std::memset(s, 0, sizeof(*s));
	s->producer = producer;
	s->environment_id = environment;
	s->season_id = season;
	s->next_sequence = 1U;
	s->latest_utc_usec = TELEMETRY_UTC_UNKNOWN;
	s->initialized = 1U;
	return true;
}
telemetry_battle_contribution_update
telemetry_battle_contribution_damage(state *s, const context &a, const context &b,
				     std::uint64_t amount, telemetry_monotonic_usec time,
				     telemetry_utc_usec utc, std::uint32_t modifiers,
				     telemetry_battle_contribution_sink sink, void *opaque) noexcept
{
	return pair_event(s, a, b, amount, 0U, time, utc, modifiers, sink, opaque,
			  pair_kind::damage);
}
telemetry_battle_contribution_update telemetry_battle_contribution_healing(
	state *s, const context &a, const context &b, std::uint64_t attempted,
	std::uint64_t effective, telemetry_monotonic_usec time, telemetry_utc_usec utc,
	std::uint32_t modifiers, telemetry_battle_contribution_sink sink, void *opaque) noexcept
{
	return pair_event(s, a, b, attempted, effective, time, utc, modifiers, sink, opaque,
			  pair_kind::healing);
}
telemetry_battle_contribution_update telemetry_battle_contribution_control(
	state *s, const context &a, const context &b, std::uint16_t applications,
	telemetry_monotonic_usec time, telemetry_utc_usec utc, std::uint32_t modifiers,
	telemetry_battle_contribution_sink sink, void *opaque) noexcept
{
	return pair_event(s, a, b, applications, 0U, time, utc, modifiers, sink, opaque,
			  pair_kind::control);
}
telemetry_battle_contribution_update telemetry_battle_contribution_cast_attempt(
	state *s, const context &c, telemetry_monotonic_usec time, telemetry_utc_usec utc,
	telemetry_battle_contribution_sink sink, void *opaque) noexcept
{
	return actor_event(s, c, time, utc, sink, opaque, TELEMETRY_BC_CASTING, 0U, nullptr);
}
telemetry_battle_contribution_update telemetry_battle_contribution_cast_finish(
	state *s, const context &c, bool completed, telemetry_monotonic_usec time,
	telemetry_utc_usec utc, telemetry_battle_contribution_sink sink, void *opaque) noexcept
{
	return actor_event(s, c, time, utc, sink, opaque, TELEMETRY_BC_CASTING, completed ? 1U : 2U,
			   nullptr);
}
telemetry_battle_contribution_update telemetry_battle_contribution_engagement(
	state *s, const context &c, const telemetry_battle_actor_key *target,
	telemetry_monotonic_usec time, telemetry_utc_usec utc,
	telemetry_battle_contribution_sink sink, void *opaque) noexcept
{
	return actor_event(s, c, time, utc, sink, opaque, TELEMETRY_BC_ENGAGEMENT, 3U, target);
}
telemetry_battle_contribution_update telemetry_battle_contribution_context_changed(
	state *s, const context &c, telemetry_monotonic_usec time, telemetry_utc_usec utc,
	telemetry_battle_contribution_sink sink, void *opaque) noexcept
{
	if (!ready(s, sink) || !admitted_context(*s, c, 0U) || !ordered_time(*s, time, utc))
		return result(outcome::invalid);
	const auto found = find_actor(*s, c.actor.actor.actor_id);
	if (!ordered_ref(found, c))
		return result(outcome::invalid);
	remember_time(*s, time, utc);
	if (!found.actor)
		return result(outcome::not_found);
	auto r = result();
	if (same_context(found.actor->value.context, c))
	{
		observed(*found.actor, c, time);
		r.outcome = outcome::idempotent;
	}
	else
		seal(*s, found, at(time, utc), end::context_changed, sink, opaque, r);
	return r;
}
telemetry_battle_contribution_update
telemetry_battle_contribution_leave(state *s, telemetry_battle_id battle,
				    telemetry_battle_actor_key actor, cut point, end reason,
				    telemetry_battle_contribution_sink sink, void *opaque) noexcept
{
	if (!ready(s, sink) || !telemetry_battle_id_is_valid(battle) ||
	    !telemetry_battle_actor_key_is_valid(actor) || !valid_cut(point) ||
	    !valid_end(reason) || !ordered_time(*s, point.decision_usec, point.decision_utc_usec))
		return result(outcome::invalid);
	const auto found = find_actor(*s, actor.id);
	if (!found.actor || !same_battle(found.owner->battle, battle) ||
	    found.actor->value.context.actor.actor.kind != actor.kind)
		return result(outcome::not_found);
	if (point.observed_usec < found.actor->last_observed_usec)
		return result(outcome::invalid);
	remember_time(*s, point.decision_usec, point.decision_utc_usec);
	auto r = result();
	seal(*s, found, point, reason, sink, opaque, r);
	return r;
}
telemetry_battle_contribution_update
telemetry_battle_contribution_close(state *s, telemetry_battle_id battle, cut point, end reason,
				    telemetry_battle_contribution_sink sink, void *opaque) noexcept
{
	if (!ready(s, sink) || !telemetry_battle_id_is_valid(battle) || !valid_cut(point) ||
	    !valid_end(reason) || !ordered_time(*s, point.decision_usec, point.decision_utc_usec))
		return result(outcome::invalid);
	auto *owner = find_battle(*s, battle);
	if (!owner)
		return result(outcome::not_found);
	for (const auto &a : owner->actors)
		if (a.occupied && point.observed_usec < a.last_observed_usec)
			return result(outcome::invalid);
	remember_time(*s, point.decision_usec, point.decision_utc_usec);
	auto r = result();
	for (auto &a : owner->actors)
		if (a.occupied)
			seal(*s, { owner, &a }, point, reason, sink, opaque, r);
	return r;
}
