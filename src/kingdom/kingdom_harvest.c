/*
 *  kingdom_harvest.c
 *  Duris
 *
 *  Resource nodes on a realm's OWN squares: which terrain yields which of
 *  KRES_MINERAL / KRES_WOOD / KRES_FIBRE / KRES_WATER, how a node spawns, is
 *  worked out and regrows, the harvest action itself, and the deposit into
 *  kingdom_realm::resources[].
 *
 *  NON-WITHDRAWABLE BY DESIGN (RULINGS.md, answer 3). Nothing in this file
 *  takes a resource back out of a realm, and nothing here hands a player an
 *  object: a harvest raises a counter on the realm record and stops there.
 *  That is the whole reason the counters are not kept as guild coin --
 *  Guild::withdraw (guild/assocs.h:293) is officer-accessible, so a store the
 *  officers could draw on would just be a slower treasury.
 *
 *  WHAT WAS MIRRORED FROM THE EXISTING GATHERING CODE, AND WHAT WAS NOT
 *  -------------------------------------------------------------------
 *  economy/mining.c is the house pattern for "work the ground a while and get
 *  something", and the SHAPE of the action is taken from it: a start check, a
 *  small trivially-copyable payload, an event every PULSE_VIOLENCE that
 *  re-validates the character before doing anything (mining.c:371-393), a
 *  vitality drain per tick with an exhaustion floor (mining.c:502, 555), the
 *  payoff on the final tick, and richness tiers rolled at spawn using mining's
 *  own 3 / 17 / 55 / 25 distribution and charge ranges (mining.c:660-696).
 *
 *  What is NOT copied is the STORAGE. A mine is a real object dropped into a
 *  room by a reload timer (mining.c:627-750) and extracted when it is worked
 *  out. A kingdom node cannot be an object: it would need a new obj vnum in
 *  world data plus an obj_index spec binding in initialize_mining()
 *  (mining.c:584-595), none of which this module owns, and it would leave
 *  extractable objects lying on land whose owner can be dissolved at any
 *  moment. A node here is a small record keyed by ROOM VNUM whose depletion
 *  and regrowth are arithmetic on time(NULL), evaluated when somebody actually
 *  swings at it. No timer, no object, nothing to leak.
 *
 *  NODE STATE IS NOT PERSISTED, ON PURPOSE. A realm's territory is one integer
 *  (kingdom_internal.h) precisely so that no per-square rows exist; writing 80
 *  node rows per realm would put them straight back. A reboot therefore
 *  reseeds every square full. That is generous rather than punitive, and it
 *  costs nothing anyone can hoard: the stores themselves are persisted by
 *  kingdom_db.c, only the state of the ground is forgotten.
 *
 *  DORMANCY. While a realm's arrears have reached KARR_NODES_DORMANT its nodes
 *  produce nothing at all -- rung 2 of the arrears ladder (kingdom_internal.h).
 *  Charges are not spent while dormant, so paying up restores the land exactly
 *  as it was left.
 */

#include "kingdom/kingdom_internal.h"

#include "core/structs.h"

#include "core/config.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "kingdom/kingdom_geometry.h"
#include "net/comm.h"

extern struct room_data *world;
extern int top_of_world;

/* ------------------------------------------------------------------ *
 * Tuning
 * ------------------------------------------------------------------ *
 * The compiled numbers are the defaults; the two that a god might want to
 * turn during a season are read from duris.properties the way mining reads
 * mines.reloadMins (mining.c:746-748). They deliberately do NOT live in
 * kingdom_config's struct, which is frozen and carries no harvest fields.
 */

/* Ticks of work a harvest takes, PULSE_VIOLENCE apart -- 3 ticks is 12s at
 * the current pulse (core/config.h:87). */
#define KINGDOM_HARVEST_TICKS_DEFAULT 3
/* Minutes a worked-out node needs before it is worth anything again. Matches
 * the mine reload interval so the two systems feel the same. */
#define KINGDOM_NODE_REGROW_MINS_DEFAULT 10
/* Vitality a tick of work costs, and the floor below which work is refused.
 * Mining spends 2-3 per tick and refuses below 10 (mining.c:502, 555). */
