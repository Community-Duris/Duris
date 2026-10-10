#include "economy/native_mobile_birth_cash_role_recipe.h"

#include <algorithm>
#include <new>
#include <utility>

bool native_mobile_birth_cash_role_recipe_valid(
	const native_mobile_birth_cash_role_recipe &value) noexcept
{
	if (value.original.wire_version !=
		    NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION ||
	    !native_mobile_birth_constructor_recipe_valid(value.original) ||
	    value.original.reset_room_vnum <= 0)
		return false;
	return (value.role == native_mobile_birth_cash_role::ordinary_wallet &&
		value.configured_shop_matches == 0 && value.original.reset_shop_index == -1) ||
	       (value.role == native_mobile_birth_cash_role::shared_shopkeeper &&
		value.configured_shop_matches == 1 && value.original.reset_shop_index >= 0);
}

bool native_mobile_birth_cash_role_recipe_encode(
	const native_mobile_birth_cash_role_recipe &value,
	native_mobile_birth_cash_role_recipe_bytes *output) noexcept
{
	if (!output || !native_mobile_birth_cash_role_recipe_valid(value))
		return false;
	try
	{
		std::vector<uint8_t> original;
		if (!native_mobile_birth_constructor_recipe_encode_blob(value.original,
									&original) ||
		    original.size() != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES)
			return false;
		native_mobile_birth_cash_role_recipe_bytes candidate{};
		candidate[0] = 'N';
		candidate[1] = 'B';
		candidate[2] = 'C';
		candidate[3] = '4';
		candidate[4] = NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_VERSION;
		std::copy(original.begin(), original.end(), candidate.begin() + 8);
		constexpr size_t role_offset =
			8 + NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES;
		candidate[role_offset] = static_cast<uint8_t>(value.role);
		for (size_t i = 0; i < 4; ++i)
			candidate[role_offset + 1 + i] =
				static_cast<uint8_t>(value.configured_shop_matches >> (8 * i));
		*output = candidate;
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

bool native_mobile_birth_cash_role_recipe_decode(
	std::span<const uint8_t> bytes, native_mobile_birth_cash_role_recipe *output) noexcept
{
	if (!output || bytes.size() != NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_BYTES ||
	    bytes[0] != 'N' || bytes[1] != 'B' || bytes[2] != 'C' || bytes[3] != '4' ||
	    bytes[4] != NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_VERSION || bytes[5] || bytes[6] ||
	    bytes[7])
		return false;
	native_mobile_birth_cash_role_recipe candidate;
	if (!native_mobile_birth_constructor_recipe_decode(
		    bytes.subspan(8, NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES),
		    &candidate.original))
		return false;
	constexpr size_t role_offset = 8 + NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES;
	candidate.role = static_cast<native_mobile_birth_cash_role>(bytes[role_offset]);
	candidate.configured_shop_matches = 0;
	for (size_t i = 0; i < 4; ++i)
		candidate.configured_shop_matches |= uint32_t(bytes[role_offset + 1 + i])
						     << (8 * i);
	if (!native_mobile_birth_cash_role_recipe_valid(candidate))
		return false;
	*output = candidate;
	return true;
}

#include "economy/economic_accounting_types.h"
#include <type_traits>
#include <iterator>

namespace
{
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

using role_error = economic_accounting_error;
using role_reserve_fn = bool (*)(size_t, void *) noexcept;

struct role_fixed_workspace
{
	std::vector<uint8_t> original;
	native_mobile_birth_cash_role_recipe_bytes candidate{};
	size_t constructor_source = 0, constructor_inline = 0, constructor_valid = 0;
};

constexpr size_t role_fixed_local_source =
	// Encode signature (role/output/reserve/context/outer), six named scalar
	// locals source/valid/base/child_outer/role_offset/i and return status.
	4 * sizeof(void *) + 7 * sizeof(size_t) + sizeof(role_error) +
	// Decode signature and eight named scalar locals; actual embedded span and
	// candidate DTO are entry objects below, not counted as source here.
	4 * sizeof(void *) + 9 * sizeof(size_t) + sizeof(role_error) +
	// role_fixed_add (reference/extra/return), admit (base/extra/callback/context/
	// return), query (two outputs/total/two return tests), preflight (work/current/
	// reserve/context/return), original role validator (receiver/return).
	sizeof(void *) + sizeof(size_t) + sizeof(bool) + 2 * sizeof(size_t) + 2 * sizeof(void *) +
	sizeof(bool) + 2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) + 3 * sizeof(void *) +
	sizeof(size_t) + sizeof(bool) + sizeof(void *) + sizeof(bool) +
	// Pure public getter/query chain and the real constexpr query return carrier.
	native_mobile_birth_cash_role_recipe_query_frame_bytes() + sizeof(size_t) +
	// Original fixed framing: role offset and four-byte loop, span subspan
	// receiver/offset/count/extent check and actual pointer+length construction.
	2 * sizeof(size_t) + 2 * sizeof(uint32_t) + sizeof(uint8_t) + 3 * sizeof(void *) +
	3 * sizeof(size_t) + sizeof(bool) + 2 * sizeof(void *) + sizeof(size_t) +
	// Exact copy transport and full original range-vector cleanup. The default
	// workspace vector really constructs, and is destroyed on every outcome.
	result_copy_frames + result_vector_lifetime_frames<uint8_t>() +
	2 * sizeof(role_fixed_workspace *) +
	// Genuine size/capacity/begin wrappers and normal-iterator constructor/base
	// receivers used to select the original vector range for std::copy.
	3 * (sizeof(void *) + sizeof(size_t)) + 4 * (2 * sizeof(void *)) +
	// Native role candidate constructor/assignment receivers and copied fields;
	// fixed DTO storage itself is in the entry-inline census, not this source.
	4 * sizeof(void *) + 2 * sizeof(bool);

bool role_fixed_add(size_t &value, size_t extra) noexcept
{
	if (extra > SIZE_MAX - value)
		return false;
	value += extra;
	return true;
}
bool role_fixed_admit(size_t base, size_t extra, role_reserve_fn reserve, void *context) noexcept
{
	return role_fixed_add(base, extra) && reserve && reserve(base, context);
}

bool role_fixed_query(size_t *source, size_t *valid) noexcept
{
	if (!source || !valid ||
	    !native_mobile_birth_constructor_recipe_valid_source_frame_bytes(valid))
		return false;
	size_t total = role_fixed_local_source;
	if (!role_fixed_add(total, *valid))
		return false;
	*source = total;
	return true;
}

bool role_constructor_preflight(role_fixed_workspace &work, size_t current, role_reserve_fn reserve,
				void *context) noexcept
{
	if (!role_fixed_admit(current, native_mobile_birth_constructor_recipe_query_frame_bytes(),
			      reserve, context) ||
	    !native_mobile_birth_constructor_recipe_own_source_frame_bytes(
		    &work.constructor_source) ||
	    !native_mobile_birth_constructor_recipe_initial_inline_bytes(&work.constructor_inline))
		return false;
	return role_fixed_add(current, work.constructor_source) &&
	       role_fixed_admit(current, work.constructor_inline, reserve, context);
}
}

bool native_mobile_birth_cash_role_recipe_source_frame_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
	size_t total = 0, valid = 0;
	if (!role_fixed_query(&total, &valid))
		return false;
	*output = total;
	return true;
}

