#include "net/comm.h"
#include "core/prototypes.h"
#include "ships/ships.h"
#include "combat/chaos_materials.h"
#include "item/enhance.h"
#include "item/item_movement_transaction.h"
#include "core/utils.h"

#include <stdlib.h>

extern long new_exp_table[];

static struct
{
	int vnum;
	const char *name;
} portdata[] = { { 559633, "flann" },
		 { 132573, "tharnadia tharn" },
		 { 82500, "myra myrabolus" },
		 { 16551, "woodseer ws" },
		 { 93610, "vella" },
		 { 45000, "charing grey" },
		 { 5302, "marigot centaur" },
		 { 37712, "nax" },
		 { 635260, "dalvik kk" },
		 { 97628, "shady" },
		 { 22410, "sp storm storm_port stormport" },
		 { 9401, "sarmiz" },
		 { 1711, "qq" },
		 { 11703, "ghore" },
		 { 70296, "moregeeth gob gobbo" },
		 { 43142, "tg tq" },
		 { 15263, "faang" },
		 { 99715, "lava lavasprings durka" },
		 { 584171, "frzzt" },
		 { 19428, "githyanki" },
		 { 96537, "ix ixarkon" },
		 { 17021, "khild khildarak" },
		 { 36544, "arach arachdrathos drow" },
		 {} };

static void chaos_port(P_char ch, const char *arg)
{
	if (!*arg)
		return send_to_char("Port to where?\n", ch);
	for (int i = 0; portdata[i].vnum; i++)
		if (isname(arg, portdata[i].name))
		{
			act("$n creates and enters a chaos portal, which then dissipates.", 0, ch,
			    0, 0, TO_ROOM);
			char_from_room(ch);
			act("You create a step through a chaos portal.", 0, ch, 0, 0, TO_CHAR);
			char_to_room(ch, real_room(portdata[i].vnum), -1);
			act("A chaos portal briefly appears to spew out $n.", 0, ch, 0, 0, TO_ROOM);
			return;
		}

	send_to_char("No portal leads there.\n", ch);
}

static struct
{
	int id;
	const char *name;
} sidenames[] = { // some plurals are weird and ungrammatic, that's ok -- we use abbrevs
	{ RACEWAR_GOOD, "goodies" },	  { RACEWAR_GOOD, "goods" },
	{ RACEWAR_EVIL, "evils" },	  { RACEWAR_UNDEAD, "undeads" },
	{ RACEWAR_NEUTRAL, "illithids" }, { RACEWAR_NEUTRAL, "squids" },
	{ RACEWAR_NEUTRAL, "seafood" }, // :p
	{ RACEWAR_NEUTRAL, "neutrals" },  {}
};

static void chaos_side(P_char ch, const char *arg)
{
	if (!*arg)
		return send_to_char(
			"Which side?  There are &+Gg&noodies, &+Re&nvils, &+Lu&nndead, and &+Mi&nllithids.\n",
			ch);

	for (int i = 0; sidenames[i].id; i++)
		if (is_abbrev(arg, sidenames[i].name))
		{
			if (ch->player.racewar == sidenames[i].id)
				return send_to_char("You reaffirm your allegiance.\n", ch);
			ch->player.racewar = sidenames[i].id;
			act("Your allegiance changes.", 0, ch, 0, 0, TO_CHAR);
			// boot from guilds?
			return;
		}

	send_to_char("There's no such side.\n", ch);
}

static void chaos_pouch_test_seed(P_char ch)
{
	static constexpr int vnums[] = { 400000, 400001, 400291, 18000, 22801 };
	bool queued = false;
	for (int vnum : vnums)
	{
		P_obj object = read_object(vnum, VIRTUAL);
		if (!object)
			continue;
		if (item_creation_grant_submit_to_player(ch, object, ch))
			queued = true;
		else
			extract_obj(object, FALSE);
	}
	if (queued)
		item_creation_grant_mark_blocking(ch);
	if (!enhancement_system_is_ready())
		boot_enhancement_system();
	send_to_char("Chaos pouch test materials prepared.\r\n", ch);
}

