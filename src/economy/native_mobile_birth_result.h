#ifndef NATIVE_MOBILE_BIRTH_RESULT_H
#define NATIVE_MOBILE_BIRTH_RESULT_H

#include "economy/native_mobile_birth_accounting.h"

#include <array>
#include <span>

constexpr size_t NATIVE_MOBILE_BIRTH_RESULT_BYTES = 120;
constexpr uint16_t NATIVE_MOBILE_BIRTH_RESULT_VERSION = 1;
constexpr uint16_t NATIVE_MOBILE_BIRTH_OUTBOX_DESTINATION = 13;
constexpr uint16_t NATIVE_MOBILE_BIRTH_OUTBOX_EVENT = 1;

// Small typed value result for the existing inbox/outbox. The complete original
// image remains in the command/journal; these digests cannot authenticate SQL,
// current source eligibility, native publication or ACK.
struct native_mobile_birth_result
{
	uint64_t mobile_instance_id = 0;
	uint64_t mobile_revision = 0;
	uint64_t stock_revision = 0;
	uint64_t cash_revision = 0;
	uint64_t wallet_mapping_id = 0;
	uint64_t item_owner_revision = 0;
	economic_digest image_digest{};
	economic_digest plan_digest{};
};

bool native_mobile_birth_result_encode(
	const native_mobile_birth_result &,
	std::array<uint8_t, NATIVE_MOBILE_BIRTH_RESULT_BYTES> *) noexcept;
bool native_mobile_birth_result_decode(std::span<const uint8_t>,
				       native_mobile_birth_result *) noexcept;

// Pure exact expected-result construction. The mapped key is a supplied value;
// only the original SQL owner can prove its created/retained mapping lifetime.
// Recompiles the actual original command with that key and checks the supplied
// plan before constructing any output. No current rows or epoch are consulted.
economic_accounting_error native_mobile_birth_result_build(const critical_command &,
							   const economic_account_key &,
							   const economic_accounting_plan &,
							   native_mobile_birth_result *) noexcept;
bool native_mobile_birth_result_matches(const critical_command &, const economic_account_key &,
					const economic_accounting_plan &,
					const native_mobile_birth_result &) noexcept;

#endif
