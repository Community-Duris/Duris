#include "telemetry/telemetry_encounter.h"

#include <algorithm>
#include <limits>

namespace
{
constexpr telemetry_quality_mask ENCOUNTER_QUALITY = TELEMETRY_QUALITY_QUEUE_DROP;

bool same_id(const telemetry_encounter_id &a, const telemetry_encounter_id &b) noexcept
{
	return a.producer.boot_id == b.producer.boot_id &&
	       a.producer.process_id == b.producer.process_id && a.sequence == b.sequence;
}

bool same_participant(const telemetry_encounter_participant &a,
			     const telemetry_encounter_participant &b) noexcept
{
	return a.subject_id == b.subject_id && a.pid == b.pid;
}

bool same_source(const telemetry_encounter_source &a, const telemetry_encounter_source &b) noexcept
{
	return a.environment_id == b.environment_id && a.season_id == b.season_id &&
	       a.zone_vnum == b.zone_vnum && a.group_key == b.group_key;
}

telemetry_encounter_update empty_update() noexcept
{
	telemetry_encounter_update result{};
	result.outcome = telemetry_encounter_update_outcome::invalid;
	return result;
}

void set_identity(telemetry_encounter_update &result,
			  const telemetry_encounter_id &encounter) noexcept
{
	result.encounter = encounter;
}

int find_slot(const telemetry_encounter_state &state, const telemetry_encounter_id &encounter) noexcept
{
	for (std::size_t index = 0U; index < TELEMETRY_ENCOUNTER_MAX_ACTIVE; ++index)
		if (state.slots[index].occupied && same_id(state.slots[index].encounter, encounter))
			return static_cast<int>(index);
	return -1;
}

int find_participant(const telemetry_encounter_state &state,
			    const telemetry_encounter_participant &participant,
			    bool active_only) noexcept
{
	for (std::size_t slot_index = 0U; slot_index < TELEMETRY_ENCOUNTER_MAX_ACTIVE;
	     ++slot_index)
	{
		const auto &slot = state.slots[slot_index];
		if (!slot.occupied || slot.terminal)
			continue;
		for (std::size_t participant_index = 0U;
		     participant_index < TELEMETRY_ENCOUNTER_MAX_PARTICIPANTS; ++participant_index)
		{
			const auto &entry = slot.participants[participant_index];
			if (entry.occupied && (!active_only || entry.active) &&
			    same_participant(entry.participant, participant))
				return static_cast<int>(slot_index);
		}
	}
	return -1;
}

int find_group_slot(const telemetry_encounter_state &state,
			    const telemetry_encounter_source &source) noexcept
{
	if (source.group_key == 0U)
		return -1;
	for (std::size_t index = 0U; index < TELEMETRY_ENCOUNTER_MAX_ACTIVE; ++index)
		if (state.slots[index].occupied && !state.slots[index].terminal &&
		    same_source(state.slots[index].source, source))
			return static_cast<int>(index);
	return -1;
}

int find_free_slot(telemetry_encounter_state &state) noexcept
{
	for (std::size_t index = 0U; index < TELEMETRY_ENCOUNTER_MAX_ACTIVE; ++index)
		if (!state.slots[index].occupied)
			return static_cast<int>(index);
	return -1;
}

int find_free_participant(telemetry_encounter_slot &slot) noexcept
{
	for (std::size_t index = 0U; index < TELEMETRY_ENCOUNTER_MAX_PARTICIPANTS; ++index)
		if (!slot.participants[index].occupied)
			return static_cast<int>(index);
	return -1;
}

telemetry_encounter_participant_state *participant_state(
	telemetry_encounter_slot &slot, const telemetry_encounter_participant &participant) noexcept
{
	for (auto &entry : slot.participants)
		if (entry.occupied && same_participant(entry.participant, participant))
			return &entry;
	return nullptr;
}

void merge_mode(telemetry_encounter_mode &current, telemetry_encounter_mode observed) noexcept
{
	if (observed == telemetry_encounter_mode::unknown || current == observed)
		return;
	if (current == telemetry_encounter_mode::unknown)
		current = observed;
	else
		current = telemetry_encounter_mode::mixed;
}

bool monotonic_ordered(telemetry_monotonic_usec prior, telemetry_monotonic_usec current) noexcept
{
	return current >= prior;
}

void mark_bounded_overflow(telemetry_encounter_slot &slot) noexcept
{
	slot.quality_flags |= ENCOUNTER_QUALITY;
}

bool emit_event(telemetry_encounter_slot &slot, telemetry_encounter_event event,
		       telemetry_encounter_event_sink sink, void *sink_context,
		       telemetry_encounter_update &result) noexcept
{
	event.quality_flags |= slot.quality_flags;
	if (slot.event_count == std::numeric_limits<std::uint16_t>::max() ||
	    slot.event_count >= TELEMETRY_ENCOUNTER_MAX_EVENTS_PER_RUN)
	{
		mark_bounded_overflow(slot);
		event.quality_flags |= slot.quality_flags;
		++result.events_attempted;
		result.quality_flags |= slot.quality_flags;
		return false;
	}
	++slot.event_count;
	++result.events_attempted;
	const bool accepted = sink != nullptr && sink(sink_context, event);
	if (accepted)
		++result.events_accepted;
	else
	{
		slot.quality_flags |= TELEMETRY_QUALITY_QUEUE_DROP;
		result.quality_flags |= TELEMETRY_QUALITY_QUEUE_DROP;
	}
	result.quality_flags |= slot.quality_flags;
	return accepted;
}

void add_elapsed(telemetry_encounter_participant_state &entry,
		 telemetry_monotonic_usec at, telemetry_quality_mask &quality) noexcept
{
	if (!entry.active)
		return;
	if (!monotonic_ordered(entry.last_observed_usec, at))
	{
		quality |= TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
		return;
	}
	const telemetry_duration_usec delta = at - entry.last_observed_usec;
	if (delta > std::numeric_limits<telemetry_duration_usec>::max() - entry.participant_usec)
	{
		entry.participant_usec = std::numeric_limits<telemetry_duration_usec>::max();
		quality |= TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
	}
	else
		entry.participant_usec += delta;
	entry.last_observed_usec = at;
}

std::uint16_t active_count(const telemetry_encounter_slot &slot) noexcept
{
	std::uint16_t count = 0U;
	for (const auto &entry : slot.participants)
		if (entry.occupied && entry.active && count != std::numeric_limits<std::uint16_t>::max())
			++count;
	return count;
}

std::uint16_t participant_count(const telemetry_encounter_slot &slot) noexcept
{
	std::uint16_t count = 0U;
	for (const auto &entry : slot.participants)
		if (entry.occupied && count != std::numeric_limits<std::uint16_t>::max())
			++count;
	return count;
}

telemetry_encounter_event base_event(const telemetry_encounter_slot &slot,
					     telemetry_encounter_event_kind kind,
					     telemetry_encounter_outcome outcome,
					     std::uint16_t revision, telemetry_monotonic_usec at,
					     telemetry_utc_usec at_utc) noexcept
{
	telemetry_encounter_event event{};
	event.encounter = slot.encounter;
	event.kind = kind;
	event.mode = slot.mode;
	event.outcome = outcome;
	event.revision = revision;
	event.source = slot.source;
	event.at_monotonic_usec = at;
	event.at_utc_usec = at_utc;
	event.start_monotonic_usec = slot.start_monotonic_usec;
	event.start_utc_usec = slot.start_utc_usec;
	event.participant_count = participant_count(slot);
	event.expected_credit_count = slot.expected_credit_count;
	event.quality_flags = slot.quality_flags;
	return event;
}

void remember_terminal(telemetry_encounter_state &state,
			       const telemetry_encounter_slot &slot) noexcept
{
	const std::size_t index = state.terminal_next % TELEMETRY_ENCOUNTER_TERMINAL_CACHE;
	state.terminal[index] = {};
	state.terminal[index].occupied = 1U;
	state.terminal[index].encounter = slot.encounter;
	state.terminal[index].outcome = slot.outcome;
	state.terminal_next = static_cast<std::uint16_t>(
		(state.terminal_next + 1U) % TELEMETRY_ENCOUNTER_TERMINAL_CACHE);
}

int terminal_index(const telemetry_encounter_state &state,
			   const telemetry_encounter_id &encounter) noexcept
{
	for (std::size_t index = 0U; index < TELEMETRY_ENCOUNTER_TERMINAL_CACHE; ++index)
		if (state.terminal[index].occupied &&
		    same_id(state.terminal[index].encounter, encounter))
			return static_cast<int>(index);
	return -1;
}

telemetry_encounter_update add_participant(
		telemetry_encounter_slot &slot,
		telemetry_encounter_participant participant, telemetry_monotonic_usec at,
		telemetry_utc_usec at_utc, telemetry_encounter_event_kind event_kind,
		telemetry_encounter_outcome outcome, telemetry_encounter_event_sink sink,
		void *sink_context) noexcept
{
	telemetry_encounter_update result{};
	result.outcome = telemetry_encounter_update_outcome::accepted;
	set_identity(result, slot.encounter);
	if (!telemetry_encounter_participant_is_valid(participant) ||
	    !monotonic_ordered(slot.start_monotonic_usec, at))
	{
		result.outcome = telemetry_encounter_update_outcome::invalid;
		return result;
	}
	if (auto *existing = participant_state(slot, participant))
	{
		if (existing->active)
		{
			result.outcome = telemetry_encounter_update_outcome::idempotent;
			result.participants_active = active_count(slot);
			return result;
		}
		existing->active = 1U;
		existing->segment_start_usec = at;
		existing->last_observed_usec = at;
		telemetry_encounter_event event = base_event(slot, event_kind, outcome,
									  static_cast<std::uint16_t>(slot.event_count + 1U),
									  at, at_utc);
		event.participant = participant;
		const bool accepted = emit_event(slot, event, sink, sink_context, result);
		if (!accepted)
			result.outcome = telemetry_encounter_update_outcome::sink_rejected;
		result.participants_active = active_count(slot);
		return result;
	}
	const int free_index = find_free_participant(slot);
	if (free_index < 0)
	{
		mark_bounded_overflow(slot);
		result.outcome = telemetry_encounter_update_outcome::capacity_full;
		result.quality_flags = slot.quality_flags;
		return result;
	}
	auto &entry = slot.participants[static_cast<std::size_t>(free_index)];
	entry = {};
	entry.occupied = 1U;
	entry.active = 1U;
	entry.participant = participant;
	entry.segment_start_usec = at;
	entry.last_observed_usec = at;
	++slot.participant_count;
	telemetry_encounter_event event = base_event(slot, event_kind, outcome,
									  static_cast<std::uint16_t>(slot.event_count + 1U),
									  at, at_utc);
	event.participant = participant;
	const bool accepted = emit_event(slot, event, sink, sink_context, result);
	if (!accepted)
		result.outcome = telemetry_encounter_update_outcome::sink_rejected;
	result.participants_active = active_count(slot);
	return result;
}

telemetry_encounter_update close_slot(
	telemetry_encounter_state &state, telemetry_encounter_slot &slot,
		telemetry_encounter_outcome outcome, std::uint16_t expected_credit_count,
		telemetry_monotonic_usec at, telemetry_utc_usec at_utc,
		telemetry_encounter_event_sink sink, void *sink_context) noexcept
{
	telemetry_encounter_update result{};
	result.outcome = telemetry_encounter_update_outcome::accepted;
	set_identity(result, slot.encounter);
	if (slot.terminal)
	{
		result.outcome = slot.outcome == outcome ? telemetry_encounter_update_outcome::idempotent :
									 telemetry_encounter_update_outcome::duplicate_conflict;
		return result;
	}
	if (!telemetry_encounter_outcome_is_valid(outcome) ||
	    outcome == telemetry_encounter_outcome::unknown ||
	    !monotonic_ordered(slot.start_monotonic_usec, at))
	{
		result.outcome = telemetry_encounter_update_outcome::invalid;
		return result;
	}
	slot.expected_credit_count = expected_credit_count;
	slot.outcome = outcome;
	for (auto &entry : slot.participants)
		if (entry.occupied && entry.active)
			add_elapsed(entry, at, slot.quality_flags);
	telemetry_encounter_event close = base_event(
		slot, telemetry_encounter_event_kind::close, outcome,
		static_cast<std::uint16_t>(slot.event_count + 1U), at, at_utc);
	close.elapsed_usec = at - slot.start_monotonic_usec;
	close.participant_count = participant_count(slot);
	close.expected_credit_count = expected_credit_count;
	if (!emit_event(slot, close, sink, sink_context, result))
		result.outcome = telemetry_encounter_update_outcome::sink_rejected;
	for (auto &entry : slot.participants)
	{
		if (!entry.occupied)
			continue;
		telemetry_encounter_event summary = base_event(
				slot, telemetry_encounter_event_kind::participant_summary, outcome,
				static_cast<std::uint16_t>(slot.event_count + 1U), at, at_utc);
		summary.participant = entry.participant;
		summary.participant_usec = entry.participant_usec;
		if (!emit_event(slot, summary, sink, sink_context, result) &&
		    result.outcome == telemetry_encounter_update_outcome::accepted)
			result.outcome = telemetry_encounter_update_outcome::sink_rejected;
		entry.active = 0U;
	}
	slot.terminal = 1U;
	remember_terminal(state, slot);
	result.participants_active = 0U;
	result.quality_flags |= slot.quality_flags;
	/* The terminal cache preserves duplicate/late outcome classification while
	 * releasing the bounded participant storage for another run. */
	slot = {};
	return result;
}

} // namespace