static void chaos_pouch_test_generate(P_char ch, const char *arg)
{
	const int count = atoi(arg);
	if (count <= 0 || count > 1000)
	{
		send_to_char("Chaos pouch test generation count must be 1..1000.\r\n", ch);
		return;
	}
	const chaos_material_pouch_usage usage = { 400000, static_cast<uint64_t>(count) };
	if (!chaos_material_pouch_record_generated(ch, &usage, 1))
		send_to_char("Chaos pouch test generation was not recorded.\r\n", ch);
}

void do_chaos(P_char ch, char *arg, int /*cmd*/)
{
	if (!IS_PC(ch))
		return;

	if (!IS_TRUSTED(ch) && !chaos_test_commands_enabled())
		return send_to_char("No, you can't have pony.  Not yours.\n", ch);

	char buff[MAX_STRING_LENGTH];

	arg = one_argument(arg, buff);
	while (*arg == ' ')
		arg++;

	if (!*buff)
		goto noarg;

	if (chaos_test_commands_enabled())
	{
		if (is_abbrev(buff, "pouchseed"))
			return chaos_pouch_test_seed(ch);
		if (is_abbrev(buff, "pouchgenerate"))
			return chaos_pouch_test_generate(ch, arg);
	}

	if (is_abbrev(buff, "platinum") || is_abbrev(buff, "plats"))
	{
		ADD_MONEY(ch, 10000000);
		send_to_char("Here's &+W10k plat&n, enjoy!\n", ch);
		return;
	}

	if (is_abbrev(buff, "level"))
	{
		if (IS_TRUSTED(ch))
			return send_to_char("So you want to become a smelly mortal? Nah.\n", ch);

		long oldl = GET_LEVEL(ch);
		long newl = strtol(arg, NULL, 10);
		if (newl < 1 || newl > 56)
			return send_to_char("1..56, please.\n", ch);
		if (oldl == newl)
			return send_to_char("Refusing to do a no-op operation!\n", ch);

		GET_EXP(ch) = new_exp_table[GET_LEVEL(ch) + 1] / 2;
		if (oldl < newl)
			advance_to_level(ch, newl);
		for (; oldl > newl; oldl--)
			lose_level(ch);

		return;
	}

	if (is_abbrev(buff, "shipfrags") || is_abbrev(buff, "sfrags"))
	{
		P_ship ship = get_ship_from_owner(GET_NAME(ch));
		if (!ship)
			return send_to_char("Ye don't own a ship, landlubber!\n", ch);

		char *err;
		unsigned long newf = strtoul(arg, &err, 10);
		if (*err || !*arg)
			return send_to_char("Gimme a numbah, pleez.\n", ch);
		if (newf > 10000)
			return send_to_char("Be real.\n", ch);
		ship->frags = newf;
		return send_to_char("Ok.\n", ch);
	}

	if (is_abbrev(buff, "crewexperience"))
	{
		P_ship ship = get_ship_from_owner(GET_NAME(ch));
		if (!ship)
			return send_to_char("Ye don't own a ship, landlubber!\n", ch);

		int max = 10000;
		int s, g, r;
		s = -1;
		if (sscanf(arg, "%d %d %d", &s, &g, &r) != 3)
			g = r = s;
		if (s < 0 || g < 0 || r < 0)
			return send_to_char("One or three numbers, for sail/gun/repair exp.\n", ch);
		if (s > max || g > max || r > max)
			return send_to_char(
				"Nobody that experienced would work for a wuss like you.\n", ch);

		ship->crew.sail_skill = s;
		ship->crew.guns_skill = g;
		ship->crew.rpar_skill = r;
		return send_to_char("Ok.\n", ch);
	}

	if (is_abbrev(buff, "portal"))
		return chaos_port(ch, arg);

	if (is_abbrev(buff, "side"))
		return chaos_side(ch, arg);

noarg:
	send_to_char(
		"Nuh uh. Can give only &+Wplat&n, &+Wlevel&n, &+Wshipfrags&n, &+Wcrewexp&n, &+Wportal&n, &+Wside&n.\n",
		ch);
}
