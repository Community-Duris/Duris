#ifndef NATIVE_MOBILE_BIRTH_CASH_ROLE_COMMAND_H
#define NATIVE_MOBILE_BIRTH_CASH_ROLE_COMMAND_H

#include "economy/native_mobile_birth_command.h"
#include "economy/native_mobile_birth_cash_role_recipe.h"

// Prospective only; the original version1/2/3 support predicate stays unchanged.
constexpr uint16_t NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION = 4;

// Complete original NMB3 image/stock/NBC3 bytes plus explicit accepted NBC4 role.
// The selected shared shop key uses original shop+1; no shop row CAS is invented.
// Pure structural values grant no factory, source claim, mapping, readiness,
// account compiler, storage, publication or ACK authority. Strong output.
economic_accounting_error native_mobile_birth_cash_role_command_build(
	const economic_operation_metadata &, const quest_mobile_native_image &,
	std::span<const native_mobile_birth_item_recipe>,
	const native_mobile_birth_cash_role_recipe &, critical_source_site,
	uint64_t accepted_at_usec, critical_command *) noexcept;

// Requires version4 and exact full intent/envelope/payload/key rebuild equality.
// Reuses original v3 validation only through a separately rebound projection;
// neither projection nor matching embedded constructor confers source authority.
// All outputs remain unchanged on every structural/allocation refusal.
economic_accounting_error
native_mobile_birth_cash_role_command_decode(const critical_command &, quest_mobile_native_image *,
					     std::vector<native_mobile_birth_item_recipe> *,
					     native_mobile_birth_cash_role_recipe *) noexcept;

#endif
