/*
 *  kingdom_guards.c
 *  Duris
 *
 *  The realm's garrison: how many guards a territory entitles it to, where
 *  they stand, and the rung of the arrears ladder that takes them away and
 *  gives them back.
 *
 *  THE GARRISON IS DERIVED, NEVER STORED
 *  -------------------------------------
 *  Nothing in this file is persisted and no guard pointer outlives the call
 *  that made it. The realm record already says everything needed to rebuild
 *  the garrison from nothing -- highest_claim gives the land, hall_rnum gives
 *  the anchor, arrears says whether guards are permitted at all -- so the
 *  whole module is one idempotent reconciliation:
 *
 *      kingdom_guards_refresh(realm)
 *          work out the posts the realm should man RIGHT NOW,
 *          keep the guards already standing on them,
 *          extract the rest, spawn into the gaps.
 *
 *  That is also the answer to duplicates across a restart, but the two kinds
 *  of restart get there differently, and an earlier draft of this banner got
 *  the second one wrong. A COLD BOOT holds no guards -- no zone file loads
 *  them -- so the first refresh simply spawns the allowance. A COPYOVER is the
 *  opposite: copyover_save() writes EVERY living NPC to copyover.dat, guards
 *  included (write_mob_entry(), persistence/copyover.c:183-245), and
 *  copyover_recover() re-spawns each one via read_mobile() and copies back
 *  room, hp, gold and birthplace (persistence/copyover.c:1061-1199). What it
 *  does NOT copy back is this module's realm stamp: struct copyover_mob has no
 *  field for the npc value[] slots, and read_mobile() zeroes them on load
 *  (world/db.c:1970-1973), so last cycle's garrison comes back stamped 0 --
 *  guards of no realm -- while the next refresh musters the allowance afresh
 *  beside it. The orphan sweep in kingdom_guards_refresh_all() is what removes
 *  them: it despawns every guard-prototype mob whose stamp does not resolve to
 *  a live realm, stamp 0 explicitly included. Note the boot order in
 *  net/comm.c's boot sequence: kingdom_initialize() -- and therefore its
 *  refresh_all -- runs BEFORE copyover_recover() restores the old roster, so
 *  the sweep that clears it is the first refresh_all AFTER the recovery, not
 *  the boot-time one. If this file instead wrote its own roster down, every
 *  restart would either resurrect a stale roster on top of the fresh one or
 *  need this same reconciliation anyway -- so it keeps only the reconciliation
 *  and throws the roster away.
 *
 *  THE LADDER (ruled 2026-08-28, RULINGS.md "6 -- UPKEEP DEGRADATION")
 *  ------------------------------------------------------------------
 *  kingdom_upkeep_event() moves `arrears` and does nothing else; its banner
 *  "THE LADDER IS STATE, NOT AN ERRAND" names kingdom_guard_allowance() as the
 *  place rung 1 is applied, so that is where it is applied -- once, in the
 *  function every caller already asks -- rather than privately inside the
 *  spawner where another caller could miss it.
 *
 *  CALLS THAT CAN FREE THEIR ARGUMENT
 *  ----------------------------------
 *  char_to_room() (world/handler.c:1119) and extract_char() (world/handler.c:4665)
 *  are both in that family: char_to_room() extracts an NPC outright when the
 *  target room is negative (world/handler.c:1136-1145), and extract_char()
 *  calls free_char() before it returns (world/handler.c:4994-5002). So every
 *  spawn here branches on char_to_room()'s RETURN VALUE and never on the
 *  character's state afterwards, and every despawn re-tests char_in_list()
 *  immediately before extracting -- the same guard char_to_room() itself uses
 *  after running a room proc (world/handler.c:1402).
 *
 *  Re-audited 2026-09-01, line by line through world/handler.c:1119-1406, and
 *  the answer is narrower than "it can free anything". Of char_to_room()'s
 *  FALSE returns only ONE frees the argument:
 *
 *    !IS_ALIVE(ch)        (:1131-1134) returns FALSE, frees nothing
 *    room < 0 && IS_NPC   (:1136-1145) extract_char()s and returns FALSE
 *                                      -- THE ONE FREEING PATH
 *    already in a room    (:1154-1160) returns FALSE, frees nothing
 *    room proc / sector   (:1397-1406 and below) FREE, but are UNREACHABLE
 *                                      from here: KINGDOM_GUARD_ENTRY_DIR is
 *                                      negative, so :1375-1378 has already
 *                                      returned TRUE before any of them run.
 *
 *  So the freeing path is guarded twice over -- kingdom_guard_valid_rnum()
 *  refuses a negative rnum before the call, and the failure branch still
 *  consults char_in_list() rather than the character -- and the post-call
 *  handling below is correct as written. char_in_list() (core/utility.c:568)
 *  only COMPARES the pointer against character_list and never dereferences it,
 *  which is what makes it safe to ask about a pointer that may already be
 *  freed; nothing is allocated between the free and the question, so the
 *  address cannot have been recycled under it either.
 *
 *  Sibling files inside src/kingdom/ are cited by FUNCTION NAME rather than by
 *  line, because they are being written alongside this one and a line number
 *  into them would be stale before it was read. Engine files outside this
 *  directory are stable and are cited file:line.
 */

#include "kingdom/kingdom_internal.h"

#include "core/structs.h"

#include "cmd/interp.h"
#include "core/config.h"
#include "core/prototypes.h"
#include "core/utility.h"
#include "core/utils.h"
#include "guild/assocs.h"
#include "kingdom/kingdom_geometry.h"
#include "net/comm.h"
#include "world/db.h"

#include <cstddef>
#include <string>
#include <vector>

/* Not declared in any header the engine exports; kingdom_geometry.c and
 * kingdom_claim.c take the same externs for the same reason. */
extern struct room_data *world;
extern int top_of_world;
extern P_char character_list;
extern P_obj object_list;
extern P_index mob_index;
extern P_index obj_index;

/* ------------------------------------------------------------------ *
 * The guard prototype
 * ------------------------------------------------------------------ */

/*
 * The mob a kingdom guard is loaded from: VMOB_KINGDOM_GUARD, declared in
 * kingdom_internal.h beside the harvest-node object vnums.
 *
 * IT IS NOT DEFINED HERE, AND THAT IS THE POINT. This file used to carry its
 * own #define of 97810 on the reasoning that the number sat inside the
 * builders' subsystem band in world/buildings.h and was unspent there. The
 * reasoning was sound and the number was still wrong, because "unspent in a
 * header's comment block" is not the same fact as "present in a .mob file",
 * and 97810 was in NO .mob file at all. read_mobile(97810, VIRTUAL) therefore
 * resolved real_mobile() to -1 and returned 0 (world/db.c:1930-1937), so every
 * spawn failed and the garrison never appeared -- silently, because that
 * particular log line is compiled out (see kingdom_guard_spawn_one()).
 *
 * The vnum now lives in the header because the .mob record and this loader
 * have to agree and one definition is the only way to keep them in step. It is
 * still deliberately NOT a tuning value: struct kingdom_config is frozen for
 * this change and has no field for a vnum, and a second reader of
 * lib/kingdom.cfg opened here would be a second place for the file's meaning
 * to drift.
 *
 * When the prototype is absent from the world the module simply fields no
 * guards -- the same state an operator produces with
 * kingdom.guards.per.squares set above 80 -- and says so once rather than once
 * per realm per upkeep cycle. That fallback is what MASKED the bad vnum, so it
 * is worth being explicit that it is a safety net and not a test: only
 * kingdom_guard_rnum() answering >= 0 against a real heavens.mob record proves
 * the number is right.
 */

/*
 * Which npc value slot carries the owning association id.
 *
 * read_mobile() zeroes every value[] slot on load (world/db.c:1970-1972), and
 * only the transport mobs otherwise claim slots, on their own prototypes
 * (world/transport.c:39-43), so slot 0 of a prototype this module owns
 * outright is free.
 *
 * This, not GET_ASSOC(), is what identifies a guard's realm. GET_ASSOC() is a
 * Guild POINTER (guild/assocs.h:18); reading it to decide whether a mob is
 * ours would dereference a freed Guild in exactly the case that matters most,
 * a guild being deleted. An int id cannot dangle.
 */
#define KINGDOM_GUARD_ASSOC_SLOT 0
#define KINGDOM_GUARD_ASSOC(ch) ((ch)->only.npc->value[KINGDOM_GUARD_ASSOC_SLOT])

/* Which ROSTER slot this body is, stored one-based so that 0 -- the value
 * read_mobile() leaves behind -- means "not a rostered guard" rather than
 * "slot 0". A guard promoted or re-classed is respawned rather than edited in
 * place, and this is how the reconciler tells which body belongs to which
 * roster line before it decides. */
#define KINGDOM_GUARD_ROSTER_SLOT 1
#define KINGDOM_GUARD_ROSTER(ch) ((ch)->only.npc->value[KINGDOM_GUARD_ROSTER_SLOT])

/* Which npc slot the CHAMPION counts rounds in while it waits to raise another
 * banner. Slots 0 and 1 mean on a champion exactly what they mean on a guard,
 * so this is the first free one. */
#define KINGDOM_CHAMPION_COUNT_SLOT 2
#define KINGDOM_CHAMPION_COUNT(ch) ((ch)->only.npc->value[KINGDOM_CHAMPION_COUNT_SLOT])

/* Which obj value slots a banner carries. An object takes its value[] straight
 * from the prototype, so slot 0 must be stamped before the banner is planted
 * or it would belong to realm 0 and tick for nobody. */
#define KINGDOM_BANNER_ASSOC_SLOT 0
#define KINGDOM_BANNER_HP_SLOT 1
#define KINGDOM_BANNER_ASSOC(obj) ((obj)->value[KINGDOM_BANNER_ASSOC_SLOT])
#define KINGDOM_BANNER_HP(obj) ((obj)->value[KINGDOM_BANNER_HP_SLOT])

/* What a banner can take before it falls, and how long the champion waits
 * before raising another. Five rounds, as ruled; a round here is one mundane
 * mob pulse, which is what the champion's proc counts. */
#define KINGDOM_BANNER_HITPOINTS 2500
#define KINGDOM_BANNER_REPLANT_ROUNDS 5

