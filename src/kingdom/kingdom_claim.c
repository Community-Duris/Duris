/*
 *  kingdom_claim.c
 *  Duris
 *
 *  Conversion, and the claim/abandon state machine.
 *
 *  A guild becomes a kingdom by anchoring a realm on its main guildhall's map
 *  square. From then on it grows ONE SQUARE AT A TIME -- always the next index
 *  in the fixed clockwise order -- and it shrinks only from the far end. That
 *  is what keeps a realm's territory expressible as the single integer
 *  highest_claim: indices 1..highest_claim are owned and nothing else is, and
 *  the owned set can never develop a hole. Every rule in this file exists to
 *  preserve that invariant.
 *
 *  RE-JUDGE, NEVER REMEMBER. Ruling 1 (RULINGS.md) requires all 80 squares to
 *  be eligible when the guildhall is sited, but a verdict reached then is a
 *  verdict about a world that has since moved: another guild may have built, a
 *  zone may have been rewritten, a hall may have been razed. So conversion
 *  re-asks kingdom_judge_footprint() about all 80 squares, and every single
 *  claim re-asks kingdom_judge_square() about the one square it is taking.
 *  Placement-time eligibility buys a good hall site; it does not buy a
 *  standing permission.
 *
 *  ORDER OF OPERATIONS, WHICH NEVER VARIES:
 *      validate everything -> take the coin -> mutate the realm -> persist.
 *  Nothing is charged for a claim that is going to be refused, and no square
 *  is granted that has not been paid for.
 *
 *  NO SPECIAL CASE FOR RECLAIMING. Ruling 6 says a ring reverted for unpaid
 *  upkeep costs the full original price to take back. Because claiming always
 *  charges kingdom_claim_cost(highest_claim + 1), and that is a pure function
 *  of the index, walking back up a reverted ring bills exactly what it billed
 *  the first time. There is deliberately no separate "rebuy" path here.
 */

#include "kingdom/kingdom_internal.h"

#include "core/structs.h"

#include "core/config.h"
#include "core/prototypes.h"
#include "core/utility.h"
#include "core/utils.h"
#include "guild/assocs.h"
#include "guild/guildhall.h"

#include <climits>

/* Not declared in any header the engine exports; kingdom_geometry.c takes the
 * same extern for the same reason. */
extern struct room_data *world;

/* ------------------------------------------------------------------ *
 * Prices
 * ------------------------------------------------------------------ */

long kingdom_claim_cost(int index)
{
	if (index < 1 || index > KINGDOM_MAX_SQUARES)
	{
		return 0;
	}

	/* base + per_square * n, as documented on kingdom_config.
	 *
	 * Both terms come from lib/kingdom.cfg, which is a hand-edited file, so
	 * they are clamped at BOTH ends before any arithmetic touches them. The
	 * floor stops a negative entry turning the price into a payment TO the
	 * guild. The ceiling stops `each * index` overflowing a signed long,
	 * which would be undefined behaviour reached from a config typo; INT_MAX
	 * per knob keeps every price this function can produce well inside a
	 * long (see the sum bound below) while still pricing land far beyond
	 * any treasury that will ever exist. */
	const long ceiling = (long)INT_MAX;
	long base = kingdom_cfg.claim_cost_base > 0 ? kingdom_cfg.claim_cost_base : 0;
	long each = kingdom_cfg.claim_cost_per_square > 0 ? kingdom_cfg.claim_cost_per_square : 0;

	if (base > ceiling)
	{
		base = ceiling;
	}
	if (each > ceiling)
	{
		each = ceiling;
	}

	/* At most INT_MAX + INT_MAX * 80, which is ~1.7e11 -- comfortably inside
	 * a 64-bit long, so the sum below cannot overflow. */
	return base + (each * (long)index);
}

long kingdom_ring_cost(int ring)
{
	const int first = kingdom_ring_first_index(ring);
	const int last = kingdom_ring_last_index(ring);

	/* The geometry helpers answer 0 for a ring outside 1..KINGDOM_MAX_RING. */
	if (first < 1 || last < first)
	{
		return 0;
	}

	/* Summed from kingdom_claim_cost() rather than given a formula of its
	 * own, so that "a reverted ring costs the full original price again" is
	 * true by construction: the ring price and the square-by-square price it
	 * is quoted against cannot drift apart. */
	long total = 0;

	for (int index = first; index <= last; index++)
	{
		total += kingdom_claim_cost(index);
	}

	return total;
}

