#include "telemetry/telemetry_combat_summary.h"

#include <algorithm>
#include <limits>

namespace
{
bool same_id(const telemetry_encounter_id &left, const telemetry_encounter_id &right) noexcept
{
	return left.producer.boot_id == right.producer.boot_id &&
	       left.producer.process_id == right.producer.process_id &&
	       left.sequence == right.sequence;
}

bool same_actor(const telemetry_combat_actor_ref &left,
		const telemetry_combat_actor_ref &right) noexcept
{
	return left.actor_id == right.actor_id && left.actor_pid == right.actor_pid &&
	       left.owner_subject_id == right.owner_subject_id && left.kind == right.kind;
}

telemetry_combat_summary_update empty_update(const telemetry_encounter_id &encounter) noexcept
{
	telemetry_combat_summary_update result{};
	result.outcome = telemetry_combat_summary_outcome::invalid;
	result.encounter = encounter;
	return result;
}

int find_slot(const telemetry_combat_summary_state &state,
	      const telemetry_encounter_id &encounter) noexcept
{
	for (std::size_t index = 0U; index < TELEMETRY_COMBAT_SUMMARY_MAX_ACTIVE; ++index)
		if (state.slots[index].occupied && same_id(state.slots[index].encounter, encounter))
			return static_cast<int>(index);
	return -1;
}

int find_slot_for_actor(const telemetry_combat_summary_state &state,
			const telemetry_combat_actor_ref &actor) noexcept
{
	for (std::size_t slot_index = 0U; slot_index < TELEMETRY_COMBAT_SUMMARY_MAX_ACTIVE;
	     ++slot_index)
	{
		const auto &slot = state.slots[slot_index];
		if (!slot.occupied)
			continue;
		for (const auto &participant : slot.participants)
			if (participant.occupied && same_actor(participant.actor, actor))
				return static_cast<int>(slot_index);
	}
	return -1;
}

int find_free_slot(telemetry_combat_summary_state &state) noexcept
{
	for (std::size_t index = 0U; index < TELEMETRY_COMBAT_SUMMARY_MAX_ACTIVE; ++index)
		if (!state.slots[index].occupied)
			return static_cast<int>(index);
	return -1;
}

int find_free_participant(telemetry_combat_summary_slot &slot) noexcept
{
	for (std::size_t index = 0U; index < TELEMETRY_COMBAT_SUMMARY_MAX_ACTORS; ++index)
		if (!slot.participants[index].occupied)
			return static_cast<int>(index);
	return -1;
}

telemetry_combat_summary_participant_state *
find_participant(telemetry_combat_summary_slot &slot,
		 const telemetry_combat_actor_ref &actor) noexcept
{
	for (auto &participant : slot.participants)
		if (participant.occupied && same_actor(participant.actor, actor))
			return &participant;
	return nullptr;
}

void mark_overflow(telemetry_combat_summary_slot &slot) noexcept
{
	slot.quality_flags |= TELEMETRY_QUALITY_CARDINALITY_OVERFLOW;
}

void add_u64(std::uint64_t &value, std::uint64_t amount, telemetry_quality_mask &quality) noexcept
{
	if (amount > std::numeric_limits<std::uint64_t>::max() - value)
	{
		value = std::numeric_limits<std::uint64_t>::max();
		quality |= TELEMETRY_QUALITY_CARDINALITY_OVERFLOW;
		return;
	}
	value += amount;
}

void add_duration(telemetry_duration_usec &value, telemetry_duration_usec amount,
		  telemetry_quality_mask &quality) noexcept
{
	add_u64(value, amount, quality);
}

telemetry_monotonic_usec ordered_time(const telemetry_combat_summary_slot &slot,
				      telemetry_monotonic_usec at,
				      telemetry_quality_mask &quality) noexcept
{
	if (at < slot.start_monotonic_usec)
	{
		quality |= TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
		return slot.start_monotonic_usec;
	}
	return at;
}

std::uint32_t actor_context_flags(const telemetry_combat_actor_ref &actor) noexcept
{
	if (actor.kind == telemetry_combat_actor_kind::pet)
		return TELEMETRY_COMBAT_MODIFIER_PET;
	if (actor.kind == telemetry_combat_actor_kind::npc)
		return TELEMETRY_COMBAT_MODIFIER_NPC;
	return TELEMETRY_COMBAT_MODIFIER_NONE;
}

void add_unique_player(telemetry_combat_summary_slot &slot,
		       const telemetry_combat_actor_ref &actor) noexcept
{
	if (actor.owner_subject_id == 0U)
		return;
	for (std::size_t index = 0U; index < slot.unique_player_count; ++index)
		if (slot.unique_players[index] == actor.owner_subject_id)
			return;
	if (slot.unique_player_count >= TELEMETRY_COMBAT_SUMMARY_MAX_UNIQUE_PLAYERS)
	{
		mark_overflow(slot);
		return;
	}
	slot.unique_players[slot.unique_player_count++] = actor.owner_subject_id;
}

telemetry_combat_summary_participant_state *
add_actor(telemetry_combat_summary_slot &slot, const telemetry_combat_actor_ref &actor,
	  telemetry_combat_summary_update &result) noexcept
{
	if (auto *existing = find_participant(slot, actor))
	{
		existing->active = 1U;
		return existing;
	}
	const int free_index = find_free_participant(slot);
	if (free_index < 0)
	{
		if (slot.dropped_participant_count != std::numeric_limits<std::uint16_t>::max())
			++slot.dropped_participant_count;
		mark_overflow(slot);
		result.outcome = telemetry_combat_summary_outcome::capacity_full;
		result.quality_flags |= slot.quality_flags;
		return nullptr;
	}
	auto &entry = slot.participants[static_cast<std::size_t>(free_index)];
	entry = {};
	entry.occupied = 1U;
	entry.active = 1U;
	entry.actor = actor;
	++slot.participant_count;
	add_unique_player(slot, actor);
	result.outcome = telemetry_combat_summary_outcome::accepted;
	return &entry;
}

telemetry_combat_summary_slot *slot_for_event(telemetry_combat_summary_state &state,
					      const telemetry_combat_actor_ref &source,
					      const telemetry_combat_actor_ref &target) noexcept
{
	int index = find_slot_for_actor(state, source);
	if (index < 0)
		index = find_slot_for_actor(state, target);
	return index < 0 ? nullptr : &state.slots[static_cast<std::size_t>(index)];
}

void finish_tanking(telemetry_combat_summary_slot &slot,
		    telemetry_combat_summary_participant_state &entry,
		    telemetry_monotonic_usec at) noexcept
{
	if (!entry.tanking_active)
		return;
	if (at < entry.tanking_start_usec)
	{
		slot.quality_flags |= TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
		entry.tanking_active = 0U;
		entry.tanking_target_id = 0U;
		return;
	}
	add_duration(entry.tanking_usec, at - entry.tanking_start_usec, slot.quality_flags);
	entry.tanking_active = 0U;
	entry.tanking_target_id = 0U;
}

void finish_casting(telemetry_combat_summary_slot &slot,
		    telemetry_combat_summary_participant_state &entry,
		    telemetry_monotonic_usec at) noexcept
{
	if (!entry.casting_active)
		return;
	if (at < entry.cast_start_usec)
	{
		slot.quality_flags |= TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
		entry.casting_active = 0U;
		return;
	}
	add_duration(entry.casting_elapsed_usec, at - entry.cast_start_usec, slot.quality_flags);
	entry.casting_active = 0U;
}

telemetry_combat_summary_update update_for_slot(const telemetry_combat_summary_slot &slot,
						telemetry_combat_summary_outcome outcome) noexcept
{
	telemetry_combat_summary_update result{};
	result.outcome = outcome;
	result.quality_flags = slot.quality_flags;
	result.encounter = slot.encounter;
	return result;
}

void merge_update(telemetry_combat_summary_update &target,
		  const telemetry_combat_summary_update &source) noexcept
{
	if (target.encounter.sequence == 0U)
		target.encounter = source.encounter;
	target.quality_flags |= source.quality_flags;
	if (source.rows_attempted >
	    std::numeric_limits<std::uint16_t>::max() - target.rows_attempted)
		target.rows_attempted = std::numeric_limits<std::uint16_t>::max();
	else
		target.rows_attempted =
			static_cast<std::uint16_t>(target.rows_attempted + source.rows_attempted);
	if (source.rows_accepted > std::numeric_limits<std::uint16_t>::max() - target.rows_accepted)
		target.rows_accepted = std::numeric_limits<std::uint16_t>::max();
	else
		target.rows_accepted =
			static_cast<std::uint16_t>(target.rows_accepted + source.rows_accepted);
	if (source.rows_dropped > std::numeric_limits<std::uint16_t>::max() - target.rows_dropped)
		target.rows_dropped = std::numeric_limits<std::uint16_t>::max();
	else
		target.rows_dropped =
			static_cast<std::uint16_t>(target.rows_dropped + source.rows_dropped);
	if (source.outcome == telemetry_combat_summary_outcome::invalid)
		target.outcome = source.outcome;
	else if (target.outcome == telemetry_combat_summary_outcome::accepted &&
		 source.outcome != telemetry_combat_summary_outcome::accepted)
		target.outcome = source.outcome;
}

telemetry_combat_summary_payload
payload_for(const telemetry_combat_summary_slot &slot,
	    const telemetry_combat_summary_participant_state &entry, std::uint16_t revision,
	    telemetry_monotonic_usec at, telemetry_utc_usec at_utc) noexcept
{
	telemetry_combat_summary_payload summary{};
	summary.encounter = slot.encounter;
	summary.source = slot.source;
	summary.mode = slot.mode;
	summary.outcome = slot.outcome;
	summary.actor_kind = entry.actor.kind;
	summary.reserved = 0U;
	summary.revision = revision;
	summary.actor_id = entry.actor.actor_id;
	summary.actor_pid = entry.actor.actor_pid;
	summary.owner_subject_id = entry.actor.owner_subject_id;
	summary.unique_player_count = slot.unique_player_count;
	summary.participant_count = slot.participant_count;
	summary.dropped_participant_count = slot.dropped_participant_count;
	summary.power_band = entry.actor.power_band;
	summary.opponent_power_band = entry.opponent_power_band;
	summary.opponent_count = entry.opponent_count;
	summary.modifier_flags = entry.modifier_flags;
	summary.start_monotonic_usec = slot.start_monotonic_usec;
	summary.end_monotonic_usec = at;
	summary.start_utc_usec = slot.start_utc_usec;
	summary.end_utc_usec = at_utc;
	summary.damage_dealt = entry.damage_dealt;
	summary.damage_taken = entry.damage_taken;
	summary.healing_attempted = entry.healing_attempted;
	summary.effective_healing = entry.effective_healing;
	summary.overhealing = entry.overhealing;
	summary.control_applications = entry.control_applications;
	summary.casting_attempts = entry.casting_attempts;
	summary.casting_completions = entry.casting_completions;
	summary.casting_aborts = entry.casting_aborts;
	summary.casting_elapsed_usec = entry.casting_elapsed_usec;
	summary.tanking_usec = entry.tanking_usec;
	summary.quality_flags = slot.quality_flags;
	return summary;
}

telemetry_combat_summary_update close_slot(telemetry_combat_summary_slot &slot,
					   telemetry_encounter_outcome outcome,
					   telemetry_monotonic_usec at, telemetry_utc_usec at_utc,
					   telemetry_combat_summary_sink sink,
					   void *sink_context) noexcept
{
	telemetry_combat_summary_update result =
		update_for_slot(slot, telemetry_combat_summary_outcome::accepted);
	if (!telemetry_encounter_outcome_is_valid(outcome) ||
	    outcome == telemetry_encounter_outcome::unknown || at < slot.start_monotonic_usec)
	{
		result.outcome = telemetry_combat_summary_outcome::invalid;
		return result;
	}
	slot.outcome = outcome;
	for (auto &entry : slot.participants)
	{
		if (!entry.occupied)
			continue;
		finish_tanking(slot, entry, at);
		if (entry.casting_active)
		{
			finish_casting(slot, entry, at);
			add_u64(entry.casting_aborts, 1U, slot.quality_flags);
			slot.quality_flags |= TELEMETRY_QUALITY_UNCLOSED_TAIL;
		}
	}
	for (std::size_t index = 0U; index < TELEMETRY_COMBAT_SUMMARY_MAX_ACTORS; ++index)
	{
		const auto &entry = slot.participants[index];
		if (!entry.occupied)
			continue;
		const std::uint16_t revision = static_cast<std::uint16_t>(index + 1U);
		telemetry_combat_summary_payload summary =
			payload_for(slot, entry, revision, at, at_utc);
		++result.rows_attempted;
		if (telemetry_combat_summary_payload_is_valid(summary) && sink != nullptr &&
		    sink(sink_context, summary))
			++result.rows_accepted;
		else
		{
			++result.rows_dropped;
			result.outcome = telemetry_combat_summary_outcome::sink_rejected;
			slot.quality_flags |= TELEMETRY_QUALITY_QUEUE_DROP;
		}
	}
	result.quality_flags |= slot.quality_flags;
	slot = {};
	return result;
}
} // namespace