/* Take every banner of one realm out of the world, or every banner there is
 * when assoc_id is 0. Defined with the banner code, called by the despawn
 * paths above it. */
static void kingdom_banners_of_realm_destroy(int assoc_id);
/* The two procs, defined below the reconciler that binds them. */
int kingdom_champion_proc(P_char ch, P_char pl, int cmd, char *arg);
int kingdom_banner_proc(P_obj obj, P_char ch, int cmd, char *arg);

/* Placement flag for char_to_room(). Any negative dir skips the aggro and
 * room-proc block and returns TRUE from world/handler.c:1375-1378; -2 also
 * skips the do_look() at world/handler.c:1363, which a mob has no use for.
 * This is the value the zone reset's own mob loads pass (world/db.c:3440).
 * The return value is still the only safe post-condition, because the paths
 * above that point can extract the mob. */
#define KINGDOM_GUARD_ENTRY_DIR (-2)

/* Who the muster notices come from, matching the steward kingdom_upkeep.c
 * signs the treasury notices with. */
static const char KINGDOM_MARSHAL[] = "The Kingdom Marshal";

/* ------------------------------------------------------------------ *
 * Small helpers
 * ------------------------------------------------------------------ */

/* rnum 0 is rejected on purpose: real_room0() returns 0 both for the first
 * room and for "no such vnum" (world/db.c:4477-4508), and every caller here
 * means the latter. Same rule as kingdom_geometry.c's valid_rnum(). */
static bool kingdom_guard_valid_rnum(int rnum)
{
	return rnum > 0 && rnum <= top_of_world;
}

/* Association ids start at 1, and 0 is the module's "no owner" answer
 * (kingdom_owner_of_room(), kingdom/kingdom.h). */
static bool kingdom_guard_valid_assoc(int assoc_id)
{
	return assoc_id > 0;
}

/*
 * The guard prototype's rnum, or -1 when the world has no such mob.
 *
 * real_mobile() is used rather than real_mobile0() because real_mobile0()
 * answers 0 for an unknown vnum (world/db.c:4550-4577), and 0 is also the rnum
 * of the FIRST mob in the table -- the same sentinel collision real_room0()
 * has. real_mobile() answers -1 (world/db.c:4584-4611), which is unambiguous.
 *
 * mob_index starts NULL and is only allocated when the mob file is indexed
 * (world/db.c:153, 559), and real_mobile() dereferences it without checking,
 * so a call before the world is booted would be a null dereference. Once
 * built the table is never regenerated in-process, so the rnum is stable for
 * the life of the server and is safe to compare guards against.
 */
static int kingdom_guard_rnum(void)
{
	if (mob_index == NULL)
		return -1;

	return real_mobile(VMOB_KINGDOM_GUARD);
}

/* Champion prototype rnum, with the same pre-boot guard as the ordinary guard
 * lookup. Shutdown and a disabled feature can reach this before mob_index has
 * been built. */
static int kingdom_champion_rnum(void)
{
	if (mob_index == NULL)
		return -1;

	return real_mobile(VMOB_KINGDOM_CHAMPION);
}

/* True when this character is a kingdom guard of any realm. Tested in this
 * order on purpose: IS_NPC() first, because only.npc and only.pc share a union
 * (core/structs.h:1535-1539) and reading npc fields off a PC is a type
 * confusion, not merely a wrong answer. */
static bool kingdom_char_is_guard(P_char ch, int guard_rnum)
{
	if (ch == NULL || guard_rnum < 0)
		return false;
	if (!IS_NPC(ch))
		return false;
	if (ch->only.npc == NULL)
		return false;

	/*
	 * A nameless mob is never claimed, however well its R_num matches, because
	 * the only thing this module ever does with a guard it recognises is
	 * extract_char() it -- and extract_char() opens with
	 * `if (!(*ch->player.name))` (world/handler.c:4680), which dereferences a
	 * null name rather than testing for one.
	 *
	 * That mob is reachable: read_mobile() pushes the new character onto
	 * character_list (world/db.c:1960-1962) BEFORE it reads the strings, and
	 * its "Error with mob: No name" path returns NULL without unlinking
	 * (world/db.c:1983-1990), so a .mob file truncated mid-record leaves a
	 * half-built mob on the list carrying this very R_num and a stamp of 0.
	 * kingdom_guards_refresh_all()'s orphan sweep would find it -- assoc 0
	 * belongs to no realm -- and crash the server trying to tidy it away.
	 *
	 * Refusing to recognise it is the honest answer: this module can neither
	 * identify whose it is nor remove it safely, so it treats it as it treats
	 * every other broken mob in the world, which is not at all. It costs
	 * nothing on the normal path, where read_mobile() has either read the name
	 * or taken the shared mob_index[].keys pointer.
	 */
	if (ch->player.name == NULL || *ch->player.name == '\0')
		return false;

	/* R_num is compared directly rather than through GET_VNUM(), which
	 * indexes mob_index[R_num] and would read out of bounds for the R_num of
	 * -1 that extract_char() knows to expect (world/handler.c:4999). */
	return ch->only.npc->R_num == guard_rnum;
}

/* True when `ch` is a kingdom guard whose realm stamp is `assoc_id`. Every
 * per-realm walk of character_list asks this; the stamp is compared as an int
 * and GET_ASSOC()'s pointer is never read. */
static bool kingdom_char_is_guard_of(P_char ch, int guard_rnum, int assoc_id)
{
	if (!kingdom_char_is_guard(ch, guard_rnum))
		return false;

	return KINGDOM_GUARD_ASSOC(ch) == assoc_id;
}

/* The champion half of kingdom_char_is_guard(). It carries the same type and
 * name checks because every extraction path uses these predicates, and
 * extract_char() dereferences player.name. */
static bool kingdom_char_is_champion(P_char ch, int champion_rnum)
{
	if (ch == NULL || champion_rnum < 0)
		return false;
	if (!IS_NPC(ch) || ch->only.npc == NULL)
		return false;
	if (ch->player.name == NULL || *ch->player.name == '\0')
		return false;

	return ch->only.npc->R_num == champion_rnum;
}

static bool kingdom_char_is_champion_of(P_char ch, int champion_rnum, int assoc_id)
{
	return kingdom_char_is_champion(ch, champion_rnum) && KINGDOM_GUARD_ASSOC(ch) == assoc_id;
}

/* ------------------------------------------------------------------ *
 * How many guards the realm may field
 * ------------------------------------------------------------------ */

/*
 * How many guards this realm may have standing RIGHT NOW.
 *
 * Territory divided by kingdom.guards.per.squares -- and ZERO while unpaid
 * upkeep has reached rung 1 of the arrears ladder. The arrears test lives here
 * because kingdom_upkeep_event()'s ladder is state rather than an errand: it
 * moves `arrears` and expects this function to be the single place the rung is
 * read. Putting the test in the spawner instead would leave every other caller
 * free to field guards the realm has forfeited, and would need a second copy
 * of the test to stay in step.
 *
 * Both display call sites read correctly with the rung applied: `kingdom
 * guards` prints "may field up to 0 guard(s)" and follows it with "Unpaid
 * upkeep takes the guards first", and the status panel prints "0 permitted"
 * above a Standing line that says the guards have gone home.
 */
int kingdom_guard_allowance(const kingdom_realm &realm)
{
	int squares = realm.highest_claim;
	int per_squares;

	if (!kingdom_cfg.enabled)
		return 0;

	/* Rung 1 of the ladder. A value below KARR_CURRENT can only come from a
	 * corrupt row; kingdom_apply_arrears() normalises it on the next cycle,
	 * and reading it as "current" until then costs nothing. */
	if (realm.arrears >= KARR_GUARDS_GONE)
		return 0;

	if (squares <= 0)
		return 0; /* a realm that owns nothing garrisons nothing */
	if (squares > KINGDOM_MAX_SQUARES)
		squares = KINGDOM_MAX_SQUARES; /* clamp a corrupt persisted value */

	/* kingdom_config_load() already refuses a denominator below 1, precisely
	 * because this line divides by it. Repeated here because a realm read
	 * before the config was, or a range widened later, would otherwise
	 * divide by zero inside a display path. */
	per_squares = kingdom_cfg.guards_per_squares;
	if (per_squares < 1)
		per_squares = 1;

	int allowed = squares / per_squares;

	if (allowed > KINGDOM_GUARD_SLOTS)
		allowed = KINGDOM_GUARD_SLOTS;

	return allowed;
}

/* ------------------------------------------------------------------ *
 * The roster: who is on it, what they cost, how high they may rise
 * ------------------------------------------------------------------ */

/* Guards actually on the books, empty slots skipped. This -- not the
 * allowance -- is how many bodies the spawner stands up: the allowance is the
 * ceiling the land permits, and a realm that has bought nothing garrisons
 * nothing however much land it holds. */
int kingdom_roster_count(const kingdom_realm &realm)
{
	int count = 0;

	for (int slot = 0; slot < KINGDOM_GUARD_SLOTS; slot++)
		if (realm.guards[slot].level > 0)
			count++;

	return count;
}

/*
 * The highest level this realm's land entitles a guard to.
 *
 * The first completed ring raises the prototype's level-45 ceiling to 50;
 * later rings add two levels apiece. A realm part way into a ring gets the
 * tier of the last ring it FINISHED -- half a ring is not half a promotion,
 * and tying the tier to the ring boundary is what makes "complete ring 3 and
 * your garrison reaches 54" a sentence a player can act on.
 *
 * Note what this does NOT do: it never lowers a guard. A realm that loses ring
 * 4 to arrears gets a lower cap for its NEXT promotion and keeps every level
 * it has already paid for (ruled 2026-09-04, and enforced in
 * kingdom_guard_promote() rather than here).
 */
int kingdom_guard_level_cap(const kingdom_realm &realm)
{
	int rings = 0;

	for (int ring = 1; ring <= KINGDOM_MAX_RING; ring++)
		if (realm.highest_claim >= kingdom_ring_last_index(ring))
			rings = ring;

	if (rings <= 0)
		return KINGDOM_GUARD_BASE_LEVEL;

	const int level = KINGDOM_GUARD_FIRST_TIER_LEVEL + (rings - 1) * KINGDOM_GUARD_TIER_STEP;

	/* Ring 4 lands exactly on the top level; the clamp is here so a widened
	 * ring count later cannot quietly raise the ceiling. */
	return level > KINGDOM_GUARD_TOP_LEVEL ? KINGDOM_GUARD_TOP_LEVEL : level;
}