/* ------------------------------------------------------------------ *
 * Paying out of the guild treasury
 * ------------------------------------------------------------------ *
 * DENOMINATIONS. 1 platinum = 1000 copper, 1 gold = 100, 1 silver = 10
 * (GET_MONEY, src/core/utils.h:467). Kingdom prices are configured in
 * copper.
 *
 * Charging goes through Guild::sub_copper() (RULINGS.md 3, guild/assocs.c),
 * which values the whole purse in copper, checks it ONCE against the price,
 * and makes change across the denominations. This file used to probe
 * Guild::sub_money() with a short list of exact-sum splits, because
 * sub_money() tests each denomination against its own balance and cannot
 * break a coin -- but a purse of 5 gold and 20 silver holds 700 copper and
 * matched none of the four spellings of 700, so a claim the treasury could
 * plainly afford was refused. sub_copper() removed the need for any of that;
 * the splitting that survives below is for MESSAGES only and never decides
 * what is payable.
 */

#define KINGDOM_COPPER_PER_PLATINUM 1000L
#define KINGDOM_COPPER_PER_GOLD 100L
#define KINGDOM_COPPER_PER_SILVER 10L

/* Fewest-coins rendering of a copper price, for refusal messages. Display
 * only: Guild::sub_copper() alone decides what the treasury can pay.
 * coins_to_string() returns a STATIC buffer (src/core/utility.c:7289), so at
 * most one result may be live in a format call at a time. */
static const char *kingdom_price_string(long price)
{
	/* Unreachable from the one caller, which charges only positive prices,
	 * but coins_to_string() cannot render all-zero coins: it falls through
	 * every branch, logs, and returns an empty string. Answer in words. */
	if (price <= 0)
	{
		return "nothing";
	}

	/* kingdom_claim_cost() caps each config knob at INT_MAX, so a price is
	 * at most 81 * INT_MAX and the platinum count at most a thousandth of
	 * that -- comfortably inside coins_to_string()'s int parameters. */
	long rest = price;

	const int plat = (int)(rest / KINGDOM_COPPER_PER_PLATINUM);
	rest %= KINGDOM_COPPER_PER_PLATINUM;
	const int gold = (int)(rest / KINGDOM_COPPER_PER_GOLD);
	rest %= KINGDOM_COPPER_PER_GOLD;
	const int silver = (int)(rest / KINGDOM_COPPER_PER_SILVER);
	rest %= KINGDOM_COPPER_PER_SILVER;

	return coins_to_string(plat, gold, silver, (int)rest, "&+y");
}

/* Debit `price` copper from the guild. True only if the whole price was
 * taken; sub_copper() checks before it mutates, so a refusal is atomic and
 * leaves no ledger line. */
static bool kingdom_pay_from_treasury(P_Guild guild, long price)
{
	if (!guild || price <= 0)
	{
		return false;
	}

	if (!guild->sub_copper(price))
	{
		return false;
	}

	/* sub_copper() adjusts the balances and writes the ledger but does
	 * not persist. Every other Guild mutator -- set_name,
	 * add_prestige, add_construction and the rest in guild/assocs.h --
	 * calls save() itself, so a debit that skipped it would be undone
	 * by the next reboot. */
	guild->save();
	return true;
}

/* ------------------------------------------------------------------ *
 * Small shared helpers
 * ------------------------------------------------------------------ */

/* True when rnum names a real room. 0 is rejected deliberately: real_room0()
 * answers 0 both for the first room and for "no such vnum", and every caller
 * here means the latter. */
static bool kingdom_valid_rnum(int rnum)
{
	return rnum > 0;
}

