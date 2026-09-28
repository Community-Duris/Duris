/*
  Procs for the human hometown tharnadia
 */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "combat/range.h"
#include "core/utils.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "world/specs.prototypes.h"
#include "magic/spells.h"

/*
  external vars
 */

extern P_char character_list;
extern P_desc descriptor_list;
extern P_index obj_index;
extern P_room world;
extern char *coin_names[];
extern const struct stat_data stat_factor[];
extern int pulse;
extern int top_of_zone_table;
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;

int tharn_tall_merchant(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (IS_FIGHTING(ch))
		return FALSE;

	if (pl)
	{
		return (0);
	}
	else
	{
		if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
		{
			switch (number(2, 10))
			{
			case 2:
				act("$n looks you up and down judging your wealth.", TRUE, ch, 0, 0,
				    TO_ROOM);
				mobsay(ch, "Anything you wanna buy, trade or sell?");
				act("$n rubs his hands together greedily.", TRUE, ch, 0, 0,
				    TO_ROOM);
				break;
			case 3:
				mobsay(ch,
				       "Listen I don't have all day, you got something to trade?");
				act("$n sighs loudly and stares at you with a blank face.", TRUE,
				    ch, 0, 0, TO_ROOM);
				break;
			case 4:
				act("$n suddenly turns in your direction!", TRUE, ch, 0, 0,
				    TO_ROOM);
				mobsay(ch,
				       "I'm Fanatic, I'm Frantic, I wanna buy! I wanna sell! I wanna buy!");
				act("$n turns away throwing his hands madly into the air!", TRUE,
				    ch, 0, 0, TO_ROOM);
				break;
			default:
				break;
			}
		}
	}
	return (FALSE);
}

int tharn_beach_guard(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (IS_FIGHTING(ch))
		return FALSE;

	if (pl)
	{
		return (0);
	}
	else
	{
		if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
		{
			switch (number(2, 10))
			{
			case 2:
				act("$n scans along the beach looking for any signs of trouble.",
				    TRUE, ch, 0, 0, TO_ROOM);
				break;
			case 3:
				mobsay(ch, "It's a beautiful day on the beach today isn't it?");
				act("$n stares up into the sky admiring it's beauty.", TRUE, ch, 0,
				    0, TO_ROOM);
				break;
			case 4:
				mobsay(ch, "Look at the way the seashells glimmer on the sand.");
				break;
			case 5:
				mobsay(ch,
				       "Being a beach guard seems like the worse job sometimes.");
				act("$n sighs loudly for a very long time.", TRUE, ch, 0, 0,
				    TO_ROOM);
				break;
			default:
				break;
			}
		}
	}
	return (FALSE);
}

int tharn_male_commoner(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (IS_FIGHTING(ch))
		return FALSE;

	if (pl)
	{
		return (0);
	}
	else
	{
		if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
		{
			switch (number(2, 10))
			{
			case 2:
				act("$n smiles happily.", TRUE, ch, 0, 0, TO_ROOM);
				break;
			case 3:
				mobsay(ch, "Well met, how are things going in the big city?");
				act("$n stares at you waiting for a reply.", TRUE, ch, 0, 0,
				    TO_ROOM);
				break;
			case 4:
				mobsay(ch,
				       "You know what? Out of all the gods Alyx does the most for us.");
				act("$n stares into the sky in awe.", TRUE, ch, 0, 0, TO_ROOM);
				mobsay(ch, "What must it be like to be immortal?");
				break;
			case 5:
				act("$n looks around the town for something interesting.", TRUE, ch,
				    0, 0, TO_ROOM);
				break;
			default:
				break;
			}
		}
	}
	return (FALSE);
}

int tharn_female_commoner(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (IS_FIGHTING(ch))
		return FALSE;

	if (pl)
	{
		return (0);
	}
	else
	{
		if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
		{
			switch (number(2, 10))
			{
			case 2:
				mobsay(ch, "Why hello, how are you this fine day?");
				act("$n smile gracefully at you.", TRUE, ch, 0, 0, TO_ROOM);
				break;
			case 3:
				mobsay(ch, "Have you seen my son or husband?");
				break;
			case 4:
				act("$n straightens out her dress.", TRUE, ch, 0, 0, TO_ROOM);
				break;
			case 5:
				mobsay(ch, "Oh I forgot to go to the market.");
				act("$n slaps herself in the forhead.", TRUE, ch, 0, 0, TO_ROOM);
			default:
				break;
			}
		}
	}
	return (FALSE);
}

