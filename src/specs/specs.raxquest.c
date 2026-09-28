/* Special procedures for the Raxquest pirate area. */

#include "core/prototypes.h"
#include <time.h>
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/handler.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"
#include "world/vnum.obj.h"

extern P_char character_list;

#define LONGJOHNSILVER_HELPER_LIMIT 4
#undef LONGJOHNSILVERHELPERLIMIT

int undead_dragon_east(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	int allowed = 0;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	allowed = 0;

	if (!ch)
		return 0;

	if (!pl)
		return 0;

	if (!(cmd == CMD_EAST))
		return 0;

	if (IS_TRUSTED(pl))
		allowed = 1;
	else
		allowed = 0;

	if (allowed)
	{
		act("$N nods, stands aside and lets $n pass.", FALSE, pl, 0, ch, TO_ROOM);
		act("$N nods and stands aside to let you pass.", FALSE, pl, 0, ch, TO_CHAR);
		return (FALSE);
	}
	/*
	 * BLOCK!
	 */
	act("$N &+RROARS&+L at you while quickly blocking the exit!&n.", FALSE, pl, 0, ch, TO_CHAR);
	act("$N &+RROARS&+L while quickly blocking the exit!&n.", FALSE, pl, 0, ch, TO_NOTVICT);
	return (TRUE);
}

/* this is hack n slashed from the pet baby dragon in dragonnia */
int undead_parrot(P_char ch, P_char pl, int cmd, char *arg)
{
	char Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	/*
	 * check for periodic event call
	 */

	if (cmd && pl)
	{
		if (pl == ch)
			return FALSE;
		switch (cmd)
		{
		case CMD_PET:
			do_action(pl, arg, CMD_PET);
			if (isname(arg, ch->player.name))
			{
				if (ch->following)
					stop_follower(ch);
				add_follower(ch, pl);
				group_add_member(pl, ch);
				act("$n says 'SQWAK!'.", 1, ch, 0, pl, TO_VICT);
			}
			return TRUE;
			break;
		case 25: /*
			              kill
			            */
		case 70: /*
			              hit
			            */
		case 154: /*
			              back stab, bash, kick
			            */
		case 157:
		case 159:
		case 236: /*
			              murder
			            */
			one_argument(arg, Gbuf1);
			if ((ch == get_char_room(Gbuf1, pl->in_room)) && (pl == ch->following))
			{
				strcpy(Gbuf2, pl->player.name);
				/* add parrot ansi later */
				act("&+LA friendly &+wsk&+Lele&+wtal &+Gp&+Ra&+Gr&+Rr&+Go&+Rt&N says 'SQWAK!' and flitters away for a moment.",
				    1, ch->following, 0, ch, TO_ROOM);
				act("&+LA friendly &+wsk&+Lele&+wtal &+Gp&+Ra&+Gr&+Rr&+Go&+Rt&N says 'SQWAK!' and flitters away for a moment.",
				    1, ch->following, 0, ch, TO_CHAR);
				return TRUE;
			}
			break;
		default:
			return FALSE;
			break;
		}

		if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch))
			return FALSE;

		switch (number(0, 100))
		{
		case 0:
			act("$n says 'SQUAWK!", TRUE, ch, 0, 0, TO_ROOM);
			return TRUE;
		case 1:
			act("$n says 'Betcha Didn't Know I could talk.", TRUE, ch, 0, 0, TO_ROOM);
			return TRUE;
		case 2:
			act("$n says 'SQUAWK!", TRUE, ch, 0, 0, TO_ROOM);
			return TRUE;
		case 3:
			act("$n says 'SQUAWK!", TRUE, ch, 0, 0, TO_ROOM);
			return TRUE;
		case 4:
			act("$n says 'SQUAWK!", TRUE, ch, 0, 0, TO_ROOM);
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

int long_john_silver_shout(P_char ch, P_char /*tch*/, int cmd, char * /*arg*/)
{
	/* variables for summon proc */
	P_char i;
	P_char ljswraith;
	P_char vict;
	int count = 0;

	/* variables for shout proc */
	int helpers[] = { 70536, 70537, 70538, 70539, 70540, 70541, 70547, 70548, 0 };

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch)
	{
		return FALSE;
	}

	if (cmd != 0)
	{
		return FALSE;
	}

	/* summon proc */
	if (IS_FIGHTING(ch))
	{
		/*
		 * attempt to "summon" a wraith pirate...only possible if less than LONGJOHNSILVER_HELP_LIMIT
		 * * in world
		 */
		for (i = character_list; i; i = i->next)
		{
			if ((IS_NPC(i)) && (GET_VNUM(i) == 70536))
			{
				count++;
			}
		}
		if (count < LONGJOHNSILVER_HELPER_LIMIT)
		{
			if (number(1, 100) < 50)
			{
				ljswraith = read_mobile(70536, VIRTUAL);
				if (!ljswraith)
				{
					logit(LOG_EXIT, "assert: error in longjohnsilver() proc");
					wizlog(MINLVLIMMORTAL, "error in proc longjohnsilver");
					return FALSE;
				}
				act("$n &+Lraises his hands in the air..&n\r\n"
				    "&+LThe wraith of a pirate appears out of thin air!&n\r\n",
				    FALSE, ch, 0, ljswraith, TO_ROOM);
				char_to_room(ljswraith, ch->in_room, 0);
				vict = GET_OPPONENT(ch); /* lets make our pets fight something! */
				MobStartFight(ljswraith, vict);
				return TRUE;
			}
			else /* if can't summon, may as well yell for help */
				return shout_and_hunt(
					ch, 100,
					"&+LArr! All hands come slay the intruder &+W%s &+LArr!&N",
					NULL, helpers, 0, 0);
		}
	}

	return FALSE;
}

