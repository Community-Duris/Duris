/*
 *  guildhalls_db.c
 *  Duris
 *
 *  Created by Torgal on 1/30/10.
 *
 */

#include "guildhall_db.h"
#include <algorithm>
#include <new>
#include <string>
#include <utility>
#include <vector>
using namespace std;

#include "prototypes.h"
#include "utility.h"
#include "utils.h"
#include "assocs.h"
#include "guildhall.h"
#include "sql/sql.h"

#ifdef __NO_MYSQL__
#include "flatfile/flatfile_association_repository.h"
#include "persistence_mode.h"
#endif

extern vector<Guildhall *> guildhalls;
extern P_room world;

int _next_guildhall_id = -1;
int _next_guildhall_room_id = -1;
int _next_guildhall_room_vnum = -1;

#ifdef __NO_MYSQL__
namespace
{
static_assert(GH_ROOM_NUM_VALUES == FLATFILE_GUILDHALL_VALUE_COUNT);
static_assert(NUM_EXITS == FLATFILE_GUILDHALL_EXIT_COUNT);

bool flat_result_succeeded(flatfile_association_result result)
{
	return result == flatfile_association_result::ok ||
	       result == flatfile_association_result::unchanged;
}

bool capture_guildhall(const Guildhall *guildhall, flatfile_guildhall_record *record,
		       std::string *error)
{
	if (!guildhall || !guildhall->guild || !record)
	{
		if (error)
			*error = "guildhall or owning guild is unavailable";
		return false;
	}
	flatfile_guildhall_record captured;
	captured.guildhall_id = guildhall->id;
	captured.association_id = guildhall->guild->get_id();
	captured.type = guildhall->type;
	captured.outside_vnum = guildhall->outside_vnum;
	captured.racewar = guildhall->racewar;
	try
	{
		captured.rooms.reserve(guildhall->rooms.size());
		for (const auto *room : guildhall->rooms)
		{
			if (!room)
			{
				if (error)
					*error = "guildhall contains a missing room";
				return false;
			}
			flatfile_guildhall_room_record captured_room;
			captured_room.room_id = room->id;
			captured_room.vnum = room->vnum;
			captured_room.name = room->name;
			captured_room.type = room->type;
			for (size_t index = 0; index < captured_room.values.size(); ++index)
				captured_room.values[index] = room->value[index];
			for (size_t index = 0; index < captured_room.exits.size(); ++index)
				captured_room.exits[index] = room->exits[index];
			captured.rooms.push_back(std::move(captured_room));
		}
	}
	catch (const std::bad_alloc &)
	{
		if (error)
			*error = "could not allocate guildhall snapshot";
		return false;
	}
	*record = std::move(captured);
	return true;
}

GuildhallRoom *make_guildhall_room(int type)
{
	switch (type)
	{
	case GH_ROOM_TYPE_ENTRANCE:
		return new EntranceRoom();
	case GH_ROOM_TYPE_INN:
		return new InnRoom();
	case GH_ROOM_TYPE_HEARTSTONE:
		return new HeartstoneRoom();
	case GH_ROOM_TYPE_PORTAL:
		return new PortalRoom();
	case GH_ROOM_TYPE_WINDOW:
		return new WindowRoom();
	case GH_ROOM_TYPE_HEAL:
		return new HealRoom();
	case GH_ROOM_TYPE_BANK:
		return new BankRoom();
	case GH_ROOM_TYPE_TOWN_PORTAL:
		return new TownPortalRoom();
	case GH_ROOM_TYPE_LIBRARY:
		return new LibraryRoom();
	case GH_ROOM_TYPE_CARGO:
		return new CargoRoom();
	default:
		return new GuildhallRoom();
	}
}

void materialize_guildhall_room(const flatfile_guildhall_room_record &record, Guildhall *guildhall)
{
	GuildhallRoom *room = make_guildhall_room(record.type);
	room->id = record.room_id;
	room->vnum = record.vnum;
	room->name = record.name;
	room->type = record.type;
	for (size_t index = 0; index < record.values.size(); ++index)
		room->value[index] = record.values[index];
	for (size_t index = 0; index < record.exits.size(); ++index)
		room->exits[index] = record.exits[index];
	guildhall->add_room(room);
}
} // namespace
#endif

