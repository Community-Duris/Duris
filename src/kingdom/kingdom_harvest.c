/*
 *  kingdom_harvest.c
 *  Duris
 *
 *  Harvest nodes: REAL OBJECTS scattered at random across the whole world --
 *  the Underdark included -- exactly as the ore and gem mines are, and the
 *  verb that works one.
 *
 *  RULED 2026-09-01 (the SECOND ruling on this module, and it supersedes the
 *  first), IN THREE PARTS
 *  ---------------------------------------------------------------------
 *  The first rewrite made nodes world objects but kept the harvest gated on
 *  land: you could only work a node standing on your own realm's ground.
 *  Review measured what that actually produces -- with nodes spread over the
 *  full map ranges the expected number inside even a maximum 80-square realm
 *  is about 0.07, and because off-realm nodes could never be worked they never
 *  depleted, permanently occupied the region quota, and turned the reload
 *  sweep into a no-op after boot. The owner re-ruled it:
 *
 *    1. NODES LOAD ANYWHERE EXCEPT ON LAND A REALM CONTROLS, and ANYONE may
 *       harvest one. There is no ownership test on the act of harvesting at
 *       all. What a realm gets instead is a YIELD BONUS DERIVED FROM THE TYPE
 *       OF LAND IT CONTROLS -- a realm holding forest works wood better, a
 *       realm holding hills and mountains works mineral better. The yield
 *       banks to the HARVESTER'S OWN realm, not to whoever owns the square.
 *    2. PLACEMENT IS COMPLETELY RANDOM ACROSS THE WHOLE WORLD, INCLUDING THE
 *       UNDERDARK, and an Underdark node must be THEMATIC -- a fungal stand,
 *       not a tree. Hence two prototype sets (kingdom_internal.h): 477-480 on
 *       the surface, 481-484 below it. No contradiction with the absolute
 *       Underdark ban: that ban is on siting a REALM, not on where a node may
 *       fall.
 *    3. A PARTIALLY HARVESTED NODE DISAPPEARS after a period. A pristine node
 *       does not rot. See "Decay" below.
 *
 *  WHAT A HARVEST DOES, EXACTLY
 *  ----------------------------
 *  Deposits are NON-WITHDRAWABLE (RULINGS.md, answer 3): nothing here takes a
 *  resource back out of a realm and nothing here hands a player an object. A
 *  completed harvest raises a counter on the harvester's own realm record and
 *  stops.
 *
 *  THREE OUTCOMES, and only the first banks anything:
 *
 *    * The harvester is a full member of a realm that is not in arrears --
 *      the yield is banked to that realm.
 *    * The harvester belongs to no realm -- the ground is still worked, and
 *      the material is left where it lies. THIS IS THE DECISION THE LANE WAS
 *      ASKED TO MAKE, and it follows from the ruling: "anyone may harvest" is
 *      not a permission to harvest if the work is refused, and there is no
 *      player-facing resource item anywhere in this design for the material
 *      to become. Working the ground for nothing is also what keeps turnover
 *      honest -- it is the mechanism the re-ruling exists to restore.
 *    * The harvester's realm is at KARR_NODES_DORMANT or worse -- same as
 *      above, and that is rung 2 of the arrears ladder biting.
 *
 *  A COMPLETED HARVEST ALWAYS SPENDS A CHARGE, in all three cases and even
 *  when the realm's store is at the cap. The previous build refused to spend a
 *  charge when nothing could be banked; that made the ground's depletion
 *  depend on WHO was digging, which is exactly the coupling the re-ruling
 *  removed. One rule now: finishing the work works the ground.
 *
 *  THE KINGDOM BONUS, AND WHY IT IS CACHED
 *  ---------------------------------------
 *  kingdom_terrain_squares() walks the realm's owned squares (claims
 *  1..highest_claim through kingdom_room_for_claim) and tallies how many
 *  favour each resource. Eighty kingdom_room_for_claim() calls -- each a
 *  geometry conversion plus a real_room0() binary search -- on every swing of
 *  every harvester is not a cost worth paying for a number that only changes
 *  when a ring is claimed or reverted, so the tally is cached per realm and
 *  stamped with (realm_id, hall_rnum, highest_claim). Anything that could
 *  change the answer changes the stamp, so there is no invalidation hook to
 *  forget to call.
 *
 *  DECAY
 *  -----
 *  value[2] is 0 on a pristine node and holds the MINUTE the node was first
 *  worked once it has been. KINGDOM_NODE_DECAY_MINS after that it is removed.
 *  Minutes rather than seconds because value[] is int: seconds-since-epoch
 *  overflows a signed 32-bit int in 2038 and minutes do not, ever.
 *
 *  The expiry is LAZY, as ruled: nothing schedules a per-node timer. A node
 *  past its window is invisible to kingdom_node_in_room() from that moment,
 *  and the object is physically removed by whichever comes first -- the
 *  reload sweep (kingdom_nodes_reap) or a player touching the room
 *  (kingdom_node_reap_room).
 *
 *  The same reap also removes any node that has come to stand on land a realm
 *  controls, which is how ruling 1's "except on land a realm controls" stays
 *  true after placement: a realm can claim a ring out from under a node, and
 *  the invariant would otherwise only hold at the instant of spawning. The
 *  quota it was occupying is freed and the sweep re-scatters it elsewhere.
 *
 *  WHAT WAS COPIED FROM mining.c, AND THE THREE THINGS DELIBERATELY NOT
 *  -------------------------------------------------------------------
 *  Copied: the region table's shape and its vnum ranges (mine_data[]), the
 *  spawn loop with its 10000-try budget, the 3/17/55/25 richness distribution
 *  and its charge ranges, the reload-count-from-duris.properties idiom, the
 *  CMD_SET_PERIODIC / CMD_PERIODIC self-extraction proc, and the per-tick
 *  event shape of the action itself.
 *
 *  NOT copied, each for a reason found by reading the engine:
 *
 *    a. mining.c binds its proc with obj_index[real_object0(V)].func.obj.
 *       real_object0() answers 0 BOTH for the first object in the table AND
 *       for "no such vnum" (world/db.c), so if the prototype is missing that
 *       line silently installs the proc on obj_index[0] -- some unrelated
 *       object. All eight prototypes (477-484) are in heavens.obj now, but a
 *       build whose world data lags the code is exactly when the hazard
 *       bites, so this file uses real_object(), which answers -1 for missing,
 *       and skips the binding with a log line.
 *
 *    b. mining.c computes its spawn window as real_room(START)..real_room(END).
 *       real_room() returns NOWHERE (-1) when that EXACT vnum is not a room
 *       (world/db.c), and the very next thing it does is number(start, end)
 *       and then world[to_room] -- a negative index into the world table. This
 *       file finds each region's rnum window by scanning the world table for
 *       rooms whose vnum falls IN the range, so a region whose endpoints do
 *       not happen to exist still works and an empty region is detected
 *       instead of indexed.
 *
 *    c. mining.c does mine->description = str_dup(...) without setting
 *       STRUNG_DESC1, and free_obj() only frees a description when that bit is
 *       set (world/db.c) -- so every mine leaks its description string. This
 *       file sets the bit alongside the assignment.
 *
 *  TWO ENGINE HAZARDS THAT SHAPE THE CODE BELOW
 *  --------------------------------------------
 *  obj_to_room() RETURNS VOID AND CAN DESTROY ITS ARGUMENT. It re-routes a
 *  sinkable object dropped in water to the Poseidon vault, merges a VOBJ_COINS
 *  pile into an existing one and extract_obj()s the loser, and can hand the
 *  object to falling_obj() (world/handler.c). extract_obj() ends in free_obj()
 *  (world/handler.c). Since there is no return value to branch on, EVERY read
 *  of the object is done BEFORE the call and nothing touches it afterwards.
 *  The same rule is why no node pointer is carried across a harvest tick: the
 *  node is re-found by number every time.
 *
 *  extract_obj() FREES, so no list is walked while extracting from it. The
 *  reaps collect into a vector (or read next_content before extracting) and
 *  extract afterwards.
 */

#include "kingdom/kingdom_internal.h"

#include "core/structs.h"

#include "cmd/interp.h"
#include "core/config.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "guild/assocs.h"
#include "kingdom/kingdom_geometry.h"
#include "net/comm.h"

#include <climits>
#include <cstddef>
#include <ctime>
#include <unordered_map>
#include <vector>

extern struct room_data *world;
extern int top_of_world;
extern P_index obj_index;
extern P_obj object_list;

/* ------------------------------------------------------------------ *
 * Tuning
 * ------------------------------------------------------------------ *
 * The compiled numbers are the defaults; anything a god might want to turn
 * during a season is read from duris.properties, the way mining reads
 * mines.maxSurfaceMap and mines.reloadMins. They deliberately do NOT live in
 * kingdom_config's struct, which is frozen and carries no harvest fields.
 */

/* Ticks of work a harvest takes, PULSE_VIOLENCE apart. */
#define KINGDOM_HARVEST_TICKS_DEFAULT 3
/* Vitality a tick of work costs, and the floor below which work is refused.
 * Mining spends 2-3 per tick and refuses below 10. */
#define KINGDOM_HARVEST_VITALITY_COST 2
#define KINGDOM_HARVEST_MIN_VITALITY 10
/* Owned squares FAVOURING A RESOURCE per step of that resource's yield bonus;
 * see kingdom_harvest_yield(). */
#define KINGDOM_HARVEST_SQUARES_PER_STEP 20
/* Ceiling on a stored resource. Deposits clamp rather than wrap: the counters
 * are persisted as signed values, and a wrapped negative store would read as a
 * debt the realm could never work off. */
#define KINGDOM_RESOURCE_CAP 1000000000L
/* Minutes between reload sweeps of one region, and the try budget of a single
 * placement. Both are mining's. */
