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
 *  is granted that has not been paid for. The persist step of any path that
 *  touched the treasury is kingdom_persist_payment() (kingdom_upkeep.c), and
 *  Both build modes make the pair crash-safe, by different mechanisms:
 *
 *      MariaDB    the debited guild and the realm record go out in ONE
 *                 transaction, so a crash between them can keep neither
 *                 without the other. This is the guarantee the design asks
 *                 for, and this is the mode that delivers it.
 *
 *      flat-file  the debited guild and realm catalogue after-images share
 *                 one recovery journal. The journal is durable before either
 *                 image, so a crash after one write is completed on the next
 *                 authority access rather than losing the paid change.
 *
 *  A write that fails outright leaves the claim standing in memory, flags the
 *  realm payment_pending, and the pair is retried by the upkeep sweep. While
 *  that flag is set nothing in this file may publish the realm record on its
 *  own; kingdom_persist_realm() carries the rule and the reason.
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

#include <cerrno>
#include <climits>
#include <cstdlib>

/* Not declared in any header the engine exports; kingdom_geometry.c takes the
 * same extern for the same reason. */
extern struct room_data *world;

/* ------------------------------------------------------------------ *
 * Prices
 * ------------------------------------------------------------------ */

/* `base` compounded by the configured growth (index - 1) times, clamped.
 *
 * The one place the claim curve is implemented: coin and material both ride
 * it, and a second copy of a price formula is exactly the sort of thing that
 * drifts until a square costs one amount to quote and another to buy. */
static long kingdom_compound(long base, int index)
{
	const long ceiling = (long)INT_MAX;
	long value = base > 0 ? base : 0;
	long growth = kingdom_cfg.claim_cost_growth_permille;

	/* Below 1000 land would get CHEAPER as a realm grew, inverting the whole
	 * point of the curve. 1000 exactly is the degenerate flat case and is
	 * allowed, because an operator may genuinely want a fixed price. */
	if (growth < 1000)
	{
		growth = 1000;
	}
	if (value > ceiling)
	{
		value = ceiling;
	}

	for (int step = 1; step < index; step++)
	{
		/* Tested BEFORE the multiply rather than after: `value * growth`
		 * overflowing is undefined behaviour, so it must not happen even
		 * once. Reaching the ceiling is not an error -- it prices land
		 * beyond any treasury that will ever exist, which is the honest
		 * answer to a runaway config. */
		if (value > ceiling / growth * 1000)
		{
			return ceiling;
		}

		value = (value * growth) / 1000;

		if (value > ceiling)
		{
			return ceiling;
		}
	}

	return value;
}

/* Copper price of claim `index` (1..KINGDOM_MAX_SQUARES), 0 outside it. A pure
 * function of the index, which is what makes retaking a reverted ring cost
 * exactly what it cost the first time. */
long kingdom_claim_cost(int index)
{
	if (index < 1 || index > KINGDOM_MAX_SQUARES)
	{
		return 0;
	}

	/* base * growth^(index-1), COMPOUNDING, as documented on kingdom_config.
	 *
	 * Both terms come from lib/kingdom.cfg, a hand-edited file, so they are
	 * clamped before any arithmetic touches them. The floor stops a negative
	 * entry turning the price into a payment TO the guild; the ceiling stops
	 * a compounding curve running away, which matters far more here than it
	 * did for the old linear one -- at the shipped x1.05 the eightieth square
	 * is about 47,200 platinum, but a typo of x2 would reach 2^79 and blow
	 * past a long well before index 80.
	 *
	 * Integer arithmetic end to end: growth is per mille, and each step
	 * multiplies then divides, so two servers always agree on a price. The
	 * rounding-down that costs is a few copper against a true exponential,
	 * which is a fair trade for an answer that does not depend on a float
	 * unit's mood. */
	return kingdom_compound(kingdom_cfg.claim_cost_base, index);
}

/* Units of EVERY resource that claim `index` costs on top of the coin, or 0
 * for an index the geometry does not know.
 *
 * Ruled 2026-09-03: land is bought with worked material as well as money, and
 * with all four kinds, so a realm cannot expand on one sort of ground alone --
 * it must send people out across the whole map. The amount rides the same
 * curve as the coin, so the last square is as much harder in labour as it is
 * in treasure.
 *
 * SCALED OFF THE COIN, NOT COMPOUNDED SEPARATELY -- and that is a bug fix, not
 * a preference. kingdom_compound() truncates to a whole unit after every step,
 * which costs nothing on a base of a million copper but is a ~1% loss per step
 * on a base of 25: compounding the material directly gave 723 at square 80
 * where the curve calls for 1,180, THIRTY-NINE PER CENT short, and the drift
 * grew with the index so the late rings -- the ones the material cost exists
 * to make hard -- were the ones it let off. The help text and the config file
 * both quote the true curve, so the code was the thing that was wrong.
 *
 * Multiplying first and dividing once keeps a single rounding at the end.
 * kingdom_claim_cost() is clamped to INT_MAX, so the product is at most
 * material_base * 2^31, which is why this arithmetic is done in long and why
 * the base is bounded (KINGDOM_CLAIM_MATERIAL_MAX) rather than trusted. */
