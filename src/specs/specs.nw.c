/* Special procedures for the Neverwinter area. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"
#include "world/weather.h"

extern P_room world;
extern struct time_info_data time_info;

/*
 * neverwinter mob procs
 */

int nw_woodelf(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "I LOVE the forest!");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "The trees are my home!");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "The forest provides the perfect shelter for me and my forest friends!");
		[[fallthrough]];
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_elfhealer(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "The woods provide us with the natural healant of life!");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "Do you not feel the natural healing vibes of the trees?");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "Sometimes, meditation amongst the trees provide the proper healant.");
		return TRUE;
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_ammaster(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "The amethyst is the perfect stone of all life.");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "The amethyst gives us our sustenance.");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "The amethyst is the staff of life.");
		return TRUE;
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_sapmaster(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "The sapphire represents power.");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "The sapphire is that which would give us strength.");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "The lion is the perfect symbol of the sapphire.");
		return TRUE;
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_diamaster(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "The diamond gives us clarity of life.");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "The diamond represents purity.");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "One must channel their thoughts through the diamond for inspiration.");
		return TRUE;
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_rubmaster(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "The ruby represents passion.");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "Extreme emotions make life interesting.");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "The ruby embodies all that we love and hate.");
		return TRUE;
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_emmaster(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "The emerald sybolizes growth.");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "The emerald is all that is nature.");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch,
		       "The trees, the rabbits, and even the ground you stand on is forcused through the emerald.");
		return TRUE;
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_human(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 80))
	{
	case 1:
	{
		mobsay(ch, "Where the hell am I?");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "I take a lousy stroll from Verzanan, and this is where I end up?");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "Shit. I need a tour guide or something.");
		return TRUE;
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_hafbreed(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "At least the forest does not care who I am.");
		return TRUE;
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_owl(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "Hoo hoo!");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "Hoo hoo!");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "Hoo hoo!");
		return TRUE;
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_golem(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "Why don't these pesky mortals just leave me alone!");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "I hate being bothered by flesh forms!");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "I must guard this tower with my life!");
		return TRUE;
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_agatha(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch,
		       "Hah! I do not wish to speak with thee, unless thee hast news of Malchor!");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch,
		       "That Malchor OWES me, and I will soon be leaving for his tower for payment!");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "Soon, the golden horse shoe will be MINE!");
		return TRUE;
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_farmer(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "I should not be here.");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "There are fields to be plowed!");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "There is corn to be grown!");
		return TRUE;
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_chicken(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "Buk buk buk bugack!");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "Buk buk buk bugack!");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "Bugack!");
		return TRUE;
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_pig(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "Oink oink!");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "Oink oink!");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "Oink oink!");
		return TRUE;
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_cow(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	case 2:
	case 3:
		do_action(ch, 0, CMD_COW);
		return TRUE;

	default:
		return FALSE;
	}
}

int nw_chief(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "There is so much to do for a farming community.");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "I must plan things around the seasons.");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "Soon, the crops will be grown, and we will all eat like kings!");
		[[fallthrough]];
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_malchor(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "It is nice to have visitors at the museum!");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "My museum is the best in all of the Neverwinter Woods!");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "There is much to learn here.");
		[[fallthrough]];
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_builder(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "I believe there needs to be a wall here.");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "I must cement these corners correctly.");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "I wonder if that roof needs thatching.");
		[[fallthrough]];
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_carpen(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "I must carve this furniture by the end of the day.");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "Hmmmm...I do not think these walls will go in correctly.");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "I wonder whether these measurements are correct?");
		[[fallthrough]];
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_logger(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "I love to walk on logs!");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "Nobody can travel logs like I can!");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "I must get these logs to the stream!");
		[[fallthrough]];
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_cutter(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "TIMBER!");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "TIMBER!");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "TIMBER!");
		[[fallthrough]];
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_foreman(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "Get those logs down to the stream!");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "Get those trees cut, PRONTO!");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "I want those planks cut by the end of the hour!");
		[[fallthrough]];
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_ansal(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "I am glad we are getting along with the elves.");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "We produce the trees properly, and the elves leave us alone.");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "Looks like our production is going well.");
		[[fallthrough]];
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_vitnor(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "We serve the BEST and ONLY drinks in these woods!");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "We have the tastiest drinks!");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "Drinks are great to end a day on!");
		[[fallthrough]];
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_brock(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "I hate these rugs-- every day, rugs rugs rugs!!");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "This store sure has plenty of dusty rugs.");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "Let's see, what rugs will I kill today?");
		[[fallthrough]];
	}
	default:
	{
		return FALSE;
	}
	}
}