#define KINGDOM_NODE_RELOAD_MINS_DEFAULT 10
#define KINGDOM_NODE_PLACEMENT_TRIES 10000
/* Percentage chance a node is placed on merely LEGAL ground rather than
 * FAVOURING ground, so that no resource is perfectly predictable from terrain.
 * mining.c uses the same 15. */
#define KINGDOM_NODE_WILD_PLACEMENT_PCT 15
/* Minutes a PARTIALLY WORKED node survives after the first charge is taken out
 * of it. An hour: long enough that a player who has to break off can come back
 * and finish, short enough that a half-worked node does not sit on the
 * region's quota all week. A pristine node is not on this clock at all. */
#define KINGDOM_NODE_DECAY_MINS_DEFAULT 60
/* A week, in minutes -- the clamp on the above, so a fat-fingered properties
 * line cannot make decay effectively infinite by accident. */
#define KINGDOM_NODE_DECAY_MINS_MAX 10080
/* Ceiling on a per-region, per-resource population. A properties typo of six
 * digits would otherwise spend six digits' worth of 10000-try placement
 * budgets inside a single sweep. */
#define KINGDOM_NODE_MAX_PER_QUOTA 500

/* kingdom_resource_name() is declared in kingdom_internal.h and DEFINED in
 * kingdom_config.c, beside the static_assert that proves the name table covers
 * every KRES_ value. It is used freely below and deliberately not redefined
 * here: a second definition is a link error, and a second table of
 * player-facing names would drift from the first. */

/* ------------------------------------------------------------------ *
 * Resource <-> prototype
 * ------------------------------------------------------------------ *
 * TWO SETS, ruled 2026-09-01: the same four resources, one flavour above
 * ground and one below. Row 0 is the surface, row 1 the Underdark; the column
 * is enum kingdom_resource, so the ORDER is load-bearing. The static_asserts
 * are the guard: reorder the enum or add a resource without adding both
 * prototypes and the build stops, rather than mineral nodes quietly yielding
 * water.
 */
#define KINGDOM_NODE_HALVES 2
#define KINGDOM_NODE_SURFACE 0
#define KINGDOM_NODE_UNDERDARK 1

static const int kingdom_node_prototypes[KINGDOM_NODE_HALVES][KRES_MAX] = {
	{ VOBJ_KINGDOM_NODE_MINERAL, VOBJ_KINGDOM_NODE_WOOD, VOBJ_KINGDOM_NODE_FIBRE,
	  VOBJ_KINGDOM_NODE_WATER },
	{ VOBJ_KINGDOM_NODE_UD_MINERAL, VOBJ_KINGDOM_NODE_UD_WOOD, VOBJ_KINGDOM_NODE_UD_FIBRE,
	  VOBJ_KINGDOM_NODE_UD_WATER }
};

static_assert(KRES_MINERAL == 0 && KRES_WOOD == 1 && KRES_FIBRE == 2 && KRES_WATER == 3,
	      "kingdom_node_prototypes is indexed by kingdom_resource; keep them in step");
static_assert((int)(sizeof(kingdom_node_prototypes[0]) / sizeof(kingdom_node_prototypes[0][0])) ==
		      KRES_MAX,
	      "every kingdom_resource needs a node prototype in both halves of the world");
static_assert((int)(sizeof(kingdom_node_prototypes) / sizeof(kingdom_node_prototypes[0])) ==
		      KINGDOM_NODE_HALVES,
	      "the prototype table has exactly two halves: surface and Underdark");

int kingdom_node_vnum_for(int res, bool underdark)
{
	/* 0, not -1: the header specifies 0 for an unknown resource, and 0 is
	 * not a usable object vnum either way. */
	if (res < 0 || res >= KRES_MAX)
		return 0;

	return kingdom_node_prototypes[underdark ? KINGDOM_NODE_UNDERDARK : KINGDOM_NODE_SURFACE]
				      [res];
}

/* kingdom_node_vnum_for_resource() is GONE: it predated the Underdark set,
 * answered only the surface vnum, and nothing called it. Its declaration in
 * kingdom_internal.h (line 79) should be removed with it -- the header is
 * frozen for this lane, so that removal is asked for rather than made. */

int kingdom_resource_for_node_vnum(int vnum)
{
	for (int half = 0; half < KINGDOM_NODE_HALVES; half++)
		for (int res = 0; res < KRES_MAX; res++)
			if (kingdom_node_prototypes[half][res] == vnum)
				return res;

	return -1;
}

bool kingdom_node_is_underdark(int vnum)
{
	for (int res = 0; res < KRES_MAX; res++)
		if (kingdom_node_prototypes[KINGDOM_NODE_UNDERDARK][res] == vnum)
			return true;

	return false;
}

/* ------------------------------------------------------------------ *
 * The region table
 * ------------------------------------------------------------------ *
 * The vnum ranges are mine_data[]'s own (economy/mining.c), so that nodes and
 * mines cover the same ground and a god who has tuned one has already learned
 * the other's shape. Each region carries a target population PER RESOURCE,
 * each overridable from duris.properties by the key spelled out beside it --
 * the keys are written in full rather than composed at runtime so that a grep
 * of the properties file and a grep of the source find the same string.
 *
 * ON THE UNDERDARK: its counts used to be token -- 12/4/4/4 -- and the comment
 * that stood here justified them by saying an Underdark node could never be
 * banked, because no realm may be sited on an Underdark square. The re-ruling
 * killed that reasoning: harvesting is no longer tied to the ground you stand
 * on, so an Underdark node banks to the digger's realm exactly as a surface
 * one does. The counts below are raised to match, and the Underdark's own
 * thematic prototypes (481-484) are what actually spawn there.
 *
 * The Tharnadia Rift stays small because it IS small: ten thousand vnums
 * against the map's hundred and sixty thousand.
 */
struct kingdom_node_quota {
	const char *prop; /* duris.properties key overriding `count` */
	int count; /* nodes of this resource kept loaded in this region */
};

struct kingdom_node_region {
	const char *name; /* for logs */
	int start_vnum; /* first room vnum of the region, inclusive */
	int end_vnum; /* last room vnum of the region, inclusive */
	bool underdark; /* the whole region is below ground */
	struct kingdom_node_quota quota[KRES_MAX]; /* indexed by kingdom_resource */
};

static const struct kingdom_node_region kingdom_node_regions[] = {
	{ "Surface Map",
	  500000,
	  659999,
	  false,
	  { { "kingdom.nodes.map.mineral", 40 },
	    { "kingdom.nodes.map.wood", 40 },
	    { "kingdom.nodes.map.fibre", 30 },
	    { "kingdom.nodes.map.water", 20 } } },
	{ "Underdark",
	  700000,
	  859999,
	  true,
	  { { "kingdom.nodes.ud.mineral", 30 },
	    { "kingdom.nodes.ud.wood", 20 },
	    { "kingdom.nodes.ud.fibre", 20 },
	    { "kingdom.nodes.ud.water", 15 } } },
	{ "Tharnadia Rift",
	  110000,
	  119999,
	  false,
	  { { "kingdom.nodes.tharnrift.mineral", 4 },
	    { "kingdom.nodes.tharnrift.wood", 2 },
	    { "kingdom.nodes.tharnrift.fibre", 2 },
	    { "kingdom.nodes.tharnrift.water", 2 } } }
};

constexpr int KINGDOM_NODE_REGION_COUNT =
	(int)(sizeof(kingdom_node_regions) / sizeof(kingdom_node_regions[0]));

/* The rnum window of each region, resolved once at initialise.
 *
 * NOT real_room(start)..real_room(end) -- see hazard (b) in the file banner.
 * The world table is sorted by vnum (real_room0() binary-searches it), so the
 * rooms of a vnum range occupy one contiguous run of rnums; this finds that
 * run's ends by inspection, and leaves last < first for a region with no rooms
 * at all, so placement skips it instead of indexing the table with -1. */
struct kingdom_node_window {
	int first;
	int last;
};

static struct kingdom_node_window kingdom_node_windows[KINGDOM_NODE_REGION_COUNT];

/* Bumped by initialise and by shutdown, and carried in every reload event's
 * payload. An event whose generation no longer matches is left over from a
 * previous life of the module -- a copyover, a reload command, a shutdown --
 * and dies without rescheduling. This is what stops a second initialise from
 * running two reload chains per region, and it needs no event-cancellation
 * API. */
static unsigned int kingdom_node_generation = 0;
static bool kingdom_nodes_running = false;

/* ------------------------------------------------------------------ *
 * Room predicates
 * ------------------------------------------------------------------ */

/* rnum 0 is rejected deliberately: real_room0() answers 0 both for the first
 * room and for "no such vnum" (world/db.c), and every caller here means the
 * latter. ch->in_room can also be NOWHERE, which is -1 (core/config.h). Same
 * rule as kingdom_geometry.c. */
static bool kingdom_harvest_valid_rnum(int rnum)
{
	return rnum > 0 && rnum <= top_of_world;
}

/* ------------------------------------------------------------------ *
 * Decay
 * ------------------------------------------------------------------ *
 * value[2] is the MINUTE (not second -- see the banner) at which the node was
 * first worked, and 0 while it is pristine.
 */

static int kingdom_node_now_minutes(void)
{
	return static_cast<int>(time(0) / 60);
}

static int kingdom_node_decay_mins(void)
{
	int mins = get_property("kingdom.nodes.decayMins", KINGDOM_NODE_DECAY_MINS_DEFAULT);

	if (mins < 1)
		mins = 1;
	if (mins > KINGDOM_NODE_DECAY_MINS_MAX)
		mins = KINGDOM_NODE_DECAY_MINS_MAX;

	return mins;
}

/* Both clocks passed in, so a loop over object_list reads the properties table
 * and the system clock once rather than once per object. */
