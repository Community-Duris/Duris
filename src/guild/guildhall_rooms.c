/*
 *  guildhall_rooms.c
 *  Duris
 *
 *  Created by Torgal on 1/31/10.
 *
 */

#include "core/prototypes.h"
#include "world/db.h"
#include "core/utility.h"
#include "core/utils.h"
#include <math.h>
#include "guild/assocs.h"
#include "guild/guildhall.h"
#include "guild/guildhall_db.h"
#include "world/map.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"

// external global variables
extern P_room world;
extern P_index obj_index;
extern const int rev_dir[];

//
// GuildhallRoom methods
//

bool GuildhallRoom::save()
{
	if (!this->valid())
	{
		logit(LOG_GUILDHALLS, "GuildhallRoom::save(%d): invalid!", this->id);
		return FALSE;
	}

	if (!save_guildhall_room(this))
	{
		logit(LOG_GUILDHALLS, "GuildhallRoom::save(%d): save_guildhall_room failed!",
		      this->id);
		return FALSE;
	}

	return TRUE;
}

bool GuildhallRoom::destroy()
{
	if (!delete_guildhall_room(this))
	{
		logit(LOG_GUILDHALLS, "GuildhallRoom::destroy(%d): delete_guildhall_room failed!",
		      this->id);
		return FALSE;
	}

	return TRUE;
}

bool GuildhallRoom::valid()
{
	if (!this->guildhall)
	{
		logit(LOG_GUILDHALLS, "GuildhallRoom::valid(%d): no guildhall pointer!", this->id);
		return FALSE;
	}

	if (this->type < 0 || this->type >= GH_ROOM_NUM_TYPES)
	{
		logit(LOG_GUILDHALLS, "GuildhallRoom::valid(%d): room with an illegal type (%d)",
		      this->id, this->type);
		return FALSE;
	}

	// make sure vnum is valid
	if (this->vnum < 0 || !real_room0(this->vnum))
	{
		logit(LOG_GUILDHALLS, "GuildhallRoom::valid(%d): invalid vnum! (%d)", this->id,
		      this->vnum);
		return FALSE;
	}

	// check to make sure there arent any other rooms already initialized with this vnum
	for (size_t i = 0; i < Guildhall::guildhalls.size(); i++)
	{
		for (size_t j = 0; j < Guildhall::guildhalls[i]->rooms.size(); j++)
		{
			if (this->id != Guildhall::guildhalls[i]->rooms[j]->id &&
			    this->vnum == Guildhall::guildhalls[i]->rooms[j]->vnum)
			{
				logit(LOG_GUILDHALLS,
				      "GuildhallRoom::valid(%d): vnum [%d] already initialized (by guildhall %d / room %d)!",
				      this->id, this->vnum, Guildhall::guildhalls[i]->id,
				      Guildhall::guildhalls[i]->rooms[j]->id);
				return FALSE;
			}
		}
	}

	return TRUE;
}

bool GuildhallRoom::init()
{
	if (!this->valid())
	{
		logit(LOG_GUILDHALLS, "GuildhallRoom::init(%d): invalid!", this->id);
		return FALSE;
	}

	this->room = &world[real_room0(this->vnum)];

	SET_BIT(this->room->room_flags, ROOM_GUILD);
	SET_BIT(this->room->room_flags, ROOM_NO_MOB);
	SET_BIT(this->room->room_flags, ROOM_NO_TRACK);
	SET_BIT(this->room->room_flags, ROOM_NO_TELEPORT);
	SET_BIT(this->room->room_flags, ROOM_NO_RECALL);
	SET_BIT(this->room->room_flags, ROOM_NO_SUMMON);
	SET_BIT(this->room->room_flags, ROOM_NO_GATE);
	SET_BIT(this->room->room_flags, ROOM_TWILIGHT);

	// set up room exits
	for (int dir = 0; dir < NUM_EXITS; dir++)
	{
		if (this->has_exit(dir) && real_room0(this->exits[dir]))
			connect_rooms(this->vnum, this->exits[dir], dir, -1);
	}

	// set up name
	if (this->name.length())
	{
		this->room->name = str_dup(this->name.c_str());
	}
	else
	{
		char buff[MAX_STRING_LENGTH];
		checked_substitute(buff, MAX_STRING_LENGTH,
				   world[real_room0(this->template_vnum)].name,
				   this->guild->get_name().c_str());
		this->room->name = str_dup(buff);
	}

	return TRUE;
}

