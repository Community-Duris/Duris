/* Special procedures for the Bloodstone zone. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "combat/damage.h"
#include "combat/justice.h"
#include "world/specs.prototypes.h"
#include <stdio.h>
#include <string.h>

int bs_boss(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		mobsay(ch, "Are you some kind of sick freak or what?");
		do_action(ch, 0, CMD_GLARE);
		return TRUE;
	case 1:
		mobsay(ch, "I am going to slaughter your hide when I am done here.");
		return TRUE;
	case 2:
		mobsay(ch, "Oh god, please have mercy!");
		do_action(ch, 0, CMD_GRUNT);
		return TRUE;
	case 3:
		act("$n squeezes out a huge chunk of shit.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 4:
		mobsay(ch, "Damn! I forgot the toilet parchment!");
		do_action(ch, 0, CMD_MOAN);
		return TRUE;
	}
	return FALSE;
}

int bs_barons_mistress(P_char ch, P_char tch, int cmd, char * /*arg*/)
{
	int helpers[] = { 74000, 74003, 74004, 74006, 74011, 74012, 74019, 74022, 74037,
			  74038, 74039, 74040, 74041, 74042, 74043, 74044, 74045, 74055,
			  74056, 74057, 74058, 74059, 74060, 74061, 74066, 74067, 74068,
			  74069, 74070, 74071, 74074, 74075, 74081, 74082, 74083, 74084,
			  74085, 74086, 74089, 74090, 74092, 74108, 74109, 74115, 74116,
			  74117, 74123, 74124, 74125, 74126, 74146, 74157, 74158, 74160,
			  74175, 74176, 74179, 74180, 74181, 74186, 74187, 74188, 74190,
			  74225, 74229, 74230, 74231, 74232, 74237, 74244, 74245, 0 };
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (!tch)
		return shout_and_hunt(ch, 4, "Somebody HELP me! I am being robbed by %s!\r\n", NULL,
				      helpers, 0, 0);
	return FALSE;
}

