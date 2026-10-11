#include "economy/native_mobile_birth_constructor_recipe.h"

#include <algorithm>
#include <bit>
#include <climits>
#include <ctime>
#include <new>
#include <limits>
#include <utility>
#include <type_traits>

namespace
{
using binding = quest_mobile_native_constructor_binding;
constexpr std::array<uint8_t, 4> magic{ 'N', 'B', 'C', '1' };
constexpr std::array<uint8_t, 4> successor_magic{ 'N', 'B', 'C', '2' };
constexpr std::array<uint8_t, 4> alchemist_magic{ 'N', 'B', 'C', '3' };
static_assert(static_cast<uint8_t>(original_alchemist_choice::not_attempted) == 0);
static_assert(static_cast<uint8_t>(original_alchemist_choice::missed) == 1);
static_assert(static_cast<uint8_t>(original_alchemist_choice::selected) == 2);
static_assert(static_cast<uint8_t>(binding::none) == 0);
static_assert(static_cast<uint8_t>(binding::thief) == 1);
static_assert(static_cast<uint8_t>(binding::teacher) == 2);
static_assert(static_cast<uint8_t>(binding::shop_keeper) == 3);
static_assert(static_cast<uint8_t>(binding::quester) == 4);
static_assert(static_cast<uint8_t>(binding::arbitrary) == 5);
static_assert(NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_BYTES ==
	      NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_BYTES + 3 * 32 + 2 * 4);
static_assert(NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES ==
	      NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_BYTES + 1 + 8);

bool binding_valid(binding value, uint16_t version) noexcept
{
	return static_cast<uint8_t>(value) <=
	       static_cast<uint8_t>(
		       (version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION ||
			version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION) ?
			       binding::arbitrary :
			       binding::quester);
}

template <typename T, size_t N> bool nonzero(const std::array<T, N> &value) noexcept
{
	return std::any_of(value.begin(), value.end(), [](T item) { return item != 0; });
}

template <size_t Capacity = NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_BYTES> struct writer
{
	std::array<uint8_t, Capacity> bytes{};
	size_t cursor = 0;

	template <typename T> void number(T value) noexcept
	{
		static_assert(std::is_unsigned_v<T>);
		for (size_t i = 0; i < sizeof(T); ++i)
			bytes[cursor++] = static_cast<uint8_t>(value >> (8 * i));
	}
	template <size_t N> void raw(const std::array<uint8_t, N> &value) noexcept
	{
		std::copy(value.begin(), value.end(), bytes.begin() + cursor);
		cursor += N;
	}
};

struct reader
{
	std::span<const uint8_t> bytes;
	size_t cursor = 0;

	template <typename T> T number() noexcept
	{
		static_assert(std::is_unsigned_v<T>);
		T value = 0;
		for (size_t i = 0; i < sizeof(T); ++i)
			value |= static_cast<T>(static_cast<T>(bytes[cursor++]) << (8 * i));
		return value;
	}
	template <size_t N> void raw(std::array<uint8_t, N> *value) noexcept
	{
		std::copy_n(bytes.begin() + cursor, N, value->begin());
		cursor += N;
	}
};
}