static bool kingdom_node_expired_at(P_obj node, int now_min, int decay_mins)
{
	if (!node)
		return false;

	const int marked = node->value[2];

	/* Pristine: never rots, as ruled. */
	if (marked <= 0)
		return false;

	/* A mark in the future is either a clock that went backwards or a value
	 * an immortal has edited. Refusing to expire is the safe answer: the
	 * node keeps its charges and somebody can work it out by hand. */
	if (now_min < marked)
		return false;

	return (now_min - marked) >= decay_mins;
}

/* Start the decay clock the first time a charge comes out of a node. Called
 * only while the node still has charges left; a node worked to nothing is
 * extracted outright and never needs a clock. */
static void kingdom_node_mark_worked(P_obj node)
{
	if (!node || node->value[2] > 0)
		return;

	const int now_min = kingdom_node_now_minutes();

	/* 0 means pristine, so never store 0 as a real mark. Only reachable in
	 * the first minute of 1970. */
	node->value[2] = now_min > 0 ? now_min : 1;
}

/* The LIVE node object standing in a room, or NULL.
 *
 * An expired node is skipped rather than returned: expiry is lazy, so the
 * object may still be sitting in the room when a player walks in, and every
 * caller here means "is there something to work". The object itself is removed
 * by kingdom_node_reap_room() or by the reload sweep.
 *
 * Placement refuses a room that already holds one, so there is normally at
 * most one; an immortal who loads a second gets the first in the list worked,
 * which is the same answer the engine's own object lookups would give. */
static P_obj kingdom_node_in_room(int rnum)
{
	if (!kingdom_harvest_valid_rnum(rnum))
		return NULL;

	const int now_min = kingdom_node_now_minutes();
	const int decay_mins = kingdom_node_decay_mins();

	for (P_obj obj = world[rnum].contents; obj; obj = obj->next_content)
	{
		if (obj->R_num < 0)
			continue;
		if (kingdom_resource_for_node_vnum(obj_index[obj->R_num].virtual_number) < 0)
			continue;
		if (kingdom_node_expired_at(obj, now_min, decay_mins))
			continue;

		return obj;
	}

	return NULL;
}

/* The room an exit leads to, or -1 when there is no usable exit that way.
 *
 * mine_friendly() dereferences world[dir_option->to_room] without checking the
 * destination at all; a NOWHERE exit there would index world[-1]. The bound
 * check costs nothing and removes the question. */
static int kingdom_neighbour_room(int rnum, int dir)
{
	if (!world[rnum].dir_option[dir])
		return -1;

	const int to_room = world[rnum].dir_option[dir]->to_room;

	return kingdom_harvest_valid_rnum(to_room) ? to_room : -1;
}

/*
 * DOES THIS SORT OF GROUND PRODUCE THIS RESOURCE -- one table, two jobs.
 *
 * It answers BOTH "may a node of this resource be scattered here" (the
 * equivalent of mine_friendly()) and "does a realm holding this square work
 * this resource better" (the kingdom bonus the re-ruling asks for). Deliberately
 * ONE predicate for both: they are the same statement about terrain, and two
 * copies of it would drift until the map a player learns from placement stopped
 * matching the bonus they are paid.
 *
 * Sector only. The neighbour rule lives in kingdom_ground_favours() below.
 */
static bool kingdom_sector_favours(int sector, int res)
{
	switch (res)
	{
	case KRES_MINERAL:
		/* Stone and metal: bare high ground above, and the Underdark's own
		 * raw rock below. SECT_MOUNTAIN is here even though a node may not
		 * STAND on one (kingdom_node_invalid_room refuses it) -- a realm
		 * that holds mountains still mines better, and the neighbour rule
		 * is what puts the node itself on the slope beneath. */
		return sector == SECT_HILLS || sector == SECT_MOUNTAIN || sector == SECT_DESERT ||
		       sector == SECT_ARCTIC || sector == SECT_UNDRWLD_MOUNTAIN ||
		       sector == SECT_UNDRWLD_WILD || sector == SECT_UNDRWLD_LIQMITH;

	case KRES_WOOD:
		/* Timber above; a mushroom forest is the Underdark's stand of
		 * timber, which is what makes prototype 482 a fungal stand. */
		return sector == SECT_FOREST || sector == SECT_SNOWY_FOREST ||
		       sector == SECT_UNDRWLD_MUSHROOM;

	case KRES_FIBRE:
		/* Flax and reeds above; cave silk clings to low ceilings and grows
		 * out of slime below. */
		return sector == SECT_FIELD || sector == SECT_SWAMP ||
		       sector == SECT_UNDRWLD_LOWCEIL || sector == SECT_UNDRWLD_SLIME;

	case KRES_WATER:
		/* Every standing-water sector, plus swamp, which is water the room
		 * is not flagged for. A node may not stand IN water (see
		 * kingdom_node_invalid_room), so on the surface this mostly reaches
		 * a node through the neighbour rule -- the spring is on the bank. */
		return sector == SECT_SWAMP || sector == SECT_WATER_SWIM ||
		       sector == SECT_WATER_NOSWIM || sector == SECT_OCEAN ||
		       sector == SECT_UNDERWATER || sector == SECT_UNDERWATER_GR ||
		       sector == SECT_UNDRWLD_WATER || sector == SECT_UNDRWLD_NOSWIM;

	default:
		return false;
	}
}

/*
 * The square itself, or any of its four compass neighbours.
 *
 * Like mine_friendly(), a square can qualify on what its NEIGHBOURS are made
 * of rather than only on its own sector, which is what puts a water node on a
 * bank, a mineral node on the ground below a mountain, and a wood node at a
 * forest's edge. It is used unchanged for the realm tally, so a realm on the
 * coast draws water better and a realm under the mountains draws mineral
 * better even though it can hold neither sector: no realm square is ever
 * water (kingdom_placement.c's settleable whitelist excludes every water
 * sector), so without the neighbour rule KRES_WATER could never earn a bonus
 * at all.
 */
static bool kingdom_ground_favours(int rnum, int res)
{
	static const int cardinals[4] = { DIR_NORTH, DIR_EAST, DIR_SOUTH, DIR_WEST };

	if (!kingdom_harvest_valid_rnum(rnum))
		return false;

	if (kingdom_sector_favours(world[rnum].sector_type, res))
		return true;

	for (int i = 0; i < 4; i++)
	{
		const int to_room = kingdom_neighbour_room(rnum, cardinals[i]);

		if (to_room >= 0 && kingdom_sector_favours(world[to_room].sector_type, res))
			return true;
	}

	return false;
}

/* Which half of the world a room belongs to, for choosing the prototype set.
 *
 * The region flag is the coarse answer -- the Underdark region is 700000-859999
 * whatever any single room's sector says -- and IS_UNDERWORLD() can only add to
 * it, never take away: an Underdark-sectored pocket inside the Tharnadia Rift
 * gets a fungal stand, which is the thematic answer the ruling asks for. */
static bool kingdom_room_is_underdark(int region, int rnum)
{
	if (region >= 0 && region < KINGDOM_NODE_REGION_COUNT &&
	    kingdom_node_regions[region].underdark)
		return true;

	return kingdom_harvest_valid_rnum(rnum) && IS_UNDERWORLD(rnum);
}

/*
 * MAY A NODE STAND HERE AT ALL -- the equivalent of invalid_mine_room().
 *
 * REIMPLEMENTED RATHER THAN REUSED. invalid_mine_room() and mine_friendly()
 * are defined in economy/mining.c with external linkage but are declared in NO
 * header, so reaching them would mean writing a second copy of their
 * signatures here -- exactly the drift this module's own header forbids. They
 * are also the wrong shape: they encode "where an ORE mine goes", banning
 * every water room, every mountain AND the whole Underdark floor, which is
 * precisely the ground ruling 2 opened up.
 *
 * NO NODE ON LAND A REALM CONTROLS. This is ruling 1's other half, and it is
 * a retry condition exactly like the sector bans: the placement loop rolls
 * again. It is O(1) -- kingdom_owner_of_room() is a hash lookup that short-
 * circuits to 0 when no square is claimed anywhere.
 *
 * WATER ROOMS ARE BANNED FOR EVERY RESOURCE, THE WATER NODE INCLUDED. An
 * object dropped in water that is not ITEM_FLOAT is swept to the Poseidon
 * vault by obj_to_room() itself (world/handler.c) -- the node would simply not
 * be where it was put. The water node therefore stands on the bank, and
 * kingdom_ground_favours() is what puts it there.
 */
static bool kingdom_node_invalid_room(int rnum)
{
	if (!kingdom_harvest_valid_rnum(rnum))
		return true;

	if (IS_ROOM(rnum, ROOM_PRIVATE) || PRIVATE_ZONE(rnum))
		return true;

	/* A room with a down exit is a shaft, a stair or a pit; mining refuses
	 * them and so does this. */
	if (world[rnum].dir_option[DIR_DOWN])
		return true;

	if (IS_WATER_ROOM(rnum))
		return true;

	/* Ruling 1: nodes load anywhere EXCEPT on land a realm controls. */
	if (kingdom_owner_of_room(rnum) != 0)
		return true;

	switch (world[rnum].sector_type)
	{
	/* Built up, indoors, or nothing to stand on. */
	case SECT_INSIDE:
	case SECT_CITY:
	case SECT_ROAD:
	case SECT_CASTLE:
	case SECT_CASTLE_WALL:
	case SECT_CASTLE_GATE:
	case SECT_NO_GROUND:
	case SECT_LAVA:
	/* Sheer rock: mining bans it for the same reason, and a node on it
	 * would be unreachable scenery. */
	case SECT_MOUNTAIN:
	case SECT_UNDRWLD_MOUNTAIN:
	case SECT_UNDRWLD_NOGROUND:
	case SECT_UNDRWLD_INSIDE:
	case SECT_UNDRWLD_CITY:
	/* The outer planes are not the world. */
	case SECT_FIREPLANE:
	case SECT_AIR_PLANE:
	case SECT_EARTH_PLANE:
	case SECT_ETHEREAL:
	case SECT_ASTRAL:
	case SECT_NEG_PLANE:
	case SECT_PLANE_OF_AVERNUS:
		return true;
	default:
		break;
	}

	/* One node to a room, whatever its type -- mining does the same for
	 * mines so that a reload cannot stack them. Asked of the CONTENTS rather
	 * than of kingdom_node_in_room(), because an expired node that has not
	 * been reaped yet is still physically in the room and a second one
	 * dropped on top of it would be indistinguishable to a player. */
	for (P_obj obj = world[rnum].contents; obj; obj = obj->next_content)
		if (obj->R_num >= 0 &&
		    kingdom_resource_for_node_vnum(obj_index[obj->R_num].virtual_number) >= 0)
			return true;

	return false;
}

