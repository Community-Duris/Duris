// Executes the guild store's arithmetic (src/kingdom/kingdom_craft_math.h)
// against the worked examples in the approved design, so a change to a
// formula that moves a published price fails here rather than in play. It
// also executes the store's binding token (src/kingdom/kingdom_craft_bind.h),
// the one thing that decides who may wear a store piece.

#include "kingdom/kingdom_craft_bind.h"
#include "kingdom/kingdom_craft_math.h"

#include <cstdio>
#include <cstring>

static int failures = 0;

static void expect(long got, long want, const char *what)
{
	if (got != want)
	{
		std::printf("FAIL: %s: got %ld, want %ld\n", what, got, want);
		++failures;
	}
	else
		std::printf("OK: %s = %ld\n", what, got);
}

int main()
{
	// The design's worked examples, at weight 1.0 and the shipped scales.
	expect(kingdom_craft_price_platinum(10, 10, 1000), 12, "price W1 L10");
	expect(kingdom_craft_material_units(10, 10, 1000), 9, "units W1 L10");
	expect(kingdom_craft_price_platinum(10, 30, 1000), 32, "price W1 L30");
	expect(kingdom_craft_material_units(10, 30, 1000), 19, "units W1 L30");
	expect(kingdom_craft_price_platinum(10, 56, 1000), 58, "price W1 L56");
	expect(kingdom_craft_material_units(10, 56, 1000), 32, "units W1 L56");

	// Other weights: one-handed/body 1.5, two-handed 2.0, shield 1.2, jewellery 0.8.
	expect(kingdom_craft_price_platinum(15, 56, 1000), 87, "price W1.5 L56");
	expect(kingdom_craft_material_units(15, 56, 1000), 48, "units W1.5 L56");
	expect(kingdom_craft_price_platinum(20, 56, 1000), 116, "price W2.0 L56");
	expect(kingdom_craft_material_units(20, 56, 1000), 64, "units W2.0 L56");
	expect(kingdom_craft_price_platinum(12, 56, 1000), 70, "price W1.2 L56 (69.6)");
	expect(kingdom_craft_price_platinum(8, 56, 1000), 46, "price W0.8 L56 (46.4)");
	expect(kingdom_craft_material_units(8, 56, 1000), 26, "units W0.8 L56 (25.6)");

	// Half rounds up: level 11 is 9.5 units at W1.
	expect(kingdom_craft_material_units(10, 11, 1000), 10, "units W1 L11 (9.5)");

	// The scales are per mille: 2000 doubles, 0 is free.
	expect(kingdom_craft_price_platinum(10, 56, 2000), 116, "price W1 L56 x2");
	expect(kingdom_craft_material_units(10, 56, 500), 16, "units W1 L56 x0.5");
	expect(kingdom_craft_price_platinum(10, 56, 0), 0, "price at scale 0");
	expect(kingdom_craft_material_units(10, 56, 0), 0, "units at scale 0");

	// The 70/30 split, primary rounded up.
	expect(kingdom_craft_primary_share(32), 23, "primary of 32");
	expect(kingdom_craft_secondary_share(32), 9, "secondary of 32");
	expect(kingdom_craft_primary_share(9), 7, "primary of 9");
	expect(kingdom_craft_secondary_share(9), 2, "secondary of 9");
	expect(kingdom_craft_primary_share(1), 1, "primary of 1");
	expect(kingdom_craft_secondary_share(1), 0, "secondary of 1");

	// The level rule: never above the buyer, never above 56, never below 1.
	expect(kingdom_craft_item_level(10), 10, "item level for a level 10");
	expect(kingdom_craft_item_level(56), 56, "item level for a level 56");
	expect(kingdom_craft_item_level(62), 56, "item level for a level 62");
	expect(kingdom_craft_item_level(0), 1, "item level for a level 0");
	// A level 62 buyer pays the level-56 price, not more.
	expect(kingdom_craft_price_platinum(10, 62, 1000), 58, "price W1 L62 is L56's");

	// Stat lines: max(1, round(top x L / 56)); zero stays zero.
	expect(kingdom_craft_scaled_stat(25, 56), 25, "AC 25 at L56");
	expect(kingdom_craft_scaled_stat(25, 10), 4, "AC 25 at L10 (4.46)");
	expect(kingdom_craft_scaled_stat(25, 30), 13, "AC 25 at L30 (13.39)");
	expect(kingdom_craft_scaled_stat(2, 1), 1, "+2 at L1 floors to 1");
	expect(kingdom_craft_scaled_stat(3, 28), 2, "+3 at L28 (1.5 rounds up)");
	expect(kingdom_craft_scaled_stat(0, 56), 0, "a zero line stays zero");
	expect(kingdom_craft_scaled_stat(15, 62), 15, "HP 15 at L62 is L56's");

	// Weapon dice bands, one- and two-handed.
	const struct
	{
		int level;
		bool two;
		int count;
		int size;
	} dice[] = {
		{ 1, false, 2, 4 },  { 10, false, 2, 4 }, { 10, true, 2, 5 },  { 11, false, 2, 5 },
		{ 11, true, 3, 4 },  { 20, true, 3, 4 },  { 21, false, 3, 4 }, { 30, true, 3, 5 },
		{ 31, false, 3, 5 }, { 40, true, 3, 6 },  { 41, false, 3, 6 }, { 50, true, 4, 5 },
		{ 51, false, 4, 5 }, { 56, true, 5, 5 },  { 62, false, 4, 5 },
	};
	for (const auto &row : dice)
	{
		const kingdom_craft_dice got = kingdom_craft_weapon_dice(row.level, row.two);
		char what[64];
		std::snprintf(what, sizeof(what), "dice L%d %s count", row.level,
			      row.two ? "2H" : "1H");
		expect(got.count, row.count, what);
		std::snprintf(what, sizeof(what), "dice L%d %s size", row.level,
			      row.two ? "2H" : "1H");
		expect(got.size, row.size, what);
	}

	// The binding token: the buyer's player id, as a whole word no character
	// name can equal. A near miss must never bind.
	char token[KINGDOM_CRAFT_BIND_TOKEN_LEN];
	expect(kingdom_craft_bind_token(1042, token, sizeof(token)), 1,
	       "token for pid 1042 is written");
	expect(std::strcmp(token, "kingdom-bound-1042") == 0, 1,
	       "the token reads kingdom-bound-1042");
	expect(kingdom_craft_bind_token(0, token, sizeof(token)), 0, "no token for pid 0");
	expect(token[0] == '\0', 1, "a refused token leaves the buffer empty");
	expect(kingdom_craft_bind_token(-3, token, sizeof(token)), 0,
	       "no token for a negative pid");
	char tiny[8];
	expect(kingdom_craft_bind_token(1042, tiny, sizeof(tiny)), 0, "no token cut short");
	expect(tiny[0] == '\0', 1, "a token cut short leaves nothing behind");

	const char *keys = "steel vambraces vambraces kingdom Tyrus kingdom-bound-1042";
	expect(kingdom_craft_binding_is(keys, 1042), 1, "the buyer's own id binds");
	expect(kingdom_craft_binding_is(keys, 104), 0, "a prefix of the id does not bind");
	expect(kingdom_craft_binding_is(keys, 10420), 0, "a longer id does not bind");
	expect(kingdom_craft_binding_is("kingdom-bound-10420", 1042), 0,
	       "the token inside a longer one does not bind");
	expect(kingdom_craft_binding_is("xkingdom-bound-1042", 1042), 0,
	       "the token glued to a word does not bind");
	expect(kingdom_craft_binding_is("kingdom-bound-1042 steel", 1042), 1,
	       "the token first in the list binds");
	expect(kingdom_craft_binding_is("steel vambraces kingdom tyrus", 1042), 0,
	       "a binding without a token binds nobody");
	expect(kingdom_craft_binding_is(nullptr, 1042), 0, "no binding binds nobody");
	expect(kingdom_craft_binding_is(keys, 0), 0, "pid 0 binds nothing");

	// Carrying a token at all -- whoever it names -- is what keeps a piece off
	// the legacy name test even when its object index is unresolved.
	expect(kingdom_craft_binding_present(keys), 1, "a piece's binding carries a token");
	expect(kingdom_craft_binding_present("kingdom-bound-1042"), 1, "a token alone is carried");
	expect(kingdom_craft_binding_present("steel kingdom-bound-7 kingdom"), 1,
	       "a token mid-list is carried");
	expect(kingdom_craft_binding_present("steel vambraces kingdom tyrus"), 0,
	       "plain words, 'kingdom' among them, carry no token");
	expect(kingdom_craft_binding_present("xkingdom-bound-1042"), 0,
	       "the prefix glued to a word is not a token");
	expect(kingdom_craft_binding_present(nullptr), 0, "no keywords carry no token");

	// Any whitespace ends the word, not a space alone: an action description
	// that has been through a tool which normalises line endings still binds.
	expect(kingdom_craft_binding_is("kingdom-bound-1042\r\n", 1042), 1,
	       "a token followed by CRLF binds");
	expect(kingdom_craft_binding_is("\nkingdom-bound-1042\n", 1042), 1,
	       "a token on a line of its own binds");
	expect(kingdom_craft_binding_is("\tkingdom-bound-1042\t", 1042), 1,
	       "a token between tabs binds");
	expect(kingdom_craft_binding_is("kingdom-bound-1042x", 1042), 0,
	       "a token glued to a letter still does not bind");
	expect(kingdom_craft_binding_present("kingdom-bound-1042\r\n"), 1,
	       "a token followed by CRLF is carried");
	expect(kingdom_craft_binding_present("\nkingdom-bound-7"), 1,
	       "a token after a newline is carried");

	if (failures)
	{
		std::printf("%d kingdom craft arithmetic check(s) failed\n", failures);
		return 1;
	}
	std::printf("kingdom craft arithmetic: OK\n");
	return 0;
}
