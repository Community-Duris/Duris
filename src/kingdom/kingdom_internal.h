/*
 *  kingdom_internal.h
 *  Duris
 *
 *  Private to src/kingdom/. Nothing outside this directory may include it --
 *  the public surface is kingdom.h and it is deliberately small.
 */

#ifndef _KINGDOM_INTERNAL_H_
#define _KINGDOM_INTERNAL_H_

#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>

#include "kingdom/kingdom.h"
#include "kingdom/kingdom_geometry.h"
#include "world/vnum.obj.h"

class Guild;
class Guildhall;
#ifdef __NO_MYSQL__
struct flatfile_association_record;
#endif

#define LOG_KINGDOM "logs/log/kingdom"

/* ------------------------------------------------------------------ *
 * Resources
 * ------------------------------------------------------------------ *
 * Typed, non-withdrawable quantities held by the realm. Ruled 2026-08-28:
 * once deposited these can only be SPENT on kingdom benefits -- there is
 * deliberately no withdraw verb, which is why they cannot live in the guild
 * coin treasury (Guild::withdraw is officer-accessible).
 *
 * Upkeep is charged in COIN, not in these.
 */
enum kingdom_resource
{
	KRES_MINERAL = 0,
	KRES_WOOD,
	KRES_FIBRE,
	KRES_WATER,
	KRES_MAX
};

const char *kingdom_resource_name(int res);

/* ------------------------------------------------------------------ *
 * World-data vnums
 * ------------------------------------------------------------------ *
 * RULED 2026-09-01: harvest nodes are REAL OBJECTS scattered at random
 * across the world, exactly as ore and gem mines are -- not virtual records
 * on owned squares. So each resource needs a prototype, and the prototypes
 * live in heavens.obj beside the mines they are modelled on (VOBJ_MINE 193,
 * VOBJ_GEMMINE 434), inside that file's own 10-1300 block so no new area
 * file and no areas/AREA manifest change is needed.
 *
 * 477-488 was measured free across every .obj file in areas/ (444 at the
 * time of the sweep); 108-113 likewise across every .mob file (448). Keep
 * these in step with the prototypes.
 */
/* TWO SETS, ruled 2026-09-01: nodes spawn in the Underdark as well as on the
 * surface, and an Underdark node must be thematic -- a mushroom, not a tree.
 * Same four resources either way; only the flavour and the map letter differ. */
/* The eight prototype vnums, VOBJ_KINGDOM_NODE_MINERAL .. _UD_WATER, are
 * defined in world/vnum.obj.h (included above) beside VOBJ_MINE and
 * VOBJ_GEMMINE, so the map renderer and this module read ONE definition. */

/* True when a node prototype is the Underdark variant. */
bool kingdom_node_is_underdark(int vnum);
/* The prototype for `res` in the given world half. */
int kingdom_node_vnum_for(int res, bool underdark);

/* The garrison mob. heavens.mob 108; 109-113 are reserved beside it should
 * racewar-specific guards be wanted later. */
#define VMOB_KINGDOM_GUARD 108
/* The champion, heavens.mob 109. A SEPARATE prototype rather than a guard with
 * its level raised, because it carries its own proc, its own description and
 * its own loadout, and binding two different procs to one prototype is not
 * possible. */
#define VMOB_KINGDOM_CHAMPION 109

/* The champion's two banners, heavens.obj 485 and 486, in the same block as
 * the harvest nodes. A banner is a real object standing in a real room: it can
 * be looked at, it can be attacked, and it can be destroyed, which is the
 * whole point of it -- an aura nobody could break would just be a passive. */
#define VOBJ_KINGDOM_BANNER_COMBAT 485
#define VOBJ_KINGDOM_BANNER_SANCTITY 486

