/*
 * ***************************************************************************
 *   File: objmisc.c                                       Part of Duris
 *   Usage: Miscellaneous stuff related to objects
 *   Copyright  1990, 1991 - see 'license.doc' for complete information.
 *   Copyright  1994, 1995, 1997 - Duris Systems Ltd.
 *
 * ***************************************************************************
 */

#include "core/prototypes.h"
#include "world/world_activity.h"
#include "core/structs.h"
#include "world/db.h"
#include "core/utils.h"
#include "item/objmisc.h"
#include "world/epic.h"
#include "world/vnum.obj.h"
#include <string.h>
#include <stdio.h>
#include "combat/damage.h"

extern P_room world; /* dyn alloc'ed array of rooms     */
// extern int rev_dir[];
extern struct zone_data *zone_table;
extern flagDef weapon_types[];
extern const char *modenhance_names[];

bool object_has_magical_proc(P_obj obj)
{
	if (!obj)
		return false;
	// Spent epic stones no longer hold detectable special power.
	if (obj->R_num >= 0 && obj_index && obj_index[obj->R_num].func.obj == epic_stone &&
	    !IS_OBJ_STAT2(obj, ITEM2_MAGIC))
		return false;
	// Match Identify's special-magic convention; random armor uses a generic handler.
	return (obj->R_num >= 0 && obj_index && obj_index[obj->R_num].func.obj &&
		obj_index[obj->R_num].virtual_number != VOBJ_RANDOM_ARMOR) ||
	       (obj->type == ITEM_WEAPON && obj->value[5] > 0);
}

/* Format food effects for item catalogue and identify output. */
char *food_modifiers(P_obj food)
{
	static char mod_string[MAX_STRING_LENGTH];
	int sub, mod;

	// Poison is in value[3].
	if (food->value[3] > 0)
	{
		sub = snprintf(mod_string, MAX_STRING_LENGTH, "&+GPOISON&N: %d, HP_REG: %d, ",
			       food->value[3], -food->value[3]);
	}
	else
	{
		mod_string[0] = '\0';
		if ((mod = food->value[1]) == 0)
		{
			mod = 1;
		}
		sub = snprintf(mod_string, MAX_STRING_LENGTH, "HP_REG: %d, ", mod * 15);
		if (food->value[2] != 0)
		{
			mod = food->value[2];
		}
		sub += snprintf(mod_string + sub, MAX_STRING_LENGTH - sub, "MV_REG: %d, ", mod);
	}
	if (food->value[4] != 0)
	{
		sub += snprintf(mod_string + sub, MAX_STRING_LENGTH - sub, "STR&CON: %d, ",
				food->value[4]);
	}
	if (food->value[5] != 0)
	{
		sub += snprintf(mod_string + sub, MAX_STRING_LENGTH - sub, "AGI&DEX: %d, ",
				food->value[5]);
	}
	if (food->value[6] != 0)
	{
		sub += snprintf(mod_string + sub, MAX_STRING_LENGTH - sub, "INT&WIS: %d, ",
				food->value[6]);
	}
	if (food->value[7] != 0)
	{
		sub += snprintf(mod_string + sub, MAX_STRING_LENGTH - sub, "HIT&DAM: %d, ",
				food->value[7]);
	}

	// Duration is in value[0].
	const int tail = (sub > 0) ? sub - 2 : sub;
	snprintf(mod_string + tail, MAX_STRING_LENGTH - tail, " for %d ticks", food->value[0]);

	return mod_string;
}

/*
 * getWeaponDamType
 */

int getWeaponDamType(const int weaptype)
{
	switch (weaptype)
	{
	case WEAPON_SICKLE:
	case WEAPON_2HANDSWORD:
	case WEAPON_SHORTSWORD:
	case WEAPON_LONGSWORD:
	case WEAPON_AXE:
		return WEAPONTYPE_SLASH;

	case WEAPON_LANCE:
	case WEAPON_TRIDENT:
	case WEAPON_HORN:
	case WEAPON_SPEAR:
	case WEAPON_POLEARM:
	case WEAPON_DAGGER:
		return WEAPONTYPE_PIERCE;

	case WEAPON_HAMMER:
	case WEAPON_MACE:
	case WEAPON_SPIKED_MACE:
	case WEAPON_CLUB:
	case WEAPON_SPIKED_CLUB:
	case WEAPON_STAFF:
	case WEAPON_NUMCHUCKS:
		return WEAPONTYPE_BLUDGEON;

	case WEAPON_FLAIL:
	case WEAPON_WHIP:
		return WEAPONTYPE_WHIP;
	}

	return WEAPONTYPE_UNDEFINED;
}

