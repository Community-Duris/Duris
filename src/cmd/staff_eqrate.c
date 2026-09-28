/* Staff equipment rating command and scoring helpers. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/mm.h"
#include "core/safe_format.h"
#include "world/db.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern P_index obj_index;
extern int top_of_objt;
extern const char *apply_types[];
extern const flagDef extra_bits[];
extern const flagDef extra2_bits[];
extern const char *item_types[];
extern const flagDef wear_bits[];
void sprintbitde(ulong, const flagDef[], char *);

/*
 * ***************************************************************************
 * * eqrate - rate equipment prototypes by their mechanical value.
 * *
 * * Ported from the Realms of Luminari do_eqrate/rate_object pair.  The
 * * scoring tiers are kept (stat = x10, max-stat = x25, race-stat = x15,
 * * hitroll = x15, damroll = x20, sanctuary = 50, etc.) but the inputs had
 * * to be remapped onto Duris' object model, which diverged years ago:
 * *
 * *   - Shields are their own item type here (ITEM_SHIELD) and carry their
 * *     AC in value[3]; armor keeps its AC in value[0].
 * *   - A weapon's value[0] is the weapon *type* (WEAPON_xxx), not a proc
 * *     value, so there is no "weapon proc value" term.
 * *   - Fire weapons contribute value[2] to archery damage (scaled by
 * *     archery.weaponFactor) and cap firing speed with value[0].
 * *   - MAGIC and BLESS live in extra2_flags here, and there is no NOBURN.
 * *   - Continuous affects are spread over bitvector..bitvector5 instead of
 * *     a single sets_affs, and several RoL affects (TRUE_SIGHT,
 * *     DRAGONSCALES, DISPLACEMENT, MIRROR_IMAGE) have no Duris analogue.
 * *   - Special procs are obj_index[].func.obj or the ITEM_PROCLIB flag;
 * *     there is no spec_flag.
 * *   - Duris-only applies (regen, spell/combat pulse, skill grants) are
 * *     scored here since they are the applies that actually matter on
 * *     modern eq.  Lower pulse is faster, so negative modifiers score.
 * ***************************************************************************
 */

#define EQRATE_MAX_LIST 500 /* hard cap on how many rows a listing may show */

struct eqrate_aff_def
{
	int bank; /* 1..5, which obj->bitvectorN the bit lives in */
	ulong bit;
	const char *name;
	int points;
};

/*
 * Continuous affects an item can hand its wearer.  Detrimental affects are
 * scored negative so cursed prototypes sort to the bottom instead of looking
 * like plain zeroes.
 */
