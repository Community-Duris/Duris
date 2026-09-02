/*****************************************************
 * ship_auto.c
 *
 * Ship autopilot
 *****************************************************/

/*
 * OVERVIEW -- where this file sits in the ship system
 * ---------------------------------------------------
 * This is the *player* autopilot: the "order sail <heading> <rooms>" command
 * that makes a ship steer itself to a map room some distance away and then
 * stop.  It is deliberately simple and is NOT the NPC combat brain; that lives
 * in ship_npc_ai.c (struct NPCShipAI, reached through ship->npc_ai).
 *
 * The two AIs are independent and a ship may in principle carry both:
 *   ship->autopilot  -> struct shipai_data   (this file, ship_auto.h)
 *   ship->npc_ai     -> struct NPCShipAI     (ship_npc_ai.c/.h)
 *
 * Lifecycle of ship->autopilot
 * ----------------------------
 *   assign_autopilot()  allocates (once) and re-initialises the block, then
 *                       sets AIB_ENABLED.
 *   engage_autopilot()  the command entry point; parses the player's heading
 *                       and distance, resolves a target room and sets
 *                       AIB_AUTOPILOT + AIM_AUTOPILOT.
 *   autopilot_activity() one tick of steering; called from ship_activity()
 *                       in ship_base.c for every loaded ship.
 *   stop_autopilot()    clears the enable bits but keeps the allocation.
 *   clear_autopilot()   frees the allocation (used when a ship sinks).
 *
 * Coordinate conventions used below
 * ---------------------------------
 * getmap() (ship_utils.c) fills the global tactical_map[101][101] with the
 * 101x101 patch of ocean centred on the ship, and ShipData::x / ::y are the
 * ship's own indices into that patch -- they sit at (50, 50) for a normal
 * ship (see reset_ship() in ship_base.c).  The map's y axis is inverted with
 * respect to compass north, which is why every lookup here reads
 * tactical_map[x][100 - y].
 *
 * Bearings are compass degrees: 0 = north, 90 = east, 180 = south, 270 = west.
 */

#include "core/prototypes.h"
#include "net/comm.h"
#include "world/db.h"
#include "cmd/interp.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "world/graph.h"
#include "item/objmisc.h"
// #include "magic/spells.h"
#include "core/structs.h"
#include "world/events.h"
#include "core/utils.h"
#include "world/map.h"
#include "ships/ship_auto.h"

extern char buf[MAX_STRING_LENGTH];

/*
 * Highest tactical_map[] index that getmap() populates.  The array is
 * [101][101], so valid subscripts are 0..100 on both axes.
 */
#define AUTOPILOT_MAP_MAX 100

/*
 * Largest distance, in map rooms, that "order sail" will accept.  Kept as a
 * named constant so the guard and the error message cannot drift apart.
 */
#define AUTOPILOT_MAX_DIST 35

// Internal variables
/*
 * Unused module-level handle, kept only so initialize_shipai() has something
 * to reset.  Nothing reads it; the live autopilot state always hangs off
 * ShipData::autopilot instead.
 */
static shipai_data *autopilot;

/*
 * Reset the module-level autopilot handle.
 *
 * Historical boot hook.  Nothing in the tree calls it today (the autopilot
 * keeps no global state that needs initialising), but it is kept so the
 * subsystem still has an obvious entry point if module-level state is ever
 * reintroduced.
 */
void initialize_shipai()
{
	autopilot = NULL;
}

/*
 * Attach a fresh, enabled autopilot block to `ship`.
 *
 * Allocates ship->autopilot on first use and otherwise reuses the existing
 * allocation, wiping it back to defaults.  All timers are zeroed, no target
 * or group is set, and AIB_ENABLED is raised.
 *
 * Returns FALSE without touching anything when `ship` is NULL or when the
 * ship already has an *enabled* autopilot (so an in-progress course is never
 * silently discarded); TRUE when the block was (re)initialised.
 */
