// Venthix's procs

#include <ctype.h>
#include <list>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <vector>
#include <algorithm>
#include <climits>
#include <memory>
#include <limits>
using namespace std;

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "combat/damage.h"
#include "world/map.h"
#include "world/specs.prototypes.h"
#include "specs/specs.venthix.h"
#include "magic/spells.h"

vector<ZombieGame *> zgames;

extern P_room world;
extern P_desc descriptor_list;
extern P_index mob_index;
extern struct zone_data *zone_table;

int roulette_pistol(P_obj obj, P_char ch, int cmd, char *arg)
{
	// The local of the live round in the pistol, 0 if none exist (fired)

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!obj || !IS_ALIVE(ch))
	{
		return FALSE;
	}

	if (arg && ((cmd == CMD_RELOAD) || (cmd == CMD_FIRE)) && isname(arg, obj->name))
	{
		if (ch->equipment[HOLD] != obj)
		{
			act("You must be holding the pistol to use it.", FALSE, ch, obj, 0,
			    TO_CHAR);
			return TRUE;
		}
	}

	if (arg && (cmd == CMD_RELOAD))
	{
		if (isname(arg, obj->name))
		{
			// Does the pistol still have a live round?
			if (obj->value[0])
			{
				act("&+yA live round is still loaded.  You spin the chamber and lock it.&n",
				    FALSE, ch, obj, 0, TO_CHAR);
				act("&+y$n opens the chamber and notices a live round already chambered.\n"
				    "&+y$e spins the chamber and locks it back.&n",
				    FALSE, ch, obj, 0, TO_ROOM);
			}

			// If no live round is found in the pistol...
			if (!obj->value[0])
			{
				act("&+y$n quickly reloads $p &+ywith a live round.", FALSE, ch,
				    obj, 0, TO_ROOM);
				act("&+yYou quickly reload $p &+ywith a live round.", FALSE, ch,
				    obj, 0, TO_CHAR);
			}

			// Randomize the live round...
			obj->value[0] = number(1, 6);
			return TRUE;
		}
	}

	if (arg && (cmd == CMD_FIRE))
	{
		if (isname(arg, obj->name))
		{
			if (!obj->value[0])
			{
				// Gun is empty, no live round.  Reload before playing...
				act("&+WClick!  &+yThere is no live round in the gun, reload you dummy!",
				    FALSE, ch, obj, 0, TO_CHAR);
				act("&+WClick!  &+y$n&+y tries to play roulette with an empty gun.",
				    FALSE, ch, obj, 0, TO_ROOM);
				return TRUE;
			}
			else
			{
				if ((obj->value[0] > 6) || (obj->value[0] < 0))
				{
					// Debug issue with code. Value out of acceptable parameters.
					act("&-RBANG!&n &+WThe gun backfires and you're dead!&n",
					    FALSE, ch, obj, 0, TO_CHAR);
					act("&-RBANG!&n &+W$n&+W's gun bacfired!&n", FALSE, ch, obj,
					    0, TO_ROOM);
					GET_HIT(ch) = (-100);
					return TRUE;
				}
				if (obj->value[0]-- != 1)
				{
					// CLICK, nothing happened, wew!
					act("&+WClick! &+yNothing happened... &+Lweeew!&n", FALSE,
					    ch, obj, 0, TO_CHAR);
					act("&+WClick! &+y$n&+y pulled the trigger, but nothing happened.",
					    FALSE, ch, obj, 0, TO_ROOM);
				}
				else
				{
					// BANG, your dead!
					act("&-RBANG!&n &+WYou're dead!&n", FALSE, ch, obj, 0,
					    TO_CHAR);
					act("&-RBANG!&n &+W$n&+W shot himself... What a loser!",
					    FALSE, ch, obj, 0, TO_ROOM);
					if (!IS_TRUSTED(ch))
					{
						// obj_from_char(unequip_char(ch, HOLD), FALSE);
						// obj_to_room(obj, ch->in_room);
						// act("$p&+y drops to the floor.", FALSE, ch, obj, 0, TO_ROOM);
						GET_HIT(ch) = (-100);
					}
					else
					{
						act("&+yThe bullet simply bounces off $n&+y's head.",
						    FALSE, ch, obj, 0, TO_ROOM);
						send_to_char("You can't die!\n", ch);
					}
				}
				return TRUE;
			}
		}
	}
	return FALSE;
}

// 67282 &+mt&+Mh&+me &+Mo&+mr&+Mb &+mo&+Mf &+md&+Me&+mc&+Me&+mp&+Mt&+mi&+Mo&+mn
int orb_of_deception(P_obj obj, P_char ch, int cmd, char *arg)
{
	int curr_time = time(NULL);

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch))
	{
		return FALSE;
	}

	if (arg && (cmd == CMD_SAY))
	{
		if (isname(arg, "mirage"))
		{
			// Every 3 min.
			if (((ch->equipment[WEAR_EARRING_L] == obj) ||
			     (ch->equipment[WEAR_EARRING_R] == obj)) &&
			    obj->timer[0] + 180 <= curr_time)
			{
				act("&+L$n&+L's $p &+Lbegins to vibrate.", FALSE, ch, obj, 0,
				    TO_ROOM);
				act("&+LYour $p &+Lbegins to vibrate.", FALSE, ch, obj, 0, TO_CHAR);
				spell_mirage(51, ch, 0, SPELL_TYPE_SPELL, ch, 0);
				obj->timer[0] = time(NULL);
				return TRUE;
			}
		}
	}
	return FALSE;
}

#define CANNON_PLAYER 0
#define CANNON_AMMO 1
#define CANNON_DIR 2
#define CANNON_FIRING 3

#define COORD_X 0
#define COORD_Y 1

#define M_PI 3.14159265358979323846

struct cannon_data
{
	int fuze; // fuze for countdown
	int ammo; // gunpowder 1-50
	int dir; // dir to go
	double x; // slope data x coordinates east and west
	double y; // slope data y coordinates north and south
	int doxy[2]; // make the move north south east or west
	int moved; // how many rooms has player moved
};

void get_cannon_heading(void *data)
{
	struct cannon_data *cdata = (struct cannon_data *)data;

	if (!cdata)
	{
		debug("Error: get_cannon_heading called with null pointer");
		return;
	}

	float rad = (float)cdata->dir * M_PI / 180.000;
	cdata->x += (float)sin(rad);
	cdata->y += (float)cos(rad);

	cdata->doxy[COORD_X] = -1;
	cdata->doxy[COORD_Y] = -1;

	if ((cdata->y >= 51.000) || (cdata->x >= 51.000) || (cdata->y < 50.000) ||
	    (cdata->x < 50.000))
	{
		if (cdata->x > 50.999)
		{
			cdata->x -= 1.000;
			cdata->doxy[COORD_X] = DIR_EAST;
		}
		else if (cdata->x < 50.000)
		{
			cdata->x += 1.000;
			cdata->doxy[COORD_X] = DIR_WEST;
		}
		if (cdata->y > 50.999)
		{
			cdata->y -= 1.000;
			cdata->doxy[COORD_Y] = DIR_NORTH;
		}
		else if (cdata->y < 50.000)
		{
			cdata->y += 1.000;
			cdata->doxy[COORD_Y] = DIR_SOUTH;
		}
	}
	debug("dir: %d, doxy[x]: %d, doxy[y]: %d, x: %f, y: %f", cdata->dir, cdata->doxy[COORD_X],
	      cdata->doxy[COORD_Y], cdata->x, cdata->y);
	return;
}

