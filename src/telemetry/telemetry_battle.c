#include "telemetry/telemetry_battle.h"

#include <algorithm>
#include <limits>

namespace
{
constexpr std::uint16_t CONTRIBUTOR_ROLES = TELEMETRY_BATTLE_ROLE_COMBAT |
					    TELEMETRY_BATTLE_ROLE_SUPPORT;
constexpr telemetry_quality_mask PARTIAL_GRAPH =
	TELEMETRY_QUALITY_QUEUE_DROP | TELEMETRY_QUALITY_CARDINALITY_OVERFLOW |
	TELEMETRY_QUALITY_CONTEXT_OVERFLOW | TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;

bool same_producer(telemetry_producer_id a, telemetry_producer_id b) noexcept
{
	return a.boot_id == b.boot_id && a.process_id == b.process_id;
}

bool same_id(telemetry_battle_id a, telemetry_battle_id b) noexcept
{
	return same_producer(a.producer, b.producer) && a.sequence == b.sequence;
}

telemetry_battle_actor_key key(const telemetry_battle_actor_context &context) noexcept
{
	return { context.actor.actor_id, context.actor.kind };
}

bool same_key(telemetry_battle_actor_key a, telemetry_battle_actor_key b) noexcept
{
	/* The high-bit live generation survives a pet gaining/losing its owner.
	 * PCs occupy the disjoint positive signed PID namespace. */
	return a.id == b.id &&
	       (a.kind == b.kind || (a.kind != telemetry_combat_actor_kind::player &&
				     b.kind != telemetry_combat_actor_kind::player));
}

bool same_context(const telemetry_battle_actor_context &a,
		  const telemetry_battle_actor_context &b) noexcept
{
	return same_key(key(a), key(b)) && a.actor.kind == b.actor.kind &&
	       a.actor.actor_pid == b.actor.actor_pid &&
	       a.actor.owner_subject_id == b.actor.owner_subject_id &&
	       a.actor.power_band == b.actor.power_band &&
	       same_producer(a.encounter.producer, b.encounter.producer) &&
	       a.encounter.sequence == b.encounter.sequence &&
	       same_producer(a.session.producer, b.session.producer) &&
	       a.session.session_seq == b.session.session_seq &&
	       a.dimensions.level_band == b.dimensions.level_band &&
	       a.dimensions.class_id == b.dimensions.class_id &&
	       a.dimensions.race_id == b.dimensions.race_id &&
	       a.dimensions.faction_id == b.dimensions.faction_id &&
	       a.dimensions.zone_vnum == b.dimensions.zone_vnum &&
	       a.dimensions.group_size == b.dimensions.group_size && a.group_key == b.group_key &&
	       a.group_revision == b.group_revision && a.context_version == b.context_version &&
	       a.quality_flags == b.quality_flags;
}

int actor_index(const telemetry_battle_slot &slot, telemetry_battle_actor_key actor) noexcept
{
	for (std::uint16_t index = 0U; index < slot.actor_count; ++index)
		if (same_key(key(slot.actors[index].context), actor))
			return index;
	return -1;
}

int slot_for_actor(const telemetry_battle_state &state, telemetry_battle_actor_key actor) noexcept
{
	for (std::size_t index = 0U; index < TELEMETRY_BATTLE_MAX_ACTIVE; ++index)
		if (state.slots[index].occupied && actor_index(state.slots[index], actor) >= 0)
			return static_cast<int>(index);
	return -1;
}

bool formal_group(const telemetry_battle_actor_context &actor) noexcept
{
	return (actor.group_key & TELEMETRY_GROUP_GENERATION_TAG) != 0U &&
	       (actor.group_key & ~TELEMETRY_GROUP_GENERATION_TAG) != 0U &&
	       actor.group_revision != 0U;
}

bool same_group(const telemetry_battle_actor_context &a,
		const telemetry_battle_actor_context &b) noexcept
{
	return formal_group(a) && a.group_key == b.group_key &&
	       a.group_revision == b.group_revision;
}

bool context_scope_matches(const telemetry_battle_actor_context &actor,
			   telemetry_producer_id producer) noexcept
{
	return actor.encounter.sequence == 0U || same_producer(actor.encounter.producer, producer);
}

std::uint64_t bit(std::uint16_t index) noexcept
{
	return std::uint64_t{ 1U } << index;
}

void classify(telemetry_battle_slot &slot) noexcept
{
	bool contradictory = false;
	bool stale_roster = false;
	unsigned components = 0U;
	for (std::uint16_t index = 0U; index < slot.actor_count; ++index)
		slot.actors[index].side = 0U;
	for (std::uint16_t component = 0U; component < slot.actor_count; ++component)
	{
		int root = -1;
		for (std::uint16_t index = 0U; index < slot.actor_count; ++index)
		{
			const auto &actor = slot.actors[index];
			if (!actor.active || actor.side != 0U)
				continue;
			if (root < 0 ||
			    actor.context.actor.actor_id <
				    slot.actors[static_cast<std::size_t>(root)]
					    .context.actor.actor_id ||
			    (actor.context.actor.actor_id ==
				     slot.actors[static_cast<std::size_t>(root)]
					     .context.actor.actor_id &&
			     actor.context.actor.kind <
				     slot.actors[static_cast<std::size_t>(root)].context.actor.kind))
				root = index;
		}
		if (root < 0)
			break;
		++components;
		std::uint16_t pending[TELEMETRY_BATTLE_MAX_ACTORS]{};
		std::uint16_t first = 0U, count = 1U;
		pending[0] = static_cast<std::uint16_t>(root);
		slot.actors[static_cast<std::size_t>(root)].side = 1U;
		while (first < count)
		{
			const auto index = pending[first++];
			const auto &actor = slot.actors[index];
			for (std::uint16_t other = 0U; other < slot.actor_count; ++other)
			{
				auto &target = slot.actors[other];
				if (!target.active || index == other)
					continue;
				stale_roster |= formal_group(actor.context) &&
						formal_group(target.context) &&
						actor.context.group_key ==
							target.context.group_key &&
						actor.context.group_revision !=
							target.context.group_revision;
				const bool hostile = (actor.hostile & bit(other)) != 0U;
				const bool friendly =
					(actor.friendly & bit(other)) != 0U ||
					same_group(actor.context, target.context) ||
					(actor.context.actor.owner_subject_id != 0U &&
					 actor.context.actor.owner_subject_id ==
						 target.context.actor.owner_subject_id);
				if (!hostile && !friendly)
					continue;
				if (hostile && friendly)
					contradictory = true;
				const std::uint8_t expected = hostile ? 3U - actor.side :
									actor.side;
				if (target.side == 0U)
				{
					target.side = expected;
					pending[count++] = other;
				}
				else if (target.side != expected)
					contradictory = true;
			}
		}
	}
	slot.side_status = contradictory ? telemetry_battle_side_status::ambiguous :
			   stale_roster || components > 1U ||
					   (slot.quality_flags & PARTIAL_GRAPH) != 0U ?
					   telemetry_battle_side_status::partial :
					   telemetry_battle_side_status::qualified_observed_graph;
	if (slot.side_status != telemetry_battle_side_status::qualified_observed_graph)
		for (std::uint16_t index = 0U; index < slot.actor_count; ++index)
			slot.actors[index].side = 0U;
	bool pvp = false, pve = false;
	for (std::uint16_t index = 0U; index < slot.actor_count; ++index)
		for (std::uint16_t other = index + 1U; other < slot.actor_count; ++other)
			if (slot.actors[index].active && slot.actors[other].active &&
			    (slot.actors[index].hostile & bit(other)) != 0U)
			{
				const bool owned_pair =
					slot.actors[index].context.actor.owner_subject_id != 0U &&
					slot.actors[other].context.actor.owner_subject_id != 0U;
				pvp |= owned_pair;
				pve |= !owned_pair;
			}
	slot.mode = pvp && pve ? telemetry_encounter_mode::mixed :
		    pvp	       ? telemetry_encounter_mode::pvp :
		    pve	       ? telemetry_encounter_mode::pve :
				 telemetry_encounter_mode::unknown;
}

std::uint16_t owner_count(const telemetry_battle_slot &slot, std::uint8_t side = 0U) noexcept
{
	telemetry_subject_id owners[TELEMETRY_BATTLE_MAX_ACTORS]{};
	std::uint16_t count = 0U;
	for (std::uint16_t index = 0U; index < slot.actor_count; ++index)
	{
		const auto &actor = slot.actors[index];
		const auto owner = actor.context.actor.owner_subject_id;
		if (!actor.active || owner == 0U || (side != 0U && actor.side != side))
			continue;
		if (std::find(owners, owners + count, owner) == owners + count)
			owners[count++] = owner;
	}
	return count;
}

void add_duration(telemetry_duration_usec &total, telemetry_duration_usec amount,
		  telemetry_battle_slot &slot) noexcept
{
	if (amount > std::numeric_limits<telemetry_duration_usec>::max() - total)
	{
		total = std::numeric_limits<telemetry_duration_usec>::max();
		slot.quality_flags |= TELEMETRY_QUALITY_CONTEXT_OVERFLOW;
	}
	else
		total += amount;
}

void seal(telemetry_battle_slot &slot, telemetry_monotonic_usec at) noexcept
{
	const auto elapsed = at - slot.cut_usec;
	const auto left = owner_count(slot, 1U), right = owner_count(slot, 2U);
	for (std::uint16_t index = 0U; index < slot.actor_count; ++index)
	{
		auto &actor = slot.actors[index];
		if (!actor.active)
			continue;
		add_duration(actor.effort.present_usec, elapsed, slot);
		if ((actor.roles & CONTRIBUTOR_ROLES) != 0U)
			add_duration(actor.effort.contributor_usec, elapsed, slot);
		auto *mode = slot.mode == telemetry_encounter_mode::pvp ?
				     &actor.effort.pvp_usec :
			     slot.mode == telemetry_encounter_mode::pve ?
				     &actor.effort.pve_usec :
			     slot.mode == telemetry_encounter_mode::mixed ?
				     &actor.effort.mixed_usec :
				     &actor.effort.unknown_mode_usec;
		add_duration(*mode, elapsed, slot);
		if (slot.side_status != telemetry_battle_side_status::qualified_observed_graph)
			add_duration(actor.effort.unknown_side_usec, elapsed, slot);
		else if (slot.mode == telemetry_encounter_mode::pvp &&
			 actor.context.actor.owner_subject_id != 0U &&
			 ((actor.side == 1U && left < right) || (actor.side == 2U && right < left)))
			add_duration(actor.effort.outnumbered_owner_usec, elapsed, slot);
	}
	slot.cut_usec = at;
}

struct pending_fact
{
	telemetry_battle_fact_kind kind;
	int actor = -1;
	telemetry_battle_actor_key related_actor{};
	telemetry_battle_relation relation{};
	telemetry_battle_id related_battle{};
};

struct packet
{
	pending_fact facts[TELEMETRY_BATTLE_MAX_FACTS_PER_MUTATION]{};
	std::uint16_t count = 0U;
	bool overflow = false;
	void append(pending_fact value) noexcept
	{
		if (count < TELEMETRY_BATTLE_MAX_FACTS_PER_MUTATION)
			facts[count++] = value;
		else
			overflow = true;
	}
};

telemetry_battle_fact fact(const telemetry_battle_state &state, const telemetry_battle_slot &slot,
			   pending_fact pending, telemetry_monotonic_usec at,
			   telemetry_utc_usec utc) noexcept
{
	telemetry_battle_fact value{};
	value.battle = slot.id;
	value.related_battle = pending.related_battle;
	value.scope = state.scope;
	value.revision = slot.revision;
	value.definition_version = TELEMETRY_BATTLE_DEFINITION_VERSION;
	value.kind = pending.kind;
	value.relation = pending.relation;
	value.side_status = slot.side_status;
	value.mode = slot.mode;
	value.actor_count = slot.actor_count;
	value.observed_owner_count = owner_count(slot);
	value.dropped_actor_count = slot.dropped_actor_count;
	value.related_actor = pending.related_actor;
	value.start_monotonic_usec = slot.start_usec;
	value.at_monotonic_usec = at;
	value.at_utc_usec = utc;
	value.observed_through_monotonic_usec = slot.last_observation_usec;
	value.last_engagement_monotonic_usec = slot.last_engagement_usec;
	value.inactivity_grace_usec = state.inactivity_grace_usec;
	value.observed_through_utc_usec = slot.last_observation_utc_usec;
	value.quality_flags = slot.quality_flags;
	for (std::uint16_t index = 0U; index < slot.actor_count; ++index)
		value.active_actor_count += slot.actors[index].active != 0U;
	if (pending.actor >= 0)
	{
		const auto &actor = slot.actors[static_cast<std::size_t>(pending.actor)];
		value.actor = actor.context;
		value.effort = actor.effort;
		value.roles = actor.roles;
		value.active = actor.active;
		value.side = actor.side;
	}
	return value;
}

void send_fact(telemetry_battle_slot &slot, telemetry_battle_fact value, std::uint16_t index,
	       std::uint16_t count, telemetry_battle_sink sink, void *context,
	       telemetry_battle_update &result) noexcept
{
	value.fact_sequence = ++slot.fact_sequence;
	value.fact_index = index;
	value.fact_count = count;
	++result.facts_attempted;
	if (sink(context, value))
		++result.facts_accepted;
	else
	{
		slot.quality_flags |= TELEMETRY_QUALITY_QUEUE_DROP;
		classify(slot);
		result.outcome = telemetry_battle_outcome::sink_rejected;
	}
}

telemetry_battle_update emit_packet(telemetry_battle_state &state, telemetry_battle_slot &slot,
				    const packet &pending, telemetry_monotonic_usec at,
				    telemetry_utc_usec utc, telemetry_battle_sink sink,
				    void *context) noexcept
{
	telemetry_battle_update result{};
	result.battle = slot.id;
	if (pending.count == 0U)
	{
		result.outcome = telemetry_battle_outcome::idempotent;
		result.quality_flags = slot.quality_flags;
		return result;
	}
	if (pending.overflow ||
	    slot.mutation_facts + pending.count > TELEMETRY_BATTLE_MAX_MUTATION_FACTS)
	{
		slot.quality_flags |= TELEMETRY_QUALITY_CONTEXT_OVERFLOW;
		classify(slot);
		result.outcome = telemetry_battle_outcome::capacity_full;
		if (!slot.overflow_announced)
		{
			slot.overflow_announced = 1U;
			++slot.revision;
			send_fact(slot,
				  fact(state, slot, { telemetry_battle_fact_kind::cut }, at, utc),
				  0U, 1U, sink, context, result);
		}
		result.quality_flags = slot.quality_flags;
		return result;
	}
	++slot.revision;
	slot.mutation_facts += pending.count;
	for (std::uint16_t index = 0U; index < pending.count; ++index)
		send_fact(slot, fact(state, slot, pending.facts[index], at, utc), index,
			  pending.count, sink, context, result);
	result.quality_flags = slot.quality_flags;
	return result;
}

bool ready(const telemetry_battle_state *state, telemetry_battle_sink sink) noexcept
{
	return state != nullptr && state->initialized != 0U && sink != nullptr;
}

bool admit_time(telemetry_battle_state &state, telemetry_monotonic_usec at) noexcept
{
	if (at < state.latest_monotonic_usec)
	{
		for (auto &slot : state.slots)
			if (slot.occupied)
			{
				slot.quality_flags |= TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
				classify(slot);
			}
		return false;
	}
	state.latest_monotonic_usec = at;
	return true;
}

bool ordered(telemetry_battle_slot &slot, telemetry_monotonic_usec at) noexcept
{
	if (at < slot.cut_usec || at < slot.last_observation_usec)
	{
		slot.quality_flags |= TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
		classify(slot);
		return false;
	}
	return slot.revision != std::numeric_limits<telemetry_revision>::max();
}

bool expired(const telemetry_battle_state &state, const telemetry_battle_slot &slot,
	     telemetry_monotonic_usec at) noexcept
{
	return at >= slot.last_engagement_usec &&
	       at - slot.last_engagement_usec >= state.inactivity_grace_usec;
}

int update_actor(telemetry_battle_slot &slot, const telemetry_battle_actor_context &actor,
		 std::uint16_t roles, packet &pending) noexcept
{
	int index = actor_index(slot, key(actor));
	if (index < 0)
	{
		index = slot.actor_count++;
		slot.actors[static_cast<std::size_t>(index)] = {};
	}
	auto &entry = slot.actors[static_cast<std::size_t>(index)];
	const auto new_roles = static_cast<std::uint16_t>(entry.roles | roles);
	const bool changed = !entry.active || new_roles != entry.roles ||
			     !same_context(entry.context, actor);
	entry.context = actor;
	entry.roles = new_roles;
	entry.active = 1U;
	slot.quality_flags |= actor.quality_flags;
	if (changed)
		pending.append({ telemetry_battle_fact_kind::actor_context, index });
	return index;
}

void remap_actor(telemetry_battle_actor_state &actor, std::uint16_t offset,
		 std::uint16_t count) noexcept
{
	const auto hostile = actor.hostile, friendly = actor.friendly,
		   presence = actor.group_presence;
	actor.hostile = actor.friendly = actor.group_presence = 0U;
	for (std::uint16_t index = 0U; index < count; ++index)
	{
		if ((hostile & bit(index)) != 0U)
			actor.hostile |= bit(index + offset);
		if ((friendly & bit(index)) != 0U)
			actor.friendly |= bit(index + offset);
		if ((presence & bit(index)) != 0U)
			actor.group_presence |= bit(index + offset);
	}
}

telemetry_battle_update refused(telemetry_battle_outcome outcome) noexcept
{
	telemetry_battle_update result{};
	result.outcome = outcome;
	return result;
}

} // namespace