/* The guild's MAIN hall, or NULL.
 *
 * Not Guildhall::find_by_assoc_id(): that finder dereferences gh->guild with
 * no null check (src/guild/guildhall.c:140-141) even though the loader leaves
 * that pointer null whenever get_guild_from_id() misses
 * (src/guild/guildhall_db.c:394), and it returns the first hall of ANY type --
 * a guild may also own outposts (GH_TYPE_OUTPOST), and an outpost must never
 * anchor a realm. Matching on the persisted assoc_id field touches no
 * possibly-null pointer at all. */
static Guildhall *kingdom_main_hall(int assoc_id)
{
	if (assoc_id <= 0)
	{
		return NULL;
	}

	for (size_t i = 0; i < Guildhall::guildhalls.size(); i++)
	{
		Guildhall *gh = Guildhall::guildhalls[i];

		if (!gh)
		{
			continue;
		}
		if (gh->assoc_id != assoc_id)
		{
			continue;
		}
		if (gh->type != GH_TYPE_MAIN)
		{
			continue;
		}

		return gh;
	}

	return NULL;
}

/* Realm ids are only a stable surrogate for the persisted row -- realms are
 * keyed by association id everywhere else. Hand out one past the highest in
 * use rather than filling a gap, so that a single id never names two different
 * realms across one boot's logs. */
static int kingdom_next_realm_id(void)
{
	int highest = 0;

	for (const auto &entry : kingdom_realms)
	{
		if (entry.second.realm_id > highest)
		{
			highest = entry.second.realm_id;
		}
	}

	return highest + 1;
}

/* kingdom_reindex_realm() is documented as a rebuild, but ABANDON SHRINKS the
 * territory, and an implementation that only adds entries would leave the
 * released square still resolving to us in kingdom_owner_of_room(). Dropping
 * the realm's entries first is correct under either implementation and costs
 * at most 80 erases. */
static void kingdom_refresh_index(const kingdom_realm &realm)
{
	kingdom_unindex_realm(realm.assoc_id);
	kingdom_reindex_realm(realm);
}

/* Persist a realm whose money-bearing state just changed. By the time this
 * runs the coin has already left the treasury, so a failed write must not be
 * silent: the realm is left dirty for kingdom_db_flush_dirty() to retry, and
 * the failure is logged with everything needed to reconstruct it by hand. */
static void kingdom_persist_realm(kingdom_realm &realm)
{
	realm.dirty = true;

	if (kingdom_db_save_realm(realm))
	{
		realm.dirty = false;
		return;
	}

	logit(LOG_KINGDOM,
	      "SAVE FAILED: realm %d (assoc %d, hall vnum %d, %d squares) left dirty for retry.",
	      realm.realm_id, realm.assoc_id, realm.hall_vnum, realm.highest_claim);
}

/* The shared front door for all three verbs: the subsystem must be on, and the
 * actor must be a player who leads a guild. Every refusal is messaged here so
 * that the verbs below read as policy rather than plumbing. NULL means the
 * actor may not act and has already been told why. */
static P_Guild kingdom_actor_guild(P_char ch)
{
	if (!ch || IS_NPC(ch))
	{
		return NULL;
	}

	if (!kingdom_enabled())
	{
		send_to_char("Kingdoms are not enabled.\r\n", ch);
		return NULL;
	}

	P_Guild guild = GET_ASSOC(ch);

	if (!guild || !IS_MEMBER(GET_A_BITS(ch)))
	{
		send_to_char("You do not belong to a guild.\r\n", ch);
		return NULL;
	}

	/* Ranks are ordered enemy < parole < normal < senior < officer < deputy
	 * < leader < god (src/guild/assocs.h:47-64 packs them into RK3..RK1 in
	 * exactly that order), so GT_DEPUTY means leader or guild god. Founding a
	 * realm, buying land and giving land up all spend the treasury and all
	 * change what the guild owes every upkeep cycle: that is the leader's
	 * call. */
	if (!GT_DEPUTY(GET_A_BITS(ch)))
	{
		send_to_char("Only your guild's leader may speak for the realm.\r\n", ch);
		return NULL;
	}

	return guild;
}

/* The actor's realm, with its anchor re-resolved against the world as it is
 * right now. NULL when there is no usable realm; the actor has been told why.
 *
 * The anchor is re-resolved on every verb because kingdom_realm persists only
 * hall_vnum -- rnum, zone and x/y are derived, and a hall that was razed or
 * moved since boot leaves them pointing at someone else's ground. */
