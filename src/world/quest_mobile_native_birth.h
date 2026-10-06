#ifndef QUEST_MOBILE_NATIVE_BIRTH_H
#define QUEST_MOBILE_NATIVE_BIRTH_H

#include "core/structs.h"
#include "persistence/critical_command_coordinator.h"

// Passive replay and game-thread lifecycle entry points. Neither values nor
// these observations authorize a native birth, SQL mutation or publication ACK.
bool quest_mobile_native_birth_restore(const critical_command &) noexcept;
bool quest_mobile_native_birth_restore(const critical_native_recovery_envelope &) noexcept;
void quest_mobile_native_birth_replay_ready(bool) noexcept;
void quest_mobile_native_birth_completions(const critical_completion *, size_t) noexcept;
void quest_mobile_native_birth_pulse(bool prepare_original_resets) noexcept;
bool quest_mobile_native_birth_lifecycle_ready() noexcept;

struct native_mobile_birth_recovery_context;
struct native_mobile_birth_recovery_effect;
class quest_mobile_native_birth_owner final
{
	friend void reset_zone(int, int);
	friend bool quest_mobile_native_birth_restore(const critical_command &) noexcept;
	friend bool
	quest_mobile_native_birth_restore(const critical_native_recovery_envelope &) noexcept;
	friend void quest_mobile_native_birth_completions(const critical_completion *,
							  size_t) noexcept;
	friend void quest_mobile_native_birth_pulse(bool) noexcept;
	static bool begin_reset(int zone, int force) noexcept;
	static void finish_reset() noexcept;
	static void seal_mobile() noexcept;
	static void block_mobile() noexcept;
	static P_char prepare_mobile(int rnum, int room, uint32_t slot, int shop) noexcept;
	static bool capture_alchemist_spawn(P_char, int original_room) noexcept;
	static P_obj prepare_item(int rnum) noexcept;
	static bool owns(P_char) noexcept;
	static bool discard_item(P_obj) noexcept;
	static bool carry(P_obj, P_char) noexcept;
	static bool equip(P_obj, P_char, int) noexcept;
	static bool nest(P_obj, P_obj, P_char) noexcept;
	static P_obj original_object(int rnum) noexcept;
	static void skipped_item(P_char, P_obj) noexcept;
	static size_t pending_mobiles(int rnum) noexcept;
	static size_t pending_items(int rnum) noexcept;
	static bool pending_shop(int rnum, int room, int shop) noexcept;
	static bool restore(const critical_command &) noexcept;
	static void completions(const critical_completion *, size_t) noexcept;
	static void pulse(bool) noexcept;
	static bool charge() noexcept;
	static bool discard(size_t) noexcept;
	static bool publish(size_t, bool allow_reconstruction = false) noexcept;
	static bool recover_cold(size_t, bool allow_reconstruction) noexcept;
	static bool clear_cold_projection(size_t) noexcept;
	static bool resume_cold_projection(size_t) noexcept;
	static bool settle_checkpoint(size_t) noexcept;
	static bool checkpoint(size_t, const native_mobile_birth_recovery_context &) noexcept;
	static int prepare_action(size_t, uint8_t, size_t, size_t) noexcept;
	static bool finish_action(size_t, const native_mobile_birth_recovery_effect &) noexcept;
	static bool restore(const critical_native_recovery_envelope &) noexcept;
	static bool cleanup_refusal(const critical_command &, const critical_completion &,
				    void *) noexcept;
};
#endif