bool native_mobile_birth_constructor_recipe_valid(
	const quest_mobile_native_constructor_recipe &value) noexcept
{
	if ((value.wire_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_VERSION &&
	     value.wire_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION &&
	     value.wire_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION) ||
	    !binding_valid(value.binding_before, value.wire_version) ||
	    !binding_valid(value.binding_after, value.wire_version) ||
	    !binding_valid(value.quest_binding, value.wire_version) ||
	    (value.binding_before != value.binding_after &&
	     (value.binding_before != binding::none ||
	      (value.binding_after != binding::thief && value.binding_after != binding::teacher))) ||
	    !nonzero(value.build_digest) || !value.template_bytes ||
	    value.template_bytes > static_cast<uint64_t>(LONG_MAX) ||
	    !nonzero(value.random.initial) || !nonzero(value.random.terminal) ||
	    (!value.random.draws && value.random.initial != value.random.terminal))
		return false;
	if (value.wire_version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_VERSION)
	{
		if (nonzero(value.procedure_before) || nonzero(value.procedure_after) ||
		    nonzero(value.reset_tail) || value.reset_room_vnum != 0 ||
		    value.reset_shop_index != -1)
			return false;
	}
	else if (!nonzero(value.procedure_before) || !nonzero(value.procedure_after) ||
		 !nonzero(value.reset_tail))
		return false;
	if (value.wire_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION)
	{
		if (value.alchemist_choice != original_alchemist_choice::not_attempted ||
		    value.alchemist_grant_uid != 0)
			return false;
	}
	else if (static_cast<uint8_t>(value.alchemist_choice) >
			 static_cast<uint8_t>(original_alchemist_choice::selected) ||
		 (value.alchemist_choice != original_alchemist_choice::selected &&
		  value.alchemist_grant_uid != 0) ||
		 value.alchemist_grant_uid == std::numeric_limits<uint64_t>::max())
		return false;
	for (const int64_t clock : value.clock_values)
		if (static_cast<int64_t>(static_cast<time_t>(clock)) != clock)
			return false;
	return true;
}

bool native_mobile_birth_constructor_recipe_encode(
	const quest_mobile_native_constructor_recipe &value,
	native_mobile_birth_constructor_recipe_bytes *output) noexcept
{
	if (!output || value.wire_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_VERSION ||
	    !native_mobile_birth_constructor_recipe_valid(value))
		return false;
	writer<> out;
	out.raw(magic);
	out.number<uint16_t>(NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_VERSION);
	out.number<uint16_t>(0);
	out.number<uint32_t>(static_cast<uint32_t>(value.mobile_vnum));
	out.number<uint32_t>(static_cast<uint32_t>(value.constructor_birthplace_vnum));
	out.number<uint8_t>(value.apply_mob_gold ? 1 : 0);
	out.number<uint8_t>(static_cast<uint8_t>(value.binding_before));
	out.number<uint8_t>(static_cast<uint8_t>(value.binding_after));
	out.number<uint8_t>(static_cast<uint8_t>(value.quest_binding));
	for (const uint64_t state : value.random.initial)
		out.number<uint64_t>(state);
	for (const uint64_t state : value.random.terminal)
		out.number<uint64_t>(state);
	out.number<uint64_t>(value.random.draws);
	for (const int64_t clock : value.clock_values)
		out.number<uint64_t>(static_cast<uint64_t>(clock));
	out.number<uint64_t>(value.template_bytes);
	out.raw(value.build_digest);
	out.raw(value.template_digest);
	out.raw(value.effective_inputs_digest);
	out.raw(value.cached_strings_digest);
	for (const auto &digest : value.string_digests)
		out.raw(digest);
	if (out.cursor != out.bytes.size())
		return false;
	*output = out.bytes;
	return true;
}