int get_weapon_msg(P_obj weapon)
{
	switch (weapon->value[0])
	{
	case WEAPON_AXE:
	case WEAPON_SHORTSWORD:
	case WEAPON_2HANDSWORD:
	case WEAPON_SICKLE:
	case WEAPON_POLEARM:
	case WEAPON_LONGSWORD:
		return MSG_SLASH;
	case WEAPON_DAGGER:
	case WEAPON_SPEAR:
	case WEAPON_TRIDENT:
	case WEAPON_HORN:
		return MSG_PIERCE;
	case WEAPON_HAMMER:
	case WEAPON_FLAIL:
	case WEAPON_CLUB:
	case WEAPON_SPIKED_CLUB:
	case WEAPON_LANCE:
		return MSG_CRUSH;
	case WEAPON_MACE:
	case WEAPON_SPIKED_MACE:
	case WEAPON_STAFF:
	case WEAPON_NUMCHUCKS:
		return MSG_BLUDGEON;
	case WEAPON_WHIP:
		return MSG_WHIP;
	default:
		return MSG_HIT;
	}
}

void event_random_exit(P_char /*ch*/, P_char /*victim*/, P_obj obj, void * /*data*/)
{
	char buf[512];
	char exit_name[32];
	int exit_dir, s_room, d_room;

	if (!obj)
		return;

	if (obj->value[0] > number(0, 99) && OBJ_ROOM(obj) &&
	    sscanf(obj->name, "%511s exit_%31s ", buf, exit_name) == 2 &&
	    (exit_dir = dir_from_keyword(exit_name)) != -1 &&
	    (d_room = real_room(obj->value[1])) != -1)
	{
		s_room = obj->loc.room;
		if (!world[s_room].dir_option[exit_dir])
		{
			CREATE(world[s_room].dir_option[exit_dir], room_direction_data, 1,
			       MEM_TAG_DIRDATA);
			memset(world[s_room].dir_option[exit_dir], 0,
			       sizeof(struct room_direction_data));
		}
		else
		{ // if an exit exists, we close off the zone the exit leads to
			// if we are using this as a random exit generator instead leading
			// to the same zone, it's ok, because we remove the closed flag of
			// the destination zone below.  Example result: Desolate is closed,
			// and Desolate Under Fire (default closed) becomes opened.  This
			// will help prevent people shifting into the zone when they shouldn't.
			if (!(zone_table[world[(world[s_room].dir_option[exit_dir])->to_room].zone]
				      .flags &
			      ZONE_CLOSED))
			{ // close it...
				SET_BIT(zone_table[world[(world[s_room].dir_option[exit_dir])->to_room]
							   .zone]
						.flags,
					ZONE_CLOSED);
			}
		}
		if (!world[d_room].dir_option[rev_dir[exit_dir]])
		{
			CREATE(world[d_room].dir_option[rev_dir[exit_dir]], room_direction_data, 1,
			       MEM_TAG_DIRDATA);
			memset(world[d_room].dir_option[rev_dir[exit_dir]], 0,
			       sizeof(struct room_direction_data));
		}
		world[s_room].dir_option[exit_dir]->to_room = real_room(obj->value[1]);
		world[d_room].dir_option[rev_dir[exit_dir]]->to_room = s_room;
		world_activity_room_exits_changed(s_room);
		world_activity_room_exits_changed(d_room);
		if (zone_table[world[d_room].zone].flags & ZONE_CLOSED)
			REMOVE_BIT(zone_table[world[d_room].zone].flags, ZONE_CLOSED);
	}

	extract_obj(obj);
}

// Hidden NPC helper equipment must never activate for a player-owned pet.
// Resolve physical custody as well as the explicit actor: periodic callbacks
// have no actor, and speech/defensive dispatch may pass someone else. A live
// holder is authoritative: an NPC defender keeps its helper proc when attacked
// by a PC pet, even though CMD_GOTHIT passes the attacking pet as the actor.
bool item_restricted_for_player_pet(P_char actor, P_obj obj)
{
	if (!obj || !(obj->extra_flags & ITEM_NOSHOW))
		return false;
	P_obj root = obj;
	for (size_t depth = 0; root && depth < 1024; ++depth)
	{
		if (root->loc_p == LOC_CARRIED || root->loc_p == LOC_WORN)
		{
			P_char holder = root->loc.carrying;
			return holder && IS_PC_PET(holder);
		}
		if (root->loc_p != LOC_INSIDE)
			return actor && IS_PC_PET(actor);
		root = root->loc.inside;
	}
	// Fail closed for a malformed/cyclic containment chain; never mutate it.
	return root != nullptr;
}

