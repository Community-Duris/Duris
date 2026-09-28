#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "economy/economic_gameplay_authority.h"
#include "item/item_command_policy.h"
#include "magic/spells.h"
#include <string.h>
void spell_magic_missile(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			 P_obj /*obj*/)
{
	struct damage_messages messages = {
		"You watch with self-pride as the &+Ymagic missile&N hits $N.",
		"You stagger as a &+Ymagic missile&N from $n hits you.",
		"$n throws a &+Ymagic missile&N at $N, who staggers under the blow.",
		"The &+Ymagic missile&N tears away the remaining life of $N.",
		"You only have time to notice $n uttering strange sounds before everything is dark.",
		"The &+Ymagic missile&N sent by $n causes $N to stagger and collapse in a lifeless heap."
	};
	int dam;

	int num_missiles = BOUNDED(1, (level / 3), 5);
	dam = (dice(1, 4) * 4 + number(1, level));

	while (num_missiles-- && spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_ALLGLOBES,
					      &messages) == DAM_NONEDEAD)
		;
}

void spell_chill_touch(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		       P_char victim, P_obj /*obj*/)
{
	struct damage_messages messages = {
		"You &+Bchill&N $N.",
		"You feel your life flowing away as $n &+Bchills&N you.",
		"$n &+Bchills&N $N who suddenly seems less lively.",
		"You &+Bchill&N $N.  Remember to put flowers on $S grave.",
		"You feel &+Bchilled&N by $n and then you feel no more - RIP.",
		"$n touches $N who slumps to the ground as a dead lump, rather chilly, isn't it?",
		0
	};

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
	{
		return;
	}

	int dam = (dice(1, 6) + 5 * 4 + level);

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_CHILL_TOUCH);

	bool failed_save = !NewSaves(victim, SAVING_SPELL, mod);

	if (failed_save)
		dam <<= 1;

	// wizlog(56,"chill touch damage = %d", dam);
	if (spell_damage(ch, victim, dam, SPLDAM_COLD, SPLDAM_ALLGLOBES, &messages) == DAM_NONEDEAD)
	{
		if ((victim) && (ch) && failed_save &&
		    !affected_by_spell(victim, SPELL_CHILL_TOUCH) && !IS_ELITE(victim) &&
		    !IS_GREATER_RACE(victim) && !IS_AFFECTED3(victim, AFF3_COLDSHIELD) &&
		    !IS_AFFECTED4(victim, AFF4_ICE_AURA))
		{
			act("&+BThe chilling cold causes $N&+B to stammer, apparently weakened.&n",
			    FALSE, ch, 0, victim, TO_CHAR);
			act("&+BThe cold goes right to the bone, you feel yourself weakening!&n",
			    FALSE, ch, 0, victim, TO_VICT);
			act("&+B$N &+Bsags, apparently weakened from the frigid cold!&n", FALSE, ch,
			    0, victim, TO_NOTVICT);

			dam /= 4;
			//  wizlog(56,"chill touch stat damage = %d", dam);
			struct affected_type af;
			memset(&af, 0, sizeof(af));
			af.type = SPELL_CHILL_TOUCH;
			af.duration = 1;

			af.location = APPLY_STR;
			af.modifier = -(number(1, dam));
			affect_to_char(victim, &af);

			if (GET_CLASS(ch, CLASS_NECROMANCER) && IS_SPECIALIZED(ch))
			{
				af.location = APPLY_AGI;
				af.modifier = -(number(1, dam));
				affect_to_char(victim, &af);

				af.location = APPLY_DEX;
				af.modifier = -(number(1, dam));
				affect_to_char(victim, &af);
			}
		}
	}
}

void spell_frostbite(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		     P_obj /*obj*/)
{
	struct damage_messages messages = {
		"&+C$N&+C screams in agony as your biting cold freezes $S soul!",
		"&+CYou scream in agony as $n's&+C biting cold freezes your soul!",
		"&+C$N&+C screams in agony as $n's&+C biting cold freezes $S soul!",
		"&+CYour intense cold has claimed $N's life!",
		"&+C$n's&+C intense cold has claimed your life!",
		"&+C$n's&+C intense cold has claimed $N's&+C life!",
		0
	};

	int dam = (int)(MIN(40, level) * 5) + number(1, 20);

	if (spell_damage(ch, victim, dam, SPLDAM_COLD, 0, &messages) != DAM_VICTDEAD)
	{
		int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_FROSTBITE);
		if (!affected_by_spell(victim, SPELL_FROSTBITE) &&
		    !NewSaves(victim, SAVING_SPELL, mod))
		{
			act("&+BThe chilling cold causes $N&+B to stammer, apparently weakened.&n",
			    FALSE, ch, 0, victim, TO_CHAR);
			act("&+BThe cold goes right to the bone, you feel yourself weakening!&n",
			    FALSE, ch, 0, victim, TO_VICT);
			act("&+B$N &+Bsags, apparently weakened from the frigid cold!&n", FALSE, ch,
			    0, victim, TO_NOTVICT);

			struct affected_type af;
			memset(&af, 0, sizeof(af));
			af.type = SPELL_FROSTBITE;
			af.duration = 1;

			af.modifier = -number(10, 15);
			af.location = APPLY_STR;
			affect_to_char(victim, &af);
		}
	}
}

void spell_burning_hands(int level, P_char ch, char * /*arg*/, int type, P_char victim,
			 P_obj /*obj*/)
{
	struct damage_messages messages = { "You &+Yburned&n $N.",
					    "You cry out in pain as $n burns you.",
					    "$N cries out as $n burns $M.",
					    "You &+Yburned&n $N to death.",
					    "You have been burned to death by $n.",
					    "$n has burned $N to death.",
					    0 };

	int num_dice = (level / 10);
	int dam = (dice((num_dice + 5), 6) * 4);

	// dragoon dragon priest bonus
	if (type > 1)
		dam += dam * 0.05;

	spell_damage(ch, victim, dam, SPLDAM_FIRE, SPLDAM_ALLGLOBES, &messages);
}

void spell_shocking_grasp(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			  P_obj /*obj*/)
{
	struct damage_messages messages = {
		"You get a good hold of the shocked $N.",
		"You get a shock as $n gets too close to you.",
		"$N looks shocked as $n grasps at $M.",
		"$N dies while looking rather shocked.",
		"$n shocks you.  Unfortunately, your heart can't stand it...",
		"$n reveals with a shock that $N has left $S body.",
		0
	};

	int num_dice = (level / 6);
	int dam = (dice(num_dice + 5, 6) * 4);
	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_SHOCKING_GRASP);
	if (!NewSaves(victim, SAVING_SPELL, mod))
		dam = (int)(dam * 2);

	spell_damage(ch, victim, dam, SPLDAM_LIGHTNING, SPLDAM_ALLGLOBES, &messages);
}