/* ------------------------------------------------------------------ *
 * The garrison roster
 * ------------------------------------------------------------------ *
 * Ruled 2026-09-04. A guard is bought, not conjured by owning land: the land
 * sets the CEILING on how many a realm may field and how high they may rise,
 * and the guild pays for each one and names its class.
 *
 * The ladder of levels is tied to completed rings, so the garrison improves
 * with the realm rather than with the treasury alone:
 *
 *     ring 1 complete  (8 squares)   level 50
 *     ring 2 complete  (24 squares)  level 52
 *     ring 3 complete  (48 squares)  level 54
 *     ring 4 complete  (80 squares)  level 56, and the champion at 60
 *
 * A newly hired guard stands at the prototype's own level and costs
 * kingdom.guard.cost.base; each promotion costs an equal share of the span up
 * to kingdom.guard.cost.max, so a guard taken all the way to 56 has cost
 * exactly the configured maximum. PROMOTION IS ONE-WAY: a guard's level never
 * falls, not on arrears, not on land lost, not on a guild's request.
 */
#define KINGDOM_GUARD_SLOTS 16
#define KINGDOM_GUARD_BASE_LEVEL 45
#define KINGDOM_GUARD_FIRST_TIER_LEVEL 50
#define KINGDOM_GUARD_TIER_STEP 2
#define KINGDOM_GUARD_TOP_LEVEL 56
#define KINGDOM_CHAMPION_LEVEL 60
/* Promotions between base and top: 50, 52, 54, 56. */
#define KINGDOM_GUARD_TIERS 4
/* The champion's slot, one past the guards, so both live in one roster table
 * keyed by (association, slot) and one loader reads them. */
#define KINGDOM_CHAMPION_SLOT KINGDOM_GUARD_SLOTS

struct kingdom_guard_slot
{
	int guard_class = 0; /* a CLASS_* bit, or 0 for an empty slot */
	int level = 0; /* 0 for an empty slot */
};

/* Resource a node prototype yields, or -1 if the vnum is not a kingdom node. */
int kingdom_resource_for_node_vnum(int vnum);

/* ------------------------------------------------------------------ *
 * Arrears ladder
 * ------------------------------------------------------------------ *
 * Ruled 2026-08-28, in this order, halting wherever payment arrives:
 *   0  current
 *   1  guards despawn
 *   2  harvest nodes go dormant
 *   3+ one OUTER ring reverts per missed cycle
 * Reclaiming a reverted ring costs the full original price again.
 */
enum kingdom_arrears
{
	KARR_CURRENT = 0,
	KARR_GUARDS_GONE,
	KARR_NODES_DORMANT,
	KARR_RINGS_REVERTING
};

/* ------------------------------------------------------------------ *
 * The realm
 * ------------------------------------------------------------------ *
 * THE TERRITORY IS ONE INTEGER. Because claims are strictly ordered and a
 * ring completes before the next opens, a realm owns exactly claim indices
 * 1..highest_claim and nothing else. That is the entire ownership record:
 * no per-square rows, no 80-row rewrite when a guard dies, and reverting a
 * ring is just lowering highest_claim to a ring boundary.
 */
struct kingdom_realm
{
	int realm_id = 0;
	int assoc_id = 0;

	/* Anchor. hall_vnum is persisted; the rest is resolved at boot and after
	 * any guildhall move, and left invalid when the hall cannot be found. */
	int hall_vnum = 0;
	int hall_rnum = 0; /* 0 = unresolved, matching real_room0's sentinel */
	int zone_idx = -1;
	int hall_x = -1;
	int hall_y = -1;

	int highest_claim = 0; /* 0..KINGDOM_MAX_SQUARES */

	long resources[KRES_MAX] = { 0, 0, 0, 0 };

	/* THE ROSTER. Ruled 2026-09-04: guards stopped being a number derived
	 * from land and became individuals a guild BUYS, names a class for and
	 * promotes. The land still says how many a realm may field
	 * (kingdom_guard_allowance()) and how high they may rise
	 * (kingdom_guard_level_cap()), but nothing stands up that was not paid
	 * for, so this is the one part of the garrison that must be persisted.
	 *
	 * Slot order is hire order and never changes, because `kingdom roster`
	 * numbers guards by it and a player who reads "upgrade 3" must get the
	 * same guard the listing showed. A slot with level 0 is empty. */
	kingdom_guard_slot guards[KINGDOM_GUARD_SLOTS] = {};

