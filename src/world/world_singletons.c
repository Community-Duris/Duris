#include "world/world_singletons.h"

#include "core/prototypes.h"
#include "core/utils.h"
#include <unordered_set>
#include <vector>

extern P_char character_list;
extern P_room world;
extern struct shop_data *shop_index;
extern int number_of_shops;
extern int top_of_world;

namespace
{
std::unordered_set<P_char> boot_shopkeepers;

size_t keeper_stock(P_char keeper)
{
	size_t count = 0;
	for (P_obj object = keeper->carrying; object; object = object->next_content)
		++count;
	for (int slot = 0; slot < MAX_WEAR; ++slot)
		if (keeper->equipment[slot])
			++count;
	return count;
}
}

int singleton_shop_id(P_char keeper)
{
	if (!keeper || !IS_NPC(keeper) || GET_MASTER(keeper))
		return -1;
	const int room = keeper->in_room >= 0 && keeper->in_room <= top_of_world ?
				 world[keeper->in_room].number :
				 -1;
	int home = -1;
	int roaming = -1;
	for (int shop = 0; shop < number_of_shops; ++shop)
	{
		if (shop_index[shop].keeper != GET_RNUM(keeper))
			continue;
		if (shop_index[shop].in_room == room)
			return shop;
		if (shop_index[shop].in_room == GET_BIRTHPLACE(keeper))
			home = shop;
		if (shop_index[shop].shop_is_roaming)
			roaming = roaming == -1 ? shop : -2;
	}
	return home >= 0 ? home : roaming;
}

void remember_boot_shopkeepers()
{
	boot_shopkeepers.clear();
	for (P_char keeper = character_list; keeper; keeper = keeper->next)
		if (singleton_shop_id(keeper) >= 0)
			boot_shopkeepers.insert(keeper);
}

void reconcile_shopkeepers(bool recovered_inventory)
{
	std::vector<std::vector<P_char>> by_shop(number_of_shops);
	for (P_char candidate = character_list; candidate; candidate = candidate->next)
	{
		const int shop = singleton_shop_id(candidate);
		if (shop >= 0)
			by_shop[shop].push_back(candidate);
	}
	for (int shop = 0; shop < number_of_shops; ++shop)
	{
		const auto &candidates = by_shop[shop];
		P_char keeper = nullptr;
		for (P_char candidate : candidates)
		{
			const bool preferred = boot_shopkeepers.count(candidate) != 0;
			const bool incumbent_preferred = keeper &&
							 boot_shopkeepers.count(keeper) != 0;
			if (!keeper || (preferred && !incumbent_preferred) ||
			    (preferred == incumbent_preferred &&
			     (keeper_stock(candidate) > keeper_stock(keeper) ||
			      (keeper_stock(candidate) == keeper_stock(keeper) &&
			       GET_IDNUM(candidate) < GET_IDNUM(keeper)))))
				keeper = candidate;
		}
		if (!keeper)
			continue;
		for (P_char duplicate : candidates)
		{
			if (duplicate == keeper)
				continue;
			// File recovery carries actual stock. Keep distinct non-produced items
			// from old duplicate keepers; Redis only carries regenerated templates.
			if (recovered_inventory && boot_shopkeepers.count(keeper) == 0)
			{
				for (int slot = 0; slot < MAX_WEAR; ++slot)
					if (duplicate->equipment[slot])
					{
						P_obj object = unequip_char(duplicate, slot);
						if (!keeper->equipment[slot])
							equip_char(keeper, object, slot, 0);
						else
							obj_to_char(object, keeper);
					}
				while (duplicate->carrying)
				{
					P_obj object = duplicate->carrying;
					obj_from_char(object);
					if (shop_producing(object, shop))
						extract_obj(object);
					else
						obj_to_char(object, keeper);
				}
			}
			extract_char(duplicate);
		}
		for (int item = 0; item < shop_index[shop].number_items_produced; ++item)
		{
			const int rnum = shop_index[shop].producing[item];
			bool found = false;
			for (P_obj object = keeper->carrying; object; object = object->next_content)
				if (object->R_num == rnum)
					found = true;
			if (!found && rnum >= 0)
				if (P_obj object = read_object(rnum, REAL))
					obj_to_char(object, keeper);
		}
		shop_index[shop].dirty = 1;
		if (candidates.size() > 1)
			logit(LOG_STATUS, "world singleton: shop=%d retained=%ld removed=%zu", shop,
			      static_cast<long>(GET_IDNUM(keeper)), candidates.size() - 1);
	}
	boot_shopkeepers.clear();
}
