#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/vnum.obj.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "magic/spells.h"
#include "item/objmisc.h"
#include "core/safe_format.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

extern Skill skills[];
extern bool identify_random(P_obj);
extern P_index obj_index;
extern const char *apply_types[];
extern const flagDef extra_bits[];
extern const flagDef affected1_bits[];
extern const flagDef affected2_bits[];
extern const flagDef affected3_bits[];
extern const flagDef affected4_bits[];
extern const flagDef affected5_bits[];
extern const char *item_types[];

extern char *spells[];

#define STRARR_ELEM 64
#define STRARR_LEN 128

void spell_identify(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		    P_obj obj)
{
	int i, currelem, temp;
	bool found;
	char Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH], Gbuf3[MAX_STRING_LENGTH];
	char strarr[STRARR_ELEM][STRARR_LEN];

	if (obj)
	{
		bzero(strarr, STRARR_ELEM * STRARR_LEN);
		currelem = 0;

		if (level < 60 && IS_SET(obj->extra_flags, ITEM_NOIDENTIFY))
		{
			if (level < 51)
			{
				send_to_char(
					"You cannot seem to glean any information about that item.\n",
					ch);
				return;
			}
			else
				send_to_char(
					"This item has an enchantment designed to make it unidentifiable, but with your superior experience, you divine its true nature regardless.\n",
					ch);
		}
		snprintf(Gbuf1, MAX_STRING_LENGTH,
			 "%s&n weighs %d pounds and is worth roughly %s.\n", obj->short_description,
			 GET_OBJ_WEIGHT(obj), coin_stringv(obj->cost));
		send_to_char(Gbuf1, ch);

		if (obj->bitvector || obj->bitvector2 || obj->bitvector3 || obj->bitvector4 ||
		    obj->bitvector5)
		{
			send_to_char("The following abilities are granted when using this item:\n",
				     ch);

			*Gbuf2 = '\0';

			if (obj->bitvector)
				sprintbitde(obj->bitvector, affected1_bits, Gbuf2);

			if (obj->bitvector2)
			{
				sprintbitde(obj->bitvector2, affected2_bits, Gbuf1);
				strcat(Gbuf2, Gbuf1);
			}

			if (obj->bitvector3)
			{
				sprintbitde(obj->bitvector3, affected3_bits, Gbuf1);
				strcat(Gbuf2, Gbuf1);
			}

			if (obj->bitvector4)
			{
				sprintbitde(obj->bitvector4, affected4_bits, Gbuf1);
				strcat(Gbuf2, Gbuf1);
			}

			if (obj->bitvector5)
			{
				sprintbitde(obj->bitvector5, affected5_bits, Gbuf1);
				strcat(Gbuf2, Gbuf1);
			}

			strcat(Gbuf2, "\n");
			send_to_char(Gbuf2, ch);
		}

		if (IS_SET(obj->extra2_flags, ITEM2_MAGIC))
			strcpy(strarr[currelem++], "magical");

		if (IS_SET(obj->extra_flags, ITEM_ARTIFACT))
			strcpy(strarr[currelem++], "an artifact");

		if (IS_SET(obj->extra_flags, ITEM_NOSLEEP))
			strcpy(strarr[currelem++], "protection against being slept");

		if (IS_SET(obj->extra_flags, ITEM_NOCHARM))
			strcpy(strarr[currelem++], "protection against being charmed");

		if (IS_SET(obj->extra_flags, ITEM_NOSUMMON))
			strcpy(strarr[currelem++], "protection against being summoned");

		if (IS_SET(obj->extra_flags, ITEM_FLOAT))
			strcpy(strarr[currelem++], "able to float on water");

		if (IS_SET(obj->extra_flags, ITEM_LEVITATES))
			strcpy(strarr[currelem++], "able to levitate in mid-air");

		if (IS_SET(obj->extra_flags, ITEM_TWOHANDS))
			strcpy(strarr[currelem++], "two-handed");

		if (IS_SET(obj->extra_flags, ITEM_WHOLE_BODY))
			strcpy(strarr[currelem++], "whole-body");

		if (IS_SET(obj->extra_flags, ITEM_WHOLE_HEAD))
			strcpy(strarr[currelem++], "whole-head");

		if (IS_SET(obj->extra_flags, ITEM_NODROP))
			strcpy(strarr[currelem++], "cursed");

		if (IS_SET(obj->extra2_flags, ITEM2_BLESS))
			strcpy(strarr[currelem++], "blessed");

		if (IS_SET(obj->extra_flags, ITEM_LIT))
			strcpy(strarr[currelem++], "lit");

		if (IS_SET(obj->extra_flags, ITEM_NOLOCATE))
			strcpy(strarr[currelem++], "not locateable");

		if (IS_SET(obj->extra_flags, ITEM_CAN_THROW1) ||
		    IS_SET(obj->extra_flags, ITEM_CAN_THROW2))
			strcpy(strarr[currelem++], "throwable");

		if (IS_SET(obj->extra_flags, ITEM_RETURNING))
			strcpy(strarr[currelem++], "automatically returning");

		if (currelem)
			send_to_char("The item is ", ch);

		for (i = 0; i < currelem; i++)
		{
			/* last entry */

			if (i == (currelem - 1))
			{
				if (currelem > 1)
					snprintf(Gbuf2, MAX_STRING_LENGTH, "and %s.\n", strarr[i]);
				else
					snprintf(Gbuf2, MAX_STRING_LENGTH, "%s.\n", strarr[i]);
			}
			else
			{
				if (currelem > 2)
					snprintf(Gbuf2, MAX_STRING_LENGTH, "%s, ", strarr[i]);
				else
					snprintf(Gbuf2, MAX_STRING_LENGTH, "%s ", strarr[i]);
			}
			send_to_char(Gbuf2, ch);
		}

		if ((obj_index[obj->R_num].func.obj &&
		     obj_index[obj->R_num].virtual_number != VOBJ_RANDOM_ARMOR) ||
		    (GET_ITEM_TYPE(obj) == ITEM_WEAPON && obj->value[5] &&
		     (GET_LEVEL(ch) < 56 || !GET_CLASS(ch, CLASS_CONJURER))))
			send_to_char(
				"This item appears to be imbued with some magic beyond your comprehension!\n",
				ch);

		if ((isname("_id_", obj->name) && !obj->str_mask) ||
		    (obj_index[obj->R_num].virtual_number == VOBJ_RANDOM_WEAPON && obj->value[5] &&
		     !strstr(obj->short_description, " of ")))
			act("You feel as if there is something more to $p that you can't quite reveal!",
			    FALSE, ch, obj, 0, TO_CHAR);

		switch (GET_ITEM_TYPE(obj))
		{
		case ITEM_SCROLL:
		case ITEM_POTION:
			snprintf(Gbuf1, MAX_STRING_LENGTH,
				 "It appears to be a %s these level %d spells:\n",
				 (GET_ITEM_TYPE(obj) == ITEM_SCROLL) ? "scroll charged with" :
								       "potion that grants",
				 obj->value[0]);
			send_to_char(Gbuf1, ch);

			if (obj->value[1] >= 1)
			{
				i = obj->value[1];
				if (i < 1)
					i = 0;
				else if (i > LAST_SPELL)
					i = LAST_SPELL;

				sprinttype(i, (const char **)spells, Gbuf1);
				strcat(Gbuf1, " ");
				send_to_char(Gbuf1, ch);
			}
			if (obj->value[2] >= 1)
			{
				i = obj->value[2];
				if (i < 1)
					i = 1;
				else if (i > LAST_SPELL)
					i = LAST_SPELL;

				sprinttype(i, (const char **)spells, Gbuf1);
				strcat(Gbuf1, " ");
				send_to_char(Gbuf1, ch);
			}
			if (obj->value[3] >= 1)
			{
				i = obj->value[3];
				if (i < 1)
					i = 1;
				else if (i > LAST_SPELL)
					i = LAST_SPELL;

				sprinttype(i, (const char **)spells, Gbuf1);
				send_to_char(Gbuf1, ch);
			}
			send_to_char("\n", ch);
			break;

		case ITEM_WAND:
		case ITEM_STAFF:
			snprintf(
				Gbuf1, MAX_STRING_LENGTH,
				"It appears to be a %s with %d out of %d charges left, granting the\n",
				((GET_ITEM_TYPE(obj) == ITEM_WAND) ? "wand" : "staff"),
				obj->value[2], obj->value[1]);
			send_to_char(Gbuf1, ch);

			if (obj->value[3] >= 1)
			{
				i = obj->value[3];
				if (i < 1)
					i = 1;
				else if (i > LAST_SPELL)
					i = LAST_SPELL;

				sprinttype(i, (const char **)spells, Gbuf2);

				checked_snprintf(Gbuf1, MAX_STRING_LENGTH,
						 "level %d spell \"%s\"\n", obj->value[0], Gbuf2);
				send_to_char(Gbuf1, ch);
			}
			break;

		case ITEM_FOOD:
			send_to_char("You magically sense the nourishment effects:\n", ch);
			send_to_char(food_modifiers(obj), ch);
			send_to_char("\n", ch);
			break;
		case ITEM_FIREWEAPON:
			snprintf(Gbuf1, MAX_STRING_LENGTH,
				 "You magically sense that this weapons rate of fire is &+W%d&n "
				 "and it's range is &+W%d&n\n",
				 obj->value[0], obj->value[1]);
			send_to_char(Gbuf1, ch);
			break;
		case ITEM_MISSILE:
			snprintf(
				Gbuf1, MAX_STRING_LENGTH,
				"You magically sense that the damage dice for this type of arrow are '%dD%d'\n",
				obj->value[1], obj->value[2]);
			send_to_char(Gbuf1, ch);
			break;
		case ITEM_WEAPON:
			snprintf(
				Gbuf1, MAX_STRING_LENGTH,
				"You magically sense that the damage dice for this weapon are '%dD%d'\n",
				obj->value[1], obj->value[2]);
			send_to_char(Gbuf1, ch);
			if (obj_index[obj->R_num].func.obj == NULL && obj->value[5] &&
			    GET_LEVEL(ch) >= 56 && GET_CLASS(ch, CLASS_CONJURER))
			{
				int spells[3];
				char spell_list[512];

				spells[0] = obj->value[5] % 1000;
				spells[1] = obj->value[5] % 1000000 / 1000;
				spells[2] = obj->value[5] % 1000000000 / 1000000;

				if (skills[spells[0]].name)
					strcpy(spell_list, skills[spells[0]].name);
				if (spells[1] && skills[spells[1]].name)
					APPENDF(spell_list, "&n and &+W%s", skills[spells[1]].name);
				if (spells[2] && skills[spells[2]].name)
					APPENDF(spell_list, "&n and &+W%s", skills[spells[2]].name);

				snprintf(
					Gbuf1, MAX_STRING_LENGTH,
					"You recognize the forces of &+W%s &nimbued within this item.\n",
					spell_list);
				send_to_char(Gbuf1, ch);
			}
			break;

		case ITEM_ARMOR:
		case ITEM_WORN:
			/* include obj affects for armor and worn items.. */

			break;
		}

		found = FALSE;

		for (i = 0; i < MAX_OBJ_AFFECT; i++)
		{
			if ((obj->affected[i].location != APPLY_NONE) &&
			    (obj->affected[i].modifier != 0))
			{
				if (!found)
				{
					send_to_char("This item also affects your:\n", ch);
					found = TRUE;
				}
				sprinttype(obj->affected[i].location, apply_types, Gbuf2);

				switch (obj->affected[i].location)
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
				case APPLY_HIT:
					if (obj->affected[i].modifier < 0)
						strcpy(Gbuf3, "negatively by ");
					else
						strcpy(Gbuf3, "positively by ");

					temp = obj->affected[i].modifier;

					if (temp > 30)
						strcat(Gbuf3, "a whole hell of a lot");
					else if (temp > 20)
						strcat(Gbuf3, "an impressive amount");
					else if (temp > 10)
						strcat(Gbuf3, "quite a bit");
					else if (temp > 5)
						strcat(Gbuf3, "enough to mean something");
					else if (temp > 2)
						strcat(Gbuf3, "a small amount");
					else
						strcat(Gbuf3, "a negligible amount");

					break;

				case APPLY_HITROLL:
				case APPLY_DAMROLL:
					if (obj->affected[i].modifier < 0)
						strcpy(Gbuf3, "negatively by ");
					else
						strcpy(Gbuf3, "positively by ");

					temp = obj->affected[i].modifier;

					if (temp > 8)
						strcat(Gbuf3, "an impressive amount");
					else if (temp > 5)
						strcat(Gbuf3, "quite a bit");
					else if (temp > 2)
						strcat(Gbuf3, "a fair amount");
					else
						strcat(Gbuf3, "a small amount");

					break;

				case APPLY_SAVING_PARA:
				case APPLY_SAVING_ROD:
				case APPLY_SAVING_FEAR:
				case APPLY_SAVING_BREATH:
				case APPLY_SAVING_SPELL:
					if (obj->affected[i].modifier > 0)
						strcpy(Gbuf3, "negatively by ");
					else
						strcpy(Gbuf3, "positively by ");

					temp = obj->affected[i].modifier;

					if (temp > 8)
						strcat(Gbuf3, "an impressive amount");
					else if (temp > 5)
						strcat(Gbuf3, "quite a bit");
					else if (temp > 2)
						strcat(Gbuf3, "a fair amount");
					else
						strcat(Gbuf3, "a small amount");

					break;

				default:
					if (obj->affected[i].modifier < 0)
						strcpy(Gbuf3, "negatively by ");
					else
						strcpy(Gbuf3, "positively by ");

					temp = obj->affected[i].modifier;

					if (temp > 8)
						strcat(Gbuf3, "an impressive amount");
					else if (temp > 5)
						strcat(Gbuf3, "quite a bit");
					else if (temp > 2)
						strcat(Gbuf3, "a fair amount");
					else
						strcat(Gbuf3, "a small amount");

					break;
				}

				checked_snprintf(Gbuf1, MAX_STRING_LENGTH, "  %s %s%d\n", Gbuf2,
						 temp > 0 ? "by +" : "by ", temp);
				send_to_char(Gbuf1, ch);
			}
		}
		snprintf(Gbuf1, MAX_STRING_LENGTH, "$p &nhas an item value of &+W%d&n.",
			 itemvalue(obj));
		act(Gbuf1, FALSE, ch, obj, 0, TO_CHAR);
	}
	else
	{
		send_to_char("This spell only works on objects, sorry.\n", ch);
		return;
	}
}