int pirate_talk(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		mobsay(ch, "Arr! This be my vessel, begone!");
		do_action(ch, 0, CMD_STARE);
		return TRUE;
	case 1:
		mobsay(ch, "You shall meet the sharp edge of my blade Arr!");
		return TRUE;
	case 2:
		mobsay(ch, "Yo Ho Ho and a Bottle of Rum");
		do_action(ch, 0, CMD_HICCUP);
		return TRUE;
	case 3:
		act("$n stumbles around bumping into things.", TRUE, ch, 0, 0, TO_ROOM);
		do_action(ch, 0, CMD_HICCUP);
		return TRUE;
	case 4:
		mobsay(ch, "A pirates life for me... A pirates life for me..");
		do_action(ch, 0, CMD_SMILE);
		return TRUE;
	}
	return FALSE;
}

int pirate_female_talk(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		mobsay(ch, "Arr! Get outta my room!");
		do_action(ch, 0, CMD_STARE);
		return TRUE;
	case 1:
		mobsay(ch, "Arr! I'm goin' to kick your ass!");
		return TRUE;
	case 2:
		mobsay(ch, "You think you've had a bad day? Just look at my skin! Arr!");
		do_action(ch, 0, CMD_CRY);
		return TRUE;
	case 3:
		mobsay(ch, "I haven't had a REAL Jolly Roger for years...");
		do_action(ch, 0, CMD_MOAN);
		return TRUE;
	case 4:
		mobsay(ch, "A pirates life for me... A pirates life for me..");
		do_action(ch, 0, CMD_SMILE);
		return TRUE;
	case 5:
		act("$n looks at you and giggles.", TRUE, ch, 0, 0, TO_ROOM);
		mobsay(ch, "I'm going to have to borrow the Lookout's telescope for this one.");
		do_action(ch, 0, CMD_LAUGH);
		return TRUE;
	}
	return FALSE;
}

