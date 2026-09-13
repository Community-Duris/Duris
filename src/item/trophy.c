/* Experience trophy observation: game-thread state, checkpoint persistence. */
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "item/trophy.h"
#include "net/comm.h"
#include "persistence/persistence_checkpoint.h"
#include "sql/sql.h"

#include <algorithm>
#include <climits>
#include <memory>
#include <new>
#include <vector>

extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern P_room world;
extern const int top_of_world;

bool record_zone_trophy_award(P_char ch, P_char victim, int credited_xp, int type)
{
	if (!ch || !IS_PC(ch) || !ch->only.pc || credited_xp <= 0 || GET_LEVEL(ch) < 25 ||
	    GET_LEVEL(ch) > MAXLVLMORTAL || IS_ILLITHID(ch) ||
	    get_property("exp.zoneTrophy.observe", 0) != 1)
		return false;

	if (type != EXP_DAMAGE && type != EXP_KILL && type != EXP_MELEE && type != EXP_HEALING &&
	    type != EXP_TANKING)
		return false;

	// Healing's victim is the patient; its opponent identifies PvE versus PvP.
	P_char opponent = type == EXP_HEALING && victim ? GET_OPPONENT(victim) : victim;
	if (!opponent || !IS_NPC(opponent) || IS_PC_PET(opponent))
		return false;

	if (!world || !zone_table || ch->in_room < 0 || ch->in_room > top_of_world)
		return false;
	const int zone_index = world[ch->in_room].zone;
	if (zone_index < 0 || zone_index > top_of_zone_table)
		return false;
	const int zone_number = zone_table[zone_index].number;
	if (zone_number <= 0)
		return false;

	// Observation covers loaded zones on both backends. Enforcement eligibility
	// (zones.trophy_zone) is deliberately not consulted or queried here.
	try
	{
		if (!ZONE_TROPHY(ch))
		{
			auto entries = std::make_unique<std::vector<zone_trophy_data>>();
			entries->push_back({ zone_number, credited_xp });
			ZONE_TROPHY(ch) = entries.release();
			return true;
		}
		for (zone_trophy_data &entry : *ZONE_TROPHY(ch))
		{
			if (entry.zone_number != zone_number)
				continue;
			const int previous = std::max(0, entry.exp);
			const int next = previous + std::min(credited_xp, INT_MAX - previous);
			if (entry.exp == next)
				return false;
			entry.exp = next;
			return true;
		}
		if (ZONE_TROPHY(ch)->size() >= ZONE_TROPHY_MAX_ZONES)
			return false;
		ZONE_TROPHY(ch)->push_back({ zone_number, credited_xp });
		return true;
	}
	catch (const std::bad_alloc &)
	{
		// Optional observation cannot interrupt an already accepted XP award.
		return false;
	}
}

void clear_zone_trophy(P_char ch)
{
	if (!ch || !IS_PC(ch) || !ch->only.pc)
		return;
	if (ZONE_TROPHY(ch))
		ZONE_TROPHY(ch)->clear();
	// Include an empty collection so old durable rows are deleted as well.
	mark_player_dirty_components(GET_PID(ch),
				     PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_TROPHIES);
}

void do_trophy(P_char ch, char *arg, int /*cmd*/)
{
	P_char rch = GET_PLYR(ch);
	if (!rch || !IS_PC(rch))
	{
		send_to_char("Mobs don't have trophy.\r\n", ch);
		return;
	}

	char target[MAX_STRING_LENGTH], option[MAX_STRING_LENGTH];
	argument_interpreter(arg, target, option);
	P_char who = rch;
	const bool own_frags = isname("frags", target);
	if (IS_TRUSTED(rch) && *target && !own_frags)
	{
		who = get_char_vis(rch, target);
		if (!who)
		{
			send_to_char("They don't appear to be in the game.\r\n", ch);
			return;
		}
	}
	if (!IS_PC(who))
	{
		send_to_char("Mobs don't have trophy.\r\n", ch);
		return;
	}
	if (own_frags || (IS_TRUSTED(rch) && isname("frags", option)))
	{
		show_frag_trophy(ch, who);
		return;
	}

	send_to_char("&+WExperience trophy&n (tracking only; no XP penalty)\r\n", ch);
	if (get_property("exp.zoneTrophy.observe", 0) != 1)
		send_to_char("Tracking is off. Showing saved totals.\r\n", ch);
	if (!ZONE_TROPHY(who) || ZONE_TROPHY(who)->empty())
	{
		send_to_char("No zone experience recorded.\r\n", ch);
		return;
	}
	for (const zone_trophy_data &entry : *ZONE_TROPHY(who))
	{
		if (entry.exp <= 0)
			continue;
		const char *name = "Unknown zone";
		if (zone_table)
			for (int index = 0; index <= top_of_zone_table; ++index)
				if (zone_table[index].number == entry.zone_number)
				{
					if (zone_table[index].name)
						name = zone_table[index].name;
					break;
				}
		char line[MAX_STRING_LENGTH];
		checked_snprintf(line, sizeof(line), " %s &n[%d]: %d XP\r\n", name,
				 entry.zone_number, entry.exp);
		send_to_char(line, ch);
	}
}