/* The next rung a bought guard can reach. The first promotion bridges the
 * prototype's level 45 to ring one's level 50; later rings add two levels.
 * Returning 0 at the top lets callers reject another promotion explicitly. */
int kingdom_guard_next_level(int level)
{
	if (level < KINGDOM_GUARD_BASE_LEVEL || level >= KINGDOM_GUARD_TOP_LEVEL)
		return 0;
	if (level < KINGDOM_GUARD_FIRST_TIER_LEVEL)
		return KINGDOM_GUARD_FIRST_TIER_LEVEL;

	for (int tier = KINGDOM_GUARD_FIRST_TIER_LEVEL + KINGDOM_GUARD_TIER_STEP;
	     tier <= KINGDOM_GUARD_TOP_LEVEL; tier += KINGDOM_GUARD_TIER_STEP)
		if (level < tier)
			return tier;

	return 0;
}

/*
 * What it costs, in copper, to take a guard from `from` to `to`.
 *
 * The whole span from the prototype's level to the top costs
 * (guard.cost.max - guard.cost.base), divided equally among the four
 * promotions, so a guard hired at the base price and promoted all the way has
 * cost exactly guard.cost.max and no arithmetic anywhere else has to know
 * that. Returns 0 for a non-promotion, which the caller reads as "nothing to
 * buy" rather than "free".
 */
long kingdom_guard_promotion_cost(int from, int to)
{
	if (to <= from || to > KINGDOM_GUARD_TOP_LEVEL || from < KINGDOM_GUARD_BASE_LEVEL)
		return 0;

	long span = kingdom_cfg.guard_cost_max - kingdom_cfg.guard_cost_base;

	if (span < 0)
		span = 0;

	const int from_tier =
		from < KINGDOM_GUARD_FIRST_TIER_LEVEL ?
			0 :
			(from - KINGDOM_GUARD_FIRST_TIER_LEVEL) / KINGDOM_GUARD_TIER_STEP + 1;
	int to_tier = to < KINGDOM_GUARD_FIRST_TIER_LEVEL ?
			      0 :
			      (to - KINGDOM_GUARD_FIRST_TIER_LEVEL) / KINGDOM_GUARD_TIER_STEP + 1;

	if (to_tier > KINGDOM_GUARD_TIERS)
		to_tier = KINGDOM_GUARD_TIERS;

	/* Difference between cumulative prices preserves any division remainder,
	 * so four promotions plus the hire price reach guard.cost.max exactly. */
	return span * to_tier / KINGDOM_GUARD_TIERS - span * from_tier / KINGDOM_GUARD_TIERS;
}

/*
 * THE CLASSES A GUARD MAY BE.
 *
 * A short, curated list, and short on purpose. The engine carries twenty-nine
 * classes, most of which mean nothing on a mob standing a border watch -- a
 * guard bard is a joke, a guard alchemist is a puzzle -- and every one offered
 * here is a fighting or casting archetype that reads immediately as a
 * garrison. A guild picks from these when it hires and again when it promotes,
 * which is the whole of the "choose the class" ruling; nothing else in the
 * module cares what the answer was except the mob's own class-driven combat.
 *
 * Order is the order `kingdom roster` lists them in, so it runs martial first,
 * then divine, then arcane, then the skirmishers.
 */
static const struct
{
	const char *name;
	int bit;
} kingdom_guard_classes[] = {
	{ "warrior", CLASS_WARRIOR },
	{ "paladin", CLASS_PALADIN },
	{ "antipaladin", CLASS_ANTIPALADIN },
	{ "berserker", CLASS_BERSERKER },
	{ "mercenary", CLASS_MERCENARY },
	{ "ranger", CLASS_RANGER },
	{ "cleric", CLASS_CLERIC },
	{ "shaman", CLASS_SHAMAN },
	{ "druid", CLASS_DRUID },
	{ "sorcerer", CLASS_SORCERER },
	{ "necromancer", CLASS_NECROMANCER },
	{ "rogue", CLASS_ROGUE },
};

constexpr int kingdom_guard_class_count =
	(int)(sizeof(kingdom_guard_classes) / sizeof(kingdom_guard_classes[0]));

int kingdom_guard_class_by_name(const char *name)
{
	if (!name || !*name)
		return 0;

	for (int i = 0; i < kingdom_guard_class_count; i++)
		if (is_abbrev(name, kingdom_guard_classes[i].name))
			return kingdom_guard_classes[i].bit;

	return 0;
}

const char *kingdom_guard_class_name(int guard_class)
{
	for (int i = 0; i < kingdom_guard_class_count; i++)
		if (kingdom_guard_classes[i].bit == guard_class)
			return kingdom_guard_classes[i].name;

	return "unschooled";
}

/* The offerable list, comma-separated, for a refusal that has to say what the
 * caller could have typed. Written into the caller's buffer rather than
 * returned as a static, so two refusals can be built at once. */
void kingdom_guard_class_list(char *out, size_t out_len)
{
	if (!out || out_len == 0)
		return;

	out[0] = '\0';

	for (int i = 0; i < kingdom_guard_class_count; i++)
	{
		const size_t used = strlen(out);

		if (used + 2 >= out_len)
			return;

		snprintf(out + used, out_len - used, "%s%s", i ? ", " : "",
			 kingdom_guard_classes[i].name);
	}
}

/*
 * The garrison the spawner should try to stand up.
 *
 * The allowance already applies the feature switch and the ladder, so the only
 * thing left is whether there is an anchor to place guards against. A realm
 * whose guildhall was destroyed keeps its land on paper but has no hall square
 * to measure claims from (kingdom_on_guildhall_changed() clears the anchor),
 * and hall_rnum 0 is that unresolved sentinel.
 *
 * kingdom_guard_posts() would in fact find nothing for such a realm anyway --
 * kingdom_room_for_claim() cannot resolve a claim against rnum 0 -- but that
 * is 80 wasted lookups per dormant realm per cycle, and relying on it would
 * make the dormancy rule an accident of another module's validation instead of
 * a stated one.
 */
int kingdom_guard_garrison(const kingdom_realm &realm)
{
	if (!kingdom_guard_valid_rnum(realm.hall_rnum))
		return 0;

	/* THE LOWER OF WHAT THE LAND PERMITS AND WHAT THE GUILD HAS BOUGHT.
	 * Before 2026-09-04 the allowance alone stood the garrison up, so land
	 * conjured guards; now land only sets the ceiling. A realm that claims
	 * its way to eighty squares and buys nothing fields nothing, and a realm
	 * that bought sixteen and then lost land to arrears fields as many as
	 * its remaining ground allows -- it keeps the rest on the books, and
	 * they return when the land does. */
	const int permitted = kingdom_guard_allowance(realm);
	const int bought = kingdom_roster_count(realm);

	return permitted < bought ? permitted : bought;
}

/* ------------------------------------------------------------------ *
 * Where the guards stand
 * ------------------------------------------------------------------ */

/*
 * Fill `posts` with the room rnums this realm should man, and return how many
 * were found.
 *
 * The posts are spread evenly through the claim order rather than bunched on
 * the outer ring, because the claim order is a clockwise spiral: an even
 * spread over 1..highest_claim puts guards on every ring the realm holds and,
 * within a ring, at regular compass intervals. Ring 4 alone would leave the
 * squares nearest the hall unwatched.
 *
 * A square with no room behind it is ordinary rather than exceptional: map
 * zones are sparse, and while kingdom_footprint_check() proves all 80 squares
 * were eligible when the hall was sited, the world can be rebuilt underneath a
 * realm afterwards. So each post walks forward through the owned claims until
 * a room resolves, and a post that finds nothing at all is simply not manned.
 */
static int kingdom_guard_posts(const kingdom_realm &realm, int wanted, int *posts, int max_posts)
{
	int territory = realm.highest_claim;
	int found = 0;

	if (posts == NULL || wanted <= 0 || territory <= 0)
		return 0;
	if (territory > KINGDOM_MAX_SQUARES)
		territory = KINGDOM_MAX_SQUARES;
	if (wanted > territory)
		wanted = territory; /* never two guards on one square */
	if (wanted > max_posts)
		wanted = max_posts;

	for (int slot = 0; slot < wanted; slot++)
	{
		/* Claim indices are 1-based, so the first post is claim 1 and the
		 * rest are spaced territory/wanted apart. */
		const int first = 1 + (slot * territory) / wanted;

		for (int step = 0; step < territory; step++)
		{
			int index = first + step;
			if (index > territory)
				index -= territory; /* wrap inside owned land only */

			/* kingdom_room_for_claim() refuses to leave the map zone,
			 * where calculate_relative_room() would wrap toroidally to
			 * the far side of the continent (world/map.c:262-270). */
			const int rnum = kingdom_room_for_claim(realm.hall_rnum, index);
			if (!kingdom_guard_valid_rnum(rnum))
				continue;

			bool taken = false;
			for (int seen = 0; seen < found; seen++)
			{
				if (posts[seen] == rnum)
				{
					taken = true;
					break;
				}
			}
			if (taken)
				continue;

			posts[found] = rnum;
			found++;
			break;
		}
	}

	return found;
}

/* ------------------------------------------------------------------ *
 * The standing loadout
 * ------------------------------------------------------------------ */

/*
 * Every kingdom guard carries the same kit, and it is applied HERE rather than
 * written into areas/mob/heavens.mob, for one reason: the .mob record is world
 * data an area builder may edit, and the garrison's competence is a rule of
 * the kingdom code, not a property of one mob file. A guard that lost its
 * detects to a stray area edit would be a bug nobody could see until a thief
 * walked through a realm unchallenged.
 *
 * Ruled 2026-09-04. The list is deliberately defensive rather than lethal --
 * these are watchmen, not a raid boss:
 *
 *   seeing      detect invis, sense life, infravision, ultravision, farsee,
 *               aware. A guard that can be walked past is not a guard.
 *   surviving   globe of invulnerability, stoneskin, haste, immune to
 *               paralysis, immune to fear, break charm, no summon. Every one
 *               of these closes a way to neutralise the garrison without
 *               fighting it -- charm it, hold it, fear it, or yank it off the
 *               land entirely.
 *   moving      fly and swim, in both the act flag the mover reads and the
 *               affect other code asks about, so water and chasm squares are
 *               patrolled like any other.
 *   keeping     scavenger and memory: it picks up what is dropped on its
 *               ground and it remembers who it fought.
 *   fighting    protector, and aggression toward the OPPOSING racewar side,
 *               set the way the guildhall golems set it
 *               (guild/guildhall_rooms.c:238-246). A neutral or undead realm
 *               gets no racewar aggression, because there is no single side
 *               for it to be against.
 *
 * The champion adds its own on top; see kingdom_champion_outfit().
 */
