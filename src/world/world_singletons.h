#ifndef WORLD_SINGLETONS_H
#define WORLD_SINGLETONS_H

#include "core/structs.h"

int singleton_shop_id(P_char keeper);
void remember_boot_shopkeepers();
void reconcile_shopkeepers(bool recovered_inventory);

#endif
