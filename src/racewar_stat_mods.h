#ifndef _RACEWAR_STAT_MODS_H_
#define _RACEWAR_STAT_MODS_H_

#include <vector>
using namespace std;

#include "structs.h"

int add_racewar_stat_mods(P_char ch, struct hold_data *affs);
int set_racewar_stat_mod(int racewar, int stat_affect, int regular_stat, int max_stat);
void reset_racewar_stat_mods();

#endif