/* ------------------------------------------------------------------ *
 * Reaping
 * ------------------------------------------------------------------ */

/* True when this object is one of ours AND has no business existing any more:
 * it has outlived its decay window, or it now stands on land a realm controls.
 * `rnum` is the room it is standing in. */
static bool kingdom_node_should_reap(P_obj obj, int rnum, int now_min, int decay_mins)
{
	if (!obj || obj->R_num < 0)
		return false;
	if (kingdom_resource_for_node_vnum(obj_index[obj->R_num].virtual_number) < 0)
		return false;

	if (kingdom_node_expired_at(obj, now_min, decay_mins))
		return true;

	return kingdom_harvest_valid_rnum(rnum) && kingdom_owner_of_room(rnum) != 0;
}

/*
 * Clear one room of dead nodes -- the "checked when touched" half of the lazy
 * expiry.
 *
 * next_content is read BEFORE the extraction, because extract_obj() ends in
 * free_obj() (world/handler.c). Extracting a top-level room object cannot free
 * one of its siblings, so the saved pointer stays good.
 */
static void kingdom_node_reap_room(int rnum)
{
	if (!kingdom_harvest_valid_rnum(rnum))
		return;

	const int now_min = kingdom_node_now_minutes();
	const int decay_mins = kingdom_node_decay_mins();
	P_obj next_obj = NULL;

	for (P_obj obj = world[rnum].contents; obj; obj = next_obj)
	{
		next_obj = obj->next_content;

		if (kingdom_node_should_reap(obj, rnum, now_min, decay_mins))
			extract_obj(obj);
	}
}

/*
 * Clear the whole world of dead nodes -- the "on the reload sweep" half.
 *
 * COLLECT FIRST, EXTRACT SECOND, for the same reason kingdom_harvest_shutdown()
 * does: extract_obj() frees, and object_list->next would be read out of freed
 * memory. Nodes are top-level room objects with no contents, so no extraction
 * in the second loop can free another entry in the vector.
 */
static int kingdom_nodes_reap(void)
{
	const int now_min = kingdom_node_now_minutes();
	const int decay_mins = kingdom_node_decay_mins();
	std::vector<P_obj> doomed;

	for (P_obj obj = object_list; obj; obj = obj->next)
	{
		/* loc.room is a union member that only means a room when LOC_ROOM
		 * is set. A node held by anything other than a room is not one
		 * this sweep can reason about, so it is left alone. */
		if (!IS_SET(obj->loc_p, LOC_ROOM))
			continue;
		if (!kingdom_harvest_valid_rnum(obj->loc.room))
			continue;

		if (kingdom_node_should_reap(obj, obj->loc.room, now_min, decay_mins))
			doomed.push_back(obj);
	}

	for (std::size_t i = 0; i < doomed.size(); i++)
		extract_obj(doomed[i]);

	return static_cast<int>(doomed.size());
}

/* ------------------------------------------------------------------ *
 * Spawning
 * ------------------------------------------------------------------ */

/* Resolve every region's rnum window. Called once from initialise; the world
 * table does not change afterwards. */
static void kingdom_node_resolve_windows(void)
{
	for (int region = 0; region < KINGDOM_NODE_REGION_COUNT; region++)
	{
		kingdom_node_windows[region].first = 1;
		kingdom_node_windows[region].last = 0; /* empty until proven otherwise */
	}

	for (int rnum = 1; rnum <= top_of_world; rnum++)
	{
		const int vnum = world[rnum].number;

		for (int region = 0; region < KINGDOM_NODE_REGION_COUNT; region++)
		{
			if (vnum < kingdom_node_regions[region].start_vnum ||
			    vnum > kingdom_node_regions[region].end_vnum)
				continue;

			if (kingdom_node_windows[region].last < kingdom_node_windows[region].first)
				kingdom_node_windows[region].first = rnum;

			kingdom_node_windows[region].last = rnum;
		}
	}

	for (int region = 0; region < KINGDOM_NODE_REGION_COUNT; region++)
		if (kingdom_node_windows[region].last < kingdom_node_windows[region].first)
			logit(LOG_KINGDOM,
			      "nodes: region %s (%d-%d) has no rooms; nothing will spawn there.",
			      kingdom_node_regions[region].name,
			      kingdom_node_regions[region].start_vnum,
			      kingdom_node_regions[region].end_vnum);
}

/* ------------------------------------------------------------------ *
 * Richness descriptions
 * ------------------------------------------------------------------ *
 * THEMATIC PER RESOURCE AND PER WORLD HALF: the strung description is the one
 * place a player actually reads a node, so a rich fungal stand must read as
 * fungus and a rich timber stand as timber -- ruling 2's "thematic" applies
 * to the words, not only to which prototype is loaded. Indexed
 * [half][resource][richness 0 poor .. 3 mother lode]; the half and resource
 * axes are kingdom_node_prototypes' own, and the static_assert beside that
 * table already pins the KRES_ order this one relies on.
 *
 * BECAUSE EVERY SPAWNED NODE IS STRUNG WITH ONE OF THESE, the description
 * lines on prototypes 477-484 in heavens.obj are dead in normal play. That
 * data stays as the FALLBACK: kingdom_node_roll() leaves the prototype's own
 * description standing on any object it cannot recognise as a kingdom node.
 */
#define KINGDOM_NODE_RICHNESS_TIERS 4

static const char *const kingdom_node_richness_descs[KINGDOM_NODE_HALVES][KRES_MAX]
						    [KINGDOM_NODE_RICHNESS_TIERS] = {
	{ /* surface */
	  { /* a seam of stone */
	    "&+LThe seam here is thin and broken, and the stone will not give up much.&n",
	    "&+LThe seam here is workable stone, if unremarkable.&n",
	    "&+LThe seam here runs &+ythick and generous&+L, and is plainly worth the work.&n",
	    "&+LThis seam is a &+Ymother lode&+L - as rich as stone ever runs.&n" },
	  { /* a stand of timber */
	    "&+LThe timber here is sparse and knotted, and will not yield much.&n",
	    "&+LThe timber here is sound enough, if unremarkable.&n",
	    "&+LThe timber here stands &+ytall and generous&+L, and is plainly worth the felling.&n",
	    "&+LThis stand is a &+Yforester's mother lode&+L - as fine as timber ever grows.&n" },
	  { /* flax and reeds */
	    "&+LThe flax here is thin and straggling, and will not yield much.&n",
	    "&+LThe flax and reeds here are workable, if unremarkable.&n",
	    "&+LThe flax and reeds here grow &+ythick and generous&+L, and are plainly worth the pulling.&n",
	    "&+LThis field is a &+Ymother lode&+L of flax, standing as heavy as fibre ever grows.&n" },
	  { /* a clean spring */
	    "&+LThe spring here is little more than a seep, and will not yield much.&n",
	    "&+LThe spring here flows steadily enough, if unremarkably.&n",
	    "&+LThe spring here rises &+yclear and generous&+L, and is plainly worth the drawing.&n",
	    "&+LThis spring is a &+Ymother lode&+L of sweet water, as fine as any that rises.&n" } },
	{ /* Underdark */
	  { /* an ore seam */
	    "&+LThe ore here is a thin vein, and the deep rock will not give up much.&n",
	    "&+LThe ore here is workable, if unremarkable.&n",
	    "&+LThe ore here runs &+ythick and generous&+L through the rock, and is plainly worth the work.&n",
	    "&+LThis seam is a &+Ymother lode&+L - as rich as the deep rock ever bears.&n" },
	  { /* a fungal stand */
	    "&+LThe fungus here is stunted and woody, and will not yield much.&n",
	    "&+LThe fungal stalks here are sound enough, if unremarkable.&n",
	    "&+LThe fungal stalks here rise &+ythick and generous&+L, and are plainly worth the felling.&n",
	    "&+LThis stand is a &+Ymother lode&+L of great fungal boles, as rich as the caverns ever grow.&n" },
	  { /* cave silk */
	    "&+LThe cave silk here hangs in thin wisps, and will not yield much.&n",
	    "&+LThe cave silk here is workable, if unremarkable.&n",
	    "&+LThe cave silk here hangs &+ythick and generous&+L, and is plainly worth the gathering.&n",
	    "&+LThe cave silk here hangs in &+Ycurtains&+L - a mother lode, as rich as any gallery ever spun.&n" },
	  { /* a dark pool */
	    "&+LThe dark pool here is scarcely a puddle, and will not yield much.&n",
	    "&+LThe dark pool here holds water enough, if unremarkable.&n",
	    "&+LThe dark pool here lies &+ydeep and generous&+L, and is plainly worth the drawing.&n",
	    "&+LThis dark pool is a &+Ymother lode&+L of pure water, as fine as the deep ever holds.&n" } }
};