static const struct eqrate_aff_def eqrate_aff_table[] = {
	{ 4, AFF4_SANCTUARY, "SANCTUARY", 50 },
	{ 1, AFF_HASTE, "HASTE", 40 },
	{ 2, AFF2_GLOBE, "MAJOR_GLOBE", 35 },
	{ 2, AFF2_FIRESHIELD, "FIRESHIELD", 30 },
	{ 3, AFF3_COLDSHIELD, "COLDSHIELD", 30 },
	{ 3, AFF3_LIGHTNINGSHIELD, "LIGHTNING_SHIELD", 30 },
	{ 1, AFF_SANCTUM_DRACONIS, "SANCTUM_DRACONIS", 30 },
	{ 3, AFF3_BLUR, "BLUR", 25 },
	{ 2, AFF2_CONCEALMENT, "CONCEALMENT", 25 },
	{ 3, AFF3_INERTIAL_BARRIER, "INERTIAL_BARRIER", 25 },
	{ 4, AFF4_NEG_SHIELD, "NEG_SHIELD", 25 },
	{ 1, AFF_MINOR_GLOBE, "MINOR_GLOBE", 20 },
	{ 1, AFF_FLY, "FLY", 20 },
	{ 1, AFF_LEVITATE, "LEVITATE", 20 },
	{ 1, AFF_BARKSKIN, "BARKSKIN", 20 },
	{ 1, AFF_STONE_SKIN, "STONE_SKIN", 20 },
	{ 1, AFF_BIOFEEDBACK, "BIOFEEDBACK", 20 },
	{ 4, AFF4_REGENERATION, "REGENERATION", 20 },
	{ 4, AFF4_DEFLECT, "DEFLECT", 20 },
	{ 2, AFF2_SOULSHIELD, "SOULSHIELD", 20 },
	{ 2, AFF2_VAMPIRIC_TOUCH, "VAMPIRIC_TOUCH", 20 },
	{ 2, AFF2_FLURRY, "FLURRY", 20 },
	{ 1, AFF_DETECT_INVISIBLE, "DETECT_INVIS", 15 },
	{ 1, AFF_SENSE_LIFE, "SENSE_LIFE", 15 },
	{ 1, AFF_WATERBREATH, "WATERBREATH", 15 },
	{ 1, AFF_PROT_FIRE, "PROT_FIRE", 15 },
	{ 2, AFF2_PROT_COLD, "PROT_COLD", 15 },
	{ 2, AFF2_PROT_LIGHTNING, "PROT_LIGHTNING", 15 },
	{ 2, AFF2_PROT_ACID, "PROT_ACID", 15 },
	{ 2, AFF2_PROT_GAS, "PROT_GAS", 15 },
	{ 1, AFF_FREEDOM_OF_MVMNT, "FREEDOM_OF_MVMT", 15 },
	{ 3, AFF3_TOWER_IRON_WILL, "TOWER_IRON_WILL", 15 },
	{ 5, AFF5_PROT_UNDEAD, "PROT_UNDEAD", 15 },
	{ 3, AFF3_SPIRIT_WARD, "SPIRIT_WARD", 15 },
	{ 3, AFF3_GR_SPIRIT_WARD, "GR_SPIRIT_WARD", 15 },
	{ 1, AFF_INFRAVISION, "INFRAVISION", 10 },
	{ 2, AFF2_ULTRAVISION, "ULTRAVISION", 10 },
	{ 1, AFF_UD_VISION, "UD_VISION", 10 },
	{ 1, AFF_FARSEE, "FARSEE", 10 },
	{ 1, AFF_PROTECT_EVIL, "PROTECT_EVIL", 10 },
	{ 1, AFF_PROTECT_GOOD, "PROTECT_GOOD", 10 },
	{ 4, AFF4_NOFEAR, "NOFEAR", 10 },
	{ 1, AFF_SNEAK, "SNEAK", 10 },
	{ 1, AFF_HIDE, "HIDE", 10 },
	{ 3, AFF3_PASS_WITHOUT_TRACE, "PASS_TRACE", 10 },
	{ 3, AFF3_NON_DETECTION, "NON_DETECTION", 10 },
	{ 1, AFF_ARMOR, "ARMOR", 10 },
	{ 1, AFF_AWARE, "AWARE", 10 },
	{ 4, AFF4_HAWKVISION, "HAWKVISION", 10 },
	{ 4, AFF4_PROT_LIVING, "PROT_LIVING", 10 },
	{ 4, AFF4_DETECT_ILLUSION, "DETECT_ILLUSION", 10 },
	{ 1, AFF_INVISIBLE, "INVISIBLE", 10 },
	{ 2, AFF2_DETECT_EVIL, "DETECT_EVIL", 5 },
	{ 2, AFF2_DETECT_GOOD, "DETECT_GOOD", 5 },
	{ 2, AFF2_DETECT_MAGIC, "DETECT_MAGIC", 5 },
	{ 1, AFF_SLOW_POISON, "SLOW_POISON", 5 },
	/* Detrimental - an item that hands these out is a liability. */
	{ 1, AFF_BLIND, "BLIND", -50 },
	{ 1, AFF_SLEEP, "SLEEP", -50 },
	{ 1, AFF_CHARM, "CHARM", -40 },
	{ 1, AFF_FEAR, "FEAR", -40 },
	{ 2, AFF2_SLOW, "SLOWNESS", -40 },
	{ 2, AFF2_POISONED, "POISONED", -30 },
	{ 2, AFF2_SILENCED, "SILENCED", -30 },
	{ 3, AFF3_FAMINE, "FAMINE", -20 },
	{ 5, AFF5_MENTAL_ANGUISH, "MENTAL_ANGUISH", -20 },
	{ 5, AFF5_MEMORY_BLOCK, "MEMORY_BLOCK", -20 },
	{ 0, 0, NULL, 0 }
};

/* Returns the obj bitvector bank named by an eqrate_aff_def. */
static ulong eqrate_bank_bits(P_obj obj, int bank)
{
	switch (bank)
	{
	case 1:
		return obj->bitvector;
	case 2:
		return obj->bitvector2;
	case 3:
		return obj->bitvector3;
	case 4:
		return obj->bitvector4;
	case 5:
		return obj->bitvector5;
	default:
		return 0;
	}
}