bool telemetry_battle_actor_context_is_valid(const telemetry_battle_actor_context &context) noexcept
{
	if (!telemetry_combat_actor_ref_is_valid(context.actor) ||
	    !telemetry_quality_mask_is_valid(context.quality_flags) ||
	    context.dimensions.zone_vnum < -1 ||
	    context.context_version != TELEMETRY_BATTLE_ACTOR_CONTEXT_VERSION)
		return false;
	if (context.actor.kind == telemetry_combat_actor_kind::player)
	{
		if (context.actor.actor_id != static_cast<telemetry_id>(context.actor.actor_pid))
			return false;
	}
	else if ((context.actor.actor_id & TELEMETRY_BATTLE_NPC_GENERATION_TAG) == 0U ||
		 (context.actor.actor_id & ~TELEMETRY_BATTLE_NPC_GENERATION_TAG) == 0U)
		return false;
	if ((context.group_key & TELEMETRY_GROUP_GENERATION_TAG) != 0U && !formal_group(context))
		return false;
	if ((context.group_key & TELEMETRY_GROUP_GENERATION_TAG) == 0U &&
	    context.group_revision != 0U)
		return false;
	const bool empty_encounter = context.encounter.producer.boot_id == 0U &&
				     context.encounter.producer.process_id == 0U &&
				     context.encounter.sequence == 0U;
	const bool empty_session = context.session.producer.boot_id == 0U &&
				   context.session.producer.process_id == 0U &&
				   context.session.session_seq == 0U;
	return (empty_encounter || telemetry_encounter_id_is_valid(context.encounter)) &&
	       (empty_session || (context.actor.kind == telemetry_combat_actor_kind::player &&
				  telemetry_session_id_is_valid(context.session)));
}

