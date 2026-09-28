#include "core/prototypes.h"
#include "combat/defense_resolution.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"
#include <strings.h>

void spell_earthen_maul(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			P_obj /*obj*/)
{
	int dam, temp, dam_flag;
	struct damage_messages messages = {
		0,
		0,
		0,
		"Your &+yearthen fist&N leaves only a battered corpse of $N behind!",
		"The &+yearthen fist&N drives the last remnants of life from you!",
		"$n's &+yearthen fist&N leaves only a battered corpse of $N behind!"
	};

	temp = MIN(20, (level / 2 + 1));
	dam = dice(5 * temp, 9);

	if (level > 50)
	{
		dam = dice(6 * temp, 9);
	}

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_EARTHEN_MAUL);
	if (NewSaves(victim, SAVING_SPELL, mod))
		dam >>= 1;

	dam_flag = 0;

	/*
	 * special nasty for casting this underground or in buildings, damage
	 * is MUCH higher (from falling objects) and not limited to opponents
	 * only! (In other words, extremely dangerous to cast when not
	 * outside).
	 */

	/* default dam_flag is 0, so don't bother checking for those cases */

	switch (world[ch->in_room].sector_type)
	{
	case SECT_WATER_SWIM:
	case SECT_WATER_NOSWIM:
	case SECT_NO_GROUND:
	case SECT_UNDERWATER:
	case SECT_FIREPLANE:
	case SECT_OCEAN:
		dam_flag = 0;
		break; /*
			        * what earth to move?
			        */
	case SECT_CITY:
	case SECT_FIELD:
	case SECT_FOREST:
	case SECT_HILLS:
	case SECT_UNDERWATER_GR:
	case SECT_ROAD:
	case SECT_DESERT:
	case SECT_ARCTIC:
	case SECT_SNOWY_FOREST:
		dam_flag = 1;
		break; /*
			        * normal damage
			        */
	case SECT_MOUNTAIN:
	case SECT_UNDRWLD_WILD:
	case SECT_UNDRWLD_CITY:
	case SECT_UNDRWLD_MOUNTAIN:
	case SECT_UNDRWLD_SLIME:
	case SECT_UNDRWLD_LOWCEIL:
	case SECT_UNDRWLD_LIQMITH:
	case SECT_UNDRWLD_MUSHROOM:
		dam_flag = 2; /*
			               * dangerous, and slightly higher dam from
			               * landslides
			               */
		break;
	case SECT_INSIDE:
	case SECT_UNDRWLD_INSIDE:
		dam_flag = 3;
		break; /* dangerous, and added damage from falling debris
			        */
	case SECT_LAVA:
		dam_flag = 4; // Molten lava -> fire damage and more hurt.
		break;
	}

	if ((dam_flag == 1) && !OUTSIDE(ch))
		dam_flag = 3;

	if (ch->specials.z_cord != 0)
		dam_flag = 0;

	switch (dam_flag)
	{
	case 0:
		send_to_char("No earth to move here, try a different spell.\n", ch);
		return;
		break;
	case 1:
		messages.attacker = "&+yYou cause the &+yEARTH to reach up and maul your opponent!";
		messages.victim = messages.room =
			"$n causes the &=LyEARTH&n to rise up in the shape of a fist!";
		break;
	case 2:
		messages.attacker =
			"&+yYou cause the &+yEARTH to reach up and maul your opponent!\n"
			"&+yThe unstable nature of your surroundings, causes extreme amounts of extra debris!";
		messages.victim = messages.room =
			"$n causes the &=LyEARTH&n to rise up in the shape of a fist!";
		break;
	case 3:
		messages.attacker =
			"&+yYou cause the &+yEARTH to reach up and maul your opponent!!\n"
			"&+yAs the ceiling begins to buckle under the stress, you realize this may not have been a good idea!";
		messages.victim = messages.room =
			"$n causes the &=LyEARTH&n to rise up in the shape of a fist!";
		break;
	case 4:
		messages.attacker =
			"&+yYou cause the &+rmolten EARTH&+y to reach up and maul your opponent!";
		messages.victim = messages.room =
			"&+y$n&+y causes the &+rmolten &=RyEARTH&n&+y to rise up in the shape of a fist!&n";
		messages.death_attacker =
			"&+yYour &+rmolten fist&+y leaves only a battered corpse of $N&+y behind!&n";
		messages.death_victim =
			"&+yThe &+rmolten fist&+y drives the last remnants of life from you!&n";
		messages.death_room =
			"&+y$n&+y's &+rmolten fist&+y leaves only a battered corpse of $N&+y behind!&n";
		break;
	default:
		send_to_char("As you are about to utter the last syllable, you choke it off,\n"
			     "realizing you could well bury yourself alive!\n",
			     ch);
		return;
		break;
	}

	dam = (int)(dam + 0.1 * dam * dam_flag);

	if (spell_damage(ch, victim, dam, (dam_flag == 4) ? SPLDAM_FIRE : SPLDAM_GENERIC, 0,
			 &messages) != DAM_NONEDEAD)
		return;

	/*
	   if(number(0, 3) || (GET_CHAR_SKILL(victim, SKILL_SAFE_FALL) >= number(1, 101))) {
	   act("$n almost dodges the &+yearth&n's maul, staying on $s feet!", TRUE, victim, 0, 0, TO_ROOM);
	   act("You almost dodge the &+yearth&n's maul, staying on your feet!", TRUE, victim, 0, 0, TO_CHAR);
	   if(IS_PC(victim)) notch_skill(victim, SKILL_SAFE_FALL, 3);
	   } else {
	   act("You are almost swallowed by the earth and injure yourself!",
	   FALSE, ch, 0, victim, TO_VICT);
	   act("$n crashes to the ground!", TRUE, victim, 0, 0, TO_ROOM);
	   SET_POS(victim, number(0, 2) + GET_STAT(victim));
	   if(GET_POS(victim) == POS_PRONE)
	   Stun(victim, ch, PULSE_VIOLENCE * 2, TRUE);
	   CharWait(victim, PULSE_VIOLENCE);
	   }
	 */
}