int pirate_cabinboy_talk(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		mobsay(ch, "Is the poop deck really what I think it is?");
		do_action(ch, 0, CMD_PUZZLE);
		return TRUE;
	case 1:
		act("$n sings 'In the Navy!", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 2:
		mobsay(ch, "Yo Ho Ho and a Bottle of Milk");
		return TRUE;
	case 3:
		act("$n air fences with his broom.", TRUE, ch, 0, 0, TO_ROOM);
		act("$n pokes himself in the eye.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 4:
		mobsay(ch, "A cabin boy's life for me... A cabin boy's life for me..");
		return TRUE;
	case 5:
		mobsay(ch,
		       "Swab this.. Swab that.... If he tells me to swab one more thing I'll swab HIM!");
		do_action(ch, 0, CMD_GROWL);
		return TRUE;
	}
	return FALSE;
}

/* Raxquest item procedures. */

int circlet_of_light(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char vict;
	int in_battle;

	vict = legacy_proc_arg<P_char>(arg);

	in_battle = cmd / 1000;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!ch && cmd == CMD_PERIODIC)
	{
		hummer(obj);
		return TRUE;
	}

	if (!IS_ALIVE(ch) || !OBJ_WORN(obj) || !OBJ_WORN_POS(obj, WEAR_HEAD) ||
	    !OBJ_WORN_BY(obj, ch))
	{
		return FALSE;
	}

	if (arg && (cmd == CMD_SAY))
	{
		if (isname(arg, "beblessed"))
		{
			act("&+wYou raise your hands in the air...&N\n", FALSE, ch, 0, 0, TO_CHAR);
			act("&+wYou send out a st&+Wre&+Cam &+wof &+Chealing &+Wenergies &+wto aid all those who are injured.&N",
			    FALSE, ch, obj, obj, TO_CHAR);
			act("$n&+w raises $s hands in the air...\n", TRUE, ch, obj, NULL, TO_ROOM);
			act("&+w$n sends out a st&+Wre&+Cam &+wof &+Chealing &+Wenergies &+wto aid all those who are injured.&N",
			    TRUE, ch, obj, NULL, TO_ROOM);

			spell_mass_heal(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
			return TRUE;
		}
		else if (isname(arg, "mystra"))
		{
			act("You whisper 'mystra' to your $q", FALSE, ch, 0, vict, TO_CHAR);
			act("&+LThe sky darkens and fills with clouds.  You hear a deep rumbling&N",
			    FALSE, ch, obj, obj, TO_CHAR);
			act("&+Lin the distance.  The rumbling gets louder and louder.  God himself&N",
			    FALSE, ch, obj, obj, TO_CHAR);
			act("&+Lappears infront of you. &+WHOW DARE YOU SPEAK THAT VILE NAME?!&N",
			    FALSE, ch, obj, obj, TO_CHAR);
			act("$n whispers 'mystra' to $q.", FALSE, ch, obj, obj, TO_ROOM);
			act("&+LThe sky darkens and fills with clouds.  You hear a deep rumbling&N",
			    TRUE, ch, obj, vict, TO_ROOM);
			act("&+Lin the distance.  The rumbling gets louder and louder.  God himself&N",
			    TRUE, ch, obj, vict, TO_ROOM);
			act("&+Lappears infront of you. &+WHOW DARE YOU SPEAK THAT VILE NAME?!&N",
			    TRUE, ch, obj, vict, TO_ROOM);
			spell_lightning_bolt(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
			return TRUE;
		}
	}

	// Past here, and you're fighting.. 1/15 chance.
	if (!in_battle || !number(0, 14))
	{
		return FALSE;
	}

	act("&+BYour $q flashes with lightning as a bolt fires out at $N.", FALSE, obj->loc.wearing,
	    obj, vict, TO_CHAR);
	act("$n's $q &+Bradiates a bolt of lightning out at you.", FALSE, obj->loc.wearing, obj,
	    vict, TO_VICT);
	act("$n's $q &+Bradiates a blot of lightning at $N.", FALSE, obj->loc.wearing, obj, vict,
	    TO_NOTVICT);
	spell_lightning_bolt(61, ch, 0, SPELL_TYPE_SPELL, vict, 0);
	if (!IS_ALIVE(ch) || !IS_ALIVE(vict))
	{
		return TRUE;
	}
	return FALSE;
}

/* long john silver's sword */
int ljs_sword(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch) ||
	    !(victim = legacy_proc_arg<P_char>(arg)))
	{
		return FALSE;
	}

	// 1/30 chance.
	if (!IS_ALIVE(victim) || number(0, 29))
	{
		return FALSE;
	}
	act("&+W$n's&N $q &+Lfl&+rar&+Res &+Lup with darkness...&N", TRUE, ch, obj, victim,
	    TO_NOTVICT);
	act("&+WYour&N $q &+Lfl&+rar&+Res &+Lup with darkness...&N", TRUE, ch, obj, victim,
	    TO_CHAR);
	act("&+W$n's&N $q &+Lfl&+rar&+Res &+Lup with darkness...&N", TRUE, ch, obj, victim,
	    TO_VICT);
	/* This is just a temporary spell, the spell will probably be made something else later, but just getting it to work for now! */
	spell_greater_spirit_anguish(25, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
	return (TRUE);
}

/* WUSS SWORD hehe */

int wuss_sword(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	// 1/30 chance.
	if (!dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch) ||
	    !(victim = legacy_proc_arg<P_char>(arg)) || number(0, 29))
	{
		return FALSE;
	}
	if (!IS_ALIVE(victim))
	{
		return FALSE;
	}

	act("&+W$n's&N $q &+Wglows &+Ybrightly&+w...&N", TRUE, ch, obj, victim, TO_NOTVICT);
	act("&+WYour&N $q &+Wglows &+Ybrightly&+w...&N", TRUE, ch, obj, victim, TO_CHAR);
	act("&+W$n's&N $q &+Wglows &+Ybrightly&+w...&N", TRUE, ch, obj, victim, TO_VICT);
	spell_magic_missile(1, ch, NULL, 0, victim, obj);
	return TRUE;
}