	/* The champion, bought only by a realm holding all eighty squares. Its
	 * class is the two-class multiclass the guild chose; level is fixed at
	 * KINGDOM_CHAMPION_LEVEL, so a non-zero class IS the champion's
	 * existence and there is no second flag to keep in step. */
	int champion_class = 0;

	time_t upkeep_paid_through = 0;
	int arrears = KARR_CURRENT;
	int missed_cycles = 0;

	bool dirty = false;

	/* A payment was taken in memory but its PAIRED write -- the debited
	 * guild together with this record -- has not yet landed. While set, the
	 * generic flush must NOT publish this record (it would record a payment
	 * the guild has not durably made) and the sweep must not bill again; the
	 * pair is retried by kingdom_upkeep_retry_pending(). Never persisted. */
	bool payment_pending = false;
};

/* ------------------------------------------------------------------ *
 * Module state
 * ------------------------------------------------------------------ */

/* Realms by association id. */
extern std::unordered_map<int, kingdom_realm> kingdom_realms;

/* Owned map ROOM VNUM -> association id. Rebuilt whenever a realm's anchor or
 * highest_claim changes. This exists because kingdom_owner_of_room() is called
 * from movement and from the map renderer, where an O(realms * 80) scan would
 * be paid on every step -- the guildhall subsystem's plain vector with linear
 * finders is exactly the shape not to copy here. */
extern std::unordered_map<int, int> kingdom_square_index;

kingdom_realm *kingdom_find_realm(int assoc_id);
void kingdom_reindex_realm(const kingdom_realm &realm);
void kingdom_unindex_realm(int assoc_id);
void kingdom_reindex_all(void);

/* Resolve hall_rnum/zone/x/y from hall_vnum. False when the anchor is gone. */
bool kingdom_resolve_anchor(kingdom_realm &realm);

/* ------------------------------------------------------------------ *
 * Config (kingdom_config.c) -- lib/kingdom.cfg with compiled fallbacks
 * ------------------------------------------------------------------ */
struct kingdom_config
{
	bool enabled = false;
	/* Coin cost of claim n: base * growth^(n-1), COMPOUNDING, in copper.
	 * Ruled 2026-09-03: land must be a project for a guild of ten or more,
	 * so the first square is 1,000 platinum and each one after costs 5%
	 * more than the last -- square 80 lands at ~47,200 platinum and the
	 * whole map at ~971,000. Growth is per MILLE so the curve is integer
	 * arithmetic end to end: 1050 is x1.05. */
	long claim_cost_base = 1000000;
	int claim_cost_growth_permille = 1050;
	/* Material cost of claim 1, per resource, on the same curve. Every
	 * square wants all four, so expansion cannot run on one kind of ground. */
	long claim_material_base = 25;
	/* Coin upkeep per owned square per cycle. */
	long upkeep_per_square = 250;
	/* Real seconds between upkeep charges. Ruled 2026-09-03: one real week,
	 * not the hour it started at -- upkeep is a standing obligation on a
	 * guild, not a tax on being logged in. */
	int upkeep_period_seconds = 604800;
	/* Guards permitted per owned square, and the ratio's denominator. */
	int guards_per_squares = 5;
	/* What a guard costs, in copper. A newly raised guard is the base; the
	 * fully upgraded level-56 guard is the cap, and the tiers in between are
	 * spaced evenly across the ring the realm has completed. Ruled
	 * 2026-09-03: 5,000 platinum to raise one, 20,000 for a level 56. */
	long guard_cost_base = 5000000;
	long guard_cost_max = 20000000;
	/* Distance in map squares a realm must keep from a hometown or a zone
	 * entrance. These are the values a server with no lib/kingdom.cfg gets,
	 * so they must be values the map can actually satisfy: at 30, the pair
	 * they started at, a survey of all 160,000 surface squares finds ZERO
	 * legal seats -- 367 zone-crossing squares each exclude a 59x59 box and
	 * together they cover the continent. Ten leaves about 500. The shipped
	 * lib/kingdom.cfg carries the full survey. Ruled 2026-09-03 down to 5,
	 * which leaves about 5,000: ten was survivable but still meant a long
	 * hunt for a legal seat. */
	int min_hometown_distance = 5;
	int min_entrance_distance = 5;
};
extern kingdom_config kingdom_cfg;
void kingdom_config_load(void);

