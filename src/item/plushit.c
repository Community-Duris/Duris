/*
   ***************************************************************************
   *  File: plushit.c                                         Part of Duris *
   *  Usage: enforce AFF3_SILVER / AFF3_PLUSONE..PLUSFIVE in melee          *
   ***************************************************************************
   *
   * WHAT WAS ALREADY THERE, AND WHAT WAS NOT
   * ----------------------------------------
   * defines.h:723-728 has carried these six bits for decades:
   *
   *     AFF3_SILVER    BIT_10   "Char needs silver to dam"
   *     AFF3_PLUSONE   BIT_11   "Char needs +1 to damage"
   *     ...
   *     AFF3_PLUSFIVE  BIT_15   "Char needs +5 to damage"
   *
   * An exhaustive search of master (2e4c95e8) finds them referenced in
   * exactly four places, none of which is combat:
   *
   *     common.c:714-719        the stat/setbit label table
   *     auction_houses.c:2078   the equipment search filters, which
   *                             describe the bits on gear as "can cut
   *                             lycanthropes" and "can hit ... magical
   *                             creatures"
   *     enhance.c:1288-1293    the item-enhancement grant table
   *     utility.c:5152          remove_plushit_bits(), which strips the
   *                             five plus bits from summoned and shaped
   *                             mobs (callers in innates.c, magic.c,
   *                             necromancy.c, sillusionist.c and three
   *                             specs.* files) so that a summon does not
   *                             inherit the protection of the thing it
   *                             copies
   *
   * That last one is the proof of intent: the engine goes out of its way
   * to remove a protection that is never enforced.
   *
   * WHAT THE CONTENT ACTUALLY SAYS  (measured over areas/ at 2e4c95e8)
   * ------------------------------------------------------------------
   * Mob sources (446 .mob files under areas/mob/, 20,127 records): FIVE carry
   * a plus requirement - forofcon.mob #139930/#139975/#140039 (PLUSFIVE),
   * mavulsk.mob #48 (PLUSONE), vecna.mob #130016 (PLUSFOUR).  ZERO carry
   * AFF3_SILVER.  Of the five, only vecna is listed in areas/AREA, so
   * exactly ONE flagged mob (#130016, Chressan) is in the world a stock
   * boot builds; the other four exist in unlisted zone files.
   *
   * Object sources (443 .obj files under areas/obj/, 21,656 records): 385
   * objects in 105 files carry ITEM2_SILVER (extra2 BIT_1, "Item harm
   * AFF_SILVER"), 199 of them weapons.  ONE object grants an attacker a
   * plus rating through bitvector3: heavens.obj #499 (PLUSFIVE), an
   * immortal-zone item.
   *
   * So the silver rule has a fully-authored weapon side and no monsters
   * to use it on; the plus rule has monsters and, outside the immortal
   * zone, no authored weapon side.  Enforcement therefore has to define
   * the weapon side, and it must not invent one that makes the flagged
   * mobs unkillable.
   *
   * THE RULE  (written out so a designer can read it without reading C)
   * ------------------------------------------------------------------
   * The rule is skipped entirely - it never blocks - when the victim is
   * a player or the attacker is trusted: the flags are authored on mobs
   * and are never enforced against players or immortals.  Otherwise:
   *
   *   victim_need = highest AFF3_PLUS* bit set on the victim  (0..5)
   *   weapon_plus = MAX of
   *                   - the largest APPLY_HITROLL modifier on the wielded
   *                     weapon itself, clamped to 0..5   <- the engine's
   *                     own historical mapping, item_enchant.c:272-299,
   *                     where a +1-hitroll weapon became ITEM2_PLUSONE
   *                     and so on
   *                   - 5 if the attacker carries AFF3_PLUSFIVE, 4 if
   *                     PLUSFOUR ... 1 if PLUSONE.  A worn item's
   *                     bitvector3 lands on affected_by3 through the
   *                     ordinary affect rebuild, so a builder may grant
   *                     the rating through gear - auction_houses.c:2079
   *                     already describes the bits on equipment exactly
   *                     that way
   *   blocked     = weapon_plus < victim_need
   *
   *   silver_need = victim has AFF3_SILVER
   *   silver_have = weapon has ITEM2_SILVER, or the weapon's material is
   *                 MAT_SILVER or MAT_MITHRIL, or the attacker carries
   *                 AFF3_SILVER
   *   blocked     = silver_need && !silver_have
   *
   * Bare-handed attacks count as weapon_plus 0 / no silver, which is the
   * point of the mechanic.  Only melee swings that resolve through hit()
   * are gated; spells, breath weapons and direct damage procs are
   * untouched.
   *
   * SHIPPING POSTURE
   * ----------------
   * Both halves default to OFF.  Turning the plus half on retunes the
   * flagged encounters, and that is the immortal's call, not this
   * patch's.  The switches are duris.properties keys read on every call
   * (get_property() is a bsearch of the in-memory table), so an immortal
   * can flip them on a running server - "properties set" when the key is
   * present in lib/duris.properties, or add the line and "properties
   * reload" - with no rebuild and no reboot:
   *
   *     combat.plushit.enforce   0
   *     combat.silver.enforce    0
   *
   * THREADING
   * ---------
   * Pure function of its arguments plus two property reads.  No state.
   */