long kingdom_claim_material_cost(int index)
{
	if (index < 1 || index > KINGDOM_MAX_SQUARES)
	{
		return 0;
	}

	const long base = kingdom_cfg.claim_material_base;

	if (base <= 0)
	{
		return 0; /* land costs coin alone on this mud */
	}

	const long first = kingdom_claim_cost(1);

	/* Land is free on this mud, so the coin curve carries no shape to scale
	 * against. Fall back to the flat base rather than dividing by zero: an
	 * operator who zeroed the coin and left the material set meant the
	 * material to still cost something. */
	if (first <= 0)
	{
		return base;
	}

	return base * kingdom_claim_cost(index) / first;
}

/* Copper price of every square in `ring` (1..KINGDOM_MAX_RING) taken
 * together: the sum of kingdom_claim_cost() over the ring's index range,
 * or 0 for a ring the geometry does not know. */
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

/* Debit `price` copper from the guild's IN-MEMORY treasury. True only if the
 * whole price was taken; sub_copper() checks before it mutates, so a refusal
 * is atomic and leaves no ledger line.
 *
 * NO REFUND, NO SAVE. Once this answers true the coin is gone: nothing that
 * follows in a claim can fail in a way that hands it back (ruling 6, and the
 * file header). And sub_copper() adjusts the balances and writes the ledger
 * but does not write the guild out -- that is deliberately left to
 * kingdom_persist_payment(), which pairs the guild write with the realm
 * record as tightly as the build mode allows (see the file header). A save
 * here would publish the debit on its own, ahead of the realm record in
 * every mode, and widen the very window that pairing exists to narrow. */
