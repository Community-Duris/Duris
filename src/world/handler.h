#ifndef DURIS_WORLD_HANDLER_H
#define DURIS_WORLD_HANDLER_H

#include "core/structs.h"

enum class obj_to_char_result
{
	placed,
	deferred,
	rejected,
	destroyed,
};

// Only placed permits callers to keep using the object without another live lookup.
obj_to_char_result obj_to_char_checked(P_obj object, P_char character);

struct shop_trade_destination_weight;
// Original game-thread preparation only. Construction/extraction may throw;
// caller must retain its original started probe and never retry an uncertain
// tail. Success installs the actual native shell only after extraction returns.
bool obj_capture_container_shell_weight(P_obj, int32_t *output);
// Original admitted shop parent owns world/custody/complete literal source proof.
// This root-destination primitive shares normal insertion/activity/dirty tails,
// but applies the verified original weight correction without a second probe.
// Values and a true return do not grant SQL, receipt or ACK authority.
bool obj_to_obj_shop_frozen_weight(P_obj, P_obj, const shop_trade_destination_weight &frozen);

int can_prime_class_use_item(P_char, P_obj);

// A committed corpse raise may need to defer player serialization until the
// durable item rows have been rehydrated.  The login hook clears this fence
// only after the player's authoritative snapshot has been loaded.
bool corpse_raise_player_save_fenced(P_char);
void corpse_raise_player_ready(P_char, bool inventory_reloaded);
bool corpse_has_death_conflict(P_obj);

// Detached original reset topology only; world enrollment remains with the
// original birth owner after authenticated SQL publication. No caller values
// grant ownership/source or ACK authority.
class quest_mobile_native_birth_owner;
class quest_mobile_native_item_stage;
class quest_mobile_native_local_stock;
// Transient original returned shell-probe witness. Only the actual factory can
// construct a valid value; the detached placement consumes it. It grants no
// durable ownership, SQL, source, recovery or acknowledgement authority.
class quest_mobile_native_container_shell final
{
    public:
	quest_mobile_native_container_shell() noexcept = default;
	quest_mobile_native_container_shell(const quest_mobile_native_container_shell &) = delete;
	quest_mobile_native_container_shell &
	operator=(const quest_mobile_native_container_shell &) = delete;

    private:
	friend class quest_mobile_native_item_stage;
	friend class quest_mobile_native_local_stock;
	P_obj target_ = nullptr;
	int target_rnum_ = -1;
	int32_t shell_weight_ = 0;
	bool valid_ = false;
};
class quest_mobile_native_local_stock final
{
	friend class quest_mobile_native_birth_owner;
	static bool carry(P_obj, P_char) noexcept;
	static bool equip(P_obj, P_char, int) noexcept;
	static bool nest(P_obj, P_obj, P_char) noexcept;
	static bool nest(P_obj, P_obj, P_char, quest_mobile_native_container_shell &) noexcept;
	// Root retains any begun unpublished placement if its original probe fails.
	static bool begin_reducing_nest(P_obj, P_obj, P_char) noexcept;
	static bool finish_reducing_nest(P_obj, P_obj, P_char,
					 quest_mobile_native_container_shell &) noexcept;
	static bool detach(P_obj, P_char) noexcept;
	static bool enroll(P_obj, P_char) noexcept;
	static bool restore_enrollment(P_obj, P_char) noexcept;
};

class quest_mobile_native_stage;
// Callback-free room/runtime projection owned exclusively by the original
// native birth stage. Progress is retained by that stage after consumption.
class quest_mobile_native_room_restore_owner final
{
	friend class quest_mobile_native_stage;
	static bool restore(P_char, int room_rnum, size_t *retained_step) noexcept;
};

#endif