static void kingdom_guard_outfit(P_char mob, P_Guild guild)
{
	if (!mob || !mob->only.npc)
		return;

	SET_BIT(mob->specials.act, ACT_SCAVENGER | ACT_MEMORY | ACT_IMMUNE_TO_PARA | ACT_NO_SUMMON |
					   ACT_CANFLY | ACT_CANSWIM | ACT_ELITE | ACT_BREAK_CHARM |
					   ACT_PROTECTOR);

	SET_BIT(mob->specials.affected_by, AFF_FARSEE | AFF_DETECT_INVISIBLE | AFF_HASTE |
						   AFF_SENSE_LIFE | AFF_STONE_SKIN |
						   AFF_INFRAVISION | AFF_FLY | AFF_AWARE);
	SET_BIT(mob->specials.affected_by2, AFF2_ULTRAVISION | AFF2_GLOBE);
	SET_BIT(mob->specials.affected_by3, AFF3_SWIMMING);
	SET_BIT(mob->specials.affected_by4, AFF4_NOFEAR);

	/* The realm's own side, so `consider` and every racewar test in the
	 * server reads the guard as belonging to the guild that raised it. The
	 * guild comes from the caller, which already holds it -- a second lookup
	 * here could disagree with the pointer the spawn is about to stamp on. */
	const int racewar = guild ? (int)guild->get_racewar() : RACEWAR_NONE;

	mob->player.racewar = (sh_int)racewar;

	if (racewar == RACEWAR_GOOD)
		SET_BIT(mob->only.npc->aggro_flags, AGGR_EVIL_RACE);
	else if (racewar == RACEWAR_EVIL)
		SET_BIT(mob->only.npc->aggro_flags, AGGR_GOOD_RACE);
}

/*
 * Put a roster line onto a body: its class, its level, and the stats that
 * follow from the level.
 *
 * HP SCALES PROPORTIONALLY rather than by a table of numbers written here.
 * The prototype in heavens.mob is the balance point an area builder tuned, and
 * a level-56 guard should be that same mob eleven levels stronger -- not a
 * figure this file invented and would then own forever. So the prototype's own
 * hit points are multiplied by level/45, and hitroll and damroll gain one
 * point per level above the base. Change the .mob record and every rank moves
 * with it, which is the property worth having.
 *
 * The formula, for a designer reading only this comment:
 *
 *     hp      = prototype hp * level / 45
 *     hitroll = prototype hitroll + (level - 45)
 *     damroll = prototype damroll + (level - 45)
 */
static void kingdom_guard_apply_rank(P_char mob, const kingdom_guard_slot &rank)
{
	if (!mob || rank.level <= 0)
		return;

	const int level = rank.level < KINGDOM_GUARD_BASE_LEVEL ? KINGDOM_GUARD_BASE_LEVEL :
								  rank.level;
	const int over = level - KINGDOM_GUARD_BASE_LEVEL;

	if (rank.guard_class)
		mob->player.m_class = static_cast<unsigned int>(rank.guard_class);

	mob->player.level = static_cast<::byte>(level);

	const long scaled = (long)GET_MAX_HIT(mob) * level / KINGDOM_GUARD_BASE_LEVEL;

	GET_MAX_HIT(mob) = static_cast<int>(scaled);
	GET_HIT(mob) = GET_MAX_HIT(mob);
	mob->points.base_hit = GET_MAX_HIT(mob);

	GET_HITROLL(mob) = static_cast<sh_int>(GET_HITROLL(mob) + over);
	GET_DAMROLL(mob) = static_cast<sh_int>(GET_DAMROLL(mob) + over);
}

/*
 * What the champion carries beyond a guard's kit. Ruled 2026-09-04.
 *
 * Every one of these is a survivability affect rather than a damage one, and
 * that is the shape of the thing: the champion's job is to stay standing long
 * enough for its banner to matter, and an attacking party's job is to get past
 * it. Inertial barrier and blur blunt what lands, deflection and the greater
 * spheres turn away what is thrown, flurry is the one concession to offence,
 * and the multiclass bit is what lets a mob answer to two callings at once --
 * the guild names both when it raises one.
 */
static void kingdom_champion_outfit(P_char mob, const kingdom_realm &realm)
{
	if (!mob || !mob->only.npc)
		return;

	SET_BIT(mob->specials.affected_by2, AFF2_FLURRY);
	SET_BIT(mob->specials.affected_by3, AFF3_INERTIAL_BARRIER | AFF3_BLUR);
	SET_BIT(mob->specials.affected_by4,
		AFF4_DEFLECT | AFF4_STORNOGS_GREATER_SPHERES | AFF4_MULTI_CLASS);

	if (realm.champion_class)
		mob->player.m_class = static_cast<unsigned int>(realm.champion_class);

	mob->player.level = static_cast<::byte>(KINGDOM_CHAMPION_LEVEL);
}

/* ------------------------------------------------------------------ *
 * Spawning and despawning
 * ------------------------------------------------------------------ */

/*
 * Load one guard and post it. Returns true only when the guard is standing in
 * `rnum` afterwards.
 *
 * Deliberately returns bool rather than the character: nothing in this module
 * is allowed to keep a guard pointer, because the next thing the engine does
 * to that mob may free it.
 */
static bool kingdom_guard_spawn_one(const kingdom_realm &realm, int rnum, P_Guild guild, int slot)
{
	P_char mob;

	if (!kingdom_guard_valid_rnum(rnum))
		return false;

	if (slot < 0 || slot >= KINGDOM_GUARD_SLOTS || realm.guards[slot].level <= 0)
		return false;

	mob = read_mobile(VMOB_KINGDOM_GUARD, VIRTUAL);
	if (mob == NULL)
	{
		/*
		 * Not logged here, but only after checking that read_mobile() really
		 * does log every failure this call site can reach. It has four NULL
		 * returns (world/db.c:1930-2086):
		 *
		 *   vnum not in the database   SILENT. The log line is wrapped in
		 *                              `#if defined(DB_NOTIFY) && DB_NOTIFY`
		 *                              and core/config.h:39 reads
		 *                              `#define DB_NOTIFYoff` -- a missing
		 *                              space, so DB_NOTIFY is undefined and
		 *                              that line is compiled out of the whole
		 *                              server. This is what made the old
		 *                              97810 fail without a word. It cannot
		 *                              be reached from here: the caller has
		 *                              already proved the rnum resolves.
		 *   no only.npc struct         wizlog(56) + logit(LOG_DEBUG)
		 *   no name in the record      wizlog(56)
		 *   malformed stat line        logit(LOG_DEBUG), and extract_char()s
		 *                              the half-built mob itself
		 *
		 * So the three reachable failures all speak, and a fourth line from
		 * this module would only repeat them once per unmanned post per
		 * upkeep cycle.
		 */
		return false;
	}

	/* The realm stamp goes on before anything can move the mob, so a guard
	 * is identifiable even if the very next call extracts it. */
	KINGDOM_GUARD_ASSOC(mob) = realm.assoc_id;
	KINGDOM_GUARD_ROSTER(mob) = slot + 1;

	kingdom_guard_apply_rank(mob, realm.guards[slot]);

	/* The engine's own convention for a guild-owned NPC: Guild::update()
	 * walks character_list and skips NPCs precisely because outpost and
	 * guildhall guards carry this (guild/assocs.c:3724-3728), and
	 * Building::load_gateguard() sets it the same way (world/buildings.c:707).
	 * A NULL guild is tolerated -- the stamp above is what this module reads,
	 * and the pointer is a courtesy to the rest of the server. */
	GET_ASSOC(mob) = guild;

	/* A GUARD PATROLS, IT DOES NOT STAND STILL -- ruled 2026-09-04 -- AND
	 * ACT_SENTINEL STAYS SET ANYWAY. Those two are not in tension, because
	 * the flag and the movement are read by different code:
	 *
	 *   MobCanGo() (mob/mobact.c) refuses a sentinel mob every direction, so
	 *   the ENGINE'S own wander cannot move a guard. That is exactly what is
	 *   wanted -- that wander knows about zones, not about realms, and would
	 *   walk a guard clean off the territory in a dozen steps.
	 *
	 *   do_move() (cmd/actmove.c) does not consult the flag at all, so
	 *   kingdom_guard_proc()'s patrol below moves the guard perfectly well
	 *   with the bit set, and it refuses any step off the realm's own
	 *   squares.
	 *
	 * So the containment is this module's own rule, enforced by this module's
	 * own code, rather than a side effect of some other subsystem's idea of
	 * where a mob may go. Clearing the flag would hand the engine back the
	 * ability to move these mobs and there is no version of that we want. */
	SET_BIT(mob->specials.act, ACT_SENTINEL);

	kingdom_guard_outfit(mob, guild);

	/* Its post is its home, exactly as the gateguards do
	 * (world/buildings.c:708-709), so anything that sends a mob home sends it
	 * back to the square it is meant to be watching. */
	mob->player.birthplace = world[rnum].number;
	mob->player.orig_birthplace = world[rnum].number;
	mob->player.hometown = world[rnum].number;

	if (!char_to_room(mob, rnum, KINGDOM_GUARD_ENTRY_DIR))
	{
		/*
		 * char_to_room() FAILED, and past this point `mob` may already be
		 * freed -- it extracts an NPC itself when the room is negative
		 * (world/handler.c:1136-1145). So the character's state is never
		 * consulted. char_in_list() is asked whether the pointer is still a
		 * live list member, which is the same test char_to_room() makes of
		 * its own argument after running a room proc (world/handler.c:1402),
		 * and only then is it extracted, so a roomless mob is not left on
		 * character_list.
		 */
		if (char_in_list(mob))
			extract_char(mob);
		return false;
	}

	return true;
}