static kingdom_realm *kingdom_actor_realm(P_char ch, P_Guild guild)
{
	kingdom_realm *realm = kingdom_find_realm((int)guild->get_id());

	if (!realm)
	{
		send_to_char("Your guild is not a kingdom.\r\n", ch);
		return NULL;
	}

	if (!kingdom_resolve_anchor(*realm) || !kingdom_valid_rnum(realm->hall_rnum))
	{
		send_to_char("Your guildhall cannot be found, so your realm has no anchor.\r\n",
			     ch);
		return NULL;
	}

	return realm;
}

/* ------------------------------------------------------------------ *
 * Conversion
 * ------------------------------------------------------------------ */

/* `kingdom convert`. Turns the actor's guild into a kingdom anchored on its
 * main guildhall's map square, holding no land yet (highest_claim = 0).
 *
 * Conversion itself is free. There is no conversion price in kingdom_config,
 * and inventing one here would bury a number in the code that no designer
 * could tune from lib/kingdom.cfg; the money is charged for land, one square
 * at a time, by kingdom_claim_next() below. */
bool kingdom_convert_guild(P_char ch)
{
	P_Guild guild = kingdom_actor_guild(ch);

	if (!guild)
	{
		return false;
	}

	const int assoc_id = (int)guild->get_id();

	/* THE ONLY PLACE A REALM IS CREATED, so the only place worth asserting
	 * that its key is one the rest of the module will accept. The square
	 * index refuses an association outside 1..MAX_ASC-1, and it refuses it
	 * SILENTLY -- a realm keyed outside that range would sit in
	 * kingdom_realms owning squares that kingdom_owner_of_room() reports as
	 * unowned, which is the worst shape a bug can take here.
	 *
	 * The range is found_asc()'s, not a guess: it allocates upward from 1 and
	 * refuses at `i >= MAX_ASC` (src/guild/assocs.c, the id-allocation loop),
	 * so a live guild is always 1..MAX_ASC-1 and anything else means the
	 * association row was hand-edited or corrupted. */
	if (assoc_id <= 0 || assoc_id >= MAX_ASC)
	{
		logit(LOG_KINGDOM, "CONVERT: refused association id %d, outside 1..%d.", assoc_id,
		      MAX_ASC - 1);
		send_to_char("Your guild's records are damaged; it cannot found a realm.\r\n", ch);
		return false;
	}

	if (kingdom_find_realm(assoc_id))
	{
		send_to_char("Your guild is already a kingdom.\r\n", ch);
		return false;
	}

	Guildhall *hall = kingdom_main_hall(assoc_id);

	if (!hall)
	{
		send_to_char("Your guild has no guildhall to build a realm around.\r\n", ch);
		return false;
	}

	/* outside_vnum is the MAP room the hall stands on -- the same field
	 * guildhall_map_check() measures hall-to-hall distance between
	 * (src/guild/guildhall_cmds.c:1509). The interior rooms live in the
	 * guildhall zone and are not map squares at all. */
	const int hall_rnum = real_room0(hall->outside_vnum);

	if (!kingdom_valid_rnum(hall_rnum))
	{
		send_to_char("Your guildhall does not stand on any map square.\r\n", ch);
		return false;
	}

	/* Ruling 1 checked all 80 squares when the hall was sited. Check them
	 * again now: halls grandfathered under the old MAX_GH_PROXIMITY_RADIUS of
	 * 3 (ruling 2) sit far closer together than a single realm's own reach,
	 * and they must be refused conversion rather than silently overlapped. */
	int bad_index = 0;
	const int verdict =
		kingdom_judge_footprint(hall_rnum, (int)guild->get_racewar(), assoc_id, &bad_index);

	if (verdict != KSQ_OK)
	{
		char why[MAX_INPUT_LENGTH];

		snprintf(why, sizeof(why),
			 "Your hall cannot anchor a realm: square %d of %d is refused (%s).\r\n",
			 bad_index, KINGDOM_MAX_SQUARES, kingdom_verdict_text(verdict));
		send_to_char(why, ch);
		return false;
	}

	kingdom_realm fresh;

	fresh.realm_id = kingdom_next_realm_id();
	fresh.assoc_id = assoc_id;
	fresh.hall_vnum = hall->outside_vnum;
	fresh.highest_claim = 0;
	/* Paid up on day one: the first upkeep cycle must bill from now and not
	 * from the epoch, or a brand-new realm is in arrears the moment it
	 * exists. It owes nothing yet in any case -- upkeep scales with squares
	 * held, and it holds none. */
	fresh.upkeep_paid_through = time(0);
	fresh.arrears = KARR_CURRENT;
	fresh.missed_cycles = 0;

	if (!kingdom_resolve_anchor(fresh))
	{
		send_to_char("Your guildhall does not stand on any map square.\r\n", ch);
		return false;
	}

	kingdom_realms[assoc_id] = fresh;

	/* Work through the stored element, never `fresh`: the index and the
	 * persistence layer must see the object the rest of the module will go on
	 * reading. */
	kingdom_realm *realm = kingdom_find_realm(assoc_id);

	if (!realm)
	{
		logit(LOG_KINGDOM,
		      "CONVERT: realm for assoc %d vanished immediately after insert.", assoc_id);
		send_to_char("Something went wrong founding your realm.\r\n", ch);
		return false;
	}

	kingdom_refresh_index(*realm);
	kingdom_persist_realm(*realm);

	logit(LOG_KINGDOM, "CONVERT: %s (assoc %d) founded realm %d on hall vnum %d.",
	      guild->get_name().c_str(), assoc_id, realm->realm_id, realm->hall_vnum);

	send_to_char("Your guild is now a kingdom. Its realm holds no land yet.\r\n", ch);
	send_to_guild(guild, "The Royal Herald",
		      "Our guild is now a kingdom. The land around our hall awaits claiming.");

	return true;
}

