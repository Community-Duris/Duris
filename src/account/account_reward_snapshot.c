#include "prototypes.h"
#include "account/account_reward_snapshot.h"
#include "db.h"
#include "objmisc.h"
#include "magic/spells.h"
#include "utility.h"
#include "utils.h"

#include <cjson/cJSON.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void add_u64(cJSON *root, const char *name, uint64_t value)
{
	char text[32];
	snprintf(text, sizeof(text), "%llu", (unsigned long long)value);
	cJSON_AddStringToObject(root, name, text);
}

static bool read_u64(cJSON *root, const char *name, uint64_t *value)
{
	cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);
	char *end = NULL;
	unsigned long long parsed;
	if (!cJSON_IsString(item) || !item->valuestring || !*item->valuestring)
		return false;
	errno = 0;
	parsed = strtoull(item->valuestring, &end, 10);
	if (errno || !end || *end != '\0')
		return false;
	*value = (uint64_t)parsed;
	return true;
}

static bool read_int(cJSON *root, const char *name, int low, int high, int *value)
{
	cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);
	if (!cJSON_IsNumber(item) || item->valuedouble < low || item->valuedouble > high ||
	    item->valuedouble != item->valueint)
		return false;
	*value = item->valueint;
	return true;
}

static void add_nullable_string(cJSON *root, const char *name, const char *value)
{
	if (value)
		cJSON_AddStringToObject(root, name, value);
	else
		cJSON_AddNullToObject(root, name);
}

static bool replace_string(P_obj obj, cJSON *root, const char *name, char **field, int mask)
{
	cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);
	if (!item || (!cJSON_IsString(item) && !cJSON_IsNull(item)))
		return false;
	if ((*field) && IS_SET(obj->str_mask, mask))
		str_free(*field);
	*field = NULL;
	REMOVE_BIT(obj->str_mask, mask);
	if (cJSON_IsString(item))
	{
		*field = str_dup(item->valuestring ? item->valuestring : "");
		if (!*field)
			return false;
		SET_BIT(obj->str_mask, mask);
	}
	return true;
}

static void clear_extra_descriptions(P_obj obj)
{
	struct extra_descr_data *ed = obj->ex_description;
	while (ed)
	{
		struct extra_descr_data *next = ed->next;
		if (ed->keyword)
			str_free(ed->keyword);
		if (ed->description)
			str_free(ed->description);
		FREE(ed);
		ed = next;
	}
	obj->ex_description = NULL;
}

char *account_reward_snapshot_serialize(P_obj obj)
{
	cJSON *root, *values, *timers, *fixed, *linked, *descriptions;
	char *json;
	if (!obj || obj->R_num < 0)
		return NULL;

	root = cJSON_CreateObject();
	if (!root)
		return NULL;
	cJSON_AddNumberToObject(root, "template_version", ACCOUNT_REWARD_TEMPLATE_VERSION);
	cJSON_AddNumberToObject(root, "vnum", OBJ_VNUM(obj));
	cJSON_AddNumberToObject(root, "type", obj->type);
	add_nullable_string(root, "name", obj->name);
	add_nullable_string(root, "description", obj->description);
	add_nullable_string(root, "short_description", obj->short_description);
	add_nullable_string(root, "action_description", obj->action_description);

	values = cJSON_AddArrayToObject(root, "values");
	for (int i = 0; i < NUMB_OBJ_VALS; ++i)
		cJSON_AddItemToArray(values, cJSON_CreateNumber(obj->value[i]));
	timers = cJSON_AddArrayToObject(root, "timers");
	for (int i = 0; i < 6; ++i)
	{
		char text[32];
		snprintf(text, sizeof(text), "%lld", (long long)obj->timer[i]);
		cJSON_AddItemToArray(timers, cJSON_CreateString(text));
	}

	add_u64(root, "wear_flags", obj->wear_flags);
	add_u64(root, "extra_flags", obj->extra_flags);
	add_u64(root, "anti_flags", obj->anti_flags);
	add_u64(root, "anti2_flags", obj->anti2_flags);
	add_u64(root, "extra2_flags", obj->extra2_flags);
	cJSON_AddNumberToObject(root, "weight", obj->weight);
	cJSON_AddNumberToObject(root, "material", obj->material);
	cJSON_AddNumberToObject(root, "cost", obj->cost);
	cJSON_AddNumberToObject(root, "trap_eff", obj->trap_eff);
	cJSON_AddNumberToObject(root, "trap_dam", obj->trap_dam);
	cJSON_AddNumberToObject(root, "trap_charge", obj->trap_charge);
	cJSON_AddNumberToObject(root, "trap_level", obj->trap_level);
	cJSON_AddNumberToObject(root, "condition", obj->condition);
	cJSON_AddNumberToObject(root, "craftsmanship", obj->craftsmanship);
	cJSON_AddNumberToObject(root, "z_cord", obj->z_cord);
	add_u64(root, "bitvector1", obj->bitvector);
	add_u64(root, "bitvector2", obj->bitvector2);
	add_u64(root, "bitvector3", obj->bitvector3);
	add_u64(root, "bitvector4", obj->bitvector4);
	add_u64(root, "bitvector5", obj->bitvector5);

	fixed = cJSON_AddArrayToObject(root, "fixed_affects");
	for (int i = 0; i < MAX_OBJ_AFFECT; ++i)
	{
		cJSON *entry = cJSON_CreateObject();
		cJSON_AddNumberToObject(entry, "location", obj->affected[i].location);
		cJSON_AddNumberToObject(entry, "modifier", obj->affected[i].modifier);
		cJSON_AddItemToArray(fixed, entry);
	}

	linked = cJSON_AddArrayToObject(root, "linked_affects");
	for (struct obj_affect *af = obj->affects; af; af = af->next)
	{
		cJSON *entry = cJSON_CreateObject();
		cJSON_AddNumberToObject(entry, "type", af->type);
		cJSON_AddNumberToObject(entry, "data", af->data);
		add_u64(entry, "extra2", af->extra2);
		cJSON_AddNumberToObject(entry, "time", obj_affect_time(obj, af));
		cJSON_AddItemToArray(linked, entry);
	}

	descriptions = cJSON_AddArrayToObject(root, "extra_descriptions");
	for (struct extra_descr_data *ed = obj->ex_description; ed; ed = ed->next)
	{
		cJSON *entry = cJSON_CreateObject();
		add_nullable_string(entry, "keyword", ed->keyword);
		add_nullable_string(entry, "description", ed->description);
		cJSON_AddItemToArray(descriptions, entry);
	}

	json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return json;
}