int next_guildhall_id()
{
#ifndef __NO_MYSQL__
	if (_next_guildhall_id == -1)
	{
		if (!qry("select coalesce(max(id), 0) from guildhalls"))
		{
			logit(LOG_GUILDHALLS, "next_guildhall_id(): query failed");
			return -1;
		}

		MYSQL_RES *res = mysql_store_result(DB);
		if (!res)
		{
			logit(LOG_DEBUG, "%s: mysql_store_result failed", __func__);
			return FALSE;
		}
		MYSQL_ROW row = mysql_fetch_row(res);

		if (!row[0])
		{
			_next_guildhall_id = 0;
		}
		else
		{
			_next_guildhall_id = atoi(row[0]);
		}
		mysql_free_result(res);
	}
#endif
	_next_guildhall_id++;

	return _next_guildhall_id;
}

int next_guildhall_room_id()
{
#ifndef __NO_MYSQL__
	if (_next_guildhall_room_id == -1)
	{
		if (!qry("select coalesce(max(id), 0) from guildhall_rooms"))
		{
			logit(LOG_GUILDHALLS, "next_guildhall_room_id(): query failed");
			return -1;
		}

		MYSQL_RES *res = mysql_store_result(DB);
		if (!res)
		{
			logit(LOG_DEBUG, "%s: mysql_store_result failed", __func__);
			return FALSE;
		}
		MYSQL_ROW row = mysql_fetch_row(res);

		if (!row[0])
		{
			_next_guildhall_room_id = 0;
		}
		else
		{
			_next_guildhall_room_id = atoi(row[0]);
		}
		mysql_free_result(res);
	}
#endif

	_next_guildhall_room_id++;

	return _next_guildhall_room_id;
}

// find the next available vnum in the GH rooms block.
// returns -1 if none are available
int next_guildhall_room_vnum()
{
	for (int vnum = GH_START_VNUM; vnum < GH_END_VNUM; vnum++)
	{
		// skip if vnum isn't valid room
		if (real_room(vnum) < 0)
			continue;

		// skip if vnum room is already set as guild room
		if (IS_ROOM(real_room0(vnum), ROOM_GUILD))
			continue;

		SET_BIT(world[real_room0(vnum)].room_flags, ROOM_GUILD);

		return vnum;
	}

	return -1;
}

void load_guildhalls(vector<Guildhall *> &guildhalls)
{
#ifdef __NO_MYSQL__
	const char *root = persistence_mode_flatfile_root();
	if (!root)
	{
		fatal_boot_error("guildhalls", "flat-file state root is unavailable");
		return;
	}
	std::string error;
	std::vector<flatfile_guildhall_record> records;
	const auto listed = flatfile_guildhall_list(root, &records, &error);
	if (listed == flatfile_association_result::not_found)
	{
		_next_guildhall_id = 0;
		_next_guildhall_room_id = 0;
		return;
	}
	if (listed != flatfile_association_result::ok)
	{
		fatal_boot_error("guildhalls", "could not load flat guildhall authority: %s",
				 error.c_str());
		return;
	}
	_next_guildhall_id = 0;
	_next_guildhall_room_id = 0;
	for (const auto &record : records)
	{
		P_Guild guild = get_guild_from_id(record.association_id);
		if (!guild)
		{
			fatal_boot_error("guildhalls",
					 "flat guildhall %d references missing guild %u",
					 record.guildhall_id, record.association_id);
			return;
		}
		Guildhall *guildhall = new Guildhall();
		guildhall->id = record.guildhall_id;
		guildhall->assoc_id = record.association_id;
		guildhall->guild = guild;
		guildhall->type = record.type;
		guildhall->outside_vnum = record.outside_vnum;
		guildhall->racewar = record.racewar;
		guildhalls.push_back(guildhall);
		_next_guildhall_id = std::max(_next_guildhall_id, record.guildhall_id);
		for (const auto &room : record.rooms)
			_next_guildhall_room_id = std::max(_next_guildhall_room_id, room.room_id);
	}
#else
	if (!qry("select id, assoc_id, type, outside_vnum, racewar from guildhalls order by id asc"))
	{
		logit(LOG_GUILDHALLS, "load_guildhalls(): query failed");
		return;
	}

	MYSQL_RES *res = mysql_store_result(DB);
	if (!res)
	{
		logit(LOG_DEBUG, "%s: mysql_store_result failed", __func__);
		return;
	}

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(res)))
	{
		int id = atoi(row[0]);
		int assoc_id = atoi(row[1]);
		P_Guild guild = get_guild_from_id(assoc_id);

		if (!guild)
		{
			logit(LOG_GUILDHALLS,
			      "load_guildhalls(): skipping guildhall %d - guild %d not found", id,
			      assoc_id);
			continue;
		}

		Guildhall *gh = new Guildhall();
		gh->id = id;
		gh->assoc_id = assoc_id;
		gh->guild = guild;
		gh->type = atoi(row[2]);
		gh->outside_vnum = atoi(row[3]);
		gh->racewar = atoi(row[4]);

		guildhalls.push_back(gh);
	}

	mysql_free_result(res);