void event_super_cannon_fire(P_char ch, P_char /*vict*/, P_obj /*obj*/, void *data)
{
	struct cannon_data *cdata = (struct cannon_data *)data;
	int troom;
	int dir;

	if (!cdata)
	{
		debug("passed null pointer to event_super_cannon_fire");
		return;
	}

	if (!ch)
	{
		debug("event_super_cannon_fire called with !ch");
		return;
	}

	if (ch)
	{
		// Lets fly through the air!
		cdata->moved++;

		if (cdata->moved <= cdata->ammo / 2)
			ch->specials.z_cord =
				MIN((int)get_property("cannon.super.max.z", 15),
				    (int)((float)cdata->moved / ((float)cdata->ammo / 2) *
					  (float)cdata->ammo / 5));
		else
			ch->specials.z_cord =
				MIN((int)get_property("cannon.super.max.z", 15),
				    (int)(((float)cdata->ammo - (float)cdata->moved) /
					  ((float)cdata->ammo / 2) * (float)cdata->ammo / 5));

		// handle movement
		get_cannon_heading(cdata);

		for (int n = COORD_X; n <= COORD_Y; n++)
		{
			dir = -1;
			if (cdata->doxy[n] != -1 &&
			    (cdata->doxy[n] == DIR_NORTH || cdata->doxy[n] == DIR_EAST ||
			     cdata->doxy[n] == DIR_SOUTH || cdata->doxy[n] == DIR_WEST))
				dir = cdata->doxy[n];
			debug("dir: %d", dir);
			if (dir < -1 || dir > 3)
			{
				debug("Error in event_super_cannon_fire: invalid dir");
				send_to_char("Ack! Something went wrong!\r\n", ch);
				ch->specials.z_cord = 0;
				return;
			}

			if (dir == -1)
				continue;

			if (!EXIT(ch, dir) || EXIT(ch, dir)->to_room == NOWHERE ||
			    !IS_SURFACE_MAP(EXIT(ch, dir)->to_room))
			{
				send_to_char("&+WSMACK! &+LYou ran into something, that hurt!\r\n",
					     ch);
				ch->specials.z_cord = 0;
				spell_damage(ch, ch, 20, SPLDAM_GENERIC,
					     SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, 0);
				SET_POS(ch, GET_STAT(ch) + POS_PRONE);
				return;
			}
			troom = EXIT(ch, dir)->to_room;
			// can we enter next room or did we hit something bad?
			if (IS_FIGHTING(ch))
				stop_fighting(ch);

			if (ch->in_room != NOWHERE)
				for (P_char attacker = world[ch->in_room].people; attacker;
				     attacker = attacker->next)
					if (IS_FIGHTING(attacker) && (GET_OPPONENT(attacker) == ch))
						stop_fighting(attacker);

			if (IS_PC(ch) && IS_RIDING(ch))
				stop_riding(ch);
			if (P_char rider = GET_RIDER(ch))
				stop_riding(rider);

			char_from_room(ch);
			char_to_room(ch, troom, -1);
		}

		if (cdata->moved >= cdata->ammo)
		{
			// landed!
			ch->specials.z_cord = 0;
			if (world[ch->in_room].sector_type == SECT_WATER_SWIM ||
			    world[ch->in_room].sector_type == SECT_WATER_NOSWIM ||
			    world[ch->in_room].sector_type == SECT_OCEAN)
			{
				send_to_char(
					"&+LYou &+Bsplash &+Linto the &+Bwater &+Las you land!\r\n",
					ch);
			}
			else
			{
				send_to_char(
					"&+LYou've landed!  You roll as you hit the ground.&n\r\n",
					ch);
				if (!affected_by_spell(ch, SPELL_FLY) &&
				    GET_CHAR_SKILL(ch, SKILL_SAFE_FALL) >= number(0, 100))
				{
					send_to_char(
						"&+WOuch! &+LYou should try a fly spell next time.&n\r\n",
						ch);
					spell_damage(ch, ch, 20, SPLDAM_GENERIC,
						     SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, 0);
				}
				SET_POS(ch, GET_STAT(ch) + POS_PRONE);
			}
			return;
		}

		add_event(event_super_cannon_fire, 1, ch, 0, 0, 0, data,
			  sizeof(struct cannon_data));
	}
}

void event_super_cannon(P_char /*pl*/, P_char /*vict*/, P_obj obj, void *data)
{
	struct cannon_data *cdata = (struct cannon_data *)data;
	P_char ch = NULL;
	int offsetaim;

	if (!cdata)
	{
		debug("Passed null pointer to event_super_cannon");
		obj->value[CANNON_PLAYER] = 0;
		return;
	}

	// If our projectile still in the room?
	if (obj->loc.room > 0)
	{
		for (P_char tch = world[obj->loc.room].people; tch; tch = tch->next_in_room)
		{
			if (IS_PC(tch) && GET_PID(tch) == obj->value[CANNON_PLAYER])
			{
				ch = tch;
				break;
			}
		}
		if (!ch || !IS_ALIVE(ch))
		{
			obj->value[CANNON_PLAYER] = 0;
			return;
		}
	}
	else
	{
		obj->value[CANNON_PLAYER] = 0;
		return;
	}

	cdata->fuze--;

	if (cdata->fuze == 2)
	{
		// perform lighting of fuze
		act("&+LA &+Rgnomish &+Ct&+Bi&+Mn&+Rk&+Ge&+Yr &+Lruns in from the s&+bh&+La&+bd&+Lo&+bw&+Ls and &+rl&+Ri&+rgh&+Rt&+rs &+Lthe fuze before everyone is ready!",
		    FALSE, ch, obj, 0, TO_CHAR);
		act("&+LA &+Rgnomish &+Ct&+Bi&+Mn&+Rk&+Ge&+Yr &+Lruns in from the s&+bh&+La&+bd&+Lo&+bw&+Ls and &+rl&+Ri&+rgh&+Rt&+rs &+Lthe fuze before everyone is ready!",
		    FALSE, ch, obj, 0, TO_ROOM);
		add_event(event_super_cannon, WAIT_SEC, 0, 0, obj, 0, data,
			  sizeof(struct cannon_data));
		return;
	}
	else if (cdata->fuze == 1)
	{
		// perform fuze burning msg
		act("&+LThe fuze &+Rsi&+rzz&+Rl&+re&+Rs &+Lquickly...&n", FALSE, ch, obj, 0,
		    TO_CHAR);
		act("&+LThe fuze &+Rsi&+rzz&+Rl&+re&+Rs &+Lquickly...&n", FALSE, ch, obj, 0,
		    TO_ROOM);
		add_event(event_super_cannon, WAIT_SEC, 0, 0, obj, 0, data,
			  sizeof(struct cannon_data));
		return;
	}
	else if (cdata->fuze == 0)
	{
		// perform firing event on player
		act("&-RBOOOOOOOOOOOOOOOOM!", FALSE, ch, obj, 0, TO_CHAR);
		act("&-RBOOOOOOOOOOOOOOOOM!", FALSE, ch, obj, 0, TO_ROOM);
		act("$n has a shocked look on his face as he flies out of $p.", FALSE, ch, obj, 0,
		    TO_ROOM);
		obj->value[CANNON_PLAYER] = 0;
		obj->value[CANNON_FIRING] = FALSE;
		cdata->x = 50.500;
		cdata->y = 50.500;
		cdata->doxy[COORD_X] = -1;
		cdata->doxy[COORD_Y] = -1;
		cdata->moved = 0;
		offsetaim = cdata->ammo / 50;
		cdata->dir += number(-offsetaim, offsetaim); // So the aim isn't perfect
		add_event(event_super_cannon_fire, 0, ch, 0, 0, 0, data,
			  sizeof(struct cannon_data));
		return;
	}
}

