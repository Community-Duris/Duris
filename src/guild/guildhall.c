/*
 *  guildhalls.c
 *  Duris
 *
 *  Created by Torgal on 1/29/10.
 *
 */

#include "prototypes.h"
#include "db.h"
#include "utility.h"
#include "utils.h"
#include "guild/guildhall.h"
#include "guild/assocs.h"
#include "guild/guildhall_db.h"
#include "specs.prototypes.h"
#include "magic/spells.h"

// global variables

extern P_room world;
extern P_index obj_index;
extern P_index mob_index;

vector<Guildhall *> Guildhall::guildhalls;

void Guildhall::initialize()
{
	obj_index[real_object0(GH_DOOR_VNUM)].func.obj = guildhall_door;
	obj_index[real_object0(GH_WINDOW_VNUM)].func.obj = guildhall_window;
	obj_index[real_object0(GH_HEARTSTONE_VNUM)].func.obj = guildhall_heartstone;
	obj_index[real_object0(GH_CARGO_BOARD_VNUM)].func.obj = guildhall_cargo_board;
	mob_index[real_mobile0(GH_GOLEM_WARRIOR)].func.mob = guildhall_golem;
	mob_index[real_mobile0(GH_GOLEM_CLERIC)].func.mob = guildhall_golem;
	mob_index[real_mobile0(GH_GOLEM_SORCERER)].func.mob = guildhall_golem;

	/* Commented this out and moved golem_noflee code into guildhall_golem.
	 * GH_GOLEM_* is the vnums 4800(0-2) defined just above this.
	  // added 020515 Gellz for golem gh blocks
	  mob_index[real_mobile0(48000)].func.mob = golem_noflee;
	  mob_index[real_mobile0(48001)].func.mob = golem_noflee;
	  mob_index[real_mobile0(48002)].func.mob = golem_noflee;
	*/

	// load guildhalls from DB
	load_guildhalls(guildhalls);

	for (size_t i = 0; i < guildhalls.size(); i++)
	{
		load_guildhall_rooms(guildhalls[i]);
		if (!guildhalls[i]->guild)
		{
			logit(LOG_GUILDHALLS,
			      "Guildhall::initialize(): guildhall %d has no guild (assoc_id %d)",
			      guildhalls[i]->id, guildhalls[i]->assoc_id);
			continue;
		}
		guildhalls[i]->init();
	}
}

void Guildhall::shutdown()
{
	for (size_t i = 0; i < guildhalls.size(); i++)
	{
		guildhalls[i]->save();
	}

	for (size_t i = 0; i < guildhalls.size(); i++)
	{
		delete (guildhalls[i]);
	}
}

void Guildhall::add(Guildhall *gh)
{
	if (!gh)
		return;

	guildhalls.push_back(gh);
}

bool Guildhall::reload()
{
	if (!deinit())
		return FALSE;

	clear_rooms();
	load_guildhall(id, this);
	load_guildhall_rooms(this);
	guild = get_guild_from_id(assoc_id);

	return this->init();
}

void Guildhall::remove(Guildhall *gh)
{
	if (!gh)
		return;

	gh->deinit();
	gh->destroy();

	for (vector<Guildhall *>::iterator it = guildhalls.begin(); it != guildhalls.end(); it++)
	{
		if ((*it) == gh)
		{
			guildhalls.erase(it);
			break;
		}
	}

	delete (gh);
}

//
// finds the Guildhall with id
//
Guildhall *Guildhall::find_by_id(int id)
{
	for (size_t i = 0; i < guildhalls.size(); i++)
	{
		if (guildhalls[i] && guildhalls[i]->id == id)
		{
			// debug("returning %d from find_by_id", i);
			return guildhalls[i];
		}
	}
	// debug("failed to return a guildhall in guildhall.c find_by_id");
	return NULL;
}