/* Roll a fresh node's richness into value[1] and its charges into value[0],
 * and put its decay clock (value[2]) back to pristine.
 *
 * The distribution and the charge ranges are mining's, verbatim: 3% mother
 * lode, 17% rich, 55% ordinary, 25% poor, with 24-32, 16-24, 12-20 and 8-16
 * draws respectively. Copying the numbers rather than inventing new ones keeps
 * the two gathering systems reading the same way to a player who has mined
 * before.
 *
 * The description is strung over the prototype's so the ground READS as rich
 * or thin before anybody swings at it, and STRUNG_DESC1 is set with it so that
 * free_obj() frees the copy. mining.c omits that bit and leaks a description
 * per mine; this does not. WHICH text is strung is answered by the node's OWN
 * vnum -- kingdom_resource_for_node_vnum() for the resource and
 * kingdom_node_is_underdark() for the world half -- so the wording matches the
 * prototype wherever the object came from, an immortal's load included. An
 * object this cannot recognise keeps its prototype description: that is the
 * heavens.obj fallback. */
static void kingdom_node_roll(P_obj node)
{
	const int roll = number(0, 99);
	int richness;

	if (roll < 3)
	{
		node->value[0] = number(24, 32);
		richness = 3;
	}
	else if (roll < 20)
	{
		node->value[0] = number(16, 24);
		richness = 2;
	}
	else if (roll < 75)
	{
		node->value[0] = number(12, 20);
		richness = 1;
	}
	else
	{
		node->value[0] = number(8, 16);
		richness = 0;
	}

	node->value[1] = richness;

	/* Pristine: the decay clock does not start until somebody takes a charge
	 * out of it. Set rather than assumed, because it comes off a prototype an
	 * immortal can edit. */
	node->value[2] = 0;

	const int vnum = node->R_num >= 0 ? obj_index[node->R_num].virtual_number : -1;
	const int res = kingdom_resource_for_node_vnum(vnum);

	if (res < 0)
		return;

	const int half = kingdom_node_is_underdark(vnum) ? KINGDOM_NODE_UNDERDARK :
							   KINGDOM_NODE_SURFACE;

	/* Free a previous strung copy before replacing it. Unreachable while a
	 * node is only ever rolled once, straight out of read_object(), which
	 * memsets the object; it is here so that re-rolling in place stays a
	 * safe edit rather than becoming a leak. */
	if (IS_SET(node->str_mask, STRUNG_DESC1) && node->description)
		str_free(node->description);

	node->description = str_dup(kingdom_node_richness_descs[half][res][richness]);
	SET_BIT(node->str_mask, STRUNG_DESC1);
}

/* One line for the player describing how good the ground is. */
static const char *kingdom_node_richness_text(int richness)
{
	switch (richness)
	{
	case 3:
		return "&+GThe pickings here are extraordinarily rich.&n";
	case 2:
		return "&+yThe pickings here are generous.&n";
	case 1:
		return "&+LThe pickings here are workable.&n";
	default:
		return "&+LThe pickings here are thin.&n";
	}
}

/*
 * Place ONE node of `res` somewhere in `region`. Modelled on load_one_mine().
 *
 * THE ROOM IS CHOSEN BEFORE THE OBJECT IS READ, which is the one structural
 * departure from load_one_mine(): the prototype depends on which half of the
 * world the room turns out to be in, so there is nothing to read until the
 * room is known. It also means a failed placement no longer has to
 * extract_obj() an object it never used.
 *
 * The whole of the object's state is set BEFORE obj_to_room(), and the room
 * vnum wanted for the log line is copied out before it too, because
 * obj_to_room() can free the object -- see the file banner.
 */
static bool kingdom_load_one_node(int region, int res)
{
	if (region < 0 || region >= KINGDOM_NODE_REGION_COUNT || res < 0 || res >= KRES_MAX)
		return false;

	const int first = kingdom_node_windows[region].first;
	const int last = kingdom_node_windows[region].last;

	/* `first < 1` covers the array's own zero-initialised state as well as an
	 * empty region, so a call that somehow arrived before
	 * kingdom_node_resolve_windows() returns at once rather than spending its
	 * whole 10000-try budget rejecting rnum 0. */
	if (first < 1 || last < first)
		return false;

	int to_room = -1;
	int tries = 0;

	do
	{
		to_room = number(first, last);

		/* number(0, 99): a true 15 in 100. mining.c rolls number(0, 100)
		 * for its 15, which is 15/101 -- the value is copied, the
		 * off-by-one is not. */
		if (!kingdom_node_invalid_room(to_room) &&
		    (kingdom_ground_favours(to_room, res) ||
		     number(0, 99) < KINGDOM_NODE_WILD_PLACEMENT_PCT))
			break;

		tries++;
	} while (tries < KINGDOM_NODE_PLACEMENT_TRIES);

	if (tries >= KINGDOM_NODE_PLACEMENT_TRIES || kingdom_node_invalid_room(to_room))
		return false;

	const bool underdark = kingdom_room_is_underdark(region, to_room);
	const int vnum = kingdom_node_vnum_for(res, underdark);

	/* read_object() itself refuses a vnum that is not in the database and
	 * answers NULL (world/db.c), so a build whose world data is missing a
	 * prototype costs a log line rather than anything worse. */
	P_obj node = read_object(vnum, VIRTUAL);
	if (!node)
	{
		logit(LOG_KINGDOM, "nodes: prototype %d (%s%s) is not in the object database.",
		      vnum, underdark ? "underdark " : "", kingdom_resource_name(res));
		return false;
	}

	kingdom_node_roll(node);

	/* Everything read from the object or the room must be read NOW: after
	 * obj_to_room() the object may not exist. */
	const int richness = node->value[1];
	const int charges = node->value[0];
	const int room_vnum = world[to_room].number;

	obj_to_room(node, to_room);
	node = NULL; /* deliberately unusable from here down */

	wizlog(56, "Kingdom %s%s node (richness %d, %d draws) loaded in room %d",
	       underdark ? "underdark " : "", kingdom_resource_name(res), richness, charges,
	       room_vnum);

	return true;
}

/* How many live nodes of each resource are standing in `region`.
 *
 * ONE walk of object_list for all four resources and both prototype sets,
 * where the previous build walked it once per resource. Reads no object it has
 * not first proved is in a room, because loc.room is a union member that only
 * means a room when LOC_ROOM is set. Expired nodes have already been extracted
 * by kingdom_nodes_reap() before this runs, so nothing here has to second-guess
 * them. */
static void kingdom_node_census(int region, int *counts)
{
	const struct kingdom_node_region *cfg = &kingdom_node_regions[region];

	for (int res = 0; res < KRES_MAX; res++)
		counts[res] = 0;

	for (P_obj obj = object_list; obj; obj = obj->next)
	{
		if (obj->R_num < 0)
			continue;

		const int res =
			kingdom_resource_for_node_vnum(obj_index[obj->R_num].virtual_number);
		if (res < 0)
			continue;

		if (!IS_SET(obj->loc_p, LOC_ROOM))
			continue;
		if (!kingdom_harvest_valid_rnum(obj->loc.room))
			continue;

		const int room_vnum = world[obj->loc.room].number;

		if (room_vnum >= cfg->start_vnum && room_vnum <= cfg->end_vnum)
			counts[res]++;
	}
}

/* The target population of `res` in `region`, from duris.properties. */
static int kingdom_node_target_count(int region, int res)
{
	const struct kingdom_node_quota *quota = &kingdom_node_regions[region].quota[res];
	int wanted = get_property(quota->prop, quota->count);

	if (wanted < 0)
		wanted = 0;
	if (wanted > KINGDOM_NODE_MAX_PER_QUOTA)
		wanted = KINGDOM_NODE_MAX_PER_QUOTA;

	return wanted;
}

struct kingdom_node_reload_data {
	int region;
	unsigned int generation;
};

static void kingdom_node_reload_event(P_char ch, P_char victim, P_obj obj, void *data);

/*
 * Schedule `region`'s next reload sweep, reloadMins from now. Called at the
 * tail of every sweep, and by initialise for the FIRST sweep of each region --
 * initialise deliberately schedules rather than runs it; see
 * kingdom_harvest_initialize().
 */
static void kingdom_node_schedule_sweep(int region)
{
	if (region < 0 || region >= KINGDOM_NODE_REGION_COUNT)
		return;

	/* Clamped at both ends. The delay below is in PULSES -- WAIT_SEC * 60 *
	 * mins -- so a fat-fingered properties line of six or seven digits would
	 * overflow a signed int and hand add_event() a negative delay, which it
	 * refuses (nevent_schedule_status::negative_delay) and the region's
	 * reload chain would end there. A day is longer than any sane setting. */
	int mins = get_property("kingdom.nodes.reloadMins", KINGDOM_NODE_RELOAD_MINS_DEFAULT);
	if (mins < 1)
		mins = 1;
	if (mins > 1440)
		mins = 1440;

	struct kingdom_node_reload_data payload = { region, kingdom_node_generation };

	/* Branch on the RETURN. add_event() answers a nevent_schedule_result
	 * whose explicit operator bool is true only for `scheduled`
	 * (core/structs.h); an unchecked refusal would silently end this
	 * region's reload chain for the rest of the boot. */
	if (!add_event(kingdom_node_reload_event, (WAIT_SEC * 60) * mins, NULL, NULL, NULL, 0,
		       &payload, (int)sizeof(payload)))
		logit(LOG_KINGDOM, "nodes: could not schedule the reload sweep for region %s.",
		      kingdom_node_regions[region].name);
}

