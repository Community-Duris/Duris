// Query the genuine complete DB provider; section GC omits uncalled operations.
// This fixture supplies no world, source, binding, SQL or publication authority.
#ifndef EXPECT_NATIVE_SOURCE_SUPPORTED
#error Select the supported or assertions-refusal profile.
#endif
#include "world/object_template.h"
#include "world/quest_mobile_native_birth.h"
#include "world/zone_reset_room_publication.h"
#include <array>
#include <cerrno>
#include <type_traits>

// The actual providers are separate complete production translation units.
// No source inclusion, private access, substitute observer or child stub.
bool quest_mobile_native_item_retained_observation_source_frame_bytes(size_t *) noexcept;
bool quest_mobile_native_item_retained_observation_initial_inline_bytes(size_t *) noexcept;
size_t quest_mobile_native_item_retained_observation_source_query_frame_bytes() noexcept;
size_t quest_mobile_native_item_retained_observation_initial_query_frame_bytes() noexcept;
bool quest_mobile_native_mobile_retained_observation_source_frame_bytes(size_t *) noexcept;
size_t quest_mobile_native_mobile_retained_observation_source_query_frame_bytes() noexcept;
#include <cassert>
#include <cstdio>
#ifdef NDEBUG
#error Fixture assertions must be enabled.
#endif

namespace
{
using passive_getter = bool (*)(size_t *) noexcept;
constexpr size_t profile_sentinel = 123456789;
constexpr bool profile_supported = EXPECT_NATIVE_SOURCE_SUPPORTED != 0;
void check_profile(const char *name, passive_getter getter, size_t expected)
{
	errno = ERANGE;
	const bool null_result = getter(nullptr);
	const int null_errno = errno;
	if (null_result || null_errno != (profile_supported ? EINVAL : ENOTSUP))
		std::fprintf(stderr, "%s null: supported=%d result=%d errno=%d expected_errno=%d\n",
			     name, int(profile_supported), int(null_result), null_errno,
			     profile_supported ? EINVAL : ENOTSUP);
	assert(!null_result);
	assert(null_errno == (profile_supported ? EINVAL : ENOTSUP));
	for (int repeat = 0; repeat != 2; ++repeat)
	{
		size_t value = profile_sentinel;
		errno = ERANGE;
		const bool result = getter(&value);
		const int result_errno = errno;
		if (result != profile_supported ||
		    value != (profile_supported ? expected : profile_sentinel) ||
		    result_errno != (profile_supported ? ERANGE : ENOTSUP))
			std::fprintf(stderr,
				     "%s value: supported=%d result=%d value=%zu expected=%zu "
				     "errno=%d expected_errno=%d\n",
				     name, int(profile_supported), int(result), value,
				     profile_supported ? expected : profile_sentinel, result_errno,
				     profile_supported ? ERANGE : ENOTSUP);
		assert(result == profile_supported);
		assert(value == (profile_supported ? expected : profile_sentinel));
		assert(result_errno == (profile_supported ? ERANGE : ENOTSUP));
	}
}
}

