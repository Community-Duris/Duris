#include "core/prototypes.h"
#include "core/structs.h"
#include "core/defines.h"
#include "core/utility.h"
#include "core/utils.h"
#include "net/comm.h"
#include "world/db.h"

extern P_room world;
extern struct zone_data *zone_table;
void do_zreset(P_char ch, char *argument, int /*cmd*/)
{
	char arg[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH];
	struct zone_data *zone_struct;
	int zone_number = 0;

	if (!ch)
	{
		logit(LOG_DEBUG, "Screw-up in do_zreset(), NULL ch.");
		return;
	}
	if (IS_NPC(ch))
	{
		return;
	}

	zone_number = world[ch->in_room].zone;
	zone_struct = &zone_table[zone_number];

	one_argument(argument, arg);

	if (!*arg)
	{
		send_to_char("Syntax: zreset ok or full(purge and reset)\n", ch);
	}
	else if (!str_cmp(arg, "ok"))
	{
		snprintf(buf, MAX_STRING_LENGTH, "Zone: %s has been reset.\n", zone_struct->name);
		send_to_char(buf, ch);
		reset_zone(zone_number, 0);
		// If char resetting zone is a mortal.. hrm..
		wizlog((GET_LEVEL(ch) > MAXLVLMORTAL) ? GET_LEVEL(ch) : MINLVLIMMORTAL,
		       "%s just reset the zone %s.", GET_NAME(ch), zone_struct->name);
		logit(LOG_WIZ, "%s just reset the zone %s.", GET_NAME(ch), zone_struct->name);
		return;
	}
	else if (!str_cmp(arg, "full"))
	{
		snprintf(buf, MAX_STRING_LENGTH, "Zone: %s has been reset.\n", zone_struct->name);
		send_to_char(buf, ch);
		zone_purge(zone_number);
		reset_zone(zone_number, 1);
		wizlog((GET_LEVEL(ch) > MAXLVLMORTAL) ? GET_LEVEL(ch) : MINLVLIMMORTAL,
		       "%s just reset the zone %s.", GET_NAME(ch), zone_struct->name);
		logit(LOG_WIZ, "%s just reset the zone %s.", GET_NAME(ch), zone_struct->name);
		return;
	}
	// We get here if someone enters a bad command.
	send_to_char("Syntax: zreset ok or full(purge and reset)\n", ch);
	return;
}
