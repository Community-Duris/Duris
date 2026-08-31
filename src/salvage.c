/* Salvage subsystem: eligibility, examination, and player command. */
#include "prototypes.h"
#include "structs.h"
#include "net/comm.h"
#include "db.h"
#include "events.h"
#include "cmd/interp.h"
#include "item_movement_transaction.h"
#include "objmisc.h"
#include "spells.h"
#include "utility.h"
#include "utils.h"
#include "tradeskill.h"
#include "crafting.h"
#include "vnum.obj.h"
#include <stdio.h>
#include <string.h>
extern P_room world;
extern const int top_of_world;
extern P_index obj_index;

namespace
{
bool grant_salvage_item(P_char ch, P_obj object)
{
	if (object && item_creation_grant_submit_to_player(ch, object, ch))
		return true;
	if (object)
		extract_obj(object, FALSE);
	send_to_char("The ownership authority is busy; no salvage item was created.\r\n", ch);
	return false;
}
}

bool is_salvageable(P_obj temp)
{
	if (!temp || GET_ITEM_TYPE(temp) == ITEM_CORPSE)
		return FALSE;

	if (OBJ_VNUM(temp) > 400237 && OBJ_VNUM(temp) < 400259)
	{
		return TRUE;
	}

	// Make sure its not food or container
	if ((temp->type == ITEM_CONTAINER || temp->type == ITEM_STORAGE) && temp->contains)
	{
		return FALSE;
	}

	if (IS_SET(temp->extra_flags, ITEM_NOSELL))
	{
		return FALSE;
	}

	if (IS_SET(temp->extra2_flags, ITEM2_SOULBIND))
	{
		return FALSE;
	}

	if (temp->type == ITEM_WAND)
	{
		return FALSE;
	}

	switch (OBJ_VNUM(temp))
	{
	case VOBJ_RANDOM_ARMOR:
	case VOBJ_RANDOM_THRUSTED:
	case VOBJ_RANDOM_WEAPON:
	case 98:
	case 352:
	case 366:
		return FALSE;
	}

	if (temp->type == ITEM_FOOD)
	{
		return FALSE;
	}

	if (temp->type == ITEM_TREASURE || temp->type == ITEM_POTION || temp->type == ITEM_MONEY ||
	    temp->type == ITEM_KEY)
	{
		return FALSE;
	}
	if (IS_OBJ_STAT2(temp, ITEM2_STOREITEM))
	{
		return FALSE;
	}
	if (IS_SET(temp->extra_flags, ITEM_ARTIFACT))
	{
		return FALSE;
	}
	if ((temp->type == ITEM_STAFF) && (temp->value[3] > 0))
	{
		return FALSE;
	}

	return TRUE;
}

void salvage_examine_item(P_char ch, P_obj item)
{
	int skill;
	if (!ch || !item || (skill = GET_CHAR_SKILL(ch, SKILL_SALVAGE)) < 1)
		return;
	if (!is_salvageable(item))
	{
		act("&+ySalvage assessment:&n $p cannot yield usable salvage materials.", FALSE, ch,
		    item, 0, TO_CHAR);
		return;
	}
	act("&+ySalvage assessment:&n $p can be broken down for salvage materials.", FALSE, ch,
	    item, 0, TO_CHAR);
	if (skill >= 50)
		send_to_char("  Expect one material piece; careful recovery may yield two.\r\n",
			     ch);
	if (skill >= 50 && has_affect(item))
		send_to_char("  Its magical traits may also yield an essence.\r\n", ch);
	if (skill >= 75 && crafting_scientific_tools_prevent_breakage() &&
	    vnum_in_inv(ch, crafting_scientific_tools_vnum()) < 1)
		send_to_char(
			"  Warning: without Lantan Scientific Tools, a failed Salvage skill roll can destroy it.\r\n",
			ch);
	if (skill >= 100)
		send_to_char(
			crafting_recipe_target_is_available(item) ?
				"  It is eligible for a possible player-recipe discovery.\r\n" :
				"  It cannot yield a player recipe under the current crafting policy.\r\n",
			ch);
}

