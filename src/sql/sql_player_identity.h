#ifndef DURIS_SQL_PLAYER_IDENTITY_H_INCLUDED
#define DURIS_SQL_PLAYER_IDENTITY_H_INCLUDED

#include "core/structs.h"

bool sql_player_exists(const char *name);
bool sql_player_rename(P_char ch, const char *new_name);
int sql_get_player_pid(const char *name);

#endif // DURIS_SQL_PLAYER_IDENTITY_H_INCLUDED