bool native_mobile_birth_constructor_recipe_encode_blob(
	const quest_mobile_native_constructor_recipe &value, std::vector<uint8_t> *output) noexcept
{
	if (!output || !native_mobile_birth_constructor_recipe_valid(value))
		return false;
	try
	{
		if (value.wire_version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_VERSION)
		{
			native_mobile_birth_constructor_recipe_bytes bytes;
			if (!native_mobile_birth_constructor_recipe_encode(value, &bytes))
				return false;
			std::vector<uint8_t> candidate(bytes.begin(), bytes.end());
			*output = std::move(candidate);
			return true;
		}
		if (value.wire_version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION)
		{
			writer<NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES> out;
			out.raw(alchemist_magic);
			out.number<uint16_t>(
				NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION);
			out.number<uint16_t>(0);
			out.number<uint32_t>(static_cast<uint32_t>(value.mobile_vnum));
			out.number<uint32_t>(
				static_cast<uint32_t>(value.constructor_birthplace_vnum));
			out.number<uint8_t>(value.apply_mob_gold ? 1 : 0);
			out.number<uint8_t>(static_cast<uint8_t>(value.binding_before));
			out.number<uint8_t>(static_cast<uint8_t>(value.binding_after));
			out.number<uint8_t>(static_cast<uint8_t>(value.quest_binding));
			for (const uint64_t state : value.random.initial)
				out.number<uint64_t>(state);
			for (const uint64_t state : value.random.terminal)
				out.number<uint64_t>(state);
			out.number<uint64_t>(value.random.draws);
			for (const int64_t clock : value.clock_values)
				out.number<uint64_t>(static_cast<uint64_t>(clock));
			out.number<uint64_t>(value.template_bytes);
			out.raw(value.build_digest);
			out.raw(value.template_digest);
			out.raw(value.effective_inputs_digest);
			out.raw(value.cached_strings_digest);
			for (const auto &digest : value.string_digests)
				out.raw(digest);
			out.raw(value.procedure_before);
			out.raw(value.procedure_after);
			out.raw(value.reset_tail);
			out.number<uint32_t>(static_cast<uint32_t>(value.reset_room_vnum));
			out.number<uint32_t>(static_cast<uint32_t>(value.reset_shop_index));
			out.number<uint8_t>(static_cast<uint8_t>(value.alchemist_choice));
			out.number<uint64_t>(value.alchemist_grant_uid);
			if (out.cursor != out.bytes.size())
				return false;
			std::vector<uint8_t> candidate(out.bytes.begin(), out.bytes.end());
			*output = std::move(candidate);
			return true;
		}
		writer<NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_BYTES> out;
		out.raw(successor_magic);
		out.number<uint16_t>(NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION);
		out.number<uint16_t>(0);
		out.number<uint32_t>(static_cast<uint32_t>(value.mobile_vnum));
		out.number<uint32_t>(static_cast<uint32_t>(value.constructor_birthplace_vnum));
		out.number<uint8_t>(value.apply_mob_gold ? 1 : 0);
		out.number<uint8_t>(static_cast<uint8_t>(value.binding_before));
		out.number<uint8_t>(static_cast<uint8_t>(value.binding_after));
		out.number<uint8_t>(static_cast<uint8_t>(value.quest_binding));
		for (const uint64_t state : value.random.initial)
			out.number<uint64_t>(state);
		for (const uint64_t state : value.random.terminal)
			out.number<uint64_t>(state);
		out.number<uint64_t>(value.random.draws);
		for (const int64_t clock : value.clock_values)
			out.number<uint64_t>(static_cast<uint64_t>(clock));
		out.number<uint64_t>(value.template_bytes);
		out.raw(value.build_digest);
		out.raw(value.template_digest);
		out.raw(value.effective_inputs_digest);
		out.raw(value.cached_strings_digest);
		for (const auto &digest : value.string_digests)
			out.raw(digest);
		out.raw(value.procedure_before);
		out.raw(value.procedure_after);
		out.raw(value.reset_tail);
		out.number<uint32_t>(static_cast<uint32_t>(value.reset_room_vnum));
		out.number<uint32_t>(static_cast<uint32_t>(value.reset_shop_index));
		if (out.cursor != out.bytes.size())
			return false;
		std::vector<uint8_t> candidate(out.bytes.begin(), out.bytes.end());
		*output = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	catch (...)
	{
		return false;
	}
}

bool native_mobile_birth_constructor_recipe_decode(
	std::span<const uint8_t> bytes, quest_mobile_native_constructor_recipe *output) noexcept
{
	if (!output || (bytes.size() != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_BYTES &&
			bytes.size() != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_BYTES &&
			bytes.size() != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES))
		return false;
	reader in{ bytes };
	std::array<uint8_t, 4> actual_magic{};
	in.raw(&actual_magic);
	const uint16_t version = in.number<uint16_t>();
	if (in.number<uint16_t>() != 0 ||
	    !((actual_magic == magic && version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_VERSION &&
	       bytes.size() == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_BYTES) ||
	      (actual_magic == successor_magic &&
	       version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION &&
	       bytes.size() == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_BYTES) ||
	      (actual_magic == alchemist_magic &&
	       version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION &&
	       bytes.size() == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES)))
		return false;
	quest_mobile_native_constructor_recipe value;
	value.wire_version = version;
	value.mobile_vnum = std::bit_cast<int32_t>(in.number<uint32_t>());
	value.constructor_birthplace_vnum = std::bit_cast<int32_t>(in.number<uint32_t>());
	const uint8_t apply_gold = in.number<uint8_t>();
	if (apply_gold > 1)
		return false;
	value.apply_mob_gold = apply_gold != 0;
	value.binding_before = static_cast<binding>(in.number<uint8_t>());
	value.binding_after = static_cast<binding>(in.number<uint8_t>());
	value.quest_binding = static_cast<binding>(in.number<uint8_t>());
	for (auto &state : value.random.initial)
		state = in.number<uint64_t>();
	for (auto &state : value.random.terminal)
		state = in.number<uint64_t>();
	value.random.draws = in.number<uint64_t>();
	for (auto &clock : value.clock_values)
		clock = std::bit_cast<int64_t>(in.number<uint64_t>());
	value.template_bytes = in.number<uint64_t>();
	in.raw(&value.build_digest);
	in.raw(&value.template_digest);
	in.raw(&value.effective_inputs_digest);
	in.raw(&value.cached_strings_digest);
	for (auto &digest : value.string_digests)
		in.raw(&digest);
	if (version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION ||
	    version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION)
	{
		in.raw(&value.procedure_before);
		in.raw(&value.procedure_after);
		in.raw(&value.reset_tail);
		value.reset_room_vnum = std::bit_cast<int32_t>(in.number<uint32_t>());
		value.reset_shop_index = std::bit_cast<int32_t>(in.number<uint32_t>());
	}
	if (version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION)
	{
		value.alchemist_choice =
			static_cast<original_alchemist_choice>(in.number<uint8_t>());
		value.alchemist_grant_uid = in.number<uint64_t>();
	}
	if (in.cursor != bytes.size() || !native_mobile_birth_constructor_recipe_valid(value))
		return false;
	*output = value;
	return true;
}

#include <cerrno>

namespace
{
bool constructor_codec_admit(size_t outer, size_t working, bool (*reserve)(size_t, void *) noexcept,
			     void *context) noexcept
{
	if (working > SIZE_MAX - outer || !reserve || !reserve(outer + working, context))
	{
		errno = ENOBUFS;
		return false;
	}
	return true;
}
}

bool native_mobile_birth_constructor_recipe_encode_blob_bounded(
	const quest_mobile_native_constructor_recipe &value, std::vector<uint8_t> *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	if (!output || !native_mobile_birth_constructor_recipe_valid(value))
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)reserve;
	(void)context;
	(void)outer_live;
	errno = ENOTSUP;
	return false;
#else
	size_t working = 0;
	if (value.wire_version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_VERSION)
	{
		// The fixed bytes survive the nested encoder's writer, and then the
		// fresh range-constructed vector. Those two phases are sequential.
		working = sizeof(native_mobile_birth_constructor_recipe_bytes) +
			  std::max(sizeof(writer<>),
				   sizeof(std::vector<uint8_t>) +
					   NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_BYTES);
	}
	else if (value.wire_version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION)
		working = sizeof(writer<NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES>) +
			  sizeof(std::vector<uint8_t>) +
			  NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES;
	else
		working = sizeof(writer<NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_BYTES>) +
			  sizeof(std::vector<uint8_t>) +
			  NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_BYTES;
	if (!constructor_codec_admit(outer_live, working, reserve, context))
		return false;
	// The original range constructor requests precisely the selected wire
	// length on the pinned policy; its complete validation/wire is unchanged.
	return native_mobile_birth_constructor_recipe_encode_blob(value, output);
#endif
}

