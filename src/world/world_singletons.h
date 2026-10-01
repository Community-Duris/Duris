#ifndef WORLD_SINGLETONS_H
#define WORLD_SINGLETONS_H

#include "core/structs.h"

int singleton_shop_id(P_char keeper);
bool is_replicated_shop(int shop);
void bind_shopkeeper(P_char keeper, int shop_nr);
void remember_boot_shopkeepers();
bool snapshot_shopkeepers_for_copyover();
void reconcile_shopkeepers(bool recovered_inventory);

#endif
