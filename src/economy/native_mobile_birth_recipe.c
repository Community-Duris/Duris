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

#include <iterator>
#include <type_traits>

namespace
{
// These are source objects/carriers, not an emitted-stack estimate. All original
// recipe DTOs, input/seen spans, four descriptor views and capacity requests stay
// in their established caller/profile ownership. Internal STL temporaries below
// are distinct source objects actually created by the selected GNU13 overloads.
constexpr size_t stock_P = sizeof(void *);
constexpr size_t stock_N = sizeof(size_t);

template <class T> constexpr size_t result_vector_cleanup_frames() noexcept
{
	using V = std::vector<T>;
	using A = std::allocator<T>;
	constexpr size_t destruction = sizeof(V *) + sizeof(V *) + sizeof(A *) + 2 * sizeof(T *) +
				       sizeof(A *) + 2 * sizeof(T *) + 2 * sizeof(T *) +
				       sizeof(bool);
	constexpr size_t element = std::is_trivially_destructible_v<T> ? 0 : 4 * sizeof(T *);
	constexpr size_t deallocation =
		sizeof(void *) + sizeof(V *) + sizeof(T *) + sizeof(size_t) + sizeof(A *) +
		sizeof(T *) + sizeof(size_t) + sizeof(A *) + sizeof(T *) + sizeof(size_t) +
		sizeof(A *) + sizeof(T *) + sizeof(size_t) + sizeof(void *) + sizeof(size_t) +
		sizeof(bool) + sizeof(A *);
	// _Vector_base's member/base cleanup really reaches implicit ~_Vector_impl,
	// ~_Vector_impl_data and ~__new_allocator, each with its own this carrier.
	return destruction + element + deallocation + 3 * stock_P;
}

template <class T> constexpr size_t stock_vector_default_source() noexcept
{
	// vector, _Vector_base, _Vector_impl, allocator, __new_allocator and
	// _Vector_impl_data default constructors: six genuine this carriers.
	return 6 * stock_P;
}
template <class T> constexpr size_t stock_vector_get_allocator_source() noexcept
{
	// _Vector_base::get_allocator(this); _M_get_Tp_allocator(this,returned-ref);
	// allocator(const&) and __new_allocator(const&): this/source for each.
	// The returned allocator value is not a member of the caller vector.
	return 7 * stock_P + sizeof(std::allocator<T>);
}
template <class T> constexpr size_t stock_vector_const_allocator_ctor_source() noexcept
{
	// vector(alloc), _Vector_base(alloc), _Vector_impl(alloc), allocator copy,
	// new_allocator copy each this/source; _Vector_impl_data default this.
	return 11 * stock_P;
}
template <class T> constexpr size_t stock_vector_data_swap_source() noexcept
{
	// _M_swap_data(this,source), its real three-pointer __tmp, data default
	// ctor(this), three _M_copy_data(this,source) calls, implicit tmp dtor(this).
	return 2 * stock_P + 3 * sizeof(T *) + stock_P + 3 * (2 * stock_P) + stock_P;
}
template <class T> constexpr size_t stock_vector_move_constructor_source() noexcept
{
	// Defaulted vector/base moves, impl move, allocator/new_allocator const
	// copies, data move: six this/source pairs. Impl makes two std::move calls
	// (reference/result each); data move's pointer() null-reset result is real.
	return 6 * (2 * stock_P) + 2 * (2 * stock_P) + sizeof(T *);
}
template <class T> constexpr size_t stock_vector_move_assignment_source() noexcept
{
	// operator=(this,source,returned-ref), named constexpr __move_storage,
	// _S_propagate_on_move_assign bool result (true short-circuits _S_always_equal),
	// std::move(ref,result), _M_move_assign(this,source,actual true_type value),
	// generated true_type ctor/dtor this. The actual __tmp vector is separate
	// from input/output vectors. get_allocator's value dies via allocator and
	// new_allocator dtors after __tmp's const-allocator construction.
	constexpr size_t entry = 3 * stock_P + 2 * sizeof(bool) + 2 * stock_P + 2 * stock_P +
				 sizeof(std::true_type) + 2 * stock_P;
	// C++20 __alloc_on_move(one,two), std::move(ref,result), generated allocator
	// assignment(this,source,returned-ref), generated new_allocator assignment
	// (this,source,returned-ref):10P, plus both _M_get_Tp_allocator(this,ref):4P.
	constexpr size_t allocator_move = 10 * stock_P + 2 * (2 * stock_P);
	return entry + sizeof(std::vector<T>) + stock_vector_get_allocator_source<T>() +
	       2 * stock_P + stock_vector_const_allocator_ctor_source<T>() +
	       2 * stock_vector_data_swap_source<T>() + allocator_move +
	       result_vector_cleanup_frames<T>();
}

constexpr size_t stock_library_default_source = stock_P;
constexpr size_t stock_library_copy_source = 2 * stock_P;
constexpr size_t stock_library_move_source = 2 * stock_P;
constexpr size_t stock_library_destructor_source = stock_P;
constexpr size_t stock_item_default_source =
	stock_P + stock_vector_default_source<native_mobile_birth_library_recipe>();
constexpr size_t stock_item_move_source =
	2 * stock_P + stock_vector_move_constructor_source<native_mobile_birth_library_recipe>();
constexpr size_t stock_item_destructor_source =
	stock_P + result_vector_cleanup_frames<native_mobile_birth_library_recipe>();

// Dynamic extent spans: generated span const-copy(this,source) and extent
// const-copy(this,source); span dtor(this) and extent dtor(this). The actual
// span values are already caller/profile objects, not new SOURCE objects here.
constexpr size_t stock_span_copy_lifetime_source = 4 * stock_P + 2 * stock_P;
constexpr size_t stock_span_size_source =
	(stock_P + stock_N) + (stock_P + stock_N); // size -> extent::_M_extent
constexpr size_t stock_span_pointer_ctor_source =
	(2 * stock_P + stock_N) + // span(this,first,count)
	2 * (2 * stock_P) + // std::to_address -> raw __to_address
	(stock_P + stock_N); // extent(this,count)
constexpr size_t stock_span_begin_source =
	2 * stock_P + 2 * stock_P + stock_P; // this/iterator, normal ctor, iterator dtor
// end constructs the iterator from pointer addition, so its const-reference
// argument binds a genuine extra pointer temporary; begin binds a stored lvalue.
constexpr size_t stock_span_end_source = stock_span_begin_source + stock_span_size_source + stock_P;

// The library value returned by read_library has a real default-member ctor,
// optional non-NRVO generated implicit return-move and local/returned DTO destructors.
constexpr size_t stock_read_library_value_source = stock_library_default_source +
						   stock_library_move_source +
						   2 * stock_library_destructor_source;

template <class T> constexpr size_t stock_reserved_push_source() noexcept
{
	// push_back(this,rvalue-ref) -> std::move(ref,result); emplace_back(this,
	// arg-ref,returned-ref) -> forward(ref,result); allocator_traits::construct
	// (allocator-ref,location,arg-ref) -> forward; construct_at(location,arg-ref,
	// returned-pointer) -> forward; actual placement-new(size,where,result).
	constexpr size_t construction = 2 * stock_P + 2 * stock_P + 3 * stock_P + 2 * stock_P +
					3 * stock_P + 2 * stock_P + 3 * stock_P + 2 * stock_P +
					stock_N + 2 * stock_P;
	// C++20 emplace return -> back(this,returned-ref) -> end(this,iterator)
	// + normal ctor(this,pointer-ref), iterator::operator-(this,difference,
	// returned-iterator) + normal ctor; dereference(this,returned-ref). Both
	// actual iterator temporaries have generated destructors(this). Capacity
	// was genuinely reserved for all rows, so _M_realloc_insert is not selected.
	constexpr size_t back =
		2 * stock_P + 2 * stock_P + 2 * stock_P + 2 * stock_P + sizeof(std::ptrdiff_t) +
		2 * stock_P + 2 * stock_P + 2 * stock_P +
		// operator-(n) constructs the returned iterator from pointer subtraction:
		// a distinct pointer temporary binds its const-reference ctor argument.
		stock_P;
	return construction + back;
}

constexpr size_t stock_byte_fill_source =
	// allocator-specialized __uninitialized_fill_n_a(first,n,value-ref,alloc-ref,
	// returned-first), genuine is_constant_evaluated result; public
	// uninitialized_fill_n(first,n,value-ref,returned-first,__can_fill).
	(4 * stock_P + stock_N + sizeof(bool)) + (3 * stock_P + stock_N + sizeof(bool)) +
	// __uninitialized_fill_n<true>::__uninit_fill_n and fill_n each
	// first/n/value-ref/result. fill_n owns both returned size integer and tag.
	2 * (3 * stock_P + stock_N) +
	2 * stock_N + // __size_to_integer(unsigned long input,returned size)
	stock_P + sizeof(std::random_access_iterator_tag) + // category input/result
	// random_access -> bidirectional -> forward -> input tag ctor/dtor chains.
	8 * stock_P +
	// __fill_n_a(first,n,value-ref,random tag,returned-first), then
	// __fill_a(first,last,value-ref), then byte __fill_a1(first,last,value-ref,
	// __tmp,__len,is_constant_evaluated result), actual memset arguments/result.
	3 * stock_P + stock_N + sizeof(std::random_access_iterator_tag) + 3 * stock_P +
	3 * stock_P + sizeof(uint8_t) + stock_N + sizeof(bool) + 2 * stock_P + sizeof(int) +
	stock_N;

template <class T> constexpr size_t stock_count_value_vector_ctor_source() noexcept
{
	// Real default allocator argument: allocator/new_allocator ctor and dtor.
	// vector(this,count,value-ref,allocator-ref); _Vector_base(this,n,alloc-ref)
	// plus its full impl/allocator/data const-copy chain; _M_create_storage(this,n).
	// _S_check_init_len(n,alloc-ref,returned-n) owns actual temporary allocator
	// copy (allocator/new_allocator this/source), value, and both destructors;
	// _S_max_size's exact descendant scalar graph stays stock_allocation_source.
	return sizeof(std::allocator<T>) + 4 * stock_P + 3 * stock_P + stock_N + 2 * stock_P +
	       stock_N + (2 * stock_P + 2 * stock_P + 2 * stock_P + stock_P) + stock_P + stock_N +
	       stock_P + 2 * stock_N + 4 * stock_P + sizeof(std::allocator<T>) + 2 * stock_P +
	       // _M_fill_initialize(this,n,value-ref) and _M_get_Tp_allocator(this,ref).
	       2 * stock_P + stock_N + 2 * stock_P;
}

bool stock_source_profile_supported() noexcept
{
#if defined(__linux__) && defined(__x86_64__) && defined(__GNUC__) && __GNUC__ == 13 &&    \
	!defined(__clang__) && __cplusplus == 202002L && defined(_GLIBCXX_RELEASE) &&      \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&                       \
	_GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG) &&                         \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                    \
	!(defined(_GLIBCXX_SANITIZE_STD_ALLOCATOR) && defined(_GLIBCXX_SANITIZE_VECTOR) && \
	  _GLIBCXX_SANITIZE_STD_ALLOCATOR && _GLIBCXX_SANITIZE_VECTOR)
	return sizeof(void *) == 8 && sizeof(size_t) == 8 && sizeof(std::ptrdiff_t) == 8 &&
	       sizeof(unsigned int) == 4 && sizeof(unsigned long) == 8;
#else
	return false;
#endif
}

