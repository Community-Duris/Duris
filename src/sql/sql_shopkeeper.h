#ifndef DURIS_SQL_SHOPKEEPER_H_INCLUDED
#define DURIS_SQL_SHOPKEEPER_H_INCLUDED

#include "core/structs.h"

bool sql_save_shopkeeper(P_char ch, int shop_nr);
bool sql_delete_shopkeeper(int shop_nr);
P_char sql_restore_shopkeeper(int shop_nr);
bool sql_restore_shopkeepers(void);
bool sql_save_dirty_shopkeepers(bool force = false);

#endif // DURIS_SQL_SHOPKEEPER_H_INCLUDED
