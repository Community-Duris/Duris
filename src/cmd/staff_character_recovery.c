/* Staff restore and affect-cleanup commands. */

#include "core/prototypes.h"
#include "telemetry/telemetry_runtime.h"
#include "world/character_maintenance.h"
#include "core/structs.h"
#include "core/defines.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/files.h"
#include "cmd/interp.h"
#include "magic/spells.h"
#include "net/comm.h"
#include "sql/sql.h"
#include <stdio.h>
#include <string.h>

extern P_desc descriptor_list;
extern char *spells[];
extern int spl_table[TOTALLVLS][MAX_CIRCLE];
static void do_reboot_restore(P_char ch, P_char victim)
{
	if (!IS_TRUSTED(ch))
		return;

	poison_common_remove(victim);

	if (affected_by_spell(victim, SPELL_CURSE))
		affect_from_char(victim, SPELL_CURSE);

	if (affected_by_spell(victim, SPELL_MALISON))
		affect_from_char(victim, SPELL_MALISON);

	if (affected_by_spell(victim, SPELL_WITHER))
		affect_from_char(victim, SPELL_WITHER);

	if (affected_by_spell(victim, SPELL_BLOODTOSTONE))
		affect_from_char(victim, SPELL_BLOODTOSTONE);

	if (affected_by_spell(victim, SPELL_SHREWTAMENESS))
		affect_from_char(victim, SPELL_SHREWTAMENESS);

	if (affected_by_spell(victim, SPELL_MOUSESTRENGTH))
		affect_from_char(victim, SPELL_MOUSESTRENGTH);

	if (affected_by_spell(victim, SPELL_MOLEVISION))
		affect_from_char(victim, SPELL_MOLEVISION);

	if (affected_by_spell(victim, SPELL_SNAILSPEED))
		affect_from_char(victim, SPELL_SNAILSPEED);

	if (affected_by_spell(victim, SPELL_FEEBLEMIND))
		affect_from_char(victim, SPELL_FEEBLEMIND);

	if (affected_by_spell(victim, SPELL_SLOW))
		affect_from_char(victim, SPELL_SLOW);

	if (IS_AFFECTED(victim, AFF_BLIND))
	{
		affect_from_char(victim, SPELL_BLINDNESS);
		REMOVE_BIT(victim->specials.affected_by, AFF_BLIND);
		telemetry_runtime_game_control_changed(victim);
	}

	if (IS_AFFECTED4(victim, AFF4_CARRY_PLAGUE))
		REMOVE_BIT(victim->specials.affected_by4, AFF4_CARRY_PLAGUE);

	if (affected_by_spell(victim, SPELL_DISEASE) || affected_by_spell(victim, SPELL_PLAGUE) ||
	    affected_by_spell(victim, SPELL_BMANTLE) ||
	    affected_by_spell(victim, SPELL_FLAMESTRIKE))
	{
		affect_from_char(victim, SPELL_DISEASE);
		affect_from_char(victim, SPELL_PLAGUE);
		affect_from_char(victim, SPELL_BMANTLE);
		affect_from_char(victim, SPELL_FLAMESTRIKE);
	}

	if (affected_by_spell(victim, TAG_ARMLOCK))
		affect_from_char(victim, TAG_ARMLOCK);

	if (affected_by_spell(victim, TAG_LEGLOCK))
		affect_from_char(victim, TAG_LEGLOCK);

	if (affected_by_spell(victim, SPELL_ENERGY_DRAIN))
		affect_from_char(victim, SPELL_ENERGY_DRAIN);

	if (affected_by_spell(victim, SPELL_SLEEP))
		affect_from_char(victim, SPELL_SLEEP);

	if (affected_by_spell(victim, SONG_SLEEP))
		affect_from_char(victim, SONG_SLEEP);

	if (affected_by_spell(victim, SPELL_RAY_OF_ENFEEBLEMENT))
		affect_from_char(victim, SPELL_RAY_OF_ENFEEBLEMENT);

	send_to_char("&+WAll your maladies are washed away...\r\n", victim);
}

