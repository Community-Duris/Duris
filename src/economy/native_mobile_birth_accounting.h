#ifndef NATIVE_MOBILE_BIRTH_ACCOUNTING_H
#define NATIVE_MOBILE_BIRTH_ACCOUNTING_H

#include "economy/native_mobile_birth_command.h"

// Existing wallet kind, with the native custody type as its fixed context.
// Player wallets remain context zero. The wallet authority is the original SQL
// mapping lifetime, not the native UID. This context/key grants no capability.
constexpr uint64_t ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT =
	static_cast<uint64_t>(item_owner_type::native_mobile);

// Pure expected original birth plan. Canonical command/intent/image correlation
// supplies values only: known cash revision1 and the complete ordered born stock.
// original_native_wallet must be the exact created/locked mapping key retained by
// the original typed root and reused on replay. Its lineage/kind/context are checked;
// value validity does not prove mapping creation, ownership or source admission.
// No IDs, source admission, store reads/writes, publication, recovery or ACK.
// The atomic original owner must prove reserved identity, absent native/cash/item
// lifetimes and admitted generation/source in the same transaction before apply.
// All failure paths, including allocation/capacity, preserve the output.
economic_accounting_error
native_mobile_birth_accounting_compile(const critical_command &,
				       const economic_account_key &original_native_wallet,
				       economic_accounting_plan *) noexcept;

// Prospective compiler for the genuine v3 cash-role projection only. Historical
// v1/v2 remain handled by the unchanged original compiler; this companion returns
// unresolved for them and grants no additional source or execution authority.
// Caller includes inputs/old outputs/inline output plan in outer_live, retaining
// the admitted absolute simultaneous peak through transfer. Full original wallet,
// cash/effects/item DFS/metadata and normalized-plan proof remain required.
// Supported requests require GCC13 libstdc++ C++11 ABI. Plan and optional scalar
// are strong; actual transferred heap scalar excludes the caller inline plan.
economic_accounting_error native_mobile_birth_accounting_compile_v3_bounded(
	const critical_command &, const economic_account_key &original_native_wallet,
	economic_accounting_plan *, bool (*)(size_t, void *) noexcept, void *, size_t outer_live,
	size_t *retained_plan_heap_bytes = nullptr) noexcept;

#endif