int assign_autopilot(P_ship ship)
{
	struct shipai_data *ai;
	int i;

	if (!ship)
	{
		return FALSE;
	}
	if (ship->autopilot)
	{
		if (IS_SET(ship->autopilot->flags, AIB_ENABLED))
		{
			return FALSE;
		}
		else
		{
			ai = ship->autopilot;
		}
	}
	else
	{
		CREATE(ai, shipai_data, 1, MEM_TAG_SHIPAI);

		ship->autopilot = ai;
	}

	ai->type = 0;
	ai->target = NULL;
	ai->flags = 0;
	ai->t_room = 0;
	ai->ship = ship;
	ai->group = NULL;
	for (i = 0; i < MAXAITIMER; i++)
	{
		ai->timer[i] = 0;
	}
	SET_BIT(ai->flags, AIB_ENABLED);
	return TRUE;
}

/*
 * Free the ship's autopilot block entirely and null the pointer.
 *
 * Use this when the ship itself is going away (sinking, deletion).  To merely
 * cancel a course and keep the allocation for reuse, call stop_autopilot().
 * Safe to call on a ship that has no autopilot.
 */
void clear_autopilot(P_ship ship)
{
	if (ship->autopilot)
		FREE(ship->autopilot);
	ship->autopilot = 0;
}

/*
 * Cancel an engaged course, leaving the autopilot block allocated.
 *
 * Only reports "Autopilot stopped." when a course was actually running
 * (both AIB_ENABLED and AIB_AUTOPILOT set), so the routine can be called
 * unconditionally from steering and damage paths.  Does nothing when the ship
 * has never been given an autopilot.
 */
void stop_autopilot(P_ship ship)
{
	/*
	 * Callers reach this from ordinary steering/damage paths, which run for
	 * ships that were never given an autopilot block at all.
	 */
	if (!ship || !ship->autopilot)
		return;

	if (IS_SET(ship->autopilot->flags, AIB_ENABLED))
	{
		if (IS_SET(ship->autopilot->flags, AIB_AUTOPILOT))
		{
			REMOVE_BIT(ship->autopilot->flags, AIB_AUTOPILOT);
			REMOVE_BIT(ship->autopilot->flags, AIB_ENABLED);
			act_to_all_in_ship(ship, "Autopilot stopped.");
		}
	}
}

/*
 * Command handler for "order sail <heading> <rooms>".
 *
 * `arg1` is either a compass heading in degrees (0-359), one of the four
 * cardinal names/abbreviations (north/n, east/e, south/s, west/w), or "off"
 * to disengage.  `arg2` is the distance to travel in map rooms
 * (0..AUTOPILOT_MAX_DIST).
 *
 * On success the target room is projected `dist + 1` rooms along `dir` from
 * the ship's own tactical_map position, stored in autopilot->t_room, and
 * AIB_AUTOPILOT / AIM_AUTOPILOT are raised so autopilot_activity() takes over
 * steering on the next tick.
 *
 * `ch` may be NULL (scripted use); all player feedback is then suppressed.
 * Always returns TRUE -- the command is considered handled either way, with
 * failures reported to `ch` rather than through the return value.
 */