void do_salvage(P_char ch, char *argument, int /*cmd*/)
{
	static bool DEBUG = TRUE;
	P_obj item, salvaged, recipe;
	char first_arg[MAX_INPUT_LENGTH], buf1[MAX_STRING_LENGTH];
	char debugBuf[MAX_STRING_LENGTH];
	int itemvnum, itemval, lowest, matvnum;
	int newcost, reciperoll;
	int scitools = vnum_in_inv(ch, crafting_scientific_tools_vnum());
	int playerroll;
	int essence_luck;
	float modifier;

	one_argument(argument, first_arg);

	if (IS_TRUSTED(ch) && !strcmp(first_arg, "debug"))
	{
		DEBUG = !DEBUG;
		debug("do_salvage: DEBUG turned %s.", DEBUG ? "ON" : "OFF");
		return;
	}

	if (GET_CHAR_SKILL(ch, SKILL_SALVAGE) < 1)
	{
		send_to_char(
			"Only &+ycrafters&n have the necessary &+yskill&n to break down &+Witems&n.\n",
			ch);
		return;
	}

	if (!(item = get_obj_in_list_vis(ch, first_arg, ch->carrying)))
	{
		act("What would you like to salvage?", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	if (IS_SET(item->extra_flags, ITEM_NODROP))
	{
		act("But your $q is so pretty.", FALSE, ch, item, 0, TO_CHAR);
		return;
	}

	itemvnum = OBJ_VNUM(item);

	// Handle salvage materials
	if ((itemvnum >= LOWEST_MAT_VNUM) && (itemvnum <= HIGHEST_MAT_VNUM))
	{
		lowest = get_matstart(item);
		if (itemvnum == lowest)
		{
			send_to_char(
				"Not possible! That &+ymaterial&n is already of the &+Llowest&n quality.\r\n",
				ch);
			return;
		}
		if (lowest <= 0)
		{
			send_to_char(
				"Could not figure out what this is made out of !?  Can bug it if you want.\n\r",
				ch);
			logit(LOG_DEBUG, "Couldn't get start material for object: '%s' %d.",
			      item->short_description, itemvnum);
			return;
		}

		grant_salvage_item(ch, read_object(--itemvnum, VIRTUAL));
		grant_salvage_item(ch, read_object(itemvnum, VIRTUAL));
		act("$n breaks down their $p into its &+ylesser&n material...", TRUE, ch, item, 0,
		    TO_ROOM);
		act("You break down your $p into its &+ylesser &+Ymaterial&n...", FALSE, ch, item,
		    0, TO_CHAR);
		obj_from_char(item);
		extract_obj(item);
		return;
	}

	if (!is_salvageable(item))
	{
		act("That item cannot be &+ysalvaged&n.", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	if (scitools < 1)
	{
		if (crafting_scientific_tools_prevent_breakage())
			act("&+yTip: Lantan Scientific Tools are consumed during salvage. They prevent failed-skill breakage and improve recipe discovery. You do not have a set.&n",
			    FALSE, ch, item, 0, TO_CHAR);
		else
			act("&+yTip: Lantan Scientific Tools are consumed during salvage and improve recipe discovery. You do not have a set.&n",
			    FALSE, ch, item, 0, TO_CHAR);
	}

	if (GET_CHAR_SKILL(ch, SKILL_SALVAGE) < number(1, 105) &&
	    (!crafting_scientific_tools_prevent_breakage() || scitools < 1))
	{
		act("&+LYou attempt to break down your $p&+L, but end up &+Rbreaking &+Lit in the process. &+yLantan Scientific Tools would prevent this failed-skill breakage.&n",
		    FALSE, ch, item, 0, TO_CHAR);
		act("$n attempts to salvage their $p, but clumsily destroys it.", TRUE, ch, item, 0,
		    TO_ROOM);
		extract_obj(item);
		notch_skill(ch, SKILL_SALVAGE, 10);
		return;
	}

	act("$n begins to tear down their $p into its core components...", TRUE, ch, item, 0,
	    TO_ROOM);
	act("You begin breaking down your $p into its &+yraw &+Ymaterials&n...", FALSE, ch, item, 0,
	    TO_CHAR);

	itemval = itemvalue(item);

	if ((itemval <= 5) && (number(1, 1000) > GET_C_LUK(ch)))
	{
		send_to_char(
			"The &+ypoor &nquality and &+Lcraftsmanship&n of the item yield to your force, &+Rbreaking&n the item into unusable bits.\r\n",
			ch);
		extract_obj(item);
		return;
	}

	// Find base material via item's material.
	// Note: these are in order of material type (MAT_NONSUBSTANTIAL = 1, MAT_FLESH = 2, ... )
	//   Also, the matvnums are in order of rarity of material (value from cheapest to most expensive)
	//     ie. MAT_NONSUBSTANTIAL = most expensive @ 400205, MAT_FEATHER is cheapest @ 400000.
	switch (item->material)
	{
	case MAT_NONSUBSTANTIAL:
		matvnum = 400205;
		break;
	case MAT_FLESH:
		matvnum = 400005;
		break;
	case MAT_CLOTH:
		matvnum = 400015;
		break;
	case MAT_BARK:
		matvnum = 400035;
		break;
	case MAT_SOFTWOOD:
		matvnum = 400040;
		break;
	case MAT_HARDWOOD:
		matvnum = 400050;
		break;
	// case MAT_SILICON:
	// matvnum = 67283;
	// break;
	case MAT_CRYSTAL:
		matvnum = 400090;
		break;
	// case MAT_CERAMIC:
	// matvnum = 67283;
	// break;
	case MAT_BONE:
		matvnum = 400065;
		break;
	case MAT_STONE:
		matvnum = 400095;
		break;
	case MAT_HIDE:
		matvnum = 400030;
		break;
	case MAT_LEATHER:
		matvnum = 400045;
		break;
	case MAT_CURED_LEATHER:
		matvnum = 400060;
		break;
	case MAT_IRON:
		matvnum = 400110;
		break;
	case MAT_STEEL:
		matvnum = 400120;
		break;
	case MAT_BRASS:
		matvnum = 400125;
		break;
	case MAT_MITHRIL:
		matvnum = 400185;
		break;
	case MAT_ADAMANTIUM:
		matvnum = 400195;
		break;
	case MAT_BRONZE:
		matvnum = 400130;
		break;
	case MAT_COPPER:
		matvnum = 400135;
		break;
	case MAT_SILVER:
		matvnum = 400140;
		break;
	case MAT_ELECTRUM:
		matvnum = 400145;
		break;
	case MAT_GOLD:
		matvnum = 400150;
		break;
	case MAT_PLATINUM:
		matvnum = 400180;
		break;
	case MAT_GEM:
		matvnum = 400155;
		break;
	case MAT_DIAMOND:
		matvnum = 400190;
		break;
	// case MAT_LEAVES:
	// matvnum = 67283;
	// break;
	case MAT_RUBY:
		matvnum = 400165;
		break;
	case MAT_EMERALD:
		matvnum = 400160;
		break;
	case MAT_SAPPHIRE:
		matvnum = 400170;
		break;
	case MAT_IVORY:
		matvnum = 400070;
		break;
	case MAT_DRAGONSCALE:
		matvnum = 400200;
		break;
	case MAT_OBSIDIAN:
		matvnum = 400175;
		break;
	case MAT_GRANITE:
		matvnum = 400100;
		break;
	case MAT_MARBLE:
		matvnum = 400105;
		break;
	// case MAT_LIMESTONE:
	// matvnum = 67283;
	// break;
	case MAT_BAMBOO:
		matvnum = 400055;
		break;
	case MAT_REEDS:
		matvnum = 400010;
		break;
	case MAT_HEMP:
		matvnum = 400020;
		break;
	case MAT_GLASSTEEL:
		matvnum = 400115;
		break;
	case MAT_CHITINOUS:
		matvnum = 400080;
		break;
	case MAT_REPTILESCALE:
		matvnum = 400085;
		break;
	case MAT_RUBBER:
		matvnum = 400025;
		break;
	case MAT_FEATHER:
		matvnum = 400000;
		break;
	case MAT_PEARL:
		matvnum = 400075;
		break;
	default:
		act("&+wYou cant seem to find anything worth &+ysalvaging&+w on that item.&n",
		    FALSE, ch, 0, 0, TO_CHAR);
		return;
		break;
	}

	// Grant Rewards based on ival of item.
	// Note: This only works because all of the materials of the same type are in sequential order.
	//   ie. Feathers are 400000, 400001, 400002, 400003, 400004 and Hemp is 400020, -021, -022, -023, -024.
	if (itemval <= 5)
	{
		act("&+wYou were able to salvage a rather &+rpoor&n material from your item...",
		    FALSE, ch, 0, 0, TO_CHAR);
	}
	else if (itemval <= 10)
	{
		matvnum++;
		act("&+wYour focused efforts allow you to salvage a &+ycommon&n material from your item...",
		    FALSE, ch, 0, 0, TO_CHAR);
	}
	else if (itemval <= 15)
	{
		matvnum += 2;
		act("&+wYou study your item as you break it down, and come away with a rather &+Yuncommon &nmaterial.",
		    FALSE, ch, 0, 0, TO_CHAR);
	}
	else if (itemval <= 20)
	{
		matvnum += 3;
		act("&+wYou make quick work of your item, salvaging a precious &+crare &nmaterial from it...",
		    FALSE, ch, 0, 0, TO_CHAR);
	}
	// craftsmanship > 20
	else
	{
		matvnum += 4;
		act("&+LUsing your ma&+wst&+Wer&+wfu&+Ll &+Wskill&+L, you delicately break apart your item, salvaging a quite &+Munique &+Lmaterial from it...",
		    FALSE, ch, 0, 0, TO_CHAR);
	}

	// A rare Luck-based essence; default multipliers preserve the historical rolls.
	essence_luck = (int)(GET_C_LUK(ch) * crafting_salvage_essence_luck_multiplier());
	if (number(60, 400) < essence_luck && number(70, 400) < essence_luck &&
	    number(80, 500) < essence_luck &&
	    number(1, 1000000) <= (int)(1000000.0 * crafting_salvage_essence_chance_multiplier()))
	{
		grant_salvage_item(ch, read_object(MAG_ESSENCE_VNUM, VIRTUAL));
		send_to_char(
			"...as you work, a small &+Mm&+Ya&+Mg&+Yi&+Mc&+Ya&+Ml&n object gently separates from your item!\r\n",
			ch);
	}

	// Any affect which makes a recipe magical also yields its guaranteed essence.
	if (has_affect(item))
	{
		grant_salvage_item(ch, read_object(MAG_ESSENCE_VNUM, VIRTUAL));
		send_to_char(
			"...as you work, a small &+Mm&+Ya&+Mg&+Yi&+Mc&+Ya&+Ml&n object gently separates from your item!\r\n",
			ch);
	}

	// Dynamic pricing - Drannak 3/21/2013
	// between 1 gold, 9 silver and 2 gold 2 silver starting point
	newcost = number(190, 220);
	// Since the vnum's are sequential, the greatest rarity gets a 1.3 modifier, lowest gets 100% of value.
	// To do this, we want 400000 to map to 1, and 400209 to map to 1.3:
	//    modifier = ((OBJ_VNUM(salvaged) - LOWEST_MAT_VNUM) * 0.3) / (HIGHEST_MAT_VNUM - LOWEST_MAT_VNUM) + 1;
	// However, 200 * 1.3 = only 2 gold, 6 silver.  We want this to be much more profitable, so, instead of
	//   mapping to 1.3, we want to map to 13 -> 2 plat, 6 gold; we set the multiplier to 13 - 1 = 12.
	modifier =
		((matvnum - LOWEST_MAT_VNUM) * 12.0) / (float)(HIGHEST_MAT_VNUM - LOWEST_MAT_VNUM) +
		1.0;
	if (DEBUG)
		snprintf(debugBuf, MAX_STRING_LENGTH,
			 "do_salvage: Newcost(initial): %d, Modifier: %.3f", newcost, modifier);
	newcost = (int)((float)newcost * modifier);
	if (DEBUG)
		checked_snprintf(debugBuf + strlen(debugBuf), MAX_STRING_LENGTH - strlen(debugBuf),
				 ", Newcost(mod): %d", newcost);
	newcost = (newcost * GET_LEVEL(ch)) / 56;
	if (DEBUG)
		checked_snprintf(debugBuf + strlen(debugBuf), MAX_STRING_LENGTH - strlen(debugBuf),
				 ", Newcost(lvl): %d", newcost);
	newcost = (newcost * GET_CHAR_SKILL(ch, SKILL_SALVAGE) / 100);
	if (DEBUG)
		checked_snprintf(debugBuf + strlen(debugBuf), MAX_STRING_LENGTH - strlen(debugBuf),
				 ", Newcost(skill): %d", newcost);

	// 67% chance to get 2 salvaged materials.
	if (!number(0, 2))
	{
		act("&+w...and at least you &+ysalvaged&n a decent amount.", FALSE, ch, 0, 0,
		    TO_CHAR);
		salvaged = read_object(matvnum, VIRTUAL);
		if (number(80, 140) < GET_C_LUK(ch))
		{
			send_to_char(
				"&+mYou &+Ygently&+m break the first &+Mmaterial &+mfree, preserving its natural form.&n\r\n",
				ch);
			salvaged->cost = (13 * newcost) / 10;
		}
		else
			salvaged->cost = newcost;

		const bool first_granted = grant_salvage_item(ch, salvaged);

		if (DEBUG && first_granted)
		{
			checked_snprintf(debugBuf + strlen(debugBuf),
					 MAX_STRING_LENGTH - strlen(debugBuf), ", Final cost: %d.",
					 salvaged->cost);
			debug("%s", debugBuf);
		}

		salvaged = read_object(matvnum, VIRTUAL);
		// Don't bother recalculating, just add a bit of randomness (1 copper variance per 1 gold value).
		newcost += number(-newcost / 100, newcost / 100);

		if (number(80, 140) < GET_C_LUK(ch))
		{
			send_to_char(
				"&+mYou &+Ygently&+m break the second &+Mmaterial &+mfree, preserving its natural form.&n\r\n",
				ch);
			salvaged->cost = (13 * newcost) / 10;
		}
		else
			salvaged->cost = newcost;

		grant_salvage_item(ch, salvaged);
	}
	else
	{
		act("&+w...and you only came up with a single piece of &+ymaterial&n.", FALSE, ch,
		    0, 0, TO_CHAR);
		salvaged = read_object(matvnum, VIRTUAL);

		if (number(80, 140) < GET_C_LUK(ch))
		{
			send_to_char(
				"&+mYou &+Ygently&+m break the &+Mmaterial &+mfree, preserving its natural form.&n\r\n",
				ch);
			salvaged->cost = (13 * newcost) / 10;
		}
		else
			salvaged->cost = newcost;

		const bool granted = grant_salvage_item(ch, salvaged);

		if (DEBUG && granted)
		{
			checked_snprintf(debugBuf + strlen(debugBuf),
					 MAX_STRING_LENGTH - strlen(debugBuf), ", Final cost: %d.",
					 salvaged->cost);
			debug("%s", debugBuf);
		}
	}

	notch_skill(ch, SKILL_SALVAGE, 4);

	reciperoll = number(1, 10000);

	if (itemval <= 5)
	{
		reciperoll /= 3;
	}
	else if (itemval <= 10)
	{
		reciperoll /= 2;
	}
	else if (itemval <= 15)
	{
		reciperoll = (reciperoll * 2) / 3;
	}
	else if (itemval >= 100)
	{
		// 100k -> autofail (playerroll will be around 500 or so at max).
		reciperoll = 100000;
	}

	playerroll = GET_C_LUK(ch) + GET_LEVEL(ch) * 2 + GET_CHAR_SKILL(ch, SKILL_SALVAGE);
	if (scitools > 0)
	{
		if (crafting_scientific_tools_prevent_breakage())
			send_to_char(
				"&+yYou consume a set of &+cLantan Scientific Tools&+y: they protect against failed-skill breakage and improve your recipe discovery chance.\r\n",
				ch);
		else
			send_to_char(
				"&+yYou consume a set of &+cLantan Scientific Tools&+y to improve your recipe discovery chance.\r\n",
				ch);
		reciperoll /= crafting_scientific_tools_recipe_roll_divisor();
		playerroll *= crafting_scientific_tools_recipe_player_multiplier();
		vnum_from_inv(ch, crafting_scientific_tools_vnum(), 1);
	}

	/*** CREATE RECIPE ***/
	if (reciperoll < playerroll)
	{
		if (DEBUG)
			debug("do_salvage: player: '%s' - reciperoll: %d, playerroll: %d, scitools: %d.",
			      J_NAME(ch), reciperoll, playerroll, scitools);

		if (itemvnum == VOBJ_RANDOM_ARMOR || itemvnum == VOBJ_RANDOM_THRUSTED ||
		    itemvnum == VOBJ_RANDOM_WEAPON)
		{
			debug("do_salvage: player: '%s' Not creating recipe for random item %d.",
			      J_NAME(ch), itemvnum);
			if (scitools)
			{
				act("With your tools, you discover that $p can not be manufactured.",
				    FALSE, ch, item, 0, TO_CHAR);
			}
		}
		else if (crafting_recipe_target_is_available(item))
		{
			recipe = read_object(SALVAGE_RECIPE_VNUM, VIRTUAL);

			SET_BIT(recipe->value[6], itemvnum);
			snprintf(buf1, MAX_STRING_LENGTH, "%s %s", recipe->short_description,
				 item->short_description);
			recipe->short_description = str_dup(buf1);
			recipe->str_mask |= STRUNG_DESC2;
			crafting_configure_recipe_scroll(recipe, item);

			const bool recipe_granted = grant_salvage_item(ch, recipe);
			if (DEBUG && recipe_granted)
				debug("do_salvage: %s created '%s'.", J_NAME(ch),
				      recipe->short_description);
			if (recipe_granted)
				act("As $n breaks down their $p, they are suddenly &+Yenlightened&n!\n"
				    "$n quickly grabs a quill and &+yvellum paper&n and starts to write down the &+Cdetailed&n\n"
				    "intricacies surrounding $p.\r\n",
				    FALSE, ch, item, 0, TO_ROOM);
			if (recipe_granted)
				act("As you break down your $p, you are suddenly &+Yenlightened&n!\n"
				    "You quickly grab a quill and &+yvellum paper&n and start to write down the &+Cdetailed&n\n"
				    "intricacies surrounding $p.\r\n",
				    FALSE, ch, item, 0, TO_CHAR);
			if (recipe_granted)
			{
				act("$n has created $p!\r\n", FALSE, ch, recipe, 0, TO_ROOM);
				act("You have created $p!\r\n", FALSE, ch, recipe, 0, TO_CHAR);
			}
		}
		else if (scitools)
		{
			act("With your tools, you determine that $p cannot be manufactured under the current crafting limits.",
			    FALSE, ch, item, 0, TO_CHAR);
		}
	}
	/*** END CREATE RECIPE ***/

	if (DEBUG)
		debug("do_salvage: player: '%s&n' just salvaged '%s&n' (%d) at [%d]!", J_NAME(ch),
		      OBJ_SHORT(item), itemvnum, ROOM_VNUM(ch->in_room));
	extract_obj(item);
	char_light(ch);
	room_light(ch->in_room, REAL);
}