void telemetry_encounter_state_init(telemetry_encounter_state *state) noexcept
{
	if (state != nullptr)
		*state = {};
}

telemetry_encounter_update telemetry_encounter_begin(
	telemetry_encounter_state *state, telemetry_encounter_id encounter,
	telemetry_encounter_source source, telemetry_encounter_mode mode,
	telemetry_encounter_participant participant, telemetry_monotonic_usec at_monotonic_usec,
	telemetry_utc_usec at_utc_usec, telemetry_encounter_event_sink sink, void *sink_context) noexcept
{
	telemetry_encounter_update result = empty_update();
	set_identity(result, encounter);
	if (state == nullptr || !telemetry_encounter_id_is_valid(encounter) ||
	    !telemetry_encounter_source_is_valid(source) || !telemetry_encounter_mode_is_valid(mode) ||
	    !telemetry_encounter_participant_is_valid(participant))
		return result;
	result.outcome = telemetry_encounter_update_outcome::accepted;
	const int existing_participant = find_participant(*state, participant, true);
	if (existing_participant >= 0)
	{
		auto &slot = state->slots[static_cast<std::size_t>(existing_participant)];
		merge_mode(slot.mode, mode);
		return add_participant(slot, participant, at_monotonic_usec, at_utc_usec,
					       telemetry_encounter_event_kind::participant_join,
					       telemetry_encounter_outcome::unknown, sink, sink_context);
	}
	const int existing_group = find_group_slot(*state, source);
	if (existing_group >= 0)
	{
		auto &slot = state->slots[static_cast<std::size_t>(existing_group)];
		merge_mode(slot.mode, mode);
		{
			telemetry_encounter_update joined = add_participant(
				slot, participant, at_monotonic_usec, at_utc_usec,
					       telemetry_encounter_event_kind::participant_join,
					       telemetry_encounter_outcome::unknown, sink, sink_context);
			if (joined.outcome == telemetry_encounter_update_outcome::accepted)
				joined.outcome = telemetry_encounter_update_outcome::joined_existing;
			return joined;
		}
	}
	const int free_index = find_free_slot(*state);
	if (free_index < 0)
	{
		result.outcome = telemetry_encounter_update_outcome::capacity_full;
		return result;
	}
	if (at_monotonic_usec == std::numeric_limits<telemetry_monotonic_usec>::max())
		return result;
	auto &slot = state->slots[static_cast<std::size_t>(free_index)];
	slot = {};
	slot.occupied = 1U;
	slot.encounter = encounter;
	slot.mode = mode;
	slot.outcome = telemetry_encounter_outcome::unknown;
	slot.source = source;
	slot.start_monotonic_usec = at_monotonic_usec;
	slot.start_utc_usec = at_utc_usec;
	telemetry_encounter_event start = base_event(
		slot, telemetry_encounter_event_kind::start, telemetry_encounter_outcome::unknown, 1U,
		at_monotonic_usec, at_utc_usec);
	if (!emit_event(slot, start, sink, sink_context, result))
		result.outcome = telemetry_encounter_update_outcome::sink_rejected;
	const auto joined = add_participant(slot, participant, at_monotonic_usec, at_utc_usec,
					    telemetry_encounter_event_kind::participant_join,
					    telemetry_encounter_outcome::unknown, sink, sink_context);
	result.events_attempted = static_cast<std::uint8_t>(
		std::min<unsigned>(std::numeric_limits<std::uint8_t>::max(),
				   static_cast<unsigned>(result.events_attempted) + joined.events_attempted));
	result.events_accepted = static_cast<std::uint8_t>(
		std::min<unsigned>(std::numeric_limits<std::uint8_t>::max(),
				   static_cast<unsigned>(result.events_accepted) + joined.events_accepted));
	result.quality_flags |= joined.quality_flags;
	result.participants_active = joined.participants_active;
	if (result.outcome == telemetry_encounter_update_outcome::accepted &&
	    joined.outcome != telemetry_encounter_update_outcome::accepted)
		result.outcome = joined.outcome;
	return result;
}