bool native_mobile_birth_constructor_recipe_decode_bounded(
	const std::span<const uint8_t> &bytes, quest_mobile_native_constructor_recipe *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	if (!output || (bytes.size() != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_BYTES &&
			bytes.size() != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_BYTES &&
			bytes.size() != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES))
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)reserve;
	(void)context;
	(void)outer_live;
	errno = ENOTSUP;
	return false;
#else
	// The actual original by-value span, reader, magic and candidate coexist.
	// Decode allocates no heap; admission also protects its named inline DTOs.
	constexpr size_t working = sizeof(std::span<const uint8_t>) + sizeof(reader) +
				   sizeof(std::array<uint8_t, 4>) +
				   sizeof(quest_mobile_native_constructor_recipe);
	if (!constructor_codec_admit(outer_live, working, reserve, context))
		return false;
	return native_mobile_birth_constructor_recipe_decode(bytes, output);
#endif
}

#include "economy/economic_accounting_types.h"

economic_accounting_error native_mobile_birth_constructor_recipe_decode_status_bounded(
	const std::span<const uint8_t> &bytes, quest_mobile_native_constructor_recipe *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	using status = economic_accounting_error;
	// Identical original null/length shape validation before any admission.
	if (!output || (bytes.size() != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_BYTES &&
			bytes.size() != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_BYTES &&
			bytes.size() != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES))
		return status::corrupt_evidence;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)reserve;
	(void)context;
	(void)outer_live;
	return status::unresolved;
