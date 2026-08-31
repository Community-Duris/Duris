#include "world_recovery_npc_items.h"

#include "enhance.h"
#include "prototypes.h"
#include "utils.h"

#include <new>
#include <unordered_map>
#include <unordered_set>
#include <vector>

extern P_index mob_index;
extern P_index obj_index;
extern struct room_data *world;
extern struct zone_data *zone_table;
extern int top_of_mobt;
extern int top_of_objt;
extern int top_of_world;
extern int top_of_zone_table;

namespace
{
using recovered_mob_list = std::vector<P_char>;

bool is_mob_load_command(char command)
{
	return command == 'M' || command == 'F' || command == 'R';
}

recovered_mob_list select_recovered_mobs(const reset_com &command, P_char const *mobs,
					 size_t mob_count, std::unordered_set<P_char> *assigned)
{
	recovered_mob_list selected;
	if (!assigned || command.arg1 < 0 || command.arg1 > top_of_mobt || command.arg3 < 0 ||
	    command.arg3 > top_of_world)
		return selected;
	const int birthplace = world[command.arg3].number;
	for (size_t index = 0; index < mob_count; ++index)
	{
		P_char mob = mobs[index];
		if (!mob || !IS_NPC(mob) || GET_RNUM(mob) != command.arg1 ||
		    GET_BIRTHPLACE(mob) != birthplace || assigned->find(mob) != assigned->end())
			continue;
		selected.push_back(mob);
		assigned->insert(mob);
	}
	return selected;
}

size_t carried_object_count(P_char mob, int object_rnum)
{
	size_t count = 0;
	for (P_obj object = mob ? mob->carrying : nullptr; object; object = object->next_content)
		if (object->R_num == object_rnum)
			++count;
	return count;
}

P_obj load_recovery_object(P_char mob, const reset_com &command, int artifact_respawn)
{
	if (!mob || command.arg1 < 0 || command.arg1 > top_of_objt || command.arg2 <= 0 ||
	    obj_index[command.arg1].number >= command.arg2)
		return nullptr;
	P_obj object = read_object(command.arg1, REAL);
	if (!object)
		return nullptr;
	arti_data artifact = {};
	if (IS_ARTIFACT(object) && get_artifact_data_sql(OBJ_VNUM(object), &artifact) &&
	    artifact.owned)
	{
		extract_obj(object);
		return nullptr;
	}
	if (IS_ARTIFACT(object) && artifact_respawn == 0)
	{
		extract_obj(object);
		return nullptr;
	}
	if (!ITEM_LOAD_CHECK(object, itemvalue(object), command.arg4) && !IS_SHOPKEEPER(mob))
	{
		enhance_on_npc_item_reset_skipped(mob, object);
		extract_obj(object);
		return nullptr;
	}
	return object;
}

size_t rehydrate_carried_item(const recovered_mob_list &mobs, const reset_com &command,
			      size_t desired_count, int artifact_respawn)
{
	if (command.arg1 < 0 || command.arg1 > top_of_objt)
		return 0;
	size_t loaded = 0;
	for (P_char mob : mobs)
	{
		if (carried_object_count(mob, command.arg1) >= desired_count)
			continue;
		P_obj object = load_recovery_object(mob, command, artifact_respawn);
		if (object)
		{
			obj_to_char(object, mob);
			++loaded;
		}
	}
	return loaded;
}

size_t rehydrate_equipped_item(const recovered_mob_list &mobs, const reset_com &command,
			       int artifact_respawn)
{
	if (command.arg1 < 0 || command.arg1 > top_of_objt || command.arg3 <= 0 ||
	    command.arg3 > CUR_MAX_WEAR)
		return 0;
	size_t loaded = 0;
	for (P_char mob : mobs)
	{
		if (mob->equipment[command.arg3])
			continue;
		P_obj object = load_recovery_object(mob, command, artifact_respawn);
		if (object)
		{
			equip_char(mob, object, command.arg3, 1);
			++loaded;
		}
	}
	return loaded;
}
} // namespace

bool world_recovery_rehydrate_npc_items(P_char const *mobs, size_t mob_count)
{
	if (!mobs && mob_count)
		return false;
	try
	{
		std::unordered_set<P_char> assigned;
		assigned.reserve(mob_count);
		const int artifact_respawn = get_property("artifact.respawn", 0);
		size_t matched_mobs = 0;
		size_t loaded_items = 0;
		for (int zone = 0; zone <= top_of_zone_table; ++zone)
		{
			recovered_mob_list selected;
			std::unordered_map<int, size_t> desired_carried;
			for (int command_index = 0;; ++command_index)
			{
				const reset_com &command = zone_table[zone].cmd[command_index];
				if (command.command == 'S')
					break;
				if (is_mob_load_command(command.command))
				{
					selected = select_recovered_mobs(command, mobs, mob_count,
									 &assigned);
					matched_mobs += selected.size();
					desired_carried.clear();
					continue;
				}
				if (selected.empty())
					continue;
				if (command.command == 'G')
				{
					const size_t desired = ++desired_carried[command.arg1];
					loaded_items += rehydrate_carried_item(
						selected, command, desired, artifact_respawn);
				}
				else if (command.command == 'E')
				{
					loaded_items += rehydrate_equipped_item(selected, command,
										artifact_respawn);
				}
			}
		}
		logit(LOG_STATUS,
		      "redis: rehydrated recovered NPC zone items matched_mobs=%zu loaded_items=%zu",
		      matched_mobs, loaded_items);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		logit(LOG_SYS, "redis: recovered NPC item rehydration allocation failed mobs=%zu",
		      mob_count);
		return false;
	}
}
