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

#include "core/config.h"
#include "core/prototypes.h"
#include "core/utility.h"
#include "core/utils.h"
#include "guild/assocs.h"
#include "kingdom/kingdom_geometry.h"
#include "world/db.h"

#include <cstddef>
#include <vector>

/* Not declared in any header the engine exports; kingdom_geometry.c and
 * kingdom_claim.c take the same externs for the same reason. */
extern struct room_data *world;
extern int top_of_world;
extern P_char character_list;
extern P_index mob_index;

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

	return squares / per_squares;
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

	return kingdom_guard_allowance(realm);
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
static bool kingdom_guard_spawn_one(const kingdom_realm &realm, int rnum, P_Guild guild)
{
	P_char mob;

	if (!kingdom_guard_valid_rnum(rnum))
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

	/* The engine's own convention for a guild-owned NPC: Guild::update()
	 * walks character_list and skips NPCs precisely because outpost and
	 * guildhall guards carry this (guild/assocs.c:3724-3728), and
	 * Building::load_gateguard() sets it the same way (world/buildings.c:707).
	 * A NULL guild is tolerated -- the stamp above is what this module reads,
	 * and the pointer is a courtesy to the rest of the server. */
	GET_ASSOC(mob) = guild;

	/* A guard holds its post. Zone resets set the same bit on mobs that must
	 * not wander from where they were placed (world/db.c:3795). */
	SET_BIT(mob->specials.act, ACT_SENTINEL);

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
	std::vector<P_char> doomed;

	if (!kingdom_guard_valid_assoc(assoc_id) || guard_rnum < 0)
		return 0;

	for (P_char tch = character_list; tch != NULL; tch = tch->next)
	{
		if (kingdom_char_is_guard_of(tch, guard_rnum, assoc_id))
			doomed.push_back(tch);
	}

	return kingdom_guard_extract_all(doomed);
}

/* Every kingdom guard in the world, whoever owns it. For shutdown, and for
 * switching the feature off. */
int kingdom_guards_despawn_all(void)
{
	const int guard_rnum = kingdom_guard_rnum();
	std::vector<P_char> doomed;

	if (guard_rnum < 0)
		return 0;

	for (P_char tch = character_list; tch != NULL; tch = tch->next)
	{
		if (kingdom_char_is_guard(tch, guard_rnum))
			doomed.push_back(tch);
	}

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
			if (!manned[post] && posts[post] == tch->in_room)
			{
				slot = post;
				break;
			}
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
		if (kingdom_guard_spawn_one(realm, posts[post], guild))
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

	if (guard_rnum >= 0)
	{
		std::vector<P_char> orphans;

		for (P_char tch = character_list; tch != NULL; tch = tch->next)
		{
			if (!kingdom_char_is_guard(tch, guard_rnum))
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

	/* kingdom_guards_refresh() only reads kingdom_realms -- it never inserts
	 * or erases -- so iterating the map across the call is safe. */
	for (std::unordered_map<int, kingdom_realm>::const_iterator it = kingdom_realms.begin();
	     it != kingdom_realms.end(); ++it)
		(void)kingdom_guards_refresh(it->second);
}