/* ------------------------------------------------------------------ *
 * Claiming
 * ------------------------------------------------------------------ */

/* `kingdom claim`. Takes the NEXT square -- always exactly highest_claim + 1,
 * never a square of the player's choosing. The order is fixed precisely so
 * that ownership stays one integer, and letting a leader pick would drag back
 * the per-square ownership table the whole design exists to avoid. */
bool kingdom_claim_next(P_char ch)
{
	P_Guild guild = kingdom_actor_guild(ch);

	if (!guild)
	{
		return false;
	}

	kingdom_realm *realm = kingdom_actor_realm(ch, guild);

	if (!realm)
	{
		return false;
	}

	/* No expanding while the arrears ladder is running. A realm whose guards
	 * have despawned and whose outer ring is reverting for want of upkeep
	 * cannot coherently be buying more land at the same time, and allowing it
	 * would let a guild buy back the very ring it is losing. Pay first;
	 * kingdom_clear_arrears() reopens this. */
	if (realm->arrears != KARR_CURRENT)
	{
		send_to_char("Your realm owes upkeep. Settle the debt before taking more land.\r\n",
			     ch);
		return false;
	}

	if (realm->highest_claim >= KINGDOM_MAX_SQUARES)
	{
		send_to_char("Your realm already holds every square it can.\r\n", ch);
		return false;
	}

	const int index = realm->highest_claim + 1;

	/* THE RE-JUDGE. kingdom_judge_square() is the single authority on whether
	 * a square may belong to a realm; nothing here second-guesses it or
	 * caches its answer. The realm's own association is excluded so that a
	 * realm never collides with itself. */
	const int verdict = kingdom_judge_square(realm->hall_rnum, index,
						 (int)guild->get_racewar(), realm->assoc_id);

	if (verdict != KSQ_OK)
	{
		char why[MAX_INPUT_LENGTH];

		snprintf(why, sizeof(why), "Square %d of %d cannot be claimed (%s).\r\n", index,
			 KINGDOM_MAX_SQUARES, kingdom_verdict_text(verdict));
		send_to_char(why, ch);
		return false;
	}

	const long price = kingdom_claim_cost(index);

	/* Everything is validated. Only now does money move. A price of zero is
	 * a deliberate configuration -- both cost knobs set to 0 means land is
	 * free on this mud -- and must go through as a free claim rather than
	 * dead-end in the debit, which refuses non-positive amounts.
	 * kingdom_claim_cost() clamps away negatives, so zero is the only such
	 * case. */
	if (price > 0 && !kingdom_pay_from_treasury(guild, price))
	{
		char poor[MAX_INPUT_LENGTH];

		snprintf(poor, sizeof(poor),
			 "Your guild treasury cannot pay the %s that square costs.\r\n",
			 kingdom_price_string(price));
		send_to_char(poor, ch);
		return false;
	}

	realm->highest_claim = index;

	kingdom_refresh_index(*realm);
	kingdom_persist_realm(*realm);

	const int ring = kingdom_ring_for_index(index);
	const int claimed_rnum = kingdom_room_for_claim(realm->hall_rnum, index);
	const int claimed_vnum = kingdom_valid_rnum(claimed_rnum) ? world[claimed_rnum].number : 0;

	logit(LOG_KINGDOM,
	      "CLAIM: %s (assoc %d) took square %d (ring %d, vnum %d) for %ld copper.",
	      guild->get_name().c_str(), realm->assoc_id, index, ring, claimed_vnum, price);

	char told[MAX_INPUT_LENGTH];

	snprintf(told, sizeof(told),
		 "You claim square %d of %d for your realm. Ring %d of %d, %d squares held.\r\n",
		 index, KINGDOM_MAX_SQUARES, ring, KINGDOM_MAX_RING, realm->highest_claim);
	send_to_char(told, ch);

	snprintf(told, sizeof(told), "The realm claims new ground: %d of %d squares now held.",
		 realm->highest_claim, KINGDOM_MAX_SQUARES);
	send_to_guild(guild, "The Royal Herald", told);

	return true;
}