constexpr size_t stock_span_access_source =
	// Full dynamic size/extent, data, index, begin/end normal iterators and
	// pointer/count/to_address/extent constructor paths. Array::data delegates
	// to array_traits::_S_ptr, each this/reference and returned pointer:4P.
	stock_span_size_source + 2 * stock_P + (2 * stock_P + stock_N) + stock_span_begin_source +
	stock_span_end_source + stock_span_pointer_ctor_source + 4 * stock_P +
	// Public validation/preflight two by-value spans and items_valid's copied
	// items span: three actual copy/extent-copy and destructor/extent-dtor paths.
	3 * stock_span_copy_lifetime_source +
	// library_valid's pointer/count seen span has its own span/extent cleanup;
	// its physical value is still recipe_validation_objects-owned.
	2 * stock_P;

constexpr size_t stock_descriptor_source =
	// descriptor_valid(description&,library), library_name(enum)/return. Four
	// named name/keyword/prefix/suffix views belong to recipe_validation_objects.
	sizeof(void *) + sizeof(native_mobile_birth_library) + sizeof(bool) +
	sizeof(native_mobile_birth_library) + sizeof(uint32_t) + sizeof(char) +
	// suffix range-for hidden range/begin/end, begin/end receivers/results;
	// front/size/empty member receivers/results and actual per-digit value.
	3 * sizeof(void *) + 4 * sizeof(void *) + 3 * (sizeof(void *) + sizeof(size_t)) +
	sizeof(void *) + sizeof(char) +
	// literal view constructor/traits length/runtime builtin strlen; std::string
	// conversion -> data + length + pointer/length view constructor.
	2 * sizeof(void *) + sizeof(void *) + sizeof(size_t) + sizeof(bool) + sizeof(void *) +
	sizeof(size_t) + 8 * sizeof(void *) + 2 * sizeof(size_t) +
	// starts_with(view) -> substr -> __sv_check + size + min + view ctor.
	// Passed/returned view values here are additional call values, distinct
	// from the four original named descriptor views charged above.
	sizeof(void *) + sizeof(std::string_view) + sizeof(bool) + sizeof(void *) +
	3 * sizeof(size_t) + sizeof(std::string_view) + 2 * sizeof(size_t) + sizeof(void *) +
	sizeof(size_t) + sizeof(void *) + sizeof(size_t) + 2 * sizeof(void *) + sizeof(bool) +
	sizeof(size_t) + 2 * sizeof(void *) + sizeof(size_t) +
	// view operator==/compare both actual by-value operands/argument; retained
	// substr return and min return remain live across traits::compare.
	3 * sizeof(std::string_view) + sizeof(bool) + sizeof(void *) + 2 * sizeof(size_t) +
	2 * sizeof(int) + 2 * sizeof(void *) + sizeof(bool) + sizeof(size_t) + 2 * sizeof(void *) +
	sizeof(size_t) + sizeof(int) + sizeof(bool) +
	// compare's actual _S_compare difference branch; builtin memcmp arguments.
	2 * sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(int) + 2 * sizeof(void *) +
	sizeof(size_t) + sizeof(int) +
	// basic_string::find(char,pos): this/c/pos, size, data and found pointer;
	// traits::find input/count/char-ref, constant evaluation and memchr result.
	// find this/c/pos plus __ret/__size/__n and returned size:5N; actual
	// __data/__p:2P. Its size(this,result) and _M_data(this,pointer) are real.
	sizeof(void *) + sizeof(char) + 5 * sizeof(size_t) + 2 * sizeof(void *) +
	(sizeof(void *) + sizeof(size_t)) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t) + sizeof(bool) + sizeof(void *) + sizeof(void *) + sizeof(int) +
	sizeof(size_t) + sizeof(void *) +
	// library_name's default-view return ctor; actual four named view dtors.
	stock_P + 4 * stock_P +
	// Five genuine lvalue const-copy calls: starts_with(prefix), its equality
	// RHS, its compare argument, direct suffix-name equality RHS and compare
	// argument. Each defaulted view const-copy has this/source:2P. Starts_with,
	// two equality pairs and both compare args have seven by-value dtors:7P.
	5 * (2 * stock_P) + 7 * stock_P +
	// front() returns a char reference, separate from the digit read value.
	stock_P;

