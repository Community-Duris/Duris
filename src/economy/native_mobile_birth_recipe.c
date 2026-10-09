#include "economy/native_mobile_birth_recipe.h"

#include "core/config.h"

#include <array>
#include <bit>
#include <climits>
#include <string_view>
#include <utility>

namespace
{
// "NBR1", version, zero reserved word, complete forest row count.
constexpr uint32_t MAGIC = 0x3152424e;
constexpr size_t HEADER_BYTES = 12;
constexpr size_t ITEM_BYTES = 28;
constexpr size_t LIBRARY_BYTES = 12;

void put(uint8_t *output, uint64_t value, size_t bytes) noexcept
{
	for (size_t i = 0; i < bytes; ++i)
		output[i] = static_cast<uint8_t>(value >> (8 * i));
}
uint64_t get(const uint8_t *input, size_t bytes) noexcept
{
	uint64_t value = 0;
	for (size_t i = 0; i < bytes; ++i)
		value |= static_cast<uint64_t>(input[i]) << (8 * i);
	return value;
}
int32_t get_i32(const uint8_t *input) noexcept
{
	return std::bit_cast<int32_t>(static_cast<uint32_t>(get(input, 4)));
}
int16_t get_i16(const uint8_t *input) noexcept
{
	return std::bit_cast<int16_t>(static_cast<uint16_t>(get(input, 2)));
}
bool delay_valid(bool requested, int32_t delay) noexcept
{
	return requested ? delay >= PULSE_MOBILE - 4 && delay <= PULSE_MOBILE + 4 : delay == 0;
}
std::string_view library_name(native_mobile_birth_library library) noexcept
{
	switch (library)
	{
	case native_mobile_birth_library::actroom:
		return "actroom";
	case native_mobile_birth_library::actworn:
		return "actworn";
	case native_mobile_birth_library::hummer:
		return "hummer";
	case native_mobile_birth_library::sayresponse:
		return "sayresponse";
	case native_mobile_birth_library::transporter:
		return "transporter";
	}
	return {};
}
bool descriptor_valid(const player_item_extra_description_snapshot &description,
		      native_mobile_birth_library library) noexcept
{
	const auto name = library_name(library);
	const std::string_view keyword(description.keyword);
	constexpr std::string_view prefix = "_proclib_";
	// proclibObj_add creates a 50-byte canonical keyword, including its terminator.
	if (name.empty() || description.spellbook || keyword.size() >= 50 ||
	    keyword.size() <= prefix.size() + name.size() || !keyword.starts_with(prefix) ||
	    keyword.substr(prefix.size(), name.size()) != name ||
	    description.description.size() > PLAYER_SNAPSHOT_MAX_STRING_BYTES ||
	    description.description.find('\0') != std::string::npos)
		return false;
	const auto suffix = keyword.substr(prefix.size() + name.size());
	if (suffix.front() < '1' || suffix.front() > '9')
		return false;
	uint32_t number = 0;
	for (const char digit : suffix)
	{
		if (digit < '0' || digit > '9' ||
		    number > (static_cast<uint32_t>(INT_MAX) - static_cast<unsigned>(digit - '0')) /
				     10)
			return false;
		number = number * 10 + static_cast<unsigned>(digit - '0');
	}
	// Parsed parameters are literal bytes already carried by the image. In
	// particular, 0xff is an original delimiter, not invalid text to normalize.
	return number != 0;
}
bool items_valid(std::span<const player_item_snapshot> items) noexcept
{
	if (items.size() > PLAYER_SNAPSHOT_MAX_ROWS)
		return false;
	size_t rows = items.size();
	for (size_t i = 0; i < items.size(); ++i)
	{
		if (!items[i].object_uid || items[i].object_uid == UINT64_MAX ||
		    items[i].extra_descriptions.size() > PLAYER_SNAPSHOT_MAX_ROWS - rows)
			return false;
		rows += items[i].extra_descriptions.size();
		for (size_t previous = 0; previous < i; ++previous)
			if (items[previous].object_uid == items[i].object_uid)
				return false;
	}
	return true;
}
bool item_fields_valid(const player_item_snapshot &item,
		       const native_mobile_birth_item_recipe &recipe, size_t library_count) noexcept
{
	if (recipe.object_uid != item.object_uid ||
	    (recipe.binding_form != native_mobile_birth_binding_form::direct &&
	     recipe.binding_form != native_mobile_birth_binding_form::bridge) ||
	    recipe.procedure > native_mobile_birth_procedure::proclib_obj_proc ||
	    library_count > item.extra_descriptions.size() ||
	    !delay_valid(recipe.general_periodic, recipe.general_delay))
		return false;
	// The supported original CMD_SET_PERIODIC branches have these exact outcomes.
	// NONE may own the original ITEM_SWITCH fallback before conversion; a final
	// literal type cannot substitute for the factory's retained original decision.
	if (recipe.procedure == native_mobile_birth_procedure::super_cannon)
		return !recipe.general_periodic;
	if (recipe.procedure != native_mobile_birth_procedure::none)
		return recipe.general_periodic;
	return !library_count || !recipe.general_periodic;
}
bool library_valid(const player_item_snapshot &item,
		   const native_mobile_birth_library_recipe &library, std::span<uint8_t> seen,
		   bool *event_requested) noexcept
{
	if (library.extra_description_index >= seen.size() ||
	    seen[library.extra_description_index] ||
	    !descriptor_valid(item.extra_descriptions[library.extra_description_index],
			      library.library) ||
	    !delay_valid(library.periodic_requested, library.delay))
		return false;
	const bool periodic_library = library.library == native_mobile_birth_library::actroom ||
				      library.library == native_mobile_birth_library::actworn ||
				      library.library == native_mobile_birth_library::hummer;
	if (library.periodic_requested != (!*event_requested && periodic_library))
		return false;
	seen[library.extra_description_index] = 1;
	*event_requested = *event_requested || library.periodic_requested;
	return true;
}
void read_item(const uint8_t *input, native_mobile_birth_item_recipe *item) noexcept
{
	item->object_uid = get(input, 8);
	item->binding_form = static_cast<native_mobile_birth_binding_form>(input[8]);
	item->procedure = static_cast<native_mobile_birth_procedure>(input[9]);
	item->general_periodic = (input[10] & 1) != 0;
	item->random_exit_requested = (input[10] & 2) != 0;
	item->general_delay = get_i32(input + 12);
	item->trap_eff = get_i16(input + 16);
	item->trap_dam = get_i16(input + 18);
	item->trap_charge = get_i16(input + 20);
	item->trap_level = get_i16(input + 22);
}
native_mobile_birth_library_recipe read_library(const uint8_t *input) noexcept
{
	native_mobile_birth_library_recipe value;
	value.library = static_cast<native_mobile_birth_library>(input[0]);
	value.periodic_requested = input[1] != 0;
	value.extra_description_index = static_cast<uint32_t>(get(input + 4, 4));
	value.delay = get_i32(input + 8);
	return value;
}
economic_accounting_error preflight(std::span<const uint8_t> bytes,
				    std::span<const player_item_snapshot> items) noexcept
{
	if (bytes.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return economic_accounting_error::capacity;
	if (bytes.size() < HEADER_BYTES || get(bytes.data(), 4) != MAGIC ||
	    get(bytes.data() + 4, 2) != NATIVE_MOBILE_BIRTH_RECIPE_VERSION)
		return economic_accounting_error::invalid_version;
	if (get(bytes.data() + 6, 2) || get(bytes.data() + 8, 4) != items.size() ||
	    !items_valid(items) || items.size() > (bytes.size() - HEADER_BYTES) / ITEM_BYTES)
		return economic_accounting_error::corrupt_evidence;
	size_t offset = HEADER_BYTES, library_rows = 0;
	for (size_t i = 0; i < items.size(); ++i)
	{
		if (bytes.size() - offset < ITEM_BYTES)
			return economic_accounting_error::corrupt_evidence;
		const auto *input = bytes.data() + offset;
		native_mobile_birth_item_recipe recipe;
		read_item(input, &recipe);
		const size_t count = static_cast<uint32_t>(get(input + 24, 4));
		if ((input[10] & ~3u) || input[11] ||
		    count > PLAYER_SNAPSHOT_MAX_ROWS - library_rows ||
		    !item_fields_valid(items[i], recipe, count))
			return economic_accounting_error::corrupt_evidence;
		offset += ITEM_BYTES;
		const size_t remaining_items = items.size() - i - 1;
		if (remaining_items > (bytes.size() - offset) / ITEM_BYTES ||
		    count > (bytes.size() - offset - remaining_items * ITEM_BYTES) / LIBRARY_BYTES)
			return economic_accounting_error::corrupt_evidence;
		library_rows += count;
		std::array<uint8_t, PLAYER_SNAPSHOT_MAX_ROWS> seen{};
		bool requested = false;
		for (size_t l = 0; l < count; ++l)
		{
			const auto *library_input = bytes.data() + offset;
			const auto library = read_library(library_input);
			if (library_input[1] > 1 || get(library_input + 2, 2) ||
			    !library_valid(items[i], library,
					   { seen.data(), items[i].extra_descriptions.size() },
					   &requested))
				return economic_accounting_error::corrupt_evidence;
			offset += LIBRARY_BYTES;
		}
	}
	return offset == bytes.size() ? economic_accounting_error::ok :
					economic_accounting_error::corrupt_evidence;
}
} // namespace

bool native_mobile_birth_recipe_valid(
	std::span<const player_item_snapshot> items,
	std::span<const native_mobile_birth_item_recipe> recipes) noexcept
{
	if (recipes.size() != items.size() || !items_valid(items))
		return false;
	size_t bytes = HEADER_BYTES, library_rows = 0;
	for (size_t i = 0; i < recipes.size(); ++i)
	{
		const auto &recipe = recipes[i];
		if (!item_fields_valid(items[i], recipe, recipe.libraries.size()) ||
		    recipe.libraries.size() > PLAYER_SNAPSHOT_MAX_ROWS - library_rows ||
		    bytes > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - ITEM_BYTES)
			return false;
		bytes += ITEM_BYTES;
		if (recipe.libraries.size() >
		    (CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - bytes) / LIBRARY_BYTES)
			return false;
		bytes += recipe.libraries.size() * LIBRARY_BYTES;
		library_rows += recipe.libraries.size();
		std::array<uint8_t, PLAYER_SNAPSHOT_MAX_ROWS> seen{};
		bool requested = false;
		for (const auto &library : recipe.libraries)
			if (!library_valid(items[i], library,
					   { seen.data(), items[i].extra_descriptions.size() },
					   &requested))
				return false;
	}
	return true;
}

economic_accounting_error
native_mobile_birth_recipe_encode(std::span<const player_item_snapshot> items,
				  std::span<const native_mobile_birth_item_recipe> recipes,
				  std::vector<uint8_t> *output) noexcept
{
	if (!output || !native_mobile_birth_recipe_valid(items, recipes))
		return economic_accounting_error::corrupt_evidence;
	size_t size = HEADER_BYTES;
	for (const auto &recipe : recipes)
		size += ITEM_BYTES + recipe.libraries.size() * LIBRARY_BYTES;
	try
	{
		std::vector<uint8_t> candidate(size, 0);
		put(candidate.data(), MAGIC, 4);
		put(candidate.data() + 4, NATIVE_MOBILE_BIRTH_RECIPE_VERSION, 2);
		put(candidate.data() + 8, recipes.size(), 4);
		size_t offset = HEADER_BYTES;
		for (const auto &recipe : recipes)
		{
			auto *item = candidate.data() + offset;
			put(item, recipe.object_uid, 8);
			item[8] = static_cast<uint8_t>(recipe.binding_form);
			item[9] = static_cast<uint8_t>(recipe.procedure);
			item[10] = static_cast<uint8_t>((recipe.general_periodic ? 1 : 0) |
							(recipe.random_exit_requested ? 2 : 0));
			put(item + 12, static_cast<uint32_t>(recipe.general_delay), 4);
			put(item + 16, static_cast<uint16_t>(recipe.trap_eff), 2);
			put(item + 18, static_cast<uint16_t>(recipe.trap_dam), 2);
			put(item + 20, static_cast<uint16_t>(recipe.trap_charge), 2);
			put(item + 22, static_cast<uint16_t>(recipe.trap_level), 2);
			put(item + 24, recipe.libraries.size(), 4);
			offset += ITEM_BYTES;
			for (const auto &library : recipe.libraries)
			{
				auto *encoded = candidate.data() + offset;
				encoded[0] = static_cast<uint8_t>(library.library);
				encoded[1] = library.periodic_requested ? 1 : 0;
				put(encoded + 4, library.extra_description_index, 4);
				put(encoded + 8, static_cast<uint32_t>(library.delay), 4);
				offset += LIBRARY_BYTES;
			}
		}
		*output = std::move(candidate);
		return economic_accounting_error::ok;
	}
	catch (...)
	{
		return economic_accounting_error::capacity;
	}
}

economic_accounting_error
native_mobile_birth_recipe_decode(std::span<const uint8_t> bytes,
				  std::span<const player_item_snapshot> items,
				  std::vector<native_mobile_birth_item_recipe> *output) noexcept
{
	if (!output)
		return economic_accounting_error::corrupt_evidence;
	const auto checked = preflight(bytes, items);
	if (checked != economic_accounting_error::ok)
		return checked;
	// Every count, remaining-byte bound, field and descriptor binding has been
	// checked without allocation. Only the complete valid recipe allocates here.
	try
	{
		std::vector<native_mobile_birth_item_recipe> candidate;
		candidate.reserve(items.size());
		size_t offset = HEADER_BYTES;
		for (size_t i = 0; i < items.size(); ++i)
		{
			native_mobile_birth_item_recipe recipe;
			read_item(bytes.data() + offset, &recipe);
			const size_t count =
				static_cast<uint32_t>(get(bytes.data() + offset + 24, 4));
			offset += ITEM_BYTES;
			recipe.libraries.reserve(count);
			for (size_t l = 0; l < count; ++l)
			{
				recipe.libraries.push_back(read_library(bytes.data() + offset));
				offset += LIBRARY_BYTES;
			}
			candidate.push_back(std::move(recipe));
		}
		*output = std::move(candidate);
		return economic_accounting_error::ok;
	}
	catch (...)
	{
		return economic_accounting_error::capacity;
	}
}


namespace
{
// descriptor_valid owns name, keyword, prefix and suffix string_view objects;
// library_valid receives one seen span. They coexist with the seen array.
constexpr size_t recipe_validation_objects =
	sizeof(std::array<uint8_t, PLAYER_SNAPSHOT_MAX_ROWS>) +
	sizeof(std::string_view) + sizeof(std::string_view) +
	sizeof(std::string_view) + sizeof(std::string_view) + sizeof(std::span<uint8_t>);
// Wire preflight additionally owns its empty recipe and decoded library value.
// Include read_library's local return value too, without assuming NRVO.
constexpr size_t recipe_preflight_objects = recipe_validation_objects +
	sizeof(native_mobile_birth_item_recipe) + sizeof(native_mobile_birth_library_recipe) +
	sizeof(native_mobile_birth_library_recipe);

bool recipe_profile_add(size_t &value, size_t amount) noexcept
{
	if (amount > SIZE_MAX - value)
		return false;
	value += amount;
	return true;
}
bool recipe_profile_array(size_t count, size_t unit, size_t *value) noexcept
{
	if (!value || (unit && count > SIZE_MAX / unit))
		return false;
	*value = count * unit;
	return true;
}
bool recipe_profile_finish(native_mobile_birth_recipe_allocation_profile &profile) noexcept
{
	if (!recipe_profile_array(profile.item_count, sizeof(native_mobile_birth_item_recipe),
				  &profile.decoded_row_storage_bytes) ||
	    !recipe_profile_array(profile.library_count, sizeof(native_mobile_birth_library_recipe),
				  &profile.decoded_library_storage_bytes))
		return false;
	profile.decoded_payload_bytes = profile.decoded_row_storage_bytes;
	if (!recipe_profile_add(profile.decoded_payload_bytes, profile.decoded_library_storage_bytes))
		return false;
	profile.validation_inline_storage_bytes = recipe_validation_objects;
	profile.preflight_inline_storage_bytes = recipe_preflight_objects;
	profile.encoder_inline_storage_bytes = sizeof(std::vector<uint8_t>);
	// candidate, current recipe, returned library and its callee local; no NRVO assumption.
	profile.decoder_inline_storage_bytes = sizeof(std::vector<native_mobile_birth_item_recipe>) +
		sizeof(native_mobile_birth_item_recipe) + sizeof(native_mobile_birth_library_recipe) +
		sizeof(native_mobile_birth_library_recipe);
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
	profile.fresh_decode_storage_policy_supported = true;
	profile.fresh_encode_storage_policy_supported = true;
	// Fresh reserve(count) and vector(size, 0) request exactly those capacities.
	profile.encoded_capacity_bytes = profile.wire_bytes;
#endif
	return true;
}
} // namespace

size_t native_mobile_birth_recipe_profile_inline_storage_bytes() noexcept
{
	// Conservatively reserve candidate and validator object phases together.
	return sizeof(native_mobile_birth_recipe_allocation_profile) + recipe_preflight_objects;
}

economic_accounting_error native_mobile_birth_recipe_encode_profile(
	std::span<const player_item_snapshot> items,
	std::span<const native_mobile_birth_item_recipe> recipes,
	native_mobile_birth_recipe_allocation_profile *output) noexcept
{
	if (!output || !native_mobile_birth_recipe_valid(items, recipes))
		return economic_accounting_error::corrupt_evidence;
	native_mobile_birth_recipe_allocation_profile profile;
	profile.item_count = items.size();
	profile.wire_bytes = HEADER_BYTES;
	for (const auto &recipe : recipes)
	{
		size_t libraries = 0;
		if (!recipe_profile_add(profile.library_count, recipe.libraries.size()) ||
		    !recipe_profile_array(recipe.libraries.size(), LIBRARY_BYTES, &libraries) ||
		    !recipe_profile_add(profile.wire_bytes, ITEM_BYTES) ||
		    !recipe_profile_add(profile.wire_bytes, libraries))
			return economic_accounting_error::capacity;
	}
	if (!recipe_profile_finish(profile))
		return economic_accounting_error::capacity;
	*output = profile;
	return economic_accounting_error::ok;
}

economic_accounting_error native_mobile_birth_recipe_decode_profile(
	std::span<const uint8_t> bytes, std::span<const player_item_snapshot> items,
	native_mobile_birth_recipe_allocation_profile *output) noexcept
{
	if (!output)
		return economic_accounting_error::corrupt_evidence;
	const auto checked = preflight(bytes, items);
	if (checked != economic_accounting_error::ok)
		return checked;
	native_mobile_birth_recipe_allocation_profile profile;
	profile.item_count = items.size();
	profile.wire_bytes = bytes.size();
	size_t offset = HEADER_BYTES;
	for (size_t i = 0; i < items.size(); ++i)
	{
		const size_t count = static_cast<uint32_t>(get(bytes.data() + offset + 24, 4));
		size_t libraries = 0;
		if (!recipe_profile_add(profile.library_count, count) ||
		    !recipe_profile_array(count, LIBRARY_BYTES, &libraries) ||
		    !recipe_profile_add(offset, ITEM_BYTES) || !recipe_profile_add(offset, libraries))
			return economic_accounting_error::capacity;
	}
	if (offset != bytes.size() || !recipe_profile_finish(profile))
		return economic_accounting_error::capacity;
	*output = profile;
	return economic_accounting_error::ok;
}