bool telemetry_battle_state_init(telemetry_battle_state *state, telemetry_producer_id producer,
				 telemetry_encounter_source scope,
				 telemetry_duration_usec inactivity_grace_usec) noexcept
{
	if (state == nullptr || !telemetry_producer_id_is_valid(producer) ||
	    !telemetry_encounter_source_is_valid(scope) || scope.group_key != 0U ||
	    scope.zone_vnum != -1 || inactivity_grace_usec == 0U)
		return false;
	*state = {};
	state->producer = producer;
	state->scope = scope;
	state->inactivity_grace_usec = inactivity_grace_usec;
	state->next_sequence = 1U;
	state->initialized = 1U;
	return true;
}

telemetry_battle_update telemetry_battle_close(telemetry_battle_state *state,
					       telemetry_battle_id id,
					       telemetry_battle_close_reason reason,
					       telemetry_monotonic_usec at, telemetry_utc_usec utc,
					       telemetry_battle_sink sink, void *context) noexcept
{
	if (!ready(state, sink) || !same_producer(id.producer, state->producer) ||
	    id.sequence == 0U || reason < telemetry_battle_close_reason::inactivity ||
	    reason > telemetry_battle_close_reason::shutdown)
		return refused(telemetry_battle_outcome::invalid);
	for (auto &slot : state->slots)
	{
		if (!slot.occupied || !same_id(slot.id, id))
			continue;
		if (!admit_time(*state, at) || !ordered(slot, at))
			return refused(telemetry_battle_outcome::invalid);
		if (reason == telemetry_battle_close_reason::inactivity &&
		    !expired(*state, slot, at))
			return refused(telemetry_battle_outcome::invalid);
		const auto end = reason == telemetry_battle_close_reason::inactivity ?
					 slot.last_observation_usec :
					 at;
		seal(slot, end);
		slot.quality_flags |= TELEMETRY_QUALITY_UNCLOSED_TAIL;
		classify(slot);
		++slot.revision;
		telemetry_battle_update result{};
		result.battle = slot.id;
		const auto count = static_cast<std::uint16_t>(slot.actor_count + 1U);
		for (std::uint16_t index = 0U; index < count; ++index)
		{
			const bool closing = index == slot.actor_count;
			auto value = fact(*state, slot,
					  { closing ? telemetry_battle_fact_kind::close :
						      telemetry_battle_fact_kind::actor_summary,
					    closing ? -1 : index },
					  at, utc);
			value.close_reason = reason;
			value.end_censored = 1U;
			value.observed_through_monotonic_usec = end;
			value.observed_through_utc_usec =
				reason == telemetry_battle_close_reason::inactivity ?
					slot.last_observation_utc_usec :
					utc;
			send_fact(slot, value, index, count, sink, context, result);
		}
		result.quality_flags = slot.quality_flags;
		state->terminal[state->terminal_next] = { id, reason };
		state->terminal_next =
			(state->terminal_next + 1U) % TELEMETRY_BATTLE_TERMINAL_CACHE;
		slot = {};
		return result;
	}
	for (const auto &terminal : state->terminal)
		if (same_id(terminal.id, id))
			return refused(terminal.reason == reason ?
					       telemetry_battle_outcome::idempotent :
					       telemetry_battle_outcome::duplicate_conflict);
	return refused(telemetry_battle_outcome::not_found);
}

