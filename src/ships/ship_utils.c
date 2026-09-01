/*****************************************************
 * ship_utils.c
 *
 * Ship system shared helpers
 *****************************************************/

/*
 * OVERVIEW -- where this file sits in the ship system
 * ---------------------------------------------------
 * The ship subsystem's toolbox.  Nothing here drives gameplay on its own;
 * every other ship_*.c file leans on this one.  If you are new to the ship
 * code, read this file second, after ships.h.
 *
 * Map of the ship subsystem
 * -------------------------
 *   ships.h          all shared types, constants and SHIP_*() accessor macros
 *   ship_variables.c the static data tables (hull classes, weapons, crews,
 *                    chiefs, ports) -- pure data, no logic
 *   ship_utils.c     THIS FILE: the ship registry, the tactical map and
 *                    contact list, geometry, crew mechanics, slot mechanics,
 *                    messaging, and the cargo jettison/salvage helpers
 *   ship_base.c      ship lifecycle: create, name, lay out rooms, load, save,
 *                    delete, dock, sink, fly; plus the per-tick ship_activity()
 *   ship_control.c   the helm: the control panel and its player commands
 *   ship_combat.c    firing, ramming, damage, sinking, frags
 *   ship_cargo.c     the commodity market, contraband and customs
 *   ship_shop.c      the shipwright and crew-hall shopkeepers
 *   ship_npc.c       spawning and running NPC ships and their crew mobs
 *   ship_npc_ai.c    the NPC ship brain (struct NPCShipAI)
 *   ship_auto.c      the player autopilot ("order sail")
 *   ship_identity.c  process-local ship handles that survive ship deletion
 *
 * The three global scratch buffers
 * --------------------------------
 * Three module-level arrays are shared by the whole subsystem and are only
 * valid until the next call that rebuilds them.  Never hold an index or a
 * pointer into one across a call that might refill it:
 *
 *   tactical_map[101][101]  the 101x101 patch of ocean around one ship.
 *                           Rebuilt by getmap().
 *   contacts[MAXSHIPS]      the ships visible from one ship, in map order.
 *                           Rebuilt by getcontacts().  This is the array that
 *                           every "target <n>" command indexes into.
 *   local_buf               the act_to_*() formatting buffer.
 *
 * Coordinate conventions
 * ----------------------
 * ShipData::x, ::y are the ship's own indices into tactical_map, normally
 * (50, 50) -- the centre.  The map's y axis is inverted with respect to
 * compass north, so cell lookups read tactical_map[x][100 - y].  Bearings and
 * headings are compass degrees (0 = north, 90 = east), kept in [0, 360) by
 * normalize_direction().  ShipData::z is altitude and is non-zero only for
 * flying ships.
 *
 * Two indexing conventions that trip people up
 * --------------------------------------------
 *   - ship_type_data[] is subscripted by the 0-based SHIP_CLASS()/m_class,
 *     but each entry's own _classid field is 1-BASED.  See get_hull_mod().
 *   - ShipRuntimeRef::slot (ship_identity.c) is likewise 1-based, so that 0
 *     can mean "no identity".
 *
 * Ownership of returned strings
 * -----------------------------
 * Several helpers here return a pointer to a static or member buffer that the
 * next call overwrites -- get_status_str(), get_description(), crew_bonuses().
 * Each says so in its own comment.  Print them; do not store them.
 */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "net/gmcp.h" // for GMCP_ENABLED macro
#include "world/graph.h"
#include "world/map.h"
#include "item/objmisc.h"
#include "ships/ships.h"
#include "magic/spells.h"

extern int top_of_world;

char buf[MAX_STRING_LENGTH];