telemetry_encounter_update telemetry_encounter_join_group(
	telemetry_encounter_state *state, telemetry_encounter_source source,
	telemetry_encounter_participant participant, telemetry_monotonic_usec at_monotonic_usec,
	telemetry_utc_usec at_utc_usec, telemetry_encounter_event_sink sink, void *sink_context) noexcept
{
	telemetry_encounter_update result = empty_update();
	if (state == nullptr || !telemetry_encounter_source_is_valid(source) ||
	    !telemetry_encounter_participant_is_valid(participant))
		return result;
	const int existing = find_group_slot(*state, source);
	if (existing < 0)
	{
		result.outcome = telemetry_encounter_update_outcome::not_found;
		return result;
	}
	return add_participant(state->slots[static_cast<std::size_t>(existing)], participant,
				       at_monotonic_usec, at_utc_usec,
				       telemetry_encounter_event_kind::participant_join,
				       telemetry_encounter_outcome::unknown, sink, sink_context);
}

telemetry_encounter_update telemetry_encounter_observe(
	telemetry_encounter_state *state, telemetry_encounter_source source,
	telemetry_encounter_participant participant, telemetry_monotonic_usec at_monotonic_usec,
	telemetry_utc_usec at_utc_usec, telemetry_encounter_event_sink sink, void *sink_context) noexcept
{
	telemetry_encounter_update result = empty_update();
	if (state == nullptr || !telemetry_encounter_source_is_valid(source) ||
	    !telemetry_encounter_participant_is_valid(participant))
		return result;
	const int existing = find_participant(*state, participant, true);
	if (existing < 0)
	{
		result.outcome = telemetry_encounter_update_outcome::not_found;
		return result;
	}
	auto &slot = state->slots[static_cast<std::size_t>(existing)];
	if (slot.source.zone_vnum == source.zone_vnum)
	{
		merge_mode(slot.mode, telemetry_encounter_mode::unknown);
		result.outcome = telemetry_encounter_update_outcome::idempotent;
		result.encounter = slot.encounter;
		result.participants_active = active_count(slot);
		return result;
	}
	return telemetry_encounter_leave(state, participant, telemetry_encounter_outcome::withdrawal,
					 at_monotonic_usec, at_utc_usec, sink, sink_context);
}