int native_birth_super_cannon_initialize(P_obj obj)
{
	// Make sure the cannon is reset on object creation.
	for (int i = 0; i < 6; ++i)
		obj->value[i] = 0;
	return FALSE;
}

int super_cannon(P_obj obj, P_char ch, int cmd, char *arg)
{
	char load[MAX_STRING_LENGTH], dirstr[MAX_STRING_LENGTH];
	char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int found = 0;

	struct cannon_data cdata;
	cdata.fuze = 3;

	if (cmd == CMD_SET_PERIODIC)
	{
		return native_birth_super_cannon_initialize(obj);
	}

	if (cmd == CMD_PERIODIC)
		return FALSE;

	if (!obj || !ch)
		return FALSE;

	if (!IS_PC(ch))
		return FALSE;

	if (arg && (cmd == CMD_ORDER))
	{
		half_chop(arg, arg1, arg2);
		if (!isname(arg1, "cannon"))
			return FALSE;

		// If player isn't in room, reset the cannon for another's use.
		for (P_char tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
		{
			if (IS_PC(tch) && GET_PID(tch) == obj->value[CANNON_PLAYER])
				found = TRUE;
		}
		if (!found)
		{
			obj->value[CANNON_PLAYER] = 0;
			obj->value[CANNON_FIRING] = FALSE;
		}
		if (obj->value[CANNON_PLAYER] != GET_PID(ch) && obj->value[CANNON_PLAYER] != 0)
		{
			act("$p is already in use!", FALSE, ch, obj, 0, TO_CHAR);
			return TRUE;
		}

		argument_interpreter(arg2, load, dirstr);

		if (!*load || !*dirstr || !isdigit(*load) || !isdigit(*dirstr))
		{
			act("Please specify the load and direction.  Example: Order Cannon 10 45",
			    FALSE, ch, 0, 0, TO_CHAR);
			return TRUE;
		}
		if (atoi(load) <= 0)
		{
			act("Zero ammo wont get you very far.", FALSE, ch, 0, 0, TO_CHAR);
			return TRUE;
		}
		if (atoi(load) > (int)get_property("cannon.super.range", 500))
		{
			snprintf(
				buf, MAX_STRING_LENGTH,
				"Are you trying to blow up the cannon? That's too much gunpowder! Try %d or less.",
				(int)get_property("cannon.super.range", 500));
			act(buf, FALSE, ch, 0, 0, TO_CHAR);
			return TRUE;
		}

		obj->value[CANNON_AMMO] = atoi(load);
		obj->value[CANNON_DIR] = atoi(dirstr);
		obj->value[CANNON_PLAYER] = GET_PID(ch);
		snprintf(
			buf, MAX_STRING_LENGTH,
			"&+L$n &+Lloads &+L$p &+Lwith %d pounds of gunpowder and tweaks it's aim.  Grinning, $e hops in.",
			atoi(load));
		act(buf, FALSE, ch, obj, 0, TO_ROOM);
		snprintf(buf, MAX_STRING_LENGTH,
			 "&+LYou load $p &+Lwith %d pounds of gunpowder and aim it.", atoi(load));
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
		return TRUE;
	}

	if (arg && (cmd == CMD_SHOUT) && obj->value[CANNON_FIRING] != TRUE)
	{
		if (isname(arg, "fire"))
		{
			if (obj->value[CANNON_PLAYER] == 0)
			{
				act("$p isn't primed for firing yet.", FALSE, ch, obj, 0, TO_CHAR);
				return TRUE;
			}

			if (obj->value[CANNON_PLAYER] != GET_PID(ch))
			{
				act("$p is already in use!", FALSE, ch, obj, 0, TO_CHAR);
				return TRUE;
			}

			if (!IS_SURFACE_MAP(ch->in_room))
			{
				act("Sorry, this cannon only works on the surface.", FALSE, ch, 0,
				    0, TO_CHAR);
				obj->value[CANNON_PLAYER] = 0;
				return TRUE;
			}
			obj->value[CANNON_FIRING] = TRUE;
			cdata.ammo = obj->value[CANNON_AMMO];
			cdata.dir = obj->value[CANNON_DIR];
			add_event(event_super_cannon, WAIT_SEC, 0, 0, obj, 0, &cdata,
				  sizeof(cdata));
			return FALSE;
		}
	}
	return FALSE;
}

void halloween_mine_proc(P_char ch)
{
	char buff[MAX_STRING_LENGTH];
	snprintf(buff, MAX_STRING_LENGTH, " %s 86", GET_NAME(ch));
	act("Your dig hits a burried &+ypumpkin&n.\n"
	    "Suddenly it begins to move and digs itself out of the mine!",
	    TRUE, ch, 0, 0, TO_CHAR);
	act("$n dig hits a burried &+ypumpkin&n.\n"
	    "Suddenly it begins to move and digs itself out of the mine!",
	    TRUE, ch, 0, 0, TO_ROOM);
	do_givepet(ch, buff, CMD_GIVEPET);
}

#define ZTIMER_STANDBY 0 // standby timer
#define ZTIMER_WAVE 1 // wave timer

#define ZOMBIES_ID 0 // ZombiesGame Class id
#define ZOMBIES_STATUS 1 // game 0=off, 1=on, 2=standby
#define ZOMBIES_LEVEL 2 // current game level
#define ZOMBIES_WAVE 3 // wave # of the level

// Same original local constructor for normal and private-born stock. Global
// game enrollment/mob assignment remains at its existing ordinary call site.
static ZombieGame *native_birth_zombie_initialize(P_obj obj)
{
	obj->value[ZOMBIES_STATUS] = FALSE;
	return new ZombieGame(obj);
}
static size_t retained_birth_zombie_reservations = 0;
bool quest_mobile_native_zombie_stage::prepare(P_obj object,
					       quest_mobile_native_zombie_stage &output) noexcept
{
	extern int top_of_mobt;
	if (!nevent_is_game_thread() || !object || !object->obj_uid || output.game_ || !mob_index ||
	    ZombieGame::next_id <= 0 || ZombieGame::next_id == INT_MAX ||
	    retained_birth_zombie_reservations == std::numeric_limits<size_t>::max() ||
	    zgames.size() >= zgames.max_size() ||
	    retained_birth_zombie_reservations >= zgames.max_size() - zgames.size())
		return false;
	const int number = real_mobile0(87);
	if (number < 0 || number > top_of_mobt || mob_index[number].virtual_number != 87)
		return false;
	try
	{
		std::unique_ptr<ZombieGame> game(native_birth_zombie_initialize(object));
		zgames.reserve(zgames.size() + retained_birth_zombie_reservations + 1);
		object->value[ZOMBIES_ID] = game->id;
		output.mob_rnum_ = number;
		output.original_mob_index_ = mob_index;
		output.original_mob_proc_ = mob_index[number].func.mob;
		output.item_uid_ = object->obj_uid;
		output.game_ = game.release();
		++retained_birth_zombie_reservations;
		return true;
	}
	catch (...)
	{
		return false;
	}
}
bool quest_mobile_native_zombie_stage::restore(P_obj object,
					       quest_mobile_native_zombie_stage &output) noexcept
{
	extern int top_of_mobt;
	if (!nevent_is_game_thread() || !object || !object->obj_uid || output.game_ || !mob_index ||
	    object->value[ZOMBIES_ID] <= 0 || object->value[ZOMBIES_ID] == INT_MAX ||
	    object->value[ZOMBIES_STATUS] != FALSE || ZombieGame::next_id <= 0 ||
	    retained_birth_zombie_reservations == std::numeric_limits<size_t>::max() ||
	    zgames.size() >= zgames.max_size() ||
	    retained_birth_zombie_reservations >= zgames.max_size() - zgames.size())
		return false;
	const int number = real_mobile0(87);
	if (number < 0 || number > top_of_mobt || mob_index[number].virtual_number != 87)
		return false;
	for (const auto *game : zgames)
		if (!game || game->id == object->value[ZOMBIES_ID] || game->generator == object)
			return false;
	try
	{
		std::unique_ptr<ZombieGame> game(new ZombieGame());
		game->generator = object;
		game->id = object->value[ZOMBIES_ID];
		// Off-state birth has no spawned zombies or current round. Periodic code
		// never reads this counter until its original start command sets it.
		game->zombies_to_load = 0;
		zgames.reserve(zgames.size() + retained_birth_zombie_reservations + 1);
		output.mob_rnum_ = number;
		output.original_mob_index_ = mob_index;
		output.original_mob_proc_ = mob_index[number].func.mob;
		output.item_uid_ = object->obj_uid;
		output.game_ = game.release();
		++retained_birth_zombie_reservations;
		// Reserve the already retained ephemeral ID; do not allocate a new ID or
		// publish a generator. Subsequent original construction must not collide.
		if (ZombieGame::next_id <= object->value[ZOMBIES_ID])
			ZombieGame::next_id = object->value[ZOMBIES_ID] + 1;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool quest_mobile_native_zombie_stage::publish(P_obj object) noexcept
{
	if (!nevent_is_game_thread() || !object || !game_ || game_->generator != object ||
	    object->obj_uid != item_uid_ || object->value[ZOMBIES_ID] != game_->id ||
	    object->value[ZOMBIES_STATUS] != FALSE || !game_->zombies.empty() ||
	    !retained_birth_zombie_reservations || zgames.size() >= zgames.capacity() ||
	    mob_index != original_mob_index_ || real_mobile0(87) != mob_rnum_ ||
	    (mob_index[mob_rnum_].func.mob != original_mob_proc_ &&
	     mob_index[mob_rnum_].func.mob != zgame_mob_proc))
		return false;
	// All fallible allocation happened during original private preparation.
	// Consume before enrolling; no callback or further object dereference follows.
	ZombieGame *game = game_;
	game_ = nullptr;
	zgames.push_back(game);
	--retained_birth_zombie_reservations;
	mob_index[mob_rnum_].func.mob = zgame_mob_proc;
	return true;
}
bool quest_mobile_native_zombie_stage::discard() noexcept
{
	if (!game_)
		return true;
	if (!nevent_is_game_thread() || !retained_birth_zombie_reservations ||
	    !game_->zombies.empty() ||
	    std::find(zgames.begin(), zgames.end(), game_) != zgames.end())
		return false;
	game_->generator = nullptr;
	delete game_; // Original unload sees an empty private vector, no extraction.
	game_ = nullptr;
	--retained_birth_zombie_reservations;
	return true;
}

// Zombies game: loads zombies per round until all players are dead. :)
// Need to setup a zombies class to load zombie vectors per spawner item.
int zombies_game(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_desc i;
	char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
	char arg3[MAX_STRING_LENGTH], arg4[MAX_STRING_LENGTH];
	char buff[MAX_STRING_LENGTH], buff2[MAX_STRING_LENGTH];
	int level = 0;
	int zone = 0;
	int num = 0;
	int palive = 0;
	int max_level = (int)get_property("zombies.game.maxlevel", 99);
	int zombies;

	if (!obj)
	{
		return FALSE;
	}
	zombies = zg_count_zombies(obj);

	if (cmd == CMD_SET_PERIODIC)
	{
		ZombieGame *zgame = native_birth_zombie_initialize(obj);
		zgames.push_back(zgame);
		obj->value[ZOMBIES_ID] = zgame->id;
		mob_index[real_mobile0(87)].func.mob = zgame_mob_proc;
		return TRUE;
	}

	ZombieGame *zgame = get_zgame_from_obj(obj);

	if (OBJ_ROOM(obj))
	{
		zone = world[obj->loc.room].zone;
	}
	else
	{
		debug("Zombies Game generator obj not placed in room (perhaps in someones inventory?");
		return FALSE;
	}

	// how many players left in the zone.
	for (i = descriptor_list; i; i = i->next)
	{
		if (!i->connected && !IS_TRUSTED(i->character) && (GET_ZONE(i->character) == zone))
		{
			palive++;
		}
	}

	if (cmd == CMD_PERIODIC)
	{
		// if game is off, don't load zombies
		if (obj->value[ZOMBIES_STATUS] == 0)
		{
			return TRUE;
		}

		// zombie count returns -1, somethings wrong, remove the object
		if (zombies < 0)
		{
			debug("Error with zg_count_zombies(): can't find zombie game data");
			send_to_zone(
				zone,
				"&+RGame has ended due to technical difficulties, please notify a god.&n\r\n");
			extract_obj(obj, TRUE); // Not an arti, but 'in game.'
			return TRUE;
		}

		// If game is active
		if (obj->value[ZOMBIES_STATUS] == 1)
		{
			// zombies are all dead, set to standby, and prepare for next round
			if (zombies == 0 && zgame->zombies_to_load <= 0)
			{
				if (obj->value[ZOMBIES_LEVEL] >= max_level)
				{
					// reached end of max level end the game
					obj->value[ZOMBIES_LEVEL] = 0;
					obj->value[ZOMBIES_STATUS] = 0;
					send_to_zone(
						zone,
						"&+WYou've completed the last level, you're game is complete.  Congrats!&n\r\n");
					return TRUE;
				}
				else
				{
					// set game to standby mode to setup the next round
					obj->value[ZOMBIES_STATUS] = 2;
					obj->timer[ZTIMER_STANDBY] = time(NULL);
					snprintf(
						buff, MAX_STRING_LENGTH,
						"&+WYou've completed round &+c%d&+W.  Prepare for the next round.&n\r\n",
						obj->value[ZOMBIES_LEVEL]);
					send_to_zone(zone, buff);
					return TRUE;
				}
			}
			else if (!palive)
			{
				snprintf(
					buff, MAX_STRING_LENGTH,
					"&+WAll players are dead, the game has ended on level &+c%d&+W.&n\r\n",
					obj->value[ZOMBIES_LEVEL]);
				send_to_zone(zone, buff);
				obj->value[ZOMBIES_STATUS] = 0;
				obj->timer[ZTIMER_STANDBY] = 0;
				obj->timer[ZTIMER_WAVE] = 0;
				zgame_clear_zombies(obj);
				return TRUE;
			}
			else
			{
				// Load zombies!!!
				if ((obj->timer[ZTIMER_WAVE] +
					     (int)get_property("zombies.game.spawn.delay", 30) <=
				     time(NULL)) ||
				    (!zombies && !number(0, 2)))
				{
					obj->value[ZOMBIES_WAVE]++;
					// Waves handling
					int max = 0;
					int waves = 0;
					int load = 0;
					char loadstr[MAX_STRING_LENGTH];
					snprintf(loadstr, MAX_STRING_LENGTH, "%d",
						 (int)get_property("zombies.game.load", 3445));
					char single[MAX_STRING_LENGTH];
					while (loadstr[waves])
					{
						snprintf(single, MAX_STRING_LENGTH, "%c",
							 loadstr[waves]);
						load = atoi(single);
						waves++;
						max += load;
					}
					snprintf(single, MAX_STRING_LENGTH, "%c",
						 loadstr[obj->value[ZOMBIES_WAVE] - 1]);
					load = atoi(single);
					debug("loadstr: %s, load: %d, max: %d, waves: %d", loadstr,
					      load, max, waves);

					// Eventually change this to leave the remaining
					//   to redistribute through waves as harder mobs/bosses
					if (obj->value[ZOMBIES_WAVE] >= waves)
					{
						num = zgame->zombies_to_load;
					}
					else
					{
						num = MAX(1, (int)(zgame->zombies_to_load * load /
								   max));
					}
					num = BOUNDED(1, num, zgame->zombies_to_load);
					if (num)
					{
						obj->timer[ZTIMER_WAVE] = time(NULL);
					}
					debug("loading %d zombies", num);
					for (int i = 0; i < num; i++)
					{
						if (zgame_load_zombie(obj))
						{
							zgame->zombies_to_load--;
						}
					}
					return TRUE;
				}
				return TRUE;
			}
		}
		int delay = BOUNDED(10,
				    (int)get_property("zombies.game.standby.delay", 15) *
					    (obj->value[ZOMBIES_LEVEL] / 10),
				    120);
		// If 2 minutes have passed and we are in standby mode begin next round
		if (obj->value[ZOMBIES_STATUS] == 2 &&
		    (obj->timer[ZTIMER_STANDBY] + delay) <= time(NULL))
		{
			obj->timer[ZTIMER_STANDBY] = 0;
			obj->timer[ZTIMER_WAVE] = 0;
			obj->value[ZOMBIES_STATUS] = 1;
			obj->value[ZOMBIES_LEVEL]++;
			obj->value[ZOMBIES_WAVE] = 0;
			// Need to scale the amount of mobs per level here
			zgame->zombies_to_load =
				MAX(obj->value[ZOMBIES_LEVEL],
				    ((obj->value[ZOMBIES_LEVEL] + (palive * 2)) * palive / 2));
			snprintf(buff, MAX_STRING_LENGTH,
				 "&+WRound &+c%d &+Wis beginning.  Good Luck!&n\r\n",
				 obj->value[ZOMBIES_LEVEL]);
			send_to_zone(zone, buff);
			return TRUE;
		}
		return TRUE;
	}

	if (!IS_ALIVE(ch) || IS_NPC(ch))
	{
		return FALSE;
	}

	if (cmd == CMD_HELP && arg && strstr(arg, "zombies") && IS_TRUSTED(ch))
	{
		half_chop(arg, arg1, arg2);
		if (isname(arg2, "begin"))
		{
			// set game to standby mode to setup the next round
			obj->value[ZOMBIES_STATUS] = 2;
			obj->timer[ZTIMER_STANDBY] =
				time(NULL) - (10); // should be 10 seconds to start
			send_to_zone(zone, "&+WThe game has started.&n\r\n");
			return TRUE;
		}
		else if (isname(arg2, "end"))
		{
			obj->value[ZOMBIES_STATUS] = 0;
			obj->timer[ZTIMER_STANDBY] = 0;
			snprintf(buff, MAX_STRING_LENGTH,
				 "&+WThe game has been stopped by %s.&n\r\n", GET_NAME(ch));
			send_to_zone(zone, buff);
			zgame_clear_zombies(obj);
			return TRUE;
		}
		else if (isname(arg2, "status"))
		{
			snprintf(buff, MAX_STRING_LENGTH, "&+WZombies Game Status&n\r\n");
			snprintf(buff2, MAX_STRING_LENGTH, "&+L           Status&+W: &+c%s&n\r\n",
				 (obj->value[ZOMBIES_STATUS] == 1 ? "On" :
				  obj->value[ZOMBIES_STATUS] == 0 ? "Off" :
								    "Standby"));
			strcat(buff, buff2);
			snprintf(buff2, MAX_STRING_LENGTH, "&+L            Round&+W: &+c%d&n\r\n",
				 obj->value[ZOMBIES_LEVEL]);
			strcat(buff, buff2);
			snprintf(buff2, MAX_STRING_LENGTH, "&+L    Players Alive&+W: &+c%d&n\r\n",
				 palive);
			strcat(buff, buff2);
			snprintf(buff2, MAX_STRING_LENGTH, "&+L    Zombies Alive&+W: &+c%d&n\r\n",
				 zombies);
			strcat(buff, buff2);
			snprintf(buff2, MAX_STRING_LENGTH, "&+LZombies Remaining&+W: &+c%d&n\r\n",
				 zgame->zombies_to_load);
			strcat(buff, buff2);
			send_to_char(buff, ch);
			return TRUE;
		}
		else if (strstr(arg2, "level"))
		{
			argument_interpreter(arg2, arg3, arg4);
			if (isdigit(*arg4))
			{
				level = atoi(arg4);
			}
			else
			{
				send_to_char("Please specify a number for the level.\r\n", ch);
				return TRUE;
			}
			if (level < 0 || level > max_level)
			{
				send_to_char("Sorry that level is too high or low.\r\n", ch);
				return TRUE;
			}
			obj->value[ZOMBIES_LEVEL] = level;
			snprintf(buff, MAX_STRING_LENGTH,
				 "&+WZombies Game level set to: &+c%d&n.\r\n", level);
			send_to_char(buff, ch);
			return TRUE;
		}
		else
		{
			send_to_char("&+WZombies Game Help&n\r\n", ch);
			send_to_char("&+LSyntax: &+wzombies &+W[&+cargument&+W]&n\r\n", ch);
			send_to_char("&+cArguments:&n\r\n", ch);
			send_to_char("&+W* &+c begin&+W: &nBegin the zombies game.\r\n", ch);
			send_to_char("&+W* &+c   end&+W: &nEnd the game.\r\n", ch);
			send_to_char("&+W* &+c level&+W: &nSet game level.\r\n", ch);
			send_to_char("&+W* &+cstatus&+W: &nView game status.\r\n", ch);
			send_to_char("&+W* &+c  help&+W: &nThis help listing.\r\n", ch);
			send_to_char("&n\r\n", ch);
			return TRUE;
		}
		return TRUE;
	}
	return FALSE;
}

int zgame_load_zombie(P_obj obj)
{
	P_char z;
	P_desc i;

	if (!obj)
		return 0;

	if (!OBJ_ROOM(obj))
		return 0;

	int zone = world[obj->loc.room].zone;

	ZombieGame *zgame = get_zgame_from_obj(obj);

	if (!zgame)
		return 0;

	z = read_mobile(87, VIRTUAL);

	if (!z)
		return 0;

	// Can randomize here where we want the zombie to load within the zone
	GET_BIRTHPLACE(z) = obj->loc.room;

	char_to_room(z, obj->loc.room, 0);
	CLEAR_MONEY(z);

	// Set memory for hunting
	for (i = descriptor_list; i; i = i->next)
	{
		if (!i->connected && !IS_TRUSTED(i->character) && (GET_ZONE(i->character) == zone))
			remember(z, i->character);
	}

	zgame->zombies.push_back(z);

	return 1;
}

void zgame_clear_zombies(P_obj obj)
{
	if (!obj)
		return;

	ZombieGame *zgame = get_zgame_from_obj(obj);

	if (!zgame)
		return;

	while (zgame->zombies.size())
	{
		P_char z = zgame->zombies.back();
		zgame->zombies.pop_back();

		if (!z)
			continue;

		extract_char(z);
	}

	return;
}

// finds zombies game class object from obj and counts alive
// zombies in the mob vector.
int zg_count_zombies(P_obj obj)
{
	if (!obj)
		return -1;

	ZombieGame *zgame = get_zgame_from_obj(obj);

	if (!zgame)
		return -1;

	return (int)zgame->zombies_alive();
}

ZombieGame *get_zgame_from_obj(P_obj obj)
{
	for (size_t i = 0; i < zgames.size(); i++)
	{
		if (zgames[i] && zgames[i]->id == obj->value[ZOMBIES_ID])
			return zgames[i];
	}
	return NULL;
}

ZombieGame *get_zgame_from_zombie(P_char z)
{
	if (!z)
		return NULL;

	for (size_t i = 0; i < zgames.size(); i++)
	{
		for (size_t j = 0; j < zgames[i]->zombies.size(); j++)
		{
			if (zgames[i]->zombies[j] == z)
				return zgames[i];
		}
	}

	return NULL;
}

int zgame_mob_proc(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC || cmd == CMD_PERIODIC)
		return FALSE;

	SET_BIT(ch->specials.act, ACT_SPEC_DIE);

	if (cmd == CMD_DEATH)
	{
		ZombieGame *zgame = get_zgame_from_zombie(ch);

		if (!zgame)
			return FALSE;

		for (size_t i = 0; i < zgame->zombies.size(); i++)
		{
			if (zgame->zombies[i] == ch)
				// This ends up moving all the iterators past this point
				// so if this becomes too inefficient when we have too many
				// mobs (ie high level), we might have to rework how we
				// are keeping track of mobs.
				zgame->zombies.erase(zgame->zombies.begin() + i);
		}
		return FALSE;
	}

	return FALSE;
}

int ZombieGame::next_id = 1;

ZombieGame::ZombieGame()
	: generator(NULL)
{
}

ZombieGame::ZombieGame(P_obj _generator)
	: generator(_generator)
{
	load();
}

ZombieGame::~ZombieGame()
{
	unload();
}

int ZombieGame::load()
{
	id = ZombieGame::next_id++;
	return TRUE;
}

int ZombieGame::unload()
{
	while (zombies.size())
	{
		P_char zombie = zombies.back();
		zombies.pop_back();

		if (!zombie)
			continue;

		extract_char(zombie);
	}
	return TRUE;
}

bool quest_mobile_native_zombie_stage::observe_published(P_obj object) noexcept
{
	extern int top_of_mobt;
	if (!nevent_is_game_thread() || !object || !object->obj_uid ||
	    object->value[ZOMBIES_ID] <= 0 || object->value[ZOMBIES_STATUS] != FALSE)
		return false;
	const ZombieGame *matched = nullptr;
	for (const auto *game : zgames)
		if (game && (game->id == object->value[ZOMBIES_ID] || game->generator == object))
		{
			if (matched || game->id != object->value[ZOMBIES_ID] ||
			    game->generator != object || !game->zombies.empty())
				return false;
			matched = game;
		}
	const int number = real_mobile0(87);
	return matched && mob_index && number >= 0 && number <= top_of_mobt &&
	       mob_index[number].virtual_number == 87 &&
	       mob_index[number].func.mob == zgame_mob_proc;
}

// Serialized owning registry observation only; no enrollment authority.
bool quest_mobile_native_zombie_registry_storage_bytes(size_t *output) noexcept
{
	if (!output || !nevent_is_game_thread())
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return false;
#else
	size_t bytes = sizeof(zgames);
	if (zgames.capacity() > (SIZE_MAX - bytes) / sizeof(ZombieGame *))
		return false;
	bytes += zgames.capacity() * sizeof(ZombieGame *);
	for (const auto *game : zgames)
	{
		if (!game || sizeof(ZombieGame) > SIZE_MAX - bytes)
			return false;
		bytes += sizeof(ZombieGame);
		if (game->zombies.capacity() > (SIZE_MAX - bytes) / sizeof(P_char))
			return false;
		bytes += game->zombies.capacity() * sizeof(P_char);
	}
	*output = bytes;
	return true;
#endif
}

bool quest_mobile_native_zombie_stage::publish_bounded(P_obj object,
						       bool (*reserve)(size_t, void *) noexcept,
						       void *context, size_t outer_live) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)object;
	(void)reserve;
	(void)context;
	(void)outer_live;
	return false;
#else
	// The original publish has one named pointer and no allocation: prepare /
	// restore already reserved the registry slot. The caller retains both the
	// current registry and this private game until ownership transfers.
	if (!reserve || sizeof(ZombieGame *) > SIZE_MAX - outer_live ||
	    !reserve(outer_live + sizeof(ZombieGame *), context))
		return false;
	return publish(object);
#endif
}

