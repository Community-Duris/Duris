#ifndef DURIS_COIN_PHYSICAL_RECOVERY_H
#define DURIS_COIN_PHYSICAL_RECOVERY_H

#include "persistence/critical_command_completion.h"

// Classification only. Failure preserves supplied outputs; success grants no
// SQL authority, save reservation, physical publication or ACK permission.
bool coin_physical_recovery_identity(const critical_command &command, int *wallet_pid,
				     uint64_t *pile_uid) noexcept;

// Synchronous game-thread backend projection owner for the original single ordinary
// room pile drop/full pickup/partial pickup. Reacquires native current and retained
// authority on every call, including ACK retries. No actor is required. A true
// result requires confirmed SQL original-session cleanup or the original flat
// identity-then-authority lock through exact retained/current/native proof. This does NOT
// ACK or release a save hold; the caller retains its reservation and exact sealed
// receipt through guarded ACK. False retains the original operation unchanged.
// Native money rendering/disposal may be used; no domain completion callback,
// player command, template parser, normal object factory or UID issuance occurs.
// Flat central admission stays closed until shared hold/replay integration and qualification.
bool coin_physical_recovery_publish(const critical_command &command,
				    const critical_completion &sealed_completion) noexcept;

#endif
