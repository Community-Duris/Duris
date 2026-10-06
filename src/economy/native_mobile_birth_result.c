#include "economy/native_mobile_birth_result.h"

#include <algorithm>
#include <new>
#include <openssl/sha.h>

namespace
{
bool valid(const native_mobile_birth_result &result) noexcept
{
	const auto nonzero = [](const economic_digest &digest) {
		return std::any_of(digest.begin(), digest.end(),
				   [](uint8_t byte) { return byte != 0; });
	};
	return result.mobile_instance_id && result.mobile_instance_id != UINT64_MAX &&
	       result.wallet_mapping_id && result.mobile_revision == 1 &&
	       result.stock_revision == 1 && result.cash_revision == 1 &&
	       result.item_owner_revision == 1 && nonzero(result.image_digest) &&
	       nonzero(result.plan_digest);
}

void put(uint8_t *output, uint64_t value) noexcept
{
	for (size_t index = 0; index != 8; ++index)
		output[index] = static_cast<uint8_t>(value >> (8 * index));
}

uint64_t get(const uint8_t *input) noexcept
{
	uint64_t value = 0;
	for (size_t index = 0; index != 8; ++index)
		value |= static_cast<uint64_t>(input[index]) << (8 * index);
	return value;
}

bool equal(const native_mobile_birth_result &left, const native_mobile_birth_result &right) noexcept
{
	return left.mobile_instance_id == right.mobile_instance_id &&
	       left.mobile_revision == right.mobile_revision &&
	       left.stock_revision == right.stock_revision &&
	       left.cash_revision == right.cash_revision &&
	       left.wallet_mapping_id == right.wallet_mapping_id &&
	       left.item_owner_revision == right.item_owner_revision &&
	       left.image_digest == right.image_digest && left.plan_digest == right.plan_digest;
}
}

bool native_mobile_birth_result_encode(
	const native_mobile_birth_result &result,
	std::array<uint8_t, NATIVE_MOBILE_BIRTH_RESULT_BYTES> *output) noexcept
{
	if (!output || !valid(result))
		return false;
	std::array<uint8_t, NATIVE_MOBILE_BIRTH_RESULT_BYTES> candidate{};
	candidate[0] = 'M';
	candidate[1] = 'B';
	candidate[2] = 'R';
	candidate[3] = '1';
	candidate[4] = 1; // Little-endian version1; remaining header bytes are zero.
	const uint64_t fields[] = { result.mobile_instance_id, result.mobile_revision,
				    result.stock_revision,     result.cash_revision,
				    result.wallet_mapping_id,  result.item_owner_revision };
	for (size_t index = 0; index != 6; ++index)
		put(candidate.data() + 8 + 8 * index, fields[index]);
	std::copy(result.image_digest.begin(), result.image_digest.end(), candidate.begin() + 56);
	std::copy(result.plan_digest.begin(), result.plan_digest.end(), candidate.begin() + 88);
	*output = candidate;
	return true;
}

bool native_mobile_birth_result_decode(std::span<const uint8_t> input,
				       native_mobile_birth_result *output) noexcept
{
	if (!output || input.size() != NATIVE_MOBILE_BIRTH_RESULT_BYTES || input[0] != 'M' ||
	    input[1] != 'B' || input[2] != 'R' || input[3] != '1' || input[4] != 1 || input[5] ||
	    input[6] || input[7])
		return false;
	native_mobile_birth_result candidate{};
	candidate.mobile_instance_id = get(input.data() + 8);
	candidate.mobile_revision = get(input.data() + 16);
	candidate.stock_revision = get(input.data() + 24);
	candidate.cash_revision = get(input.data() + 32);
	candidate.wallet_mapping_id = get(input.data() + 40);
	candidate.item_owner_revision = get(input.data() + 48);
	std::copy_n(input.begin() + 56, candidate.image_digest.size(),
		    candidate.image_digest.begin());
	std::copy_n(input.begin() + 88, candidate.plan_digest.size(),
		    candidate.plan_digest.begin());
	if (!valid(candidate))
		return false;
	*output = candidate;
	return true;
}

economic_accounting_error native_mobile_birth_result_build(
	const critical_command &command, const economic_account_key &wallet,
	const economic_accounting_plan &plan, native_mobile_birth_result *output) noexcept
{
	using error = economic_accounting_error;
	if (!output)
		return error::invalid_identity;
	try
	{
		quest_mobile_native_image image;
		auto status = native_mobile_birth_command_decode(command, &image);
		if (status != error::ok)
			return status;
		economic_accounting_plan expected;
		status = native_mobile_birth_accounting_compile(command, wallet, &expected);
		if (status != error::ok)
			return status;
		economic_digest expected_digest{};
		status = economic_plan_digest(expected, &expected_digest);
		if (status != error::ok)
			return status;
		native_mobile_birth_result candidate{};
		status = economic_plan_digest(plan, &candidate.plan_digest);
		if (status != error::ok)
			return status;
		if (candidate.plan_digest != expected_digest)
			return error::payload_conflict;
		candidate.mobile_instance_id = image.reference.mobile_instance_id;
		candidate.mobile_revision = image.reference.mobile_revision;
		candidate.stock_revision = image.reference.stock_revision;
		candidate.cash_revision = image.cash->revision;
		candidate.wallet_mapping_id = wallet.authority_id;
		candidate.item_owner_revision = 1;
		if (!SHA256(command.payload.data(), command.payload.size(),
			    candidate.image_digest.data()) ||
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

bool native_mobile_birth_result_matches(const critical_command &command,
					const economic_account_key &wallet,
					const economic_accounting_plan &plan,
					const native_mobile_birth_result &result) noexcept
{
	native_mobile_birth_result expected{};
	return native_mobile_birth_result_build(command, wallet, plan, &expected) ==
		       economic_accounting_error::ok &&
	       equal(expected, result);
}
