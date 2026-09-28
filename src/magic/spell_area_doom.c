#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include "magic/spell_words_of_power.h"

struct CDoomData
{
	int level;
	int waves;
	int area;
};

static void spell_single_cdoom_wave(int level, P_char ch, char *arg, int /*type*/, P_char victim,
				    P_obj /*obj*/)
{
	struct damage_messages messages = {
		"&+LYou send &+ma wave of &+Linsects &+mand &+Larachnids &+magainst $N!",
		"&+mA sea of &+Larachnids &+mand &+Linsects &+mconsume and overwhelm you!",
		"&+mA sea of &+Larachnids &+mand &+Linsects &+mconsume and overwhelm $N!",
		"&+mA sea of &+Linsects &+mand &+Larachnids &+mconsumed $N &+mcompletely "
		"leaving nothing but a few &+Wbones&+L..",
		"&+LYou suffer a terrible death as &+ma sea of &+Linsects &+mand &+Larachnids "
		"&+mdevours you &+Lalive...",
		"&+mA sea of &+Linsects &+mand &+Larachnids &+mconsumed $N &+mcompletely "
		"leaving nothing but a few &+Wbones&+L..",
		0
	};

	if (!IS_ALIVE(ch))
		return;

	int doomdam = 40 + level + number(0, 20);

	switch (world[victim->in_room].sector_type)
	{
	case SECT_WATER_SWIM:
	case SECT_WATER_NOSWIM:
	case SECT_NO_GROUND:
	case SECT_UNDRWLD_NOGROUND:
	case SECT_UNDERWATER:
	case SECT_FIREPLANE:
	case SECT_UNDERWATER_GR:
	case SECT_OCEAN:
	case SECT_UNDRWLD_LIQMITH:
	case SECT_LAVA:
		doomdam -= 20;
		break;
	case SECT_INSIDE:
	case SECT_UNDRWLD_INSIDE:
		doomdam -= 10;
		break;
	/*case SECT_CITY:
		case SECT_UNDRWLD_CITY:
		case SECT_ROAD:
		  doomdam += 10;
		  break;*/
	case SECT_HILLS:
	case SECT_MOUNTAIN:
	case SECT_UNDRWLD_MOUNTAIN:
	case SECT_FIELD:
	case SECT_DESERT:
	case SECT_UNDRWLD_WILD:
		doomdam += 10;
		break;
	case SECT_FOREST:
	case SECT_SWAMP:
	case SECT_UNDRWLD_SLIME:
	case SECT_UNDRWLD_MUSHROOM:
		doomdam += 20;
		break;
	default:
		break;
	}

	if (get_spell_from_room(&world[victim->in_room], SPELL_SUMMON_INSECTS))
		doomdam += 20;

	if (GET_SPEC(ch, CLASS_DRUID, SPEC_WOODLAND))
		doomdam += 20;

	if (IS_AFFECTED3(victim, AFF3_COLDSHIELD) || IS_AFFECTED2(victim, AFF2_FIRESHIELD) ||
	    IS_AFFECTED3(victim, AFF3_LIGHTNINGSHIELD))
	{
		doomdam = (int)(doomdam * 0.75);
	}

	if (arg) // area
	{
		if (IS_PC(ch) && IS_PC(victim))
			doomdam = doomdam * get_property("spell.area.damage.to.pc", 0.5);
	}
	else // single target, stays with target
	{
		doomdam = doomdam * 1.20;
	}

	doomdam = doomdam * get_property("spell.area.damage.factor.creepingDoom", 1.000);

	spell_damage(ch, victim, doomdam, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
}

static void event_cdoom(P_char ch, P_char victim, P_obj obj, void *data)
{
	CDoomData *cDoomData = (CDoomData *)data;

	if (!cDoomData->area)
	{
		if (!IS_ALIVE(victim) || victim->in_room <= 0)
		{
			cDoomData->waves = 0;
		}
	}

	if (cDoomData->waves == 0)
	{
		act("&+LThe sea of &+minsects &+Land &+marachnids &+Lfades away...", FALSE, ch, 0,
		    victim, TO_CHAR);
		if (!cDoomData->area)
		{
			act("&+LThe sea of &+minsects &+Land &+marachnids &+Lfades away...", FALSE,
			    ch, 0, victim, TO_VICT);
		}
		else
		{
			act("&+LThe sea of &+minsects &+Land &+marachnids &+Lfades away...", FALSE,
			    ch, 0, victim, TO_ROOM);
		}
		return;
	}
	else
		cDoomData->waves--;

	if (cDoomData->area)
	{
		act("&+LA wave of &+minsects &+Land &+marachnids &+Lcrawls about the area...",
		    FALSE, ch, 0, victim, TO_CHAR);
		act("&+LA wave of &+minsects &+Land &+marachnids &+Lcrawls about the area...",
		    FALSE, ch, 0, victim, TO_ROOM);
	}
	else
	{
		// act("&+LA wave of &+marachnids&+L crawls about $N...", FALSE, ch, 0, victim, TO_CHAR);
		// act("&+LA wave of &+marachnids&+L crawls about $N...", FALSE, victim, 0, victim, TO_ROOM);
		// act("&+LA wave of &+marachnids&+L crawls about you...", FALSE, ch, 0, victim, TO_VICT);
	}
	// if doom is single-target, replace with direct call to spell_single_cdoom_wave
	if (cDoomData->area)
	{
		cast_as_damage_area(ch, spell_single_cdoom_wave, cDoomData->level, victim,
				    get_property("spell.area.minChance.creepingDoom", 50),
				    get_property("spell.area.chanceStep.creepingDoom", 20));
		add_event(event_cdoom, PULSE_VIOLENCE, ch, 0, NULL, 0, cDoomData,
			  sizeof(CDoomData));
	}
	else
	{
		spell_single_cdoom_wave(cDoomData->level, ch, 0, 0, victim, obj);
		if (IS_ALIVE(victim))
		{
			add_event(event_cdoom, PULSE_VIOLENCE, ch, victim, NULL, 0, cDoomData,
				  sizeof(CDoomData));
		}
	}
}

static bool has_scheduled_area_doom(P_char ch)
{
	P_nevent e;

	LOOP_EVENTS_CH(e, ch->nevents)
	{
		if (e->func == event_cdoom && ((CDoomData *)(e->data))->area)
		{
			return TRUE;
		}
	}

	return FALSE;
}

void spell_cdoom(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim, P_obj /*obj*/)
{
	if (!IS_ALIVE(ch))
	{
		return;
	}

	// If ch already has an AREA doom going.. fail.
	if (has_scheduled_area_doom(ch) && !victim)
	{
		send_to_char("You are already controlling all the bugs in the area!\n", ch);
		return;
	}

	if (victim)
	{
		if (!is_char_in_room(victim, ch->in_room))
		{
			send_to_char("Your victim is no longer here.\r\n", ch);
			return;
		}
		if (!CAN_SEE(ch, victim))
		{
			send_to_char("You cannot see your victim.\r\n", ch);
			return;
		}
	}

	CDoomData cDoomData;
	cDoomData.waves = number(4, 5);
	cDoomData.level = level;
	cDoomData.area = victim ? 0 : 1;

	/* either this or damage bonus in single wave
	if(GET_SPEC(ch, CLASS_DRUID, SPEC_WOODLAND) ||
	  (world[ch->in_room].sector_type == SECT_FOREST))
	    cDoomData.waves++;*/

	act("&+LA &+gpl&+Lag&+gue &+Lof &+minsects and arachnids&+L flow like an ocean.", TRUE, ch,
	    0, victim, TO_ROOM);
	act("&+LYou send out a &+mwave of &+Linsects &+mand &+Larachnids&+m!", TRUE, ch, 0, victim,
	    TO_CHAR);

	// engage(ch, victim);
	if (victim)
		add_event(event_cdoom, 0, ch, victim, NULL, 0, &cDoomData, sizeof(CDoomData));
	else
	/* Moving doom back to a wave spell. - Lohrr
	    {
	        cast_as_damage_area(ch, spell_single_doom_aoe, level, victim,
	                      get_property("spell.area.minChance.creepingDoom", 50),
	                      get_property("spell.area.chanceStep.creepingDoom", 20));

	    }
	*/
	{
		add_event(event_cdoom, 0, ch, 0, NULL, 0, &cDoomData, sizeof(CDoomData));
		zone_spellmessage(
			ch->in_room, TRUE,
			"&+LThe &+minsects &+Lof the &+yarea&+L seem to be called away...&n\r\n",
			"&+LThe &+minsects &+Lof the &+yarea&+L seem to be called away to the %s...&n\r\n");
	}
}

struct apoc_data
{
	P_char victim;
	int stage;
	int level;
	int room;
	int next_affect;
};

static void event_apocalypse(P_char ch, [[maybe_unused]] P_char victim, P_obj /*obj*/, void *data)
{
	struct apoc_data *d = (struct apoc_data *)data;
	P_char vict, tch;
	int max_affected, i, opponents;

	struct damage_messages d_messages = {
		"$N screams in &+La&+rg&+Lo&+rn&+Ly&n as a &+Ldark h&+wa&+Lz&+we&n engulfs $M.",
		"You scream in &+La&+rg&+Lo&+rn&+Ly&n as a &+Ldark h&+wa&+Lz&+we&n engulfs you!",
		"$N screams in &+La&+rg&+Lo&+rn&+Ly&n as a &+Ldark h&+wa&+Lz&+we&n engulfs $M.",
		"$N screams in &+La&+rg&+Lo&+rn&+Ly&n as a &+Ldark h&+wa&+Lz&+we&n consumes $M completely!",
		"You scream in &+La&+rg&+Lo&+rn&+Ly&n as a &+Ldark h&+wa&+Lz&+we&n engulfs you completely!",
		"$N screams in &+La&+rg&+Lo&+rn&+Ly&n as a &+Ldark h&+wa&+Lz&+we&n engulfs $M completely!.",
		0
	};
	struct damage_messages p_messages = {
		"$N's face turns &+gg&+Lr&+ge&+Le&+gn&n as a &+Gs&+gi&+Gc&+gk&+Gl&+gy &+gc&+Ll&+go&+Lu&+gd&n descends upon $M.",
		"A &+Gs&+gi&+Gc&+gk&+Gl&+gy &+gc&+Ll&+go&+Lu&+gd&n descends upon you making you &+gc&+Lh&+go&+Lk&+ge&n and writhe in pain!",
		"$N's face turns &+gg&+Lr&+ge&+Le&+gn&n as a &+Gs&+gi&+Gc&+gk&+Gl&+gy &+gc&+Ll&+go&+Lu&+gd&n descends upon $M.",
		"$N's dies screaming as a &+Gs&+gi&+Gc&+gk&+Gl&+gy &+gc&+Ll&+go&+Lu&+gd&n descends upon $M.",
		"A &+Gs&+gi&+Gc&+gk&+Gl&+gy &+gc&+Ll&+go&+Lu&+gd&n descends upon you and never lifts again..",
		"$N's dies screaming as a &+Gs&+gi&+Gc&+gk&+Gl&+gy &+gc&+Ll&+go&+Lu&+gd&n descends upon $M."
	};

	if (!IS_ALIVE(ch))
	{
		return;
	}

	if (d->next_affect == 4 || ch->in_room != d->room)
	{
		send_to_room(
			"&+LAs the powerful summoning fades the r&+wi&+Ld&+we&+Lr&+ws &+Lof &+rh&+Le&+rl&+Ll return to the Abyss.&n\n\n",
			ch->in_room);
		return;
	}
	else if (d->stage == 0)
	{
		send_to_room("&+LA distant &+wroar &+Lcan be heard from the skies...\n\n",
			     ch->in_room);

		zone_spellmessage(ch->in_room, TRUE,
				  "&+LA distant &+wroar &+Lcan be heard off in the distance...\n\n",
				  "&+LA distant &+wroar &+Lcan be heard off from the %s...\n\n");

		d->stage++;

		add_event(event_apocalypse, (int)(PULSE_VIOLENCE / 2), ch, 0, 0, 0, d, sizeof(*d));
		return;
	}
	else if (d->stage == 1)
	{
		send_to_room(
			"&+L   ...the r&+wi&+Ld&+we&+Lr&+ws &+Lof &+rh&+Le&+rl&+Ll broke free again!\n\n",
			ch->in_room);

		d->stage++;
		add_event(event_apocalypse, (int)(PULSE_VIOLENCE / 2), ch, 0, 0, 0, d, sizeof(*d));
		return;
	}

	if (((vict = d->victim) == NULL) || !is_char_in_room(vict, ch->in_room))
	{
		for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
		{
			if (ch != tch && should_area_hit(ch, tch) && !number(0, 1))
			{
				vict = tch;
				d->victim = tch;
			}
		}
	}

	// End event_apocalypse right now. We do not have a valid vict.
	if (!vict || vict->in_room != ch->in_room)
	{
		send_to_room(
			"&+LAs the powerful summoning fades the r&+wi&+Ld&+we&+Lr&+ws &+Lof &+rh&+Le&+rl&+Ll return to the Abyss.\n",
			ch->in_room);
		return;
	}

	max_affected = (int)(get_property("spell.apocalypse.maxAffected", 4.000));

	if (d->next_affect == 0)
	{
		send_to_room("&+LThe Horseman of D&+We&+La&+Wt&+Lh appears in the sky overhead!\n",
			     ch->in_room);

		act("&+LThe Horseman of &+LD&+We&+La&+Wt&+Lh cackles and swings a &+wmassive &+Wdeadly &+Lscythe &+Lat $n!",
		    TRUE, vict, 0, 0, TO_ROOM);
		act("&+LThe Horseman of &+LD&+We&+La&+Wt&+Lh cackles and swings a &+wmassive &+Wdeadly &+Lscythe &+Lat you!",
		    TRUE, vict, 0, 0, TO_CHAR);

		if (!IS_MAGIC_DARK(ch->in_room))
		{
			spell_darkness(50, ch, 0, 0, NULL, NULL);
		}

		if (!number(0, 2) && should_area_hit(ch, vict))
		{
			spell_cloak_of_fear(40, ch, 0, 0, NULL, 0);
		}

		if (ch->in_room == vict->in_room)
		{
			spell_damage(ch, vict, dice(20, 30), SPLDAM_PSI, 0, &d_messages);
		}

		if (!IS_ALIVE(ch))
		{
			return;
		}

		d->next_affect++;
	}
	else if (d->next_affect == 1)
	{
		send_to_room("The Horseman of &+RW&+ra&+Rr&N appears in the skies!\n", ch->in_room);

		i = 0;

		for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
		{
			if (should_area_hit(ch, tch) && !number(0, 2) &&
			    !affected_by_spell(tch, SKILL_BERSERK))
			{
				if (NewSaves(tch, SAVING_SPELL, 2))
				{
					continue;
				}

				act("&+LThe Horseman of &+RW&+ra&+Rr &+Lglares with &+renraged &+Reyes &+Lat $n!&N",
				    TRUE, tch, 0, 0, TO_ROOM);
				act("&+LThe Horseman of &+RW&+ra&+Rr &+Lglares with &+renraged &+Reyes &+Lat you!",
				    TRUE, tch, 0, 0, TO_CHAR);
				berserk(tch, 1 * PULSE_VIOLENCE);

				if (++i >= max_affected)
				{
					break;
				}
			}
		}

		opponents = 0;
		for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
		{
			if (ch != tch && !grouped(ch, tch))
			{
				opponents++;
			}
		}

		if (opponents <= max_affected && should_area_hit(ch, vict))
		{
			spell_incendiary_cloud(25, ch, NULL, 0, vict, NULL);
		}
		else
		{
			spell_immolate(GET_LEVEL(ch), ch, NULL, 0, vict, NULL);
		}

		if (!IS_ALIVE(ch))
		{
			return;
		}

		d->next_affect++;
	}
	else if (d->next_affect == 2)
	{
		send_to_room(
			"&+LThe Horseman of &+yF&+Ya&+ym&+Yi&+yn&+Ye &+Lappears in the sky overhead...\n",
			ch->in_room);

		i = 0;
		for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
		{
			if (should_area_hit(ch, tch) && !number(0, 2) && !IS_STUNNED(tch))
			{
				act("The Horseman of &+yF&+Ya&+ym&+Yi&+yn&+Ye&N glares around!",
				    TRUE, tch, 0, 0, TO_ROOM);
				act("&+LThe Horseman of &+yF&+Ya&+ym&+Yi&+yn&+Ye &+Lglares at you with &+rdeathly g&+Rl&+wo&+Ww&+wi&+Rn&+rg eyes\n"
				    "&+Lcausing you to lose your &+Yconcentration.",
				    TRUE, tch, 0, 0, TO_CHAR);

				Stun(tch, ch, PULSE_VIOLENCE * 1, TRUE);
				StopCasting(tch);
				stop_memorizing(tch);

				if (++i >= max_affected)
				{
					break;
				}
			}
		}
		d->next_affect++;
	}
	else if (d->next_affect == 3)
	{
		send_to_room(
			"The Horseman of &+gP&+Le&+gs&+Lt&+gi&+Ll&+ge&+Ln&+gc&+Le&N appears in the skies!\n",
			ch->in_room);

		i = 0;
		for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
		{
			if (should_area_hit(ch, tch) && !number(0, 2))
			{
				act("The Horseman of &+gP&+Le&+gs&+Lt&+gi&+Ll&+ge&+Ln&+gc&+Le&N breathes on $n!",
				    TRUE, tch, 0, 0, TO_ROOM);
				act("The Horseman of &+gP&+Le&+gs&+Lt&+gi&+Ll&+ge&+Ln&+gc&+Le&N chokes you with its &+ywretched breath&n!",
				    TRUE, tch, 0, 0, TO_CHAR);

				if (!affected_by_spell(tch, SPELL_WITHER) && !number(0, 2))
				{
					spell_wither(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, tch,
						     0);
				}

				if (!number(0, 2))
				{
					poison_weakness(GET_LEVEL(ch), ch, 0, 0, tch, 0);
				}

				if (!affected_by_spell(tch, SPELL_DISEASE) && !number(0, 2))
				{
					spell_disease(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL,
						      tch, 0);
				}

				spell_damage(ch, tch, 3 * GET_LEVEL(ch), SPLDAM_GAS, 0,
					     &p_messages);

				if (!IS_ALIVE(ch))
				{
					return;
				}

				if (++i >= max_affected)
				{
					break;
				}
			}
		}
		d->next_affect++;
	}
	else if (d->next_affect == 4)
	{
		i = 0;
		for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
		{
			if (should_area_hit(ch, tch) && !number(0, 2))
			{
				astral_banishment(ch, tch, BANISHMENT_UNHOLY_WORD, d->level);
			}

			if (++i >= max_affected)
			{
				break;
			}
		}
		// Setting stage 3 will exit from routine after the next event_apoc.
		d->stage = 3;
	}

	if (IS_ALIVE(ch) && d->next_affect <= 4)
	{
		add_event(event_apocalypse, PULSE_VIOLENCE, ch, 0, 0, 0, d, sizeof(*d));
	}
}

// d.stage controls messages. Stages 0 and 1 have messages.
// d.next_affect controls spells and only functions while d.stage is 2.

void spell_apocalypse(int level, P_char ch, char * /*arg*/, int /*type*/, P_char vict,
		      P_obj /*obj*/)
{
	struct apoc_data d;
	P_char tch;

	if (!IS_ALIVE(ch))
	{
		return;
	}

	appear(ch);

	for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
	{
		// Only one apoc per room for players. 2 Apr 09 -Lucrot
		if (IS_PC(ch) && get_scheduled(tch, event_apocalypse))
		{
			send_to_char("Your call upon the &+Lforces of darkness&N fails!\n", ch);
			return;
		}
	}

	send_to_char("You call upon the &+Lforces of darkness&N to aid you.\n", ch);
	act("$n calls upon the &+Lforces of darkness&N!", FALSE, ch, 0, 0, TO_ROOM);
	d.stage = 0;
	d.victim = vict;
	d.level = level;
	d.room = ch->in_room;
	d.next_affect = 0;

	add_event(event_apocalypse, (int)(PULSE_VIOLENCE / 2), ch, 0, 0, 0, &d, sizeof(d));
}