/*
 * Score for a single obj->affected[] entry.  Kept in one place so the summary
 * and the breakdown can never drift apart.
 */
static int eqrate_apply_score(int loc, int mod)
{
	switch (loc)
	{
	case APPLY_STR:
	case APPLY_DEX:
	case APPLY_INT:
	case APPLY_WIS:
	case APPLY_CON:
	case APPLY_AGI:
	case APPLY_POW:
	case APPLY_CHA:
	case APPLY_KARMA:
	case APPLY_LUCK:
		return mod * 10;

	case APPLY_STR_MAX:
	case APPLY_DEX_MAX:
	case APPLY_INT_MAX:
	case APPLY_WIS_MAX:
	case APPLY_CON_MAX:
	case APPLY_AGI_MAX:
	case APPLY_POW_MAX:
	case APPLY_CHA_MAX:
	case APPLY_KARMA_MAX:
	case APPLY_LUCK_MAX:
		return mod * 25;

	case APPLY_STR_RACE:
	case APPLY_DEX_RACE:
	case APPLY_INT_RACE:
	case APPLY_WIS_RACE:
	case APPLY_CON_RACE:
	case APPLY_AGI_RACE:
	case APPLY_POW_RACE:
	case APPLY_CHA_RACE:
	case APPLY_KARMA_RACE:
	case APPLY_LUCK_RACE:
		return mod * 15;

	case APPLY_HIT:
		return mod / 5;

	case APPLY_MANA:
		return mod / 10;

	case APPLY_MOVE:
		return mod / 20;

	case APPLY_AC: /* also APPLY_ARMOR - lower AC is better */
		return -mod * 5;

	case APPLY_HITROLL:
		return mod * 15;

	case APPLY_DAMROLL:
		return mod * 20;

	case APPLY_FIRE_PROT:
		return 20;

	case APPLY_SAVING_PARA:
	case APPLY_SAVING_ROD:
	case APPLY_SAVING_FEAR:
	case APPLY_SAVING_BREATH:
	case APPLY_SAVING_SPELL:
		return -mod * 8; /* lower saves are better */

	/* Duris-only applies below. */
	case APPLY_HIT_REG:
		return mod * 5;

	case APPLY_MANA_REG:
		return mod * 3;

	case APPLY_MOVE_REG:
		return mod * 2;

	case APPLY_COMBAT_PULSE: /* negative pulse swings faster */
		return -mod * 40;

	case APPLY_SPELL_PULSE:
		return -mod * 30;

	case APPLY_SKILL_GRANT: /* modifier is a skill number, not a size */
		return 25;

	case APPLY_SKILL_ADD:
		return mod * 5;

	case APPLY_CURSE:
		return -mod * 10;

	default:
		return 0;
	}
}

/*
 * Object rating based on stats, affects, armor/damage values, flags and procs.
 */
