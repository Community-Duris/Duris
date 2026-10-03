#ifndef DURIS_TELEMETRY_BATTLE_CONTRIBUTION_H
#define DURIS_TELEMETRY_BATTLE_CONTRIBUTION_H

#include "telemetry/telemetry_types.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

inline constexpr std::size_t TELEMETRY_BATTLE_CONTRIBUTION_MAX_BATTLES = 128U;
inline constexpr std::size_t TELEMETRY_BATTLE_CONTRIBUTION_MAX_ACTORS = 64U;
inline constexpr std::uint16_t TELEMETRY_BATTLE_CONTRIBUTION_VERSION = 1U;
inline constexpr std::uint32_t TELEMETRY_BC_DAMAGE = 1U;
inline constexpr std::uint32_t TELEMETRY_BC_HEALING = 2U;
inline constexpr std::uint32_t TELEMETRY_BC_CONTROL = 4U;
inline constexpr std::uint32_t TELEMETRY_BC_CASTING = 8U;
inline constexpr std::uint32_t TELEMETRY_BC_ENGAGEMENT = 16U;
inline constexpr std::uint32_t TELEMETRY_BC_METRICS = 31U;
inline constexpr std::size_t TELEMETRY_BATTLE_CONTRIBUTION_WIRE_BYTES = 0U
#define TELEMETRY_BC_FIELD(name, member, width, signed_value) +width
#include "telemetry/telemetry_battle_contribution_fields.inc"
#undef TELEMETRY_BC_FIELD
	;

/* Exact association reference and native context, supplied by the owning
 * collector. Availability names reviewed producer families, not complete
 * historical coverage. Missing families have unknown, not measured-zero, totals. */
struct telemetry_battle_contribution_context
{
	telemetry_battle_id battle;
	telemetry_encounter_source scope;
	telemetry_battle_actor_context actor;
	telemetry_revision association_revision;
	std::uint32_t association_fact_sequence;
	std::uint32_t available_metrics;
	telemetry_battle_side_status side_status;
	telemetry_encounter_mode mode;
	std::uint8_t side;
	std::uint8_t reserved;
	telemetry_quality_mask quality_flags;
};

struct telemetry_battle_contribution_counters
{
	std::uint64_t damage_dealt;
	std::uint64_t damage_taken;
	std::uint64_t healing_attempted;
	std::uint64_t effective_healing;
	std::uint64_t overhealing;
	std::uint64_t healing_received;
	std::uint64_t control_applications;
	std::uint64_t control_received;
	std::uint64_t casting_attempts;
	std::uint64_t casting_completions;
	std::uint64_t casting_aborts;
	std::uint64_t casting_unresolved;
	telemetry_duration_usec casting_elapsed_usec;
	/* Observed opponent-link duration; it does not establish incoming pressure
	 * or prevention and must not be relabeled as tanking. */
	telemetry_duration_usec engaged_target_usec;
};

enum class telemetry_battle_contribution_end : std::uint8_t
{
	context_changed = 1,
	actor_left = 2,
	battle_ended = 3,
	source_gap = 4,
};

struct telemetry_battle_contribution_cut
{
	telemetry_monotonic_usec observed_usec;
	telemetry_utc_usec observed_utc_usec;
	telemetry_monotonic_usec decision_usec;
	telemetry_utc_usec decision_utc_usec;
};

/* One sealed disjoint segment, emitted once. Its process-wide segment sequence
 * is independent of battle/actor identity and the later transport receipt.
 * A battle-ID/context change seals the old totals; no cumulative amount is
 * copied into the new segment. Alias resolution is the publisher's job. */
struct telemetry_battle_contribution_payload
{
	telemetry_battle_contribution_context context;
	telemetry_sequence sequence;
	telemetry_revision last_association_revision;
	std::uint32_t last_association_fact_sequence;
	std::uint32_t modifier_flags;
	telemetry_monotonic_usec start_usec;
	telemetry_utc_usec start_utc_usec;
	telemetry_battle_contribution_cut cut;
	telemetry_battle_contribution_counters counters;
	telemetry_quality_mask quality_flags;
	std::uint16_t definition_version;
	telemetry_battle_contribution_end end_reason;
	std::uint8_t reserved;
};

using telemetry_battle_contribution_sink =
	bool (*)(void *, const telemetry_battle_contribution_payload &) noexcept;

enum class telemetry_battle_contribution_outcome : std::uint8_t
{
	accepted = 0,
	idempotent = 1,
	not_found = 2,
	invalid = 3,
	capacity_full = 4,
	sink_rejected = 5,
};

struct telemetry_battle_contribution_update
{
	telemetry_battle_contribution_outcome outcome;
	std::uint16_t rows_attempted;
	std::uint16_t rows_accepted;
	telemetry_quality_mask quality_flags;
};

struct telemetry_battle_contribution_actor_state
{
	telemetry_battle_contribution_payload value;
	telemetry_monotonic_usec last_observed_usec;
	telemetry_monotonic_usec casting_start_usec;
	telemetry_monotonic_usec engagement_start_usec;
	telemetry_battle_actor_key engagement_target;
	std::uint8_t occupied;
	std::uint8_t casting;
	std::uint8_t engaged;
};
struct telemetry_battle_contribution_slot
{
	telemetry_battle_id battle;
	telemetry_quality_mask quality_flags;
	std::uint16_t actor_count;
	std::uint8_t occupied;
	telemetry_battle_contribution_actor_state actors[TELEMETRY_BATTLE_CONTRIBUTION_MAX_ACTORS];
};
struct telemetry_battle_contribution_state
{
	telemetry_producer_id producer;
	telemetry_id environment_id;
	telemetry_id season_id;
	telemetry_sequence next_sequence;
	telemetry_monotonic_usec latest_usec;
	telemetry_utc_usec latest_utc_usec;
	telemetry_quality_mask quality_flags;
	std::uint8_t initialized;
	telemetry_battle_contribution_slot slots[TELEMETRY_BATTLE_CONTRIBUTION_MAX_BATTLES];
};

