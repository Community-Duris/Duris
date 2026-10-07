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

#endif
