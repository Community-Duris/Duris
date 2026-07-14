/****************************************************************************
 *  File: enhance.c                                           Part of Duris
 *  Usage: Item enhancement system
 *  Extracted from drannak.c 2026-07-14
 * ***************************************************************************
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "mm.h"
#include "new_combat_def.h"
#include "prototypes.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include "enhance.h"
#include "tradeskill.h"
#include "objmisc.h"

/* Forward declarations for hash functions used in enhance() and do_enhance() */
static int enhance_hash(int key);
static int enhance_stat_hash(int key);

void enhance(P_char ch, P_obj source, P_obj material)
{
	char  buf[MAX_STRING_LENGTH];
	P_obj robj;
	int   cost, searchcount, maxsearch, tries, sval, level;
	bool  validobj;
	int   newval, minval, chluck, wearflags;
	int   cascade_dir, cascade_step, cascade_ival;
	struct enhance_index_entry *entry;

	if (!ch || !source || !material)
		return;

	chluck      = (GET_C_LUK(ch));
	sval        = itemvalue(source);
	minval      = itemvalue(source) - enhance_material_ival_delta;
	searchcount = 0;
	maxsearch   = enhance_search_max_attempts;
	// Only search matching wear flags unless none matching, then just search source wear flags.
	//  We skip ITEM_TAKE 'cause it's not really a wear flag.  We skip ITEM_HOLD, ITEM_ATTACH_BELT, and
	//  ITEM_WEAR_BACK because these are too common and override what people really want (i.e. a quiver).
	wearflags = (source->wear_flags & material->wear_flags) & ~(ITEM_TAKE | ITEM_HOLD | ITEM_ATTACH_BELT | ITEM_WEAR_BACK);
	if (!wearflags)
		wearflags = (source->wear_flags) & ~(ITEM_TAKE | ITEM_HOLD | ITEM_ATTACH_BELT | ITEM_WEAR_BACK);

	if (!wearflags)
	{
		send_to_char("This item can not be enhanced.\n", ch);
		return;
	}

	// Can enhance up to 3x level, same as forge/craft. --Eikel
	if (sval > GET_LEVEL(ch) * 3)
	{
		send_to_char("This item is too powerful to be enhanced further.\n", ch);
		return;
	}

	if (IS_SET(source->wear_flags, ITEM_GUILD_INSIGNIA))
		minval += enhance_guild_insignia_ival_bonus;

	if (itemvalue(material) < minval)
	{
		char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
		snprintf(buf2, MAX_STRING_LENGTH, "%s", source->short_description);
		snprintf(buf, MAX_STRING_LENGTH, "&+REnhancing %s requires an item with at least an &+Witem value of: %d&n\r\n", buf2, minval);
		send_to_char(buf, ch);
		return;
	}

	if (sval <= enhance_cost_low_ival_threshold)
	{
		cost = enhance_cost_low_amount;
	}
	else
	{
		cost = enhance_cost_high_amount;
	}

	if (GET_MONEY(ch) < cost)
	{
		snprintf(buf, MAX_STRING_LENGTH, "It will require &+W%d platinum&n to &+Benhance&n this item.\r\n", cost / 1000);
		send_to_char(buf, ch);
		return;
	}

	if (number(1, enhance_luck_extreme_range) < chluck)
	{
		newval = sval + enhance_ival_gain_extreme;
		maxsearch *= 4;
		send_to_char("&+YYou feel &+MEXTREMELY Lucky&+Y!\r\n", ch);
	}
	else if (number(1, enhance_luck_very_range) < chluck)
	{
		newval = sval + enhance_ival_gain_very;
		maxsearch *= 3;
		send_to_char("&+YYou feel &+MVery Lucky&+Y!\r\n", ch);
	}
	else if (number(1, enhance_luck_lucky_range) < chluck)
	{
		newval = sval + enhance_ival_gain_lucky;
		maxsearch *= 2;
		send_to_char("&+YYou feel &+MLucky&+Y!\r\n", ch);
	}
	else
	{
		newval = sval + enhance_ival_gain_normal;
	}

	/* Cascade search through the ival hash table.
	 * Try exact match first, then cascade in the configured direction.
	 * cascade_down_first=1: try lower ival values first, then higher
	 * cascade_down_first=0: try higher ival values first, then lower
	 */
	robj = NULL;
	for (cascade_step = 0; cascade_step <= enhance_original_max_roll; cascade_step++)
	{
		for (cascade_dir = 0; cascade_dir < 2; cascade_dir++)
		{
			if (cascade_step == 0)
			{
				/* Exact match — only one try */
				if (cascade_dir > 0)
					continue;
				cascade_ival = newval;
			}
			else if (enhance_original_cascade_down_first)
			{
				/* Down first: try -step, then +step */
				cascade_ival = (cascade_dir == 0) ? (newval - cascade_step) : (newval + cascade_step);
			}
			else
			{
				/* Up first: try +step, then -step */
				cascade_ival = (cascade_dir == 0) ? (newval + cascade_step) : (newval - cascade_step);
			}

			if (cascade_ival < 1 || cascade_ival > enhance_ival_cap + enhance_original_max_roll)
				continue;

			/* Look up in hash table */
			for (entry = enhance_ival_table[enhance_hash(cascade_ival)]; entry; entry = entry->next)
			{
				if (entry->ival != cascade_ival)
					continue;

				/* Check wear flags match */
				if (!(wearflags & entry->wear_flags))
					continue;

				/* Check not same vnum */
				if (entry->vnum == OBJ_VNUM(source))
					continue;

				/* Found a match — read the object */
				robj = read_object(entry->vnum, VIRTUAL);
				if (robj)
				{
					validobj = TRUE;
					break;
				}
			}

			if (robj)
				break;
		}
		if (robj)
			break;
		searchcount++;
		if (searchcount > maxsearch)
			break;
	}

	if (!robj)
	{
		act("&+GThe &+ritem gods&+G could not find a better type of &+yitem &+Gthan your &n$p&+G this time. &+WTry again&+G. If your item's value is above &+W55&+G you may have the &+Wbest&+G "
		    "item of that type!\r\n",
		    FALSE,
		    ch,
		    source,
		    0,
		    TO_CHAR);
		return;
	}

	// Remove Curse, Secret, add Invis
	if (IS_SET(robj->extra_flags, ITEM_SECRET))
	{
		REMOVE_BIT(robj->extra_flags, ITEM_SECRET);
	}
	if (IS_SET(robj->extra_flags, ITEM_NODROP))
	{
		REMOVE_BIT(robj->extra_flags, ITEM_NODROP);
	}

	if (IS_SET(robj->extra_flags, ITEM_INVISIBLE))
	{
		REMOVE_BIT(robj->extra_flags, ITEM_INVISIBLE);
	}
	SET_BIT(robj->extra_flags, ITEM_NOREPAIR);
	SUB_MONEY(ch, cost, 0);
	send_to_char("Your pockets feel &+Wlighter&n.\r\n", ch);

	act("&+BYour enhancement is a success! You now have &n$p&+B!\r\n", FALSE, ch, robj, 0, TO_CHAR);
	obj_to_char(robj, ch);
	obj_from_char(source);
	extract_obj(source);
	obj_from_char(material);
	extract_obj(material);
	statuslog(ch->player.level,
	          "&+BEnhancement&n:&n %s&n just got [%d] '%s&n' ival [%d] at [%d]!",
	          GET_NAME(ch),
	          obj_index[robj->R_num].virtual_number,
	          robj->short_description,
	          itemvalue(robj),
	          (ch->in_room == NOWHERE) ? -1 : world[ch->in_room].number);
	return;
}