#else
	// Full original decoder's genuinely simultaneous by-value span, reader,
	// magic, and complete fixed-value candidate; no heap is allocated here.
	constexpr size_t objects = sizeof(std::span<const uint8_t>) + sizeof(reader) +
				   sizeof(std::array<uint8_t, 4>) +
				   sizeof(quest_mobile_native_constructor_recipe);
	// Actual status/decode parameter and return carriers, version/apply_gold,
	// three range references; reader number's this/value/i/return and raw's
	// this/value parameters. These are source scopes, not emitted stack.
	constexpr size_t decoder_frames =
		4 * sizeof(void *) + sizeof(size_t) + sizeof(status) + sizeof(bool) +
		2 * sizeof(void *) + sizeof(uint16_t) + sizeof(uint8_t) + 3 * sizeof(void *) +
		sizeof(void *) + 2 * sizeof(uint64_t) + sizeof(size_t) + 2 * sizeof(void *);
	// Actual fixed array copy_n/copy/source scopes (iterators, count/result,
	// runtime tag, memmove arguments), bit_cast's source/value return, and
	// scalar reader's index/arithmetic/query carriers. No byte buffer copy.
	constexpr size_t transport_frames = 4 * (3 * sizeof(void *) + sizeof(void *)) +
					    4 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(char) +
					    3 * sizeof(void *) + sizeof(size_t) + sizeof(void *) +
					    2 * sizeof(uint64_t) + sizeof(bool);
	// Full original valid() parameter, clock iteration/local cast and result;
	// binding_valid's value/version; nonzero()/any_of/find_if predicate/range
	// carriers and the actual uint64_t predicate item. The array values remain
	// inline in the genuine candidate already counted above.
	constexpr size_t validation_frames =
		2 * sizeof(void *) + sizeof(int64_t) + sizeof(time_t) + sizeof(bool) +
		sizeof(binding) + sizeof(uint16_t) + sizeof(bool) + 2 * sizeof(void *) +
		3 * sizeof(void *) + sizeof(uint64_t) + 4 * (3 * sizeof(void *) + sizeof(bool)) +
		2 * sizeof(size_t) + 3 * sizeof(char);
	constexpr size_t working = objects + decoder_frames + transport_frames + validation_frames;
	// Return the failure directly, including checked-add refusal before the
	// callback. No errno/global-state inference can turn capacity into corrupt
	// evidence. The callback owns admission only; no authority is granted.
	if (working > SIZE_MAX - outer_live || !reserve || !reserve(outer_live + working, context))
		return status::capacity;
	// Original complete NBC1/NBC2/NBC3 magic/header/version/body/semantic decode
	// remains authoritative and preserves output on any malformed evidence.
	return native_mobile_birth_constructor_recipe_decode(bytes, output) ?
		       status::ok :
		       status::corrupt_evidence;