void spell_fireball(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		    P_obj /*tar_obj*/)
{
	struct damage_messages messages = {
		"You throw a &+rfireball&N at $N and have the satisfaction of seeing $M enveloped in flames.",
		"You are enveloped in &+rflames from a fireball&N sent by $n - OUCH!",
		"$n smirks as $s &+rfireball&N explodes into the face of $N.",
		"Your &+rfireball&N hits $N with full force, causing an immediate death.",
		"$n grins evilly as you burst into &+rflames&N and die.",
		"The heat from $n's &+rfireball&N turns $N into a charred corpse.",
		0
	};

	int dam = (dice(((int)(level / 3) + 5), 6) * 4);

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_FIREBALL);
	if (!NewSaves(victim, SAVING_SPELL, mod))
		dam = (int)(dam * 2);

	spell_damage(ch, victim, dam, SPLDAM_FIRE, SPLDAM_GLOBE, &messages);
}

void spell_cyclone(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim, P_obj /*obj*/)
{
	int dam;
	int svchance, affchance;
	bool casterIsTempestMagus = false;
	struct damage_messages messages = {
		"&+WThe cyclone whirls and rips about, sending $N &+Wreeling!",
		"$n's &+Wcyclone hammers at your body!",
		"$n's &+Wcyclone tears at $N, &+Wpounding and rending flesh!",
		"&+WThe violent winds prove too much for $N!",
		"$n's &+Wcyclone has proven too much for your now soft and pulpy body.",
		"$n's &+Wcyclone tears and pummels $N until $N's lifeless body collapses."
	};

	if (victim == ch)
	{
		send_to_char("You suddenly decide against that, oddly enough.\n", ch);
		return;
	}

	casterIsTempestMagus = GET_SPEC(ch, CLASS_ETHERMANCER, SPEC_TEMPESTMAGUS);
	// dam = dice(level * 2, 9);
	dam = dice((int)(MIN(level, 40) * 2), 10);
	if (casterIsTempestMagus)
	{
		dam = (int)(dam * (1.5 + MAX(level - 30, 20) / 40.0));
	}

	svchance = (int)(level / 12);

	if (IS_AFFECTED(victim, AFF_FLY) && !NewSaves(victim, SAVING_PARA, svchance) &&
	    (!IS_ELITE(victim) || casterIsTempestMagus))
	{
		affchance = number(1, 100);

		if (affchance <= 50) /* && (!check_wall(tch->in_room, door)) */
		{
			act("The gail force of your spell sends $N flying through the room!", FALSE,
			    ch, 0, victim, TO_CHAR);
			act("The gail force of $n's spell sends you flying through the room!",
			    FALSE, ch, 0, victim, TO_VICT);
			act("The gail force of $n's spell sends $N flying through the room!", FALSE,
			    ch, 0, victim, TO_NOTVICT);
			// SET_POS(victim, POS_SITTING + GET_STAT(victim));

			if (!IS_STUNNED(victim))
			{
				Stun(victim, ch, PULSE_VIOLENCE / 2, TRUE);
			}
		}
		else if (affchance <= 5)
		{
			act("Your gail force sends $N flying through the room.", FALSE, ch, 0,
			    victim, TO_CHAR);
			act("The gail force of $n's spell sends you flying through the room.",
			    FALSE, ch, 0, victim, TO_VICT);
			act("The gail force of $n's spell sends $N flying through the room.", FALSE,
			    ch, 0, victim, TO_NOTVICT);
			// SET_POS(victim, POS_PRONE + GET_STAT(victim));

			if (!IS_STUNNED(victim) && !number(0, 2))
			{
				Stun(victim, ch, PULSE_VIOLENCE, TRUE);
			}
		}
	}

	if (!StatSave(victim, APPLY_AGI, (GET_LEVEL(victim) - GET_LEVEL(ch)) / 5) &&
	    (!IS_ELITE(victim) || casterIsTempestMagus))
	{
		spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
	}
	else
	{
		spell_damage(ch, victim, dam >> 1, SPLDAM_GENERIC, 0, &messages);
	}
}