telemetry_battle_update
telemetry_battle_observe(telemetry_battle_state *state, telemetry_battle_relation relation,
			 const telemetry_battle_actor_context &source,
			 const telemetry_battle_actor_context &target, telemetry_monotonic_usec at,
			 telemetry_utc_usec utc, telemetry_battle_sink sink, void *context) noexcept
{
	if (!ready(state, sink) || !telemetry_battle_actor_context_is_valid(source) ||
	    !telemetry_battle_actor_context_is_valid(target) ||
	    !context_scope_matches(source, state->producer) ||
	    !context_scope_matches(target, state->producer) ||
	    relation < telemetry_battle_relation::hostile ||
	    relation > telemetry_battle_relation::group_presence ||
	    (same_key(key(source), key(target)) &&
	     relation != telemetry_battle_relation::support) ||
	    (same_key(key(source), key(target)) && !same_context(source, target)))
		return refused(telemetry_battle_outcome::invalid);
	if (relation == telemetry_battle_relation::group_presence &&
	    (!same_group(source, target) || source.dimensions.zone_vnum < 0 ||
	     source.dimensions.zone_vnum != target.dimensions.zone_vnum))
		return refused(telemetry_battle_outcome::invalid);
	if (!admit_time(*state, at))
	{
		auto result = refused(telemetry_battle_outcome::invalid);
		result.quality_flags = TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
		return result;
	}
	telemetry_battle_update prior{};
	auto finish = [&prior](telemetry_battle_update result) noexcept
	{
		result.facts_attempted += prior.facts_attempted;
		result.facts_accepted += prior.facts_accepted;
		result.quality_flags |= prior.quality_flags;
		if (prior.outcome == telemetry_battle_outcome::sink_rejected)
			result.outcome = prior.outcome;
		return result;
	};
	int first = slot_for_actor(*state, key(source)),
	    second = slot_for_actor(*state, key(target));
	for (int candidate : { first, second })
		if (candidate >= 0 && state->slots[static_cast<std::size_t>(candidate)].occupied &&
		    expired(*state, state->slots[static_cast<std::size_t>(candidate)], at))
		{
			const auto closed = telemetry_battle_close(
				state, state->slots[static_cast<std::size_t>(candidate)].id,
				telemetry_battle_close_reason::inactivity, at, utc, sink, context);
			prior.facts_attempted += closed.facts_attempted;
			prior.facts_accepted += closed.facts_accepted;
			prior.quality_flags |= closed.quality_flags;
			if (closed.outcome == telemetry_battle_outcome::invalid)
				return finish(refused(telemetry_battle_outcome::invalid));
			if (closed.outcome == telemetry_battle_outcome::sink_rejected)
				prior.outcome = closed.outcome;
		}
	first = slot_for_actor(*state, key(source));
	second = slot_for_actor(*state, key(target));
	const int anchor = relation == telemetry_battle_relation::support ? second : first;
	if (relation != telemetry_battle_relation::hostile &&
	    (anchor < 0 ||
	     !state->slots[static_cast<std::size_t>(anchor)]
		      .actors[static_cast<std::size_t>(actor_index(
			      state->slots[static_cast<std::size_t>(anchor)],
			      relation == telemetry_battle_relation::support ? key(target) :
									       key(source)))]
		      .active))
		return finish(refused(telemetry_battle_outcome::not_found));
	if ((first >= 0 && !ordered(state->slots[static_cast<std::size_t>(first)], at)) ||
	    (second >= 0 && !ordered(state->slots[static_cast<std::size_t>(second)], at)))
		return finish(refused(telemetry_battle_outcome::invalid));
	packet pending{};
	telemetry_battle_id merged{};
	int selected = first >= 0 ? first : second;
	if (selected < 0)
	{
		if (relation != telemetry_battle_relation::hostile ||
		    (source.actor.owner_subject_id == 0U && target.actor.owner_subject_id == 0U))
			return finish(refused(telemetry_battle_outcome::not_found));
		if (state->next_sequence == 0U)
			return finish(refused(telemetry_battle_outcome::capacity_full));
		for (std::size_t index = 0U; index < TELEMETRY_BATTLE_MAX_ACTIVE; ++index)
			if (!state->slots[index].occupied)
			{
				selected = static_cast<int>(index);
				break;
			}
		if (selected < 0)
			return finish(refused(telemetry_battle_outcome::capacity_full));
		auto &slot = state->slots[static_cast<std::size_t>(selected)];
		slot = {};
		slot.occupied = 1U;
		slot.id = { state->producer, state->next_sequence++ };
		slot.start_usec = slot.cut_usec = at;
		slot.last_observation_usec = slot.last_engagement_usec = at;
		slot.last_observation_utc_usec = utc;
		pending.append({ telemetry_battle_fact_kind::start });
	}
	else if (first >= 0 && second >= 0 && first != second)
	{
		const auto first_seq = state->slots[static_cast<std::size_t>(first)].id.sequence;
		const auto second_seq = state->slots[static_cast<std::size_t>(second)].id.sequence;
		selected = first_seq < second_seq ? first : second;
		const int donor_index = selected == first ? second : first;
		auto &keeper = state->slots[static_cast<std::size_t>(selected)];
		auto &donor = state->slots[static_cast<std::size_t>(donor_index)];
		if (keeper.actor_count + donor.actor_count > TELEMETRY_BATTLE_MAX_ACTORS)
		{
			seal(keeper, at);
			seal(donor, at);
			keeper.quality_flags |= TELEMETRY_QUALITY_CARDINALITY_OVERFLOW;
			donor.quality_flags |= TELEMETRY_QUALITY_CARDINALITY_OVERFLOW;
			classify(keeper);
			classify(donor);
			auto result = refused(telemetry_battle_outcome::capacity_full);
			result.battle = keeper.id;
			result.related_battle = donor.id;
			result.quality_flags = keeper.quality_flags | donor.quality_flags;
			return finish(result);
		}
		seal(keeper, at);
		seal(donor, at);
		merged = donor.id;
		const auto offset = keeper.actor_count;
		for (std::uint16_t index = 0U; index < donor.actor_count; ++index)
		{
			keeper.actors[offset + index] = donor.actors[index];
			remap_actor(keeper.actors[offset + index], offset, donor.actor_count);
		}
		keeper.actor_count += donor.actor_count;
		keeper.quality_flags |= donor.quality_flags | TELEMETRY_QUALITY_CONTEXT_UNKNOWN;
		if (donor.dropped_actor_count >
		    std::numeric_limits<std::uint32_t>::max() - keeper.dropped_actor_count)
		{
			keeper.dropped_actor_count = std::numeric_limits<std::uint32_t>::max();
			keeper.quality_flags |= TELEMETRY_QUALITY_CONTEXT_OVERFLOW;
		}
		else
			keeper.dropped_actor_count += donor.dropped_actor_count;
		donor = {};
		pending.append({ telemetry_battle_fact_kind::merge_alias, -1, {}, {}, merged });
	}
	auto &slot = state->slots[static_cast<std::size_t>(selected)];
	const auto source_roles =
		relation == telemetry_battle_relation::hostile ? TELEMETRY_BATTLE_ROLE_COMBAT :
		relation == telemetry_battle_relation::support ? TELEMETRY_BATTLE_ROLE_SUPPORT :
								 0U;
	const auto target_roles = relation == telemetry_battle_relation::hostile ?
					  TELEMETRY_BATTLE_ROLE_COMBAT :
				  relation == telemetry_battle_relation::group_presence ?
					  TELEMETRY_BATTLE_ROLE_GROUP_PRESENCE :
					  0U;
	const int prior_source = actor_index(slot, key(source)),
		  prior_target = actor_index(slot, key(target));
	if (pending.count == 0U && prior_source >= 0 && prior_target >= 0)
	{
		const auto &source_actor = slot.actors[static_cast<std::size_t>(prior_source)];
		const auto &target_actor = slot.actors[static_cast<std::size_t>(prior_target)];
		const auto edges = relation == telemetry_battle_relation::hostile ?
					   source_actor.hostile :
				   relation == telemetry_battle_relation::support ?
					   source_actor.friendly :
					   source_actor.group_presence;
		if (source_actor.active && target_actor.active &&
		    same_context(source_actor.context, source) &&
		    same_context(target_actor.context, target) &&
		    (source_actor.roles & source_roles) == source_roles &&
		    (target_actor.roles & target_roles) == target_roles &&
		    (edges & bit(static_cast<std::uint16_t>(prior_target))) != 0U)
		{
			slot.last_observation_usec = at;
			slot.last_observation_utc_usec = utc;
			if (relation != telemetry_battle_relation::group_presence)
				slot.last_engagement_usec = at;
			auto result = refused(telemetry_battle_outcome::idempotent);
			result.battle = slot.id;
			result.quality_flags = slot.quality_flags;
			return finish(result);
		}
	}
	const bool new_source = actor_index(slot, key(source)) < 0;
	const bool new_target = actor_index(slot, key(target)) < 0;
	if (static_cast<std::size_t>(slot.actor_count) + new_source +
		    (new_target && !same_key(key(source), key(target))) >
	    TELEMETRY_BATTLE_MAX_ACTORS)
	{
		seal(slot, at);
		slot.quality_flags |= TELEMETRY_QUALITY_CARDINALITY_OVERFLOW;
		if (slot.dropped_actor_count != std::numeric_limits<std::uint32_t>::max())
			++slot.dropped_actor_count;
		classify(slot);
		auto result = refused(telemetry_battle_outcome::capacity_full);
		result.battle = slot.id;
		result.quality_flags = slot.quality_flags;
		return finish(result);
	}
	seal(slot, at);
	const auto source_index =
		static_cast<std::uint16_t>(update_actor(slot, source, source_roles, pending));
	const auto target_index =
		static_cast<std::uint16_t>(update_actor(slot, target, target_roles, pending));
	auto &source_actor = slot.actors[source_index];
	auto &target_actor = slot.actors[target_index];
	auto &source_edges =
		relation == telemetry_battle_relation::hostile ? source_actor.hostile :
		relation == telemetry_battle_relation::support ? source_actor.friendly :
								 source_actor.group_presence;
	auto &target_edges =
		relation == telemetry_battle_relation::hostile ? target_actor.hostile :
		relation == telemetry_battle_relation::support ? target_actor.friendly :
								 target_actor.group_presence;
	if ((source_edges & bit(target_index)) == 0U)
	{
		source_edges |= bit(target_index);
		target_edges |= bit(source_index);
		pending.append({ telemetry_battle_fact_kind::relation, source_index, key(target),
				 relation });
	}
	slot.last_observation_usec = at;
	slot.last_observation_utc_usec = utc;
	if (relation != telemetry_battle_relation::group_presence)
		slot.last_engagement_usec = at;
	classify(slot);
	if (pending.count != 0U)
		pending.append({ telemetry_battle_fact_kind::cut });
	auto result = emit_packet(*state, slot, pending, at, utc, sink, context);
	result.related_battle = merged;
	return finish(result);
}