#endif
}

void load_guildhall(int id, Guildhall *gh)
{
	if (!gh)
	{
		logit(LOG_GUILDHALLS, "load_guildhall(): invalid gh");
		return;
	}

	if (gh->id <= 0)
	{
		logit(LOG_GUILDHALLS, "load_guildhall(%d): invalid id", id);
		return;
	}

#ifdef __NO_MYSQL__
	const char *root = persistence_mode_flatfile_root();
	if (!root)
	{
		logit(LOG_GUILDHALLS, "load_guildhall(%d): flat-file state root unavailable", id);
		return;
	}
	std::string error;
	std::vector<flatfile_guildhall_record> records;
	const auto listed = flatfile_guildhall_list(root, &records, &error);
	if (listed != flatfile_association_result::ok)
	{
		logit(LOG_GUILDHALLS, "load_guildhall(%d): %s", id, error.c_str());
		return;
	}
	const auto record = std::find_if(records.begin(), records.end(), [id](const auto &entry)
					 { return entry.guildhall_id == id; });
	if (record == records.end())
	{
		logit(LOG_GUILDHALLS, "load_guildhall(%d): guildhall not found", id);
		return;
	}
	gh->id = record->guildhall_id;
	gh->assoc_id = record->association_id;
	gh->guild = get_guild_from_id(record->association_id);
	gh->type = record->type;
	gh->outside_vnum = record->outside_vnum;
	gh->racewar = record->racewar;
#else
	if (!qry("select id, assoc_id, type, outside_vnum, racewar from guildhalls where id = %d",
		 id))
	{
		logit(LOG_GUILDHALLS, "load_guildhall(%d): query failed", id);
		return;
	}

	MYSQL_RES *res = mysql_store_result(DB);
	if (!res)
	{
		logit(LOG_DEBUG, "%s: mysql_store_result failed", __func__);
		return;
	}

	MYSQL_ROW row;

	if (!(row = mysql_fetch_row(res)))
	{
		logit(LOG_GUILDHALLS, "load_guildhall(%d): guildhall with id not found!", id);
		mysql_free_result(res);
		return;
	}

	gh->id = atoi(row[0]);
	gh->assoc_id = atoi(row[1]);
	gh->guild = get_guild_from_id(gh->assoc_id);
	gh->type = atoi(row[2]);
	gh->outside_vnum = atoi(row[3]);
	gh->racewar = atoi(row[4]);

	mysql_free_result(res);
#endif
}