#define KINGDOM_HARVEST_VITALITY_COST 2
#define KINGDOM_HARVEST_MIN_VITALITY 10
/* Owned squares per step of the yield ladder; see kingdom_harvest_yield(). */
#define KINGDOM_HARVEST_SQUARES_PER_STEP 20
/* Ceiling on a stored resource. Deposits clamp rather than wrap: the counters
 * are persisted as signed values, and a wrapped negative store would read as a
 * debt the realm could never work off. */
#define KINGDOM_RESOURCE_CAP 1000000000L

/* kingdom_resource_name() is declared in kingdom_internal.h and DEFINED in
 * kingdom_config.c, beside the static_assert that proves the name table covers
 * every KRES_ value. It is used freely below and deliberately not redefined
 * here: a second definition is a link error, and a second table of
 * player-facing names would drift from the first. */

/* ------------------------------------------------------------------ *
 * Terrain -> resource
 * ------------------------------------------------------------------ */

/* rnum 0 is rejected deliberately: real_room0() answers 0 both for the first
 * room and for "no such vnum" (world/db.c:4477-4508), and every caller here
 * means the latter. ch->in_room can also be NOWHERE, which is -1
 * (core/config.h:154). Same rule as kingdom_geometry.c. */
static bool kingdom_harvest_valid_rnum(int rnum)
{
	return rnum > 0 && rnum <= top_of_world;
}

/* What a square's OWN terrain grows.
 *
 * sector_type is the only terrain signal there is: room_data once carried a
 * `resources` bitfield to answer exactly this question, but it is commented
 * out (core/structs.h:881) along with the old kingdom_num / kingdom_type
 * fields, so the RESOURCE_ORE / RESOURCE_TIMBER / RESOURCE_HERB bits at
 * core/defines.h:491-499 are dead constants that no world file populates.
 *
 * Only sectors a realm could actually hold appear here. Underdark sectors are
 * absent on purpose -- the Underdark ban is absolute (RULINGS.md, answer 5),
 * so no owned square is ever underground, and a KRES_ entry for one would be
 * dead code quietly inviting the ban to be relaxed by accident. Built-up and
 * indoor sectors (SECT_CITY, SECT_ROAD, SECT_INSIDE, the castle sectors) fall
 * through to "grows nothing": there is no soil left to work. */
static int kingdom_sector_resource(int sector)
{
	switch (sector)
	{
	case SECT_MOUNTAIN:
	case SECT_HILLS:
	/* Bare rock, sand and ice are all worked for stone, salt and ore
	 * rather than for anything that grows. */
	case SECT_DESERT:
	case SECT_ARCTIC:
		return KRES_MINERAL;
	case SECT_FOREST:
	case SECT_SNOWY_FOREST:
		return KRES_WOOD;
	/* Grass, flax and reed beds: the realm's rope, cloth and thatch. */
	case SECT_FIELD:
	case SECT_SWAMP:
		return KRES_FIBRE;
	/* Placement normally refuses open water, but a square that does pass the
	 * gate should behave sensibly rather than fall through to the default. */
	case SECT_WATER_SWIM:
	case SECT_WATER_NOSWIM:
	case SECT_OCEAN:
		return KRES_WATER;
	default:
		return -1;
	}
}

/* True when one of the four cardinally neighbouring MAP squares is open water.
 *
 * Walked through kingdom_room_at() rather than through dir_option[] on
 * purpose: a map square's exits may be missing where the grid plainly
 * continues, and territory is defined on the grid, not on the exits.
 * kingdom_room_at() also refuses to leave the zone, where
 * calculate_relative_room() would wrap toroidally to the far side of the
 * continent (world/map.c:244-270). */
static bool kingdom_square_touches_water(int rnum)
{
	int zone_idx = -1, sx = -1, sy = -1;
	if (!kingdom_square_of_room(rnum, &zone_idx, &sx, &sy))
		return false;

	static const struct kingdom_offset cardinals[4] = { { 0, -1 },
							    { 1, 0 },
							    { 0, 1 },
							    { -1, 0 } };

	for (int i = 0; i < 4; i++)
	{
		const int neighbour =
			kingdom_room_at(zone_idx, sx + cardinals[i].dx, sy + cardinals[i].dy);
		if (!kingdom_harvest_valid_rnum(neighbour))
			continue;
		if (IS_WATER_ROOM(neighbour))
			return true;
	}

	return false;
}