void spell_lightning_bolt(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			  P_obj /*obj*/)
{
	struct damage_messages messages = {
		"The &=LBlightning bolt&N hits $N with full impact.",
		"YOU'RE HIT!  A &=LBlightning bolt&N from $n has reached its goal.",
		"$N wavers under the impact of the &=LBlightning bolt&N sent by $n.",
		"Your &=LBlightning bolt&N shatters $N to pieces.",
		"You feel enlightened by the &=LBlightning bolt&N $n sends, and then all is dark - RIP.",
		"$N receives the full &+yblast&N of a &=LBlightning bolt&+B from $n ... and is no more.",
		0
	};

	int num_dice = (level / 5);
	int dam = (dice(num_dice + 5, 6) * 4);
	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_LIGHTNING_BOLT);
	if (!NewSaves(victim, SAVING_SPELL, mod))
		dam = (int)(dam * 1.33);

	spell_damage(ch, victim, dam, SPLDAM_LIGHTNING, SPLDAM_GLOBE | SPLDAM_GRSPIRIT, &messages);
}

void spell_cone_of_cold(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			P_char victim, P_obj /*obj*/)
{
	struct damage_messages messages = {
		"&+BYour blast of cold strikes $N &+Bdead on, who shudders from the pain.",
		"&+BA blast of cold sent by $n &+Bstrikes you dead on, freezing up your limbs! BRRR.",
		"$n &+Bfires a blast of cold at $N&+B, who screams out in pain!",
		"&+BYour blast of cold&N freezes $N &+Bsolid, who lifelessly falls to the ground.",
		"Your body turns to ice as $n &+Bfires a blast of cold at you, AARRGHGHHGH!!",
		"$n fires a &+Bblast of cold at $N&+B, who freezes solid and dies instantly!",
		0
	};

	int num_dice = (level / 4);
	int dam = (dice(num_dice + 5, 6) * 4);

	if (spell_damage(ch, victim, dam, SPLDAM_COLD, SPLDAM_GLOBE | SPLDAM_GRSPIRIT, &messages) ==
	    DAM_NONEDEAD)
	{
		bool is_wet = IS_AFFECTED5(victim, AFF5_WET) ? TRUE : FALSE;
		if (!affected_by_spell(victim, SPELL_CONE_OF_COLD) &&
		    !NewSaves(victim, SAVING_SPELL, is_wet ? 5 : 0))
		{
			int duration = (int)(WAIT_SEC * 1.5 * level / 50);
			int modifier = -number(5, 10);

			struct affected_type af;
			memset(&af, 0, sizeof(af));

			if (is_wet)
			{
				act("&+BThe chilling cold causes $N&+B to stammer, apparently weakened and slowed.&n",
				    FALSE, ch, 0, victim, TO_CHAR);
				act("&+BThe cold goes right to the bone, you feel yourself weakening and slowing down!&n",
				    FALSE, ch, 0, victim, TO_VICT);
				act("&+B$N &+Bsags, apparently weakened and slowed from the frigid cold!&n",
				    FALSE, ch, 0, victim, TO_NOTVICT);
				duration += 2;
				modifier -= number(5, 10);
				af.bitvector2 = AFF2_SLOW;
			}
			else
			{
				act("&+BThe chilling cold causes $N&+B to stammer, apparently weakened.&n",
				    FALSE, ch, 0, victim, TO_CHAR);
				act("&+BThe cold goes right to the bone, you feel yourself weakening!&n",
				    FALSE, ch, 0, victim, TO_VICT);
				act("&+B$N &+Bsags, apparently weakened from the frigid cold!&n",
				    FALSE, ch, 0, victim, TO_NOTVICT);
			}

			af.type = SPELL_CONE_OF_COLD;
			af.flags = AFFTYPE_SHORT | AFFTYPE_NOSAVE;
			af.duration = duration;
			af.location = APPLY_STR;
			af.modifier = modifier;

			affect_to_char(victim, &af);
		}
	}
}

void spell_harm(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		P_obj /*obj*/)
{
	int dam;
	struct damage_messages messages = {
		"The power of your god causes $N's body to suffer from your harm spell!",
		"A mighty force from above rips you apart as a harm spell hits you.",
		"$n calls upon the power of the gods to tear apart the insides of $N with a mighty harm spell!",
		"Your harm spell reaches deep into the soul of $N and crushes it!",
		"$n calls upon down the power of the gods to crush you in a harm spell...death soon follows..",
		"$N is slowly ripped into many tiny bits as the harm spell of $n, ends his life.",
		0
	};

	if (saves_spell(victim, SAVING_SPELL))
		dam = 100;
	else
		dam = 200;

	if (GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT))
	{
		dam = (int)(dam * 1.25);
	}

	spell_damage(ch, victim, dam, SPLDAM_HOLY, RAWDAM_NOKILL, &messages);
}

void spell_full_harm(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		     P_obj /*obj*/)
{
	int dam;
	struct damage_messages messages = {
		"&+WYou call down the FULL wrath of your god on $N&+W!",
		"&+W$n&N calls down a holy blast of might at you, ouch!",
		"&+WA beam of pure holy wrath is called down on $N &+Wby $n!",
		0,
		0,
		0,
		0
	};

	dam = (level / 3 * 11);
	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_FULL_HARM);
	if (!NewSaves(victim, SAVING_SPELL, mod))
		dam = (dam * 2);

	if (GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT))
	{
		dam = (int)(dam * 1.25);
	}

	spell_damage(ch, victim, dam, SPLDAM_HOLY, RAWDAM_NOKILL, &messages);
}

void spell_cause_light(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		       P_obj /*obj*/)
{
	int dam;
	struct damage_messages messages = {
		"Your touch sends shivers of pain through $N's body.",
		"You are filled with pain as $n touches you.",
		"$n touches $N, who shivers in pain.",
		"Your light wounds are enough to send $N over the brink....",
		"For a moment, $n wracks you with pain.   Then, there is nothing.",
		"The wounds inflicted by $n are enough to end the life of $N."
	};

	dam = 4 * dice(1, 8);

	if (GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT))
	{
		dam = (int)(dam * 1.25);
	}

	spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
}

void spell_cause_serious(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			 P_obj /*obj*/)
{
	int dam;
	struct damage_messages messages = {
		"$N's body is wracked with spasms as you touch $M!",
		"Blinding &+Rpain&n shoots through your body as $n touches you!",
		"$n's touch doesn't seem so casual as $N doubles over in pain!",
		"You seriously wound $N unto the point of death.",
		"$n sends blistering pain down your spine.  Then there is only darkness.",
		"$n's spell causes sufficient wounds to kill $N."
	};

	dam = 4 * dice(2, 8);

	if (GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT))
	{
		dam = (int)(dam * 1.25);
	}

	spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
}

