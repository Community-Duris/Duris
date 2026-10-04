/* Staff character and ship attribute editing. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/defines.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/safe_format.h"
#include "net/comm.h"
#include "ships/ships.h"
#include "sql/sql.h"
#include "telemetry/telemetry_runtime.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static void sa_byteCopy(P_char ch, unsigned long offset, int value)
{
	::byte new_value = (::byte)value;

	bcopy((char *)&new_value, (char *)ch + offset, sizeof(::byte));
}

/*
 ** Used by do_setattr to perform short int copying
 */
static void sa_shortCopy(P_char ch, unsigned long offset, int value)
{
	sh_int new_value = (sh_int)value;

	bcopy((char *)&new_value, (char *)ch + offset, sizeof(sh_int));
}

/*
 ** Used by do_setattr to do integer copying
 */
static void sa_intCopy(P_char ch, unsigned long offset, int value)
{
	bcopy((char *)&value, (char *)ch + offset, sizeof(int));
	if (offset == offsetof(char_data, specials.affected_by))
		telemetry_runtime_game_control_changed(ch);
}

/*
 ** Used by do_setattr to do set the year of player to "value"
 */
static void sa_ageCopy(P_char ch, unsigned long /*offset*/, int value)
{
	long secs;
	time_t curr_time = time(NULL);

	secs = (value - 17) * SECS_PER_MUD_YEAR;
	ch->player.time.birth = curr_time - secs;
}

static void do_setship(P_char ch, char *arg)
{
	char name[MAX_STRING_LENGTH];
	char Gbuf1[MAX_STRING_LENGTH];
	P_ship ship;

	arg = one_argument(arg, name);
	if ((ship = get_ship_from_owner(name)) != NULL)
	{
		// Debugging:
		//    snprintf(Gbuf1, MAX_STRING_LENGTH, "Found ship '%s', Owner: '%s'.\n\r", ship->name, ship->ownername );
		//    send_to_char( Gbuf1, ch );

		arg = one_argument(arg, name);
		if (isname(name, "frags"))
		{
			arg = one_argument(arg, name);
			if (!*name || atoi(name) < 0)
			{
				send_to_char(
					"You must enter a postive amount to set ship frags to.\n\r",
					ch);
				return;
			}
			snprintf(Gbuf1, MAX_STRING_LENGTH,
				 "Name: '%s&n' Owner: '%s&n'\n\rOld Ship Frags: %d, ", ship->name,
				 ship->ownername, ship->frags);
			send_to_char(Gbuf1, ch);
			ship->frags = atoi(name);
			snprintf(Gbuf1, MAX_STRING_LENGTH, "New Ship frags: %d.\n\r", ship->frags);
			send_to_char(Gbuf1, ch);
		}
		else if (isname(name, "guns"))
		{
			arg = one_argument(arg, name);
			if (!*name)
			{
				send_to_char(
					"You must enter an amount to raise the guns crew exp.\n\r",
					ch);
				return;
			}
			ship->guns_skill_raise(ch, atof(name));
		}
		else if (isname(name, "repair"))
		{
			arg = one_argument(arg, name);
			if (!*name)
			{
				send_to_char(
					"You must enter an amount to raise the repair crew exp.\n\r",
					ch);
				return;
			}
			ship->rpar_skill_raise(ch, atof(name));
		}
		else if (isname(name, "sail"))
		{
			arg = one_argument(arg, name);
			if (!*name)
			{
				send_to_char(
					"You must enter an amount to raise the sail crew exp.\n\r",
					ch);
				return;
			}
			ship->sail_skill_raise(ch, atof(name));
		}
		else
		{
			send_to_char(
				"Valid Ship settings: 'frags', 'guns', 'repair', or 'sail'.\n\r",
				ch);
		}
	}
	else
	{
		checked_snprintf(Gbuf1, MAX_STRING_LENGTH, "Couldn't find ship '%s'.\n\r", name);
		send_to_char(Gbuf1, ch);
	}
}

/*
 * ** This command gives a wizard the power to modify a player's **
 * attributes.  See below for a list of modifiable player's ** attributes.
 * ** ** Usage: **     setattr player attribute value **
 */