int rate_object(P_obj obj)
{
	int rating = 0;
	int i;

	if (!obj)
		return 0;

	/* 1. Base item type contributions. */
	if (obj->type == ITEM_ARMOR)
	{
		rating += obj->value[0] * 5;
	}
	else if (obj->type == ITEM_SHIELD)
	{
		rating += obj->value[3] * 5;
	}
	else if (obj->type == ITEM_WEAPON)
	{
		if (obj->value[1] > 0 && obj->value[2] > 0)
			rating += (int)((float)obj->value[1] * (float)(obj->value[2] + 1) / 2.0f *
					8.0f);
	}
	else if (obj->type == ITEM_FIREWEAPON)
	{
		rating += obj->value[2] * 4; /* damage bonus added to every shot */
		rating += obj->value[0] * 2; /* firing speed cap */
	}
	else if (obj->type == ITEM_MISSILE)
	{
		if (obj->value[1] > 0 && obj->value[2] > 0)
			rating += (int)((float)obj->value[1] * (float)(obj->value[2] + 1) / 2.0f *
					8.0f);
	}

	/* 2. Affects attached to the object. */
	for (i = 0; i < MAX_OBJ_AFFECT; i++)
		if (obj->affected[i].location)
			rating += eqrate_apply_score(obj->affected[i].location,
						     obj->affected[i].modifier);

	/* 3. Extra flags. */
	if (IS_SET(obj->extra_flags, ITEM_ARTIFACT))
		rating += 50;
	if (IS_SET(obj->extra_flags, ITEM_NOSLEEP))
		rating += 15;
	if (IS_SET(obj->extra_flags, ITEM_NOCHARM))
		rating += 15;
	if (IS_SET(obj->extra_flags, ITEM_RETURNING))
		rating += 10;
	if (IS_SET(obj->extra_flags, ITEM_FLOAT))
		rating += 5;
	if (IS_SET(obj->extra_flags, ITEM_LEVITATES))
		rating += 5;
	if (IS_SET(obj->extra_flags, ITEM_GLOW))
		rating += 2;
	if (IS_SET(obj->extra_flags, ITEM_NODROP))
		rating -= 20;

	/* 3b. Extra2 flags - MAGIC and BLESS live here in Duris. */
	if (IS_SET(obj->extra2_flags, ITEM2_MAGIC))
		rating += 10;
	if (IS_SET(obj->extra2_flags, ITEM2_BLESS))
		rating += 5;
	if (IS_SET(obj->extra2_flags, ITEM2_SILVER))
		rating += 10;
	if (IS_SET(obj->extra2_flags, ITEM2_ENHANCED))
		rating += 10;
	if (IS_SET(obj->extra2_flags, ITEM2_SLAY_GOOD))
		rating += 15;
	if (IS_SET(obj->extra2_flags, ITEM2_SLAY_EVIL))
		rating += 15;
	if (IS_SET(obj->extra2_flags, ITEM2_SLAY_UNDEAD))
		rating += 15;
	if (IS_SET(obj->extra2_flags, ITEM2_SLAY_LIVING))
		rating += 15;

	/* 4. Continuous affects. */
	for (i = 0; eqrate_aff_table[i].name; i++)
		if (IS_SET(eqrate_bank_bits(obj, eqrate_aff_table[i].bank),
			   eqrate_aff_table[i].bit))
			rating += eqrate_aff_table[i].points;

	/* 5. Special procedures and traps. */
	if ((obj->R_num >= 0 && obj_index[obj->R_num].func.obj != NULL) ||
	    IS_SET(obj->extra_flags, ITEM_PROCLIB))
		rating += 25;

	if (obj->trap_eff > 0 || obj->trap_dam > 0)
		rating += 15 + (obj->trap_dam / 5);

	return rating;
}

/* Bounded append helper - the breakdown can run long on heavily affected eq. */
static void eqrate_cat(char *buf, size_t size, size_t *len, const char *fmt, ...)
{
	va_list args;
	int wrote;

	if (*len + 1 >= size)
		return;

	va_start(args, fmt);
	wrote = vsnprintf(buf + *len, size - *len, fmt, args);
	va_end(args);

	if (wrote > 0)
		*len += MIN((size_t)wrote, size - *len - 1);
}

/*
 * One breakdown row: factor, detail, score.  Colour codes sit outside the
 * padded fields so every row lands its score in the same column.
 */
static void eqrate_row(char *buf, size_t size, size_t *len, const char *factor, const char *detail,
		       int score)
{
	eqrate_cat(buf, size, len, " &+c%-33.33s&N  &+w%-33.33s&N  &+G%+6d&N\n", factor, detail,
		   score);
}