#endif
}

#include <iterator>

namespace
{
bool fixed_constructor_profile_supported() noexcept
{
#if defined(__linux__) && defined(__x86_64__) && defined(__GNUC__) && __GNUC__ == 13 &&    \
	!defined(__clang__) && __cplusplus == 202002L && defined(_GLIBCXX_RELEASE) &&      \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&                       \
	_GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG) &&                         \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                    \
	!(defined(_GLIBCXX_SANITIZE_STD_ALLOCATOR) && defined(_GLIBCXX_SANITIZE_VECTOR) && \
	  _GLIBCXX_SANITIZE_STD_ALLOCATOR && _GLIBCXX_SANITIZE_VECTOR)
	return sizeof(void *) == 8 && sizeof(size_t) == 8 && sizeof(std::ptrdiff_t) == 8 &&
	       sizeof(long) == 8 && sizeof(time_t) == 8;
#else
	return false;
#endif
}

// Genuine primitive algorithm closures. Definitions/control pins are shared
// explicitly with the result owner packet; no broad unrelated allowance.
constexpr size_t result_valid_frames =
	// Original valid(result), nonzero(digest), empty local closure, any_of ->
	// none_of -> find_if -> both __find_if levels, RA trip count/tag, wrapper
	// predicate construction/call and actual byte/results; array begin/end.
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(bool) +
	3 * (2 * sizeof(void *) + sizeof(char) + sizeof(bool)) +
	2 * (3 * sizeof(void *) + sizeof(char)) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 6 * sizeof(void *) + 4 * sizeof(char) +
	3 * sizeof(bool) + sizeof(uint8_t) + 4 * (2 * sizeof(void *));

constexpr size_t result_equal_frames =
	// Original equal(left,right) and array operator==; std::equal/_equal_aux/
	// _equal_aux1/_equal byte specialization, iterator wrappers and memcmp.
	4 * sizeof(void *) + 2 * sizeof(bool) + 4 * (3 * sizeof(void *) + sizeof(bool)) +
	sizeof(size_t) + sizeof(bool) + 3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) +
	sizeof(size_t) + sizeof(int) + 3 * (2 * sizeof(void *));

constexpr size_t result_copy_frames =
	// Original copy_n -> copy_n(RA) -> copy/copy_move_a/a1/a2/copy_m plus
	// iterator normalization/wrapping and the actual memcpy/memmove leaf.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(std::random_access_iterator_tag) +
	2 * sizeof(size_t) + 5 * (3 * sizeof(void *) + sizeof(void *)) +
	2 * (sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	2 * sizeof(void *) + sizeof(void *) + 2 * sizeof(void *) + 3 * sizeof(void *) +
	sizeof(size_t) + sizeof(std::ptrdiff_t) +
	// Span input size/data/begin and both actual array begin/size accessors.
	4 * (sizeof(void *) + sizeof(size_t)) + 8 * (2 * sizeof(void *));

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
	return destruction + element + deallocation;
}

template <class T> constexpr size_t result_vector_lifetime_frames() noexcept
{
	return sizeof(std::vector<T> *) + 2 * sizeof(void *) + 2 * sizeof(std::allocator<T> *) +
	       sizeof(void *) + result_vector_cleanup_frames<T>();
}