int head_guard_sword(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch) ||
	    !(victim = legacy_proc_arg<P_char>(arg)) || number(0, 29))
	{
		return FALSE;
	}
	if (!IS_ALIVE(victim))
	{
		return FALSE;
	}

	act("&+W$n's&N $q &+rB&+RU&+YRS&+RT&+rS &+Linto &+rfl&+Ram&+Yes&+L...&N", TRUE, ch, obj,
	    victim, TO_NOTVICT);
	act("&+WYour&N $q &+rB&+RU&+YRS&+RT&+rS &+Linto &+rfl&+Ram&+Yes&+L...&N", TRUE, ch, obj,
	    victim, TO_CHAR);
	act("&+W$n's&N $q &+rB&+RU&+YRS&+RT&+rS &+Linto &+rfl&+Ram&+Yes&+L...&N", TRUE, ch, obj,
	    victim, TO_VICT);
	spell_magma_burst(30, ch, NULL, 0, victim, obj);
	return TRUE;
}

int alch_rod(P_obj obj, P_char ch, int cmd, char *arg)
{
	int rand;
	int dam = cmd / 1000;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch) ||
	    !(victim = legacy_proc_arg<P_char>(arg)))
	{
		return FALSE;
	}
	// 1/30 chance.
	if (!IS_ALIVE(victim) || number(0, 29))
	{
		return FALSE;
	}

	act("&+W$n's&N $q &+Lcalls upon &+cmy&+Cst&+Wi&+Cca&+cl &+rpo&+Rwe&+rrs&+L...&N", TRUE, ch,
	    obj, victim, TO_NOTVICT);
	act("&+WYour&N $q &+Lcalls upon &+cmy&+Cst&+Wi&+Cca&+cl &+rpo&+Rwe&+rrs&+L...&N", TRUE, ch,
	    obj, victim, TO_CHAR);
	act("&+W$n's&N $q &+Lcalls upon &+cmy&+Cst&+Wi&+Cca&+cl &+rpo&+Rwe&+rrs&+L...&N", TRUE, ch,
	    obj, victim, TO_VICT);
	rand = number(0, 1);
	switch (rand)
	{
	case 0:
		spell_living_stone(30, ch, 0, 0, victim, obj);
		break;
	case 1:
		spell_shadow_monster(30, ch, NULL, 0, victim, obj);
		break;
	}
	return TRUE;
}

