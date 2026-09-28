/* Area-owned special procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"
#include "combat/damage.h"
#include "combat/justice.h"
#include "combat/range.h"
#include "magic/spells.h"
#include <string.h>

int dryad(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	P_char vict, tmp_ch, next_vict_ch, next_tmp_ch;
	int InRoom, HasCharmies;
	bool princess = FALSE;
	char Gbuf1[MAX_STRING_LENGTH], Gbuf4[MAX_STRING_LENGTH];

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	Gbuf4[0] = 0;
	/*
	 * SAM 7-94 add check to see if pl exists
	 */
	if ((pl) && GET_MASTER(pl) == ch)
		switch (cmd)
		{
		case CMD_SCORE:
		case CMD_TELL:
		case CMD_SHOUT:
		case CMD_LOOK:
		case CMD_HELP:
		case CMD_WHO:
		case CMD_WEATHER:
		case CMD_SAVE:
		case CMD_QUIT:
		case CMD_TIME:
		case CMD_TOGGLE:
		case CMD_CHANNEL:
		case CMD_GCC:
		case CMD_COMMANDS:
		case CMD_ATTRIBUTES:
		case CMD_PETITION:
			break;
		default:
			send_to_char(
				"Your thoughts are too hazy, soley focused on this lovely forest maiden.\r\n",
				pl);
			send_to_char(
				"You can do nothing but stand here and tend to her every whim..\r\n",
				pl);
			return (TRUE);
			break;
		}
	if (pl)
		return (0);
	else
	{
		if (GET_VNUM(ch) == 5702)
			princess = TRUE;

		if (IS_FIGHTING(ch))
		{
			for (tmp_ch = world[ch->in_room].people; tmp_ch; tmp_ch = next_tmp_ch)
			{
				next_tmp_ch = tmp_ch->next_in_room;

				if (GET_MASTER(tmp_ch) == ch &&
				    MIN_POS(tmp_ch, POS_STANDING + STAT_SLEEPING))
				{
					for (vict = world[ch->in_room].people; vict;
					     vict = next_vict_ch)
					{
						next_vict_ch = vict->next_in_room;

						if (IS_FIGHTING(vict) && (vict != ch) &&
						    GET_MASTER(vict) != ch)
						{
							snprintf(
								Gbuf4, MAX_STRING_LENGTH,
								"The dryad screams at %s, 'Protect me slave!'",
								(IS_NPC(tmp_ch) ?
									 tmp_ch->player.short_descr :
									 GET_NAME(tmp_ch)));
							act(Gbuf4, FALSE, ch, 0, tmp_ch,
							    TO_NOTVICT);
							snprintf(
								Gbuf4, MAX_STRING_LENGTH,
								"The dryad screams at you, 'Protect me slave!'\r\n");
							send_to_char(Gbuf4, tmp_ch);
							snprintf(
								Gbuf4, MAX_STRING_LENGTH,
								"%s dives inbetween %s and the dryad, and takes up the fight!",
								(IS_NPC(tmp_ch) ?
									 tmp_ch->player.short_descr :
									 GET_NAME(tmp_ch)),
								(IS_NPC(vict) ?
									 vict->player.short_descr :
									 GET_NAME(vict)));
							act(Gbuf4, FALSE, ch, 0, 0, TO_NOTVICT);
							snprintf(
								Gbuf4, MAX_STRING_LENGTH,
								"You dive inbetween your dryad and %s, taking up the fight!\r\n",
								(IS_NPC(vict) ?
									 vict->player.short_descr :
									 GET_NAME(vict)));
							send_to_char(Gbuf4, tmp_ch);
							stop_fighting(vict);
							hit(tmp_ch, vict,
							    tmp_ch->equipment[PRIMARY_WEAPON]);
							return (TRUE);
						}
					}
				}
			}
			/*
			      LOOP_THRU_PEOPLE(tmp_ch, ch) {
			*/
			for (tmp_ch = world[ch->in_room].people; tmp_ch; tmp_ch = next_tmp_ch)
			{
				next_tmp_ch = tmp_ch->next_in_room;

				if ((tmp_ch != ch) && IS_FIGHTING(tmp_ch) &&
				    GET_MASTER(tmp_ch) != ch)
				{
					if (GET_SEX(tmp_ch) == SEX_MALE)
					{
						if (!NewSaves(tmp_ch, SAVING_SPELL,
							      princess ? 15 : 10))
						{
							act("The dryad utters an arcane phrase and throws her hands outwards.\r\n"
							    "The dryad's powerful spell springs forth and strikes you in the chest!\r\n"
							    "You feel yourself falling deep into a powerful trance, focused on the dryad.\r\n"
							    "The charm sets upon you fully... You feel completely entranced by her, \r\n"
							    "totally in love with her. You feel is if you would do Anything for her..\r\n"
							    "For you, this lovely dryad has now become the center of the universe... ",
							    FALSE, tmp_ch, 0, ch, TO_CHAR);
							act("The dryad utters a arcane phrase and throws her hands towards $N.\r\n"
							    "The dryad's powerful spell springs forth and strikes $N in the chest!\r\n"
							    "$N's eyes go blank as $E falls victim to the dryad's powerful charm... ",
							    FALSE, ch, 0, tmp_ch, TO_NOTVICT);
							stop_fighting(tmp_ch);
							stop_fighting(ch);
							if (tmp_ch->following)
								stop_follower(tmp_ch);
							add_follower(tmp_ch, ch);
							setup_pet(tmp_ch, ch,
								  24 * 18 * (princess ? 2 : 1), 0);
							if (princess)
								snprintf(
									Gbuf4, MAX_STRING_LENGTH,
									"An exceptionally beautiful dryad princess is here, tending to her slaves..\r\n");
							else
								snprintf(
									Gbuf4, MAX_STRING_LENGTH,
									"A beautiful dryad is standing here, tending to her slaves..\r\n");

							if ((ch->only.npc->str_mask &
							     STRUNG_DESC1) &&
							    ch->player.long_descr)
							{
								FREE(ch->player.long_descr);
							}
							ch->only.npc->str_mask |= STRUNG_DESC1;
							ch->player.long_descr =
								(char *)str_dup(Gbuf4);
							if (IS_NPC(tmp_ch))
							{
								snprintf(
									Gbuf4, MAX_STRING_LENGTH,
									"%s is standing here with a totally blank expression.\r\n",
									tmp_ch->player.short_descr);
								if ((tmp_ch->only.npc->str_mask &
								     STRUNG_DESC1) &&
								    tmp_ch->player.long_descr)
								{
									FREE(tmp_ch->player
										     .long_descr);
								}
								tmp_ch->only.npc->str_mask |=
									STRUNG_DESC1;
								tmp_ch->player.long_descr =
									(char *)str_dup(Gbuf4);
							}
							return (TRUE);
						}
						else
						{
							act("The dryad utters an arcane phrase and throws her hands outwards.\r\n"
							    "The dryad's powerful spell springs forth and strikes you in the chest!\r\n"
							    "You go blank for an instant, but resist the dryad's powerful charm.",
							    FALSE, ch, 0, tmp_ch, TO_CHAR);
							act("The dryad utters a arcane phrase and throws her hands towards $N.\r\n"
							    "The dryad's powerful spell springs forth and strikes $N in the chest!\r\n"
							    "$N goes black for an instant, but resists the dryad's powerful charm.",
							    FALSE, ch, 0, tmp_ch, TO_NOTVICT);
							return (FALSE);
						}
					}
				}
			}
		}
		else if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
		{
			HasCharmies = 0;
			/*      LOOP_THRU_PEOPLE(tmp_ch, ch) {*/
			for (tmp_ch = world[ch->in_room].people; tmp_ch; tmp_ch = next_tmp_ch)
			{
				next_tmp_ch = tmp_ch->next_in_room;

				if (GET_MASTER(tmp_ch) == ch)
				{
					HasCharmies = 1;
					if (number(0, 1) == 0)
					{
						strcpy(Gbuf4, "dryad");
						switch (dice(2, 17))
						{
						case 2:
							mobsay(ch, "Groom my hair, slave..");
							do_action(tmp_ch, Gbuf4, CMD_COMB);
							break;
						case 3:
							mobsay(ch,
							       "Massage me my prince, I desire it...");
							do_action(tmp_ch, Gbuf4, CMD_MASSAGE);
							break;
						case 4:
							mobsay(ch,
							       "Grovel to me my slave, show your subservience!");
							do_action(tmp_ch, Gbuf4, CMD_GROVEL);
							break;
						case 5:
							act("$n lets out a small provacative moan at you..",
							    TRUE, ch, 0, 0, TO_ROOM);
							do_action(tmp_ch, Gbuf4, CMD_UNDRESS);
							break;
						case 6:
							mobsay(ch,
							       "Mmmmmmmm my body feels so tense!");
							do_action(tmp_ch, Gbuf4, CMD_CARESS);
							break;
						case 7:
							do_action(ch, 0, CMD_PUCKER);
							do_action(tmp_ch, Gbuf4, CMD_FRENCH);
							break;
						case 8:
							mobsay(ch, "Do you love me, my slave?");
							do_action(tmp_ch, Gbuf4, CMD_LOVE);
							break;
						case 9:
							mobsay(ch,
							       "Do you wish to stay with me forever my slave?");
							mobsay(tmp_ch,
							       "Yes! Forever! I love you my beautiful princess!");
							do_action(tmp_ch, Gbuf4, CMD_DREAM);
							strcpy(Gbuf4, tmp_ch->player.name);
							do_action(ch, Gbuf4, CMD_KISS);
							break;
						case 10:
							mobsay(ch,
							       "I think that soon we will make love, my slave..");
							do_action(tmp_ch, Gbuf4, CMD_SEDUCE);
							break;
						case 11:
							mobsay(ch,
							       "Bathe me, my slave, I wish to feel clean..");
							do_action(tmp_ch, Gbuf4, CMD_BATHE);
							break;
						case 12:
							mobsay(ch, "Do you find me attractive?");
							mobsay(tmp_ch,
							       "Yes! You are the most beautiful woman I have ever seen!");
							do_action(tmp_ch, Gbuf4, CMD_UNDRESS);
							break;
						case 13:
							act("The dryads beauty nearly overwhealms you with passion!",
							    TRUE, ch, 0, 0, TO_ROOM);
							do_action(tmp_ch, Gbuf4, CMD_MELT);
							break;
						case 14:
							mobsay(ch,
							       "You must please me slave, or I shall cast you out!");
							do_action(tmp_ch, 0, CMD_SULK);
							do_action(tmp_ch, 0, CMD_CRY);
							break;
						case 15:
							mobsay(ch, "Prove your love to me slave!");
							do_action(tmp_ch, Gbuf4, CMD_OGLE);
							do_action(tmp_ch, Gbuf4, CMD_EMBRACE);
							do_action(tmp_ch, 0, CMD_WHIMPER);
							strcpy(Gbuf4, tmp_ch->player.name);
							do_action(ch, Gbuf4, CMD_FLUTTER);
							break;
						case 16:
							do_action(tmp_ch, Gbuf4, CMD_ROSE);
							strcpy(Gbuf4, tmp_ch->player.name);
							do_action(ch, Gbuf4, CMD_PAT);
							break;
						case 17:
							mobsay(ch,
							       "Hmmmmm, I prefer my slaves to remain naked.. Disrobe for me.");
							do_action(ch, 0, CMD_GRIN);
							strcpy(Gbuf4, "all");
							do_remove(tmp_ch, Gbuf4, 0);
							strcpy(Gbuf4, "dryad");
							do_action(tmp_ch, Gbuf4, CMD_SEDUCE);
							break;
						default:
							break;
						}
					}
				}
			}
			if (HasCharmies == 1)
			{
				InRoom = world[ch->in_room].number;
				if ((princess && (InRoom != 5744)) ||
				    (!princess && ((InRoom < 5733) || (InRoom > 5744))))
				{
					mobsay(ch, "Come my slave, let us go to my hidden abode.");
					act("The dryad utters an arcane magical phrase.", TRUE, ch,
					    0, 0, TO_ROOM);
					act("$n disappears in a blinding flash of light!", FALSE,
					    ch, 0, tmp_ch, TO_NOTVICT);
					act("FOOOOOOOOOOSH! With a flash of light, you are instantly teleported!",
					    TRUE, ch, 0, 0, TO_ROOM);
					snprintf(
						Gbuf4, MAX_STRING_LENGTH,
						"A beautiful dryad slowly fades into exsistance!\r\n");
					LOOP_THRU_PEOPLE(tmp_ch, ch)
						if (GET_MASTER(tmp_ch) == ch)
							snprintf(
								Gbuf1, MAX_STRING_LENGTH,
								"%s slowly fades into existance, standing obediently behind the dryad.\r\n",
								(IS_NPC(tmp_ch) ?
									 tmp_ch->player.short_descr :
									 GET_NAME(tmp_ch)));
					strcat(Gbuf4, Gbuf1);
					send_to_room(Gbuf4,
						     princess ? real_room(5744) : real_room(5739));
					/*          LOOP_THRU_PEOPLE(tmp_ch, ch) {*/
					for (tmp_ch = world[ch->in_room].people; tmp_ch;
					     tmp_ch = next_tmp_ch)
					{
						next_tmp_ch = tmp_ch->next_in_room;

						if (GET_MASTER(tmp_ch) == ch)
						{
							act("$N disappears in a blinding flash of light!",
							    FALSE, ch, 0, tmp_ch, TO_NOTVICT);
							char_from_room(tmp_ch);
							char_to_room(tmp_ch,
								     princess ? real_room(5744) :
										real_room(5739),
								     -1);
						}
					}
					char_from_room(ch);
					char_to_room(ch,
						     princess ? real_room(5744) : real_room(5739),
						     -1);
				}
			}
			else
			{
				if (princess)
					snprintf(
						Gbuf4, MAX_STRING_LENGTH,
						"An exceptionally beautiful dryad princess is here, observing you quietly.\r\n");
				else
					snprintf(
						Gbuf4, MAX_STRING_LENGTH,
						"A beautiful dryad is standing here, observing you shyly.\r\n");
				if ((ch->only.npc->str_mask & STRUNG_DESC1) &&
				    ch->player.long_descr)
				{
					FREE(ch->player.long_descr);
				}
				ch->only.npc->str_mask |= STRUNG_DESC1;
				ch->player.long_descr = (char *)str_dup(Gbuf4);
				switch (dice(3, 8))
				{
				case 3:
					act("$n looks at you, both shy and nervous.", TRUE, ch, 0,
					    0, TO_ROOM);
					break;
				case 4:
					mobsay(ch, "May you go in peace throgh our forest...");
					break;
				case 5:
					act("$n sings a beautiful song that is filled with soft tones.",
					    TRUE, ch, 0, 0, TO_ROOM);
					break;
				case 6:
					act("$n smiles at you.", TRUE, ch, 0, 0, TO_ROOM);
					break;
				case 7:
					mobsay(ch, "Welcome traveller.");
					act("$n smiles.", TRUE, ch, 0, 0, TO_ROOM);
					[[fallthrough]];
				case 8:
					act("$n keeps a wary eye on you.", TRUE, ch, 0, 0, TO_ROOM);
					break;
				default:
					break;
				}
			}
		}
	}
	return (FALSE);
}

