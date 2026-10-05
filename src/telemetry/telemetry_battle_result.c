#include "telemetry/telemetry_battle_result.h"

#include <algorithm>
#include <bit>
#include <cstring>
#include <type_traits>

namespace
{
using observation = telemetry_battle_result_observation;
template <std::size_t Width, bool Signed, typename T>
void encode_value(T value, std::uint8_t *&bytes) noexcept
{
	static_assert(sizeof(T) == Width);
	static_assert(std::is_signed_v<T> == Signed);
	const auto number = static_cast<std::uint64_t>(value);
	for (std::size_t index = 0U; index < Width; ++index)
		*bytes++ = static_cast<std::uint8_t>(number >> ((Width - 1U - index) * 8U));
}

template <std::size_t Width, bool Signed, typename T>
void decode_value(T &value, const std::uint8_t *&bytes) noexcept
{
	static_assert(sizeof(T) == Width);
	static_assert(std::is_signed_v<T> == Signed);
	std::uint64_t number = 0U;
	for (std::size_t index = 0U; index < Width; ++index)
		number = (number << 8U) | *bytes++;
	if constexpr (Signed)
		value = std::bit_cast<T>(static_cast<std::make_unsigned_t<T>>(number));
	else
		value = static_cast<T>(number);
}
} // namespace

bool telemetry_battle_result_observation_same_key(const observation &a,
						  const observation &b) noexcept
{
	return a.sequence && a.sequence == b.sequence &&
	       telemetry_producer_id_is_valid(a.producer) &&
	       a.producer.boot_id == b.producer.boot_id &&
	       a.producer.process_id == b.producer.process_id;
}

bool telemetry_battle_result_observation_equal(const observation &a, const observation &b) noexcept
{
	if (!telemetry_battle_result_observation_is_valid(a) ||
	    !telemetry_battle_result_observation_is_valid(b))
		return false;
#define TELEMETRY_RESULT_FIELD(name, member, width, signed_value) \
	if (a.member != b.member)                                 \
		return false;
#define TELEMETRY_RESULT_BYTES(name, member, width)                                      \
	if (!std::equal(std::begin(a.member), std::end(a.member), std::begin(b.member))) \
		return false;
#include "telemetry/telemetry_battle_result_fields.inc"
#undef TELEMETRY_RESULT_FIELD
#undef TELEMETRY_RESULT_BYTES
	return true;
}

bool telemetry_battle_result_observation_encode(const observation &v, std::uint8_t *bytes,
						std::size_t size) noexcept
{
	if (!bytes || size != TELEMETRY_BATTLE_RESULT_WIRE_BYTES ||
	    !telemetry_battle_result_observation_is_valid(v))
		return false;
#define TELEMETRY_RESULT_FIELD(name, member, width, signed_value) \
	encode_value<width, signed_value>(v.member, bytes);
#define TELEMETRY_RESULT_BYTES(name, member, width) \
	std::memcpy(bytes, v.member, width);        \
	bytes += width;
#include "telemetry/telemetry_battle_result_fields.inc"
#undef TELEMETRY_RESULT_FIELD
#undef TELEMETRY_RESULT_BYTES
	return true;
}

bool telemetry_battle_result_observation_decode(const std::uint8_t *bytes, std::size_t size,
						observation *output) noexcept
{
	if (!output)
		return false;
	*output = {};
	if (!bytes || size != TELEMETRY_BATTLE_RESULT_WIRE_BYTES)
		return false;
	observation value{};
#define TELEMETRY_RESULT_FIELD(name, member, width, signed_value) \
	decode_value<width, signed_value>(value.member, bytes);
#define TELEMETRY_RESULT_BYTES(name, member, width) \
	std::memcpy(value.member, bytes, width);    \
	bytes += width;
#include "telemetry/telemetry_battle_result_fields.inc"
#undef TELEMETRY_RESULT_FIELD
#undef TELEMETRY_RESULT_BYTES
	if (!telemetry_battle_result_observation_is_valid(value))
		return false;
	*output = value;
	return true;
}