#include <stdio.h>
#include <string.h>

#include "prototypes.h"
#include "structs.h"
#include "net/comm.h"
#include "item/objmisc.h"
#include "utils.h"
#include "utility.h"
#include "item/plushit.h"

/* highest plus-rating a character DEMANDS of its attackers, 0 = none */
static int plushit_required(P_char victim)
{
	if (!victim)
		return 0;
	if (IS_AFFECTED3(victim, AFF3_PLUSFIVE))
		return 5;
	if (IS_AFFECTED3(victim, AFF3_PLUSFOUR))
		return 4;
	if (IS_AFFECTED3(victim, AFF3_PLUSTHREE))
		return 3;
	if (IS_AFFECTED3(victim, AFF3_PLUSTWO))
		return 2;
	if (IS_AFFECTED3(victim, AFF3_PLUSONE))
		return 1;
	return 0;
}

/* plus-rating an attacker BRINGS, 0 = mundane */
static int plushit_available(P_char ch, P_obj weapon)
{
	int rating = 0, i;

	if (weapon)
	{
		for (i = 0; i < MAX_OBJ_AFFECT; i++)
		{
			if (weapon->affected[i].location == APPLY_HITROLL &&
			    (int)weapon->affected[i].modifier > rating)
				rating = (int)weapon->affected[i].modifier;
		}
		if (rating > 5)
			rating = 5;
		if (rating < 0)
			rating = 0;
	}

	/* a builder may state the rating outright through bitvector3 */
	if (ch)
	{
		int direct = plushit_required(ch); /* same bits, other side */

		if (direct > rating)
			rating = direct;
	}
	return rating;
}

static int silver_required(P_char victim)
{
	return victim && IS_AFFECTED3(victim, AFF3_SILVER);
}

static int silver_available(P_char ch, P_obj weapon)
{
	if (weapon)
	{
		if (IS_SET(weapon->extra2_flags, ITEM2_SILVER))
			return TRUE;
		if (weapon->material == MAT_SILVER || weapon->material == MAT_MITHRIL)
			return TRUE;
	}
	if (ch && IS_AFFECTED3(ch, AFF3_SILVER))
		return TRUE;
	return FALSE;
}

int plushit_blocks(P_char ch, P_char victim, P_obj weapon)
{
	int need;

	if (!ch || !victim)
		return FALSE;
	/* the flags are authored on mobs; never turn them on a player, and
	   never on an immortal's attack */
	if (IS_PC(victim) || IS_TRUSTED(ch))
		return FALSE;

	if (get_property("combat.silver.enforce", 0) && silver_required(victim) &&
	    !silver_available(ch, weapon))
	{
		act("Your blow passes through $N as if $E were not there - silver, and nothing else, will bite.",
		    FALSE, ch, 0, victim, TO_CHAR);
		act("$n's blow passes harmlessly through $N.", TRUE, ch, 0, victim, TO_ROOM);
		return TRUE;
	}

	if (get_property("combat.plushit.enforce", 0))
	{
		need = plushit_required(victim);
		if (need > 0 && plushit_available(ch, weapon) < need)
		{
			act("Your weapon is not enchanted enough to harm $N.", FALSE, ch, 0, victim,
			    TO_CHAR);
			act("$n's weapon glances off $N without leaving a mark.", TRUE, ch, 0,
			    victim, TO_ROOM);
			return TRUE;
		}
	}
	return FALSE;
}
