#ifndef DURIS_TELEMETRY_COMBAT_SUMMARY_H
#define DURIS_TELEMETRY_COMBAT_SUMMARY_H

#include "telemetry/telemetry_types.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

/* Fixed budgets are part of the producer contract, not tuning knobs hidden in
 * a gameplay hook.  A close emits at most one row per retained actor. */
inline constexpr std::size_t TELEMETRY_COMBAT_SUMMARY_MAX_ACTIVE = 128U;
inline constexpr std::size_t TELEMETRY_COMBAT_SUMMARY_MAX_ACTORS =
	TELEMETRY_COMBAT_SUMMARY_MAX_PARTICIPANTS;

struct telemetry_combat_actor_ref
{
	telemetry_id actor_id;
	telemetry_pid actor_pid;
	telemetry_subject_id owner_subject_id;
	telemetry_combat_actor_kind kind;
	std::uint8_t reserved[3];
	std::uint16_t power_band;
};

constexpr bool telemetry_combat_actor_ref_is_valid(const telemetry_combat_actor_ref &actor) noexcept
{
	if (!telemetry_combat_actor_kind_is_valid(actor.kind) || actor.actor_id == 0U ||
	    actor.reserved[0] != 0U || actor.reserved[1] != 0U || actor.reserved[2] != 0U)
		return false;
	if (actor.kind == telemetry_combat_actor_kind::player)
		return actor.actor_pid > 0 && actor.owner_subject_id == actor.actor_id;
	if (actor.kind == telemetry_combat_actor_kind::pet)
		return actor.actor_pid == TELEMETRY_UNKNOWN_PID && actor.owner_subject_id != 0U;
	return actor.actor_pid == TELEMETRY_UNKNOWN_PID && actor.owner_subject_id == 0U;
}

enum class telemetry_combat_summary_outcome : std::uint8_t
{
	accepted = 0,
	idempotent = 1,
	not_found = 2,
	capacity_full = 3,
	invalid = 4,
	sink_rejected = 5,
};

struct telemetry_combat_summary_update
{
	telemetry_combat_summary_outcome outcome;
	std::uint16_t rows_attempted;
	std::uint16_t rows_accepted;
	std::uint16_t rows_dropped;
	telemetry_quality_mask quality_flags;
	telemetry_encounter_id encounter;
};

using telemetry_combat_summary_sink =
	bool (*)(void *context, const telemetry_combat_summary_payload &summary) noexcept;

struct telemetry_combat_summary_participant_state
{
	std::uint8_t occupied;
	std::uint8_t active;
	std::uint8_t casting_active;
	std::uint8_t tanking_active;
	std::uint16_t reserved;
	telemetry_combat_actor_ref actor;
	std::uint32_t modifier_flags;
	std::uint16_t opponent_power_band;
	std::uint16_t opponent_count;
	telemetry_id tanking_target_id;
	telemetry_monotonic_usec cast_start_usec;
	telemetry_monotonic_usec tanking_start_usec;
	telemetry_duration_usec damage_dealt;
	telemetry_duration_usec damage_taken;
	telemetry_duration_usec healing_attempted;
	telemetry_duration_usec effective_healing;
	telemetry_duration_usec overhealing;
	std::uint64_t control_applications;
	std::uint64_t casting_attempts;
	std::uint64_t casting_completions;
	std::uint64_t casting_aborts;
	telemetry_duration_usec casting_elapsed_usec;
	telemetry_duration_usec tanking_usec;
};

struct telemetry_combat_summary_slot
{
	std::uint8_t occupied;
	std::uint8_t reserved;
	std::uint16_t participant_count;
	std::uint16_t dropped_participant_count;
	std::uint16_t revision;
	std::uint16_t unique_player_count;
	telemetry_encounter_id encounter;
	telemetry_encounter_source source;
	telemetry_encounter_mode mode;
	telemetry_encounter_outcome outcome;
	std::uint8_t reserved2[2];
	telemetry_monotonic_usec start_monotonic_usec;
	telemetry_utc_usec start_utc_usec;
	telemetry_quality_mask quality_flags;
	telemetry_subject_id unique_players[TELEMETRY_COMBAT_SUMMARY_MAX_UNIQUE_PLAYERS];
	telemetry_combat_summary_participant_state participants[TELEMETRY_COMBAT_SUMMARY_MAX_ACTORS];
};