static bool kingdom_pay_from_treasury(P_Guild guild, long price)
{
	if (!guild || price <= 0)
	{
		return false;
	}

	return guild->sub_copper(price);
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

/* The guild's MAIN hall comes from kingdom_main_hall() in kingdom.c -- the
 * module's one finder, declared in kingdom_internal.h. */

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

/* Persist a realm whose state changed WITHOUT any coin moving -- abandon is
 * the one such path here. Money-bearing paths must not use this: they go
 * through kingdom_persist_payment(), which writes the guild alongside.
 *
 * THE PENDING RULE, which every kingdom_db_save_realm() caller outside
 * kingdom_db.c and kingdom_persist_payment() obeys: payment_pending means a
 * debit this realm has already taken is still only in memory, unpaired with
 * the guild that owes it. Writing the record now would publish the realm's
 * paid-up mark and its square count while that debit is still unwritten --
 * precisely the half-state the paired write exists to prevent, and it would
 * arrive by a path that never charged anything. So a pending realm is left
 * dirty and NOT written, whatever else changed about it. The change stands in
 * memory meanwhile, and it is carried out with the guild once
 * kingdom_upkeep_retry_pending() lands the pair -- whatever the record holds
 * by then, this abandon's shrink included, since the record is written whole.
 *
 * True when the record is on disk. False -- held for a pending payment, or a
 * write that failed -- leaves the realm dirty and logs everything needed to
 * reconstruct the write by hand. */
static bool kingdom_persist_realm(kingdom_realm &realm)
{
	realm.dirty = true;

	if (realm.payment_pending)
	{
		logit(LOG_KINGDOM,
		      "HELD: realm %d (assoc %d, hall vnum %d, %d squares) has a payment pending; "
		      "the record is left dirty for the paired retry, not published alone.",
		      realm.realm_id, realm.assoc_id, realm.hall_vnum, realm.highest_claim);
		return false;
	}

	if (kingdom_db_save_realm(realm))
	{
		realm.dirty = false;
		return true;
	}

	logit(LOG_KINGDOM,
	      "SAVE FAILED: realm %d (assoc %d, hall vnum %d, %d squares) left dirty for retry.",
	      realm.realm_id, realm.assoc_id, realm.hall_vnum, realm.highest_claim);
	return false;
}

/* Make a realm change durable TOGETHER WITH the guild whose treasury it
 * belongs to, by way of kingdom_persist_payment().
 *
 * True means both records are durable: under MariaDB they share one database
 * transaction; under the flat-file build their after-images share one recovery
 * journal. The file header spells out the retry behavior in each mode.
 *
 * False means the change stands in memory (any debit is kept -- there is no
 * refund path), kingdom_persist_payment() has flagged the realm
 * payment_pending so no flush publishes the record alone, and the pair will
 * be retried by kingdom_upkeep_retry_pending(); the failure is logged here
 * with the verb that caused it. The actor is told by
 * kingdom_tell_record_pending(), after the verb's own message. */
static bool kingdom_persist_paid_change(P_Guild guild, kingdom_realm &realm, const char *verb)
{
	if (kingdom_persist_payment(guild, realm))
	{
		return true;
	}

	logit(LOG_KINGDOM,
	      "%s: realm %d (assoc %d, hall vnum %d, %d squares) could not be written with its "
	      "guild; left payment_pending for retry.",
	      verb, realm.realm_id, realm.assoc_id, realm.hall_vnum, realm.highest_claim);
	return false;
}

/* Tell the actor that the verb took effect but its record is not on disk
 * yet and will be retried. Sent AFTER the verb's own message, so the player
 * learns what happened before learning that it is still pending. */
static void kingdom_tell_record_pending(P_char ch)
{
	send_to_char("The change stands, but your realm's records could not be written just "
		     "now; they will be retried shortly.\r\n",
		     ch);
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
		logit(LOG_KINGDOM, "CONVERT: realm for assoc %d vanished immediately after insert.",
		      assoc_id);
		send_to_char("Something went wrong founding your realm.\r\n", ch);
		return false;
	}

	kingdom_refresh_index(*realm);

	/* Conversion moves no coin, but the founding record is written through
	 * the paired path all the same: the guild row and its brand-new realm row
	 * go out together, as tightly as the build mode allows, and a founding
	 * that could not be written is retried with its guild by the upkeep sweep
	 * rather than published alone by the generic flush. The realm exists in
	 * memory either way -- the guild IS a kingdom from this line on, whatever
	 * the disk says yet. */
	const bool durable = kingdom_persist_paid_change(guild, *realm, "CONVERT");

	logit(LOG_KINGDOM, "CONVERT: %s (assoc %d) founded realm %d on hall vnum %d%s.",
	      guild->get_name().c_str(), assoc_id, realm->realm_id, realm->hall_vnum,
	      durable ? "" : " (record pending)");

	send_to_char("Your guild is now a kingdom. Its realm holds no land yet.\r\n", ch);
	if (!durable)
	{
		kingdom_tell_record_pending(ch);
	}
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
	 * kingdom_clear_arrears() reopens this.
	 *
	 * UNLESS NOTHING IS OWED. A realm the ladder has stripped to its last
	 * square, or to none, owes nothing -- upkeep scales with squares held --
	 * and there is no payment left that could ever clear the rung. Refusing
	 * it would wedge the realm out of the game for good: it cannot pay, so it
	 * cannot claim, so it can never again have anything to pay for. The
	 * refusal stands whenever a debt is actually outstanding, and a claim let
	 * through by this exception steps the realm off the ladder below. */
	const bool on_the_ladder = (realm->arrears != KARR_CURRENT);

	if (on_the_ladder && kingdom_upkeep_due(*realm) > 0)
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
	const int verdict = kingdom_judge_square(realm->hall_rnum, index, (int)guild->get_racewar(),
						 realm->assoc_id);

	if (verdict != KSQ_OK)
	{
		char why[MAX_INPUT_LENGTH];

		snprintf(why, sizeof(why), "Square %d of %d cannot be claimed (%s).\r\n", index,
			 KINGDOM_MAX_SQUARES, kingdom_verdict_text(verdict));
		send_to_char(why, ch);
		return false;
	}

	const long price = kingdom_claim_cost(index);
	const long material = kingdom_claim_material_cost(index);

	long bill[KRES_MAX];

	for (int res = 0; res < KRES_MAX; res++)
	{
		bill[res] = material;
	}

	/* MATERIAL IS CHECKED BEFORE COIN MOVES, and that order is the whole of
	 * the safety here. Ruling 6 says nothing in this module refunds, so a
	 * claim that debited the treasury and only then discovered the realm was
	 * two logs short would burn the coin for nothing. The store is read
	 * first, refused loudly with the shortfall named, and only spent once the
	 * coin is already gone -- at which point kingdom_resource_spend() cannot
	 * fail, because nothing between the two runs a tick or another command. */
	if (material > 0)
	{
		/* Four clauses at most, each "999999 more mineral" and a comma;
		 * sized for that rather than for MAX_INPUT_LENGTH so the refusal
		 * it feeds cannot be said to overrun its own buffer. */
		char lack[192];

		lack[0] = '\0';

		for (int res = 0; res < KRES_MAX; res++)
		{
			if (realm->resources[res] >= material)
			{
				continue;
			}

			char one[64];

			snprintf(one, sizeof(one), "%s%ld more %s", lack[0] ? ", " : "",
				 material - realm->resources[res], kingdom_resource_name(res));
			strncat(lack, one, sizeof(lack) - strlen(lack) - 1);
		}

		if (lack[0])
		{
			char poor[MAX_INPUT_LENGTH];

			snprintf(poor, sizeof(poor),
				 "That square wants %ld of every worked resource as well as coin. "
				 "Your realm needs %s.\r\n",
				 material, lack);
			send_to_char(poor, ch);
			return false;
		}
	}

	/* Everything is validated. Only now does money move. A price of zero is
	 * a deliberate configuration -- the cost knobs set to 0 means land is
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

	/* Checked above, so this cannot refuse for want of material; it can still
	 * answer false on a mud configured for no material at all, which is why
	 * the call is gated on the same condition the check was. */
	if (material > 0)
	{
		kingdom_resource_spend(*realm, bill);
	}

	realm->highest_claim = index;

	/* The claim was allowed while a rung was set, which by the test above can
	 * only mean nothing was owed. Step off the ladder now rather than leave a
	 * realm that owes nothing sitting on a rung that forbids its guards and
	 * keeps its nodes dormant until some later sweep happens to notice: the
	 * square it has just bought would arrive ungarrisoned and barren, and
	 * kingdom_upkeep_due() is what decides whether a debt exists, not the
	 * rung. Done BEFORE the persist so the cleared rung goes out in the same
	 * record, and before the garrison reconcile so kingdom_guard_allowance()
	 * reads a realm that is current. */
	if (on_the_ladder)
	{
		kingdom_clear_arrears(*realm);
	}

	kingdom_refresh_index(*realm);

	/* The coin is gone and the square is ours in memory; now make the two
	 * facts durable as one. A free claim (price 0) takes the same path: the
	 * guild write is then a no-op in effect, and one persist path for every
	 * claim beats a second one that is only exercised on muds where land is
	 * free. */
	const bool durable = kingdom_persist_paid_change(guild, *realm, "CLAIM");

	const int ring = kingdom_ring_for_index(index);
	const int claimed_rnum = kingdom_room_for_claim(realm->hall_rnum, index);
	const int claimed_vnum = kingdom_valid_rnum(claimed_rnum) ? world[claimed_rnum].number : 0;

	logit(LOG_KINGDOM,
	      "CLAIM: %s (assoc %d) took square %d (ring %d, vnum %d) for %ld copper and %ld of "
	      "each resource%s.",
	      guild->get_name().c_str(), realm->assoc_id, index, ring, claimed_vnum, price,
	      material, durable ? "" : " (record pending)");

	char told[MAX_INPUT_LENGTH];

	snprintf(told, sizeof(told),
		 "You claim square %d of %d for your realm. Ring %d of %d, %d squares held.\r\n",
		 index, KINGDOM_MAX_SQUARES, ring, KINGDOM_MAX_RING, realm->highest_claim);
	send_to_char(told, ch);
	if (!durable)
	{
		kingdom_tell_record_pending(ch);
	}

	snprintf(told, sizeof(told), "The realm claims new ground: %d of %d squares now held.",
		 realm->highest_claim, KINGDOM_MAX_SQUARES);
	send_to_guild(guild, "The Royal Herald", told);

	/* The land changed, so the world must agree with it at once rather than
	 * on the next sweep: the garrison's allowance grows with the square
	 * count, and a harvest node standing on the square just taken is now on
	 * land a realm controls, which ruling 1 forbids. Both happen AFTER the
	 * index refresh, because both read kingdom_owner_of_room().
	 *
	 * They also happen AFTER the messages above, and that ordering is the
	 * point: kingdom_guards_refresh() announces a muster to the whole guild,
	 * and running it earlier had the guild reading "fresh guards muster"
	 * before the claimer had been told the claim went through and before
	 * anyone had been told there was new land to garrison. Cause is
	 * announced first, consequence second. */
	kingdom_garrison_refresh(*realm);
	if (kingdom_valid_rnum(claimed_rnum))
	{
		kingdom_node_reap_room(claimed_rnum);
	}

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
	const int released_vnum = kingdom_valid_rnum(released_rnum) ? world[released_rnum].number :
								      0;

	realm->highest_claim = index - 1;

	kingdom_refresh_index(*realm);

	/* No coin moved, so the realm record travels alone: kingdom_persist_realm
	 * and the generic dirty flush, not the paired payment path. It refuses to
	 * write at all while a payment is pending -- an abandon must not be the
	 * thing that publishes an unpaired debit -- and the actor is then told the
	 * record is still to come, which is true either way. */
	const bool durable = kingdom_persist_realm(*realm);

	/* Fewer squares may mean fewer guards allowed; reconcile now so a guard
	 * never stands on ground the realm has just given up. */
	kingdom_garrison_refresh(*realm);

	logit(LOG_KINGDOM, "ABANDON: %s (assoc %d) released square %d (vnum %d); %d remain%s.",
	      guild->get_name().c_str(), realm->assoc_id, index, released_vnum,
	      realm->highest_claim, durable ? "" : " (record pending)");

	char told[MAX_INPUT_LENGTH];

	snprintf(told, sizeof(told),
		 "You give up square %d. Your realm holds %d of %d squares, and nothing is "
		 "refunded.\r\n",
		 index, realm->highest_claim, KINGDOM_MAX_SQUARES);
	send_to_char(told, ch);

	if (!durable)
	{
		kingdom_tell_record_pending(ch);
	}

	snprintf(told, sizeof(told), "The realm gives up ground: %d of %d squares now held.",
		 realm->highest_claim, KINGDOM_MAX_SQUARES);
	send_to_guild(guild, "The Royal Herald", told);

	return true;
}

