#define WRONG_INGREDIENT -1
#define NIGHTSHADE       1
#define MANDRAKE_ROOT    2
#define GARLIC           3
#define FAERIE_DUST      4
#define DRAGONS_BLOOD    5
#define GREEN_HERB       6
#define LIVING_STONE     7
#define BONE             8

#define TONGUE 9
#define FACE   10
#define ARMS   11
#define LEGS   12
#define SCALP  13
#define EARS   14
#define SKULL  15
#define BOWELS 16
#define EYES   17

#define LAST_BASIC_INGREDIENT BONE
#define FIRST_POTION_VIRTUAL  850

#define MAX_INGREDIENTS 9
void set_long_description(P_obj, const char);
void set_keywords(P_obj, const char);
void set_short_descrption(P_obj, const char);

struct potion
{
	int spell_type;
	int spell_level;
	int ingredients[MAX_INGREDIENTS + 1];
	int vnum;
};
