#ifndef DURIS_TELEMETRY_BATTLE_RESULT_H
#define DURIS_TELEMETRY_BATTLE_RESULT_H

#include "telemetry/telemetry_types.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

inline constexpr std::size_t TELEMETRY_BATTLE_RESULT_WIRE_BYTES = 0U
#define TELEMETRY_RESULT_FIELD(name, member, width, signed_value) +width
#define TELEMETRY_RESULT_BYTES(name, member, width) +width
#include "telemetry/telemetry_battle_result_fields.inc"
#undef TELEMETRY_RESULT_FIELD
#undef TELEMETRY_RESULT_BYTES
	;

bool telemetry_battle_result_observation_same_key(
	const telemetry_battle_result_observation &,
	const telemetry_battle_result_observation &) noexcept;
bool telemetry_battle_result_observation_equal(const telemetry_battle_result_observation &,
					       const telemetry_battle_result_observation &) noexcept;
bool telemetry_battle_result_observation_encode(const telemetry_battle_result_observation &,
						std::uint8_t *, std::size_t) noexcept;
bool telemetry_battle_result_observation_decode(const std::uint8_t *, std::size_t,
						telemetry_battle_result_observation *) noexcept;

static_assert(std::is_trivially_copyable_v<telemetry_battle_result_observation>);
static_assert(std::is_standard_layout_v<telemetry_battle_result_observation>);
static_assert(sizeof(telemetry_battle_result_observation) + sizeof(telemetry_record_header) <=
	      TELEMETRY_RECORD_MAX_BYTES);
static_assert(TELEMETRY_BATTLE_RESULT_WIRE_BYTES + sizeof(telemetry_record_header) <=
	      TELEMETRY_RECORD_MAX_BYTES);

#endif