void do_affectpurge(P_char ch, char *argument, int /*cmd*/)
{
	P_char victim;
	char arg1[MAX_STRING_LENGTH], rest[MAX_STRING_LENGTH];
	int spell = 0, qend;
	struct affected_type *af, *next_af;

	if (!ch || !IS_TRUSTED(ch))
	{
		return;
	}

	half_chop(argument, arg1, rest);
	if (!*arg1)
	{
		send_to_char("Who do you wish to unaffect?\n", ch);
		return;
	}

	victim = get_char(arg1);

	if (!victim)
	{
		send_to_char("Usage: affectpurge <name> <all | '<spell>'>\n", ch);
		return;
	}

	if (GET_LEVEL(victim) > GET_LEVEL(ch))
	{
		send_to_char("You may not unaffect entities more superior than yourself.\n", ch);
		return;
	}

	if (!*rest)
	{
		send_to_char("Usage: affectpurge <name> <all | '<spell>'>\n", ch);
		return;
	}

	if (!strcmp(rest, "all"))
	{
		spell = -1;
		logit(LOG_WIZ, "%s purged affect ALL from %s", GET_NAME(ch), GET_NAME(victim));
		sql_log(ch, WIZLOG, "Purged all affects from %s", GET_NAME(victim));
	}
	else
	{
		if (*rest != '\'')
		{
			send_to_char("You must use single quotes around the affect\n", ch);
			return;
		}
		for (qend = 1; *(rest + qend) && (*(rest + qend) != '\''); qend++)
			*(rest + qend) = LOWER(*(rest + qend));

		if (*(rest + qend) != '\'')
		{
			send_to_char("You must use single quotes around the affect\n", ch);
			return;
		}
		spell = old_search_block(rest, 1, ((uint)(MAX(0, qend - 1))), (const char **)spells,
					 0);
		if (spell != -1)
			spell--;
		if (spell == -1)
		{
			send_to_char("There is no such affect.\n", ch);
			return;
		}
		logit(LOG_WIZ, "%s purged affect %s from %s", GET_NAME(ch), rest, GET_NAME(victim));
		sql_log(ch, WIZLOG, "Purged affect %s from %s", rest, GET_NAME(victim));
	}
	if (victim && spell != 0)
	{
		act("&+W$n&+W waves $s mighty hand over your body...", FALSE, ch, 0, victim,
		    TO_VICT);
		act("&+WYou waves your mighty hand over $N&+W's body...", FALSE, ch, 0, victim,
		    TO_CHAR);
		act("&+W$n&+W waves $s mighty hand over $N&+W's body...", FALSE, ch, 0, victim,
		    TO_NOTVICT);
		for (af = victim->affected; af; af = next_af)
		{
			next_af = af->next;
			if (spell == -1 || spell == af->type)
			{
				affect_remove(victim, af);
				send_to_char("Affect removed.\n", ch);
			}
		}
		update_pos(victim);
	}
}

