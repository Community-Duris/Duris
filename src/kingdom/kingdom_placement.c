/*
 *  kingdom_placement.c
 *  Duris
 *
 *  THE SINGLE AUTHORITY on whether a map square may belong to a realm.
 *  kingdom.c's guildhall gate, every claim in kingdom_claim.c and every cell
 *  of kingdom_display.c's grid ask THESE functions. None of them re-derives
 *  the rule, because two copies of an availability predicate drift and the
 *  weaker one lies.
 *
 *  A square is judged in a fixed order and the FIRST refusal is the one the
 *  player is shown, so the order runs from most fundamental to most local:
 *
 *      1  it is on the grid, and there is a room behind it
 *      2  its sector is ground a settlement could stand on
 *      3  it is not in the Underdark          (absolute -- RULINGS.md, 5)
 *      4  it is not within min_hometown_distance of a hometown
 *      5  it is not within min_entrance_distance of a way off the map
 *      6  it is not inside -- or touching -- another realm's 9x9 footprint
 *      7  no guildhall stands on it
 *
 *  WHERE THE TOWNS AND THE DOORWAYS ARE
 *  ------------------------------------
 *  Neither is a coordinate the engine publishes. A hometown is a ZONE
 *  property (zone_table[].hometown, core/structs.h:702) and town zones are not
 *  map zones, so a town owns no map square of its own. What it does own is the
 *  map squares that lead into it and the outdoor squares its guards patrol --
 *  world[].justice_area, which load_justice_area() stamps onto OUTSIDE rooms
 *  from the justice file (combat/justice.c:783-820). A "zone entrance" is,
 *  likewise, just a map square with an exit into some other zone.
 *
 *  Both are therefore derived once, by a single pass over the world table, and
 *  cached per map zone. The obvious alternative -- scanning the 61x61 box
 *  around each candidate -- is roughly 300,000 real_room0() binary searches per
 *  judged footprint, and kingdom_display.c judges a whole footprint every time
 *  a player looks at the territory grid.
 *
 *  Nothing in this file mutates the world, and nothing here charges anybody.
 *  It answers questions.
 */

#include "kingdom/kingdom_internal.h"

#include "core/structs.h"

#include "core/prototypes.h"
#include "core/utility.h"
#include "core/utils.h"
#include "guild/guildhall.h"
#include "world/map.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

extern struct room_data *world;
extern struct zone_data *zone_table;
extern int top_of_world;

/* ------------------------------------------------------------------ *
 * Verdicts
 * ------------------------------------------------------------------ */

/*
 * These are fragments, not sentences: callers paste them into "square 37 of 80
 * is refused (%s)" and into " 12 squares -- %s". NEVER return NULL -- every
 * caller hands the answer straight to a %s, and kingdom_display.c defends
 * against a null only because it cannot see this function's body.
 */
const char *kingdom_verdict_text(int verdict)
{
	switch (verdict)
	{
	case KSQ_OK:
		return "the ground would be granted";
	case KSQ_NO_ROOM:
		return "there is nothing there to hold";
	case KSQ_OFF_GRID:
		return "it lies beyond the edge of the map";
	case KSQ_BAD_SECTOR:
		return "the ground will not carry a settlement";
	case KSQ_UNDERDARK:
		return "the Underdark is barred to realms";
	case KSQ_NEAR_HOMETOWN:
		return "it lies too close to a hometown";
	case KSQ_NEAR_ENTRANCE:
		return "it lies too close to the way into another region";
	case KSQ_OTHER_REALM:
		return "it lies within another realm's domain";
	case KSQ_HAS_GUILDHALL:
		return "a guildhall already stands there";
	default:
		return "it is barred for a reason this code cannot name";
	}
}

/* ------------------------------------------------------------------ *
 * Terrain
 * ------------------------------------------------------------------ */

/*
 * A WHITELIST, and deliberately so. This is a permission predicate: one that
 * failed open would grant land nobody meant to grant, and core/defines.h gains
 * sector types over time (SECT_LAVA at 39 is the most recent). An unrecognised
 * sector is refused until somebody adds it here on purpose.
 */
