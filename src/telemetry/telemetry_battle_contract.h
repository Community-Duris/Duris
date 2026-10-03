#ifndef DURIS_TELEMETRY_BATTLE_CONTRACT_H
#define DURIS_TELEMETRY_BATTLE_CONTRACT_H

#include "telemetry/telemetry_types.h"

#include <cstddef>
#include <cstdint>

inline constexpr std::size_t TELEMETRY_BATTLE_PACKET_MAX_FACTS = TELEMETRY_BATTLE_MAX_ACTORS + 1U;
inline constexpr std::size_t TELEMETRY_BATTLE_WIRE_BYTES = 0U
#define TELEMETRY_BATTLE_FIELD(name, member, width, signed_value) +width
#include "telemetry/telemetry_battle_fields.inc"
#undef TELEMETRY_BATTLE_FIELD
	;

/* This identity belongs to the battle domain, independently of the enclosing
 * transport's admitted replay key. It must never be recast as an encounter. */
struct telemetry_battle_fact_key
{
	telemetry_battle_id battle;
	std::uint32_t sequence;
};

bool telemetry_battle_fact_same_key(const telemetry_battle_fact &,
				    const telemetry_battle_fact &) noexcept;
bool telemetry_battle_fact_equal(const telemetry_battle_fact &,
				 const telemetry_battle_fact &) noexcept;
bool telemetry_battle_packet_is_valid(const telemetry_battle_fact *, std::size_t) noexcept;

/* Exact-length network byte order. Encode refuses invalid values; decode clears
 * the destination on every refusal. No allocation, strings, I/O or ABI padding. */
bool telemetry_battle_fact_encode(const telemetry_battle_fact &, std::uint8_t *,
				  std::size_t) noexcept;
bool telemetry_battle_fact_decode(const std::uint8_t *, std::size_t,
				  telemetry_battle_fact *) noexcept;

enum class telemetry_battle_packet_result : std::uint8_t
{
	pending = 0,
	complete = 1,
	duplicate_identical = 2,
	duplicate_conflict = 3,
	other_packet = 4,
	invalid = 5,
};

/* One bounded mutation packet. Reordering and identical replays are accepted;
 * a missing ordinal never reaches complete. Consumers retain/reset the packet
 * explicitly after publication. This is not a cross-packet coverage ledger. */
struct telemetry_battle_packet_state
{
	telemetry_battle_id battle;
	telemetry_revision revision;
	std::uint32_t first_sequence;
	std::uint16_t count;
	std::uint16_t received;
	std::uint8_t complete;
	std::uint8_t conflicted;
	std::uint64_t seen[2];
	telemetry_battle_fact facts[TELEMETRY_BATTLE_PACKET_MAX_FACTS];
};

void telemetry_battle_packet_reset(telemetry_battle_packet_state *) noexcept;
telemetry_battle_packet_result
telemetry_battle_packet_receive(telemetry_battle_packet_state *,
				const telemetry_battle_fact &) noexcept;

static_assert(TELEMETRY_BATTLE_WIRE_BYTES + sizeof(telemetry_record_header) <=
	      TELEMETRY_RECORD_MAX_BYTES);
static_assert(TELEMETRY_BATTLE_PACKET_MAX_FACTS == 65U);
static_assert(sizeof(telemetry_battle_packet_state) < 32U * 1024U);
static_assert(std::is_trivially_copyable_v<telemetry_battle_packet_state>);

#endif