bool account_reward_snapshot_apply(P_obj obj, const char *json, int template_version)
{
	cJSON *root = NULL, *array, *entry;
	uint64_t value;
	int number;
	if (!obj || !json || template_version != ACCOUNT_REWARD_TEMPLATE_VERSION)
		return false;
	root = cJSON_Parse(json);
	if (!root || !cJSON_IsObject(root))
		goto fail;
	if (!read_int(root, "template_version", template_version, template_version, &number))
		goto fail;
	if (!read_int(root, "vnum", 1, INT_MAX, &number) || number != OBJ_VNUM(obj))
		goto fail;
	if (!read_int(root, "type", ITEM_LOWEST, ITEM_LAST, &number))
		goto fail;
	obj->type = (::byte)number;
	if (!replace_string(obj, root, "name", &obj->name, STRUNG_KEYS))
		goto fail;
	if (!replace_string(obj, root, "description", &obj->description, STRUNG_DESC1))
		goto fail;
	if (!replace_string(obj, root, "short_description", &obj->short_description, STRUNG_DESC2))
		goto fail;
	if (!replace_string(obj, root, "action_description", &obj->action_description,
			    STRUNG_DESC3))
		goto fail;

	array = cJSON_GetObjectItemCaseSensitive(root, "values");
	if (!cJSON_IsArray(array) || cJSON_GetArraySize(array) != NUMB_OBJ_VALS)
		goto fail;
	for (int i = 0; i < NUMB_OBJ_VALS; ++i)
	{
		entry = cJSON_GetArrayItem(array, i);
		if (!cJSON_IsNumber(entry) || entry->valuedouble < INT_MIN ||
		    entry->valuedouble > INT_MAX || entry->valuedouble != entry->valueint)
			goto fail;
		obj->value[i] = entry->valueint;
	}
	array = cJSON_GetObjectItemCaseSensitive(root, "timers");
	if (!cJSON_IsArray(array) || cJSON_GetArraySize(array) != 6)
		goto fail;
	for (int i = 0; i < 6; ++i)
	{
		char *end = NULL;
		long long parsed;
		entry = cJSON_GetArrayItem(array, i);
		if (!cJSON_IsString(entry) || !entry->valuestring)
			goto fail;
		errno = 0;
		parsed = strtoll(entry->valuestring, &end, 10);
		if (errno || !end || *end != '\0')
			goto fail;
		obj->timer[i] = (time_t)parsed;
	}

#define APPLY_U64(name, field)                     \
	do                                         \
	{                                          \
		if (!read_u64(root, name, &value)) \
			goto fail;                 \
		field = (decltype(field))value;    \
	} while (0)
	APPLY_U64("wear_flags", obj->wear_flags);
	APPLY_U64("extra_flags", obj->extra_flags);
	APPLY_U64("anti_flags", obj->anti_flags);
	APPLY_U64("anti2_flags", obj->anti2_flags);
	APPLY_U64("extra2_flags", obj->extra2_flags);
	APPLY_U64("bitvector1", obj->bitvector);
	APPLY_U64("bitvector2", obj->bitvector2);
	APPLY_U64("bitvector3", obj->bitvector3);
	APPLY_U64("bitvector4", obj->bitvector4);
	APPLY_U64("bitvector5", obj->bitvector5);
#undef APPLY_U64
#define APPLY_REWARD_INT(name, low, high, field, cast_type)    \
	do                                                     \
	{                                                      \
		if (!read_int(root, name, low, high, &number)) \
			goto fail;                             \
		field = (cast_type)number;                     \
	} while (0)
	APPLY_REWARD_INT("weight", INT_MIN, INT_MAX, obj->weight, int);
	APPLY_REWARD_INT("material", MAT_UNDEFINED, MAT_HIGHEST, obj->material, ::byte);
	APPLY_REWARD_INT("cost", INT_MIN, INT_MAX, obj->cost, int);
	APPLY_REWARD_INT("trap_eff", SHRT_MIN, SHRT_MAX, obj->trap_eff, sh_int);
	APPLY_REWARD_INT("trap_dam", SHRT_MIN, SHRT_MAX, obj->trap_dam, sh_int);
	APPLY_REWARD_INT("trap_charge", SHRT_MIN, SHRT_MAX, obj->trap_charge, sh_int);
	APPLY_REWARD_INT("trap_level", SHRT_MIN, SHRT_MAX, obj->trap_level, sh_int);
	APPLY_REWARD_INT("condition", SHRT_MIN, SHRT_MAX, obj->condition, sh_int);
	APPLY_REWARD_INT("craftsmanship", SHRT_MIN, SHRT_MAX, obj->craftsmanship, sh_int);
	APPLY_REWARD_INT("z_cord", SHRT_MIN, SHRT_MAX, obj->z_cord, sh_int);
#undef APPLY_REWARD_INT

	array = cJSON_GetObjectItemCaseSensitive(root, "fixed_affects");
	if (!cJSON_IsArray(array) || cJSON_GetArraySize(array) != MAX_OBJ_AFFECT)
		goto fail;
	for (int i = 0; i < MAX_OBJ_AFFECT; ++i)
	{
		entry = cJSON_GetArrayItem(array, i);
		if (!cJSON_IsObject(entry) ||
		    !read_int(entry, "location", APPLY_NONE, APPLY_LAST, &number))
			goto fail;
		obj->affected[i].location = (::byte)number;
		if (!read_int(entry, "modifier", SCHAR_MIN, SCHAR_MAX, &number))
			goto fail;
		obj->affected[i].modifier = (sbyte)number;
	}

	while (obj->affects)
		affect_from_obj(obj, obj->affects->type);
	array = cJSON_GetObjectItemCaseSensitive(root, "linked_affects");
	if (!cJSON_IsArray(array))
		goto fail;
	for (int pass = 0; pass < 2; ++pass)
	{
		cJSON_ArrayForEach(entry, array)
		{
			int type, data, time;
			if (!cJSON_IsObject(entry) ||
			    !read_int(entry, "type", SHRT_MIN, SHRT_MAX, &type) ||
			    !read_int(entry, "data", SHRT_MIN, SHRT_MAX, &data) ||
			    !read_int(entry, "time", -1, INT_MAX, &time) ||
			    !read_u64(entry, "extra2", &value))
				goto fail;
			bool altered_extra2 = type == TAG_ALTERED_EXTRA2;
			if ((pass == 0 && altered_extra2) || (pass == 1 && !altered_extra2))
				set_obj_affected_extra(obj, time, (sh_int)type, (sh_int)data,
						       (ulong)value);
		}
	}

	clear_extra_descriptions(obj);
	array = cJSON_GetObjectItemCaseSensitive(root, "extra_descriptions");
	if (!cJSON_IsArray(array))
		goto fail;
	{
		struct extra_descr_data **tail = &obj->ex_description;
		cJSON_ArrayForEach(entry, array)
		{
			cJSON *keyword = cJSON_GetObjectItemCaseSensitive(entry, "keyword"),
			      *description = cJSON_GetObjectItemCaseSensitive(entry, "description");
			struct extra_descr_data *ed;
			if (!cJSON_IsObject(entry) ||
			    (!cJSON_IsString(keyword) && !cJSON_IsNull(keyword)) ||
			    (!cJSON_IsString(description) && !cJSON_IsNull(description)))
				goto fail;
			CREATE(ed, struct extra_descr_data, 1, MEM_TAG_EXDESCD);
			ed->keyword = cJSON_IsString(keyword) ? str_dup(keyword->valuestring) :
								NULL;
			ed->description = cJSON_IsString(description) ?
						  str_dup(description->valuestring) :
						  NULL;
			ed->next = NULL;
			*tail = ed;
			tail = &ed->next;
		}
	}
	cJSON_Delete(root);
	return true;
fail:
	if (root)
		cJSON_Delete(root);
	return false;
}