int invoke_object_special(P_obj obj, P_char actor, int command, char *argument)
{
	if (!obj || item_restricted_for_player_pet(actor, obj) || obj->R_num < 0 || !obj_index ||
	    !obj_index[obj->R_num].func.obj)
		return FALSE;
	return (*obj_index[obj->R_num].func.obj)(obj, actor, command, argument);
}

void item_restrict_player_pet_equipment(P_char actor)
{
	if (!actor || !IS_PC_PET(actor))
		return;
	for (int slot = 0; slot < MAX_WEAR; ++slot)
	{
		P_obj obj = actor->equipment[slot];
		if (obj && item_restricted_for_player_pet(actor, obj))
		{
			// Equipment topology changes, not ownership: retain the object UID,
			// complete child graph and the same pet's custody, without a grant.
			obj = unequip_char(actor, slot, FALSE);
			if (obj)
				obj_to_char(obj, actor);
		}
	}
}

int obj_zone_id(P_obj o)
{
	P_obj tobj = o;

	while (tobj && OBJ_INSIDE(tobj))
		tobj = tobj->loc.inside;

	int zone_id = -1;

	if (!tobj)
	{
		return -1;
	}
	else if (OBJ_ROOM(tobj))
	{
		zone_id = world[tobj->loc.room].zone;
	}
	else if (OBJ_CARRIED(tobj) && tobj->loc.carrying->in_room != NOWHERE)
	{
		zone_id = world[tobj->loc.carrying->in_room].zone;
	}
	else if (OBJ_WORN(tobj) && tobj->loc.wearing->in_room != NOWHERE)
	{
		zone_id = world[tobj->loc.wearing->in_room].zone;
	}

	return zone_id;
}

int obj_room_id(P_obj o)
{
	P_obj tobj = o;

	while (tobj && OBJ_INSIDE(tobj))
		tobj = tobj->loc.inside;

	int room_id = -1;

	if (!tobj)
	{
		return -1;
	}
	else if (OBJ_ROOM(tobj))
	{
		room_id = tobj->loc.room;
	}
	else if (OBJ_CARRIED(tobj) && tobj->loc.carrying->in_room != NOWHERE)
	{
		room_id = tobj->loc.carrying->in_room;
	}
	else if (OBJ_WORN(tobj) && tobj->loc.wearing->in_room != NOWHERE)
	{
		room_id = tobj->loc.wearing->in_room;
	}

	return room_id;
}

const char *type_names[ITEM_LAST + 1] = {
	"nothing",  "light source", "scroll",	  "wand",      "staff",	  "weapon",
	"launcher", "missile",	    "treasure",	  "armor",     "potion",  "clothing",
	"other",    "trash",	    "wall",	  "container", "note",	  "drink container",
	"key",	    "food",	    "money",	  "pen",       "boat",	  "book",
	"corpse",   "teleport",	    "timer",	  "vehicle",   "ship",	  "switch",
	"quiver",   "lockpick",	    "instrument", "totem",     "storage", "scabbard",
	"shield",   "bandage",	    "spawner",	  "herb",      "pipe",	  0
};

#define TH (ITEM_TAKE | ITEM_HOLD)

