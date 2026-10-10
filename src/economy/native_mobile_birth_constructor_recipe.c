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