int tharn_human_merchant(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (IS_FIGHTING(ch))
		return FALSE;

	if (pl)
	{
		return (0);
	}
	else
	{
		if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
		{
			switch (number(2, 10))
			{
			case 2:
				mobsay(ch, "excuse me, do you know where the bazaar is?");
				break;
			case 3:
				act("$n scans around the city.", TRUE, ch, 0, 0, TO_ROOM);
				mobsay(ch, "I never realized how big this is.");
				break;
			case 4:
				mobsay(ch, "Hmmm, Where can a man go to have a good time?");
				break;
			case 5:
				mobsay(ch, "This seems like a very good trading town.");
				act("$n begins to ponder on something.", TRUE, ch, 0, 0, TO_ROOM);
			default:
				break;
			}
		}
	}
	return (FALSE);
}

int tharn_lighthouse_attendent(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (IS_FIGHTING(ch))
		return FALSE;

	if (pl)
	{
		return (0);
	}
	else
	{
		if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
		{
			switch (number(2, 10))
			{
			case 2:
				act("$n polishes up around the room making sure everything is clean.",
				    TRUE, ch, 0, 0, TO_ROOM);
				break;
			case 3:
				mobsay(ch, "Please watch your step.");
				break;
			case 4:
				mobsay(ch, "Any problems I have from you, I'll call the guards.");
				act("$n eyes you nervously.", TRUE, ch, 0, 0, TO_ROOM);
				break;
			case 5:
				mobsay(ch, "I too keep this place in tiptop order everyday.");
				break;
			default:
				break;
			}
		}
	}
	return (FALSE);
}

