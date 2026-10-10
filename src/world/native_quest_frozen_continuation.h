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
class item_native_recovery_replay_owner;
class player_save_coin_replay_budget_scope_owner;
bool quest_native_current_source_frames(size_t *) noexcept;
size_t quest_native_current_source_profile_query_frames() noexcept;
size_t quest_native_restore_phase2_source_frame_bytes() noexcept;
size_t quest_native_restore_phase2_source_profile_query_frames() noexcept;
size_t quest_native_restore_phase2_entry_inline_bytes() noexcept;
constexpr size_t quest_native_restore_phase2_entry_inline_query_frames() noexcept
{
	return sizeof(size_t);
}

// Pure whole original quest continuation/gameplay/reward CURRENT. Includes live
// and passive maps, original reward recovery, unique flows/commands/envelopes
// and branch programs. Read with item CURRENT providers in the same actual
// game-thread cut: commands allocated in those providers are excluded here.
// Native pools/gameplay bodies remain the selecting ROOT G's ownership.
// Strong output; no budget, persistence, readiness or publication operation.
bool quest_native_retained_storage_bytes(size_t *) noexcept;

// Private original publication owner only, after its guarded ACK/release and
// exact same-session original root/obligation/native proof. Neither a record
// value nor a caller boolean supplies that authority to ordinary callers.
class quest_native_frozen_continuation_owner final
{
    private:
	friend class item_native_quest_publication_owner;
	friend class item_native_recovery_replay_owner;
	friend class quest_native_gameplay_owner;
	friend class quest_native_reward_ack_owner;
	static bool restore(const critical_native_recovery_envelope &) noexcept;
	static bool restore_bounded(const critical_native_recovery_envelope &,
				    player_save_coin_replay_budget_scope_owner &,
				    bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
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
