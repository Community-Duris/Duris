#ifndef DURIS_COMBAT_ATTACK_EFFECTS_H
#define DURIS_COMBAT_ATTACK_EFFECTS_H

#include "core/structs.h"
#include <stddef.h>

extern struct attack_hit_type attack_hit_text[];
bool tainted_blade(P_char ch, P_char victim);
int anatomy_strike(P_char ch, P_char victim, int msg, struct damage_messages *messages,
		   char *attacker_msg, char *victim_msg, char *room_msg, size_t msg_size, int dam);
int battle_frenzy(P_char ch, P_char victim);
bool monk_critic(P_char ch, P_char victim, int *damAccumulator);

#endif