/* Detailed rating breakdown for a single object. */
void rate_object_detailed(P_char ch, P_obj obj)
{
	char *buf;
	char wear_buf[MAX_STRING_LENGTH];
	char extra_buf[MAX_STRING_LENGTH];
	char extra2_buf[MAX_STRING_LENGTH];
	char type_buf[MAX_STRING_LENGTH];
	char aff_name[64];
	char factor[64];
	char detail[64];
	size_t len = 0;
	int total_rating = 0;
	int i, vnum;

	if (!ch || !obj)
		return;

	CREATE(buf, char, MAX_STRING_LENGTH, MEM_TAG_BUFFER);
	buf[0] = '\0';

	vnum = (obj->R_num >= 0) ? obj_index[obj->R_num].virtual_number : 0;
	sprintbitde(obj->wear_flags, wear_bits, wear_buf);
	sprintbitde(obj->extra_flags, extra_bits, extra_buf);
	sprintbitde(obj->extra2_flags, extra2_bits, extra2_buf);
	sprinttype(GET_ITEM_TYPE(obj), item_types, type_buf);

	eqrate_cat(
		buf, MAX_STRING_LENGTH, &len,
		"&+B==============================================================================&N\n"
		"&+W Rating Breakdown for: &+Y[%5d] &+W%s&N\n"
		"&+B------------------------------------------------------------------------------&N\n"
		" &+CItem Type:&N   %-15s &+CWear Bits:&N %s\n"
		" &+CExtra Flags:&N %s\n"
		" &+CExtra2 Flags:&N %s\n"
		"&+B------------------------------------------------------------------------------&N\n"
		" &+W%-33.33s  %-33.33s  %6s&N\n"
		"&+B------------------------------------------------------------------------------&N\n",
		vnum, obj->short_description ? obj->short_description : "undefined", type_buf,
		wear_buf, extra_buf, extra2_buf, "Factor", "Detail", "Score");

	/* 1. Base item type contributions. */
	if (obj->type == ITEM_ARMOR)
	{
		int score = obj->value[0] * 5;
		total_rating += score;
		snprintf(detail, sizeof(detail), "AC apply: %d", obj->value[0]);
		eqrate_row(buf, MAX_STRING_LENGTH, &len, "Base Armor AC", detail, score);
	}
	else if (obj->type == ITEM_SHIELD)
	{
		int score = obj->value[3] * 5;
		total_rating += score;
		checked_snprintf(detail, sizeof(detail), "AC apply: %d", obj->value[3]);
		eqrate_row(buf, MAX_STRING_LENGTH, &len, "Base Shield AC", detail, score);
	}
	else if (obj->type == ITEM_WEAPON)
	{
		if (obj->value[1] > 0 && obj->value[2] > 0)
		{
			float avg_dam = (float)obj->value[1] * (float)(obj->value[2] + 1) / 2.0f;
			int score = (int)(avg_dam * 8.0f);
			total_rating += score;
			checked_snprintf(detail, sizeof(detail), "Dice: %dD%d (avg %.1f)",
					 obj->value[1], obj->value[2], avg_dam);
			eqrate_row(buf, MAX_STRING_LENGTH, &len, "Base Weapon Damage", detail,
				   score);
		}
	}
	else if (obj->type == ITEM_FIREWEAPON)
	{
		int score = obj->value[2] * 4;
		total_rating += score;
		snprintf(detail, sizeof(detail), "Bonus: %d", obj->value[2]);
		eqrate_row(buf, MAX_STRING_LENGTH, &len, "Ranged Damage Bonus", detail, score);

		score = obj->value[0] * 2;
		total_rating += score;
		checked_snprintf(detail, sizeof(detail), "Speed cap: %d", obj->value[0]);
		eqrate_row(buf, MAX_STRING_LENGTH, &len, "Ranged Firing Speed", detail, score);
	}
	else if (obj->type == ITEM_MISSILE)
	{
		if (obj->value[1] > 0 && obj->value[2] > 0)
		{
			float avg_dam = (float)obj->value[1] * (float)(obj->value[2] + 1) / 2.0f;
			int score = (int)(avg_dam * 8.0f);
			total_rating += score;
			checked_snprintf(detail, sizeof(detail), "Dice: %dD%d (avg %.1f)",
					 obj->value[1], obj->value[2], avg_dam);
			eqrate_row(buf, MAX_STRING_LENGTH, &len, "Missile Damage", detail, score);
		}
	}

	/* 2. Affects. */
	for (i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		int loc = obj->affected[i].location;
		int mod;
		int score;

		if (!loc)
			continue;

		mod = obj->affected[i].modifier;
		score = eqrate_apply_score(loc, mod);
		total_rating += score;

		sprinttype(loc, apply_types, aff_name);
		snprintf(factor, sizeof(factor), "Affect [%d]", i + 1);
		checked_snprintf(detail, sizeof(detail), "%s %+d", aff_name, mod);
		eqrate_row(buf, MAX_STRING_LENGTH, &len, factor, detail, score);
	}

	/* 3. Extra flags. */
	{
		static const struct
		{
			uint bit;
			const char *name;
			int points;
		} extra_score[] = { { ITEM_ARTIFACT, "ARTIFACT", 50 },
				    { ITEM_NOSLEEP, "NOSLEEP", 15 },
				    { ITEM_NOCHARM, "NOCHARM", 15 },
				    { ITEM_RETURNING, "RETURNING", 10 },
				    { ITEM_FLOAT, "FLOAT", 5 },
				    { ITEM_LEVITATES, "LEVITATES", 5 },
				    { ITEM_GLOW, "GLOW", 2 },
				    { ITEM_NODROP, "NODROP", -20 },
				    { 0, NULL, 0 } };
		static const struct
		{
			uint bit;
			const char *name;
			int points;
		} extra2_score[] = { { ITEM2_MAGIC, "MAGIC", 10 },
				     { ITEM2_BLESS, "BLESS", 5 },
				     { ITEM2_SILVER, "SILVER", 10 },
				     { ITEM2_ENHANCED, "ENHANCED", 10 },
				     { ITEM2_SLAY_GOOD, "SLAY_GOOD", 15 },
				     { ITEM2_SLAY_EVIL, "SLAY_EVIL", 15 },
				     { ITEM2_SLAY_UNDEAD, "SLAY_UNDEAD", 15 },
				     { ITEM2_SLAY_LIVING, "SLAY_LIVING", 15 },
				     { 0, NULL, 0 } };

		for (i = 0; extra_score[i].name; i++)
			if (IS_SET(obj->extra_flags, extra_score[i].bit))
			{
				total_rating += extra_score[i].points;
				eqrate_row(buf, MAX_STRING_LENGTH, &len, "Extra Flag",
					   extra_score[i].name, extra_score[i].points);
			}

		for (i = 0; extra2_score[i].name; i++)
			if (IS_SET(obj->extra2_flags, extra2_score[i].bit))
			{
				total_rating += extra2_score[i].points;
				eqrate_row(buf, MAX_STRING_LENGTH, &len, "Extra2 Flag",
					   extra2_score[i].name, extra2_score[i].points);
			}
	}

	/* 4. Continuous affects. */
	for (i = 0; eqrate_aff_table[i].name; i++)
		if (IS_SET(eqrate_bank_bits(obj, eqrate_aff_table[i].bank),
			   eqrate_aff_table[i].bit))
		{
			total_rating += eqrate_aff_table[i].points;
			eqrate_row(buf, MAX_STRING_LENGTH, &len, "Continuous Affect",
				   eqrate_aff_table[i].name, eqrate_aff_table[i].points);
		}

	/* 5. Special proc. */
	if ((obj->R_num >= 0 && obj_index[obj->R_num].func.obj != NULL) ||
	    IS_SET(obj->extra_flags, ITEM_PROCLIB))
	{
		total_rating += 25;
		eqrate_row(buf, MAX_STRING_LENGTH, &len, "Special Procedure", "Attached", 25);
	}

	/* 6. Trap. */
	if (obj->trap_eff > 0 || obj->trap_dam > 0)
	{
		int score = 15 + (obj->trap_dam / 5);
		total_rating += score;
		snprintf(detail, sizeof(detail), "Damage: %d", obj->trap_dam);
		eqrate_row(buf, MAX_STRING_LENGTH, &len, "Trapped Item", detail, score);
	}

	eqrate_cat(
		buf, MAX_STRING_LENGTH, &len,
		"&+B==============================================================================&N\n"
		" &+W%-33.33s&N  &+w%-33.33s&N  &+Y%6d&N\n"
		"&+B==============================================================================&N\n",
		"TOTAL EQUIPMENT RATING", "", total_rating);

	if (ch->desc)
		page_string(ch->desc, buf, 1);
	else
		send_to_char(buf, ch);

	FREE(buf);
}