void telemetry_combat_summary_state_init(telemetry_combat_summary_state *state) noexcept
{
	if (state != nullptr)
		*state = {};
}

telemetry_combat_summary_update telemetry_combat_summary_begin(
	telemetry_combat_summary_state *state, telemetry_encounter_id encounter,
	telemetry_encounter_source source, telemetry_encounter_mode mode,
	telemetry_monotonic_usec at_monotonic_usec, telemetry_utc_usec at_utc_usec) noexcept
{
	telemetry_combat_summary_update result = empty_update(encounter);
	if (state == nullptr || !telemetry_encounter_id_is_valid(encounter) ||
	    !telemetry_encounter_source_is_valid(source) ||
	    !telemetry_encounter_mode_is_valid(mode) || mode == telemetry_encounter_mode::unknown)
		return result;
	if (find_slot(*state, encounter) >= 0)
	{
		result.outcome = telemetry_combat_summary_outcome::idempotent;
		return result;
	}
	const int free_index = find_free_slot(*state);
	if (free_index < 0)
	{
		result.outcome = telemetry_combat_summary_outcome::capacity_full;
		result.quality_flags = TELEMETRY_QUALITY_CARDINALITY_OVERFLOW;
		return result;
	}
	if (at_monotonic_usec == std::numeric_limits<telemetry_monotonic_usec>::max())
		return result;
	auto &slot = state->slots[static_cast<std::size_t>(free_index)];
	slot = {};
	slot.occupied = 1U;
	slot.encounter = encounter;
	slot.source = source;
	slot.mode = mode;
	slot.outcome = telemetry_encounter_outcome::unknown;
	slot.start_monotonic_usec = at_monotonic_usec;
	slot.start_utc_usec = at_utc_usec;
	result.outcome = telemetry_combat_summary_outcome::accepted;
	return result;
}