static struct
{
	const char *name;
	unsigned int slots;
} wear_names[] = {
	{ "ring", TH | ITEM_WEAR_FINGER },
	{ "necklace", TH | ITEM_WEAR_NECK },
	{ "armor", TH | ITEM_WEAR_BODY },
	{ "helmet", TH | ITEM_WEAR_HEAD },
	{ "pants", TH | ITEM_WEAR_LEGS },
	{ "boots", TH | ITEM_WEAR_FEET },
	{ "gloves", TH | ITEM_WEAR_HANDS },
	{ "sleeves", TH | ITEM_WEAR_ARMS },
	{ "shield", TH | ITEM_WEAR_SHIELD },
	{ "cloak", TH | ITEM_WEAR_ABOUT },
	{ "belt", TH | ITEM_WEAR_WAIST },
	{ "bracelet", TH | ITEM_WEAR_WRIST },
	{ "glasses", TH | ITEM_WEAR_EYES },
	{ "mask", TH | ITEM_WEAR_FACE },
	{ "earring", TH | ITEM_WEAR_EARRING },
	{ "quiver", TH | ITEM_WEAR_QUIVER },
	{ "badge", TH | ITEM_GUILD_INSIGNIA },
	{ "backpack", TH | ITEM_WEAR_BACK },
	{ "saddle", TH | ITEM_HORSE_BODY },
	{ "tail ring", TH | ITEM_WEAR_TAIL },
	{ "nose ring", TH | ITEM_WEAR_NOSE },
	{ "horns", TH | ITEM_WEAR_HORN },
	{ "ioun", TH | ITEM_WEAR_IOUN },
	{ "arachnid robes", TH | ITEM_SPIDER_BODY },

	{ "bra", TH | ITEM_WEAR_BODY | ITEM_WEAR_HEAD },
	{ "cubes", TH | ITEM_WEAR_FEET | ITEM_WEAR_HANDS },
	{ "tubes", TH | ITEM_WEAR_LEGS | ITEM_WEAR_ARMS },
	{ "shawl", TH | ITEM_WEAR_NECK | ITEM_WEAR_ABOUT },
	{ "robe", TH | ITEM_WEAR_BODY | ITEM_WEAR_ABOUT },
	{ "rope", TH | ITEM_WEAR_NECK | ITEM_WEAR_WAIST },
	{ "charm", TH | ITEM_WEAR_NECK | ITEM_WEAR_WRIST },
	{ "bandana", TH | ITEM_WEAR_NECK | ITEM_WEAR_HEAD | ITEM_WEAR_WRIST },
	{ "eye", TH | ITEM_WEAR_EYES | ITEM_GUILD_INSIGNIA },
	{ "shield", TH | ITEM_WEAR_SHIELD | ITEM_WEAR_BACK },
	{ "pack", TH | ITEM_WEAR_ABOUT | ITEM_WEAR_BACK },
	{ "ring", TH | ITEM_WEAR_FINGER | ITEM_ATTACH_BELT },
	{ "amulet", TH | ITEM_WEAR_NECK | ITEM_ATTACH_BELT },
	{ "wreath", TH | ITEM_WEAR_HEAD | ITEM_ATTACH_BELT },
	{ "smallshield", TH | ITEM_WEAR_SHIELD | ITEM_ATTACH_BELT },
	{ "shirt", TH | ITEM_WEAR_ABOUT | ITEM_ATTACH_BELT },
	{ "feather", TH | ITEM_WEAR_EARRING | ITEM_ATTACH_BELT },
	{ "sigil", TH | ITEM_GUILD_INSIGNIA | ITEM_ATTACH_BELT },
	{ "shard", TH | ITEM_WIELD | ITEM_GUILD_INSIGNIA | ITEM_ATTACH_BELT },
	{ "sigil", TH | ITEM_WEAR_QUIVER | ITEM_GUILD_INSIGNIA | ITEM_ATTACH_BELT },
	{ "braid",
	  TH | ITEM_WEAR_WAIST | ITEM_WEAR_QUIVER | ITEM_GUILD_INSIGNIA | ITEM_ATTACH_BELT },
	{ "sack", TH | ITEM_WEAR_BACK | ITEM_ATTACH_BELT },
	{ "baldric", TH | ITEM_WEAR_QUIVER | ITEM_WEAR_BACK | ITEM_ATTACH_BELT },
	{ "blanket", TH | ITEM_WEAR_ABOUT | ITEM_HORSE_BODY },
	{ "backpack", TH | ITEM_WEAR_BACK | ITEM_HORSE_BODY },
	{ "rucksack", TH | ITEM_WEAR_BACK | ITEM_ATTACH_BELT | ITEM_HORSE_BODY },
	{ "ring", TH | ITEM_WEAR_FINGER | ITEM_WEAR_TAIL },
	{ "tangle", TH | ITEM_WEAR_ABOUT | ITEM_WEAR_TAIL },
	{ "circle", TH | ITEM_WEAR_FINGER | ITEM_WEAR_EARRING | ITEM_WEAR_TAIL },
	{ "hoop", TH | ITEM_WEAR_EARRING | ITEM_WEAR_NOSE },
	{ "wad", TH | ITEM_WEAR_EYES | ITEM_WEAR_FACE | ITEM_WEAR_EARRING | ITEM_WEAR_NOSE },
	{ "circle", TH | ITEM_WEAR_FINGER | ITEM_WEAR_TAIL | ITEM_WEAR_NOSE },
	{ "hairpin", TH | ITEM_WEAR_HEAD | ITEM_WEAR_TAIL | ITEM_WEAR_NOSE },
	{ "hoop", TH | ITEM_WEAR_FINGER | ITEM_WEAR_EARRING | ITEM_WEAR_TAIL | ITEM_WEAR_NOSE },
	{ "wreath", TH | ITEM_WEAR_NECK | ITEM_WEAR_WRIST | ITEM_HORSE_BODY | ITEM_WEAR_HORN },
	{ "wreath", TH | ITEM_WEAR_NECK | ITEM_WEAR_HEAD | ITEM_WEAR_WAIST | ITEM_WEAR_TAIL |
			    ITEM_WEAR_HORN },
	{ "ribbon", TH | ITEM_GUILD_INSIGNIA | ITEM_ATTACH_BELT | ITEM_WEAR_TAIL | ITEM_WEAR_HORN },
	{ "sheen", TH | ITEM_WEAR_NECK | ITEM_WEAR_LEGS | ITEM_WEAR_ARMS | ITEM_WEAR_WRIST |
			   ITEM_WEAR_EYES | ITEM_HORSE_BODY | ITEM_WEAR_TAIL | ITEM_WEAR_HORN },
	{ "sinew", TH | ITEM_WEAR_NECK | ITEM_WEAR_HEAD | ITEM_WEAR_WAIST | ITEM_WEAR_WRIST |
			   ITEM_ATTACH_BELT | ITEM_HORSE_BODY | ITEM_WEAR_TAIL | ITEM_WEAR_HORN },
	{ "loop", TH | ITEM_WEAR_WRIST | ITEM_WEAR_NOSE | ITEM_WEAR_HORN },
	{ "ringlets", TH | ITEM_WEAR_FINGER | ITEM_WEAR_TAIL | ITEM_WEAR_NOSE | ITEM_WEAR_HORN },
	{ "star", TH | ITEM_GUILD_INSIGNIA | ITEM_WEAR_IOUN },
	{ "barding", TH | ITEM_HORSE_BODY | ITEM_SPIDER_BODY },
	{ 0, 0 }
};