int tharn_crier_one(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	char buf[MAX_INPUT_LENGTH];

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (IS_FIGHTING(ch))
		return (FALSE);

	if (pl)
	{
		return (0);
	}
	else
	{
		if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
		{
			switch (number(1, 150))
			{
			case 2:
				act("$n smells the fresh air.", TRUE, ch, 0, 0, TO_ROOM);
				break;
			case 3:
				act("$n clears his throat.", TRUE, ch, 0, 0, TO_ROOM);
				break;
			case 4:
				/*
					          mobsay (ch, "Careful when traveling east of Tharnadia!");
					          mobsay (ch, "The Bugbears are a troublesome type.");
					*/
				mobsay(ch,
				       "Theres a small forest just beyond the east gates, I've heard its a good place for new adventurers.");
				break;
			case 5:
				act("$n wonders why you are looking at him.", TRUE, ch, 0, 0,
				    TO_ROOM);
				break;
			case 6:
				act("$n wipes snot from his nose.", TRUE, ch, 0, 0, TO_ROOM);
				break;
			case 7:
				/*
					          mobsay (ch, "Which god is the true god of Dragons? Alyx or Io?");
					          mobsay (ch, "Well thats what the scholars of Tharnadia are trying to figure out.");
					*/
				mobsay(ch,
				       "Lord Braddinstock's mansion lies by the northern beach, but be careful exploring it! Rumor has it it's haunted!");
				break;
			case 8:
				mobsay(ch,
				       "Does anyone know if the secret home of the elves is a myth or true?");
				break;
			case 9:
				mobsay(ch,
				       "For a good time head over to Vella's Bordello, you'll definitely like it.");
				act("$n grins evilly.", TRUE, ch, 0, 0, TO_ROOM);
				break;
			case 10:
				mobsay(ch,
				       "If you have experience in fighting and such, they're plenty of guilds.");
				mobsay(ch,
				       "Make sure you join one and take up the fight against the evil races.");
				break;
			case 11:
				strcpy(buf, "If you're headin' outside of Tharnadia be careful!");
				do_yell(ch, buf, 0);
				strcpy(buf,
				       "Trolls, giants, and many dangers may threaten your lives.");
				do_yell(ch, buf, 0);
				break;
			case 12:
				act("$n spins around and jumps up and down.", TRUE, ch, 0, 0,
				    TO_ROOM);
				break;
			case 13:
				mobsay(ch,
				       "Careful - some say that drow and duergar are being seen around here.");
				mobsay(ch, "Those who left to go after them haven't returned.");
				break;
			case 14:
				act("The town crier burps loudly.", TRUE, ch, 0, 0, TO_ROOM);
				break;
			case 15:
				strcpy(buf, "We can use all the warriors and fighters there is!");
				do_yell(ch, buf, 0);
				strcpy(buf,
				       "Force those evils back into the Underdark where they belong.");
				do_yell(ch, buf, 0);
				break;
			case 16:
				/*          mobsay (ch, "People have heard screams near the graveyard!"); */
				mobsay(ch,
				       "Be kind to your fellow adventurers! Who knows when you will need their help!");
				break;
			case 17:
				mobsay(ch,
				       "Hey, folks, I hear there are good things for sale in the bazaar!");
				break;
			case 18:
				mobsay(ch, "If you have anything and I mean anything for sale,");
				mobsay(ch, "you now can auction it - though let's get real,");
				mobsay(ch, "you'll just kill for it.");
				act("$n snickers softly to himself.", TRUE, ch, 0, 0, TO_ROOM);
				break;
			case 19:
				/*
					          mobsay (ch, "Hunting can be great if you head east then north to kill buffalos.");
					          mobsay (ch, "Be careful up there, I hear they're mighty tough!");
					*/
				mobsay(ch,
				       "Remember when leaving the city limits to take caution. Undead and Evil forces prowl the wilderness.");
				break;
			case 20:
				mobsay(ch, "The gods always make sure I'm okay.");
				act("$n winks knowingly.", TRUE, ch, 0, 0, TO_ROOM);
				break;
			case 21:
				/*          strcpy (buf, "Anyone caught killing citizens within our protection will be outcasted!");
					          do_yell (ch, buf, 0);
					          act ("$n glares around the room looking serious.", TRUE, ch, 0, 0, TO_ROOM);
					*/
				mobsay(ch,
				       "Buying a boat is a good investment, but sailing takes practice.");
				mobsay(ch,
				       "The Tharnadian western and southern docks are a good place to get advice.");
				break;
			case 22:
				mobsay(ch,
				       "The paladins are sensing evil grow stronger day by day.");
				mobsay(ch,
				       "Be wary for raids, denizens of the underdark and the walking dead have been sighted out in the wilderness.");
				break;
			case 23:
				/*          mobsay (ch, "Remember to fill your barrels and rations up before leaving town!");
					            mobsay (ch, "It's a hard and dangerous world out there.");
					*/
				act("A &+cfemale &+Ccommoner&N says 'Excuse me, is that a gleaming two handed magical enchanted glowing mythical broadsword?'",
				    TRUE, ch, 0, 0, TO_ROOM);
				mobsay(ch,
				       "Why, yes it is. Hey, self-defense is no laughing matter!");
				mobsay(ch,
				       "Thats why when I want number one I pack a gleaming two handed magical enchanted glowing mythical broadsword...accept no substitutes.");
				break;
			case 24:
				act("$n says softly, 'If you want to see beautiful sights go west to the docks and head to moonshae.'",
				    TRUE, ch, 0, 0, TO_ROOM);
				act("$n smiles gaily, doing a pirouette.", TRUE, ch, 0, 0, TO_ROOM);
				break;
			case 25:
				/*
					          mobsay (ch, "Please leave the Hermit east of Tharnadia alone.");
					          mobsay (ch, "He's quite old and likes to be to himself.");
					*/
				mobsay(ch,
				       "If you are looking for armor, follow the Tharnadian Way west from here and look for Sten Hardik's armory.");
				mobsay(ch,
				       "If you are looking for weapons, look for Brinn's Weaponry on Keats Street.");
				mobsay(ch,
				       "If you are looking for misc travelling gear, why not stop by Jelian's Supply Shoppe? Go West down Tharnadian Way past Brinn's until just before the Shr'nn Mesa, then "
				       "North along The Western Wall Road then East into her shop.");
				mobsay(ch,
				       "For more useful locations why not stop by Kabanon, the prophet on the Shr'nn mesa, he sells maps of the city.");
				break;
			case 26:
				/*
					          mobsay (ch, "Warning to all traveling south!");
					          mobsay (ch, "The old ruin is said to be hollow..");
					          mobsay (ch, "From what I hear, there is a portal to hell there!");
					          act ("$n shivers uncomfortably.", TRUE, ch, 0, 0, TO_ROOM);
					*/
				mobsay(ch,
				       "The patrol guards outside are there to help you, but even they can't protect you from everything.");
				break;
			case 27:
				mobsay(ch, "Be warned!");
				mobsay(ch, "Not much is known about the drow race.");
				mobsay(ch, "They seem to know almost everything about us.");
				break;
			case 28:
				mobsay(ch, "Oh! And one important thing I have to tell..");
				act("$n is struck in the back of the head by an airborne tomato, though his assailant cannot be seen.",
				    TRUE, ch, 0, 0, TO_ROOM);
				mobsay(ch, "HEY! That hurt!");
				break;
			case 29:
				/*          mobsay (ch, "I have heard that the knights in Myranthea has the best protection on duris.");
					            mobsay (ch, "Stop by there some time and see King Alyyx's beautiful castle.");
					*/
				break;
			case 30:
				mobsay(ch, "It's a long way to the Moonshaes!");
				mobsay(ch,
				       "Only way to go is by ship, and even that takes a while.");
				break;
			case 31:
				strcpy(buf, "Bars are open all night boys!");
				do_yell(ch, buf, 0);
				break;
			case 32:
				strcpy(buf,
				       "If anyone wants me to shut up just tell me, don't kill me.");
				do_yell(ch, buf, 0);
				break;
			case 33:
				mobsay(ch,
				       "I used to be a great warrior once, I like this job better though.");
				act("$n winks knowingly.", TRUE, ch, 0, 0, TO_ROOM);
				break;
			case 34:
				mobsay(ch,
				       "Oooo, I haven't seen someone as ugly as you in a LONG time!");
				act("$n falls down laughing.", TRUE, ch, 0, 0, TO_ROOM);
				break;
			case 35:
				mobsay(ch,
				       "If you're a warrior and don't wanna leave your family but wanna help,");
				mobsay(ch, "do the right thing and join Tharnadia's guard.");
				break;
			case 36:
				strcpy(buf,
				       "If you're in need of healing just go to the hospital.");
				do_yell(ch, buf, 0);
				strcpy(buf,
				       "The doctors will take great care of you there .. for a price.");
				do_yell(ch, buf, 0);
				break;
			case 37:
				act("$n drinks some water to clear his throat.", TRUE, ch, 0, 0,
				    TO_ROOM);
				break;
			case 38:
				mobsay(ch,
				       "The gods have brought good weather to the farmers again this year!");
				strcpy(buf, "Blessed be the glorious gods of Duris!");
				do_yell(ch, buf, 0);
				break;
			case 39:
				act("A boy runs through the room, hitting the crier and knocking him over.",
				    TRUE, ch, 0, 0, TO_ROOM);
				act("A boy says, 'Shut your stupid hole, fool!', and runs off",
				    TRUE, ch, 0, 0, TO_ROOM);
				act("$n screams, 'Hey!  Stop that little shit!'", TRUE, ch, 0, 0,
				    TO_ROOM);
				break;
			case 40:
				mobsay(ch, "There once was a man from Nantucket ..");
				act("$n giggles to himself like a little schoolgirl.", TRUE, ch, 0,
				    0, TO_ROOM);
				break;
			case 41:
				/*          mobsay (ch, "There are rumors of an ancient CRYPT inhabited by a powerful necromancer named Theodeus Nak'ral..  Perhaps you should check it out.");
					        act ("$n nudges you surreptiously.", TRUE, ch, 0, 0, TO_ROOM);
					*/
				mobsay(ch,
				       "The wagon leaves from the eastern gates! Its a good way to see the sights and a safe way to get to Woodseer.");
				break;
			default:
				break;
			}
		}
	}
	return (FALSE);
}