struct eqrate_entry
{
	int vnum;
	int rating;
	int r_num;
	int type;
};

static int eqrate_compare(const void *a, const void *b)
{
	const struct eqrate_entry *ra = (const struct eqrate_entry *)a;
	const struct eqrate_entry *rb = (const struct eqrate_entry *)b;

	if (rb->rating != ra->rating)
		return (rb->rating - ra->rating);

	return (ra->vnum - rb->vnum);
}

/* Loads a prototype copy, rates it in detail, and throws the copy away. */
static void eqrate_show_vnum(P_char ch, int vnum)
{
	int r_num = real_object(vnum);
	P_obj obj;

	if (r_num < 0)
	{
		send_to_char("No object exists with that virtual number.\n", ch);
		return;
	}

	if (!(obj = read_object(r_num, REAL)))
	{
		send_to_char("Failed to load object prototype.\n", ch);
		return;
	}

	rate_object_detailed(ch, obj);
	extract_obj(obj, FALSE);
}

/* Command to list equipment prototypes by wear slot and rating. */
void do_eqrate(P_char ch, char *argument, int /*cmd*/)
{
	struct eqrate_entry *ratings = NULL;
	char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char *out_buf = NULL;
	size_t len = 0;
	unsigned int wear_bit = 0;
	int num_found = 0;
	int max_display = 100;
	int is_all = 0;
	int i;

	if (!ch)
		return;

	if (!argument)
		argument = (char *)"";

	argument = one_argument(argument, arg);

	if (!*arg)
	{
		send_to_char(
			"&+WUsage:&N  eqrate <wear position | all> [limit]\n"
			"        eqrate <vnum>\n"
			"        eqrate check <vnum>\n\n"
			"&+WValid positions:&N\n"
			"  finger/ring, neck, body/torso, head/helm, legs, feet/boots,\n"
			"  hands/gloves, arms, shield, about/cloak, waist, wrist/bracer,\n"
			"  wield/weapon, hold, throw, eyes, face/mask, earring/ear, quiver,\n"
			"  back, belt, horse, tail, nose, horn, ioun, spider, insignia/badge,\n"
			"  all\n",
			ch);
		return;
	}

	/* eqrate <vnum> */
	if (isdigit(*arg))
	{
		eqrate_show_vnum(ch, atoi(arg));
		return;
	}

	/* eqrate check|stats|show|detail <vnum> */
	if (!str_cmp(arg, "check") || !str_cmp(arg, "stats") || !str_cmp(arg, "show") ||
	    !str_cmp(arg, "detail"))
	{
		one_argument(argument, arg2);
		if (!*arg2 || !isdigit(*arg2))
		{
			send_to_char("Usage: eqrate check <vnum>\n", ch);
			return;
		}
		eqrate_show_vnum(ch, atoi(arg2));
		return;
	}

	/* Map the wear position onto a wear_flags bit. */
	if (!str_cmp(arg, "finger") || !str_cmp(arg, "ring"))
		wear_bit = ITEM_WEAR_FINGER;
	else if (!str_cmp(arg, "neck"))
		wear_bit = ITEM_WEAR_NECK;
	else if (!str_cmp(arg, "body") || !str_cmp(arg, "torso") || !str_cmp(arg, "chest") ||
		 !str_cmp(arg, "armor"))
		wear_bit = ITEM_WEAR_BODY;
	else if (!str_cmp(arg, "head") || !str_cmp(arg, "helm") || !str_cmp(arg, "helmet"))
		wear_bit = ITEM_WEAR_HEAD;
	else if (!str_cmp(arg, "legs") || !str_cmp(arg, "leg") || !str_cmp(arg, "leggings"))
		wear_bit = ITEM_WEAR_LEGS;
	else if (!str_cmp(arg, "feet") || !str_cmp(arg, "foot") || !str_cmp(arg, "boots"))
		wear_bit = ITEM_WEAR_FEET;
	else if (!str_cmp(arg, "hands") || !str_cmp(arg, "hand") || !str_cmp(arg, "gloves") ||
		 !str_cmp(arg, "gauntlets"))
		wear_bit = ITEM_WEAR_HANDS;
	else if (!str_cmp(arg, "arms") || !str_cmp(arg, "arm") || !str_cmp(arg, "sleeves"))
		wear_bit = ITEM_WEAR_ARMS;
	else if (!str_cmp(arg, "shield"))
		wear_bit = ITEM_WEAR_SHIELD;
	else if (!str_cmp(arg, "about") || !str_cmp(arg, "cloak") || !str_cmp(arg, "cape"))
		wear_bit = ITEM_WEAR_ABOUT;
	else if (!str_cmp(arg, "waist"))
		wear_bit = ITEM_WEAR_WAIST;
	else if (!str_cmp(arg, "wrist") || !str_cmp(arg, "bracer") || !str_cmp(arg, "bracelet"))
		wear_bit = ITEM_WEAR_WRIST;
	else if (!str_cmp(arg, "wield") || !str_cmp(arg, "weapon") || !str_cmp(arg, "primary") ||
		 !str_cmp(arg, "mainhand"))
		wear_bit = ITEM_WIELD;
	else if (!str_cmp(arg, "hold") || !str_cmp(arg, "held") || !str_cmp(arg, "offhand") ||
		 !str_cmp(arg, "secondary"))
		wear_bit = ITEM_HOLD;
	else if (!str_cmp(arg, "throw") || !str_cmp(arg, "thrown") || !str_cmp(arg, "missile"))
		wear_bit = ITEM_THROW;
	else if (!str_cmp(arg, "eyes") || !str_cmp(arg, "eye") || !str_cmp(arg, "glasses") ||
		 !str_cmp(arg, "goggles"))
		wear_bit = ITEM_WEAR_EYES;
	else if (!str_cmp(arg, "face") || !str_cmp(arg, "mask") || !str_cmp(arg, "visor"))
		wear_bit = ITEM_WEAR_FACE;
	else if (!str_cmp(arg, "earring") || !str_cmp(arg, "ear") || !str_cmp(arg, "ears"))
		wear_bit = ITEM_WEAR_EARRING;
	else if (!str_cmp(arg, "quiver"))
		wear_bit = ITEM_WEAR_QUIVER;
	else if (!str_cmp(arg, "insignia") || !str_cmp(arg, "badge"))
		wear_bit = ITEM_GUILD_INSIGNIA;
	else if (!str_cmp(arg, "back"))
		wear_bit = ITEM_WEAR_BACK;
	else if (!str_cmp(arg, "belt"))
		wear_bit = ITEM_ATTACH_BELT;
	else if (!str_cmp(arg, "horse") || !str_cmp(arg, "barding"))
		wear_bit = ITEM_HORSE_BODY;
	else if (!str_cmp(arg, "tail"))
		wear_bit = ITEM_WEAR_TAIL;
	else if (!str_cmp(arg, "nose"))
		wear_bit = ITEM_WEAR_NOSE;
	else if (!str_cmp(arg, "horn") || !str_cmp(arg, "horns"))
		wear_bit = ITEM_WEAR_HORN;
	else if (!str_cmp(arg, "ioun") || !str_cmp(arg, "iounstone"))
		wear_bit = ITEM_WEAR_IOUN;
	else if (!str_cmp(arg, "spider"))
		wear_bit = ITEM_SPIDER_BODY;
	else if (!str_cmp(arg, "all") || !str_cmp(arg, "any") || !str_cmp(arg, "*"))
		is_all = 1;
	else
	{
		send_to_char(
			"Invalid wear position. Type 'eqrate' with no arguments for valid positions.\n",
			ch);
		return;
	}

	/* Optional display limit: eqrate <slot> [limit] */
	one_argument(argument, arg2);
	if (*arg2 && isdigit(*arg2))
		max_display = BOUNDED(1, atoi(arg2), EQRATE_MAX_LIST);

	CREATE(ratings, struct eqrate_entry, top_of_objt + 1, MEM_TAG_ARRAY);

	for (int r_num = 0; r_num <= top_of_objt; r_num++)
	{
		P_obj obj = read_object(r_num, REAL);

		if (!obj)
			continue;

		if (is_all || IS_SET(obj->wear_flags, wear_bit))
		{
			ratings[num_found].vnum = obj_index[r_num].virtual_number;
			ratings[num_found].rating = rate_object(obj);
			ratings[num_found].r_num = r_num;
			ratings[num_found].type = GET_ITEM_TYPE(obj);
			num_found++;
		}

		extract_obj(obj, FALSE);
	}

	if (num_found > 0)
		qsort(ratings, num_found, sizeof(struct eqrate_entry), eqrate_compare);

	CREATE(out_buf, char, MAX_STRING_LENGTH, MEM_TAG_BUFFER);
	out_buf[0] = '\0';

	eqrate_cat(
		out_buf, MAX_STRING_LENGTH, &len,
		"&+WTop equipment for &+C%s&+W slot (showing %d of %d items found):&N\n"
		"&+B==============================================================================&N\n"
		" &+W%5s   %6s   %-10.10s  %s&N\n"
		"&+B------------------------------------------------------------------------------&N\n",
		is_all ? "ALL" : arg, MIN(num_found, max_display), num_found, "Vnum", "Rating",
		"Type", "Name");

	for (i = 0; i < num_found && i < max_display; i++)
	{
		const char *t_name = (ratings[i].type >= 0 && ratings[i].type <= ITEM_LAST) ?
					     item_types[ratings[i].type] :
					     "OTHER";
		const char *name = obj_index[ratings[i].r_num].desc2;

		eqrate_cat(out_buf, MAX_STRING_LENGTH, &len,
			   " &+Y%5d&N   &+G%6d&N   &+c%-10.10s&N  %s&N\n", ratings[i].vnum,
			   ratings[i].rating, t_name, (name && *name) ? name : "Unnamed");
	}

	eqrate_cat(
		out_buf, MAX_STRING_LENGTH, &len,
		"&+B==============================================================================&N\n"
		"&+WTotal items found for this slot: &+Y%d&N\n",
		num_found);

	if (ch->desc)
		page_string(ch->desc, out_buf, 1);
	else
		send_to_char(out_buf, ch);

	FREE(out_buf);
	FREE(ratings);
}
