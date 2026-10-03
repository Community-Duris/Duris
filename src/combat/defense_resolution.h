#ifndef DURIS_COMBAT_DEFENSE_RESOLUTION_H
#define DURIS_COMBAT_DEFENSE_RESOLUTION_H

#include "core/structs.h"

int generic_parry_proc(P_obj obj, P_char ch, int cmd, char *arg);
int generic_riposte_proc(P_obj obj, P_char ch, int cmd, char *arg);

int leapSucceed(P_char victim, P_char attacker);
int dodgeSucceed(P_char char_dodger, P_char attacker, P_obj wpn);
int blockSucceed(P_char victim, P_char attacker, P_obj wpn);
int MonkRiposte(P_char victim, P_char attacker, P_obj wpn);
int parrySucceed(P_char victim, P_char attacker, P_obj wpn);
bool mangleSucceed(P_char ch, P_char victim, P_obj weap);
bool fear_check(P_char ch, bool force = false);

#endif