void load_guildhall_rooms(Guildhall *guildhall)
{
	if (!guildhall)
	{
		logit(LOG_GUILDHALLS, "read_guildhall_rooms(): invalid guildhall!");
		return;
	}

	if (guildhall->id <= 0)
	{
		logit(LOG_GUILDHALLS, "read_guildhall_rooms(): invalid id (%d)", guildhall->id);
		return;
	}

#ifdef __NO_MYSQL__
	const char *root = persistence_mode_flatfile_root();
	if (!root)
	{
		fatal_boot_error("guildhalls",
				 "flat-file state root unavailable while loading rooms");
		return;
	}
	std::string error;
	std::vector<flatfile_guildhall_record> records;
	const auto listed = flatfile_guildhall_list(root, &records, &error);
	if (listed != flatfile_association_result::ok)
	{
		fatal_boot_error("guildhalls", "could not load flat guildhall rooms: %s",
				 error.c_str());
		return;
	}
	const auto record = std::find_if(records.begin(), records.end(),
					 [guildhall](const auto &entry)
					 { return entry.guildhall_id == guildhall->id; });
	if (record == records.end())
	{
		fatal_boot_error("guildhalls", "flat guildhall %d disappeared while loading rooms",
				 guildhall->id);
		return;
	}
	for (const auto &room : record->rooms)
		materialize_guildhall_room(room, guildhall);
#else
	if (!qry("select id, vnum, guildhall_id, name, type, value0, value1, value2, value3, value4, value5, value6, value7, exit0, exit1, exit2, exit3, exit4, exit5, exit6, exit7, exit8, exit9 from "
		 "guildhall_rooms where guildhall_id = %d order by vnum",
		 guildhall->id))
	{
		logit(LOG_GUILDHALLS, "read_guildhall_rooms(): query failed");
		return;
	}

	MYSQL_RES *res = mysql_store_result(DB);
	if (!res)
	{
		logit(LOG_DEBUG, "%s: mysql_store_result failed", __func__);
		return;
	}

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(res)))
	{
		GuildhallRoom *room = NULL;

		switch (atoi(row[4]))
		{
		case GH_ROOM_TYPE_ENTRANCE:
			room = new EntranceRoom();
			break;
		case GH_ROOM_TYPE_INN:
			room = new InnRoom();
			break;
		case GH_ROOM_TYPE_HEARTSTONE:
			room = new HeartstoneRoom();
			break;
		case GH_ROOM_TYPE_PORTAL:
			room = new PortalRoom();
			break;
		case GH_ROOM_TYPE_WINDOW:
			room = new WindowRoom();
			break;
		case GH_ROOM_TYPE_HEAL:
			room = new HealRoom();
			break;
		case GH_ROOM_TYPE_BANK:
			room = new BankRoom();
			break;
		case GH_ROOM_TYPE_TOWN_PORTAL:
			room = new TownPortalRoom();
			break;
		case GH_ROOM_TYPE_LIBRARY:
			room = new LibraryRoom();
			break;
		case GH_ROOM_TYPE_CARGO:
			room = new CargoRoom();
			break;
		default:
			room = new GuildhallRoom();
		}

		if (!room)
		{
			logit(LOG_GUILDHALLS,
			      "load_guildhall_rooms(): couldn't allocate new guildhallroom!");
			mysql_free_result(res);
			return;
		}

		room->id = atoi(row[0]);
		room->vnum = atoi(row[1]);
		room->assoc_id = guildhall->assoc_id;
		room->guild = guildhall->get_assoc();
		room->name = string(row[3]);
		room->type = atoi(row[4]);

		for (int i = 0; i < GH_ROOM_NUM_VALUES; i++)
		{
			room->value[i] = atoi(row[5 + i]);
		}

		for (int i = 0; i < NUM_EXITS; i++)
		{
			room->exits[i] = atoi(row[5 + GH_ROOM_NUM_VALUES + i]);
		}

		guildhall->add_room(room);
	}

	mysql_free_result(res);
#endif
}

bool save_guildhall(Guildhall *gh)
{
	if (!gh)
	{
		logit(LOG_GUILDHALLS, "save_guildhall(): invalid gh!");
		return FALSE;
	}

	if (gh->outside_vnum < 0)
	{
		logit(LOG_GUILDHALLS, "save_guildhall(): invalid outside_vnum! (%d)",
		      gh->outside_vnum);
		return FALSE;
	}

	if (gh->id <= 0)
	{
		logit(LOG_GUILDHALLS, "save_guildhall(): invalid id! (%d)", gh->id);
		return FALSE;
	}

#ifdef __NO_MYSQL__
	const char *root = persistence_mode_flatfile_root();
	std::string error;
	flatfile_guildhall_record record;
	if (!root || !capture_guildhall(gh, &record, &error))
	{
		logit(LOG_GUILDHALLS, "save_guildhall(%d): %s", gh->id,
		      root ? error.c_str() : "flat-file state root unavailable");
		return FALSE;
	}
	const auto saved = flatfile_guildhall_save(root, record, &error);
	if (!flat_result_succeeded(saved))
	{
		logit(LOG_GUILDHALLS, "save_guildhall(%d): %s", gh->id, error.c_str());
		return FALSE;
	}
	return TRUE;
#else
	if (!qry("replace into guildhalls (id, assoc_id, type, outside_vnum, racewar) values (%d, %d, %d, %d, %d)",
		 gh->id, gh->guild->get_id(), gh->type, gh->outside_vnum, gh->racewar))
	{
		logit(LOG_GUILDHALLS, "save_guildhall(): replace query failed!");
		return FALSE;
	}

	return TRUE;
#endif
}