int engage_autopilot(P_char ch, P_ship ship, char *arg1, char *arg2)
{
	int dir = 0;
	if (is_number(arg1))
	{
		dir = atoi(arg1);
		if (dir < 0 || dir > 359)
		{
			if (ch)
				send_to_char("0-359 degrees or N E S W!\r\n", ch);
			return TRUE;
		}
	}
	else
	{
		if (isname(arg1, "north n"))
			dir = 0;
		else if (isname(arg1, "east e"))
			dir = 90;
		else if (isname(arg1, "south s"))
			dir = 180;
		else if (isname(arg1, "west w"))
			dir = 270;
		else if (isname(arg1, "off"))
		{
			if (ship->autopilot)
			{
				if (!IS_SET(ship->autopilot->flags, AIB_ENABLED) &&
				    !IS_SET(ship->autopilot->flags, AIB_AUTOPILOT))
				{
					if (ch)
						send_to_char(
							"There IS no active autopilot atm!\r\n",
							ch);
					return TRUE;
				}
				else
				{
					REMOVE_BIT(ship->autopilot->flags, AIB_ENABLED);
					REMOVE_BIT(ship->autopilot->flags, AIB_AUTOPILOT);
					act_to_all_in_ship(ship, "Autopilot disengaged.");
					return TRUE;
				}
			}
			else
			{
				if (ch)
					send_to_char("There IS no active autopilot atm!\r\n", ch);
				return TRUE;
			}
		}
		else
		{
			if (ch)
				send_to_char(
					"Valid syntax is order sail <N/E/S/W/Heading> <number of rooms>\r\n",
					ch);
			return TRUE;
		}
	}
	int dist = 0;
	if (is_number(arg2))
	{
		dist = atoi(arg2);
		/*
		 * `dist` feeds the tactical_map[] subscripts below, so a negative
		 * value has to be rejected here and not merely clamped: it would
		 * project the target far outside the 101x101 patch.
		 */
		if (dist < 0 || dist > AUTOPILOT_MAX_DIST)
		{
			if (ch)
				send_to_char("Maximum number of rooms is 35!\r\n", ch);
			return TRUE;
		}
	}
	else
	{
		if (ch)
			send_to_char(
				"Valid syntax is order sail <N/E/S/W/Heading> <number of rooms>\r\n",
				ch);
		return TRUE;
	}
	if (ship->autopilot)
		REMOVE_BIT(ship->autopilot->flags, AIB_ENABLED);

	/* Refuse to plot a course while the ship is off the ocean map. */
	if (!getmap(ship))
		return TRUE;

	assign_autopilot(ship);

	/*
	 * Project `dist + 1` rooms along the requested compass heading.  sin()
	 * gives the eastward component and cos() the northward one; the map's y
	 * axis runs the other way, hence the "100 - y" below.
	 */
	float rad = (float)((float)dir * M_PI / 180);
	int xdist = (int)(sin(rad) * (dist + 1));
	int ydist = (int)(cos(rad) * (dist + 1));
	int map_x = (int)(xdist + ship->x);
	int map_y = AUTOPILOT_MAP_MAX - (int)(ydist + ship->y);

	/*
	 * ship->x / ship->y are normally (50, 50), which keeps the projection
	 * well inside the patch, but they are float state that other subsystems
	 * write (ramming copies the target's position, flying nudges them).
	 * Clamp before subscripting so a stale or unusual position cannot walk
	 * off the ends of tactical_map[].
	 */
	map_x = BOUNDED(0, map_x, AUTOPILOT_MAP_MAX);
	map_y = BOUNDED(0, map_y, AUTOPILOT_MAP_MAX);

	ship->autopilot->t_room = tactical_map[map_x][map_y].rroom;

	SET_BIT(ship->autopilot->flags, AIB_AUTOPILOT);
	ship->autopilot->mode = AIM_AUTOPILOT;
	act_to_all_in_ship_f(ship, "Autopilot engaged, heading %d for %d rooms. Target room is %d",
			     dir, dist, world[ship->autopilot->t_room].number);
	return TRUE;
}

/*
 * Detach `autopilot` from its ship group, freeing the group node.
 *
 * Group nodes (struct shipgroup_data) form a singly linked list hanging off
 * the leader; each member points at its own node.  When the leader leaves,
 * the remaining nodes are either destroyed (AIB_DRONE fleets) or re-pointed
 * at a new leader (AIB_MOB fleets).
 *
 * Returns FALSE when there was nothing to detach, TRUE otherwise.
 *
 * NOTE FOR MAINTAINERS: ship groups are currently dormant -- neither this
 * routine nor shipgroupadd() has a caller anywhere in the tree, and no
 * shipai_data ever gets a non-NULL ->group.  It is retained as the reference
 * implementation for fleet behaviour rather than as live code, so treat its
 * details as unverified against a running game.
 */