/*
 * Reap the dead, bring one region back up to strength, and schedule the next
 * sweep.
 *
 * REPLENISHMENT IS DEFICIT-BASED: every sweep spawns up to the measured
 * shortfall of each resource, bounded by the region quota --
 * kingdom_node_target_count() clamps the target itself, and the census is
 * what is subtracted from it. The first build copied load_mines() and dripped
 * ONE node per resource per sweep after boot; that pace suits mining, whose
 * mines only deplete where its narrowly-licensed verb works them, but ruling
 * 1 lets ANYONE work a node the moment it lands, so consumption can outrun a
 * fixed drip indefinitely and the shortfall itself is what a sweep has to
 * replace.
 */
static void kingdom_nodes_reload(int region)
{
	if (region < 0 || region >= KINGDOM_NODE_REGION_COUNT)
		return;

	/* World-wide, not region-scoped, and before the census: a node that has
	 * rotted or been enclosed by a realm must stop counting against the quota
	 * in the same pass that refills it, or the region never recovers the
	 * slot. */
	kingdom_nodes_reap();

	int counts[KRES_MAX];

	kingdom_node_census(region, counts);

	for (int res = 0; res < KRES_MAX; res++)
	{
		const int wanted = kingdom_node_target_count(region, res);

		/* A failed placement ends this resource's refill for the pass:
		 * whatever refused it -- an exhausted 10000-try budget, a
		 * missing prototype -- would only bill the same cost again for
		 * every node still short. The next sweep starts fresh. */
		for (int have = counts[res]; have < wanted; have++)
			if (!kingdom_load_one_node(region, res))
				break;
	}

	kingdom_node_schedule_sweep(region);
}

static void kingdom_node_reload_event(P_char /*ch*/, P_char /*victim*/, P_obj, void *data)
{
	if (!data)
		return;

	const struct kingdom_node_reload_data work =
		*(const struct kingdom_node_reload_data *)data;

	/* A sweep left over from a previous initialise, or from before a
	 * shutdown. Dying without rescheduling is what keeps exactly one chain
	 * per region alive across a copyover or a reload command. */
	if (!kingdom_nodes_running || work.generation != kingdom_node_generation)
		return;

	kingdom_nodes_reload(work.region);
}

/* ------------------------------------------------------------------ *
 * The spec proc
 * ------------------------------------------------------------------ *
 * Bound to all EIGHT prototypes by kingdom_harvest_initialize(), the way
 * initialize_mining() binds `mine`.
 *
 * It exists for ONE job: extracting a node whose charges have run out. The
 * harvest verb already extracts a node it empties itself, so this is the net
 * beneath an immortal-loaded node, a node whose prototype ships with value[0]
 * of 0, or any future path that spends a charge without finishing the job.
 *
 * IT DELIBERATELY DOES NOT CHECK DECAY. The ruling asks for a lazy expiry
 * rather than a per-node timer, and this proc is exactly a per-node timer --
 * putting the check here would quietly reintroduce the thing that was ruled
 * out. Expiry is handled by kingdom_node_reap_room() when a player touches the
 * room and by kingdom_nodes_reap() on the sweep.
 *
 * SELF-EXTRACTION IS SAFE HERE, and it is worth saying why, because it is not
 * safe everywhere. The periodic driver is event_object_proc() (world/db.c),
 * which re-reads current_nevent after calling the proc and stops if the object
 * has gone -- extract_obj() detaches an object's events before free_obj(). The
 * OTHER driver, item_procs() (specs/specs.assign.c), walks object_list and
 * would read i->next out of freed memory; it is dead code with no callers
 * anywhere in the tree.
 */
static int kingdom_node_proc(P_obj obj, P_char /*ch*/, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd == CMD_PERIODIC && obj && obj->value[0] <= 0)
	{
		/* FALSE (the default), not mining's TRUE: gone_for_good exists to
		 * purge artifact rows (core/prototypes.h) and a node is not an
		 * artifact, so TRUE would be a claim about this object that is
		 * not true. */
		extract_obj(obj);
		return TRUE;
	}

	return FALSE;
}

/* ------------------------------------------------------------------ *
 * The kingdom bonus: what a realm's land is made of
 * ------------------------------------------------------------------ */

/* Cached per association id, and STAMPED with everything that could change the
 * answer. Terrain is static world data, so the tally depends only on which
 * squares the realm holds -- that is (hall_rnum, highest_claim) -- and
 * realm_id is carried alongside because association ids are reused
 * (found_asc() hands out the lowest free one) and a recycled id must not
 * inherit the previous guild's tally.
 *
 * A stamp mismatch recomputes in place, so there is no invalidation hook that
 * can be forgotten. kingdom_harvest_prune()/_release() drop entries as well,
 * for the day something starts calling them. */
struct kingdom_terrain_tally {
	int realm_id;
	int hall_rnum;
	int highest_claim;
	int squares[KRES_MAX];
};

static std::unordered_map<int, kingdom_terrain_tally> kingdom_terrain_cache;

/* How many of the realm's own squares favour `res`. */
static int kingdom_terrain_squares(const kingdom_realm &realm, int res)
{
	if (res < 0 || res >= KRES_MAX)
		return 0;

	/* operator[] value-initialises a fresh entry, so an unseen realm starts
	 * as an all-zero stamp. That can only collide with a real realm whose
	 * realm_id, hall_rnum and highest_claim are all 0 -- and a realm holding
	 * no squares has an all-zero tally anyway, so the collision is the right
	 * answer rather than a stale one. */
	kingdom_terrain_tally &tally = kingdom_terrain_cache[realm.assoc_id];

	if (tally.realm_id == realm.realm_id && tally.hall_rnum == realm.hall_rnum &&
	    tally.highest_claim == realm.highest_claim)
		return tally.squares[res];

	tally.realm_id = realm.realm_id;
	tally.hall_rnum = realm.hall_rnum;
	tally.highest_claim = realm.highest_claim;

	for (int i = 0; i < KRES_MAX; i++)
		tally.squares[i] = 0;

	/* highest_claim is loaded from persistence, so clamp rather than trust. */
	int claims = realm.highest_claim;
	if (claims < 0)
		claims = 0;
	if (claims > KINGDOM_MAX_SQUARES)
		claims = KINGDOM_MAX_SQUARES;

	for (int claim_index = 1; claim_index <= claims; claim_index++)
	{
		/* 0 for a square that falls off the grid or has no room behind it
		 * (kingdom_geometry.h), which is also real_room0's "not found",
		 * hence the >0 test inside kingdom_ground_favours(). */
		const int rnum = kingdom_room_for_claim(realm.hall_rnum, claim_index);

		if (!kingdom_harvest_valid_rnum(rnum))
			continue;

		/* A square may favour more than one resource -- a swamp is fibre
		 * AND water, a field on a river bank likewise -- and it counts for
		 * each. Terrain variety is meant to be worth something. */
		for (int i = 0; i < KRES_MAX; i++)
			if (kingdom_ground_favours(rnum, i))
				tally.squares[i]++;
	}

	return tally.squares[res];
}

/* ------------------------------------------------------------------ *
 * Dormancy, yield and deposit
 * ------------------------------------------------------------------ */

/* Rung 2 of the arrears ladder. Anything at or past KARR_NODES_DORMANT is
 * dormant, so the ring-reverting rung above it keeps the nodes idle too. */
bool kingdom_nodes_dormant(const kingdom_realm &realm)
{
	return realm.arrears >= KARR_NODES_DORMANT;
}

/*
 * YIELD PER DRAW = (1 + richness + bonus) + random(0 .. 1 + richness + bonus)
 *
 *     richness = 0 poor .. 3 mother lode
 *     bonus    = (owned squares FAVOURING THIS RESOURCE) / 20
 *
 *   favouring squares    bonus    poor node    mother lode
 *          0 - 19          0        1 -  2       4 -  8
 *         20 - 39          1        2 -  4       5 - 10
 *         40 - 59          2        3 -  6       6 - 12
 *         60 - 79          3        4 -  8       7 - 14
 *             80           4        5 - 10       8 - 16
 *
 * THE TYPE OF LAND THE REALM HOLDS IS THE SCALING INPUT, as the re-ruling
 * requires. A realm whose eighty squares are all forest works wood at roughly
 * five times the rate of a realm with none, and works stone no better than a
 * landless digger does. Territory still scales the total -- more squares means
 * more of them favour something -- which is what keeps this a counterweight to
 * upkeep, which scales with the same number (kingdom_upkeep_due): growth only
 * pays for itself while the coin keeps arriving, and a realm that stops paying
 * loses the nodes at rung 2 before it loses any land at rung 3.
 *
 * A landless harvester banks nothing at all, so this is never called for one.
 */
static long kingdom_harvest_yield(const kingdom_realm &realm, int res, int richness)
{
	/* value[1] came off an object an immortal can edit. */
	if (richness < 0)
		richness = 0;
	if (richness > 3)
		richness = 3;

	const int bonus = kingdom_terrain_squares(realm, res) / KINGDOM_HARVEST_SQUARES_PER_STEP;
	const int step = 1 + richness + bonus;

	return (long)step + (long)number(0, step);
}

/* Add to a realm's store, clamped at KINGDOM_RESOURCE_CAP. Returns what was
 * actually banked, which is 0 when the store is already full.
 *
 * This is the ONLY function in the module that moves a resource, and it only
 * ever moves it inward. There is no matching withdraw and there must never be
 * one: the ruling is that these counters are spendable on kingdom benefits and
 * on nothing else. */
long kingdom_resource_deposit(kingdom_realm &realm, int res, long amount)
{
	if (res < 0 || res >= KRES_MAX || amount <= 0)
		return 0;

	const long headroom = KINGDOM_RESOURCE_CAP - realm.resources[res];
	if (headroom <= 0)
		return 0;

	const long banked = amount < headroom ? amount : headroom;

	realm.resources[res] += banked;
	realm.dirty = true;

	return banked;
}