telemetry_encounter_update telemetry_encounter_leave(
	telemetry_encounter_state *state, telemetry_encounter_participant participant,
		telemetry_encounter_outcome outcome, telemetry_monotonic_usec at_monotonic_usec,
		telemetry_utc_usec at_utc_usec, telemetry_encounter_event_sink sink, void *sink_context) noexcept
{
	telemetry_encounter_update result = empty_update();
	if (state == nullptr || !telemetry_encounter_participant_is_valid(participant) ||
	    !telemetry_encounter_outcome_is_valid(outcome) || outcome == telemetry_encounter_outcome::unknown)
		return result;
	const int existing = find_participant(*state, participant, true);
	if (existing < 0)
	{
		result.outcome = telemetry_encounter_update_outcome::not_found;
		return result;
	}
	auto &slot = state->slots[static_cast<std::size_t>(existing)];
	auto *entry = participant_state(slot, participant);
	if (entry == nullptr)
		return result;
	add_elapsed(*entry, at_monotonic_usec, slot.quality_flags);
	entry->active = 0U;
	telemetry_encounter_event event = base_event(
		slot, telemetry_encounter_event_kind::participant_leave, outcome,
		static_cast<std::uint16_t>(slot.event_count + 1U), at_monotonic_usec, at_utc_usec);
	event.participant = participant;
	event.participant_usec = entry->participant_usec;
	result.encounter = slot.encounter;
	result.outcome = telemetry_encounter_update_outcome::accepted;
	if (!emit_event(slot, event, sink, sink_context, result))
		result.outcome = telemetry_encounter_update_outcome::sink_rejected;
	result.participants_active = active_count(slot);
	if (result.participants_active == 0U)
	{
		const auto closed = close_slot(*state, slot, outcome, 0U, at_monotonic_usec, at_utc_usec,
					       sink, sink_context);
		result.events_attempted = static_cast<std::uint8_t>(
			std::min<unsigned>(std::numeric_limits<std::uint8_t>::max(),
					   static_cast<unsigned>(result.events_attempted) + closed.events_attempted));
		result.events_accepted = static_cast<std::uint8_t>(
			std::min<unsigned>(std::numeric_limits<std::uint8_t>::max(),
					   static_cast<unsigned>(result.events_accepted) + closed.events_accepted));
		result.quality_flags |= closed.quality_flags;
		if (result.outcome == telemetry_encounter_update_outcome::accepted &&
		    closed.outcome != telemetry_encounter_update_outcome::accepted)
			result.outcome = closed.outcome;
	}
	return result;
}