int shipgroupremove(struct shipai_data *autopilot)
{
	struct shipgroup_data *tmpgroup;
	struct shipgroup_data *tmpgroup2;
	struct shipai_data *leader = NULL;

	if (!autopilot)
	{
		return FALSE;
	}
	if (!autopilot->group)
	{
		return FALSE;
	}

	if (autopilot->group->leader == autopilot)
	{
		if (!autopilot->group->next)
		{
			autopilot->group->leader = NULL;
			autopilot->group->ai = NULL;
			FREE(autopilot->group);
			autopilot->group = NULL;
			return TRUE;
		}
		tmpgroup2 = autopilot->group;
		tmpgroup = autopilot->group->next;
		tmpgroup2->leader = NULL;
		tmpgroup2->ai = NULL;
		FREE(tmpgroup2);
		autopilot->group = NULL;

		while (tmpgroup)
		{
			if (IS_SET(autopilot->flags, AIB_DRONE))
			{
				// Destroy drone code
				tmpgroup2 = tmpgroup;
				tmpgroup = tmpgroup->next;
				tmpgroup2->ai->group = NULL;
				tmpgroup2->leader = NULL;
				tmpgroup2->ai = NULL;
				FREE(tmpgroup2);
				continue;
			}
			if (IS_SET(autopilot->flags, AIB_MOB))
			{
				if (!leader)
				{
					tmpgroup->leader = autopilot;
					leader = autopilot;
				}
				else
				{
					tmpgroup->leader = leader;
				}
				if (IS_SET(autopilot->flags, AIB_HUNTER))
				{
					autopilot->mode = AIM_SEEK;
				}
				else
				{
					autopilot->mode = AIM_WAIT;
				}
			}
			tmpgroup = tmpgroup->next;
		}
		return TRUE;
	}
	tmpgroup2 = autopilot->group;
	if (!tmpgroup2->leader || !tmpgroup2->leader->group)
	{
		return FALSE;
	}
	tmpgroup = tmpgroup2->leader->group;
	while (tmpgroup && tmpgroup->next != tmpgroup2)
	{
		tmpgroup = tmpgroup->next;
	}
	if (!tmpgroup)
	{
		return FALSE;
	}
	tmpgroup->next = tmpgroup2->next;
	tmpgroup2->leader = NULL;
	tmpgroup2->ai = NULL;
	FREE(tmpgroup2);
	autopilot->group = NULL;
	return TRUE;
}

/*
 * Add `autopilot` to a ship group, creating the group when `group` is NULL.
 *
 * Passing group == NULL makes `autopilot` the leader of a brand new one-ship
 * group.  Passing an existing group appends a node at the tail that inherits
 * that group's leader.  Either way autopilot->group ends up pointing at the
 * node that represents this ship.
 *
 * Returns FALSE when `autopilot` is NULL or already belongs to a group,
 * TRUE once the node is linked in.
 *
 * NOTE FOR MAINTAINERS: dormant, exactly as described on shipgroupremove().
 */
int shipgroupadd(struct shipai_data *autopilot, struct shipgroup_data *group)
{
	struct shipgroup_data *newgroup;
	struct shipgroup_data *tmpgroup;

	if (!autopilot)
	{
		return FALSE;
	}
	if (autopilot->group)
	{
		return FALSE;
	}
	if (!group)
	{
		CREATE(group, shipgroup_data, 1, MEM_TAG_SHIPGRP);

		group->leader = autopilot;
		group->next = NULL;
		group->ai = autopilot;
		autopilot->group = group;
		return TRUE;
	}
	else
	{
		CREATE(newgroup, shipgroup_data, 1, MEM_TAG_SHIPGRP);

		newgroup->leader = group->leader;
		newgroup->ai = autopilot;
		newgroup->next = NULL;
		tmpgroup = group;
		while (tmpgroup->next)
		{
			tmpgroup = tmpgroup->next;
		}
		tmpgroup->next = newgroup;
		autopilot->group = newgroup;
		return TRUE;
	}
}

/*
 * Tell everyone aboard that the autopilot changed the ship's set heading.
 */
void announceheading(P_ship ship, int heading)
{
	act_to_all_in_ship_f(ship, "AI:Heading changed to %d", heading);
}

/*
 * Tell everyone aboard that the autopilot changed the ship's set speed.
 */
void announcespeed(P_ship ship, int speed)
{
	act_to_all_in_ship_f(ship, "AI:Speed changed to %d", speed);
}