void do_setattr(P_char ch, char *arg, int /*cmd*/)
{
	/* Internal Macros */

#define OFFSET(Field) OFFSET_OF(P_char, Field)

	/* Note that following macro will only work on non-Cray   */
	/* architectures. */

#define OFFSET_OF(Type, Field) ((size_t)(((char *)(&(((Type)NULL)->Field))) - ((char *)NULL)))

	/* Declaration */

	char name[MAX_STRING_LENGTH];
	char attribute[MAX_STRING_LENGTH];
	char value_str[MAX_STRING_LENGTH];
	int value, i;
	P_char victim;

	/*
	 * ** The following structure is used to make adding new fields **
	 * that are modifiable by this wiz command as easy as possible. ** **
	 * Note that "str" is a special case and is handled ** separately
	 * below.
	 */

	static struct
	{
		const char *attr_name; /* Name of attribute  */
		unsigned long attr_offset; /* Offset from beginning */
		/* Function used to copy  */
		void (*attr_func)(P_char, unsigned long, int);

	} attr_tuples[] = {
		{ "age", OFFSET(player.time.birth), sa_ageCopy },
		{ "sex", OFFSET(player.sex), sa_byteCopy },
		{ "str", OFFSET(base_stats.Str), sa_byteCopy },
		{ "dex", OFFSET(base_stats.Dex), sa_byteCopy },
		{ "agi", OFFSET(base_stats.Agi), sa_byteCopy },
		{ "con", OFFSET(base_stats.Con), sa_byteCopy },
		{ "pow", OFFSET(base_stats.Pow), sa_byteCopy },
		{ "int", OFFSET(base_stats.Int), sa_byteCopy },
		{ "wis", OFFSET(base_stats.Wis), sa_byteCopy },
		{ "cha", OFFSET(base_stats.Cha), sa_byteCopy },
		{ "luck", OFFSET(base_stats.Luk), sa_byteCopy },
		{ "Karma", OFFSET(base_stats.Kar), sa_byteCopy },
		{ "hit", OFFSET(points.max_hit), sa_shortCopy },
		{ "hitroll", OFFSET(points.hitroll), sa_byteCopy },
		{ "damroll", OFFSET(points.damroll), sa_byteCopy },
		{ "drunk", OFFSET(specials.conditions[0]), sa_byteCopy },
		{ "hunger", OFFSET(specials.conditions[1]), sa_byteCopy },
		{ "thirst", OFFSET(specials.conditions[2]), sa_byteCopy },
		{ "alignment", OFFSET(specials.alignment), sa_intCopy },
		{ "affected", OFFSET(specials.affected_by), sa_intCopy },
		{ "act", OFFSET(specials.act), sa_intCopy },
		{ "npcflag", OFFSET(specials.act), sa_intCopy },
		{ "pcflag", OFFSET(specials.affected_by), sa_intCopy },
	};

#undef OFFSET
#undef OFFSET_OF

	/* Executable section */

	if (IS_NPC(ch))
	{
		return;
	}
	wizlog(GET_LEVEL(ch), "%s: setattr: %s", GET_NAME(ch), arg);
	logit(LOG_WIZ, "%s: setattr: %s", GET_NAME(ch), arg);
	sql_log(ch, WIZLOG, "setattr: %s", arg);

	/* Get name of player  */

	arg = one_argument(arg, name);

	if (!*name)
	{
		send_to_char("Current settable attributes are:\n\n", ch);

		for (i = 0; i < static_cast<int>(ARRAY_SIZE(attr_tuples)); i++)
		{
			send_to_char(attr_tuples[i].attr_name, ch);
			send_to_char("\n", ch);
		}
		return;
	}

	if (isname(name, "ship"))
	{
		do_setship(ch, arg);
		return;
	}

	if (!(victim = get_char_vis(ch, name)))
	{
		send_to_char("No one by that name here...\n", ch);
		return;
	}
	/* Get name of attribute */

	arg = one_argument(arg, attribute);

	if (!*attribute)
	{
		send_to_char("Current settable attributes are:\n\n", ch);

		for (i = 0; i < static_cast<int>(ARRAY_SIZE(attr_tuples)); i++)
		{
			send_to_char(attr_tuples[i].attr_name, ch);
			send_to_char("\n", ch);
		}
		send_to_char("ship\n", ch);
		return;
	}

	/* Get value */

	arg = one_argument(arg, value_str);

	if (!*value_str)
	{
		send_to_char("Current settable attributes are:\n\n", ch);

		for (i = 0; i < static_cast<int>(ARRAY_SIZE(attr_tuples)); i++)
		{
			send_to_char(attr_tuples[i].attr_name, ch);
			send_to_char("\n", ch);
		}
		send_to_char("ship\n", ch);
		return;
	}

	value = atoi(value_str);

	/* Handle the special cases ...  */

	/* Find out which attributes to set  */

	for (i = 0; i < static_cast<int>(ARRAY_SIZE(attr_tuples)); i++)
	{
		if (!str_cmp(attr_tuples[i].attr_name, attribute))
			break;
	}

	if (i == static_cast<int>(ARRAY_SIZE(attr_tuples)))
	{
		send_to_char("Current settable attributes are:\n\n", ch);

		for (i = 0; i < static_cast<int>(ARRAY_SIZE(attr_tuples)); i++)
		{
			send_to_char(attr_tuples[i].attr_name, ch);
			send_to_char("\n", ch);
		}
		send_to_char("ship\n", ch);
		return;
	}
	/* Now set attributes  */

	(*(attr_tuples[i].attr_func))(victim, attr_tuples[i].attr_offset, value);
	send_to_char("OK.\n", ch);
	balance_affects(victim);

	return;
}