void spell_unholy_wind(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		       P_obj /*obj*/)
{
	int dam;
	struct damage_messages messages = {
		"&+LYou send a ghastly wind out to rot $N's&+L flesh.",
		"&+L$n&+L sends a ghastly wind out to rot your flesh.",
		"&+L$n&+L send a ghastly wind out to rot $N's&+L flesh.",
		"&+LYour evil wind kills $N&+L, leaving only a hump of blackness.",
		"&+L$n's&+L evil wind destroys you, leaving only a hump of blackness.",
		"&+L$n's&+L evil wind kills $N&+L, leaving only a hump of blackness."
	};

	if (IS_UNDEADRACE(victim))
	{
		send_to_char("&+LYour victim is not among the living.&n\n", ch);
		return;
	}

	dam = (dice(1, 6) + 5 * 4 + level);

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_UNHOLY_WIND);
	bool failed_save = !NewSaves(victim, SAVING_SPELL, mod);

	if (failed_save)
		dam <<= 1;

	spell_damage(ch, victim, dam, SPLDAM_HOLY, SPLDAM_ALLGLOBES, &messages);
}

void spell_purge_living(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			P_obj /*obj*/)
{
	int dam, temp;
	struct damage_messages messages = {
		"&+MYou direct a &+Ldark beam&+M at $N attempting to purge $S soul!",
		"&+M$n&+M directs a &+Ldark beam&+M at you attempting to purge your soul!",
		"&+M$n&+M direct a &+Ldark beam&+M at $N&+M attempting to purge $S soul!",
		"&+MYou have purged the realms of yet another living soul!",
		"&+M$n&+M has purged the realms of yet another living soul... YOU!",
		"&+M$n&+M has purged the realms of yet another living soul!"
	};

	if (IS_UNDEADRACE(victim))
	{
		send_to_char("&+LYour victim is not among the living.&n\n", ch);
		return;
	}
	temp = MIN(35, (level + 1));
	dam = dice(temp, 10);

	spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, 0, &messages);
}

void spell_cause_critical(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			  P_obj /*obj*/)
{
	int dam;
	struct damage_messages messages = {
		"Your touch sends wracking pains through $N's body.",
		"You are almost dissolved by pain as $n touches you.",
		"$N screams as $n critically injures $M.",
		"You kill $N with a critical touch.",
		"You are wracked with searing pain from $n. Then, no more.",
		"$n's touch critically wounds $N, who dies from the pain."
	};

	dam = dice(12, 8) + 40;

	if (GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT))
	{
		dam = (int)(dam * 1.25);
	}

	spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
}

void spell_acid_stream(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim, P_obj obj)
{
	int dam;
	struct damage_messages messages = {
		"You direct a stream of &+Gacid&n into $N; $E cries out as flesh that was once $S is now lost to the elements.",
		"$n sends a stream of &+Gacid&n at you, burning away skin and muscle that you rather miss.",
		"$n sends a stream of &+Gacid&n into $N, vaporizing fragile carbon compounds and creating a horrible stench.",
		"You direct a stream of &+Gacid&n into $N; $S life is quickly terminated.",
		"$n sends a stream of &+Gacid&n at you, coating your entire body! You don't feel so good.",
		"$n sends a perfectly-aimed stream of &+Gacid&n arcing towards $N, turning $M into so many base compounds!"
	};

	dam = dice(MIN(51, level - 4) + 3, 14);
	if (spell_damage(ch, victim, dam, SPLDAM_ACID, 0, &messages) == DAM_NONEDEAD)
		spell_slow(level, ch, 0, 0, victim, obj);
}

void spell_acid_blast(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		      P_obj /*obj*/)
{
	struct damage_messages messages = {
		"$N screams in pain as your &+Gacid blast&n burns $M.",
		"$n blasts you with acid... OUCH!",
		"$n &+Gblasts&n $N with &+Gacid,&n you cringe.",
		"$N is dissolved into a sticky ooze.",
		"You are hit by a &+Gblast of acid&n from $n. Goodbye cruel world.",
		"$n turns $N into a &+Gsticky puddle.&n",
		0
	};

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim) || ch->in_room != victim->in_room)
		return;

	int dam = dice(MIN(level, 21), 10) + level / 2;

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_ACID_BLAST);
	if (!NewSaves(victim, SAVING_SPELL, mod))
		dam = (int)(dam * 1.25);

	spell_damage(ch, victim, dam, SPLDAM_ACID, SPLDAM_ALLGLOBES, &messages);
}