/* Stat name → APPLY location mapping for stat-enhance */
static const struct {
	const char *name;
	int         apply_loc;
	const char *color;
	const char *display_name;
} enhance_stat_names[] = {
	{"str",       APPLY_STR,     "&+r", "strength"},
	{"strength",  APPLY_STR,     "&+r", "strength"},
	{"dex",       APPLY_DEX,     "&+g", "dexterity"},
	{"dexterity", APPLY_DEX,     "&+g", "dexterity"},
	{"int",       APPLY_INT,     "&+M", "intelligence"},
	{"intelligence", APPLY_INT,  "&+M", "intelligence"},
	{"wis",       APPLY_WIS,     "&+c", "wisdom"},
	{"wisdom",    APPLY_WIS,     "&+c", "wisdom"},
	{"con",       APPLY_CON,     "&+c", "constitution"},
	{"constitution", APPLY_CON,  "&+c", "constitution"},
	{"agi",       APPLY_AGI,     "&+B", "agility"},
	{"agility",   APPLY_AGI,     "&+B", "agility"},
	{"pow",       APPLY_POW,     "&+L", "power"},
	{"power",     APPLY_POW,     "&+L", "power"},
	{"cha",       APPLY_CHA,     "&+C", "charisma"},
	{"charisma",  APPLY_CHA,     "&+C", "charisma"},
	{"hit",       APPLY_HIT,     "&+R", "health"},
	{"health",    APPLY_HIT,     "&+R", "health"},
	{"ac",        APPLY_AC,      "&+W", "armor class"},
	{"armor",     APPLY_AC,      "&+W", "armor class"},
	{"hitroll",   APPLY_HITROLL, "&+y", "precision"},
	{"damroll",   APPLY_DAMROLL, "&+y", "damage"},
	{"str_max",   APPLY_STR_MAX, "&+r", "greater strength"},
	{"dex_max",   APPLY_DEX_MAX, "&+g", "greater dexterity"},
	{"int_max",   APPLY_INT_MAX, "&+M", "greater intelligence"},
	{"wis_max",   APPLY_WIS_MAX, "&+c", "greater wisdom"},
	{"con_max",   APPLY_CON_MAX, "&+c", "greater constitution"},
	{"agi_max",   APPLY_AGI_MAX, "&+B", "greater agility"},
	{"pow_max",   APPLY_POW_MAX, "&+L", "greater power"},
	{"cha_max",   APPLY_CHA_MAX, "&+C", "greater charisma"},
	{NULL,        0,             NULL,  NULL}
};

