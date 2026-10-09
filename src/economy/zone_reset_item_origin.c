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

namespace
{
[[maybe_unused]] bool origin_bound_add(size_t &total, size_t amount) noexcept
{
	if (amount > SIZE_MAX - total)
		return false;
	total += amount;
	return true;
}
[[maybe_unused]] bool origin_bound_rows(size_t &total, size_t count, size_t width) noexcept
{
	return (!width || count <= SIZE_MAX / width) && origin_bound_add(total, count * width);
}
[[maybe_unused]] bool origin_bound_string(size_t &total, const std::string &value) noexcept
{
	// Supported C++11 string uses its inline 15-character buffer. Actual dynamic
	// capacity, including the NUL request, is retained after the bounded decode.
	return value.capacity() <= 15 ||
	       (value.capacity() < SIZE_MAX && origin_bound_add(total, value.capacity() + 1));
}
[[maybe_unused]] bool origin_bound_image_heap(const zone_reset_item_image &image,
					      size_t *output) noexcept
{
	size_t bytes = 0;
	if (!origin_bound_rows(bytes, image.items.capacity(), sizeof(player_item_snapshot)) ||
	    !origin_bound_rows(bytes, image.recipes.capacity(),
			       sizeof(native_mobile_birth_item_recipe)) ||
	    !origin_bound_rows(bytes, image.coins.capacity(), sizeof(zone_reset_coin_output)))
		return false;
	for (const auto &item : image.items)
	{
		if (!origin_bound_string(bytes, item.name) ||
		    !origin_bound_string(bytes, item.short_description) ||
		    !origin_bound_string(bytes, item.description) ||
		    !origin_bound_string(bytes, item.action_description) ||
		    !origin_bound_rows(bytes, item.dynamic_affects.capacity(),
				       sizeof(player_item_dynamic_affect_snapshot)) ||
		    !origin_bound_rows(bytes, item.extra_descriptions.capacity(),
				       sizeof(player_item_extra_description_snapshot)))
			return false;
		for (const auto &extra : item.extra_descriptions)
			if (!origin_bound_string(bytes, extra.keyword) ||
			    !origin_bound_string(bytes, extra.description) ||
			    !origin_bound_rows(bytes, extra.spell_ids.capacity(), sizeof(int32_t)))
				return false;
	}
	for (const auto &recipe : image.recipes)
		if (!origin_bound_rows(bytes, recipe.libraries.capacity(),
				       sizeof(native_mobile_birth_library_recipe)))
			return false;
	*output = bytes;
	return true;
}
[[maybe_unused]] error origin_correlate_bounded(const critical_command &command,
						const std::span<const uint8_t> &bytes,
						bool (*reserve)(size_t, void *) noexcept,
						void *context, size_t outer_live)
{
	size_t live = outer_live;
	if (!origin_bound_add(live, sizeof(zone_reset_item_image)) || !reserve(live, context))
		return error::capacity;
	zone_reset_item_image image;
	const auto decoded =
		zone_reset_item_command_decode_bounded(command, &image, reserve, context, live);
	if (decoded != error::ok)
		return decoded;
	size_t heap = 0, peak = live;
	if (!origin_bound_image_heap(image, &heap) || !origin_bound_add(peak, heap) ||
	    !origin_bound_add(peak, 2 * sizeof(item_transfer_result)) ||
	    !origin_bound_add(peak, sizeof(std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES>)) ||
	    !reserve(peak, context))
		return error::capacity;
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
} // namespace

economic_accounting_error zone_reset_item_origin_encode_bounded(
	const critical_command &command, const std::span<const uint8_t> &result,
	std::vector<uint8_t> *output, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept
{
	if (!output)
		return error::invalid_identity;
	if (!reserve)
		return error::capacity;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)command;
	(void)result;
	(void)context;
	(void)outer_live;
	return error::capacity;
#else
	try
	{
		size_t live = outer_live;
		if (!origin_bound_add(live, 2 * sizeof(std::vector<uint8_t>)) ||
		    !reserve(live, context))
			return error::capacity;
		std::vector<uint8_t> encoded, candidate;
		const auto checked =
			origin_correlate_bounded(command, result, reserve, context, live);
		if (checked != error::ok)
			return checked;
		const auto code =
			critical_command_encode_bounded(command, &encoded, reserve, context, live);
		if (code != critical_command_codec_result::ok)
			return code == critical_command_codec_result::overflow ?
				       error::capacity :
				       error::corrupt_evidence;
		if (encoded.empty() || encoded.size() > CRITICAL_COMMAND_MAX_ENCODED_BYTES)
			return error::capacity;
		size_t bytes = ZONE_RESET_ITEM_ORIGIN_HEADER_BYTES, peak = live;
		if (!origin_bound_add(bytes, encoded.size()) ||
		    !origin_bound_add(bytes, result.size()) ||
		    !origin_bound_add(peak, encoded.capacity()) || !origin_bound_add(peak, bytes) ||
		    !reserve(peak, context))
			return error::capacity;
		candidate.assign(bytes, 0);
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
#endif
}
namespace
{
struct origin_decode_workspace
{
	zone_reset_item_retained_origin candidate;
	std::vector<uint8_t> canonical;
	std::span<const uint8_t> result;
	size_t command_heap = 0;
};
}
economic_accounting_error zone_reset_item_origin_decode_bounded(
	const std::span<const uint8_t> &bytes, zone_reset_item_retained_origin *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live,
	size_t *retained_origin_heap_bytes) noexcept
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
	if (!reserve)
		return error::capacity;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)context;
	(void)outer_live;
	(void)retained_origin_heap_bytes;
	return error::capacity;
#else
	try
	{
		size_t live = outer_live;
		if (!origin_bound_add(live, sizeof(origin_decode_workspace)) ||
		    !reserve(live, context))
			return error::capacity;
		origin_decode_workspace work;
		const auto code = critical_command_decode_bounded(
			bytes.data() + ZONE_RESET_ITEM_ORIGIN_HEADER_BYTES,
			static_cast<size_t>(command_size), &work.candidate.original, reserve,
			context, live, &work.command_heap);
		if (code != critical_command_codec_result::ok)
			return code == critical_command_codec_result::overflow ?
				       error::capacity :
				       error::corrupt_evidence;
		if (!origin_bound_add(live, work.command_heap))
			return error::capacity;
		std::copy_n(bytes.end() - ITEM_TRANSFER_RESULT_BYTES, ITEM_TRANSFER_RESULT_BYTES,
			    work.candidate.result.begin());
		work.result = work.candidate.result;
		const auto checked = zone_reset_item_origin_encode_bounded(work.candidate.original,
									   work.result,
									   &work.canonical, reserve,
									   context, live);
		if (checked != error::ok)
			return checked;
		if (work.canonical.size() != bytes.size() ||
		    !std::equal(work.canonical.begin(), work.canonical.end(), bytes.begin()))
			return error::corrupt_evidence;
		static_assert(std::is_nothrow_move_assignable_v<zone_reset_item_retained_origin>);
		*output = std::move(work.candidate);
		if (retained_origin_heap_bytes)
			*retained_origin_heap_bytes = work.command_heap;
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
#endif
}
