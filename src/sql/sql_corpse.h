#ifndef DURIS_SQL_CORPSE_H_INCLUDED
#define DURIS_SQL_CORPSE_H_INCLUDED

#include "core/structs.h"

bool sql_save_corpse(P_obj corpse);
bool sql_delete_corpse(const char *player_name, int save_id);
bool sql_load_all_corpses(void);

#endif // DURIS_SQL_CORPSE_H_INCLUDED
