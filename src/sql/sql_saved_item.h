#ifndef DURIS_SQL_SAVED_ITEM_H_INCLUDED
#define DURIS_SQL_SAVED_ITEM_H_INCLUDED

#include "core/structs.h"

bool sql_save_saved_item(P_obj item, const char *item_key);
bool sql_delete_saved_item(const char *item_key);
void sql_restore_saved_items(void);

#endif // DURIS_SQL_SAVED_ITEM_H_INCLUDED