telemetry_combat_summary_update telemetry_combat_summary_add_actor(
	telemetry_combat_summary_state *state, telemetry_encounter_id encounter,
	telemetry_combat_actor_ref actor, telemetry_monotonic_usec at_monotonic_usec) noexcept
{
	telemetry_combat_summary_update result = empty_update(encounter);
	if (state == nullptr || !telemetry_encounter_id_is_valid(encounter) ||
	    !telemetry_combat_actor_ref_is_valid(actor))
		return result;
	const int index = find_slot(*state, encounter);
	if (index < 0)
	{
		result.outcome = telemetry_combat_summary_outcome::not_found;
		return result;
	}
	auto &slot = state->slots[static_cast<std::size_t>(index)];
	if (at_monotonic_usec < slot.start_monotonic_usec)
		slot.quality_flags |= TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
	if (find_participant(slot, actor) != nullptr)
	{
		result.outcome = telemetry_combat_summary_outcome::idempotent;
		result.quality_flags = slot.quality_flags;
		return result;
	}
	(void)add_actor(slot, actor, result);
	result.quality_flags |= slot.quality_flags;
	return result;
}

telemetry_combat_summary_update telemetry_combat_summary_leave_actor(
	telemetry_combat_summary_state *state, telemetry_encounter_id encounter,
	telemetry_combat_actor_ref actor, telemetry_monotonic_usec at_monotonic_usec) noexcept
{
	telemetry_combat_summary_update result = empty_update(encounter);
	if (state == nullptr || !telemetry_encounter_id_is_valid(encounter) ||
	    !telemetry_combat_actor_ref_is_valid(actor))
		return result;
	const int index = find_slot(*state, encounter);
	if (index < 0)
	{
		result.outcome = telemetry_combat_summary_outcome::not_found;
		return result;
	}
	auto &slot = state->slots[static_cast<std::size_t>(index)];
	auto *entry = find_participant(slot, actor);
	if (entry == nullptr)
	{
		result.outcome = telemetry_combat_summary_outcome::not_found;
		return result;
	}
	entry->active = 0U;
	if (at_monotonic_usec < slot.start_monotonic_usec)
		slot.quality_flags |= TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
	result.outcome = telemetry_combat_summary_outcome::accepted;
	result.quality_flags = slot.quality_flags;
	return result;
}