/* ------------------------------------------------------------------ *
 * Placement (kingdom_placement.c)
 * ------------------------------------------------------------------ *
 * THE SINGLE AUTHORITY on "may this square be part of a realm". Every
 * display, every command and the guildhall gate ask THIS -- none of them
 * re-derives the rule, because two copies of an availability predicate drift
 * and the weaker one lies.
 */
enum kingdom_square_verdict
{
	KSQ_OK = 0,
	KSQ_NO_ROOM, /* sparse map: nothing behind the square */
	KSQ_OFF_GRID, /* would leave the map zone */
	KSQ_BAD_SECTOR, /* impassable / water / not settleable */
	KSQ_UNDERDARK, /* absolute ban, ruled 2026-08-28 */
	KSQ_NEAR_HOMETOWN,
	KSQ_NEAR_ENTRANCE,
	KSQ_OTHER_REALM, /* inside another realm's footprint */
	KSQ_HAS_GUILDHALL
};

const char *kingdom_verdict_text(int verdict);

/* Judge one square of a prospective realm anchored at hall_rnum.
 * `ignore_assoc` excludes a realm from the overlap test so a realm does not
 * collide with itself. */
int kingdom_judge_square(int hall_rnum, int index, int racewar, int ignore_assoc);

/* Judge the whole 80-square footprint. Returns KSQ_OK when every square
 * passes; otherwise the first failing verdict, with *bad_index set. */
int kingdom_judge_footprint(int hall_rnum, int racewar, int ignore_assoc, int *bad_index);

/* Say where the trouble is, in words a player can act on: which way the
 * refused square lies from the hall and, for the keep-away verdicts, which way
 * the offending gateway lies, how far off it is and how far a realm needs.
 * Writes a fragment with no leading capital and no trailing stop, so a caller
 * can set it inside its own sentence, and writes nothing when it has nothing
 * useful to add. Called only on the refusal path, so it re-derives what it
 * needs rather than burdening kingdom_judge_square()'s seam with out-params. */
void kingdom_explain_refusal(int hall_rnum, int index, int verdict, char *out, size_t out_len);

/* `kingdom prospect`. Judge the actor's own square as a realm seat and, when
 * it will not serve, name the nearest squares that would. Open to anyone --
 * no guild, no realm, no rank -- because it is the verb someone uses to decide
 * whether founding one is worth it. */
void kingdom_prospect(struct char_data *ch);

/* ------------------------------------------------------------------ *
 * Cross-file surface within src/kingdom/
 * ------------------------------------------------------------------ *
 * Every function one kingdom file calls in another is declared HERE. Callers
 * must not forward-declare a sibling locally: a local `extern` is a second
 * copy of the signature that drifts silently when the definition changes,
 * and the compiler cannot catch the mismatch across translation units.
 *
 * These carry no file:line references on purpose. Three lanes reported their
 * own line citations had already gone stale during authoring; a line number
 * in a comment is a second copy of a moving target.
 */

/* --- kingdom.c : the module core --- */
/* The guild's MAIN hall, or NULL: matched on the persisted assoc_id, never on
 * gh->guild (the loader leaves that NULL when the association is gone), and
 * GH_TYPE_MAIN only, because an outpost can never anchor a realm. THE ONE
 * finder -- three copies once disagreed about the same hall. */