/*
 * Extract every guard in `doomed`, skipping any that has already gone.
 *
 * Collected first and extracted second on purpose: extract_char() unlinks the
 * character from character_list and frees it (world/handler.c:4904-4925,
 * 4994-5002), so extracting during a walk of that list would follow a freed
 * ->next. It can also take other characters with it -- die_follower() and
 * clear_all_links() run inside it -- which is why every entry is re-tested
 * with char_in_list() rather than assumed live.
 */
static int kingdom_guard_extract_all(std::vector<P_char> &doomed)
{
	int removed = 0;

	for (size_t i = 0; i < doomed.size(); i++)
	{
		if (!char_in_list(doomed[i]))
			continue;

		extract_char(doomed[i]);
		removed++;
	}

	doomed.clear();
	return removed;
}

/* ------------------------------------------------------------------ *
 * The patrol
 * ------------------------------------------------------------------ */

/*
 * THE BORDER IS THE LEASH. A guard may step to any adjacent room that belongs
 * to ITS OWN realm and to nowhere else -- kingdom_owner_of_room() is asked of
 * the destination and its answer must equal the guard's stamp. That single
 * test is the whole containment rule, and it is deliberately asked of the
 * DESTINATION rather than tracked as a distance from the hall: territory
 * changes under a guard's feet when a ring reverts to arrears, and a guard
 * standing on ground the realm no longer owns must walk back in, not further
 * out. It also means a guard can never be lured across a border by a player
 * who steps outside, which is exactly the trick ACT_SENTINEL used to prevent
 * by nailing the mob to one room.
 *
 * Guards do NOT chase off the land. A guard in combat is left alone by this
 * proc entirely -- the engine's own combat movement is what follows a fleeing
 * opponent, and second-guessing it here would fight the fight for it.
 */
static bool kingdom_guard_may_enter(P_char ch, int to_room)
{
	if (!kingdom_guard_valid_rnum(to_room))
		return false;

	if (IS_ROOM(to_room, ROOM_NO_MOB))
		return false;

	return kingdom_owner_of_room(to_room) == KINGDOM_GUARD_ASSOC(ch);
}

/*
 * One patrol step. Picks among the legal exits at random rather than walking a
 * fixed circuit: a predictable beat is a beat a thief can time.
 */
static void kingdom_guard_patrol(P_char ch)
{
	int legal[NUM_EXITS];
	int count = 0;

	for (int dir = 0; dir < NUM_EXITS; dir++)
	{
		if (!CAN_GO(ch, dir))
			continue;
		if (!kingdom_guard_may_enter(ch, EXIT(ch, dir)->to_room))
			continue;

		legal[count] = dir;
		count++;
	}

	if (count == 0)
		return;

	const int chosen = legal[number(0, count - 1)];

	ch->only.npc->last_direction = chosen;
	do_move(ch, 0, exitnumb_to_cmd(chosen));
}

/*
 * A guard under attack calls the garrison in.
 *
 * Every other guard of the SAME realm that is not already fighting is pointed
 * at the room the call came from, one step at a time: the hunter bit plus a
 * hunt target is the engine's own way of saying "go there and deal with it"
 * (mob/mobact.c MobHuntCheck), and it costs nothing when the guard is already
 * on top of the trouble. Guards of another realm ignore it -- a border war is
 * not a mutual-defence pact -- and so does a guard whose own hands are full.
 */
static void kingdom_guard_call_the_garrison(P_char caller, P_char attacker)
{
	if (!caller || !caller->only.npc || caller->in_room == NOWHERE)
		return;

	const int assoc_id = KINGDOM_GUARD_ASSOC(caller);
	const int where = caller->in_room;
	const int guard_rnum = kingdom_guard_rnum();

	if (assoc_id <= 0 || guard_rnum < 0)
		return;

	char cry[MAX_INPUT_LENGTH];

	if (attacker && IS_ALIVE(attacker))
		snprintf(cry, sizeof(cry), "To me! %s is upon us at %s!", J_NAME(attacker),
			 world[where].name);
	else
		snprintf(cry, sizeof(cry), "To me! We are attacked at %s!", world[where].name);

	do_shout(caller, cry, CMD_SHOUT);

	/* MobHuntCheck may move and extract its caller through a destination room
	 * proc. Snapshot candidates before issuing any hunt order, then verify each
	 * pointer is still on character_list before dereferencing it. */
	std::vector<P_char> responders;
	for (P_char mob = character_list; mob; mob = mob->next)
	{
		if (mob == caller || !kingdom_char_is_guard_of(mob, guard_rnum, assoc_id))
			continue;
		if (!IS_ALIVE(mob) || IS_FIGHTING(mob) || mob->in_room == NOWHERE)
			continue;
		if (mob->in_room == where)
			continue;

		responders.push_back(mob);
	}

	for (P_char mob : responders)
	{
		if (!char_in_list(mob) || !IS_ALIVE(mob))
			continue;

		/* Hunting is how a mob crosses rooms toward something it cannot
		 * see; the bit is removed again by the engine when the hunt ends.
		 * The target is the ATTACKER rather than the room, because a hunt
		 * for a room the attacker has already left is a patrol with extra
		 * steps.
		 *
		 * ACT_HUNTER IS THE ONE WAY A GUARD CAN LEAVE ITS REALM, and that
		 * is deliberate rather than overlooked: MobCanGo() exempts a
		 * hunter from the sentinel bar, so a guard chasing whoever struck
		 * the garrison can follow them off the land. Chasing an attacker
		 * who runs is what a garrison is for. It is also self-correcting
		 * -- kingdom_guards_refresh() runs every upkeep tick and replaces
		 * any guard not standing on its post -- so a guard kited away is
		 * a guard replaced within the minute, not one lost. */
		if (attacker && char_in_list(attacker) && IS_ALIVE(attacker))
		{
			SET_BIT(mob->specials.act, ACT_HUNTER);
			MobHuntCheck(mob, attacker);
		}
	}
}

/*
 * The kingdom guard spec proc, bound on the prototype by
 * kingdom_guards_bind_proc() and therefore reached by EVERY mob of that vnum.
 * A mob whose stamp is not a live realm's is left entirely alone: the orphan
 * sweep in kingdom_guards_refresh_all() is what removes those, and a proc that
 * tried to patrol one would walk a mob with no borders.
 */
int kingdom_guard_proc(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	/* TRUE, AND IT MATTERS. read_mobile() arms a mob's spec heartbeat only
	 * for an ACT_SPEC prototype whose proc answers TRUE here
	 * (world/db.c:2706-2710); answering FALSE would leave the patrol below
	 * unreachable and the guards standing exactly where they were posted --
	 * the pre-2026-09-04 behaviour, silently. The prototype's own ACT_SPEC
	 * bit is the other half of that pair and lives in areas/mob/heavens.mob;
	 * neither half works without the other. */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || IS_PC(ch) || !ch->only.npc || !IS_ALIVE(ch))
		return FALSE;

	if (KINGDOM_GUARD_ASSOC(ch) <= 0 || !kingdom_find_realm(KINGDOM_GUARD_ASSOC(ch)))
		return FALSE;

	/* CMD_GOTHIT ONLY, and never CMD_HIT or CMD_KILL. Those two reach a mob
	 * proc through special() (cmd/interp.c), which offers EVERY command
	 * typed in the room to every mob standing in it -- so reacting to them
	 * would muster the whole garrison at a player who killed a rat next to a
	 * guard. CMD_GOTHIT is dispatched from the damage path itself
	 * (combat/fight.c) and means precisely "this mob was struck". */
	if (cmd == CMD_GOTHIT)
	{
		/* Once per fight, not once per swing. The muster is a shout to
		 * the whole zone and a hunt order to every other guard; firing it
		 * on every blow would fill the channel and re-target the garrison
		 * several times a second. ACT2_COMBAT_NEARBY is the engine's own
		 * "there is a fight next door" marker and is cleared by
		 * mobile_activity, so it doubles as the once-per-fight latch. */
		if (!IS_SET(ch->specials.act2, ACT2_COMBAT_NEARBY))
		{
			SET_BIT(ch->specials.act2, ACT2_COMBAT_NEARBY);
			kingdom_guard_call_the_garrison(ch, pl);
		}
		return FALSE;
	}

	/* CMD_PERIODIC, not CMD_MOB_MUNDANE: the mundane dispatch to spec procs
	 * at mobact.c:7625-7642 is COMMENTED OUT, and the live heartbeat is
	 * event_mob_proc()'s CMD_PERIODIC every PULSE_MOBILE. A patrol written
	 * against the mundane command would never once run. */
	if (cmd != CMD_PERIODIC)
		return FALSE;

	if (IS_FIGHTING(ch) || GET_POS(ch) != POS_STANDING || !CAN_ACT(ch))
		return FALSE;

	/* Not every tick. A guard that stepped on every pulse would cross its
	 * whole territory in a couple of minutes and read as frantic; one step
	 * in four is a walk. */
	if (number(0, 3) != 0)
		return FALSE;

	kingdom_guard_patrol(ch);
	return FALSE;
}

/*
 * Install the proc on the guard prototype. Called from kingdom_initialize()
 * beside the harvest nodes' binding, and self-gating the same way: a world
 * without the prototype simply has no guards, which is already this module's
 * documented fallback.
 */
void kingdom_guards_bind_proc(void)
{
	const int rnum = kingdom_guard_rnum();

	if (rnum >= 0)
		mob_index[rnum].func.mob = kingdom_guard_proc;

	const int champion = kingdom_champion_rnum();

	if (champion >= 0)
		mob_index[champion].func.mob = kingdom_champion_proc;

	/* The banners' tick and their breakability both come from this binding;
	 * without it a planted banner is scenery. Bound through real_object()
	 * rather than real_object0() for the reason kingdom_harvest.c spells
	 * out: real_object0() answers 0 both for the first object in the table
	 * and for a vnum that is not there at all. */
	const int combat = real_object(VOBJ_KINGDOM_BANNER_COMBAT);
	const int sanctity = real_object(VOBJ_KINGDOM_BANNER_SANCTITY);

	if (combat >= 0)
		obj_index[combat].func.obj = kingdom_banner_proc;
	if (sanctity >= 0)
		obj_index[sanctity].func.obj = kingdom_banner_proc;
}