/* kingdom_resource_for_room() from the pre-ruling design is GONE: the
 * resource is the node object's own type now, so every caller asks the node. */

/* ------------------------------------------------------------------ *
 * Lifecycle
 * ------------------------------------------------------------------ */

/*
 * Bind the spec procs and SCHEDULE the first sweep. Called from
 * kingdom_initialize(), after the square index is built -- placement refuses
 * land a realm controls, so the index has to be there first.
 *
 * NOTHING IS SEEDED SYNCHRONOUSLY HERE, and that is load-bearing on a
 * copyover boot. kingdom_initialize() runs from run_the_game() during boot,
 * and game_loop()'s copyover_recover() -- which puts the previous life's node
 * objects back in their rooms -- runs AFTER it (net/comm.c). Seeding every
 * region to quota here and then restoring the old nodes on top doubled every
 * population, and nothing culls a surplus. The sweep is deficit-based, so the
 * first SCHEDULED sweep -- which fires from the game loop, after any restore
 * -- tops up only the shortfall; on a cold boot it seeds to quota one reload
 * interval after boot.
 */
void kingdom_harvest_initialize(void)
{
	/* End any previous life's reload chain before starting a new one; see
	 * kingdom_node_generation. */
	kingdom_node_generation++;
	kingdom_nodes_running = false;
	kingdom_terrain_cache.clear();

	if (!kingdom_enabled())
	{
		logit(LOG_KINGDOM, "nodes: kingdoms are disabled; no harvest nodes placed.");
		return;
	}

	/*
	 * real_object(), NOT real_object0(). real_object0() answers 0 for a
	 * vnum that is not in the database as well as for the first object in
	 * the table (world/db.c), so the mining idiom would install this proc
	 * on obj_index[0] -- an unrelated object -- on any build whose .obj
	 * data does not carry all eight prototypes.
	 */
	for (int half = 0; half < KINGDOM_NODE_HALVES; half++)
	{
		for (int res = 0; res < KRES_MAX; res++)
		{
			/* Through the exported helper, not the table it wraps:
			 * kingdom_node_vnum_for() is the one authority on
			 * resource -> prototype, and this loop going through it
			 * is what keeps every path in the module answering the
			 * same vnum. */
			const int vnum =
				kingdom_node_vnum_for(res, half == KINGDOM_NODE_UNDERDARK);
			const int r_num = real_object(vnum);

			if (r_num < 0)
			{
				logit(LOG_KINGDOM,
				      "nodes: prototype %d (%s%s) missing from the object "
				      "database; that resource will not spawn %s.",
				      vnum, half == KINGDOM_NODE_UNDERDARK ? "underdark " : "",
				      kingdom_resource_name(res),
				      half == KINGDOM_NODE_UNDERDARK ? "in the Underdark"
								     : "on the surface");
				continue;
			}

			obj_index[r_num].func.obj = kingdom_node_proc;
		}
	}

	kingdom_node_resolve_windows();

	kingdom_nodes_running = true;

	for (int region = 0; region < KINGDOM_NODE_REGION_COUNT; region++)
		kingdom_node_schedule_sweep(region);
}

/*
 * A realm no longer holds some of its squares.
 *
 * Nothing on the ground to clean up -- nodes are world objects that belong to
 * nobody and carry no association id -- but the realm's terrain tally is now
 * one ring out of date, so it is dropped. The stamp inside
 * kingdom_terrain_squares() would catch this on its own; dropping the entry
 * here is the cheaper of the two and keeps the cache from holding a tally for
 * a shape the realm no longer has.
 *
 * Called from kingdom_on_guildhall_changed() in kingdom.c.
 */
void kingdom_harvest_prune(const kingdom_realm &realm)
{
	kingdom_terrain_cache.erase(realm.assoc_id);
}

/*
 * A realm was dissolved.
 *
 * Association ids are reused -- found_asc() hands out the lowest free one -- so
 * the tally cached against this id is dropped before the next guild can take
 * it. The stamp is a second line of defence rather than the only one.
 *
 * Called from kingdom_on_guild_deleted() in kingdom.c. The stamp check makes
 * the module correct even without that call: a recycled id whose realm_id,
 * hall and claim count all matched the dead realm's would have the same
 * eighty squares and therefore the same tally.
 */
void kingdom_harvest_release(int assoc_id)
{
	kingdom_terrain_cache.erase(assoc_id);
}

/*
 * Take every node standing in a room back out of the world.
 *
 * COLLECT FIRST, EXTRACT SECOND. extract_obj() ends in free_obj()
 * (world/handler.c), so extracting while walking object_list would read
 * obj->next out of freed memory. Nodes are top-level room objects with no
 * contents, so no extraction in the second loop can free another entry in the
 * vector -- and the LOC_ROOM test below, the same one kingdom_nodes_reap()
 * applies, is what makes that a checked fact rather than an assumption: a
 * node an immortal has picked up or boxed is not a top-level room object, and
 * it is left with its holder here exactly as the reap leaves it.
 */
void kingdom_harvest_shutdown(void)
{
	std::vector<P_obj> doomed;

	for (P_obj obj = object_list; obj; obj = obj->next)
	{
		if (obj->R_num < 0)
			continue;
		if (kingdom_resource_for_node_vnum(obj_index[obj->R_num].virtual_number) < 0)
			continue;
		if (!IS_SET(obj->loc_p, LOC_ROOM))
			continue;

		doomed.push_back(obj);
	}

	for (std::size_t i = 0; i < doomed.size(); i++)
		extract_obj(doomed[i]);

	kingdom_terrain_cache.clear();

	/* Retire the reload chain: any sweep still queued sees the bumped
	 * generation and the cleared flag, and dies without rescheduling. */
	kingdom_node_generation++;
	kingdom_nodes_running = false;
}

/* ------------------------------------------------------------------ *
 * The harvest action
 * ------------------------------------------------------------------ */

/* Carried across the ticks of one harvest. Trivially copyable, and it holds NO
 * POINTERS and NO ASSOCIATION ID: a kingdom_realm* cannot survive a tick
 * because the realm can be erased from kingdom_realms in between
 * (kingdom_on_guild_deleted), and an id captured at the start would be a claim
 * about the digger's membership that four seconds can falsify. The realm is
 * re-derived from the character on every tick, and the node is re-found by
 * number. */
struct kingdom_harvest_work {
	int room_vnum;
	int ticks;
};

static void kingdom_harvest_tick(P_char ch, P_char victim, P_obj obj, void *data);

/*
 * The realm this character digs for, or NULL when they dig for nobody.
 *
 * NOT kingdom_owner_of_room(): the re-ruling moved the credit from the ground
 * to the digger. Membership is the engine's own three-part test, the one
 * IS_ASSOC_MEMBER spells out (guild/assocs.h) and kingdom_char_owns_room()
 * already uses -- GET_ASSOC() alone is not membership, because Guild::apply()
 * points an applicant's assoc pointer at the guild it is applying to and a
 * banned character keeps the pointer too. An applicant, a banned character and
 * someone on parole all dig for nobody.
 */
static kingdom_realm *kingdom_realm_of_char(P_char ch)
{
	if (!ch || IS_NPC(ch) || !GET_ASSOC(ch))
		return NULL;

	if (!IS_MEMBER(GET_A_BITS(ch)) || !GT_PAROLE(GET_A_BITS(ch)))
		return NULL;

	const unsigned int id = GET_ASSOC(ch)->get_id();

	/* get_id() is unsigned; kingdom_find_realm() takes an int and does its
	 * own 0 < id < MAX_ASC range check, so all this has to do is refuse an
	 * id that cannot be represented as one. */
	if (id > static_cast<unsigned int>(INT_MAX))
		return NULL;

	return kingdom_find_realm(static_cast<int>(id));
}

/* The per-tick body. Everything it needs is re-derived from the character and
 * the room, never carried as a pointer, because a realm can be dissolved, a
 * node extracted and a character thrown out of a guild between two ticks four
 * seconds apart. */