#undef STRARR_ELEM
#undef STRARR_LEN

void spell_lore(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		P_obj obj)
{
	int i, percent = 0;
	bool found;
	char Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH], Gbuf3[256];

	if (obj)
	{
		if (IS_SET(obj->extra_flags, ITEM_NOIDENTIFY))
		{
			if (GET_LEVEL(ch) < AVATAR)
			{
				send_to_char(
					"You recall no legends nor stories ever told about this item.",
					ch);
				return;
			}
		}
		snprintf(Gbuf1, MAX_STRING_LENGTH,
			 "'%s'\nWeight %d, Item type: ", obj->short_description,
			 GET_OBJ_WEIGHT(obj));
		sprinttype(GET_ITEM_TYPE(obj), item_types, Gbuf2);
		strcat(Gbuf1, Gbuf2);
		strcat(Gbuf1, "\n");
		send_to_char(Gbuf1, ch);

		if (obj->bitvector || obj->bitvector2)
		{
			if (obj->bitvector)
				sprintbitde(obj->bitvector, affected1_bits, Gbuf1);

			if (obj->bitvector2)
			{
				sprintbitde(obj->bitvector2, affected2_bits, Gbuf2);
				strcat(Gbuf1, Gbuf2);
			}

			send_to_char("Item will give you following abilities:  ", ch);
			strcat(Gbuf1, "\n");
			send_to_char(Gbuf1, ch);
		}
		send_to_char("Item is: ", ch);
		sprintbitde(obj->extra_flags, extra_bits, Gbuf1);
		strcat(Gbuf1, "\n");
		send_to_char(Gbuf1, ch);

		switch (GET_ITEM_TYPE(obj))
		{
		case ITEM_FOOD:
			send_to_char("You magically sense the nourishment effects:\n", ch);
			send_to_char(food_modifiers(obj), ch);
			send_to_char("\n", ch);
			break;

		case ITEM_SCROLL:
		case ITEM_POTION:
			send_to_char("Contains spells of: ", ch);
			if (obj->value[1] >= 1)
			{
				sprinttype(obj->value[1], (const char **)spells, Gbuf1);
				strcat(Gbuf1, "\n");
				send_to_char(Gbuf1, ch);
			}
			if (obj->value[2] >= 1)
			{
				sprinttype(obj->value[2], (const char **)spells, Gbuf1);
				strcat(Gbuf1, "\n");
				send_to_char(Gbuf1, ch);
			}
			if (obj->value[3] >= 1)
			{
				sprinttype(obj->value[3], (const char **)spells, Gbuf1);
				strcat(Gbuf1, "\n");
				send_to_char(Gbuf1, ch);
			}
			break;

		case ITEM_WAND:
		case ITEM_STAFF:
			/*
				   if(!obj->value[1])
				   return;
				 */
			percent = 100 - (obj->value[1] ? (100 / obj->value[1]) : 100) *
						(obj->value[1] - obj->value[2]);
			snprintf(Gbuf1, MAX_STRING_LENGTH,
				 "%d%% of its charges remain, and it contains the spell of: ",
				 percent);
			send_to_char(Gbuf1, ch);

			if (obj->value[3] >= 1)
			{
				sprinttype(obj->value[3], (const char **)spells, Gbuf1);
				strcat(Gbuf1, "\n");
				send_to_char(Gbuf1, ch);
			}
			break;

		case ITEM_WEAPON:
			snprintf(Gbuf1, MAX_STRING_LENGTH, "Damage Dice is '%dD%d'\n",
				 obj->value[1], obj->value[2]);
			send_to_char(Gbuf1, ch);
			break;

		case ITEM_ARMOR:
			snprintf(Gbuf1, MAX_STRING_LENGTH, "AC-apply is %d\n", obj->value[0]);
			send_to_char(Gbuf1, ch);
			break;
		}

		found = FALSE;
		for (i = 0; i < MAX_OBJ_AFFECT; i++)
		{
			if ((obj->affected[i].location != APPLY_NONE) &&
			    (obj->affected[i].modifier != 0))
			{
				if (found)
					send_to_char(" and ", ch);
				else
				{
					send_to_char("This item will also affect your", ch);
					found = TRUE;
				}
				sprinttype(obj->affected[i].location, apply_types, Gbuf2);

				if ((obj->affected[i].location >= APPLY_SAVING_PARA) &&
				    (obj->affected[i].location <= APPLY_SAVING_SPELL))
				{
					if (obj->affected[i].modifier < 0)
						strcpy(Gbuf3, "positively");
					else if (obj->affected[i].modifier > 0)
						strcpy(Gbuf3, "negatively");
					else
						strcpy(Gbuf3, "not at all");
				}
				else if (obj->affected[i].location != APPLY_FIRE_PROT)
				{
					if (obj->affected[i].modifier > 0)
						strcpy(Gbuf3, "positively");
					else if (obj->affected[i].modifier < 0)
						strcpy(Gbuf3, "negatively");
					else
						strcpy(Gbuf3, "not at all");
				}
				else
					Gbuf3[0] = 0;
				checked_snprintf(Gbuf1, MAX_STRING_LENGTH, " %s %s", Gbuf2, Gbuf3);
				send_to_char(Gbuf1, ch);
			}
		}
		if (found)
			send_to_char(".\n", ch);

		snprintf(Gbuf1, MAX_STRING_LENGTH, "Estimated Value: %s\n",
			 coin_stringv(obj->cost));
		send_to_char(Gbuf1, ch);
		return;
	}
	/* no object, must be mob/player */
	act("The tales have heard nothing on $n.", FALSE, ch, 0, 0, TO_CHAR);
}