bool save_guildhall_room(GuildhallRoom *room)
{
	if (!room)
	{
		logit(LOG_GUILDHALLS, "save_guildhall_room(): invalid gh!");
		return FALSE;
	}

	if (room->vnum < 0)
	{
		logit(LOG_GUILDHALLS, "save_guildhall_room(): invalid vnum! (%d)", room->vnum);
		return FALSE;
	}

	if (room->id <= 0)
	{
		logit(LOG_GUILDHALLS, "save_guildhall_room(): invalid id! (%d)", room->id);
		return FALSE;
	}

#ifdef __NO_MYSQL__
	if (!room->guildhall)
	{
		logit(LOG_GUILDHALLS, "save_guildhall_room(%d): missing guildhall", room->id);
		return FALSE;
	}
	const char *root = persistence_mode_flatfile_root();
	std::string error;
	flatfile_guildhall_record record;
	if (!root || !capture_guildhall(room->guildhall, &record, &error))
	{
		logit(LOG_GUILDHALLS, "save_guildhall_room(%d): %s", room->id,
		      root ? error.c_str() : "flat-file state root unavailable");
		return FALSE;
	}
	const auto saved = flatfile_guildhall_save(root, record, &error);
	if (!flat_result_succeeded(saved))
	{
		logit(LOG_GUILDHALLS, "save_guildhall_room(%d): %s", room->id, error.c_str());
		return FALSE;
	}
	return TRUE;
#else
	if (!qry("replace into guildhall_rooms (id, vnum, guildhall_id, name, type, value0, value1, value2, value3, value4, value5, value6, value7, exit0, exit1, exit2, exit3, exit4, exit5, exit6, "
		 "exit7, exit8, exit9) values (%d, %d, %d, '%s', %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)",
		 room->id, room->vnum, room->guildhall->id, escape_str(room->name.c_str()).c_str(),
		 room->type, room->value[0], room->value[1], room->value[2], room->value[3],
		 room->value[4], room->value[5], room->value[6], room->value[7], room->exits[0],
		 room->exits[1], room->exits[2], room->exits[3], room->exits[4], room->exits[5],
		 room->exits[6], room->exits[7], room->exits[8], room->exits[9]))
	{
		logit(LOG_GUILDHALLS, "save_guildhall_room(): replace query failed!");
		return FALSE;
	}

	return TRUE;
#endif
}

bool delete_guildhall(Guildhall *gh)
{
	if (!gh)
	{
		logit(LOG_GUILDHALLS, "delete_guildhall(): invalid gh!");
		return FALSE;
	}

	if (gh->id <= 0)
	{
		logit(LOG_GUILDHALLS, "delete_guildhall(): invalid id! (%d)", gh->id);
		return FALSE;
	}

#ifdef __NO_MYSQL__
	const char *root = persistence_mode_flatfile_root();
	std::string error;
	const auto erased = root ? flatfile_guildhall_erase(root, gh->id, &error) :
				   flatfile_association_result::io_error;
	if (!flat_result_succeeded(erased))
	{
		logit(LOG_GUILDHALLS, "delete_guildhall(%d): %s", gh->id,
		      root ? error.c_str() : "flat-file state root unavailable");
		return FALSE;
	}
	return TRUE;
#else
	if (!qry("delete from guildhalls where id = %d", gh->id))
	{
		logit(LOG_GUILDHALLS, "delete_guildhall(): delete query failed!");
		return FALSE;
	}

	return TRUE;
#endif
}

bool delete_guildhall_room(GuildhallRoom *room)
{
	if (!room)
	{
		logit(LOG_GUILDHALLS, "delete_guildhall_room(): invalid room!");
		return FALSE;
	}

	if (room->id <= 0)
	{
		logit(LOG_GUILDHALLS, "delete_guildhall_room(): invalid id! (%d)", room->id);
		return FALSE;
	}

#ifdef __NO_MYSQL__
	if (!room->guildhall)
	{
		logit(LOG_GUILDHALLS, "delete_guildhall_room(%d): missing guildhall", room->id);
		return FALSE;
	}
	const char *root = persistence_mode_flatfile_root();
	std::string error;
	const auto erased =
		root ? flatfile_guildhall_room_erase(root, room->guildhall->id, room->id, &error) :
		       flatfile_association_result::io_error;
	if (!flat_result_succeeded(erased))
	{
		logit(LOG_GUILDHALLS, "delete_guildhall_room(%d): %s", room->id,
		      root ? error.c_str() : "flat-file state root unavailable");
		return FALSE;
	}
	return TRUE;
#else
	if (!qry("delete from guildhall_rooms where id = %d", room->id))
	{
		logit(LOG_GUILDHALLS, "delete_guildhall_room(): delete query failed!");
		return FALSE;
	}

	return TRUE;
#endif
}