bool quest_mobile_native_zombie_stage::restore_bounded(P_obj object,
						       quest_mobile_native_zombie_stage &output,
						       bool (*reserve)(size_t, void *) noexcept,
						       void *context, size_t outer_live) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)object;
	(void)output;
	(void)reserve;
	(void)context;
	(void)outer_live;
	return false;
#else
	extern int top_of_mobt;
	if (!reserve || !nevent_is_game_thread() || !object || !object->obj_uid || output.game_ ||
	    !mob_index || object->value[ZOMBIES_ID] <= 0 || object->value[ZOMBIES_ID] == INT_MAX ||
	    object->value[ZOMBIES_STATUS] != FALSE || ZombieGame::next_id <= 0 ||
	    retained_birth_zombie_reservations == std::numeric_limits<size_t>::max() ||
	    zgames.size() >= zgames.max_size() ||
	    retained_birth_zombie_reservations >= zgames.max_size() - zgames.size())
		return false;
	const int number = real_mobile0(87);
	if (number < 0 || number > top_of_mobt || mob_index[number].virtual_number != 87)
		return false;
	for (const auto *game : zgames)
		if (!game || game->id == object->value[ZOMBIES_ID] || game->generator == object)
			return false;
	// Caller owns current global registry exactly once; output is empty.
	struct frame
	{
		size_t live, registry, requested, replacement;
	};
	constexpr size_t locals = sizeof(frame) + sizeof(std::unique_ptr<ZombieGame>) + sizeof(int);
	if (locals > SIZE_MAX - outer_live || !reserve(outer_live + locals, context))
		return false;
	frame work{ outer_live + locals, 0, 0, 0 };
	if (!quest_mobile_native_zombie_registry_storage_bytes(&work.registry) ||
	    work.registry > outer_live || sizeof(ZombieGame) > SIZE_MAX - work.live)
		return false;
	work.live += sizeof(ZombieGame);
	if (!reserve(work.live, context))
		return false;
	try
	{
		std::unique_ptr<ZombieGame> game(new ZombieGame());
		game->generator = object;
		game->id = object->value[ZOMBIES_ID];
		// Off-state birth has no spawned zombies or current round. Periodic code
		// never reads this counter until its original start command sets it.
		game->zombies_to_load = 0;
		work.requested = zgames.size() + retained_birth_zombie_reservations + 1;
		if (work.requested > zgames.capacity())
		{
			if (work.requested > SIZE_MAX / sizeof(ZombieGame *))
				return false;
			work.replacement = work.requested * sizeof(ZombieGame *);
			// Actual reserve keeps old current vector and this private game live.
			if (work.replacement > SIZE_MAX - work.live ||
			    !reserve(work.live + work.replacement, context))
				return false;
		}
		zgames.reserve(work.requested);
		output.mob_rnum_ = number;
		output.original_mob_index_ = mob_index;
		output.original_mob_proc_ = mob_index[number].func.mob;
		output.item_uid_ = object->obj_uid;
		output.game_ = game.release();
		++retained_birth_zombie_reservations;
		// Reserve the already retained ephemeral ID; do not allocate a new ID or
		// publish a generator. Subsequent original construction must not collide.
		if (ZombieGame::next_id <= object->value[ZOMBIES_ID])
			ZombieGame::next_id = object->value[ZOMBIES_ID] + 1;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

#include <cerrno>
#include <stdexcept>
// Native-only original warm preparation with fresh genuine CURRENT admission.
// Outer excludes G and includes the existing private candidate/stage prefix.
bool quest_mobile_native_zombie_stage::prepare_bounded(
	P_obj object, quest_mobile_native_zombie_stage &output,
	bool (*current_global)(size_t *, void *) noexcept, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
#if !defined(__linux__) || !defined(__LP64__) || !defined(_GLIBCXX_RELEASE) || \
	_GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || !_GLIBCXX_USE_CXX11_ABI
	(void)object;
	(void)output;
	(void)current_global;
	(void)reserve;
	(void)context;
	(void)outer_live;
	errno = ENOTSUP;
	return false;
#else
	extern int top_of_mobt;
	struct frame
	{
		size_t global = 0, game = 0, requested = 0, replacement = 0, live = 0, locals = 0;
		int number = -1, failure = 0, saved_errno = errno;
	} work;
	if (!current_global || !reserve || !nevent_is_game_thread())
	{
		errno = EINVAL;
		return false;
	}
	auto observe = [&](size_t pending) noexcept -> bool
	{
		errno = 0;
		if (!current_global(&work.global, context))
		{
			if (!work.failure)
				work.failure = EIO;
			return false;
		}
		if (work.locals > SIZE_MAX - outer_live ||
		    work.game > SIZE_MAX - outer_live - work.locals ||
		    pending > SIZE_MAX - outer_live - work.locals - work.game ||
		    work.global > SIZE_MAX - outer_live - work.locals - work.game - pending)
		{
			if (!work.failure)
				work.failure = EOVERFLOW;
			return false;
		}
		work.live = outer_live + work.locals + work.game + pending + work.global;
		errno = 0;
		if (!reserve(work.live, context))
		{
			if (!work.failure)
				work.failure = errno ? errno : ENOBUFS;
			return false;
		}
		return true;
	};
	work.locals = sizeof(work) + sizeof(observe) + sizeof(size_t) +
		      sizeof(std::unique_ptr<ZombieGame>) + sizeof(object) + sizeof(&output) +
		      sizeof(current_global) + sizeof(reserve) + sizeof(context) +
		      sizeof(outer_live);
	if (!observe(0))
	{
		errno = work.failure;
		return false;
	}
	if (!object || !object->obj_uid || output.game_ || !mob_index || ZombieGame::next_id <= 0 ||
	    ZombieGame::next_id == INT_MAX ||
	    retained_birth_zombie_reservations == std::numeric_limits<size_t>::max() ||
	    zgames.size() >= zgames.max_size() ||
	    retained_birth_zombie_reservations >= zgames.max_size() - zgames.size())
	{
		errno = EINVAL;
		return false;
	}
	work.number = real_mobile0(87);
	if (work.number < 0 || work.number > top_of_mobt ||
	    mob_index[work.number].virtual_number != 87)
	{
		errno = EINVAL;
		return false;
	}
	if (!observe(sizeof(ZombieGame)))
	{
		errno = work.failure;
		return false;
	}
	try
	{
		// Exactly the original initializer/constructor/load and ID/status order.
		// The original game has no allocated zombie-vector entries at creation.
		std::unique_ptr<ZombieGame> game(native_birth_zombie_initialize(object));
		work.game = sizeof(ZombieGame);
		work.requested = zgames.size() + retained_birth_zombie_reservations + 1;
		if (work.requested > zgames.capacity())
		{
			if (work.requested > SIZE_MAX / sizeof(ZombieGame *))
				work.failure = EOVERFLOW;
			else
				work.replacement = work.requested * sizeof(ZombieGame *);
		}
		// Current G retains the old registry allocation while reserve allocates
		// its replacement. This private game is owned locally, outside G.
		if (!work.failure && observe(work.replacement))
		{
			zgames.reserve(work.requested);
			work.replacement = 0;
			if (observe(0))
			{
				object->value[ZOMBIES_ID] = game->id;
				output.mob_rnum_ = work.number;
				output.original_mob_index_ = mob_index;
				output.original_mob_proc_ = mob_index[work.number].func.mob;
				output.item_uid_ = object->obj_uid;
				output.game_ = game.release();
				++retained_birth_zombie_reservations;
				errno = work.saved_errno;
				return true;
			}
		}
		// The original failed prepare destroys the private, empty game. Keep
		// its consumed ephemeral ID/status effects and any enlarged registry.
	}
	catch (const std::length_error &)
	{
		work.failure = EOVERFLOW;
	}
	catch (...)
	{
		work.failure = ENOMEM;
	}
	work.game = 0;
	(void)observe(0); // Fresh G after cleanup; first refusal remains authoritative.
	errno = work.failure ? work.failure : EIO;
	return false;
#endif
}

#include <cerrno>
#include <new>
#include <stdexcept>
// PRIVATE UNSEALED: genuine immediate CMD_SET_PERIODIC shell constructor.
// Outer excludes G and this helper's retained shell_game_, but includes the
// actual listed probe's private heap and the complete live caller prefix.
namespace
{
struct shell_zombie_budget
{
	bool (*current_global)(size_t *, void *) noexcept;
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	ZombieGame *const *private_game;
	size_t exclusive = 0;
	int refusal = 0;
	bool fail(int error) noexcept
	{
		if (!refusal)
			refusal = error;
		errno = refusal;
		return false;
	}
	~shell_zombie_budget()
	{
		if (refusal)
			errno = refusal;
	}
	bool private_bytes(size_t *output) noexcept
	{
		size_t bytes = 0;
		if (*private_game)
		{
			bytes = sizeof(ZombieGame);
			if ((*private_game)->zombies.capacity() >
			    (SIZE_MAX - bytes) / sizeof(P_char))
				return fail(EOVERFLOW);
			bytes += (*private_game)->zombies.capacity() * sizeof(P_char);
		}
		*output = bytes;
		return true;
	}
	bool live(size_t *output, size_t pending = 0) noexcept
	{
		size_t global = 0, private_heap = 0;
		const int saved = errno;
		errno = 0;
		if (!current_global(&global, context))
			return fail(errno ? errno : EIO);
		errno = saved;
		if (!private_bytes(&private_heap))
			return false;
		if (private_heap > SIZE_MAX - exclusive ||
		    pending > SIZE_MAX - exclusive - private_heap ||
		    global > SIZE_MAX - exclusive - private_heap - pending)
			return fail(EOVERFLOW);
		*output = exclusive + private_heap + pending + global;
		return true;
	}
	bool request(size_t pending = 0) noexcept
	{
		size_t full = 0;
		if (!live(&full, pending))
			return false;
		const int saved = errno;
		errno = 0;
		if (!reserve(full, context))
			return fail(errno ? errno : ENOBUFS);
		errno = saved;
		return true;
	}
};

// Complete source carriers for the original constructor/load and genuine
// GNU13 pointer-vector push_back/reallocation path. No heap for the empty
// ZombieGame::zombies vector; its inline representation is in sizeof(game).
constexpr size_t shell_zombie_constructor_frames =
	// native_birth_zombie_initialize(obj), new expression, ZombieGame(obj), load.
	sizeof(P_obj) + sizeof(void *) + 2 * sizeof(P_obj) + 2 * sizeof(void *) + sizeof(int) +
	// std::vector<P_char> default ctor/base/impl/data ctor this carriers.
	4 * sizeof(void *) +
	// Actual vector<ZombieGame*>::push_back(const T&), allocator construct,
	// _M_realloc_insert(pos,args), len/old start/finish/new start/finish/elements.
	3 * sizeof(void *) + 3 * sizeof(void *) + sizeof(size_t) + 6 * sizeof(void *) +
	// _M_check_len(n,s): this/n/s/len + size/max_size/_S_max_size allocator/diffmax.
	2 * sizeof(void *) + 2 * sizeof(size_t) + 6 * sizeof(void *) + 3 * sizeof(size_t) +
	// _M_allocate -> allocator_traits::allocate -> new_allocator::allocate.
	3 * sizeof(void *) + 3 * sizeof(size_t) + 3 * sizeof(void *) + sizeof(size_t) +
	// Pointer relocation: _S_relocate/_S_nothrow_relocate, __relocate_a/a_1,
	// n, memcpy source/destination/request; address/normal iterator carriers.
	12 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(ptrdiff_t) + 3 * sizeof(void *) +
	// Old allocation deallocate and _Vector_impl_data pointer replacement.
	8 * sizeof(void *) + 3 * sizeof(size_t) +
	// Real zg_count_zombies -> get_zgame_from_obj loop -> zombies_alive.
	2 * sizeof(P_obj) + 2 * sizeof(ZombieGame *) + sizeof(size_t) + 2 * sizeof(void *) +
	// Real real_mobile0(virt) original binary-search bot/top/mid carriers.
	4 * sizeof(int);
}

bool quest_mobile_native_zombie_stage::shell_retained_private_storage_bytes(
	size_t *output) const noexcept
{
	if (!output || !nevent_is_game_thread())
	{
		errno = EINVAL;
		return false;
	}
#if !defined(__linux__) || !defined(__LP64__) || !defined(_GLIBCXX_RELEASE) ||                   \
	_GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || !_GLIBCXX_USE_CXX11_ABI || \
	defined(_GLIBCXX_DEBUG)
	errno = ENOTSUP;
	return false;
#else
	size_t bytes = 0;
	if (shell_game_)
	{
		bytes = sizeof(ZombieGame);
		if (shell_game_->zombies.capacity() > (SIZE_MAX - bytes) / sizeof(P_char))
		{
			errno = EOVERFLOW;
			return false;
		}
		bytes += shell_game_->zombies.capacity() * sizeof(P_char);
	}
	*output = bytes;
	return true;
#endif
}

bool quest_mobile_native_zombie_stage::initialize_shell_bounded(
	P_obj object, quest_mobile_native_zombie_stage &output, int *periodic, bool *returned,
	bool (*current_global)(size_t *, void *) noexcept, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	if (!periodic || !returned || !current_global || !reserve)
	{
		errno = EINVAL;
		return false;
	}
	*returned = false;
#if !defined(__linux__) || !defined(__LP64__) || !defined(_GLIBCXX_RELEASE) ||                   \
	_GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || !_GLIBCXX_USE_CXX11_ABI || \
	defined(_GLIBCXX_DEBUG)
	errno = ENOTSUP;
	return false;
#else
	extern int top_of_mobt;
	const int saved_errno = errno;
	shell_zombie_budget budget{ current_global, reserve, context, &output.shell_game_ };
	const size_t frames = sizeof(budget) + sizeof(object) + sizeof(&output) + sizeof(periodic) +
			      sizeof(returned) + sizeof(current_global) + sizeof(reserve) +
			      sizeof(context) + sizeof(outer_live) + 8 * sizeof(int) +
			      3 * sizeof(size_t) + 3 * sizeof(bool) + 3 * sizeof(void *) +
			      shell_zombie_constructor_frames;
	if (frames > SIZE_MAX - outer_live)
		return budget.fail(EOVERFLOW);
	budget.exclusive = outer_live + frames;
	if (!budget.request())
		return false;
	if (!nevent_is_game_thread() || !object || !object->obj_uid || output.shell_game_ ||
	    !mob_index || top_of_mobt < 0 || ZombieGame::next_id <= 0 ||
	    ZombieGame::next_id == INT_MAX)
		return budget.fail(EINVAL);
	// Authenticate the existing catalog before original effects, without moving
	// the real_mobile0 call from its original position after registry/ID writes.
	int matching = 0;
	int previous = -1;
	for (int number = 0; number <= top_of_mobt; ++number)
	{
		if (mob_index[number].virtual_number <= previous)
			return budget.fail(EINVAL);
		previous = mob_index[number].virtual_number;
		if (mob_index[number].virtual_number == 87)
			++matching;
	}
	if (matching != 1)
		return budget.fail(EINVAL);
	// Full original fractional property lookup/fuss/wizlog remains mandatory.
	// The provider's completed-return marker survives later admission denial.
	size_t property_outer = 0;
	if (!budget.live(&property_outer))
		return false;
	int max_level = 0;
	bool property_returned = false;
	const bool property_ok = get_property_int_bounded("zombies.game.maxlevel", 99, &max_level,
							  &property_returned, current_global,
							  reserve, context, property_outer, true);
	if (!property_ok || !property_returned)
		return budget.fail(errno ? errno : EIO);
	// These values are still evaluated in the actual original order, even though
	// CMD_SET_PERIODIC does not consume max_level or the existing zombie count.
	(void)max_level;
	const int zombies = zg_count_zombies(object);
	(void)zombies;
	if (!budget.request(sizeof(ZombieGame)))
		return false;
	try
	{
		// Status then original constructor/load/ID issuance. Latch private ownership
		// immediately after new, before any prospective registry request can refuse.
		output.shell_game_ = native_birth_zombie_initialize(object);
		size_t replacement = 0;
		if (zgames.size() == zgames.capacity())
		{
			// Actual GNU13 _M_check_len(1) policy, used by push_back itself below.
			if (zgames.size() == zgames.max_size())
				return budget.fail(EOVERFLOW);
			const size_t old = zgames.size(), extra = old ? old : 1;
			size_t length = old + extra;
			if (length < old || length > zgames.max_size())
				length = zgames.max_size();
			if (length > SIZE_MAX / sizeof(ZombieGame *))
				return budget.fail(EOVERFLOW);
			replacement = length * sizeof(ZombieGame *);
		}
		// Old registry allocation is already in fresh G; the true replacement and
		// private game are admitted beside it, without reserve+publish substitution.
		if (!budget.request(replacement))
			return false;
		ZombieGame *game = output.shell_game_;
		zgames.push_back(game);
		output.shell_game_ = nullptr; // Actual registry now owns it; G counts it once.
		object->value[ZOMBIES_ID] = game->id;
		mob_index[real_mobile0(87)].func.mob = zgame_mob_proc;
		*periodic = TRUE;
		*returned = true; // Original callback returned, before fallible fresh G.
		if (!budget.request())
			return false;
		errno = saved_errno;
		return true;
	}
	catch (const std::length_error &)
	{
		return budget.fail(EOVERFLOW);
	}
	catch (const std::bad_alloc &)
	{
		return budget.fail(ENOMEM);
	}
	catch (...)
	{
		return budget.fail(EIO);
	}
#endif
}

// Exact original super_cannon local type remains in its owning translation
// unit, including installed-ABI padding. The getter grants no callback permit.
size_t native_mobile_birth_super_cannon_source_frame_bytes() noexcept
{
	return sizeof(P_obj) + sizeof(P_char) + sizeof(int) + sizeof(char *) +
	       5 * MAX_STRING_LENGTH * sizeof(char) + sizeof(int) + sizeof(cannon_data) +
	       sizeof(P_obj) + sizeof(int) +
	       2 * sizeof(int); // initialize(obj), loop i, both returned int results
}
