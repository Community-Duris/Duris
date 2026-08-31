#ifndef _TRADESKILL_H_

#include "economy/mining.h"

#define GUILD_COST 5000000

#define REG_FAERIE_BAG_VNUM 400217
#define RARE_FAERIE_BAG_VNUM 400235
#define EXCEPT_FAERIE_BAG_VNUM 400233

#define TURKEY_INNARDS_VNUM 400232
#define TURKEY_WING_CAPE_VNUM 400237
#define TURKEY_FOOD_VNUM 400236

#define HAMMER_VNUM 252
#define POLE_VNUM 336
#define PARCHMENT_VNUM 251
#define MAX_NEEDED_ORE 5

#define LOWEST_MAT_VNUM 400000
#define HIGHEST_MAT_VNUM 400209
#define SALVAGE_RECIPE_VNUM 400210
#define MAG_ESSENCE_VNUM 400211

void salvage_examine_item(P_char ch, P_obj item);

// The maximum level mob that can be conjured without use of a greater orb of magic.
#define CONJURE_MAXLVL_NO_ORB 56

#define ALLOW 1
#define ANTI 0

struct forge_item
{
	int id;
	const char *keywords;
	const char *long_desc;
	const char *short_desc;
	int ore_needed[5];

	int loc0;
	int min0;
	int max0;

	int loc1;
	int min1;
	int max1;

	int skill_min;
	int how_rare;

	int allow_anti;
	unsigned int classes;
	unsigned int wear_flags;
	unsigned int aff1;
	unsigned int aff2;
	unsigned int aff3;
	unsigned int aff4;
};

struct fishing_data
{
	int room;
	int counter;
	int fish_quality;
};

/*

234 - crude iron bar
235 - refined iron bar
236 - crude steel bar
237 - refined steel bar
238 - crude copper bar
240 - refined copper bar
241 - crude silver bar
242 - refined silver bar
243 - crude gold bar
244 - refined gold bar
247 - crude platinum bar
248 - refined platinum bar
249 - crude mithril bar
250 - refined mithril bar
*/

void initialize_tradeskills();
void event_fish_check(P_char ch, P_char victim, P_obj, void *data);
int assoc_founder(P_char mob, P_char pl, int cmd, char *arg);

bool player_recipes_exists(char *charname);
void create_recipes_file(const char *dir, char *name);
void create_recipes_name(char *name);
void create_recipe(P_char ch, P_obj temp);

#endif // _TRADESKILL_H_
