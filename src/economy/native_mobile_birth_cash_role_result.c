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