void spell_reveal_true_name(int /*level*/, P_char ch, char * /*arg*/, int /*type*/,
			    P_char /*victim*/, P_obj obj)
{
	char newname = FALSE;
	char *eol;

	if (obj_index[obj->R_num].virtual_number == VOBJ_RANDOM_WEAPON && identify_random(obj))
	{
		act("You discover that you are in fact in possession of $p!", FALSE, ch, obj, 0,
		    TO_CHAR);
		return;
	}

	if (isname("_id_", obj->name) && !obj->str_mask)
	{ /* some special ID info */
		struct extra_descr_data *ex;

		for (ex = obj->ex_description; ex; ex = ex->next)
		{ /* find new name */
			if (isname("_id_name_", ex->keyword) && ex->description)
				break;
		}
		if (ex)
		{ /* restring name */
			obj->name = str_dup(ex->description);
			if ((eol = rindex(obj->name, '\r')))
				*eol = '\0';
			SET_BIT(obj->str_mask, STRUNG_KEYS);
			newname = TRUE;
		}
		for (ex = obj->ex_description; ex; ex = ex->next)
		{ /* find new short */
			if (isname("_id_short_", ex->keyword) && ex->description)
				break;
		}
		if (ex)
		{ /* restring short desc */
			/* free(obj->short_description); */
			obj->short_description = str_dup(ex->description);
			if ((eol = rindex(obj->short_description, '\r')))
				*eol = '\0';
			SET_BIT(obj->str_mask, STRUNG_DESC2);
			newname = TRUE;
		}
		for (ex = obj->ex_description; ex; ex = ex->next)
		{ /*find new desc */
			if (isname("_id_desc_", ex->keyword) && ex->description)
				break;
		}
		if (ex)
		{ /* restring description */
			/*free(obj->description); */
			obj->description = str_dup(ex->description);
			if ((eol = rindex(obj->description, '\r')))
				*eol = '\0';
			SET_BIT(obj->str_mask, STRUNG_DESC1);
			newname = TRUE;
		}
	}
	else
	{
		send_to_char("No more information can be gleaned about that item.\n", ch);
		return;
	}

	if (newname)
	{
		send_to_char("You glean the true nature of the item and rename it thusly.\n", ch);
	}
	else // this could happen..
	{
		send_to_char("No more information can be gleaned about that item.\n", ch);
	}
}