void spell_earthquake(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		      P_obj /*obj*/)
{
	int save, dam_flag = 0, dam;
	P_char tch, next;

	/*
	 * special nasty for casting this underground or in buildings, damage
	 * is MUCH higher (from falling objects) and not limited to opponents
	 * only! (In other words, extremely dangerous to cast when not
	 * outside).
	 */
	if (!require_char(ch, "spell_earthquake", "called in magic.c with no ch"))
		return;
	if (ch->in_room > 0)
	{
		switch (world[ch->in_room].sector_type)
		{
		case SECT_EARTH_PLANE:
			dam_flag = 3;
			break;
		case SECT_WATER_SWIM:
		case SECT_WATER_NOSWIM:
		case SECT_NO_GROUND:
		case SECT_UNDERWATER:
		case SECT_FIREPLANE:
		case SECT_LAVA:
		case SECT_OCEAN:
			dam_flag = 0;
			break; /*
				        * what earthquake?
				        */
		case SECT_CITY:
		case SECT_FIELD:
		case SECT_FOREST:
		case SECT_HILLS:
		case SECT_ROAD:
			dam_flag = 1;
			break; /*
				        * normal damage
				        */
		case SECT_MOUNTAIN:
			dam_flag = 2; /*
				               * dangerous, and slightly higher dam from
				               * landslides
				               */
			break;
		case SECT_INSIDE:
			dam_flag = 3; /*
				               * dangerous, and added damage from
				               * falling debris
				               */
			break;
		default:
			dam_flag = 1;
			break;
		}

		if ((dam_flag == 1) && !OUTSIDE(ch))
		{
			dam_flag = 3;
		}
		if (ch->specials.z_cord != 0)
		{
			dam_flag = 0;
		}
		switch (dam_flag)
		{
		case 0:
			send_to_char("No earth to quake here, try a different spell.\n", ch);
			return;
			break;
		case 1:
			send_to_char("&+yYou cause the earth to shake, crack and buckle!\n", ch);
			act("$n causes an &=LyEARTHQUAKE!", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 2:
			send_to_char("&+yYou cause the earth to shake, crack and buckle!\n", ch);
			send_to_char(
				"&+yThe unstable nature of your surroundings, causes extreme amounts of extra debris!\n",
				ch);
			act("$n causes an &=LyEARTHQUAKE!", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 3:
			send_to_char("&+yYou cause the earth to shake, crack and buckle!\n", ch);
			send_to_char(
				"&+yAs the ceiling begins to cave in on you, you realize this may not have been a good idea!\n",
				ch);
			act("$n causes an &=LyEARTHQUAKE!", FALSE, ch, 0, 0, TO_ROOM);
			break;
		default:
			send_to_char(
				"As you are about to utter the last syllable, you choke it off,\nrealizing you could well bury yourself alive!\n",
				ch);
			return;
			break;
		}

		for (tch = world[ch->in_room].people; tch; tch = next)
		{
			next = tch->next_in_room;

			if (tch == ch || IS_TRUSTED(tch))
			{
				continue;
			}
			if (!IS_ALIVE(tch) || !IS_ALIVE(ch))
			{
				continue;
			}

			if (IS_GREATER_RACE(tch))
			{
				save = 0;
			}
			else
			{
				save = 4;
			}

			// Expanded the immune races. Oct08 -Lucrot
			if (GET_RACE(tch) == RACE_FLYING_ANIMAL || GET_RACE(tch) == RACE_FAERIE ||
			    LEGLESS(tch) || IS_IMMATERIAL(tch))
			{
				act("The ground rumbles beneath you, but affects you not at all.",
				    FALSE, ch, 0, tch, TO_VICT);
				act("$n seems unaffected by the quake.", TRUE, tch, 0, 0, TO_ROOM);
				continue;
			}
			if (!should_area_hit(ch, tch))
			{
				if (GET_POS(tch) < POS_STANDING)
				{
					continue;
				}

				if (StatSave(tch, APPLY_AGI, save))
				{
					act("&+LYou stagger, but manage to keep your balance!&n",
					    FALSE, ch, 0, tch, TO_VICT);
					act("$n&n &+wstaggers slightly but manages to keep $s balance.&n",
					    TRUE, tch, 0, 0, TO_ROOM);
				}
				else
				{
					act("&+mYou stagger and fall to your knees!&n", FALSE, ch,
					    0, tch, TO_VICT);
					act("$n&n &+mstaggers and falls to $s knees!&n", TRUE, tch,
					    0, 0, TO_ROOM);
					SET_POS(tch, POS_KNEELING + GET_STAT(tch));
					CharWait(tch, PULSE_VIOLENCE * 1);
				}
			}
			else
			{
				if (GET_POS(tch) < POS_STANDING)
				{
					continue;
				}

				if (tch && !StatSave(tch, APPLY_AGI, save))
				{
					if (IS_PC(tch) &&
					    GET_CHAR_SKILL(tch, SKILL_SAFE_FALL) >= number(1, 101))
					{
						act("$n&n &+Gdoes a &+ydouble sommersault &+Gand lands on $s feet!&n",
						    TRUE, tch, 0, 0, TO_ROOM);
						act("&+GYou do a &+Ydouble sommersault &+Gand land on your feet!",
						    TRUE, tch, 0, 0, TO_CHAR);
						notch_skill(tch, SKILL_SAFE_FALL, 3);
					}
					else
					{
						act("&+WYou fall and injure yourself!&n", FALSE, ch,
						    0, tch, TO_VICT);
						act("$n&n &+Wcrashes to the ground!&n", TRUE, tch,
						    0, 0, TO_ROOM);
						if (level >= 0)
						{
							dam = (int)(dice(1, 30) + level);
							if (GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT))
							{
								dam = (int)(dam * 1.25);
							}

							if (spell_damage(ch, tch, dam,
									 SPLDAM_GENERIC,
									 SPLDAM_NOSHRUG |
										 SPLDAM_NODEFLECT,
									 0) == DAM_NONEDEAD)
							{
								SET_POS(tch, number(0, 2) +
										     GET_STAT(tch));
								if (GET_POS(tch) == POS_PRONE &&
								    !number(0, 1))
								{
									Stun(tch, ch,
									     PULSE_VIOLENCE * 1,
									     FALSE);
									CharWait(tch,
										 PULSE_VIOLENCE);
								}
							}
						}
						// Lvl < 0 -> from earthen rain so we do takedown only no damage.
						else
						{
							SET_POS(tch, number(0, 2) + GET_STAT(tch));
							if (GET_POS(tch) == POS_PRONE &&
							    !number(0, 1))
							{
								Stun(tch, ch, PULSE_VIOLENCE * 1,
								     FALSE);
								CharWait(tch, PULSE_VIOLENCE);
							}
						}
					}
				}
				else if (tch && ch->specials.z_cord == tch->specials.z_cord)
				{
					act("&+LYou stagger and almost break your leg!&n", FALSE,
					    ch, 0, tch, TO_VICT);
					act("$n&n &+Lstaggers and almost falls!&n", TRUE, tch, 0, 0,
					    TO_ROOM);
					dam = (int)(dice(1, 4) + (dam_flag * (level / 2)));
					// lvl < 0 -> don't damage them as this is from earthen rain -> fall only.
					if ((level >= 0) &&
					    spell_damage(ch, tch, dam, SPLDAM_EARTH,
							 SPLDAM_NOSHRUG | SPLDAM_BREATH |
								 SPLDAM_NODEFLECT,
							 0) != DAM_NONEDEAD)
					{
						return;
					}
				}
			}
		}
		if (IS_ALIVE(ch))
		{
			zone_spellmessage(
				ch->in_room, TRUE,
				"&+yThe ea&+Lrt&+yh tr&+Lemb&+yle&+Ls an&+yd sh&+Liv&+yers!\n",
				"&+yThe ea&+Lrt&+yh tr&+Lemb&+yle&+Ls an&+yd sh&+Liv&+yers &+Lto the %s!\n");
		}
	}
}

void spell_flamestrike(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		       P_char victim, P_obj /*obj*/)
{
	struct damage_messages messages = {
		"You call down a roaring &+rflamestrike&N which hits $N dead on!",
		"$n calls down a roaring &+rflamestrike&N which hits you dead on!",
		"$n calls down a roaring &+rflamestrike&N at $N who starts to melt!",
		"$N turns into a pile of ash as your &+rflamestrike&N hits!",
		"Your body turns to ash as $n calls down a &+rflamestrike&N on you!",
		"$n calls down a &+rflamestrike&N on $N, who turns into a pile of ash!",
		0
	};
	int num_dice = (level / 5);
	int dam = (dice(num_dice, 6) * 4);

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_FLAMESTRIKE);
	if (!NewSaves(victim, SAVING_SPELL, mod))
		dam = (int)(dam * 2);

	if (GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT))
	{
		dam = (int)(dam * get_property("zealots.flamestrike.damageMulti", 1.25));
	}

	spell_damage(ch, victim, (dam / 2), SPLDAM_HOLY, RAWDAM_NOKILL, 0);
	if (IS_ALIVE(victim))
	{
		spell_damage(ch, victim, (dam / 2), SPLDAM_FIRE, 0, &messages);
	}

	if (!IS_ALIVE(victim) || !IS_ALIVE(ch))
		return;

	if (GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT))
	{
		struct affected_type *paf = NULL;
		int duration = level / 10;
		int healBlocked =
			(int)(dam * get_property("zealots.flamestrike.blockedMulti", 2.0));

		// find the flamestrike affect
		if ((paf = get_spell_from_char(victim, SPELL_FLAMESTRIKE)) == NULL)
		{
			// add it if not found
			struct affected_type af;
			bzero(&af, sizeof(af));
			af.duration = duration;
			af.type = SPELL_FLAMESTRIKE;
			paf = affect_to_char(victim, &af);

			act("You have &+rbranded&n $N with your &+Wholy &+Yf&+Rl&+Yam&+Re&+Ys&n!",
			    FALSE, ch, 0, victim, TO_CHAR);
			act("You have been &+rbranded&n by $n's &+Wholy &+Yf&+Rl&+Yam&+Re&+Ys&n!",
			    FALSE, ch, 0, victim, TO_VICT);
			act("$N has been &+rbranded&n by $n's &+Wholy &+Yf&+Rl&+Yam&+Re&+Ys&n!",
			    FALSE, ch, 0, victim, TO_NOTVICT);
		}
		// reset duration and update modifier
		paf->duration = duration;
		paf->modifier += healBlocked;
	}
}

