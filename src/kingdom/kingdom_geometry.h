/*
 *  kingdom_geometry.h
 *  Duris
 *
 *  Ring geometry for the kingdom code: the claim-order table and the
 *  room <-> map-square conversions.
 *
 *  A realm is four concentric rings around the guildhall entrance square:
 *
 *      ring 1  3x3 box minus centre    8 squares    indices  1..8
 *      ring 2  5x5 ring               16 squares    indices  9..24
 *      ring 3  7x7 ring               24 squares    indices 25..48
 *      ring 4  9x9 ring               32 squares    indices 49..80
 *
 *  80 claimable squares, 81 including the hall. A ring must be completed
 *  before the next opens. Within a ring the order runs CLOCKWISE starting
 *  DUE NORTH of the hall: east along the top, south down the right, west
 *  along the bottom, north up the left, then closing east back to the start.
 *
 *  Axis convention, proven from the engine rather than assumed:
 *  origin (0,0) of a map zone is its NORTH-WEST corner, +x is EAST and
 *  +y is SOUTH (src/cmd/testcmd.c:182-189 labels disproom(0,-1) "One north"
 *  and disproom(0,1) "One south"; surface.wld room 570424 has north->vnum-400
 *  and south->vnum+400 with mapx 400). So dy is NEGATIVE to the north, and
 *  index 1 -- due north of the hall -- is offset (0,-1).
 *
 *  The table below is GENERATED AT COMPILE TIME from that rule and then
 *  checked by static_assert, so there is no hand-typed list of 80 pairs to
 *  get wrong and no runtime initialisation order to worry about.
 */

#ifndef _KINGDOM_GEOMETRY_H_
#define _KINGDOM_GEOMETRY_H_

#include <array>
#include <cstddef>

/* ------------------------------------------------------------------ *
 * Shape constants
 * ------------------------------------------------------------------ */

#define KINGDOM_MAX_RING 4
/* Chebyshev radius of the outermost ring: the footprint is a 9x9 box. */
#define KINGDOM_FOOTPRINT_RADIUS KINGDOM_MAX_RING
#define KINGDOM_FOOTPRINT_SIDE (2 * KINGDOM_FOOTPRINT_RADIUS + 1)
/* Claimable squares. The hall's own square is not one of them. */
#define KINGDOM_MAX_SQUARES 80

/* Two realms must not even touch, so their 9x9 footprints need at least one
 * unclaimed square between them: centres must differ by more than twice the
 * radius. Chebyshev 9 would let the footprints share an edge; 10 leaves a gap.
 * This supersedes the guildhall subsystem's MAX_GH_PROXIMITY_RADIUS of 3,
 * which is smaller than a single realm's own reach. */
#define KINGDOM_MIN_HALL_SEPARATION (2 * KINGDOM_FOOTPRINT_RADIUS + 2)

struct kingdom_offset
{
	int dx; /* +east */
	int dy; /* +south */
};

/* ------------------------------------------------------------------ *
 * The claim order, generated at compile time
 * ------------------------------------------------------------------ */

namespace kingdom_geometry_detail
{

/* One-based indexing: slot 0 is the hall itself and is never claimed, so
 * kingdom_claim_order()[n] is the offset of claim number n directly. */
using claim_table = std::array<kingdom_offset, KINGDOM_MAX_SQUARES + 1>;

constexpr claim_table build_claim_order()
{
	claim_table t{};
	t[0] = kingdom_offset{ 0, 0 }; /* the hall */

	std::size_t n = 1;
	for (int r = 1; r <= KINGDOM_MAX_RING; ++r)
	{
		/* Clockwise from due north. Each leg stops one short of the
		 * corner the next leg starts on, so no square is emitted twice. */
		for (int x = 0; x <= r; ++x) /* north edge, centre -> NE corner */
			t[n++] = kingdom_offset{ x, -r };
		for (int y = -r + 1; y <= r; ++y) /* east edge, down to SE corner */
			t[n++] = kingdom_offset{ r, y };
		for (int x = r - 1; x >= -r; --x) /* south edge, west to SW corner */
			t[n++] = kingdom_offset{ x, r };
		for (int y = r - 1; y >= -r; --y) /* west edge, north to NW corner */
			t[n++] = kingdom_offset{ -r, y };
		for (int x = -r + 1; x < 0; ++x) /* north edge, back toward centre */
			t[n++] = kingdom_offset{ x, -r };
	}
	return t;
}

constexpr int chebyshev(int dx, int dy)
{
	const int ax = dx < 0 ? -dx : dx;
	const int ay = dy < 0 ? -dy : dy;
	return ax > ay ? ax : ay;
}

} /* namespace kingdom_geometry_detail */

constexpr kingdom_geometry_detail::claim_table KINGDOM_CLAIM_ORDER =
	kingdom_geometry_detail::build_claim_order();

/* ------------------------------------------------------------------ *
 * Compile-time proof of the table's shape
 * ------------------------------------------------------------------ */