telemetry_battle_update telemetry_battle_context(telemetry_battle_state *state,
						 const telemetry_battle_actor_context &actor,
						 telemetry_monotonic_usec at,
						 telemetry_utc_usec utc, telemetry_battle_sink sink,
						 void *context) noexcept
{
	if (!ready(state, sink) || !telemetry_battle_actor_context_is_valid(actor) ||
	    !context_scope_matches(actor, state->producer))
		return refused(telemetry_battle_outcome::invalid);
	if (!admit_time(*state, at))
		return refused(telemetry_battle_outcome::invalid);
	const int index = slot_for_actor(*state, key(actor));
	if (index < 0)
		return refused(telemetry_battle_outcome::not_found);
	auto &slot = state->slots[static_cast<std::size_t>(index)];
	if (expired(*state, slot, at))
		return telemetry_battle_close(state, slot.id,
					      telemetry_battle_close_reason::inactivity, at, utc,
					      sink, context);
	if (!ordered(slot, at))
		return refused(telemetry_battle_outcome::invalid);
	const auto position = static_cast<std::uint16_t>(actor_index(slot, key(actor)));
	auto &entry = slot.actors[position];
	if (same_context(entry.context, actor))
		return refused(telemetry_battle_outcome::idempotent);
	seal(slot, at);
	if (entry.context.group_key != actor.group_key && (entry.roles & CONTRIBUTOR_ROLES) == 0U)
		entry.active = 0U;
	entry.context = actor;
	slot.quality_flags |= actor.quality_flags;
	slot.last_observation_usec = at;
	slot.last_observation_utc_usec = utc;
	classify(slot);
	packet pending{};
	pending.append({ telemetry_battle_fact_kind::actor_context, position });
	pending.append({ telemetry_battle_fact_kind::cut });
	return emit_packet(*state, slot, pending, at, utc, sink, context);
}