const char *guess_item_type(P_obj obj)
{
	if (obj->type == ITEM_WEAPON && (obj->value[0] > 0 && obj->value[0] <= WEAPON_HIGHEST))
		return weapon_types[obj->value[0]].flagLong;

	unsigned int worn = obj->wear_flags;
	for (auto *i = wear_names; i->name; ++i)
		if (i->slots == worn)
			return i->name;

	return type_names[obj->type];
}

static const char *encrust_gradients[] = {
	"bBWBb",  "gGWGg",  "cCWCc",	 "rRWRr",   "mMWMm",   "yYWYy", "wWWWw",
	"LbBbL",  "LgGgL",  "LcCcL",	 "LrRrL",   "LmMmL",   "LyYyL", "LwWwL",
	"rygcbm", "RYGCBM", "LryYWYyrL", "LgGYGgL", "ryryryr", "wLyLw",
};

void describe_encrusted_enhanced(P_obj obj)
{
	char typebuf[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH], name[256];

	const char *modstring = modenhance_names[obj->affected[2].location];
	if (!modstring)
		modstring = "&+wof &=rYbugginess&n";

	// the same id fpr the same combination of object and encrustment, but without
	// taking enhancement into the hash.
	uint64_t id = hash64(OBJ_VNUM(obj)) ^ hash64(obj->bitvector) ^ hash64(obj->bitvector2);

	get_name(name, 9, id);

	const char *type = guess_item_type(obj);
	snprintf(typebuf, sizeof typebuf, "encrusted %s", type);
	AnsiString typedesc(typebuf);
	typedesc.colorize(Gradient(encrust_gradients[id % ARRAY_SIZE(encrust_gradients)]));
	typedesc.ansi(typebuf);

	snprintf(buf, sizeof buf, "%s %s encrusted encrust enhanced", type, name);
	set_keywords(obj, buf);
	checked_snprintf(buf, sizeof buf, "%s '%s' %s&n", typebuf, name, modstring);
	set_short_description(obj, buf);
}