static void kingdom_harvest_tick(P_char ch, P_char /*victim*/, P_obj, void *data)
{
	if (!data)
		return;

	/* Copy the payload before anything else. The buffer behind `data`
	 * belongs to the event that is firing and is retired when this returns,
	 * so the rescheduling call at the bottom hands add_event a live local to
	 * copy rather than the buffer being torn down. */
	const struct kingdom_harvest_work work = *(const struct kingdom_harvest_work *)data;

	/* SanityCheck() can itself char_from_room()/char_to_room() a character
	 * it finds in NOWHERE -- but it returns FALSE on that path, so returning
	 * here is the only safe thing to do with the answer. Never test ch's
	 * room after it and carry on. */
	if (!SanityCheck(ch, "kingdom_harvest_tick"))
		return;

	if (!kingdom_enabled())
		return;

	if (!ch->desc || IS_FIGHTING(ch) || !IS_AWAKE(ch) ||
	    IS_STUNNED(ch) || IS_CASTING(ch) || !MIN_POS(ch, POS_STANDING + STAT_NORMAL))
	{
		send_to_char("You stop working the land.\r\n", ch);
		return;
	}

	const int rnum = ch->in_room;
	if (!kingdom_harvest_valid_rnum(rnum) || world[rnum].number != work.room_vnum)
	{
		send_to_char("You have wandered off the ground you were working.\r\n", ch);
		return;
	}

	/* Re-found, never carried: the node may have been emptied by somebody
	 * else, extracted by its own periodic proc, or have rotted out from under
	 * this harvest since the last tick. */
	P_obj node = kingdom_node_in_room(rnum);
	if (!node || node->R_num < 0)
	{
		send_to_char("There is nothing left here to work.\r\n", ch);
		return;
	}

	if (node->value[0] <= 0)
	{
		send_to_char("&+LThis node is worked out.&n\r\n", ch);
		return;
	}

	if (GET_VITALITY(ch) < KINGDOM_HARVEST_MIN_VITALITY)
	{
		send_to_char("You are far too exhausted to keep working.\r\n", ch);
		return;
	}

	GET_VITALITY(ch) -= KINGDOM_HARVEST_VITALITY_COST;

	struct kingdom_harvest_work next = work;
	if (--next.ticks > 0)
	{
		/* Branch on the RETURN VALUE. add_event() can refuse -- a dead
		 * owner, an exhausted sequence (core/structs.h) -- and an
		 * unchecked refusal here would leave the player standing in a
		 * half-finished harvest with no event to finish it and no word of
		 * why. */
		if (!add_event(kingdom_harvest_tick, PULSE_VIOLENCE, ch, NULL, NULL, 0, &next,
			       (int)sizeof(next)))
		{
			send_to_char("Your concentration breaks and the work stops.\r\n", ch);
			return;
		}

		send_to_char("You keep working the land...\r\n", ch);
		return;
	}

	const int res = kingdom_resource_for_node_vnum(obj_index[node->R_num].virtual_number);
	if (res < 0)
	{
		/* kingdom_node_in_room() only ever returns kingdom nodes, so this
		 * is unreachable; it is here because the alternative to checking
		 * is indexing the resource tables with -1. */
		send_to_char("There is nothing here you know how to work.\r\n", ch);
		return;
	}

	/*
	 * Asked NOW, not at the start of the work: a player can be thrown out of
	 * a guild, and a realm can fall to rung 2 of the arrears ladder, while
	 * the work is in progress.
	 */
	kingdom_realm *realm = kingdom_realm_of_char(ch);
	long banked = 0;

	if (realm && !kingdom_nodes_dormant(*realm))
		banked = kingdom_resource_deposit(*realm, res,
						  kingdom_harvest_yield(*realm, res,
									node->value[1]));

	/*
	 * THE CHARGE IS SPENT WHATEVER THE OUTCOME. The ground does not care who
	 * is digging; that is the whole of ruling 1. See the file banner for why
	 * this differs from the previous build, which refused to spend a charge
	 * when nothing could be banked.
	 */
	node->value[0]--;

	if (banked > 0)
		send_to_char_f(ch, "&+yYou add &+Y%ld&+y %s to your realm's stores.&n\r\n",
			       banked, kingdom_resource_name(res));
	else if (!realm)
		send_to_char_f(ch,
			       "&+LYou work the %s free, but you serve no realm and it is left "
			       "where it lies.&n\r\n",
			       kingdom_resource_name(res));
	else if (kingdom_nodes_dormant(*realm))
		send_to_char_f(ch,
			       "&+LYour realm's works are idle for unpaid upkeep; the %s is left "
			       "where it lies.&n\r\n",
			       kingdom_resource_name(res));
	else
		send_to_char_f(ch, "Your realm's %s stores will hold no more.\r\n",
			       kingdom_resource_name(res));

	act("$n gathers from the land.", TRUE, ch, NULL, NULL, TO_ROOM);

	/* Worked out. The message goes FIRST because extract_obj() frees the
	 * object and nothing below it may read the object -- `node` is cleared
	 * so that a later edit cannot. The region's reload sweep will put a
	 * fresh node, with a fresh richness roll, somewhere else. */
	if (node->value[0] <= 0)
	{
		send_to_char("&+LThat was the last of it; the node is exhausted.&n\r\n", ch);
		extract_obj(node);
		node = NULL;
		return;
	}

	/* Partially worked, so it is now on the clock: ruling 3. */
	kingdom_node_mark_worked(node);
}

/* `kingdom harvest`. Takes no argument.
 *
 * one_argument() silently swallows fill words such as "the" and "on", so a
 * sub-argument here would parse unpredictably -- and there is nothing to
 * choose between anyway, since placement allows only one node to a room. */
void kingdom_harvest_command(struct char_data *ch, char * /*argument*/)
{
	if (!SanityCheck(ch, "kingdom_harvest_command"))
		return;

	if (!kingdom_enabled())
	{
		send_to_char("Kingdoms are not enabled.\r\n", ch);
		return;
	}

	if (IS_NPC(ch))
	{
		send_to_char("You have no interest in honest work.\r\n", ch);
		return;
	}

	if (get_scheduled(ch, kingdom_harvest_tick))
	{
		send_to_char("You are already working the land!\r\n", ch);
		return;
	}

	if (IS_FIGHTING(ch))
	{
		send_to_char("You are rather busy for honest work.\r\n", ch);
		return;
	}

	if (!MIN_POS(ch, POS_STANDING + STAT_NORMAL))
	{
		send_to_char("You are far too relaxed to work.\r\n", ch);
		return;
	}

	const int rnum = ch->in_room;

	/* Touching the room is one of the two moments a lazy expiry is resolved.
	 * Done before the node is looked for, so a player who arrives at a rotted
	 * node is told there is nothing here rather than being allowed to start
	 * work on something the next sweep will delete. */
	kingdom_node_reap_room(rnum);

	P_obj node = kingdom_node_in_room(rnum);
	if (!node || node->R_num < 0)
	{
		send_to_char("There is no resource node here to work.\r\n", ch);
		return;
	}

	const int res = kingdom_resource_for_node_vnum(obj_index[node->R_num].virtual_number);
	if (res < 0)
	{
		send_to_char("There is nothing here you know how to work.\r\n", ch);
		return;
	}

	if (node->value[0] <= 0)
	{
		send_to_char("&+LThis node is worked out.&n\r\n", ch);
		return;
	}

	if (GET_VITALITY(ch) < KINGDOM_HARVEST_MIN_VITALITY)
	{
		send_to_char("You are far too exhausted for this.\r\n", ch);
		return;
	}

	/*
	 * NO OWNERSHIP TEST. Anyone may work any node -- ruling 1. The realm is
	 * looked up only to warn the digger up front when the work will bank
	 * nothing, so that nobody spends three ticks to find out.
	 */
	kingdom_realm *realm = kingdom_realm_of_char(ch);

	int ticks = get_property("kingdom.harvest.ticks", KINGDOM_HARVEST_TICKS_DEFAULT);
	if (ticks < 1)
		ticks = 1;
	/* Gods get a single tick, so that testing the wiring does not mean
	 * standing still for a quarter of a minute; mining does the same for the
	 * same reason. */
	if (IS_TRUSTED(ch))
		ticks = 1;

	struct kingdom_harvest_work work = { world[rnum].number, ticks };

	if (!add_event(kingdom_harvest_tick, PULSE_VIOLENCE, ch, NULL, NULL, 0, &work,
		       (int)sizeof(work)))
	{
		send_to_char("You cannot settle to the work just now.\r\n", ch);
		return;
	}

	send_to_char_f(ch, "You set to work gathering %s.\r\n", kingdom_resource_name(res));

	if (!realm)
		send_to_char("&+LYou serve no realm, so there is nowhere to bank what you "
			     "gather.&n\r\n",
			     ch);
	else if (kingdom_nodes_dormant(*realm))
		send_to_char("&+LYour realm's works are idle: the upkeep is unpaid, and nothing "
			     "you gather will be stored.&n\r\n",
			     ch);

	act("$n sets to work on the land.", TRUE, ch, NULL, NULL, TO_ROOM);
}

/*
 * One report on the ground underfoot.
 *
 * This lives in the harvest module rather than in kingdom_display.c because it
 * reports NODE state, and nodes are this file's business. It describes the node
 * wherever it is found -- every node is off kingdom land by construction now --
 * and reports where the yield would go as a following line rather than as a
 * gate.
 */
void kingdom_harvest_survey(struct char_data *ch)
{
	if (!SanityCheck(ch, "kingdom_harvest_survey"))
		return;

	if (!kingdom_enabled())
	{
		send_to_char("Kingdoms are not enabled.\r\n", ch);
		return;
	}

	const int rnum = ch->in_room;

	if (!kingdom_harvest_valid_rnum(rnum))
	{
		send_to_char("There is no ground here to survey.\r\n", ch);
		return;
	}

	/* The other moment a lazy expiry is resolved. */
	kingdom_node_reap_room(rnum);

	P_obj node = kingdom_node_in_room(rnum);
	if (!node || node->R_num < 0)
	{
		send_to_char("You find no resource node on this square.\r\n", ch);
		return;
	}

	const int res = kingdom_resource_for_node_vnum(obj_index[node->R_num].virtual_number);
	const int charges = node->value[0];
	const int richness = node->value[1];
	const bool worked = node->value[2] > 0;

	send_to_char_f(ch, "This node yields %s. %s %d draw%s left.\r\n",
		       kingdom_resource_name(res), kingdom_node_richness_text(richness), charges,
		       charges == 1 ? "" : "s");

	if (worked)
		send_to_char("&+LIt has been worked already, and what is left of it will not "
			     "keep.&n\r\n",
			     ch);

	kingdom_realm *realm = kingdom_realm_of_char(ch);

	if (!realm)
	{
		send_to_char("&+LYou serve no realm, so anything you draw from it would be "
			     "left where it lies.&n\r\n",
			     ch);
		return;
	}

	if (kingdom_nodes_dormant(*realm))
	{
		send_to_char("&+LYour realm's works are idle: the upkeep is unpaid.&n\r\n", ch);
		return;
	}

	if (kingdom_terrain_squares(*realm, res) >= KINGDOM_HARVEST_SQUARES_PER_STEP)
		send_to_char_f(ch,
			       "&+GYour realm's land favours %s, and it will bank what you "
			       "draw.&n\r\n",
			       kingdom_resource_name(res));
	else
		send_to_char("&+GYour realm will bank what you draw, though its land gives you "
			     "no advantage here.&n\r\n",
			     ch);
}
