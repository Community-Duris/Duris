/* Staff newbie assistance and pet grant commands. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/files.h"
#include "magic/spells.h"
#include "net/comm.h"
#include "world/rested.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void newb_spellup(P_char ch, P_char victim)
{
	wizlog(58, "(%s) newb buffed: (%s).", GET_NAME(ch), GET_NAME(victim));
	logit(LOG_WIZ, "(%s) newb buffed: (%s).", GET_NAME(ch), GET_NAME(victim));

	// Insert cool ascii shit here
	if (isname("Jexni", ch->player.name))
	{
		send_to_char(file_to_string("lib/creation/hypnotoad"), victim);
	}

	// And lets spellup the noob!  A little good will goes a long way.
	spell_bless(61, ch, 0, SPELL_TYPE_SPELL, victim, 0);
	spell_spirit_armor(61, victim, 0, SPELL_TYPE_SPELL, victim, 0);
	spell_barkskin(61, victim, 0, SPELL_TYPE_SPELL, victim, 0);
	spell_enhance_armor(61, victim, 0, SPELL_TYPE_SPELL, victim, 0);
	spell_stone_skin(61, ch, 0, SPELL_TYPE_SPELL, victim, 0);
	spell_fly(61, ch, 0, SPELL_TYPE_SPELL, victim, 0);
	spell_haste(61, ch, 0, SPELL_TYPE_SPELL, victim, 0);
	spell_strength(61, ch, 0, SPELL_TYPE_SPELL, victim, 0);
	spell_agility(61, ch, 0, SPELL_TYPE_SPELL, victim, 0);
	spell_dexterity(61, ch, 0, SPELL_TYPE_SPELL, victim, 0);
	spell_accel_healing(61, ch, 0, SPELL_TYPE_SPELL, victim, 0);
	grant_staff_rested_bonus(ch, victim);

	send_to_char("\nEnjoy your blessings.\n", victim);
}

void do_newb_spellup_all(P_char ch, char *arg, int /*cmd*/)
{
	community_spellup_command(ch, arg);
}

void do_newb_spellup(P_char ch, char *arg, int /*cmd*/)
{
	char buf[MAX_STRING_LENGTH];
	P_char victim;

	one_argument(arg, buf);

	if (!*arg)
	{
		send_to_char("&+WFormat: &+wnewbsu <player_name>&+W.&n\n", ch);
		return;
	}

	if (!(victim = get_char_vis(ch, buf)))
	{
		send_to_char("Nobody with that name.\n", ch);
		return;
	}

	if (IS_NPC(victim))
	{
		send_to_char("Only works on players.\n\r", ch);
		return;
	}

	newb_spellup(ch, victim);
	send_to_char("Done.\n", ch);
}

void do_givepet(P_char ch, char *arg, int /*cmd*/)
{
	char msg[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH], pet[MAX_STRING_LENGTH];
	P_char mob = NULL;

	arg = one_argument(arg, buf);
	arg = one_argument(arg, pet);

	if (*buf && *pet)
	{
		P_char victim = get_char_room_vis(ch, buf);

		if (!victim)
		{
			send_to_char("Nobody with that name here.\n", ch);
			return;
		}
		// debug("found victim");
		if (isname("dog", pet))
		{
			mob = read_mobile(28124, VIRTUAL);
			snprintf(msg, MAX_STRING_LENGTH, "$N runs up to you and barks playfully.");
		}
		else if (isname("cat", pet))
		{
			mob = read_mobile(74132, VIRTUAL);
			snprintf(msg, MAX_STRING_LENGTH,
				 "$N walks up to you and rubs up against your leg.");
		}
		else if (isdigit(*pet))
		{
			msg[0] = '\0';
			mob = read_mobile(atoi(pet), VIRTUAL);
			if (!mob)
			{
				send_to_char("Let's try something real!\n", ch);
				return;
			}
			wizlog(56, "%s has loaded pet %s(Level: %d) for %s.", GET_NAME(ch),
			       mob->player.short_descr, GET_LEVEL(mob), GET_NAME(victim));
			logit(LOG_WIZ, "(%s) has loaded pet (%s)(Level: %d) for (%s).",
			      GET_NAME(ch), mob->player.short_descr, GET_LEVEL(mob),
			      GET_NAME(victim));
		}
		else
		{
			send_to_char("Valid pets are 'dog', 'cat', or vnum of mob!\n", ch);
			return;
		}

		if (mob)
		{
			SET_BIT(mob->specials.act, ACT_SENTINEL);
			mob->only.npc->aggro_flags = 0;
			char_to_room(mob, victim->in_room, 0);
			setup_pet(mob, victim, -1, PET_NOCASH | PET_NOAGGRO);
			if (*msg)
				act(msg, TRUE, victim, 0, mob, TO_CHAR);
			add_follower(mob, victim);
		}
		else
		{
			debug("givepet(): Error loading mob.");
		}
	}
	else
	{
		send_to_char("Syntax: givepet playername [dog|cat|mob_vnum]", ch);
	}

	return;
}

