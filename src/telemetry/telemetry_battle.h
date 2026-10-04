#ifndef DURIS_TELEMETRY_BATTLE_H
#define DURIS_TELEMETRY_BATTLE_H

#include "telemetry/telemetry_types.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

/* Pure association state. Native producers must supply actual observation
 * boundaries and unique live NPC generations, rather than prototype IDs.
 * Persistence uses the separate battle record family, never an encounter cast. */
inline constexpr std::size_t TELEMETRY_BATTLE_MAX_ACTIVE = 128U;
inline constexpr std::size_t TELEMETRY_BATTLE_TERMINAL_CACHE = 64U;

using telemetry_battle_sink = bool (*)(void *, const telemetry_battle_fact &) noexcept;

enum class telemetry_battle_outcome : std::uint8_t
{
	accepted = 0,
	idempotent = 1,
	not_found = 2,
	invalid = 3,
	capacity_full = 4,
	sink_rejected = 5,
	duplicate_conflict = 6,
};

struct telemetry_battle_update
{
	telemetry_battle_outcome outcome;
	telemetry_battle_id battle;
	telemetry_battle_id related_battle;
	std::uint16_t facts_attempted;
	std::uint16_t facts_accepted;
	telemetry_quality_mask quality_flags;
};

struct telemetry_battle_actor_state
{
	telemetry_battle_actor_context context;
	telemetry_battle_effort effort;
	std::uint64_t hostile;
	std::uint64_t friendly;
	std::uint64_t group_presence;
	std::uint16_t roles;
	std::uint8_t active;
	std::uint8_t side;
};

struct telemetry_battle_slot
{
	telemetry_battle_id id;
	telemetry_revision revision;
	std::uint32_t fact_sequence;
	std::uint32_t mutation_facts;
	std::uint32_t dropped_actor_count;
	std::uint16_t actor_count;
	std::uint8_t occupied;
	std::uint8_t overflow_announced;
	std::uint8_t suspended;
	telemetry_battle_side_status side_status;
	telemetry_encounter_mode mode;
	telemetry_monotonic_usec start_usec;
	telemetry_monotonic_usec cut_usec;
	telemetry_monotonic_usec last_engagement_usec;
	telemetry_monotonic_usec last_observation_usec;
	telemetry_utc_usec last_observation_utc_usec;
	telemetry_quality_mask quality_flags;
	telemetry_battle_actor_state actors[TELEMETRY_BATTLE_MAX_ACTORS];
};

struct telemetry_battle_terminal
{
	telemetry_battle_id id;
	telemetry_battle_close_reason reason;
};

struct telemetry_battle_state
{
	telemetry_producer_id producer;
	telemetry_encounter_source scope;
	telemetry_duration_usec inactivity_grace_usec;
	telemetry_sequence next_sequence;
	telemetry_monotonic_usec latest_monotonic_usec;
	std::uint16_t terminal_next;
	std::uint8_t initialized;
	std::uint8_t suspended;
	telemetry_battle_slot slots[TELEMETRY_BATTLE_MAX_ACTIVE];
	telemetry_battle_terminal terminal[TELEMETRY_BATTLE_TERMINAL_CACHE];
};

bool telemetry_battle_state_init(telemetry_battle_state *, telemetry_producer_id,
				 telemetry_encounter_source, telemetry_duration_usec) noexcept;

/* Configuration boundaries retain battle IDs, rosters and cumulative effort.
 * Checkpoints carry the context effective at the cut; effort accrued before
 * that cut belongs to the preceding context. A same-scope retry emits nothing.
 * Environment/season changes require the owning lifecycle boundary instead. */
telemetry_battle_update telemetry_battle_reconfigure(telemetry_battle_state *,
						     telemetry_encounter_source,
						     telemetry_monotonic_usec, telemetry_utc_usec,
						     telemetry_battle_sink, void *) noexcept;
/* Call once when effective capture context becomes unavailable. Preserve the
 * known prefix, mark the subsequent source gap and measure unknown mode until
 * a qualified configuration resumes. This does not refresh hostile activity. */
telemetry_battle_update telemetry_battle_suspend(telemetry_battle_state *, telemetry_monotonic_usec,
						 telemetry_utc_usec, telemetry_battle_sink,
						 void *) noexcept;

telemetry_battle_update telemetry_battle_observe(telemetry_battle_state *,
						 telemetry_battle_relation,
						 const telemetry_battle_actor_context &,
						 const telemetry_battle_actor_context &,
						 telemetry_monotonic_usec, telemetry_utc_usec,
						 telemetry_battle_sink, void *) noexcept;
telemetry_battle_update telemetry_battle_context(telemetry_battle_state *,
						 const telemetry_battle_actor_context &,
						 telemetry_monotonic_usec, telemetry_utc_usec,
						 telemetry_battle_sink, void *) noexcept;
telemetry_battle_update telemetry_battle_leave(telemetry_battle_state *, telemetry_battle_actor_key,
					       telemetry_monotonic_usec, telemetry_utc_usec,
					       telemetry_battle_sink, void *) noexcept;
telemetry_battle_update telemetry_battle_close(telemetry_battle_state *, telemetry_battle_id,
					       telemetry_battle_close_reason,
					       telemetry_monotonic_usec, telemetry_utc_usec,
					       telemetry_battle_sink, void *) noexcept;
telemetry_battle_update telemetry_battle_expire(telemetry_battle_state *, telemetry_monotonic_usec,
						telemetry_utc_usec, telemetry_battle_sink,
						void *) noexcept;
telemetry_battle_update telemetry_battle_close_all(telemetry_battle_state *,
						   telemetry_battle_close_reason,
						   telemetry_monotonic_usec, telemetry_utc_usec,
						   telemetry_battle_sink, void *) noexcept;

static_assert(TELEMETRY_BATTLE_MAX_ACTORS <= 64U);
static_assert(std::is_trivially_copyable_v<telemetry_battle_state>);
static_assert(sizeof(telemetry_battle_state) <= 2U * 1024U * 1024U);
static_assert(std::is_trivially_copyable_v<telemetry_battle_fact>);
static_assert(std::is_standard_layout_v<telemetry_battle_fact>);
static_assert(sizeof(telemetry_battle_fact) + sizeof(telemetry_record_header) <=
	      TELEMETRY_RECORD_MAX_BYTES);

#endif
