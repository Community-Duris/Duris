#ifndef DURIS_TELEMETRY_BATTLE_BUILD_OBSERVATION_H
#define DURIS_TELEMETRY_BATTLE_BUILD_OBSERVATION_H

#include "telemetry/telemetry_battle_build_context.h"

#include <cstddef>
#include <cstdint>

inline constexpr std::size_t TELEMETRY_BATTLE_BUILD_WIRE_BYTES = 0U
#define TELEMETRY_BUILD_FIELD(name, member, width, signed_value) +width
#define TELEMETRY_BUILD_BYTES(name, member, width) +width
#include "telemetry/telemetry_battle_build_fields.inc"
#undef TELEMETRY_BUILD_FIELD
#undef TELEMETRY_BUILD_BYTES
	;

/* Point observations with exact association references. A snapshot does not
 * prove uninterrupted validity or a whole-battle outcome. Unavailable markers
 * preserve the boundary and source reason without copying the preceding build. */
bool telemetry_battle_build_observation_same_key(
	const telemetry_battle_build_observation &,
	const telemetry_battle_build_observation &) noexcept;
bool telemetry_battle_build_observation_equal(const telemetry_battle_build_observation &,
					      const telemetry_battle_build_observation &) noexcept;

/* Exact-length kind-12 network contract. These functions never admit a
 * transport record or resolve an association link. */
bool telemetry_battle_build_observation_encode(const telemetry_battle_build_observation &,
					       std::uint8_t *, std::size_t) noexcept;
bool telemetry_battle_build_observation_decode(const std::uint8_t *, std::size_t,
					       telemetry_battle_build_observation *) noexcept;

/* Select the reviewed persisted subset from one native reader result. The
 * caller supplies an observed association, one clock pair and a fresh sequence.
 * Refusal clears output; no session, battle, ownership or coverage is inferred. */
bool telemetry_battle_build_observation_from_context(const telemetry_battle_build_context &,
						     const telemetry_battle_contribution_context &,
						     telemetry_sequence, telemetry_monotonic_usec,
						     telemetry_utc_usec,
						     telemetry_battle_build_boundary,
						     telemetry_battle_build_observation *) noexcept;

static_assert(TELEMETRY_BATTLE_BUILD_WIRE_BYTES == 447U);
static_assert(TELEMETRY_BATTLE_BUILD_WIRE_BYTES + sizeof(telemetry_record_header) <=
	      TELEMETRY_RECORD_MAX_BYTES);

#endif