constexpr size_t stock_validation_source =
	// items_valid: span already charged, rows/i/previous and return boolean.
	3 * sizeof(size_t) + sizeof(bool) +
	// item_fields_valid(item&,recipe&,count) and delay_valid(requested,delay).
	2 * sizeof(void *) + sizeof(size_t) + sizeof(bool) + sizeof(bool) + sizeof(int32_t) +
	sizeof(bool) +
	// library_valid(item&,library&,seen,event*): seen already charged; actual
	// periodic boolean and returned boolean. Nested descriptor carries above.
	3 * sizeof(void *) + 2 * sizeof(bool) +
	// recipe_valid: input spans already charged, bytes/library_rows/i, recipe&,
	// requested plus hidden library range/begin/end and actual library reference.
	3 * sizeof(size_t) + sizeof(void *) + 2 * sizeof(bool) + 4 * sizeof(void *) +
	// vector size/index/range/normal-iterator dereference/comparison/increment.
	4 * (sizeof(void *) + sizeof(size_t)) + 4 * (2 * sizeof(void *)) + 2 * sizeof(void *) +
	sizeof(bool) + sizeof(void *) + stock_span_access_source + stock_descriptor_source;

constexpr size_t stock_wire_source =
	// put(output,value,bytes,i), get(input,bytes,value,i), i16/i32 input and
	// actual unsigned/signed returns plus bit_cast const-reference/return.
	sizeof(void *) + sizeof(uint64_t) + 2 * sizeof(size_t) + sizeof(void *) +
	2 * sizeof(size_t) + sizeof(uint64_t) + 2 * sizeof(void *) + sizeof(int32_t) +
	sizeof(int16_t) + 2 * sizeof(void *) + sizeof(uint32_t) + sizeof(uint16_t) +
	// read_item input/item receivers. read_library's DTO/local+return are in the
	// old profile, while its actual input pointer and primitive calls are here.
	3 * sizeof(void *) + stock_read_library_value_source;