int nw_merthol(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || IS_FIGHTING(ch) || cmd)
		return FALSE;

	switch (number(1, 100))
	{
	case 1:
	{
		mobsay(ch, "Let's see if that dolt Brock can sell more than I can!");
		return TRUE;
	}
	case 2:
	{
		mobsay(ch,
		       "My furs are of much better quality than those ugly dust - ridden rugs at Brock's.");
		return TRUE;
	}
	case 3:
	{
		mobsay(ch, "My furs will keep you warm in the winter.");
		[[fallthrough]];
	}
	default:
	{
		return FALSE;
	}
	}
}

/*
 * support function for nw_mirroid, sends message to both rooms, and blocks/unblocks the
 * given exit.
 */
void nw_block_exit(int room, int dir, int flag)
{
	P_char t_ch;
	int i, room2;
	const char Gbuf1[] = "You see shifting reflections.\r\n";
	const char Gbuf2[] = "You hear a faint creaking, and see shifting reflections.\r\n";
	const char Gbuf3[] = "You hear a faint creaking.\r\n";

	if ((room == NOWHERE) || !VIRTUAL_EXIT(room, dir))
		return;

	if (world[room].dir_option[dir]->to_room)
	{
		room2 = world[room].dir_option[dir]->to_room;
	}
	else
	{
		return;
	}

	if (room2 == NOWHERE)
	{
		return;
	}

	// TODO: Fix this such that is uses vis_mode = get_vis_mode(ch, room_no);
	//   'cause some people see in dark, some in light, some in both.
	// WTH does this for loop do?  Looks like crap to me.
	for (i = room; i != room2; i = room2)
	{
		if (world[i].people && (!IS_ROOM(i, ROOM_SILENT) || IS_LIGHT(i)))
		{
			if (IS_LIGHT(i))
			{
				LOOP_THRU_PEOPLE(t_ch, world[i].people)
				{
					if (IS_AWAKE(t_ch) && !IS_AFFECTED(t_ch, AFF_BLIND))
					{
						if (!IS_ROOM(i, ROOM_SILENT))
						{
							send_to_char(Gbuf2, t_ch);
						}
						else
						{
							send_to_char(Gbuf1, t_ch);
						}
					}
					else if (IS_AWAKE(t_ch))
					{
						send_to_char(Gbuf3, t_ch);
					}
				}
			}
			else
			{
				LOOP_THRU_PEOPLE(t_ch, world[i].people)
				{
					if (IS_AWAKE(t_ch))
					{
						send_to_char(Gbuf3, t_ch);
					}
				}
			}
		}
	}

	if (flag)
	{
		SET_BIT(world[room].dir_option[dir]->exit_info, EX_BLOCKED);
		SET_BIT(world[room2].dir_option[(int)rev_dir[dir]]->exit_info, EX_BLOCKED);
	}
	else
	{
		REMOVE_BIT(world[room].dir_option[dir]->exit_info, EX_BLOCKED);
		REMOVE_BIT(world[room2].dir_option[(int)rev_dir[dir]]->exit_info, EX_BLOCKED);
	}
}

/*
 * support function for nw_mirroid, reset the 'loops' in maze.  JAB
 */

void nw_reset_maze(int room)
{
	int other_room;

	if (world[room].number == BOUNDED(99202, world[room].number, 99206))
	{
		other_room = real_room0(world[room].number + 29);
		world[room].dir_option[3]->to_room = other_room;
		world[other_room].dir_option[1]->to_room = room;
	}
	else if (world[room].number == BOUNDED(99231, world[room].number, 99235))
	{
		other_room = real_room0(world[room].number - 29);
		world[room].dir_option[1]->to_room = other_room;
		world[other_room].dir_option[3]->to_room = room;
	}
	else if (world[room].number == 99201)
	{
		other_room = real_room0(99236);
		world[room].dir_option[1]->to_room = other_room;
		world[other_room].dir_option[3]->to_room = room;
	}
	else if (world[room].number == 99236)
	{
		other_room = real_room0(99201);
		world[room].dir_option[1]->to_room = other_room;
		world[other_room].dir_option[3]->to_room = room;
	}
}

