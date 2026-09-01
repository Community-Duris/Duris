/*
 *  kingdom_geometry.c
 *  Duris
 *
 *  Ring index helpers and the room <-> map-square conversions for the
 *  kingdom code. Pure arithmetic plus read-only world lookups: nothing here
 *  mutates the world, allocates, or performs I/O.
 *
 *  The claim-order table itself lives in kingdom_geometry.h, generated at
 *  compile time and checked by static_assert.
 */

#include "kingdom/kingdom_geometry.h"

#include "core/structs.h"

#include "core/prototypes.h"
#include "core/utils.h"
#include "world/map.h"

extern struct room_data *world;
extern struct zone_data *zone_table;
extern int top_of_world;

/* ------------------------------------------------------------------ *
 * Pure index helpers
 * ------------------------------------------------------------------ */

/* The last claim index of ring r is 4r(r+1): 8, 24, 48, 80. */
static int ring_last(int ring) { return 4 * ring * (ring + 1); }

int kingdom_ring_for_index(int index)
{
	if (index < 1 || index > KINGDOM_MAX_SQUARES)
		return 0;
	for (int ring = 1; ring <= KINGDOM_MAX_RING; ring++)
		if (index <= ring_last(ring))
			return ring;
	return 0;
}

int kingdom_ring_first_index(int ring)
{
	if (ring < 1 || ring > KINGDOM_MAX_RING)
		return 0;
	return ring_last(ring - 1) + 1;
}

int kingdom_ring_last_index(int ring)
{
	if (ring < 1 || ring > KINGDOM_MAX_RING)
		return 0;
	return ring_last(ring);
}

int kingdom_ring_size(int ring)
{
	if (ring < 1 || ring > KINGDOM_MAX_RING)
		return 0;
	return 8 * ring;
}

bool kingdom_offset_for_index(int index, int *dx, int *dy)
{
	if (index < 1 || index > KINGDOM_MAX_SQUARES)
		return false;
	if (dx)
		*dx = KINGDOM_CLAIM_ORDER[index].dx;
	if (dy)
		*dy = KINGDOM_CLAIM_ORDER[index].dy;
	return true;
}

int kingdom_index_for_offset(int dx, int dy)
{
	if (dx == 0 && dy == 0)
		return 0; /* the hall is not a claim */
	if (dx < -KINGDOM_FOOTPRINT_RADIUS || dx > KINGDOM_FOOTPRINT_RADIUS)
		return 0;
	if (dy < -KINGDOM_FOOTPRINT_RADIUS || dy > KINGDOM_FOOTPRINT_RADIUS)
		return 0;
	for (int i = 1; i <= KINGDOM_MAX_SQUARES; i++)
		if (KINGDOM_CLAIM_ORDER[i].dx == dx && KINGDOM_CLAIM_ORDER[i].dy == dy)
			return i;
	return 0;
}

/* ------------------------------------------------------------------ *
 * World access
 * ------------------------------------------------------------------ */

/* True when rnum indexes a real room. rnum 0 is rejected on purpose: it is
 * both the first room and real_room0()'s "no such vnum" answer, and every
 * caller here means the latter. */
static bool valid_rnum(int rnum) { return rnum > 0 && rnum <= top_of_world; }

bool kingdom_square_of_room(int rnum, int *zone_out, int *x, int *y)
{
	if (!valid_rnum(rnum))
		return false;

	const int zone_idx = world[rnum].zone;
	if (!IS_MAP_ZONE(zone_idx))
		return false;

	const struct zone_data *zone = &zone_table[zone_idx];
	if (zone->mapx <= 0 || zone->mapy <= 0)
		return false;

	const int zone_start_vnum = world[zone->real_bottom].number;
	const int offset = world[rnum].number - zone_start_vnum;
	if (offset < 0)
		return false;

	/* Same formula the engine uses (src/world/map.c:254-260) and the same one
	 * GMCP publishes as worldX/worldY (src/net/gmcp.c:456-472). */
	const int local_x = offset % zone->mapx;
	const int local_y = (offset / zone->mapx) % zone->mapy;

	if (zone_out)
		*zone_out = zone_idx;
	if (x)
		*x = local_x;
	if (y)
		*y = local_y;
	return true;
}

int kingdom_room_at(int zone_idx, int x, int y)
{
	if (zone_idx < 0)
		return 0;
	if (!IS_MAP_ZONE(zone_idx))
		return 0;

	const struct zone_data *zone = &zone_table[zone_idx];
	if (zone->mapx <= 0 || zone->mapy <= 0)
		return 0;

	/* THE WHOLE POINT OF THIS FUNCTION. calculate_relative_room() wraps a
	 * square that steps off the grid round to the opposite edge, so a hall
	 * near a boundary would claim land on the far side of the continent.
	 * Refuse instead. */
	if (x < 0 || x >= zone->mapx || y < 0 || y >= zone->mapy)
		return 0;

	const int zone_start_vnum = world[zone->real_bottom].number;
	const int rnum = real_room0(zone_start_vnum + x + (y * zone->mapx));

	/* real_room0() answers 0 for an unknown vnum, and most map zones are
	 * sparse (underdark defines 13,587 rooms across 160,000 squares), so a
	 * hole here is ordinary rather than exceptional. */
	if (!valid_rnum(rnum))
		return 0;

	/* Guard the non-injective tail: the surface zone defines 160,004 rooms in
	 * a 400x400 grid, so its four "Dispersement" rooms alias squares that are
	 * already occupied. Only accept the room that maps back to the square we
	 * asked for. */
	int back_zone = -1, back_x = -1, back_y = -1;
	if (!kingdom_square_of_room(rnum, &back_zone, &back_x, &back_y))
		return 0;
	if (back_zone != zone_idx || back_x != x || back_y != y)
		return 0;

	return rnum;
}

int kingdom_room_for_claim(int hall_rnum, int index)
{
	int dx = 0, dy = 0;
	if (!kingdom_offset_for_index(index, &dx, &dy))
		return 0;

	int zone_idx = -1, hx = -1, hy = -1;
	if (!kingdom_square_of_room(hall_rnum, &zone_idx, &hx, &hy))
		return 0;

	return kingdom_room_at(zone_idx, hx + dx, hy + dy);
}
