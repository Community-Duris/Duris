/*
 *  kingdom_craft_math.h
 *  Duris
 *
 *  The guild store's arithmetic and nothing else: what a piece costs in
 *  platinum, how much realm material it draws, how a stat line scales with the
 *  level it is made at, and the weapon dice for each level band.
 *
 *  Pure integer functions with no engine dependency, on purpose: the approved
 *  worked examples are executed by tests/async/kingdom_craft_math_harness.cpp,
 *  which compiles this header on its own without linking a server, and
 *  kingdom_craft.c static_asserts a few of them so drift is a build error.
 *
 *  Private to src/kingdom/, like kingdom_internal.h. Every formula is written
 *  out in words above its function so a designer can read it without reading
 *  the C++.
 *
 *  WEIGHTS ARE IN TENTHS. The approved item weights are 1.5 (a one-handed
 *  weapon or body armour), 2.0 (a two-handed weapon), 1.2 (a shield), 1.0 (any
 *  other armour or cloth piece, cloak or belt) and 0.8 (jewellery); they are
 *  passed here as 15, 20, 12, 10 and 8 so the whole curve stays integer and two
 *  servers always agree on a price.
 *
 *  SCALES ARE PER MILLE, like kingdom.claim.cost.growth.permille: 1000 is the
 *  approved curve, 2000 doubles it, 0 makes it free.
 */

#ifndef _KINGDOM_CRAFT_MATH_H_
#define _KINGDOM_CRAFT_MATH_H_

/* The highest level store gear is ever made at: a mortal's ceiling. A buyer
 * above it (an immortal) still gets level-56 work. */
constexpr int KINGDOM_CRAFT_TOP_LEVEL = 56;

/*
 * THE LEVEL AN ITEM IS MADE AT
 *
 *     item level = the buyer's own level, but never above 56 and never below 1
 *
 * This is the whole of the ruling "a level 10 must not be able to buy level-56
 * gear": nothing a buyer can type reaches this number except their own level.
 */
constexpr int kingdom_craft_item_level(int buyer_level)
{
	if (buyer_level < 1)
		return 1;
	if (buyer_level > KINGDOM_CRAFT_TOP_LEVEL)
		return KINGDOM_CRAFT_TOP_LEVEL;
	return buyer_level;
}

/*
 * PRICE, in whole platinum
 *
 *     price = round( W x (2 + L) x scale )
 *
 *     W      the item's weight (1.5 one-handed/body, 2.0 two-handed, 1.2 shield,
 *            1.0 other pieces, 0.8 jewellery)
 *     L      the level the item is made at (the buyer's, capped at 56)
 *     scale  kingdom.craft.price.permille / 1000
 *
 * At W = 1: level 10 costs 12p, level 30 32p, level 56 58p.
 * Rounds half up. The platinum is DESTROYED on purchase, not banked anywhere.
 */
constexpr long kingdom_craft_price_platinum(int weight_tenths, int level, int price_permille)
{
	if (weight_tenths <= 0 || price_permille <= 0)
		return 0;
	const long l = kingdom_craft_item_level(level);
	return ((long)weight_tenths * (2 + l) * price_permille + 5000) / 10000;
}

/*
 * MATERIAL, in units of the realm's stores, all kinds together
 *
 *     units = round( W x (4 + L/2) x scale )
 *
 *     scale  kingdom.craft.resource.permille / 1000
 *
 * At W = 1: level 10 draws 9 units, level 30 19, level 56 32. Rounds half up,
 * so level 11 (9.5) draws 10.
 */
constexpr long kingdom_craft_material_units(int weight_tenths, int level, int resource_permille)
{
	if (weight_tenths <= 0 || resource_permille <= 0)
		return 0;
	const long l = kingdom_craft_item_level(level);
	return ((long)weight_tenths * (8 + l) * resource_permille + 10000) / 20000;
}

