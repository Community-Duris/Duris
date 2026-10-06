#ifndef NPC_ALCHEMIST_H
#define NPC_ALCHEMIST_H
#include "core/structs.h"
void npc_alchemist_cache_templates();
int npc_alchemist_caster_interval(P_char ch);
bool npc_alchemist_combat(P_char ch);
// Only finalized fresh zone spawns call this; restoration and summons do not.
void npc_alchemist_world_spawn(P_char ch);

#include <cstdint>

enum class native_alchemist_vial_choice : uint8_t
{
	not_attempted = 0,
	missed = 1,
	selected = 2,
};

class quest_mobile_native_birth_owner;
// Decision/latch values provide no source, UID, SQL or publication authority.
class npc_alchemist_original_birth
{
	friend class quest_mobile_native_birth_owner;
	static bool capture(P_char exact_detached_actor, int original_room_rnum,
			    native_alchemist_vial_choice *) noexcept;
	static bool restore_latch(P_char, native_alchemist_vial_choice) noexcept;
};
#endif
