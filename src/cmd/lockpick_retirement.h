#ifndef DURIS_LOCKPICK_RETIREMENT_H
#define DURIS_LOCKPICK_RETIREMENT_H
#include "item/item_movement_transaction.h"
#include "item/lockpick_retirement_continuation.h"

// Only the active do_pick branch calls this at its original selected break cut.
// Definite refusal preserves the actual held graph; uncertainty retains its root.
bool lockpick_retirement_submit(P_char, P_obj, lockpick_retirement_branch) noexcept;
// Shared movement owner supplies its original committed receipt/runtime cut.
// This callback is physical publication only, never independent ACK authority.
bool lockpick_retirement_publication(const critical_operation_id &, P_char, bool,
				     const item_transfer_result &, unsigned int, const uint8_t *,
				     size_t) noexcept;
// The typed owner suppresses an already-started notice on authentic cold resume.
bool lockpick_retirement_publish_physical(const critical_operation_id &, P_char, bool,
					  const item_transfer_result &,
					  const lockpick_retirement_terms &, bool notify) noexcept;
#endif