void spell_destroy_undead(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			  P_obj /*obj*/)
{
	int dam;
	struct damage_messages messages = {
		"$N wavers under your deity's power!",
		"Piercing light from $n's holy symbol hurts you!",
		"$N wavers under the onslaught caused by $n's deity's power!",
		"$N is disintegrated by the unleashed power of your deity!",
		"You see $n's raised holy symbol, and nothing more.",
		"$N is dispelled entirely by $n's deity's power!",
		0
	};

	struct damage_messages notcleric_msgs = {
		"$N wavers under the power of your will!",
		"$n's pure will causes you horrible pain!",
		"$N wavers under the power of $n's will!",
		"$N is completely destroyed by your mastery of the walking dead!",
		"$n's mastery over you is complete as he wills you to exist no more!",
		"$N is completely destroyed by $n's raw will!",
		0
	};

	if (!require_char(ch, "spell_destroy_undead", "called in magic.c with no ch"))
		return;

	if (ch && victim && !IS_UNDEADRACE(victim) && !IS_AFFECTED(victim, AFF_WRAITHFORM))
	{
		send_to_char("Your victim isn't even dead, much less undead!\n", ch);
		return;
	}

	dam = (15 * MIN(level, 56)) + number(-40, 40);
	// dam = 13 * level + number(0, level);
	if (saves_spell(victim, SAVING_SPELL))
		dam >>= 1;

	if (GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT))
	{
		dam = (int)(dam * 1.25);
	}

	spell_damage(ch, victim, dam, SPLDAM_HOLY, 0, IS_CLERIC(ch) ? &messages : &notcleric_msgs);
}
void spell_solar_flare(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		       P_obj /*obj*/)
{
	int dam;
	struct damage_messages messages = {
		"You send a burst of &+Rpure &+rf&+Ri&+rr&+Re&n towards&n $N &+Rburning $S flesh.",
		"A burst of &+Rpure &+rf&+Ri&+rr&+Re&n slams into your chest burning the skin of your body.",
		"A burst of &+Rpure &+rf&+Ri&+rr&+Re&n leaps from&n $n &+Rburning the flesh of&n $N.",
		"You send a burst of &+Rpure &+rf&+Ri&+rr&+Re&n towards&n $N &+Rturning $M into a pile of ashes.",
		"A burst of &+Rpure &+rf&+Ri&+rr&+Re&n slamming into your chest is the last thing you see..",
		"A burst of &+Rpure &+rf&+Ri&+rr&+Re&n leaps from&n $n &+Rturning&n $N &+Rinto a pile of ashes.",
		0
	};

	dam = (6 * level) + number(1, 25);

	if (spell_damage(ch, victim, dam, SPLDAM_FIRE, 0, &messages))
		return;

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_SOLAR_FLARE);
	if (!number(0, 1) && !NewSaves(victim, SAVING_SPELL, mod))
		blind(ch, victim, number(4, 12) * WAIT_SEC);

	if (!number(0, 1))
		spell_immolate(level, ch, NULL, 0, victim, NULL);
}
void spell_life_bolt(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		     P_obj /*obj*/)
{
	struct damage_messages holy_messages = {
		"&+WYou sacrifice part of your lifeforce and send a pure white beam of &+Yholy energy&+W at $N&+W!",
		"&+wYou recoil in pain as $n &+wstretches out $s &+whands and a &+Wpure white&n&+w beam of &+Yholy energy&n&+w hits you dead-on!",
		"&+w$n stretches out $s hands and a &+Wpure white&n&+w beam of &+Yholy energy&n&+w hits $N &+wdead-on!",
		"&+WYou sacrifice part of your lifeforce and &+Rdisintegrate&n $N &+Wwith a pure white stream of &+Yholy energy&+W!",
		"&+w$n &+Rtears&+W your &+Ysoul&n&+W apart with a pure white stream of &+Yholy energy&+W beaming from $s outstretched hands!",
		"&+w$n stretches out $s hands and &+Rdisintegrates&n $N &+w with a &+Wpure white&n&+w beam of &+Yholy energy&n&+w!",
	};

	struct damage_messages unholy_messages = {
		"&+WYou sacrifice part of your lifeforce and send a &+Lblack beam&+W of &n&+munholy energy&+W at $N&+W!",
		"&+wYou recoil in pain as $n &+wstretches out $s &+whands and a &+Lblack beam of &+munholy energy&n&+w hits you dead-on!",
		"&+w$n stretches out $s hands and a &+Lwhite beam of &+munholy energy&n&+w hits $N &+wdead-on!",
		"&+WYou sacrifice part of your lifeforce and &+Rdisintegrate&n $N &+Wwith a stream of &+munholy energy&+W!",
		"&+w$n &+Rtears&+W your &+Ysoul&n&+W apart with a &+Lblack stream&n&+W of &+munholy energy&+W beaming from $s outstretched hands!",
		"&+w$n stretches out $s hands and &+Rdisintegrates&n $N &+w with a &+Lpure black&n&+w beam of &+munholy energy&n&+w!",
	};

	struct damage_messages neutral_messages = {
		"&+WYou sacrifice part of your lifeforce and send a white beam of &+Yholy energy&+W at $N&+W!",
		"&+wYou recoil in pain as $n &+wstretches out $s &+whands and a &+Wwhite&n&+w beam of &+Ypure energy&n&+w hits you dead-on!",
		"&+w$n stretches out $s hands and a &+Wwhite&n&+w beam of &+Yenergy&n&+w hits $N &+wdead-on!",
		"&+WYou sacrifice part of your lifeforce and &+Rdisintegrate&n $N &+Wwith a white stream of &+Yenergy&+W!",
		"&+w$n &+Rtears&+W your &+Ysoul&n&+W apart with a &+Wwhite stream of &+Yenergy&+W beaming from $s outstretched hands!",
		"&+w$n stretches out $s hands and &+Rdisintegrates&n $N &+w with a &+Wwhite&n&+w beam of &+Yenergy&n&+w!",
	};

	int dam, self_dam = 0;

	int num_missiles = BOUNDED(1, (level / 3), 5);

	bool opposing_align;

	dam = (dice(1, 4) * 4 + number(1, level)) * num_missiles;

	if (!victim)
	{
		send_to_char("You need someone as a target to your spell.\r\n", ch);
		return;
	}

	if (get_property("spell.lifebolt.selfdam.lvl", 0.000) &&
	    GET_LEVEL(ch) >= get_property("spell.lifebolt.selfdam.lvl", 0.000) &&
	    GET_SPEC(ch, CLASS_THEURGIST, SPEC_TEMPLAR))
	{
		self_dam = dam;

		if (IS_PC(victim))
			self_dam >>= 2;
		else
			self_dam >>= 1;

		if (GET_HIT(ch) < self_dam)
		{
			send_to_char(
				"&+WYou're too weak to sacrifice your life force in this way! You gain no bonus.\r\n",
				ch);
		}
		else if (self_dam)
		{
			send_to_char(
				"&+WYou send a quick prayer to your Deity, offering your &+Rlifeforce&+W and asking for divine energy to flow through you!\r\n",
				ch);

			vamp(ch, self_dam / 6,
			     static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
			// if (spell_damage(ch, ch, self_dam, SPLDAM_HOLY, RAWDAM_NOKILL | SPLDAM_NOSHRUG, 0) != DAM_NONEDEAD)
			//        return;
		}
	}

	opposing_align = IS_OPPOSING_ALIGN(ch, victim);

	if (!opposing_align)
	{
		dam >>= 1;
	}

	dam += self_dam;

	if (resists_spell(ch, victim))
		return;

	if (IS_EVIL(ch))
		spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, 0, &unholy_messages);
	else if (IS_GOOD(ch))
		spell_damage(ch, victim, dam, SPLDAM_HOLY, 0, &holy_messages);
	else
		spell_damage(ch, victim, dam, SPLDAM_HOLY, 0, &neutral_messages);

	if (opposing_align)
	{
		if (IS_ALIVE(victim))
			send_to_char("&+WThe beam &+Rrends&+W your soul!\r\n", victim);

		if (IS_ALIVE(ch))
			send_to_char("&+WYour victim seems to be in excruciating pain!\r\n", ch);
	}
}

void spell_negative_concussion_blast(int level, P_char ch, char * /*arg*/, int /*type*/,
				     P_char victim, P_obj /*obj*/)
{
	struct damage_messages messages = {
		"&+LYour concussion &+Yblast&+L rips into $N, &+Wsh&+Lat&Nte&+Wri&+Lng $S&+r soul&+L!&N",
		"&+L$n's concussion &+Yblast&+L rips into you, &+Wsh&+Lat&Nte&+Wri&+Lng your &+rsoul&+L!&N",
		"&+L$n's concussion &+Yblast&+L rips into $N, &+Wsh&+Lat&Nte&+Wri&+Lng $S&+r soul&+L!&N",
		"&+LYour blast shatters $N &+Linto a million pieces!&N",
		"&+LYou scream as $n &+Lblasts you into the next life!&N",
		"$n &+Lblasts $N &+Linto the next life!&N"
	};
	int dam;

	if (!ch)
	{
		logit(LOG_EXIT, "spell_negative_concussion_blast called in magic.c with no ch");
		return;
	}
	if (ch)
	{
		if (!IS_ALIVE(ch))
		{
			return;
		}

		if (resists_spell(ch, victim))
		{
			return;
		}

		dam = dice(2 * level, 6);

		spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, SPLDAM_NOSHRUG, &messages);
	}
}

