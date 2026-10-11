#include "economy/native_mobile_birth_cash_role_result.h"

#include <algorithm>
#include <climits>
#include <new>
#include <openssl/sha.h>

namespace
{
using error = economic_accounting_error;
bool valid(const native_mobile_birth_cash_role_result &value) noexcept
{
	const auto nonzero = [](const economic_digest &digest) {
		return std::any_of(digest.begin(), digest.end(),
				   [](uint8_t byte) { return byte != 0; });
	};
	if (!value.mobile_instance_id || value.mobile_instance_id == UINT64_MAX ||
	    value.mobile_revision != 1 || value.stock_revision != 1 || value.cash_revision != 1 ||
	    !nonzero(value.image_digest) || !nonzero(value.payload_digest) ||
	    !nonzero(value.role_digest) || !nonzero(value.plan_digest))
		return false;
	const auto &shared = value.shared;
	if (value.role == native_mobile_birth_cash_role::ordinary_wallet)
		return value.wallet_mapping_id && value.item_owner_id == value.mobile_instance_id &&
		       value.item_owner_revision == 1 && !shared.shop_id &&
		       !shared.shop_before_present && !shared.shop_after_present &&
		       !shared.owner_before_present && !shared.owner_after_present &&
		       !shared.shop_revision_before && !shared.shop_revision_after &&
		       !shared.owner_revision_before && !shared.owner_revision_after &&
		       std::all_of(shared.born_cash.begin(), shared.born_cash.end(),
				   [](int64_t part) { return part == 0; });
	return value.role == native_mobile_birth_cash_role::shared_shopkeeper &&
	       !value.wallet_mapping_id &&
	       native_mobile_birth_shared_shop_participant_valid(shared) &&
	       value.item_owner_id == item_shopkeeper_owner_id(shared.shop_id) &&
	       value.item_owner_revision == shared.owner_revision_after;
}
void put(uint8_t *output, uint64_t value, size_t bytes = 8) noexcept
{
	for (size_t i = 0; i < bytes; ++i)
		output[i] = static_cast<uint8_t>(value >> (8 * i));
}
uint64_t get(const uint8_t *input, size_t bytes = 8) noexcept
{
	uint64_t value = 0;
	for (size_t i = 0; i < bytes; ++i)
		value |= static_cast<uint64_t>(input[i]) << (8 * i);
	return value;
}
bool equal(const native_mobile_birth_cash_role_result &left,
	   const native_mobile_birth_cash_role_result &right) noexcept
{
	std::array<uint8_t, NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES> a{}, b{};
	return native_mobile_birth_cash_role_result_encode(left, &a) &&
	       native_mobile_birth_cash_role_result_encode(right, &b) && a == b;
}
error build(const critical_command &command, const economic_account_key *wallet,
	    const native_mobile_birth_shared_shop_participant *shared,
	    const economic_accounting_plan &plan,
	    native_mobile_birth_cash_role_result *output) noexcept
{
	if (!output || bool(wallet) == bool(shared))
		return error::invalid_identity;
	try
	{
		quest_mobile_native_image image;
		std::vector<native_mobile_birth_item_recipe> recipes;
		native_mobile_birth_cash_role_recipe role;
		auto status = native_mobile_birth_cash_role_command_decode(command, &image,
									   &recipes, &role);
		if (status != error::ok)
			return status;
		economic_accounting_plan expected;
		status = wallet ? native_mobile_birth_cash_role_accounting_compile(command, *wallet,
										   &expected) :
				  native_mobile_birth_cash_role_accounting_compile(command, *shared,
										   &expected);
		if (status != error::ok)
			return status;
		std::vector<uint8_t> expected_bytes, plan_bytes;
		status = economic_plan_encode(expected, &expected_bytes);
		if (status != error::ok)
			return status;
		status = economic_plan_encode(plan, &plan_bytes);
		if (status != error::ok)
			return status;
		if (expected_bytes != plan_bytes)
			return error::payload_conflict;
		native_mobile_birth_cash_role_result candidate;
		candidate.role = role.role;
		candidate.mobile_instance_id = image.reference.mobile_instance_id;
		candidate.mobile_revision = image.reference.mobile_revision;
		candidate.stock_revision = image.reference.stock_revision;
		candidate.cash_revision = image.cash->revision;
		if (wallet)
		{
			candidate.wallet_mapping_id = wallet->authority_id;
			candidate.item_owner_id = image.reference.mobile_instance_id;
			candidate.item_owner_revision = 1;
		}
		else
		{
			candidate.shared = *shared;
			candidate.item_owner_id = item_shopkeeper_owner_id(shared->shop_id);
			candidate.item_owner_revision = shared->owner_revision_after;
		}
		std::vector<uint8_t> image_bytes;
		const auto image_status = quest_mobile_native_image_encode(image, &image_bytes);
		if (image_status != player_snapshot_codec_result::ok)
			return image_status == player_snapshot_codec_result::allocation_failure ||
					       image_status ==
						       player_snapshot_codec_result::limit_exceeded ?
				       error::capacity :
				       error::corrupt_evidence;
		native_mobile_birth_cash_role_recipe_bytes role_bytes;
		if (!native_mobile_birth_cash_role_recipe_encode(role, &role_bytes))
			return error::capacity;
		if (!SHA256(image_bytes.data(), image_bytes.size(),
			    candidate.image_digest.data()) ||
		    !SHA256(command.payload.data(), command.payload.size(),
			    candidate.payload_digest.data()) ||
		    !SHA256(role_bytes.data(), role_bytes.size(), candidate.role_digest.data()) ||
		    !SHA256(plan_bytes.data(), plan_bytes.size(), candidate.plan_digest.data()) ||
		    !valid(candidate))
			return error::corrupt_evidence;
		*output = candidate;
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
} // namespace

bool native_mobile_birth_cash_role_result_encode(
	const native_mobile_birth_cash_role_result &value,
	std::array<uint8_t, NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES> *output) noexcept
{
	if (!output || !valid(value))
		return false;
	std::array<uint8_t, NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES> bytes{};
	bytes[0] = 'M';
	bytes[1] = 'B';
	bytes[2] = 'R';
	bytes[3] = '4';
	bytes[4] = NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_VERSION;
	bytes[5] = static_cast<uint8_t>(value.role);
	const uint64_t fields[] = { value.mobile_instance_id, value.mobile_revision,
				    value.stock_revision,     value.cash_revision,
				    value.wallet_mapping_id,  value.item_owner_id,
				    value.item_owner_revision };
	for (size_t i = 0; i < 7; ++i)
		put(bytes.data() + 8 + 8 * i, fields[i]);
	const auto &shared = value.shared;
	put(bytes.data() + 64, shared.shop_id, 4);
	bytes[68] = shared.shop_before_present;
	bytes[69] = shared.shop_after_present;
	bytes[70] = shared.owner_before_present;
	bytes[71] = shared.owner_after_present;
	const uint64_t clocks[] = { shared.shop_revision_before, shared.shop_revision_after,
				    shared.owner_revision_before, shared.owner_revision_after };
	for (size_t i = 0; i < 4; ++i)
	{
		put(bytes.data() + 72 + 8 * i, clocks[i]);
		put(bytes.data() + 104 + 8 * i, static_cast<uint64_t>(shared.born_cash[i]));
	}
	std::copy(value.image_digest.begin(), value.image_digest.end(), bytes.begin() + 136);
	std::copy(value.payload_digest.begin(), value.payload_digest.end(), bytes.begin() + 168);
	std::copy(value.role_digest.begin(), value.role_digest.end(), bytes.begin() + 200);
	std::copy(value.plan_digest.begin(), value.plan_digest.end(), bytes.begin() + 232);
	*output = bytes;
	return true;
}

bool native_mobile_birth_cash_role_result_decode(
	std::span<const uint8_t> bytes, native_mobile_birth_cash_role_result *output) noexcept
{
	if (!output || bytes.size() != NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES ||
	    bytes[0] != 'M' || bytes[1] != 'B' || bytes[2] != 'R' || bytes[3] != '4' ||
	    bytes[4] != NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_VERSION || bytes[6] || bytes[7])
		return false;
	for (size_t i = 68; i < 72; ++i)
		if (bytes[i] > 1)
			return false;
	native_mobile_birth_cash_role_result value;
	value.role = static_cast<native_mobile_birth_cash_role>(bytes[5]);
	value.mobile_instance_id = get(bytes.data() + 8);
	value.mobile_revision = get(bytes.data() + 16);
	value.stock_revision = get(bytes.data() + 24);
	value.cash_revision = get(bytes.data() + 32);
	value.wallet_mapping_id = get(bytes.data() + 40);
	value.item_owner_id = get(bytes.data() + 48);
	value.item_owner_revision = get(bytes.data() + 56);
	auto &shared = value.shared;
	shared.shop_id = static_cast<uint32_t>(get(bytes.data() + 64, 4));
	shared.shop_before_present = bytes[68];
	shared.shop_after_present = bytes[69];
	shared.owner_before_present = bytes[70];
	shared.owner_after_present = bytes[71];
	shared.shop_revision_before = get(bytes.data() + 72);
	shared.shop_revision_after = get(bytes.data() + 80);
	shared.owner_revision_before = get(bytes.data() + 88);
	shared.owner_revision_after = get(bytes.data() + 96);
	for (size_t i = 0; i < 4; ++i)
	{
		const uint64_t cash = get(bytes.data() + 104 + 8 * i);
		if (cash > INT64_MAX)
			return false;
		shared.born_cash[i] = static_cast<int64_t>(cash);
	}
	std::copy_n(bytes.begin() + 136, value.image_digest.size(), value.image_digest.begin());
	std::copy_n(bytes.begin() + 168, value.payload_digest.size(), value.payload_digest.begin());
	std::copy_n(bytes.begin() + 200, value.role_digest.size(), value.role_digest.begin());
	std::copy_n(bytes.begin() + 232, value.plan_digest.size(), value.plan_digest.begin());
	std::array<uint8_t, NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES> canonical{};
	if (!native_mobile_birth_cash_role_result_encode(value, &canonical) ||
	    !std::equal(canonical.begin(), canonical.end(), bytes.begin()))
		return false;
	*output = value;
	return true;
}

economic_accounting_error native_mobile_birth_cash_role_result_build(
	const critical_command &command, const economic_account_key &wallet,
	const economic_accounting_plan &plan, native_mobile_birth_cash_role_result *output) noexcept
{
	return build(command, &wallet, nullptr, plan, output);
}
economic_accounting_error native_mobile_birth_cash_role_result_build(
	const critical_command &command, const native_mobile_birth_shared_shop_participant &shared,
	const economic_accounting_plan &plan, native_mobile_birth_cash_role_result *output) noexcept
{
	return build(command, nullptr, &shared, plan, output);
}
bool native_mobile_birth_cash_role_result_matches(
	const critical_command &command, const economic_account_key &wallet,
	const economic_accounting_plan &plan,
	const native_mobile_birth_cash_role_result &result) noexcept
{
	native_mobile_birth_cash_role_result expected;
	return native_mobile_birth_cash_role_result_build(command, wallet, plan, &expected) ==
		       error::ok &&
	       equal(expected, result);
}
bool native_mobile_birth_cash_role_result_matches(
	const critical_command &command, const native_mobile_birth_shared_shop_participant &shared,
	const economic_accounting_plan &plan,
	const native_mobile_birth_cash_role_result &result) noexcept
{
	native_mobile_birth_cash_role_result expected;
	return native_mobile_birth_cash_role_result_build(command, shared, plan, &expected) ==
		       error::ok &&
	       equal(expected, result);
}

#include <type_traits>
#include <utility>

namespace
{
bool role_result_add(size_t &bytes, size_t extra) noexcept
{
	if (extra > SIZE_MAX - bytes)
		return false;
	bytes += extra;
	return true;
}
bool role_result_admit(size_t base, size_t extra, bool (*reserve)(size_t, void *) noexcept,
		       void *context) noexcept
{
	return extra <= SIZE_MAX - base && reserve && reserve(base + extra, context);
}
struct role_result_build_workspace
{
	quest_mobile_native_image image;
	std::vector<native_mobile_birth_item_recipe> recipes;
	native_mobile_birth_cash_role_recipe role;
	economic_accounting_plan expected;
	std::vector<uint8_t> expected_bytes, plan_bytes, image_bytes;
	native_mobile_birth_cash_role_result candidate;
	native_mobile_birth_cash_role_recipe_bytes role_bytes;
	economic_accounting_plan_allocation_profile profile;
	size_t image_heap = 0, recipe_heap = 0, expected_heap = 0;
};
struct role_result_build_live
{
	role_result_build_workspace &work;
	size_t base;
	bool bytes(size_t &out) const noexcept
	{
		out = base;
		return role_result_add(out, work.image_heap) &&
		       role_result_add(out, work.recipe_heap) &&
		       role_result_add(out, work.expected_heap) &&
		       role_result_add(out, work.expected_bytes.capacity()) &&
		       role_result_add(out, work.plan_bytes.capacity()) &&
		       role_result_add(out, work.image_bytes.capacity());
	}
};
error role_result_plan_encode_bounded(const economic_accounting_plan &plan,
				      std::vector<uint8_t> *output,
				      economic_accounting_plan_allocation_profile &profile,
				      bool (*reserve)(size_t, void *) noexcept, void *context,
				      size_t outer) noexcept
{
	if (!role_result_admit(outer, economic_plan_allocation_preflight_working_bytes(), reserve,
			       context))
		return error::capacity;
	auto status = economic_plan_allocation_preflight(plan, &profile);
	if (status != error::ok)
		return status;
	if (!profile.storage_policy_supported ||
	    !role_result_admit(outer, profile.encode_working_bytes, reserve, context))
		return error::capacity;
	return economic_plan_encode(plan, output);
}
error role_result_role_encode_bounded(const native_mobile_birth_cash_role_recipe &role,
				      native_mobile_birth_cash_role_recipe_bytes *output,
				      bool (*reserve)(size_t, void *) noexcept, void *context,
				      size_t outer) noexcept
{
	if (!output || !native_mobile_birth_cash_role_recipe_valid(role))
		return error::capacity;
	struct workspace
	{
		std::vector<uint8_t> original;
		native_mobile_birth_cash_role_recipe_bytes candidate{};
	};
	size_t base = outer;
	if (!role_result_add(base, sizeof(workspace)) ||
	    !role_result_admit(base, 0, reserve, context))
		return error::capacity;
	workspace work;
	if (!native_mobile_birth_constructor_recipe_encode_blob_bounded(
		    role.original, &work.original, reserve, context, base) ||
	    work.original.size() != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES)
		return error::capacity;
	work.candidate[0] = 'N';
	work.candidate[1] = 'B';
	work.candidate[2] = 'C';
	work.candidate[3] = '4';
	work.candidate[4] = NATIVE_MOBILE_BIRTH_CASH_ROLE_RECIPE_VERSION;
	std::copy(work.original.begin(), work.original.end(), work.candidate.begin() + 8);
	constexpr size_t offset = 8 + NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_BYTES;
	work.candidate[offset] = static_cast<uint8_t>(role.role);
	for (size_t i = 0; i < 4; ++i)
		work.candidate[offset + 1 + i] =
			static_cast<uint8_t>(role.configured_shop_matches >> (8 * i));
	*output = work.candidate;
	return error::ok;
}
error role_result_build_bounded(const critical_command &command, const economic_account_key *wallet,
				const native_mobile_birth_shared_shop_participant *shared,
				const economic_accounting_plan &plan,
				native_mobile_birth_cash_role_result *output,
				bool (*reserve)(size_t, void *) noexcept, void *context,
				size_t outer) noexcept
{
	if (!output || bool(wallet) == bool(shared))
		return error::invalid_identity;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)command;
	(void)plan;
	(void)reserve;
	(void)context;
	(void)outer;
	return error::unresolved;
#else
	size_t base = outer;
	if (!role_result_add(base, sizeof(role_result_build_workspace)) ||
	    !role_result_add(base, sizeof(role_result_build_live)) ||
	    !role_result_admit(base, 0, reserve, context))
		return error::capacity;
	try
	{
		role_result_build_workspace work;
		role_result_build_live live{ work, base };
		auto status = native_mobile_birth_cash_role_command_decode_bounded(
			command, &work.image, &work.recipes, &work.role, reserve, context, base,
			&work.image_heap, &work.recipe_heap);
		if (status != error::ok)
			return status;
		size_t current = 0;
		if (!live.bytes(current))
			return error::capacity;
		status = wallet ? native_mobile_birth_cash_role_accounting_compile_bounded(
					  command, *wallet, &work.expected, reserve, context,
					  current, &work.expected_heap) :
				  native_mobile_birth_cash_role_accounting_compile_bounded(
					  command, *shared, &work.expected, reserve, context,
					  current, &work.expected_heap);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		status = role_result_plan_encode_bounded(work.expected, &work.expected_bytes,
							 work.profile, reserve, context, current);
		if (status != error::ok)
			return status;
		if (!live.bytes(current))
			return error::capacity;
		status = role_result_plan_encode_bounded(plan, &work.plan_bytes, work.profile,
							 reserve, context, current);
		if (status != error::ok)
			return status;
		if (work.expected_bytes != work.plan_bytes)
			return error::payload_conflict;
		auto &candidate = work.candidate;
		candidate.role = work.role.role;
		candidate.mobile_instance_id = work.image.reference.mobile_instance_id;
		candidate.mobile_revision = work.image.reference.mobile_revision;
		candidate.stock_revision = work.image.reference.stock_revision;
		candidate.cash_revision = work.image.cash->revision;
		if (wallet)
		{
			candidate.wallet_mapping_id = wallet->authority_id;
			candidate.item_owner_id = work.image.reference.mobile_instance_id;
			candidate.item_owner_revision = 1;
		}
		else
		{
			candidate.shared = *shared;
			candidate.item_owner_id = item_shopkeeper_owner_id(shared->shop_id);
			candidate.item_owner_revision = shared->owner_revision_after;
		}
		if (!live.bytes(current))
			return error::capacity;
		const auto image_status = quest_mobile_native_image_encode_bounded(
			work.image, &work.image_bytes, reserve, context, current);
		if (image_status != player_snapshot_codec_result::ok)
			return image_status == player_snapshot_codec_result::allocation_failure ||
					       image_status ==
						       player_snapshot_codec_result::limit_exceeded ?
				       error::capacity :
				       error::corrupt_evidence;
		if (!live.bytes(current))
			return error::capacity;
		status = role_result_role_encode_bounded(work.role, &work.role_bytes, reserve,
							 context, current);
		if (status != error::ok)
			return status;
		if (!SHA256(work.image_bytes.data(), work.image_bytes.size(),
			    candidate.image_digest.data()) ||
		    !SHA256(command.payload.data(), command.payload.size(),
			    candidate.payload_digest.data()) ||
		    !SHA256(work.role_bytes.data(), work.role_bytes.size(),
			    candidate.role_digest.data()) ||
		    !SHA256(work.plan_bytes.data(), work.plan_bytes.size(),
			    candidate.plan_digest.data()) ||
		    !valid(candidate))
			return error::corrupt_evidence;
		static_assert(
			std::is_nothrow_copy_assignable_v<native_mobile_birth_cash_role_result>);
		*output = candidate;
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
bool role_result_matches_bounded(const critical_command &command,
				 const economic_account_key *wallet,
				 const native_mobile_birth_shared_shop_participant *shared,
				 const economic_accounting_plan &plan,
				 const native_mobile_birth_cash_role_result &result,
				 bool (*reserve)(size_t, void *) noexcept, void *context,
				 size_t outer) noexcept
{
	size_t base = outer;
	if (!role_result_add(base, sizeof(native_mobile_birth_cash_role_result)) ||
	    !role_result_admit(base, 0, reserve, context))
		return false;
	native_mobile_birth_cash_role_result expected;
	if (role_result_build_bounded(command, wallet, shared, plan, &expected, reserve, context,
				      base) != error::ok)
		return false;
	// equal() retains both output arrays; original fixed encoder's own bytes,
	// seven-field and four-clock arrays coexist through its complete encode.
	constexpr size_t equality =
		3 * sizeof(std::array<uint8_t, NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES>) +
		sizeof(uint64_t[7]) + sizeof(uint64_t[4]);
	if (!role_result_admit(base, equality, reserve, context))
		return false;
	return equal(expected, result);
}
}

economic_accounting_error native_mobile_birth_cash_role_result_build_bounded(
	const critical_command &command, const economic_account_key &wallet,
	const economic_accounting_plan &plan, native_mobile_birth_cash_role_result *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	return role_result_build_bounded(command, &wallet, nullptr, plan, output, reserve, context,
					 outer_live);
}
economic_accounting_error native_mobile_birth_cash_role_result_build_bounded(
	const critical_command &command, const native_mobile_birth_shared_shop_participant &shared,
	const economic_accounting_plan &plan, native_mobile_birth_cash_role_result *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	return role_result_build_bounded(command, nullptr, &shared, plan, output, reserve, context,
					 outer_live);
}
bool native_mobile_birth_cash_role_result_matches_bounded(
	const critical_command &command, const economic_account_key &wallet,
	const economic_accounting_plan &plan, const native_mobile_birth_cash_role_result &result,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	return role_result_matches_bounded(command, &wallet, nullptr, plan, result, reserve,
					   context, outer_live);
}
bool native_mobile_birth_cash_role_result_matches_bounded(
	const critical_command &command, const native_mobile_birth_shared_shop_participant &shared,
	const economic_accounting_plan &plan, const native_mobile_birth_cash_role_result &result,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	return role_result_matches_bounded(command, nullptr, &shared, plan, result, reserve,
					   context, outer_live);
}

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

constexpr size_t result_fixed_sha_assembly_frames =
	2 * 4 * 64 + 4 * sizeof(void *) + 6 * sizeof(uint64_t) + (256 * 4 - 1) + 2 * sizeof(void *);
constexpr size_t result_fixed_sha_c_small_frames =
	16 * sizeof(unsigned int) + 12 * sizeof(unsigned int) + sizeof(unsigned int) + sizeof(int) +
	sizeof(void *);
constexpr size_t result_fixed_sha_c_normal_frames = 16 * sizeof(unsigned int) +
						    11 * sizeof(unsigned int) + 2 * sizeof(int) +
						    2 * sizeof(void *);
constexpr size_t result_fixed_sha_init_frames = sizeof(void *) + sizeof(int);
constexpr size_t result_fixed_sha_update_frames = 2 * sizeof(void *) + sizeof(size_t) +
						  2 * sizeof(void *) + sizeof(unsigned int) +
						  sizeof(size_t) + sizeof(int);
constexpr size_t result_fixed_sha_final_frames = 3 * sizeof(void *) + sizeof(size_t) +
						 sizeof(unsigned long) + sizeof(unsigned int) +
						 sizeof(int);
// Real SHA256_Init/Update/Final memcpy/memset call arguments and result;
// OPENSSL_cleanse(buf,len) and the x86_64 leaf's return address. C fallback
// cleanse's actual ptr/len/pointer-result carriers are included as well.
constexpr size_t result_fixed_sha_memory_frames =
	2 * sizeof(void *) + sizeof(size_t) + sizeof(int) + sizeof(void *);
constexpr size_t result_fixed_sha_cleanse_frames =
	// mem_clr.c ptr/len and loaded volatile function pointer remain live
	// through its authentic indirect memset leaf; asm fallback is smaller.
	2 * sizeof(void *) + sizeof(size_t) + result_fixed_sha_memory_frames;
constexpr size_t result_fixed_sha_block_frames =
	// C compression ctx/in/num plus its actual typed locals; assembly term
	// already includes its own real caller return address.
	std::max(result_fixed_sha_assembly_frames,
		 2 * sizeof(void *) + sizeof(size_t) +
			 std::max(result_fixed_sha_c_small_frames,
				  result_fixed_sha_c_normal_frames));
constexpr size_t result_fixed_sha_frames =
	std::max(result_fixed_sha_init_frames,
		 std::max(result_fixed_sha_update_frames, result_fixed_sha_final_frames)) +
	std::max(result_fixed_sha_block_frames,
		 std::max(result_fixed_sha_memory_frames, result_fixed_sha_cleanse_frames));

using cash_result_error = economic_accounting_error;
using cash_result_reserve_fn = bool (*)(size_t, void *) noexcept;

bool cash_result_profile_supported() noexcept
{
#if defined(__linux__) && defined(__x86_64__) && defined(__GNUC__) && __GNUC__ == 13 &&    \
	!defined(__clang__) && __cplusplus == 202002L && defined(_GLIBCXX_RELEASE) &&      \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&                       \
	_GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG) &&                         \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                    \
	!(defined(_GLIBCXX_SANITIZE_STD_ALLOCATOR) && defined(_GLIBCXX_SANITIZE_VECTOR) && \
	  _GLIBCXX_SANITIZE_STD_ALLOCATOR && _GLIBCXX_SANITIZE_VECTOR) &&                  \
	!defined(OPENSSL_NO_DEPRECATED_3_0) && !defined(OPENSSL_NO_SHA256) &&              \
	defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR == 3 &&                    \
	defined(OPENSSL_VERSION_MINOR) && OPENSSL_VERSION_MINOR == 0
	return sizeof(void *) == 8 && sizeof(size_t) == 8 && sizeof(std::ptrdiff_t) == 8 &&
	       sizeof(unsigned int) == 4 && sizeof(unsigned long) == 8 && sizeof(SHA_LONG) == 4;
#else
	return false;
#endif
}
bool cash_result_add(size_t &bytes, size_t extra) noexcept
{
	if (extra > SIZE_MAX - bytes)
		return false;
	bytes += extra;
	return true;
}
bool cash_result_admit(size_t bytes, size_t extra, cash_result_reserve_fn reserve,
		       void *context) noexcept
{
	return cash_result_add(bytes, extra) && reserve && reserve(bytes, context);
}

struct cash_fixed_result_workspace
{
	quest_mobile_native_image image;
	std::vector<native_mobile_birth_item_recipe> recipes;
	native_mobile_birth_cash_role_recipe role;
	economic_accounting_plan expected;
	std::vector<uint8_t> expected_bytes, plan_bytes, image_bytes;
	native_mobile_birth_cash_role_result candidate;
	native_mobile_birth_cash_role_recipe_bytes role_bytes;
	size_t image_heap = 0, recipe_heap = 0, plan_heap = 0;
	size_t query = 0, source = 0, initial = 0, image_supplement = 0;
};
struct cash_fixed_result_budget
{
	cash_fixed_result_workspace &work;
	size_t base;
	bool live(size_t *output) const noexcept
	{
		if (!output)
			return false;
		size_t total = base;
		if (!cash_result_add(total, work.image_heap) ||
		    !cash_result_add(total, work.recipe_heap) ||
		    !cash_result_add(total, work.plan_heap) ||
		    !cash_result_add(total, work.expected_bytes.capacity()) ||
		    !cash_result_add(total, work.plan_bytes.capacity()) ||
		    !cash_result_add(total, work.image_bytes.capacity()))
			return false;
		*output = total;
		return true;
	}
};

// These pure getter calls are preadmitted before access. Each NEW child owns
// its complete SOURCE/entry itself. The existing image bounded entry needs ONLY
// its actual missing-source supplement retained in its outer, not full SOURCE.
bool cash_result_command_preflight(cash_fixed_result_workspace &work, size_t current,
				   cash_result_reserve_fn reserve, void *context) noexcept
{
	constexpr size_t query_frames =
		native_mobile_birth_cash_role_command_source_query_frame_bytes();
	work.query = query_frames;
	if (!cash_result_admit(current, work.query, reserve, context) ||
	    !native_mobile_birth_cash_role_command_decode_source_frame_bytes(&work.source) ||
	    !native_mobile_birth_cash_role_command_decode_initial_inline_bytes(&work.initial))
		return false;
	return cash_result_add(current, work.source) &&
	       cash_result_admit(current, work.initial, reserve, context);
}
bool cash_result_compile_preflight(cash_fixed_result_workspace &work, size_t current,
				   cash_result_reserve_fn reserve, void *context) noexcept
{
	work.query = native_mobile_birth_cash_role_accounting_compile_profile_query_frame_bytes();
	if (!cash_result_admit(current, work.query, reserve, context) ||
	    !native_mobile_birth_cash_role_accounting_compile_own_source_frame_bytes(
		    &work.source) ||
	    !native_mobile_birth_cash_role_accounting_compile_initial_inline_bytes(&work.initial))
		return false;
	return cash_result_add(current, work.source) &&
	       cash_result_admit(current, work.initial, reserve, context);
}
bool cash_result_plan_preflight(cash_fixed_result_workspace &work, size_t current,
				cash_result_reserve_fn reserve, void *context) noexcept
{
	work.query = economic_plan_bounded_profile_query_frame_bytes();
	if (!cash_result_admit(current, work.query, reserve, context) ||
	    !economic_plan_encode_source_frame_bytes(&work.source) ||
	    !economic_plan_encode_initial_inline_bytes(&work.initial))
		return false;
	return cash_result_add(current, work.source) &&
	       cash_result_admit(current, work.initial, reserve, context);
}
bool cash_result_image_preflight(cash_fixed_result_workspace &work, size_t current,
				 cash_result_reserve_fn reserve, void *context) noexcept
{
	work.query = quest_mobile_native_image_source_query_frame_bytes();
	if (!cash_result_admit(current, work.query, reserve, context) ||
	    !quest_mobile_native_image_encode_source_frame_bytes(&work.source) ||
	    !quest_mobile_native_image_encode_initial_inline_bytes(&work.initial) ||
	    !quest_mobile_native_image_encode_source_supplement_frame_bytes(&work.image_supplement))
		return false;
	return cash_result_add(current, work.source) &&
	       cash_result_admit(current, work.initial, reserve, context);
}
bool cash_result_role_preflight(cash_fixed_result_workspace &work, size_t current,
				cash_result_reserve_fn reserve, void *context) noexcept
{
	work.query = native_mobile_birth_cash_role_recipe_query_frame_bytes();
	if (!cash_result_admit(current, work.query, reserve, context) ||
	    !native_mobile_birth_cash_role_recipe_source_frame_bytes(&work.source) ||
	    !native_mobile_birth_cash_role_recipe_initial_inline_bytes(&work.initial))
		return false;
	return cash_result_add(current, work.source) &&
	       cash_result_admit(current, work.initial, reserve, context);
}

constexpr size_t cash_result_shared_valid_source =
	// participant validator receiver/hidden cash range/begin/end/part/cash,
	// economic_coin_value(vector&,output) total/index/status, narrow(wide,out),
	// genuine numeric_limits min/max and array size/index/begin/end carriers.
	5 * sizeof(void *) + 2 * sizeof(int64_t) + sizeof(bool) + 2 * sizeof(void *) +
	sizeof(__int128_t) + sizeof(size_t) + sizeof(cash_result_error) + sizeof(__int128_t) +
	sizeof(void *) + sizeof(bool) + 2 * sizeof(int64_t) +
	4 * (sizeof(void *) + sizeof(size_t)) + 4 * (2 * sizeof(void *)) +
	// item_shopkeeper_owner_id(uint32_t) has one true argument and uint64 result.
	sizeof(uint32_t) + sizeof(uint64_t);

constexpr size_t cash_result_scalar_source =
	// New build helper + one build overload + match overload: actual pointer
	// parameters, outer scalar, base/current/source and typed status results.
	7 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(cash_result_error) + 6 * sizeof(void *) +
	sizeof(size_t) + sizeof(cash_result_error) + 7 * sizeof(void *) + sizeof(size_t) +
	sizeof(cash_result_error) +
	// New decode signature and original fixed decoder/encoder scalar graph:
	// two by-value spans, output/encode output, loop, cash, fields and clocks
	// references. Fixed value/canonical/encoder buffers are inline below.
	2 * sizeof(std::span<const uint8_t>) + 4 * sizeof(void *) + 3 * sizeof(size_t) +
	sizeof(uint64_t) + sizeof(cash_result_error) +
	// Original valid/cash-zero all_of specialization: uint64 predicate argument
	// replaces byte; full any_of closure stays shared with actual result profile.
	result_valid_frames + result_valid_frames - sizeof(uint8_t) + sizeof(int64_t) +
	2 * sizeof(void *) + sizeof(bool) + cash_result_shared_valid_source +
	// Original encode/decode put/get actual pointer/value/byte-count/index.
	2 * sizeof(void *) + 3 * sizeof(size_t) + 2 * sizeof(uint64_t) + result_copy_frames +
	result_equal_frames +
	// Actual optional cash operator->/_M_get/addressof result carriers.
	4 * (2 * sizeof(void *)) +
	// Actual vector byte comparison: operator== size/begin/end then std::equal;
	// real capacity accessors used for all three returned encoded vectors.
	6 * (sizeof(void *) + sizeof(size_t)) + 4 * (2 * sizeof(void *)) +
	// Budget live receiver/output/total; add/admit params/return; preflight one
	// genuine sequential family signature. Pure child query carriers admitted
	// by each exact child metadata accessor before calling that child.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(bool) + sizeof(void *) + sizeof(size_t) +
	sizeof(bool) + 2 * sizeof(size_t) + 2 * sizeof(void *) + sizeof(bool) +
	// Complete command query is required constexpr; its actual named size_t
	// local replaces the accessor result carrier, separate from current.
	// Other genuine child query accessor results use the same single N.
	3 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);

constexpr size_t cash_result_plan_lifetime_source =
	// Actual six direct expected plan vectors; nested metadata string/optional
	// constructors/cleanup are the same original definition as general result.
	2 * sizeof(economic_accounting_plan *) +
	result_vector_lifetime_frames<economic_account_effect>() +
	result_vector_lifetime_frames<economic_coin_posting>() +
	result_vector_lifetime_frames<economic_child_link>() +
	2 * result_vector_lifetime_frames<economic_item_snapshot>() +
	result_vector_lifetime_frames<economic_item_event>() +
	// Actual metadata/operation metadata ctor+dtor and one optional source
	// default/cleanup chain; stored optional flags remain inside its object.
	4 * sizeof(void *) + 8 * sizeof(void *) + 8 * sizeof(void *);

constexpr size_t cash_result_local_source =
	cash_result_scalar_source + cash_result_plan_lifetime_source +
	3 * result_vector_lifetime_frames<uint8_t>() + 2 * sizeof(cash_fixed_result_workspace *) +
	// Fixed SHA helper actual data/length/output plus SHA init/update/final
	// selected complete C and assembly source leaves, shared exact source pins.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(bool) + result_fixed_sha_frames;

#if defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR == 3 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0) && !defined(OPENSSL_NO_SHA256)
constexpr size_t cash_result_sha_inline = sizeof(SHA256_CTX);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
bool cash_result_hash(const uint8_t *bytes, size_t size, economic_digest *output) noexcept
{
	SHA256_CTX state{};
	return SHA256_Init(&state) == 1 && SHA256_Update(&state, bytes, size) == 1 &&
	       SHA256_Final(output->data(), &state) == 1;
}
#pragma GCC diagnostic pop
#else
constexpr size_t cash_result_sha_inline = 0;
#endif

constexpr size_t cash_result_fixed_wire_inline =
	// Original decode value + canonical and delegated encoder candidate, with
	// both original seven-field/four-clock arrays. Equal additionally owns a/b;
	// its two encodes are sequential, so one actual encoder body is retained.
	sizeof(native_mobile_birth_cash_role_result) +
	3 * NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES + 11 * sizeof(uint64_t);

cash_result_error cash_result_build_fixed(const critical_command &command,
					  const economic_account_key *wallet,
					  const native_mobile_birth_shared_shop_participant *shared,
					  const economic_accounting_plan &plan,
					  native_mobile_birth_cash_role_result *output,
					  cash_result_reserve_fn reserve, void *context,
					  size_t outer) noexcept
{
	if (!output || bool(wallet) == bool(shared))
		return cash_result_error::invalid_identity;
	size_t source = 0;
	if (!cash_result_admit(
		    outer, native_mobile_birth_cash_role_result_fixed_source_query_frame_bytes(),
		    reserve, context))
		return cash_result_error::capacity;
	if (!native_mobile_birth_cash_role_result_fixed_source_frame_bytes(&source))
		return cash_result_error::capacity;
#if defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR == 3 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0) && !defined(OPENSSL_NO_SHA256)
	size_t base = outer;
	if (!cash_result_add(base, source) ||
	    !cash_result_add(base, sizeof(cash_fixed_result_workspace) +
					   sizeof(cash_fixed_result_budget) +
					   cash_result_sha_inline) ||
	    !cash_result_admit(base, 0, reserve, context))
		return cash_result_error::capacity;
	try
	{
		cash_fixed_result_workspace work;
		cash_fixed_result_budget budget{ work, base };
		size_t current = base;
		if (!cash_result_command_preflight(work, current, reserve, context))
			return cash_result_error::capacity;
		auto status = native_mobile_birth_cash_role_command_decode_fixed_bounded(
			command, &work.image, &work.recipes, &work.role, reserve, context, current,
			&work.image_heap, &work.recipe_heap);
		if (status != cash_result_error::ok)
			return status;
		if (!budget.live(&current) ||
		    !cash_result_compile_preflight(work, current, reserve, context))
			return cash_result_error::capacity;
		status = wallet ? native_mobile_birth_cash_role_accounting_compile_fixed_bounded(
					  command, *wallet, &work.expected, reserve, context,
					  current, &work.plan_heap) :
				  native_mobile_birth_cash_role_accounting_compile_fixed_bounded(
					  command, *shared, &work.expected, reserve, context,
					  current, &work.plan_heap);
		if (status != cash_result_error::ok)
			return status;
		if (!budget.live(&current) ||
		    !cash_result_plan_preflight(work, current, reserve, context))
			return cash_result_error::capacity;
		status = economic_plan_encode_bounded(work.expected, &work.expected_bytes, reserve,
						      context, current);
		if (status != cash_result_error::ok)
			return status;
		if (!budget.live(&current) ||
		    !cash_result_plan_preflight(work, current, reserve, context))
			return cash_result_error::capacity;
		status = economic_plan_encode_bounded(plan, &work.plan_bytes, reserve, context,
						      current);
		if (status != cash_result_error::ok)
			return status;
		if (!budget.live(&current))
			return cash_result_error::capacity;
		if (work.expected_bytes != work.plan_bytes)
			return cash_result_error::payload_conflict;
		work.candidate.role = work.role.role;
		work.candidate.mobile_instance_id = work.image.reference.mobile_instance_id;
		work.candidate.mobile_revision = work.image.reference.mobile_revision;
		work.candidate.stock_revision = work.image.reference.stock_revision;
		work.candidate.cash_revision = work.image.cash->revision;
		if (wallet)
		{
			work.candidate.wallet_mapping_id = wallet->authority_id;
			work.candidate.item_owner_id = work.image.reference.mobile_instance_id;
			work.candidate.item_owner_revision = 1;
		}
		else
		{
			work.candidate.shared = *shared;
			work.candidate.item_owner_id = item_shopkeeper_owner_id(shared->shop_id);
			work.candidate.item_owner_revision = shared->owner_revision_after;
		}
		if (!cash_result_image_preflight(work, current, reserve, context) ||
		    !cash_result_add(current, work.image_supplement))
			return cash_result_error::capacity;
		const auto image_status = quest_mobile_native_image_encode_bounded(
			work.image, &work.image_bytes, reserve, context, current);
		if (image_status != player_snapshot_codec_result::ok)
			return image_status == player_snapshot_codec_result::allocation_failure ||
					       image_status ==
						       player_snapshot_codec_result::limit_exceeded ?
				       cash_result_error::capacity :
				       cash_result_error::corrupt_evidence;
		if (!budget.live(&current) ||
		    !cash_result_role_preflight(work, current, reserve, context))
			return cash_result_error::capacity;
		// Original result flattens every failed NBC4 encoding to capacity.
		if (native_mobile_birth_cash_role_recipe_encode_fixed_bounded(
			    work.role, &work.role_bytes, reserve, context, current) !=
		    cash_result_error::ok)
			return cash_result_error::capacity;
		if (!cash_result_admit(current, 0, reserve, context))
			return cash_result_error::capacity;
		if (!cash_result_hash(work.image_bytes.data(), work.image_bytes.size(),
				      &work.candidate.image_digest) ||
		    !cash_result_hash(command.payload.data(), command.payload.size(),
				      &work.candidate.payload_digest) ||
		    !cash_result_hash(work.role_bytes.data(), work.role_bytes.size(),
				      &work.candidate.role_digest) ||
		    !cash_result_hash(work.plan_bytes.data(), work.plan_bytes.size(),
				      &work.candidate.plan_digest) ||
		    !valid(work.candidate))
			return cash_result_error::corrupt_evidence;
		*output = work.candidate;
		return cash_result_error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return cash_result_error::capacity;
	}
	catch (...)
	{
		return cash_result_error::corrupt_evidence;
	}
#else
	(void)command;
	(void)wallet;
	(void)shared;
	(void)plan;
	(void)reserve;
	(void)context;
	(void)outer;
	return cash_result_error::capacity;
#endif
}
}

