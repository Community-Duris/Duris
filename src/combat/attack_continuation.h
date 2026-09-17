#ifndef DURIS_ATTACK_CONTINUATION_H
#define DURIS_ATTACK_CONTINUATION_H

#include "core/structs.h"

#include <cstdint>

/*
 * A combat callback may synchronously kill, extract, relocate, or replace a
 * participant or the selected weapon.  Callers must check this contract before
 * reading any borrowed state after such a callback.
 */
enum class attack_continuation_outcome
{
	continue_attack,
	actor_gone,
	target_gone,
	relocated,
	weapon_changed,
	cancelled
};

struct attack_continuation
{
	P_char actor = nullptr;
	P_char target = nullptr;
	P_obj weapon = nullptr;
	uint64_t actor_runtime_id = 0;
	uint64_t target_runtime_id = 0;
	uint64_t weapon_uid = 0;
	int room = NOWHERE;
	int height = 0;
	int weapon_slot = -1;
};

struct attack_continuation_result
{
	attack_continuation_outcome outcome = attack_continuation_outcome::cancelled;
	P_char actor = nullptr;
	P_char target = nullptr;
	P_obj weapon = nullptr;

	bool can_continue() const noexcept
	{
		return outcome == attack_continuation_outcome::continue_attack;
	}
};

/*
 * Capture identities and the location policy before entering a callback. The
 * location policy rejects a participant that is no longer in the captured
 * room/height; it intentionally does not claim to detect an away-and-back move
 * without a departure epoch in the character lifecycle.
 */
attack_continuation begin_attack_continuation(P_char actor, P_char target, P_obj weapon = nullptr,
					      int weapon_slot = -1) noexcept;

/* Re-resolve all captured state; returned pointers are borrowed for this call. */
attack_continuation_result check_attack_continuation(const attack_continuation &) noexcept;

#endif