constexpr size_t stock_preflight_source =
	// preflight's two spans/seen/current recipe/two library values already
	// charged. Named offset/library_rows/i/count/remaining_items/l, input and
	// library_input pointers, requested and typed returned status remain here.
	6 * sizeof(size_t) + 2 * sizeof(void *) + sizeof(bool) + sizeof(economic_accounting_error) +
	stock_wire_source + stock_validation_source +
	// preflight really constructs and destroys its empty recipe/library vector.
	stock_item_default_source + stock_item_destructor_source;

constexpr size_t stock_profile_source =
	// add(ref,amount,bool), array(count,unit,out,bool), finish(receiver,bool),
	// encode/decode profile output pointer, typed status, scalar scans and range.
	sizeof(void *) + sizeof(size_t) + sizeof(bool) + 2 * sizeof(size_t) + sizeof(void *) +
	sizeof(bool) + sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + 5 * sizeof(size_t) +
	4 * sizeof(void *) + 2 * sizeof(economic_accounting_error) +
	// Actual profile default-member ctor(this), generated assignment
	// (this,source,returned-reference). Physical DTO stays old inline-owned.
	4 * sizeof(void *);

constexpr size_t stock_max_size_source =
	// _S_max_size(allocator-ref,__diffmax,__allocmax,returned size), allocator
	// traits max_size(allocator-ref,returned size), min(two refs,returned ref,bool).
	(stock_P + 3 * stock_N) + (stock_P + stock_N) + 3 * stock_P + sizeof(bool);