bool native_mobile_birth_cash_role_result_fixed_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !cash_result_profile_supported())
		return false;
	size_t image = 0, recipes = 0;
	if (!quest_mobile_native_image_lifetime_source_frame_bytes(&image) ||
	    !native_mobile_birth_recipe_value_lifecycle_source_frame_bytes(&recipes) ||
	    !cash_result_add(image, recipes) || !cash_result_add(image, cash_result_local_source))
		return false;
	*output = image;
	return true;
}
bool native_mobile_birth_cash_role_result_fixed_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !cash_result_profile_supported())
		return false;
	constexpr size_t build = sizeof(cash_fixed_result_workspace) +
				 sizeof(cash_fixed_result_budget) + cash_result_sha_inline;
	*output = sizeof(native_mobile_birth_cash_role_result) +
		  (build > cash_result_fixed_wire_inline ? build : cash_result_fixed_wire_inline);
	return true;
}

economic_accounting_error native_mobile_birth_cash_role_result_decode_fixed_bounded(
	std::span<const uint8_t> bytes, native_mobile_birth_cash_role_result *output,
	cash_result_reserve_fn reserve, void *context, size_t outer) noexcept
{
	if (!output)
		return cash_result_error::corrupt_evidence;
	size_t source = 0;
	if (!cash_result_admit(
		    outer, native_mobile_birth_cash_role_result_fixed_source_query_frame_bytes(),
		    reserve, context) ||
	    !native_mobile_birth_cash_role_result_fixed_source_frame_bytes(&source) ||
	    !cash_result_add(source, cash_result_fixed_wire_inline) ||
	    !cash_result_admit(outer, source, reserve, context))
		return cash_result_error::capacity;
	return native_mobile_birth_cash_role_result_decode(bytes, output) ?
		       cash_result_error::ok :
		       cash_result_error::corrupt_evidence;
}