telemetry_encounter_update telemetry_encounter_close(
	telemetry_encounter_state *state, telemetry_encounter_id encounter,
		telemetry_encounter_outcome outcome, std::uint16_t expected_credit_count,
		telemetry_monotonic_usec at_monotonic_usec, telemetry_utc_usec at_utc_usec,
		telemetry_encounter_event_sink sink, void *sink_context) noexcept
{
	telemetry_encounter_update result = empty_update();
	set_identity(result, encounter);
	if (state == nullptr || !telemetry_encounter_id_is_valid(encounter) ||
	    !telemetry_encounter_outcome_is_valid(outcome) ||
	    outcome == telemetry_encounter_outcome::unknown)
		return result;
	const int existing = find_slot(*state, encounter);
	if (existing >= 0)
		return close_slot(*state, state->slots[static_cast<std::size_t>(existing)], outcome,
				  expected_credit_count, at_monotonic_usec, at_utc_usec, sink, sink_context);
	const int terminal = terminal_index(*state, encounter);
	if (terminal >= 0)
	{
		result.outcome = state->terminal[static_cast<std::size_t>(terminal)].outcome == outcome ?
					 telemetry_encounter_update_outcome::idempotent :
					 telemetry_encounter_update_outcome::duplicate_conflict;
		return result;
	}
	result.outcome = telemetry_encounter_update_outcome::not_found;
	return result;
}