telemetry_combat_summary_update telemetry_combat_summary_record_damage(
	telemetry_combat_summary_state *state, telemetry_combat_actor_ref source,
	telemetry_combat_actor_ref target, std::uint64_t amount,
	telemetry_monotonic_usec at_monotonic_usec, std::uint32_t modifier_flags) noexcept
{
	telemetry_combat_summary_update result = empty_update({});
	if (state == nullptr || !telemetry_combat_actor_ref_is_valid(source) ||
	    !telemetry_combat_actor_ref_is_valid(target) ||
	    !telemetry_combat_modifier_flags_are_valid(modifier_flags))
		return result;
	auto *slot = slot_for_event(*state, source, target);
	if (slot == nullptr)
	{
		result.outcome = telemetry_combat_summary_outcome::not_found;
		return result;
	}
	result.encounter = slot->encounter;
	auto *source_entry = add_actor(*slot, source, result);
	auto *target_entry = add_actor(*slot, target, result);
	if (source_entry != nullptr)
	{
		source_entry->modifier_flags |= modifier_flags | actor_context_flags(source);
		add_u64(source_entry->damage_dealt, amount, slot->quality_flags);
	}
	if (target_entry != nullptr)
	{
		target_entry->modifier_flags |= actor_context_flags(target);
		add_u64(target_entry->damage_taken, amount, slot->quality_flags);
	}
	(void)ordered_time(*slot, at_monotonic_usec, slot->quality_flags);
	result.outcome = source_entry != nullptr ? telemetry_combat_summary_outcome::accepted :
						   telemetry_combat_summary_outcome::capacity_full;
	result.quality_flags = slot->quality_flags;
	return result;
}

