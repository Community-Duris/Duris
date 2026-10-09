#ifndef NATIVE_MOBILE_BIRTH_CASH_ROLE_ACCOUNTING_H
#define NATIVE_MOBILE_BIRTH_CASH_ROLE_ACCOUNTING_H

#include "economy/native_mobile_birth_cash_role_command.h"
#include "economy/native_mobile_birth_accounting.h"

// Supplied original SHOP participant values, never a storage/source capability.
// Presence distinguishes a missing owner from a genuine zero owner clock.
// Birth-to-SHOP CAS/increment policy is not established by existing birth code;
// the future atomic owner must authenticate it and every supplied native fact.
struct native_mobile_birth_shared_shop_participant
{
	uint32_t shop_id = 0;
	bool shop_before_present = false, shop_after_present = false;
	bool owner_before_present = false, owner_after_present = false;
	uint64_t shop_revision_before = 0, shop_revision_after = 0;
	uint64_t owner_revision_before = 0, owner_revision_after = 0;
	economic_coin_vector born_cash{};
};

// Structural presence/cash shape only. No live source or CAS is authenticated.
bool native_mobile_birth_shared_shop_participant_valid(
	const native_mobile_birth_shared_shop_participant &) noexcept;

// Ordinary NBC4 only: exact original wallet compiler effects through a genuine
// separately rebuilt NMB3 projection, rebound to the complete original NMB4
// metadata. The original atomic owner must supply/prove the typed wallet key.
economic_accounting_error
native_mobile_birth_cash_role_accounting_compile(const critical_command &,
						 const economic_account_key &original_native_wallet,
						 economic_accounting_plan *) noexcept;

// Shared NBC4 only: full absent-to-born stock under original SHOP owner shop+1,
// preserving original equipped roots and child slots in custody and native image.
// Born cash is SHOP compatibility evidence, not a wallet/treasury/posting.
// All native stock must be genuinely absent before the future atomic owner
// applies this plan, preserving unrelated selected-shop custody and history.
// Strong outputs; supplied values do not authorize a factory, write or receipt.
economic_accounting_error native_mobile_birth_cash_role_accounting_compile(
	const critical_command &, const native_mobile_birth_shared_shop_participant &,
	economic_accounting_plan *) noexcept;

// Complete prospective NBC4 wallet/shared companions. Caller includes input,
// old output and inline output plan storage in outer_live and holds the admitted
// absolute simultaneous peak through transfer. Full original role/metadata,
// genuine wallet projection or SHOP participant/forest proof and normalization
// remain required; no inferred wallet/source/CAS/publication authority is added.
// Supported requests require GCC13 libstdc++ C++11 ABI. Both output plan and
// optional actual transferred heap scalar (excluding inline plan) remain strong.
economic_accounting_error native_mobile_birth_cash_role_accounting_compile_bounded(
	const critical_command &, const economic_account_key &original_native_wallet,
	economic_accounting_plan *, bool (*)(size_t, void *) noexcept, void *, size_t outer_live,
	size_t *retained_plan_heap_bytes = nullptr) noexcept;
economic_accounting_error native_mobile_birth_cash_role_accounting_compile_bounded(
	const critical_command &, const native_mobile_birth_shared_shop_participant &,
	economic_accounting_plan *, bool (*)(size_t, void *) noexcept, void *, size_t outer_live,
	size_t *retained_plan_heap_bytes = nullptr) noexcept;

#endif