static bool kingdom_sector_is_settleable(int sector)
{
	switch (sector)
	{
	/* Open ground a settlement can stand on. Wider than the guildhall
	 * subsystem's forest/hills/field (guild/guildhall_cmds.c:1523-1526),
	 * which is the rule for ONE room: a realm covers 81 squares and would
	 * never find that many of a single sector in one 9x9 box. */
	case SECT_CITY:
	case SECT_FIELD:
	case SECT_FOREST:
	case SECT_HILLS:
	case SECT_MOUNTAIN:
	case SECT_DESERT:
	case SECT_ARCTIC:
	case SECT_SWAMP:
	case SECT_ROAD:
	case SECT_SNOWY_FOREST:
		return true;

	/* Underdark FLOOR. Admitted here on purpose, even though no realm may
	 * ever hold one: the reason these squares are refused is the absolute
	 * Underdark ban, and the player is owed that reason rather than "bad
	 * terrain". The very next test in kingdom_judge_square() refuses every
	 * Underdark square whatever its sector, so admitting them here cannot
	 * grant anything. Underdark water, liquid mithril and no-ground fall
	 * through to the default and are refused twice over. */
	case SECT_UNDRWLD_WILD:
	case SECT_UNDRWLD_CITY:
	case SECT_UNDRWLD_INSIDE:
	case SECT_UNDRWLD_MOUNTAIN:
	case SECT_UNDRWLD_SLIME:
	case SECT_UNDRWLD_LOWCEIL:
	case SECT_UNDRWLD_MUSHROOM:
		return true;

	default:
		/* Out: every water sector, no-ground, lava, the elemental and outer
		 * planes, indoor rooms, and the castle sectors -- someone else's
		 * fortification is not open ground. */
		return false;
	}
}

/* True when (x,y) is inside this zone's own grid.
 *
 * kingdom_room_at() applies the same bound, but it answers 0 both for "off the
 * grid" and for "sparse map, no room at this square", and those are different
 * things to tell a player. Asking the bound separately is what lets
 * kingdom_judge_square() distinguish KSQ_OFF_GRID from KSQ_NO_ROOM. */
static bool kingdom_square_on_grid(int zone_idx, int x, int y)
{
	if (zone_idx < 0)
		return false;
	if (!IS_MAP_ZONE(zone_idx))
		return false;

	const struct zone_data *zone = &zone_table[zone_idx];

	if (zone->mapx <= 0 || zone->mapy <= 0)
		return false;

	return x >= 0 && x < zone->mapx && y >= 0 && y < zone->mapy;
}

/* Chebyshev (box) distance in map squares, within one zone.
 *
 * calculate_map_distance() is NOT used, here or anywhere in this module: it
 * returns the SQUARE of a Euclidean distance and -1 across zones
 * (world/map.c:278-305), and a caller that forgot either fact would compare a
 * squared distance against a radius and silently allow ~30x too much.
 *
 * Box distance also matches the realm's own geometry -- a ring is a Chebyshev
 * shell -- and refuses slightly MORE ground than a circle of the same radius,
 * which is the safe direction for a keep-away rule. */
static int kingdom_square_distance(int ax, int ay, int bx, int by)
{
	const int dx = ax > bx ? ax - bx : bx - ax;
	const int dy = ay > by ? ay - by : by - ay;

	return dx > dy ? dx : dy;
}

/* ------------------------------------------------------------------ *
 * The gateway index
 * ------------------------------------------------------------------ */

/* A map square of interest, in zone-local coordinates. The zone is the key of
 * the bucket it lives in, so it is not repeated per entry. */
struct kingdom_gate
{
	int x;
	int y;
};

/* Bucketed by map zone: only same-zone distances mean anything, and bucketing
 * keeps each judged square's scan to the one zone it sits in. */
static std::unordered_map<int, std::vector<kingdom_gate>> kingdom_hometown_gates;
static std::unordered_map<int, std::vector<kingdom_gate>> kingdom_entrance_gates;

/* Every map zone seen to contain an Underdark vnum. */
static std::unordered_set<int> kingdom_underdark_zones;

/* What the index above was built from. The world table is fixed after boot --
 * zone resets move mobs and objects, never rooms -- so this is built once on
 * first use. Recording the table it was built from means a world that WAS
 * rebuilt underneath us rebuilds the index instead of answering from a stale
 * one full of dangling rnums. */
static const struct room_data *kingdom_gates_world = NULL;
static int kingdom_gates_top = -1;
static bool kingdom_gates_built = false;

