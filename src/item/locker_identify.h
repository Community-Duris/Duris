#ifndef DURIS_LOCKER_IDENTIFY_H
#define DURIS_LOCKER_IDENTIFY_H
#include "core/structs.h"
void locker_identify(P_char ch, P_obj obj, int cost);
void locker_identify_pulse();
bool locker_identify_init(const char *journal_directory);
void locker_identify_shutdown();
void locker_identify_replay(P_char ch);
#endif