telemetry_combat_summary_update telemetry_combat_summary_record_healing(
	telemetry_combat_summary_state *state, telemetry_combat_actor_ref healer,
	telemetry_combat_actor_ref target, std::uint64_t attempted, std::uint64_t effective,
	telemetry_monotonic_usec at_monotonic_usec, std::uint32_t modifier_flags) noexcept
{
	telemetry_combat_summary_update result = empty_update({});
	if (state == nullptr || !telemetry_combat_actor_ref_is_valid(healer) ||
	    !telemetry_combat_actor_ref_is_valid(target) ||
	    !telemetry_combat_modifier_flags_are_valid(modifier_flags))
		return result;
	auto *slot = slot_for_event(*state, healer, target);
	if (slot == nullptr)
	{
		result.outcome = telemetry_combat_summary_outcome::not_found;
		return result;
	}
	result.encounter = slot->encounter;
	auto *healer_entry = add_actor(*slot, healer, result);
	(void)add_actor(*slot, target, result);
	if (healer_entry != nullptr)
	{
		effective = std::min(effective, attempted);
		healer_entry->modifier_flags |= modifier_flags | actor_context_flags(healer);
		add_u64(healer_entry->healing_attempted, attempted, slot->quality_flags);
		add_u64(healer_entry->effective_healing, effective, slot->quality_flags);
		add_u64(healer_entry->overhealing, attempted - effective, slot->quality_flags);
	}
	(void)ordered_time(*slot, at_monotonic_usec, slot->quality_flags);
	result.outcome = healer_entry != nullptr ? telemetry_combat_summary_outcome::accepted :
						   telemetry_combat_summary_outcome::capacity_full;
	result.quality_flags = slot->quality_flags;
	return result;
}

telemetry_combat_summary_update telemetry_combat_summary_record_control(
	telemetry_combat_summary_state *state, telemetry_combat_actor_ref source,
	telemetry_combat_actor_ref target, std::uint16_t applications,
	telemetry_monotonic_usec at_monotonic_usec, std::uint32_t modifier_flags) noexcept
{
	telemetry_combat_summary_update result = empty_update({});
	if (state == nullptr || !telemetry_combat_actor_ref_is_valid(source) ||
	    !telemetry_combat_actor_ref_is_valid(target) ||
	    !telemetry_combat_modifier_flags_are_valid(modifier_flags))
		return result;
	auto *slot = slot_for_event(*state, source, target);
	if (slot == nullptr)
	{
		result.outcome = telemetry_combat_summary_outcome::not_found;
		return result;
	}
	result.encounter = slot->encounter;
	auto *entry = add_actor(*slot, source, result);
	(void)add_actor(*slot, target, result);
	if (entry != nullptr)
	{
		entry->modifier_flags |= modifier_flags | TELEMETRY_COMBAT_MODIFIER_CONTROL |
					 actor_context_flags(source);
		add_u64(entry->control_applications, applications, slot->quality_flags);
	}
	(void)ordered_time(*slot, at_monotonic_usec, slot->quality_flags);
	result.outcome = entry != nullptr ? telemetry_combat_summary_outcome::accepted :
					    telemetry_combat_summary_outcome::capacity_full;
	result.quality_flags = slot->quality_flags;
	return result;
}

