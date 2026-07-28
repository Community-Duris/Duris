#ifndef _MINING_H_
#define _MINING_H_

#define PICK_VNUM 253

#define LOWEST_ORE_VNUM  400260
#define NUMBER_ORE_TYPES 8

// The below reg ore vnums are outdated; the new vnums are in order starting with above.
#define SMALL_IRON_ORE  194
#define MEDIUM_IRON_ORE 196
#define LARGE_IRON_ORE  197

#define SMALL_STEEL_ORE  198
#define MEDIUM_STEEL_ORE 199
#define LARGE_STEEL_ORE  200

#define SMALL_COPPER_ORE  201
#define MEDIUM_COPPER_ORE 202
#define LARGE_COPPER_ORE  219

#define SMALL_SILVER_ORE  220
#define MEDIUM_SILVER_ORE 221
#define LARGE_SILVER_ORE  222

#define SMALL_GOLD_ORE  223
#define MEDIUM_GOLD_ORE 224
#define LARGE_GOLD_ORE  225

#define SMALL_PLATINUM_ORE  226
#define MEDIUM_PLATINUM_ORE 229
#define LARGE_PLATINUM_ORE  230

#define SMALL_MITHRIL_ORE  231
#define MEDIUM_MITHRIL_ORE 232
#define LARGE_MITHRIL_ORE  233

#define SMALL_ADAMANTIUM_ORE  501
#define MEDIUM_ADAMANTIUM_ORE 502
#define LARGE_ADAMANTIUM_ORE  503

// Gemstones!
#define TINY_IMP_TOPAZ 504
#define REG_IMP_TOPAZ  505
#define LG_IMP_TOPAZ   506

#define TINY_REG_TOPAZ 507
#define REG_REG_TOPAZ  508
#define LG_REG_TOPAZ   509

#define FLAWLESS_TOPAZ    510
#define LG_FLAWLESS_TOPAZ 511

#define TINY_IMP_SAPPHIRE 512
#define REG_IMP_SAPPHIRE  513
#define LG_IMP_SAPPHIRE   514

#define TINY_REG_SAPPHIRE 515
#define REG_REG_SAPPHIRE  516
#define LG_REG_SAPPHIRE   517

#define FLAWLESS_SAPPHIRE    518
#define LG_FLAWLESS_SAPPHIRE 519

#define TINY_IMP_EMERALD 520
#define REG_IMP_EMERALD  521
#define LG_IMP_EMERALD   522

#define TINY_REG_EMERALD 523
#define REG_REG_EMERALD  524
#define LG_REG_EMERALD   525

#define FLAWLESS_EMERALD    526
#define LG_FLAWLESS_EMERALD 527

#define TINY_IMP_DIAMOND 528
#define REG_IMP_DIAMOND  529
#define LG_IMP_DIAMOND   530

#define TINY_REG_DIAMOND 531
#define REG_REG_DIAMOND  532
#define LG_REG_DIAMOND   533

#define FLAWLESS_DIAMOND    534
#define LG_FLAWLESS_DIAMOND 535

#define TINY_IMP_RUBY 536
#define REG_IMP_RUBY  537
#define LG_IMP_RUBY   538

#define TINY_REG_RUBY 539
#define REG_REG_RUBY  540
#define LG_REG_RUBY   541

#define FLAWLESS_RUBY    542
#define LG_FLAWLESS_RUBY 543

#define MINES_MAP_SURFACE   0
#define MINES_MAP_UD        1
#define MINES_MAP_THARNRIFT 2
#define MINES_GEM_SURFACE   3
#define MINES_GEM_UD        4

struct mining_data
{
	int room;
	int counter;
	int mine_quality;
	int mine_type;
};

void initialize_mining();
int mines_properties(int map);
bool load_one_mine(int map);
void load_mines(bool set_event, bool load_all, int map);
void do_mine(P_char ch, char *arg, int cmd);
int mine(P_obj obj, P_char ch, int cmd, char *arg);
void event_mine_check(P_char ch, P_char victim, P_obj obj, void *data);
void event_load_mines(P_char ch, P_char victim, P_obj obj, void *data);

#endif
