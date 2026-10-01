#ifndef NPC_ALCHEMIST_H
#define NPC_ALCHEMIST_H
#include "core/structs.h"
void npc_alchemist_cache_templates();
int npc_alchemist_caster_interval(P_char ch);
bool npc_alchemist_combat(P_char ch);
// Only finalized fresh zone spawns call this; restoration and summons do not.
void npc_alchemist_world_spawn(P_char ch);
#endif