telemetry_combat_summary_update telemetry_combat_summary_record_tanking(
	telemetry_combat_summary_state *state, telemetry_combat_actor_ref actor,
	const telemetry_combat_actor_ref *target, telemetry_monotonic_usec at_monotonic_usec,
	std::uint32_t modifier_flags) noexcept
{
	telemetry_combat_summary_update result = empty_update({});
	if (state == nullptr || !telemetry_combat_actor_ref_is_valid(actor) ||
	    (target != nullptr && !telemetry_combat_actor_ref_is_valid(*target)) ||
	    !telemetry_combat_modifier_flags_are_valid(modifier_flags))
		return result;
	const int index = find_slot_for_actor(*state, actor);
	if (index < 0)
	{
		result.outcome = telemetry_combat_summary_outcome::not_found;
		return result;
	}
	auto &slot = state->slots[static_cast<std::size_t>(index)];
	result.encounter = slot.encounter;
	auto *entry = find_participant(slot, actor);
	if (entry == nullptr)
	{
		result.outcome = telemetry_combat_summary_outcome::not_found;
		return result;
	}
	entry->modifier_flags |= modifier_flags | TELEMETRY_COMBAT_MODIFIER_TANKING |
				 actor_context_flags(actor);
	const telemetry_id target_id = target == nullptr ? 0U : target->actor_id;
	if (entry->tanking_active && entry->tanking_target_id != target_id)
		finish_tanking(slot, *entry,
			       ordered_time(slot, at_monotonic_usec, slot.quality_flags));
	if (target != nullptr)
	{
		(void)add_actor(slot, *target, result);
		entry->opponent_power_band =
			std::max(entry->opponent_power_band, target->power_band);
		if (!entry->tanking_active || entry->tanking_target_id != target_id)
		{
			if (entry->opponent_count != std::numeric_limits<std::uint16_t>::max())
				++entry->opponent_count;
			entry->tanking_start_usec =
				ordered_time(slot, at_monotonic_usec, slot.quality_flags);
			entry->tanking_target_id = target_id;
			entry->tanking_active = 1U;
		}
	}
	else
		finish_tanking(slot, *entry,
			       ordered_time(slot, at_monotonic_usec, slot.quality_flags));
	result.outcome = telemetry_combat_summary_outcome::accepted;
	result.quality_flags = slot.quality_flags;
	return result;
}

telemetry_combat_summary_update telemetry_combat_summary_cast_attempt(
	telemetry_combat_summary_state *state, telemetry_combat_actor_ref actor, int spell,
	telemetry_monotonic_usec at_monotonic_usec, std::uint32_t modifier_flags) noexcept
{
	(void)spell;
	telemetry_combat_summary_update result = empty_update({});
	if (state == nullptr || !telemetry_combat_actor_ref_is_valid(actor) ||
	    !telemetry_combat_modifier_flags_are_valid(modifier_flags))
		return result;
	const int index = find_slot_for_actor(*state, actor);
	if (index < 0)
	{
		result.outcome = telemetry_combat_summary_outcome::not_found;
		return result;
	}
	auto &slot = state->slots[static_cast<std::size_t>(index)];
	result.encounter = slot.encounter;
	auto *entry = find_participant(slot, actor);
	if (entry == nullptr)
	{
		result.outcome = telemetry_combat_summary_outcome::not_found;
		return result;
	}
	if (entry->casting_active)
	{
		finish_casting(slot, *entry,
			       ordered_time(slot, at_monotonic_usec, slot.quality_flags));
		add_u64(entry->casting_aborts, 1U, slot.quality_flags);
	}
	entry->modifier_flags |= modifier_flags | TELEMETRY_COMBAT_MODIFIER_SPELL |
				 actor_context_flags(actor);
	add_u64(entry->casting_attempts, 1U, slot.quality_flags);
	entry->cast_start_usec = ordered_time(slot, at_monotonic_usec, slot.quality_flags);
	entry->casting_active = 1U;
	result.outcome = telemetry_combat_summary_outcome::accepted;
	result.quality_flags = slot.quality_flags;
	return result;
}