/* ------------------------------------------------------------------ *
 * The garrison roster
 * ------------------------------------------------------------------ *
 * Ruled 2026-09-04. Guards are BOUGHT, one at a time, and promoted with the
 * realm's completed rings. The three verbs live here rather than in
 * kingdom_guards.c because each one spends the treasury and must land the
 * realm and the guild as a single paired write -- which is what this file is
 * for -- while kingdom_guards.c owns the arithmetic (what a promotion costs,
 * how high the land allows) and the standing-up of bodies.
 *
 * Nothing here despawns or spawns a guard directly. Every verb ends with
 * kingdom_guards_refresh(), which reconciles the world against the roster it
 * has just changed; a hire therefore makes a guard appear and a promotion
 * replaces the old body with a stronger one, both through the one
 * reconciliation path that also handles boot, arrears and lost land.
 */

/* `kingdom roster`. Read-only, and the only roster verb any member may run:
 * seeing who the guild is paying for is not a leader's privilege. */
void kingdom_roster_show(P_char ch)
{
	if (!ch || IS_NPC(ch) || !kingdom_enabled())
	{
		return;
	}

	P_Guild guild = GET_ASSOC(ch);

	if (!guild || !IS_MEMBER(GET_A_BITS(ch)) || !GT_PAROLE(GET_A_BITS(ch)))
	{
		send_to_char("You do not belong to a guild.\r\n", ch);
		return;
	}

	kingdom_realm *realm = kingdom_find_realm((int)guild->get_id());

	if (!realm)
	{
		send_to_char("Your guild is not a kingdom.\r\n", ch);
		return;
	}

	const int cap = kingdom_guard_level_cap(*realm);
	const int permitted = kingdom_guard_allowance(*realm);
	const int bought = kingdom_roster_count(*realm);

	char line[MAX_INPUT_LENGTH];

	send_to_char("&+WThe realm's garrison&n\r\n\r\n", ch);

	if (bought == 0)
	{
		send_to_char("  No guards on the books.\r\n", ch);
	}
	else
	{
		send_to_char("   &+w#  class         level  to promote&n\r\n", ch);
	}

	for (int slot = 0; slot < KINGDOM_GUARD_SLOTS; slot++)
	{
		if (realm->guards[slot].level <= 0)
		{
			continue;
		}

		const int level = realm->guards[slot].level;
		const int next = kingdom_guard_next_level(level);
		const long step = kingdom_guard_promotion_cost(level, next);

		if (level >= cap)
		{
			snprintf(line, sizeof(line), "  %2d  %-12s  %5d  %s\r\n", slot + 1,
				 kingdom_guard_class_name(realm->guards[slot].guard_class), level,
				 level >= KINGDOM_GUARD_TOP_LEVEL ?
					 "at the highest rank there is" :
					 "held back by the realm's land");
		}
		else
		{
			snprintf(line, sizeof(line), "  %2d  %-12s  %5d  %s to level %d\r\n",
				 slot + 1,
				 kingdom_guard_class_name(realm->guards[slot].guard_class), level,
				 kingdom_price_string(step), next);
		}
		send_to_char(line, ch);
	}

	if (realm->champion_class)
	{
		/* The champion is not slot 17 of the same list -- it is not
		 * promoted, not replaced and not counted against the land's
		 * allowance -- so it gets its own line rather than a row that
		 * would invite 'kingdom promote 17'. */
		/* champion_class is exactly two CLASS_* bits, set together by
		 * kingdom_roster_champion() and never changed. Split into the
		 * lower bit and the remainder to name them both; with two bits
		 * set the remainder IS the higher one. */
		const int lower = realm->champion_class & -realm->champion_class;
		const int higher = realm->champion_class & ~lower;

		snprintf(line, sizeof(line),
			 "\r\n  &+YChampion&n: level %d, sworn to %s and %s.\r\n",
			 KINGDOM_CHAMPION_LEVEL, kingdom_guard_class_name(lower),
			 kingdom_guard_class_name(higher));
		send_to_char(line, ch);
	}
	else if (realm->highest_claim >= KINGDOM_MAX_SQUARES)
	{
		snprintf(line, sizeof(line),
			 "\r\n  Your realm holds every square: it may raise a champion for %s "
			 "('&+Wkingdom champion <class> <class>&n').\r\n",
			 kingdom_price_string(KINGDOM_CHAMPION_COST));
		send_to_char(line, ch);
	}

	snprintf(line, sizeof(line),
		 "\r\n  %d bought, %d the land permits, level %d the highest the land allows.\r\n",
		 bought, permitted, cap);
	send_to_char(line, ch);

	if (bought < permitted)
	{
		snprintf(line, sizeof(line),
			 "  A new guard costs %s: '&+Wkingdom hire <class>&n'.\r\n",
			 kingdom_price_string(kingdom_cfg.guard_cost_base));
		send_to_char(line, ch);
	}
	else if (bought >= KINGDOM_GUARD_SLOTS)
	{
		send_to_char("  The roster is full.\r\n", ch);
	}
	else
	{
		send_to_char("  More land is needed before another guard may be raised.\r\n", ch);
	}

	send_to_char("  '&+Wkingdom promote <#> [class]&n' raises one to the next realm tier. "
		     "A guard's rank never falls.\r\n",
		     ch);
}