void do_enhance(P_char ch, char *argument, int cmd)
{
	P_obj source, material, donor;
	char  first[MAX_INPUT_LENGTH];
	char  second[MAX_INPUT_LENGTH];
	char  third[MAX_INPUT_LENGTH];
	char  rest[MAX_INPUT_LENGTH];
	int   i, apply_loc, orig_mod, new_mod, cap;
	bool  found_stat;
	struct enhance_index_entry *entry;

	if (IS_NPC(ch))
		return;

	if (!argument || !*argument)
	{
		send_to_char("&+yWhich &+Witem &+ywould you like to &+men&+Mhan&+mce&+y? &n\r\n"
		             "Syntax: enhance <source item> <material item>\r\n"
		             "        enhance <source item> <stat> <donor item>\r\n", ch);
		return;
	}

	half_chop(argument, first, rest);
	half_chop(rest, second, third);
	half_chop(third, third, rest);

	/* Check for stat-enhance syntax: enhance <source> <stat> <donor> (3 args) */
	if (*third)
	{
		/* Look up the stat name */
		found_stat = FALSE;
		apply_loc = 0;
		if (!(source = get_obj_in_list_vis(ch, first, ch->carrying)))
		{
			act("&+yWhich &+Witem &+ywould you like to &+men&+Mhan&+mce&+y?", FALSE, ch, 0, 0, TO_CHAR);
			return;
		}
		if (!is_salvageable(source))
		{
			act("&+yYour $p&+y cannot be used in this way... try something else&n.", FALSE, ch, source, 0, TO_CHAR);
			return;
		}

		for (i = 0; enhance_stat_names[i].name; i++)
		{
			if (!strcasecmp(second, enhance_stat_names[i].name))
			{
				apply_loc = enhance_stat_names[i].apply_loc;
				found_stat = TRUE;
				break;
			}
		}

		if (!found_stat)
		{
			send_to_char("&+yUnknown stat.  Try: str, dex, int, wis, con, agi, pow, cha, hit, ac, hitroll, damroll, or the _max variants.\r\n", ch);
			return;
		}

		if (!(donor = get_obj_in_list_vis(ch, third, ch->carrying)))
		{
			act("&+yWhich &+Wdonor item &+ywould you like to draw the &+mstat&+y from?", FALSE, ch, 0, 0, TO_CHAR);
			return;
		}
		if (!is_salvageable(donor))
		{
			act("&+yYour $p&+y cannot be used as a donor&n.", FALSE, ch, donor, 0, TO_CHAR);
			return;
		}
		if (OBJ_VNUM(donor) == OBJ_VNUM(source))
		{
			send_to_char("&+yYou cannot use the same item as both source and donor!\r\n", ch);
			return;
		}

		/* Look up donor in stat hash table */
		entry = enhance_stat_table[enhance_stat_hash(apply_loc)];
		while (entry)
		{
			if (entry->vnum == OBJ_VNUM(donor))
				break;
			entry = entry->next;
		}

		if (!entry)
		{
			send_to_char("&+yThat donor item is not a valid template for this stat.\r\n", ch);
			return;
		}

		/* Find the stat on the donor */
		orig_mod = 0;
		for (i = 0; i < MAX_OBJ_AFFECT; i++)
		{
			if (entry->apply_loc[i] == apply_loc)
			{
				orig_mod = entry->apply_mod[i];
				break;
			}
		}

		if (orig_mod <= 0)
		{
			send_to_char("&+yThat donor item does not have the desired stat.\r\n", ch);
			return;
		}

		/* Find the existing stat on the source */
		new_mod = 0;
		for (i = 0; i < MAX_OBJ_AFFECT; i++)
		{
			if (source->affected[i].location == apply_loc)
			{
				new_mod = source->affected[i].modifier;
				break;
			}
		}

		/* Cap at 2x original donor value */
		cap = orig_mod * 2;
		if (new_mod >= cap)
		{
			char buf[MAX_STRING_LENGTH];
			snprintf(buf, MAX_STRING_LENGTH, "&+yYour item already has the maximum %s modifier for this stat! (max %d)\r\n", enhance_stat_names[i].display_name, cap);
			send_to_char(buf, ch);
			return;
		}

		/* Apply the stat */
		new_mod += orig_mod;
		if (new_mod > cap)
			new_mod = cap;

		/* Set the affect */
		for (i = 0; i < MAX_OBJ_AFFECT; i++)
		{
			if (source->affected[i].location == 0 || source->affected[i].location == apply_loc)
			{
				source->affected[i].location = apply_loc;
				source->affected[i].modifier = new_mod;
				break;
			}
		}

		/* Consume the donor */
		obj_from_char(donor);
		extract_obj(donor);

		{
			char buf[MAX_STRING_LENGTH];
			snprintf(buf, MAX_STRING_LENGTH,
			         "&+BYour &+Wenhancement&+B thrums with %s%s&+B energy!  Your item's %s&+B has grown to &+W%d&n!\r\n",
			         enhance_stat_names[i].color,
			         enhance_stat_names[i].display_name,
			         enhance_stat_names[i].display_name,
			         new_mod);
			send_to_char(buf, ch);
		}

		statuslog(ch->player.level,
		          "&+BStat-Enhance&n: %s&n enhanced %s '%s&n' with %s (%d) at [%d]!",
		          GET_NAME(ch),
		          enhance_stat_names[i].color,
		          source->short_description,
		          enhance_stat_names[i].display_name,
		          new_mod,
		          (ch->in_room == NOWHERE) ? -1 : world[ch->in_room].number);
		return;
	}

	/* Original 2-arg enhance */
	half_chop(argument, first, rest);
	half_chop(rest, second, rest);

	if (!(source = get_obj_in_list_vis(ch, first, ch->carrying)))
	{
		act("&+yWhich &+Witem &+ywould you like to &+men&+Mhan&+mce&+y?", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	if (!(material = get_obj_in_list_vis(ch, second, ch->carrying)))
	{
		act("And which object is the enhancement object?", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}
	if (!strcmp(first, second))
	{
		send_to_char("&+yYou cannot enhance an item with itself!\r\n", ch);
		return;
	}
	if (!is_salvageable(source))
	{
		act("&+yYour $p&+y cannot be used in this way... try something else&n.", FALSE, ch, source, 0, TO_CHAR);
		return;
	}
	if (!is_salvageable(material))
	{
		act("&+yYour $p&+y cannot be used in this way... try something else&n.", FALSE, ch, material, 0, TO_CHAR);
		return;
	}
	else if (OBJ_VNUM(material) == OBJ_VNUM(source))
	{
		send_to_char("&+yYou cannot enhance an item with itself!\r\n", ch);
		return;
	}
	// If source is a weapon, material must be either a weapon or an essence.
	if (IS_SET(source->wear_flags, ITEM_WIELD) && !IS_SET(material->wear_flags, ITEM_WIELD) && (OBJ_VNUM(material) < 400238 || OBJ_VNUM(material) > 400258))
	{
		send_to_char("&+YWeapons&+y can only enhance other &+Yweapons&n!\r\n", ch);
		return;
	}

	if (OBJ_VNUM(material) > 400237 && OBJ_VNUM(material) < 400259)
		modenhance(ch, source, material);
	else
		enhance(ch, source, material);
}

void modenhance(P_char ch, P_obj source, P_obj material)
{

	if (!ch || !source || !material)
		return;

	char  buf[MAX_STRING_LENGTH], modstring[MAX_STRING_LENGTH];
	P_obj robj;
	long  robjint;
	int   mod = 0, loctype = 0;
	int   validobj, cost, searchcount = 0, tries;
	int   sval = itemvalue(source);
	validobj   = 0;
	int val    = itemvalue(material);
	int minval = itemvalue(source) - 5;

	if (val <= 20)
	{
		cost = 1000;
		if (GET_MONEY(ch) < 1000)
		{
			send_to_char("It will require &+W1 platinum&n to &+Benhance&n this item.\r\n", ch);
			return;
		}
	}
	if (val > 20 && val < 30)
	{
		cost = 10000;
		if (GET_MONEY(ch) < 20000)
		{
			send_to_char("It will require &+W20 platinum&n to &+Benhance&n this item.\r\n", ch);
			return;
		}
	}
	if (val > 30)
	{
		cost = 50000;
		if (GET_MONEY(ch) < 100000)
		{
			send_to_char("It will require &+W100 platinum&n to &+Benhance&n this item.\r\n", ch);
			return;
		}
	}

	int modtype = OBJ_VNUM(material);
	switch (modtype)
	{
		case 400238:
			if (source->affected[2].location == APPLY_INT)
				loctype = 1;
			else
				source->affected[2].location = APPLY_INT;
			snprintf(modstring, MAX_STRING_LENGTH, "&+wof &+Mintelligence&n");
			mod = 1;
			break;
		case 400239:
			if (source->affected[2].location == APPLY_INT_MAX)
				loctype = 1;
			else
				source->affected[2].location = APPLY_INT_MAX;
			snprintf(modstring, MAX_STRING_LENGTH, "&+wof &+Mgreater intelligence&n");
			mod = 1;
			break;
		case 400240:
			if (source->affected[2].location == APPLY_CON)
				loctype = 1;
			else
				source->affected[2].location = APPLY_CON;
			snprintf(modstring, MAX_STRING_LENGTH, "&+wof &+cconstitution&n");
			mod = 1;
			break;
		case 400241:
			if (source->affected[2].location == APPLY_CON_MAX)
				loctype = 1;
			else
				source->affected[2].location = APPLY_CON_MAX;
			snprintf(modstring, MAX_STRING_LENGTH, "&+wof &+cgreater constitution&n");
			mod = 1;
			break;
		case 400242:
			if (source->affected[2].location == APPLY_AGI)
				loctype = 1;
			else
				source->affected[2].location = APPLY_AGI;
			snprintf(modstring, MAX_STRING_LENGTH, "&+wof &+Bagility&n");
			mod = 1;
			break;
		case 400243:
			if (source->affected[2].location == APPLY_AGI_MAX)
				loctype = 1;
			else
				source->affected[2].location = APPLY_AGI_MAX;
			snprintf(modstring, MAX_STRING_LENGTH, "&+wof &+Bgreater agility&n");
			mod = 1;
			break;
		case 400244:
			if (source->affected[2].location == APPLY_DEX)
				loctype = 1;
			else
				source->affected[2].location = APPLY_DEX;
			snprintf(modstring, MAX_STRING_LENGTH, "&+wof &+gdexterity&n");
			mod = 1;
			break;
		case 400245:
			if (source->affected[2].location == APPLY_DEX_MAX)
				loctype = 1;
			else
				source->affected[2].location = APPLY_DEX_MAX;
			snprintf(modstring, MAX_STRING_LENGTH, "&+wof &+ggreater dexterity&n");
			mod = 1;
			break;
		case 400246:
			if (source->affected[2].location == APPLY_STR)
				loctype = 1;
			else
				source->affected[2].location = APPLY_STR;
			snprintf(modstring, MAX_STRING_LENGTH, "&+wof &+rstrength&n");
			mod = 1;
			break;
		case 400247:
			if (source->affected[2].location == APPLY_STR_MAX)
				loctype = 1;
			else
				source->affected[2].location = APPLY_STR_MAX;
			snprintf(modstring, MAX_STRING_LENGTH, "&+wof &+rgreater strength&n");
			mod = 1;
			break;
		case 400248:
			if (source->affected[2].location == APPLY_CHA)
				loctype = 1;
			else
				source->affected[2].location = APPLY_CHA;
			snprintf(modstring, MAX_STRING_LENGTH, "&+wof &+Ccharisma&n");
			mod = 1;
			break;
		case 400249:
			if (source->affected[2].location == APPLY_CHA_MAX)
				loctype = 1;
			else
				source->affected[2].location = APPLY_CHA_MAX;
			snprintf(modstring, MAX_STRING_LENGTH, "&+wof &+Cgreater charisma&n");
			mod = 1;
			break;
		case 400250:
			if (source->affected[2].location == APPLY_WIS)
				loctype = 1;
			else
				source->affected[2].location = APPLY_WIS;
			snprintf(modstring, MAX_STRING_LENGTH, "&+wof &+cwisdom&n");
			mod = 1;
			break;
		case 400251:
			if (source->affected[2].location == APPLY_WIS_MAX)
				loctype = 1;
			else
				source->affected[2].location = APPLY_WIS_MAX;
			snprintf(modstring, MAX_STRING_LENGTH, "&+wof &+cgreater wisdom&n");
			mod = 1;
			break;
		case 400252:
			if (source->affected[2].location == APPLY_POW)
				loctype = 1;
			else
				source->affected[2].location = APPLY_POW;
			snprintf(modstring, MAX_STRING_LENGTH, "&+wof &+Lpower&n");
			mod = 1;
			break;
		case 400253:
			if (source->affected[2].location == APPLY_POW_MAX)
				loctype = 1;
			else
				source->affected[2].location = APPLY_POW_MAX;
			snprintf(modstring, MAX_STRING_LENGTH, "&+wof &+Lgreater power&n");
			mod = 1;
			break;
		case 400254:
			if (source->affected[2].location == APPLY_HIT)
				loctype = 1;
			else
				source->affected[2].location = APPLY_HIT;
			snprintf(modstring, MAX_STRING_LENGTH, "&+wof &+Rhealth&n");
			mod = 3;
			break;
		case 400255:
			if (source->affected[2].location == APPLY_HITROLL)
				loctype = 1;
			else
				source->affected[2].location = APPLY_HITROLL;
			snprintf(modstring, MAX_STRING_LENGTH, "&+wof &+yprecision&n");
			mod = 1;
			break;
		case 400256:
			if (source->affected[2].location == APPLY_DAMROLL)
				loctype = 1;
			else
				source->affected[2].location = APPLY_DAMROLL;
			snprintf(modstring, MAX_STRING_LENGTH, "&+wof &+ydamage&n");
			mod = 1;
			break;
		case 400257:
			if (source->affected[2].location == APPLY_HIT_REG)
				loctype = 1;
			else
				source->affected[2].location = APPLY_HIT_REG;
			snprintf(modstring, MAX_STRING_LENGTH, "&+wof &+gregeneration&n");
			mod = 3;
			break;
		case 400258:
			if (source->affected[2].location == APPLY_MOVE_REG)
				loctype = 1;
			else
				source->affected[2].location = APPLY_MOVE_REG;
			snprintf(modstring, MAX_STRING_LENGTH, "&+wof &+Gendurance&n");
			mod = 3;
			break;

		default:
			break;
	}

	if (loctype == 1)
	{
		// IF they've been modified less than 3 times.
		if (source->affected[2].modifier / mod < 3)
			source->affected[2].modifier += mod;
		else
		{
			send_to_char("Your enhancement was a failure.  Too much magic.\n", ch);
			return;
		}
	}
	else
		source->affected[2].modifier = mod;

	SUB_MONEY(ch, cost, 0);
	send_to_char("Your pockets feel &+Wlighter&n.\r\n", ch);

	act("&+BYour enhancement is a success! Your &n$p&+B now feels slightly more powerful!\r\n", FALSE, ch, source, 0, TO_CHAR);

	obj_from_char(material);
	extract_obj(material);

	P_obj tempobj = read_object(OBJ_VNUM(source), VIRTUAL);
	char  tempdesc[MAX_STRING_LENGTH], short_desc[MAX_STRING_LENGTH], keywords[MAX_STRING_LENGTH];

	snprintf(keywords, MAX_STRING_LENGTH, "%s enhanced", tempobj->name);

	snprintf(tempdesc, MAX_STRING_LENGTH, "%s", tempobj->short_description);
	snprintf(short_desc, MAX_STRING_LENGTH, "%s %s&n", tempdesc, modstring);
	set_keywords(source, keywords);
	set_short_description(source, short_desc);
	extract_obj(tempobj);

	return;
}

int get_progress(P_char ch, int ach, uint required)
{
	int                   prog = 0, percentage = 0;
	struct affected_type *findaf, *next_af; // initialize affects

	for (findaf = ch->affected; findaf; findaf = next_af)
	{
		next_af = findaf->next;
		if (findaf && findaf->type == ach)
			prog = findaf->modifier;
	}

	if (prog > 0)
	{
		prog *= 100;
		percentage = prog / required;
	}

	if (prog < 0)
		return 0;

	return percentage;
}

void thanksgiving_proc(P_char ch)
{
	P_char mob;
	if (!ch)
		return;
	char buff[MAX_STRING_LENGTH];
	snprintf(buff, MAX_STRING_LENGTH, " %s 86", GET_NAME(ch));
	act("&+YSuddenly and without warning, a &+rPlump &+yTurkey &+Yappears out of no where, seemly attracted to the freshly spilled &+Rblood&n!", TRUE, ch, 0, 0, TO_CHAR);
	act("&+YSuddenly and without warning, a &+rPlump &+yTurkey &+Yappears out of no where, seemly attracted to the freshly spilled &+Rblood&n!", TRUE, ch, 0, 0, TO_ROOM);
	// do_givepet(ch, buff, CMD_GIVEPET);
	mob = read_mobile(400005, VIRTUAL);
	if (!mob)
		return;
	obj_to_char(read_object(400232, VIRTUAL), mob);
	char_to_room(mob, ch->in_room, 0);
}

// This function assumes ch exists and is a mob (Verified in fight.c before call).
void enhancematload(P_char ch, P_char killer)
{
	int reward;
	int moblvl = GET_LEVEL(ch);
	if (IS_ELITE(ch))
	{
		moblvl * 10;
	}
	if (number(1, 3000) < moblvl)
	{
		debug("enhancematload: mob: '%s' (%d) moblvl %d%s", J_NAME(ch), GET_VNUM(ch), moblvl, IS_ELITE(ch) ? " ELITE." : ".");
		if (number(1, 4000) < moblvl)
		{
			switch (number(1, 8))
			{
				case 1:
					reward = 400239;
					break;
				case 2:
					reward = 400241;
					break;
				case 3:
					reward = 400243;
					break;
				case 4:
					reward = 400245;
					break;
				case 5:
					reward = 400247;
					break;
				case 6:
					reward = 400249;
					break;
				case 7:
					reward = 400251;
					break;
				case 8:
					reward = 400253;
					break;
			}
		}
		else
		{
			reward = number(1, 13);
			switch (reward)
			{
				case 1:
					reward = 400238;
					break;
				case 2:
					reward = 400240;
					break;
				case 3:
					reward = 400242;
					break;
				case 4:
					reward = 400244;
					break;
				case 5:
					reward = 400246;
					break;
				case 6:
					reward = 400248;
					break;
				case 7:
					reward = 400250;
					break;
				case 8:
					reward = 400252;
					break;
				case 9:
					reward = 400254;
					break;
				case 10:
					reward = 400255;
					break;
				case 11:
					reward = 400256;
					break;
				case 12:
					reward = 400257;
					break;
				case 13:
					reward = 400258;
					break;
			}
		}
		P_obj gift = read_object(reward, VIRTUAL);
		if (gift)
		{
			// Show reward to master if killer is a pet.
			debug("enhancematload: '%s' (%d) rewarded to %s.", gift->short_description, OBJ_VNUM(gift), IS_PC_PET(killer) ? J_NAME(get_linked_char(killer, LNK_PET)) : J_NAME(killer));
					obj_to_char(gift, ch);
				}
			}
		}

		/* =============================================================================
		 *  ENHANCE SYSTEM — GLOBALS, HASH TABLES, CONFIG PARSER, INDEX BUILDER
		 * =============================================================================
		 */

		/* ---- Config settings (defaults) ---- */
		int enhance_ival_cap                         = 95;
		int enhance_material_ival_delta              = 5;
		int enhance_guild_insignia_ival_bonus        = 5;
		int enhance_cost_low_ival_threshold          = 20;
		int enhance_cost_low_amount                  = 1000;
		int enhance_cost_high_amount                 = 10000;
		int enhance_search_vnum_min                  = 1300;
		int enhance_search_vnum_max                  = 134000;
		int enhance_search_max_attempts              = 20000;
		int enhance_wear_skip_mask                   = 15;
		int enhance_original_max_roll                = 4;
		int enhance_original_cascade_down_first      = 1;
		int enhance_level_gate_a                     = 41;
		int enhance_level_gate_b                     = 3;
		int enhance_level_gate_c                     = 1;
		int enhance_luck_extreme_range               = 1200;
		int enhance_luck_very_range                  = 800;
		int enhance_luck_lucky_range                 = 400;
		int enhance_ival_gain_extreme                = 4;
		int enhance_ival_gain_very                   = 3;
		int enhance_ival_gain_lucky                  = 2;
		int enhance_ival_gain_normal                 = 1;

		/* ---- Bitvector allow masks ---- */
		unsigned long enhance_allow_mask  = 0;
		unsigned long enhance_allow_mask2 = 0;
		unsigned long enhance_allow_mask3 = 0;
		unsigned long enhance_allow_mask4 = 0;
		unsigned long enhance_allow_mask5 = 0;

		/* ---- Hash tables ---- */
		struct enhance_index_entry *enhance_ival_table[ENHANCE_IVAL_TABLE_SIZE] = {0};
		struct enhance_index_entry *enhance_stat_table[ENHANCE_STAT_TABLE_SIZE] = {0};

		/* ---- Flag name → bitvalue lookup table ---- */
		static const struct {
			const char     *name;
			unsigned long   bit;
			int             section; /* 0=bitvector, 1=bitvector2, 2=bitvector3, 3=bitvector4, 4=bitvector5 */
		} enhance_flag_lookup[] = {
			/* bitvector (AFF_) */
			{"AFF_NONE",              AFF_NONE,             0},
			{"AFF_BLIND",             AFF_BLIND,            0},
			{"AFF_INVISIBLE",         AFF_INVISIBLE,        0},
			{"AFF_FARSEE",            AFF_FARSEE,           0},
			{"AFF_DETECT_INVISIBLE",  AFF_DETECT_INVISIBLE, 0},
			{"AFF_HASTE",             AFF_HASTE,            0},
			{"AFF_SENSE_LIFE",        AFF_SENSE_LIFE,       0},
			{"AFF_MINOR_GLOBE",       AFF_MINOR_GLOBE,      0},
			{"AFF_STONE_SKIN",        AFF_STONE_SKIN,       0},
			{"AFF_UD_VISION",         AFF_UD_VISION,        0},
			{"AFF_ARMOR",             AFF_ARMOR,            0},
			{"AFF_WRAITHFORM",        AFF_WRAITHFORM,       0},
			{"AFF_WATERBREATH",       AFF_WATERBREATH,      0},
			{"AFF_KNOCKED_OUT",       AFF_KNOCKED_OUT,      0},
			{"AFF_PROTECT_EVIL",      AFF_PROTECT_EVIL,     0},
			{"AFF_BOUND",             AFF_BOUND,            0},
			{"AFF_SLOW_POISON",       AFF_SLOW_POISON,      0},
			{"AFF_PROTECT_GOOD",      AFF_PROTECT_GOOD,     0},
			{"AFF_SLEEP",             AFF_SLEEP,            0},
			{"AFF_SKILL_AWARE",       AFF_SKILL_AWARE,      0},
			{"AFF_SNEAK",             AFF_SNEAK,            0},
			{"AFF_HIDE",              AFF_HIDE,             0},
			{"AFF_FEAR",              AFF_FEAR,             0},
			{"AFF_CHARM",             AFF_CHARM,            0},
			{"AFF_MEDITATE",          AFF_MEDITATE,         0},
			{"AFF_BARKSKIN",          AFF_BARKSKIN,         0},
			{"AFF_INFRAVISION",       AFF_INFRAVISION,      0},
			{"AFF_LEVITATE",          AFF_LEVITATE,         0},
			{"AFF_FLY",               AFF_FLY,              0},
			{"AFF_AWARE",             AFF_AWARE,            0},
			{"AFF_PROT_FIRE",         AFF_PROT_FIRE,        0},
			{"AFF_CAMPING",           AFF_CAMPING,          0},
			{"AFF_BIOFEEDBACK",       AFF_BIOFEEDBACK,      0},
			{"AFF_INFERNAL_FURY",     AFF_INFERNAL_FURY,    0},
			{"AFF_FREEDOM_OF_MVMNT",  AFF_FREEDOM_OF_MVMNT, 0},
			{"AFF_SANCTUM_DRACONIS",  AFF_SANCTUM_DRACONIS, 0},
			/* bitvector2 (AFF2_) */
			{"AFF2_FIRESHIELD",       AFF2_FIRESHIELD,      1},
			{"AFF2_ULTRAVISION",      AFF2_ULTRAVISION,     1},
			{"AFF2_DETECT_EVIL",      AFF2_DETECT_EVIL,     1},
			{"AFF2_DETECT_GOOD",      AFF2_DETECT_GOOD,     1},
			{"AFF2_DETECT_MAGIC",     AFF2_DETECT_MAGIC,    1},
			{"AFF2_MAJOR_PHYSICAL",   AFF2_MAJOR_PHYSICAL,  1},
			{"AFF2_PROT_COLD",        AFF2_PROT_COLD,       1},
			{"AFF2_PROT_LIGHTNING",   AFF2_PROT_LIGHTNING,  1},
			{"AFF2_MINOR_PARALYSIS",  AFF2_MINOR_PARALYSIS, 1},
			{"AFF2_MAJOR_PARALYSIS",  AFF2_MAJOR_PARALYSIS, 1},
			{"AFF2_SLOW",             AFF2_SLOW,            1},
			{"AFF2_GLOBE",            AFF2_GLOBE,           1},
			{"AFF2_PROT_GAS",         AFF2_PROT_GAS,        1},
			{"AFF2_PROT_ACID",        AFF2_PROT_ACID,       1},
			{"AFF2_POISONED",         AFF2_POISONED,        1},
			{"AFF2_SOULSHIELD",       AFF2_SOULSHIELD,      1},
			{"AFF2_SILENCED",         AFF2_SILENCED,        1},
			{"AFF2_CONCEALMENT",      AFF2_CONCEALMENT,     1},
			{"AFF2_VAMPIRIC_TOUCH",   AFF2_VAMPIRIC_TOUCH,  1},
			{"AFF2_STUNNED",          AFF2_STUNNED,         1},
			{"AFF2_EARTH_AURA",       AFF2_EARTH_AURA,      1},
			{"AFF2_WATER_AURA",       AFF2_WATER_AURA,      1},
			{"AFF2_FIRE_AURA",        AFF2_FIRE_AURA,       1},
			{"AFF2_AIR_AURA",         AFF2_AIR_AURA,        1},
			{"AFF2_HOLDING_BREATH",   AFF2_HOLDING_BREATH,  1},
			{"AFF2_MEMORIZING",       AFF2_MEMORIZING,      1},
			{"AFF2_IS_DROWNING",      AFF2_IS_DROWNING,     1},
			{"AFF2_PASSDOOR",         AFF2_PASSDOOR,        1},
			{"AFF2_FLURRY",           AFF2_FLURRY,          1},
			{"AFF2_CASTING",          AFF2_CASTING,         1},
			{"AFF2_SCRIBING",         AFF2_SCRIBING,        1},
			{"AFF2_HUNTER",           AFF2_HUNTER,          1},
			/* bitvector3 (AFF3_) */
			{"AFF3_TENSORS_DISC",       AFF3_TENSORS_DISC,       2},
			{"AFF3_TRACKING",           AFF3_TRACKING,           2},
			{"AFF3_SINGING",            AFF3_SINGING,            2},
			{"AFF3_ECTOPLASMIC_FORM",   AFF3_ECTOPLASMIC_FORM,   2},
			{"AFF3_ABSORBING",          AFF3_ABSORBING,          2},
			{"AFF3_PROT_ANIMAL",        AFF3_PROT_ANIMAL,        2},
			{"AFF3_SPIRIT_WARD",        AFF3_SPIRIT_WARD,        2},
			{"AFF3_GR_SPIRIT_WARD",     AFF3_GR_SPIRIT_WARD,     2},
			{"AFF3_NON_DETECTION",      AFF3_NON_DETECTION,      2},
			{"AFF3_SILVER",             AFF3_SILVER,             2},
			{"AFF3_PLUSONE",            AFF3_PLUSONE,            2},
			{"AFF3_PLUSTWO",            AFF3_PLUSTWO,            2},
			{"AFF3_PLUSTHREE",          AFF3_PLUSTHREE,          2},
			{"AFF3_PLUSFOUR",           AFF3_PLUSFOUR,           2},
			{"AFF3_PLUSFIVE",           AFF3_PLUSFIVE,           2},
			{"AFF3_ENLARGE",            AFF3_ENLARGE,            2},
			{"AFF3_REDUCE",             AFF3_REDUCE,             2},
			{"AFF3_COVER",              AFF3_COVER,              2},
			{"AFF3_FOUR_ARMS",          AFF3_FOUR_ARMS,          2},
			{"AFF3_INERTIAL_BARRIER",   AFF3_INERTIAL_BARRIER,   2},
			{"AFF3_LIGHTNINGSHIELD",    AFF3_LIGHTNINGSHIELD,    2},
			{"AFF3_COLDSHIELD",         AFF3_COLDSHIELD,         2},
			{"AFF3_CANNIBALIZE",        AFF3_CANNIBALIZE,        2},
			{"AFF3_SWIMMING",           AFF3_SWIMMING,           2},
			{"AFF3_TOWER_IRON_WILL",    AFF3_TOWER_IRON_WILL,    2},
			{"AFF3_UNDERWATER",         AFF3_UNDERWATER,         2},
			{"AFF3_BLUR",               AFF3_BLUR,               2},
			{"AFF3_ENHANCE_HEALING",    AFF3_ENHANCE_HEALING,    2},
			{"AFF3_ELEMENTAL_FORM",     AFF3_ELEMENTAL_FORM,     2},
			{"AFF3_PASS_WITHOUT_TRACE", AFF3_PASS_WITHOUT_TRACE, 2},
			{"AFF3_PALADIN_AURA",       AFF3_PALADIN_AURA,       2},
			{"AFF3_FAMINE",             AFF3_FAMINE,             2},
			{"AFF3_VIVERNAE_CONCORDIA", AFF3_VIVERNAE_CONCORDIA, 2},
			/* bitvector4 (AFF4_) */
			{"AFF4_LOOTER",                   AFF4_LOOTER,                   3},
			{"AFF4_CARRY_PLAGUE",             AFF4_CARRY_PLAGUE,             3},
			{"AFF4_SACKING",                  AFF4_SACKING,                  3},
			{"AFF4_SENSE_FOLLOWER",           AFF4_SENSE_FOLLOWER,           3},
			{"AFF4_STORNOGS_SPHERES",         AFF4_STORNOGS_SPHERES,         3},
			{"AFF4_STORNOGS_GREATER_SPHERES", AFF4_STORNOGS_GREATER_SPHERES, 3},
			{"AFF4_VAMPIRE_FORM",             AFF4_VAMPIRE_FORM,             3},
			{"AFF4_NO_UNMORPH",               AFF4_NO_UNMORPH,               3},
			{"AFF4_HOLY_SACRIFICE",           AFF4_HOLY_SACRIFICE,           3},
			{"AFF4_BATTLE_ECSTASY",           AFF4_BATTLE_ECSTASY,           3},
			{"AFF4_DAZZLER",                  AFF4_DAZZLER,                  3},
			{"AFF4_PHANTASMAL_FORM",          AFF4_PHANTASMAL_FORM,          3},
			{"AFF4_NOFEAR",                   AFF4_NOFEAR,                   3},
			{"AFF4_REGENERATION",             AFF4_REGENERATION,             3},
			{"AFF4_DEAF",                     AFF4_DEAF,                     3},
			{"AFF4_BATTLETIDE",               AFF4_BATTLETIDE,               3},
			{"AFF4_EPIC_INCREASE",            AFF4_EPIC_INCREASE,            3},
			{"AFF4_MAGE_FLAME",               AFF4_MAGE_FLAME,               3},
			{"AFF4_GLOBE_OF_DARKNESS",        AFF4_GLOBE_OF_DARKNESS,        3},
			{"AFF4_DEFLECT",                  AFF4_DEFLECT,                  3},
			{"AFF4_HAWKVISION",               AFF4_HAWKVISION,               3},
			{"AFF4_MULTI_CLASS",              AFF4_MULTI_CLASS,              3},
			{"AFF4_SANCTUARY",                AFF4_SANCTUARY,                3},
			{"AFF4_HELLFIRE",                 AFF4_HELLFIRE,                 3},
			{"AFF4_SENSE_HOLINESS",           AFF4_SENSE_HOLINESS,           3},
			{"AFF4_PROT_LIVING",              AFF4_PROT_LIVING,              3},
			{"AFF4_DETECT_ILLUSION",          AFF4_DETECT_ILLUSION,          3},
			{"AFF4_ICE_AURA",                 AFF4_ICE_AURA,                 3},
			{"AFF4_REV_POLARITY",             AFF4_REV_POLARITY,             3},
			{"AFF4_NEG_SHIELD",               AFF4_NEG_SHIELD,               3},
			{"AFF4_TUPOR",                    AFF4_TUPOR,                    3},
			{"AFF4_WILDMAGIC",                AFF4_WILDMAGIC,                3},
			/* bitvector5 (AFF5_) */
			{"AFF5_DAZZLEE",           AFF5_DAZZLEE,           4},
			{"AFF5_MENTAL_ANGUISH",    AFF5_MENTAL_ANGUISH,    4},
			{"AFF5_MEMORY_BLOCK",      AFF5_MEMORY_BLOCK,      4},
			{"AFF5_VINES",             AFF5_VINES,             4},
			{"AFF5_ETHEREAL_ALLIANCE", AFF5_ETHEREAL_ALLIANCE, 4},
			{"AFF5_BLOOD_SCENT",       AFF5_BLOOD_SCENT,       4},
			{"AFF5_FLESH_ARMOR",       AFF5_FLESH_ARMOR,       4},
			{"AFF5_WET",               AFF5_WET,               4},
			{"AFF5_HOLY_DHARMA",       AFF5_HOLY_DHARMA,       4},
			{"AFF5_ENH_HIDE",          AFF5_ENH_HIDE,          4},
			{"AFF5_LISTEN",            AFF5_LISTEN,            4},
			{"AFF5_PROT_UNDEAD",       AFF5_PROT_UNDEAD,       4},
			{"AFF5_IMPRISON",          AFF5_IMPRISON,          4},
			{"AFF5_TITAN_FORM",        AFF5_TITAN_FORM,        4},
			{"AFF5_DELIRIUM",          AFF5_DELIRIUM,          4},
			{"AFF5_SHADE_MOVEMENT",    AFF5_SHADE_MOVEMENT,    4},
			{"AFF5_NOBLIND",           AFF5_NOBLIND,           4},
			{"AFF5_MAGICAL_GLOW",      AFF5_MAGICAL_GLOW,      4},
			{"AFF5_REFRESHING_GLOW",   AFF5_REFRESHING_GLOW,   4},
			{"AFF5_MINE",              AFF5_MINE,              4},
			{"AFF5_STANCE_OFFENSIVE",  AFF5_STANCE_OFFENSIVE,  4},
			{"AFF5_STANCE_DEFENSIVE",  AFF5_STANCE_DEFENSIVE,  4},
			{"AFF5_OBSCURING_MIST",    AFF5_OBSCURING_MIST,    4},
			{"AFF5_NOT_OFFENSIVE",     AFF5_NOT_OFFENSIVE,     4},
			{"AFF5_DECAYING_FLESH",    AFF5_DECAYING_FLESH,    4},
			{"AFF5_DREADNAUGHT",       AFF5_DREADNAUGHT,       4},
			{"AFF5_FOREST_SIGHT",      AFF5_FOREST_SIGHT,      4},
			{"AFF5_THORNSKIN",         AFF5_THORNSKIN,         4},
			{"AFF5_FOLLOWING",         AFF5_FOLLOWING,         4},
			{"AFF5_ORDERING",          AFF5_ORDERING,          4},
			{"AFF5_STONED",            AFF5_STONED,            4},
			{"AFF5_JUDICIUM_FIDEI",    AFF5_JUDICIUM_FIDEI,    4},
			{NULL,                     0,                       0}
		};

		/* ---- Hash functions ---- */
		static int enhance_hash(int key)
		{
			int h = abs(key) % ENHANCE_IVAL_TABLE_SIZE;
			return h;
		}

		static int enhance_stat_hash(int key)
		{
			int h = abs(key) % ENHANCE_STAT_TABLE_SIZE;
			return h;
		}

		/* =============================================================================
		 *  load_enhance_config() — Parse lib/enhance.cfg for settings and bitvector masks
		 * =============================================================================
		 */
		void load_enhance_config(void)
		{
			FILE *fp;
			char  line[1024];
			char  section[64];
			int   section_idx;
			int   i, line_num;
			char  flag_name[128];
			char *p, *eq, *comma;

			fp = fopen("lib/enhance.cfg", "r");
			if (!fp)
			{
				fprintf(stderr, "WARNING: Cannot open lib/enhance.cfg — using defaults.\r\n");
				logit(LOG_STATUS, "WARNING: Cannot open lib/enhance.cfg — using defaults.");
				return;
			}

			section[0]   = '\0';
			section_idx  = -1;
			line_num     = 0;

			while (fgets(line, sizeof(line), fp))
			{
				line_num++;

				/* Strip trailing whitespace / newline */
				p = line + strlen(line);
				while (p > line && (*(p - 1) == '\n' || *(p - 1) == '\r' || *(p - 1) == ' ' || *(p - 1) == '	'))
					*--p = '\0';

				/* Skip empty lines and comments */
				if (line[0] == '\0' || line[0] == '#')
					continue;

				/* Detect section header */
				if (line[0] == '[')
				{
					p = line + 1;
					i = 0;
					while (*p && *p != ']' && i < (int)sizeof(section) - 1)
						section[i++] = *p++;
					section[i] = '\0';

					if      (!strcmp(section, "settings"))    section_idx = 0;
					else if (!strcmp(section, "enhance_stat")) section_idx = 1;
					else if (!strcmp(section, "bitvector"))    section_idx = 10;
					else if (!strcmp(section, "bitvector2"))   section_idx = 11;
					else if (!strcmp(section, "bitvector3"))   section_idx = 12;
					else if (!strcmp(section, "bitvector4"))   section_idx = 13;
					else if (!strcmp(section, "bitvector5"))   section_idx = 14;
					else if (!strcmp(section, "spell"))        section_idx = 20;
					else                                       section_idx = -1;
					continue;
				}

				/* Parse key = value lines */
				eq = strchr(line, '=');
				if (!eq)
					continue;

				*eq = '\0';
				/* Trim key */
				p = line;
				while (*p == ' ' || *p == '	')
					p++;
				char *key = p;
				p = eq - 1;
				while (p > key && (*p == ' ' || *p == '	'))
					*p-- = '\0';

				/* Trim value */
				p = eq + 1;
				while (*p == ' ' || *p == '	')
					p++;
				char *val = p;

				if (section_idx == 0)
				{
					/* [settings] section */
					int ival = atoi(val);

					if      (!strcmp(key, "enhance.ival.cap"))                      enhance_ival_cap                    = ival;
					else if (!strcmp(key, "enhance.material.ival.delta"))           enhance_material_ival_delta         = ival;
					else if (!strcmp(key, "enhance.guild.insignia.ival.bonus"))    enhance_guild_insignia_ival_bonus   = ival;
					else if (!strcmp(key, "enhance.cost.low.ival.threshold"))       enhance_cost_low_ival_threshold     = ival;
					else if (!strcmp(key, "enhance.cost.low.amount"))               enhance_cost_low_amount             = ival;
					else if (!strcmp(key, "enhance.cost.high.amount"))              enhance_cost_high_amount            = ival;
					else if (!strcmp(key, "enhance.search.vnum.min"))               enhance_search_vnum_min             = ival;
					else if (!strcmp(key, "enhance.search.vnum.max"))               enhance_search_vnum_max             = ival;
					else if (!strcmp(key, "enhance.search.max.attempts"))           enhance_search_max_attempts         = ival;
					else if (!strcmp(key, "enhance.wear.skip.mask"))                enhance_wear_skip_mask              = ival;
					else if (!strcmp(key, "enhance.original.max.roll"))             enhance_original_max_roll           = ival;
					else if (!strcmp(key, "enhance.original.cascade.down.first"))  enhance_original_cascade_down_first = ival;
					else if (!strcmp(key, "enhance.level.gate.a"))                  enhance_level_gate_a                = ival;
					else if (!strcmp(key, "enhance.level.gate.b"))                  enhance_level_gate_b                = ival;
					else if (!strcmp(key, "enhance.level.gate.c"))                  enhance_level_gate_c                = ival;
					else if (!strcmp(key, "enhance.luck.extreme.range"))            enhance_luck_extreme_range          = ival;
					else if (!strcmp(key, "enhance.luck.very.range"))               enhance_luck_very_range             = ival;
					else if (!strcmp(key, "enhance.luck.lucky.range"))              enhance_luck_lucky_range            = ival;
					else if (!strcmp(key, "enhance.ival.gain.extreme"))             enhance_ival_gain_extreme           = ival;
					else if (!strcmp(key, "enhance.ival.gain.very"))                enhance_ival_gain_very              = ival;
					else if (!strcmp(key, "enhance.ival.gain.lucky"))               enhance_ival_gain_lucky             = ival;
					else if (!strcmp(key, "enhance.ival.gain.normal"))              enhance_ival_gain_normal            = ival;
					/* skip unknown settings silently */
				}
				else if (section_idx >= 10 && section_idx <= 14)
				{
					/* [bitvector] through [bitvector5] sections */
					/* Format: FLAG_NAME = ival, allowed */
					comma = strchr(val, ',');
					if (!comma)
						continue;

					*comma = '\0';
					/* Trim allowed value */
					p = comma + 1;
					while (*p == ' ' || *p == '	')
						p++;
					int allowed = atoi(p);

					/* Trim flag name from key */
					strncpy(flag_name, key, sizeof(flag_name) - 1);
					flag_name[sizeof(flag_name) - 1] = '\0';

					/* Look up flag name */
					int bit_section = section_idx - 10; /* 0=bitvector, 1=bitvector2, ... */
					for (i = 0; enhance_flag_lookup[i].name; i++)
					{
						if (enhance_flag_lookup[i].section == bit_section &&
						    !strcmp(flag_name, enhance_flag_lookup[i].name))
						{
							if (allowed)
							{
								switch (bit_section)
								{
									case 0: SET_BIT(enhance_allow_mask,  enhance_flag_lookup[i].bit); break;
									case 1: SET_BIT(enhance_allow_mask2, enhance_flag_lookup[i].bit); break;
									case 2: SET_BIT(enhance_allow_mask3, enhance_flag_lookup[i].bit); break;
									case 3: SET_BIT(enhance_allow_mask4, enhance_flag_lookup[i].bit); break;
									case 4: SET_BIT(enhance_allow_mask5, enhance_flag_lookup[i].bit); break;
								}
							}
							break;
						}
					}
				}
				/* ignore [enhance_stat] and [spell] sections for now */
			}

			fclose(fp);
			fprintf(stderr, "-- Loaded enhance config from lib/enhance.cfg\r\n");
			logit(LOG_STATUS, "Loaded enhance config from lib/enhance.cfg");
		}

		/* =============================================================================
		 *  is_enhance_banned() — Return TRUE if item has any AFF bit not in allow masks
		 * =============================================================================
		 */
		bool is_enhance_banned(P_obj item)
		{
			if (!item)
				return TRUE;

			/* If the item has any AFF bit set that is NOT in the allow mask, it's banned */
			if (item->bitvector  & ~enhance_allow_mask)  return TRUE;
			if (item->bitvector2 & ~enhance_allow_mask2) return TRUE;
			if (item->bitvector3 & ~enhance_allow_mask3) return TRUE;
			if (item->bitvector4 & ~enhance_allow_mask4) return TRUE;
			if (item->bitvector5 & ~enhance_allow_mask5) return TRUE;

			return FALSE;
		}

		/* =============================================================================
		 *  load_enhance_index() — Build ival and stat hash tables from vnum range
		 * =============================================================================
		 */
		void load_enhance_index(void)
		{
			int    vnum, ival, h, sh, j, count;
			P_obj  obj;
			struct enhance_index_entry *entry;

			fprintf(stderr, "-- Building enhance index (vnum %d to %d)...\r\n",
			        enhance_search_vnum_min, enhance_search_vnum_max);
			logit(LOG_STATUS, "Building enhance index (vnum %d to %d)...",
			      enhance_search_vnum_min, enhance_search_vnum_max);

			count = 0;

			for (vnum = enhance_search_vnum_min; vnum <= enhance_search_vnum_max; vnum++)
			{
				obj = read_object(vnum, VIRTUAL);
				if (!obj)
					continue;

				/* Validate with same criteria as enhance() */
				if (!IS_SET(obj->wear_flags, ITEM_TAKE) ||
				    IS_SET(obj->extra_flags, ITEM_ARTIFACT) ||
				    IS_SET(obj->extra_flags, ITEM_NOSELL) ||
				    IS_SET(obj->extra_flags, ITEM_NORENT) ||
				    IS_SET(obj->extra_flags, ITEM_NOSHOW) ||
				    IS_SET(obj->extra_flags, ITEM_TRANSIENT) ||
				    IS_OBJ_STAT2(obj, ITEM2_QUESTITEM))
				{
					extract_obj(obj);
					continue;
				}

				if (obj->type == ITEM_STAFF && obj->value[3] > 0)
				{
					extract_obj(obj);
					continue;
				}

				if (obj->type == ITEM_TREASURE || obj->type == ITEM_POTION ||
				    obj->type == ITEM_MONEY || obj->type == ITEM_KEY || obj->type == ITEM_WAND)
				{
					extract_obj(obj);
					continue;
				}

				ival = itemvalue(obj);

				/* Allocate and fill index entry */
				entry = (struct enhance_index_entry *)malloc(sizeof(struct enhance_index_entry));
				if (!entry)
				{
					extract_obj(obj);
					continue;
				}

				entry->vnum       = vnum;
				entry->ival       = ival;
				entry->wear_flags = obj->wear_flags;
				entry->material   = obj->material;

				for (j = 0; j < MAX_OBJ_AFFECT; j++)
				{
					entry->apply_loc[j] = obj->affected[j].location;
					entry->apply_mod[j] = obj->affected[j].modifier;
				}

				/* Insert into ival hash table */
				h = enhance_hash(ival);
				entry->next = enhance_ival_table[h];
				enhance_ival_table[h] = entry;

				/* Insert into stat hash table (by each apply location that has a modifier) */
				for (j = 0; j < MAX_OBJ_AFFECT; j++)
				{
					if (obj->affected[j].location != APPLY_NONE && obj->affected[j].modifier != 0)
					{
						/* Allocate a separate entry for stat table */
						struct enhance_index_entry *stat_entry;
						stat_entry = (struct enhance_index_entry *)malloc(sizeof(struct enhance_index_entry));
						if (!stat_entry)
							continue;

						memcpy(stat_entry, entry, sizeof(struct enhance_index_entry));
						sh = enhance_stat_hash(obj->affected[j].location);
						stat_entry->next = enhance_stat_table[sh];
						enhance_stat_table[sh] = stat_entry;
					}
				}

				extract_obj(obj);
				count++;
			}

			fprintf(stderr, "-- Enhance index built: %d entries indexed\r\n", count);
			logit(LOG_STATUS, "Enhance index built: %d entries indexed", count);
		}