int tharn_shady_mercenary(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (IS_FIGHTING(ch))
		return FALSE;

	if (pl)
	{
		return (0);
	}
	else
	{
		if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
		{
			switch (number(2, 10))
			{
			case 2:
				act("$n glares at you wondering why you are watching him.", TRUE,
				    ch, 0, 0, TO_ROOM);
				break;
			case 3:
				mobsay(ch, "What you got something to say?");
				break;
			case 4:
				mobsay(ch, "If you got a job for me, I'll get it done.");
				act("$n cracks his knuckles.", TRUE, ch, 0, 0, TO_ROOM);
				break;
			case 5:
				mobsay(ch, "Is it hot in here or is it just me?");
				break;
			default:
				break;
			}
		}
	}
	return (FALSE);
}

int tharn_shady_youth(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (IS_FIGHTING(ch))
		return FALSE;

	if (pl)
	{
		return (0);
	}
	else
	{
		if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
		{
			switch (number(2, 10))
			{
			case 2:
				act("$n stares at you in scorn.", TRUE, ch, 0, 0, TO_ROOM);
				break;
			case 3:
				act("$n spits in your direction.", TRUE, ch, 0, 0, TO_ROOM);
				break;
			case 4:
				act("$n looks around as if he's lost something.", TRUE, ch, 0, 0,
				    TO_ROOM);
				break;
			default:
				break;
			}
		}
	}
	return (FALSE);
}