/*
 * Stand the champion up, or take it away.
 *
 * Reconciled the same way the guards are and for the same reason -- one
 * idempotent function that can be called at boot, after a claim, after an
 * arrears cycle -- but kept separate from kingdom_guards_refresh() because the
 * champion answers to different rules: one only, at the hall rather than on a
 * border post, and only while the realm holds every square. Losing a square
 * sends it home; the realm keeps what it paid for and the champion returns
 * when the land does.
 */
int kingdom_champion_refresh(const kingdom_realm &realm)
{
	const int champion_rnum = kingdom_champion_rnum();

	if (champion_rnum < 0 || !kingdom_guard_valid_assoc(realm.assoc_id))
		return 0;

	const bool wanted = realm.champion_class != 0 &&
			    realm.highest_claim >= KINGDOM_MAX_SQUARES &&
			    realm.arrears < KARR_GUARDS_GONE &&
			    kingdom_guard_valid_rnum(realm.hall_rnum) && kingdom_cfg.enabled;

	std::vector<P_char> doomed;
	int standing = 0;

	for (P_char tch = character_list; tch; tch = tch->next)
	{
		if (!kingdom_char_is_champion_of(tch, champion_rnum, realm.assoc_id))
			continue;

		if (wanted && standing == 0)
			standing++;
		else
			doomed.push_back(tch);
	}

	(void)kingdom_guard_extract_all(doomed);

	if (!wanted || standing > 0)
		return standing;

	P_char mob = read_mobile(VMOB_KINGDOM_CHAMPION, VIRTUAL);

	if (!mob)
		return 0;

	P_Guild guild = get_guild_from_id(realm.assoc_id);

	KINGDOM_GUARD_ASSOC(mob) = realm.assoc_id;
	KINGDOM_GUARD_ROSTER(mob) = KINGDOM_CHAMPION_SLOT + 1;
	KINGDOM_CHAMPION_COUNT(mob) = 0;

	GET_ASSOC(mob) = guild;
	/* Set for the reason kingdom_guard_spawn_one() spells out: it stops the
	 * ENGINE moving this mob, and does not stop the patrol, which moves it
	 * through do_move(). */
	SET_BIT(mob->specials.act, ACT_SENTINEL);

	kingdom_guard_outfit(mob, guild);
	kingdom_champion_outfit(mob, realm);

	mob->player.birthplace = world[realm.hall_rnum].number;
	mob->player.orig_birthplace = world[realm.hall_rnum].number;
	mob->player.hometown = world[realm.hall_rnum].number;

	if (!char_to_room(mob, realm.hall_rnum, KINGDOM_GUARD_ENTRY_DIR))
	{
		if (char_in_list(mob))
			extract_char(mob);
		return 0;
	}

	if (guild)
		send_to_guild(guild, KINGDOM_MARSHAL,
			      "The realm's champion takes the field, banner braced.");

	return 1;
}

/* ------------------------------------------------------------------ *
 * The champion and its banners
 * ------------------------------------------------------------------ *
 * Ruled 2026-09-04. A realm holding all eighty squares -- and only such a
 * realm -- may raise ONE champion: a level-60 multiclass captain who plants a
 * banner and fights beside the garrison it inspires.
 *
 * THE BANNER IS THE WHOLE MECHANIC, and it is deliberately a real object in a
 * real room rather than a status effect on the guards. An aura that could not
 * be found or broken would be a passive the attacker simply eats; a banner
 * planted in the dirt is a thing an attacking party can see in the room
 * description, decide to go for, and destroy -- at the cost of the rounds they
 * spend not fighting the garrison. That trade is the design.
 *
 *   the banner of combat <realm>    a damage buff on every surviving guard
 *   the banner of sanctity <realm>  a heal on every surviving guard
 *
 * ONE AT A TIME, per realm. The champion plants whichever it judges the moment
 * calls for -- sanctity when the garrison is hurt, combat otherwise -- and
 * will not plant a second while the first still stands. Destroy it and the
 * champion raises another five rounds later, which is long enough that
 * breaking a banner is worth doing and short enough that it must be done
 * again.
 */

/* Defined below the proc that calls it. */
static bool kingdom_garrison_is_hurt(int assoc_id);

/* True for either kind of live garrison body belonging to `assoc_id`. Both the
 * banner tick and its cleanup use this exact predicate, so every body granted
 * combat fury is also one from which the falling banner removes it. */
static bool kingdom_char_is_garrison_of(P_char mob, int guard_rnum, int champion_rnum, int assoc_id)
{
	return (guard_rnum >= 0 && kingdom_char_is_guard_of(mob, guard_rnum, assoc_id)) ||
	       kingdom_char_is_champion_of(mob, champion_rnum, assoc_id);
}

/* True when `obj` is either kingdom banner. Compared on the prototype vnum
 * rather than on a name, because a name is world data an area edit can change
 * and a vnum is the contract between this file and heavens.obj. */
static bool kingdom_obj_is_banner(P_obj obj)
{
	if (!obj || obj->R_num < 0)
		return false;

	const int vnum = obj_index[obj->R_num].virtual_number;

	return vnum == VOBJ_KINGDOM_BANNER_COMBAT || vnum == VOBJ_KINGDOM_BANNER_SANCTITY;
}

/* The realm's banner if one is standing anywhere in the world, else NULL. One
 * walk of object_list; there are at most a handful of banners in existence,
 * and the alternative -- caching a pointer on the realm -- would be a pointer
 * to an object the engine may extract without telling this module. */
static P_obj kingdom_banner_of(int assoc_id)
{
	if (assoc_id <= 0)
		return NULL;

	for (P_obj obj = object_list; obj; obj = obj->next)
	{
		if (!kingdom_obj_is_banner(obj))
			continue;
		if (KINGDOM_BANNER_ASSOC(obj) != assoc_id)
			continue;
		/* Only a PLANTED banner counts. An object's location is a union
		 * discriminated by loc_p, so "is it standing in a room" must be
		 * asked with OBJ_ROOM(); reading loc.room without it would read a
		 * character pointer as an rnum. */
		if (!OBJ_ROOM(obj))
			continue;

		return obj;
	}

	return NULL;
}

/* Take down every banner of `assoc_id`, or EVERY banner in the world when
 * assoc_id is 0. A banner outlives the champion that planted it otherwise --
 * it is an ordinary world object with its own event -- and one still ticking
 * for a realm that no longer exists would buff nothing and confuse everyone.
 *
 * Collected first and extracted second, for the reason kingdom_guard_extract_
 * all() gives about characters: extract_obj() unlinks from object_list and
 * frees, so extracting during a walk of that list would follow a freed
 * ->next. */
static void kingdom_banners_of_realm_destroy(int assoc_id)
{
	std::vector<P_obj> doomed;

	for (P_obj obj = object_list; obj; obj = obj->next)
	{
		if (!kingdom_obj_is_banner(obj))
			continue;
		if (assoc_id != 0 && KINGDOM_BANNER_ASSOC(obj) != assoc_id)
			continue;

		doomed.push_back(obj);
	}

	for (size_t i = 0; i < doomed.size(); i++)
		extract_obj(doomed[i]);
}

/* Plant one. The description carries the realm's name, so a party that walks
 * into a room knows whose banner it is looking at and who they are fighting. */
static void kingdom_banner_plant(P_char champion, int assoc_id, bool sanctity)
{
	if (!champion || champion->in_room == NOWHERE || assoc_id <= 0)
		return;

	P_obj banner = read_object(
		sanctity ? VOBJ_KINGDOM_BANNER_SANCTITY : VOBJ_KINGDOM_BANNER_COMBAT, VIRTUAL);

	if (!banner)
		return;

	KINGDOM_BANNER_ASSOC(banner) = assoc_id;
	KINGDOM_BANNER_HP(banner) = KINGDOM_BANNER_HITPOINTS;

	P_Guild guild = get_guild_from_id(assoc_id);
	const std::string realm = guild ? guild->get_name() : "the realm";

	char named[MAX_INPUT_LENGTH];

	/* RESTRINGING, THE ENGINE'S WAY. The prototype's strings are SHARED
	 * between every instance -- read_object() copies the pointers -- so a
	 * per-realm name must be a fresh allocation on this object. It must ALSO
	 * be declared in str_mask: free_obj() (world/db.c) frees an object's
	 * strings only when the matching STRUNG_ bit says the string was altered
	 * in game, and leaves the rest alone precisely because they belong to the
	 * prototype. Allocate without setting the bit and every banner ever
	 * planted leaks both strings; set the bit without allocating and the
	 * first banner destroyed frees the prototype's strings out from under
	 * every other object of that vnum. */
	snprintf(named, sizeof(named), "%sthe banner of %s %s&n", sanctity ? "&+W" : "&+R",
		 sanctity ? "sanctity" : "combat", realm.c_str());
	banner->short_description = str_dup(named);
	banner->str_mask |= STRUNG_DESC2;

	snprintf(named, sizeof(named), "%sThe banner of %s %s&%s stands planted here.&n",
		 sanctity ? "&+W" : "&+R", sanctity ? "sanctity" : "combat", realm.c_str(),
		 sanctity ? "+W" : "+R");
	banner->description = str_dup(named);
	banner->str_mask |= STRUNG_DESC1;

	obj_to_room(banner, champion->in_room);

	act(sanctity ? "$n plants a banner of sanctity, and the cloth glows white." :
		       "$n plants a banner of combat, and the cloth snaps taut.",
	    FALSE, champion, 0, 0, TO_ROOM);
}

/*
 * The banner's own proc: it ticks, and it can be broken.
 *
 * CMD_PERIODIC is the engine's object heartbeat (world/db.c event_object_proc),
 * armed by answering TRUE to CMD_SET_PERIODIC. Every tick the banner reaches
 * EVERY LIVING GUARD OF ITS REALM, wherever they stand -- not merely the ones
 * in its own room. That is the ruling and it is also what makes a banner worth
 * hunting: a garrison spread across eighty squares is being held up by one
 * object, and the object is findable.
 */
