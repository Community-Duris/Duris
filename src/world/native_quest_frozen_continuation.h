#ifndef NATIVE_QUEST_FROZEN_CONTINUATION_H
#define NATIVE_QUEST_FROZEN_CONTINUATION_H

#include "persistence/critical_command.h"
#include <cstdint>
#include <cstddef>

struct critical_native_recovery_envelope;
struct quest_reward_obligation_record;
struct quest_reward_ack_completion;
struct native_quest_frozen_recovery_state;
class item_native_quest_publication_owner;

// Private original publication owner only, after its guarded ACK/release and
// exact same-session original root/obligation/native proof. Neither a record
// value nor a caller boolean supplies that authority to ordinary callers.
class quest_native_frozen_continuation_owner final
{
    private:
	friend class item_native_quest_publication_owner;
	friend class quest_native_gameplay_owner;
	friend class quest_native_reward_ack_owner;
	static bool restore(const critical_native_recovery_envelope &) noexcept;
	// Post-ACK handoff only. Copies the actual coordinator continuation after
	// the guarded ACK has returned; never called by the startup observer.
	static bool published(const critical_command &) noexcept;
	// Only the net owner forwards genuine drained original reward ACKs.
	static void acknowledged(const quest_reward_ack_completion &) noexcept;
	// Only the original gameplay pulse settles existing writers/observed ACK.
	static void pulse() noexcept;
	static bool drive(native_quest_frozen_recovery_state &, bool allow_effects) noexcept;
	static bool checkpoint(native_quest_frozen_recovery_state &) noexcept;
	static bool retire_completed(native_quest_frozen_recovery_state &) noexcept;
	static bool budget(const native_quest_frozen_recovery_state *, size_t) noexcept;
	static bool publish(const critical_command &, const quest_reward_obligation_record &,
			    uint64_t original_player_runtime_id, bool *started) noexcept;
};

#endif