bool GuildhallRoom::deinit()
{
	if (this->room)
	{
		// TODO: make sure removing ROOM_GUILD doesnt mess up enter_game calculations, etc
		REMOVE_BIT(this->room->room_flags, ROOM_GUILD);
		REMOVE_BIT(this->room->room_flags, ROOM_NO_MOB);
		REMOVE_BIT(this->room->room_flags, ROOM_NO_TRACK);
		REMOVE_BIT(this->room->room_flags, ROOM_NO_TELEPORT);
		REMOVE_BIT(this->room->room_flags, ROOM_NO_RECALL);
		REMOVE_BIT(this->room->room_flags, ROOM_NO_SUMMON);
		REMOVE_BIT(this->room->room_flags, ROOM_NO_GATE);
		REMOVE_BIT(this->room->room_flags, ROOM_TWILIGHT);
	}

	// set up room exits
	for (int dir = 0; dir < NUM_EXITS; dir++)
	{
		disconnect_rooms(this->vnum, this->exits[dir]);
	}

	return TRUE;
}

//
// init methods for specific room types
//

bool EntranceRoom::init()
{
	GuildhallRoom::init();

	this->guildhall->entrance_room = this;

	// connect entrance room to outside room
	connect_rooms(this->vnum, this->guildhall->outside_vnum, this->value[GH_VALUE_ENTRANCE_DIR],
		      -1);

	// load door object
	if ((this->door = read_object(GH_DOOR_VNUM, VIRTUAL)))
	{
		char buff[MAX_STRING_LENGTH];
		this->door->str_mask = STRUNG_DESC1;
		checked_substitute(buff, MAX_STRING_LENGTH, this->door->description,
				   this->guild->get_name().c_str());
		this->door->description = str_dup(buff);
		this->door->value[0] = this->vnum;
		obj_to_room(this->door, real_room0(this->guildhall->outside_vnum));
	}

	// load golems
	for (int i = 0; i < GH_GOLEM_NUM_SLOTS; i++)
	{
		int vnum = 0;
		switch (this->value[GH_GOLEM_SLOT + i])
		{
		case GH_GOLEM_TYPE_WARRIOR:
			vnum = GH_GOLEM_WARRIOR;
			break;
		case GH_GOLEM_TYPE_CLERIC:
			vnum = GH_GOLEM_CLERIC;
			break;
		case GH_GOLEM_TYPE_SORCERER:
			vnum = GH_GOLEM_SORCERER;
			break;
		default:
			// no golem
			break;
		}

		if (vnum)
		{
			P_char golem = read_mobile(vnum, VIRTUAL);

			if (!golem)
			{
				logit(LOG_MOB,
				      "EntranceRoom::init(): couldn't load guildhall golem mob %d!",
				      vnum);
				continue;
			}

			SET_BIT(golem->specials.act, ACT_SPEC);
			SET_BIT(golem->specials.act, ACT_SPEC_DIE);

			GET_PLATINUM(golem) = 0;
			GET_GOLD(golem) = 0;
			GET_SILVER(golem) = 0;
			GET_COPPER(golem) = 0;

			GET_HOME(golem) = GET_BIRTHPLACE(golem) = GET_ORIG_BIRTHPLACE(golem) =
				this->vnum;

			GET_ASSOC(golem) = this->guild;
			if (this->guildhall->racewar == RACEWAR_GOOD)
			{
				SET_BIT(golem->only.npc->aggro_flags, AGGR_EVIL_RACE);
			}
			else if (this->guildhall->racewar == RACEWAR_EVIL)
			{
				SET_BIT(golem->only.npc->aggro_flags, AGGR_GOOD_RACE);
			}
			add_tag_to_char(golem, TAG_GUILDHALL, this->guildhall->id, AFFTYPE_PERM);
			add_tag_to_char(golem, TAG_DIRECTION,
					rev_dir[this->value[GH_VALUE_ENTRANCE_DIR]], AFFTYPE_PERM);
			golem->player.racewar = guildhall->racewar;

			char_to_room(golem, real_room0(this->vnum), -1);

			this->golems[i] = golem;
		}
	}

	// TODO: set up gate opposite 'exit'
	// TODO: set up room proc, "look out" or "look south"

	return TRUE;
}