int bs_citizen(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	     check for periodic event calls
	*/
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		mobsay(ch, "Hail stranger!  Tis a fine day, is it not?");
		return TRUE;
	case 1:
		mobsay(ch, "Damn!  Taxes were raised again!");
		return TRUE;
	case 2:
		mobsay(ch, "Have you seen my wife around here anywhere?");
		return TRUE;
	case 3:
		mobsay(ch, "I hear Verzanan is a nice place to visit.");
		return TRUE;
	case 4:
		do_action(ch, 0, CMD_BURP);
		do_action(ch, 0, CMD_BLUSH);
		mobsay(ch, "Excuse me.");
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_comwoman(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		mobsay(ch, "Have you been to the garden?");
		return TRUE;
	case 1:
		do_action(ch, 0, CMD_PONDER);
		mobsay(ch, "Now where did that boy go?");
		return TRUE;
	case 2:
		act("$n pulls out a shopping list and studies it carefully.", TRUE, ch, 0, 0,
		    TO_ROOM);
		return TRUE;
	case 3:
		act("$n looks at you, and winks suggestively.", TRUE, ch, 0, 0, TO_ROOM);
		do_action(ch, 0, CMD_BLUSH);
		return TRUE;
	case 4:
		mobsay(ch, "Have you traveled far?  Bloodstone does offer a nice inn.");
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_brat(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		do_action(ch, 0, CMD_SNICKER);
		mobsay(ch, "I am hiding from my mom!");
		return TRUE;
	case 1:
		do_action(ch, 0, CMD_SNORT);
		mobsay(ch, "Yummy, something big!");
		return TRUE;
	case 2:
		act("$n looks at you and bursts into laughter!", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 3:
		act("$n picks $s nose and casually places the treasure in $s mouth.", TRUE, ch, 0,
		    0, TO_ROOM);
		return TRUE;
	case 4:
		mobsay(ch, "My pop could snap your neck instantly!");
		do_action(ch, 0, CMD_STRUT);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_holyman(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		act("$n raises $s unholy symbol, and cracks a wicked smile.", TRUE, ch, 0, 0,
		    TO_ROOM);
		return TRUE;
	case 1:
		mobsay(ch, "Praise be to &+yHoar&N, &+LThe Doombringer&N!");
		return TRUE;
	case 2:
		act("$n chants 'Fernum Acti Blas' and looks at you with contempt.", TRUE, ch, 0, 0,
		    TO_ROOM);
		return TRUE;
	case 3:
		mobsay(ch, "Leave me quickly, before I unleash my dark powers upon you!");
		return TRUE;
	case 4:
		do_action(ch, 0, CMD_KNEEL);
		mobsay(ch, "Oh mighty &+yHoar&N, please grant me your dark powers!");
		act("A bolt of &+Bblue lightning&N strikes the holyman!", TRUE, ch, 0, 0, TO_ROOM);
		do_action(ch, 0, CMD_CACKLE);
		mobsay(ch, "Praise be to &+yHoar&N!");
		do_action(ch, 0, CMD_STAND);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_merchant(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		do_action(ch, 0, CMD_PONDER);
		return TRUE;
	case 1:
		do_action(ch, 0, CMD_SMIRK);
		mobsay(ch, "Peasant trash!");
		return TRUE;
	case 2:
		mobsay(ch, "I must hurry, I have business with Waqar!");
		return TRUE;
	case 3:
		mobsay(ch, "A saber, a black silk sash...hmm what else was there?");
		do_action(ch, 0, CMD_PONDER);
		[[fallthrough]];
	default:
		return FALSE;
	}
}

int bs_wino(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodici event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		do_action(ch, 0, CMD_BURP);
		mobsay(ch, "Ahhh an excellent vintage!");
		return TRUE;
	case 1:
		act("$n kneels down close to the ground and covers $s mouth.", TRUE, ch, 0, 0,
		    TO_ROOM);
		do_action(ch, 0, CMD_PUKE);
		return TRUE;
	case 2:
		do_action(ch, 0, CMD_HUM);
		mobsay(ch, "How dry I am, how dry I am..");
		return TRUE;
	case 3:
		do_action(ch, 0, CMD_PONDER);
		mobsay(ch, "A roasted stirge would hit the spot!");
		do_action(ch, 0, CMD_DROOL);
		return TRUE;
	case 4:
		do_action(ch, 0, CMD_BOW);
		mobsay(ch, "At your service!");
		return TRUE;
	case 5:
		act("$n breaks out a bottle and takes a quick swig.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 6:
		mobsay(ch, "A tribute to the Baron!");
		do_action(ch, 0, CMD_FART);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_watcher(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 50))
	{
	case 0:
		mobsay(ch, "Move along, move along.");
		return TRUE;
	case 1:
		mobsay(ch, "Be careful you don't wander along the outside walls.");
		return TRUE;
	case 2:
		act("$n keeps a close eye on you.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 3:
		mobsay(ch, "I hear manticores make good mounts!");
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_guard(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 80))
	{
	case 0:
		mobsay(ch, "Beware the soultaker!");
		return TRUE;
	case 1:
		act("$n dazzles you with a sword technique.", TRUE, ch, 0, 0, TO_ROOM);
		do_action(ch, 0, CMD_SMILE);
		return TRUE;
	case 2:
		mobsay(ch, "Now where did that apprentice go?  You're not him!");
		do_action(ch, 0, CMD_FROWN);
		return TRUE;
	case 3:
		mobsay(ch, "Like my equipment eh? Visit Waqar to get a fine saber like this!");
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_squire(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 80))
	{
	case 0:
		mobsay(ch, "I better get back to my master before I'm missed!");
		return TRUE;
	case 1:
		act("$n bungles a simple dagger technique.", TRUE, ch, 0, 0, TO_ROOM);
		do_action(ch, 0, CMD_BLUSH);
		return TRUE;
	case 2:
		mobsay(ch, "With enough practice, I too can be a guard!");
		act("$n manages to execute a perfect dagger parry.", TRUE, ch, 0, 0, TO_ROOM);
		do_action(ch, 0, CMD_STRUT);
		return TRUE;
	case 3:
		mobsay(ch, "I used to be a wandering soul such as yourself.");
		mobsay(ch, "Now my life has purpose!");
		do_action(ch, 0, CMD_SMIRK);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_peddler(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 80))
	{
	case 0:
		mobsay(ch, "Can I sell you something?");
		return TRUE;
	case 1:
		act("$n displays to you his wares.", TRUE, ch, 0, 0, TO_ROOM);
		do_action(ch, 0, CMD_SMILE);
		return TRUE;
	case 2:
		mobsay(ch, "What?!  This is the finest merchandise in town!");
		do_action(ch, 0, CMD_FROWN);
		return TRUE;
	case 3:
		mobsay(ch, "Got something you don't want?  I'll buy most anything!");
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_critter(P_char ch, P_char pl, int cmd, char *arg)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	if (devour(ch, pl, cmd, arg))
		return TRUE;

	switch (number(0, 80))
	{
	case 0:
		do_action(ch, 0, CMD_SNARL);
		return TRUE;
	case 1:
		do_action(ch, 0, CMD_GROWL);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_timid(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		mobsay(ch, "I swear I didn't do it!");
		do_action(ch, 0, CMD_CRY);
		return TRUE;
	case 1:
		mobsay(ch, "This place is too dangerous for me!");
		do_action(ch, 0, CMD_SNIFF);
		return TRUE;
	case 2:
		mobsay(ch, "Let me out! Let me out!");
		return TRUE;
	case 3:
		mobsay(ch, "I wouldn't go upstairs if I were you!");
		do_action(ch, 0, CMD_WINCE);
		return TRUE;
	case 4:
		mobsay(ch, "I wish that I was back home in Verzanan!");
		do_action(ch, 0, CMD_SIGH);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_shady(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		mobsay(ch, "The food is terrible here!");
		return TRUE;
	case 1:
		mobsay(ch, "Hey you're pretty cute!");
		do_action(ch, 0, CMD_DROOL);
		return TRUE;
	case 2:
		mobsay(ch, "Outta my way punk!");
		do_action(ch, 0, CMD_LAUGH);
		return TRUE;
	case 3:
		act("$n looks you over very carefully.", TRUE, ch, 0, 0, TO_ROOM);
		act("$n winks at you in a seductive way.", TRUE, ch, 0, 0, TO_ROOM);
		mobsay(ch, "What do you say you and me get together?");
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_sinister(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		mobsay(ch, "Ya, I killed him.  What of it?");
		return TRUE;
	case 1:
		mobsay(ch, "Hey, you're pretty cute!");
		do_action(ch, 0, CMD_DROOL);
		return TRUE;
	case 2:
		mobsay(ch, "Go away before I rip your throat out!");
		do_action(ch, 0, CMD_GLARE);
		return TRUE;
	case 3:
		mobsay(ch, "Care to spar with me, chump?");
		do_action(ch, 0, CMD_FLEX);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_menacing(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 80))
	{
	case 0:
		mobsay(ch, "I feel like eating somebody's heart!");
		do_action(ch, 0, CMD_CACKLE);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_executioner(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		mobsay(ch, "Nothing like a fine axe blade, eh?");
		do_action(ch, 0, CMD_CACKLE);
		return TRUE;
	case 1:
		mobsay(ch, "She said, 'Sir, please don't kill my husband!'");
		do_action(ch, 0, CMD_LAUGH);
		return TRUE;
	case 2:
		act("$n swings $s heavy axe over $s shoulder.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 3:
		mobsay(ch, "You'd have to be pretty strong to wield this axe!");
		do_action(ch, 0, CMD_FLEX);
		return TRUE;
	case 4:
		do_action(ch, 0, CMD_SMIRK);
		mobsay(ch, "Stay out of trouble or I'll add ya to my necklace!");
		do_action(ch, 0, CMD_CACKLE);
		mobsay(ch, "Understand me?!");
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_baron(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		do_action(ch, 0, CMD_PONDER);
		mobsay(ch, "Methinks it is time to raise taxes!");
		do_action(ch, 0, CMD_CHUCKLE);
		return TRUE;
	case 1:
		mobsay(ch, "Where are my servants?!");
		do_action(ch, 0, CMD_POUT);
		return TRUE;
	case 2:
		act("$n removes a sparkling ring, and then replaces it!", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 3:
		mobsay(ch, "If my brother could only see me now!");
		do_action(ch, 0, CMD_LAUGH);
		return TRUE;
	case 4:
		do_action(ch, 0, CMD_GRUMBLE);
		mobsay(ch, "I have business to tend to.  Go purchase something!");
		mobsay(ch, "We have some of the finest shops around!");
		return TRUE;
	case 5:
		do_action(ch, 0, CMD_YAWN);
		mobsay(ch, "Servants!  Time for my nap.");
		do_action(ch, 0, CMD_SNAP);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_sparrow(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 80))
	{
	case 0:
		do_action(ch, 0, CMD_HOP);
		return TRUE;
	case 1:
		act("$n turns $s eyes towards the ground searching for some food.", TRUE, ch, 0, 0,
		    TO_ROOM);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_squirrel(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 80))
	{
	case 0:
		act("$n forages for some food.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 1:
		act("$n looks up to see what's around.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_crow(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 80))
	{
	case 0:
		do_action(ch, 0, CMD_HOP);
		act("$n eyes the area for some food.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 1:
		act("$n stares in your direction with malicious intent.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_mountainman(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		do_action(ch, 0, CMD_YODEL);
		return TRUE;
	case 1:
		act("$n strikes the ground with $s iron pick.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 2:
		mobsay(ch, "Hail stranger!  On your way to Bloodstone Keep?");
		return TRUE;
	case 3:
		mobsay(ch, "I'd keep clear of the manticore lair if I were you!");
		return TRUE;
	case 4:
		mobsay(ch, "To the far west is where the dead sleep.");
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_salesman(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		mobsay(ch, "Pssst!  I've got some items you might want!");
		return TRUE;
	case 1:
		mobsay(ch, "Buy low, sell high...A sound method, is it not?");
		return TRUE;
	case 2:
		mobsay(ch, "I buy what you do not want!");
		do_action(ch, 0, CMD_SMILE);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_nomad(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		mobsay(ch, "Which way to the closest inn?  I am oh so very tired.");
		return TRUE;
	case 1:
		do_action(ch, 0, CMD_WINCE);
		mobsay(ch,
		       "I've seen many vile creatures in my days, but NONE as gruesome as you!");
		do_action(ch, 0, CMD_CRINGE);
		return TRUE;
	case 2:
		mobsay(ch, "This place is too dangerous for a wimp like you!");
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_insane(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		mobsay(ch, "Who are you? What do you want? Leave me alone!");
		return TRUE;
	case 1:
		mobsay(ch, "Get them off of me!");
		do_action(ch, 0, CMD_HOP);
		return TRUE;
	case 2:
		mobsay(ch, "Kill them, kill them!!");
		do_action(ch, 0, CMD_STOMP);
		return TRUE;
	case 3:
		do_action(ch, 0, CMD_FIDGET);
		mobsay(ch, "Look!  A six foot stirge!");
		do_action(ch, 0, CMD_LAUGH);
		return TRUE;
	case 4:
		do_action(ch, 0, CMD_CRY);
		mobsay(ch, "Damn elves!  Damn dwarves!  Damn them all!");
		do_action(ch, 0, CMD_LAUGH);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_homeless(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		mobsay(ch, "Spare a few coins?");
		do_action(ch, 0, CMD_BEG);
		return TRUE;
	case 1:
		mobsay(ch, "I haven't always been this way...");
		do_action(ch, 0, CMD_CRY);
		return TRUE;
	case 2:
		do_action(ch, 0, CMD_SHIVER);
		mobsay(ch, "It is so cold, could you spare some coins for a room?");
		do_action(ch, 0, CMD_BEG);
		return TRUE;
	case 3:
		do_action(ch, 0, CMD_CRY);
		mobsay(ch, "Please help me, I haven't eaten in three moons.");
		do_action(ch, 0, CMD_SNIFF);
		return TRUE;
	case 4:
		mobsay(ch, "Will work for food!");
		return TRUE;
	case 5:
		mobsay(ch, "Spare some clothing for a homeless man?");
		do_action(ch, 0, CMD_SHIVER);
		mobsay(ch, "It's terribly cold here at night.");
		return TRUE;
	case 6:
		do_action(ch, 0, CMD_COUGH);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_servant(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		mobsay(ch, "Time for the Baron's bath.");
		return TRUE;
	case 1:
		mobsay(ch, "The Baron is such a demanding master!");
		do_action(ch, 0, CMD_SNIFF);
		return TRUE;
	case 2:
		do_action(ch, 0, CMD_WINCE);
		mobsay(ch, "I hope that the Baron spares me from my daily beating.");
		mobsay(ch, "I have worked hard for him today!");
		do_action(ch, 0, CMD_PONDER);
		return TRUE;
	case 3:
		mobsay(ch, "Hmm..I wonder what the Baron will want to eat tonight?");
		do_action(ch, 0, CMD_PONDER);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_wolf(P_char ch, P_char pl, int cmd, char *arg)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	if (devour(ch, pl, cmd, arg))
		return TRUE;

	switch (number(0, 100))
	{
	case 0:
		do_action(ch, 0, CMD_SNARL);
		return TRUE;
	case 1:
		do_action(ch, 0, CMD_GROWL);
		return TRUE;
	case 2:
		do_action(ch, 0, CMD_BARK);
		return TRUE;
	case 3:
		act("$n lifts $s head up and lets out a piercing howl.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_gnoll(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		do_action(ch, 0, CMD_SNARL);
		return TRUE;
	case 1:
		act("$n looks at you with malicious intent.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_ettin(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		do_action(ch, 0, CMD_SNARL);
		return TRUE;
	case 1:
		act("$n scratches one of $s heads.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 2:
		act("$n begins picking $s teeth again.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_griffon(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		do_action(ch, 0, CMD_SNARL);
		return TRUE;
	case 1:
		act("$n fans $s enormous wings.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 2:
		act("$n begins eyeing you quite intently.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 3:
		act("$n snaps the bone of a carcus $e is devouring.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_boar(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		do_action(ch, 0, CMD_SNARL);
		return TRUE;
	case 1:
		do_action(ch, 0, CMD_GROWL);
		return TRUE;
	case 2:
		act("$n lifts $s head up and lets out a piercing howl.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_cub(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 100))
	{
	case 0:
		act("$n rubs up against your leg.", TRUE, ch, 0, 0, TO_ROOM);
		do_action(ch, 0, CMD_PURR);
		return TRUE;
	case 1:
		do_action(ch, 0, CMD_GROWL);
		return TRUE;
	case 2:
		act("$n swats at a leaf turning circles in the wind.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_fierce(P_char ch, P_char pl, int cmd, char *arg)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	if (devour(ch, pl, cmd, arg))
		return TRUE;

	switch (number(0, 100))
	{
	case 0:
		act("$n eyes you very carefully.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 1:
		do_action(ch, 0, CMD_GROWL);
		return TRUE;
	case 2:
		act("$n lets out a tremendous roar.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 3:
		act("$n begins grooming itself.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 4:
		do_action(ch, 0, CMD_SNARL);
		return TRUE;
	default:
		return FALSE;
	}
}

int bs_stirge(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(0, 80))
	{
	case 0:
		do_action(ch, 0, CMD_HOP);
		return TRUE;
	case 1:
		act("$n turns $s eyes towards you in search of some fresh blood.", TRUE, ch, 0, 0,
		    TO_ROOM);
		return TRUE;
	default:
		return FALSE;
	}
}
