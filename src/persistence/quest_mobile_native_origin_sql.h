#ifndef QUEST_MOBILE_NATIVE_ORIGIN_SQL_H
#define QUEST_MOBILE_NATIVE_ORIGIN_SQL_H

#include "economy/native_mobile_birth_recovery.h"
#include "persistence/quest_mobile_native_sql.h"

// Immutable published constructor evidence in the existing native lifetime
// domain. A historical absence is unknown, never a constructor to synthesize.
struct quest_mobile_native_published_origin
{
	bool present = false;
	critical_native_recovery_envelope original;
};

// The genuine birth owner supplies its actual terminal phase2 envelope. The
// caller owns reconnect-disabled IN_TRANS and must confirm its original commit
// before journal retirement. Proves original receipt/root/current born image and
// complete custody; only identical existing origin bytes permit a retry. No
// transaction boundary, journal/world effects, IDs, source or ACK authority.
int quest_mobile_native_origin_sql_retain_locked(
	MYSQL *, const critical_native_recovery_envelope &) noexcept;

// Call BEFORE locking current native/item custody: this owner establishes the
// original birth-inbox -> current native -> immutable origin lock order. Full
// original command/receipt and immutable native reference are authenticated;
// progressed current revisions are permitted. Current stock/cash/custody and
// world absence remain independently required before reconstruction. Strong
// output on refusal, including query/session/allocation/codec errors.
int quest_mobile_native_origin_sql_lock(MYSQL *, const quest_mobile_native_reference &,
					quest_mobile_native_published_origin *) noexcept;

// Distinct ordinary NMB4 / MBR4 published origins. Shared/unknown roles refuse;
// old entrypoints retain their historical command/result policy. The complete
// original prospective terminal attachment is stored byte-for-byte in the SAME
// existing origin table, under the same original session/lock/DML contract.
// Retention authenticates the exact still-born publication image/custody/wallet.
// Lock authenticates historical committed origin first, then the exact supplied
// current lifetime reference; advanced cash/stock revisions remain permitted.
// An absent origin remains present=false and grants no reconstruction authority.
// No COMMIT, journal/world effects, schema, source admission or ACK authority.
int quest_mobile_native_origin_sql_retain_ordinary_wallet_locked(
	MYSQL *, const critical_native_recovery_envelope &) noexcept;
int quest_mobile_native_origin_sql_lock_ordinary_wallet(
	MYSQL *, const quest_mobile_native_reference &,
	quest_mobile_native_published_origin *) noexcept;

// Distinct initial shared SHOP NMB4/MBR4 origin policy, in the SAME existing
// origin table. Retention requires the genuine successful terminal envelope,
// its complete original checkpoint and full still-born SQL publication proof.
// Caller owns the original reconnect-disabled IN_TRANS session and confirms
// COMMIT before journal retirement. Identical retained bytes alone permit retry.
// Reader authenticates canonical shared terminal/history before current native
// lifetime and immutable origin locks; advanced native/stock revisions are
// permitted. Current SHOP/owner/cash/custody/physical/world proof is separate.
// No historical/ordinary predicate is broadened. Absence is unknown, not a
// reconstruction permission. No transaction lifecycle, source, world or ACK
// authority is granted. All refusing reads preserve the caller's output.
int quest_mobile_native_origin_sql_retain_shared_shop_locked(
	MYSQL *, const critical_native_recovery_envelope &) noexcept;
int quest_mobile_native_origin_sql_lock_shared_shop(MYSQL *, const quest_mobile_native_reference &,
						    quest_mobile_native_published_origin *) noexcept;

#endif