int kingdom_banner_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
	if (!obj)
		return FALSE;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE; /* arm the tick */

	const int assoc_id = KINGDOM_BANNER_ASSOC(obj);

	if (assoc_id <= 0)
		return FALSE;

	if (cmd == CMD_PERIODIC)
	{
		const int guard_rnum = kingdom_guard_rnum();
		const int champion_rnum = kingdom_champion_rnum();
		const bool sanctity = obj_index[obj->R_num].virtual_number ==
				      VOBJ_KINGDOM_BANNER_SANCTITY;

		for (P_char mob = character_list; mob; mob = mob->next)
		{
			const bool ours = kingdom_char_is_garrison_of(mob, guard_rnum,
								      champion_rnum, assoc_id);

			if (!ours || !IS_ALIVE(mob))
				continue;

			if (sanctity)
			{
				/* A heal, scaled to the guard rather than a flat
				 * number, so it means the same to a level-45
				 * watchman and to the champion. Never above the
				 * maximum: a banner mends, it does not inflate. */
				const int mend = GET_MAX_HIT(mob) / 20;

				if (GET_HIT(mob) < GET_MAX_HIT(mob))
				{
					GET_HIT(mob) += mend;
					if (GET_HIT(mob) > GET_MAX_HIT(mob))
						GET_HIT(mob) = GET_MAX_HIT(mob);
				}
			}
			else if (!IS_AFFECTED(mob, AFF_INFERNAL_FURY))
			{
				/* The damage buff is applied as the engine's own
				 * fury affect rather than as a number this module
				 * adds and would then have to remember to take
				 * away. Set while the banner stands; the guard
				 * loses it when the banner falls, below. */
				SET_BIT(mob->specials.affected_by, AFF_INFERNAL_FURY);
			}
		}
		return FALSE;
	}

	/* BREAKING IT. An object proc sees the raw command, so a player typing
	 * `kill banner` reaches here with the argument they typed; answering
	 * TRUE consumes the command so the engine does not then complain that
	 * there is no such creature. The damage is the attacker's own -- level
	 * and damroll -- so a stronger party breaks a banner faster, which is
	 * the same trade every other target in the game offers. */
	if ((cmd == CMD_KILL || cmd == CMD_HIT) && ch && arg && *arg && OBJ_ROOM(obj) &&
	    obj->loc.room == ch->in_room && isname(arg, obj->name))
	{
		const int blow =
			number(1, GET_LEVEL(ch) > 0 ? GET_LEVEL(ch) : 1) + GET_DAMROLL(ch) + 1;

		KINGDOM_BANNER_HP(obj) -= blow;

		if (KINGDOM_BANNER_HP(obj) > 0)
		{
			act("You tear at $p.", FALSE, ch, obj, 0, TO_CHAR);
			act("$n tears at $p.", FALSE, ch, obj, 0, TO_ROOM);
			return TRUE;
		}

		act("$p tears loose and falls, its light going out.", FALSE, ch, obj, 0, TO_CHAR);
		act("$p tears loose and falls, its light going out.", FALSE, ch, obj, 0, TO_ROOM);

		/* The buff goes with it, and goes NOW rather than on some later
		 * sweep: a banner that fell but whose fury lingered would be a
		 * banner that could not really be broken. The heal needs no
		 * undoing -- what was mended stays mended. */
		const int guard_rnum = kingdom_guard_rnum();
		const int champion_rnum = kingdom_champion_rnum();

		for (P_char mob = character_list; mob; mob = mob->next)
			if (kingdom_char_is_garrison_of(mob, guard_rnum, champion_rnum, assoc_id))
				REMOVE_BIT(mob->specials.affected_by, AFF_INFERNAL_FURY);

		P_Guild guild = get_guild_from_id(assoc_id);

		if (guild)
			send_to_guild(guild, KINGDOM_MARSHAL,
				      "The champion's banner has been torn down.");

		extract_obj(obj);
		return TRUE;
	}

	return FALSE;
}

/*
 * The champion's proc. Everything a guard's proc does -- patrol inside the
 * borders, call the garrison when struck -- plus the banner.
 */
int kingdom_champion_proc(P_char ch, P_char pl, int cmd, char *arg)
{
	/* TRUE for the same reason kingdom_guard_proc() answers TRUE: it is what
	 * arms the heartbeat that runs the banner clock below. */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || IS_PC(ch) || !ch->only.npc || !IS_ALIVE(ch))
		return FALSE;

	const int assoc_id = KINGDOM_GUARD_ASSOC(ch);

	if (assoc_id <= 0 || !kingdom_find_realm(assoc_id))
		return FALSE;

	/* CMD_PERIODIC is the live spec heartbeat, one per PULSE_MOBILE -- the
	 * round this clock counts. A champion IN COMBAT still counts and still
	 * plants: the banner is what it is for, and a champion that could only
	 * raise one while nothing was happening would raise one only when it did
	 * not matter. It just cannot do it while held, stunned or on the floor. */
	if (cmd == CMD_PERIODIC && GET_POS(ch) >= POS_STANDING && CAN_ACT(ch))
	{
		/* THE FIVE-ROUND CLOCK. Counted on the champion rather than kept
		 * as a timestamp on the realm, so a champion that dies and is
		 * mustered again starts its count afresh -- which is the honest
		 * reading of "five rounds after the banner went". */
		if (!kingdom_banner_of(assoc_id))
		{
			KINGDOM_CHAMPION_COUNT(ch)++;

			if (KINGDOM_CHAMPION_COUNT(ch) >= KINGDOM_BANNER_REPLANT_ROUNDS)
			{
				KINGDOM_CHAMPION_COUNT(ch) = 0;

				/* Sanctity when the garrison is hurt, combat
				 * otherwise. The champion reads the field it is
				 * standing in rather than alternating blindly. */
				kingdom_banner_plant(ch, assoc_id,
						     kingdom_garrison_is_hurt(assoc_id));
			}
		}
		else
		{
			KINGDOM_CHAMPION_COUNT(ch) = 0;
		}
	}

	/* Then behave as any guard does. */
	return kingdom_guard_proc(ch, pl, cmd, arg);
}

/* True when any of the realm's guards is meaningfully wounded. What decides
 * which banner the champion plants, and deliberately a simple test: the
 * champion is a soldier, not a triage nurse. */
static bool kingdom_garrison_is_hurt(int assoc_id)
{
	const int guard_rnum = kingdom_guard_rnum();
	const int champion_rnum = kingdom_champion_rnum();

	if (guard_rnum < 0 && champion_rnum < 0)
		return false;

	for (P_char mob = character_list; mob; mob = mob->next)
	{
		if (!kingdom_char_is_garrison_of(mob, guard_rnum, champion_rnum, assoc_id) ||
		    !IS_ALIVE(mob))
			continue;
		if (GET_HIT(mob) * 4 < GET_MAX_HIT(mob) * 3)
			return true;
	}

	return false;
}

/* ------------------------------------------------------------------ *
 * The public roster operations
 * ------------------------------------------------------------------ */

/*
 * How many of this realm's guards are standing in the world RIGHT NOW: a live
 * census of character_list by realm stamp, not a read of the allowance.
 *
 * That distinction is the point for its caller, the `kingdom guards` display
 * (kingdom_cmds.c), which shows this beside kingdom_guard_allowance() as
 * standing-versus-permitted: the two disagree exactly while a refresh has not
 * yet caught up with a change -- an arrears rung, lost territory, guards
 * killed at their posts -- which is precisely what a player asking the
 * command wants to see. Guards are counted wherever they stand, on post or
 * off, because a mis-posted guard still exists until the next refresh removes
 * it and a census that hid it would just be the allowance again.
 */
int kingdom_guards_count(int assoc_id)
{
	const int guard_rnum = kingdom_guard_rnum();
	int count = 0;

	if (!kingdom_guard_valid_assoc(assoc_id) || guard_rnum < 0)
		return 0;

	for (P_char tch = character_list; tch != NULL; tch = tch->next)
	{
		if (kingdom_char_is_guard_of(tch, guard_rnum, assoc_id))
			count++;
	}

	return count;
}

/*
 * Send every one of this realm's guards home. Returns how many were removed.
 *
 * The caller is kingdom_on_guild_deleted() (kingdom.c): when a guild is
 * disbanded its garrison must leave with it, immediately, not linger until the
 * next sweep notices the realm is gone -- association ids are reused, so a
 * leftover guard would be adopted by the id's next owner. That hook runs
 * BEFORE the Guild object is freed, which is fine either way here, because
 * guards are matched on the stamped int id and never on GET_ASSOC()'s
 * pointer.
 *
 * NOT gated on kingdom_cfg.enabled: switching the feature off must be able to
 * clear the world of guards, and a realm being deleted must be able to clear
 * its own whatever the config says.
 */
int kingdom_guards_despawn(int assoc_id)
{
	const int guard_rnum = kingdom_guard_rnum();
	const int champion_rnum = kingdom_champion_rnum();
	std::vector<P_char> doomed;

	if (!kingdom_guard_valid_assoc(assoc_id))
		return 0;

	for (P_char tch = character_list; tch != NULL; tch = tch->next)
	{
		/* THE CHAMPION LEAVES WITH THE GARRISON. This is the guild-deleted
		 * path, and the reason it must take the champion too is the same
		 * reason it takes the guards: association ids are reused, so a
		 * champion left standing would be adopted by whoever founds the
		 * next guild on that id -- a level-60 multiclass mob planting
		 * banners for a realm that never bought it. */
		if (guard_rnum >= 0 && kingdom_char_is_guard_of(tch, guard_rnum, assoc_id))
			doomed.push_back(tch);
		else if (kingdom_char_is_champion_of(tch, champion_rnum, assoc_id))
			doomed.push_back(tch);
	}

	kingdom_banners_of_realm_destroy(assoc_id);

	return kingdom_guard_extract_all(doomed);
}

/* Every kingdom guard and champion in the world, whoever owns them, and every
 * banner with them. For shutdown, and for switching the feature off. */
int kingdom_guards_despawn_all(void)
{
	const int guard_rnum = kingdom_guard_rnum();
	const int champion_rnum = kingdom_champion_rnum();
	std::vector<P_char> doomed;

	for (P_char tch = character_list; tch != NULL; tch = tch->next)
	{
		if (guard_rnum >= 0 && kingdom_char_is_guard(tch, guard_rnum))
			doomed.push_back(tch);
		else if (kingdom_char_is_champion(tch, champion_rnum))
			doomed.push_back(tch);
	}

	kingdom_banners_of_realm_destroy(0);

	return kingdom_guard_extract_all(doomed);
}