struct call_lightning_data
{
	int room;
	int waves;
	int level;
};

void event_call_lightning(P_char ch, P_char vict, P_obj /*obj*/, void *data)
{
	struct call_lightning_data *clData = (struct call_lightning_data *)data;
	int result, dam = dice(MIN(40, clData->level), 9);
	struct damage_messages messages = {
		"A &+WMASSIVE&n &=LBlightning bolt&N strikes $N from the sky!",
		"A &+WMASSIVE&n &=LBlightning bolt&N from the heavens strikes you!",
		"A &+WMASSIVE&n &=LBlightning bolt&N strikes $N from the sky!",
		"Your &+WMASSIVE&n &=LBlightning bolt&N shatters $N to pieces.",
		"Your last vision is that of little kites circling your head, all being struck by lightning.",
		"$N is utterly shattered from the force of a &=LBlightning bolt&n from the sky.",
		0
	};

	if (!vict || vict->in_room != clData->room)
	{
		vict = get_random_char_in_room(clData->room, ch, DISALLOW_SELF | DISALLOW_GROUPED);
	}

	if (!vict)
	{
		return;
	}

	result = spell_damage(ch, vict, dam, SPLDAM_LIGHTNING, 0, &messages);

	if (result == DAM_BOTHDEAD)
	{
		return;
	}
	else if (!fear_check(vict) && !IS_GREATER_RACE(vict) && !IS_ELITE(vict) &&
		 result != DAM_VICTDEAD && !NewSaves(vict, SAVING_FEAR, 0))
	{
		send_to_char(
			"The &+Rma&+rss&+Riv&+re &+mbolt&n of &+Clig&+ch&+Wtn&+Cing&n is intimidating!\n",
			vict);
		do_flee(vict, 0, 0);
	}
	else if (result != DAM_VICTDEAD && !IS_ELITE(vict) && !IS_GREATER_RACE(vict))
	{
		Stun(vict, ch, (int)(PULSE_VIOLENCE / 2), TRUE);
	}

	if (result != DAM_CHARDEAD && (clData->waves++ < 2))
	{
		if (number(0, 1))
		{
			zone_spellmessage(
				ch->in_room, TRUE,
				"&+wThe air is filled with &+c&+Ce&+cl&+Ce&+cc&+Ct&+cr&+Ci&+cc &+Cs&+ct&+Ca&+ct&+Ci&+cc.\n",
				"&+wThe air to the %s is filled with &+c&+Ce&+cl&+Ce&+cc&+Ct&+cr&+Ci&+cc &+Cs&+ct&+Ca&+ct&+Ci&+cc.\n");
		}
		else
		{
			zone_spellmessage(
				ch->in_room, TRUE,
				"&+WA clap of &+Lthunder&n &+Wbellows off in the distance.\n",
				"&+WA clap of &+Lthunder&n &+Wbellows off to the %s.\n");
		}

		add_event(event_call_lightning, (int)(PULSE_VIOLENCE / 2), ch, vict, NULL, 0,
			  clData, sizeof(struct call_lightning_data));
	}
	else
	{
		send_to_room("&+LThe clouds overhead disperse.\n", clData->room);
	}
}