telemetry_combat_summary_update
telemetry_combat_summary_cast_complete(telemetry_combat_summary_state *state,
				       telemetry_combat_actor_ref actor,
				       telemetry_monotonic_usec at_monotonic_usec) noexcept
{
	telemetry_combat_summary_update result = empty_update({});
	if (state == nullptr || !telemetry_combat_actor_ref_is_valid(actor))
		return result;
	const int index = find_slot_for_actor(*state, actor);
	if (index < 0)
	{
		result.outcome = telemetry_combat_summary_outcome::not_found;
		return result;
	}
	auto &slot = state->slots[static_cast<std::size_t>(index)];
	result.encounter = slot.encounter;
	auto *entry = find_participant(slot, actor);
	if (entry == nullptr || !entry->casting_active)
	{
		result.outcome = telemetry_combat_summary_outcome::idempotent;
		return result;
	}
	finish_casting(slot, *entry, ordered_time(slot, at_monotonic_usec, slot.quality_flags));
	add_u64(entry->casting_completions, 1U, slot.quality_flags);
	result.outcome = telemetry_combat_summary_outcome::accepted;
	result.quality_flags = slot.quality_flags;
	return result;
}

telemetry_combat_summary_update
telemetry_combat_summary_cast_abort(telemetry_combat_summary_state *state,
				    telemetry_combat_actor_ref actor,
				    telemetry_monotonic_usec at_monotonic_usec) noexcept
{
	telemetry_combat_summary_update result = empty_update({});
	if (state == nullptr || !telemetry_combat_actor_ref_is_valid(actor))
		return result;
	const int index = find_slot_for_actor(*state, actor);
	if (index < 0)
	{
		result.outcome = telemetry_combat_summary_outcome::not_found;
		return result;
	}
	auto &slot = state->slots[static_cast<std::size_t>(index)];
	result.encounter = slot.encounter;
	auto *entry = find_participant(slot, actor);
	if (entry == nullptr || !entry->casting_active)
	{
		result.outcome = telemetry_combat_summary_outcome::idempotent;
		return result;
	}
	finish_casting(slot, *entry, ordered_time(slot, at_monotonic_usec, slot.quality_flags));
	add_u64(entry->casting_aborts, 1U, slot.quality_flags);
	result.outcome = telemetry_combat_summary_outcome::accepted;
	result.quality_flags = slot.quality_flags;
	return result;
}

telemetry_combat_summary_update telemetry_combat_summary_close(
	telemetry_combat_summary_state *state, telemetry_encounter_id encounter,
	telemetry_encounter_outcome outcome, telemetry_monotonic_usec at_monotonic_usec,
	telemetry_utc_usec at_utc_usec, telemetry_combat_summary_sink sink,
	void *sink_context) noexcept
{
	telemetry_combat_summary_update result = empty_update(encounter);
	if (state == nullptr || !telemetry_encounter_id_is_valid(encounter))
		return result;
	const int index = find_slot(*state, encounter);
	if (index < 0)
	{
		result.outcome = telemetry_combat_summary_outcome::not_found;
		return result;
	}
	return close_slot(state->slots[static_cast<std::size_t>(index)], outcome, at_monotonic_usec,
			  at_utc_usec, sink, sink_context);
}

telemetry_combat_summary_update telemetry_combat_summary_close_all(
	telemetry_combat_summary_state *state, telemetry_encounter_outcome outcome,
	telemetry_monotonic_usec at_monotonic_usec, telemetry_utc_usec at_utc_usec,
	telemetry_combat_summary_sink sink, void *sink_context) noexcept
{
	telemetry_combat_summary_update result{};
	result.outcome = telemetry_combat_summary_outcome::accepted;
	if (state == nullptr || !telemetry_encounter_outcome_is_valid(outcome) ||
	    outcome == telemetry_encounter_outcome::unknown)
	{
		result.outcome = telemetry_combat_summary_outcome::invalid;
		return result;
	}
	for (std::size_t index = 0U; index < TELEMETRY_COMBAT_SUMMARY_MAX_ACTIVE; ++index)
	{
		if (!state->slots[index].occupied)
			continue;
		const auto closed = close_slot(state->slots[index], outcome, at_monotonic_usec,
					       at_utc_usec, sink, sink_context);
		merge_update(result, closed);
	}
	return result;
}