struct telemetry_combat_summary_state
{
	telemetry_combat_summary_slot slots[TELEMETRY_COMBAT_SUMMARY_MAX_ACTIVE];
};

void telemetry_combat_summary_state_init(telemetry_combat_summary_state *state) noexcept;

telemetry_combat_summary_update telemetry_combat_summary_begin(
	telemetry_combat_summary_state *state, telemetry_encounter_id encounter,
	telemetry_encounter_source source, telemetry_encounter_mode mode,
	telemetry_monotonic_usec at_monotonic_usec, telemetry_utc_usec at_utc_usec) noexcept;

telemetry_combat_summary_update telemetry_combat_summary_add_actor(
	telemetry_combat_summary_state *state, telemetry_encounter_id encounter,
	telemetry_combat_actor_ref actor, telemetry_monotonic_usec at_monotonic_usec) noexcept;

telemetry_combat_summary_update telemetry_combat_summary_leave_actor(
	telemetry_combat_summary_state *state, telemetry_encounter_id encounter,
	telemetry_combat_actor_ref actor, telemetry_monotonic_usec at_monotonic_usec) noexcept;

telemetry_combat_summary_update telemetry_combat_summary_record_damage(
	telemetry_combat_summary_state *state, telemetry_combat_actor_ref source,
	telemetry_combat_actor_ref target, std::uint64_t amount,
	telemetry_monotonic_usec at_monotonic_usec, std::uint32_t modifier_flags) noexcept;

telemetry_combat_summary_update telemetry_combat_summary_record_healing(
	telemetry_combat_summary_state *state, telemetry_combat_actor_ref healer,
	telemetry_combat_actor_ref target, std::uint64_t attempted, std::uint64_t effective,
	telemetry_monotonic_usec at_monotonic_usec, std::uint32_t modifier_flags) noexcept;

telemetry_combat_summary_update telemetry_combat_summary_record_control(
	telemetry_combat_summary_state *state, telemetry_combat_actor_ref source,
	telemetry_combat_actor_ref target, std::uint16_t applications,
	telemetry_monotonic_usec at_monotonic_usec, std::uint32_t modifier_flags) noexcept;

telemetry_combat_summary_update telemetry_combat_summary_record_tanking(
	telemetry_combat_summary_state *state, telemetry_combat_actor_ref actor,
	const telemetry_combat_actor_ref *target, telemetry_monotonic_usec at_monotonic_usec,
	std::uint32_t modifier_flags) noexcept;

telemetry_combat_summary_update telemetry_combat_summary_cast_attempt(
	telemetry_combat_summary_state *state, telemetry_combat_actor_ref actor, int spell,
	telemetry_monotonic_usec at_monotonic_usec, std::uint32_t modifier_flags) noexcept;

telemetry_combat_summary_update
telemetry_combat_summary_cast_complete(telemetry_combat_summary_state *state,
				       telemetry_combat_actor_ref actor,
				       telemetry_monotonic_usec at_monotonic_usec) noexcept;

telemetry_combat_summary_update
telemetry_combat_summary_cast_abort(telemetry_combat_summary_state *state,
				    telemetry_combat_actor_ref actor,
				    telemetry_monotonic_usec at_monotonic_usec) noexcept;

telemetry_combat_summary_update telemetry_combat_summary_close(
	telemetry_combat_summary_state *state, telemetry_encounter_id encounter,
	telemetry_encounter_outcome outcome, telemetry_monotonic_usec at_monotonic_usec,
	telemetry_utc_usec at_utc_usec, telemetry_combat_summary_sink sink,
	void *sink_context) noexcept;

telemetry_combat_summary_update telemetry_combat_summary_close_all(
	telemetry_combat_summary_state *state, telemetry_encounter_outcome outcome,
	telemetry_monotonic_usec at_monotonic_usec, telemetry_utc_usec at_utc_usec,
	telemetry_combat_summary_sink sink, void *sink_context) noexcept;

static_assert(std::is_trivially_copyable_v<telemetry_combat_actor_ref>);
static_assert(std::is_standard_layout_v<telemetry_combat_actor_ref>);
static_assert(std::is_trivially_copyable_v<telemetry_combat_summary_state>);

#endif