telemetry_battle_update telemetry_battle_leave(telemetry_battle_state *state,
					       telemetry_battle_actor_key actor,
					       telemetry_monotonic_usec at, telemetry_utc_usec utc,
					       telemetry_battle_sink sink, void *context) noexcept
{
	if (!ready(state, sink))
		return refused(telemetry_battle_outcome::invalid);
	if (!admit_time(*state, at))
		return refused(telemetry_battle_outcome::invalid);
	const int index = slot_for_actor(*state, actor);
	if (index < 0)
		return refused(telemetry_battle_outcome::not_found);
	auto &slot = state->slots[static_cast<std::size_t>(index)];
	if (expired(*state, slot, at))
		return telemetry_battle_close(state, slot.id,
					      telemetry_battle_close_reason::inactivity, at, utc,
					      sink, context);
	if (!ordered(slot, at))
		return refused(telemetry_battle_outcome::invalid);
	const auto position = static_cast<std::uint16_t>(actor_index(slot, actor));
	if (!slot.actors[position].active)
		return refused(telemetry_battle_outcome::idempotent);
	seal(slot, at);
	slot.actors[position].active = 0U;
	slot.last_observation_usec = at;
	slot.last_observation_utc_usec = utc;
	classify(slot);
	packet pending{};
	pending.append({ telemetry_battle_fact_kind::actor_context, position });
	pending.append({ telemetry_battle_fact_kind::cut });
	return emit_packet(*state, slot, pending, at, utc, sink, context);
}