/* `kingdom hire <class>`. One guard, at the base price, at the prototype's own
 * level. The class is chosen now and may be changed at each promotion. */
void kingdom_roster_hire(P_char ch, char *rest)
{
	P_Guild guild = kingdom_actor_guild(ch);

	if (!guild)
	{
		return;
	}

	kingdom_realm *realm = kingdom_actor_realm(ch, guild);

	if (!realm)
	{
		return;
	}

	char classes[256];

	kingdom_guard_class_list(classes, sizeof(classes));

	char wanted[MAX_INPUT_LENGTH];

	one_argument(rest, wanted);

	const int guard_class = kingdom_guard_class_by_name(wanted);

	if (!guard_class)
	{
		char say[MAX_STRING_LENGTH];

		snprintf(say, sizeof(say),
			 "Name the guard's calling: %s.\r\nFor example '&+Wkingdom hire "
			 "warrior&n'.\r\n",
			 classes);
		send_to_char(say, ch);
		return;
	}

	const int permitted = kingdom_guard_allowance(*realm);
	const int bought = kingdom_roster_count(*realm);

	if (bought >= KINGDOM_GUARD_SLOTS)
	{
		send_to_char("The roster is full; no realm may keep more.\r\n", ch);
		return;
	}

	if (bought >= permitted)
	{
		char say[MAX_INPUT_LENGTH];

		snprintf(say, sizeof(say),
			 "Your land supports %d guard(s) and you have %d. Claim more ground "
			 "before raising another.\r\n",
			 permitted, bought);
		send_to_char(say, ch);
		return;
	}

	/* The first empty slot, so a guard killed off the books is replaced in
	 * the number the roster listing already showed rather than appended at
	 * the end and renumbering everything below it. */
	int slot = -1;

	for (int i = 0; i < KINGDOM_GUARD_SLOTS; i++)
	{
		if (realm->guards[i].level <= 0)
		{
			slot = i;
			break;
		}
	}

	if (slot < 0)
	{
		send_to_char("The roster is full; no realm may keep more.\r\n", ch);
		return;
	}

	const long price = kingdom_cfg.guard_cost_base;

	if (price > 0 && !kingdom_pay_from_treasury(guild, price))
	{
		char poor[MAX_INPUT_LENGTH];

		snprintf(poor, sizeof(poor),
			 "Your guild treasury cannot pay the %s a guard costs.\r\n",
			 kingdom_price_string(price));
		send_to_char(poor, ch);
		return;
	}

	realm->guards[slot].guard_class = guard_class;
	realm->guards[slot].level = KINGDOM_GUARD_BASE_LEVEL;

	const bool durable = kingdom_persist_paid_change(guild, *realm, "HIRE");

	char told[MAX_INPUT_LENGTH];

	snprintf(told, sizeof(told),
		 "A %s takes the realm's coin and the realm's oath. Guard #%d, level %d.\r\n",
		 kingdom_guard_class_name(guard_class), slot + 1, KINGDOM_GUARD_BASE_LEVEL);
	send_to_char(told, ch);

	if (!durable)
	{
		kingdom_tell_record_pending(ch);
	}

	snprintf(told, sizeof(told), "A %s is sworn into the realm's garrison.",
		 kingdom_guard_class_name(guard_class));
	send_to_guild(guild, "The Kingdom Marshal", told);

	logit(LOG_KINGDOM, "HIRE: %s (assoc %d) raised guard %d, %s, for %ld copper%s.",
	      guild->get_name().c_str(), realm->assoc_id, slot + 1,
	      kingdom_guard_class_name(guard_class), price, durable ? "" : " (record pending)");

	/* The body appears here, through the one reconciliation path. */
	kingdom_garrison_refresh(*realm);
}