bool telemetry_battle_contribution_context_is_valid(
	const telemetry_battle_contribution_context &) noexcept;
bool telemetry_battle_contribution_payload_is_valid(
	const telemetry_battle_contribution_payload &) noexcept;
/* Domain identity is (battle producer, process-wide segment sequence), not
 * (battle ID, actor ID) and not the transport's admitted receipt. */
bool telemetry_battle_contribution_same_key(const telemetry_battle_contribution_payload &,
					    const telemetry_battle_contribution_payload &) noexcept;
bool telemetry_battle_contribution_equal(const telemetry_battle_contribution_payload &,
					 const telemetry_battle_contribution_payload &) noexcept;
/* Exact-length network order; decode clears its destination on every refusal.
 * This codec alone does not add a runtime record family or SQL projection. */
bool telemetry_battle_contribution_encode(const telemetry_battle_contribution_payload &,
					  std::uint8_t *, std::size_t) noexcept;
bool telemetry_battle_contribution_decode(const std::uint8_t *, std::size_t,
					  telemetry_battle_contribution_payload *) noexcept;
bool telemetry_battle_contribution_state_init(telemetry_battle_contribution_state *,
					      telemetry_producer_id, telemetry_id environment,
					      telemetry_id season) noexcept;
telemetry_battle_contribution_update telemetry_battle_contribution_damage(
	telemetry_battle_contribution_state *, const telemetry_battle_contribution_context &,
	const telemetry_battle_contribution_context &, std::uint64_t amount,
	telemetry_monotonic_usec, telemetry_utc_usec, std::uint32_t modifiers,
	telemetry_battle_contribution_sink, void *) noexcept;
telemetry_battle_contribution_update telemetry_battle_contribution_healing(
	telemetry_battle_contribution_state *, const telemetry_battle_contribution_context &,
	const telemetry_battle_contribution_context &, std::uint64_t attempted,
	std::uint64_t effective, telemetry_monotonic_usec, telemetry_utc_usec,
	std::uint32_t modifiers, telemetry_battle_contribution_sink, void *) noexcept;
telemetry_battle_contribution_update telemetry_battle_contribution_control(
	telemetry_battle_contribution_state *, const telemetry_battle_contribution_context &,
	const telemetry_battle_contribution_context &, std::uint16_t applications,
	telemetry_monotonic_usec, telemetry_utc_usec, std::uint32_t modifiers,
	telemetry_battle_contribution_sink, void *) noexcept;
telemetry_battle_contribution_update
telemetry_battle_contribution_cast_attempt(telemetry_battle_contribution_state *,
					   const telemetry_battle_contribution_context &,
					   telemetry_monotonic_usec, telemetry_utc_usec,
					   telemetry_battle_contribution_sink, void *) noexcept;
telemetry_battle_contribution_update telemetry_battle_contribution_cast_finish(
	telemetry_battle_contribution_state *, const telemetry_battle_contribution_context &,
	bool completed, telemetry_monotonic_usec, telemetry_utc_usec,
	telemetry_battle_contribution_sink, void *) noexcept;
telemetry_battle_contribution_update telemetry_battle_contribution_engagement(
	telemetry_battle_contribution_state *, const telemetry_battle_contribution_context &,
	const telemetry_battle_actor_key *target, telemetry_monotonic_usec, telemetry_utc_usec,
	telemetry_battle_contribution_sink, void *) noexcept;
/* Context/leave never creates a metric stream. Close accepts a later decision
 * clock but measures only the supplied actual observed prefix. */
telemetry_battle_contribution_update
telemetry_battle_contribution_context_changed(telemetry_battle_contribution_state *,
					      const telemetry_battle_contribution_context &,
					      telemetry_monotonic_usec, telemetry_utc_usec,
					      telemetry_battle_contribution_sink, void *) noexcept;
telemetry_battle_contribution_update
telemetry_battle_contribution_leave(telemetry_battle_contribution_state *, telemetry_battle_id,
				    telemetry_battle_actor_key, telemetry_battle_contribution_cut,
				    telemetry_battle_contribution_end,
				    telemetry_battle_contribution_sink, void *) noexcept;
telemetry_battle_contribution_update
telemetry_battle_contribution_close(telemetry_battle_contribution_state *, telemetry_battle_id,
				    telemetry_battle_contribution_cut,
				    telemetry_battle_contribution_end,
				    telemetry_battle_contribution_sink, void *) noexcept;

static_assert(std::is_trivially_copyable_v<telemetry_battle_contribution_state>);
static_assert(std::is_trivially_copyable_v<telemetry_battle_contribution_payload>);
static_assert(std::is_standard_layout_v<telemetry_battle_contribution_payload>);
static_assert(sizeof(telemetry_battle_contribution_payload) + sizeof(telemetry_record_header) <=
	      TELEMETRY_RECORD_MAX_BYTES);
static_assert(sizeof(telemetry_battle_contribution_state) <= 4U * 1024U * 1024U);
static_assert(TELEMETRY_BATTLE_CONTRIBUTION_WIRE_BYTES + sizeof(telemetry_record_header) <=
	      TELEMETRY_RECORD_MAX_BYTES);
#endif