Guildhall *kingdom_main_hall(int assoc_id);

/* --- kingdom_claim.c : conversion and the claim/abandon state machine --- */
long kingdom_claim_cost(int index);
/* Units of EVERY resource claim `index` costs on top of the coin, on the same
 * compounding curve. Every square wants all four kinds. */
long kingdom_claim_material_cost(int index);
long kingdom_ring_cost(int ring);
bool kingdom_convert_guild(struct char_data *ch);
bool kingdom_claim_next(struct char_data *ch);
bool kingdom_abandon_last(struct char_data *ch);

/* --- kingdom_upkeep.c : the periodic charge and the arrears ladder --- */
long kingdom_upkeep_due(const kingdom_realm &realm);
void kingdom_apply_arrears(kingdom_realm &realm);
void kingdom_clear_arrears(kingdom_realm &realm);
/* Make a money-bearing change durable as ONE unit: the guild whose treasury
 * moved and the realm record that explains why. One SQL transaction under
 * MariaDB; one recovery-journal transaction under the flat-file build. False marks
 * the realm payment_pending for kingdom_upkeep_retry_pending() and never
 * lets the generic flush publish the record alone. Used by upkeep, claim
 * and convert -- any path that debits a treasury and records the result. */
bool kingdom_persist_payment(Guild *guild, kingdom_realm &realm);
/* Retry every pending pair; called at the top of each sweep, before the
 * shutdown flush and before the copyover flush. */
void kingdom_upkeep_retry_pending(void);
/* A guild is going away: forget any pending retry keyed on its id, so a
 * reused association id never inherits it. */
void kingdom_upkeep_forget_guild(int assoc_id);
/* Drop all upkeep module state (pending lists, cycle stamp). Shutdown only. */
void kingdom_upkeep_reset(void);

/* --- kingdom_guards.c : the garrison --- */
int kingdom_guard_allowance(const kingdom_realm &realm);
int kingdom_guard_garrison(const kingdom_realm &realm);
int kingdom_guards_count(int assoc_id);
int kingdom_guards_despawn(int assoc_id);
int kingdom_guards_despawn_all(void);
int kingdom_guards_refresh(const kingdom_realm &realm);
/* Guards AND champion. The entry point every caller outside kingdom_guards.c
 * uses, so that nothing has to remember there are two things to reconcile. */
int kingdom_garrison_refresh(const kingdom_realm &realm);
void kingdom_guards_refresh_all(void);
/* Bind the patrol proc to the guard prototype. Without it guards stand where
 * they were posted, which is the state the pre-2026-09-04 module shipped.
 * Self-gates on the prototype being present. */
void kingdom_guards_bind_proc(void);

/* --- the roster (kingdom_guards.c) --- */
/* Guards on the books, empty slots skipped. */
int kingdom_roster_count(const kingdom_realm &realm);
/* The highest level this realm's completed rings entitle a guard to. */
int kingdom_guard_level_cap(const kingdom_realm &realm);
/* The next promotion rung after `level`, or 0 at/outside the ladder. */
int kingdom_guard_next_level(int level);
/* Copper to promote one guard from `from` to `to`; 0 for a non-promotion. */
long kingdom_guard_promotion_cost(int from, int to);
/* The CLASS_* bit for a class name a player typed, or 0 for one no guard may
 * take. The list is deliberately short -- see kingdom_guard_classes. */
int kingdom_guard_class_by_name(const char *name);
/* Display name of a guard class bit, or "unschooled" for 0. */
const char *kingdom_guard_class_name(int guard_class);
/* Every offerable class, comma-separated, into the caller's buffer. */
void kingdom_guard_class_list(char *out, size_t out_len);
/* Stand the realm's champion up, or take it away. Idempotent, like the guards'
 * refresh; returns 1 when a champion is standing afterwards. */