/*
 * RESALE VALUE, in copper (1 platinum = 1000 copper)
 *
 *     value = round( W x (2 + L) x price scale x resale scale ), in copper
 *
 *     scale  kingdom.craft.resale.permille / 1000, so the shipped 100 makes a
 *            piece worth a tenth of what it cost
 *
 * Taken from the price curve BEFORE it is rounded to whole platinum, so the
 * value moves smoothly with weight and level. Scaling the rounded platinum
 * price instead left cliffs: everything under half a platinum fell to the
 * 1-copper floor while anything just over it jumped to 100.
 *
 * Store gear is ordinary property (ruled 2026-09-16): it is looted, given and
 * sold like anything else, and this is what it is worth in a shop's ledger. A
 * shopkeeper pays its own fraction of that again (shop_index[].buy_percent),
 * so what a looter actually nets is smaller still.
 *
 * At W = 1 and the shipped scales: level-10 gear is worth 1,200 copper (1p 2g)
 * against 12p to buy, level-56 gear 5,800 (5p 8g) against 58p.
 *
 * A piece is never worth 0 unless the knob turns resale off altogether: a shop
 * refuses anything worth less than 1 (trade_with(), economy/shop.c), and gear
 * no shop will take is the very thing this ruling undoes.
 */
constexpr long kingdom_craft_resale_copper(int weight_tenths, int level, int price_permille,
					   int resale_permille)
{
	if (weight_tenths <= 0 || resale_permille <= 0)
		return 0;
	const long l = kingdom_craft_item_level(level);
	const long value =
		((long)weight_tenths * (2 + l) * price_permille * resale_permille + 5000) / 10000;
	return value < 1 ? 1 : value;
}

/*
 * THE SPLIT between a station's two resources
 *
 *     primary   = 70% of the units, rounded UP
 *     secondary = whatever is left
 *
 * The forge draws mineral and wood, the loom fibre and water, the jeweller
 * mineral and water, primary first. 32 units split 23 / 9; 9 split 7 / 2.
 */
constexpr long kingdom_craft_primary_share(long units)
{
	if (units <= 0)
		return 0;
	return (units * 7 + 9) / 10;
}

constexpr long kingdom_craft_secondary_share(long units)
{
	return units <= 0 ? 0 : units - kingdom_craft_primary_share(units);
}

/*
 * A STAT LINE AT LEVEL L
 *
 *     stat = max( 1, round( top x L / 56 ) )     for every line that is not zero
 *
 *     top    the line's value on a level-56 piece (the catalogue table)
 *
 * AC counts as a line like any other. A zero line stays zero, so a level-1
 * piece never gains a stat its level-56 twin does not have, and every line it
 * does have is worth at least 1.
 */
constexpr int kingdom_craft_scaled_stat(int top, int level)
{
	if (top == 0)
		return 0;
	const int l = kingdom_craft_item_level(level);
	const int magnitude = top < 0 ? -top : top;
	int scaled = (magnitude * l * 2 + KINGDOM_CRAFT_TOP_LEVEL) / (2 * KINGDOM_CRAFT_TOP_LEVEL);
	if (scaled < 1)
		scaled = 1;
	return top < 0 ? -scaled : scaled;
}

/*
 * WEAPON DICE BY LEVEL BAND
 *
 *     level     one-handed   two-handed
 *      1-10        2d4          2d5
 *     11-20        2d5          3d4
 *     21-30        3d4          3d5
 *     31-40        3d5          3d6
 *     41-50        3d6          4d5
 *     51-56        4d5          5d5
 */
struct kingdom_craft_dice
{
	int count;
	int size;
};

constexpr kingdom_craft_dice kingdom_craft_weapon_dice(int level, bool two_handed)
{
	const int l = kingdom_craft_item_level(level);
	if (l <= 10)
		return two_handed ? kingdom_craft_dice{ 2, 5 } : kingdom_craft_dice{ 2, 4 };
	if (l <= 20)
		return two_handed ? kingdom_craft_dice{ 3, 4 } : kingdom_craft_dice{ 2, 5 };
	if (l <= 30)
		return two_handed ? kingdom_craft_dice{ 3, 5 } : kingdom_craft_dice{ 3, 4 };
	if (l <= 40)
		return two_handed ? kingdom_craft_dice{ 3, 6 } : kingdom_craft_dice{ 3, 5 };
	if (l <= 50)
		return two_handed ? kingdom_craft_dice{ 4, 5 } : kingdom_craft_dice{ 3, 6 };
	return two_handed ? kingdom_craft_dice{ 5, 5 } : kingdom_craft_dice{ 4, 5 };
}

#endif /* _KINGDOM_CRAFT_MATH_H_ */