telemetry_encounter_update telemetry_encounter_close_for_participant(
	telemetry_encounter_state *state, telemetry_encounter_participant participant,
		telemetry_encounter_outcome outcome, std::uint16_t expected_credit_count,
		telemetry_monotonic_usec at_monotonic_usec, telemetry_utc_usec at_utc_usec,
		telemetry_encounter_event_sink sink, void *sink_context) noexcept
{
	telemetry_encounter_update result = empty_update();
	if (state == nullptr || !telemetry_encounter_participant_is_valid(participant))
		return result;
	const int existing = find_participant(*state, participant, false);
	if (existing < 0)
	{
		for (std::size_t index = 0U; index < TELEMETRY_ENCOUNTER_MAX_ACTIVE; ++index)
			if (state->slots[index].occupied && !state->slots[index].terminal &&
			    state->slots[index].source.group_key ==
				    static_cast<telemetry_id>(participant.pid))
				return close_slot(*state, state->slots[index], outcome, expected_credit_count,
						 at_monotonic_usec, at_utc_usec, sink, sink_context);
		result.outcome = telemetry_encounter_update_outcome::not_found;
		return result;
	}
	return close_slot(*state, state->slots[static_cast<std::size_t>(existing)], outcome,
				  expected_credit_count, at_monotonic_usec, at_utc_usec, sink, sink_context);
}