/* ------------------------------------------------------------------ *
 * Abandoning
 * ------------------------------------------------------------------ */

/* `kingdom abandon`. Gives up the LAST square held -- index highest_claim --
 * and only that one. Releasing any other square would punch a hole in a
 * territory whose entire representation assumes there are none.
 *
 * There is no refund. Ruling 6 makes a ring lost to arrears cost full price to
 * retake; a voluntary release that paid coin back would be strictly cheaper
 * than defaulting, and would turn the treasury into a way to park land value at
 * no risk. Deliberately allowed while in arrears, because shedding squares is
 * exactly how a realm shrinks its upkeep back to something it can pay. */
bool kingdom_abandon_last(P_char ch)
{
	P_Guild guild = kingdom_actor_guild(ch);

	if (!guild)
	{
		return false;
	}

	kingdom_realm *realm = kingdom_actor_realm(ch, guild);

	if (!realm)
	{
		return false;
	}

	if (realm->highest_claim < 1)
	{
		send_to_char("Your realm holds no land to give up.\r\n", ch);
		return false;
	}

	const int index = realm->highest_claim;

	/* Read the room BEFORE shrinking, while the square is still ours. */
	const int released_rnum = kingdom_room_for_claim(realm->hall_rnum, index);
	const int released_vnum =
		kingdom_valid_rnum(released_rnum) ? world[released_rnum].number : 0;

	realm->highest_claim = index - 1;

	kingdom_refresh_index(*realm);
	kingdom_persist_realm(*realm);

	logit(LOG_KINGDOM, "ABANDON: %s (assoc %d) released square %d (vnum %d); %d remain.",
	      guild->get_name().c_str(), realm->assoc_id, index, released_vnum,
	      realm->highest_claim);

	char told[MAX_INPUT_LENGTH];

	snprintf(told, sizeof(told),
		 "You give up square %d. Your realm holds %d of %d squares, and nothing is "
		 "refunded.\r\n",
		 index, realm->highest_claim, KINGDOM_MAX_SQUARES);
	send_to_char(told, ch);

	snprintf(told, sizeof(told), "The realm gives up ground: %d of %d squares now held.",
		 realm->highest_claim, KINGDOM_MAX_SQUARES);
	send_to_guild(guild, "The Royal Herald", told);

	return true;
}