/* Build -- or rebuild, if the world table has been replaced since -- the
 * per-zone hometown and zone-entrance gate lists and the Underdark zone set,
 * in one pass over every map-square room. A no-op once built against the
 * current world; kingdom_judge_square() calls it lazily. */
static void kingdom_build_gate_cache(void)
{
	size_t home_total = 0;
	size_t door_total = 0;

	if (kingdom_gates_built && kingdom_gates_world == world &&
	    kingdom_gates_top == top_of_world)
		return;

	kingdom_hometown_gates.clear();
	kingdom_entrance_gates.clear();
	kingdom_underdark_zones.clear();

	/*
	 * rnum 0 is skipped, as everywhere in this module: it is both the first
	 * room and real_room0()'s "no such vnum" answer (world/db.c:4477-4508),
	 * and load_justice_area() writes the town number of every vnum it cannot
	 * resolve into world[0].justice_area (combat/justice.c:807), so that
	 * room's fields are not evidence of anything.
	 */
	for (int rnum = 1; rnum <= top_of_world; rnum++)
	{
		int zone_idx = -1, sx = -1, sy = -1;
		bool near_town = false;
		bool is_doorway = false;

		/* Also the map-zone filter: this fails for every room that is not a
		 * square on some grid. */
		if (!kingdom_square_of_room(rnum, &zone_idx, &sx, &sy))
			continue;

		if (IS_UD_MAP(rnum))
			kingdom_underdark_zones.insert(zone_idx);

		if (zone_table[zone_idx].hometown != 0 || world[rnum].justice_area != 0)
			near_town = true;

		for (int dir = 0; dir < NUM_EXITS; dir++)
		{
			const struct room_direction_data *ex = world[rnum].dir_option[dir];

			if (!ex)
				continue;

			/* to_room is an RNUM by now, not a vnum: renum_world()
			 * rewrote every exit at boot and FREED the ones that did not
			 * resolve (world/db.c:1475-1497), so a surviving exit points
			 * at a real room. The bound is still checked, because reading
			 * world[] past its top is not a bug worth risking on a
			 * comment. */
			const int to_rnum = ex->to_room;

			if (to_rnum <= 0 || to_rnum > top_of_world)
				continue;
			if (world[to_rnum].zone == zone_idx)
				continue;

			/* A crossing into any other zone is a doorway: the square is a
			 * chokepoint whatever lies on the far side, and a realm must
			 * not squat on it. */
			is_doorway = true;

			if (zone_table[world[to_rnum].zone].hometown != 0)
				near_town = true;
		}

		const kingdom_gate gate{ .x = sx, .y = sy };

		/*
		 * A town gateway is filed under BOTH lists. The hometown test runs
		 * first and would normally answer for it, but the two distances are
		 * independently configurable in lib/kingdom.cfg -- a short hometown
		 * radius must not let a town door slip past the longer entrance
		 * radius as well.
		 */
		if (near_town)
		{
			kingdom_hometown_gates[zone_idx].push_back(gate);
			kingdom_entrance_gates[zone_idx].push_back(gate);
			home_total++;
			door_total++;
		}
		else if (is_doorway)
		{
			kingdom_entrance_gates[zone_idx].push_back(gate);
			door_total++;
		}
	}

	kingdom_gates_world = world;
	kingdom_gates_top = top_of_world;
	kingdom_gates_built = true;

	logit(LOG_KINGDOM,
	      "placement: gateway index built -- %zu town square(s), %zu zone entrance(s), "
	      "%zu Underdark map zone(s).",
	      home_total, door_total, kingdom_underdark_zones.size());
}

/* True when (x,y) is nearer than `min_distance` squares to any gateway in its
 * zone. Strictly nearer: a square exactly min_distance away is the closest one
 * a realm may hold, which is what "a minimum distance of N" means. A
 * min_distance of 0 or less switches the rule off, as kingdom_config.c's lower
 * bound of 0 intends. */
static bool kingdom_near_gate(const std::unordered_map<int, std::vector<kingdom_gate>> &gates,
			      int zone_idx, int x, int y, int min_distance)
{
	if (min_distance <= 0)
		return false;

	const std::unordered_map<int, std::vector<kingdom_gate>>::const_iterator bucket =
		gates.find(zone_idx);

	if (bucket == gates.end())
		return false;

	for (const kingdom_gate &gate : bucket->second)
		if (kingdom_square_distance(x, y, gate.x, gate.y) < min_distance)
			return true;

	return false;
}