/* `kingdom promote <#> [class]`. Two levels, at the price of one tier, up to
 * whatever the realm's completed rings allow. The class may be changed at the
 * same time -- a promotion is the natural moment to re-school a guard -- but
 * the LEVEL never falls, which is why there is no demote verb and no way to
 * spend the difference back. */
void kingdom_roster_upgrade(P_char ch, char *rest)
{
	P_Guild guild = kingdom_actor_guild(ch);

	if (!guild)
	{
		return;
	}

	kingdom_realm *realm = kingdom_actor_realm(ch, guild);

	if (!realm)
	{
		return;
	}

	char which[MAX_INPUT_LENGTH];
	char wanted[MAX_INPUT_LENGTH];

	rest = one_argument(rest, which);
	one_argument(rest, wanted);

	char *end = NULL;
	errno = 0;
	const long typed = strtol(which, &end, 10);
	const int slot = errno || end == which || *end != '\0' || typed < 1 ||
					 typed > KINGDOM_GUARD_SLOTS ?
				 -1 :
				 static_cast<int>(typed) - 1;

	if (slot < 0 || slot >= KINGDOM_GUARD_SLOTS || realm->guards[slot].level <= 0)
	{
		send_to_char("Name the guard by its number on '&+Wkingdom roster&n', as "
			     "'&+Wkingdom promote 3&n'.\r\n",
			     ch);
		return;
	}

	/* An empty second word keeps the guard's present calling; a named one
	 * must be a calling a guard may take. */
	int guard_class = realm->guards[slot].guard_class;

	if (*wanted)
	{
		guard_class = kingdom_guard_class_by_name(wanted);

		if (!guard_class)
		{
			char classes[256];
			char say[MAX_STRING_LENGTH];

			kingdom_guard_class_list(classes, sizeof(classes));
			snprintf(say, sizeof(say),
				 "No guard takes that calling. Choose from: %s.\r\n", classes);
			send_to_char(say, ch);
			return;
		}
	}

	const int level = realm->guards[slot].level;
	const int cap = kingdom_guard_level_cap(*realm);
	const int to = kingdom_guard_next_level(level);

	if (to <= level)
	{
		send_to_char("That guard stands at the highest rank a realm can raise.\r\n", ch);
		return;
	}

	if (to > cap)
	{
		char say[MAX_INPUT_LENGTH];

		snprintf(say, sizeof(say),
			 "Your realm's land supports level %d, and that guard is already there. "
			 "Complete the next ring first.\r\n",
			 cap);
		send_to_char(say, ch);
		return;
	}

	const long price = kingdom_guard_promotion_cost(level, to);

	if (price > 0 && !kingdom_pay_from_treasury(guild, price))
	{
		char poor[MAX_INPUT_LENGTH];

		snprintf(poor, sizeof(poor),
			 "Your guild treasury cannot pay the %s that promotion costs.\r\n",
			 kingdom_price_string(price));
		send_to_char(poor, ch);
		return;
	}

	const int was_class = realm->guards[slot].guard_class;

	realm->guards[slot].level = to;
	realm->guards[slot].guard_class = guard_class;

	const bool durable = kingdom_persist_paid_change(guild, *realm, "PROMOTE");

	char told[MAX_INPUT_LENGTH];

	if (guard_class != was_class)
	{
		snprintf(told, sizeof(told),
			 "Guard #%d is raised to level %d and re-schooled from %s to %s.\r\n",
			 slot + 1, to, kingdom_guard_class_name(was_class),
			 kingdom_guard_class_name(guard_class));
	}
	else
	{
		snprintf(told, sizeof(told), "Guard #%d is raised to level %d.\r\n", slot + 1, to);
	}
	send_to_char(told, ch);

	if (!durable)
	{
		kingdom_tell_record_pending(ch);
	}

	snprintf(told, sizeof(told), "A %s of the garrison is raised to level %d.",
		 kingdom_guard_class_name(guard_class), to);
	send_to_guild(guild, "The Kingdom Marshal", told);

	logit(LOG_KINGDOM, "PROMOTE: %s (assoc %d) raised guard %d to %d (%s) for %ld copper%s.",
	      guild->get_name().c_str(), realm->assoc_id, slot + 1, to,
	      kingdom_guard_class_name(guard_class), price, durable ? "" : " (record pending)");

	/* The old body no longer matches its roster line's level, so the
	 * reconciler replaces it with one that does. */
	kingdom_garrison_refresh(*realm);
}