telemetry_battle_update
telemetry_battle_close_all(telemetry_battle_state *state, telemetry_battle_close_reason reason,
			   telemetry_monotonic_usec at, telemetry_utc_usec utc,
			   telemetry_battle_sink sink, void *context) noexcept
{
	if (!ready(state, sink) || reason < telemetry_battle_close_reason::inactivity ||
	    reason > telemetry_battle_close_reason::shutdown)
		return refused(telemetry_battle_outcome::invalid);
	if (!admit_time(*state, at))
		return refused(telemetry_battle_outcome::invalid);
	telemetry_battle_update result{};
	for (const auto &slot : state->slots)
		if (slot.occupied)
		{
			const auto closed = telemetry_battle_close(state, slot.id, reason, at, utc,
								   sink, context);
			result.facts_attempted += closed.facts_attempted;
			result.facts_accepted += closed.facts_accepted;
			result.quality_flags |= closed.quality_flags;
			if (closed.outcome != telemetry_battle_outcome::accepted)
				result.outcome = closed.outcome;
		}
	return result;
}

telemetry_battle_update telemetry_battle_expire(telemetry_battle_state *state,
						telemetry_monotonic_usec at, telemetry_utc_usec utc,
						telemetry_battle_sink sink, void *context) noexcept
{
	if (!ready(state, sink))
		return refused(telemetry_battle_outcome::invalid);
	if (!admit_time(*state, at))
		return refused(telemetry_battle_outcome::invalid);
	telemetry_battle_update result{};
	for (const auto &slot : state->slots)
		if (slot.occupied && expired(*state, slot, at))
		{
			const auto closed = telemetry_battle_close(
				state, slot.id, telemetry_battle_close_reason::inactivity, at, utc,
				sink, context);
			result.facts_attempted += closed.facts_attempted;
			result.facts_accepted += closed.facts_accepted;
			result.quality_flags |= closed.quality_flags;
			if (closed.outcome != telemetry_battle_outcome::accepted)
				result.outcome = closed.outcome;
		}
	return result;
}