constexpr size_t fixed_constructor_scalar_frames =
	// writer::number receiver/value/i; reader::number receiver/value/i/result;
	// writer::raw and reader::raw receiver/array-reference; actual array/span
	// indexing and size/data begin/end wrappers and iterator arithmetic.
	2 * sizeof(void *) + 3 * sizeof(uint64_t) + 2 * sizeof(size_t) + 4 * sizeof(void *) +
	6 * (2 * sizeof(void *)) + 4 * (2 * sizeof(void *) + sizeof(size_t)) +
	// bit_cast's actual source reference and largest 64-bit return; binding
	// validation's two typed parameters and boolean; numeric_limits result.
	sizeof(void *) + sizeof(uint64_t) + sizeof(binding) + sizeof(uint16_t) + sizeof(bool) +
	sizeof(uint64_t);

constexpr size_t fixed_constructor_valid_frames =
	// Original recipe validation receiver, clock range and true desugared
	// range/begin/end carriers, clock and cast time_t, returned boolean.
	4 * sizeof(void *) + sizeof(int64_t) + sizeof(time_t) + sizeof(bool) +
	// nonzero's full algorithm chain is shared with the result provider, but
	// this actual predicate receives uint64_t for RNG state, not just uint8_t.
	result_valid_frames - sizeof(void *) - sizeof(char) - sizeof(bool) + sizeof(uint64_t) -
	sizeof(uint8_t) +
	// Original random.initial != terminal array equality (64-bit specialization
	// uses the same genuine pointer/count memcmp chain) and digest range.
	result_equal_frames - 2 * sizeof(void *) - sizeof(bool) + 4 * sizeof(void *) +
	// Actual binding validator/numeric_limits leaves reached by valid only.
	sizeof(binding) + sizeof(uint16_t) + sizeof(bool) + sizeof(uint64_t);

constexpr size_t fixed_constructor_vector_frames =
	// Range vector ctor this/first/last/allocator; default allocator and base/
	// impl/data receivers; _M_range_initialize first/last/tag and receiver.
	4 * sizeof(void *) + 6 * sizeof(void *) + 3 * sizeof(void *) +
	sizeof(std::forward_iterator_tag) +
	// distance -> __distance(RA), category and returned difference.
	2 * (2 * sizeof(void *) + sizeof(std::ptrdiff_t)) +
	sizeof(std::random_access_iterator_tag) + 2 * sizeof(void *) +
	// _M_allocate/allocator_traits/allocator/new_allocator, true C++20 constant
	// evaluation result, size/max_size/check_init_len and actual new args/result.
	4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(bool) +
	3 * (sizeof(void *) + sizeof(size_t)) + sizeof(size_t) + sizeof(void *) +
	// uninitialized_copy -> _aux -> trivial copy with real temporary iterator
	// carriers. Copy's full transport leaf is separately selected below.
	3 * (3 * sizeof(void *) + sizeof(void *)) + sizeof(bool) +
	// Move assignment -> base allocator-equality branch -> _M_move_assign(true)
	// temporary vector/_M_swap_data actual data receivers/copy; old output
	// capacity is already caller-owned and remains so through real deallocation.
	6 * sizeof(void *) + 2 * sizeof(bool) + sizeof(std::true_type) + 6 * sizeof(void *) +
	3 * sizeof(void *) + 3 * sizeof(void *) + sizeof(std::allocator<uint8_t>) +
	sizeof(std::vector<uint8_t>) + result_vector_lifetime_frames<uint8_t>();

constexpr size_t fixed_constructor_entry_frames =
	// Fixed bool and status wrappers, original bounded wrapper, original blob/
	// fixed encode/decode signatures/results and actual working/base variables.
	5 * (4 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	sizeof(economic_accounting_error) + 4 * sizeof(size_t) +
	// Original encode version branches, raw digest desugared iteration, decoder
	// version/apply_gold/loop references and both return/output assignment paths.
	2 * sizeof(void *) + 4 * sizeof(void *) + sizeof(uint16_t) + sizeof(uint8_t) +
	4 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(bool) +
	// Original constructor_codec_admit and fixed wrapper checked addition.
	2 * sizeof(size_t) + 2 * sizeof(void *) + sizeof(bool) + sizeof(size_t *) + sizeof(size_t) +
	sizeof(bool);

constexpr size_t fixed_constructor_source_frames =
	fixed_constructor_entry_frames + fixed_constructor_valid_frames +
	fixed_constructor_scalar_frames + fixed_constructor_vector_frames +
	std::max(result_copy_frames, result_equal_frames);

constexpr size_t fixed_constructor_decode_inline = sizeof(std::span<const uint8_t>) +
						   sizeof(reader) + sizeof(std::array<uint8_t, 4>) +
						   sizeof(quest_mobile_native_constructor_recipe);
constexpr size_t fixed_constructor_encode_initial_inline =
	// Actual largest original writer + range-constructed vector and request.
	// Its temporary default allocator is genuine, not a padding margin.
	sizeof(writer<NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES>) +
	sizeof(std::vector<uint8_t>) + NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES +
	sizeof(std::allocator<uint8_t>);
}

