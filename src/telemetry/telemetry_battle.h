#ifndef DURIS_TELEMETRY_BATTLE_H
#define DURIS_TELEMETRY_BATTLE_H

#include "telemetry/telemetry_combat_summary.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

/* Pure association state. Native producers must supply actual observation
 * boundaries and unique live NPC generations, rather than prototype IDs.
 * These facts are not yet a member of the persistent telemetry record union. */
inline constexpr std::uint16_t TELEMETRY_BATTLE_DEFINITION_VERSION = 1U;
inline constexpr std::uint16_t TELEMETRY_BATTLE_ACTOR_CONTEXT_VERSION = 1U;
inline constexpr std::size_t TELEMETRY_BATTLE_MAX_ACTIVE = 128U;
inline constexpr std::size_t TELEMETRY_BATTLE_MAX_ACTORS = 64U;
inline constexpr std::size_t TELEMETRY_BATTLE_TERMINAL_CACHE = 64U;
inline constexpr std::uint32_t TELEMETRY_BATTLE_MAX_MUTATION_FACTS = 4096U;
inline constexpr std::size_t TELEMETRY_BATTLE_MAX_FACTS_PER_MUTATION = 5U;
inline constexpr telemetry_id TELEMETRY_BATTLE_NPC_GENERATION_TAG = 1ULL << 63U;
inline constexpr std::uint16_t TELEMETRY_BATTLE_ROLE_COMBAT = 1U << 0U;
inline constexpr std::uint16_t TELEMETRY_BATTLE_ROLE_SUPPORT = 1U << 1U;
inline constexpr std::uint16_t TELEMETRY_BATTLE_ROLE_GROUP_PRESENCE = 1U << 2U;

struct telemetry_battle_id
{
	telemetry_producer_id producer;
	telemetry_sequence sequence;
};

struct telemetry_battle_actor_key
{
	telemetry_id id;
	telemetry_combat_actor_kind kind;
};

struct telemetry_battle_actor_context
{
	telemetry_combat_actor_ref actor;
	telemetry_encounter_id encounter;
	telemetry_session_id session;
	telemetry_dimensions dimensions;
	telemetry_id group_key;
	std::uint16_t group_revision;
	std::uint16_t context_version;
	telemetry_quality_mask quality_flags;
};

enum class telemetry_battle_relation : std::uint8_t
{
	hostile = 1,
	support = 2,
	group_presence = 3,
};

enum class telemetry_battle_side_status : std::uint8_t
{
	qualified_observed_graph = 1,
	ambiguous = 2,
	partial = 3,
};

enum class telemetry_battle_fact_kind : std::uint8_t
{
	start = 1,
	actor_context = 2,
	relation = 3,
	cut = 4,
	merge_alias = 5,
	actor_summary = 6,
	close = 7,
};

enum class telemetry_battle_close_reason : std::uint8_t
{
	unknown = 0,
	inactivity = 1,
	copyover = 2,
	shutdown = 3,
};

/* Per-actor presence is distinct from input-derived activity, unique owner
 * characters, controllers and canonical reward-credit membership. */
struct telemetry_battle_effort
{
	telemetry_duration_usec present_usec;
	telemetry_duration_usec contributor_usec;
	telemetry_duration_usec pve_usec;
	telemetry_duration_usec pvp_usec;
	telemetry_duration_usec mixed_usec;
	telemetry_duration_usec unknown_mode_usec;
	telemetry_duration_usec outnumbered_owner_usec;
	telemetry_duration_usec unknown_side_usec;
};

struct telemetry_battle_fact
{
	telemetry_battle_id battle;
	telemetry_battle_id related_battle;
	telemetry_encounter_source scope;
	telemetry_revision revision;
	std::uint32_t fact_sequence;
	std::uint16_t fact_index;
	std::uint16_t fact_count;
	std::uint16_t definition_version;
	telemetry_battle_fact_kind kind;
	telemetry_battle_relation relation;
	telemetry_battle_side_status side_status;
	telemetry_encounter_mode mode;
	telemetry_battle_close_reason close_reason;
	std::uint8_t active;
	std::uint8_t side;
	std::uint8_t end_censored;
	std::uint16_t roles;
	std::uint16_t actor_count;
	std::uint16_t active_actor_count;
	std::uint16_t observed_owner_count;
	std::uint32_t dropped_actor_count;
	telemetry_battle_actor_context actor;
	telemetry_battle_actor_key related_actor;
	telemetry_battle_effort effort;
	telemetry_monotonic_usec start_monotonic_usec;
	telemetry_monotonic_usec at_monotonic_usec;
	telemetry_monotonic_usec observed_through_monotonic_usec;
	telemetry_monotonic_usec last_engagement_monotonic_usec;
	telemetry_duration_usec inactivity_grace_usec;
	telemetry_utc_usec at_utc_usec;
	telemetry_utc_usec observed_through_utc_usec;
	telemetry_quality_mask quality_flags;
};

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
	telemetry_battle_slot slots[TELEMETRY_BATTLE_MAX_ACTIVE];
	telemetry_battle_terminal terminal[TELEMETRY_BATTLE_TERMINAL_CACHE];
};

bool telemetry_battle_state_init(telemetry_battle_state *, telemetry_producer_id,
				 telemetry_encounter_source, telemetry_duration_usec) noexcept;
bool telemetry_battle_actor_context_is_valid(const telemetry_battle_actor_context &) noexcept;

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