// This function toggles a player's newbie status
void do_newbie(P_char ch, char *argument, int /*cmd*/)
{
	P_char victim;
	char buf[MAX_INPUT_LENGTH];

	if (IS_NPC(ch))
	{
		return;
	}

	one_argument(argument, buf);

	if (!*buf)
	{
		send_to_char("Who's newbie status do you wish to toggle?\n", ch);
		return;
	}

	if (!(victim = get_char_vis(ch, buf)))
	{
		send_to_char("No-one by that name around.\n", ch);
		return;
	}

	// Can set self for testing.
	if (IS_TRUSTED(victim) && victim != ch)
	{
		send_to_char("Haha. Very funny.\n", ch);
		return;
	}

	if (GET_LEVEL(victim) > 49 && victim != ch)
	{
		send_to_char("Aren't they a little high level to be considered a newbie?\n", ch);
		return;
	}

	(void)PLR2_TOG_CHK(victim, PLR2_NEWBIE);

	if (IS_SET(PLR2_FLAGS(victim), PLR2_NEWBIE))
	{
		send_to_char("You turned on their newbie status.\n", ch);
		SET_BIT(victim->specials.act2, PLR2_NCHAT);
	}
	else
	{
		send_to_char("You turned off their newbie status.\n", ch);
		REMOVE_BIT(victim->specials.act2, PLR2_NCHAT);
	}

	if (!do_save_silent(victim, 1))
		logit(LOG_WIZ, "Failed to save %s after wizard flag change.", GET_NAME(victim));

	logit(LOG_WIZ, "%s toggled %s's newbie status.", ch->player.name, victim->player.name);
}

// This function toggles a player's newbie helper status
void do_make_guide(P_char ch, char *argument, int /*cmd*/)
{
	P_char victim;
	char buf[MAX_INPUT_LENGTH];

	if (IS_NPC(ch))
		return;

	one_argument(argument, buf);

	if (!*buf)
	{
		send_to_char("Who's newbie helper status do you wish to toggle?\n", ch);
		return;
	}

	if (!(victim = get_char_vis(ch, buf)))
	{
		send_to_char("No-one by that name around.\n", ch);
		return;
	}

	(void)PLR2_TOG_CHK(victim, PLR2_NEWBIE_GUIDE);

	if (IS_SET(PLR2_FLAGS(victim), PLR2_NEWBIE_GUIDE))
	{
		send_to_char("You made them a newbie helper.\n", ch);
		send_to_char("You are now an official &+GGuide&N!\n", victim);
		SET_BIT(victim->specials.act2, PLR2_NCHAT);
	}
	else
	{
		send_to_char("You turned off their newbie helper status.\n", ch);
		send_to_char("You are no longer an official &+GGuide&N.\n", victim);
		REMOVE_BIT(victim->specials.act2, PLR2_NCHAT);
	}

	if (!do_save_silent(victim, 1))
		logit(LOG_WIZ, "Failed to save %s after wizard flag change.", GET_NAME(victim));

	logit(LOG_WIZ, "%s toggled %s's newbie helper status.", ch->player.name,
	      victim->player.name);
}