/* ------------------------------------------------------------------ *
 * The absolute Underdark ban
 * ------------------------------------------------------------------ */

/*
 * Ruled 2026-08-28 (RULINGS.md, answer 5): the ban is ABSOLUTE. Evil and
 * undead realms anchor on the surface like everyone else, so nothing here
 * consults the racewar.
 *
 * Three independent signals, because a ban that must never be evaded should
 * not rest on one datum being right: the vnum ranges the engine itself uses
 * (world/map.h:20), the Underdark sector family (core/utils.h:272-284), and
 * any map zone that was seen to contain an Underdark vnum at all -- which
 * catches a room whose vnum was placed outside those ranges but whose zone is
 * plainly the Underdark.
 */
static bool kingdom_square_is_underdark(int rnum, int zone_idx)
{
	if (IS_UD_MAP(rnum))
		return true;
	if (IS_UNDERWORLD(rnum))
		return true;

	return kingdom_underdark_zones.count(zone_idx) > 0;
}

/* ------------------------------------------------------------------ *
 * Other realms, and other halls
 * ------------------------------------------------------------------ */

/*
 * Ruling 2: two realms' footprints may not even TOUCH. A realm reserves its
 * whole 9x9 box whether or not it has claimed the squares yet, plus one square
 * of clear ground all the way round.
 *
 * The threshold is derived from KINGDOM_MIN_HALL_SEPARATION rather than typed
 * again, so the two can never drift apart. A candidate square sits at most
 * KINGDOM_FOOTPRINT_RADIUS from its own seat, so refusing every square closer
 * than (MIN_HALL_SEPARATION - FOOTPRINT_RADIUS) to a foreign seat puts the two
 * seats at least MIN_HALL_SEPARATION apart -- exactly the separation
 * kingdom_min_hall_separation() promises the guildhall subsystem.
 */
static bool kingdom_square_in_other_realm(int zone_idx, int x, int y, int ignore_assoc)
{
	const int keep_clear = KINGDOM_MIN_HALL_SEPARATION - KINGDOM_FOOTPRINT_RADIUS;

	for (const std::pair<const int, kingdom_realm> &entry : kingdom_realms)
	{
		int other_zone = -1, ox = -1, oy = -1;

		/* ignore_assoc 0 means "exclude nobody": kingdom.c passes it when
		 * siting a hall, where there is no realm yet to collide with. */
		if (ignore_assoc != 0 && entry.first == ignore_assoc)
			continue;

		/*
		 * Re-derived from the seat ROOM rather than read out of the realm's
		 * cached hall_x/hall_y. The room is the single source of truth for
		 * where a realm sits, and a record whose cache had drifted would
		 * move another guild's reservation with nobody noticing. A realm
		 * whose seat will not resolve is dormant and reserves nothing --
		 * kingdom_on_guildhall_changed() has already logged that.
		 */
		if (!kingdom_square_of_room(entry.second.hall_rnum, &other_zone, &ox, &oy))
			continue;
		if (other_zone != zone_idx)
			continue;

		if (kingdom_square_distance(x, y, ox, oy) < keep_clear)
			return true;
	}

	return false;
}

/*
 * find_by_outside_vnum(), NOT find_by_vnum(). find_by_vnum() searches a hall's
 * INTERIOR rooms (guild/guildhall.c:153-170), which live in the guildhall zone
 * at vnums 48020-48999 and are never map squares; outside_vnum is the map room
 * the hall actually stands on (guild/guildhall.c:175-183).
 * guildhall_map_check() hands a MAP vnum to find_by_vnum()
 * (guild/guildhall_cmds.c:1426), so its "there is already a guildhall here"
 * test can never match. This one asks the question that means something.
 *
 * This is not filtered by ignore_assoc: a realm's own second hall occupies its
 * square just as thoroughly as a stranger's, and the seat itself is offset
 * (0,0) and never judged as a claim.
 */
static bool kingdom_square_has_guildhall(int rnum)
{
	return Guildhall::find_by_outside_vnum(world[rnum].number) != NULL;
}

/* ------------------------------------------------------------------ *
 * The judgements
 * ------------------------------------------------------------------ */