void spell_call_lightning(int level, P_char ch, P_char victim, P_obj /*obj*/)
{
	struct call_lightning_data clData;

	if (!OUTSIDE(ch))
	{
		send_to_char("You must be outdoors to summon lightning!\n", ch);
		return;
	}

	zone_spellmessage(ch->in_room, TRUE, "&+LA storm is brewing nearby...\n",
			  "&+LA storm is brewing to the %s...\n");

	send_to_room("&+LDark and ominous clouds aggregate overhead.\n", ch->in_room);

	clData.room = ch->in_room;
	clData.waves = 0;
	clData.level = level;

	add_event(event_call_lightning, PULSE_VIOLENCE, ch, victim, NULL, 0, &clData,
		  sizeof(struct call_lightning_data));
}

void spell_grow_spike(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		      P_obj /*obj*/)
{
	int dam, temp, dam_flag;
	struct damage_messages messages = { 0, 0, 0, 0, 0, 0 };

	temp = MIN(20, (level / 2 + 1));
	dam = dice(6 * temp, 6);

	if (level > 50)
	{
		dam = dice(6 * temp, 7);
	}

	dam = dam;

	if (NewSaves(victim, SAVING_SPELL, (IS_AFFECTED(victim, AFF_FLY) ? -3 : 3)))
		dam >>= 1;

	dam_flag = 0;

	/* default dam_flag is 0, so don't bother checking for those cases */

	switch (world[ch->in_room].sector_type)
	{
	case SECT_WATER_SWIM:
	case SECT_WATER_NOSWIM:
	case SECT_NO_GROUND:
	case SECT_UNDERWATER:
	case SECT_FIREPLANE:
	case SECT_OCEAN:
		dam_flag = 0;
		break; /*
			        * what earth to move?
			        */
	case SECT_CITY:
	case SECT_FIELD:
	case SECT_FOREST:
	case SECT_HILLS:
	case SECT_UNDERWATER_GR:
	case SECT_ROAD:
		dam_flag = 1;
		break; /*
			        * normal damage
			        */
	case SECT_MOUNTAIN:
	case SECT_UNDRWLD_WILD:
	case SECT_UNDRWLD_CITY:
	case SECT_UNDRWLD_MOUNTAIN:
	case SECT_UNDRWLD_SLIME:
	case SECT_UNDRWLD_LOWCEIL:
	case SECT_UNDRWLD_MUSHROOM:
		dam_flag = 2; /*
			               * dangerous, and slightly higher dam from
			               * landslides
			               */
		break;
	case SECT_INSIDE:
	case SECT_UNDRWLD_INSIDE:
		dam_flag = 3; /*
			               * dangerous, and added damage from
			               * falling debris
			               */
		break;
	case SECT_UNDRWLD_LIQMITH:
	case SECT_LAVA:
		dam_flag = 4;
		break;
	}

	if ((dam_flag == 1) && !OUTSIDE(ch))
		dam_flag = 3;

	if (ch->specials.z_cord != 0)
		dam_flag = 0;

	switch (dam_flag)
	{
	case 0:
		send_to_char("No earth to grow spikes from here, try a different spell.\n", ch);
		return;
		break;
	case 1:
		messages.attacker = "&+yYou cause the earth to form into a &+yspike&n and rise up!";
		messages.victim = messages.room =
			"$n causes the earth to form into a &+yspike&n and rise up!!";
		break;
	case 2:
	case 3:
		messages.attacker =
			"&+yYou cause &+Ya HUGE SPIKE&+y to burst out of the ground at your opponent!&n";
		messages.victim = messages.room =
			"&+y$n&+y causes the earth to form into a &+Yspike&+y and rise up!&n";
		break;
	case 4:
		messages.attacker =
			"&+yYou cause &+ra MOLTEN SPIKE&+y to burst out of the ground at your opponent!&n";
		messages.victim = messages.room =
			"&+y$n&+y causes the earth to form into a &+rspike&+y and rise up!&n";
		break;
	default:
		send_to_char("As you are about to utter the last syllable, you choke it off,\n"
			     "realizing you could well bury yourself alive!\n",
			     ch);
		return;
		break;
	}

	dam = (int)(dam + 0.1 * dam * dam_flag);

	if (spell_damage(ch, victim, dam, (dam_flag == 4) ? SPLDAM_FIRE : SPLDAM_GENERIC,
			 dam_flag > 1 ? 0 : SPLDAM_GLOBE, &messages))
		return;

	/*
	   if(number(0, 3) || (GET_CHAR_SKILL(victim, SKILL_SAFE_FALL) >= number(1, 101))) {
	   act("$n almost dodges the &+yearth&n's maul, staying on $s feet!", TRUE, victim, 0, 0, TO_ROOM);
	   act("You almost dodge the &+yearth&n's maul, staying on your feet!", TRUE, victim, 0, 0, TO_CHAR);
	   if(IS_PC(victim)) notch_skill(victim, SKILL_SAFE_FALL, 3);
	   } else {
	   act("You are almost swallowed by the earth and injure yourself!",
	   FALSE, ch, 0, victim, TO_VICT);
	   act("$n crashes to the ground!", TRUE, victim, 0, 0, TO_ROOM);
	   SET_POS(victim, number(0, 2) + GET_STAT(victim));
	   if(GET_POS(victim) == POS_PRONE)
	   Stun(victim, PULSE_VIOLENCE * 2, TRUE);
	   CharWait(victim, PULSE_VIOLENCE);
	   }
	 */
}