/*
 * code to allow mob to sail ship cannot be defined in spec proc because
 * mobile spec proc is called only once every 40 pulses.  Mobs need to
 * order ship to sailin 10 pulses interval. -DCL
 */

int navagator(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	int realms_helpers[] = { 11102, 11103, 11104, 11105, 11107, 11108, 11110, 11112, 11114, 0 };
	int silver_helpers[] = { 11102, 11103, 11104, 11105, 11107, 11108, 11110, 11112, 11114, 0 };
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	/*
	 * check for shout_and_hunt() stuff on ship navigators
	 */

	if (!pl)
	{
		if (GET_VNUM(ch) == 11101) /*
		                            * realms master
		                            */
			return shout_and_hunt(ch, 30, "Help me mates!  We be under attack by %s!",
					      NULL, realms_helpers, 0, 0);
		if (GET_VNUM(ch) == 11301) /*
		                            * silver lady
		                            */
			return shout_and_hunt(ch, 30, "Help me mates!  We be under attack by %s!",
					      NULL, silver_helpers, 0, 0);
		return FALSE;
	}
	if ((cmd != CMD_ORDER) || (ch == pl))
		return FALSE;

	act("$n growls at $N, 'only I can order my ship to sail!'", FALSE, ch, 0, pl, TO_NOTVICT);
	act("$n growls at you, 'only I can order my ship to sail!'", FALSE, ch, 0, pl, TO_VICT);
	return TRUE;
}

int sword_named_magik(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	P_char vict;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!dam || !IS_ALIVE(ch) || !OBJ_WORN_POS(obj, WIELD))
	{
		return FALSE;
	}

	vict = legacy_proc_arg<P_char>(arg);

	if (!IS_ALIVE(vict))
	{
		return FALSE;
	}

	if (OBJ_WORN_BY(obj, ch))
	{
		// 1/30 chance.
		if (!number(0, 29))
		{
			act("&+BYour $q engulfs $N  in its bright &+bblue &+Baura!&N", FALSE,
			    obj->loc.wearing, obj, vict, TO_CHAR);
			act("&+B$n's $q engulfs $N  in its bright &+bblue &+Baura!&N", FALSE,
			    obj->loc.wearing, obj, vict, TO_ROOM);
			spell_dispel_magic(30, ch, 0, SPELL_TYPE_SPELL, vict, 0);
		}
		else
		{
			if (!GET_OPPONENT(ch))
			{
				set_fighting(ch, vict);
			}
		}
	}
	if (GET_OPPONENT(ch))
	{
		return (FALSE);
	}
	else
	{
		return (TRUE);
	}
}