/* The verdict on claim `index` of a realm seated at hall_rnum, tested in the
 * banner's order so the first refusal is the most fundamental one. A bad index
 * and a seat that will not resolve to a map square both answer KSQ_OFF_GRID. */
int kingdom_judge_square(int hall_rnum, int index, int racewar, int ignore_assoc)
{
	int dx = 0, dy = 0;
	int zone_idx = -1, hall_x = -1, hall_y = -1;

	/*
	 * `racewar` is deliberately not consulted, and that is a ruling rather
	 * than an omission. The siting rules LOOK racewar-sensitive -- the old
	 * guildhall code sent evil halls to Shady Grove and Khildarak
	 * (guild/guildhall_cmds.c:1449-1500) -- but ruling 5 made the one rule
	 * that would have varied, the Underdark, absolute for every side, and no
	 * other rule below distinguishes them. It stays on the seam because
	 * changing the signature would touch a frozen header and every caller;
	 * nothing here may branch on it without a new ruling.
	 */
	(void)racewar;

	if (!kingdom_offset_for_index(index, &dx, &dy))
		return KSQ_OFF_GRID; /* not a claim index at all */

	/* Fails for rnum 0, for a room outside the world table, and for any room
	 * that is not on a map grid -- so an unresolved seat cannot silently be
	 * treated as sitting at the origin of some zone. */
	if (!kingdom_square_of_room(hall_rnum, &zone_idx, &hall_x, &hall_y))
		return KSQ_OFF_GRID;

	{
		const int x = hall_x + dx;
		const int y = hall_y + dy;

		/* Asked before kingdom_room_at(), which cannot tell these two apart:
		 * it answers 0 for a square off the grid and for a hole in a sparse
		 * map alike. */
		if (!kingdom_square_on_grid(zone_idx, x, y))
			return KSQ_OFF_GRID;

		const int rnum = kingdom_room_at(zone_idx, x, y);

		/* kingdom_room_at() refuses to wrap at the map edge and never hands
		 * back rnum 0 as a success (kingdom_geometry.c:130-170). Most map
		 * zones are sparse, so a hole here is ordinary, not exceptional. */
		if (rnum <= 0)
			return KSQ_NO_ROOM;

		if (!kingdom_sector_is_settleable(world[rnum].sector_type))
			return KSQ_BAD_SECTOR;

		/* Builds on first use only; the Underdark zone set and both gateway
		 * lists come out of the same single pass over the world. */
		kingdom_build_gate_cache();

		if (kingdom_square_is_underdark(rnum, zone_idx))
			return KSQ_UNDERDARK;

		if (kingdom_near_gate(kingdom_hometown_gates, zone_idx, x, y,
				      kingdom_cfg.min_hometown_distance))
			return KSQ_NEAR_HOMETOWN;

		if (kingdom_near_gate(kingdom_entrance_gates, zone_idx, x, y,
				      kingdom_cfg.min_entrance_distance))
			return KSQ_NEAR_ENTRANCE;

		if (kingdom_square_in_other_realm(zone_idx, x, y, ignore_assoc))
			return KSQ_OTHER_REALM;

		if (kingdom_square_has_guildhall(rnum))
			return KSQ_HAS_GUILDHALL;
	}

	return KSQ_OK;
}

/*
 * Ruled 2026-08-28 (RULINGS.md, answer 1): ALL 80 squares must be eligible at
 * hall placement, so a hall may only be sited where a complete realm could
 * later exist. That is why this walks every index rather than stopping at the
 * realm's current highest_claim -- there is no partial-footprint case anywhere
 * in this subsystem.
 *
 * It stops at the FIRST refusal because the caller shows one reason, and
 * because the common case on a crowded map is refusal in ring 1.
 */
int kingdom_judge_footprint(int hall_rnum, int racewar, int ignore_assoc, int *bad_index)
{
	/* Cleared up front so a caller that prints *bad_index after a KSQ_OK
	 * cannot read an uninitialised local of its own. kingdom.c treats 0 as
	 * "no particular square" (kingdom.c:465). */
	if (bad_index)
		*bad_index = 0;

	for (int index = 1; index <= KINGDOM_MAX_SQUARES; index++)
	{
		const int verdict = kingdom_judge_square(hall_rnum, index, racewar, ignore_assoc);

		if (verdict != KSQ_OK)
		{
			if (bad_index)
				*bad_index = index;
			return verdict;
		}
	}

	return KSQ_OK;
}
