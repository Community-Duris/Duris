#ifndef PLAYER_DEATH_RESTITUTION_LOCKER_H
#define PLAYER_DEATH_RESTITUTION_LOCKER_H

#include "core/structs.h"

constexpr int PLAYER_DEATH_RESTITUTION_BAG_VNUM = 7670;
constexpr const char *PLAYER_DEATH_RESTITUTION_BAG_MARKER = "restitution_lost_items";

bool player_death_restitution_is_locker_bag(P_obj obj);
void player_death_restitution_locker_notice(P_char ch);

#endif