bool EntranceRoom::golem_died(P_char golem)
{
	bool changed = false;
	for (int i = 0; i < GH_GOLEM_NUM_SLOTS; i++)
	{
		if (golem == this->golems[i])
		{
			this->value[GH_GOLEM_SLOT + i] = 0;
			this->golems[i] = NULL;
			changed = true;
		}
	}

	return changed;
}

bool InnRoom::init()
{
	GuildhallRoom::init();

	this->guildhall->inn_room = this;

	// set inn proc
	world[real_room0(this->vnum)].funct = inn;

	// set up guild board
	if ((this->board = read_object(GH_BOARD_START + this->guild->get_id(), VIRTUAL)))
	{
		obj_to_room(board, real_room0(this->vnum));
	}

	return TRUE;
}

bool HeartstoneRoom::init()
{
	GuildhallRoom::init();

	this->guildhall->heartstone_room = this;

	// load heartstone object
	if ((this->heartstone = read_object(GH_HEARTSTONE_VNUM, VIRTUAL)))
	{
		this->heartstone->value[0] = this->guildhall->id;
		obj_to_room(this->heartstone, real_room0(this->vnum));
	}

	return TRUE;
}

bool PortalRoom::init()
{
	GuildhallRoom::init();

	this->guildhall->portal_room = this;

	// TODO: initialize portals to outposts
	return TRUE;
}

bool WindowRoom::init()
{
	GuildhallRoom::init();

	// load window object
	if ((this->window = read_object(GH_WINDOW_VNUM, VIRTUAL)))
	{
		this->window->value[0] = this->guildhall->outside_vnum;
		obj_to_room(this->window, real_room0(this->vnum));
	}

	return TRUE;
}

bool HealRoom::init()
{
	GuildhallRoom::init();

	SET_BIT(this->room->room_flags, ROOM_HEAL);

	// load fountain object
	if ((this->fountain = read_object(GH_FOUNTAIN_VNUM, VIRTUAL)))
	{
		obj_to_room(this->fountain, real_room0(this->vnum));
	}

	return TRUE;
}

bool BankRoom::init()
{
	GuildhallRoom::init();

	// set room proc
	world[real_room0(this->vnum)].funct = guildhall_bank_room;

	// set up bank counter
	if ((this->counter = read_object(GH_BANK_COUNTER_VNUM, VIRTUAL)))
	{
		obj_to_room(this->counter, real_room0(this->vnum));
	}

	return TRUE;
}

