/* structures/defines for rand mobs and items */

/* vnums of items that are code converted, randomly */
#define RANDOM_ART 5
#define RANDOM_GEM 6
#define RANDOM_POTION 2000
#define RANDOM_SCROLL 2100
#define RANDOM_RING 2200
#define RANDOM_WAND 2300
#define RANDOM_ROD 2400
#define RANDOM_BOOK 2500
#define RANDOM_JEWELRY 2600
#define RANDOM_CLOAK 2700
#define RANDOM_BOOT 2800
#define RANDOM_BELT 2900
#define RANDOM_BAG 3000
#define RANDOM_ODD 3100
#define RANDOM_TOOL 3200
#define RANDOM_INSTRUMENT 3300
#define RANDOM_MUNDANE_ARMOR 800
#define RANDOM_MAGIC_ARMOR 3400
#define RANDOM_MAGIC_METAL_ARMOR 3409
#define RANDOM_MAGIC_LEATHER_ARMOR 3410
#define RANDOM_MAGIC_WEAPON 3500
#define RANDOM_MUNDANE_WEAPON 300

/* random mob vnums */
#define RANDOM_HUMAN 3000
#define RANDOM_HALFELF 3001
#define RANDOM_ELF 3002
#define RANDOM_DWARF 3003
#define RANDOM_HALFLING 3004
#define RANDOM_GNOME 3005
#define RANDOM_HALFORC 3006
#define RANDOM_H_MAGE 3025
#define RANDOM_THIEF 3026
#define RANDOM_MERCHANT 3050
#define RANDOM_SAILOR 3051
#define RANDOM_BEGGAR 3052

#define RANDOM_MOB(ch) (IS_MOB(ch) && GET_MOB_ORIG_VNUM(ch) >= 3000 && GET_MOB_ORIG_VNUM(ch) < 3100)

#define LOC_BODY 0
#define LOC_ARM 1
#define LOC_LEG 2
#define LOC_FOOT 3
#define LOC_SHIELD 4
#define LOC_HEAD 5
#define LOC_HANDS 6
#define LOC_WEAPON1 7
#define LOC_WEAPON2 8

#define TYPE_LEATHER 0
#define TYPE_STUDDED 10
#define TYPE_SCALE 20
#define TYPE_CHAIN 30
#define TYPE_ELVEN 40
#define TYPE_BRONZE 50
#define TYPE_PLATE 60
#define TYPE_FIELD 70

#define MAX_SCROLL 81
#define MAX_POTION_COLORS 13

#define MAX_ADJ 180
#define MAX_MATL 120
#define MAX_WEAPON 50
#define MAX_WEAPON_MOD 36
#define MAX_METAL_ARMOR 8
#define MAX_METAL_ARMOR_TYPE 20
#define MAX_METAL_ARMOR_ADJ 28
#define MAX_LEATHER_ARMOR 11
#define MAX_LEATHER_ARMOR_TYPE 13
#define MAX_LEATHER_ARMOR_ADJ 20

/*
 * structure for material properties
 */
struct matl_data
{
	char *name;
	int value;
	int hardness;
	int flags;
	int color;
};

/*
 * structure for adjective data
 */
struct adj_data
{
	char *adjective;
	int modifier;
	int flags;
};

/*
 * structure for weapon modifiers
 */
struct weapon_mod_data
{
	char *adjective;
	int hitroll;
	int damroll;
	int weight;
};

/*
 * structure for weapons
 */
struct weapon_data
{
	char *a_an;
	char *name;
	int no_dice;
	int size_dice;
	int lno_dice;
	int lsize_dice;
	int dam_type;
	int obj_type;
	int speed;
	int weapon_type;
	int weight;
	int cost;
	int spare;
	int flags;
};

/*
 * struct for armor types
 */
struct metal_armor_type_data
{
	char *pattern;
	int modifier;
	int weight;
	int wear;
};

/*
 * struct for armor adjectives
 */
struct metal_armor_adj_data
{
	char *name;
	int modifier;
	int weight;
	int extra;
};

/*
 * struct for metal armors
 */
struct metal_armor_data
{
	char *name;
	int armor;
};

/*
 * leather armor structs
 */
struct leather_armor_adj_data
{
	char *name;
	int modifier;
};

struct leather_armor_type_data
{
	char *pattern;
	int weight;
	int wear;
};

struct leather_armor_data
{
	char *name;
	int armor;
	int extra;
};

/*   colors */
#define COLOR_BLACK 1
#define COLOR_WHITE 2
#define COLOR_RED 4
#define COLOR_ORANGE 8
#define COLOR_YELLOW 16
#define COLOR_GREEN 32
#define COLOR_BLUE 64
#define COLOR_PURPLE 128
#define COLOR_SILVER 256
#define COLOR_GOLD 512
#define COLOR_BROWN 1024
#define COLOR_LIGHT 2048
#define COLOR_DARK 4096
#define COLOR_TRANSPARENT 8192
#define COLOR_TRANSLUCENT 16384

/*
 *  adjective bits
 */
#define ADJ_JEWELRY 1
#define ADJ_POSITIVE 2
#define ADJ_NEGATIVE 4
#define ADJ_COLOR 8
#define ADJ_QUALITY 16
#define ADJ_SHAPE 32
#define ADJ_SCROLL 64
#define ADJ_STAFF 128

/*
 * Tables for renaming items.
 */
#define TABLE_SCROLL 0
#define TABLE_JEWELRY 1
#define TABLE_STAFF 2
#define TABLE_WAND 3
#define TABLE_POTION 4
#define TABLE_MAGIC_METAL_ARMOR 5
#define TABLE_MAGIC_LEATHER_ARMOR 6
#define TABLE_METAL_ARMOR 7
#define TABLE_LEATHER_ARMOR 8

#define ITEM_ALLOW_ALL_BUT_MAGE_THIEF                                                     \
	(ITEM_ALLOW_CLERIC + ITEM_ALLOW_DRUID + ITEM_ALLOW_FIGHTER + ITEM_ALLOW_PALADIN + \
	 ITEM_ALLOW_RANGER + ITEM_ALLOW_BARD)
#define ITEM_ALLOW_ALL_BUT_THIEF                                                          \
	(ITEM_ALLOW_CLERIC + ITEM_ALLOW_DRUID + ITEM_ALLOW_FIGHTER + ITEM_ALLOW_PALADIN + \
	 ITEM_ALLOW_RANGER + ITEM_ALLOW_MAGIC_USER + ITEM_ALLOW_ILLUSIONIST + ITEM_ALLOW_BARD)
#define ITEM_ALLOW_ALL_BUT_MAGE                                                           \
	(ITEM_ALLOW_CLERIC + ITEM_ALLOW_DRUID + ITEM_ALLOW_FIGHTER + ITEM_ALLOW_PALADIN + \
	 ITEM_ALLOW_RANGER + ITEM_ALLOW_THIEF + ITEM_ALLOW_BARD)