constexpr size_t stock_allocation_source =
	// _M_allocate(this,n,returned pointer); allocator_traits::allocate(alloc-ref,
	// n,returned pointer); allocator::allocate(this,n,returned pointer,actual
	// constant-evaluation result); new_allocator::allocate(this,n,hint,result),
	// its _M_max_size(this,result), then ordinary operator new(n,result).
	3 * (2 * stock_P + stock_N) + sizeof(bool) + (3 * stock_P + stock_N) + (stock_P + stock_N) +
	(stock_N + stock_P);

constexpr size_t stock_value_lifecycle_source =
	// Public value-lifecycle query owns the real vector<recipe> empty default,
	// allocator-stealing move assignment and populated cleanup. Stealing does
	// not default-construct/move any rows; those genuine row operations are
	// separate decoder/preflight controllers below. Nested destructor body is
	// reached from populated outer-vector cleanup, with no library allocation.
	stock_vector_default_source<native_mobile_birth_item_recipe>() +
	stock_vector_move_assignment_source<native_mobile_birth_item_recipe>() +
	result_vector_cleanup_frames<native_mobile_birth_item_recipe>() +
	stock_item_destructor_source;

constexpr size_t stock_encode_vector_source =
	// Genuine count/value overload only: no unrelated empty-vector constructor.
	// The zero value and allocator argument/actual count/base/fill controllers
	// are not the old inline candidate vector. Full max-size and allocation
	// leaves are separate typed controllers; candidate cleanup and actual output
	// move-assign temporary each own their own real destructor paths.
	sizeof(uint8_t) + stock_count_value_vector_ctor_source<uint8_t>() + stock_max_size_source +
	stock_allocation_source + stock_byte_fill_source + result_vector_cleanup_frames<uint8_t>() +
	stock_vector_move_assignment_source<uint8_t>();