int tharn_jailor(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (IS_FIGHTING(ch))
		return FALSE;

	if (pl)
	{
		return (0);
	}
	else
	{
		if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
		{
			switch (number(2, 10))
			{
			case 2:
				act("$n looks around the area with caution.", TRUE, ch, 0, 0,
				    TO_ROOM);
				break;
			case 3:
				mobsay(ch, "This is where you'll end up if you misbehave.");
				break;
			case 4:
				mobsay(ch, "No one escapes from this jail.");
				break;
			default:
				break;
			}
		}
	}
	return (FALSE);
}

int tharn_old_man(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (IS_FIGHTING(ch))
		return FALSE;

	if (pl)
	{
		return (0);
	}
	else
	{
		if (MIN_POS(ch, POS_STANDING + STAT_NORMAL))
		{
			switch (number(2, 10))
			{
			case 2:
				act("$n squints and peers at you.", TRUE, ch, 0, 0, TO_ROOM);
				break;
			case 3:
				mobsay(ch, "You warriors nowadays are weak compared to us.");
				mobsay(ch,
				       "I remember I had to walk in 7inches of snow for a battle.");
				mobsay(ch, "Stop fooling around and clean this place up.");
				break;
			case 4:
				act("$n waves his cane at you.", TRUE, ch, 0, 0, TO_ROOM);
				mobsay(ch, "Damn youngens, all you do is die nowadays.");
				mobsay(ch, "If I could pick up a sword again, I'll slash you one.");
				break;
			case 5:
				mobsay(ch, "One time I killed 20 Trolls with one rock.");
				act("$n snickers evilly to himself.", TRUE, ch, 0, 0, TO_ROOM);
				break;
			default:
				break;
			}
		}
	}
	return (FALSE);
}

int die_roller(P_obj obj, P_char ch, int cmd, char *arg)
{
	int i, pos = -1, numb;
	char Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!obj || !ch || cmd == CMD_PERIODIC || !IS_AWAKE(ch))
		return (FALSE);

	if (!OBJ_WORN(obj))
		return (FALSE);

	if (obj->loc.wearing != ch)
	{
		logit(LOG_DEBUG, "Buggy equip in die_roller(): obj->loc.wearing != ch.");
		return (FALSE);
	}
	/*
	   item must be held to work -- can modify
	 */
	for (i = 0; i < MAX_WEAR; i++)
	{
		if (obj->loc.wearing->equipment[i] == obj)
		{
			pos = i;
			break;
		}
	}

	/*
	   temporary kludge until we get secondary-hold as we should
	 */
	if ((pos != HOLD) && (pos != WIELD))
		return (FALSE);

	if (cmd == CMD_ROLL)
	{ /*
		 roll
	   */
		argument_interpreter(arg, Gbuf1, Gbuf2);
		/*
		   item must be held to work
		 */
		if (isname(Gbuf1, obj->loc.wearing->equipment[pos]->name))
		{
			/*
			   roll the damn die
			 */
			if (obj->value[0] <= 0)
			{
				send_to_char("error in die..  tell somebody.\n", ch);
				return TRUE;
			}

			numb = number(1, obj->value[0]);

			obj_to_room(unequip_char(ch, pos), ch->in_room);

			snprintf(Gbuf1, MAX_STRING_LENGTH,
				 "Tossing the $q&n onto the ground, you roll a %u.", numb);
			snprintf(Gbuf2, MAX_STRING_LENGTH,
				 "Tossing $p&n onto the ground, $n rolls a %u.", numb);

			act(Gbuf1, FALSE, ch, obj, 0, TO_CHAR);
			act(Gbuf2, FALSE, ch, obj, 0, TO_ROOM);

			return (TRUE);
		}
	}

	return (FALSE);
}