bool native_mobile_birth_cash_role_recipe_initial_inline_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
	size_t valid = 0;
	if (!native_mobile_birth_constructor_recipe_valid_source_frame_bytes(&valid))
		return false;
	constexpr size_t encode = sizeof(role_fixed_workspace);
	constexpr size_t decode =
		sizeof(native_mobile_birth_cash_role_recipe) + sizeof(std::span<const uint8_t>);
	*output = encode > decode ? encode : decode;
	return true;
}

economic_accounting_error native_mobile_birth_cash_role_recipe_encode_fixed_bounded(
	const native_mobile_birth_cash_role_recipe &role,
	native_mobile_birth_cash_role_recipe_bytes *output, role_reserve_fn reserve, void *context,
	size_t outer) noexcept
{
	size_t source = 0, valid = 0;
	if (!role_fixed_admit(outer, native_mobile_birth_cash_role_recipe_query_frame_bytes(),
			      reserve, context))
		return role_error::capacity;
	if (!role_fixed_query(&source, &valid))
		return role_error::unresolved;
	size_t base = outer;
	if (!role_fixed_add(base, source) ||
	    !role_fixed_admit(base, sizeof(role_fixed_workspace), reserve, context))
		return role_error::capacity;
	if (!output || !native_mobile_birth_cash_role_recipe_valid(role))
		return role_error::corrupt_evidence;
	try
	{
		role_fixed_workspace work;
		work.constructor_valid = valid;
		if (!role_fixed_add(base, sizeof(work)))
			return role_error::capacity;
		// The preceding role validator has returned. The child owns its full
		// constructor SOURCE, including that validator, so drop only that exact
		// completed phase from its outer rather than adding it a second time.
		const size_t child_outer = base - valid;
		if (!role_constructor_preflight(work, child_outer, reserve, context))
			return role_error::capacity;
		if (!native_mobile_birth_constructor_recipe_encode_blob_fixed_bounded(
			    role.original, &work.original, reserve, context, child_outer))
			return role_error::capacity;
		if (work.original.size() != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES)
			return role_error::capacity;
		if (!role_fixed_admit(base, work.original.capacity(), reserve, context))
			return role_error::capacity;
		work.candidate[0] = 'N';
		work.candidate[1] = 'B';
		work.candidate[2] = 'C';
		work.candidate[3] = '4';
		work.candidate[4] = NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_VERSION;
		std::copy(work.original.begin(), work.original.end(), work.candidate.begin() + 8);
		constexpr size_t role_offset =
			8 + NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES;
		work.candidate[role_offset] = static_cast<uint8_t>(role.role);
		for (size_t i = 0; i < 4; ++i)
			work.candidate[role_offset + 1 + i] =
				static_cast<uint8_t>(role.configured_shop_matches >> (8 * i));
		*output = work.candidate;
		return role_error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return role_error::capacity;
	}
	catch (...)
	{
		return role_error::corrupt_evidence;
	}
}