bool TownPortalRoom::init()
{
	GuildhallRoom::init();

	// set up town portal
	if ((this->portal = read_object(GH_TOWN_PORTAL_VNUM, VIRTUAL)))
	{
		if (this->guildhall->racewar == RACEWAR_GOOD)
		{
			int town[] = { TH_MAP_VNUM, KI_MAP_VNUM, WS_MAP_VNUM };
			int inn[] = { TH_INN_VNUM, KI_INN_VNUM, WS_INN_VNUM };
			int whichtown[3];
			int dest = 0;
			int dist = 0;
			int i;

			for (i = 0; i < 3; i++)
				whichtown[i] = 0;

			for (i = 0; i < 3; i++)
			{
				whichtown[i] = calculate_map_distance(
					real_room(this->guildhall->outside_vnum),
					real_room(town[i]));
				if (whichtown[i])
					whichtown[i] = (int)sqrt(whichtown[i]);
				if (dist < whichtown[i] || dist == 0)
				{
					dist = whichtown[i];
					dest = inn[i];
				}
			}
			char buff[MAX_STRING_LENGTH];
			if (dest == WS_INN_VNUM)
			{
				checked_substitute(buff, MAX_STRING_LENGTH,
						   this->portal->description, "&+GWoodseer&n");
				this->portal->value[0] = WS_INN_VNUM;
			}
			else if (dest == KI_INN_VNUM)
			{
				checked_substitute(buff, MAX_STRING_LENGTH,
						   this->portal->description, "&+YKimordril&n");
				this->portal->value[0] = KI_INN_VNUM;
			}
			else
			{
				checked_substitute(buff, MAX_STRING_LENGTH,
						   this->portal->description, "&+cTharnadia&n");
				this->portal->value[0] = TH_INN_VNUM;
			}
			this->portal->description = str_dup(buff);
			this->portal->str_mask = STRUNG_DESC1;
			obj_to_room(this->portal, real_room0(this->vnum));
		}
		else if (this->guildhall->racewar == RACEWAR_EVIL)
		{
			char buff[MAX_STRING_LENGTH];
			if (IS_UD_MAP(real_room(this->guildhall->outside_vnum)))
			{
				checked_substitute(buff, MAX_STRING_LENGTH,
						   this->portal->description, "&+LShady Grove&n");
				this->portal->value[0] = SHADY_INN_VNUM;
			}
			else
			{
				checked_substitute(buff, MAX_STRING_LENGTH,
						   this->portal->description, "&+rKhildarak&n");
				this->portal->value[0] = KHILD_INN_VNUM;
			}
			this->portal->description = str_dup(buff);
			this->portal->str_mask = STRUNG_DESC1;
			obj_to_room(this->portal, real_room0(this->vnum));
		}
	}

	return TRUE;
}

bool LibraryRoom::init()
{
	GuildhallRoom::init();

	// set up tome
	if ((this->tome = read_object(GH_LIBRARY_TOME_VNUM, VIRTUAL)))
	{
		obj_to_room(this->tome, real_room0(this->vnum));
	}

	return TRUE;
}

bool CargoRoom::init()
{
	GuildhallRoom::init();

	// load window object
	if ((this->board = read_object(GH_CARGO_BOARD_VNUM, VIRTUAL)))
	{
		obj_to_room(this->board, real_room0(this->vnum));
	}

	return TRUE;
}

//
// Kingdom workshops and the guild store
//

bool WorkshopRoom::init()
{
	if (!GuildhallRoom::init())
		return FALSE;

	/* GuildhallRoom::init() took the NAME from the template; the description
	 * is new, since no other guildhall room has one. It is a COPY owned by
	 * this object, so nothing that later frees or replaces the room's
	 * description can reach the template's own string. */
	const int template_rnum = real_room(this->template_vnum);

	if (template_rnum >= 0 && world[template_rnum].description)
	{
		this->plain_description = this->room->description;
		this->workshop_description = str_dup(world[template_rnum].description);
		this->room->description = this->workshop_description;
	}

	if ((this->prop = read_object(this->prop_vnum, VIRTUAL)))
		obj_to_room(this->prop, real_room0(this->vnum));

	/* The store answers `list` and `buy`; the three workshops need no proc. */
	if (this->type == GH_ROOM_TYPE_GUILDSTORE)
		this->room->funct = guildhall_store_room;

	return TRUE;
}

//
//
//