/*
 * `kingdom champion <class> <class>`. One per realm, and only for a realm that
 * holds all eighty squares.
 *
 * TWO CALLINGS, NOT ONE. The champion is the module's only multiclass mob, so
 * the verb takes two class names and refuses one -- a realm that wants a plain
 * warrior has fifteen guard slots for that. They must differ, because a
 * multiclass of one thing is just that thing at a higher price.
 *
 * There is no upgrade path and no refund: the champion is level 60 the moment
 * it is raised and stays there. Losing a square sends it home rather than
 * unmaking it, and it takes the field again when the eightieth square is
 * retaken -- the guild does not pay twice for land it already bought once.
 */
void kingdom_roster_champion(P_char ch, char *rest)
{
	P_Guild guild = kingdom_actor_guild(ch);

	if (!guild)
	{
		return;
	}

	kingdom_realm *realm = kingdom_actor_realm(ch, guild);

	if (!realm)
	{
		return;
	}

	if (realm->champion_class)
	{
		send_to_char("Your realm already has its champion. There is only ever one.\r\n",
			     ch);
		return;
	}

	if (realm->highest_claim < KINGDOM_MAX_SQUARES)
	{
		char say[MAX_INPUT_LENGTH];

		snprintf(say, sizeof(say),
			 "A champion answers only to a realm that holds every square. Yours "
			 "holds %d of %d.\r\n",
			 realm->highest_claim, KINGDOM_MAX_SQUARES);
		send_to_char(say, ch);
		return;
	}

	char first[MAX_INPUT_LENGTH];
	char second[MAX_INPUT_LENGTH];

	rest = one_argument(rest, first);
	one_argument(rest, second);

	const int class_one = kingdom_guard_class_by_name(first);
	const int class_two = kingdom_guard_class_by_name(second);

	if (!class_one || !class_two || class_one == class_two)
	{
		char classes[256];
		char say[MAX_STRING_LENGTH];

		kingdom_guard_class_list(classes, sizeof(classes));
		snprintf(say, sizeof(say),
			 "A champion is sworn to TWO callings, and they must differ. Choose from: "
			 "%s.\r\nFor example '&+Wkingdom champion warrior cleric&n'.\r\n",
			 classes);
		send_to_char(say, ch);
		return;
	}

	const long price = KINGDOM_CHAMPION_COST;

	if (!kingdom_pay_from_treasury(guild, price))
	{
		char poor[MAX_INPUT_LENGTH];

		snprintf(poor, sizeof(poor),
			 "Your guild treasury cannot pay the %s a champion costs.\r\n",
			 kingdom_price_string(price));
		send_to_char(poor, ch);
		return;
	}

	realm->champion_class = class_one | class_two;

	const bool durable = kingdom_persist_paid_change(guild, *realm, "CHAMPION");

	char told[MAX_INPUT_LENGTH];

	snprintf(told, sizeof(told),
		 "The realm raises its champion: a %s and %s both, at level %d.\r\n",
		 kingdom_guard_class_name(class_one), kingdom_guard_class_name(class_two),
		 KINGDOM_CHAMPION_LEVEL);
	send_to_char(told, ch);

	if (!durable)
	{
		kingdom_tell_record_pending(ch);
	}

	send_to_guild(guild, "The Kingdom Marshal",
		      "The realm raises a champion, and a banner with it.");

	logit(LOG_KINGDOM, "CHAMPION: %s (assoc %d) raised a %s/%s champion for %ld copper%s.",
	      guild->get_name().c_str(), realm->assoc_id, kingdom_guard_class_name(class_one),
	      kingdom_guard_class_name(class_two), price, durable ? "" : " (record pending)");

	kingdom_champion_refresh(*realm);
}