economic_accounting_error native_mobile_birth_cash_role_result_build_fixed_bounded(
	const critical_command &command, const economic_account_key &wallet,
	const economic_accounting_plan &plan, native_mobile_birth_cash_role_result *output,
	cash_result_reserve_fn reserve, void *context, size_t outer) noexcept
{
	return cash_result_build_fixed(command, &wallet, nullptr, plan, output, reserve, context,
				       outer);
}
economic_accounting_error native_mobile_birth_cash_role_result_build_fixed_bounded(
	const critical_command &command, const native_mobile_birth_shared_shop_participant &shared,
	const economic_accounting_plan &plan, native_mobile_birth_cash_role_result *output,
	cash_result_reserve_fn reserve, void *context, size_t outer) noexcept
{
	return cash_result_build_fixed(command, nullptr, &shared, plan, output, reserve, context,
				       outer);
}

economic_accounting_error native_mobile_birth_cash_role_result_matches_fixed_bounded(
	const critical_command &command, const economic_account_key &wallet,
	const economic_accounting_plan &plan, const native_mobile_birth_cash_role_result &result,
	bool *matches, cash_result_reserve_fn reserve, void *context, size_t outer) noexcept
{
	if (!matches)
		return cash_result_error::invalid_identity;
	size_t source = 0;
	if (!cash_result_admit(
		    outer, native_mobile_birth_cash_role_result_fixed_source_query_frame_bytes(),
		    reserve, context) ||
	    !native_mobile_birth_cash_role_result_fixed_source_frame_bytes(&source) ||
	    !cash_result_add(source, cash_result_fixed_wire_inline) ||
	    !cash_result_admit(outer, source, reserve, context) ||
	    !cash_result_add(outer, sizeof(native_mobile_birth_cash_role_result)))
		return cash_result_error::capacity;
	native_mobile_birth_cash_role_result expected;
	const auto status = native_mobile_birth_cash_role_result_build_fixed_bounded(
		command, wallet, plan, &expected, reserve, context, outer);
	if (status != cash_result_error::ok)
		return status;
	// Build's workspace has ended; admit the genuine equal/encoder phase fresh.
	// source still contains fixed_wire_inline, whose result is this exact
	// expected already retained in outer. Drop it once for this phase.
	if (!cash_result_admit(outer, source - sizeof(expected), reserve, context))
		return cash_result_error::capacity;
	*matches = equal(expected, result);
	return cash_result_error::ok;
}
economic_accounting_error native_mobile_birth_cash_role_result_matches_fixed_bounded(
	const critical_command &command, const native_mobile_birth_shared_shop_participant &shared,
	const economic_accounting_plan &plan, const native_mobile_birth_cash_role_result &result,
	bool *matches, cash_result_reserve_fn reserve, void *context, size_t outer) noexcept
{
	if (!matches)
		return cash_result_error::invalid_identity;
	size_t source = 0;
	if (!cash_result_admit(
		    outer, native_mobile_birth_cash_role_result_fixed_source_query_frame_bytes(),
		    reserve, context) ||
	    !native_mobile_birth_cash_role_result_fixed_source_frame_bytes(&source) ||
	    !cash_result_add(source, cash_result_fixed_wire_inline) ||
	    !cash_result_admit(outer, source, reserve, context) ||
	    !cash_result_add(outer, sizeof(native_mobile_birth_cash_role_result)))
		return cash_result_error::capacity;
	native_mobile_birth_cash_role_result expected;
	const auto status = native_mobile_birth_cash_role_result_build_fixed_bounded(
		command, shared, plan, &expected, reserve, context, outer);
	if (status != cash_result_error::ok)
		return status;
	if (!cash_result_admit(outer, source - sizeof(expected), reserve, context))
		return cash_result_error::capacity;
	*matches = equal(expected, result);
	return cash_result_error::ok;
}