/* The resource a square yields, or -1 when it yields nothing.
 *
 * A SHORELINE SQUARE ANSWERS KRES_WATER WHATEVER IT IS MADE OF, because the
 * water is drawn from the square next to it. This mirrors mine_friendly()
 * (mining.c:115-141), which already lets a square be worked for what its
 * NEIGHBOURS are made of rather than only its own sector. Without the rule
 * KRES_WATER would be unreachable, since placement refuses open water; with
 * it, a coastal realm trades one thin band of forest or field for the only
 * source of water on the map, and a landlocked realm simply has none. */
int kingdom_resource_for_room(int rnum)
{
	if (!kingdom_harvest_valid_rnum(rnum))
		return -1;

	if (kingdom_square_touches_water(rnum))
		return KRES_WATER;

	return kingdom_sector_resource((int)world[rnum].sector_type);
}

/* ------------------------------------------------------------------ *
 * Node state
 * ------------------------------------------------------------------ */

/* The state of one square's ground. Held only in memory -- see the file
 * banner for why it is deliberately not persisted. */
struct kingdom_node
{
	int assoc_id; /* realm the node was seeded for */
	int resource; /* KRES_* */
	int richness; /* 0 poor .. 3 mother lode, mining's own scale */
	int charges; /* draws left before the square is worked out */
	time_t regrow_at; /* when a worked-out node refills; 0 = not depleted */
};

/* Keyed by room VNUM, matching kingdom_square_index, because vnums survive a
 * world rebuild that renumbers rnums. Bounded by the squares a realm has
 * actually held -- at most 80 each -- and kingdom_harvest_release() /
 * kingdom_harvest_prune() drop entries when land is lost. */
static std::unordered_map<int, kingdom_node> kingdom_harvest_nodes;

static int kingdom_node_regrow_seconds(void)
{
	int mins = get_property("kingdom.node.regrowMins", KINGDOM_NODE_REGROW_MINS_DEFAULT);
	if (mins < 1)
		mins = 1;
	return mins * 60;
}

/* Roll a fresh node's richness and charge count.
 *
 * The distribution and the charge ranges are mining's, verbatim
 * (mining.c:660-696): 3% mother lode, 17% rich, 55% ordinary, 25% poor, with
 * 24-32, 16-24, 12-20 and 8-16 draws respectively. Copying the numbers rather
 * than inventing new ones keeps the two gathering systems reading the same
 * way to a player who has mined before. */
static void kingdom_node_roll(kingdom_node &node)
{
	const int roll = number(0, 99);

	if (roll < 3)
	{
		node.richness = 3;
		node.charges = number(24, 32);
	}
	else if (roll < 20)
	{
		node.richness = 2;
		node.charges = number(16, 24);
	}
	else if (roll < 75)
	{
		node.richness = 1;
		node.charges = number(12, 20);
	}
	else
	{
		node.richness = 0;
		node.charges = number(8, 16);
	}

	node.regrow_at = 0;
}

/* One line for the player describing how good the ground is. */
static const char *kingdom_node_richness_text(int richness)
{
	switch (richness)
	{
	case 3:
		return "&+GThe ground here is extraordinarily rich.&n";
	case 2:
		return "&+yThe ground here is generous.&n";
	case 1:
		return "&+LThe ground here is workable.&n";
	default:
		return "&+LThe ground here is thin.&n";
	}
}

/* Fetch the node on an owned square, seeding it on first touch. Returns NULL
 * when the square grows nothing at all. */
static kingdom_node *kingdom_node_for_room(int rnum, int assoc_id)
{
	const int res = kingdom_resource_for_room(rnum);
	if (res < 0)
		return NULL;

	/* Safe: kingdom_resource_for_room() already refused an invalid rnum. */
	const int vnum = world[rnum].number;

	auto found = kingdom_harvest_nodes.find(vnum);
	if (found == kingdom_harvest_nodes.end())
	{
		kingdom_node fresh{};
		fresh.assoc_id = assoc_id;
		fresh.resource = res;
		kingdom_node_roll(fresh);

		found = kingdom_harvest_nodes.emplace(vnum, fresh).first;
		return &found->second;
	}

	kingdom_node &node = found->second;

	/* The square changed hands, or the terrain under it was rebuilt. Reseed
	 * rather than let one realm inherit another realm's exhausted ground --
	 * guild ids are reused, found_asc() hands out the lowest free one. */
	if (node.assoc_id != assoc_id || node.resource != res)
	{
		node.assoc_id = assoc_id;
		node.resource = res;
		kingdom_node_roll(node);
	}

	return &node;
}