void spell_chaos_volley(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			P_obj /*obj*/)
{
	struct damage_messages messages = {
		"&+YC&+yr&+Ya&+yc&+Yk&+yl&+Ying magical &+Cbolts&+L erupt from your palms and slam into&n $N&+L!&n",
		"&+LPow&+rerf&+Lul ma&+rgic&+Lal &+Cbolts&+L slam into you from&n $n's&+L open palms!&n",
		"$n&n&+L has a blank expression as $s &+rC&+Rh&+rA&+RO&+rti&+RC&+C bolts&+L slam into&n $N!&n",
		"&+LYou nod in approval as your chaotic &+Cbolt&+L smashes&n $N&+L into &+rob&+Rli&+rvi&+Ron!&n",
		"&+LThe last thing you see as your &+Wlife&n &+Lflashes before your eyes is the &+yground! &+RR.I.P.&n",
		"$n&n&+L is greatly pleased as $s magical &+Cbolts&+L decimate&n $N.&n",
		0
	};

	int luckmod;
	int cvdam = BOUNDED(0, (GET_MAX_HIT(ch) - GET_HIT(ch)), 250); // Health damage modifier
	int dam = MIN(level, 56) * 9 +
		  number(1, 20); // Bigby's clenched fist * 10; Bigby's crushing hand *12

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
		return;

	if (GET_HIT(ch) > (GET_MAX_HIT(ch) / 2)) // Drains hps up to 1/2 max!!!
		GET_HIT(ch) -= (dam / 30);

	dam = dam + cvdam; // Adding in current health modifier

	// Luck influences chaos
	luckmod = BOUNDED(-2, (GET_C_LUK(ch) - GET_C_LUK(victim)) / 10, 2);

	if (!NewSaves(victim, SAVING_SPELL, luckmod))
		dam *= 2; // Failed bigby's clenched and cruahsing do * 2 dam

	int dam1 = number(1, dam); // This randomizes 1st damage amount
	int dam2 = dam - dam1; // This is the 2nd damage amount

	switch (number(1, 11))
	{
	case 1:
		spell_damage(ch, victim, dam1, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
		if (IS_ALIVE(ch) && IS_ALIVE(victim))
			spell_damage(ch, victim, dam2, SPLDAM_GENERIC, SPLDAM_NOSHRUG, &messages);
		break;
	case 2:
		spell_damage(ch, victim, dam1, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
		if (IS_ALIVE(ch) && IS_ALIVE(victim))
			spell_damage(ch, victim, dam2, SPLDAM_FIRE, SPLDAM_NOSHRUG, &messages);
		break;
	case 3:
		spell_damage(ch, victim, dam1, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
		if (IS_ALIVE(ch) && IS_ALIVE(victim))
			spell_damage(ch, victim, dam2, SPLDAM_COLD, SPLDAM_NOSHRUG, &messages);
		break;
	case 4:
		spell_damage(ch, victim, dam1, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
		if (IS_ALIVE(ch) && IS_ALIVE(victim))
			spell_damage(ch, victim, dam2, SPLDAM_LIGHTNING, SPLDAM_NOSHRUG, &messages);
		break;
	case 5:
		spell_damage(ch, victim, dam1, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
		if (IS_ALIVE(ch) && IS_ALIVE(victim))
			spell_damage(ch, victim, dam2, SPLDAM_GAS, SPLDAM_NOSHRUG, &messages);
		break;
	case 6:
		spell_damage(ch, victim, dam1, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
		if (IS_ALIVE(ch) && IS_ALIVE(victim))
			spell_damage(ch, victim, dam2, SPLDAM_ACID, SPLDAM_NOSHRUG, &messages);
		break;
	case 7:
		spell_damage(ch, victim, dam1, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
		if (IS_ALIVE(ch) && IS_ALIVE(victim))
			spell_damage(ch, victim, dam2, SPLDAM_NEGATIVE, SPLDAM_NOSHRUG, &messages);
		break;
	case 8:
		spell_damage(ch, victim, dam1, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
		if (IS_ALIVE(ch) && IS_ALIVE(victim))
			spell_damage(ch, victim, dam2, SPLDAM_HOLY, SPLDAM_NOSHRUG, &messages);
		break;
	case 9:
		spell_damage(ch, victim, dam1, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
		if (IS_ALIVE(ch) && IS_ALIVE(victim))
			spell_damage(ch, victim, dam2, SPLDAM_PSI, SPLDAM_NOSHRUG, &messages);
		break;
	case 10:
		spell_damage(ch, victim, dam1, SPLDAM_GENERIC, SPLDAM_NODEFLECT, &messages);
		if (IS_ALIVE(ch) && IS_ALIVE(victim))
			spell_damage(ch, victim, dam2, SPLDAM_SPIRIT, SPLDAM_NOSHRUG, &messages);
		break;
	case 11:
		spell_damage(ch, victim, dam, SPLDAM_SOUND, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT,
			     &messages);
		break;
	}
}

void spell_single_doom_aoe(int level, P_char ch, char * /*args*/, int /*type*/, P_char victim,
			   P_obj /*obj*/)
{
	int dam;
	struct damage_messages messages = {
		"",
		"",
		"",
		"$N's body turns to &+Wbone&n as the &+yinsects&n consume $M.",
		"Your body succumbs to the overwhelming &+yplague&n of &+ginsects&n.",
		"$N's body turns to &+Wbone&n as the &+yinsects&n consume $M.",
		0
	};
	dam = 110 + level * 3 + number(1, 10);

	spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
}

#define RIPPLE_STUN 1
#define RIPPLE_BLIND 2
#define RIPPLE_FIREBALL 3
#define RIPPLE_TENTACLE 4
#define RIPPLE_PARA 5
#define RIPPLE_SWITCH 6
#define RIPPLE_BOLTS 7

void spell_chaotic_ripple(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			  P_char victim, P_obj /*obj*/)
{
	int dam, rays;
	struct affected_type af;
	P_char tch;
	struct damage_messages messages = {
		0,
		0,
		0,
		"&+LYour&n &+Cr&+ci&+Cp&+cp&+Cl&+ce&+Cs&n &+Lof&n &+RCh&+rA&+Ro&+rT&+Ri&+rC energy&n &+Lengulf&n $N &+Lblasting the &+wlife &+Lfrom $S body!",
		"&+LRipples of&n &+RCh&+rA&+Ro&+rT&+Ri&+rC energy &+Lengulf you\nblasting the life from your body.&n",
		"$n's&n &+Cr&+ci&+Cp&+cp&+Cl&+ce&+Cs&n &+Lof&n &+RCh&+rA&+Ro&+rT&+Ri&+rC energy&n &+Lengulf&n $N &+Lblasting the life from $S body!",
		0
	};
	if (!ch)
	{
		logit(LOG_EXIT, "spell_chaotic_ripple called in magic.c with no ch");
		return;
	}
	messages.attacker =
		"&+LYou shatter the fabric of reality sending&n &+Cr&+ci&+Cp&+cp&+Cl&+ce&+Cs&n &+Lof&n &+RCh&+rA&+Ro&+rT&+Ri&+rC energy&n &+Lflowing into&n $N.";
	messages.victim =
		"&+LRipples of&n &+RCh&+rA&+Ro&+rT&+Ri&+rC energy &+Lslam into you as reality comes crashing down.&n";
	messages.room =
		"$n &+Lshatters the fabric of reality\n&+Lsending&n &+Cr&+ci&+Cp&+cp&+Cl&+ce&+Cs&n &+Lof&n &+RCh&+rA&+Ro&+rT&+Ri&+rC energy &+Lflowing into&n $N.";

	dam = 370 + (30 * MAX(1, level - 50)) + number(1, level);

	if (spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG,
			 &messages) != DAM_NONEDEAD)
	{
		return;
	}
	rays = number(2, 4);
	if (!number(0, 99)) // 1%
	{
		rays += number(2, 4);
	}
	else if (!number(0, 19)) // 5%
	{
		rays += number(1, 2);
	}

	while (rays > 0)
	{
		int ray_type;

		if (!char_in_list(victim))
		{
			break;
		}
		ray_type = number(0, 6);
		rays--;
		memset(&af, 0, sizeof(struct affected_type));
		switch (ray_type)
		{
		case RIPPLE_STUN:
			act("&+wClutching $S head $N tries to escape the\n&+rm&+wa&+rd&+wd&+re&+wn&+ri&+wn&+rg&n &+wimages circling $M.&n",
			    TRUE, victim, 0, victim, TO_ROOM);
			act("&+wClutching your head you try to escape the\n&+rm&+wa&+rd&+wd&+re&+wn&+ri&+wn&+rg&n &+Wimages circling you!&n",
			    TRUE, victim, 0, victim, TO_CHAR);
			Stun(victim, ch, PULSE_VIOLENCE / 2, FALSE);
			break;
		case RIPPLE_BLIND:
			act("&+LDarkness&n flows from the rift encasing $N in an impenetrable shell.&n",
			    TRUE, victim, 0, victim, TO_ROOM);
			act("&+LDarkness&n flows from the rift encasing you in an impenetrable shell!&n",
			    TRUE, victim, 0, victim, TO_CHAR);
			blind(ch, victim, PULSE_VIOLENCE);
			break;
		case RIPPLE_FIREBALL:
			messages.attacker = messages.room =
				"&+RBOOOOOOOOOM!&n\n$N explodes in flames as a &+rGIGANTIC &+Rfireball&n engulfs $m.";
			messages.victim =
				"&+RBOOOOOOOOOM!&n\nYou explode in flames as a &+rGIGANTIC &+Rfireball&n engulfs you.";
			if (spell_damage(ch, victim, (int)(0.4 * dam), SPLDAM_FIRE, 0, &messages) !=
			    DAM_NONEDEAD)
			{
				return;
			}
			break;
		case RIPPLE_TENTACLE:
			if (!StatSave(victim, APPLY_AGI,
				      number(-2, 2))) // chaos spell, mod is random
			{
				act("&+LA tentacle of &+Wsolified m&+wi&+Wst &+Lcoils&n itself around $N\ntossing $M to the ground.",
				    TRUE, victim, 0, victim, TO_ROOM);
				act("&+LA tentacle of &+Wsolified m&+wi&+Wst &+Lcoils&n itself around you\ntossing you to the ground.",
				    TRUE, victim, 0, victim, TO_CHAR);
				SET_POS(victim, POS_SITTING + GET_STAT(victim));
				CharWait(victim, PULSE_VIOLENCE);
			}
			break;
		case RIPPLE_PARA:
			act("&+CIce &+Wcrystals&n form around $N as &+Cintense cold&n causes $S body to &+Cfreeze.&n",
			    TRUE, victim, 0, victim, TO_ROOM);
			act("&+CIce &+Wcrystals&n form around you as &+Cintense cold&n causes your body to &+Cfreeze.&n",
			    TRUE, victim, 0, victim, TO_CHAR);
			if (spell_damage(ch, victim, (int)(0.4 * dam), SPLDAM_COLD, 0, &messages) !=
			    DAM_NONEDEAD)
			{
				return;
			}
			if (!NewSaves(victim, SAVING_PARA, number(-2, 2)) &&
			    !IS_GREATER_RACE(victim) &&
			    !check_freedom_of_movement(victim, number(0, 1)))
			{
				af.type = SPELL_MAJOR_PARALYSIS;
				af.flags = AFFTYPE_SHORT;
				af.bitvector2 = AFF2_MAJOR_PARALYSIS;
				af.duration = PULSE_VIOLENCE / 2;
				affect_to_char(victim, &af);
			}
			break;
		case RIPPLE_SWITCH:
			if (IS_PC(victim) || IS_PC_PET(victim))
			{
				tch = get_random_char_in_room(victim->in_room, victim,
							      DISALLOW_SELF);
				if (tch && on_front_line(tch) && victim->group == tch->group)
				{
					act("A glint of &+rm&+wa&+rd&+wd&+re&+wn&+ri&+wn&+rg&n touches $N's eyes\nas unseen powers corrupt $M.",
					    TRUE, victim, 0, victim, TO_ROOM);
					act("A glint of &+rm&+wa&+rd&+wd&+re&+wn&+ri&+wn&+rg&n touches your eyes\nas unseen powers corrupt you.",
					    TRUE, victim, 0, victim, TO_CHAR);
					attack(victim, tch);
				}
				break;
			}
			[[fallthrough]]; // Non-pet NPCs continue on to RIPPLE_BOLTS.
		case RIPPLE_BOLTS:
			messages.attacker = messages.room =
				"Twin &+Bbolts&n of writhing power slam into $N's chest.&n";
			messages.victim =
				"Twin &+Bbolts&n of writhing power slam into your chest.&n";

			if (spell_damage(ch, victim, (int)(.35 * dam), SPLDAM_GENERIC,
					 SPLDAM_NODEFLECT | SPLDAM_NOSHRUG,
					 &messages) != DAM_NONEDEAD)
			{
				return;
			}
			break;
		}
	}
}

void spell_anti_magic_ray(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			  P_obj obj)
{
	int dam, temp, save, noshrug;
	struct damage_messages messages = {
		"&+BReality &nseems to twist and bend as your &+Yray&n collides with $N's body!",
		"$n laughs maniacally as their &+Ldevastating &+Yray&n of pure &+Benergy &ncollides with your body!",
		"$n points at $N, and a powerful &+Yray&n of &+Lanti-&+Bmagic &nlances forth from their fingertip!",
		"Your &+Yray&n strips the very last bit of the magic called '&+CLife&n' from $N's body.",
		"You feel your very soul ceasing to be, as $n's &+Yray&n saps the last of the magic from your body.",
		"$N turns &+wpale&n, and suddenly collapses as $n's &+Yray&n saps the remainder of their lifeforce!",
		0
	};

	if (!IS_ALIVE(ch))
		return;

	temp = MIN(46, (level + 1));
	dam = dice(5 * temp, 10);

	if (saves_spell(victim, SAVING_SPELL))
		dam >>= 1;

	// This spell has a chance to be !shrug.
	if (number(0, 99) <= get_property("spell.shrug.chance.anti_magic_ray", 60))
		noshrug = SPLDAM_NOSHRUG;
	else
		noshrug = 0;

	if (spell_damage(ch, victim, dam, SPLDAM_GENERIC, noshrug, &messages) == DAM_NONEDEAD)
	{
		save = victim->specials.apply_saving_throw[SAVING_SPELL];
		victim->specials.apply_saving_throw[SAVING_SPELL] +=
			10 +
			(GET_LEVEL(ch) - 56); // 10-15 (roughly 50-75% increased chance to fail)
		spell_dispel_magic(level, ch, 0, SPELL_TYPE_SPELL, victim, obj);
		victim->specials.apply_saving_throw[SAVING_SPELL] = save;
	}
}

void spell_harmonic_resonance(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			      P_obj /*obj*/)
{
	if (!ch)
		return;

	send_to_room(
		"&+cA &+Cchorus &+cof &+Cresonant &+Wsound &+Cinteracts &+cwith your &+Csurroundings!\n",
		ch->in_room);
	switch (world[ch->in_room].sector_type)
	{
	case SECT_FOREST:
		LOOP_THRU_PEOPLE(victim, ch)
		{
			if (should_area_hit(ch, victim))
			{
				spell_cdoom(level, ch, 0, SPELL_TYPE_SPELL, victim, NULL);
			}
		}
		break;
	case SECT_FIELD:
		cast_call_lightning(level, ch, 0, SPELL_TYPE_SPELL, 0, NULL);
		break;
	case SECT_HILLS:
		spell_earthquake(level, ch, 0, SPELL_TYPE_SPELL, 0, NULL);
		break;
	case SECT_MOUNTAIN:
	case SECT_EARTH_PLANE:
		spell_earthen_rain(level, ch, 0, SPELL_TYPE_SPELL, 0, NULL);
		break;
	case SECT_SWAMP:
		LOOP_THRU_PEOPLE(victim, ch)
		{
			if (should_area_hit(ch, victim))
				spell_entangle(level, ch, 0, SPELL_TYPE_SPELL, victim, NULL);
		}
		break;
	case SECT_OCEAN:
	case SECT_WATER_SWIM:
	case SECT_WATER_NOSWIM:
	case SECT_WATER_PLANE:
		spell_miracle(level, ch, 0, SPELL_TYPE_SPELL, 0, NULL);
		break;
	case SECT_DESERT:
		spell_firestorm(level, ch, 0, SPELL_TYPE_SPELL, 0, NULL);
		break;
	case SECT_UNDERWATER:
		spell_tranquility(level, ch, 0, SPELL_TYPE_SPELL, 0, NULL);
		break;
	case SECT_NO_GROUND:
	case SECT_AIR_PLANE:
		LOOP_THRU_PEOPLE(victim, ch)
		{
			if (should_area_hit(ch, victim))
				spell_cyclone(level, ch, 0, SPELL_TYPE_SPELL, victim, NULL);
		}
		break;
	case SECT_FIREPLANE:
		spell_nova(level, ch, 0, SPELL_TYPE_SPELL, 0, NULL);
		break;
	case SECT_LAVA:
		spell_firestorm(level, ch, 0, SPELL_TYPE_SPELL, 0, NULL);
		break;
	}
}

void spell_recharger(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		     P_char victim, P_obj /*tar_obj*/)
{
	struct affected_type af;
	int mod, m_points;

	if (affected_by_spell(victim, SPELL_RECHARGER))
	{
		send_to_char("Nothing seems to happen.\n", ch);
		return;
	}
	if (victim == ch)
	{
		send_to_char("How can you recharge yourself, silly?\n", ch);
		return;
	}
	/*  if(resists_spell(ch, victim))
	   return;
	 */

	mod = (GET_LEVEL(ch) - GET_LEVEL(victim)) / 2;

	bzero(&af, sizeof(af));
	af.type = SPELL_RECHARGER;
	af.duration = (((mod > 0 ? mod : -mod) + 1));

	if (!NewSaves(victim, SAVING_PARA, mod))
	{
		send_to_char("You feel drained.\n", victim);
		send_to_char("You feel recharged.\n", ch);
		m_points = BOUNDED(0, (GET_MANA(victim) / 2 + mod * 2), GET_MANA(victim));

		victim->points.mana -= m_points;
		ch->points.mana += number(GET_LEVEL(ch) / 2, GET_LEVEL(ch)) * m_points / 100;

		affect_to_char(victim, &af);
	}
	else
	{ /*
	   * Muhahahaha!
	   */
		mod = -mod - 2;
		if (!NewSaves(ch, SAVING_SPELL, mod))
		{
			send_to_char("BACKFIRE!!  you feel your power draining away!\n", ch);
			send_to_char("You feel recharged!\n", victim);
			m_points = BOUNDED(0, (GET_MANA(ch) / 3 + mod), GET_MANA(ch));

			victim->points.mana += m_points;
			ch->points.mana -= m_points;

			af.duration = MAX(1, ((mod > 0 ? mod : -mod) - 1));
			affect_to_char(ch, &af);
		}
		else
		{
			send_to_char("You fail to drain any mana\n", ch);
			send_to_char("You feel a brief inner tug, which passes quickly.\n", victim);
		}
	}
}

void spell_disintegrate(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			P_obj obj)
{
	int i, dam, noshrug;
	struct damage_messages messages = {
		"You smile happily as your disintegration ray hits $N!",
		"Your body quivers and shakes as $n hits you with a disintegration ray, but you manage to stay together.",
		"$n cackles as $s disintegration ray strikes $N hard!",
		"You disintegrate $N into small bits!",
		"You scream as you fly into a million pieces. $n grins evilly.",
		"$n disintegrates $N into a pile of dust!",
		0
	};
	struct damage_messages eqburnmsg = {
		"$p turns red hot, $N screams as $p burns him.",
		"$p turns red hot and burns you!",
		"$p turns red hot and burns $N!",
		"$p turns red hot and burns the last bit of life out of $N!",
		"$p turns red hot and burns the last bit of life out of you!",
		"$p turns red hot and burns the last bit of life out of $N!",
		0
	};

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
	{
		return;
	}

	dam = dice(level, 13);

	act("$n sends a bright &+Ggreen ray of light&n streaking towards&n $N!", TRUE, ch, 0,
	    victim, TO_NOTVICT);
	act("$n sends a &+Ggreen ray of light&n coming .. straight towards YOU!", TRUE, ch, 0,
	    victim, TO_VICT);
	act("You grin evilly as you send a &+Ggreen beam of disintegration&n streaking towards&n $N!",
	    TRUE, ch, 0, victim, TO_CHAR);
	// Making Disintegrate !shrug -- Eikel.
	// if(resists_spell(ch, victim))
	//  return;
	// This spell has a chance to be !shrug.
	if (number(0, 99) <= get_property("spell.shrug.chance.disintegrate", 50))
		noshrug = SPLDAM_NOSHRUG;
	else
		noshrug = 0;

	if (!saves_spell(victim, SAVING_SPELL))
	{
		if (!IS_AFFECTED4(victim, AFF4_NEG_SHIELD) && !IS_UNDEADRACE(victim))
		{
			dam = (dam * 2);

			if (!IS_ROOM(victim->in_room, ROOM_ARENA))
			{
				i = 0;
				do
				{ /* could make this check the carried EQ as well...  */
					if (victim->equipment[i])
					{
						obj = victim->equipment[i];

						if (!(economic_gameplay_authority::active() &&
						      item_command_uses_durable_ownership(obj)) &&
						    !NewSaves(victim, SAVING_SPELL, -5) &&
						    !CHAR_IN_ARENA(victim) && !IS_ARTIFACT(obj) &&
						    !IS_NOSHOW(obj))
						{
							eqburnmsg.obj = obj;
							spell_damage(
								ch, victim,
								(int)get_property(
									"spell.disintegrate.burn.dmg",
									3),
								SPLDAM_FIRE, 0, &eqburnmsg);
							obj->condition -= BOUNDED(
								0,
								number(1,
								       (int)get_property(
									       "spell.disintegrate.max.eq.dmg",
									       10)),
								(obj->condition - 1));
							act("$p cracks from the heat.", FALSE, ch,
							    obj, victim, TO_VICT);
							/* Getting rid of this spammy useless message crap - Jexni 7/17/08
							statuslog(AVATAR, "%s just disintegrated %s from %s at [%d]", GET_NAME(ch),
							  obj->short_description, GET_NAME(victim), world[ch->in_room].number);

							act("$p turns red hot, $N screams, then it disappears in a puff of smoke!",
							   TRUE, ch, obj, victim, TO_CHAR);

							if( obj->loc.wearing || obj->loc.carrying)
							{
							  act("$p, held by $N, disappears in a puff of smoke!", TRUE, ch, obj, victim, TO_ROOM);
							}

							// remove the obj
							if(OBJ_CARRIED(obj))
							{
							  obj_from_char(obj);
							}
							else if(OBJ_WORN(obj))
							{
							  unequip_char_dale(obj);
							}
							else if(OBJ_INSIDE(obj))
							{
							  obj_from_obj(obj);
							}
							if(obj->contains)
							{
							  while (obj->contains)
							  {
							    x = obj->contains;
							    obj_from_obj(x);
							    obj_to_room(x, ch->in_room);
							  }
							}
							if(obj)
							{
							  extract_obj(obj);
							  obj = NULL;
							}
						  }
						  else
						  {
							if(obj)
							{
							  act("$p resists the disintegration ray completely!", TRUE, ch,
							      obj, victim, TO_VICT);
							  act("$p, carried by $N, resists the disintegration ray!", TRUE,
							      ch, obj, victim, TO_ROOM);
							}
						  */
						}
					}
					i++;
				} while (i < MAX_WEAR);
			}
		}
	} /* else dam = 0; */
	spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, noshrug, &messages);
}

void spell_shatter(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim, P_obj obj)
{
	int i, dam, savemod;

	struct damage_messages messages = {
		"&+C$N screams as $E is hit by your ghastly wave of sound!",
		"$n &+Cbombards you with a massive wave of sound. You feel as if your head will explode!",
		"&+CYou wince as $n &+Csends out an agonizing wave of sound at $N!",
		"&+LYour sound wave has utterly shattered $N's &+Linternal organs!",
		"&+LYou feel your internal organs burst as $n &+Lemits a vile sound.",
		"$N doubles over as $S internal organs burst and begin leaking out of $S body! $n snorts lewdly.",
		0
	};

	// Corresponds to approx bigbys + 10 damage saved (This is 9th circle vs bigbys 8th for Bards).
	dam = (10 * level) + dice(level, 3);
	// 92 - 100 pow has no bonus, < 92 pow means easier to save, > 100 pow means harder to save.
	savemod = STAT_INDEX(GET_C_POW(ch)) - 15;

	act("$n &+Lglares at $N&+L, and begins emanating a &+RHORRIBLE&+L sound!", TRUE, ch, 0,
	    victim, TO_NOTVICT);
	act("$n &+Lglares at you, and begins to emanate a deep, vicious sound!", TRUE, ch, 0,
	    victim, TO_VICT);
	act("&+LYou glare at $N&+L, and begin to generate a wretched sound of death!", TRUE, ch, 0,
	    victim, TO_CHAR);
	if (!NewSaves(victim, SAVING_SPELL, savemod))
	{
		if (!CHAR_IN_ARENA(ch) && !CHAR_IN_ARENA(victim))
		{
			i = 0;
			do
			{
				if (victim->equipment[i])
				{
					obj = victim->equipment[i];
					// Hits 2 out of 3 non-artifact eq'd items.
					if (number(0, 2) && !IS_ARTIFACT(victim->equipment[i]))
					{
						// 4% chance to destroy it outright.
						int destroy = !number(0, 24);

						DamageOneItem(victim, SPLDAM_SOUND, obj, destroy);
						if (destroy)
						{
							statuslog(
								AVATAR,
								"%s just shattered %s from %s at [%d]",
								GET_NAME(ch),
								obj->short_description,
								GET_NAME(victim),
								world[ch->in_room].number);
						}
					}
				}
				i++;
			} while (i < MAX_WEAR);
			// Could make this check the carried EQ as well...
		}
	}
	// If they saved
	else
	{
		dam = (dam * 2) / 3;
	}
	spell_damage(ch, victim, dam, SPLDAM_SOUND, SPLDAM_NOSHRUG, &messages);
}