float hull_mod[MAXSHIPCLASS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
/*
 * Hull toughness multiplier for this ship class, sqrt(hull weight).
 *
 * Memoised in the file-static hull_mod[] table because sqrt() is called from
 * damage resolution.  Note the subscript: ShipTypeData::_classid is 1-BASED
 * (1..MAXSHIPCLASS) while SHIP_CLASS()/m_class -- the value used to subscript
 * ship_type_data[] itself -- is 0-based, so `_classid - 1` is exactly the
 * table index of this entry.  See ship_variables.c for the table.
 */
float ShipTypeData::get_hull_mod() const
{
	if (hull_mod[_classid - 1])
		return hull_mod[_classid - 1];
	hull_mod[_classid - 1] = sqrt(_hull);
	return hull_mod[_classid - 1];
}

static float ship_range(P_ship ship, P_ship target, int x, int y);

ShipObjHash shipObjHash;

/*
 * Build an empty ship hash.  A single global instance, shipObjHash, is the
 * authoritative list of every ship that exists in the running game.
 */
ShipObjHash::ShipObjHash()
{
	for (int i = 0; i < SHIP_OBJ_TABLE_SIZE; i++)
		table[i] = 0;
	sz = 0;
}
/*
 * Look up the ship whose hull object is `key`.
 *
 * `key` is the ITEM_SHIP object that represents the ship out on the ocean
 * (ShipData::shipobj) -- this is how code that walks room contents gets from
 * "an object in this room" to "the ship it belongs to".
 *
 * Returns NULL when no ship owns that object.
 */
P_ship ShipObjHash::find(P_obj key)
{
	unsigned hash_value = (unsigned)(unsigned long)key;
	unsigned t_index = hash_value % SHIP_OBJ_TABLE_SIZE;
	P_ship curr = table[t_index];
	while (curr != 0)
	{
		if (curr->shipobj == key)
			break;
		curr = curr->next;
	}
	return curr;
}
/*
 * Insert `ship` into the hash, keyed on its current shipobj.
 *
 * Returns false if the ship is already present in that bucket, true once it
 * is linked in.  IMPORTANT: the hash chains through ShipData::next, so a ship
 * may only be in one hash at a time, and its shipobj must not change while it
 * is a member -- erase() rehashes from shipobj and would look in the wrong
 * bucket.  Callers that swap a ship's hull object must erase, swap, re-add.
 */
bool ShipObjHash::add(P_ship ship)
{
	unsigned hash_value = (unsigned)(unsigned long)ship->shipobj;
	unsigned t_index = hash_value % SHIP_OBJ_TABLE_SIZE;
	P_ship curr = table[t_index];
	while (curr != 0)
	{
		if (curr == ship)
			return false;
		curr = curr->next;
	}
	ship->next = table[t_index];
	table[t_index] = ship;
	sz++;
	return true;
}
/*
 * Remove `ship` from the hash, locating its bucket from its shipobj.
 *
 * Returns false when the ship was not found (which, given the shipobj caveat
 * on add(), can also mean "its shipobj changed since insertion").
 */
bool ShipObjHash::erase(P_ship ship)
{
	unsigned hash_value = (unsigned)(unsigned long)ship->shipobj;
	return erase(ship, hash_value % SHIP_OBJ_TABLE_SIZE);
}
/*
 * Remove `ship` from bucket `t_index` specifically.
 *
 * The overload used by the iterator, which already knows the bucket and must
 * not recompute it from shipobj.  Unlinks the node, clears ShipData::next and
 * decrements the size.  Returns false if the ship was not in that bucket.
 */
bool ShipObjHash::erase(P_ship ship, unsigned t_index)
{
	P_ship curr = table[t_index];
	P_ship prev = 0;
	while (curr != 0)
	{
		if (curr == ship)
		{
			if (prev != 0)
				prev->next = curr->next;
			else
				table[t_index] = curr->next;
			curr->next = 0;
			sz--;
			return true;
		}
		prev = curr;
		curr = curr->next;
	}
	return false;
}
/*
 * Seat `vs` on the first ship in the hash.
 *
 * Iteration idiom used everywhere in the ship code:
 *
 *     ShipVisitor svs;
 *     for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
 *             ... svs->field ...
 *
 * Returns false when the hash is empty, in which case `vs` is not usable.
 */
bool ShipObjHash::get_first(visitor &vs)
{
	for (vs.t_index = 0; vs.t_index < SHIP_OBJ_TABLE_SIZE; ++vs.t_index)
	{
		if (table[vs.t_index] != 0)
		{
			vs.curr = table[vs.t_index];
			return true;
		}
	}
	return false;
}
/*
 * Advance `vs` to the next ship, walking the current chain and then on to
 * later buckets.  Returns false once the whole table has been visited.
 *
 * Do NOT erase the visited ship with the plain erase() overloads during a
 * walk -- that clears ShipData::next and strands the iterator.  Use
 * erase(visitor &) instead, which advances first.
 */
bool ShipObjHash::get_next(visitor &vs)
{
	if (vs.curr->next != 0)
	{
		vs.curr = vs.curr->next;
		return true;
	}
	for (++vs.t_index; vs.t_index < SHIP_OBJ_TABLE_SIZE; ++vs.t_index)
	{
		if (table[vs.t_index] != 0)
		{
			vs.curr = table[vs.t_index];
			return true;
		}
	}
	return false;
}
/*
 * Erase the ship `vs` is currently seated on and advance `vs` past it.
 *
 * The safe way to delete while iterating: the successor is captured before
 * the node is unlinked.  Returns what get_next() returned, i.e. false once
 * the walk is finished.
 */
bool ShipObjHash::erase(visitor &vs)
{
	unsigned t_index = vs.t_index;
	P_ship curr = vs.curr;
	bool res = get_next(vs);
	erase(curr, t_index);
	return res;
}

//--------------------------------------------------------------------
/*
 * Find the ship owned by the player named `ownername`.
 *
 * Matching is by isname(), so an abbreviation or one word of a multi-word
 * owner string will match.  Returns NULL when no ship has that owner.
 */
P_ship get_ship_from_owner(char *ownername)
{
	ShipVisitor svs;
	for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
	{
		if (isname(ownername, svs->ownername))
			return svs;
	}
	return NULL;
}

//--------------------------------------------------------------------

/*
 * Show `ch` the world outside the ship -- the ocean map around ship->location.
 *
 * Temporarily raises ch's z coordinate so tall hulls (cruisers, dreadnoughts)
 * and flying ships see further, then restores it.  GMCP-capable clients are
 * additionally sent a Room.Map package before the textual look.
 *
 * `ch` need not be on the bridge, or even aboard; the caller decides who is
 * entitled to a view.
 */
void look_out_ship(P_ship ship, P_char ch)
{
	int saved_z_cord = ch->specials.z_cord;

	if (ship->m_class == SH_CRUISER || ship->m_class == SH_DREADNOUGHT)
		ch->specials.z_cord = 2;
	if (SHIP_FLYING(ship))
		ch->specials.z_cord = 4;

	// For WebSocket/GMCP clients, send ocean map via GMCP
	// display_map_room will skip text output but send Room.Map GMCP
	if (ch->desc && GMCP_ENABLED(ch) && IS_MAP_ROOM(ship->location))
	{
		int n = map_view_distance(ch, ship->location);
		if (n > 1)
		{
			display_map_room(ch, ship->location, n, MAP_IGNORE_TOGGLE, 0);
		}
	}

	new_look(ch, 0, CMD_LOOKOUT, ship->location);
	ch->specials.z_cord = saved_z_cord;
}

/*
 * Refresh the PvP delay timer on everyone aboard `ship`.
 *
 * Called when the ship does something that should make its passengers
 * legitimate targets for a while.  Any existing PVPDELAY is removed first so
 * the full WAIT_PVPDELAY is reapplied rather than left to expire early.
 */
void set_pvp_on_passengers(P_ship ship)
{
	P_char ch, ch_next;
	int i;

	for (i = 0; i < ship->room_count; i++)
	{
		for (ch = world[real_room(ship->room[i].roomnum)].people; ch; ch = ch_next)
		{
			if (ch)
			{
				ch_next = ch->next_in_room;
				affect_from_char(ch, TAG_PVPDELAY);
				set_short_affected_by(ch, TAG_PVPDELAY, WAIT_PVPDELAY);
			}
		}
	}
}

/*
 * Push a fresh outside view to every passenger who has the ship-map toggle
 * (PLR2_SHIPMAP) enabled.  Called after the ship moves so watchers see the
 * world scroll past.  Passengers without the toggle are left alone.
 */
void everyone_look_out_ship(P_ship ship)
{
	P_char ch, ch_next;
	int i;

	for (i = 0; i < ship->room_count; i++)
	{
		for (ch = world[real_room(ship->room[i].roomnum)].people; ch; ch = ch_next)
		{
			if (ch)
			{
				ch_next = ch->next_in_room;
				if (IS_SET(ch->specials.act2, PLR2_SHIPMAP))
				{
					look_out_ship(ship, ch);
				}
			}
		}
	}
}

/*
 * Move every character and (almost) every object out of the ship's rooms and
 * into the ship's own location -- the ocean room the hull sits in.
 *
 * Used when the ship stops being a habitable place but still exists.  The
 * control panel stays put, and player corpses are left where they are;
 * everything else is dumped overboard along with the crew.
 */
void kick_everyone_off(P_ship ship)
{
	P_char ch, ch_next;
	P_obj obj, obj_next;
	int i;

	for (i = 0; i < ship->room_count; i++)
	{
		for (ch = world[real_room(ship->room[i].roomnum)].people; ch; ch = ch_next)
		{
			if (ch)
			{
				ch_next = ch->next_in_room;
				char_from_room(ch);
				char_to_room(ch, ship->location, -1);
			}
		}
		for (obj = world[real_room(ship->room[i].roomnum)].contents; obj; obj = obj_next)
		{
			if (obj)
			{
				obj_next = obj->next_content;
				if (obj != ship->panel && !(obj->type == ITEM_CORPSE &&
							    !IS_SET(obj->value[1], NPC_CORPSE)))
				{
					obj_from_room(obj);
					obj_to_room(obj, ship->location);
				}
			}
		}
	}
}

/*
 * Empty the ship's rooms permanently, as part of destroying it.
 *
 * Harsher than kick_everyone_off(): PCs are moved out to ship->location, but
 * NPCs are extracted outright and all objects are destroyed.  The two
 * exceptions are the control panel, which is left in place for the caller to
 * dispose of, and player corpses, which are moved out rather than deleted so
 * nobody loses their equipment with the hull.
 */
void clear_ship_content(P_ship ship)
{
	P_char ch, ch_next;
	P_obj obj, obj_next;
	int i;

	for (i = 0; i < ship->room_count; i++)
	{
		for (ch = world[real_room(ship->room[i].roomnum)].people; ch; ch = ch_next)
		{
			if (ch)
			{
				ch_next = ch->next_in_room;
				if (IS_PC(ch))
				{
					char_from_room(ch);
					char_to_room(ch, ship->location, -1);
				}
				else
				{
					extract_char(ch);
				}
			}
		}
		for (obj = world[real_room(ship->room[i].roomnum)].contents; obj; obj = obj_next)
		{
			if (obj)
			{
				obj_next = obj->next_content;
				if (obj == ship->panel)
					continue;
				if (obj->type == ITEM_CORPSE && !IS_SET(obj->value[1], NPC_CORPSE))
				{
					obj_from_room(obj);
					obj_to_room(obj, ship->location);
					continue;
				}
				extract_obj(obj, TRUE);
			}
		}
	}
}

static char local_buf[MAX_STRING_LENGTH];
/*
 * printf-style act_to_all_in_ship(): format `msg` and send it to everyone
 * aboard `ship`.
 *
 * Formats into a shared module-level buffer, so the result must be consumed
 * before the next call -- do not hold on to anything derived from it.  A NULL
 * ship is ignored.
 */
void act_to_all_in_ship_f(P_ship ship, const char *msg, ...)
{
	if (ship == NULL)
		return;

	va_list args;
	va_start(args, msg);
	vsnprintf(local_buf, sizeof(local_buf) - 1, msg, args);
	va_end(args);

	act_to_all_in_ship(ship, local_buf);
}

/*
 * Send `msg`, already formatted, to every character in every room of `ship`.
 *
 * Each room is messaged through act() twice (TO_ROOM and TO_CHAR) off its
 * first occupant, which is how the message reaches that room's whole
 * population.  A NULL ship is ignored.
 */
void act_to_all_in_ship(P_ship ship, const char *msg)
{
	if (ship == NULL)
		return;

	for (int i = 0; i < ship->room_count; i++)
	{
		if ((SHIP_ROOM_NUM(ship, i) != -1) &&
		    world[real_room(SHIP_ROOM_NUM(ship, i))].people)
		{
			act(msg, FALSE, world[real_room(SHIP_ROOM_NUM(ship, i))].people, 0, 0,
			    TO_ROOM);
			act(msg, FALSE, world[real_room(SHIP_ROOM_NUM(ship, i))].people, 0, 0,
			    TO_CHAR);
		}
	}
}

/*
 * As act_to_all_in_ship(), but with `victim` bound as act()'s target so the
 * message may use the $N/$M/$S victim escapes.
 */
void act_to_all_in_ship(P_ship ship, const char *msg, P_char victim)
{
	if (ship == NULL)
		return;

	for (int i = 0; i < ship->room_count; i++)
	{
		if ((SHIP_ROOM_NUM(ship, i) != -1) &&
		    world[real_room(SHIP_ROOM_NUM(ship, i))].people)
		{
			act(msg, FALSE, world[real_room(SHIP_ROOM_NUM(ship, i))].people, 0, victim,
			    TO_ROOM);
			act(msg, FALSE, world[real_room(SHIP_ROOM_NUM(ship, i))].people, 0, victim,
			    TO_CHAR);
		}
	}
}

/*
 * Broadcast a formatted message to every ocean room within `rng` map rooms of
 * `ship` -- what people in the surrounding water see and hear.
 *
 * Builds the tactical map first and returns silently if the ship is not on
 * the ocean map.  Shares the same module-level format buffer as
 * act_to_all_in_ship_f().
 */
void act_to_outside(P_ship ship, int rng, const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	vsnprintf(local_buf, sizeof(local_buf) - 1, msg, args);
	va_end(args);

	if (!getmap(ship))
		return;
	for (int i = 0; i < 100; i++)
	{
		for (int j = 0; j < 100; j++)
		{
			if ((range(50, 50, ship->z, j, i, 0) < rng) &&
			    world[tactical_map[j][i].rroom].people)
			{
				act(local_buf, FALSE, world[tactical_map[j][i].rroom].people, 0, 0,
				    TO_ROOM);
				act(local_buf, FALSE, world[tactical_map[j][i].rroom].people, 0, 0,
				    TO_CHAR);
			}
		}
	}
}

/*
 * Broadcast a formatted message to the crews of other ships within `rng` map
 * rooms of `ship`.
 *
 * Unlike act_to_outside(), which speaks to whoever is swimming nearby, this
 * reaches *inside* neighbouring hulls.  `ship` itself never receives the
 * message; pass `target` to exclude a second ship as well (typically the one
 * the message is about, which is being told something different).  Pass NULL
 * for `target` to exclude nobody else.
 *
 * Shares the module-level format buffer with act_to_all_in_ship_f().
 */
void act_to_outside_ships(P_ship ship, P_ship target, int rng, const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	vsnprintf(local_buf, sizeof(local_buf) - 1, msg, args);
	va_end(args);

	if (!getmap(ship))
		return;
	for (int i = 0; i < 100; i++)
	{
		for (int j = 0; j < 100; j++)
		{
			if ((range(50, 50, ship->z, j, i, 0) < rng) &&
			    world[tactical_map[j][i].rroom].contents)
			{
				for (P_obj obj = world[tactical_map[j][i].rroom].contents; obj;
				     obj = obj->next_content)
				{
					if ((GET_ITEM_TYPE(obj) == ITEM_SHIP) &&
					    (obj->value[6] == 1) && (obj != ship->shipobj))
					{
						if (target != NULL && obj == target->shipobj)
							continue;

						act_to_all_in_ship(shipObjHash.find(obj),
								   local_buf);
					}
				}
			}
		}
	}
}

/*
 * Return the ship `ch` is standing on, or NULL if they are not aboard one.
 *
 * Works by matching ch's room vnum against every ship's room list, so it is
 * O(ships x rooms) -- fine at call rates, but do not put it in a tight loop.
 * Ships that are not loaded are skipped.
 *
 * Deliberately tolerant: a dead character is still "on" the ship, since
 * corpses stay in the room.  A NULL ch, or one whose in_room is out of range,
 * yields NULL.
 */
P_ship get_ship_from_char(P_char ch)
{
	int j, roomVnum;
	ShipVisitor svs;
	bool fn;

	// Dead chars can still be on a ship (but we need them in a room to be on the ship).
	if (ch == NULL || ch->in_room < 0 || ch->in_room > top_of_world)
	{
		return NULL;
	}

	// Just to speed up derefrencing.
	roomVnum = world[ch->in_room].number;

	// For each ship that exists.
	for (fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
	{
		// Skip ships that are not loaded (an empty ship memory location).
		if (!SHIP_LOADED(svs))
			continue;

		// For each room on the ship.
		for (j = 0; j < svs->room_count; j++)
		{
			// If the char is in that room, return that ship..
			if (roomVnum == svs->room[j].roomnum)
			{
				return svs;
			}
		}
	}

	// Couldn't find a ship that the ch is on, so return NULL.
	return NULL;
}

/*
 * Count the ship's "real" occupants, for capacity checks.
 *
 * Deliberately does not count immortals, ship-crew pirate mobs (vnums
 * 40201..40299) or images (vnum 250) -- crew you were given do not eat into
 * the berths you paid for.  Returns 0 for a ship that is not loaded.
 */
int num_people_in_ship(P_ship ship)
{
	int i, num = 0;
	P_char ch;

	if (!SHIP_LOADED(ship))
		return 0;

	for (i = 0; i < ship->room_count; i++)
	{
		for (ch = world[real_room0(ship->room[i].roomnum)].people; ch;
		     ch = ch->next_in_room)
		{
			if (IS_TRUSTED(ch))
				continue;
			if (IS_NPC(ch) &&
			    ((GET_VNUM(ch) > 40200 && GET_VNUM(ch) < 40300) || // pirates
			     (GET_VNUM(ch) == 250))) // images
			{
				continue;
			}
			num++;
		}
	}
	return (num);
}

/*
 * Degrees of heading this ship can turn in one tick.
 *
 * Built from the class's base turn rate (SHIP_HDDC) and then scaled by three
 * things: how fast the ship is moving relative to boarding speed (a ship
 * crawling along turns at three-quarters rate or worse), the crew's sailing
 * skill, and the crew's stamina.  Returns 1 for an immobile ship so it can
 * still be pointed somewhere.
 */
float get_turning_speed(P_ship ship)
{
	if (SHIP_IMMOBILE(ship))
		return 1;

	float tspeed = (float)SHIP_HDDC(ship);
	tspeed *=
		0.75 +
		0.25 * (float)(ship->speed - BOARDING_SPEED) /
			(float)(SHIPTYPE_SPEED(SHIP_CLASS(ship)) -
				BOARDING_SPEED); // only 3/4 turn at boarding speed, even less if slower
	tspeed *= (1.0 + ship->crew.sail_mod_applied);
	tspeed *= ship->crew.get_stamina_mod();
	return tspeed;
}

/*
 * Signed degrees to add to ship->heading this tick to close on setheading.
 *
 * Picks the short way round the compass (the difference is normalised into
 * -180..180) and clamps the step to get_turning_speed().  Returns 0 when the
 * ship is already on its ordered heading.
 */
float get_next_heading_change(P_ship ship)
{
	if (ship->heading == ship->setheading)
		return 0;

	float hdspeed = get_turning_speed(ship);

	float diff = ship->setheading - ship->heading;
	if (diff > 180)
		diff -= 360;
	if (diff < -180)
		diff += 360;

	float change = 0;
	if (diff >= 0)
		change = MIN(diff, hdspeed);
	else
		change = MAX(diff, -hdspeed);

	return change;
}

/*
 * Speed units this ship can gain or shed in one tick.
 *
 * The class's base acceleration scaled by crew sailing skill and stamina --
 * the same two crew factors that scale turning.
 */
int get_acceleration(P_ship ship)
{
	float accel = SHIP_ACCEL(ship);
	accel *= (1.0 + ship->crew.sail_mod_applied);
	accel *= ship->crew.get_stamina_mod();
	return (int)accel;
}
/*
 * Signed speed delta to apply this tick to close on setspeed.
 *
 * One acceleration step towards the ordered speed, or exactly the remaining
 * difference when that would overshoot.  Returns 0 when the ship is already
 * at its ordered speed.  Deceleration uses the same acceleration figure.
 */
int get_next_speed_change(P_ship ship)
{
	int accel = get_acceleration(ship);
	if (ship->setspeed > ship->speed)
	{
		if (ship->speed + accel <= ship->setspeed)
			return accel;
		else
			return ship->setspeed - ship->speed;
	}
	if (ship->setspeed < ship->speed)
	{
		if (ship->speed - accel >= ship->setspeed)
			return -accel;
		else
			return ship->setspeed - ship->speed;
	}
	return 0;
}

/*
 * Recompute and store ship->maxspeed from the ship's current condition.
 *
 * `breach_count` is how many hull arcs have been holed.  A single breach
 * stops a surface ship dead (maxspeed 0), as does losing the mainsail
 * entirely; a flying ship survives one breach at half speed.
 *
 * Otherwise the class ceiling is scaled by, in order: crew sailing skill,
 * carried weight beyond the class's free allowances for equipment and cargo,
 * and the remaining condition of the sail.  The result is clamped to at least
 * 1 and at most the ceiling.
 *
 * Call this after anything that changes weight, sail condition, breaches or
 * crew -- see update_ship_status() (ship_combat.c), which is the usual entry
 * point and calls this for you.
 *
 * (The flying bonus is deliberately applied to both the ceiling and the
 * running total; that is long-standing behaviour and is left as-is.)
 */
void update_maxspeed(P_ship ship, int breach_count)
{
	if ((breach_count >= 1 && !SHIP_FLYING(ship)) || ship->mainsail == 0)
	{
		ship->maxspeed = 0;
		return;
	}

	int equipment_weight = ship->slot_weight(SLOT_WEAPON) + ship->slot_weight(SLOT_EQUIPMENT);
	int equipment_weight_mod = MIN(SHIP_FREE_EQUIPMENT(ship), equipment_weight);
	int cargo_weight = ship->slot_weight(SLOT_CARGO) + ship->slot_weight(SLOT_CONTRABAND);
	int cargo_weight_mod = MIN(SHIP_FREE_CARGO(ship), cargo_weight);

	float weight_mod =
		1.0 - ((float)(SHIP_SLOT_WEIGHT(ship) - equipment_weight_mod - cargo_weight_mod) /
		       (float)SHIP_MAX_WEIGHT(ship));

	int ceil = SHIPTYPE_SPEED(ship->m_class) + ship->crew.get_maxspeed_mod();
	if (breach_count == 0 && SHIP_FLYING(ship))
		ceil *= 1.2;

	float maxspeed = ceil;
	if (breach_count == 0 && SHIP_FLYING(ship))
		maxspeed *= 1.2;
	if (breach_count == 1 && SHIP_FLYING(ship))
		maxspeed *= 0.5;
	maxspeed = maxspeed * (1.0 + ship->crew.sail_mod_applied);
	maxspeed = maxspeed * weight_mod;
	maxspeed = maxspeed * (float)ship->mainsail /
		   (float)SHIP_MAX_SAIL(ship); // Adjust for sail condition
	ship->maxspeed = BOUNDED(1, (int)maxspeed, ceil);
}

/*
 * What ship->maxspeed would be if the hold were empty.
 *
 * Same computation as update_maxspeed() minus the cargo weight term, so
 * shop and status screens can show players what their cargo is costing them.
 * Does not modify the ship.  Returns 0 for an already-immobile ship.
 */
int get_maxspeed_without_cargo(P_ship ship)
{
	if (ship->get_maxspeed() == 0)
		return 0;

	int equipment_weight = ship->slot_weight(SLOT_WEAPON) + ship->slot_weight(SLOT_EQUIPMENT);
	int equipment_weight_mod = MIN(SHIP_FREE_EQUIPMENT(ship), equipment_weight);

	float weight_mod = 1.0 - ((float)(equipment_weight - equipment_weight_mod) /
				  (float)SHIP_MAX_WEIGHT(ship));

	int ceil = SHIPTYPE_SPEED(ship->m_class) + ship->crew.get_maxspeed_mod();
	float maxspeed = ceil;
	maxspeed = maxspeed * (1.0 + ship->crew.sail_mod_applied);
	maxspeed = maxspeed * weight_mod;
	maxspeed = maxspeed * (float)ship->mainsail /
		   (float)SHIP_MAX_SAIL(ship); // Adjust for sail condition
	return BOUNDED(1, (int)maxspeed, ceil);
}

/* get max speed, includes player racial bonus if ch provided */
/*
 * The ship's effective maximum speed, including flat bonuses.
 *
 * `ch` is optional: pass the character asking (usually whoever is at the
 * helm) and a sailor with the SEADOG innate adds 2.  Pass NULL for the
 * ship's own unmodified figure.
 */
int ShipData::get_maxspeed(P_char ch) const
{
	int speed = maxspeed + maxspeed_bonus;
	if (ch && has_innate(ch, INNATE_SEADOG))
		speed += 2;
	return speed;
}

/*
 * Give `ship` its two-letter contact designation -- the "[AB]" tag other
 * ships see on the tactical display and in combat messages.
 *
 * Pass id == NULL to mint a fresh unused designation.  Player ships draw
 * their first letter from A..W; NPC ships (npc == true) draw from X..Z, so a
 * designation's first letter tells you which kind of ship you are looking at.
 * Uniqueness is enforced by scanning every live ship; after 1000 failed
 * attempts the routine gives up, wizlogs, and leaves the id unchanged.
 *
 * Pass id == "**" to set the placeholder designation used by ships that are
 * docked and not yet on the map.  Any other explicit `id` is currently
 * ignored -- the routine has no branch for it.
 *
 * The designation is str_dup()ed onto SHIP_ID(); callers that reassign an id
 * are responsible for the previous string.
 */
void assignid(P_ship ship, const char *id, bool npc)
{
	if (!id)
	{
		// assigning new contact id

		bool found_id = false;
		char newid[] = "--";

		int c = 0; // to make sure its not an infinite loop

		while (!found_id)
		{
			if (npc)
				newid[0] = 'X' + number(0, 2);
			else
				newid[0] = 'A' + number(0, 22);
			newid[1] = 'A' + number(0, 25);

			bool taken = false;
			ShipVisitor svs;
			for (bool fn = shipObjHash.get_first(svs); fn;
			     fn = shipObjHash.get_next(svs))
			{
				if (strcmp(newid, SHIP_ID(svs)) == 0)
				{
					taken = true;
					break;
				}
			}

			if (!taken)
				found_id = true;

			if (c++ > 1000)
			{
				wizlog(56,
				       "error in ships code assignid(): cannot find new contact id");
				return;
			}
		}

		SHIP_ID(ship) = str_dup(newid);
	}
	else
	{
		// assigning specific id
		if (isname(id, "**"))
		{
			SHIP_ID(ship) = str_dup("**");
			return;
		}
	}
}

/*
 * Fill the global tactical_map[101][101] with the ocean around `ship`.
 *
 * This is the ship system's scratch view of the world and MUST be called
 * before anything reads tactical_map -- getcontacts(), the autopilot, the
 * NPC AI and the on-screen tactical display all depend on it, and they all
 * share the one buffer, so the map is only valid until the next call.
 *
 * The ship sits at [50][50], marked "**".  Each cell gets the terrain symbol
 * for its sector, or the "&+W<id>&N" designation of a ship sitting there.
 * Note the inverted y axis: cell [x][y] is y - 50 rooms NORTH of the ship,
 * so lookups elsewhere read tactical_map[x][100 - y].
 *
 * `limit_range` true hides ships beyond the crew's contact range (35 rooms
 * plus the crew's scout bonus); false shows every ship on the patch.
 *
 * Returns FALSE without touching the map when the ship is not on a map room.
 */
bool getmap(P_ship ship, bool limit_range)
{
	int x, y, rroom;
	P_obj obj;
	float rng = 35 + ship->crew.get_contact_range_mod();

	if (!IS_MAP_ROOM(ship->location))
		return FALSE;

	for (y = 0; y < 100; y++)
	{
		for (x = 0; x < 100; x++)
		{
			strcpy(tactical_map[x][y].map, "  ");
		}
	}
	for (y = 99; y >= 0; y--)
	{
		for (x = 0; x < 100; x++)
		{
			rroom = calculate_relative_room(ship->location, x - 50, y - 50);
			tactical_map[x][y].rroom = rroom;
			if ((world[rroom].sector_type < NUM_SECT_TYPES) &&
			    (world[rroom].sector_type > -1))
			{
				sprintf(tactical_map[x][y].map, "%s",
					ship_symbol[(int)world[rroom].sector_type]);
			}
			else
			{
				sprintf(tactical_map[x][y].map, "%s", ship_symbol[0]);
			}
			if (world[rroom].contents)
			{
				for (obj = world[rroom].contents; obj; obj = obj->next_content)
				{
					if ((GET_ITEM_TYPE(obj) == ITEM_SHIP) &&
					    (obj->value[6] == 1))
					{
						P_ship temp = shipObjHash.find(obj);
						if (temp == NULL)
							continue;
						if (!limit_range ||
						    (ship_range(ship, temp, x, 100 - y) <= rng))
							sprintf(tactical_map[x][y].map, "&+W%s&N",
								temp->id);
					}
				}
			}
		}
	}
	sprintf(tactical_map[50][50].map, "&+W**&N");

	return TRUE;
}

/*
 * Which of the ship's four arcs a contact at `bearing` falls into, given the
 * ship is pointing at `heading`.  Both are compass degrees.
 *
 * Returns SIDE_FORE, SIDE_PORT, SIDE_REAR or SIDE_STAR.  The forward and
 * rear arcs are 80 degrees wide and the beam arcs 100, so the sides of a
 * ship are the easier targets -- which is also why they carry more armour
 * (see ship_arc_properties[] in ship_variables.c).
 */
int get_arc(float heading, float bearing)
{
	bearing -= heading;
	normalize_direction(bearing);
	if (bearing <= 40 || bearing >= 320)
		return SIDE_FORE;
	if (bearing >= 140 && bearing <= 220)
		return SIDE_REAR;
	if (bearing >= 40 && bearing <= 140)
		return SIDE_STAR;
	if (bearing >= 220 && bearing <= 320)
		return SIDE_PORT;
	return SIDE_FORE;
}

/*
 * One-letter arc tag for tactical displays: "F", "P", "R", "S", or "*" for
 * an unrecognised arc.  Returns a static literal; never free it.
 */
const char *get_arc_indicator(int arc_no)
{
	switch (arc_no)
	{
	case SIDE_FORE:
		return "F";
	case SIDE_REAR:
		return "R";
	case SIDE_STAR:
		return "S";
	case SIDE_PORT:
		return "P";
	}
	return "*";
}

/*
 * Populate contacts[i] with `target` as seen from `ship`.
 *
 * `x`, `y` are the target's cell in the tactical map (the ship itself being
 * at 50,50).  Fills in bearing, range, position, and a two-letter arc tag:
 * first letter is which of *our* arcs the target bears in, second is which of
 * *its* arcs we bear in -- so "SF" means the target is off our starboard and
 * we are dead ahead of it.
 *
 * Writes into the shared global contacts[] array; the caller owns the index
 * and is responsible for keeping it below MAXSHIPS.
 */
void setcontact(int i, P_ship target, P_ship ship, int x, int y)
{
	contacts[i].bearing = bearing(ship->x, ship->y, (float)x + (target->x - 50.0),
				      (float)y + (target->y - 50.0));

	contacts[i].range = ship_range(ship, target, x, y);

	contacts[i].x = x;
	contacts[i].y = y;

	contacts[i].z = (int)target->z;
	contacts[i].ship = target;

	sprintf(contacts[i].arc, "%s%s",
		get_arc_indicator(get_arc(ship->heading, contacts[i].bearing)),
		get_arc_indicator(get_arc(target->heading, (contacts[i].bearing >= 180) ?
								   (contacts[i].bearing - 180) :
								   (contacts[i].bearing + 180))));
}

/*
 * Fill the global contacts[] array with every other ship `ship` can see, and
 * return how many were found.
 *
 * The result is the ship's sensor picture and is what every "pick target N"
 * command indexes into.  Like tactical_map, contacts[] is a single shared
 * buffer: it is valid only until the next getcontacts() call, so never hold
 * an index across one.
 *
 * `limit_range` true -- the default -- keeps only ships within the crew's
 * contact range (35 rooms plus the crew's scout bonus).  Pass false to
 * enumerate every ship sharing the map patch regardless of distance.
 */
int getcontacts(P_ship ship, bool limit_range)
{
	int counter = 0;
	float max_range = 35 + ship->crew.get_contact_range_mod();

	ShipVisitor svs;
	for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
	{
		/* contacts[] is fixed at MAXSHIPS entries; never write past it. */
		if (counter >= MAXSHIPS)
			break;

		P_ship vict = svs;
		if (vict == ship)
			continue;

		int x, y;
		if (!calculate_map_coords(ship->location, vict->location, x, y))
			continue;

		// somehow (0,0) is represented by (50,50)
		x = 50 + x;
		y = 50 - y;
		if (limit_range && ship_range(ship, vict, x, y) > max_range)
			continue;

		setcontact(counter++, vict, ship, x, y);
	}

	return counter;
}

/*
 * Compass bearing in degrees from (x1,y1) to (x2,y2), 0 = north, 90 = east.
 *
 * Map coordinates, so +y is north and +x is east.  The four exactly-axial
 * cases are special-cased to avoid dividing by zero.
 */
float bearing(float x1, float y1, float x2, float y2)
{
	float val;

	if (y1 == y2)
	{
		if (x1 > x2)
			return 270;
		return 90;
	}
	if (x1 == x2)
	{
		if (y1 > y2)
			return 180;
		else
			return 0;
	}
	val = atan((x2 - x1) / (y2 - y1)) * 180 / M_PI;
	if (y1 < y2)
	{
		if (val >= 0)
			return val;
		return (val + 360);
	}
	else
	{
		return val + 180;
	}
	return 0;
}

/*
 * Dump the ship's full internal state to `ch` -- an immortal diagnostic,
 * not a player-facing display.
 *
 * Prints identity, heading and speed, the weight budget, the cargo budget,
 * and then every one of the MAXSLOTS slots with its raw val0..val4 fields.
 * The player-facing equivalents are the status and slot listings in
 * ship_control.c.
 */
void ShipData::show(P_char ch) const
{
	send_to_char("Ship Information\r\n-----------------------------------\r\n", ch);

	send_to_char_f(ch, "Name: %s\r\n", this->name);

	send_to_char_f(ch, "Owner: %s\r\n", this->ownername);

	send_to_char_f(ch, "ID: %s\r\n", this->id);

	send_to_char_f(ch, "Heading: %d, Set Heading: %d\r\n", (int)this->heading,
		       (int)this->setheading);

	send_to_char_f(ch, "Speed: %d, Set Speed: %d, Max Speed: %d, Sailcond: %d\r\n", this->speed,
		       this->setspeed, this->maxspeed, this->mainsail);

	send_to_char_f(ch,
		       "Hull weight: %d, Max load: %d, Slot weight: %d, Available weight: %d\r\n",
		       SHIP_HULL_WEIGHT(this), SHIP_MAX_WEIGHT(this), slot_weight(-1),
		       SHIP_AVAIL_WEIGHT(this));

	send_to_char_f(ch, "Max cargo: %d, Current cargo: %d, Available cargo: %d\r\n",
		       SHIP_MAX_CARGO(this), SHIP_CARGO(this), SHIP_AVAIL_CARGO_LOAD(this));

	send_to_char("\r\nSlots:\r\n---------------------------------------------------\r\n", ch);

	for (int i = 0; i < MAXSLOTS; i++)
	{
		send_to_char_f(ch, "%-2d) ", i);
		this->slot[i].show(ch, this);
		send_to_char("\r\n", ch);
	}
}

/*
 * Print one raw slot row for ShipData::show() -- type, mounting position,
 * weight, and the five untyped val fields.
 *
 * The val fields mean different things per slot type; see the field comments
 * on struct ShipSlot in ships.h.
 */
void ShipSlot::show(P_char ch, const ShipData *ship) const
{
	switch (this->type)
	{
	case SLOT_EMPTY:
		send_to_char("Empty       ", ch);
		break;

	case SLOT_WEAPON:
		send_to_char("Weapon      ", ch);
		break;

	case SLOT_CARGO:
		send_to_char("Cargo       ", ch);
		break;

	case SLOT_CONTRABAND:
		send_to_char("Contraband  ", ch);
		break;

	default:
		send_to_char("(unknown)   ", ch);
		break;
	}

	switch (this->position)
	{
	case SLOT_FORE:
		send_to_char("F  ", ch);
		break;

	case SLOT_PORT:
		send_to_char("P  ", ch);
		break;

	case SLOT_REAR:
		send_to_char("R  ", ch);
		break;

	case SLOT_STAR:
		send_to_char("S  ", ch);
		break;

	case SLOT_EQUI:
		send_to_char("E  ", ch);
		break;

	case SLOT_HOLD:
		send_to_char("H  ", ch);
		break;

	default:
		send_to_char("?  ", ch);
		break;
	}

	send_to_char_f(ch, "%-3d  ", this->get_weight(ship));

	send_to_char_f(ch, "%-5d %-7d %-5d %-5d %-5d  ", this->val0, this->val1, this->val2,
		       this->val3, this->val4);
}

/*
 * Total weight of the ship's occupied slots.
 *
 * Pass a SLOT_* constant to weigh only slots of that type, or a negative
 * `type` to weigh everything -- which is what the SHIP_SLOT_WEIGHT() macro
 * does.  Empty slots never contribute.
 */
int ShipData::slot_weight(int type) const
{
	int weight = 0;

	for (int i = 0; i < MAXSLOTS; i++)
	{
		if (this->slot[i].type != SLOT_EMPTY && (type < 0 || type == this->slot[i].type))
			weight += this->slot[i].get_weight(this);
	}

	return weight;
}

/*
 * Human-readable condition of this slot, for weapon and equipment listings:
 * "Destroyed", "Badly Damaged", "Damaged", "Out of Ammo", a reload countdown,
 * or "Ready".  Cargo and empty slots yield "".
 *
 * Returns a pointer to the slot's own `status` buffer, which is overwritten
 * on every call -- print it, do not keep it.
 */
char *ShipSlot::get_status_str()
{
	if (type == SLOT_WEAPON || type == SLOT_EQUIPMENT)
	{
		if (val2 > 100)
			sprintf(status, "&+LDestroyed");
		else if (val2 > 20)
			sprintf(status, "&+RBadly Damaged");
		else if (val2 > 0)
			sprintf(status, "&+WDamaged");
		else if (val1 == 0 && type == SLOT_WEAPON)
			sprintf(status, "&+LOut of Ammo");
		else if (timer > 0)
			sprintf(status, "&+Y%-6d", timer);
		else if (timer == 0)
			strcpy(status, "&NReady");
	}
	else
		strcpy(status, "");

	return status;
}

/*
 * Where this slot is mounted, spelled out: "Forward", "Rear", "Port",
 * "Starboard", "Equipment", "Cargo Hold", or "ERROR" for an unset position.
 * Returns a static literal.
 */
const char *ShipSlot::get_position_str()
{
	switch (position)
	{
	case SLOT_FORE:
		return "Forward";
	case SLOT_REAR:
		return "Rear";
	case SLOT_PORT:
		return "Port";
	case SLOT_STAR:
		return "Starboard";
	case SLOT_EQUI:
		return "Equipment";
	case SLOT_HOLD:
		return "Cargo Hold";
	default:
		return "ERROR";
	}
}

/*
 * Name of whatever occupies this slot -- the weapon, the equipment, or the
 * cargo/contraband commodity.  Empty slots yield "".
 *
 * Returns a pointer to the slot's own `desc` buffer, overwritten on every
 * call.
 */
char *ShipSlot::get_description()
{
	if (type == SLOT_WEAPON)
	{
		sprintf(desc, "%s", weapon_data[index].name);
	}
	else if (type == SLOT_EQUIPMENT)
	{
		sprintf(desc, "%s", equipment_data[index].name);
	}
	else if (type == SLOT_CARGO)
	{
		sprintf(desc, "%s", cargo_type_name(index));
	}
	else if (type == SLOT_CONTRABAND)
	{
		sprintf(desc, "%s", contra_type_name(index));
	}
	else
	{
		desc[0] = 0;
	}
	return desc;
}

/*
 * Reset the slot to empty.
 *
 * Note the -1 fill on index, position and val0..val4: that is the "unset"
 * marker, and it is why callers must check `type != SLOT_EMPTY` before using
 * `index` to subscript weapon_data[] or equipment_data[].
 */
void ShipSlot::clear()
{
	strcpy(desc, "");
	strcpy(status, "");
	type = SLOT_EMPTY;
	index = -1;
	position = -1;
	timer = 0;
	val0 = -1;
	val1 = -1;
	val2 = -1;
	val3 = -1;
	val4 = -1;
}

/*
 * How much this slot's contents weigh against the ship's weight budget.
 *
 * Weapons and equipment carry their table weight, except for two special
 * cases that scale with the hull: a ram and a levistone.  A levistone weighs
 * nothing while the ship is actually flying -- it is holding itself up.
 * Cargo and contraband weigh per crate.
 *
 * `ship` is needed for those hull-relative cases; empty slots weigh 0.
 */
int ShipSlot::get_weight(const ShipData *ship) const
{
	int wt = 0;
	if (type == SLOT_WEAPON)
	{
		wt = weapon_data[index].weight;
	}
	else if (type == SLOT_EQUIPMENT)
	{
		wt = equipment_data[index].weight;
		if (index == E_RAM)
			wt = eq_ram_weight(ship);
		if (index == E_LEVISTONE)
		{
			if (SHIP_FLYING(ship))
				wt = 0;
			else
				wt = eq_levistone_weight(ship);
		}
	}
	else if (type == SLOT_CARGO)
	{
		wt = val0 * WEIGHT_CARGO;
	}
	else if (type == SLOT_CONTRABAND)
	{
		wt = val0 * WEIGHT_CONTRABAND;
	}
	return wt;
}

/*
 * Copy `other`'s contents into this slot.
 *
 * Copies the state fields only -- not the cached `desc` and `status` display
 * buffers, which are regenerated on demand.  Used by the shop's slot-swap.
 */
void ShipSlot::clone(const ShipSlot &other)
{
	type = other.type;
	index = other.index;
	position = other.position;
	timer = other.timer;
	val0 = other.val0;
	val1 = other.val1;
	val2 = other.val2;
	val3 = other.val3;
	val4 = other.val4;
}

/*
 * Whether this crew can be hired in `room` (a room vnum).
 *
 * Each crew lists up to five hiring rooms in ship_crew_data[]; unused entries
 * are 0, which no real room vnum matches.
 */
bool ShipCrewData::hire_room(int room) const
{
	for (int i = 0; i < 5; i++)
		if (hire_rooms[i] == room)
			return true;
	return false;
}

/*
 * Iterate this crew's passive bonuses, one description per call.
 *
 * `cur` is the caller's cursor: start it at 0 and pass the same pointer back
 * each time.  Returns the next bonus's description and leaves `*cur` on that
 * bonus's bit, or returns "" and sets `*cur` to -1 once the flags are
 * exhausted.  See crew_bonuses() for the assembled one-line form.
 */
const char *ShipCrewData::get_next_bonus(int *cur) const
{
	for ((*cur)++; *cur < 32; (*cur)++)
	{
		/* 1UL, not 1: `1 << 31` overflows a signed int. */
		ulong flag = 1UL << (*cur);
		if (IS_SET(flags, flag))
		{
			return get_bonus_string(flag);
		}
	}
	*cur = -1;
	return "";
}

/*
 * Description of a single CF_* crew bonus flag, e.g. "Scout Range +2".
 * Returns "Unknown" for a flag with no description, and always a static
 * literal.
 */
const char *ShipCrewData::get_bonus_string(ulong flag) const
{
	if (flag == CF_SCOUT_RANGE_1)
		return "Scout Range +1";
	if (flag == CF_SCOUT_RANGE_2)
		return "Scout Range +2";
	if (flag == CF_MAXSPEED_1)
		return "Maximum Speed +1";
	if (flag == CF_MAXSPEED_2)
		return "Maximum Speed +2";
	if (flag == CF_MAXCARGO_10)
		return "Cargo Load +10%";
	if (flag == CF_HULL_REPAIR_2)
		return "Hull Repair x2";
	if (flag == CF_HULL_REPAIR_3)
		return "Hull Repair x3";
	if (flag == CF_WEAPONS_REPAIR_2)
		return "Weapons Repair x2";
	if (flag == CF_WEAPONS_REPAIR_3)
		return "Weapons Repair x3";
	if (flag == CF_SAIL_REPAIR_2)
		return "Sail Repair x2";
	if (flag == CF_SAIL_REPAIR_3)
		return "Sail Repair x3";

	return "Unknown";
}

/*
 * Whether this chief can be hired in `room` (a room vnum).  Same five-entry,
 * 0-terminated convention as ShipCrewData::hire_room().
 */
bool ShipChiefData::hire_room(int room) const
{
	for (int i = 0; i < 5; i++)
		if (hire_rooms[i] == room)
			return true;
	return false;
}

/*
 * The chief's speciality as a display word: "Deck", "Guns" or
 * "Maintenance".  Returns "" for NO_CHIEF.
 */
const char *ShipChiefData::get_spec() const
{
	switch (type)
	{
	case SAIL_CHIEF:
		return "Deck";
	case GUNS_CHIEF:
		return "Guns";
	case RPAR_CHIEF:
		return "Maintenance";
	};
	return "";
}

/*
 * Replace `percent` of the crew with fresh hands.
 *
 * Each of the three skills decays `percent` of the way from its current
 * value back down to the crew type's base -- so a crew that has trained well
 * above base loses the most, and a crew still at base loses nothing.  Called
 * when casualties are taken.
 */
void ShipCrew::replace_members(float percent)
{
	sail_skill = (float)ship_crew_data[index].base_sail_skill +
		     (sail_skill - (float)ship_crew_data[index].base_sail_skill) *
			     (100.0 - percent) / 100.0;
	guns_skill = (float)ship_crew_data[index].base_guns_skill +
		     (guns_skill - (float)ship_crew_data[index].base_guns_skill) *
			     (100.0 - percent) / 100.0;
	rpar_skill = (float)ship_crew_data[index].base_rpar_skill +
		     (rpar_skill - (float)ship_crew_data[index].base_rpar_skill) *
			     (100.0 - percent) / 100.0;
}

/*
 * Fatigue multiplier applied to turning and acceleration, in (0, 1].
 *
 * Returns 1.0 while stamina remains; past zero the crew keeps working but
 * progressively slower, the penalty deepening with how far into deficit they
 * have gone relative to their maximum.
 */
float ShipCrew::get_stamina_mod()
{
	if (stamina > 0)
		return 1.0;
	return 1.0 / (1.0 + (-stamina) / max_stamina / 3.0);
}
/*
 * Colour code for the crew's stamina readout: green while rested, yellow
 * into deficit, red once the deficit exceeds their maximum.
 */
const char *ShipCrew::get_stamina_prefix()
{
	if (stamina > 0)
		return "&+G";
	if (stamina > -max_stamina)
		return "&+Y";
	return "&+R";
}
/*
 * Stamina as shown to players.  Positive stamina is reported as-is;
 * a deficit is compressed through a square root so an exhausted crew shows a
 * small negative number rather than an alarming one.
 */
int ShipCrew::get_display_stamina()
{
	if (stamina > 0)
		return (int)stamina;
	return (int)-sqrt(-stamina);
}

/*
 * Train the crew's sailing skill by `raise`, subject to their deck chief's
 * bonuses and floor.  See skill_raise() for the shared rules.
 */
void ShipCrew::sail_skill_raise(float raise)
{
	skill_raise(raise, sail_skill, sail_chief);
}
/*
 * Train the crew's gunnery skill by `raise`, subject to their gunnery
 * chief's bonuses and floor.  See skill_raise().
 */
void ShipCrew::guns_skill_raise(float raise)
{
	skill_raise(raise, guns_skill, guns_chief);
}
/*
 * Train the crew's repair skill by `raise`, subject to their maintenance
 * chief's bonuses and floor.  See skill_raise().
 */
void ShipCrew::rpar_skill_raise(float raise)
{
	skill_raise(raise, rpar_skill, rpar_chief);
}

/*
 * Shared skill-gain rule for all three crew skills.
 *
 * `skill` is the field to raise, by reference.  `chief` is the index into
 * ship_chief_data[] of the chief responsible for it.
 *
 * Two effects: the chief's skill_gain_bonus scales all gains, and below the
 * chief's min_skill the crew learns at four times the rate -- a good chief
 * pulls a raw crew up to his own floor quickly, then trains them normally
 * above it.  The accelerated portion is consumed exactly, so a raise that
 * crosses the floor is not double-counted.
 */
void ShipCrew::skill_raise(float raise, float &skill, int chief)
{
	raise *= 1.0 + (float)ship_chief_data[chief].skill_gain_bonus / 100;
	if (skill < ship_chief_data[chief].min_skill)
	{
		skill += raise * 4;
		if (skill > ship_chief_data[chief].min_skill)
		{
			raise = (skill - ship_chief_data[chief].min_skill) / 4;
			skill = ship_chief_data[chief].min_skill;
		}
		else
			raise = 0;
	}
	skill += raise;
}

/*
 * Spend `val` stamina.  Stamina is allowed to go negative; see
 * get_stamina_mod() for what a deficit costs.  The ship parameter is unused
 * and retained only for the commented-out debug line below.
 */
void ShipCrew::reduce_stamina(float val, P_ship /*ship*/)
{
	stamina -= val;
	// act_to_all_in_ship_f(ship, "stamina: -%f", val);
}

/*
 * Flat sailing modifier from the crew's own quality plus their deck chief.
 * Feeds sail_mod_applied in update().
 */
int ShipCrew::sail_mod()
{
	return ship_crew_data[index].level + ship_crew_data[index].sail_mod +
	       ship_chief_data[sail_chief].skill_mod;
}

/*
 * Flat gunnery modifier from the crew's own quality plus their gunnery
 * chief.  Feeds guns_mod_applied in update().
 */
int ShipCrew::guns_mod()
{
	return ship_crew_data[index].level + ship_crew_data[index].guns_mod +
	       ship_chief_data[guns_chief].skill_mod;
}

/*
 * Flat repair modifier from the crew's own quality plus their maintenance
 * chief.  Feeds rpar_mod_applied in update().
 */
int ShipCrew::rpar_mod()
{
	return ship_crew_data[index].level + ship_crew_data[index].rpar_mod +
	       ship_chief_data[rpar_chief].skill_mod;
}

/*
 * Recompute the crew's derived state; call after anything changes the crew,
 * their chiefs, or their skills.
 *
 * Floors each skill at the crew type's base (a crew never performs worse than
 * the type you hired), refreshes max_stamina, and recomputes the three
 * *_mod_applied multipliers the rest of the ship system actually reads.
 *
 * Note that sailing and gunnery each blend in 20% of the repair figure, so a
 * crew that cannot maintain the ship is slightly worse at everything.
 */
void ShipCrew::update()
{
	sail_skill = MAX((float)ship_crew_data[index].base_sail_skill, sail_skill);
	guns_skill = MAX((float)ship_crew_data[index].base_guns_skill, guns_skill);
	rpar_skill = MAX((float)ship_crew_data[index].base_rpar_skill, rpar_skill);
	max_stamina = ship_crew_data[index].base_stamina;
	// TODO max_stamina = (int)((float)ship_crew_data[index].base_stamina * (1.0 + sqrt((float)(skill - ship_crew_data[index].min_skill) / 1000.0) / 300.0) );
	if (stamina > max_stamina)
		stamina = max_stamina;

	rpar_mod_applied = sqrt((float)rpar_skill) / 150.0 + (float)rpar_mod() * 0.03;
	sail_mod_applied = sqrt((float)sail_skill) / 150.0 + (float)sail_mod() * 0.03;
	sail_mod_applied = (sail_mod_applied * 8 + rpar_mod_applied * 2) / 10.0;
	guns_mod_applied = sqrt((float)guns_skill) / 150.0 + (float)guns_mod() * 0.03;
	guns_mod_applied = (guns_mod_applied * 8 + rpar_mod_applied * 2) / 10.0;
}

/*
 * Restore the crew to full stamina.  Used on hire and after shore leave.
 */
void ShipCrew::reset_stamina()
{
	stamina = max_stamina;
}

/*
 * Extra tactical contact range, in rooms, from the crew's scouting bonus.
 * Returns 0, 1 or 2; added to the base 35-room range in getmap() and
 * getcontacts().
 */
int ShipCrew::get_contact_range_mod() const
{
	if (IS_SET(ship_crew_data[index].flags, CF_SCOUT_RANGE_2))
		return 2;
	if (IS_SET(ship_crew_data[index].flags, CF_SCOUT_RANGE_1))
		return 1;
	return 0;
}
/*
 * Multiplier on sail repair rate from crew bonuses: 1, 2 or 3.
 */
int ShipCrew::get_sail_repair_mod() const
{
	if (IS_SET(ship_crew_data[index].flags, CF_SAIL_REPAIR_3))
		return 3;
	if (IS_SET(ship_crew_data[index].flags, CF_SAIL_REPAIR_2))
		return 2;
	return 1;
}
/*
 * Multiplier on weapon repair rate from crew bonuses: 1, 2 or 3.
 */
int ShipCrew::get_weapon_repair_mod() const
{
	if (IS_SET(ship_crew_data[index].flags, CF_WEAPONS_REPAIR_3))
		return 3;
	if (IS_SET(ship_crew_data[index].flags, CF_WEAPONS_REPAIR_2))
		return 2;
	return 1;
}
/*
 * Multiplier on hull repair rate from crew bonuses: 1, 2 or 3.
 */
int ShipCrew::get_hull_repair_mod() const
{
	if (IS_SET(ship_crew_data[index].flags, CF_HULL_REPAIR_3))
		return 3;
	if (IS_SET(ship_crew_data[index].flags, CF_HULL_REPAIR_2))
		return 2;
	return 1;
}
/*
 * Flat bonus to the ship class's speed ceiling from crew bonuses: 0, 1 or 2.
 * Applied in update_maxspeed().
 */
int ShipCrew::get_maxspeed_mod() const
{
	if (IS_SET(ship_crew_data[index].flags, CF_MAXSPEED_2))
		return 2;
	if (IS_SET(ship_crew_data[index].flags, CF_MAXSPEED_1))
		return 1;
	return 0;
}
/*
 * Multiplier on the ship class's cargo capacity from crew bonuses: 1.0 or
 * 1.1.  Applied by the SHIP_MAX_CARGO() macro.
 */
float ShipCrew::get_maxcargo_mod() const
{
	if (IS_SET(ship_crew_data[index].flags, CF_MAXCARGO_10))
		return 1.1;
	return 1.0;
}

/*
 * Recompute `ship`'s crew derived state.  Free-function wrapper around
 * ShipCrew::update() for call sites that only have a P_ship.
 */
void update_crew(P_ship ship)
{
	ship->crew.update();
}

/*
 * Restore `ship`'s crew to full stamina.  Free-function wrapper around
 * ShipCrew::reset_stamina().
 */
void reset_crew_stamina(P_ship ship)
{
	ship->crew.reset_stamina();
}

/*
 * Swap `ship` onto crew type `crew_index`, KEEPING their trained skills.
 *
 * This is the "hire a better crew" path: the new crew inherits whatever the
 * old one had learned, floored at the new type's base, so a promotion never
 * costs you training.  `skill_drop` true applies a small random 1-5% loss
 * first, representing the churn of replacing hands.
 *
 * `crew_index` is an index into ship_crew_data[]; out-of-range values and a
 * NULL ship are ignored.  Contrast set_crew(), which can reset skills.
 */
void change_crew(P_ship ship, int crew_index, bool skill_drop)
{
	/*
	 * ship_crew_data[] holds MAXCREWS entries, so MAXCREWS itself is one
	 * past the end.  The index is also stored on the ship, where every
	 * later ship_crew_data[index] read would inherit the overrun.
	 */
	if (crew_index < 0 || crew_index >= MAXCREWS)
		return;

	if (ship == NULL)
		return;

	if (skill_drop)
	{
		ship->crew.sail_skill -= ship->crew.sail_skill * ((float)number(20, 100) / 2000);
		ship->crew.guns_skill -= ship->crew.guns_skill * ((float)number(20, 100) / 2000);
		ship->crew.rpar_skill -= ship->crew.rpar_skill * ((float)number(20, 100) / 2000);
	}

	ship->crew.index = crew_index;
	ship->crew.sail_skill =
		MAX((float)ship_crew_data[crew_index].base_sail_skill, ship->crew.sail_skill);
	ship->crew.guns_skill =
		MAX((float)ship_crew_data[crew_index].base_guns_skill, ship->crew.guns_skill);
	ship->crew.rpar_skill =
		MAX((float)ship_crew_data[crew_index].base_rpar_skill, ship->crew.rpar_skill);
	ship->crew.update();
	ship->crew.reset_stamina();
}

/*
 * Put `ship` on crew type `crew_index`, optionally resetting skills.
 *
 * `reset_skills` true -- the default -- drops all three skills back to the
 * new type's base; that is what ship creation and immortal commands want.
 * Pass false to keep the existing skills, floored at the new base by the
 * subsequent update().
 *
 * `crew_index` is an index into ship_crew_data[]; out-of-range values and a
 * NULL ship are ignored.
 */
void set_crew(P_ship ship, int crew_index, bool reset_skills)
{
	/* Same one-past-the-end guard as change_crew(); see the note there. */
	if (crew_index < 0 || crew_index >= MAXCREWS)
		return;

	if (ship == NULL)
		return;

	ship->crew.index = crew_index;
	if (reset_skills)
	{
		ship->crew.sail_skill = ship_crew_data[crew_index].base_sail_skill;
		ship->crew.guns_skill = ship_crew_data[crew_index].base_guns_skill;
		ship->crew.rpar_skill = ship_crew_data[crew_index].base_rpar_skill;
	}
	ship->crew.update();
	ship->crew.reset_stamina();
}

/*
 * Assign the chief at `chief_index` to the department they specialise in.
 *
 * A chief slots into exactly one of deck/guns/maintenance according to their
 * own `type` field, so the caller does not choose the department.  The one
 * exception is NO_CHIEF (index 0), which clears all three at once.
 *
 * `chief_index` indexes ship_chief_data[]; out-of-range values and a NULL
 * ship are ignored.
 */
void set_chief(P_ship ship, int chief_index)
{
	/*
	 * Reached from the immortal "setship <ship> chief <n>" command with a
	 * raw atoi() value, so the index has to be validated here.
	 */
	if (!ship || chief_index < 0 || chief_index >= MAXCHIEFS)
		return;

	if (ship_chief_data[chief_index].type == SAIL_CHIEF)
		ship->crew.sail_chief = chief_index;
	if (ship_chief_data[chief_index].type == GUNS_CHIEF)
		ship->crew.guns_chief = chief_index;
	if (ship_chief_data[chief_index].type == RPAR_CHIEF)
		ship->crew.rpar_chief = chief_index;
	if (ship_chief_data[chief_index].type == NO_CHIEF)
	{
		ship->crew.sail_chief = chief_index;
		ship->crew.guns_chief = chief_index;
		ship->crew.rpar_chief = chief_index;
	}
}

/*
 * Empty every cargo and contraband slot on `ship`.
 *
 * Marks the slots SLOT_EMPTY without producing crates or paying anyone --
 * the cargo simply ceases to exist.  For the player-visible version that
 * drops crates into the water, see jettison_cargo() / jettison_contraband().
 */
void clear_cargo(P_ship ship)
{
	for (int i = 0; i < MAXSLOTS; i++)
	{
		if (ship->slot[i].type == SLOT_CARGO || ship->slot[i].type == SLOT_CONTRABAND)
		{
			ship->slot[i].type = SLOT_EMPTY;
		}
	}
}

/*
 * Return the ship's owner if they are personally aboard, else NULL.
 *
 * Matches by name against SHIP_OWNER(), skipping NPCs.  Used to gate the
 * things only the owner may do from on deck.  A NULL ship yields NULL.
 */
P_char captain_is_aboard(P_ship ship)
{
	if (!(ship))
		return NULL;

	for (int i = 0; i < ship->room_count; i++)
	{
		P_char ch_next = 0;
		for (P_char ch = world[real_room(ship->room[i].roomnum)].people; ch; ch = ch_next)
		{
			if (ch)
			{
				ch_next = ch->next_in_room;

				if (IS_NPC(ch))
					continue;

				if (IS_PC(ch) && isname(GET_NAME(ch), SHIP_OWNER(ship)))
					return ch;
			}
		}
	}
	return NULL;
}

/*
 * Whether any player character is aboard `ship`.
 *
 * Used to hold back destructive automation while people are still on board:
 * finish_sinking() postpones the sinking, and the NPC AI refuses to unload
 * the ship.  NPCs, including the ship's own crew mobs, do not count.
 */
bool pc_is_aboard(P_ship ship)
{
	if (!(ship))
		return false;

	for (int i = 0; i < ship->room_count; i++)
	{
		for (P_char ch = world[real_room(ship->room[i].roomnum)].people; ch;
		     ch = ch->next_in_room)
		{
			if (IS_PC(ch))
				return true;
		}
	}
	return false;
}

/*
 * Map a ship's interior room vnum to that ship's anchor room.
 *
 * The anchor is where a character in this room really "is" for purposes
 * outside the ship -- teleport targets, tracking, and so on.  Returns `room`
 * unchanged when it does not belong to any ship, so this is safe to call on
 * ordinary world rooms.
 */
int anchor_room(int room)
{
	ShipVisitor svs;
	for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
	{
		for (int i = 0; i < svs->room_count; i++)
		{
			if (room == svs->room[i].roomnum)
				return svs->anchor;
		}
	}
	return room;
}

/*
 * Mount weapon type `w_num` in `slot`, facing `arc`.
 *
 * Initialises the slot to a fresh, undamaged, fully loaded weapon: `arc` is
 * one of the SIDE_* constants, ammunition starts at the weapon's magazine
 * size, and the reload timer starts clear.
 *
 * Performs no validation -- the caller must have checked that the slot is
 * free, the weapon is legal for the hull (ship_allowed_weapons[]), the arc
 * has a free mount, and the weight fits.  See buy_weapon() in ship_shop.c
 * for the full purchase path.
 */
void set_weapon(P_ship ship, int slot, int w_num, int arc)
{
	ship->slot[slot].type = SLOT_WEAPON;
	ship->slot[slot].index = w_num;
	ship->slot[slot].position = arc;
	ship->slot[slot].timer = 0;
	ship->slot[slot].val0 = w_num; // ammo type
	ship->slot[slot].val1 = weapon_data[w_num].ammo; // ammo count
	ship->slot[slot].val2 = 0; // damage level
}

/*
 * Fit equipment type `e_num` in `slot`.
 *
 * Equipment always mounts at SLOT_EQUI rather than in an arc.  As with
 * set_weapon(), all validation is the caller's job; see buy_equipment() in
 * ship_shop.c.
 */
void set_equipment(P_ship ship, int slot, int e_num)
{
	ship->slot[slot].type = SLOT_EQUIPMENT;
	ship->slot[slot].index = e_num;
	ship->slot[slot].position = SLOT_EQUI;
	ship->slot[slot].timer = 0;
	ship->slot[slot].val0 = 0;
	ship->slot[slot].val1 = 0;
	ship->slot[slot].val2 = 0;
}

/*
 * Expected hull damage per volley from this weapon, before armour.
 *
 * Mean damage per fragment, times fragment count, times the weapon's hull
 * damage modifier, times the fraction of shots that do NOT hit sails.  Used
 * to rank weapons on shop listings, not in combat resolution.
 */
float WeaponData::average_hull_damage() const
{
	return ((float)(min_damage + max_damage) / 2.0) * ((float)fragments) *
	       ((float)hull_dam / 100.0) * ((100.0 - (float)sail_hit) / 100.0);
}

/*
 * Expected sail damage per volley from this weapon.
 *
 * The mirror of average_hull_damage(): the same mean damage scaled by the
 * sail damage modifier and the fraction of shots that DO hit sails.
 */
float WeaponData::average_sail_damage() const
{
	return ((float)(min_damage + max_damage) / 2.0) * ((float)fragments) *
	       ((float)sail_dam / 100.0) * ((float)sail_hit / 100.0);
}

/*
 * Fold `dir` in place into [0, 360).  Used wherever headings and bearings
 * are added or subtracted.
 */
void normalize_direction(float &dir)
{
	while (dir >= 360)
		dir = dir - 360;
	while (dir < 0)
		dir = dir + 360;
}

/*
 * Spelled-out arc name for player messages: "forward", "port", "rear",
 * "starboard", or "" for an unrecognised arc.  Returns a static literal.
 * Compare get_arc_indicator(), which gives the one-letter form.
 */
const char *get_arc_name(int arc)
{
	switch (arc)
	{
	case SIDE_FORE:
		return "forward";
	case SIDE_PORT:
		return "port";
	case SIDE_REAR:
		return "rear";
	case SIDE_STAR:
		return "starboard";
	}
	return "";
}

/*
 * Colour code for a condition readout: green above two thirds, yellow above
 * one third, red below.
 *
 * `light` selects the bright variant of each colour, which is how armour and
 * internal-structure readouts are told apart on the damage display.  Returns
 * a static literal; see the SHIP_ARMOR_COND / SHIP_INTERNAL_COND macros.
 */
const char *condition_prefix(int maxhp, int curhp, bool light)
{
	if (curhp < (maxhp / 3))
	{
		return light ? "&+R" : "&+r";
	}
	else if (curhp < ((maxhp * 2) / 3))
	{
		return light ? "&+Y" : "&+y";
	}
	else
	{
		return light ? "&+G" : "&+g";
	}
}

/*
 * Distance from `ship` to `target`, where `target` sits at tactical map cell
 * (`x`, `y`).
 *
 * The target's own fractional offset within its cell is added in, so ships
 * sharing a cell still have a meaningful sub-room separation; the z axis
 * covers flying ships.
 */
static float ship_range(P_ship ship, P_ship target, int x, int y)
{
	return range(ship->x, ship->y, ship->z, x + target->x - 50.0, y + target->y - 50.0,
		     target->z);
}

/*
 * Straight-line distance between two points in map space.  Plain 3-D
 * Euclidean; the z axis is altitude, which is how flying ships stay out of
 * reach of surface guns.
 */
float range(float x1, float y1, float z1, float x2, float y2, float z2)
{
	float dx, dy, dz, range;

	dx = x2 - x1;
	dy = y2 - y1;
	dz = z2 - z1;

	range = sqrt((dx * dx) + (dy * dy) + (dz * dz));
	return range;
}

/*
 * Whether `ship` may occupy `room` (a real room index).
 *
 * Everything below vnum 110000 is off limits -- that is the boundary of the
 * sailable world.  Above it, a surface ship needs an ocean map room, while a
 * flying ship will take any map room except mountains.
 */
bool is_valid_sailing_location(P_ship ship, int room)
{
	if (world[room].number < 110000)
		return false;

	if (SHIP_FLYING(ship))
	{
		if (!IS_MAP_ROOM(room) || world[room].sector_type == SECT_MOUNTAIN)
		{
			return false;
		}
	}
	else
	{
		if (!IS_MAP_ROOM(room) || world[room].sector_type != SECT_OCEAN)
		{
			return false;
		}
	}
	return true;
}

/*
 * Whether `ship` has a ram fitted.  See try_ram_ship() in ship_combat.c.
 */
bool has_eq_ram(const ShipData *ship)
{
	return eq_ram_slot(ship) != -1;
}
/*
 * Index of the slot holding the ship's ram, or -1 if it has none.
 */
int eq_ram_slot(const ShipData *ship)
{
	for (int slot = 0; slot < MAXSLOTS; slot++)
		if (ship->slot[slot].type == SLOT_EQUIPMENT && ship->slot[slot].index == E_RAM)
			return slot;
	return -1;
}
/*
 * Damage a ram strike from this ship inflicts.  Equal to the ram's weight,
 * so heavier hulls hit harder -- see eq_ram_weight().
 */
int eq_ram_damage(const ShipData *ship)
{
	return eq_ram_weight(ship);
}
/*
 * Weight of a ram sized for this hull.  Scales with the class's hull weight
 * rather than being a flat figure, so a ram costs every class roughly the
 * same fraction of its weight budget.
 */
int eq_ram_weight(const ShipData *ship)
{
	return (SHIP_HULL_WEIGHT(ship) + 10) / 24;
}
/*
 * Purchase price of a ram for this hull, again scaled by hull weight.
 */
int eq_ram_cost(const ShipData *ship)
{
	return SHIP_HULL_WEIGHT(ship) * 1000;
}

/*
 * Whether `ship` has a levistone fitted -- the equipment that lets a hull
 * leave the water.  See fly_ship() / land_ship() in ship_base.c.
 */
bool has_eq_levistone(const ShipData *ship)
{
	return eq_levistone_slot(ship) != -1;
}
/*
 * Index of the slot holding the ship's levistone, or -1 if it has none.
 */
int eq_levistone_slot(const ShipData *ship)
{
	for (int slot = 0; slot < MAXSLOTS; slot++)
		if (ship->slot[slot].type == SLOT_EQUIPMENT &&
		    ship->slot[slot].index == E_LEVISTONE)
			return slot;
	return -1;
}
/*
 * Weight of a levistone sized for this hull.  Charged only while the ship is
 * on the water; ShipSlot::get_weight() zeroes it in flight, since the stone
 * is carrying itself.
 */
int eq_levistone_weight(const ShipData *ship)
{
	return (SHIP_HULL_WEIGHT(ship) + 50) / 40;
}

/*
 * Whether `slot` holds the diplomat -- the equipment that legitimises
 * contraband.  `slot` must be a valid slot index; unlike most helpers here
 * this one does not search, it tests one slot.
 */
bool is_diplomat_slot(const ShipData *ship, int slot)
{
	if (ship->slot[slot].type == SLOT_EQUIPMENT && ship->slot[slot].index == E_DIPLOMAT)
	{
		return TRUE;
	}
	return FALSE;
}

/*
 * Index of the slot holding the ship's diplomat, or -1 if it has none.
 */
int eq_diplomat_slot(const ShipData *ship)
{
	for (int slot = 0; slot < MAXSLOTS; slot++)
	{
		if (is_diplomat_slot(ship, slot))
		{
			return slot;
		}
	}
	return -1;
}

/*
 * Whether `ship` carries a diplomat.  See check_contraband() in
 * ship_cargo.c for what that buys you.
 */
bool has_eq_diplomat(const ShipData *ship)
{
	return eq_diplomat_slot(ship) != -1;
}

/*
 * Weight of a diplomat's quarters aboard this hull, scaled by hull weight.
 */
int eq_diplomat_weight(const ShipData *ship)
{
	return (SHIP_HULL_WEIGHT(ship) + 1) / 24;
}

/*
 * Whether `ship` has no weapon mounted in any slot.  Used to decide that a
 * small unarmed hull is harmless enough not to trigger open-ocean PvP.
 */
bool has_no_weapons(const ShipData *ship)
{
	int i;
	for (i = 0; i < MAXSLOTS; i++)
	{
		if (ship->slot[i].type == SLOT_WEAPON)
			return FALSE;
	}
	return TRUE;
}

/*
 * Whether any two hostile player ships are currently in contact anywhere on
 * the ocean -- the global flag that puts the seas into a PvP state.
 *
 * A ship counts only if it is undocked, not an NPC ship, and not a token
 * hull (sloops never count; yachts count only when armed).  Two such ships
 * of different races within contact range of each other is enough.
 *
 * Expensive: this walks every ship and rebuilds the shared contacts[] buffer
 * for each, so any contacts[] contents the caller was holding are destroyed.
 */
bool ocean_pvp_state()
{
	ShipVisitor svs;
	for (bool fn = shipObjHash.get_first(svs); fn; fn = shipObjHash.get_next(svs))
	{
		P_ship ship = svs;

		// Docked ships and NPC ships don't trigger a PvP state.
		if (SHIP_DOCKED(ship) || ship->race == NPCSHIP)
			continue;

		// Tiny ships don't trigger PvP?  Hrm... Let's make that weaponless small ships.
		if (SHIP_CLASS(ship) == SH_SLOOP ||
		    (SHIP_CLASS(ship) == SH_YACHT && has_no_weapons(ship)))
		{
			continue;
		}

		int contact_count = getcontacts(ship, false);
		if (contact_count == 0)
		{
			continue;
		}

		for (int i = 0; i < contact_count; i++)
		{
			if (contacts[i].ship == ship)
				continue;
			if (SHIP_DOCKED(contacts[i].ship) || contacts[i].ship->race == NPCSHIP)
				continue;

			// If they're two non-NPC ships (handled above) that have different races.
			if (contacts[i].ship->race != ship->race)
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

/*
 * Drop up to `crates` cargo crates into the water at the ship's position.
 *
 * Each crate independently has a 50% chance of surviving the drop, so
 * roughly half of what is thrown over actually becomes salvageable objects;
 * the rest is lost.  `index` is the commodity, `type` is 1 for cargo and 2
 * for contraband, and both are stamped onto the crate object so
 * salvage_cargo() can reconstitute them.
 *
 * Returns FALSE if the ship is not over water or the crate prototype cannot
 * be loaded.
 */
int jettison_crates(P_ship ship, int crates, int index, int type)
{
	if (!IS_WATER_ROOM(ship->location))
		return FALSE;
	for (int i = 0; i < crates; i++)
	{
		if (number(1, 2) == 1)
			continue;

		int r_num;
		if ((r_num = real_object(VOBJ_CARGO_CRATE)) < 0)
			return FALSE;
		P_obj crate = read_object(r_num, REAL);
		if (!crate)
			return FALSE;

		crate->value[0] = index;
		crate->value[1] = type;
		obj_to_room(crate, ship->location);
	}
	return TRUE;
}

/*
 * Throw up to `left` units of ordinary cargo overboard.
 *
 * Works through the slots in order, emptying each and moving on until the
 * requested amount is gone.  Roughly half becomes floating crates that
 * anyone can salvage; see jettison_crates().
 *
 * `ch` may be NULL for automated jettison (see jettison_all()), in which case
 * no messages are sent.  Always returns TRUE.  Queues a ship save when
 * anything was actually thrown.
 */
int jettison_cargo(P_char ch, P_ship ship, int left)
{
	int done = 0;

	for (int i = 0; i < MAXSLOTS; i++)
	{
		if (ship->slot[i].type == SLOT_CARGO)
		{
			if (ship->slot[i].val0 > left)
			{
				if (ch)
					send_to_char_f(ch,
						       "%d units of %s have been jettisoned!\r\n",
						       left, cargo_type_name(ship->slot[i].index));
				jettison_crates(ship, left, ship->slot[i].index, 1);
				ship->slot[i].val0 -= left;
				done += left;
				break;
			}
			else
			{
				if (ch)
					send_to_char_f(ch,
						       "%d units of %s have been jettisoned!\r\n",
						       ship->slot[i].val0,
						       cargo_type_name(ship->slot[i].index));
				jettison_crates(ship, ship->slot[i].val0, ship->slot[i].index, 1);
				left -= ship->slot[i].val0;
				done += ship->slot[i].val0;
				ship->slot[i].clear();
				if (left == 0)
					break;
			}
		}
	}
	if (done == 0)
	{
		if (ch)
			send_to_char("You have no cargo to jettison!\r\n", ch);
	}
	else
	{
		update_ship_status(ship);
		queue_ship_save(ship, "cargo jettison");
	}
	return TRUE;
}

/*
 * Throw up to `left` units of contraband overboard.
 *
 * Identical to jettison_cargo() but for contraband slots -- most often used
 * to destroy the evidence before a customs inspection.  `ch` may be NULL.
 * Always returns TRUE.
 */
int jettison_contraband(P_char ch, P_ship ship, int left)
{
	int done = 0;

	for (int i = 0; i < MAXSLOTS; i++)
	{
		if (ship->slot[i].type == SLOT_CONTRABAND)
		{
			if (ship->slot[i].val0 > left)
			{
				if (ch)
					send_to_char_f(ch,
						       "%d units of %s have been jettisoned!\r\n",
						       left, contra_type_name(ship->slot[i].index));
				jettison_crates(ship, left, ship->slot[i].index, 2);
				ship->slot[i].val0 -= left;
				done += left;
				break;
			}
			else
			{
				if (ch)
					send_to_char_f(ch,
						       "%d units of %s have been jettisoned!\r\n",
						       ship->slot[i].val0,
						       contra_type_name(ship->slot[i].index));
				jettison_crates(ship, ship->slot[i].val0, ship->slot[i].index, 2);
				left -= ship->slot[i].val0;
				done += ship->slot[i].val0;
				ship->slot[i].clear();
				if (left == 0)
					break;
			}
		}
	}
	if (done == 0)
	{
		if (ch)
			send_to_char("You have no contraband to jettison!\r\n", ch);
	}
	else
	{
		update_ship_status(ship);
		queue_ship_save(ship, "cargo jettison");
	}
	return TRUE;
}

/*
 * Dump the entire hold, cargo and contraband alike, with no messages.
 * Used when the ship is lost or seized.
 */
void jettison_all(P_ship ship)
{
	jettison_cargo(0, ship, INT_MAX);
	jettison_contraband(0, ship, INT_MAX);
}

/*
 * Add one salvaged crate of commodity `index` to the hold.
 *
 * `type` is 1 for cargo, 2 for contraband.  Prefers to stack onto an
 * existing slot of the same commodity -- but only one with val1 == 0, i.e.
 * one that was not bought at a recorded invoice price, so salvage never
 * contaminates a purchased lot's cost basis.  Failing that it claims an
 * empty slot.
 *
 * Returns FALSE when every slot is full.
 */
int add_crate(P_ship ship, int index, int type)
{
	int slot = 0;
	for (; slot < MAXSLOTS; ++slot)
	{
		if (type == 1 && ship->slot[slot].type == SLOT_CARGO &&
		    ship->slot[slot].index == index && ship->slot[slot].val1 == 0)
			break;
		if (type == 2 && ship->slot[slot].type == SLOT_CONTRABAND &&
		    ship->slot[slot].index == index && ship->slot[slot].val1 == 0)
			break;
	}
	if (slot != MAXSLOTS)
	{
		ship->slot[slot].val0++;
	}
	else
	{
		for (slot = 0; slot < MAXSLOTS; ++slot)
		{
			if (ship->slot[slot].type == SLOT_EMPTY)
				break;
		}
		if (slot == MAXSLOTS)
			return FALSE;

		if (type == 2)
			ship->slot[slot].type = SLOT_CONTRABAND;
		else
			ship->slot[slot].type = SLOT_CARGO;
		ship->slot[slot].index = index;
		ship->slot[slot].position = SLOT_HOLD;
		ship->slot[slot].val0 = 1;
		ship->slot[slot].val1 = 0;
	}
	return TRUE;
}

/*
 * Fish up to `crates` floating crates out of the water into the hold.
 *
 * Capacity is the lesser of the ship's remaining cargo allowance and its
 * remaining weight allowance; warships, which have no cargo rating, are
 * allowed a fifth of their weight budget instead.  Stops early when the hold
 * fills or the water runs out of crates.
 *
 * `ch` may be NULL to salvage silently.  Always returns TRUE.
 */
int salvage_cargo(P_char ch, P_ship ship, int crates)
{
	if (crates <= 0)
	{
		if (ch)
			send_to_char("Invalid number of crates!\r\n", ch);
		return TRUE;
	}

	int available = SHIP_AVAIL_CARGO_SALVAGE(ship);
	if (available <= 0)
	{
		if (ch)
			send_to_char("You have no space on your ship!\r\n", ch);
		return TRUE;
	}

	crates = MIN(crates, available);

	int i = 0;
	P_obj obj = world[ship->location].contents;
	while (obj && i < crates)
	{
		P_obj next_obj = obj->next_content;
		if (obj_index[obj->R_num].virtual_number == VOBJ_CARGO_CRATE)
		{
			if (!add_crate(ship, obj->value[0], obj->value[1]))
			{
				if (ch)
					send_to_char("There is no more space on your ship.\r\n",
						     ch);
				break;
			}
			i++;
			extract_obj(obj, TRUE);
		}
		obj = next_obj;
	}
	if (i == 0)
	{
		if (ch)
			send_to_char("There is not nothing to salvage here.\r\n", ch);
		return TRUE;
	}
	if (ch)
		send_to_char_f(ch, "Your crew hooks %d crates from the ocean surface.\r\n", i);

	update_ship_status(ship);
	return TRUE;
}

/*
 * What the ship is worth: the hull's list price plus the list price of every
 * weapon and piece of equipment fitted.  Ignores cargo, crew and damage.
 */
int calculate_full_cost(P_ship ship)
{
	int cost = SHIPTYPE_COST(ship->m_class);

	for (int slot = 0; slot < MAXSLOTS; ++slot)
	{
		if (ship->slot[slot].type == SLOT_WEAPON)
			cost += weapon_data[ship->slot[slot].index].cost;
		if (ship->slot[slot].type == SLOT_EQUIPMENT)
			cost += equipment_data[ship->slot[slot].index].cost;
	}

	return cost;
}

/*
 * Raise the crew's gunnery skill and report the before/after figures to
 * `ch`.  An immortal tool -- ordinary gunnery training goes through
 * ShipCrew::guns_skill_raise() with no output.
 */
void ShipData::guns_skill_raise(P_char ch, float raise)
{
	char Gbuf1[MAX_STRING_LENGTH];

	sprintf(Gbuf1, "Name: '%s&n' Owner: '%s&n'\n\rOld Guns Skill: %.2f, ", this->name,
		this->ownername, this->crew.guns_skill);
	send_to_char(Gbuf1, ch);
	this->crew.guns_skill_raise(raise);
	sprintf(Gbuf1, "New Guns Skill: %.2f.\n\r", this->crew.guns_skill);
	send_to_char(Gbuf1, ch);
}

/*
 * Raise the crew's repair skill and report before/after to `ch`.  Immortal
 * tool; see guns_skill_raise().
 */
void ShipData::rpar_skill_raise(P_char ch, float raise)
{
	char Gbuf1[MAX_STRING_LENGTH];

	sprintf(Gbuf1, "Name: '%s&n' Owner: '%s&n'\n\rOld Repair Skill: %.2f, ", this->name,
		this->ownername, this->crew.rpar_skill);
	send_to_char(Gbuf1, ch);
	this->crew.rpar_skill_raise(raise);
	sprintf(Gbuf1, "New Repair Skill: %.2f.\n\r", this->crew.rpar_skill);
	send_to_char(Gbuf1, ch);
}

/*
 * Raise the crew's sailing skill and report before/after to `ch`.  Immortal
 * tool; see guns_skill_raise().
 */
void ShipData::sail_skill_raise(P_char ch, float raise)
{
	char Gbuf1[MAX_STRING_LENGTH];

	sprintf(Gbuf1, "Name: '%s&n' Owner: '%s&n'\n\rOld Sail Skill: %.2f, ", this->name,
		this->ownername, this->crew.sail_skill);
	send_to_char(Gbuf1, ch);
	this->crew.sail_skill_raise(raise);
	sprintf(Gbuf1, "New Sail Skill: %.2f.\n\r", this->crew.sail_skill);
	send_to_char(Gbuf1, ch);
}

/*
 * Assemble a crew's passive bonuses into one comma-separated sentence, e.g.
 * "Scout Range +2, Maximum Speed +1."
 *
 * Returns "" for a crew with no bonuses.  Otherwise returns a pointer to a
 * static buffer that is overwritten on the next call, so print it straight
 * away and do not retain it.  Iterates via ShipCrewData::get_next_bonus().
 */
const char *crew_bonuses(const ShipCrewData crew)
{
	int bonus_num = 0;
	static char buf[MAX_STRING_LENGTH];

	sprintf(buf, "%s", crew.get_next_bonus(&bonus_num));
	while (bonus_num != -1)
	{
		strcat(buf, ", ");
		strcat(buf, crew.get_next_bonus(&bonus_num));
	}

	if (strlen(buf) < 2)
	{
		return "";
	}
	else
	{
		// Replace the final ", " with "."
		buf[strlen(buf) - 2] = '.';
		buf[strlen(buf) - 1] = '\0';
	}

	return buf;
}

/*
 * Whether the ship already carries a capital item, and of which kind.
 *
 * Capital items are the one-per-ship heavy fittings flagged CAPITAL in
 * weapon_data[] / equipment_data[].  Returns SLOT_WEAPON or SLOT_EQUIPMENT
 * to say which kind was found, or SLOT_EMPTY for none.
 */
int ShipData::has_capital()
{
	for (int i = 0; i < MAXSLOTS; i++)
	{
		if ((slot[i].type == SLOT_WEAPON) &&
		    IS_SET(weapon_data[slot[i].index].flags, CAPITAL))
		{
			return SLOT_WEAPON;
		}
		if ((slot[i].type == SLOT_EQUIPMENT) &&
		    IS_SET(equipment_data[slot[i].index].flags, CAPITAL))
		{
			return SLOT_EQUIPMENT;
		}
	}
	return SLOT_EMPTY;
}

/*
 * Purchase-time guard for capital items: returns TRUE and explains to `ch`
 * why they cannot buy another, or FALSE if the ship has no capital item and
 * the sale may proceed.
 */
bool ShipData::buy_check_capital(P_char ch)
{
	switch (has_capital())
	{
	case SLOT_WEAPON:
		send_to_char(
			"&+gYou already have a capital weapon! You can only have one capital item.&n\n",
			ch);
		return TRUE;
	case SLOT_EQUIPMENT:
		send_to_char(
			"&+gYou already have capital equipment! You can only have one capital item.&n\n",
			ch);
		return TRUE;
	}
	return FALSE;
}