/*
 * ok, mirroids are very xenophobic, and move away from anything that's not
 * another mirroid, blocking the passageway behind them in the process.
 * They are also very claustrophobic, and mildy agoraphobic, so if they
 * become trapped in a room with no exits, they WILL open one, and if they
 * are in a room with exits in all directions, they are real likely to close
 * one of them off.
 *
 * Mirroids can move and/or block or unblock an exit at the same time.
 *
 * Maze rooms are 99201-99236, and form a 6x6 wrap-around helical grid, they
 * are identical except for exits.  Room 99237 is the 'exit' room, Room 99200
 * is the 'entrance' room.  Rooms 99201 thru 99206 are the 'western edge',
 * and at most 1 (one) of them will hold the 'exit' to 99237.  Rooms 99231
 * thru 99236 are the 'eastern edge', and at most 1 (one) of them will hold
 * the 'entrance' to 99200.
 *
 * It's quite possible that ALL exits/entrances will be blocked, in which
 * case, anyone wanting to use them will just have to wait, until a mirroid
 * decides to unblock one.
 *
 * Mirroids are stay-zone and sentinel, this routine limits their movement to
 * the rooms 99201-99236.
 *
 * What makes this so devilishly complex, is we have may have to change up to
 * 5 dir_option[] entries, in up to 5 seperate rooms (2 pairs of maze rooms,
 * plus possibly an entrance/exit room) each time a mirroid (un)blocks an exit.
 *
 * 99200 dir 3 will always point to one of the rooms in range 99231-99236,
 * and 99237 dir 1 will always point to one of the rooms 99201-99206, and the
 * corresponding room will point back, even if the exit happens to be blocked.
 *
 * When we shift the exit, we have to restore the old loop, then kill then new
 * loop, then add the entrance/exit connection.
 */