int kingdom_champion_refresh(const kingdom_realm &realm);
/* What a champion costs, in copper. Ruled 2026-09-04: 25,000 platinum, and a
 * fixed price rather than a config knob -- there is exactly one per realm and
 * only a complete realm may have it. */
#define KINGDOM_CHAMPION_COST 25000000L

/* --- the roster verbs (kingdom_claim.c) --- *
 * They live beside claim and convert rather than beside the spawner because
 * they are the same KIND of thing: leader-only acts that spend the treasury
 * and must persist the realm and the guild as one paired write. The spawner
 * knows how to stand a guard up; it has no business knowing how one is paid
 * for. */
void kingdom_roster_show(struct char_data *ch);
void kingdom_roster_hire(struct char_data *ch, char *rest);
void kingdom_roster_upgrade(struct char_data *ch, char *rest);
void kingdom_roster_champion(struct char_data *ch, char *rest);

/* --- kingdom_harvest.c : world harvest nodes and the realm resource store --- */
bool kingdom_nodes_dormant(const kingdom_realm &realm);
long kingdom_resource_deposit(kingdom_realm &realm, int res, long amount);
/* Take costs[res] of every resource, all or nothing. False -- and nothing
 * moved -- when the realm is short of any one of them, or when the bill is
 * empty. The store's only outward path. */
bool kingdom_resource_spend(kingdom_realm &realm, const long costs[KRES_MAX]);
void kingdom_harvest_command(struct char_data *ch, char *argument);
void kingdom_harvest_survey(struct char_data *ch);
void kingdom_harvest_prune(const kingdom_realm &realm);
void kingdom_harvest_release(int assoc_id);
/* Extract every node in one room that has expired or now stands on land a
 * realm controls. A claim calls it for the square it just took, so a node
 * enclosed by the new border is gone at once rather than on the next sweep. */
void kingdom_node_reap_room(int rnum);
/* Bind the node prototypes to the spec proc, resolve the spawn regions and
 * start the reload sweep. Called from kingdom_initialize(); self-gates on
 * kingdom_enabled(), so it is safe to call unconditionally. */
void kingdom_harvest_initialize(void);
void kingdom_harvest_shutdown(void);

/* --- kingdom_display.c : status, map overlay and the 9x9 ring grid --- *
 * kingdom_show_grid() takes a bare anchor rather than a realm so a prospective
 * site can be surveyed before any realm exists; pass highest_claim 0 then. */
void kingdom_show_status(struct char_data *ch, const kingdom_realm &realm);
void kingdom_show_map(struct char_data *ch, const kingdom_realm &realm);
void kingdom_show_grid(struct char_data *ch, int hall_rnum, int racewar, int ignore_assoc,
		       int highest_claim, const char *heading);

/* ------------------------------------------------------------------ *
 * Persistence (kingdom_db.c, both backends behind __NO_MYSQL__)
 * ------------------------------------------------------------------ *
 * Dual backend on purpose: under the default mariadb build
 * persistence_mode_flatfile_root() returns NULL and no flat-file directories
 * are provisioned, so a flat-file-only store would silently not persist in
 * production.
 */
bool kingdom_db_load_all(void);
bool kingdom_db_save_realm(const kingdom_realm &realm);
/* Publish the garrison roster. Called by kingdom_db_save_realm(), so no caller
 * outside kingdom_db.c needs it; declared because the flat-file backend
 * defines its own and the two must agree. */
bool kingdom_db_save_roster(const kingdom_realm &realm);
#ifdef __NO_MYSQL__
/* Recoverably commit a flat guild debit and its paid realm after-image. */
bool kingdom_db_save_payment_pair(const std::string &root,
				  const flatfile_association_record &association,
				  const kingdom_realm &realm, std::string *error);
#endif
bool kingdom_db_delete_realm(int assoc_id);
void kingdom_db_flush_dirty(void);

#endif /* _KINGDOM_INTERNAL_H_ */