namespace kingdom_geometry_detail
{

/* Squares in ring r, and the last claim index of ring r. */
constexpr int ring_size(int r)
{
	return 8 * r;
}
constexpr int ring_last_index(int r)
{
	return 4 * r * (r + 1);
}

constexpr bool table_is_sound()
{
	/* every claimed square sits on the ring its index implies */
	for (int r = 1; r <= KINGDOM_MAX_RING; ++r)
	{
		const int lo = ring_last_index(r - 1) + 1;
		const int hi = ring_last_index(r);
		if (hi - lo + 1 != ring_size(r))
			return false;
		for (int i = lo; i <= hi; ++i)
			if (chebyshev(KINGDOM_CLAIM_ORDER[i].dx, KINGDOM_CLAIM_ORDER[i].dy) != r)
				return false;
	}
	/* no square is claimed twice, and none is the hall */
	for (std::size_t i = 1; i <= KINGDOM_MAX_SQUARES; ++i)
	{
		if (KINGDOM_CLAIM_ORDER[i].dx == 0 && KINGDOM_CLAIM_ORDER[i].dy == 0)
			return false;
		for (std::size_t j = i + 1; j <= KINGDOM_MAX_SQUARES; ++j)
			if (KINGDOM_CLAIM_ORDER[i].dx == KINGDOM_CLAIM_ORDER[j].dx &&
			    KINGDOM_CLAIM_ORDER[i].dy == KINGDOM_CLAIM_ORDER[j].dy)
				return false;
	}
	return true;
}

} /* namespace kingdom_geometry_detail */

static_assert(kingdom_geometry_detail::ring_last_index(KINGDOM_MAX_RING) == KINGDOM_MAX_SQUARES,
	      "the four rings must total exactly 80 claimable squares");
static_assert(kingdom_geometry_detail::ring_last_index(1) == 8, "ring 1 ends at claim 8");
static_assert(kingdom_geometry_detail::ring_last_index(2) == 24, "ring 2 ends at claim 24");
static_assert(kingdom_geometry_detail::ring_last_index(3) == 48, "ring 3 ends at claim 48");
/* claim 1 is due north; claim 2 is the north-east diagonal, i.e. clockwise */
static_assert(KINGDOM_CLAIM_ORDER[1].dx == 0 && KINGDOM_CLAIM_ORDER[1].dy == -1,
	      "claim 1 is the square due NORTH of the hall");
static_assert(KINGDOM_CLAIM_ORDER[2].dx == 1 && KINGDOM_CLAIM_ORDER[2].dy == -1,
	      "claim 2 is north-east: the ring runs CLOCKWISE");
static_assert(KINGDOM_CLAIM_ORDER[8].dx == -1 && KINGDOM_CLAIM_ORDER[8].dy == -1,
	      "claim 8 closes ring 1 on the north-west diagonal");
static_assert(KINGDOM_CLAIM_ORDER[9].dx == 0 && KINGDOM_CLAIM_ORDER[9].dy == -2,
	      "claim 9 opens ring 2 due north again");
static_assert(kingdom_geometry_detail::table_is_sound(),
	      "claim order must cover each ring exactly once, with no repeats");

/* ------------------------------------------------------------------ *
 * Pure index helpers (no world access)
 * ------------------------------------------------------------------ */

/* Ring number 1..KINGDOM_MAX_RING for a claim index, or 0 if out of range. */
int kingdom_ring_for_index(int index);
/* First and last claim index of a ring, or 0 for a bad ring. */
int kingdom_ring_first_index(int ring);
int kingdom_ring_last_index(int ring);
/* Number of squares in a ring, or 0 for a bad ring. */
int kingdom_ring_size(int ring);
/* Offset of claim `index`; returns false and leaves the outputs alone when
 * index is outside 1..KINGDOM_MAX_SQUARES. */
bool kingdom_offset_for_index(int index, int *dx, int *dy);
/* Inverse: claim index for an offset, or 0 when the offset is the hall or
 * lies outside the 9x9 footprint. */
int kingdom_index_for_offset(int dx, int dy);

/* ------------------------------------------------------------------ *
 * World access -- room <-> map square
 * ------------------------------------------------------------------ */

/* Zone-local square of a room. Returns false when the room is invalid or its
 * zone is not a ZONE_MAP zone. `zone` receives the zone index. */
bool kingdom_square_of_room(int rnum, int *zone, int *x, int *y);

/* The room at zone-local (x,y), or 0 when there is none.
 *
 * This deliberately does NOT use calculate_relative_room(): that helper WRAPS
 * toroidally at the zone edges (src/world/map.c:262-270), so a hall four
 * squares from a boundary would silently claim squares on the far side of the
 * continent. This one refuses to leave the grid. It also never returns rnum 0
 * as a success, because real_room0() uses 0 both for "the first room" and for
 * "no such vnum" (src/world/db.c:4477-4508). */
int kingdom_room_at(int zone, int x, int y);

/* The room `index` claims relative to a hall, or 0 if it falls off the grid or
 * has no room behind it. */
int kingdom_room_for_claim(int hall_rnum, int index);

#endif /* _KINGDOM_GEOMETRY_H_ */