int main()
{
#ifdef TEST_NATIVE_BIRTH_CURRENT_PROFILE
	size_t room = 0;
	if (profile_supported)
		assert(zone_reset_room_publication_owner::
			       retained_registered_literal_pool_source_frame_bytes(&room));
	// Compare the complete birth provider with the normative independently
	// frozen DB child profiles, joined to the genuine shared ROOM provider.
	check_profile("quest_mobile_native_birth_current_source_frames",
		      quest_mobile_native_birth_current_source_frames,
		      2118 + room + 274 + 1327 + 632);
	check_profile("quest_mobile_native_birth_current_initial_inline_bytes",
		      quest_mobile_native_birth_current_initial_inline_bytes, 32);
	for (int repeat = 0; repeat != 2; ++repeat)
	{
		errno = ERANGE;
		const size_t source_query =
			quest_mobile_native_birth_current_source_profile_query_frames();
		const size_t expected_source_query =
			profile_supported ?
				125 +
					zone_reset_room_publication_owner::
						retained_registered_literal_pool_source_query_frame_bytes() +
					73 + 27 + 27 :
				SIZE_MAX;
		if (source_query != expected_source_query)
			std::fprintf(stderr,
				     "Birth Source query: supported=%d value=%zu expected=%zu\n",
				     int(profile_supported), source_query, expected_source_query);
		assert(source_query == expected_source_query);
		assert(errno == ERANGE);
		const size_t initial_query =
			quest_mobile_native_birth_current_initial_query_frames();
		const size_t expected_initial_query =
			profile_supported ?
				67 + 27 +
					zone_reset_room_publication_owner::
						retained_registered_literal_pool_initial_query_frame_bytes() :
				SIZE_MAX;
		if (initial_query != expected_initial_query)
			std::fprintf(stderr,
				     "Birth INITIAL query: supported=%d value=%zu expected=%zu\n",
				     int(profile_supported), initial_query, expected_initial_query);
		assert(initial_query == expected_initial_query);
		assert(errno == ERANGE);
	}
	std::printf("NATIVE_BIRTH_CURRENT_PROFILE {\"supported\":%s,\"getters\":2,"
		    "\"queries\":2,\"null_refusals\":2,\"strong_outputs\":4,"
		    "\"initial\":%zu}\n",
		    profile_supported ? "true" : "false",
		    profile_supported ? size_t{ 32 } : size_t{ 0 });
	return 0;
#else
	// These are declared-source inventories, not synthetic native retention.
	// Both genuine stage observer bodies and their selected source helpers are
	// compiled and linked in full. Real nonempty construction/census belongs to the separate
	// owned runtime qualification and is not claimed from these passive values.
	check_profile("quest_mobile_native_item_retained_observation_source_frame_bytes",
		      quest_mobile_native_item_retained_observation_source_frame_bytes, 1327);
	check_profile("quest_mobile_native_item_retained_observation_initial_inline_bytes",
		      quest_mobile_native_item_retained_observation_initial_inline_bytes, 32);
	check_profile("quest_mobile_native_mobile_retained_observation_source_frame_bytes",
		      quest_mobile_native_mobile_retained_observation_source_frame_bytes, 632);
	errno = ERANGE;
	assert(quest_mobile_native_item_retained_observation_source_query_frame_bytes() ==
	       (profile_supported ? 27 : SIZE_MAX));
	assert(quest_mobile_native_item_retained_observation_initial_query_frame_bytes() ==
	       (profile_supported ? 27 : SIZE_MAX));
	assert(quest_mobile_native_mobile_retained_observation_source_query_frame_bytes() ==
	       (profile_supported ? 27 : SIZE_MAX));
	assert(errno == ERANGE);
	// Protected strings INLINE has an actual trivial four-pointer array; its
	// declaration is excluded from the new Source, with no constructor/copy.
	static_assert(sizeof(std::array<char *, 4>) == 32);
	static_assert(std::is_trivially_copyable_v<std::array<char *, 4>>);
	std::printf("NATIVE_DB_RETENTION_PROFILE {\"supported\":%s,\"getters\":3,"
		    "\"queries\":3,\"null_refusals\":3,\"strong_outputs\":6,"
		    "\"initial\":%zu}\n",
		    profile_supported ? "true" : "false",
		    profile_supported ? size_t{ 32 } : size_t{ 0 });

	constexpr bool supported = EXPECT_NATIVE_SOURCE_SUPPORTED != 0;
	constexpr size_t sentinel = 123456789;
	using binding = shop_trade_original_procedure_binding_stage;
	static_assert(sizeof(void *) == 8 && sizeof(size_t) == 8 && sizeof(bool) == 1);
	errno = ERANGE;
	assert(!binding::retained_source_frame_bytes(nullptr));
	assert(errno == (supported ? EINVAL : ENOTSUP));
	for (int repeat = 0; repeat < 2; ++repeat)
	{
		size_t source = sentinel;
		errno = ERANGE;
		assert(binding::retained_source_frame_bytes(&source) == supported);
		// The supported profile is a normative source-declaration inventory,
		// independently selected from the original binding/chain/scope bodies.
		assert(source == (supported ? 274 : sentinel));
		assert(errno == (supported ? ERANGE : ENOTSUP));
		errno = ERANGE;
		assert(binding::retained_source_query_frame_bytes() == (supported ? 73 : SIZE_MAX));
		assert(errno == ERANGE);
	}
	std::printf("NATIVE_BINDING_SOURCE_PROFILE {\"supported\":%s,"
		    "\"source\":%zu,\"query\":%zu,\"strong_outputs\":2,"
		    "\"null_refusals\":1}\n",
		    supported ? "true" : "false", supported ? size_t{ 274 } : size_t{ 0 },
		    binding::retained_source_query_frame_bytes());
	return 0;
#endif
}
