#ifndef DURIS_SQL_PLAYER_MIGRATION_H_INCLUDED
#define DURIS_SQL_PLAYER_MIGRATION_H_INCLUDED

#include "core/structs.h"

P_char sql_load_player(const char *name);
bool sql_migrate_player(const char *name);
bool sql_verify_player(const char *name);
int sql_migrate_all_players(void);

#endif // DURIS_SQL_PLAYER_MIGRATION_H_INCLUDED
