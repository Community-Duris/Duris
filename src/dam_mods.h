typedef struct
{
	double baseDamage;
	double addedMod; // +/- base
	double increasedMod; // first multi
	double moreMod; // second multi
} damage_profile;

typedef enum
{
	None,
	Added,
	Increased,
	More
} dam_mod_type;

typedef struct
{
	dam_mod_type type;
	double	     mod;
} damage_mod;

typedef void (*dam_mod_predicate)(P_char, P_char, double, int, uint, damage_mod *,
				  struct damage_messages *);

#define MAKE_DAM_MOD_PRED()                                                                \
	(dam_mod_predicate)[](P_char caster, P_char victim, double damage, int damageType, \
			      uint flags, damage_mod *dam_mod, struct damage_messages *messages)
#define NUM_SPELL_PREDICATES 35
#define NUM_RAW_PREDICATES 15