void do_restore(P_char ch, char *argument, int cmd)
{
	P_char victim;
	P_desc d;
	P_obj obj;
	int i = 0, j;
	char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];

	if (IS_NPC(ch))
		return;

	argument = one_argument(argument, arg1);
	one_argument(argument, arg2);

	if (!*arg1)
		send_to_char("Who do you wish to restore?\n", ch);
	else if (!str_cmp("all", arg1))
	{
		if (GET_LEVEL(ch) < FORGER && !god_check(ch->player.name))
		{
			send_to_char("Sorry, thou canst.\n", ch);

			return;
		}

		for (d = descriptor_list; d; d = d->next)
			if (!d->connected)
			{
				victim = d->character;
				if (affected_by_spell(victim, TAG_BUILDING))
					continue;
				balance_affects(victim);
				if (GET_HIT(victim) < GET_MAX_HIT(victim))
					GET_HIT(victim) = GET_MAX_HIT(victim);
				GET_VITALITY(victim) = GET_MAX_VITALITY(victim);
				if (GET_CLASS(victim, CLASS_PSIONICIST) ||
				    IS_RACEWAR_UNDEAD(victim) || GET_CLASS(ch, CLASS_MINDFLAYER))
					GET_MANA(victim) = GET_MAX_MANA(victim);
				if (USES_SPELL_SLOTS(victim))
				{
					victim->specials.undead_spell_slots[0] = 0;
					for (j = 1; j <= MAX_CIRCLE; j++)
						victim->specials.undead_spell_slots[j] =
							max_spells_in_circle(victim, j);
				}
				GET_COND(victim, FULL) = IS_TRUSTED(victim) ? -1 : 24;
				GET_COND(victim, THIRST) = IS_TRUSTED(victim) ? -1 : 24;
				GET_COND(victim, DRUNK) = 0;
				if (GET_STAT(victim) < STAT_SLEEPING)
					SET_POS(victim, GET_POS(victim) + STAT_NORMAL);
				character_maintenance_changed(victim);

				send_to_char(
					"&+BA haze of magical energies fall from the heavens, engulfing all that you see.\n"
					"&+BAs they subside, you feel refreshed...&n\n",
					victim);
				if (ch != victim)
					do_reboot_restore(ch, victim);

				act("You have been fully restored by $N!", FALSE, victim, 0, ch,
				    TO_CHAR);

				if (isname("Kvark", ch->player.name))
					send_to_char(file_to_string("lib/creation/boom"), victim);
				if (isname("Zion", ch->player.name))
					send_to_char(file_to_string("lib/creation/hypnotoad"),
						     victim);
				if (isname("Venthix", ch->player.name))
					send_to_char(file_to_string("lib/creation/skullsword"),
						     victim);
				if (isname("Jexni", ch->player.name))
					send_to_char(file_to_string("lib/creation/hypnotoad"),
						     victim);
				if (isname("Gellz", ch->player.name))
					send_to_char(file_to_string("lib/creation/cookie"), victim);
			}
		send_to_char("Restoration of all players completed.\n", ch);
	}
	else if (!str_cmp("items", arg1))
	{
		if (GET_LEVEL(ch) < FORGER)
		{
			send_to_char("Whoops, you can't!\n", ch);
			return;
		}

		for (d = descriptor_list; d; d = d->next)
		{
			if (!d->connected)
			{
				victim = d->character;
				if (affected_by_spell(victim, TAG_BUILDING))
					continue;
				for (i = 0; i < MAX_WEAR; i++)
					if (victim->equipment[i])
						victim->equipment[i]->condition =
							100; // victim->equipment[i]->max_condition; wipe2011
				for (obj = victim->carrying; obj; obj = obj->next_content)
					obj->condition = 100; // obj->max_condition; wipe2011
				send_to_char(
					"&+gFrom out of nowhere, little gremlin-like creatures about 6 inches tall pop up.\n"
					"&+gThey grab all of your equipment, and fiddle with it before returning to you.\n"
					"&+gThey then vanish as quickly as they came.\n",
					victim);
			}
		}
	}
	else if (!(victim = get_char_vis(ch, arg1)))
	{
		send_to_char("No-one by that name in the world.\n", ch);
	}
	else if (!str_cmp("skills", arg2))
	{
		for (i = 0; i < MAX_SKILLS; i++)
		{
			victim->only.pc->skills[i].learned = victim->only.pc->skills[i].taught;
		}
	}
	else
	{
		if (affected_by_spell(victim, TAG_BUILDING))
		{
			send_to_char("Not allowed to restore an outpost!\n", ch);
			return;
		}

		balance_affects(victim);

		GET_MANA(victim) = GET_MAX_MANA(victim);
		GET_HIT(victim) = GET_MAX_HIT(victim);
		GET_VITALITY(victim) = GET_MAX_VITALITY(victim);

		/*
		 * Restore the NPCs complement of spells available for casting. - SKB
		 */

		if (IS_NPC(victim))
		{
			victim->specials.undead_spell_slots[0] = 0;

			for (i = 1; i <= MAX_CIRCLE; i++)
				victim->specials.undead_spell_slots[i] =
					spl_table[GET_LEVEL(victim)][i - 1];
		}
		else if (USES_SPELL_SLOTS(victim))
		{
			victim->specials.undead_spell_slots[0] = 0;

			for (i = 1; i <= MAX_CIRCLE; i++)
				victim->specials.undead_spell_slots[i] =
					max_spells_in_circle(victim, i);
		}
		if (GET_STAT(victim) < STAT_SLEEPING)
			SET_POS(victim, GET_POS(victim) + STAT_NORMAL);
		character_maintenance_changed(victim);

		if (IS_PC(ch))
		{
			GET_COND(victim, FULL) = IS_TRUSTED(victim) ? -1 : 24;
			GET_COND(victim, THIRST) = IS_TRUSTED(victim) ? -1 : 24;
			GET_COND(victim, DRUNK) = 0;
		}

		if ((cmd != -4) && !IS_TRUSTED(victim))
		{
			wizlog(GET_LEVEL(ch), "%s has restored %s", GET_NAME(ch), GET_NAME(victim));
			logit(LOG_WIZ, "%s has restored %s", GET_NAME(ch), GET_NAME(victim));
			sql_log(ch, WIZLOG, "Restored %s", GET_NAME(victim));
		}
		if (IS_TRUSTED(victim))
		{
			for (i = 0; i < MAX_SKILLS; i++)
			{
				victim->only.pc->skills[i].learned = 100;
				victim->only.pc->skills[i].taught = 100;
			}
			for (i = 0; i < MAX_TONGUE; i++)
				GET_LANGUAGE(victim, i) = 100;
		}

		update_pos(victim);
		if (ch != victim)
		{
			send_to_char("Done.\n", ch);
			act("You have been fully restored by $N!", FALSE, victim, 0, ch, TO_CHAR);
		}
		else
			send_to_char("Restored.\n", ch);
	}
}