economic_accounting_error native_mobile_birth_cash_role_recipe_decode_fixed_bounded(
	const std::span<const uint8_t> &bytes, native_mobile_birth_cash_role_recipe *output,
	role_reserve_fn reserve, void *context, size_t outer) noexcept
{
	size_t source = 0, valid = 0;
	if (!role_fixed_admit(outer, native_mobile_birth_cash_role_recipe_query_frame_bytes(),
			      reserve, context))
		return role_error::capacity;
	if (!role_fixed_query(&source, &valid))
		return role_error::unresolved;
	constexpr size_t local =
		sizeof(native_mobile_birth_cash_role_recipe) + sizeof(std::span<const uint8_t>);
	size_t base = outer;
	if (!role_fixed_add(base, source) || !role_fixed_add(base, local) ||
	    !role_fixed_admit(base, 0, reserve, context))
		return role_error::capacity;
	if (!output || bytes.size() != NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_BYTES ||
	    bytes[0] != 'N' || bytes[1] != 'B' || bytes[2] != 'C' || bytes[3] != '4' ||
	    bytes[4] != NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_VERSION || bytes[5] || bytes[6] ||
	    bytes[7])
		return role_error::corrupt_evidence;
	native_mobile_birth_cash_role_recipe candidate;
	const auto embedded =
		bytes.subspan(8, NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES);
	size_t child_source = 0, child_inline = 0;
	const size_t child_outer = base - valid;
	if (!role_fixed_admit(child_outer,
			      native_mobile_birth_constructor_recipe_query_frame_bytes(), reserve,
			      context) ||
	    !native_mobile_birth_constructor_recipe_own_source_frame_bytes(&child_source) ||
	    !native_mobile_birth_constructor_recipe_initial_inline_bytes(&child_inline) ||
	    !role_fixed_add(child_source, child_inline) ||
	    !role_fixed_admit(child_outer, child_source, reserve, context))
		return role_error::capacity;
	const auto status = native_mobile_birth_constructor_recipe_decode_status_fixed_bounded(
		embedded, &candidate.original, reserve, context, child_outer);
	if (status != role_error::ok)
		return status;
	constexpr size_t role_offset = 8 + NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES;
	candidate.role = static_cast<native_mobile_birth_cash_role>(bytes[role_offset]);
	candidate.configured_shop_matches = 0;
	for (size_t i = 0; i < 4; ++i)
		candidate.configured_shop_matches |= uint32_t(bytes[role_offset + 1 + i])
						     << (8 * i);
	if (!native_mobile_birth_cash_role_recipe_valid(candidate))
		return role_error::corrupt_evidence;
	*output = candidate;
	return role_error::ok;
}