telemetry_encounter_update telemetry_encounter_close_all(
	telemetry_encounter_state *state, telemetry_encounter_outcome outcome,
		telemetry_monotonic_usec at_monotonic_usec, telemetry_utc_usec at_utc_usec,
		telemetry_encounter_event_sink sink, void *sink_context) noexcept
{
	telemetry_encounter_update result = empty_update();
	result.outcome = telemetry_encounter_update_outcome::accepted;
	if (state == nullptr || !telemetry_encounter_outcome_is_valid(outcome) ||
	    outcome == telemetry_encounter_outcome::unknown)
	{
		result.outcome = telemetry_encounter_update_outcome::invalid;
		return result;
	}
	for (std::size_t index = 0U; index < TELEMETRY_ENCOUNTER_MAX_ACTIVE; ++index)
	{
		if (!state->slots[index].occupied || state->slots[index].terminal)
			continue;
		const auto closed = close_slot(*state, state->slots[index], outcome, 0U,
					       at_monotonic_usec, at_utc_usec, sink, sink_context);
		result.events_attempted = static_cast<std::uint8_t>(
			std::min<unsigned>(std::numeric_limits<std::uint8_t>::max(),
					   static_cast<unsigned>(result.events_attempted) + closed.events_attempted));
		result.events_accepted = static_cast<std::uint8_t>(
			std::min<unsigned>(std::numeric_limits<std::uint8_t>::max(),
					   static_cast<unsigned>(result.events_accepted) + closed.events_accepted));
		result.quality_flags |= closed.quality_flags;
		if (result.encounter.sequence == 0U)
			result.encounter = closed.encounter;
		if (closed.outcome != telemetry_encounter_update_outcome::accepted)
			result.outcome = closed.outcome;
	}
	return result;
}
