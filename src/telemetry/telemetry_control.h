#ifndef DURIS_TELEMETRY_CONTROL_H
#define DURIS_TELEMETRY_CONTROL_H

#include "telemetry/telemetry_types.h"

#include <cstddef>
#include <cstdint>

inline constexpr std::size_t TELEMETRY_CONTROL_WIRE_BYTES = 0U
#define TELEMETRY_CONTROL_FIELD(name, member, width, signed_value) +width
#include "telemetry/telemetry_control_fields.inc"
#undef TELEMETRY_CONTROL_FIELD
	;

/* Intrinsic value checks establish neither complete source coverage nor a
 * verified association, identity or duration chain. The consumer must qualify
 * those independently before publishing comparable denominators. */
bool telemetry_control_observation_same_key(const telemetry_control_observation &,
					    const telemetry_control_observation &) noexcept;
bool telemetry_control_observation_equal(const telemetry_control_observation &,
					 const telemetry_control_observation &) noexcept;
bool telemetry_control_observation_encode(const telemetry_control_observation &, std::uint8_t *,
					  std::size_t) noexcept;
bool telemetry_control_observation_decode(const std::uint8_t *, std::size_t,
					  telemetry_control_observation *) noexcept;

inline constexpr std::size_t TELEMETRY_CONTROL_MAX_TARGETS = 512U;
using telemetry_control_sink = bool (*)(void *, const telemetry_control_observation &) noexcept;

enum class telemetry_control_outcome : std::uint8_t
{
	accepted = 0,
	idempotent = 1,
	not_found = 2,
	invalid = 3,
	capacity_full = 4,
	sequence_exhausted = 5,
	sink_rejected = 6,
};

struct telemetry_control_update
{
	telemetry_control_outcome outcome;
	std::uint16_t rows_attempted;
	std::uint16_t rows_accepted;
	telemetry_quality_mask quality_flags;
};

struct telemetry_control_target_state
{
	telemetry_control_observation value;
	std::uint8_t occupied;
	std::uint8_t delivery_gap;
};

struct telemetry_control_state
{
	telemetry_producer_id producer;
	telemetry_environment_id environment_id;
	telemetry_season_id season_id;
	telemetry_sequence next_sequence;
	telemetry_monotonic_usec latest_decision_usec;
	std::uint8_t initialized;
	telemetry_control_target_state targets[TELEMETRY_CONTROL_MAX_TARGETS];
};

bool telemetry_control_state_init(telemetry_control_state *, telemetry_producer_id,
				  telemetry_environment_id, telemetry_season_id) noexcept;
/* Inputs have sequence/previous_state_sequence cleared. Resolutions produce one
 * independent operation. State entries supply an actual selected-state read;
 * equal values coalesce, changes seal disjoint target intervals. No caller
 * pointer or duration declaration is retained as a live game object. */
telemetry_control_update telemetry_control_observe(telemetry_control_state *,
						   const telemetry_control_observation &,
						   telemetry_control_sink, void *) noexcept;
/* A later close decision measures at most the target's last observed prefix,
 * never a grace tail. Unknown/lost state requires a fresh admitted entry. */
telemetry_control_update telemetry_control_close(telemetry_control_state *,
						 telemetry_battle_actor_key,
						 telemetry_monotonic_usec, telemetry_utc_usec,
						 telemetry_control_boundary, telemetry_control_sink,
						 void *) noexcept;

static_assert(TELEMETRY_CONTROL_WIRE_BYTES + sizeof(telemetry_record_header) <=
	      TELEMETRY_RECORD_MAX_BYTES);
static_assert(std::is_trivially_copyable_v<telemetry_control_state>);
static_assert(sizeof(telemetry_control_state) <= 256U * 1024U);

#endif