/*
 * Tharnadia patrols justice  - Kvark
 *
 * Basicly scan and track, and if enaged shout for assistence.
 *
 *
 */

/*
 *Patrol leader Mob Proc
 */

/*

*/

int tower_data[5][6] = {
	{ 150115, 150122, 150129, 150130, 150131, 150132 }, // Human
	{ 150118, 150124, 150133, 150134, 0, 0 }, // Gnome
	{ 150119, 150126, 150139, 150140, 0, 0 }, // dwarf
	{ 150116, 150123, 150135, 150136, 0, 0 }, // barb
	{ 150117, 150125, 150137, 150138, 0, 0 } // halfling
};

int outpost_captain(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	int direction;
	bool CombatInRoom;
	int distance = 10;
	int helpers_1[6]; // 4 Elites lvl 50
	int helpers_2[6]; // 4 elites lvl 50 + captain 55
	int helpers_3[6];
	int ii = 0;
	P_desc d;

	// return 0;
	int how_many;
	P_char tmp_ch;
	P_char t_ch;
	struct follow_type *k, *next_dude;
	char buf[256];

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd)
		return FALSE;

	if (GET_VITALITY(ch) < 10) /* ok dont get too tired */
		return TRUE;

	if (time_info.hour == 5)
		distance = (int)(distance / 3) + number(1, 3);
	else if (time_info.hour == 6)
		distance = (int)(distance / 3) + number(1, 2);
	else if (time_info.hour == 15)
		distance = (int)(distance / 3) + number(1, 3);
	else if (time_info.hour == 16)
		distance = (int)(distance / 3) + number(1, 3);
	else if ((time_info.hour > 17) || (time_info.hour < 7))
	{
		distance = (int)(distance / 3) + number(0, 1);
		if (!number(0, 60) && !(GET_STAT(ch) == STAT_SLEEPING))
		{
			act("&+WThe sounds of violent snoring filter through the area.", FALSE, ch,
			    0, 0, TO_ROOM);
			do_sit(ch, 0, 0);
			do_sleep(ch, 0, 0);
			return FALSE;
		}
		if (!number(0, 10) && GET_STAT(ch) == STAT_SLEEPING)
		{
			act("&+WThe sounds of violent snoring filter through the area.", FALSE, ch,
			    0, 0, TO_ROOM);
			return FALSE;
		}
		if (GET_STAT(ch) == STAT_SLEEPING)
			return FALSE;
	}
	else
	{
		distance = distance + number(0, 2);
		if (GET_STAT(ch) == STAT_SLEEPING)
		{
			do_wake(ch, 0, 0);
			do_stand(ch, 0, 0);
			do_alert(ch, 0, 0);
			if (!number(0, 3))
				act("&+WThe sounds of snoring slowly dissipates.", FALSE, ch, 0, 0,
				    TO_ROOM);
			return FALSE;
		}
	}

	/* ok check to make sure followers are not out of move */

	if (ch->followers)
	{
		for (k = ch->followers; k; k = next_dude)
		{
			next_dude = k->next;
			if (IS_NPC(k->follower) && (GET_VITALITY(k->follower) < 10))
				return TRUE;
		}
	}

	CombatInRoom = FALSE;

	if (!ALONE(ch))
	{
		if (IS_FIGHTING(ch))
			CombatInRoom = TRUE;
		else
		{
			LOOP_THRU_PEOPLE(tmp_ch, ch)
				if (IS_FIGHTING(tmp_ch))
				{
					CombatInRoom = TRUE;
					break;
				}
		}
	}
	how_many = 0;
	/* Fix this to check how many in room */
	if (IS_FIGHTING(ch) && number(1, 3) == 1)
	{
		if ((IS_PC(GET_OPPONENT(ch)) && IS_RACEWAR_EVIL(GET_OPPONENT(ch))) ||
		    (IS_RACEWAR_UNDEAD(GET_OPPONENT(ch)) && IS_PC(GET_OPPONENT(ch))))
		{
			LOOP_THRU_PEOPLE(t_ch, ch)
			{
				if ((IS_PC(t_ch) && IS_RACEWAR_EVIL(t_ch)) ||
				    (IS_PC(t_ch) && IS_RACEWAR_UNDEAD(t_ch)))
				{
					if (!IS_TRUSTED(ch))
						how_many++;
				}
			}
			ii = 0;
			if (GET_RACE(ch) == RACE_HUMAN)
				ii = 0;
			if (GET_RACE(ch) == RACE_GNOME)
				ii = 1;
			if (GET_RACE(ch) == RACE_MOUNTAIN)
				ii = 2;
			if (GET_RACE(ch) == RACE_BARBARIAN)
				ii = 3;
			if (GET_RACE(ch) == RACE_HALFLING)
				ii = 4;
			helpers_1[0] = tower_data[ii][number(0, 5)];

			helpers_2[0] = tower_data[ii][number(0, 5)];
			helpers_2[1] = tower_data[ii][number(0, 5)];

			helpers_3[0] = tower_data[ii][number(0, 5)];
			helpers_3[1] = tower_data[ii][number(0, 5)];
			helpers_3[2] = tower_data[ii][number(0, 5)];

			if (how_many <= 4 && how_many > 1 && !number(0, 8))
				return shout_and_hunt(
					ch, 100,
					"&+WGuardians of good come aid me in vanquishing the evil invaders!",
					NULL, helpers_1, 0, 0);
			if (how_many > 4 && how_many <= 6 && !number(0, 5))
				return shout_and_hunt(
					ch, 100,
					"&+WGuardians of good come help me destroy the evil invasion!",
					NULL, helpers_2, 0, 0);
			if (how_many > 6 && !number(0, 1))
				return shout_and_hunt(
					ch, 100,
					"&+Wguardians of good come help me defend our homeland!",
					NULL, helpers_3, 0, 0);
		}
		else if (IS_PC(GET_OPPONENT(ch)) && IS_RACEWAR_GOOD(GET_OPPONENT(ch)))
		{
			strcpy(buf, "You moron I am here to protect you!");
			do_say(ch, buf, CMD_SAY);
		}
	}

	if (!CombatInRoom && (ch->in_room != NOWHERE) && !IS_ROOM(ch->in_room, ROOM_SILENT) &&
	    !IS_SET(zone_table[world[ch->in_room].zone].flags, ZONE_SILENT) &&
	    (MIN_POS(ch, POS_STANDING + STAT_NORMAL)))
	{
		/* ok we check if there is any evils near 50 rooms, if
		 * soo, piss it off!
		 */

		// TRACK IF EVILS

		d = descriptor_list;
		ii = 0;
		if (number(0, 1))
			while (d)
			{
				if (!d->connected &&
				    (world[ch->in_room].zone ==
				     world[d->character->in_room].zone) &&
				    !(IS_ROOM(ch->in_room, ROOM_GUILD)))
				{
					/*found char in same zone */
					if ((IS_PC(d->character) && IS_RACEWAR_EVIL(d->character) &&
					     !IS_TRUSTED(d->character)) ||
					    (IS_PC(d->character) &&
					     IS_RACEWAR_UNDEAD(d->character) &&
					     !IS_TRUSTED(d->character)))
					{
						if (how_close(ch->in_room, d->character->in_room,
							      distance) > 0)
							if (number(0, 11) >
							    BOUNDED(0,
								    how_close(ch->in_room,
									      d->character->in_room,
									      distance),
								    10))
							{ // the longer the less offen
								ii++;
								if (number(0, 10) < ii)
								{ // the more in group the easier
									remember(ch, d->character);
									wizlog(56,
									       "Mob(%s), starts to hunt(%s) on tharn",
									       ch->player
										       .short_descr,
									       GET_NAME(
										       d->character));
									if (!number(0, 1))
										act("&+WSomeone has discovered your presence and starts to track you!",
										    FALSE,
										    d->character, 0,
										    0, TO_CHAR);
								}
							}
					}
				}
				d = d->next;
			}

		// END TRACK IF EVILS!
		/* ok we check if there is combat near */

		if ((direction = range_scan(ch, NULL, 2, SCAN_COMBAT)) >= 0)
		{
			if (EXIT(ch, direction))
			{
				if ((EXIT(ch, direction))->to_room &&
				    world[EXIT(ch, direction)->to_room].justice_area ==
					    world[ch->in_room].justice_area)
				{
					ch->only.npc->last_direction = direction;
					do_move(ch, 0, exitnumb_to_cmd(direction));
					return TRUE;
				}
			}
		}
	}

	/* seem we can try to move, since nothing else to do */

	return FALSE;
}