// Both fresh reserve calls have genuinely empty old ranges. These actual
// non-bitwise relocation controllers are reused sequentially for item/library
// DTOs (default member initializers prevent the library DTO being trivial).
// No element relocation body executes in this accepted fresh-reserve domain.
constexpr size_t stock_fresh_empty_relocation_source =
	5 * stock_P + // _S_relocate(first,last,result,allocator-ref,returned-pointer)
	5 * stock_P + // __relocate_a: same four arguments and returned pointer
	3 * (2 * stock_P) + // raw __niter_base(input,returned-pointer), three calls
	6 * stock_P + // non-bitwise __relocate_a_1: four args, __cur and return
	2 * stock_P; // reserve's actual _M_get_Tp_allocator(this,returned-ref)

constexpr size_t stock_decode_vector_source =
	// Genuine fresh reserve calls for recipe and library vectors: this/n/
	// old_size/tmp; size/capacity/max_size, _S_relocate and empty range relocate
	// inputs. No element relocate body or realloc-insert executes in this route.
	2 * (stock_P + 2 * stock_N + stock_P) + 4 * (stock_P + stock_N) +
	stock_fresh_empty_relocation_source +
	// Both reserve max_size wrappers -> _M_get_Tp_allocator -> _S_max_size.
	2 * ((stock_P + stock_N) + 2 * stock_P + stock_max_size_source) + stock_allocation_source +
	// Real pre-reserved library/item push -> emplace -> traits::construct ->
	// construct_at -> placement new and actual back/iterator return descendants.
	stock_reserved_push_source<native_mobile_birth_library_recipe>() +
	stock_reserved_push_source<native_mobile_birth_item_recipe>() + stock_library_move_source +
	stock_item_default_source + stock_item_move_source +
	// Current row is a separate stack object, including its moved-from cleanup.
	stock_item_destructor_source + stock_value_lifecycle_source;

constexpr size_t stock_encode_source =
	stock_validation_source + stock_profile_source + stock_encode_vector_source +
	stock_wire_source +
	// Original encoder spans/vector already charged; output, size/offset, two
	// desugared recipe ranges, library range and true encoded/item pointers.
	sizeof(void *) + 2 * sizeof(size_t) + 12 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(economic_accounting_error) +
	// Actual encoder/profile input-copy lifetimes beyond called validation.
	4 * stock_span_copy_lifetime_source;
constexpr size_t stock_decode_source =
	stock_preflight_source + stock_profile_source + stock_decode_vector_source +
	// Original decoder named output/check/offset/i/count/l, wire data accessor;
	// candidate, recipe and two library DTOs are already in original profile.
	sizeof(void *) + 5 * sizeof(size_t) + 2 * sizeof(economic_accounting_error) +
	stock_span_access_source +
	// Actual decoder/profile input-copy lifetimes beyond called preflight.
	4 * stock_span_copy_lifetime_source;
}

bool native_mobile_birth_recipe_encode_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !stock_source_profile_supported())
		return false;
	*output = stock_encode_source;
	return true;
}
bool native_mobile_birth_recipe_decode_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !stock_source_profile_supported())
		return false;
	*output = stock_decode_source;
	return true;
}
bool native_mobile_birth_recipe_encode_source_supplement_frame_bytes(size_t *output) noexcept
{
	if (!output || !stock_source_profile_supported())
		return false;
	*output = stock_encode_source;
	return true;
}
bool native_mobile_birth_recipe_decode_source_supplement_frame_bytes(size_t *output) noexcept
{
	if (!output || !stock_source_profile_supported())
		return false;
	*output = stock_decode_source;
	return true;
}
bool native_mobile_birth_recipe_encode_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !stock_source_profile_supported())
		return false;
	*output = 2 * sizeof(native_mobile_birth_recipe_allocation_profile) +
		  recipe_preflight_objects + 2 * sizeof(std::span<const player_item_snapshot>) +
		  2 * sizeof(std::span<const native_mobile_birth_item_recipe>);
	return true;
}
bool native_mobile_birth_recipe_decode_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !stock_source_profile_supported())
		return false;
	*output = 2 * sizeof(native_mobile_birth_recipe_allocation_profile) +
		  recipe_preflight_objects + 2 * sizeof(std::span<const uint8_t>) +
		  2 * sizeof(std::span<const player_item_snapshot>);
	return true;
}
bool native_mobile_birth_recipe_value_lifecycle_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !stock_source_profile_supported())
		return false;
	*output = stock_value_lifecycle_source;
	return true;
}
bool native_mobile_birth_recipe_valid_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !stock_source_profile_supported())
		return false;
	*output = stock_validation_source;
	return true;
}
