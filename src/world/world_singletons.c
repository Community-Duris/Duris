#include "world/world_singletons.h"

#include "core/prototypes.h"
#include "core/utils.h"
#include "persistence/persistence_mode.h"
#include "sql/sql_player.h"
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
	const int bound = keeper->only.npc ? keeper->only.npc->shopkeeper_shop_id : -1;
	if (bound >= 0)
	{
		if (bound < number_of_shops && shop_index[bound].keeper == GET_RNUM(keeper))
			return bound;
		// A stale binding is safer than falling back to a template/room guess.
		return -1;
	}
	const int room = keeper->in_room >= 0 && keeper->in_room <= top_of_world ?
				 world[keeper->in_room].number :
				 -1;
	int home = -1;
	int room_match = -1;
	int roaming = -1;
	for (int shop = 0; shop < number_of_shops; ++shop)
	{
		if (shop_index[shop].keeper != GET_RNUM(keeper))
			continue;
		if (!shop_index[shop].shop_is_roaming && shop_index[shop].in_room == room)
		{
			if (room_match >= 0)
				return -1;
			room_match = shop;
		}
		if (!shop_index[shop].shop_is_roaming &&
		    shop_index[shop].in_room == GET_BIRTHPLACE(keeper))
			home = shop;
		if (shop_index[shop].shop_is_roaming)
			roaming = roaming == -1 ? shop : -2;
	}
	if (room_match >= 0)
		return room_match;
	return home >= 0 ? home : (roaming >= 0 ? roaming : -1);
}

void bind_shopkeeper(P_char keeper, int shop_nr)
{
	if (!keeper || !IS_NPC(keeper) || GET_MASTER(keeper) || !keeper->only.npc || !shop_index ||
	    shop_nr < 0 || shop_nr >= number_of_shops ||
	    shop_index[shop_nr].keeper != GET_RNUM(keeper))
		return;
	keeper->only.npc->shopkeeper_shop_id = shop_nr;
}

void remember_boot_shopkeepers()
{
	boot_shopkeepers.clear();
	for (P_char keeper = character_list; keeper; keeper = keeper->next)
		if (const int shop = singleton_shop_id(keeper); shop >= 0)
		{
			bind_shopkeeper(keeper, shop);
			boot_shopkeepers.insert(keeper);
		}
}

bool snapshot_shopkeepers_for_copyover()
{
	// Flat-file trades already commit their full stock and custody atomically.
	if (persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY)
		return true;
	if (!sql_save_dirty_shopkeepers(true))
		return false;
	std::unordered_set<int> saved;
	for (P_char keeper = character_list; keeper; keeper = keeper->next)
	{
		const int shop = singleton_shop_id(keeper);
		if (shop < 0)
			continue;
		if (keeper->in_room < 0 || keeper->in_room > top_of_world)
			return false;
		// Never silently choose between duplicate live inventories at handoff.
		if (!saved.insert(shop).second || !sql_save_shopkeeper(keeper, shop))
			return false;
		shop_index[shop].dirty = 0;
		shopkeeper_save_retry_reset(&shop_index[shop].dirty_save_retry);
	}
	return true;
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
		bind_shopkeeper(keeper, shop);
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
		shopkeeper_save_retry_reset(&shop_index[shop].dirty_save_retry);
		if (candidates.size() > 1)
			logit(LOG_STATUS, "world singleton: shop=%d retained=%ld removed=%zu", shop,
			      static_cast<long>(GET_IDNUM(keeper)), candidates.size() - 1);
	}
	boot_shopkeepers.clear();
}