bool WorkshopRoom::deinit()
{
	/* Only our own proc: a room handed back to the pool must not keep
	 * selling, and must not lose a proc something else put there. */
	if (this->room && this->room->funct == guildhall_store_room)
		this->room->funct = NULL;

	if (this->prop)
	{
		/* An immortal may have purged the prop since init(), so extract it
		 * only if it still stands here as the object it was. */
		const int rnum = real_room0(this->vnum);
		const int prop_rnum = real_object(this->prop_vnum);

		for (P_obj obj = world[rnum].contents; obj; obj = obj->next_content)
		{
			if (obj == this->prop && obj->R_num == prop_rnum)
			{
				extract_obj(obj);
				break;
			}
		}
		this->prop = NULL;
	}

	if (this->workshop_description)
	{
		/* Hand the room its own description back only while ours is still
		 * the one on it; anything that replaced it owns what it put there.
		 * The copy is ours either way, so it is always freed -- leaving it
		 * behind when something else had replaced it leaked it. */
		if (this->room && this->room->description == this->workshop_description)
			this->room->description = this->plain_description;
		str_free(this->workshop_description);
		this->workshop_description = NULL;
		this->plain_description = NULL;
	}

	/* The base DEINIT, not init(): every older room type below calls
	 * GuildhallRoom::init() from its deinit(), which re-flags the room and
	 * dups its name again on the way out. */
	return GuildhallRoom::deinit();
}

bool EntranceRoom::deinit()
{
	GuildhallRoom::init();

	this->guildhall->entrance_room = NULL;

	// connect entrance room to outside room
	disconnect_rooms(this->vnum, this->guildhall->outside_vnum);

	if (this->door)
	{
		obj_from_room(this->door);
		this->door = NULL;
	}

	// remove golems
	for (int i = 0; i < GH_GOLEM_NUM_SLOTS; i++)
	{
		if (this->golems[i])
		{
			extract_char(this->golems[i]);
			this->golems[i] = NULL;
		}
	}

	// TODO: remove gate to the north

	return TRUE;
}

bool InnRoom::deinit()
{
	GuildhallRoom::init();

	this->guildhall->inn_room = NULL;

	// remove inn proc
	world[real_room0(this->vnum)].funct = NULL;

	if (this->board)
	{
		obj_from_room(this->board);
		this->board = NULL;
	}

	return TRUE;
}

bool HeartstoneRoom::deinit()
{
	GuildhallRoom::init();

	this->guildhall->heartstone_room = NULL;

	// deinit heartstone obj
	if (this->heartstone)
	{
		obj_from_room(this->heartstone);
		this->heartstone = NULL;
	}

	return TRUE;
}

bool PortalRoom::deinit()
{
	GuildhallRoom::init();

	this->guildhall->portal_room = NULL;

	// TODO: deinitialize portals to outposts
	return TRUE;
}

bool WindowRoom::deinit()
{
	GuildhallRoom::init();

	// deinit window obj
	if (this->window)
	{
		obj_from_room(this->window);
		this->window = NULL;
	}

	return TRUE;
}

bool HealRoom::deinit()
{
	GuildhallRoom::init();

	REMOVE_BIT(this->room->room_flags, ROOM_HEAL);

	// deinit fountain obj
	if (this->fountain)
	{
		obj_from_room(this->fountain);
		this->fountain = NULL;
	}

	return TRUE;
}

bool BankRoom::deinit()
{
	GuildhallRoom::init();

	world[real_room0(this->vnum)].funct = NULL;

	// deinit counter obj
	if (this->counter)
	{
		obj_from_room(this->counter);
		this->counter = NULL;
	}

	return TRUE;
}

bool TownPortalRoom::deinit()
{
	GuildhallRoom::init();

	// deinit portal obj
	if (this->portal)
	{
		obj_from_room(this->portal);
		this->portal = NULL;
	}

	return TRUE;
}

bool LibraryRoom::deinit()
{
	GuildhallRoom::init();

	// deinit tome obj
	if (this->tome)
	{
		obj_from_room(this->tome);
		this->tome = NULL;
	}

	return TRUE;
}

bool CargoRoom::deinit()
{
	GuildhallRoom::init();

	// deinit board obj
	if (this->board)
	{
		obj_from_room(this->board);
		this->board = NULL;
	}

	return TRUE;
}