Guildhall *Guildhall::find_by_assoc_id(int id)
{
	if (id < 0)
		return NULL;

	for (size_t i = 0; i < guildhalls.size(); i++)
	{
		if (guildhalls[i] &&
		    guildhalls[i]->guild->get_id() == static_cast<unsigned int>(id))
		{
			return guildhalls[i];
		}
	}
	// debug("failed to return a guildhall in guildhall.c find_by_assoc_id");
	return NULL;
}

//
//  finds the Guildhall that ch is currently inside of
//
Guildhall *Guildhall::find_by_vnum(int vnum)
{
	for (size_t i = 0; i < guildhalls.size(); i++)
	{
		if (!guildhalls[i])
			continue;

		for (size_t j = 0; j < guildhalls[i]->rooms.size(); j++)
		{
			if (!guildhalls[i]->rooms[j])
				continue;

			if (guildhalls[i]->rooms[j]->vnum == vnum)
				return guildhalls[i];
		}
	}
	return NULL;
}

//
// finds the Guildhall by the outside vnum
//
Guildhall *Guildhall::find_by_outside_vnum(int vnum)
{
	for (size_t i = 0; i < guildhalls.size(); i++)
	{
		if (guildhalls[i] && guildhalls[i]->outside_vnum == vnum)
			return guildhalls[i];
	}
	return NULL;
}

//
// finds Guildhall by the ch's guildhall tag
//
Guildhall *Guildhall::find_from_ch(P_char ch)
{
	for (struct affected_type *afp = ch->affected; afp; afp = afp->next)
	{
		if (afp->type == TAG_GUILDHALL)
		{
			return Guildhall::find_by_id(afp->modifier);
		}
	}
	return NULL;
}

//
// finds GuildhallRoom with id
//
GuildhallRoom *Guildhall::find_room_by_id(int id)
{
	for (size_t i = 0; i < guildhalls.size(); i++)
	{
		if (!guildhalls[i])
			continue;

		for (size_t j = 0; j < guildhalls[i]->rooms.size(); j++)
		{
			if (guildhalls[i]->rooms[j] && guildhalls[i]->rooms[j]->id == id)
				return guildhalls[i]->rooms[j];
		}
	}
	return NULL;
}

//
//  finds GuildhallRoom from current vnum
//
GuildhallRoom *Guildhall::find_room_by_vnum(int vnum)
{
	for (size_t i = 0; i < guildhalls.size(); i++)
	{
		if (!guildhalls[i])
			continue;

		for (size_t j = 0; j < guildhalls[i]->rooms.size(); j++)
		{
			if (guildhalls[i]->rooms[j] && guildhalls[i]->rooms[j]->vnum == vnum)
				return guildhalls[i]->rooms[j];
		}
	}
	return NULL;
}

//
// finds a library room from vnum, if any
//
LibraryRoom *Guildhall::find_library_by_vnum(int vnum)
{
	for (size_t i = 0; i < guildhalls.size(); i++)
	{
		if (!guildhalls[i])
			continue;

		for (size_t j = 0; j < guildhalls[i]->rooms.size(); j++)
		{
			if (guildhalls[i]->rooms[j] &&
			    guildhalls[i]->rooms[j]->type == GH_ROOM_TYPE_LIBRARY &&
			    guildhalls[i]->rooms[j]->vnum == vnum)
				return (LibraryRoom *)guildhalls[i]->rooms[j];
		}
	}
	return NULL;
}

//
// returns the count by association id of a certain GH type
//
int Guildhall::count_by_assoc_id(int assoc_id, int type)
{
	if (assoc_id < 0)
		return 0;

	int count = 0;

	for (size_t i = 0; i < guildhalls.size(); i++)
	{
		if (!guildhalls[i])
			continue;

		if (guildhalls[i]->guild->get_id() == static_cast<unsigned int>(assoc_id) &&
		    guildhalls[i]->type == type)
			count++;
	}
	return count;
}

//
// Guildhall methods
//
//