bool native_mobile_birth_constructor_recipe_own_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !fixed_constructor_profile_supported())
		return false;
	*output = fixed_constructor_source_frames;
	return true;
}

bool native_mobile_birth_constructor_recipe_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !fixed_constructor_profile_supported())
		return false;
	*output = fixed_constructor_encode_initial_inline > fixed_constructor_decode_inline ?
			  fixed_constructor_encode_initial_inline :
			  fixed_constructor_decode_inline;
	return true;
}

bool native_mobile_birth_constructor_recipe_valid_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !fixed_constructor_profile_supported())
		return false;
	*output = fixed_constructor_valid_frames;
	return true;
}

bool native_mobile_birth_constructor_recipe_encode_blob_fixed_bounded(
	const quest_mobile_native_constructor_recipe &value, std::vector<uint8_t> *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	if (!fixed_constructor_profile_supported())
	{
		errno = ENOTSUP;
		return false;
	}
	// The unchanged old encoder supplies its exact version-specific writer,
	// range-vector/private request admission. Add only missing SOURCE and the
	// real temporary allocator; keep old object/request bytes out of outer.
	constexpr size_t supplement =
		fixed_constructor_source_frames + sizeof(std::allocator<uint8_t>);
	if (supplement > SIZE_MAX - outer || !reserve || !reserve(outer + supplement, context))
	{
		errno = ENOBUFS;
		return false;
	}
	return native_mobile_birth_constructor_recipe_encode_blob_bounded(
		value, output, reserve, context, outer + supplement);
}

economic_accounting_error native_mobile_birth_constructor_recipe_decode_status_fixed_bounded(
	const std::span<const uint8_t> &bytes, quest_mobile_native_constructor_recipe *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	using status = economic_accounting_error;
	// Preserve the original typed decoder's null/shape refusal before the
	// unsupported-profile branch. No candidate is constructed on these paths.
	if (!output || (bytes.size() != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_BYTES &&
			bytes.size() != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_BYTES &&
			bytes.size() != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES))
		return status::corrupt_evidence;
	if (!fixed_constructor_profile_supported())
		return status::unresolved;
	constexpr size_t supplement =
		fixed_constructor_source_frames + fixed_constructor_decode_inline;
	if (supplement > SIZE_MAX - outer || !reserve || !reserve(outer + supplement, context))
		return status::capacity;
	// The original full allocation-free decoder retains exact NBC1/2/3 magic,
	// size, fields, validators and strong output. No old partial source charge
	// is added again; its genuine private objects are admitted above.
	return native_mobile_birth_constructor_recipe_decode(bytes, output) ?
		       status::ok :
		       status::corrupt_evidence;
}

bool native_mobile_birth_constructor_recipe_decode_fixed_bounded(
	const std::span<const uint8_t> &bytes, quest_mobile_native_constructor_recipe *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	const auto status = native_mobile_birth_constructor_recipe_decode_status_fixed_bounded(
		bytes, output, reserve, context, outer);
	if (status == economic_accounting_error::capacity)
		errno = ENOBUFS;
	else if (status == economic_accounting_error::unresolved)
		errno = ENOTSUP;
	return status == economic_accounting_error::ok;
}
