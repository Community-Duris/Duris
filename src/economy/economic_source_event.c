#include "economy/economic_source_event.h"
#include "economy/economic_accounting_types.h"

#include <algorithm>
#include <bit>
#include <type_traits>

namespace
{
struct reader
{
	std::span<const uint8_t> bytes;
	size_t offset = 0;
	bool good = true;
	std::span<const uint8_t> take(size_t count)
	{
		if (!good || offset > bytes.size() || count > bytes.size() - offset)
		{
			good = false;
			return {};
		}
		const auto result = bytes.subspan(offset, count);
		offset += count;
		return result;
	}
	template <typename T> T integer()
	{
		const auto input = take(sizeof(T));
		uint64_t value = 0;
		for (size_t i = 0; i < input.size(); ++i)
			value |= static_cast<uint64_t>(input[i]) << (8 * i);
		if constexpr (std::is_signed_v<T>)
			return std::bit_cast<T>(static_cast<std::make_unsigned_t<T>>(value));
		else
			return static_cast<T>(value);
	}
	template <size_t N> void block(std::array<uint8_t, N> &output)
	{
		const auto input = take(N);
		if (good)
			std::copy(input.begin(), input.end(), output.begin());
	}
	critical_operation_id id()
	{
		critical_operation_id result = {};
		block(result.bytes);
		return result;
	}
};
} // namespace

bool economic_source_event_valid(const economic_source_event &event)
{
	return event.kind >= economic_source_kind::quest_completion &&
	       event.kind <= economic_source_kind::loot &&
	       !critical_operation_id_is_zero(event.source) &&
	       !critical_operation_id_is_zero(event.generation);
}

size_t economic_source_event_decode_object_bytes() noexcept
{
	// reader::id/block/take can keep both local and returned identity/span
	// objects alive with the decoder's input and result. No wire allocation.
	return sizeof(reader) + sizeof(economic_source_event) +
	       2 * sizeof(critical_operation_id) + 2 * sizeof(std::span<const uint8_t>);
}

economic_accounting_error
economic_source_event_encode(const economic_source_event &event,
			     std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> *encoded)
{
	if (!encoded || !economic_source_event_valid(event))
		return economic_accounting_error::invalid_identity;
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> result = {};
	const auto kind = static_cast<uint16_t>(event.kind);
	result[0] = static_cast<uint8_t>(kind);
	result[1] = static_cast<uint8_t>(kind >> 8);
	result[2] = static_cast<uint8_t>(ECONOMIC_ACCOUNTING_VERSION);
	result[3] = static_cast<uint8_t>(ECONOMIC_ACCOUNTING_VERSION >> 8);
	std::copy(event.source.bytes.begin(), event.source.bytes.end(), result.begin() + 4);
	std::copy(event.generation.bytes.begin(), event.generation.bytes.end(),
		  result.begin() + 20);
	for (size_t byte = 0; byte < 8; ++byte)
		result[36 + byte] = static_cast<uint8_t>(event.sequence >> (byte * 8));
	for (size_t byte = 0; byte < 4; ++byte)
		result[44 + byte] = static_cast<uint8_t>(event.slot >> (byte * 8));
	*encoded = result;
	return economic_accounting_error::ok;
}

economic_accounting_error economic_source_event_decode(std::span<const uint8_t> encoded,
						       economic_source_event *event)
{
	if (!event || encoded.size() != ECONOMIC_SOURCE_EVENT_BYTES)
		return economic_accounting_error::corrupt_evidence;
	reader input{ encoded };
	economic_source_event result = {};
	result.kind = static_cast<economic_source_kind>(input.integer<uint16_t>());
	if (input.integer<uint16_t>() != ECONOMIC_ACCOUNTING_VERSION)
		return economic_accounting_error::invalid_version;
	result.source = input.id();
	result.generation = input.id();
	result.sequence = input.integer<uint64_t>();
	result.slot = input.integer<uint32_t>();
	if (!input.good || !economic_source_event_valid(result))
		return economic_accounting_error::invalid_identity;
	*event = result;
	return economic_accounting_error::ok;
}