bool Guildhall::save()
{
	if (!this->valid())
	{
		logit(LOG_GUILDHALLS, "Guildhall::save(%d): invalid!", this->id);
		return FALSE;
	}

	if (!save_guildhall(this))
	{
		logit(LOG_GUILDHALLS, "Guildhall::save(%d): save_guildhall failed!", this->id);
		return FALSE;
	}

	// The flat authority stores the hall and its rooms in the save above.
#ifndef __NO_MYSQL__
	for (size_t i = 0; i < this->rooms.size(); i++)
	{
		this->rooms[i]->save();
	}
#endif

	return TRUE;
}

bool Guildhall::destroy()
{
	if (!delete_guildhall(this))
	{
		logit(LOG_GUILDHALLS, "Guildhall::destroy(%d): delete_guildhall failed!", this->id);
		return FALSE;
	}

	// The flat authority removes the hall and all nested rooms in the erase above.
#ifndef __NO_MYSQL__
	for (size_t i = 0; i < this->rooms.size(); i++)
	{
		this->rooms[i]->destroy();
	}
#endif

	return TRUE;
}

bool Guildhall::can_add_room()
{
	return (this->rooms.size() + 1 <= this->max_rooms);
}

void Guildhall::add_room(GuildhallRoom *room)
{
	if (!room)
	{
		logit(LOG_GUILDHALLS, "Guildhall::add_room(): invalid room!");
		return;
	}

	room->assoc_id = this->assoc_id;
	room->guild = this->guild;
	room->guildhall = this;
	this->rooms.push_back(room);
}

bool Guildhall::init()
{
	// guildhall initialization
	// TODO: set up upkeep events

	// initialize rooms
	for (size_t i = 0; i < this->rooms.size(); i++)
	{
		this->rooms[i]->assoc_id = this->assoc_id;
		this->rooms[i]->guild = this->guild;

		if (!this->rooms[i]->guild)
		{
			logit(LOG_GUILDHALLS, "Guildhall::init(%d): room %d has no guild!",
			      this->id, this->rooms[i]->id);
			return FALSE;
		}

		if (!this->rooms[i]->init())
		{
			logit(LOG_GUILDHALLS, "Guildhall::init(%d): room %d init failed!", this->id,
			      this->rooms[i]->id);
			return FALSE;
		}
	}

	return TRUE;
}

bool Guildhall::deinit()
{
	// guildhall deinitialization
	// TODO: remove upkeep events

	// deinitialize rooms
	for (size_t i = 0; i < this->rooms.size(); i++)
	{
		if (!this->rooms[i]->deinit())
		{
			logit(LOG_GUILDHALLS, "Guildhall::deinit(%d): room %d deinit failed!",
			      this->id, this->rooms[i]->id);
			return FALSE;
		}
	}

	return TRUE;
}

bool Guildhall::valid()
{
	// initialize *rooms array and run init on each room
	for (size_t i = 0; i < this->rooms.size(); i++)
	{
		GuildhallRoom *room = this->rooms[i];

		if (!room)
		{
			logit(LOG_GUILDHALLS, "Guildhall::valid(%d): invalid room pointer!",
			      this->id);
			return FALSE;
		}

		if (!room->valid())
		{
			logit(LOG_GUILDHALLS, "Guildhall::valid(%d): invalid room! (%d)", this->id,
			      room->id);
			return FALSE;
		}
	}

	if (this->rooms.size() > this->max_rooms)
	{
		logit(LOG_GUILDHALLS, "Guildhall::init(%d): too many rooms! (%zu, max %d)",
		      this->id, this->rooms.size(), this->max_rooms);
		return FALSE;
	}

	return TRUE;
}

void Guildhall::golem_died(P_char golem)
{
	if (!this->entrance_room)
		return;

	if (this->entrance_room->golem_died(golem))
	{
		this->save();
	}
}
Guildhall *find_gh_from_vnum(int room)
{
	Guildhall *gh = Guildhall::find_by_vnum(room);
	return gh;
}
