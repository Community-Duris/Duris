#include "telemetry/telemetry_battle_contract.h"

#include <bit>
#include <limits>
#include <string_view>
#include <type_traits>

namespace
{
bool same_id(telemetry_battle_id a, telemetry_battle_id b) noexcept
{
	return a.producer.boot_id == b.producer.boot_id &&
	       a.producer.process_id == b.producer.process_id && a.sequence == b.sequence;
}

bool same_actor(const telemetry_battle_actor_context &a,
		const telemetry_battle_actor_context &b) noexcept
{
	/* Kind can change between pet and NPC while the live generation remains. */
	return a.actor.actor_id == b.actor.actor_id;
}

constexpr bool actor_field(std::string_view name) noexcept
{
	return name.starts_with("battle_actor_") && name != "battle_actor_side";
}

bool same_actor_values(const telemetry_battle_fact &a, const telemetry_battle_fact &b) noexcept
{
#define TELEMETRY_BATTLE_FIELD(name, member, width, signed_value) \
	if constexpr (actor_field(#name))                         \
		if (a.member != b.member)                         \
			return false;
#include "telemetry/telemetry_battle_fields.inc"
#undef TELEMETRY_BATTLE_FIELD
	return a.effort.present_usec == b.effort.present_usec &&
	       a.effort.contributor_usec == b.effort.contributor_usec &&
	       a.effort.pve_usec == b.effort.pve_usec && a.effort.pvp_usec == b.effort.pvp_usec &&
	       a.effort.mixed_usec == b.effort.mixed_usec &&
	       a.effort.unknown_mode_usec == b.effort.unknown_mode_usec &&
	       a.effort.outnumbered_owner_usec == b.effort.outnumbered_owner_usec &&
	       a.effort.unknown_side_usec == b.effort.unknown_side_usec &&
	       (a.side_status != b.side_status || a.side == b.side);
}

bool same_packet_header(const telemetry_battle_fact &a, const telemetry_battle_fact &b) noexcept
{
	return same_id(a.battle, b.battle) && a.revision == b.revision &&
	       a.fact_count == b.fact_count && a.definition_version == b.definition_version &&
	       a.scope.environment_id == b.scope.environment_id &&
	       a.scope.season_id == b.scope.season_id && a.scope.config_id == b.scope.config_id &&
	       a.scope.classifier_version == b.scope.classifier_version &&
	       a.scope.policy_version == b.scope.policy_version &&
	       a.scope.zone_vnum == b.scope.zone_vnum && a.scope.group_key == b.scope.group_key &&
	       a.mode == b.mode && a.close_reason == b.close_reason &&
	       a.end_censored == b.end_censored && a.actor_count == b.actor_count &&
	       a.active_actor_count == b.active_actor_count &&
	       a.observed_owner_count == b.observed_owner_count &&
	       a.dropped_actor_count == b.dropped_actor_count &&
	       a.start_monotonic_usec == b.start_monotonic_usec &&
	       a.at_monotonic_usec == b.at_monotonic_usec &&
	       a.observed_through_monotonic_usec == b.observed_through_monotonic_usec &&
	       a.last_engagement_monotonic_usec == b.last_engagement_monotonic_usec &&
	       a.inactivity_grace_usec == b.inactivity_grace_usec &&
	       a.at_utc_usec == b.at_utc_usec &&
	       a.observed_through_utc_usec == b.observed_through_utc_usec;
}

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
	if constexpr (std::is_signed_v<T>)
		value = std::bit_cast<T>(static_cast<std::make_unsigned_t<T>>(number));
	else
		value = static_cast<T>(number);
}
} // namespace

bool telemetry_battle_fact_same_key(const telemetry_battle_fact &a,
				    const telemetry_battle_fact &b) noexcept
{
	return same_id(a.battle, b.battle) && a.fact_sequence == b.fact_sequence;
}

bool telemetry_battle_fact_equal(const telemetry_battle_fact &a,
				 const telemetry_battle_fact &b) noexcept
{
	if (!telemetry_battle_fact_is_valid(a) || !telemetry_battle_fact_is_valid(b))
		return false;
#define TELEMETRY_BATTLE_FIELD(name, member, width, signed_value) \
	if (a.member != b.member)                                 \
		return false;
#include "telemetry/telemetry_battle_fields.inc"
#undef TELEMETRY_BATTLE_FIELD
	return true;
}

bool telemetry_battle_fact_encode(const telemetry_battle_fact &fact, std::uint8_t *bytes,
				  std::size_t length) noexcept
{
	if (!bytes || length != TELEMETRY_BATTLE_WIRE_BYTES ||
	    !telemetry_battle_fact_is_valid(fact))
		return false;
#define TELEMETRY_BATTLE_FIELD(name, member, width, signed_value) \
	encode_value<width, signed_value>(fact.member, bytes);
#include "telemetry/telemetry_battle_fields.inc"
#undef TELEMETRY_BATTLE_FIELD
	return true;
}

bool telemetry_battle_fact_decode(const std::uint8_t *bytes, std::size_t length,
				  telemetry_battle_fact *fact) noexcept
{
	if (!fact)
		return false;
	*fact = {};
	if (!bytes || length != TELEMETRY_BATTLE_WIRE_BYTES)
		return false;
	telemetry_battle_fact decoded{};
#define TELEMETRY_BATTLE_FIELD(name, member, width, signed_value) \
	decode_value<width, signed_value>(decoded.member, bytes);
#include "telemetry/telemetry_battle_fields.inc"
#undef TELEMETRY_BATTLE_FIELD
	if (!telemetry_battle_fact_is_valid(decoded))
		return false;
	*fact = decoded;
	return true;
}

bool telemetry_battle_packet_is_valid(const telemetry_battle_fact *facts,
				      std::size_t count) noexcept
{
	if (!facts || count == 0U || count > TELEMETRY_BATTLE_PACKET_MAX_FACTS ||
	    facts[0].fact_count != count ||
	    facts[0].fact_sequence > std::numeric_limits<std::uint32_t>::max() - (count - 1U))
		return false;
	telemetry_quality_mask previous_quality = 0U;
	for (std::size_t index = 0U; index < count; ++index)
	{
		const auto &fact = facts[index];
		if (!telemetry_battle_fact_is_valid(fact) || !same_packet_header(facts[0], fact) ||
		    fact.fact_index != index ||
		    fact.fact_sequence != facts[0].fact_sequence + index ||
		    (previous_quality & ~fact.quality_flags) != 0U)
			return false;
		/* A failed sink can add loss/uncertainty after an earlier frame. The final
		 * cut remains authoritative; requiring uniform side status hides loss. */
		previous_quality = fact.quality_flags;
	}
	const auto &last = facts[count - 1U];
	for (std::size_t index = 0U; index + 1U < count; ++index)
		if (facts[index].kind == telemetry_battle_fact_kind::relation)
			for (std::size_t actor = 0U; actor < index; ++actor)
				if (facts[actor].kind ==
					    telemetry_battle_fact_kind::actor_context &&
				    ((same_actor(facts[index].actor, facts[actor].actor) &&
				      !same_actor_values(facts[index], facts[actor])) ||
				     (facts[index].related_actor.id ==
					      facts[actor].actor.actor.actor_id &&
				      facts[index].related_actor.kind !=
					      facts[actor].actor.actor.kind)))
					return false;
	if (last.kind == telemetry_battle_fact_kind::close)
	{
		std::uint16_t active = 0U, owner_count = 0U;
		telemetry_subject_id owners[TELEMETRY_BATTLE_MAX_ACTORS]{};
		for (std::size_t index = 0U; index + 1U < count; ++index)
		{
			const auto &fact = facts[index];
			if (fact.kind != telemetry_battle_fact_kind::actor_summary)
				return false;
			for (std::size_t prior = 0U; prior < index; ++prior)
				if (same_actor(fact.actor, facts[prior].actor))
					return false;
			active += fact.active;
			if (fact.active && fact.actor.actor.owner_subject_id != 0U)
			{
				bool seen = false;
				for (std::uint16_t prior = 0U; prior < owner_count; ++prior)
					seen |= owners[prior] == fact.actor.actor.owner_subject_id;
				if (!seen)
					owners[owner_count++] = fact.actor.actor.owner_subject_id;
			}
		}
		return active == last.active_actor_count &&
		       owner_count == last.observed_owner_count;
	}
	if (last.kind != telemetry_battle_fact_kind::cut)
		return false;
	if (facts[0].kind == telemetry_battle_fact_kind::start)
	{
		if (count != 5U || facts[0].fact_sequence != 1U || facts[0].actor_count != 2U ||
		    facts[0].active_actor_count != 2U || facts[0].observed_owner_count == 0U ||
		    facts[0].at_monotonic_usec != facts[0].start_monotonic_usec ||
		    facts[1].kind != telemetry_battle_fact_kind::actor_context ||
		    facts[2].kind != telemetry_battle_fact_kind::actor_context ||
		    facts[3].kind != telemetry_battle_fact_kind::relation ||
		    facts[3].relation != telemetry_battle_relation::hostile ||
		    same_actor(facts[1].actor, facts[2].actor) || !facts[1].active ||
		    !facts[2].active || facts[1].roles != TELEMETRY_BATTLE_ROLE_COMBAT ||
		    facts[2].roles != TELEMETRY_BATTLE_ROLE_COMBAT ||
		    facts[1].effort.present_usec != 0U || facts[2].effort.present_usec != 0U ||
		    facts[3].actor.actor.actor_id != facts[1].actor.actor.actor_id ||
		    facts[3].related_actor.id != facts[2].actor.actor.actor_id ||
		    facts[3].related_actor.kind != facts[2].actor.actor.kind)
			return false;
		const auto first_owner = facts[1].actor.actor.owner_subject_id;
		const auto second_owner = facts[2].actor.actor.owner_subject_id;
		const auto owners =
			(first_owner != 0U) + (second_owner != 0U && second_owner != first_owner);
		return owners == facts[0].observed_owner_count &&
		       facts[0].mode == (first_owner != 0U && second_owner != 0U ?
						 telemetry_encounter_mode::pvp :
						 telemetry_encounter_mode::pve);
	}
	if (facts[0].revision == 1U)
		return false;
	if (count == 1U)
		return (last.quality_flags & TELEMETRY_QUALITY_CONTEXT_OVERFLOW) != 0U;
	const bool alias = facts[0].kind == telemetry_battle_fact_kind::merge_alias;
	std::size_t contexts = 0U, relations = 0U;
	for (std::size_t index = alias ? 1U : 0U; index + 1U < count; ++index)
	{
		const auto &fact = facts[index];
		if (fact.kind == telemetry_battle_fact_kind::actor_context)
		{
			if (relations != 0U || ++contexts > 2U)
				return false;
			for (std::size_t prior = alias ? 1U : 0U; prior < index; ++prior)
				if (same_actor(fact.actor, facts[prior].actor))
					return false;
		}
		else if (fact.kind == telemetry_battle_fact_kind::relation)
		{
			if (++relations > 1U || index + 2U != count ||
			    (alias && fact.actor.actor.actor_id == fact.related_actor.id))
				return false;
		}
		else
			return false;
	}
	return alias ? relations == 1U : contexts != 0U || relations != 0U;
}

void telemetry_battle_packet_reset(telemetry_battle_packet_state *state) noexcept
{
	if (state)
		*state = {};
}

telemetry_battle_packet_result
telemetry_battle_packet_receive(telemetry_battle_packet_state *state,
				const telemetry_battle_fact &fact) noexcept
{
	if (!state || !telemetry_battle_fact_is_valid(fact) ||
	    state->count > TELEMETRY_BATTLE_PACKET_MAX_FACTS)
		return telemetry_battle_packet_result::invalid;
	if (state->conflicted)
		return telemetry_battle_packet_result::duplicate_conflict;
	const auto first_sequence = fact.fact_sequence - fact.fact_index;
	if (state->count == 0U)
	{
		state->battle = fact.battle;
		state->revision = fact.revision;
		state->first_sequence = first_sequence;
		state->count = fact.fact_count;
	}
	else if (!same_id(state->battle, fact.battle) || state->revision != fact.revision)
		return telemetry_battle_packet_result::other_packet;
	if (state->first_sequence != first_sequence || state->count != fact.fact_count)
	{
		state->conflicted = 1U;
		state->complete = 0U;
		return telemetry_battle_packet_result::duplicate_conflict;
	}
	const auto word = fact.fact_index / 64U;
	const auto bit = std::uint64_t{ 1U } << (fact.fact_index % 64U);
	if ((state->seen[word] & bit) != 0U)
	{
		if (telemetry_battle_fact_equal(state->facts[fact.fact_index], fact))
			return telemetry_battle_packet_result::duplicate_identical;
		state->conflicted = 1U;
		state->complete = 0U;
		return telemetry_battle_packet_result::duplicate_conflict;
	}
	state->facts[fact.fact_index] = fact;
	state->seen[word] |= bit;
	if (++state->received < state->count)
		return telemetry_battle_packet_result::pending;
	if (!telemetry_battle_packet_is_valid(state->facts, state->count))
	{
		state->conflicted = 1U;
		return telemetry_battle_packet_result::invalid;
	}
	state->complete = 1U;
	return telemetry_battle_packet_result::complete;
}