int nw_mirroid(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	P_char t_ch = NULL;
	bool run_away = FALSE;
	int mode = 0, i, e_count = 0, c_dir = -1, e_dir = -1, e_room = 0;

	/*
	 * check for periodic event calls
	 */

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (pl || cmd || !IS_AWAKE(ch) || !CAN_ACT(ch))
		return FALSE;

	/*
	 * mode numbers: 0 - do nothing 1 - open an exit and stay (only way to open
	 * an exit/entrance) 2 - open an exit and use it 3 - close an exit and stay
	 * (only way to close an exit/entrance) 4 - use an exit and close it behind 5
	 *
	 * - use an exit and leave it unchanged
	 */

	/*
	 * count unblocked exits
	 */

	for (i = 0; i < 4; i++)
		if (EXIT(ch, i) && !IS_SET(EXIT(ch, i)->exit_info, EX_BLOCKED))
			e_count++;
	if (IS_FIGHTING(ch))
	{
		run_away = TRUE;

		/*
		 * preferable mode is 4, but failing that, we use 2
		 */
		if (e_count)
			mode = 4;
		else
			mode = 2;
	}
	else
	{
		/*
		 * we aren't fighting, so we check our parameters and pick a mode
		 */

		/*
		 * check for non-mirroids in our room
		 */
		LOOP_THRU_PEOPLE(t_ch, ch)
			if ((IS_PC(t_ch) || (GET_RNUM(t_ch) != GET_RNUM(ch))) && CAN_SEE(ch, t_ch))
			{
				run_away = TRUE;
				break;
			}
		if (run_away)
		{
			/*
			 * if there is an exit, mode 4, if not, mode 2
			 */
			if (e_count)
				mode = 4;
			else
				mode = 2;
		}
	}

	if (!mode)
	{
		/*
		 * ok, at this point, we aren't fighting, nor are there non-mirroids in our
		 *
		 * room (that we can see), so we can be a little more flexible.
		 */

		switch (e_count)
		{
		case 0:
			/*
				 * no exits, do we sit and think, or open a path?
				 */
			if (number(0, 9))
			{
				if (number(0, 1))
					mode = 1;
				else
					mode = 2;
			}
			break;
		case 1:
			/*
				 * one exit, really likely to want to open another, but possibly we
				 * close it.
				 */
			switch (number(0, 20))
			{
			case 0:
			case 1:
			case 2:
				break;
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
				mode = 1;
				break;
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
				mode = 2;
				break;
			case 16:
			case 17:
				mode = 3;
				break;
			case 18:
			case 19:
				mode = 4;
				break;
			case 20:
				mode = 5;
				break;
			}
			break;
		case 2:
			/*
				 * two exits, about equal probabilities on all options.
				 */
			switch (number(0, 20))
			{
			case 0:
			case 1:
				break;
			case 2:
			case 3:
			case 4:
			case 5:
				mode = 1;
				break;
			case 6:
			case 7:
			case 8:
			case 9:
				mode = 2;
				break;
			case 10:
			case 11:
			case 12:
			case 13:
				mode = 3;
				break;
			case 14:
			case 15:
			case 16:
			case 17:
				mode = 4;
				break;
			case 18:
			case 19:
			case 20:
				mode = 5;
				break;
			}
			break;
		case 3:
			/*
				 * three exits, more likely to close one than open a new one.
				 */
			switch (number(0, 20))
			{
			case 0:
				break;
			case 1:
			case 2:
				mode = 5;
				break;
			case 3:
			case 4:
				mode = 1;
				break;
			case 5:
			case 6:
			case 7:
				mode = 2;
				break;
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
				mode = 3;
				break;
			case 14:
			case 15:
			case 16:
			case 17:
			case 18:
			case 19:
			case 20:
				mode = 4;
				break;
			}
			break;
		case 4:
			/*
				 * four exits, REAL likely to close one
				 */
			switch (number(0, 20))
			{
			case 0:
			case 1:
				break;
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
				mode = 3;
				break;
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
			case 16:
			case 17:
			case 18:
			case 19:
				mode = 4;
				break;
			case 20:
				mode = 5;
				break;
			}
			break;
		}
	}
	if (!mode)
		return TRUE;

	/*
	 * quickie check on 'entrance' status
	 */

	if (world[ch->in_room].number == BOUNDED(99231, world[ch->in_room].number, 99236))
	{
		/*
		 * we are in one of the rooms that an exit is legal from
		 */
		e_dir = 1;
		for (i = 99231; i < 99237; i++)
			if (!IS_SET(EXIT(ch, 1)->exit_info, EX_BLOCKED) &&
			    (EXIT(ch, 1)->to_room == real_room(99200)))
			{
				e_room = i;
				break;
			}
		if (e_room)
		{
			if (real_room(e_room) == ch->in_room)
				e_room = -1;
		}
		else
			e_room = -2;
	}
	else if (world[ch->in_room].number == BOUNDED(99201, world[ch->in_room].number, 99206))
	{
		/*
		 * we are in one of the rooms that an exit is legal from
		 */
		e_dir = 3;
		for (i = 99201; i < 99207; i++)
			if (!IS_SET(EXIT(ch, 3)->exit_info, EX_BLOCKED) &&
			    (EXIT(ch, 3)->to_room == real_room(99237)))
			{
				e_room = i;
				break;
			}
		if (e_room)
		{
			if (real_room(e_room) == ch->in_room)
				e_room = -1;
		}
		else
			e_room = -2;
	}
	/*
	 * ok, e_room: -2 there is no exit, but we are in a room we can MAKE an exit
	 * from. -1 there is an exit, and it's in our room. 0 we aren't in a position
	 *
	 * to change the exit. + there is an exit, it's not in our room, but we can
	 * change it.
	 */

	/*
	 * at this point, we have a mode between 1 and 5, and we are aware of the
	 * exit status, now we have to make some final sanity checks, and actually DO
	 *
	 * something.
	 */

	switch (mode)
	{
	case 1: /*
		         * open up and sit still
		         */
	case 2: /*
		         * open up and use it
		         */
		if (e_room == -2)
		{
			/*
				 * no entrance/exit, but we can make one, do so, 50%
				 */
			if (number(0, 1))
				c_dir = e_dir;
		}
		else if (e_room > 0)
		{
			/*
				 * there IS an exit, but we might prefer having it here, change it 1 in
				 *
				 * 5
				 */
			if (!number(0, 4))
			{
				nw_reset_maze(e_room);
				c_dir = e_dir;
			}
		}
		if (c_dir < 0)
		{
			/*
				 * we haven't found an appropriate direction yet.
				 */

			for (c_dir = number(0, 3), i = 0; i < 4; i++, c_dir++)
			{
				if (c_dir > 3)
					c_dir = 0;

				if (IS_SET(EXIT(ch, c_dir)->exit_info, EX_BLOCKED))
					break;
			}
		}
		break;

	case 3: /*
		         * close up and sit still
		         */
	case 4: /*
		         * use an exit and close it behind us
		         */
	case 5: /*
		         * just wander
		         */

		for (c_dir = number(0, 3), i = 0; i < 4; i++, c_dir++)
		{
			if (c_dir > 3)
				c_dir = 0;

			if ((EXIT(ch, c_dir) && !IS_SET(EXIT(ch, c_dir)->exit_info, EX_BLOCKED)))
			{
				if ((mode == 3) ||
				    (world[EXIT(ch, c_dir)->to_room].number ==
				     BOUNDED(99201, world[EXIT(ch, c_dir)->to_room].number, 99236)))
					break;
			}
		}
		break;
	}

	if (c_dir == -1)
		return FALSE;

	if (mode < 3)
	{
		nw_block_exit(ch->in_room, c_dir, 0); /*
		                                       * unblock
		                                       */
		if (mode == 2)
			do_move(ch, 0, exitnumb_to_cmd(c_dir));
	}
	else if (mode == 3)
	{
		nw_block_exit(ch->in_room, c_dir, 1); /*
		                                       * block
		                                       */
	}
	else
	{
		int old_room = ch->in_room;

		do_move(ch, 0, exitnumb_to_cmd(c_dir));
		if (mode == 4)
			nw_block_exit(old_room, c_dir, 1); /*
			                                    * block
			                                    */
	}

	return TRUE;
}
