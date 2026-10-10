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

// Prospective complete role result build/matches. Caller includes inputs, old
// output and inline output result in outer_live, retaining absolute simultaneous
// admission through transfer. Full original repeated command/compiler proof,
// canonical plan pair, image/role/payload/plan digests and exact result equality
// remain required. Supported requests require GCC13 libstdc++ C++11 ABI.
// Build output stays strong; matching remains false on any refusal. A caller's
// sticky forwarding callback may distinguish its budget refusal without errno.
// These helpers grant no source, wallet, SHOP, storage or ACK authority.
economic_accounting_error native_mobile_birth_cash_role_result_build_bounded(
	const critical_command &, const economic_account_key &, const economic_accounting_plan &,
	native_mobile_birth_cash_role_result *, bool (*)(size_t, void *) noexcept, void *,
	size_t outer_live) noexcept;
economic_accounting_error native_mobile_birth_cash_role_result_build_bounded(
	const critical_command &, const native_mobile_birth_shared_shop_participant &,
	const economic_accounting_plan &, native_mobile_birth_cash_role_result *,
	bool (*)(size_t, void *) noexcept, void *, size_t outer_live) noexcept;
bool native_mobile_birth_cash_role_result_matches_bounded(
	const critical_command &, const economic_account_key &, const economic_accounting_plan &,
	const native_mobile_birth_cash_role_result &, bool (*)(size_t, void *) noexcept, void *,
	size_t outer_live) noexcept;
bool native_mobile_birth_cash_role_result_matches_bounded(
	const critical_command &, const native_mobile_birth_shared_shop_participant &,
	const economic_accounting_plan &, const native_mobile_birth_cash_role_result &,
	bool (*)(size_t, void *) noexcept, void *, size_t outer_live) noexcept;

// Full original MBR4 result route, retaining complete canonical plan-pair proof
// and four original digest inputs. Additive, strong typed outputs. New entries
// self-own SOURCE/entry objects; old outputs and inputs remain in outer.
economic_accounting_error native_mobile_birth_cash_role_result_build_fixed_bounded(
	const critical_command &, const economic_account_key &, const economic_accounting_plan &,
	native_mobile_birth_cash_role_result *, bool (*)(size_t, void *) noexcept, void *,
	size_t) noexcept;
economic_accounting_error native_mobile_birth_cash_role_result_build_fixed_bounded(
	const critical_command &, const native_mobile_birth_shared_shop_participant &,
	const economic_accounting_plan &, native_mobile_birth_cash_role_result *,
	bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
economic_accounting_error native_mobile_birth_cash_role_result_matches_fixed_bounded(
	const critical_command &, const economic_account_key &, const economic_accounting_plan &,
	const native_mobile_birth_cash_role_result &, bool *, bool (*)(size_t, void *) noexcept,
	void *, size_t) noexcept;
economic_accounting_error native_mobile_birth_cash_role_result_matches_fixed_bounded(
	const critical_command &, const native_mobile_birth_shared_shop_participant &,
	const economic_accounting_plan &, const native_mobile_birth_cash_role_result &, bool *,
	bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
economic_accounting_error native_mobile_birth_cash_role_result_decode_fixed_bounded(
	std::span<const uint8_t>, native_mobile_birth_cash_role_result *,
	bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
bool native_mobile_birth_cash_role_result_fixed_source_frame_bytes(size_t *) noexcept;
bool native_mobile_birth_cash_role_result_fixed_initial_inline_bytes(size_t *) noexcept;
constexpr size_t native_mobile_birth_cash_role_result_fixed_source_query_frame_bytes() noexcept
{
	// Own getter (output/two scalar totals and conditions), actual image lifetime
	// and recipe lifecycle queries, and two genuine checked-add helpers.
	return sizeof(size_t *) + 2 * sizeof(size_t) + 3 * sizeof(bool) +
	       quest_mobile_native_image_lifetime_source_query_frame_bytes() +
	       native_mobile_birth_recipe_source_query_frame_bytes() +
	       2 * (sizeof(size_t *) + sizeof(size_t) + sizeof(bool));
}

#endif
