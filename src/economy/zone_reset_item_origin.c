#include "economy/zone_reset_item_origin.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

namespace
{
using error = economic_accounting_error;
constexpr uint8_t MAGIC[] = { 'Z', 'R', 'O', '1' };

void put(uint8_t *out, uint64_t value, size_t count) noexcept
{
	for (size_t index = 0; index < count; ++index)
		out[index] = static_cast<uint8_t>(value >> (index * 8));
}
uint64_t get(const uint8_t *in, size_t count) noexcept
{
	uint64_t value = 0;
	for (size_t index = 0; index < count; ++index)
		value |= static_cast<uint64_t>(in[index]) << (index * 8);
	return value;
}
error correlate(const critical_command &command, std::span<const uint8_t> bytes)
{
	zone_reset_item_image image;
	const auto decoded = zone_reset_item_command_decode(command, &image);
	if (decoded != error::ok)
		return decoded;
	item_transfer_result result{};
	std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> canonical{};
	if (bytes.size() != canonical.size() ||
	    !item_transfer_command_decode_result(bytes.data(), bytes.size(), &result) ||
	    !item_transfer_command_encode_result(result, &canonical) ||
	    !std::equal(canonical.begin(), canonical.end(), bytes.begin()))
		return error::corrupt_evidence;
	if (image.items.empty() ||
	    image.expected_room_revision == std::numeric_limits<uint64_t>::max() ||
	    result.root_item_uid != image.items.front().object_uid ||
	    result.item_count != image.items.size() || result.from_owner_revision != 0 ||
	    result.to_owner_revision != image.expected_room_revision + 1 ||
	    result.max_item_revision != 1 || result.corpse_revision != 0 ||
	    result.collector_catalog_changed)
		return error::payload_conflict;
	return error::ok;
}
}

economic_accounting_error zone_reset_item_origin_encode(const critical_command &command,
							std::span<const uint8_t> result,
							std::vector<uint8_t> *output) noexcept
{
	if (!output)
		return error::invalid_identity;
	try
	{
		const auto checked = correlate(command, result);
		if (checked != error::ok)
			return checked;
		std::vector<uint8_t> encoded;
		const auto code = critical_command_encode(command, &encoded);
		if (code != critical_command_codec_result::ok)
			return code == critical_command_codec_result::overflow ?
				       error::capacity :
				       error::corrupt_evidence;
		if (encoded.empty() || encoded.size() > CRITICAL_COMMAND_MAX_ENCODED_BYTES)
			return error::capacity;
		std::vector<uint8_t> candidate(
			ZONE_RESET_ITEM_ORIGIN_HEADER_BYTES + encoded.size() + result.size(), 0);
		std::copy(std::begin(MAGIC), std::end(MAGIC), candidate.begin());
		put(candidate.data() + 4, ZONE_RESET_ITEM_ORIGIN_VERSION, 2);
		put(candidate.data() + 8, encoded.size(), 4);
		put(candidate.data() + 12, result.size(), 4);
		std::copy(encoded.begin(), encoded.end(),
			  candidate.begin() + ZONE_RESET_ITEM_ORIGIN_HEADER_BYTES);
		std::copy(result.begin(), result.end(), candidate.end() - result.size());
		*output = std::move(candidate);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
}

economic_accounting_error
zone_reset_item_origin_decode(std::span<const uint8_t> bytes,
			      zone_reset_item_retained_origin *output) noexcept
{
	if (!output)
		return error::invalid_identity;
	if (bytes.size() < ZONE_RESET_ITEM_ORIGIN_HEADER_BYTES + ITEM_TRANSFER_RESULT_BYTES ||
	    bytes.size() > ZONE_RESET_ITEM_ORIGIN_MAX_BYTES)
		return error::corrupt_evidence;
	if (std::memcmp(bytes.data(), MAGIC, sizeof(MAGIC)) ||
	    get(bytes.data() + 4, 2) != ZONE_RESET_ITEM_ORIGIN_VERSION ||
	    get(bytes.data() + 6, 2) != 0)
		return error::invalid_version;
	const auto command_size = get(bytes.data() + 8, 4);
	if (command_size < CRITICAL_COMMAND_HEADER_BYTES ||
	    command_size > CRITICAL_COMMAND_MAX_ENCODED_BYTES ||
	    get(bytes.data() + 12, 4) != ITEM_TRANSFER_RESULT_BYTES ||
	    command_size !=
		    bytes.size() - ZONE_RESET_ITEM_ORIGIN_HEADER_BYTES - ITEM_TRANSFER_RESULT_BYTES)
		return error::corrupt_evidence;
	try
	{
		zone_reset_item_retained_origin candidate;
		const auto code = critical_command_decode(
			bytes.data() + ZONE_RESET_ITEM_ORIGIN_HEADER_BYTES,
			static_cast<size_t>(command_size), &candidate.original);
		if (code != critical_command_codec_result::ok)
			return code == critical_command_codec_result::overflow ?
				       error::capacity :
				       error::corrupt_evidence;
		std::copy_n(bytes.end() - ITEM_TRANSFER_RESULT_BYTES, ITEM_TRANSFER_RESULT_BYTES,
			    candidate.result.begin());
		std::vector<uint8_t> canonical;
		const auto checked = zone_reset_item_origin_encode(candidate.original,
								   candidate.result, &canonical);
		if (checked != error::ok)
			return checked;
		if (canonical.size() != bytes.size() ||
		    !std::equal(canonical.begin(), canonical.end(), bytes.begin()))
			return error::corrupt_evidence;
		static_assert(std::is_nothrow_move_assignable_v<zone_reset_item_retained_origin>);
		*output = std::move(candidate);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
}