int dragon_skull_helm(P_obj obj, P_char ch, int cmd, char * /*argument*/)
{
	int rand;
	int curr_time;
	P_char victim = NULL;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd != CMD_PERIODIC || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (ch != obj->loc.wearing) ||
	    !OBJ_WORN_POS(obj, WEAR_HEAD))
	{
		return FALSE;
	}

	curr_time = time(NULL);
	if (!IS_ROOM(ch->in_room, ROOM_NO_MAGIC))
	{
		// 1 min timer.
		if (obj->timer[0] + 60 <= curr_time)
		{
			obj->timer[0] = curr_time;
			rand = number(0, 7);
			switch (rand)
			{
			case 0:
				act("&+LYour $q&+L causes your eyes to &+Rg&+rlo&+Rw&+L.&n", TRUE,
				    ch, obj, victim, TO_CHAR);
				spell_infravision(60, ch, 0, 0, ch, 0);
				break;
			case 1:
				act("&+LYour $q&+L causes your eyes to &+Wglow&+L.&n", TRUE, ch,
				    obj, victim, TO_CHAR);
				spell_detect_invisibility(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
				break;
			case 2:
				act("&+LYour $q&+L causes your eyes to &+Yglow&+L.&n", TRUE, ch,
				    obj, victim, TO_CHAR);
				spell_detect_good(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
				break;
			case 3:
				act("&+LYour $q&+L causes your eyes to &+wglow&+L.&n", TRUE, ch,
				    obj, victim, TO_CHAR);
				spell_farsee(60, ch, NULL, 0, ch, 0);
				break;
			case 4:
				act("&+LYour $q&+L causes your eyes to &+Lglow&+L.&n", TRUE, ch,
				    obj, victim, TO_CHAR);
				spell_sense_life(60, ch, 0, 0, ch, 0);
				break;
			case 5:
				act("&+LYour $q&+L causes your eyes to &+Wg&+wlo&+Ww&+L.&n", TRUE,
				    ch, obj, victim, TO_CHAR);
				spell_hawkvision(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
				break;
			case 6:
				act("&+LYour $q&+L causes your eyes to &+rglow&+L.&n", TRUE, ch,
				    obj, victim, TO_CHAR);
				spell_detect_evil(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
				break;
			case 7:
				act("&+LYour $q&+L causes your eyes to &+Bglow&+L.&n", TRUE, ch,
				    obj, victim, TO_CHAR);
				spell_detect_magic(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
				break;
			}
			return FALSE;
		}
		if (IS_FIGHTING(ch) && number(0, 1))
		{
			victim = GET_OPPONENT(ch);
			if (!IS_ALIVE(victim))
			{
				return FALSE;
			}
			if (IS_UNDEAD(victim))
			{
				act("&+L$n's $q &+L causes &+W$m &+Leyes to &+Rp&+ru&+Rl&+rs&+Re &+Lwith &+Bincredible &+Wenergy&+L.",
				    TRUE, ch, obj, victim, TO_NOTVICT);
				act("&+LYour $q &+L causes your eyes to &+Rp&+ru&+Rl&+rs&+Re &+Lwith &+Bincredible &+Wenergy&+L.",
				    TRUE, ch, obj, victim, TO_CHAR);
				act("&+L$n's $q &+L causes &+W$n &+Leyes to &+Rp&+ru&+Rl&+rs&+Re &+Lwith &+Bincredible &+Wenergy&+L.",
				    TRUE, ch, obj, victim, TO_VICT);
				spell_destroy_undead(60, ch, NULL, 0, victim, 0);
			}
		}
	}
	return FALSE;
}

int priest_rudder(P_obj obj, P_char ch, int cmd, char * /*argument*/)
{
	int curr_time;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd != CMD_PERIODIC || !OBJ_WORN(obj))
	{
		return FALSE;
	}

	ch = obj->loc.wearing;

	if (!IS_ALIVE(ch))
	{
		return FALSE;
	}

	curr_time = time(NULL);
	// 2 min timer.
	if (obj->timer[0] + 120 <= curr_time)
	{
		act("&+L$n's $q &+chums &+wsoftly&+L.&N", FALSE, ch, obj, 0, TO_ROOM);
		act("&+LYour $q &+chums &+wsoftly&+L.&N", FALSE, ch, obj, 0, TO_CHAR);
		if (!IS_ROOM(ch->in_room, ROOM_NO_MAGIC))
		{
			spell_mass_heal(35, ch, 0, 0, ch, 0);
		}
		obj->timer[0] = curr_time;
	}

	return FALSE;
}

int ljs_armor(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	P_char vict;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd != CMD_PERIODIC)
	{
		return FALSE;
	}

	if (!OBJ_WORN(obj) || !OBJ_WORN_POS(obj, WEAR_BODY))
	{
		return FALSE;
	}

	ch = obj->loc.wearing;
	if (!IS_ALIVE(ch))
	{
		return FALSE;
	}
	vict = GET_OPPONENT(ch);
	if (!IS_ALIVE(vict))
	{
		return FALSE;
	}

	// 1/10 chance.
	if (IS_FIGHTING(ch) && !number(0, 9))
	{
		if (IS_UNDEAD(vict) && !IS_UNDEAD(ch))
		{
			act("&+L$n's&N $q &+Lgrows a &+wtough &+Llayer of skin.&N", TRUE, ch, obj,
			    vict, TO_NOTVICT);
			act("&+LYour&N $q &+Lgrows a &+wtough &+Llayer of skin.&N", TRUE, ch, obj,
			    vict, TO_CHAR);
			act("&+L$n's&N $q &+Lgrows a &+wtough &+Llayer of skin.&N", TRUE, ch, obj,
			    vict, TO_VICT);
			if (!IS_ROOM(ch->in_room, ROOM_NO_MAGIC))
			{
				spell_prot_from_undead(60, ch, 0, 0, ch, 0);
			}
			return TRUE;
		}
		if (!IS_UNDEAD(vict))
		{
			act("&+w$n's&N $q &+Lgrows a &+Wtough &+Llayer of skin.&N", TRUE, ch, obj,
			    vict, TO_NOTVICT);
			act("&+wYour&N $q &+Lgrows a &+Wtough &+Llayer of skin.&N", TRUE, ch, obj,
			    vict, TO_CHAR);
			act("&+w$n's&N $q &+Lgrows a &+Wtough &+Llayer of skin.&N", TRUE, ch, obj,
			    vict, TO_VICT);
			if (!IS_ROOM(ch->in_room, ROOM_NO_MAGIC))
			{
				spell_protection_from_living(60, ch, 0, 0, ch, 0);
			}
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

int alch_bag(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	int curr_time;
	P_obj ingred;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd != CMD_PERIODIC)
	{
		return FALSE;
	}

	if (!IS_ALIVE(ch) || !OBJ_WORN(obj) || ch != obj->loc.wearing)
	{
		return FALSE;
	}

	if (!OBJ_WORN_POS(obj, WEAR_ATTACH_BELT_1) && !OBJ_WORN_POS(obj, WEAR_ATTACH_BELT_2) &&
	    !OBJ_WORN_POS(obj, WEAR_ATTACH_BELT_3))
	{
		return FALSE;
	}

	curr_time = time(NULL);
	if (obj->timer[0] + 60 + number(0, 30) <= curr_time)
	{
		/* if this somehow works, it is a MIRACLE as I had nothing to copy from */
		/* should add a full set of ingredients into the pouch! */
		/* obj #'s to use....... 821, 822, 823, 824, 825, 826, 827, 839 -Raxxel*/

		/* begin experimental code! */
		obj->timer[0] = curr_time;

		// nightshade
		ingred = read_object(VOBJ_FORAGE_NIGHTSHADE, VIRTUAL);
		if (!ingred)
		{
			return FALSE;
		}
		obj_to_obj(ingred, obj);

		// mandrake
		ingred = read_object(VOBJ_FORAGE_MANDRAKE, VIRTUAL);
		if (!ingred)
		{
			return FALSE;
		}
		obj_to_obj(ingred, obj);

		// garlic
		ingred = read_object(VOBJ_FORAGE_GARLIC, VIRTUAL);
		if (!ingred)
		{
			return FALSE;
		}
		obj_to_obj(ingred, obj);

		// faerie dust
		ingred = read_object(VOBJ_FORAGE_FAERIE_DUST, VIRTUAL);
		if (!ingred)
		{
			return FALSE;
		}
		obj_to_obj(ingred, obj);

		// dragons blood
		ingred = read_object(VOBJ_FORAGE_DRAGON_BLOOD, VIRTUAL);
		if (!ingred)
		{
			return FALSE;
		}
		obj_to_obj(ingred, obj);

		// green herb
		ingred = read_object(VOBJ_FORAGE_GREEN_HERB, VIRTUAL);
		if (!ingred)
		{
			return FALSE;
		}
		obj_to_obj(ingred, obj);

		// strange stone
		ingred = read_object(VOBJ_FORAGE_STRANGE_STONE, VIRTUAL);
		if (!ingred)
		{
			return FALSE;
		}
		obj_to_obj(ingred, obj);

		// empty potion bottle
		ingred = read_object(VOBJ_POTION_BOTTLES, VIRTUAL);
		if (!ingred)
		{
			return FALSE;
		}
		obj_to_obj(ingred, obj);
		/* end experimental code! */

		/* let those who need to know, know ya just finished doing something spiffy */
		act("&+L$n's&N $q &+rp&+Mu&+Rl&+Ms&+res &+Lslightly and &+wgrows &+La little larger.&N",
		    FALSE, ch, obj, 0, TO_ROOM);
		act("&+LYour$N $q &+rp&+Mu&+Rl&+Ms&+res &+Lslightly and &+wgrows &+La little larger.&N",
		    FALSE, ch, obj, 0, TO_CHAR);
	}
	return FALSE;
}
