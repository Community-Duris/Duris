#ifndef _TROPHY_H_
#define _TROPHY_H_

#include "core/structs.h"
#include "item/trophy_state.h"

// Called only after gain_exp has credited a positive award. Returns whether the
// caller must include trophies in the same dirty revision as player XP.
bool record_zone_trophy_award(P_char ch, P_char victim, int credited_xp, int type);
void clear_zone_trophy(P_char ch);

#define ZONE_TROPHY(ch) (ch->only.pc->zone_trophy)

#endif