/*
 * Reconcile one realm's garrison with what its record says it should be, and
 * return the number of guards standing afterwards.
 *
 * Idempotent: calling it twice in a row does nothing the second time, which is
 * what lets one function serve every moment the garrison can change. It is
 * reached through kingdom_guards_refresh_all() from kingdom_initialize() at
 * boot and from kingdom_upkeep_event() after each cycle -- the cycle being
 * where an arrears rung moves and where a paid debt restores the allowance --
 * and, for the one realm they change, from kingdom_claim_next(),
 * kingdom_abandon_last() and kingdom_on_guildhall_changed().
 */
int kingdom_guards_refresh(const kingdom_realm &realm)
{
	const int guard_rnum = kingdom_guard_rnum();
	int posts[KINGDOM_MAX_SQUARES] = {};
	bool manned[KINGDOM_MAX_SQUARES] = {};
	std::vector<P_char> doomed;
	int target, post_count, standing = 0, before;
	P_Guild guild;

	if (!kingdom_guard_valid_assoc(realm.assoc_id))
		return 0;

	if (guard_rnum < 0)
	{
		/* No prototype in the world, so there is nothing to spawn and,
		 * equally, nothing that could already be standing. Warned once per
		 * boot rather than once per realm per upkeep cycle. */
		static bool warned = false;
		if (!warned && kingdom_cfg.enabled)
		{
			warned = true;
			logit(LOG_KINGDOM,
			      "mob %d is not in the world; realms will field no guards.",
			      VMOB_KINGDOM_GUARD);
		}
		return 0;
	}

	target = kingdom_guard_garrison(realm);
	post_count = kingdom_guard_posts(realm, target, posts, KINGDOM_MAX_SQUARES);

	/* WHICH ROSTER LINE MANS WHICH POST. Filled slots in slot order against
	 * posts in post order, so the pairing is stable across refreshes: a
	 * guard is not shuffled to a different square merely because its
	 * neighbour was promoted. post_count is already <= the roster count via
	 * kingdom_guard_garrison(), so the walk cannot run out of slots, but the
	 * bound is written anyway rather than assumed. */
	int roster_for_post[KINGDOM_MAX_SQUARES];
	int paired = 0;

	for (int slot = 0; slot < KINGDOM_GUARD_SLOTS && paired < post_count; slot++)
	{
		if (realm.guards[slot].level <= 0)
			continue;

		roster_for_post[paired] = slot;
		paired++;
	}

	/* Fewer bodies bought than posts found: only the posts a paid guard can
	 * stand on are posts at all. */
	post_count = paired;

	/*
	 * One walk of character_list, deciding only. Nothing is extracted inside
	 * it -- see kingdom_guard_extract_all() for why -- and nothing is spawned
	 * inside it either, because read_mobile() pushes the new mob onto the
	 * HEAD of character_list (world/db.c:1960-1962), which this very loop
	 * would then walk into.
	 */
	for (P_char tch = character_list; tch != NULL; tch = tch->next)
	{
		int slot = -1;

		if (!kingdom_char_is_guard_of(tch, guard_rnum, realm.assoc_id))
			continue;

		for (int post = 0; post < post_count; post++)
		{
			if (manned[post] || posts[post] != tch->in_room)
				continue;

			/* RIGHT SQUARE IS NOT ENOUGH. The body must also BE the
			 * roster line that post belongs to, at the rank that line
			 * currently says. A promoted or re-classed guard fails
			 * this test and is replaced -- the level and class are
			 * baked into the mob at spawn and there is no safe way to
			 * raise them under a body that may be mid-fight. */
			const int line = roster_for_post[post];

			if (KINGDOM_GUARD_ROSTER(tch) != line + 1)
				continue;
			if (GET_LEVEL(tch) != realm.guards[line].level)
				continue;

			slot = post;
			break;
		}

		if (slot >= 0)
		{
			/* Already exactly where it should be: leave it alone, so a
			 * refresh does not churn the whole garrison every cycle. */
			manned[slot] = true;
			standing++;
		}
		else
		{
			/* Surplus to the allowance, standing on land the realm no
			 * longer holds, or in NOWHERE. All three are removals. */
			doomed.push_back(tch);
		}
	}

	before = standing + static_cast<int>(doomed.size());
	(void)kingdom_guard_extract_all(doomed);

	guild = get_guild_from_id(realm.assoc_id);

	for (int post = 0; post < post_count; post++)
	{
		if (manned[post])
			continue;
		if (kingdom_guard_spawn_one(realm, posts[post], guild, roster_for_post[post]))
			standing++;
	}

	/* Only on a change, so a steady realm gets no message every cycle. */
	if (standing == before)
		return standing;

	if (guild != NULL)
	{
		if (standing == 0)
			send_to_guild(guild, KINGDOM_MARSHAL,
				      "The garrison disperses; no one is left to hold the "
				      "realm's borders.");
		else if (standing > before)
			send_to_guild(guild, KINGDOM_MARSHAL,
				      "Fresh guards muster and take up their posts along the "
				      "realm's borders.");
		else
			send_to_guild(guild, KINGDOM_MARSHAL,
				      "The garrison thins; the realm can no longer keep every "
				      "post manned.");
	}

	logit(LOG_KINGDOM, "realm %d garrison %d -> %d (permitted %d, posts %d).", realm.assoc_id,
	      before, standing, target, post_count);

	return standing;
}

/* Reconcile the garrison AND the champion. Every caller outside this file goes
 * through here rather than through the two separately: the champion changes
 * for the same reasons the guards do -- a claim, an arrears rung, a hall moved
 * -- and a second entry point would be a second thing to remember to call.
 * kingdom_guards_refresh() stays separate underneath it so the guard
 * reconciliation can still be reasoned about on its own. */
int kingdom_garrison_refresh(const kingdom_realm &realm)
{
	const int standing = kingdom_guards_refresh(realm);

	return standing + kingdom_champion_refresh(realm);
}

/*
 * Reconcile every realm, and clear away guards whose stamp does not resolve to
 * a live realm.
 *
 * The orphan sweep is not belt and braces; it has three real customers:
 *
 *   - reused association ids. found_asc() hands out the lowest free id, which
 *     is why kingdom.h insists a deleted guild's realm is dropped before the
 *     id can be reissued -- a guard left standing with a stamp of 7 would be
 *     silently adopted by the next realm to take id 7.
 *   - copyover survivors. copyover_recover() restores last cycle's guards
 *     with their stamp zeroed (see the file banner), so after a copyover the
 *     whole old garrison stands here stamped 0, doubled up with the fresh one.
 *   - hand-loaded copies of the prototype, which carry assoc 0 the same way.
 *     The vnum belongs to this module, so removing them is the intended
 *     reading rather than a side effect.
 *
 * The last two are why stamp 0 is refused EXPLICITLY, by
 * kingdom_guard_valid_assoc(), rather than left to a map lookup: kingdom.h's
 * kingdom_guild_has_realm() answers for ids that could name an association,
 * and "no owner at all" should not depend on kingdom_realms merely happening
 * never to hold a key of 0.
 */
void kingdom_guards_refresh_all(void)
{
	const int guard_rnum = kingdom_guard_rnum();
	const int champion_rnum = kingdom_champion_rnum();

	if (guard_rnum >= 0 || champion_rnum >= 0)
	{
		std::vector<P_char> orphans;

		for (P_char tch = character_list; tch != NULL; tch = tch->next)
		{
			/* THE CHAMPION IS SWEPT WITH THE GUARDS, and for exactly the
			 * same reason: copyover_recover() restores every living NPC
			 * but not this module's value[] stamps, so last cycle's
			 * champion comes back stamped 0 -- a champion of no realm --
			 * while the next refresh musters a fresh one beside it. */
			const bool ours =
				(guard_rnum >= 0 && kingdom_char_is_guard(tch, guard_rnum)) ||
				kingdom_char_is_champion(tch, champion_rnum);

			if (!ours)
				continue;

			/* A guard stands only for a stamp that is a plausible
			 * association id AND names a live realm. The first test is
			 * what despawns the stamp-0 copyover survivors and hand-loads;
			 * the second is the same answer Guild::is_kingdom() gets. */
			const int stamp = KINGDOM_GUARD_ASSOC(tch);
			if (!kingdom_guard_valid_assoc(stamp) || !kingdom_guild_has_realm(stamp))
				orphans.push_back(tch);
		}

		const int swept = kingdom_guard_extract_all(orphans);
		if (swept > 0)
			logit(LOG_KINGDOM,
			      "swept %d guard(s) whose stamp resolves to no live realm.", swept);
	}

	/* And any banner whose realm has gone with them: a banner is an ordinary
	 * world object with its own event, so it outlives the champion that
	 * planted it unless something takes it down. A copyover leaves banners
	 * stamped 0 exactly as it leaves guards stamped 0.
	 *
	 * Done in place rather than through kingdom_banners_of_realm_destroy(),
	 * whose 0 means "every banner in the world" -- passing an orphan's stamp
	 * of 0 to it would tear down the banners of every LIVE realm too. */
	{
		std::vector<P_obj> orphans;

		for (P_obj obj = object_list; obj; obj = obj->next)
		{
			if (!kingdom_obj_is_banner(obj))
				continue;

			const int stamp = KINGDOM_BANNER_ASSOC(obj);

			if (!kingdom_guard_valid_assoc(stamp) || !kingdom_guild_has_realm(stamp))
				orphans.push_back(obj);
		}

		for (size_t i = 0; i < orphans.size(); i++)
			extract_obj(orphans[i]);

		if (!orphans.empty())
			logit(LOG_KINGDOM,
			      "swept %zu banner(s) whose stamp resolves to no live realm.",
			      orphans.size());
	}

	/* kingdom_guards_refresh() only reads kingdom_realms -- it never inserts
	 * or erases -- so iterating the map across the call is safe. */
	for (std::unordered_map<int, kingdom_realm>::const_iterator it = kingdom_realms.begin();
	     it != kingdom_realms.end(); ++it)
		(void)kingdom_garrison_refresh(it->second);
}