/* True when the node has a draw left, rerolling it in passing once its
 * regrowth time has come. This is where depletion is undone: there is no
 * timer. The reroll is deliberate -- a mine that is worked out is extracted
 * and a fresh one loads elsewhere with fresh quality (mining.c:280-284,
 * 627-696), so the ground here is likewise worth a new roll rather than
 * being permanently as good or as bad as the day it was claimed. */
static bool kingdom_node_ready(kingdom_node &node, time_t now)
{
	if (node.charges > 0)
		return true;

	if (node.regrow_at != 0 && now >= node.regrow_at)
	{
		kingdom_node_roll(node);
		return true;
	}

	return false;
}

/* Whole minutes of regrowth still owed by a worked-out node.
 *
 * Only ever called after kingdom_node_ready() has said no, so the answer is
 * floored at 1 in every branch: "about 0 minutes" would be a worse lie than
 * rounding up, and a node with no charges and no regrow time set -- which the
 * charge ranges make unreachable -- must still read as "wait a moment". */
static int kingdom_node_regrow_minutes(const kingdom_node &node, time_t now)
{
	if (node.regrow_at <= now)
		return 1;

	const long remaining = (long)(node.regrow_at - now);
	const int minutes = (int)((remaining + 59) / 60);

	return minutes < 1 ? 1 : minutes;
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
 * YIELD PER DRAW = (base + richness) + random(0 .. base + richness)
 *
 *     base = 1 + owned_squares / 20        richness = 0 poor .. 3 mother lode
 *
 *      owned squares    base    poor node    mother lode
 *          0 - 19         1        1 - 2        4 -  8
 *         20 - 39         2        2 - 4        5 - 10
 *         40 - 59         3        3 - 6        6 - 12
 *         60 - 79         4        4 - 8        7 - 14
 *             80          5        5 - 10       8 - 16
 *
 * TERRITORY IS THE SCALING INPUT, as the design requires: a full realm draws
 * roughly five times what a first-ring realm does. That is the counterweight
 * to upkeep, which scales with the same number (kingdom_upkeep_due) -- growth
 * only pays for itself while the coin keeps arriving, and a realm that stops
 * paying loses the nodes at rung 2 before it loses any land at rung 3.
 *
 * static because kingdom_node is private to this file: an external linkage
 * symbol nothing outside could ever call would only look like a seam.
 */
static long kingdom_harvest_yield(const kingdom_realm &realm, const kingdom_node &node)
{
	int squares = realm.highest_claim;

	/* highest_claim is loaded from persistence, so clamp rather than trust. */
	if (squares < 0)
		squares = 0;
	if (squares > KINGDOM_MAX_SQUARES)
		squares = KINGDOM_MAX_SQUARES;

	int richness = node.richness;
	if (richness < 0)
		richness = 0;
	if (richness > 3)
		richness = 3;

	const int step = 1 + (squares / KINGDOM_HARVEST_SQUARES_PER_STEP) + richness;

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

/* ------------------------------------------------------------------ *
 * Lifecycle
 * ------------------------------------------------------------------ */

/* Drop every node record belonging to a realm. Call when the realm is
 * dissolved, and after any change that makes its claims untrustworthy (a
 * guildhall move, for instance). A reclaimed square then seeds a fresh node,
 * which is generous, but the alternative -- leaving records keyed to a dead
 * association id -- would hand the next guild to take that id, and found_asc()
 * reuses the lowest free one, somebody else's worked-out ground. */
void kingdom_harvest_release(int assoc_id)
{
	for (auto it = kingdom_harvest_nodes.begin(); it != kingdom_harvest_nodes.end();)
	{
		if (it->second.assoc_id == assoc_id)
			it = kingdom_harvest_nodes.erase(it);
		else
			++it;
	}
}

/* Drop the nodes on squares a realm no longer holds. This is the cheap half of
 * kingdom_harvest_release(), for rung 3 of the arrears ladder where one outer
 * ring reverts per missed cycle and the inner rings must keep the ground they
 * have been working. */
void kingdom_harvest_prune(const kingdom_realm &realm)
{
	if (!kingdom_harvest_valid_rnum(realm.hall_rnum))
		return;

	int hall_zone = -1, hall_x = -1, hall_y = -1;
	if (!kingdom_square_of_room(realm.hall_rnum, &hall_zone, &hall_x, &hall_y))
		return;

	for (auto it = kingdom_harvest_nodes.begin(); it != kingdom_harvest_nodes.end();)
	{
		if (it->second.assoc_id != realm.assoc_id)
		{
			++it;
			continue;
		}

		/* real_room0() answers 0 for a vnum that is no longer in the
		 * database as well as for the first room, so a 0 here means the
		 * square is gone and the record with it. */
		const int rnum = real_room0(it->first);
		int node_zone = -1, node_x = -1, node_y = -1;

		if (!kingdom_harvest_valid_rnum(rnum) ||
		    !kingdom_square_of_room(rnum, &node_zone, &node_x, &node_y) ||
		    node_zone != hall_zone)
		{
			it = kingdom_harvest_nodes.erase(it);
			continue;
		}

		const int index = kingdom_index_for_offset(node_x - hall_x, node_y - hall_y);

		/* index 0 means the square is not part of this realm's footprint
		 * at all any more; anything past highest_claim has reverted. */
		if (index < 1 || index > realm.highest_claim)
			it = kingdom_harvest_nodes.erase(it);
		else
			++it;
	}
}

void kingdom_harvest_shutdown(void)
{
	kingdom_harvest_nodes.clear();
}

/* ------------------------------------------------------------------ *
 * The harvest action
 * ------------------------------------------------------------------ */

/* Carried across the ticks of one harvest. Trivially copyable, and it holds
 * NO POINTERS: a kingdom_realm* cannot survive a tick because the realm can be
 * erased from kingdom_realms in between (kingdom_on_guild_deleted), and a
 * P_char is the event system's business, not ours. */
struct kingdom_harvest_work
{
	int assoc_id;
	int room_vnum;
	int ticks;
};

static void kingdom_harvest_tick(P_char ch, P_char victim, P_obj obj, void *data);

/* Resolve the realm whose land `ch` is standing on and that `ch` may work.
 * Sends the refusal itself and returns NULL when the answer is no. */
static kingdom_realm *kingdom_harvest_ground(P_char ch)
{
	const int rnum = ch->in_room;

	if (!kingdom_harvest_valid_rnum(rnum))
	{
		send_to_char("There is no ground here to work.\r\n", ch);
		return NULL;
	}

	/* kingdom_owner_of_room() answers 0 for "nobody", which is also why the
	 * module never uses association id 0 for a realm. */
	const int owner = kingdom_owner_of_room(rnum);
	if (owner == 0)
	{
		send_to_char("This land belongs to no realm.\r\n", ch);
		return NULL;
	}

	if (!kingdom_char_owns_room(ch, rnum))
	{
		send_to_char("&+rThis is not your realm's land to work.&n\r\n", ch);
		return NULL;
	}

	kingdom_realm *realm = kingdom_find_realm(owner);
	if (!realm)
	{
		/* The square index outlived its realm: a bug worth a log line
		 * rather than a crash. */
		logit(LOG_KINGDOM, "harvest: room %d indexed to assoc %d with no realm record.",
		      world[rnum].number, owner);
		send_to_char("This land answers to no-one you can find.\r\n", ch);
		return NULL;
	}

	/* Node records are keyed to realm.assoc_id and released by the id the
	 * rest of the server knows. If the record's own id ever disagreed with
	 * the index that found it, released nodes would outlive their realm and
	 * be inherited by the next guild to take the id. Refuse, do not guess. */
	if (realm->assoc_id != owner)
	{
		logit(LOG_KINGDOM, "harvest: room %d indexed to assoc %d but realm records %d.",
		      world[rnum].number, owner, realm->assoc_id);
		send_to_char("This land answers to no-one you can find.\r\n", ch);
		return NULL;
	}

	return realm;
}

/* The per-tick body. Everything it needs is re-derived from the payload, never
 * carried as a pointer, because a realm can be dissolved and a character can
 * be moved between two ticks four seconds apart. */
static void kingdom_harvest_tick(P_char ch, P_char /*victim*/, P_obj, void *data)
{
	if (!data)
		return;

	/* Copy the payload before anything else. The buffer behind `data` belongs
	 * to the event that is firing and is retired when this returns, so the
	 * rescheduling call at the bottom hands add_event a live local to copy
	 * rather than the buffer being torn down. */
	const struct kingdom_harvest_work work = *(const struct kingdom_harvest_work *)data;

	/* SanityCheck() can itself char_from_room()/char_to_room() a character it
	 * finds in NOWHERE (core/utility.c:3200-3222) -- but it returns FALSE on
	 * that path, so returning here is the only safe thing to do with the
	 * answer. Never test ch's room after it and carry on. */
	if (!SanityCheck(ch, "kingdom_harvest_tick"))
		return;

	if (!kingdom_enabled())
		return;

	if (!ch->desc || IS_FIGHTING(ch) || IS_DESTROYING(ch) || !IS_AWAKE(ch) ||
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

	/* Ownership is re-asked every tick, not assumed from the start of the
	 * work: a ring can revert under rung 3 of the arrears ladder, and a
	 * player can be thrown out of the guild, while the work is in progress. */
	kingdom_realm *realm = kingdom_find_realm(work.assoc_id);
	if (!realm || kingdom_owner_of_room(rnum) != work.assoc_id ||
	    !kingdom_char_owns_room(ch, rnum))
	{
		send_to_char("&+rThis is no longer your realm's land to work.&n\r\n", ch);
		return;
	}

	if (kingdom_nodes_dormant(*realm))
	{
		send_to_char("&+LThe works stop dead: your realm's upkeep is unpaid.&n\r\n", ch);
		return;
	}

	kingdom_node *node = kingdom_node_for_room(rnum, work.assoc_id);
	if (!node)
	{
		send_to_char("There is nothing worth gathering here after all.\r\n", ch);
		return;
	}

	const time_t now = time(NULL);

	if (!kingdom_node_ready(*node, now))
	{
		const int wait_minutes = kingdom_node_regrow_minutes(*node, now);

		send_to_char_f(ch, "&+LThis ground is worked out; give it about %d minute%s.&n\r\n",
			       wait_minutes, wait_minutes == 1 ? "" : "s");
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
		 * owner, an exhausted sequence (core/structs.h:2075-2085) -- and
		 * an unchecked refusal here would leave the player standing in a
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

	const long yield = kingdom_harvest_yield(*realm, *node);
	const long banked = kingdom_resource_deposit(*realm, node->resource, yield);

	/* A full store spends no charge: the square is untouched and the work can
	 * be repeated the moment the realm has spent something. */
	if (banked <= 0)
	{
		send_to_char_f(ch, "Your realm's %s stores will hold no more.\r\n",
			       kingdom_resource_name(node->resource));
		return;
	}

	node->charges--;
	if (node->charges <= 0)
	{
		node->charges = 0;
		node->regrow_at = now + (time_t)kingdom_node_regrow_seconds();
	}

	send_to_char_f(ch, "&+yYou add &+Y%ld&+y %s to your realm's stores.&n\r\n", banked,
		       kingdom_resource_name(node->resource));
	act("$n gathers for the realm.", TRUE, ch, NULL, NULL, TO_ROOM);

	if (node->regrow_at != 0)
		send_to_char("&+LThat was the last of it; this square is worked out.&n\r\n", ch);
}

/* `kingdom harvest`. Takes no argument.
 *
 * one_argument() silently swallows fill words such as "the" and "on", so a
 * sub-argument here would parse unpredictably -- and there is nothing on a
 * square to choose between anyway, since a square grows exactly one thing. */
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
		send_to_char("You have no realm to gather for.\r\n", ch);
		return;
	}

	if (get_scheduled(ch, kingdom_harvest_tick))
	{
		send_to_char("You are already working the land!\r\n", ch);
		return;
	}

	if (IS_FIGHTING(ch) || IS_DESTROYING(ch))
	{
		send_to_char("You are rather busy for honest work.\r\n", ch);
		return;
	}

	if (!MIN_POS(ch, POS_STANDING + STAT_NORMAL))
	{
		send_to_char("You are far too relaxed to work.\r\n", ch);
		return;
	}

	kingdom_realm *realm = kingdom_harvest_ground(ch);
	if (!realm)
		return;

	if (kingdom_nodes_dormant(*realm))
	{
		send_to_char("&+LThe land lies idle: your realm's upkeep is unpaid.&n\r\n",
			     ch);
		return;
	}

	const int rnum = ch->in_room;
	kingdom_node *node = kingdom_node_for_room(rnum, realm->assoc_id);
	if (!node)
	{
		send_to_char("There is nothing worth gathering on this square.\r\n", ch);
		return;
	}

	const time_t now = time(NULL);

	if (!kingdom_node_ready(*node, now))
	{
		const int wait_minutes = kingdom_node_regrow_minutes(*node, now);

		send_to_char_f(ch,
			       "&+LThis ground is worked out; give it about %d minute%s.&n\r\n",
			       wait_minutes, wait_minutes == 1 ? "" : "s");
		return;
	}

	if (GET_VITALITY(ch) < KINGDOM_HARVEST_MIN_VITALITY)
	{
		send_to_char("You are far too exhausted for this.\r\n", ch);
		return;
	}

	int ticks = get_property("kingdom.harvest.ticks", KINGDOM_HARVEST_TICKS_DEFAULT);
	if (ticks < 1)
		ticks = 1;
	/* Gods get a single tick, so that testing the wiring does not mean
	 * standing still for a quarter of a minute (mining.c:332-335 does the
	 * same for the same reason). */
	if (IS_TRUSTED(ch))
		ticks = 1;

	struct kingdom_harvest_work work = { .assoc_id = realm->assoc_id,
					     .room_vnum = world[rnum].number,
					     .ticks = ticks };

	if (!add_event(kingdom_harvest_tick, PULSE_VIOLENCE, ch, NULL, NULL, 0, &work,
		       (int)sizeof(work)))
	{
		send_to_char("You cannot settle to the work just now.\r\n", ch);
		return;
	}

	send_to_char_f(ch, "You set to work gathering %s for the realm.\r\n",
		       kingdom_resource_name(node->resource));
	act("$n sets to work on the land.", TRUE, ch, NULL, NULL, TO_ROOM);
}

/* One report on the square underfoot. This lives in the harvest module rather
 * than in kingdom_display.c because it reports NODE state, and node state
 * exists nowhere but this file. */
void kingdom_harvest_survey(struct char_data *ch)
{
	if (!SanityCheck(ch, "kingdom_harvest_survey"))
		return;

	if (!kingdom_enabled())
	{
		send_to_char("Kingdoms are not enabled.\r\n", ch);
		return;
	}

	kingdom_realm *realm = kingdom_harvest_ground(ch);
	if (!realm)
		return;

	const int rnum = ch->in_room;
	const int res = kingdom_resource_for_room(rnum);

	if (res < 0)
	{
		send_to_char("This square yields nothing that can be worked.\r\n", ch);
		return;
	}

	/* Reported before the node is touched, so that a dormant realm cannot
	 * seed nodes by surveying and does not learn what it is missing. */
	if (kingdom_nodes_dormant(*realm))
	{
		send_to_char_f(ch, "This square would yield %s, but the works are idle.\r\n",
			       kingdom_resource_name(res));
		return;
	}

	kingdom_node *node = kingdom_node_for_room(rnum, realm->assoc_id);
	if (!node)
	{
		send_to_char("This square yields nothing that can be worked.\r\n", ch);
		return;
	}

	const time_t now = time(NULL);

	if (!kingdom_node_ready(*node, now))
	{
		const int wait_minutes = kingdom_node_regrow_minutes(*node, now);

		send_to_char_f(ch, "This ground is worked out for about another %d minute%s.\r\n",
			       wait_minutes, wait_minutes == 1 ? "" : "s");
		return;
	}

	send_to_char_f(ch, "This square yields %s. %s %d draw%s left.\r\n",
		       kingdom_resource_name(node->resource),
		       kingdom_node_richness_text(node->richness), node->charges,
		       node->charges == 1 ? "" : "s");
}
