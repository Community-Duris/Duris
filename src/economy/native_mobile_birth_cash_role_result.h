#ifndef NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_H
#define NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_H

#include "economy/native_mobile_birth_cash_role_accounting.h"

#include <array>
#include <span>

constexpr uint16_t NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_VERSION = 4;
constexpr size_t NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES = 264;

// Distinct MBR4 values; MBR1 and its nonzero ordinary wallet remain untouched.
// The role fixes item owner type/context: ordinary native_mobile/0, shared
// shopkeeper/0. Explicit owner ID is native UID or the selected shop+1 key.
// Full original payload/image/role/plan bindings grant no source, current-row,
// factory, CAS, publication or ACK authority. Unused branch fields must be zero.
struct native_mobile_birth_cash_role_result
{
	native_mobile_birth_cash_role role{};
	uint64_t mobile_instance_id = 0, mobile_revision = 0, stock_revision = 0, cash_revision = 0;
	uint64_t wallet_mapping_id = 0, item_owner_id = 0, item_owner_revision = 0;
	native_mobile_birth_shared_shop_participant shared;
	economic_digest image_digest{}, payload_digest{}, role_digest{}, plan_digest{};
};

bool native_mobile_birth_cash_role_result_encode(
	const native_mobile_birth_cash_role_result &,
	std::array<uint8_t, NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES> *) noexcept;
bool native_mobile_birth_cash_role_result_decode(std::span<const uint8_t>,
						 native_mobile_birth_cash_role_result *) noexcept;

// Recompile exact role-specific effects and compare complete canonical plan
// bytes before constructing a result from genuine supplied participant values.
// Strong output. Distinct overloads refuse the wrong NBC4 role.
economic_accounting_error
native_mobile_birth_cash_role_result_build(const critical_command &, const economic_account_key &,
					   const economic_accounting_plan &,
					   native_mobile_birth_cash_role_result *) noexcept;
economic_accounting_error native_mobile_birth_cash_role_result_build(
	const critical_command &, const native_mobile_birth_shared_shop_participant &,
	const economic_accounting_plan &, native_mobile_birth_cash_role_result *) noexcept;
bool native_mobile_birth_cash_role_result_matches(
	const critical_command &, const economic_account_key &, const economic_accounting_plan &,
	const native_mobile_birth_cash_role_result &) noexcept;
bool native_mobile_birth_cash_role_result_matches(
	const critical_command &, const native_mobile_birth_shared_shop_participant &,
	const economic_accounting_plan &, const native_mobile_birth_cash_role_result &) noexcept;

#endif
