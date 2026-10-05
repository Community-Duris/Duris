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

#endif