/*
 * Ask for a new set speed on behalf of the autopilot.
 *
 * `speed` is capped at the ship's current maxspeed, and the change is only
 * applied (and announced) when it actually differs from the standing order,
 * which keeps the tick from spamming the crew every pulse.
 */
void aishipspeedadjust(P_ship ship, int speed)
{
	if (speed > ship->maxspeed)
	{
		speed = ship->maxspeed;
	}
	if (speed != ship->setspeed)
	{
		ship->setspeed = speed;
		announcespeed(ship, ship->setspeed);
	}
}

/*
 * Run one autopilot tick for `ship`.
 *
 * Called once per ship from ship_activity() (ship_base.c).  Returns
 * immediately unless the ship has an autopilot block with AIB_ENABLED set.
 *
 * Only AI_LINE is implemented: it rescans the freshly built tactical map for
 * the stored target room, steers onto the bearing to it, and throttles back
 * as the ship closes -- full speed beyond 2.5 rooms, then 60 / 30 / 10, and a
 * full stop on arrival (which also clears AIB_AUTOPILOT and AIB_ENABLED and
 * reports "Destination has been reached.").  While the heading error exceeds
 * 30 degrees the ship is stopped so it can turn on the spot.
 *
 * AI_STOP and AI_PATH are placeholders with no behaviour yet.
 */
void autopilot_activity(P_ship ship)
{
	struct shipai_data *ai;
	int i, j, k, b, x, y, head;
	float r;

	if (ship->autopilot == NULL)
	{
		return;
	}
	if (!IS_SET(ship->autopilot->flags, AIB_ENABLED))
	{
		return;
	}
	ai = ship->autopilot;
	/* Ship AI starts here */

	switch (ai->type)
	{
	case AI_LINE:
		if (!getmap(ship))
			return;
		/*
		 * Sweep the whole patch for the target room.  k stays 0 when the
		 * target has drifted out of scanning range, in which case x and y
		 * are never read below.
		 */
		k = 0;
		for (i = 0; i < 101; i++)
		{
			for (j = 0; j < 101; j++)
			{
				if (tactical_map[i][100 - j].rroom == ai->t_room)
				{
					k = 1;
					x = i;
					y = j;
					b = bearing(50, 50, i, j);
					if ((int)ship->setheading != b &&
					    ai->t_room != ship->location)
					{
						ship->setheading = b;
						announceheading(ship, b);
					}
				}
			}
		}
		if (k)
		{
			/* Absolute heading error, in degrees, still to be turned off. */
			head = ship->setheading - ship->heading;
			if (head < 0)
			{
				head *= -1;
			}
			// If we're off by more than 30 degrees.
			if (head > 30)
			{
				aishipspeedadjust(ai->ship, 0);
			}
			else
			{
				if (ship->location == ai->t_room)
				{
					aishipspeedadjust(ai->ship, 0);
					if (IS_SET(ai->flags, AIB_AUTOPILOT))
					{
						REMOVE_BIT(ai->flags, AIB_AUTOPILOT);
						REMOVE_BIT(ai->flags, AIB_ENABLED);
						act_to_all_in_ship(ship,
								   "Destination has been reached.");
						return;
					}
				}
				else
				{
					/* Throttle back as the target room draws closer. */
					r = 0;
					r = range(50, 50, 0, x, y, 0);
					if (IS_SET(ai->flags, AIB_BATTLER))
					{
						aishipspeedadjust(ship, ship->get_maxspeed());
					}
					else if (r < 0.50)
					{
						aishipspeedadjust(ai->ship, 10);
					}
					else if (r < 1.50)
					{
						aishipspeedadjust(ai->ship, 30);
					}
					else if (r < 2.50)
					{
						aishipspeedadjust(ai->ship, 60);
					}
					else
					{
						aishipspeedadjust(ship, ship->get_maxspeed());
					}
				}
			}
		}
		else
		{
			act_to_all_in_ship(ai->ship, "Target room not found!\r\n");
		}
		break;
	case AI_STOP:
		break;
	case AI_PATH:
		break;
	default:
		break;
	}
}
